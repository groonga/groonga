/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012-2018 Brazil
  Copyright(C) 2018-2019 Kouhei Sutou <kou@clear-code.com>

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

#include <string.h>

#include "grn_normalizer.h"
#include "grn_romaji.h"
#include <groonga/normalizer.h>
#include <groonga/tokenizer.h>

grn_rc
grn_normalizer_register(grn_ctx *ctx,
                        const char *name_ptr,
                        int name_length,
                        grn_proc_func *init,
                        grn_proc_func *next,
                        grn_proc_func *fin)
{
  grn_expr_var vars[1];
  memset(vars, 0, sizeof(vars));
  GRN_PTR_INIT(&vars[0].value, 0, GRN_ID_NIL);

  if (name_length < 0) {
    name_length = strlen(name_ptr);
  }

  {
    grn_obj * const normalizer = grn_proc_create(ctx,
                                                 name_ptr, name_length,
                                                 GRN_PROC_NORMALIZER,
                                                 init, next, fin,
                                                 sizeof(*vars) / sizeof(vars),
                                                 vars);
    if (!normalizer) {
      GRN_PLUGIN_ERROR(ctx, GRN_NORMALIZER_ERROR,
                       "[normalizer] failed to register normalizer: <%.*s>",
                       name_length, name_ptr);
      return ctx->rc;
    }
  }
  return GRN_SUCCESS;
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

static unsigned char symbol[] = {
  ',', '.', 0, ':', ';', '?', '!', 0, 0, 0, '`', 0, '^', '~', '_', 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, '-', '-', '/', '\\', 0, 0, '|', 0, 0, 0, '\'', 0,
  '"', '(', ')', 0, 0, '[', ']', '{', '}', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '+', '-', 0, 0, 0, '=', 0, '<', '>', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '$', 0, 0, '%', '#', '&', '*', '@', 0, 0, 0, 0, 0, 0, 0, 0
};

grn_inline static grn_obj *
eucjp_normalize(grn_ctx *ctx, grn_string *nstr)
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
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_, b;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size * 2 + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[string][eucjp] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC((size * 2 + 1) * sizeof(int16_t)))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[string][eucjp] failed to allocate checks space");
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
          "[string][eucjp] failed to allocate character types space");
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
            ctype = GRN_CHAR_KATAKANA;
          } else {
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_OTHERS;
          }
          break;
        case 0x09 :
          *d++ = c1; *d = c2;
          ctype = GRN_CHAR_OTHERS;
          break;
        case 0x0a :
          switch (c1 & 0x0f) {
          case 1 :
            switch (c2) {
            case 0xbc :
              *d++ = c1; *d = c2;
              ctype = GRN_CHAR_KATAKANA;
              break;
            case 0xb9 :
              *d++ = c1; *d = c2;
              ctype = GRN_CHAR_KANJI;
              break;
            case 0xa1 :
              if (removeblankp) {
                if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
                continue;
              } else {
                *d = ' ';
                ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
              }
              break;
            default :
              if (c2 >= 0xa4 && (c3 = symbol[c2 - 0xa4])) {
                *d = c3;
                ctype = GRN_CHAR_SYMBOL;
              } else {
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_OTHERS;
              }
              break;
            }
            break;
          case 2 :
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_SYMBOL;
            break;
          case 3 :
            c3 = c2 - 0x80;
            if ('a' <= c3 && c3 <= 'z') {
              ctype = GRN_CHAR_ALPHA;
              *d = c3;
            } else if ('A' <= c3 && c3 <= 'Z') {
              ctype = GRN_CHAR_ALPHA;
              *d = c3 + 0x20;
            } else if ('0' <= c3 && c3 <= '9') {
              ctype = GRN_CHAR_DIGIT;
              *d = c3;
            } else {
              ctype = GRN_CHAR_OTHERS;
              *d++ = c1; *d = c2;
            }
            break;
          case 4 :
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_HIRAGANA;
            break;
          case 5 :
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_KATAKANA;
            break;
          case 6 :
          case 7 :
          case 8 :
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_SYMBOL;
            break;
          default :
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_OTHERS;
            break;
          }
          break;
        default :
          *d++ = c1; *d = c2;
          ctype = GRN_CHAR_KANJI;
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
            ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
          }
        } else {
          *d = c;
          ctype = GRN_CHAR_SYMBOL;
        }
        break;
      case 3 :
        *d = c;
        ctype = (c <= 0x39) ? GRN_CHAR_DIGIT : GRN_CHAR_SYMBOL;
        break;
      case 4 :
        *d = ('A' <= c) ? c + 0x20 : c;
        ctype = (c == 0x40) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
        break;
      case 5 :
        *d = (c <= 'Z') ? c + 0x20 : c;
        ctype = (c <= 0x5a) ? GRN_CHAR_ALPHA : GRN_CHAR_SYMBOL;
        break;
      case 6 :
        *d = c;
        ctype = (c == 0x60) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
        break;
      case 7 :
        *d = c;
        ctype = (c <= 0x7a) ? GRN_CHAR_ALPHA : (c == 0x7f ? GRN_CHAR_OTHERS : GRN_CHAR_SYMBOL);
        break;
      default :
        *d = c;
        ctype = GRN_CHAR_OTHERS;
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
  if (cp) { *cp = GRN_CHAR_NULL; }
  *d = '\0';
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

