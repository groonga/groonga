/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2020-2025  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx.h"

typedef enum grn_compression_type {
  GRN_COMPRESSION_TYPE_NONE,
  GRN_COMPRESSION_TYPE_ZLIB,
  GRN_COMPRESSION_TYPE_LZ4,
  GRN_COMPRESSION_TYPE_ZSTD,
  GRN_COMPRESSION_TYPE_BLOSC,
} grn_compression_type;

typedef struct grn_compress_data {
  /* Input: Start */
  grn_compression_type type;
  void *header;
  uint32_t header_len;
  void *body;
  uint32_t body_len;

  /* Only for GRN_COMPRESSION_TYPE_BLOSC: Start */
  size_t body_n_elements;
  grn_column_flags body_column_flags;
  grn_id body_range;
  /* Only for GRN_COMPRESSION_TYPE_BLOSC: End */

  void *footer;
  uint32_t footer_len;
  /* Input: End */

  /* Output: Start */
  void *packed_value;
  uint32_t packed_value_len;
  void *compressed_value;
  uint32_t compressed_value_len;
  /* Output: End */
} grn_compress_data;

typedef struct grn_decompress_data {
  /* Input */
  grn_compression_type type;
  void *raw_value;
  uint32_t raw_value_len;

  /* Output */
  void *unpacked_value;
  uint32_t unpacked_value_len;
  void *decompressed_value;
  uint32_t decompressed_value_len;

  /* Internal */
  void *compressed_value;
  uint32_t compressed_value_len;
} grn_decompress_data;

/*
 * data->compression_type must not be GRN_COMPRESSION_TYPE_NONE.
 *
 * If GRN_SUCCESS isn't returned, you don't need to free anything.
 *
 * If GRN_SUCCESS is returned:
 *
 * * If data->packed_value_len > 0: You need to free
 *   data->packed_value by GRN_FREE().
 * * Else: You need to free data->compressed_value by GRN_FREE().
 */
grn_rc
grn_compressor_compress(grn_ctx *ctx, grn_compress_data *data);

/*
 * data->compression_type must not be GRN_COMPRESSION_TYPE_NONE.
 *
 * If GRN_SUCCESS isn't returned, you don't need to free anything.
 *
 * If GRN_SUCCESS is returned:
 * * If data->unpacked_value_len > 0: You don't need to free anything.
 * * Else: You need to free data->decompressed_value by GRN_FREE().
 */
grn_rc
grn_compressor_decompress(grn_ctx *ctx, grn_decompress_data *data);
