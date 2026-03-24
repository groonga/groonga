/*
  Copyright (C) 2016  Brazil
  Copyright (C) 2020-2026  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_proc.h"
#include "../grn_db.h"
#include "../grn_vector.h"

#include <groonga/plugin.h>

static void
command_object_list_dump_flags(grn_ctx *ctx, grn_obj_spec *spec)
{
  grn_obj flags;

  GRN_TEXT_INIT(&flags, 0);

  switch (spec->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    grn_dump_table_create_flags(ctx, spec->header.flags, &flags);
    break;
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_INDEX:
    grn_dump_column_create_flags(ctx, spec->header.flags, &flags);
    break;
  case GRN_TYPE:
    if (spec->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
      GRN_TEXT_PUTS(ctx, &flags, "KEY_VAR_SIZE");
    } else {
      switch (spec->header.flags & GRN_OBJ_KEY_MASK) {
      case GRN_OBJ_KEY_UINT:
        GRN_TEXT_PUTS(ctx, &flags, "KEY_UINT");
        break;
      case GRN_OBJ_KEY_INT:
        GRN_TEXT_PUTS(ctx, &flags, "KEY_INT");
        break;
      case GRN_OBJ_KEY_FLOAT:
        GRN_TEXT_PUTS(ctx, &flags, "KEY_FLOAT");
        break;
      case GRN_OBJ_KEY_GEO_POINT:
        GRN_TEXT_PUTS(ctx, &flags, "KEY_GEO_POINT");
        break;
      }
    }
    break;
  }
  if (spec->header.flags & GRN_OBJ_CUSTOM_NAME) {
    if (GRN_TEXT_LEN(&flags) > 0) {
      GRN_TEXT_PUTS(ctx, &flags, "|");
    }
    GRN_TEXT_PUTS(ctx, &flags, "CUSTOM_NAME");
  }

  grn_ctx_output_str(ctx, GRN_TEXT_VALUE(&flags), GRN_TEXT_LEN(&flags));

  GRN_OBJ_FIN(ctx, &flags);
}

static void
command_object_list_output_table_modules(grn_ctx *ctx,
                                         grn_obj *spec_vector,
                                         unsigned int spec_index,
                                         const char *singular,
                                         const char *plural)
{
  const grn_id *ids;
  int n_ids;

  unsigned int n_elements = grn_vector_size(ctx, spec_vector);
  if (n_elements > spec_index) {
    uint32_t element_size;

    element_size = grn_vector_get_element(ctx,
                                          spec_vector,
                                          spec_index,
                                          (const char **)&ids,
                                          NULL,
                                          NULL);
    n_ids = (int)(element_size / sizeof(grn_id));
  } else {
    ids = NULL;
    n_ids = 0;
  }

  grn_ctx_output_cstr(ctx, plural);
  grn_ctx_output_array_open(ctx, plural, n_ids);
  grn_obj *db = grn_ctx_db(ctx);
  for (int i = 0; i < n_ids; i++) {
    grn_id id = ids[i];
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_table_get_key(ctx, db, id, name, GRN_TABLE_MAX_KEY_SIZE);

    grn_ctx_output_map_open(ctx, singular, 2);
    {
      grn_ctx_output_cstr(ctx, "id");
      grn_ctx_output_uint64(ctx, id);

      grn_ctx_output_cstr(ctx, "name");
      if (name_size == 0) {
        grn_ctx_output_null(ctx);
      } else {
        grn_ctx_output_str(ctx, name, (size_t)name_size);
      }
    }
    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_array_close(ctx);
}

static grn_obj *
command_object_list(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_db *db;
  int n_objects = 0;
  grn_obj vector;

  db = (grn_db *)grn_ctx_db(ctx);
  if (!db->specs) {
    grn_ctx_output_map_open(ctx, "objects", n_objects);
    grn_ctx_output_map_close(ctx);
    return NULL;
  }

  GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                             (grn_obj *)db,
                             cursor,
                             id,
                             GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING)
  {
    grn_io_win jw;
    uint32_t value_len;
    char *value;

    value = grn_ja_ref(ctx, db->specs, id, &jw, &value_len);
    if (value) {
      n_objects++;
      grn_ja_unref(ctx, &jw);
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);

  GRN_OBJ_INIT(&vector, GRN_VECTOR, 0, GRN_DB_TEXT);

  grn_ctx_output_map_open(ctx, "objects", n_objects);
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                             (grn_obj *)db,
                             cursor,
                             id,
                             GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING)
  {
    void *name;
    int name_size;
    grn_io_win jw;
    uint32_t value_len;
    char *value;
    unsigned int n_elements;

    value = grn_ja_ref(ctx, db->specs, id, &jw, &value_len);
    if (!value) {
      continue;
    }

    name_size = grn_table_cursor_get_key(ctx, cursor, &name);

    grn_ctx_output_str(ctx, name, (size_t)name_size);

    GRN_BULK_REWIND(&vector);
    grn_rc rc = grn_vector_unpack(ctx, &vector, value, value_len, 0, NULL);
    if (rc != GRN_SUCCESS) {
      grn_ctx_output_map_open(ctx, "object", 4);
      {
        grn_ctx_output_cstr(ctx, "id");
        grn_ctx_output_int64(ctx, id);
        grn_ctx_output_cstr(ctx, "name");
        grn_ctx_output_str(ctx, name, (size_t)name_size);
        grn_ctx_output_cstr(ctx, "opened");
        grn_ctx_output_bool(ctx, grn_ctx_is_opened(ctx, id));
        grn_ctx_output_cstr(ctx, "value_size");
        grn_ctx_output_uint64(ctx, value_len);
      }
      grn_ctx_output_map_close(ctx);
      goto next;
    }

    n_elements = grn_vector_size(ctx, &vector);

    {
      uint32_t element_size;
      grn_obj_spec *spec;
      int n_properties = 8;
      bool need_sources = false;
      bool need_generator = false;
      bool need_token_filters = false;
      bool need_normalizers = false;
      bool need_extractors = false;

      element_size = grn_vector_get_element(ctx,
                                            &vector,
                                            GRN_SERIALIZED_SPEC_INDEX_SPEC,
                                            (const char **)&spec,
                                            NULL,
                                            NULL);
      if (element_size == 0) {
        grn_ctx_output_map_open(ctx, "object", 4);
        {
          grn_ctx_output_cstr(ctx, "id");
          grn_ctx_output_int64(ctx, id);
          grn_ctx_output_cstr(ctx, "name");
          grn_ctx_output_str(ctx, name, (size_t)name_size);
          grn_ctx_output_cstr(ctx, "opened");
          grn_ctx_output_bool(ctx, grn_ctx_is_opened(ctx, id));
          grn_ctx_output_cstr(ctx, "n_elements");
          grn_ctx_output_uint64(ctx, n_elements);
        }
        grn_ctx_output_map_close(ctx);
        goto next;
      }

      switch (spec->header.type) {
      case GRN_COLUMN_FIX_SIZE:
      case GRN_COLUMN_VAR_SIZE:
        need_sources = true;
        n_properties++;
        need_generator = true;
        n_properties++;
        break;
      case GRN_COLUMN_INDEX:
        need_sources = true;
        n_properties++;
        break;
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
      case GRN_TABLE_HASH_KEY:
        need_token_filters = true;
        n_properties++;
        need_normalizers = true;
        n_properties++;
        need_extractors = true;
        n_properties++;
        break;
      case GRN_TABLE_NO_KEY:
        break;
      }
      grn_ctx_output_map_open(ctx, "object", n_properties);
      {
        grn_ctx_output_cstr(ctx, "id");
        grn_ctx_output_uint64(ctx, id);

        grn_ctx_output_cstr(ctx, "name");
        grn_ctx_output_str(ctx, name, (size_t)name_size);

        grn_ctx_output_cstr(ctx, "opened");
        grn_ctx_output_bool(ctx, grn_ctx_is_opened(ctx, id));

        grn_ctx_output_cstr(ctx, "n_elements");
        grn_ctx_output_uint64(ctx, n_elements);

        grn_ctx_output_cstr(ctx, "type");
        grn_ctx_output_map_open(ctx, "type", 2);
        {
          grn_ctx_output_cstr(ctx, "id");
          grn_ctx_output_uint64(ctx, spec->header.type);
          grn_ctx_output_cstr(ctx, "name");
          grn_ctx_output_cstr(ctx, grn_obj_type_to_string(spec->header.type));
        }
        grn_ctx_output_map_close(ctx);

        grn_ctx_output_cstr(ctx, "flags");
        grn_ctx_output_map_open(ctx, "flags", 2);
        {
          grn_ctx_output_cstr(ctx, "value");
          grn_ctx_output_uint64(ctx, spec->header.flags);
          grn_ctx_output_cstr(ctx, "names");
          command_object_list_dump_flags(ctx, spec);
        }
        grn_ctx_output_map_close(ctx);

        grn_ctx_output_cstr(ctx, "path");
        char path[PATH_MAX];
        grn_obj_spec_get_path(ctx, spec, id, path, db, &vector);
        size_t path_size = strlen(path);
        if (path_size == 0) {
          grn_ctx_output_null(ctx);
        } else {
          grn_ctx_output_str(ctx, path, path_size);
        }

        switch (spec->header.type) {
        case GRN_TYPE:
          grn_ctx_output_cstr(ctx, "size");
          grn_ctx_output_uint64(ctx, spec->range);
          break;
        case GRN_PROC:
          grn_ctx_output_cstr(ctx, "plugin_id");
          grn_ctx_output_uint64(ctx, spec->range);
          break;
        default:
          grn_ctx_output_cstr(ctx, "range");
          grn_ctx_output_map_open(ctx, "range", 2);
          {
            char name[GRN_TABLE_MAX_KEY_SIZE];
            int name_size;

            name_size = grn_table_get_key(ctx,
                                          (grn_obj *)db,
                                          spec->range,
                                          name,
                                          GRN_TABLE_MAX_KEY_SIZE);

            grn_ctx_output_cstr(ctx, "id");
            grn_ctx_output_uint64(ctx, spec->range);

            grn_ctx_output_cstr(ctx, "name");
            if (name_size == 0) {
              grn_ctx_output_null(ctx);
            } else {
              grn_ctx_output_str(ctx, name, (size_t)name_size);
            }
          }
          grn_ctx_output_map_close(ctx);
          break;
        }

        if (need_sources) {
          const grn_id *source_ids;
          int n_source_ids;
          int i;

          if (n_elements > GRN_SERIALIZED_SPEC_INDEX_SOURCE) {
            uint32_t element_size;

            element_size =
              grn_vector_get_element(ctx,
                                     &vector,
                                     GRN_SERIALIZED_SPEC_INDEX_SOURCE,
                                     (const char **)&source_ids,
                                     NULL,
                                     NULL);
            n_source_ids = (int)(element_size / sizeof(grn_id));
          } else {
            source_ids = NULL;
            n_source_ids = 0;
          }

          grn_ctx_output_cstr(ctx, "sources");
          grn_ctx_output_array_open(ctx, "sources", n_source_ids);
          for (i = 0; i < n_source_ids; i++) {
            grn_id source_id;
            char name[GRN_TABLE_MAX_KEY_SIZE];
            int name_size;

            source_id = source_ids[i];
            name_size = grn_table_get_key(ctx,
                                          (grn_obj *)db,
                                          source_id,
                                          name,
                                          GRN_TABLE_MAX_KEY_SIZE);

            grn_ctx_output_map_open(ctx, "source", 2);
            {
              grn_ctx_output_cstr(ctx, "id");
              grn_ctx_output_uint64(ctx, source_id);

              grn_ctx_output_cstr(ctx, "name");
              if (name_size == 0) {
                grn_ctx_output_null(ctx);
              } else {
                grn_ctx_output_str(ctx, name, (size_t)name_size);
              }
            }
            grn_ctx_output_map_close(ctx);
          }
          grn_ctx_output_array_close(ctx);
        }

        if (need_generator) {
          grn_raw_string generator;
          GRN_RAW_STRING_INIT(generator);
          if (n_elements > GRN_SERIALIZED_SPEC_INDEX_JA_GENERATOR) {
            generator.length =
              grn_vector_get_element(ctx,
                                     &vector,
                                     GRN_SERIALIZED_SPEC_INDEX_JA_GENERATOR,
                                     &(generator.value),
                                     NULL,
                                     NULL);
          }

          grn_ctx_output_cstr(ctx, "generator");
          grn_ctx_output_str(ctx, generator.value, generator.length);
        }

        if (need_token_filters) {
          command_object_list_output_table_modules(
            ctx,
            &vector,
            GRN_SERIALIZED_SPEC_INDEX_TOKEN_FILTERS,
            "token_filter",
            "token_filters");
        }

        if (need_normalizers) {
          command_object_list_output_table_modules(
            ctx,
            &vector,
            GRN_SERIALIZED_SPEC_INDEX_NORMALIZERS,
            "normalizer",
            "normalizers");
        }

        if (need_extractors) {
          command_object_list_output_table_modules(
            ctx,
            &vector,
            GRN_SERIALIZED_SPEC_INDEX_EXTRACTORS,
            "extractor",
            "extractors");
        }
      }
      grn_ctx_output_map_close(ctx);
    }

  next:
    grn_ja_unref(ctx, &jw);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &vector);

  return NULL;
}

void
grn_proc_init_object_list(grn_ctx *ctx)
{
  grn_plugin_command_create(ctx,
                            "object_list",
                            -1,
                            command_object_list,
                            0,
                            NULL);
}
