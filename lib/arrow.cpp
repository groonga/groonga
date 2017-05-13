/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017 Brazil

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
#include <groonga.hpp>

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/ipc/api.h>

namespace grnarrow {
  class ColumnImportVisitor : public arrow::ArrayVisitor {
  public:
    ColumnImportVisitor(grn_ctx *ctx,
                        const grn_id *ids,
                        grn_obj *grn_column,
                        std::shared_ptr<arrow::Array> &arrow_array)
      : ctx_(ctx),
        ids_(ids),
        grn_column_(grn_column),
        arrow_array_(arrow_array) {
      switch (grn_column_->header.type) {
      case GRN_DB_BOOL :
        GRN_BOOL_INIT(&buffer_, 0);
      default :
        GRN_VOID_INIT(&buffer_);
        break;
      }
    }

    ~ColumnImportVisitor() {
      GRN_OBJ_FIN(ctx_, &buffer_);
    }

    arrow::Status Visit(const arrow::BooleanArray &array) {
      int64_t n_rows = array.length();
      for (int i = 0; i < n_rows; ++i) {
        auto id = ids_[i];
        GRN_BULK_REWIND(&buffer_);
        GRN_BOOL_SET(ctx_, &buffer_, array.Value(i));
        grn_obj_set_value(ctx_, grn_column_, id, &buffer_, GRN_OBJ_SET);
      }
      return arrow::Status::OK();
    }

  private:
    grn_ctx *ctx_;
    const grn_id *ids_;
    grn_obj *grn_column_;
    std::shared_ptr<arrow::Array> arrow_array_;
    grn_obj buffer_;
  };

  class Importer {
  public:
    Importer(grn_ctx *ctx, grn_obj *grn_table)
      : ctx_(ctx),
        grn_table_(grn_table),
        key_column_name_(nullptr) {
    }

    ~Importer() {
    }

    grn_rc import_table(const std::shared_ptr<arrow::Table> &arrow_table) {
      int n_columns = arrow_table->num_columns();

      if (key_column_name_.empty()) {
        grn_obj ids;
        GRN_RECORD_INIT(&ids, GRN_OBJ_VECTOR, grn_obj_id(ctx_, grn_table_));
        auto n_records = arrow_table->num_rows();
        for (int64_t i = 0; i < n_records; ++i) {
          auto id = grn_table_add(ctx_, grn_table_, NULL, 0, NULL);
          GRN_RECORD_PUT(ctx_, &ids, id);
        }
        for (int i = 0; i < n_columns; ++i) {
          int64_t offset = 0;
          auto arrow_column = arrow_table->column(i);
          auto column_name = arrow_column->name();
          auto grn_column = grn_obj_column(ctx_, grn_table_,
                                           column_name.data(),
                                           column_name.size());
          auto arrow_chunked_data = arrow_column->data();
          for (auto arrow_array : arrow_chunked_data->chunks()) {
            grn_id *sub_ids =
              reinterpret_cast<grn_id *>(GRN_BULK_HEAD(&ids)) + offset;
            ColumnImportVisitor visitor(ctx_,
                                        sub_ids,
                                        grn_column,
                                        arrow_array);
            arrow_array->Accept(&visitor);
            offset += arrow_array->length();
          }
          if (grn_obj_is_accessor(ctx_, grn_column)) {
            grn_obj_unlink(ctx_, grn_column);
          }
        }
        GRN_OBJ_FIN(ctx_, &ids);
      } else {
      }
      return ctx_->rc;
    };

    grn_rc import_record_batch(const std::shared_ptr<arrow::RecordBatch> &arrow_record_batch) {
      std::shared_ptr<arrow::Table> arrow_table;
      std::vector<std::shared_ptr<arrow::RecordBatch>> arrow_record_batches(1);
      arrow_record_batches[0] = arrow_record_batch;
      auto status =
        arrow::Table::FromRecordBatches(arrow_record_batches, &arrow_table);
      // TODO: check status
      return import_table(arrow_table);
    };

  private:
    grn_ctx *ctx_;
    grn_obj *grn_table_;
    std::string key_column_name_;
  };
}

extern "C" {
grn_rc
grn_arrow_import_from_path(grn_ctx *ctx,
                           grn_obj *table,
                           const char *path)
{
  std::shared_ptr<arrow::io::MemoryMappedFile> input;
  auto status =
    arrow::io::MemoryMappedFile::Open(path, arrow::io::FileMode::READ, &input);
  // TODO: check status
  std::shared_ptr<arrow::ipc::FileReader> reader;
  status = arrow::ipc::FileReader::Open(input, &reader);
  // TODO: check status

  grnarrow::Importer importer(ctx, table);
  int n_record_batches = reader->num_record_batches();
  for (int i = 0; i < n_record_batches; ++i) {
    std::shared_ptr<arrow::RecordBatch> record_batch;
    status = reader->GetRecordBatch(i, &record_batch);
    // TODO: check status
    importer.import_record_batch(record_batch);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }

  return ctx->rc;
}
}
