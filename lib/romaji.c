/*
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_romaji.h"
#include "grn_str.h"

static grn_inline grn_bool
grn_romaji_hepburn_is_pbm(const unsigned char *utf8,
                          size_t length)
{
  if (length != 3) {
    return GRN_FALSE;
  }

  switch (utf8[0]) {
  case 0xe3 :
    switch (utf8[1]) {
    case 0x81 :
      switch (utf8[2]) {
      case 0xb0 : /* U+3070 HIRAGANA LETTER BA */
      case 0xb1 : /* U+3071 HIRAGANA LETTER PA */
      case 0xb3 : /* U+3073 HIRAGANA LETTER BI */
      case 0xb4 : /* U+3074 HIRAGANA LETTER PI */
      case 0xb6 : /* U+3076 HIRAGANA LETTER BU */
      case 0xb7 : /* U+3077 HIRAGANA LETTER PU */
      case 0xb9 : /* U+3079 HIRAGANA LETTER BE */
      case 0xba : /* U+307A HIRAGANA LETTER PE */
        return GRN_TRUE;
      default :
        /* U+3079 HIRAGANA LETTER BO ..
         * U+307F HIRAGANA LETTER MI */
        return utf8[2] >= 0xbc;
      }
    case 0x82 :
      /* U+3080 HIRAGANA LETTER MU ..
       * U+3082 HIRAGANA LETTER MO */
      return (0x80 <= utf8[2] && utf8[2] <= 0x82);
    case 0x83 :
      switch (utf8[2]) {
      case 0x90 : /* U+30D0 KATAKANA LETTER BA */
      case 0x91 : /* U+30D1 KATAKANA LETTER PA */
      case 0x93 : /* U+30D3 KATAKANA LETTER BI */
      case 0x94 : /* U+30D4 KATAKANA LETTER PI */
      case 0x96 : /* U+30D6 KATAKANA LETTER BU */
      case 0x97 : /* U+30D7 KATAKANA LETTER PU */
      case 0x99 : /* U+30D9 KATAKANA LETTER BE */
      case 0x9a : /* U+30DA KATAKANA LETTER PE */
        return GRN_TRUE;
      default :
        /* U+30DC KATAKANA LETTER BO ..
         * U+30E2 KATAKANA LETTER MO */
        return (0x9c <= utf8[2] && utf8[2] <= 0xa2);
      }
    default :
      return GRN_FALSE;
    }
  default :
    return GRN_FALSE;
  }
}

static grn_inline grn_bool
grn_romaji_hepburn_is_aiueoy(const unsigned char *utf8,
                             size_t length)
{
  if (length != 3) {
    return GRN_FALSE;
  }

  switch (utf8[0]) {
  case 0xe3 :
    switch (utf8[1]) {
    case 0x81 :
      switch (utf8[2]) {
      case 0x82 : /* U+3042 HIRAGANA LETTER A */
      case 0x84 : /* U+3044 HIRAGANA LETTER I */
      case 0x86 : /* U+3046 HIRAGANA LETTER U */
      case 0x88 : /* U+3048 HIRAGANA LETTER E */
      case 0x8a : /* U+304A HIRAGANA LETTER O */
        return GRN_TRUE;
      default :
        return GRN_FALSE;
      }
    case 0x82 :
      switch (utf8[2]) {
      case 0x84 : /* U+3084 HIRAGANA LETTER YA */
      case 0x86 : /* U+3086 HIRAGANA LETTER YU */
      case 0x88 : /* U+3088 HIRAGANA LETTER YO */
      case 0xa2 : /* U+30A2 KATAKANA LETTER A */
      case 0xa4 : /* U+30A4 KATAKANA LETTER I */
      case 0xa6 : /* U+30A6 KATAKANA LETTER U */
      case 0xa8 : /* U+30A8 KATAKANA LETTER E */
      case 0xaa : /* U+30AA KATAKANA LETTER O */
        return GRN_TRUE;
      default :
        return GRN_FALSE;
      }
    case 0x83 :
      switch (utf8[2]) {
      case 0xa4 : /* U+30E4 KATAKANA LETTER YA */
      case 0xa6 : /* U+30E6 KATAKANA LETTER YU */
      case 0xa8 : /* U+30E8 KATAKANA LETTER YO */
        return GRN_TRUE;
      default :
        return GRN_FALSE;
      }
    default :
      return GRN_FALSE;
    }
  default :
    return GRN_FALSE;
  }
}

