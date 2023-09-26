/*
  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_str.h>
#include <groonga/plugin.h>

#include <string.h>

static grn_obj *
func_str_len(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  unsigned int length = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 1) {
    grn_obj *text = args[0];
    length = grn_str_len(ctx,
                         GRN_TEXT_VALUE(text),
                         GRN_TEXT_VALUE(text) + GRN_TEXT_LEN(text),
                         ctx->encoding,
                         NULL);
  }
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, length);
  }
  return obj;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_proc_create(ctx, "str_len", strlen("str_len"), GRN_PROC_FUNCTION,
                  func_str_len, NULL, NULL, 0, NULL);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
