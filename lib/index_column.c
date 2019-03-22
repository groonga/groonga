/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2015 Brazil
  Copyright(C) 2018-2019 Kouhei Sutou <kou@clear-code.com>

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
#include <math.h>

static uint64_t grn_index_sparsity = 10;
static grn_bool grn_index_chunk_split_enable = GRN_TRUE;
static size_t grn_index_column_diff_cache_size_max = 256;

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

  {
    char grn_index_column_diff_cache_size_max_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_INDEX_COLUMN_DIFF_CACHE_SIZE_MAX",
               grn_index_column_diff_cache_size_max_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_index_column_diff_cache_size_max_env[0]) {
      uint64_t size;
      errno = 0;
      size = strtoull(grn_index_column_diff_cache_size_max_env, NULL, 0);
      if (errno == 0 && size > 0) {
        grn_index_column_diff_cache_size_max = size;
      }
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
    grn_column_cache *cache;
    cache = grn_column_cache_open(ctx, column);
    if (cache) {
      GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                                 table,
                                 cursor,
                                 id,
                                 cursor_flags) {
        void *raw_value;
        size_t value_size;
        GRN_BULK_REWIND(&value);
        raw_value = grn_column_cache_ref(ctx, cache, id, &value_size);
        grn_bulk_write(ctx, &value, raw_value, value_size);
        grn_index_column_build_call_hook(ctx, column, id, &old_value, &value, 0);
      } GRN_TABLE_EACH_END(ctx, cursor);
      grn_column_cache_close(ctx, cache);
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

static const char *postings_column_name = "postings";
static const char *remains_column_name = "remains";
static const char *missings_column_name = "missings";

typedef struct {
  grn_id token_id;
  grn_id diff_id;
  grn_obj postings;
  size_t offset;
} grn_index_column_diff_cache;

typedef struct {
  grn_obj *lexicon;
  grn_ii *ii;
  struct {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    grn_bool with_section;
    grn_bool with_position;
    uint32_t n_elements;
  } index;
  size_t n_posting_elements;
  grn_obj *source_table;
  grn_obj source_columns;
  grn_obj *diff;
  grn_obj *postings;
  grn_obj *remains;
  grn_obj *missings;
  struct {
    grn_id token_id;
    grn_id diff_id;
    grn_posting posting;
    grn_bool is_new_diff;
  } current;
  struct {
    grn_obj value;
    grn_obj postings;
  } buffers;
  struct {
    unsigned int n_records;
    unsigned int i;
    unsigned int interval;
    int n_records_digits;
    grn_log_level log_level;
    grn_timeval start_time;
    grn_timeval previous_time;
  } progress;
  grn_index_column_diff_cache *cache;
  size_t cache_size;
} grn_index_column_diff_data;

static void
grn_index_column_diff_data_init(grn_ctx *ctx,
                                grn_index_column_diff_data *data)
{
  GRN_PTR_INIT(&(data->source_columns), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_VOID_INIT(&(data->buffers.value));
  GRN_UINT32_INIT(&(data->buffers.postings), GRN_OBJ_VECTOR);

  data->cache = NULL;
  data->cache_size = 0;
}

static void
grn_index_column_diff_data_init_cache(grn_ctx *ctx,
                                      grn_index_column_diff_data *data)
{
  size_t cache_size;
  const size_t n_tokens = grn_table_size(ctx, data->lexicon);
  if (n_tokens == 0) {
    cache_size = 1;
  } else if (n_tokens < grn_index_column_diff_cache_size_max) {
    cache_size = n_tokens;
  } else {
    cache_size = n_tokens * 0.01;
    if (cache_size >= grn_index_column_diff_cache_size_max) {
      cache_size = grn_index_column_diff_cache_size_max;
    }
  }

  data->cache = GRN_MALLOCN(grn_index_column_diff_cache, cache_size);
  if (!data->cache) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[index-column][diff] failed to allocate cache: <%" GRN_FMT_SIZE ">: %s",
        cache_size,
        message);
    return;
  }
  for (size_t i = 0; i < cache_size; i++) {
    data->cache[i].token_id = GRN_ID_NIL;
    data->cache[i].diff_id = GRN_ID_NIL;
    GRN_UINT32_INIT(&(data->cache[i].postings), GRN_OBJ_VECTOR);
    data->cache[i].offset = 0;
  }
  data->cache_size = cache_size;
}