static grn_inline unsigned char
grn_romaji_hepburn_consonant(grn_ctx *ctx,
                             const unsigned char *current,
                             size_t char_length,
                             const unsigned char *end)
{
  if (char_length != 3) {
    return '\0';
  }

  switch (current[0]) {
  case 0xe3 :
    switch (current[1]) {
    case 0x81 :
      if (0x81 <= current[2] && current[2] <= 0x8a) {
        /* U+3042 HIRAGANA LETTER SMALL A ..
         * U+304A HIRAGANA LETTER O */
        if ((current[2] % 2) == 1) { /* SMALL */
          return 'x';
        }
      } else if (0x8b <= current[2] && current[2] <= 0x94) {
        /* U+304B HIRAGANA LETTER KA ..
         * U+3054 HIRAGANA LETTER GO */
        const char *gk = "gk";
        return (unsigned char)(gk[current[2] % 2]);
      } else if (0x95 <= current[2] && current[2] <= 0x9e) {
        /* U+3055 HIRAGANA LETTER SA ..
         * U+305E HIRAGANA LETTER ZO */
        if (current[2] == 0x97) {
          /* U+3057 HIRAGANA LETTER SI */
          return 's';
        } else if (current[2] == 0x98) {
          /* U+3058 HIRAGANA LETTER ZI */
          return 'j';
        } else {
          const char *zs = "zs";
          return (unsigned char)(zs[current[2] % 2]);
        }
      } else if (0x9f <= current[2] && current[2] <= 0xa9) {
        /* U+305F HIRAGANA LETTER TA ..
         * U+3069 HIRAGANA LETTER DO */
        const char *tdtjxtztdtd = "tdtjxtztdtd";
        return (unsigned char)(tdtjxtztdtd[current[2] - 0x9f]);
      } else if (0xaa <= current[2] && current[2] <= 0xae) {
        /* U+306A HIRAGANA LETTER NA ..
         * U+306E HIRAGANA LETTER NO */
        return 'n';
      } else if (0xaf <= current[2] && current[2] <= 0xbd) {
        /* U+306F HIRAGANA LETTER HA ..
         * U+307D HIRAGANA LETTER PO */
        const char *phb = "phb";
        return (unsigned char)(phb[current[2] % 3]);
      } else if (0xbe <= current[2] && current[2] <= 0xbf) {
        /* U+307E HIRAGANA LETTER MA ..
         * U+307F HIRAGANA LETTER MI */
        return 'm';
      }
      break;
    case 0x82 :
      if (0x80 <= current[2] && current[2] <= 0x82) {
        /* U+3080 HIRAGANA LETTER MU ..
         * U+3082 HIRAGANA LETTER MO */
        return 'm';
      } else if (0x83 <= current[2] && current[2] <= 0x88) {
        /* U+3083 HIRAGANA LETTER SMALL YA ..
         * U+3088 HIRAGANA LETTER YO */
        if ((current[2] % 2) == 1) { /* SMALL */
          return 'x';
        } else {
          return 'y';
        }
      } else if (0x89 <= current[2] && current[2] <= 0x8d) {
        /* U+3089 HIRAGANA LETTER RA ..
         * U+308D HIRAGANA LETTER RO */
        return 'r';
      } else if (0x8e <= current[2] && current[2] <= 0x92) {
        /* U+308E HIRAGANA LETTER SMALL WA ..
         * U+3092 HIRAGANA LETTER WO */
        if (current[2] == 0x8e) { /* SMALL */
          return 'x';
        } else {
          return 'w';
        }
      } else if (current[2] == 0x93) {
        /* U+3093 HIRAGANA LETTER N */
        const unsigned char *next = current + char_length;
        size_t next_char_length =
          (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
        if (grn_romaji_hepburn_is_pbm(next, next_char_length)) {
          return 'm';
        } else {
          return 'n';
        }
      } else if (current[2] == 0x94) {
        /* U+3094 HIRAGANA LETTER VU */
        return 'v';
      } else if (current[2] == 0x95) {
        /* U+3095 HIRAGANA LETTER SMALL KA */
        return 'x';
      } else if (current[2] == 0x96) {
        /* U+3096 HIRAGANA LETTER SMALL KE */
        return 'x';
      } else if (0xa1 <= current[2] && current[2] <= 0xaa) {
        /* U+30A1 KATAKANA LETTER SMALL A ..
         * U+30AA KATAKANA LETTER O */
        if ((current[2] % 2) == 1) { /* SMALL */
          return 'x';
        }
      } else if (0xab <= current[2] && current[2] <= 0xb4) {
        /* U+30AB KATAKANA LETTER KA ..
         * U+30B4 KATAKANA LETTER GO */
        const char *gk = "gk";
        return (unsigned char)(gk[current[2] % 2]);
      } else if (0xb5 <= current[2] && current[2] <= 0xbe) {
        /* U+30B5 KATAKANA LETTER SA ..
         * U+30BE KATAKANA LETTER ZO */
        if (current[2] == 0xb7) {
          /* U+30B7 KATAKANA LETTER SI */
          return 's';
        } else if (current[2] == 0x98) {
          /* U+30B8 KATAKANA LETTER ZI */
          return 'j';
        } else {
          const char *zs = "zs";
          return (unsigned char)(zs[current[2] % 2]);
        }
      } else if (current[2] == 0xbf) {
        /* U+30BF KATAKANA LETTER TA */
        return 't';
      }
      break;
    case 0x83 :
      if (0x80 <= current[2] && current[2] <= 0x89) {
        /* U+30C0 KATAKANA LETTER DA ..
         * U+30C9 KATAKANA LETTER DO */
        const char *dtjxtztdtd = "dtjxtztdtd";
        return (unsigned char)(dtjxtztdtd[current[2] - 0x80]);
      } else if (0x8a <= current[2] && current[2] <= 0x8e) {
        /* U+30CA KATAKANA LETTER NA ..
         * U+30CE KATAKANA LETTER NO */
        return 'n';
      } else if (0x8f <= current[2] && current[2] <= 0x9d) {
        /* U+30CF KATAKANA LETTER HA ..
         * U+30DD KATAKANA LETTER PO */
        const char *bph = "bph";
        return (unsigned char)(bph[current[2] % 3]);
      } else if (0x9e <= current[2] && current[2] <= 0xa2) {
        /* U+30DE KATAKANA LETTER MA ..
         * U+30E2 KATAKANA LETTER MO */
        return 'm';
      } else if (0xa3 <= current[2] && current[2] <= 0xa8) {
        /* U+30E3 KATAKANA LETTER SMALL YA ..
         * U+30E8 KATAKANA LETTER YO */
        if ((current[2] % 2) == 1) { /* SMALL */
          return 'x';
        } else {
          return 'y';
        }
      } else if (0xa9 <= current[2] && current[2] <= 0xad) {
        /* U+30E9 KATAKANA LETTER RA ..
         * U+30ED KATAKANA LETTER RO */
        return 'r';
      } else if (0xae <= current[2] && current[2] <= 0xb2) {
        /* U+30EE KATAKANA LETTER SMALL WA ..
         * U+30F2 KATAKANA LETTER WO */
        if (current[2] == 0xae) { /* SMALL */
          return 'x';
        } else {
          return 'w';
        }
      } else if (current[2] == 0xb3) {
        /* U+30F3 KATAKANA LETTER N */
        const unsigned char *next = current + char_length;
        size_t next_char_length =
          (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
        if (grn_romaji_hepburn_is_pbm(next, next_char_length)) {
          return 'm';
        } else {
          return 'n';
        }
      } else if (current[2] == 0xb4) {
        /* U+30F4 KATAKANA LETTER VU */
        return 'v';
      } else if (current[2] == 0xb5) {
        /* U+30F5 KATAKANA LETTER SMALL KA */
        return 'x';
      } else if (current[2] == 0xb6) {
        /* U+30F6 KATAKANA LETTER SMALL KE */
        return 'x';
      } else if (0xb7 <= current[2] && current[2] <= 0xba) {
        /* U+30F7 KATAKANA LETTER VA ..
         * U+30FA KATAKANA LETTER VO */
        return 'v';
      }
      break;
    default :
      break;
    }
  }

  return '\0';
}

const unsigned char *
grn_romaji_hepburn_convert(grn_ctx *ctx,
                           const unsigned char *current,
                           const unsigned char *end,
                           size_t *n_used_bytes,
                           size_t *n_used_characters,
                           unsigned char *buffer,
                           size_t *n_bytes,
                           size_t *n_characters)
{
  size_t char_length;
  const unsigned char *next = NULL;
  size_t next_char_length = 0;
  grn_bool next_small_y = GRN_FALSE;
  unsigned char next_small_yayuyo = '\0';
  const char aiueo[] = "aiueo";
  const char auo[] = "auo";
  const char aaieo[] = "aaieo";

  char_length = (size_t)grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (char_length == 3) {
    next = current + char_length;
    next_char_length = (size_t)grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3) {
      switch (next[0]) {
      case 0xe3 :
        switch (next[1]) {
        case 0x82 :
          switch (next[2]) {
          case 0x83 : /* U+3083 HIRAGANA LETTER SMALL YA */
          case 0x85 : /* U+3085 HIRAGANA LETTER SMALL YU */
          case 0x87 : /* U+3087 HIRAGANA LETTER SMALL YO */
            next_small_y = GRN_TRUE;
            next_small_yayuyo = (unsigned char)(aiueo[(next[2] - 1) % 5]);
            break;
          default :
            break;
          }
        case 0x83 :
          switch (next[2]) {
          case 0xa3 : /* U+30E3 KATAKANA LETTER SMALL YA */
          case 0xa5 : /* U+30E5 KATAKANA LETTER SMALL YU */
          case 0xa7 : /* U+30E7 KATAKANA LETTER SMALL YO */
            next_small_y = GRN_TRUE;
            next_small_yayuyo = (unsigned char)(aiueo[(next[2] - 3) % 5]);
            break;
          default :
            break;
          }
        default :
          break;
        }
      default :
        break;
      }
    }
  }

  switch (current[0]) {
  case 0xe3 :
    switch (current[1]) {
    case 0x81 :
      if (0x81 <= current[2] && current[2] <= 0x8a) {
        /* U+3042 HIRAGANA LETTER SMALL A ..
         * U+304A HIRAGANA LETTER O */
        if ((current[2] % 2) == 1) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0x81) / 2]);
      } else if (0x8b <= current[2] && current[2] <= 0x94) {
        /* U+304B HIRAGANA LETTER KA ..
         * U+3054 HIRAGANA LETTER GO */
        const char *gk = "gk";
        buffer[(*n_bytes)++] = (unsigned char)(gk[current[2] % 2]);
        if (next_small_y &&
            (current[2] == 0x8d ||  /* U+304D HIRAGANA LETTER KI */
             current[2] == 0x8e)) { /* U+304E HIRAGANA LETTER GI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0x8b) / 2]);
        }
      } else if (0x95 <= current[2] && current[2] <= 0x9e) {
        /* U+3055 HIRAGANA LETTER SA ..
         * U+305E HIRAGANA LETTER ZO */
        if (next_small_y && current[2] == 0x97) {
          /* U+3057 HIRAGANA LETTER SI */
          buffer[(*n_bytes)++] = 's';
          buffer[(*n_bytes)++] = 'h';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else if (next_small_y && current[2] == 0x98) {
          /* U+3058 HIRAGANA LETTER ZI */
          buffer[(*n_bytes)++] = 'j';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          if (current[2] == 0x97) {
            /* U+3057 HIRAGANA LETTER SI */
            buffer[(*n_bytes)++] = 's';
            buffer[(*n_bytes)++] = 'h';
          } else if (current[2] == 0x98) {
            /* U+3058 HIRAGANA LETTER ZI */
            buffer[(*n_bytes)++] = 'j';
          } else {
            const char *zs = "zs";
            buffer[(*n_bytes)++] = (unsigned char)(zs[current[2] % 2]);
          }
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0x95) / 2]);
        }
      } else if (0x9f <= current[2] && current[2] <= 0xa9) {
        /* U+305F HIRAGANA LETTER TA ..
         * U+3069 HIRAGANA LETTER DO */
        if (next_small_y && current[2] == 0xa1) {
          /* U+3061 HIRAGANA LETTER TI */
          buffer[(*n_bytes)++] = 'c';
          buffer[(*n_bytes)++] = 'h';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else if (next_small_y && current[2] == 0xa2) {
          /* U+3062 HIRAGANA LETTER DI */
          buffer[(*n_bytes)++] = 'j';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else if (current[2] == 0xa3) {
          /* U+3063 HIRAGANA LETTER SMALL TU */
          const unsigned char next_consonant =
            grn_romaji_hepburn_consonant(ctx, next, next_char_length, end);
          if (next_consonant == '\0') {
            buffer[(*n_bytes)++] = 'x';
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else {
            buffer[(*n_bytes)++] = next_consonant;
          }
        } else {
          const char *aaiiuuueeoo = "aaiiuuueeoo";
          if (current[2] == 0xa1) {
            /* U+3061 HIRAGANA LETTER TI */
            buffer[(*n_bytes)++] = 'c';
            buffer[(*n_bytes)++] = 'h';
          } else if (current[2] == 0xa2) {
            /* U+3062 HIRAGANA LETTER DI */
            buffer[(*n_bytes)++] = 'j';
          } else if (current[2] == 0xa4) {
            /* U+3064 HIRAGANA LETTER TU */
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else {
            const char *td____ztdtd = "td____ztdtd";
            buffer[(*n_bytes)++] =
              (unsigned char)(td____ztdtd[current[2] - 0x9f]);
          }
          buffer[(*n_bytes)++] =
            (unsigned char)(aaiiuuueeoo[current[2] - 0x9f]);
        }
      } else if (0xaa <= current[2] && current[2] <= 0xae) {
        /* U+306A HIRAGANA LETTER NA ..
         * U+306E HIRAGANA LETTER NO */
        buffer[(*n_bytes)++] = 'n';
        if (next_small_y && current[2] == 0xab) {
          /* U+306B HIRAGANA LETTER NI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0xaa)]);
        }
      } else if (0xaf <= current[2] && current[2] <= 0xbd) {
        /* U+306F HIRAGANA LETTER HA ..
         * U+307D HIRAGANA LETTER PO */
        const char *phb = "phb";
        buffer[(*n_bytes)++] = (unsigned char)(phb[current[2] % 3]);
        if (next_small_y &&
            (0xb2 <= current[2] &&  /* U+3072 HIRAGANA LETTER HI .. */
             current[2] <= 0xb4)) { /* U+3074 HIRAGANA LETTER PI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0xaf) / 3]);
        }
      } else if (0xbe <= current[2] && current[2] <= 0xbf) {
        /* U+307E HIRAGANA LETTER MA ..
         * U+307F HIRAGANA LETTER MI */
        buffer[(*n_bytes)++] = 'm';
        if (next_small_y && current[2] == 0xbf) {
          /* U+307F HIRAGANA LETTER MI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0xbe)]);
        }
      }
      break;
    case 0x82 :
      if (0x80 <= current[2] && current[2] <= 0x82) {
        /* U+3080 HIRAGANA LETTER MU ..
         * U+3082 HIRAGANA LETTER MO */
        buffer[(*n_bytes)++] = 'm';
        if (next_small_y && current[2] == 0xbf) {
          /* U+307F HIRAGANA LETTER MI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0x80) + 2]);
        }
      } else if (0x83 <= current[2] && current[2] <= 0x88) {
        /* U+3083 HIRAGANA LETTER SMALL YA ..
         * U+3088 HIRAGANA LETTER YO */
        if ((current[2] % 2) == 1) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'y';
        buffer[(*n_bytes)++] = (unsigned char)(auo[(current[2] - 0x83) / 2]);
      } else if (0x89 <= current[2] && current[2] <= 0x8d) {
        /* U+3089 HIRAGANA LETTER RA ..
         * U+308D HIRAGANA LETTER RO */
        buffer[(*n_bytes)++] = 'r';
        if (next_small_y && current[2] == 0x8a) {
          /* U+308A HIRAGANA LETTER RI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[current[2] - 0x89]);
        }
      } else if (0x8e <= current[2] && current[2] <= 0x92) {
        /* U+308E HIRAGANA LETTER SMALL WA ..
         * U+3092 HIRAGANA LETTER WO */
        if (current[2] == 0x8e) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'w';
        buffer[(*n_bytes)++] = (unsigned char)(aaieo[current[2] - 0x8e]);
      } else if (current[2] == 0x93) {
        /* U+3093 HIRAGANA LETTER N */
        if (grn_romaji_hepburn_is_pbm(next, next_char_length)) {
          buffer[(*n_bytes)++] = 'm';
        } else {
          buffer[(*n_bytes)++] = 'n';
        }
        if (grn_romaji_hepburn_is_aiueoy(next, next_char_length)) {
          buffer[(*n_bytes)++] = '-';
        }
      } else if (current[2] == 0x94) {
        /* U+3094 HIRAGANA LETTER VU */
        buffer[(*n_bytes)++] = 'v';
        buffer[(*n_bytes)++] = 'u';
      } else if (current[2] == 0x95) {
        /* U+3095 HIRAGANA LETTER SMALL KA */
        buffer[(*n_bytes)++] = 'x';
        buffer[(*n_bytes)++] = 'k';
        buffer[(*n_bytes)++] = 'a';
      } else if (current[2] == 0x96) {
        /* U+3096 HIRAGANA LETTER SMALL KE */
        buffer[(*n_bytes)++] = 'x';
        buffer[(*n_bytes)++] = 'k';
        buffer[(*n_bytes)++] = 'e';
      } else if (0xa1 <= current[2] && current[2] <= 0xaa) {
        /* U+30A1 KATAKANA LETTER SMALL A ..
         * U+30AA KATAKANA LETTER O */
        if ((current[2] % 2) == 1) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0xa1) / 2]);
      } else if (0xab <= current[2] && current[2] <= 0xb4) {
        /* U+30AB KATAKANA LETTER KA ..
         * U+30B4 KATAKANA LETTER GO */
        const char *gk = "gk";
        buffer[(*n_bytes)++] = (unsigned char)(gk[current[2] % 2]);
        if (next_small_y &&
            (current[2] == 0xad ||  /* U+30AD KATAKANA LETTER KI */
             current[2] == 0xae)) { /* U+U+30AE KATAKANA LETTER GI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0xab) / 2]);
        }
      } else if (0xb5 <= current[2] && current[2] <= 0xbe) {
        /* U+30B5 KATAKANA LETTER SA ..
         * U+30BE KATAKANA LETTER ZO */
        if (next_small_y && current[2] == 0xb7) {
          /* U+30B7 KATAKANA LETTER SI */
          buffer[(*n_bytes)++] = 's';
          buffer[(*n_bytes)++] = 'h';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else if (next_small_y && current[2] == 0xb8) {
          /* U+30B8 KATAKANA LETTER ZI */
          buffer[(*n_bytes)++] = 'j';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          if (current[2] == 0xb7) {
            /* U+30B7 KATAKANA LETTER SI */
            buffer[(*n_bytes)++] = 's';
            buffer[(*n_bytes)++] = 'h';
          } else if (current[2] == 0xb8) {
            /* U+30B8 KATAKANA LETTER ZI */
            buffer[(*n_bytes)++] = 'j';
          } else {
            const char *zs = "zs";
            buffer[(*n_bytes)++] = (unsigned char)(zs[current[2] % 2]);
          }
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0xb5) / 2]);
        }
      } else if (current[2] == 0xbf) {
        /* U+30BF KATAKANA LETTER TA */
        buffer[(*n_bytes)++] = 't';
        buffer[(*n_bytes)++] = 'a';
      }
      break;
    case 0x83 :
      if (0x80 <= current[2] && current[2] <= 0x89) {
        /* U+30C0 KATAKANA LETTER DA ..
         * U+30C9 KATAKANA LETTER DO */
        if (next_small_y && current[2] == 0x81) {
          /* U+30C1 KATAKANA LETTER TI */
          buffer[(*n_bytes)++] = 'c';
          buffer[(*n_bytes)++] = 'h';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else if (next_small_y && current[2] == 0x82) {
          /* U+30C2 KATAKANA LETTER DI */
          buffer[(*n_bytes)++] = 'j';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else if (current[2] == 0x83) {
          /* U+30C3 KATAKANA LETTER SMALL TU */
          const unsigned char next_consonant =
            grn_romaji_hepburn_consonant(ctx, next, next_char_length, end);
          if (next_consonant == '\0') {
            buffer[(*n_bytes)++] = 'x';
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else {
            buffer[(*n_bytes)++] = next_consonant;
          }
        } else {
          const char *aiiuuueeoo = "aiiuuueeoo";
          if (current[2] == 0x81) {
            /* U+30C1 KATAKANA LETTER TI */
            buffer[(*n_bytes)++] = 'c';
            buffer[(*n_bytes)++] = 'h';
          } else if (current[2] == 0x82) {
            /* U+30C2 KATAKANA LETTER DI */
            buffer[(*n_bytes)++] = 'j';
          } else if (current[2] == 0x83) {
            /* U+30C3 KATAKANA LETTER SMALL TU */
            buffer[(*n_bytes)++] = 'x';
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else if (current[2] == 0x84) {
            /* U+30C4 KATAKANA LETTER TU */
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else {
            const char *d____ztdtd = "d____ztdtd";
            buffer[(*n_bytes)++] =
              (unsigned char)(d____ztdtd[current[2] - 0x80]);
          }
          buffer[(*n_bytes)++] =
            (unsigned char)(aiiuuueeoo[current[2] - 0x80]);
        }
      } else if (0x8a <= current[2] && current[2] <= 0x8e) {
        /* U+30CA KATAKANA LETTER NA ..
         * U+30CE KATAKANA LETTER NO */
        buffer[(*n_bytes)++] = 'n';
        if (next_small_y && current[2] == 0x8b) {
          /* U+30CB KATAKANA LETTER NI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] =
            (unsigned char)(aiueo[current[2] - 0x8a]);
        }
      } else if (0x8f <= current[2] && current[2] <= 0x9d) {
        /* U+30CF KATAKANA LETTER HA ..
         * U+30DD KATAKANA LETTER PO */
        const char *bph = "bph";
        buffer[(*n_bytes)++] = (unsigned char)(bph[current[2] % 3]);
        if (next_small_y &&
            (0x92 <= current[2] &&  /* U+30D2 KATAKANA LETTER HI .. */
             current[2] <= 0x94)) { /* U+30D4 KATAKANA LETTER PI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[(current[2] - 0x8f) / 3]);
        }
      } else if (0x9e <= current[2] && current[2] <= 0xa2) {
        /* U+30DE KATAKANA LETTER MA ..
         * U+30E2 KATAKANA LETTER MO */
        buffer[(*n_bytes)++] = 'm';
        if (next_small_y && current[2] == 0x9f) {
          /* U+30DF KATAKANA LETTER MI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[current[2] - 0x9e]);
        }
      } else if (0xa3 <= current[2] && current[2] <= 0xa8) {
        /* U+30E3 KATAKANA LETTER SMALL YA ..
         * U+30E8 KATAKANA LETTER YO */
        if ((current[2] % 2) == 1) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'y';
        buffer[(*n_bytes)++] = (unsigned char)(auo[(current[2] - 0xa3) / 2]);
      } else if (0xa9 <= current[2] && current[2] <= 0xad) {
        /* U+30E9 KATAKANA LETTER RA ..
         * U+30ED KATAKANA LETTER RO */
        buffer[(*n_bytes)++] = 'r';
        if (next_small_y && current[2] == 0xaa) {
          /* U+30EA KATAKANA LETTER RI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = (unsigned char)(aiueo[current[2] - 0xa9]);
        }
      } else if (0xae <= current[2] && current[2] <= 0xb2) {
        /* U+30EE KATAKANA LETTER SMALL WA ..
         * U+30F2 KATAKANA LETTER WO */
        if (current[2] == 0xae) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'w';
        buffer[(*n_bytes)++] = (unsigned char)(aaieo[current[2] - 0xae]);
      } else if (current[2] == 0xb3) {
        /* U+30F3 KATAKANA LETTER N */
        if (grn_romaji_hepburn_is_pbm(next, next_char_length)) {
          buffer[(*n_bytes)++] = 'm';
        } else {
          buffer[(*n_bytes)++] = 'n';
        }
        if (grn_romaji_hepburn_is_aiueoy(next, next_char_length)) {
          buffer[(*n_bytes)++] = '-';
        }
      } else if (current[2] == 0xb4) {
        /* U+30F4 KATAKANA LETTER VU */
        buffer[(*n_bytes)++] = 'v';
        buffer[(*n_bytes)++] = 'u';
      } else if (current[2] == 0xb5) {
        /* U+30F5 KATAKANA LETTER SMALL KA */
        buffer[(*n_bytes)++] = 'x';
        buffer[(*n_bytes)++] = 'k';
        buffer[(*n_bytes)++] = 'a';
      } else if (current[2] == 0xb6) {
        /* U+30F6 KATAKANA LETTER SMALL KE */
        buffer[(*n_bytes)++] = 'x';
        buffer[(*n_bytes)++] = 'k';
        buffer[(*n_bytes)++] = 'e';
      } else if (0xb7 <= current[2] && current[2] <= 0xba) {
        /* U+30F7 KATAKANA LETTER VA ..
         * U+30FA KATAKANA LETTER VO */
        static char aieo[] = "aieo";
        buffer[(*n_bytes)++] = 'v';
        buffer[(*n_bytes)++] = (unsigned char)(aieo[current[2] - 0xb7]);
      }
      break;
    default :
      break;
    }
    break;
  default :
    break;
  }

  if (*n_bytes > 0) {
    *n_characters = *n_bytes;
    return buffer;
  } else {
    *n_bytes = *n_used_bytes;
    *n_characters = *n_used_characters;
    return current;
  }
}
