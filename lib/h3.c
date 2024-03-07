/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_WITH_H3
#  ifdef GRN_WITH_H3_BUNDLED
#    include <h3api.h>
#  else
#    include <h3/h3api.h>
#  endif
#endif

#ifdef GRN_WITH_H3
static const char *
grn_h3_error_to_string(H3Error error)
{
  switch (error) {
  case E_SUCCESS:
    return "success";
  case E_FAILED:
    return "failed";
  case E_DOMAIN:
    return "outside of acceptable range";
  case E_LATLNG_DOMAIN:
    return "latitude or longitude is outside of acceptable range";
  case E_RES_DOMAIN:
    return "resolution is outside of acceptable range";
  case E_CELL_INVALID:
    return "invalid cell";
  case E_DIR_EDGE_INVALID:
    return "invalid directed edge";
  case E_UNDIR_EDGE_INVALID:
    return "invalid undirected edge";
  case E_VERTEX_INVALID:
    return "invalid vertex";
  case E_PENTAGON:
    return "pentagon distortion is encountered";
  case E_DUPLICATE_INPUT:
    return "duplicate input is encountered";
  case E_NOT_NEIGHBORS:
    return "cells aren't neighbors";
  case E_RES_MISMATCH:
    return "cells have incompatible resolutions";
  case E_MEMORY_ALLOC:
    return "failed to allocate memory";
  case E_MEMORY_BOUNDS:
    return "bounds of provided memory are not large enough";
  case E_OPTION_INVALID:
    return "invalid option";
  default:
    return "unknown";
  }
}
#endif

uint64_t
grn_h3_compute_cell(grn_ctx *ctx,
                    grn_geo_point *geo_point,
                    int32_t resolution,
                    const char *tag)
{
#ifdef GRN_WITH_H3
  GRN_API_ENTER;

  if (!(0 <= resolution && resolution <= 15)) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s resolution must be in 0..15: %d",
        tag,
        resolution);
    GRN_API_RETURN(ctx->rc);
  }

  LatLng lat_lng = {
    .lat = GRN_GEO_MSEC2RADIAN(geo_point->latitude),
    .lng = GRN_GEO_MSEC2RADIAN(geo_point->longitude),
  };
  H3Index h3_index = 0;
  H3Error error = latLngToCell(&lat_lng, resolution, &h3_index);
  if (error != E_SUCCESS) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s failed to convert a point to H3 index: %s(%u): %fx%f",
        tag,
        grn_h3_error_to_string(error),
        error,
        GRN_GEO_MSEC2DEGREE(geo_point->latitude),
        GRN_GEO_MSEC2DEGREE(geo_point->longitude));
  }
  GRN_API_RETURN(h3_index);
#else
  GRN_API_ENTER;
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s H3 isn't enabled", tag);
  GRN_API_RETURN(0);
#endif
}

grn_rc
grn_h3_compute_grid_disk(grn_ctx *ctx,
                         grn_geo_point *geo_point,
                         int32_t resolution,
                         int32_t k,
                         grn_obj *h3_indices,
                         const char *tag)
{
#ifdef GRN_WITH_H3
  GRN_API_ENTER;

  if (!(0 <= resolution && resolution <= 15)) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s resolution must be in 0..15: %d",
        tag,
        resolution);
    GRN_API_RETURN(ctx->rc);
  }

  if (k < 0) {
    ERR(GRN_INVALID_ARGUMENT, "%s k must be 0 or larger: %d", tag, k);
    GRN_API_RETURN(ctx->rc);
  }

  int64_t n_h3_indices = 0;
  H3Error error = maxGridDiskSize(k, &n_h3_indices);
  if (error != E_SUCCESS) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s failed to compute the max number of indexes for grid disk: "
        "%s(%u): k:<%d>",
        tag,
        grn_h3_error_to_string(error),
        error,
        k);
  }

  H3Index h3_index = grn_h3_compute_cell(ctx, geo_point, resolution, tag);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_API_RETURN(ctx->rc);
  }

  size_t offset = GRN_BULK_VSIZE(h3_indices);
  int64_t i;
  for (i = 0; i < n_h3_indices; i++) {
    GRN_UINT64_PUT(ctx, h3_indices, 0);
  }
  error =
    gridDisk(h3_index, k, (H3Index *)(GRN_BULK_HEAD(h3_indices) + offset));
  if (error != E_SUCCESS) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s failed to compute grid disk: %s(%u): k:<%d>, geo_point:<%fx%f>(%d)",
        tag,
        grn_h3_error_to_string(error),
        error,
        k,
        GRN_GEO_MSEC2DEGREE(geo_point->latitude),
        GRN_GEO_MSEC2DEGREE(geo_point->longitude),
        resolution);
  }

  GRN_API_RETURN(ctx->rc);
#else
  GRN_API_ENTER;
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s H3 isn't enabled", tag);
  GRN_API_RETURN(0);
#endif
}
