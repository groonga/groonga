/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2020  Sutou Kouhei <kou@clear-code.com>

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

#include <math.h>
#include <string.h>

#include "grn_token.h"
#include "grn_token_cursor.h"

#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_ii.h"
#include "grn_onigmo.h"
#include "grn_plugin.h"
#include "grn_string.h"

grn_obj *grn_tokenizer_uvector = NULL;

typedef struct {
  grn_tokenizer_token token;
  byte *curr;
  byte *tail;
  uint32_t unit;
} grn_uvector_tokenizer;

static grn_obj *
uvector_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *str, *flags, *mode;
  grn_uvector_tokenizer *tokenizer;
  if (!(flags = grn_ctx_pop(ctx))) {
    ERR(GRN_INVALID_ARGUMENT, "[tokenizer][uvector] missing argument: flags");
    return NULL;
  }
  if (!(str = grn_ctx_pop(ctx))) {
    ERR(GRN_INVALID_ARGUMENT, "[tokenizer][uvector] missing argument: string");
    return NULL;
  }
  if (!(mode = grn_ctx_pop(ctx))) {
    ERR(GRN_INVALID_ARGUMENT, "[tokenizer][uvector] missing argument: mode");
    return NULL;
  }
  if (!(tokenizer = GRN_MALLOC(sizeof(grn_uvector_tokenizer)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][uvector] "
        "memory allocation to grn_uvector_tokenizer failed");
    return NULL;
  }
  user_data->ptr = tokenizer;

  grn_tokenizer_token_init(ctx, &(tokenizer->token));
  tokenizer->curr = (byte *)GRN_TEXT_VALUE(str);
  tokenizer->tail = tokenizer->curr + GRN_TEXT_LEN(str);
  tokenizer->unit = sizeof(grn_id);
  return NULL;
}

static grn_obj *
uvector_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_uvector_tokenizer *tokenizer = user_data->ptr;
  byte *p = tokenizer->curr + tokenizer->unit;
  if (tokenizer->tail < p) {
    grn_tokenizer_token_push(ctx, &(tokenizer->token),
                             (const char *)tokenizer->curr, 0,
                             GRN_TOKEN_LAST);
  } else {
    grn_token_status status;
    if (tokenizer->tail == p) {
      status = GRN_TOKEN_LAST;
    } else {
      status = GRN_TOKEN_CONTINUE;
    }
    grn_tokenizer_token_push(ctx, &(tokenizer->token),
                             (const char *)tokenizer->curr, tokenizer->unit,
                             status);
    tokenizer->curr = p;
  }
  return NULL;
}

static grn_obj *
uvector_fin(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_uvector_tokenizer *tokenizer = user_data->ptr;
  if (!tokenizer) {
    return NULL;
  }
  grn_tokenizer_token_fin(ctx, &(tokenizer->token));
  GRN_FREE(tokenizer);
  return NULL;
}

typedef struct {
  grn_obj delimiters;
#ifdef GRN_SUPPORT_REGEXP
  OnigRegex regex;
#endif /* GRN_SUPPORT_REGEXP */
} grn_delimit_options;

typedef struct {
  const char *delimiter;
  size_t delimiter_length;
} grn_delimit_options_default;

typedef struct {
  grn_tokenizer_query *query;
  grn_delimit_options *options;
  grn_bool have_tokenized_delimiter;
  grn_encoding encoding;
  const unsigned char *start;
  const unsigned char *next;
  const unsigned char *end;
} grn_delimit_tokenizer;

static void
delimit_options_init(grn_delimit_options *options)
{
  GRN_TEXT_INIT(&(options->delimiters), GRN_OBJ_VECTOR);
#ifdef GRN_SUPPORT_REGEXP
  options->regex = NULL;
#endif /* GRN_SUPPORT_REGEXP */
}

static void *
delimit_open_options(grn_ctx *ctx,
                     grn_obj *tokenizer,
                     grn_obj *raw_options,
                     void *user_data)
{
  grn_delimit_options *options;
  grn_delimit_options_default *options_default = user_data;
  grn_bool have_delimiter = GRN_FALSE;

  options = GRN_MALLOC(sizeof(grn_delimit_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][delimit] "
        "failed to allocate memory for options");
    return NULL;
  }

  delimit_options_init(options);

  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "delimiter")) {
      const char *delimiter;
      unsigned int delimiter_length;
      grn_id domain;

      have_delimiter = GRN_TRUE;
      delimiter_length = grn_vector_get_element(ctx,
                                                raw_options,
                                                i,
                                                &delimiter,
                                                NULL,
                                                &domain);
      if (grn_type_id_is_text_family(ctx, domain) && delimiter_length > 0) {
        grn_vector_add_element(ctx,
                               &(options->delimiters),
                               delimiter,
                               delimiter_length,
                               0,
                               GRN_DB_TEXT);
      }
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "pattern")) {
#ifdef GRN_SUPPORT_REGEXP
      const char *pattern;
      unsigned int pattern_length;
      grn_id domain;

      pattern_length = grn_vector_get_element(ctx,
                                              raw_options,
                                              i,
                                              &pattern,
                                              NULL,
                                              &domain);
      if (grn_type_id_is_text_family(ctx, domain) && pattern_length > 0) {
        if (options->regex) {
          onig_free(options->regex);
        }
        options->regex = grn_onigmo_new(ctx,
                                        pattern,
                                        pattern_length,
                                        GRN_ONIGMO_OPTION_DEFAULT,
                                        GRN_ONIGMO_SYNTAX_DEFAULT,
                                        "[tokenizer][delimit]");
      }
#endif /* GRN_SUPPORT_REGEXP */
    }
  } GRN_OPTION_VALUES_EACH_END();

  if (!have_delimiter) {
    grn_vector_add_element(ctx,
                           &(options->delimiters),
                           options_default->delimiter,
                           options_default->delimiter_length,
                           0,
                           GRN_DB_TEXT);
  }

  return options;
}

static void
delimit_close_options(grn_ctx *ctx, void *data)
{
  grn_delimit_options *options = data;

  GRN_OBJ_FIN(ctx, &(options->delimiters));
#ifdef GRN_SUPPORT_REGEXP
  if (options->regex) {
    onig_free(options->regex);
  }
#endif /* GRN_SUPPORT_REGEXP */
  GRN_FREE(options);
}

static void *
delimit_init_raw(grn_ctx *ctx,
                 grn_tokenizer_query *query,
                 grn_delimit_options *options)
{
  grn_delimit_tokenizer *tokenizer;

  if (!(tokenizer = GRN_MALLOC(sizeof(grn_delimit_tokenizer)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][delimit] "
        "memory allocation to grn_delimit_tokenizer failed");
    return NULL;
  }

  tokenizer->query = query;
  tokenizer->options = options;

  {
    const char *raw_string;
    size_t raw_string_length;
    grn_encoding encoding;

    raw_string = grn_tokenizer_query_get_raw_string(ctx,
                                                    tokenizer->query,
                                                    &raw_string_length);
    encoding = grn_tokenizer_query_get_encoding(ctx, tokenizer->query);
    tokenizer->have_tokenized_delimiter =
      grn_tokenizer_have_tokenized_delimiter(ctx,
                                             raw_string,
                                             raw_string_length,
                                             encoding);
    tokenizer->encoding = encoding;
  }
  {
    grn_obj *string;
    const char *normalized;
    unsigned int normalized_length_in_bytes;

    string = grn_tokenizer_query_get_normalized_string(ctx, tokenizer->query);
    grn_string_get_normalized(ctx,
                              string,
                              &normalized, &normalized_length_in_bytes,
                              NULL);
    tokenizer->start = (const unsigned char *)normalized;
    tokenizer->next = tokenizer->start;
    tokenizer->end = tokenizer->start + normalized_length_in_bytes;
  }

  return tokenizer;
}

static void *
delimit_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_delimit_options *options;
  grn_delimit_options_default options_default;

  options_default.delimiter = " ";
  options_default.delimiter_length = 1;

  options = grn_table_cache_default_tokenizer_options(ctx,
                                                      lexicon,
                                                      delimit_open_options,
                                                      delimit_close_options,
                                                      &options_default);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  return delimit_init_raw(ctx, query, options);
}

static void
delimit_next(grn_ctx *ctx,
             grn_tokenizer_query *query,
             grn_token *token,
             void *user_data)
{
  grn_delimit_tokenizer *tokenizer = user_data;

  if (tokenizer->have_tokenized_delimiter) {
    unsigned int rest_length;
    rest_length = tokenizer->end - tokenizer->next;
    tokenizer->next =
      (unsigned char *)grn_tokenizer_next_by_tokenized_delimiter(
        ctx,
        token,
        (const char *)tokenizer->next,
        rest_length,
        tokenizer->encoding);
#ifdef GRN_SUPPORT_REGEXP
  } else if (tokenizer->options->regex) {
    OnigPosition position;
    OnigRegion region;

    onig_region_init(&region);
    position = onig_search(tokenizer->options->regex,
                           tokenizer->start,
                           tokenizer->end,
                           tokenizer->next,
                           tokenizer->end,
                           &region,
                           ONIG_OPTION_NONE);
    if (position == ONIG_MISMATCH) {
      grn_token_set_data(ctx,
                         token,
                         tokenizer->next,
                         tokenizer->end - tokenizer->next);
      grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
    } else {
      grn_token_set_data(ctx,
                         token,
                         tokenizer->next,
                         (tokenizer->start + region.beg[0]) - tokenizer->next);
      grn_token_set_status(ctx, token, GRN_TOKEN_CONTINUE);
      tokenizer->next = tokenizer->start + region.end[0];
      onig_region_free(&region, 0);
    }
#endif /* GRN_SUPPORT_REGEXP */
  } else {
    size_t cl;
    const unsigned char *p = tokenizer->next, *r;
    const unsigned char *e = tokenizer->end;
    grn_token_status status;
    grn_obj *delimiters;
    unsigned int i, n_delimiters;
    delimiters = &(tokenizer->options->delimiters);
    n_delimiters = grn_vector_size(ctx, delimiters);
    for (r = p; r < e; r += cl) {
      if (!(cl = grn_charlen_(ctx, (char *)r, (char *)e, tokenizer->encoding))) {
        tokenizer->next = (unsigned char *)e;
        break;
      }
      {
        grn_bool found_delimiter = GRN_FALSE;
        const unsigned char *current_end = r;
        while (GRN_TRUE) {
          grn_bool found_delimiter_sub = GRN_FALSE;
          for (i = 0; i < n_delimiters; i++) {
            const char *delimiter;
            unsigned int delimiter_length;
            delimiter_length = grn_vector_get_element(ctx,
                                                      delimiters,
                                                      i,
                                                      &delimiter,
                                                      NULL,
                                                      NULL);
            if (current_end + delimiter_length <= e &&
                memcmp(current_end, delimiter, delimiter_length) == 0) {
              current_end += delimiter_length;
              tokenizer->next = current_end;
              found_delimiter = GRN_TRUE;
              found_delimiter_sub = GRN_TRUE;
            }
          }
          if (!found_delimiter_sub) {
            break;
          }
        }
        if (found_delimiter) {
          break;
        }
      }
    }
    if (r == e) {
      status = GRN_TOKEN_LAST;
    } else {
      status = GRN_TOKEN_CONTINUE;
    }
    grn_token_set_data(ctx,
                       token,
                       (const char *)p,
                       r - p);
    grn_token_set_status(ctx, token, status);
  }
}

static void
delimit_fin(grn_ctx *ctx, void *user_data)
{
  grn_delimit_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }
  GRN_FREE(tokenizer);
}

static void *
delimit_null_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_delimit_options *options;
  grn_delimit_options_default options_default;

  options_default.delimiter = "\0";
  options_default.delimiter_length = 1;

  options = grn_table_cache_default_tokenizer_options(ctx,
                                                      lexicon,
                                                      delimit_open_options,
                                                      delimit_close_options,
                                                      &options_default);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  return delimit_init_raw(ctx, query, options);
}

