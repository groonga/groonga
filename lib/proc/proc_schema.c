/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015-2016 Brazil

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

#include "../grn_proc.h"

#include "../grn_db.h"

#include <groonga/plugin.h>

static void
command_schema_output_name(grn_ctx *ctx, grn_obj *obj)
{
  if (obj) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    unsigned int name_size;
    name_size = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
    grn_ctx_output_str(ctx, name, name_size);
  } else {
    grn_ctx_output_null(ctx);
  }
}

static void
command_schema_output_column_name(grn_ctx *ctx, grn_obj *column)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  unsigned int name_size;
  name_size = grn_column_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
  grn_ctx_output_str(ctx, name, name_size);
}

static void
command_schema_output_type(grn_ctx *ctx, const char *type_label, grn_obj *type)
{
  if (!type) {
    grn_ctx_output_null(ctx);
    return;
  }

  grn_ctx_output_map_open(ctx, type_label, 2);

  grn_ctx_output_cstr(ctx, "name");
  command_schema_output_name(ctx, type);

  grn_ctx_output_cstr(ctx, "type");
  if (grn_obj_is_table(ctx, type)) {
    grn_ctx_output_cstr(ctx, "reference");
  } else {
    grn_ctx_output_cstr(ctx, "type");
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_schema_output_key_type(grn_ctx *ctx, grn_obj *key_type)
{
  command_schema_output_type(ctx, "key_type", key_type);
}

static void
command_schema_output_value_type(grn_ctx *ctx, grn_obj *value_type)
{
  command_schema_output_type(ctx, "value_type", value_type);
}

static void
command_schema_output_command(grn_ctx *ctx,
                           const char *command_name,
                           grn_obj *arguments)
{
  grn_ctx_output_map_open(ctx, "command", 3);

  grn_ctx_output_cstr(ctx, "name");
  grn_ctx_output_cstr(ctx, command_name);

  grn_ctx_output_cstr(ctx, "arguments");
  {
    int i, n;

    n = grn_vector_size(ctx, arguments);
    grn_ctx_output_map_open(ctx, "arguments", n / 2);
    for (i = 0; i < n; i += 2) {
      const char *name;
      unsigned int name_size;
      const char *value;
      unsigned int value_size;

      name_size  = grn_vector_get_element(ctx, arguments, i, &name,
                                          NULL, NULL);
      value_size = grn_vector_get_element(ctx, arguments, i + 1, &value,
                                          NULL, NULL);
      grn_ctx_output_str(ctx, name, name_size);
      grn_ctx_output_str(ctx, value, value_size);
    }
    grn_ctx_output_map_close(ctx);
  }

  grn_ctx_output_cstr(ctx, "command_line");
  {
    int i, n;
    grn_obj command_line;

    GRN_TEXT_INIT(&command_line, 0);
    GRN_TEXT_PUTS(ctx, &command_line, command_name);
    n = grn_vector_size(ctx, arguments);
    for (i = 0; i < n; i += 2) {
      const char *name;
      unsigned int name_size;
      const char *value;
      unsigned int value_size;

      name_size  = grn_vector_get_element(ctx, arguments, i, &name,
                                          NULL, NULL);
      value_size = grn_vector_get_element(ctx, arguments, i + 1, &value,
                                          NULL, NULL);
      grn_text_printf(ctx, &command_line,
                      " --%.*s %.*s",
                      name_size, name,
                      value_size, value);
    }
    grn_ctx_output_str(ctx,
                       GRN_TEXT_VALUE(&command_line),
                       GRN_TEXT_LEN(&command_line));
    GRN_OBJ_FIN(ctx, &command_line);
  }

  grn_ctx_output_map_close(ctx);
}

static void
command_schema_output_plugins(grn_ctx *ctx)
{
  grn_obj plugin_names;
  unsigned int i, n;

  GRN_TEXT_INIT(&plugin_names, GRN_OBJ_VECTOR);

  grn_plugin_get_names(ctx, &plugin_names);

  grn_ctx_output_cstr(ctx, "plugins");

  n = grn_vector_size(ctx, &plugin_names);
  grn_ctx_output_map_open(ctx, "plugins", n);
  for (i = 0; i < n; i++) {
    const char *name;
    unsigned int name_size;

    name_size = grn_vector_get_element(ctx, &plugin_names, i, &name, NULL, NULL);
    grn_ctx_output_str(ctx, name, name_size);

    grn_ctx_output_map_open(ctx, "plugin", 1);
    grn_ctx_output_cstr(ctx, "name");
    grn_ctx_output_str(ctx, name, name_size);
    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &plugin_names);
}

static void
command_schema_output_types(grn_ctx *ctx)
{
  grn_obj types;
  unsigned int i, n;

  GRN_PTR_INIT(&types, GRN_OBJ_VECTOR, GRN_DB_OBJECT);

  grn_ctx_get_all_types(ctx, &types);

  grn_ctx_output_cstr(ctx, "types");

  n = GRN_BULK_VSIZE(&types) / sizeof(grn_obj *);
  grn_ctx_output_map_open(ctx, "types", n);
  for (i = 0; i < n; i++) {
    grn_obj *type;

    type = GRN_PTR_VALUE_AT(&types, i);

    command_schema_output_name(ctx, type);

    grn_ctx_output_map_open(ctx, "type", 4);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_name(ctx, type);

    grn_ctx_output_cstr(ctx, "size");
    grn_ctx_output_int64(ctx, grn_type_size(ctx, type));

    grn_ctx_output_cstr(ctx, "can_be_key_type");
    grn_ctx_output_bool(ctx, grn_type_size(ctx, type) <= GRN_TABLE_MAX_KEY_SIZE);

    grn_ctx_output_cstr(ctx, "can_be_value_type");
    grn_ctx_output_bool(ctx, !(type->header.flags & GRN_OBJ_KEY_VAR_SIZE));

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &types);
}

static void
command_schema_output_tokenizers(grn_ctx *ctx)
{
  grn_obj tokenizers;
  unsigned int i, n;

  GRN_PTR_INIT(&tokenizers, GRN_OBJ_VECTOR, GRN_DB_OBJECT);

  grn_ctx_get_all_tokenizers(ctx, &tokenizers);

  grn_ctx_output_cstr(ctx, "tokenizers");

  n = GRN_BULK_VSIZE(&tokenizers) / sizeof(grn_obj *);
  grn_ctx_output_map_open(ctx, "tokenizers", n);
  for (i = 0; i < n; i++) {
    grn_obj *tokenizer;

    tokenizer = GRN_PTR_VALUE_AT(&tokenizers, i);

    command_schema_output_name(ctx, tokenizer);

    grn_ctx_output_map_open(ctx, "tokenizer", 1);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_name(ctx, tokenizer);

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &tokenizers);
}

static void
command_schema_output_normalizers(grn_ctx *ctx)
{
  grn_obj normalizers;
  unsigned int i, n;

  GRN_PTR_INIT(&normalizers, GRN_OBJ_VECTOR, GRN_DB_OBJECT);

  grn_ctx_get_all_normalizers(ctx, &normalizers);

  grn_ctx_output_cstr(ctx, "normalizers");

  n = GRN_BULK_VSIZE(&normalizers) / sizeof(grn_obj *);
  grn_ctx_output_map_open(ctx, "normalizers", n);
  for (i = 0; i < n; i++) {
    grn_obj *normalizer;

    normalizer = GRN_PTR_VALUE_AT(&normalizers, i);

    command_schema_output_name(ctx, normalizer);

    grn_ctx_output_map_open(ctx, "normalizer", 1);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_name(ctx, normalizer);

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &normalizers);
}

static void
command_schema_output_token_filters(grn_ctx *ctx)
{
  grn_obj token_filters;
  int i, n;

  GRN_PTR_INIT(&token_filters, GRN_OBJ_VECTOR, GRN_DB_OBJECT);

  grn_ctx_get_all_token_filters(ctx, &token_filters);

  grn_ctx_output_cstr(ctx, "token_filters");

  n = GRN_BULK_VSIZE(&token_filters) / sizeof(grn_obj *);
  grn_ctx_output_map_open(ctx, "token_filters", n);
  for (i = 0; i < n; i++) {
    grn_obj *token_filter;

    token_filter = GRN_PTR_VALUE_AT(&token_filters, i);

    command_schema_output_name(ctx, token_filter);

    grn_ctx_output_map_open(ctx, "token_filter", 1);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_name(ctx, token_filter);

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &token_filters);
}

static const char *
command_schema_table_type_name(grn_ctx *ctx, grn_obj *table)
{
  const char *name = "unknown";

  switch (table->header.type) {
  case GRN_TABLE_NO_KEY :
    name = "array";
    break;
  case GRN_TABLE_HASH_KEY :
    name = "hash table";
    break;
  case GRN_TABLE_PAT_KEY :
    name = "patricia trie";
    break;
  case GRN_TABLE_DAT_KEY :
    name = "double array trie";
    break;
  default :
    break;
  }

  return name;
}

static void
command_schema_table_output_key_type(grn_ctx *ctx, grn_obj *table)
{
  grn_obj *key_type = NULL;

  if (table->header.type != GRN_TABLE_NO_KEY &&
      table->header.domain != GRN_ID_NIL) {
    key_type = grn_ctx_at(ctx, table->header.domain);
  }

  command_schema_output_key_type(ctx, key_type);
}

static void
command_schema_table_output_value_type(grn_ctx *ctx, grn_obj *table)
{
  grn_obj *value_type = NULL;
  grn_id range = GRN_ID_NIL;

  if (table->header.type != GRN_TABLE_DAT_KEY) {
    range = grn_obj_get_range(ctx, table);
  }
  if (range != GRN_ID_NIL) {
    value_type = grn_ctx_at(ctx, range);
  }

  command_schema_output_value_type(ctx, value_type);
}

static void
command_schema_table_output_tokenizer(grn_ctx *ctx, grn_obj *table)
{
  grn_obj *tokenizer;

  tokenizer = grn_obj_get_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER, NULL);
  if (!tokenizer) {
    grn_ctx_output_null(ctx);
    return;
  }

  grn_ctx_output_map_open(ctx, "tokenizer", 1);

  grn_ctx_output_cstr(ctx, "name");
  command_schema_output_name(ctx, tokenizer);

  grn_ctx_output_map_close(ctx);
}

static void
command_schema_table_output_normalizer(grn_ctx *ctx, grn_obj *table)
{
  grn_obj *normalizer;

  normalizer = grn_obj_get_info(ctx, table, GRN_INFO_NORMALIZER, NULL);
  if (!normalizer) {
    grn_ctx_output_null(ctx);
    return;
  }

  grn_ctx_output_map_open(ctx, "normalizer", 1);

  grn_ctx_output_cstr(ctx, "name");
  command_schema_output_name(ctx, normalizer);

  grn_ctx_output_map_close(ctx);
}

static void
command_schema_table_output_token_filters(grn_ctx *ctx, grn_obj *table)
{
  grn_obj token_filters;
  int i, n;

  GRN_PTR_INIT(&token_filters, GRN_OBJ_VECTOR, GRN_DB_OBJECT);
  if (table->header.type != GRN_TABLE_NO_KEY) {
    grn_obj_get_info(ctx, table, GRN_INFO_TOKEN_FILTERS, &token_filters);
  }

  n = GRN_BULK_VSIZE(&token_filters) / sizeof(grn_obj *);
  grn_ctx_output_array_open(ctx, "token_filters", n);
  for (i = 0; i < n; i++) {
    grn_obj *token_filter;

    token_filter = GRN_PTR_VALUE_AT(&token_filters, i);

    grn_ctx_output_map_open(ctx, "token_filter", 1);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_name(ctx, token_filter);

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_array_close(ctx);

  GRN_OBJ_FIN(ctx, &token_filters);
}

static void
command_schema_table_command_collect_arguments(grn_ctx *ctx,
                                               grn_obj *table,
                                               grn_obj *arguments)
{
#define ADD(name_, value_)                              \
  grn_vector_add_element(ctx, arguments,                \
                         name_, strlen(name_),          \
                         0, GRN_DB_TEXT);               \
  grn_vector_add_element(ctx, arguments,                \
                         value_, strlen(value_),        \
                         0, GRN_DB_TEXT)

#define ADD_OBJECT_NAME(name_, object_) do {                    \
    char object_name[GRN_TABLE_MAX_KEY_SIZE];                   \
    unsigned int object_name_size;                              \
    object_name_size = grn_obj_name(ctx, object_,               \
                                    object_name,                \
                                    GRN_TABLE_MAX_KEY_SIZE);    \
    object_name[object_name_size] = '\0';                       \
    ADD(name_, object_name);                                    \
  } while (GRN_FALSE)

  ADD_OBJECT_NAME("name", table);

  {
    grn_obj flags;
    grn_table_flags table_flags;
    grn_table_flags ignored_flags = GRN_OBJ_KEY_NORMALIZE | GRN_OBJ_PERSISTENT;
    GRN_TEXT_INIT(&flags, 0);
    grn_table_get_info(ctx, table, &table_flags, NULL, NULL, NULL, NULL);
    grn_dump_table_create_flags(ctx,
                                table_flags & ~ignored_flags,
                                &flags);
    GRN_TEXT_PUTC(ctx, &flags, '\0');
    ADD("flags", GRN_TEXT_VALUE(&flags));
    GRN_OBJ_FIN(ctx, &flags);
  }

  {
    grn_obj *key_type = NULL;

    if (table->header.type != GRN_TABLE_NO_KEY &&
        table->header.domain != GRN_ID_NIL) {
      key_type = grn_ctx_at(ctx, table->header.domain);
    }
    if (key_type) {
      ADD_OBJECT_NAME("key_type", key_type);
    }
  }

  {
    grn_obj *value_type = NULL;
    grn_id range = GRN_ID_NIL;

    if (table->header.type != GRN_TABLE_DAT_KEY) {
      range = grn_obj_get_range(ctx, table);
    }
    if (range != GRN_ID_NIL) {
      value_type = grn_ctx_at(ctx, range);
    }
    if (value_type) {
      ADD_OBJECT_NAME("value_type", value_type);
    }
  }

  {
    grn_obj *tokenizer;
    tokenizer = grn_obj_get_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER, NULL);
    if (tokenizer) {
      ADD_OBJECT_NAME("default_tokenizer", tokenizer);
    }
  }

  {
    grn_obj *normalizer;
    normalizer = grn_obj_get_info(ctx, table, GRN_INFO_NORMALIZER, NULL);
    if (!normalizer && (table->header.flags & GRN_OBJ_KEY_NORMALIZE)) {
      normalizer = grn_ctx_get(ctx, "NormalizerAuto", -1);
    }
    if (normalizer) {
      ADD_OBJECT_NAME("normalizer", normalizer);
    }
  }

  if (table->header.type != GRN_TABLE_NO_KEY) {
    grn_obj token_filters;
    int n;

    GRN_PTR_INIT(&token_filters, GRN_OBJ_VECTOR, GRN_DB_OBJECT);
    grn_obj_get_info(ctx, table, GRN_INFO_TOKEN_FILTERS, &token_filters);
    n = GRN_BULK_VSIZE(&token_filters) / sizeof(grn_obj *);
    if (n > 0) {
      grn_obj token_filter_names;
      int i;

      GRN_TEXT_INIT(&token_filter_names, 0);
      for (i = 0; i < n; i++) {
        grn_obj *token_filter;
        char name[GRN_TABLE_MAX_KEY_SIZE];
        int name_size;

        token_filter = GRN_PTR_VALUE_AT(&token_filters, i);
        name_size = grn_obj_name(ctx, token_filter,
                                 name, GRN_TABLE_MAX_KEY_SIZE);
        if (i > 0) {
          GRN_TEXT_PUTC(ctx, &token_filter_names, ',');
        }
        GRN_TEXT_PUT(ctx, &token_filter_names, name, name_size);
      }
      GRN_TEXT_PUTC(ctx, &token_filter_names, '\0');
      ADD("token_filters", GRN_TEXT_VALUE(&token_filter_names));
      GRN_OBJ_FIN(ctx, &token_filter_names);
    }
    GRN_OBJ_FIN(ctx, &token_filters);
  }

#undef ADD_OBJECT_NAME
#undef ADD
}

