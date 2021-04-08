/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2019-2021  Sutou Kouhei <kou@clear-code.com>

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
#include <string.h>
#include "grn_request_canceler.h"
#include "grn_request_timer.h"
#include "grn_tokenizers.h"
#include "grn_ctx_impl.h"
#include "grn_encoding.h"
#include "grn_ii.h"
#include "grn_pat.h"
#include "grn_index_column.h"
#include "grn_proc.h"
#include "grn_plugin.h"
#include "grn_snip.h"
#include "grn_output.h"
#include "grn_normalizer.h"
#include "grn_mrb.h"
#include "grn_ctx_impl_mrb.h"
#include "grn_ctx_impl_lua.h"
#include "grn_logger.h"
#include "grn_cache.h"
#include "grn_expr.h"
#include "grn_table_selector.h"
#include "grn_token_column.h"
#include "grn_window_function_executor.h"
#include "grn_windows.h"
#include "grn_group.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef GRN_SUPPORT_REGEXP
# include <onigmo.h>
#endif /* GRN_SUPPORT_REGEXP */

#ifdef WIN32
# include <share.h>
# include <dbghelp.h>
# ifndef MAXUSHORT
#  define MAXUSHORT 0xffff
# endif
#else /* WIN32 */
# include <netinet/in.h>
# ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
# endif /* HAVE_EXECINFO_H */
#endif /* WIN32 */

#define GRN_CTX_INITIALIZER(enc)                \
  {                                             \
    /* rc */ GRN_SUCCESS,                       \
    /* flags */ 0,                              \
    /* encoding */ enc,                         \
    /* ntrace */ 0,                             \
    /* errlvl */ GRN_LOG_NOTICE,                \
    /* stat */ GRN_CTX_FIN,                     \
    /* seqno */ 0,                              \
    /* subno */ 0,                              \
    /* seqno2 */ 0,                             \
    /* errline */ 0,                            \
    /* user_data */ {0},                        \
    /* prev */ NULL,                            \
    /* next */ NULL,                            \
    /* errfile */ NULL,                         \
    /* errfunc */ NULL,                         \
    /* impl */ NULL,                            \
    /* trace */ {0},                            \
    /* errbuf */ {0}                            \
  }

#define GRN_CTX_CLOSED(ctx) ((ctx)->stat == GRN_CTX_FIN)

grn_ctx grn_gctx = GRN_CTX_INITIALIZER(GRN_ENC_DEFAULT);
int grn_pagesize;
grn_critical_section grn_glock;
uint32_t grn_gtick;
int grn_lock_timeout = GRN_LOCK_TIMEOUT;

#ifdef USE_UYIELD
int grn_uyield_count = 0;
#endif

#ifdef WIN32
static DWORD grn_exception_filter_running = TLS_OUT_OF_INDEXES;
/* Address is important. Value is meaningless. */
static bool grn_exception_filter_running_true;
static bool grn_exception_filter_running_false;
static void
grn_exception_filter_running_init(void)
{
  grn_exception_filter_running = TlsAlloc();
}

static void
grn_exception_filter_running_fin(void)
{
  if (grn_exception_filter_running != TLS_OUT_OF_INDEXES) {
    TlsFree(grn_exception_filter_running);
    grn_exception_filter_running = TLS_OUT_OF_INDEXES;
  }
}

static bool
grn_exception_filter_running_get(void)
{
  if (grn_exception_filter_running == TLS_OUT_OF_INDEXES) {
    return false;
  }

  return TlsGetValue(grn_exception_filter_running) ==
    &grn_exception_filter_running_true;
}

static void
grn_exception_filter_running_set(bool running)
{
  if (grn_exception_filter_running == TLS_OUT_OF_INDEXES) {
    return;
  }

  if (running) {
    TlsSetValue(grn_exception_filter_running,
                &grn_exception_filter_running_true);
  } else {
    TlsSetValue(grn_exception_filter_running,
                &grn_exception_filter_running_false);
  }
}
#endif

static grn_bool grn_ctx_per_db = GRN_FALSE;

static void
grn_init_from_env(void)
{
  {
    char grn_ctx_per_db_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_CTX_PER_DB",
               grn_ctx_per_db_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ctx_per_db_env[0] && strcmp(grn_ctx_per_db_env, "yes") == 0) {
      grn_ctx_per_db = GRN_TRUE;
    }
  }

  grn_alloc_init_from_env();
  grn_mrb_init_from_env();
  grn_ctx_impl_mrb_init_from_env();
  grn_io_init_from_env();
  grn_ii_init_from_env();
  grn_db_init_from_env();
  grn_expr_init_from_env();
  grn_index_column_init_from_env();
  grn_output_init_from_env();
  grn_proc_init_from_env();
  grn_proc_query_init_from_env();
  grn_plugin_init_from_env();
  grn_token_column_init_from_env();
  grn_group_init_from_env();
  grn_window_function_executor_init_from_env();
  grn_table_selector_init_from_env();
}

static void
grn_init_external_libraries(void)
{
#ifdef GRN_SUPPORT_REGEXP
  onig_init();
#endif /*  GRN_SUPPORT_REGEXP */
}

static void
grn_fin_external_libraries(void)
{
#ifdef GRN_SUPPORT_REGEXP
  onig_end();
#endif /*  GRN_SUPPORT_REGEXP */
}

void
grn_sleep(uint32_t seconds)
{
#ifdef WIN32
  Sleep(seconds * 1000);
#else  // WIN32
  sleep(seconds);
#endif  // WIN32
}

void
grn_nanosleep(uint64_t nanoseconds)
{
#ifdef WIN32
  Sleep((DWORD)(nanoseconds / 1000000));
#else  // WIN32
  struct timespec interval;
  interval.tv_sec = (time_t)(nanoseconds / 1000000000);
  interval.tv_nsec = (long)(nanoseconds % 1000000000);
  nanosleep(&interval, NULL);
#endif  // WIN32
}

const char *
grn_get_global_error_message(void)
{
  return grn_gctx.errbuf;
}

