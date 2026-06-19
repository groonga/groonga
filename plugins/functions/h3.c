/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_h3
#endif

#include <groonga/plugin.h>

static grn_obj *
grn_h3_ensure_wgs84_geo_point(grn_ctx *ctx,
                              grn_obj *input,
                              grn_obj *buffer,
                              const char *tag,
                              const char *input_name)
{
  if (grn_obj_is_bulk(ctx, input) &&
      input->header.domain == GRN_DB_WGS84_GEO_POINT) {
    return input;
  }
  if (grn_obj_is_text_family_bulk(ctx, input)) {
    GRN_BULK_REWIND(buffer);
    grn_rc rc = grn_obj_cast(ctx, input, buffer, false);
    if (rc == GRN_SUCCESS) {
      return buffer;
    }
  }

  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, input);
  GRN_PLUGIN_ERROR(ctx,
                   GRN_INVALID_ARGUMENT,
                   "%s %s must be WGS84GeoPoint: %.*s",
                   tag,
                   input_name,
                   (int)GRN_TEXT_LEN(&inspected),
                   GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
  return NULL;
}

static grn_obj *
func_h3_grid_distance(grn_ctx *ctx,
                      int n_args,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  const char *tag = "h3_grid_distance():";

  if (n_args != 3) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 3)",
                     tag,
                     n_args);
    return NULL;
  }

  grn_obj *distance_obj = NULL;

  grn_obj *point1_obj = args[0];
  grn_obj *point2_obj = args[1];
  grn_obj *resolution_obj = args[2];

  grn_obj point1_buffer;
  GRN_WGS84_GEO_POINT_INIT(&point1_buffer, 0);
  grn_obj point2_buffer;
  GRN_WGS84_GEO_POINT_INIT(&point2_buffer, 0);

  int32_t resolution =
    grn_plugin_proc_get_value_int32(ctx, resolution_obj, -1, tag);

  point1_obj = grn_h3_ensure_wgs84_geo_point(ctx,
                                             point1_obj,
                                             &point1_buffer,
                                             tag,
                                             "point1");
  if (!point1_obj) {
    goto exit;
  }
  point2_obj = grn_h3_ensure_wgs84_geo_point(ctx,
                                             point2_obj,
                                             &point2_buffer,
                                             tag,
                                             "point2");
  if (!point2_obj) {
    goto exit;
  }

  uint64_t h3_index1 = grn_h3_compute_cell(ctx,
                                           GRN_GEO_POINT_VALUE_RAW(point1_obj),
                                           resolution,
                                           tag);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  uint64_t h3_index2 = grn_h3_compute_cell(ctx,
                                           GRN_GEO_POINT_VALUE_RAW(point2_obj),
                                           resolution,
                                           tag);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  uint64_t distance =
    grn_h3_compute_grid_distance(ctx, h3_index1, h3_index2, tag);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  distance_obj = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_UINT64, 0);
  if (!distance_obj) {
    return NULL;
  }
  GRN_UINT64_SET(ctx, distance_obj, distance);

exit:
  GRN_OBJ_FIN(ctx, &point1_buffer);
  GRN_OBJ_FIN(ctx, &point2_buffer);

  return distance_obj;
}

