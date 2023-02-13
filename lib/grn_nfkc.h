/*
  Copyright(C) 2009-2016  Brazil
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

#pragma once

#include "grn.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef grn_char_type (*grn_nfkc_char_type_func)(const unsigned char *utf8);
typedef const char *(*grn_nfkc_decompose_func)(const unsigned char *utf8);
typedef const char *(*grn_nfkc_compose_func)(const unsigned char *prefix_utf8,
                                             const unsigned char *suffix_utf8);

typedef struct {
  grn_nfkc_char_type_func char_type_func;
  grn_nfkc_decompose_func decompose_func;
  grn_nfkc_compose_func compose_func;
  grn_bool include_removed_source_location;
  grn_bool report_source_offset;
  grn_bool unify_kana;
  grn_bool unify_kana_case;
  grn_bool unify_kana_voiced_sound_mark;
  grn_bool unify_hyphen;
  grn_bool unify_prolonged_sound_mark;
  grn_bool unify_hyphen_and_prolonged_sound_mark;
  grn_bool unify_middle_dot;
  grn_bool unify_katakana_v_sounds;
  grn_bool unify_katakana_bu_sound;
  grn_bool unify_katakana_du_small_sounds;
  grn_bool unify_katakana_du_sound;
  grn_bool unify_katakana_zu_small_sounds;
  grn_bool unify_katakana_wo_sound;
  grn_bool unify_katakana_di_sound;
  grn_bool unify_katakana_gu_small_sounds;
  grn_bool unify_katakana_trailing_o;
  grn_bool unify_kana_prolonged_sound_mark;
  grn_bool unify_to_romaji;
  grn_bool unify_to_katakana;
  grn_bool remove_blank;
  bool remove_new_line;
  bool remove_symbol;
  bool strip;
} grn_nfkc_normalize_options;

typedef void (*grn_nfkc_normalize_options_init_func)(grn_ctx *ctx,
                                                     grn_nfkc_normalize_options *options);

const char *grn_nfkc_decompose(const unsigned char *utf8);
const char *grn_nfkc_compose(const unsigned char *prefix_utf8,
                             const unsigned char *suffix_utf8);

GRN_API grn_char_type grn_nfkc50_char_type(const unsigned char *utf8);
const char *grn_nfkc50_decompose(const unsigned char *utf8);
const char *grn_nfkc50_compose(const unsigned char *prefix_utf8,
                               const unsigned char *suffix_utf8);

GRN_API grn_char_type grn_nfkc100_char_type(const unsigned char *utf8);
const char *grn_nfkc100_decompose(const unsigned char *utf8);
const char *grn_nfkc100_compose(const unsigned char *prefix_utf8,
                                const unsigned char *suffix_utf8);

GRN_API grn_char_type grn_nfkc121_char_type(const unsigned char *utf8);
const char *grn_nfkc121_decompose(const unsigned char *utf8);
const char *grn_nfkc121_compose(const unsigned char *prefix_utf8,
                                const unsigned char *suffix_utf8);

GRN_API grn_char_type grn_nfkc130_char_type(const unsigned char *utf8);
const char *grn_nfkc130_decompose(const unsigned char *utf8);
const char *grn_nfkc130_compose(const unsigned char *prefix_utf8,
                                const unsigned char *suffix_utf8);

GRN_API grn_char_type grn_nfkc150_char_type(const unsigned char *utf8);
const char *grn_nfkc150_decompose(const unsigned char *utf8);
const char *grn_nfkc150_compose(const unsigned char *prefix_utf8,
                                const unsigned char *suffix_utf8);

void grn_nfkc_normalize_options_init(grn_ctx *ctx,
                                     grn_nfkc_normalize_options *options,
                                     grn_nfkc_char_type_func char_type_func,
                                     grn_nfkc_decompose_func decompose_func,
                                     grn_nfkc_compose_func compose_func);
void grn_nfkc100_normalize_options_init(grn_ctx *ctx,
                                        grn_nfkc_normalize_options *options);
void grn_nfkc121_normalize_options_init(grn_ctx *ctx,
                                        grn_nfkc_normalize_options *options);
void grn_nfkc130_normalize_options_init(grn_ctx *ctx,
                                        grn_nfkc_normalize_options *options);
void grn_nfkc150_normalize_options_init(grn_ctx *ctx,
                                        grn_nfkc_normalize_options *options);
grn_rc grn_nfkc_normalize_options_apply(grn_ctx *ctx,
                                        grn_nfkc_normalize_options *options,
                                        grn_obj *raw_options);
void grn_nfkc_normalize_options_fin(grn_ctx *ctx,
                                    grn_nfkc_normalize_options *options);

#ifdef __cplusplus
}
#endif
