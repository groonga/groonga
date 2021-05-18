/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018  Brazil
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

#pragma once

#include "grn_ctx.h"

#define grn_tokenizer_query grn_tokenizer_query_deprecated
#include <groonga/tokenizer_query_deprecated.h>
#undef grn_tokenizer_query

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_tokenizer_query {
  /* Start _grn_tokenizer_query_deprecated compatible layout. */
  grn_obj *normalized_query;
  char *query_buf;
  const char *ptr;
  unsigned int length;
  grn_encoding encoding;
  uint32_t flags;
  grn_bool have_tokenized_delimiter;
  /* Deprecated since 4.0.8. Use tokenize_mode instead. */
  grn_token_mode token_mode;
  grn_tokenize_mode tokenize_mode;
  /* End _grn_tokenizer_query_deprecated compatible layout. */

  grn_obj *lexicon;
  unsigned int token_filter_index;
  grn_obj *source_column;
  grn_id source_id;
  grn_obj *index_column;
  uint32_t normalize_flags;
  grn_bool need_normalize;
  grn_bool need_delimiter_check;

  grn_obj *options;
} grn_tokenizer_query;

grn_rc
grn_tokenizer_query_init(grn_ctx *ctx,
                         grn_tokenizer_query *query);
void
grn_tokenizer_query_fin(grn_ctx *ctx,
                        grn_tokenizer_query *query);
grn_rc
grn_tokenizer_query_set_raw_string(grn_ctx *ctx,
                                   grn_tokenizer_query *query,
                                   const char *string,
                                   size_t string_length);
grn_rc
grn_tokenizer_query_set_flags(grn_ctx *ctx,
                              grn_tokenizer_query *query,
                              uint32_t flags);
grn_rc
grn_tokenizer_query_set_mode(grn_ctx *ctx,
                             grn_tokenizer_query *query,
                             grn_tokenize_mode mode);
grn_rc
grn_tokenizer_query_set_lexicon(grn_ctx *ctx,
                                grn_tokenizer_query *query,
                                grn_obj *lexicon);
grn_rc
grn_tokenizer_query_set_token_filter_index(grn_ctx *ctx,
                                           grn_tokenizer_query *query,
                                           unsigned int index);
grn_rc
grn_tokenizer_query_set_source_column(grn_ctx *ctx,
                                      grn_tokenizer_query *query,
                                      grn_obj *column);
grn_rc
grn_tokenizer_query_set_source_id(grn_ctx *ctx,
                                  grn_tokenizer_query *query,
                                  grn_id id);
grn_rc
grn_tokenizer_query_set_index_column(grn_ctx *ctx,
                                     grn_tokenizer_query *query,
                                     grn_obj *column);
grn_rc
grn_tokenizer_query_set_options(grn_ctx *ctx,
                                grn_tokenizer_query *query,
                                grn_obj *options);

#ifdef __cplusplus
}
#endif

#include <groonga/tokenizer.h>
