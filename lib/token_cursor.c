/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_token_cursor.h"

#include "grn_dat.h"
#include "grn_obj.h"
#include "grn_pat.h"
#include "grn_string.h"
#include "grn_table.h"

static void
grn_token_cursor_open_initialize_token_filters(grn_ctx *ctx,
                                               grn_token_cursor *token_cursor)
{
  grn_obj *token_filters = token_cursor->token_filter.objects;
  unsigned int i, n_token_filters;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);

  token_cursor->token_filter.data = NULL;

  if (token_filters) {
    n_token_filters = GRN_BULK_VSIZE(token_filters) / sizeof(grn_obj *);
  } else {
    n_token_filters = 0;
  }

  if (n_token_filters == 0) {
    return;
  }

  token_cursor->token_filter.data = GRN_CALLOC(sizeof(void *) * n_token_filters);
  if (!token_cursor->token_filter.data) {
    return;
  }

  for (i = 0; i < n_token_filters; i++) {
    grn_obj *token_filter_object = GRN_PTR_VALUE_AT(token_filters, i);
    grn_proc *token_filter = (grn_proc *)token_filter_object;

    if (token_filter->callbacks.token_filter.init_query) {
      grn_tokenizer_query_set_token_filter_index(ctx, query, i);
      token_cursor->token_filter.data[i] =
        token_filter->callbacks.token_filter.init_query(ctx, query);
    } else {
      token_cursor->token_filter.data[i] =
        token_filter->callbacks.token_filter.init(ctx,
                                                  token_cursor->table,
                                                  token_cursor->mode);
    }
  }
}

grn_token_cursor *
grn_token_cursor_open(grn_ctx *ctx, grn_obj *table,
                      const char *str, size_t str_len,
                      grn_tokenize_mode mode,
                      uint32_t flags)
{
  GRN_API_ENTER;

  grn_token_cursor *token_cursor;
  grn_encoding encoding;
  grn_obj *tokenizer;
  grn_obj *token_filters;
  grn_table_flags table_flags;
  if (grn_table_get_info(ctx, table, &table_flags, &encoding, &tokenizer,
                         NULL, &token_filters)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token-cursor][open] failed to get table information");
    GRN_API_RETURN(NULL);
  }
  if (!(token_cursor = GRN_CALLOC(sizeof(grn_token_cursor)))) {
    GRN_API_RETURN(NULL);
  }
  token_cursor->initialized = false;
  token_cursor->table = table;
  token_cursor->mode = mode;
  token_cursor->encoding = encoding;
  token_cursor->flags = flags;
  token_cursor->tokenizer.object = tokenizer;
  grn_tokenizer_query_init(ctx, &(token_cursor->tokenizer.query));
  grn_tokenizer_query_set_lexicon(ctx, &(token_cursor->tokenizer.query), table);
  grn_tokenizer_query_set_flags(ctx, &(token_cursor->tokenizer.query), flags);
  grn_tokenizer_query_set_mode(ctx, &(token_cursor->tokenizer.query), mode);
  grn_token_init(ctx, &(token_cursor->tokenizer.current_token));
  grn_token_init(ctx, &(token_cursor->tokenizer.next_token));
  grn_token_init(ctx, &(token_cursor->tokenizer.original_token));
  token_cursor->token_filter.objects = token_filters;
  token_cursor->token_filter.data = NULL;
  GRN_VOID_INIT(&(token_cursor->original_buffer));
  token_cursor->orig = (const unsigned char *)str;
  token_cursor->orig_blen = str_len;
  token_cursor->curr = NULL;
  token_cursor->curr_size = 0;
  token_cursor->pos = -1;
  token_cursor->status = GRN_TOKEN_CURSOR_DOING;
  token_cursor->pending.status = GRN_TOKEN_CURSOR_DOING;
  token_cursor->pending.token = NULL;
  GRN_API_RETURN(token_cursor);
}

