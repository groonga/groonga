/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* Groonga::ChildContext and Groonga::ChildObject: a proxy
 * mechanism to run Ruby code in another grn_ctx's mruby.
 *
 * Each grn_ctx has its own mrb_state. Groonga::ChildContext pulls
 * a child grn_ctx and initializes its mruby. Ruby code in the current
 * mrb_state can load code in the child's mruby by
 * Groonga::ChildContext#require. A value that can't be copied between
 * mrb_states is wrapped as a Groonga::ChildObject proxy. A method
 * call that isn't defined in Groonga::ChildObject itself is forwarded
 * to the child's mruby via method_missing. Note that methods that are
 * inherited from Object such as inspect and == aren't forwarded.
 *
 * All accesses to the child's mruby are serialized by a per context
 * mutex. This is a preparation for executing a forwarded method call
 * as a grn::TaskExecutor task on a worker thread. In this synchronous
 * version all calls are run on the caller's thread.
 *
 * Values that can be copied between mrb_states: nil, true, false,
 * Integer, Float, Symbol, String, Array and Hash. A block/Proc can't
 * be passed.
 */

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#  include "../grn_ctx_impl_mrb.h"

#  include <mruby.h>
#  include <mruby/array.h>
#  include <mruby/class.h>
#  include <mruby/data.h>
#  include <mruby/error.h>
#  include <mruby/hash.h>
#  include <mruby/string.h>
#  include <mruby/variable.h>

#  include "mrb_child_context.h"

#  include <mutex>
#  include <vector>

namespace {
  constexpr int max_convert_depth = 32;

  struct ChildContextData {
    grn_ctx *ctx;
    /* nullptr after Groonga::ChildContext#release */
    grn_ctx *child_ctx;
    std::mutex mutex;
    /* Values in the child's mruby that are wrapped as
     * Groonga::ChildObject. They are kept by mrb_gc_register()
     * against the child's mruby until #release. */
    std::vector<mrb_value> retained;

    ChildContextData(grn_ctx *ctx)
      : ctx(ctx),
        child_ctx(nullptr),
        mutex(),
        retained()
    {
    }
  };

  struct ChildObjectData {
    /* Borrowed. The proxy keeps its Groonga::ChildContext alive
     * via the @context instance variable. */
    ChildContextData *context_data;
    mrb_value child_value;
  };

  mrb_state *
  child_mrb(ChildContextData *data)
  {
    return data->child_ctx->impl->mrb.state;
  }

  /* The caller must ensure that nobody is using the child's mruby:
   * the mutex doesn't help here because a queued but not started task
   * isn't blocked by it. The asynchronous version must wait all tasks
   * of this context before this. */
  void
  child_context_release(mrb_state *mrb, ChildContextData *data)
  {
    if (!data->child_ctx) {
      return;
    }
    auto mrb_child = child_mrb(data);
    for (const auto &retained_value : data->retained) {
      mrb_gc_unregister(mrb_child, retained_value);
    }
    data->retained.clear();
    grn_ctx_release_child(data->ctx, data->child_ctx);
    data->child_ctx = nullptr;
  }

  void
  child_context_free(mrb_state *mrb, void *data)
  {
    auto context_data = static_cast<ChildContextData *>(data);
    child_context_release(mrb, context_data);
    delete context_data;
  }

  void
  child_object_free(mrb_state *mrb, void *data)
  {
    /* The wrapped value in the child's mruby is released at
     * Groonga::ChildContext#release not here. Unregistering it here
     * requires the context mutex in GC that may cause a long GC
     * pause. */
    delete static_cast<ChildObjectData *>(data);
  }

  const mrb_data_type mrb_grn_child_context_type = {
    "Groonga::ChildContext",
    child_context_free,
  };

  const mrb_data_type mrb_grn_child_object_type = {
    "Groonga::ChildObject",
    child_object_free,
  };

  ChildContextData *
  child_context_data(mrb_state *mrb, mrb_value mrb_context)
  {
    auto data = static_cast<ChildContextData *>(
      mrb_data_get_ptr(mrb, mrb_context, &mrb_grn_child_context_type));
    if (!data->child_ctx) {
      mrb_raise(mrb,
                mrb_class_get(mrb, "RuntimeError"),
                "[child-context] already released");
    }
    return data;
  }

