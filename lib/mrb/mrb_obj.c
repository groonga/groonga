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
#include <mruby/variable.h>
#include <mruby/data.h>

#include "../db.h"
#include "mrb_obj.h"

static struct mrb_data_type mrb_grn_accessor_type = {
  "Groonga::Accessor",
  NULL
};

mrb_value
mrb_grn_accessor_new(mrb_state *mrb, grn_accessor *accessor)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;
  mrb_value mrb_accessor_ptr;

  mrb_accessor_ptr = mrb_cptr_value(mrb, accessor);
  klass = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(module),
                                      mrb_intern(mrb, "Accessor")));
  return mrb_obj_new(mrb, klass, 1, &mrb_accessor_ptr);
}

static mrb_value
mrb_grn_accessor_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_accessor_ptr;

  mrb_get_args(mrb, "o", &mrb_accessor_ptr);
  DATA_TYPE(self) = &mrb_grn_accessor_type;
  DATA_PTR(self) = mrb_cptr(mrb_accessor_ptr);
  return self;
}

static mrb_value
mrb_grn_accessor_next(mrb_state *mrb, mrb_value self)
{
  grn_accessor *accessor;

  accessor = DATA_PTR(self);
  if (!accessor->next) { return mrb_nil_value(); }
  return mrb_cptr_value(mrb, accessor->next);
}

void
grn_mrb_obj_init(grn_ctx *ctx)
{
  mrb_state *mrb = ctx->impl->mrb.state;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Object", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  klass = mrb_define_class_under(mrb, module, "Accessor", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_define_method(mrb, klass, "initialize", mrb_grn_accessor_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "next", mrb_grn_accessor_next, MRB_ARGS_NONE());
}
#endif
