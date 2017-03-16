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

grn_rc
grn_table_apply_expr(grn_ctx *ctx,
                           grn_obj *table,
                           grn_obj *output_column,
                           grn_obj *expr)
{
  grn_obj *record;

  GRN_API_ENTER;

  record = grn_expr_get_var_by_offset(ctx, expr, 0);
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, table, cursor, id, GRN_CURSOR_BY_ID) {
    grn_obj *value;
    GRN_RECORD_SET(ctx, record, id);
    value = grn_expr_exec(ctx, expr, 0);
    if (value) {
      grn_obj_set_value(ctx, output_column, id, value, GRN_OBJ_SET);
    }
  } GRN_TABLE_EACH_END(ctx, cursor);

  GRN_API_RETURN(ctx->rc);
}