  void
  retain_child_value(ChildContextData *data, mrb_value child_value)
  {
    if (mrb_immediate_p(child_value)) {
      return;
    }
    mrb_gc_register(child_mrb(data), child_value);
    data->retained.push_back(child_value);
  }

  /* Convert a value in the caller's mruby to a value in the child's
   * mruby. A value that can't be converted raises ArgumentError in
   * the caller's mruby except a Groonga::ChildObject of the same
   * context that is unwrapped to the value in the child's mruby. */
  mrb_value
  value_to_child(mrb_state *mrb,
                 ChildContextData *data,
                 mrb_value value,
                 int depth = 0)
  {
    if (depth > max_convert_depth) {
      mrb_raise(mrb,
                mrb_class_get(mrb, "ArgumentError"),
                "[child-object] too deep value");
    }

    auto mrb_child = child_mrb(data);
    switch (mrb_type(value)) {
    case MRB_TT_FALSE:
      return mrb_nil_p(value) ? mrb_nil_value() : mrb_false_value();
    case MRB_TT_TRUE:
      return mrb_true_value();
    case MRB_TT_INTEGER:
      return mrb_int_value(mrb_child, mrb_integer(value));
    case MRB_TT_FLOAT:
      return mrb_float_value(mrb_child, mrb_float(value));
    case MRB_TT_SYMBOL:
      {
        mrb_int length;
        auto name = mrb_sym2name_len(mrb, mrb_symbol(value), &length);
        return mrb_symbol_value(mrb_intern(mrb_child, name, length));
      }
    case MRB_TT_STRING:
      return mrb_str_new(mrb_child, RSTRING_PTR(value), RSTRING_LEN(value));
    case MRB_TT_ARRAY:
      {
        auto length = RARRAY_LEN(value);
        auto converted = mrb_ary_new_capa(mrb_child, length);
        for (mrb_int i = 0; i < length; ++i) {
          auto arena_index = mrb_gc_arena_save(mrb_child);
          mrb_ary_push(
            mrb_child,
            converted,
            value_to_child(mrb, data, mrb_ary_entry(value, i), depth + 1));
          mrb_gc_arena_restore(mrb_child, arena_index);
        }
        return converted;
      }
    case MRB_TT_HASH:
      {
        auto converted = mrb_hash_new(mrb_child);
        auto keys = mrb_hash_keys(mrb, value);
        auto length = RARRAY_LEN(keys);
        for (mrb_int i = 0; i < length; ++i) {
          auto key = mrb_ary_entry(keys, i);
          auto arena_index = mrb_gc_arena_save(mrb_child);
          mrb_hash_set(mrb_child,
                       converted,
                       value_to_child(mrb, data, key, depth + 1),
                       value_to_child(mrb,
                                      data,
                                      mrb_hash_get(mrb, value, key),
                                      depth + 1));
          mrb_gc_arena_restore(mrb_child, arena_index);
        }
        return converted;
      }
    case MRB_TT_DATA:
      if (DATA_TYPE(value) == &mrb_grn_child_object_type) {
        auto object_data = static_cast<ChildObjectData *>(DATA_PTR(value));
        if (object_data->context_data == data) {
          return object_data->child_value;
        }
        mrb_raise(mrb,
                  mrb_class_get(mrb, "ArgumentError"),
                  "[child-object] "
                  "can't pass Groonga::ChildObject of another context");
      }
      break;
    default:
      break;
    }

    mrb_raisef(mrb,
               mrb_class_get(mrb, "ArgumentError"),
               "[child-object] can't pass %S",
               mrb_str_new_cstr(mrb, mrb_obj_classname(mrb, value)));
  }