static void
grn_index_column_diff_data_fin(grn_ctx *ctx,
                               grn_index_column_diff_data *data)
{
  {
    size_t n_columns = GRN_PTR_VECTOR_SIZE(&(data->source_columns));
    for (size_t i = 0; i < n_columns; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(&(data->source_columns), i);
      if (grn_obj_is_accessor(ctx, column)) {
        grn_obj_close(ctx, column);
      }
    }
    GRN_OBJ_FIN(ctx, &(data->source_columns));
  }

  GRN_OBJ_FIN(ctx, &(data->buffers.value));
  GRN_OBJ_FIN(ctx, &(data->buffers.postings));

  for (size_t i = 0; i < data->cache_size; i++) {
    GRN_OBJ_FIN(ctx, &(data->cache[i].postings));
  }
  GRN_FREE(data->cache);
}

static void
grn_index_column_diff_init_progress(grn_ctx *ctx,
                                    grn_index_column_diff_data *data)
{
  data->progress.n_records = grn_table_size(ctx, data->source_table);
  data->progress.i = 0;
  data->progress.interval = 10000;
  data->progress.log_level = GRN_LOG_DEBUG;
  data->progress.n_records_digits =
    ceil(log10(data->progress.n_records + 1));
  grn_timeval_now(ctx, &(data->progress.start_time));
  data->progress.previous_time = data->progress.start_time;
}

static double
grn_index_column_diff_format_time(grn_ctx *ctx,
                                  double seconds,
                                  const char **unit)
{
  if (seconds < 60) {
    *unit = "s";
    return seconds;
  } else if (seconds < (60 * 60)) {
    *unit = "m";
    return seconds / 60;
  } else if (seconds < (60 * 60 * 24)) {
    *unit = "h";
    return seconds / 60 / 60;
  } else {
    *unit = "d";
    return seconds / 60 / 60 / 24;
  }
}

static double
grn_index_column_diff_format_memory(grn_ctx *ctx,
                                    uint64_t usage,
                                    const char **unit)
{
  if (usage < 1024) {
    *unit = "B";
    return usage;
  } else if (usage < (1024 * 1024)) {
    *unit = "KiB";
    return usage / 1024.0;
  } else if (usage < (1024 * 1024 * 1024)) {
    *unit = "MiB";
    return usage / 1024.0 / 1024.0;
  } else {
    *unit = "GiB";
    return usage / 1024.0 / 1024.0 / 1024.0;
  }
}

