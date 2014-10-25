/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014 Brazil

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

#include <str.h>

#include <groonga.h>
#include <groonga/token_filter.h>

#include <string.h>

#include <libstemmer.h>

typedef struct {
  struct sb_stemmer *stemmer;
  grn_tokenizer_token token;
} grn_stem_token_filter;

static void *
stem_init(grn_ctx *ctx, grn_obj *table, grn_token_mode mode)
{
  grn_stem_token_filter *token_filter;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_stem_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][stem] "
                     "failed to allocate grn_stem_token_filter");
    return NULL;
  }

  token_filter->stemmer = NULL;
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
stem_filter(grn_ctx *ctx,
            grn_token *current_token,
            grn_token *next_token,
            void *user_data)
{
  grn_stem_token_filter *token_filter = user_data;
  grn_obj *data;

  if (GRN_CTX_GET_ENCODING(ctx) != GRN_ENC_UTF8) {
    return;
  }

  data = grn_token_get_data(ctx, current_token);

  if (token_filter->stemmer) {
    sb_stemmer_delete(token_filter->stemmer);
  }
  {
    /* TODO: Detect algorithm from the current token. */
    const char *algorithm = "english";
    const char *encoding = "UTF_8";
    token_filter->stemmer = sb_stemmer_new(algorithm, encoding);
    if (!token_filter->stemmer) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "[token-filter][stem] "
                       "failed to create stemmer: "
                       "algorithm=<%s>, encoding=<%s>",
                       algorithm, encoding);
      return;
    }
  }

  {
    const sb_symbol *stemmed;

    stemmed = sb_stemmer_stem(token_filter->stemmer,
                              GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (stemmed) {
      grn_token_set_data(ctx, next_token,
                         stemmed,
                         sb_stemmer_length(token_filter->stemmer));
    } else {
      GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                       "[token-filter][stem] "
                       "failed to allocate memory for stemmed word: <%.*s>",
                       (int)GRN_TEXT_LEN(data), GRN_TEXT_VALUE(data));
      return;
    }
  }
}

static void
stem_fin(grn_ctx *ctx, void *user_data)
{
  grn_stem_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }

  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  if (token_filter->stemmer) {
    sb_stemmer_delete(token_filter->stemmer);
  }
  GRN_PLUGIN_FREE(ctx, token_filter);
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc;

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterStem", -1,
                                 stem_init,
                                 stem_filter,
                                 stem_fin);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
