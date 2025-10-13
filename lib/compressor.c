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

#include "grn_compressor.h"

#include "grn.h"
#include "grn_ctx.h"

#ifdef GRN_WITH_ZLIB
#  include <zlib.h>
#endif

#ifdef GRN_WITH_LZ4
#  include <lz4.h>
#endif

#ifdef GRN_WITH_ZSTD
#  include <zstd.h>
#endif

#ifdef GRN_WITH_BLOSC
#  include <blosc2.h>
#  include <blosc2/filters-registry.h>

#  define GRN_BLOSC_META_HEADER "header"
#  define GRN_BLOSC_META_FOOTER "footer"
#endif

/*
 * Compressed value uses the following format:
 *
 * +-------------------+------+
 * | uint64_t metadata | data |
 * +-------------------+------+
 *
 * Metadata:
 * * Upper 32bit is flags
 * * Lower 32bit is data length in byte of the original data
 *
 * Available flags:
 *
 * * COMPRESSED_VALUE_META_FLAG_RAW: Data is raw data (not compressed
 *   original data). If raw data is small (< COMPRESS_THRESHOLD_BYTE),
 *   compression is meaningless. So raw data is stored as-is.
 *
 * If COMPRESSED_VALUE_META_FLAG_RAW isn't used, data is compressed
 * data.
 */

#define COMPRESSED_VALUE_LEN(data_len)       (sizeof(uint64_t) + (data_len))
#define COMPRESSED_VALUE_GET_METADATA(value) (*((uint64_t *)(value)))
#define COMPRESSED_VALUE_SET_METADATA(value, metadata)                         \
  *((uint64_t *)(value)) = (metadata)
#define COMPRESSED_VALUE_GET_DATA(value)         ((void *)(((uint64_t *)(value)) + 1))
#define COMPRESSED_VALUE_GET_DATA_LEN(value_len) (value_len - sizeof(uint64_t))

#define COMPRESSED_VALUE_METADATA_PACK(original_len, flags)                    \
  ((original_len) | (flags))
#define COMPRESSED_VALUE_METADATA_FLAG(metadata)                               \
  ((metadata) & 0xf000000000000000)
#define COMPRESSED_VALUE_METADATA_FLAG_RAW 0x1000000000000000
#define COMPRESSED_VALUE_METADATA_ORIGINAL_LEN(metadata)                       \
  ((metadata) & 0x0fffffffffffffff)

#define COMPRESS_THRESHOLD_BYTE 256

/*
 * Create a compressed value format data for small data. It has
 * the COMPRESSED_VALUE_META_FLAG_RAW flag.
 */
static inline grn_rc
grn_compressor_pack(grn_ctx *ctx, grn_compress_data *data)
{
  uint64_t input_len = data->header_len + data->body_len + data->footer_len;
  data->packed_value_len = COMPRESSED_VALUE_LEN(input_len);
  uint64_t packed_value_metadata =
    COMPRESSED_VALUE_METADATA_PACK(input_len,
                                   COMPRESSED_VALUE_METADATA_FLAG_RAW);

  COMPRESSED_VALUE_SET_METADATA(data->packed_value, packed_value_metadata);
  char *packed_value_data = COMPRESSED_VALUE_GET_DATA(data->packed_value);
  size_t offset = 0;
  if (data->header_len > 0) {
    grn_memcpy(packed_value_data + offset, data->header, data->header_len);
    offset += data->header_len;
  }
  if (data->body_len > 0) {
    grn_memcpy(packed_value_data + offset, data->body, data->body_len);
    offset += data->body_len;
  }
  if (data->footer_len > 0) {
    grn_memcpy(packed_value_data + offset, data->footer, data->footer_len);
    offset += data->footer_len;
  }

  return ctx->rc;
}

/*
 * Parse a compressed value format data. If it has the
 * COMPRESSED_VALUE_METADATA_FLAG_RAW flag, data is stored in
 * data->unpacked_value and data->unpacked_value_len. Otherwise, data
 * is stored in data->compressed_value and data->compressed_value_len.
 */
