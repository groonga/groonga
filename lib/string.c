/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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
#include <string.h>
#include "grn_string.h"
#include "grn_normalizer.h"
#include "grn_str.h"
#include "grn_util.h"

#include <groonga/tokenizer.h>

const char *
grn_char_type_to_string(grn_char_type type)
{
  const char *string = "unknown";

#define CHAR_TYPE_STRING_WITH_BLANK(type_string) do {   \
    if (GRN_CHAR_IS_BLANK(type)) {                      \
      string = type_string "|blank";                    \
    } else {                                            \
      string = type_string;                             \
    }                                                   \
  } while (GRN_FALSE)

  switch (GRN_CHAR_TYPE(type)) {
  case GRN_CHAR_NULL :
    CHAR_TYPE_STRING_WITH_BLANK("null");
    break;
  case GRN_CHAR_ALPHA :
    CHAR_TYPE_STRING_WITH_BLANK("alpha");
    break;
  case GRN_CHAR_DIGIT :
    CHAR_TYPE_STRING_WITH_BLANK("digit");
    break;
  case GRN_CHAR_SYMBOL :
    CHAR_TYPE_STRING_WITH_BLANK("symbol");
    break;
  case GRN_CHAR_HIRAGANA :
    CHAR_TYPE_STRING_WITH_BLANK("hiragana");
    break;
  case GRN_CHAR_KATAKANA :
    CHAR_TYPE_STRING_WITH_BLANK("katakana");
    break;
  case GRN_CHAR_KANJI :
    CHAR_TYPE_STRING_WITH_BLANK("kanji");
    break;
  case GRN_CHAR_OTHERS :
    CHAR_TYPE_STRING_WITH_BLANK("others");
    break;
  case GRN_CHAR_EMOJI :
    CHAR_TYPE_STRING_WITH_BLANK("emoji");
    break;
  default :
    CHAR_TYPE_STRING_WITH_BLANK("unknown");
    break;
  }

#undef CHAR_TYPE_STRING_WITH_BLANK

  return string;
}

static grn_string *
grn_fake_string_open(grn_ctx *ctx, grn_string *string)
{
  /* TODO: support GRN_STRING_REMOVE_BLANK flag and ctypes */
  grn_string *nstr = string;
  const char *str;
  unsigned int str_len;

  str = nstr->original;
  str_len = nstr->original_length_in_bytes;

  if (!(nstr->normalized = GRN_MALLOC(str_len + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][fake] failed to allocate normalized text space");
    grn_string_close(ctx, (grn_obj *)nstr);
    return NULL;
  }

  if (nstr->flags & GRN_STRING_REMOVE_TOKENIZED_DELIMITER &&
      ctx->encoding == GRN_ENC_UTF8) {
    int char_length;
    const char *source_current = str;
    const char *source_end = str + str_len;
    char *destination = nstr->normalized;
    unsigned int destination_length = 0;
    while ((char_length = grn_charlen(ctx, source_current, source_end)) > 0) {
      if (!grn_tokenizer_is_tokenized_delimiter(ctx,
                                                source_current, char_length,
                                                ctx->encoding)) {
        grn_memcpy(destination, source_current, char_length);
        destination += char_length;
        destination_length += char_length;
      }
      source_current += char_length;
    }
    nstr->normalized[destination_length] = '\0';
    nstr->normalized_length_in_bytes = destination_length;
  } else {
    grn_memcpy(nstr->normalized, str, str_len);
    nstr->normalized[str_len] = '\0';
    nstr->normalized_length_in_bytes = str_len;
  }

  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    int16_t f = 0;
    unsigned char c;
    size_t i;
    if (!(nstr->checks = (int16_t *) GRN_MALLOC(sizeof(int16_t) * str_len))) {
      grn_string_close(ctx, (grn_obj *)nstr);
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][fake] failed to allocate checks space");
      return NULL;
    }
    switch (nstr->encoding) {
    case GRN_ENC_EUC_JP:
      for (i = 0; i < str_len; i++) {
        if (!f) {
          c = (unsigned char) str[i];
          f = ((c >= 0xa1U && c <= 0xfeU) || c == 0x8eU ? 2 : (c == 0x8fU ? 3 : 1)
            );
          nstr->checks[i] = f;
        } else {
          nstr->checks[i] = 0;
        }
        f--;
      }
      break;
    case GRN_ENC_SJIS:
      for (i = 0; i < str_len; i++) {
        if (!f) {
          c = (unsigned char) str[i];
          f = (c >= 0x81U && ((c <= 0x9fU) || (c >= 0xe0U && c <= 0xfcU)) ? 2 : 1);
          nstr->checks[i] = f;
        } else {
          nstr->checks[i] = 0;
        }
        f--;
      }
      break;
    case GRN_ENC_UTF8:
      for (i = 0; i < str_len; i++) {
        if (!f) {
          c = (unsigned char) str[i];
          f = (c & 0x80U ? (c & 0x20U ? (c & 0x10U ? 4 : 3)
                           : 2)
               : 1);
          nstr->checks[i] = f;
        } else {
          nstr->checks[i] = 0;
        }
        f--;
      }
      break;
    default:
      for (i = 0; i < str_len; i++) {
        nstr->checks[i] = 1;
      }
      break;
    }
  }
  return nstr;
}

