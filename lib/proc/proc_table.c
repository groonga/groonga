/*
  Copyright(C) 2009-2018  Brazil
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

#include "../grn_proc.h"

#include "../grn_ctx.h"
#include "../grn_str.h"
#include "../grn_db.h"

#include <groonga/plugin.h>

static grn_table_flags
command_table_create_parse_flags(grn_ctx *ctx,
                                 const char *nptr,
                                 const char *end)
{
  grn_table_flags flags = 0;
  while (nptr < end) {
    size_t name_size;

    if (*nptr == '|' || *nptr == ' ') {
      nptr += 1;
      continue;
    }

#define CHECK_FLAG(name)                                                \
    name_size = strlen(#name);                                          \
    if ((end - nptr) >= name_size &&                                    \
        memcmp(nptr, #name, name_size) == 0) {                          \
      flags |= GRN_OBJ_ ## name;                                        \
      nptr += name_size;                                                \
      continue;                                                         \
    }

    CHECK_FLAG(TABLE_HASH_KEY);
    CHECK_FLAG(TABLE_PAT_KEY);
    CHECK_FLAG(TABLE_DAT_KEY);
    CHECK_FLAG(TABLE_NO_KEY);
    CHECK_FLAG(KEY_NORMALIZE);
    CHECK_FLAG(KEY_WITH_SIS);
    CHECK_FLAG(KEY_LARGE);

#undef CHECK_FLAG

    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][create][flags] unknown flag: <%.*s>",
                     (int)(end - nptr), nptr);
    return 0;
  }
  return flags;
}

grn_bool
grn_proc_table_set_token_filters(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_raw_string *token_filters_raw)
{
  grn_bool succeeded = GRN_FALSE;
  grn_obj token_filters;

  if (token_filters_raw->length == 0) {
    return GRN_TRUE;
  }

  GRN_TEXT_INIT(&token_filters, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET(ctx,
               &token_filters,
               token_filters_raw->value,
               token_filters_raw->length);
  grn_obj_set_info(ctx, table, GRN_INFO_TOKEN_FILTERS, &token_filters);
  GRN_OBJ_FIN(ctx, &token_filters);

  return succeeded;
}

static grn_obj *
command_table_create(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_raw_string name_raw;
  grn_raw_string flags_raw;
  grn_raw_string key_type_raw;
  grn_raw_string value_type_raw;
  grn_obj *default_tokenizer_raw;
  grn_obj *normalizer_raw;
  grn_raw_string token_filters_raw;
  grn_obj *path_raw;
  grn_obj *normalizers_raw;
  grn_obj *key_type = NULL;
  grn_obj *value_type = NULL;
  grn_obj *table = NULL;
  const char *rest;
  grn_table_flags flags;

#define GET_VALUE(name)                                         \
  name ## _raw.value =                                          \
    grn_plugin_proc_get_var_string(ctx,                         \
                                   user_data,                   \
                                   #name,                       \
                                   strlen(#name),               \
                                   &(name ## _raw.length))

  GET_VALUE(name);
  GET_VALUE(flags);
  GET_VALUE(key_type);
  GET_VALUE(value_type);
  default_tokenizer_raw = grn_plugin_proc_get_var(ctx, user_data,
                                                  "default_tokenizer", -1);
  normalizer_raw = grn_plugin_proc_get_var(ctx, user_data, "normalizer", -1);
  GET_VALUE(token_filters);
  path_raw = grn_plugin_proc_get_var(ctx, user_data, "path", -1);
  normalizers_raw = grn_plugin_proc_get_var(ctx, user_data, "normalizers", -1);

#undef GET_VALUE

  flags = grn_atoi(flags_raw.value,
                   flags_raw.value + flags_raw.length,
                   &rest);
  if (flags_raw.value == rest) {
    flags = command_table_create_parse_flags(ctx,
                                             flags_raw.value,
                                             flags_raw.value + flags_raw.length);
    if (ctx->rc) { goto exit; }
  }

  if (name_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][create] should not create anonymous table");
    goto exit;
  }

  {
    if (key_type_raw.length > 0) {
      key_type = grn_ctx_get(ctx,
                             key_type_raw.value,
                             key_type_raw.length);
      if (!key_type) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[table][create] "
                         "key type doesn't exist: <%.*s> (%.*s)",
                         (int)name_raw.length,
                         name_raw.value,
                         (int)key_type_raw.length,
                         key_type_raw.value);
        goto exit;
      }
    }

    if (value_type_raw.length > 0) {
      value_type = grn_ctx_get(ctx,
                               value_type_raw.value,
                               value_type_raw.length);
      if (!value_type) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[table][create] "
                         "value type doesn't exist: <%.*s> (%.*s)",
                         (int)name_raw.length,
                         name_raw.value,
                         (int)value_type_raw.length,
                         value_type_raw.value);
        goto exit;
      }
    }

    const char *path = NULL;
    if (GRN_TEXT_LEN(path_raw) > 0) {
      GRN_TEXT_PUTC(ctx, path_raw, '\0');
      path = GRN_TEXT_VALUE(path_raw);
    }
    flags |= GRN_OBJ_PERSISTENT;
    table = grn_table_create(ctx,
                             name_raw.value,
                             name_raw.length,
                             path,
                             flags,
                             key_type,
                             value_type);
    if (!table) {
      goto exit;
    }

    if (GRN_TEXT_LEN(default_tokenizer_raw) > 0) {
      grn_obj_set_info(ctx, table,
                       GRN_INFO_DEFAULT_TOKENIZER,
                       default_tokenizer_raw);
      if (ctx->rc != GRN_SUCCESS) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[table][create][%.*s] "
                         "failed to set default tokenizer: <%.*s>: %s",
                         (int)name_raw.length,
                         name_raw.value,
                         (int)GRN_TEXT_LEN(default_tokenizer_raw),
                         GRN_TEXT_VALUE(default_tokenizer_raw),
                         ctx->errbuf);
        goto exit;
      }
    }

    if (GRN_TEXT_LEN(normalizers_raw) == 0 &&
        GRN_TEXT_LEN(normalizer_raw) > 0) {
      normalizers_raw = normalizer_raw;
    }
    if (GRN_TEXT_LEN(normalizers_raw) > 0) {
      grn_obj_set_info(ctx, table, GRN_INFO_NORMALIZERS, normalizers_raw);
      if (ctx->rc != GRN_SUCCESS) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[table][create][%.*s] "
                         "failed to set normalizers: <%.*s>: %s",
                         (int)name_raw.length,
                         name_raw.value,
                         (int)GRN_TEXT_LEN(normalizers_raw),
                         GRN_TEXT_VALUE(normalizers_raw),
                         ctx->errbuf);
        goto exit;
      }
    }

    if (!grn_proc_table_set_token_filters(ctx, table, &token_filters_raw)) {
      goto exit;
    }
  }

exit :
  {
    grn_bool success = (ctx->rc == GRN_SUCCESS);
    if (key_type) {
      grn_obj_unref(ctx, key_type);
    }
    if (value_type) {
      grn_obj_unref(ctx, value_type);
    }
    if (success) {
      if (table) {
        grn_obj_unlink(ctx, table);
      }
    } else {
      if (table) {
        grn_obj_remove(ctx, table);
      }
    }
    grn_ctx_output_bool(ctx, success);
  }
  return NULL;
}

void
grn_proc_init_table_create(grn_ctx *ctx)
{
  grn_expr_var vars[9];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "key_type", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "value_type", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "default_tokenizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[5]), "normalizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[6]), "token_filters", -1);
  grn_plugin_expr_var_init(ctx, &(vars[7]), "path", -1);
  grn_plugin_expr_var_init(ctx, &(vars[8]), "normalizers", -1);
  grn_plugin_command_create(ctx,
                            "table_create", -1,
                            command_table_create,
                            9,
                            vars);
}

static void
output_table_info(grn_ctx *ctx, grn_obj *table)
{
  grn_obj buffer;
  grn_id id;
  const char *path;
  grn_table_flags flags;
  grn_obj *default_tokenizer;
  grn_obj *normalizer;
  grn_obj *token_filters;

  GRN_TEXT_INIT(&buffer, 0);

  grn_ctx_output_array_open(ctx, "TABLE", 8);

  id = grn_obj_id(ctx, table);
  grn_ctx_output_int64(ctx, id);
  grn_proc_output_object_id_name(ctx, id);
  path = grn_obj_path(ctx, table);
  grn_ctx_output_cstr(ctx, path);

  grn_table_get_info(ctx, table,
                     &flags,
                     NULL,
                     &default_tokenizer,
                     &normalizer,
                     &token_filters);

  grn_dump_table_create_flags(ctx, flags, &buffer);
  grn_ctx_output_obj(ctx, &buffer, NULL);

  grn_proc_output_object_id_name(ctx, table->header.domain);
  grn_proc_output_object_id_name(ctx, grn_obj_get_range(ctx, table));

  if (default_tokenizer) {
    GRN_BULK_REWIND(&buffer);
    grn_table_get_default_tokenizer_string(ctx, table, &buffer);
    grn_ctx_output_obj(ctx, &buffer, NULL);
  } else {
    grn_ctx_output_null(ctx);
  }

  if (normalizer) {
    GRN_BULK_REWIND(&buffer);
    grn_table_get_normalizers_string(ctx, table, &buffer);
    grn_ctx_output_obj(ctx, &buffer, NULL);
  } else {
    grn_ctx_output_null(ctx);
  }

  grn_ctx_output_array_close(ctx);

  GRN_OBJ_FIN(ctx, &buffer);
}

static grn_obj *
command_table_list(grn_ctx *ctx, int nargs, grn_obj **args,
                   grn_user_data *user_data)
{
  grn_obj *db;
  grn_obj tables;
  int n_top_level_elements;
  int n_elements_for_header = 1;
  int n_tables;
  int i;

  db = grn_ctx_db(ctx);

  {
    grn_table_cursor *cursor;
    grn_id id;
    grn_obj *prefix;
    const void *min = NULL;
    unsigned int min_size = 0;
    int flags = 0;

    prefix = grn_plugin_proc_get_var(ctx, user_data, "prefix", -1);
    if (GRN_TEXT_LEN(prefix) > 0) {
      min = GRN_TEXT_VALUE(prefix);
      min_size = GRN_TEXT_LEN(prefix);
      flags |= GRN_CURSOR_PREFIX;
    }
    cursor = grn_table_cursor_open(ctx, db,
                                   min, min_size,
                                   NULL, 0,
                                   0, -1, flags);
    if (!cursor) {
      return NULL;
    }

    GRN_PTR_INIT(&tables, GRN_OBJ_VECTOR, GRN_ID_NIL);
    while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
      grn_obj *object;
      const char *name;
      void *key;
      int i, key_size;
      grn_bool have_period = GRN_FALSE;

      key_size = grn_table_cursor_get_key(ctx, cursor, &key);
      name = key;
      for (i = 0; i < key_size; i++) {
        if (name[i] == '.') {
          have_period = GRN_TRUE;
          break;
        }
      }
      if (have_period) {
        continue;
      }

      object = grn_ctx_at(ctx, id);
      if (object) {
        if (grn_obj_is_table(ctx, object)) {
          GRN_PTR_PUT(ctx, &tables, object);
        } else {
          grn_obj_unlink(ctx, object);
        }
      } else {
        if (ctx->rc != GRN_SUCCESS) {
          ERRCLR(ctx);
        }
      }
    }
    grn_table_cursor_close(ctx, cursor);
  }
  n_tables = GRN_BULK_VSIZE(&tables) / sizeof(grn_obj *);
  n_top_level_elements = n_elements_for_header + n_tables;
  grn_ctx_output_array_open(ctx, "TABLE_LIST", n_top_level_elements);

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
  grn_ctx_output_cstr(ctx, "default_tokenizer");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_open(ctx, "PROPERTY", 2);
  grn_ctx_output_cstr(ctx, "normalizer");
  grn_ctx_output_cstr(ctx, "ShortText");
  grn_ctx_output_array_close(ctx);
  grn_ctx_output_array_close(ctx);

  for (i = 0; i < n_tables; i++) {
    grn_obj *table = GRN_PTR_VALUE_AT(&tables, i);
    output_table_info(ctx, table);
    grn_obj_unlink(ctx, table);
  }
  GRN_OBJ_FIN(ctx, &tables);

  grn_ctx_output_array_close(ctx);

  return NULL;
}

void
grn_proc_init_table_list(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "prefix", -1);
  grn_plugin_command_create(ctx,
                            "table_list", -1,
                            command_table_list,
                            1,
                            vars);
}

static grn_obj *
command_table_remove(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_obj *name;
  grn_obj *table;
  grn_bool dependent;

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  dependent = grn_plugin_proc_get_var_bool(ctx, user_data, "dependent", -1,
                                           GRN_FALSE);
  table = grn_ctx_get(ctx,
                      GRN_TEXT_VALUE(name),
                      GRN_TEXT_LEN(name));
  if (!table) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][remove] table isn't found: <%.*s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    grn_ctx_output_bool(ctx, GRN_FALSE);
    return NULL;
  }

  if (!grn_obj_is_table(ctx, table)) {
    const char *type_name;
    type_name = grn_obj_type_to_string(table->header.type);
    grn_obj_unlink(ctx, table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][remove] not table: <%.*s>: <%s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name),
                     type_name);
    grn_ctx_output_bool(ctx, GRN_FALSE);
    return NULL;
  }

  if (dependent) {
    grn_obj_remove_dependent(ctx, table);
  } else {
    grn_obj_remove(ctx, table);
  }
  grn_ctx_output_bool(ctx, !ctx->rc);
  return NULL;
}

void
grn_proc_init_table_remove(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "dependent", -1);
  grn_plugin_command_create(ctx,
                            "table_remove", -1,
                            command_table_remove,
                            2,
                            vars);
}

static grn_obj *
command_table_rename(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *name;
  grn_obj *new_name;
  grn_obj *table = NULL;

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  new_name = grn_plugin_proc_get_var(ctx, user_data, "new_name", -1);
  if (GRN_TEXT_LEN(name) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx, rc, "[table][rename] table name isn't specified");
    goto exit;
  }
  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name));
  if (!table) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[table][rename] table isn't found: <%.*s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    goto exit;
  }
  if (GRN_TEXT_LEN(new_name) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[table][rename] new table name isn't specified: <%.*s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    goto exit;
  }
  rc = grn_table_rename(ctx, table,
                        GRN_TEXT_VALUE(new_name),
                        GRN_TEXT_LEN(new_name));
  if (rc != GRN_SUCCESS && ctx->rc == GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "[table][rename] failed to rename: <%.*s> -> <%.*s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name),
                     (int)GRN_TEXT_LEN(new_name),
                     GRN_TEXT_VALUE(new_name));
  }
exit :
  grn_ctx_output_bool(ctx, !rc);
  if (table) { grn_obj_unlink(ctx, table); }
  return NULL;
}

void
grn_proc_init_table_rename(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "new_name", -1);
  grn_plugin_command_create(ctx,
                            "table_rename", -1,
                            command_table_rename,
                            2,
                            vars);
}

static grn_obj *
command_table_resolve_target(grn_ctx *ctx,
                             const char *label,
                             grn_obj *name,
                             const char *tag)
{
  if (GRN_TEXT_LEN(name) == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s %s name isn't specified",
                     tag,
                     label);
    return NULL;
  }
  grn_obj *table = grn_ctx_get(ctx,
                               GRN_TEXT_VALUE(name),
                               GRN_TEXT_LEN(name));
  if (!table) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s %s table isn't found: <%.*s>",
                     tag,
                     label,
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
    return NULL;
  }

  return table;
}

static grn_obj *
command_table_copy(grn_ctx *ctx,
                   int nargs,
                   grn_obj **args,
                   grn_user_data *user_data)
{
  const char *tag = "[table][copy]";
  grn_obj *from_table = NULL;
  grn_obj *to_table = NULL;
  grn_obj *from_name;
  grn_obj *to_name;

  from_name = grn_plugin_proc_get_var(ctx, user_data, "from_name", -1);
  to_name   = grn_plugin_proc_get_var(ctx, user_data, "to_name", -1);

  from_table = command_table_resolve_target(ctx, "from", from_name, tag);
  if (!from_table) {
    goto exit;
  }
  to_table = command_table_resolve_target(ctx, "to", to_name, tag);
  if (!to_table) {
    goto exit;
  }

  grn_table_copy(ctx, from_table, to_table);

exit :
  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  if (to_table) {
    grn_obj_unlink(ctx, to_table);
  }
  if (from_table) {
    grn_obj_unlink(ctx, from_table);
  }

  return NULL;
}

void
grn_proc_init_table_copy(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "from_name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "to_name", -1);
  grn_plugin_command_create(ctx,
                            "table_copy", -1,
                            command_table_copy,
                            2,
                            vars);
}

static grn_obj *
command_table_create_similar(grn_ctx *ctx,
                             int nargs,
                             grn_obj **args,
                             grn_user_data *user_data)
{
  const char *tag = "[table][create][similar]";
  grn_obj *name;
  grn_obj *base_table_name;
  grn_obj *base_table = NULL;
  grn_obj *table;

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  base_table_name = grn_plugin_proc_get_var(ctx, user_data, "base_table", -1);

  base_table = command_table_resolve_target(ctx,
                                            "base_table",
                                            base_table_name,
                                            tag);
  if (!base_table) {
    goto exit;
  }

  table = grn_table_create_similar(ctx,
                                   GRN_TEXT_VALUE(name),
                                   GRN_TEXT_LEN(name),
                                   NULL,
                                   base_table);

exit :
  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  if (base_table) {
    grn_obj_unref(ctx, base_table);
  }
  if (table) {
    grn_obj_unref(ctx, table);
  }

  return NULL;
}

void
grn_proc_init_table_create_similar(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "base_table", -1);
  grn_plugin_command_create(ctx,
                            "table_create_similar", -1,
                            command_table_create_similar,
                            2,
                            vars);
}
