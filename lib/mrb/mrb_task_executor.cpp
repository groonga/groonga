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

/* Groonga::TaskExecutor: mruby bindings of grn::TaskExecutor.
 *
 * Groonga::TaskExecutor#execute evaluates a block in a child grn_ctx
 * as a grn::TaskExecutor task. Each grn_ctx has its own mrb_state
 * and mruby isn't thread safe. So nothing in the caller's mrb_state
 * is shared with the child's mrb_state:
 *
 *   * The block is serialized to mruby bytecode by mrb_dump_irep()
 *     in the caller's mrb_state and deserialized by
 *     mrb_load_irep_buf_cxt() in the child's mrb_state. The block is
 *     evaluated at the top level of the child's mrb_state without
 *     its environment. So local variables outside the block are nil
 *     in the block and lexical scopes of the block such as enclosing
 *     modules aren't reproduced.
 *   * Arguments and the result of the block are copied via
 *     grn::mrb::DetachedValue.
 *
 * The caller's mrb_state is touched only by the caller's thread. A
 * worker thread pulls a child grn_ctx, evaluates the block in it and
 * releases the child grn_ctx. So the number of child grn_ctxs that
 * are used at the same time doesn't exceed the number of workers.
 *
 * Temporary objects created in a child grn_ctx are registered to the
 * root grn_ctx not the child grn_ctx. So a block can return an ID of
 * a temporary table and the caller can use it after the child
 * grn_ctx is released. */

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#  include "../grn_ctx.hpp"
#  include "../grn_ctx_impl_mrb.h"

#  include <mruby.h>
#  include <mruby/array.h>
#  include <mruby/class.h>
#  include <mruby/compile.h>
#  include <mruby/data.h>
#  include <mruby/dump.h>
#  include <mruby/error.h>
#  include <mruby/hash.h>
#  include <mruby/irep.h>
#  include <mruby/proc.h>
#  include <mruby/string.h>

#  include "mrb_ctx.h"
#  include "mrb_detached_value.hpp"
#  include "mrb_task_executor.h"

#  include <memory>
#  include <string>
#  include <unordered_map>
#  include <vector>

namespace {
  using grn::mrb::DetachedValue;

  struct Task {
    uintptr_t id;
    std::string tag;
    /* Serialized block */
    std::string irep;
    std::vector<DetachedValue> arguments;
    /* Written by a worker thread. Read by the caller's thread after
     * the task is waited. */
    DetachedValue result;
    /* Error of the task. GRN_SUCCESS if the task is
     * succeeded. Written by a worker thread. Read by the caller's
     * thread after the task is waited. */
    grn_rc rc;
    std::string error_message;
    /* Where the error is reported. Static strings and a line number
     * like grn_ctx's errfile, errline and errfunc. */
    const char *error_file;
    unsigned int error_line;
    const char *error_function;
  };

  struct TaskExecutorData {
    /* The caller's context */
    grn_ctx *ctx;
    grn::TaskExecutor *executor;
    /* Tasks that aren't waited yet. Workers keep pointers to
     * tasks. So tasks aren't moved until they are waited. */
    std::unordered_map<uintptr_t, std::unique_ptr<Task>> tasks;
    /* IDs of tasks in execute order. */
    std::vector<uintptr_t> task_ids;
  };

  void
  task_executor_free(mrb_state *mrb, void *data)
  {
    auto executor_data = static_cast<TaskExecutorData *>(data);
    if (!executor_data->tasks.empty()) {
      /* Workers may be using tasks. */
      executor_data->executor->wait_all();
    }
    delete executor_data;
  }

  const mrb_data_type mrb_grn_task_executor_type = {
    "Groonga::TaskExecutor",
    task_executor_free,
  };

  TaskExecutorData *
  task_executor_data(mrb_state *mrb, mrb_value self)
  {
    return static_cast<TaskExecutorData *>(
      mrb_data_get_ptr(mrb, self, &mrb_grn_task_executor_type));
  }

  mrb_value
  task_executor_initialize(mrb_state *mrb, mrb_value self)
  {
    auto ctx = static_cast<grn_ctx *>(mrb->ud);
    auto data = new TaskExecutorData();
    data->ctx = ctx;
    data->executor = grn_ctx_get_task_executor(ctx);
    mrb_data_init(self, data, &mrb_grn_task_executor_type);
    return self;
  }

  mrb_value
  task_executor_get_n_workers(mrb_state *mrb, mrb_value self)
  {
    auto data = task_executor_data(mrb, self);
    return mrb_int_value(mrb, data->executor->get_n_workers());
  }

