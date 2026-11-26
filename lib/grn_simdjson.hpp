/*
  Copyright (C) 2026  YOLOW92

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

#include "grn_ctx_impl.h"

#ifdef GRN_WITH_SIMDJSON
#  include <simdjson.h>

#  include <cstddef>
#  include <new>
#  include <optional>
#endif

namespace grn {
  namespace simdjson_parser {
    inline void
    init(grn_ctx *ctx)
    {
#ifdef GRN_WITH_SIMDJSON
      ctx->impl->simdjson.parser = nullptr;
      ctx->impl->simdjson.parser_in_use = false;
#else
      (void)ctx;
#endif
    }

    inline void
    fin(grn_ctx *ctx)
    {
#ifdef GRN_WITH_SIMDJSON
      if (ctx->impl->simdjson.parser) {
        delete static_cast<::simdjson::ondemand::parser *>(
          ctx->impl->simdjson.parser);
        ctx->impl->simdjson.parser = nullptr;
      }
      ctx->impl->simdjson.parser_in_use = false;
#else
      (void)ctx;
#endif
    }
  } // namespace simdjson_parser

#ifdef GRN_WITH_SIMDJSON
  class ScopedSIMDJSONParser {
  public:
    // simdjson::ondemand::parser retains ~5.67x capacity() bytes of
    // internal buffers (string_buf, structural_indexes, and fixed stacks).
    // 64 KiB covers ShortText (<= 4 KiB) and Text (<= 64 KiB) inputs while
    // bounding per-context retained memory to ~372 KiB.
    static constexpr size_t max_retained_capacity = 64 * 1024;

    explicit ScopedSIMDJSONParser(grn_ctx *ctx, size_t document_size = 0)
      : ctx_(ctx),
        parser_(nullptr),
        local_parser_(),
        owns_ctx_parser_(false)
    {
      if (ctx_ && ctx_->impl && !ctx_->impl->simdjson.parser_in_use &&
          document_size <= max_retained_capacity) {
        if (!ctx_->impl->simdjson.parser) {
          ctx_->impl->simdjson.parser =
            new (std::nothrow) ::simdjson::ondemand::parser();
        }
        if (ctx_->impl->simdjson.parser) {
          ctx_->impl->simdjson.parser_in_use = true;
          parser_ = static_cast<::simdjson::ondemand::parser *>(
            ctx_->impl->simdjson.parser);
          owns_ctx_parser_ = true;
          return;
        }
      }
      local_parser_.emplace();
      parser_ = &(*local_parser_);
    }

    ~ScopedSIMDJSONParser()
    {
      if (owns_ctx_parser_) {
        if (parser_->capacity() > max_retained_capacity) {
          delete parser_;
          ctx_->impl->simdjson.parser = nullptr;
        }
        ctx_->impl->simdjson.parser_in_use = false;
      }
    }

    ScopedSIMDJSONParser(const ScopedSIMDJSONParser &) = delete;
    ScopedSIMDJSONParser &
    operator=(const ScopedSIMDJSONParser &) = delete;
    ScopedSIMDJSONParser(ScopedSIMDJSONParser &&) = delete;
    ScopedSIMDJSONParser &
    operator=(ScopedSIMDJSONParser &&) = delete;

    ::simdjson::ondemand::parser &
    operator*()
    {
      return *parser_;
    }

    ::simdjson::ondemand::parser *
    operator->()
    {
      return parser_;
    }

    ::simdjson::ondemand::parser &
    get()
    {
      return *parser_;
    }

  private:
    grn_ctx *ctx_;
    ::simdjson::ondemand::parser *parser_;
    std::optional<::simdjson::ondemand::parser> local_parser_;
    bool owns_ctx_parser_;
  };
#endif
} // namespace grn
