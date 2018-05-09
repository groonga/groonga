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

static unsigned int
parse_tokenize_flags(grn_ctx *ctx, grn_raw_string *flags_raw)
{
  unsigned int flags = 0;
  const char *names, *names_end;

  names = flags_raw->value;
  names_end = names + flags_raw->length;
  while (names < names_end) {
    if (*names == '|' || *names == ' ') {
      names += 1;
      continue;
    }

#define CHECK_FLAG(name)\
    if (((names_end - names) >= (sizeof(#name) - 1)) &&\
        (!memcmp(names, #name, sizeof(#name) - 1))) {\
      flags |= GRN_TOKEN_CURSOR_ ## name;\
      names += sizeof(#name) - 1;\
      continue;\
    }

    CHECK_FLAG(ENABLE_TOKENIZED_DELIMITER);

#define GRN_TOKEN_CURSOR_NONE 0
    CHECK_FLAG(NONE);
#undef GRN_TOKEN_CURSOR_NONE

    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[tokenize] invalid flag: <%.*s>",
                     (int)(names_end - names), names);
    return 0;
#undef CHECK_FLAG
  }

  return flags;
}

typedef struct {
  grn_id id;
  int32_t position;
  grn_bool force_prefix;
  uint64_t source_offset;
  uint32_t source_length;
} tokenize_token;

static void
output_tokens(grn_ctx *ctx,
              grn_obj *tokens,
              grn_obj *lexicon,
              grn_obj *index_column)
{
  int i, n_tokens, n_elements;
  grn_obj estimated_size;
  grn_bool have_source_location;

  n_tokens = GRN_BULK_VSIZE(tokens) / sizeof(tokenize_token);
  n_elements = 3;
  if (index_column) {
    n_elements++;
    GRN_UINT32_INIT(&estimated_size, 0);
  }
  for (i = 0; i < n_tokens; i++) {
    tokenize_token *token;
    token = ((tokenize_token *)(GRN_BULK_HEAD(tokens))) + i;
    if (token->source_offset > 0 || token->source_length > 0) {
      have_source_location = GRN_TRUE;
      break;
    }
  }
  if (have_source_location) {
    n_elements += 2;
  }

  grn_ctx_output_array_open(ctx, "TOKENS", n_tokens);
  for (i = 0; i < n_tokens; i++) {
    tokenize_token *token;
    char value[GRN_TABLE_MAX_KEY_SIZE];
    unsigned int value_size;

    token = ((tokenize_token *)(GRN_BULK_HEAD(tokens))) + i;

    grn_ctx_output_map_open(ctx, "TOKEN", n_elements);

    grn_ctx_output_cstr(ctx, "value");
    value_size = grn_table_get_key(ctx, lexicon, token->id,
                                   value, GRN_TABLE_MAX_KEY_SIZE);
    grn_ctx_output_str(ctx, value, value_size);

    grn_ctx_output_cstr(ctx, "position");
    grn_ctx_output_int32(ctx, token->position);

    grn_ctx_output_cstr(ctx, "force_prefix");
    grn_ctx_output_bool(ctx, token->force_prefix);

    if (index_column) {
      GRN_BULK_REWIND(&estimated_size);
      grn_obj_get_value(ctx, index_column, token->id, &estimated_size);
      grn_ctx_output_cstr(ctx, "estimated_size");
      grn_ctx_output_int64(ctx, GRN_UINT32_VALUE(&estimated_size));
    }

    if (have_source_location) {
      grn_ctx_output_cstr(ctx, "source_offset");
      grn_ctx_output_uint64(ctx, token->source_offset);

      grn_ctx_output_cstr(ctx, "source_length");
      grn_ctx_output_uint32(ctx, token->source_length);
    }

    grn_ctx_output_map_close(ctx);
  }

  if (index_column) {
    GRN_OBJ_FIN(ctx, &estimated_size);
  }

  grn_ctx_output_array_close(ctx);
}

static void
tokenize(grn_ctx *ctx,
         grn_obj *lexicon,
         grn_raw_string *string_raw,
         grn_tokenize_mode mode,
         unsigned int flags,
         grn_obj *tokens)
{
  grn_token_cursor *token_cursor;

  token_cursor =
    grn_token_cursor_open(ctx,
                          lexicon,
                          string_raw->value,
                          string_raw->length,
                          mode,
                          flags);
  if (!token_cursor) {
    return;
  }

  while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
    grn_id token_id = grn_token_cursor_next(ctx, token_cursor);
    grn_token *token;
    tokenize_token *current_token;
    if (token_id == GRN_ID_NIL) {
      continue;
    }
    token = grn_token_cursor_get_token(ctx, token_cursor);
    grn_bulk_space(ctx, tokens, sizeof(tokenize_token));
    current_token = ((tokenize_token *)(GRN_BULK_CURR(tokens))) - 1;
    current_token->id = token_id;
    current_token->position = token_cursor->pos;
    current_token->force_prefix = token_cursor->force_prefix;
    current_token->source_offset = grn_token_get_source_offset(ctx, token);
    current_token->source_length = grn_token_get_source_length(ctx, token);
  }
  grn_token_cursor_close(ctx, token_cursor);
}

static grn_obj *
command_table_tokenize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_raw_string table_raw;
  grn_raw_string string_raw;
  grn_raw_string flags_raw;
  grn_raw_string mode_raw;
  grn_raw_string index_column_raw;

#define GET_VALUE(name)                                         \
  name ## _raw.value =                                          \
    grn_plugin_proc_get_var_string(ctx,                         \
                                   user_data,                   \
                                   #name,                       \
                                   strlen(#name),               \
                                   &(name ## _raw.length))

  GET_VALUE(table);
  GET_VALUE(string);
  GET_VALUE(flags);
  GET_VALUE(mode);
  GET_VALUE(index_column);

#undef GET_VALUE

  if (table_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[table_tokenize] table name is missing");
    return NULL;
  }

  if (string_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[table_tokenize] string is missing");
    return NULL;
  }

  {
    unsigned int flags;
    grn_obj *lexicon;
    grn_obj *index_column = NULL;

    flags = parse_tokenize_flags(ctx, &flags_raw);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }

    lexicon = grn_ctx_get(ctx,
                          table_raw.value,
                          table_raw.length);
    if (!lexicon) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "[table_tokenize] nonexistent lexicon: <%.*s>",
                       (int)table_raw.length,
                       table_raw.value);
      return NULL;
    }

    if (index_column_raw.length > 0) {
      index_column = grn_obj_column(ctx, lexicon,
                                    index_column_raw.value,
                                    index_column_raw.length);
      if (!index_column) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[table_tokenize] nonexistent index column: <%.*s>",
                         (int)index_column_raw.length,
                         index_column_raw.value);
        goto exit;
      }
      if (index_column->header.type != GRN_COLUMN_INDEX) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[table_tokenize] "
                         "index column must be COLUMN_INDEX: <%.*s>",
                         (int)index_column_raw.length,
                         index_column_raw.value);
        goto exit;
      }
    }

    {
      grn_obj tokens;
      GRN_VALUE_FIX_SIZE_INIT(&tokens, GRN_OBJ_VECTOR, GRN_ID_NIL);
      if (mode_raw.length == 0 ||
          GRN_RAW_STRING_EQUAL_CSTRING(mode_raw, "GET")) {
        tokenize(ctx, lexicon, &string_raw, GRN_TOKEN_GET, flags, &tokens);
        output_tokens(ctx, &tokens, lexicon, index_column);
      } else if (GRN_RAW_STRING_EQUAL_CSTRING(mode_raw, "ADD")) {
        tokenize(ctx, lexicon, &string_raw, GRN_TOKEN_ADD, flags, &tokens);
        output_tokens(ctx, &tokens, lexicon, index_column);
      } else {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[table_tokenize] invalid mode: <%.*s>",
                         (int)mode_raw.length,
                         mode_raw.value);
      }
      GRN_OBJ_FIN(ctx, &tokens);
    }
