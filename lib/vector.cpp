/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Brazil

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
#include "grn_ctx.h"

namespace grn {
  template <typename NUMERIC>
  NUMERIC
  vector_get_element(grn_ctx *ctx,
                     grn_obj *vector,
                     unsigned int offset,
                     NUMERIC default_value)
  {
    const char *raw_value = NULL;
    grn_id domain;
    unsigned int length;
    NUMERIC value = default_value;

    GRN_API_ENTER;

    length = grn_vector_get_element(ctx,
                                    vector,
                                    offset,
                                    &raw_value,
                                    NULL,
                                    &domain);
    if (length > 0) {
      switch (domain) {
      case GRN_DB_INT8 :
        value = *reinterpret_cast<const int8_t *>(raw_value);
        break;
      case GRN_DB_UINT8 :
        value = *reinterpret_cast<const uint8_t *>(raw_value);
        break;
      case GRN_DB_INT16 :
        value = *reinterpret_cast<const int16_t *>(raw_value);
        break;
      case GRN_DB_UINT16 :
        value = *reinterpret_cast<const uint16_t *>(raw_value);
        break;
      case GRN_DB_INT32 :
        value = *reinterpret_cast<const int32_t *>(raw_value);
        break;
      case GRN_DB_UINT32 :
        value = *reinterpret_cast<const uint32_t *>(raw_value);
        break;
      case GRN_DB_INT64 :
        value = *reinterpret_cast<const int64_t *>(raw_value);
        break;
      case GRN_DB_UINT64 :
        value = *reinterpret_cast<const uint64_t *>(raw_value);
        break;
      case GRN_DB_FLOAT :
        value = *reinterpret_cast<const double *>(raw_value);
        break;
      default :
        break;
      }
    }

    GRN_API_RETURN(value);
  }
}

extern "C" {
  int8_t
  grn_vector_get_element_int8(grn_ctx *ctx,
                              grn_obj *vector,
                              unsigned int offset,
                              int8_t default_value)
  {
    return grn::vector_get_element<int8_t>(ctx, vector, offset, default_value);
  }

  uint8_t
  grn_vector_get_element_uint8(grn_ctx *ctx,
                               grn_obj *vector,
                               unsigned int offset,
                               uint8_t default_value)
  {
    return grn::vector_get_element<uint8_t>(ctx, vector, offset, default_value);
  }

  int16_t
  grn_vector_get_element_int16(grn_ctx *ctx,
                               grn_obj *vector,
                               unsigned int offset,
                               int16_t default_value)
  {
    return grn::vector_get_element<int16_t>(ctx, vector, offset, default_value);
  }

  uint16_t
  grn_vector_get_element_uint16(grn_ctx *ctx,
                                grn_obj *vector,
                                unsigned int offset,
                                uint16_t default_value)
  {
    return grn::vector_get_element<uint16_t>(ctx, vector, offset, default_value);
  }

  int32_t
  grn_vector_get_element_int32(grn_ctx *ctx,
                               grn_obj *vector,
                               unsigned int offset,
                               int32_t default_value)
  {
    return grn::vector_get_element<int32_t>(ctx, vector, offset, default_value);
  }

  uint32_t
  grn_vector_get_element_uint32(grn_ctx *ctx,
                                grn_obj *vector,
                                unsigned int offset,
                                uint32_t default_value)
  {
    return grn::vector_get_element<uint32_t>(ctx, vector, offset, default_value);
  }

  int64_t
  grn_vector_get_element_int64(grn_ctx *ctx,
                               grn_obj *vector,
                               unsigned int offset,
                               int64_t default_value)
  {
    return grn::vector_get_element<int64_t>(ctx, vector, offset, default_value);
  }

  uint64_t
  grn_vector_get_element_uint64(grn_ctx *ctx,
                                grn_obj *vector,
                                unsigned int offset,
                                uint64_t default_value)
  {
    return grn::vector_get_element<uint64_t>(ctx, vector, offset, default_value);
  }
}
