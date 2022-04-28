/*
  Copyright (C) 2014  Brazil
  Copyright (C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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
#  define GRN_PLUGIN_FUNCTION_TAG token_filters_stem
#endif

#include <grn_str.h>

#include <groonga.h>
#include <groonga/token_filter.h>

#include <ctype.h>
#include <string.h>

#include <libstemmer.h>

typedef struct {
  grn_obj algorithm;
  bool keep_original;
} grn_stem_token_filter_options;

typedef struct {
  grn_stem_token_filter_options *options;
  grn_tokenize_mode mode;
  bool is_enabled;
  struct sb_stemmer *stemmer;
  grn_tokenizer_token token;
  grn_obj buffer;
} grn_stem_token_filter;

static void
stem_options_init(grn_ctx *ctx, grn_stem_token_filter_options *options)
{
  GRN_TEXT_INIT(&(options->algorithm), 0);
  GRN_TEXT_SETS(ctx, &(options->algorithm), "english");
  GRN_TEXT_PUTC(ctx, &(options->algorithm), '\0');
  options->keep_original = false;
}

static void *
stem_open_options(grn_ctx *ctx,
                  grn_obj *token_filter,
                  grn_obj *raw_options,
                  void *user_data)
{
  grn_stem_token_filter_options *options;

  options = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_stem_token_filter_options));
  if (!options) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][stem] "
                     "failed to allocate memory for options");
    return NULL;
  }

  stem_options_init(ctx, options);

  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "algorithm")) {
      const char *algorithm;
      unsigned int length;
      length = grn_vector_get_element(ctx,
                                      raw_options,
                                      i,
                                      &algorithm,
                                      NULL,
                                      NULL);
      GRN_TEXT_SET(ctx, &(options->algorithm), algorithm, length);
      GRN_TEXT_PUTC(ctx, &(options->algorithm), '\0');
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "keep_original")) {
      options->keep_original =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->keep_original);
    }
  } GRN_OPTION_VALUES_EACH_END();

  return options;
}

static void
stem_close_options(grn_ctx *ctx, void *data)
{
  grn_stem_token_filter_options *options = data;
  GRN_OBJ_FIN(ctx, &(options->algorithm));
  GRN_PLUGIN_FREE(ctx, options);
}

static void *
stem_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon;
  unsigned int i;
  grn_stem_token_filter_options *options;
  grn_stem_token_filter *token_filter;

  lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  i = grn_tokenizer_query_get_token_filter_index(ctx, query);
  options = grn_table_cache_token_filters_options(ctx,
                                                  lexicon,
                                                  i,
                                                  stem_open_options,
                                                  stem_close_options,
                                                  NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_stem_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][stem] "
                     "failed to allocate grn_stem_token_filter");
    return NULL;
  }
  token_filter->options = options;
  token_filter->mode = grn_tokenizer_query_get_mode(ctx, query);
  token_filter->is_enabled = true;
  grn_obj *query_options = grn_tokenizer_query_get_options(ctx, query);
  if (query_options) {
    grn_proc_prefixed_options_parse(ctx,
                                    query_options,
                                    "TokenFilterStem.",
                                    "[token-filter][stem]",
                                    "enable",
                                    GRN_PROC_OPTION_VALUE_BOOL,
                                    &(token_filter->is_enabled),
                                    NULL);
    if (ctx->rc != GRN_SUCCESS) {
      GRN_PLUGIN_FREE(ctx, token_filter);
      return NULL;
    }
  }

  {
    const char *algorithm = GRN_TEXT_VALUE(&(token_filter->options->algorithm));
    /* TODO: Support other encoding. */
    const char *encoding = "UTF_8";
    token_filter->stemmer = sb_stemmer_new(algorithm, encoding);
    if (!token_filter->stemmer) {
      GRN_PLUGIN_FREE(ctx, token_filter);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "[token-filter][stem] "
                       "failed to create stemmer: "
                       "algorithm=<%s>, encoding=<%s>",
                       algorithm, encoding);
      return NULL;
    }
  }
  grn_tokenizer_token_init(ctx, &(token_filter->token));
  GRN_TEXT_INIT(&(token_filter->buffer), 0);

  return token_filter;
}

static grn_bool
is_stemmable(grn_obj *data, grn_bool *is_all_upper)
{
  const char *current, *end;
  grn_bool have_lower = GRN_FALSE;
  grn_bool have_upper = GRN_FALSE;

  *is_all_upper = GRN_FALSE;

  switch (data->header.domain) {
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    break;
  default :
    return GRN_FALSE;
  }

  current = GRN_TEXT_VALUE(data);
  end = current + GRN_TEXT_LEN(data);

  for (; current < end; current++) {
    if (islower((unsigned char)*current)) {
      have_lower = GRN_TRUE;
      continue;
    }
    if (isupper((unsigned char)*current)) {
      have_upper = GRN_TRUE;
      continue;
    }
    if (isdigit((unsigned char)*current)) {
      continue;
    }
    switch (*current) {
    case '-' :
    case '\'' :
      break;
    default :
      return GRN_FALSE;
    }
  }

  if (!have_lower && have_upper) {
    *is_all_upper = GRN_TRUE;
  }

  return GRN_TRUE;
}

