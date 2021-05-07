/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2020-2021  Sutou Kouhei <kou@clear-code.com>

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

GRN_API uint32_t grn_vector_size(grn_ctx *ctx, grn_obj *vector);

GRN_API grn_rc grn_vector_add_element(grn_ctx *ctx,
                                      grn_obj *vector,
                                      const char *str,
                                      uint32_t str_len,
                                      uint32_t weight,
                                      grn_id domain);
GRN_API grn_rc grn_vector_add_element_float(grn_ctx *ctx,
                                            grn_obj *vector,
                                            const char *str,
                                            uint32_t str_len,
                                            float weight,
                                            grn_id domain);

GRN_API uint32_t grn_vector_get_element(grn_ctx *ctx,
                                        grn_obj *vector,
                                        uint32_t offset,
                                        const char **str,
                                        uint32_t *weight,
                                        grn_id *domain);
GRN_API uint32_t grn_vector_get_element_float(grn_ctx *ctx,
                                              grn_obj *vector,
                                              uint32_t offset,
                                              const char **str,
                                              float *weight,
                                              grn_id *domain);
GRN_API bool grn_vector_get_element_bool(grn_ctx *ctx,
                                         grn_obj *vector,
                                         uint32_t offset,
                                         bool default_value);
GRN_API int8_t grn_vector_get_element_int8(grn_ctx *ctx,
                                           grn_obj *vector,
                                           uint32_t offset,
                                           int8_t default_value);
GRN_API uint8_t grn_vector_get_element_uint8(grn_ctx *ctx,
                                             grn_obj *vector,
                                             uint32_t offset,
                                             uint8_t default_value);
GRN_API int16_t grn_vector_get_element_int16(grn_ctx *ctx,
                                             grn_obj *vector,
                                             uint32_t offset,
                                             int16_t default_value);
GRN_API uint16_t grn_vector_get_element_uint16(grn_ctx *ctx,
                                               grn_obj *vector,
                                               uint32_t offset,
                                               uint16_t default_value);
GRN_API int32_t grn_vector_get_element_int32(grn_ctx *ctx,
                                             grn_obj *vector,
                                             uint32_t offset,
                                             int32_t default_value);
GRN_API uint32_t grn_vector_get_element_uint32(grn_ctx *ctx,
                                               grn_obj *vector,
                                               uint32_t offset,
                                               uint32_t default_value);
GRN_API int64_t grn_vector_get_element_int64(grn_ctx *ctx,
                                             grn_obj *vector,
                                             uint32_t offset,
                                             int64_t default_value);
GRN_API uint64_t grn_vector_get_element_uint64(grn_ctx *ctx,
                                               grn_obj *vector,
                                               uint32_t offset,
                                               uint64_t default_value);
GRN_API float grn_vector_get_element_float32(grn_ctx *ctx,
                                             grn_obj *vector,
                                             uint32_t offset,
                                             float default_value);
GRN_API double grn_vector_get_element_float64(grn_ctx *ctx,
                                              grn_obj *vector,
                                              uint32_t offset,
                                              double default_value);

GRN_API uint32_t grn_vector_pop_element(grn_ctx *ctx,
                                        grn_obj *vector,
                                        const char **str,
                                        uint32_t *weight,
                                        grn_id *domain);
GRN_API uint32_t grn_vector_pop_element_float(grn_ctx *ctx,
                                              grn_obj *vector,
                                              const char **str,
                                              float *weight,
                                              grn_id *domain);

GRN_API grn_rc grn_vector_copy(grn_ctx *ctx,
                               grn_obj *src,
                               grn_obj *dest);

GRN_API uint32_t grn_uvector_size(grn_ctx *ctx, grn_obj *uvector);
GRN_API uint32_t grn_uvector_element_size(grn_ctx *ctx, grn_obj *uvector);

GRN_API grn_rc grn_uvector_add_element(grn_ctx *ctx,
                                       grn_obj *uvector,
                                       grn_id id,
                                       uint32_t weight);

GRN_API grn_id grn_uvector_get_element(grn_ctx *ctx,
                                       grn_obj *uvector,
                                       uint32_t offset,
                                       uint32_t *weight);

GRN_API grn_rc grn_uvector_add_element_record(grn_ctx *ctx,
                                              grn_obj *uvector,
                                              grn_id id,
                                              float weight);
GRN_API grn_id grn_uvector_get_element_record(grn_ctx *ctx,
                                              grn_obj *uvector,
                                              uint32_t offset,
                                              float *weight);
GRN_API grn_rc grn_uvector_copy(grn_ctx *ctx,
                                grn_obj *src,
                                grn_obj *dest);

#ifdef __cplusplus
}
#endif
