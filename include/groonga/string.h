/*
  Copyright(C) 2009-2018  Brazil
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

#ifdef  __cplusplus
extern "C" {
#endif

/* grn_str: deprecated. use grn_string instead. */

typedef struct {
  const char *orig;
  char *norm;
  int16_t *checks;
  uint8_t *ctypes;
  int flags;
  unsigned int orig_blen;
  unsigned int norm_blen;
  unsigned int length;
  grn_encoding encoding;
} grn_str;

#define GRN_STR_REMOVEBLANK            (0x01<<0)
#define GRN_STR_WITH_CTYPES            (0x01<<1)
#define GRN_STR_WITH_CHECKS            (0x01<<2)
#define GRN_STR_NORMALIZE              GRN_OBJ_KEY_NORMALIZE

GRN_API grn_str *grn_str_open(grn_ctx *ctx, const char *str, unsigned int str_len,
                              int flags);
GRN_API grn_rc grn_str_close(grn_ctx *ctx, grn_str *nstr);


/* grn_string */

#define GRN_STRING_REMOVE_BLANK               (0x01<<0)
#define GRN_STRING_WITH_TYPES                 (0x01<<1)
#define GRN_STRING_WITH_CHECKS                (0x01<<2)
#define GRN_STRING_REMOVE_TOKENIZED_DELIMITER (0x01<<3)

#define GRN_NORMALIZER_AUTO ((grn_obj *)1)

#define GRN_CHAR_BLANK 0x80
#define GRN_CHAR_IS_BLANK(c) ((c) & (GRN_CHAR_BLANK))
#define GRN_REMOVED_CHAR_TYPE_SYMBOL 0x40
#define GRN_REMOVED_CHAR_TYPE_IS_SYMBOL(char_type) \
  ((char_type) & (GRN_REMOVED_CHAR_TYPE_SYMBOL))
#define GRN_CHAR_TYPE(c) ((c) & 0x3f)

typedef enum {
  GRN_CHAR_NULL = 0,
  GRN_CHAR_ALPHA,
  GRN_CHAR_DIGIT,
  GRN_CHAR_SYMBOL,
  GRN_CHAR_HIRAGANA,
  GRN_CHAR_KATAKANA,
  GRN_CHAR_KANJI,
  GRN_CHAR_OTHERS,
  GRN_CHAR_EMOJI
} grn_char_type;

GRN_API const char *grn_char_type_to_string(grn_char_type type);

GRN_API grn_obj *grn_string_open(grn_ctx *ctx,
                                 const char *string,
                                 unsigned int length_in_bytes,
                                 grn_obj *lexicon_or_normalizer,
                                 int flags);
GRN_API grn_rc grn_string_get_original(grn_ctx *ctx, grn_obj *string,
                                       const char **original,
                                       unsigned int *length_in_bytes);
GRN_API int grn_string_get_flags(grn_ctx *ctx, grn_obj *string);
GRN_API grn_rc grn_string_get_normalized(grn_ctx *ctx, grn_obj *string,
                                         const char **normalized,
                                         unsigned int *length_in_bytes,
                                         unsigned int *n_characters);
GRN_API grn_rc grn_string_set_normalized(grn_ctx *ctx, grn_obj *string,
                                         char *normalized,
                                         unsigned int length_in_bytes,
                                         unsigned int n_characters);
GRN_API const int16_t *grn_string_get_checks(grn_ctx *ctx, grn_obj *string);
GRN_API grn_rc grn_string_set_checks(grn_ctx *ctx,
                                     grn_obj *string,
                                     int16_t *checks);
GRN_API const uint8_t *grn_string_get_types(grn_ctx *ctx, grn_obj *string);
GRN_API grn_rc grn_string_set_types(grn_ctx *ctx,
                                    grn_obj *string,
                                    uint8_t *types);
GRN_API const uint64_t *grn_string_get_offsets(grn_ctx *ctx, grn_obj *string);
GRN_API grn_rc grn_string_set_offsets(grn_ctx *ctx,
                                      grn_obj *string,
                                      uint64_t *offsets);
GRN_API grn_encoding grn_string_get_encoding(grn_ctx *ctx, grn_obj *string);
GRN_API grn_obj *grn_string_get_table(grn_ctx *ctx, grn_obj *string);
GRN_API uint32_t grn_string_get_normalizer_index(grn_ctx *ctx, grn_obj *string);


GRN_API int grn_charlen(grn_ctx *ctx, const char *str, const char *end);

#ifdef __cplusplus
}
#endif