static void
command_schema_table_output_command(grn_ctx *ctx, grn_obj *table)
{
  grn_obj arguments;

  GRN_TEXT_INIT(&arguments, GRN_OBJ_VECTOR);
  command_schema_table_command_collect_arguments(ctx, table, &arguments);

  command_schema_output_command(ctx, "table_create", &arguments);

  GRN_OBJ_FIN(ctx, &arguments);
}

static void
command_schema_column_output_type(grn_ctx *ctx, grn_obj *column)
{
  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
    switch (column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
    case GRN_OBJ_COLUMN_SCALAR :
      grn_ctx_output_cstr(ctx, "scalar");
      break;
    case GRN_OBJ_COLUMN_VECTOR :
      grn_ctx_output_cstr(ctx, "vector");
      break;
    }
    break;
  case GRN_COLUMN_INDEX :
    grn_ctx_output_cstr(ctx, "index");
    break;
  }
}

static void
command_schema_column_output_value_type(grn_ctx *ctx, grn_obj *column)
{
  grn_obj *value_type;
  value_type = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));
  command_schema_output_value_type(ctx, value_type);
}

static void
command_schema_column_output_compress(grn_ctx *ctx, grn_obj *column)
{
  const char *compress = NULL;

  if (column->header.type != GRN_COLUMN_INDEX) {
    if (column->header.flags & GRN_OBJ_COMPRESS_ZLIB) {
      compress = "zlib";
    } else if (column->header.flags & GRN_OBJ_COMPRESS_LZ4) {
      compress = "lz4";
    }
  }

  if (compress) {
    grn_ctx_output_cstr(ctx, compress);
  } else {
    grn_ctx_output_null(ctx);
  }
}

