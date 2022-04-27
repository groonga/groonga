/*
  Copyright (C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_proc.h"

#include "../grn_ctx.h"
#include "../grn_db.h"
#include "../grn_str.h"

#include <groonga/plugin.h>

static const char *remains_column_name = "remains";
static const char *missings_column_name = "missings";

typedef struct {
  const char *tag;
  int n_args;
  grn_obj **args;
  grn_user_data *user_data;
  grn_raw_string table_name;
  grn_obj *table;
  grn_raw_string column_name;
  grn_obj *column;
} index_column_data;

static bool
index_column_data_init(grn_ctx *ctx, index_column_data *data)
{
  data->table_name.value =
    grn_plugin_proc_get_var_string(ctx,
                                   data->user_data,
                                   "table", -1,
                                   &(data->table_name.length));
  data->column_name.value =
    grn_plugin_proc_get_var_string(ctx,
                                   data->user_data,
                                   "name", -1,
                                   &(data->column_name.length));

  data->table = grn_ctx_get(ctx,
                            data->table_name.value,
                            (int)(data->table_name.length));
  if (!data->table) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[index-column]%s table doesn't exist: <%.*s>",
                     data->tag,
                     (int)(data->table_name.length),
                     data->table_name.value);
    return false;
  }
  if (!grn_obj_is_table_with_key(ctx, data->table)) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[index-column]%s table must be lexicon: <%.*s>: %s",
                     data->tag,
                     (int)(data->table_name.length),
                     data->table_name.value,
                     grn_obj_type_to_string(data->table->header.type));
    return false;
  }

  data->column = grn_obj_column(ctx,
                                data->table,
                                data->column_name.value,
                                (uint32_t)(data->column_name.length));
  if (!data->column) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[index-column]%s column doesn't exist: <%.*s>: <%.*s>",
                     data->tag,
                     (int)(data->table_name.length),
                     data->table_name.value,
                     (int)(data->column_name.length),
                     data->column_name.value);
    return false;
  }
  if (!grn_obj_is_index_column(ctx, data->column)) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[index-column]%s column must be index column: "
                     "<%.*s>: <%.*s>: %s",
                     data->tag,
                     (int)(data->table_name.length),
                     data->table_name.value,
                     (int)(data->column_name.length),
                     data->column_name.value,
                     grn_obj_type_to_string(data->column->header.type));
    return false;
  }

  return true;
}

static void
index_column_data_fin(grn_ctx *ctx, index_column_data *data)
{
  if (grn_obj_is_accessor(ctx, data->column)) {
    grn_obj_close(ctx, data->column);
  }
}

static void
index_column_diff_output_postings(grn_ctx *ctx,
                                  grn_column_flags index_column_flags,
                                  grn_obj *postings,
                                  const char *name)
{
  size_t i;
  size_t n_elements = 1;
  if (index_column_flags & GRN_OBJ_WITH_SECTION) {
    n_elements++;
  }
  if (index_column_flags & GRN_OBJ_WITH_POSITION) {
    n_elements++;
  }
  size_t n = GRN_UINT32_VECTOR_SIZE(postings);
  grn_ctx_output_array_open(ctx, name, (int)(n / n_elements));
  for (i = 0; i < n; i += n_elements) {
    grn_ctx_output_map_open(ctx, "posting", (int)n_elements);
    {
      size_t j = i;
      grn_ctx_output_cstr(ctx, "record_id");
      grn_id record_id = GRN_UINT32_VALUE_AT(postings, j);
      grn_ctx_output_uint32(ctx, record_id);
      if (index_column_flags & GRN_OBJ_WITH_SECTION) {
        j++;
        grn_ctx_output_cstr(ctx, "section_id");
        grn_id section_id = GRN_UINT32_VALUE_AT(postings, j);
        grn_ctx_output_uint32(ctx, section_id);
      }
      if (index_column_flags & GRN_OBJ_WITH_POSITION) {
        j++;
        grn_ctx_output_cstr(ctx, "position");
        uint32_t position = GRN_UINT32_VALUE_AT(postings, j);
        grn_ctx_output_uint32(ctx, position);
      }
    }
    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_array_close(ctx);
}

static void
index_column_diff_output(grn_ctx *ctx,
                         grn_obj *diff,
                         grn_obj *lexicon,
                         grn_column_flags index_column_flags)
{
  grn_obj *remains_column =
    grn_obj_column(ctx,
                   diff,
                   remains_column_name,
                   (uint32_t)strlen(remains_column_name));
  grn_obj *missings_column =
    grn_obj_column(ctx,
                   diff,
                   missings_column_name,
                   (uint32_t)strlen(missings_column_name));
  grn_obj key;
  GRN_OBJ_INIT(&key, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY, lexicon->header.domain);
  grn_obj remains;
  GRN_UINT32_INIT(&remains, GRN_OBJ_VECTOR);
  grn_obj missings;
  GRN_UINT32_INIT(&missings, GRN_OBJ_VECTOR);
  grn_ctx_output_array_open(ctx, "diffs", (int)grn_table_size(ctx, diff));
  {
    GRN_TABLE_EACH_BEGIN(ctx, diff, cursor, id) {
      grn_ctx_output_map_open(ctx, "diff", 3);
      {
        grn_ctx_output_cstr(ctx, "token");
        grn_ctx_output_map_open(ctx, "token", 2);
        {
          grn_ctx_output_cstr(ctx, "id");
          void *token_id_buffer;
          grn_table_cursor_get_key(ctx, cursor, &token_id_buffer);
          grn_id token_id = *((grn_id *)token_id_buffer);
          grn_ctx_output_uint32(ctx, token_id);

          grn_ctx_output_cstr(ctx, "value");
          char key_buffer[GRN_TABLE_MAX_KEY_SIZE];
          int key_size = grn_table_get_key(ctx,
                                           lexicon,
                                           token_id,
                                           key_buffer,
                                           sizeof(key_buffer));
          GRN_TEXT_SET(ctx, &key, key_buffer, key_size);
          grn_ctx_output_obj(ctx, &key, NULL);
        }
        grn_ctx_output_map_close(ctx);

        grn_ctx_output_cstr(ctx, "remains");
        GRN_BULK_REWIND(&remains);
        grn_obj_get_value(ctx, remains_column, id, &remains);
        index_column_diff_output_postings(ctx,
                                          index_column_flags,
                                          &remains,
                                          "remains");

        grn_ctx_output_cstr(ctx, "missings");
        GRN_BULK_REWIND(&missings);
        grn_obj_get_value(ctx, missings_column, id, &missings);
        index_column_diff_output_postings(ctx,
                                          index_column_flags,
                                          &missings,
                                          "missings");
      }
      grn_ctx_output_map_close(ctx);
    } GRN_TABLE_EACH_END(ctx, cursor);
  }
  grn_ctx_output_array_close(ctx);
  GRN_OBJ_FIN(ctx, &missings);
  GRN_OBJ_FIN(ctx, &remains);
  GRN_OBJ_FIN(ctx, &key);
}

static grn_obj *
command_index_column_diff(grn_ctx *ctx,
                          int n_args,
                          grn_obj **args,
                          grn_user_data *user_data)
{
  index_column_data data;
  grn_obj *diff = NULL;

  data.tag = "[diff]";
  data.n_args = n_args;
  data.args = args;
  data.user_data = user_data;
  data.table = NULL;
  data.column = NULL;
  if (!index_column_data_init(ctx, &data)) {
    goto exit;
  }

  grn_index_column_diff(ctx, data.column, &diff);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "[index-column]%s failed to diff: "
                     "<%.*s>: <%.*s>: %s",
                     data.tag,
                     (int)(data.table_name.length),
                     data.table_name.value,
                     (int)(data.column_name.length),
                     data.column_name.value,
                     ctx->errbuf);
    goto exit;
  }

  index_column_diff_output(ctx,
                           diff,
                           data.table,
                           grn_column_get_flags(ctx, data.column));

exit :
  index_column_data_fin(ctx, &data);

  if (diff) {
    grn_obj_close(ctx, diff);
  }

  return NULL;
}

void
grn_proc_init_index_column_diff(grn_ctx *ctx)
{
  grn_expr_var vars[2];
  unsigned int n_vars = 0;

  grn_plugin_expr_var_init(ctx, &(vars[n_vars++]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[n_vars++]), "name", -1);
  grn_plugin_command_create(ctx,
                            "index_column_diff", -1,
                            command_index_column_diff,
                            n_vars,
                            vars);
}
