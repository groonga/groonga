/*
  Copyright (C) 2021-2022  Sutou Kouhei <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
  GRN_TABLE_SORT_KEYS_PARSE_MODE_SORT,
  GRN_TABLE_SORT_KEYS_PARSE_MODE_GROUP,
} grn_table_sort_keys_parse_mode;

grn_table_sort_key *
grn_table_sort_keys_parse_internal(grn_ctx *ctx,
                                   grn_obj *table,
                                   const char *raw_keys,
                                   int32_t raw_keys_length,
                                   uint32_t *n_keys,
                                   grn_table_sort_keys_parse_mode mode);

#ifdef __cplusplus
}
#endif