grn_inline static grn_obj *
sjis_normalize(grn_ctx *ctx, grn_string *nstr)
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
  int16_t *ch;
  const unsigned char *s, *s_;
  unsigned char *d, *d0, *d_, b, *e;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size * 2 + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[string][sjis] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC((size * 2 + 1) * sizeof(int16_t)))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[string][sjis] failed to allocate checks space");
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
          "[string][sjis] failed to allocate character types space");
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
        ctype = GRN_CHAR_KATAKANA;
      } else {
        if ((s + 1) < e && 0x40 <= *(s + 1) && *(s + 1) <= 0xfc) {
          unsigned char c1 = *s++, c2 = *s, c3 = 0;
          if (0x81 <= c1 && c1 <= 0x87) {
            switch (c1 & 0x0f) {
            case 1 :
              switch (c2) {
              case 0x5b :
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_KATAKANA;
                break;
              case 0x58 :
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_KANJI;
                break;
              case 0x40 :
                if (removeblankp) {
                  if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
                  continue;
                } else {
                  *d = ' ';
                  ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
                }
                break;
              default :
                if (0x43 <= c2 && c2 <= 0x7e && (c3 = symbol[c2 - 0x43])) {
                  *d = c3;
                  ctype = GRN_CHAR_SYMBOL;
                } else if (0x7f <= c2 && c2 <= 0x97 && (c3 = symbol[c2 - 0x44])) {
                  *d = c3;
                  ctype = GRN_CHAR_SYMBOL;
                } else {
                  *d++ = c1; *d = c2;
                  ctype = GRN_CHAR_OTHERS;
                }
                break;
              }
              break;
            case 2 :
              c3 = c2 - 0x1f;
              if (0x4f <= c2 && c2 <= 0x58) {
                ctype = GRN_CHAR_DIGIT;
                *d = c2 - 0x1f;
              } else if (0x60 <= c2 && c2 <= 0x79) {
                ctype = GRN_CHAR_ALPHA;
                *d = c2 + 0x01;
              } else if (0x81 <= c2 && c2 <= 0x9a) {
                ctype = GRN_CHAR_ALPHA;
                *d = c2 - 0x20;
              } else if (0x9f <= c2 && c2 <= 0xf1) {
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_HIRAGANA;
              } else {
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_OTHERS;
              }
              break;
            case 3 :
              if (0x40 <= c2 && c2 <= 0x96) {
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_KATAKANA;
              } else {
                *d++ = c1; *d = c2;
                ctype = GRN_CHAR_SYMBOL;
              }
              break;
            case 4 :
            case 7 :
              *d++ = c1; *d = c2;
              ctype = GRN_CHAR_SYMBOL;
              break;
            default :
              *d++ = c1; *d = c2;
              ctype = GRN_CHAR_OTHERS;
              break;
            }
          } else {
            *d++ = c1; *d = c2;
            ctype = GRN_CHAR_KANJI;
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
            ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
          }
        } else {
          *d = c;
          ctype = GRN_CHAR_SYMBOL;
        }
        break;
      case 3 :
        *d = c;
        ctype = (c <= 0x39) ? GRN_CHAR_DIGIT : GRN_CHAR_SYMBOL;
        break;
      case 4 :
        *d = ('A' <= c) ? c + 0x20 : c;
        ctype = (c == 0x40) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
        break;
      case 5 :
        *d = (c <= 'Z') ? c + 0x20 : c;
        ctype = (c <= 0x5a) ? GRN_CHAR_ALPHA : GRN_CHAR_SYMBOL;
        break;
      case 6 :
        *d = c;
        ctype = (c == 0x60) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
        break;
      case 7 :
        *d = c;
        ctype = (c <= 0x7a) ? GRN_CHAR_ALPHA : (c == 0x7f ? GRN_CHAR_OTHERS : GRN_CHAR_SYMBOL);
        break;
      default :
        *d = c;
        ctype = GRN_CHAR_OTHERS;
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
  if (cp) { *cp = GRN_CHAR_NULL; }
  *d = '\0';
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

#ifdef GRN_WITH_NFKC
typedef struct {
  size_t size;

  unsigned char *dest;
  unsigned char *dest_end;
  unsigned char *d;
  unsigned char *d_; /* -1 */
  size_t n_characters;

  int16_t *checks;
  int16_t *c;

  uint8_t *types;
  uint8_t *t;

  uint64_t *offsets;
  uint64_t *o;
} grn_nfkc_normalize_context;

typedef struct {
  grn_string *string;
  grn_nfkc_normalize_options *options;
  grn_nfkc_normalize_context context;
  grn_bool remove_blank_p;
  grn_bool remove_tokenized_delimiter_p;
} grn_nfkc_normalize_data;

grn_inline static void
grn_nfkc_normalize_context_init(grn_ctx *ctx,
                                grn_nfkc_normalize_context *context,
                                grn_bool need_checks,
                                grn_bool need_types,
                                grn_bool need_offsets,
                                const char *context_tag)
{
  if (!(context->dest = GRN_MALLOC(context->size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalize][nfkc] failed to allocate normalized text space");
    return;
  }
  context->dest_end = context->dest + context->size;
  context->d = context->dest;
  context->d_ = NULL;

  if (need_checks) {
    if (!(context->checks = GRN_MALLOC(sizeof(int16_t) * (context->size + 1)))) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalize][nfkc] failed to allocate checks space");
      return;
    }
    context->checks[0] = 0;
  }
  context->c = context->checks;

  if (need_types) {
    if (!(context->types = GRN_MALLOC(sizeof(uint8_t) * (context->size + 1)))) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalize][nfkc] failed to allocate character types space");
      return;
    }
  }
  context->t = context->types;

  if (need_offsets) {
    if (!(context->offsets = GRN_MALLOC(sizeof(uint64_t) * (context->size + 1)))) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalize][nfkc] failed to allocate offsets space");
      return;
    }
  }
  context->o = context->offsets;
}

grn_inline static void
grn_nfkc_normalize_context_fin(grn_ctx *ctx,
                               grn_nfkc_normalize_context *context)
{
  if (context->dest) {
    GRN_FREE(context->dest);
  }
  if (context->checks) {
    GRN_FREE(context->checks);
  }
  if (context->types) {
    GRN_FREE(context->types);
  }
  if (context->offsets) {
    GRN_FREE(context->offsets);
  }
}

grn_inline static void
grn_nfkc_normalize_context_swap(grn_ctx *ctx,
                                grn_nfkc_normalize_context *context1,
                                grn_nfkc_normalize_context *context2)
{
  grn_nfkc_normalize_context tmp;
  tmp = *context1;
  *context1 = *context2;
  *context2 = tmp;
}

grn_inline static void
grn_nfkc_normalize_context_rewind(grn_ctx *ctx,
                                  grn_nfkc_normalize_context *context)
{
  context->d = context->dest;
  context->d_ = NULL;
  context->n_characters = 0;
  context->c = context->checks;
  context->t = context->types;
  context->o = context->offsets;
}

grn_inline static void
grn_nfkc_normalize_data_init(grn_ctx *ctx,
                             grn_nfkc_normalize_data *data,
                             grn_obj *string,
                             grn_nfkc_normalize_options *options)
{
  size_t size;

  memset(data, 0, sizeof(grn_nfkc_normalize_data));
  data->string = (grn_string *)string;
  data->options = options;
  data->remove_blank_p =
    (data->string->flags & GRN_STRING_REMOVE_BLANK) ||
    data->options->remove_blank;
  data->remove_tokenized_delimiter_p =
    (data->string->flags & GRN_STRING_REMOVE_TOKENIZED_DELIMITER);

  size = data->string->original_length_in_bytes;
  data->context.size = size * 3;

  grn_nfkc_normalize_context_init(ctx,
                                  &(data->context),
                                  data->string->flags & GRN_STRING_WITH_CHECKS,
                                  data->string->flags & GRN_STRING_WITH_TYPES,
                                  data->options->report_source_offset,
                                  "");
}

grn_inline static void
grn_nfkc_normalize_context_expand(grn_ctx *ctx,
                                  grn_nfkc_normalize_context *context,
                                  size_t least_required_size,
                                  const char *context_tag)
{
  unsigned char *dest;
  context->size += (context->size >> 1) + least_required_size;
  dest = GRN_REALLOC(context->dest, context->size + 1);
  if (!dest) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalize][nfkc]%s failed to expand destination text space",
        context_tag);
    return;
  }
  context->dest_end = dest + context->size;
  context->d = dest + (context->d - context->dest);
  context->dest = dest;
  if (context->c) {
    int16_t *checks;
    if (!(checks = GRN_REALLOC(context->checks,
                               sizeof(int16_t) * (context->size + 1)))) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalize][nfkc]%s failed to expand checks space",
          context_tag);
      return;
    }
    context->c = checks + (context->c - context->checks);
    context->checks = checks;
  }
  if (context->t) {
    uint8_t *types;
    if (!(types = GRN_REALLOC(context->types,
                              sizeof(uint8_t) * (context->size + 1)))) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalize][nfkc]%s failed to expand character types space",
        context_tag);
      return;
    }
    context->t = types + (context->t - context->types);
    context->types = types;
  }
  if (context->o) {
    uint64_t *offsets;
    if (!(offsets = GRN_REALLOC(context->offsets,
                                sizeof(uint64_t) * (context->size + 1)))) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalize][nfkc]%s failed to expand offsets space",
          context_tag);
      return;
    }
    context->o = offsets + (context->o - context->offsets);
    context->offsets = offsets;
  }
}

grn_inline static void
grn_nfkc_normalize_expand(grn_ctx *ctx,
                          grn_nfkc_normalize_data *data,
                          size_t least_required_size)
{
  grn_nfkc_normalize_context_expand(ctx,
                                    &(data->context),
                                    least_required_size,
                                    "");
}

