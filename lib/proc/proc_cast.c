/*
  Copyright(C) 2018 Horimoto Yasuhiro <horimoto@clear-code.com>

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
#include "../grn_expr.h"

#include <groonga/plugin.h>

static grn_obj *
func_cast_loose(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *type;
  grn_obj *value;
  grn_obj *default_value;
  grn_id type_id;
  grn_rc rc;
  grn_obj *casted_value = NULL;

  if (nargs != 3) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "cast_loose(): wrong number of arguments (%d for 3)",
                     nargs);
    return NULL;
  }

  type = args[0];
  if (!grn_obj_is_type(ctx, type)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, type);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "cast_loose(): the first argument must be type: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }
  type_id = grn_obj_id(ctx, type);

  value = args[1];
  default_value = args[2];

  casted_value = grn_plugin_proc_alloc(ctx, user_data, type_id, 0);
  if (!casted_value) {
    return NULL;
  }

  rc = grn_obj_cast(ctx, value, casted_value, GRN_FALSE);
  if (rc != GRN_SUCCESS) {
    rc = grn_obj_cast(ctx, default_value, casted_value, GRN_FALSE);
    if (rc != GRN_SUCCESS) {
      char type_name[GRN_TABLE_MAX_KEY_SIZE];
      int type_name_size;
      grn_obj inspected;
      type_name_size = grn_obj_name(ctx, type, type_name, sizeof(type_name));
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, default_value);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "cast_loose(): failed to set the default value: "
                       "<%.*s>: <%.*s>",
                       type_name_size,
                       type_name,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
  }

  return casted_value;
}

void
grn_proc_init_cast_loose(grn_ctx *ctx)
{
  grn_proc_create(ctx, "cast_loose", -1, GRN_PROC_FUNCTION,
                  func_cast_loose, NULL, NULL, 0, NULL);
}
