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

#include <groonga/plugin.h>

static grn_obj *
command_log_level(grn_ctx *ctx,
                  int n_args,
                  grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *level_name = grn_plugin_proc_get_var(ctx,
                                                user_data,
                                                "level", -1);
  if (GRN_TEXT_LEN(level_name) > 0) {
    grn_log_level max_level;
    GRN_TEXT_PUTC(ctx, level_name, '\0');
    if (grn_log_level_parse(GRN_TEXT_VALUE(level_name), &max_level)) {
      grn_logger_set_max_level(ctx, max_level);
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "invalid log level: <%s>", GRN_TEXT_VALUE(level_name));
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "log level is missing");
  }
  GRN_OUTPUT_BOOL(ctx->rc == GRN_SUCCESS);
  return NULL;
}

void
grn_proc_init_log_level(grn_ctx *ctx)
{
  grn_expr_var vars[1];
  grn_plugin_expr_var_init(ctx, &(vars[0]), "level", -1);
  grn_plugin_command_create(ctx,
                            "log_level", -1,
                            command_log_level,
                            1,
                            vars);
}

static grn_obj *
command_log_put(grn_ctx *ctx,
                int n_args,
                grn_obj **args,
                grn_user_data *user_data)
{
  grn_obj *level_name = grn_plugin_proc_get_var(ctx,
                                                user_data,
                                                "level", -1);
  grn_obj *message = grn_plugin_proc_get_var(ctx,
                                             user_data,
                                             "message", -1);
  if (GRN_TEXT_LEN(level_name) > 0) {
    grn_log_level level;
    GRN_TEXT_PUTC(ctx, level_name, '\0');
    if (grn_log_level_parse(GRN_TEXT_VALUE(level_name), &level)) {
      GRN_LOG(ctx, level, "%.*s",
              (int)GRN_TEXT_LEN(message),
              GRN_TEXT_VALUE(message));
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "invalid log level: <%s>", GRN_TEXT_VALUE(level_name));
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "log level is missing");
  }
  GRN_OUTPUT_BOOL(ctx->rc == GRN_SUCCESS);
  return NULL;
}

void
grn_proc_init_log_put(grn_ctx *ctx)
{
  grn_expr_var vars[2];
  grn_plugin_expr_var_init(ctx, &(vars[0]), "level", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "message", -1);
  grn_plugin_command_create(ctx,
                            "log_put", -1,
                            command_log_put,
                            2,
                            vars);
}

static grn_obj *
command_log_reopen(grn_ctx *ctx,
                   int n_args,
                   grn_obj **args,
                   grn_user_data *user_data)
{
  grn_log_reopen(ctx);
  GRN_OUTPUT_BOOL(ctx->rc == GRN_SUCCESS);
  return NULL;
}

void
grn_proc_init_log_reopen(grn_ctx *ctx)
{
  grn_plugin_command_create(ctx,
                            "log_reopen", -1,
                            command_log_reopen,
                            0,
                            NULL);
}