  /* Convert a value in the child's mruby to a value in the caller's
   * mruby. A value that can't be converted is wrapped as a
   * Groonga::ChildObject. */
  mrb_value
  value_from_child(mrb_state *mrb,
                   ChildContextData *data,
                   mrb_value mrb_context,
                   mrb_value value,
                   int depth = 0)
  {
    if (depth > max_convert_depth) {
      mrb_raise(mrb,
                mrb_class_get(mrb, "ArgumentError"),
                "[child-object] too deep value");
    }

    auto mrb_child = child_mrb(data);
    switch (mrb_type(value)) {
    case MRB_TT_FALSE:
      return mrb_nil_p(value) ? mrb_nil_value() : mrb_false_value();
    case MRB_TT_TRUE:
      return mrb_true_value();
    case MRB_TT_INTEGER:
      return mrb_int_value(mrb, mrb_integer(value));
    case MRB_TT_FLOAT:
      return mrb_float_value(mrb, mrb_float(value));
    case MRB_TT_SYMBOL:
      {
        mrb_int length;
        auto name = mrb_sym2name_len(mrb_child, mrb_symbol(value), &length);
        return mrb_symbol_value(mrb_intern(mrb, name, length));
      }
    case MRB_TT_STRING:
      return mrb_str_new(mrb, RSTRING_PTR(value), RSTRING_LEN(value));
    case MRB_TT_ARRAY:
      {
        auto length = RARRAY_LEN(value);
        auto converted = mrb_ary_new_capa(mrb, length);
        for (mrb_int i = 0; i < length; ++i) {
          auto arena_index = mrb_gc_arena_save(mrb);
          mrb_ary_push(mrb,
                       converted,
                       value_from_child(mrb,
                                        data,
                                        mrb_context,
                                        mrb_ary_entry(value, i),
                                        depth + 1));
          mrb_gc_arena_restore(mrb, arena_index);
        }
        return converted;
      }
    case MRB_TT_HASH:
      {
        auto converted = mrb_hash_new(mrb);
        auto keys = mrb_hash_keys(mrb_child, value);
        auto length = RARRAY_LEN(keys);
        for (mrb_int i = 0; i < length; ++i) {
          auto key = mrb_ary_entry(keys, i);
          auto arena_index = mrb_gc_arena_save(mrb);
          mrb_hash_set(mrb,
                       converted,
                       value_from_child(mrb, data, mrb_context, key, depth + 1),
                       value_from_child(mrb,
                                        data,
                                        mrb_context,
                                        mrb_hash_get(mrb_child, value, key),
                                        depth + 1));
          mrb_gc_arena_restore(mrb, arena_index);
        }
        return converted;
      }
    default:
      break;
    }

    retain_child_value(data, value);
    auto ctx = static_cast<grn_ctx *>(mrb->ud);
    auto klass = mrb_class_get_under(mrb, ctx->impl->mrb.module, "ChildObject");
    auto mrb_object = mrb_obj_new(mrb, klass, 1, &mrb_context);
    auto object_data = static_cast<ChildObjectData *>(DATA_PTR(mrb_object));
    object_data->child_value = value;
    return mrb_object;
  }

  struct ChildCall {
    mrb_value receiver;
    mrb_sym name;
    mrb_int n_arguments;
    const mrb_value *arguments;
  };

  mrb_value
  child_call_body(mrb_state *mrb, void *user_data)
  {
    auto call = static_cast<ChildCall *>(user_data);
    return mrb_funcall_argv(mrb,
                            call->receiver,
                            call->name,
                            call->n_arguments,
                            call->arguments);
  }

  mrb_value
  child_to_s_body(mrb_state *mrb, void *user_data)
  {
    return mrb_obj_as_string(mrb, *static_cast<mrb_value *>(user_data));
  }

