/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2017  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_column.h"
#include "grn_db.h"
#include "grn_ii.h"

grn_column_flags
grn_column_get_flags(grn_ctx *ctx, grn_obj *column)
{
  grn_column_flags flags = 0;

  GRN_API_ENTER;

  if (!column) {
    GRN_API_RETURN(0);
  }

  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE :
    flags = column->header.flags;
    break;
  case GRN_COLUMN_VAR_SIZE :
    flags = grn_ja_get_flags(ctx, (grn_ja *)column);
    break;
  case GRN_COLUMN_INDEX :
    flags = grn_ii_get_flags(ctx, (grn_ii *)column);
    break;
  default :
    break;
  }

  GRN_API_RETURN(flags);
}

grn_column_cache *
grn_column_cache_open(grn_ctx *ctx, grn_obj *column)
{
  grn_column_cache *cache;

  GRN_API_ENTER;

  if (!column) {
    GRN_API_RETURN(NULL);
  }

  grn_ra *ra = NULL;
  grn_obj *accessor = NULL;
  switch (column->header.type) {
  case GRN_ACCESSOR :
    {
      grn_accessor *a;
      for (a = (grn_accessor *)column; a->next; a = a->next) {
        switch (a->action) {
        case GRN_ACCESSOR_GET_KEY :
        case GRN_ACCESSOR_GET_VALUE :
          break;
        case GRN_ACCESSOR_GET_COLUMN_VALUE :
          if (a->next) {
            GRN_API_RETURN(NULL);
          }
          break;
        default :
          GRN_API_RETURN(NULL);
        }
      }
      grn_accessor *last_accessor = a;
      if (last_accessor->action != GRN_ACCESSOR_GET_COLUMN_VALUE) {
        GRN_API_RETURN(NULL);
      }
      if (last_accessor->obj->header.type != GRN_COLUMN_FIX_SIZE) {
        GRN_API_RETURN(NULL);
      }
      for (a = (grn_accessor *)column; a->next; a = a->next) {
        switch (a->action) {
        case GRN_ACCESSOR_GET_KEY :
          {
            grn_obj *domain = grn_ctx_at(ctx, a->obj->header.domain);
            bool is_table = grn_obj_is_table(ctx, domain);
            grn_obj_unref(ctx, domain);
            if (!is_table) {
              GRN_API_RETURN(NULL);
            }
          }
          break;
        case GRN_ACCESSOR_GET_VALUE :
          {
            grn_obj *range = grn_ctx_at(ctx, DB_OBJ(a->obj)->range);
            bool is_table = grn_obj_is_table(ctx, range);
            grn_obj_unref(ctx, range);
            if (!is_table) {
              GRN_API_RETURN(NULL);
            }
          }
          break;
        default :
          break;
        }
      }
      ra = (grn_ra *)(last_accessor->obj);
      accessor = column;
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
    ra = (grn_ra *)column;
    break;
  default :
    GRN_API_RETURN(NULL);
  }

  cache = GRN_MALLOC(sizeof(grn_column_cache));
  if (!cache) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[column-cache][open] failed to allocate memory");
    GRN_API_RETURN(NULL);
  }

  cache->ra = ra;
  cache->accessor = accessor;
  GRN_RA_CACHE_INIT(column, &(cache->ra_cache));

  GRN_API_RETURN(cache);
}

void
grn_column_cache_close(grn_ctx *ctx, grn_column_cache *cache)
{
  GRN_API_ENTER;

  if (!cache) {
    GRN_API_RETURN();
  }

  GRN_RA_CACHE_FIN(ctx, cache->ra, &(cache->ra_cache));
  GRN_FREE(cache);

  GRN_API_RETURN();
}

