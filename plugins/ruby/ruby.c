/* -*- c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
  Copyright(C) 2013 Brazil

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

#include <mrb.h>
#include <output.h>
#include <db.h>
#include <util.h>

#include <groonga/plugin.h>

#define VAR GRN_PROC_GET_VAR_BY_OFFSET

static void
output_result(grn_ctx *ctx, mrb_value result)
{
  grn_obj grn_result;

  GRN_OUTPUT_MAP_OPEN("result", 1);
  GRN_OUTPUT_CSTR("value");
  GRN_VOID_INIT(&grn_result);
  if (grn_mrb_to_grn(ctx, result, &grn_result) == GRN_SUCCESS) {
    GRN_OUTPUT_OBJ(&grn_result, NULL);
  } else {
    GRN_OUTPUT_CSTR("unsupported return value");
  }
  grn_obj_unlink(ctx, &grn_result);
  GRN_OUTPUT_MAP_CLOSE();
}

static grn_obj *
command_ruby_eval(grn_ctx *ctx, int nargs, grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *script;
  mrb_value result;

  script = VAR(0);
  switch (script->header.domain) {
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    break;
  default :
    {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, script);
      ERR(GRN_INVALID_ARGUMENT, "script must be a string: <%.*s>",
          (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
    break;
  }

  result = grn_mrb_eval(ctx, GRN_TEXT_VALUE(script), GRN_TEXT_LEN(script));
  output_result(ctx, result);

  return NULL;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

#define DEF_VAR(v,x) do {\
  (v).name = (x);\
  (v).name_size = (x) ? sizeof(x) - 1 : 0;\
  GRN_TEXT_INIT(&(v).value, 0);\
} while (0)

#define DEF_COMMAND(name, func, nvars, vars)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1),\
                   GRN_PROC_COMMAND, (func), NULL, NULL, (nvars), (vars)))

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_expr_var vars[1];

  DEF_VAR(vars[0], "script");
  DEF_COMMAND("ruby_eval", command_ruby_eval, 1, vars);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
