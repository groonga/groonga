/*
  Copyright(C) 2009-2016  Brazil
  Copyright(C) 2018-2023  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_ctx.h"
#include "../grn_db.h"
#include "../grn_str.h"

#include <groonga/plugin.h>

grn_column_flags
grn_proc_column_parse_flags(grn_ctx *ctx,
                            const char *error_message_tag,
                            const char *text,
                            const char *end)
{
  grn_column_flags flags = 0;
  while (text < end) {
    size_t name_size;

    if (*text == '|' || *text == ' ') {
      text += 1;
      continue;
    }

#define CHECK_FLAG(name)                                                       \
  name_size = strlen(#name);                                                   \
  if ((size_t)(end - text) >= name_size &&                                     \
      memcmp(text, #name, name_size) == 0) {                                   \
    flags |= GRN_OBJ_##name;                                                   \
    text += name_size;                                                         \
    continue;                                                                  \
  }

    CHECK_FLAG(COLUMN_SCALAR);
    CHECK_FLAG(COLUMN_VECTOR);
    CHECK_FLAG(COLUMN_INDEX);
    CHECK_FLAG(COMPRESS_ZLIB);
    CHECK_FLAG(COMPRESS_LZ4);
    CHECK_FLAG(COMPRESS_ZSTD);
    CHECK_FLAG(WITH_SECTION);
    CHECK_FLAG(WITH_WEIGHT);
    CHECK_FLAG(WITH_POSITION);
    CHECK_FLAG(RING_BUFFER);
    CHECK_FLAG(INDEX_SMALL);
    CHECK_FLAG(INDEX_MEDIUM);
    CHECK_FLAG(INDEX_LARGE);
    CHECK_FLAG(WEIGHT_FLOAT32);
    CHECK_FLAG(MISSING_ADD);
    CHECK_FLAG(MISSING_IGNORE);
    CHECK_FLAG(MISSING_NIL);
    CHECK_FLAG(INVALID_ERROR);
    CHECK_FLAG(INVALID_WARN);
    CHECK_FLAG(INVALID_IGNORE);
    CHECK_FLAG(COMPRESS_FILTER_SHUFFLE);
    CHECK_FLAG(COMPRESS_FILTER_BYTE_DELTA);
    CHECK_FLAG(COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE);
    CHECK_FLAG(COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES);

#undef CHECK_FLAG

    ERR(GRN_INVALID_ARGUMENT,
        "%s unknown flag: <%.*s>",
        error_message_tag,
        (int)(end - text),
        text);
    return 0;
  }
  return flags;
}

static grn_rc
command_column_create_resolve_source_name(grn_ctx *ctx,
                                          grn_obj *table,
                                          const char *source_name,
                                          uint32_t source_name_length,
                                          grn_obj *source_ids)
{
  grn_obj *column;

  column = grn_obj_column(ctx, table, source_name, source_name_length);
  if (!column) {
    ERR(GRN_INVALID_ARGUMENT,
        "[column][create] nonexistent source: <%.*s>",
        source_name_length,
        source_name);
    return ctx->rc;
  }

  if (column->header.type == GRN_ACCESSOR) {
    if (strncmp(source_name, "_key", source_name_length) == 0) {
      grn_id source_id = grn_obj_id(ctx, table);
      GRN_UINT32_PUT(ctx, source_ids, source_id);
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "[column][create] pseudo column except <_key> is invalid: <%.*s>",
          source_name_length,
          source_name);
    }
  } else {
    grn_id source_id = grn_obj_id(ctx, column);
    GRN_UINT32_PUT(ctx, source_ids, source_id);
  }
  grn_obj_unlink(ctx, column);

  return ctx->rc;
}

static grn_rc
command_column_create_resolve_source_names(grn_ctx *ctx,
                                           grn_obj *table,
                                           grn_obj *source_names,
                                           grn_obj *source_ids)
{
  size_t i, names_length;
  size_t start, source_name_length;
  const char *names;

  names = GRN_TEXT_VALUE(source_names);
  start = 0;
  source_name_length = 0;
  names_length = GRN_TEXT_LEN(source_names);
  for (i = 0; i < names_length; i++) {
    switch (names[i]) {
    case ' ':
      if (source_name_length == 0) {
        start++;
      }
      break;
    case ',':
      {
        grn_rc rc;
        const char *source_name = names + start;
        rc = command_column_create_resolve_source_name(
          ctx,
          table,
          source_name,
          (uint32_t)source_name_length,
          source_ids);
        if (rc) {
          return rc;
        }
        start = i + 1;
        source_name_length = 0;
      }
      break;
    default:
      source_name_length++;
      break;
    }
  }

  if (source_name_length > 0) {
    grn_rc rc;
    const char *source_name = names + start;
    rc = command_column_create_resolve_source_name(ctx,
                                                   table,
                                                   source_name,
                                                   (uint32_t)source_name_length,
                                                   source_ids);
    if (rc) {
      return rc;
    }
  }

  return GRN_SUCCESS;
}

static grn_obj *
command_column_create(grn_ctx *ctx,
                      int nargs,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  grn_bool succeeded = GRN_TRUE;
  grn_obj *table;
  grn_obj *column;
  grn_obj *table_raw;
  grn_obj *name;
  grn_obj *flags_raw;
  grn_obj *type_raw;
  grn_obj *source_raw;
  grn_obj *path_raw;
  grn_column_flags flags;
  grn_obj *type = NULL;

  table_raw = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  flags_raw = grn_plugin_proc_get_var(ctx, user_data, "flags", -1);
  type_raw = grn_plugin_proc_get_var(ctx, user_data, "type", -1);
  source_raw = grn_plugin_proc_get_var(ctx, user_data, "source", -1);
  path_raw = grn_plugin_proc_get_var(ctx, user_data, "path", -1);

  table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(table_raw), (int)GRN_TEXT_LEN(table_raw));
  if (!table) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][create] table doesn't exist: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    succeeded = GRN_FALSE;
    goto exit;
  }

  {
    const char *rest;
    flags = (grn_column_flags)(grn_atoi(GRN_TEXT_VALUE(flags_raw),
                                        GRN_BULK_CURR(flags_raw),
                                        &rest));
    if (GRN_TEXT_VALUE(flags_raw) == rest) {
      flags = grn_proc_column_parse_flags(ctx,
                                          "[column][create][flags]",
                                          GRN_TEXT_VALUE(flags_raw),
                                          GRN_BULK_CURR(flags_raw));
      if (ctx->rc) {
        succeeded = GRN_FALSE;
        goto exit;
      }
    }
  }

  type =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(type_raw), (int)GRN_TEXT_LEN(type_raw));
  if (!type) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][create] type doesn't exist: <%.*s>",
                     (int)GRN_TEXT_LEN(type_raw),
                     GRN_TEXT_VALUE(type_raw));
    succeeded = GRN_FALSE;
    goto exit;
  }

  if (GRN_TEXT_LEN(name) == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][create] name is missing");
    succeeded = GRN_FALSE;
    goto exit;
  }

  const char *path = NULL;
  if (GRN_TEXT_LEN(path_raw) > 0) {
    GRN_TEXT_PUTC(ctx, path_raw, '\0');
    path = GRN_TEXT_VALUE(path_raw);
  }
  flags |= GRN_OBJ_PERSISTENT;
  column = grn_column_create(ctx,
                             table,
                             GRN_TEXT_VALUE(name),
                             (unsigned int)GRN_TEXT_LEN(name),
                             path,
                             flags,
                             type);
  if (!column) {
    succeeded = GRN_FALSE;
    goto exit;
  }

  if (GRN_TEXT_LEN(source_raw) > 0) {
    grn_obj source_ids;
    GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
    grn_obj *source_table;
    if (column->header.type == GRN_COLUMN_INDEX) {
      source_table = type;
    } else {
      source_table = table;
    }
    grn_rc rc = command_column_create_resolve_source_names(ctx,
                                                           source_table,
                                                           source_raw,
                                                           &source_ids);
    if (rc == GRN_SUCCESS && GRN_BULK_VSIZE(&source_ids) > 0) {
      grn_obj_set_info(ctx, column, GRN_INFO_SOURCE, &source_ids);
      rc = ctx->rc;
    }
    GRN_OBJ_FIN(ctx, &source_ids);
    if (rc != GRN_SUCCESS) {
      grn_rc original_rc = ctx->rc;
      ctx->rc = GRN_SUCCESS;
      grn_obj_remove(ctx, column);
      ctx->rc = original_rc;
      succeeded = GRN_FALSE;
      goto exit;
    }
  }

  grn_obj_unlink(ctx, column);

exit:
  grn_ctx_output_bool(ctx, succeeded);
  if (table) {
    grn_obj_unlink(ctx, table);
  }
  if (type) {
    grn_obj_unlink(ctx, type);
  }

  return NULL;
}

void
grn_proc_init_column_create(grn_ctx *ctx)
{
  grn_expr_var vars[6];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "type", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "source", -1);
  grn_plugin_expr_var_init(ctx, &(vars[5]), "path", -1);
  grn_plugin_command_create(ctx,
                            "column_create",
                            -1,
                            command_column_create,
                            6,
                            vars);
}

static grn_obj *
command_column_remove(grn_ctx *ctx,
                      int nargs,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  grn_obj *table_raw;
  grn_obj *name;
  grn_obj *table;
  grn_obj *column;
  char fullname[GRN_TABLE_MAX_KEY_SIZE];
  int fullname_len;

  table_raw = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);

  table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(table_raw), (int)GRN_TEXT_LEN(table_raw));
  if (!table) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][remove] table isn't found: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    grn_ctx_output_bool(ctx, GRN_FALSE);
    return NULL;
  }

  fullname_len = grn_obj_name(ctx, table, fullname, GRN_TABLE_MAX_KEY_SIZE);
  if (fullname_len == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][remove] table isn't found: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    grn_ctx_output_bool(ctx, GRN_FALSE);
    grn_obj_unref(ctx, table);
    return NULL;
  }

  fullname[fullname_len] = GRN_DB_DELIMITER;
  fullname_len++;
  if ((size_t)fullname_len + GRN_TEXT_LEN(name) > GRN_TABLE_MAX_KEY_SIZE) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][remove] column name is too long: <%d> > <%u>: "
                     "<%.*s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TABLE_MAX_KEY_SIZE - fullname_len,
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    grn_ctx_output_bool(ctx, GRN_FALSE);
    grn_obj_unref(ctx, table);
    return NULL;
  }
  grn_memcpy(fullname + fullname_len, GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name));
  fullname_len += (int)GRN_TEXT_LEN(name);
  column = grn_ctx_get(ctx, fullname, fullname_len);
  if (!column) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][remove] column isn't found: <%.*s%c%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw),
                     GRN_DB_DELIMITER,
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    grn_ctx_output_bool(ctx, GRN_FALSE);
    grn_obj_unref(ctx, table);
    return NULL;
  }

  grn_obj_remove(ctx, column);
  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);
  grn_obj_unref(ctx, table);
  return NULL;
}

void
grn_proc_init_column_remove(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "name", -1);
  grn_plugin_command_create(ctx,
                            "column_remove",
                            -1,
                            command_column_remove,
                            2,
                            vars);
}

static grn_obj *
command_column_rename(grn_ctx *ctx,
                      int nargs,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *table_raw;
  grn_obj *name;
  grn_obj *new_name;
  grn_obj *table = NULL;
  grn_obj *column = NULL;

  table_raw = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  new_name = grn_plugin_proc_get_var(ctx, user_data, "new_name", -1);

  if (GRN_TEXT_LEN(table_raw) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx, rc, "[column][rename] table name isn't specified");
    goto exit;
  }

  table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(table_raw), (int)GRN_TEXT_LEN(table_raw));
  if (!table) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[column][rename] table isn't found: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    goto exit;
  }

  if (GRN_TEXT_LEN(name) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[column][rename] column name isn't specified: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    goto exit;
  }

  column = grn_obj_column(ctx,
                          table,
                          GRN_TEXT_VALUE(name),
                          (uint32_t)GRN_TEXT_LEN(name));
  if (!column) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[column][rename] column isn't found: <%.*s%c%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw),
                     GRN_DB_DELIMITER,
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    goto exit;
  }

  if (GRN_TEXT_LEN(new_name) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[column][rename] new column name isn't specified: "
                     "<%.*s%c%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw),
                     GRN_DB_DELIMITER,
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    goto exit;
  }

  rc = grn_column_rename(ctx,
                         column,
                         GRN_TEXT_VALUE(new_name),
                         (unsigned int)GRN_TEXT_LEN(new_name));
  if (rc != GRN_SUCCESS && ctx->rc == GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[column][rename] failed to rename: "
                     "<%.*s%c%.*s> -> <%.*s%c%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw),
                     GRN_DB_DELIMITER,
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name),
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw),
                     GRN_DB_DELIMITER,
                     (int)GRN_TEXT_LEN(new_name),
                     GRN_TEXT_VALUE(new_name));
    goto exit;
  }

exit:
  grn_ctx_output_bool(ctx, rc == GRN_SUCCESS);
  if (column) {
    grn_obj_unlink(ctx, column);
  }
  if (table) {
    grn_obj_unlink(ctx, table);
  }
  return NULL;
}

void
grn_proc_init_column_rename(grn_ctx *ctx)
{
  grn_expr_var vars[3];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "new_name", -1);
  grn_plugin_command_create(ctx,
                            "column_rename",
                            -1,
                            command_column_rename,
                            3,
                            vars);
}

static void
output_column_name(grn_ctx *ctx, grn_obj *column)
{
  grn_obj bulk;
  int name_len;
  char name[GRN_TABLE_MAX_KEY_SIZE];

  GRN_TEXT_INIT(&bulk, GRN_OBJ_DO_SHALLOW_COPY);
  name_len = grn_column_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
  GRN_TEXT_SET(ctx, &bulk, name, name_len);

  grn_ctx_output_obj(ctx, &bulk, NULL);
  GRN_OBJ_FIN(ctx, &bulk);
}

static int
output_column_info(grn_ctx *ctx, grn_obj *column)
{
  grn_obj o;
  grn_id id;
  const char *type;
  const char *path;

  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE:
    type = "fix";
    break;
  case GRN_COLUMN_VAR_SIZE:
    type = "var";
    break;
  case GRN_COLUMN_INDEX:
    type = "index";
    break;
  default:
    GRN_LOG(ctx,
            GRN_LOG_NOTICE,
            "invalid header type %d\n",
            column->header.type);
    return 0;
  }
  id = grn_obj_id(ctx, column);
  path = grn_obj_path(ctx, column);
  GRN_TEXT_INIT(&o, 0);
  grn_ctx_output_array_open(ctx, "COLUMN", 8);
  grn_ctx_output_int64(ctx, id);
  output_column_name(ctx, column);
  grn_ctx_output_cstr(ctx, path);
  grn_ctx_output_cstr(ctx, type);
  grn_dump_column_create_flags(ctx, grn_column_get_flags(ctx, column), &o);
  grn_ctx_output_obj(ctx, &o, NULL);
  grn_proc_output_object_id_name(ctx, column->header.domain);
  grn_proc_output_object_id_name(ctx, grn_obj_get_range(ctx, column));
  {
    grn_db_obj *obj = (grn_db_obj *)column;
    grn_id *s = obj->source;
    size_t i = 0, n = obj->source_size / sizeof(grn_id);
    grn_ctx_output_array_open(ctx, "SOURCES", (int)n);
    for (i = 0; i < n; i++, s++) {
      grn_proc_output_object_id_name(ctx, *s);
    }
    grn_ctx_output_array_close(ctx);
  }
  /* output_obj_source(ctx, (grn_db_obj *)column); */
  grn_ctx_output_array_close(ctx);
  GRN_OBJ_FIN(ctx, &o);
  return 1;
}

