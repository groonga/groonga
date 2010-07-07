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

static grn_rc
grn_accessor_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  return grn_column_name_(ctx, obj, buf);
}

static grn_rc
grn_ii_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_obj sources;
  int name_size, i, n, have_flags = 0;
  grn_id range_id, *source_ids;

  GRN_TEXT_PUTS(ctx, buf, "#<column:index ");
  name_size = grn_obj_name(ctx, obj, NULL, 0);
  if (name_size) {
    grn_bulk_space(ctx, buf, name_size);
    grn_obj_name(ctx, obj, GRN_BULK_CURR(buf) - name_size, name_size);
  }

  range_id = grn_obj_get_range(ctx, obj);
  if (range_id) {
    grn_obj *range = grn_ctx_at(ctx, range_id);
    if (range) {
      GRN_TEXT_PUTS(ctx, buf, " range:");
      name_size = grn_obj_name(ctx, range, NULL, 0);
      if (name_size) {
        grn_bulk_space(ctx, buf, name_size);
        grn_obj_name(ctx, range, GRN_BULK_CURR(buf) - name_size, name_size);
      }
      grn_obj_unlink(ctx, range);
    }
  }

  GRN_TEXT_INIT(&sources, 0);
  grn_obj_get_info(ctx, obj, GRN_INFO_SOURCE, &sources);
  source_ids = (grn_id *)GRN_BULK_HEAD(&sources);
  n = GRN_BULK_VSIZE(&sources) / sizeof(grn_id);
  GRN_TEXT_PUTS(ctx, buf, " sources:[");
  for (i = 0; i < n; i++) {
    grn_id source_id;
    grn_obj *source;
    if (i) { GRN_TEXT_PUTS(ctx, buf, ", "); }
    source_id = source_ids[i];
    source = grn_ctx_at(ctx, source_id);
    if (source) {
      name_size = grn_obj_name(ctx, source, NULL, 0);
      if (name_size) {
        grn_bulk_space(ctx, buf, name_size);
        grn_obj_name(ctx, source, GRN_BULK_CURR(buf) - name_size, name_size);
      }
    } else {
      grn_text_lltoa(ctx, buf, source_id);
    }
  }
  GRN_TEXT_PUTS(ctx, buf, "]");
  GRN_OBJ_FIN(ctx, &sources);

  GRN_TEXT_PUTS(ctx, buf, " flags:");
  if (obj->header.flags & GRN_OBJ_WITH_SECTION) {
    GRN_TEXT_PUTS(ctx, buf, "SECTION");
    have_flags = 1;
  }
  if (obj->header.flags & GRN_OBJ_WITH_WEIGHT) {
    if (have_flags) { GRN_TEXT_PUTS(ctx, buf, "|"); }
    GRN_TEXT_PUTS(ctx, buf, "WEIGHT");
    have_flags = 1;
  }
  if (obj->header.flags & GRN_OBJ_WITH_POSITION) {
    if (have_flags) { GRN_TEXT_PUTS(ctx, buf, "|"); }
    GRN_TEXT_PUTS(ctx, buf, "POSITION");
    have_flags = 1;
  }
  if (!have_flags) {
    GRN_TEXT_PUTS(ctx, buf, "NONE");
  }

  GRN_TEXT_PUTS(ctx, buf, ">");

  return GRN_SUCCESS;
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

  switch (obj->header.type) {
  case GRN_EXPR :
    grn_expr_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_ACCESSOR :
  case GRN_ACCESSOR_VIEW :
    grn_accessor_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_COLUMN_INDEX :
    grn_ii_inspect(ctx, buffer, obj);
  default:
    break;
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
