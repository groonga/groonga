/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012 Brazil

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

#include "groonga_in.h"
#include <string.h>
#include "normalizer.h"
#include "str.h"

grn_id
grn_normalizer_find(grn_ctx *ctx, grn_encoding encoding)
{
  grn_id normalizer_id = GRN_ID_NIL;

  switch (encoding) {
  case GRN_ENC_EUC_JP :
    normalizer_id = GRN_DB_NORMALIZER_EUC_JP;
    break;
  case GRN_ENC_UTF8 :
#ifdef WITH_NFKC
    normalizer_id = GRN_DB_NORMALIZER_UTF8_NFKC;
#else /* WITH_NFKC */
    normalizer_id = GRN_DB_NORMALIZER_ASCII;
#endif /* WITH_NFKC */
    break;
  case GRN_ENC_SJIS :
    normalizer_id = GRN_DB_NORMALIZER_SJIS;
    break;
  case GRN_ENC_LATIN1 :
    normalizer_id = GRN_DB_NORMALIZER_LATIN1;
    break;
  case GRN_ENC_KOI8R :
    normalizer_id = GRN_DB_NORMALIZER_KOI8R;
    break;
  default :
    normalizer_id = GRN_DB_NORMALIZER_ASCII;
    break;
  }

  return normalizer_id;
}

grn_rc
grn_normalizer_init(void)
{
  return GRN_SUCCESS;
}

grn_rc
grn_normalizer_fin(void)
{
  return GRN_SUCCESS;
}

grn_obj *
grn_normalized_text_open(grn_ctx *ctx, grn_obj *normalizer,
                         const char *str, unsigned int str_len,
                         grn_encoding encoding, int flags)
{
  grn_normalized_text *normalized_text;
  grn_obj *obj;

  if (!normalizer) {
    return NULL;
  }

  normalized_text = GRN_MALLOCN(grn_normalized_text, 1);
  if (!normalized_text) {
    return NULL;
  }

  GRN_API_ENTER;
  obj = (grn_obj *)normalized_text;
  GRN_OBJ_INIT(obj, GRN_NORMALIZED_TEXT, GRN_OBJ_ALLOCATED, GRN_ID_NIL);
  normalized_text->orig = str;
  normalized_text->orig_blen = str_len;
  normalized_text->norm = NULL;
  normalized_text->norm_blen = 0;
  normalized_text->length = 0;
  normalized_text->checks = NULL;
  normalized_text->ctypes = NULL;
  normalized_text->encoding = encoding;
  normalized_text->flags = flags;

  ((grn_proc *)normalizer)->funcs[PROC_NEXT](ctx, 1, &obj, NULL);

  GRN_API_RETURN(obj);
}

grn_rc
grn_normalized_text_get_original_value(grn_ctx *ctx, grn_obj *normalized_text,
                                       const char **value,
                                       unsigned int *length_in_bytes)
{
  grn_rc rc;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    if (value) { *value = text->orig; }
    if (length_in_bytes) { *length_in_bytes = text->orig_blen; }
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

int
grn_normalized_text_get_flags(grn_ctx *ctx, grn_obj *normalized_text)
{
  int flags;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    flags = text->flags;
  } else {
    flags = 0;
  }
  GRN_API_RETURN(flags);
}

