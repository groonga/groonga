/*
  Copyright (C) 2012-2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_token_metadata.h"

grn_rc
grn_token_init(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  GRN_TEXT_INIT(&(token->data), GRN_OBJ_DO_SHALLOW_COPY);
  token->status = GRN_TOKEN_CONTINUE;
  token->source_offset = 0;
  token->source_length = 0;
  token->source_first_character_length = 0;
  token->have_overlap = GRN_FALSE;
  grn_token_metadata_init(ctx, &(token->metadata));
  token->force_prefix_search = GRN_FALSE;
  token->position = 0;
  token->weight = 0;
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_fin(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  grn_token_metadata_fin(ctx, &(token->metadata));
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
    str_length = (int)strlen(str_ptr);
  }
  GRN_TEXT_SET(ctx, &(token->data), str_ptr, str_length);
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_id
grn_token_get_domain(grn_ctx *ctx,grn_token *token)
{
  GRN_API_ENTER;
  grn_id domain = GRN_ID_NIL;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][data][set] token must not be NULL");
    goto exit;
  }
  domain = token->data.header.domain;
exit:
  GRN_API_RETURN(domain);
}

grn_rc
grn_token_set_domain(grn_ctx *ctx,grn_token *token, grn_id domain)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][data][set] token must not be NULL");
    goto exit;
  }
  token->data.header.domain = domain;
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

grn_rc
grn_token_add_status(grn_ctx *ctx,
                     grn_token *token,
                     grn_token_status status)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][status][add] token must not be NULL");
    goto exit;
  }
  token->status |= status;
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_token_remove_status(grn_ctx *ctx,
                        grn_token *token,
                        grn_token_status status)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][status][remove] token must not be NULL");
    goto exit;
  }
  token->status &= ~status;
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

uint32_t
grn_token_get_source_first_character_length(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][source-first-character-length][get] token must not be NULL");
    GRN_API_RETURN(0);
  }
  GRN_API_RETURN(token->source_first_character_length);
}

grn_rc
grn_token_set_source_first_character_length(grn_ctx *ctx,
                                            grn_token *token,
                                            uint32_t length)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][source-first-character-length][set] token must not be NULL");
    goto exit;
  }
  token->source_first_character_length = length;
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
        "[token][overlap][set] token must not be NULL");
    goto exit;
  }
  token->have_overlap = have_overlap;
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_token_get_metadata(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][metadata][get] token must not be NULL");
    GRN_API_RETURN(NULL);
  }
  GRN_API_RETURN(&(token->metadata));
}

bool
grn_token_get_force_prefix_search(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][force-prefix-search][get] token must not be NULL");
    GRN_API_RETURN(false);
  }
  GRN_API_RETURN(token->force_prefix_search);
}

grn_rc
grn_token_set_force_prefix_search(grn_ctx *ctx, grn_token *token, grn_bool force)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][force-prefix-search][set] token must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  token->force_prefix_search = force;
  GRN_API_RETURN(ctx->rc);
}

uint32_t
grn_token_get_position(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][position][get] token must not be NULL");
    GRN_API_RETURN(0);
  }
  GRN_API_RETURN(token->position);
}

grn_rc
grn_token_set_position(grn_ctx *ctx, grn_token *token, uint32_t position)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][position][set] token must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  token->position = position;
  GRN_API_RETURN(ctx->rc);
}

float
grn_token_get_weight(grn_ctx *ctx, grn_token *token)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][weight][get] token must not be NULL");
    GRN_API_RETURN(0.0);
  }
  GRN_API_RETURN(token->weight);
}

grn_rc
grn_token_set_weight(grn_ctx *ctx, grn_token *token, float weight)
{
  GRN_API_ENTER;
  if (!token) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][float][set] token must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  token->weight = weight;
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
  token->source_first_character_length = 0;
  token->have_overlap = GRN_FALSE;
  grn_token_metadata_reset(ctx, &(token->metadata));
  token->force_prefix_search = GRN_FALSE;
  token->position = 0;
  token->weight = 0.0;
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
  token->source_first_character_length = source->source_first_character_length;
  token->have_overlap = source->have_overlap;
  grn_token_metadata_reset(ctx, &(token->metadata));
  grn_token_metadata_copy(ctx, &(token->metadata), &(source->metadata));
  token->force_prefix_search = source->force_prefix_search;
  token->position = source->position;
  token->weight = source->weight;
exit:
  GRN_API_RETURN(ctx->rc);
}