static void
command_schema_column_output_sources(grn_ctx *ctx, grn_obj *column)
{
  grn_obj *source_table;
  grn_obj source_ids;
  unsigned int i, n_ids;

  source_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));

  GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);

  if (column->header.type == GRN_COLUMN_INDEX) {
    grn_obj_get_info(ctx, column, GRN_INFO_SOURCE, &source_ids);
  }

  n_ids = GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id);
  grn_ctx_output_array_open(ctx, "sources", n_ids);
  for (i = 0; i < n_ids; i++) {
    grn_id source_id;
    grn_obj *source;

    source_id = GRN_RECORD_VALUE_AT(&source_ids, i);
    source = grn_ctx_at(ctx, source_id);

    grn_ctx_output_map_open(ctx, "source", 3);

    grn_ctx_output_cstr(ctx, "name");
    if (grn_obj_is_table(ctx, source)) {
      grn_ctx_output_cstr(ctx, "_key");
    } else {
      command_schema_output_column_name(ctx, source);
    }

    grn_ctx_output_cstr(ctx, "table");
    command_schema_output_name(ctx, source_table);

    grn_ctx_output_cstr(ctx, "full_name");
    if (grn_obj_is_table(ctx, source)) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      unsigned int name_size;
      name_size = grn_obj_name(ctx, source, name, GRN_TABLE_MAX_KEY_SIZE);
      name[name_size] = '\0';
      grn_strcat(name, GRN_TABLE_MAX_KEY_SIZE, "._key");
      grn_ctx_output_cstr(ctx, name);
    } else {
      command_schema_output_name(ctx, source);
    }

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_array_close(ctx);

  GRN_OBJ_FIN(ctx, &source_ids);
}

