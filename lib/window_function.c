/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016-2017 Brazil
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
#include "grn_db.h"
#include "grn_expr.h"
#include "grn_window_function.h"

#include <string.h>

static void
grn_window_shard_init(grn_ctx *ctx,
                      grn_window_shard *shard,
                      grn_obj *table,
                      bool is_context_table,
                      grn_obj *window_function_call,
                      grn_obj *output_column)
{
  shard->table = table;
  shard->is_context_table = is_context_table;
  shard->window_function_call = window_function_call;
  grn_expr *expr = (grn_expr *)(window_function_call);
  shard->window_function = (grn_proc *)(expr->codes[0].value);
  shard->arguments = grn_obj_open(ctx, GRN_UVECTOR, 0, GRN_ID_NIL);
  int32_t n = expr->codes_curr - 1;
  for (int32_t i = 1; i < n; i++) {
    /* TODO: Check op. */
    GRN_PTR_PUT(ctx, shard->arguments, expr->codes[i].value);
  }
  shard->output_column = output_column;
  shard->ids = grn_obj_open(ctx, GRN_UVECTOR, 0, grn_obj_id(ctx, table));
  shard->current_index = -1;
}

static void
grn_window_shard_fin(grn_ctx *ctx,
                     grn_window_shard *shard)
{
  grn_obj_close(ctx, shard->arguments);
  grn_obj_close(ctx, shard->ids);
}

grn_rc
grn_window_init(grn_ctx *ctx,
                grn_window *window)
{
  GRN_API_ENTER;

  window->shards = NULL;
  window->n_shards = 0;
  window->current_shard = -1;
  window->direction = GRN_WINDOW_DIRECTION_ASCENDING;

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_window_fin(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  grn_window_reset(ctx, window);

  GRN_API_RETURN(ctx->rc);
}

grn_id
grn_window_next(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    GRN_API_RETURN(GRN_ID_NIL);
  }

  if (window->current_shard < 0) {
    GRN_API_RETURN(GRN_ID_NIL);
  }

  grn_window_shard *shard = &(window->shards[window->current_shard]);
  if (window->direction == GRN_WINDOW_DIRECTION_ASCENDING) {
    if (shard->current_index >= GRN_RECORD_VECTOR_SIZE(shard->ids)) {
      if (window->current_shard + 1 < window->n_shards) {
        window->current_shard++;
        shard = &(window->shards[window->current_shard]);
      } else {
        GRN_API_RETURN(GRN_ID_NIL);
      }
    }
  } else {
    if (shard->current_index < 0) {
      if (window->current_shard > 0) {
        window->current_shard--;
        shard = &(window->shards[window->current_shard]);
      } else {
        GRN_API_RETURN(GRN_ID_NIL);
      }
    }
  }

  grn_id next_id = GRN_RECORD_VALUE_AT(shard->ids, shard->current_index);
  if (window->direction == GRN_WINDOW_DIRECTION_ASCENDING) {
    shard->current_index++;
  } else {
    shard->current_index--;
  }

  GRN_API_RETURN(next_id);
}

grn_rc
grn_window_rewind(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][rewind] window is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (window->direction == GRN_WINDOW_DIRECTION_ASCENDING) {
    window->current_shard = 0;
    for (size_t i = 0; i < window->n_shards; i++) {
      grn_window_shard *shard = &(window->shards[i]);
      shard->current_index = 0;
    }
  } else {
    window->current_shard = window->n_shards - 1;
    for (size_t i = 0; i < window->n_shards; i++) {
      grn_window_shard *shard = &(window->shards[i]);
      shard->current_index = GRN_RECORD_VECTOR_SIZE(shard->ids) - 1;
    }
  }

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_obj *
grn_window_get_table(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][table][get] window is NULL");
    GRN_API_RETURN(NULL);
  }

  if (window->current_shard < 0) {
    GRN_API_RETURN(NULL);
  }

  grn_window_shard *shard = &(window->shards[window->current_shard]);
  GRN_API_RETURN(shard->table);
}

bool
grn_window_is_context_table(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][is-context-table] window is NULL");
    GRN_API_RETURN(false);
  }

  if (window->current_shard < 0) {
    GRN_API_RETURN(false);
  }

  grn_window_shard *shard = &(window->shards[window->current_shard]);
  GRN_API_RETURN(shard->is_context_table);
}