/* ngram tokenizer */

static grn_bool grn_ngram_tokenizer_remove_blank_enable = GRN_TRUE;

typedef struct {
  uint8_t unit;
  grn_bool unify_alphabet;
  grn_bool unify_digit;
  grn_bool unify_symbol;
  grn_bool ignore_blank;
  grn_bool remove_blank;
  grn_bool loose_symbol;
  grn_bool loose_blank;
  grn_bool report_source_location;
  grn_bool include_removed_source_location;
} grn_ngram_options;

typedef struct {
  grn_tokenizer_token token;
  grn_tokenizer_query *query;
  grn_ngram_options options;
  grn_bool overlap;
  struct {
    grn_bool ing;
    grn_bool need;
    grn_bool need_end_mark;
    grn_obj text;
    uint_least8_t *ctypes;
    int16_t *checks;
    uint64_t *offsets;
  } loose;
  int32_t pos;
  uint32_t skip;
  unsigned int n_chars;
  const unsigned char *start;
  const unsigned char *next;
  const unsigned char *end;
  const uint_least8_t *ctypes;
  const int16_t *checks;
  const uint64_t *offsets;
  uint32_t tail;
  uint64_t source_offset;
} grn_ngram_tokenizer;

static void
ngram_options_init(grn_ngram_options *options, uint8_t unit)
{
  options->unit = unit;
  options->unify_alphabet = GRN_TRUE;
  options->unify_digit = GRN_TRUE;
  options->unify_symbol = GRN_TRUE;
  options->ignore_blank = GRN_FALSE;
  options->remove_blank = grn_ngram_tokenizer_remove_blank_enable;
  options->loose_symbol = GRN_FALSE;
  options->loose_blank = GRN_FALSE;
  options->report_source_location = GRN_FALSE;
  options->include_removed_source_location = GRN_TRUE;
}

static void
ngram_switch_to_loose_mode(grn_ctx *ctx,
                           grn_ngram_tokenizer *tokenizer)
{
  grn_obj *string;
  const char *normalized;
  unsigned int normalized_length_in_bytes;
  unsigned int normalized_length_in_chars;
  const char *normalized_end;
  const uint_least8_t *types = tokenizer->ctypes;
  const int16_t *checks = tokenizer->checks;
  const uint64_t *offsets = tokenizer->offsets;

  string = grn_tokenizer_query_get_normalized_string(ctx, tokenizer->query);
  grn_string_get_normalized(ctx,
                            string,
                            &normalized,
                            &normalized_length_in_bytes,
                            &normalized_length_in_chars);
  normalized_end = normalized + normalized_length_in_bytes;

  if (types) {
    grn_encoding encoding =
      grn_tokenizer_query_get_encoding(ctx, tokenizer->query);
    uint_least8_t *loose_types;
    int16_t *loose_checks = NULL;
    uint64_t *loose_offsets = NULL;
    const int16_t *removed_checks = NULL;
    uint64_t last_offset = 0;
    unsigned int n_chars = 0;

    tokenizer->loose.ctypes =
      GRN_MALLOC(sizeof(uint_least8_t) * (normalized_length_in_chars + 1));
    if (!tokenizer->loose.ctypes) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[tokenizer][ngram][loose] "
          "failed to allocate memory for character types");
      return;
    }
    loose_types = tokenizer->loose.ctypes;
    if (checks) {
      tokenizer->loose.checks =
        GRN_CALLOC(sizeof(int16_t) * normalized_length_in_bytes);
      if (!tokenizer->loose.checks) {
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "[tokenizer][ngram][loose] "
            "failed to allocate memory for character lengths");
        return;
      }
    }
    if (offsets) {
      tokenizer->loose.offsets =
        GRN_CALLOC(sizeof(uint64_t) * (normalized_length_in_chars + 1));
      if (!tokenizer->loose.offsets) {
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "[tokenizer][ngram][loose] "
            "failed to allocate memory for character offsets");
        return;
      }
    }
    loose_types = tokenizer->loose.ctypes;
    loose_checks = tokenizer->loose.checks;
    loose_offsets = tokenizer->loose.offsets;
    while (normalized < normalized_end) {
      size_t length;
      length = grn_charlen_(ctx,
                            (char *)normalized,
                            (char *)normalized_end,
                            encoding);
      if (length == 0) {
        break;
      }
      if ((tokenizer->options.loose_symbol &&
           GRN_STR_CTYPE(*types) == GRN_CHAR_SYMBOL) ||
          (!tokenizer->options.remove_blank &&
           tokenizer->options.loose_blank &&
           GRN_STR_ISBLANK(*types))) {
        if (tokenizer->options.include_removed_source_location) {
          if (!removed_checks) {
            removed_checks = checks;
          }
        }
        if (offsets && last_offset == 0) {
          last_offset = *offsets;
        }
      } else {
        GRN_TEXT_PUT(ctx, &(tokenizer->loose.text), normalized, length);
        *loose_types = *types;
        if (tokenizer->options.loose_blank && GRN_STR_ISBLANK(*types)) {
          *loose_types &= ~GRN_STR_BLANK;
        }
        loose_types++;
        if (loose_checks) {
          size_t i;
          if (tokenizer->options.include_removed_source_location) {
            for (; removed_checks && removed_checks < checks; removed_checks++) {
              if (*removed_checks > 0) {
                *loose_checks += *removed_checks;
              }
            }
            removed_checks = NULL;
          }
          for (i = 0; i < length; i++) {
            if (checks[i] != -1) {
              loose_checks[i] += checks[i];
            }
          }
          loose_checks += length;
        }
        if (loose_offsets) {
          *loose_offsets = *offsets;
          loose_offsets++;
          last_offset = 0;
        }
        n_chars++;
      }
      normalized += length;
      types++;
      if (checks) {
        checks += length;
      }
      if (offsets) {
        offsets++;
      }
    }
    *loose_types = *types;
    if (offsets) {
      if (last_offset) {
        *loose_offsets = last_offset;
      } else {
        *loose_offsets = *offsets;
      }
    }
    tokenizer->start =
      (const unsigned char *)GRN_TEXT_VALUE(&(tokenizer->loose.text));
    tokenizer->next = tokenizer->start;
    tokenizer->end = tokenizer->start + GRN_TEXT_LEN(&(tokenizer->loose.text));
    tokenizer->ctypes = tokenizer->loose.ctypes;
    tokenizer->checks = tokenizer->loose.checks;
    tokenizer->offsets = tokenizer->loose.offsets;
    tokenizer->n_chars = n_chars;
  } else {
    tokenizer->start = normalized;
    tokenizer->next = tokenizer->start;
    tokenizer->end = normalized_end;
  }

  tokenizer->pos = 0;
  tokenizer->skip = 0;
  tokenizer->overlap = GRN_FALSE;
  tokenizer->loose.ing = GRN_TRUE;
  tokenizer->source_offset = 0;
}

static void *
ngram_init_raw(grn_ctx *ctx,
               grn_tokenizer_query *query,
               const grn_ngram_options *options)
{
  unsigned int normalize_flags =
    GRN_STRING_REMOVE_BLANK |
    GRN_STRING_WITH_TYPES |
    GRN_STRING_REMOVE_TOKENIZED_DELIMITER;
  grn_ngram_tokenizer *tokenizer;

  if (!options->remove_blank) {
    normalize_flags &= ~GRN_STRING_REMOVE_BLANK;
  }
  if (options->report_source_location) {
    normalize_flags |= GRN_STRING_WITH_CHECKS;
  }
  grn_tokenizer_query_set_normalize_flags(ctx, query, normalize_flags);

  if (!(tokenizer = GRN_MALLOC(sizeof(grn_ngram_tokenizer)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][ngram] "
        "memory allocation to grn_ngram_tokenizer failed");
    return NULL;
  }

  grn_tokenizer_token_init(ctx, &(tokenizer->token));
  tokenizer->query = query;

  tokenizer->options = *options;
  tokenizer->overlap = GRN_FALSE;
  tokenizer->loose.ing = GRN_FALSE;
  tokenizer->loose.need = GRN_FALSE;
  tokenizer->loose.need_end_mark = GRN_FALSE;
  GRN_TEXT_INIT(&(tokenizer->loose.text), 0);
  tokenizer->loose.ctypes = NULL;
  tokenizer->loose.checks = NULL;
  tokenizer->loose.offsets = NULL;
  tokenizer->pos = 0;
  tokenizer->skip = 0;
  tokenizer->source_offset = 0;

  {
    grn_obj *string;
    const char *normalized_raw;
    unsigned int normalized_length_in_bytes;
    unsigned int normalized_length_in_chars;
    const unsigned char *start;
    const unsigned char *next;
    const unsigned char *end;
    unsigned int n_chars;
    const uint_least8_t *ctypes;
    const int16_t *checks;
    const uint64_t *offsets;

    string = grn_tokenizer_query_get_normalized_string(ctx, tokenizer->query);
    grn_string_get_normalized(ctx,
                              string,
                              &normalized_raw,
                              &normalized_length_in_bytes,
                              &normalized_length_in_chars);
    tokenizer->start = start = (const unsigned char *)normalized_raw;
    tokenizer->next = next = tokenizer->start;
    tokenizer->end = end = tokenizer->start + normalized_length_in_bytes;
    tokenizer->n_chars = n_chars = normalized_length_in_chars;
    tokenizer->ctypes = ctypes = grn_string_get_types(ctx, string);
    tokenizer->checks = checks = grn_string_get_checks(ctx, string);
    tokenizer->offsets = offsets = grn_string_get_offsets(ctx, string);
    if (grn_tokenizer_query_get_mode(ctx, tokenizer->query) == GRN_TOKEN_GET) {
      ngram_switch_to_loose_mode(ctx, tokenizer);
      if (tokenizer->n_chars == 0) {
        tokenizer->start = start;
        tokenizer->next = next;
        tokenizer->end = end;
        tokenizer->n_chars = n_chars;
        tokenizer->ctypes = ctypes;
        tokenizer->checks = checks;
        tokenizer->offsets = offsets;
      }
    }
  }

  return tokenizer;
}

static grn_obj *
ngram_init_deprecated(grn_ctx *ctx,
                      int nargs,
                      grn_obj **args,
                      grn_user_data *user_data,
                      const grn_ngram_options *options)
{
  unsigned int normalize_flags =
    GRN_STRING_REMOVE_BLANK |
    GRN_STRING_WITH_TYPES |
    GRN_STRING_REMOVE_TOKENIZED_DELIMITER;
  grn_tokenizer_query *query;

  if (!options->remove_blank) {
    normalize_flags &= ~GRN_STRING_REMOVE_BLANK;
  }
  query = grn_tokenizer_query_open(ctx, nargs, args, normalize_flags);
  if (!query) {
    return NULL;
  }

  user_data->ptr = ngram_init_raw(ctx, query, options);
  if (!user_data->ptr) {
    grn_tokenizer_query_close(ctx, query);
  }
  return NULL;
}

static grn_obj *
unigram_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 1);
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigram_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
trigram_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 3);
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigrams_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.unify_symbol = GRN_FALSE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigramsa_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.unify_symbol = GRN_FALSE;
  options.unify_alphabet = GRN_FALSE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigramsad_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.unify_symbol = GRN_FALSE;
  options.unify_alphabet = GRN_FALSE;
  options.unify_digit = GRN_FALSE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigrami_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.ignore_blank = GRN_TRUE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigramis_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.ignore_blank = GRN_TRUE;
  options.unify_symbol = GRN_FALSE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigramisa_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.ignore_blank = GRN_TRUE;
  options.unify_symbol = GRN_FALSE;
  options.unify_alphabet = GRN_FALSE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static grn_obj *
bigramisad_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_options options;
  ngram_options_init(&options, 2);
  options.ignore_blank = GRN_TRUE;
  options.unify_symbol = GRN_FALSE;
  options.unify_alphabet = GRN_FALSE;
  options.unify_digit = GRN_FALSE;
  return ngram_init_deprecated(ctx, nargs, args, user_data, &options);
}

