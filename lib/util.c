/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010-2013 Brazil

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

#include "db.h"
#include "pat.h"
#include "ii.h"
#include "util.h"
#include "string_in.h"

#include <string.h>
#include <stdio.h>

grn_rc
grn_normalize_offset_and_limit(grn_ctx *ctx, int size, int *p_offset, int *p_limit)
{
  int end;
  int offset = *p_offset;
  int limit = *p_limit;

  if (offset < 0) {
    offset += size;
    if (offset < 0) {
      *p_offset = 0;
      *p_limit = 0;
      return GRN_TOO_SMALL_OFFSET;
    }
  } else if (offset != 0 && offset >= size) {
    *p_offset = 0;
    *p_limit = 0;
    return GRN_TOO_LARGE_OFFSET;
  }

  if (limit < 0) {
    limit += size + 1;
    if (limit < 0) {
      *p_offset = 0;
      *p_limit = 0;
      return GRN_TOO_SMALL_LIMIT;
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
}

grn_obj *
grn_inspect_name(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  int name_size;

  name_size = grn_obj_name(ctx, obj, NULL, 0);
  if (name_size) {
    grn_bulk_space(ctx, buf, name_size);
    grn_obj_name(ctx, obj, GRN_BULK_CURR(buf) - name_size, name_size);
  } else {
    GRN_TEXT_PUTS(ctx, buf, "(nil)");
  }

  return buf;
}

grn_obj *
grn_inspect_encoding(grn_ctx *ctx, grn_obj *buf, grn_encoding encoding)
{
  switch (encoding) {
  case GRN_ENC_DEFAULT :
    GRN_TEXT_PUTS(ctx, buf, "default(");
    grn_inspect_encoding(ctx, buf, grn_get_default_encoding());
    GRN_TEXT_PUTS(ctx, buf, ")");
    break;
  case GRN_ENC_NONE :
    GRN_TEXT_PUTS(ctx, buf, "none");
    break;
  case GRN_ENC_EUC_JP :
    GRN_TEXT_PUTS(ctx, buf, "EUC-JP");
    break;
  case GRN_ENC_UTF8 :
    GRN_TEXT_PUTS(ctx, buf, "UTF-8");
    break;
  case GRN_ENC_SJIS :
    GRN_TEXT_PUTS(ctx, buf, "Shift_JIS");
    break;
  case GRN_ENC_LATIN1 :
    GRN_TEXT_PUTS(ctx, buf, "Latin-1");
    break;
  case GRN_ENC_KOI8R :
    GRN_TEXT_PUTS(ctx, buf, "KOI8-R");
    break;
  default :
    GRN_TEXT_PUTS(ctx, buf, "unknown(");
    grn_text_itoa(ctx, buf, encoding);
    GRN_TEXT_PUTS(ctx, buf, ")");
    break;
  }

  return buf;
}

grn_obj *
grn_inspect_type(grn_ctx *ctx, grn_obj *buf, unsigned char type)
{
  switch (type) {
  case GRN_VOID :
    GRN_TEXT_PUTS(ctx, buf, "GRN_VOID");
    break;
  case GRN_BULK :
    GRN_TEXT_PUTS(ctx, buf, "GRN_BULK");
    break;
  case GRN_PTR :
    GRN_TEXT_PUTS(ctx, buf, "GRN_PTR");
    break;
  case GRN_UVECTOR :
    GRN_TEXT_PUTS(ctx, buf, "GRN_UVECTOR");
    break;
  case GRN_PVECTOR :
    GRN_TEXT_PUTS(ctx, buf, "GRN_PVECTOR");
    break;
  case GRN_VECTOR :
    GRN_TEXT_PUTS(ctx, buf, "GRN_VECTOR");
    break;
  case GRN_MSG :
    GRN_TEXT_PUTS(ctx, buf, "GRN_MSG");
    break;
  case GRN_QUERY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_QUERY");
    break;
  case GRN_ACCESSOR :
    GRN_TEXT_PUTS(ctx, buf, "GRN_ACCESSOR");
    break;
  case GRN_SNIP :
    GRN_TEXT_PUTS(ctx, buf, "GRN_SNIP");
    break;
  case GRN_PATSNIP :
    GRN_TEXT_PUTS(ctx, buf, "GRN_PATSNIP");
    break;
  case GRN_STRING :
    GRN_TEXT_PUTS(ctx, buf, "GRN_STRING");
    break;
  case GRN_CURSOR_TABLE_HASH_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_CURSOR_TABLE_HASH_KEY");
    break;
  case GRN_CURSOR_TABLE_PAT_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_CURSOR_TABLE_PAT_KEY");
    break;
  case GRN_CURSOR_TABLE_DAT_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_CURSOR_TABLE_DAT_KEY");
    break;
  case GRN_CURSOR_TABLE_NO_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_CURSOR_TABLE_NO_KEY");
    break;
  case GRN_CURSOR_COLUMN_INDEX :
    GRN_TEXT_PUTS(ctx, buf, "GRN_CURSOR_COLUMN_INDEX");
    break;
  case GRN_CURSOR_COLUMN_GEO_INDEX :
    GRN_TEXT_PUTS(ctx, buf, "GRN_CURSOR_COLUMN_GEO_INDEX");
    break;
  case GRN_TYPE :
    GRN_TEXT_PUTS(ctx, buf, "GRN_TYPE");
    break;
  case GRN_PROC :
    GRN_TEXT_PUTS(ctx, buf, "GRN_PROC");
    break;
  case GRN_EXPR :
    GRN_TEXT_PUTS(ctx, buf, "GRN_EXPR");
    break;
  case GRN_TABLE_HASH_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_TABLE_HASH_KEY");
    break;
  case GRN_TABLE_PAT_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_TABLE_PAT_KEY");
    break;
  case GRN_TABLE_DAT_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_TABLE_DAT_KEY");
    break;
  case GRN_TABLE_NO_KEY :
    GRN_TEXT_PUTS(ctx, buf, "GRN_TABLE_NO_KEY");
    break;
  case GRN_DB :
    GRN_TEXT_PUTS(ctx, buf, "GRN_DB");
    break;
  case GRN_COLUMN_FIX_SIZE :
    GRN_TEXT_PUTS(ctx, buf, "GRN_COLUMN_FIX_SIZE");
    break;
  case GRN_COLUMN_VAR_SIZE :
    GRN_TEXT_PUTS(ctx, buf, "GRN_COLUMN_VAR_SIZE");
    break;
  case GRN_COLUMN_INDEX :
    GRN_TEXT_PUTS(ctx, buf, "GRN_COLUMN_INDEX");
    break;
  default:
    {
      char type_in_hex[5]; /* "0xXX" */
      sprintf(type_in_hex, "%#02x", type);
      GRN_TEXT_PUTS(ctx, buf, "(unknown: ");
      GRN_TEXT_PUTS(ctx, buf, type_in_hex);
      GRN_TEXT_PUTS(ctx, buf, ")");
    }
    break;
  }

  return buf;
}

static grn_rc
grn_proc_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_proc *proc = (grn_proc *)obj;
  uint32_t i;

  GRN_TEXT_PUTS(ctx, buf, "#<proc:");
  switch (proc->type) {
  case GRN_PROC_TOKENIZER :
    GRN_TEXT_PUTS(ctx, buf, "tokenizer");
    break;
  case GRN_PROC_COMMAND :
    GRN_TEXT_PUTS(ctx, buf, "command");
    break;
  case GRN_PROC_FUNCTION :
    GRN_TEXT_PUTS(ctx, buf, "function");
    break;
  case GRN_PROC_HOOK :
    GRN_TEXT_PUTS(ctx, buf, "hook");
    break;
  case GRN_PROC_NORMALIZER :
    GRN_TEXT_PUTS(ctx, buf, "normalizer");
    break;
  }
  GRN_TEXT_PUTS(ctx, buf, " ");

  grn_inspect_name(ctx, buf, obj);
  GRN_TEXT_PUTS(ctx, buf, " ");

  GRN_TEXT_PUTS(ctx, buf, "arguments:[");
  for (i = 0; i < proc->nvars; i++) {
    grn_expr_var *var = proc->vars + i;
    if (i != 0) {
      GRN_TEXT_PUTS(ctx, buf, ", ");
    }
    GRN_TEXT_PUT(ctx, buf, var->name, var->name_size);
  }
  GRN_TEXT_PUTS(ctx, buf, "]");

  GRN_TEXT_PUTS(ctx, buf, ">");

  return GRN_SUCCESS;
}

static grn_rc
grn_accessor_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  return grn_column_name_(ctx, obj, buf);
}

