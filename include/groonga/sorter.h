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

#pragma once

#include <groonga.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_sorter_data grn_sorter_data;

GRN_API grn_obj *
grn_sorter_data_get_table(grn_ctx *ctx, grn_sorter_data *data);
GRN_API size_t
grn_sorter_data_get_offset(grn_ctx *ctx, grn_sorter_data *data);
GRN_API size_t
grn_sorter_data_get_limit(grn_ctx *ctx, grn_sorter_data *data);
GRN_API bool
grn_sorter_data_is_ascending(grn_ctx *ctx, grn_sorter_data *data);
GRN_API grn_obj *
grn_sorter_data_get_result(grn_ctx *ctx, grn_sorter_data *data);
GRN_API grn_obj *
grn_sorter_data_get_sorter(grn_ctx *ctx, grn_sorter_data *data);
GRN_API grn_obj **
grn_sorter_data_get_args(grn_ctx *ctx, grn_sorter_data *data, size_t *n_args);

/* TODO:
 * GRN_API bool
 * grn_sorter_data_is_greater(grn_ctx *ctx, grn_sorter_data *data, grn_id id):
 *
 * Returns whether the next sort key is greater or not. Sorter can
 * call this when the target record's value is the same value. This is
 * needed on multi column sort.
 */

typedef grn_rc
grn_sorter_func(grn_ctx *ctx, grn_sorter_data *data);

GRN_API grn_rc
grn_proc_set_sorter(grn_ctx *ctx, grn_obj *proc, grn_sorter_func sorter);

#ifdef __cplusplus
}
#endif