static void
grn_loader_init(grn_loader *loader)
{
  GRN_TEXT_INIT(&loader->values, 0);
  GRN_UINT32_INIT(&loader->level, GRN_OBJ_VECTOR);
  loader->columns = NULL;
  GRN_PTR_INIT(&loader->ranges, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(&loader->indexes, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_UINT32_INIT(&loader->ids, GRN_OBJ_VECTOR);
  GRN_INT32_INIT(&loader->return_codes, GRN_OBJ_VECTOR);
  GRN_TEXT_INIT(&loader->error_messages, GRN_OBJ_VECTOR);
  loader->id_offset = -1;
  loader->key_offset = -1;
  loader->last = NULL;
  loader->table = NULL;
  loader->ifexists = NULL;
  loader->each = NULL;
  loader->values_size = 0;
  loader->n_records = 0;
  loader->n_record_errors = 0;
  loader->n_column_errors = 0;
  loader->stat = GRN_LOADER_BEGIN;
  loader->columns_status = GRN_LOADER_COLUMNS_UNSET;
  loader->rc = GRN_SUCCESS;
  loader->errbuf[0] = '\0';
  loader->output_ids = GRN_FALSE;
  loader->output_errors = GRN_FALSE;
}

void
grn_ctx_loader_clear(grn_ctx *ctx)
{
  grn_loader *loader = &ctx->impl->loader;
  grn_obj *v = (grn_obj *)(GRN_BULK_HEAD(&loader->values));
  grn_obj *ve = (grn_obj *)(GRN_BULK_CURR(&loader->values));
  if (ctx->impl->db) {
    size_t i;
    size_t n_ranges = GRN_PTR_VECTOR_SIZE(&(loader->ranges));
    for (i = 0; i < n_ranges; i++) {
      grn_obj_unlink(ctx, GRN_PTR_VALUE_AT(&(loader->ranges), i));
    }
    size_t n_indexes = GRN_PTR_VECTOR_SIZE(&(loader->indexes));
    for (i = 0; i < n_indexes; i++) {
      grn_obj_unlink(ctx, GRN_PTR_VALUE_AT(&(loader->indexes), i));
    }
    if (loader->columns) {
      GRN_HASH_EACH_BEGIN(ctx, loader->columns, cursor, id) {
        void *value;
        grn_hash_cursor_get_value(ctx, cursor, &value);
        if (value) {
          grn_obj *column = *((grn_obj **)value);
          grn_obj_unlink(ctx, column);
        }
      } GRN_HASH_EACH_END(ctx, cursor);
    }
    if (loader->table) {
      grn_obj_unlink(ctx, loader->table);
    }
  }
  if (loader->ifexists) { grn_obj_unlink(ctx, loader->ifexists); }
  if (loader->each) { grn_obj_unlink(ctx, loader->each); }
  while (v < ve) { GRN_OBJ_FIN(ctx, v++); }
  GRN_OBJ_FIN(ctx, &loader->values);
  GRN_OBJ_FIN(ctx, &loader->level);
  if (loader->columns) {
    grn_hash_close(ctx, loader->columns);
  }
  GRN_OBJ_FIN(ctx, &loader->ranges);
  GRN_OBJ_FIN(ctx, &loader->indexes);
  GRN_OBJ_FIN(ctx, &loader->ids);
  GRN_OBJ_FIN(ctx, &loader->return_codes);
  GRN_OBJ_FIN(ctx, &loader->error_messages);
#ifdef GRN_WITH_APACHE_ARROW
  if (ctx->impl->arrow_stream_loader) {
    grn_arrow_stream_loader_close(ctx, ctx->impl->arrow_stream_loader);
    ctx->impl->arrow_stream_loader = NULL;
  }
#endif
  grn_loader_init(loader);
}

#define IMPL_SIZE ((sizeof(struct _grn_ctx_impl) + (grn_pagesize - 1)) & ~(grn_pagesize - 1))

#ifdef GRN_WITH_MESSAGE_PACK
static int
grn_msgpack_buffer_write(void *data, const char *buf, msgpack_size_t len)
{
  grn_ctx *ctx = (grn_ctx *)data;
  return grn_bulk_write(ctx, ctx->impl->output.buf, buf, len);
}
#endif

typedef struct {
  void *data;
  grn_close_func close_func;
} grn_ctx_local_variable;

static grn_rc
grn_ctx_impl_init(grn_ctx *ctx)
{
  grn_io_mapinfo mi;
  if (!(ctx->impl = grn_io_anon_map(ctx, &mi, IMPL_SIZE))) {
    return ctx->rc;
  }
  grn_alloc_init_ctx_impl(ctx);
  ctx->impl->encoding = ctx->encoding;
  ctx->impl->lifoseg = -1;
  ctx->impl->currseg = -1;

  ctx->impl->values = NULL;
  ctx->impl->temporary_columns = NULL;
  ctx->impl->temporary_options = NULL;
  ctx->impl->columns_cache = NULL;
  ctx->impl->ios = NULL;
  ctx->impl->expr_vars = NULL;
  ctx->impl->stack = NULL;
  ctx->impl->current_selector_data = NULL;

  CRITICAL_SECTION_INIT(ctx->impl->lock);
  CRITICAL_SECTION_INIT(ctx->impl->columns_cache_lock);
  if (!(ctx->impl->values = grn_array_create(ctx, NULL, sizeof(grn_db_obj *),
                                             GRN_ARRAY_TINY))) {
    goto exit;
  }
  if (!(ctx->impl->temporary_columns = grn_pat_create(ctx, NULL,
                                                      GRN_TABLE_MAX_KEY_SIZE,
                                                      sizeof(grn_obj *),
                                                      0))) {
    goto exit;
  }
  ctx->impl->temporary_options =
    grn_options_create(ctx, NULL, "[ctx][impl][create]");
  if (!ctx->impl->temporary_options) {
    goto exit;
  }
  if (!(ctx->impl->columns_cache = grn_hash_create(ctx,
                                                   NULL,
                                                   sizeof(grn_id),
                                                   sizeof(grn_hash *),
                                                   GRN_OBJ_TABLE_HASH_KEY|
                                                   GRN_HASH_TINY))) {
    goto exit;
  }
  if (!(ctx->impl->ios = grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE,
                                         sizeof(grn_io *),
                                         GRN_OBJ_KEY_VAR_SIZE|GRN_HASH_TINY))) {
    goto exit;
  }
  ctx->impl->db = NULL;

  if (!(ctx->impl->expr_vars = grn_hash_create(ctx, NULL, sizeof(grn_id),
                                               sizeof(grn_obj *), 0))) {
    goto exit;
  }
  if (!(ctx->impl->stack = GRN_MALLOCN(grn_obj *, GRN_STACK_SIZE))) {
    goto exit;
  }
  ctx->impl->stack_curr = 0;
  ctx->impl->stack_size = GRN_STACK_SIZE;
  ctx->impl->curr_expr = NULL;
  GRN_TEXT_INIT(&ctx->impl->current_request_id, 0);
  ctx->impl->current_request_timer_id = NULL;
  ctx->impl->parser = NULL;

  GRN_TEXT_INIT(&ctx->impl->output.names, GRN_OBJ_VECTOR);
  GRN_UINT32_INIT(&ctx->impl->output.levels, GRN_OBJ_VECTOR);

  ctx->impl->command.flags = 0;
  if (ctx == &grn_gctx) {
    ctx->impl->command.version = GRN_COMMAND_VERSION_STABLE;
  } else {
    ctx->impl->command.version = grn_get_default_command_version();
  }
  ctx->impl->command.keep.command = NULL;
  ctx->impl->command.keep.version = ctx->impl->command.version;

  if (ctx == &grn_gctx) {
    ctx->impl->match_escalation_threshold =
      GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD;
  } else {
    ctx->impl->match_escalation_threshold =
      grn_get_default_match_escalation_threshold();
  }
  ctx->impl->force_match_escalation = GRN_FALSE;

  ctx->impl->finalizer = NULL;

  ctx->impl->com = NULL;
  ctx->impl->output.buf = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  ctx->impl->output.func = NULL;
  ctx->impl->output.data.ptr = NULL;
#ifdef GRN_WITH_MESSAGE_PACK
  msgpack_packer_init(&ctx->impl->output.msgpacker,
                      ctx, grn_msgpack_buffer_write);
#endif
  ctx->impl->output.arrow_stream_writer = NULL;
  grn_timeval_now(ctx, &ctx->impl->tv);
  ctx->impl->edge = NULL;
#ifdef GRN_WITH_APACHE_ARROW
  ctx->impl->arrow_stream_loader = NULL;
#endif
  grn_loader_init(&ctx->impl->loader);
  ctx->impl->plugin_path = NULL;

  GRN_TEXT_INIT(&ctx->impl->query_log_buf, 0);

  ctx->impl->previous_errbuf[0] = '\0';
  ctx->impl->n_same_error_messages = 0;

  grn_ctx_impl_mrb_init(ctx);
  grn_ctx_impl_lua_init(ctx);

  GRN_TEXT_INIT(&(ctx->impl->temporary_open_spaces.stack), 0);
  ctx->impl->temporary_open_spaces.current = NULL;

  ctx->impl->variables = grn_hash_create(ctx,
                                         NULL,
                                         GRN_TABLE_MAX_KEY_SIZE,
                                         sizeof(grn_ctx_local_variable),
                                         GRN_OBJ_KEY_VAR_SIZE|GRN_HASH_TINY);
  if (!ctx->impl->variables) {
    goto exit;
  }

  CRITICAL_SECTION_INIT(ctx->impl->children.lock);
  /* grn_obj * isn't grn_ctx * but sizeof(grn_obj *) and
   * sizeof(grn_ctx *) is same. So we reuse GRN_PVECTOR here. */
  GRN_PTR_INIT(&(ctx->impl->children.pool), GRN_OBJ_VECTOR, GRN_ID_NIL);
  ctx->impl->parent = NULL;

exit :
  if (ctx->rc != GRN_SUCCESS) {
    if (ctx->impl->variables) {
      grn_hash_close(ctx, ctx->impl->variables);
    }
    GRN_OBJ_FIN(ctx, &(ctx->impl->temporary_open_spaces.stack));
    GRN_OBJ_FIN(ctx, &ctx->impl->query_log_buf);
    grn_obj_close(ctx, ctx->impl->output.buf);
    GRN_OBJ_FIN(ctx, &ctx->impl->output.levels);
    GRN_OBJ_FIN(ctx, &ctx->impl->output.names);
    GRN_OBJ_FIN(ctx, &ctx->impl->current_request_id);
    if (ctx->impl->stack) {
      GRN_FREE(ctx->impl->stack);
    }
    if (ctx->impl->expr_vars) {
      grn_hash_close(ctx, ctx->impl->expr_vars);
    }
    if (ctx->impl->ios) {
      grn_hash_close(ctx, ctx->impl->ios);
    }
    if (ctx->impl->columns_cache) {
      grn_hash_close(ctx, ctx->impl->columns_cache);
    }
    if (ctx->impl->temporary_options) {
      grn_options_close(ctx, ctx->impl->temporary_options);
    }
    if (ctx->impl->temporary_columns) {
      grn_pat_close(ctx, ctx->impl->temporary_columns);
    }
    if (ctx->impl->values) {
      grn_array_close(ctx, ctx->impl->values);
    }
    CRITICAL_SECTION_FIN(ctx->impl->columns_cache_lock);
    CRITICAL_SECTION_FIN(ctx->impl->lock);
    grn_io_anon_unmap(ctx, &mi, IMPL_SIZE);
    ctx->impl = NULL;
  }

  return ctx->rc;
}

void
grn_ctx_set_keep_command(grn_ctx *ctx, grn_obj *command)
{
  ctx->impl->command.keep.command = command;
  ctx->impl->command.keep.version = ctx->impl->command.version;
}

grn_ctx *
grn_ctx_pull_child(grn_ctx *ctx)
{
  grn_ctx *child_ctx = NULL;
  CRITICAL_SECTION_ENTER(ctx->impl->children.lock);
  if (GRN_PTR_VECTOR_SIZE(&(ctx->impl->children.pool)) == 0) {
    child_ctx = grn_ctx_open(0);
  } else {
    grn_obj *value;
    GRN_PTR_POP(&(ctx->impl->children.pool), value);
    child_ctx = (grn_ctx *)value;
  }
  grn_ctx_use(child_ctx, grn_ctx_db(ctx));
  child_ctx->impl->parent = ctx;
  CRITICAL_SECTION_LEAVE(ctx->impl->children.lock);
  return child_ctx;
}

grn_rc
grn_ctx_release_child(grn_ctx *ctx, grn_ctx *child_ctx)
{
  if (child_ctx->impl->parent != ctx) {
    return GRN_INVALID_ARGUMENT;
  }

  CRITICAL_SECTION_ENTER(ctx->impl->children.lock);
  if (ctx->rc == GRN_SUCCESS && child_ctx->rc != GRN_SUCCESS) {
    ctx->rc = child_ctx->rc;
    ctx->errlvl = child_ctx->errlvl;
    ctx->errfile = child_ctx->errfile;
    ctx->errline = child_ctx->errline;
    ctx->errfunc = child_ctx->errfunc;
    grn_strcpy(ctx->errbuf, GRN_CTX_MSGSIZE, child_ctx->errbuf);
  }
  ERRCLR(child_ctx);
  child_ctx->impl->parent = NULL;
  grn_ctx_use(child_ctx, NULL);
  GRN_PTR_PUT(ctx, &(ctx->impl->children.pool), child_ctx);
  CRITICAL_SECTION_LEAVE(ctx->impl->children.lock);
  return ctx->rc;
}

static void
grn_ctx_impl_clear_n_same_error_messagges(grn_ctx *ctx)
{
  if (ctx->impl->n_same_error_messages == 0) {
    return;
  }

  GRN_LOG(ctx, GRN_LOG_NOTICE, "(%u same messages are truncated)",
          ctx->impl->n_same_error_messages);
  ctx->impl->n_same_error_messages = 0;
}

grn_bool
grn_ctx_impl_should_log(grn_ctx *ctx)
{
  if (!ctx->impl) {
    return GRN_TRUE;
  }

  if (strcmp(ctx->errbuf, ctx->impl->previous_errbuf) == 0) {
    ctx->impl->n_same_error_messages++;
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

void
grn_ctx_impl_set_current_error_message(grn_ctx *ctx)
{
  if (!ctx->impl) {
    return;
  }

  grn_ctx_impl_clear_n_same_error_messagges(ctx);
  grn_strcpy(ctx->impl->previous_errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
}

void
grn_ctx_impl_columns_cache_clear(grn_ctx *ctx)
{
  CRITICAL_SECTION_ENTER(ctx->impl->columns_cache_lock);
  GRN_HASH_EACH_BEGIN(ctx, ctx->impl->columns_cache, cursor, id) {
    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    grn_hash *columns = *((grn_hash **)value);
    grn_hash_close(ctx, columns);
    grn_hash_cursor_delete(ctx, cursor, NULL);
  } GRN_HASH_EACH_END(ctx, cursor);
  CRITICAL_SECTION_LEAVE(ctx->impl->columns_cache_lock);
}

void
grn_ctx_impl_columns_cache_delete(grn_ctx *ctx, grn_id table_id)
{
  CRITICAL_SECTION_ENTER(ctx->impl->columns_cache_lock);
  grn_id cache_id;
  void *value;
  cache_id = grn_hash_get(ctx,
                          ctx->impl->columns_cache,
                          &table_id,
                          sizeof(grn_id),
                          &value);
  if (cache_id != GRN_ID_NIL) {
    grn_hash *columns = *((grn_hash **)value);
    grn_hash_close(ctx, columns);
    grn_hash_delete_by_id(ctx, ctx->impl->columns_cache, cache_id, NULL);
  }
  CRITICAL_SECTION_LEAVE(ctx->impl->columns_cache_lock);
}

static grn_rc
grn_ctx_impl_fin(grn_ctx *ctx)
{
  grn_rc rc;

  grn_ctx_impl_clear_n_same_error_messagges(ctx);
  if (ctx->impl->finalizer) {
    ctx->impl->finalizer(ctx, 0, NULL, &(ctx->user_data));
  }

  {
    while (GRN_PTR_VECTOR_SIZE(&(ctx->impl->children.pool)) > 0) {
      grn_obj *value;
      GRN_PTR_POP(&(ctx->impl->children.pool), value);
      grn_ctx *child_ctx = (grn_ctx *)value;
      grn_ctx_close(child_ctx);
    }
    GRN_OBJ_FIN(ctx, &(ctx->impl->children.pool));
    CRITICAL_SECTION_FIN(ctx->impl->children.lock);
  }

  GRN_HASH_EACH_BEGIN(ctx, ctx->impl->variables, cursor, id) {
    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    grn_ctx_local_variable *variable = value;
    if (variable->close_func) {
      variable->close_func(ctx, variable->data);
    }
  } GRN_HASH_EACH_END(ctx, cursor);
  grn_hash_close(ctx, ctx->impl->variables);
  {
    grn_obj *stack;
    grn_obj *spaces;
    unsigned int i, n_spaces;

    stack = &(ctx->impl->temporary_open_spaces.stack);
    spaces = (grn_obj *)GRN_BULK_HEAD(stack);
    n_spaces = GRN_BULK_VSIZE(stack) / sizeof(grn_obj);
    for (i = 0; i < n_spaces; i++) {
      grn_obj *space = spaces + (n_spaces - i - 1);
      GRN_OBJ_FIN(ctx, space);
    }
    GRN_OBJ_FIN(ctx, stack);
  }
  grn_ctx_impl_lua_fin(ctx);
  grn_ctx_impl_mrb_fin(ctx);
  grn_ctx_loader_clear(ctx);
  if (ctx->impl->parser) {
    grn_expr_parser_close(ctx);
  }
  GRN_OBJ_FIN(ctx, &ctx->impl->current_request_id);
  if (ctx->impl->values) {
#ifndef USE_MEMORY_DEBUG
    grn_db_obj *o;
    GRN_ARRAY_EACH(ctx, ctx->impl->values, 0, 0, id, &o, {
      grn_obj_close(ctx, *((grn_obj **)o));
    });
#endif
    grn_array_close(ctx, ctx->impl->values);
  }
  if (ctx->impl->temporary_columns) {
#ifndef USE_MEMORY_DEBUG
    grn_obj *value;
    GRN_PAT_EACH(ctx, ctx->impl->temporary_columns, id, NULL, NULL, &value, {
      grn_obj_close(ctx, *((grn_obj **)value));
    });
#endif
    grn_pat_close(ctx, ctx->impl->temporary_columns);
  }
  if (ctx->impl->temporary_options) {
    grn_options_close(ctx, ctx->impl->temporary_options);
  }
  if (ctx->impl->columns_cache) {
    grn_ctx_impl_columns_cache_clear(ctx);
    grn_hash_close(ctx, ctx->impl->columns_cache);
  }
  if (ctx->impl->ios) {
    grn_hash_close(ctx, ctx->impl->ios);
  }
  if (ctx->impl->com) {
    if (ctx->stat != GRN_CTX_QUIT) {
      int flags;
      char *str;
      unsigned int str_len;
      grn_ctx_send(ctx, "quit", 4, GRN_CTX_HEAD);
      grn_ctx_recv(ctx, &str, &str_len, &flags);
    }
    grn_ctx_send(ctx, "ACK", 3, GRN_CTX_HEAD);
    rc = grn_com_close(ctx, ctx->impl->com);
  }
  GRN_OBJ_FIN(ctx, &ctx->impl->query_log_buf);
  if (ctx->impl->output.arrow_stream_writer) {
    grn_arrow_stream_writer_close(ctx, ctx->impl->output.arrow_stream_writer);
  }
  GRN_OBJ_FIN(ctx, &ctx->impl->output.names);
  GRN_OBJ_FIN(ctx, &ctx->impl->output.levels);
  rc = grn_obj_close(ctx, ctx->impl->output.buf);
  if (ctx->impl->expr_vars) {
    grn_hash **vp;
    grn_obj *value;
    GRN_HASH_EACH(ctx, ctx->impl->expr_vars, eid, NULL, NULL, &vp, {
      if (*vp) {
        GRN_HASH_EACH(ctx, *vp, id, NULL, NULL, &value, {
          GRN_OBJ_FIN(ctx, value);
        });
      }
      grn_hash_close(ctx, *vp);
    });
    grn_hash_close(ctx, ctx->impl->expr_vars);
  }
  if (ctx->impl->stack) {
    GRN_FREE(ctx->impl->stack);
  }
  if (ctx->impl->db && ctx->flags & GRN_CTX_PER_DB) {
    grn_obj *db = ctx->impl->db;
    ctx->impl->db = NULL;
    grn_obj_close(ctx, db);
  }
  grn_alloc_fin_ctx_impl(ctx);
  grn_alloc_info_dump(ctx);
  grn_alloc_info_free(ctx);
  CRITICAL_SECTION_FIN(ctx->impl->columns_cache_lock);
  CRITICAL_SECTION_FIN(ctx->impl->lock);
  {
    grn_io_mapinfo mi;
    mi.map = (void *)ctx->impl;
    grn_io_anon_unmap(ctx, &mi, IMPL_SIZE);
  }

  return rc;
}

static grn_rc
grn_ctx_init_internal(grn_ctx *ctx, int flags)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  // if (ctx->stat != GRN_CTX_FIN) { return GRN_INVALID_ARGUMENT; }
  ctx->rc = GRN_SUCCESS;
  ERRCLR(ctx);
  ctx->flags = flags;
  if (grn_ctx_per_db) {
    ctx->flags |= GRN_CTX_PER_DB;
  }
  ctx->stat = GRN_CTX_INITED;
  ctx->encoding = grn_gctx.encoding;
  ctx->seqno = 0;
  ctx->seqno2 = 0;
  ctx->subno = 0;
  ctx->impl = NULL;
  ctx->user_data.ptr = NULL;
  CRITICAL_SECTION_ENTER(grn_glock);
  ctx->next = grn_gctx.next;
  ctx->prev = &grn_gctx;
  grn_gctx.next->prev = ctx;
  grn_gctx.next = ctx;
  CRITICAL_SECTION_LEAVE(grn_glock);
  ctx->errline = 0;
  ctx->errfile = "";
  ctx->errfunc = "";
  ctx->trace[0] = NULL;
  ctx->errbuf[0] = '\0';
  return GRN_SUCCESS;
}

grn_rc
grn_ctx_init(grn_ctx *ctx, int flags)
{
  grn_rc rc;

  rc = grn_ctx_init_internal(ctx, flags);
  if (rc == GRN_SUCCESS) {
    grn_ctx_impl_init(ctx);
    rc = ctx->rc;
    if (rc != GRN_SUCCESS) {
      grn_ctx_fin(ctx);
      if (flags & GRN_CTX_ALLOCATED) {
        CRITICAL_SECTION_ENTER(grn_glock);
        ctx->next->prev = ctx->prev;
        ctx->prev->next = ctx->next;
        CRITICAL_SECTION_LEAVE(grn_glock);
      }
    }
  }

  return rc;
}

grn_ctx *
grn_ctx_open(int flags)
{
  grn_ctx *ctx = GRN_GMALLOCN(grn_ctx, 1);
  if (ctx) {
    grn_ctx_init(ctx, flags|GRN_CTX_ALLOCATED);
    if (ERRP(ctx, GRN_ERROR)) {
      GRN_GFREE(ctx);
      ctx = NULL;
    }
  }
  return ctx;
}

grn_rc
grn_ctx_fin(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  if (ctx->stat == GRN_CTX_FIN) { return GRN_INVALID_ARGUMENT; }
  if (!(ctx->flags & GRN_CTX_ALLOCATED)) {
    CRITICAL_SECTION_ENTER(grn_glock);
    ctx->next->prev = ctx->prev;
    ctx->prev->next = ctx->next;
    CRITICAL_SECTION_LEAVE(grn_glock);
  }
  if (ctx->impl) {
    rc = grn_ctx_impl_fin(ctx);
    ctx->impl = NULL;
  }
  ctx->stat = GRN_CTX_FIN;
  return rc;
}

grn_rc
grn_ctx_set_finalizer(grn_ctx *ctx, grn_proc_func *finalizer)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  if (!ctx->impl) {
    if (ERRP(ctx, GRN_ERROR)) { return ctx->rc; }
  }
  ctx->impl->finalizer = finalizer;
  return GRN_SUCCESS;
}

grn_timeval grn_starttime;

static void
check_overcommit_memory(grn_ctx *ctx)
{
  FILE *file;
  int value;
  file = grn_fopen("/proc/sys/vm/overcommit_memory", "r");
  if (!file) { return; }
  value = fgetc(file);
  if (value != '1') {
    GRN_LOG(ctx, GRN_LOG_NOTICE,
            "vm.overcommit_memory kernel parameter should be 1: <%c>: "
            "See INFO level log to resolve this",
            value);
    GRN_LOG(ctx, GRN_LOG_INFO,
            "Some processings with vm.overcommit_memory != 1 "
            "may break DB under low memory condition.");
    GRN_LOG(ctx, GRN_LOG_INFO,
            "To set vm.overcommit_memory to 1");
    GRN_LOG(ctx, GRN_LOG_INFO,
            "add 'vm.overcommit_memory = 1' to /etc/sysctl.conf and "
            "restart your system or");
    GRN_LOG(ctx, GRN_LOG_INFO,
            "run 'sudo /sbin/sysctl vm.overcommit_memory=1' command.");
  }
  fclose(file);
}

grn_rc
grn_init(void)
{
  grn_rc rc;
  grn_ctx *ctx = &grn_gctx;
#ifdef WIN32
  grn_exception_filter_running_init();
  grn_windows_init();
#endif
  grn_init_from_env();
  grn_init_external_libraries();
  grn_alloc_info_init();
  grn_logger_init();
  grn_query_logger_init();
  CRITICAL_SECTION_INIT(grn_glock);
  grn_gtick = 0;
  ctx->next = ctx;
  ctx->prev = ctx;
  rc = grn_ctx_init_internal(ctx, 0);
  if (rc) {
    goto fail_ctx_init_internal;
  }
  ctx->encoding = grn_encoding_parse(GRN_DEFAULT_ENCODING);
  rc = grn_timeval_now(ctx, &grn_starttime);
  if (rc) {
    goto fail_start_time;
  }
#ifdef WIN32
  {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    grn_pagesize = si.dwAllocationGranularity;
  }
#else /* WIN32 */
  if ((grn_pagesize = sysconf(_SC_PAGESIZE)) == -1) {
    SERR("_SC_PAGESIZE");
    rc = ctx->rc;
    goto fail_page_size;
  }
#endif /* WIN32 */
  if (grn_pagesize & (grn_pagesize - 1)) {
    GRN_LOG(ctx, GRN_LOG_CRIT, "pagesize=%x", grn_pagesize);
  }
  // expand_stack();
  if ((rc = grn_com_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_com_init failed (%d)", rc);
    goto fail_com;
  }
  if ((rc = grn_ctx_impl_init(ctx))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_ctx_impl_init failed (%d)", rc);
    goto fail_ctx_impl;
  }
  if ((rc = grn_plugins_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_plugins_init failed (%d)", rc);
    goto fail_plugins;
  }
  if ((rc = grn_normalizer_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_normalizer_init failed (%d)", rc);
    goto fail_normalizer;
  }
  if ((rc = grn_tokenizers_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_tokenizers_init failed (%d)", rc);
    goto fail_tokenizer;
  }
  grn_cache_init();
  if (!grn_request_canceler_init()) {
    rc = ctx->rc;
    GRN_LOG(ctx, GRN_LOG_ALERT,
            "failed to initialize request canceler (%d)", rc);
    goto fail_request_canceler;
  }
  if (!grn_request_timer_init()) {
    rc = ctx->rc;
    GRN_LOG(ctx, GRN_LOG_ALERT,
            "failed to initialize request timer (%d)", rc);
    goto fail_request_timer;
  }
  GRN_LOG(ctx, GRN_LOG_NOTICE, "grn_init: <%s>", grn_get_version());
  check_overcommit_memory(ctx);
  return rc;

fail_request_timer:
  grn_request_canceler_fin();
fail_request_canceler:
  grn_cache_fin();
fail_tokenizer:
  grn_normalizer_fin();
fail_normalizer:
  grn_plugins_fin();
fail_plugins:
  grn_ctx_fin(ctx);
fail_ctx_impl:
  grn_com_fin();
fail_com:
#ifndef WIN32
fail_page_size:
#endif /* WIN32 */
fail_start_time:
fail_ctx_init_internal:
  GRN_LOG(ctx, GRN_LOG_NOTICE, "grn_init: <%s>: failed", grn_get_version());
  grn_query_logger_fin(ctx);
  grn_logger_fin(ctx);
  CRITICAL_SECTION_FIN(grn_glock);
  grn_alloc_info_fin();
  grn_fin_external_libraries();
#ifdef WIN32
  grn_windows_fin();
  grn_exception_filter_running_fin();
#endif
  return rc;
}

grn_encoding
grn_get_default_encoding(void)
{
  return grn_gctx.encoding;
}

grn_rc
grn_set_default_encoding(grn_encoding encoding)
{
  switch (encoding) {
  case GRN_ENC_DEFAULT :
    grn_gctx.encoding = grn_encoding_parse(GRN_DEFAULT_ENCODING);
    return GRN_SUCCESS;
  case GRN_ENC_NONE :
  case GRN_ENC_EUC_JP :
  case GRN_ENC_UTF8 :
  case GRN_ENC_SJIS :
  case GRN_ENC_LATIN1 :
  case GRN_ENC_KOI8R :
    grn_gctx.encoding = encoding;
    return GRN_SUCCESS;
  default :
    return GRN_INVALID_ARGUMENT;
  }
}

grn_command_version
grn_get_default_command_version(void)
{
  return grn_ctx_get_command_version(&grn_gctx);
}

grn_rc
grn_set_default_command_version(grn_command_version version)
{
  return grn_ctx_set_command_version(&grn_gctx, version);
}

long long int
grn_get_default_match_escalation_threshold(void)
{
  return grn_ctx_get_match_escalation_threshold(&grn_gctx);
}

grn_rc
grn_set_default_match_escalation_threshold(long long int threshold)
{
  return grn_ctx_set_match_escalation_threshold(&grn_gctx, threshold);
}

int
grn_get_lock_timeout(void)
{
  return grn_lock_timeout;
}

grn_rc
grn_set_lock_timeout(int timeout)
{
  grn_lock_timeout = timeout;
  return GRN_SUCCESS;
}

grn_rc
grn_fin(void)
{
  grn_ctx *ctx, *ctx_;
  if (grn_gctx.stat == GRN_CTX_FIN) { return GRN_INVALID_ARGUMENT; }
  /* Clear ctx pool to avoid double free. */
  for (ctx = grn_gctx.next; ctx != &grn_gctx; ctx = ctx->next) {
    if (!ctx->impl) {
      continue;
    }
    if (GRN_PTR_VECTOR_SIZE(&(ctx->impl->children.pool)) == 0) {
      continue;
    }
    GRN_BULK_REWIND(&(ctx->impl->children.pool));
  }
  for (ctx = grn_gctx.next; ctx != &grn_gctx; ctx = ctx_) {
    ctx_ = ctx->next;
    if (ctx->stat != GRN_CTX_FIN) { grn_ctx_fin(ctx); }
    if (ctx->flags & GRN_CTX_ALLOCATED) {
      ctx->next->prev = ctx->prev;
      ctx->prev->next = ctx->next;
      GRN_GFREE(ctx);
    }
  }
  grn_query_logger_fin(ctx);
  grn_request_timer_fin();
  grn_request_canceler_fin();
  grn_cache_fin();
  grn_tokenizers_fin();
  grn_normalizer_fin();
  grn_plugins_fin();
  grn_ctx_fin(ctx);
  grn_com_fin();
  GRN_LOG(ctx, GRN_LOG_NOTICE, "grn_fin (%d)", grn_alloc_count());
  grn_logger_fin(ctx);
  CRITICAL_SECTION_FIN(grn_glock);
  grn_alloc_info_fin();
  grn_fin_external_libraries();
#ifdef WIN32
  grn_windows_fin();
  grn_exception_filter_running_fin();
#endif
  return GRN_SUCCESS;
}

grn_rc
grn_ctx_connect(grn_ctx *ctx, const char *host, int port, int flags)
{
  GRN_API_ENTER;
  if (!ctx->impl) { goto exit; }
  {
    grn_com *com = grn_com_copen(ctx, NULL, host, port);
    if (com) {
      ctx->impl->com = com;
    }
  }
exit :
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_ctx_close(grn_ctx *ctx)
{
  grn_rc rc = grn_ctx_fin(ctx);
  CRITICAL_SECTION_ENTER(grn_glock);
  ctx->next->prev = ctx->prev;
  ctx->prev->next = ctx->next;
  CRITICAL_SECTION_LEAVE(grn_glock);
  GRN_GFREE(ctx);
  return rc;
}

grn_command_version
grn_ctx_get_command_version(grn_ctx *ctx)
{
  if (ctx->impl) {
    return ctx->impl->command.version;
  } else {
    return GRN_COMMAND_VERSION_STABLE;
  }
}

grn_rc
grn_ctx_set_command_version(grn_ctx *ctx, grn_command_version version)
{
  switch (version) {
  case GRN_COMMAND_VERSION_DEFAULT :
    ctx->impl->command.version = GRN_COMMAND_VERSION_STABLE;
    return GRN_SUCCESS;
  default :
    if (GRN_COMMAND_VERSION_MIN <= version &&
        version <= GRN_COMMAND_VERSION_MAX) {
      ctx->impl->command.version = version;
      return GRN_SUCCESS;
    } else {
      return GRN_UNSUPPORTED_COMMAND_VERSION;
    }
  }
}

grn_content_type
grn_ctx_get_output_type(grn_ctx *ctx)
{
  if (ctx->impl) {
    return ctx->impl->output.type;
  } else {
    return GRN_CONTENT_NONE;
  }
}

grn_rc
grn_ctx_set_output_type(grn_ctx *ctx, grn_content_type type)
{
  grn_rc rc = GRN_SUCCESS;

  if (ctx->impl) {
    ctx->impl->output.type = type;
    switch (ctx->impl->output.type) {
    case GRN_CONTENT_NONE :
      ctx->impl->output.mime_type = "application/octet-stream";
      break;
    case GRN_CONTENT_TSV :
      ctx->impl->output.mime_type = "text/tab-separated-values";
      break;
    case GRN_CONTENT_JSON :
      ctx->impl->output.mime_type = "application/json";
      break;
    case GRN_CONTENT_XML :
      ctx->impl->output.mime_type = "text/xml";
      break;
    case GRN_CONTENT_MSGPACK :
      ctx->impl->output.mime_type = "application/x-msgpack";
      break;
    case GRN_CONTENT_GROONGA_COMMAND_LIST :
      ctx->impl->output.mime_type = "text/x-groonga-command-list";
      break;
    case GRN_CONTENT_APACHE_ARROW :
      ctx->impl->output.mime_type = "application/x-apache-arrow-streaming";
      break;
    }
  } else {
    rc = GRN_INVALID_ARGUMENT;
  }

  return rc;
}

const char *
grn_ctx_get_mime_type(grn_ctx *ctx)
{
  if (ctx->impl) {
    return ctx->impl->output.mime_type;
  } else {
    return NULL;
  }
}

long long int
grn_ctx_get_match_escalation_threshold(grn_ctx *ctx)
{
  if (ctx->impl) {
    return ctx->impl->match_escalation_threshold;
  } else {
    return GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD;
  }
}

grn_rc
grn_ctx_set_match_escalation_threshold(grn_ctx *ctx, long long int threshold)
{
  ctx->impl->match_escalation_threshold = threshold;
  return GRN_SUCCESS;
}

grn_bool
grn_ctx_get_force_match_escalation(grn_ctx *ctx)
{
  if (ctx->impl) {
    return ctx->impl->force_match_escalation;
  } else {
    return GRN_FALSE;
  }
}

grn_rc
grn_ctx_set_force_match_escalation(grn_ctx *ctx, grn_bool force)
{
  if (ctx->impl) {
    ctx->impl->force_match_escalation = force;
    return GRN_SUCCESS;
  } else {
    return GRN_INVALID_ARGUMENT;
  }
}

grn_rc
grn_ctx_set_variable(grn_ctx *ctx,
                     const char *name,
                     int name_size,
                     void *data,
                     grn_close_func close_func)
{
  GRN_API_ENTER;
  if (!ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT,
        "[ctx][variable][set] not initialized ctx");
    goto exit;
  }
  if (name_size < 0) {
    name_size = strlen(name);
  }
  if (data) {
    void *value;
    int added;
    grn_id id = grn_hash_add(ctx,
                             ctx->impl->variables,
                             name,
                             name_size,
                             &value,
                             &added);
    if (id == GRN_ID_NIL) {
      grn_rc rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_UNKNOWN_ERROR;
      }
      ERR(rc, "[ctx][variable][set] failed to add variable");
      goto exit;
    }
    grn_ctx_local_variable *variable = value;
    if (!added) {
      if (variable->close_func) {
        variable->close_func(ctx, variable->data);
      }
    }
    variable->data = data;
    variable->close_func = close_func;
  } else {
    void *value;
    grn_id id = grn_hash_get(ctx,
                             ctx->impl->variables,
                             name,
                             name_size,
                             &value);
    if (id != GRN_ID_NIL) {
      grn_ctx_local_variable *variable = value;
      if (variable->close_func) {
        variable->close_func(ctx, variable->data);
      }
      grn_hash_delete_by_id(ctx, ctx->impl->variables, id, NULL);
    }
  }
exit :
  GRN_API_RETURN(ctx->rc);
}

void *
grn_ctx_get_variable(grn_ctx *ctx,
                     const char *name,
                     int name_size)
{
  void *data = NULL;
  GRN_API_ENTER;
  if (!ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT,
        "[ctx][variable][get] not initialized ctx");
    goto exit;
  }
  if (name_size < 0) {
    name_size = strlen(name);
  }
  void *value;
  grn_id id = grn_hash_get(ctx,
                           ctx->impl->variables,
                           name,
                           name_size,
                           &value);
  if (id == GRN_ID_NIL) {
    goto exit;
  }
  grn_ctx_local_variable *variable = value;
  data = variable->data;
exit :
  GRN_API_RETURN(data);
}

grn_rc
grn_unset_variable(const char *name,
                   int name_size)
{
  grn_rc rc = GRN_SUCCESS;
  grn_ctx *ctx = grn_gctx.next;
  while (ctx != &grn_gctx) {
    if (ctx->impl) {
      rc = grn_ctx_set_variable(ctx, name, name_size, NULL, NULL);
      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    CRITICAL_SECTION_ENTER(grn_glock);
    ctx = ctx->next;
    CRITICAL_SECTION_LEAVE(grn_glock);
  }
  return rc;
}

grn_content_type
grn_get_ctype(grn_obj *var)
{
  return grn_content_type_parse(NULL, var, GRN_CONTENT_JSON);
}

grn_content_type
grn_content_type_parse(grn_ctx *ctx,
                       grn_obj *var,
                       grn_content_type default_value)
{
  grn_content_type ct = default_value;
  if (var->header.domain == GRN_DB_INT32) {
    ct = GRN_INT32_VALUE(var);
  } else {
    grn_raw_string input;
    GRN_RAW_STRING_SET(input, var);
    if (GRN_RAW_STRING_EQUAL_CSTRING_CI(input, "tsv")) {
      ct = GRN_CONTENT_TSV;
    } else if (GRN_RAW_STRING_EQUAL_CSTRING_CI(input, "json")) {
      ct = GRN_CONTENT_JSON;
    } else if (GRN_RAW_STRING_EQUAL_CSTRING_CI(input, "xml")) {
      ct = GRN_CONTENT_XML;
    } else if (GRN_RAW_STRING_EQUAL_CSTRING_CI(input, "apache-arrow")) {
      ct = GRN_CONTENT_APACHE_ARROW;
    }
  }
  return ct;
}

static void
get_content_mime_type(grn_ctx *ctx, const char *p, const char *pe)
{
  ctx->impl->output.type = GRN_CONTENT_NONE;
  ctx->impl->output.mime_type = "application/octet-stream";

  grn_raw_string type;
  type.value = p;
  type.length = pe - p;

  if (type.length < 2) {
    return;
  }

  switch (type.value[0]) {
  case 'c' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "css")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "text/css";
    }
    break;
  case 'g' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "gif")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "image/gif";
    }
    break;
  case 'h' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "html")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "text/html";
    }
    break;
  case 'j' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "js")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "text/javascript";
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(type, "json")) {
      ctx->impl->output.type = GRN_CONTENT_JSON;
      ctx->impl->output.mime_type = "application/json";
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(type, "jpg") ||
               GRN_RAW_STRING_EQUAL_CSTRING(type, "jpeg")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "image/jpeg";
    }
    break;
