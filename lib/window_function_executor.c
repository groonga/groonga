/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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

#include "grn_ctx.h"
#include "grn_window_function_executor.h"

grn_rc
grn_window_function_executor_init(grn_ctx *ctx,
                                  grn_window_function_executor *executor)
{
  GRN_API_ENTER;

  GRN_TEXT_INIT(&(executor->tag), 0);
  GRN_PTR_INIT(&(executor->tables), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_BOOL_INIT(&(executor->is_context_tables), 0);
  GRN_TEXT_INIT(&(executor->source), 0);
  GRN_TEXT_INIT(&(executor->sort_keys), 0);
  GRN_TEXT_INIT(&(executor->group_keys), 0);
  GRN_TEXT_INIT(&(executor->output_column_name), 0);
  executor->context.sort_keys = NULL;
  executor->context.n_sort_keys = 0;
  executor->context.group_keys = NULL;
  executor->context.n_group_keys = 0;
  executor->context.window_sort_keys = NULL;
  executor->context.n_window_sort_keys = 0;
  executor->context.sorted = NULL;
  executor->values.n = 0;
  executor->values.previous = NULL;
  executor->values.current = NULL;
  GRN_PTR_INIT(&(executor->window_function_calls), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(&(executor->output_columns), GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_window_init(ctx, &(executor->window));

  GRN_API_RETURN(ctx->rc);
}

static void
grn_window_function_executor_rewind(grn_ctx *ctx,
                                    grn_window_function_executor *executor)
{
  grn_obj *window_function_calls = &(executor->window_function_calls);
  const size_t n_calls = GRN_PTR_VECTOR_SIZE(window_function_calls);
  for (size_t i = 0; i < n_calls; i++) {
    grn_obj *window_function_call = GRN_PTR_VALUE_AT(window_function_calls, i);
    if (window_function_call) {
      grn_obj_close(ctx, window_function_call);
    }
  }
  GRN_BULK_REWIND(window_function_calls);

  grn_obj *output_columns = &(executor->output_columns);
  const size_t n_output_columns = GRN_PTR_VECTOR_SIZE(output_columns);
  for (size_t i = 0; i < n_output_columns; i++) {
    grn_obj *output_column = GRN_PTR_VALUE_AT(output_columns, i);
    if (grn_obj_is_accessor(ctx, output_column)) {
      grn_obj_close(ctx, output_column);
    }
  }
  GRN_BULK_REWIND(output_columns);
}

grn_rc
grn_window_function_executor_fin(grn_ctx *ctx,
                                 grn_window_function_executor *executor)
{
  GRN_API_ENTER;

  if (!executor) {
    GRN_API_RETURN(GRN_SUCCESS);
  }

  grn_window_fin(ctx, &(executor->window));

  grn_window_function_executor_rewind(ctx, executor);
  GRN_OBJ_FIN(ctx, &(executor->output_columns));
  GRN_OBJ_FIN(ctx, &(executor->window_function_calls));

  if (executor->values.n > 0) {
    for (size_t i = 0; i < executor->values.n; i++) {
      GRN_OBJ_FIN(ctx, &(executor->values.previous[i]));
      GRN_OBJ_FIN(ctx, &(executor->values.current[i]));
    }
    GRN_FREE(executor->values.previous);
    GRN_FREE(executor->values.current);
  }

  if (executor->context.sorted) {
    grn_obj_close(ctx, executor->context.sorted);
  }
  if (executor->context.window_sort_keys) {
    GRN_FREE(executor->context.window_sort_keys);
  }
  if (executor->context.group_keys) {
    grn_table_sort_key_close(ctx,
                             executor->context.group_keys,
                             executor->context.n_group_keys);
  }
  if (executor->context.sort_keys) {
    grn_table_sort_key_close(ctx,
                             executor->context.sort_keys,
                             executor->context.n_sort_keys);
  }

  GRN_OBJ_FIN(ctx, &(executor->output_column_name));
  GRN_OBJ_FIN(ctx, &(executor->group_keys));
  GRN_OBJ_FIN(ctx, &(executor->sort_keys));
  GRN_OBJ_FIN(ctx, &(executor->source));
  GRN_OBJ_FIN(ctx, &(executor->is_context_tables));
  GRN_OBJ_FIN(ctx, &(executor->tables));
  GRN_OBJ_FIN(ctx, &(executor->tag));

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_window_function_executor *
grn_window_function_executor_open(grn_ctx *ctx)
{
  GRN_API_ENTER;

  grn_window_function_executor *executor;

  executor = GRN_CALLOC(sizeof(grn_window_function_executor));
  if (!executor) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc,
        "[window-function-executor][open] failed to allocate: %s",
        errbuf);
    GRN_API_RETURN(NULL);
  }

  grn_window_function_executor_init(ctx, executor);

  if (ctx->rc != GRN_SUCCESS) {
    GRN_FREE(executor);
    executor = NULL;
  }

  GRN_API_RETURN(executor);
}

grn_rc
grn_window_function_executor_close(grn_ctx *ctx,
                                   grn_window_function_executor *executor)
{
  GRN_API_ENTER;

  if (!executor) {
    GRN_API_RETURN(GRN_SUCCESS);
  }

  grn_window_function_executor_fin(ctx, executor);
  GRN_FREE(executor);

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_window_function_executor_set_tag(grn_ctx *ctx,
                                     grn_window_function_executor *executor,
                                     const char *tag,
                                     size_t tag_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "[window-function-executor][tag][set] executor is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->tag), tag, tag_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_add_table(grn_ctx *ctx,
                                       grn_window_function_executor *executor,
                                       grn_obj *table)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][table][add] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_function_executor_rewind(ctx, executor);

  GRN_PTR_PUT(ctx, &(executor->tables), table);
  GRN_BOOL_PUT(ctx, &(executor->is_context_tables), false);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_add_context_table(grn_ctx *ctx,
                                               grn_window_function_executor *executor,
                                               grn_obj *table)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][context-table][add] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_function_executor_rewind(ctx, executor);

  GRN_PTR_PUT(ctx, &(executor->tables), table);
  GRN_BOOL_PUT(ctx, &(executor->is_context_tables), true);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_source(grn_ctx *ctx,
                                        grn_window_function_executor *executor,
                                        const char *source,
                                        size_t source_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][source][set] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->source), source, source_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_sort_keys(grn_ctx *ctx,
                                           grn_window_function_executor *executor,
                                           const char *sort_keys,
                                           size_t sort_keys_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][sort-keys][set] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->sort_keys), sort_keys, sort_keys_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_group_keys(grn_ctx *ctx,
                                            grn_window_function_executor *executor,
                                            const char *group_keys,
                                            size_t group_keys_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][group-keys][set] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->group_keys), group_keys, group_keys_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_output_column_name(grn_ctx *ctx,
                                                    grn_window_function_executor *executor,
                                                    const char *name,
                                                    size_t name_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][output-column-name][set] "
        "executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->output_column_name), name, name_size);

  GRN_API_RETURN(ctx->rc);
}