static void *
ngram_open_options(grn_ctx *ctx,
                   grn_obj *tokenizer,
                   grn_obj *raw_options,
                   void *user_data)
{
  grn_ngram_options *options;

  options = GRN_MALLOC(sizeof(grn_ngram_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][ngram] "
        "failed to allocate memory for options");
    return NULL;
  }

  ngram_options_init(options, 2);

  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "n")) {
      options->unit = grn_vector_get_element_uint8(ctx,
                                                   raw_options,
                                                   i,
                                                   options->unit);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_blank")) {
      options->remove_blank = grn_vector_get_element_bool(ctx,
                                                          raw_options,
                                                          i,
                                                          options->remove_blank);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "loose_symbol")) {
      options->loose_symbol = grn_vector_get_element_bool(ctx,
                                                          raw_options,
                                                          i,
                                                          options->loose_symbol);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "loose_blank")) {
      options->loose_blank = grn_vector_get_element_bool(ctx,
                                                         raw_options,
                                                         i,
                                                         options->loose_blank);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "report_source_location")) {
      options->report_source_location =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->report_source_location);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "include_removed_source_location")) {
      options->include_removed_source_location =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->include_removed_source_location);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_alphabet")) {
      options->unify_alphabet =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_alphabet);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_digit")) {
      options->unify_digit =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_digit);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_symbol")) {
      options->unify_symbol =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_symbol);
    }
  } GRN_OPTION_VALUES_EACH_END();

  return options;
}

static void
ngram_close_options(grn_ctx *ctx, void *data)
{
  grn_ngram_options *options = data;
  GRN_FREE(options);
}

static void *
ngram_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_ngram_options *options;

  options = grn_table_cache_default_tokenizer_options(ctx,
                                                      lexicon,
                                                      ngram_open_options,
                                                      ngram_close_options,
                                                      NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  return ngram_init_raw(ctx, query, options);
}

static void
ngram_next(grn_ctx *ctx,
           grn_tokenizer_query *query,
           grn_token *token,
           void *user_data)
{
  grn_ngram_tokenizer *tokenizer = user_data;
  size_t cl;
  const unsigned char *p = tokenizer->next, *r = p, *e = tokenizer->end;
  int32_t n_characters = 0;
  int32_t pos = tokenizer->pos + tokenizer->skip;
  grn_token_status status = 0;
  const uint_least8_t *cp = tokenizer->ctypes ? tokenizer->ctypes + pos : NULL;
  const int16_t *checks = NULL;
  const uint64_t *offsets = tokenizer->offsets ? tokenizer->offsets + pos : NULL;
  grn_encoding encoding = grn_tokenizer_query_get_encoding(ctx, query);

  if (tokenizer->checks) {
    checks = tokenizer->checks + (p - tokenizer->start);
  }

  if (tokenizer->loose.ing && tokenizer->loose.need_end_mark) {
    grn_token_set_data(ctx,
                       token,
                       GRN_TOKENIZER_END_MARK_UTF8,
                       GRN_TOKENIZER_END_MARK_UTF8_LEN);
    grn_token_set_status(ctx, token, status);
    if (offsets) {
      grn_token_set_source_offset(ctx,
                                  token,
                                  tokenizer->offsets[tokenizer->n_chars]);
    } else if (checks) {
      grn_token_set_source_offset(ctx,
                                  token,
                                  tokenizer->source_offset);
    }
    ngram_switch_to_loose_mode(ctx, tokenizer);
    tokenizer->loose.need_end_mark = GRN_FALSE;
    return;
  }

#define LOOSE_NEED_CHECK(cp, tokenizer) do {                            \
    if (cp &&                                                           \
        !tokenizer->loose.ing &&                                        \
        !tokenizer->loose.need &&                                       \
        ((tokenizer->options.loose_symbol &&                            \
          GRN_STR_CTYPE(*cp) == GRN_CHAR_SYMBOL) ||                     \
         (tokenizer->options.loose_blank && GRN_STR_ISBLANK(*cp)))) {   \
      tokenizer->loose.need = GRN_TRUE;                                 \
    }                                                                   \
  } while (GRN_FALSE)

  LOOSE_NEED_CHECK(cp, tokenizer);

  if (cp && tokenizer->options.unify_alphabet &&
      GRN_STR_CTYPE(*cp) == GRN_CHAR_ALPHA) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, encoding))) {
      n_characters++;
      r += cl;
      LOOSE_NEED_CHECK(cp, tokenizer);
      if (/* !tokenizer->options.ignore_blank && */ GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != GRN_CHAR_ALPHA) { break; }
    }
    tokenizer->next = r;
    tokenizer->overlap = GRN_FALSE;
  } else if (cp &&
             tokenizer->options.unify_digit &&
             GRN_STR_CTYPE(*cp) == GRN_CHAR_DIGIT) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, encoding))) {
      n_characters++;
      r += cl;
      LOOSE_NEED_CHECK(cp, tokenizer);
      if (/* !tokenizer->options.ignore_blank && */ GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != GRN_CHAR_DIGIT) { break; }
    }
    tokenizer->next = r;
    tokenizer->overlap = GRN_FALSE;
  } else if (cp &&
             tokenizer->options.unify_symbol &&
             GRN_STR_CTYPE(*cp) == GRN_CHAR_SYMBOL) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, encoding))) {
      n_characters++;
      r += cl;
      LOOSE_NEED_CHECK(cp, tokenizer);
      if (!tokenizer->options.ignore_blank && GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != GRN_CHAR_SYMBOL) { break; }
    }
    tokenizer->next = r;
    tokenizer->overlap = GRN_FALSE;
  } else {
#ifdef PRE_DEFINED_UNSPLIT_WORDS
    const unsigned char *key = NULL;
    // todo : grn_pat_lcp_search
    if ((tid = grn_sym_common_prefix_search(sym, p))) {
      if (!(key = _grn_sym_key(sym, tid))) {
        tokenizer->status = GRN_TOKEN_CURSOR_NOT_FOUND;
        return;
      }
      len = grn_str_len(key, encoding, NULL);
    }
    r = p + grn_charlen_(ctx, p, e, encoding);
    if (tid && (len > 1 || r == p)) {
      if (r != p && pos + len - 1 <= tokenizer->tail) { continue; }
      p += strlen(key);
      if (!*p && tokenizer->mode == GRN_TOKEN_GET) {
        tokenizer->status = GRN_TOKEN_CURSOR_DONE;
      }
    }
#endif /* PRE_DEFINED_UNSPLIT_WORDS */
    if ((cl = grn_charlen_(ctx, (char *)r, (char *)e, encoding))) {
      n_characters++;
      r += cl;
      tokenizer->next = r;
      while (n_characters < tokenizer->options.unit &&
             (cl = grn_charlen_(ctx, (char *)r, (char *)e, encoding))) {
        if (cp) {
          LOOSE_NEED_CHECK(cp, tokenizer);
          if (!tokenizer->options.ignore_blank && GRN_STR_ISBLANK(*cp)) { break; }
          cp++;
          if ((tokenizer->options.unify_alphabet &&
               GRN_STR_CTYPE(*cp) == GRN_CHAR_ALPHA) ||
              (tokenizer->options.unify_digit &&
               GRN_STR_CTYPE(*cp) == GRN_CHAR_DIGIT) ||
              (tokenizer->options.unify_symbol &&
               GRN_STR_CTYPE(*cp) == GRN_CHAR_SYMBOL)) {
            break;
          }
        }
        n_characters++;
        r += cl;
      }
      if (tokenizer->overlap) {
        status |= GRN_TOKEN_OVERLAP;
      }
      if (n_characters < tokenizer->options.unit) {
        status |= GRN_TOKEN_UNMATURED;
      }
      tokenizer->overlap = (n_characters > 1) ? GRN_TRUE : GRN_FALSE;
    }
  }
  tokenizer->pos = pos;
  tokenizer->tail = pos + n_characters - 1;
  if (p == r || tokenizer->next == e) {
    tokenizer->skip = 0;
    status |= GRN_TOKEN_LAST;
  } else {
    tokenizer->skip = tokenizer->overlap ? 1 : n_characters;
  }
  if (r == e) { status |= GRN_TOKEN_REACH_END; }

  {
    size_t data_size = r - p;
    if ((status & (GRN_TOKEN_LAST | GRN_TOKEN_REACH_END)) &&
        !tokenizer->loose.ing && tokenizer->loose.need) {
      status &= ~(GRN_TOKEN_LAST | GRN_TOKEN_REACH_END);
      tokenizer->loose.ing = GRN_TRUE;
      tokenizer->loose.need_end_mark = GRN_TRUE;
    }
    grn_token_set_data(ctx, token, p, data_size);
    grn_token_set_status(ctx, token, status);
    grn_token_set_overlap(ctx, token, tokenizer->overlap);
    /* TODO: Clean and complete... */
    if (offsets) {
      grn_token_set_source_offset(ctx, token, offsets[0]);
      if (checks) {
        size_t i;
        uint32_t source_first_character_length = 0;
        if (checks[0] == -1) {
          size_t n_leading_bytes = p - tokenizer->start;
          for (i = 1; i <= n_leading_bytes; i++) {
            if (checks[-i] > 0) {
              source_first_character_length = checks[-i];
              break;
            }
          }
        }
        {
          for (i = 0; i < data_size; i++) {
            if (checks[i] > 0) {
              if (source_first_character_length == 0) {
                source_first_character_length = checks[i];
              }
            }
          }
        }
        grn_token_set_source_length(ctx,
                                    token,
                                    offsets[n_characters] - offsets[0]);
        grn_token_set_source_first_character_length(ctx,
                                                    token,
                                                    source_first_character_length);
      }
    } else {
      if (checks) {
        size_t i;
        uint32_t source_length = 0;
        uint32_t source_first_character_length = 0;
        uint64_t next_offset = tokenizer->source_offset;
        grn_token_set_source_offset(ctx, token, tokenizer->source_offset);
        if (checks[0] == -1) {
          size_t n_leading_bytes = p - tokenizer->start;
          for (i = 1; i <= n_leading_bytes; i++) {
            if (checks[-i] > 0) {
              source_length = source_first_character_length = checks[-i];
              if (!tokenizer->overlap) {
                next_offset += checks[-i];
              }
              break;
            }
          }
        }
        {
          uint64_t first_offset = 0;
          for (i = 0; i < data_size; i++) {
            if (checks[i] > 0) {
              if ((tokenizer->overlap && first_offset == 0) ||
                  !tokenizer->overlap) {
                if (first_offset == 0) {
                  first_offset = checks[i];
                }
                next_offset += checks[i];
              }
              if (source_first_character_length == 0) {
                source_first_character_length = checks[i];
              }
              source_length += checks[i];
            } else if (checks[i] < 0) {
              if (tokenizer->overlap) {
                next_offset -= first_offset;
              }
            }
          }
        }
        grn_token_set_source_length(ctx, token, source_length);
        grn_token_set_source_first_character_length(ctx,
                                                    token,
                                                    source_first_character_length);
        tokenizer->source_offset = next_offset;
      }
    }
  }
}

static grn_obj *
ngram_next_deprecated(grn_ctx *ctx,
                      int nargs,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  grn_ngram_tokenizer *tokenizer = user_data->ptr;
  grn_token token;
  grn_obj *token_data;

  grn_token_init(ctx, &token);
  ngram_next(ctx, tokenizer->query, &token, tokenizer);
  token_data = grn_token_get_data(ctx, &token);
  grn_tokenizer_token_push(ctx,
                           &(tokenizer->token),
                           GRN_TEXT_VALUE(token_data),
                           GRN_TEXT_LEN(token_data),
                           grn_token_get_status(ctx, &token));
  grn_token_fin(ctx, &token);
  return NULL;
}

