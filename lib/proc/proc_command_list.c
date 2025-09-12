/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_db.h"
#include "../grn_proc.h"

#include <groonga/plugin.h>

static grn_obj *
command_command_list(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  const bool is_close_opened_object_mode = (grn_thread_get_limit() == 1);
  grn_obj command_ids;

  GRN_RECORD_INIT(&command_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_DB_EACH_BEGIN_BY_KEY(ctx, cursor, id)
  {
    if (id == GRN_DB_MECAB) {
      /* TokenMecab must not be a command. TokenMecab may exist as a
       * dummy object for historically reason. */
      continue;
    }

    void *name;
    int name_size = grn_table_cursor_get_key(ctx, cursor, &name);
    if (grn_obj_name_is_column(ctx, name, name_size)) {
      continue;
    }

    if (is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    grn_obj *object = grn_ctx_at(ctx, id);
    if (object) {
      if (grn_obj_is_command_proc(ctx, object)) {
        GRN_RECORD_PUT(ctx, &command_ids, id);
      }
    } else {
      GRN_PLUGIN_CLEAR_ERROR(ctx);
    }

    if (is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
  }
  GRN_DB_EACH_END(ctx, cursor);

  size_t n = GRN_RECORD_VECTOR_SIZE(&command_ids);
  grn_ctx_output_map_open(ctx, "commands", (int)n);
  for (size_t i = 0; i < n; i++) {
    if (is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    grn_id command_id = GRN_RECORD_VALUE_AT(&command_ids, i);
    grn_obj *command = grn_ctx_at(ctx, command_id);

    GRN_DEFINE_NAME(command);

    grn_ctx_output_str(ctx, name, name_size);

    grn_ctx_output_map_open(ctx, "command", 2);

    grn_ctx_output_cstr(ctx, "id");
    grn_ctx_output_uint32(ctx, command_id);

    grn_ctx_output_cstr(ctx, "name");
    grn_ctx_output_str(ctx, name, name_size);

    /* TODO: Output more information. */

    grn_ctx_output_map_close(ctx);

    if (is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
  }
  grn_ctx_output_map_close(ctx);

  GRN_OBJ_FIN(ctx, &command_ids);

  return NULL;
}

void
grn_proc_init_command_list(grn_ctx *ctx)
{
  grn_plugin_command_create(ctx,
                            "command_list",
                            -1,
                            command_command_list,
                            0,
                            NULL);
}
