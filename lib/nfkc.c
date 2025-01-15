/*
  Copyright (C) 2010-2016  Brazil
  Copyright (C) 2018-2025  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn.h"
#include "grn_error.h"
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
  options->include_removed_source_location = true;
  options->report_source_offset = false;
  options->unify_latin_alphabet_with = false;
  options->unify_kana = false;
  options->unify_kana_case = false;
  options->unify_kana_voiced_sound_mark = false;
  options->unify_hyphen = false;
  options->unify_prolonged_sound_mark = false;
  options->unify_hyphen_and_prolonged_sound_mark = false;
  options->unify_middle_dot = false;
  options->unify_katakana_v_sounds = false;
  options->unify_katakana_bu_sound = false;
  options->unify_katakana_du_small_sounds = false;
  options->unify_katakana_du_sound = false;
  options->unify_katakana_zu_small_sounds = false;
  options->unify_katakana_wo_sound = false;
  options->unify_katakana_di_sound = false;
  options->unify_katakana_gu_small_sounds = false;
  options->unify_kana_hyphen = false;
  options->unify_kana_prolonged_sound_mark = false;
  options->unify_katakana_trailing_o = false;
  options->unify_to_romaji = false;
  options->unify_to_katakana = false;
  options->remove_blank = false;
  options->remove_blank_force = false;
  options->remove_blank_force_is_set = false;
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

void
grn_nfkc150_normalize_options_init(grn_ctx *ctx,
                                   grn_nfkc_normalize_options *options)
{
  grn_nfkc_normalize_options_init(ctx,
                                  options,
                                  grn_nfkc150_char_type,
                                  grn_nfkc150_decompose,
                                  grn_nfkc150_compose);
}

void
grn_nfkc160_normalize_options_init(grn_ctx *ctx,
                                   grn_nfkc_normalize_options *options)
{
  grn_nfkc_normalize_options_init(ctx,
                                  options,
                                  grn_nfkc160_char_type,
                                  grn_nfkc160_decompose,
                                  grn_nfkc160_compose);
}

static const grn_nfkc_funcs grn_nfkc_funcs_null = {
  NULL,
  NULL,
  NULL,
};

static const grn_nfkc_funcs grn_nfkc50_funcs = {
  grn_nfkc50_char_type,
  grn_nfkc50_decompose,
  grn_nfkc50_compose,
};

static const grn_nfkc_funcs grn_nfkc100_funcs = {
  grn_nfkc100_char_type,
  grn_nfkc100_decompose,
  grn_nfkc100_compose,
};

static const grn_nfkc_funcs grn_nfkc121_funcs = {
  grn_nfkc121_char_type,
  grn_nfkc121_decompose,
  grn_nfkc121_compose,
};

static const grn_nfkc_funcs grn_nfkc130_funcs = {
  grn_nfkc130_char_type,
  grn_nfkc130_decompose,
  grn_nfkc130_compose,
};

static const grn_nfkc_funcs grn_nfkc150_funcs = {
  grn_nfkc150_char_type,
  grn_nfkc150_decompose,
  grn_nfkc150_compose,
};

static const grn_nfkc_funcs grn_nfkc160_funcs = {
  grn_nfkc160_char_type,
  grn_nfkc160_decompose,
  grn_nfkc160_compose,
};

grn_nfkc_funcs
grn_nfkc_version_option_process(grn_ctx *ctx,
                                grn_obj *raw_options,
                                unsigned int i,
                                grn_raw_string *name_raw,
                                const char *tag)
{
  const char *version;
  grn_id domain;
  unsigned int version_length =
    grn_vector_get_element(ctx, raw_options, i, &version, NULL, &domain);
  if (!grn_type_id_is_text_family(ctx, domain)) {
    grn_obj value;
    GRN_VALUE_FIX_SIZE_INIT(&value, GRN_OBJ_DO_SHALLOW_COPY, domain);
    GRN_TEXT_SET(ctx, &value, version, version_length);
    grn_obj inspected;
    grn_inspect(ctx, &inspected, &value);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must be a text: <%.*s>",
        tag,
        (int)(name_raw->length),
        name_raw->value,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_OBJ_FIN(ctx, &value);
    return grn_nfkc_funcs_null;
  }
  if (version_length == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must not be empty",
        tag,
        (int)(name_raw->length),
        name_raw->value);
    return grn_nfkc_funcs_null;
  }
  grn_raw_string version_raw;
  version_raw.value = version;
  version_raw.length = version_length;
  if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "5.0.0")) {
    return grn_nfkc50_funcs;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "10.0.0")) {
    return grn_nfkc100_funcs;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "12.1.0")) {
    return grn_nfkc121_funcs;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "13.0.0")) {
    return grn_nfkc130_funcs;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "15.0.0")) {
    return grn_nfkc150_funcs;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(version_raw, "16.0.0")) {
    return grn_nfkc160_funcs;
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must be one of \"5.0.0\", \"10.0.0\", \"12.1.0\", "
        "\"13.0.0\", \"15.0.0\" or \"16.0.0\": <%.*s>",
        tag,
        (int)(name_raw->length),
        name_raw->value,
        (int)(version_raw.length),
        version_raw.value);
    return grn_nfkc_funcs_null;
  }
}

grn_rc
grn_nfkc_normalize_options_apply(grn_ctx *ctx,
                                 grn_nfkc_normalize_options *options,
                                 grn_obj *raw_options,
                                 const char *tag)
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
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "unify_latin_alphabet_with")) {
      options->unify_latin_alphabet_with =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_latin_alphabet_with);
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
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_du_small_sounds")) {
      options->unify_katakana_du_small_sounds =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_du_small_sounds);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_du_sound")) {
      options->unify_katakana_du_sound =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_du_sound);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_zu_small_sounds")) {
      options->unify_katakana_zu_small_sounds =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_zu_small_sounds);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_wo_sound")) {
      options->unify_katakana_wo_sound =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_wo_sound);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_di_sound")) {
      options->unify_katakana_di_sound =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_di_sound);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_gu_small_sounds")) {
      options->unify_katakana_gu_small_sounds =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_gu_small_sounds);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_kana_hyphen")) {
      options->unify_kana_hyphen =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_kana_hyphen);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_kana_prolonged_sound_mark")) {
      options->unify_kana_prolonged_sound_mark =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_kana_prolonged_sound_mark);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "unify_katakana_trailing_o")) {
      options->unify_katakana_trailing_o =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->unify_katakana_trailing_o);
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
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_blank_force")) {
      options->remove_blank_force =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->remove_blank_force);
      options->remove_blank_force_is_set = true;
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
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "version")) {
      grn_nfkc_funcs funcs =
        grn_nfkc_version_option_process(ctx, raw_options, i, &name_raw, tag);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      options->char_type_func = funcs.char_type_func;
      options->decompose_func = funcs.decompose_func;
      options->compose_func = funcs.compose_func;
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