  mrb_value
  task_executor_is_parallel(mrb_state *mrb, mrb_value self)
  {
    auto data = task_executor_data(mrb, self);
    return mrb_bool_value(data->executor->is_parallel());
  }

  /* Run in the child's mruby on a worker thread. The task is
   * reported as an error by raising. */
  mrb_value
  run_task_body(mrb_state *mrb, void *user_data)
  {
    auto task = static_cast<Task *>(user_data);

    auto compile_context = mrb_ccontext_new(mrb);
    compile_context->no_exec = TRUE;
    auto mrb_proc = mrb_load_irep_buf_cxt(mrb,
                                          task->irep.data(),
                                          task->irep.size(),
                                          compile_context);
    mrb_ccontext_free(mrb, compile_context);
    if (mrb->exc) {
      auto exception = mrb_obj_value(mrb->exc);
      mrb->exc = nullptr;
      mrb_exc_raise(mrb, exception);
    }
    if (!mrb_proc_p(mrb_proc)) {
      mrb_raise(mrb,
                mrb_class_get(mrb, "RuntimeError"),
                "failed to deserialize the block");
    }

    /* The block is evaluated at the top level. Constants are resolved
     * from Object. */
    auto self = mrb_top_self(mrb);
    auto klass = mrb->object_class;

    /* An Array in mruby is used instead of std::vector because a raise
     * is longjmp() that skips C++ destructors. */
    auto n_arguments = static_cast<mrb_int>(task->arguments.size());
    auto mrb_arguments = mrb_ary_new_capa(mrb, n_arguments);
    for (const auto &argument : task->arguments) {
      auto arena_index = mrb_gc_arena_save(mrb);
      mrb_ary_push(mrb, mrb_arguments, argument.to_mrb(mrb));
      mrb_gc_arena_restore(mrb, arena_index);
    }

    auto result = mrb_yield_with_class(mrb,
                                       mrb_proc,
                                       n_arguments,
                                       RARRAY_PTR(mrb_arguments),
                                       self,
                                       klass);
    const char *error_class_name = nullptr;
    if (!DetachedValue::from_mrb(mrb,
                                 result,
                                 task->result,
                                 &error_class_name)) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "can't return %S from the block",
                 mrb_str_new_cstr(mrb, error_class_name));
    }
    return mrb_nil_value();
  }

  mrb_value
  to_s_body(mrb_state *mrb, void *user_data)
  {
    return mrb_obj_as_string(mrb, *static_cast<mrb_value *>(user_data));
  }

  /* Keep the error of the child context in the task. It's used to
   * report the error to the caller's context. */
  void
  record_task_error(Task *task, grn_ctx *ctx)
  {
    task->rc = ctx->rc;
    task->error_message = ctx->errbuf;
    task->error_file = ctx->errfile;
    task->error_line = ctx->errline;
    task->error_function = ctx->errfunc;
  }

  /* Run on a worker thread. This must not touch the caller's
   * mruby. An error is reported via the child grn_ctx and kept in
   * the task. */
  bool
  run_task(grn_ctx *parent_ctx, Task *task)
  {
    auto child_ctx = grn_ctx_pull_child(parent_ctx);
    if (!child_ctx) {
      task->rc = GRN_NO_MEMORY_AVAILABLE;
      task->error_message = task->tag + "[" + std::to_string(task->id) + "] " +
                            "failed to pull a child context";
      task->error_file = __FILE__;
      task->error_line = __LINE__;
      task->error_function = __FUNCTION__;
      GRN_LOG(parent_ctx, GRN_LOG_ERROR, "%s", task->error_message.c_str());
      return false;
    }
    grn::ChildCtxReleaser releaser(parent_ctx, child_ctx);

    /* All Groonga APIs called in the task must be run in the API
     * scope of the child context. Otherwise, an error set in the
     * child context is cleared by a following Groonga API call
     * because GRN_API_ENTER clears the error at the top level. */
    auto ctx = child_ctx;
    GRN_API_ENTER;
    grn_ctx_impl_mrb_ensure_init(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      record_task_error(task, ctx);
      GRN_API_RETURN(false);
    }
    auto mrb = ctx->impl->mrb.state;
    if (!mrb) {
      ERR(GRN_UNKNOWN_ERROR,
          "%s[%llu] mruby is disabled",
          task->tag.c_str(),
          static_cast<unsigned long long>(task->id));
      record_task_error(task, ctx);
      GRN_API_RETURN(false);
    }

    auto arena_index = mrb_gc_arena_save(mrb);
    mrb_bool error = FALSE;
    auto exception = mrb_protect_error(mrb, run_task_body, task, &error);
    if (error) {
      if (ctx->rc == GRN_SUCCESS) {
        auto class_name = mrb_obj_classname(mrb, exception);
        mrb_bool message_error = FALSE;
        auto message =
          mrb_protect_error(mrb, to_s_body, &exception, &message_error);
        if (message_error || mrb_type(message) != MRB_TT_STRING) {
          ERR(GRN_UNKNOWN_ERROR,
              "%s[%llu] %s",
              task->tag.c_str(),
              static_cast<unsigned long long>(task->id),
              class_name);
        } else {
          ERR(GRN_UNKNOWN_ERROR,
              "%s[%llu] %s: %.*s",
              task->tag.c_str(),
              static_cast<unsigned long long>(task->id),
              class_name,
              static_cast<int>(RSTRING_LEN(message)),
              RSTRING_PTR(message));
        }
      }
      record_task_error(task, ctx);
    }
    mrb_gc_arena_restore(mrb, arena_index);
    GRN_API_RETURN(!error);
  }

  /* Report the first error of tasks to the caller's context if the
   * error isn't reported yet. An error in a child context is
   * normally propagated by grn_ctx_release_child() but the error may
   * be lost. The error message is already logged by the task. So
   * this doesn't log it again. This must be run on the caller's
   * thread after all tasks are finished. */
  void
  report_task_errors(TaskExecutorData *data)
  {
    auto ctx = data->ctx;
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
    for (const auto id : data->task_ids) {
      auto it = data->tasks.find(id);
      if (it == data->tasks.end()) {
        continue;
      }
      auto task = it->second.get();
      if (task->rc == GRN_SUCCESS) {
        continue;
      }
      ctx->rc = task->rc;
      ctx->errlvl = GRN_LOG_ERROR;
      ctx->errfile = task->error_file;
      ctx->errline = task->error_line;
      ctx->errfunc = task->error_function;
      grn_strcpy(ctx->errbuf, GRN_CTX_MSGSIZE, task->error_message.c_str());
      return;
    }
  }

  /* Groonga::TaskExecutor#execute(id, tag, *arguments, &block)
   *
   * Evaluates the given block with the given arguments in a child
   * context as a task. The task is run on a worker thread if this
   * executor is parallel.
   *
   * Each child context has its own mruby VM. So the block is
   * serialized to bytecode here and deserialized in the child
   * context's mruby VM. This has some restrictions:
   *
   *   * The block must not refer any local variable outside the
   *     block. Pass them as arguments instead. This isn't validated:
   *     a local variable outside the block is silently nil in the
   *     child context because the block is evaluated without its
   *     environment. `yield` and `super` don't work for the same
   *     reason.
   *   * The block is evaluated at the top level: `self` is the top
   *     level object and constants are resolved from Object. Use
   *     full names such as `Groonga::Sharding::Parameters` even if
   *     the block is defined in a class or a module. `require` can
   *     be used in the block to load scripts in the child context.
   *   * Arguments and the result of the block must be nil, true,
   *     false, Integer, Float, Symbol, String, Array or Hash. They
   *     are copied. Temporary Groonga objects can be passed by ID.
   *
   * `id` is an Integer that identifies the task. It's used as a key
   * of the result of #wait_all. `tag` is used for error messages. */
  mrb_value
  task_executor_execute(mrb_state *mrb, mrb_value self)
  {
    mrb_int id;
    char *tag;
    mrb_int tag_length;
    const mrb_value *mrb_arguments;
    mrb_int n_arguments;
    mrb_value mrb_block;
    mrb_get_args(mrb,
                 "is*&",
                 &id,
                 &tag,
                 &tag_length,
                 &mrb_arguments,
                 &n_arguments,
                 &mrb_block);

    auto ctx = static_cast<grn_ctx *>(mrb->ud);
    auto data = task_executor_data(mrb, self);

    /* All validations that may raise are done before any C++ object
     * with a destructor is created because a raise is longjmp() that
     * skips C++ destructors. */
    if (id < 0) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "%S[execute] ID must be a non-negative integer: %S",
                 mrb_str_new(mrb, tag, tag_length),
                 mrb_int_value(mrb, id));
    }
    if (data->tasks.find(static_cast<uintptr_t>(id)) != data->tasks.end()) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "%S[execute] ID is already used: %S",
                 mrb_str_new(mrb, tag, tag_length),
                 mrb_int_value(mrb, id));
    }
    if (!mrb_proc_p(mrb_block)) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "%S[execute] block is required",
                 mrb_str_new(mrb, tag, tag_length));
    }
    auto proc = mrb_proc_ptr(mrb_block);
    if (MRB_PROC_CFUNC_P(proc)) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "%S[execute] block must be written in Ruby",
                 mrb_str_new(mrb, tag, tag_length));
    }
    auto irep = proc->body.irep;
    uint8_t *irep_binary = nullptr;
    size_t irep_binary_size = 0;
    auto dump_result = mrb_dump_irep(mrb,
                                     irep,
                                     MRB_DUMP_DEBUG_INFO,
                                     &irep_binary,
                                     &irep_binary_size);
    if (dump_result != MRB_DUMP_OK) {
      mrb_raisef(mrb,
                 mrb_class_get(mrb, "ArgumentError"),
                 "%S[execute] failed to serialize the block: %S",
                 mrb_str_new(mrb, tag, tag_length),
                 mrb_int_value(mrb, dump_result));
    }

    auto task = new Task();
    task->id = static_cast<uintptr_t>(id);
    task->tag.assign(tag, tag_length);
    task->rc = GRN_SUCCESS;
    task->error_file = nullptr;
    task->error_line = 0;
    task->error_function = nullptr;
    task->irep.assign(reinterpret_cast<const char *>(irep_binary),
                      irep_binary_size);
    mrb_free(mrb, irep_binary);
    task->arguments.resize(n_arguments);
    for (mrb_int i = 0; i < n_arguments; ++i) {
      const char *error_class_name = nullptr;
      if (!DetachedValue::from_mrb(mrb,
                                   mrb_arguments[i],
                                   task->arguments[i],
                                   &error_class_name)) {
        delete task;
        mrb_raisef(mrb,
                   mrb_class_get(mrb, "ArgumentError"),
                   "%S[execute] can't pass %S as the %S-th argument",
                   mrb_str_new(mrb, tag, tag_length),
                   mrb_str_new_cstr(mrb, error_class_name),
                   mrb_int_value(mrb, i + 1));
      }
    }

    data->tasks.emplace(task->id, std::unique_ptr<Task>(task));
    data->task_ids.push_back(task->id);
    auto execute = [ctx, task]() { return run_task(ctx, task); };
    /* The task's tag is alive until the task is waited. */
    data->executor->execute(task->id, execute, task->tag.c_str());
    if (!data->executor->is_parallel()) {
      /* The task is already finished in sequential mode. */
      report_task_errors(data);
    }
    /* This raises when submitting the task is failed in parallel mode
     * or when the task is failed in sequential mode. */
    grn_mrb_ctx_check(mrb);
    return mrb_nil_value();
  }

  /* Convert the result of the task to a value in the caller's mruby
   * and remove the task. The task must be finished. */
  mrb_value
  take_result(mrb_state *mrb, TaskExecutorData *data, uintptr_t id)
  {
    auto it = data->tasks.find(id);
    if (it == data->tasks.end()) {
      return mrb_nil_value();
    }
    auto result = it->second->result.to_mrb(mrb);
    data->tasks.erase(it);
    return result;
  }

  /* Groonga::TaskExecutor#wait_all: {id => result, ...} */
  mrb_value
  task_executor_wait_all(mrb_state *mrb, mrb_value self)
  {
    auto data = task_executor_data(mrb, self);
    data->executor->wait_all();
    report_task_errors(data);

    auto results =
      mrb_hash_new_capa(mrb, static_cast<mrb_int>(data->task_ids.size()));
    for (const auto id : data->task_ids) {
      auto arena_index = mrb_gc_arena_save(mrb);
      mrb_hash_set(mrb,
                   results,
                   mrb_int_value(mrb, static_cast<mrb_int>(id)),
                   take_result(mrb, data, id));
      mrb_gc_arena_restore(mrb, arena_index);
    }
    data->task_ids.clear();
    data->tasks.clear();
    grn_mrb_ctx_check(mrb);
    return results;
  }
} // namespace

extern "C" void
grn_mrb_task_executor_init(grn_ctx *ctx)
{
  auto mrb = ctx->impl->mrb.state;
  auto module = ctx->impl->mrb.module;
  auto klass =
    mrb_define_class_under(mrb, module, "TaskExecutor", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb,
                    klass,
                    "initialize",
                    task_executor_initialize,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb,
                    klass,
                    "n_workers",
                    task_executor_get_n_workers,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb,
                    klass,
                    "parallel?",
                    task_executor_is_parallel,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb,
                    klass,
                    "execute",
                    task_executor_execute,
                    MRB_ARGS_REQ(2) | MRB_ARGS_REST() | MRB_ARGS_BLOCK());
  mrb_define_method(mrb,
                    klass,
                    "wait_all",
                    task_executor_wait_all,
                    MRB_ARGS_NONE());
}
#endif