static void
normalize(grn_ctx *ctx,
          const char *string, unsigned int length,
          grn_obj *normalized)
{
  const char *current, *end;
  const char *unwritten;

  current = unwritten = string;
  end = current + length;

  for (; current < end; current++) {
    if (isupper((unsigned char)*current)) {
      if (current > unwritten) {
        GRN_TEXT_PUT(ctx, normalized, unwritten, current - unwritten);
      }
      GRN_TEXT_PUTC(ctx, normalized, (char)tolower((unsigned char)*current));
      unwritten = current + 1;
    }
  }

  if (current != unwritten) {
    GRN_TEXT_PUT(ctx, normalized, unwritten, current - unwritten);
  }
}

static void
unnormalize(grn_ctx *ctx,
            const char *string, unsigned int length,
            grn_obj *normalized)
{
  const char *current, *end;
  const char *unwritten;

  current = unwritten = string;
  end = current + length;

  for (; current < end; current++) {
    if (islower((unsigned char)*current)) {
      if (current > unwritten) {
        GRN_TEXT_PUT(ctx, normalized, unwritten, current - unwritten);
      }
      GRN_TEXT_PUTC(ctx, normalized, (char)toupper((unsigned char)*current));
      unwritten = current + 1;
    }
  }

  if (current != unwritten) {
    GRN_TEXT_PUT(ctx, normalized, unwritten, current - unwritten);
  }
}

static void
stem_filter(grn_ctx *ctx,
            grn_token *current_token,
            grn_token *next_token,
            void *user_data)
{
  grn_stem_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_bool is_all_upper = GRN_FALSE;

  if (!token_filter->is_enabled) {
    return;
  }

  if (GRN_CTX_GET_ENCODING(ctx) != GRN_ENC_UTF8) {
    return;
  }

  data = grn_token_get_data(ctx, current_token);
  if (!is_stemmable(data, &is_all_upper)) {
    return;
  }

  {
    const sb_symbol *stemmed;

    if (is_all_upper) {
      grn_obj *buffer;
      buffer = &(token_filter->buffer);
      GRN_BULK_REWIND(buffer);
      normalize(ctx,
                GRN_TEXT_VALUE(data),
                (unsigned int)GRN_TEXT_LEN(data),
                buffer);
      stemmed = sb_stemmer_stem(token_filter->stemmer,
                                GRN_TEXT_VALUE(buffer),
                                (int)GRN_TEXT_LEN(buffer));
      if (!stemmed) {
        GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                         "[token-filter][stem] "
                         "failed to allocate memory for stemmed word: <%.*s> "
                         "(normalized: <%.*s>)",
                         (int)GRN_TEXT_LEN(data), GRN_TEXT_VALUE(data),
                         (int)GRN_TEXT_LEN(buffer), GRN_TEXT_VALUE(buffer));
        return;
      }
      GRN_BULK_REWIND(buffer);
      unnormalize(ctx,
                  stemmed,
                  (unsigned int)sb_stemmer_length(token_filter->stemmer),
                  buffer);
      grn_token_set_data(ctx,
                         next_token,
                         GRN_TEXT_VALUE(buffer),
                         (int)GRN_TEXT_LEN(buffer));
    } else {
      stemmed = sb_stemmer_stem(token_filter->stemmer,
                                GRN_TEXT_VALUE(data),
                                (int)GRN_TEXT_LEN(data));
      if (!stemmed) {
        GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                         "[token-filter][stem] "
                         "failed to allocate memory for stemmed word: <%.*s>",
                         (int)GRN_TEXT_LEN(data), GRN_TEXT_VALUE(data));
        return;
      }
      grn_token_set_data(ctx, next_token,
                         stemmed,
                         sb_stemmer_length(token_filter->stemmer));
    }
  }

  if (token_filter->mode == GRN_TOKENIZE_ADD &&
      token_filter->options->keep_original) {
    grn_token_add_status(ctx, next_token, GRN_TOKEN_KEEP_ORIGINAL);
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
  GRN_OBJ_FIN(ctx, &(token_filter->buffer));
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
  grn_obj *token_filter;

  token_filter = grn_token_filter_create(ctx, "TokenFilterStem", -1);
  grn_token_filter_set_init_func(ctx, token_filter, stem_init);
  grn_token_filter_set_filter_func(ctx, token_filter, stem_filter);
  grn_token_filter_set_fin_func(ctx, token_filter, stem_fin);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