  /* Run `body` in the child's mruby under the context mutex. On
   * error, the child's exception is re-raised in the caller's mruby
   * as RuntimeError.
   *
   * The returned value is a value in the child's mruby that is
   * protected in the child's arena. */
  mrb_value
  run_in_child(mrb_state *mrb,
               ChildContextData *data,
               mrb_protect_error_func *body,
               void *user_data)
  {
    char error_message[GRN_CTX_MSGSIZE];
    mrb_bool error = FALSE;
    mrb_value result;
    {
      std::lock_guard<std::mutex> lock(data->mutex);
      auto mrb_child = child_mrb(data);
      result = mrb_protect_error(mrb_child, body, user_data, &error);
      if (error) {
        auto class_name = mrb_obj_classname(mrb_child, result);
        mrb_bool message_error = FALSE;
        auto message = mrb_protect_error(mrb_child,
                                         child_to_s_body,
                                         &result,
                                         &message_error);
        if (message_error || mrb_type(message) != MRB_TT_STRING) {
          grn_snprintf(error_message,
                       GRN_CTX_MSGSIZE,
                       GRN_CTX_MSGSIZE,
                       "[child-object] %s",
                       class_name);
        } else {
          grn_snprintf(error_message,
                       GRN_CTX_MSGSIZE,
                       GRN_CTX_MSGSIZE,
                       "[child-object] %s: %.*s",
                       class_name,
                       static_cast<int>(RSTRING_LEN(message)),
                       RSTRING_PTR(message));
        }
      }
    }
    if (error) {
      /* Raise after unlocking the mutex. mruby's raise is longjmp()
       * that skips C++ destructors such as std::lock_guard's one. */
      mrb_raise(mrb, mrb_class_get(mrb, "RuntimeError"), error_message);
    }
    return result;
  }

  /* Run `caller_body` under mrb_protect_error() against the caller's
   * mruby with surrounding mrb_gc_arena_save()/restore() of the
   * child's mruby then re-raise a raised exception.
   *
   * All values allocated in the child's mruby by `caller_body` such
   * as translated arguments and a return value of run_in_child() are
   * kept in the child's arena until the mrb_gc_arena_restore()
   * here. Nobody shrinks the child's arena for allocations done
   * outside of the child's VM execution. Without this, each call
   * grows the child's arena and the arena overflows eventually.
   *
   * `caller_body` may raise in the caller's mruby: run_in_child() for
   * an error in the child and value_to_child()/value_from_child() for
   * a value that can't be converted. mrb_protect_error() is used to run
   * mrb_gc_arena_restore() even for these cases because a raise is
   * longjmp() that skips the rest of this function without it. */
  mrb_value
  run_with_child_arena(mrb_state *mrb,
                       ChildContextData *data,
                       mrb_protect_error_func *caller_body,
                       void *user_data)
  {
    auto mrb_child = child_mrb(data);
    auto child_arena_index = mrb_gc_arena_save(mrb_child);
    mrb_bool error = FALSE;
    auto result = mrb_protect_error(mrb, caller_body, user_data, &error);
    mrb_gc_arena_restore(mrb_child, child_arena_index);
    if (error) {
      mrb_exc_raise(mrb, result);
    }
    return result;
  }

  mrb_value
  child_context_initialize(mrb_state *mrb, mrb_value self)
  {
    auto ctx = static_cast<grn_ctx *>(mrb->ud);
    auto data = new ChildContextData(ctx);
    mrb_data_init(self, data, &mrb_grn_child_context_type);
    data->child_ctx = grn_ctx_pull_child(ctx);
    if (!data->child_ctx) {
      mrb_raise(mrb,
                mrb_class_get(mrb, "RuntimeError"),
                "[child-context] failed to pull a child context");
    }
    grn_ctx_impl_mrb_ensure_init(data->child_ctx);
    if (data->child_ctx->rc != GRN_SUCCESS ||
        !data->child_ctx->impl->mrb.state) {
      grn_ctx_release_child(ctx, data->child_ctx);
      data->child_ctx = nullptr;
      mrb_raise(mrb,
                mrb_class_get(mrb, "RuntimeError"),
                "[child-context] "
                "failed to initialize mruby for a child context");
    }
    return self;
  }

  struct RequireData {
    ChildContextData *data;
    mrb_value mrb_context;
    const char *name;
    mrb_int length;
  };

