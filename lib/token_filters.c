/*
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_token_filters.h"
#include "grn_normalizer.h"

#include <groonga/token_filter.h>

typedef struct {
  grn_nfkc_normalize_options *options;
  grn_string string;
  grn_tokenizer_token token;
} grn_nfkc_token_filter;

static void *
nfkc_open_options(grn_ctx *ctx,
                  grn_obj *token_filter,
                  grn_obj *raw_options,
                  void *user_data,
                  grn_nfkc_normalize_options_init_func init_func,
                  const char *tag)
{
  grn_nfkc_normalize_options *options;

  options = GRN_CALLOC(sizeof(grn_nfkc_normalize_options));
  if (!options) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter]%s "
                     "failed to allocate memory for options",
                     tag);
    return NULL;
  }

  init_func(ctx, options);

  grn_nfkc_normalize_options_apply(ctx, options, raw_options);

  return options;
}


static void
nfkc_close_options(grn_ctx *ctx, void *data)
{
  grn_nfkc_normalize_options *options = data;
  grn_nfkc_normalize_options_fin(ctx, options);
  GRN_PLUGIN_FREE(ctx, options);
}

static void *
nfkc_init(grn_ctx *ctx,
          grn_tokenizer_query *query,
          grn_table_module_open_options_func open_options_func,
          const char *normalizer_name,
          const char *tag)
{
  grn_obj *lexicon;
  unsigned int i;
  grn_nfkc_normalize_options *options;
  grn_nfkc_token_filter *token_filter;

  lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  i = grn_tokenizer_query_get_token_filter_index(ctx, query);
  options = grn_table_cache_token_filters_options(ctx,
                                                  lexicon,
                                                  i,
                                                  open_options_func,
                                                  nfkc_close_options,
                                                  NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_nfkc_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter]%s "
                     "failed to allocate grn_nfkc_token_filter",
                     tag);
    return NULL;
  }

  token_filter->options = options;
  grn_string_init(ctx,
                  (grn_obj *)(&(token_filter->string)),
                  grn_ctx_get(ctx, normalizer_name, -1),
                  0,
                  GRN_CTX_GET_ENCODING(ctx));
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
nfkc_filter(grn_ctx *ctx,
            grn_token *current_token,
            grn_token *next_token,
            void *user_data)
{
  grn_nfkc_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_obj *string;

  if (!token_filter) {
    return;
  }

  if (GRN_CTX_GET_ENCODING(ctx) != GRN_ENC_UTF8) {
    return;
  }

  data = grn_token_get_data(ctx, current_token);
  string = (grn_obj *)(&(token_filter->string));
  grn_string_set_original(ctx,
                          string,
                          GRN_TEXT_VALUE(data),
                          (unsigned int)GRN_TEXT_LEN(data));
  grn_nfkc_normalize(ctx,
                     string,
                     token_filter->options);
  {
    const char *normalized;
    unsigned int normalized_length;
    grn_string_get_normalized(ctx,
                              string,
                              &normalized,
                              &normalized_length,
                              NULL);
    grn_token_set_data(ctx, next_token, normalized, (int)normalized_length);
  }
}

static void
nfkc_fin(grn_ctx *ctx, void *user_data)
{
  grn_nfkc_token_filter *token_filter = user_data;

  if (!token_filter) {
    return;
  }

  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  grn_string_fin(ctx, (grn_obj *)(&(token_filter->string)));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

static void *
nfkc100_open_options(grn_ctx *ctx,
                     grn_obj *token_filter,
                     grn_obj *raw_options,
                     void *user_data)
{
  return nfkc_open_options(ctx,
                           token_filter,
                           raw_options,
                           user_data,
                           grn_nfkc100_normalize_options_init,
                           "[nfkc100]");
}

static void *
nfkc100_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  return nfkc_init(ctx,
                   query,
                   nfkc100_open_options,
                   "NormalierNFKC100",
                   "[nfkc100]");
}

static void *
nfkc121_open_options(grn_ctx *ctx,
                     grn_obj *token_filter,
                     grn_obj *raw_options,
                     void *user_data)
{
  return nfkc_open_options(ctx,
                           token_filter,
                           raw_options,
                           user_data,
                           grn_nfkc121_normalize_options_init,
                           "[nfkc121]");
}

static void *
nfkc121_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  return nfkc_init(ctx,
                   query,
                   nfkc121_open_options,
                   "NormalierNFKC121",
                   "[nfkc121]");
}

static void *
nfkc130_open_options(grn_ctx *ctx,
                     grn_obj *token_filter,
                     grn_obj *raw_options,
                     void *user_data)
{
  return nfkc_open_options(ctx,
                           token_filter,
                           raw_options,
                           user_data,
                           grn_nfkc130_normalize_options_init,
                           "[nfkc130]");
}

static void *
nfkc130_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  return nfkc_init(ctx,
                   query,
                   nfkc130_open_options,
                   "NormalierNFKC130",
                   "[nfkc130]");
}

static void *
nfkc150_open_options(grn_ctx *ctx,
                     grn_obj *token_filter,
                     grn_obj *raw_options,
                     void *user_data)
{
  return nfkc_open_options(ctx,
                           token_filter,
                           raw_options,
                           user_data,
                           grn_nfkc150_normalize_options_init,
                           "[nfkc150]");
}

static void *
nfkc150_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  return nfkc_init(ctx,
                   query,
                   nfkc150_open_options,
                   "NormalierNFKC150",
                   "[nfkc150]");
}

grn_rc
grn_db_init_builtin_token_filters(grn_ctx *ctx)
{
  {
    grn_obj *token_filter;
    token_filter = grn_token_filter_create(ctx, "TokenFilterNFKC100", -1);
    grn_token_filter_set_init_func(ctx, token_filter, nfkc100_init);
    grn_token_filter_set_filter_func(ctx, token_filter, nfkc_filter);
    grn_token_filter_set_fin_func(ctx, token_filter, nfkc_fin);
  }

  {
    grn_obj *token_filter;
    token_filter = grn_token_filter_create(ctx, "TokenFilterNFKC121", -1);
    grn_token_filter_set_init_func(ctx, token_filter, nfkc121_init);
    grn_token_filter_set_filter_func(ctx, token_filter, nfkc_filter);
    grn_token_filter_set_fin_func(ctx, token_filter, nfkc_fin);
  }

  {
    grn_obj *token_filter;
    token_filter = grn_token_filter_create(ctx, "TokenFilterNFKC130", -1);
    grn_token_filter_set_init_func(ctx, token_filter, nfkc130_init);
    grn_token_filter_set_filter_func(ctx, token_filter, nfkc_filter);
    grn_token_filter_set_fin_func(ctx, token_filter, nfkc_fin);
  }

  {
    grn_obj *token_filter;
    token_filter = grn_token_filter_create(ctx, "TokenFilterNFKC150", -1);
    grn_token_filter_set_init_func(ctx, token_filter, nfkc150_init);
    grn_token_filter_set_filter_func(ctx, token_filter, nfkc_filter);
    grn_token_filter_set_fin_func(ctx, token_filter, nfkc_fin);
  }

  return GRN_SUCCESS;
}
