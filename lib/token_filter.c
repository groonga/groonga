/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014 Brazil

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

#include <string.h>

#include "groonga_in.h"
#include <groonga/token_filter.h>

grn_rc
grn_token_filter_register(grn_ctx *ctx,
                          const char *plugin_name_ptr,
                          int plugin_name_length,
                          grn_proc_func *init,
                          grn_proc_func *next,
                          grn_proc_func *fin)
{
  grn_expr_var vars[] = {
    { NULL, 0 },
    { NULL, 0 },
    { NULL, 0 }
  };
  GRN_TEXT_INIT(&vars[0].value, 0);
  GRN_TEXT_INIT(&vars[1].value, 0);
  GRN_UINT32_INIT(&vars[2].value, 0);

  if (plugin_name_length == -1) {
    plugin_name_length = strlen(plugin_name_ptr);
  }
  {
    grn_obj * const obj = grn_proc_create(ctx,
                                          plugin_name_ptr,
                                          plugin_name_length,
                                          GRN_PROC_TOKENIZER,
                                          init, next, fin, 3, vars);
    if (obj == NULL) {
      GRN_PLUGIN_ERROR(ctx, GRN_TOKEN_FILTER_ERROR, "grn_proc_create() failed");
      return ctx->rc;
    }
  }
  return GRN_SUCCESS;
}
