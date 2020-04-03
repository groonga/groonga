/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017 Brazil
  Copyright(C) 2019-2020 Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_time
#endif

#include <groonga/plugin.h>

static grn_rc
selector_index_column_df_ratio_between(grn_ctx *ctx,
                                       grn_obj *table,
                                       grn_obj *index,
                                       int n_args,
                                       grn_obj **args,
                                       grn_obj *res,
                                       grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *index_column;
  grn_ii *ii;
  double min;
  double max;
  grn_obj *source_table;
  unsigned int n_documents;
  grn_posting posting;

  if ((n_args - 1) != 3) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "index_column_df_ratio_between(): "
                     "wrong number of arguments (%d for 3)", n_args - 1);
    rc = ctx->rc;
    goto exit;
  }

  index_column = args[1];
  ii = (grn_ii *)index_column;
  min = GRN_FLOAT_VALUE(args[2]);
  max = GRN_FLOAT_VALUE(args[3]);

  source_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, index_column));
  n_documents = grn_table_size(ctx, source_table);
  memset(&posting, 0, sizeof(grn_posting));
  posting.sid = 1;

  if (op == GRN_OP_AND) {
    GRN_TABLE_EACH_BEGIN(ctx, res, cursor, record_id) {
      void *key;
      grn_id term_id;
      uint32_t n_match_documents;
      double df_ratio;

      grn_table_cursor_get_key(ctx, cursor, &key);
      term_id = *(grn_id *)key;
      n_match_documents = grn_ii_estimate_size(ctx, ii, term_id);
      if (n_match_documents > n_documents) {
        n_match_documents = n_documents;
      }
      df_ratio = (double)n_match_documents / (double)n_documents;
      if (min <= df_ratio && df_ratio <= max) {
        posting.rid = term_id;
        grn_ii_posting_add(ctx, &posting, (grn_hash *)res, op);
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  } else {
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, term_id) {
      uint32_t n_match_documents;
      double df_ratio;

      n_match_documents = grn_ii_estimate_size(ctx, ii, term_id);
      if (n_match_documents > n_documents) {
        n_match_documents = n_documents;
      }
      df_ratio = (double)n_match_documents / (double)n_documents;
      {
        void *key;
        int key_size;
        key_size = grn_table_cursor_get_key(ctx, cursor, &key);
      }
      if (min <= df_ratio && df_ratio <= max) {
        posting.rid = term_id;
        grn_ii_posting_add(ctx, &posting, (grn_hash *)res, op);
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
  }

exit :
  return rc;
}

typedef struct {
  grn_id term_id;
  grn_obj *term_table;
  grn_obj *index_column;
} caller_index_info;

static void
caller_index_info_fin(grn_ctx *ctx,
                      caller_index_info *caller_index_info)
{
  if (caller_index_info->index_column) {
    grn_obj_unref(ctx, caller_index_info->index_column);
  }
  if (caller_index_info->term_table) {
    grn_obj_unref(ctx, caller_index_info->term_table);
  }
}

static grn_rc
caller_index_info_init(grn_ctx *ctx,
                       caller_index_info *caller_index_info,
                       grn_obj *index_column_name,
                       grn_user_data *user_data,
                       const char *error_message_label)
{
  caller_index_info->term_id = GRN_ID_NIL;
  caller_index_info->term_table = NULL;
  caller_index_info->index_column = NULL;

  {
    grn_obj *expr;
    grn_obj *variable;

    expr = grn_plugin_proc_get_caller(ctx, user_data);
    if (!expr) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s: "
                       "called directly", error_message_label);
      return ctx->rc;
    }

    variable = grn_expr_get_var_by_offset(ctx, expr, 0);
    if (!variable) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s: "
                       "caller expression must have target record information",
                       error_message_label);
      return ctx->rc;
    }

    caller_index_info->term_table = grn_ctx_at(ctx, variable->header.domain);
    caller_index_info->term_id = GRN_RECORD_VALUE(variable);
    while (GRN_TRUE) {
      grn_obj *key_type;

      key_type = grn_ctx_at(ctx, caller_index_info->term_table->header.domain);
      if (!grn_obj_is_table(ctx, key_type)) {
        grn_obj_unref(ctx, key_type);
        break;
      }

      grn_table_get_key(ctx,
                        caller_index_info->term_table,
                        caller_index_info->term_id,
                        &(caller_index_info->term_id),
                        sizeof(grn_id));
      grn_obj_unref(ctx, caller_index_info->term_table);
      caller_index_info->term_table = key_type;
    }
  }

  if (!grn_obj_is_text_family_bulk(ctx, index_column_name)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, index_column_name);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s: "
                     "the first argument must be index column name: %.*s",
                     error_message_label,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    caller_index_info_fin(ctx, caller_index_info);
    return ctx->rc;
  }

  caller_index_info->index_column = grn_obj_column(ctx,
                                                   caller_index_info->term_table,
                                                   GRN_TEXT_VALUE(index_column_name),
                                                   GRN_TEXT_LEN(index_column_name));
  if (!caller_index_info->index_column) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s: "
                     "nonexistent object: <%.*s>",
                     error_message_label,
                     (int)GRN_TEXT_LEN(index_column_name),
                     GRN_TEXT_VALUE(index_column_name));
    caller_index_info_fin(ctx, caller_index_info);
    return ctx->rc;
  }

  return GRN_SUCCESS;
}

