/*
  Copyright(C) 2019-2020  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include "../grn_db.h"
#include "mrb_bulk.h"
#include "mrb_object.h"

static struct mrb_data_type mrb_grn_uvector_type = {
  "Groonga::UVector",
  NULL
};

static mrb_value
mrb_grn_uvector_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_uvector_ptr;

  mrb_get_args(mrb, "o", &mrb_uvector_ptr);
  DATA_TYPE(self) = &mrb_grn_uvector_type;
  DATA_PTR(self) = mrb_cptr(mrb_uvector_ptr);
  return self;
}

mrb_value
grn_mrb_value_from_uvector(mrb_state *mrb, grn_obj *uvector)
{
  if (!uvector) {
    return mrb_nil_value();
  }

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  unsigned int n = grn_uvector_size(ctx, uvector);
  unsigned int element_size = grn_uvector_element_size(ctx, uvector);
  uint8_t *raw_elements = GRN_BULK_HEAD(uvector);
  mrb_value mrb_uvector = mrb_ary_new_capa(mrb, n);
  grn_obj element;
  GRN_VALUE_FIX_SIZE_INIT(&element,
                          GRN_OBJ_DO_SHALLOW_COPY,
                          uvector->header.domain);
  for (unsigned int i = 0; i < n; i++) {
    uint8_t *raw_element = raw_elements + (element_size * i);
    GRN_TEXT_SET(ctx, &element, raw_element, element_size);
    mrb_ary_push(mrb, mrb_uvector, grn_mrb_value_from_bulk(mrb, &element));
  }
  GRN_OBJ_FIN(ctx, &element);

  return mrb_uvector;
}

static mrb_value
mrb_grn_uvector_get_value(mrb_state *mrb, mrb_value self)
{
  return grn_mrb_value_from_uvector(mrb, DATA_PTR(self));
}

static mrb_value
mrb_grn_uvector_equal(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_other;

  mrb_get_args(mrb, "o", &mrb_other);

  if (!mrb_obj_is_kind_of(mrb, mrb_other, mrb_class(mrb, self))) {
    return mrb_false_value();
  }

  return mrb_bool_value(DATA_PTR(self) == DATA_PTR(mrb_other));
}

void
grn_mrb_uvector_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "UVector", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_uvector_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "value",
                    mrb_grn_uvector_get_value, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "domain_id",
                    grn_mrb_object_get_domain_id, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "true?",
                    grn_mrb_object_is_true, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "==",
                    mrb_grn_uvector_equal, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "inspect",
                    grn_mrb_object_inspect, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "close",
                    grn_mrb_object_close, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "closed?",
                    grn_mrb_object_is_closed, MRB_ARGS_NONE());
}
#endif
