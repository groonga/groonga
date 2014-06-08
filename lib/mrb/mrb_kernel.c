/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014 Brazil

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

#include "../ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>

#include "../mrb.h"
#include "mrb_kernel.h"

static mrb_value
kernel_print(mrb_state *mrb, mrb_value self)
{
  char *content;
  int content_length;

  mrb_get_args(mrb, "s", &content, &content_length);
  printf("%.*s", content_length, content);

  return mrb_nil_value();
}

void
grn_mrb_kernel_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;

  mrb_define_method(mrb, mrb->kernel_module,
                    "print", kernel_print, MRB_ARGS_REQ(1));

  grn_mrb_load(ctx, "kernel.rb");
}
#endif
