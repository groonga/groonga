/*
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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

namespace grn {
  class SharedObj {
  public:
    SharedObj(grn_ctx *ctx, grn_obj *obj) :
      ctx_(ctx),
      obj_(obj) {
    }

    SharedObj(grn_ctx *ctx, const char *name, int name_size) :
      ctx_(ctx),
      obj_(grn_ctx_get(ctx_, name, name_size)) {
    }

    SharedObj(grn_ctx *ctx, grn_id id) :
      ctx_(ctx),
      obj_(grn_ctx_at(ctx_, id)) {
    }

    SharedObj(SharedObj &&shared_obj) :
      ctx_(shared_obj.ctx_),
      obj_(shared_obj.obj_) {
      grn_obj_refer(ctx_, obj_);
    }

    ~SharedObj() {
      if (obj_) {
        grn_obj_unref(ctx_, obj_);
      }
    }

    grn_obj *get() {
      return obj_;
    }

    grn_obj *release() {
      grn_obj *obj = obj_;
      obj_ = nullptr;
      return obj;
    }

    void reset(grn_obj *obj) {
      obj_ = obj;
    }

  private:
    grn_ctx *ctx_;
    grn_obj *obj_;
  };

  class UniqueObj {
  public:
    UniqueObj(grn_ctx *ctx, grn_obj *obj) :
      ctx_(ctx),
      obj_(obj) {
    }

    UniqueObj(UniqueObj &&unique_obj) :
      ctx_(unique_obj.ctx_),
      obj_(unique_obj.obj_) {
      unique_obj.obj_ = nullptr;
    }

    ~UniqueObj() {
      if (obj_) {
        grn_obj_close(ctx_, obj_);
      }
    }

    grn_obj *get() {
      return obj_;
    }

    grn_obj *release() {
      grn_obj *obj = obj_;
      obj_ = nullptr;
      return obj;
    }

    void reset(grn_obj *obj) {
      obj_ = obj;
    }

  private:
    grn_ctx *ctx_;
    grn_obj *obj_;
  };
}