static grn_rc
grn_type_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_id range_id;

  GRN_TEXT_PUTS(ctx, buf, "#<type ");
  grn_inspect_name(ctx, buf, obj);

  range_id = grn_obj_get_range(ctx, obj);
  GRN_TEXT_PUTS(ctx, buf, " size:");
  grn_text_lltoa(ctx, buf, range_id);

  GRN_TEXT_PUTS(ctx, buf, " type:");
  if (obj->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    GRN_TEXT_PUTS(ctx, buf, "var_size");
  } else {
    switch (obj->header.flags & GRN_OBJ_KEY_MASK) {
    case GRN_OBJ_KEY_UINT :
      GRN_TEXT_PUTS(ctx, buf, "uint");
      break;
    case GRN_OBJ_KEY_INT :
      GRN_TEXT_PUTS(ctx, buf, "int");
      break;
    case GRN_OBJ_KEY_FLOAT :
      GRN_TEXT_PUTS(ctx, buf, "float");
      break;
    case GRN_OBJ_KEY_GEO_POINT :
      GRN_TEXT_PUTS(ctx, buf, "geo_point");
      break;
    default :
      break;
    }
  }

  GRN_TEXT_PUTS(ctx, buf, ">");
  return GRN_SUCCESS;
}

static grn_rc
grn_column_inspect_common(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_id range_id;

  grn_inspect_name(ctx, buf, obj);

  range_id = grn_obj_get_range(ctx, obj);
  if (range_id) {
    grn_obj *range = grn_ctx_at(ctx, range_id);
    GRN_TEXT_PUTS(ctx, buf, " range:");
    if (range) {
      grn_inspect_name(ctx, buf, range);
      grn_obj_unlink(ctx, range);
    } else {
      grn_text_lltoa(ctx, buf, range_id);
    }
  }

  return GRN_SUCCESS;
}

