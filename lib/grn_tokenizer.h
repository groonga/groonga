/*
  Copyright (C) 2018  Brazil
  Copyright (C) 2018-2025  Sutou Kouhei <kou@clear-code.com>

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
  char *data;
  const char *ptr;
  uint32_t size;
  grn_encoding encoding;
  uint32_t flags;
  bool have_tokenized_delimiter;
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
  bool need_normalize;
  bool need_delimiter_check;

  grn_obj *options;

  grn_id domain;
} grn_tokenizer_query;

grn_rc
grn_tokenizer_query_init(grn_ctx *ctx, grn_tokenizer_query *query);
void
grn_tokenizer_query_fin(grn_ctx *ctx, grn_tokenizer_query *query);
grn_rc
grn_tokenizer_query_set_raw_string(grn_ctx *ctx,
                                   grn_tokenizer_query *query,
                                   const char *string,
                                   size_t string_length);
grn_rc
grn_tokenizer_query_set_data(grn_ctx *ctx,
                             grn_tokenizer_query *query,
                             const char *data,
                             size_t size,
                             grn_id domain);
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

#ifdef __cplusplus
extern "C" {
#endif

bool
grn_tokenizer_have_build_func(grn_ctx *ctx, grn_obj *tokenizer);

typedef grn_rc
grn_tokenizer_build_processed_n_records_func(grn_ctx *ctx,
                                             grn_tokenizer_build_data *data,
                                             uint32_t n_records,
                                             void *user_data);
typedef grn_rc
grn_tokenizer_build_start_vectorize_func(grn_ctx *ctx,
                                         grn_tokenizer_build_data *data,
                                         uint32_t n_total_records,
                                         void *user_data);
typedef grn_rc
grn_tokenizer_build_start_cluster_func(grn_ctx *ctx,
                                       grn_tokenizer_build_data *data,
                                       uint32_t n_total_records,
                                       void *user_data);
typedef grn_rc
grn_tokenizer_build_start_load_func(grn_ctx *ctx,
                                    grn_tokenizer_build_data *data,
                                    uint32_t n_total_records,
                                    void *user_data);
typedef grn_rc
grn_tokenizer_build_start_record_func(grn_ctx *ctx,
                                      grn_tokenizer_build_data *data,
                                      grn_id rid,
                                      void *user_data);
typedef grn_rc
grn_tokenizer_build_start_section_func(grn_ctx *ctx,
                                       grn_tokenizer_build_data *data,
                                       uint32_t sid,
                                       void *user_data);
typedef grn_rc
grn_tokenizer_build_append_tokens_func(grn_ctx *ctx,
                                       grn_tokenizer_build_data *data,
                                       grn_obj *tokens,
                                       void *user_data);
typedef grn_rc
grn_tokenizer_build_finish_section_func(grn_ctx *ctx,
                                        grn_tokenizer_build_data *data,
                                        void *user_data);
typedef grn_rc
grn_tokenizer_build_finish_record_func(grn_ctx *ctx,
                                       grn_tokenizer_build_data *data,
                                       void *user_data);

struct grn_tokenizer_build_data {
  grn_obj *source_table;
  grn_obj *source_columns;
  grn_obj *lexicon;
  grn_obj *index_column;
  grn_tokenizer_build_processed_n_records_func *processed_n_records_func;
  grn_tokenizer_build_start_vectorize_func *start_vectorize_func;
  grn_tokenizer_build_start_cluster_func *start_cluster_func;
  grn_tokenizer_build_start_load_func *start_load_func;
  grn_tokenizer_build_start_record_func *start_record_func;
  grn_tokenizer_build_start_section_func *start_section_func;
  grn_tokenizer_build_append_tokens_func *append_tokens_func;
  grn_tokenizer_build_finish_section_func *finish_section_func;
  grn_tokenizer_build_finish_record_func *finish_record_func;
  void *user_data;
};

grn_rc
grn_tokenizer_build(grn_ctx *ctx,
                    grn_obj *tokenizer,
                    grn_tokenizer_build_data *data);

#ifdef __cplusplus
}
#endif
