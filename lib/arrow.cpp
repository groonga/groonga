/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017 Brazil
  Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>

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
#include "grn_db.h"
#include "grn_arrow.h"

#ifdef GRN_WITH_APACHE_ARROW
#include <groonga/arrow.hpp>

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/ipc/api.h>

#include <sstream>

namespace grnarrow {
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

  grn_bool check_status(grn_ctx *ctx,
                        arrow::Status &status,
                        const char *context) {
    if (status.ok()) {
      return GRN_TRUE;
    } else {
      auto rc = status_to_rc(status);
      auto message = status.ToString();
      ERR(rc, "%s: %s", context, message.c_str());
      return GRN_FALSE;
    }
  }

  grn_bool check_status(grn_ctx *ctx,
                        arrow::Status &status,
                        std::ostream &output) {
    return check_status(ctx,
                        status,
                        static_cast<std::stringstream &>(output).str().c_str());
  }

  class RecordAddVisitor : public arrow::ArrayVisitor {
  public:
    RecordAddVisitor(grn_ctx *ctx,
                     grn_obj *grn_table,
                     std::vector<grn_id> *record_ids)
      : ctx_(ctx),
        grn_table_(grn_table),
        record_ids_(record_ids) {
    }

    ~RecordAddVisitor() {
    }

    arrow::Status Visit(const arrow::BooleanArray &array) {
      return add_records(array, sizeof(bool));
    }

    arrow::Status Visit(const arrow::Int8Array &array) {
      return add_records(array, sizeof(int8_t));
    }

    arrow::Status Visit(const arrow::UInt8Array &array) {
      return add_records(array, sizeof(uint8_t));
    }

    arrow::Status Visit(const arrow::Int16Array &array) {
      return add_records(array, sizeof(int16_t));
    }

    arrow::Status Visit(const arrow::UInt16Array &array) {
      return add_records(array, sizeof(uint16_t));
    }

    arrow::Status Visit(const arrow::Int32Array &array) {
      return add_records(array, sizeof(int32_t));
    }

    arrow::Status Visit(const arrow::UInt32Array &array) {
      return add_records(array, sizeof(uint32_t));
    }

    arrow::Status Visit(const arrow::Int64Array &array) {
      return add_records(array, sizeof(int64_t));
    }

    arrow::Status Visit(const arrow::UInt64Array &array) {
      return add_records(array, sizeof(uint64_t));
    }

    arrow::Status Visit(const arrow::HalfFloatArray &array) {
      return add_records(array, sizeof(uint16_t));
    }

    arrow::Status Visit(const arrow::FloatArray &array) {
      return add_records(array, sizeof(float));
    }

    arrow::Status Visit(const arrow::DoubleArray &array) {
      return add_records(array, sizeof(double));
    }

    arrow::Status Visit(const arrow::StringArray &array) {
      const auto n_rows = array.length();
      for (int64_t i = 0; i < n_rows; ++i) {
        const auto &key = array.GetView(i);
        const auto record_id = grn_table_add(ctx_,
                                             grn_table_,
                                             key.data(),
                                             key.size(),
                                             NULL);
        record_ids_->push_back(record_id);
      }
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::Date64Array &array) {
      return add_records(array, sizeof(int64_t));
    }

    arrow::Status Visit(const arrow::TimestampArray &array) {
      return add_records(array, sizeof(int64_t));
    }

  private:
    grn_ctx *ctx_;
    grn_obj *grn_table_;
    std::vector<grn_id> *record_ids_;

    template <typename T>
    arrow::Status add_records(const T &array, size_t key_size) {
      const auto n_rows = array.length();
      for (int64_t i = 0; i < n_rows; ++i) {
        const auto &key = array.Value(i);
        const auto record_id = grn_table_add(ctx_,
                                             grn_table_,
                                             &key,
                                             key_size,
                                             NULL);
        record_ids_->push_back(record_id);
      }
      return arrow::Status::OK();
    }
  };

  class ValueLoadVisitor : public arrow::ArrayVisitor {
  public:
    ValueLoadVisitor(grn_ctx *ctx, grn_obj *buffer, int64_t index)
      : ctx_(ctx),
        buffer_(buffer),
        index_(index) {
    }

