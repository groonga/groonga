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

#include "grn.h"
#include "grn_float.h"

#include <math.h>
#include <float.h>

#ifdef GRN_HAVE_BFLOAT16
float
grn_bfloat16_to_float32(grn_bfloat16 value)
{
  float value_float = 0;
  memcpy(((char *)(&value_float)) + sizeof(float) - sizeof(grn_bfloat16), &value, sizeof(grn_bfloat16));
  return value_float;
}

grn_bfloat16
grn_float32_to_bfloat16(float value)
{
  grn_bfloat16 value_bfloat16 = 0;
  memcpy(&value_bfloat16,
         ((char *)&value) + sizeof(float) - sizeof(grn_bfloat16),
         sizeof(grn_bfloat16));
  return value_bfloat16;
}

bool
grn_bfloat16_is_zero(grn_bfloat16 value)
{
  return grn_float32_is_zero(grn_bfloat16_to_float32(value));
}
#endif

bool
grn_float32_is_zero(float value)
{
  return fabsf(value) < FLT_EPSILON;
}

bool
grn_float_is_zero(double value)
{
  return fabs(value) < DBL_EPSILON;
}
