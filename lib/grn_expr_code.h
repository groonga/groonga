/*
  Copyright(C) 2015-2018  Brazil
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_EXPR_CODE_RELATIONAL_EXPRESSION (0x01)

typedef struct {
  grn_obj *value;
  int32_t nargs;
  grn_operator op;
  uint8_t flags;
  int32_t modify;
} grn_expr_code;

uint32_t grn_expr_code_n_used_codes(grn_ctx *ctx,
                                    grn_expr_code *start,
                                    grn_expr_code *target);

#ifdef __cplusplus
}
#endif