static void
grn_index_column_diff_progress(grn_ctx *ctx,
                               grn_index_column_diff_data *data)
{
  data->progress.i++;

  const unsigned int i = data->progress.i;
  const unsigned int interval = data->progress.interval;
  const unsigned int n_records = data->progress.n_records;
  if (grn_logger_pass(ctx, data->progress.log_level) &&
      (((i % interval) == 0) ||
       (i == n_records))) {
    grn_timeval current_time;
    grn_timeval_now(ctx, &current_time);
    const grn_timeval *start_time = &(data->progress.start_time);
    const grn_timeval *previous_time = &(data->progress.previous_time);
    const double elapsed_seconds =
      (current_time.tv_sec + current_time.tv_nsec / GRN_TIME_NSEC_PER_SEC_F) -
      (start_time->tv_sec + start_time->tv_nsec / GRN_TIME_NSEC_PER_SEC_F);
    const double current_interval_seconds =
      (current_time.tv_sec + current_time.tv_nsec / GRN_TIME_NSEC_PER_SEC_F) -
      (previous_time->tv_sec + previous_time->tv_nsec / GRN_TIME_NSEC_PER_SEC_F);
    const double throughput = interval / current_interval_seconds;
    const double remained_seconds =
      elapsed_seconds + ((n_records - i) / throughput);
    const char *elapsed_unit = NULL;
    const double elapsed_time =
      grn_index_column_diff_format_time(ctx, elapsed_seconds, &elapsed_unit);
    const char *remained_unit = NULL;
    const double remained_time =
      grn_index_column_diff_format_time(ctx, remained_seconds, &remained_unit);
    const char *interval_unit = NULL;
    const double interval_time =
      grn_index_column_diff_format_time(ctx,
                                        current_interval_seconds,
                                        &interval_unit);
    const char *memory_unit = NULL;
    const double memory_usage =
      grn_index_column_diff_format_memory(ctx,
                                          grn_memory_get_usage(ctx),
                                          &memory_unit);

    GRN_LOG(ctx,
            data->progress.log_level,
            "[index-column][diff][progress][%.*s] "
            "%*u/%u %3.0f%% %.2f%s/%.2f%s %.2f%s(%.2frecords/s) %.2f%s",
            data->index.name_size,
            data->index.name,
            data->progress.n_records_digits,
            i,
            n_records,
            ((double)i / (double)n_records) * 100,
            elapsed_time, elapsed_unit,
            remained_time, remained_unit,
            interval_time, interval_unit,
            throughput,
            memory_usage, memory_unit);
    data->progress.previous_time = current_time;
  }
}

static size_t
grn_index_column_diff_cache_compute_key(grn_ctx *ctx,
                                        grn_index_column_diff_data *data,
                                        grn_id token_id)
{
  return token_id % data->cache_size;
}

static void
grn_index_column_diff_cache_flush(grn_ctx *ctx,
                                  grn_index_column_diff_data *data,
                                  size_t cache_key)
{
  const grn_id cached_token_id = data->cache[cache_key].token_id;
  if (cached_token_id == GRN_ID_NIL) {
    return;
  }

  grn_obj *new_postings = &(data->buffers.postings);
  GRN_BULK_REWIND(new_postings);
  grn_obj *postings = &(data->cache[cache_key].postings);
  const size_t n_posting_elements = data->n_posting_elements;
  const size_t offset = data->cache[cache_key].offset;
  const uint32_t *postings_raw =
    ((uint32_t *)GRN_BULK_HEAD(postings)) +
    (n_posting_elements * offset);
  const size_t postings_raw_size =
    sizeof(uint32_t) *
    (GRN_UINT32_VECTOR_SIZE(postings) - (n_posting_elements * offset));
  grn_bulk_write(ctx,
                 new_postings,
                 (const char *)postings_raw,
                 postings_raw_size);
  grn_obj_set_value(ctx,
                    data->postings,
                    data->cache[cache_key].diff_id,
                    new_postings,
                    GRN_OBJ_SET);

  data->cache[cache_key].token_id = GRN_ID_NIL;
  data->cache[cache_key].diff_id = GRN_ID_NIL;
  data->cache[cache_key].offset = 0;
  GRN_BULK_REWIND(&(data->cache[cache_key].postings));
}

static void
grn_index_column_diff_cache_flush_all(grn_ctx *ctx,
                                      grn_index_column_diff_data *data)
{
  for (size_t i = 0; i < data->cache_size; i++) {
    grn_index_column_diff_cache_flush(ctx, data, i);
  }
}

static grn_obj *
grn_index_column_diff_cache_allocate(grn_ctx *ctx,
                                     grn_index_column_diff_data *data)
{
  const grn_id token_id = data->current.token_id;
  const size_t cache_key =
    grn_index_column_diff_cache_compute_key(ctx, data, token_id);
  grn_index_column_diff_cache_flush(ctx, data, cache_key);
  data->cache[cache_key].token_id = token_id;
  data->cache[cache_key].diff_id = data->current.diff_id;
  return &(data->cache[cache_key].postings);
}

