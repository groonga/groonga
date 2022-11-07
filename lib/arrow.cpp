/*
  Copyright (C) 2017  Brazil
  Copyright (C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_arrow.h"
#include "grn_cast.h"
#include "grn_column.h"
#include "grn_db.h"
#include "grn_output.h"
#include "grn_string.h"

#ifdef GRN_WITH_APACHE_ARROW
#include "grn_arrow.hpp"
#include <groonga/arrow.hpp>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>

#include <map>
#include <sstream>

#if ARROW_VERSION_MAJOR >= 10
using string_view = std::string_view;
#else
using string_view = arrow::util::string_view;
#endif

namespace grnarrow {
  static int64_t grn_arrow_long_time_threshold_usec = GRN_TIME_USEC_PER_SEC;

  grn_rc status_to_rc(const arrow::Status &status) {
    switch (status.code()) {
    case arrow::StatusCode::OK:
      return GRN_SUCCESS;
    case arrow::StatusCode::OutOfMemory:
      return GRN_NO_MEMORY_AVAILABLE;
    case arrow::StatusCode::KeyError:
      return GRN_INVALID_ARGUMENT; // TODO
    case arrow::StatusCode::TypeError:
      return GRN_INVALID_ARGUMENT; // TODO
    case arrow::StatusCode::Invalid:
      return GRN_INVALID_ARGUMENT;
    case arrow::StatusCode::IOError:
      return GRN_INPUT_OUTPUT_ERROR;
    case arrow::StatusCode::UnknownError:
      return GRN_UNKNOWN_ERROR;
    case arrow::StatusCode::NotImplemented:
      return GRN_FUNCTION_NOT_IMPLEMENTED;
    default:
      return GRN_UNKNOWN_ERROR;
    }
  }

  arrow::Status check(grn_ctx *ctx,
                      grn_rc rc,
                      const char *context) {
    switch (rc) {
    case GRN_SUCCESS:
      return arrow::Status::OK();
    default:
      // TODO
      return arrow::Status::UnknownError(context, ": <", rc, ">");
    }
  }

  arrow::Status check(grn_ctx *ctx,
                      grn_rc rc,
                      std::string context) {
    return check(ctx, rc, context.c_str());
  }

  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             const char *context) {
    if (status.ok()) {
      return true;
    } else {
      auto rc = status_to_rc(status);
      auto message = status.ToString();
      ERR(rc, "%s: %s", context, message.c_str());
      return false;
    }
  }

  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             const std::string &context) {
    return check(ctx, status, context.c_str());
  }

  bool check(grn_ctx *ctx,
             const arrow::Status &status,
             std::ostream &output) {
    return check(ctx,
                 status,
                 static_cast<std::stringstream &>(output).str());
  }

  class ObjectCache {
  public:
    ObjectCache(grn_ctx *ctx) : ctx_(ctx) {
    }

    ~ObjectCache() {
      for (auto &it : cached_objects_) {
        auto object = it.second;
        if (object) {
          grn_obj_unref(ctx_, object);
        }
      }
    }

    grn_obj *operator[](grn_id id) {
      auto it = cached_objects_.find(id);
      if (it != cached_objects_.end()) {
        return it->second;
      } else {
        auto object = grn_ctx_at(ctx_, id);
        if (object) {
          cached_objects_[id] = object;
        }
        return object;
      }
    }

  private:
    grn_ctx *ctx_;
    std::map<grn_id, grn_obj *> cached_objects_;
  };

  void put_time_value(grn_ctx *ctx,
                      grn_obj *bulk,
                      int64_t time_value,
                      arrow::TimeUnit::type time_unit) {
    switch (time_unit) {
    case arrow::TimeUnit::SECOND :
      GRN_TIME_PUT(ctx, bulk, GRN_TIME_PACK(time_value, 0));
      break;
    case arrow::TimeUnit::MILLI :
      GRN_TIME_PUT(ctx, bulk, GRN_TIME_MSEC_TO_USEC(time_value));
      break;
    case arrow::TimeUnit::MICRO :
      GRN_TIME_PUT(ctx, bulk, time_value);
      break;
    case arrow::TimeUnit::NANO :
      GRN_TIME_PUT(ctx, bulk, GRN_TIME_NSEC_TO_USEC(time_value));
      break;
    }
  }

  std::shared_ptr<arrow::DataType> grn_type_id_to_arrow_type(grn_ctx *ctx,
                                                             grn_id type_id) {
    switch (type_id) {
    case GRN_DB_BOOL :
      return arrow::boolean();
    case GRN_DB_UINT8 :
      return arrow::uint8();
    case GRN_DB_INT8 :
      return arrow::int8();
    case GRN_DB_UINT16 :
      return arrow::uint16();
    case GRN_DB_INT16 :
      return arrow::int16();
    case GRN_DB_UINT32 :
      return arrow::uint32();
    case GRN_DB_INT32 :
      return arrow::int32();
    case GRN_DB_UINT64 :
      return arrow::uint64();
    case GRN_DB_INT64 :
      return arrow::int64();
    case GRN_DB_FLOAT32 :
      return arrow::float32();
    case GRN_DB_FLOAT :
      return arrow::float64();
    case GRN_DB_TIME :
      return arrow::timestamp(arrow::TimeUnit::NANO);
    case GRN_DB_SHORT_TEXT :
    case GRN_DB_TEXT :
    case GRN_DB_LONG_TEXT :
      return arrow::utf8();
    default :
      return nullptr;
    }
  }

  std::shared_ptr<arrow::DataType> grn_column_to_arrow_type(
    grn_ctx *ctx,
    grn_obj *column,
    ObjectCache &object_cache) {
    switch (column->header.type) {
    case GRN_TYPE :
      return grn_type_id_to_arrow_type(ctx, grn_obj_id(ctx, column));
    case GRN_ACCESSOR :
    case GRN_COLUMN_FIX_SIZE :
    case GRN_COLUMN_VAR_SIZE :
      {
        grn_id range_id = GRN_ID_NIL;
        grn_obj_flags range_flags = 0;
        grn_obj_get_range_info(ctx, column, &range_id, &range_flags);
        auto arrow_type = grn_type_id_to_arrow_type(ctx, range_id);
        if (!arrow_type) {
          auto range = object_cache[range_id];
          if (grn_obj_is_table_with_key(ctx, range)) {
            auto domain = object_cache[range->header.domain];
            arrow_type = grn_column_to_arrow_type(ctx, domain, object_cache);
            if (arrow_type == arrow::utf8()) {
              arrow_type = arrow::dictionary(arrow::int32(), arrow_type);
            }
          }
        }
        if (range_flags & GRN_OBJ_VECTOR) {
          return arrow::list(arrow_type);
        } else {
          return arrow_type;
        }
      }
      break;
    case GRN_COLUMN_INDEX :
      return arrow::uint32();
    default :
      return nullptr;
    }
    return nullptr;
  }

  const std::shared_ptr<arrow::DataType> undictionary(
    const std::shared_ptr<arrow::DataType> data_type)
  {
    if (data_type->id() != arrow::Type::DICTIONARY) {
      return data_type;
    }
    auto dictionary_data_type =
      std::static_pointer_cast<arrow::DictionaryType>(data_type);
    return dictionary_data_type->value_type();
  }

  class RecordAddVisitor : public arrow::ArrayVisitor {
  public:
    RecordAddVisitor(grn_ctx *ctx,
                     grn_loader *grn_loader,
                     std::vector<grn_id> *record_ids,
                     bool is_key)
      : ctx_(ctx),
        grn_loader_(grn_loader),
        record_ids_(record_ids),
        is_key_(is_key),
        bulk_() {
      GRN_VOID_INIT(&bulk_);
    }

    ~RecordAddVisitor() {
      GRN_OBJ_FIN(ctx_, &bulk_);
    }

    arrow::Status Visit(const arrow::BooleanArray &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_BOOL, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_BOOL_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::Int8Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_INT8, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_INT8_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::UInt8Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_UINT8, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_UINT8_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::Int16Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_INT16, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_INT16_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::UInt16Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_UINT16, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_UINT16_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::Int32Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_INT32, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_INT32_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::UInt32Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_UINT32, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_UINT32_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::Int64Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_INT64, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_INT64_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::UInt64Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_UINT64, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_UINT64_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::HalfFloatArray &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_FLOAT32, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_FLOAT32_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::FloatArray &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_FLOAT32, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_FLOAT32_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::DoubleArray &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_FLOAT, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_FLOAT_SET(ctx_, &bulk_, value);
                         });
    }

    arrow::Status Visit(const arrow::StringArray &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_TEXT, GRN_OBJ_DO_SHALLOW_COPY);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.GetView(i);
                           GRN_TEXT_SET(ctx_,
                                        &bulk_,
                                        value.data(),
                                        value.size());
                         });
    }

    arrow::Status Visit(const arrow::Date64Array &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_TIME, 0);
      return add_records(array,
                         [&](int64_t i) {
                           const auto &value = array.Value(i);
                           GRN_TIME_SET(ctx_,
                                        &bulk_,
                                        GRN_TIME_MSEC_TO_USEC(value));
                         });
    }

    arrow::Status Visit(const arrow::TimestampArray &array) {
      grn_obj_reinit(ctx_, &bulk_, GRN_DB_TIME, 0);
      const auto &arrow_timestamp_type =
        std::static_pointer_cast<arrow::TimestampType>(array.type());
      const auto time_unit = arrow_timestamp_type->unit();
      return add_records(array,
                         [&](int64_t i) {
                           GRN_BULK_REWIND(&bulk_);
                           put_time_value(ctx_,
                                          &bulk_,
                                          array.Value(i),
                                          time_unit);
                         });
    }

  private:
    grn_ctx *ctx_;
    grn_loader *grn_loader_;
    std::vector<grn_id> *record_ids_;
    bool is_key_;
    grn_obj bulk_;

    template <typename Array, typename SetBulk>
    arrow::Status add_records(const Array &array, SetBulk set_bulk) {
      const auto n_rows = array.length();
      for (int64_t i = 0; i < n_rows; ++i) {
        grn_id record_id;
        if (array.IsNull(i)) {
          record_id = GRN_ID_NIL;
          grn_loader_on_no_identifier_error(ctx_,
                                            grn_loader_,
                                            grn_loader_->table);
        } else {
          set_bulk(i);
          record_id = add_record();
        }
        grn_loader_on_record_added(ctx_, grn_loader_, record_id);
        record_ids_->push_back(record_id);
      }
      return arrow::Status::OK();
    }

    grn_id add_record() {
      if (is_key_) {
        return grn_table_add_by_key(ctx_,
                                    grn_loader_->table,
                                    &bulk_,
                                    NULL);
      }

      grn_id requested_record_id = GRN_ID_NIL;
      switch (bulk_.header.domain) {
      case GRN_DB_UINT32 :
        requested_record_id = GRN_UINT32_VALUE(&bulk_);
        break;
      case GRN_DB_INT32 :
        requested_record_id = GRN_INT32_VALUE(&bulk_);
        break;
      case GRN_DB_INT64 :
        requested_record_id = static_cast<grn_id>(GRN_INT64_VALUE(&bulk_));
        break;
      default :
        {
          grn_obj casted_record_id;
          GRN_UINT32_INIT(&casted_record_id, 0);
          auto rc = grn_obj_cast(ctx_, &bulk_, &casted_record_id, false);
          if (rc == GRN_SUCCESS) {
            requested_record_id = GRN_UINT32_VALUE(&casted_record_id);
          } else {
            auto ctx = ctx_;
            GRN_DEFINE_NAME(grn_loader_->table);
            grn_obj inspected;
            GRN_TEXT_INIT(&inspected, 0);
            grn_inspect(ctx_, &inspected, &bulk_);
            ERR(GRN_INVALID_ARGUMENT,
                "[table][load][%.*s][%s] failed to cast to <UInt32>: <%.*s>",
                name_size, name,
                GRN_COLUMN_NAME_ID,
                (int)GRN_TEXT_LEN(&inspected),
                GRN_TEXT_VALUE(&inspected));
            GRN_OBJ_FIN(ctx_, &inspected);
          }
          GRN_OBJ_FIN(ctx_, &casted_record_id);
        }
        break;
      }

      if (requested_record_id == GRN_ID_NIL) {
        if (ctx_->rc == GRN_SUCCESS) {
          return grn_table_add(ctx_, grn_loader_->table, NULL, 0, NULL);
        } else {
          return GRN_ID_NIL;
        }
      } else {
        return grn_table_at(ctx_,
                            grn_loader_->table,
                            requested_record_id);
      }
    }
  };

  class ValueLoadVisitor : public arrow::ArrayVisitor {
  public:
    ValueLoadVisitor(grn_ctx *ctx,
                     grn_obj *grn_column,
                     grn_obj *bulk,
                     int64_t index,
                     ObjectCache *object_cache)
      : ctx_(ctx),
        grn_column_(grn_column),
        bulk_(bulk),
        index_(index),
        buffer_(),
        loaded_value_(nullptr),
        object_cache_(object_cache) {
      GRN_VOID_INIT(&buffer_);
    }

    ~ValueLoadVisitor() {
      GRN_OBJ_FIN(ctx_, &buffer_);
    }

    arrow::Status Visit(const arrow::BooleanArray &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_BOOL, 0);
                          GRN_BOOL_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::Int8Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_INT8, 0);
                          GRN_INT8_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::UInt8Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_UINT8, 0);
                          GRN_UINT8_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::Int16Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_INT16, 0);
                          GRN_INT16_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::UInt16Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_UINT16, 0);
                          GRN_UINT16_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::Int32Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_INT32, 0);
                          GRN_INT32_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::UInt32Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_UINT32, 0);
                          GRN_UINT32_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::Int64Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_INT64, 0);
                          GRN_INT64_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::UInt64Array &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_UINT64, 0);
                          GRN_UINT64_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::HalfFloatArray &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_FLOAT32, 0);
                          GRN_FLOAT32_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::FloatArray &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_FLOAT32, 0);
                          GRN_FLOAT32_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::DoubleArray &array) override {
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_FLOAT, 0);
                          GRN_FLOAT_SET(ctx_, &buffer_, array.Value(index_));
                        });
    }

    arrow::Status Visit(const arrow::StringArray &array) override {
      return load_value([&]() {
                          const auto value = array.GetView(index_);
                          grn_obj_reinit(ctx_,
                                         &buffer_,
                                         GRN_DB_TEXT,
                                         GRN_OBJ_DO_SHALLOW_COPY);
                          GRN_TEXT_SET(ctx_,
                                       &buffer_,
                                       value.data(),
                                       value.size());
                        });
    }

    arrow::Status Visit(const arrow::Date64Array &array) override {
      return load_value([&]() {
                          const auto value = array.Value(index_);
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_TIME, 0);
                          GRN_TIME_SET(ctx_,
                                       &buffer_,
                                       GRN_TIME_MSEC_TO_USEC(value));
                        });
    }

    arrow::Status Visit(const arrow::TimestampArray &array) override {
      const auto &arrow_timestamp_type =
        std::static_pointer_cast<arrow::TimestampType>(array.type());
      return load_value([&]() {
                          grn_obj_reinit(ctx_, &buffer_, GRN_DB_TIME, 0);
                          put_time_value(ctx_,
                                         &buffer_,
                                         array.Value(index_),
                                         arrow_timestamp_type->unit());
                        });
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::ListArray &array) override {
      const auto &value_array = array.value_slice(index_);
      switch (value_array->type_id()) {
      case arrow::Type::STRUCT :
        for (int64_t i = 0; i < value_array->length(); ++i) {
          ValueLoadVisitor sub_visitor(ctx_, grn_column_, bulk_, i, object_cache_);
          ARROW_RETURN_NOT_OK(value_array->Accept(&sub_visitor));
        }
        break;
      case arrow::Type::LIST :
        for (int64_t i = 0; i < value_array->length(); ++i) {
          const auto &sub_list_array =
            std::static_pointer_cast<arrow::ListArray>(value_array);
          const auto &sub_value_array = sub_list_array->value_slice(i);
          for (int64_t j = 0; j < sub_value_array->length(); ++j) {
            ValueLoadVisitor sub_visitor(ctx_, grn_column_, bulk_, j, object_cache_);
            ARROW_RETURN_NOT_OK(sub_value_array->Accept(&sub_visitor));
          }
        }
        break;
      default :
        {
          grn_obj sub_buffer;
          if (grn_type_id_is_text_family(ctx_, bulk_->header.domain)) {
            GRN_TEXT_INIT(&sub_buffer, GRN_OBJ_VECTOR);
          } else {
            GRN_VALUE_FIX_SIZE_INIT(&sub_buffer,
                                    GRN_OBJ_VECTOR,
                                    bulk_->header.domain);
          }
          for (int64_t i = 0; i < value_array->length(); ++i) {
            GRN_BULK_REWIND(&sub_buffer);
            ValueLoadVisitor sub_visitor(ctx_,
                                         grn_column_,
                                         &sub_buffer,
                                         i,
                                         object_cache_);
            ARROW_RETURN_NOT_OK(value_array->Accept(&sub_visitor));
            if (ctx_->rc != GRN_SUCCESS) {
              continue;
            }
            auto loaded_value = sub_visitor.loaded_value();
            if (grn_obj_is_vector(ctx_, bulk_)) {
              auto original_value = sub_visitor.original_value();
              if (grn_obj_is_text_family_bulk(ctx_, original_value) &&
                  GRN_TEXT_LEN(original_value) == 0 &&
                  grn_type_id_is_text_family(ctx_, bulk_->header.domain)) {
                grn_vector_add_element_float(ctx_,
                                             bulk_,
                                             NULL,
                                             0,
                                             0.0,
                                             bulk_->header.domain);
              } else {
                grn_vector_copy(ctx_, loaded_value, bulk_);
              }
            } else {
              auto original_value = sub_visitor.original_value();
              if (grn_obj_is_text_family_bulk(ctx_, original_value) &&
                  GRN_TEXT_LEN(original_value) == 0 &&
                  grn_id_maybe_table(ctx_, bulk_->header.domain)) {
                grn_uvector_add_element_record(ctx_, bulk_, GRN_ID_NIL, 0.0);
              } else {
                grn_uvector_copy(ctx_, loaded_value, bulk_);
              }
            }
          }
          GRN_OBJ_FIN(ctx_, &sub_buffer);
        }
        break;
      }
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::StructArray &array) override {
      const auto &value_column = array.GetFieldByName("value");
      if (!value_column) {
        return arrow::Status::OK();
      }
      if (value_column->type_id() != arrow::Type::STRING) {
        return arrow::Status::OK();
      }
      const auto &weight_column = array.GetFieldByName("weight");
      if (!weight_column) {
        return arrow::Status::OK();
      }
      float weight;
      switch (weight_column->type_id()) {
      case arrow::Type::INT32 :
        weight = std::static_pointer_cast<arrow::Int32Array>(weight_column)->Value(index_);
        break;
      case arrow::Type::FLOAT :
        weight = std::static_pointer_cast<arrow::FloatArray>(weight_column)->Value(index_);
        break;
      default :
        return arrow::Status::OK();
      }

      const auto &value =
        std::static_pointer_cast<arrow::StringArray>(value_column)->GetView(index_);
      const grn_id domain = bulk_->header.domain;
      const auto raw_value = value.data();
      const auto raw_value_size = value.size();
      grn_obj *value_bulk;
      grn_obj value_buffer;
      if (grn_type_id_is_text_family(ctx_, domain)) {
        GRN_TEXT_INIT(&value_buffer, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET(ctx_, &value_buffer, raw_value, raw_value_size);
        value_bulk = &value_buffer;
      } else {
        grn_obj raw_value_buffer;
        GRN_TEXT_INIT(&raw_value_buffer, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET(ctx_, &raw_value_buffer, raw_value, raw_value_size);
        GRN_VALUE_FIX_SIZE_INIT(&value_buffer, 0, domain);
        value_bulk = grn_column_cast_value(ctx_,
                                           grn_column_,
                                           &raw_value_buffer,
                                           &value_buffer,
                                           GRN_OBJ_SET);
        if (ctx_->rc != GRN_SUCCESS &&
            (grn_obj_is_vector(ctx_, bulk_) ||
             grn_obj_is_uvector(ctx_, bulk_))) {
          ERRCLR(ctx_);
        }
        GRN_OBJ_FIN(ctx_, &raw_value_buffer);
      }
      if (value_bulk && GRN_BULK_VSIZE(value_bulk) > 0) {
        if (grn_type_id_is_text_family(ctx_, domain)) {
          const auto value_bulk_size =
            static_cast<unsigned int>(GRN_BULK_VSIZE(value_bulk));
          grn_vector_add_element_float(ctx_,
                                       bulk_,
                                       GRN_BULK_HEAD(value_bulk),
                                       value_bulk_size,
                                       weight,
                                       domain);
        } else {
          // TODO: Support weight vector for number such as Int64
          auto id = grn_uvector_get_element_record(ctx_, value_bulk, 0, nullptr);
          grn_uvector_add_element_record(ctx_, bulk_, id, weight);
        }
      }
      GRN_OBJ_FIN(ctx_, &value_buffer);
      return arrow::Status::OK();
    }

    grn_obj *original_value() {
      return &buffer_;
    }

    grn_obj *loaded_value() {
      if (loaded_value_) {
        return loaded_value_;
      }
      return bulk_;
    }

  private:
    grn_ctx *ctx_;
    grn_obj *grn_column_;
    grn_obj *bulk_;
    int64_t index_;
    grn_obj buffer_;
    grn_obj *loaded_value_;
    ObjectCache *object_cache_;

    template <typename LoadBulk>
    arrow::Status
    load_value(LoadBulk load_bulk) {
      load_bulk();
      loaded_value_ = &buffer_;
      if (bulk_->header.domain == GRN_DB_VOID) {
        return arrow::Status::OK();
      }
      if (!grn_column_) {
        return arrow::Status::OK();
      }
      if (grn_obj_is_accessor(ctx_, grn_column_)) {
        if (grn_obj_cast(ctx_, &buffer_, bulk_, true) != GRN_SUCCESS) {
          auto ctx = ctx_;
          auto range = (*object_cache_)[bulk_->header.domain];
          ERR_CAST(grn_column_, range, &buffer_);
        }
        loaded_value_ = bulk_;
      } else {
        loaded_value_ = grn_column_cast_value(ctx_,
                                              grn_column_,
                                              &buffer_,
                                              bulk_,
                                              GRN_OBJ_SET);
        if (ctx_->rc != GRN_SUCCESS &&
            (grn_obj_is_vector(ctx_, bulk_) ||
             grn_obj_is_uvector(ctx_, bulk_))) {
          ERRCLR(ctx_);
        }
      }
      return arrow::Status::OK();
    }
  };

  class ColumnLoadVisitor : public arrow::ArrayVisitor {
  public:
    ColumnLoadVisitor(grn_ctx *ctx,
                      grn_loader *grn_loader,
                      grn_obj *grn_table,
                      const std::shared_ptr<arrow::Field> &arrow_field,
                      const grn_id *record_ids,
                      ObjectCache *object_cache)
      : ctx_(ctx),
        grn_loader_(grn_loader),
        grn_table_(grn_table),
        record_ids_(record_ids),
        column_name_(arrow_field->name()),
        grn_column_(nullptr),
        buffer_(),
        object_cache_(object_cache) {
      if (grn_loader_) {
        grn_column_ = grn_loader_get_column(ctx_,
                                            grn_loader_,
                                            column_name_.data(),
                                            column_name_.size());
      } else {
        grn_column_ = grn_obj_column(ctx_,
                                     grn_table,
                                     column_name_.data(),
                                     column_name_.size());
      }

      const auto &arrow_type = arrow_field->type();
      grn_id arrow_type_id = GRN_DB_VOID;
      grn_obj_flags flags = 0;
      detect_type(arrow_type, &arrow_type_id, &flags);
      if (arrow_type_id == GRN_DB_VOID) {
        // TODO
        GRN_VOID_INIT(&buffer_);
        return;
      }

      if (!grn_column_) {
        if (!grn_loader_) {
          grn_column_ =
            grn_column_create(ctx_,
                              grn_table_,
                              column_name_.data(),
                              static_cast<unsigned int>(column_name_.size()),
                              NULL,
                              GRN_OBJ_COLUMN_SCALAR,
                              (*object_cache_)[arrow_type_id]);
        }
      }
      grn_id range_id = GRN_ID_NIL;
      if (grn_column_) {
        grn_obj_get_range_info(ctx, grn_column_, &range_id, &flags);
      }
      if (range_id == GRN_ID_NIL) {
        range_id = arrow_type_id;
      }
      if (grn_type_id_is_text_family(ctx_, range_id)) {
        GRN_VALUE_VAR_SIZE_INIT(&buffer_, flags, range_id);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&buffer_, flags, range_id);
        if (flags & GRN_OBJ_WITH_WEIGHT) {
          buffer_.header.flags |= GRN_OBJ_WITH_WEIGHT;
        }
      }
    }

    ~ColumnLoadVisitor() {
      if (!grn_loader_ && grn_obj_is_accessor(ctx_, grn_column_)) {
        grn_obj_unlink(ctx_, grn_column_);
      }
      GRN_OBJ_FIN(ctx_, &buffer_);
    }

    arrow::Status Visit(const arrow::BooleanArray &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::Int8Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::UInt8Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::Int16Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::UInt16Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::Int32Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::UInt32Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::Int64Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::UInt64Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::HalfFloatArray &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::FloatArray &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::DoubleArray &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::StringArray &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::Date64Array &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::TimestampArray &array) override {
      return set_values(array);
    }

    arrow::Status Visit(const arrow::ListArray &array) override {
      return set_values(array);
    }

  private:
    grn_ctx *ctx_;
    grn_loader *grn_loader_;
    grn_obj *grn_table_;
    const grn_id *record_ids_;
    std::string column_name_;
    grn_obj *grn_column_;
    grn_obj buffer_;
    ObjectCache *object_cache_;

    void detect_type(const std::shared_ptr<arrow::DataType> &arrow_type,
                     grn_id *type_id,
                     grn_obj_flags *flags) {
      switch (arrow_type->id()) {
      case arrow::Type::BOOL :
        *type_id = GRN_DB_BOOL;
        break;
      case arrow::Type::UINT8 :
        *type_id = GRN_DB_UINT8;
        break;
      case arrow::Type::INT8 :
        *type_id = GRN_DB_INT8;
        break;
      case arrow::Type::UINT16 :
        *type_id = GRN_DB_UINT16;
        break;
      case arrow::Type::INT16 :
        *type_id = GRN_DB_INT16;
        break;
      case arrow::Type::UINT32 :
        *type_id = GRN_DB_UINT32;
        break;
      case arrow::Type::INT32 :
        *type_id = GRN_DB_INT32;
        break;
      case arrow::Type::UINT64 :
        *type_id = GRN_DB_UINT64;
        break;
      case arrow::Type::INT64 :
        *type_id = GRN_DB_INT64;
        break;
      case arrow::Type::HALF_FLOAT :
      case arrow::Type::FLOAT :
        *type_id = GRN_DB_FLOAT32;
        break;
      case arrow::Type::DOUBLE :
        *type_id = GRN_DB_FLOAT;
        break;
      case arrow::Type::STRING :
        *type_id = GRN_DB_TEXT;
        break;
      case arrow::Type::DATE64 :
        *type_id = GRN_DB_TIME;
        break;
      case arrow::Type::TIMESTAMP :
        *type_id = GRN_DB_TIME;
        break;
      case arrow::Type::LIST :
        *flags |= GRN_OBJ_VECTOR;
        {
          grn_obj_flags sub_flags = 0;
          const auto &arrow_list_type =
            std::static_pointer_cast<arrow::ListType>(arrow_type);
          detect_type(arrow_list_type->value_type(), type_id, &sub_flags);
          *flags |= (sub_flags & GRN_OBJ_WITH_WEIGHT);
        }
        break;
      case arrow::Type::STRUCT :
        // Must be weight vector: {"value": string, "weight": int32}
        *type_id = GRN_DB_TEXT;
        *flags |= GRN_OBJ_WITH_WEIGHT;
        break;
      case arrow::Type::MAP :
        // TODO: Support as weight vector
        // *type_id = GRN_DB_TEXT;
        // *flags |= GRN_OBJ_VECTOR | GRN_OBJ_WITH_WEIGHT;
      default :
        *type_id = GRN_DB_VOID;
        break;
      }
    }

    template <typename T>
    arrow::Status set_values(const T &array) {
      int64_t n_rows = array.length();
      for (int i = 0; i < n_rows; ++i) {
        const auto record_id = record_ids_[i];
        if (record_id == GRN_ID_NIL) {
          continue;
        }

        GRN_BULK_REWIND(&buffer_);
        ValueLoadVisitor visitor(ctx_, grn_column_, &buffer_, i, object_cache_);
        ARROW_RETURN_NOT_OK(array.Accept(&visitor));
        if (!grn_column_) {
          auto ctx = ctx_;
          GRN_DEFINE_NAME(grn_table_);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][load][%.*s] nonexistent column: <%s>",
              name_size, name,
              column_name_.data());
          if (grn_loader_) {
            grn_loader_add_record_data data;
            init_grn_loader_add_record_data(&data,
                                            record_id,
                                            visitor.original_value());
            grn_loader_on_column_set(ctx_, grn_loader_, &data);
          }
          continue;
        }

        if (ctx_->rc != GRN_SUCCESS) {
          if (grn_loader_) {
            grn_loader_add_record_data data;
            init_grn_loader_add_record_data(&data,
                                            record_id,
                                            visitor.original_value());
            grn_loader_on_column_set(ctx_, grn_loader_, &data);
          }
          continue;
        }

        auto value = visitor.loaded_value();
        grn_obj_set_value(ctx_, grn_column_, record_id, value, GRN_OBJ_SET);
        if (ctx_->rc != GRN_SUCCESS) {
          if (grn_loader_) {
            grn_loader_add_record_data data;
            init_grn_loader_add_record_data(&data,
                                            record_id,
                                            value);
            grn_loader_on_column_set(ctx_, grn_loader_, &data);
          }
        }
      }
      return check(ctx_, ctx_->rc, "[arrow][column-loader][set-value]");
    }

    void init_grn_loader_add_record_data(grn_loader_add_record_data *data,
                                         grn_id record_id,
                                         grn_obj *value)
    {
      data->table = grn_loader_->table;
      data->depth = 0;
      data->record_value = nullptr;
      data->id = record_id;
      data->key = nullptr;
      data->current.column_name = column_name_.data();
      data->current.column_name_size = column_name_.size();
      data->current.column = grn_column_;
      data->current.value = value;
    }
  };

  class FileLoader {
  public:
    FileLoader(grn_ctx *ctx, grn_obj *grn_table)
      : ctx_(ctx),
        grn_table_(grn_table),
        key_column_name_(""),
        object_cache_(ctx_) {
    }

    ~FileLoader() {
    }

    grn_rc load_table(const std::shared_ptr<arrow::Table> &arrow_table) {
      auto n_columns = arrow_table->num_columns();

      if (key_column_name_.empty()) {
        grn_obj ids;
        GRN_RECORD_INIT(&ids, GRN_OBJ_VECTOR, grn_obj_id(ctx_, grn_table_));
        auto n_records = arrow_table->num_rows();
        for (int64_t i = 0; i < n_records; ++i) {
          auto id = grn_table_add(ctx_, grn_table_, NULL, 0, NULL);
          GRN_RECORD_PUT(ctx_, &ids, id);
        }
        const auto& arrow_schema = arrow_table->schema();
        for (int i = 0; i < n_columns; ++i) {
          int64_t offset = 0;
          const auto& arrow_field = arrow_schema->field(i);
          const auto& arrow_chunked_array = arrow_table->column(i);
          for (const auto& arrow_array : arrow_chunked_array->chunks()) {
            grn_id *sub_ids =
              reinterpret_cast<grn_id *>(GRN_BULK_HEAD(&ids)) + offset;
            ColumnLoadVisitor visitor(ctx_,
                                      nullptr,
                                      grn_table_,
                                      arrow_field,
                                      sub_ids,
                                      &object_cache_);
            auto status = arrow_array->Accept(&visitor);
            offset += arrow_array->length();
          }
        }
        GRN_OBJ_FIN(ctx_, &ids);
      } else {
        auto status = arrow::Status::NotImplemented("_key isn't supported yet");
        check(ctx_, status, "[arrow][load]");
      }
      return ctx_->rc;
    };

    grn_rc load_record_batch(const std::shared_ptr<arrow::RecordBatch> &arrow_record_batch) {
      std::vector<std::shared_ptr<arrow::RecordBatch>> arrow_record_batches =
        {arrow_record_batch};
      auto arrow_table = arrow::Table::FromRecordBatches(arrow_record_batches);
      if (!check(ctx_,
                 arrow_table,
                 "[arrow][load] "
                 "failed to convert record batch to table")) {
        return ctx_->rc;
      }
      return load_table(*arrow_table);
    };

  private:
    grn_ctx *ctx_;
    grn_obj *grn_table_;
    std::string key_column_name_;
    ObjectCache object_cache_;
  };

  class FileDumper {
  public:
    FileDumper(grn_ctx *ctx, grn_obj *grn_table, grn_obj *grn_columns)
      : ctx_(ctx),
        grn_table_(grn_table),
        grn_columns_(grn_columns),
        object_cache_(ctx_),
        tag_("[arrow][dump]") {
    }

    ~FileDumper() {
    }

    grn_rc dump(arrow::io::OutputStream *output) {
      std::vector<std::shared_ptr<arrow::Field>> fields;
      auto n_columns = GRN_BULK_VSIZE(grn_columns_) / sizeof(grn_obj *);
      for (size_t i = 0; i < n_columns; ++i) {
        auto column = GRN_PTR_VALUE_AT(grn_columns_, i);

        char column_name[GRN_TABLE_MAX_KEY_SIZE];
        int column_name_size;
        column_name_size =
          grn_column_name(ctx_, column, column_name, GRN_TABLE_MAX_KEY_SIZE);
        std::string field_name(column_name, column_name_size);
        auto range_id = grn_obj_get_range(ctx_, column);
        auto range = object_cache_[range_id];
        auto field_type = grn_column_to_arrow_type(ctx_, range, object_cache_);
        if (!field_type) {
          continue;
        }
        // We can't use dictionary with FileWriter. It doesn't support
        // dictionary delta.
        field_type = undictionary(field_type);

        auto field = std::make_shared<arrow::Field>(field_name,
                                                    field_type,
                                                    false);
        fields.push_back(field);
      };

      auto schema = std::make_shared<arrow::Schema>(fields);
      auto writer_result = arrow::ipc::MakeFileWriter(output, schema);

      if (!check(ctx_,
                 writer_result,
                 tag_ + " failed to create file format writer")) {
        return ctx_->rc;
      }
      auto writer = *writer_result;

      std::vector<grn_id> ids;
      size_t n_records_per_batch = 1000;
      GRN_TABLE_EACH_BEGIN(ctx_, grn_table_, table_cursor, record_id) {
        ids.push_back(record_id);
        if (ids.size() == n_records_per_batch) {
          write_record_batch(ids, schema, writer);
          ids.clear();
          if (ctx_->rc != GRN_SUCCESS) {
            break;
          }
        }
      } GRN_TABLE_EACH_END(ctx_, table_cursor);
      if (!ids.empty()) {
        write_record_batch(ids, schema, writer);
      }
      if (ctx_->rc != GRN_SUCCESS) {
        return ctx_->rc;
      }
      auto status = writer->Close();
      if (!check(ctx_,
                 status,
                 tag_ + " failed to close writer file format writer")) {
        return ctx_->rc;
      }

      return ctx_->rc;
    }

  private:
    grn_ctx *ctx_;
    grn_obj *grn_table_;
    grn_obj *grn_columns_;
    ObjectCache object_cache_;
    std::string tag_;

    void write_record_batch(std::vector<grn_id> &ids,
                            std::shared_ptr<arrow::Schema> &schema,
                            std::shared_ptr<arrow::ipc::RecordBatchWriter> &writer) {
      std::vector<std::shared_ptr<arrow::Array>> columns;
      auto n_columns = GRN_BULK_VSIZE(grn_columns_) / sizeof(grn_obj *);
      for (size_t i = 0; i < n_columns; ++i) {
        auto grn_column = GRN_PTR_VALUE_AT(grn_columns_, i);

        arrow::Status status;
        std::shared_ptr<arrow::Array> column;

        switch (grn_obj_get_range(ctx_, grn_column)) {
        case GRN_DB_BOOL :
          status = build_boolean_array(ids, grn_column, &column);
          break;
        case GRN_DB_UINT8 :
          status = build_uint8_array(ids, grn_column, &column);
          break;
        case GRN_DB_INT8 :
          status = build_int8_array(ids, grn_column, &column);
          break;
        case GRN_DB_UINT16 :
          status = build_uint16_array(ids, grn_column, &column);
          break;
        case GRN_DB_INT16 :
          status = build_int16_array(ids, grn_column, &column);
          break;
        case GRN_DB_UINT32 :
          status = build_uint32_array(ids, grn_column, &column);
          break;
        case GRN_DB_INT32 :
          status = build_int32_array(ids, grn_column, &column);
          break;
        case GRN_DB_UINT64 :
          status = build_uint64_array(ids, grn_column, &column);
          break;
        case GRN_DB_INT64 :
          status = build_int64_array(ids, grn_column, &column);
          break;
        case GRN_DB_FLOAT32 :
          status = build_float_array(ids, grn_column, &column);
          break;
        case GRN_DB_FLOAT :
          status = build_double_array(ids, grn_column, &column);
          break;
        case GRN_DB_TIME :
          status = build_timestamp_array(ids, grn_column, &column);
          break;
        case GRN_DB_SHORT_TEXT :
        case GRN_DB_TEXT :
        case GRN_DB_LONG_TEXT :
          status = build_utf8_array(ids, grn_column, &column);
          break;
        default :
          status =
            arrow::Status::NotImplemented("[arrow][dumper] not supported type: TODO");
          break;
        }
        if (!status.ok()) {
          continue;
        }
        columns.push_back(column);
      }

      auto record_batch = arrow::RecordBatch::Make(schema, ids.size(), columns);
      check(ctx_,
            writer->WriteRecordBatch(*record_batch),
            tag_ + " failed to write record batch");
    }

    arrow::Status build_boolean_array(std::vector<grn_id> &ids,
                                      grn_obj *grn_column,
                                      std::shared_ptr<arrow::Array> *array) {
      arrow::BooleanBuilder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const grn_bool *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_uint8_array(std::vector<grn_id> &ids,
                                    grn_obj *grn_column,
                                    std::shared_ptr<arrow::Array> *array) {
      arrow::UInt8Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const uint8_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_int8_array(std::vector<grn_id> &ids,
                                   grn_obj *grn_column,
                                   std::shared_ptr<arrow::Array> *array) {
      arrow::Int8Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const int8_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_uint16_array(std::vector<grn_id> &ids,
                                     grn_obj *grn_column,
                                     std::shared_ptr<arrow::Array> *array) {
      arrow::UInt16Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const uint16_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_int16_array(std::vector<grn_id> &ids,
                                    grn_obj *grn_column,
                                    std::shared_ptr<arrow::Array> *array) {
      arrow::Int16Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const int16_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_uint32_array(std::vector<grn_id> &ids,
                                     grn_obj *grn_column,
                                     std::shared_ptr<arrow::Array> *array) {
      arrow::UInt32Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const uint32_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_int32_array(std::vector<grn_id> &ids,
                                    grn_obj *grn_column,
                                    std::shared_ptr<arrow::Array> *array) {
      arrow::Int32Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const int32_t *>(data))));
      }
      return builder.Finish(array);
    }
    arrow::Status build_uint64_array(std::vector<grn_id> &ids,
                                     grn_obj *grn_column,
                                     std::shared_ptr<arrow::Array> *array) {
      arrow::UInt64Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const uint64_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_int64_array(std::vector<grn_id> &ids,
                                    grn_obj *grn_column,
                                    std::shared_ptr<arrow::Array> *array) {
      arrow::Int64Builder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const int64_t *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_float_array(std::vector<grn_id> &ids,
                                    grn_obj *grn_column,
                                    std::shared_ptr<arrow::Array> *array) {
      arrow::FloatBuilder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const float *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_double_array(std::vector<grn_id> &ids,
                                     grn_obj *grn_column,
                                     std::shared_ptr<arrow::Array> *array) {
      arrow::DoubleBuilder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(
          builder.Append(*(reinterpret_cast<const double *>(data))));
      }
      return builder.Finish(array);
    }

    arrow::Status build_timestamp_array(std::vector<grn_id> &ids,
                                        grn_obj *grn_column,
                                        std::shared_ptr<arrow::Array> *array) {
      auto timestamp_ns_data_type =
        std::make_shared<arrow::TimestampType>(arrow::TimeUnit::NANO);
      arrow::TimestampBuilder builder(timestamp_ns_data_type,
                                      arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        auto timestamp_micro = *(reinterpret_cast<const int64_t *>(data));
        ARROW_RETURN_NOT_OK(
          builder.Append(GRN_TIME_USEC_TO_NSEC(timestamp_micro)));
      }
      return builder.Finish(array);
    }

    arrow::Status build_utf8_array(std::vector<grn_id> &ids,
                                   grn_obj *grn_column,
                                   std::shared_ptr<arrow::Array> *array) {
      arrow::StringBuilder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        ARROW_RETURN_NOT_OK(builder.Append(data, size));
      }
      return builder.Finish(array);
    }
  };

  class BufferInputStream : public arrow::io::InputStream,
                            public arrow::io::Seekable {
  public:
    BufferInputStream()
      : buffer_(),
        offset_(0),
        closed_(false) {
    }

    ~BufferInputStream() {
    }

    void feed(const char *data, size_t data_size) {
      if (offset_ == 0) {
        buffer_.append(data, data_size);
      } else {
        buffer_.erase(0, offset_);
        buffer_.append(data, data_size);
        offset_ = 0;
      }
    }

    arrow::Status Close() override {
      closed_ = true;
      return arrow::Status::OK();
    }

    arrow::Status Abort() override {
      closed_ = true;
      return arrow::Status::OK();
    }

    int64_t tell() const {
      return offset_;
    }

    arrow::Result<int64_t> Tell() const override {
      return tell();
    }

    arrow::Status Seek(int64_t position) override {
      offset_ = position;
      return arrow::Status::OK();
    }

    bool closed() const override {
      return closed_;
    }

    string_view peek(int64_t nbytes) {
      const int64_t bytes_available =
        std::min(nbytes,
                 static_cast<int64_t>(buffer_.size() - offset_));
      return string_view(buffer_.data() + offset_,
                         static_cast<size_t>(bytes_available));
    }

    arrow::Result<string_view> Peek(int64_t nbytes) override {
      return peek(nbytes);
    }

    int64_t read(int64_t nbytes, void* out) {
      const int64_t bytes_available =
        std::min(nbytes,
                 static_cast<int64_t>(buffer_.size() - offset_));
      if (bytes_available > 0) {
        grn_memcpy(out, buffer_.data() + offset_, bytes_available);
        offset_ += bytes_available;
        return bytes_available;
      } else {
        return 0;
      }
    }

    arrow::Result<int64_t> Read(int64_t nbytes, void* out) override {
      return read(nbytes, out);
    }

    std::shared_ptr<arrow::Buffer> read(int64_t nbytes) {
      const int64_t bytes_available =
        std::min(nbytes,
                 static_cast<int64_t>(buffer_.size() - offset_));
      auto output = std::make_shared<arrow::Buffer>(
        reinterpret_cast<const uint8_t *>(buffer_.data() + offset_),
        bytes_available);
      offset_ += bytes_available;
      return output;
    }

    arrow::Result<std::shared_ptr<arrow::Buffer>> Read(int64_t nbytes) override {
      return read(nbytes);
    }

    bool supports_zero_copy() const override {
      return false;
    }

  private:
    std::string buffer_;
    int64_t offset_;
    bool closed_;
  };

  class StreamLoader : public arrow::ipc::Listener {
  public:
    StreamLoader(grn_ctx *ctx, grn_loader *loader)
      : ctx_(ctx),
        grn_loader_(loader),
        decoder_(std::shared_ptr<StreamLoader>(this, [](void*) {})),
        buffer_(nullptr),
        object_cache_(ctx_),
        tag_("[arrow][stream-loader]") {
      grn_timeval_now(ctx_, &last_recordbatch_decoded_time_);
    }

    grn_rc consume(const char *data, size_t data_size) {
      if (data_size == 0) {
        return GRN_SUCCESS;
      }

      return consume_decoder(data, data_size);
    }

    arrow::Status OnRecordBatchDecoded(std::shared_ptr<arrow::RecordBatch> record_batch) override {
      grn_timeval current_time;
      grn_timeval_now(ctx_, &current_time);
      int64_t wait_time = GRN_TIME_PACK(last_recordbatch_decoded_time_.tv_sec, GRN_TIME_NSEC_TO_USEC(last_recordbatch_decoded_time_.tv_nsec)) -
                          GRN_TIME_PACK(current_time.tv_sec, GRN_TIME_NSEC_TO_USEC(current_time.tv_nsec));
      if (wait_time > grn_arrow_long_time_threshold_usec) {
        int64_t sec;
        int32_t usec;
        GRN_TIME_UNPACK(wait_time, sec, usec);
        GRN_LOG(ctx_, GRN_LOG_DEBUG, "[Arrow][StreamLoader][OnRecordBatchDecoded] took a long time to wait for a next recordbatch: "
                "(%" GRN_FMT_INT64D ".%" GRN_FMT_INT32D ")",
                sec, usec);
      }
      auto result = process_record_batch(std::move(record_batch));
      grn_timeval_now(ctx_, &last_recordbatch_decoded_time_);
      return result;
    }

  private:
    grn_rc consume_decoder(const char *data, size_t data_size) {
      if (!buffer_) {
        auto buffer = arrow::AllocateResizableBuffer(0);
        if (!check(ctx_,
                   buffer,
                   tag_ + "[consume] failed to allocate buffer")) {
          return ctx_->rc;
        }
        buffer_ = std::move(*buffer);
      }

      auto current_buffer_size = buffer_->size();
      if (!check(ctx_,
                 buffer_->Resize(current_buffer_size + data_size),
                 tag_ + "[consume] failed to resize buffer")) {
        return ctx_->rc;
      }
      grn_memcpy(buffer_->mutable_data() + current_buffer_size,
                 data,
                 data_size);

      if (buffer_->size() < decoder_.next_required_size()) {
        return ctx_->rc;
      }

      std::shared_ptr<arrow::Buffer> chunk(buffer_.release());
      if (!check(ctx_,
                 decoder_.Consume(chunk),
                 tag_ + "[consume] failed to consume")) {
        return ctx_->rc;
      }
      return ctx_->rc;
    }

    arrow::Status process_record_batch(
      std::shared_ptr<arrow::RecordBatch> record_batch) {
      auto grn_table = grn_loader_->table;
      const auto &key_column = record_batch->GetColumnByName("_key");
      const auto &id_column = record_batch->GetColumnByName("_id");
      const auto n_records = record_batch->num_rows();
      std::vector<grn_id> record_ids;
      if (key_column) {
        RecordAddVisitor visitor(ctx_,
                                 grn_loader_,
                                 &record_ids,
                                 true);
        ARROW_RETURN_NOT_OK(key_column->Accept(&visitor));
      } else if (id_column) {
        RecordAddVisitor visitor(ctx_,
                                 grn_loader_,
                                 &record_ids,
                                 false);
        ARROW_RETURN_NOT_OK(id_column->Accept(&visitor));
      } else {
        for (int64_t i = 0; i < n_records; ++i) {
          const auto record_id = grn_table_add(ctx_, grn_table, NULL, 0, NULL);
          grn_loader_on_record_added(ctx_, grn_loader_, record_id);
          record_ids.push_back(record_id);
        }
      }

      const auto &schema = record_batch->schema();
      const auto n_columns = record_batch->num_columns();
      for (int i = 0; i < n_columns; ++i) {
        const auto &column = record_batch->column(i);
        if (column == key_column) {
          continue;
        }
        ColumnLoadVisitor visitor(ctx_,
                                  grn_loader_,
                                  grn_table,
                                  schema->field(i),
                                  record_ids.data(),
                                  &object_cache_);
        ARROW_RETURN_NOT_OK(column->Accept(&visitor));
      }
      for (const auto record_id : record_ids) {
        if (record_id == GRN_ID_NIL) {
          continue;
        }
        grn_loader_apply_each(ctx_, grn_loader_, record_id);
      }
      return check(ctx_,
                   ctx_->rc,
                   tag_ + "[consume][record-batch-decoded]");
    }

    grn_ctx *ctx_;
    grn_loader *grn_loader_;
    arrow::ipc::StreamDecoder decoder_;
    std::unique_ptr<arrow::ResizableBuffer> buffer_;
    ObjectCache object_cache_;
    std::string tag_;
    grn_timeval last_recordbatch_decoded_time_;
  };

  class BulkOutputStream : public arrow::io::OutputStream {
  public:
    BulkOutputStream(grn_ctx *ctx, grn_obj *bulk)
      : arrow::io::OutputStream(),
        ctx_(ctx),
        bulk_(bulk),
        position_(0),
        is_open_(true) {
    }

    ~BulkOutputStream() override {
    }

    grn_obj *bulk() const {
      return bulk_;
    }

    arrow::Status Close() override {
      is_open_ = false;
      return arrow::Status::OK();
    }

    bool closed() const override {
      return !is_open_;
    }

    arrow::Result<int64_t> Tell() const override {
      return position_;
    }

    arrow::Status Write(const void *data, int64_t n_bytes) override {
      if (ARROW_PREDICT_FALSE(!is_open_)) {
        return arrow::Status::IOError("BulkOutputStream is closed");
      }
      if (ARROW_PREDICT_TRUE(n_bytes > 0)) {
        auto rc = grn_bulk_write(ctx_,
                                 bulk_,
                                 static_cast<const char *>(data),
                                 n_bytes);
        if (ARROW_PREDICT_TRUE(rc == GRN_SUCCESS)) {
          position_ += n_bytes;
          return arrow::Status::OK();
        } else {
          return check(ctx_, rc, "[arrow][bulk-output-stream][write]");
        }
      } else {
        return arrow::Status::OK();
      }
    }

    using arrow::io::OutputStream::Write;

  private:
    grn_ctx *ctx_;
    grn_obj *bulk_;
    int64_t position_;
    bool is_open_;
  };

  class StreamWriter {
  public:
    StreamWriter(grn_ctx *ctx, grn_obj *bulk)
      : ctx_(ctx),
        output_(ctx, bulk),
        schema_builder_(),
        schema_(),
        writer_(),
        record_batch_builder_(),
        n_records_(0),
        current_column_index_(0),
        object_cache_(ctx_),
        tag_("[arrow][stream-writer]") {
    }

    ~StreamWriter() {
      flush();
      if (writer_) {
        std::ignore = writer_->Close();
      }
      std::ignore = output_.Close();
    }

    void add_metadata(const char *key, const char *value) {
      arrow::KeyValueMetadata metadata;
      metadata.Append(key, value);
      auto status = schema_builder_.AddMetadata(metadata);
      if (!status.ok()) {
        std::stringstream context;
        check(ctx_,
              status,
              context <<
              tag_ <<
              "[add-meatadata] " <<
              "failed to add metadata: <" <<
              key <<
              ">: <" <<
              value <<
              ">");
      }
    }

    void add_field(const char *name, grn_obj *column) {
      auto type = grn_column_to_arrow_type(ctx_, column, object_cache_);
      if (!type) {
        auto ctx = ctx_;
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx_, &inspected, column);
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s[add-field] unsupported column: <%.*s>",
            tag_.c_str(),
            (int)GRN_TEXT_LEN(&inspected),
            GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx_, &inspected);
        return;
      }
      auto field = arrow::field(name, type);
      auto status = schema_builder_.AddField(field);
      if (!status.ok()) {
        std::stringstream context;
        check(ctx_,
              status,
              context <<
              tag_ <<
              "[add-field] " <<
              "failed to add field: <" <<
              field->ToString() <<
              ">");
      }
    }

    void write_schema() {
      auto schema_result = schema_builder_.Finish();
      if (!check(ctx_,
                 schema_result,
                 tag_ + "[write-schema] failed to create schema")) {
          return;
      }
      schema_builder_.Reset();
      schema_ = *schema_result;

      auto option = arrow::ipc::IpcWriteOptions::Defaults();
      option.emit_dictionary_deltas = true;
      auto writer_result =
        arrow::ipc::MakeStreamWriter(&output_, schema_, option);

      if (!check(ctx_,
                 writer_result,
                 tag_ + "[write-schema] failed to create writer")) {
        return;
      }
      writer_ = *writer_result;

#if ARROW_VERSION_MAJOR >= 9
      auto record_batch_builder_result =
        arrow::RecordBatchBuilder::Make(schema_,
                                        arrow::default_memory_pool());
      if (record_batch_builder_result.ok()) {
        record_batch_builder_.swap(*record_batch_builder_result);
      }
      auto status = record_batch_builder_result.status();
#else
      auto status =
        arrow::RecordBatchBuilder::Make(schema_,
                                        arrow::default_memory_pool(),
                                        &record_batch_builder_);
#endif
      check(ctx_,
            status,
            tag_ + "[write-schema] failed to create record batch builder");
    }

    void open_record() {
      current_column_index_ = 0;
    }

    void close_record() {
      n_records_++;
      if (n_records_ == grn_output_auto_flush_interval) {
        flush();
      }
    }

    void add_column_string(const char *value, size_t value_length) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::StringBuilder>(
          current_column_index_++);
      auto status = column_builder->Append(value, value_length);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][string] " <<
            "failed to add a column value: <" <<
            string_view(value, value_length) <<
            ">");
    }

    void add_column_int8(int8_t value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::Int8Builder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][int8] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_int32(int32_t value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::Int32Builder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][int32] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_uint32(uint32_t value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::UInt32Builder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][uint32] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_int64(int64_t value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::Int64Builder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][int64] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_uint64(int64_t value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::UInt64Builder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][uint64] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_float32(float value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::FloatBuilder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][float32] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_float(double value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::DoubleBuilder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][float] " <<
            "failed to add a column value: <" << value << ">");
    }

    void add_column_timestamp(grn_timeval value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::TimestampBuilder>(
          current_column_index_++);
      auto status = column_builder->Append(GRN_TIMEVAL_TO_NSEC(&value));
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][timestamp] " <<
            "failed to add a column value: <" <<
            (value.tv_sec + (value.tv_nsec / GRN_TIME_NSEC_PER_SEC_F)) <<
            ">");
    }

    void add_column_double(double value) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::DoubleBuilder>(
          current_column_index_++);
      auto status = column_builder->Append(value);
      if (!status.ok()) {
        return;
      }
      std::stringstream context;
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][double] " <<
            "failed to add a column value: <" <<
            value <<
            ">");
    }

    void add_column_record(grn_obj *record) {
      auto table = object_cache_[record->header.domain];
      char key[GRN_TABLE_MAX_KEY_SIZE];
      auto key_size = grn_table_get_key(ctx_,
                                        table,
                                        GRN_RECORD_VALUE(record),
                                        key,
                                        sizeof(key));
      switch (table->header.domain) {
      case GRN_DB_INT32 :
        add_column_int32(*reinterpret_cast<int32_t *>(key));
        return;
      default :
        break;
      }
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::StringDictionaryBuilder>(
          current_column_index_++);
      auto status =
        column_builder->Append(string_view(key, key_size));
      if (status.ok()) {
        return;
      }
      std::stringstream context;
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx_, &inspected, record);
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][record] " <<
            "failed to add a column value: <" <<
            string_view(GRN_TEXT_VALUE(&inspected),
                        GRN_TEXT_LEN(&inspected)) <<
            ">");
      GRN_OBJ_FIN(ctx_, &inspected);
    }

    void add_column_uvector(grn_obj *uvector) {
      auto column_builder =
        record_batch_builder_->GetFieldAs<arrow::ListBuilder>(
          current_column_index_++);
      auto status = column_builder->Append();
      if (status.ok()) {
        auto domain = object_cache_[uvector->header.domain];
        auto raw_elements = GRN_BULK_HEAD(uvector);
        auto element_size = grn_uvector_element_size(ctx_, uvector);
        auto n = GRN_BULK_VSIZE(uvector) / element_size;
        auto raw_value_builder = column_builder->value_builder();
        if (grn_obj_is_table_with_key(ctx_, domain)) {
          auto value_builder =
            static_cast<arrow::StringDictionaryBuilder *>(raw_value_builder);
          for (size_t i = 0; i < n; ++i) {
            auto record_id =
              *reinterpret_cast<grn_id *>(raw_elements + (element_size * i));
            char key[GRN_TABLE_MAX_KEY_SIZE];
            auto key_size = grn_table_get_key(ctx_,
                                              domain,
                                              record_id,
                                              key,
                                              sizeof(key));
            status =
              value_builder->Append(string_view(key, key_size));
            if (!status.ok()) {
              break;
            }
          }
        } else {
          auto value_builder =
            static_cast<arrow::Int32Builder *>(raw_value_builder);
          for (size_t i = 0; i < n; ++i) {
            // TODO: check type
            auto element =
              *reinterpret_cast<int32_t *>(raw_elements + (element_size * i));
            status = value_builder->Append(element);
            if (!status.ok()) {
              break;
            }
          }
        }
      }
      if (status.ok()) {
        return;
      }
      std::stringstream context;
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx_, &inspected, uvector);
      check(ctx_,
            status,
            context <<
            tag_ <<
            "[add-column][uvector] " <<
            "failed to add a column value: <" <<
            string_view(GRN_TEXT_VALUE(&inspected),
                        GRN_TEXT_LEN(&inspected)) <<
            ">");
      GRN_OBJ_FIN(ctx_, &inspected);
    }

    void flush() {
      if (n_records_ == 0) {
        return;
      }

      std::shared_ptr<arrow::RecordBatch> record_batch;
#if ARROW_VERSION_MAJOR >= 9
      auto record_batch_result = record_batch_builder_->Flush();
      if (record_batch_result.ok()) {
        record_batch = *record_batch_result;
      }
      auto status = record_batch_result.status();
#else
      auto status = record_batch_builder_->Flush(&record_batch);
#endif
      if (check(ctx_,
                status,
                tag_ + "[flush] failed to flush record batch")) {
        status = writer_->WriteRecordBatch(*record_batch);
        check(ctx_,
              status,
              tag_ + "[flush] failed to write flushed record batch");
      }

      auto n_fields = record_batch_builder_->num_fields();
      for (int i = 0; i < n_fields; ++i) {
        reset_full(record_batch_builder_->GetField(i));
      }

      n_records_ = 0;
    }

  private:
    grn_ctx *ctx_;
    BulkOutputStream output_;
    arrow::SchemaBuilder schema_builder_;
    std::shared_ptr<arrow::Schema> schema_;
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer_;
    std::unique_ptr<arrow::RecordBatchBuilder> record_batch_builder_;
    size_t n_records_;
    int current_column_index_;
    ObjectCache object_cache_;
    std::string tag_;

    // We need to execute ResetFull() with an appropriate dictionary length
    // in order to avoid memory exhaustion and so on.
    // - Dictionary data in DictionaryBuilder are held in memory until
    //   ResetFull() is executed.
    // - Some Apache Arrow implementations (clients) have a 2GB limitation on
    //   the value size of array: C#.
    void reset_full(arrow::ArrayBuilder *builder) {
      switch (builder->type()->id()) {
      case arrow::Type::DICTIONARY :
        {
          const int64_t dictionary_length_threshold = 10000;

          // The value type of Dictionary is always string for now.
          auto dictionary_builder =
            static_cast<arrow::StringDictionaryBuilder *>(builder);
          if (dictionary_builder->dictionary_length() >
              dictionary_length_threshold) {
            dictionary_builder->ResetFull();
          }
        }
        break;
      case arrow::Type::LIST :
        {
          auto list_builder = static_cast<arrow::ListBuilder *>(builder);
          auto value_builder = list_builder->value_builder();
          reset_full(value_builder);
        }
        break;
      default :
        break;
      }
    }
  };
}

namespace grn {
  namespace arrow {
    struct ArrayBuilder::Impl {
      struct ColumnAppender : ::arrow::TypeVisitor {
        grn_ctx *ctx_;
        grn_obj *column_;
        grn_table_cursor *cursor_;
        ::arrow::ArrayBuilder *builder_;

        ColumnAppender(grn_ctx *ctx,
                       grn_obj *column,
                       grn_table_cursor *cursor,
                       ::arrow::ArrayBuilder *builder) :
          ctx_(ctx),
          column_(column),
          cursor_(cursor),
          builder_(builder) {
        }

#define VISIT(TYPE)                                             \
        ::arrow::Status                                         \
        Visit(const TYPE &type) override {                      \
          return Append(type);                                  \
        }

        VISIT(::arrow::BooleanType)
        VISIT(::arrow::Int8Type)
        VISIT(::arrow::UInt8Type)
        VISIT(::arrow::Int16Type)
        VISIT(::arrow::UInt16Type)
        VISIT(::arrow::Int32Type)
        VISIT(::arrow::UInt32Type)
        VISIT(::arrow::Int64Type)
        VISIT(::arrow::UInt64Type)
        VISIT(::arrow::FloatType)
        VISIT(::arrow::DoubleType)
        VISIT(::arrow::StringType)
        VISIT(::arrow::TimestampType)

#undef VISIT

        template <typename Type>
        ::arrow::Status Append(const Type& type) {
          using Builder = typename ::arrow::TypeTraits<Type>::BuilderType;
          using CType = typename ::arrow::TypeTraits<Type>::CType;
          auto builder = static_cast<Builder *>(builder_);
          grn_id id;
          while ((id = grn_table_cursor_next(ctx_, cursor_)) != GRN_ID_NIL) {
            uint32_t size;
            auto raw_data = grn_obj_get_value_(ctx_, column_, id, &size);
            const auto data = *reinterpret_cast<const CType *>(raw_data);
            ARROW_RETURN_NOT_OK(builder->Append(data));
          }
          return ::arrow::Status::OK();
        }

        ::arrow::Status Append(const ::arrow::StringType& type) {
          using Builder = ::arrow::StringBuilder;
          auto builder = static_cast<Builder *>(builder_);
          grn_id id;
          while ((id = grn_table_cursor_next(ctx_, cursor_)) != GRN_ID_NIL) {
            uint32_t size;
            auto raw_data = grn_obj_get_value_(ctx_, column_, id, &size);
            ARROW_RETURN_NOT_OK(builder->Append(raw_data, size));
          }
          return ::arrow::Status::OK();
        }
      };

      grn_ctx *ctx_;
      std::unique_ptr<::arrow::ArrayBuilder> builder_;
      grnarrow::ObjectCache object_cache_;

      Impl(grn_ctx *ctx) : ctx_(ctx),
                           builder_(nullptr),
                           object_cache_(ctx_) {
      }

      ::arrow::Status add_column(grn_obj *column,
                                 grn_table_cursor *cursor) {
        auto arrow_type =
          grn_column_to_arrow_type(ctx_, column, object_cache_);
        if (!arrow_type) {
          grn::TextBulk inspected(ctx_);
          grn_inspect(ctx_, *inspected, column);
          return ::arrow::Status::NotImplemented(
            "[arrow][array-builder][add-column] "
            "unsupported column: ", inspected.value());
        }
        arrow_type = grnarrow::undictionary(arrow_type);
        if (!builder_) {
          ARROW_RETURN_NOT_OK(
            ::arrow::MakeBuilder(::arrow::default_memory_pool(),
                                 arrow_type,
                                 &builder_));
        }
        ColumnAppender appender(ctx_, column, cursor, builder_.get());
        return arrow_type->Accept(&appender);
      }

      ::arrow::Result<std::shared_ptr<::arrow::Array>>
      finish() {
        return builder_->Finish();
      }
    };

    ArrayBuilder::ArrayBuilder(grn_ctx *ctx) : impl_(new Impl(ctx)) {
    }

    ArrayBuilder::~ArrayBuilder() = default;

    ::arrow::Status ArrayBuilder::add_column(grn_obj *column,
                                             grn_table_cursor *cursor) {
      return impl_->add_column(column, cursor);
    }

    ::arrow::Result<std::shared_ptr<::arrow::Array>>
    ArrayBuilder::finish() {
      return impl_->finish();
    }

    namespace {
      class ArrayValueGetter : public ::arrow::ArrayVisitor {
      public:
        ArrayValueGetter(grn_ctx *ctx,
                         int64_t index,
                         grn_obj *value) :
          ctx_(ctx),
          index_(index),
          value_(value) {
        }

        ::arrow::Status Visit(const ::arrow::BooleanArray& array) {
          GRN_BOOL_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::Int8Array& array) {
          GRN_INT8_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::UInt8Array& array) {
          GRN_UINT8_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::Int16Array& array) {
          GRN_INT16_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::UInt16Array& array) {
          GRN_UINT16_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::Int32Array& array) {
          GRN_INT32_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::UInt32Array& array) {
          GRN_UINT32_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::Int64Array& array) {
          GRN_INT64_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::UInt64Array& array) {
          GRN_UINT64_PUT(ctx_, value_, array.Value(index_));
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::TimestampArray& array) {
          const auto &arrow_timestamp_type =
            std::static_pointer_cast<::arrow::TimestampType>(array.type());
          const auto time_unit = arrow_timestamp_type->unit();
          grnarrow::put_time_value(ctx_,
                                   value_,
                                   array.Value(index_),
                                   time_unit);
          return ::arrow::Status::OK();
        }

        ::arrow::Status Visit(const ::arrow::StringArray& array) {
          auto raw_value = array.GetView(index_);
          grn_bulk_write(ctx_, value_, raw_value.data(), raw_value.length());
          return ::arrow::Status::OK();
        }

      private:
        grn_ctx *ctx_;
        int64_t index_;
        grn_obj *value_;
      };
    }

    grn_rc get_value(grn_ctx *ctx,
                     const ::arrow::Array *array,
                     int64_t index,
                     grn_obj *value) {
      ArrayValueGetter getter(ctx, index, value);
      auto status = array->Accept(&getter);
      grnarrow::check(ctx, status, "[arrow][value][get] failed");
      return ctx->rc;
    }
  }
}
#endif /* GRN_WITH_APACHE_ARROW */

