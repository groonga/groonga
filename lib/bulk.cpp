/*
  Copyright (C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_bulk.h"

namespace grn {
  namespace bulk {
    template <typename NUMERIC>
    NUMERIC
    get(grn_ctx *ctx, grn_obj *bulk, NUMERIC default_value) {
      switch (bulk->header.domain) {
      case GRN_DB_INT8 :
        return GRN_INT8_VALUE(bulk);
      case GRN_DB_UINT8 :
        return GRN_UINT8_VALUE(bulk);
      case GRN_DB_INT16 :
        return GRN_INT16_VALUE(bulk);
      case GRN_DB_UINT16 :
        return GRN_UINT16_VALUE(bulk);
      case GRN_DB_INT32 :
        return GRN_INT32_VALUE(bulk);
      case GRN_DB_UINT32 :
        return GRN_UINT32_VALUE(bulk);
      case GRN_DB_INT64 :
        return static_cast<NUMERIC>(GRN_INT64_VALUE(bulk));
      case GRN_DB_UINT64 :
        return static_cast<NUMERIC>(GRN_UINT64_VALUE(bulk));
      case GRN_DB_FLOAT32 :
        return static_cast<NUMERIC>(GRN_FLOAT32_VALUE(bulk));
      case GRN_DB_FLOAT :
        return static_cast<NUMERIC>(GRN_FLOAT_VALUE(bulk));
      default :
        return default_value;
      }
    }

    template <typename NUMERIC>
    grn_rc
    set(grn_ctx *ctx, grn_obj *bulk, NUMERIC value) {
      switch (bulk->header.domain) {
      case GRN_DB_INT8 :
        GRN_INT8_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT8 :
        GRN_UINT8_SET(ctx, bulk, value);
        break;
      case GRN_DB_INT16 :
        GRN_INT16_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT16 :
        GRN_UINT16_SET(ctx, bulk, value);
        break;
      case GRN_DB_INT32 :
        GRN_INT32_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT32 :
        GRN_UINT32_SET(ctx, bulk, value);
        break;
      case GRN_DB_INT64 :
        GRN_INT64_SET(ctx, bulk, value);
        break;
      case GRN_DB_UINT64 :
        GRN_UINT64_SET(ctx, bulk, value);
        break;
      case GRN_DB_FLOAT32 :
        GRN_FLOAT32_SET(ctx, bulk, value);
        break;
      case GRN_DB_FLOAT :
        GRN_FLOAT_SET(ctx, bulk, value);
        break;
      default :
        break;
      }
      return ctx->rc;
    }
  };
};

extern "C" {
  int64_t
  grn_bulk_get_int64(grn_ctx *ctx, grn_obj *bulk)
  {
    return grn::bulk::get<int64_t>(ctx, bulk, 0);
  }

  grn_rc
  grn_bulk_set_int64(grn_ctx *ctx, grn_obj *bulk, int64_t value)
  {
    return grn::bulk::set<int64_t>(ctx, bulk, value);
  }

  double
  grn_bulk_get_float(grn_ctx *ctx, grn_obj *bulk)
  {
    return grn::bulk::get<double>(ctx, bulk, 0);
  }

  grn_rc
  grn_bulk_set_float(grn_ctx *ctx, grn_obj *bulk, double value)
  {
    return grn::bulk::set<double>(ctx, bulk, value);
  }

  grn_rc
  grn_bulk_copy(grn_ctx *ctx, grn_obj *bulk, grn_obj *dest)
  {
    unsigned char flags = 0;
    switch (bulk->header.type) {
    case GRN_PVECTOR :
    case GRN_UVECTOR :
    case GRN_VECTOR :
      flags += GRN_OBJ_VECTOR;
      break;
    default :
      break;
    }
    grn_obj_reinit(ctx,
                   dest,
                   bulk->header.domain,
                   flags);
    return grn_obj_cast(ctx, bulk, dest, false);
  }
}
