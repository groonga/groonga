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

#include <groonga/plugin.h>

static grn_obj *
func_in_records(grn_ctx *ctx,
                int n_args,
                grn_obj **args,
                grn_user_data *user_data)
{
  grn_obj *found;
  grn_obj *condition_table;
  grn_obj condition_columns;
  grn_operator_exec_func **condition_functions = NULL;
  int i;

  found = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_BOOL, 0);
  if (!found) {
    return NULL;
  }
  GRN_BOOL_SET(ctx, found, GRN_FALSE);

  if (n_args < 4) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): wrong number of arguments (%d for 4..)",
                     n_args);
    return found;
  }

  if ((n_args % 3) != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): the number of arguments must be 1 + 3n (%d)",
                     n_args);
    return found;
  }

  condition_table = args[0];
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
    return found;
  }

  GRN_PTR_INIT(&condition_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  for (i = 1; i < n_args; i += 3) {
    int column_name_i = i + 2;
    grn_obj *column_name;
    grn_obj *condition_column;

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

  condition_functions =
    GRN_PLUGIN_MALLOCN(ctx, grn_operator_exec_func *, (n_args - 1 / 3));
  for (i = 1; i < n_args; i += 3) {
    int nth = (i - 1) / 3;
    int mode_name_i = i + 1;
    grn_obj *mode_name;
    grn_operator mode;

    mode_name = args[mode_name_i];
    mode = grn_proc_option_value_mode(ctx,
                                      mode_name,
                                      GRN_OP_EQUAL,
                                      "in_records()");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }

    switch (mode) {
    case GRN_OP_EQUAL :
      condition_functions[nth] = grn_operator_exec_equal;
      break;
    case GRN_OP_NOT_EQUAL :
      condition_functions[nth] = grn_operator_exec_not_equal;
      break;
    case GRN_OP_LESS :
      condition_functions[nth] = grn_operator_exec_less;
      break;
    case GRN_OP_GREATER :
      condition_functions[nth] = grn_operator_exec_greater;
      break;
    case GRN_OP_LESS_EQUAL :
      condition_functions[nth] = grn_operator_exec_less_equal;
      break;
    case GRN_OP_GREATER_EQUAL :
      condition_functions[nth] = grn_operator_exec_greater_equal;
      break;
    case GRN_OP_MATCH :
      condition_functions[nth] = grn_operator_exec_match;
      break;
    case GRN_OP_PREFIX :
      condition_functions[nth] = grn_operator_exec_prefix;
      break;
    case GRN_OP_REGEXP :
      condition_functions[nth] = grn_operator_exec_regexp;
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
  }

  {
    grn_obj condition_column_value;

    GRN_VOID_INIT(&condition_column_value);
    GRN_TABLE_EACH_BEGIN(ctx, condition_table, cursor, id) {
      grn_bool found_record = GRN_TRUE;

      for (i = 1; i < n_args; i += 3) {
        int nth = (i - 1) / 3;
        grn_obj *condition_column;
        grn_operator_exec_func *condition_function;
        grn_obj *value = args[i];

        condition_column = GRN_PTR_VALUE_AT(&condition_columns, nth);
        condition_function = condition_functions[nth];
        if (grn_obj_is_data_column(ctx, condition_column)) {
          grn_bool found_value = GRN_FALSE;

          GRN_BULK_REWIND(&condition_column_value);
          grn_obj_get_value(ctx,
                            condition_column,
                            id,
                            &condition_column_value);

          found_value = condition_function(ctx,
                                           value,
                                           &condition_column_value);
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
  }

exit :
  GRN_PLUGIN_FREE(ctx, condition_functions);

  for (i = 1; i < n_args; i += 2) {
    int nth = (i - 1) / 3;
    grn_obj *condition_column;
    condition_column = GRN_PTR_VALUE_AT(&condition_columns, nth);
    if (condition_column && condition_column->header.type == GRN_ACCESSOR) {
      grn_obj_unlink(ctx, condition_column);
    }
  }
  GRN_OBJ_FIN(ctx, &condition_columns);

  return found;
}

void
grn_proc_init_in_records(grn_ctx *ctx)
{
  grn_proc_create(ctx, "in_records", -1, GRN_PROC_FUNCTION,
                  func_in_records, NULL, NULL, 0, NULL);
}
