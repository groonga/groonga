/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017 Brazil

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

#include "grn.h"
#include "grn_ctx.h"
#include "grn_expr_executor.h"

grn_rc
grn_table_apply_expr(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *output_column,
                     grn_obj *expr)
{
  grn_expr_executor *executor;

  GRN_API_ENTER;

  if (!grn_obj_is_data_column(ctx, output_column)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, output_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply-expr] output column isn't data column: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  if (!grn_obj_is_expr(ctx, expr)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, expr);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply-expr] expr is invalid: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  executor = grn_expr_executor_open(ctx, expr);
  if (!executor) {
    GRN_API_RETURN(ctx->rc);
  }
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, table, cursor, id, GRN_CURSOR_BY_ID) {
    grn_obj *value;
    value = grn_expr_executor_exec(ctx, executor, id);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    if (value) {
      grn_obj_set_value(ctx, output_column, id, value, GRN_OBJ_SET);
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  grn_expr_executor_close(ctx, executor);

  GRN_API_RETURN(ctx->rc);
}
