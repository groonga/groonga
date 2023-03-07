/*
  Copyright(C) 2012-2018  Brazil
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>
  Copyright(C) 2022  Horimoto Yasuhiro <horimoto@clear-code.com>

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

#include "grn_ii.h"
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
    name_length = (int)strlen(name_ptr);
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
                *d++ = (unsigned char)(c >> 8);
                *d = (unsigned char)(c & 0xff);
              }
              break;
            case 0xa1eb :
              if (d > d0 + 1 && d[-2] == 0xa5
                  && 0xcf <= d[-1] && d[-1] <= 0xdb && (b = handaku[d[-1] - 0xcf])) {
                *(d - 1) = b;
                if (ch) { ch[-1] += 2; s_ += 2; }
                continue;
              } else {
                *d++ = (unsigned char)(c >> 8);
                *d = (unsigned char)(c & 0xff);
              }
              break;
            default :
              *d++ = (unsigned char)(c >> 8);
              *d = (unsigned char)(c & 0xff);
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
  nstr->n_characters = (unsigned int)length;
  nstr->normalized_length_in_bytes =
    (unsigned int)(d - (unsigned char *)nstr->normalized);
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
            *d++ = (unsigned char)(c >> 8);
            *d = (unsigned char)(c & 0xff);
          }
          break;
        case 0x814b :
          if (d > d0 + 1 && d[-2] == 0x83
              && 0x6e <= d[-1] && d[-1] <= 0x7a && (b = handaku[d[-1] - 0x6e])) {
            *(d - 1) = b;
            if (ch) { ch[-1]++; s_++; }
            continue;
          } else {
            *d++ = (unsigned char)(c >> 8);
            *d = (unsigned char)(c & 0xff);
          }
          break;
        default :
          *d++ = (unsigned char)(c >> 8);
          *d = (unsigned char)(c & 0xff);
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
  nstr->n_characters = (unsigned int)length;
  nstr->normalized_length_in_bytes =
    (unsigned int)(d - (unsigned char *)nstr->normalized);
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
      unsigned char mod3 = (unsigned char)((utf8_char[2] - 1) % 3);
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
      unsigned char mod3 = (unsigned char)((utf8_char[2] - 2) % 3);
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

grn_inline static bool
grn_nfkc_normalize_is_hyphen(const unsigned char *utf8_char,
                             size_t length)
{
  /* U+002D HYPHEN-MINUS */
  return (length == 1 && utf8_char[0] == '-');
}

grn_inline static bool
grn_nfkc_normalize_is_prolonged_sound_mark(const unsigned char *utf8_char,
                                           size_t length)
{
  /* U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK */
  return (length == 3 &&
          utf8_char[0] == 0xe3 &&
          utf8_char[1] == 0x83 &&
          utf8_char[2] == 0xbc);
}