static const uint32_t *
grn_index_column_diff_cache_get(grn_ctx *ctx,
                                grn_index_column_diff_data *data,
                                size_t *n_postings)
{
  const grn_id token_id = data->current.token_id;
  const size_t cache_key
    = grn_index_column_diff_cache_compute_key(ctx, data, token_id);
  const grn_id cached_token_id = data->cache[cache_key].token_id;
  grn_obj *postings;
  if (cached_token_id == token_id) {
    postings = &(data->cache[cache_key].postings);
  } else {
    postings = grn_index_column_diff_cache_allocate(ctx, data);
    const grn_id diff_id = data->current.diff_id;
    grn_obj_get_value(ctx, data->postings, diff_id, postings);
  }
  const size_t n_posting_elements = data->n_posting_elements;
  const size_t offset = data->cache[cache_key].offset;
  *n_postings =
    (GRN_UINT32_VECTOR_SIZE(postings) / n_posting_elements) -
    offset;
  return ((uint32_t *)GRN_BULK_HEAD(postings)) + (n_posting_elements * offset);
}

static void
grn_index_column_diff_cache_remove_posting(grn_ctx *ctx,
                                           grn_index_column_diff_data *data,
                                           size_t nth_posting)
{
  const grn_id token_id = data->current.token_id;
  const size_t cache_key =
    grn_index_column_diff_cache_compute_key(ctx, data, token_id);
  const size_t n_posting_elements = data->n_posting_elements;
  size_t n_postings = 0;
  const uint32_t *postings =
    grn_index_column_diff_cache_get(ctx, data, &n_postings);

  if (nth_posting > 0) {
    grn_obj *remains = &(data->buffers.postings);
    GRN_BULK_REWIND(remains);
    const size_t posting_size = sizeof(uint32_t) * n_posting_elements;
    grn_bulk_write(ctx,
                   remains,
                   (const char *)postings,
                   posting_size * nth_posting);
    const grn_id diff_id = data->current.diff_id;
    grn_obj_set_value(ctx, data->remains, diff_id, remains, GRN_OBJ_APPEND);
  }
  data->cache[cache_key].offset += nth_posting + 1;
}

static const uint32_t *
grn_index_column_diff_get_postings(grn_ctx *ctx,
                                   grn_index_column_diff_data *data,
                                   size_t *n_postings)
{
  if (!data->current.is_new_diff) {
    return grn_index_column_diff_cache_get(ctx, data, n_postings);
  }

  grn_obj *postings = grn_index_column_diff_cache_allocate(ctx, data);
  const grn_id token_id = data->current.token_id;
  const unsigned int ii_cursor_flags = 0;
  grn_ii_cursor *ii_cursor = grn_ii_cursor_open(ctx,
                                                data->ii,
                                                token_id,
                                                GRN_ID_NIL,
                                                GRN_ID_MAX,
                                                data->index.n_elements,
                                                ii_cursor_flags);
  if (ii_cursor) {
    const grn_bool with_section = data->index.with_section;
    const grn_bool with_position = data->index.with_position;
    if (with_position) {
      while (grn_ii_cursor_next(ctx, ii_cursor)) {
        grn_posting *posting;
        while ((posting = grn_ii_cursor_next_pos(ctx, ii_cursor))) {
          GRN_UINT32_PUT(ctx, postings, posting->rid);
          if (with_section) {
            GRN_UINT32_PUT(ctx, postings, posting->sid);
          }
          GRN_UINT32_PUT(ctx, postings, posting->pos);
        }
      }
    } else {
      grn_posting *posting;
      while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
        GRN_UINT32_PUT(ctx, postings, posting->rid);
        if (with_section) {
          GRN_UINT32_PUT(ctx, postings, posting->sid);
        }
      }
    }
    grn_ii_cursor_close(ctx, ii_cursor);
  }
  *n_postings = GRN_UINT32_VECTOR_SIZE(postings) / data->n_posting_elements;
  return (const uint32_t *)(GRN_BULK_HEAD(postings));
}

