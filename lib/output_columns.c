/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#include "grn_expr.h"
#include "grn_output_columns.h"

grn_obj *
grn_output_columns_parse(grn_ctx *ctx,
                         grn_obj *table,
                         const char *raw_output_columns,
                         size_t raw_output_columns_size)
{
  grn_obj *output_columns = NULL;
  grn_obj *variable;

  GRN_API_ENTER;

  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, output_columns, variable);
  if (ctx->rc == GRN_SUCCESS) {
    grn_expr_parse(ctx,
                   output_columns,
                   raw_output_columns,
                   raw_output_columns_size,
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_OUTPUT_COLUMNS);
  }

  GRN_API_RETURN(output_columns);
}

grn_rc
grn_output_columns_get_offsets(grn_ctx *ctx,
                               grn_obj *output_columns,
                               grn_obj *offsets)
{
  int previous_comma_offset = -1;
  grn_bool is_first_comma = GRN_TRUE;
  grn_bool have_comma = GRN_FALSE;
  grn_expr *expr = (grn_expr *)output_columns;
  grn_expr_code *code;
  grn_expr_code *code_end = expr->codes + expr->codes_curr;

  GRN_API_ENTER;

  for (code = expr->codes; code < code_end; code++) {
    int code_start_offset;

    if (code->op != GRN_OP_COMMA) {
      continue;
    }

    have_comma = GRN_TRUE;
    if (is_first_comma) {
      unsigned int n_used_codes;
      int code_end_offset;

      n_used_codes = grn_expr_code_n_used_codes(ctx, expr->codes, code - 1);
      code_end_offset = code - expr->codes - n_used_codes;
      GRN_UINT32_PUT(ctx, offsets, 0);
      GRN_UINT32_PUT(ctx, offsets, code_end_offset);
      code_start_offset = code_end_offset;
      is_first_comma = GRN_FALSE;
    } else {
      code_start_offset = previous_comma_offset + 1;
    }

    GRN_UINT32_PUT(ctx, offsets, code_start_offset);
    GRN_UINT32_PUT(ctx, offsets, code - expr->codes);

    previous_comma_offset = code - expr->codes;
  }

  if (!have_comma && expr->codes_curr > 0) {
    GRN_UINT32_PUT(ctx, offsets, 0);
    GRN_UINT32_PUT(ctx, offsets, expr->codes_curr);
  }

  GRN_API_RETURN(ctx->rc);
}

static void
grn_output_columns_apply_one(grn_ctx *ctx,
                             grn_obj *output_columns,
                             grn_obj *ids,
                             grn_obj *variable,
                             int code_start_offset,
                             int code_end_offset,
                             grn_obj *column)
{
  grn_expr *expr = (grn_expr *)output_columns;
  grn_expr_code *codes = expr->codes;
  uint32_t codes_curr = expr->codes_curr;
  grn_expr_executor executor;
  size_t i;
  size_t n_ids;

  expr->codes += code_start_offset;
  expr->codes_curr = code_end_offset - code_start_offset;
  grn_expr_executor_init(ctx, &executor, output_columns);
  n_ids = GRN_BULK_VSIZE(ids) / sizeof(grn_id);
  for (i = 0; i < n_ids; i += 2) {
    grn_id source_id = GRN_RECORD_VALUE_AT(ids, i);
    grn_id target_id = GRN_RECORD_VALUE_AT(ids, i + 1);
    grn_obj *value;

    value = grn_expr_executor_exec(ctx, &executor, source_id);
    if (value) {
      grn_obj_set_value(ctx, column, target_id, value, GRN_OBJ_SET);
    }
  }
  grn_expr_executor_fin(ctx, &executor);

  expr->codes = codes;
  expr->codes_curr = codes_curr;
}

