/*
  Copyright(C) 2010-2016  Brazil
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

#include "grn.h"
#include "grn_nfkc.h"
#include <groonga/nfkc.h>

#ifdef GRN_WITH_NFKC

grn_char_type
grn_nfkc_char_type(const unsigned char *utf8)
{
  return grn_nfkc50_char_type(utf8);
}

const char *
grn_nfkc_decompose(const unsigned char *utf8)
{
  return grn_nfkc50_decompose(utf8);
}

const char *
grn_nfkc_compose(const unsigned char *prefix_utf8,
                 const unsigned char *suffix_utf8)
{
  return grn_nfkc50_compose(prefix_utf8, suffix_utf8);
}

void
grn_nfkc_normalize_options_init(grn_ctx *ctx,
                                grn_nfkc_normalize_options *options,
                                grn_nfkc_char_type_func char_type_func,
                                grn_nfkc_decompose_func decompose_func,
                                grn_nfkc_compose_func compose_func)
{
  options->char_type_func = char_type_func;
  options->decompose_func = decompose_func;
  options->compose_func = compose_func;
  options->include_removed_source_location = GRN_TRUE;
  options->report_source_offset = GRN_FALSE;
  options->unify_kana = GRN_FALSE;
  options->unify_kana_case = GRN_FALSE;
  options->unify_kana_voiced_sound_mark = GRN_FALSE;
  options->unify_hyphen = GRN_FALSE;
  options->unify_prolonged_sound_mark = GRN_FALSE;
  options->unify_hyphen_and_prolonged_sound_mark = GRN_FALSE;
  options->unify_middle_dot = GRN_FALSE;
  options->unify_katakana_v_sounds = GRN_FALSE;
  options->unify_katakana_bu_sound = GRN_FALSE;
  options->unify_to_romaji = GRN_FALSE;
  options->unify_to_katakana = GRN_FALSE;
  options->remove_blank = GRN_FALSE;
  options->remove_new_line = true;
  options->remove_symbol = false;
  options->strip = false;
}

void
grn_nfkc100_normalize_options_init(grn_ctx *ctx,
                                   grn_nfkc_normalize_options *options)
{
  grn_nfkc_normalize_options_init(ctx,
                                  options,
                                  grn_nfkc100_char_type,
                                  grn_nfkc100_decompose,
                                  grn_nfkc100_compose);
}

void
grn_nfkc121_normalize_options_init(grn_ctx *ctx,
                                   grn_nfkc_normalize_options *options)
{
  grn_nfkc_normalize_options_init(ctx,
                                  options,
                                  grn_nfkc121_char_type,
                                  grn_nfkc121_decompose,
                                  grn_nfkc121_compose);
}

void
grn_nfkc130_normalize_options_init(grn_ctx *ctx,
                                   grn_nfkc_normalize_options *options)
{
  grn_nfkc_normalize_options_init(ctx,
                                  options,
                                  grn_nfkc130_char_type,
                                  grn_nfkc130_decompose,
                                  grn_nfkc130_compose);
}

grn_rc
grn_nfkc_normalize_options_apply(grn_ctx *ctx,
                                 grn_nfkc_normalize_options *options,
                                 grn_obj *raw_options)
{
  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length) {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                     "include_removed_source_location")) {
      options->include_removed_source_location =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->include_removed_source_location);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "report_source_offset")) {
      options->report_source_offset =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->report_source_offset);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_kana")) {
      options->unify_kana = grn_vector_get_element_bool(ctx,
                                                        raw_options,
                                                        i,
                                                        options->unify_kana);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_kana_case")) {
      options->unify_kana_case =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_kana_case);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "unify_kana_voiced_sound_mark")) {
      options->unify_kana_voiced_sound_mark =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_kana_voiced_sound_mark);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_hyphen")) {
      options->unify_hyphen = grn_vector_get_element_bool(ctx,
                                                          raw_options,
                                                          i,
                                                          options->unify_hyphen);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "unify_prolonged_sound_mark")) {
      options->unify_prolonged_sound_mark =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_prolonged_sound_mark);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "unify_hyphen_and_prolonged_sound_mark")) {
      options->unify_hyphen_and_prolonged_sound_mark =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_hyphen_and_prolonged_sound_mark);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_middle_dot")) {
      options->unify_middle_dot =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_middle_dot);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_v_sounds")) {
      options->unify_katakana_v_sounds =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_v_sounds);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_bu_sound")) {
      options->unify_katakana_bu_sound =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_bu_sound);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_to_romaji")) {
      options->unify_to_romaji =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_to_romaji);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_to_katakana")) {
      options->unify_to_katakana =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_to_katakana);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_blank")) {
      options->remove_blank =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->remove_blank);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_new_line")) {
      options->remove_new_line =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->remove_new_line);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_symbol")) {
      options->remove_symbol =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->remove_symbol);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "strip")) {
      options->strip =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->strip);
    }
  } GRN_OPTION_VALUES_EACH_END();

  return ctx->rc;
}

void
grn_nfkc_normalize_options_fin(grn_ctx *ctx,
                               grn_nfkc_normalize_options *options)
{
}

#endif /* GRN_WITH_NFKC */