static grn_obj *
command_column_list(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *table_raw;
  grn_obj *table;
  grn_hash *cols;
  grn_obj *col;
  int column_list_size = -1;

  table_raw = grn_plugin_proc_get_var(ctx, user_data, "table", -1);

  table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(table_raw), (int)GRN_TEXT_LEN(table_raw));
  if (!table) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][list] table doesn't exist: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    return NULL;
  }

  if (!grn_obj_is_table(ctx, table)) {
    const char *type_name;
    type_name = grn_obj_type_to_string(table->header.type);
    grn_obj_unlink(ctx, table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][list] not table: <%.*s>: <%s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw),
                     type_name);
    return NULL;
  }

  column_list_size = 1; /* [header, (key), (COLUMNS)] */
  if (table->header.type != GRN_TABLE_NO_KEY) {
    column_list_size++;
  }
  cols = grn_hash_create(ctx,
                         NULL,
                         sizeof(grn_id),
                         0,
                         GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!cols) {
    grn_obj_unlink(ctx, table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[column][list] "
                     "failed to create temporary table to list columns: <%.*s>",
                     (int)GRN_TEXT_LEN(table_raw),
                     GRN_TEXT_VALUE(table_raw));
    return NULL;
  }

  column_list_size += grn_table_columns(ctx, table, NULL, 0, (grn_obj *)cols);

  grn_ctx_output_array_open(ctx, "COLUMN_LIST", column_list_size);
  grn_ctx_output_array_open(ctx, "HEADER", 8);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "id");
  grn_ctx_output_cstr(ctx, "UInt32");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "name");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "path");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "type");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "flags");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "domain");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "range");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "source");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_close(ctx);

  if ((col = grn_obj_column(ctx,
                            table,
                            GRN_COLUMN_NAME_KEY,
                            GRN_COLUMN_NAME_KEY_LEN))) {
    int name_len;
    char name_buf[GRN_TABLE_MAX_KEY_SIZE];
    grn_id id;
    grn_obj buf;
    GRN_TEXT_INIT(&buf, 0);
    grn_ctx_output_array_open(ctx, "COLUMN", 8);
    id = grn_obj_id(ctx, table);
    grn_ctx_output_int64(ctx, id);
    grn_ctx_output_cstr(ctx, GRN_COLUMN_NAME_KEY);
    grn_ctx_output_cstr(ctx, "");
    grn_ctx_output_cstr(ctx, "");
    grn_dump_column_create_flags(ctx, 0, &buf);
    grn_ctx_output_obj(ctx, &buf, NULL);
    name_len = grn_obj_name(ctx, table, name_buf, GRN_TABLE_MAX_KEY_SIZE);
    grn_ctx_output_str(ctx, name_buf, (size_t)name_len);
    grn_proc_output_object_id_name(ctx, table->header.domain);
    grn_ctx_output_array_open(ctx, "SOURCES", 0);
    grn_ctx_output_array_close(ctx);
    grn_ctx_output_array_close(ctx);
    GRN_OBJ_FIN(ctx, &buf);
    grn_obj_unlink(ctx, col);
  }
  {
    grn_id *key;
    GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
      if ((col = grn_ctx_at(ctx, *key))) {
        output_column_info(ctx, col);
        grn_obj_unlink(ctx, col);
      }
    });
  }
  grn_ctx_output_array_close(ctx);
  grn_hash_close(ctx, cols);
  grn_obj_unlink(ctx, table);

  return NULL;
}