static void
ngram_fin(grn_ctx *ctx, void *user_data)
{
  grn_ngram_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }
  if (tokenizer->loose.ctypes) {
    GRN_FREE(tokenizer->loose.ctypes);
  }
  if (tokenizer->loose.checks) {
    GRN_FREE(tokenizer->loose.checks);
  }
  if (tokenizer->loose.offsets) {
    GRN_FREE(tokenizer->loose.offsets);
  }
  GRN_OBJ_FIN(ctx, &(tokenizer->loose.text));
  grn_tokenizer_token_fin(ctx, &(tokenizer->token));
  GRN_FREE(tokenizer);
}

static grn_obj *
ngram_fin_deprecated(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_ngram_tokenizer *tokenizer = user_data->ptr;
  if (tokenizer) {
    grn_tokenizer_query_close(ctx, tokenizer->query);
    ngram_fin(ctx, tokenizer);
  }
  return NULL;
}

/* regexp tokenizer */

typedef struct {
  grn_tokenizer_token token;
  grn_tokenizer_query *query;
  struct {
    int32_t n_skip_tokens;
  } get;
  grn_bool is_begin;
  grn_bool is_end;
  grn_bool is_start_token;
  grn_bool is_overlapping;
  const char *next;
  const char *end;
  unsigned int nth_char;
  const uint_least8_t *char_types;
  grn_obj buffer;
} grn_regexp_tokenizer;

static grn_obj *
regexp_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  unsigned int normalize_flags = GRN_STRING_WITH_TYPES;
  grn_tokenizer_query *query;
  grn_regexp_tokenizer *tokenizer;

  query = grn_tokenizer_query_open(ctx, nargs, args, normalize_flags);
  if (!query) {
    return NULL;
  }

  tokenizer = GRN_MALLOC(sizeof(grn_regexp_tokenizer));
  if (!tokenizer) {
    grn_tokenizer_query_close(ctx, query);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][regexp] failed to allocate memory");
    return NULL;
  }
  user_data->ptr = tokenizer;

  grn_tokenizer_token_init(ctx, &(tokenizer->token));
  tokenizer->query = query;

  tokenizer->get.n_skip_tokens = 0;

  tokenizer->is_begin = GRN_TRUE;
  tokenizer->is_end   = GRN_FALSE;
  tokenizer->is_start_token = GRN_TRUE;
  tokenizer->is_overlapping = GRN_FALSE;

  {
    grn_obj *string;
    const char *normalized;
    unsigned int normalized_length_in_bytes;

    string = grn_tokenizer_query_get_normalized_string(ctx, tokenizer->query);
    grn_string_get_normalized(ctx,
                              string,
                              &normalized, &normalized_length_in_bytes,
                              NULL);
    tokenizer->next = normalized;
    tokenizer->end = tokenizer->next + normalized_length_in_bytes;
    tokenizer->nth_char = 0;
    tokenizer->char_types = grn_string_get_types(ctx, string);
  }

  GRN_TEXT_INIT(&(tokenizer->buffer), 0);

  return NULL;
}

static grn_obj *
regexp_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int char_len;
  grn_token_status status = 0;
  grn_regexp_tokenizer *tokenizer = user_data->ptr;
  unsigned int n_characters = 0;
  int ngram_unit = 2;
  grn_obj *buffer = &(tokenizer->buffer);
  const char *current = tokenizer->next;
  const char *end = tokenizer->end;
  const uint_least8_t *char_types = tokenizer->char_types;
  const grn_tokenize_mode mode =
    grn_tokenizer_query_get_mode(ctx, tokenizer->query);
  grn_encoding encoding =
    grn_tokenizer_query_get_encoding(ctx, tokenizer->query);
  grn_bool is_begin = tokenizer->is_begin;
  grn_bool is_start_token = tokenizer->is_start_token;
  grn_bool break_by_blank = GRN_FALSE;
  grn_bool break_by_end_mark = GRN_FALSE;

  GRN_BULK_REWIND(buffer);
  tokenizer->is_begin = GRN_FALSE;
  tokenizer->is_start_token = GRN_FALSE;

  if (char_types) {
    char_types += tokenizer->nth_char;
  }

  if (mode != GRN_TOKEN_GET) {
    if (is_begin) {
      grn_tokenizer_token_push(ctx,
                               &(tokenizer->token),
                               GRN_TOKENIZER_BEGIN_MARK_UTF8,
                               GRN_TOKENIZER_BEGIN_MARK_UTF8_LEN,
                               status);
      return NULL;
    }

    if (tokenizer->is_end) {
      status |= GRN_TOKEN_LAST | GRN_TOKEN_REACH_END;
      grn_tokenizer_token_push(ctx,
                               &(tokenizer->token),
                               GRN_TOKENIZER_END_MARK_UTF8,
                               GRN_TOKENIZER_END_MARK_UTF8_LEN,
                               status);
      return NULL;
    }
    if (is_start_token) {
      if (char_types && GRN_STR_ISBLANK(char_types[-1])) {
        status |= GRN_TOKEN_SKIP;
        grn_tokenizer_token_push(ctx, &(tokenizer->token), "", 0, status);
        return NULL;
      }
    }
  }

  char_len = grn_charlen_(ctx, current, end, encoding);
  if (char_len == 0) {
    status |= GRN_TOKEN_LAST | GRN_TOKEN_REACH_END;
    grn_tokenizer_token_push(ctx, &(tokenizer->token), "", 0, status);
    return NULL;
  }

  if (mode == GRN_TOKEN_GET) {
    if (is_begin &&
        char_len == GRN_TOKENIZER_BEGIN_MARK_UTF8_LEN &&
        memcmp(current, GRN_TOKENIZER_BEGIN_MARK_UTF8, char_len) == 0) {
      tokenizer->is_start_token = GRN_TRUE;
      n_characters++;
      GRN_TEXT_PUT(ctx, buffer, current, char_len);
      current += char_len;
      tokenizer->next = current;
      tokenizer->nth_char++;
      if (current == end) {
        status |= GRN_TOKEN_LAST | GRN_TOKEN_REACH_END;
      }
      grn_tokenizer_token_push(ctx,
                               &(tokenizer->token),
                               GRN_TOKENIZER_BEGIN_MARK_UTF8,
                               GRN_TOKENIZER_BEGIN_MARK_UTF8_LEN,
                               status);
      return NULL;
    }

    if (current + char_len == end &&
        char_len == GRN_TOKENIZER_END_MARK_UTF8_LEN &&
        memcmp(current, GRN_TOKENIZER_END_MARK_UTF8, char_len) == 0) {
      status |= GRN_TOKEN_LAST | GRN_TOKEN_REACH_END;
      grn_tokenizer_token_push(ctx,
                               &(tokenizer->token),
                               GRN_TOKENIZER_END_MARK_UTF8,
                               GRN_TOKENIZER_END_MARK_UTF8_LEN,
                               status);
      return NULL;
    }
  }

  while (GRN_TRUE) {
    n_characters++;
    GRN_TEXT_PUT(ctx, buffer, current, char_len);
    current += char_len;
    if (n_characters == 1) {
      tokenizer->next = current;
      tokenizer->nth_char++;
    }

    if (char_types) {
      uint_least8_t char_type;
      char_type = char_types[0];
      char_types++;
      if (GRN_STR_ISBLANK(char_type)) {
        break_by_blank = GRN_TRUE;
      }
    }

    char_len = grn_charlen_(ctx,
                            (const char *)current,
                            (const char *)end,
                            encoding);
    if (char_len == 0) {
      break;
    }

    if (mode == GRN_TOKEN_GET &&
        current + char_len == end &&
        char_len == GRN_TOKENIZER_END_MARK_UTF8_LEN &&
        memcmp(current, GRN_TOKENIZER_END_MARK_UTF8, char_len) == 0) {
      break_by_end_mark = GRN_TRUE;
    }

    if (break_by_blank || break_by_end_mark) {
      break;
    }

    if (n_characters == ngram_unit) {
      break;
    }
  }

  if (tokenizer->is_overlapping) {
    status |= GRN_TOKEN_OVERLAP;
  }
  if (n_characters < ngram_unit) {
    status |= GRN_TOKEN_UNMATURED;
  }
  tokenizer->is_overlapping = (n_characters > 1);

  if (mode == GRN_TOKEN_GET) {
    if (current == end) {
      tokenizer->is_end = GRN_TRUE;
      status |= GRN_TOKEN_LAST | GRN_TOKEN_REACH_END;
      if (status & GRN_TOKEN_UNMATURED) {
        status |= GRN_TOKEN_FORCE_PREFIX;
      }
    } else {
      if (break_by_blank) {
        tokenizer->get.n_skip_tokens = 0;
        tokenizer->is_start_token = GRN_TRUE;
      } else if (break_by_end_mark) {
        if (!is_start_token && (status & GRN_TOKEN_UNMATURED)) {
          status |= GRN_TOKEN_SKIP;
        }
      } else if (tokenizer->get.n_skip_tokens > 0) {
        tokenizer->get.n_skip_tokens--;
        status |= GRN_TOKEN_SKIP;
      } else {
        tokenizer->get.n_skip_tokens = ngram_unit - 1;
      }
    }
  } else {
    if (tokenizer->next == end) {
      tokenizer->is_end = GRN_TRUE;
    }
    if (break_by_blank) {
      tokenizer->is_start_token = GRN_TRUE;
    }
  }

  grn_tokenizer_token_push(ctx,
                           &(tokenizer->token),
                           GRN_TEXT_VALUE(buffer),
                           GRN_TEXT_LEN(buffer),
                           status);

  return NULL;
}

static grn_obj *
regexp_fin(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_regexp_tokenizer *tokenizer = user_data->ptr;
  if (!tokenizer) {
    return NULL;
  }
  grn_tokenizer_token_fin(ctx, &(tokenizer->token));
  grn_tokenizer_query_close(ctx, tokenizer->query);
  GRN_OBJ_FIN(ctx, &(tokenizer->buffer));
  GRN_FREE(tokenizer);
  return NULL;
}

/* pattern tokenizer */

typedef struct {
#ifdef GRN_SUPPORT_REGEXP
  OnigRegex regex;
#else /* GRN_SUPPORT_REGEXP */
  void *regex;
#endif /* GRN_SUPPORT_REGEXP */
} grn_pattern_options;

typedef struct {
  grn_tokenizer_token token;
  grn_tokenizer_query *query;
  grn_pattern_options *options;
  grn_bool have_tokenized_delimiter;
  grn_encoding encoding;
  const unsigned char *start;
  const unsigned char *next;
  const unsigned char *end;
  const unsigned char *current;
  size_t current_length;
} grn_pattern_tokenizer;

static void
pattern_options_init(grn_pattern_options *options)
{
  options->regex = NULL;
}

static void *
pattern_open_options(grn_ctx *ctx,
                     grn_obj *tokenizer,
                     grn_obj *raw_options,
                     void *user_data)
{
  grn_pattern_options *options;
  grn_obj all_patterns;

  options = GRN_MALLOC(sizeof(grn_pattern_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][pattern] "
        "failed to allocate memory for options");
    return NULL;
  }

  pattern_options_init(options);
  GRN_TEXT_INIT(&all_patterns, 0);
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "pattern")) {
      const char *pattern;
      unsigned int pattern_length;
      grn_id domain;

      pattern_length = grn_vector_get_element(ctx,
                                              raw_options,
                                              i,
                                              &pattern,
                                              NULL,
                                              &domain);
      if (grn_type_id_is_text_family(ctx, domain) && pattern_length > 0) {
        if (GRN_TEXT_LEN(&all_patterns) > 0) {
          GRN_TEXT_PUTS(ctx, &all_patterns, "|");
        }
        GRN_TEXT_PUTS(ctx, &all_patterns, "(?:");
        GRN_TEXT_PUT(ctx, &all_patterns, pattern, pattern_length);
        GRN_TEXT_PUTS(ctx, &all_patterns, ")");
      }
    }
  } GRN_OPTION_VALUES_EACH_END();

  if (GRN_TEXT_LEN(&all_patterns) > 0) {
#ifdef GRN_SUPPORT_REGEXP
    options->regex = grn_onigmo_new(ctx,
                                    GRN_TEXT_VALUE(&all_patterns),
                                    GRN_TEXT_LEN(&all_patterns),
                                    GRN_ONIGMO_OPTION_DEFAULT,
                                    GRN_ONIGMO_SYNTAX_DEFAULT,
                                    "[tokenizer][pattern]");
#endif /* GRN_SUPPORT_REGEXP */
  }
  GRN_OBJ_FIN(ctx, &all_patterns);

  return options;
}

