/*
  Copyright (C) 2021-2024  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx.h"
#include "grn_task_executor.hpp"

GRN_API grn::TaskExecutor *
grn_ctx_get_task_executor(grn_ctx *ctx);

namespace grn {
  class ChildCtxReleaser {
  public:
    ChildCtxReleaser(grn_ctx *ctx, grn_ctx *child_ctx)
      : ctx_(ctx),
        child_ctx_(child_ctx) {};

    ~ChildCtxReleaser()
    {
      if (child_ctx_) {
        grn_ctx_release_child(ctx_, child_ctx_);
      }
    }

  private:
    grn_ctx *ctx_;
    grn_ctx *child_ctx_;
  };
} // namespace grn
