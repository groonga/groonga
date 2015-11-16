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

#ifndef GRN_TS_BUF_H
#define GRN_TS_BUF_H

#include "../grn.h"

#include "ts_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void *ptr;   /* The starting address. */
  size_t size; /* The size in bytes. */
  size_t pos;  /* The current position for grn_ts_buf_write(). */
} grn_ts_buf;

/* grn_ts_buf_init() initializes a buffer. */
void grn_ts_buf_init(grn_ctx *ctx, grn_ts_buf *buf);

/* grn_ts_buf_open() creates a buffer. */
/*grn_rc grn_ts_buf_open(grn_ctx *ctx, grn_ts_buf **buf);*/

/* grn_ts_buf_fin() finalizes a buffer. */
void grn_ts_buf_fin(grn_ctx *ctx, grn_ts_buf *buf);

/* grn_ts_buf_close() destroys a buffer. */
/*void grn_ts_buf_close(grn_ctx *ctx, grn_ts_buf *buf);*/

/*
 * grn_ts_buf_reserve() reserves enough memory to store new_size bytes.
 * Note that this function never shrinks a buffer and does nothing if new_size
 * is not greater than the current size.
 */
grn_rc grn_ts_buf_reserve(grn_ctx *ctx, grn_ts_buf *buf, size_t new_size);

/* grn_ts_buf_resize() resizes a buffer. */
grn_rc grn_ts_buf_resize(grn_ctx *ctx, grn_ts_buf *buf, size_t new_size);

/*
 * grn_ts_buf_write() writes data into a buffer. buf->pos specifies the
 * position and it will be modified on success.
 * Note that this function resizes a buffer if required.
 */
grn_rc grn_ts_buf_write(grn_ctx *ctx, grn_ts_buf *buf,
                        const void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_BUF_H */