static int
grn_index_column_diff_compare_posting(grn_ctx *ctx,
                                      grn_index_column_diff_data *data,
                                      const uint32_t *posting)
{
  const grn_bool with_section = data->index.with_section;
  const grn_bool with_position = data->index.with_position;
  const grn_posting *current_posting = &(data->current.posting);

  size_t i = 0;
  const grn_id record_id = posting[i];
  if (record_id < current_posting->rid) {
    return -1;
  } else if (record_id > current_posting->rid) {
    return 1;
  }

  if (with_section) {
    i++;
    const uint32_t section_id = posting[i];
    if (section_id < current_posting->sid) {
      return -1;
    } else if (section_id > current_posting->sid) {
      return 1;
    }
  }

  if (with_position) {
    i++;
    const uint32_t position = posting[i];
    if (position < current_posting->pos) {
      return -1;
    } else if (position > current_posting->pos) {
      return 1;
    }
  }

  return 0;
}

static int64_t
grn_index_column_diff_find_posting(grn_ctx *ctx,
                                   grn_index_column_diff_data *data)
{
  size_t n_postings = 0;
  const uint32_t *postings =
    grn_index_column_diff_get_postings(ctx, data, &n_postings);
  const size_t n_posting_elements = data->n_posting_elements;
  for (size_t i = 0; i < n_postings; i++) {
    const uint32_t *posting = postings + (n_posting_elements * i);
    const int compared =
      grn_index_column_diff_compare_posting(ctx, data, posting);
    if (compared == 0) {
      return i;
    } else if (compared > 0) {
      return -1;
    }
  }
  return -1;
}

static void
grn_index_column_diff_process_token_id(grn_ctx *ctx,
                                       grn_index_column_diff_data *data)
{
  {
    int added = 0;
    data->current.diff_id =
      grn_table_add(ctx,
                    data->diff,
                    &(data->current.token_id), sizeof(grn_id),
                    &added);
    data->current.is_new_diff = (added != 0);
  }

  const int64_t nth_posting =
    grn_index_column_diff_find_posting(ctx, data);
  if (nth_posting >= 0) {
    grn_index_column_diff_cache_remove_posting(ctx,
                                               data,
                                               (size_t)nth_posting);
  } else {
    grn_obj *postings = &(data->buffers.postings);
    GRN_BULK_REWIND(postings);
    const grn_posting *posting = &(data->current.posting);
    GRN_UINT32_PUT(ctx, postings, posting->rid);
    if (data->index.with_section) {
      GRN_UINT32_PUT(ctx, postings, posting->sid);
    }
    if (data->index.with_position) {
      GRN_UINT32_PUT(ctx, postings, posting->pos);
    }
    grn_obj_set_value(ctx,
                      data->missings,
                      data->current.diff_id,
                      postings,
                      GRN_OBJ_APPEND);
  }
}

static void
grn_index_column_diff_process_token(grn_ctx *ctx,
                                    grn_index_column_diff_data *data,
                                    const void *value_data,
                                    size_t value_size)
{
  if (value_size == 0) {
    return;
  }

  const unsigned int token_cursor_flags = 0;
  grn_token_cursor *token_cursor =
    grn_token_cursor_open(ctx,
                          data->lexicon,
                          value_data,
                          value_size,
                          GRN_TOKEN_ADD,
                          token_cursor_flags);
  if (!token_cursor) {
    return;
  }

  const grn_bool with_position = data->index.with_position;
  while (grn_token_cursor_get_status(ctx, token_cursor) ==
         GRN_TOKEN_CURSOR_DOING) {
    data->current.token_id = grn_token_cursor_next(ctx, token_cursor);
    if (data->current.token_id == GRN_ID_NIL) {
      continue;
    }

    if (with_position) {
      grn_token *token = grn_token_cursor_get_token(ctx, token_cursor);
      data->current.posting.pos = grn_token_get_position(ctx, token);
    }

    grn_index_column_diff_process_token_id(ctx, data);
  }
  grn_token_cursor_close(ctx, token_cursor);
}