void
grn_proc_init_column_list(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_command_create(ctx,
                            "column_list",
                            -1,
                            command_column_list,
                            1,
                            vars);
}

static grn_obj *
command_column_copy(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *from_table = NULL;
  grn_obj *from_column = NULL;
  grn_obj *to_table = NULL;
  grn_obj *to_column = NULL;
  grn_obj *from_table_name;
  grn_obj *from_column_name;
  grn_obj *to_table_name;
  grn_obj *to_column_name;

  from_table_name = grn_plugin_proc_get_var(ctx, user_data, "from_table", -1);
  from_column_name = grn_plugin_proc_get_var(ctx, user_data, "from_name", -1);
  to_table_name = grn_plugin_proc_get_var(ctx, user_data, "to_table", -1);
  to_column_name = grn_plugin_proc_get_var(ctx, user_data, "to_name", -1);

  from_table = grn_proc_get_value_object(ctx,
                                         from_table_name,
                                         "[column][copy][from_table]");
  if (!from_table) {
    goto exit;
  }
  from_column = grn_proc_get_value_column(ctx,
                                          from_column_name,
                                          from_table,
                                          "[column][copy][from_name]");
  if (!from_column) {
    goto exit;
  }
  to_table =
    grn_proc_get_value_object(ctx, to_table_name, "[column][copy][to_table]");
  if (!to_table) {
    goto exit;
  }
  to_column = grn_proc_get_value_column(ctx,
                                        to_column_name,
                                        to_table,
                                        "[column][copy][to_name]");
  if (!to_column) {
    goto exit;
  }

  grn_column_copy(ctx, from_column, to_column);

exit:
  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  if (to_column) {
    grn_obj_unref(ctx, to_column);
  }
  if (to_table) {
    grn_obj_unref(ctx, to_table);
  }
  if (from_column) {
    grn_obj_unref(ctx, from_column);
  }
  if (from_table) {
    grn_obj_unref(ctx, from_table);
  }

  return NULL;
}