grn_inline static grn_bool
grn_nfkc_normalize_is_hyphen_family(const unsigned char *utf8_char,
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
grn_nfkc_normalize_is_prolonged_sound_mark_family(const unsigned char *utf8_char,
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
                                     unsigned char *unified)
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
                                   grn_nfkc_normalize_context *unify,
                                   bool before)
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

    char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
    unified_char_length = char_length;

    if (data->context.t) {
      char_type = data->context.types[i_character];
    } else {
      char_type = data->options->char_type_func(current);
    }

    if (before &&
        data->options->unify_kana &&
        GRN_CHAR_TYPE(char_type) == GRN_CHAR_KATAKANA &&
        unified_char_length == 3) {
      unifying = grn_nfkc_normalize_unify_kana(unifying, unified_kana);
      if (unifying == unified_kana) {
        char_type = GRN_CHAR_HIRAGANA | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (before &&
        data->options->unify_to_katakana &&
        GRN_CHAR_TYPE(char_type) == GRN_CHAR_HIRAGANA &&
        unified_char_length == 3) {
      unifying = grn_nfkc_normalize_unify_to_katakana(unifying,
                                                      unified_katakana);
      if (unifying == unified_katakana) {
        char_type = GRN_CHAR_KATAKANA | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (!before && data->options->unify_kana_case) {
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

    if (before && data->options->unify_kana_voiced_sound_mark) {
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

    if (before && data->options->unify_hyphen) {
      if (grn_nfkc_normalize_is_hyphen_family(unifying, unified_char_length)) {
        unifying = unified_hyphen;
        unified_char_length = sizeof(unified_hyphen);
        char_type = GRN_CHAR_SYMBOL | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (before && data->options->unify_prolonged_sound_mark) {
      if (grn_nfkc_normalize_is_prolonged_sound_mark_family(unifying,
                                                            unified_char_length)) {
        unifying = unified_prolonged_sound_mark;
        unified_char_length = sizeof(unified_prolonged_sound_mark);
        char_type = GRN_CHAR_KATAKANA | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (before && data->options->unify_hyphen_and_prolonged_sound_mark) {
      if (grn_nfkc_normalize_is_hyphen_family(unifying, unified_char_length) ||
          grn_nfkc_normalize_is_prolonged_sound_mark_family(unifying,
                                                            unified_char_length)) {
        unifying = unified_hyphen;
        unified_char_length = sizeof(unified_hyphen);
        char_type = GRN_CHAR_SYMBOL | (char_type & GRN_CHAR_BLANK);
      }
    }

    if (before && data->options->unify_middle_dot) {
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
      *(unify->t++) = (uint8_t)char_type;
    }
    if (unify->c) {
      size_t i;
      if (unifying == current) {
        memcpy(unify->c,
               data->context.checks + i_byte,
               sizeof(int16_t) * char_length);
        unify->c += char_length;
      } else {
        *(unify->c++) += data->context.checks[i_byte];
        for (i = 1; i < unified_char_length; i++) {
          *(unify->c++) = 0;
        }
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
                                          const unsigned char *start,
                                          const unsigned char *current,
                                          const unsigned char *end,
                                          size_t *n_used_bytes,
                                          size_t *n_used_characters,
                                          unsigned char *unified_buffer,
                                          size_t *n_unified_bytes,
                                          size_t *n_unified_characters,
                                          void *user_data);

static const unsigned char *
grn_nfkc_normalize_unify_katakana_v_sounds(grn_ctx *ctx,
                                           const unsigned char *start,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters,
                                           void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  /* U+30F4 KATAKANA LETTER VU */
  if (char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0xb4) {
    const unsigned char *next = current + char_length;
    size_t next_char_length;

    next_char_length = (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
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
                                           const unsigned char *start,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters,
                                           void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
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

    next_char_length = (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
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

static const unsigned char *
grn_nfkc_normalize_unify_katakana_du_small_sounds(grn_ctx *ctx,
                                                  const unsigned char *start,
                                                  const unsigned char *current,
                                                  const unsigned char *end,
                                                  size_t *n_used_bytes,
                                                  size_t *n_used_characters,
                                                  unsigned char *unified_buffer,
                                                  size_t *n_unified_bytes,
                                                  size_t *n_unified_characters,
                                                  void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (char_length == 3 &&
      /* U+30C5 KATAKANA LETTER DU */
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0x85) {
    const unsigned char *next = current + char_length;
    size_t next_char_length;

    next_char_length = (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3 &&
        next[0] == 0xe3 &&
        next[1] == 0x82) {
      if (next[2] == 0xa1) { /* U+30A1 KATAKANA LETTER SMALL A */
        /* U+30B6 KATAKANA LETTER ZA */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xb6;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa3) { /* U+30A3 KATAKANA LETTER SMALL I */
        /* U+30B8 KATAKANA LETTER ZI */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xb8;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa7) { /* U+30A7 KATAKANA LETTER SMALL E */
        /* U+30BC KATAKANA LETTER ZE */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xbC;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa9) { /* U+30A8 KATAKANA LETTER SMALL O */
        /* U+30BE KATAKANA LETTER ZO */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xbe;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      }
    }
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_du_sound(grn_ctx *ctx,
                                           const unsigned char *start,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters,
                                           void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (char_length == 3 &&
      /* U+30C5 KATAKANA LETTER DU */
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0x85) {
    /* U+30BA KATAKANA LETTER ZU */
    unified_buffer[(*n_unified_bytes)++] = current[0];
    unified_buffer[(*n_unified_bytes)++] = 0x82;
    unified_buffer[(*n_unified_bytes)++] = 0xba;
    (*n_unified_characters)++;
    return unified_buffer;
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_zu_small_sounds(grn_ctx *ctx,
                                                  const unsigned char *start,
                                                  const unsigned char *current,
                                                  const unsigned char *end,
                                                  size_t *n_used_bytes,
                                                  size_t *n_used_characters,
                                                  unsigned char *unified_buffer,
                                                  size_t *n_unified_bytes,
                                                  size_t *n_unified_characters,
                                                  void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (char_length == 3 &&
      /* U+30BA KATAKANA LETTER ZU */
      current[0] == 0xe3 &&
      current[1] == 0x82 &&
      current[2] == 0xba) {
    const unsigned char *next = current + char_length;
    size_t next_char_length;

    next_char_length = (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3 &&
        next[0] == 0xe3 &&
        next[1] == 0x82) {
      if (next[2] == 0xa1) { /* U+30A1 KATAKANA LETTER SMALL A */
        /* U+30B6 KATAKANA LETTER ZA */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xb6;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa3) { /* U+30A3 KATAKANA LETTER SMALL I */
        /* U+30B8 KATAKANA LETTER ZI */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xb8;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa7) { /* U+30A7 KATAKANA LETTER SMALL E */
        /* U+30BC KATAKANA LETTER ZE */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xbC;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa9) { /* U+30A8 KATAKANA LETTER SMALL O */
        /* U+30BE KATAKANA LETTER ZO */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xbe;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      }
    }
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_wo_sound(grn_ctx *ctx,
                                           const unsigned char *start,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters,
                                           void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  /* U+30F2 KATAKANA LETTER WO */
  if (char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0xb2) {
    (*n_unified_characters)++;
    /* U+30AA KATAKANA LETTER O */
    unified_buffer[(*n_unified_bytes)++] = current[0];
    unified_buffer[(*n_unified_bytes)++] = 0x82;
    unified_buffer[(*n_unified_bytes)++] = 0xaa;
    return unified_buffer;
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_di_sound(grn_ctx *ctx,
                                           const unsigned char *start,
                                           const unsigned char *current,
                                           const unsigned char *end,
                                           size_t *n_used_bytes,
                                           size_t *n_used_characters,
                                           unsigned char *unified_buffer,
                                           size_t *n_unified_bytes,
                                           size_t *n_unified_characters,
                                           void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  /* U+30C2 KATAKANA LETTER DI */
  if (char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x83 &&
      current[2] == 0x82) {
    (*n_unified_characters)++;
    /* U+30B8 KATAKANA LETTER ZI */
    unified_buffer[(*n_unified_bytes)++] = current[0];
    unified_buffer[(*n_unified_bytes)++] = 0x82;
    unified_buffer[(*n_unified_bytes)++] = 0xb8;
    return unified_buffer;
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_gu_small_sounds(grn_ctx *ctx,
                                                  const unsigned char *start,
                                                  const unsigned char *current,
                                                  const unsigned char *end,
                                                  size_t *n_used_bytes,
                                                  size_t *n_used_characters,
                                                  unsigned char *unified_buffer,
                                                  size_t *n_unified_bytes,
                                                  size_t *n_unified_characters,
                                                  void *user_data)
{
  size_t char_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  /* U+30B0 KATAKANA LETTER GU */
  if (char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x82 &&
      current[2] == 0xb0) {
    const unsigned char *next = current + char_length;
    size_t next_char_length;

    next_char_length = (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3 &&
        next[0] == 0xe3 &&
        next[1] == 0x82) {
      if (next[2] == 0xa1) { /* U+30A1 KATAKANA LETTER SMALL A */
        /* U+30AC KATAKANA LETTER GA */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0xac;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa3) { /* U+30A3 KATAKANA LETTER SMALL I */
        /* U+30AE KATAKANA LETTER GI */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0xae;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa7) { /* U+30A7 KATAKANA LETTER SMALL E */
        /* U+30B2 KATAKANA LETTER GE */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0xb2;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      } else if (next[2] == 0xa9) { /* U+30A8 KATAKANA LETTER SMALL O */
        /* U+30B4 KATAKANA LETTER GO */
        unified_buffer[(*n_unified_bytes)++] = current[0];
        unified_buffer[(*n_unified_bytes)++] = current[1];
        unified_buffer[(*n_unified_bytes)++] = 0xb4;
        (*n_unified_characters)++;
        (*n_used_bytes) += next_char_length;
        (*n_used_characters)++;
        return unified_buffer;
      }
    }
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_katakana_trailing_o(grn_ctx *ctx,
                                             const unsigned char *start,
                                             const unsigned char *current,
                                             const unsigned char *end,
                                             size_t *n_used_bytes,
                                             size_t *n_used_characters,
                                             unsigned char *unified_buffer,
                                             size_t *n_unified_bytes,
                                             size_t *n_unified_characters,
                                             void *user_data)
{
  size_t char_length;
  bool *need_trailing_check = (bool *)user_data;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (*need_trailing_check &&
      /* U+30AA KATAKANA LETTER O */
      char_length == 3 &&
      current[0] == 0xe3 &&
      current[1] == 0x82 &&
      current[2] == 0xaa) {
    /* U+30A6 KATAKANA LETTER U */
    unified_buffer[(*n_unified_bytes)++] = current[0];
    unified_buffer[(*n_unified_bytes)++] = current[1];
    unified_buffer[(*n_unified_bytes)++] = 0xa6;
    (*n_unified_characters)++;
    *need_trailing_check = false;
    return unified_buffer;
  }

  *need_trailing_check = char_length == 3 &&
                         current[0] == 0xe3 &&
                         /* U+30AA KATAKANA LETTER O */
                         ((current[1] == 0x82 && current[2] == 0xaa) ||
                          /* U+30B3 KATAKANA LETTER KO */
                          (current[1] == 0x82 && current[2] == 0xb3) ||
                          /* U+30BD KATAKANA LETTER SO */
                          (current[1] == 0x82 && current[2] == 0xbd) ||
                          /* U+30C8 KATAKANA LETTER TO */
                          (current[1] == 0x83 && current[2] == 0x88) ||
                          /* U+30CE KATAKANA LETTER NO */
                          (current[1] == 0x83 && current[2] == 0x8e) ||
                          /* U+30DB KATAKANA LETTER HO */
                          (current[1] == 0x83 && current[2] == 0x9b) ||
                          /* U+30E2 KATAKANA LETTER MO */
                          (current[1] == 0x83 && current[2] == 0xa2) ||
                          /* U+30E8 KATAKANA LETTER YO */
                          (current[1] == 0x83 && current[2] == 0xa8) ||
                          /* U+30ED KATAKANA LETTER RO */
                          (current[1] == 0x83 && current[2] == 0xad) ||
                          /* U+30B4 KATAKANA LETTER GO */
                          (current[1] == 0x82 && current[2] == 0xb4) ||
                          /* U+30BE KATAKANA LETTER ZO */
                          (current[1] == 0x82 && current[2] == 0xbe) ||
                          /* U+30C9 KATAKANA LETTER DO */
                          (current[1] == 0x83 && current[2] == 0x89) ||
                          /* U+30DC KATAKANA LETTER BO */
                          (current[1] == 0x83 && current[2] == 0x9c) ||
                          /* U+30DD KATAKANA LETTER PO */
                          (current[1] == 0x83 && current[2] == 0x9d));
  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

typedef bool
(*grn_nfkc_normalize_is_target_char_func)(const unsigned char *utf8_char,
                                          size_t length);

typedef struct {
  grn_nfkc_normalize_is_target_char_func is_target_char;
  size_t previous_length;
} grn_nfkc_normalize_prolonged_sound_mark_like_data;

static const unsigned char *
grn_nfkc_normalize_unify_kana_prolonged_sound_mark_like(grn_ctx *ctx,
                                                        const unsigned char *start,
                                                        const unsigned char *current,
                                                        const unsigned char *end,
                                                        size_t *n_used_bytes,
                                                        size_t *n_used_characters,
                                                        unsigned char *unified_buffer,
                                                        size_t *n_unified_bytes,
                                                        size_t *n_unified_characters,
                                                        void *user_data)
{
  size_t char_length;
  grn_nfkc_normalize_prolonged_sound_mark_like_data *data = user_data;
  size_t previous_length = data->previous_length;

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
  data->previous_length = char_length;

  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (previous_length == 3 &&
      data->is_target_char(current, char_length)) {
    const unsigned char *previous = current - previous_length;
    if (previous[0] == 0xe3) {
      if (/* U+3041 HIRAGANA LETTER SMALL A */
          (previous[1] == 0x81 && previous[2] == 0x81) ||
          /* U+3042 HIRAGANA LETTER A */
          (previous[1] == 0x81 && previous[2] == 0x82) ||
          /* U+304B HIRAGANA LETTER KA */
          (previous[1] == 0x81 && previous[2] == 0x8b) ||
          /* U+304C HIRAGANA LETTER GA */
          (previous[1] == 0x81 && previous[2] == 0x8c) ||
          /* U+3055 HIRAGANA LETTER SA */
          (previous[1] == 0x81 && previous[2] == 0x95) ||
          /* U+3056 HIRAGANA LETTER ZA */
          (previous[1] == 0x81 && previous[2] == 0x96) ||
          /* U+305F HIRAGANA LETTER TA */
          (previous[1] == 0x81 && previous[2] == 0x9f) ||
          /* U+3060 HIRAGANA LETTER DA */
          (previous[1] == 0x81 && previous[2] == 0xa0) ||
          /* U+306A HIRAGANA LETTER NA */
          (previous[1] == 0x81 && previous[2] == 0xaa) ||
          /* U+306F HIRAGANA LETTER HA */
          (previous[1] == 0x81 && previous[2] == 0xaf) ||
          /* U+3070 HIRAGANA LETTER BA */
          (previous[1] == 0x81 && previous[2] == 0xb0) ||
          /* U+3071 HIRAGANA LETTER PA */
          (previous[1] == 0x81 && previous[2] == 0xb1) ||
          /* U+307E HIRAGANA LETTER MA */
          (previous[1] == 0x81 && previous[2] == 0xbe) ||
          /* U+3083 HIRAGANA LETTER SMALL YA */
          (previous[1] == 0x82 && previous[2] == 0x83) ||
          /* U+3084 HIRAGANA LETTER YA */
          (previous[1] == 0x82 && previous[2] == 0x84) ||
          /* U+3089 HIRAGANA LETTER RA */
          (previous[1] == 0x82 && previous[2] == 0x89) ||
          /* U+308E HIRAGANA LETTER SMALL WA */
          (previous[1] == 0x82 && previous[2] == 0x8e) ||
          /* U+308F HIRAGANA LETTER WA */
          (previous[1] == 0x82 && previous[2] == 0x8f) ||
          /* U+3095 HIRAGANA LETTER SMALL KA */
          (previous[1] == 0x82 && previous[2] == 0x95)) {
        /* U+3042 HIRAGANA LETTER A */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x81;
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+3043 HIRAGANA LETTER SMALL I */
                 (previous[1] == 0x81 && previous[2] == 0x83) ||
                 /* U+3044 HIRAGANA LETTER I */
                 (previous[1] == 0x81 && previous[2] == 0x84) ||
                 /* U+304D HIRAGANA LETTER KI */
                 (previous[1] == 0x81 && previous[2] == 0x8d) ||
                 /* U+304E HIRAGANA LETTER GI */
                 (previous[1] == 0x81 && previous[2] == 0x8e) ||
                 /* U+3057 HIRAGANA LETTER SI */
                 (previous[1] == 0x81 && previous[2] == 0x97) ||
                 /* U+3058 HIRAGANA LETTER ZI */
                 (previous[1] == 0x81 && previous[2] == 0x98) ||
                 /* U+3061 HIRAGANA LETTER TI */
                 (previous[1] == 0x81 && previous[2] == 0xa1) ||
                 /* U+3062 HIRAGANA LETTER DI */
                 (previous[1] == 0x81 && previous[2] == 0xa2) ||
                 /* U+306B HIRAGANA LETTER NI */
                 (previous[1] == 0x81 && previous[2] == 0xab) ||
                 /* U+3072 HIRAGANA LETTER HI */
                 (previous[1] == 0x81 && previous[2] == 0xb2) ||
                 /* U+3073 HIRAGANA LETTER BI */
                 (previous[1] == 0x81 && previous[2] == 0xb3) ||
                 /* U+3074 HIRAGANA LETTER PI */
                 (previous[1] == 0x81 && previous[2] == 0xb4) ||
                 /* U+307F HIRAGANA LETTER MI */
                 (previous[1] == 0x81 && previous[2] == 0xbf) ||
                 /* U+308A HIRAGANA LETTER RI */
                 (previous[1] == 0x82 && previous[2] == 0x8a) ||
                 /* U+3090 HIRAGANA LETTER WI */
                 (previous[1] == 0x82 && previous[2] == 0x90)) {
        /* U+3044 HIRAGANA LETTER I */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x81;
        unified_buffer[(*n_unified_bytes)++] = 0x84;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+3045 HIRAGANA LETTER SMALL U */
                 (previous[1] == 0x81 && previous[2] == 0x85) ||
                 /* U+3046 HIRAGANA LETTER U */
                 (previous[1] == 0x81 && previous[2] == 0x86) ||
                 /* U+304F HIRAGANA LETTER KU */
                 (previous[1] == 0x81 && previous[2] == 0x8f) ||
                 /* U+3050 HIRAGANA LETTER GU */
                 (previous[1] == 0x81 && previous[2] == 0x90) ||
                 /* U+3059 HIRAGANA LETTER SU */
                 (previous[1] == 0x81 && previous[2] == 0x99) ||
                 /* U+305A HIRAGANA LETTER ZU */
                 (previous[1] == 0x81 && previous[2] == 0x9a) ||
                 /* U+3063 HIRAGANA LETTER SMALL TU */
                 (previous[1] == 0x81 && previous[2] == 0xa3) ||
                 /* U+3064 HIRAGANA LETTER TU */
                 (previous[1] == 0x81 && previous[2] == 0xa4) ||
                 /* U+3065 HIRAGANA LETTER DU */
                 (previous[1] == 0x81 && previous[2] == 0xa5) ||
                 /* U+306C HIRAGANA LETTER NU */
                 (previous[1] == 0x81 && previous[2] == 0xac) ||
                 /* U+3075 HIRAGANA LETTER HU */
                 (previous[1] == 0x81 && previous[2] == 0xb5) ||
                 /* U+3076 HIRAGANA LETTER BU */
                 (previous[1] == 0x81 && previous[2] == 0xb6) ||
                 /* U+3077 HIRAGANA LETTER PU */
                 (previous[1] == 0x81 && previous[2] == 0xb7) ||
                 /* U+3080 HIRAGANA LETTER MU */
                 (previous[1] == 0x82 && previous[2] == 0x80) ||
                 /* U+3085 HIRAGANA LETTER SMALL YU */
                 (previous[1] == 0x82 && previous[2] == 0x85) ||
                 /* U+3086 HIRAGANA LETTER YU */
                 (previous[1] == 0x82 && previous[2] == 0x86) ||
                 /* U+308B HIRAGANA LETTER RU */
                 (previous[1] == 0x82 && previous[2] == 0x8b) ||
                 /* U+3094 HIRAGANA LETTER VU */
                 (previous[1] == 0x82 && previous[2] == 0x94)) {
        /* U+3046 HIRAGANA LETTER U */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x81;
        unified_buffer[(*n_unified_bytes)++] = 0x86;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+3047 HIRAGANA LETTER SMALL E */
                 (previous[1] == 0x81 && previous[2] == 0x87) ||
                 /* U+3048 HIRAGANA LETTER E */
                 (previous[1] == 0x81 && previous[2] == 0x88) ||
                 /* U+3051 HIRAGANA LETTER KE */
                 (previous[1] == 0x81 && previous[2] == 0x91) ||
                 /* U+3052 HIRAGANA LETTER GE */
                 (previous[1] == 0x81 && previous[2] == 0x92) ||
                 /* U+305B HIRAGANA LETTER SE */
                 (previous[1] == 0x81 && previous[2] == 0x9b) ||
                 /* U+305C HIRAGANA LETTER ZE */
                 (previous[1] == 0x81 && previous[2] == 0x9c) ||
                 /* U+3066 HIRAGANA LETTER TE */
                 (previous[1] == 0x81 && previous[2] == 0xa6) ||
                 /* U+3067 HIRAGANA LETTER DE */
                 (previous[1] == 0x81 && previous[2] == 0xa7) ||
                 /* U+306D HIRAGANA LETTER NE */
                 (previous[1] == 0x81 && previous[2] == 0xad) ||
                 /* U+3078 HIRAGANA LETTER HE */
                 (previous[1] == 0x81 && previous[2] == 0xb8) ||
                 /* U+3079 HIRAGANA LETTER BE */
                 (previous[1] == 0x81 && previous[2] == 0xb9) ||
                 /* U+307A HIRAGANA LETTER PE */
                 (previous[1] == 0x81 && previous[2] == 0xba) ||
                 /* U+3081 HIRAGANA LETTER ME */
                 (previous[1] == 0x82 && previous[2] == 0x81) ||
                 /* U+308C HIRAGANA LETTER RE */
                 (previous[1] == 0x82 && previous[2] == 0x8c) ||
                 /* U+3096 HIRAGANA LETTER SMALL KE */
                 (previous[1] == 0x82 && previous[2] == 0x96) ||
                 /* U+3091 HIRAGANA LETTER WE */
                 (previous[1] == 0x82 && previous[2] == 0x91)) {
        /* U+3048 HIRAGANA LETTER E */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x81;
        unified_buffer[(*n_unified_bytes)++] = 0x88;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+3049 HIRAGANA LETTER SMALL O */
                 (previous[1] == 0x81 && previous[2] == 0x89) ||
                 /* U+304A HIRAGANA LETTER O */
                 (previous[1] == 0x81 && previous[2] == 0x8a) ||
                 /* U+3053 HIRAGANA LETTER KO */
                 (previous[1] == 0x81 && previous[2] == 0x93) ||
                 /* U+3054 HIRAGANA LETTER GO */
                 (previous[1] == 0x81 && previous[2] == 0x94) ||
                 /* U+305D HIRAGANA LETTER SO */
                 (previous[1] == 0x81 && previous[2] == 0x9d) ||
                 /* U+305E HIRAGANA LETTER ZO */
                 (previous[1] == 0x81 && previous[2] == 0x9e) ||
                 /* U+3068 HIRAGANA LETTER TO */
                 (previous[1] == 0x81 && previous[2] == 0xa8) ||
                 /* U+3069 HIRAGANA LETTER DO */
                 (previous[1] == 0x81 && previous[2] == 0xa9) ||
                 /* U+306E HIRAGANA LETTER NO */
                 (previous[1] == 0x81 && previous[2] == 0xae) ||
                 /* U+307B HIRAGANA LETTER HO */
                 (previous[1] == 0x81 && previous[2] == 0xbb) ||
                 /* U+307C HIRAGANA LETTER BO */
                 (previous[1] == 0x81 && previous[2] == 0xbc) ||
                 /* U+307D HIRAGANA LETTER PO */
                 (previous[1] == 0x81 && previous[2] == 0xbd) ||
                 /* U+3082 HIRAGANA LETTER MO */
                 (previous[1] == 0x82 && previous[2] == 0x82) ||
                 /* U+3087 HIRAGANA LETTER SMALL YO */
                 (previous[1] == 0x82 && previous[2] == 0x87) ||
                 /* U+3088 HIRAGANA LETTER YO */
                 (previous[1] == 0x82 && previous[2] == 0x88) ||
                 /* U+308D HIRAGANA LETTER RO */
                 (previous[1] == 0x82 && previous[2] == 0x8d) ||
                 /* U+3092 HIRAGANA LETTER WO */
                 (previous[1] == 0x82 && previous[2] == 0x92)) {
        /* U+304A HIRAGANA LETTER O */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x81;
        unified_buffer[(*n_unified_bytes)++] = 0x8a;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+3093 HIRAGANA LETTER N */
                 previous[1] == 0x82 && previous[2] == 0x93) {
        /* U+3093 HIRAGANA LETTER N */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = previous[1];
        unified_buffer[(*n_unified_bytes)++] = previous[2];
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+30A1 KATAKANA LETTER SMALL A */
                 (previous[1] == 0x82 && previous[2] == 0xa1) ||
                 /* U+30A2 KATAKANA LETTER A */
                 (previous[1] == 0x82 && previous[2] == 0xa2) ||
                 /* U+30AB KATAKANA LETTER KA */
                 (previous[1] == 0x82 && previous[2] == 0xab) ||
                 /* U+30AC KATAKANA LETTER GA */
                 (previous[1] == 0x82 && previous[2] == 0xac) ||
                 /* U+30B5 KATAKANA LETTER SA */
                 (previous[1] == 0x82 && previous[2] == 0xb5) ||
                 /* U+30B6 KATAKANA LETTER ZA */
                 (previous[1] == 0x82 && previous[2] == 0xb6) ||
                 /* U+30BF KATAKANA LETTER TA */
                 (previous[1] == 0x82 && previous[2] == 0xbf) ||
                 /* U+30C0 KATAKANA LETTER DA */
                 (previous[1] == 0x83 && previous[2] == 0x80) ||
                 /* U+30CA KATAKANA LETTER NA */
                 (previous[1] == 0x83 && previous[2] == 0x8a) ||
                 /* U+30CF KATAKANA LETTER HA */
                 (previous[1] == 0x83 && previous[2] == 0x8f) ||
                 /* U+30D0 KATAKANA LETTER BA */
                 (previous[1] == 0x83 && previous[2] == 0x90) ||
                 /* U+30D1 KATAKANA LETTER PA */
                 (previous[1] == 0x83 && previous[2] == 0x91) ||
                 /* U+30DE KATAKANA LETTER MA */
                 (previous[1] == 0x83 && previous[2] == 0x9e) ||
                 /* U+30E9 KATAKANA LETTER RA */
                 (previous[1] == 0x83 && previous[2] == 0xa9) ||
                 /* U+30E3 KATAKANA LETTER SMALL YA */
                 (previous[1] == 0x83 && previous[2] == 0xa3) ||
                 /* U+30E4 KATAKANA LETTER YA */
                 (previous[1] == 0x83 && previous[2] == 0xa4) ||
                 /* U+30EE KATAKANA LETTER SMALL WA */
                 (previous[1] == 0x83 && previous[2] == 0xae) ||
                 /* U+30EF KATAKANA LETTER WA */
                 (previous[1] == 0x83 && previous[2] == 0xaf) ||
                 /* U+30F5 KATAKANA LETTER SMALL KA */
                 (previous[1] == 0x83 && previous[2] == 0xb5) ||
                 /* U+30F7 KATAKANA LETTER VA */
                 (previous[1] == 0x83 && previous[2] == 0xb7)) {
        /* U+30A2 KATAKANA LETTER A */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xa2;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+30A3 KATAKANA LETTER SMALL I */
                 (previous[1] == 0x82 && previous[2] == 0xa3) ||
                 /* U+30A4 KATAKANA LETTER I */
                 (previous[1] == 0x82 && previous[2] == 0xa4) ||
                 /* U+30AD KATAKANA LETTER KI */
                 (previous[1] == 0x82 && previous[2] == 0xad) ||
                 /* U+30AE KATAKANA LETTER GI */
                 (previous[1] == 0x82 && previous[2] == 0xae) ||
                 /* U+30B7 KATAKANA LETTER SI */
                 (previous[1] == 0x82 && previous[2] == 0xb7) ||
                 /* U+30B8 KATAKANA LETTER ZI */
                 (previous[1] == 0x82 && previous[2] == 0xb8) ||
                 /* U+30C1 KATAKANA LETTER TI */
                 (previous[1] == 0x83 && previous[2] == 0x81) ||
                 /* U+30C2 KATAKANA LETTER DI */
                 (previous[1] == 0x83 && previous[2] == 0x82) ||
                 /* U+30CB KATAKANA LETTER NI */
                 (previous[1] == 0x83 && previous[2] == 0x8b) ||
                 /* U+30D2 KATAKANA LETTER HI */
                 (previous[1] == 0x83 && previous[2] == 0x92) ||
                 /* U+30D3 KATAKANA LETTER BI */
                 (previous[1] == 0x83 && previous[2] == 0x93) ||
                 /* U+30D4 KATAKANA LETTER PI */
                 (previous[1] == 0x83 && previous[2] == 0x94) ||
                 /* U+30DF KATAKANA LETTER MI */
                 (previous[1] == 0x83 && previous[2] == 0x9f) ||
                 /* U+30EA KATAKANA LETTER RI */
                 (previous[1] == 0x83 && previous[2] == 0xaa) ||
                 /* U+30F0 KATAKANA LETTER WI */
                 (previous[1] == 0x83 && previous[2] == 0xb0) ||
                 /* U+30F8 KATAKANA LETTER VI */
                 (previous[1] == 0x83 && previous[2] == 0xb8)) {
        /* U+30A4 KATAKANA LETTER I */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xa4;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+30A5 KATAKANA LETTER SMALL U */
                 (previous[1] == 0x82 && previous[2] == 0xa5) ||
                 /* U+30A6 KATAKANA LETTER U */
                 (previous[1] == 0x82 && previous[2] == 0xa6) ||
                 /* U+30AF KATAKANA LETTER KU */
                 (previous[1] == 0x82 && previous[2] == 0xaf) ||
                 /* U+30B0 KATAKANA LETTER GU */
                 (previous[1] == 0x82 && previous[2] == 0xb0) ||
                 /* U+30B9 KATAKANA LETTER SU */
                 (previous[1] == 0x82 && previous[2] == 0xb9) ||
                 /* U+30BA KATAKANA LETTER ZU */
                 (previous[1] == 0x82 && previous[2] == 0xba) ||
                 /* U+30C4 KATAKANA LETTER TU */
                 (previous[1] == 0x83 && previous[2] == 0x84) ||
                 /* U+30C5 KATAKANA LETTER DU */
                 (previous[1] == 0x83 && previous[2] == 0x85) ||
                 /* U+30CC KATAKANA LETTER NU */
                 (previous[1] == 0x83 && previous[2] == 0x8c) ||
                 /* U+30D5 KATAKANA LETTER HU */
                 (previous[1] == 0x83 && previous[2] == 0x95) ||
                 /* U+30D6 KATAKANA LETTER BU */
                 (previous[1] == 0x83 && previous[2] == 0x96) ||
                 /* U+30D7 KATAKANA LETTER PU */
                 (previous[1] == 0x83 && previous[2] == 0x97) ||
                 /* U+30E0 KATAKANA LETTER MU */
                 (previous[1] == 0x83 && previous[2] == 0xa0) ||
                 /* U+30E5 KATAKANA LETTER SMALL YU */
                 (previous[1] == 0x83 && previous[2] == 0xa5) ||
                 /* U+30E6 KATAKANA LETTER YU */
                 (previous[1] == 0x83 && previous[2] == 0xa6) ||
                 /* U+30EB KATAKANA LETTER RU */
                 (previous[1] == 0x83 && previous[2] == 0xab) ||
                 /* U+30F4 KATAKANA LETTER VU */
                 (previous[1] == 0x83 && previous[2] == 0xb4)) {
        /* U+30A6 KATAKANA LETTER U */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xa6;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+30A7 KATAKANA LETTER SMALL E */
                 (previous[1] == 0x82 && previous[2] == 0xa7) ||
                 /* U+30A8 KATAKANA LETTER E */
                 (previous[1] == 0x82 && previous[2] == 0xa8) ||
                 /* U+30B1 KATAKANA LETTER KE */
                 (previous[1] == 0x82 && previous[2] == 0xb1) ||
                 /* U+30B2 KATAKANA LETTER GE */
                 (previous[1] == 0x82 && previous[2] == 0xb2) ||
                 /* U+30BB KATAKANA LETTER SE */
                 (previous[1] == 0x82 && previous[2] == 0xbb) ||
                 /* U+30BC KATAKANA LETTER ZE */
                 (previous[1] == 0x82 && previous[2] == 0xbc) ||
                 /* U+30C6 KATAKANA LETTER TE */
                 (previous[1] == 0x83 && previous[2] == 0x86) ||
                 /* U+30C7 KATAKANA LETTER DE */
                 (previous[1] == 0x83 && previous[2] == 0x87) ||
                 /* U+30CD KATAKANA LETTER NE */
                 (previous[1] == 0x83 && previous[2] == 0x8d) ||
                 /* U+30D8 KATAKANA LETTER HE */
                 (previous[1] == 0x83 && previous[2] == 0x98) ||
                 /* U+30D9 KATAKANA LETTER BE */
                 (previous[1] == 0x83 && previous[2] == 0x99) ||
                 /* U+30DA KATAKANA LETTER PE */
                 (previous[1] == 0x83 && previous[2] == 0x9a) ||
                 /* U+30E1 KATAKANA LETTER ME */
                 (previous[1] == 0x83 && previous[2] == 0xa1) ||
                 /* U+30EC KATAKANA LETTER RE */
                 (previous[1] == 0x83 && previous[2] == 0xac) ||
                 /* U+30F1 KATAKANA LETTER WE */
                 (previous[1] == 0x83 && previous[2] == 0xb1) ||
                 /* U+30F6 KATAKANA LETTER SMALL KE */
                 (previous[1] == 0x83 && previous[2] == 0xb6) ||
                 /* U+30F9 KATAKANA LETTER VE */
                 (previous[1] == 0x83 && previous[2] == 0xb9)) {
        /* U+30A8 KATAKANA LETTER E */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xa8;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+30A9 KATAKANA LETTER SMALL O */
                 (previous[1] == 0x82 && previous[2] == 0xa9) ||
                 /* U+30AA KATAKANA LETTER O */
                 (previous[1] == 0x82 && previous[2] == 0xaa) ||
                 /* U+30B3 KATAKANA LETTER KO */
                 (previous[1] == 0x82 && previous[2] == 0xb3) ||
                 /* U+30B4 KATAKANA LETTER GO */
                 (previous[1] == 0x82 && previous[2] == 0xb4) ||
                 /* U+30BD KATAKANA LETTER SO */
                 (previous[1] == 0x82 && previous[2] == 0xbd) ||
                 /* U+30BE KATAKANA LETTER ZO */
                 (previous[1] == 0x82 && previous[2] == 0xbe) ||
                 /* U+30C8 KATAKANA LETTER TO */
                 (previous[1] == 0x83 && previous[2] == 0x88) ||
                 /* U+30C9 KATAKANA LETTER DO */
                 (previous[1] == 0x83 && previous[2] == 0x89) ||
                 /* U+30CE KATAKANA LETTER NO */
                 (previous[1] == 0x83 && previous[2] == 0x8e) ||
                 /* U+30DB KATAKANA LETTER HO */
                 (previous[1] == 0x83 && previous[2] == 0x9b) ||
                 /* U+30DC KATAKANA LETTER BO */
                 (previous[1] == 0x83 && previous[2] == 0x9c) ||
                 /* U+30DD KATAKANA LETTER PO */
                 (previous[1] == 0x83 && previous[2] == 0x9d) ||
                 /* U+30E2 KATAKANA LETTER MO */
                 (previous[1] == 0x83 && previous[2] == 0xa2) ||
                 /* U+30E7 KATAKANA LETTER SMALL YO */
                 (previous[1] == 0x83 && previous[2] == 0xa7) ||
                 /* U+30E8 KATAKANA LETTER YO */
                 (previous[1] == 0x83 && previous[2] == 0xa8) ||
                 /* U+30ED KATAKANA LETTER RO */
                 (previous[1] == 0x83 && previous[2] == 0xad) ||
                 /* U+30F2 KATAKANA LETTER WO */
                 (previous[1] == 0x83 && previous[2] == 0xb2) ||
                 /* U+30FA KATAKANA LETTER VO */
                 (previous[1] == 0x83 && previous[2] == 0xba)) {
        /* U+30AA KATAKANA LETTER O */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = 0x82;
        unified_buffer[(*n_unified_bytes)++] = 0xaa;
        (*n_unified_characters)++;
        return unified_buffer;
      } else if (/* U+30F3 KATAKANA LETTER N */
                 previous[1] == 0x83 && previous[2] == 0xb3) {
        /* U+30F3 KATAKANA LETTER N */
        unified_buffer[(*n_unified_bytes)++] = previous[0];
        unified_buffer[(*n_unified_bytes)++] = previous[1];
        unified_buffer[(*n_unified_bytes)++] = previous[2];
        (*n_unified_characters)++;
        return unified_buffer;
      }
    }
  }

  *n_unified_bytes = *n_used_bytes;
  *n_unified_characters = *n_used_characters;

  return current;
}

static const unsigned char *
grn_nfkc_normalize_unify_romaji(grn_ctx *ctx,
                                const unsigned char *start,
                                const unsigned char *current,
                                const unsigned char *end,
                                size_t *n_used_bytes,
                                size_t *n_used_characters,
                                unsigned char *unified_buffer,
                                size_t *n_unified_bytes,
                                size_t *n_unified_characters,
                                void *user_data)
{
  return grn_romaji_hepburn_convert(ctx,
                                    current,
                                    end,
                                    n_used_bytes,
                                    n_used_characters,
                                    unified_buffer,
                                    n_unified_bytes,
                                    n_unified_characters);
}

static const unsigned char *
grn_nfkc_normalize_strip(grn_ctx *ctx,
                         const unsigned char *start,
                         const unsigned char *current,
                         const unsigned char *end,
                         size_t *n_used_bytes,
                         size_t *n_used_characters,
                         unsigned char *unified_buffer,
                         size_t *n_unified_bytes,
                         size_t *n_unified_characters,
                         void *user_data)
{
  bool *striped_from_end = user_data;
  const bool from_start = (start == current);
  const unsigned char *current_keep = current;
  while (current < end) {
    size_t char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
    (*n_used_bytes) += char_length;
    (*n_used_characters)++;

    const bool is_blank = (char_length == 1 && current[0] == ' ');
    if (!is_blank) {
      if (from_start) {
        *n_unified_bytes = char_length;
        *n_unified_characters = 1;
        return current;
      } else {
        *n_unified_bytes = *n_used_bytes;
        *n_unified_characters = *n_used_characters;
        return current_keep;
      }
    }

    current += char_length;
  }
  *striped_from_end = true;
  return current;
}

grn_inline static void
grn_nfkc_normalize_unify_stateful(grn_ctx *ctx,
                                  grn_nfkc_normalize_data *data,
                                  grn_nfkc_normalize_context *unify,
                                  grn_nfkc_normalize_unify_stateful_func func,
                                  void *func_data,
                                  const char *context_tag)
{
  const unsigned char *start = data->context.dest;
  const unsigned char *current = start;
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
                   start, current, end, &n_used_bytes, &n_used_characters,
                   unified_buffer, &n_unified_bytes, &n_unified_characters,
                   func_data);

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
      unify->n_characters += n_unified_characters;
      if (unify->t) {
        memcpy(unify->t,
               data->context.types + i_character,
               sizeof(uint8_t) * n_unified_characters);
        unify->t += n_unified_characters;
      }
      if (unify->c) {
        memcpy(unify->c,
               data->context.checks + i_byte,
               sizeof(int16_t) * n_unified_bytes);
        unify->c += n_unified_bytes;
        unify->c[0] = 0;
      }
      if (unify->o) {
        memcpy(unify->o,
               data->context.offsets + i_character,
               sizeof(uint64_t) * n_unified_characters);
        unify->o += n_unified_characters;
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

            unified_char_length = (size_t)grn_charlen_(ctx,
                                                       unified_current,
                                                       unified_end,
                                                       GRN_ENC_UTF8);
            if (unify->t) {
              grn_char_type unified_char_type =
                data->options->char_type_func(unified_current);
              *(unify->t++) = (uint8_t)unified_char_type;
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
        data->options->unify_katakana_du_small_sounds ||
        data->options->unify_katakana_du_sound ||
        data->options->unify_katakana_zu_small_sounds ||
        data->options->unify_katakana_wo_sound ||
        data->options->unify_katakana_di_sound ||
        data->options->unify_katakana_gu_small_sounds ||
        data->options->unify_kana_hyphen ||
        data->options->unify_kana_prolonged_sound_mark ||
        data->options->unify_katakana_trailing_o ||
        data->options->unify_to_romaji ||
        data->options->unify_to_katakana ||
        data->options->strip)) {
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
      data->options->unify_kana_voiced_sound_mark ||
      data->options->unify_hyphen ||
      data->options->unify_prolonged_sound_mark ||
      data->options->unify_hyphen_and_prolonged_sound_mark ||
      data->options->unify_middle_dot ||
      data->options->unify_to_katakana) {
    grn_nfkc_normalize_unify_stateless(ctx, data, &unify, true);
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
                                      NULL,
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
                                      NULL,
                                      "[unify][katakana-bu-sound]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_du_small_sounds) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_du_small_sounds,
                                      NULL,
                                      "[unify][katakana-du-small-sounds]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_du_sound) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_du_sound,
                                      NULL,
                                      "[unify][katakana-du-sound]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_zu_small_sounds) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_zu_small_sounds,
                                      NULL,
                                      "[unify][katakana-zu-small-sounds]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_wo_sound) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_wo_sound,
                                      NULL,
                                      "[unify][katakana-wo-sound]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_di_sound) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_di_sound,
                                      NULL,
                                      "[unify][katakana-di-sound]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_gu_small_sounds) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_gu_small_sounds,
                                      NULL,
                                      "[unify][katakana-gu-small-sounds]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_kana_hyphen) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_prolonged_sound_mark_like_data subdata;
    subdata.is_target_char = grn_nfkc_normalize_is_hyphen;
    subdata.previous_length = 0;
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_kana_prolonged_sound_mark_like,
                                      &subdata,
                                      "[unify][kana-hyphen]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_kana_prolonged_sound_mark) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_prolonged_sound_mark_like_data subdata;
    subdata.is_target_char = grn_nfkc_normalize_is_prolonged_sound_mark;
    subdata.previous_length = 0;
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_kana_prolonged_sound_mark_like,
                                      &subdata,
                                      "[unify][kana-prolonged-sound-mark]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_katakana_trailing_o) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    bool need_trailing_check = false;
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_unify_katakana_trailing_o,
                                      &need_trailing_check,
                                      "[unify][katakana-trailing-o]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->unify_kana_case) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    grn_nfkc_normalize_unify_stateless(ctx, data, &unify, false);
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
                                      grn_nfkc_normalize_unify_romaji,
                                      NULL,
                                      "[unify][romaji]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    need_swap = GRN_TRUE;
  }

  if (data->options->strip) {
    if (need_swap) {
      grn_nfkc_normalize_context_swap(ctx, &(data->context), &unify);
      grn_nfkc_normalize_context_rewind(ctx, &unify);
    }
    bool striped_from_end = false;
    grn_nfkc_normalize_unify_stateful(ctx,
                                      data,
                                      &unify,
                                      grn_nfkc_normalize_strip,
                                      &striped_from_end,
                                      "[strip]");
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    if (striped_from_end) {
      if (unify.types && unify.n_characters > 0) {
        unify.types[unify.n_characters - 1] |= GRN_CHAR_BLANK;
      }
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

static grn_inline bool
grn_nfkc_normalize_remove_target_blank_character_p(
  grn_ctx *ctx,
  const grn_nfkc_normalize_data *data,
  const unsigned char *current,
  size_t current_length)
{
  if (current_length != 1) {
    return false;
  }

  if (current[0] > ' ') {
    return false;
  }

  switch (current[0]) {
  case ' ' :
    return data->remove_blank_p;
  case '\r' :
  case '\n' :
    return data->options->remove_new_line;
  default :
    /* skip unprintable ascii */
    return true;
  }
}

static grn_inline bool
grn_nfkc_normalize_remove_target_non_blank_character_p(
  grn_ctx *ctx,
  const grn_nfkc_normalize_data *data,
  const unsigned char *current,
  size_t current_length)
{
  if (data->options->char_type_func(current) == GRN_CHAR_SYMBOL) {
    return data->options->remove_symbol;
  }
  return false;
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
    source_char_length =
      (size_t)grn_charlen_(ctx, source, source_end, GRN_ENC_UTF8);
    if (source_char_length == 0) {
      break;
    }
    if (data.remove_tokenized_delimiter_p &&
        grn_tokenizer_is_tokenized_delimiter(ctx,
                                             (const char *)source,
                                             (unsigned int)source_char_length,
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
        current_length =
          (size_t)grn_charlen_(ctx, current, current_end, GRN_ENC_UTF8);
        if (current_length == 0) {
          break;
        }
        if (grn_nfkc_normalize_remove_target_blank_character_p(ctx,
                                                               &data,
                                                               current,
                                                               current_length)) {
          if (context->t > context->types) {
            context->t[-1] |= GRN_CHAR_BLANK;
          }
          if (!data.options->include_removed_source_location) {
            source_ += current_length;
          }
        } else if (grn_nfkc_normalize_remove_target_non_blank_character_p(
                     ctx,
                     &data,
                     current,
                     current_length)) {
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
  data.string->n_characters = (unsigned int)(context->n_characters);
  data.string->normalized = context->dest;
  data.string->normalized_length_in_bytes =
    (unsigned int)(context->d - context->dest);
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
  nstr->n_characters = (unsigned int)length;
  nstr->normalized_length_in_bytes =
    (unsigned int)(d - (unsigned char *)nstr->normalized);
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
  nstr->n_characters = (unsigned int)length;
  nstr->normalized_length_in_bytes =
    (unsigned int)(d - (unsigned char *)nstr->normalized);
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
  nstr->n_characters = (unsigned int)length;
  nstr->normalized_length_in_bytes =
    (unsigned int)(d - (unsigned char *)nstr->normalized);
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

  options = GRN_CALLOC(sizeof(grn_nfkc_normalize_options));
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
    uint32_t i = grn_string_get_normalizer_index(ctx, string);
    options = grn_table_cache_normalizers_options(ctx,
                                                  table,
                                                  i,
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

  options = GRN_CALLOC(sizeof(grn_nfkc_normalize_options));
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
    uint32_t i = grn_string_get_normalizer_index(ctx, string);
    options = grn_table_cache_normalizers_options(ctx,
                                                  table,
                                                  i,
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

static void *
nfkc130_open_options(grn_ctx *ctx,
                     grn_obj *normalizer,
                     grn_obj *raw_options,
                     void *user_data)
{
  grn_nfkc_normalize_options *options;

  options = GRN_CALLOC(sizeof(grn_nfkc_normalize_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][nfkc130] "
        "failed to allocate memory for options");
    return NULL;
  }

  grn_nfkc130_normalize_options_init(ctx, options);

  grn_nfkc_normalize_options_apply(ctx, options, raw_options);

  return options;
}

static void
nfkc130_close_options(grn_ctx *ctx, void *data)
{
  grn_nfkc_normalize_options *options = data;
  grn_nfkc_normalize_options_fin(ctx, options);
  GRN_FREE(options);
}

static grn_obj *
nfkc130_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_obj *table;
  grn_nfkc_normalize_options *options;
  grn_nfkc_normalize_options options_raw;

  table = grn_string_get_table(ctx, string);
  if (table) {
    uint32_t i = grn_string_get_normalizer_index(ctx, string);
    options = grn_table_cache_normalizers_options(ctx,
                                                  table,
                                                  i,
                                                  nfkc130_open_options,
                                                  nfkc130_close_options,
                                                  NULL);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  } else {
    grn_nfkc130_normalize_options_init(ctx, &options_raw);
    options = &options_raw;
  }

  grn_nfkc_normalize(ctx, string, options);
  if (!table) {
    grn_nfkc_normalize_options_fin(ctx, options);
  }
  return NULL;
}

static void *
nfkc150_open_options(grn_ctx *ctx,
                     grn_obj *normalizer,
                     grn_obj *raw_options,
                     void *user_data)
{
  grn_nfkc_normalize_options *options;

  options = GRN_CALLOC(sizeof(grn_nfkc_normalize_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][nfkc150] "
        "failed to allocate memory for options");
    return NULL;
  }

  grn_nfkc150_normalize_options_init(ctx, options);

  grn_nfkc_normalize_options_apply(ctx, options, raw_options);

  return options;
}

static void
nfkc150_close_options(grn_ctx *ctx, void *data)
{
  grn_nfkc_normalize_options *options = data;
  grn_nfkc_normalize_options_fin(ctx, options);
  GRN_FREE(options);
}

static grn_obj *
nfkc150_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_obj *table;
  grn_nfkc_normalize_options *options;
  grn_nfkc_normalize_options options_raw;

  table = grn_string_get_table(ctx, string);
  if (table) {
    uint32_t i = grn_string_get_normalizer_index(ctx, string);
    options = grn_table_cache_normalizers_options(ctx,
                                                  table,
                                                  i,
                                                  nfkc150_open_options,
                                                  nfkc150_close_options,
                                                  NULL);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  } else {
    grn_nfkc150_normalize_options_init(ctx, &options_raw);
    options = &options_raw;
  }

  grn_nfkc_normalize(ctx, string, options);
  if (!table) {
    grn_nfkc_normalize_options_fin(ctx, options);
  }
  return NULL;
}
#endif /* GRN_WITH_NFKC */

typedef grn_char_type (*grn_get_char_type_func)(const unsigned char *character);

/* This is for general substitute based normalizers. */
typedef struct {
  const char *tag;
  grn_get_char_type_func get_char_type;
  grn_obj normalized;
  grn_obj checks;
  bool need_checks;
  grn_obj types;
  bool need_types;
  size_t current_source_offset;
  grn_obj offsets;
  bool need_offsets;
  size_t n_normalized_characters;
  bool count_n_normalized_characters_in_checks;
  bool count_n_normalized_characters_in_types;
} substitutor_data;

static void
substitutor_data_init(grn_ctx *ctx,
                      substitutor_data *data,
                      const char *tag,
                      grn_get_char_type_func get_char_type,
                      grn_string *string,
                      bool report_source_offset)
{
  data->tag = tag;
  data->get_char_type = get_char_type;
  GRN_TEXT_INIT(&(data->normalized), 0);
  GRN_INT16_INIT(&(data->checks), GRN_OBJ_VECTOR);
  data->need_checks = (string->flags & GRN_STRING_WITH_CHECKS);
  GRN_UINT8_INIT(&(data->types), GRN_OBJ_VECTOR);
  data->need_types =
    (string->flags & GRN_STRING_WITH_TYPES) &&
    (ctx->encoding == GRN_ENC_UTF8);
  data->current_source_offset = 0;
  GRN_UINT64_INIT(&(data->offsets), GRN_OBJ_VECTOR);
  data->need_offsets = report_source_offset;
  data->n_normalized_characters = 0;
  data->count_n_normalized_characters_in_checks = false;
  data->count_n_normalized_characters_in_types = false;
  if (data->need_checks) {
    data->count_n_normalized_characters_in_checks = true;
  } else if (data->need_types) {
    data->count_n_normalized_characters_in_types = true;
  }
}

static void
substitutor_data_fin(grn_ctx *ctx, substitutor_data *data)
{
  GRN_OBJ_FIN(ctx, &(data->normalized));
  GRN_OBJ_FIN(ctx, &(data->checks));
  GRN_OBJ_FIN(ctx, &(data->types));
  GRN_OBJ_FIN(ctx, &(data->offsets));
}

static void
substitutor_add_checks_and_offsets(grn_ctx *ctx,
                                   substitutor_data *data,
                                   const char *source,
                                   size_t source_length,
                                   const char *normalized,
                                   size_t normalized_length)
{
  grn_obj *checks = &(data->checks);
  grn_obj *offsets = &(data->offsets);

  size_t current_source_offset = data->current_source_offset;
  const char *source_current = source;
  const char *previous_source_current = source;
  const char *source_end = source_current + source_length;
  const char *normalized_current = normalized;
  const char *normalized_end = normalized_current + normalized_length;
  while (source_current < source_end &&
         normalized_current < normalized_end) {
    int source_char_length = grn_charlen(ctx, source_current, source_end);
    if (source_char_length == 0) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s invalid character in source text: <%.*s>",
          data->tag,
          (int)(source_end - source_current),
          source_current);
      return;
    }
    int normalized_char_length;
    if (source_current == normalized_current) {
      normalized_char_length = source_char_length;
    } else {
      normalized_char_length = grn_charlen(ctx,
                                           normalized_current,
                                           normalized_end);
      if (normalized_char_length == 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s invalid character in normalized text: <%.*s>",
            data->tag,
            (int)(normalized_end - normalized_current),
            normalized_current);
        return;
      }
    }
    if (data->need_checks) {
      GRN_INT16_PUT(ctx, checks, source_char_length);
      int i;
      for (i = 1; i < normalized_char_length; i++) {
        GRN_INT16_PUT(ctx, checks, 0);
      }
    }
    if (data->need_offsets) {
      GRN_UINT64_PUT(ctx,
                     offsets,
                     (uint64_t)(source_current - source) + current_source_offset);
    }
    previous_source_current = source_current;
    source_current += source_char_length;
    normalized_current += normalized_char_length;
    if (data->count_n_normalized_characters_in_checks) {
      data->n_normalized_characters++;
    }
  }
  if (source_current < source_end) {
    size_t last_check_index = GRN_INT16_VECTOR_SIZE(checks);
    if (last_check_index == 0) {
      return;
    }
    for (last_check_index--; last_check_index > 0; last_check_index--) {
      if (GRN_INT16_VALUE_AT(checks, last_check_index) != 0) {
        break;
      }
    }
    while (source_current < source_end) {
      int source_char_length = grn_charlen(ctx,
                                             source_current,
                                             source_end);
      if (source_char_length == 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s invalid character in source text: <%.*s>",
            data->tag,
            (int)(source_end - source_current),
            source_current);
        return;
      }
      GRN_INT16_VALUE_AT(checks, last_check_index) +=
        (int16_t)source_char_length;
      source_current += source_char_length;
    }
  } else if (normalized_current < normalized_end) {
    while (normalized_current < normalized_end) {
      int normalized_char_length = grn_charlen(ctx,
                                               normalized_current,
                                               normalized_end);
      if (normalized_char_length == 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s invalid character in normalized text: <%.*s>",
            data->tag,
            (int)(normalized_end - normalized_current),
            normalized_current);
        return;
      }
      if (data->need_checks) {
        GRN_INT16_PUT(ctx, checks, -1);
        int i;
        for (i = 1; i < normalized_char_length; i++) {
          GRN_INT16_PUT(ctx, checks, 0);
        }
      }
      if (data->need_offsets) {
        GRN_UINT64_PUT(ctx,
                       offsets,
                       (uint64_t)(previous_source_current - source) +
                       current_source_offset);
      }
      normalized_current += normalized_char_length;
      if (data->count_n_normalized_characters_in_checks) {
        data->n_normalized_characters++;
      }
    }
  }
}

static void
substitutor_add_types(grn_ctx *ctx,
                      substitutor_data *data,
                      const char *normalized,
                      size_t normalized_length)
{
  grn_obj *types = &(data->types);
  const char *normalized_end = normalized + normalized_length;
  while (normalized < normalized_end) {
    int normalized_char_length = grn_charlen(ctx, normalized, normalized_end);
    if (normalized_char_length == 0) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s invalid character in normalized text: <%.*s>",
          data->tag,
          (int)(normalized_end - normalized),
          normalized);
      return;
    }
    grn_char_type char_type = data->get_char_type(normalized);
    GRN_UINT8_PUT(ctx, types, char_type);
    normalized += normalized_char_length;
    if (data->count_n_normalized_characters_in_types) {
      data->n_normalized_characters++;
    }
  }
}

static void
substitutor_normalized(grn_ctx *ctx,
                       substitutor_data *data,
                       const char *source,
                       size_t source_length,
                       const char *normalized,
                       size_t normalized_length)
{
  if (data->need_checks || data->need_offsets) {
    substitutor_add_checks_and_offsets(ctx,
                                    data,
                                    source,
                                    source_length,
                                    normalized,
                                    normalized_length);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  if (data->need_types) {
    substitutor_add_types(ctx, data, normalized, normalized_length);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  data->current_source_offset += source_length;
}

static void
substitutor_finished(grn_ctx *ctx,
                     substitutor_data *data,
                     grn_string *string)
{
  if (!data->count_n_normalized_characters_in_checks &&
      !data->count_n_normalized_characters_in_types) {
    const char *current = GRN_TEXT_VALUE(&(data->normalized));
    const char *end = current + GRN_TEXT_LEN(&(data->normalized));
    while (current < end) {
      int char_length = grn_charlen(ctx, current, end);
      if (char_length == 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s invalid character in normalized text: <%.*s>",
            data->tag,
            (int)(end - current),
            current);
        return;
      }
      data->n_normalized_characters++;
      current += char_length;
    }
  }

  string->n_characters = (unsigned int)(data->n_normalized_characters);
  string->normalized_length_in_bytes =
    (unsigned int)GRN_TEXT_LEN(&(data->normalized));
  string->normalized = grn_bulk_detach(ctx, &(data->normalized));

  if (data->need_checks) {
    string->checks = (int16_t *)grn_bulk_detach(ctx, &(data->checks));
  }

  if (data->need_types) {
    GRN_UINT64_PUT(ctx, &(data->types), GRN_CHAR_NULL);
    string->ctypes = (uint_least8_t *)grn_bulk_detach(ctx, &(data->types));
  }

  if (data->need_offsets) {
    GRN_UINT64_PUT(ctx, &(data->offsets), string->original_length_in_bytes);
    string->offsets = (uint64_t *)grn_bulk_detach(ctx, &(data->offsets));
  }
}

typedef struct {
  grn_obj *target_table;
  grn_obj *target_index_column;
  uint32_t target_section;
  grn_obj *normalized_column;
  grn_get_char_type_func get_char_type;
  bool report_source_offset;
} table_options;

static void
table_options_init(grn_ctx *ctx, table_options *options)
{
  options->target_table = NULL;
  options->target_index_column = NULL;
  options->target_section = 0;
  options->normalized_column = NULL;
  options->get_char_type = grn_nfkc_char_type;
  options->report_source_offset = false;
}

static void
table_options_fin(grn_ctx *ctx, table_options *options)
{
  /* We can't call grn_obj_unlink() safety here. Options are cached
   * and closed when the table that have this option is closed. It may
   * be occurred when DB is closed. In the case,
   * options->target_table, options->target_index_column or
   * options->normalized_column are already closed. If we call
   * grn_obj_unlink() with closed column/table, Groonga may be
   * crashed. */

  if (options->normalized_column) {
    /* grn_obj_unlink(ctx, options->normalized_column); */
    options->normalized_column = NULL;
  }
  if (options->target_index_column) {
    /* grn_obj_unlink(ctx, options->target_index_column); */
    options->target_index_column = NULL;
  }
  if (options->target_table) {
    /* grn_obj_unlink(ctx, options->target_table); */
    options->target_table = NULL;
  }
}

static void
table_options_close(grn_ctx *ctx, void *data)
{
  table_options *options = data;
  table_options_fin(ctx, options);
  GRN_FREE(options);
}

static grn_get_char_type_func
unicode_version_option_process(grn_ctx *ctx,
                               grn_obj *raw_options,
                               unsigned int i,
                               grn_raw_string *name_raw,
                               const char *tag)
{
  const char *version;
  grn_id domain;
  unsigned int version_length = grn_vector_get_element(ctx,
                                                       raw_options,
                                                       i,
                                                       &version,
                                                       NULL,
                                                       &domain);
  if (!grn_type_id_is_text_family(ctx, domain)) {
    grn_obj value;
    GRN_VALUE_FIX_SIZE_INIT(&value, GRN_OBJ_DO_SHALLOW_COPY, domain);
    GRN_TEXT_SET(ctx, &value, version, version_length);
    grn_obj inspected;
    grn_inspect(ctx, &inspected, &value);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must be a text: <%.*s>",
        tag,
        (int)(name_raw->length), name_raw->value,
        (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_OBJ_FIN(ctx, &value);
    return NULL;
  }
  if (version_length == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must not be empty",
        tag,
        (int)(name_raw->length), name_raw->value);
    return NULL;
  }
  grn_raw_string version_raw;
  version_raw.value = version;
  version_raw.length = version_length;
  if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "5.0.0")) {
    return grn_nfkc_char_type;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "10.0.0")) {
    return grn_nfkc100_char_type;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "12.1.0")) {
    return grn_nfkc121_char_type;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "13.0.0")) {
    return grn_nfkc130_char_type;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "15.0.0")) {
    return grn_nfkc150_char_type;
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must be one of \"5.0.0\", \"10.0.0\", \"12.1.0\", "
        "\"13.0.0\" or \"15.0.0\": <%.*s>",
        tag,
        (int)(name_raw->length), name_raw->value,
        (int)(version_raw.length), version_raw.value);
    return NULL;
  }
}

static void *
table_options_open(grn_ctx *ctx,
                   grn_obj *normalizer,
                   grn_obj *raw_options,
                   void *user_data)
{
  const char *tag = "[normalizer][table]";

  table_options *options = GRN_CALLOC(sizeof(table_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options",
        tag);
    return NULL;
  }

  table_options_init(ctx, options);
  grn_obj target_name_buffer;
  GRN_TEXT_INIT(&target_name_buffer, 0);
  grn_obj *target_name = &target_name_buffer;
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "column") ||
        GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "normalized")) {
      if (options->normalized_column) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must not specify "
            "both of <column> (deprecated) and <normalized>",
            tag,
            (int)(name_raw.length), name_raw.value);
        break;
      }
      const char *name;
      grn_id domain;
      unsigned int name_length = grn_vector_get_element(ctx,
                                                        raw_options,
                                                        i,
                                                        &name,
                                                        NULL,
                                                        &domain);
      if (!grn_type_id_is_text_family(ctx, domain)) {
        grn_obj value;
        GRN_VALUE_FIX_SIZE_INIT(&value, GRN_OBJ_DO_SHALLOW_COPY, domain);
        GRN_TEXT_SET(ctx, &value, name, name_length);
        grn_obj inspected;
        grn_inspect(ctx, &inspected, &value);
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must be a text: <%.*s>",
            tag,
            (int)(name_raw.length), name_raw.value,
            (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        GRN_OBJ_FIN(ctx, &value);
        break;
      }
      if (name_length == 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must not be empty",
            tag,
            (int)(name_raw.length), name_raw.value);
        break;
      }
      options->normalized_column = grn_ctx_get(ctx, name, (int)name_length);
      if (!options->normalized_column) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] nonexistent column: <%.*s>",
            tag,
            (int)(name_raw.length), name_raw.value,
            (int)name_length, name);
        break;
      }
      if (!grn_obj_is_text_family_scalar_column(ctx,
                                                options->normalized_column)) {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect_limited(ctx, &inspected, options->normalized_column);
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must be a text family scalar column: <%.*s>: <%.*s>",
            tag,
            (int)(name_raw.length), name_raw.value,
            (int)name_length, name,
            (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        break;
      }
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "target")) {
      const char *name;
      grn_id domain;
      unsigned int name_length = grn_vector_get_element(ctx,
                                                        raw_options,
                                                        i,
                                                        &name,
                                                        NULL,
                                                        &domain);
      if (!grn_type_id_is_text_family(ctx, domain)) {
        grn_obj value;
        GRN_VALUE_FIX_SIZE_INIT(&value, GRN_OBJ_DO_SHALLOW_COPY, domain);
        GRN_TEXT_SET(ctx, &value, name, name_length);
        grn_obj inspected;
        grn_inspect(ctx, &inspected, &value);
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must be a text: <%.*s>",
            tag,
            (int)(name_raw.length), name_raw.value,
            (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        GRN_OBJ_FIN(ctx, &value);
        break;
      }
      if (name_length == 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must not be empty",
            tag,
            (int)(name_raw.length), name_raw.value);
        break;
      }
      GRN_TEXT_SET(ctx, target_name, name, name_length);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unicode_version")) {
      options->get_char_type =
        unicode_version_option_process(ctx,
                                       raw_options,
                                       i,
                                       &name_raw,
                                       tag);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "report_source_offset")) {
      options->report_source_offset =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->report_source_offset);
    }
  } GRN_OPTION_VALUES_EACH_END();
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  if (!options->normalized_column) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[normalized] not specified",
        tag);
    goto exit;
  }

  const char *target_table_option_name = "";
  grn_raw_string raw_target_name;
  GRN_RAW_STRING_SET(raw_target_name, target_name);
  if (GRN_RAW_STRING_EQUAL_CSTRING(raw_target_name, "") ||
      GRN_RAW_STRING_EQUAL_CSTRING(raw_target_name, "_key")) {
    target_table_option_name = "normalized";
    options->target_table =
      grn_ctx_at(ctx, options->normalized_column->header.domain);
  /* TODO: Enable this when we add support for
     "Lexicon.index_column['source_name']" syntax. */
  /*
  } else if (grn_raw_string_have_sub_string_cstring(ctx,
                                                    &raw_target_name,
                                                    ".")) {
    target_table_option_name = "target";
    options->target_index_column =
      grn_ctx_get(ctx,
                  raw_target_name.value,
                  raw_target_name.length);
    if (!grn_obj_is_index_column(ctx, options->target_index_column)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect_limited(ctx, &inspected, options->target_index_column);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[target] must be a column "
          "in the same table as the normalized column or "
          "index column to resolve the table of the normalized column: <%.*s>",
          tag,
          (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }
    if (DB_OBJ(options->target_index_column)->range !=
        options->normalized_column->header.domain) {
      grn_obj inspected_target;
      GRN_TEXT_INIT(&inspected_target, 0);
      grn_inspect_limited(ctx, &inspected_target, options->target_index_column);
      grn_obj inspected_normalized;
      GRN_TEXT_INIT(&inspected_normalized, 0);
      grn_obj *normalized_table =
        grn_ctx_at(ctx, options->normalized_column->header.domain);
      grn_inspect_limited(ctx, &inspected_normalized, normalized_table);
      grn_obj_unref(ctx, normalized_table);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[target] index column must be "
          "for the table of the normalized column: <%.*s>: <%.*s>",
          tag,
          (int)GRN_TEXT_LEN(&inspected_target),
          GRN_TEXT_VALUE(&inspected_target),
          (int)GRN_TEXT_LEN(&inspected_normalized),
          GRN_TEXT_VALUE(&inspected_normalized));
      GRN_OBJ_FIN(ctx, &inspected_normalized);
      GRN_OBJ_FIN(ctx, &inspected_target);
      goto exit;
    }
    options->target_table =
      grn_ctx_at(ctx, options->target_index_column->header.domain);
  */
  } else {
    target_table_option_name = "target";
    grn_obj *normalized_table =
      grn_ctx_at(ctx, options->normalized_column->header.domain);
    grn_obj *target_column = grn_table_column(ctx,
                                              normalized_table,
                                              raw_target_name.value,
                                              (ssize_t)(raw_target_name.length));
    grn_obj_unref(ctx, normalized_table);
    if (!target_column) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%s] nonexistent column: <%.*s>",
          tag,
          target_table_option_name,
          (int)(raw_target_name.length),
          raw_target_name.value);
      goto exit;
    }
    grn_index_datum index_datum;
    unsigned int n_indexes = grn_column_find_index_data(ctx,
                                                        target_column,
                                                        GRN_OP_EQUAL,
                                                        &index_datum,
                                                        1);
    grn_obj_unref(ctx, target_column);
    if (n_indexes == 0) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[target] no equal index: <%.*s>",
          tag,
          (int)(raw_target_name.length),
          raw_target_name.value);
      goto exit;
    }
    options->target_index_column = index_datum.index;
    options->target_section = index_datum.section;
    options->target_table =
      grn_ctx_at(ctx, options->target_index_column->header.domain);
  }

  if (options->target_table->header.type != GRN_TABLE_PAT_KEY) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, options->target_table);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%s] table must be a TABLE_PAT_KEY: <%.*s>",
        tag,
        target_table_option_name,
        (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }
  if (options->target_table->header.domain != GRN_DB_SHORT_TEXT) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, options->target_table);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%s] table's key must be ShortText: <%.*s>",
        tag,
        target_table_option_name,
        (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }

