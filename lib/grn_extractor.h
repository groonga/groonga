/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga/extractor.h>

#ifdef __cplusplus
extern "C" {
#endif

struct grn_extract_data {
  grn_obj *table;
  uint32_t index;
  grn_obj *value;
};

grn_rc
grn_extract_data_init(grn_ctx *ctx, grn_extract_data *data);

grn_obj *
grn_extractor_extract(grn_ctx *ctx, grn_obj *extractor, grn_extract_data *data);

#ifdef __cplusplus
}
#endif