void *
grn_column_cache_ref(grn_ctx *ctx,
                     grn_column_cache *cache,
                     grn_id id,
                     size_t *value_size)
{
  void *value;

  GRN_API_ENTER;

  if (!cache) {
    *value_size = 0;
    GRN_API_RETURN(NULL);
  }

  if (cache->accessor) {
    id = grn_accessor_resolve_id(ctx, cache->accessor, id);
    if (id == GRN_ID_NIL) {
      *value_size = 0;
      GRN_API_RETURN(NULL);
    }
  }

  value = grn_ra_ref_cache(ctx, cache->ra, id, &(cache->ra_cache));
  if (value_size) {
    if (value) {
      *value_size = cache->ra->header->element_size;
    } else {
      *value_size = 0;
    }
  }

  GRN_API_RETURN(value);
}

static void
grn_column_copy_same_table(grn_ctx *ctx,
                           grn_obj *table,
                           grn_obj *from_column,
                           grn_obj *to_column)
{
  grn_id from_range_id = GRN_ID_NIL;
  grn_obj_flags from_range_flags = 0;
  grn_id to_range_id = GRN_ID_NIL;
  grn_obj_flags to_range_flags = 0;
  bool need_validate = false;
  if (grn_obj_is_reference_column(ctx, from_column) &&
      grn_obj_is_reference_column(ctx, to_column)) {
    grn_obj_get_range_info(ctx, from_column, &from_range_id, &from_range_flags);
    grn_obj_get_range_info(ctx, to_column, &to_range_id, &to_range_flags);
    need_validate = (from_range_id == to_range_id &&
                     from_range_flags == to_range_flags);
  }

  grn_obj value;
  GRN_VOID_INIT(&value);
  if (need_validate) {
    grn_obj validated_value;
    GRN_RECORD_INIT(&validated_value, to_range_flags, to_range_id);
    bool is_uvector = grn_obj_is_uvector(ctx, &validated_value);
    grn_obj *range = grn_ctx_at(ctx, to_range_id);
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, from_column, id, &value);
      GRN_BULK_REWIND(&validated_value);
      if (is_uvector) {
        uint32_t n_elements = grn_uvector_size(ctx, &value);
        uint32_t i;
        for (i = 0; i < n_elements; i++) {
          float weight;
          grn_id record_id = grn_uvector_get_element_record(ctx,
                                                            &value,
                                                            i,
                                                            &weight);
          if (record_id == GRN_ID_NIL ||
              record_id >= GRN_ID_MAX ||
              !grn_table_at(ctx, range, record_id)) {
            GRN_DEFINE_NAME(from_column);
            GRN_LOG(ctx,
                    GRN_LOG_WARNING,
                    "[column][copy][%.*s][%u] "
                    "ignore invalid reference value: %u",
                    name_size, name,
                    id,
                    record_id);
            continue;
          }
          grn_uvector_add_element_record(ctx,
                                         &validated_value,
                                         record_id,
                                         weight);
        }
      } else {
        if (GRN_BULK_VSIZE(&value) > 0) {
          grn_id record_id = GRN_RECORD_VALUE(&value);
          if (record_id == GRN_ID_NIL ||
              record_id >= GRN_ID_MAX ||
              !grn_table_at(ctx, range, record_id)) {
            GRN_DEFINE_NAME(from_column);
            GRN_LOG(ctx,
                    GRN_LOG_WARNING,
                    "[column][copy][%.*s][%u] "
                    "ignore invalid reference value: %u",
                    name_size, name,
                    id,
                    record_id);
          } else {
            GRN_RECORD_SET(ctx, &validated_value, record_id);
          }
        }
      }
      grn_obj_set_value(ctx, to_column, id, &validated_value, GRN_OBJ_SET);
    } GRN_TABLE_EACH_END(ctx, cursor);
    grn_obj_unref(ctx, range);
    GRN_OBJ_FIN(ctx, &validated_value);
  } else {
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, from_column, id, &value);
      grn_obj_set_value(ctx, to_column, id, &value, GRN_OBJ_SET);
    } GRN_TABLE_EACH_END(ctx, cursor);
  }
  GRN_OBJ_FIN(ctx, &value);
}