exit :
  GRN_OBJ_FIN(ctx, &target_name_buffer);
  if (ctx->rc != GRN_SUCCESS) {
    table_options_fin(ctx, options);
    table_options_close(ctx, options);
    return NULL;
  }

  return options;
}

typedef struct {
  const char *tag;
  table_options *options;
  substitutor_data substitutor_data;
} table_data;

static void
table_normalize(grn_ctx *ctx, grn_string *string, table_options *options)
{
  table_data data;
  data.tag = "[normalizer][table]";
  data.options = options;
  substitutor_data_init(ctx,
                     &(data.substitutor_data),
                     data.tag,
                     options->get_char_type,
                     string,
                     options->report_source_offset);

  string->n_characters = 0;
  string->normalized = NULL;
  string->normalized_length_in_bytes = 0;
  string->checks = NULL;
  string->ctypes = NULL;
  string->offsets = NULL;

  grn_obj *normalized = &(data.substitutor_data.normalized);
  const char *source = string->original;
  const char *source_end = source + string->original_length_in_bytes;
  while (source < source_end) {
#define MAX_N_HITS 16
    grn_pat_scan_hit hits[MAX_N_HITS];
    const char *rest;
    int i, n_hits;
    unsigned int previous = 0;
    size_t chunk_length;

    n_hits = grn_pat_scan(ctx,
                          (grn_pat *)(options->target_table),
                          source,
                          (unsigned int)(source_end - source),
                          hits,
                          MAX_N_HITS,
                          &rest);
    for (i = 0; i < n_hits; i++) {
      if (hits[i].offset > previous) {
        const char *source_previous = source + previous;
        size_t source_previous_length = hits[i].offset - previous;
        GRN_TEXT_PUT(ctx, normalized, source_previous, source_previous_length);
        substitutor_normalized(ctx,
                               &(data.substitutor_data),
                               source_previous,
                               source_previous_length,
                               source_previous,
                               source_previous_length);
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
      }
      size_t before_offset = GRN_TEXT_LEN(normalized);
      if (options->target_index_column) {
        bool found = false;
        grn_ii *ii = (grn_ii *)(options->target_index_column);
        grn_ii_cursor *cursor = grn_ii_cursor_open(ctx,
                                                   ii,
                                                   hits[i].id,
                                                   GRN_ID_NIL,
                                                   GRN_ID_MAX,
                                                   (int)(ii->n_elements),
                                                   0);
        if (cursor) {
          grn_posting *ii_posting;
          while ((ii_posting = grn_ii_cursor_next(ctx, cursor))) {
            if (options->target_section == 0 ||
                ii_posting->sid == options->target_section) {
              grn_obj_get_value(ctx,
                                options->normalized_column,
                                ii_posting->rid,
                                normalized);
              found = true;
              break;
            }
          }
          grn_ii_cursor_close(ctx, cursor);
        }
        if (!found) {
          GRN_TEXT_PUT(ctx,
                       normalized,
                       source + hits[i].offset,
                       hits[i].length);
        }
      } else {
        grn_obj_get_value(ctx,
                          options->normalized_column,
                          hits[i].id,
                          normalized);
      }
      substitutor_normalized(ctx,
                             &(data.substitutor_data),
                             source + hits[i].offset,
                             hits[i].length,
                             GRN_TEXT_VALUE(normalized) + before_offset,
                             GRN_TEXT_LEN(normalized) - before_offset);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
      previous = hits[i].offset + hits[i].length;
    }

    chunk_length = (size_t)(rest - source);
    if (chunk_length - previous > 0) {
      const char *source_previous = source + previous;
      size_t source_previous_length = (size_t)(source_end - source - previous);
      GRN_TEXT_PUT(ctx, normalized, source_previous, source_previous_length);
      substitutor_normalized(ctx,
                             &(data.substitutor_data),
                             source_previous,
                             source_previous_length,
                             source_previous,
                             source_previous_length);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }
    source = rest;
#undef MAX_N_HITS
  }

  substitutor_finished(ctx, &(data.substitutor_data), string);

exit:
  substitutor_data_fin(ctx, &(data.substitutor_data));
}

