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

#include "grn_sorter.h"
#include "grn_expr.h"

grn_obj *
grn_sorter_data_get_table(grn_ctx *ctx, grn_sorter_data *data)
{
  return data->table;
}

size_t
grn_sorter_data_get_offset(grn_ctx *ctx, grn_sorter_data *data)
{
  return data->offset;
}

size_t
grn_sorter_data_get_limit(grn_ctx *ctx, grn_sorter_data *data)
{
  return data->limit;
}

bool
grn_sorter_data_is_ascending(grn_ctx *ctx, grn_sorter_data *data)
{
  return !(data->keys[0].flags & GRN_TABLE_SORT_DESC);
}

grn_table_sort_key *
grn_sorter_data_get_keys(grn_ctx *ctx, grn_sorter_data *data, size_t *n_keys)
{
  *n_keys = data->n_keys;
  return data->keys;
}

grn_obj *
grn_sorter_data_get_result(grn_ctx *ctx, grn_sorter_data *data)
{
  return data->result;
}

grn_obj *
grn_sorter_data_get_sorter(grn_ctx *ctx, grn_sorter_data *data)
{
  return data->sorter;
}

grn_obj **
grn_sorter_data_get_args(grn_ctx *ctx, grn_sorter_data *data, size_t *n_args)
{
  *n_args = GRN_PTR_VECTOR_SIZE(&(data->args));
  return (grn_obj **)GRN_BULK_HEAD(&(data->args));
}

static bool
grn_sorter_data_extract(grn_ctx *ctx, grn_sorter_data *data)
{
  grn_obj *first_key = data->keys[0].key;
  if (!grn_obj_is_expr(ctx, first_key)) {
    return false;
  }

  grn_expr *expr = (grn_expr *)first_key;
  grn_expr_code *codes = expr->codes;
  uint32_t codes_curr = expr->codes_curr;
  if (codes_curr < 3) {
    return false;
  }
  if (codes[0].op != GRN_OP_PUSH) {
    return false;
  }
  if (!grn_obj_is_sorter_proc(ctx, codes[0].value)) {
    return false;
  }

  data->sorter = codes[0].value;
  if (codes[0].modify == 0) {
    return false;
  }
  uint32_t call_i = codes[0].modify;
  if (codes[call_i].op != GRN_OP_CALL) {
    return false;
  }
  if (call_i != codes_curr - 1) {
    if (call_i + 1 == codes_curr - 1 && codes[call_i + 1].op == GRN_OP_MINUS) {
      /* -sorter(...): OK */
      data->keys[0].flags = GRN_TABLE_SORT_DESC;
    } else {
      return false;
    }
  }

  for (uint32_t i = 1; i < call_i; i++) {
    switch (codes[i].op) {
    case GRN_OP_GET_VALUE:
    case GRN_OP_PUSH:
      if (codes[i].modify == 0) {
        GRN_PTR_PUT(ctx, &(data->args), codes[i].value);
      } else {
        uint32_t sub_call_i = i + codes[i].modify;
        if (codes[sub_call_i].op != GRN_OP_CALL) {
          return false;
        }
        /* Literal argument expressions are only supported for
         * now. Nested function call and so on aren't supported
         * yet. */
        if (!grn_obj_is_function_proc(ctx, codes[i].value)) {
          return false;
        }
        for (uint32_t j = i + 1; j < sub_call_i; j++) {
          if (codes[j].op != GRN_OP_PUSH) {
            return false;
          }
          grn_obj *sub_arg = codes[j].value;
          if (!sub_arg) {
            return false;
          }
          switch (sub_arg->header.type) {
          case GRN_BULK:
          case GRN_VECTOR:
          case GRN_UVECTOR:
          case GRN_PVECTOR:
            /* supported */
            break;
          default:
            return false;
          }
        }
        expr->codes += i;
        expr->codes_curr = call_i - i + 1;
        grn_expr_executor executor;
        grn_expr_executor_init(ctx, &executor, (grn_obj *)expr);
        grn_obj *arg = grn_expr_executor_exec(ctx, &executor, GRN_ID_NIL);
        GRN_PTR_PUT(ctx, &(data->args), arg);
        grn_expr_executor_fin(ctx, &executor);
        expr->codes = codes;
        expr->codes_curr = codes_curr;
        i += codes[i].modify;
      }
      break;
    default:
      return false;
    }
  }

  return true;
}

bool
grn_sorter_data_init(grn_ctx *ctx,
                     grn_sorter_data *data,
                     grn_obj *table,
                     size_t offset,
                     size_t limit,
                     grn_table_sort_key *keys,
                     size_t n_keys,
                     grn_obj *result)
{
  data->table = table;
  data->offset = offset;
  data->limit = limit;
  data->keys = keys;
  data->n_keys = n_keys;
  data->result = result;
  data->sorter = NULL;
  GRN_PTR_INIT(&(data->args), GRN_OBJ_VECTOR, GRN_ID_NIL);

  return grn_sorter_data_extract(ctx, data);
}

void
grn_sorter_data_fin(grn_ctx *ctx, grn_sorter_data *data)
{
  GRN_OBJ_FIN(ctx, &(data->args));
}

grn_rc
grn_sorter_data_run(grn_ctx *ctx, grn_sorter_data *data)
{
  grn_proc *proc = (grn_proc *)(data->sorter);
  return proc->callbacks.function.sorter(ctx, data);
}