    arrow::Status Visit(const arrow::BooleanArray &array) override {
      GRN_BOOL_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::Int8Array &array) override {
      GRN_INT8_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::UInt8Array &array) override {
      GRN_UINT8_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::Int16Array &array) override {
      GRN_INT16_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::UInt16Array &array) override {
      GRN_UINT16_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::Int32Array &array) override {
      GRN_INT32_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::UInt32Array &array) override {
      GRN_UINT32_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::Int64Array &array) override {
      GRN_INT64_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::UInt64Array &array) override {
      GRN_UINT64_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::HalfFloatArray &array) override {
      GRN_FLOAT_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::FloatArray &array) override {
      GRN_FLOAT_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::DoubleArray &array) override {
      GRN_FLOAT_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::StringArray &array) override {
      int32_t size;
      const auto data = array.GetValue(index_, &size);
      if (buffer_->header.type == GRN_VECTOR) {
        unsigned int weight = 0;
        grn_id domain = GRN_DB_SHORT_TEXT;
        grn_vector_add_element(ctx_,
                               buffer_,
                               reinterpret_cast<const char *>(data),
                               size,
                               weight,
                               domain);
      } else {
        GRN_TEXT_SET(ctx_, buffer_, data, size);
      }
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::Date64Array &array) override {
      GRN_TIME_PUT(ctx_, buffer_, array.Value(index_));
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::TimestampArray &array) override {
      const auto &arrow_timestamp_type =
        std::static_pointer_cast<arrow::TimestampType>(array.type());
      switch (arrow_timestamp_type->unit()) {
      case arrow::TimeUnit::SECOND :
        GRN_TIME_PUT(ctx_, buffer_, GRN_TIME_PACK(array.Value(index_), 0));
        break;
      case arrow::TimeUnit::MILLI :
        GRN_TIME_PUT(ctx_, buffer_, array.Value(index_) * 1000);
        break;
      case arrow::TimeUnit::MICRO :
        GRN_TIME_PUT(ctx_, buffer_, array.Value(index_));
        break;
      case arrow::TimeUnit::NANO :
        GRN_TIME_PUT(ctx_, buffer_, array.Value(index_) / 1000);
        break;
      }
      return arrow::Status::OK();
    }

    arrow::Status Visit(const arrow::ListArray &array) override {
      const auto &value_array = array.value_slice(index_);
      for (int64_t i = 0; i < value_array->length(); ++i) {
        ValueLoadVisitor sub_visitor(ctx_, buffer_, i);
        ARROW_RETURN_NOT_OK(value_array->Accept(&sub_visitor));
      }
      return arrow::Status::OK();
    }

  private:
    grn_ctx *ctx_;
    grn_obj *buffer_;
    int64_t index_;
  };

  class ColumnLoadVisitor : public arrow::ArrayVisitor {
  public:
    ColumnLoadVisitor(grn_ctx *ctx,
                      grn_obj *grn_table,
                      const std::shared_ptr<arrow::Field> &arrow_field,
                      const grn_id *record_ids)
      : ctx_(ctx),
        grn_table_(grn_table),
        record_ids_(record_ids) {
      const auto &column_name = arrow_field->name();
      grn_column_ = grn_obj_column(ctx_, grn_table_,
                                   column_name.data(),
                                   column_name.size());

      const auto &arrow_type = arrow_field->type();
      grn_id type_id = GRN_DB_VOID;
      grn_obj_flags flags = 0;
      detect_type(arrow_type, &type_id, &flags);
      if (type_id == GRN_DB_VOID) {
        // TODO
        return;
      }

      if (!grn_column_) {
        grn_column_ = grn_column_create(ctx_,
                                        grn_table_,
                                        column_name.data(),
                                        column_name.size(),
                                        NULL,
                                        GRN_OBJ_COLUMN_SCALAR,
                                        grn_ctx_at(ctx_, type_id));
      }
      if (type_id == GRN_DB_TEXT) {
        GRN_TEXT_INIT(&buffer_, flags);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&buffer_, flags, type_id);
      }
    }

