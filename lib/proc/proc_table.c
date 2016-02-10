/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil

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

#include <groonga/plugin.h>

static grn_obj_flags
command_table_create_parse_flags(grn_ctx *ctx,
                                 const char *nptr,
                                 const char *end)
{
  grn_obj_flags flags = 0;
  while (nptr < end) {
    if (*nptr == '|' || *nptr == ' ') {
      nptr += 1;
      continue;
    }
    if (!memcmp(nptr, "TABLE_HASH_KEY", 14)) {
      flags |= GRN_OBJ_TABLE_HASH_KEY;
      nptr += 14;
    } else if (!memcmp(nptr, "TABLE_PAT_KEY", 13)) {
      flags |= GRN_OBJ_TABLE_PAT_KEY;
      nptr += 13;
    } else if (!memcmp(nptr, "TABLE_DAT_KEY", 13)) {
      flags |= GRN_OBJ_TABLE_DAT_KEY;
      nptr += 13;
    } else if (!memcmp(nptr, "TABLE_NO_KEY", 12)) {
      flags |= GRN_OBJ_TABLE_NO_KEY;
      nptr += 12;
    } else if (!memcmp(nptr, "KEY_NORMALIZE", 13)) {
      flags |= GRN_OBJ_KEY_NORMALIZE;
      nptr += 13;
    } else if (!memcmp(nptr, "KEY_WITH_SIS", 12)) {
      flags |= GRN_OBJ_KEY_WITH_SIS;
      nptr += 12;
    } else {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[table][create][flags] unknown flag: <%.*s>",
                       (int)(end - nptr), nptr);
      return 0;
    }
  }
  return flags;
}

static grn_bool
grn_proc_table_set_token_filters_put(grn_ctx *ctx,
                                     grn_obj *token_filters,
                                     const char *token_filter_name,
                                     int token_filter_name_length)
{
  grn_obj *token_filter;

  token_filter = grn_ctx_get(ctx,
                             token_filter_name,
                             token_filter_name_length);
  if (token_filter) {
    GRN_PTR_PUT(ctx, token_filters, token_filter);
    return GRN_TRUE;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][create][token-filter] "
                     "nonexistent token filter: <%.*s>",
                     token_filter_name_length, token_filter_name);
    return GRN_FALSE;
  }
}

static grn_bool
grn_proc_table_set_token_filters_fill(grn_ctx *ctx,
                                      grn_obj *token_filters,
                                      grn_obj *token_filter_names)
{
  const char *start, *current, *end;
  const char *name_start, *name_end;
  const char *last_name_end;

  start = GRN_TEXT_VALUE(token_filter_names);
  end = start + GRN_TEXT_LEN(token_filter_names);
  current = start;
  name_start = NULL;
  name_end = NULL;
  last_name_end = start;
  while (current < end) {
    switch (current[0]) {
    case ' ' :
      if (name_start && !name_end) {
        name_end = current;
      }
      break;
    case ',' :
      if (!name_start) {
        goto break_loop;
      }
      if (!name_end) {
        name_end = current;
      }
      grn_proc_table_set_token_filters_put(ctx,
                                           token_filters,
                                           name_start,
                                           name_end - name_start);
      last_name_end = name_end + 1;
      name_start = NULL;
      name_end = NULL;
      break;
    default :
      if (!name_start) {
        name_start = current;
      }
      break;
    }
    current++;
  }

break_loop:
  if (!name_start) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][create][token-filter] empty token filter name: "
                     "<%.*s|%.*s|%.*s>",
                     (int)(last_name_end - start), start,
                     (int)(current - last_name_end), last_name_end,
                     (int)(end - current), current);
    return GRN_FALSE;
  }

  if (!name_end) {
    name_end = current;
  }
  grn_proc_table_set_token_filters_put(ctx,
                                       token_filters,
                                       name_start,
                                       name_end - name_start);

  return GRN_TRUE;
}

void
grn_proc_table_set_token_filters(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_obj *token_filter_names)
{
  grn_obj token_filters;

  if (GRN_TEXT_LEN(token_filter_names) == 0) {
    return;
  }

  GRN_PTR_INIT(&token_filters, GRN_OBJ_VECTOR, 0);
  if (grn_proc_table_set_token_filters_fill(ctx,
                                            &token_filters,
                                            token_filter_names)) {
    grn_obj_set_info(ctx, table, GRN_INFO_TOKEN_FILTERS, &token_filters);
  }
  grn_obj_unlink(ctx, &token_filters);
}

