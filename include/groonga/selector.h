/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include <groonga.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef grn_rc grn_selector_func(grn_ctx *ctx, grn_obj *table, grn_obj *index,
                                 int nargs, grn_obj **args,
                                 grn_obj *res, grn_operator op);

GRN_API grn_rc grn_proc_set_selector(grn_ctx *ctx, grn_obj *proc,
                                     grn_selector_func selector);
GRN_API grn_rc grn_proc_set_selector_operator(grn_ctx *ctx,
                                              grn_obj *proc,
                                              grn_operator selector_op);
GRN_API grn_operator grn_proc_get_selector_operator(grn_ctx *ctx,
                                                    grn_obj *proc);

typedef struct _grn_selector_data grn_selector_data;

GRN_API grn_selector_data *
grn_selector_data_get(grn_ctx *ctx);
GRN_API grn_obj *
grn_selector_data_get_selector(grn_ctx *ctx,
                               grn_selector_data *data);
GRN_API grn_obj *
grn_selector_data_get_expr(grn_ctx *ctx,
                           grn_selector_data *data);
GRN_API grn_obj *
grn_selector_data_get_table(grn_ctx *ctx,
                            grn_selector_data *data);
GRN_API grn_obj *
grn_selector_data_get_index(grn_ctx *ctx,
                            grn_selector_data *data);
GRN_API grn_obj **
grn_selector_data_get_args(grn_ctx *ctx,
                           grn_selector_data *data,
                           size_t *n_args);
GRN_API grn_obj *
grn_selector_data_get_res(grn_ctx *ctx,
                          grn_selector_data *data);
GRN_API grn_operator
grn_selector_data_get_op(grn_ctx *ctx,
                         grn_selector_data *data);


#ifdef __cplusplus
}
#endif