static inline grn_rc
grn_compressor_unpack(grn_ctx *ctx, grn_decompress_data *data)
{
  uint64_t compressed_value_metadata =
    COMPRESSED_VALUE_GET_METADATA(data->raw_value);
  data->compressed_value = COMPRESSED_VALUE_GET_DATA(data->raw_value);
  data->compressed_value_len =
    COMPRESSED_VALUE_GET_DATA_LEN(data->raw_value_len);

  data->decompressed_value_len =
    (uint32_t)COMPRESSED_VALUE_METADATA_ORIGINAL_LEN(compressed_value_metadata);
  if (COMPRESSED_VALUE_METADATA_FLAG(compressed_value_metadata) &
      COMPRESSED_VALUE_METADATA_FLAG_RAW) {
    data->unpacked_value = data->compressed_value;
    data->compressed_value = NULL;
    data->unpacked_value_len = data->compressed_value_len;
    data->compressed_value_len = 0;
    data->decompressed_value_len = 0;
  }

  return ctx->rc;
}

#ifdef GRN_WITH_ZLIB
static inline const char *
grn_zrc_to_string(int zrc)
{
  switch (zrc) {
  case Z_OK:
    return "OK";
  case Z_STREAM_END:
    return "Stream is end";
  case Z_NEED_DICT:
    return "Need dictionary";
  case Z_ERRNO:
    return "See errno";
  case Z_STREAM_ERROR:
    return "Stream error";
  case Z_DATA_ERROR:
    return "Data error";
  case Z_MEM_ERROR:
    return "Memory error";
  case Z_BUF_ERROR:
    return "Buffer error";
  case Z_VERSION_ERROR:
    return "Version error";
  default:
    return "Unknown";
  }
}

