/*
  Copyright (C) 2016  Brazil
  Copyright (C) 2019-2023  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx.h"

grn_rc
grn_dump_table_create_flags(grn_ctx *ctx,
                            grn_table_flags flags,
                            grn_obj *buffer)
{
  GRN_API_ENTER;

  switch (flags & GRN_OBJ_TABLE_TYPE_MASK) {
  case GRN_OBJ_TABLE_HASH_KEY:
    GRN_TEXT_PUTS(ctx, buffer, "TABLE_HASH_KEY");
    break;
  case GRN_OBJ_TABLE_PAT_KEY:
    GRN_TEXT_PUTS(ctx, buffer, "TABLE_PAT_KEY");
    break;
  case GRN_OBJ_TABLE_DAT_KEY:
    GRN_TEXT_PUTS(ctx, buffer, "TABLE_DAT_KEY");
    break;
  case GRN_OBJ_TABLE_NO_KEY:
    GRN_TEXT_PUTS(ctx, buffer, "TABLE_NO_KEY");
    break;
  }
  if (flags & GRN_OBJ_KEY_LARGE) {
    GRN_TEXT_PUTS(ctx, buffer, "|KEY_LARGE");
  }
  if (flags & GRN_OBJ_KEY_WITH_SIS) {
    GRN_TEXT_PUTS(ctx, buffer, "|KEY_WITH_SIS");
  }
  if (flags & GRN_OBJ_KEY_NORMALIZE) {
    GRN_TEXT_PUTS(ctx, buffer, "|KEY_NORMALIZE");
  }
  if (flags & GRN_OBJ_PERSISTENT) {
    GRN_TEXT_PUTS(ctx, buffer, "|PERSISTENT");
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_dump_column_create_flags(grn_ctx *ctx,
                             grn_column_flags flags,
                             grn_obj *buffer)
{
  GRN_API_ENTER;

  switch (flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_SCALAR:
    GRN_TEXT_PUTS(ctx, buffer, "COLUMN_SCALAR");
    break;
  case GRN_OBJ_COLUMN_VECTOR:
    GRN_TEXT_PUTS(ctx, buffer, "COLUMN_VECTOR");
    if (flags & GRN_OBJ_WITH_WEIGHT) {
      GRN_TEXT_PUTS(ctx, buffer, "|WITH_WEIGHT");
    }
    if (flags & GRN_OBJ_WEIGHT_FLOAT32) {
      GRN_TEXT_PUTS(ctx, buffer, "|WEIGHT_FLOAT32");
    }
    if (flags & GRN_OBJ_WEIGHT_BFLOAT16) {
      GRN_TEXT_PUTS(ctx, buffer, "|WEIGHT_BFLOAT16");
    }
    break;
  case GRN_OBJ_COLUMN_INDEX:
    GRN_TEXT_PUTS(ctx, buffer, "COLUMN_INDEX");
    if (flags & GRN_OBJ_WITH_SECTION) {
      GRN_TEXT_PUTS(ctx, buffer, "|WITH_SECTION");
    }
    if (flags & GRN_OBJ_WITH_WEIGHT) {
      GRN_TEXT_PUTS(ctx, buffer, "|WITH_WEIGHT");
    }
    if (flags & GRN_OBJ_WITH_POSITION) {
      GRN_TEXT_PUTS(ctx, buffer, "|WITH_POSITION");
    }
    if (flags & GRN_OBJ_INDEX_SMALL) {
      GRN_TEXT_PUTS(ctx, buffer, "|INDEX_SMALL");
    }
    if (flags & GRN_OBJ_INDEX_MEDIUM) {
      GRN_TEXT_PUTS(ctx, buffer, "|INDEX_MEDIUM");
    }
    if (flags & GRN_OBJ_INDEX_LARGE) {
      GRN_TEXT_PUTS(ctx, buffer, "|INDEX_LARGE");
    }
    if (flags & GRN_OBJ_WEIGHT_FLOAT32) {
      GRN_TEXT_PUTS(ctx, buffer, "|WEIGHT_FLOAT32");
    }
    if (flags & GRN_OBJ_WEIGHT_BFLOAT16) {
      GRN_TEXT_PUTS(ctx, buffer, "|WEIGHT_BFLOAT16");
    }
    break;
  }
  switch (flags & GRN_OBJ_COMPRESS_MASK) {
  case GRN_OBJ_COMPRESS_NONE:
    break;
  case GRN_OBJ_COMPRESS_ZLIB:
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_ZLIB");
    break;
  case GRN_OBJ_COMPRESS_LZ4:
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_LZ4");
    break;
  case GRN_OBJ_COMPRESS_ZSTD:
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_ZSTD");
    break;
  }
  switch (flags & GRN_OBJ_MISSING_MASK) {
  case GRN_OBJ_MISSING_ADD:
    break;
  case GRN_OBJ_MISSING_IGNORE:
    GRN_TEXT_PUTS(ctx, buffer, "|MISSING_IGNORE");
    break;
  case GRN_OBJ_MISSING_NIL:
    GRN_TEXT_PUTS(ctx, buffer, "|MISSING_NIL");
    break;
  }
  switch (flags & GRN_OBJ_INVALID_MASK) {
  case GRN_OBJ_INVALID_ERROR:
    break;
  case GRN_OBJ_INVALID_WARN:
    GRN_TEXT_PUTS(ctx, buffer, "|INVALID_WARN");
    break;
  case GRN_OBJ_INVALID_IGNORE:
    GRN_TEXT_PUTS(ctx, buffer, "|INVALID_IGNORE");
    break;
  }
  if (flags & GRN_OBJ_COMPRESS_FILTER_SHUFFLE) {
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_FILTER_SHUFFLE");
  }
  if (flags & GRN_OBJ_COMPRESS_FILTER_BYTE_DELTA) {
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_FILTER_BYTE_DELTA");
  }
  if (flags & GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES) {
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES");
  } else if (flags & GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE) {
    GRN_TEXT_PUTS(ctx, buffer, "|COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE");
  }
  if (flags & GRN_OBJ_PERSISTENT) {
    GRN_TEXT_PUTS(ctx, buffer, "|PERSISTENT");
  }

  GRN_API_RETURN(ctx->rc);
}
