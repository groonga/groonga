/*
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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

static grn_obj *
command_wal_recover(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  uint32_t thread_limit = grn_thread_get_limit();
  if (thread_limit != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[wal][recover] must be the thread limit is 1: <%u>",
                     thread_limit);
    goto exit;
  }

  grn_db *db = (grn_db *)grn_ctx_db(ctx);
  grn_db_wal_recover(ctx, db);

exit :
  grn_ctx_output_bool(ctx, (ctx->rc == GRN_SUCCESS));
  return NULL;
}

void
grn_proc_init_wal_recover(grn_ctx *ctx)
{
  grn_plugin_command_create(ctx,
                            "wal_recover", -1,
                            command_wal_recover,
                            0,
                            NULL);
}
