/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013-2015 Brazil
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/data.h>

#include "../grn_db.h"
#include "mrb_accessor.h"
#include "mrb_bulk.h"
#include "mrb_converter.h"
#include "mrb_ctx.h"
#include "mrb_operator.h"
#include "mrb_options.h"

static struct mrb_data_type mrb_grn_accessor_type = {
  "Groonga::Accessor",
  NULL
};

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
  return grn_mrb_value_from_grn_obj(mrb, (grn_obj *)(accessor->next));
}

static mrb_value
mrb_grn_accessor_have_next_p(mrb_state *mrb, mrb_value self)
{
  grn_accessor *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(accessor->next != NULL);
}

static mrb_value
mrb_grn_accessor_object(mrb_state *mrb, mrb_value self)
{
  grn_accessor *accessor;

  accessor = DATA_PTR(self);
  return grn_mrb_value_from_grn_obj(mrb, accessor->obj);
}

static mrb_value
mrb_grn_accessor_id_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_id_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_key_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_key_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_value_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_value_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_score_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_score_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_nsubrecs_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_nsubrecs_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_max_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_max_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_min_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_min_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_sum_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_sum_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_avg_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_avg_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_mean_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_mean_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_column_value_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;

  accessor = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_column_value_accessor(ctx, accessor));
}

static mrb_value
mrb_grn_accessor_name(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_rc rc;
  grn_obj *accessor;
  grn_obj name;
  mrb_value mrb_name;

  accessor = DATA_PTR(self);
  GRN_TEXT_INIT(&name, 0);
  rc = grn_column_name_(ctx, accessor, &name);
  if (rc == GRN_SUCCESS) {
    mrb_name = mrb_str_new(mrb, GRN_TEXT_VALUE(&name), GRN_TEXT_LEN(&name));
    GRN_OBJ_FIN(ctx, &name);
  } else {
    mrb_name = mrb_nil_value();
    GRN_OBJ_FIN(ctx, &name);
    grn_mrb_ctx_check(mrb);
  }

  return mrb_name;
}

static mrb_value
mrb_grn_accessor_estimate_size_for_query(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *accessor;
  mrb_value mrb_query;
  grn_obj query;
  mrb_value mrb_options = mrb_nil_value();
  grn_search_optarg optarg;
  uint32_t size;

  accessor = DATA_PTR(self);
  mrb_get_args(mrb, "o|H", &mrb_query, &mrb_options);

  GRN_VOID_INIT(&query);
  grn_mrb_value_to_bulk(mrb, mrb_query, &query);

  memset(&optarg, 0, sizeof(grn_search_optarg));
  optarg.mode = GRN_OP_EXACT;

  if (!mrb_nil_p(mrb_options)) {
    mrb_value mrb_mode;

    mrb_mode = grn_mrb_options_get_lit(mrb, mrb_options, "mode");
    if (!mrb_nil_p(mrb_mode)) {
      optarg.mode = grn_mrb_value_to_operator(mrb, mrb_mode);
    }
  }

  size = grn_accessor_estimate_size_for_query(ctx,
                                              accessor,
                                              &query,
                                              &optarg);
  GRN_OBJ_FIN(ctx, &query);

  grn_mrb_ctx_check(mrb);

  return mrb_int_value(mrb, size);
}

void
grn_mrb_accessor_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Accessor", data->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_accessor_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "next",
                    mrb_grn_accessor_next, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "have_next?",
                    mrb_grn_accessor_have_next_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "object",
                    mrb_grn_accessor_object, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "id?",
                    mrb_grn_accessor_id_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "key?",
                    mrb_grn_accessor_key_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "value?",
                    mrb_grn_accessor_value_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "score?",
                    mrb_grn_accessor_score_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "nsubrecs?",
                    mrb_grn_accessor_nsubrecs_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "max?",
                    mrb_grn_accessor_max_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "min?",
                    mrb_grn_accessor_min_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "sum?",
                    mrb_grn_accessor_sum_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "avg?",
                    mrb_grn_accessor_avg_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "mean?",
                    mrb_grn_accessor_mean_p, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "column_value?",
                    mrb_grn_accessor_column_value_p, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "name",
                    mrb_grn_accessor_name, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "estimate_size_for_query",
                    mrb_grn_accessor_estimate_size_for_query,
                    MRB_ARGS_ARG(1, 1));
}
#endif