static grn_obj *
table_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_obj *table;
  table_options *options;
  table_options options_raw;

  table = grn_string_get_table(ctx, string);
  if (table) {
    uint32_t i = grn_string_get_normalizer_index(ctx, string);
    options = grn_table_cache_normalizers_options(ctx,
                                                  table,
                                                  i,
                                                  table_options_open,
                                                  table_options_close,
                                                  NULL);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  } else {
    table_options_init(ctx, &options_raw);
    if (ctx->rc != GRN_SUCCESS) {
      table_options_fin(ctx, &options_raw);
      return NULL;
    }
    options = &options_raw;
  }

  table_normalize(ctx, (grn_string *)string, options);

  if (!table) {
    table_options_fin(ctx, options);
  }
  return NULL;
}

typedef struct {
  bool remove_tag;
  bool expand_character_reference;
  grn_get_char_type_func get_char_type;
  bool report_source_offset;
} html_options;

static void
html_options_init(grn_ctx *ctx, html_options *options)
{
  options->remove_tag = true;
  options->expand_character_reference = true;
  options->get_char_type = grn_nfkc_char_type;
  options->report_source_offset = false;
}

static void
html_options_fin(grn_ctx *ctx, html_options *options)
{
}

static void
html_options_close(grn_ctx *ctx, void *data)
{
  html_options *options = data;
  html_options_fin(ctx, options);
  GRN_FREE(options);
}

