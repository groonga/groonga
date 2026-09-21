/*
  Copyright (C) 2020-2026  Sutou Kouhei <kou@clear-code.com>

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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GRN_HAVE_BFLOAT16
/**
 * \deprecated Since 16.1.1. Use `(float)value` instead.
 */
GRN_API float
grn_bfloat16_to_float32(grn_bfloat16 value);
/**
 * \deprecated Since 16.1.1. Use `(grn_bfloat16)value` instead.
 */
GRN_API grn_bfloat16
grn_float32_to_bfloat16(float value);
#endif

#ifdef __cplusplus
}
#endif