#undef MODE_NAME_EQUAL

exit:
    grn_obj_unlink(ctx, lexicon);
    if (index_column) {
      grn_obj_unlink(ctx, index_column);
    }
  }

  return NULL;
}

void
grn_proc_init_table_tokenize(grn_ctx *ctx)
{
  grn_expr_var vars[5];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "string", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "mode", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "index_column", -1);
  grn_plugin_command_create(ctx,
                            "table_tokenize", -1,
                            command_table_tokenize,
                            5,
                            vars);
}

static grn_obj *
command_tokenize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  const char *context_tag = "[tokenize]";
  grn_raw_string tokenizer_raw;
  grn_raw_string string_raw;
  grn_raw_string normalizer_raw;
  grn_raw_string flags_raw;
  grn_raw_string mode_raw;
  grn_raw_string token_filters_raw;

#define GET_VALUE(name)                                         \
  name ## _raw.value =                                          \
    grn_plugin_proc_get_var_string(ctx,                         \
                                   user_data,                   \
                                   #name,                       \
                                   strlen(#name),               \
                                   &(name ## _raw.length))

  GET_VALUE(tokenizer);
  GET_VALUE(string);
  GET_VALUE(normalizer);
  GET_VALUE(flags);
  GET_VALUE(mode);
  GET_VALUE(token_filters);

#undef GET_VALUE

  if (tokenizer_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s tokenizer name is missing",
                     context_tag);
    return NULL;
  }

  if (string_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s string is missing",
                     context_tag);
    return NULL;
  }

  {
    unsigned int flags;
    grn_obj *lexicon;

    flags = parse_tokenize_flags(ctx, &flags_raw);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }

    lexicon = grn_proc_lexicon_open(ctx,
                                    &tokenizer_raw,
                                    &normalizer_raw,
                                    &token_filters_raw,
                                    context_tag);
    if (!lexicon) {
      return NULL;
    }

    {
      grn_obj tokens;
      GRN_VALUE_FIX_SIZE_INIT(&tokens, GRN_OBJ_VECTOR, GRN_ID_NIL);
      if (mode_raw.length == 0 ||
          GRN_RAW_STRING_EQUAL_CSTRING(mode_raw, "ADD")) {
        tokenize(ctx, lexicon, &string_raw, GRN_TOKEN_ADD, flags, &tokens);
        output_tokens(ctx, &tokens, lexicon, NULL);
      } else if (GRN_RAW_STRING_EQUAL_CSTRING(mode_raw, "GET")) {
        tokenize(ctx, lexicon, &string_raw, GRN_TOKEN_ADD, flags, &tokens);
        GRN_BULK_REWIND(&tokens);
        tokenize(ctx, lexicon, &string_raw, GRN_TOKEN_GET, flags, &tokens);
        output_tokens(ctx, &tokens, lexicon, NULL);
      } else {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "%s invalid mode: <%.*s>",
                         context_tag,
                         (int)mode_raw.length,
                         mode_raw.value);
      }
      GRN_OBJ_FIN(ctx, &tokens);
    }
#undef MODE_NAME_EQUAL

    grn_obj_unlink(ctx, lexicon);
  }

  return NULL;
}

void
grn_proc_init_tokenize(grn_ctx *ctx)
{
  grn_expr_var vars[6];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "tokenizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "string", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "normalizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "mode", -1);
  grn_plugin_expr_var_init(ctx, &(vars[5]), "token_filters", -1);
  grn_plugin_command_create(ctx,
                            "tokenize", -1,
                            command_tokenize,
                            6,
                            vars);
}
