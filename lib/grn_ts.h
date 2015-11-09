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

#include <stddef.h>

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------
 * Built-in data types.
 */

/* ID (_id). */
typedef grn_id grn_ts_id;

/* Score (_score). */
typedef float grn_ts_score;

/* Record (_id, _score). */
typedef struct {
  grn_ts_id id;
  grn_ts_score score;
} grn_ts_record;

/* Built-in scalar data types. */

/* Bool. */
typedef grn_bool grn_ts_bool;

/* Int. */
typedef int64_t grn_ts_int;

/* Float. */
typedef double grn_ts_float;

/* Time. */
typedef int64_t grn_ts_time;

/* Text. */
typedef struct {
  const char *ptr;
  size_t size;
} grn_ts_text;

/* GeoPoint. */
typedef grn_geo_point grn_ts_geo_point;
typedef grn_geo_point grn_ts_tokyo_geo_point;
typedef grn_geo_point grn_ts_wgs84_geo_point;

/* Ref. */
typedef grn_ts_record grn_ts_ref;

/* Built-in vector data types. */

/* BoolVector. */
typedef struct {
  const grn_ts_bool *ptr;
  size_t size;
} grn_ts_bool_vector;

/* IntVector. */
typedef struct {
  const grn_ts_int *ptr;
  size_t size;
} grn_ts_int_vector;

/* FloatVector. */
typedef struct {
  const grn_ts_float *ptr;
  size_t size;
} grn_ts_float_vector;

/* TimeVector. */
typedef struct {
  const grn_ts_time *ptr;
  size_t size;
} grn_ts_time_vector;

/* TextVector. */
typedef struct {
  const grn_ts_text *ptr;
  size_t size;
} grn_ts_text_vector;

/* GeoPointVector. */
typedef struct {
  const grn_ts_geo_point *ptr;
  size_t size;
} grn_ts_geo_point_vector;
typedef grn_ts_geo_point_vector grn_ts_tokyo_geo_point_vector;
typedef grn_ts_geo_point_vector grn_ts_wgs84_geo_point_vector;

/* RefVector. */
typedef struct {
  const grn_ts_ref *ptr;
  size_t size;
} grn_ts_ref_vector;

/*-------------------------------------------------------------
 * API.
 */

/*
 * grn_ts_select() finds records passing through a filter and writes the values
 * of output columns (the evaluation results of output expressions) into the
 * output buffer (`ctx->impl->outbuf`).
 *
 * Note that the first `offset` records will be discarded and at most `limit`
 * records will be output.
 *
 * On success, grn_ts_select() returns GRN_SUCCESS.
 * On failure, grn_ts_select() returns an error code and set the details into
 * `ctx`.
 */
grn_rc grn_ts_select(grn_ctx *ctx, grn_obj *table,
                     const char *filter_ptr, size_t filter_size,
                     const char *output_columns_ptr,
                     size_t output_columns_size,
                     size_t offset, size_t limit);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_H */