    ~ColumnLoadVisitor() {
      if (grn_obj_is_accessor(ctx_, grn_column_)) {
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
    grn_obj *grn_table_;
    const grn_id *record_ids_;
    grn_obj *grn_column_;
    grn_obj buffer_;

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
      case arrow::Type::DOUBLE :
        *type_id = GRN_DB_FLOAT;
        break;
      case arrow::Type::STRING :
        *type_id = GRN_DB_TEXT;
        *flags |= GRN_OBJ_DO_SHALLOW_COPY;
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
        }
        break;
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
        GRN_BULK_REWIND(&buffer_);
        ValueLoadVisitor visitor(ctx_, &buffer_, i);
        ARROW_RETURN_NOT_OK(array.Accept(&visitor));
        grn_obj_set_value(ctx_, grn_column_, record_id, &buffer_, GRN_OBJ_SET);
      }
      return arrow::Status::OK();
    }
  };

  class FileLoader {
  public:
    FileLoader(grn_ctx *ctx, grn_obj *grn_table)
      : ctx_(ctx),
        grn_table_(grn_table),
        key_column_name_("") {
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
#if ARROW_VERSION_MAJOR == 0 && ARROW_VERSION_MINOR <= 14
          const auto& arrow_column = arrow_table->column(i);
          const auto& arrow_chunked_array = arrow_column->data();
#else
          const auto& arrow_chunked_array = arrow_table->column(i);
#endif
          for (const auto& arrow_array : arrow_chunked_array->chunks()) {
            grn_id *sub_ids =
              reinterpret_cast<grn_id *>(GRN_BULK_HEAD(&ids)) + offset;
            ColumnLoadVisitor visitor(ctx_,
                                      grn_table_,
                                      arrow_field,
                                      sub_ids);
            arrow_array->Accept(&visitor);
            offset += arrow_array->length();
          }
        }
        GRN_OBJ_FIN(ctx_, &ids);
      } else {
        auto status = arrow::Status::NotImplemented("_key isn't supported yet");
        check_status(ctx_, status, "[arrow][load]");
      }
      return ctx_->rc;
    };

    grn_rc load_record_batch(const std::shared_ptr<arrow::RecordBatch> &arrow_record_batch) {
      std::shared_ptr<arrow::Table> arrow_table;
      std::vector<std::shared_ptr<arrow::RecordBatch>> arrow_record_batches(1);
      arrow_record_batches[0] = arrow_record_batch;
      auto status =
        arrow::Table::FromRecordBatches(arrow_record_batches, &arrow_table);
      if (!check_status(ctx_,
                        status,
                        "[arrow][load] "
                        "failed to convert record batch to table")) {
        return ctx_->rc;
      }
      return load_table(arrow_table);
    };

  private:
    grn_ctx *ctx_;
    grn_obj *grn_table_;
    std::string key_column_name_;
  };

  class FileDumper {
  public:
    FileDumper(grn_ctx *ctx, grn_obj *grn_table, grn_obj *grn_columns)
      : ctx_(ctx),
        grn_table_(grn_table),
        grn_columns_(grn_columns) {
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
        std::shared_ptr<arrow::DataType> field_type;
        switch (grn_obj_get_range(ctx_, column)) {
        case GRN_DB_BOOL :
          field_type = arrow::boolean();
          break;
        case GRN_DB_UINT8 :
          field_type = arrow::uint8();
          break;
        case GRN_DB_INT8 :
          field_type = arrow::int8();
          break;
        case GRN_DB_UINT16 :
          field_type = arrow::uint16();
          break;
        case GRN_DB_INT16 :
          field_type = arrow::int16();
          break;
        case GRN_DB_UINT32 :
          field_type = arrow::uint32();
          break;
        case GRN_DB_INT32 :
          field_type = arrow::int32();
          break;
        case GRN_DB_UINT64 :
          field_type = arrow::uint64();
          break;
        case GRN_DB_INT64 :
          field_type = arrow::int64();
          break;
        case GRN_DB_FLOAT :
          field_type = arrow::float64();
          break;
        case GRN_DB_TIME :
          field_type =
            std::make_shared<arrow::TimestampType>(arrow::TimeUnit::MICRO);
          break;
        case GRN_DB_SHORT_TEXT :
        case GRN_DB_TEXT :
        case GRN_DB_LONG_TEXT :
          field_type = arrow::utf8();
          break;
        default :
          break;
        }
        if (!field_type) {
          continue;
        }

        auto field = std::make_shared<arrow::Field>(field_name,
                                                    field_type,
                                                    false);
        fields.push_back(field);
      };

      auto schema = std::make_shared<arrow::Schema>(fields);

      std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
      auto status =
        arrow::ipc::RecordBatchFileWriter::Open(output, schema, &writer);
      if (!check_status(ctx_,
                        status,
                        "[arrow][dump] failed to create file format writer")) {
        return ctx_->rc;
      }

      std::vector<grn_id> ids;
      size_t n_records_per_batch = 1000;
      GRN_TABLE_EACH_BEGIN(ctx_, grn_table_, table_cursor, record_id) {
        ids.push_back(record_id);
        if (ids.size() == n_records_per_batch) {
          write_record_batch(ids, schema, writer);
          ids.clear();
        }
      } GRN_TABLE_EACH_END(ctx_, table_cursor);
      if (!ids.empty()) {
        write_record_batch(ids, schema, writer);
      }
      writer->Close();

      return ctx_->rc;
    }

  private:
    grn_ctx *ctx_;
    grn_obj *grn_table_;
    grn_obj *grn_columns_;

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
      writer->WriteRecordBatch(*record_batch);
    }

    arrow::Status build_boolean_array(std::vector<grn_id> &ids,
                                      grn_obj *grn_column,
                                      std::shared_ptr<arrow::Array> *array) {
      arrow::BooleanBuilder builder(arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        builder.Append(*(reinterpret_cast<const grn_bool *>(data)));
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
        builder.Append(*(reinterpret_cast<const uint8_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const int8_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const uint16_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const int16_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const uint32_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const int32_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const uint64_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const int64_t *>(data)));
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
        builder.Append(*(reinterpret_cast<const double *>(data)));
      }
      return builder.Finish(array);
    }

    arrow::Status build_timestamp_array(std::vector<grn_id> &ids,
                                        grn_obj *grn_column,
                                        std::shared_ptr<arrow::Array> *array) {
      auto timestamp_ns_data_type =
        std::make_shared<arrow::TimestampType>(arrow::TimeUnit::MICRO);
      arrow::TimestampBuilder builder(timestamp_ns_data_type,
                                      arrow::default_memory_pool());
      for (auto id : ids) {
        uint32_t size;
        auto data = grn_obj_get_value_(ctx_, grn_column, id, &size);
        auto timestamp_ns = *(reinterpret_cast<const int64_t *>(data));
        builder.Append(timestamp_ns);
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
        builder.Append(data, size);
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

    arrow::Status Tell(int64_t* position) const override {
      *position = offset_;
      return arrow::Status::OK();
    }

    arrow::Status Seek(int64_t position) override {
      offset_ = position;
      return arrow::Status::OK();
    }

    bool closed() const override {
      return closed_;
    }

    arrow::Status Peek(int64_t nbytes, arrow::util::string_view* out) override {
      const int64_t bytes_available =
        std::min(nbytes,
                 static_cast<int64_t>(buffer_.size() - offset_));
      *out = arrow::util::string_view(buffer_.data() + offset_,
                                      static_cast<size_t>(bytes_available));
      return arrow::Status::OK();
    }

    arrow::Status Read(int64_t nbytes, int64_t* bytes_read, void* out) override {
      const int64_t bytes_available =
        std::min(nbytes,
                 static_cast<int64_t>(buffer_.size() - offset_));
      if (bytes_available > 0) {
        *bytes_read = bytes_available;
        memcpy(out, buffer_.data() + offset_, bytes_available);
        offset_ += bytes_available;
      } else {
        *bytes_read = 0;
      }
      return arrow::Status::OK();
    }

    arrow::Status Read(int64_t nbytes,
                       std::shared_ptr<arrow::Buffer>* out) override {
      const int64_t bytes_available =
        std::min(nbytes,
                 static_cast<int64_t>(buffer_.size() - offset_));
      *out = std::make_shared<arrow::Buffer>(
        reinterpret_cast<const uint8_t *>(buffer_.data() + offset_),
        bytes_available);
      offset_ += bytes_available;
      return arrow::Status::OK();
    }

    bool supports_zero_copy() const override {
      return false;
    }

  private:
    std::string buffer_;
    int64_t offset_;
    bool closed_;
  };

  class StreamLoader {
  public:
    StreamLoader(grn_ctx *ctx, grn_loader *loader)
      : ctx_(ctx),
        grn_loader_(loader),
        input_(),
        reader_(nullptr) {
    }

    ~StreamLoader() {
    }

    grn_rc feed(const char *data, size_t data_size) {
      if (data_size == 0) {
        return GRN_SUCCESS;
      }

      input_.feed(data, data_size);
      if (!reader_) {
        const auto status = arrow::ipc::RecordBatchStreamReader::Open(&input_,
                                                                      &reader_);
        if (!status.ok()) {
          return status_to_rc(status);
        }
      }

      int64_t position;
      input_.Tell(&position);
      const auto &stream_reader =
        std::static_pointer_cast<arrow::ipc::RecordBatchStreamReader>(reader_);
      std::shared_ptr<arrow::RecordBatch> record_batch;
      {
        const auto status = stream_reader->ReadNext(&record_batch);
        if (!status.ok()) {
          input_.Seek(position);
          return ctx_->rc;
        }
      }

      auto grn_table = grn_loader_->table;
      const auto &key_column = record_batch->GetColumnByName("_key");
      const auto n_records = record_batch->num_rows();
      std::vector<grn_id> record_ids;
      if (key_column) {
        RecordAddVisitor visitor(ctx_,
                                 grn_table,
                                 &record_ids);
        key_column->Accept(&visitor);
      } else {
        for (int64_t i = 0; i < n_records; ++i) {
          const auto record_id = grn_table_add(ctx_, grn_table, NULL, 0, NULL);
          record_ids.push_back(record_id);
        }
      }
      grn_loader_->n_records += record_ids.size();

      const auto &schema = record_batch->schema();
      const auto n_columns = record_batch->num_columns();
      for (int i = 0; i < n_columns; ++i) {
        const auto &column = record_batch->column(i);
        if (column == key_column) {
          continue;
        }
        ColumnLoadVisitor visitor(ctx_,
                                  grn_table,
                                  schema->field(i),
                                  record_ids.data());
        column->Accept(&visitor);
      }
      return ctx_->rc;
    };

  private:
    grn_ctx *ctx_;
    grn_loader *grn_loader_;
    BufferInputStream input_;
    std::shared_ptr<arrow::ipc::RecordBatchReader> reader_;
  };
}
#endif /* GRN_WITH_APACHE_ARROW */