grn_inline static const unsigned char *
grn_nfkc_normalize_unify_kana(const unsigned char *utf8_char,
                              unsigned char *unified)
{
  if (utf8_char[0] == 0xe3 &&
      /* U+30A1 KATAKANA LETTER SMALL A ..
       * U+30F6 KATAKANA LETTER SMALL KE
       *
       * U+30FD KATAKANA ITERATION MARK ..
       * U+30F6 KATAKANA LETTER SMALL KE */
      ((utf8_char[1] == 0x82 && 0xa1 <= utf8_char[2]) ||
       (utf8_char[1] == 0x83 && utf8_char[2] <= 0xb6) ||
       (utf8_char[1] == 0x83 && (0xbd <= utf8_char[2] &&
                                 utf8_char[2] <= 0xbe)))) {
    unified[0] = utf8_char[0];
    if (utf8_char[2] & 0x20) {
      unified[1] = utf8_char[1] - 1;
    } else {
      unified[1] = utf8_char[1] - 2;
    }
    unified[2] = utf8_char[2] ^ 0x20;
    return unified;
  }

  return utf8_char;
}

grn_inline static const unsigned char *
grn_nfkc_normalize_unify_hiragana_case(const unsigned char *utf8_char,
                                       unsigned char *unified)
{
  if (utf8_char[0] == 0xe3) {
    if ((utf8_char[1] == 0x81 && (0x81 <= utf8_char[2] &&
                                  utf8_char[2] <= 0x89)) ||
        (utf8_char[1] == 0x81 && utf8_char[2] == 0xa3) ||
        (utf8_char[1] == 0x82 && (0x83 <= utf8_char[2] &&
                                  utf8_char[2] <= 0x87))) {
      /* U+3041 HIRAGANA LETTER SMALL A ..
       * U+3049 HIRAGANA LETTER SMALL O
       *
       * U+3063 HIRAGANA LETTER SMALL TU
       *
       * U+3083 HIRAGANA LETTER SMALL YA ..
       * U+3087 HIRAGANA LETTER SMALL YO */
      if (utf8_char[2] & 0x1) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] + 1;
        return unified;
      }
    } else if (utf8_char[1] == 0x82 && utf8_char[2] == 0x8e) {
      /* U+308E HIRAGANA LETTER SMALL WA */
      unified[0] = utf8_char[0];
      unified[1] = utf8_char[1];
      unified[2] = utf8_char[2] + 1;
      return unified;
    } else if (utf8_char[1] == 0x82 && utf8_char[2] == 0x95) {
      /* U+3095 HIRAGANA LETTER SMALL KA */
      unified[0] = utf8_char[0];
      unified[1] = 0x81;
      unified[2] = 0x8b;
      return unified;
    } else if (utf8_char[1] == 0x82 && utf8_char[2] == 0x96) {
      /* U+3096 HIRAGANA LETTER SMALL KE */
      unified[0] = utf8_char[0];
      unified[1] = 0x81;
      unified[2] = 0x91;
      return unified;
    }
  }

  return utf8_char;
}

grn_inline static const unsigned char *
grn_nfkc_normalize_unify_katakana_case(const unsigned char *utf8_char,
                                       unsigned char *unified)
{
  if (utf8_char[0] == 0xe3) {
    if ((utf8_char[1] == 0x82 && (0xa1 <= utf8_char[2] &&
                                  utf8_char[2] <= 0xa9)) ||
        (utf8_char[1] == 0x83 && utf8_char[2] == 0x83) ||
        (utf8_char[1] == 0x83 && (0xa3 <= utf8_char[2] &&
                                  utf8_char[2] <= 0xa7))) {
      /* U+30A1 KATAKANA LETTER SMALL A ..
       * U+30A9 KATAKANA LETTER SMALL O
       *
       * U+30C3 KATAKANA LETTER SMALL TU
       *
       * U+30E3 KATAKANA LETTER SMALL YA ..
       * U+30E7 KATAKANA LETTER SMALL YO */
      if (utf8_char[2] & 0x1) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] + 1;
        return unified;
      }
    } else if (utf8_char[1] == 0x83 && utf8_char[2] == 0xae) {
      /* U+30EE KATAKANA LETTER SMALL WA */
      unified[0] = utf8_char[0];
      unified[1] = utf8_char[1];
      unified[2] = utf8_char[2] + 1;
      return unified;
    } else if (utf8_char[1] == 0x83 && utf8_char[2] == 0xb5) {
      /* U+3095 HIRAGANA LETTER SMALL KA */
      unified[0] = utf8_char[0];
      unified[1] = 0x82;
      unified[2] = 0xab;
      return unified;
    } else if (utf8_char[1] == 0x83 && utf8_char[2] == 0xb6) {
      /* U+3096 HIRAGANA LETTER SMALL KE */
      unified[0] = utf8_char[0];
      unified[1] = 0x82;
      unified[2] = 0xb1;
      return unified;
    }
  }

  return utf8_char;
}

grn_inline static const unsigned char *
grn_nfkc_normalize_unify_hiragana_voiced_sound_mark(const unsigned char *utf8_char,
                                                    unsigned char *unified)
{
  if (utf8_char[0] == 0xe3) {
    if ((utf8_char[1] == 0x81 && (0x8c <= utf8_char[2] &&
                                  utf8_char[2] <= 0xa2))) {
      /* U+304C HIRAGANA LETTER GA ..
       * U+3062 HIRAGANA LETTER DI */
      if (!(utf8_char[2] & 0x1)) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] - 1;
        return unified;
      }
    } else if ((utf8_char[1] == 0x81 && (0xa5 <= utf8_char[2] &&
                                         utf8_char[2] <= 0xa9))) {
      /* U+3065 HIRAGANA LETTER DU ..
       * U+3069 HIRAGANA LETTER DO */
      if (utf8_char[2] & 0x1) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] - 1;
        return unified;
      }
    } else if ((utf8_char[1] == 0x81 && (0xb0 <= utf8_char[2] &&
                                         utf8_char[2] <= 0xbd))) {
      /* U+3070 HIRAGANA LETTER BA ..
       * U+307D HIRAGANA LETTER PO */
      unsigned char mod3 = (utf8_char[2] - 1) % 3;
      if (mod3 != 0) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] - mod3;
        return unified;
      }
    }
  }

  return utf8_char;
}

grn_inline static const unsigned char *
grn_nfkc_normalize_unify_katakana_voiced_sound_mark(const unsigned char *utf8_char,
                                                    unsigned char *unified)
{
  if (utf8_char[0] == 0xe3) {
    if (utf8_char[1] == 0x83 && utf8_char[2] == 0x80) {
      /* U+30C0 KATAKANA LETTER DA */
      unified[0] = utf8_char[0];
      unified[1] = 0x82;
      unified[2] = 0xbf;
      return unified;
    } else if ((utf8_char[1] == 0x82 && 0xac <= utf8_char[2]) ||
               (utf8_char[1] == 0x83 && utf8_char[2] <= 0x82)) {
      /* U+30AC KATAKANA LETTER GA ..
       * U+30C2 KATAKANA LETTER DI */
      if (!(utf8_char[2] & 0x1)) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] - 1;
        return unified;
      }
    } else if ((utf8_char[1] == 0x83 && (0x85 <= utf8_char[2] &&
                                         utf8_char[2] <= 0x89))) {
      /* U+30C5 KATAKANA LETTER DU ..
       * U+30C9 KATAKANA LETTER DO */
      if (utf8_char[2] & 0x1) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] - 1;
        return unified;
      }
    } else if ((utf8_char[1] == 0x83 && (0x90 <= utf8_char[2] &&
                                         utf8_char[2] <= 0x9d))) {
      /* U+30D0 KATAKANA LETTER BA ..
       * U+30DD KATAKANA LETTER PO */
      unsigned char mod3 = (utf8_char[2] - 2) % 3;
      if (mod3 != 0) {
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1];
        unified[2] = utf8_char[2] - mod3;
        return unified;
      }
    }
  }

  return utf8_char;
}

