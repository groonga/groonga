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

#include "../grn_proc.h"
#include "../grn_db.h"
#include "../grn_store.h"

#include <groonga/plugin.h>

typedef struct {
  int n_conditions;
  grn_obj *condition_table;
  grn_obj condition_columns;
  grn_operator_exec_func **condition_functions;
  grn_ra **ras;
  grn_ra_cache *ra_caches;
  unsigned int *ra_element_sizes;
  grn_obj *ra_values;
} grn_in_records_data;

static void
grn_in_records_data_free(grn_ctx *ctx, grn_in_records_data *data)
{
  int i;

  if (!data) {
    return;
  }

  GRN_PLUGIN_FREE(ctx, data->condition_functions);

  for (i = 0; i < data->n_conditions; i++) {
    grn_obj *condition_column;
    condition_column = GRN_PTR_VALUE_AT(&(data->condition_columns), i);
    if (condition_column && condition_column->header.type == GRN_ACCESSOR) {
      grn_obj_unlink(ctx, condition_column);
    }
  }
  GRN_OBJ_FIN(ctx, &(data->condition_columns));

  for (i = 0; i < data->n_conditions; i++) {
    grn_ra *ra = data->ras[i];
    grn_ra_cache *ra_cache = data->ra_caches + i;
    grn_obj *ra_value = data->ra_values + i;

    GRN_OBJ_FIN(ctx, ra_value);

    if (!ra || !ra_cache) {
      continue;
    }
    GRN_RA_CACHE_FIN(ra, ra_cache);
  }
  GRN_PLUGIN_FREE(ctx, data->ras);
  GRN_PLUGIN_FREE(ctx, data->ra_caches);
  GRN_PLUGIN_FREE(ctx, data->ra_element_sizes);
  GRN_PLUGIN_FREE(ctx, data->ra_values);

  GRN_PLUGIN_FREE(ctx, data);
}

static grn_obj *
func_in_records_init(grn_ctx *ctx,
                     int n_args,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_in_records_data *data;
  grn_obj *condition_table;
  grn_expr_code *codes;
  int n_arg_codes;
  int n_logical_args;
  int n_conditions;
  int i;
  int nth;

  {
    grn_obj *caller;
    grn_expr *expr;
    grn_expr_code *call_code;

    caller = grn_plugin_proc_get_caller(ctx, user_data);
    expr = (grn_expr *)caller;
    call_code = expr->codes + expr->codes_curr - 1;
    n_logical_args = call_code->nargs - 1;
    codes = expr->codes + 1;
    n_arg_codes = expr->codes_curr - 2;
  }

  if (n_logical_args < 4) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): wrong number of arguments (%d for 4..)",
                     n_logical_args);
    return NULL;
  }

  if ((n_logical_args % 3) != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): the number of arguments must be 1 + 3n (%d)",
                     n_logical_args);
    return NULL;
  }

  n_conditions = (n_logical_args - 1) / 3;

  condition_table = codes[0].value;
  if (!grn_obj_is_table(ctx, condition_table)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, condition_table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): the first argument must be a table: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  data = GRN_PLUGIN_CALLOC(ctx, sizeof(grn_in_records_data));
  user_data->ptr = data;

  data->n_conditions = n_conditions;
  data->condition_table = condition_table;
  GRN_PTR_INIT(&(data->condition_columns), GRN_OBJ_VECTOR, GRN_ID_NIL);
  data->condition_functions =
    GRN_PLUGIN_MALLOCN(ctx, grn_operator_exec_func *, n_conditions);
  data->ras = GRN_PLUGIN_MALLOCN(ctx, grn_ra *, n_conditions);
  data->ra_caches = GRN_PLUGIN_MALLOCN(ctx, grn_ra_cache, n_conditions);
  data->ra_element_sizes = GRN_PLUGIN_MALLOCN(ctx, unsigned int, n_conditions);
  data->ra_values = GRN_PLUGIN_MALLOCN(ctx, grn_obj, n_conditions);
  for (i = 0; i < n_conditions; i++) {
    data->ras[i] = NULL;
    GRN_VOID_INIT(data->ra_values + i);
  }

  for (i = 1, nth = 0; i < n_arg_codes; nth++) {
    int value_i = i;
    int mode_name_i;
    grn_obj *mode_name;
    grn_operator mode;
    int column_name_i;
    grn_obj *column_name;
    grn_obj *condition_column;

    value_i += codes[value_i].modify;

    mode_name_i = value_i + 1;
    mode_name = codes[mode_name_i].value;
    mode = grn_proc_option_value_mode(ctx,
                                      mode_name,
                                      GRN_OP_EQUAL,
                                      "in_records()");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }

    switch (mode) {
    case GRN_OP_EQUAL :
      data->condition_functions[nth] = grn_operator_exec_equal;
      break;
    case GRN_OP_NOT_EQUAL :
      data->condition_functions[nth] = grn_operator_exec_not_equal;
      break;
    case GRN_OP_LESS :
      data->condition_functions[nth] = grn_operator_exec_less;
      break;
    case GRN_OP_GREATER :
      data->condition_functions[nth] = grn_operator_exec_greater;
      break;
    case GRN_OP_LESS_EQUAL :
      data->condition_functions[nth] = grn_operator_exec_less_equal;
      break;
    case GRN_OP_GREATER_EQUAL :
      data->condition_functions[nth] = grn_operator_exec_greater_equal;
      break;
    case GRN_OP_MATCH :
      data->condition_functions[nth] = grn_operator_exec_match;
      break;
    case GRN_OP_PREFIX :
      data->condition_functions[nth] = grn_operator_exec_prefix;
      break;
    case GRN_OP_REGEXP :
      data->condition_functions[nth] = grn_operator_exec_regexp;
      break;
    default :
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument is unsupported mode: <%.*s>",
                       mode_name_i,
                       (int)GRN_TEXT_LEN(mode_name),
                       GRN_TEXT_VALUE(mode_name));
      goto exit;
      break;
    }


    column_name_i = mode_name_i + 1;
    column_name = codes[column_name_i].value;
    if (!grn_obj_is_text_family_bulk(ctx, column_name)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, condition_table);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument must be column name as string: "
                       "<%.*s>",
                       column_name_i,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }

    condition_column = grn_obj_column(ctx, condition_table,
                                      GRN_TEXT_VALUE(column_name),
                                      GRN_TEXT_LEN(column_name));
    if (!condition_column) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, condition_table);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument must be existing column name: "
                       "<%.*s>: <%.*s>",
                       column_name_i,
                       (int)GRN_TEXT_LEN(column_name),
                       GRN_TEXT_VALUE(column_name),
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }
    GRN_PTR_PUT(ctx, &(data->condition_columns), condition_column);

    if (condition_column->header.type == GRN_COLUMN_FIX_SIZE) {
      data->ras[nth] = (grn_ra *)condition_column;
      GRN_RA_CACHE_INIT(data->ras[nth], data->ra_caches + nth);
      grn_ra_info(ctx, data->ras[nth], &(data->ra_element_sizes[nth]));
      grn_obj_reinit(ctx,
                     data->ra_values + nth,
                     grn_obj_get_range(ctx, condition_column),
                     0);
    }

    i = column_name_i + 1;
  }

  return NULL;

