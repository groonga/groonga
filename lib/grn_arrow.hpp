/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019-2020 Sutou Kouhei <kou@clear-code.com>

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

#include <arrow/api.h>

namespace grnarrow {
  grn_rc status_to_rc(const arrow::Status &status);
  arrow::Status check(grn_ctx *ctx,
                      grn_rc rc,
                      const char *context);
  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             const char *context);
  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             const std::string &context);
  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             std::ostream &output);

  template <typename TYPE>
  bool check(grn_ctx *ctx,
             arrow::Result<TYPE> &result,
             const char *context) {
    return check(ctx, result.status(), context);
  }

  template <typename TYPE>
  bool check(grn_ctx *ctx,
             arrow::Result<TYPE> &result,
             const std::string &context) {
    return check(ctx, result.status(), context);
  }

  template <typename TYPE>
  bool check(grn_ctx *ctx,
             arrow::Result<TYPE> &result,
             std::ostream &output) {
    return check(ctx, result.status(), output);
  }

  template <typename ... ARGS>
  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             ARGS&&... args) {
    auto context = ::arrow::util::StringBuilder(std::forward<ARGS>(args)...);
    return check(ctx, status, context.c_str());
  }

  template <typename TYPE, typename ... ARGS>
  bool check(grn_ctx *ctx,
             arrow::Result<TYPE> &result,
             ARGS&&... args) {
    auto context = ::arrow::util::StringBuilder(std::forward<ARGS>(args)...);
    return check(ctx, result.status(), context.c_str());
  }
}
