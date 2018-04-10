/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil

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
#include "../grn_token_cursor.h"

#include <groonga/plugin.h>

static int
parse_normalize_flags(grn_ctx *ctx, grn_obj *flag_names)
{
  int flags = 0;
  const char *names, *names_end;
  int length;

  names = GRN_TEXT_VALUE(flag_names);
  length = GRN_TEXT_LEN(flag_names);
  names_end = names + length;
  while (names < names_end) {
    if (*names == '|' || *names == ' ') {
      names += 1;
      continue;
    }

#define CHECK_FLAG(name)\
    if (((names_end - names) >= (sizeof(#name) - 1)) &&\
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

static grn_bool
is_normalizer(grn_ctx *ctx, grn_obj *object)
{
  if (object->header.type != GRN_PROC) {
    return GRN_FALSE;
  }

  if (grn_proc_get_type(ctx, object) != GRN_PROC_NORMALIZER) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_obj *
command_normalize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *normalizer_name;
  grn_obj *string;
  grn_obj *flag_names;

  normalizer_name = grn_plugin_proc_get_var(ctx, user_data, "normalizer", -1);
  string = grn_plugin_proc_get_var(ctx, user_data, "string", -1);
  flag_names = grn_plugin_proc_get_var(ctx, user_data, "flags", -1);
  if (GRN_TEXT_LEN(normalizer_name) == 0) {
    ERR(GRN_INVALID_ARGUMENT, "normalizer name is missing");
    return NULL;
  }

  {
    grn_obj *normalizer;
    grn_obj *grn_string;
    int flags;
    unsigned int normalized_length_in_bytes;
    unsigned int normalized_n_characters;

    flags = parse_normalize_flags(ctx, flag_names);
    normalizer = grn_ctx_get(ctx,
                             GRN_TEXT_VALUE(normalizer_name),
                             GRN_TEXT_LEN(normalizer_name));
    if (!normalizer) {
      ERR(GRN_INVALID_ARGUMENT,
          "[normalize] nonexistent normalizer: <%.*s>",
          (int)GRN_TEXT_LEN(normalizer_name),
          GRN_TEXT_VALUE(normalizer_name));
      return NULL;
    }

    if (!is_normalizer(ctx, normalizer)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, normalizer);
      ERR(GRN_INVALID_ARGUMENT,
          "[normalize] not normalizer: %.*s",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      grn_obj_unlink(ctx, normalizer);
      return NULL;
    }

    grn_string = grn_string_open(ctx,
                                 GRN_TEXT_VALUE(string), GRN_TEXT_LEN(string),
                                 normalizer, flags);
    grn_obj_unlink(ctx, normalizer);

    grn_ctx_output_map_open(ctx, "RESULT", 3);
    {
      const char *normalized;

      grn_string_get_normalized(ctx, grn_string,
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
        grn_ctx_output_array_open(ctx, "types", normalized_n_characters);
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
        grn_ctx_output_array_open(ctx, "checks", normalized_length_in_bytes);
        for (i = 0; i < normalized_length_in_bytes; i++) {
          grn_ctx_output_int32(ctx, checks[i]);
        }
        grn_ctx_output_array_close(ctx);
      } else {
        grn_ctx_output_array_open(ctx, "checks", 0);
        grn_ctx_output_array_close(ctx);
      }
    }
    grn_ctx_output_map_close(ctx);

    grn_obj_unlink(ctx, grn_string);
  }

  return NULL;
}

void
grn_proc_init_normalize(grn_ctx *ctx)
{
  grn_expr_var vars[3];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "normalizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "string", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_command_create(ctx,
                            "normalize", -1,
                            command_normalize,
                            3,
                            vars);
}
