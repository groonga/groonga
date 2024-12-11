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
#  define GRN_PLUGIN_FUNCTION_TAG tokenizers_h3_index
#endif

#include <groonga.h>
#include <groonga/tokenizer.h>

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
  uint64_t index;
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

static void
grn_h3_index_next(grn_ctx *ctx,
                  grn_tokenizer_query *query,
                  grn_token *token,
                  void *user_data)
{
  grn_h3_index_tokenizer *tokenizer = user_data;

  tokenizer->index = grn_h3_compute_cell(ctx,
                                         &(tokenizer->geo_point),
                                         tokenizer->options.resolution,
                                         grn_h3_index_tag);
  if (ctx->rc != GRN_SUCCESS) {
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