grn_rc
grn_string_init(grn_ctx *ctx,
                grn_obj *string,
                grn_obj *lexicon_or_normalizer,
                int flags,
                grn_encoding encoding)
{
  grn_string *string_ = (grn_string *)string;

  GRN_OBJ_INIT(string, GRN_STRING, 0, GRN_ID_NIL);
  string_->original = NULL;
  string_->original_length_in_bytes = 0;
  string_->normalized = NULL;
  string_->normalized_length_in_bytes = 0;
  string_->n_characters = 0;
  string_->checks = NULL;
  string_->ctypes = NULL;
  string_->offsets = NULL;
  string_->encoding = encoding;
  string_->flags = flags;
  string_->lexicon = NULL;
  string_->normalizer_index = 0;

  if (lexicon_or_normalizer &&
      lexicon_or_normalizer != GRN_NORMALIZER_AUTO &&
      grn_obj_is_table(ctx, lexicon_or_normalizer)) {
    string_->lexicon = lexicon_or_normalizer;
  }

  return ctx->rc;
}

grn_obj *
grn_string_open_(grn_ctx *ctx,
                 const char *str,
                 unsigned int str_len,
                 grn_obj *lexicon_or_normalizer,
                 int flags,
                 grn_encoding encoding)
{
  grn_string *string_;
  grn_obj *string;

  if (!str || !str_len) {
    return NULL;
  }

  string_ = GRN_MALLOCN(grn_string, 1);
  if (!string_) {
    GRN_LOG(ctx, GRN_LOG_ALERT,
            "[string][open] failed to allocate memory");
    return NULL;
  }

  string = (grn_obj *)string_;
  grn_string_init(ctx, string, lexicon_or_normalizer, flags, encoding);
  string->header.impl_flags |= GRN_OBJ_ALLOCATED;
  string_->original = str;
  string_->original_length_in_bytes = str_len;

  if (!lexicon_or_normalizer) {
    return (grn_obj *)grn_fake_string_open(ctx, string_);
  }

  {
    grn_obj normalizers;
    GRN_PTR_INIT(&normalizers, GRN_OBJ_VECTOR, GRN_ID_NIL);
    if (lexicon_or_normalizer) {
      if (string_->lexicon) {
        grn_obj_get_info(ctx,
                         string_->lexicon,
                         GRN_INFO_NORMALIZERS,
                         &normalizers);
      } else {
        grn_bool is_normalizer_auto;
        is_normalizer_auto = (lexicon_or_normalizer == GRN_NORMALIZER_AUTO);
        if (is_normalizer_auto) {
          grn_obj *normalizer = grn_ctx_get(ctx, GRN_NORMALIZER_AUTO_NAME, -1);
          if (!normalizer) {
            grn_obj_close(ctx, string);
            ERR(GRN_INVALID_ARGUMENT,
                "[string][open] NormalizerAuto normalizer isn't available");
            return NULL;
          }
          GRN_PTR_PUT(ctx, &normalizers, normalizer);
        } else {
          grn_obj *normalizer = lexicon_or_normalizer;
          GRN_PTR_PUT(ctx, &normalizers, normalizer);
        }
      }
    }
    size_t n = GRN_PTR_VECTOR_SIZE(&normalizers);
    if (n == 0) {
      GRN_OBJ_FIN(ctx, &normalizers);
      return (grn_obj *)grn_fake_string_open(ctx, string_);
    }
    char *previous_normalized = NULL;
    unsigned int previous_normalized_length_in_bytes = 0;
    int16_t *previous_checks = NULL;
    uint8_t *previous_types = NULL;
    uint64_t *previous_offsets = NULL;
    size_t i;
    for (i = 0; i < n; i++) {
      grn_obj *normalizer = GRN_PTR_VALUE_AT(&normalizers, i);
      string_->normalizer_index = i;
      if (i > 0) {
        previous_normalized = string_->normalized;
        previous_normalized_length_in_bytes =
          string_->normalized_length_in_bytes;
        previous_checks = string_->checks;
        previous_types = string_->ctypes;
        previous_offsets = string_->offsets;
        string_->original = previous_normalized;
        string_->original_length_in_bytes = previous_normalized_length_in_bytes;
      }
      grn_normalizer_normalize(ctx, normalizer, string);
      if (i > 0) {
        if (previous_checks) {
          if (string_->checks) {
            unsigned int previous_i = 0;
            unsigned int current_i = 0;
            for (current_i = 0;
                 current_i < string_->normalized_length_in_bytes;
                 current_i++) {
              int16_t previous_check = string_->checks[current_i];
              if (previous_check > 0) {
                int16_t original_check = previous_checks[previous_i];
                string_->checks[current_i] = original_check;
                previous_i += previous_check;
              }
            }
          }
          GRN_FREE(previous_checks);
        }
        if (previous_types) {
          GRN_FREE(previous_types);
        }
        if (previous_offsets) {
          if (string_->offsets) {
            uint64_t previous_offset = 0;
            const char *previous_start = string_->original;
            const char *previous_end =
              previous_start + string_->original_length_in_bytes;
            unsigned int previous_i = 0;
            unsigned int current_i = 0;
            for (current_i = 0;
                 current_i < string_->n_characters;
                 current_i++) {
              while (string_->offsets[current_i] > previous_offset) {
                int previous_character_length =
                  grn_charlen(ctx, previous_start, previous_end);
                if (previous_character_length == 0) {
                  ERR(GRN_INVALID_ARGUMENT,
                      "[string][open] invalid character in normalized string: "
                      "<%.*s>",
                      (int)(previous_end - previous_start),
                      previous_start);
                  break;
                }
                previous_start += previous_character_length;
                previous_offset += previous_character_length;
                previous_i++;
              }
              if (ctx->rc != GRN_SUCCESS) {
                break;
              }
              string_->offsets[current_i] = previous_offsets[previous_i];
            }
          }
          GRN_FREE(previous_offsets);
        }
        GRN_FREE(previous_normalized);
      }
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    string_->original = str;
    string_->original_length_in_bytes = str_len;
    string_->normalizer_index = 0;
    GRN_OBJ_FIN(ctx, &normalizers);
  }
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj_close(ctx, string);
    string = NULL;
  }

  return string;
}

