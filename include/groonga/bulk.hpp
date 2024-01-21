// Copyright (C) 2020-2023  Sutou Kouhei <kou@clear-code.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

#pragma once

#include <cmath>
#include <cstring>
#include <string>

namespace grn {
  namespace numeric {
#ifdef GRN_HAVE_BFLOAT16
    template <typename TYPE>
    grn_bfloat16
    to_bfloat16(TYPE value)
    {
      if constexpr (std::is_same_v<TYPE, grn_bfloat16>) {
        return value;
      } else {
        grn_bfloat16 value_bfloat16 = 0;
        float value_float = value;
        std::memcpy(&value_bfloat16,
                    reinterpret_cast<char *>(&value_float) + sizeof(float) -
                      sizeof(grn_bfloat16),
                    sizeof(grn_bfloat16));
        return value_bfloat16;
      }
    }

    inline float
    bfloat16_to_float32(grn_bfloat16 value)
    {
      float value_float = 0;
      std::memcpy(reinterpret_cast<char *>(&value_float) + sizeof(float) -
                    sizeof(grn_bfloat16),
                  &value,
                  sizeof(grn_bfloat16));
      return value_float;
    }
#endif

    template <typename TYPE>
    std::enable_if_t<std::is_integral_v<TYPE>, bool>
    is_zero(TYPE value)
    {
      return value == 0;
    }

    template <typename TYPE>
    std::enable_if_t<std::is_floating_point_v<TYPE>, bool>
    is_zero(TYPE value)
    {
      return std::abs(value) < std::numeric_limits<TYPE>::epsilon();
    }

#ifdef GRN_HAVE_BFLOAT16
    template <typename TYPE>
    std::enable_if_t<std::is_same_v<TYPE, grn_bfloat16>, bool>
    is_zero(TYPE value)
    {
      return is_zero(bfloat16_to_float32(value));
    }
#endif
  }; // namespace numeric

  namespace bulk {
    template <typename TYPE>
    TYPE
    get(grn_ctx *ctx, grn_obj *bulk, TYPE default_value)
    {
      switch (bulk->header.domain) {
      case GRN_DB_BOOL:
        return GRN_BOOL_VALUE(bulk);
      case GRN_DB_INT8:
        return GRN_INT8_VALUE(bulk);
      case GRN_DB_UINT8:
        return GRN_UINT8_VALUE(bulk);
      case GRN_DB_INT16:
        return GRN_INT16_VALUE(bulk);
      case GRN_DB_UINT16:
        return GRN_UINT16_VALUE(bulk);
      case GRN_DB_INT32:
        return GRN_INT32_VALUE(bulk);
      case GRN_DB_UINT32:
        return GRN_UINT32_VALUE(bulk);
      case GRN_DB_INT64:
        return static_cast<TYPE>(GRN_INT64_VALUE(bulk));
      case GRN_DB_UINT64:
        return static_cast<TYPE>(GRN_UINT64_VALUE(bulk));
#ifdef GRN_HAVE_BFLOAT16
      case GRN_DB_BFLOAT16:
        return static_cast<TYPE>(GRN_BFLOAT16_VALUE(bulk));
#endif
      case GRN_DB_FLOAT32:
        return static_cast<TYPE>(GRN_FLOAT32_VALUE(bulk));
      case GRN_DB_FLOAT:
        return static_cast<TYPE>(GRN_FLOAT_VALUE(bulk));
      case GRN_DB_TIME:
        return static_cast<TYPE>(GRN_TIME_VALUE(bulk));
      default:
        return default_value;
      }
    }

    template <typename NUMERIC>
    grn_rc
    set(grn_ctx *ctx, grn_obj *bulk, NUMERIC value)
    {
      switch (bulk->header.domain) {
      case GRN_DB_BOOL:
        GRN_BOOL_SET(ctx, bulk, !numeric::is_zero(value));
        break;
      case GRN_DB_INT8:
        GRN_INT8_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT8:
        GRN_UINT8_SET(ctx, bulk, value);
        break;
      case GRN_DB_INT16:
        GRN_INT16_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT16:
        GRN_UINT16_SET(ctx, bulk, value);
        break;
      case GRN_DB_INT32:
        GRN_INT32_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT32:
        GRN_UINT32_SET(ctx, bulk, value);
        break;
      case GRN_DB_INT64:
        GRN_INT64_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT64:
        GRN_UINT64_SET(ctx, bulk, value);
        break;
#ifdef GRN_HAVE_BFLOAT16
      case GRN_DB_BFLOAT16:
        GRN_BFLOAT16_SET(ctx, bulk, numeric::to_bfloat16(value));
        break;
#endif
      case GRN_DB_FLOAT32:
        GRN_FLOAT32_SET(ctx, bulk, value);
        break;
      case GRN_DB_FLOAT:
        GRN_FLOAT_SET(ctx, bulk, value);
        break;
      case GRN_DB_TIME:
        if constexpr (std::is_floating_point_v<NUMERIC>) {
          GRN_TIME_SET(ctx, bulk, std::llround(value * GRN_TIME_USEC_PER_SEC));
        } else {
          GRN_TIME_SET(ctx, bulk, value);
        }
      default:
        break;
      }
      return ctx->rc;
    }
  }; // namespace bulk

  class TextBulk {
  public:
    TextBulk(grn_ctx *ctx) : ctx_(ctx) { GRN_TEXT_INIT(&bulk_, 0); }

    ~TextBulk() { GRN_OBJ_FIN(ctx_, &bulk_); }

    grn_obj *
    operator*()
    {
      return &bulk_;
    }

    std::string
    value()
    {
      return std::string(GRN_TEXT_VALUE(&bulk_), GRN_TEXT_LEN(&bulk_));
    };

  private:
    grn_ctx *ctx_;
    grn_obj bulk_;
  };
} // namespace grn
