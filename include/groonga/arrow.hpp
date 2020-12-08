/*
  Copyright(C) 2017  Brazil
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <groonga.hpp>
#include <arrow/array.h>

namespace grn {
  namespace arrow {
    class ArrayBuilder {
    public:
      ArrayBuilder(grn_ctx *ctx);
      ~ArrayBuilder();

      ::arrow::Status add_column(grn_obj *column,
                                 grn_table_cursor *cursor);
      ::arrow::Result<std::shared_ptr<::arrow::Array>> finish();

    private:
      struct Impl;
      std::unique_ptr<Impl> impl_;
    };
  }
}
