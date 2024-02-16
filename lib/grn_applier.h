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

#include "grn.h"
#include "grn_ctx_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_applier_data {
  grn_obj *applier;
  grn_obj *table;
  grn_obj *output_column;
  grn_obj args;
};

void
grn_applier_data_init(grn_ctx *ctx,
                      grn_applier_data *data,
                      grn_obj *table,
                      grn_obj *output_column);
void
grn_applier_data_fin(grn_ctx *ctx, grn_applier_data *data);
bool
grn_applier_data_extract(grn_ctx *ctx, grn_applier_data *data, grn_obj *expr);

grn_rc
grn_applier_data_run(grn_ctx *ctx, grn_applier_data *data);

#ifdef __cplusplus
}
#endif
