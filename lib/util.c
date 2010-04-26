/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>
#include <stdio.h>
#include "db.h"
#include "util.h"

grn_rc
grn_normalize_offset_and_limit(grn_ctx *ctx, int size, int *p_offset, int *p_limit)
{
  int end;
  int offset = *p_offset;
  int limit = *p_limit;

  if (offset < 0) {
    offset += size;
    if (offset < 0) {
      ERR(GRN_INVALID_ARGUMENT, "too small offset");
      goto exit;
    }
  } else if (offset != 0 && offset >= size) {
    ERR(GRN_INVALID_ARGUMENT, "too large offset");
    goto exit;
  }

  if (limit < 0) {
    limit += size + 1;
    if (limit < 0) {
      ERR(GRN_INVALID_ARGUMENT, "too small limit");
      goto exit;
    }
  } else if (limit > size) {
    limit = size;
  }

  /* At this point, offset and limit must be zero or positive. */
  end = offset + limit;
  if (end > size) {
    limit -= end - size;
  }
  *p_offset = offset;
  *p_limit = limit;
  return GRN_SUCCESS;
exit:
  *p_offset = 0;
  *p_limit = 0;
  return ctx->rc;
}

grn_obj *
grn_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj)
{
  if (!buffer) {
    buffer = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  }

  if (!obj) {
    GRN_TEXT_PUTS(ctx, buffer, "(NULL)");
    return buffer;
  }

  if (obj->header.type == GRN_EXPR) {
    grn_expr_inspect(ctx, buffer, obj);
    return buffer;
  }

  grn_text_otoj(ctx, buffer, obj, NULL);
  return buffer;
}

void
grn_p(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj buffer;

  GRN_TEXT_INIT(&buffer, 0);
  grn_inspect(ctx, &buffer, obj);
  printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));
  grn_obj_unlink(ctx, &buffer);
}