grn_obj *
grn_window_get_output_column(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][output-column][get] window is NULL");
    GRN_API_RETURN(NULL);
  }

  if (window->current_shard < 0) {
    GRN_API_RETURN(NULL);
  }

  grn_window_shard *shard = &(window->shards[window->current_shard]);
  if (shard->is_context_table) {
    GRN_API_RETURN(NULL);
  } else {
    GRN_API_RETURN(shard->output_column);
  }
}

size_t
grn_window_get_n_arguments(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][n-arguments][get] window is NULL");
    GRN_API_RETURN(0);
  }

  if (window->current_shard < 0) {
    GRN_API_RETURN(0);
  }

  grn_window_shard *shard = &(window->shards[window->current_shard]);
  GRN_API_RETURN(GRN_PTR_VECTOR_SIZE(shard->arguments));
}

grn_obj *
grn_window_get_argument(grn_ctx *ctx, grn_window *window, size_t i)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][argument][get] window is NULL");
    GRN_API_RETURN(NULL);
  }

  if (window->current_shard < 0) {
    GRN_API_RETURN(NULL);
  }

  grn_window_shard *shard = &(window->shards[window->current_shard]);
  if (i < GRN_PTR_VECTOR_SIZE(shard->arguments)) {
    GRN_API_RETURN(GRN_PTR_VALUE_AT(shard->arguments, i));
  } else {
    GRN_API_RETURN(NULL);
  }
}

grn_rc
grn_window_set_direction(grn_ctx *ctx,
                         grn_window *window,
                         grn_window_direction direction)
{
  GRN_API_ENTER;

  const char *tag = "[window][direction][set]";
  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "%s window is NULL", tag);
    GRN_API_RETURN(ctx->rc);
  }

  switch (direction) {
  case GRN_WINDOW_DIRECTION_ASCENDING :
    window->direction = direction;
    break;
  case GRN_WINDOW_DIRECTION_DESCENDING :
    window->direction = direction;
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT,
        "%s direction must be "
        "GRN_WINDOW_DIRECTION_ASCENDING(%d) or "
        "GRN_WINDOW_DIRECTION_DESCENDING(%d): %d",
        tag,
        GRN_WINDOW_DIRECTION_ASCENDING,
        GRN_WINDOW_DIRECTION_DESCENDING,
        direction);
    GRN_API_RETURN(ctx->rc);
    break;
  }
  grn_window_rewind(ctx, window);

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_window_reset(grn_ctx *ctx,
                 grn_window *window)
{
  GRN_API_ENTER;
  for (size_t i = 0; i < window->n_shards; i++) {
    grn_window_shard *shard = &(window->shards[i]);
    grn_window_shard_fin(ctx, shard);
  }
  if (window->shards) {
    GRN_FREE(window->shards);
    window->shards = NULL;
    window->n_shards = 0;
    window->current_shard = -1;
  }
  GRN_API_RETURN(ctx->rc);
}

static grn_bool
grn_expr_is_window_function_call(grn_ctx *ctx,
                                 grn_obj *window_function_call)
{
  grn_expr *expr = (grn_expr *)window_function_call;
  grn_expr_code *func;
  grn_expr_code *call;

  func = &(expr->codes[0]);
  call = &(expr->codes[expr->codes_curr - 1]);

  if (func->op != GRN_OP_PUSH) {
    return GRN_FALSE;
  }
  if (!grn_obj_is_window_function_proc(ctx, func->value)) {
    return GRN_FALSE;
  }

  if (call->op != GRN_OP_CALL) {
    return GRN_FALSE;
  }
  if (call->nargs != (expr->codes_curr - 1)) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static bool
grn_window_add_record_validate(grn_ctx *ctx,
                               grn_window *window,
                               grn_obj *table,
                               grn_obj *window_function_call,
                               grn_obj *output_column,
                               const char *tag)
{
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT, "%s table is NULL", tag);
    return false;
  }

  if (!grn_expr_is_window_function_call(ctx, window_function_call)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, window_function_call);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be window function call: %.*s",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return false;
  }

  if (!output_column) {
    ERR(GRN_INVALID_ARGUMENT, "%s output column is NULL", tag);
    return false;
  }

  return true;
}

