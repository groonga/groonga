/*
  Copyright (C) 2019-2023  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_geo.h"
#include "grn_pat.h"
#include "grn_str.h"

#include <groonga/bulk.hpp>

#ifdef GRN_WITH_RAPIDJSON
#  include "grn_db.h"
#  include <groonga/smart_obj.hpp>

#  include <limits>
#  include <string>
#  include <vector>

#  define RAPIDJSON_NAMESPACE grn_rapidjson
#  include <rapidjson/document.h>
#  include <rapidjson/memorystream.h>

namespace {
  struct Int8Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    Int8Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      GRN_INT8_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Uint(unsigned int value)
    {
      return Int(value);
    }

    bool
    Int64(int64_t value)
    {
      return Int(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Int(value);
    }
  };

  struct UInt8Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    UInt8Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Uint(value);
    }

    bool
    Uint(unsigned int value)
    {
      GRN_UINT8_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Int64(int64_t value)
    {
      return Uint(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Uint(value);
    }
  };

  struct Int16Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    Int16Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      GRN_INT16_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Uint(unsigned int value)
    {
      return Int(value);
    }

    bool
    Int64(int64_t value)
    {
      return Int(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Int(value);
    }
  };

  struct UInt16Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    UInt16Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Uint(value);
    }

    bool
    Uint(unsigned int value)
    {
      GRN_UINT16_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Int64(int64_t value)
    {
      return Uint(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Uint(value);
    }
  };

  struct Int32Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    Int32Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      GRN_INT32_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Uint(unsigned int value)
    {
      return Int(value);
    }

    bool
    Int64(int64_t value)
    {
      return Int(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Int(value);
    }
  };

  struct UInt32Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    UInt32Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Uint(value);
    }

    bool
    Uint(unsigned int value)
    {
      GRN_UINT32_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Int64(int64_t value)
    {
      return Uint(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Uint(value);
    }
  };

  struct Int64Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    Int64Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Int64(value);
    }

    bool
    Uint(unsigned int value)
    {
      return Int64(value);
    }

    bool
    Int64(int64_t value)
    {
      GRN_INT64_PUT(ctx_, caster_->dest, value);
      return true;
    }

    bool
    Uint64(uint64_t value)
    {
      return Int64(value);
    }
  };

  struct UInt64Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    UInt64Handler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Uint64(value);
    }

    bool
    Uint(unsigned int value)
    {
      return Uint64(value);
    }

    bool
    Int64(int64_t value)
    {
      return Uint64(value);
    }

    bool
    Uint64(uint64_t value)
    {
      GRN_UINT64_PUT(ctx_, caster_->dest, value);
      return true;
    }
  };

  struct FloatHandler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    FloatHandler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Double(value);
    }

    bool
    Uint(unsigned int value)
    {
      return Double(value);
    }

    bool
    Int64(int64_t value)
    {
      return Double(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Double(value);
    }

    bool
    Double(double value)
    {
      GRN_FLOAT_PUT(ctx_, caster_->dest, value);
      return true;
    }
  };

  struct Float32Handler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    Float32Handler(grn_ctx *ctx, grn_caster *caster)
      : ctx_(ctx),
        caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    Int(int value)
    {
      return Double(value);
    }

    bool
    Uint(unsigned int value)
    {
      return Double(value);
    }

    bool
    Int64(int64_t value)
    {
      return Double(value);
    }

    bool
    Uint64(uint64_t value)
    {
      return Double(value);
    }

    bool
    Double(double value)
    {
      GRN_FLOAT32_PUT(ctx_, caster_->dest, value);
      return true;
    }
  };

  struct TableWeightHandler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    TableWeightHandler(grn_ctx *ctx, grn_caster *caster)
      : ctx_(ctx),
        caster_(caster),
        table_(grn_ctx_at(ctx, caster->dest->header.domain)),
        weight_(0.0)
    {
    }

    ~TableWeightHandler() { grn_obj_unref(ctx_, table_); }

    bool
    Default()
    {
      return false;
    }

    bool
    String(const char *data, size_t size, bool copy)
    {
      uint32_t missing_mode = (caster_->flags & GRN_OBJ_MISSING_MASK);
      grn_id id;
      if (missing_mode == GRN_OBJ_MISSING_ADD) {
        id = grn_table_add(ctx_, table_, data, size, NULL);
      } else {
        id = grn_table_get(ctx_, table_, data, size);
      }
      if (id == GRN_ID_NIL) {
        uint32_t invalid_mode = (caster_->flags & GRN_OBJ_INVALID_MASK);
        if (invalid_mode != GRN_OBJ_INVALID_ERROR) {
          ERRCLR(ctx_);
        }
        grn_ctx *ctx = ctx_;
        grn_obj src;
        GRN_TEXT_INIT(&src, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET(ctx, &src, data, size);
        grn_caster element_caster = {
          &src,
          caster_->dest,
          caster_->flags,
          caster_->target,
        };
        CAST_FAILED(&element_caster);
        if (ctx->rc == GRN_SUCCESS) {
          if (missing_mode != GRN_OBJ_MISSING_NIL) {
            return true;
          }
        } else {
          ERRCLR(ctx);
          return true;
        }
      }
      auto rc =
        grn_uvector_add_element_record(ctx_, caster_->dest, id, weight_);
      return rc == GRN_SUCCESS;
    }

    bool
    Int(int value)
    {
      weight_ = value;
      return true;
    }

    bool
    Uint(unsigned int value)
    {
      weight_ = value;
      return true;
    }

    bool
    Double(double value)
    {
      weight_ = value;
      return true;
    }

  private:
    grn_ctx *ctx_;
    grn_caster *caster_;
    grn_obj *table_;
    float weight_;
  };

  class Array {
  public:
    Array(grn_ctx *ctx) : ctx_(ctx), values_() {}

    ~Array()
    {
      for (auto value : values_) {
        grn_obj_close(ctx_, value);
      }
    }

    bool
    add_bool_value(const bool value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_BOOL);
      GRN_BOOL_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_int32_value(const int32_t value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT32);
      GRN_INT32_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_uint32_value(const uint32_t value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_UINT32);
      GRN_UINT32_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_int64_value(const int64_t value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT64);
      GRN_INT64_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_uint64_value(const uint64_t value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_UINT64);
      GRN_UINT64_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_double_value(const double value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_FLOAT);
      GRN_FLOAT_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_string_value(const std::string &value)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
      GRN_TEXT_SET(ctx_, bulk, value.data(), value.size());
      values_.push_back(bulk);
      return true;
    }

    bool
    add_record_value(grn_id domain, grn_id id)
    {
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, domain);
      GRN_RECORD_SET(ctx_, bulk, id);
      values_.push_back(bulk);
      return true;
    }

    const std::vector<grn_obj *> &
    values() const
    {
      return values_;
    }

  private:
    grn_ctx *ctx_;
    std::vector<grn_obj *> values_;
  };

  class Record {
  public:
    Record(grn_ctx *ctx, grn_obj *table)
      : ctx_(ctx),
        table_(table),
        _id_index_(std::numeric_limits<size_t>::max()),
        _key_index_(std::numeric_limits<size_t>::max()),
        _id_(GRN_ID_NIL),
        keys_(),
        values_()
    {
    }

    ~Record()
    {
      for (auto value : values_) {
        grn_obj_close(ctx_, value);
      }
    }

    bool
    add_key(const std::string &key)
    {
      if (key == GRN_COLUMN_NAME_ID) {
        if (_id_index_ != std::numeric_limits<size_t>::max()) {
          return false;
        }
        if (_key_index_ != std::numeric_limits<size_t>::max()) {
          return false;
        }
        _id_index_ = keys_.size();
      } else if (key == GRN_COLUMN_NAME_KEY) {
        if (_id_index_ != std::numeric_limits<size_t>::max()) {
          return false;
        }
        if (_key_index_ != std::numeric_limits<size_t>::max()) {
          return false;
        }
        _key_index_ = keys_.size();
      }
      keys_.push_back(key);
      return true;
    }

    bool
    add_bool_value(const bool value)
    {
      if (_id_index_ == keys_.size() - 1) {
        return false;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_BOOL);
      GRN_BOOL_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_int32_value(const int32_t value)
    {
      if (_id_index_ == keys_.size() - 1) {
        if (value == 0) {
          return false;
        }
        _id_ = value;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT32);
      GRN_INT32_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_uint32_value(const uint32_t value)
    {
      if (_id_index_ == keys_.size() - 1) {
        if (value == 0) {
          return false;
        }
        _id_ = value;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_UINT32);
      GRN_UINT32_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_int64_value(const int64_t value)
    {
      if (_id_index_ == keys_.size() - 1) {
        if (value == 0) {
          return false;
        }
        _id_ = value;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT64);
      GRN_INT64_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_uint64_value(const uint64_t value)
    {
      if (_id_index_ == keys_.size() - 1) {
        if (value == 0) {
          return false;
        }
        _id_ = value;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_UINT64);
      GRN_UINT64_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_double_value(const double value)
    {
      if (_id_index_ == keys_.size() - 1) {
        return false;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_FLOAT);
      GRN_FLOAT_SET(ctx_, bulk, value);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_string_value(const std::string &value)
    {
      if (_id_index_ == keys_.size() - 1) {
        return false;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
      GRN_TEXT_SET(ctx_, bulk, value.data(), value.size());
      values_.push_back(bulk);
      return true;
    }

    bool
    add_record_value(grn_id domain, grn_id id)
    {
      if (_id_index_ == keys_.size() - 1) {
        return false;
      }
      auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, domain);
      GRN_RECORD_SET(ctx_, bulk, id);
      values_.push_back(bulk);
      return true;
    }

    bool
    add_array_value(const Array *array)
    {
      if (_id_index_ == keys_.size() - 1) {
        return false;
      }
      auto vector = grn_obj_open(ctx_, GRN_VECTOR, 0, GRN_ID_NIL);
      values_.push_back(vector);
      for (auto value : array->values()) {
        grn_vector_add_element(ctx_,
                               vector,
                               GRN_BULK_HEAD(value),
                               GRN_BULK_VSIZE(value),
                               0,
                               value->header.domain);
      }
      return true;
    }

    grn_obj *
    table() const
    {
      return table_;
    }

    grn_id
    id() const
    {
      return _id_;
    }

    bool
    have__id() const
    {
      return _id_index_ != std::numeric_limits<size_t>::max();
    }

    size_t
    _id_index() const
    {
      return _id_index_;
    }

    bool
    have__key() const
    {
      return _key_index_ != std::numeric_limits<size_t>::max();
    }

    size_t
    _key_index() const
    {
      return _key_index_;
    }

    const std::string &
    get_key(size_t i) const
    {
      return keys_[i];
    }

    grn_obj *
    get_value(size_t i)
    {
      return values_[i];
    }

  private:
    grn_ctx *ctx_;
    grn_obj *table_;
    size_t _id_index_;
    size_t _key_index_;
    grn_id _id_;
    std::vector<std::string> keys_;
    std::vector<grn_obj *> values_;
  };

  class TableHandler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
  public:
    TableHandler(grn_ctx *ctx, grn_caster *caster)
      : ctx_(ctx),
        caster_(caster),
        table_stack_(),
        record_stack_(),
        array_stack_(),
        container_type_stack_()
    {
      table_stack_.push_back(grn_ctx_at(ctx, caster_->dest->header.domain));
    }

    ~TableHandler()
    {
      for (auto table : table_stack_) {
        grn_obj_unref(ctx_, table);
      }
    }

    bool
    Default()
    {
      return false;
    }

    bool
    StartObject()
    {
      // Top level must be array.
      if (container_type_stack_.size() == 0) {
        return false;
      }
      // TODO: Nested object isn't supported yet.
      // Resolve the current key as a column name and find the target table.
      if (container_type_stack_.size() == 2) {
        return false;
      }
      container_type_stack_.push_back(ContainerType::OBJECT);
      const auto table = table_stack_.back();
      record_stack_.emplace_back(ctx_, table);
      return true;
    }

    bool
    Key(const char *data, size_t size, bool copy)
    {
      auto record = &(record_stack_.back());
      return record->add_key(std::string(data, size));
    }

    bool
    EndObject(size_t n)
    {
      auto record = &(record_stack_.back());
      auto table = record->table();
      grn_id domain = DB_OBJ(table)->id;
      grn_id id = GRN_ID_NIL;
      uint32_t missing_mode = (caster_->flags & GRN_OBJ_MISSING_MASK);
      if (record->have__id()) {
        id = record->id();
        if (id != GRN_ID_NIL) {
          if (grn_table_at(ctx_, table, id) != id) {
            id = GRN_ID_NIL;
          }
        }
      } else if (record->have__key()) {
        auto _key_index = record->_key_index();
        auto _key = record->get_value(_key_index);
        if (missing_mode == GRN_OBJ_MISSING_ADD) {
          id = grn_table_add_by_key(ctx_, table, _key, NULL);
        } else {
          id = grn_table_get_by_key(ctx_, table, _key);
        }
      } else {
        if (grn_obj_is_table_with_key(ctx_, table)) {
          return false;
        }
        if (missing_mode == GRN_OBJ_MISSING_ADD) {
          id = grn_table_add(ctx_, table, NULL, 0, NULL);
        }
      }
      if (id == GRN_ID_NIL) {
        if ((caster_->flags & GRN_OBJ_INVALID_MASK) == GRN_OBJ_INVALID_ERROR) {
          return false;
        }
        if (missing_mode == GRN_OBJ_MISSING_IGNORE) {
          return GRN_SUCCESS;
        }
      }
      for (size_t i = 0; i < n; ++i) {
        if (record->have__id() && i == record->_id_index()) {
          continue;
        }
        if (record->have__key() && i == record->_key_index()) {
          continue;
        }
        auto column_name = record->get_key(i);
        auto column =
          grn_obj_column(ctx_, table, column_name.data(), column_name.size());
        if (!column) {
          return false;
        }
        grn::SharedObj shared_column(ctx_, column);
        auto value = record->get_value(i);
        grn_obj_set_value(ctx_, column, id, value, GRN_OBJ_SET);
        if (ctx_->rc != GRN_SUCCESS) {
          return false;
        }
      }
      record_stack_.pop_back();
      container_type_stack_.pop_back();
      bool success = add_value(
        [&](Record *record) {
          record->add_record_value(domain, id);
          return true;
        },
        [&](Array *array) {
          array->add_record_value(domain, id);
          return true;
        });
      return success;
    }

    bool
    StartArray()
    {
      container_type_stack_.push_back(ContainerType::ARRAY);
      array_stack_.emplace_back(ctx_);
      return true;
    }

    bool
    EndArray(size_t n)
    {
      auto array = &(array_stack_.back());
      container_type_stack_.pop_back();
      if (container_type_stack_.empty()) {
        uint32_t missing_mode = (caster_->flags & GRN_OBJ_MISSING_MASK);
        for (auto value : array->values()) {
          grn_id id = GRN_ID_NIL;
          if (value->header.domain == caster_->dest->header.domain) {
            id = GRN_RECORD_VALUE(value);
          } else {
            grn_obj casted_value;
            GRN_RECORD_INIT(&casted_value,
                            GRN_BULK,
                            caster_->dest->header.domain);
            grn_caster value_caster = {
              value,
              &casted_value,
              caster_->flags,
              caster_->target,
            };
            auto rc = grn_caster_cast(ctx_, &value_caster);
            if (rc == GRN_SUCCESS && GRN_BULK_VSIZE(&casted_value) > 0) {
              id = GRN_RECORD_VALUE(&casted_value);
            }
            GRN_OBJ_FIN(ctx_, &casted_value);
          }
          if (id == GRN_ID_NIL) {
            if (ctx_->rc == GRN_SUCCESS) {
              if (missing_mode != GRN_OBJ_MISSING_NIL) {
                continue;
              }
            } else {
              ERRCLR(ctx_);
              continue;
            }
          }
          auto rc = grn_uvector_add_element_record(ctx_, caster_->dest, id, 0);
          if (rc != GRN_SUCCESS) {
            return false;
          }
        }
        return true;
      } else {
        const auto container_type = container_type_stack_.back();
        switch (container_type) {
        case ContainerType::OBJECT:
          {
            auto record = &(record_stack_.back());
            record->add_array_value(array);
            return true;
          }
        case ContainerType::ARRAY:
          // Nested array isn't supported.
          return false;
        default:
          return false;
        }
      }
      array_stack_.pop_back();
      return true;
    }

    bool
    Bool(bool value)
    {
      return add_value(
        [&](Record *record) {
          record->add_bool_value(value);
          return true;
        },
        [&](Array *array) {
          array->add_bool_value(value);
          return true;
        });
    }

    bool
    Int(int value)
    {
      return add_value(
        [&](Record *record) {
          record->add_int32_value(value);
          return true;
        },
        [&](Array *array) {
          array->add_int32_value(value);
          return true;
        });
    }

    bool
    Uint(unsigned int value)
    {
      return add_value(
        [&](Record *record) {
          record->add_uint32_value(value);
          return true;
        },
        [&](Array *array) {
          array->add_uint32_value(value);
          return true;
        });
    }

    bool
    Int64(int value)
    {
      return add_value(
        [&](Record *record) {
          record->add_int64_value(value);
          return true;
        },
        [&](Array *array) {
          array->add_int64_value(value);
          return true;
        });
    }

    bool
    Uint64(unsigned int value)
    {
      return add_value(
        [&](Record *record) {
          record->add_uint64_value(value);
          return true;
        },
        [&](Array *array) {
          array->add_uint64_value(value);
          return true;
        });
    }

    bool
    Double(double value)
    {
      return add_value(
        [&](Record *record) {
          record->add_double_value(value);
          return true;
        },
        [&](Array *array) {
          array->add_double_value(value);
          return true;
        });
    }

    bool
    String(const char *data, size_t size, bool copy)
    {
      return add_value(
        [&](Record *record) {
          record->add_string_value(std::string(data, size));
          return true;
        },
        [&](Array *array) {
          array->add_string_value(std::string(data, size));
          return true;
        });
    }

  private:
    template <typename ObjectAddValue, typename ArrayAddValue>
    bool
    add_value(ObjectAddValue object_add_value, ArrayAddValue array_add_value)
    {
      const auto container_type = container_type_stack_.back();
      switch (container_type) {
      case ContainerType::OBJECT:
        {
          auto record = &(record_stack_.back());
          return object_add_value(record);
        }
      case ContainerType::ARRAY:
        {
          auto array = &(array_stack_.back());
          return array_add_value(array);
        }
      default:
        return false;
      }
    }

    enum class ContainerType {
      OBJECT,
      ARRAY,
    };

    grn_ctx *ctx_;
    grn_caster *caster_;
    std::vector<grn_obj *> table_stack_;
    std::vector<Record> record_stack_;
    std::vector<Array> array_stack_;
    std::vector<ContainerType> container_type_stack_;
  };

  struct TextHandler : public RAPIDJSON_NAMESPACE::BaseReaderHandler<> {
    grn_ctx *ctx_;
    grn_caster *caster_;

    TextHandler(grn_ctx *ctx, grn_caster *caster) : ctx_(ctx), caster_(caster)
    {
    }

    bool
    Default()
    {
      return false;
    }

    bool
    String(const char *data, size_t size, bool copy)
    {
      grn_vector_add_element_float(ctx_,
                                   caster_->dest,
                                   data,
                                   size,
                                   0.0,
                                   GRN_DB_SHORT_TEXT);
      return true;
    }
  };

  template <typename Handler>
  grn_rc
  json_to_uvector(grn_ctx *ctx,
                  RAPIDJSON_NAMESPACE::Document *document,
                  grn_caster *caster)
  {
    Handler handler(ctx, caster);
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

  grn_rc
  json_to_text_vector(grn_ctx *ctx,
                      RAPIDJSON_NAMESPACE::Document *document,
                      grn_caster *caster)
  {
    TextHandler handler(ctx, caster);
    if (document->IsArray()) {
      auto n = document->Size();
      for (size_t i = 0; i < n; ++i) {
        const auto &element = (*document)[i];
        if (!element.Accept(handler)) {
          return GRN_INVALID_ARGUMENT;
        }
      }
      return GRN_SUCCESS;
    } else {
      return GRN_INVALID_ARGUMENT;
    }
  }

  bool
  json_maybe_container(const char *json, size_t length)
  {
    const char *start = json;
    const char *end = json + length;
    for (; start < end; start++) {
      bool is_whitespace = false;
      switch (start[0]) {
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        is_whitespace = true;
        break;
      default:
        break;
      }
      if (!is_whitespace) {
        break;
      }
    }
    if (start >= end) {
      return false;
    }
    switch (start[0]) {
    case '[':
    case '{':
      return true;
    default:
      return false;
    }
  }

  grn_rc
  cast_text_to_uvector(grn_ctx *ctx, grn_caster *caster)
  {
    if (!json_maybe_container(GRN_TEXT_VALUE(caster->src),
                              GRN_TEXT_LEN(caster->src))) {
      return GRN_INVALID_ARGUMENT;
    }

    RAPIDJSON_NAMESPACE::Document document;
    RAPIDJSON_NAMESPACE::MemoryStream stream(GRN_TEXT_VALUE(caster->src),
                                             GRN_TEXT_LEN(caster->src));
    document.ParseStream(stream);
    if (document.HasParseError()) {
      return GRN_INVALID_ARGUMENT;
    }

    switch (caster->dest->header.domain) {
    case GRN_DB_INT8:
      return json_to_uvector<Int8Handler>(ctx, &document, caster);
    case GRN_DB_UINT8:
      return json_to_uvector<UInt8Handler>(ctx, &document, caster);
    case GRN_DB_INT16:
      return json_to_uvector<Int16Handler>(ctx, &document, caster);
    case GRN_DB_UINT16:
      return json_to_uvector<UInt16Handler>(ctx, &document, caster);
    case GRN_DB_INT32:
      return json_to_uvector<Int32Handler>(ctx, &document, caster);
    case GRN_DB_UINT32:
      return json_to_uvector<UInt32Handler>(ctx, &document, caster);
    case GRN_DB_INT64:
      return json_to_uvector<Int64Handler>(ctx, &document, caster);
    case GRN_DB_UINT64:
      return json_to_uvector<UInt64Handler>(ctx, &document, caster);
    case GRN_DB_FLOAT:
      return json_to_uvector<FloatHandler>(ctx, &document, caster);
    case GRN_DB_FLOAT32:
      return json_to_uvector<Float32Handler>(ctx, &document, caster);
    default:
      {
        grn_rc rc = GRN_INVALID_ARGUMENT;
        auto domain = grn_ctx_at(ctx, caster->dest->header.domain);
        if (grn_obj_is_weight_uvector(ctx, caster->dest)) {
          if (grn_obj_is_table_with_key(ctx, domain)) {
            rc = json_to_uvector<TableWeightHandler>(ctx, &document, caster);
          }
        } else {
          TableHandler handler(ctx, caster);
          if (document.Accept(handler)) {
            rc = GRN_SUCCESS;
          }
        }
        grn_obj_unref(ctx, domain);
        return rc;
      }
    }
  }

  grn_rc
  cast_text_to_text_vector(grn_ctx *ctx, grn_caster *caster)
  {
    RAPIDJSON_NAMESPACE::Document document;
    RAPIDJSON_NAMESPACE::MemoryStream stream(GRN_TEXT_VALUE(caster->src),
                                             GRN_TEXT_LEN(caster->src));
    document.ParseStream(stream);
    if (document.HasParseError()) {
      return GRN_INVALID_ARGUMENT;
    }
    return json_to_text_vector(ctx, &document, caster);
  }
} // namespace
#endif

// NOTE: We may need to change the following current design:
//
// This is called for a whole value of a scalar column and an element
// value of a vector column.
//
// If this is called for a whole value of a scalar column, an error is
// reported as both of an error log and an return value with
// GRN_OBJ_INVALID_ERROR.
//
// If this is called for an element value for a vector column, an
// error is reported as only an error log with
// GRN_OBJ_INVALID_ERROR. Return value is GRN_SUCCESS.
//
// Calling a cast function only for a whole value may be better for
// consistent implementation but there are custom cast processes in
// load.c and arrow.cpp for a vector value. We can reconsider the
// current design later.
grn_inline static grn_rc
grn_caster_cast_to_record(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_SUCCESS;

  grn_obj *table = grn_ctx_at(ctx, caster->dest->header.domain);
  if (!GRN_OBJ_TABLEP(table)) {
    grn_obj_unref(ctx, table);
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  uint32_t missing_mode = (caster->flags & GRN_OBJ_MISSING_MASK);
  uint32_t invalid_mode = (caster->flags & GRN_OBJ_INVALID_MASK);
  grn_id id = GRN_ID_NIL;
  if (table->header.type != GRN_TABLE_NO_KEY) {
    grn_obj *p_key = caster->src;
    grn_obj key;
    GRN_OBJ_INIT(&key, GRN_BULK, 0, table->header.domain);
    if (!grn_type_id_is_compatible(ctx,
                                   caster->src->header.domain,
                                   table->header.domain)) {
      grn_caster key_caster = {
        caster->src,
        &key,
        caster->flags,
        caster->target,
      };
      rc = grn_caster_cast(ctx, &key_caster);
      p_key = &key;
    }
    if (rc == GRN_SUCCESS) {
      if (GRN_BULK_VSIZE(p_key) > 0) {
        if (missing_mode == GRN_OBJ_MISSING_ADD) {
          id = grn_table_add_by_key(ctx, table, p_key, NULL);
        } else {
          id = grn_table_get_by_key(ctx, table, p_key);
        }
        if (id == GRN_ID_NIL) {
          if (invalid_mode == GRN_OBJ_INVALID_ERROR) {
            rc = GRN_INVALID_ARGUMENT;
            bool called_from_grn_obj_cast = (caster->target == NULL);
            if (missing_mode != GRN_OBJ_MISSING_ADD &&
                !called_from_grn_obj_cast /* For backward compatibility */) {
              CAST_FAILED(caster);
              ERRCLR(ctx);
            }
          } else {
            ERRCLR(ctx);
            if (missing_mode == GRN_OBJ_MISSING_NIL) {
              GRN_RECORD_SET(ctx, caster->dest, GRN_ID_NIL);
            }
            CAST_FAILED(caster);
          }
        } else {
          GRN_RECORD_SET(ctx, caster->dest, id);
        }
      } else {
        if (missing_mode != GRN_OBJ_MISSING_IGNORE) {
          GRN_RECORD_SET(ctx, caster->dest, GRN_ID_NIL);
        }
      }
    }
    GRN_OBJ_FIN(ctx, &key);
  } else {
    grn_obj record_id;
    GRN_UINT32_INIT(&record_id, 0);
    grn_caster record_caster = {
      caster->src,
      &record_id,
      caster->flags,
      caster->target,
    };
    rc = grn_caster_cast(ctx, &record_caster);
    if (rc == GRN_SUCCESS) {
      bool valid = true;
      if (GRN_BULK_VSIZE(&record_id) == 0) {
        if (missing_mode == GRN_OBJ_MISSING_NIL) {
          id = GRN_ID_NIL;
        } else {
          valid = false;
        }
      } else {
        id = GRN_UINT32_VALUE(&record_id);
        if (!(GRN_ID_NIL < id && id <= GRN_ID_MAX &&
              grn_table_at(ctx, table, id) == id)) {
          valid = false;
        }
      }
      if (valid) {
        GRN_RECORD_SET(ctx, caster->dest, id);
      } else {
        CAST_FAILED(caster);
        rc = ctx->rc;
      }
    }
  }

  grn_obj_unref(ctx, table);

  return rc;
}

