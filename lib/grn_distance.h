/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

void
grn_distance_init_external_libraries(void);
void
grn_distance_init_from_env(void);

grn_rc
grn_distance_cosine_applier(grn_ctx *ctx, grn_applier_data *data);
grn_rc
grn_distance_inner_product_applier(grn_ctx *ctx, grn_applier_data *data);
grn_rc
grn_distance_l1_norm_applier(grn_ctx *ctx, grn_applier_data *data);
grn_rc
grn_distance_l2_norm_squared_applier(grn_ctx *ctx, grn_applier_data *data);

#ifdef __cplusplus
}
#endif