grn_rc
grn_window_add_record(grn_ctx *ctx,
                      grn_window *window,
                      grn_obj *table,
                      bool is_context_table,
                      grn_id record_id,
                      grn_obj *window_function_call,
                      grn_obj *output_column)
{
  GRN_API_ENTER;
  const char *tag = "[window][record][add]";
  if (window->n_shards == 0) {
    if (!grn_window_add_record_validate(ctx,
                                        window,
                                        table,
                                        window_function_call,
                                        output_column,
                                        tag)) {
      GRN_API_RETURN(ctx->rc);
    }
    window->shards = GRN_MALLOCN(grn_window_shard, 1);
    grn_window_shard_init(ctx,
                          &(window->shards[0]),
                          table,
                          is_context_table,
                          window_function_call,
                          output_column);
    window->current_shard = 0;
    window->n_shards = 1;
  } else if (window->shards[window->n_shards - 1].table != table) {
    if (!grn_window_add_record_validate(ctx,
                                        window,
                                        table,
                                        window_function_call,
                                        output_column,
                                        tag)) {
      GRN_API_RETURN(ctx->rc);
    }
    const size_t new_n_shards = window->n_shards + 1;
    grn_window_shard *shards =
      GRN_REALLOC(window->shards, sizeof(grn_window_shard) * new_n_shards);
    if (!shards) {
      grn_rc rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_NO_MEMORY_AVAILABLE;
      }
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(rc,
          "%s failed to expand shards: %s",
          tag,
          message);
      GRN_API_RETURN(ctx->rc);
    }
    window->shards = shards;
    window->n_shards = new_n_shards;
    grn_window_shard_init(ctx,
                          &(window->shards[window->n_shards - 1]),
                          table,
                          is_context_table,
                          window_function_call,
                          output_column);
  }
  GRN_RECORD_PUT(ctx,
                 window->shards[window->n_shards - 1].ids,
                 record_id);
  GRN_API_RETURN(ctx->rc);
}

bool
grn_window_is_empty(grn_ctx *ctx,
                    grn_window *window)
{
  GRN_API_ENTER;
  bool is_empty = true;
  for (size_t i = 0; i < window->n_shards; i++) {
    grn_window_shard *shard = &(window->shards[i]);
    if (GRN_RECORD_VECTOR_SIZE(shard->ids) > 0) {
      is_empty = false;
      break;
    }
  }
  GRN_API_RETURN(is_empty);
}

bool
grn_window_is_sorted(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][is-sorted] window is NULL");
    GRN_API_RETURN(GRN_FALSE);
  }

  GRN_API_RETURN(window->is_sorted);
}

grn_rc
grn_window_set_is_sorted(grn_ctx *ctx, grn_window *window, bool is_sorted)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][is-sorted][set] window is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  window->is_sorted = is_sorted;

  GRN_API_RETURN(ctx->rc);
}

size_t
grn_window_get_size(grn_ctx *ctx,
                    grn_window *window)
{
  GRN_API_ENTER;
  size_t n_ids = 0;
  for (size_t i = 0; i < window->n_shards; i++) {
    grn_window_shard *shard = &(window->shards[i]);
    n_ids += GRN_RECORD_VECTOR_SIZE(shard->ids);
  }
  GRN_API_RETURN(n_ids);
}

grn_obj *
grn_window_function_create(grn_ctx *ctx,
                           const char *name,
                           int name_size,
                           grn_window_function_func *func)
{
  grn_obj *window_function = NULL;

  GRN_API_ENTER;

  if (name_size == -1) {
    name_size = strlen(name);
  }

  window_function = grn_proc_create(ctx,
                                    name,
                                    name_size,
                                    GRN_PROC_WINDOW_FUNCTION,
                                    NULL, NULL, NULL, 0, NULL);
  if (!window_function) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_WINDOW_FUNCTION_ERROR,
        "[window-function][%.*s] failed to create proc: %s",
        name_size, name,
        errbuf);
    GRN_API_RETURN(NULL);
  }

  {
    grn_proc *proc = (grn_proc *)window_function;
    proc->callbacks.window_function = func;
  }

  GRN_API_RETURN(window_function);
}

