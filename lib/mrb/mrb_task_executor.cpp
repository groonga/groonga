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

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#  include "../grn_ctx.hpp"

#  include <mruby.h>
#  include <mruby/class.h>
#  include <mruby/data.h>

#  include "mrb_ctx.h"
#  include "mrb_task_executor.h"

namespace {
  const mrb_data_type mrb_grn_task_executor_type = {
    "Groonga::TaskExecutor",
    nullptr,
  };

  grn::TaskExecutor *
  task_executor_get(mrb_state *mrb, mrb_value self)
  {
    return static_cast<grn::TaskExecutor *>(
      mrb_data_get_ptr(mrb, self, &mrb_grn_task_executor_type));
  }

  mrb_value
  task_executor_initialize(mrb_state *mrb, mrb_value self)
  {
    auto ctx = static_cast<grn_ctx *>(mrb->ud);
    mrb_data_init(self,
                  grn_ctx_get_task_executor(ctx),
                  &mrb_grn_task_executor_type);
    return self;
  }

  mrb_value
  task_executor_get_n_workers(mrb_state *mrb, mrb_value self)
  {
    auto task_executor = task_executor_get(mrb, self);
    return mrb_int_value(mrb, task_executor->get_n_workers());
  }

  mrb_value
  task_executor_is_parallel(mrb_state *mrb, mrb_value self)
  {
    auto task_executor = task_executor_get(mrb, self);
    return mrb_bool_value(task_executor->is_parallel());
  }

  mrb_value
  task_executor_wait_all(mrb_state *mrb, mrb_value self)
  {
    auto task_executor = task_executor_get(mrb, self);
    auto success = task_executor->wait_all();
    grn_mrb_ctx_check(mrb);
    return mrb_bool_value(success);
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
                    "wait_all",
                    task_executor_wait_all,
                    MRB_ARGS_NONE());
}
#endif
