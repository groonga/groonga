/*
  Copyright (C) 2009-2015  Brazil
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

#include "grn_index_column.h"
#include "grn_ii.h"
#include "grn_hash.h"

#include <string.h>
#include <math.h>

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
    grn_proc_ctx pctx;
    grn_proc_ctx_init(&pctx, hooks, 4, 4);
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
  grn_obj *src, *target;
  grn_id *s = DB_OBJ(index_column)->source;

  if (!(DB_OBJ(index_column)->source_size) || !s) { return ctx->rc; }

  src = grn_ctx_at(ctx, *s);
  if (!src) {
    ERR(GRN_INVALID_ARGUMENT, "invalid source");
    return ctx->rc;
  }

  target = GRN_OBJ_TABLEP(src) ? src : grn_ctx_at(ctx, src->header.domain);
  if (!target) {
    ERR(GRN_INVALID_ARGUMENT, "invalid target");
    return ctx->rc;
  }

  grn_table_flags lexicon_flags;
  grn_ii *ii = (grn_ii *)index_column;
  grn_bool use_grn_ii_build;
  grn_obj *tokenizer = NULL;
  grn_table_get_info(ctx,
                     ii->lexicon,
                     &lexicon_flags,
                     NULL,
                     &tokenizer,
                     NULL,
                     NULL);
  switch (lexicon_flags & GRN_OBJ_TABLE_TYPE_MASK) {
  case GRN_OBJ_TABLE_PAT_KEY :
  case GRN_OBJ_TABLE_DAT_KEY :
    use_grn_ii_build = GRN_TRUE;
    break;
  default :
    use_grn_ii_build = GRN_FALSE;
    break;
  }
  const grn_column_flags flags = grn_ii_get_flags(ctx, ii);
  if ((flags & GRN_OBJ_WITH_POSITION) &&
      (!tokenizer &&
       !GRN_TYPE_IS_TEXT_FAMILY(ii->lexicon->header.domain))) {
    /* TODO: Support offline index construction for WITH_POSITION
     * index against UInt32 vector column. */
    use_grn_ii_build = GRN_FALSE;
  }

  size_t n_columns = DB_OBJ(index_column)->source_size / sizeof(grn_id);
  grn_obj **columns = GRN_MALLOC(n_columns * sizeof(grn_obj *));
  if (!columns) {
    return ctx->rc;
  }

  grn_obj **cp;
  size_t i;
  for (cp = columns, i = n_columns;
       i > 0;
       s++, cp++, i--) {
    if (!(*cp = grn_ctx_at(ctx, *s))) {
      ERR(GRN_INVALID_ARGUMENT, "source invalid, i=%" GRN_FMT_SIZE, i);
      GRN_FREE(columns);
      return ctx->rc;
    }
  }
  grn_obj_set_visibility(ctx, index_column, false);
  if (use_grn_ii_build) {
    if (grn_index_chunk_split_enable) {
      grn_ii_build2(ctx, ii, NULL);
    } else {
      grn_ii_build(ctx, ii, grn_index_sparsity);
    }
  } else {
    for (i = 0; i < n_columns; i++) {
      grn_obj *column = columns[i];
      grn_index_column_build_column(ctx, index_column, target, column);
    }
  }
  grn_obj_set_visibility(ctx, index_column, true);
  for (i = 0; i < n_columns; i++) {
    grn_obj_unref(ctx, columns[i]);
  }
  GRN_FREE(columns);
  grn_obj_touch(ctx, index_column, NULL);

  if (target != src) {
    grn_obj_unref(ctx, target);
  }
  grn_obj_unref(ctx, src);

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

typedef enum {
  GRN_INDEX_COLUMN_DIFF_TYPE_REMAINS,
  GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS,
  GRN_INDEX_COLUMN_DIFF_TYPE_LOOK_AHEAD,
} grn_index_column_diff_type;
/* Except LOOK_AHEAD */
#define N_GRN_INDEX_COLUMN_DIFF_PERSISTENT_TYPES 2