grn_obj *
grn_string_open(grn_ctx *ctx,
                const char *str,
                unsigned int str_len,
                grn_obj *lexicon_or_normalizer,
                int flags)
{
  return grn_string_open_(ctx,
                          str,
                          str_len,
                          lexicon_or_normalizer,
                          flags,
                          ctx->encoding);
}

grn_rc
grn_string_get_original(grn_ctx *ctx, grn_obj *string,
                        const char **original,
                        unsigned int *length_in_bytes)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    if (original) { *original = string_->original; }
    if (length_in_bytes) {
      *length_in_bytes = string_->original_length_in_bytes;
    }
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_string_set_original(grn_ctx *ctx,
                        grn_obj *string,
                        const char *original,
                        unsigned int length_in_bytes)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    grn_string_fin(ctx, string);
    string_->original = original;
    string_->original_length_in_bytes = length_in_bytes;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

int
grn_string_get_flags(grn_ctx *ctx, grn_obj *string)
{
  int flags = 0;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    flags = string_->flags;
  }
  GRN_API_RETURN(flags);
}

grn_rc
grn_string_get_normalized(grn_ctx *ctx, grn_obj *string,
                          const char **normalized,
                          unsigned int *length_in_bytes,
                          unsigned int *n_characters)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    if (normalized) { *normalized = string_->normalized; }
    if (length_in_bytes) {
      *length_in_bytes = string_->normalized_length_in_bytes;
    }
    if (n_characters) { *n_characters = string_->n_characters; }
    rc = GRN_SUCCESS;
  } else {
    if (normalized) { *normalized = NULL; }
    if (length_in_bytes) { *length_in_bytes = 0; }
    if (n_characters) { *n_characters = 0; }
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_string_set_normalized(grn_ctx *ctx, grn_obj *string,
                          char *normalized, unsigned int length_in_bytes,
                          unsigned int n_characters)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    if (string_->normalized) { GRN_FREE(string_->normalized); }
    string_->normalized = normalized;
    string_->normalized_length_in_bytes = length_in_bytes;
    string_->n_characters = n_characters;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

const int16_t *
grn_string_get_checks(grn_ctx *ctx, grn_obj *string)
{
  int16_t *checks = NULL;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    checks = string_->checks;
  } else {
    checks = NULL;
  }
  GRN_API_RETURN(checks);
}

grn_rc
grn_string_set_checks(grn_ctx *ctx, grn_obj *string, int16_t *checks)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    if (string_->checks) { GRN_FREE(string_->checks); }
    string_->checks = checks;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

const uint8_t *
grn_string_get_types(grn_ctx *ctx, grn_obj *string)
{
  uint8_t *types = NULL;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    types = string_->ctypes;
  } else {
    types = NULL;
  }
  GRN_API_RETURN(types);
}

grn_rc
grn_string_set_types(grn_ctx *ctx, grn_obj *string, uint8_t *types)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    if (string_->ctypes) { GRN_FREE(string_->ctypes); }
    string_->ctypes = types;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