#ifdef GRN_WITH_MESSAGE_PACK
  case 'm' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "msgpack")) {
      ctx->impl->output.type = GRN_CONTENT_MSGPACK;
      ctx->impl->output.mime_type = "application/x-msgpack";
    }
    break;
#endif
#ifdef GRN_WITH_APACHE_ARROW
  case 'a' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "arrow") ||
        GRN_RAW_STRING_EQUAL_CSTRING(type, "apache-arrow")) {
      ctx->impl->output.type = GRN_CONTENT_APACHE_ARROW;
      ctx->impl->output.mime_type = "application/x-apache-arrow-streaming";
    }
    break;
#endif
  case 'p' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "png")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "image/png";
    }
    break;
  case 't' :
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "txt")) {
      ctx->impl->output.type = GRN_CONTENT_NONE;
      ctx->impl->output.mime_type = "text/plain";
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(type, "tsv")) {
      ctx->impl->output.type = GRN_CONTENT_TSV;
      ctx->impl->output.mime_type = "text/tab-separated-values";
    }
    break;
  case 'x':
    if (GRN_RAW_STRING_EQUAL_CSTRING(type, "xml")) {
      ctx->impl->output.type = GRN_CONTENT_XML;
      ctx->impl->output.mime_type = "text/xml";
    }
    break;
  }
}