static grn_rc
grn_store_inspect_body(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_column_inspect_common(ctx, buf, obj);
  GRN_TEXT_PUTS(ctx, buf, " type:");
  switch (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_VECTOR :
    GRN_TEXT_PUTS(ctx, buf, "vector");
    break;
  case GRN_OBJ_COLUMN_SCALAR :
    GRN_TEXT_PUTS(ctx, buf, "scalar");
    break;
  default:
    break;
  }

  GRN_TEXT_PUTS(ctx, buf, " compress:");
  switch (obj->header.flags & GRN_OBJ_COMPRESS_MASK) {
  case GRN_OBJ_COMPRESS_NONE :
    GRN_TEXT_PUTS(ctx, buf, "none");
    break;
  case GRN_OBJ_COMPRESS_ZLIB :
    GRN_TEXT_PUTS(ctx, buf, "zlib");
    break;
  case GRN_OBJ_COMPRESS_LZO :
    GRN_TEXT_PUTS(ctx, buf, "lzo");
    break;
  default:
    break;
  }

  if (obj->header.flags & GRN_OBJ_RING_BUFFER) {
    GRN_TEXT_PUTS(ctx, buf, " ring_buffer:true");
  }

  return GRN_SUCCESS;
}

static grn_rc
grn_ra_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  GRN_TEXT_PUTS(ctx, buf, "#<column:fix_size ");
  grn_store_inspect_body(ctx, buf, obj);
  GRN_TEXT_PUTS(ctx, buf, ">");
  return GRN_SUCCESS;
}

static grn_rc
grn_ja_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  GRN_TEXT_PUTS(ctx, buf, "#<column:var_size ");
  grn_store_inspect_body(ctx, buf, obj);
  GRN_TEXT_PUTS(ctx, buf, ">");
  return GRN_SUCCESS;
}