extern "C" {

void
grn_arrow_init_from_env(void)
{
#ifdef GRN_WITH_APACHE_ARROW
  {
    char grn_arrow_long_time_threshold_usec_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_ARROW_LONG_TIME_THRESHOLD_USEC",
               grn_arrow_long_time_threshold_usec_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_arrow_long_time_threshold_usec_env[0]) {
      grnarrow::grn_arrow_long_time_threshold_usec =
        grn_atoll(grn_arrow_long_time_threshold_usec_env,
                  grn_arrow_long_time_threshold_usec_env +
                  strlen(grn_arrow_long_time_threshold_usec_env),
                  NULL);
    }
  }
#endif
}

grn_rc
grn_arrow_load(grn_ctx *ctx,
               grn_obj *table,
               const char *path)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  auto input_result =
    arrow::io::MemoryMappedFile::Open(path, arrow::io::FileMode::READ);
  std::ostringstream context;
  if (!grnarrow::check(ctx,
                       input_result,
                       context <<
                       "[arrow][load] failed to open path: " <<
                       "<" << path << ">")) {
    GRN_API_RETURN(ctx->rc);
  }
  auto input = *input_result;
  auto reader_result = arrow::ipc::RecordBatchFileReader::Open(input);
  if (!grnarrow::check(ctx,
                       reader_result,
                       "[arrow][load] "
                       "failed to create file format reader")) {
    GRN_API_RETURN(ctx->rc);
  }
  auto reader = *reader_result;

  grnarrow::FileLoader loader(ctx, table);
  int n_record_batches = reader->num_record_batches();
  for (int i = 0; i < n_record_batches; ++i) {
    auto record_batch_result = reader->ReadRecordBatch(i);
    std::ostringstream context;
    if (!grnarrow::check(ctx,
                         record_batch_result,
                         context <<
                         "[arrow][load] failed to get " <<
                         "the " << i << "-th " << "record")) {
      break;
    }
    auto record_batch = *record_batch_result;
    loader.load_record_batch(record_batch);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
#else /* GRN_WITH_APACHE_ARROW */
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][load] Apache Arrow support isn't enabled");
#endif /* GRN_WITH_APACHE_ARROW */
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_dump(grn_ctx *ctx,
               grn_obj *table,
               const char *path)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  auto all_columns =
    grn_hash_create(ctx,
                    NULL,
                    sizeof(grn_id),
                    0,
                    GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  grn_table_columns(ctx,
                    table,
                    "", 0,
                    reinterpret_cast<grn_obj *>(all_columns));

  grnarrow::ObjectCache object_cache(ctx);
  grn_obj columns;
  GRN_PTR_INIT(&columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_HASH_EACH_BEGIN(ctx, all_columns, cursor, id) {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    auto column_id = static_cast<grn_id *>(key);
    auto column = object_cache[*column_id];
    GRN_PTR_PUT(ctx, &columns, column);
  } GRN_HASH_EACH_END(ctx, cursor);
  grn_hash_close(ctx, all_columns);

  grn_arrow_dump_columns(ctx, table, &columns, path);
  GRN_OBJ_FIN(ctx, &columns);
#else /* GRN_WITH_APACHE_ARROW */
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][dump] Apache Arrow support isn't enabled");
#endif /* GRN_WITH_APACHE_ARROW */
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_dump_columns(grn_ctx *ctx,
                       grn_obj *table,
                       grn_obj *columns,
                       const char *path)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  auto output_result = arrow::io::FileOutputStream::Open(path);
  std::stringstream context;
  if (!grnarrow::check(ctx,
                       output_result,
                       context <<
                       "[arrow][dump] failed to open path: " <<
                       "<" << path << ">")) {
    GRN_API_RETURN(ctx->rc);
  }
  auto output = *output_result;

  grnarrow::FileDumper dumper(ctx, table, columns);
  dumper.dump(output.get());
#else /* GRN_WITH_APACHE_ARROW */
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][dump] Apache Arrow support isn't enabled");
#endif /* GRN_WITH_APACHE_ARROW */
  GRN_API_RETURN(ctx->rc);
}