static void
grn_str_get_mime_type(grn_ctx *ctx, const char *p, const char *pe,
                     const char **key_end, const char **filename_end)
{
  const char *pd = NULL;
  for (; p < pe && *p != '?' && *p != '#'; p++) {
    if (*p == '.') { pd = p; }
  }
  *filename_end = p;
  if (pd && pd < p) {
    get_content_mime_type(ctx, pd + 1, p);
    *key_end = pd;
  } else {
    *key_end = pe;
  }
}

static void
get_command_version(grn_ctx *ctx, const char *p, const char *pe)
{
  grn_command_version version;
  const char *rest;

  version = grn_atoui(p, pe, &rest);
  if (pe == rest) {
    grn_rc rc;
    rc = grn_ctx_set_command_version(ctx, version);
    if (rc == GRN_UNSUPPORTED_COMMAND_VERSION) {
      ERR(rc,
          "unsupported command version is specified: %d: "
          "stable command version: %d: "
          "available command versions: %d-%d",
          version,
          GRN_COMMAND_VERSION_STABLE,
          GRN_COMMAND_VERSION_MIN, GRN_COMMAND_VERSION_MAX);
    }
  }
}

#define INDEX_HTML          "index.html"
#define OUTPUT_TYPE         "output_type"
#define COMMAND_VERSION     "command_version"
#define REQUEST_ID          "request_id"
#define REQUEST_TIMEOUT     "request_timeout"
#define OUTPUT_PRETTY       "output_pretty"
#define EXPR_MISSING        "expr_missing"
#define OUTPUT_TYPE_LEN     (sizeof(OUTPUT_TYPE) - 1)
#define COMMAND_VERSION_LEN (sizeof(COMMAND_VERSION) - 1)
#define REQUEST_ID_LEN      (sizeof(REQUEST_ID) - 1)
#define REQUEST_TIMEOUT_LEN (sizeof(REQUEST_TIMEOUT) - 1)
#define OUTPUT_PRETTY_LEN   (sizeof(OUTPUT_PRETTY) - 1)