static void
pattern_close_options(grn_ctx *ctx, void *data)
{
  grn_pattern_options *options = data;

#ifdef GRN_SUPPORT_REGEXP
  if (options->regex) {
    onig_free(options->regex);
  }
#endif /* GRN_SUPPORT_REGEXP */
  GRN_FREE(options);
}

static void *
pattern_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_pattern_options *options;
  grn_pattern_tokenizer *tokenizer;

  options = grn_table_cache_default_tokenizer_options(ctx,
                                                      lexicon,
                                                      pattern_open_options,
                                                      pattern_close_options,
                                                      NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  if (!(tokenizer = GRN_MALLOC(sizeof(grn_pattern_tokenizer)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][pattern] "
        "memory allocation to grn_pattern_tokenizer failed");
    return NULL;
  }

  tokenizer->query = query;
  tokenizer->options = options;

  {
    const char *raw_string;
    size_t raw_string_length;
    grn_encoding encoding;

    raw_string = grn_tokenizer_query_get_raw_string(ctx,
                                                    tokenizer->query,
                                                    &raw_string_length);
    encoding = grn_tokenizer_query_get_encoding(ctx, tokenizer->query);
    tokenizer->have_tokenized_delimiter =
      grn_tokenizer_have_tokenized_delimiter(ctx,
                                             raw_string,
                                             raw_string_length,
                                             encoding);
    tokenizer->encoding = encoding;
  }
  {
    grn_obj *string;
    const char *normalized;
    unsigned int normalized_length_in_bytes;

    string = grn_tokenizer_query_get_normalized_string(ctx, tokenizer->query);
    grn_string_get_normalized(ctx,
                              string,
                              &normalized, &normalized_length_in_bytes,
                              NULL);
    tokenizer->start = (const unsigned char *)normalized;
    tokenizer->next = tokenizer->start;
    tokenizer->end = tokenizer->start + normalized_length_in_bytes;
    tokenizer->current = NULL;
    tokenizer->current_length = 0;
  }

  return tokenizer;
}

#ifdef GRN_SUPPORT_REGEXP
static void
pattern_search(grn_ctx *ctx,
               grn_pattern_tokenizer *tokenizer)
{
  OnigPosition position;
  OnigRegion region;

  onig_region_init(&region);
  position = onig_search(tokenizer->options->regex,
                         tokenizer->start,
                         tokenizer->end,
                         tokenizer->next,
                         tokenizer->end,
                         &region,
                         ONIG_OPTION_NONE);
  if (position == ONIG_MISMATCH) {
    tokenizer->current = NULL;
    tokenizer->current_length = 0;
    tokenizer->next = tokenizer->end;
  } else {
    tokenizer->current = tokenizer->start + region.beg[0];
    tokenizer->current_length = region.end[0] - region.beg[0];
    tokenizer->next = tokenizer->start + region.end[0];
  }
  onig_region_free(&region, 0);
}
#endif

static void
pattern_next(grn_ctx *ctx,
             grn_tokenizer_query *query,
             grn_token *token,
             void *user_data)
{
  grn_pattern_tokenizer *tokenizer = user_data;

  if (tokenizer->have_tokenized_delimiter) {
    unsigned int rest_length;
    rest_length = tokenizer->end - tokenizer->next;
    tokenizer->next =
      (unsigned char *)grn_tokenizer_next_by_tokenized_delimiter(
        ctx,
        token,
        (const char *)tokenizer->next,
        rest_length,
        tokenizer->encoding);
#ifdef GRN_SUPPORT_REGEXP
  } else if (tokenizer->options->regex) {
    grn_token_status status = GRN_TOKEN_CONTINUE;
    if (tokenizer->next == tokenizer->start) {
      pattern_search(ctx, tokenizer);
    }
    grn_token_set_data(ctx,
                       token,
                       tokenizer->current,
                       tokenizer->current_length);
    if (tokenizer->next != tokenizer->end) {
      pattern_search(ctx, tokenizer);
    }
    if (tokenizer->next == tokenizer->end) {
      status = GRN_TOKEN_LAST;
    }
    grn_token_set_status(ctx, token, status);
#endif /* GRN_SUPPORT_REGEXP */
  } else {
    grn_token_set_data(ctx, token, NULL, 0);
    grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
  }
}

static void
pattern_fin(grn_ctx *ctx, void *user_data)
{
  grn_pattern_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }
  GRN_FREE(tokenizer);
}

/* table tokenizer */

typedef struct {
  grn_obj *table;
} grn_table_options;

typedef struct {
  grn_tokenizer_token token;
  grn_tokenizer_query *query;
  grn_table_options *options;
  grn_bool have_tokenized_delimiter;
  grn_encoding encoding;
  const unsigned char *start;
  const unsigned char *current;
  const unsigned char *next;
  const unsigned char *end;
  grn_pat_scan_hit hits[1024];
  int n_hits;
  int current_hit;
} grn_table_tokenizer;

static void
table_options_init(grn_table_options *options)
{
  options->table = NULL;
}

static void *
table_open_options(grn_ctx *ctx,
                   grn_obj *tokenizer,
                   grn_obj *raw_options,
                   void *user_data)
{
  grn_table_options *options;

  options = GRN_MALLOC(sizeof(grn_table_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][table] "
        "failed to allocate memory for options");
    return NULL;
  }

  table_options_init(options);
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "table")) {
      const char *name;
      unsigned int name_length;
      grn_id domain;

      name_length = grn_vector_get_element(ctx,
                                           raw_options,
                                           i,
                                           &name,
                                           NULL,
                                           &domain);
      if (grn_type_id_is_text_family(ctx, domain) && name_length > 0) {
        options->table = grn_ctx_get(ctx, name, name_length);
        if (!options->table) {
          ERR(GRN_INVALID_ARGUMENT,
              "[tokenizer][table] nonexistent table: <%.*s>",
              (int)name_length, name);
          break;
        }
        if (options->table->header.type != GRN_TABLE_PAT_KEY) {
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          grn_inspect(ctx, &inspected, options->table);
          ERR(GRN_INVALID_ARGUMENT,
              "[tokenizer][table] table must be a patricia trie table: "
              "<%.*s>: <%.*s>",
              (int)name_length,
              name,
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
          GRN_OBJ_FIN(ctx, &inspected);
          break;
        }
      }
    }
  } GRN_OPTION_VALUES_EACH_END();

  if (ctx->rc == GRN_SUCCESS && !options->table) {
    ERR(GRN_INVALID_ARGUMENT,
        "[tokenizer][table] table isn't specified");
  }

  return options;
}

static void
table_close_options(grn_ctx *ctx, void *data)
{
  grn_table_options *options = data;
  GRN_FREE(options);
}

static void *
table_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_table_options *options;
  grn_table_tokenizer *tokenizer;

  options = grn_table_cache_default_tokenizer_options(ctx,
                                                      lexicon,
                                                      table_open_options,
                                                      table_close_options,
                                                      NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  if (!(tokenizer = GRN_MALLOC(sizeof(grn_table_tokenizer)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][table] "
        "memory allocation to grn_table_tokenizer failed");
    return NULL;
  }

  tokenizer->query = query;
  tokenizer->options = options;

  {
    const char *raw_string;
    size_t raw_string_length;
    grn_encoding encoding;

    raw_string = grn_tokenizer_query_get_raw_string(ctx,
                                                    tokenizer->query,
                                                    &raw_string_length);
    encoding = grn_tokenizer_query_get_encoding(ctx, tokenizer->query);
    tokenizer->have_tokenized_delimiter =
      grn_tokenizer_have_tokenized_delimiter(ctx,
                                             raw_string,
                                             raw_string_length,
                                             encoding);
    tokenizer->encoding = encoding;
  }
  {
    grn_obj *string;
    const char *normalized;
    unsigned int normalized_length_in_bytes;

    string = grn_tokenizer_query_get_normalized_string(ctx, tokenizer->query);
    grn_string_get_normalized(ctx,
                              string,
                              &normalized, &normalized_length_in_bytes,
                              NULL);
    tokenizer->start = (const unsigned char *)normalized;
    tokenizer->next = tokenizer->start;
    tokenizer->current = tokenizer->start;
    tokenizer->end = tokenizer->start + normalized_length_in_bytes;
  }

  tokenizer->n_hits = 0;
  tokenizer->current_hit = -1;

  return tokenizer;
}

static void
table_scan(grn_ctx *ctx,
           grn_table_tokenizer *tokenizer)
{
  const char *rest;
  tokenizer->n_hits = grn_pat_scan(ctx,
                                   (grn_pat *)(tokenizer->options->table),
                                   tokenizer->next,
                                   tokenizer->end - tokenizer->next,
                                   tokenizer->hits,
                                   sizeof(tokenizer->hits) /
                                   sizeof(*(tokenizer->hits)),
                                   &rest);
  tokenizer->current = tokenizer->next;
  tokenizer->next = rest;
  tokenizer->current_hit = 0;
}

static void
table_next(grn_ctx *ctx,
           grn_tokenizer_query *query,
           grn_token *token,
           void *user_data)
{
  grn_table_tokenizer *tokenizer = user_data;

  if (tokenizer->have_tokenized_delimiter) {
    unsigned int rest_length;
    rest_length = tokenizer->end - tokenizer->next;
    tokenizer->next =
      (unsigned char *)grn_tokenizer_next_by_tokenized_delimiter(
        ctx,
        token,
        (const char *)tokenizer->next,
        rest_length,
        tokenizer->encoding);
  } else {
    if (tokenizer->current_hit == -1) {
      table_scan(ctx, tokenizer);
    }
    if (tokenizer->current_hit < tokenizer->n_hits) {
      grn_pat_scan_hit *hit = &(tokenizer->hits[tokenizer->current_hit]);
      grn_token_set_data(ctx,
                         token,
                         tokenizer->current + hit->offset,
                         hit->length);
      tokenizer->current_hit++;
      if (tokenizer->current_hit == tokenizer->n_hits) {
        grn_token_status status = GRN_TOKEN_CONTINUE;
        tokenizer->current_hit = -1;
        if (tokenizer->next != tokenizer->end) {
          table_scan(ctx, tokenizer);
        }
        if (tokenizer->next == tokenizer->end) {
          status = GRN_TOKEN_LAST;
        }
        grn_token_set_status(ctx, token, status);
      } else {
        grn_token_set_status(ctx, token, GRN_TOKEN_CONTINUE);
      }
    } else {
      grn_token_set_data(ctx, token, NULL, 0);
      grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
    }
  }
}

static void
table_fin(grn_ctx *ctx, void *user_data)
{
  grn_table_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }
  GRN_FREE(tokenizer);
}

/* Common code for IDF based document vector tokenizers */

typedef struct {
  grn_obj *index_column;
  grn_obj *df_column;
  bool normalize;
  float k1;
  float b;
} grn_document_vector_idf_base_options;

typedef struct {
  grn_tokenizer_token token;
  grn_document_vector_idf_base_options *options;
  grn_obj token_ids;
  grn_obj normalized_token_ids;
  size_t n_token_ids;
  size_t token_id_offset;
  grn_id current_token_id;
} grn_document_vector_idf_base_tokenizer;

typedef enum {
  DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_TF_IDF,
  DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25,
} grn_document_vector_idf_base_algorithm;

