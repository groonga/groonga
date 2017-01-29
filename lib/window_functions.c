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

#include "grn_db.h"
#include "grn_window_functions.h"

static grn_rc
window_function_record_number(grn_ctx *ctx,
                              grn_obj *output_column,
                              grn_window *window,
                              grn_obj **args,
                              int n_args)
{
  grn_id id;
  uint32_t nth_record = 1;
  grn_obj value;

  GRN_UINT32_INIT(&value, 0);
  while ((id = grn_window_next(ctx, window))) {
    GRN_UINT32_SET(ctx, &value, nth_record);
    grn_obj_set_value(ctx, output_column, id, &value, GRN_OBJ_SET);
    nth_record++;
  }
  GRN_OBJ_FIN(ctx, &value);

  return GRN_SUCCESS;
}

grn_rc
grn_db_init_builtin_window_functions(grn_ctx *ctx)
{
  grn_window_function_create(ctx,
                             "record_number", -1,
                             window_function_record_number);
  return GRN_SUCCESS;
}
