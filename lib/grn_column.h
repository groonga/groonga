/*
  Copyright(C) 2017  Brazil
  Copyright(C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_store.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct _grn_column_cache {
  grn_ra *ra;
  grn_ra_cache ra_cache;
  grn_obj *accessor;
};

grn_obj *
grn_column_cast_value(grn_ctx *ctx,
                      grn_obj *column,
                      grn_obj *value,
                      grn_obj *buffer,
                      int set_flags);

#ifdef __cplusplus
}
#endif
