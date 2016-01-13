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

#include "grn_proc.h"

#include <groonga/plugin.h>

static grn_obj *
command_conf_get(grn_ctx *ctx,
                 int nargs,
                 grn_obj **args,
                 grn_user_data *user_data)
{
  grn_obj *key;
  const char *value;
  uint32_t value_size;

  key = grn_plugin_proc_get_var(ctx, user_data, "key", -1);
  if (GRN_TEXT_LEN(key) == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[conf][get] key is missing");
    return NULL;
  }

  grn_conf_get(ctx,
               GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key),
               &value, &value_size);
  if (ctx->rc) {
    return NULL;
  }

  grn_ctx_output_str(ctx, value, value_size);

  return NULL;
}

static grn_obj *
command_conf_set(grn_ctx *ctx,
                 int nargs,
                 grn_obj **args,
                 grn_user_data *user_data)
{
  grn_obj *key;
  grn_obj *value;

  key = grn_plugin_proc_get_var(ctx, user_data, "key", -1);
  if (GRN_TEXT_LEN(key) == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[conf][set] key is missing");
    return NULL;
  }

  value = grn_plugin_proc_get_var(ctx, user_data, "value", -1);
  grn_conf_set(ctx,
               GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key),
               GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));

  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  return NULL;
}

static grn_obj *
command_conf_delete(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *key;

  key = grn_plugin_proc_get_var(ctx, user_data, "key", -1);
  if (GRN_TEXT_LEN(key) == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[conf][delete] key is missing");
    return NULL;
  }

  grn_conf_delete(ctx,
                  GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key));

  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  return NULL;
}

void
grn_proc_init_conf_get(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "key", -1);
  grn_plugin_command_create(ctx,
                            "conf_get", -1,
                            command_conf_get,
                            1,
                            vars);
}

void
grn_proc_init_conf_set(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "key", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "value", -1);
  grn_plugin_command_create(ctx,
                            "conf_set", -1,
                            command_conf_set,
                            2,
                            vars);
}

void
grn_proc_init_conf_delete(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "key", -1);
  grn_plugin_command_create(ctx,
                            "conf_delete", -1,
                            command_conf_delete,
                            1,
                            vars);
}
