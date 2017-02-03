/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2017 Brazil

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

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_JSON_LOAD_OPEN_BRACKET 0x40000000
#define GRN_JSON_LOAD_OPEN_BRACE   0x40000001

GRN_API void grn_load_(grn_ctx *ctx, grn_content_type input_type,
                       const char *table, unsigned int table_len,
                       const char *columns, unsigned int columns_len,
                       const char *values, unsigned int values_len,
                       const char *ifexists, unsigned int ifexists_len,
                       const char *each, unsigned int each_len,
                       grn_obj *output_ids,
                       uint32_t emit_level);

#ifdef __cplusplus
}
#endif