static inline grn_rc
grn_compressor_compress_zlib(grn_ctx *ctx, grn_compress_data *data)
{
  const char *tag = "[compressor][compress][zlib] failed to compress:";

  z_stream zstream = {0};
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  int zwindow_bits = 15;
  int zmem_level = 8;
  int zrc = deflateInit2(&zstream,
                         Z_DEFAULT_COMPRESSION,
                         Z_DEFLATED,
                         zwindow_bits,
                         zmem_level,
                         Z_DEFAULT_STRATEGY);
  if (zrc != Z_OK) {
    ERR(GRN_ZLIB_ERROR, "%s initialize: %s", tag, grn_zrc_to_string(zrc));
    return ctx->rc;
  }

  uint64_t input_len = data->header_len + data->body_len + data->footer_len;
  zstream.avail_out = deflateBound(&zstream, input_len);
  data->compressed_value = GRN_MALLOC(COMPRESSED_VALUE_LEN(zstream.avail_out));
  if (!data->compressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_ZLIB_ERROR, "%s allocate buffer: %s", tag, message);
    deflateEnd(&zstream);
    return ctx->rc;
  }
  COMPRESSED_VALUE_SET_METADATA(data->compressed_value,
                                COMPRESSED_VALUE_METADATA_PACK(input_len, 0));
  zstream.next_out = COMPRESSED_VALUE_GET_DATA(data->compressed_value);

  if (data->header_len == 0 && data->footer_len == 0) {
    zstream.next_in = data->body;
    zstream.avail_in = data->body_len;
    zrc = deflate(&zstream, Z_FINISH);
    if (zrc != Z_STREAM_END) {
      ERR(GRN_ZLIB_ERROR, "%s finish: %s", tag, grn_zrc_to_string(zrc));
      deflateEnd(&zstream);
      GRN_FREE(data->compressed_value);
      data->compressed_value = NULL;
      return ctx->rc;
    }
  } else {
    if (data->header_len > 0) {
      zstream.next_in = data->header;
      zstream.avail_in = data->header_len;
      /* TODO: Z_NO_FLUSH here may not be safe. All zstream.next_in
       * may not be processed. */
      zrc = deflate(&zstream, Z_NO_FLUSH);
      if (zrc != Z_OK) {
        ERR(GRN_ZLIB_ERROR, "%s header: %s", tag, grn_zrc_to_string(zrc));
        deflateEnd(&zstream);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }

    if (data->body_len > 0) {
      zstream.next_in = data->body;
      zstream.avail_in = data->body_len;
      /* TODO: Z_NO_FLUSH here may not be safe. All zstream.next_in
       * may not be processed. */
      zrc = deflate(&zstream, Z_NO_FLUSH);
      if (zrc != Z_OK) {
        ERR(GRN_ZLIB_ERROR, "%s body: %s", tag, grn_zrc_to_string(zrc));
        deflateEnd(&zstream);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }

    if (data->footer_len > 0) {
      zstream.next_in = data->footer;
      zstream.avail_in = data->footer_len;
      /* TODO: Z_NO_FLUSH here may not be safe. All zstream.next_in
       * may not be processed. */
      zrc = deflate(&zstream, Z_NO_FLUSH);
      if (zrc != Z_OK) {
        ERR(GRN_ZLIB_ERROR, "%s footer: %s", tag, grn_zrc_to_string(zrc));
        deflateEnd(&zstream);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }

    zrc = deflate(&zstream, Z_FINISH);
    if (zrc != Z_STREAM_END) {
      ERR(GRN_ZLIB_ERROR, "%s finish: %s", tag, grn_zrc_to_string(zrc));
      deflateEnd(&zstream);
      GRN_FREE(data->compressed_value);
      data->compressed_value = NULL;
      return ctx->rc;
    }
  }
  data->compressed_value_len = COMPRESSED_VALUE_LEN(zstream.total_out);

  zrc = deflateEnd(&zstream);
  if (zrc != Z_OK) {
    ERR(GRN_ZLIB_ERROR, "%s end: %s", tag, grn_zrc_to_string(zrc));
    GRN_FREE(data->compressed_value);
    data->compressed_value = NULL;
    data->compressed_value_len = 0;
    return ctx->rc;
  }

  return ctx->rc;
}

static inline grn_rc
grn_compressor_decompress_zlib(grn_ctx *ctx, grn_decompress_data *data)
{
  const char *tag = "[compressor][decompress][zlib] failed to decompress:";

  z_stream zstream = {0};
  zstream.next_in = (Bytef *)(data->compressed_value);
  zstream.avail_in = data->compressed_value_len;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  int zrc = inflateInit2(&zstream, 15 /* windowBits */);
  if (zrc != Z_OK) {
    ERR(GRN_ZLIB_ERROR, "%s initialize: %s", tag, grn_zrc_to_string(zrc));
    return ctx->rc;
  }
  data->decompressed_value = GRN_MALLOC(data->decompressed_value_len);
  if (!data->decompressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_ZLIB_ERROR, "%s allocate buffer: %s", tag, message);
    inflateEnd(&zstream);
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  zstream.next_out = (Bytef *)(data->decompressed_value);
  zstream.avail_out = data->decompressed_value_len;
  zrc = inflate(&zstream, Z_FINISH);
  if (zrc != Z_STREAM_END) {
    ERR(GRN_ZLIB_ERROR, "%s finish: %s", tag, grn_zrc_to_string(zrc));
    inflateEnd(&zstream);
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  data->decompressed_value_len = zstream.total_out;
  zrc = inflateEnd(&zstream);
  if (zrc != Z_OK) {
    ERR(GRN_ZLIB_ERROR, "%s end: %s", tag, grn_zrc_to_string(zrc));
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  return ctx->rc;
}
#endif /* GRN_WITH_ZLIB */

#ifdef GRN_WITH_LZ4
static inline grn_rc
grn_compressor_compress_lz4(grn_ctx *ctx, grn_compress_data *data)
{
  const char *tag = "[compressor][compress][lz4] failed to compress:";

  size_t input_len = data->header_len + data->body_len + data->footer_len;
  if (input_len > (size_t)LZ4_MAX_INPUT_SIZE) {
    data->packed_value = GRN_MALLOC(COMPRESSED_VALUE_LEN(input_len));
    if (!data->packed_value) {
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(GRN_LZ4_ERROR, "%s allocate packed buffer: %s", tag, message);
      return ctx->rc;
    }
    grn_rc rc = grn_compressor_pack(ctx, data);
    if (rc != GRN_SUCCESS) {
      GRN_FREE(data->packed_value);
      data->packed_value = NULL;
      data->packed_value_len = 0;
    }
    return rc;
  }

  int lz4_value_len_max = LZ4_compressBound((int)input_len);
  data->compressed_value = GRN_MALLOC(COMPRESSED_VALUE_LEN(lz4_value_len_max));
  if (!data->compressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_LZ4_ERROR, "%s allocate compressed buffer: %s", tag, message);
    return ctx->rc;
  }
  COMPRESSED_VALUE_SET_METADATA(data->compressed_value,
                                COMPRESSED_VALUE_METADATA_PACK(input_len, 0));

  char *lz4_value = COMPRESSED_VALUE_GET_DATA(data->compressed_value);
  grn_obj value;
  if (data->header_len == 0 && data->footer_len == 0) {
    GRN_TEXT_INIT(&value, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET(ctx, &value, data->body, data->body_len);
  } else {
    GRN_TEXT_INIT(&value, 0);
    if (data->header_len > 0) {
      GRN_TEXT_PUT(ctx, &value, data->header, data->header_len);
    }
    if (data->body_len > 0) {
      GRN_TEXT_PUT(ctx, &value, data->body, data->body_len);
    }
    if (data->footer_len > 0) {
      GRN_TEXT_PUT(ctx, &value, data->footer, data->footer_len);
    }
  }
  int lz4_value_len_real = LZ4_compress_default(GRN_TEXT_VALUE(&value),
                                                lz4_value,
                                                GRN_TEXT_LEN(&value),
                                                lz4_value_len_max);
  GRN_OBJ_FIN(ctx, &value);
  if (lz4_value_len_real <= 0) {
    ERR(GRN_LZ4_ERROR, "%s compress", tag);
    GRN_FREE(data->compressed_value);
    data->compressed_value = NULL;
    return ctx->rc;
  }
  data->compressed_value_len = COMPRESSED_VALUE_LEN(lz4_value_len_real);

  return ctx->rc;
}

static inline grn_rc
grn_compressor_decompress_lz4(grn_ctx *ctx, grn_decompress_data *data)
{
  const char *tag = "[compressor][decompress][lz4] failed to decompress:";

  data->decompressed_value = GRN_MALLOC(data->decompressed_value_len);
  if (!data->decompressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_LZ4_ERROR, "%s allocate buffer: %s", tag, message);
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  if (LZ4_decompress_safe((const char *)(data->compressed_value),
                          (char *)(data->decompressed_value),
                          data->compressed_value_len,
                          data->decompressed_value_len) < 0) {
    ERR(GRN_LZ4_ERROR, "%s decompress", tag);
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }

  return ctx->rc;
}
#endif /* GRN_WITH_LZ4 */

#ifdef GRN_WITH_ZSTD
static inline grn_rc
grn_compressor_compress_zstd(grn_ctx *ctx, grn_compress_data *data)
{
  const char *tag = "[compressor][compress][zstd] failed to compress:";

  const int zstd_compression_level = ZSTD_CLEVEL_DEFAULT;

  ZSTD_CCtx *zstd_cctx = ZSTD_createCCtx();
  if (!zstd_cctx) {
    ERR(GRN_ZSTD_ERROR, "%s allocate compress context", tag);
    return ctx->rc;
  }

  size_t input_len = data->header_len + data->body_len + data->footer_len;
  size_t zstd_value_len_max = ZSTD_compressBound(input_len);
  data->compressed_value = GRN_MALLOC(COMPRESSED_VALUE_LEN(zstd_value_len_max));
  if (!data->compressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_ZSTD_ERROR, "%s allocate compress buffer: %s", tag, message);
    ZSTD_freeCCtx(zstd_cctx);
    return ctx->rc;
  }
  void *zstd_value = COMPRESSED_VALUE_GET_DATA(data->compressed_value);
  COMPRESSED_VALUE_SET_METADATA(data->compressed_value,
                                COMPRESSED_VALUE_METADATA_PACK(input_len, 0));

  if (data->header_len == 0 && data->footer_len == 0) {
    size_t zstd_value_len_real = ZSTD_compressCCtx(zstd_cctx,
                                                   zstd_value,
                                                   zstd_value_len_max,
                                                   data->body,
                                                   data->body_len,
                                                   zstd_compression_level);
    if (ZSTD_isError(zstd_value_len_real)) {
      ERR(GRN_ZSTD_ERROR,
          "%s compress: %s",
          tag,
          ZSTD_getErrorName(zstd_value_len_real));
      ZSTD_freeCCtx(zstd_cctx);
      GRN_FREE(data->compressed_value);
      data->compressed_value = NULL;
      return ctx->rc;
    }
    data->compressed_value_len = COMPRESSED_VALUE_LEN(zstd_value_len_real);
  } else {
    ZSTD_outBuffer zstd_output = {0};
    zstd_output.dst = zstd_value;
    zstd_output.size = zstd_value_len_max;

    if (data->header_len > 0) {
      ZSTD_inBuffer zstd_input = {0};
      zstd_input.src = data->header;
      zstd_input.size = data->header_len;
      zstd_input.pos = 0;
      size_t zstd_result = ZSTD_compressStream2(zstd_cctx,
                                                &zstd_output,
                                                &zstd_input,
                                                ZSTD_e_flush);
      if (ZSTD_isError(zstd_result)) {
        ERR(GRN_ZSTD_ERROR,
            "%s compress header: %s",
            tag,
            ZSTD_getErrorName(zstd_result));
        ZSTD_freeCCtx(zstd_cctx);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }

    if (data->body_len > 0) {
      ZSTD_inBuffer zstd_input = {0};
      zstd_input.src = data->body;
      zstd_input.size = data->body_len;
      zstd_input.pos = 0;
      size_t zstd_result = ZSTD_compressStream2(zstd_cctx,
                                                &zstd_output,
                                                &zstd_input,
                                                ZSTD_e_flush);
      if (ZSTD_isError(zstd_result)) {
        ERR(GRN_ZSTD_ERROR,
            "%s compress body: %s",
            tag,
            ZSTD_getErrorName(zstd_result));
        ZSTD_freeCCtx(zstd_cctx);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }

    if (data->footer_len > 0) {
      ZSTD_inBuffer zstd_input = {0};
      zstd_input.src = data->footer;
      zstd_input.size = data->footer_len;
      zstd_input.pos = 0;
      size_t zstd_result = ZSTD_compressStream2(zstd_cctx,
                                                &zstd_output,
                                                &zstd_input,
                                                ZSTD_e_flush);
      if (ZSTD_isError(zstd_result)) {
        ERR(GRN_ZSTD_ERROR,
            "%s compress footer: %s",
            tag,
            ZSTD_getErrorName(zstd_result));
        ZSTD_freeCCtx(zstd_cctx);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }

    {
      ZSTD_inBuffer zstd_input = {0};
      size_t zstd_result =
        ZSTD_compressStream2(zstd_cctx, &zstd_output, &zstd_input, ZSTD_e_end);
      if (ZSTD_isError(zstd_result)) {
        ERR(GRN_ZSTD_ERROR, "%s end: %s", tag, ZSTD_getErrorName(zstd_result));
        ZSTD_freeCCtx(zstd_cctx);
        GRN_FREE(data->compressed_value);
        data->compressed_value = NULL;
        return ctx->rc;
      }
    }
    data->compressed_value_len = COMPRESSED_VALUE_LEN(zstd_output.pos);
  }
  ZSTD_freeCCtx(zstd_cctx);

  return ctx->rc;
}

static inline grn_rc
grn_compressor_decompress_zstd(grn_ctx *ctx, grn_decompress_data *data)
{
  const char *tag = "[compressor][decompress][zstd] failed to decompress:";

  data->decompressed_value = GRN_MALLOC(data->decompressed_value_len);
  if (!data->decompressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_ZSTD_ERROR, "%s allocate buffer: %s", tag, message);
    data->decompressed_value_len = 0;
    return ctx->rc;
  }

  size_t written_len = ZSTD_decompress((char *)(data->decompressed_value),
                                       data->decompressed_value_len,
                                       data->compressed_value,
                                       data->compressed_value_len);
  if (ZSTD_isError(written_len)) {
    ERR(GRN_ZSTD_ERROR,
        "%s decompress: %s",
        tag,
        ZSTD_getErrorName(written_len));
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }

  return ctx->rc;
}
#endif /* GRN_WITH_ZSTD */

#ifdef GRN_WITH_BLOSC
static inline blosc2_schunk *
grn_compressor_compress_blosc_create_schunk(grn_ctx *ctx,
                                            grn_compress_data *data,
                                            blosc2_cparams *cparams,
                                            blosc2_storage *storage)
{
  *cparams = BLOSC2_CPARAMS_DEFAULTS;
  switch (data->body_column_flags & GRN_OBJ_COMPRESS_MASK) {
#  ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB:
    cparams->compcode = BLOSC_ZLIB;
    break;
#  endif
#  ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4:
    cparams->compcode = BLOSC_LZ4;
    break;
#  endif
#  ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD:
    cparams->compcode = BLOSC_ZSTD;
    break;
#  endif
  default:
    break;
  }
  if (grn_type_id_is_text_family(ctx, data->body_range)) {
    cparams->typesize = sizeof(char);
  } else {
    cparams->typesize = grn_type_id_size(ctx, data->body_range);
    if (data->body_column_flags & GRN_OBJ_WITH_WEIGHT) {
      if (data->body_column_flags & GRN_OBJ_WEIGHT_BFLOAT16) {
#  ifdef GRN_HAVE_BFLOAT16
        cparams->typesize += sizeof(grn_bfloat16);
#  else
        cparams->typesize += sizeof(float);
#  endif
      } else if (data->body_column_flags & GRN_OBJ_WEIGHT_FLOAT32) {
        cparams->typesize += sizeof(float);
      } else {
        cparams->typesize += sizeof(double);
      }
    }
  }
  size_t current_filter_id = BLOSC2_MAX_FILTERS - 1;
  cparams->filters[current_filter_id] = BLOSC_NOFILTER;
  if (data->body_n_elements > 1) {
    if (data->body_column_flags & GRN_OBJ_COMPRESS_FILTER_BYTE_DELTA) {
      cparams->filters[current_filter_id] = BLOSC_FILTER_BYTEDELTA;
      cparams->filters_meta[current_filter_id] = cparams->typesize;
      current_filter_id--;
    }
    if (cparams->typesize > (int32_t)sizeof(char)) {
      if (data->body_column_flags & GRN_OBJ_COMPRESS_FILTER_SHUFFLE) {
        cparams->filters[current_filter_id] = BLOSC_SHUFFLE;
        current_filter_id--;
      }
    }
    if (data->body_range == GRN_DB_FLOAT ||
        data->body_range == GRN_DB_FLOAT32) {
      if (data->body_column_flags &
          GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES) {
        cparams->filters[current_filter_id] = BLOSC_TRUNC_PREC;
        cparams->filters_meta[current_filter_id] = -16;
        current_filter_id--;
      } else if (data->body_column_flags &
                 GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE) {
        cparams->filters[current_filter_id] = BLOSC_TRUNC_PREC;
        cparams->filters_meta[current_filter_id] = -8;
        current_filter_id--;
      }
    }
  }

  *storage = BLOSC2_STORAGE_DEFAULTS;
  storage->contiguous = true;
  storage->cparams = cparams;

  return blosc2_schunk_new(storage);
}

static inline grn_rc
grn_compressor_compress_blosc(grn_ctx *ctx, grn_compress_data *data)
{
  const char *tag = "[compressor][compress][blosc] failed to compress:";

  blosc2_cparams cparams;
  blosc2_storage storage;
  blosc2_schunk *schunk =
    grn_compressor_compress_blosc_create_schunk(ctx, data, &cparams, &storage);

  if (data->header_len > 0) {
    int meta_index = blosc2_meta_add(schunk,
                                     GRN_BLOSC_META_HEADER,
                                     data->header,
                                     data->header_len);
    if (meta_index < 0) {
      ERR(GRN_BLOSC_ERROR, "%s set header: %s", tag, print_error(meta_index));
      blosc2_schunk_free(schunk);
      return ctx->rc;
    }
  }

  if (data->footer_len > 0) {
    int meta_index = blosc2_meta_add(schunk,
                                     GRN_BLOSC_META_FOOTER,
                                     data->footer,
                                     data->footer_len);
    if (meta_index < 0) {
      ERR(GRN_BLOSC_ERROR, "%s set footer: %s", tag, print_error(meta_index));
      blosc2_schunk_free(schunk);
      return ctx->rc;
    }
  }

  if (data->body_len > 0) {
    int64_t n_chunks =
      blosc2_schunk_append_buffer(schunk, data->body, data->body_len);
    if (n_chunks < 0) {
      ERR(GRN_BLOSC_ERROR, "%s compress body: %s", tag, print_error(n_chunks));
      blosc2_schunk_free(schunk);
      return ctx->rc;
    }
  }

  uint8_t *blosc_value = NULL;
  bool blosc_value_need_free = false;
  int64_t blosc_value_len =
    blosc2_schunk_to_buffer(schunk, &blosc_value, &blosc_value_need_free);
  if (blosc_value_len < 0) {
    ERR(GRN_BLOSC_ERROR,
        "%s serialize compressed value: %s",
        tag,
        print_error(blosc_value_len));
    blosc2_schunk_free(schunk);
    return ctx->rc;
  }

  data->compressed_value_len = COMPRESSED_VALUE_LEN(blosc_value_len);
  data->compressed_value = GRN_MALLOC(data->compressed_value_len);
  if (!data->compressed_value) {
    ERR(GRN_BLOSC_ERROR, "%s allocate packed value buffer", tag);
    if (blosc_value_need_free) {
      free(blosc_value);
    }
    blosc2_schunk_free(schunk);
    data->compressed_value_len = 0;
    return ctx->rc;
  }
  size_t input_len = data->header_len + data->body_len + data->footer_len;
  COMPRESSED_VALUE_SET_METADATA(data->compressed_value,
                                COMPRESSED_VALUE_METADATA_PACK(input_len, 0));
  grn_memcpy(COMPRESSED_VALUE_GET_DATA(data->compressed_value),
             blosc_value,
             blosc_value_len);
  if (blosc_value_need_free) {
    free(blosc_value);
  }
  blosc2_schunk_free(schunk);

  return ctx->rc;
}

static inline bool
grn_compressor_decompress_blosc_copy_meta(grn_ctx *ctx,
                                          blosc2_schunk *schunk,
                                          uint8_t **destination,
                                          int32_t *destination_len,
                                          const char *name,
                                          const char *tag)
{
  int i = 0;
  for (i = 0; i < schunk->nmetalayers; i++) {
    blosc2_metalayer *metalayer = schunk->metalayers[i];
    if (strcmp(metalayer->name, name) == 0) {
      if (metalayer->content_len > *destination_len) {
        ERR(GRN_BLOSC_ERROR,
            "%s %s is too large: %d > %d",
            tag,
            name,
            metalayer->content_len,
            *destination_len);
        return false;
      }
      memcpy(*destination, metalayer->content, metalayer->content_len);
      *destination += metalayer->content_len;
      *destination_len -= metalayer->content_len;
      break;
    }
  }
  return true;
}

static grn_rc
grn_compressor_decompress_blosc(grn_ctx *ctx, grn_decompress_data *data)
{
  const char *tag = "[compressor][decompress][blosc] failed to decompress:";

  data->decompressed_value = GRN_MALLOC(data->decompressed_value_len);
  if (!data->decompressed_value) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_BLOSC_ERROR, "%s allocate buffer: %s", tag, message);
    data->decompressed_value_len = 0;
    return ctx->rc;
  }

  blosc2_schunk *schunk = blosc2_schunk_from_buffer(data->compressed_value,
                                                    data->compressed_value_len,
                                                    false);
  if (!schunk) {
    ERR(GRN_BLOSC_ERROR, "%s failed to load super chunk", tag);
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  uint8_t *destination = data->decompressed_value;
  int32_t destination_len = data->decompressed_value_len;
  if (!grn_compressor_decompress_blosc_copy_meta(ctx,
                                                 schunk,
                                                 &destination,
                                                 &destination_len,
                                                 GRN_BLOSC_META_HEADER,
                                                 tag)) {
    blosc2_schunk_free(schunk);
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  {
    int64_t i;
    for (i = 0; i < schunk->nchunks; i++) {
      int decompressed_len =
        blosc2_schunk_decompress_chunk(schunk, i, destination, destination_len);
      if (decompressed_len <= 0) {
        ERR(GRN_BLOSC_ERROR,
            "%s failed to decode a chunk: %" GRN_FMT_INT64D ": %s",
            tag,
            i,
            print_error(decompressed_len));
        blosc2_schunk_free(schunk);
        GRN_FREE(data->decompressed_value);
        data->decompressed_value = NULL;
        data->decompressed_value_len = 0;
        return ctx->rc;
      }
      destination += decompressed_len;
      destination_len -= decompressed_len;
    }
  }
  if (!grn_compressor_decompress_blosc_copy_meta(ctx,
                                                 schunk,
                                                 &destination,
                                                 &destination_len,
                                                 GRN_BLOSC_META_FOOTER,
                                                 tag)) {
    blosc2_schunk_free(schunk);
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }
  blosc2_schunk_free(schunk);
  if (destination_len != 0) {
    ERR(GRN_BLOSC_ERROR,
        "%s decompressed size isn't match: expected:%u actual:%d",
        tag,
        data->decompressed_value_len,
        destination_len);
    GRN_FREE(data->decompressed_value);
    data->decompressed_value = NULL;
    data->decompressed_value_len = 0;
    return ctx->rc;
  }

  return ctx->rc;
}
#endif

grn_rc
grn_compressor_compress(grn_ctx *ctx, grn_compress_data *data)
{
  size_t input_len = data->header_len + data->body_len + data->footer_len;
  if (input_len < COMPRESS_THRESHOLD_BYTE) {
    data->packed_value = GRN_MALLOC(COMPRESSED_VALUE_LEN(input_len));
    if (!data->packed_value) {
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      grn_rc rc;
      switch (data->type) {
      case GRN_COMPRESSION_TYPE_ZLIB:
        rc = GRN_ZLIB_ERROR;
        break;
      case GRN_COMPRESSION_TYPE_LZ4:
        rc = GRN_LZ4_ERROR;
        break;
      case GRN_COMPRESSION_TYPE_ZSTD:
        rc = GRN_ZSTD_ERROR;
        break;
      case GRN_COMPRESSION_TYPE_BLOSC:
        rc = GRN_BLOSC_ERROR;
        break;
      default:
        rc = ctx->rc == GRN_SUCCESS ? GRN_UNKNOWN_ERROR : ctx->rc;
        break;
      }
      ERR(rc,
          "[compressor][compress] failed to allocate packed buffer: %s",
          message);
      return ctx->rc;
    }

    grn_rc rc = grn_compressor_pack(ctx, data);
    if (rc != GRN_SUCCESS) {
      GRN_FREE(data->packed_value);
      data->packed_value = NULL;
      data->packed_value_len = 0;
    }
    return rc;
  }

  switch (data->type) {
#ifdef GRN_WITH_ZLIB
  case GRN_COMPRESSION_TYPE_ZLIB:
    return grn_compressor_compress_zlib(ctx, data);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_COMPRESSION_TYPE_LZ4:
    return grn_compressor_compress_lz4(ctx, data);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_COMPRESSION_TYPE_ZSTD:
    return grn_compressor_compress_zstd(ctx, data);
#endif /* GRN_WITH_ZSTD */
#ifdef GRN_WITH_BLOSC
  case GRN_COMPRESSION_TYPE_BLOSC:
    return grn_compressor_compress_blosc(ctx, data);
#endif
  default:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[compressor][compress] unsupported type: %d",
        data->type);
    return ctx->rc;
  }
}

grn_rc
grn_compressor_decompress(grn_ctx *ctx, grn_decompress_data *data)
{
  grn_rc rc = grn_compressor_unpack(ctx, data);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (data->unpacked_value) {
    return ctx->rc;
  }

  switch (data->type) {
#ifdef GRN_WITH_ZLIB
  case GRN_COMPRESSION_TYPE_ZLIB:
    return grn_compressor_decompress_zlib(ctx, data);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_COMPRESSION_TYPE_LZ4:
    return grn_compressor_decompress_lz4(ctx, data);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_COMPRESSION_TYPE_ZSTD:
    return grn_compressor_decompress_zstd(ctx, data);
#endif /* GRN_WITH_ZSTD */
#ifdef GRN_WITH_BLOSC
  case GRN_COMPRESSION_TYPE_BLOSC:
    return grn_compressor_decompress_blosc(ctx, data);
#endif
  default:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[compressor][decompress] unsupported type: %d",
        data->type);
    return ctx->rc;
  }
}
