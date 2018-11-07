/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

const unsigned char *
grn_romaji_convert_hepburn(grn_ctx *ctx,
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
  char next_small_yayuyo = '\0';
  grn_bool next_pbm = GRN_FALSE;
  grn_bool next_aiueoy = GRN_FALSE;
  char next_consonant = '\0';
  const char aiueo[] = "aiueo";
  const char auo[] = "auo";
  const char aaieo[] = "aaieo";

  char_length = grn_charlen_(ctx, current, end, GRN_ENC_UTF8);
  *n_used_bytes = char_length;
  *n_used_characters = 1;

  if (char_length == 3) {
    next = current + char_length;
    next_char_length = grn_charlen_(ctx, next, end, GRN_ENC_UTF8);
    if (next_char_length == 3) {
      if (next[0] == 0xe3 &&
          next[1] == 0x82 &&
          (next[2] == 0x83 || /* U+3083 HIRAGANA LETTER SMALL YA */
           next[2] == 0x85 || /* U+3085 HIRAGANA LETTER SMALL YU */
           next[2] == 0x87)) { /* U+3087 HIRAGANA LETTER SMALL YO */
        next_small_y = GRN_TRUE;
        next_small_yayuyo = aiueo[(next[2] - 1) % 5];
      } else if (next[0] == 0xe3 &&
                 next[1] == 0x83 &&
                 (next[2] == 0xa3 || /* U+30E3 KATAKANA LETTER SMALL YA */
                  next[2] == 0xa5 || /* U+30E5 KATAKANA LETTER SMALL YU */
                  next[2] == 0xa7)) { /* U+30E7 KATAKANA LETTER SMALL YO */
        next_small_y = GRN_TRUE;
        next_small_yayuyo = aiueo[(next[2] - 3) % 5];
      } else if (next[0] == 0xe3 &&
                 ((next[1] == 0x81 &&
                   (next[2] == 0xb0 || /* U+3070 HIRAGANA LETTER BA */
                    next[2] == 0xb1 || /* U+3071 HIRAGANA LETTER PA */
                    next[2] == 0xb3 || /* U+3073 HIRAGANA LETTER BI */
                    next[2] == 0xb4 || /* U+3074 HIRAGANA LETTER PI */
                    next[2] == 0xb6 || /* U+3076 HIRAGANA LETTER BU */
                    next[2] == 0xb7 || /* U+3077 HIRAGANA LETTER PU */
                    next[2] == 0xb9 || /* U+3079 HIRAGANA LETTER BE */
                    next[2] == 0xba || /* U+307A HIRAGANA LETTER PE */
                    /* U+3079 HIRAGANA LETTER BO ..
                     * U+307F HIRAGANA LETTER MI */
                    0xbc <= next[2])) ||
                  (next[1] == 0x82 &&
                  /* U+3080 HIRAGANA LETTER MU ..
                   * U+3082 HIRAGANA LETTER MO */
                   (0x80 <= next[2] && next[2] <= 0x82)))) {
        next_pbm = GRN_TRUE;
      } else if (next[0] == 0xe3 &&
                 next[1] == 0x83 &&
                 (next[2] == 0x90 || /* U+30D0 KATAKANA LETTER BA */
                   next[2] == 0x91 || /* U+30D1 KATAKANA LETTER PA */
                   next[2] == 0x93 || /* U+30D3 KATAKANA LETTER BI */
                   next[2] == 0x94 || /* U+30D4 KATAKANA LETTER PI */
                   next[2] == 0x96 || /* U+30D6 KATAKANA LETTER BU */
                   next[2] == 0x97 || /* U+30D7 KATAKANA LETTER PU */
                   next[2] == 0x99 || /* U+30D9 KATAKANA LETTER BE */
                   next[2] == 0x9a || /* U+30DA KATAKANA LETTER PE */
                  /* U+30DC KATAKANA LETTER BO ..
                   * U+30E2 KATAKANA LETTER MO */
                  (0x9c <= next[2] && next[2] <= 0xa2))) {
        next_pbm = GRN_TRUE;
      } else if (next[0] == 0xe3 &&
                 next[1] == 0x81 &&
                 (next[2] == 0x82 || /* U+3042 HIRAGANA LETTER A */
                  next[2] == 0x84 || /* U+3044 HIRAGANA LETTER I */
                  next[2] == 0x86 || /* U+3046 HIRAGANA LETTER U */
                  next[2] == 0x88 || /* U+3048 HIRAGANA LETTER E */
                  next[2] == 0x8a)) { /* U+304A HIRAGANA LETTER O */
        next_aiueoy = GRN_TRUE;
      } else if (next[0] == 0xe3 &&
                 next[1] == 0x82 &&
                 (next[2] == 0x84 || /* U+3084 HIRAGANA LETTER YA */
                  next[2] == 0x86 || /* U+3086 HIRAGANA LETTER YU */
                  next[2] == 0x88)) { /* U+3088 HIRAGANA LETTER YO */
        next_aiueoy = GRN_TRUE;
      } else if (next[0] == 0xe3 &&
                 next[1] == 0x82 &&
                 (next[2] == 0xa2 || /* U+30A2 KATAKANA LETTER A */
                  next[2] == 0xa4 || /* U+30A4 KATAKANA LETTER I */
                  next[2] == 0xa6 || /* U+30A6 KATAKANA LETTER U */
                  next[2] == 0xa8 || /* U+30A8 KATAKANA LETTER E */
                  next[2] == 0xaa)) { /* U+30AA KATAKANA LETTER O */
        next_aiueoy = GRN_TRUE;
      } else if (next[0] == 0xe3 &&
                 next[1] == 0x83 &&
                 (next[2] == 0xa4 || /* U+30E4 KATAKANA LETTER YA */
                  next[2] == 0xa6 || /* U+30E6 KATAKANA LETTER YU */
                  next[2] == 0xa8)) { /* U+30E8 KATAKANA LETTER YO */
        next_aiueoy = GRN_TRUE;
      }

      switch (next[0]) {
      case 0xe3 :
        switch (next[1]) {
        case 0x81 :
          if (0x81 <= next[2] && next[2] <= 0x8a) {
            /* U+3042 HIRAGANA LETTER SMALL A ..
             * U+304A HIRAGANA LETTER O */
            if ((next[2] % 2) == 1) { /* SMALL */
              next_consonant = 'x';
            }
          } else if (0x8b <= next[2] && next[2] <= 0x94) {
            /* U+304B HIRAGANA LETTER KA ..
             * U+3054 HIRAGANA LETTER GO */
            const char *gk = "gk";
            next_consonant = gk[next[2] % 2];
          } else if (0x95 <= next[2] && next[2] <= 0x9e) {
            /* U+3055 HIRAGANA LETTER SA ..
             * U+305E HIRAGANA LETTER ZO */
            if (next[2] == 0x97) {
              /* U+3057 HIRAGANA LETTER SI */
              next_consonant = 's';
            } else if (next[2] == 0x98) {
              /* U+3058 HIRAGANA LETTER ZI */
              next_consonant = 'j';
            } else {
              const char *zs = "zs";
              next_consonant = zs[next[2] % 2];
            }
          } else if (0x9f <= next[2] && next[2] <= 0xa9) {
            /* U+305F HIRAGANA LETTER TA ..
             * U+3069 HIRAGANA LETTER DO */
            const char *tdtjxtztdtd = "tdtjxtztdtd";
            next_consonant = tdtjxtztdtd[next[2] - 0x9f];
          } else if (0xaa <= next[2] && next[2] <= 0xae) {
            /* U+306A HIRAGANA LETTER NA ..
             * U+306E HIRAGANA LETTER NO */
            next_consonant = 'n';
          } else if (0xaf <= next[2] && next[2] <= 0xbd) {
            /* U+306F HIRAGANA LETTER HA ..
             * U+307D HIRAGANA LETTER PO */
            const char *phb = "phb";
            next_consonant = phb[next[2] % 3];
          } else if (0xbe <= next[2] && next[2] <= 0xbf) {
            /* U+307E HIRAGANA LETTER MA ..
             * U+307F HIRAGANA LETTER MI */
            next_consonant = 'm';
          }
          break;
        case 0x82 :
          if (0x80 <= next[2] && next[2] <= 0x82) {
            /* U+3080 HIRAGANA LETTER MU ..
             * U+3082 HIRAGANA LETTER MO */
            next_consonant = 'm';
          } else if (0x83 <= next[2] && next[2] <= 0x88) {
            /* U+3083 HIRAGANA LETTER SMALL YA ..
             * U+3088 HIRAGANA LETTER YO */
            if ((next[2] % 2) == 1) { /* SMALL */
              next_consonant = 'x';
            } else {
              next_consonant = 'y';
            }
          } else if (0x89 <= next[2] && next[2] <= 0x8d) {
            /* U+3089 HIRAGANA LETTER RA ..
             * U+308D HIRAGANA LETTER RO */
            next_consonant = 'r';
          } else if (0x8e <= next[2] && next[2] <= 0x92) {
            /* U+308E HIRAGANA LETTER SMALL WA ..
             * U+3092 HIRAGANA LETTER WO */
            if (next[2] == 0x8e) { /* SMALL */
              next_consonant = 'x';
            } else {
              next_consonant = 'w';
            }
          } else if (next[2] == 0x93) {
            /* U+3093 HIRAGANA LETTER N */
            /* TODO: Maybe 'm' */
            next_consonant = 'n';
          } else if (next[2] == 0x94) {
            /* U+3094 HIRAGANA LETTER VU */
            next_consonant = 'v';
          } else if (next[2] == 0x95) {
            /* U+3095 HIRAGANA LETTER SMALL KA */
            next_consonant = 'x';
          } else if (next[2] == 0x96) {
            /* U+3096 HIRAGANA LETTER SMALL KE */
            next_consonant = 'x';
          } else if (0xa1 <= next[2] && next[2] <= 0xaa) {
            /* U+30A1 KATAKANA LETTER SMALL A ..
             * U+30AA KATAKANA LETTER O */
            if ((next[2] % 2) == 1) { /* SMALL */
              next_consonant = 'x';
            }
          } else if (0xab <= next[2] && next[2] <= 0xb4) {
            /* U+30AB KATAKANA LETTER KA ..
             * U+30B4 KATAKANA LETTER GO */
            const char *gk = "gk";
            next_consonant = gk[next[2] % 2];
          } else if (0xb5 <= next[2] && next[2] <= 0xbe) {
            /* U+30B5 KATAKANA LETTER SA ..
             * U+30BE KATAKANA LETTER ZO */
            if (next[2] == 0xb7) {
              /* U+30B7 KATAKANA LETTER SI */
              next_consonant = 's';
            } else if (next[2] == 0x98) {
              /* U+30B8 KATAKANA LETTER ZI */
              next_consonant = 'j';
            } else {
              const char *zs = "zs";
              next_consonant = zs[next[2] % 2];
            }
          } else if (next[2] == 0xbf) {
            /* U+30BF KATAKANA LETTER TA */
            next_consonant = 't';
          }
          break;
        case 0x83 :
          if (0x80 <= next[2] && next[2] <= 0x89) {
            /* U+30C0 KATAKANA LETTER DA ..
             * U+30C9 KATAKANA LETTER DO */
            const char *dtjxtztdtd = "dtjxtztdtd";
            next_consonant = dtjxtztdtd[next[2] - 0x80];
          } else if (0x8a <= next[2] && next[2] <= 0x8e) {
            /* U+30CA KATAKANA LETTER NA ..
             * U+30CE KATAKANA LETTER NO */
            next_consonant = 'n';
          } else if (0x8f <= next[2] && next[2] <= 0x9d) {
            /* U+30CF KATAKANA LETTER HA ..
             * U+30DD KATAKANA LETTER PO */
            const char *bph = "bph";
            next_consonant = bph[next[2] % 3];
          } else if (0x9e <= next[2] && next[2] <= 0xa2) {
            /* U+30DE KATAKANA LETTER MA ..
             * U+30E2 KATAKANA LETTER MO */
            next_consonant = 'm';
          } else if (0xa3 <= next[2] && next[2] <= 0xa8) {
            /* U+30E3 KATAKANA LETTER SMALL YA ..
             * U+30E8 KATAKANA LETTER YO */
            if ((next[2] % 2) == 1) { /* SMALL */
              next_consonant = 'x';
            } else {
              next_consonant = 'y';
            }
          } else if (0xa9 <= next[2] && next[2] <= 0xad) {
            /* U+30E9 KATAKANA LETTER RA ..
             * U+30ED KATAKANA LETTER RO */
            next_consonant = 'r';
          } else if (0xae <= next[2] && next[2] <= 0xb2) {
            /* U+30EE KATAKANA LETTER SMALL WA ..
             * U+30F2 KATAKANA LETTER WO */
            if (next[2] == 0xae) { /* SMALL */
              next_consonant = 'x';
            } else {
              next_consonant = 'w';
            }
          } else if (next[2] == 0xb3) {
            /* U+30F3 KATAKANA LETTER N */
            /* TODO: Maybe 'm' */
            next_consonant = 'n';
          } else if (next[2] == 0xb4) {
            /* U+30F4 KATAKANA LETTER VU */
            next_consonant = 'v';
          } else if (next[2] == 0xb5) {
            /* U+30F5 KATAKANA LETTER SMALL KA */
            next_consonant = 'x';
          } else if (next[2] == 0xb6) {
            /* U+30F6 KATAKANA LETTER SMALL KE */
            next_consonant = 'x';
          } else if (0xb7 <= next[2] && next[2] <= 0xba) {
            /* U+30F7 KATAKANA LETTER VA ..
             * U+30FA KATAKANA LETTER VO */
            next_consonant = 'v';
          }
          break;
        default :
          break;
        }
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
        buffer[(*n_bytes)++] = aiueo[(current[2] - 0x81) / 2];
      } else if (0x8b <= current[2] && current[2] <= 0x94) {
        /* U+304B HIRAGANA LETTER KA ..
         * U+3054 HIRAGANA LETTER GO */
        const char *gk = "gk";
        buffer[(*n_bytes)++] = gk[current[2] % 2];
        if (next_small_y &&
            (current[2] == 0x8d ||  /* U+304D HIRAGANA LETTER KI */
             current[2] == 0x8e)) { /* U+304E HIRAGANA LETTER GI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0x8b) / 2];
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
            buffer[(*n_bytes)++] = zs[current[2] % 2];
          }
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0x95) / 2];
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
        } else if (next_consonant != '\0' && current[2] == 0xa3) {
          /* U+3063 HIRAGANA LETTER SMALL TU */
          buffer[(*n_bytes)++] = next_consonant;
        } else {
          const char *aaiiuuueeoo = "aaiiuuueeoo";
          if (current[2] == 0xa1) {
            /* U+3061 HIRAGANA LETTER TI */
            buffer[(*n_bytes)++] = 'c';
            buffer[(*n_bytes)++] = 'h';
          } else if (current[2] == 0xa2) {
            /* U+3062 HIRAGANA LETTER DI */
            buffer[(*n_bytes)++] = 'j';
          } else if (current[2] == 0xa3) {
            /* U+3063 HIRAGANA LETTER SMALL TU */
            buffer[(*n_bytes)++] = 'x';
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else if (current[2] == 0xa4) {
            /* U+3064 HIRAGANA LETTER TU */
            buffer[(*n_bytes)++] = 't';
            buffer[(*n_bytes)++] = 's';
          } else {
            const char *td____ztdtd = "td____ztdtd";
            buffer[(*n_bytes)++] = td____ztdtd[current[2] - 0x9f];
          }
          buffer[(*n_bytes)++] = aaiiuuueeoo[current[2] - 0x9f];
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
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0xaa)];
        }
      } else if (0xaf <= current[2] && current[2] <= 0xbd) {
        /* U+306F HIRAGANA LETTER HA ..
         * U+307D HIRAGANA LETTER PO */
        const char *phb = "phb";
        buffer[(*n_bytes)++] = phb[current[2] % 3];
        if (next_small_y &&
            (0xb2 <= current[2] &&  /* U+3072 HIRAGANA LETTER HI .. */
             current[2] <= 0xb4)) { /* U+3074 HIRAGANA LETTER PI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0xaf) / 3];
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
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0xbe)];
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
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0x80) + 2];
        }
      } else if (0x83 <= current[2] && current[2] <= 0x88) {
        /* U+3083 HIRAGANA LETTER SMALL YA ..
         * U+3088 HIRAGANA LETTER YO */
        if ((current[2] % 2) == 1) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'y';
        buffer[(*n_bytes)++] = auo[(current[2] - 0x83) / 2];
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
          buffer[(*n_bytes)++] = aiueo[current[2] - 0x89];
        }
      } else if (0x8e <= current[2] && current[2] <= 0x92) {
        /* U+308E HIRAGANA LETTER SMALL WA ..
         * U+3092 HIRAGANA LETTER WO */
        if (current[2] == 0x8e) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'w';
        buffer[(*n_bytes)++] = aaieo[current[2] - 0x8e];
      } else if (current[2] == 0x93) {
        /* U+3093 HIRAGANA LETTER N */
        if (next_pbm) {
          buffer[(*n_bytes)++] = 'm';
        } else {
          buffer[(*n_bytes)++] = 'n';
        }
        if (next_aiueoy) {
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
        buffer[(*n_bytes)++] = aiueo[(current[2] - 0xa1) / 2];
      } else if (0xab <= current[2] && current[2] <= 0xb4) {
        /* U+30AB KATAKANA LETTER KA ..
         * U+30B4 KATAKANA LETTER GO */
        const char *gk = "gk";
        buffer[(*n_bytes)++] = gk[current[2] % 2];
        if (next_small_y &&
            (current[2] == 0xad ||  /* U+30AD KATAKANA LETTER KI */
             current[2] == 0xae)) { /* U+U+30AE KATAKANA LETTER GI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0xab) / 2];
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
            buffer[(*n_bytes)++] = zs[current[2] % 2];
          }
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0xb5) / 2];
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
        } else if (next_consonant != '\0' && current[2] == 0x83) {
          /* U+30C3 KATAKANA LETTER SMALL TU */
          buffer[(*n_bytes)++] = next_consonant;
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
            buffer[(*n_bytes)++] = d____ztdtd[current[2] - 0x80];
          }
          buffer[(*n_bytes)++] = aiiuuueeoo[current[2] - 0x80];
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
          buffer[(*n_bytes)++] = aiueo[current[2] - 0x8a];
        }
      } else if (0x8f <= current[2] && current[2] <= 0x9d) {
        /* U+30CF KATAKANA LETTER HA ..
         * U+30DD KATAKANA LETTER PO */
        const char *bph = "bph";
        buffer[(*n_bytes)++] = bph[current[2] % 3];
        if (next_small_y &&
            (0x92 <= current[2] &&  /* U+30D2 KATAKANA LETTER HI .. */
             current[2] <= 0x94)) { /* U+30D4 KATAKANA LETTER PI */
          buffer[(*n_bytes)++] = 'y';
          buffer[(*n_bytes)++] = next_small_yayuyo;
          (*n_used_bytes) += next_char_length;
          (*n_used_characters)++;
        } else {
          buffer[(*n_bytes)++] = aiueo[(current[2] - 0x8f) / 3];
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
          buffer[(*n_bytes)++] = aiueo[current[2] - 0x9e];
        }
      } else if (0xa3 <= current[2] && current[2] <= 0xa8) {
        /* U+30E3 KATAKANA LETTER SMALL YA ..
         * U+30E8 KATAKANA LETTER YO */
        if ((current[2] % 2) == 1) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'y';
        buffer[(*n_bytes)++] = auo[(current[2] - 0xa3) / 2];
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
          buffer[(*n_bytes)++] = aiueo[current[2] - 0xa9];
        }
      } else if (0xae <= current[2] && current[2] <= 0xb2) {
        /* U+30EE KATAKANA LETTER SMALL WA ..
         * U+30F2 KATAKANA LETTER WO */
        if (current[2] == 0xae) { /* SMALL */
          buffer[(*n_bytes)++] = 'x';
        }
        buffer[(*n_bytes)++] = 'w';
        buffer[(*n_bytes)++] = aaieo[current[2] - 0xae];
      } else if (current[2] == 0xb3) {
        /* U+30F3 KATAKANA LETTER N */
        if (next_pbm) {
          buffer[(*n_bytes)++] = 'm';
        } else {
          buffer[(*n_bytes)++] = 'n';
        }
        if (next_aiueoy) {
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
        buffer[(*n_bytes)++] = aieo[current[2] - 0xb7];
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