grn_rc
grn_window_execute(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (window->n_shards == 0) {
    GRN_API_RETURN(ctx->rc);
  }
  bool have_non_context_table = false;
  size_t i;
  for (i = 0; i < window->n_shards; i++) {
    if (!window->shards[i].is_context_table) {
      have_non_context_table = true;
      break;
    }
  }
  if (!have_non_context_table) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_rewind(ctx, window);
  grn_window_shard *shard = &(window->shards[window->current_shard]);
  grn_window_function_func *window_function_func =
    shard->window_function->callbacks.window_function;
  grn_rc rc = window_function_func(ctx,
                                   shard->output_column,
                                   window,
                                   (grn_obj **)GRN_BULK_HEAD(shard->arguments),
                                   GRN_PTR_VECTOR_SIZE(shard->arguments));

  GRN_API_RETURN(rc);
}

/* Deprecated since 9.0.2. */
grn_rc
grn_table_apply_window_function(grn_ctx *ctx,
                                grn_obj *table,
                                grn_obj *output_column,
                                grn_window_definition *definition,
                                grn_obj *window_function_call)
{
  GRN_API_ENTER;

  if (!table) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply][window-function] table is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  const size_t n_sort_keys = definition->n_group_keys + definition->n_sort_keys;
  grn_table_sort_key *sort_keys = GRN_MALLOCN(grn_table_sort_key, n_sort_keys);
  if (!sort_keys) {
    grn_rc rc = ctx->rc;
    char errbuf[GRN_CTX_MSGSIZE];
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(rc,
        "[table][apply][window-function] "
        "failed to allocate internal sort keys: %s",
        errbuf);
    GRN_API_RETURN(ctx->rc);
  }

  for (size_t i = 0; i < definition->n_group_keys; i++) {
    sort_keys[i] = definition->group_keys[i];
  }
  for (size_t i = 0; i < definition->n_sort_keys; i++) {
    sort_keys[i + definition->n_group_keys] = definition->sort_keys[i];
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
    GRN_FREE(sort_keys);
    ERR(rc,
        "[table][apply][window-function] "
        "failed to allocate table to store sorted result: %s",
        errbuf);
    GRN_API_RETURN(ctx->rc);
  }
  grn_table_sort(ctx,
                 table,
                 0, -1,
                 sorted,
                 sort_keys, n_sort_keys);

  grn_window window;
  grn_window_init(ctx, &window);
  grn_window_set_is_sorted(ctx, &window, definition->n_sort_keys > 0);
  if (definition->n_group_keys > 0) {
    grn_obj *previous_values = GRN_MALLOCN(grn_obj, definition->n_group_keys);
    grn_obj *current_values = GRN_MALLOCN(grn_obj, definition->n_group_keys);

    const size_t n = definition->n_group_keys;
    for (size_t i = 0; i < n; i++) {
      GRN_VOID_INIT(&(previous_values[i]));
      GRN_VOID_INIT(&(current_values[i]));
    }

    GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
      void *value;
      grn_table_cursor_get_value(ctx, cursor, &value);
      const grn_id record_id = *((grn_id *)value);

      bool is_group_key_changed = false;
      for (size_t i = 0; i < n; i++) {
        const size_t reverse_i = n - i - 1;
        grn_obj *previous_value = &(previous_values[reverse_i]);
        grn_obj *current_value = &(current_values[reverse_i]);
        grn_obj *group_key = definition->group_keys[reverse_i].key;

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

      if (is_group_key_changed && !grn_window_is_empty(ctx, &window)) {
        grn_window_execute(ctx, &window);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
        grn_window_reset(ctx, &window);
      }
      grn_window_add_record(ctx,
                            &window,
                            table,
                            false,
                            record_id,
                            window_function_call,
                            output_column);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
    for (size_t i = 0; i < definition->n_group_keys; i++) {
      GRN_OBJ_FIN(ctx, &(previous_values[i]));
      GRN_OBJ_FIN(ctx, &(current_values[i]));
    }
    GRN_FREE(previous_values);
    GRN_FREE(current_values);
  } else {
    GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
      void *value;
      grn_table_cursor_get_value(ctx, cursor, &value);
      const grn_id record_id = *((grn_id *)value);
      grn_window_add_record(ctx,
                            &window,
                            table,
                            false,
                            record_id,
                            window_function_call,
                            output_column);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
  }
  if (ctx->rc == GRN_SUCCESS && !grn_window_is_empty(ctx, &window)) {
    grn_window_execute(ctx, &window);
  }

  grn_window_fin(ctx, &window);

  grn_obj_close(ctx, sorted);

  GRN_FREE(sort_keys);

  GRN_API_RETURN(ctx->rc);
}
