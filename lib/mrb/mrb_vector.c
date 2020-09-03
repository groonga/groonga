/* -*- c-basic-offset: 2 -*- */
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

static struct mrb_data_type mrb_grn_vector_type = {
  "Groonga::Vector",
  NULL
};

static mrb_value
mrb_grn_vector_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_vector_ptr;

  mrb_get_args(mrb, "o", &mrb_vector_ptr);
  DATA_TYPE(self) = &mrb_grn_vector_type;
  DATA_PTR(self) = mrb_cptr(mrb_vector_ptr);
  return self;
}

mrb_value
grn_mrb_value_from_vector(mrb_state *mrb, grn_obj *vector)
{
  if (!vector) {
    return mrb_nil_value();
  }

  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  unsigned int n = grn_vector_size(ctx, vector);
  mrb_value mrb_vector = mrb_ary_new_capa(mrb, n);
  grn_obj element;
  GRN_TEXT_INIT(&element, GRN_OBJ_DO_SHALLOW_COPY);
  for (unsigned int i = 0; i < n; i++) {
    const char *content;
    unsigned int content_length;
    content_length = grn_vector_get_element(ctx,
                                            vector,
                                            i,
                                            &content,
                                            NULL,
                                            &(element.header.domain));
    GRN_TEXT_SET(ctx, &element, content, content_length);
    mrb_ary_push(mrb, mrb_vector, grn_mrb_value_from_bulk(mrb, &element));
    element.header.domain = GRN_DB_TEXT;
  }
  GRN_OBJ_FIN(ctx, &element);

  return mrb_vector;
}

static mrb_value
mrb_grn_vector_get_value(mrb_state *mrb, mrb_value self)
{
  return grn_mrb_value_from_vector(mrb, DATA_PTR(self));
}

static mrb_value
mrb_grn_vector_equal(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_other;

  mrb_get_args(mrb, "o", &mrb_other);

  if (!mrb_obj_is_kind_of(mrb, mrb_other, mrb_class(mrb, self))) {
    return mrb_false_value();
  }

  return mrb_bool_value(DATA_PTR(self) == DATA_PTR(mrb_other));
}

void
grn_mrb_vector_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Vector", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_vector_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "value",
                    mrb_grn_vector_get_value, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "true?",
                    grn_mrb_object_is_true, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "==",
                    mrb_grn_vector_equal, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "inspect",
                    grn_mrb_object_inspect, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "close",
                    grn_mrb_object_close, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "closed?",
                    grn_mrb_object_is_closed, MRB_ARGS_NONE());
}
#endif