static void *
html_options_open(grn_ctx *ctx,
                  grn_obj *normalizer,
                  grn_obj *raw_options,
                  void *user_data)
{
  const char *tag = "[normalizer][html]";

  html_options *options = GRN_CALLOC(sizeof(html_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options",
        tag);
    return NULL;
  }

  html_options_init(ctx, options);
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_tag")) {
      options->remove_tag =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->remove_tag);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "expand_character_reference")) {
      options->expand_character_reference =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->expand_character_reference);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unicode_version")) {
      options->get_char_type =
        unicode_version_option_process(ctx,
                                       raw_options,
                                       i,
                                       &name_raw,
                                       tag);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "report_source_offset")) {
      options->report_source_offset =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->report_source_offset);
    }
  } GRN_OPTION_VALUES_EACH_END();

  if (ctx->rc != GRN_SUCCESS) {
    html_options_fin(ctx, options);
    html_options_close(ctx, options);
    return NULL;
  }

  return options;
}

static bool
html_expand_numeric_char_ref(grn_ctx *ctx,
                             grn_obj *buffer,
                             uint32_t code_point)
{
  /* Special code points.
   * See also:
   * https://html.spec.whatwg.org/multipage/parsing.html#numeric-character-reference-end-state */
  switch (code_point) {
  case 0x80 :
    code_point = 0x20AC;
    break;
  case 0x82 :
    code_point = 0x201A;
    break;
  case 0x83 :
    code_point = 0x0192;
    break;
  case 0x84 :
    code_point = 0x201E;
    break;
  case 0x85 :
    code_point = 0x2026;
    break;
  case 0x86 :
    code_point = 0x2020;
    break;
  case 0x87 :
    code_point = 0x2021;
    break;
  case 0x88 :
    code_point = 0x02C6;
    break;
  case 0x89 :
    code_point = 0x2030;
    break;
  case 0x8A :
    code_point = 0x0160;
    break;
  case 0x8B :
    code_point = 0x2039;
    break;
  case 0x8C :
    code_point = 0x0152;
    break;
  case 0x8E :
    code_point = 0x017D;
    break;
  case 0x91 :
    code_point = 0x2018;
    break;
  case 0x92 :
    code_point = 0x2019;
    break;
  case 0x93 :
    code_point = 0x201C;
    break;
  case 0x94 :
    code_point = 0x201D;
    break;
  case 0x95 :
    code_point = 0x2022;
    break;
  case 0x96 :
    code_point = 0x2013;
    break;
  case 0x97 :
    code_point = 0x2014;
    break;
  case 0x98 :
    code_point = 0x02DC;
    break;
  case 0x99 :
    code_point = 0x2122;
    break;
  case 0x9A :
    code_point = 0x0161;
    break;
  case 0x9B :
    code_point = 0x203A;
    break;
  case 0x9C :
    code_point = 0x0153;
    break;
  case 0x9E :
    code_point = 0x017E;
    break;
  case 0x9F :
    code_point = 0x0178;
    break;
  default :
    break;
  }
  grn_text_code_point(ctx, buffer, code_point);
  return true;
}

