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
  grn_obj *table;
  grn_obj columns;
  int i;

  found = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_BOOL, 0);
  if (!found) {
    return NULL;
  }
  GRN_BOOL_SET(ctx, found, GRN_FALSE);

  if (n_args < 3) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): wrong number of arguments (%d for 3..)",
                     n_args);
    return found;
  }

  if ((n_args % 2) != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): the number of arguments must be odd (%d)",
                     n_args);
    return found;
  }

  table = args[0];
  if (!grn_obj_is_table(ctx, table)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "in_records(): the first argument must be a table: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return found;
  }

  GRN_PTR_INIT(&columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  for (i = 1; i < n_args; i += 2) {
    grn_obj *column_name = args[i + 1];
    grn_obj *column;

    if (!grn_obj_is_text_family_bulk(ctx, column_name)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, table);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument must be column name as string: "
                       "<%.*s>",
                       i + 1,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }

    column = grn_obj_column(ctx, table,
                            GRN_TEXT_VALUE(column_name),
                            GRN_TEXT_LEN(column_name));
    if (!column) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, table);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "in_records(): "
                       "the %dth argument must be existing column name: "
                       "<%.*s>",
                       i + 1,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }
    GRN_PTR_PUT(ctx, &columns, column);
  }

  {
    grn_obj column_value;

    GRN_VOID_INIT(&column_value);
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
      grn_bool found_record = GRN_TRUE;

      for (i = 1; i < n_args; i += 2) {
        grn_obj *value = args[i];
        grn_obj *column = GRN_PTR_VALUE_AT(&columns, (i - 1) / 2);

        if (grn_obj_is_data_column(ctx, column)) {
          grn_bool found_value = GRN_FALSE;

          GRN_BULK_REWIND(&column_value);
          grn_obj_get_value(ctx, column, id, &column_value);

          found_value = grn_operator_exec_equal(ctx, value, &column_value);
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
    GRN_OBJ_FIN(ctx, &column_value);
  }

exit :
  for (i = 1; i < n_args; i += 2) {
    grn_obj *column = GRN_PTR_VALUE_AT(&columns, (i - 1) / 2);
    if (column->header.type == GRN_ACCESSOR) {
      grn_obj_unlink(ctx, column);
    }
  }
  GRN_OBJ_FIN(ctx, &columns);

  return found;
}

void
grn_proc_init_in_records(grn_ctx *ctx)
{
  grn_proc_create(ctx, "in_records", -1, GRN_PROC_FUNCTION,
                  func_in_records, NULL, NULL, 0, NULL);
}