grn_inline static grn_bool
grn_nfkc_normalize_is_hyphen_famity(const unsigned char *utf8_char,
                                    size_t length)
{
  if (length == 1) {
    if (utf8_char[0] == '-') {
      /* U+002D HYPHEN-MINUS */
      return GRN_TRUE;
    }
  } else if (length == 2) {
    switch (utf8_char[0]) {
    case 0xcb :
      if (utf8_char[1] == 0x97) {
        /* U+02D7 MODIFIER LETTER MINUS SIGN */
        return GRN_TRUE;
      }
      break;
    case 0xd6 :
      if (utf8_char[1] == 0x8a) {
        /* U+058A ARMENIAN HYPHEN */
        return GRN_TRUE;
      }
      break;
    default :
      break;
    }
  } else if (length == 3) {
    if (utf8_char[0] == 0xe2) {
      if (utf8_char[1] == 0x80 &&
          (0x90 <= utf8_char[2] && utf8_char[2] <= 0x93)) {
        /* U+2010 HYPHEN ..
         * U+2013 EN DASH */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0x81 &&
                 (utf8_char[2] == 0x83 ||
                  utf8_char[2] == 0xbb)) {
        /* U+2043 HYPHEN BULLET */
        /* U+207B SUPERSCRIPT MINUS */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0x82 && utf8_char[2] == 0x8b) {
        /* U+208B SUBSCRIPT MINUS */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0x88 && utf8_char[2] == 0x92) {
        /* U+2212 MINUS SIGN */
        return GRN_TRUE;
      }
    }
  }

  return GRN_FALSE;
}

grn_inline static grn_bool
grn_nfkc_normalize_is_prolonged_sound_mark_famity(const unsigned char *utf8_char,
                                                  size_t length)
{
  if (length == 3) {
    if (utf8_char[0] == 0xe2) {
      if (utf8_char[1] == 0x80 &&
          (0x94 <= utf8_char[2] && utf8_char[2] <= 0x95)) {
        /* U+2014 EM DASH ..
         * U+2015 HORIZONTAL BAR */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0x94 &&
          (0x80 <= utf8_char[2] && utf8_char[2] <= 0x81)) {
        /* U+2500 BOX DRAWINGS LIGHT HORIZONTAL ..
         * U+2501 BOX DRAWINGS HEAVY HORIZONTAL */
        return GRN_TRUE;
      }
    } else if (utf8_char[0] == 0xe3) {
      if (utf8_char[1] == 0x83 && utf8_char[2] == 0xbc) {
        /* U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK */
        return GRN_TRUE;
      }
    } else if (utf8_char[0] == 0xef) {
      if (utf8_char[1] == 0xbd && utf8_char[2] == 0xb0) {
        /* U+FF70 HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK */
        return GRN_TRUE;
      }
    }
  }

  return GRN_FALSE;
}

grn_inline static grn_bool
grn_nfkc_normalize_is_middle_dot_family(const unsigned char *utf8_char,
                                        size_t length)
{
  if (length == 3) {
    if (utf8_char[0] == 0xe1) {
      if (utf8_char[1] == 0x90 && utf8_char[2] == 0xa7) {
        /* U+1427 CANADIAN SYLLABICS FINAL MIDDLE DOT */
        return GRN_TRUE;
      }
    } else if (utf8_char[0] == 0xe2) {
      if (utf8_char[1] == 0x80 && utf8_char[2] == 0xa2) {
        /* U+2022 BULLET */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0x88 && utf8_char[2] == 0x99) {
        /* U+2219 BULLET OPERATOR */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0x8b && utf8_char[2] == 0x85) {
        /* U+22C5 DOT OPERATOR */
        return GRN_TRUE;
      } else if (utf8_char[1] == 0xb8 && utf8_char[2] == 0xb1) {
        /* U+2E31 WORD SEPARATOR MIDDLE DOT */
        return GRN_TRUE;
      }
    } else if (utf8_char[0] == 0xe3) {
      if (utf8_char[1] == 0x83 && utf8_char[2] == 0xbb) {
        /* U+30FB KATAKANA MIDDLE DOT */
        return GRN_TRUE;
      }
    } else if (utf8_char[0] == 0xef) {
      if (utf8_char[1] == 0xbd && utf8_char[2] == 0xa5) {
        /* U+FF65 HALFWIDTH KATAKANA MIDDLE DOT */
        return GRN_TRUE;
      }
    }
  }

  return GRN_FALSE;
}

grn_inline static const unsigned char *
grn_nfkc_normalize_unify_to_katakana(const unsigned char *utf8_char,
                                     unsigned char *unified,
                                     size_t length)
{
  if (utf8_char[0] == 0xe3) {
    if (utf8_char[1] == 0x81 &&
        utf8_char[2] >= 0x81 && utf8_char[2] <= 0x9f) {
      /* U+3041 HIRAGANA LETTER SMALL A ..
       * U+305F HIRAGANA LETTER TA */
      unified[0] = utf8_char[0];
      unified[1] = utf8_char[1] + 0x01;
      unified[2] = utf8_char[2] + 0x20;
      return unified;
    } else if (utf8_char[1] == 0x81 &&
               utf8_char[2] >= 0xa0 && utf8_char[2] <= 0xbf) {
      /* U+3060 HIRAGANA LETTER DA ..
       * U+305F HIRAGANA LETTER MI */
      unified[0] = utf8_char[0];
      unified[1] = utf8_char[1] + 0x02;
      unified[2] = utf8_char[2] - 0x20;
      return unified;
    } else if (utf8_char[1] == 0x82) {
      if ((utf8_char[2] >= 0x80 && utf8_char[2] <= 0x96)
          || (utf8_char[2] >= 0x9d && utf8_char[2] <= 0x9e)) {
        /* U+3041 HIRAGANA LETTER MU ..
         * U+3096 HIRAGANA LETTER KE
         * U+309D HIRAGANA ITERATION MARK
         * U+309E HIRAGANA VOICED ITERATION MARK */
        unified[0] = utf8_char[0];
        unified[1] = utf8_char[1] + 0x01;
        unified[2] = utf8_char[2] + 0x20;
        return unified;
      }
    }
  }
  return utf8_char;
}