grn_inline static grn_rc
grn_caster_cast_bool(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_SUCCESS;

  switch (caster->dest->header.domain) {
  case GRN_DB_BOOL:
    GRN_BOOL_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_INT8:
    GRN_INT8_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_UINT8:
    GRN_UINT8_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_INT16:
    GRN_INT16_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_UINT16:
    GRN_UINT16_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_INT32:
    GRN_INT32_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_UINT32:
    GRN_UINT32_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_INT64:
    GRN_INT64_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_UINT64:
    GRN_UINT64_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_FLOAT32:
    GRN_FLOAT32_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_FLOAT:
    GRN_FLOAT_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_TIME:
    GRN_TIME_SET(ctx, caster->dest, GRN_BOOL_VALUE(caster->src));
    break;
  case GRN_DB_SHORT_TEXT:
  case GRN_DB_TEXT:
  case GRN_DB_LONG_TEXT:
    {
      const char *bool_text;
      bool_text = GRN_BOOL_VALUE(caster->src) ? "true" : "false";
      GRN_TEXT_PUTS(ctx, caster->dest, bool_text);
    }
    break;
  case GRN_DB_TOKYO_GEO_POINT:
  case GRN_DB_WGS84_GEO_POINT:
    rc = GRN_INVALID_ARGUMENT;
    break;
  default:
    rc = grn_caster_cast_to_record(ctx, caster);
    break;
  }
  return rc;
}

