/*
  Copyright(C) 2013-2018  Brazil
  Copyright(C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

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
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>

#include "../grn_expr.h"
#include "../grn_proc.h"
#include "../grn_util.h"
#include "../grn_mrb.h"
#include "mrb_accessor.h"
#include "mrb_ctx.h"
#include "mrb_expr.h"
#include "mrb_operator.h"
#include "mrb_converter.h"
#include "mrb_options.h"

static struct mrb_data_type mrb_grn_scan_info_type = {
  "Groonga::ScanInfo",
  NULL
};
static struct mrb_data_type mrb_grn_expr_code_type = {
  "Groonga::ExpressionCode",
  NULL
};
static struct mrb_data_type mrb_grn_expression_type = {
  "Groonga::Expression",
  NULL
};

static mrb_value
mrb_grn_scan_info_new(mrb_state *mrb, scan_info *scan_info)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;
  mrb_value mrb_scan_info;

  mrb_scan_info = mrb_cptr_value(mrb, scan_info);
  klass = mrb_class_get_under(mrb, module, "ScanInfo");
  return mrb_obj_new(mrb, klass, 1, &mrb_scan_info);
}

static mrb_value
mrb_grn_expr_code_new(mrb_state *mrb, grn_expr_code *code)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;
  mrb_value mrb_code;

  mrb_code = mrb_cptr_value(mrb, code);
  klass = mrb_class_get_under(mrb, module, "ExpressionCode");
  return mrb_obj_new(mrb, klass, 1, &mrb_code);
}

static mrb_value
mrb_grn_scan_info_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_ptr;

  mrb_get_args(mrb, "o", &mrb_ptr);
  DATA_TYPE(self) = &mrb_grn_scan_info_type;
  DATA_PTR(self) = mrb_cptr(mrb_ptr);
  return self;
}

static mrb_value
mrb_grn_expr_code_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_code;

  mrb_get_args(mrb, "o", &mrb_code);
  DATA_TYPE(self) = &mrb_grn_expr_code_type;
  DATA_PTR(self) = mrb_cptr(mrb_code);
  return self;
}

static mrb_value
mrb_grn_scan_info_put_index(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  scan_info *si;
  mrb_value mrb_index;
  mrb_int sid;
  mrb_float weight;
  mrb_value mrb_scorer;
  mrb_value mrb_scorer_args_expr;
  mrb_int scorer_args_expr_offset;
  grn_obj *index;
  grn_obj *scorer = NULL;
  grn_obj *scorer_args_expr = NULL;

  mrb_get_args(mrb, "oifooi",
               &mrb_index, &sid, &weight,
               &mrb_scorer,
               &mrb_scorer_args_expr,
               &scorer_args_expr_offset);
  si = DATA_PTR(self);
  index = DATA_PTR(mrb_index);
  if (!mrb_nil_p(mrb_scorer)) {
    scorer = DATA_PTR(mrb_scorer);
  }
  if (!mrb_nil_p(mrb_scorer_args_expr)) {
    scorer_args_expr = DATA_PTR(mrb_scorer_args_expr);
  }
  grn_scan_info_put_index(ctx, si, index, (uint32_t)sid, (float)weight,
                          scorer,
                          scorer_args_expr,
                          (uint32_t)scorer_args_expr_offset);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  grn_operator op;

  si = DATA_PTR(self);
  op = grn_scan_info_get_op(si);
  return grn_mrb_value_from_operator(mrb, op);
}

static mrb_value
mrb_grn_scan_info_set_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_value mrb_op;
  grn_operator op;

  mrb_get_args(mrb, "o", &mrb_op);
  si = DATA_PTR(self);
  op = grn_mrb_value_to_operator(mrb, mrb_op);
  grn_scan_info_set_op(si, op);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_weight_factor(mrb_state *mrb, mrb_value self)
{
  scan_info *si = DATA_PTR(self);
  float factor = grn_scan_info_get_weight_factor(si);
  return mrb_float_value(mrb, factor);
}

static mrb_value
mrb_grn_scan_info_set_weight_factor(mrb_state *mrb, mrb_value self)
{
  mrb_float factor;
  mrb_get_args(mrb, "f", &factor);
  scan_info *si = DATA_PTR(self);
  grn_scan_info_set_weight_factor(si, (float)factor);
  return self;
}

static mrb_value
mrb_grn_scan_info_set_end(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int end;

  mrb_get_args(mrb, "i", &end);
  si = DATA_PTR(self);
  grn_scan_info_set_end(si, (uint32_t)end);
  return self;
}

static mrb_value
mrb_grn_scan_info_set_query(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_value mrb_query;

  mrb_get_args(mrb, "o", &mrb_query);
  si = DATA_PTR(self);
  if (mrb_nil_p(mrb_query)) {
    grn_scan_info_set_query(si, NULL);
  } else {
    grn_scan_info_set_query(si, DATA_PTR(mrb_query));
  }
  return self;
}

static mrb_value
mrb_grn_scan_info_set_flags(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int flags;

  mrb_get_args(mrb, "i", &flags);
  si = DATA_PTR(self);
  grn_scan_info_set_flags(si, (int)flags);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_flags(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int flags;

  si = DATA_PTR(self);
  flags = grn_scan_info_get_flags(si);
  return mrb_int_value(mrb, flags);
}

static mrb_value
mrb_grn_scan_info_set_logical_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_value mrb_logical_op;
  grn_operator logical_op;

  mrb_get_args(mrb, "o", &mrb_logical_op);
  si = DATA_PTR(self);
  logical_op = grn_mrb_value_to_operator(mrb, mrb_logical_op);
  grn_scan_info_set_logical_op(si, logical_op);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_logical_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  grn_operator logical_op;

  si = DATA_PTR(self);
  logical_op = grn_scan_info_get_logical_op(si);
  return grn_mrb_value_from_operator(mrb, logical_op);
}

static mrb_value
mrb_grn_scan_info_set_max_interval(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int max_interval;

  mrb_get_args(mrb, "i", &max_interval);
  si = DATA_PTR(self);
  grn_scan_info_set_max_interval(si, (int)max_interval);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_max_interval(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int max_interval;

  si = DATA_PTR(self);
  max_interval = grn_scan_info_get_max_interval(si);
  return mrb_int_value(mrb, max_interval);
}

static mrb_value
mrb_grn_scan_info_set_additional_last_interval(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int additional_last_interval;

  mrb_get_args(mrb, "i", &additional_last_interval);
  si = DATA_PTR(self);
  grn_scan_info_set_additional_last_interval(si, (int)additional_last_interval);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_additional_last_interval(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int additional_last_interval;

  si = DATA_PTR(self);
  additional_last_interval = grn_scan_info_get_additional_last_interval(si);
  return mrb_int_value(mrb, additional_last_interval);
}

static mrb_value
mrb_grn_scan_info_set_max_element_intervals(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)(mrb->ud);
  scan_info *si = DATA_PTR(self);

  mrb_value mrb_max_element_intervals;
  mrb_get_args(mrb, "A", &mrb_max_element_intervals);

  if (mrb_nil_p(mrb_max_element_intervals)) {
    grn_scan_info_set_max_element_intervals(ctx, si, NULL);
    return self;
  }

  grn_obj max_element_intervals;
  GRN_INT32_INIT(&max_element_intervals, GRN_OBJ_VECTOR);
  mrb_int n = RARRAY_LEN(mrb_max_element_intervals);
  mrb_int i;
  for (i = 0; i < n; i++) {
    mrb_value mrb_max_element_interval =
      RARRAY_PTR(mrb_max_element_intervals)[i];
    mrb_int max_element_interval = mrb_integer(mrb_max_element_interval);
    GRN_INT32_PUT(ctx, &max_element_intervals, max_element_interval);
  }
  grn_scan_info_set_max_element_intervals(ctx, si, &max_element_intervals);
  GRN_OBJ_FIN(ctx, &max_element_intervals);

  return self;
}

static mrb_value
mrb_grn_scan_info_get_max_element_intervals(mrb_state *mrb, mrb_value self)
{
  scan_info *si = DATA_PTR(self);
  grn_obj *max_element_intervals = grn_scan_info_get_max_element_intervals(si);
  size_t n = GRN_INT32_VECTOR_SIZE(max_element_intervals);
  mrb_value mrb_max_element_intervals = mrb_ary_new_capa(mrb, n);
  size_t i;
  for (i = 0; i < n; i++) {
    int32_t max_element_interval = GRN_INT32_VALUE_AT(max_element_intervals, i);
    mrb_ary_push(mrb,
                 mrb_max_element_intervals,
                 mrb_int_value(mrb, max_element_interval));
  }
  return mrb_max_element_intervals;
}

static mrb_value
mrb_grn_scan_info_set_min_interval(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int min_interval;

  mrb_get_args(mrb, "i", &min_interval);
  si = DATA_PTR(self);
  grn_scan_info_set_min_interval(si, (int)min_interval);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_min_interval(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int min_interval;

  si = DATA_PTR(self);
  min_interval = grn_scan_info_get_min_interval(si);
  return mrb_int_value(mrb, min_interval);
}

static mrb_value
mrb_grn_scan_info_set_similarity_threshold(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int similarity_threshold;

  mrb_get_args(mrb, "i", &similarity_threshold);
  si = DATA_PTR(self);
  grn_scan_info_set_similarity_threshold(si, (int)similarity_threshold);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_similarity_threshold(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int similarity_threshold;

  si = DATA_PTR(self);
  similarity_threshold = grn_scan_info_get_similarity_threshold(si);
  return mrb_int_value(mrb, similarity_threshold);
}

static mrb_value
mrb_grn_scan_info_set_quorum_threshold(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int quorum_threshold;

  mrb_get_args(mrb, "i", &quorum_threshold);
  si = DATA_PTR(self);
  grn_scan_info_set_quorum_threshold(si, (int)quorum_threshold);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_quorum_threshold(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int quorum_threshold;

  si = DATA_PTR(self);
  quorum_threshold = grn_scan_info_get_quorum_threshold(si);
  return mrb_int_value(mrb, quorum_threshold);
}

static mrb_value
mrb_grn_scan_info_get_arg(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  scan_info *si;
  mrb_int index;
  grn_obj *arg;

  mrb_get_args(mrb, "i", &index);

  si = DATA_PTR(self);
  arg = grn_scan_info_get_arg(ctx, si, (int)index);

  return grn_mrb_value_from_grn_obj(mrb, arg);
}

static mrb_value
mrb_grn_scan_info_push_arg(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  scan_info *si;
  mrb_value mrb_arg;
  grn_bool success;

  mrb_get_args(mrb, "o", &mrb_arg);

  si = DATA_PTR(self);
  success = grn_scan_info_push_arg(ctx, si, DATA_PTR(mrb_arg));

  return mrb_bool_value(success);
}

static mrb_value
mrb_grn_scan_info_get_start_position(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int start_position;

  si = DATA_PTR(self);
  start_position = grn_scan_info_get_start_position(si);
  return mrb_int_value(mrb, start_position);
}

static mrb_value
mrb_grn_scan_info_set_start_position(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_int start_position;

  mrb_get_args(mrb, "i", &start_position);
  si = DATA_PTR(self);
  grn_scan_info_set_start_position(si, (uint32_t)start_position);
  return self;
}

static mrb_value
mrb_grn_scan_info_reset_position(mrb_state *mrb, mrb_value self)
{
  scan_info *si;

  si = DATA_PTR(self);
  grn_scan_info_reset_position(si);
  return self;
}

static mrb_value
mrb_grn_expr_code_inspect(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_expr_code *code;
  mrb_value inspected;

  code = DATA_PTR(self);

  inspected = mrb_str_buf_new(mrb, 48);

  mrb_str_cat_lit(mrb, inspected, "#<");
  mrb_str_cat_cstr(mrb, inspected, mrb_obj_classname(mrb, self));
  mrb_str_cat_lit(mrb, inspected, ":");
  mrb_str_concat(mrb, inspected, mrb_ptr_to_str(mrb, mrb_cptr(self)));

  {
    float weight;
    uint32_t offset;

    weight = grn_expr_code_get_weight(ctx, DATA_PTR(self), &offset);

    mrb_str_cat_lit(mrb, inspected, " weight=");
    mrb_str_concat(mrb, inspected,
                   mrb_funcall(mrb,
                               mrb_float_value(mrb, weight),
                               "inspect",
                               0));
    mrb_str_cat_lit(mrb, inspected, ", offset=");
    mrb_str_concat(mrb, inspected,
                   mrb_funcall(mrb,
                               mrb_int_value(mrb, offset),
                               "inspect",
                               0));
  }

  mrb_str_cat_lit(mrb, inspected, ", n_args=");
  mrb_str_concat(mrb, inspected,
                 mrb_funcall(mrb,
                             mrb_int_value(mrb, code->nargs),
                             "inspect",
                             0));

  mrb_str_cat_lit(mrb, inspected, ", modify=");
  mrb_str_concat(mrb, inspected,
                 mrb_funcall(mrb,
                             mrb_int_value(mrb, code->modify),
                             "inspect",
                             0));

  mrb_str_cat_lit(mrb, inspected, ", op=");
  mrb_str_concat(mrb, inspected,
                 mrb_funcall(mrb,
                             grn_mrb_value_from_operator(mrb, code->op),
                             "inspect",
                             0));

  mrb_str_cat_lit(mrb, inspected, ", flags=");
  mrb_str_concat(mrb, inspected,
                 mrb_funcall(mrb,
                             mrb_int_value(mrb, code->flags),
                             "inspect",
                             0));

  mrb_str_cat_lit(mrb, inspected, ", value=");
  mrb_str_concat(mrb, inspected,
                 mrb_funcall(mrb,
                             grn_mrb_value_from_grn_obj(mrb, code->value),
                             "inspect",
                             0));

  mrb_str_cat_lit(mrb, inspected, ">");

  return inspected;
}

static mrb_value
mrb_grn_expr_code_get_weight(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  float weight;
  uint32_t offset;
  mrb_value mrb_values[2];

  weight = grn_expr_code_get_weight(ctx, DATA_PTR(self), &offset);
  mrb_values[0] = mrb_float_value(mrb, weight);
  mrb_values[1] = mrb_int_value(mrb, offset);
  return mrb_ary_new_from_values(mrb, 2, mrb_values);
}

static mrb_value
mrb_grn_expr_code_get_value(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *expr_code;

  expr_code = DATA_PTR(self);
  return grn_mrb_value_from_grn_obj(mrb, expr_code->value);
}

static mrb_value
mrb_grn_expr_code_get_n_args(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *expr_code;

  expr_code = DATA_PTR(self);
  return mrb_int_value(mrb, expr_code->nargs);
}

static mrb_value
mrb_grn_expr_code_get_op(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *expr_code;

  expr_code = DATA_PTR(self);
  return grn_mrb_value_from_operator(mrb, expr_code->op);
}

static mrb_value
mrb_grn_expr_code_get_flags(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *expr_code;

  expr_code = DATA_PTR(self);
  return mrb_int_value(mrb, expr_code->flags);
}

static mrb_value
mrb_grn_expr_code_get_modify(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *expr_code;

  expr_code = DATA_PTR(self);
  return mrb_int_value(mrb, expr_code->modify);
}

static mrb_value
mrb_grn_expression_class_create(mrb_state *mrb, mrb_value klass)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_expr;
  mrb_value mrb_table;
  mrb_value mrb_new_arguments[1];
  grn_obj *expr, *variable = NULL;

  mrb_get_args(mrb, "o", &mrb_table);
  if (mrb_nil_p(mrb_table)) {
    expr = grn_expr_create(ctx, NULL, 0);
  } else {
    grn_obj *table = DATA_PTR(mrb_table);
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expr, variable);
  }

  if (!expr) {
    grn_mrb_ctx_check(mrb);
    return mrb_nil_value();
  }

  mrb_new_arguments[0] = mrb_cptr_value(mrb, expr);
  mrb_expr = mrb_obj_new(mrb, mrb_class_ptr(klass), 1, mrb_new_arguments);
  {
    mrb_value mrb_variable = mrb_nil_value();
    if (variable) {
      mrb_variable = grn_mrb_value_from_grn_obj(mrb, variable);
    }
    mrb_iv_set(mrb, mrb_expr, mrb_intern_lit(mrb, "@variable"), mrb_variable);
  }

  return mrb_expr;
}

static mrb_value
mrb_grn_expression_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_expression_ptr;

  mrb_get_args(mrb, "o", &mrb_expression_ptr);
  DATA_TYPE(self) = &mrb_grn_expression_type;
  DATA_PTR(self) = mrb_cptr(mrb_expression_ptr);
  return self;
}

static mrb_value
mrb_grn_expression_is_empty(mrb_state *mrb, mrb_value self)
{
  grn_expr *expr;

  expr = DATA_PTR(self);
  return mrb_bool_value(expr->codes_curr == 0);
}

static mrb_value
mrb_grn_expression_codes(mrb_state *mrb, mrb_value self)
{
  grn_expr *expr;
  mrb_value mrb_codes;
  uint32_t i;

  expr = DATA_PTR(self);
  mrb_codes = mrb_ary_new_capa(mrb, expr->codes_curr);
  for (i = 0; i < expr->codes_curr; i++) {
    grn_expr_code *code = expr->codes + i;
    mrb_ary_push(mrb, mrb_codes, mrb_grn_expr_code_new(mrb, code));
  }

  return mrb_codes;
}

static mrb_value
mrb_grn_expression_array_reference(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_key;
  grn_obj *var;

  mrb_get_args(mrb, "o", &mrb_key);

  expr = DATA_PTR(self);
  switch (mrb_type(mrb_key)) {
  case MRB_TT_SYMBOL :
    {
      const char *name;
      mrb_int name_length;

      name = mrb_sym2name_len(mrb, mrb_symbol(mrb_key), &name_length);
      var = grn_expr_get_var(ctx, expr, name, (unsigned int)name_length);
    }
    break;
  case MRB_TT_STRING :
    var = grn_expr_get_var(ctx, expr,
                           RSTRING_PTR(mrb_key), RSTRING_LEN(mrb_key));
    break;
  case MRB_TT_INTEGER :
    var = grn_expr_get_var_by_offset(ctx,
                                     expr,
                                     (unsigned int)mrb_integer(mrb_key));
    break;
  default :
    mrb_raisef(mrb, E_ARGUMENT_ERROR,
               "key must be Symbol, String or Fixnum: %S",
               mrb_key);
    break;
  }

  return grn_mrb_value_from_grn_obj(mrb, var);
}

static mrb_value
mrb_grn_expression_set_condition(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_condition;

  mrb_get_args(mrb, "o", &mrb_condition);

  expr = DATA_PTR(self);
  grn_expr_set_condition(ctx, expr, GRN_MRB_DATA_PTR(mrb_condition));

  return mrb_nil_value();
}

static mrb_value
mrb_grn_expression_take_object(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_object;
  grn_obj *grn_object;

  mrb_get_args(mrb, "o", &mrb_object);
  expr = DATA_PTR(self);
  grn_object = DATA_PTR(mrb_object);
  grn_expr_take_obj(ctx, expr, grn_object);

  return mrb_object;
}

static mrb_value
mrb_grn_expression_allocate_constant(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_object;
  grn_obj *grn_object;

  mrb_get_args(mrb, "o", &mrb_object);
  expr = DATA_PTR(self);

  switch (mrb_type(mrb_object)) {
  case MRB_TT_STRING:
    grn_object = grn_expr_alloc_const(ctx, expr);
    if (!grn_object) {
      grn_mrb_ctx_check(mrb);
    }
    GRN_TEXT_INIT(grn_object, 0);
    GRN_TEXT_SET(ctx, grn_object,
                 RSTRING_PTR(mrb_object), RSTRING_LEN(mrb_object));
    break;
  case MRB_TT_TRUE:
    grn_object = grn_expr_alloc_const(ctx, expr);
    if (!grn_object) {
      grn_mrb_ctx_check(mrb);
    }
    GRN_BOOL_INIT(grn_object, 0);
    GRN_BOOL_SET(ctx, grn_object, GRN_TRUE);
    break;
  default:
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "unsupported type: %S", mrb_object);
    break;
  }

  return grn_mrb_value_from_grn_obj(mrb, grn_object);
}

static mrb_value
mrb_grn_expression_parse(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  char *query;
  mrb_int query_size;
  grn_obj *default_column = NULL;
  grn_operator default_mode = GRN_OP_MATCH;
  grn_operator default_operator = GRN_OP_AND;
  grn_expr_flags flags = GRN_EXPR_SYNTAX_SCRIPT;
  mrb_value mrb_options = mrb_nil_value();

  expr = DATA_PTR(self);
  mrb_get_args(mrb, "s|H", &query, &query_size, &mrb_options);

  if (!mrb_nil_p(mrb_options)) {
    mrb_value mrb_default_column;
    mrb_value mrb_flags;

    mrb_default_column =
      grn_mrb_options_get_lit(mrb, mrb_options, "default_column");
    default_column = GRN_MRB_DATA_PTR(mrb_default_column);

    mrb_flags = grn_mrb_options_get_lit(mrb, mrb_options, "flags");
    if (!mrb_nil_p(mrb_flags)) {
      flags = (grn_expr_flags)mrb_integer(mrb_flags);
    }
  }

  grn_expr_parse(ctx, expr, query, (unsigned int)query_size, default_column,
                 default_mode, default_operator, flags);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_expression_append_object(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_object;
  grn_obj *object;
  mrb_value mrb_op;
  grn_operator op;
  mrb_int mrb_n_args;
  int n_args;

  expr = DATA_PTR(self);
  mrb_get_args(mrb, "ooi", &mrb_object, &mrb_op, &mrb_n_args);

  object = DATA_PTR(mrb_object);
  op = grn_mrb_value_to_operator(mrb, mrb_op);
  n_args = (int)mrb_n_args;
  grn_expr_append_obj(ctx, expr, object, op, n_args);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_expression_append_constant(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_constant;
  mrb_value mrb_op;
  grn_operator op;
  mrb_int mrb_n_args;
  int n_args;

  expr = DATA_PTR(self);
  mrb_get_args(mrb, "ooi", &mrb_constant, &mrb_op, &mrb_n_args);

  op = grn_mrb_value_to_operator(mrb, mrb_op);
  n_args = (int)mrb_n_args;
  switch (mrb_type(mrb_constant)) {
  case MRB_TT_FALSE :
    if (mrb_nil_p(mrb_constant)) {
      grn_obj constant;
      GRN_VOID_INIT(&constant);
      grn_expr_append_const(ctx, expr, &constant, op, n_args);
      GRN_OBJ_FIN(ctx, &constant);
    } else {
      grn_obj constant;
      GRN_BOOL_INIT(&constant, 0);
      GRN_BOOL_SET(ctx, &constant, GRN_FALSE);
      grn_expr_append_const(ctx, expr, &constant, op, n_args);
      GRN_OBJ_FIN(ctx, &constant);
    }
    break;
  case MRB_TT_TRUE :
    {
      grn_obj constant;
      GRN_BOOL_INIT(&constant, 0);
      GRN_BOOL_SET(ctx, &constant, GRN_TRUE);
      grn_expr_append_const(ctx, expr, &constant, op, n_args);
      GRN_OBJ_FIN(ctx, &constant);
    }
    break;
  case MRB_TT_INTEGER:
    grn_expr_append_const_int32(ctx,
                                expr,
                                (int32_t)mrb_integer(mrb_constant),
                                op,
                                n_args);
    break;
  case MRB_TT_SYMBOL :
    {
      const char *value;
      mrb_int mrb_value_length;

      value = mrb_sym2name_len(mrb, mrb_symbol(mrb_constant), &mrb_value_length);
      unsigned int value_length = (unsigned int)mrb_value_length;
      grn_expr_append_const_str(ctx, expr, value, value_length, op, n_args);
    }
    break;
  case MRB_TT_FLOAT :
    {
      mrb_float value = mrb_float(mrb_constant);
      if (sizeof(mrb_float) == sizeof(float)) {
        grn_expr_append_const_float32(ctx,
                                      expr,
                                      (float)value,
                                      op,
                                      n_args);
      } else {
        grn_expr_append_const_float(ctx,
                                    expr,
                                    value,
                                    op,
                                    n_args);
      }
    }
    break;
  case MRB_TT_STRING :
    grn_expr_append_const_str(ctx, expr,
                              RSTRING_PTR(mrb_constant),
                              RSTRING_LEN(mrb_constant),
                              op, n_args);
    break;
  default :
    {
      grn_mrb_data *data = &(ctx->impl->mrb);
      struct RClass *klass;

      klass = mrb_class(mrb, mrb_constant);
      if (klass == data->builtin.time_class) {
        grn_obj constant;
        mrb_value mrb_sec;
        mrb_value mrb_usec;

        mrb_sec = mrb_funcall(mrb, mrb_constant, "to_i", 0);
        mrb_usec = mrb_funcall(mrb, mrb_constant, "usec", 0);
        GRN_TIME_INIT(&constant, 0);
        GRN_TIME_SET(ctx, &constant,
                     GRN_TIME_PACK(mrb_integer(mrb_sec), mrb_integer(mrb_usec)));
        grn_expr_append_const(ctx, expr, &constant, op, n_args);
        GRN_OBJ_FIN(ctx, &constant);
      } else if (klass == mrb_class_get_under(mrb, data->module, "Record")) {
        grn_obj constant;
        grn_id id;
        mrb_value mrb_table;
        grn_obj *domain;

        id = (grn_id)mrb_integer(mrb_funcall(mrb, mrb_constant, "id", 0));
        mrb_table = mrb_funcall(mrb, mrb_constant, "table", 0);
        domain = DATA_PTR(mrb_table);
        GRN_RECORD_INIT(&constant, 0, grn_obj_id(ctx, domain));
        GRN_RECORD_SET(ctx, &constant, id);
        grn_expr_append_const(ctx, expr, &constant, op, n_args);
        GRN_OBJ_FIN(ctx, &constant);
      } else if (klass == mrb_class_get_under(mrb, data->module, "Bulk")) {
        grn_obj *bulk = DATA_PTR(mrb_constant);
        grn_expr_append_const(ctx, expr, bulk, op, n_args);
      } else if (klass == mrb_class_get_under(mrb, data->module, "Vector")) {
        grn_obj *vector = DATA_PTR(mrb_constant);
        grn_expr_append_const(ctx, expr, vector, op, n_args);
      } else {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "unsupported constant to append to expression: %S",
                   mrb_constant);
      }
    }
    break;
  }

  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

static mrb_value
mrb_grn_expression_append_operator(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *expr;
  mrb_value mrb_op;
  mrb_int n_args;
  grn_operator op;

  expr = DATA_PTR(self);
  mrb_get_args(mrb, "oi", &mrb_op, &n_args);

  op = grn_mrb_value_to_operator(mrb, mrb_op);
  grn_expr_append_op(ctx, expr, op, (int)n_args);
  grn_mrb_ctx_check(mrb);

  return mrb_nil_value();
}

void
grn_mrb_expr_init(grn_ctx *ctx)
{
  mrb_state *mrb = ctx->impl->mrb.state;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *object_class = ctx->impl->mrb.object_class;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "ScanInfo", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_scan_info_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "put_index",
                    mrb_grn_scan_info_put_index, MRB_ARGS_REQ(6));
  mrb_define_method(mrb, klass, "op",
                    mrb_grn_scan_info_get_op, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "op=",
                    mrb_grn_scan_info_set_op, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "weight_factor",
                    mrb_grn_scan_info_get_weight_factor, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "weight_factor=",
                    mrb_grn_scan_info_set_weight_factor, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "end=",
                    mrb_grn_scan_info_set_end, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "query=",
                    mrb_grn_scan_info_set_query, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "flags",
                    mrb_grn_scan_info_get_flags, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "flags=",
                    mrb_grn_scan_info_set_flags, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "logical_op",
                    mrb_grn_scan_info_get_logical_op, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "logical_op=",
                    mrb_grn_scan_info_set_logical_op, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "max_interval",
                    mrb_grn_scan_info_get_max_interval, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "max_interval=",
                    mrb_grn_scan_info_set_max_interval, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "additional_last_interval",
                    mrb_grn_scan_info_get_additional_last_interval,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "additional_last_interval=",
                    mrb_grn_scan_info_set_additional_last_interval,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "max_element_intervals",
                    mrb_grn_scan_info_get_max_element_intervals,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "max_element_intervals=",
                    mrb_grn_scan_info_set_max_element_intervals,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "min_interval",
                    mrb_grn_scan_info_get_min_interval, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "min_interval=",
                    mrb_grn_scan_info_set_min_interval, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "similarity_threshold",
                    mrb_grn_scan_info_get_similarity_threshold, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "similarity_threshold=",
                    mrb_grn_scan_info_set_similarity_threshold, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "quorum_threshold",
                    mrb_grn_scan_info_get_quorum_threshold, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "quorum_threshold=",
                    mrb_grn_scan_info_set_quorum_threshold, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "get_arg",
                    mrb_grn_scan_info_get_arg, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "push_arg",
                    mrb_grn_scan_info_push_arg, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "start_position",
                    mrb_grn_scan_info_get_start_position, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "start_position=",
                    mrb_grn_scan_info_set_start_position, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "reset_position",
                    mrb_grn_scan_info_reset_position, MRB_ARGS_NONE());

  klass = mrb_define_class_under(mrb, module,
                                 "ExpressionCode", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_expr_code_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "inspect",
                    mrb_grn_expr_code_inspect, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "weight",
                    mrb_grn_expr_code_get_weight, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "value",
                    mrb_grn_expr_code_get_value, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "n_args",
                    mrb_grn_expr_code_get_n_args, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "op",
                    mrb_grn_expr_code_get_op, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "flags",
                    mrb_grn_expr_code_get_flags, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "modify",
                    mrb_grn_expr_code_get_modify, MRB_ARGS_NONE());

  {
    struct RClass *expression_code_class = klass;
    struct RClass *flags_module;
    flags_module = mrb_define_module_under(mrb, expression_code_class, "Flags");
    mrb_define_const(mrb, flags_module, "RELATIONAL_EXPRESSION",
                     mrb_int_value(mrb, GRN_EXPR_CODE_RELATIONAL_EXPRESSION));
  }

  klass = mrb_define_class_under(mrb, module, "Expression", object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

#define DEFINE_FLAG(name)                               \
  mrb_define_const(mrb, klass,                          \
                   #name,                               \
                   mrb_int_value(mrb, GRN_EXPR_ ## name))

  DEFINE_FLAG(SYNTAX_QUERY);
  DEFINE_FLAG(SYNTAX_SCRIPT);
  DEFINE_FLAG(SYNTAX_OUTPUT_COLUMNS);
  DEFINE_FLAG(ALLOW_PRAGMA);
  DEFINE_FLAG(ALLOW_COLUMN);
  DEFINE_FLAG(ALLOW_UPDATE);
  DEFINE_FLAG(ALLOW_LEADING_NOT);

#undef DEFINE_FLAG

  mrb_define_class_method(mrb, klass, "create",
                          mrb_grn_expression_class_create,
                          MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_expression_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "empty?",
                    mrb_grn_expression_is_empty, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "codes",
                    mrb_grn_expression_codes, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "[]",
                    mrb_grn_expression_array_reference, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "condition=",
                    mrb_grn_expression_set_condition, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "take_object",
                    mrb_grn_expression_take_object, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "allocate_constant",
                    mrb_grn_expression_allocate_constant, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "parse",
                    mrb_grn_expression_parse, MRB_ARGS_ARG(1, 1));

  mrb_define_method(mrb, klass, "append_object",
                    mrb_grn_expression_append_object, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, klass, "append_constant",
                    mrb_grn_expression_append_constant, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, klass, "append_operator",
                    mrb_grn_expression_append_operator, MRB_ARGS_REQ(2));
}

grn_obj *
grn_mrb_expr_rewrite(grn_ctx *ctx, grn_obj *expr)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  mrb_value mrb_expression;
  mrb_value mrb_rewritten_expression;
  grn_obj *rewritten_expression = NULL;
  int arena_index;

  arena_index = mrb_gc_arena_save(mrb);

  mrb_expression = grn_mrb_value_from_grn_obj(mrb, expr);
  mrb_rewritten_expression = mrb_funcall(mrb, mrb_expression, "rewrite", 0);
  if (mrb_nil_p(mrb_rewritten_expression)) {
    goto exit;
  }

  if (mrb_type(mrb_rewritten_expression) == MRB_TT_EXCEPTION) {
    mrb->exc = mrb_obj_ptr(mrb_rewritten_expression);
    mrb_print_error(mrb);
    goto exit;
  }

  rewritten_expression = DATA_PTR(mrb_rewritten_expression);

exit:
  mrb_gc_arena_restore(mrb, arena_index);

  return rewritten_expression;
}

scan_info **
grn_mrb_scan_info_build(grn_ctx *ctx,
                        grn_obj *expr,
                        int *n,
                        grn_operator op,
                        grn_bool record_exist)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  mrb_value mrb_expression;
  mrb_value mrb_sis;
  scan_info **sis = NULL;
  int i;
  int arena_index;

  arena_index = mrb_gc_arena_save(mrb);

  mrb_expression = grn_mrb_value_from_grn_obj(mrb, expr);
  mrb_sis = mrb_funcall(mrb, mrb_expression, "build_scan_info", 2,
                        grn_mrb_value_from_operator(mrb, op),
                        mrb_bool_value(record_exist));

  if (mrb_nil_p(mrb_sis)) {
    goto exit;
  }

  if (mrb_type(mrb_sis) == MRB_TT_EXCEPTION) {
    mrb->exc = mrb_obj_ptr(mrb_sis);
    mrb_print_error(mrb);
    goto exit;
  }

  *n = RARRAY_LEN(mrb_sis);
  sis = GRN_MALLOCN(scan_info *, *n);
  for (i = 0; i < *n; i++) {
    mrb_value mrb_si;
    mrb_value mrb_si_data;
    scan_info *si;
    int start;

    mrb_si_data = RARRAY_PTR(mrb_sis)[i];
    start = (int)mrb_integer(mrb_funcall(mrb, mrb_si_data, "start", 0));
    si = grn_scan_info_open(ctx, start);
    mrb_si = mrb_grn_scan_info_new(mrb, si);
    mrb_funcall(mrb, mrb_si, "apply", 1, mrb_si_data);
    sis[i] = si;
  }

exit:
  mrb_gc_arena_restore(mrb, arena_index);

  return sis;
}

unsigned int
grn_mrb_expr_estimate_size(grn_ctx *ctx, grn_obj *expr, grn_obj *table)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  mrb_value mrb_expression;
  mrb_value mrb_table;
  mrb_value mrb_size;
  unsigned int size;
  int arena_index;

  arena_index = mrb_gc_arena_save(mrb);

  mrb_expression = grn_mrb_value_from_grn_obj(mrb, expr);
  mrb_table = grn_mrb_value_from_grn_obj(mrb, table);
  mrb_size = mrb_funcall(mrb, mrb_expression, "estimate_size", 1, mrb_table);
  if (mrb->exc) {
    size = grn_table_size(ctx, table);
  } else {
    size = (unsigned int)mrb_integer(mrb_size);
  }

  mrb_gc_arena_restore(mrb, arena_index);

  return size;
}
#endif