typedef struct {
  grn_document_vector_idf_base_algorithm algorithm;
  grn_tokenizer_query *query;
  grn_obj *lexicon;
  grn_obj *token_table;
  uint32_t n_documents;
  float average_dl;
  grn_hash *token_histogram;
  uint32_t dl;
} grn_document_vector_idf_base_metadata;

static void
document_vector_idf_base_init_tag(grn_ctx *ctx,
                                  grn_obj *tag,
                                  const char *name,
                                  grn_tokenizer_query *query)
{
  GRN_TEXT_INIT(tag, 0);
  grn_text_printf(ctx, tag, "[tokenizer][%s]", name);
  grn_obj *index_column = grn_tokenizer_query_get_index_column(ctx, query);
  if (index_column) {
    GRN_TEXT_PUTC(ctx, tag, '[');
    grn_inspect_name(ctx, tag, index_column);
    GRN_TEXT_PUTC(ctx, tag, ']');
  }
  GRN_TEXT_PUTC(ctx, tag, '\0');
}

static void
document_vector_idf_base_options_init(grn_ctx *ctx,
                                      grn_document_vector_idf_base_options *options)
{
  options->index_column = NULL;
  options->df_column = NULL;
  options->normalize = true;
  options->k1 = 2.0;
  options->b = 0.75;
}

static void
document_vector_idf_base_options_fin(
  grn_ctx *ctx,
  grn_document_vector_idf_base_options *options)
{
  if (ctx->impl->db && ((grn_db *)(ctx->impl->db))->is_closing) {
    return;
  }
  grn_obj_unref(ctx, options->index_column);
  grn_obj_unref(ctx, options->df_column);
}

static void
document_vector_idf_base_options_parse(
  grn_ctx *ctx,
  grn_document_vector_idf_base_options *options,
  grn_obj *raw_options,
  grn_obj *lexicon,
  grn_document_vector_idf_base_algorithm algorithm,
  const char *tag)
{
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "index_column")) {
      const char *name;
      unsigned int name_length;
      grn_id domain;

      name_length = grn_vector_get_element(ctx,
                                           raw_options,
                                           i,
                                           &name,
                                           NULL,
                                           &domain);
      if (grn_type_id_is_text_family(ctx, domain) && name_length > 0) {
        grn_obj *source_lexicon = grn_ctx_at(ctx, lexicon->header.domain);
        char source_lexicon_name[GRN_TABLE_MAX_KEY_SIZE];
        int source_lexicon_name_length =
          grn_obj_name(ctx,
                       source_lexicon,
                       source_lexicon_name,
                       GRN_TABLE_MAX_KEY_SIZE);
        if (!grn_obj_is_table_with_key(ctx, source_lexicon)) {
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          grn_inspect(ctx, &inspected, source_lexicon);
          ERR(GRN_INVALID_ARGUMENT,
              "%s table's key must be a table with key: <%.*s>",
              tag,
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
          GRN_OBJ_FIN(ctx, &inspected);
          grn_obj_unref(ctx, source_lexicon);
          break;
        }
        options->index_column = grn_obj_column(ctx,
                                               source_lexicon,
                                               name,
                                               name_length);
        grn_obj_unref(ctx, source_lexicon);
        if (!options->index_column) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s[index_column] nonexistent index column: <%.*s.%.*s>",
              tag,
              source_lexicon_name_length, source_lexicon_name,
              (int)name_length, name);
          break;
        }
        if (!grn_obj_is_index_column(ctx, options->index_column)) {
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          grn_inspect(ctx, &inspected, options->index_column);
          ERR(GRN_INVALID_ARGUMENT,
              "%s[index_column] must be an index column: <%.*s.%.*s>: <%.*s>",
              tag,
              source_lexicon_name_length,
              source_lexicon_name,
              (int)name_length,
              name,
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
          break;
        }
      }
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "df_column")) {
      const char *name;
      unsigned int name_length;
      grn_id domain;

      name_length = grn_vector_get_element(ctx,
                                           raw_options,
                                           i,
                                           &name,
                                           NULL,
                                           &domain);
      if (grn_type_id_is_text_family(ctx, domain) && name_length > 0) {
        options->df_column = grn_obj_column(ctx,
                                            lexicon,
                                            name,
                                            name_length);
        if (!options->df_column) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s[df_column] nonexistent document frequency column: <%.*s>",
              tag,
              (int)name_length, name);
          break;
        }
        if (!(grn_obj_is_scalar_column(ctx, options->df_column) &&
              options->df_column->header.domain != GRN_DB_UINT32)) {
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          grn_inspect(ctx, &inspected, options->df_column);
          ERR(GRN_INVALID_ARGUMENT,
              "%s[df_column] must be an UInt32 scalar column: <%.*s>: <%.*s>",
              tag,
              (int)name_length,
              name,
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
          GRN_OBJ_FIN(ctx, &inspected);
          break;
        }
      }
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "normalize")) {
      options->normalize = grn_vector_get_element_bool(ctx,
                                                       raw_options,
                                                       i,
                                                       options->normalize);
    } else {
      if (algorithm == DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25) {
        if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "k1")) {
          options->k1 = grn_vector_get_element_float32(ctx,
                                                       raw_options,
                                                       i,
                                                       options->k1);
        } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "b")) {
          options->b = grn_vector_get_element_float32(ctx,
                                                      raw_options,
                                                      i,
                                                      options->b);
        }
      }
    }
  } GRN_OPTION_VALUES_EACH_END();

  if (ctx->rc == GRN_SUCCESS) {
    if (!options->index_column) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[index_column] missing",
          tag);
    } else if (!options->df_column) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[df_column] missing",
          tag);
    }
  }
}

static void
document_vector_idf_base_tokenizer_init(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_document_vector_idf_base_options *options)
{
  tokenizer->options = options;

  GRN_RECORD_INIT(&(tokenizer->token_ids),
                  GRN_OBJ_VECTOR,
                  tokenizer->options->index_column->header.domain);
  tokenizer->token_ids.header.flags |= GRN_OBJ_WITH_WEIGHT;

  GRN_RECORD_INIT(&(tokenizer->normalized_token_ids),
                  GRN_OBJ_VECTOR,
                  tokenizer->options->index_column->header.domain);
  tokenizer->normalized_token_ids.header.flags |= GRN_OBJ_WITH_WEIGHT;

  tokenizer->n_token_ids = 0;
  tokenizer->token_id_offset = 0;
  tokenizer->current_token_id = GRN_ID_NIL;
}

static void
document_vector_idf_base_tokenizer_fin(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer)
{
  GRN_OBJ_FIN(ctx, &(tokenizer->token_ids));
  GRN_OBJ_FIN(ctx, &(tokenizer->normalized_token_ids));
}

static void
document_vector_idf_base_tokenizer_init_metadata(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_document_vector_idf_base_metadata *metadata)
{
  const char *metadata_option_name = "metadata";

  metadata->token_table =
    grn_ctx_at(ctx, tokenizer->options->index_column->header.domain);
  metadata->token_histogram = NULL;
  metadata->dl = 0;

  if (grn_table_size(ctx, metadata->lexicon) > 0) {
    grn_obj metadata_vector;
    GRN_VOID_INIT(&metadata_vector);
    grn_obj_get_option_values(ctx,
                              metadata->lexicon,
                              metadata_option_name,
                              -1,
                              GRN_OPTION_REVISION_NONE,
                              &metadata_vector);
    size_t n_metadata = grn_vector_size(ctx, &metadata_vector);

    metadata->n_documents = 0;
    if (n_metadata > 0) {
      metadata->n_documents = grn_vector_get_element_uint32(ctx,
                                                            &metadata_vector,
                                                            0,
                                                            0);
    }

    if (metadata->algorithm == DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25) {
      metadata->average_dl = 0;
      if (n_metadata > 1) {
        metadata->average_dl = grn_vector_get_element_float32(ctx,
                                                              &metadata_vector,
                                                              1,
                                                              0);
      }
    }

    GRN_OBJ_FIN(ctx, &metadata_vector);

    return;
  }

  grn_ii *ii = (grn_ii *)(tokenizer->options->index_column);
  uint64_t total_tf = 0;
  grn_obj *lexicon = metadata->lexicon;
  grn_obj *df_column = tokenizer->options->df_column;
  grn_obj df_value;
  GRN_UINT32_INIT(&df_value, 0);
  GRN_TABLE_EACH_BEGIN(ctx, metadata->token_table, cursor, source_id) {
    grn_id id = grn_table_add(ctx,
                              lexicon,
                              (const char *)&source_id,
                              sizeof(grn_id),
                              NULL);
    uint32_t df = 0;
    grn_ii_cursor *ii_cursor = grn_ii_cursor_open(ctx,
                                                  ii,
                                                  source_id,
                                                  GRN_ID_NIL,
                                                  GRN_ID_MAX,
                                                  ii->n_elements,
                                                  0);
    if (ii_cursor) {
      while (grn_ii_cursor_next(ctx, ii_cursor)) {
        df++;
        if (metadata->algorithm == DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25) {
          while (grn_ii_cursor_next_pos(ctx, ii_cursor)) {
            total_tf++;
          }
        }
      }
      grn_ii_cursor_close(ctx, ii_cursor);
    }
    GRN_UINT32_SET(ctx, &df_value, df);
    grn_obj_set_value(ctx, df_column, id, &df_value, GRN_OBJ_SET);
  } GRN_TABLE_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &df_value);

  {
    const grn_id source_id =
      grn_obj_get_range(ctx, tokenizer->options->index_column);
    grn_obj *source = grn_ctx_at(ctx, source_id);
    metadata->n_documents = grn_table_size(ctx, source);
    grn_obj_unref(ctx, source);

    grn_obj metadata_vector;
    GRN_VOID_INIT(&metadata_vector);
    grn_obj_ensure_vector(ctx, &metadata_vector);
    grn_vector_add_element(ctx,
                           &metadata_vector,
                           (const char *)&(metadata->n_documents),
                           sizeof(uint32_t),
                           0,
                           GRN_DB_UINT32);
    if (metadata->algorithm == DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25) {
      metadata->average_dl = 0.0;
      if (metadata->n_documents > 0) {
        metadata->average_dl = total_tf / (float)(metadata->n_documents);
      }
      grn_vector_add_element(ctx,
                             &metadata_vector,
                             (const char *)&(metadata->average_dl),
                             sizeof(float),
                             0,
                             GRN_DB_FLOAT32);
    }
    grn_obj_set_option_values(ctx,
                              lexicon,
                              metadata_option_name,
                              -1,
                              &metadata_vector);
    GRN_OBJ_FIN(ctx, &metadata_vector);
  }
}

static void
document_vector_idf_base_tokenizer_fin_metadata(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_document_vector_idf_base_metadata *metadata)
{
  if (metadata->token_histogram) {
    grn_hash_close(ctx, metadata->token_histogram);
  }
  grn_obj_unref(ctx, metadata->token_table);
}

