/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GRN_TOKEN_CURSOR_DOING = 0,
  GRN_TOKEN_CURSOR_DONE,
  GRN_TOKEN_CURSOR_DONE_SKIP,
  GRN_TOKEN_CURSOR_NOT_FOUND
} grn_token_cursor_status;

#define GRN_TOKEN_CURSOR_ENABLE_TOKENIZED_DELIMITER (0x01 << 0)
#define GRN_TOKEN_CURSOR_PARALLEL                   (0x01 << 1)

typedef struct _grn_token_cursor grn_token_cursor;

GRN_API grn_token_cursor *
grn_token_cursor_open(grn_ctx *ctx,
                      grn_obj *table,
                      const char *str,
                      size_t str_len,
                      grn_tokenize_mode mode,
                      uint32_t flags);

GRN_API grn_rc
grn_token_cursor_set_source_column(grn_ctx *ctx,
                                   grn_token_cursor *token_cursor,
                                   grn_obj *column);
GRN_API grn_rc
grn_token_cursor_set_source_id(grn_ctx *ctx,
                               grn_token_cursor *token_cursor,
                               grn_id id);
GRN_API grn_rc
grn_token_cursor_set_index_column(grn_ctx *ctx,
                                  grn_token_cursor *token_cursor,
                                  grn_obj *column);
GRN_API grn_rc
grn_token_cursor_set_query_options(grn_ctx *ctx,
                                   grn_token_cursor *token_cursor,
                                   grn_obj *query_options);
GRN_API grn_rc
grn_token_cursor_set_query_domain(grn_ctx *ctx,
                                  grn_token_cursor *token_cursor,
                                  grn_id domain);
GRN_API grn_id
grn_token_cursor_next(grn_ctx *ctx, grn_token_cursor *token_cursor);

GRN_API grn_token_cursor_status
grn_token_cursor_get_status(grn_ctx *ctx, grn_token_cursor *token_cursor);

GRN_API grn_rc
grn_token_cursor_close(grn_ctx *ctx, grn_token_cursor *token_cursor);

GRN_API grn_token *
grn_token_cursor_get_token(grn_ctx *ctx, grn_token_cursor *token_cursor);

#ifdef __cplusplus
}
#endif
