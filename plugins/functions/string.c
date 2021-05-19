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
#include <grn_onigmo.h>

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
#define string_substring_tag "[string_substring]"

  grn_obj *target;
  grn_obj *from_raw;
  grn_obj *length_raw = NULL;
  grn_obj *default_value = NULL;
  grn_obj *options = NULL;
  int64_t from = 0;
  int64_t length = -1;
  const char *start = NULL;
  const char *end = NULL;
  grn_obj *substring = NULL;

  if (n_args < 2 || n_args > 4) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s "
                     "wrong number of arguments (%d for 2..4)",
                     string_substring_tag,
                     n_args);
    return NULL;
  }

  target = args[0];
  from_raw = args[1];

  if (n_args >= 3) {
    grn_obj *length_or_options = args[2];
    if (grn_obj_is_number_family_bulk(ctx, length_or_options)) {
      length_raw = length_or_options;
    } else if (length_or_options->header.type == GRN_TABLE_HASH_KEY) {
      options = length_or_options;
    } else {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, length_or_options);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s "
                       "3rd argument must be a long or a hash table: %.*s",
                       string_substring_tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
  }
  if (n_args == 4) {
    //options type will be checked in grn_proc_options_parse
    options = args[3];
  }

  if (options) {
    grn_rc rc = grn_proc_options_parse(ctx,
                                       options,
                                       string_substring_tag,
                                       "default_value",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &default_value,
                                       NULL);

    if (rc != GRN_SUCCESS) {
      return NULL;
    }

    if (default_value && !grn_obj_is_text_family_bulk(ctx, default_value)) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, default_value);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s[default_value] must be a text bulk: <%.*s>",
                       string_substring_tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
  }

  if (!grn_obj_is_text_family_bulk(ctx, target)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s[target] must be a text bulk: <%.*s>",
                     string_substring_tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  from = grn_plugin_proc_get_value_int64(ctx,
                                         from_raw,
                                         0,
                                         string_substring_tag "[from]");
  length = grn_plugin_proc_get_value_int64(ctx,
                                           length_raw,
                                           -1,
                                           string_substring_tag "[length]");

  if (GRN_TEXT_LEN(target) == 0) {
    goto exit;
  }
  if (length == 0) {
    return grn_plugin_proc_alloc(ctx, user_data, target->header.domain, 0);
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
    substring = grn_plugin_proc_alloc(ctx, user_data, target->header.domain, 0);
    if (!substring) {
      return NULL;
    }
    GRN_TEXT_SET(ctx, substring, start, end - start);
  }

exit:

  if (!substring) {
    if (!default_value) {
      default_value = grn_plugin_proc_alloc(ctx, user_data, target->header.domain, 0);
      if (!default_value) {
        return NULL;
      }
    }
    substring = default_value;
  } else if (GRN_TEXT_LEN(substring) == 0 && default_value) {
    substring = default_value;
  }

  return substring;
#undef string_substring_tag
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

static grn_obj *
string_regexp_slice(grn_ctx *ctx, int n_args, grn_obj **args, grn_user_data *user_data)
{
#ifdef GRN_SUPPORT_REGEXP
#define string_regexp_slice_tag "[string_slice]"

  grn_obj *target_raw, *pattern, *nth_or_name, *default_value = NULL, *result = NULL;

  if (!(n_args == 3 || n_args == 4)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 3...4)",
                     string_regexp_slice_tag,
                     n_args);
    return NULL;
  }

  target_raw = args[0];
  pattern = args[1];
  nth_or_name = args[2];

  if (n_args == 4) {
    grn_obj *options = args[3];
    grn_rc rc = grn_proc_options_parse(ctx,
                                       options,
                                       string_regexp_slice_tag,
                                       "default_value",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &default_value,
                                       NULL);

    if (rc != GRN_SUCCESS) {
      return NULL;
    }

    if (default_value && !grn_obj_is_text_family_bulk(ctx, default_value)) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, default_value);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s[default_value] must be a text bulk: <%.*s>",
                       string_regexp_slice_tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
  }

  if (!grn_obj_is_text_family_bulk(ctx, nth_or_name) && !grn_obj_is_number_family_bulk(ctx, nth_or_name)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, nth_or_name);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s[nth_or_name] must be a text or number bulk: %.*s",
                     string_regexp_slice_tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  //TODO: should cache
  OnigRegex regexp = grn_onigmo_new(ctx,
                                    GRN_TEXT_VALUE(pattern),
                                    GRN_TEXT_LEN(pattern),
                                    GRN_ONIGMO_OPTION_DEFAULT,
                                    GRN_ONIGMO_SYNTAX_DEFAULT,
                                    string_regexp_slice_tag "[regexp]");

  if (!regexp) {
    return NULL;
  }

  const char *target = GRN_TEXT_VALUE(target_raw);
  int64_t target_length = GRN_TEXT_LEN(target_raw);

  OnigRegion region;
  onig_region_init(&region);

  //Cannot use normalized string.
  //The matching parts of the original string cannot be inferred from the matching parts of the normalized string.
  OnigPosition position = onig_search(regexp,
                                      target,
                                      target + target_length,
                                      target,
                                      target + target_length,
                                      &region,
                                      ONIG_OPTION_NONE);

  if (position != ONIG_MISMATCH) {
    int64_t nth = -1;

    if (grn_obj_is_text_family_bulk(ctx, nth_or_name)) {
      const char *name = GRN_TEXT_VALUE(nth_or_name);
      int64_t name_length = GRN_TEXT_LEN(nth_or_name);

      nth = onig_name_to_backref_number(regexp,
                                        name,
                                        name + name_length,
                                        &region);

    } else if (grn_obj_is_number_family_bulk(ctx, nth_or_name)) {
      nth = grn_plugin_proc_get_value_int64(ctx,
                                            nth_or_name,
                                            0,
                                            string_regexp_slice_tag "[nth]");
    }

    if (nth >= 0 && nth < region.num_regs) {
      OnigPosition start = region.beg[nth];
      OnigPosition end = region.end[nth];

      result = grn_plugin_proc_alloc(ctx, user_data, target_raw->header.domain, 0);
      if (!result) {
        goto exit;
      }
      GRN_TEXT_SET(ctx, result, target + start, end - start);
    }
  }

  if (!result) {
    if (!default_value) {
      default_value = grn_plugin_proc_alloc(ctx, user_data, target_raw->header.domain, 0);
      if (!default_value) {
        goto exit;
      }
    }
    result = default_value;
  }