#include "normalizer_html_expand_named_char_ref.c"

typedef enum {
  HTML_STATE_TEXT,
  HTML_STATE_IN_TAG,
  HTML_STATE_IN_CHAR_REF,
} html_state;

static void
html_normalize(grn_ctx *ctx, grn_string *string, html_options *options)
{
  const char *tag = "[normalizer][html]";
  substitutor_data data;
  substitutor_data_init(ctx,
                        &data,
                        tag,
                        options->get_char_type,
                        string,
                        options->report_source_offset);

  string->n_characters = 0;
  string->normalized = NULL;
  string->normalized_length_in_bytes = 0;
  string->checks = NULL;
  string->ctypes = NULL;
  string->offsets = NULL;

  grn_obj *normalized = &(data.normalized);
  const char *current = string->original;
  const char *end = current + string->original_length_in_bytes;

  html_state state = HTML_STATE_TEXT;
  const char *start_tag = NULL;
  const char *start_char_ref = NULL;

  int char_length = 0;
  for (; current < end; current += char_length) {
    char_length = grn_charlen(ctx, current, end);
    if (char_length == 0) {
      /* Set error? */
      break;
    }

    if (char_length == 1) {
      bool processed = false;
      switch (state) {
      case HTML_STATE_IN_TAG:
        if (*current == '>') {
          substitutor_normalized(ctx,
                                 &data,
                                 start_tag,
                                 (size_t)(current - start_tag + 1),
                                 NULL,
                                 0);
          state = HTML_STATE_TEXT;
          start_tag = NULL;
          processed = true;
        }
        break;
      case HTML_STATE_IN_CHAR_REF :
        if (*current == ';') {
          bool valid = false;
          size_t normalized_length_before = GRN_TEXT_LEN(normalized);
          if (start_char_ref[1] == '#') {
            long code_point = -1;
            if (start_char_ref[2] == 'x' || start_char_ref[2] == 'X') {
              /* "&#xH...;" */
              const char *code_point_end = NULL;
              code_point = grn_htoui(start_char_ref + 3,
                                     current,
                                     &code_point_end);
              if (code_point_end != current) {
                code_point = -1;
              }
            } else {
              /* "&#D...;" */
              const char *code_point_end = NULL;
              code_point = grn_atoll(start_char_ref + 2,
                                     current,
                                     &code_point_end);
              if (code_point_end != current) {
                code_point = -1;
              }
            }
            if (code_point >= 0) {
              valid = html_expand_numeric_char_ref(ctx,
                                                   normalized,
                                                   (uint32_t)code_point);
            }
          } else {
            /* "&NAME;" */
            valid =
              html_expand_named_char_ref(ctx,
                                         normalized,
                                         start_char_ref + 1,
                                         (size_t)(current - start_char_ref - 1));
          }
          if (valid) {
            const char *current_normalized =
              GRN_TEXT_VALUE(normalized) + normalized_length_before;
            size_t current_normalized_length =
              GRN_TEXT_LEN(normalized) - normalized_length_before;
            substitutor_normalized(ctx,
                                   &data,
                                   start_char_ref,
                                   (size_t)(current - start_char_ref + 1),
                                   current_normalized,
                                   current_normalized_length);
            if (ctx->rc != GRN_SUCCESS) {
              goto exit;
            }
            state = HTML_STATE_TEXT;
            start_char_ref = NULL;
            processed = true;
          }
        } else if (((current - start_char_ref) == 1 && *current == '#') ||
                   ('A' <= *current && *current <= 'Z') ||
                   ('a' <= *current && *current <= 'z') ||
                   ('0' <= *current && *current <= '9')) {
          processed = true;
        }
      default:
        if (options->remove_tag && *current == '<') {
          state = HTML_STATE_IN_TAG;
          start_tag = current;
          processed = true;
        } else if (options->expand_character_reference && *current == '&') {
          state = HTML_STATE_IN_CHAR_REF;
          start_char_ref = current;
          processed = true;
        }
        break;
      }
      if (processed) {
        continue;
      }
    }

    switch (state) {
    case HTML_STATE_IN_TAG:
      break;
    case HTML_STATE_IN_CHAR_REF:
      {
        /* "&...(any multibyte character)...;" case: Write it as-is. */
        size_t normalized_length =
          (size_t)(current - start_char_ref) + (size_t)char_length;
        grn_bulk_write(ctx,
                       normalized,
                       start_char_ref,
                       normalized_length);
        substitutor_normalized(ctx,
                               &data,
                               start_char_ref,
                               normalized_length,
                               start_char_ref,
                               normalized_length);
        state = HTML_STATE_TEXT;
        start_char_ref = NULL;
      }
    default :
      grn_bulk_write(ctx, normalized, current, (size_t)char_length);
      substitutor_normalized(ctx,
                             &data,
                             current,
                             (size_t)char_length,
                             current,
                             (size_t)char_length);
      break;
    }
  }
  if (state == HTML_STATE_IN_TAG) {
    substitutor_normalized(ctx,
                           &data,
                           start_tag,
                           (size_t)(current - start_tag),
                           NULL,
                           0);
  }

  substitutor_finished(ctx, &data, string);

exit :
  substitutor_data_fin(ctx, &data);
}

