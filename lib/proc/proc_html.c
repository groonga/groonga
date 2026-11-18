/*
  Copyright (C) 2026  Abe Tomoaki <abe@clear-code.com>

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

#include "../grn_proc.h"
#include "../grn_extractors.h"

#include <groonga/plugin.h>

static grn_obj *
func_html_extract(grn_ctx *ctx,
                  int n_args,
                  grn_obj **args,
                  grn_user_data *user_data)
{
  if (!(n_args == 1 || n_args == 2)) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "html_extract(): wrong number of arguments (%d for 1..2)",
                     n_args);
    return NULL;
  }

  grn_obj *html = args[0];
  if (!grn_obj_is_text_family_bulk(ctx, html)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, html);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "html_extract(): the first argument must be "
                     "an HTML string: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  grn_raw_string selector = {NULL, 0};
  if (n_args == 2) {
    grn_rc rc = grn_proc_options_parse(ctx,
                                       args[1],
                                       "[html-extract]",
                                       "selector",
                                       GRN_PROC_OPTION_VALUE_RAW_STRING,
                                       &selector,
                                       NULL);
    if (rc != GRN_SUCCESS) {
      return NULL;
    }
  }

  grn_obj *text = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
  if (!text) {
    return NULL;
  }
  grn_html_extract(ctx,
                   GRN_TEXT_VALUE(html),
                   GRN_TEXT_LEN(html),
                   selector.value,
                   selector.length,
                   text);
  return text;
}

void
grn_proc_init_html_extract(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "html_extract",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_html_extract,
                  NULL,
                  NULL,
                  0,
                  NULL);
}
