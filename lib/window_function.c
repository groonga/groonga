/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016-2017 Brazil

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

grn_rc
grn_window_init(grn_ctx *ctx,
                grn_window *window,
                grn_obj *table,
                grn_obj *grouped_table)
{
  GRN_API_ENTER;

  window->table = table;
  window->grouped_table = grouped_table;
  GRN_RECORD_INIT(&(window->ids), GRN_OBJ_VECTOR, grn_obj_id(ctx, table));
  window->n_ids = 0;
  window->current_index = 0;
  window->direction = GRN_WINDOW_DIRECTION_ASCENDING;
  window->sort_keys = NULL;
  window->n_sort_keys = 0;

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_window_fin(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  GRN_OBJ_FIN(ctx, &(window->ids));

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_id
grn_window_next(grn_ctx *ctx, grn_window *window)
{
  grn_id next_id;

  GRN_API_ENTER;

  if (!window) {
    GRN_API_RETURN(GRN_ID_NIL);
  }

  if (window->direction == GRN_WINDOW_DIRECTION_ASCENDING) {
    if (window->current_index >= window->n_ids) {
      GRN_API_RETURN(GRN_ID_NIL);
    }
  } else {
    if (window->current_index < 0) {
      GRN_API_RETURN(GRN_ID_NIL);
    }
  }

  next_id = GRN_RECORD_VALUE_AT(&(window->ids), window->current_index);
  if (window->direction == GRN_WINDOW_DIRECTION_ASCENDING) {
    window->current_index++;
  } else {
    window->current_index--;
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
    window->current_index = 0;
  } else {
    window->current_index = window->n_ids - 1;
  }

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_obj *
grn_window_get_table(grn_ctx *ctx, grn_window *window)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][rewind] window is NULL");
    GRN_API_RETURN(NULL);
  }

  GRN_API_RETURN(window->table);
}

grn_rc
grn_window_set_direction(grn_ctx *ctx,
                         grn_window *window,
                         grn_window_direction direction)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][set][direction] window is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  switch (direction) {
  case GRN_WINDOW_DIRECTION_ASCENDING :
    window->direction = direction;
    window->current_index = 0;
    break;
  case GRN_WINDOW_DIRECTION_DESCENDING :
    window->direction = direction;
    window->current_index = window->n_ids - 1;
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT,
        "[window][set][direction] direction must be "
        "GRN_WINDOW_DIRECTION_ASCENDING(%d) or "
        "GRN_WINDOW_DIRECTION_DESCENDING(%d): %d",
        GRN_WINDOW_DIRECTION_ASCENDING,
        GRN_WINDOW_DIRECTION_DESCENDING,
        direction);
    GRN_API_RETURN(ctx->rc);
    break;
  }

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_window_set_sort_keys(grn_ctx *ctx,
                         grn_window *window,
                         grn_table_sort_key *sort_keys,
                         size_t n_sort_keys)
{
  GRN_API_ENTER;

  if (!window) {
    ERR(GRN_INVALID_ARGUMENT, "[window][set][sort-keys] window is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (sort_keys) {
    grn_obj *sorted;

    sorted = grn_table_create(ctx,
                              NULL, 0, NULL,
                              GRN_OBJ_TABLE_NO_KEY,
                              NULL,
                              window->grouped_table);
    if (!sorted) {
      ERR(ctx->rc,
          "[window][set][sort-keys] "
          "failed to create a table to store sorted records: %s",
          ctx->errbuf);
      GRN_API_RETURN(ctx->rc);
    }

    grn_table_sort(ctx,
                   window->grouped_table,
                   0, -1,
                   sorted,
                   sort_keys, n_sort_keys);
    if (ctx->rc != GRN_SUCCESS) {
      ERR(ctx->rc,
          "[window][set][sort-keys] "
          "failed to sort: %s",
          ctx->errbuf);
      grn_obj_unlink(ctx, sorted);
      GRN_API_RETURN(ctx->rc);
    }

    GRN_BULK_REWIND(&(window->ids));
    GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
      void *value;
      grn_id record_id;

      grn_table_cursor_get_value(ctx, cursor, &value);
      if (window->table == window->grouped_table) {
        record_id = *((grn_id *)value);
      } else {
        grn_id grouped_record_id;
        grouped_record_id = *((grn_id *)value);
        grn_table_get_key(ctx,
                          window->grouped_table,
                          grouped_record_id,
                          &record_id, sizeof(grn_id));
      }
      GRN_RECORD_PUT(ctx, &(window->ids), record_id);
    } GRN_TABLE_EACH_END(ctx, cursor);

    grn_obj_close(ctx, sorted);
  } else {
    GRN_BULK_REWIND(&(window->ids));
    GRN_TABLE_EACH_BEGIN(ctx, window->grouped_table, cursor, id) {
      grn_id record_id;
      if (window->table == window->grouped_table) {
        record_id = id;
      } else {
        grn_table_get_key(ctx,
                          window->grouped_table,
                          id,
                          &record_id, sizeof(grn_id));
      }
      GRN_RECORD_PUT(ctx, &(window->ids), record_id);
    } GRN_TABLE_EACH_END(ctx, cursor);
  }

  window->n_ids = GRN_BULK_VSIZE(&(window->ids)) / sizeof(grn_id);

  grn_window_set_direction(ctx, window, window->direction);

  window->sort_keys = sort_keys;
  window->n_sort_keys = n_sort_keys;

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_bool
grn_window_is_sorted(grn_ctx *ctx, grn_window *window)
{
  grn_bool is_sorted;
  GRN_API_ENTER;
  is_sorted = (window && window->n_sort_keys > 0);
  GRN_API_RETURN(is_sorted);
}

grn_obj *
grn_window_function_create(grn_ctx *ctx,
                           const char *name,
                           int name_size,
                           grn_window_function_func func)
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
    ERR(GRN_WINDOW_FUNCTION_ERROR,
        "[window-function][%.*s] failed to create proc: %s",
        name_size, name,
        ctx->errbuf);
    GRN_API_RETURN(NULL);
  }

  {
    grn_proc *proc = (grn_proc *)window_function;
    proc->callbacks.window_function = func;
  }

  GRN_API_RETURN(window_function);
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
  if (call->nargs != (expr->codes_curr - 2)) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_rc