const uint64_t *
grn_string_get_offsets(grn_ctx *ctx, grn_obj *string)
{
  uint64_t *offsets = NULL;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    offsets = string_->offsets;
  } else {
    offsets = NULL;
  }
  GRN_API_RETURN(offsets);
}

grn_rc
grn_string_set_offsets(grn_ctx *ctx, grn_obj *string, uint64_t *offsets)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    if (string_->offsets) { GRN_FREE(string_->offsets); }
    string_->offsets = offsets;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

grn_encoding
grn_string_get_encoding(grn_ctx *ctx, grn_obj *string)
{
  grn_encoding encoding = GRN_ENC_NONE;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    encoding = string_->encoding;
  }
  GRN_API_RETURN(encoding);
}

grn_obj *
grn_string_get_table(grn_ctx *ctx, grn_obj *string)
{
  grn_obj *table = NULL;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    table = string_->lexicon;
  }
  GRN_API_RETURN(table);
}

grn_rc
grn_string_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *string)
{
  grn_string *string_ = (grn_string *)string;

  GRN_TEXT_PUTS(ctx, buffer, "#<string:");

  GRN_TEXT_PUTS(ctx, buffer, " original:<");
  GRN_TEXT_PUT(ctx, buffer,
               string_->original,
               string_->original_length_in_bytes);
  GRN_TEXT_PUTS(ctx, buffer, ">");
  GRN_TEXT_PUTS(ctx, buffer, "(");
  grn_text_itoa(ctx, buffer, string_->original_length_in_bytes);
  GRN_TEXT_PUTS(ctx, buffer, ")");

  GRN_TEXT_PUTS(ctx, buffer, " normalized:<");
  GRN_TEXT_PUT(ctx, buffer,
               string_->normalized,
               string_->normalized_length_in_bytes);
  GRN_TEXT_PUTS(ctx, buffer, ">");
  GRN_TEXT_PUTS(ctx, buffer, "(");
  grn_text_itoa(ctx, buffer, string_->normalized_length_in_bytes);
  GRN_TEXT_PUTS(ctx, buffer, ")");

  GRN_TEXT_PUTS(ctx, buffer, " n_characters:");
  grn_text_itoa(ctx, buffer, string_->n_characters);

  GRN_TEXT_PUTS(ctx, buffer, " encoding:");
  grn_inspect_encoding(ctx, buffer, string_->encoding);

  GRN_TEXT_PUTS(ctx, buffer, " flags:");
  if (string_->flags & GRN_STRING_REMOVE_BLANK) {
  GRN_TEXT_PUTS(ctx, buffer, "REMOVE_BLANK|");
  }
  if (string_->flags & GRN_STRING_WITH_TYPES) {
    GRN_TEXT_PUTS(ctx, buffer, "WITH_TYPES|");
  }
  if (string_->flags & GRN_STRING_WITH_CHECKS) {
    GRN_TEXT_PUTS(ctx, buffer, "WITH_CHECKS|");
  }
  if (string_->flags & GRN_STRING_REMOVE_TOKENIZED_DELIMITER) {
    GRN_TEXT_PUTS(ctx, buffer, "REMOVE_TOKENIZED_DELIMITER|");
  }
  if (GRN_TEXT_VALUE(buffer)[GRN_TEXT_LEN(buffer) - 1] == '|') {
    grn_bulk_truncate(ctx, buffer, GRN_TEXT_LEN(buffer) - 1);
  }

  GRN_TEXT_PUTS(ctx, buffer, ">");

  return GRN_SUCCESS;
}

uint32_t
grn_string_get_normalizer_index(grn_ctx *ctx, grn_obj *string)
{
  uint32_t index = 0;
  grn_string *string_ = (grn_string *)string;
  GRN_API_ENTER;
  if (string_) {
    index = string_->normalizer_index;
  }
  GRN_API_RETURN(index);
}

grn_rc
grn_string_fin(grn_ctx *ctx, grn_obj *string)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  if (string_) {
    if (string_->normalized) {
      GRN_FREE(string_->normalized);
      string_->normalized = NULL;
      string_->normalized_length_in_bytes = 0;
      string_->n_characters = 0;
    }
    if (string_->checks) {
      GRN_FREE(string_->checks);
      string_->checks = NULL;
    }
    if (string_->ctypes) {
      GRN_FREE(string_->ctypes);
      string_->ctypes = NULL;
    }
    if (string_->offsets) {
      GRN_FREE(string_->offsets);
      string_->ctypes = NULL;
    }
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  return rc;
}

grn_rc
grn_string_close(grn_ctx *ctx, grn_obj *string)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  if (string_) {
    grn_string_fin(ctx, string);
    GRN_FREE(string);
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  return rc;
}