extern "C" {
grn_rc
grn_arrow_load(grn_ctx *ctx,
               grn_obj *table,
               const char *path)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  std::shared_ptr<arrow::io::MemoryMappedFile> input;
  auto status =
    arrow::io::MemoryMappedFile::Open(path, arrow::io::FileMode::READ, &input);
  std::ostringstream context;
  if (!grnarrow::check_status(ctx,
                              status,
                              context <<
                              "[arrow][load] failed to open path: " <<
                              "<" << path << ">")) {
    GRN_API_RETURN(ctx->rc);
  }
  std::shared_ptr<arrow::ipc::RecordBatchFileReader> reader;
  status = arrow::ipc::RecordBatchFileReader::Open(input, &reader);
  if (!grnarrow::check_status(ctx,
                              status,
                              "[arrow][load] "
                              "failed to create file format reader")) {
    GRN_API_RETURN(ctx->rc);
  }

  grnarrow::FileLoader loader(ctx, table);
  int n_record_batches = reader->num_record_batches();
  for (int i = 0; i < n_record_batches; ++i) {
    std::shared_ptr<arrow::RecordBatch> record_batch;
    status = reader->ReadRecordBatch(i, &record_batch);
    std::ostringstream context;
    if (!grnarrow::check_status(ctx,
                                status,
                                context <<
                                "[arrow][load] failed to get " <<
                                "the " << i << "-th " << "record")) {
      break;
    }
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

  grn_obj columns;
  GRN_PTR_INIT(&columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_HASH_EACH_BEGIN(ctx, all_columns, cursor, id) {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    auto column_id = static_cast<grn_id *>(key);
    auto column = grn_ctx_at(ctx, *column_id);
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
  std::shared_ptr<arrow::io::FileOutputStream> output;
  auto status = arrow::io::FileOutputStream::Open(path, &output);
  std::stringstream context;
  if (!grnarrow::check_status(ctx,
                              status,
                              context <<
                              "[arrow][dump] failed to open path: " <<
                              "<" << path << ">")) {
    GRN_API_RETURN(ctx->rc);
  }

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
  arrow_stream_loader = GRN_MALLOCN(grn_arrow_stream_loader, 1);
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
grn_arrow_stream_loader_feed(grn_ctx *ctx,
                             grn_arrow_stream_loader *loader,
                             const char *data,
                             size_t data_size)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_APACHE_ARROW
  loader->loader->feed(data, data_size);
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[arrow][stream-loader][feed] Apache Arrow support isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}
}
