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

#ifndef GRN_MRB_H
#define GRN_MRB_H

#include "groonga_in.h"
#include "ctx.h"

#ifdef GRN_WITH_MRUBY
# include <mruby.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void grn_ctx_impl_mrb_init(grn_ctx *ctx);
void grn_ctx_impl_mrb_fin(grn_ctx *ctx);

#ifdef GRN_WITH_MRUBY
mrb_value grn_mrb_eval(grn_ctx *ctx, const char *script, int script_length);
grn_rc grn_mrb_send(grn_ctx *ctx, grn_obj *grn_recv, const char *name, int argc,
                    grn_obj *grn_argv, grn_obj *grn_object);
grn_rc grn_mrb_to_grn(grn_ctx *ctx, mrb_value mrb_object, grn_obj *grn_object);
mrb_value grn_mrb_from_grn(grn_ctx *ctx, grn_obj **argv);
mrb_value grn_mrb_obj_new(mrb_state *mrb, const char *cname, void *ptr);
#endif

#ifdef __cplusplus
}
#endif

#endif /* GRN_MRB_H */