static void
grn_nfkc_normalize_unify_stateless(grn_ctx *ctx,
                                   grn_nfkc_normalize_data *data,
                                   grn_nfkc_normalize_context *unify)
{
  const unsigned char *current = data->context.dest;
  const unsigned char *end = data->context.d;
  size_t i_byte;
  size_t i_character;

  i_byte = 0;
  i_character = 0;
  while (current < end) {
    unsigned char unified_kana[3];
    unsigned char unified_kana_case[3];
    unsigned char unified_kana_voiced_sound_mark[3];
    unsigned char unified_hyphen[] = {'-'};
    /* U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    unsigned char unified_prolonged_sound_mark[] = {0xe3, 0x83, 0xbc};
    /* U+00B7 MIDDLE DOT */
    unsigned char unified_middle_dot[] = {0xc2, 0xb7};
    unsigned char unified_katakana[3];
    const unsigned char *unifying = current;
    size_t char_length;
    size_t unified_char_length;
    grn_char_type char_type;

    char_length = grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
    unified_char_length = char_length;

    if (data->context.t) {
      char_type = data->context.types[i_character];
    } else {
      char_type = data->options->char_type_func(current);
    }

    if (data->options->unify_kana &&
        GRN_CHAR_TYPE(char_type) == GRN_CHAR_KATAKANA &&
        unified_char_length == 3) {
      unifying = grn_nfkc_normalize_unify_kana(unifying, unified_kana);
      if (unifying == unified_kana) {
        char_type = GRN_CHAR_HIRAGANA | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (data->options->unify_to_katakana &&
        GRN_CHAR_TYPE(char_type) == GRN_CHAR_HIRAGANA &&
        unified_char_length == 3) {
      unifying = grn_nfkc_normalize_unify_to_katakana(unifying,
                                                      unified_katakana,
                                                      unified_char_length);
      if (unifying == unified_katakana) {
        char_type = GRN_CHAR_KATAKANA | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (data->options->unify_kana_case) {
      switch (GRN_CHAR_TYPE(char_type)) {
      case GRN_CHAR_HIRAGANA :
        if (unified_char_length == 3) {
          unifying = grn_nfkc_normalize_unify_hiragana_case(unifying,
                                                            unified_kana_case);
        }
        break;
      case GRN_CHAR_KATAKANA :
        if (unified_char_length == 3) {
          unifying = grn_nfkc_normalize_unify_katakana_case(unifying,
                                                            unified_kana_case);
        }
        break;
      default :
        break;
      }
    }

    if (data->options->unify_kana_voiced_sound_mark) {
      switch (GRN_CHAR_TYPE(char_type)) {
      case GRN_CHAR_HIRAGANA :
        if (unified_char_length == 3) {
          unifying = grn_nfkc_normalize_unify_hiragana_voiced_sound_mark(
            unifying, unified_kana_voiced_sound_mark);
        }
        break;
      case GRN_CHAR_KATAKANA :
        if (unified_char_length == 3) {
          unifying = grn_nfkc_normalize_unify_katakana_voiced_sound_mark(
            unifying, unified_kana_voiced_sound_mark);
        }
        break;
      default :
        break;
      }
    }

    if (data->options->unify_hyphen) {
      if (grn_nfkc_normalize_is_hyphen_famity(unifying, unified_char_length)) {
        unifying = unified_hyphen;
        unified_char_length = sizeof(unified_hyphen);
        char_type = GRN_CHAR_SYMBOL | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (data->options->unify_prolonged_sound_mark) {
      if (grn_nfkc_normalize_is_prolonged_sound_mark_famity(unifying,
                                                            unified_char_length)) {
        unifying = unified_prolonged_sound_mark;
        unified_char_length = sizeof(unified_prolonged_sound_mark);
        char_type = GRN_CHAR_KATAKANA | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (data->options->unify_hyphen_and_prolonged_sound_mark) {
      if (grn_nfkc_normalize_is_hyphen_famity(unifying, unified_char_length) ||
          grn_nfkc_normalize_is_prolonged_sound_mark_famity(unifying,
                                                            unified_char_length)) {
        unifying = unified_hyphen;
        unified_char_length = sizeof(unified_hyphen);
        char_type = GRN_CHAR_SYMBOL | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (data->options->unify_middle_dot) {
      if (grn_nfkc_normalize_is_middle_dot_family(unifying,
                                                  unified_char_length)) {
        unifying = unified_middle_dot;
        unified_char_length = sizeof(unified_middle_dot);
        char_type = GRN_CHAR_SYMBOL | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (unify->d + unified_char_length >= unify->dest_end) {
      grn_nfkc_normalize_context_expand(ctx,
                                        unify,
                                        unified_char_length,
                                        "[unify]");
      if (ctx->rc != GRN_SUCCESS) {
        return;
      }
    }

    grn_memcpy(unify->d, unifying, unified_char_length);
    unify->d += unified_char_length;
    unify->n_characters++;
    if (unify->t) {
      *(unify->t++) = char_type;
    }
    if (unify->c) {
      size_t i;
      *(unify->c++) += data->context.checks[i_byte];
      for (i = 1; i < unified_char_length; i++) {
        *(unify->c++) = 0;
      }
      unify->c[0] = 0;
    }
    if (unify->o) {
      *(unify->o++) = data->context.offsets[i_character];
    }

    i_byte += char_length;
    current += char_length;
    i_character++;
  }
}

typedef const unsigned char *
(*grn_nfkc_normalize_unify_stateful_func)(grn_ctx *ctx,
                                          const unsigned char *current,
                                          const unsigned char *end,
                                          size_t *n_used_bytes,
                                          size_t *n_used_characters,
                                          unsigned char *unified_buffer,
                                          size_t *n_unified_bytes,
                                          size_t *n_unified_characters);

static const unsigned char *
grn_nfkc_normalize_unify_katakana_v_sounds(grn_ctx *ctx,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters)
{
  size_t char_length;

  char_length = grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  /* U+30F4 KATAKANA LETTER VU */
  if (char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0xb4) {
    const unsigned char *next = current + char_length;
    size_t next_char_length;

    next_char_length = grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3 &&
        next[0] == 0xe3 &&
        next[1] == 0x82) {
      if (next[2] == 0xa1) { /* U+30A1 KATAKANA LETTER SMALL A */
        /* U+30D0 KATAKANA LETTER BA */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0x90;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa3) { /* U+30A3 KATAKANA LETTER SMALL I */
        /* U+30D3 KATAKANA LETTER BI */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0x93;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa5) { /* U+30A5 KATAKANA LETTER SMALL U */
        /* U+30D6 KATAKANA LETTER BU */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0x96;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa7) { /* U+30A7 KATAKANA LETTER SMALL E */
        /* U+30D9 KATAKANA LETTER BE */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0x99;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa9) { /* U+30A8 KATAKANA LETTER SMALL O */
        /* U+30DC KATAKANA LETTER BO */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0x9c;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      }
    }
    /* U+30D6 KATAKANA LETTER BU */
    unified_buffer[(*n_unified_bytes)++] = current[0];
    unified_buffer[(*n_unified_bytes)++] = current[1];
    unified_buffer[(*n_unified_bytes)++] = 0x96;
    (*n_unified_characters)++;
    return unified_buffer;
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_bu_sound(grn_ctx *ctx,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters)
{
  size_t char_length;

  char_length = grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
  *n_used_bytes = char_length;
  *n_used_characters = 1;

  /* U+30F4 KATAKANA LETTER VU */
  if (char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0xb4) {
    const unsigned char *next = current + char_length;
    size_t next_char_length;

    /* U+30D6 KATAKANA LETTER BU */
    unified_buffer[(*n_unified_bytes)++] = current[0];
    unified_buffer[(*n_unified_bytes)++] = current[1];
    unified_buffer[(*n_unified_bytes)++] = 0x96;
    (*n_unified_characters)++;

    next_char_length = grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3 &&
        next[0] == 0xe3 &&
        next[1] == 0x82 &&
        /* U+30A1 KATAKANA LETTER SMALL A */
        /* U+30A3 KATAKANA LETTER SMALL I */
        /* U+30A5 KATAKANA LETTER SMALL U */
        /* U+30A7 KATAKANA LETTER SMALL E */
        /* U+30A9 KATAKANA LETTER SMALL O */
        (next[2] == 0xa1 ||
         next[2] == 0xa3 ||
         next[2] == 0xa5 ||
         next[2] == 0xa7 ||
         next[2] == 0xa9)) {
      (*n_used_bytes) += next_char_length;
      (*n_used_characters)++;
    }
    return unified_buffer;
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

grn_inline static void
grn_nfkc_normalize_unify_stateful(grn_ctx *ctx,
                                  grn_nfkc_normalize_data *data,
                                  grn_nfkc_normalize_context *unify,
                                  grn_nfkc_normalize_unify_stateful_func func,
                                  const char *context_tag)
{
  const unsigned char *current = data->context.dest;
  const unsigned char *end = data->context.d;
  size_t i_byte = 0;
  size_t i_character = 0;

  while (current < end) {
    size_t n_used_bytes = 0;
    size_t n_used_characters = 0;
    const unsigned char *unified;
    unsigned char unified_buffer[64]; /* Will be enough. */
    size_t n_unified_bytes = 0;
    size_t n_unified_characters = 0;

    unified = func(ctx,
                   current, end, &n_used_bytes, &n_used_characters,
                   unified_buffer, &n_unified_bytes, &n_unified_characters);

    if (unify->d + n_unified_bytes >= unify->dest_end) {
      grn_nfkc_normalize_context_expand(ctx,
                                        unify,
                                        n_unified_bytes,
                                        context_tag);
      if (ctx->rc != GRN_SUCCESS) {
        return;
      }
    }

    if (unified == current && n_unified_bytes == n_used_bytes) {
      grn_memcpy(unify->d, unified, n_unified_bytes);
      unify->d += n_unified_bytes;
      unify->n_characters++;
      if (unify->t) {
        *(unify->t++) = data->context.types[i_character];
      }
      if (unify->c) {
        size_t i;
        *(unify->c++) = data->context.checks[i_byte];
        for (i = 1; i < n_unified_bytes; i++) {
          *(unify->c++) = 0;
        }
        unify->c[0] = 0;
      }
      if (unify->o) {
        *(unify->o++) = data->context.offsets[i_character];
      }
    } else {
      if (n_unified_bytes > 0) {
        grn_memcpy(unify->d, unified, n_unified_bytes);

        unify->d += n_unified_bytes;
        unify->n_characters += n_unified_characters;
        if (unify->t || unify->c || unify->o) {
          const unsigned char *unified_current = unified;
          const unsigned char *unified_end = unified + n_unified_bytes;

          while (unified_current < unified_end) {
            size_t unified_char_length;

            unified_char_length = grn_charlen_(ctx,
                                               unified_current,
                                               unified_end,
                                               GRN_ENC_UTF8);
            if (unify->t) {
              *(unify->t++) = data->options->char_type_func(unified_current);
            }
            if (unify->c) {
              size_t i;
              if (unified_current == unified) {
                unify->c[0] = data->context.checks[i_byte];
                for (i = 1; i < n_used_bytes; i++) {
                  if (data->context.checks[i_byte + i] > 0) {
                    unify->c[0] += data->context.checks[i_byte + i];
                  }
                }
                unify->c++;
              } else {
                *(unify->c++) = -1;
              }
              for (i = 1; i < unified_char_length; i++) {
                *(unify->c++) = 0;
              }
              unify->c[0] = 0;
            }
            if (unify->o) {
              *(unify->o++) = data->context.offsets[i_character];
            }
            unified_current += unified_char_length;
          }
        }
      }
    }

    i_byte += n_used_bytes;
    current += n_used_bytes;
    i_character += n_used_characters;
  }
}

static void
grn_nfkc_normalize_unify(grn_ctx *ctx,
                         grn_nfkc_normalize_data *data)
{
  grn_nfkc_normalize_context unify;
  grn_bool need_swap = GRN_FALSE;

  if (!(data->options->unify_kana ||
        data->options->unify_kana_case ||
        data->options->unify_kana_voiced_sound_mark ||
        data->options->unify_hyphen ||
        data->options->unify_prolonged_sound_mark ||
        data->options->unify_hyphen_and_prolonged_sound_mark ||
        data->options->unify_middle_dot ||
        data->options->unify_katakana_v_sounds ||
        data->options->unify_katakana_bu_sound ||
        data->options->unify_to_romaji ||
        data->options->unify_to_katakana)) {
    return;
  }

  memset(&unify, 0, sizeof(grn_nfkc_normalize_context));
  unify.size = data->context.size;
  grn_nfkc_normalize_context_init(ctx,
                                  &unify,
                                  data->context.checks != NULL,
                                  data->context.types != NULL,
                                  data->context.offsets != NULL,
                                  "[unify]");
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  if (data->options->unify_kana ||
      data->options->unify_kana_case ||
      data->options->unify_kana_voiced_sound_mark ||
      data->options->unify_hyphen ||
      data->options->unify_prolonged_sound_mark ||
      data->options->unify_hyphen_and_prolonged_sound_mark ||
      data->options->unify_middle_dot ||
      data->options->unify_to_katakana) {
    grn_nfkc_normalize_unify_stateless(ctx, data, &unify);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_v_sounds) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_v_sounds,
                                      "[unify][katakana-v-sounds]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_bu_sound) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_bu_sound,
                                      "[unify][katakana-bu-sound]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_to_romaji) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_romaji_hepburn_convert,
                                      "[unify][romaji]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  grn_nfkc_normalize_context_fin(ctx, &(data->context));

  data->context.size = unify.size;

  data->context.dest = unify.dest;
  data->context.d = unify.d;
  data->context.d_ = unify.d_;
  data->context.n_characters = unify.n_characters;
  unify.dest = NULL;

  data->context.checks = unify.checks;
  data->context.c = unify.c;
  unify.checks = NULL;

  data->context.types = unify.types;
  data->context.t = unify.t;
  unify.types = NULL;

  data->context.offsets = unify.offsets;
  data->context.o = unify.o;
  unify.offsets = NULL;

exit:
  grn_nfkc_normalize_context_fin(ctx, &unify);
}

grn_rc
grn_nfkc_normalize(grn_ctx *ctx,
                   grn_obj *string,
                   grn_nfkc_normalize_options *options)
{
  grn_nfkc_normalize_data data;
  const unsigned char *source;
  const unsigned char *source_ = NULL; /* -1 */
  const unsigned char *source__ = NULL; /* -2 */
  const unsigned char *source_end;
  size_t source_char_length;
  grn_nfkc_normalize_context *context;

  grn_nfkc_normalize_data_init(ctx, &data, string, options);
  context = &(data.context);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  source = source_ = (unsigned char *)(data.string->original);
  source_end = source + data.string->original_length_in_bytes;
  for (; source < source_end; source += source_char_length) {
    source_char_length = grn_charlen_(ctx, source, source_end, GRN_ENC_UTF8);
    if (source_char_length == 0) {
      break;
    }
    if (data.remove_tokenized_delimiter_p &&
        grn_tokenizer_is_tokenized_delimiter(ctx,
                                             (const char *)source,
                                             source_char_length,
                                             GRN_ENC_UTF8)) {
      continue;
    }
    {
      const char *decomposed;
      const unsigned char *current;
      const unsigned char *current_end;
      size_t current_length;

      decomposed = data.options->decompose_func(source);
      if (decomposed) {
        current = decomposed;
        current_end = current + strlen(decomposed);
      } else {
        current = source;
        current_end = current + source_char_length;
      }
      if (context->d_) {
        const char *composed = NULL;
        composed = options->compose_func(context->d_, current);
        if (composed) {
          current = composed;
          current_end = current + strlen(composed);
          if (context->t) { context->t--; }
          if (context->c) {
            context->c -= (context->d - context->d_);
            if (context->c[0] >= 0) {
              source_ = source__;
            }
          }
          if (context->o) {
            context->o--;
          }
          context->d = context->d_;
          context->n_characters--;
        }
      }
      for (; current < current_end; current += current_length) {
        current_length = grn_charlen_(ctx, current, current_end, GRN_ENC_UTF8);
        if (current_length == 0) {
          break;
        }
        if ((current[0] == ' ' && data.remove_blank_p) ||
            current[0] < 0x20 /* skip unprintable ascii */) {
          if (context->t > context->types) {
            context->t[-1] |= GRN_CHAR_BLANK;
          }
          if (!data.options->include_removed_source_location) {
            source_ += current_length;
          }
        } else {
          if (context->dest_end <= context->d + current_length) {
            grn_nfkc_normalize_expand(ctx, &data, current_length);
            if (ctx->rc != GRN_SUCCESS) {
              goto exit;
            }
          }

          grn_memcpy(context->d, current, current_length);
          context->d_ = context->d;
          context->d += current_length;
          context->n_characters++;
          if (context->t) {
            grn_char_type char_type;
            char_type = data.options->char_type_func(current);
            *(context->t++) = char_type;
          }
          if (context->c) {
            size_t i;
            if (source_ == source + source_char_length) {
              *(context->c++) = -1;
            } else {
              *(context->c++) = (int16_t)(source + source_char_length - source_);
              source__ = source_;
              source_ = source + source_char_length;
            }
            for (i = current_length; i > 1; i--) { *(context->c++) = 0; }
          }
          if (context->o) {
            *(context->o++) =
              (uint64_t)(source - (const unsigned char *)(data.string->original));
          }
        }
      }
    }
  }
  grn_nfkc_normalize_unify(ctx, &data);
  if (context->t) { *(context->t) = GRN_CHAR_NULL; }
  if (context->o) { *(context->o) = data.string->original_length_in_bytes; }
  *(context->d) = '\0';
  data.string->n_characters = context->n_characters;
  data.string->normalized = context->dest;
  data.string->normalized_length_in_bytes = (size_t)(context->d - context->dest);
  data.string->checks = context->checks;
  data.string->ctypes = context->types;
  data.string->offsets = context->offsets;
  context->dest = NULL;
  context->checks = NULL;
  context->types = NULL;
  context->offsets = NULL;
exit:
  if (ctx->rc != GRN_SUCCESS) {
    grn_nfkc_normalize_context_fin(ctx, context);
  }
  return ctx->rc;
}
#endif /* GRN_WITH_NFKC */

grn_inline static grn_obj *
ascii_normalize(grn_ctx *ctx, grn_string *nstr)
{
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[string][ascii] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC((size + 1) * sizeof(int16_t)))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[string][ascii] failed to allocate checks space");
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
          "[string][ascii] failed to allocate character types space");
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
          ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
        }
      } else {
        *d = c;
        ctype = GRN_CHAR_SYMBOL;
      }
      break;
    case 3 :
      *d = c;
      ctype = (c <= 0x39) ? GRN_CHAR_DIGIT : GRN_CHAR_SYMBOL;
      break;
    case 4 :
      *d = ('A' <= c) ? c + 0x20 : c;
      ctype = (c == 0x40) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 5 :
      *d = (c <= 'Z') ? c + 0x20 : c;
      ctype = (c <= 0x5a) ? GRN_CHAR_ALPHA : GRN_CHAR_SYMBOL;
      break;
    case 6 :
      *d = c;
      ctype = (c == 0x60) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 7 :
      *d = c;
      ctype = (c <= 0x7a) ? GRN_CHAR_ALPHA : (c == 0x7f ? GRN_CHAR_OTHERS : GRN_CHAR_SYMBOL);
      break;
    default :
      *d = c;
      ctype = GRN_CHAR_OTHERS;
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
  if (cp) { *cp = GRN_CHAR_NULL; }
  *d = '\0';
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

/* use cp1252 as latin1 */
grn_inline static grn_obj *
latin1_normalize(grn_ctx *ctx, grn_string *nstr)
{
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[string][latin1] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC((size + 1) * sizeof(int16_t)))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[string][latin1] failed to allocate checks space");
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
          ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
        }
      } else {
        *d = c;
        ctype = GRN_CHAR_SYMBOL;
      }
      break;
    case 3 :
      *d = c;
      ctype = (c <= 0x39) ? GRN_CHAR_DIGIT : GRN_CHAR_SYMBOL;
      break;
    case 4 :
      *d = ('A' <= c) ? c + 0x20 : c;
      ctype = (c == 0x40) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 5 :
      *d = (c <= 'Z') ? c + 0x20 : c;
      ctype = (c <= 0x5a) ? GRN_CHAR_ALPHA : GRN_CHAR_SYMBOL;
      break;
    case 6 :
      *d = c;
      ctype = (c == 0x60) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 7 :
      *d = c;
      ctype = (c <= 0x7a) ? GRN_CHAR_ALPHA : (c == 0x7f ? GRN_CHAR_OTHERS : GRN_CHAR_SYMBOL);
      break;
    case 8 :
      if (c == 0x8a || c == 0x8c || c == 0x8e) {
        *d = c + 0x10;
        ctype = GRN_CHAR_ALPHA;
      } else {
        *d = c;
        ctype = GRN_CHAR_SYMBOL;
      }
      break;
    case 9 :
      if (c == 0x9a || c == 0x9c || c == 0x9e || c == 0x9f) {
        *d = (c == 0x9f) ? c + 0x60 : c;
        ctype = GRN_CHAR_ALPHA;
      } else {
        *d = c;
        ctype = GRN_CHAR_SYMBOL;
      }
      break;
    case 0x0c :
      *d = c + 0x20;
      ctype = GRN_CHAR_ALPHA;
      break;
    case 0x0d :
      *d = (c == 0xd7 || c == 0xdf) ? c : c + 0x20;
      ctype = (c == 0xd7) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 0x0e :
      *d = c;
      ctype = GRN_CHAR_ALPHA;
      break;
    case 0x0f :
      *d = c;
      ctype = (c == 0xf7) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    default :
      *d = c;
      ctype = GRN_CHAR_OTHERS;
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
  if (cp) { *cp = GRN_CHAR_NULL; }
  *d = '\0';
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

grn_inline static grn_obj *
koi8r_normalize(grn_ctx *ctx, grn_string *nstr)
{
  int16_t *ch;
  const unsigned char *s, *s_, *e;
  unsigned char *d, *d0, *d_;
  uint_least8_t *cp, *ctypes, ctype;
  size_t size = nstr->original_length_in_bytes, length = 0;
  int removeblankp = nstr->flags & GRN_STRING_REMOVE_BLANK;
  if (!(nstr->normalized = GRN_MALLOC(size + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[string][koi8r] failed to allocate normalized text space");
    return NULL;
  }
  d0 = (unsigned char *) nstr->normalized;
  if (nstr->flags & GRN_STRING_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC((size + 1) * sizeof(int16_t)))) {
      GRN_FREE(nstr->normalized);
      nstr->normalized = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[string][koi8r] failed to allocate checks space");
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
          "[string][koi8r] failed to allocate character types space");
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
          ctype = GRN_CHAR_BLANK|GRN_CHAR_SYMBOL;
        }
      } else {
        *d = c;
        ctype = GRN_CHAR_SYMBOL;
      }
      break;
    case 3 :
      *d = c;
      ctype = (c <= 0x39) ? GRN_CHAR_DIGIT : GRN_CHAR_SYMBOL;
      break;
    case 4 :
      *d = ('A' <= c) ? c + 0x20 : c;
      ctype = (c == 0x40) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 5 :
      *d = (c <= 'Z') ? c + 0x20 : c;
      ctype = (c <= 0x5a) ? GRN_CHAR_ALPHA : GRN_CHAR_SYMBOL;
      break;
    case 6 :
      *d = c;
      ctype = (c == 0x60) ? GRN_CHAR_SYMBOL : GRN_CHAR_ALPHA;
      break;
    case 7 :
      *d = c;
      ctype = (c <= 0x7a) ? GRN_CHAR_ALPHA : (c == 0x7f ? GRN_CHAR_OTHERS : GRN_CHAR_SYMBOL);
      break;
    case 0x0a :
      *d = c;
      ctype = (c == 0xa3) ? GRN_CHAR_ALPHA : GRN_CHAR_OTHERS;
      break;
    case 0x0b :
      if (c == 0xb3) {
        *d = c - 0x10;
        ctype = GRN_CHAR_ALPHA;
      } else {
        *d = c;
        ctype = GRN_CHAR_OTHERS;
      }
      break;
    case 0x0c :
    case 0x0d :
      *d = c;
      ctype = GRN_CHAR_ALPHA;
      break;
    case 0x0e :
    case 0x0f :
      *d = c - 0x20;
      ctype = GRN_CHAR_ALPHA;
      break;
    default :
      *d = c;
      ctype = GRN_CHAR_OTHERS;
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
  if (cp) { *cp = GRN_CHAR_NULL; }
  *d = '\0';
  nstr->n_characters = length;
  nstr->normalized_length_in_bytes = (size_t)(d - (unsigned char *)nstr->normalized);
  return NULL;
}

static grn_obj *
auto_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_string *string_ = (grn_string *)(string);
  switch (string_->encoding) {
  case GRN_ENC_EUC_JP :
    eucjp_normalize(ctx, string_);
    break;
  case GRN_ENC_UTF8 :
#ifdef GRN_WITH_NFKC
    {
      grn_nfkc_normalize_options options;
      grn_nfkc_normalize_options_init(ctx,
                                      &options,
                                      grn_nfkc_char_type,
                                      grn_nfkc_decompose,
                                      grn_nfkc_compose);
      grn_nfkc_normalize(ctx, string, &options);
      grn_nfkc_normalize_options_fin(ctx, &options);
    }
#else /* GRN_WITH_NFKC */
    ascii_normalize(ctx, string_);
#endif /* GRN_WITH_NFKC */
    break;
  case GRN_ENC_SJIS :
    sjis_normalize(ctx, string_);
    break;
  case GRN_ENC_LATIN1 :
    latin1_normalize(ctx, string_);
    break;
  case GRN_ENC_KOI8R :
    koi8r_normalize(ctx, string_);
    break;
  default :
    ascii_normalize(ctx, string_);
    break;
  }
  return NULL;
}

#ifdef GRN_WITH_NFKC
static grn_obj *
nfkc51_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_nfkc_normalize_options options;

  grn_nfkc_normalize_options_init(ctx,
                                  &options,
                                  grn_nfkc50_char_type,
                                  grn_nfkc50_decompose,
                                  grn_nfkc50_compose);
  grn_nfkc_normalize(ctx, string, &options);
  grn_nfkc_normalize_options_fin(ctx, &options);
  return NULL;
}

static void *
nfkc100_open_options(grn_ctx *ctx,
                     grn_obj *normalizer,
                     grn_obj *raw_options,
                     void *user_data)
{
  grn_nfkc_normalize_options *options;

  options = GRN_MALLOC(sizeof(grn_nfkc_normalize_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][nfkc100] "
        "failed to allocate memory for options");
    return NULL;
  }

  grn_nfkc100_normalize_options_init(ctx, options);

  grn_nfkc_normalize_options_apply(ctx, options, raw_options);

  return options;
}

static void
nfkc100_close_options(grn_ctx *ctx, void *data)
{
  grn_nfkc_normalize_options *options = data;
  grn_nfkc_normalize_options_fin(ctx, options);
  GRN_FREE(options);
}

static grn_obj *
nfkc100_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_obj *table;
  grn_nfkc_normalize_options *options;
  grn_nfkc_normalize_options options_raw;

  table = grn_string_get_table(ctx, string);
  if (table) {
    options = grn_table_cache_normalizer_options(ctx,
                                                 table,
                                                 string,
                                                 nfkc100_open_options,
                                                 nfkc100_close_options,
                                                 NULL);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  } else {
    grn_nfkc100_normalize_options_init(ctx, &options_raw);
    options = &options_raw;
  }

  grn_nfkc_normalize(ctx, string, options);
  if (!table) {
    grn_nfkc_normalize_options_fin(ctx, options);
  }
  return NULL;
}

static void *
nfkc121_open_options(grn_ctx *ctx,
                     grn_obj *normalizer,
                     grn_obj *raw_options,
                     void *user_data)
{
  grn_nfkc_normalize_options *options;

  options = GRN_MALLOC(sizeof(grn_nfkc_normalize_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][nfkc121] "
        "failed to allocate memory for options");
    return NULL;
  }

  grn_nfkc121_normalize_options_init(ctx, options);

  grn_nfkc_normalize_options_apply(ctx, options, raw_options);

  return options;
}

static void
nfkc121_close_options(grn_ctx *ctx, void *data)
{
  grn_nfkc_normalize_options *options = data;
  grn_nfkc_normalize_options_fin(ctx, options);
  GRN_FREE(options);
}

static grn_obj *
nfkc121_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_obj *table;
  grn_nfkc_normalize_options *options;
  grn_nfkc_normalize_options options_raw;

  table = grn_string_get_table(ctx, string);
  if (table) {
    options = grn_table_cache_normalizer_options(ctx,
                                                 table,
                                                 string,
                                                 nfkc121_open_options,
                                                 nfkc121_close_options,
                                                 NULL);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  } else {
    grn_nfkc121_normalize_options_init(ctx, &options_raw);
    options = &options_raw;
  }

  grn_nfkc_normalize(ctx, string, options);
  if (!table) {
    grn_nfkc_normalize_options_fin(ctx, options);
  }
  return NULL;
}
#endif /* GRN_WITH_NFKC */

grn_rc
grn_normalizer_normalize(grn_ctx *ctx, grn_obj *normalizer, grn_obj *string)
{
  grn_rc rc;
  int nargs = 0;

  grn_ctx_push(ctx, string);
  nargs++;
  rc = grn_proc_call(ctx, normalizer, nargs, NULL);
  grn_ctx_pop(ctx);

  return rc;
}

grn_rc
grn_db_init_builtin_normalizers(grn_ctx *ctx)
{
  const char *normalizer_nfkc51_name = "NormalizerNFKC51";
  const char *normalizer_nfkc100_name = "NormalizerNFKC100";
  const char *normalizer_nfkc121_name = "NormalizerNFKC121";

  grn_normalizer_register(ctx, GRN_NORMALIZER_AUTO_NAME, -1,
                          NULL, auto_next, NULL);

#ifdef GRN_WITH_NFKC
  grn_normalizer_register(ctx, normalizer_nfkc51_name, -1,
                          NULL, nfkc51_next, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc100_name, -1,
                          NULL, nfkc100_next, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc121_name, -1,
                          NULL, nfkc121_next, NULL);
#else /* GRN_WITH_NFKC */
  grn_normalizer_register(ctx, normalizer_nfkc51_name, -1,
                          NULL, NULL, NULL);
#endif /* GRN_WITH_NFKC */
/*
  grn_normalizer_register(ctx, "NormalizerUCA", -1,
                          NULL, uca_next, NULL);
*/

  return GRN_SUCCESS;
}
