/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_selector.h"

struct _grn_selector_data {
  grn_obj *selector;
  grn_obj *expr;
  grn_obj *table;
  grn_obj *index;
  size_t n_args;
  grn_obj **args;
  grn_obj *res;
  grn_operator op;
};

grn_selector_data *
grn_selector_data_get(grn_ctx *ctx)
{
  return ctx->impl->current_selector_data;
}

grn_rc
grn_selector_run(grn_ctx *ctx,
                 grn_obj *selector,
                 grn_obj *expr,
                 grn_obj *table,
                 grn_obj *index,
                 size_t n_args,
                 grn_obj **args,
                 grn_obj *res,
                 grn_operator op)
{
  grn_selector_data data;
  data.selector = selector;
  data.expr = expr;
  data.table = table;
  data.index = index;
  data.n_args = n_args;
  data.args = args;
  data.res = res;
  data.op = op;

  grn_selector_data *previous_data = ctx->impl->current_selector_data;
  ctx->impl->current_selector_data = &data;
  grn_proc *proc = (grn_proc *)selector;
  grn_rc rc = proc->callbacks.function.selector(ctx,
                                                table,
                                                index,
                                                n_args,
                                                args,
                                                res,
                                                op);
  ctx->impl->current_selector_data = previous_data;
  return rc;
}

grn_obj *
grn_selector_data_get_selector(grn_ctx *ctx,
                               grn_selector_data *data)
{
  return data->selector;
}

grn_obj *
grn_selector_data_get_expr(grn_ctx *ctx,
                           grn_selector_data *data)
{
  return data->expr;
}

grn_obj *
grn_selector_data_get_table(grn_ctx *ctx,
                            grn_selector_data *data)
{
  return data->table;
}

grn_obj *
grn_selector_data_get_index(grn_ctx *ctx,
                            grn_selector_data *data)
{
  return data->index;
}

grn_obj **
grn_selector_data_get_args(grn_ctx *ctx,
                           grn_selector_data *data,
                           size_t *n_args)
{
  if (n_args) {
    *n_args = data->n_args;
  }
  return data->args;
}

grn_obj *
grn_selector_data_get_res(grn_ctx *ctx,
                          grn_selector_data *data)
{
  return data->res;
}

grn_operator
grn_selector_data_get_op(grn_ctx *ctx,
                         grn_selector_data *data)
{
  return data->op;
}