#ifdef GRN_WITH_APACHE_ARROW
struct _grn_arrow_stream_loader {
  grnarrow::StreamLoader *loader;
};
#endif

grn_arrow_stream_loader *
grn_arrow_stream_loader_open(grn_ctx *ctx,
                             grn_loader *loader)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  grn_arrow_stream_loader *arrow_stream_loader;
  arrow_stream_loader = static_cast<grn_arrow_stream_loader *>(
    GRN_CALLOC(sizeof(grn_arrow_stream_loader)));
  arrow_stream_loader->loader = new grnarrow::StreamLoader(ctx, loader);
  GRN_API_RETURN(arrow_stream_loader);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-loader][open] Apache Arrow support isn't enabled");
  GRN_API_RETURN(NULL);
#endif
}

grn_rc
grn_arrow_stream_loader_close(grn_ctx *ctx,
                              grn_arrow_stream_loader *loader)
{
  if (!loader) {
    return ctx->rc;
  }

  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  delete loader->loader;
  GRN_FREE(loader);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-loader][close] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_loader_consume(grn_ctx *ctx,
                                grn_arrow_stream_loader *loader,
                                const char *data,
                                size_t data_size)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  loader->loader->consume(data, data_size);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-loader][consume] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

#ifdef GRN_WITH_APACHE_ARROW
struct _grn_arrow_stream_writer {
  grnarrow::StreamWriter *writer;
};
#endif

