/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2012 Brazil

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
#include "string_in.h"
#include "str.h"

#include <groonga/tokenizer.h>

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
  grn_string *nstr = (grn_string *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_, b;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size * 2 + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][eucjp] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * 2 * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][eucjp] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STRING_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->normalized);
      nstr->checks = NULL;
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][eucjp] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->original + size;
  for (s = s_ = (unsigned char *) nstr->original, d = d_ = d0; s < e; s++) {
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
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
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
  grn_string *nstr = (grn_string *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_;
  unsigned char *d, *d0, *d_, b, *e;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size * 2 + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][sjis] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * 2 * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][sjis] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STRING_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->normalized);
      nstr->checks = NULL;
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][sjis] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->original + size;
  for (s = s_ = (unsigned char *) nstr->original, d = d_ = d0; s < e; s++) {
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
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

#ifdef WITH_NFKC
uint_least8_t grn_nfkc_ctype(const unsigned char *str);
const char *grn_nfkc_map1(const unsigned char *str);
const char *grn_nfkc_map2(const unsigned char *prefix, const unsigned char *suffix);

static inline int
grn_str_charlen_utf8(grn_ctx *ctx, const unsigned char *str, const unsigned char *end)
{
  /* MEMO: This function allows non-null-terminated string as str. */
  /*       But requires the end of string. */
  const unsigned char *p = str;
  if (end <= p || !*p) { return 0; }
  if (*p & 0x80) {
    int b, w;
    int size;
    int i;
    for (b = 0x40, w = 0; b && (*p & b); b >>= 1, w++);
    if (!w) {
      GRN_LOG(ctx, GRN_LOG_WARNING,
              "invalid utf8 string: the first bit is 0x80: <%.*s>: <%.*s>",
              (int)(end - p), p,
              (int)(end - str), str);
      return 0;
    }
    size = w + 1;
    for (i = 1; i < size; i++) {
      if (++p >= end) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "invalid utf8 string: too short: "
                "%d byte is required but %d byte is given: <%.*s>",
                size, i,
                (int)(end - str), str);
        return 0;
      }
      if (!*p) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "invalid utf8 string: NULL character is found: <%.*s>",
                (int)(end - str), str);
        return 0;
      }
      if ((*p & 0xc0) != 0x80) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "invalid utf8 string: 0x80 is not allowed: <%.*s>: <%.*s>",
                (int)(end - p), p,
                (int)(end - str), str);
        return 0;
      }
    }
    return size;
  } else {
    return 1;
  }
  return 0;
}