static bool
grn_token_cursor_ensure_initialize(grn_ctx *ctx,
                                   grn_token_cursor *token_cursor)
{
  if (token_cursor->initialized) {
    goto exit;
  }

  token_cursor->initialized = true;

  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);
  if (token_cursor->orig_blen == 0) {
    grn_obj *source_column = grn_tokenizer_query_get_source_column(ctx, query);
    grn_id source_id = grn_tokenizer_query_get_source_id(ctx, query);
    if (source_column && source_id != GRN_ID_NIL) {
      GRN_BULK_REWIND(&(token_cursor->original_buffer));
      grn_obj_get_value(ctx,
                        source_column,
                        source_id,
                        &(token_cursor->original_buffer));
      token_cursor->orig = GRN_BULK_HEAD(&(token_cursor->original_buffer));
      token_cursor->orig_blen = GRN_BULK_VSIZE(&(token_cursor->original_buffer));
    }
  }

  if (token_cursor->tokenizer.object) {
    grn_proc *tokenizer_proc = (grn_proc *)(token_cursor->tokenizer.object);
    if (tokenizer_proc->callbacks.tokenizer.init) {
      token_cursor->tokenizer.user_data = NULL;
      grn_tokenizer_query_set_raw_string(ctx,
                                         query,
                                         token_cursor->orig,
                                         token_cursor->orig_blen);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
      token_cursor->tokenizer.user_data =
        tokenizer_proc->callbacks.tokenizer.init(ctx, query);
    } else if (tokenizer_proc->funcs[PROC_INIT]) {
      grn_obj str_, flags_, mode_;
      GRN_TEXT_INIT(&str_, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_SET_REF(&str_,
                       token_cursor->orig,
                       token_cursor->orig_blen);
      GRN_UINT32_INIT(&flags_, 0);
      GRN_UINT32_SET(ctx, &flags_, token_cursor->flags);
      GRN_UINT32_INIT(&mode_, 0);
      GRN_UINT32_SET(ctx, &mode_, token_cursor->mode);
      token_cursor->tokenizer.pctx.caller = NULL;
      token_cursor->tokenizer.pctx.user_data.ptr = NULL;
      token_cursor->tokenizer.pctx.proc = tokenizer_proc;
      token_cursor->tokenizer.pctx.hooks = NULL;
      token_cursor->tokenizer.pctx.currh = NULL;
      token_cursor->tokenizer.pctx.phase = PROC_INIT;
      grn_ctx_push(ctx, &mode_);
      grn_ctx_push(ctx, &str_);
      grn_ctx_push(ctx, &flags_);
      tokenizer_proc->funcs[PROC_INIT](ctx,
                                       1,
                                       &(token_cursor->table),
                                       &(token_cursor->tokenizer.pctx.user_data));
      grn_obj_close(ctx, &flags_);
      grn_obj_close(ctx, &str_);
      grn_obj_close(ctx, &mode_);
    }
  } else {
    grn_obj *string;
    const char *normalized;

    grn_tokenizer_query_set_raw_string(ctx,
                                       query,
                                       token_cursor->orig,
                                       token_cursor->orig_blen);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    string = grn_tokenizer_query_get_normalized_string(ctx, query);
    grn_string_get_normalized(ctx,
                              string,
                              &normalized,
                              &(token_cursor->curr_size),
                              NULL);
    token_cursor->curr = (const unsigned char *)normalized;
    grn_token_set_data(ctx,
                       &(token_cursor->tokenizer.current_token),
                       token_cursor->curr,
                       token_cursor->curr_size);
    grn_token_set_status(ctx,
                         &(token_cursor->tokenizer.current_token),
                         GRN_TOKEN_LAST);
  }

  if (ctx->rc == GRN_SUCCESS) {
    grn_token_cursor_open_initialize_token_filters(ctx, token_cursor);
  }

exit :
  return ctx->rc == GRN_SUCCESS;
}

grn_rc
grn_token_cursor_set_source_column(grn_ctx *ctx,
                                   grn_token_cursor *token_cursor,
                                   grn_obj *column)
{
  GRN_API_ENTER;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);
  grn_rc rc = grn_tokenizer_query_set_source_column(ctx, query, column);
  GRN_API_RETURN(rc);
}

