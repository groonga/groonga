/*
  Copyright (C) 2020-2023  Sutou Kouhei <kou@clear-code.com>

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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GRN_HAVE_BFLOAT16
float
grn_bfloat16_to_float32(grn_bfloat16 value);
grn_bfloat16
grn_float32_to_bfloat16(float value);
#endif

#ifdef __cplusplus
}
#endif