inline static grn_obj *
utf8_normalize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int16_t *ch;
  const unsigned char *s, *s_, *s__ = NULL, *p, *p2, *pe, *e;
  unsigned char *d, *d_, *de;
  uint_least8_t *cp;
  grn_string *nstr = (grn_string *)args[0];
  size_t length = 0, ls, lp, size = nstr->original_length_in_bytes, ds = size * 3;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  grn_bool remove_tokenized_delimiter_p =
    nstr->flags & GRN_STRING_REMOVE_TOKENIZED_DELIMITER;
  if (!(nstr->normalized = GRN_MALLOC(ds + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][utf8] failed to allocate normalized text space");
    return NULL;
  }
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(ds * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][utf8] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STRING_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(ds + 1))) {
      if (nstr->checks) { GRN_FREE(nstr->checks); nstr->checks = NULL; }
      GRN_FREE(nstr->normalized); nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][utf8] failed to allocate character types space");
      return NULL;
    }
  }
  cp = nstr->ctypes;
  d = (unsigned char *)nstr->normalized;
  de = d + ds;
  d_ = NULL;
  e = (unsigned char *)nstr->original + size;
  for (s = s_ = (unsigned char *)nstr->original; ; s += ls) {
    if (!(ls = grn_str_charlen_utf8(ctx, s, e))) {
      break;
    }
    if (remove_tokenized_delimiter_p &&
        grn_tokenizer_is_tokenized_delimiter(ctx, s, ls, GRN_ENC_UTF8)) {
      continue;
    }
    if ((p = (unsigned char *)grn_nfkc_map1(s))) {
      pe = p + strlen((char *)p);
    } else {
      p = s;
      pe = p + ls;
    }
    if (d_ && (p2 = (unsigned char *)grn_nfkc_map2(d_, p))) {
      p = p2;
      pe = p + strlen((char *)p);
      if (cp) { cp--; }
      if (ch) {
        ch -= (d - d_);
        if (ch[0] >= 0) {
          s_ = s__;
        }
      }
      d = d_;
      length--;
    }
    for (; ; p += lp) {
      if (!(lp = grn_str_charlen_utf8(ctx, p, pe))) {
        break;
      }
      if ((*p == ' ' && removeblankp) || *p < 0x20  /* skip unprintable ascii */ ) {
        if (cp > nstr->ctypes) { *(cp - 1) |= GRN_STR_BLANK; }
      } else {
        if (de <= d + lp) {
          unsigned char *normalized;
          ds += (ds >> 1) + lp;
          if (!(normalized = GRN_REALLOC(nstr->normalized, ds + 1))) {
            if (nstr->ctypes) { GRN_FREE(nstr->ctypes); nstr->ctypes = NULL; }
            if (nstr->checks) { GRN_FREE(nstr->checks); nstr->checks = NULL; }
            GRN_FREE(nstr->normalized); nstr->normalized = NULL;
            ERR(GRN_NO_MEMORY_AVAILABLE,
                "[strinig][utf8] failed to expand normalized text space");
            return NULL;
          }
          de = normalized + ds;
          d = normalized + (d - (unsigned char *)nstr->normalized);
          nstr->normalized = normalized;
          if (ch) {
            int16_t *checks;
            if (!(checks = GRN_REALLOC(nstr->checks, ds * sizeof(int16_t)+ 1))) {
              if (nstr->ctypes) { GRN_FREE(nstr->ctypes); nstr->ctypes = NULL; }
              GRN_FREE(nstr->checks); nstr->checks = NULL;
              GRN_FREE(nstr->normalized); nstr->normalized = NULL;
              ERR(GRN_NO_MEMORY_AVAILABLE,
                  "[strinig][utf8] failed to expand checks space");
              return NULL;
            }
            ch = checks + (ch - nstr->checks);
            nstr->checks = checks;
          }
          if (cp) {
            uint_least8_t *ctypes;
            if (!(ctypes = GRN_REALLOC(nstr->ctypes, ds + 1))) {
              GRN_FREE(nstr->ctypes); nstr->ctypes = NULL;
              if (nstr->checks) { GRN_FREE(nstr->checks); nstr->checks = NULL; }
              GRN_FREE(nstr->normalized); nstr->normalized = NULL;
              ERR(GRN_NO_MEMORY_AVAILABLE,
                  "[strinig][utf8] failed to expand character types space");
              return NULL;
            }
            cp = ctypes + (cp - nstr->ctypes);
            nstr->ctypes = ctypes;
          }
        }
        memcpy(d, p, lp);
        d_ = d;
        d += lp;
        length++;
        if (cp) { *cp++ = grn_nfkc_ctype(p); }
        if (ch) {
          size_t i;
          if (s_ == s + ls) {
            *ch++ = -1;
          } else {
            *ch++ = (int16_t)(s + ls - s_);
            s__ = s_;
            s_ = s + ls;
          }
          for (i = lp; i > 1; i--) { *ch++ = 0; }
        }
      }
    }
  }
  if (cp) { *cp = grn_str_null; }
  *d = '\0';
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}
#endif /* WITH_NFKC */

