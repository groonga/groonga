/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

/* TS is an acronym for "Turbo Selector". */

#ifdef GRN_WITH_TS

#include "grn_ts.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>

#include <iostream>  // for debug!

#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_output.h"
#include "grn_str.h"

// TODO: Error handling.

namespace {

enum {
  GRN_TS_MAX_DATA_TYPE = GRN_DB_WGS84_GEO_POINT,
  GRN_TS_MAX_BATCH_SIZE = 1024
};

grn_ts_data_kind grn_ts_data_type_to_kind(grn_ts_data_type data_type) {
  switch (data_type) {
    case GRN_DB_VOID: {
      return GRN_TS_VOID;
    }
    case GRN_DB_BOOL: {
      return GRN_TS_BOOL;
    }
    case GRN_DB_INT8:
    case GRN_DB_INT16:
    case GRN_DB_INT32:
    case GRN_DB_INT64:
    case GRN_DB_UINT8:
    case GRN_DB_UINT16:
    case GRN_DB_UINT32:
    case GRN_DB_UINT64: {
      return GRN_TS_INT;
    }
    case GRN_DB_FLOAT: {
      return GRN_TS_FLOAT;
    }
    case GRN_DB_TIME: {
      return GRN_TS_TIME;
    }
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      return GRN_TS_TEXT;
    }
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT: {
      return GRN_TS_GEO_POINT;
    }
    default: {
      return GRN_TS_REF;
    }
  }
}

}  // namespace

namespace grn {
namespace ts {

// -- TableCursor --

// TableCursor is a wrapper for grn_table_cursor:
// - GRN_CURSOR_PAT_KEY
// - GRN_CURSOR_DAT_KEY
// - GRN_CURSOR_HASH_KEY
// - GRN_CURSOR_NO_KEY
class TableCursor : public Cursor {
 public:
  ~TableCursor() {
    grn_table_cursor_close(ctx_, cursor_);
  }

  static grn_rc open(grn_ctx *ctx, grn_obj *cursor, Score default_score,
                     Cursor **wrapper);

  grn_rc read(Record *records, size_t size, size_t *count);

 private:
  grn_ctx *ctx_;
  grn_obj *cursor_;
  Score default_score_;

