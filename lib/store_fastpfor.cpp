/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_ctx.h"
#include "grn_store.h"

#ifdef GRN_WITH_FASTPFOR
#include <fastpfor/compositecodec.h>
#include <fastpfor/simdfastpfor.h>
#include <fastpfor/variablebyte.h>

namespace {
  size_t
  align(size_t value, size_t factor) {
    if (value == 0) {
      return 0;
    }
    return (1 + (value - 1) / factor) * factor;
  }
}

using Codec = FastPForLib::CompositeCodec<FastPForLib::SIMDFastPFor<8>,
                                          FastPForLib::VariableByte>;

extern "C" {
  void *
  grn_ja_ref_fastpfor(grn_ctx *ctx,
                      grn_ja *ja,
                      grn_id id,
                      grn_io_win *iw,
                      uint32_t *value_len)
  {
    void *raw_value;
    uint32_t raw_value_len;
    void *fastpfor_value;
    uint32_t fastpfor_value_len;
    void *unpacked_value;
    uint32_t uncompressed_value_len;

    if (!(raw_value = grn_ja_ref_raw(ctx, ja, id, iw, &raw_value_len))) {
      iw->uncompressed_value = NULL;
      *value_len = 0;
      return NULL;
    }

    unpacked_value = grn_ja_ref_packed(ctx,
                                       iw, value_len,
                                       raw_value, raw_value_len,
                                       &fastpfor_value, &fastpfor_value_len,
                                       &uncompressed_value_len);
    if (unpacked_value) {
      return unpacked_value;
    }

    auto aligned_uncompressed_value_len =
      align(uncompressed_value_len, sizeof(uint32_t));
    if (!(iw->uncompressed_value = GRN_MALLOC(aligned_uncompressed_value_len))) {
      iw->uncompressed_value = NULL;
      *value_len = 0;
      return NULL;
    }

    Codec codec;
    auto uncompressed_size = aligned_uncompressed_value_len / sizeof(uint32_t);
    // TODO: catch exception
    codec.decodeArray(static_cast<uint32_t *>(fastpfor_value),
                      fastpfor_value_len / sizeof(uint32_t),
                      static_cast<uint32_t *>(iw->uncompressed_value),
                      uncompressed_size);
    *value_len = uncompressed_value_len;
    return iw->uncompressed_value;
  }

  grn_rc
  grn_ja_put_fastpfor(grn_ctx *ctx,
                      grn_ja *ja,
                      grn_id id,
                      void *value,
                      uint32_t value_len,
                      int flags,
                      uint64_t *cas)
  {
    if (value_len == 0) {
      return grn_ja_put_raw(ctx, ja, id, value, value_len, flags, cas);
    }

    if (value_len < GRN_JA_COMPRESS_THRESHOLD_BYTE) {
      return grn_ja_put_packed(ctx, ja, id, value, value_len, flags, cas);
    }

    Codec codec;
    std::vector<uint32_t> compressed_output((value_len / sizeof(uint32_t)) +
                                            1024);
    auto compressed_data = compressed_output.data();
    auto compressed_size = compressed_output.size();
    *reinterpret_cast<uint64_t *>(compressed_data) = value_len;
    compressed_data += sizeof(uint64_t) / sizeof(uint32_t);
    compressed_size -= sizeof(uint64_t) / sizeof(uint32_t);
    if ((value_len % sizeof(uint32_t)) == 0) {
      // TODO: catch exception
      codec.encodeArray(static_cast<uint32_t *>(value),
                        value_len / sizeof(uint32_t),
                        compressed_data,
                        compressed_size);
    } else {
      std::vector<uint32_t> input(align(value_len, sizeof(uint32_t)) /
                                  sizeof(uint32_t));
      memcpy(reinterpret_cast<uint8_t *>(input.data()),
             value,
             value_len);
      // TODO: catch exception
      codec.encodeArray(input.data(),
                        input.size(),
                        compressed_data,
                        compressed_size);
    }
    return grn_ja_put_raw(ctx,
                          ja,
                          id,
                          compressed_output.data(),
                          sizeof(uint64_t) + compressed_size * sizeof(uint32_t),
                          flags,
                          cas);
  }

  grn_rc
  grn_ja_putv_fastpfor(grn_ctx *ctx,
                       grn_ja *ja,
                       grn_id id,
                       grn_obj *header,
                       grn_obj *body,
                       grn_obj *footer,
                       int flags)
  {
    const size_t header_size = GRN_BULK_VSIZE(header);
    const size_t body_size = body ? GRN_BULK_VSIZE(body) : 0;
    const size_t footer_size = GRN_BULK_VSIZE(footer);
    const size_t size = header_size + body_size + footer_size;

    if (size < GRN_JA_COMPRESS_THRESHOLD_BYTE) {
      return grn_ja_putv_packed(ctx, ja, id, header, body, footer, flags);
    }

    Codec codec;
    std::vector<uint32_t> input(align(size, sizeof(uint32_t)) /
                                sizeof(uint32_t));
    memcpy(reinterpret_cast<uint8_t *>(input.data()),
           GRN_BULK_HEAD(header),
           header_size);
    if (body) {
      memcpy(reinterpret_cast<uint8_t *>(input.data()) + header_size,
             GRN_BULK_HEAD(body),
             body_size);
    }
    memcpy(reinterpret_cast<uint8_t *>(input.data()) + header_size + body_size,
           GRN_BULK_HEAD(footer),
           footer_size);
    std::vector<uint32_t> compressed_output((size / sizeof(uint32_t)) + 1024);
    size_t compressed_size = compressed_output.size();
    // TODO: catch exception
    codec.encodeArray(input.data(),
                      input.size(),
                      compressed_output.data(),
                      compressed_size);
    return grn_ja_putv_compressed(ctx,
                                  ja,
                                  id,
                                  compressed_output.data(),
                                  compressed_size * sizeof(uint32_t),
                                  size,
                                  flags);
  }
}
#endif
