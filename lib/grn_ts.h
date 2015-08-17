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

#ifndef GRN_TS_H
#define GRN_TS_H

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constant values.

typedef enum {
  GRN_TS_VOID,     // GRN_DB_VOID.
  GRN_TS_BOOL,     // GRN_DB_BOOL.
  GRN_TS_INT,      // GRN_DB_(U)INT8/16/32/64.
  GRN_TS_FLOAT,    // GRN_DB_FLOAT.
  GRN_TS_TIME,     // GRN_DB_TIME.
  GRN_TS_TEXT,     // GRN_DB_(SHORT_/LONG_)TEST
  GRN_TS_GEO_POINT // GRN_DB_TOKYO/WGS84_GEO_POINT.
} grn_ts_data_type;

typedef enum {
  GRN_TS_NOP,
  GRN_TS_LOGICAL_NOT,
  GRN_TS_LOGICAL_AND,
  GRN_TS_LOGICAL_OR,
  GRN_TS_EQUAL,
  GRN_TS_NOT_EQUAL,
  GRN_TS_LESS,
  GRN_TS_LESS_EQUAL,
  GRN_TS_GREATER,
  GRN_TS_GREATER_EQUAL
} grn_ts_operator_type;

typedef enum {
  GRN_TS_ID_NODE,
  GRN_TS_SCORE_NODE,
  GRN_TS_CONSTANT_NODE,
  GRN_TS_COLUMN_NODE,
  GRN_TS_OPERATOR_NODE
} grn_ts_expression_node_type;

typedef enum {
  GRN_TS_INCOMPLETE,
  GRN_TS_ID,
  GRN_TS_SCORE,
  GRN_TS_CONSTANT,
  GRN_TS_VARIABLE
} grn_ts_expression_type;

// Built-in data types.

typedef grn_id grn_ts_id;
typedef float grn_ts_score;
typedef struct {
  grn_ts_id id;
  grn_ts_score score;
} grn_ts_record;

typedef grn_bool grn_ts_bool;
typedef int64_t grn_ts_int;
typedef double grn_ts_float;
typedef int64_t grn_ts_time;
typedef struct {
  const char *ptr;
  size_t size;
} grn_ts_text;
typedef grn_geo_point grn_ts_geo_point;
typedef struct {
  const void *ptr;
  size_t size;
} grn_ts_vector;

/*
 * grn_ts_select() finds records passing through a filter (specified by
 * `filter' and `filter_size') and writes the associated values (specified by
 * `output_columns' and `output_columns_size') into the output buffer of `ctx'
 * (`ctx->impl->outbuf').
 *
 * Note that the first `offset` records will be discarded and at most `limit`
 * records will be output.
 *
 * On success, grn_ts_select() returns GRN_SUCCESS.
 * On failure, grn_ts_select() returns an error code and set the details into
 * `ctx`.
 */
grn_rc grn_ts_select(grn_ctx *ctx, grn_obj *table,
                     const char *filter, size_t filter_size,
                     const char *output_columns, size_t output_columns_size,
                     int offset, int limit);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_H */