grn_rc
grn_output_columns_apply(grn_ctx *ctx,
                         grn_obj *output_columns,
                         grn_obj *columns)
{
  grn_obj *variable;
  grn_obj *source_table;
  grn_obj *target_table = NULL;
  grn_bool use_keys = GRN_FALSE;
  size_t n_columns;
  grn_obj source_key_buffer;
  grn_obj target_key_buffer;
  grn_obj ids;
  grn_obj offsets;

  GRN_API_ENTER;

  variable = grn_expr_get_var_by_offset(ctx, output_columns, 0);
  source_table = grn_ctx_at(ctx, variable->header.domain);

  n_columns = GRN_BULK_VSIZE(columns) / sizeof(grn_obj *);
  if (n_columns == 0) {
    goto exit;
  }
  {
    grn_obj *first_column;

    first_column = GRN_PTR_VALUE_AT(columns, 0);
    if (grn_obj_is_accessor(ctx, first_column)) {
      target_table = ((grn_accessor *)first_column)->obj;
    } else {
      grn_id target_table_id;
      target_table_id = first_column->header.domain;
      target_table = grn_ctx_at(ctx, target_table_id);
    }
  }

  use_keys = (target_table->header.type != GRN_TABLE_NO_KEY);
  if (use_keys) {
    if (source_table->header.domain == GRN_DB_SHORT_TEXT) {
      GRN_SHORT_TEXT_INIT(&source_key_buffer, 0);
    } else {
      GRN_VALUE_FIX_SIZE_INIT(&source_key_buffer,
                              0,
                              source_table->header.domain);
    }
    if (target_table->header.domain == GRN_DB_SHORT_TEXT) {
      GRN_SHORT_TEXT_INIT(&target_key_buffer, 0);
    } else {
      GRN_VALUE_FIX_SIZE_INIT(&target_key_buffer,
                              0,
                              target_table->header.domain);
    }
  }

  GRN_RECORD_INIT(&ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_UINT32_INIT(&offsets, GRN_OBJ_VECTOR);

  if (use_keys) {
    GRN_TABLE_EACH_BEGIN(ctx, source_table, cursor, source_id) {
      void *key;
      int key_size;
      grn_rc cast_rc;
      grn_id target_id;

      GRN_BULK_REWIND(&source_key_buffer);
      GRN_BULK_REWIND(&target_key_buffer);

      /* TODO: no key -> key case */
      key_size = grn_table_cursor_get_key(ctx, cursor, &key);
      grn_bulk_write(ctx, &source_key_buffer, key, key_size);
      cast_rc = grn_obj_cast(ctx,
                             &source_key_buffer,
                             &target_key_buffer,
                             GRN_FALSE);
      if (cast_rc != GRN_SUCCESS) {
        break;
      }

      target_id = grn_table_add(ctx,
                                target_table,
                                GRN_BULK_HEAD(&target_key_buffer),
                                GRN_BULK_VSIZE(&target_key_buffer),
                                NULL);
      if (target_id == GRN_ID_NIL) {
        break;
      }

      GRN_RECORD_PUT(ctx, &ids, source_id);
      GRN_RECORD_PUT(ctx, &ids, target_id);
    } GRN_TABLE_EACH_END(ctx, cursor);
  } else {
    GRN_TABLE_EACH_BEGIN(ctx, source_table, cursor, source_id) {
      grn_id target_id;

      target_id = grn_table_add(ctx,
                                target_table,
                                NULL,
                                0,
                                NULL);
      if (target_id == GRN_ID_NIL) {
        break;
      }

      GRN_RECORD_PUT(ctx, &ids, source_id);
      GRN_RECORD_PUT(ctx, &ids, target_id);
    } GRN_TABLE_EACH_END(ctx, cursor);
  }

  {
    size_t i, n;

    grn_output_columns_get_offsets(ctx, output_columns, &offsets);
    if (ctx->rc != GRN_SUCCESS) {
      GRN_OBJ_FIN(ctx, &offsets);
      goto exit;
    }

    n = GRN_BULK_VSIZE(&offsets) / sizeof(uint32_t) / 2;
    for (i = 0; i < n; i++) {
      uint32_t code_start_offset;
      uint32_t code_end_offset;

      code_start_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2);
      code_end_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2 + 1);
      grn_output_columns_apply_one(ctx,
                                   output_columns,
                                   &ids,
                                   variable,
                                   code_start_offset,
                                   code_end_offset,
                                   GRN_PTR_VALUE_AT(columns, i));
    }
  }

exit :
  if (target_table) {
    if (use_keys) {
      GRN_OBJ_FIN(ctx, &source_key_buffer);
      GRN_OBJ_FIN(ctx, &target_key_buffer);
    }
    GRN_OBJ_FIN(ctx, &ids);
    GRN_OBJ_FIN(ctx, &offsets);
  }

  GRN_API_RETURN(ctx->rc);
}
