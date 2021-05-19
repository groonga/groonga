/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2020  Sutou Kouhei <kou@clear-code.com>

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
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, output_columns);
      output_columns = NULL;
    }
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
grn_output_columns_apply_inspect_columns(grn_ctx *ctx,
                                         grn_obj *buffer,
                                         grn_obj *columns)
{
  size_t i, n;

  GRN_TEXT_PUTS(ctx, buffer, "[");
  n = GRN_BULK_VSIZE(columns) / sizeof(grn_obj *);
  for (i = 0; i < n; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(columns, i);

    if (i > 0) {
      GRN_TEXT_PUTS(ctx, buffer, ", ");
    }
    if (grn_obj_is_accessor(ctx, column)) {
      grn_inspect(ctx, buffer, column);
    } else {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;

      name_size = grn_obj_name(ctx, column, name, sizeof(name));
      GRN_TEXT_PUTS(ctx, buffer, "<");
      GRN_TEXT_PUT(ctx, buffer, name, name_size);
      GRN_TEXT_PUTS(ctx, buffer, ">");
    }
  }
  GRN_TEXT_PUTS(ctx, buffer, "]");
}

static grn_obj *
grn_output_columns_apply_detect_target_table(grn_ctx *ctx,
                                             grn_obj *columns)
{
  grn_obj *target_table = NULL;
  size_t i, n;

  n = GRN_BULK_VSIZE(columns) / sizeof(grn_obj *);
  for (i = 0; i < n; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(columns, i);
    grn_obj *table;

    if (grn_obj_is_accessor(ctx, column)) {
      table = ((grn_accessor *)column)->obj;
      grn_obj_refer(ctx, table);
    } else {
      grn_id table_id;
      table_id = column->header.domain;
      table = grn_ctx_at(ctx, table_id);
    }
    if (target_table) {
      const bool valid_table = (table == target_table);
      grn_obj_unref(ctx, table);
      if (!valid_table) {
        char column_name[GRN_TABLE_MAX_KEY_SIZE];
        int column_name_size;
        char target_table_name[GRN_TABLE_MAX_KEY_SIZE];
        int target_table_name_size;

        column_name_size = grn_obj_name(ctx,
                                        column,
                                        column_name,
                                        sizeof(column_name));
        target_table_name_size = grn_obj_name(ctx,
                                              target_table,
                                              target_table_name,
                                              sizeof(target_table_name));
        ERR(GRN_INVALID_ARGUMENT,
            "[output-columns][apply] "
            "all columns must belong to the same table: <%.*s>: <%.*s>",
            column_name_size, column_name,
            target_table_name_size, target_table_name);
        grn_obj_unref(ctx, table);
        return NULL;
      }
    } else {
      target_table = table;
    }
  }

  return target_table;
}

static void
grn_output_columns_apply_add_records_no_key(grn_ctx *ctx,
                                            grn_obj *source_table,
                                            grn_obj *target_table,
                                            grn_obj *ids)
{
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

    GRN_RECORD_PUT(ctx, ids, source_id);
    GRN_RECORD_PUT(ctx, ids, target_id);
  } GRN_TABLE_EACH_END(ctx, cursor);
}

static void
grn_output_columns_apply_add_records_key(grn_ctx *ctx,
                                         grn_obj *output_columns,
                                         grn_obj *columns,
                                         grn_obj *offsets,
                                         grn_obj *source_table,
                                         grn_obj *target_table,
                                         grn_obj *ids)
{
  grn_obj target_key_buffer;
  size_t n_columns;
  size_t key_offset;

  if (target_table->header.domain == GRN_DB_SHORT_TEXT) {
    GRN_SHORT_TEXT_INIT(&target_key_buffer, 0);
  } else {
    GRN_VALUE_FIX_SIZE_INIT(&target_key_buffer,
                            0,
                            target_table->header.domain);
  }

  n_columns = GRN_BULK_VSIZE(columns) / sizeof(grn_obj *);
  {
    grn_bool have_key = GRN_FALSE;
    size_t i;
    for (i = 0; i < n_columns; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(columns, i);
      if (grn_obj_is_key_accessor(ctx, column)) {
        if (have_key) {
          grn_obj inspected_columns;
          GRN_TEXT_INIT(&inspected_columns, 0);
          grn_output_columns_apply_inspect_columns(ctx,
                                                   &inspected_columns,
                                                   columns);
          ERR(GRN_INVALID_ARGUMENT,
              "[output-columns][apply] "
              "_key is specified multiple times: %.*s",
              (int)(GRN_TEXT_LEN(&inspected_columns)),
              GRN_TEXT_VALUE(&inspected_columns));
          GRN_OBJ_FIN(ctx, &inspected_columns);
          goto exit;
        }
        key_offset = i;
        have_key = GRN_TRUE;
      }
    }

    if (!have_key) {
      grn_obj inspected_columns;
      GRN_TEXT_INIT(&inspected_columns, 0);
      grn_output_columns_apply_inspect_columns(ctx, &inspected_columns, columns);
      ERR(GRN_INVALID_ARGUMENT,
          "[output-columns][apply] "
          "_key must be specified: %.*s",
          (int)(GRN_TEXT_LEN(&inspected_columns)),
          GRN_TEXT_VALUE(&inspected_columns));
      GRN_OBJ_FIN(ctx, &inspected_columns);
      goto exit;
    }
  }

  {
    grn_expr *expr = (grn_expr *)output_columns;
    grn_expr_code *codes = expr->codes;
    uint32_t codes_curr = expr->codes_curr;
    uint32_t code_start_offset;
    uint32_t code_end_offset;
    grn_expr_executor executor;

    code_start_offset = GRN_UINT32_VALUE_AT(offsets, key_offset * 2);
    code_end_offset = GRN_UINT32_VALUE_AT(offsets, key_offset * 2 + 1);
    expr->codes += code_start_offset;
    expr->codes_curr = code_end_offset - code_start_offset;
    grn_expr_executor_init(ctx, &executor, output_columns);
    GRN_TABLE_EACH_BEGIN(ctx, source_table, cursor, source_id) {
      grn_rc cast_rc;
      grn_id target_id;
      grn_obj *value;

      value = grn_expr_executor_exec(ctx, &executor, source_id);
      if (!value) {
        continue;
      }

      GRN_BULK_REWIND(&target_key_buffer);
      cast_rc = grn_obj_cast(ctx, value, &target_key_buffer, GRN_FALSE);
      if (cast_rc != GRN_SUCCESS) {
        continue;
      }

      target_id = grn_table_add(ctx,
                                target_table,
                                GRN_BULK_HEAD(&target_key_buffer),
                                GRN_BULK_VSIZE(&target_key_buffer),
                                NULL);
      if (target_id == GRN_ID_NIL) {
        continue;
      }

      GRN_RECORD_PUT(ctx, ids, source_id);
      GRN_RECORD_PUT(ctx, ids, target_id);
    } GRN_TABLE_EACH_END(ctx, cursor);
    expr->codes = codes;
    expr->codes_curr = codes_curr;
  }

exit :
  GRN_OBJ_FIN(ctx, &target_key_buffer);
}