static grn_obj *
func_index_column_df_ratio(grn_ctx *ctx,
                           int n_args,
                           grn_obj **args,
                           grn_user_data *user_data)
{
  grn_ii *ii;
  caller_index_info caller_index_info;

  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "index_column_df_ratio(): "
                     "wrong number of arguments (%d for 1)", n_args - 1);
    return NULL;
  }

  if (caller_index_info_init(ctx,
                             &caller_index_info,
                             args[0],
                             user_data,
                             "index_column_df_ratio()") != GRN_SUCCESS) {
    return NULL;
  }

  ii = (grn_ii *)caller_index_info.index_column;

  {
    grn_obj *source_table;
    unsigned int n_documents;
    uint32_t n_match_documents;
    double df_ratio;
    grn_obj *df_ratio_value;

    source_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, caller_index_info.index_column));
    n_documents = grn_table_size(ctx, source_table);
    n_match_documents = grn_ii_estimate_size(ctx, ii, caller_index_info.term_id);
    if (n_match_documents > n_documents) {
      n_match_documents = n_documents;
    }
    df_ratio = (double)n_match_documents / (double)n_documents;
    grn_obj_unref(ctx, source_table);

    df_ratio_value = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_FLOAT, 0);
    if (df_ratio_value) {
      GRN_FLOAT_SET(ctx, df_ratio_value, df_ratio);
    }
    caller_index_info_fin(ctx, &caller_index_info);
    return df_ratio_value;
  }
}

static grn_obj *
func_index_column_source_records(grn_ctx *ctx,
                                 int n_args,
                                 grn_obj **args,
                                 grn_user_data *user_data)
{
  grn_ii *ii;
  caller_index_info caller_index_info;
  int64_t limit = -1;

  if (n_args < 1 || n_args > 2) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "index_column_source_records(): "
                     "wrong number of arguments (%d for 1..2)", n_args - 1);
    return NULL;
  }

  if (caller_index_info_init(ctx,
                             &caller_index_info,
                             args[0],
                             user_data,
                             "index_column_source_records()") != GRN_SUCCESS) {
    return NULL;
  }

  if (n_args == 2) {
    grn_obj *options = args[1];

    switch (options->header.type) {
    case GRN_TABLE_HASH_KEY :
      {
        grn_hash_cursor *cursor;
        void *key;
        int key_size;
        cursor = grn_hash_cursor_open(ctx, (grn_hash *)options,
                                      NULL, 0, NULL, 0,
                                      0, -1, 0);
        if (!cursor) {
          GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                           "index_column_source_records(): failed to open cursor for options");
          caller_index_info_fin(ctx, &caller_index_info);
          return NULL;
        }
        while (grn_hash_cursor_next(ctx, cursor) != GRN_ID_NIL) {
          void *value;
          grn_obj *proc_value;
          grn_hash_cursor_get_key_value(ctx, cursor, &key, &key_size, &value);
          proc_value = value;

#define KEY_EQUAL(name)                                                 \
          (key_size == strlen(name) && memcmp(key, name, strlen(name)) == 0)

          if (KEY_EQUAL("limit")) {
            limit = grn_plugin_proc_get_value_int64(ctx,
                                                    proc_value,
                                                    limit,
                                                    "index_column_source_records()");
            if (ctx->rc != GRN_SUCCESS) {
              grn_hash_cursor_close(ctx, cursor);
              caller_index_info_fin(ctx, &caller_index_info);
              return NULL;
            }
          } else {
            GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                             "index_column_source_records(): unknown option name: <%.*s>",
                             key_size, (char *)key);
            grn_hash_cursor_close(ctx, cursor);
            caller_index_info_fin(ctx, &caller_index_info);
            return NULL;
          }
#undef KEY_EQUAL
        }
        grn_hash_cursor_close(ctx, cursor);
      }
      break;
    default :
      {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, options);
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "index_column_source_records(): "
                         "2nd argument must be object literal: <%.*s>",
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        caller_index_info_fin(ctx, &caller_index_info);
        return NULL;
      }
    }
  }

  ii = (grn_ii *)caller_index_info.index_column;

  {
    grn_obj *records;

    records = grn_plugin_proc_alloc(ctx, user_data,
                                    grn_obj_get_range(ctx, caller_index_info.index_column),
                                    GRN_OBJ_VECTOR);
    if (!records) {
      caller_index_info_fin(ctx, &caller_index_info);
      return NULL;
    }

    {
      grn_ii_cursor *ii_cursor;
      grn_posting *posting;
      int64_t n_records = 0;
      ii_cursor = grn_ii_cursor_open(ctx, ii, caller_index_info.term_id,
                                     GRN_ID_NIL, GRN_ID_MAX,
                                     grn_ii_get_n_elements(ctx, ii), 0);
      if (ii_cursor) {
        while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
          if (limit > 0 && n_records >= limit) {
            break;
          }
          GRN_RECORD_PUT(ctx, records, posting->rid);
          n_records++;
        }
        grn_ii_cursor_close(ctx, ii_cursor);
      }
    }
    caller_index_info_fin(ctx, &caller_index_info);
    return records;
  }
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_obj *selector_proc;

  selector_proc = grn_proc_create(ctx, "index_column_df_ratio_between", -1,
                                  GRN_PROC_FUNCTION,
                                  NULL, NULL, NULL, 0, NULL);
  grn_proc_set_selector(ctx, selector_proc,
                        selector_index_column_df_ratio_between);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);

  grn_proc_create(ctx, "index_column_df_ratio", -1,
                  GRN_PROC_FUNCTION,
                  func_index_column_df_ratio, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "index_column_source_records", -1,
                  GRN_PROC_FUNCTION,
                  func_index_column_source_records, NULL, NULL, 0, NULL);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
