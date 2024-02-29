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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG tokenizers_h3_index
#endif

#ifdef WIN32
#  define _USE_MATH_DEFINES
/* We want use M_PI macro in math.h.
   But we can't use M_PI in VC++ just because we include math.h.
   We need to define _USE_MATH_DEFINES before including math.h in order to use
   M_PI.

   See: https://learn.microsoft.com/en-us/cpp/c-runtime-library/math-constants

   math.h is included in groonga.h.
   So, we define _USE_MATH_DEFINES before including groonga.h
*/
#endif /* WIN32 */

#include <groonga.h>
#include <groonga/tokenizer.h>

#include <config.h>

#ifdef GRN_WITH_H3_BUNDLED
#  include <h3api.h>
#else
#  include <h3/h3api.h>
#endif

static const char *grn_h3_index_tag = "[tokenizer][h3-index]";

typedef struct {
  uint8_t resolution;
} grn_h3_index_options;

static void
grn_h3_index_options_init(grn_ctx *ctx,
                          grn_h3_index_options *options,
                          int32_t resolution)
{
  options->resolution = resolution;
}

static void
grn_h3_index_options_fin(grn_ctx *ctx, grn_h3_index_options *options)
{
}

typedef struct {
  grn_h3_index_options options;
  grn_geo_point geo_point;
  H3Index index;
} grn_h3_index_tokenizer;

static void
grn_h3_index_close_options(grn_ctx *ctx, void *data)
{
  grn_h3_index_options *options = data;
  grn_h3_index_options_fin(ctx, options);
  GRN_PLUGIN_FREE(ctx, options);
}

static void *
grn_h3_index_open_options(grn_ctx *ctx,
                          grn_obj *tokenizer,
                          grn_obj *raw_options,
                          void *user_data)
{
  grn_h3_index_options *options =
    GRN_PLUGIN_CALLOC(ctx, sizeof(grn_h3_index_options));
  if (!options) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_NO_MEMORY_AVAILABLE,
                     "%s failed to allocate memory for options",
                     grn_h3_index_tag);
    return NULL;
  }

  grn_h3_index_options_init(ctx, options, 15);
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length)
  {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "resolution")) {
      options->resolution =
        grn_vector_get_element_uint8(ctx, raw_options, i, options->resolution);
    }
  }
  GRN_OPTION_VALUES_EACH_END();

  return options;
}

static void
grn_h3_index_fin(grn_ctx *ctx, void *user_data)
{
  grn_h3_index_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }

  GRN_PLUGIN_FREE(ctx, tokenizer);
}

static void *
grn_h3_index_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_geo_point geo_point;
  size_t query_size;
  grn_id query_domain;
  const char *raw_query =
    grn_tokenizer_query_get_data(ctx, query, &query_size, &query_domain);
  if (query_domain == GRN_DB_WGS84_GEO_POINT) {
    if (query_size != sizeof(grn_geo_point)) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s query must be a WGS84GeoPoint: %zu != %zu",
                       grn_h3_index_tag,
                       query_size,
                       sizeof(grn_geo_point));
      return NULL;
    }
    geo_point = *((const grn_geo_point *)raw_query);
  } else {
    if (grn_type_id_is_text_family(ctx, query_domain)) {
      grn_obj text_geo_point;
      GRN_TEXT_INIT(&text_geo_point, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_SET(ctx, &text_geo_point, raw_query, query_size);
      grn_obj geo_point_buffer;
      GRN_WGS84_GEO_POINT_INIT(&geo_point_buffer, 0);
      grn_rc rc = grn_obj_cast(ctx, &text_geo_point, &geo_point_buffer, false);
      if (rc != GRN_SUCCESS) {
        GRN_OBJ_FIN(ctx, &geo_point_buffer);
        GRN_PLUGIN_ERROR(
          ctx,
          rc,
          "%s query must be valid WGS84GeoPoint text representation: <%.*s>",
          grn_h3_index_tag,
          (int)query_size,
          raw_query);
        return NULL;
      }
      GRN_GEO_POINT_VALUE(&geo_point_buffer,
                          geo_point.latitude,
                          geo_point.longitude);
      GRN_OBJ_FIN(ctx, &geo_point_buffer);
    } else {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s query must be WGS84GeoPoint: %s(%u)",
                       grn_h3_index_tag,
                       grn_type_id_to_string_builtin(ctx, query_domain),
                       query_domain);
      return NULL;
    }
  }

  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_h3_index_options *options =
    grn_table_cache_default_tokenizer_options(ctx,
                                              lexicon,
                                              grn_h3_index_open_options,
                                              grn_h3_index_close_options,
                                              query);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  grn_h3_index_tokenizer *tokenizer =
    GRN_PLUGIN_CALLOC(ctx, sizeof(grn_h3_index_tokenizer));
  if (!tokenizer) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_NO_MEMORY_AVAILABLE,
                     "[tokenizer][h3] failed to allocate tokenizer");
    return NULL;
  }

  tokenizer->options = *options;
  tokenizer->geo_point = geo_point;

  return tokenizer;
}

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

static void
grn_h3_index_next(grn_ctx *ctx,
                  grn_tokenizer_query *query,
                  grn_token *token,
                  void *user_data)
{
  grn_h3_index_tokenizer *tokenizer = user_data;

  LatLng lat_lng = {
    .lat = GRN_GEO_MSEC2RADIAN(tokenizer->geo_point.latitude),
    .lng = GRN_GEO_MSEC2RADIAN(tokenizer->geo_point.longitude),
  };
  H3Error error =
    latLngToCell(&lat_lng, tokenizer->options.resolution, &(tokenizer->index));
  if (error != E_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s failed to convert point to H3 index: %u: %s",
                     grn_h3_index_tag,
                     error,
                     grn_h3_error_to_string(error));
    return;
  }
  grn_token_set_data(ctx,
                     token,
                     (const char *)&(tokenizer->index),
                     sizeof(uint64_t));
  grn_token_set_domain(ctx, token, GRN_DB_UINT64);
  grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_obj *tokenizer = grn_tokenizer_create(ctx, "TokenH3Index", -1);
  if (tokenizer) {
    grn_tokenizer_set_init_func(ctx, tokenizer, grn_h3_index_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, grn_h3_index_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, grn_h3_index_fin);
  }
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
