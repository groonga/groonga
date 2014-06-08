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
#include <mruby/class.h>
#include <mruby/variable.h>

#include "../db.h"
#include "mrb_index_info.h"
#include "mrb_converter.h"

mrb_value
mrb_grn_index_info_new(mrb_state *mrb, grn_obj *index, int section_id)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;
  mrb_value args[2];

  args[0] = grn_mrb_value_from_grn_obj(mrb, index);
  args[1] = mrb_fixnum_value(section_id);
  klass = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(module),
                                      mrb_intern(mrb, "IndexInfo")));
  return mrb_obj_new(mrb, klass, 2, args);
}

static mrb_value
mrb_grn_index_info_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value index;
  mrb_value section_id;

  mrb_get_args(mrb, "oo", &index, &section_id);
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "index"), index);
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "section_id"), section_id);
  return self;
}

static mrb_value
mrb_grn_index_info_get_index(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "index"));
}

static mrb_value
mrb_grn_index_info_get_section_id(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "section_id"));
}

void
grn_mrb_index_info_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "IndexInfo", data->object_class);
  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_index_info_initialize, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, klass, "index",
                    mrb_grn_index_info_get_index, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "section_id",
                    mrb_grn_index_info_get_section_id, MRB_ARGS_NONE());
}
#endif
