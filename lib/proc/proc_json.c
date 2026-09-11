/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

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
#include "../grn_expr.h"
#include "../grn_store.h"

#include <groonga/plugin.h>

typedef struct {
  grn_obj *json;
  grn_obj json_buffer;
  grn_json_path *json_path;
} grn_json_extract_data;

static void
grn_json_extract_data_free(grn_ctx *ctx, grn_json_extract_data *data)
{
  if (!data) {
    return;
  }

  GRN_OBJ_FIN(ctx, &(data->json_buffer));
  if (data->json_path) {
    grn_json_path_close(ctx, data->json_path);
  }

  GRN_PLUGIN_FREE(ctx, data);
}

static grn_obj *
func_json_extract_init(grn_ctx *ctx,
                       int n_args,
                       grn_obj **args,
                       grn_user_data *user_data)
{
  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "json_extract(): wrong number of arguments (%d for 2)",
                     n_args);
    return NULL;
  }

  grn_json_extract_data *data =
    GRN_PLUGIN_CALLOC(ctx, sizeof(grn_json_extract_data));
  if (!data) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "json_extract(): failed to allocate internal data");
    return NULL;
  }
  user_data->ptr = data;

  GRN_JSON_INIT(&(data->json_buffer), 0);
  grn_obj *json = args[0];
  if (json->header.domain == GRN_DB_JSON) {
    data->json = json;
  } else if (grn_obj_is_text_family_bulk(ctx, json)) {
    grn_rc rc = grn_obj_cast(ctx, json, &(data->json_buffer), false);
    if (rc != GRN_SUCCESS) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "json_extract(): failed to parse JSON: <%.*s>",
                       (int)GRN_TEXT_LEN(json),
                       GRN_TEXT_VALUE(json));
      return NULL;
    }
    data->json = &(data->json_buffer);
  } else {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, json);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "json_extract(): the first argument must be "
                     "a parsed JSON or JSON string: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  grn_obj *json_path = args[1];
  if (!grn_obj_is_text_family_bulk(ctx, json_path)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, json_path);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "json_extract(): the second argument must be "
                     "a JSONPath string: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }
  data->json_path =
    grn_json_path_open(ctx, GRN_TEXT_VALUE(json_path), GRN_TEXT_LEN(json_path));
  if (!data->json_path) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "json_extract(): failed to parse JSONPath: <%.*s>",
                     (int)GRN_TEXT_LEN(json_path),
                     GRN_TEXT_VALUE(json_path));
    return NULL;
  }

  return NULL;
}

static grn_obj *
func_json_extract_next(grn_ctx *ctx,
                       int n_args,
                       grn_obj **args,
                       grn_user_data *user_data)
{
  grn_json_extract_data *data = user_data->ptr;

  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  grn_obj *values =
    grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, GRN_OBJ_VECTOR);
  if (!values) {
    return NULL;
  }
  grn_rc rc = grn_json_extract(ctx, data->json, data->json_path, values);
  if (rc != GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "json_extract(): "
                     "failed to extract values: %s",
                     ctx->errbuf);
    return NULL;
  }

  return values;
}

static grn_obj *
func_json_extract_fin(grn_ctx *ctx,
                      int n_args,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  grn_json_extract_data *data = user_data->ptr;

  grn_json_extract_data_free(ctx, data);

  return NULL;
}

void
grn_proc_init_json_extract(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "json_extract",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_json_extract_init,
                  func_json_extract_next,
                  func_json_extract_fin,
                  0,
                  NULL);
}