static bool
document_vector_idf_base_tokenizer_init_token_ids_token_column(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_document_vector_idf_base_metadata *metadata,
  const char *tag)
{
  grn_obj *source_column =
    grn_tokenizer_query_get_source_column(ctx, metadata->query);
  if (!source_column) {
    return false;
  }
  grn_id source_id =
    grn_tokenizer_query_get_source_id(ctx, metadata->query);
  if (source_id == GRN_ID_NIL) {
    return false;
  }
  grn_obj *index_column =
    grn_tokenizer_query_get_index_column(ctx, metadata->query);
  if (!index_column) {
    return false;
  }

  grn_obj all_hooked_columns;
  GRN_PTR_INIT(&all_hooked_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_column_get_all_hooked_columns(ctx, source_column, &all_hooked_columns);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_OBJ_FIN(ctx, &all_hooked_columns);
    return false;
  }

  grn_obj *token_column = NULL;
  size_t n_hooked_columns = GRN_PTR_VECTOR_SIZE(&all_hooked_columns);
  size_t i;
  bool collected = false;
  for (i = 0; i < n_hooked_columns; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(&all_hooked_columns, i);
    if (column == index_column) {
      break;
    }
    if (grn_obj_is_token_column(ctx, column) &&
        (grn_obj_get_range(ctx, column) ==
         tokenizer->options->index_column->header.domain)) {
      token_column = column;
      break;
    }
  }

  if (!token_column) {
    goto exit;
  }

  if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
    grn_obj message;
    GRN_TEXT_INIT(&message, 0);
    GRN_TEXT_PUTC(ctx, &message, '<');
    grn_inspect_name(ctx, &message, token_column);
    GRN_TEXT_PUTC(ctx, &message, '>');
    GRN_LOG(ctx, GRN_LOG_DEBUG,
            "%s[tokenize][token-column] %.*s",
            tag,
            (int)GRN_TEXT_LEN(&message),
            GRN_TEXT_VALUE(&message));
    GRN_OBJ_FIN(ctx, &message);
  }
  grn_obj tokens;
  GRN_RECORD_INIT(&tokens,
                  GRN_OBJ_VECTOR,
                  grn_obj_get_range(ctx, token_column));
  grn_obj_get_value(ctx, token_column, source_id, &tokens);
  metadata->dl = 0;
  size_t j;
  size_t n_tokens = GRN_RECORD_VECTOR_SIZE(&tokens);
  for (j = 0; j < n_tokens; j++) {
    grn_id token_id = GRN_RECORD_VALUE_AT(&tokens, j);
    if (token_id == GRN_ID_NIL) {
      continue;
    }

    metadata->dl++;

    void *value;
    grn_id id = grn_hash_add(ctx,
                             metadata->token_histogram,
                             &token_id,
                             sizeof(grn_id),
                             &value,
                             NULL);
    if (id == GRN_ID_NIL) {
      continue;
    }
    uint32_t *tf = value;
    *tf += 1;
  }
  collected = true;

exit :
  for (i = 0; i < n_hooked_columns; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(&all_hooked_columns, i);
    grn_obj_unref(ctx, column);
  }
  GRN_OBJ_FIN(ctx, &all_hooked_columns);

  return collected;
}

static bool
document_vector_idf_base_tokenizer_init_token_ids_token_cursor(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_document_vector_idf_base_metadata *metadata,
  const char *tag)
{
  const char *raw_string;
  size_t raw_string_length;
  raw_string = grn_tokenizer_query_get_raw_string(ctx,
                                                  metadata->query,
                                                  &raw_string_length);
  grn_token_cursor *token_cursor =
    grn_token_cursor_open(ctx,
                          metadata->token_table,
                          raw_string,
                          raw_string_length,
                          GRN_TOKENIZE_ADD,
                          0);
  if (!token_cursor) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to create token cursor",
        tag);
    return false;
  }

  metadata->dl = 0;
  while (grn_token_cursor_get_status(ctx, token_cursor) ==
         GRN_TOKEN_CURSOR_DOING) {
    grn_id token_id = grn_token_cursor_next(ctx, token_cursor);
    if (token_id == GRN_ID_NIL) {
      continue;
    }

    metadata->dl++;

    void *value;
    grn_id id = grn_hash_add(ctx,
                             metadata->token_histogram,
                             &token_id,
                             sizeof(grn_id),
                             &value,
                             NULL);
    if (id == GRN_ID_NIL) {
      continue;
    }
    uint32_t *tf = value;
    *tf += 1;
  }
  grn_token_cursor_close(ctx, token_cursor);

  return true;
}

static void
document_vector_idf_base_tokenizer_init_token_ids(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_document_vector_idf_base_metadata *metadata,
  const char *tag)
{
  metadata->token_histogram =
    grn_hash_create(ctx,
                    NULL,
                    sizeof(grn_id),
                    sizeof(uint32_t),
                    GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY);
  if (!metadata->token_histogram) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to create token histogram",
        tag);
    return;
  }

  if (!document_vector_idf_base_tokenizer_init_token_ids_token_column(
        ctx,
        tokenizer,
        metadata,
        tag)) {
    if (!document_vector_idf_base_tokenizer_init_token_ids_token_cursor(
          ctx,
          tokenizer,
          metadata,
          tag)) {
      return;
    }
  }

  {
    grn_obj *lexicon = metadata->lexicon;
    grn_obj *df_column = tokenizer->options->df_column;
    grn_obj df_value;
    GRN_UINT32_INIT(&df_value, 0);
    GRN_HASH_EACH_BEGIN(ctx,
                        metadata->token_histogram,
                        cursor,
                        token_histogram_id) {
      void *key;
      unsigned int key_size;
      void *value;
      grn_hash_cursor_get_key_value(ctx, cursor, &key, &key_size, &value);
      const grn_id token_id = *((grn_id *)key);
      const uint32_t tf = *((uint32_t *)value);
      const grn_id id = grn_table_get(ctx, lexicon, &token_id, sizeof(grn_id));
      GRN_BULK_REWIND(&df_value);
      grn_obj_get_value(ctx, df_column, id, &df_value);
      uint32_t df = 0;
      if (GRN_BULK_VSIZE(&df_value) > 0) {
        df = GRN_UINT32_VALUE(&df_value);
      }
      if (df == 0) {
        continue;
      }
      float weight;
      if (metadata->algorithm == DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_TF_IDF) {
        /* Use the same formula as gensim:
         * https://radimrehurek.com/gensim/models/tfidfmodel.html */
        const float tf_idf = tf * log2f(metadata->n_documents / (float)df);
        weight = tf_idf;
      } else {
        /* Use the formula in Wikipedia:
         * https://en.wikipedia.org/wiki/Okapi_BM25 */
        const float idf =
          logf(((metadata->n_documents - df + 0.5) / (df + 0.5)) + 1);
        const float k1 = tokenizer->options->k1;
        const float b = tokenizer->options->b;
        const float bm25 =
          idf * ((tf * (k1 + 1)) /
                 (tf + k1 *
                  (1 - b + b * (metadata->dl / metadata->average_dl))));
        weight = bm25;
      }
      grn_uvector_add_element_record(ctx,
                                     &(tokenizer->token_ids),
                                     id,
                                     weight);
    } GRN_HASH_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &df_value);
  }

  tokenizer->n_token_ids = grn_uvector_size(ctx, &(tokenizer->token_ids));

  if (tokenizer->options->normalize && tokenizer->n_token_ids > 0) {
    float l2_norm = 0.0;
    size_t i;
    for (i = 0; i < tokenizer->n_token_ids; i++) {
      float weight = 0.0;
      grn_uvector_get_element_record(ctx,
                                     &(tokenizer->token_ids),
                                     i,
                                     &weight);
      l2_norm += weight * weight;
    }
    l2_norm = sqrtf(l2_norm);
    for (i = 0; i < tokenizer->n_token_ids; i++) {
      float weight = 0.0;
      grn_id id = grn_uvector_get_element_record(ctx,
                                                 &(tokenizer->token_ids),
                                                 i,
                                                 &weight);
      grn_uvector_add_element_record(ctx,
                                     &(tokenizer->normalized_token_ids),
                                     id,
                                     weight / l2_norm);
    }
  }
}

static void
document_vector_idf_base_tokenizer_next(
  grn_ctx *ctx,
  grn_document_vector_idf_base_tokenizer *tokenizer,
  grn_token *token)
{
  if (tokenizer->n_token_ids == 0) {
    grn_token_set_data(ctx, token, NULL, 0);
    grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
    return;
  }

  grn_obj *token_ids;
  if (tokenizer->options->normalize) {
    token_ids = &(tokenizer->normalized_token_ids);
  } else {
    token_ids = &(tokenizer->token_ids);
  }
  float weight = 0.0;
  tokenizer->current_token_id =
    grn_uvector_get_element_record(ctx,
                                   token_ids,
                                   tokenizer->token_id_offset,
                                   &weight);
  grn_token_set_data(ctx,
                     token,
                     (const char *)(&(tokenizer->current_token_id)),
                     sizeof(grn_id));
  grn_token_set_weight(ctx, token, weight);

  if (tokenizer->token_id_offset == tokenizer->n_token_ids - 1) {
    grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
  } else {
    grn_token_set_status(ctx, token, GRN_TOKEN_CONTINUE);
  }

  tokenizer->token_id_offset++;
}

/* document vector TF/IDF tokenizer */

static void
document_vector_tf_idf_close_options(grn_ctx *ctx, void *data)
{
  grn_document_vector_idf_base_options *options = data;
  document_vector_idf_base_options_fin(ctx, options);
  GRN_FREE(options);
}

static void *
document_vector_tf_idf_open_options(grn_ctx *ctx,
                                    grn_obj *tokenizer,
                                    grn_obj *raw_options,
                                    void *user_data)
{
  grn_tokenizer_query *query = user_data;
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_obj tag;
  document_vector_idf_base_init_tag(ctx,
                                    &tag,
                                    "document-vector-tf-idf",
                                    query);

  grn_document_vector_idf_base_options *options;
  options = GRN_MALLOC(sizeof(grn_document_vector_idf_base_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options",
        GRN_TEXT_VALUE(&tag));
    GRN_OBJ_FIN(ctx, &tag);
    return NULL;
  }

  document_vector_idf_base_options_init(ctx, options);
  document_vector_idf_base_options_parse(
    ctx,
    options,
    raw_options,
    lexicon,
    DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_TF_IDF,
    GRN_TEXT_VALUE(&tag));
  GRN_OBJ_FIN(ctx, &tag);

  if (ctx->rc != GRN_SUCCESS) {
    document_vector_tf_idf_close_options(ctx, options);
    options = NULL;
  }

  return options;
}

static void
document_vector_tf_idf_fin(grn_ctx *ctx, void *user_data)
{
  grn_document_vector_idf_base_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }

  document_vector_idf_base_tokenizer_fin(ctx, tokenizer);
  GRN_FREE(tokenizer);
}

static void *
document_vector_tf_idf_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_document_vector_idf_base_options *options =
    grn_table_cache_default_tokenizer_options(
      ctx,
      lexicon,
      document_vector_tf_idf_open_options,
      document_vector_tf_idf_close_options,
      query);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  grn_obj tag;
  document_vector_idf_base_init_tag(ctx,
                                    &tag,
                                    "document-vector-tf-idf",
                                    query);
  grn_document_vector_idf_base_tokenizer *tokenizer =
    GRN_MALLOC(sizeof(grn_document_vector_idf_base_tokenizer));
  if (!tokenizer) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate tokenizer",
        GRN_TEXT_VALUE(&tag));
    GRN_OBJ_FIN(ctx, &tag);
    return NULL;
  }

  document_vector_idf_base_tokenizer_init(ctx, tokenizer, options);

  grn_document_vector_idf_base_metadata metadata;
  metadata.algorithm = DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_TF_IDF;
  metadata.query = query;
  metadata.lexicon = lexicon;
  document_vector_idf_base_tokenizer_init_metadata(ctx, tokenizer, &metadata);
  if (ctx->rc != GRN_SUCCESS) {
    document_vector_idf_base_tokenizer_fin_metadata(ctx, tokenizer, &metadata);
    document_vector_tf_idf_fin(ctx, tokenizer);
    GRN_OBJ_FIN(ctx, &tag);
    return NULL;
  }

  if (metadata.n_documents == 0) {
    document_vector_idf_base_tokenizer_fin_metadata(ctx, tokenizer, &metadata);
    GRN_OBJ_FIN(ctx, &tag);
    return tokenizer;
  }

  document_vector_idf_base_tokenizer_init_token_ids(ctx,
                                                    tokenizer,
                                                    &metadata,
                                                    GRN_TEXT_VALUE(&tag));
  GRN_OBJ_FIN(ctx, &tag);

  document_vector_idf_base_tokenizer_fin_metadata(ctx, tokenizer, &metadata);
  if (ctx->rc != GRN_SUCCESS) {
    document_vector_tf_idf_fin(ctx, tokenizer);
    return NULL;
  }

  return tokenizer;
}

