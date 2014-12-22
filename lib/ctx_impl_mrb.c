/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013-2014 Brazil

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

#include <string.h>

#include "grn_ctx_impl_mrb.h"
#include "grn_ctx_impl.h"

#include "grn_mrb.h"
#include "mrb/mrb_error.h"
#include "mrb/mrb_id.h"
#include "mrb/mrb_operator.h"
#include "mrb/mrb_ctx.h"
#include "mrb/mrb_logger.h"
#include "mrb/mrb_void.h"
#include "mrb/mrb_bulk.h"
#include "mrb/mrb_obj.h"
#include "mrb/mrb_database.h"
#include "mrb/mrb_column.h"
#include "mrb/mrb_fixed_size_column.h"
#include "mrb/mrb_variable_size_column.h"
#include "mrb/mrb_index_column.h"
#include "mrb/mrb_expr.h"
#include "mrb/mrb_accessor.h"
#include "mrb/mrb_procedure.h"

#ifdef GRN_WITH_MRUBY
static mrb_value
mrb_kernel_load(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  char *path;

  mrb_get_args(mrb, "z", &path);

  grn_mrb_load(ctx, path);

  grn_mrb_ctx_check(mrb);

  return mrb_true_value();
}

static void
grn_ctx_impl_mrb_init_bindings(grn_ctx *ctx)
{
  mrb_state *mrb = ctx->impl->mrb.state;

  mrb->ud = ctx;
  ctx->impl->mrb.module = mrb_define_module(mrb, "Groonga");

  mrb_define_class(mrb, "LoadError", mrb_class_get(mrb, "ScriptError"));
  mrb_define_method(mrb, mrb->kernel_module,
                    "load", mrb_kernel_load, MRB_ARGS_REQ(1));

  grn_mrb_load(ctx, "backtrace_entry.rb");

  grn_mrb_error_init(ctx);
  grn_mrb_id_init(ctx);
  grn_mrb_operator_init(ctx);
  grn_mrb_ctx_init(ctx);
  grn_mrb_logger_init(ctx);
  grn_mrb_void_init(ctx);
  grn_mrb_bulk_init(ctx);
  grn_mrb_obj_init(ctx);
  grn_mrb_database_init(ctx);
  grn_mrb_column_init(ctx);
  grn_mrb_fixed_size_column_init(ctx);
  grn_mrb_variable_size_column_init(ctx);
  grn_mrb_index_column_init(ctx);
  grn_mrb_expr_init(ctx);
  grn_mrb_accessor_init(ctx);
  grn_mrb_procedure_init(ctx);
}

static void
grn_ctx_impl_mrb_init_eval(grn_ctx *ctx)
{
  grn_mrb_load(ctx, "eval_context.rb");
}

void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
  const char *grn_mruby_enabled;
  grn_mruby_enabled = getenv("GRN_MRUBY_ENABLED");
  if (grn_mruby_enabled && strcmp(grn_mruby_enabled, "no") == 0) {
    ctx->impl->mrb.state = NULL;
    ctx->impl->mrb.base_directory[0] = '\0';
    ctx->impl->mrb.module = NULL;
    ctx->impl->mrb.object_class = NULL;
  } else {
    ctx->impl->mrb.state = mrb_open();
    ctx->impl->mrb.base_directory[0] = '\0';
    grn_ctx_impl_mrb_init_bindings(ctx);
    grn_ctx_impl_mrb_init_eval(ctx);
  }
}

void
grn_ctx_impl_mrb_fin(grn_ctx *ctx)
{
  if (ctx->impl->mrb.state) {
    mrb_close(ctx->impl->mrb.state);
    ctx->impl->mrb.state = NULL;
  }
}
#else
void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
}

void
grn_ctx_impl_mrb_fin(grn_ctx *ctx)
{
}
#endif
