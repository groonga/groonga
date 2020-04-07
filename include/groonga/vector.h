/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018  Brazil
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

#include <groonga.h>

#ifdef __cplusplus
extern "C" {
#endif

GRN_API unsigned int grn_vector_size(grn_ctx *ctx, grn_obj *vector);

GRN_API grn_rc grn_vector_add_element(grn_ctx *ctx, grn_obj *vector,
                                      const char *str, unsigned int str_len,
                                      unsigned int weight, grn_id domain);

GRN_API unsigned int grn_vector_get_element(grn_ctx *ctx, grn_obj *vector,
                                            unsigned int offset, const char **str,
                                            unsigned int *weight, grn_id *domain);
GRN_API bool grn_vector_get_element_bool(grn_ctx *ctx,
                                         grn_obj *vector,
                                         unsigned int offset,
                                         bool default_value);
GRN_API int8_t grn_vector_get_element_int8(grn_ctx *ctx,
                                           grn_obj *vector,
                                           unsigned int offset,
                                           int8_t default_value);
GRN_API uint8_t grn_vector_get_element_uint8(grn_ctx *ctx,
                                             grn_obj *vector,
                                             unsigned int offset,
                                             uint8_t default_value);
GRN_API int16_t grn_vector_get_element_int16(grn_ctx *ctx,
                                             grn_obj *vector,
                                             unsigned int offset,
                                             int16_t default_value);
GRN_API uint16_t grn_vector_get_element_uint16(grn_ctx *ctx,
                                               grn_obj *vector,
                                               unsigned int offset,
                                               uint16_t default_value);
GRN_API int32_t grn_vector_get_element_int32(grn_ctx *ctx,
                                             grn_obj *vector,
                                             unsigned int offset,
                                             int32_t default_value);
GRN_API uint32_t grn_vector_get_element_uint32(grn_ctx *ctx,
                                               grn_obj *vector,
                                               unsigned int offset,
                                               uint32_t default_value);
GRN_API int64_t grn_vector_get_element_int64(grn_ctx *ctx,
                                             grn_obj *vector,
                                             unsigned int offset,
                                             int64_t default_value);
GRN_API uint64_t grn_vector_get_element_uint64(grn_ctx *ctx,
                                               grn_obj *vector,
                                               unsigned int offset,
                                               uint64_t default_value);

GRN_API unsigned int grn_vector_pop_element(grn_ctx *ctx, grn_obj *vector,
                                            const char **str,
                                            unsigned int *weight,
                                            grn_id *domain);

#ifdef __cplusplus
}
#endif