static void
grn_output_columns_apply_add_records(grn_ctx *ctx,
                                     grn_obj *output_columns,
                                     grn_obj *columns,
                                     grn_obj *offsets,
                                     grn_obj *ids)
{
  grn_obj *target_table;
  grn_obj *variable;
  grn_obj *source_table;

  target_table = grn_output_columns_apply_detect_target_table(ctx, columns);
  if (!target_table) {
    return;
  }

  variable = grn_expr_get_var_by_offset(ctx, output_columns, 0);
  source_table = grn_ctx_at(ctx, variable->header.domain);

  if (target_table->header.type == GRN_TABLE_NO_KEY) {
    grn_output_columns_apply_add_records_no_key(ctx,
                                                source_table,
                                                target_table,
                                                ids);
  } else {
    grn_output_columns_apply_add_records_key(ctx,
                                             output_columns,
                                             columns,
                                             offsets,
                                             source_table,
                                             target_table,
                                             ids);
  }
  grn_obj_unref(ctx, target_table);
}

static void
grn_output_columns_apply_one(grn_ctx *ctx,
                             grn_obj *output_columns,
                             grn_obj *ids,
                             uint32_t code_start_offset,
                             uint32_t code_end_offset,
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
  size_t n_columns;
  grn_obj offsets;
  grn_obj ids;
  size_t n_offsets;

  GRN_API_ENTER;

  GRN_UINT32_INIT(&offsets, GRN_OBJ_VECTOR);
  GRN_RECORD_INIT(&ids, GRN_OBJ_VECTOR, GRN_ID_NIL);

  n_columns = GRN_BULK_VSIZE(columns) / sizeof(grn_obj *);
  if (n_columns == 0) {
    goto exit;
  }

  grn_output_columns_get_offsets(ctx, output_columns, &offsets);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  n_offsets = GRN_BULK_VSIZE(&offsets) / sizeof(uint32_t) / 2;
  if (n_columns != n_offsets) {
    grn_obj inspected_columns;
    GRN_TEXT_INIT(&inspected_columns, 0);
    grn_output_columns_apply_inspect_columns(ctx,
                                             &inspected_columns,
                                             columns);
    ERR(GRN_INVALID_ARGUMENT,
        "[output-columns][apply] "
        "the number of columns (%" GRN_FMT_SIZE ") must be %" GRN_FMT_SIZE ": "
        "%.*s",
        n_columns,
        n_offsets,
        (int)GRN_TEXT_LEN(&inspected_columns),
        GRN_TEXT_VALUE(&inspected_columns));
    GRN_OBJ_FIN(ctx, &inspected_columns);
    goto exit;
  }

  grn_output_columns_apply_add_records(ctx,
                                       output_columns,
                                       columns,
                                       &offsets,
                                       &ids);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  {
    size_t i;
    for (i = 0; i < n_offsets; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(columns, i);
      uint32_t code_start_offset;
      uint32_t code_end_offset;

      if (grn_obj_is_key_accessor(ctx, column)) {
        continue;
      }

      code_start_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2);
      code_end_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2 + 1);
      grn_output_columns_apply_one(ctx,
                                   output_columns,
                                   &ids,
                                   code_start_offset,
                                   code_end_offset,
                                   column);
    }
  }

exit :
  GRN_OBJ_FIN(ctx, &offsets);
  GRN_OBJ_FIN(ctx, &ids);

  GRN_API_RETURN(ctx->rc);
}
