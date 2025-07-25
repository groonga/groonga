/*
  Copyright (C) 2016-2017  Brazil
  Copyright (C) 2019-2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../grn_pat.h"
#include "../grn_dat.h"
#include "../grn_ii.h"

#include "../grn_proc.h"

#include <groonga/plugin.h>

static void
command_object_inspect_dispatch(grn_ctx *ctx, grn_obj *obj);

static void
command_object_inspect_obj_name(grn_ctx *ctx, grn_obj *obj)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_size;

  name_size = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
  grn_ctx_output_str(ctx, name, (size_t)name_size);
}

static void
command_object_inspect_obj_type(grn_ctx *ctx, uint8_t type)
{
  grn_ctx_output_map_open(ctx, "type", 2);
  {
    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint64(ctx, type);
    grn_ctx_output_cstr(ctx, "name");
    grn_ctx_output_cstr(ctx, grn_obj_type_to_string(type));
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_type(grn_ctx *ctx, grn_obj *type)
{
  if (!type) {
    grn_ctx_output_null(ctx);
    return;
  }

  grn_ctx_output_map_open(ctx, "type", 4);
  {
    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint64(ctx, grn_obj_id(ctx, type));
    grn_ctx_output_cstr(ctx, "name");
    command_object_inspect_obj_name(ctx, type);
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_obj_type(ctx, type->header.type);
    grn_ctx_output_cstr(ctx, "size");
    if (type->header.type == GRN_TYPE) {
      grn_ctx_output_uint64(ctx, grn_type_size(ctx, type));
    } else {
      grn_ctx_output_uint64(ctx, sizeof(grn_id));
    }
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_disk_usage(grn_ctx *ctx, grn_obj *obj)
{
  grn_ctx_output_uint64(ctx, grn_obj_get_disk_usage(ctx, obj));
}

static void
command_object_inspect_table_hash_key_key(grn_ctx *ctx, grn_hash *hash)
{
  grn_ctx_output_map_open(ctx, "key", 3);
  {
    grn_ctx_output_cstr(ctx, "type");
    grn_obj *type = grn_ctx_at(ctx, hash->obj.header.domain);
    command_object_inspect_type(ctx, type);
    grn_obj_unref(ctx, type);
    grn_ctx_output_cstr(ctx, "total_size");
    grn_ctx_output_uint64(ctx, grn_hash_total_key_size(ctx, hash));
    grn_ctx_output_cstr(ctx, "max_total_size");
    grn_ctx_output_uint64(ctx, grn_hash_max_total_key_size(ctx, hash));
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_table_pat_key_key(grn_ctx *ctx, grn_pat *pat)
{
  grn_ctx_output_map_open(ctx, "key", 3);
  {
    grn_ctx_output_cstr(ctx, "type");
    grn_obj *type = grn_ctx_at(ctx, pat->obj.header.domain);
    command_object_inspect_type(ctx, type);
    grn_obj_unref(ctx, type);
    grn_ctx_output_cstr(ctx, "total_size");
    grn_ctx_output_uint64(ctx, grn_pat_total_key_size(ctx, pat));
    grn_ctx_output_cstr(ctx, "max_total_size");
    grn_ctx_output_uint64(ctx, grn_pat_max_total_key_size(ctx, pat));
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_table_dat_key_key(grn_ctx *ctx, grn_dat *dat)
{
  grn_ctx_output_map_open(ctx, "key", 1);
  {
    grn_ctx_output_cstr(ctx, "type");
    grn_obj *type = grn_ctx_at(ctx, dat->obj.header.domain);
    command_object_inspect_type(ctx, type);
    grn_obj_unref(ctx, type);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_table_key(grn_ctx *ctx, grn_obj *table)
{
  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
    command_object_inspect_table_hash_key_key(ctx, (grn_hash *)table);
    break;
  case GRN_TABLE_PAT_KEY:
    command_object_inspect_table_pat_key_key(ctx, (grn_pat *)table);
    break;
  case GRN_TABLE_DAT_KEY:
    command_object_inspect_table_dat_key_key(ctx, (grn_dat *)table);
    break;
  case GRN_TABLE_NO_KEY:
    grn_ctx_output_null(ctx);
    break;
  default:
    break;
  }
}

static void
command_object_inspect_table_value(grn_ctx *ctx, grn_obj *table)
{
  if (table->header.type == GRN_TABLE_DAT_KEY) {
    grn_ctx_output_null(ctx);
  } else {
    grn_ctx_output_map_open(ctx, "value", 1);
    {
      grn_id range_id = grn_obj_get_range(ctx, table);
      grn_ctx_output_cstr(ctx, "type");
      grn_obj *type = grn_ctx_at(ctx, range_id);
      command_object_inspect_type(ctx, type);
      grn_obj_unref(ctx, type);
    }
    grn_ctx_output_map_close(ctx);
  }
}

static void
command_object_inspect_table(grn_ctx *ctx, grn_obj *obj)
{
  grn_ctx_output_map_open(ctx, "table", 7);
  {
    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint64(ctx, grn_obj_id(ctx, obj));
    grn_ctx_output_cstr(ctx, "name");
    command_object_inspect_obj_name(ctx, obj);
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_obj_type(ctx, obj->header.type);
    grn_ctx_output_cstr(ctx, "key");
    command_object_inspect_table_key(ctx, obj);
    grn_ctx_output_cstr(ctx, "value");
    command_object_inspect_table_value(ctx, obj);
    grn_ctx_output_cstr(ctx, "n_records");
    grn_ctx_output_uint64(ctx, grn_table_size(ctx, obj));
    grn_ctx_output_cstr(ctx, "disk_usage");
    command_object_inspect_disk_usage(ctx, obj);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_column_name(grn_ctx *ctx, grn_obj *column)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_size;

  name_size = grn_column_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
  name[name_size] = '\0';
  grn_ctx_output_str(ctx, name, (size_t)name_size);
}

static void
command_object_inspect_column_type_name(grn_ctx *ctx, grn_obj *column)
{
  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
    switch (column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
    case GRN_OBJ_COLUMN_SCALAR:
      grn_ctx_output_cstr(ctx, "scalar");
      break;
    case GRN_OBJ_COLUMN_VECTOR:
      grn_ctx_output_cstr(ctx, "vector");
      break;
    }
    break;
  case GRN_COLUMN_INDEX:
    grn_ctx_output_cstr(ctx, "index");
    break;
  default:
    break;
  }
}

static void
command_object_inspect_column_type(grn_ctx *ctx, grn_obj *column)
{
  grn_ctx_output_map_open(ctx, "type", 2);
  {
    grn_ctx_output_cstr(ctx, "name");
    command_object_inspect_column_type_name(ctx, column);

    grn_ctx_output_cstr(ctx, "raw");
    grn_ctx_output_map_open(ctx, "raw", 2);
    {
      grn_ctx_output_cstr(ctx, "id");
      grn_ctx_output_uint64(ctx, column->header.type);
      grn_ctx_output_cstr(ctx, "name");
      grn_ctx_output_cstr(ctx, grn_obj_type_to_string(column->header.type));
    }
    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_column_index_value_statistics(grn_ctx *ctx, grn_ii *ii)
{
  grn_ctx_output_map_open(ctx, "statistics", 13);
  {
    grn_ii_header_common *h = ii->header.common;

    grn_ctx_output_cstr(ctx, "max_section_id");
    grn_ctx_output_uint64(ctx, grn_ii_max_section(ii));

    {
      uint32_t max_id = 0;
      uint32_t n_garbage_segments = 0;
      uint32_t n_array_segments = 0;
      uint32_t n_buffer_segments = 0;

      grn_ctx_output_cstr(ctx, "n_garbage_segments");
      {
        uint32_t i;

        for (i = h->bgqtail; i != h->bgqhead;
             i = ((i + 1) & (GRN_II_BGQSIZE - 1))) {
          uint32_t id = h->bgqbody[i];
          n_garbage_segments++;
          if (id > max_id) {
            max_id = id;
          }
        }
        grn_ctx_output_uint64(ctx, n_garbage_segments);
      }

      const uint32_t n_logical_segments = grn_ii_n_logical_segments(ii);
      grn_ctx_output_cstr(ctx, "max_array_segment_id");
      grn_ctx_output_uint64(ctx, h->amax);
      grn_ctx_output_cstr(ctx, "n_array_segments");
      {
        for (uint32_t lseg = 0; lseg < n_logical_segments; lseg++) {
          const uint32_t pseg = grn_ii_get_array_pseg(ii, lseg);
          if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
            if (pseg > max_id) {
              max_id = pseg;
            }
            n_array_segments++;
          }
        }
        grn_ctx_output_uint64(ctx, n_array_segments);
      }

      grn_ctx_output_cstr(ctx, "max_buffer_segment_id");
      grn_ctx_output_uint64(ctx, h->bmax);
      grn_ctx_output_cstr(ctx, "n_buffer_segments");
      {
        for (uint32_t lseg = 0; lseg < n_logical_segments; lseg++) {
          const uint32_t pseg = grn_ii_get_buffer_pseg(ii, lseg);
          if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
            if (pseg > max_id) {
              max_id = pseg;
            }
            n_buffer_segments++;
          }
        }
        grn_ctx_output_uint64(ctx, n_buffer_segments);
      }

      grn_ctx_output_cstr(ctx, "max_in_use_physical_segment_id");
      grn_ctx_output_uint64(ctx, max_id);

      grn_ctx_output_cstr(ctx, "n_unmanaged_segments");
      grn_ctx_output_uint64(ctx,
                            h->pnext - n_array_segments - n_buffer_segments -
                              n_garbage_segments);
    }

    {
      grn_ctx_output_cstr(ctx, "total_chunk_size");
      grn_ctx_output_uint64(ctx, h->total_chunk_size);
      grn_ctx_output_cstr(ctx, "max_in_use_chunk_id");
      {
        uint32_t i;
        uint32_t max_id;

        for (max_id = 0, i = 0; i < (GRN_II_MAX_CHUNK >> 3); i++) {
          uint8_t sub_chunk_info = h->chunks[i];
          uint8_t bit;

          if (sub_chunk_info == 0) {
            continue;
          }
          for (bit = 0; bit < 8; bit++) {
            if (sub_chunk_info & (1 << bit)) {
              max_id = (i << 3) + sub_chunk_info;
            }
          }
        }
        grn_ctx_output_uint64(ctx, max_id);
      }
      grn_ctx_output_cstr(ctx, "n_garbage_chunks");
      grn_ctx_output_array_open(ctx,
                                "n_garbage_chunks",
                                GRN_II_N_CHUNK_VARIATION + 1);
      {
        uint32_t i;
        for (i = 0; i <= GRN_II_N_CHUNK_VARIATION; i++) {
          grn_ctx_output_uint64(ctx, h->ngarbages[i]);
        }
      }
      grn_ctx_output_array_close(ctx);
    }

    grn_ctx_output_cstr(ctx, "next_physical_segment_id");
    grn_ctx_output_uint64(ctx, h->pnext);

    grn_ctx_output_cstr(ctx, "max_n_physical_segments");
    grn_ctx_output_uint64(ctx, ii->seg->header->max_segment);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_column_data_value_compress(grn_ctx *ctx, grn_obj *column)
{
  const char *compress = NULL;
  grn_column_flags column_flags;

  column_flags = grn_column_get_flags(ctx, column);
  switch (column_flags & GRN_OBJ_COMPRESS_MASK) {
  case GRN_OBJ_COMPRESS_ZLIB:
    compress = "zlib";
    break;
  case GRN_OBJ_COMPRESS_LZ4:
    compress = "lz4";
    break;
  case GRN_OBJ_COMPRESS_ZSTD:
    compress = "zstd";
    break;
  default:
    break;
  }

  if (compress) {
    grn_ctx_output_cstr(ctx, compress);
  } else {
    grn_ctx_output_null(ctx);
  }
}

static void
command_object_inspect_column_data_value_compress_filters(grn_ctx *ctx,
                                                          grn_obj *column)
{
  grn_column_flags column_flags = grn_column_get_flags(ctx, column);
  uint32_t n_filters = 0;
  if (column_flags & GRN_OBJ_COMPRESS_FILTER_SHUFFLE) {
    n_filters++;
  }
  if (column_flags & GRN_OBJ_COMPRESS_FILTER_BYTE_DELTA) {
    n_filters++;
  }
  grn_ctx_output_array_open(ctx, "compress_filters", n_filters);
  if (column_flags & GRN_OBJ_COMPRESS_FILTER_SHUFFLE) {
    grn_ctx_output_cstr(ctx, "shuffle");
  }
  if (column_flags & GRN_OBJ_COMPRESS_FILTER_BYTE_DELTA) {
    grn_ctx_output_cstr(ctx, "byte_delta");
  }
  if (column_flags & GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES) {
    grn_ctx_output_cstr(ctx, "truncate_precision_2bytes");
  } else if (column_flags & GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE) {
    grn_ctx_output_cstr(ctx, "truncate_precision_1byte");
  }
  grn_ctx_output_array_close(ctx);
}

static void
command_object_inspect_column_value(grn_ctx *ctx, grn_obj *column)
{
  int n_elements = 1;
  grn_column_flags column_flags = grn_column_get_flags(ctx, column);
  bool is_index = false;
  bool is_vector = false;
  switch (column_flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_VECTOR:
    is_vector = true;
    break;
  case GRN_OBJ_COLUMN_INDEX:
    is_index = true;
    break;
  default:
    break;
  }
  bool is_generated_column = grn_obj_is_generated_column(ctx, column);

  if (is_index) {
    n_elements += 5;
  } else {
    n_elements += 2;
    if (is_vector) {
      n_elements += 3;
    }
    if (is_generated_column) {
      n_elements += 1;
    }
  }
  grn_ctx_output_map_open(ctx, "value", n_elements);
  {
    grn_id range_id;

    range_id = grn_obj_get_range(ctx, column);

    grn_ctx_output_cstr(ctx, "type");
    grn_obj *range = grn_ctx_at(ctx, range_id);
    command_object_inspect_type(ctx, range);
    grn_obj_unref(ctx, range);
    if (is_index) {
      grn_ctx_output_cstr(ctx, "section");
      grn_ctx_output_bool(ctx, (column_flags & GRN_OBJ_WITH_SECTION) != 0);
      grn_ctx_output_cstr(ctx, "weight");
      grn_ctx_output_bool(ctx, (column_flags & GRN_OBJ_WITH_WEIGHT) != 0);
      grn_ctx_output_cstr(ctx, "position");
      grn_ctx_output_bool(ctx, (column_flags & GRN_OBJ_WITH_POSITION) != 0);
      grn_ctx_output_cstr(ctx, "size");
      if ((column_flags & GRN_OBJ_INDEX_SMALL) != 0) {
        grn_ctx_output_cstr(ctx, "small");
      } else if ((column_flags & GRN_OBJ_INDEX_MEDIUM) != 0) {
        grn_ctx_output_cstr(ctx, "medium");
      } else if ((column_flags & GRN_OBJ_INDEX_LARGE) != 0) {
        grn_ctx_output_cstr(ctx, "large");
      } else {
        grn_ctx_output_cstr(ctx, "normal");
      }
      grn_ctx_output_cstr(ctx, "statistics");
      command_object_inspect_column_index_value_statistics(ctx,
                                                           (grn_ii *)column);
    } else {
      grn_ctx_output_cstr(ctx, "compress");
      command_object_inspect_column_data_value_compress(ctx, column);
      if (is_vector) {
        grn_ctx_output_cstr(ctx, "weight");
        grn_ctx_output_bool(ctx, (column_flags & GRN_OBJ_WITH_WEIGHT) != 0);
        grn_ctx_output_cstr(ctx, "weight_float32");
        grn_ctx_output_bool(ctx, (column_flags & GRN_OBJ_WEIGHT_FLOAT32) != 0);
        grn_ctx_output_cstr(ctx, "weight_bfloat16");
        grn_ctx_output_bool(ctx, (column_flags & GRN_OBJ_WEIGHT_BFLOAT16) != 0);
      }
      grn_ctx_output_cstr(ctx, "compress_filters");
      command_object_inspect_column_data_value_compress_filters(ctx, column);
      if (is_generated_column) {
        grn_ctx_output_cstr(ctx, "generator");
        grn_obj generator;
        GRN_TEXT_INIT(&generator, 0);
        grn_obj_get_info(ctx, column, GRN_INFO_GENERATOR, &generator);
        grn_ctx_output_str(ctx,
                           GRN_TEXT_VALUE(&generator),
                           GRN_TEXT_LEN(&generator));
        GRN_OBJ_FIN(ctx, &generator);
      }
    }
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_column_sources(grn_ctx *ctx, grn_obj *column)
{
  const bool is_generated_column = grn_obj_is_generated_column(ctx, column);
  grn_obj *source_table;
  if (is_generated_column) {
    source_table = grn_ctx_at(ctx, column->header.domain);
  } else {
    source_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));
  }

  grn_obj source_ids;
  GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_obj_get_info(ctx, column, GRN_INFO_SOURCE, &source_ids);

  int n_ids = (int)(GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id));
  grn_ctx_output_array_open(ctx, "sources", n_ids);
  int i;
  for (i = 0; i < n_ids; i++) {
    grn_id source_id;
    grn_obj *source;

    source_id = GRN_RECORD_VALUE_AT(&source_ids, i);
    source = grn_ctx_at(ctx, source_id);

    grn_ctx_output_map_open(ctx, "source", 4);
    {
      grn_ctx_output_cstr(ctx, "id");
      if (grn_obj_is_table(ctx, source)) {
        grn_ctx_output_null(ctx);
      } else {
        grn_ctx_output_uint64(ctx, source_id);
      }

      grn_ctx_output_cstr(ctx, "name");
      if (grn_obj_is_table(ctx, source)) {
        grn_ctx_output_cstr(ctx, "_key");
      } else {
        command_object_inspect_column_name(ctx, source);
      }

      grn_ctx_output_cstr(ctx, "table");
      command_object_inspect_table(ctx, source_table);

      grn_ctx_output_cstr(ctx, "full_name");
      if (grn_obj_is_table(ctx, source)) {
        char name[GRN_TABLE_MAX_KEY_SIZE];
        int name_size;
        name_size = grn_obj_name(ctx, source, name, GRN_TABLE_MAX_KEY_SIZE);
        name[name_size] = '\0';
        grn_strcat(name, GRN_TABLE_MAX_KEY_SIZE, "._key");
        grn_ctx_output_cstr(ctx, name);
      } else {
        command_object_inspect_obj_name(ctx, source);
      }
    }
    grn_ctx_output_map_close(ctx);

    grn_obj_unref(ctx, source);
  }
  grn_ctx_output_array_close(ctx);

  GRN_OBJ_FIN(ctx, &source_ids);

  grn_obj_unref(ctx, source_table);
}

static void
command_object_inspect_column(grn_ctx *ctx, grn_obj *column)
{
  int n_elements = 7;
  bool have_sources = (grn_obj_is_index_column(ctx, column) ||
                       grn_obj_is_token_column(ctx, column) ||
                       grn_obj_is_generated_column(ctx, column));

  if (have_sources) {
    n_elements += 1;
  }
  grn_ctx_output_map_open(ctx, "column", n_elements);
  {
    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint64(ctx, grn_obj_id(ctx, column));
    grn_ctx_output_cstr(ctx, "name");
    command_object_inspect_column_name(ctx, column);
    grn_ctx_output_cstr(ctx, "table");
    grn_obj *table = grn_ctx_at(ctx, column->header.domain);
    command_object_inspect_table(ctx, table);
    grn_obj_unref(ctx, table);
    grn_ctx_output_cstr(ctx, "full_name");
    command_object_inspect_obj_name(ctx, column);
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_column_type(ctx, column);
    grn_ctx_output_cstr(ctx, "value");
    command_object_inspect_column_value(ctx, column);
    if (have_sources) {
      grn_ctx_output_cstr(ctx, "sources");
      command_object_inspect_column_sources(ctx, column);
    }
    grn_ctx_output_cstr(ctx, "disk_usage");
    command_object_inspect_disk_usage(ctx, column);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_db(grn_ctx *ctx, grn_obj *obj)
{
  grn_db *db = (grn_db *)obj;

  grn_ctx_output_map_open(ctx, "database", 3);
  {
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_obj_type(ctx, obj->header.type);
    grn_ctx_output_cstr(ctx, "name_table");
    command_object_inspect_dispatch(ctx, db->keys);
    grn_ctx_output_cstr(ctx, "disk_usage");
    command_object_inspect_disk_usage(ctx, obj);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_dispatch(grn_ctx *ctx, grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_TYPE:
    command_object_inspect_type(ctx, obj);
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    command_object_inspect_table(ctx, obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_INDEX:
    command_object_inspect_column(ctx, obj);
    break;
  case GRN_DB:
    command_object_inspect_db(ctx, obj);
    break;
  default:
    {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_FUNCTION_NOT_IMPLEMENTED,
                       "[object][inspect] unsupported type: <%s>(%#x)",
                       grn_obj_type_to_string(obj->header.type),
                       obj->header.type);
      grn_ctx_output_null(ctx);
      break;
    }
  }
}

static grn_obj *
command_object_inspect(grn_ctx *ctx,
                       int nargs,
                       grn_obj **args,
                       grn_user_data *user_data)
{
  grn_obj *name;
  grn_obj *target;
  bool need_unref = false;

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  if (GRN_TEXT_LEN(name) == 0) {
    target = grn_ctx_db(ctx);
  } else {
    target = grn_ctx_get(ctx, GRN_TEXT_VALUE(name), (int)GRN_TEXT_LEN(name));
    if (!target) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[object][inspect] nonexistent target: <%.*s>",
                       (int)GRN_TEXT_LEN(name),
                       GRN_TEXT_VALUE(name));
      grn_ctx_output_null(ctx);
      return NULL;
    }
    need_unref = true;
  }

  command_object_inspect_dispatch(ctx, target);
  if (need_unref) {
    grn_obj_unref(ctx, target);
  }

  return NULL;
}

void
grn_proc_init_object_inspect(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_command_create(ctx,
                            "object_inspect",
                            -1,
                            command_object_inspect,
                            1,
                            vars);
}
