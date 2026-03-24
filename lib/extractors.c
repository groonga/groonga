/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_extractor.h"
#include "grn_extractors.h"

typedef struct {
  bool remove_tag;
  bool expand_character_reference;
} html_extract_options;

static void
html_extract_options_init(grn_ctx *ctx, html_extract_options *options)
{
  options->remove_tag = true;
  options->expand_character_reference = true;
}

static void
html_extract_options_fin(grn_ctx *ctx, html_extract_options *options)
{
}

static void
html_extract_close_options(grn_ctx *ctx, void *data)
{
  html_extract_options *options = data;
  html_extract_options_fin(ctx, options);
  GRN_FREE(options);
}

static void *
html_extract_open_options(grn_ctx *ctx,
                          grn_obj *extractor,
                          grn_obj *raw_options,
                          void *user_data)
{
  const char *tag = "[extractor][html]";
  html_extract_options *options = GRN_CALLOC(sizeof(html_extract_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options",
        tag);
    return NULL;
  }

  html_extract_options_init(ctx, options);

  GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length)
  {
    grn_raw_string name_raw;
    name_raw.value = name;
    name_raw.length = name_length;

    if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw, "remove_tag")) {
      options->remove_tag =
        grn_vector_get_element_bool(ctx, raw_options, i, options->remove_tag);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(name_raw,
                                            "expand_character_reference")) {
      options->expand_character_reference =
        grn_vector_get_element_bool(ctx,
                                    raw_options,
                                    i,
                                    options->expand_character_reference);
    }
  }
  GRN_OPTION_VALUES_EACH_END();

  return options;
}

static bool
html_expand_numeric_char_ref(grn_ctx *ctx, grn_obj *buffer, uint32_t code_point)
{
  /* Special code points.
   * See also:
   * https://html.spec.whatwg.org/multipage/parsing.html#numeric-character-reference-end-state
   */
  switch (code_point) {
  case 0x80:
    code_point = 0x20AC;
    break;
  case 0x82:
    code_point = 0x201A;
    break;
  case 0x83:
    code_point = 0x0192;
    break;
  case 0x84:
    code_point = 0x201E;
    break;
  case 0x85:
    code_point = 0x2026;
    break;
  case 0x86:
    code_point = 0x2020;
    break;
  case 0x87:
    code_point = 0x2021;
    break;
  case 0x88:
    code_point = 0x02C6;
    break;
  case 0x89:
    code_point = 0x2030;
    break;
  case 0x8A:
    code_point = 0x0160;
    break;
  case 0x8B:
    code_point = 0x2039;
    break;
  case 0x8C:
    code_point = 0x0152;
    break;
  case 0x8E:
    code_point = 0x017D;
    break;
  case 0x91:
    code_point = 0x2018;
    break;
  case 0x92:
    code_point = 0x2019;
    break;
  case 0x93:
    code_point = 0x201C;
    break;
  case 0x94:
    code_point = 0x201D;
    break;
  case 0x95:
    code_point = 0x2022;
    break;
  case 0x96:
    code_point = 0x2013;
    break;
  case 0x97:
    code_point = 0x2014;
    break;
  case 0x98:
    code_point = 0x02DC;
    break;
  case 0x99:
    code_point = 0x2122;
    break;
  case 0x9A:
    code_point = 0x0161;
    break;
  case 0x9B:
    code_point = 0x203A;
    break;
  case 0x9C:
    code_point = 0x0153;
    break;
  case 0x9E:
    code_point = 0x017E;
    break;
  case 0x9F:
    code_point = 0x0178;
    break;
  default:
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

static grn_obj *
html_extract(grn_ctx *ctx, grn_obj *extractor, grn_extract_data *data)
{
  const char *tag = "[extractor][html]";

  grn_obj *value = grn_extract_data_get_value(ctx, data);
  if (!grn_type_id_is_text_family(ctx, value->header.domain)) {
    /* Do nothing */
    return NULL;
  }

  grn_obj *table = grn_extract_data_get_table(ctx, data);
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT, "%s related table information is missing", tag);
    return NULL;
  }

  uint32_t i = grn_extract_data_get_index(ctx, data);
  html_extract_options *options =
    grn_table_cache_extractors_options(ctx,
                                       table,
                                       i,
                                       html_extract_open_options,
                                       html_extract_close_options,
                                       NULL);
  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  grn_obj *output = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  const char *current = GRN_TEXT_VALUE(value);
  const char *end = current + GRN_TEXT_LEN(value);

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
          if (!options->remove_tag) {
            GRN_TEXT_PUT(ctx, output, start_tag, (current - start_tag + 1));
          }
          state = HTML_STATE_TEXT;
          start_tag = NULL;
          processed = true;
        }
        break;
      case HTML_STATE_IN_CHAR_REF:
        if (*current == ';') {
          bool valid = false;
          if (start_char_ref[1] == '#') {
            long code_point = -1;
            if (start_char_ref[2] == 'x' || start_char_ref[2] == 'X') {
              /* "&#xH...;" */
              const char *code_point_end = NULL;
              code_point =
                grn_htoui(start_char_ref + 3, current, &code_point_end);
              if (code_point_end != current) {
                code_point = -1;
              }
            } else {
              /* "&#D...;" */
              const char *code_point_end = NULL;
              code_point =
                grn_atoll(start_char_ref + 2, current, &code_point_end);
              if (code_point_end != current) {
                code_point = -1;
              }
            }
            if (code_point >= 0) {
              valid =
                html_expand_numeric_char_ref(ctx, output, (uint32_t)code_point);
            }
          } else {
            /* "&NAME;" */
            valid = html_expand_named_char_ref(
              ctx,
              output,
              start_char_ref + 1,
              (size_t)(current - start_char_ref - 1));
          }
          if (valid) {
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
        break;
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
        size_t length =
          (size_t)(current - start_char_ref) + (size_t)char_length;
        GRN_TEXT_PUT(ctx, output, start_char_ref, length);
        state = HTML_STATE_TEXT;
        start_char_ref = NULL;
      }
      break;
    default:
      GRN_TEXT_PUT(ctx, output, current, (size_t)char_length);
      break;
    }
  }
  if (state == HTML_STATE_IN_TAG) {
    if (!options->remove_tag) {
      GRN_TEXT_PUT(ctx, output, start_tag, (size_t)(current - start_tag));
    }
  }

  return output;
}

grn_rc
grn_db_init_builtin_extractors(grn_ctx *ctx)
{
  {
    grn_obj *extractor;
    extractor = grn_extractor_create(ctx, "ExtractorHTML", -1);
    grn_extractor_set_extract_func(ctx, extractor, html_extract);
  }

  return GRN_SUCCESS;
}
