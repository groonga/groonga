/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014-2015 Brazil

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
#include <mruby/data.h>
#include <mruby/hash.h>

#include "mrb_ctx.h"
#include "mrb_table.h"
#include "mrb_converter.h"

static mrb_value
mrb_grn_table_is_locked(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  unsigned int is_locked;

  is_locked = grn_obj_is_locked(ctx, DATA_PTR(self));
  grn_mrb_ctx_check(mrb);

  return mrb_bool_value(is_locked != 0);
}

static mrb_value
mrb_grn_table_get_size(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  unsigned int size;

  size = grn_table_size(ctx, DATA_PTR(self));
  grn_mrb_ctx_check(mrb);

  return mrb_fixnum_value(size);
}

static mrb_value
mrb_grn_table_select(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *table;
  grn_obj *expr;
  grn_obj *result = NULL;
  grn_operator operator = GRN_OP_OR;
  mrb_value mrb_expr;
  mrb_value mrb_options = mrb_nil_value();

  table = DATA_PTR(self);
  mrb_get_args(mrb, "o|H", &mrb_expr, &mrb_options);

  expr = DATA_PTR(mrb_expr);

  if (!mrb_nil_p(mrb_options)) {
    mrb_value mrb_result;
    mrb_value mrb_operator;

    mrb_result = mrb_hash_get(mrb, mrb_options,
                              mrb_symbol_value(mrb_intern_lit(mrb, "result")));
    if (!mrb_nil_p(mrb_result)) {
      result = DATA_PTR(mrb_result);
    }

    mrb_operator = mrb_hash_get(mrb, mrb_options,
                                mrb_symbol_value(mrb_intern_lit(mrb, "operator")));
    if (!mrb_nil_p(mrb_operator)) {
      operator = mrb_fixnum(mrb_operator);
    }
  }

  result = grn_table_select(ctx, table, expr, result, operator);
  grn_mrb_ctx_check(mrb);

  return grn_mrb_value_from_grn_obj(mrb, result);
}

void
grn_mrb_table_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *object_class = data->object_class;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Table", object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "locked?",
                    mrb_grn_table_is_locked, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "size",
                    mrb_grn_table_get_size, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "select",
                    mrb_grn_table_select, MRB_ARGS_ARG(1, 1));
}
#endif