exit:

  onig_region_free(&region, 0);
  onig_free(regexp);

  return result;
#undef string_regexp_slice_tag
#else //GRN_SUPPORT_REGEXP
  return NULL;
#endif //GRN_SUPPORT_REGEXP
}

static grn_obj *
func_string_slice(grn_ctx *ctx, int n_args, grn_obj **args,
                  grn_user_data *user_data)
{
  const char *tag = "[string_slice]";

  if (n_args < 2 || n_args > 4) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s "
                     "wrong number of arguments (%d for 2..4)",
                     tag,
                     n_args);
    return NULL;
  }

  if (grn_obj_is_number_family_bulk(ctx, args[1])) {
    if (n_args >= 3) {
      return func_string_substring(ctx, n_args, args, user_data);
    }

    grn_obj *new_args[3];
    grn_obj third_arg;
    grn_obj *result;

    GRN_INT64_INIT(&third_arg, 0);
    GRN_INT64_SET(ctx, &third_arg, 1);

    new_args[0] = args[0];
    new_args[1] = args[1];
    new_args[2] = &third_arg;

    result = func_string_substring(ctx, 3, new_args, user_data);

    GRN_OBJ_FIN(ctx, &third_arg);

    return result;
  } else if (grn_obj_is_text_family_bulk(ctx, args[1])) {
    return string_regexp_slice(ctx, n_args, args, user_data);
  } else {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, args[1]);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s "
                     "2nd argument must be a text or number bulk: %.*s",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);

    return NULL;
  }
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

  grn_proc_create(ctx, "string_slice", -1,
                  GRN_PROC_FUNCTION,
                  func_string_slice,
                  NULL, NULL, 0, NULL);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