static void
grn_index_column_diff_compute(grn_ctx *ctx,
                              grn_index_column_diff_data *data)
{
  grn_obj *source_columns = &(data->source_columns);
  const size_t n_source_columns = GRN_PTR_VECTOR_SIZE(source_columns);
  grn_obj *value = &(data->buffers.value);
  grn_obj *postings = &(data->buffers.postings);

  grn_index_column_diff_init_progress(ctx, data);

  GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                             data->source_table,
                             cursor,
                             id,
                             GRN_CURSOR_BY_ID) {
    grn_index_column_diff_progress(ctx, data);
    for (size_t i = 0; i < n_source_columns; i++) {
      grn_obj *source = GRN_PTR_VALUE_AT(source_columns, i);
      const grn_bool is_reference = grn_obj_is_reference_column(ctx, source);

      data->current.posting.rid = id;
      data->current.posting.sid = i + 1;

      GRN_BULK_REWIND(value);
      grn_obj_get_value(ctx, source, id, value);

      switch (value->header.type) {
      case GRN_VECTOR :
        {
          const size_t n_elements = grn_vector_size(ctx, value);
          for (size_t j = 0; j < n_elements; j++) {
            const char *element = NULL;
            const unsigned int element_size =
              grn_vector_get_element(ctx, value, j, &element, NULL, NULL);
            data->current.posting.sid = j + 1;
            grn_index_column_diff_process_token(ctx,
                                                data,
                                                element,
                                                element_size);
          }
        }
        break;
      case GRN_UVECTOR :
        if (is_reference) {
          const size_t n_elements = grn_uvector_size(ctx, value);
          for (size_t j = 0; j < n_elements; j++) {
            const grn_id element =
              grn_uvector_get_element(ctx, value, j, NULL);
            data->current.token_id = element;
            data->current.posting.pos = 0;
            grn_index_column_diff_process_token_id(ctx, data);
          }
        } else {
          const size_t n_elements = grn_uvector_size(ctx, value);
          const size_t element_size = grn_uvector_element_size(ctx, value);
          for (size_t j = 0; j < n_elements; j++) {
            const char *element = GRN_BULK_HEAD(value) + (element_size * j);
            data->current.token_id =
              grn_table_get(ctx, data->lexicon, element, element_size);
            if (data->current.token_id == GRN_ID_NIL) {
              continue;
            }
            data->current.posting.pos = 0;
            grn_index_column_diff_process_token_id(ctx, data);
          }
        }
        break;
      case GRN_BULK :
        if (GRN_BULK_VSIZE(value) > 0) {
          if (is_reference) {
            data->current.token_id = GRN_RECORD_VALUE(value);
            data->current.posting.pos = 0;
            grn_index_column_diff_process_token_id(ctx, data);
          } else {
            grn_index_column_diff_process_token(ctx,
                                                data,
                                                GRN_BULK_HEAD(value),
                                                GRN_BULK_VSIZE(value));
          }
        }
        break;
      default :
        break;
      }
    }
  } GRN_TABLE_EACH_END(ctx, cursor);

  grn_index_column_diff_cache_flush_all(ctx, data);

  {
    GRN_TABLE_EACH_BEGIN(ctx, data->diff, cursor, id) {
      GRN_BULK_REWIND(postings);
      grn_obj_get_value(ctx, data->postings, id, postings);
      if (GRN_UINT32_VECTOR_SIZE(postings) > 0) {
        grn_obj_set_value(ctx, data->remains, id, postings, GRN_OBJ_APPEND);
        continue;
      }
      GRN_BULK_REWIND(postings);
      grn_obj_get_value(ctx, data->remains, id, postings);
      if (GRN_UINT32_VECTOR_SIZE(postings) > 0) {
        continue;
      }
      GRN_BULK_REWIND(postings);
      grn_obj_get_value(ctx, data->missings, id, postings);
      if (GRN_UINT32_VECTOR_SIZE(postings) > 0) {
        continue;
      }
      grn_table_cursor_delete(ctx, cursor);
    } GRN_TABLE_EACH_END(ctx, cursor);
  }

  grn_obj_close(ctx, data->postings);
}