static void
document_vector_tf_idf_next(grn_ctx *ctx,
                            grn_tokenizer_query *query,
                            grn_token *token,
                            void *user_data)
{
  grn_document_vector_idf_base_tokenizer *tokenizer = user_data;
  document_vector_idf_base_tokenizer_next(ctx, tokenizer, token);
}

/* document vector BM25 tokenizer */

static void
document_vector_bm25_close_options(grn_ctx *ctx, void *data)
{
  grn_document_vector_idf_base_options *options = data;
  document_vector_idf_base_options_fin(ctx, options);
  GRN_FREE(options);
}

static void *
document_vector_bm25_open_options(grn_ctx *ctx,
                                  grn_obj *tokenizer,
                                  grn_obj *raw_options,
                                  void *user_data)
{
  grn_tokenizer_query *query = user_data;
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_obj tag;
  document_vector_idf_base_init_tag(ctx,
                                    &tag,
                                    "document-vector-bm25",
                                    query);

  grn_document_vector_idf_base_options *options =
    GRN_MALLOC(sizeof(grn_document_vector_idf_base_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options",
        GRN_TEXT_VALUE(&tag));
    GRN_OBJ_FIN(ctx, &tag);
    return NULL;
  }

  document_vector_idf_base_options_init(ctx, options);
  document_vector_idf_base_options_parse(
    ctx,
    options,
    raw_options,
    lexicon,
    DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25,
    GRN_TEXT_VALUE(&tag));
  GRN_OBJ_FIN(ctx, &tag);


  if (ctx->rc != GRN_SUCCESS) {
    document_vector_tf_idf_close_options(ctx, options);
    options = NULL;
  }

  return options;
}

static void
document_vector_bm25_fin(grn_ctx *ctx, void *user_data)
{
  grn_document_vector_idf_base_tokenizer *tokenizer = user_data;

  if (!tokenizer) {
    return;
  }

  document_vector_idf_base_tokenizer_fin(ctx, tokenizer);
  GRN_FREE(tokenizer);
}

static void *
document_vector_bm25_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_obj *lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
  grn_document_vector_idf_base_options *options =
    grn_table_cache_default_tokenizer_options(
      ctx,
      lexicon,
      document_vector_bm25_open_options,
      document_vector_bm25_close_options,
      query);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  grn_obj tag;
  document_vector_idf_base_init_tag(ctx,
                                    &tag,
                                    "document-vector-bm25",
                                    query);
  grn_document_vector_idf_base_tokenizer *tokenizer =
    GRN_MALLOC(sizeof(grn_document_vector_idf_base_tokenizer));
  if (!tokenizer) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate tokenizer",
        GRN_TEXT_VALUE(&tag));
    GRN_OBJ_FIN(ctx, &tag);
    return NULL;
  }

  document_vector_idf_base_tokenizer_init(ctx, tokenizer, options);

  grn_document_vector_idf_base_metadata metadata;
  metadata.algorithm = DOCUMENT_VECTOR_IDF_BASE_ALGORITHM_BM25;
  metadata.query = query;
  metadata.lexicon = lexicon;
  document_vector_idf_base_tokenizer_init_metadata(ctx, tokenizer, &metadata);
  if (ctx->rc != GRN_SUCCESS) {
    document_vector_idf_base_tokenizer_fin_metadata(ctx, tokenizer, &metadata);
    document_vector_bm25_fin(ctx, tokenizer);
    GRN_OBJ_FIN(ctx, &tag);
    return NULL;
  }

  if (metadata.n_documents == 0) {
    document_vector_idf_base_tokenizer_fin_metadata(ctx, tokenizer, &metadata);
    GRN_OBJ_FIN(ctx, &tag);
    return tokenizer;
  }

  document_vector_idf_base_tokenizer_init_token_ids(ctx,
                                                    tokenizer,
                                                    &metadata,
                                                    GRN_TEXT_VALUE(&tag));
  GRN_OBJ_FIN(ctx, &tag);

  document_vector_idf_base_tokenizer_fin_metadata(ctx, tokenizer, &metadata);
  if (ctx->rc != GRN_SUCCESS) {
    document_vector_bm25_fin(ctx, tokenizer);
    return NULL;
  }

  return tokenizer;
}

static void
document_vector_bm25_next(grn_ctx *ctx,
                          grn_tokenizer_query *query,
                          grn_token *token,
                          void *user_data)
{
  grn_document_vector_idf_base_tokenizer *tokenizer = user_data;
  document_vector_idf_base_tokenizer_next(ctx, tokenizer, token);
}

/* external */

grn_rc
grn_tokenizers_init(void)
{
  static grn_proc _grn_tokenizer_uvector;
  _grn_tokenizer_uvector.obj.db = NULL;
  _grn_tokenizer_uvector.obj.id = GRN_ID_NIL;
  _grn_tokenizer_uvector.obj.header.domain = GRN_ID_NIL;
  _grn_tokenizer_uvector.obj.range = GRN_ID_NIL;
  _grn_tokenizer_uvector.funcs[PROC_INIT] = uvector_init;
  _grn_tokenizer_uvector.funcs[PROC_NEXT] = uvector_next;
  _grn_tokenizer_uvector.funcs[PROC_FIN] = uvector_fin;
  grn_tokenizer_uvector = (grn_obj *)&_grn_tokenizer_uvector;
  return GRN_SUCCESS;
}

grn_rc
grn_tokenizers_fin(void)
{
  return GRN_SUCCESS;
}

grn_rc
grn_db_init_mecab_tokenizer(grn_ctx *ctx)
{
  switch (GRN_CTX_GET_ENCODING(ctx)) {
  case GRN_ENC_EUC_JP :
  case GRN_ENC_UTF8 :
  case GRN_ENC_SJIS :
#if defined(GRN_EMBEDDED) && defined(GRN_WITH_MECAB)
    {
      GRN_PLUGIN_DECLARE_FUNCTIONS(tokenizers_mecab);
      grn_rc rc;
      rc = GRN_PLUGIN_IMPL_NAME_TAGGED(init, tokenizers_mecab)(ctx);
      if (rc == GRN_SUCCESS) {
        rc = GRN_PLUGIN_IMPL_NAME_TAGGED(register, tokenizers_mecab)(ctx);
        if (rc != GRN_SUCCESS) {
          GRN_PLUGIN_IMPL_NAME_TAGGED(fin, tokenizers_mecab)(ctx);
        }
      }
      return rc;
    }
#else /* defined(GRN_EMBEDDED) && defined(GRN_WITH_MECAB) */
    {
      const char *mecab_plugin_name = "tokenizers/mecab";
      char *path;
      path = grn_plugin_find_path(ctx, mecab_plugin_name);
      if (path) {
        GRN_FREE(path);
        return grn_plugin_register(ctx, mecab_plugin_name);
      } else {
        return GRN_NO_SUCH_FILE_OR_DIRECTORY;
      }
    }
#endif /* defined(GRN_EMBEDDED) && defined(GRN_WITH_MECAB) */
    break;
  default :
    return GRN_OPERATION_NOT_SUPPORTED;
  }
}

#define DEF_TOKENIZER(name, init, next, fin, vars)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1),\
                   GRN_PROC_TOKENIZER, (init), (next), (fin), 3, (vars)))

grn_rc
grn_db_init_builtin_tokenizers(grn_ctx *ctx)
{
  grn_obj *obj;
  grn_expr_var vars[3];

  vars[0].name = NULL;
  vars[0].name_size = 0;
  GRN_TEXT_INIT(&vars[0].value, 0);
  vars[1].name = NULL;
  vars[1].name_size = 0;
  GRN_TEXT_INIT(&vars[1].value, 0);
  vars[2].name = NULL;
  vars[2].name_size = 0;
  GRN_UINT32_INIT(&vars[2].value, 0);

  {
    char grn_ngram_tokenizer_remove_blank_enable_env[GRN_ENV_BUFFER_SIZE];

    grn_getenv("GRN_NGRAM_TOKENIZER_REMOVE_BLANK_ENABLE",
               grn_ngram_tokenizer_remove_blank_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ngram_tokenizer_remove_blank_enable_env[0]) {
      if (strcmp(grn_ngram_tokenizer_remove_blank_enable_env, "no") == 0) {
        grn_ngram_tokenizer_remove_blank_enable = GRN_FALSE;
      } else {
        grn_ngram_tokenizer_remove_blank_enable = GRN_TRUE;
      }
    } else {
      /* Deprecated. Use GRN_NGRAM_TOKENIZER_REMOVE_BLANK_ENABLE instead. */
      char grn_ngram_tokenizer_remove_blank_disable_env[GRN_ENV_BUFFER_SIZE];

      grn_getenv("GRN_NGRAM_TOKENIZER_REMOVE_BLANK_DISABLE",
                 grn_ngram_tokenizer_remove_blank_disable_env,
                 GRN_ENV_BUFFER_SIZE);
      if (grn_ngram_tokenizer_remove_blank_disable_env[0]) {
        grn_ngram_tokenizer_remove_blank_enable = GRN_FALSE;
      }
    }
  }

  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenDelimit", -1);
    if (!tokenizer || DB_OBJ(tokenizer)->id != GRN_DB_DELIMIT) {
      return GRN_FILE_CORRUPT;
    }
    grn_tokenizer_set_init_func(ctx, tokenizer, delimit_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, delimit_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, delimit_fin);
  }
  obj = DEF_TOKENIZER("TokenUnigram",
                      unigram_init,
                      ngram_next_deprecated,
                      ngram_fin_deprecated,
                      vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_UNIGRAM) { return GRN_FILE_CORRUPT; }
  obj = DEF_TOKENIZER("TokenBigram",
                      bigram_init,
                      ngram_next_deprecated,
                      ngram_fin_deprecated,
                      vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_BIGRAM) { return GRN_FILE_CORRUPT; }
  obj = DEF_TOKENIZER("TokenTrigram",
                      trigram_init,
                      ngram_next_deprecated,
                      ngram_fin_deprecated,
                      vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_TRIGRAM) { return GRN_FILE_CORRUPT; }

  DEF_TOKENIZER("TokenBigramSplitSymbol",
                bigrams_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  DEF_TOKENIZER("TokenBigramSplitSymbolAlpha",
                bigramsa_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  DEF_TOKENIZER("TokenBigramSplitSymbolAlphaDigit",
                bigramsad_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlank",
                bigrami_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlankSplitSymbol",
                bigramis_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlankSplitSymbolAlpha",
                bigramisa_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlankSplitSymbolAlphaDigit",
                bigramisad_init,
                ngram_next_deprecated,
                ngram_fin_deprecated,
                vars);
  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenDelimitNull", -1);
    grn_tokenizer_set_init_func(ctx, tokenizer, delimit_null_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, delimit_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, delimit_fin);
  }
  DEF_TOKENIZER("TokenRegexp",
                regexp_init, regexp_next, regexp_fin, vars);
  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenNgram", -1);
    grn_tokenizer_set_init_func(ctx, tokenizer, ngram_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, ngram_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, ngram_fin);
  }
  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenPattern", -1);
    grn_tokenizer_set_init_func(ctx, tokenizer, pattern_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, pattern_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, pattern_fin);
  }
  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenTable", -1);
    grn_tokenizer_set_init_func(ctx, tokenizer, table_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, table_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, table_fin);
  }
  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenDocumentVectorTFIDF", -1);
    grn_tokenizer_set_init_func(ctx, tokenizer, document_vector_tf_idf_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, document_vector_tf_idf_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, document_vector_tf_idf_fin);
  }
  {
    grn_obj *tokenizer;
    tokenizer = grn_tokenizer_create(ctx, "TokenDocumentVectorBM25", -1);
    grn_tokenizer_set_init_func(ctx, tokenizer, document_vector_bm25_init);
    grn_tokenizer_set_next_func(ctx, tokenizer, document_vector_bm25_next);
    grn_tokenizer_set_fin_func(ctx, tokenizer, document_vector_bm25_fin);
  }

  return GRN_SUCCESS;
}
