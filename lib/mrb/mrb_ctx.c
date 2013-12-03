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

#include "../ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include "mrb_ctx.h"
#include "mrb_converter.h"

static mrb_value
ctx_class_instance(mrb_state *mrb, mrb_value klass)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_ctx;

  mrb_ctx = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_DATA, mrb_class_ptr(klass)));
  DATA_PTR(mrb_ctx) = ctx;

  return mrb_ctx;
}

static mrb_value
ctx_array_reference(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *object;
  char *name;
  int name_length;

  mrb_get_args(mrb, "s", &name, &name_length);
  object = grn_ctx_get(ctx, name, name_length);

  return grn_mrb_value_from_grn_obj(mrb, object);
}

void
grn_mrb_ctx_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Context", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_class_method(mrb, klass, "instance",
                          ctx_class_instance, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "[]", ctx_array_reference, MRB_ARGS_REQ(1));
}
#endif