namespace {
  template <typename NUMERIC>
  grn_rc
  num2time(grn_ctx *ctx, grn_caster *caster, NUMERIC value)
  {
    switch (caster->src->header.domain) {
    case GRN_DB_TIME:
    case GRN_DB_FLOAT32:
    case GRN_DB_FLOAT:
      return grn::bulk::set<NUMERIC>(ctx, caster->dest, value);
    default:
      return grn::bulk::set<int64_t>(ctx,
                                     caster->dest,
                                     static_cast<int64_t>(value) *
                                       GRN_TIME_USEC_PER_SEC);
    }
  }

  template <typename FLOATING_POINT, typename NUMERIC>
  grn_rc
  num2float(grn_ctx *ctx, grn_caster *caster, NUMERIC value)
  {
    if (caster->src->header.domain == GRN_DB_TIME) {
      return grn::bulk::set<FLOATING_POINT>(ctx,
                                            caster->dest,
                                            static_cast<FLOATING_POINT>(value) /
                                              GRN_TIME_USEC_PER_SEC);
    } else {
      return grn::bulk::set<FLOATING_POINT>(ctx, caster->dest, value);
    }
  }

  template <typename NUMERIC>
  std::enable_if_t<std::is_integral_v<NUMERIC> && std::is_signed_v<NUMERIC>,
                   grn_rc>
  num2text(grn_ctx *ctx, grn_caster *caster, NUMERIC value)
  {
    return grn_text_lltoa(ctx, caster->dest, value);
  }