static grn_rc
applier_h3_grid_distance(grn_ctx *ctx, grn_applier_data *data)
{
  const char *tag = "h3_grid_distance():";

  grn_obj *table = grn_applier_data_get_table(ctx, data);
  grn_obj *output_column = grn_applier_data_get_output_column(ctx, data);
  size_t n_args;
  grn_obj **args = grn_applier_data_get_args(ctx, data, &n_args);

  if (n_args != 3) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%zu for 3)",
                     tag,
                     n_args);
    return ctx->rc;
  }

  grn_obj *input_column = args[0];
  grn_obj *point2_obj = args[1];
  grn_obj *resolution_obj = args[2];

  int32_t resolution =
    grn_plugin_proc_get_value_int32(ctx, resolution_obj, -1, tag);

  if (!(grn_obj_is_scalar_column(ctx, input_column) ||
        grn_obj_is_scalar_accessor(ctx, input_column))) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, input_column);
    GRN_PLUGIN_ERROR(
      ctx,
      GRN_INVALID_ARGUMENT,
      "%s 1st argument must be a scalar column or accessor: %.*s",
      tag,
      (int)(GRN_TEXT_LEN(&inspected)),
      GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return ctx->rc;
  }
  grn_id input_column_range = grn_obj_get_range(ctx, input_column);
  if (input_column_range != GRN_DB_WGS84_GEO_POINT) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s 1st argument's type must be WGS84GeoPoint: %s",
                     tag,
                     grn_type_id_to_string_builtin(ctx, input_column_range));
    return ctx->rc;
  }

  grn_obj point2_buffer;
  GRN_WGS84_GEO_POINT_INIT(&point2_buffer, 0);
  point2_obj = grn_h3_ensure_wgs84_geo_point(ctx,
                                             point2_obj,
                                             &point2_buffer,
                                             tag,
                                             "point2");
  uint64_t h3_index2 = grn_h3_compute_cell(ctx,
                                           GRN_GEO_POINT_VALUE_RAW(point2_obj),
                                           resolution,
                                           tag);
  GRN_OBJ_FIN(ctx, &point2_buffer);
  if (ctx->rc != GRN_SUCCESS) {
    return ctx->rc;
  }

  if (grn_table_size(ctx, table) == 0) {
    return ctx->rc;
  }

  grn_obj point1_buffer;
  GRN_WGS84_GEO_POINT_INIT(&point1_buffer, 0);
  grn_obj distance_buffer;
  GRN_UINT64_INIT(&distance_buffer, 0);
  GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id)
  {
    GRN_BULK_REWIND(&point1_buffer);
    grn_obj_get_value(ctx, input_column, id, &point1_buffer);
    if (GRN_BULK_VSIZE(&point1_buffer) != sizeof(grn_geo_point)) {
      continue;
    }
    uint64_t h3_index1 =
      grn_h3_compute_cell(ctx,
                          GRN_GEO_POINT_VALUE_RAW(&point1_buffer),
                          resolution,
                          tag);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    uint64_t distance_raw =
      grn_h3_compute_grid_distance(ctx, h3_index1, h3_index2, tag);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    GRN_UINT64_SET(ctx, &distance_buffer, distance_raw);
    grn_obj_set_value(ctx, output_column, id, &distance_buffer, GRN_OBJ_SET);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &point1_buffer);
  GRN_OBJ_FIN(ctx, &distance_buffer);

  return ctx->rc;
}

static grn_obj *
func_h3_in_grid_disk(grn_ctx *ctx,
                     int n_args,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  const char *tag = "h3_in_grid_disk():";
  if (n_args != 4) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 4)",
                     tag,
                     n_args);
    return NULL;
  }

  grn_obj *in_obj = NULL;

  grn_obj *target_obj = args[0];
  grn_obj *origin_obj = args[1];
  grn_obj *resolution_obj = args[2];
  grn_obj *k_obj = args[3];

  grn_obj target_buffer;
  GRN_WGS84_GEO_POINT_INIT(&target_buffer, 0);
  grn_obj origin_buffer;
  GRN_WGS84_GEO_POINT_INIT(&origin_buffer, 0);
  grn_obj h3_indices;
  GRN_UINT64_INIT(&h3_indices, GRN_OBJ_VECTOR);

  int32_t resolution =
    grn_plugin_proc_get_value_int32(ctx, resolution_obj, -1, tag);
  int32_t k = grn_plugin_proc_get_value_int32(ctx, k_obj, -1, tag);

  target_obj = grn_h3_ensure_wgs84_geo_point(ctx,
                                             target_obj,
                                             &target_buffer,
                                             tag,
                                             "target");
  if (!target_obj) {
    goto exit;
  }
  uint64_t target_index =
    grn_h3_compute_cell(ctx,
                        GRN_GEO_POINT_VALUE_RAW(target_obj),
                        resolution,
                        tag);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  origin_obj = grn_h3_ensure_wgs84_geo_point(ctx,
                                             origin_obj,
                                             &origin_buffer,
                                             tag,
                                             "origin");
  if (!origin_obj) {
    goto exit;
  }

  if (grn_h3_compute_grid_disk(ctx,
                               GRN_GEO_POINT_VALUE_RAW(origin_obj),
                               resolution,
                               k,
                               &h3_indices,
                               tag) != GRN_SUCCESS) {
    goto exit;
  }
  bool in = false;
  size_t i;
  size_t n_h3_indices = GRN_UINT64_VECTOR_SIZE(&h3_indices);
  for (i = 0; i < n_h3_indices; i++) {
    uint64_t h3_index = GRN_UINT64_VALUE_AT(&h3_indices, i);
    if (h3_index == target_index) {
      in = true;
      break;
    }
  }

  in_obj = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_BOOL, 0);
  if (!in_obj) {
    return NULL;
  }
  GRN_BOOL_SET(ctx, in_obj, in);

