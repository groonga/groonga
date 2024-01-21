/*
  Copyright (C) 2020-2024  Sutou Kouhei <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

int64_t grn_bulk_get_int64(grn_ctx *ctx, grn_obj *bulk, int64_t default_value, const char *tag);
grn_rc grn_bulk_set_int64(grn_ctx *ctx, grn_obj *bulk, int64_t value);
double grn_bulk_get_float(grn_ctx *ctx, grn_obj *bulk, double default_value, const char *tag);
grn_rc grn_bulk_set_float(grn_ctx *ctx, grn_obj *bulk, double value);
grn_rc grn_bulk_copy(grn_ctx *ctx, grn_obj *bulk, grn_obj *dest);

#ifdef __cplusplus
}
#endif