static const char *
grn_index_column_diff_type_to_string(grn_index_column_diff_type type)
{
  switch (type) {
  case GRN_INDEX_COLUMN_DIFF_TYPE_REMAINS :
    return "remains";
  case GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS :
    return "missings";
  case GRN_INDEX_COLUMN_DIFF_TYPE_LOOK_AHEAD :
    return "look-ahead";
  default :
    return "unknown";
  }
}

static const char *remains_column_name = "remains";
static const char *missings_column_name = "missings";

typedef struct {
  grn_id token_id;
  grn_obj remains;
  grn_obj missings;
  grn_obj look_ahead;
  size_t look_ahead_offset;
  grn_posting last_posting;
  grn_ii_cursor *cursor;
  bool need_cursor_next;
  size_t n_postings[N_GRN_INDEX_COLUMN_DIFF_PERSISTENT_TYPES];
  size_t thresholds[N_GRN_INDEX_COLUMN_DIFF_PERSISTENT_TYPES];
} grn_index_column_diff_posting_list;

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
  bool have_tokenizer;
  size_t n_posting_elements;
  grn_obj *source_table;
  grn_obj source_columns;
  grn_obj *diff;
  grn_hash *posting_lists;
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
} grn_index_column_diff_data;

static void
grn_index_column_diff_data_init(grn_ctx *ctx,
                                grn_index_column_diff_data *data)
{
  GRN_PTR_INIT(&(data->source_columns), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_VOID_INIT(&(data->buffers.value));
  GRN_UINT32_INIT(&(data->buffers.postings), GRN_OBJ_VECTOR);
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
}

static void
grn_index_column_diff_init_progress(grn_ctx *ctx,
                                    grn_index_column_diff_data *data)
{
  data->progress.n_records = grn_table_size(ctx, data->source_table);
  data->progress.i = 0;
  data->progress.interval = 10000;
  data->progress.log_level = GRN_LOG_DEBUG;
  double n_records_digits = ceil(log10(data->progress.n_records + 1));
  data->progress.n_records_digits = (int)n_records_digits;
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
    return (double)usage;
  } else if (usage < (1024 * 1024)) {
    *unit = "KiB";
    return (double)usage / 1024.0;
  } else if (usage < (1024 * 1024 * 1024)) {
    *unit = "MiB";
    return (double)usage / 1024.0 / 1024.0;
  } else {
    *unit = "GiB";
    return (double)usage / 1024.0 / 1024.0 / 1024.0;
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
      ((double)(current_time.tv_sec) +
       current_time.tv_nsec / GRN_TIME_NSEC_PER_SEC_F) -
      ((double)(start_time->tv_sec) +
       start_time->tv_nsec / GRN_TIME_NSEC_PER_SEC_F);
    const double current_interval_seconds =
      ((double)(current_time.tv_sec) +
       current_time.tv_nsec / GRN_TIME_NSEC_PER_SEC_F) -
      ((double)(previous_time->tv_sec) +
       previous_time->tv_nsec / GRN_TIME_NSEC_PER_SEC_F);
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

typedef enum {
  GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL,
  GRN_INDEX_COLUMN_DIFF_COMPARED_LESS,
  GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER,
} grn_index_column_diff_compared;

static char
grn_index_column_diff_compared_to_char(grn_index_column_diff_compared compared)
{
  switch (compared) {
  case GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL :
    return '=';
  case GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER :
    return '>';
  case GRN_INDEX_COLUMN_DIFF_COMPARED_LESS :
    return '<';
  default :
    return '?';
  }
}

static grn_index_column_diff_compared
grn_index_column_diff_compare_posting(grn_ctx *ctx,
                                      grn_index_column_diff_data *data,
                                      const grn_posting *posting)
{
  const grn_bool with_section = data->index.with_section;
  const grn_bool with_position = data->index.with_position;
  const grn_posting *current_posting = &(data->current.posting);

  if (posting->rid < current_posting->rid) {
    return GRN_INDEX_COLUMN_DIFF_COMPARED_LESS;
  } else if (posting->rid > current_posting->rid) {
    return GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER;
  }

  if (with_section) {
    if (posting->sid < current_posting->sid) {
      return GRN_INDEX_COLUMN_DIFF_COMPARED_LESS;
    } else if (posting->sid > current_posting->sid) {
      return GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER;
    }
  }

  if (with_position) {
    if (posting->pos < current_posting->pos) {
      return GRN_INDEX_COLUMN_DIFF_COMPARED_LESS;
    } else if (posting->pos > current_posting->pos) {
      return GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER;
    }
  }

  return GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL;
}

static int
grn_index_column_diff_token_id_width(grn_ctx *ctx,
                                     grn_index_column_diff_data *data)
{
  unsigned int n_tokens = grn_table_size(ctx, data->lexicon);
  if (n_tokens == 0) {
    return 0;
  } else {
    double width = floor(log10(n_tokens)) + 1;
    return (int)width;
  }
}

static void
grn_index_column_diff_posting_list_init(grn_ctx *ctx,
                                        grn_index_column_diff_data *data,
                                        grn_index_column_diff_posting_list *posting_list)
{
  posting_list->token_id = data->current.token_id;
  GRN_UINT32_INIT(&(posting_list->remains), GRN_OBJ_VECTOR);
  GRN_UINT32_INIT(&(posting_list->missings), GRN_OBJ_VECTOR);
  GRN_UINT32_INIT(&(posting_list->look_ahead), GRN_OBJ_VECTOR);
  posting_list->look_ahead_offset = 0;
  posting_list->last_posting.tf = 0;

  const int ii_cursor_flags = 0;
  posting_list->cursor = grn_ii_cursor_open(ctx,
                                            data->ii,
                                            posting_list->token_id,
                                            GRN_ID_NIL,
                                            GRN_ID_MAX,
                                            (int)(data->index.n_elements),
                                            ii_cursor_flags);
  posting_list->need_cursor_next = true;
  for (size_t i = 0; i < N_GRN_INDEX_COLUMN_DIFF_PERSISTENT_TYPES; i++) {
    posting_list->n_postings[i] = 0;
    posting_list->thresholds[i] = 4096;
  }
}

static void
grn_index_column_diff_posting_list_fin(grn_ctx *ctx,
                                       grn_index_column_diff_data *data,
                                       grn_index_column_diff_posting_list *posting_list)
{
  grn_obj *remains = &(posting_list->remains);
  grn_obj *missings = &(posting_list->missings);
  {
    grn_obj *look_ahead = &(posting_list->look_ahead);
    const size_t offset = posting_list->look_ahead_offset * sizeof(uint32_t);
    grn_bulk_write(ctx,
                   remains,
                   GRN_BULK_HEAD(look_ahead) + offset,
                   GRN_BULK_VSIZE(look_ahead) - offset);
    GRN_OBJ_FIN(ctx, look_ahead);
  }

  grn_ii_cursor *cursor = posting_list->cursor;
  if (cursor) {
    const grn_bool with_section = data->index.with_section;
    const grn_bool with_position = data->index.with_position;
    if (with_position) {
      while (!posting_list->need_cursor_next ||
             grn_ii_cursor_next(ctx, cursor)) {
        grn_posting *posting;
        while ((posting = grn_ii_cursor_next_pos(ctx, cursor))) {
          posting_list->need_cursor_next = false;
          GRN_UINT32_PUT(ctx, remains, posting->rid);
          if (with_section) {
            GRN_UINT32_PUT(ctx, remains, posting->sid);
          }
          GRN_UINT32_PUT(ctx, remains, posting->pos);
        }
        posting_list->need_cursor_next = true;
      }
    } else {
      grn_posting *posting;
      while ((posting = grn_ii_cursor_next(ctx, cursor))) {
        GRN_UINT32_PUT(ctx, remains, posting->rid);
        if (with_section) {
          GRN_UINT32_PUT(ctx, remains, posting->sid);
        }
      }
    }
    grn_ii_cursor_close(ctx, cursor);
    posting_list->cursor = NULL;
  }

  if (GRN_BULK_VSIZE(remains) > 0 || GRN_BULK_VSIZE(missings) > 0) {
    const grn_id diff_id = grn_table_add(ctx,
                                         data->diff,
                                         &(posting_list->token_id),
                                         sizeof(grn_id),
                                         NULL);
    if (diff_id != GRN_ID_NIL) {
      if (GRN_BULK_VSIZE(remains) > 0) {
        grn_obj_set_value(ctx,
                          data->remains,
                          diff_id,
                          remains,
                          GRN_OBJ_APPEND);
      }
      if (GRN_BULK_VSIZE(missings) > 0) {
        grn_obj_set_value(ctx,
                          data->missings,
                          diff_id,
                          missings,
                          GRN_OBJ_APPEND);
      }
    }
  }
  GRN_OBJ_FIN(ctx, remains);
  GRN_OBJ_FIN(ctx, missings);
}

static void
grn_index_column_diff_posting_list_put(grn_ctx *ctx,
                                       grn_index_column_diff_data *data,
                                       grn_index_column_diff_posting_list *posting_list,
                                       grn_index_column_diff_type type,
                                       const grn_posting *posting)
{
  grn_obj *column;
  grn_obj *postings;
  switch (type) {
  case GRN_INDEX_COLUMN_DIFF_TYPE_REMAINS :
    column = data->remains;
    postings = &(posting_list->remains);
    break;
  case GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS :
    column = data->missings;
    postings = &(posting_list->missings);
    break;
  default :
    column = NULL;
    postings = &(posting_list->look_ahead);
    break;
  }

  GRN_UINT32_PUT(ctx, postings, posting->rid);
  if (data->index.with_section) {
    GRN_UINT32_PUT(ctx, postings, posting->sid);
  }
  if (data->index.with_position) {
    GRN_UINT32_PUT(ctx, postings, posting->pos);
  }

  if (!column) {
    return;
  }

  posting_list->n_postings[type]++;
  const size_t current_n_postings =
    GRN_UINT32_VECTOR_SIZE(postings) / data->n_posting_elements;
  if (current_n_postings < posting_list->thresholds[type]) {
    return;
  }

  const grn_id diff_id = grn_table_add(ctx,
                                       data->diff,
                                       &(posting_list->token_id),
                                       sizeof(grn_id),
                                       NULL);
  if (diff_id == GRN_ID_NIL) {
    return;
  }
  grn_obj_set_value(ctx, column, diff_id, postings, GRN_OBJ_APPEND);
  if (ctx->rc == GRN_SUCCESS) {
    GRN_BULK_REWIND(postings);
  }
  posting_list->thresholds[type] *= 2;
  GRN_LOG(ctx,
          GRN_LOG_DUMP,
          "[index-column][diff][append][%.*s][%u][%*s] "
          "%" GRN_FMT_SIZE "(+%" GRN_FMT_SIZE ")",
          data->index.name_size,
          data->index.name,
          grn_index_column_diff_token_id_width(ctx, data),
          data->current.token_id,
          grn_index_column_diff_type_to_string(type),
          posting_list->n_postings[type],
          current_n_postings);
}

static void
grn_index_column_diff_posting_list_consume_look_ahead(
  grn_ctx *ctx,
  grn_index_column_diff_data *data,
  grn_index_column_diff_posting_list *posting_list)
{
  grn_obj *look_ahead = &(posting_list->look_ahead);
  posting_list->look_ahead_offset++;
  if (posting_list->look_ahead_offset ==
      (GRN_UINT32_VECTOR_SIZE(look_ahead) / data->n_posting_elements)) {
    GRN_BULK_REWIND(look_ahead);
    posting_list->look_ahead_offset = 0;
  }
}

static void
grn_index_column_diff_process_token_id(grn_ctx *ctx,
                                       grn_index_column_diff_data *data)
{
  void *value = NULL;
  int added = 0;
  const grn_id posting_list_id =
    grn_hash_add(ctx,
                 data->posting_lists,
                 &(data->current.token_id), sizeof(grn_id),
                 &value,
                 &added);
  if (posting_list_id == GRN_ID_NIL) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_UNKNOWN_ERROR;
    }
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(rc,
        "[index-column][diff] failed to add a record to keep a posting list: "
        "%u: %s",
        data->current.token_id,
        message);
    return;
  }

  grn_index_column_diff_posting_list *posting_list = value;
  if (added) {
    grn_index_column_diff_posting_list_init(ctx, data, posting_list);
  }

  const grn_bool with_section = data->index.with_section;
  const grn_bool with_position = data->index.with_position;

  if (posting_list->last_posting.tf > 0) {
    const grn_index_column_diff_compared compared =
      grn_index_column_diff_compare_posting(ctx,
                                            data,
                                            &(posting_list->last_posting));
    GRN_LOG(ctx,
            GRN_LOG_DUMP,
            "[index-column][diff][process][%.*s][%*u] "
            "    last[%u:%u:%u:%u] %c current[%u:%u:%u]",
            data->index.name_size,
            data->index.name,
            grn_index_column_diff_token_id_width(ctx, data),
            data->current.token_id,
            posting_list->last_posting.rid,
            posting_list->last_posting.sid,
            posting_list->last_posting.pos,
            posting_list->last_posting.tf,
            grn_index_column_diff_compared_to_char(compared),
            data->current.posting.rid,
            data->current.posting.sid,
            data->current.posting.pos);
    if (compared == GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL) {
      posting_list->last_posting.tf--;
      return;
    } else {
      posting_list->last_posting.tf = 0;
    }
  }

  grn_obj *look_ahead = &(posting_list->look_ahead);
  const size_t look_ahead_offset = posting_list->look_ahead_offset;

  const size_t n_posting_elements = data->n_posting_elements;
  const size_t n_postings =
    GRN_UINT32_VECTOR_SIZE(look_ahead) /
    n_posting_elements;
  const uint32_t *raw_postings = (const uint32_t *)GRN_BULK_HEAD(look_ahead);
  for (size_t i = look_ahead_offset; i < n_postings; i++) {
    const size_t offset = i * data->n_posting_elements;
    const uint32_t *raw_posting = raw_postings + offset;
    grn_posting posting;
    size_t i = 0;
    posting.rid = raw_posting[i++];
    if (with_section) {
      posting.sid = raw_posting[i++];
    }
    if (with_position) {
      posting.pos = raw_posting[i++];
    }
    const grn_index_column_diff_compared compared =
      grn_index_column_diff_compare_posting(ctx, data, &posting);
    GRN_LOG(ctx,
            GRN_LOG_DUMP,
            "[index-column][diff][process][%.*s][%*u] "
            "look-ahead[%u:%u:%u] %c current[%u:%u:%u]",
            data->index.name_size,
            data->index.name,
            grn_index_column_diff_token_id_width(ctx, data),
            data->current.token_id,
            posting.rid,
            posting.sid,
            posting.pos,
            grn_index_column_diff_compared_to_char(compared),
            data->current.posting.rid,
            data->current.posting.sid,
            data->current.posting.pos);
    switch (compared) {
    case GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL :
      grn_index_column_diff_posting_list_consume_look_ahead(ctx,
                                                            data,
                                                            posting_list);
      return;
    case GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER :
      grn_index_column_diff_posting_list_put(ctx,
                                             data,
                                             posting_list,
                                             GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS,
                                             &(data->current.posting));
      return;
    default :
      grn_index_column_diff_posting_list_put(ctx,
                                             data,
                                             posting_list,
                                             GRN_INDEX_COLUMN_DIFF_TYPE_REMAINS,
                                             &posting);
      grn_index_column_diff_posting_list_consume_look_ahead(ctx,
                                                            data,
                                                            posting_list);
      break;
    }
  }

  grn_ii_cursor *cursor = posting_list->cursor;
  if (cursor) {
    if (with_position) {
      while (!posting_list->need_cursor_next ||
             grn_ii_cursor_next(ctx, cursor)) {
        grn_posting *posting;
        while ((posting = grn_ii_cursor_next_pos(ctx, cursor))) {
          posting_list->need_cursor_next = false;
          const grn_index_column_diff_compared compared =
            grn_index_column_diff_compare_posting(ctx, data, posting);
          GRN_LOG(ctx,
                  GRN_LOG_DUMP,
                  "[index-column][diff][process][%.*s][%*u] "
                  "      read[%u:%u:%u] %c current[%u:%u:%u]",
                  data->index.name_size,
                  data->index.name,
                  grn_index_column_diff_token_id_width(ctx, data),
                  data->current.token_id,
                  posting->rid,
                  posting->sid,
                  posting->pos,
                  grn_index_column_diff_compared_to_char(compared),
                  data->current.posting.rid,
                  data->current.posting.sid,
                  data->current.posting.pos);
          switch (compared) {
          case GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL :
            return;
          case GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER :
            grn_index_column_diff_posting_list_put(
              ctx,
              data,
              posting_list,
              GRN_INDEX_COLUMN_DIFF_TYPE_LOOK_AHEAD,
              posting);
            grn_index_column_diff_posting_list_put(
              ctx,
              data,
              posting_list,
              GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS,
              &(data->current.posting));
            return;
          default :
            grn_index_column_diff_posting_list_put(
              ctx,
              data,
              posting_list,
              GRN_INDEX_COLUMN_DIFF_TYPE_REMAINS,
              posting);
            break;
          }
        }
        posting_list->need_cursor_next = true;
      }
    } else {
      grn_posting *posting;
      while ((posting = grn_ii_cursor_next(ctx, cursor))) {
        const grn_index_column_diff_compared compared =
          grn_index_column_diff_compare_posting(ctx, data, posting);
        GRN_LOG(ctx,
                GRN_LOG_DUMP,
                "[index-column][diff][process][%.*s][%*u] "
                "    read[%u:%u:%u:%u] %c current[%u:%u:%u]",
                data->index.name_size,
                data->index.name,
                grn_index_column_diff_token_id_width(ctx, data),
                data->current.token_id,
                posting->rid,
                posting->sid,
                posting->pos,
                posting->tf,
                grn_index_column_diff_compared_to_char(compared),
                data->current.posting.rid,
                data->current.posting.sid,
                data->current.posting.pos);
        switch (compared) {
        case GRN_INDEX_COLUMN_DIFF_COMPARED_EQUAL :
          if (!with_position && posting->tf > 0) {
            posting_list->last_posting = *posting;
            posting_list->last_posting.tf--;
          }
          return;
        case GRN_INDEX_COLUMN_DIFF_COMPARED_GREATER :
          grn_index_column_diff_posting_list_put(
            ctx,
            data,
            posting_list,
            GRN_INDEX_COLUMN_DIFF_TYPE_LOOK_AHEAD,
            posting);
          grn_index_column_diff_posting_list_put(
            ctx,
            data,
            posting_list,
            GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS,
            &(data->current.posting));
          return;
        default :
          grn_index_column_diff_posting_list_put(
            ctx,
            data,
            posting_list,
            GRN_INDEX_COLUMN_DIFF_TYPE_REMAINS,
            posting);
          break;
        }
      }
    }
  }

  GRN_LOG(ctx,
          GRN_LOG_DUMP,
          "[index-column][diff][process][%.*s][%*u] "
          " no posting list:    current[%u:%u:%u]",
          data->index.name_size,
          data->index.name,
          grn_index_column_diff_token_id_width(ctx, data),
          data->current.token_id,
          data->current.posting.rid,
          data->current.posting.sid,
          data->current.posting.pos);
  grn_index_column_diff_posting_list_put(ctx,
                                         data,
                                         posting_list,
                                         GRN_INDEX_COLUMN_DIFF_TYPE_MISSINGS,
                                         &(data->current.posting));
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

    if (with_position && data->have_tokenizer) {
      grn_token *token = grn_token_cursor_get_token(ctx, token_cursor);
      data->current.posting.pos = grn_token_get_position(ctx, token);
    }

    grn_index_column_diff_process_token_id(ctx, data);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
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
      data->current.posting.sid = (uint32_t)(i + 1);

      GRN_BULK_REWIND(value);
      grn_obj_get_value(ctx, source, id, value);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }

      switch (value->header.type) {
      case GRN_VECTOR :
        {
          const size_t n_elements = grn_vector_size(ctx, value);
          for (size_t j = 0; j < n_elements; j++) {
            const char *element = NULL;
            const unsigned int element_size =
              grn_vector_get_element(ctx,
                                     value,
                                     (uint32_t)j,
                                     &element,
                                     NULL,
                                     NULL);
            data->current.posting.sid = (uint32_t)(j + 1);
            if (!data->have_tokenizer) {
              data->current.posting.pos = j;
            }
            grn_index_column_diff_process_token(ctx,
                                                data,
                                                element,
                                                element_size);
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
        }
        break;
      case GRN_UVECTOR :
        if (is_reference) {
          const size_t n_elements = grn_uvector_size(ctx, value);
          for (size_t j = 0; j < n_elements; j++) {
            const grn_id element =
              grn_uvector_get_element(ctx, value, (uint32_t)j, NULL);
            if (element == GRN_ID_NIL) {
              continue;
            }
            data->current.token_id = element;
            if (data->have_tokenizer) {
              data->current.posting.pos = 0;
            } else {
              data->current.posting.pos = j;
            }
            grn_index_column_diff_process_token_id(ctx, data);
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
        } else {
          const size_t n_elements = grn_uvector_size(ctx, value);
          const size_t element_size = grn_uvector_element_size(ctx, value);
          for (size_t j = 0; j < n_elements; j++) {
            const char *element = GRN_BULK_HEAD(value) + (element_size * j);
            data->current.token_id =
              grn_table_get(ctx,
                            data->lexicon,
                            element,
                            (const unsigned int)element_size);
            if (data->current.token_id == GRN_ID_NIL) {
              continue;
            }
            if (data->have_tokenizer) {
              data->current.posting.pos = 0;
            } else {
              data->current.posting.pos = j;
            }
            grn_index_column_diff_process_token_id(ctx, data);
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
        }
        break;
      case GRN_BULK :
        if (GRN_BULK_VSIZE(value) > 0) {
          if (is_reference) {
            data->current.token_id = GRN_RECORD_VALUE(value);
            if (data->current.token_id != GRN_ID_NIL) {
              data->current.posting.pos = 0;
              grn_index_column_diff_process_token_id(ctx, data);
            }
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

      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  } GRN_TABLE_EACH_END(ctx, cursor);

  GRN_HASH_EACH_BEGIN(ctx, data->posting_lists, cursor, id) {
    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    grn_index_column_diff_posting_list *posting_list = value;
    grn_index_column_diff_posting_list_fin(ctx, data, posting_list);
  } GRN_HASH_EACH_END(ctx, cursor);
  grn_hash_close(ctx, data->posting_lists);
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
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  data.have_tokenizer = grn_table_have_tokenizer(ctx, data.lexicon);

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
  data.posting_lists =
    grn_hash_create(ctx,
                    NULL,
                    sizeof(grn_id),
                    sizeof(grn_index_column_diff_posting_list),
                    GRN_OBJ_TABLE_HASH_KEY);
  if (!data.posting_lists) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "[index-column][diff] failed to create hash table for posting lists: "
        "<%.*s>: %s",
        data.index.name_size,
        data.index.name,
        message);
    goto exit;
  }
  DB_OBJ(data.posting_lists)->header.domain = grn_obj_id(ctx, data.diff);
  data.remains = grn_column_create(ctx,
                                   data.diff,
                                   remains_column_name,
                                   (unsigned int)strlen(remains_column_name),
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
                                    (unsigned int)strlen(missings_column_name),
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

grn_obj *
grn_index_column_get_tokenizer(grn_ctx *ctx, grn_obj *index_column)
{
  grn_obj *tokenizer;
  grn_obj *lexicon;

  lexicon = grn_ctx_at(ctx, index_column->header.domain);
  if (!lexicon) {
    return NULL;
  }

  grn_table_get_info(ctx, lexicon, NULL, NULL, &tokenizer, NULL, NULL);
  grn_obj_unref(ctx, lexicon);

  return tokenizer;
}

static bool
grn_index_column_is_usable_equal(grn_ctx *ctx,
                                 grn_obj *index_column,
                                 grn_operator op)
{
  grn_obj *tokenizer;
  tokenizer = grn_index_column_get_tokenizer(ctx, index_column);
  if (!tokenizer) {
    return true;
  }
  /* TODO: Improve this. Which tokenizer can be used with equal operation?
     Do we need metadata in tokenizer? */
  if (DB_OBJ(tokenizer)->id == GRN_DB_DELIMIT) {
    return true;
  }
  return false;
}

static bool
grn_index_column_is_usable_match(grn_ctx *ctx,
                                 grn_obj *index_column,
                                 grn_operator op)
{
  return true;
}

static bool
grn_index_column_is_usable_range(grn_ctx *ctx,
                                 grn_obj *index_column,
                                 grn_operator op)
{
  grn_obj *tokenizer;
  grn_obj *lexicon;

  lexicon = grn_ctx_at(ctx, index_column->header.domain);
  if (!lexicon) {
    return false;
  }

  switch (lexicon->header.type) {
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
    break;
  default :
    grn_obj_unref(ctx, lexicon);
    return false;
  }

  grn_table_get_info(ctx, lexicon, NULL, NULL, &tokenizer, NULL, NULL);
  grn_obj_unref(ctx, lexicon);
  return tokenizer == NULL;
}

static bool
grn_index_column_is_usable_regexp(grn_ctx *ctx,
                                  grn_obj *index_column,
                                  grn_operator op)
{
  grn_obj *tokenizer;
  tokenizer = grn_index_column_get_tokenizer(ctx, index_column);
  if (!tokenizer) {
    return false;
  }
  return tokenizer == grn_ctx_get(ctx, "TokenRegexp", -1);
}

bool
grn_index_column_is_usable(grn_ctx *ctx,
                           grn_obj *index_column,
                           grn_operator op)
{
  GRN_API_ENTER;

  if (!grn_obj_is_index_column(ctx, index_column)) {
    GRN_API_RETURN(false);
  }

  if (!grn_obj_is_visible(ctx, index_column)) {
    GRN_API_RETURN(false);
  }

  bool usable = false;
  switch (op) {
  case GRN_OP_EQUAL :
  case GRN_OP_NOT_EQUAL :
    usable = grn_index_column_is_usable_equal(ctx, index_column, op);
    break;
  case GRN_OP_MATCH :
  case GRN_OP_NEAR :
  case GRN_OP_NEAR_NO_OFFSET :
  case GRN_OP_NEAR_PHRASE :
  case GRN_OP_ORDERED_NEAR_PHRASE :
  case GRN_OP_NEAR_PHRASE_PRODUCT :
  case GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT :
  case GRN_OP_SIMILAR :
  case GRN_OP_PREFIX :
  case GRN_OP_SUFFIX :
  case GRN_OP_FUZZY :
  case GRN_OP_QUORUM :
    usable = grn_index_column_is_usable_match(ctx, index_column, op);
    break;
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
  case GRN_OP_CALL :
    usable = grn_index_column_is_usable_range(ctx, index_column, op);
    break;
  case GRN_OP_REGEXP :
    usable = grn_index_column_is_usable_regexp(ctx, index_column, op);
    break;
  default :
    break;
  }

  GRN_API_RETURN(usable);
}
