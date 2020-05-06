/* -*- c-basic-offset: 2 -*- */
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

grn_rc grn_vector_delimit(grn_ctx *ctx,
                          grn_obj *vector,
                          float weight,
                          grn_id domain);
grn_obj *grn_vector_pack(grn_ctx *ctx,
                         grn_obj *vector,
                         uint32_t offset,
                         uint32_t n,
                         bool is_weight_float32,
                         grn_obj *header,
                         grn_obj *footer);
grn_rc grn_vector_unpack(grn_ctx *ctx,
                         grn_obj *vector,
                         const char *data,
                         uint32_t data_size,
                         bool is_weight_float32);

grn_obj *grn_vector_body(grn_ctx *ctx, grn_obj *vector);

#ifdef __cplusplus
}
#endif