static void
command_schema_column_output_indexes(grn_ctx *ctx, grn_obj *column)
{
  uint32_t i;
  grn_index_datum *index_data = NULL;
  uint32_t n_index_data = 0;

  if (column) {
    n_index_data = grn_column_get_all_index_data(ctx, column, NULL, 0);
    if (n_index_data > 0) {
      index_data = GRN_PLUGIN_MALLOC(ctx,
                                     sizeof(grn_index_datum) * n_index_data);
      if (!index_data) {
        GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                         "[schema] failed to allocate memory for indexes");
        return;
      }
      grn_column_get_all_index_data(ctx, column, index_data, n_index_data);
    }
  }

  grn_ctx_output_array_open(ctx, "indexes", n_index_data);
  for (i = 0; i < n_index_data; i++) {
    grn_obj *lexicon;

    grn_ctx_output_map_open(ctx, "index", 4);

    grn_ctx_output_cstr(ctx, "full_name");
    command_schema_output_name(ctx, index_data[i].index);

    grn_ctx_output_cstr(ctx, "table");
    lexicon = grn_ctx_at(ctx, index_data[i].index->header.domain);
    command_schema_output_name(ctx, lexicon);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_column_name(ctx, index_data[i].index);

    grn_ctx_output_cstr(ctx, "section");
    grn_ctx_output_uint64(ctx, index_data[i].section);

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_array_close(ctx);

  if (index_data) {
    GRN_PLUGIN_FREE(ctx, index_data);
  }
}