static grn_rc
grn_ii_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_obj sources;
  int i, n, have_flags = 0;
  grn_id *source_ids;

  GRN_TEXT_PUTS(ctx, buf, "#<column:index ");
  grn_column_inspect_common(ctx, buf, obj);

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
      grn_inspect_name(ctx, buf, source);
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

  GRN_TEXT_PUTS(ctx, buf, " elements:");
  grn_ii_inspect_elements(ctx, (grn_ii *)obj, buf);

  GRN_TEXT_PUTS(ctx, buf, ">");

  return GRN_SUCCESS;
}

static grn_rc
grn_table_type_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
    GRN_TEXT_PUTS(ctx, buf, "hash");
    break;
  case GRN_TABLE_PAT_KEY:
    GRN_TEXT_PUTS(ctx, buf, "pat");
    break;
  case GRN_TABLE_NO_KEY:
    GRN_TEXT_PUTS(ctx, buf, "no_key");
    break;
  }

  return GRN_SUCCESS;
}

static grn_rc
grn_table_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_hash *cols;
  grn_id range_id;
  grn_obj *range;

  GRN_TEXT_PUTS(ctx, buf, "#<table:");
  grn_table_type_inspect(ctx, buf, obj);
  GRN_TEXT_PUTS(ctx, buf, " ");

  grn_inspect_name(ctx, buf, obj);

  if (obj->header.type != GRN_TABLE_NO_KEY) {
    grn_obj *domain;
    grn_id domain_id;
    GRN_TEXT_PUTS(ctx, buf, " key:");
    domain_id = obj->header.domain;
    domain = grn_ctx_at(ctx, domain_id);
    if (domain) {
      grn_inspect_name(ctx, buf, domain);
      grn_obj_unlink(ctx, domain);
    } else if (domain_id) {
      grn_text_lltoa(ctx, buf, domain_id);
    } else {
      GRN_TEXT_PUTS(ctx, buf, "(nil)");
    }
  }

  GRN_TEXT_PUTS(ctx, buf, " value:");
  range_id = grn_obj_get_range(ctx, obj);
  range = grn_ctx_at(ctx, range_id);
  if (range) {
    grn_inspect_name(ctx, buf, range);
  } else if (range_id) {
    grn_text_lltoa(ctx, buf, range_id);
  } else {
    GRN_TEXT_PUTS(ctx, buf, "(nil)");
  }

  GRN_TEXT_PUTS(ctx, buf, " size:");
  grn_text_lltoa(ctx, buf, grn_table_size(ctx, obj));

  GRN_TEXT_PUTS(ctx, buf, " columns:[");
  if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                              GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
    if (grn_table_columns(ctx, obj, "", 0, (grn_obj *)cols)) {
      int i = 0;
      grn_id *key;
      GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
          grn_obj *col = grn_ctx_at(ctx, *key);
          if (col) {
            if (i++ > 0) { GRN_TEXT_PUTS(ctx, buf, ", "); }
            grn_column_name_(ctx, col, buf);
            grn_obj_unlink(ctx, col);
          }
        });
    }
    grn_hash_close(ctx, cols);
  }
  GRN_TEXT_PUTS(ctx, buf, "]");

  if (obj->header.type == GRN_TABLE_NO_KEY) {
    grn_table_cursor *tc;
    GRN_TEXT_PUTS(ctx, buf, " ids:[");
    tc = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0,
                               0, -1, GRN_CURSOR_ASCENDING);
    if (tc) {
      int i = 0;
      grn_id id;
      while ((id = grn_table_cursor_next(ctx, tc))) {
        if (i++ > 0) { GRN_TEXT_PUTS(ctx, buf, ", "); }
        grn_text_lltoa(ctx, buf, id);
      }
      grn_table_cursor_close(ctx, tc);
    }
    GRN_TEXT_PUTS(ctx, buf, "]");
  } else {
    grn_table_cursor *tc;
    grn_obj *default_tokenizer;
    grn_obj *normalizer;

    GRN_TEXT_PUTS(ctx, buf, " default_tokenizer:");
    default_tokenizer = grn_obj_get_info(ctx, obj,
                                         GRN_INFO_DEFAULT_TOKENIZER, NULL);
    if (default_tokenizer) {
      grn_inspect_name(ctx, buf, default_tokenizer);
      grn_obj_unlink(ctx, default_tokenizer);
    } else {
      GRN_TEXT_PUTS(ctx, buf, "(nil)");
    }

    GRN_TEXT_PUTS(ctx, buf, " normalizer:");
    normalizer = grn_obj_get_info(ctx, obj, GRN_INFO_NORMALIZER, NULL);
    if (normalizer) {
      grn_inspect_name(ctx, buf, normalizer);
      grn_obj_unlink(ctx, normalizer);
    } else {
      GRN_TEXT_PUTS(ctx, buf, "(nil)");
    }

    GRN_TEXT_PUTS(ctx, buf, " keys:[");
    tc = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0,
                               0, -1, GRN_CURSOR_ASCENDING);
    if (tc) {
      int i = 0;
      grn_id id;
      grn_obj key;
      GRN_OBJ_INIT(&key, GRN_BULK, 0, obj->header.domain);
      while ((id = grn_table_cursor_next(ctx, tc))) {
        if (i++ > 0) { GRN_TEXT_PUTS(ctx, buf, ", "); }
        grn_table_get_key2(ctx, obj, id, &key);
        grn_inspect(ctx, buf, &key);
        GRN_BULK_REWIND(&key);
      }
      GRN_OBJ_FIN(ctx, &key);
      grn_table_cursor_close(ctx, tc);
    }
    GRN_TEXT_PUTS(ctx, buf, "]");
  }

  if (obj->header.type == GRN_TABLE_PAT_KEY) {
    GRN_TEXT_PUTS(ctx, buf, " nodes:");
    grn_pat_inspect_nodes(ctx, (grn_pat *)obj, buf);
  }

  GRN_TEXT_PUTS(ctx, buf, ">");

  return GRN_SUCCESS;
}