exit:
  GRN_OBJ_FIN(ctx, &target_buffer);
  GRN_OBJ_FIN(ctx, &origin_buffer);
  GRN_OBJ_FIN(ctx, &h3_indices);

  return in_obj;
}

static grn_rc
selector_h3_in_grid_disk(grn_ctx *ctx,
                         grn_obj *table,
                         grn_obj *index,
                         int n_args,
                         grn_obj **args,
                         grn_obj *res,
                         grn_operator op)
{
  const char *tag = "h3_in_grid_disk():";

  if ((n_args - 1) != 4) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 4)",
                     tag,
                     (n_args - 1));
    return ctx->rc;
  }

  grn_obj *origin_obj = args[2];
  grn_obj *resolution_obj = args[3];
  grn_obj *k_obj = args[4];

  grn_obj origin_buffer;
  GRN_WGS84_GEO_POINT_INIT(&origin_buffer, 0);

  grn_obj h3_indices;
  GRN_UINT64_INIT(&h3_indices, GRN_OBJ_VECTOR);

  int32_t resolution =
    grn_plugin_proc_get_value_int32(ctx, resolution_obj, -1, tag);
  int32_t k = grn_plugin_proc_get_value_int32(ctx, k_obj, -1, tag);

  origin_obj = grn_h3_ensure_wgs84_geo_point(ctx,
                                             origin_obj,
                                             &origin_buffer,
                                             tag,
                                             "origin");
  if (!origin_obj) {
    goto exit;
  }
  if (grn_h3_compute_grid_disk(ctx,
                               GRN_GEO_POINT_VALUE_RAW(origin_obj),
                               resolution,
                               k,
                               &h3_indices,
                               tag) != GRN_SUCCESS) {
    goto exit;
  }
  grn_ii *ii = (grn_ii *)index;
  grn_obj *lexicon = grn_ii_get_lexicon(ctx, ii);
  size_t i;
  size_t n_h3_indices = GRN_UINT64_VECTOR_SIZE(&h3_indices);
  for (i = 0; i < n_h3_indices; i++) {
    uint64_t h3_index = GRN_UINT64_VALUE_AT(&h3_indices, i);
    if (h3_index == 0) {
      continue;
    }
    grn_id id = grn_table_get(ctx, lexicon, &h3_index, sizeof(uint64_t));
    if (id == GRN_ID_NIL) {
      continue;
    }
    grn_ii_select_by_id(ctx, ii, id, (grn_hash *)res, op);
  }
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);

exit:
  GRN_OBJ_FIN(ctx, &origin_buffer);
  GRN_OBJ_FIN(ctx, &h3_indices);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  {
    grn_obj *proc = grn_proc_create(ctx,
                                    "h3_grid_distance",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_h3_grid_distance,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_applier(ctx, proc, applier_h3_grid_distance);
  }

  {
    grn_obj *proc = grn_proc_create(ctx,
                                    "h3_in_grid_disk",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_h3_in_grid_disk,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, proc, selector_h3_in_grid_disk);
    grn_proc_set_selector_operator(ctx, proc, GRN_OP_MATCH);
  }

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