static void
command_schema_column_command_collect_arguments(grn_ctx *ctx,
                                             grn_obj *table,
                                             grn_obj *column,
                                             grn_obj *arguments)
{
#define ADD(name_, value_)                              \
  grn_vector_add_element(ctx, arguments,                \
                         name_, strlen(name_),          \
                         0, GRN_DB_TEXT);               \
  grn_vector_add_element(ctx, arguments,                \
                         value_, strlen(value_),        \
                         0, GRN_DB_TEXT)

#define ADD_OBJECT_NAME(name_, object_) do {                    \
    char object_name[GRN_TABLE_MAX_KEY_SIZE];                   \
    unsigned int object_name_size;                              \
    object_name_size = grn_obj_name(ctx, object_,               \
                                    object_name,                \
                                    GRN_TABLE_MAX_KEY_SIZE);    \
    object_name[object_name_size] = '\0';                       \
    ADD(name_, object_name);                                    \
  } while (GRN_FALSE)

  ADD_OBJECT_NAME("table", table);
  {
    char column_name[GRN_TABLE_MAX_KEY_SIZE];
    unsigned int column_name_size;
    column_name_size = grn_column_name(ctx, column,
                                       column_name, GRN_TABLE_MAX_KEY_SIZE);
    column_name[column_name_size] = '\0';
    ADD("name", column_name);
  }

  {
    grn_obj flags;
    grn_column_flags column_flags;

    GRN_TEXT_INIT(&flags, 0);
    column_flags = grn_column_get_flags(ctx, column);
    grn_dump_column_create_flags(ctx,
                                 column_flags & ~GRN_OBJ_PERSISTENT,
                                 &flags);
    GRN_TEXT_PUTC(ctx, &flags, '\0');
    ADD("flags", GRN_TEXT_VALUE(&flags));
    GRN_OBJ_FIN(ctx, &flags);
  }

  {
    grn_obj *value_type;

    value_type = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));
    ADD_OBJECT_NAME("type", value_type);
  }

  if (column->header.type == GRN_COLUMN_INDEX) {
    grn_obj source_ids;
    unsigned int n_ids;

    GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_obj_get_info(ctx, column, GRN_INFO_SOURCE, &source_ids);

    n_ids = GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id);
    if (n_ids > 0) {
      grn_obj sources;
      unsigned int i;

      GRN_TEXT_INIT(&sources, 0);
      for (i = 0; i < n_ids; i++) {
        grn_id source_id;
        grn_obj *source;
        char name[GRN_TABLE_MAX_KEY_SIZE];
        unsigned int name_size;

        source_id = GRN_RECORD_VALUE_AT(&source_ids, i);
        source = grn_ctx_at(ctx, source_id);

        if (grn_obj_is_table(ctx, source)) {
          grn_strcpy(name, GRN_TABLE_MAX_KEY_SIZE, "_key");
          name_size = strlen(name);
        } else {
          name_size = grn_column_name(ctx, source, name, GRN_TABLE_MAX_KEY_SIZE);
        }
        if (i > 0) {
          GRN_TEXT_PUTC(ctx, &sources, ',');
        }
        GRN_TEXT_PUT(ctx, &sources, name, name_size);
      }
      GRN_TEXT_PUTC(ctx, &sources, '\0');
      ADD("sources", GRN_TEXT_VALUE(&sources));
      GRN_OBJ_FIN(ctx, &sources);
    }
    GRN_OBJ_FIN(ctx, &source_ids);
  }