grn_arrow_stream_writer *
grn_arrow_stream_writer_open(grn_ctx *ctx,
                             grn_obj *output_buffer)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  grn_arrow_stream_writer *arrow_stream_writer;
  arrow_stream_writer = static_cast<grn_arrow_stream_writer *>(
    GRN_CALLOC(sizeof(grn_arrow_stream_writer)));
  arrow_stream_writer->writer = new grnarrow::StreamWriter(ctx, output_buffer);
  GRN_API_RETURN(arrow_stream_writer);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][open] Apache Arrow support isn't enabled");
  GRN_API_RETURN(NULL);
#endif
}

grn_rc
grn_arrow_stream_writer_close(grn_ctx *ctx,
                              grn_arrow_stream_writer *writer)
{
  if (!writer) {
    return ctx->rc;
  }

  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  delete writer->writer;
  GRN_FREE(writer);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][close] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_field(grn_ctx *ctx,
                                  grn_arrow_stream_writer *writer,
                                  const char *name,
                                  grn_obj *column)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_field(name, column);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-field] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_metadata(grn_ctx *ctx,
                                     grn_arrow_stream_writer *writer,
                                     const char *key,
                                     const char *value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_metadata(key, value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-metadata] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_write_schema(grn_ctx *ctx,
                                     grn_arrow_stream_writer *writer)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->write_schema();
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][write-schema] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_open_record(grn_ctx *ctx,
                                    grn_arrow_stream_writer *writer)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->open_record();
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][open-record] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_close_record(grn_ctx *ctx,
                                     grn_arrow_stream_writer *writer)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->close_record();
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][close-record] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_string(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          const char *value,
                                          size_t value_length)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_string(value, value_length);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][string] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_int8(grn_ctx *ctx,
                                        grn_arrow_stream_writer *writer,
                                        int8_t value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_int8(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][int8] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_int32(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         int32_t value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_int32(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][int32] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_uint32(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          uint32_t value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_uint32(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][uint32] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_int64(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         int64_t value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_int64(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][int64] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_uint64(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          uint64_t value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_uint64(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][uint64] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_float32(grn_ctx *ctx,
                                           grn_arrow_stream_writer *writer,
                                           float value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_float32(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][float32] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_float(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         double value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_float(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][float] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_timestamp(grn_ctx *ctx,
                                             grn_arrow_stream_writer *writer,
                                             grn_timeval value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_timestamp(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][timestamp] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_double(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          double value)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_double(value);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][double] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_record(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          grn_obj *record)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_record(record);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][record] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_add_column_uvector(grn_ctx *ctx,
                                           grn_arrow_stream_writer *writer,
                                           grn_obj *uvector)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->add_column_uvector(uvector);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][add-column][uvector] "
      "Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_arrow_stream_writer_flush(grn_ctx *ctx,
                              grn_arrow_stream_writer *writer)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  writer->writer->flush();
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-writer][flush] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}
}
