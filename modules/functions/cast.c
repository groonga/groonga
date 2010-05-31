/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "proc.h"
#include "ql.h"
#include "db.h"
#include "util.h"

static grn_obj *
func_cast(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 2 && GRN_DB_OBJP(args[1])) {
    obj = grn_expr_alloc(ctx, caller, DB_OBJ(args[1])->id, 0);
    grn_obj_cast(ctx, args[0], obj, 0);
  } else {
    obj = grn_expr_alloc(ctx, caller, GRN_DB_INT32, 0);
  }
  return obj;
}

grn_rc
grn_module_init_cast(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
grn_module_register_cast(grn_ctx *ctx)
{
  grn_proc_create(ctx, "cast", 4, GRN_PROC_FUNCTION, func_cast, NULL, NULL, 0, NULL);

  return ctx->rc;
}

grn_rc
grn_module_fin_cast(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
