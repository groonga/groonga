/*
  Copyright (C) 2010-2018  Brazil
  Copyright (C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t grn_output_auto_flush_interval;
void
grn_output_init_from_env(void);

GRN_API void
grn_output_result_set_open_metadata(grn_ctx *ctx,
                                    grn_obj *outbuf,
                                    grn_content_type output_type,
                                    grn_obj *result_set,
                                    grn_obj_format *format,
                                    uint32_t n_additional_elements);
GRN_API void
grn_output_result_set_open(grn_ctx *ctx,
                           grn_obj *outbuf,
                           grn_content_type output_type,
                           grn_obj *result_set,
                           grn_obj_format *format,
                           uint32_t n_additional_elements);
GRN_API void
grn_output_result_set_close(grn_ctx *ctx,
                            grn_obj *outbuf,
                            grn_content_type output_type,
                            grn_obj *result_set,
                            grn_obj_format *format);
GRN_API void
grn_output_result_set(grn_ctx *ctx,
                      grn_obj *outbuf,
                      grn_content_type output_type,
                      grn_obj *result_set,
                      grn_obj_format *format);
GRN_API void
grn_output_table_columns(grn_ctx *ctx,
                         grn_obj *outbuf,
                         grn_content_type output_type,
                         grn_obj *table,
                         grn_obj_format *format);
GRN_API void
grn_output_table_records_open(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              int n_records);
GRN_API void
grn_output_table_records_content(grn_ctx *ctx,
                                 grn_obj *outbuf,
                                 grn_content_type output_type,
                                 grn_obj *table,
                                 grn_obj_format *format);
GRN_API void
grn_output_table_records_close(grn_ctx *ctx,
                               grn_obj *outbuf,
                               grn_content_type output_type);
GRN_API void
grn_output_table_records(grn_ctx *ctx,
                         grn_obj *outbuf,
                         grn_content_type output_type,
                         grn_obj *table,
                         grn_obj_format *format);

grn_rc
grn_obj_format_set_columns(grn_ctx *ctx,
                           grn_obj_format *format,
                           grn_obj *table,
                           const char *columns,
                           unsigned int columns_len);

#define GRN_OUTPUT_ARRAY_OPEN(name, nelements)                                 \
  (grn_ctx_output_array_open(ctx, name, nelements))
#define GRN_OUTPUT_ARRAY_CLOSE() (grn_ctx_output_array_close(ctx))
#define GRN_OUTPUT_MAP_OPEN(name, nelements)                                   \
  (grn_ctx_output_map_open(ctx, name, nelements))
#define GRN_OUTPUT_MAP_CLOSE()   (grn_ctx_output_map_close(ctx))
#define GRN_OUTPUT_NULL()        (grn_ctx_output_null(ctx))
#define GRN_OUTPUT_INT32(value)  (grn_ctx_output_int32(ctx, value))
#define GRN_OUTPUT_INT64(value)  (grn_ctx_output_int64(ctx, value))
#define GRN_OUTPUT_UINT32(value) (grn_ctx_output_uint32(ctx, value))
#define GRN_OUTPUT_UINT64(value) (grn_ctx_output_uint64(ctx, value))
#define GRN_OUTPUT_FLOAT(value)  (grn_ctx_output_float(ctx, value))
#define GRN_OUTPUT_CSTR(value)   (grn_ctx_output_cstr(ctx, value))
#define GRN_OUTPUT_STR(value, value_len)                                       \
  (grn_ctx_output_str(ctx, value, value_len))
#define GRN_OUTPUT_BOOL(value)      (grn_ctx_output_bool(ctx, value))
#define GRN_OUTPUT_OBJ(obj, format) (grn_ctx_output_obj(ctx, obj, format))
#define GRN_OUTPUT_RESULT_SET_OPEN_METADATA(result_set,                        \
                                            format,                            \
                                            n_additional_elements)             \
  (grn_ctx_output_result_set_open_metadata(ctx,                                \
                                           result_set,                         \
                                           format,                             \
                                           n_additional_elements))
#define GRN_OUTPUT_RESULT_SET_OPEN(result_set, format, n_additional_elements)  \
  (grn_ctx_output_result_set_open(ctx,                                         \
                                  result_set,                                  \
                                  format,                                      \
                                  n_additional_elements))
#define GRN_OUTPUT_RESULT_SET_CLOSE(result_set, format)                        \
  (grn_ctx_output_result_set_close(ctx, result_set, format))
#define GRN_OUTPUT_RESULT_SET(result_set, format, n_additional_elements)       \
  (grn_ctx_output_result_set(ctx, result_set, format, n_additional_elements))
#define GRN_OUTPUT_TABLE_COLUMNS(table, format)                                \
  (grn_ctx_output_table_columns(ctx, table, format))
#define GRN_OUTPUT_TABLE_RECORDS(table, format)                                \
  (grn_ctx_output_table_records(ctx, table, format))

#ifdef __cplusplus
}
#endif