grn_rc
grn_token_cursor_set_source_id(grn_ctx *ctx,
                               grn_token_cursor *token_cursor,
                               grn_id id)
{
  GRN_API_ENTER;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);
  grn_rc rc = grn_tokenizer_query_set_source_id(ctx, query, id);
  GRN_API_RETURN(rc);
}

grn_rc
grn_token_cursor_set_index_column(grn_ctx *ctx,
                                  grn_token_cursor *token_cursor,
                                  grn_obj *column)
{
  GRN_API_ENTER;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);
  grn_rc rc = grn_tokenizer_query_set_index_column(ctx, query, column);
  GRN_API_RETURN(rc);
}

grn_rc
grn_token_cursor_set_query_options(grn_ctx *ctx,
                                   grn_token_cursor *token_cursor,
                                   grn_obj *query_options)
{
  GRN_API_ENTER;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);
  grn_rc rc = grn_tokenizer_query_set_options(ctx, query, query_options);
  GRN_API_RETURN(rc);
}

static int
grn_token_cursor_next_apply_token_filters(grn_ctx *ctx,
                                          grn_token_cursor *token_cursor)
{
  grn_obj *token_filters = token_cursor->token_filter.objects;
  unsigned int i, n_token_filters;
  grn_token *current_token = &(token_cursor->tokenizer.current_token);
  grn_token *next_token = &(token_cursor->tokenizer.next_token);
  grn_token *original_token = &(token_cursor->tokenizer.original_token);
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);

  if (token_filters) {
    n_token_filters = GRN_BULK_VSIZE(token_filters) / sizeof(grn_obj *);
  } else {
    n_token_filters = 0;
  }

  if (n_token_filters > 0) {
    grn_token_copy(ctx, original_token, current_token);
    grn_token_copy(ctx, next_token, current_token);
    for (i = 0; i < n_token_filters; i++) {
      grn_obj *token_filter_object = GRN_PTR_VALUE_AT(token_filters, i);
      grn_proc *token_filter = (grn_proc *)token_filter_object;
      void *data = token_cursor->token_filter.data[i];

      grn_tokenizer_query_set_token_filter_index(ctx, query, i);

#define SKIP_FLAGS                              \
      (GRN_TOKEN_SKIP |                         \
       GRN_TOKEN_SKIP_WITH_POSITION)
      if (grn_token_get_status(ctx, current_token) & SKIP_FLAGS) {
        break;
      }
#undef SKIP_FLAGS

      token_filter->callbacks.token_filter.filter(ctx,
                                                  current_token,
                                                  next_token,
                                                  data);
      grn_token_copy(ctx, current_token, next_token);
    }
  }

  {
    size_t size;
    token_cursor->curr = grn_token_get_data_raw(ctx, current_token, &size);
    token_cursor->curr_size = size;
  }

  return grn_token_get_status(ctx, current_token);
}

static bool
grn_token_cursor_next_need_keep_original(grn_ctx *ctx,
                                         grn_token_cursor *token_cursor)
{
  grn_token *current_token = &(token_cursor->tokenizer.current_token);
  grn_token *original_token = &(token_cursor->tokenizer.original_token);

  if (!(grn_token_get_status(ctx, current_token) & GRN_TOKEN_KEEP_ORIGINAL)) {
    return false;
  }

  grn_raw_string current_data;
  current_data.value = grn_token_get_data_raw(ctx,
                                              current_token,
                                              &(current_data.length));
  grn_raw_string original_data;
  original_data.value = grn_token_get_data_raw(ctx,
                                               original_token,
                                               &(original_data.length));
  return !GRN_RAW_STRING_EQUAL(current_data, original_data);
}