static grn_rc
grn_geo_point_inspect_point(grn_ctx *ctx, grn_obj *buf, int point)
{
  GRN_TEXT_PUTS(ctx, buf, "(");
  grn_text_itoa(ctx, buf, point / 1000 / 3600 % 3600);
  GRN_TEXT_PUTS(ctx, buf, ", ");
  grn_text_itoa(ctx, buf, point / 1000 / 60 % 60);
  GRN_TEXT_PUTS(ctx, buf, ", ");
  grn_text_itoa(ctx, buf, point / 1000 % 60);
  GRN_TEXT_PUTS(ctx, buf, ", ");
  grn_text_itoa(ctx, buf, point % 1000);
  GRN_TEXT_PUTS(ctx, buf, ")");

  return GRN_SUCCESS;
}

static grn_rc
grn_geo_point_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  int latitude, longitude;

  GRN_GEO_POINT_VALUE(obj, latitude, longitude);

  GRN_TEXT_PUTS(ctx, buf, "[");
  GRN_TEXT_PUTS(ctx, buf, "(");
  grn_text_itoa(ctx, buf, latitude);
  GRN_TEXT_PUTS(ctx, buf, ",");
  grn_text_itoa(ctx, buf, longitude);
  GRN_TEXT_PUTS(ctx, buf, ")");

  GRN_TEXT_PUTS(ctx, buf, " (");
  grn_geo_point_inspect_point(ctx, buf, latitude);
  GRN_TEXT_PUTS(ctx, buf, ",");
  grn_geo_point_inspect_point(ctx, buf, longitude);
  GRN_TEXT_PUTS(ctx, buf, ")");

  {
    int i, j;
    grn_geo_point point;
    uint8_t encoded[sizeof(grn_geo_point)];

    GRN_TEXT_PUTS(ctx, buf, " [");
    point.latitude = latitude;
    point.longitude = longitude;
    grn_gton(encoded, &point, sizeof(grn_geo_point));
    for (i = 0; i < sizeof(grn_geo_point); i++) {
      if (i != 0) {
        GRN_TEXT_PUTS(ctx, buf, " ");
      }
      for (j = 0; j < 8; j++) {
        grn_text_itoa(ctx, buf, (encoded[i] >> (7 - j)) & 1);
      }
    }
    GRN_TEXT_PUTS(ctx, buf, "]");
  }

  GRN_TEXT_PUTS(ctx, buf, "]");

  return GRN_SUCCESS;
}

