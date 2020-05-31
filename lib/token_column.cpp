/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_token_column.h"
#include "grn_token_cursor.h"

extern "C" {
  void
  grn_token_column_update(grn_ctx *ctx,
                          grn_obj *column,
                          grn_id id,
                          int section,
                          grn_obj *old_value,
                          grn_obj *new_value)
  {
    grn_obj *lexicon = grn_ctx_at(ctx, DB_OBJ(column)->range);
    grn_obj tokens;
    GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, DB_OBJ(lexicon)->id);
    unsigned int token_flags = 0;
    grn_token_cursor *token_cursor =
      grn_token_cursor_open(ctx,
                            lexicon,
                            GRN_TEXT_VALUE(new_value),
                            GRN_TEXT_LEN(new_value),
                            GRN_TOKEN_ADD,
                            token_flags);
    if (token_cursor) {
      while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
        grn_id token_id = grn_token_cursor_next(ctx, token_cursor);
        GRN_RECORD_PUT(ctx, &tokens, token_id);
      }
      grn_token_cursor_close(ctx, token_cursor);
    }
    grn_obj_set_value(ctx, column, id, &tokens, GRN_OBJ_SET);
    GRN_OBJ_FIN(ctx, &tokens);
    grn_obj_unref(ctx, lexicon);
  }

  void
  grn_token_column_build(grn_ctx *ctx, grn_obj *column)
  {
    grn_obj *table = grn_ctx_at(ctx, column->header.domain);
    grn_obj *lexicon = grn_ctx_at(ctx, DB_OBJ(column)->range);
    grn_id *source_ids = static_cast<grn_id *>(DB_OBJ(column)->source);
    grn_obj *source = grn_ctx_at(ctx, source_ids[0]);
    grn_obj_set_visibility(ctx, column, false);
    grn_obj tokens;
    GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, DB_OBJ(lexicon)->id);
    unsigned int token_flags = 0;
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
      GRN_BULK_REWIND(&tokens);
      uint32_t value_size;
      const char *value = grn_obj_get_value_(ctx, source, id, &value_size);
      if (value_size > 0) {
        grn_token_cursor *token_cursor = grn_token_cursor_open(ctx,
                                                               lexicon,
                                                               value,
                                                               value_size,
                                                               GRN_TOKEN_ADD,
                                                               token_flags);
        if (token_cursor) {
          while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
            grn_id token_id = grn_token_cursor_next(ctx, token_cursor);
            GRN_RECORD_PUT(ctx, &tokens, token_id);
          }
          grn_token_cursor_close(ctx, token_cursor);
        }
      }
      grn_obj_set_value(ctx, column, id, &tokens, GRN_OBJ_SET);
    } GRN_TABLE_EACH_END(ctx, cursor);
    grn_obj_set_visibility(ctx, column, true);
    GRN_OBJ_FIN(ctx, &tokens);
    grn_obj_unref(ctx, source);
    grn_obj_unref(ctx, lexicon);
    grn_obj_unref(ctx, table);
  }
}
