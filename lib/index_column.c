/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2015 Brazil
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

#include "grn_index_column.h"
#include "grn_ii.h"
#include "grn_hash.h"

#include <string.h>

static uint64_t grn_index_sparsity = 10;
static grn_bool grn_index_chunk_split_enable = GRN_TRUE;

void
grn_index_column_init_from_env(void)
{
  {
    char grn_index_sparsity_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_INDEX_SPARSITY",
               grn_index_sparsity_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_index_sparsity_env[0]) {
      uint64_t sparsity;
      errno = 0;
      sparsity = strtoull(grn_index_sparsity_env, NULL, 0);
      if (errno == 0) {
        grn_index_sparsity = sparsity;
      }
    }
  }

  {
    char grn_index_chunk_split_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_INDEX_CHUNK_SPLIT_ENABLE",
               grn_index_chunk_split_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_index_chunk_split_enable_env, "no") == 0) {
      grn_index_chunk_split_enable = GRN_FALSE;
    } else {
      grn_index_chunk_split_enable = GRN_TRUE;
    }
  }
}

grn_inline static void
grn_index_column_build_call_hook(grn_ctx *ctx,
                                 grn_obj *obj,
                                 grn_id id,
                                 grn_obj *old_value,
                                 grn_obj *value,
                                 int flags)
{
  grn_hook *hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET];

  if (hooks) {
    /* todo : grn_proc_ctx_open() */
    grn_obj id_, flags_;
    grn_proc_ctx pctx = {{0}, hooks->proc, NULL, hooks, hooks, PROC_INIT, 4, 4};
    GRN_UINT32_INIT(&id_, 0);
    GRN_UINT32_INIT(&flags_, 0);
    GRN_UINT32_SET(ctx, &id_, id);
    GRN_UINT32_SET(ctx, &flags_, flags);
    while (hooks) {
      grn_ctx_push(ctx, &id_);
      grn_ctx_push(ctx, old_value);
      grn_ctx_push(ctx, value);
      grn_ctx_push(ctx, &flags_);
      pctx.caller = NULL;
      pctx.currh = hooks;
      if (hooks->proc) {
        hooks->proc->funcs[PROC_INIT](ctx, 1, &obj, &pctx.user_data);
      } else {
        grn_obj_default_set_value_hook(ctx, 1, &obj, &pctx.user_data);
      }
      if (ctx->rc) {
        return;
      }
      hooks = hooks->next;
      pctx.offset++;
    }
  }
}

static void
grn_index_column_build_column(grn_ctx *ctx,
                              grn_obj *index_column,
                              grn_obj *table,
                              grn_obj *column)
{
  grn_obj old_value;
  grn_obj value;
  int cursor_flags = GRN_CURSOR_BY_ID;

  GRN_VOID_INIT(&old_value);
  grn_obj_reinit_for(ctx, &old_value, column);
  GRN_VOID_INIT(&value);
  grn_obj_reinit_for(ctx, &value, column);
  if (GRN_OBJ_TABLEP(column)) {
    GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                               table,
                               cursor,
                               id,
                               cursor_flags) {
      GRN_BULK_REWIND(&value);
      grn_table_get_key2(ctx, column, id, &value);
      grn_index_column_build_call_hook(ctx, column, id, &old_value, &value, 0);
    } GRN_TABLE_EACH_END(ctx, cursor);
  } else {
    GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                               table,
                               cursor,
                               id,
                               cursor_flags) {
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, column, id, &value);
      grn_index_column_build_call_hook(ctx, column, id, &old_value, &value, 0);
    } GRN_TABLE_EACH_END(ctx, cursor);
  }
  GRN_OBJ_FIN(ctx, &old_value);
  GRN_OBJ_FIN(ctx, &value);
}

grn_rc
grn_index_column_build(grn_ctx *ctx, grn_obj *index_column)
{
  grn_obj *src, **cp, **col, *target;
  grn_id *s = DB_OBJ(index_column)->source;
  if (!(DB_OBJ(index_column)->source_size) || !s) { return ctx->rc; }
  if ((src = grn_ctx_at(ctx, *s))) {
    target = GRN_OBJ_TABLEP(src) ? src : grn_ctx_at(ctx, src->header.domain);
    if (target) {
      int i, ncol = DB_OBJ(index_column)->source_size / sizeof(grn_id);
      grn_table_flags flags;
      grn_ii *ii = (grn_ii *)index_column;
      grn_bool use_grn_ii_build;
      grn_obj *tokenizer = NULL;
      grn_table_get_info(ctx, ii->lexicon, &flags, NULL, &tokenizer, NULL, NULL);
      switch (flags & GRN_OBJ_TABLE_TYPE_MASK) {
      case GRN_OBJ_TABLE_PAT_KEY :
      case GRN_OBJ_TABLE_DAT_KEY :
        use_grn_ii_build = GRN_TRUE;
        break;
      default :
        use_grn_ii_build = GRN_FALSE;
        break;
      }
      if ((ii->header->flags & GRN_OBJ_WITH_WEIGHT)) {
        use_grn_ii_build = GRN_FALSE;
      }
      if ((ii->header->flags & GRN_OBJ_WITH_POSITION) &&
          (!tokenizer &&
           !GRN_TYPE_IS_TEXT_FAMILY(ii->lexicon->header.domain))) {
        /* TODO: Support offline index construction for WITH_POSITION
         * index against UInt32 vector column. */
        use_grn_ii_build = GRN_FALSE;
      }
      if ((col = GRN_MALLOC(ncol * sizeof(grn_obj *)))) {
        for (cp = col, i = ncol; i; s++, cp++, i--) {
          if (!(*cp = grn_ctx_at(ctx, *s))) {
            ERR(GRN_INVALID_ARGUMENT, "source invalid, n=%d",i);
            GRN_FREE(col);
            return ctx->rc;
          }
          if (GRN_OBJ_TABLEP(grn_ctx_at(ctx, DB_OBJ(*cp)->range))) {
            use_grn_ii_build = GRN_FALSE;
          }
        }
        if (use_grn_ii_build) {
          if (grn_index_chunk_split_enable) {
            grn_ii_build2(ctx, ii, NULL);
          } else {
            grn_ii_build(ctx, ii, grn_index_sparsity);
          }
        } else {
          for (i = 0; i < ncol; i++) {
            grn_obj *column = col[i];
            grn_index_column_build_column(ctx, index_column, target, column);
          }
        }
        GRN_FREE(col);
        grn_obj_touch(ctx, index_column, NULL);
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid target");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid source");
  }
  return ctx->rc;
}

grn_rc
grn_index_column_rebuild(grn_ctx *ctx, grn_obj *index_column)
{
  grn_ii *ii = (grn_ii *)index_column;

  GRN_API_ENTER;

  grn_ii_truncate(ctx, ii);
  grn_index_column_build(ctx, index_column);

  GRN_API_RETURN(ctx->rc);
}