#undef ADD_OBJECT_NAME
#undef ADD
}

static void
command_schema_column_output_command(grn_ctx *ctx, grn_obj *table, grn_obj *column)
{
  grn_obj arguments;

  GRN_TEXT_INIT(&arguments, GRN_OBJ_VECTOR);
  command_schema_column_command_collect_arguments(ctx, table, column, &arguments);

  command_schema_output_command(ctx, "column_create", &arguments);

  GRN_OBJ_FIN(ctx, &arguments);
}

static void
command_schema_column_output(grn_ctx *ctx, grn_obj *table, grn_obj *column)
{
  if (!column) {
    return;
  }

  command_schema_output_column_name(ctx, column);

  grn_ctx_output_map_open(ctx, "column", 12);

  grn_ctx_output_cstr(ctx, "name");
  command_schema_output_column_name(ctx, column);

  grn_ctx_output_cstr(ctx, "table");
  command_schema_output_name(ctx, table);

  grn_ctx_output_cstr(ctx, "full_name");
  command_schema_output_name(ctx, column);

  grn_ctx_output_cstr(ctx, "type");
  command_schema_column_output_type(ctx, column);

  grn_ctx_output_cstr(ctx, "value_type");
  command_schema_column_output_value_type(ctx, column);

  grn_ctx_output_cstr(ctx, "compress");
  command_schema_column_output_compress(ctx, column);

  grn_ctx_output_cstr(ctx, "section");
  grn_ctx_output_bool(ctx, (column->header.flags & GRN_OBJ_WITH_SECTION) != 0);

  grn_ctx_output_cstr(ctx, "weight");
  grn_ctx_output_bool(ctx, (column->header.flags & GRN_OBJ_WITH_WEIGHT) != 0);

  grn_ctx_output_cstr(ctx, "position");
  grn_ctx_output_bool(ctx, (column->header.flags & GRN_OBJ_WITH_POSITION) != 0);

  grn_ctx_output_cstr(ctx, "sources");
  command_schema_column_output_sources(ctx, column);

  grn_ctx_output_cstr(ctx, "indexes");
  command_schema_column_output_indexes(ctx, column);

  grn_ctx_output_cstr(ctx, "command");
  command_schema_column_output_command(ctx, table, column);

  grn_ctx_output_map_close(ctx);
}