static grn_obj *
command_table_create(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_obj *name;
  grn_obj *flags_raw;
  grn_obj *key_type_name;
  grn_obj *value_type_name;
  grn_obj *default_tokenizer;
  grn_obj *normalizer;
  grn_obj *token_filters;
  grn_obj *table;
  const char *rest;
  grn_obj_flags flags;

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  flags_raw = grn_plugin_proc_get_var(ctx, user_data, "flags", -1);
  key_type_name = grn_plugin_proc_get_var(ctx, user_data, "key_type", -1);
  value_type_name = grn_plugin_proc_get_var(ctx, user_data, "value_type", -1);
  default_tokenizer =
    grn_plugin_proc_get_var(ctx, user_data, "default_tokenizer", -1);
  normalizer =
    grn_plugin_proc_get_var(ctx, user_data, "normalizer", -1);
  token_filters =
    grn_plugin_proc_get_var(ctx, user_data, "token_filters", -1);

  flags = grn_atoi(GRN_TEXT_VALUE(flags_raw),
                   GRN_BULK_CURR(flags_raw),
                   &rest);
  if (GRN_TEXT_VALUE(flags_raw) == rest) {
    flags = command_table_create_parse_flags(ctx,
                                             GRN_TEXT_VALUE(flags_raw),
                                             GRN_BULK_CURR(flags_raw));
    if (ctx->rc) { goto exit; }
  }
  if (GRN_TEXT_LEN(name)) {
    grn_obj *key_type = NULL;
    grn_obj *value_type = NULL;
    if (GRN_TEXT_LEN(key_type_name) > 0) {
      key_type = grn_ctx_get(ctx,
                             GRN_TEXT_VALUE(key_type_name),
                             GRN_TEXT_LEN(key_type_name));
      if (!key_type) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[table][create] "
                         "key type doesn't exist: <%.*s> (%.*s)",
                         (int)GRN_TEXT_LEN(name),
                         GRN_TEXT_VALUE(name),
                         (int)GRN_TEXT_LEN(key_type_name),
                         GRN_TEXT_VALUE(key_type_name));
        return NULL;
      }
    }
    if (GRN_TEXT_LEN(value_type_name) > 0) {
      value_type = grn_ctx_get(ctx,
                               GRN_TEXT_VALUE(value_type_name),
                               GRN_TEXT_LEN(value_type_name));
      if (!value_type) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[table][create] "
                         "value type doesn't exist: <%.*s> (%.*s)",
                         (int)GRN_TEXT_LEN(name),
                         GRN_TEXT_VALUE(name),
                         (int)GRN_TEXT_LEN(value_type_name),
                         GRN_TEXT_VALUE(value_type_name));
        return NULL;
      }
    }
    flags |= GRN_OBJ_PERSISTENT;
    table = grn_table_create(ctx,
                             GRN_TEXT_VALUE(name),
                             GRN_TEXT_LEN(name),
                             NULL, flags,
                             key_type,
                             value_type);
    if (table) {
      grn_obj_set_info(ctx, table,
                       GRN_INFO_DEFAULT_TOKENIZER,
                       grn_ctx_get(ctx,
                                   GRN_TEXT_VALUE(default_tokenizer),
                                   GRN_TEXT_LEN(default_tokenizer)));
      if (GRN_TEXT_LEN(normalizer) > 0) {
        grn_obj_set_info(ctx, table,
                         GRN_INFO_NORMALIZER,
                         grn_ctx_get(ctx,
                                     GRN_TEXT_VALUE(normalizer),
                                     GRN_TEXT_LEN(normalizer)));
      }
      grn_proc_table_set_token_filters(ctx, table, token_filters);
      grn_obj_unlink(ctx, table);
    }
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][create] should not create anonymous table");
  }
exit :
  grn_ctx_output_bool(ctx, !ctx->rc);
  return NULL;
}

void
grn_proc_init_table_create(grn_ctx *ctx)
{
  grn_expr_var vars[7];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "key_type", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "value_type", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "default_tokenizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[5]), "normalizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[6]), "token_filters", -1);
  grn_plugin_command_create(ctx,
                            "table_create", -1,
                            command_table_create,
                            7,
                            vars);
}

static int
output_table_info(grn_ctx *ctx, grn_obj *table)
{
  grn_id id;
  grn_obj o;
  const char *path;
  grn_obj *default_tokenizer;
  grn_obj *normalizer;

  id = grn_obj_id(ctx, table);
  path = grn_obj_path(ctx, table);
  GRN_TEXT_INIT(&o, 0);
  grn_ctx_output_array_open(ctx, "TABLE", 8);
  grn_ctx_output_int64(ctx, id);
  grn_proc_output_object_id_name(ctx, id);
  grn_ctx_output_cstr(ctx, path);
  GRN_BULK_REWIND(&o);
  grn_dump_table_create_flags(ctx, table->header.flags, &o);
  grn_ctx_output_obj(ctx, &o, NULL);
  grn_proc_output_object_id_name(ctx, table->header.domain);
  grn_proc_output_object_id_name(ctx, grn_obj_get_range(ctx, table));
  default_tokenizer = grn_obj_get_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER,
                                       NULL);
  grn_proc_output_object_name(ctx, default_tokenizer);
  normalizer = grn_obj_get_info(ctx, table, GRN_INFO_NORMALIZER, NULL);
  grn_proc_output_object_name(ctx, normalizer);
  grn_obj_unlink(ctx, normalizer);
  grn_ctx_output_array_close(ctx);
  GRN_OBJ_FIN(ctx, &o);
  return 1;
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

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  table = grn_ctx_get(ctx,
                      GRN_TEXT_VALUE(name),
                      GRN_TEXT_LEN(name));
  if (table) {
    grn_obj_remove(ctx, table);
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[table][remove] table isn't found: <%.*s>",
                     (int)GRN_TEXT_LEN(name),
                     GRN_TEXT_VALUE(name));
  }
  grn_ctx_output_bool(ctx, !ctx->rc);
  return NULL;
}

void
grn_proc_init_table_remove(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_command_create(ctx,
                            "table_remove", -1,
                            command_table_remove,
                            1,
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