  TableCursor(grn_ctx *ctx, grn_obj *cursor, Score default_score)
    : Cursor(), ctx_(ctx), cursor_(cursor), default_score_(default_score) {}
};

grn_rc TableCursor::open(grn_ctx *ctx, grn_obj *cursor, Score default_score,
                         Cursor **wrapper) {
  if (!ctx || !cursor || !wrapper) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (cursor->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
    case GRN_CURSOR_TABLE_DAT_KEY:
    case GRN_CURSOR_TABLE_HASH_KEY:
    case GRN_CURSOR_TABLE_NO_KEY: {
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  TableCursor *new_wrapper =
    new (std::nothrow) TableCursor(ctx, cursor, default_score);
  if (!new_wrapper) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  *wrapper = new_wrapper;
  return GRN_SUCCESS;
}

grn_rc TableCursor::read(Record *records, size_t size, size_t *count) {
  if ((!records && (size != 0)) || !count) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (cursor_->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY: {
      for (size_t i = 0; i < size; ++i) {
        grn_id id = grn_pat_cursor_next(
          ctx_, reinterpret_cast<grn_pat_cursor *>(cursor_));
        if (id == GRN_ID_NIL) {
          *count = i;
          return GRN_SUCCESS;
        }
        records[i].id = id;
        records[i].score = default_score_;
      }
      break;
    }
    case GRN_CURSOR_TABLE_DAT_KEY: {
      for (size_t i = 0; i < size; ++i) {
        grn_id id = grn_dat_cursor_next(
          ctx_, reinterpret_cast<grn_dat_cursor *>(cursor_));
        if (id == GRN_ID_NIL) {
          *count = i;
          return GRN_SUCCESS;
        }
        records[i].id = id;
        records[i].score = default_score_;
      }
      break;
    }
    case GRN_CURSOR_TABLE_HASH_KEY: {
      for (size_t i = 0; i < size; ++i) {
        grn_id id = grn_hash_cursor_next(
          ctx_, reinterpret_cast<grn_hash_cursor *>(cursor_));
        if (id == GRN_ID_NIL) {
          *count = i;
          return GRN_SUCCESS;
        }
        records[i].id = id;
        records[i].score = default_score_;
      }
      break;
    }
    case GRN_CURSOR_TABLE_NO_KEY: {
      for (size_t i = 0; i < size; ++i) {
        grn_id id = grn_array_cursor_next(
          ctx_, reinterpret_cast<grn_array_cursor *>(cursor_));
        if (id == GRN_ID_NIL) {
          *count = i;
          return GRN_SUCCESS;
        }
        records[i].id = id;
        records[i].score = default_score_;
      }
      break;
    }
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
  *count = size;
  return GRN_SUCCESS;
}

// -- Cursor --

grn_rc Cursor::open_table_cursor(
  grn_ctx *ctx, grn_obj *table, Cursor **cursor) {
  if (!ctx || !grn_obj_is_table(ctx, table) || !cursor) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_table_cursor *table_cursor = grn_table_cursor_open(
    ctx, table, NULL, 0, NULL, 0, 0, -1,
    GRN_CURSOR_ASCENDING | GRN_CURSOR_BY_ID);
  if (!table_cursor) {
    return ctx->rc;
  }
  grn_rc rc = TableCursor::open(ctx, table_cursor, 0.0, cursor);
  if (rc != GRN_SUCCESS) {
    grn_table_cursor_close(ctx, table_cursor);
  }
  return rc;
}

grn_rc Cursor::read(Record *records, size_t size, size_t *count) {
  if ((!records && (size != 0)) || !count) {
    return GRN_INVALID_ARGUMENT;
  }
  *count = 0;
  return GRN_SUCCESS;
}

// -- ExpressionNode --

class ExpressionNode {
 public:
  ExpressionNode() {}
  virtual ~ExpressionNode() {}

  virtual ExpressionNodeType type() const = 0;
  virtual DataKind data_kind() const = 0;
  virtual DataType data_type() const = 0;
  virtual grn_obj *ref_table() const = 0;
  virtual int dimension() const = 0;

  virtual grn_rc filter(Record *input, size_t input_size,
                        Record *output, size_t *output_size) {
    return GRN_OPERATION_NOT_SUPPORTED;
  }
  virtual grn_rc adjust(Record *records, size_t num_records) {
    return GRN_OPERATION_NOT_SUPPORTED;
  }
  virtual grn_rc evaluate(const Record *records, size_t num_records,
                          void *results) = 0;
};

// -- IDNode --

class IDNode : public ExpressionNode {
 public:
  ~IDNode() {}

  static grn_rc open(ExpressionNode **node) {
    ExpressionNode *new_node = new (std::nothrow) IDNode;
    if (!new_node) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    *node = new_node;
    return GRN_SUCCESS;
  }

  ExpressionNodeType type() const {
    return GRN_TS_ID_NODE;
  }
  DataKind data_kind() const {
    return GRN_TS_INT;
  }
  DataType data_type() const {
    return GRN_DB_UINT32;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return 0;
  }

  grn_rc evaluate(const Record *records, size_t num_records, void *results) {
    for (size_t i = 0; i < num_records; ++i) {
      static_cast<grn_ts_int *>(results)[i] = records[i].id;
    }
    return GRN_SUCCESS;
  }

 private:
  IDNode() : ExpressionNode() {}
};

// -- ScoreNode --

class ScoreNode : public ExpressionNode {
 public:
  ~ScoreNode() {}

  static grn_rc open(ExpressionNode **node) {
    ExpressionNode *new_node = new (std::nothrow) ScoreNode;
    if (!new_node) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    *node = new_node;
    return GRN_SUCCESS;
  }

  ExpressionNodeType type() const {
    return GRN_TS_SCORE_NODE;
  }
  DataKind data_kind() const {
    return GRN_TS_FLOAT;
  }
  DataType data_type() const {
    return GRN_DB_FLOAT;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return 0;
  }

  grn_rc adjust(Record *records, size_t num_records) {
    return GRN_SUCCESS;
  }
  grn_rc evaluate(
    const Record *records, size_t num_records, void *results) {
    for (size_t i = 0; i < num_records; ++i) {
      static_cast<grn_ts_float *>(results)[i] = records[i].score;
    }
    return GRN_SUCCESS;
  }

 private:
  ScoreNode() : ExpressionNode() {}
};

// -- ConstantNode --

class ConstantNode : public ExpressionNode {
 public:
  ~ConstantNode() {
    if (obj_) {
      grn_obj_unlink(ctx_, obj_);
    }
    if (buf_) {
      grn_obj_unlink(ctx_, buf_);
    }
  }

  static grn_rc open(grn_ctx *ctx, grn_obj *obj, ExpressionNode **node);

  ExpressionNodeType type() const {
    return GRN_TS_CONSTANT_NODE;
  }
  DataKind data_kind() const {
    return data_kind_;
  }
  DataType data_type() const {
    return data_type_;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return dimension_;
  }

  grn_rc filter(Record *input, size_t input_size,
                Record *output, size_t *output_size);
  grn_rc adjust(Record *records, size_t num_records);
  grn_rc evaluate(const Record *records, size_t num_records, void *results);

 private:
  grn_ctx *ctx_;
  grn_obj *obj_;
  grn_obj *buf_;
  DataKind data_kind_;
  DataType data_type_;
  int dimension_;

  static grn_rc convert(grn_ctx *ctx, grn_obj *obj,
                        grn_obj **new_obj, grn_obj **buf);
  static grn_rc convert_bulk(grn_ctx *ctx, grn_obj *obj,
                             grn_obj **new_obj, grn_obj **buf);
  static grn_rc convert_uvector(grn_ctx *ctx, grn_obj *obj,
                                grn_obj **new_obj, grn_obj **buf);
  static grn_rc convert_vector(grn_ctx *ctx, grn_obj *obj,
                               grn_obj **new_obj, grn_obj **buf);

  ConstantNode(grn_ctx *ctx, grn_obj *obj, grn_obj *buf,
               DataType data_type, int dimension)
    : ExpressionNode(),
      ctx_(ctx),
      obj_(obj),
      buf_(buf),
      data_kind_(grn_ts_data_type_to_kind(data_type)),
      data_type_(data_type),
      dimension_(dimension) {}
};

grn_rc ConstantNode::open(grn_ctx *ctx, grn_obj *obj, ExpressionNode **node) {
  grn_obj *new_obj;
  grn_obj *buf;
  grn_rc rc = convert(ctx, obj, &new_obj, &buf);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  DataType data_type = obj->header.domain;
  int dimension = obj->header.type != GRN_BULK;
  ConstantNode *new_node = new (std::nothrow) ConstantNode(
    ctx, new_obj ? new_obj : obj, buf, data_type, dimension);
  if (!new_node) {
    if (new_obj) {
      grn_obj_close(ctx, new_obj);
    }
    if (buf) {
      grn_obj_close(ctx, buf);
    }
    return GRN_NO_MEMORY_AVAILABLE;
  }
  if (new_obj) {
    grn_obj_unlink(ctx, obj);
  }
  *node = new_node;
  return GRN_SUCCESS;
}

grn_rc ConstantNode::filter(Record *input, size_t input_size,
                            Record *output, size_t *output_size) {
  if ((dimension() != 0) || (data_kind() != GRN_TS_BOOL)) {
    return GRN_OPERATION_NOT_PERMITTED;
  }
  grn_ts_bool value = GRN_BOOL_VALUE(obj_);
  if (value) {
    // If the I/O areas are the same, there is no need to copy records.
    if (input != output) {
      for (size_t i = 0; i < input_size; ++i) {
        output[i] = input[i];
      }
    }
    *output_size = input_size;
  } else {
    *output_size = 0;
  }
  return GRN_SUCCESS;
}

grn_rc ConstantNode::adjust(Record *records, size_t num_records) {
  if ((dimension() != 0) || (data_kind() != GRN_TS_FLOAT)) {
    return GRN_OPERATION_NOT_PERMITTED;
  }
  grn_ts_float value = GRN_FLOAT_VALUE(obj_);
  for (size_t i = 0; i < num_records; ++i) {
    records[i].score = value;
  }
  return GRN_SUCCESS;
}

grn_rc ConstantNode::evaluate(const Record *records, size_t num_records,
                              void *results) {
  if (num_records == 0) {
    return GRN_SUCCESS;
  }
  if (dimension() == 0) {
    // Scalar types.
    switch (data_kind()) {
      case GRN_TS_BOOL: {
        grn_ts_bool value = GRN_BOOL_VALUE(obj_);
        for (size_t i = 0; i < num_records; ++i) {
          static_cast<grn_ts_bool *>(results)[i] = value;
        }
        break;
      }
      case GRN_TS_INT:
      case GRN_TS_FLOAT:
      case GRN_TS_TIME:
      case GRN_TS_GEO_POINT: {
        const void *head = GRN_BULK_HEAD(obj_);
        char *ptr = static_cast<char *>(results);
        for (size_t i = 0; i < num_records; ++i) {
          std::memcpy(ptr, head, 8);
          ptr += 8;
        }
        break;
      }
      case GRN_TS_TEXT: {
        grn_ts_text value;
        value.ptr = GRN_BULK_HEAD(obj_);
        value.size = GRN_BULK_VSIZE(obj_);
        for (size_t i = 0; i < num_records; ++i) {
          static_cast<grn_ts_text *>(results)[i] = value;
        }
        break;
      }
      default: {
        return GRN_UNKNOWN_ERROR;
      }
    }
  } else {
    // Vector types.
    switch (data_kind()) {
      case GRN_TS_BOOL:
      case GRN_TS_INT:
      case GRN_TS_FLOAT:
      case GRN_TS_TIME:
      case GRN_TS_GEO_POINT: {
        grn_ts_vector value;
        value.ptr = GRN_BULK_HEAD(obj_);
        value.size = grn_vector_size(ctx_, obj_);
        for (size_t i = 0; i < num_records; ++i) {
          static_cast<grn_ts_vector *>(results)[i] = value;
        }
        break;
      }
      case GRN_TS_TEXT: {
        grn_ts_vector value;
        value.ptr = GRN_BULK_HEAD(obj_);
        value.size = GRN_BULK_VSIZE(obj_) / sizeof(grn_ts_text);
        for (size_t i = 0; i < num_records; ++i) {
          static_cast<grn_ts_vector *>(results)[i] = value;
        }
        break;
      }
      default: {
        return GRN_UNKNOWN_ERROR;
      }
    }
  }
  return GRN_SUCCESS;
}

grn_rc ConstantNode::convert(grn_ctx *ctx, grn_obj *obj,
                             grn_obj **new_obj, grn_obj **buf) {
  *new_obj = NULL;
  *buf = NULL;
  switch (obj->header.type) {
    case GRN_BULK: {
      return convert_bulk(ctx, obj, new_obj, buf);
    }
    case GRN_UVECTOR: {
      return convert_uvector(ctx, obj, new_obj, buf);
    }
    case GRN_VECTOR: {
      return convert_vector(ctx, obj, new_obj, buf);
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

#define GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(type)\
  case GRN_DB_ ## type: {\
    GRN_INT64_SET(ctx, *new_obj, GRN_ ## type ## _VALUE(obj));\
    break;\
  }
grn_rc ConstantNode::convert_bulk(grn_ctx *ctx, grn_obj *obj,
                                  grn_obj **new_obj, grn_obj **buf) {
  switch (obj->header.domain) {
    case GRN_DB_BOOL:
    case GRN_DB_INT64:
    case GRN_DB_FLOAT:
    case GRN_DB_TIME:
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT:
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT: {
      return GRN_SUCCESS;
    }
    case GRN_DB_INT8:
    case GRN_DB_INT16:
    case GRN_DB_INT32:
    case GRN_DB_UINT8:
    case GRN_DB_UINT16:
    case GRN_DB_UINT32:
    case GRN_DB_UINT64: {
      *new_obj = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_INT64);
      if (!*new_obj) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      switch (obj->header.domain) {
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(INT8)
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(INT16)
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(INT32)
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(UINT8)
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(UINT16)
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(UINT32)
        GRN_TS_CONVERT_BULK_INT_CASE_BLOCK(UINT64)
        default: {
          return GRN_UNKNOWN_ERROR;
        }
      }
      return GRN_SUCCESS;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_CONVERT_BULK_INT_CASE_BLOCK

#define GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(type)\
  case GRN_DB_ ## type: {\
    for (size_t i = 0; i < size; ++i) {\
      GRN_INT64_SET_AT(ctx, *new_obj, i, GRN_ ## type ## _VALUE_AT(obj, i));\
    }\
    break;\
  }
grn_rc ConstantNode::convert_uvector(grn_ctx *ctx, grn_obj *obj,
                                     grn_obj **new_obj, grn_obj **buf) {
  switch (obj->header.domain) {
    case GRN_DB_BOOL:
    case GRN_DB_INT64:
    case GRN_DB_FLOAT:
    case GRN_DB_TIME:
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT: {
      return GRN_SUCCESS;
    }
    case GRN_DB_INT8:
    case GRN_DB_INT16:
    case GRN_DB_INT32:
    case GRN_DB_UINT8:
    case GRN_DB_UINT16:
    case GRN_DB_UINT32:
    case GRN_DB_UINT64: {
      *new_obj = grn_obj_open(ctx, GRN_UVECTOR, 0, GRN_DB_INT64);
      if (!new_obj) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      size_t size = grn_vector_size(ctx, obj);
      switch (obj->header.domain) {
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(INT8)
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(INT16)
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(INT32)
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(UINT8)
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(UINT16)
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(UINT32)
        GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK(UINT64)
        default: {
          return GRN_UNKNOWN_ERROR;
        }
      }
      return GRN_SUCCESS;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_CONVERT_UVECTOR_INT_CASE_BLOCK

grn_rc ConstantNode::convert_vector(grn_ctx *ctx, grn_obj *obj,
                                    grn_obj **new_obj, grn_obj **buf) {
  switch (obj->header.domain) {
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  grn_rc rc = GRN_SUCCESS;
  *new_obj = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  if (!*new_obj) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  size_t size = grn_vector_size(ctx, obj);
  rc = grn_bulk_resize(ctx, *new_obj, sizeof(grn_ts_text) * size);
  if (rc == GRN_SUCCESS) {
    *buf = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
    if (!*buf) {
      grn_obj_close(ctx, *new_obj);
      return GRN_NO_MEMORY_AVAILABLE;
    }
    grn_ts_text *values =
      reinterpret_cast<grn_ts_text *>(GRN_BULK_HEAD(*new_obj));
    for (size_t i = 0; i < size; ++i) {
      const char *ptr;
      values[i].size = grn_vector_get_element(ctx, obj, i, &ptr, NULL, NULL);
      GRN_TEXT_PUT(ctx, *buf, ptr, values[i].size);
      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    if (rc == GRN_SUCCESS) {
      const char *ptr = GRN_BULK_HEAD(*buf);
      for (size_t i = 0; i < size; ++i) {
        values[i].ptr = ptr;
        ptr += values[i].size;
      }
    }
  }
  if (rc != GRN_SUCCESS) {
    if (*new_obj) {
      grn_obj_close(ctx, *new_obj);
    }
    if (*buf) {
      grn_obj_close(ctx, *buf);
    }
  }
  return GRN_SUCCESS;
}

// -- ColumnNode --

class ColumnNode : public ExpressionNode {
 public:
  ~ColumnNode() {
    if (column_) {
      grn_obj_unlink(ctx_, column_);
    }
    if (ref_table_) {
      grn_obj_unlink(ctx_, ref_table_);
    }
    if (buf_) {
      grn_obj_close(ctx_, buf_);
    }
    if (deep_buf_) {
      grn_obj_close(ctx_, deep_buf_);
    }
  }

  static grn_rc open(grn_ctx *ctx, grn_obj *column, ExpressionNode **node);

  ExpressionNodeType type() const {
    return GRN_TS_COLUMN_NODE;
  }
  DataKind data_kind() const {
    return data_kind_;
  }
  DataType data_type() const {
    return data_type_;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return dimension_;
  }

  grn_rc filter(Record *input, size_t input_size,
                Record *output, size_t *output_size);
  grn_rc adjust(Record *records, size_t num_records);
  grn_rc evaluate(const Record *records, size_t num_records, void *results);

 private:
  grn_ctx *ctx_;
  grn_obj *column_;
  grn_obj *buf_;
  grn_obj *deep_buf_;
  DataKind data_kind_;
  DataType data_type_;
  grn_obj *ref_table_;
  int dimension_;

  ColumnNode(grn_ctx *ctx, grn_obj *column, DataType data_type,
             grn_obj *ref_table, int dimension)
    : ExpressionNode(),
      ctx_(ctx),
      column_(column),
      buf_(NULL),
      deep_buf_(NULL),
      data_kind_(grn_ts_data_type_to_kind(data_type)),
      data_type_(data_type),
      ref_table_(ref_table),
      dimension_(dimension) {}

  grn_rc evaluate_scalar(const Record *records, size_t num_records,
                         void *results);
  grn_rc evaluate_scalar_text(const Record *records, size_t num_records,
                              void *results);
  grn_rc evaluate_vector(const Record *records, size_t num_records,
                         void *results);
  grn_rc evaluate_vector_int(const Record *records, size_t num_records,
                             void *results);
  grn_rc evaluate_vector_text(const Record *records, size_t num_records,
                              void *results);
};

grn_rc ColumnNode::open(grn_ctx *ctx, grn_obj *column, ExpressionNode **node) {
  DataType data_type = GRN_DB_VOID;
  grn_obj *ref_table = NULL;
  int dimension = 0;
  switch (column->header.type) {
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE: {
      data_type = DB_OBJ(column)->range;
      if (column->header.type == GRN_COLUMN_VAR_SIZE) {
        grn_obj_flags column_type =
          column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK;
        if (column_type == GRN_OBJ_COLUMN_VECTOR) {
          dimension = 1;
        }
      }
      break;
    }
    case GRN_ACCESSOR: {
      grn_accessor *accessor = (grn_accessor *)column;
      switch (accessor->action) {
        case GRN_ACCESSOR_GET_ID: {
          return IDNode::open(node);
        }
        case GRN_ACCESSOR_GET_KEY: {
          data_type = accessor->obj->header.domain;
          break;
        }
        case GRN_ACCESSOR_GET_VALUE: {
          data_type = DB_OBJ(accessor->obj)->range;
          break;
        }
        case GRN_ACCESSOR_GET_SCORE: {
          return IDNode::open(node);
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  if (data_type > GRN_TS_MAX_DATA_TYPE) {
    ref_table = grn_ctx_at(ctx, column->header.domain);
    if (!ref_table) {
      if (ctx->rc != GRN_SUCCESS) {
        return ctx->rc;
      }
      return GRN_UNKNOWN_ERROR;
    } else if (!grn_obj_is_table(ctx, ref_table)) {
      grn_obj_unlink(ctx, ref_table);
      return GRN_UNKNOWN_ERROR;
    }
  }
  ColumnNode *new_node = new (std::nothrow) ColumnNode(
    ctx, column, data_type, ref_table, dimension);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  *node = new_node;
  return GRN_SUCCESS;
}

grn_rc ColumnNode::filter(Record *input, size_t input_size,
                          Record *output, size_t *output_size) {
  if ((dimension() != 0) || (data_kind() != GRN_TS_BOOL)) {
    return GRN_OPERATION_NOT_PERMITTED;
  }
  grn_obj value;
  GRN_BOOL_INIT(&value, 0);
  size_t count = 0;
  for (size_t i = 0; i < input_size; ++i) {
    GRN_BULK_REWIND(&value);
    grn_obj_get_value(ctx_, column_, input[i].id, &value);
    if (ctx_->rc != GRN_SUCCESS) {
      return ctx_->rc;
    }
    if (GRN_BOOL_VALUE(&value)) {
      output[count] = input[i];
      ++count;
    }
  }
  GRN_OBJ_FIN(ctx_, &value);
  *output_size = count;
  return GRN_SUCCESS;
}

grn_rc ColumnNode::adjust(Record *records, size_t num_records) {
  if ((dimension() != 0) || (data_kind() != GRN_TS_FLOAT)) {
    return GRN_OPERATION_NOT_PERMITTED;
  }
  grn_obj value;
  GRN_FLOAT_INIT(&value, 0);
  for (size_t i = 0; i < num_records; ++i) {
    GRN_BULK_REWIND(&value);
    grn_obj_get_value(ctx_, column_, records[i].id, &value);
    records[i].score = GRN_FLOAT_VALUE(&value);
  }
  GRN_OBJ_FIN(ctx_, &value);
  return GRN_SUCCESS;
}

grn_rc ColumnNode::evaluate(const Record *records, size_t num_records,
                            void *results) {
  if (num_records == 0) {
    return GRN_SUCCESS;
  }
  if (dimension() == 0) {
    return evaluate_scalar(records, num_records, results);
  } else {
    return evaluate_vector(records, num_records, results);
  }
}

#define GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(type, ts_type)\
  case GRN_DB_ ## type: {\
    GRN_ ## type ## _INIT(&value, 0);\
    for (size_t i = 0; i < num_records; ++i) {\
      GRN_BULK_REWIND(&value);\
      grn_obj_get_value(ctx_, column_, records[i].id, &value);\
      if (ctx_->rc != GRN_SUCCESS) {\
        break;\
      }\
      static_cast<grn_ts_ ## ts_type *>(results)[i] =\
        GRN_ ## type ## _VALUE(&value);\
    }\
    break;\
  }
grn_rc ColumnNode::evaluate_scalar(const Record *records, size_t num_records,
                                   void *results) {
  grn_obj value;
  switch (data_type()) {
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(BOOL, bool)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(INT8, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(INT16, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(INT32, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(INT64, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(UINT8, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(UINT16, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(UINT32, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(UINT64, int)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(FLOAT, float)
    GRN_TS_EVALUATE_SCALAR_CASE_BLOCK(TIME, time)
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      return evaluate_scalar_text(records, num_records, results);
    }
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT: {
      if (data_type() == GRN_DB_TOKYO_GEO_POINT) {
        GRN_TOKYO_GEO_POINT_INIT(&value, 0);
      } else {
        GRN_WGS84_GEO_POINT_INIT(&value, 0);
      }
      for (size_t i = 0; i < num_records; ++i) {
        GRN_BULK_REWIND(&value);
        grn_obj_get_value(ctx_, column_, records[i].id, &value);
        if (ctx_->rc != GRN_SUCCESS) {
          break;
        }
        grn_ts_geo_point *ptr = &static_cast<grn_ts_geo_point *>(results)[i];
        GRN_GEO_POINT_VALUE(&value, ptr->latitude, ptr->longitude);
      }
      break;
    }
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
  GRN_OBJ_FIN(ctx_, &value);
  if (ctx_->rc != GRN_SUCCESS) {
    return ctx_->rc;
  }
  return GRN_SUCCESS;
}
#undef GRN_TS_EVALUATE_CASE_BLOCK

grn_rc ColumnNode::evaluate_scalar_text(const Record *records,
                                        size_t num_records, void *results) {
  if (!buf_) {
    buf_ = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
    if (!buf_) {
      if (ctx_->rc != GRN_SUCCESS) {
        return ctx_->rc;
      }
      return GRN_UNKNOWN_ERROR;
    }
  }
  GRN_BULK_REWIND(buf_);
  grn_ts_text *values = static_cast<grn_ts_text *>(results);
  size_t offset = 0;
  for (size_t i = 0; i < num_records; ++i) {
    grn_obj_get_value(ctx_, column_, records[i].id, buf_);
    if (ctx_->rc != GRN_SUCCESS) {
      return ctx_->rc;
    }
    size_t size = GRN_TEXT_LEN(buf_);
    values[i].size = size - offset;
    offset = size;
  }
  const char *ptr = GRN_TEXT_VALUE(buf_);
  for (size_t i = 0; i < num_records; ++i) {
    values[i].ptr = ptr;
    ptr += values[i].size;
  }
  return GRN_SUCCESS;
}

#define GRN_TS_EVALUATE_VECTOR_CASE_BLOCK(type, ts_type)\
  case GRN_DB_ ## type: {\
    if (!buf_) {\
      buf_ = grn_obj_open(ctx_, GRN_UVECTOR, 0, GRN_DB_ ## type);\
      if (!buf_) {\
        if (ctx_->rc != GRN_SUCCESS) {\
          return ctx_->rc;\
        }\
        return GRN_UNKNOWN_ERROR;\
      }\
    }\
    GRN_BULK_REWIND(buf_);\
    grn_ts_vector *vectors = static_cast<grn_ts_vector *>(results);\
    size_t offset = 0;\
    for (size_t i = 0; i < num_records; i++) {\
      grn_obj_get_value(ctx_, column_, records[i].id, buf_);\
      if (ctx_->rc != GRN_SUCCESS) {\
        return ctx_->rc;\
      }\
      size_t size = grn_vector_size(ctx_, buf_);\
      vectors[i].size = size - offset;\
      offset = size;\
    }\
    grn_ts_ ## ts_type *ptr =\
      reinterpret_cast<grn_ts_ ## ts_type *>(GRN_BULK_HEAD(buf_));\
    for (size_t i = 0; i < num_records; i++) {\
      vectors[i].ptr = ptr;\
      ptr += vectors[i].size;\
    }\
    return GRN_SUCCESS;\
  }
grn_rc ColumnNode::evaluate_vector(const Record *records, size_t num_records,
                                   void *results) {
  switch (data_type()) {
    case GRN_DB_INT8:
    case GRN_DB_INT16:
    case GRN_DB_INT32:
    case GRN_DB_UINT8:
    case GRN_DB_UINT16:
    case GRN_DB_UINT32: {
      return evaluate_vector_int(records, num_records, results);
    }
    GRN_TS_EVALUATE_VECTOR_CASE_BLOCK(BOOL, bool)
    GRN_TS_EVALUATE_VECTOR_CASE_BLOCK(INT64, int)
    GRN_TS_EVALUATE_VECTOR_CASE_BLOCK(UINT64, int)
    GRN_TS_EVALUATE_VECTOR_CASE_BLOCK(FLOAT, float)
    GRN_TS_EVALUATE_VECTOR_CASE_BLOCK(TIME, float)
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      return evaluate_vector_text(records, num_records, results);
    }
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT: {
      if (!buf_) {
        buf_ = grn_obj_open(ctx_, GRN_UVECTOR, 0, data_type());
        if (!buf_) {
          if (ctx_->rc != GRN_SUCCESS) {
            return ctx_->rc;
          }
          return GRN_UNKNOWN_ERROR;
        }
      }
      GRN_BULK_REWIND(buf_);
      grn_ts_vector *vectors = static_cast<grn_ts_vector *>(results);
      size_t offset = 0;
      for (size_t i = 0; i < num_records; i++) {
        grn_obj_get_value(ctx_, column_, records[i].id, buf_);
        if (ctx_->rc != GRN_SUCCESS) {
          return ctx_->rc;
        }
        size_t size = grn_vector_size(ctx_, buf_);
        vectors[i].size = size - offset;
        offset = size;
      }
      grn_ts_geo_point *ptr =
        reinterpret_cast<grn_ts_geo_point *>(GRN_BULK_HEAD(buf_));
      for (size_t i = 0; i < num_records; i++) {
        vectors[i].ptr = ptr;
        ptr += vectors[i].size;
      }
      return GRN_SUCCESS;
    }
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
}

#define GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(type)\
  case GRN_DB_ ## type: {\
    if (!deep_buf_) {\
      deep_buf_ = grn_obj_open(ctx_, GRN_UVECTOR, 0, GRN_DB_ ## type);\
      if (!deep_buf_) {\
        if (ctx_->rc != GRN_SUCCESS) {\
          return ctx_->rc;\
        }\
        return GRN_UNKNOWN_ERROR;\
      }\
    }\
    GRN_BULK_REWIND(deep_buf_);\
    grn_ts_vector *vectors = static_cast<grn_ts_vector *>(results);\
    size_t offset = 0;\
    for (size_t i = 0; i < num_records; i++) {\
      grn_obj_get_value(ctx_, column_, records[i].id, deep_buf_);\
      if (ctx_->rc != GRN_SUCCESS) {\
        return ctx_->rc;\
      }\
      size_t size = grn_vector_size(ctx_, deep_buf_);\
      vectors[i].size = size - offset;\
      offset = size;\
    }\
    grn_rc rc = grn_bulk_resize(ctx_, buf_, sizeof(grn_ts_int) * offset);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
    grn_ts_int *ptr = reinterpret_cast<grn_ts_int *>(GRN_BULK_HEAD(buf_));\
    for (size_t i = 0; i < offset; ++i) {\
      ptr[i] = GRN_ ## type ## _VALUE_AT(deep_buf_, i);\
    }\
    for (size_t i = 0; i < num_records; i++) {\
      vectors[i].ptr = ptr;\
      ptr += vectors[i].size;\
    }\
    return GRN_SUCCESS;\
  }
grn_rc ColumnNode::evaluate_vector_int(const Record *records,
                                       size_t num_records, void *results) {
  if (!buf_) {
    buf_ = grn_obj_open(ctx_, GRN_UVECTOR, 0, GRN_DB_INT64);
    if (!buf_) {
      if (ctx_->rc != GRN_SUCCESS) {
        return ctx_->rc;
      }
      return GRN_UNKNOWN_ERROR;
    }
  }
  GRN_BULK_REWIND(buf_);
  switch (data_type()) {
    GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(INT8)
    GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(INT16)
    GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(INT32)
    GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT8)
    GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT16)
    GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT32)
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
}
#undef GRN_TS_EVALUATE_VECTOR_INT_CASE_BLOCK

grn_rc ColumnNode::evaluate_vector_text(const Record *records,
                                        size_t num_records, void *results) {
  if (!deep_buf_) {
    deep_buf_ = grn_obj_open(ctx_, GRN_VECTOR, 0, GRN_DB_TEXT);
    if (!deep_buf_) {
      if (ctx_->rc != GRN_SUCCESS) {
        return ctx_->rc;
      }
      return GRN_UNKNOWN_ERROR;
    }
  }
  GRN_BULK_REWIND(deep_buf_);
  grn_ts_vector *vectors = static_cast<grn_ts_vector *>(results);
  size_t offset = 0;
  for (size_t i = 0; i < num_records; ++i) {
    grn_obj_get_value(ctx_, column_, records[i].id, deep_buf_);
    if (ctx_->rc != GRN_SUCCESS) {
      return ctx_->rc;
    }
    size_t size = grn_vector_size(ctx_, deep_buf_);
    vectors[i].size = size - offset;
    offset = size;
  }
  if (!buf_) {
    buf_ = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
    if (!buf_) {
      if (ctx_->rc != GRN_SUCCESS) {
        return ctx_->rc;
      }
      return GRN_UNKNOWN_ERROR;
    }
  }
  grn_rc rc = grn_bulk_resize(ctx_, buf_, sizeof(grn_ts_text) * offset);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  grn_ts_text *texts = reinterpret_cast<grn_ts_text *>(GRN_BULK_HEAD(buf_));
  offset = 0;
  for (size_t i = 0; i < num_records; ++i) {
    for (size_t j = 0; j < vectors[i].size; ++j) {
      texts[offset + j].size = grn_vector_get_element(
        ctx_, deep_buf_, offset + j, &texts[offset + j].ptr, NULL, NULL);
    }
    vectors[i].ptr = &texts[offset];
    offset += vectors[i].size;
  }
  return GRN_SUCCESS;
}

// -- OperatorNode --

class OperatorNode : public ExpressionNode {
 public:
  OperatorNode() : ExpressionNode() {}
  virtual ~OperatorNode() {}

  ExpressionNodeType type() const {
    return GRN_TS_OPERATOR_NODE;
  }
};

/*
grn_rc operator_node_fill_arg_values(const Record *records, size_t num_records,
                                     ExpressionNode *arg,
                                     std::vector<char> *arg_values) {
  size_t value_size = 0;
  switch (arg->data_kind()) {
    case GRN_TS_BOOL: {
      value_size = sizeof(grn_ts_bool);
      break;
    }
    case GRN_TS_INT: {
      value_size = sizeof(grn_ts_int);
      break;
    }
    case GRN_TS_FLOAT: {
      value_size = sizeof(grn_ts_float);
      break;
    }
    case GRN_TS_TIME: {
      value_size = sizeof(grn_ts_time);
      break;
    }
    case GRN_TS_TEXT: {
      value_size = sizeof(grn_ts_text);
      break;
    }
    case GRN_TS_GEO_POINT: {
      value_size = sizeof(grn_ts_geo_point);
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  size_t old_size = arg_values->size() / value_size;
  if (old_size < num_records) try {
    arg_values->resize(value_size * num_records);
  } catch (const std::bad_alloc &) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  return arg->evaluate(records, num_records, &*arg_values->begin());
}
*/

//// --- UnaryNode ---

//template <typename T, typename U>
//class UnaryNode : public OperatorNode<T> {
// public:
//  explicit UnaryNode(ExpressionNode *arg)
//    : OperatorNode<T>(), arg_(static_cast<TypedNode<U> *>(arg)),
//      arg_values_() {}
//  virtual ~UnaryNode() {
//    delete arg_;
//  }

// protected:
//  TypedNode<U> *arg_;
//  std::vector<U> arg_values_;

//  grn_rc fill_arg_values(const Record *records, size_t num_records) {
//    return operator_node_fill_arg_values(
//      records, num_records, arg_, &arg_values_);
//  }
//};

//// --- BinaryNode ---

//template <typename T, typename U, typename V>
//class BinaryNode : public OperatorNode<T> {
// public:
//  BinaryNode(ExpressionNode *arg1, ExpressionNode *arg2)
//    : OperatorNode<T>(),
//      arg1_(static_cast<TypedNode<U> *>(arg1)),
//      arg2_(static_cast<TypedNode<V> *>(arg2)),
//      arg1_values_(), arg2_values_() {}
//  virtual ~BinaryNode() {
//    delete arg1_;
//    delete arg2_;
//  }

// protected:
//  TypedNode<U> *arg1_;
//  TypedNode<V> *arg2_;
//  std::vector<U> arg1_values_;
//  std::vector<V> arg2_values_;

//  grn_rc fill_arg1_values(const Record *records, size_t num_records) {
//    return operator_node_fill_arg_values(
//      records, num_records, arg1_, &arg1_values_);
//  }
//  grn_rc fill_arg2_values(const Record *records, size_t num_records) {
//    return operator_node_fill_arg_values(
//      records, num_records, arg2_, &arg2_values_);
//  }
//};

// ---- LogicalNotNode ----

class LogicalNotNode : public OperatorNode {
 public:
  ~LogicalNotNode() {
    delete arg_;
  }

  static grn_rc open(ExpressionNode *arg, ExpressionNode **node) {
    LogicalNotNode *new_node = new (std::nothrow) LogicalNotNode(arg);
    if (!new_node) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    *node = new_node;
    return GRN_SUCCESS;
  }

  DataKind data_kind() const {
    return GRN_TS_BOOL;
  }
  DataType data_type() const {
    return GRN_DB_BOOL;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return 0;
  }

  grn_rc filter(Record *input, size_t input_size,
                Record *output, size_t *output_size);

  grn_rc evaluate(const Record *records, size_t num_records, void *results);

 private:
  ExpressionNode *arg_;
  std::vector<Record> temp_records_;

  explicit LogicalNotNode(ExpressionNode *arg)
    : OperatorNode(), arg_(arg), temp_records_() {}
};

grn_rc LogicalNotNode::filter(Record *input, size_t input_size,
                              Record *output, size_t *output_size) {
  if (temp_records_.size() <= input_size) {
    try {
      temp_records_.resize(input_size + 1);
      temp_records_[input_size].id = GRN_ID_NIL;
    } catch (const std::bad_alloc &) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
  }
  size_t temp_size;
  grn_rc rc =
    arg_->filter(input, input_size, &*temp_records_.begin(), &temp_size);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (temp_size == 0) {
    if (input != output) {
      for (size_t i = 0; i < input_size; ++i) {
        output[i] = input[i];
      }
    }
    *output_size = input_size;
    return GRN_SUCCESS;
  } else if (temp_size == input_size) {
    *output_size = 0;
    return GRN_SUCCESS;
  }

  size_t count = 0;
  for (size_t i = 0; i < input_size; ++i) {
    if (input[i].id != temp_records_[i - count].id) {
      output[count] = input[i];
      ++count;
    }
  }
  *output_size = count;
  return GRN_SUCCESS;
}

grn_rc LogicalNotNode::evaluate(const Record *records, size_t num_records,
                                void *results) {
  grn_rc rc = arg_->evaluate(records, num_records, results);
  if (rc == GRN_SUCCESS) {
    for (size_t i = 0; i < num_records; ++i) {
      ((grn_ts_bool *)results)[i] = !((grn_ts_bool *)results)[i];
    }
  }
  return rc;
}

// ---- LogicalAndNode ----

class LogicalAndNode : public OperatorNode {
 public:
  ~LogicalAndNode() {}

  static grn_rc open(ExpressionNode *arg1, ExpressionNode *arg2,
                     ExpressionNode **node);

  DataKind data_kind() const {
    return GRN_TS_BOOL;
  }
  DataType data_type() const {
    return GRN_DB_BOOL;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return 0;
  }

  grn_rc filter(Record *input, size_t input_size,
                Record *output, size_t *output_size);
  grn_rc evaluate(const Record *records, size_t num_records, void *results);

 private:
  ExpressionNode *arg1_;
  ExpressionNode *arg2_;
  std::vector<grn_ts_bool> arg_values_;
  std::vector<Record> temp_records_;

  LogicalAndNode(ExpressionNode *arg1, ExpressionNode *arg2)
    : OperatorNode(), arg1_(arg1), arg2_(arg2), arg_values_(),
      temp_records_() {}
};

grn_rc LogicalAndNode::open(ExpressionNode *arg1, ExpressionNode *arg2,
                            ExpressionNode **node) {
  LogicalAndNode *new_node = new (std::nothrow) LogicalAndNode(arg1, arg2);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  *node = new_node;
  return GRN_SUCCESS;
}

grn_rc LogicalAndNode::filter(Record *input, size_t input_size,
                              Record *output, size_t *output_size) {
  grn_rc rc = arg1_->filter(input, input_size, output, output_size);
  if (rc == GRN_SUCCESS) {
    rc = arg2_->filter(output, *output_size, output, output_size);
  }
  return rc;
}

grn_rc LogicalAndNode::evaluate(const Record *records, size_t num_records,
                                void *results) {
  // Evaluate "arg1" for all the records.
  // Then, evaluate "arg2" for non-false records.
  grn_rc rc = arg1_->evaluate(records, num_records, results);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (temp_records_.size() < num_records) try {
    temp_records_.resize(num_records);
  } catch (const std::bad_alloc &) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  size_t count = 0;
  for (size_t i = 0; i < num_records; ++i) {
    if (((grn_ts_bool *)results)[i]) {
      temp_records_[count] = records[i];
      ++count;
    }
  }
  if (count == 0) {
    // Nothing to do.
    return GRN_SUCCESS;
  }

  size_t old_size = arg_values_.size() / sizeof(grn_ts_bool);
  if (old_size < num_records) try {
    arg_values_.resize(sizeof(grn_ts_bool) * num_records);
  } catch (const std::bad_alloc &) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  rc = arg2_->evaluate(&*temp_records_.begin(), count, &*arg_values_.begin());
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  // Merge the evaluation results.
  count = 0;
  for (size_t i = 0; i < num_records; ++i) {
    if (((grn_ts_bool *)results)[i]) {
      ((grn_ts_bool *)results)[i] = arg_values_[count];
      ++count;
    }
  }
  return GRN_SUCCESS;
}

// ---- LogicalOrNode ----

class LogicalOrNode : public OperatorNode {
 public:
  ~LogicalOrNode() {}

  static grn_rc open(ExpressionNode *arg1, ExpressionNode *arg2,
                     ExpressionNode **node);

  DataKind data_kind() const {
    return GRN_TS_BOOL;
  }
  DataType data_type() const {
    return GRN_DB_BOOL;
  }
  grn_obj *ref_table() const {
    return NULL;
  }
  int dimension() const {
    return 0;
  }

  grn_rc filter(Record *input, size_t input_size,
                Record *output, size_t *output_size);
  grn_rc evaluate(const Record *records, size_t num_records, void *results);

 private:
  ExpressionNode *arg1_;
  ExpressionNode *arg2_;
  std::vector<grn_ts_bool> arg1_values_;
  std::vector<grn_ts_bool> arg2_values_;
  std::vector<Record> temp_records_;

  LogicalOrNode(ExpressionNode *arg1, ExpressionNode *arg2)
    : OperatorNode(), arg1_(arg1), arg2_(arg2), arg1_values_(),
      arg2_values_(), temp_records_() {}

  grn_rc fill_arg_values(ExpressionNode *arg,
                         const Record *records, size_t num_records,
                         std::vector<grn_ts_bool> *arg_values);
};

grn_rc LogicalOrNode::open(ExpressionNode *arg1, ExpressionNode *arg2,
                            ExpressionNode **node) {
  LogicalOrNode *new_node = new (std::nothrow) LogicalOrNode(arg1, arg2);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  *node = new_node;
  return GRN_SUCCESS;
}

grn_rc LogicalOrNode::filter(Record *input, size_t input_size,
                             Record *output, size_t *output_size) {
  // Evaluate "arg1" for all the records.
  // Then, evaluate "arg2" for false records.
  grn_rc rc = fill_arg_values(arg1_, input, input_size, &arg1_values_);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (temp_records_.size() < input_size) try {
    temp_records_.resize(input_size);
  } catch (const std::bad_alloc &) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  size_t count = 0;
  for (size_t i = 0; i < input_size; ++i) {
    if (!arg1_values_[i]) {
      temp_records_[count] = input[i];
      ++count;
    }
  }
  if (count == 0) {
    if (input != output) {
      for (size_t i = 0; i < input_size; ++i) {
        output[i] = input[i];
      }
    }
    *output_size = input_size;
    return GRN_SUCCESS;
  }
  rc = fill_arg_values(arg2_, &*temp_records_.begin(), count, &arg2_values_);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  // Merge the evaluation results.
  count = 0;
  size_t output_count = 0;
  for (size_t i = 0; i < input_size; ++i) {
    if (arg1_values_[i]) {
      output[output_count] = input[i];
      ++output_count;
    } else {
      if (arg2_values_[count]) {
        output[output_count] = input[i];
        ++output_count;
      }
      ++count;
    }
  }
  *output_size = output_count;
  return GRN_SUCCESS;
}

grn_rc LogicalOrNode::evaluate(const Record *records, size_t num_records,
                               void *results) {
  // Evaluate "arg1" for all the records.
  // Then, evaluate "arg2" for false records.
  grn_rc rc = arg1_->evaluate(records, num_records, results);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (temp_records_.size() < num_records) try {
    temp_records_.resize(num_records);
  } catch (const std::bad_alloc &) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  size_t count = 0;
  for (size_t i = 0; i < num_records; ++i) {
    if (!static_cast<grn_ts_bool *>(results)[i]) {
      temp_records_[count] = records[i];
      ++count;
    }
  }
  if (count == 0) {
    // Nothing to do.
    return GRN_SUCCESS;
  }
  rc = fill_arg_values(arg2_, &*temp_records_.begin(), count, &arg2_values_);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  // Merge the evaluation results.
  count = 0;
  for (size_t i = 0; i < num_records; ++i) {
    if (!static_cast<grn_ts_bool *>(results)[i]) {
      static_cast<grn_ts_bool *>(results)[i] = arg2_values_[count];
      ++count;
    }
  }
  return GRN_SUCCESS;
}

grn_rc LogicalOrNode::fill_arg_values(ExpressionNode *arg,
                                      const Record *records,
                                      size_t num_records,
                                      std::vector<grn_ts_bool> *arg_values) {
  size_t old_size = arg_values->size() / sizeof(grn_ts_bool);
  if (old_size < num_records) try {
    arg_values->resize(sizeof(grn_ts_bool) * num_records);
  } catch (const std::bad_alloc &) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  return arg->evaluate(records, num_records, &*arg_values->begin());
}

//// -- GenericBinaryNode --

//template <typename T,
//          typename U = typename T::Value,
//          typename V = typename T::Arg1,
//          typename W = typename T::Arg2>
//class GenericBinaryNode : public BinaryNode<U, V, W> {
// public:
//  GenericBinaryNode(ExpressionNode *arg1, ExpressionNode *arg2)
//    : BinaryNode<U, V, W>(arg1, arg2), operator_() {}
//  ~GenericBinaryNode() {}

//  grn_rc evaluate(
//    const Record *records, size_t num_records, Bool *results);

// private:
//  T operator_;
//};

//template <typename T, typename U, typename V, typename W>
//grn_rc GenericBinaryNode<T, U, V, W>::evaluate(
//    const Record *records, size_t num_records, Bool *results) {
//  grn_rc rc = this->fill_arg1_values(records, num_records);
//  if (rc != GRN_SUCCESS) {
//    return rc;
//  }
//  rc = this->fill_arg2_values(records, num_records);
//  if (rc != GRN_SUCCESS) {
//    return rc;
//  }
//  for (size_t i = 0; i < num_records; ++i) {
//    results[i] = operator_(this->arg1_values_[i], this->arg2_values_[i]);
//  }
//  return GRN_SUCCESS;
//}

//template <typename T, typename V, typename W>
//class GenericBinaryNode<T, Bool, V, W> : public BinaryNode<Bool, V, W> {
// public:
//  GenericBinaryNode(ExpressionNode *arg1, ExpressionNode *arg2)
//    : BinaryNode<Bool, V, W>(arg1, arg2), operator_() {}
//  ~GenericBinaryNode() {}

//  grn_rc filter(
//    Record *input, size_t input_size,
//    Record *output, size_t *output_size);

//  grn_rc evaluate(
//    const Record *records, size_t num_records, Bool *results);

// private:
//  T operator_;
//};

//template <typename T, typename V, typename W>
//grn_rc GenericBinaryNode<T, Bool, V, W>::filter(
//    Record *input, size_t input_size,
//    Record *output, size_t *output_size) {
//  grn_rc rc = this->fill_arg1_values(input, input_size);
//  if (rc != GRN_SUCCESS) {
//    return rc;
//  }
//  rc = this->fill_arg2_values(input, input_size);
//  if (rc != GRN_SUCCESS) {
//    return rc;
//  }
//  size_t count = 0;
//  for (size_t i = 0; i < input_size; ++i) {
//    if (operator_(this->arg1_values_[i], this->arg2_values_[i]).raw ==
//      GRN_TRUE) {
//      output[count] = input[i];
//      ++count;
//    }
//  }
//  *output_size = count;
//  return GRN_SUCCESS;
//}

//template <typename T, typename V, typename W>
//grn_rc GenericBinaryNode<T, Bool, V, W>::evaluate(
//  const Record *records, size_t num_records, Bool *results) {
//  grn_rc rc = this->fill_arg1_values(records, num_records);
//  if (rc != GRN_SUCCESS) {
//    return rc;
//  }
//  rc = this->fill_arg2_values(records, num_records);
//  if (rc != GRN_SUCCESS) {
//    return rc;
//  }
//  for (size_t i = 0; i < num_records; ++i) {
//    results[i] = operator_(this->arg1_values_[i], this->arg2_values_[i]);
//  }
//  return GRN_SUCCESS;
//}

//// ----- EqualNode -----

//template <typename T>
//struct EqualOperator {
//  typedef Bool Value;
//  typedef T Arg1;
//  typedef T Arg2;
//  Value operator()(const Arg1 &arg1, const Arg2 &arg2) const {
//    return Bool(arg1 == arg2);
//  }
//};

//template <typename T>
//grn_rc equal_node_open(EqualOperator<T> op,
//  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
//  GenericBinaryNode<EqualOperator<T> > *new_node =
//    new (std::nothrow) GenericBinaryNode<EqualOperator<T> >(arg1, arg2);
//  if (!new_node) {
//    return GRN_NO_MEMORY_AVAILABLE;
//  }
//  *node = new_node;
//  return GRN_SUCCESS;
//}

//// ----- NotEqualNode -----

//template <typename T>
//struct NotEqualOperator {
//  typedef Bool Value;
//  typedef T Arg1;
//  typedef T Arg2;
//  Value operator()(const Arg1 &arg1, const Arg2 &arg2) const {
//    return Bool(arg1 != arg2);
//  }
//};

//template <typename T>
//grn_rc not_equal_node_open(NotEqualOperator<T> op,
//  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
//  GenericBinaryNode<NotEqualOperator<T> > *new_node =
//    new (std::nothrow) GenericBinaryNode<NotEqualOperator<T> >(arg1, arg2);
//  if (!new_node) {
//    return GRN_NO_MEMORY_AVAILABLE;
//  }
//  *node = new_node;
//  return GRN_SUCCESS;
//}

//// ----- LessNode -----

//template <typename T>
//struct LessOperator {
//  typedef Bool Value;
//  typedef T Arg1;
//  typedef T Arg2;
//  Value operator()(const Arg1 &arg1, const Arg2 &arg2) const {
//    return Bool(arg1 < arg2);
//  }
//};

//template <typename T>
//grn_rc less_node_open(LessOperator<T> op,
//  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
//  GenericBinaryNode<LessOperator<T> > *new_node =
//    new (std::nothrow) GenericBinaryNode<LessOperator<T> >(arg1, arg2);
//  if (!new_node) {
//    return GRN_NO_MEMORY_AVAILABLE;
//  }
//  *node = new_node;
//  return GRN_SUCCESS;
//}

//// ----- LessEqualNode -----

//template <typename T>
//struct LessEqualOperator {
//  typedef Bool Value;
//  typedef T Arg1;
//  typedef T Arg2;
//  Value operator()(const Arg1 &arg1, const Arg2 &arg2) const {
//    return Bool(arg1 < arg2);
//  }
//};

//template <typename T>
//grn_rc less_equal_node_open(LessEqualOperator<T> op,
//  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
//  GenericBinaryNode<LessEqualOperator<T> > *new_node =
//    new (std::nothrow) GenericBinaryNode<LessEqualOperator<T> >(arg1, arg2);
//  if (!new_node) {
//    return GRN_NO_MEMORY_AVAILABLE;
//  }
//  *node = new_node;
//  return GRN_SUCCESS;
//}

//// ----- GreaterNode -----

//template <typename T>
//struct GreaterOperator {
//  typedef Bool Value;
//  typedef T Arg1;
//  typedef T Arg2;
//  Value operator()(const Arg1 &arg1, const Arg2 &arg2) const {
//    return Bool(arg1 < arg2);
//  }
//};

//template <typename T>
//grn_rc greater_node_open(GreaterOperator<T> op,
//  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
//  GenericBinaryNode<GreaterOperator<T> > *new_node =
//    new (std::nothrow) GenericBinaryNode<GreaterOperator<T> >(arg1, arg2);
//  if (!new_node) {
//    return GRN_NO_MEMORY_AVAILABLE;
//  }
//  *node = new_node;
//  return GRN_SUCCESS;
//}

//// ----- GreaterEqualNode -----

//template <typename T>
//struct GreaterEqualOperator {
//  typedef Bool Value;
//  typedef T Arg1;
//  typedef T Arg2;
//  Value operator()(const Arg1 &arg1, const Arg2 &arg2) const {
//    return Bool(arg1 < arg2);
//  }
//};

//template <typename T>
//grn_rc greater_equal_node_open(GreaterEqualOperator<T> op,
//  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
//  GenericBinaryNode<GreaterEqualOperator<T> > *new_node =
//    new (std::nothrow) GenericBinaryNode<GreaterEqualOperator<T> >(arg1, arg2);
//  if (!new_node) {
//    return GRN_NO_MEMORY_AVAILABLE;
//  }
//  *node = new_node;
//  return GRN_SUCCESS;
//}

// -- ExpressionToken --

enum ExpressionTokenType {
  DUMMY_TOKEN,
  CONSTANT_TOKEN,
  NAME_TOKEN,
  UNARY_OPERATOR_TOKEN,
  BINARY_OPERATOR_TOKEN,
  DEREFERENCE_TOKEN,
  BRACKET_TOKEN
};

enum ExpressionBracketType {
  LEFT_ROUND_BRACKET,
  RIGHT_ROUND_BRACKET,
  LEFT_SQUARE_BRACKET,
  RIGHT_SQUARE_BRACKET
};

// TODO: std::string should not be used.
class ExpressionToken {
 public:
  ExpressionToken() : string_(), type_(DUMMY_TOKEN), dummy_(0), priority_(0) {}
  ExpressionToken(const std::string &string, ExpressionTokenType token_type)
    : string_(string), type_(token_type), dummy_(0), priority_(0) {}
  ExpressionToken(const std::string &string,
    ExpressionBracketType bracket_type)
      : string_(string), type_(BRACKET_TOKEN), bracket_type_(bracket_type),
        priority_(0) {}
  ExpressionToken(const std::string &string, OperatorType operator_type)
      : string_(string), type_(get_operator_token_type(operator_type)),
        operator_type_(operator_type),
        priority_(get_operator_priority(operator_type)) {}

  const std::string &string() const {
    return string_;
  }
  ExpressionTokenType type() const {
    return type_;
  }
  ExpressionBracketType bracket_type() const {
    return bracket_type_;
  }
  OperatorType operator_type() const {
    return operator_type_;
  }
  int priority() const {
    return priority_;
  }

 private:
  std::string string_;
  ExpressionTokenType type_;
  union {
    int dummy_;
    ExpressionBracketType bracket_type_;
    OperatorType operator_type_;
  };
  int priority_;

  static ExpressionTokenType get_operator_token_type(
    OperatorType operator_type);
  static int get_operator_priority(OperatorType operator_type);
};

ExpressionTokenType ExpressionToken::get_operator_token_type(
  OperatorType operator_type) {
  switch (operator_type) {
    case GRN_TS_LOGICAL_NOT: {
      return UNARY_OPERATOR_TOKEN;
    }
    case GRN_TS_LOGICAL_AND:
    case GRN_TS_LOGICAL_OR:
    case GRN_TS_EQUAL:
    case GRN_TS_NOT_EQUAL:
    case GRN_TS_LESS:
    case GRN_TS_LESS_EQUAL:
    case GRN_TS_GREATER:
    case GRN_TS_GREATER_EQUAL: {
      return BINARY_OPERATOR_TOKEN;
    }
    default: {
      // TODO: ERROR_TOKEN or something should be used...?
      //       Or, default should be removed?
      return DUMMY_TOKEN;
    }
  }
}

int ExpressionToken::get_operator_priority(
  OperatorType operator_type) {
  switch (operator_type) {
    case GRN_TS_LOGICAL_NOT: {
//    case GRN_OP_BITWISE_NOT:
//    case GRN_OP_POSITIVE:
//    case GRN_OP_NEGATIVE:
//    case GRN_OP_TO_INT:
//    case GRN_OP_TO_FLOAT: {
      return 3;
    }
    case GRN_TS_LOGICAL_AND: {
      return 13;
    }
    case GRN_TS_LOGICAL_OR: {
      return 14;
    }
    case GRN_TS_EQUAL:
    case GRN_TS_NOT_EQUAL: {
      return 9;
    }
    case GRN_TS_LESS:
    case GRN_TS_LESS_EQUAL:
    case GRN_TS_GREATER:
    case GRN_TS_GREATER_EQUAL: {
      return 8;
    }
//    case GRN_OP_BITWISE_AND: {
//      return 10;
//    }
//    case GRN_OP_BITWISE_OR: {
//      return 12;
//    }
//    case GRN_OP_BITWISE_XOR: {
//      return 11;
//    }
//    case GRN_OP_PLUS:
//    case GRN_OP_MINUS: {
//      return 6;
//    }
//    case GRN_OP_MULTIPLICATION:
//    case GRN_OP_DIVISION:
//    case GRN_OP_MODULUS: {
//      return 5;
//    }
//    case GRN_OP_STARTS_WITH:
//    case GRN_OP_ENDS_WITH:
//    case GRN_OP_CONTAINS: {
//      return 7;
//    }
//    case GRN_OP_SUBSCRIPT: {
//      return 2;
//    }
    default: {
      return 100;
    }
  }
}

// -- ExpressionParser --

class ExpressionParser {
 public:
  static grn_rc parse(grn_ctx *ctx, grn_obj *table,
    const char *query, size_t query_size, Expression **expression);

 private:
  grn_ctx *ctx_;
  grn_obj *table_;
  std::vector<ExpressionToken> tokens_;
  std::vector<ExpressionToken> stack_;
  Expression *expression_;

  ExpressionParser(grn_ctx *ctx, grn_obj *table)
    : ctx_(ctx), table_(table), tokens_(), stack_(), expression_(NULL) {}
  ~ExpressionParser() {
    delete expression_;
  }

  grn_rc tokenize(const char *query, size_t query_size);
  grn_rc compose();
  grn_rc push_token(const ExpressionToken &token);
};

grn_rc ExpressionParser::parse(grn_ctx *ctx, grn_obj *table,
  const char *query, size_t query_size, Expression **expression) {
  ExpressionParser *parser = new (std::nothrow) ExpressionParser(ctx, table);
  if (!parser) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_rc rc = parser->tokenize(query, query_size);
  if (rc == GRN_SUCCESS) {
    rc = parser->compose();
    if (rc == GRN_SUCCESS) {
      *expression = parser->expression_;
      parser->expression_ = NULL;
    }
  }
  delete parser;
  return rc;
}

grn_rc ExpressionParser::tokenize(const char *query, size_t query_size) {
  const char *rest = query;
  size_t rest_size = query_size;
  while (rest_size != 0) {
    // Ignore white-space characters.
    size_t pos;
    for (pos = 0; pos < rest_size; ++pos) {
      if (!std::isspace(static_cast<uint8_t>(rest[pos]))) {
        break;
      }
    }
    rest += pos;
    rest_size -= pos;
    switch (rest[0]) {
      case '!': {
//        if ((rest_size >= 2) && (rest[1] == '=')) {
//          tokens_.push_back(ExpressionToken("!=", GRN_TS_NOT_EQUAL));
//          rest += 2;
//          rest_size -= 2;
//        } else {
          tokens_.push_back(ExpressionToken("!", GRN_TS_LOGICAL_NOT));
          ++rest;
          --rest_size;
//        }
        break;
      }
////      case '~': {
////        tokens_.push_back(ExpressionToken("~", GRN_OP_BITWISE_NOT));
////        rest = rest.substring(1);
////        break;
////      }
//      case '=': {
//        if ((rest_size >= 2) && (rest[1] == '=')) {
//          tokens_.push_back(ExpressionToken("==", GRN_TS_EQUAL));
//          rest += 2;
//          rest_size -= 2;
//        } else {
//          return GRN_INVALID_ARGUMENT;
//        }
//        break;
//      }
//      case '<': {
//        if ((rest_size >= 2) && (rest[1] == '=')) {
//          tokens_.push_back(ExpressionToken("<=", GRN_TS_LESS_EQUAL));
//          rest += 2;
//          rest_size -= 2;
//        } else {
//          tokens_.push_back(ExpressionToken("<", GRN_TS_LESS));
//          ++rest;
//          --rest_size;
//        }
//        break;
//      }
//      case '>': {
//        if ((rest_size >= 2) && (rest[1] == '=')) {
//          tokens_.push_back(ExpressionToken(">=", GRN_TS_GREATER_EQUAL));
//          rest += 2;
//          rest_size -= 2;
//        } else {
//          tokens_.push_back(ExpressionToken(">", GRN_TS_GREATER));
//          ++rest;
//          --rest_size;
//        }
//        break;
//      }
      case '&': {
        if ((rest_size >= 2) && (rest[1] == '&')) {
          tokens_.push_back(ExpressionToken("&&", GRN_TS_LOGICAL_AND));
          rest += 2;
          rest_size -= 2;
        } else {
//          tokens_.push_back(ExpressionToken("&", GRN_OP_BITWISE_AND));
//          ++rest;
//          --rest_size;
          return GRN_INVALID_ARGUMENT;
        }
        break;
      }
//      case '|': {
//        if ((rest_size >= 2) && (rest[1] == '|')) {
//          tokens_.push_back(ExpressionToken("||", GRN_TS_LOGICAL_OR));
//          rest += 2;
//          rest_size -= 2;
//        } else {
////          tokens_.push_back(ExpressionToken("|", GRN_OP_BITWISE_OR));
////          ++rest;
////          --rest_size;
//          return GRN_INVALID_ARGUMENT;
//        }
//        break;
//      }
//      case '^': {
//        tokens_.push_back(ExpressionToken("^", GRN_OP_BITWISE_XOR));
//        rest = rest.substring(1);
//        break;
//      }
//      case '+': {
//        tokens_.push_back(ExpressionToken("+", GRN_OP_PLUS));
//        rest = rest.substring(1);
//        break;
//      }
//      case '-': {
//        tokens_.push_back(ExpressionToken("-", GRN_OP_MINUS));
//        rest = rest.substring(1);
//        break;
//      }
//      case '*': {
//        tokens_.push_back(ExpressionToken("*", GRN_OP_MULTIPLICATION));
//        rest = rest.substring(1);
//        break;
//      }
//      case '/': {
//        tokens_.push_back(ExpressionToken("/", GRN_OP_DIVISION));
//        rest = rest.substring(1);
//        break;
//      }
//      case '%': {
//        tokens_.push_back(ExpressionToken("%", GRN_OP_MODULUS));
//        rest = rest.substring(1);
//        break;
//      }
//      case '@': {
//        if ((rest_size >= 2) && (rest[1] == '^')) {
//          tokens_.push_back(ExpressionToken("@^", GRN_OP_STARTS_WITH));
//          rest = rest.substring(2);
//        } else if ((rest_size >= 2) && (rest[1] == '$')) {
//          tokens_.push_back(ExpressionToken("@$", GRN_OP_ENDS_WITH));
//          rest = rest.substring(2);
//        } else {
//          tokens_.push_back(ExpressionToken("@", GRN_OP_CONTAINS));
//          rest = rest.substring(1);
//        }
//        break;
//      }
//      case '.': {
//        tokens_.push_back(ExpressionToken(".", DEREFERENCE_TOKEN));
//        rest = rest.substring(1);
//        break;
//      }
      case '(': {
        tokens_.push_back(ExpressionToken("(", LEFT_ROUND_BRACKET));
        ++rest;
        --rest_size;
        break;
      }
      case ')': {
        tokens_.push_back(ExpressionToken(")", RIGHT_ROUND_BRACKET));
        ++rest;
        --rest_size;
        break;
      }
//      case '[': {
//        tokens_.push_back(ExpressionToken("[", LEFT_SQUARE_BRACKET));
//        rest = rest.substring(1);
//        break;
//      }
//      case ']': {
//        tokens_.push_back(ExpressionToken("]", RIGHT_SQUARE_BRACKET));
//        rest = rest.substring(1);
//        break;
//      }
      case '"': {
        for (pos = 1; pos < rest_size; ++pos) {
          if (rest[pos] == '\\') {
            if (pos == rest_size) {
              break;
            }
            ++pos;
          } else if (rest[pos] == '"') {
            break;
          }
        }
        if (pos == rest_size) {
          return GRN_INVALID_ARGUMENT;
        }
        tokens_.push_back(
          ExpressionToken(std::string(rest + 1, pos - 1), CONSTANT_TOKEN));
        rest += pos + 1;
        rest_size -= pos + 1;
        break;
      }
      case '0' ... '9': {
        // TODO: Improve this.
        for (pos = 1; pos < rest_size; ++pos) {
          if (!std::isdigit(static_cast<uint8_t>(rest[pos]))) {
            break;
          }
        }
        tokens_.push_back(
          ExpressionToken(std::string(rest, pos), CONSTANT_TOKEN));
        rest += pos;
        rest_size -= pos;
        break;
      }
      case '_':
      case 'A' ... 'Z':
      case 'a' ... 'z': {
        // TODO: Improve this.
        for (pos = 1; pos < rest_size; ++pos) {
          if ((rest[pos] != '_') && (!std::isalnum(rest[pos]))) {
            break;
          }
        }
        std::string token(rest, pos);
        if ((token == "true") || (token == "false")) {
          tokens_.push_back(ExpressionToken(token, CONSTANT_TOKEN));
        } else {
          tokens_.push_back(ExpressionToken(token, NAME_TOKEN));
        }
        rest += pos;
        rest_size -= pos;
        break;
      }
      default: {
        return GRN_INVALID_ARGUMENT;
      }
    }
  }
  return GRN_SUCCESS;
}

grn_rc ExpressionParser::compose() {
  if (tokens_.size() == 0) {
    return GRN_INVALID_ARGUMENT;
  }
  expression_ = new (std::nothrow) Expression(ctx_, table_);
  grn_rc rc = push_token(ExpressionToken("(", LEFT_ROUND_BRACKET));
  if (rc == GRN_SUCCESS) {
    for (size_t i = 0; i < tokens_.size(); ++i) {
      rc = push_token(tokens_[i]);
      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    if (rc == GRN_SUCCESS) {
      rc = push_token(ExpressionToken(")", RIGHT_ROUND_BRACKET));
    }
  }
  return rc;
}

grn_rc ExpressionParser::push_token(const ExpressionToken &token) {
  grn_rc rc = GRN_SUCCESS;
  switch (token.type()) {
    case DUMMY_TOKEN: {
      if ((stack_.size() != 0) && (stack_.back().type() == DUMMY_TOKEN)) {
        return GRN_INVALID_ARGUMENT;
      }
      stack_.push_back(token);
      break;
    }
    case CONSTANT_TOKEN: {
      grn_obj *obj;
      const std::string string = token.string();
      if (std::isdigit(static_cast<uint8_t>(string[0]))) {
        if (string.find_first_of('.') == string.npos) {
          obj = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_INT64);
          if (obj) {
            GRN_INT64_SET(ctx_, obj, strtoll(string.c_str(), NULL, 10));
          }
        } else {
          obj = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_FLOAT);
          if (obj) {
            GRN_FLOAT_SET(ctx_, obj, strtod(string.c_str(), NULL));
          }
        }
      } else if (string == "true") {
        obj = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_BOOL);
        if (obj) {
          GRN_BOOL_SET(ctx_, obj, GRN_TRUE);
        }
      } else if (string == "false") {
        obj = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_BOOL);
        if (obj) {
          GRN_BOOL_SET(ctx_, obj, GRN_FALSE);
        }
      } else {
        obj = grn_obj_open(ctx_, GRN_BULK, 0, GRN_DB_TEXT);
        if (obj) {
          GRN_TEXT_SET(ctx_, obj, string.data(), string.size());
        }
      }
      if (!obj) {
        if (ctx_->rc != GRN_SUCCESS) {
          return ctx_->rc;
        }
        return GRN_UNKNOWN_ERROR;
      }
      rc = push_token(ExpressionToken(string, DUMMY_TOKEN));
      if (rc == GRN_SUCCESS) {
        rc = expression_->push_object(obj);
      }
      if (rc != GRN_SUCCESS) {
        grn_obj_close(ctx_, obj);
      }
      break;
    }
    case NAME_TOKEN: {
      rc = push_token(ExpressionToken(token.string(), DUMMY_TOKEN));
      if (rc == GRN_SUCCESS) {
        grn_obj *column = grn_obj_column(
          ctx_, table_, token.string().data(), token.string().size());
        rc = expression_->push_object(column);
      }
      break;
    }
    case UNARY_OPERATOR_TOKEN: {
      if ((stack_.size() != 0) && (stack_.back().type() == DUMMY_TOKEN)) {
        // A unary operator must not follow an operand.
        return GRN_INVALID_ARGUMENT;
      }
      stack_.push_back(token);
      break;
    }
    case BINARY_OPERATOR_TOKEN: {
      if ((stack_.size() == 0) || (stack_.back().type() != DUMMY_TOKEN)) {
        // A binary operator must follow an operand.
        return GRN_INVALID_ARGUMENT;
      }
      // Apply previous operators if those are prior to the new operator.
      while (stack_.size() >= 2) {
        ExpressionToken operator_token = stack_[stack_.size() - 2];
//        if (operator_token.type() == DEREFERENCE_TOKEN) {
//          expression_->end_subexpression();
//          stack_.pop_back();
//          stack_.pop_back();
//          push_token(ExpressionToken("", DUMMY_TOKEN));
//        } else if (operator_token.type() == UNARY_OPERATOR_TOKEN) {
        if (operator_token.type() == UNARY_OPERATOR_TOKEN) {
          rc = expression_->push_operator(operator_token.operator_type());
          if (rc == GRN_SUCCESS) {
            stack_.pop_back();
            stack_.pop_back();
            rc = push_token(ExpressionToken("", DUMMY_TOKEN));
          }
        } else if ((operator_token.type() == BINARY_OPERATOR_TOKEN) &&
                   (operator_token.priority() <= token.priority())) {
          rc = expression_->push_operator(operator_token.operator_type());
          if (rc == GRN_SUCCESS) {
            stack_.pop_back();
            stack_.pop_back();
            stack_.pop_back();
            rc = push_token(ExpressionToken("", DUMMY_TOKEN));
          }
        } else {
          break;
        }
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      stack_.push_back(token);
      break;
    }
//    case DEREFERENCE_TOKEN: {
//      builder_->begin_subexpression();
//      stack_.pop_back();
//      stack_.push_back(token);
//      break;
//    }
    case BRACKET_TOKEN: {
      if (token.bracket_type() == LEFT_ROUND_BRACKET) {
        // A left round bracket must not follow a dummy.
        if ((stack_.size() != 0) && (stack_.back().type() == DUMMY_TOKEN)) {
          return GRN_INVALID_ARGUMENT;
        }
        stack_.push_back(token);
      } else if (token.bracket_type() == RIGHT_ROUND_BRACKET) {
        // A right round bracket must follow a dummy.
        // A left round bracket must exist before a right round bracket.
        if ((stack_.size() < 2) || (stack_.back().type() != DUMMY_TOKEN)) {
          return GRN_INVALID_ARGUMENT;
        }
        // Apply operators in brackets.
        while (stack_.size() >= 2) {
          ExpressionToken operator_token = stack_[stack_.size() - 2];
//          if (operator_token.type() == DEREFERENCE_TOKEN) {
//            rc = expression_->end_subexpression();
//            if (rc == GRN_SUCCESS) {
//              stack_.pop_back();
//              stack_.pop_back();
//              rc = push_token(ExpressionToken("", DUMMY_TOKEN));
//            }
//          } else if (operator_token.type() == UNARY_OPERATOR_TOKEN) {
          if (operator_token.type() == UNARY_OPERATOR_TOKEN) {
            rc = expression_->push_operator(operator_token.operator_type());
            if (rc == GRN_SUCCESS) {
              stack_.pop_back();
              stack_.pop_back();
              rc = push_token(ExpressionToken("", DUMMY_TOKEN));
            }
          } else if (operator_token.type() == BINARY_OPERATOR_TOKEN) {
            rc = expression_->push_operator(operator_token.operator_type());
            if (rc == GRN_SUCCESS) {
              stack_.pop_back();
              stack_.pop_back();
              stack_.pop_back();
              rc = push_token(ExpressionToken("", DUMMY_TOKEN));
            }
          } else {
            break;
          }
          if (rc != GRN_SUCCESS) {
            return rc;
          }
        }
        if ((stack_.size() < 2) ||
            (stack_[stack_.size() - 2].type() != BRACKET_TOKEN) ||
            (stack_[stack_.size() - 2].bracket_type() != LEFT_ROUND_BRACKET)) {
          return GRN_INVALID_ARGUMENT;
        }
        stack_[stack_.size() - 2] = stack_.back();
        stack_.pop_back();
//      } else if (token.bracket_type() == LEFT_SQUARE_BRACKET) {
//        // A left square bracket must follow a dummy.
//        if ((stack_.size() == 0) || (stack_.back().type() != DUMMY_TOKEN)) {
//          return GRN_INVALID_ARGUMENT;
//        }
//        stack_.push_back(token);
//      } else if (token.bracket_type() == RIGHT_SQUARE_BRACKET) {
//        // A right round bracket must follow a dummy.
//        // A left round bracket must exist before a right round bracket.
//        if ((stack_.size() < 2) || (stack_.back().type() != DUMMY_TOKEN)) {
//          return GRN_INVALID_ARGUMENT;
//        }
//        // Apply operators in bracket.
//        while (stack_.size() >= 2) {
//          ExpressionToken operator_token = stack_[stack_.size() - 2];
//          if (operator_token.type() == DEREFERENCE_TOKEN) {
//            builder_->end_subexpression();
//            stack_.pop_back();
//            stack_.pop_back();
//            push_token(ExpressionToken("", DUMMY_TOKEN));
//          } else if (operator_token.type() == UNARY_OPERATOR_TOKEN) {
//            builder_->push_operator(operator_token.operator_type());
//            stack_.pop_back();
//            stack_.pop_back();
//            push_token(ExpressionToken("", DUMMY_TOKEN));
//          } else if (operator_token.type() == BINARY_OPERATOR_TOKEN) {
//            builder_->push_operator(operator_token.operator_type());
//            stack_.pop_back();
//            stack_.pop_back();
//            stack_.pop_back();
//            push_token(ExpressionToken("", DUMMY_TOKEN));
//          } else {
//            break;
//          }
//        }
//        if ((stack_.size() < 2) ||
//            (stack_[stack_.size() - 2].type() != BRACKET_TOKEN) ||
//            (stack_[stack_.size() - 2].bracket_type() != LEFT_SQUARE_BRACKET)) {
//          return GRN_INVALID_ARGUMENT;
//        }
//        stack_.pop_back();
//        stack_.pop_back();
//        builder_->push_operator(GRNXX_SUBSCRIPT);
      } else {
        return GRN_INVALID_ARGUMENT;
      }
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  return rc;
}

// -- Expression --

Expression::Expression(grn_ctx *ctx, grn_obj *table)
  : ctx_(ctx), table_(table), type_(GRN_TS_INCOMPLETE),
    output_type_(GRN_DB_VOID), stack_() {}

Expression::~Expression() {
  for (size_t i = 0; i < stack_.size(); ++i) {
    delete stack_[i];
  }
}

grn_rc Expression::open(
  grn_ctx *ctx, grn_obj *table, Expression **expression) {
  if (!ctx || !grn_obj_is_table(ctx, table) || !expression) {
    return GRN_INVALID_ARGUMENT;
  }
  Expression *new_expression = new (std::nothrow) Expression(ctx, table);
  if (!new_expression) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  *expression = new_expression;
  return GRN_SUCCESS;
}

grn_rc Expression::parse(grn_ctx *ctx, grn_obj *table,
  const char *query, size_t query_size, Expression **expression) {
  if (!ctx || !grn_obj_is_table(ctx, table) ||
      !query || (query_size == 0) || !expression) {
    return GRN_INVALID_ARGUMENT;
  }
  return ExpressionParser::parse(ctx, table, query, query_size, expression);
}

DataKind Expression::data_kind() const {
  ExpressionNode *root = this->root();
  return root ? root->data_kind() : GRN_TS_VOID;
}

DataType Expression::data_type() const {
  ExpressionNode *root = this->root();
  return root ? root->data_type() : GRN_ID_NIL;
}

DataType Expression::output_type() const {
  return output_type_;
}

int Expression::dimension() const {
  ExpressionNode *root = this->root();
  return root ? root->dimension() : 0;
}

grn_rc Expression::push_object(grn_obj *obj) {
  if (!obj) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_rc rc = GRN_UNKNOWN_ERROR;
  switch (obj->header.type) {
    case GRN_BULK:
    case GRN_UVECTOR:
    case GRN_VECTOR: {
      rc = push_constant_object(obj);
      break;
    }
    case GRN_ACCESSOR:
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE: {
      rc = push_column_object(obj);
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  if (rc == GRN_SUCCESS) {
    update_type();
  }
  return rc;
}

grn_rc Expression::push_operator(OperatorType operator_type) {
  grn_rc rc = GRN_UNKNOWN_ERROR;
  ExpressionNode *node;
  switch (operator_type) {
    case GRN_TS_LOGICAL_NOT: {
      if (stack_.size() < 1) {
        return GRN_INVALID_FORMAT;
      }
      ExpressionNode *arg = stack_[stack_.size() - 1];
      rc = create_unary_node(operator_type, arg, &node);
      if (rc == GRN_SUCCESS) {
        stack_.resize(stack_.size() - 1);
      }
      break;
    }
    case GRN_TS_LOGICAL_AND:
    case GRN_TS_LOGICAL_OR:
    case GRN_TS_EQUAL:
    case GRN_TS_NOT_EQUAL:
    case GRN_TS_LESS:
    case GRN_TS_LESS_EQUAL:
    case GRN_TS_GREATER:
    case GRN_TS_GREATER_EQUAL: {
      if (stack_.size() < 2) {
        return GRN_INVALID_FORMAT;
      }
      ExpressionNode *arg1 = stack_[stack_.size() - 2];
      ExpressionNode *arg2 = stack_[stack_.size() - 1];
      rc = create_binary_node(operator_type, arg1, arg2, &node);
      if (rc == GRN_SUCCESS) {
        stack_.resize(stack_.size() - 2);
      }
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  if (rc == GRN_SUCCESS) {
    stack_.push_back(node);
    update_type();
  }
  return rc;
}

grn_rc Expression::filter(
  Record *input, size_t input_size,
  Record *output, size_t *output_size) {
  if ((!input && (input_size != 0)) ||
      ((output > input) && (output < (input + input_size))) || !output_size) {
    return GRN_INVALID_ARGUMENT;
  }
  ExpressionNode *root = this->root();
  if (!root) {
    return GRN_UNKNOWN_ERROR;
  }
  if (!output) {
    output = input;
  }
  size_t total_output_size = 0;
  while (input_size > 0) {
    size_t batch_input_size = GRN_TS_MAX_BATCH_SIZE;
    if (input_size < batch_input_size) {
      batch_input_size = input_size;
    }
    size_t batch_output_size;
    grn_rc rc = root->filter(
      input, batch_input_size, output, &batch_output_size);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    input += batch_input_size;
    input_size -= batch_input_size;
    output += batch_output_size;
    total_output_size += batch_output_size;
  }
  *output_size = total_output_size;
  return GRN_SUCCESS;
}

grn_rc Expression::adjust(Record *records, size_t num_records) {
  if (!records && (num_records != 0)) {
    return GRN_INVALID_ARGUMENT;
  }
  ExpressionNode *root = this->root();
  if (!root) {
    return GRN_UNKNOWN_ERROR;
  }
  while (num_records > 0) {
    size_t batch_size = GRN_TS_MAX_BATCH_SIZE;
    if (num_records < batch_size) {
      batch_size = num_records;
    }
    grn_rc rc = root->adjust(records, batch_size);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    records += batch_size;
    num_records -= batch_size;
  }
  return GRN_SUCCESS;
}

grn_rc Expression::evaluate(const Record *records, size_t num_records,
                            void *results) {
  if ((!records || !results) && (num_records != 0)) {
    return GRN_INVALID_ARGUMENT;
  }
  ExpressionNode *root = this->root();
  if (!root) {
    return GRN_UNKNOWN_ERROR;
  }
  // FIXME: Records should be processed per block.
  //        However, the contents of old blocks will be lost.
  return root->evaluate(records, num_records, results);
}

ExpressionNode *Expression::root() const {
  if (stack_.size() != 1) {
    return NULL;
  }
  return stack_.front();
}

void Expression::update_type() {
  ExpressionNode *root = this->root();
  if (!root) {
    type_ = GRN_TS_INCOMPLETE;
  } else {
    switch (root->type()) {
      case GRN_TS_ID_NODE: {
        type_ = GRN_TS_ID;
        break;
      }
      case GRN_TS_SCORE_NODE: {
        type_ = GRN_TS_SCORE;
        break;
      }
      case GRN_TS_CONSTANT_NODE: {
        type_ = GRN_TS_CONSTANT;
        break;
      }
      case GRN_TS_COLUMN_NODE:
      case GRN_TS_OPERATOR_NODE: {
        type_ = GRN_TS_VARIABLE;
        break;
      }
      default: {
        type_ = GRN_TS_INCOMPLETE;
        break;
      }
    }
    output_type_ = data_type();
  }
}

grn_rc Expression::push_constant_object(grn_obj *obj) {
  ExpressionNode *node;
  grn_rc rc = ConstantNode::open(ctx_, obj, &node);
  if (rc == GRN_SUCCESS) try {
    stack_.push_back(node);
  } catch (const std::bad_alloc &) {
    delete node;
    return GRN_NO_MEMORY_AVAILABLE;
  }
  return rc;
}

grn_rc Expression::push_column_object(grn_obj *obj) {
  if ((obj->header.type == GRN_COLUMN_FIX_SIZE) ||
      (obj->header.type == GRN_COLUMN_VAR_SIZE)) {
    grn_obj *owner_table = grn_column_table(ctx_, obj);
    if (owner_table != table_) {
      return GRN_INVALID_ARGUMENT;
    }
  }
  ExpressionNode *node;
  grn_rc rc = ColumnNode::open(ctx_, obj, &node);
  if (rc == GRN_SUCCESS) try {
    stack_.push_back(node);
  } catch (const std::bad_alloc &) {
    delete node;
    return GRN_NO_MEMORY_AVAILABLE;
  }
  return rc;
}

grn_rc Expression::create_unary_node(OperatorType operator_type,
  ExpressionNode *arg, ExpressionNode **node) {
  grn_rc rc = GRN_SUCCESS;
  switch (operator_type) {
    case GRN_TS_LOGICAL_NOT: {
      if (arg->data_kind() != GRN_TS_BOOL) {
        return GRN_UNKNOWN_ERROR;
      }
      rc = LogicalNotNode::open(arg, node);
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  return rc;
}

grn_rc Expression::create_binary_node(OperatorType operator_type,
  ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node) {
  switch (operator_type) {
    case GRN_TS_LOGICAL_AND: {
      if ((arg1->data_kind() != GRN_TS_BOOL) ||
          (arg1->data_kind() != GRN_TS_BOOL)) {
        return GRN_INVALID_FORMAT;
      }
      return LogicalAndNode::open(arg1, arg2, node);
    }
//    case GRN_TS_LOGICAL_OR: {
//      if ((arg1->data_kind() != GRN_TS_BOOL) ||
//          (arg1->data_kind() != GRN_TS_BOOL)) {
//        return GRN_INVALID_FORMAT;
//      }
//      return LogicalOrNode::open(arg1, arg2, node);
//    }
//    case GRN_TS_EQUAL: {
//      if (arg1->data_kind() != arg2->data_kind()) {
//        return GRN_INVALID_FORMAT;
//      }
//      switch (arg1->data_kind()) {
//        case GRN_TS_BOOL: {
//          return equal_node_open(EqualOperator<Bool>(), arg1, arg2, node);
//        }
//        case GRN_TS_INT: {
//          return equal_node_open(EqualOperator<Int>(), arg1, arg2, node);
//        }
//        case GRN_TS_FLOAT: {
//          return equal_node_open(EqualOperator<Float>(), arg1, arg2, node);
//        }
//        case GRN_TS_TIME: {
//          return equal_node_open(EqualOperator<Time>(), arg1, arg2, node);
//        }
//        case GRN_TS_TEXT: {
//          return equal_node_open(EqualOperator<Text>(), arg1, arg2, node);
//        }
//        case GRN_TS_GEO_POINT: {
//          return equal_node_open(EqualOperator<GeoPoint>(), arg1, arg2, node);
//        }
//        default: {
//          return GRN_UNKNOWN_ERROR;
//        }
//      }
//    }
//    case GRN_TS_NOT_EQUAL: {
//      if (arg1->data_kind() != arg2->data_kind()) {
//        return GRN_INVALID_FORMAT;
//      }
//      switch (arg1->data_kind()) {
//        case GRN_TS_BOOL: {
//          return not_equal_node_open(
//            NotEqualOperator<Bool>(), arg1, arg2, node);
//        }
//        case GRN_TS_INT: {
//          return not_equal_node_open(
//            NotEqualOperator<Int>(), arg1, arg2, node);
//        }
//        case GRN_TS_FLOAT: {
//          return not_equal_node_open(
//            NotEqualOperator<Float>(), arg1, arg2, node);
//        }
//        case GRN_TS_TIME: {
//          return not_equal_node_open(
//            NotEqualOperator<Time>(), arg1, arg2, node);
//        }
//        case GRN_TS_TEXT: {
//          return not_equal_node_open(
//            NotEqualOperator<Text>(), arg1, arg2, node);
//        }
//        case GRN_TS_GEO_POINT: {
//          return not_equal_node_open(
//            NotEqualOperator<GeoPoint>(), arg1, arg2, node);
//        }
//        default: {
//          return GRN_UNKNOWN_ERROR;
//        }
//      }
//    }
//    case GRN_TS_LESS: {
//      if (arg1->data_kind() != arg2->data_kind()) {
//        return GRN_INVALID_FORMAT;
//      }
//      switch (arg1->data_kind()) {
//        case GRN_TS_INT: {
//          return less_node_open(LessOperator<Int>(), arg1, arg2, node);
//        }
//        case GRN_TS_FLOAT: {
//          return less_node_open(LessOperator<Float>(), arg1, arg2, node);
//        }
//        case GRN_TS_TIME: {
//          return less_node_open(LessOperator<Time>(), arg1, arg2, node);
//        }
//        case GRN_TS_TEXT: {
//          return less_node_open(LessOperator<Text>(), arg1, arg2, node);
//        }
//        default: {
//          return GRN_UNKNOWN_ERROR;
//        }
//      }
//    }
//    case GRN_TS_LESS_EQUAL: {
//      if (arg1->data_kind() != arg2->data_kind()) {
//        return GRN_INVALID_FORMAT;
//      }
//      switch (arg1->data_kind()) {
//        case GRN_TS_INT: {
//          return less_equal_node_open(
//            LessEqualOperator<Int>(), arg1, arg2, node);
//        }
//        case GRN_TS_FLOAT: {
//          return less_equal_node_open(
//            LessEqualOperator<Float>(), arg1, arg2, node);
//        }
//        case GRN_TS_TIME: {
//          return less_equal_node_open(
//            LessEqualOperator<Time>(), arg1, arg2, node);
//        }
//        case GRN_TS_TEXT: {
//          return less_equal_node_open(
//            LessEqualOperator<Text>(), arg1, arg2, node);
//        }
//        default: {
//          return GRN_UNKNOWN_ERROR;
//        }
//      }
//    }
//    case GRN_TS_GREATER: {
//      if (arg1->data_kind() != arg2->data_kind()) {
//        return GRN_INVALID_FORMAT;
//      }
//      switch (arg1->data_kind()) {
//        case GRN_TS_INT: {
//          return greater_node_open(GreaterOperator<Int>(), arg1, arg2, node);
//        }
//        case GRN_TS_FLOAT: {
//          return greater_node_open(GreaterOperator<Float>(), arg1, arg2, node);
//        }
//        case GRN_TS_TIME: {
//          return greater_node_open(GreaterOperator<Time>(), arg1, arg2, node);
//        }
//        case GRN_TS_TEXT: {
//          return greater_node_open(GreaterOperator<Text>(), arg1, arg2, node);
//        }
//        default: {
//          return GRN_UNKNOWN_ERROR;
//        }
//      }
//    }
//    case GRN_TS_GREATER_EQUAL: {
//      if (arg1->data_kind() != arg2->data_kind()) {
//        return GRN_INVALID_FORMAT;
//      }
//      switch (arg1->data_kind()) {
//        case GRN_TS_INT: {
//          return greater_equal_node_open(
//            GreaterEqualOperator<Int>(), arg1, arg2, node);
//        }
//        case GRN_TS_FLOAT: {
//          return greater_equal_node_open(
//            GreaterEqualOperator<Float>(), arg1, arg2, node);
//        }
//        case GRN_TS_TIME: {
//          return greater_equal_node_open(
//            GreaterEqualOperator<Time>(), arg1, arg2, node);
//        }
//        case GRN_TS_TEXT: {
//          return greater_equal_node_open(
//            GreaterEqualOperator<Text>(), arg1, arg2, node);
//        }
//        default: {
//          return GRN_UNKNOWN_ERROR;
//        }
//      }
//    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

}  // namespace ts
}  // namespace grn

#ifdef __cplusplus
extern "C" {
#endif

static grn_rc
grn_ts_select_filter(grn_ctx *ctx, grn_obj *table,
                     const char *filter, size_t filter_size,
                     int offset, int limit,
                     std::vector<grn_ts_record> *records,
                     size_t *num_hits) {
  if (offset < 0) {
    offset = 0;
  }
  if (limit < 0) {
    limit = std::numeric_limits<int>::max();
  }
  grn::ts::Cursor *cursor;
  grn_rc rc = grn::ts::Cursor::open_table_cursor(ctx, table, &cursor);
  if (rc == GRN_SUCCESS) {
    grn::ts::Expression *expression;
    rc = grn::ts::Expression::parse(
      ctx, table, filter, filter_size, &expression);
    if (rc == GRN_SUCCESS) {
      size_t count = 0;
      for ( ; ; ) {
        size_t records_offset = records->size();
        try {
          records->resize(records->size() + GRN_TS_MAX_BATCH_SIZE);
        } catch (const std::bad_alloc &) {
          rc = GRN_NO_MEMORY_AVAILABLE;
          break;
        }
        size_t batch_size;
        rc = cursor->read(&(*records)[records_offset],
                          GRN_TS_MAX_BATCH_SIZE, &batch_size);
        if (rc != GRN_SUCCESS) {
          break;
        }
        if (batch_size == 0) {
          records->resize(records_offset);
          break;
        }
        rc = expression->filter(&(*records)[records_offset], batch_size,
                                NULL, &batch_size);
        if (rc != GRN_SUCCESS) {
          break;
        }
        count += batch_size;
        if (offset > 0) {
          if (offset >= batch_size) {
            offset -= batch_size;
            batch_size = 0;
          } else {
            std::memcpy(&(*records)[0], &(*records)[offset],
                        sizeof(grn_ts_record) * (batch_size - offset));
            batch_size -= offset;
            offset = 0;
          }
        }
        if (limit >= batch_size) {
          limit -= batch_size;
        } else {
          batch_size = limit;
          limit = 0;
        }
        records->resize(records_offset + batch_size);
      }
      delete expression;
      *num_hits = count;
    }
    delete cursor;
  }
  return rc;
}

static grn_rc
grn_ts_select_output(grn_ctx *ctx, grn_obj *table,
                     const char *output_columns, size_t output_columns_size,
                     const grn_ts_record *records, size_t num_records,
                     size_t num_hits) {
  grn_rc rc = GRN_SUCCESS;
  std::vector<std::string> names;
  std::vector<grn::ts::Expression *> expressions;

  const char *rest = output_columns;
  size_t rest_size = output_columns_size;
  while (rest_size != 0) {
    size_t pos;
    for (pos = 0; pos < rest_size; ++pos) {
      if ((rest[pos] != ',') &&
          !std::isspace(static_cast<unsigned char>(rest[pos]))) {
        break;
      }
    }
    if (pos >= rest_size) {
      break;
    }
    rest += pos;
    rest_size -= pos;
    for (pos = 0; pos < rest_size; ++pos) {
      if ((rest[pos] == ',') ||
          std::isspace(static_cast<unsigned char>(rest[pos]))) {
        break;
      }
    }
    // TODO: Error handling.
    std::string name(rest, pos);
    if (name == "*") {
      grn_hash *columns;
      if ((columns = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                     GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, table, "", 0, (grn_obj *)columns)) {
          grn_id *key;
          GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
            grn_obj *column = grn_ctx_at(ctx, *key);
            if (column) {
              char name_buf[1024];
              int name_size = grn_column_name(
                ctx, column, name_buf, sizeof(name_buf));
              grn::ts::Expression *expression;
              grn_rc r = grn::ts::Expression::open(ctx, table, &expression);
              if (r == GRN_SUCCESS) {
                r = expression->push_object(column);
                if (r == GRN_SUCCESS) {
                  names.push_back(std::string(name_buf, name_size));
                  expressions.push_back(expression);
                }
              }
            }
          });
        }
        grn_hash_close(ctx, columns);
      }
    } else {
      grn::ts::Expression *expression;
      grn_rc r = grn::ts::Expression::parse(
        ctx, table, rest, pos, &expression);
      if (r == GRN_SUCCESS) {
        names.push_back(name);
        expressions.push_back(expression);
      }
    }
    if (pos >= rest_size) {
      break;
    }
    rest += pos + 1;
    rest_size -= pos + 1;
  }

  GRN_OUTPUT_ARRAY_OPEN("RESULT", 1);
  GRN_OUTPUT_ARRAY_OPEN("RESULTSET", 2 + num_records);
  GRN_OUTPUT_ARRAY_OPEN("NHITS", 1);
  grn_text_ulltoa(ctx, ctx->impl->outbuf, num_hits);
  GRN_OUTPUT_ARRAY_CLOSE();  // NHITS.
  GRN_OUTPUT_ARRAY_OPEN("COLUMNS", expressions.size());
  for (size_t i = 0; i < expressions.size(); ++i) {
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, names[i].data(), names[i].size());
    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, "\",\"", 3);
    switch (expressions[i]->output_type()) {
      case GRN_DB_BOOL: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Bool");
        break;
      }
      case GRN_DB_INT8: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Int8");
        break;
      }
      case GRN_DB_INT16: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Int16");
        break;
      }
      case GRN_DB_INT32: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Int32");
        break;
      }
      case GRN_DB_INT64: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Int64");
        break;
      }
      case GRN_DB_UINT8: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "UInt8");
        break;
      }
      case GRN_DB_UINT16: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "UInt16");
        break;
      }
      case GRN_DB_UINT32: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "UInt32");
        break;
      }
      case GRN_DB_UINT64: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "UInt64");
        break;
      }
      case GRN_DB_FLOAT: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Float");
        break;
      }
      case GRN_DB_TIME: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Time");
        break;
      }
      case GRN_DB_SHORT_TEXT: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "ShortText");
        break;
      }
      case GRN_DB_TEXT: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Text");
        break;
      }
      case GRN_DB_LONG_TEXT: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "LongText");
        break;
      }
      case GRN_DB_TOKYO_GEO_POINT: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "TokyoGeoPoint");
        break;
      }
      case GRN_DB_WGS84_GEO_POINT: {
        GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "WGS84GeoPoint");
        break;
      }
      default: {
        grn_obj *obj = grn_ctx_at(ctx, expressions[i]->output_type());
        if (obj && grn_obj_is_table(ctx, obj)) {
          char name[GRN_TABLE_MAX_KEY_SIZE];
          int len = grn_obj_name(ctx, obj, name, sizeof(name));
          GRN_TEXT_PUT(ctx, ctx->impl->outbuf, name, len);
          grn_obj_unlink(ctx, obj);
        }
        break;
      }
    }
    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
    GRN_OUTPUT_ARRAY_CLOSE();
  }
  GRN_OUTPUT_ARRAY_CLOSE();  // COLUMNS.
  if (num_records != 0) {
    size_t count = 0;
    std::vector<std::vector<char> > bufs(expressions.size());
    while (count < num_records) {
      size_t batch_size = GRN_TS_MAX_BATCH_SIZE;
      if (batch_size > (num_records - count)) {
        batch_size = num_records - count;
      }
      for (size_t i = 0; i < expressions.size(); ++i) {
        if (expressions[i]->dimension() == 0) {
          switch (expressions[i]->data_kind()) {
            case GRN_TS_BOOL: {
              bufs[i].resize(sizeof(grn_ts_bool) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn::ts::Bool *)&bufs[i][0]);
              break;
            }
            case GRN_TS_INT: {
              bufs[i].resize(sizeof(grn_ts_int) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn::ts::Int *)&bufs[i][0]);
              break;
            }
            case GRN_TS_FLOAT: {
              bufs[i].resize(sizeof(grn_ts_float) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn::ts::Float *)&bufs[i][0]);
              break;
            }
            case GRN_TS_TIME: {
              bufs[i].resize(sizeof(grn_ts_time) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn::ts::Time *)&bufs[i][0]);
              break;
            }
            case GRN_TS_TEXT: {
              bufs[i].resize(sizeof(grn_ts_text) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn::ts::Text *)&bufs[i][0]);
              break;
            }
            case GRN_TS_GEO_POINT: {
              bufs[i].resize(sizeof(grn_ts_geo_point) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn::ts::GeoPoint *)&bufs[i][0]);
              break;
            }
            default: {
              break;
            }
          }
        } else {
          switch (expressions[i]->data_kind()) {
            case GRN_TS_BOOL: {
              bufs[i].resize(sizeof(grn_ts_vector) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn_ts_vector *)&bufs[i][0]);
              break;
            }
            case GRN_TS_INT: {
              bufs[i].resize(sizeof(grn_ts_vector) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn_ts_vector *)&bufs[i][0]);
              break;
            }
            case GRN_TS_FLOAT: {
              bufs[i].resize(sizeof(grn_ts_vector) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn_ts_vector *)&bufs[i][0]);
              break;
            }
            case GRN_TS_TIME: {
              bufs[i].resize(sizeof(grn_ts_vector) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn_ts_vector *)&bufs[i][0]);
              break;
            }
            case GRN_TS_TEXT: {
              bufs[i].resize(sizeof(grn_ts_vector) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn_ts_vector *)&bufs[i][0]);
              break;
            }
            case GRN_TS_GEO_POINT: {
              bufs[i].resize(sizeof(grn_ts_vector) * batch_size);
              expressions[i]->evaluate(records + count, batch_size,
                                       (grn_ts_vector *)&bufs[i][0]);
              break;
            }
            default: {
              break;
            }
          }
        }
      }
      for (size_t i = 0; i < batch_size; ++i) {
        GRN_OUTPUT_ARRAY_OPEN("HIT", expressions.size());
        for (size_t j = 0; j < expressions.size(); ++j) {
          if (j != 0) {
            GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
          }
          if (expressions[j]->dimension() == 0) {
            switch (expressions[j]->data_kind()) {
              case GRN_TS_BOOL: {
                if (((grn_ts_bool *)&bufs[j][0])[i]) {
                  GRN_TEXT_PUT(ctx, ctx->impl->outbuf, "true", 4);
                } else {
                  GRN_TEXT_PUT(ctx, ctx->impl->outbuf, "false", 5);
                }
                break;
              }
              case GRN_TS_INT: {
                grn_text_lltoa(ctx, ctx->impl->outbuf,
                               ((grn_ts_int *)&bufs[j][0])[i]);
                break;
              }
              case GRN_TS_FLOAT: {
                grn_text_ftoa(ctx, ctx->impl->outbuf,
                              ((grn_ts_float *)&bufs[j][0])[i]);
                break;
              }
              case GRN_TS_TIME: {
                grn_text_ftoa(ctx, ctx->impl->outbuf,
                              ((grn_ts_time *)&bufs[j][0])[i] * 0.000001);
                break;
              }
              case GRN_TS_TEXT: {
                grn_ts_text text = ((grn_ts_text *)&bufs[j][0])[i];
                grn_text_esc(ctx, ctx->impl->outbuf, text.ptr, text.size);
                break;
              }
              case GRN_TS_GEO_POINT: {
                grn_ts_geo_point geo_point =
                  ((grn_ts_geo_point *)&bufs[j][0])[i];
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
                grn_text_itoa(ctx, ctx->impl->outbuf, geo_point.latitude);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, 'x');
                grn_text_itoa(ctx, ctx->impl->outbuf, geo_point.longitude);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
                break;
              }
              default: {
                break;
              }
            }
          } else {
            grn_ts_vector vector =
              reinterpret_cast<grn_ts_vector *>(&bufs[j][0])[i];
            switch (expressions[j]->data_kind()) {
              case GRN_TS_BOOL: {
                const grn_ts_bool *ptr =
                  static_cast<const grn_ts_bool *>(vector.ptr);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '[');
                for (size_t k = 0; k < vector.size; ++k) {
                  if (k != 0) {
                    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
                  }
                  if (ptr[k]) {
                    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, "true", 4);
                  } else {
                    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, "false", 5);
                  }
                }
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ']');
                break;
              }
              case GRN_TS_INT: {
                const grn_ts_int *ptr =
                  static_cast<const grn_ts_int *>(vector.ptr);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '[');
                for (size_t k = 0; k < vector.size; ++k) {
                  if (k != 0) {
                    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
                  }
                  grn_text_lltoa(ctx, ctx->impl->outbuf, ptr[k]);
                }
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ']');
                break;
              }
              case GRN_TS_FLOAT: {
                const grn_ts_float *ptr =
                  static_cast<const grn_ts_float *>(vector.ptr);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '[');
                for (size_t k = 0; k < vector.size; ++k) {
                  if (k != 0) {
                    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
                  }
                  grn_text_ftoa(ctx, ctx->impl->outbuf, ptr[k]);
                }
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ']');
                break;
              }
              case GRN_TS_TIME: {
                const grn_ts_time *ptr =
                  static_cast<const grn_ts_time *>(vector.ptr);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '[');
                for (size_t k = 0; k < vector.size; ++k) {
                  if (k != 0) {
                    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
                  }
                  grn_text_ftoa(ctx, ctx->impl->outbuf, ptr[k] * 0.000001);
                }
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ']');
                break;
              }
              case GRN_TS_TEXT: {
                const grn_ts_text *ptr =
                  static_cast<const grn_ts_text *>(vector.ptr);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '[');
                for (size_t k = 0; k < vector.size; ++k) {
                  if (k != 0) {
                    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
                  }
                  grn_text_esc(ctx, ctx->impl->outbuf, ptr[k].ptr, ptr[k].size);
                }
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ']');
                break;
              }
              case GRN_TS_GEO_POINT: {
                const grn_ts_geo_point *ptr =
                  static_cast<const grn_ts_geo_point *>(vector.ptr);
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '[');
                for (size_t k = 0; k < vector.size; ++k) {
                  if (k != 0) {
                    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
                  }
                  GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
                  grn_text_itoa(ctx, ctx->impl->outbuf, ptr[k].latitude);
                  GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, 'x');
                  grn_text_itoa(ctx, ctx->impl->outbuf, ptr[k].longitude);
                  GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
                }
                GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ']');
                break;
              }
              default: {
                break;
              }
            }
          }
        }
        GRN_OUTPUT_ARRAY_CLOSE();  // HITS.
      }
      count += batch_size;
    }
  }
  GRN_OUTPUT_ARRAY_CLOSE();  // RESULTSET.
  GRN_OUTPUT_ARRAY_CLOSE();  // RESET.
  for (size_t i = 0; i < expressions.size(); ++i) {
    delete expressions[i];
  }
  return rc;
}

grn_rc
grn_ts_select(grn_ctx *ctx, grn_obj *table,
              const char *filter, size_t filter_size,
              const char *output_columns, size_t output_columns_size,
              int offset, int limit) {
  if (!ctx || !grn_obj_is_table(ctx, table) ||
      (!filter && (filter_size != 0)) ||
      (!output_columns && (output_columns_size != 0))) {
    return GRN_INVALID_ARGUMENT;
  }
  std::vector<grn_ts_record> records;
  size_t num_hits;
  grn_rc rc = grn_ts_select_filter(ctx, table, filter, filter_size,
                                   offset, limit, &records, &num_hits);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_select_output(ctx, table, output_columns, output_columns_size,
                              &*records.begin(), records.size(), num_hits);
  }
  return rc;
}

#ifdef __cplusplus
}
#endif

#endif  // GRN_WITH_TS