grn_rc
grn_index_column_diff(grn_ctx *ctx,
                      grn_obj *index_column,
                      grn_obj **diff)
{
  grn_index_column_diff_data data = {0};

  GRN_API_ENTER;

  grn_index_column_diff_data_init(ctx, &data);

  if (!index_column) {
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] index column must not NULL");
    goto exit;
  }
  if (!grn_obj_is_index_column(ctx, index_column)) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, index_column, name, sizeof(name));
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] invalid index column: <%.*s>: <%s>",
        name_size, name,
        grn_obj_type_to_string(index_column->header.type));
    goto exit;
  }
  data.ii = (grn_ii *)index_column;
  {
    data.index.name_size =
      grn_obj_name(ctx,
                   index_column,
                   data.index.name,
                   sizeof(data.index.name));
    grn_column_flags flags = grn_column_get_flags(ctx, index_column);
    data.index.with_section =
      ((flags & GRN_OBJ_WITH_SECTION) == GRN_OBJ_WITH_SECTION);
    data.index.with_position =
      ((flags & GRN_OBJ_WITH_POSITION) == GRN_OBJ_WITH_POSITION);
    data.index.n_elements = grn_ii_get_n_elements(ctx, data.ii);
  }

  data.n_posting_elements = 1;
  if (data.index.with_section) {
    data.n_posting_elements++;
  }
  if (data.index.with_position) {
    data.n_posting_elements++;
  }

  data.source_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, index_column));
  {
    grn_obj source_columns;
    GRN_RECORD_INIT(&source_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_obj_get_info(ctx, index_column, GRN_INFO_SOURCE, &source_columns);
    size_t n_columns = GRN_RECORD_VECTOR_SIZE(&source_columns);
    for (size_t i = 0; i < n_columns; i++) {
      grn_id source_id = GRN_RECORD_VALUE_AT(&source_columns, i);
      grn_obj *source = grn_ctx_at(ctx, source_id);
      if (grn_obj_is_table(ctx, source)) {
        source = grn_obj_column(ctx,
                                source,
                                GRN_COLUMN_NAME_KEY,
                                GRN_COLUMN_NAME_KEY_LEN);
      }
      GRN_PTR_PUT(ctx, &(data.source_columns), source);
    }
    GRN_OBJ_FIN(ctx, &source_columns);
  }

  data.lexicon = grn_ctx_at(ctx, index_column->header.domain);
  grn_index_column_diff_data_init_cache(ctx, &data);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  data.diff = grn_table_create(ctx,
                               NULL, 0,
                               NULL,
                               GRN_TABLE_HASH_KEY,
                               data.lexicon,
                               NULL);
  if (!data.diff) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] failed to create diff table: <%.*s>: %s",
        data.index.name_size,
        data.index.name,
        message);
    goto exit;
  }
  data.postings = grn_column_create(ctx,
                                    data.diff,
                                    postings_column_name,
                                    strlen(postings_column_name),
                                    NULL,
                                    GRN_OBJ_COLUMN_VECTOR,
                                    grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!data.postings) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] failed to create postings column: <%.*s>: %s",
        data.index.name_size,
        data.index.name,
        message);
    goto exit;
  }
  data.remains = grn_column_create(ctx,
                                   data.diff,
                                   remains_column_name,
                                   strlen(remains_column_name),
                                   NULL,
                                   GRN_OBJ_COLUMN_VECTOR,
                                   grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!data.remains) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] failed to create remains column: <%.*s>: %s",
        data.index.name_size,
        data.index.name,
        message);
    goto exit;
  }
  data.missings = grn_column_create(ctx,
                                    data.diff,
                                    missings_column_name,
                                    strlen(missings_column_name),
                                    NULL,
                                    GRN_OBJ_COLUMN_VECTOR,
                                    grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!data.missings) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] failed to create missings column: <%.*s>: %s",
        data.index.name_size,
        data.index.name,
        message);
    goto exit;
  }

  grn_index_column_diff_compute(ctx, &data);
  *diff = data.diff;
  data.diff = NULL;

exit :
  if (data.diff) {
    grn_obj_close(ctx, data.diff);
  }

  grn_index_column_diff_data_fin(ctx, &data);

  GRN_API_RETURN(ctx->rc);
}
