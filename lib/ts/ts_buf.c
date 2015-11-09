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

#include "ts_buf.h"

#include "../grn_ctx_impl.h"

#include "ts_log.h"

void
grn_ts_buf_init(grn_ctx *ctx, grn_ts_buf *buf)
{
  buf->ptr = NULL;
  buf->size = 0;
  buf->pos = 0;
}

/*
grn_rc
grn_ts_buf_open(grn_ctx *ctx, grn_ts_buf **buf)
{
  grn_ts_buf *new_buf = GRN_MALLOCN(grn_ts_buf, 1);
  if (!new_buf) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_MALLOCN failed: %zu x 1",
                      sizeof(grn_ts_buf));
  }
  grn_ts_buf_init(ctx, new_buf);
  *buf = new_buf;
  return GRN_SUCCESS;
}
*/

void
grn_ts_buf_fin(grn_ctx *ctx, grn_ts_buf *buf)
{
  if (buf->ptr) {
    GRN_FREE(buf->ptr);
  }
}

/*
void
grn_ts_buf_close(grn_ctx *ctx, grn_ts_buf *buf)
{
  if (buf) {
    grn_ts_buf_fin(ctx, buf);
  }
}
*/

grn_rc
grn_ts_buf_reserve(grn_ctx *ctx, grn_ts_buf *buf, size_t new_size)
{
  void *new_ptr;
  size_t enough_size;
  if (new_size <= buf->size) {
    return GRN_SUCCESS;
  }
  enough_size = buf->size ? (buf->size << 1) : 1;
  while (enough_size < new_size) {
    enough_size <<= 1;
  }
  new_ptr = GRN_REALLOC(buf->ptr, enough_size);
  if (!new_ptr) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_REALLOC failed: %zu",
                      enough_size);
  }
  buf->ptr = new_ptr;
  buf->size = enough_size;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_buf_resize(grn_ctx *ctx, grn_ts_buf *buf, size_t new_size)
{
  void *new_ptr;
  if (new_size == buf->size) {
    return GRN_SUCCESS;
  }
  if (!new_size) {
    if (buf->ptr) {
      GRN_FREE(buf->ptr);
      buf->ptr = NULL;
      buf->size = new_size;
    }
    return GRN_SUCCESS;
  }
  new_ptr = GRN_REALLOC(buf->ptr, new_size);
  if (!new_ptr) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_REALLOC failed: %zu",
                      new_size);
  }
  buf->ptr = new_ptr;
  buf->size = new_size;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_buf_write(grn_ctx *ctx, grn_ts_buf *buf, const void *ptr, size_t size)
{
  size_t new_pos = buf->pos + size;
  if (new_pos > buf->size) {
    grn_rc rc = grn_ts_buf_reserve(ctx, buf, new_pos);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  grn_memcpy((char *)buf->ptr + buf->pos, ptr, size);
  buf->pos += size;
  return GRN_SUCCESS;
}
