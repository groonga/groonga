/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019-2020  Sutou Kouhei <kou@clear-code.com>

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
  namespace {
    struct Int32Handler : public rapidjson::BaseReaderHandler<>
    {
      grn_ctx *ctx_;
      grn_obj *uvector_;

      Int32Handler(grn_ctx *ctx,
                   grn_obj *uvector,
                   bool add_record_if_not_exist)
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

    struct TableHandler : public rapidjson::BaseReaderHandler<>
    {
      TableHandler(grn_ctx *ctx,
                   grn_obj *uvector,
                   bool add_record_if_not_exist)
        : ctx_(ctx),
          uvector_(uvector),
          add_record_if_not_exist_(add_record_if_not_exist),
          table_(grn_ctx_at(ctx, uvector->header.domain)),
          weight_(0.0) {
      }

      ~TableHandler() {
        grn_obj_unref(ctx_, table_);
      }

      bool Default() {return false;}

      bool String(const char *data, size_t size, bool copy) {
        grn_id id;
        if (add_record_if_not_exist_) {
          id = grn_table_add(ctx_, table_, data, size, NULL);
        } else {
          id = grn_table_get(ctx_, table_, data, size);
        }
        if (id == GRN_ID_NIL) {
          return false;
        }
        auto rc = grn_uvector_add_element_record(ctx_, uvector_, id, weight_);
        return rc == GRN_SUCCESS;
      }

      bool Int(int value) {
        weight_ = value;
        return true;
      }

      bool Uint(unsigned int value) {
        weight_ = value;
        return true;
      }

      bool Double(double value) {
        weight_ = value;
        return true;
      }

    private:
      grn_ctx *ctx_;
      grn_obj *uvector_;
      bool add_record_if_not_exist_;
      grn_obj *table_;
      float weight_;
    };

    template <typename Handler>
    grn_rc
    json_to_uvector(grn_ctx *ctx,
                    rapidjson::Document *document,
                    grn_obj *dest,
                    bool add_record_if_not_exist) {
      Handler handler(ctx, dest, add_record_if_not_exist);
      if (document->IsArray()) {
        auto n = document->Size();
        for (size_t i = 0; i < n; ++i) {
          const auto &element = (*document)[i];
          if (!element.Accept(handler)) {
            return GRN_INVALID_ARGUMENT;
          }
        }
        return GRN_SUCCESS;
      } else if (document->IsObject()) {
        for (auto member = document->MemberBegin();
             member != document->MemberEnd();
             ++member) {
          if (!member->name.IsString()) {
            return GRN_INVALID_ARGUMENT;
          }
          if (!member->value.IsNumber()) {
            return GRN_INVALID_ARGUMENT;
          }
          if (!member->value.Accept(handler)) {
            return GRN_INVALID_ARGUMENT;
          }
          if (!member->name.Accept(handler)) {
            return GRN_INVALID_ARGUMENT;
          }
        }
        return GRN_SUCCESS;
      } else {
        return GRN_INVALID_ARGUMENT;
      }
    }
  }

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
      auto domain = grn_ctx_at(ctx, dest->header.domain);
      grn_rc rc = GRN_INVALID_ARGUMENT;
      if (grn_obj_is_lexicon(ctx, domain)) {
        grn_obj dest_record;
        GRN_RECORD_INIT(&dest_record, GRN_BULK, dest->header.domain);
        rc = grn_obj_cast(ctx, src, &dest_record, add_record_if_not_exist);
        if (rc == GRN_SUCCESS && GRN_BULK_VSIZE(&dest_record) > 0) {
          auto id = GRN_RECORD_VALUE(&dest_record);
          GRN_RECORD_PUT(ctx, dest, id);
        }
        GRN_OBJ_FIN(ctx, &dest_record);
      }
      grn_obj_unref(ctx, domain);
      return rc;
    }
    switch (dest->header.domain) {
    case GRN_DB_INT32 :
      return json_to_uvector<Int32Handler>(ctx,
                                           &document,
                                           dest,
                                           add_record_if_not_exist);
    default :
      {
        grn_rc rc = GRN_INVALID_ARGUMENT;
        auto domain = grn_ctx_at(ctx, dest->header.domain);
        if (grn_obj_is_lexicon(ctx, domain)) {
          rc = json_to_uvector<TableHandler>(ctx,
                                             &document,
                                             dest,
                                             add_record_if_not_exist);
        }
        grn_obj_unref(ctx, domain);
        return rc;
      }
    }
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