grn_expr_call_window_function(grn_ctx *ctx,
                              grn_obj *output_column,
                              grn_window *window,
                              grn_obj *window_function_call)
{
  grn_rc rc;
  grn_expr *expr = (grn_expr *)window_function_call;
  grn_proc *proc;
  int32_t i, n;
  grn_obj args;

  proc = (grn_proc *)(expr->codes[0].value);

  GRN_PTR_INIT(&args, GRN_OBJ_VECTOR, GRN_ID_NIL);
  n = expr->codes_curr - 1;
  for (i = 1; i < n; i++) {
    /* TODO: Check op. */
    GRN_PTR_PUT(ctx, &args, expr->codes[i].value);
  }
  rc = proc->callbacks.window_function(ctx,
                                       output_column,
                                       window,
                                       (grn_obj **)GRN_BULK_HEAD(&args),
                                       GRN_BULK_VSIZE(&args) / sizeof(grn_obj *));
  GRN_OBJ_FIN(ctx, &args);

  return rc;
}

static grn_rc
grn_table_apply_window_function_per_group(grn_ctx *ctx,
                                          grn_obj *table,
                                          grn_obj *grouped_table,
                                          grn_window_definition *definition,
                                          grn_obj *output_column,
                                          grn_obj *window_function_call)
{
  grn_window window;

  grn_window_init(ctx, &window, table, grouped_table);
  grn_window_set_sort_keys(ctx, &window,
                           definition->sort_keys,
                           definition->n_sort_keys);
  grn_expr_call_window_function(ctx,
                                output_column,
                                &window,
                                window_function_call);
  grn_window_fin(ctx, &window);

  return GRN_SUCCESS;
}

grn_rc
grn_table_apply_window_function(grn_ctx *ctx,
                                grn_obj *table,
                                grn_obj *output_column,
                                grn_window_definition *definition,
                                grn_obj *window_function_call)
{
  grn_rc rc;

  GRN_API_ENTER;

  if (!table) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply][window-function] table is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (!grn_expr_is_window_function_call(ctx, window_function_call)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, window_function_call);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply][window-function] must be window function call: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  if (definition->group_keys) {
    grn_table_group_result grouped;
    grn_obj subrecs;

    grouped.table = NULL;
    grouped.key_begin = 0;
    grouped.key_end = definition->n_group_keys;
    grouped.limit = -1;
    grouped.flags = GRN_TABLE_GROUP_CALC_COUNT;
    grouped.op = 0;
    grouped.max_n_subrecs = grn_table_size(ctx, table);
    grouped.calc_target = NULL;

    /* TODO: grn_table_group() should support table output for sub records? */
    GRN_RECORD_INIT(&subrecs, GRN_OBJ_VECTOR, grn_obj_id(ctx, table));
    grn_bulk_reserve(ctx, &subrecs, sizeof(grn_obj *) * grouped.max_n_subrecs);
    grn_table_group(ctx,
                    table,
                    definition->group_keys,
                    definition->n_group_keys,
                    &grouped,
                    1);
    GRN_TABLE_EACH_BEGIN(ctx, grouped.table, cursor, id) {
      grn_obj *grouped_table;

      grouped_table = grn_table_create(ctx,
                                       NULL, 0,
                                       NULL,
                                       GRN_TABLE_HASH_KEY,
                                       table,
                                       NULL);
      {
        unsigned int i, n;
        GRN_BULK_REWIND(&subrecs);
        n = grn_table_get_subrecs(ctx,
                                  grouped.table,
                                  id,
                                  (grn_id *)GRN_BULK_HEAD(&subrecs),
                                  NULL,
                                  grouped.max_n_subrecs);
        for (i = 0; i < n; i++) {
          grn_id subrec_id;
          subrec_id = GRN_RECORD_VALUE_AT(&subrecs, i);
          grn_table_add(ctx, grouped_table, &subrec_id, sizeof(grn_id), NULL);
        }
      }
      /* TODO: rc handling */
      rc = grn_table_apply_window_function_per_group(ctx,
                                                     table,
                                                     grouped_table,
                                                     definition,
                                                     output_column,
                                                     window_function_call);
      grn_obj_close(ctx, grouped_table);
    } GRN_TABLE_EACH_END(ctx, cursor);

    grn_obj_close(ctx, grouped.table);
    GRN_OBJ_FIN(ctx, &subrecs);
  } else {
    rc = grn_table_apply_window_function_per_group(ctx,
                                                   table,
                                                   table,
                                                   definition,
                                                   output_column,
                                                   window_function_call);
  }

  GRN_API_RETURN(rc);
}
