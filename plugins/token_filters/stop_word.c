/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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
#  define GRN_PLUGIN_FUNCTION_TAG token_filters_stop_word
#endif

#include <grn_str.h>

#include <groonga.h>
#include <groonga/token_filter.h>

#include <string.h>

typedef struct {
  grn_obj column;
} grn_stop_word_token_filter_options;

typedef struct {
  grn_stop_word_token_filter_options *options;
  grn_obj *lexicon;
  grn_obj *column;
  grn_obj value;
  bool is_enabled;
  grn_tokenizer_token token;
} grn_stop_word_token_filter;

static void
stop_word_options_init(grn_ctx *ctx,
                       grn_stop_word_token_filter_options *options)
{
  GRN_TEXT_INIT(&(options->column), 0);
  GRN_TEXT_SETS(ctx, &(options->column), "is_stop_word");
}

static void *
stop_word_open_options(grn_ctx *ctx,
                       grn_obj *token_filter,
                       grn_obj *raw_options,
                       void *user_data)
{
  grn_stop_word_token_filter_options *options;

  options = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_stop_word_token_filter_options));
  if (!options) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][stop-word] "
                     "failed to allocate memory for options");
    return NULL;
  }

  stop_word_options_init(ctx, options);

  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "column")) {
      const char *column;
      unsigned int length;
      length = grn_vector_get_element(ctx,
                                      raw_options,
                                      i,
                                      &column,
                                      NULL,
                                      NULL);
      GRN_TEXT_SET(ctx, &(options->column), column, length);
    }
  } GRN_OPTION_VALUES_EACH_END();

  return options;
}

static void
stop_word_close_options(grn_ctx *ctx, void *data)
{
  grn_stop_word_token_filter_options *options = data;
  GRN_OBJ_FIN(ctx, &(options->column));
  GRN_PLUGIN_FREE(ctx, options);
}

static void *
stop_word_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_tokenize_mode mode;
  grn_obj *lexicon;
  unsigned int i;
  grn_stop_word_token_filter_options *options;
  grn_stop_word_token_filter *token_filter;

  mode = grn_tokenizer_query_get_mode(ctx, query);
  if (mode != GRN_TOKEN_GET) {
    return NULL;
  }

  lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  i = grn_tokenizer_query_get_token_filter_index(ctx, query);
  options = grn_table_cache_token_filter_options(ctx,
                                                 lexicon,
                                                 i,
                                                 stop_word_open_options,
                                                 stop_word_close_options,
                                                 NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_stop_word_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][stop-word] "
                     "failed to allocate grn_stop_word_token_filter");
    return NULL;
  }

  token_filter->options = options;
  token_filter->lexicon = lexicon;
  token_filter->column = grn_obj_column(ctx,
                                        token_filter->lexicon,
                                        GRN_TEXT_VALUE(&(options->column)),
                                        GRN_TEXT_LEN(&(options->column)));
  if (!token_filter->column) {
    char lexicon_name[GRN_TABLE_MAX_KEY_SIZE];
    unsigned int lexicon_name_size;

    lexicon_name_size = grn_obj_name(ctx,
                                     token_filter->lexicon,
                                     lexicon_name,
                                     GRN_TABLE_MAX_KEY_SIZE);
    GRN_PLUGIN_ERROR(ctx, GRN_TOKEN_FILTER_ERROR,
                     "[token-filter][stop-word] "
                     "column for judging stop word doesn't exit: <%.*s.%.*s>",
                     lexicon_name_size,
                     lexicon_name,
                     (int)(GRN_TEXT_LEN(&(options->column))),
                     GRN_TEXT_VALUE(&(options->column)));
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }

  GRN_BOOL_INIT(&(token_filter->value), 0);
  token_filter->is_enabled = true;
  grn_obj *query_options = grn_tokenizer_query_get_options(ctx, query);
  if (query_options) {
    grn_proc_prefixed_options_parse(ctx,
                                    query_options,
                                    "TokenFilterStopWord.",
                                    "[token-filter][stop-word]",
                                    "enable",
                                    GRN_PROC_OPTION_VALUE_BOOL,
                                    &(token_filter->is_enabled),
                                    NULL);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_unlink(ctx, token_filter->column);
      GRN_OBJ_FIN(ctx, &(token_filter->value));
      GRN_PLUGIN_FREE(ctx, token_filter);
      return NULL;
    }
  }
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
stop_word_filter(grn_ctx *ctx,
                 grn_token *current_token,
                 grn_token *next_token,
                 void *user_data)
{
  grn_stop_word_token_filter *token_filter = user_data;
  grn_id id;
  grn_obj *data;

  if (!token_filter) {
    return;
  }

  if (!token_filter->is_enabled) {
    return;
  }

  data = grn_token_get_data(ctx, current_token);
  id = grn_table_get(ctx,
                     token_filter->lexicon,
                     GRN_TEXT_VALUE(data),
                     GRN_TEXT_LEN(data));
  if (id != GRN_ID_NIL) {
    GRN_BULK_REWIND(&(token_filter->value));
    grn_obj_get_value(ctx,
                      token_filter->column,
                      id,
                      &(token_filter->value));
    if (GRN_BOOL_VALUE(&(token_filter->value))) {
      grn_tokenizer_status status;
      status = grn_token_get_status(ctx, current_token);
      status |= GRN_TOKEN_SKIP;
      grn_token_set_status(ctx, next_token, status);
    }
  }
}

static void
stop_word_fin(grn_ctx *ctx, void *user_data)
{
  grn_stop_word_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }

  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  grn_obj_unlink(ctx, token_filter->column);
  GRN_OBJ_FIN(ctx, &(token_filter->value));
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

  token_filter = grn_token_filter_create(ctx, "TokenFilterStopWord", -1);
  grn_token_filter_set_init_func(ctx, token_filter, stop_word_init);
  grn_token_filter_set_filter_func(ctx, token_filter, stop_word_filter);
  grn_token_filter_set_fin_func(ctx, token_filter, stop_word_fin);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
