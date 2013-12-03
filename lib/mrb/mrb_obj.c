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

#include "mrb_obj.h"

static mrb_value
object_get_name(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *object;
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_length;

  object = DATA_PTR(self);
  name_length = grn_obj_name(ctx, object, name, GRN_TABLE_MAX_KEY_SIZE);

  return mrb_str_new(mrb, name, name_length);
}

void
grn_mrb_obj_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Object", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  data->object_class = klass;

  mrb_define_method(mrb, klass, "name", object_get_name, MRB_ARGS_NONE());
}
#endif