  mrb_value
  child_context_require_caller_body(mrb_state *mrb, void *user_data)
  {
    auto require_data = static_cast<RequireData *>(user_data);
    auto data = require_data->data;
    auto mrb_child = child_mrb(data);
    auto child_name =
      mrb_str_new(mrb_child, require_data->name, require_data->length);
    ChildCall call;
    call.receiver = mrb_top_self(mrb_child);
    call.name = mrb_intern_lit(mrb_child, "require");
    call.n_arguments = 1;
    call.arguments = &child_name;
    auto child_result = run_in_child(mrb, data, child_call_body, &call);
    return value_from_child(mrb, data, require_data->mrb_context, child_result);
  }

  mrb_value
  child_context_require(mrb_state *mrb, mrb_value self)
  {
    char *name;
    mrb_int length;
    mrb_get_args(mrb, "s", &name, &length);

    auto data = child_context_data(mrb, self);
    RequireData require_data = {data, self, name, length};
    return run_with_child_arena(mrb,
                                data,
                                child_context_require_caller_body,
                                &require_data);
  }

  mrb_value
  child_context_array_reference_body(mrb_state *mrb, void *user_data)
  {
    auto mrb_id_or_name = static_cast<mrb_value *>(user_data);
    auto ctx = static_cast<grn_ctx *>(mrb->ud);
    auto context_class =
      mrb_class_get_under(mrb, ctx->impl->mrb.module, "Context");
    auto context = mrb_funcall_argv(mrb,
                                    mrb_obj_value(context_class),
                                    mrb_intern_lit(mrb, "instance"),
                                    0,
                                    nullptr);
    return mrb_funcall_argv(mrb,
                            context,
                            mrb_intern_lit(mrb, "[]"),
                            1,
                            mrb_id_or_name);
  }

  struct ArrayReferenceData {
    ChildContextData *data;
    mrb_value mrb_context;
    mrb_value mrb_id_or_name;
  };

  mrb_value
  child_context_array_reference_caller_body(mrb_state *mrb, void *user_data)
  {
    auto reference_data = static_cast<ArrayReferenceData *>(user_data);
    auto data = reference_data->data;
    auto child_id_or_name =
      value_to_child(mrb, data, reference_data->mrb_id_or_name);
    auto child_result = run_in_child(mrb,
                                     data,
                                     child_context_array_reference_body,
                                     &child_id_or_name);
    return value_from_child(mrb,
                            data,
                            reference_data->mrb_context,
                            child_result);
  }

  mrb_value
  child_context_array_reference(mrb_state *mrb, mrb_value self)
  {
    mrb_value mrb_id_or_name;
    mrb_get_args(mrb, "o", &mrb_id_or_name);

    auto data = child_context_data(mrb, self);
    ArrayReferenceData reference_data = {data, self, mrb_id_or_name};
    return run_with_child_arena(mrb,
                                data,
                                child_context_array_reference_caller_body,
                                &reference_data);
  }

  mrb_value
  child_context_release(mrb_state *mrb, mrb_value self)
  {
    auto data = static_cast<ChildContextData *>(
      mrb_data_get_ptr(mrb, self, &mrb_grn_child_context_type));
    child_context_release(mrb, data);
    return mrb_nil_value();
  }

  mrb_value
  child_context_is_released(mrb_state *mrb, mrb_value self)
  {
    auto data = static_cast<ChildContextData *>(
      mrb_data_get_ptr(mrb, self, &mrb_grn_child_context_type));
    return mrb_bool_value(!data->child_ctx);
  }

  mrb_value
  child_object_initialize(mrb_state *mrb, mrb_value self)
  {
    mrb_value mrb_context;
    mrb_get_args(mrb, "o", &mrb_context);

    /* This also validates that mrb_context is a Groonga::ChildContext
     * that isn't released. */
    auto data = child_context_data(mrb, mrb_context);
    auto object_data = new ChildObjectData{data, mrb_nil_value()};
    mrb_data_init(self, object_data, &mrb_grn_child_object_type);
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@context"), mrb_context);
    return self;
  }

  /* The same limit as mruby's mrb_funcall() that also uses a fixed
   * size arguments buffer on stack. This is configurable by
   * mrbconf.h. The default value is in mruby's src/vm.c. */
#  ifndef MRB_FUNCALL_ARGC_MAX
#    define MRB_FUNCALL_ARGC_MAX 16
#  endif
  constexpr mrb_int max_call_arguments = MRB_FUNCALL_ARGC_MAX;

