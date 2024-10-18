/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn.h"
#include "grn_expr_executor.h"
#include "grn_generated_column.h"
#include "grn_store.h"

void
grn_generated_column_update(grn_ctx *ctx,
                            grn_obj *column,
                            grn_id id,
                            int section,
                            grn_obj *old_value,
                            grn_obj *new_value)
{
  grn_ja *ja = (grn_ja *)column;
  grn_expr_executor executor;
  grn_expr_executor_init(ctx, &executor, ja->parsed_generator);
  grn_id *source_ids = DB_OBJ(column)->source;
  grn_expr_executor_add_replace_value(ctx, &executor, source_ids[0], new_value);
  grn_obj *value = grn_expr_executor_exec(ctx, &executor, id);
  if (ctx->rc == GRN_SUCCESS) {
    grn_obj_set_value(ctx, column, id, value, GRN_OBJ_SET);
  }
  grn_expr_executor_fin(ctx, &executor);
}

void
grn_generated_column_build(grn_ctx *ctx, grn_obj *column)
{
  grn_obj *table = grn_ctx_at(ctx, column->header.domain);
  grn_id *source_ids = DB_OBJ(column)->source;
  grn_obj *source = grn_ctx_at(ctx, source_ids[0]);
  grn_obj_set_visibility(ctx, column, false);
  if (ctx->rc == GRN_SUCCESS) {
    grn_obj *generator = ((grn_ja *)(column))->parsed_generator;
    grn_table_apply_expr(ctx,table,column, generator);
  }
  if (ctx->rc == GRN_SUCCESS) {
    grn_obj_set_visibility(ctx, column, true);
  }
  grn_obj_unref(ctx, source);
  grn_obj_unref(ctx, table);
}
