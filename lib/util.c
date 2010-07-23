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
#include "pat.h"
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

    GRN_TEXT_PUTS(ctx, buf, " default_tokenizer:");
    default_tokenizer = grn_obj_get_info(ctx, obj,
                                         GRN_INFO_DEFAULT_TOKENIZER, NULL);
    if (default_tokenizer) {
      grn_inspect_name(ctx, buf, default_tokenizer);
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
  int i;
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
  case GRN_BULK :
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
  case GRN_EXPR :
    grn_expr_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_ACCESSOR :
  case GRN_ACCESSOR_VIEW :
    grn_accessor_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_TYPE :
    grn_type_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_COLUMN_FIX_SIZE :
    grn_ra_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_COLUMN_VAR_SIZE :
    grn_ja_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_COLUMN_INDEX :
    grn_ii_inspect(ctx, buffer, obj);
    return buffer;
  case GRN_CURSOR_TABLE_PAT_KEY :
    grn_pat_cursor_inspect(ctx, (grn_pat_cursor *)obj, buffer);
    return buffer;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_NO_KEY :
    grn_table_inspect(ctx, buffer, obj);
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
