/*
  Copyright(C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include "mrb_converter.h"
#include "mrb_ctx.h"
#include "mrb_window_function_executor.h"

static void
mrb_grn_window_function_executor_free(mrb_state *mrb, void *data)
{
  grn_window_function_executor *executor = data;

  if (!executor) {
    return;
  }

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor_close(ctx, executor);
}

static struct mrb_data_type mrb_grn_window_function_executor_type = {
  "Groonga::WindowFunctionExecutor",
  mrb_grn_window_function_executor_free
};

static mrb_value
mrb_grn_window_function_executor_initialize(mrb_state *mrb, mrb_value self)
{
  DATA_TYPE(self) = &mrb_grn_window_function_executor_type;

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor =
    grn_window_function_executor_open(ctx);
  grn_mrb_ctx_check(mrb);
  DATA_PTR(self) = executor;

  return self;
}

static mrb_value
mrb_grn_window_function_executor_close(mrb_state *mrb, mrb_value self)
{
  grn_window_function_executor *executor = DATA_PTR(self);
  if (executor) {
    mrb_grn_window_function_executor_free(mrb, executor);
    DATA_PTR(self) = NULL;
  }

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_set_source(mrb_state *mrb, mrb_value self)
{
  char *source;
  mrb_int source_size;
  mrb_get_args(mrb, "s!", &source, &source_size);

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_window_function_executor_set_source(ctx,
                                          executor,
                                          source,
                                          (int)source_size);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_add_table(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_table;
  mrb_get_args(mrb, "o", &mrb_table);

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_obj *table = GRN_MRB_DATA_PTR(mrb_table);
  grn_window_function_executor_add_table(ctx, executor, table);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_add_context_table(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_table;
  mrb_get_args(mrb, "o", &mrb_table);

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_obj *table = GRN_MRB_DATA_PTR(mrb_table);
  grn_window_function_executor_add_context_table(ctx, executor, table);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_set_sort_keys(mrb_state *mrb, mrb_value self)
{
  char *keys;
  mrb_int keys_size;
  mrb_get_args(mrb, "s!", &keys, &keys_size);

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_window_function_executor_set_sort_keys(ctx,
                                             executor,
                                             keys,
                                             (int)keys_size);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_set_group_keys(mrb_state *mrb, mrb_value self)
{
  char *keys;
  mrb_int keys_size;
  mrb_get_args(mrb, "s!", &keys, &keys_size);

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_window_function_executor_set_group_keys(ctx,
                                              executor,
                                              keys,
                                              (int)keys_size);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_set_output_column_name(mrb_state *mrb,
                                                        mrb_value self)
{
  char *name;
  mrb_int name_size;
  mrb_get_args(mrb, "s!", &name, &name_size);

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_window_function_executor_set_output_column_name(ctx,
                                                      executor,
                                                      name,
                                                      (int)name_size);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_window_function_executor_execute(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_window_function_executor *executor = DATA_PTR(self);
  grn_window_function_executor_execute(ctx, executor);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

void
grn_mrb_window_function_executor_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "WindowFunctionExecutor",
                                 mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_window_function_executor_initialize,
                    MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "close",
                    mrb_grn_window_function_executor_close, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "add_table",
                    mrb_grn_window_function_executor_add_table,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "add_context_table",
                    mrb_grn_window_function_executor_add_context_table,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "source=",
                    mrb_grn_window_function_executor_set_source,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "sort_keys=",
                    mrb_grn_window_function_executor_set_sort_keys,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "group_keys=",
                    mrb_grn_window_function_executor_set_group_keys,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "output_column_name=",
                    mrb_grn_window_function_executor_set_output_column_name,
                    MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "execute",
                    mrb_grn_window_function_executor_execute,
                    MRB_ARGS_NONE());
}
#endif