static void
grn_column_copy_same_key_type(grn_ctx *ctx,
                              grn_obj *from_table,
                              grn_obj *from_column,
                              grn_obj *to_table,
                              grn_obj *to_column)
{
  grn_table_cursor *cursor;
  grn_id from_id;
  grn_obj value;

  cursor = grn_table_cursor_open(ctx, from_table,
                                 NULL, 0,
                                 NULL, 0,
                                 0, -1, 0);
  if (!cursor) {
    return;
  }

  GRN_VOID_INIT(&value);
  while ((from_id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    void *key;
    int key_size;
    grn_id to_id;

    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    to_id = grn_table_add(ctx, to_table, key, key_size, NULL);
    if (to_id == GRN_ID_NIL) {
      char from_name[GRN_TABLE_MAX_KEY_SIZE];
      int from_name_size;
      char to_name[GRN_TABLE_MAX_KEY_SIZE];
      int to_name_size;
      grn_obj key_buffer;
      grn_obj inspected_key;

      from_name_size = grn_obj_name(ctx,
                                    from_column,
                                    from_name,
                                    sizeof(from_name));
      to_name_size = grn_obj_name(ctx,
                                  to_column,
                                  to_name,
                                  sizeof(to_name));
      if (from_table->header.domain == GRN_DB_SHORT_TEXT) {
        GRN_SHORT_TEXT_INIT(&key_buffer, 0);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&key_buffer, 0, from_table->header.domain);
      }
      grn_bulk_write(ctx, &key_buffer, key, key_size);
      grn_inspect(ctx, &inspected_key, &key_buffer);
      ERR(GRN_INVALID_ARGUMENT,
          "[column][copy] failed to copy key: <%.*s>: "
          "<%.*s> -> <%.*s>",
          (int)GRN_TEXT_LEN(&inspected_key),
          GRN_TEXT_VALUE(&inspected_key),
          from_name_size, from_name,
          to_name_size, to_name);
      GRN_OBJ_FIN(ctx, &inspected_key);
      GRN_OBJ_FIN(ctx, &key_buffer);
      break;
    }

    GRN_BULK_REWIND(&value);
    grn_obj_get_value(ctx, from_column, from_id, &value);
    grn_obj_set_value(ctx, to_column, to_id, &value, GRN_OBJ_SET);
  }
  GRN_OBJ_FIN(ctx, &value);
  grn_table_cursor_close(ctx, cursor);
}

