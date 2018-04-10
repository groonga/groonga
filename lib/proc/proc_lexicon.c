/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil

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
  grn_obj *normalizer = NULL;

  if (normalizer_raw->length > 0) {
    normalizer = grn_ctx_get(ctx,
                             normalizer_raw->value,
                             normalizer_raw->length);
    if (!normalizer) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s nonexistent normalizer: <%.*s>",
                       context_tag,
                       (int)normalizer_raw->length,
                       normalizer_raw->value);
      return NULL;
    }

    if (!grn_obj_is_normalizer_proc(ctx, normalizer)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, normalizer);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s not normalizer: %.*s",
                       context_tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      grn_obj_unlink(ctx, normalizer);
      return NULL;
    }
  }

  lexicon = grn_table_create(ctx, NULL, 0,
                             NULL,
                             GRN_OBJ_TABLE_HASH_KEY,
                             grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                             NULL);
  {
    grn_obj tokenizer;
    GRN_TEXT_INIT(&tokenizer, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET(ctx, &tokenizer, tokenizer_raw->value, tokenizer_raw->length);
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
  if (normalizer) {
    grn_obj_set_info(ctx, lexicon,
                     GRN_INFO_NORMALIZER, normalizer);
    grn_obj_unlink(ctx, normalizer);
  }
  grn_proc_table_set_token_filters(ctx, lexicon, token_filters_raw);

  return lexicon;
}
