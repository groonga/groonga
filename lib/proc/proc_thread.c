/*
  Copyright(C) 2009-2018  Brazil
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

#include "../grn_ctx.h"
#include "../grn_output.h"
#include "../grn_proc.h"
#include "../grn_logger.h"
#include "../grn_str.h"

#include <groonga/plugin.h>

static grn_obj *
command_thread_limit(grn_ctx *ctx,
                     int n_args,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  grn_obj *max_bulk;
  uint32_t current_limit;

  current_limit = grn_thread_get_limit_with_ctx(ctx);
  GRN_OUTPUT_INT64(current_limit);

  max_bulk = grn_plugin_proc_get_var(ctx, user_data, "max", -1);
  if (GRN_TEXT_LEN(max_bulk) > 0) {
    uint32_t max;
    const char *max_text = GRN_TEXT_VALUE(max_bulk);
    const char *max_text_end;
    const char *max_text_rest;

    max_text_end = max_text + GRN_TEXT_LEN(max_bulk);
    max = grn_atoui(max_text, max_text_end, &max_text_rest);
    if (max_text_rest != max_text_end) {
      ERR(GRN_INVALID_ARGUMENT,
          "[thread_limit] max must be unsigned integer value: <%.*s>",
          (int)GRN_TEXT_LEN(max_bulk),
          max_text);
      return NULL;
    }
    if (max == 0) {
      ERR(GRN_INVALID_ARGUMENT,
          "[thread_limit] max must be 1 or larger: <%.*s>",
          (int)GRN_TEXT_LEN(max_bulk),
          max_text);
      return NULL;
    }
    grn_thread_set_limit_with_ctx(ctx, max);
  }

  return NULL;
}

void
grn_proc_init_thread_limit(grn_ctx *ctx)
{
  grn_expr_var vars[1];
  grn_plugin_expr_var_init(ctx, &(vars[0]), "max", -1);
  grn_plugin_command_create(ctx,
                            "thread_limit", -1,
                            command_thread_limit,
                            1,
                            vars);
}