static grn_obj *
html_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *string = args[0];
  grn_obj *table;
  html_options *options;
  html_options options_raw;

  table = grn_string_get_table(ctx, string);
  if (table) {
    uint32_t i = grn_string_get_normalizer_index(ctx, string);
    options = grn_table_cache_normalizers_options(ctx,
                                                  table,
                                                  i,
                                                  html_options_open,
                                                  html_options_close,
                                                  NULL);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  } else {
    html_options_init(ctx, &options_raw);
    if (ctx->rc != GRN_SUCCESS) {
      html_options_fin(ctx, &options_raw);
      return NULL;
    }
    options = &options_raw;
  }

  html_normalize(ctx, (grn_string *)string, options);

  if (!table) {
    html_options_fin(ctx, options);
  }
  return NULL;
}

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
  const char *normalizer_nfkc130_name = "NormalizerNFKC130";
  const char *normalizer_nfkc150_name = "NormalizerNFKC150";

  grn_normalizer_register(ctx, GRN_NORMALIZER_AUTO_NAME, -1,
                          NULL, auto_next, NULL);

#ifdef GRN_WITH_NFKC
  grn_normalizer_register(ctx, normalizer_nfkc51_name, -1,
                          NULL, nfkc51_next, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc100_name, -1,
                          NULL, nfkc100_next, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc121_name, -1,
                          NULL, nfkc121_next, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc130_name, -1,
                          NULL, nfkc130_next, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc150_name, -1,
                          NULL, nfkc150_next, NULL);
#else /* GRN_WITH_NFKC */
  grn_normalizer_register(ctx, normalizer_nfkc51_name, -1,
                          NULL, NULL, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc100_name, -1,
                          NULL, NULL, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc121_name, -1,
                          NULL, NULL, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc130_name, -1,
                          NULL, NULL, NULL);
  grn_normalizer_register(ctx, normalizer_nfkc150_name, -1,
                          NULL, NULL, NULL);
#endif /* GRN_WITH_NFKC */

  grn_normalizer_register(ctx, "NormalizerTable", -1,
                          NULL, table_next, NULL);

  grn_normalizer_register(ctx, "NormalizerHTML", -1,
                          NULL, html_next, NULL);

/*
  grn_normalizer_register(ctx, "NormalizerUCA", -1,
                          NULL, uca_next, NULL);
*/

  return GRN_SUCCESS;
}