grn_rc
grn_normalized_text_get_value(grn_ctx *ctx, grn_obj *normalized_text,
                              const char **value, unsigned int *length,
                              unsigned int *length_in_bytes)
{
  grn_rc rc;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    if (value) { *value = text->norm; }
    if (length) { *length = text->length; }
    if (length_in_bytes) { *length_in_bytes = text->norm_blen; }
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_normalized_text_set_value(grn_ctx *ctx, grn_obj *normalized_text,
                              char *value, unsigned int length,
                              unsigned int length_in_bytes)
{
  grn_rc rc;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    if (text->norm) { GRN_FREE(text->norm); }
    text->norm = value;
    text->length = length;
    text->norm_blen = length_in_bytes;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

short *
grn_normalized_text_get_checks(grn_ctx *ctx, grn_obj *normalized_text)
{
  int16_t *checks = NULL;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    checks = text->checks;
  } else {
    checks = NULL;
  }
  GRN_API_RETURN(checks);
}

grn_rc
grn_normalized_text_set_checks(grn_ctx *ctx, grn_obj *normalized_text,
                               short *checks)
{
  grn_rc rc;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    if (text->checks) { GRN_FREE(text->checks); }
    text->checks = checks;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

unsigned char *
grn_normalized_text_get_types(grn_ctx *ctx, grn_obj *normalized_text)
{
  uint_least8_t *types = NULL;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    types = text->ctypes;
  } else {
    types = NULL;
  }
  GRN_API_RETURN(types);
}

grn_rc
grn_normalized_text_set_types(grn_ctx *ctx, grn_obj *normalized_text,
                              unsigned char *types)
{
  grn_rc rc;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  GRN_API_ENTER;
  if (text) {
    if (text->ctypes) { GRN_FREE(text->ctypes); }
    text->ctypes = types;
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_normalized_text_close(grn_ctx *ctx, grn_obj *normalized_text)
{
  grn_rc rc;
  grn_normalized_text *text = (grn_normalized_text *)normalized_text;
  if (text) {
    if (text->norm) { GRN_FREE(text->norm); }
    if (text->ctypes) { GRN_FREE(text->ctypes); }
    if (text->checks) { GRN_FREE(text->checks); }
    GRN_FREE(text);
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  return rc;
}

static unsigned char symbol[] = {
  ',', '.', 0, ':', ';', '?', '!', 0, 0, 0, '`', 0, '^', '~', '_', 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, '-', '-', '/', '\\', 0, 0, '|', 0, 0, 0, '\'', 0,
  '"', '(', ')', 0, 0, '[', ']', '{', '}', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '+', '-', 0, 0, 0, '=', 0, '<', '>', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '$', 0, 0, '%', '#', '&', '*', '@', 0, 0, 0, 0, 0, 0, 0, 0
};

inline static grn_obj *
eucjp_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
                grn_user_data *user_data)
{
  static uint16_t hankana[] = {
    0xa1a1, 0xa1a3, 0xa1d6, 0xa1d7, 0xa1a2, 0xa1a6, 0xa5f2, 0xa5a1, 0xa5a3,
    0xa5a5, 0xa5a7, 0xa5a9, 0xa5e3, 0xa5e5, 0xa5e7, 0xa5c3, 0xa1bc, 0xa5a2,
    0xa5a4, 0xa5a6, 0xa5a8, 0xa5aa, 0xa5ab, 0xa5ad, 0xa5af, 0xa5b1, 0xa5b3,
    0xa5b5, 0xa5b7, 0xa5b9, 0xa5bb, 0xa5bd, 0xa5bf, 0xa5c1, 0xa5c4, 0xa5c6,
    0xa5c8, 0xa5ca, 0xa5cb, 0xa5cc, 0xa5cd, 0xa5ce, 0xa5cf, 0xa5d2, 0xa5d5,
    0xa5d8, 0xa5db, 0xa5de, 0xa5df, 0xa5e0, 0xa5e1, 0xa5e2, 0xa5e4, 0xa5e6,
    0xa5e8, 0xa5e9, 0xa5ea, 0xa5eb, 0xa5ec, 0xa5ed, 0xa5ef, 0xa5f3, 0xa1ab,
    0xa1eb
  };
  static unsigned char dakuten[] = {
    0xf4, 0, 0, 0, 0, 0xac, 0, 0xae, 0, 0xb0, 0, 0xb2, 0, 0xb4, 0, 0xb6, 0,
    0xb8, 0, 0xba, 0, 0xbc, 0, 0xbe, 0, 0xc0, 0, 0xc2, 0, 0, 0xc5, 0, 0xc7,
    0, 0xc9, 0, 0, 0, 0, 0, 0, 0xd0, 0, 0, 0xd3, 0, 0, 0xd6, 0, 0, 0xd9, 0,
    0, 0xdc
  };
  static unsigned char handaku[] = {
    0xd1, 0, 0, 0xd4, 0, 0, 0xd7, 0, 0, 0xda, 0, 0, 0xdd
  };
  grn_normalized_text *nstr = (grn_normalized_text *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_, b;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->orig_blen, length = 0;
  int removeblankp = nstr->flags & GRN_NORMALIZE_REMOVE_BLANK;
  if (!(nstr->norm = GRN_MALLOC(size * 2 + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][eucjp] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->norm;
  if (nstr->flags & GRN_NORMALIZE_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * 2 * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][eucjp] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_NORMALIZE_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->norm);
      nstr->checks = NULL;
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][eucjp] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->orig + size;
  for (s = s_ = (unsigned char *) nstr->orig, d = d_ = d0; s < e; s++) {
    if ((*s & 0x80)) {
      if (((s + 1) < e) && (*(s + 1) & 0x80)) {
        unsigned char c1 = *s++, c2 = *s, c3 = 0;
        switch (c1 >> 4) {
        case 0x08 :
          if (c1 == 0x8e && 0xa0 <= c2 && c2 <= 0xdf) {
            uint16_t c = hankana[c2 - 0xa0];
            switch (c) {
            case 0xa1ab :
              if (d > d0 + 1 && d[-2] == 0xa5
                  && 0xa6 <= d[-1] && d[-1] <= 0xdb && (b = dakuten[d[-1] - 0xa6])) {
                *(d - 1) = b;
                if (ch) { ch[-1] += 2; s_ += 2; }
                continue;
              } else {
                *d++ = c >> 8; *d = c & 0xff;
              }
              break;
            case 0xa1eb :
              if (d > d0 + 1 && d[-2] == 0xa5
                  && 0xcf <= d[-1] && d[-1] <= 0xdb && (b = handaku[d[-1] - 0xcf])) {
                *(d - 1) = b;
                if (ch) { ch[-1] += 2; s_ += 2; }
                continue;
              } else {
                *d++ = c >> 8; *d = c & 0xff;
              }
              break;
            default :
              *d++ = c >> 8; *d = c & 0xff;
              break;
            }
            ctype = grn_char_katakana;
          } else {
            *d++ = c1; *d = c2;
            ctype = grn_char_others;
          }
          break;
        case 0x09 :
          *d++ = c1; *d = c2;
          ctype = grn_char_others;
          break;
        case 0x0a :
          switch (c1 & 0x0f) {
          case 1 :
            switch (c2) {
            case 0xbc :
              *d++ = c1; *d = c2;
              ctype = grn_char_katakana;
              break;
            case 0xb9 :
              *d++ = c1; *d = c2;
              ctype = grn_char_kanji;
              break;
            case 0xa1 :
              if (removeblankp) {
                if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
                continue;
              } else {
                *d = ' ';
                ctype = GRN_CHAR_BLANK|grn_char_symbol;
              }
              break;
            default :
              if (c2 >= 0xa4 && (c3 = symbol[c2 - 0xa4])) {
                *d = c3;
                ctype = grn_char_symbol;
              } else {
                *d++ = c1; *d = c2;
                ctype = grn_char_others;
              }
              break;
            }
            break;
          case 2 :
            *d++ = c1; *d = c2;
            ctype = grn_char_symbol;
            break;
          case 3 :
            c3 = c2 - 0x80;
            if ('a' <= c3 && c3 <= 'z') {
              ctype = grn_char_alpha;
              *d = c3;
            } else if ('A' <= c3 && c3 <= 'Z') {
              ctype = grn_char_alpha;
              *d = c3 + 0x20;
            } else if ('0' <= c3 && c3 <= '9') {
              ctype = grn_char_digit;
              *d = c3;
            } else {
              ctype = grn_char_others;
              *d++ = c1; *d = c2;
            }
            break;
          case 4 :
            *d++ = c1; *d = c2;
            ctype = grn_char_hiragana;
            break;
          case 5 :
            *d++ = c1; *d = c2;
            ctype = grn_char_katakana;
            break;
          case 6 :
          case 7 :
          case 8 :
            *d++ = c1; *d = c2;
            ctype = grn_char_symbol;
            break;
          default :
            *d++ = c1; *d = c2;
            ctype = grn_char_others;
            break;
          }
          break;
        default :
          *d++ = c1; *d = c2;
          ctype = grn_char_kanji;
          break;
        }
      } else {
        /* skip invalid character */
        continue;
      }
    } else {
      unsigned char c = *s;
      switch (c >> 4) {
      case 0 :
      case 1 :
        /* skip unprintable ascii */
        if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
        continue;
      case 2 :
        if (c == 0x20) {
          if (removeblankp) {
            if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
            continue;
          } else {
            *d = ' ';
            ctype = GRN_CHAR_BLANK|grn_char_symbol;
          }
        } else {
          *d = c;
          ctype = grn_char_symbol;
        }
        break;
      case 3 :
        *d = c;
        ctype = (c <= 0x39) ? grn_char_digit : grn_char_symbol;
        break;
      case 4 :
        *d = ('A' <= c) ? c + 0x20 : c;
        ctype = (c == 0x40) ? grn_char_symbol : grn_char_alpha;
        break;
      case 5 :
        *d = (c <= 'Z') ? c + 0x20 : c;
        ctype = (c <= 0x5a) ? grn_char_alpha : grn_char_symbol;
        break;
      case 6 :
        *d = c;
        ctype = (c == 0x60) ? grn_char_symbol : grn_char_alpha;
        break;
      case 7 :
        *d = c;
        ctype = (c <= 0x7a) ? grn_char_alpha : (c == 0x7f ? grn_char_others : grn_char_symbol);
        break;
      default :
        *d = c;
        ctype = grn_char_others;
        break;
      }
    }
    d++;
    length++;
    if (cp) { *cp++ = ctype; }
    if (ch) {
      *ch++ = (int16_t)(s + 1 - s_);
      s_ = s + 1;
      while (++d_ < d) { *ch++ = 0; }
    }
  }
  if (cp) { *cp = grn_char_null; }
  *d = '\0';
  nstr->length = length;
  nstr->norm_blen = (size_t)(d - (unsigned char *)nstr->norm);
  return NULL;
}

inline static grn_obj *
sjis_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
               grn_user_data *user_data)
{
  static uint16_t hankana[] = {
    0x8140, 0x8142, 0x8175, 0x8176, 0x8141, 0x8145, 0x8392, 0x8340, 0x8342,
    0x8344, 0x8346, 0x8348, 0x8383, 0x8385, 0x8387, 0x8362, 0x815b, 0x8341,
    0x8343, 0x8345, 0x8347, 0x8349, 0x834a, 0x834c, 0x834e, 0x8350, 0x8352,
    0x8354, 0x8356, 0x8358, 0x835a, 0x835c, 0x835e, 0x8360, 0x8363, 0x8365,
    0x8367, 0x8369, 0x836a, 0x836b, 0x836c, 0x836d, 0x836e, 0x8371, 0x8374,
    0x8377, 0x837a, 0x837d, 0x837e, 0x8380, 0x8381, 0x8382, 0x8384, 0x8386,
    0x8388, 0x8389, 0x838a, 0x838b, 0x838c, 0x838d, 0x838f, 0x8393, 0x814a,
    0x814b
  };
  static unsigned char dakuten[] = {
    0x94, 0, 0, 0, 0, 0x4b, 0, 0x4d, 0, 0x4f, 0, 0x51, 0, 0x53, 0, 0x55, 0,
    0x57, 0, 0x59, 0, 0x5b, 0, 0x5d, 0, 0x5f, 0, 0x61, 0, 0, 0x64, 0, 0x66,
    0, 0x68, 0, 0, 0, 0, 0, 0, 0x6f, 0, 0, 0x72, 0, 0, 0x75, 0, 0, 0x78, 0,
    0, 0x7b
  };
  static unsigned char handaku[] = {
    0x70, 0, 0, 0x73, 0, 0, 0x76, 0, 0, 0x79, 0, 0, 0x7c
  };
  grn_normalized_text *nstr = (grn_normalized_text *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_;
  unsigned char *d, *d0, *d_, b, *e;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->orig_blen, length = 0;
  int removeblankp = nstr->flags & GRN_NORMALIZE_REMOVE_BLANK;
  if (!(nstr->norm = GRN_MALLOC(size * 2 + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][sjis] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->norm;
  if (nstr->flags & GRN_NORMALIZE_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * 2 * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][sjis] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_NORMALIZE_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->norm);
      nstr->checks = NULL;
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][sjis] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->orig + size;
  for (s = s_ = (unsigned char *) nstr->orig, d = d_ = d0; s < e; s++) {
    if ((*s & 0x80)) {
      if (0xa0 <= *s && *s <= 0xdf) {
        uint16_t c = hankana[*s - 0xa0];
        switch (c) {
        case 0x814a :
          if (d > d0 + 1 && d[-2] == 0x83
              && 0x45 <= d[-1] && d[-1] <= 0x7a && (b = dakuten[d[-1] - 0x45])) {
            *(d - 1) = b;
            if (ch) { ch[-1]++; s_++; }
            continue;
          } else {
            *d++ = c >> 8; *d = c & 0xff;
          }
          break;
        case 0x814b :
          if (d > d0 + 1 && d[-2] == 0x83
              && 0x6e <= d[-1] && d[-1] <= 0x7a && (b = handaku[d[-1] - 0x6e])) {
            *(d - 1) = b;
            if (ch) { ch[-1]++; s_++; }
            continue;
          } else {
            *d++ = c >> 8; *d = c & 0xff;
          }
          break;
        default :
          *d++ = c >> 8; *d = c & 0xff;
          break;
        }
        ctype = grn_char_katakana;
      } else {
        if ((s + 1) < e && 0x40 <= *(s + 1) && *(s + 1) <= 0xfc) {
          unsigned char c1 = *s++, c2 = *s, c3 = 0;
          if (0x81 <= c1 && c1 <= 0x87) {
            switch (c1 & 0x0f) {
            case 1 :
              switch (c2) {
              case 0x5b :
                *d++ = c1; *d = c2;
                ctype = grn_char_katakana;
                break;
              case 0x58 :
                *d++ = c1; *d = c2;
                ctype = grn_char_kanji;
                break;
              case 0x40 :
                if (removeblankp) {
                  if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
                  continue;
                } else {
                  *d = ' ';
                  ctype = GRN_CHAR_BLANK|grn_char_symbol;
                }
                break;
              default :
                if (0x43 <= c2 && c2 <= 0x7e && (c3 = symbol[c2 - 0x43])) {
                  *d = c3;
                  ctype = grn_char_symbol;
                } else if (0x7f <= c2 && c2 <= 0x97 && (c3 = symbol[c2 - 0x44])) {
                  *d = c3;
                  ctype = grn_char_symbol;
                } else {
                  *d++ = c1; *d = c2;
                  ctype = grn_char_others;
                }
                break;
              }
              break;
            case 2 :
              c3 = c2 - 0x1f;
              if (0x4f <= c2 && c2 <= 0x58) {
                ctype = grn_char_digit;
                *d = c2 - 0x1f;
              } else if (0x60 <= c2 && c2 <= 0x79) {
                ctype = grn_char_alpha;
                *d = c2 + 0x01;
              } else if (0x81 <= c2 && c2 <= 0x9a) {
                ctype = grn_char_alpha;
                *d = c2 - 0x20;
              } else if (0x9f <= c2 && c2 <= 0xf1) {
                *d++ = c1; *d = c2;
                ctype = grn_char_hiragana;
              } else {
                *d++ = c1; *d = c2;
                ctype = grn_char_others;
              }
              break;
            case 3 :
              if (0x40 <= c2 && c2 <= 0x96) {
                *d++ = c1; *d = c2;
                ctype = grn_char_katakana;
              } else {
                *d++ = c1; *d = c2;
                ctype = grn_char_symbol;
              }
              break;
            case 4 :
            case 7 :
              *d++ = c1; *d = c2;
              ctype = grn_char_symbol;
              break;
            default :
              *d++ = c1; *d = c2;
              ctype = grn_char_others;
              break;
            }
          } else {
            *d++ = c1; *d = c2;
            ctype = grn_char_kanji;
          }
        } else {
          /* skip invalid character */
          continue;
        }
      }
    } else {
      unsigned char c = *s;
      switch (c >> 4) {
      case 0 :
      case 1 :
        /* skip unprintable ascii */
        if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
        continue;
      case 2 :
        if (c == 0x20) {
          if (removeblankp) {
            if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
            continue;
          } else {
            *d = ' ';
            ctype = GRN_CHAR_BLANK|grn_char_symbol;
          }
        } else {
          *d = c;
          ctype = grn_char_symbol;
        }
        break;
      case 3 :
        *d = c;
        ctype = (c <= 0x39) ? grn_char_digit : grn_char_symbol;
        break;
      case 4 :
        *d = ('A' <= c) ? c + 0x20 : c;
        ctype = (c == 0x40) ? grn_char_symbol : grn_char_alpha;
        break;
      case 5 :
        *d = (c <= 'Z') ? c + 0x20 : c;
        ctype = (c <= 0x5a) ? grn_char_alpha : grn_char_symbol;
        break;
      case 6 :
        *d = c;
        ctype = (c == 0x60) ? grn_char_symbol : grn_char_alpha;
        break;
      case 7 :
        *d = c;
        ctype = (c <= 0x7a) ? grn_char_alpha : (c == 0x7f ? grn_char_others : grn_char_symbol);
        break;
      default :
        *d = c;
        ctype = grn_char_others;
        break;
      }
    }
    d++;
    length++;
    if (cp) { *cp++ = ctype; }
    if (ch) {
      *ch++ = (int16_t)(s + 1 - s_);
      s_ = s + 1;
      while (++d_ < d) { *ch++ = 0; }
    }
  }
  if (cp) { *cp = grn_char_null; }
  *d = '\0';
  nstr->length = length;
  nstr->norm_blen = (size_t)(d - (unsigned char *)nstr->norm);
  return NULL;
}

inline static grn_obj *
ascii_normalize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_normalized_text *nstr = (grn_normalized_text *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->orig_blen, length = 0;
  int removeblankp = nstr->flags & GRN_NORMALIZE_REMOVE_BLANK;
  if (!(nstr->norm = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][ascii] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->norm;
  if (nstr->flags & GRN_NORMALIZE_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][ascii] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_NORMALIZE_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->norm);
      nstr->checks = NULL;
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][ascii] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->orig + size;
  for (s = s_ = (unsigned char *) nstr->orig, d = d_ = d0; s < e; s++) {
    unsigned char c = *s;
    switch (c >> 4) {
    case 0 :
    case 1 :
      /* skip unprintable ascii */
      if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
      continue;
    case 2 :
      if (c == 0x20) {
        if (removeblankp) {
          if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
          continue;
        } else {
          *d = ' ';
          ctype = GRN_CHAR_BLANK|grn_char_symbol;
        }
      } else {
        *d = c;
        ctype = grn_char_symbol;
      }
      break;
    case 3 :
      *d = c;
      ctype = (c <= 0x39) ? grn_char_digit : grn_char_symbol;
      break;
    case 4 :
      *d = ('A' <= c) ? c + 0x20 : c;
      ctype = (c == 0x40) ? grn_char_symbol : grn_char_alpha;
      break;
    case 5 :
      *d = (c <= 'Z') ? c + 0x20 : c;
      ctype = (c <= 0x5a) ? grn_char_alpha : grn_char_symbol;
      break;
    case 6 :
      *d = c;
      ctype = (c == 0x60) ? grn_char_symbol : grn_char_alpha;
      break;
    case 7 :
      *d = c;
      ctype = (c <= 0x7a) ? grn_char_alpha : (c == 0x7f ? grn_char_others : grn_char_symbol);
      break;
    default :
      *d = c;
      ctype = grn_char_others;
      break;
    }
    d++;
    length++;
    if (cp) { *cp++ = ctype; }
    if (ch) {
      *ch++ = (int16_t)(s + 1 - s_);
      s_ = s + 1;
      while (++d_ < d) { *ch++ = 0; }
    }
  }
  if (cp) { *cp = grn_char_null; }
  *d = '\0';
  nstr->length = length;
  nstr->norm_blen = (size_t)(d - (unsigned char *)nstr->norm);
  return NULL;
}

/* use cp1252 as latin1 */
inline static grn_obj *
latin1_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
                 grn_user_data *user_data)
{
  grn_normalized_text *nstr = (grn_normalized_text *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = strlen(nstr->orig), length = 0;
  int removeblankp = nstr->flags & GRN_NORMALIZE_REMOVE_BLANK;
  if (!(nstr->norm = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][latin1] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->norm;
  if (nstr->flags & GRN_NORMALIZE_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][latin1] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_NORMALIZE_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->norm);
      nstr->checks = NULL;
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][latin1] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->orig + size;
  for (s = s_ = (unsigned char *) nstr->orig, d = d_ = d0; s < e; s++) {
    unsigned char c = *s;
    switch (c >> 4) {
    case 0 :
    case 1 :
      /* skip unprintable ascii */
      if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
      continue;
    case 2 :
      if (c == 0x20) {
        if (removeblankp) {
          if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
          continue;
        } else {
          *d = ' ';
          ctype = GRN_CHAR_BLANK|grn_char_symbol;
        }
      } else {
        *d = c;
        ctype = grn_char_symbol;
      }
      break;
    case 3 :
      *d = c;
      ctype = (c <= 0x39) ? grn_char_digit : grn_char_symbol;
      break;
    case 4 :
      *d = ('A' <= c) ? c + 0x20 : c;
      ctype = (c == 0x40) ? grn_char_symbol : grn_char_alpha;
      break;
    case 5 :
      *d = (c <= 'Z') ? c + 0x20 : c;
      ctype = (c <= 0x5a) ? grn_char_alpha : grn_char_symbol;
      break;
    case 6 :
      *d = c;
      ctype = (c == 0x60) ? grn_char_symbol : grn_char_alpha;
      break;
    case 7 :
      *d = c;
      ctype = (c <= 0x7a) ? grn_char_alpha : (c == 0x7f ? grn_char_others : grn_char_symbol);
      break;
    case 8 :
      if (c == 0x8a || c == 0x8c || c == 0x8e) {
        *d = c + 0x10;
        ctype = grn_char_alpha;
      } else {
        *d = c;
        ctype = grn_char_symbol;
      }
      break;
    case 9 :
      if (c == 0x9a || c == 0x9c || c == 0x9e || c == 0x9f) {
        *d = (c == 0x9f) ? c + 0x60 : c;
        ctype = grn_char_alpha;
      } else {
        *d = c;
        ctype = grn_char_symbol;
      }
      break;
    case 0x0c :
      *d = c + 0x20;
      ctype = grn_char_alpha;
      break;
    case 0x0d :
      *d = (c == 0xd7 || c == 0xdf) ? c : c + 0x20;
      ctype = (c == 0xd7) ? grn_char_symbol : grn_char_alpha;
      break;
    case 0x0e :
      *d = c;
      ctype = grn_char_alpha;
      break;
    case 0x0f :
      *d = c;
      ctype = (c == 0xf7) ? grn_char_symbol : grn_char_alpha;
      break;
    default :
      *d = c;
      ctype = grn_char_others;
      break;
    }
    d++;
    length++;
    if (cp) { *cp++ = ctype; }
    if (ch) {
      *ch++ = (int16_t)(s + 1 - s_);
      s_ = s + 1;
      while (++d_ < d) { *ch++ = 0; }
    }
  }
  if (cp) { *cp = grn_char_null; }
  *d = '\0';
  nstr->length = length;
  nstr->norm_blen = (size_t)(d - (unsigned char *)nstr->norm);
  return NULL;
}

inline static grn_obj *
koi8r_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
                grn_user_data *user_data)
{
  grn_normalized_text *nstr = (grn_normalized_text *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = strlen(nstr->orig), length = 0;
  int removeblankp = nstr->flags & GRN_NORMALIZE_REMOVE_BLANK;
  if (!(nstr->norm = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][koi8r] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->norm;
  if (nstr->flags & GRN_NORMALIZE_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][koi8r] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_NORMALIZE_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->norm);
      nstr->checks = NULL;
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][koi8r] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->orig + size;
  for (s = s_ = (unsigned char *) nstr->orig, d = d_ = d0; s < e; s++) {
    unsigned char c = *s;
    switch (c >> 4) {
    case 0 :
    case 1 :
      /* skip unprintable ascii */
      if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
      continue;
    case 2 :
      if (c == 0x20) {
        if (removeblankp) {
          if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
          continue;
        } else {
          *d = ' ';
          ctype = GRN_CHAR_BLANK|grn_char_symbol;
        }
      } else {
        *d = c;
        ctype = grn_char_symbol;
      }
      break;
    case 3 :
      *d = c;
      ctype = (c <= 0x39) ? grn_char_digit : grn_char_symbol;
      break;
    case 4 :
      *d = ('A' <= c) ? c + 0x20 : c;
      ctype = (c == 0x40) ? grn_char_symbol : grn_char_alpha;
      break;
    case 5 :
      *d = (c <= 'Z') ? c + 0x20 : c;
      ctype = (c <= 0x5a) ? grn_char_alpha : grn_char_symbol;
      break;
    case 6 :
      *d = c;
      ctype = (c == 0x60) ? grn_char_symbol : grn_char_alpha;
      break;
    case 7 :
      *d = c;
      ctype = (c <= 0x7a) ? grn_char_alpha : (c == 0x7f ? grn_char_others : grn_char_symbol);
      break;
    case 0x0a :
      *d = c;
      ctype = (c == 0xa3) ? grn_char_alpha : grn_char_others;
      break;
    case 0x0b :
      if (c == 0xb3) {
        *d = c - 0x10;
        ctype = grn_char_alpha;
      } else {
        *d = c;
        ctype = grn_char_others;
      }
      break;
    case 0x0c :
    case 0x0d :
      *d = c;
      ctype = grn_char_alpha;
      break;
    case 0x0e :
    case 0x0f :
      *d = c - 0x20;
      ctype = grn_char_alpha;
      break;
    default :
      *d = c;
      ctype = grn_char_others;
      break;
    }
    d++;
    length++;
    if (cp) { *cp++ = ctype; }
    if (ch) {
      *ch++ = (int16_t)(s + 1 - s_);
      s_ = s + 1;
      while (++d_ < d) { *ch++ = 0; }
    }
  }
  if (cp) { *cp = grn_char_null; }
  *d = '\0';
  nstr->length = length;
  nstr->norm_blen = (size_t)(d - (unsigned char *)nstr->norm);
  return NULL;
}

#define DEF_NORMALIZERIZER(name, normalize)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1),\
                   GRN_PROC_NORMALIZER, NULL, (normalize), NULL, 0, NULL))

