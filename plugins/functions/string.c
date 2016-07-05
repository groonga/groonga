/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016 Brazil

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
  size_t string_length = 0;
  unsigned long long from = 0;
  unsigned long long length = 0;
  const char *start;
  const char *end;
  grn_obj *grn_text;

  if (n_args < 2) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "string_substring(): wrong number of arguments (%d for 2..)",
                     n_args);
    return NULL;
  }

  target = args[0];
  from = GRN_UINT64_VALUE(args[1]);

  if (n_args == 3) {
    length = GRN_UINT64_VALUE(args[2]);
  }

  if (!(target->header.type == GRN_BULK &&
        ((target->header.domain == GRN_DB_SHORT_TEXT) ||
         (target->header.domain == GRN_DB_TEXT) ||
         (target->header.domain == GRN_DB_LONG_TEXT)))) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "string_substring(): target object must be a text bulk: "
                     "<%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  {
    const char *p;
    unsigned int cl = 0;
    start = GRN_TEXT_VALUE(target);
    end = GRN_BULK_CURR(target);
    for (p = start; p < end && (cl = grn_charlen(ctx, p, end)); p += cl) {
      if (string_length == from) {
        start = p;
      }
      if (length > 0 && string_length - from == length) {
        end = p;
        break;
      }
      string_length++;
    }
  }

  grn_text = grn_plugin_proc_alloc(ctx, user_data, target->header.domain, 0);
  if (!grn_text) {
    return NULL;
  }

  GRN_TEXT_SET(ctx, grn_text, start, end - start);

  return grn_text;
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

  grn_proc_create(ctx, "string_length", -1, GRN_PROC_FUNCTION, func_string_length,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "string_substring", -1, GRN_PROC_FUNCTION, func_string_substring,
                  NULL, NULL, 0, NULL);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