static grn_rc
grn_record_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_id id;
  grn_obj *table;
  grn_hash *cols;

  table = grn_ctx_at(ctx, obj->header.domain);
  GRN_TEXT_PUTS(ctx, buf, "#<record:");
  grn_table_type_inspect(ctx, buf, table);
  GRN_TEXT_PUTS(ctx, buf, ":");
  grn_inspect_name(ctx, buf, table);

  GRN_TEXT_PUTS(ctx, buf, " id:");
  id = GRN_RECORD_VALUE(obj);
  grn_text_lltoa(ctx, buf, id);

  if (grn_table_at(ctx, table, id)) {
    if (table->header.type != GRN_TABLE_NO_KEY) {
      grn_obj key;
      GRN_TEXT_PUTS(ctx, buf, " key:");
      GRN_OBJ_INIT(&key, GRN_BULK, 0, table->header.domain);
      grn_table_get_key2(ctx, table, id, &key);
      grn_inspect(ctx, buf, &key);
      GRN_OBJ_FIN(ctx, &key);
    }
    if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
      if (grn_table_columns(ctx, table, "", 0, (grn_obj *)cols)) {
        grn_id *key;
        GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col = grn_ctx_at(ctx, *key);
            if (col) {
              grn_obj value;
              GRN_TEXT_INIT(&value, 0);
              GRN_TEXT_PUTS(ctx, buf, " ");
              grn_column_name_(ctx, col, buf);
              GRN_TEXT_PUTS(ctx, buf, ":");
              grn_obj_get_value(ctx, col, id, &value);
              grn_inspect(ctx, buf, &value);
              GRN_OBJ_FIN(ctx, &value);
              grn_obj_unlink(ctx, col);
            }
          });
      }
      grn_hash_close(ctx, cols);
    }
  } else {
    GRN_TEXT_PUTS(ctx, buf, "(nonexistent)");
  }
  GRN_TEXT_PUTS(ctx, buf, ">");

  grn_obj_unlink(ctx, table);

  return GRN_SUCCESS;
}

static grn_rc
grn_uvector_record_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  int i = 0;
  grn_id *v, *ve;
  grn_obj record;

  v = (grn_id *)GRN_BULK_HEAD(obj);
  ve = (grn_id *)GRN_BULK_CURR(obj);
  GRN_RECORD_INIT(&record, 0, obj->header.domain);
  GRN_TEXT_PUTS(ctx, buf, "[");
  while (v < ve) {
    if (i++ > 0) { GRN_TEXT_PUTS(ctx, buf, ", "); }
    GRN_RECORD_SET(ctx, &record, *v);
    grn_inspect(ctx, buf, &record);
    v++;
  }
  GRN_TEXT_PUTS(ctx, buf, "]");
  GRN_OBJ_FIN(ctx, &record);

  return GRN_SUCCESS;
}

