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

#ifndef GRN_TS_HPP
#define GRN_TS_HPP

#include <cstring>
#include <vector>

#include "grn_ts.h"

namespace grn {
namespace ts {

// Constant values.

typedef grn_ts_operator_type OperatorType;
typedef grn_ts_data_type DataType;
typedef grn_ts_data_kind DataKind;

typedef grn_ts_expression_node_type ExpressionNodeType;
typedef grn_ts_expression_type ExpressionType;

// Built-in data types.

typedef grn_ts_id ID;
typedef grn_ts_score Score;
typedef grn_ts_record Record;

struct Bool {
  typedef grn_ts_bool Raw;
  Raw raw;

  Bool() : raw() {}
  Bool(const Bool &value) : raw(value.raw) {}
  explicit Bool(Raw value) : raw(value) {}
  ~Bool() {}
};

inline bool operator!(Bool value) { return !value.raw; }
inline bool operator==(Bool lhs, Bool rhs) { return lhs.raw == rhs.raw; }
inline bool operator!=(Bool lhs, Bool rhs) { return lhs.raw != rhs.raw; }

struct Int {
  typedef grn_ts_int Raw;
  Raw raw;

  Int() : raw() {}
  Int(const Int &value) : raw(value.raw) {}
  explicit Int(Raw value) : raw(value) {}
  ~Int() {}
};

inline bool operator==(Int lhs, Int rhs) { return lhs.raw == rhs.raw; }
inline bool operator!=(Int lhs, Int rhs) { return lhs.raw != rhs.raw; }
inline bool operator<(Int lhs, Int rhs) { return lhs.raw < rhs.raw; }
inline bool operator<=(Int lhs, Int rhs) { return lhs.raw <= rhs.raw; }
inline bool operator>(Int lhs, Int rhs) { return lhs.raw > rhs.raw; }
inline bool operator>=(Int lhs, Int rhs) { return lhs.raw >= rhs.raw; }

struct Float {
  typedef grn_ts_float Raw;
  Raw raw;

  Float() : raw() {}
  Float(const Float &value) : raw(value.raw) {}
  explicit Float(Raw value) : raw(value) {}
  ~Float() {}
};

inline bool operator==(Float lhs, Float rhs) {
  return (lhs.raw <= rhs.raw) && (lhs.raw >= rhs.raw);
}
inline bool operator!=(Float lhs, Float rhs) {
  return (lhs.raw < rhs.raw) || (lhs.raw > rhs.raw);
}
inline bool operator<(Float lhs, Float rhs) { return lhs.raw < rhs.raw; }
inline bool operator<=(Float lhs, Float rhs) { return lhs.raw <= rhs.raw; }
inline bool operator>(Float lhs, Float rhs) { return lhs.raw > rhs.raw; }
inline bool operator>=(Float lhs, Float rhs) { return lhs.raw >= rhs.raw; }

struct Time {
  typedef grn_ts_time Raw;
  Raw raw;

  Time() : raw() {}
  Time(const Time &value) : raw(value.raw) {}
  explicit Time(Raw value) : raw(value) {}
  ~Time() {}
};

inline bool operator==(Time lhs, Time rhs) { return lhs.raw == rhs.raw; }
inline bool operator!=(Time lhs, Time rhs) { return lhs.raw != rhs.raw; }
inline bool operator<(Time lhs, Time rhs) { return lhs.raw < rhs.raw; }
inline bool operator<=(Time lhs, Time rhs) { return lhs.raw <= rhs.raw; }
inline bool operator>(Time lhs, Time rhs) { return lhs.raw > rhs.raw; }
inline bool operator>=(Time lhs, Time rhs) { return lhs.raw >= rhs.raw; }

struct Text {
  typedef grn_ts_text Raw;
  Raw raw;

