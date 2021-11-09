/*
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

#include "grn.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define GRN_TYPE_SHORT_TEXT_SIZE GRN_TABLE_MAX_KEY_SIZE
#define GRN_TYPE_TEXT_SIZE (1U << 16)
#define GRN_TYPE_LONG_TEXT_SIZE (1U << 31)

grn_obj *
grn_type_create_internal(grn_ctx *ctx,
                         grn_id id,
                         grn_obj_flags flags,
                         unsigned int size);

#ifdef __cplusplus
}
#endif