grn_id
grn_token_cursor_next(grn_ctx *ctx, grn_token_cursor *token_cursor)
{
  GRN_API_ENTER;

  if (!grn_token_cursor_ensure_initialize(ctx, token_cursor)) {
    token_cursor->status = GRN_TOKEN_CURSOR_DONE;
    GRN_API_RETURN(GRN_ID_NIL);
  }

  int status;
  grn_id tid = GRN_ID_NIL;
  grn_obj *table = token_cursor->table;
  grn_obj *tokenizer = token_cursor->tokenizer.object;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);
  grn_token *current_token = &(token_cursor->tokenizer.current_token);
  void *user_data = token_cursor->tokenizer.user_data;
  while (token_cursor->status != GRN_TOKEN_CURSOR_DONE) {
    if (token_cursor->pending.token) {
      token_cursor->status = token_cursor->pending.status;
      grn_token_copy(ctx, current_token, token_cursor->pending.token);
      grn_token_remove_status(ctx,
                              current_token,
                              GRN_TOKEN_KEEP_ORIGINAL);
      size_t size;
      token_cursor->curr = grn_token_get_data_raw(ctx, current_token, &size);
      token_cursor->curr_size = size;
      token_cursor->pending.token = NULL;
      token_cursor->pos--;
    } else if (tokenizer) {
      grn_proc *tokenizer_proc = (grn_proc *)tokenizer;
      grn_token_reset(ctx, current_token);
      if (tokenizer_proc->callbacks.tokenizer.next) {
        tokenizer_proc->callbacks.tokenizer.next(ctx,
                                                 query,
                                                 current_token,
                                                 user_data);
      } else if (tokenizer_proc->funcs[PROC_NEXT]) {
        grn_obj *data, *status;
        tokenizer_proc->funcs[PROC_NEXT](ctx,
                                         1,
                                         &table,
                                         &token_cursor->tokenizer.pctx.user_data);
        status = grn_ctx_pop(ctx);
        data = grn_ctx_pop(ctx);
        grn_token_set_data(ctx,
                           current_token,
                           GRN_TEXT_VALUE(data),
                           GRN_TEXT_LEN(data));
        grn_token_set_status(ctx, current_token, GRN_UINT32_VALUE(status));
      }
      status = grn_token_cursor_next_apply_token_filters(ctx, token_cursor);
      token_cursor->status =
        ((status & GRN_TOKEN_LAST) ||
         (token_cursor->mode == GRN_TOKENIZE_GET &&
          (status & GRN_TOKEN_REACH_END)))
        ? GRN_TOKEN_CURSOR_DONE : GRN_TOKEN_CURSOR_DOING;
#define SKIP_FLAGS \
      (GRN_TOKEN_SKIP | GRN_TOKEN_SKIP_WITH_POSITION)
      if (status & SKIP_FLAGS) {
        if (status & GRN_TOKEN_SKIP) {
          token_cursor->pos++;
        }
        if (token_cursor->status == GRN_TOKEN_CURSOR_DONE && tid == GRN_ID_NIL) {
          token_cursor->status = GRN_TOKEN_CURSOR_DONE_SKIP;
          break;
        } else {
          continue;
        }
      }
#undef SKIP_FLAGS
      if (status & GRN_TOKEN_FORCE_PREFIX) {
        grn_token_set_force_prefix_search(ctx, current_token, GRN_TRUE);
      }
      if (token_cursor->curr_size == 0) {
        if (token_cursor->status != GRN_TOKEN_CURSOR_DONE) {
          char tokenizer_name[GRN_TABLE_MAX_KEY_SIZE];
          int tokenizer_name_length;
          tokenizer_name_length =
            grn_obj_name(ctx, token_cursor->tokenizer.object,
                         tokenizer_name, GRN_TABLE_MAX_KEY_SIZE);
          GRN_LOG(ctx, GRN_WARN,
                  "[token_next] ignore an empty token: <%.*s>: <%.*s>",
                  tokenizer_name_length, tokenizer_name,
                  token_cursor->orig_blen, token_cursor->orig);
        }
        continue;
      }
      if (token_cursor->curr_size > GRN_TABLE_MAX_KEY_SIZE) {
        GRN_LOG(ctx, GRN_WARN,
                "[token_next] ignore too long token. "
                "Token must be less than or equal to %d: <%d>(<%.*s>)",
                GRN_TABLE_MAX_KEY_SIZE,
                token_cursor->curr_size,
                token_cursor->curr_size, token_cursor->curr);
        continue;
      }
      if (status & GRN_TOKEN_UNMATURED) {
        if (status & GRN_TOKEN_OVERLAP) {
          if (token_cursor->mode == GRN_TOKENIZE_GET) {
            token_cursor->pos++;
            continue;
          }
        } else {
          if (status & GRN_TOKEN_REACH_END) {
            grn_token_set_force_prefix_search(ctx, current_token, GRN_TRUE);
          }
        }
      }
    } else {
      grn_token_cursor_next_apply_token_filters(ctx, token_cursor);
      token_cursor->status = GRN_TOKEN_CURSOR_DONE;
    }
    if (token_cursor->mode == GRN_TOKENIZE_ADD) {
      switch (table->header.type) {
      case GRN_TABLE_PAT_KEY :
        if (token_cursor->flags & GRN_TOKEN_CURSOR_PARALLEL) {
          tid = grn_pat_get(ctx,
                            (grn_pat *)table,
                            token_cursor->curr,
                            token_cursor->curr_size,
                            NULL);
        }
        if (tid == GRN_ID_NIL) {
          GRN_TABLE_LOCK_BEGIN(ctx, table) {
            tid = grn_pat_add(ctx,
                              (grn_pat *)table,
                              token_cursor->curr,
                              token_cursor->curr_size,
                              NULL,
                              NULL);
          } GRN_TABLE_LOCK_END(ctx);
        }
        break;
      case GRN_TABLE_DAT_KEY :
        if (token_cursor->flags & GRN_TOKEN_CURSOR_PARALLEL) {
          tid = grn_dat_get(ctx,
                            (grn_dat *)table,
                            token_cursor->curr,
                            token_cursor->curr_size,
                            NULL);
        }
        if (tid == GRN_ID_NIL) {
          GRN_TABLE_LOCK_BEGIN(ctx, table) {
            tid = grn_dat_add(ctx,
                              (grn_dat *)table,
                              token_cursor->curr,
                              token_cursor->curr_size,
                              NULL,
                              NULL);
          } GRN_TABLE_LOCK_END(ctx);
        }
        break;
      case GRN_TABLE_HASH_KEY :
        if (token_cursor->flags & GRN_TOKEN_CURSOR_PARALLEL) {
          tid = grn_hash_get(ctx,
                             (grn_hash *)table,
                             token_cursor->curr,
                             token_cursor->curr_size,
                             NULL);
        }
        GRN_TABLE_LOCK_BEGIN(ctx, table) {
          tid = grn_hash_add(ctx,
                             (grn_hash *)table,
                             token_cursor->curr,
                             token_cursor->curr_size,
                             NULL,
                             NULL);
        } GRN_TABLE_LOCK_END(ctx);
        break;
      case GRN_TABLE_NO_KEY :
        if (token_cursor->curr_size == sizeof(grn_id)) {
          tid = *((grn_id *)token_cursor->curr);
        } else {
          tid = GRN_ID_NIL;
        }
        break;
      }
    } else if (token_cursor->mode != GRN_TOKENIZE_ONLY) {
      switch (table->header.type) {
      case GRN_TABLE_PAT_KEY :
        tid = grn_pat_get(ctx, (grn_pat *)table, token_cursor->curr, token_cursor->curr_size, NULL);
        break;
      case GRN_TABLE_DAT_KEY :
        tid = grn_dat_get(ctx, (grn_dat *)table, token_cursor->curr, token_cursor->curr_size, NULL);
        break;
      case GRN_TABLE_HASH_KEY :
        tid = grn_hash_get(ctx, (grn_hash *)table, token_cursor->curr, token_cursor->curr_size, NULL);
        break;
      case GRN_TABLE_NO_KEY :
        if (token_cursor->curr_size == sizeof(grn_id)) {
          tid = *((grn_id *)token_cursor->curr);
        } else {
          tid = GRN_ID_NIL;
        }
        break;
      }
    }
    if (token_cursor->mode != GRN_TOKENIZE_ONLY &&
        tid == GRN_ID_NIL && token_cursor->status != GRN_TOKEN_CURSOR_DONE) {
      token_cursor->status = GRN_TOKEN_CURSOR_NOT_FOUND;
    }
    if (grn_token_cursor_next_need_keep_original(ctx, token_cursor)) {
      token_cursor->pending.status = token_cursor->status;
      token_cursor->status = GRN_TOKEN_CURSOR_DOING;
      token_cursor->pending.token = &(token_cursor->tokenizer.original_token);
    }
    token_cursor->pos++;
    grn_token_set_position(ctx, current_token, token_cursor->pos);
    break;
  }
  GRN_API_RETURN(tid);
}