  struct MethodMissingData {
    ChildContextData *data;
    mrb_value mrb_context;
    mrb_value receiver;
    mrb_sym name;
    mrb_value *mrb_arguments;
    mrb_int n_arguments;
    /* Not std::vector because a raise in
     * child_object_method_missing_caller_body() is longjmp() that skips C++
     * destructors. */
    mrb_value child_arguments[max_call_arguments];
  };

  mrb_value
  child_object_method_missing_caller_body(mrb_state *mrb, void *user_data)
  {
    auto method_missing_data = static_cast<MethodMissingData *>(user_data);
    auto data = method_missing_data->data;
    auto mrb_child = child_mrb(data);
    mrb_int name_length;
    auto name = mrb_sym2name_len(mrb, method_missing_data->name, &name_length);
    ChildCall call;
    call.receiver = method_missing_data->receiver;
    call.name = mrb_intern(mrb_child, name, name_length);
    call.n_arguments = method_missing_data->n_arguments;
    for (mrb_int i = 0; i < method_missing_data->n_arguments; ++i) {
      method_missing_data->child_arguments[i] =
        value_to_child(mrb, data, method_missing_data->mrb_arguments[i]);
    }
    call.arguments = method_missing_data->child_arguments;
    auto child_result = run_in_child(mrb, data, child_call_body, &call);
    return value_from_child(mrb,
                            data,
                            method_missing_data->mrb_context,
                            child_result);
  }

  mrb_value
  child_object_method_missing(mrb_state *mrb, mrb_value self)
  {
    mrb_sym name;
    mrb_value *mrb_arguments;
    mrb_int n_arguments;
    mrb_get_args(mrb, "n*", &name, &mrb_arguments, &n_arguments);
    if (n_arguments > max_call_arguments) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "[child-object] too many arguments: %S (max: %S)",
                 mrb_int_value(mrb, n_arguments),
                 mrb_int_value(mrb, max_call_arguments));
    }

    auto object_data = static_cast<ChildObjectData *>(
      mrb_data_get_ptr(mrb, self, &mrb_grn_child_object_type));
    auto mrb_context = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@context"));
    auto data = child_context_data(mrb, mrb_context);
    MethodMissingData method_missing_data;
    method_missing_data.data = data;
    method_missing_data.mrb_context = mrb_context;
    method_missing_data.receiver = object_data->child_value;
    method_missing_data.name = name;
    method_missing_data.mrb_arguments = mrb_arguments;
    method_missing_data.n_arguments = n_arguments;
    return run_with_child_arena(mrb,
                                data,
                                child_object_method_missing_caller_body,
                                &method_missing_data);
  }
} // namespace

extern "C" void
grn_mrb_child_context_init(grn_ctx *ctx)
{
  auto mrb = ctx->impl->mrb.state;
  auto module = ctx->impl->mrb.module;

  auto context_class =
    mrb_define_class_under(mrb, module, "ChildContext", mrb->object_class);
  MRB_SET_INSTANCE_TT(context_class, MRB_TT_DATA);
  mrb_define_method(mrb,
                    context_class,
                    "initialize",
                    child_context_initialize,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb,
                    context_class,
                    "require",
                    child_context_require,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb,
                    context_class,
                    "[]",
                    child_context_array_reference,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb,
                    context_class,
                    "release",
                    child_context_release,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb,
                    context_class,
                    "released?",
                    child_context_is_released,
                    MRB_ARGS_NONE());

  auto object_class =
    mrb_define_class_under(mrb, module, "ChildObject", mrb->object_class);
  MRB_SET_INSTANCE_TT(object_class, MRB_TT_DATA);
  mrb_define_method(mrb,
                    object_class,
                    "initialize",
                    child_object_initialize,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb,
                    object_class,
                    "method_missing",
                    child_object_method_missing,
                    MRB_ARGS_REQ(1) | MRB_ARGS_REST());
}
#endif
