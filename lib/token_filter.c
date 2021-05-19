/*
  Copyright(C) 2014-2015 Brazil

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

#include "grn.h"
#include "grn_db.h"
#include <groonga/token_filter.h>

grn_rc
grn_token_filter_register(grn_ctx *ctx,
                          const char *plugin_name_ptr,
                          int plugin_name_length,
                          grn_token_filter_init_func *init,
                          grn_token_filter_filter_func *filter,
                          grn_token_filter_fin_func *fin)
{
  if (plugin_name_length == -1) {
    plugin_name_length = strlen(plugin_name_ptr);
  }

  {
    grn_obj *token_filter_object = grn_proc_create(ctx,
                                                   plugin_name_ptr,
                                                   plugin_name_length,
                                                   GRN_PROC_TOKEN_FILTER,
                                                   NULL, NULL, NULL, 0, NULL);
    if (token_filter_object == NULL) {
      GRN_PLUGIN_ERROR(ctx, GRN_TOKEN_FILTER_ERROR,
                       "[token-filter][%.*s] failed to grn_proc_create()",
                       plugin_name_length, plugin_name_ptr);
      return ctx->rc;
    }

    {
      grn_proc *token_filter = (grn_proc *)token_filter_object;
      token_filter->callbacks.token_filter.init = init;
      token_filter->callbacks.token_filter.filter = filter;
      token_filter->callbacks.token_filter.fin = fin;
    }
  }

  return GRN_SUCCESS;
}

grn_obj *
grn_token_filter_create(grn_ctx *ctx, const char *name, int length)
{
  grn_obj *token_filter;

  GRN_API_ENTER;
  token_filter = grn_proc_create(ctx,
                                 name,
                                 length,
                                 GRN_PROC_TOKEN_FILTER,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL);
  if (!token_filter) {
    if (length < 0) {
      length = strlen(name);
    }
    GRN_PLUGIN_ERROR(ctx,
                     GRN_TOKEN_FILTER_ERROR,
                     "[token-filter][create] failed to create: <%.*s>",
                     length, name);
  }

  GRN_API_RETURN(token_filter);
}

grn_rc
grn_token_filter_set_init_func(grn_ctx *ctx,
                               grn_obj *token_filter,
                               grn_token_filter_init_query_func *init)
{
  GRN_API_ENTER;
  if (token_filter) {
    ((grn_proc *)token_filter)->callbacks.token_filter.init_query = init;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[token-filter][init][set] token filter is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_filter_set_filter_func(grn_ctx *ctx,
                                 grn_obj *token_filter,
                                 grn_token_filter_filter_func *filter)
{
  GRN_API_ENTER;
  if (token_filter) {
    ((grn_proc *)token_filter)->callbacks.token_filter.filter = filter;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[token-filter][filter][set] token filter is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_filter_set_fin_func(grn_ctx *ctx,
                              grn_obj *token_filter,
                              grn_token_filter_fin_func *fin)
{
  GRN_API_ENTER;
  if (token_filter) {
    ((grn_proc *)token_filter)->callbacks.token_filter.fin = fin;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[token-filter][fin][set] token filter is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}