grn_obj *
grn_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj)
{
  grn_obj *domain;

  if (!buffer) {
    buffer = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  }

  if (!obj) {
    GRN_TEXT_PUTS(ctx, buffer, "(NULL)");
    return buffer;
  }

  switch (obj->header.type) {
  case GRN_VOID :
    /* TODO */
    break;
  case GRN_BULK :
    switch (obj->header.domain) {
    case GRN_DB_TOKYO_GEO_POINT :
    case GRN_DB_WGS84_GEO_POINT :
      grn_geo_point_inspect(ctx, buffer, obj);
      return buffer;
    default :
      domain = grn_ctx_at(ctx, obj->header.domain);
      if (domain) {
        grn_id type = domain->header.type;
        grn_obj_unlink(ctx, domain);
        switch (type) {
        case GRN_TABLE_HASH_KEY :
        case GRN_TABLE_PAT_KEY :
        case GRN_TABLE_NO_KEY :
          grn_record_inspect(ctx, buffer, obj);
          return buffer;
        default :
          break;
        }
      }
    }
    break;
  case GRN_PTR :
    /* TODO */
    break;
  case GRN_UVECTOR :
    domain = grn_ctx_at(ctx, obj->header.domain);
    if (domain) {
      grn_id type = domain->header.type;
      grn_obj_unlink(ctx, domain);
      switch (type) {
      case GRN_TABLE_HASH_KEY :
      case GRN_TABLE_PAT_KEY :
      case GRN_TABLE_NO_KEY :
        grn_uvector_record_inspect(ctx, buffer, obj);
        return buffer;
      default :
        break;
      }
    }
    break;
  case GRN_PVECTOR :
    /* TODO */
    break;
  case GRN_VECTOR :
    /* TODO */
    break;
  case GRN_MSG :
    /* TODO */
    break;
  case GRN_ACCESSOR :
    grn_accessor_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_SNIP :
  case GRN_PATSNIP :
    /* TODO */
    break;
  case GRN_STRING :
    grn_string_inspect(ctx, buffer, obj);
    break;
  case GRN_CURSOR_TABLE_HASH_KEY :
    /* TODO */
    break;
  case GRN_CURSOR_TABLE_PAT_KEY :
    grn_pat_cursor_inspect(ctx, (grn_pat_cursor *)obj, buffer);
    return buffer;
  case GRN_CURSOR_TABLE_DAT_KEY :
  case GRN_CURSOR_TABLE_NO_KEY :
  case GRN_CURSOR_COLUMN_INDEX :
  case GRN_CURSOR_COLUMN_GEO_INDEX :
    /* TODO */
    break;
  case GRN_TYPE :
    grn_type_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_PROC :
    grn_proc_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_EXPR :
    grn_expr_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
  case GRN_TABLE_NO_KEY :
    grn_table_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_DB :
    /* TODO */
    break;
  case GRN_COLUMN_FIX_SIZE :
    grn_ra_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_COLUMN_VAR_SIZE :
    grn_ja_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_COLUMN_INDEX :
    grn_ii_inspect(ctx, buffer, obj);
    return buffer;
  default :
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

void
grn_p_geo_point(grn_ctx *ctx, grn_geo_point *point)
{
  grn_obj obj;

  GRN_WGS84_GEO_POINT_INIT(&obj, 0);
  GRN_GEO_POINT_SET(ctx, &obj, point->latitude, point->longitude);
  grn_p(ctx, &obj);
  grn_obj_unlink(ctx, &obj);
}

#ifdef WIN32
static char *win32_base_dir = NULL;
const char *
grn_win32_base_dir(void)
{
  if (!win32_base_dir) {
    HMODULE dll;
    const wchar_t *dll_filename = GRN_DLL_FILENAME;
    wchar_t absolute_dll_filename[MAX_PATH];
    DWORD absolute_dll_filename_size;
    dll = GetModuleHandleW(dll_filename);
    absolute_dll_filename_size = GetModuleFileNameW(dll,
                                                    absolute_dll_filename,
                                                    MAX_PATH);
    if (absolute_dll_filename_size == 0) {
      win32_base_dir = ".";
    } else {
      DWORD ansi_dll_filename_size;
      ansi_dll_filename_size =
        WideCharToMultiByte(CP_ACP, 0,
                            absolute_dll_filename, absolute_dll_filename_size,
                            NULL, 0, NULL, NULL);
      if (ansi_dll_filename_size == 0) {
        win32_base_dir = ".";
      } else {
        char *path;
        win32_base_dir = malloc(ansi_dll_filename_size + 1);
        WideCharToMultiByte(CP_ACP, 0,
                            absolute_dll_filename, absolute_dll_filename_size,
                            win32_base_dir, ansi_dll_filename_size,
                            NULL, NULL);
        win32_base_dir[ansi_dll_filename_size] = '\0';
        if ((path = strrchr(win32_base_dir, '\\'))) {
          *path = '\0';
        }
        path = strrchr(win32_base_dir, '\\');
        if (path && (strcasecmp(path + 1, "bin") == 0 ||
                     strcasecmp(path + 1, "lib") == 0)) {
          *path = '\0';
        } else {
          path = win32_base_dir + strlen(win32_base_dir);
          *path = '\0';
        }
        for (path = win32_base_dir; *path; path++) {
          if (*path == '\\') {
            *path = '/';
          }
        }
      }
    }
  }
  return win32_base_dir;
}
#endif
