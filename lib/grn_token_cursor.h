/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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
#include "grn_token.h"
#include "grn_tokenizer.h"
#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_token_cursor {
  grn_obj *table;
  const unsigned char *orig;
  const unsigned char *curr;
  uint32_t orig_blen;
  uint32_t curr_size;
  int32_t pos;
  grn_tokenize_mode mode;
  grn_token_cursor_status status;
  grn_obj_flags table_flags;
  grn_encoding encoding;
  uint32_t flags;
  struct {
    grn_obj *object;
    grn_proc_ctx pctx;
    grn_tokenizer_query query;
    void *user_data;
    grn_token current_token;
    grn_token next_token;
  } tokenizer;
  struct {
    grn_obj *objects;
    void **data;
  } token_filter;
  uint32_t variant;
};

#ifdef __cplusplus
}
#endif
