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

#pragma once

#include "grn.h"
#include "grn_load.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_arrow_stream_loader grn_arrow_stream_loader;

grn_arrow_stream_loader *
grn_arrow_stream_loader_open(grn_ctx *ctx, grn_loader *loader);
grn_rc
grn_arrow_stream_loader_close(grn_ctx *ctx, grn_arrow_stream_loader *loader);
grn_rc
grn_arrow_stream_loader_consume(grn_ctx *ctx,
                                grn_arrow_stream_loader *loader,
                                const char *data,
                                size_t data_size);

typedef struct _grn_arrow_stream_writer grn_arrow_stream_writer;
grn_arrow_stream_writer *
grn_arrow_stream_writer_open(grn_ctx *ctx, grn_obj *output_buffer);
grn_rc
grn_arrow_stream_writer_close(grn_ctx *ctx, grn_arrow_stream_writer *writer);
grn_rc
grn_arrow_stream_writer_add_field(grn_ctx *ctx,
                                  grn_arrow_stream_writer *writer,
                                  const char *name,
                                  grn_obj *column);
grn_rc
grn_arrow_stream_writer_add_field_text_dictionary(
  grn_ctx *ctx,
  grn_arrow_stream_writer *writer,
  const char *name,
  grn_obj *index_type);
grn_rc
grn_arrow_stream_writer_add_field_union(grn_ctx *ctx,
                                        grn_arrow_stream_writer *writer,
                                        const char *name,
                                        grn_obj **columns,
                                        size_t n_columns);
grn_rc
grn_arrow_stream_writer_add_metadata(grn_ctx *ctx,
                                     grn_arrow_stream_writer *writer,
                                     const char *name,
                                     const char *value);
grn_rc
grn_arrow_stream_writer_write_schema(grn_ctx *ctx,
                                     grn_arrow_stream_writer *writer);
grn_rc
grn_arrow_stream_writer_open_record(grn_ctx *ctx,
                                    grn_arrow_stream_writer *writer);
grn_rc
grn_arrow_stream_writer_close_record(grn_ctx *ctx,
                                     grn_arrow_stream_writer *writer);
grn_rc
grn_arrow_stream_writer_add_column_null(grn_ctx *ctx,
                                        grn_arrow_stream_writer *writer);
grn_rc
grn_arrow_stream_writer_add_column_text(grn_ctx *ctx,
                                        grn_arrow_stream_writer *writer,
                                        const char *value,
                                        size_t value_length);
grn_rc
grn_arrow_stream_writer_add_column_text_dictionary(
  grn_ctx *ctx,
  grn_arrow_stream_writer *writer,
  const char *value,
  size_t value_length);
grn_rc
grn_arrow_stream_writer_add_column_int8(grn_ctx *ctx,
                                        grn_arrow_stream_writer *writer,
                                        int8_t value);
grn_rc
grn_arrow_stream_writer_add_column_uint16(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          uint16_t value);
grn_rc
grn_arrow_stream_writer_add_column_int32(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         int32_t value);
grn_rc
grn_arrow_stream_writer_add_column_uint32(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          uint32_t value);
grn_rc
grn_arrow_stream_writer_add_column_int64(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         int64_t value);
grn_rc
grn_arrow_stream_writer_add_column_uint64(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          uint64_t value);
grn_rc
grn_arrow_stream_writer_add_column_float32(grn_ctx *ctx,
                                           grn_arrow_stream_writer *writer,
                                           float value);
grn_rc
grn_arrow_stream_writer_add_column_float(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         double value);
grn_rc
grn_arrow_stream_writer_add_column_timestamp(grn_ctx *ctx,
                                             grn_arrow_stream_writer *writer,
                                             grn_timeval value);
grn_rc
grn_arrow_stream_writer_add_column_record(grn_ctx *ctx,
                                          grn_arrow_stream_writer *writer,
                                          grn_obj *record);
grn_rc
grn_arrow_stream_writer_add_column_uvector(grn_ctx *ctx,
                                           grn_arrow_stream_writer *writer,
                                           grn_obj *uvector);
grn_rc
grn_arrow_stream_writer_add_column_union(grn_ctx *ctx,
                                         grn_arrow_stream_writer *writer,
                                         int8_t type);
grn_rc
grn_arrow_stream_writer_flush(grn_ctx *ctx, grn_arrow_stream_writer *writer);

#ifdef __cplusplus
}
#endif
