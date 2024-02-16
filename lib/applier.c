/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_applier.h"
#include "grn_expr.h"

grn_obj *
grn_applier_data_get_applier(grn_ctx *ctx, grn_applier_data *data)
{
  return data->applier;
}

grn_obj *
grn_applier_data_get_table(grn_ctx *ctx, grn_applier_data *data)
{
  return data->table;
}

grn_obj *
grn_applier_data_get_output_column(grn_ctx *ctx, grn_applier_data *data)
{
  return data->output_column;
}

grn_obj **
grn_applier_data_get_args(grn_ctx *ctx, grn_applier_data *data, size_t *n_args)
{
  *n_args = GRN_PTR_VECTOR_SIZE(&(data->args));
  return (grn_obj **)GRN_BULK_HEAD(&(data->args));
}

void
grn_applier_data_init(grn_ctx *ctx,
                      grn_applier_data *data,
                      grn_obj *table,
                      grn_obj *output_column)
{
  data->table = table;
  data->output_column = output_column;
  data->applier = NULL;
  GRN_PTR_INIT(&(data->args), GRN_OBJ_VECTOR, GRN_ID_NIL);
}

void
grn_applier_data_fin(grn_ctx *ctx, grn_applier_data *data)
{
  GRN_OBJ_FIN(ctx, &(data->args));
}

bool
grn_applier_data_extract(grn_ctx *ctx, grn_applier_data *data, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  if (e->codes_curr < 3) {
    return false;
  }
  if (e->codes[0].op != GRN_OP_PUSH) {
    return false;
  }
  if (!grn_obj_is_applier_proc(ctx, e->codes[0].value)) {
    return false;
  }
  data->applier = e->codes[0].value;
  for (uint32_t i = 1; i < e->codes_curr - 1; i++) {
    switch (e->codes[0].op) {
    case GRN_OP_GET_VALUE:
    case GRN_OP_PUSH:
      GRN_PTR_PUT(ctx, &(data->args), e->codes[i].value);
      break;
    default:
      return false;
    }
  }
  if (e->codes[e->codes_curr - 1].op != GRN_OP_CALL) {
    return false;
  }
  return true;
}

grn_rc
grn_applier_data_run(grn_ctx *ctx, grn_applier_data *data)
{
  grn_proc *proc = (grn_proc *)(data->applier);
  return proc->callbacks.function.applier(ctx, data);
}
