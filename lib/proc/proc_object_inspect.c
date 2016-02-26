/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016 Brazil

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

#include "../grn_pat.h"

#include "../grn_proc.h"

#include <groonga/plugin.h>

static void
command_object_inspect_obj_name(grn_ctx *ctx, grn_obj *obj)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_size;

  name_size = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
  grn_ctx_output_str(ctx, name, name_size);
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
command_object_inspect_table_hash_key_key(grn_ctx *ctx, grn_hash *hash)
{
  grn_ctx_output_map_open(ctx, "key", 3);
  {
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_type(ctx, grn_ctx_at(ctx, hash->obj.header.domain));
    grn_ctx_output_cstr(ctx, "total_size");
    grn_ctx_output_uint64(ctx, grn_hash_total_key_size(ctx, hash));
    grn_ctx_output_cstr(ctx, "max_total_size");
    grn_ctx_output_uint64(ctx, grn_hash_max_total_key_size(ctx, hash));
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_table_hash_key(grn_ctx *ctx, grn_obj *obj)
{
  grn_hash *hash = (grn_hash *)obj;

  grn_ctx_output_map_open(ctx, "object", 4);
  {
    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint64(ctx, grn_obj_id(ctx, obj));
    grn_ctx_output_cstr(ctx, "name");
    command_object_inspect_obj_name(ctx, obj);
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_obj_type(ctx, obj->header.type);
    grn_ctx_output_cstr(ctx, "key");
    command_object_inspect_table_hash_key_key(ctx, hash);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_table_pat_key_key(grn_ctx *ctx, grn_pat *pat)
{
  grn_ctx_output_map_open(ctx, "key", 3);
  {
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_type(ctx, grn_ctx_at(ctx, pat->obj.header.domain));
    grn_ctx_output_cstr(ctx, "total_size");
    grn_ctx_output_uint64(ctx, grn_pat_total_key_size(ctx, pat));
    grn_ctx_output_cstr(ctx, "max_total_size");
    grn_ctx_output_uint64(ctx, GRN_PAT_MAX_TOTAL_KEY_SIZE);
  }
  grn_ctx_output_map_close(ctx);
}

static void
command_object_inspect_table_pat_key(grn_ctx *ctx, grn_obj *obj)
{
  grn_pat *pat = (grn_pat *)obj;

  grn_ctx_output_map_open(ctx, "object", 4);
  {
    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint64(ctx, grn_obj_id(ctx, obj));
    grn_ctx_output_cstr(ctx, "name");
    command_object_inspect_obj_name(ctx, obj);
    grn_ctx_output_cstr(ctx, "type");
    command_object_inspect_obj_type(ctx, obj->header.type);
    grn_ctx_output_cstr(ctx, "key");
    command_object_inspect_table_pat_key_key(ctx, pat);
  }
  grn_ctx_output_map_close(ctx);
}

static grn_obj *
command_object_inspect(grn_ctx *ctx,
                       int nargs,
                       grn_obj **args,
                       grn_user_data *user_data)
{
  grn_obj *name;
  grn_obj *target;

  name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
  if (GRN_TEXT_LEN(name) == 0) {
    target = grn_ctx_db(ctx);
  } else {
    target = grn_ctx_get(ctx,
                         GRN_TEXT_VALUE(name),
                         GRN_TEXT_LEN(name));
    if (!target) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[object][inspect] nonexistent target: <%.*s>",
                       (int)GRN_TEXT_LEN(name),
                       GRN_TEXT_VALUE(name));
      grn_ctx_output_null(ctx);
      return NULL;
    }
  }

  switch (target->header.type) {
  case GRN_TYPE :
    command_object_inspect_type(ctx, target);
    break;
  case GRN_TABLE_HASH_KEY :
    command_object_inspect_table_hash_key(ctx, target);
    break;
  case GRN_TABLE_PAT_KEY :
    command_object_inspect_table_pat_key(ctx, target);
    break;
  default :
    {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_FUNCTION_NOT_IMPLEMENTED,
                       "[object][inspect] unsupported type: <%s>(%#x)",
                       grn_obj_type_to_string(target->header.type),
                       target->header.type);
      grn_ctx_output_null(ctx);
      break;
    }
  }

  return NULL;
}

void
grn_proc_init_object_inspect(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_command_create(ctx,
                            "object_inspect", -1,
                            command_object_inspect,
                            1,
                            vars);
}