exit :
  grn_in_records_data_free(ctx, data);

  return NULL;
}

static grn_obj *
func_in_records_next(grn_ctx *ctx,
                     int n_args,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_in_records_data *data = user_data->ptr;
  grn_obj *found;
  grn_obj condition_column_value;
  int i;

  found = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_BOOL, 0);
  if (!found) {
    return NULL;
  }
  GRN_BOOL_SET(ctx, found, GRN_FALSE);

  if (!data) {
    return found;
  }

  GRN_VOID_INIT(&condition_column_value);
  GRN_TABLE_EACH_BEGIN(ctx, data->condition_table, cursor, id) {
    grn_bool found_record = GRN_TRUE;

    for (i = 1; i < n_args; i += 3) {
      int nth = (i - 1) / 3;
      grn_obj *condition_column;
      grn_operator_exec_func *condition_function;
      grn_obj *value = args[i];
      grn_obj *condition_value;

      condition_column = GRN_PTR_VALUE_AT(&(data->condition_columns), nth);
      condition_function = data->condition_functions[nth];
      if (grn_obj_is_data_column(ctx, condition_column)) {
        grn_bool found_value = GRN_FALSE;

        if (data->ras[nth]) {
          void *raw_value;

          raw_value = grn_ra_ref_cache(ctx,
                                       data->ras[nth],
                                       id,
                                       data->ra_caches + nth);
          condition_value = data->ra_values + nth;
          GRN_BULK_REWIND(condition_value);
          grn_bulk_write(ctx,
                         condition_value,
                         raw_value,
                         data->ra_element_sizes[nth]);
        } else {
          condition_value = &condition_column_value;
          GRN_BULK_REWIND(condition_value);
          grn_obj_get_value(ctx,
                            condition_column,
                            id,
                            condition_value);
        }

        found_value = condition_function(ctx, value, condition_value);
        if (ctx->rc != GRN_SUCCESS) {
          found_record = GRN_FALSE;
          break;
        }

        if (!found_value) {
          found_record = GRN_FALSE;
          break;
        }
      } else {
        found_record = GRN_FALSE;
        break;
      }
    }

    if (found_record) {
      GRN_BOOL_SET(ctx, found, GRN_TRUE);
      break;
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &condition_column_value);

  return found;
}

static grn_obj *
func_in_records_fin(grn_ctx *ctx,
                    int n_args,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_in_records_data *data = user_data->ptr;

  grn_in_records_data_free(ctx, data);

  return NULL;
}

