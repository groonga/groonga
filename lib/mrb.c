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

#include "mrb.h"
#include "ctx_impl.h"

#ifdef WITH_MRUBY
# include <mruby/proc.h>
# include <mruby/compile.h>
#endif

#ifdef WITH_MRUBY
void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
  const char *grn_mrb_enabled;
  grn_mrb_enabled = getenv("GRN_MRB_ENABLED");
  if (!grn_mrb_enabled || strcmp(grn_mrb_enabled, "yes") == 0) {
    ctx->impl->mrb = mrb_open();
  } else {
    ctx->impl->mrb = NULL;
  }
}

void
grn_ctx_impl_mrb_fin(grn_ctx *ctx)
{
  if (ctx->impl->mrb) {
    mrb_close(ctx->impl->mrb);
    ctx->impl->mrb = NULL;
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
