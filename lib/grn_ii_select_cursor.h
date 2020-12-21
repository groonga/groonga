/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct grn_ii_select_cursor_ grn_ii_select_cursor;

typedef struct {
  grn_id rid;
  uint32_t sid;
  uint32_t pos;
  uint32_t start_pos;
  uint32_t end_pos;
  uint32_t tf;
  int32_t weight;
  double score;
} grn_ii_select_cursor_posting;

grn_ii_select_cursor_posting *
grn_ii_select_cursor_next(grn_ctx *ctx, grn_ii_select_cursor *cursor);

grn_ii *
grn_ii_select_cursor_get_ii(grn_ctx *ctx, grn_ii_select_cursor *cursor);

#ifdef __cplusplus
}
#endif