#define HTTP_QUERY_PAIR_DELIMITER   "="
#define HTTP_QUERY_PAIRS_DELIMITERS "&;"

static grn_inline int
command_proc_p(grn_obj *expr)
{
  return (expr->header.type == GRN_PROC &&
          ((grn_proc *)expr)->type == GRN_PROC_COMMAND);
}

grn_obj *
grn_ctx_qe_exec_uri(grn_ctx *ctx, const char *path, uint32_t path_len)
{
  grn_obj buf, *expr, *val;
  grn_obj request_id;
  double request_timeout;
  const char *p = path, *e = path + path_len, *v, *key_end, *filename_end;

  request_timeout = grn_get_default_request_timeout();

  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_INIT(&request_id, 0);
  p = grn_text_urldec(ctx, &buf, p, e, '?');
  if (!GRN_TEXT_LEN(&buf)) { GRN_TEXT_SETS(ctx, &buf, INDEX_HTML); }
  v = GRN_TEXT_VALUE(&buf);
  grn_str_get_mime_type(ctx, v, GRN_BULK_CURR(&buf), &key_end, &filename_end);
  if ((GRN_TEXT_LEN(&buf) >= 2 && v[0] == 'd' && v[1] == '/')) {
    const char *command_name = v + 2;
    int command_name_size = key_end - command_name;
    expr = grn_ctx_get(ctx, command_name, command_name_size);
    if (expr && command_proc_p(expr)) {
      while (p < e) {
        int l;
        GRN_BULK_REWIND(&buf);
        p = grn_text_cgidec(ctx, &buf, p, e, HTTP_QUERY_PAIR_DELIMITER);
        v = GRN_TEXT_VALUE(&buf);
        l = GRN_TEXT_LEN(&buf);
        if (l == OUTPUT_TYPE_LEN && !memcmp(v, OUTPUT_TYPE, OUTPUT_TYPE_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_cgidec(ctx, &buf, p, e, HTTP_QUERY_PAIRS_DELIMITERS);
          v = GRN_TEXT_VALUE(&buf);
          get_content_mime_type(ctx, v, GRN_BULK_CURR(&buf));
        } else if (l == COMMAND_VERSION_LEN &&
                   !memcmp(v, COMMAND_VERSION, COMMAND_VERSION_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_cgidec(ctx, &buf, p, e, HTTP_QUERY_PAIRS_DELIMITERS);
          get_command_version(ctx, GRN_TEXT_VALUE(&buf), GRN_BULK_CURR(&buf));
          if (ctx->rc) { goto exit; }
        } else if (l == REQUEST_ID_LEN &&
                   !memcmp(v, REQUEST_ID, REQUEST_ID_LEN)) {
          GRN_BULK_REWIND(&request_id);
          p = grn_text_cgidec(ctx, &request_id, p, e,
                              HTTP_QUERY_PAIRS_DELIMITERS);
        } else if (l == REQUEST_TIMEOUT_LEN &&
                   !memcmp(v, REQUEST_TIMEOUT, REQUEST_TIMEOUT_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_cgidec(ctx, &buf, p, e, HTTP_QUERY_PAIRS_DELIMITERS);
          GRN_TEXT_PUTC(ctx, &buf, '\0');
          request_timeout = strtod(GRN_TEXT_VALUE(&buf), NULL);
        } else if (l == OUTPUT_PRETTY_LEN &&
                   !memcmp(v, OUTPUT_PRETTY, OUTPUT_PRETTY_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_cgidec(ctx, &buf, p, e, HTTP_QUERY_PAIRS_DELIMITERS);
          if (GRN_TEXT_LEN(&buf) == strlen("yes") &&
              !memcmp(GRN_TEXT_VALUE(&buf), "yes", GRN_TEXT_LEN(&buf))) {
            ctx->impl->output.is_pretty = GRN_TRUE;
          } else {
            ctx->impl->output.is_pretty = GRN_FALSE;
          }
        } else {
          if (!(val = grn_expr_get_or_add_var(ctx, expr, v, l))) {
            val = &buf;
          }
          grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
          p = grn_text_cgidec(ctx, val, p, e, HTTP_QUERY_PAIRS_DELIMITERS);
        }
      }
      if (request_timeout > 0 && GRN_TEXT_LEN(&request_id) == 0) {
        grn_text_printf(ctx, &request_id, "%p", ctx);
      }
      if (GRN_TEXT_LEN(&request_id) > 0) {
        GRN_TEXT_SET(ctx, &ctx->impl->current_request_id,
                     GRN_TEXT_VALUE(&request_id),
                     GRN_TEXT_LEN(&request_id));
        grn_request_canceler_register(ctx,
                                      GRN_TEXT_VALUE(&request_id),
                                      GRN_TEXT_LEN(&request_id));
        if (request_timeout > 0.0) {
          ctx->impl->current_request_timer_id =
            grn_request_timer_register(GRN_TEXT_VALUE(&request_id),
                                       GRN_TEXT_LEN(&request_id),
                                       request_timeout);
        }
      }
      ctx->impl->curr_expr = expr;
      grn_expr_exec(ctx, expr, 0);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid command name: %.*s",
          command_name_size, command_name);
    }
  } else if ((expr = grn_ctx_get(ctx, GRN_EXPR_MISSING_NAME,
                                 strlen(GRN_EXPR_MISSING_NAME)))) {
    if ((val = grn_expr_get_var_by_offset(ctx, expr, 0))) {
      grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
      GRN_TEXT_SET(ctx, val, v, filename_end - v);
    }
    ctx->impl->curr_expr = expr;
    grn_expr_exec(ctx, expr, 0);
  }
exit :
  GRN_OBJ_FIN(ctx, &request_id);
  GRN_OBJ_FIN(ctx, &buf);

  return expr;
}

grn_obj *
grn_ctx_qe_exec(grn_ctx *ctx, const char *str, uint32_t str_len)
{
  char tok_type;
  int offset = 0;
  grn_obj buf, *expr = NULL, *val = NULL;
  grn_obj request_id;
  double request_timeout;
  const char *p = str, *e = str + str_len, *v;

  request_timeout = grn_get_default_request_timeout();

  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_INIT(&request_id, 0);
  p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
  expr = grn_ctx_get(ctx, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
  while (p < e) {
    GRN_BULK_REWIND(&buf);
    p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
    v = GRN_TEXT_VALUE(&buf);
    switch (tok_type) {
    case GRN_TOK_VOID :
      p = e;
      break;
    case GRN_TOK_SYMBOL :
      if (GRN_TEXT_LEN(&buf) > 2 && v[0] == '-' && v[1] == '-') {
        int l = GRN_TEXT_LEN(&buf) - 2;
        v += 2;
        if (l == OUTPUT_TYPE_LEN && !memcmp(v, OUTPUT_TYPE, OUTPUT_TYPE_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
          v = GRN_TEXT_VALUE(&buf);
          get_content_mime_type(ctx, v, GRN_BULK_CURR(&buf));
        } else if (l == COMMAND_VERSION_LEN &&
                   !memcmp(v, COMMAND_VERSION, COMMAND_VERSION_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
          get_command_version(ctx, GRN_TEXT_VALUE(&buf), GRN_BULK_CURR(&buf));
          if (ctx->rc) { goto exit; }
        } else if (l == REQUEST_ID_LEN &&
                   !memcmp(v, REQUEST_ID, REQUEST_ID_LEN)) {
          GRN_BULK_REWIND(&request_id);
          p = grn_text_unesc_tok(ctx, &request_id, p, e, &tok_type);
        } else if (l == REQUEST_TIMEOUT_LEN &&
                   !memcmp(v, REQUEST_TIMEOUT, REQUEST_TIMEOUT_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
          GRN_TEXT_PUTC(ctx, &buf, '\0');
          request_timeout = strtod(GRN_TEXT_VALUE(&buf), NULL);
        } else if (l == OUTPUT_PRETTY_LEN &&
                   !memcmp(v, OUTPUT_PRETTY, OUTPUT_PRETTY_LEN)) {
          GRN_BULK_REWIND(&buf);
          p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
          if (GRN_TEXT_LEN(&buf) == strlen("yes") &&
              !memcmp(GRN_TEXT_VALUE(&buf), "yes", GRN_TEXT_LEN(&buf))) {
            ctx->impl->output.is_pretty = GRN_TRUE;
          } else {
            ctx->impl->output.is_pretty = GRN_FALSE;
          }
        } else if (expr && (val = grn_expr_get_or_add_var(ctx, expr, v, l))) {
          grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
          p = grn_text_unesc_tok(ctx, val, p, e, &tok_type);
        } else {
          p = e;
        }
        break;
      }
      // fallthru
    case GRN_TOK_STRING :
    case GRN_TOK_QUOTE :
      if (expr && (val = grn_expr_get_var_by_offset(ctx, expr, offset++))) {
        grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
        GRN_TEXT_PUT(ctx, val, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
      } else {
        p = e;
      }
      break;
    }
  }
  if (request_timeout > 0 && GRN_TEXT_LEN(&request_id) == 0) {
    grn_text_printf(ctx, &request_id, "%p", ctx);
  }
  if (GRN_TEXT_LEN(&request_id) > 0) {
    GRN_TEXT_SET(ctx, &ctx->impl->current_request_id,
                 GRN_TEXT_VALUE(&request_id),
                 GRN_TEXT_LEN(&request_id));
    grn_request_canceler_register(ctx,
                                  GRN_TEXT_VALUE(&request_id),
                                  GRN_TEXT_LEN(&request_id));
    if (request_timeout > 0.0) {
      ctx->impl->current_request_timer_id =
        grn_request_timer_register(GRN_TEXT_VALUE(&request_id),
                                   GRN_TEXT_LEN(&request_id),
                                   request_timeout);
    }
  }
  ctx->impl->curr_expr = expr;
  if (expr && command_proc_p(expr)) {
    grn_expr_exec(ctx, expr, 0);
  } else {
    GRN_BULK_REWIND(&buf);
    grn_text_unesc_tok(ctx, &buf, str, str + str_len, &tok_type);
    if (GRN_TEXT_LEN(&buf)) {
      ERR(GRN_INVALID_ARGUMENT, "invalid command name: %.*s",
          (int)GRN_TEXT_LEN(&buf), GRN_TEXT_VALUE(&buf));
    }
  }
exit :
  GRN_OBJ_FIN(ctx, &request_id);
  GRN_OBJ_FIN(ctx, &buf);

  return expr;
}

grn_rc
grn_ctx_sendv(grn_ctx *ctx, int argc, char **argv, int flags)
{
  grn_obj buf;
  GRN_API_ENTER;
  GRN_TEXT_INIT(&buf, 0);
  while (argc--) {
    // todo : encode into json like syntax
    GRN_TEXT_PUTS(ctx, &buf, *argv);
    argv++;
    if (argc) { GRN_TEXT_PUTC(ctx, &buf, ' '); }
  }
  grn_ctx_send(ctx, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf), flags);
  GRN_OBJ_FIN(ctx, &buf);
  GRN_API_RETURN(ctx->rc);
}

static int
comment_command_p(const char *command, unsigned int length)
{
  const char *p, *e;

  e = command + length;
  for (p = command; p < e; p++) {
    switch (*p) {
    case '#' :
      return GRN_TRUE;
    case ' ' :
    case '\t' :
      break;
    default :
      return GRN_FALSE;
    }
  }
  return GRN_FALSE;
}

unsigned int
grn_ctx_send(grn_ctx *ctx, const char *str, unsigned int str_len, int flags)
{
  if (!ctx) { return 0; }
  GRN_API_ENTER;
  if (ctx->impl) {
    if ((flags & GRN_CTX_MORE)) { flags |= GRN_CTX_QUIET; }
    if (ctx->stat == GRN_CTX_QUIT) { flags |= GRN_CTX_QUIT; }

    ctx->impl->command.flags = flags;
    if (ctx->impl->com) {
      grn_rc rc;
      grn_com_header sheader;
      grn_timeval_now(ctx, &ctx->impl->tv);
      sheader.proto = GRN_COM_PROTO_GQTP;
      sheader.qtype = 0;
      sheader.keylen = 0;
      sheader.level = 0;
      sheader.flags = flags;
      sheader.status = 0;
      sheader.opaque = 0;
      sheader.cas = 0;
      if ((rc = grn_com_send(ctx, ctx->impl->com, &sheader, (char *)str, str_len, 0))) {
        ERR(rc, "grn_com_send failed");
      }
      goto exit;
    } else {
      grn_command_version command_version;
      grn_obj *expr = NULL;
      bool is_comment = false;
      bool processed = false;

      command_version = grn_ctx_get_command_version(ctx);
      if (ctx->impl->command.keep.command) {
        grn_obj *val;
        expr = ctx->impl->command.keep.command;
        ctx->impl->command.keep.command = NULL;
        grn_ctx_set_command_version(ctx, ctx->impl->command.keep.version);
        if ((val = grn_expr_get_var_by_offset(ctx, expr, 0))) {
          grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
          GRN_TEXT_PUT(ctx, val, str, str_len);
        }
        grn_expr_exec(ctx, expr, 0);
      } else if (comment_command_p(str, str_len)) {
        is_comment = true;
      } else {
        GRN_BULK_REWIND(ctx->impl->output.buf);
        GRN_BULK_REWIND(&(ctx->impl->output.levels));
        ctx->impl->output.type = GRN_CONTENT_JSON;
        ctx->impl->output.mime_type = "application/json";
        ctx->impl->output.is_pretty = GRN_FALSE;
        grn_timeval_now(ctx, &ctx->impl->tv);
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_COMMAND,
                      ">", "%.*s", str_len, str);
        if (str_len && *str == '/') {
          expr = grn_ctx_qe_exec_uri(ctx, str + 1, str_len - 1);
        } else {
          expr = grn_ctx_qe_exec(ctx, str, str_len);
        }
      }
      if (ctx->stat == GRN_CTX_QUITTING) { ctx->stat = GRN_CTX_QUIT; }
      if (!is_comment) {
        if (ctx->impl->command.keep.command) {
          ERRCLR(ctx);
        } else {
          if (ctx->impl->current_request_timer_id) {
            void *timer_id = ctx->impl->current_request_timer_id;
            ctx->impl->current_request_timer_id = NULL;
            grn_request_timer_unregister(timer_id);
          }
          if (GRN_TEXT_LEN(&ctx->impl->current_request_id) > 0) {
            grn_obj *request_id = &ctx->impl->current_request_id;
            grn_request_canceler_unregister(ctx,
                                            GRN_TEXT_VALUE(request_id),
                                            GRN_TEXT_LEN(request_id));
            GRN_BULK_REWIND(&ctx->impl->current_request_id);
          }
          processed = true;
        }
      }
      if (!(ctx->impl->command.flags & GRN_CTX_QUIET) &&
          ctx->impl->output.func) {
        ctx->impl->output.func(ctx, GRN_CTX_TAIL, ctx->impl->output.data.ptr);
      }
      if (processed) {
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_RESULT_CODE,
                      "<", "rc=%d", ctx->rc);
      }
      if (expr) { grn_expr_clear_vars(ctx, expr); }
      grn_ctx_set_command_version(ctx, command_version);
      if (processed) {
        grn_db_command_processed(ctx, ctx->impl->db);
      }
      goto exit;
    }
  }
  ERR(GRN_INVALID_ARGUMENT, "invalid ctx assigned");
exit :
  GRN_API_RETURN(0);
}

unsigned int
grn_ctx_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }

  *flags = 0;

  if (ctx->stat == GRN_CTX_QUIT) {
    grn_bool have_buffer = GRN_FALSE;

    if (ctx->impl &&
        !ctx->impl->com &&
        GRN_TEXT_LEN(ctx->impl->output.buf) > 0) {
      have_buffer = GRN_TRUE;
    }

    *flags |= GRN_CTX_QUIT;
    if (!have_buffer) {
      *str = NULL;
      *str_len = 0;
      return 0;
    }
  }

  GRN_API_ENTER;
  if (ctx->impl) {
    if (ctx->impl->com) {
      grn_com_header header;
      if (grn_com_recv(ctx, ctx->impl->com, &header, ctx->impl->output.buf)) {
        *str = NULL;
        *str_len = 0;
        *flags = 0;
      } else {
        *str = GRN_BULK_HEAD(ctx->impl->output.buf);
        *str_len = GRN_BULK_VSIZE(ctx->impl->output.buf);
        if (header.flags & GRN_CTX_QUIT) {
          ctx->stat = GRN_CTX_QUIT;
          *flags |= GRN_CTX_QUIT;
        } else {
          if (!(header.flags & GRN_CTX_TAIL)) {
            *flags |= GRN_CTX_MORE;
          }
        }
        ctx->impl->output.type = header.qtype;
        ctx->rc = (int16_t)ntohs(header.status);
        ctx->errbuf[0] = '\0';
        ctx->errline = 0;
        ctx->errfile = NULL;
        ctx->errfunc = NULL;
      }
      goto exit;
    } else {
      grn_obj *buf = ctx->impl->output.buf;
      unsigned int head = 0, tail = GRN_BULK_VSIZE(buf);
      *str = GRN_BULK_HEAD(buf) + head;
      *str_len = tail - head;
      GRN_BULK_REWIND(ctx->impl->output.buf);
      goto exit;
    }
  }
  ERR(GRN_INVALID_ARGUMENT, "invalid ctx assigned");
exit :
  GRN_API_RETURN(0);
}

void
grn_ctx_stream_out_func(grn_ctx *ctx, int flags, void *stream)
{
  if (ctx && ctx->impl) {
    grn_obj *buf = ctx->impl->output.buf;
    uint32_t size = GRN_BULK_VSIZE(buf);
    if (size) {
      if (fwrite(GRN_BULK_HEAD(buf), 1, size, (FILE *)stream)) {
        fputc('\n', (FILE *)stream);
        fflush((FILE *)stream);
      }
      GRN_BULK_REWIND(buf);
    }
  }
}

void
grn_ctx_recv_handler_set(grn_ctx *ctx,
                         grn_recv_handler_func func,
                         void *func_arg)
{
  if (ctx && ctx->impl) {
    ctx->impl->output.func = func;
    ctx->impl->output.data.ptr = func_arg;
  }
}

grn_rc
grn_ctx_info_get(grn_ctx *ctx, grn_ctx_info *info)
{
  if (!ctx || !ctx->impl) { return GRN_INVALID_ARGUMENT; }
  if (ctx->impl->com) {
    info->fd = ctx->impl->com->fd;
    info->com_status = ctx->impl->com_status;
    info->outbuf = ctx->impl->output.buf;
    info->stat = ctx->stat;
  } else {
    info->fd = -1;
    info->com_status = 0;
    info->outbuf = ctx->impl->output.buf;
    info->stat = ctx->stat;
  }
  return GRN_SUCCESS;
}

#define DB_P(s) ((s) && (s)->header.type == GRN_DB)

grn_rc
grn_ctx_use(grn_ctx *ctx, grn_obj *db)
{
  GRN_API_ENTER;
  if (db && !DB_P(db)) {
    ctx->rc = GRN_INVALID_ARGUMENT;
  } else {
    if (!ctx->rc) {
      ctx->impl->db = db;
      if (db) {
        grn_obj buf;
        GRN_TEXT_INIT(&buf, 0);
        grn_obj_get_info(ctx, db, GRN_INFO_ENCODING, &buf);
        ctx->encoding = *(grn_encoding *)GRN_BULK_HEAD(&buf);
        grn_obj_close(ctx, &buf);
      }
    }
  }
  GRN_API_RETURN(ctx->rc);
}

/* don't handle error inside logger functions */

void
grn_ctx_log(grn_ctx *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  grn_ctx_logv(ctx, fmt, ap);
  va_end(ap);
}

void
grn_ctx_logv(grn_ctx *ctx, const char *fmt, va_list ap)
{
  char buffer[GRN_CTX_MSGSIZE];
  grn_vsnprintf(buffer, GRN_CTX_MSGSIZE, fmt, ap);
  grn_strcpy(ctx->errbuf, GRN_CTX_MSGSIZE, buffer);
}

#ifdef WIN32
static void
grn_ctx_log_back_trace_windows(grn_ctx *ctx, grn_log_level level)
{
  ULONG frames_to_skip = 0;
  ULONG frames_to_capture = MAXUSHORT;
  void *back_trace[MAXUSHORT];
  USHORT n_frames = CaptureStackBackTrace(frames_to_skip,
                                          frames_to_capture,
                                          back_trace,
                                          NULL);
  if (n_frames == 0) {
    return;
  }

  HANDLE process = GetCurrentProcess();
  if (!grn_windows_symbol_initialize(ctx, process)) {
    GRN_LOG(ctx,
            level,
            "[log][back-trace] failed to initialize symbol handler");
    return;
  }

  USHORT i;
  for (i = 0; i < n_frames; i++) {
    void *address = back_trace[i];
    grn_windows_log_trace(ctx, level, process, (DWORD64)address);
  }

  grn_windows_symbol_cleanup(ctx, process);
}
#else
# ifdef HAVE_BACKTRACE
static void
grn_ctx_log_back_trace_backtrace(grn_ctx *ctx, grn_log_level level)
{
#  define MAX_N_TRACES 16
  void *traces[MAX_N_TRACES];
  int n_traces = backtrace(traces, MAX_N_TRACES);
#  undef MAX_N_TRACES
  char **symbols;
  symbols = backtrace_symbols(traces, n_traces);
  if (symbols) {
    int i;
    for (i = 0; i < n_traces; i++) {
      GRN_LOG(ctx, level, "%s", symbols[i]);
    }
    free(symbols);
  } else {
    GRN_LOG(ctx, level, "[log][back-trace] backtrace_symbols() is failed");
  }
}
# endif
#endif

void
grn_ctx_log_back_trace(grn_ctx *ctx, grn_log_level level)
{
#ifdef WIN32
  grn_ctx_log_back_trace_windows(ctx, level);
#else
# ifdef HAVE_BACKTRACE
  grn_ctx_log_back_trace_backtrace(ctx, level);
# endif
#endif
}

void
grn_assert(grn_ctx *ctx, int cond, const char* file, int line, const char* func)
{
  if (!cond) {
    GRN_LOG(ctx, GRN_LOG_WARNING, "ASSERT fail on %s %s:%d", func, file, line);
  }
}

const char *
grn_get_version(void)
{
  return GRN_VERSION;
}

uint32_t
grn_get_version_major(void)
{
  return GRN_VERSION_MAJOR;
}

uint32_t
grn_get_version_minor(void)
{
  return GRN_VERSION_MINOR;
}

uint32_t
grn_get_version_micro(void)
{
  return GRN_VERSION_MICRO;
}

const char *
grn_get_package(void)
{
  return PACKAGE;
}

const char *
grn_get_package_label(void)
{
  return PACKAGE_LABEL;
}

#ifdef WIN32
static LONG WINAPI
grn_exception_filter(EXCEPTION_POINTERS *info)
{
  grn_ctx *ctx = &grn_gctx;
  const char *log_start_mark = "-- CRASHED!!! --";
  const char *log_end_mark =   "----------------";
  HANDLE process;
  HANDLE thread;
  CONTEXT *context;
  STACKFRAME64 frame;
  DWORD machine_type;

  GRN_LOG(ctx, GRN_LOG_CRIT, "%s", log_start_mark);

  process = GetCurrentProcess();
  thread = GetCurrentThread();
  context = info->ContextRecord;

  if (grn_exception_filter_running_get()) {
    GRN_LOG(ctx, GRN_LOG_CRIT, "exception is occurred in exception filter");
    GRN_LOG(ctx, GRN_LOG_CRIT, "%s", log_end_mark);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  grn_exception_filter_running_set(true);

  if (!grn_windows_symbol_initialize(ctx, process)) {
    GRN_LOG(ctx, GRN_LOG_CRIT, "failed to initialize symbol handler");
    GRN_LOG(ctx, GRN_LOG_CRIT, "%s", log_end_mark);
    grn_exception_filter_running_set(false);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  memset(&frame, 0, sizeof(STACKFRAME64));
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrReturn.Mode = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrBStore.Mode = AddrModeFlat;
# ifdef _M_IX86
  machine_type = IMAGE_FILE_MACHINE_I386;
  frame.AddrPC.Offset = context->Eip;
  frame.AddrFrame.Offset = context->Ebp;
  frame.AddrStack.Offset = context->Esp;
# elif defined(_M_IA64) /* _M_IX86 */
  machine_type = IMAGE_FILE_MACHINE_IA64;
  frame.AddrPC.Offset = context->StIIP;
  frame.AddrStack.Offset = context->IntSP; /* SP is IntSP? */
  frame.AddrBStore.Offset = context->RsBSP;
# elif defined(_M_AMD64) /* _M_IX86 */
  machine_type = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = context->Rip;
  frame.AddrFrame.Offset = context->Rbp;
  frame.AddrStack.Offset = context->Rsp;
# else /* _M_IX86 */
#  error "Intel x86, Intel Itanium and x64 are only supported architectures"
# endif /* _M_IX86 */

  DWORD64 previous_address = 0;
  while (GRN_TRUE) {
    if (!StackWalk64(machine_type,
                     process,
                     thread,
                     &frame,
                     context,
                     NULL,
                     NULL,
                     NULL,
                     NULL)) {
      break;
    }

    DWORD64 address = frame.AddrPC.Offset;
    if (previous_address != 0 && address == previous_address) {
      break;
    }

    grn_windows_log_trace(ctx, GRN_LOG_CRIT, process, address);
    previous_address = address;
  }
  GRN_LOG(ctx, GRN_LOG_CRIT, "%s", log_end_mark);

  grn_windows_symbol_cleanup(ctx, process);

  grn_exception_filter_running_set(false);

  return EXCEPTION_CONTINUE_SEARCH;
}
#elif defined(HAVE_SIGNAL_H) /* WIN32 */
static int segv_received = 0;
static void
grn_segv_handler(int signal_number, siginfo_t *info, void *context)
{
  grn_ctx *ctx = &grn_gctx;

  if (segv_received) {
    GRN_LOG(ctx, GRN_LOG_CRIT, "SEGV received in SEGV handler.");
    exit(EXIT_FAILURE);
  }
  segv_received = 1;

  GRN_LOG(ctx, GRN_LOG_CRIT, "-- CRASHED!!! --");
# ifdef HAVE_BACKTRACE
#  define N_TRACE_LEVEL 1024
  {
    static void *trace[N_TRACE_LEVEL];
    int n = backtrace(trace, N_TRACE_LEVEL);
    char **symbols = backtrace_symbols(trace, n);
    int i;

    if (symbols) {
      for (i = 0; i < n; i++) {
        GRN_LOG(ctx, GRN_LOG_CRIT, "%s", symbols[i]);
      }
      free(symbols);
    }
  }
# else /* HAVE_BACKTRACE */
  GRN_LOG(ctx, GRN_LOG_CRIT, "backtrace() isn't available.");
# endif /* HAVE_BACKTRACE */
  GRN_LOG(ctx, GRN_LOG_CRIT, "----------------");
  abort();
}
#endif /* WIN32 */

grn_rc
grn_set_segv_handler(void)
{
  grn_rc rc = GRN_SUCCESS;
#ifdef WIN32
  SetUnhandledExceptionFilter(grn_exception_filter);
#elif defined(HAVE_SIGNAL_H) /* WIN32 */
  grn_ctx *ctx = &grn_gctx;
  struct sigaction action;

  sigemptyset(&action.sa_mask);
  action.sa_sigaction = grn_segv_handler;
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;

  if (sigaction(SIGSEGV, &action, NULL)) {
    SERR("failed to set SIGSEGV action");
    rc = ctx->rc;
  };
#endif /* WIN32 */
  return rc;
}

#if defined(HAVE_SIGNAL_H) && !defined(WIN32)
static struct sigaction old_int_handler;
static void
int_handler(int signal_number, siginfo_t *info, void *context)
{
  grn_gctx.stat = GRN_CTX_QUIT;
  sigaction(signal_number, &old_int_handler, NULL);
}

static struct sigaction old_term_handler;
static void
term_handler(int signal_number, siginfo_t *info, void *context)
{
  grn_gctx.stat = GRN_CTX_QUIT;
  sigaction(signal_number, &old_term_handler, NULL);
}
#endif /* defined(HAVE_SIGNAL_H) && !defined(WIN32) */

grn_rc
grn_set_int_handler(void)
{
  grn_rc rc = GRN_SUCCESS;
#if defined(HAVE_SIGNAL_H) && !defined(WIN32)
  grn_ctx *ctx = &grn_gctx;
  struct sigaction action;

  sigemptyset(&action.sa_mask);
  action.sa_sigaction = int_handler;
  action.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &action, &old_int_handler)) {
    SERR("failed to set SIGINT action");
    rc = ctx->rc;
  }
#endif
  return rc;
}

grn_rc
grn_set_term_handler(void)
{
  grn_rc rc = GRN_SUCCESS;
#if defined(HAVE_SIGNAL_H) && !defined(WIN32)
  grn_ctx *ctx = &grn_gctx;
  struct sigaction action;

  sigemptyset(&action.sa_mask);
  action.sa_sigaction = term_handler;
  action.sa_flags = SA_SIGINFO;

  if (sigaction(SIGTERM, &action, &old_term_handler)) {
    SERR("failed to set SIGTERM action");
    rc = ctx->rc;
  }
#endif
  return rc;
}

void
grn_ctx_output_flush(grn_ctx *ctx, int flags)
{
  if (flags & GRN_CTX_QUIET) {
    return;
  }
  if (!ctx->impl->output.func) {
    return;
  }
  if (ctx->impl->output.arrow_stream_writer) {
    grn_arrow_stream_writer_flush(ctx, ctx->impl->output.arrow_stream_writer);
  }
  ctx->impl->output.func(ctx, 0, ctx->impl->output.data.ptr);
}

void
grn_ctx_output_array_open(grn_ctx *ctx, const char *name, int nelements)
{
  grn_output_array_open(ctx,
                        ctx->impl->output.buf,
                        ctx->impl->output.type,
                        name, nelements);
}

void
grn_ctx_output_array_close(grn_ctx *ctx)
{
  grn_output_array_close(ctx,
                         ctx->impl->output.buf,
                         ctx->impl->output.type);
}

void
grn_ctx_output_map_open(grn_ctx *ctx, const char *name, int nelements)
{
  grn_output_map_open(ctx,
                      ctx->impl->output.buf,
                      ctx->impl->output.type,
                      name, nelements);
}

void
grn_ctx_output_map_close(grn_ctx *ctx)
{
  grn_output_map_close(ctx,
                       ctx->impl->output.buf,
                       ctx->impl->output.type);
}

void
grn_ctx_output_null(grn_ctx *ctx)
{
  grn_output_null(ctx,
                  ctx->impl->output.buf,
                  ctx->impl->output.type);
}

void
grn_ctx_output_int32(grn_ctx *ctx, int32_t value)
{
  grn_output_int32(ctx,
                   ctx->impl->output.buf,
                   ctx->impl->output.type,
                   value);
}

void
grn_ctx_output_uint32(grn_ctx *ctx, uint32_t value)
{
  grn_output_uint32(ctx,
                    ctx->impl->output.buf,
                    ctx->impl->output.type,
                    value);
}

void
grn_ctx_output_int64(grn_ctx *ctx, int64_t value)
{
  grn_output_int64(ctx,
                   ctx->impl->output.buf,
                   ctx->impl->output.type,
                   value);
}

void
grn_ctx_output_uint64(grn_ctx *ctx, uint64_t value)
{
  grn_output_uint64(ctx,
                    ctx->impl->output.buf,
                    ctx->impl->output.type,
                    value);
}

void
grn_ctx_output_float32(grn_ctx *ctx, float value)
{
  grn_output_float32(ctx,
                     ctx->impl->output.buf,
                     ctx->impl->output.type,
                     value);
}

void
grn_ctx_output_float(grn_ctx *ctx, double value)
{
  grn_output_float(ctx,
                   ctx->impl->output.buf,
                   ctx->impl->output.type,
                   value);
}

void
grn_ctx_output_cstr(grn_ctx *ctx, const char *value)
{
  grn_output_cstr(ctx,
                  ctx->impl->output.buf,
                  ctx->impl->output.type,
                  value);
}

void
grn_ctx_output_str(grn_ctx *ctx, const char *value, unsigned int value_len)
{
  grn_output_str(ctx,
                 ctx->impl->output.buf,
                 ctx->impl->output.type,
                 value, value_len);
}

void
grn_ctx_output_bool(grn_ctx *ctx, grn_bool value)
{
  grn_output_bool(ctx,
                  ctx->impl->output.buf,
                  ctx->impl->output.type,
                  value);
}

void
grn_ctx_output_obj(grn_ctx *ctx, grn_obj *value, grn_obj_format *format)
{
  grn_output_obj(ctx,
                 ctx->impl->output.buf,
                 ctx->impl->output.type,
                 value, format);
}

void
grn_ctx_output_result_set_open(grn_ctx *ctx,
                               grn_obj *result_set,
                               grn_obj_format *format,
                               uint32_t n_additional_elements)
{
  grn_output_result_set_open(ctx,
                             ctx->impl->output.buf,
                             ctx->impl->output.type,
                             result_set,
                             format,
                             n_additional_elements);
}

void
grn_ctx_output_result_set_close(grn_ctx *ctx,
                                grn_obj *result_set,
                                grn_obj_format *format)
{
  grn_output_result_set_close(ctx,
                              ctx->impl->output.buf,
                              ctx->impl->output.type,
                              result_set,
                              format);
}

void
grn_ctx_output_result_set(grn_ctx *ctx,
                          grn_obj *result_set,
                          grn_obj_format *format)
{
  grn_output_result_set(ctx,
                        ctx->impl->output.buf,
                        ctx->impl->output.type,
                        result_set,
                        format);
}

void
grn_ctx_output_table_columns(grn_ctx *ctx, grn_obj *table,
                             grn_obj_format *format)
{
  grn_output_table_columns(ctx,
                           ctx->impl->output.buf,
                           ctx->impl->output.type,
                           table,
                           format);
}

void
grn_ctx_output_table_records_open(grn_ctx *ctx, int n_records)
{
  grn_output_table_records_open(ctx,
                                ctx->impl->output.buf,
                                ctx->impl->output.type,
                                n_records);
}

void
grn_ctx_output_table_records_close(grn_ctx *ctx)
{
  grn_output_table_records_close(ctx,
                                 ctx->impl->output.buf,
                                 ctx->impl->output.type);
}

void
grn_ctx_output_table_records_content(grn_ctx *ctx,
                                     grn_obj *table,
                                     grn_obj_format *format)
{
  grn_output_table_records_content(ctx,
                                   ctx->impl->output.buf,
                                   ctx->impl->output.type,
                                   table,
                                   format);
}

void
grn_ctx_output_table_records(grn_ctx *ctx,
                             grn_obj *table,
                             grn_obj_format *format)
{
  grn_output_table_records(ctx,
                           ctx->impl->output.buf,
                           ctx->impl->output.type,
                           table,
                           format);
}
