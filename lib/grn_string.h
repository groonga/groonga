/*
  Copyright(C) 2012-2018  Brazil
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_ctx.h"
#include "grn_db.h"
#include "grn_str.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  grn_obj_header header;
  const char *original;
  unsigned int original_length_in_bytes;
  char *normalized;
  unsigned int normalized_length_in_bytes;
  unsigned int n_characters;
  int16_t *checks;
  uint8_t *ctypes;
  uint64_t *offsets;
  grn_encoding encoding;
  int flags;
  grn_obj *lexicon;
  uint32_t normalizer_index;
} grn_string;

grn_rc grn_string_init(grn_ctx *ctx,
                       grn_obj *string,
                       grn_obj *lexicon_or_normalizer,
                       int flags,
                       grn_encoding encoding);
grn_obj *grn_string_open_(grn_ctx *ctx,
                          const char *str,
                          unsigned int str_len,
                          grn_obj *lexicon_or_normalizer,
                          int flags,
                          grn_encoding encoding);
grn_rc grn_string_fin(grn_ctx *ctx, grn_obj *string);
grn_rc grn_string_close(grn_ctx *ctx, grn_obj *string);
grn_rc grn_string_set_original(grn_ctx *ctx,
                               grn_obj *string,
                               const char *original,
                               unsigned int length_in_bytes);
grn_rc grn_string_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *string);

#ifdef __cplusplus
}
#endif