static bool
grn_window_function_executor_is_ascending(grn_ctx *ctx,
                                          grn_window_function_executor *executor)
{
  const size_t sort_keys_len = GRN_TEXT_LEN(&(executor->sort_keys));
  if (sort_keys_len == 0) {
    return true;
  }

  const char *sort_keys = GRN_TEXT_VALUE(&(executor->sort_keys));
  for (size_t i = 0; i < sort_keys_len; i++) {
    switch (sort_keys[i]) {
    case ' ' :
      break;
    case '-' :
      return false;
    default :
      return true;
    }
  }

  return true;
}

grn_rc
grn_window_function_executor_execute(grn_ctx *ctx,
                                     grn_window_function_executor *executor)
{
  const char *tag = "[window-function-executor][execute]";

  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s%s executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)),
        tag);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj *source = &(executor->source);
  if (GRN_TEXT_LEN(source) == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s%s no source",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)),
        tag);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj *output_column_name = &(executor->output_column_name);
  if (GRN_TEXT_LEN(output_column_name) == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s%s no output column",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)),
        tag);
    GRN_API_RETURN(ctx->rc);
  }

  const size_t n_tables = GRN_PTR_VECTOR_SIZE(&(executor->tables));
  if (n_tables == 0) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_function_executor_rewind(ctx, executor);

  const bool is_ascending =
    grn_window_function_executor_is_ascending(ctx, executor);
  bool in_before_context = true;
  bool in_after_context = false;
  for (size_t i = 0; i < n_tables; i++) {
    size_t nth_table = (is_ascending ? i : (n_tables - i - 1));
    grn_obj *table = GRN_PTR_VALUE_AT(&(executor->tables), nth_table);
    const bool is_context_table =
      GRN_BOOL_VALUE_AT(&(executor->is_context_tables), nth_table);
    if (in_before_context) {
      if (!is_context_table) {
        in_before_context = false;
      }
    } else if (!in_after_context) {
      if (is_context_table) {
        in_after_context = true;
      }
    }

    grn_obj *output_column = grn_obj_column(ctx,
                                            table,
                                            GRN_TEXT_VALUE(output_column_name),
                                            GRN_TEXT_LEN(output_column_name));
    if (!output_column) {
      char table_name[GRN_TABLE_MAX_KEY_SIZE];
      int table_name_size;
      table_name_size = grn_obj_name(ctx,
                                     table,
                                     table_name,
                                     GRN_TABLE_MAX_KEY_SIZE);
      if (table_name_size == 0) {
        grn_strcpy(table_name, GRN_TABLE_MAX_KEY_SIZE, "(anonymous)");
        table_name_size = strlen(table_name);
      }
      ERR(GRN_INVALID_ARGUMENT,
          "%.*s%s output column doesn't exist: <%.*s>: <%.*s>",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          table_name_size,
          table_name,
          (int)GRN_TEXT_LEN(output_column_name),
          GRN_TEXT_VALUE(output_column_name));
      GRN_API_RETURN(ctx->rc);
    }
    GRN_PTR_PUT(ctx, &(executor->output_columns), output_column);

    grn_obj *window_function_call;
    grn_obj *record;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, window_function_call, record);
    if (!window_function_call) {
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(GRN_INVALID_ARGUMENT,
          "%.*s%s failed to create expression to compile window function call: "
          "%s",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          message);
      GRN_API_RETURN(ctx->rc);
    }
    GRN_PTR_PUT(ctx, &(executor->window_function_calls), window_function_call);
    grn_expr_parse(ctx,
                   window_function_call,
                   GRN_TEXT_VALUE(source),
                   GRN_TEXT_LEN(source),
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(ctx->rc,
          "%.*s%s failed to parse window function call: <%.*s>: %s",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          (int)GRN_TEXT_LEN(source),
          GRN_TEXT_VALUE(source),
          message);
      GRN_API_RETURN(ctx->rc);
    }

    unsigned int n_sort_keys = 0;
    grn_table_sort_key *sort_keys = NULL;
    if (GRN_TEXT_LEN(&(executor->sort_keys)) > 0) {
      sort_keys =
        grn_table_sort_key_from_str(ctx,
                                    GRN_TEXT_VALUE(&(executor->sort_keys)),
                                    GRN_TEXT_LEN(&(executor->sort_keys)),
                                    table,
                                    &n_sort_keys);
      if (!sort_keys) {
        ERR(ctx->rc,
            "%.*s%s failed to parse sort keys: <%.*s>",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            (int)GRN_TEXT_LEN(&(executor->sort_keys)),
            GRN_TEXT_VALUE(&(executor->sort_keys)));
        GRN_API_RETURN(ctx->rc);
      }
      if (executor->context.sort_keys) {
        grn_table_sort_key_close(ctx,
                                 executor->context.sort_keys,
                                 executor->context.n_sort_keys);
      }
      executor->context.sort_keys = sort_keys;
      executor->context.n_sort_keys = n_sort_keys;
    }

    unsigned int n_group_keys = 0;
    grn_table_sort_key *group_keys = NULL;
    if (GRN_TEXT_LEN(&(executor->group_keys)) > 0) {
      group_keys =
        grn_table_sort_key_from_str(ctx,
                                    GRN_TEXT_VALUE(&(executor->group_keys)),
                                    GRN_TEXT_LEN(&(executor->group_keys)),
                                    table,
                                    &n_group_keys);
      if (!group_keys) {
        ERR(ctx->rc,
            "%.*s%s failed to parse group keys: <%.*s>",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            (int)GRN_TEXT_LEN(&(executor->group_keys)),
            GRN_TEXT_VALUE(&(executor->group_keys)));
        GRN_API_RETURN(ctx->rc);
      }
      if (executor->context.group_keys) {
        grn_table_sort_key_close(ctx,
                                 executor->context.group_keys,
                                 executor->context.n_group_keys);
      }
      executor->context.group_keys = group_keys;
      executor->context.n_group_keys = n_group_keys;
    }

    const size_t n_window_sort_keys = n_sort_keys + n_group_keys;
    if (executor->context.n_window_sort_keys < n_window_sort_keys) {
      if (executor->context.window_sort_keys) {
        GRN_FREE(executor->context.window_sort_keys);
      }
      executor->context.window_sort_keys =
        GRN_MALLOCN(grn_table_sort_key, n_window_sort_keys);
      if (!executor->context.window_sort_keys) {
        grn_rc rc = ctx->rc;
        char message[GRN_CTX_MSGSIZE];
        if (rc == GRN_SUCCESS) {
          rc = GRN_NO_MEMORY_AVAILABLE;
        }
        grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
        ERR(rc,
            "%.*s%s failed to allocate internal sort keys: %s",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            message);
        GRN_API_RETURN(ctx->rc);
      }
      executor->context.n_window_sort_keys = n_window_sort_keys;
    }
    grn_table_sort_key *window_sort_keys = executor->context.window_sort_keys;
    for (size_t j = 0; j < n_group_keys; j++) {
      window_sort_keys[j] = group_keys[j];
    }
    for (size_t j = 0; j < n_sort_keys; j++) {
      window_sort_keys[j + n_group_keys] = sort_keys[j];
    }

    grn_obj *sorted = grn_table_create(ctx,
                                       NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY,
                                       NULL,
                                       table);
    if (!sorted) {
      grn_rc rc = ctx->rc;
      char errbuf[GRN_CTX_MSGSIZE];
      if (rc == GRN_SUCCESS) {
        rc = GRN_NO_MEMORY_AVAILABLE;
      }
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(rc,
          "%.*s%s failed to allocate table to store sorted result: %s",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          errbuf);
      GRN_API_RETURN(ctx->rc);
    }
    if (executor->context.sorted) {
      grn_obj_close(ctx, executor->context.sorted);
    }
    executor->context.sorted = sorted;
    grn_table_sort(ctx,
                   table,
                   0, -1,
                   sorted,
                   window_sort_keys,
                   n_window_sort_keys);

    grn_window_set_is_sorted(ctx, &(executor->window), n_sort_keys > 0);
    if (n_group_keys > 0) {
      if (executor->values.n == 0) {
        executor->values.n = n_group_keys;
        executor->values.previous = GRN_MALLOCN(grn_obj, n_group_keys);
        executor->values.current = GRN_MALLOCN(grn_obj, n_group_keys);
        for (size_t j = 0; j < n_group_keys; j++) {
          GRN_VOID_INIT(&(executor->values.previous[j]));
          GRN_VOID_INIT(&(executor->values.current[j]));
        }
      }
      if (n_group_keys != executor->values.n) {
        ERR(GRN_INVALID_ARGUMENT,
            "%.*s%s the number of group keys in tables is erratic: "
            "<%u>: <%" GRN_FMT_SIZE ">",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            n_group_keys,
            executor->values.n);
        GRN_API_RETURN(ctx->rc);
      }

      GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
        void *value;
        grn_table_cursor_get_value(ctx, cursor, &value);
        grn_id record_id = *((grn_id *)value);

        bool is_group_key_changed = false;
        for (size_t j = 0; j < n_group_keys; j++) {
          size_t reverse_j = n_group_keys - j - 1;
          grn_obj *previous_value = &(executor->values.previous[reverse_j]);
          grn_obj *current_value = &(executor->values.current[reverse_j]);
          grn_obj *group_key = group_keys[reverse_j].key;

          if (is_group_key_changed) {
            GRN_BULK_REWIND(previous_value);
            grn_obj_get_value(ctx, group_key, record_id, previous_value);
          } else {
            GRN_BULK_REWIND(current_value);
            grn_obj_get_value(ctx, group_key, record_id, current_value);
            if ((GRN_BULK_VSIZE(current_value) !=
                 GRN_BULK_VSIZE(previous_value)) ||
                (memcmp(GRN_BULK_HEAD(current_value),
                        GRN_BULK_HEAD(previous_value),
                        GRN_BULK_VSIZE(current_value)) != 0)) {
              is_group_key_changed = true;
              grn_bulk_write_from(ctx,
                                  previous_value,
                                  GRN_BULK_HEAD(current_value),
                                  0,
                                  GRN_BULK_VSIZE(current_value));
            }
          }
        }

        if (is_group_key_changed &&
            !grn_window_is_empty(ctx, &(executor->window))) {
          if (!in_before_context) {
            grn_window_execute(ctx, &(executor->window));
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
          grn_window_reset(ctx, &(executor->window));
          if (in_after_context) {
            break;
          }
        }
        grn_window_add_record(ctx,
                              &(executor->window),
                              table,
                              is_context_table,
                              record_id,
                              window_function_call,
                              output_column);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      } GRN_TABLE_EACH_END(ctx, cursor);
    } else {
      GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
        void *value;
        grn_id record_id;

        grn_table_cursor_get_value(ctx, cursor, &value);
        record_id = *((grn_id *)value);
        grn_window_add_record(ctx,
                              &(executor->window),
                              table,
                              is_context_table,
                              record_id,
                              window_function_call,
                              output_column);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      } GRN_TABLE_EACH_END(ctx, cursor);
    }
  }
  if (ctx->rc == GRN_SUCCESS &&
      !grn_window_is_empty(ctx, &(executor->window))) {
    grn_window_execute(ctx, &(executor->window));
    grn_window_reset(ctx, &(executor->window));
  }

  GRN_API_RETURN(ctx->rc);
}