static void
grn_column_copy_different(grn_ctx *ctx,
                          grn_obj *from_table,
                          grn_obj *from_column,
                          grn_obj *to_table,
                          grn_obj *to_column)
{
  grn_table_cursor *cursor;
  grn_id from_id;
  grn_obj from_key_buffer;
  grn_obj to_key_buffer;
  grn_obj value;

  cursor = grn_table_cursor_open(ctx, from_table,
                                 NULL, 0,
                                 NULL, 0,
                                 0, -1, 0);
  if (!cursor) {
    return;
  }

  if (from_table->header.domain == GRN_DB_SHORT_TEXT) {
    GRN_SHORT_TEXT_INIT(&from_key_buffer, 0);
  } else {
    GRN_VALUE_FIX_SIZE_INIT(&from_key_buffer, 0, from_table->header.domain);
  }
  if (to_table->header.domain == GRN_DB_SHORT_TEXT) {
    GRN_SHORT_TEXT_INIT(&to_key_buffer, 0);
  } else {
    GRN_VALUE_FIX_SIZE_INIT(&to_key_buffer, 0, to_table->header.domain);
  }
  GRN_VOID_INIT(&value);
  while ((from_id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    void *key;
    int key_size;
    grn_rc cast_rc;
    grn_id to_id;

    GRN_BULK_REWIND(&from_key_buffer);
    GRN_BULK_REWIND(&to_key_buffer);

    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    grn_bulk_write(ctx, &from_key_buffer, key, key_size);
    cast_rc = grn_obj_cast(ctx, &from_key_buffer, &to_key_buffer, GRN_FALSE);
    if (cast_rc != GRN_SUCCESS) {
      char from_name[GRN_TABLE_MAX_KEY_SIZE];
      int from_name_size;
      char to_name[GRN_TABLE_MAX_KEY_SIZE];
      int to_name_size;
      grn_obj *to_key_type;
      grn_obj inspected_key;
      grn_obj inspected_to_key_type;

      from_name_size = grn_obj_name(ctx,
                                    from_column,
                                    from_name,
                                    sizeof(from_name));
      to_name_size = grn_obj_name(ctx,
                                  to_column,
                                  to_name,
                                  sizeof(to_name));
      to_key_type = grn_ctx_at(ctx, to_table->header.domain);
      GRN_TEXT_INIT(&inspected_key, 0);
      GRN_TEXT_INIT(&inspected_to_key_type, 0);
      grn_inspect(ctx, &inspected_key, &from_key_buffer);
      grn_inspect(ctx, &inspected_to_key_type, to_key_type);
      ERR(cast_rc,
          "[column][copy] failed to cast key: <%.*s> -> %.*s: "
          "<%.*s> -> <%.*s>",
          (int)GRN_TEXT_LEN(&inspected_key),
          GRN_TEXT_VALUE(&inspected_key),
          (int)GRN_TEXT_LEN(&inspected_to_key_type),
          GRN_TEXT_VALUE(&inspected_to_key_type),
          from_name_size, from_name,
          to_name_size, to_name);
      GRN_OBJ_FIN(ctx, &inspected_key);
      GRN_OBJ_FIN(ctx, &inspected_to_key_type);
      break;
    }
    to_id = grn_table_add(ctx, to_table,
                          GRN_BULK_HEAD(&to_key_buffer),
                          GRN_BULK_VSIZE(&to_key_buffer),
                          NULL);
    if (to_id == GRN_ID_NIL) {
      continue;
    }

    GRN_BULK_REWIND(&value);
    grn_obj_get_value(ctx, from_column, from_id, &value);
    grn_obj_set_value(ctx, to_column, to_id, &value, GRN_OBJ_SET);
  }
  GRN_OBJ_FIN(ctx, &from_key_buffer);
  GRN_OBJ_FIN(ctx, &to_key_buffer);
  GRN_OBJ_FIN(ctx, &value);

  grn_table_cursor_close(ctx, cursor);
}

grn_rc
grn_column_copy(grn_ctx *ctx, grn_obj *from, grn_obj *to)
{
  grn_obj *from_table;
  grn_obj *to_table;

  GRN_API_ENTER;

  from_table = grn_ctx_at(ctx, from->header.domain);
  to_table = grn_ctx_at(ctx, to->header.domain);
  if ((from_table->header.type == GRN_TABLE_NO_KEY ||
       to_table->header.type == GRN_TABLE_NO_KEY) &&
      from_table != to_table) {
    char from_name[GRN_TABLE_MAX_KEY_SIZE];
    int from_name_size;
    char to_name[GRN_TABLE_MAX_KEY_SIZE];
    int to_name_size;

    from_name_size = grn_obj_name(ctx, from, from_name, sizeof(from_name));
    to_name_size = grn_obj_name(ctx, to, to_name, sizeof(to_name));
    ERR(GRN_OPERATION_NOT_SUPPORTED,
        "[column][copy] copy from/to TABLE_NO_KEY isn't supported: "
        "<%.*s> -> <%.*s>",
        from_name_size, from_name,
        to_name_size, to_name);
    goto exit;
  }

  if (from_table == to_table) {
    grn_column_copy_same_table(ctx, from_table, from, to);
  } else if (from_table->header.domain == to_table->header.domain) {
    grn_column_copy_same_key_type(ctx,
                                  from_table, from,
                                  to_table, to);
  } else {
    grn_column_copy_different(ctx,
                              from_table, from,
                              to_table, to);
  }

exit :
  GRN_API_RETURN(ctx->rc);
}

