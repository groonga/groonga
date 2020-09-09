/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016  Brazil
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_string
#endif

#include <groonga/plugin.h>

/*
 * func_string_length() returns the number of characters in a string.
 * If the string contains an invalid byte sequence, this function returns the
 * number of characters before the invalid byte sequence.
 */
static grn_obj *
func_string_length(grn_ctx *ctx, int n_args, grn_obj **args,
                   grn_user_data *user_data)
{
  grn_obj *target;
  unsigned int length = 0;
  grn_obj *grn_length;

  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "string_length(): wrong number of arguments (%d for 1)",
                     n_args);
    return NULL;
  }

  target = args[0];
  if (!(target->header.type == GRN_BULK &&
        ((target->header.domain == GRN_DB_SHORT_TEXT) ||
         (target->header.domain == GRN_DB_TEXT) ||
         (target->header.domain == GRN_DB_LONG_TEXT)))) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "string_length(): target object must be a text bulk: "
                     "<%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  {
    const char *s = GRN_TEXT_VALUE(target);
    const char *e = GRN_TEXT_VALUE(target) + GRN_TEXT_LEN(target);
    const char *p;
    unsigned int cl = 0;
    for (p = s; p < e && (cl = grn_charlen(ctx, p, e)); p += cl) {
      length++;
    }
  }

  grn_length = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_UINT32, 0);
  if (!grn_length) {
    return NULL;
  }

  GRN_UINT32_SET(ctx, grn_length, length);

  return grn_length;
}

static grn_obj *
func_string_substring(grn_ctx *ctx, int n_args, grn_obj **args,
                      grn_user_data *user_data)
{
  grn_obj *target;
  grn_obj *from_raw;
  grn_obj *length_raw = NULL;
  int64_t from = 0;
  int64_t length = -1;
  const char *start = NULL;
  const char *end = NULL;
  grn_obj *substring;

  if (!(n_args == 2 || n_args == 3)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[string_substring] "
                     "wrong number of arguments (%d for 2..3)",
                     n_args);
    return NULL;
  }

  target = args[0];
  from_raw = args[1];
  if (n_args == 3) {
    length_raw = args[2];
  }

  if (!grn_obj_is_text_family_bulk(ctx, target)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[string_substring][target] must be a text bulk: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  from = grn_plugin_proc_get_value_int64(ctx,
                                         from_raw,
                                         0,
                                         "[string_substring][from]");
  length = grn_plugin_proc_get_value_int64(ctx,
                                           length_raw,
                                           -1,
                                           "[string_substring][length]");

  substring = grn_plugin_proc_alloc(ctx, user_data, target->header.domain, 0);
  if (!substring) {
    return NULL;
  }

  GRN_BULK_REWIND(substring);

  if (GRN_TEXT_LEN(target) == 0) {
    return substring;
  }
  if (length == 0) {
    return substring;
  }

  while (from < 0) {
    from += GRN_TEXT_LEN(target);
  }

  {
    const char *p;

    start = NULL;
    p = GRN_TEXT_VALUE(target);
    end = p + GRN_TEXT_LEN(target);

    if (from == 0) {
      start = p;
    } else {
      unsigned int char_length = 0;
      size_t n_chars = 0;

      for (;
           p < end && (char_length = grn_charlen(ctx, p, end));
           p += char_length, n_chars++) {
        if (n_chars == from) {
          start = p;
          break;
        }
      }
    }

    if (start && length > 0) {
      unsigned int char_length = 0;
      size_t n_chars = 0;

      for (;
           p < end && (char_length = grn_charlen(ctx, p, end));
           p += char_length, n_chars++) {
        if (n_chars == length) {
          end = p;
          break;
        }
      }
    }
  }

  if (start) {
    GRN_TEXT_SET(ctx, substring, start, end - start);
  }

  return substring;
}

static grn_obj *
func_string_tokenize(grn_ctx *ctx, int n_args, grn_obj **args,
                     grn_user_data *user_data)
{
  grn_obj *target;
  grn_obj *lexicon;
  grn_obj *options = NULL;

  if (!(n_args == 2 || n_args == 3)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[string_tokenize] wrong number of arguments (%d for 2..3)",
                     n_args);
    return NULL;
  }

  target = args[0];
  lexicon = args[1];
  if (n_args == 3) {
    options = args[2];
  }

  if (!grn_obj_is_text_family_bulk(ctx, target)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[string_tokenize][target] must be a text bulk: %.*s",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  if (!grn_obj_is_table_with_key(ctx, lexicon)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, lexicon);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[string_tokenize][lexicon] must be a table with key: %.*s",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  grn_tokenize_mode mode = GRN_TOKEN_GET;
  uint32_t flags = 0;
  if (options) {
    grn_rc rc = grn_proc_options_parse(ctx,
                                       options,
                                       "[string_tokenize]",
                                       "mode",
                                       GRN_PROC_OPTION_VALUE_TOKENIZE_MODE,
                                       &mode,
                                       "flags",
                                       GRN_PROC_OPTION_VALUE_TOKEN_CURSOR_FLAGS,
                                       &flags,
                                       NULL);
    if (rc != GRN_SUCCESS) {
      return NULL;
    }
  }

  grn_obj *tokens = grn_plugin_proc_alloc(ctx,
                                          user_data,
                                          grn_obj_id(ctx, lexicon),
                                          GRN_OBJ_VECTOR);
  if (!tokens) {
    return NULL;
  }
  tokens->header.flags |= GRN_OBJ_WITH_WEIGHT;

  grn_token_cursor *token_cursor;
  token_cursor = grn_token_cursor_open(ctx,
                                       lexicon,
                                       GRN_TEXT_VALUE(target),
                                       GRN_TEXT_LEN(target),
                                       mode,
                                       flags);
  if (!token_cursor) {
    return tokens;
  }
  while (grn_token_cursor_get_status(ctx, token_cursor) ==
         GRN_TOKEN_CURSOR_DOING) {
    grn_id token_id = grn_token_cursor_next(ctx, token_cursor);

    if (token_id == GRN_ID_NIL) {
      continue;
    }

    grn_token *token = grn_token_cursor_get_token(ctx, token_cursor);
    float weight = grn_token_get_weight(ctx, token);
    grn_uvector_add_element_record(ctx, tokens, token_id, weight);
  }
  grn_token_cursor_close(ctx, token_cursor);

  return tokens;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  grn_proc_create(ctx, "string_length", -1,
                  GRN_PROC_FUNCTION,
                  func_string_length,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "string_substring", -1,
                  GRN_PROC_FUNCTION,
                  func_string_substring,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "string_tokenize", -1,
                  GRN_PROC_FUNCTION,
                  func_string_tokenize,
                  NULL, NULL, 0, NULL);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
