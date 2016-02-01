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
  switch (target->header.type) {
  case GRN_BULK :
    {
      const char *s = GRN_TEXT_VALUE(target);
      const char *e = GRN_TEXT_VALUE(target) + GRN_TEXT_LEN(target);
      const char *p;
      unsigned int cl = 0;
      for (p = s; p < e && (cl = grn_charlen(ctx, p, e)); p += cl, length++);
    }
    break;
  default :
    {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, target, &inspected);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "string_length(): target object must be bulk: <%.*s>",
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
    break;
  }

  grn_length = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_UINT32, 0);
  if (!grn_length) {
    return NULL;
  }

  GRN_UINT32_SET(ctx, grn_length, length);

  return grn_length;
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

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