grn_rc
grn_db_init_builtin_normalizers(grn_ctx *ctx)
{
  grn_obj *obj;

  obj = DEF_NORMALIZERIZER("NormalizerASCII", ascii_normalize);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_NORMALIZER_ASCII) {
    return GRN_FILE_CORRUPT;
  }
#ifdef WITH_NFKC
  if (grn_plugin_register(ctx, "normalizers/nfkc")) {
    ERRCLR(ctx);
#endif
    grn_obj_register(ctx, grn_ctx_db(ctx), "NormalizerUTF8NFKC", 18);
#ifdef WITH_NFKC
  }
#endif
  obj = DEF_NORMALIZERIZER("NormalizerEUCJP", eucjp_normalize);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_NORMALIZER_EUC_JP) {
    return GRN_FILE_CORRUPT;
  }
  obj = DEF_NORMALIZERIZER("NormalizerSJIS", sjis_normalize);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_NORMALIZER_SJIS) {
    return GRN_FILE_CORRUPT;
  }
  obj = DEF_NORMALIZERIZER("NormalizerLATIN1", latin1_normalize);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_NORMALIZER_LATIN1) {
    return GRN_FILE_CORRUPT;
  }
  obj = DEF_NORMALIZERIZER("NormalizerKOI8R", koi8r_normalize);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_NORMALIZER_KOI8R) {
    return GRN_FILE_CORRUPT;
  }
  /* obj = DEF_NORMALIZERIZER("NormalizerUTF8UCA", utf8_uca_normalize); */
  /* if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_NORMALIZER_UTF8_UCA) { */
  /*   return GRN_FILE_CORRUPT; */
  /* } */

  return GRN_SUCCESS;
}
