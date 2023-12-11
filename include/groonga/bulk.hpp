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

#include <string>

namespace grn {
  namespace bulk {
    template <typename TYPE>
    grn_rc
    set(grn_ctx *ctx, grn_obj *bulk, TYPE value)
    {
      return grn_bulk_write_from(ctx,
                                 bulk,
                                 reinterpret_cast<char *>(&value),
                                 0,
                                 sizeof(TYPE));
    }

    template <typename TYPE>
    TYPE
    get(grn_obj *bulk)
    {
      return *reinterpret_cast<TYPE *>(GRN_BULK_HEAD(bulk));
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
