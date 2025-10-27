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

#include "grn.h"
#include "grn_ctx_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_sorter_data {
  grn_obj *table;
  size_t offset;
  size_t limit;
  grn_table_sort_key *keys;
  size_t n_keys;
  grn_obj *result;
  grn_obj *sorter;
  grn_obj args;
};

bool
grn_sorter_data_init(grn_ctx *ctx,
                     grn_sorter_data *data,
                     grn_obj *table,
                     size_t offset,
                     size_t limit,
                     grn_table_sort_key *keys,
                     size_t n_keys,
                     grn_obj *result);
void
grn_sorter_data_fin(grn_ctx *ctx, grn_sorter_data *data);

grn_rc
grn_sorter_data_run(grn_ctx *ctx, grn_sorter_data *data);

#ifdef __cplusplus
}
#endif
