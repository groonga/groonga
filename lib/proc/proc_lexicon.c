/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#include "../grn_proc.h"
#include "../grn_ctx.h"

#include <groonga/plugin.h>

grn_obj *
grn_proc_lexicon_open(grn_ctx *ctx,
                      grn_raw_string *tokenizer_raw,
                      grn_raw_string *normalizer_raw,
                      grn_raw_string *token_filters_raw,
                      const char *context_tag)
{
  grn_obj *lexicon;

  lexicon = grn_table_create(ctx, NULL, 0,
                             NULL,
                             GRN_OBJ_TABLE_HASH_KEY,
                             grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                             NULL);
  {
    grn_obj tokenizer;
    GRN_TEXT_INIT(&tokenizer, GRN_OBJ_DO_SHALLOW_COPY);
    if (tokenizer_raw) {
      GRN_TEXT_SET(ctx, &tokenizer, tokenizer_raw->value, tokenizer_raw->length);
    }
    grn_obj_set_info(ctx, lexicon, GRN_INFO_DEFAULT_TOKENIZER, &tokenizer);
    GRN_OBJ_FIN(ctx, &tokenizer);
  }
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj_close(ctx, lexicon);
    GRN_PLUGIN_ERROR(ctx, ctx->rc,
                     "%s failed to set tokenizer: <%.*s>: %s",
                     context_tag,
                     (int)(tokenizer_raw->length),
                     tokenizer_raw->value,
                     ctx->errbuf);
    return NULL;
  }
  {
    grn_obj normalizer;
    GRN_TEXT_INIT(&normalizer, GRN_OBJ_DO_SHALLOW_COPY);
    if (normalizer_raw) {
      GRN_TEXT_SET(ctx,
                   &normalizer,
                   normalizer_raw->value,
                   normalizer_raw->length);
    }
    grn_obj_set_info(ctx, lexicon, GRN_INFO_NORMALIZER, &normalizer);
    GRN_OBJ_FIN(ctx, &normalizer);
  }
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj_close(ctx, lexicon);
    GRN_PLUGIN_ERROR(ctx, ctx->rc,
                     "%s failed to set normalizer: <%.*s>: %s",
                     context_tag,
                     (int)(normalizer_raw->length),
                     normalizer_raw->value,
                     ctx->errbuf);
    return NULL;
  }
  if (token_filters_raw) {
    grn_proc_table_set_token_filters(ctx, lexicon, token_filters_raw);
  }
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj_close(ctx, lexicon);
    GRN_PLUGIN_ERROR(ctx, ctx->rc,
                     "%s failed to set token filters: <%.*s>: %s",
                     context_tag,
                     (int)(token_filters_raw->length),
                     token_filters_raw->value,
                     ctx->errbuf);
    return NULL;
  }

  return lexicon;
}