grn_token_cursor_status
grn_token_cursor_get_status(grn_ctx *ctx, grn_token_cursor *token_cursor)
{
  GRN_API_ENTER;
  if (!token_cursor) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token-cursor][status] token cursor must not NULL");
    GRN_API_RETURN(GRN_TOKEN_CURSOR_DONE);
  }
  GRN_API_RETURN(token_cursor->status);
}

static void
grn_token_cursor_close_token_filters(grn_ctx *ctx,
                                     grn_token_cursor *token_cursor)
{
  grn_obj *token_filters = token_cursor->token_filter.objects;
  unsigned int i, n_token_filters;
  grn_tokenizer_query *query = &(token_cursor->tokenizer.query);

  if (!token_cursor->token_filter.data) {
    return;
  }

  if (token_filters) {
    n_token_filters = GRN_BULK_VSIZE(token_filters) / sizeof(grn_obj *);
  } else {
    n_token_filters = 0;
  }

  if (n_token_filters == 0) {
    return;
  }

  for (i = 0; i < n_token_filters; i++) {
    grn_obj *token_filter_object = GRN_PTR_VALUE_AT(token_filters, i);
    grn_proc *token_filter = (grn_proc *)token_filter_object;
    void *data = token_cursor->token_filter.data[i];

    grn_tokenizer_query_set_token_filter_index(ctx, query, i);
    token_filter->callbacks.token_filter.fin(ctx, data);
  }
  GRN_FREE(token_cursor->token_filter.data);
}

