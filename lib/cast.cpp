/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019-2021  Sutou Kouhei <kou@clear-code.com>

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
# include "grn_db.h"
# include <groonga/smart_obj.hpp>

# include <limits>
# include <string>
# include <vector>

# include <rapidjson/document.h>
# include <rapidjson/memorystream.h>

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

    struct TableWeightHandler : public rapidjson::BaseReaderHandler<>
    {
      TableWeightHandler(grn_ctx *ctx,
                         grn_obj *uvector,
                         bool add_record_if_not_exist)
        : ctx_(ctx),
          uvector_(uvector),
          add_record_if_not_exist_(add_record_if_not_exist),
          table_(grn_ctx_at(ctx, uvector->header.domain)),
          weight_(0.0) {
      }

      ~TableWeightHandler() {
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

    class Array
    {
    public:
      Array(grn_ctx *ctx)
        : ctx_(ctx),
          values_() {
      }

      ~Array() {
        for (auto value : values_) {
          grn_obj_close(ctx_, value);
        }
      }

      bool add_bool_value(const bool value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_BOOL);
        GRN_BOOL_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_int32_value(const int32_t value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT32);
        GRN_INT32_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_uint32_value(const uint32_t value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_UINT32);
        GRN_UINT32_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_int64_value(const int64_t value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT64);
        GRN_INT64_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_uint64_value(const uint64_t value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_UINT64);
        GRN_UINT64_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_double_value(const double value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_FLOAT);
        GRN_FLOAT_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_string_value(const std::string &value) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
        GRN_TEXT_SET(ctx_, bulk, value.data(), value.size());
        values_.push_back(bulk);
        return true;
      }

      bool add_record_value(grn_id domain, grn_id id) {
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, domain);
        GRN_RECORD_SET(ctx_, bulk, id);
        values_.push_back(bulk);
        return true;
      }

      const std::vector<grn_obj *> &values() const {
        return values_;
      }

    private:
      grn_ctx *ctx_;
      std::vector<grn_obj *> values_;
    };

    class Record
    {
    public:
      Record(grn_ctx *ctx, grn_obj *table)
        : ctx_(ctx),
          table_(table),
          _id_index_(std::numeric_limits<size_t>::max()),
          _key_index_(std::numeric_limits<size_t>::max()),
          _id_(GRN_ID_NIL),
          keys_(),
          values_() {
      }

      ~Record() {
        for (auto value : values_) {
          grn_obj_close(ctx_, value);
        }
      }

      bool add_key(const std::string &key) {
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

      bool add_bool_value(const bool value) {
        if (_id_index_ == keys_.size() - 1) {
          return false;
        }
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_BOOL);
        GRN_BOOL_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_int32_value(const int32_t value) {
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

      bool add_uint32_value(const uint32_t value) {
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

      bool add_int64_value(const int64_t value) {
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

      bool add_uint64_value(const uint64_t value) {
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

      bool add_double_value(const double value) {
        if (_id_index_ == keys_.size() - 1) {
          return false;
        }
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_FLOAT);
        GRN_FLOAT_SET(ctx_, bulk, value);
        values_.push_back(bulk);
        return true;
      }

      bool add_string_value(const std::string &value) {
        if (_id_index_ == keys_.size() - 1) {
          return false;
        }
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
        GRN_TEXT_SET(ctx_, bulk, value.data(), value.size());
        values_.push_back(bulk);
        return true;
      }

      bool add_record_value(grn_id domain, grn_id id) {
        if (_id_index_ == keys_.size() - 1) {
          return false;
        }
        auto bulk = grn_obj_open(ctx_, GRN_BULK, 0, domain);
        GRN_RECORD_SET(ctx_, bulk, id);
        values_.push_back(bulk);
        return true;
      }

      bool add_array_value(const Array *array) {
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

      grn_obj *table() const {
        return table_;
      }

      grn_id id() const {
        return _id_;
      }

      bool have__id() const {
        return _id_index_ != std::numeric_limits<size_t>::max();
      }

      size_t _id_index() const {
        return _id_index_;
      }

      bool have__key() const {
        return _key_index_ != std::numeric_limits<size_t>::max();
      }

      size_t _key_index() const {
        return _key_index_;
      }

      const std::string &get_key(size_t i) const {
        return keys_[i];
      }

      grn_obj *get_value(size_t i) {
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

    class TableHandler : public rapidjson::BaseReaderHandler<>
    {
    public:
      TableHandler(grn_ctx *ctx,
                   grn_obj *uvector,
                   bool add_record_if_not_exist)
        : ctx_(ctx),
          uvector_(uvector),
          add_record_if_not_exist_(add_record_if_not_exist),
          table_stack_(),
          record_stack_(),
          array_stack_(),
          container_type_stack_() {
        table_stack_.push_back(grn_ctx_at(ctx, uvector->header.domain));
      }

      ~TableHandler() {
        for (auto table : table_stack_) {
          grn_obj_unref(ctx_, table);
        }
      }

      bool Default() {return false;}

      bool StartObject() {
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

      bool Key(const char *data, size_t size, bool copy) {
        auto record = &(record_stack_.back());
        return record->add_key(std::string(data, size));
      }

      bool EndObject(size_t n) {
        auto record = &(record_stack_.back());
        auto table = record->table();
        grn_id domain = DB_OBJ(table)->id;
        grn_id id = GRN_ID_NIL;
        if (record->have__id()) {
          if (id == GRN_ID_NIL) {
            return false;
          }
          id = record->id();
          if (grn_table_at(ctx_, table, id) != id) {
            return false;
          }
        } else if (record->have__key()) {
          auto _key_index = record->_key_index();
          auto _key = record->get_value(_key_index);
          if (add_record_if_not_exist_) {
            id = grn_table_add_by_key(ctx_, table, _key, NULL);
          } else {
            id = grn_table_get_by_key(ctx_, table, _key);
          }
        } else {
          if (grn_obj_is_table_with_key(ctx_, table)) {
            return false;
          }
          if (!add_record_if_not_exist_) {
            return false;
          }
          id = grn_table_add(ctx_, table, NULL, 0, NULL);
        }
        if (id == GRN_ID_NIL) {
          return false;
        }
        for (size_t i = 0; i < n; ++i) {
          if (record->have__id() && i == record->_id_index()) {
            continue;
          }
          if (record->have__key() && i == record->_key_index()) {
            continue;
          }
          auto column_name = record->get_key(i);
          auto column = grn_obj_column(ctx_,
                                       table,
                                       column_name.data(),
                                       column_name.size());
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

      bool StartArray() {
        container_type_stack_.push_back(ContainerType::ARRAY);
        array_stack_.emplace_back(ctx_);
        return true;
      }

      bool EndArray(size_t n) {
        auto array = &(array_stack_.back());
        container_type_stack_.pop_back();
        if (container_type_stack_.empty()) {
          for (auto value : array->values()) {
            grn_id id = GRN_ID_NIL;
            if (value->header.domain == uvector_->header.domain) {
              id = GRN_RECORD_VALUE(value);
            } else {
              grn_obj casted_value;
              GRN_RECORD_INIT(&casted_value, GRN_BULK, uvector_->header.domain);
              auto rc = grn_obj_cast(ctx_,
                                     value,
                                     &casted_value,
                                     add_record_if_not_exist_);
              if (rc == GRN_SUCCESS && GRN_BULK_VSIZE(&casted_value)) {
                id = GRN_RECORD_VALUE(&casted_value);
              }
              GRN_OBJ_FIN(ctx_, &casted_value);
            }
            if (id == GRN_ID_NIL) {
              return false;
            }
            auto rc = grn_uvector_add_element_record(ctx_, uvector_, id, 0);
            if (rc != GRN_SUCCESS) {
              return false;
            }
          }
          return true;
        } else {
          const auto container_type = container_type_stack_.back();
          switch (container_type) {
          case ContainerType::OBJECT :
            {
              auto record = &(record_stack_.back());
              record->add_array_value(array);
              return true;
            }
          case ContainerType::ARRAY :
            // Nested array isn't supported.
            return false;
          default :
            return false;
          }
        }
        array_stack_.pop_back();
        return true;
      }

      bool Bool(bool value) {
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

      bool Int(int value) {
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

      bool Uint(unsigned int value) {
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

      bool Int64(int value) {
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

      bool Uint64(unsigned int value) {
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

      bool Double(double value) {
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

      bool String(const char *data, size_t size, bool copy) {
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
      bool add_value(ObjectAddValue object_add_value,
                     ArrayAddValue array_add_value) {
        const auto container_type = container_type_stack_.back();
        switch (container_type) {
        case ContainerType::OBJECT :
          {
            auto record = &(record_stack_.back());
            return object_add_value(record);
          }
        case ContainerType::ARRAY :
          {
            auto array = &(array_stack_.back());
            return array_add_value(array);
          }
        default :
          return false;
        }
      }

      enum class ContainerType {
        OBJECT,
        ARRAY,
      };

      grn_ctx *ctx_;
      grn_obj *uvector_;
      bool add_record_if_not_exist_;
      std::vector<grn_obj *> table_stack_;
      std::vector<Record> record_stack_;
      std::vector<Array> array_stack_;
      std::vector<ContainerType> container_type_stack_;
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
      if (grn_obj_is_table_with_key(ctx, domain)) {
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
        if (grn_obj_is_weight_uvector(ctx, dest)) {
          if (grn_obj_is_table_with_key(ctx, domain)) {
            rc = json_to_uvector<TableWeightHandler>(ctx,
                                                     &document,
                                                     dest,
                                                     add_record_if_not_exist);
          }
        } else {
          TableHandler handler(ctx, dest, add_record_if_not_exist);
          if (document.Accept(handler)) {
            rc = GRN_SUCCESS;
          }
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
