/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>

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

#include "grn_cast.h"

#ifdef GRN_WITH_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>

namespace grn {
  struct Int32Handler : public rapidjson::BaseReaderHandler<>
  {
    grn_ctx *ctx_;
    grn_obj *uvector_;

    Int32Handler(grn_ctx *ctx,
                 grn_obj *uvector)
      : ctx_(ctx),
        uvector_(uvector) {
    }

    bool Default() {return false;}

    bool Int(int value) {
      GRN_INT32_PUT(ctx_, uvector_, value);
      return true;
    }

    bool Uint(unsigned int value) {
      return Int(value);
    }

    bool Int64(int64_t value) {
      return Int(value);
    }

    bool Uint64(uint64_t value) {
      return Int(value);
    }
  };

  grn_rc
  cast_text_to_uvector(grn_ctx *ctx,
                       grn_obj *src,
                       grn_obj *dest,
                       bool add_record_if_not_exist)
  {
    rapidjson::Document document;
    rapidjson::MemoryStream stream(GRN_TEXT_VALUE(src), GRN_TEXT_LEN(src));
    document.ParseStream(stream);
    if (document.HasParseError()) {
      return GRN_INVALID_ARGUMENT;
    }
    if (!document.IsArray()) {
      return GRN_INVALID_ARGUMENT;
    }
    if (dest->header.domain != GRN_DB_INT32) {
      return GRN_INVALID_ARGUMENT;
    }
    auto n = document.Size();
    Int32Handler handler(ctx, dest);
    for (size_t i = 0; i < n; ++i) {
      const auto &element = document[i];
      if (!element.Accept(handler)) {
        return GRN_INVALID_ARGUMENT;
      }
    }
    return GRN_SUCCESS;
  }
}
#endif

extern "C" {
  grn_rc
  grn_obj_cast_text_to_uvector(grn_ctx *ctx,
                               grn_obj *src,
                               grn_obj *dest,
                               bool add_record_if_not_exist)
  {
#ifdef GRN_WITH_RAPIDJSON
    return grn::cast_text_to_uvector(ctx, src, dest, add_record_if_not_exist);
#else
    return GRN_INVALID_ARGUMENT;
#endif
  }
}