inline static grn_obj *
ascii_normalize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_string *nstr = (grn_string *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][ascii] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][ascii] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STRING_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->normalized);
      nstr->checks = NULL;
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][ascii] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->original + size;
  for (s = s_ = (unsigned char *) nstr->original, d = d_ = d0; s < e; s++) {
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
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

/* use cp1252 as latin1 */
inline static grn_obj *
latin1_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
                 grn_user_data *user_data)
{
  grn_string *nstr = (grn_string *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][latin1] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][latin1] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STRING_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->normalized);
      nstr->checks = NULL;
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][latin1] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->original + size;
  for (s = s_ = (unsigned char *) nstr->original, d = d_ = d0; s < e; s++) {
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
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

inline static grn_obj *
koi8r_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
                grn_user_data *user_data)
{
  grn_string *nstr = (grn_string *)args[0];
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[strinig][koi8r] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(size * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][koi8r] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STRING_WITH_TYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(size + 1))) {
      GRN_FREE(nstr->checks);
      GRN_FREE(nstr->normalized);
      nstr->checks = NULL;
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[strinig][koi8r] failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes = nstr->ctypes;
  e = (unsigned char *)nstr->original + size;
  for (s = s_ = (unsigned char *) nstr->original, d = d_ = d0; s < e; s++) {
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
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
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
        memcpy(destination, source_current, char_length);
        destination += char_length;
        destination_length += char_length;
      }
      source_current += char_length;
    }
    nstr->normalized[destination_length] = '\0';
    nstr->normalized_length_in_bytes = destination_length;
  } else {
    memcpy(nstr->normalized, str, str_len);
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

grn_obj *
grn_string_open_(grn_ctx *ctx, const char *str, unsigned int str_len,
                 grn_obj *normalizer, int flags, grn_encoding encoding)
{
  grn_string *string;
  grn_obj *obj;
  grn_obj *args[1];

  if (!str || !str_len) {
    return NULL;
  }

  string = GRN_MALLOCN(grn_string, 1);
  if (!string) {
    GRN_LOG(ctx, GRN_LOG_ALERT,
            "[string][open] failed to allocate memory");
    return NULL;
  }

  obj = (grn_obj *)string;
  GRN_OBJ_INIT(obj, GRN_STRING, GRN_OBJ_ALLOCATED, GRN_ID_NIL);
  string->original = str;
  string->original_length_in_bytes = str_len;
  string->normalized = NULL;
  string->normalized_length_in_bytes = 0;
  string->n_characters = 0;
  string->checks = NULL;
  string->ctypes = NULL;
  string->encoding = encoding;
  string->flags = flags;

  if (!normalizer) {
    return (grn_obj *)grn_fake_string_open(ctx, string);
  }

  args[0] = obj;
  switch (encoding) {
  case GRN_ENC_EUC_JP :
    eucjp_normalize(ctx, 1, args, NULL);
    break;
  case GRN_ENC_UTF8 :
#ifdef WITH_NFKC
    utf8_normalize(ctx, 1, args, NULL);
#else /* WITH_NFKC */
    ascii_normalize(ctx, 1, args, NULL);
#endif /* WITH_NFKC */
    break;
  case GRN_ENC_SJIS :
    sjis_normalize(ctx, 1, args, NULL);
    break;
  case GRN_ENC_LATIN1 :
    latin1_normalize(ctx, 1, args, NULL);
    break;
  case GRN_ENC_KOI8R :
    koi8r_normalize(ctx, 1, args, NULL);
    break;
  default :
    ascii_normalize(ctx, 1, args, NULL);
    break;
  }
  if (ctx->rc) {
    grn_obj_close(ctx, obj);
    obj = NULL;
  }

  return obj;
}

grn_obj *
grn_string_open(grn_ctx *ctx, const char *str, unsigned int str_len,
                grn_obj *normalizer, int flags)
{
  return grn_string_open_(ctx, str, str_len, normalizer, flags, ctx->encoding);
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

const short *
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
grn_string_set_checks(grn_ctx *ctx, grn_obj *string, short *checks)
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

const unsigned char *
grn_string_get_types(grn_ctx *ctx, grn_obj *string)
{
  unsigned char *types = NULL;
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
grn_string_set_types(grn_ctx *ctx, grn_obj *string, unsigned char *types)
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

grn_rc
grn_string_close(grn_ctx *ctx, grn_obj *string)
{
  grn_rc rc;
  grn_string *string_ = (grn_string *)string;
  if (string_) {
    if (string_->normalized) { GRN_FREE(string_->normalized); }
    if (string_->ctypes) { GRN_FREE(string_->ctypes); }
    if (string_->checks) { GRN_FREE(string_->checks); }
    GRN_FREE(string);
    rc = GRN_SUCCESS;
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }
  return rc;
}
