/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

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
#ifndef GRN_NFKC_H
#define GRN_NFKC_H

#ifdef	__cplusplus
extern "C" {
#endif

enum {
  grn_str_null = 0,
  grn_str_alpha,
  grn_str_digit,
  grn_str_symbol,
  grn_str_hiragana,
  grn_str_katakana,
  grn_str_kanji,
  grn_str_others
};

#ifdef __cplusplus
}
#endif

#endif /* GRN_NFKC_H */