void
grn_proc_init_column_copy(grn_ctx *ctx)
{
  grn_expr_var vars[4];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "from_table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "from_name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "to_table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "to_name", -1);
  grn_plugin_command_create(ctx,
                            "column_copy",
                            -1,
                            command_column_copy,
                            4,
                            vars);
}

static grn_obj *
command_column_create_similar(grn_ctx *ctx,
                              int nargs,
                              grn_obj **args,
                              grn_user_data *user_data)
{
  grn_obj *table_name;
  grn_obj *table = NULL;
  grn_obj *name;
  grn_obj *base_column_name;
  grn_obj *base_column = NULL;
  grn_obj *column = NULL;

  table_name = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  base_column_name = grn_plugin_proc_get_var(ctx, user_data, "base_column", -1);

  table = grn_proc_get_value_object(ctx,
                                    table_name,
                                    "[column][create][similar][table]");
  if (!table) {
    goto exit;
  }
  base_column =
    grn_proc_get_value_object(ctx,
                              base_column_name,
                              "[column][create][similar][base_column]");
  if (!base_column) {
    goto exit;
  }

  column = grn_column_create_similar(ctx,
                                     table,
                                     GRN_TEXT_VALUE(name),
                                     (uint32_t)GRN_TEXT_LEN(name),
                                     NULL,
                                     base_column);

exit:
  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  if (table) {
    grn_obj_unref(ctx, table);
  }
  if (base_column) {
    grn_obj_unref(ctx, base_column);
  }
  if (column) {
    grn_obj_unref(ctx, column);
  }

  return NULL;
}

void
grn_proc_init_column_create_similar(grn_ctx *ctx)
{
  grn_expr_var vars[3];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "base_column", -1);
  grn_plugin_command_create(ctx,
                            "column_create_similar",
                            -1,
                            command_column_create_similar,
                            3,
                            vars);
}