static void
command_schema_table_output_columns(grn_ctx *ctx, grn_obj *table)
{
  grn_hash *columns;

  columns = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                            GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    grn_ctx_output_map_open(ctx, "columns", 0);
    grn_ctx_output_map_close(ctx);
    return;
  }

  grn_table_columns(ctx, table, "", 0, (grn_obj *)columns);
  grn_ctx_output_map_open(ctx, "columns", grn_hash_size(ctx, columns));
  {
    grn_id *key;
    GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
      grn_obj *column;
      column = grn_ctx_at(ctx, *key);
      command_schema_column_output(ctx, table, column);
    });
  }
  grn_ctx_output_map_close(ctx);
  grn_hash_close(ctx, columns);
}

static void
command_schema_output_tables(grn_ctx *ctx)
{
  grn_obj tables;
  unsigned int i, n;

  GRN_PTR_INIT(&tables, GRN_OBJ_VECTOR, GRN_DB_OBJECT);

  grn_ctx_get_all_tables(ctx, &tables);

  grn_ctx_output_cstr(ctx, "tables");

  n = GRN_BULK_VSIZE(&tables) / sizeof(grn_obj *);
  grn_ctx_output_map_open(ctx, "tables", n);
  for (i = 0; i < n; i++) {
    grn_obj *table;

    table = GRN_PTR_VALUE_AT(&tables, i);

    command_schema_output_name(ctx, table);

    grn_ctx_output_map_open(ctx, "table", 10);

    grn_ctx_output_cstr(ctx, "name");
    command_schema_output_name(ctx, table);

    grn_ctx_output_cstr(ctx, "type");
    grn_ctx_output_cstr(ctx, command_schema_table_type_name(ctx, table));

    grn_ctx_output_cstr(ctx, "key_type");
    command_schema_table_output_key_type(ctx, table);

    grn_ctx_output_cstr(ctx, "value_type");
    command_schema_table_output_value_type(ctx, table);

    grn_ctx_output_cstr(ctx, "tokenizer");
    command_schema_table_output_tokenizer(ctx, table);

    grn_ctx_output_cstr(ctx, "normalizer");
    command_schema_table_output_normalizer(ctx, table);

    grn_ctx_output_cstr(ctx, "token_filters");
    command_schema_table_output_token_filters(ctx, table);

    grn_ctx_output_cstr(ctx, "indexes");
    if (table->header.type == GRN_TABLE_NO_KEY) {
      command_schema_column_output_indexes(ctx, NULL);
    } else {
      grn_obj *key_column;
      key_column = grn_obj_column(ctx, table,
                                  GRN_COLUMN_NAME_KEY,
                                  GRN_COLUMN_NAME_KEY_LEN);
      command_schema_column_output_indexes(ctx, key_column);
      grn_obj_unlink(ctx, key_column);
    }

    grn_ctx_output_cstr(ctx, "command");
    command_schema_table_output_command(ctx, table);

    grn_ctx_output_cstr(ctx, "columns");
    command_schema_table_output_columns(ctx, table);

    grn_ctx_output_map_close(ctx);
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &tables);
}

static grn_obj *
command_schema(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ctx_output_map_open(ctx, "schema", 6);
  command_schema_output_plugins(ctx);
  command_schema_output_types(ctx);
  command_schema_output_tokenizers(ctx);
  command_schema_output_normalizers(ctx);
  command_schema_output_token_filters(ctx);
  command_schema_output_tables(ctx);
  grn_ctx_output_map_close(ctx);

  return NULL;
}

void
grn_proc_init_schema(grn_ctx *ctx)
{
  grn_plugin_command_create(ctx,
                            "schema", -1,
                            command_schema,
                            0,
                            NULL);
}
