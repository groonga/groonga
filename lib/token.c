/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012-2018 Brazil

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

#include "grn_token.h"

grn_rc
grn_token_init(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  GRN_TEXT_INIT(&(token->data), GRN_OBJ_DO_SHALLOW_COPY);
  token->status = GRN_TOKEN_CONTINUE;
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_fin(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  GRN_OBJ_FIN(ctx, &(token->data));
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_token_get_data(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "token must not be NULL");
    GRN_API_RETURN(NULL);
  }
  GRN_API_RETURN(&(token->data));
}

const char *
grn_token_get_data_raw(grn_ctx *ctx, grn_token *token, size_t *length)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "token must not be NULL");
    if (length) {
      *length = 0;
    }
    GRN_API_RETURN(NULL);
  }
  if (length) {
    *length = GRN_TEXT_LEN(&(token->data));
  }
  GRN_API_RETURN(GRN_TEXT_VALUE(&(token->data)));
}

grn_rc
grn_token_set_data(grn_ctx *ctx,
                   grn_token *token,
                   const char *str_ptr,
                   int str_length)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "token must not be NULL");
    goto exit;
  }
  if (str_length == -1) {
    str_length = strlen(str_ptr);
  }
  GRN_TEXT_SET(ctx, &(token->data), str_ptr, str_length);
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_token_status
grn_token_get_status(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "token must not be NULL");
    GRN_API_RETURN(GRN_TOKEN_CONTINUE);
  }
  GRN_API_RETURN(token->status);
}

grn_rc
grn_token_set_status(grn_ctx *ctx,
                     grn_token *token,
                     grn_token_status status)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "token must not be NULL");
    goto exit;
  }
  token->status = status;
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_copy(grn_ctx *ctx,
               grn_token *token,
               grn_token *source)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "[token][copy] token must not be NULL");
    goto exit;
  }
  GRN_TEXT_SET(ctx,
               &(token->data),
               GRN_TEXT_VALUE(&(source->data)),
               GRN_TEXT_LEN(&(source->data)));
  token->status = source->status;
exit:
  GRN_API_RETURN(ctx->rc);
}