grn_rc
grn_token_cursor_close(grn_ctx *ctx, grn_token_cursor *token_cursor)
{
  GRN_API_ENTER;
  if (!token_cursor) {
    GRN_API_RETURN(GRN_INVALID_ARGUMENT);
  }

  if (token_cursor->tokenizer.object && token_cursor->initialized) {
    grn_proc *tokenizer_proc = (grn_proc *)(token_cursor->tokenizer.object);
    if (tokenizer_proc->callbacks.tokenizer.fin) {
      void *user_data = token_cursor->tokenizer.user_data;
      if (user_data) {
        tokenizer_proc->callbacks.tokenizer.fin(ctx, user_data);
      }
    } else if (tokenizer_proc->funcs[PROC_FIN]) {
      tokenizer_proc->funcs[PROC_FIN](ctx,
                                      1,
                                      &token_cursor->table,
                                      &token_cursor->tokenizer.pctx.user_data);
    }
  }
  grn_token_fin(ctx, &(token_cursor->tokenizer.current_token));
  grn_token_fin(ctx, &(token_cursor->tokenizer.next_token));
  grn_token_fin(ctx, &(token_cursor->tokenizer.original_token));
  grn_tokenizer_query_fin(ctx, &(token_cursor->tokenizer.query));
  grn_token_cursor_close_token_filters(ctx, token_cursor);
  GRN_OBJ_FIN(ctx, &(token_cursor->original_buffer));
  GRN_FREE(token_cursor);
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_token *
grn_token_cursor_get_token(grn_ctx *ctx, grn_token_cursor *token_cursor)
{
  GRN_API_ENTER;
  GRN_API_RETURN(&(token_cursor->tokenizer.current_token));
}