  template <typename NUMERIC>
  std::enable_if_t<std::is_integral_v<NUMERIC> && !std::is_signed_v<NUMERIC>,
                   grn_rc>
  num2text(grn_ctx *ctx, grn_caster *caster, NUMERIC value)
  {
    return grn_text_ulltoa(ctx, caster->dest, value);
  }

  template <typename NUMERIC>
  std::enable_if_t<std::is_same_v<NUMERIC, float>, grn_rc>
  num2text(grn_ctx *ctx, grn_caster *caster, NUMERIC value)
  {
    return grn_text_f32toa(ctx, caster->dest, value);
  }

  template <typename NUMERIC>
  std::enable_if_t<std::is_same_v<NUMERIC, double>, grn_rc>
  num2text(grn_ctx *ctx, grn_caster *caster, NUMERIC value)
  {
    return grn_text_ftoa(ctx, caster->dest, value);
  }

  template <typename SOURCE>
  grn_rc
  num2dest(grn_ctx *ctx, grn_caster *caster)
  {
    switch (caster->dest->header.domain) {
    case GRN_DB_BOOL:
      return grn::bulk::set<SOURCE>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_INT8:
      return grn::bulk::set<int8_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_UINT8:
      return grn::bulk::set<uint8_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_INT16:
      return grn::bulk::set<int16_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_UINT16:
      return grn::bulk::set<uint16_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_INT32:
      return grn::bulk::set<int32_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_UINT32:
      return grn::bulk::set<uint32_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_INT64:
      return grn::bulk::set<int64_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_UINT64:
      return grn::bulk::set<uint64_t>(
        ctx,
        caster->dest,
        grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_TIME:
      return num2time(ctx, caster, grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_FLOAT32:
      return num2float<float>(ctx,
                              caster,
                              grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_FLOAT:
      return num2float<double>(ctx,
                               caster,
                               grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT:
      return num2text(ctx, caster, grn::bulk::get<SOURCE>(ctx, caster->src, 0));
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT:
      return GRN_INVALID_ARGUMENT;
    default:
      return grn_caster_cast_to_record(ctx, caster);
    }
  }
} // namespace

#define TEXT2DEST(type, tonum, setvalue)                                       \
  do {                                                                         \
    const char *cur, *str = GRN_TEXT_VALUE(caster->src);                       \
    const char *str_end = GRN_BULK_CURR(caster->src);                          \
    type i = tonum(str, str_end, &cur);                                        \
    if (cur == str_end) {                                                      \
      setvalue(ctx, caster->dest, i);                                          \
    } else if (cur != str) {                                                   \
      const char *rest;                                                        \
      grn_obj buf;                                                             \
      GRN_VOID_INIT(&buf);                                                     \
      rc = grn_aton(ctx, str, str_end, &rest, &buf);                           \
      if (rc == GRN_SUCCESS) {                                                 \
        if (rest == str_end) {                                                 \
          grn_caster number_caster = {                                         \
            &buf,                                                              \
            caster->dest,                                                      \
            caster->flags,                                                     \
            caster->target,                                                    \
          };                                                                   \
          rc = grn_caster_cast(ctx, &number_caster);                           \
        } else {                                                               \
          rc = GRN_INVALID_ARGUMENT;                                           \
        }                                                                      \
      }                                                                        \
      GRN_OBJ_FIN(ctx, &buf);                                                  \
    } else {                                                                   \
      rc = GRN_INVALID_ARGUMENT;                                               \
    }                                                                          \
  } while (0)

static grn_rc
grn_caster_cast_text_to_bulk(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_SUCCESS;
  switch (caster->dest->header.domain) {
  case GRN_DB_BOOL:
    GRN_BOOL_SET(ctx, caster->dest, GRN_TEXT_LEN(caster->src) > 0);
    break;
  case GRN_DB_INT8:
    TEXT2DEST(int8_t, grn_atoi8, GRN_INT8_SET);
    break;
  case GRN_DB_UINT8:
    TEXT2DEST(uint8_t, grn_atoui8, GRN_UINT8_SET);
    break;
  case GRN_DB_INT16:
    TEXT2DEST(int16_t, grn_atoi16, GRN_INT16_SET);
    break;
  case GRN_DB_UINT16:
    TEXT2DEST(uint16_t, grn_atoui16, GRN_UINT16_SET);
    break;
  case GRN_DB_INT32:
    TEXT2DEST(int32_t, grn_atoi, GRN_INT32_SET);
    break;
  case GRN_DB_UINT32:
    TEXT2DEST(uint32_t, grn_atoui, GRN_UINT32_SET);
    break;
  case GRN_DB_TIME:
    {
      grn_timeval v;
      int len = GRN_TEXT_LEN(caster->src);
      char *str = GRN_TEXT_VALUE(caster->src);
      if (grn_str2timeval(str, len, &v)) {
        double d;
        char *end;
        grn_obj buf;
        GRN_TEXT_INIT(&buf, 0);
        GRN_TEXT_PUT(ctx, &buf, str, len);
        GRN_TEXT_PUTC(ctx, &buf, '\0');
        errno = 0;
        d = strtod(GRN_TEXT_VALUE(&buf), &end);
        if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
          v.tv_sec = d;
          v.tv_nsec = ((d - v.tv_sec) * GRN_TIME_NSEC_PER_SEC);
        } else {
          rc = GRN_INVALID_ARGUMENT;
        }
        GRN_OBJ_FIN(ctx, &buf);
      }
      GRN_TIME_SET(
        ctx,
        caster->dest,
        GRN_TIME_PACK((int64_t)v.tv_sec, GRN_TIME_NSEC_TO_USEC(v.tv_nsec)));
    }
    break;
  case GRN_DB_INT64:
    TEXT2DEST(int64_t, grn_atoll, GRN_INT64_SET);
    break;
  case GRN_DB_UINT64:
    TEXT2DEST(int64_t, grn_atoll, GRN_UINT64_SET);
    break;
  case GRN_DB_FLOAT32:
  case GRN_DB_FLOAT:
    {
      char *end;
      grn_obj buf;
      GRN_TEXT_INIT(&buf, 0);
      GRN_TEXT_PUT(ctx,
                   &buf,
                   GRN_TEXT_VALUE(caster->src),
                   GRN_TEXT_LEN(caster->src));
      GRN_TEXT_PUTC(ctx, &buf, '\0');
      errno = 0;
      if (caster->dest->header.domain == GRN_DB_FLOAT32) {
        float value;
        value = strtof(GRN_TEXT_VALUE(&buf), &end);
        if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
          GRN_FLOAT32_SET(ctx, caster->dest, value);
        } else {
          rc = GRN_INVALID_ARGUMENT;
        }
      } else {
        double value;
        value = strtod(GRN_TEXT_VALUE(&buf), &end);
        if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
          GRN_FLOAT_SET(ctx, caster->dest, value);
        } else {
          rc = GRN_INVALID_ARGUMENT;
        }
      }
      GRN_OBJ_FIN(ctx, &buf);
    }
    break;
  case GRN_DB_SHORT_TEXT:
  case GRN_DB_TEXT:
  case GRN_DB_LONG_TEXT:
    GRN_TEXT_PUT(ctx,
                 caster->dest,
                 GRN_TEXT_VALUE(caster->src),
                 GRN_TEXT_LEN(caster->src));
    break;
  case GRN_DB_TOKYO_GEO_POINT:
  case GRN_DB_WGS84_GEO_POINT:
    {
      int latitude, longitude;
      double degree;
      const char *cur, *str = GRN_TEXT_VALUE(caster->src);
      const char *str_end = GRN_BULK_CURR(caster->src);
      if (str == str_end) {
        GRN_GEO_POINT_SET(ctx, caster->dest, 0, 0);
      } else {
        char *end;
        grn_obj buf, *buf_p = NULL;
        latitude = grn_atoi(str, str_end, &cur);
        if (cur < str_end && cur[0] == '.') {
          GRN_TEXT_INIT(&buf, 0);
          GRN_TEXT_PUT(ctx, &buf, str, GRN_TEXT_LEN(caster->src));
          GRN_TEXT_PUTC(ctx, &buf, '\0');
          buf_p = &buf;
          errno = 0;
          degree = strtod(GRN_TEXT_VALUE(buf_p), &end);
          if (errno) {
            rc = GRN_INVALID_ARGUMENT;
          } else {
            latitude = GRN_GEO_DEGREE2MSEC(degree);
            cur = str + (end - GRN_TEXT_VALUE(buf_p));
          }
        }
        if (!rc && (cur[0] == 'x' || cur[0] == ',') && cur + 1 < str_end) {
          const char *c = cur + 1;
          longitude = grn_atoi(c, str_end, &cur);
          if (cur < str_end && cur[0] == '.') {
            if (!buf_p) {
              GRN_TEXT_INIT(&buf, 0);
              GRN_TEXT_PUT(ctx, &buf, str, GRN_TEXT_LEN(caster->src));
              GRN_TEXT_PUTC(ctx, &buf, '\0');
              buf_p = &buf;
            }
            errno = 0;
            degree = strtod(GRN_TEXT_VALUE(buf_p) + (c - str), &end);
            if (errno) {
              rc = GRN_INVALID_ARGUMENT;
            } else {
              longitude = GRN_GEO_DEGREE2MSEC(degree);
              cur = str + (end - GRN_TEXT_VALUE(buf_p));
            }
          }
          if (!rc && cur == str_end) {
            if ((GRN_GEO_MIN_LATITUDE <= latitude &&
                 latitude <= GRN_GEO_MAX_LATITUDE) &&
                (GRN_GEO_MIN_LONGITUDE <= longitude &&
                 longitude <= GRN_GEO_MAX_LONGITUDE)) {
              GRN_GEO_POINT_SET(ctx, caster->dest, latitude, longitude);
            } else {
              rc = GRN_INVALID_ARGUMENT;
            }
          } else {
            rc = GRN_INVALID_ARGUMENT;
          }
        } else {
          rc = GRN_INVALID_ARGUMENT;
        }
        if (buf_p) {
          GRN_OBJ_FIN(ctx, buf_p);
        }
      }
    }
    break;
  default:
    rc = grn_caster_cast_to_record(ctx, caster);
    break;
  }
  return rc;
}

static grn_rc
grn_caster_cast_text(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_SUCCESS;
  switch (caster->dest->header.type) {
  case GRN_BULK:
    rc = grn_caster_cast_text_to_bulk(ctx, caster);
    break;
  case GRN_UVECTOR:
    rc = grn_caster_cast_text_to_uvector(ctx, caster);
    break;
  default:
    rc = GRN_INVALID_ARGUMENT;
    break;
  }
  return rc;
}

static grn_rc
grn_caster_cast_record_data(grn_ctx *ctx,
                            grn_caster *caster,
                            grn_obj *src_table)
{
  grn_obj *key_accessor = grn_obj_column(ctx,
                                         src_table,
                                         GRN_COLUMN_NAME_KEY,
                                         GRN_COLUMN_NAME_KEY_LEN);
  if (!key_accessor) {
    return ctx->rc;
  }

  grn_rc rc = GRN_SUCCESS;
  grn_obj key;
  GRN_VOID_INIT(&key);
  if (grn_obj_is_vector(ctx, caster->dest)) {
    if (grn_obj_is_uvector(ctx, caster->src)) {
      uint32_t i;
      uint32_t n = grn_uvector_size(ctx, caster->src);
      for (i = 0; i < n; i++) {
        GRN_BULK_REWIND(&key);
        grn_id src_id =
          grn_uvector_get_element_record(ctx, caster->src, i, NULL);
        grn_obj_get_value(ctx, key_accessor, src_id, &key);
        if (ctx->rc != GRN_SUCCESS) {
          rc = ctx->rc;
          break;
        }
        grn_caster key_caster = {
          &key,
          caster->dest,
          caster->flags,
          caster->target,
        };
        rc = grn_caster_cast(ctx, &key_caster);
        if (rc != GRN_SUCCESS) {
          break;
        }
      }
    } else {
      if (GRN_BULK_VSIZE(caster->src) > 0) {
        grn_obj_get_value(ctx,
                          key_accessor,
                          GRN_RECORD_VALUE(caster->src),
                          &key);
        if (ctx->rc == GRN_SUCCESS) {
          grn_caster key_caster = {
            &key,
            caster->dest,
            caster->flags,
            caster->target,
          };
          rc = grn_caster_cast(ctx, &key_caster);
        } else {
          rc = ctx->rc;
        }
      }
    }
  } else {
    if (GRN_BULK_VSIZE(caster->src) > 0) {
      grn_obj_get_value(ctx, key_accessor, GRN_RECORD_VALUE(caster->src), &key);
      if (ctx->rc == GRN_SUCCESS) {
        grn_caster key_caster = {
          &key,
          caster->dest,
          caster->flags,
          caster->target,
        };
        rc = grn_caster_cast(ctx, &key_caster);
      } else {
        rc = ctx->rc;
      }
    }
  }
  GRN_OBJ_FIN(ctx, &key);

  grn_obj_unlink(ctx, key_accessor);
  return rc;
}

static grn_rc
grn_caster_cast_record_record(grn_ctx *ctx,
                              grn_caster *caster,
                              grn_obj *src_table,
                              grn_obj *dest_table)
{
  if (GRN_RECORD_VALUE(caster->src) == GRN_ID_NIL) {
    if (grn_obj_is_uvector(ctx, caster->dest)) {
      grn_uvector_add_element_record(ctx,
                                     caster->dest,
                                     GRN_RECORD_VALUE(caster->src),
                                     0.0);
    } else {
      GRN_RECORD_PUT(ctx, caster->dest, GRN_RECORD_VALUE(caster->src));
    }
    return GRN_SUCCESS;
  }

  bool get_key_optimizable =
    (!grn_obj_is_patricia_trie(ctx, src_table) ||
     !grn_pat_is_key_encoded(ctx, (grn_pat *)src_table));
  grn_obj key_buffer;
  GRN_VOID_INIT(&key_buffer);
  uint32_t key_size;
  const char *key;
  if (get_key_optimizable) {
    key =
      _grn_table_key(ctx, src_table, GRN_RECORD_VALUE(caster->src), &key_size);
  } else {
    grn_table_get_key2(ctx,
                       src_table,
                       GRN_RECORD_VALUE(caster->src),
                       &key_buffer);
    key = GRN_BULK_HEAD(&key_buffer);
    key_size = GRN_BULK_VSIZE(&key_buffer);
  }

  grn_id dest_id;
  uint32_t missing_mode = (caster->flags & GRN_OBJ_MISSING_MASK);
  if (missing_mode == GRN_OBJ_MISSING_ADD) {
    dest_id = grn_table_add(ctx, dest_table, key, key_size, NULL);
  } else {
    dest_id = grn_table_get(ctx, dest_table, key, key_size);
  }
  GRN_OBJ_FIN(ctx, &key_buffer);

  if (dest_id == GRN_ID_NIL) {
    if ((caster->flags & GRN_OBJ_INVALID_MASK) == GRN_OBJ_INVALID_ERROR) {
      return GRN_INVALID_ARGUMENT;
    }
    if (missing_mode == GRN_OBJ_MISSING_IGNORE) {
      return GRN_SUCCESS;
    }
  }
  if (grn_obj_is_uvector(ctx, caster->dest)) {
    grn_uvector_add_element_record(ctx, caster->dest, dest_id, 0.0);
  } else {
    GRN_RECORD_PUT(ctx, caster->dest, dest_id);
  }
  return GRN_SUCCESS;
}

static grn_rc
grn_caster_cast_record(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  grn_obj *src_table = NULL;
  grn_obj *dest_table = NULL;

  if (caster->src->header.domain == caster->dest->header.domain) {
    if (grn_obj_is_uvector(ctx, caster->dest)) {
      if (grn_obj_is_uvector(ctx, caster->src)) {
        uint32_t i;
        uint32_t n_elements = grn_uvector_size(ctx, caster->src);
        for (i = 0; i < n_elements; i++) {
          float weight;
          grn_id id =
            grn_uvector_get_element_record(ctx, caster->src, i, &weight);
          grn_uvector_add_element_record(ctx, caster->dest, id, weight);
        }
      } else {
        grn_uvector_add_element_record(ctx,
                                       caster->dest,
                                       GRN_RECORD_VALUE(caster->src),
                                       0.0);
      }
    } else {
      GRN_RECORD_PUT(ctx, caster->dest, GRN_RECORD_VALUE(caster->src));
    }
    rc = GRN_SUCCESS;
    goto exit;
  }

  src_table = grn_ctx_at(ctx, caster->src->header.domain);
  if (!src_table) {
    rc = GRN_INVALID_ARGUMENT;
    goto exit;
  }
  if (src_table->header.type == GRN_TABLE_NO_KEY) {
    rc = GRN_INVALID_ARGUMENT;
    goto exit;
  }

  if (grn_type_id_is_builtin(ctx, caster->dest->header.domain)) {
    rc = grn_caster_cast_record_data(ctx, caster, src_table);
  } else {
    dest_table = grn_ctx_at(ctx, caster->dest->header.domain);
    if (!dest_table) {
      rc = GRN_INVALID_ARGUMENT;
      goto exit;
    }
    if (!grn_obj_is_table_with_key(ctx, dest_table)) {
      rc = GRN_INVALID_ARGUMENT;
      goto exit;
    }

    rc = grn_caster_cast_record_record(ctx, caster, src_table, dest_table);
  }

exit:
  grn_obj_unref(ctx, src_table);
  grn_obj_unref(ctx, dest_table);

  return rc;
}

extern "C" grn_rc
grn_caster_cast_text_to_uvector(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
#ifdef GRN_WITH_RAPIDJSON
  rc = cast_text_to_uvector(ctx, caster);
  if (rc == GRN_SUCCESS) {
    return rc;
  }
#endif

  auto domain_id = caster->dest->header.domain;
  grn_obj dest;
  GRN_VALUE_FIX_SIZE_INIT(&dest, 0, domain_id);
  grn_caster sub_caster = {
    caster->src,
    &dest,
    caster->flags,
    caster->target,
  };
  rc = grn_caster_cast(ctx, &sub_caster);
  if (rc == GRN_SUCCESS) {
    if (GRN_BULK_VSIZE(&dest) > 0) {
      grn_bulk_write(ctx,
                     caster->dest,
                     GRN_BULK_HEAD(&dest),
                     GRN_BULK_VSIZE(&dest));
      if (grn_obj_is_weight_uvector(ctx, caster->dest)) {
        GRN_FLOAT32_PUT(ctx, caster->dest, 0.0);
      }
    }
  }
  GRN_OBJ_FIN(ctx, &dest);
  return rc;
}

extern "C" grn_rc
grn_caster_cast_text_to_text_vector(grn_ctx *ctx, grn_caster *caster)
{
#ifdef GRN_WITH_RAPIDJSON
  return cast_text_to_text_vector(ctx, caster);
#else
  return GRN_INVALID_ARGUMENT;
#endif
}

extern "C" grn_rc
grn_caster_cast(grn_ctx *ctx, grn_caster *caster)
{
  grn_rc rc = GRN_SUCCESS;
  switch (caster->src->header.domain) {
  case GRN_DB_BOOL:
    rc = grn_caster_cast_bool(ctx, caster);
    break;
  case GRN_DB_INT8:
    rc = num2dest<int8_t>(ctx, caster);
    break;
  case GRN_DB_UINT8:
    rc = num2dest<uint8_t>(ctx, caster);
    break;
  case GRN_DB_INT16:
    rc = num2dest<int16_t>(ctx, caster);
    break;
  case GRN_DB_UINT16:
    rc = num2dest<uint16_t>(ctx, caster);
    break;
  case GRN_DB_INT32:
    rc = num2dest<int32_t>(ctx, caster);
    break;
  case GRN_DB_UINT32:
    rc = num2dest<uint32_t>(ctx, caster);
    break;
  case GRN_DB_INT64:
    rc = num2dest<int64_t>(ctx, caster);
    break;
  case GRN_DB_UINT64:
    rc = num2dest<uint64_t>(ctx, caster);
    break;
  case GRN_DB_FLOAT32:
    rc = num2dest<float>(ctx, caster);
    break;
  case GRN_DB_FLOAT:
    rc = num2dest<double>(ctx, caster);
    break;
  case GRN_DB_TIME:
    rc = num2dest<int64_t>(ctx, caster);
    break;
  case GRN_DB_SHORT_TEXT:
  case GRN_DB_TEXT:
  case GRN_DB_LONG_TEXT:
    rc = grn_caster_cast_text(ctx, caster);
    break;
  case GRN_DB_TOKYO_GEO_POINT:
  case GRN_DB_WGS84_GEO_POINT:
    if (caster->src->header.domain == caster->dest->header.domain) {
      GRN_TEXT_PUT(ctx,
                   caster->dest,
                   GRN_TEXT_VALUE(caster->src),
                   GRN_TEXT_LEN(caster->src));
    } else {
      int latitude, longitude;
      double latitude_in_degree, longitude_in_degree;
      GRN_GEO_POINT_VALUE(caster->src, latitude, longitude);
      latitude_in_degree = GRN_GEO_MSEC2DEGREE(latitude);
      longitude_in_degree = GRN_GEO_MSEC2DEGREE(longitude);
      /* TokyoGeoPoint <-> WGS84GeoPoint is based on
         http://www.jalan.net/jw/jwp0200/jww0203.do

         jx: longitude in degree in Tokyo Geodetic System.
         jy: latitude in degree in Tokyo Geodetic System.
         wx: longitude in degree in WGS 84.
         wy: latitude in degree in WGS 84.

         jy = wy * 1.000106961 - wx * 0.000017467 - 0.004602017
         jx = wx * 1.000083049 + wy * 0.000046047 - 0.010041046

         wy = jy - jy * 0.00010695 + jx * 0.000017464 + 0.0046017
         wx = jx - jy * 0.000046038 - jx * 0.000083043 + 0.010040
      */
      if (caster->dest->header.domain == GRN_DB_TOKYO_GEO_POINT) {
        double wgs84_latitude_in_degree = latitude_in_degree;
        double wgs84_longitude_in_degree = longitude_in_degree;
        int tokyo_latitude, tokyo_longitude;
        double tokyo_latitude_in_degree, tokyo_longitude_in_degree;
        tokyo_latitude_in_degree = wgs84_latitude_in_degree * 1.000106961 -
                                   wgs84_longitude_in_degree * 0.000017467 -
                                   0.004602017;
        tokyo_longitude_in_degree = wgs84_longitude_in_degree * 1.000083049 +
                                    wgs84_latitude_in_degree * 0.000046047 -
                                    0.010041046;
        tokyo_latitude = GRN_GEO_DEGREE2MSEC(tokyo_latitude_in_degree);
        tokyo_longitude = GRN_GEO_DEGREE2MSEC(tokyo_longitude_in_degree);
        GRN_GEO_POINT_SET(ctx, caster->dest, tokyo_latitude, tokyo_longitude);
      } else {
        double tokyo_latitude_in_degree = latitude_in_degree;
        double tokyo_longitude_in_degree = longitude_in_degree;
        int wgs84_latitude, wgs84_longitude;
        double wgs84_latitude_in_degree, wgs84_longitude_in_degree;
        wgs84_latitude_in_degree =
          tokyo_latitude_in_degree - tokyo_latitude_in_degree * 0.00010695 +
          tokyo_longitude_in_degree * 0.000017464 + 0.0046017;
        wgs84_longitude_in_degree =
          tokyo_longitude_in_degree - tokyo_latitude_in_degree * 0.000046038 -
          tokyo_longitude_in_degree * 0.000083043 + 0.010040;
        wgs84_latitude = GRN_GEO_DEGREE2MSEC(wgs84_latitude_in_degree);
        wgs84_longitude = GRN_GEO_DEGREE2MSEC(wgs84_longitude_in_degree);
        GRN_GEO_POINT_SET(ctx, caster->dest, wgs84_latitude, wgs84_longitude);
      }
    }
    break;
  case GRN_VOID:
    rc = grn_obj_reinit(ctx,
                        caster->dest,
                        caster->dest->header.domain,
                        caster->dest->header.flags);
    break;
  default:
    if (caster->src->header.domain >= GRN_N_RESERVED_TYPES) {
      grn_obj *table;
      table = grn_ctx_at(ctx, caster->src->header.domain);
      switch (table->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
      case GRN_TABLE_NO_KEY:
        rc = grn_caster_cast_record(ctx, caster);
        break;
      default:
        rc = GRN_FUNCTION_NOT_IMPLEMENTED;
        break;
      }
    } else {
      rc = GRN_FUNCTION_NOT_IMPLEMENTED;
    }
    break;
  }
  return rc;
}

extern "C" grn_rc
grn_obj_cast(grn_ctx *ctx,
             grn_obj *src,
             grn_obj *dest,
             bool add_record_if_not_exist)
{
  uint32_t flags = 0;
  if (add_record_if_not_exist) {
    flags |= GRN_OBJ_MISSING_ADD | GRN_OBJ_INVALID_ERROR;
  } else {
    flags |= GRN_OBJ_MISSING_NIL | GRN_OBJ_INVALID_ERROR;
  }
  grn_caster caster = {
    src,
    dest,
    flags,
    NULL,
  };
  return grn_caster_cast(ctx, &caster);
}
