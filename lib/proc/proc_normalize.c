/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2021-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_proc.h"
#include "../grn_ctx.h"

#include <groonga/plugin.h>

static int
parse_normalize_flags(grn_ctx *ctx, grn_raw_string *flags_raw)
{
  int flags = 0;
  const char *names, *names_end;

  names = flags_raw->value;
  names_end = names + flags_raw->length;
  while (names < names_end) {
    if (*names == '|' || *names == ' ') {
      names += 1;
      continue;
    }

#define CHECK_FLAG(name)\
    if (((size_t)(names_end - names) >= (sizeof(#name) - 1)) && \
        (!memcmp(names, #name, sizeof(#name) - 1))) {\
      flags |= GRN_STRING_ ## name;\
      names += sizeof(#name) - 1;\
      continue;\
    }

    CHECK_FLAG(REMOVE_BLANK);
    CHECK_FLAG(WITH_TYPES);
    CHECK_FLAG(WITH_CHECKS);
    CHECK_FLAG(REMOVE_TOKENIZED_DELIMITER);

#define GRN_STRING_NONE 0
    CHECK_FLAG(NONE);
#undef GRN_STRING_NONE

    ERR(GRN_INVALID_ARGUMENT, "[normalize] invalid flag: <%.*s>",
        (int)(names_end - names), names);
    return 0;
#undef CHECK_FLAG
  }

  return flags;
}

static grn_obj *
command_normalize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  const char *context_tag = "[normalize]";
  grn_raw_string normalizer_raw;
  grn_raw_string string_raw;
  grn_raw_string flags_raw;
  grn_raw_string normalizers_raw;

#define GET_VALUE(name)                                         \
  name ## _raw.value =                                          \
    grn_plugin_proc_get_var_string(ctx,                         \
                                   user_data,                   \
                                   #name,                       \
                                   strlen(#name),               \
                                   &(name ## _raw.length))

  GET_VALUE(normalizer);
  GET_VALUE(string);
  GET_VALUE(flags);
  GET_VALUE(normalizers);

#undef GET_VALUE

  if (normalizers_raw.length == 0 && normalizer_raw.length > 0) {
    normalizers_raw = normalizer_raw;
  }
  if (normalizers_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s normalizer name is missing",
                     context_tag);
    return NULL;
  }

  {
    int flags;
    int n_elements = 3;
    grn_obj *lexicon;
    grn_obj *grn_string;
    unsigned int normalized_length_in_bytes;
    unsigned int normalized_n_characters;
    const uint64_t *offsets;

    flags = parse_normalize_flags(ctx, &flags_raw);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }

    lexicon = grn_proc_lexicon_open(ctx,
                                    NULL,
                                    &normalizers_raw,
                                    NULL,
                                    context_tag);
    if (!lexicon) {
      return NULL;
    }

    grn_string = grn_string_open(ctx,
                                 string_raw.value,
                                 (unsigned int)(string_raw.length),
                                 lexicon,
                                 flags);

    offsets = grn_string_get_offsets(ctx, grn_string);
    if (offsets) {
      n_elements++;
    }

    grn_ctx_output_map_open(ctx, "RESULT", n_elements);
    {
      const char *normalized;

      grn_string_get_normalized(ctx,
                                grn_string,
                                &normalized,
                                &normalized_length_in_bytes,
                                &normalized_n_characters);
      grn_ctx_output_cstr(ctx, "normalized");
      grn_ctx_output_str(ctx, normalized, normalized_length_in_bytes);
    }
    {
      const unsigned char *types;

      types = grn_string_get_types(ctx, grn_string);
      grn_ctx_output_cstr(ctx, "types");
      if (types) {
        unsigned int i;
        grn_ctx_output_array_open(ctx, "types", (int)normalized_n_characters);
        for (i = 0; i < normalized_n_characters; i++) {
          grn_ctx_output_cstr(ctx, grn_char_type_to_string(types[i]));
        }
        grn_ctx_output_array_close(ctx);
      } else {
        grn_ctx_output_array_open(ctx, "types", 0);
        grn_ctx_output_array_close(ctx);
      }
    }
    {
      const short *checks;

      checks = grn_string_get_checks(ctx, grn_string);
      grn_ctx_output_cstr(ctx, "checks");
      if (checks) {
        unsigned int i;
        grn_ctx_output_array_open(ctx, "checks", (int)normalized_length_in_bytes);
        for (i = 0; i < normalized_length_in_bytes; i++) {
          grn_ctx_output_int32(ctx, checks[i]);
        }
        grn_ctx_output_array_close(ctx);
      } else {
        grn_ctx_output_array_open(ctx, "checks", 0);
        grn_ctx_output_array_close(ctx);
      }
    }
    if (offsets) {
      unsigned int i;

      grn_ctx_output_cstr(ctx, "offsets");
      grn_ctx_output_array_open(ctx, "offsets", (int)normalized_n_characters);
      for (i = 0; i < normalized_n_characters; i++) {
        grn_ctx_output_uint64(ctx, offsets[i]);
      }
      grn_ctx_output_array_close(ctx);
    }

    grn_ctx_output_map_close(ctx);

    grn_obj_unlink(ctx, grn_string);

    grn_obj_unlink(ctx, lexicon);
  }

  return NULL;
}

void
grn_proc_init_normalize(grn_ctx *ctx)
{
  grn_expr_var vars[4];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "normalizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "string", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "normalizers", -1);
  grn_plugin_command_create(ctx,
                            "normalize", -1,
                            command_normalize,
                            4,
                            vars);
}
