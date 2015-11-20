/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

#ifndef GRN_TS_CURSOR_H
#define GRN_TS_CURSOR_H

#include "../grn.h"

#include "ts_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GRN_TS_HASH_CURSOR,
  GRN_TS_PAT_CURSOR,
  GRN_TS_DAT_CURSOR,
  GRN_TS_ARRAY_CURSOR
} grn_ts_cursor_type;

typedef struct {
  grn_ts_cursor_type type;
} grn_ts_cursor;

/* grn_ts_cursor_open() creates a cursor. */
grn_rc grn_ts_cursor_open(grn_ctx *ctx, grn_ts_cursor **cursor);

/* grn_ts_cursor_close() destroys a cursor. */
grn_rc grn_ts_cursor_close(grn_ctx *ctx, grn_ts_cursor *cursor);

/* grn_ts_cursor_read() reads records from a cursor. */
grn_rc grn_ts_cursor_read(grn_ctx *ctx, grn_ts_cursor *cursor,
                          grn_ts_record *out, size_t max_n_out, size_t *n_out);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_CURSOR_H */