static grn_rc
selector_in_records(grn_ctx *ctx,
                    grn_obj *table,
                    grn_obj *index,
                    int n_args,
                    grn_obj **args,
                    grn_obj *res,
                    grn_operator op)
{
  grn_obj *condition_table;
  grn_operator *condition_modes = NULL;
  grn_obj condition_columns;
  int i, nth;

  /* TODO: Enable me when function call is supported. */
  return GRN_FUNCTION_NOT_IMPLEMENTED;

  if (n_args < 5) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): wrong number of arguments (%d for 4..)",
                     n_args - 1);
    return ctx->rc;
  }

  condition_table = args[1];
  if (!grn_obj_is_table(ctx, condition_table)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, condition_table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): the first argument must be a table: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return ctx->rc;
  }

  condition_modes = GRN_PLUGIN_MALLOCN(ctx, grn_operator, (n_args - 2) / 3);
  GRN_PTR_INIT(&condition_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  for (i = 2, nth = 0; i < n_args; i += 3, nth++) {
    int mode_name_i = i + 1;
    int column_name_i = i + 2;
    grn_obj *mode_name;
    grn_operator mode;
    grn_obj *column_name;
    grn_obj *condition_column;

    mode_name = args[mode_name_i];
    mode = grn_proc_option_value_mode(ctx,
                                      mode_name,
                                      GRN_OP_EQUAL,
                                      "in_records()");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }

    condition_modes[nth] = mode;

    column_name = args[column_name_i];
    if (!grn_obj_is_text_family_bulk(ctx, column_name)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, condition_table);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument must be column name as string: "
                       "<%.*s>",
                       column_name_i,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }

    condition_column = grn_obj_column(ctx, condition_table,
                                      GRN_TEXT_VALUE(column_name),
                                      GRN_TEXT_LEN(column_name));
    if (!condition_column) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, condition_table);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument must be existing column name: "
                       "<%.*s>: <%.*s>",
                       column_name_i,
                       (int)GRN_TEXT_LEN(column_name),
                       GRN_TEXT_VALUE(column_name),
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }
    GRN_PTR_PUT(ctx, &condition_columns, condition_column);
  }

  {
    grn_obj condition_column_value;

    GRN_VOID_INIT(&condition_column_value);
    GRN_TABLE_EACH_BEGIN(ctx, condition_table, cursor, id) {
      grn_obj *sub_res = NULL;

      for (i = 2; i < n_args; i += 3) {
        int nth = (i - 2) / 3;
        grn_operator sub_op;
        grn_obj *condition_column;
        grn_operator condition_mode;
        grn_obj *column = args[i];
        grn_obj *expr;
        grn_obj *variable;

        if (nth == 0) {
          sub_op = GRN_OP_OR;
        } else {
          sub_op = GRN_OP_AND;
        }

        condition_column = GRN_PTR_VALUE_AT(&condition_columns, nth);
        condition_mode = condition_modes[nth];

        GRN_BULK_REWIND(&condition_column_value);
        grn_obj_get_value(ctx,
                          condition_column,
                          id,
                          &condition_column_value);

        GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expr, variable);
        if (!expr) {
          GRN_PLUGIN_ERROR(ctx,
                           GRN_INVALID_ARGUMENT,
                           "in_records(): failed to create expression");
          GRN_OBJ_FIN(ctx, &condition_column_value);
          if (sub_res) {
            grn_obj_close(ctx, sub_res);
          }
          goto exit;
        }
        grn_expr_append_obj(ctx, expr, column, GRN_OP_GET_VALUE, 1);
        grn_expr_append_obj(ctx, expr, &condition_column_value, GRN_OP_PUSH, 1);
        grn_expr_append_op(ctx, expr, condition_mode, 2);
        sub_res = grn_table_select(ctx, table, expr, sub_res, sub_op);
        grn_obj_close(ctx, expr);
      }

      if (sub_res) {
        grn_table_setoperation(ctx, res, sub_res, res, op);
        grn_obj_close(ctx, sub_res);
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &condition_column_value);
  }

exit :
  GRN_PLUGIN_FREE(ctx, condition_modes);

  for (i = 2; i < n_args; i += 3) {
    int nth = (i - 2) / 3;
    grn_obj *condition_column;
    condition_column = GRN_PTR_VALUE_AT(&condition_columns, nth);
    if (condition_column && condition_column->header.type == GRN_ACCESSOR) {
      grn_obj_unlink(ctx, condition_column);
    }
  }
  GRN_OBJ_FIN(ctx, &condition_columns);

  return ctx->rc;
}

void
grn_proc_init_in_records(grn_ctx *ctx)
{
  grn_obj *selector_proc;

  selector_proc = grn_proc_create(ctx, "in_records", -1, GRN_PROC_FUNCTION,
                                  func_in_records_init,
                                  func_in_records_next,
                                  func_in_records_fin,
                                  0,
                                  NULL);
  grn_proc_set_selector(ctx, selector_proc, selector_in_records);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
}