  Text() : raw() {}
  Text(const Text &value) : raw(value.raw) {}
  explicit Text(const Raw &value) : raw(value) {}
  Text(const char *ptr, size_t size) : raw((Raw){ptr, size}) {}
  ~Text() {}
};

inline bool operator==(const Text &lhs, const Text &rhs) {
  if (lhs.raw.size != rhs.raw.size) {
    return false;
  }
  return std::memcmp(lhs.raw.ptr, rhs.raw.ptr, lhs.raw.size) == 0;
}
inline bool operator!=(const Text &lhs, const Text &rhs) {
  if (lhs.raw.size != rhs.raw.size) {
    return true;
  }
  return std::memcmp(lhs.raw.ptr, rhs.raw.ptr, lhs.raw.size) != 0;
}
inline bool operator<(const Text &lhs, const Text &rhs) {
  size_t min_size = (lhs.raw.size < rhs.raw.size) ?
                    lhs.raw.size : rhs.raw.size;
  int result = std::memcmp(lhs.raw.ptr, rhs.raw.ptr, min_size);
  if (result == 0) {
    return lhs.raw.size < rhs.raw.size;
  }
  return result < 0;
}
inline bool operator<=(const Text &lhs, const Text &rhs) {
  size_t min_size = (lhs.raw.size < rhs.raw.size) ?
                    lhs.raw.size : rhs.raw.size;
  int result = std::memcmp(lhs.raw.ptr, rhs.raw.ptr, min_size);
  if (result == 0) {
    return lhs.raw.size <= rhs.raw.size;
  }
  return result <= 0;
}
inline bool operator>(const Text &lhs, const Text &rhs) { return rhs < lhs; }
inline bool operator>=(const Text &lhs, const Text &rhs) { return rhs <= lhs; }

struct GeoPoint {
  typedef grn_ts_geo_point Raw;
  Raw raw;

  GeoPoint() : raw() {}
  GeoPoint(const GeoPoint &value) : raw(value.raw) {}
  explicit GeoPoint(Raw value) : raw(value) {}
  GeoPoint(int latitude, int longitude) : raw((Raw){ latitude, longitude }) {}
  ~GeoPoint() {}
};

inline bool operator==(GeoPoint lhs, GeoPoint rhs) {
  return (lhs.raw.latitude == rhs.raw.latitude) &&
         (lhs.raw.longitude == rhs.raw.longitude);
}
inline bool operator!=(GeoPoint lhs, GeoPoint rhs) {
  return (lhs.raw.latitude != rhs.raw.latitude) ||
         (lhs.raw.longitude != rhs.raw.longitude);
}

// Cursor is a base class which provides an interface for sequential access to
// records.
class Cursor {
 public:
  Cursor() {}
  virtual ~Cursor() {}

  // FIXME: Give me options.
  static grn_rc open_table_cursor(grn_ctx *ctx, grn_obj *table,
                                  Cursor **cursor);

  virtual grn_rc read(Record *records, size_t size, size_t *count);
};

// ExpressionNode is an element of Expression.
class ExpressionNode;

// Expression is a class which represents an expression.
class Expression {
 public:
  Expression(grn_ctx *ctx, grn_obj *table);
  ~Expression();

  static grn_rc open(grn_ctx *ctx, grn_obj *table, Expression **expression);
  static grn_rc parse(grn_ctx *ctx, grn_obj *table,
                      const char *query, size_t query_size,
                      Expression **expression);

  ExpressionType type() const {
    return type_;
  }
  DataKind data_kind() const;
  DataType data_type() const;
  DataType output_type() const;
  int dimension() const;

  grn_rc push_object(grn_obj *obj);
  grn_rc push_operator(OperatorType operator_type);

  grn_rc filter(Record *input, size_t input_size,
                Record *output, size_t *output_size);
  grn_rc adjust(Record *records, size_t num_records);

  grn_rc evaluate(const Record *records, size_t num_records, void *results);

 private:
  grn_ctx *ctx_;
  grn_obj *table_;
  ExpressionType type_;
  DataType output_type_;
  std::vector<ExpressionNode *> stack_;

  // Disable copy and assignment.
  Expression(const Expression &);
  Expression &operator=(const Expression &);

  ExpressionNode *root() const;

  void update_type();

  grn_rc push_constant_object(grn_obj *obj);
  grn_rc push_column_object(grn_obj *obj);

  grn_rc create_unary_node(OperatorType operator_type,
    ExpressionNode *arg, ExpressionNode **node);
  grn_rc create_binary_node(OperatorType operator_type,
    ExpressionNode *arg1, ExpressionNode *arg2, ExpressionNode **node);
};

}  // namespace ts
}  // namespace grn

#endif  // GRN_TS_HPP
