/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013 Brazil

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

#include "ctx_impl_mrb.h"
#include "ctx_impl.h"

#include "mrb/mrb_obj.h"

#ifdef GRN_WITH_MRUBY
static void
grn_ctx_impl_mrb_init_bindings(grn_ctx *ctx)
{
  mrb_state *mrb = ctx->impl->mrb.state;

  ctx->impl->mrb.module = mrb_define_module(mrb, "Groonga");

  grn_mrb_obj_init(ctx);
}

void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
  const char *grn_mruby_enabled;
  grn_mruby_enabled = getenv("GRN_MRUBY_ENABLED");
  if (grn_mruby_enabled && strcmp(grn_mruby_enabled, "no") == 0) {
    ctx->impl->mrb.state = NULL;
    ctx->impl->mrb.module = NULL;
  } else {
    ctx->impl->mrb.state = mrb_open();
    grn_ctx_impl_mrb_init_bindings(ctx);
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
