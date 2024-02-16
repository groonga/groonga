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

#pragma once

#include <groonga.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_applier_data grn_applier_data;

GRN_API grn_obj *
grn_applier_data_get_applier(grn_ctx *ctx, grn_applier_data *data);
GRN_API grn_obj *
grn_applier_data_get_table(grn_ctx *ctx, grn_applier_data *data);
GRN_API grn_obj *
grn_applier_data_get_output_column(grn_ctx *ctx, grn_applier_data *data);
GRN_API grn_obj **
grn_applier_data_get_args(grn_ctx *ctx, grn_applier_data *data, size_t *n_args);

typedef grn_rc
grn_applier_func(grn_ctx *ctx, grn_applier_data *data);

GRN_API grn_rc
grn_proc_set_applier(grn_ctx *ctx, grn_obj *proc, grn_applier_func applier);

#ifdef __cplusplus
}
#endif
