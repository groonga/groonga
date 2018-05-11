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
  token->source_offset = 0;
  token->source_length = 0;
  token->have_overlap = GRN_FALSE;
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
    ERR(GRN_INVALID_ARGUMENT,
        "[token][data][get] token must not be NULL");
    GRN_API_RETURN(NULL);
  }
  GRN_API_RETURN(&(token->data));
}

const char *
grn_token_get_data_raw(grn_ctx *ctx, grn_token *token, size_t *length)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][data][get][raw] token must not be NULL");
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
    ERR(GRN_INVALID_ARGUMENT,
        "[token][data][set] token must not be NULL");
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
    ERR(GRN_INVALID_ARGUMENT,
        "[token][status][get] token must not be NULL");
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
    ERR(GRN_INVALID_ARGUMENT,
        "[token][status][set] token must not be NULL");
    goto exit;
  }
  token->status = status;
exit:
  GRN_API_RETURN(ctx->rc);
}

uint64_t
grn_token_get_source_offset(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][source-offset][get] token must not be NULL");
    GRN_API_RETURN(0);
  }
  GRN_API_RETURN(token->source_offset);
}

grn_rc
grn_token_set_source_offset(grn_ctx *ctx,
                            grn_token *token,
                            uint64_t offset)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][source-offset][set] token must not be NULL");
    goto exit;
  }
  token->source_offset = offset;
exit:
  GRN_API_RETURN(ctx->rc);
}

uint32_t
grn_token_get_source_length(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][source-length][get] token must not be NULL");
    GRN_API_RETURN(0);
  }
  GRN_API_RETURN(token->source_length);
}

grn_rc
grn_token_set_source_length(grn_ctx *ctx,
                            grn_token *token,
                            uint32_t length)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][source-length][set] token must not be NULL");
    goto exit;
  }
  token->source_length = length;
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_bool
grn_token_have_overlap(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][overlap][have] token must not be NULL");
    GRN_API_RETURN(0);
  }
  GRN_API_RETURN(token->have_overlap);
}

grn_rc
grn_token_set_overlap(grn_ctx *ctx,
                      grn_token *token,
                      grn_bool have_overlap)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][overlapping][set] token must not be NULL");
    goto exit;
  }
  token->have_overlap = have_overlap;
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_reset(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT, "[token][reset] token must not be NULL");
    goto exit;
  }
  GRN_BULK_REWIND(&(token->data));
  token->status = GRN_TOKEN_CONTINUE;
  token->source_offset = 0;
  token->source_length = 0;
  token->have_overlap = GRN_FALSE;
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
  token->source_offset = source->source_offset;
  token->source_length = source->source_length;
  token->have_overlap = source->have_overlap;
exit:
  GRN_API_RETURN(ctx->rc);
}
