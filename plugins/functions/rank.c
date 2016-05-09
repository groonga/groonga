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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_rank
#endif

#include <groonga/plugin.h>

static grn_obj *
func_rank(grn_ctx *ctx, int n_args, grn_obj **args,
          grn_user_data *user_data)
{
  grn_obj *rank;
  grn_obj *expression;
  grn_obj *record;
  grn_id id;

  if (n_args != 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "rank(): wrong number of arguments (%d for 0)",
                     n_args);
    return NULL;
  }

  grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);
  if (!expression) {
    return NULL;
  }

  record = grn_expr_get_var_by_offset(ctx, expression, 0);
  if (!record) {
    return NULL;
  }

  id = GRN_RECORD_VALUE(record);
  if (id == GRN_ID_NIL) {
    return NULL;
  }

  rank = grn_plugin_proc_alloc(ctx,
                               user_data,
                               GRN_DB_UINT32,
                               0);
  if (!rank) {
    return NULL;
  }

  GRN_UINT32_SET(ctx, rank, id);

  return rank;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  grn_proc_create(ctx,
                  "rank", -1,
                  GRN_PROC_FUNCTION,
                  func_rank,
                  NULL, NULL, 0, NULL);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
