/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "groonga_in.h"
#include <string.h>
#include "token.h"
#include "ql.h"
#include "pat.h"
#include "snip.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#define GRN_CTX_INITIALIZER(enc) \
  { GRN_SUCCESS, 0, enc, 0, GRN_LOG_NOTICE,\
      GRN_CTX_FIN, 0, 0, 0, 0, {0}, NULL, NULL, NULL, NULL, NULL }

#define GRN_CTX_CLOSED(ctx) ((ctx)->stat == GRN_CTX_FIN)

#ifdef USE_EXACT_ALLOC_COUNT
#define GRN_ADD_ALLOC_COUNT(count) \
{ \
  uint32_t alloced; \
  GRN_ATOMIC_ADD_EX(&alloc_count, count, alloced); \
}
#else /* USE_EXACT_ALLOC_COUNT */
#define GRN_ADD_ALLOC_COUNT(count) \
{ \
  alloc_count += count; \
}
#endif

grn_ctx grn_gctx = GRN_CTX_INITIALIZER(GRN_ENC_DEFAULT);
int grn_pagesize;
grn_mutex grn_glock;
uint32_t grn_gtick;

#ifdef USE_UYIELD
int grn_uyield_count = 0;
#endif

/* fixme by 2038 */

grn_rc
grn_timeval_now(grn_ctx *ctx, grn_timeval *tv)
{
#ifdef WIN32
  time_t t;
  struct _timeb tb;
  time(&t);
  _ftime(&tb);
  tv->tv_sec = (int32_t) t;
  tv->tv_usec = tb.millitm * 1000;
  return GRN_SUCCESS;
#else /* WIN32 */
  struct timeval t;
  if (gettimeofday(&t, NULL)) {
    SERR("gettimeofday");
  } else {
    tv->tv_sec = (int32_t) t.tv_sec;
    tv->tv_usec = t.tv_usec;
  }
  return ctx->rc;
#endif /* WIN32 */
}

grn_rc
grn_timeval2str(grn_ctx *ctx, grn_timeval *tv, char *buf)
{
  struct tm *ltm;
#ifdef WIN32
  time_t tvsec = (time_t) tv->tv_sec;
  ltm = localtime(&tvsec);
#else /* WIN32 */
  struct tm tm;
  time_t t = tv->tv_sec;
  ltm = localtime_r(&t, &tm);
#endif /* WIN32 */
  if (!ltm) { SERR("localtime"); }
  snprintf(buf, GRN_TIMEVAL_STR_SIZE - 1, GRN_TIMEVAL_STR_FORMAT,
           ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday,
           ltm->tm_hour, ltm->tm_min, ltm->tm_sec, (int) tv->tv_usec);
  buf[GRN_TIMEVAL_STR_SIZE - 1] = '\0';
  return ctx->rc;
}

grn_rc
grn_str2timeval(const char *str, uint32_t str_len, grn_timeval *tv)
{
  struct tm tm;
  const char *r1, *r2, *rend = str + str_len;
  uint32_t uv;
  memset(&tm, 0, sizeof(struct tm));

  tm.tm_year = (int)grn_atoui(str, rend, &r1) - 1900;
  if ((r1 + 1) >= rend || (*r1 != '/' && *r1 != '-') ||
      tm.tm_year < 0) { return GRN_INVALID_ARGUMENT; }
  r1++;
  tm.tm_mon = (int)grn_atoui(r1, rend, &r1) - 1;
  if ((r1 + 1) >= rend || (*r1 != '/' && *r1 != '-') ||
      tm.tm_mon < 0 || tm.tm_mon >= 12) { return GRN_INVALID_ARGUMENT; }
  r1++;
  tm.tm_mday = (int)grn_atoui(r1, rend, &r1);
  if ((r1 + 1) >= rend || *r1 != ' ' ||
      tm.tm_mday < 1 || tm.tm_mday > 31) { return GRN_INVALID_ARGUMENT; }

  tm.tm_hour = (int)grn_atoui(++r1, rend, &r2);
  if ((r2 + 1) >= rend || r1 == r2 || *r2 != ':' ||
      tm.tm_hour < 0 || tm.tm_hour >= 24) {
    return GRN_INVALID_ARGUMENT;
  }
  r1 = r2 + 1;
  tm.tm_min = (int)grn_atoui(r1, rend, &r2);
  if ((r2 + 1) >= rend || r1 == r2 || *r2 != ':' ||
      tm.tm_min < 0 || tm.tm_min >= 60) {
    return GRN_INVALID_ARGUMENT;
  }
  r1 = r2 + 1;
  tm.tm_sec = (int)grn_atoui(r1, rend, &r2);
  if (r1 == r2 ||
      tm.tm_sec < 0 || tm.tm_sec > 61 /* leap 2sec */) {
    return GRN_INVALID_ARGUMENT;
  }
  r1 = r2;

  if ((tv->tv_sec = (int32_t) mktime(&tm)) == -1) { return GRN_INVALID_ARGUMENT; }
  if ((r1 + 1) < rend && *r1 == '.') { r1++; }
  uv = grn_atoi(r1, rend, &r2);
  while (r2 < r1 + 6) {
    uv *= 10;
    r2++;
  }
  if (uv >= 1000000) { return GRN_INVALID_ARGUMENT; }
  tv->tv_usec = uv;
  return GRN_SUCCESS;
}

#ifdef USE_FAIL_MALLOC
int grn_fmalloc_prob = 0;
char *grn_fmalloc_func = NULL;
char *grn_fmalloc_file = NULL;
int grn_fmalloc_line = 0;
#endif /* USE_FAIL_MALLOC */

#define N_SEGMENTS 512

#define SEGMENT_SIZE    (1<<22)
#define SEGMENT_MASK    (SEGMENT_SIZE - 1)

#define SEGMENT_WORD    (1<<31)
#define SEGMENT_VLEN    (1<<30)
#define SEGMENT_LIFO    (1<<29)

#ifdef USE_DYNAMIC_MALLOC_CHANGE
static void
grn_ctx_impl_init_malloc(grn_ctx *ctx)
{
#  ifdef USE_FAIL_MALLOC
  ctx->impl->malloc_func = grn_malloc_fail;
  ctx->impl->calloc_func = grn_calloc_fail;
  ctx->impl->realloc_func = grn_realloc_fail;
  ctx->impl->strdup_func = grn_strdup_fail;
#  else
  ctx->impl->malloc_func = grn_malloc_default;
  ctx->impl->calloc_func = grn_calloc_default;
  ctx->impl->realloc_func = grn_realloc_default;
  ctx->impl->strdup_func = grn_strdup_default;
#  endif
}
#endif

static void
grn_loader_init(grn_loader *loader)
{
  GRN_TEXT_INIT(&loader->values, 0);
  GRN_UINT32_INIT(&loader->level, GRN_OBJ_VECTOR);
  GRN_PTR_INIT(&loader->columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  loader->table = NULL;
  loader->last = NULL;
  loader->ifexists = NULL;
  loader->values_size = 0;
  loader->nrecords = 0;
  loader->stat = GRN_LOADER_BEGIN;
}

void
grn_ctx_loader_clear(grn_ctx *ctx)
{
  grn_loader *loader = &ctx->impl->loader;
  grn_obj *v = (grn_obj *)(GRN_BULK_HEAD(&loader->values));
  grn_obj *ve = (grn_obj *)(GRN_BULK_CURR(&loader->values));
  grn_obj **p = (grn_obj **)GRN_BULK_HEAD(&loader->columns);
  uint32_t i = GRN_BULK_VSIZE(&loader->columns) / sizeof(grn_obj *);
  if (ctx->impl->db) { while (i--) { grn_obj_unlink(ctx, *p++); } }
  if (loader->ifexists) { grn_obj_unlink(ctx, loader->ifexists); }
  while (v < ve) { GRN_OBJ_FIN(ctx, v++); }
  GRN_OBJ_FIN(ctx, &loader->values);
  GRN_OBJ_FIN(ctx, &loader->level);
  GRN_OBJ_FIN(ctx, &loader->columns);
  grn_loader_init(loader);
}

static void
grn_ctx_impl_init(grn_ctx *ctx)
{
  if (!(ctx->impl = GRN_MALLOC(sizeof(struct _grn_ctx_impl)))) { return; }
  if (!(ctx->impl->segs = grn_io_anon_map(ctx, &ctx->impl->mi,
                                          sizeof(grn_io_mapinfo) * N_SEGMENTS))) {
    GRN_FREE(ctx->impl);
    ctx->impl = NULL;
    return;
  }
#ifdef USE_DYNAMIC_MALLOC_CHANGE
  grn_ctx_impl_init_malloc(ctx);
#endif
  if (!(ctx->impl->values = grn_array_create(ctx, NULL, sizeof(grn_tmp_db_obj),
                                             GRN_ARRAY_TINY))) {
    grn_io_anon_unmap(ctx, &ctx->impl->mi, sizeof(grn_io_mapinfo) * N_SEGMENTS);
    GRN_FREE(ctx->impl);
    ctx->impl = NULL;
    return;
  }
  ctx->impl->encoding = ctx->encoding;
  ctx->impl->lifoseg = -1;
  ctx->impl->currseg = -1;
  ctx->impl->db = NULL;

  ctx->impl->expr_vars = grn_hash_create(ctx, NULL, sizeof(grn_id), sizeof(grn_expr_vars), 0);
  ctx->impl->stack_curr = 0;
  ctx->impl->qe_next = NULL;
  ctx->impl->parser = NULL;

  ctx->impl->phs = NIL;
  ctx->impl->code = NIL;
  ctx->impl->dump = NIL;
  ctx->impl->op = GRN_OP_T0LVL;
  ctx->impl->args = NIL;
  ctx->impl->envir = NIL;
  ctx->impl->value = NIL;
  ctx->impl->ncells = 0;
  ctx->impl->n_entries = 0;
  ctx->impl->seqno = 0;
  ctx->impl->lseqno = 0;
  ctx->impl->nbinds = 0;
  ctx->impl->nunbinds = 0;
  ctx->impl->feed_mode = grn_ql_atonce;
  ctx->impl->cur = NULL;
  ctx->impl->str_end = NULL;
  ctx->impl->batchmode = 0;
  ctx->impl->gc_verbose = 0;
  ctx->impl->inbuf = NULL;
  ctx->impl->co.mode = 0;
  ctx->impl->co.func = NULL;
  ctx->impl->objects = NULL;
  ctx->impl->symbols = NULL;
  ctx->impl->com = NULL;
  ctx->impl->outbuf = grn_obj_open(ctx, GRN_BULK, 0, 0);
  GRN_TEXT_INIT(&ctx->impl->subbuf, 0);
  grn_loader_init(&ctx->impl->loader);
}

void
grn_ctx_set_next_expr(grn_ctx *ctx, grn_obj *expr)
{
  ctx->impl->qe_next = expr;
}

void
grn_ctx_impl_err(grn_ctx *ctx)
{
  if (ctx->impl) {
    ctx->impl->cur = ctx->impl->str_end;
    ctx->impl->op = GRN_OP_ERR0;
  }
}

static void
grn_ctx_ql_init(grn_ctx *ctx, int flags)
{
  if (!ctx->impl) {
    grn_ctx_impl_init(ctx);
    if (ERRP(ctx, GRN_ERROR)) { return; }
  }
  if (flags & GRN_CTX_BATCH_MODE) { ctx->impl->batchmode = 1; }
  if ((ctx->impl->objects = grn_array_create(ctx, NULL, sizeof(grn_cell),
                                             GRN_ARRAY_TINY))) {
    if ((ctx->impl->symbols = grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE,
                                              sizeof(grn_cell),
                                              GRN_OBJ_KEY_VAR_SIZE|GRN_HASH_TINY))) {
      if (!ERRP(ctx, GRN_ERROR)) {
        grn_ql_init_globals(ctx);
        if (!ERRP(ctx, GRN_ERROR)) {
          return;
        }
      }
      grn_hash_close(ctx, ctx->impl->symbols);
      ctx->impl->symbols = NULL;
    } else {
      MERR("ctx->impl->symbols init failed");
    }
    grn_array_close(ctx, ctx->impl->objects);
    ctx->impl->objects = NULL;
  } else {
    MERR("ctx->impl->objects init failed");
  }
}

grn_rc
grn_ctx_init(grn_ctx *ctx, int flags)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  // if (ctx->stat != GRN_CTX_FIN) { return GRN_INVALID_ARGUMENT; }
  ERRCLR(ctx);
  ctx->flags = flags;
  ctx->stat = GRN_QL_WAIT_EXPR;
  ctx->encoding = grn_gctx.encoding;
  ctx->seqno = 0;
  ctx->seqno2 = 0;
  ctx->subno = 0;
  ctx->impl = NULL;
  if (flags & GRN_CTX_USE_QL) {
    grn_ctx_ql_init(ctx, flags);
    if (ERRP(ctx, GRN_ERROR)) { return ctx->rc; }
  }
  ctx->user_data.ptr = NULL;
  MUTEX_LOCK(grn_glock);
  ctx->next = grn_gctx.next;
  ctx->prev = &grn_gctx;
  grn_gctx.next->prev = ctx;
  grn_gctx.next = ctx;
  MUTEX_UNLOCK(grn_glock);
  return ctx->rc;
}

#define GRN_CTX_ALLOCATED             (0x80)

grn_ctx *
grn_ctx_open(int flags)
{
  grn_ctx *ctx = GRN_GMALLOCN(grn_ctx, 1);
  if (ctx) {
    grn_ctx_init(ctx, flags|GRN_CTX_ALLOCATED);
    if (ERRP(ctx, GRN_ERROR)) {
      grn_ctx_fin(ctx);
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
    MUTEX_LOCK(grn_glock);
    ctx->next->prev = ctx->prev;
    ctx->prev->next = ctx->next;
    MUTEX_UNLOCK(grn_glock);
  }
  if (ctx->impl) {
    grn_ctx_loader_clear(ctx);
    if (ctx->impl->objects) {
      grn_cell *o;
      GRN_ARRAY_EACH(ctx, ctx->impl->objects, 0, 0, id, &o, {
        grn_cell_clear(ctx, o);
      });
      grn_array_close(ctx, ctx->impl->objects);
    }
    if (ctx->impl->parser) {
      grn_expr_parser_close(ctx);
    }
    if (ctx->impl->values) {
      grn_tmp_db_obj *o;
      GRN_ARRAY_EACH(ctx, ctx->impl->values, 0, 0, id, &o, {
        grn_obj_close(ctx, (grn_obj *)o->obj);
      });
      grn_array_close(ctx, ctx->impl->values);
    }
    if (ctx->impl->symbols) {
      grn_hash_close(ctx, ctx->impl->symbols);
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
    rc = grn_obj_close(ctx, ctx->impl->outbuf);
    rc = grn_bulk_fin(ctx, &ctx->impl->subbuf);
    {
      int i;
      grn_io_mapinfo *mi;
      for (i = 0, mi = ctx->impl->segs; i < N_SEGMENTS; i++, mi++) {
        if (mi->map) {
          if (mi->count & SEGMENT_VLEN) {
            grn_io_anon_unmap(ctx, mi, mi->nref * grn_pagesize);
          } else {
            grn_io_anon_unmap(ctx, mi, SEGMENT_SIZE);
          }
        }
      }
    }
    grn_io_anon_unmap(ctx, &ctx->impl->mi, sizeof(grn_io_mapinfo) * N_SEGMENTS);
    {
      uint32_t i;
      grn_expr_var *v;
      grn_expr_vars *vp;
      GRN_HASH_EACH(ctx, ctx->impl->expr_vars, id, NULL, NULL, &vp, {
        for (v = vp->vars, i = vp->nvars; i; v++, i--) {
          GRN_OBJ_FIN(ctx, &v->value);
        }
        GRN_FREE(vp->vars);
      });
    }
    grn_hash_close(ctx, ctx->impl->expr_vars);
    GRN_FREE(ctx->impl);
  }
  ctx->stat = GRN_CTX_FIN;
  return rc;
}

grn_timeval grn_starttime;

static FILE *default_logger_fp = NULL;

static void
default_logger_func(int level, const char *time, const char *title,
                    const char *msg, const char *location, void *func_arg)
{
  const char slev[] = " EACewnid-";
  if (!default_logger_fp) {
    MUTEX_LOCK(grn_glock);
    if (!default_logger_fp) {
      default_logger_fp = fopen(GROONGA_LOG_PATH, "a");
    }
    MUTEX_UNLOCK(grn_glock);
  }
  if (default_logger_fp) {
    if (location && *location) {
      fprintf(default_logger_fp, "%s|%c|%s %s %s\n",
              time, *(slev + level), title, msg, location);
    } else {
      fprintf(default_logger_fp, "%s|%c|%s %s\n", time, *(slev + level), title, msg);
    }
    fflush(default_logger_fp);
  }
}

static grn_logger_info default_logger = {
  GRN_LOG_DEFAULT_LEVEL,
  GRN_LOG_TIME|GRN_LOG_MESSAGE,
  default_logger_func
};

static const grn_logger_info *grn_logger = &default_logger;

static grn_obj grn_true_, grn_false_, grn_null_;
grn_obj *grn_true, *grn_false, *grn_null;

grn_rc
grn_init(void)
{
  grn_rc rc;
  grn_ctx *ctx = &grn_gctx;
  grn_logger = &default_logger;
  MUTEX_INIT(grn_glock);
  grn_gtick = 0;
  grn_ql_init_const();
  ctx->next = ctx;
  ctx->prev = ctx;
  grn_ctx_init(ctx, 0);
  ctx->encoding = grn_strtoenc(GROONGA_DEFAULT_ENCODING);
  grn_true = &grn_true_;
  grn_false = &grn_false_;
  grn_null = &grn_null_;
  GRN_VOID_INIT(grn_true);
  GRN_VOID_INIT(grn_false);
  GRN_VOID_INIT(grn_null);
  grn_timeval_now(ctx, &grn_starttime);
#ifdef WIN32
  {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    grn_pagesize = si.dwAllocationGranularity;
  }
#else /* WIN32 */
  if ((grn_pagesize = sysconf(_SC_PAGESIZE)) == -1) {
    SERR("_SC_PAGESIZE");
    return ctx->rc;
  }
#endif /* WIN32 */
  if (grn_pagesize & (grn_pagesize - 1)) {
    GRN_LOG(ctx, GRN_LOG_CRIT, "pagesize=%x", grn_pagesize);
  }
  // expand_stack();
#ifdef USE_AIO
  if (getenv("GRN_DEBUG_PRINT")) {
    grn_debug_print = atoi(getenv("GRN_DEBUG_PRINT"));
  } else {
    grn_debug_print = 0;
  }
  if (getenv("GRN_AIO_ENABLED")) {
    grn_aio_enabled = atoi(getenv("GRN_AIO_ENABLED"));
  } else {
    grn_aio_enabled = 0;
  }
  if (grn_aio_enabled) {
    GRN_LOG(ctx, GRN_LOG_NOTICE, "AIO and DIO enabled");
    grn_cache_open();
  }
#endif /* USE_AIO */
#ifdef USE_FAIL_MALLOC
  if (getenv("GRN_FMALLOC_PROB")) {
    grn_fmalloc_prob = strtod(getenv("GRN_FMALLOC_PROB"), 0) * RAND_MAX;
    if (getenv("GRN_FMALLOC_SEED")) {
      srand((unsigned int)atoi(getenv("GRN_FMALLOC_SEED")));
    } else {
      srand((unsigned int)time(NULL));
    }
  }
  if (getenv("GRN_FMALLOC_FUNC")) {
    grn_fmalloc_func = getenv("GRN_FMALLOC_FUNC");
  }
  if (getenv("GRN_FMALLOC_FILE")) {
    grn_fmalloc_file = getenv("GRN_FMALLOC_FILE");
  }
  if (getenv("GRN_FMALLOC_LINE")) {
    grn_fmalloc_line = atoi(getenv("GRN_FMALLOC_LINE"));
  }
#endif /* USE_FAIL_MALLOC */
  if ((rc = grn_token_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_token_init failed (%d)", rc);
    return rc;
  }
  if ((rc = grn_com_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_com_init failed (%d)", rc);
    return rc;
  }
  grn_ctx_ql_init(ctx, 0);
  if ((rc = ctx->rc)) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "gctx initialize failed (%d)", rc);
    return rc;
  }
  if ((rc = grn_io_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "io initialize failed (%d)", rc);
    return rc;
  }
  /*
  if ((rc = grn_index_init())) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "index initialize failed (%d)", rc);
    return rc;
  }
  */
  GRN_LOG(ctx, GRN_LOG_NOTICE, "grn_init");
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
    grn_gctx.encoding = grn_strtoenc(GROONGA_DEFAULT_ENCODING);
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

static int alloc_count = 0;

grn_rc
grn_fin(void)
{
  grn_ctx *ctx, *ctx_;
  grn_rc rc = GRN_SUCCESS;
  for (ctx = grn_gctx.next; ctx != &grn_gctx; ctx = ctx_) {
    ctx_ = ctx->next;
    if (ctx->stat != GRN_CTX_FIN) { grn_ctx_fin(ctx); }
    if (ctx->flags & GRN_CTX_ALLOCATED) {
      ctx->next->prev = ctx->prev;
      ctx->prev->next = ctx->next;
      GRN_GFREE(ctx);
    }
  }
  grn_io_fin();
  grn_ctx_fin(ctx);
  grn_token_fin();
  grn_com_fin();
  GRN_LOG(ctx, GRN_LOG_NOTICE, "grn_fin (%d)", alloc_count);
  grn_logger_fin();
  MUTEX_DESTROY(grn_glock);
  return rc;
}

grn_rc
grn_ctx_connect(grn_ctx *ctx, const char *host, int port, int flags)
{
  if (!ctx->impl) { grn_ctx_impl_init(ctx); }
  if (!ctx->impl) { return ctx->rc; }
  {
    grn_com *com = grn_com_copen(ctx, NULL, host, port);
    if (com) {
      ctx->impl->com = com;
      return GRN_SUCCESS;
    }
  }
  return ctx->rc;
}

grn_rc
grn_ctx_close(grn_ctx *ctx)
{
  grn_rc rc = grn_ctx_fin(ctx);
  GRN_GFREE(ctx);
  return rc;
}

#define EXPR_MISSING "expr_missing"

static void
put_response_header(grn_ctx *ctx, const char *p, const char *pe,
                    grn_content_type *ct, const char **name, unsigned int *name_len)
{
  const char *pd = NULL;
  grn_obj *head = ctx->impl->outbuf;
  for (*name = p; p < pe && *p != '?'; p++) {
    if (*p == '.') {
      pd = p;
    } else if (*p == '/') {
      *name = p + 1;
    }
  }
  GRN_TEXT_INIT(head, 0);
  GRN_TEXT_PUTS(ctx, head, "HTTP/1.1 200 OK\r\n");
  GRN_TEXT_PUTS(ctx, head, "Connection: close\r\n");
  if (pd && pd < p) {
    *name_len = pd - *name;
    switch (*++pd) {
    case 'c' :
      if (pd + 3 == p && !memcmp(pd, "css", 3)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: text/css\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    case 'g' :
      if (pd + 3 == p && !memcmp(pd, "gif", 3)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: image/gif\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    case 'h' :
      if (pd + 4 == p && !memcmp(pd, "html", 4)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: text/html\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    case 'j' :
      if (!memcmp(pd, "js", 2)) {
        if (pd + 2 == p) {
          GRN_TEXT_PUTS(ctx, head, "Content-Type: text/javascript\r\n\r\n");
          *ct = GRN_CONTENT_NONE;
        } else if (pd + 4 == p && !memcmp(pd + 2, "on", 2)) {
          GRN_TEXT_PUTS(ctx, head, "Content-Type: text/javascript\r\n\r\n");
          *ct = GRN_CONTENT_JSON;
        }
      } else if (pd + 3 == p && !memcmp(pd, "jpg", 3)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: image/jpeg\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    case 'p' :
      if (pd + 3 == p && !memcmp(pd, "png", 3)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: image/png\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    case 't' :
      if (pd + 3 == p && !memcmp(pd, "txt", 3)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: text/plain\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    case 'x':
      if (pd + 3 == p && !memcmp(pd, "xml", 3)) {
        GRN_TEXT_PUTS(ctx, head, "Content-Type: text/xml\r\n\r\n");
        *ct = GRN_CONTENT_NONE;
      }
      break;
    }
  } else {
    *name_len = p - *name;
    GRN_TEXT_PUTS(ctx, head, "Content-Type: text/javascript\r\n\r\n");
    *ct = GRN_CONTENT_JSON;
  }
}

#define OUTPUT_TYPE "output_type"
#define OUTPUT_TYPE_LEN (sizeof(OUTPUT_TYPE) - 1)

grn_obj *
grn_ctx_qe_exec_uri(grn_ctx *ctx, const char *str, uint32_t str_size)
{
  const char *p, *e;
  grn_obj *expr, *val = NULL;
  if (ctx->impl->qe_next) {
    expr = ctx->impl->qe_next;
    ctx->impl->qe_next = NULL;
    if ((val = grn_expr_get_var_by_offset(ctx, expr, 0))) {
      grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
      GRN_TEXT_PUT(ctx, val, str, str_size);
    }
    grn_ctx_push(ctx, ctx->impl->outbuf);
    grn_expr_exec(ctx, expr, 1);
    val = grn_ctx_pop(ctx);
    grn_expr_clear_vars(ctx, expr);
  } else {
    grn_obj key;
    const char *g, *name;
    unsigned int name_len;
    grn_content_type ot;
    GRN_TEXT_INIT(&key, 0);
    p = str;
    e = p + str_size;
    g = grn_text_urldec(ctx, &key, p, e, '?');
    put_response_header(ctx, GRN_TEXT_VALUE(&key), GRN_TEXT_VALUE(&key) + GRN_TEXT_LEN(&key),
                        &ot, &name, &name_len);
    /* TODO: /cgi-bin/ */
    if ((expr = grn_ctx_get(ctx, name, name_len))) {
      while (g < e) {
        GRN_BULK_REWIND(&key);
        g = grn_text_cgidec(ctx, &key, g, e, '=');
        if (!(val = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(&key), GRN_TEXT_LEN(&key)))) {
          val = &key;
        }
        grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
        g = grn_text_cgidec(ctx, val, g, e, '&');
      }
      if ((val = grn_expr_get_var(ctx, expr, OUTPUT_TYPE, OUTPUT_TYPE_LEN))) {
        grn_obj_reinit(ctx, val, GRN_DB_INT32, 0);
        GRN_INT32_SET(ctx, val, (int32_t)ot);
      }

      grn_ctx_push(ctx, ctx->impl->outbuf);
      grn_expr_exec(ctx, expr, 1);
      val = grn_ctx_pop(ctx);
      grn_expr_clear_vars(ctx, expr);
    } else if ((expr = grn_ctx_get(ctx, GRN_EXPR_MISSING_NAME,
                                   strlen(GRN_EXPR_MISSING_NAME)))) {
      if ((val = grn_expr_get_var_by_offset(ctx, expr, 0))) {
        grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
        GRN_TEXT_PUT(ctx, val, str, str_size);
      }
      grn_ctx_push(ctx, ctx->impl->outbuf);
      grn_expr_exec(ctx, expr, 1);
      val = grn_ctx_pop(ctx);
      grn_expr_clear_vars(ctx, expr);
    }
    GRN_OBJ_FIN(ctx, &key);
  }
  return val;
}

grn_obj *
grn_ctx_qe_exec(grn_ctx *ctx, const char *str, uint32_t str_size)
{
  grn_obj *expr = NULL, *val = NULL;
  if (ctx->impl->qe_next) {
    expr = ctx->impl->qe_next;
    ctx->impl->qe_next = NULL;
    if ((val = grn_expr_get_var_by_offset(ctx, expr, 0))) {
      grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
      GRN_TEXT_PUT(ctx, val, str, str_size);
    }
  } else {
    grn_obj buf;
    char tok_type;
    int offset = 0;
    const char *v, *p = str, *e = str + str_size;
    GRN_TEXT_INIT(&buf, 0);
    p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
    if ((expr = grn_ctx_get(ctx, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf)))) {
      while (p < e) {
        GRN_BULK_REWIND(&buf);
        p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
        switch (tok_type) {
        case GRN_TOK_VOID :
          p = e;
          break;
        case GRN_TOK_SYMBOL :
          v = GRN_TEXT_VALUE(&buf);
          if (GRN_TEXT_LEN(&buf) > 2 && v[0] == '-' && v[1] == '-') {
            if ((val = grn_expr_get_var(ctx, expr, v + 2, GRN_TEXT_LEN(&buf) - 2))) {
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
          if ((val = grn_expr_get_var_by_offset(ctx, expr, offset++))) {
            grn_obj_reinit(ctx, val, GRN_DB_TEXT, 0);
            GRN_TEXT_PUT(ctx, val, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
          } else {
            p = e;
          }
          break;
        }
      }
    }
    GRN_OBJ_FIN(ctx, &buf);
  }
  if (expr) {
    grn_content_type ot = GRN_CONTENT_JSON;
    if ((val = grn_expr_get_var(ctx, expr, OUTPUT_TYPE, OUTPUT_TYPE_LEN))) {
      grn_obj_reinit(ctx, val, GRN_DB_INT32, 0);
      GRN_INT32_SET(ctx, val, (int32_t)ot);
    }
    grn_ctx_push(ctx, ctx->impl->outbuf);
    grn_expr_exec(ctx, expr, 1);
    val = grn_ctx_pop(ctx);
    grn_expr_clear_vars(ctx, expr);
  }
  return val;
}

grn_rc
grn_ctx_sendv(grn_ctx *ctx, int argc, char **argv, int flags)
{
  grn_obj buf;
  GRN_TEXT_INIT(&buf, 0);
  while (argc--) {
    // todo : encode into json like syntax
    GRN_TEXT_PUTS(ctx, &buf, *argv);
    argv++;
    if (argc) { GRN_TEXT_PUTC(ctx, &buf, ' '); }
  }
  grn_ctx_send(ctx, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf), flags);
  GRN_OBJ_FIN(ctx, &buf);
  return ctx->rc;
}

unsigned int
grn_ctx_send(grn_ctx *ctx, char *str, unsigned int str_len, int flags)
{
  if (!ctx) { return 0; }
  GRN_API_ENTER;
  if (ctx->impl) {
    if (ctx->impl->com) {
      grn_rc rc;
      grn_com_header sheader;
      if ((flags & GRN_CTX_MORE)) { flags |= GRN_CTX_QUIET; }
      if (ctx->stat == GRN_CTX_QUIT) { flags |= GRN_CTX_QUIT; }
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
      if (ctx->impl->symbols) {
        grn_ql_feed(ctx, str, str_len, flags);
        if (ctx->stat == GRN_CTX_QUITTING) { ctx->stat = GRN_CTX_QUIT; }
        if (!ERRP(ctx, GRN_CRIT)) {
          if (!(flags & GRN_CTX_QUIET) && ctx->impl->output) {
            ctx->impl->output(ctx, 0, ctx->impl->data.ptr);
          }
        }
        goto exit;
      } else {
        if (str_len && *str == '/') {
          grn_ctx_qe_exec_uri(ctx, str + 1, str_len - 1);
        } else {
          grn_ctx_qe_exec(ctx, str, str_len);
        }
        if (ctx->stat == GRN_CTX_QUITTING) { ctx->stat = GRN_CTX_QUIT; }
        if (!ERRP(ctx, GRN_CRIT)) {
          if (!(flags & GRN_CTX_QUIET) && ctx->impl->output) {
            ctx->impl->output(ctx, 0, ctx->impl->data.ptr);
          }
        }
        goto exit;
      }
    }
  }
  ERR(GRN_INVALID_ARGUMENT, "invalid ctx assigned");
exit :
  GRN_API_RETURN(0);
}

unsigned
grn_ctx_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  if (ctx->stat == GRN_CTX_QUIT) {
    *str = NULL;
    *str_len = 0;
    *flags = GRN_CTX_QUIT;
    return 0;
  }
  GRN_API_ENTER;
  if (ctx->impl) {
    if (ctx->impl->com) {
      grn_com_header header;
      if (grn_com_recv(ctx, ctx->impl->com, &header, ctx->impl->outbuf)) {
        *str = NULL;
        *str_len = 0;
        *flags = 0;
      } else {
        *str = GRN_BULK_HEAD(ctx->impl->outbuf);
        *str_len = GRN_BULK_VSIZE(ctx->impl->outbuf);
        if (header.flags & GRN_CTX_QUIT) {
          ctx->stat = GRN_CTX_QUIT;
          *flags = GRN_CTX_QUIT;
        } else {
          *flags = (header.flags & GRN_CTX_TAIL) ? 0 : GRN_CTX_MORE;
        }
      }
      if (ctx->rc) {
        ERR(ctx->rc, "grn_com_recv failed!");
      }
      goto exit;
    } else {
      if (ctx->impl->symbols) {
        grn_obj *buf = ctx->impl->outbuf;
        unsigned int head, tail;
        unsigned int *offsets = (unsigned int *) GRN_BULK_HEAD(&ctx->impl->subbuf);
        int npackets = GRN_BULK_VSIZE(&ctx->impl->subbuf) / sizeof(unsigned int);
        if (npackets < ctx->impl->bufcur) {
          ERR(GRN_INVALID_ARGUMENT, "invalid argument");
          goto exit;
        }
        head = ctx->impl->bufcur ? offsets[ctx->impl->bufcur - 1] : 0;
        tail = ctx->impl->bufcur < npackets ? offsets[ctx->impl->bufcur] : GRN_BULK_VSIZE(buf);
        *str = GRN_BULK_HEAD(buf) + head;
        *str_len = tail - head;
        *flags = ctx->impl->bufcur++ < npackets ? GRN_CTX_MORE : 0;
        goto exit;
      }
    }
  }
  ERR(GRN_INVALID_ARGUMENT, "invalid ctx assigned");
exit :
  GRN_API_RETURN(0);
}

void
grn_ctx_concat_func(grn_ctx *ctx, int flags, void *dummy)
{
  if (ctx && ctx->impl && (flags & GRN_CTX_MORE)) {
    unsigned int size = GRN_BULK_VSIZE(ctx->impl->outbuf);
    grn_bulk_write(ctx, &ctx->impl->subbuf, (char *) &size, sizeof(unsigned int));
  }
}

void
grn_ctx_stream_out_func(grn_ctx *ctx, int flags, void *stream)
{
  if (ctx && ctx->impl) {
    grn_obj *buf = ctx->impl->outbuf;
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
grn_ctx_recv_handler_set(grn_ctx *ctx, void (*func)(grn_ctx *, int, void *), void *func_arg)
{
  if (ctx && ctx->impl) {
    ctx->impl->output = func;
    ctx->impl->data.ptr = func_arg;
  }
}

grn_rc
grn_ctx_info_get(grn_ctx *ctx, grn_ctx_info *info)
{
  if (!ctx || !ctx->impl) { return GRN_INVALID_ARGUMENT; }
  if (ctx->impl->com) {
    info->fd = ctx->impl->com->fd;
    info->com_status = ctx->impl->com_status;
    info->outbuf = ctx->impl->outbuf;
    info->stat = ctx->stat;
  } else {
    info->fd = -1;
    info->com_status = 0;
    info->outbuf = ctx->impl->outbuf;
    info->stat = ctx->stat;
  }
  return GRN_SUCCESS;
}


grn_cell *
grn_get(const char *key)
{
  grn_cell *obj;
  if (!grn_gctx.impl || !grn_gctx.impl->symbols ||
      !grn_hash_add(&grn_gctx, grn_gctx.impl->symbols, key, strlen(key),
                    (void **) &obj, NULL)) {
    GRN_LOG(&grn_gctx, GRN_LOG_WARNING, "grn_get(%s) failed", key);
    return F;
  }
  if (!obj->header.impl_flags) {
    obj->header.impl_flags |= GRN_CELL_SYMBOL;
    obj->header.type = GRN_VOID;
  }
  return obj;
}

grn_cell *
grn_at(const char *key)
{
  grn_cell *obj;
  if (!grn_gctx.impl || grn_gctx.impl->symbols ||
      !grn_hash_get(&grn_gctx, grn_gctx.impl->symbols,
                   key, strlen(key), (void **) &obj)) {
    return F;
  }
  return obj;
}

grn_rc
grn_del(const char *key)
{
  if (!grn_gctx.impl || !grn_gctx.impl->symbols) {
    GRN_LOG(&grn_gctx, GRN_LOG_WARNING, "grn_del(%s) failed", key);
    return GRN_INVALID_ARGUMENT;
  }
  return grn_hash_delete(&grn_gctx, grn_gctx.impl->symbols, key, strlen(key), NULL);
}

/**** memory allocation ****/

#define ALIGN_SIZE (1<<3)
#define ALIGN_MASK (ALIGN_SIZE-1)

void *
grn_ctx_alloc(grn_ctx *ctx, size_t size, int flags,
              const char* file, int line, const char *func)
{
  if (!ctx) { return NULL; }
  if (!ctx->impl) {
    grn_ctx_impl_init(ctx);
    if (ERRP(ctx, GRN_ERROR)) { return NULL; }
  }
  {
    int32_t i;
    uint64_t *header;
    grn_io_mapinfo *mi;
    size = ((size + ALIGN_MASK) & ~ALIGN_MASK) + ALIGN_SIZE;
    if (size > SEGMENT_SIZE) {
      uint64_t npages = (size + (grn_pagesize - 1)) / grn_pagesize;
      if (npages >= (1LL<<32)) {
        MERR("too long request size=%zu", size);
        return NULL;
      }
      for (i = 0, mi = ctx->impl->segs;; i++, mi++) {
        if (i >= N_SEGMENTS) {
          MERR("all segments are full");
          return NULL;
        }
        if (!mi->map) { break; }
      }
      if (!grn_io_anon_map(ctx, mi, npages * grn_pagesize)) { return NULL; }
      //GRN_LOG(ctx, GRN_LOG_NOTICE, "map i=%d (%d)", i, npages * grn_pagesize);
      mi->nref = (uint32_t) npages;
      mi->count = SEGMENT_VLEN;
      ctx->impl->currseg = -1;
      header = mi->map;
      *header = (uint64_t) i;
      return &header[1];
    } else {
      i = ctx->impl->currseg;
      mi = &ctx->impl->segs[i];
      if (i < 0 || size + mi->nref > SEGMENT_SIZE) {
        for (i = 0, mi = ctx->impl->segs;; i++, mi++) {
          if (i >= N_SEGMENTS) {
            MERR("all segments are full");
            return NULL;
          }
          if (!mi->map) { break; }
        }
        if (!grn_io_anon_map(ctx, mi, SEGMENT_SIZE)) { return NULL; }
        //GRN_LOG(ctx, GRN_LOG_NOTICE, "map i=%d", i);
        mi->nref = 0;
        mi->count = SEGMENT_WORD;
        ctx->impl->currseg = i;
      }
      header = (uint64_t *)((byte *)mi->map + mi->nref);
      mi->nref += size;
      mi->count++;
      *header = (uint64_t) i;
      return &header[1];
    }
  }
}

void
grn_ctx_free(grn_ctx *ctx, void *ptr,
             const char* file, int line, const char *func)
{
  if (!ctx) { return; }
  if (!ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT,"ctx without impl passed.");
    return;
  }
  {
    uint64_t *header = &((uint64_t *)ptr)[-1];
    if (*header >= N_SEGMENTS) {
      ERR(GRN_INVALID_ARGUMENT,"invalid ptr passed. ptr=%p seg=%zu", ptr, *header);
      return;
    }
    {
      int32_t i = (int32_t)*header;
      grn_io_mapinfo *mi = &ctx->impl->segs[i];
      if (mi->count & SEGMENT_VLEN) {
        if (mi->map != header) {
          ERR(GRN_INVALID_ARGUMENT,"invalid ptr passed.. ptr=%p seg=%d", ptr, i);
          return;
        }
        //GRN_LOG(ctx, GRN_LOG_NOTICE, "umap i=%d (%d)", i, mi->nref * grn_pagesize);
        grn_io_anon_unmap(ctx, mi, mi->nref * grn_pagesize);
        mi->map = NULL;
      } else {
        if (!mi->map) {
          ERR(GRN_INVALID_ARGUMENT,"invalid ptr passed... ptr=%p seg=%d", ptr, i);
          return;
        }
        mi->count--;
        if (!(mi->count & SEGMENT_MASK)) {
          //GRN_LOG(ctx, GRN_LOG_NOTICE, "umap i=%d", i);
          if (i == ctx->impl->currseg) {
            memset(mi->map, 0, mi->nref);
            mi->nref = 0;
          } else {
            grn_io_anon_unmap(ctx, mi, SEGMENT_SIZE);
            mi->map = NULL;
          }
        }
      }
    }
  }
}

#define DB_P(s) ((s) && (s)->header.type == GRN_DB)

grn_rc
grn_ctx_use(grn_ctx *ctx, grn_obj *db)
{
  GRN_API_ENTER;
  if (db && !DB_P(db)) {
    ctx->rc = GRN_INVALID_ARGUMENT;
  } else {
    if (!ctx->impl) { grn_ctx_impl_init(ctx); }
    if (!ctx->rc) {
      ctx->impl->db = db;
      if (db) {
        grn_obj buf;
        if (ctx->impl->symbols) { grn_ql_def_db_funcs(ctx); }
        GRN_TEXT_INIT(&buf, 0);
        grn_obj_get_info(ctx, db, GRN_INFO_ENCODING, &buf);
        ctx->encoding = *(grn_encoding *)GRN_BULK_HEAD(&buf);
        grn_obj_close(ctx, &buf);
      }
    }
  }
  GRN_API_RETURN(ctx->rc);
}

void *
grn_ctx_alloc_lifo(grn_ctx *ctx, size_t size, int flags,
                   const char* file, int line, const char *func)
{
  if (!ctx) { return NULL; }
  if (!ctx->impl) {
    grn_ctx_impl_init(ctx);
    if (ERRP(ctx, GRN_ERROR)) { return NULL; }
  }
  {
    int32_t i = ctx->impl->lifoseg;
    grn_io_mapinfo *mi = &ctx->impl->segs[i];
    if (size > SEGMENT_SIZE) {
      uint64_t npages = (size + (grn_pagesize - 1)) / grn_pagesize;
      if (npages >= (1LL<<32)) {
        MERR("too long request size=%zu", size);
        return NULL;
      }
      for (;;) {
        if (++i >= N_SEGMENTS) {
          MERR("all segments are full");
          return NULL;
        }
        mi++;
        if (!mi->map) { break; }
      }
      if (!grn_io_anon_map(ctx, mi, npages * grn_pagesize)) { return NULL; }
      mi->nref = (uint32_t) npages;
      mi->count = SEGMENT_VLEN|SEGMENT_LIFO;
      ctx->impl->lifoseg = i;
      return mi->map;
    } else {
      size = (size + ALIGN_MASK) & ~ALIGN_MASK;
      if (i < 0 || (mi->count & SEGMENT_VLEN) || size + mi->nref > SEGMENT_SIZE) {
        for (;;) {
          if (++i >= N_SEGMENTS) {
            MERR("all segments are full");
            return NULL;
          }
          if (!(++mi)->map) { break; }
        }
        if (!grn_io_anon_map(ctx, mi, SEGMENT_SIZE)) { return NULL; }
        mi->nref = 0;
        mi->count = SEGMENT_WORD|SEGMENT_LIFO;
        ctx->impl->lifoseg = i;
      }
      {
        uint32_t u = mi->nref;
        mi->nref += size;
        return (byte *)mi->map + u;
      }
    }
  }
}

void
grn_ctx_free_lifo(grn_ctx *ctx, void *ptr,
                  const char* file, int line, const char *func)
{
  if (!ctx) { return; }
  if (!ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT,"ctx without impl passed.");
    return;
  }
  {
    int32_t i = ctx->impl->lifoseg, done = 0;
    grn_io_mapinfo *mi = &ctx->impl->segs[i];
    if (i < 0) {
      ERR(GRN_INVALID_ARGUMENT, "lifo buffer is void");
      return;
    }
    for (; i >= 0; i--, mi--) {
      if (!(mi->count & SEGMENT_LIFO)) { continue; }
      if (done) { break; }
      if (mi->count & SEGMENT_VLEN) {
        if (mi->map == ptr) { done = 1; }
        grn_io_anon_unmap(ctx, mi, mi->nref * grn_pagesize);
        mi->map = NULL;
      } else {
        if (mi->map == ptr) {
          done = 1;
        } else {
          if (mi->map < ptr && ptr < (void *)((byte*)mi->map + mi->nref)) {
            mi->nref = (uint32_t) ((uintptr_t)ptr - (uintptr_t)mi->map);
            break;
          }
        }
        grn_io_anon_unmap(ctx, mi, SEGMENT_SIZE);
        mi->map = NULL;
      }
    }
    ctx->impl->lifoseg = i;
  }
}

#if USE_DYNAMIC_MALLOC_CHANGE
grn_malloc_func
grn_ctx_get_malloc(grn_ctx *ctx)
{
  if (!ctx || !ctx->impl) { return NULL; }
  return ctx->impl->malloc_func;
}

void
grn_ctx_set_malloc(grn_ctx *ctx, grn_malloc_func malloc_func)
{
  if (!ctx || !ctx->impl) { return; }
  ctx->impl->malloc_func = malloc_func;
}

grn_calloc_func
grn_ctx_get_calloc(grn_ctx *ctx)
{
  if (!ctx || !ctx->impl) { return NULL; }
  return ctx->impl->calloc_func;
}

void
grn_ctx_set_calloc(grn_ctx *ctx, grn_calloc_func calloc_func)
{
  if (!ctx || !ctx->impl) { return; }
  ctx->impl->calloc_func = calloc_func;
}

grn_realloc_func
grn_ctx_get_realloc(grn_ctx *ctx)
{
  if (!ctx || !ctx->impl) { return NULL; }
  return ctx->impl->realloc_func;
}

void
grn_ctx_set_realloc(grn_ctx *ctx, grn_realloc_func realloc_func)
{
  if (!ctx || !ctx->impl) { return; }
  ctx->impl->realloc_func = realloc_func;
}

grn_strdup_func
grn_ctx_get_strdup(grn_ctx *ctx)
{
  if (!ctx || !ctx->impl) { return NULL; }
  return ctx->impl->strdup_func;
}

void
grn_ctx_set_strdup(grn_ctx *ctx, grn_strdup_func strdup_func)
{
  if (!ctx || !ctx->impl) { return; }
  ctx->impl->strdup_func = strdup_func;
}

void *
grn_malloc(grn_ctx *ctx, size_t size, const char* file, int line, const char *func)
{
  if (ctx && ctx->impl && ctx->impl->malloc_func) {
    return ctx->impl->malloc_func(ctx, size, file, line, func);
  } else {
    return grn_malloc_default(ctx, size, file, line, func);
  }
}

void *
grn_calloc(grn_ctx *ctx, size_t size, const char* file, int line, const char *func)
{
  if (ctx && ctx->impl && ctx->impl->calloc_func) {
    return ctx->impl->calloc_func(ctx, size, file, line, func);
  } else {
    return grn_calloc_default(ctx, size, file, line, func);
  }
}

void *
grn_realloc(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func)
{
  if (ctx && ctx->impl && ctx->impl->realloc_func) {
    return ctx->impl->realloc_func(ctx, ptr, size, file, line, func);
  } else {
    return grn_realloc_default(ctx, ptr, size, file, line, func);
  }
}

char *
grn_strdup(grn_ctx *ctx, const char *string, const char* file, int line, const char *func)
{
  if (ctx && ctx->impl && ctx->impl->strdup_func) {
    return ctx->impl->strdup_func(ctx, string, file, line, func);
  } else {
    return grn_strdup_default(ctx, string, file, line, func);
  }
}
#endif

void *
grn_malloc_default(grn_ctx *ctx, size_t size, const char* file, int line, const char *func)
{
  if (!ctx) { return NULL; }
  {
    void *res = malloc(size);
    if (res) {
      GRN_ADD_ALLOC_COUNT(1);
    } else {
      if (!(res = malloc(size))) {
        MERR("malloc fail (%d)=%p (%s:%d) <%d>", size, res, file, line, alloc_count);
      } else {
        GRN_ADD_ALLOC_COUNT(1);
      }
    }
    return res;
  }
}

void *
grn_calloc_default(grn_ctx *ctx, size_t size, const char* file, int line, const char *func)
{
  if (!ctx) { return NULL; }
  {
    void *res = calloc(size, 1);
    if (res) {
      GRN_ADD_ALLOC_COUNT(1);
    } else {
      if (!(res = calloc(size, 1))) {
        MERR("calloc fail (%d)=%p (%s:%d) <%d>", size, res, file, line, alloc_count);
      } else {
        GRN_ADD_ALLOC_COUNT(1);
      }
    }
    return res;
  }
}

void
grn_free(grn_ctx *ctx, void *ptr, const char* file, int line)
{
  if (!ctx) { return; }
  {
    free(ptr);
    if (ptr) {
        GRN_ADD_ALLOC_COUNT(-1);
    } else {
      GRN_LOG(ctx, GRN_LOG_ALERT, "free fail (%p) (%s:%d) <%d>", ptr, file, line, alloc_count);
    }
  }
}

void *
grn_realloc_default(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func)
{
  void *res;
  if (!ctx) { return NULL; }
  if (size) {
    if (!(res = realloc(ptr, size))) {
      if (!(res = realloc(ptr, size))) {
        MERR("realloc fail (%p,%zu)=%p (%s:%d) <%d>", ptr, size, res, file, line, alloc_count);
        return NULL;
      }
    }
    if (!ptr) { GRN_ADD_ALLOC_COUNT(1); }
  } else {
    if (!ptr) { return NULL; }
    GRN_ADD_ALLOC_COUNT(-1);
#if defined __FreeBSD__
    free(ptr);
    return NULL;
#else /* __FreeBSD__ */
    res = realloc(ptr, size);
    if (res) {
      GRN_LOG(ctx, GRN_LOG_ALERT, "realloc(%p,%zu)=%p (%s:%d) <%d>", ptr, size, res, file, line, alloc_count);
      // grn_free(ctx, res, file, line);
    }
#endif /* __FreeBSD__ */
  }
  return res;
}

int
grn_alloc_count(void)
{
  return alloc_count;
}

char *
grn_strdup_default(grn_ctx *ctx, const char *s, const char* file, int line, const char *func)
{
  if (!ctx) { return NULL; }
  {
    char *res = strdup(s);
    if (res) {
      GRN_ADD_ALLOC_COUNT(1);
    } else {
      if (!(res = strdup(s))) {
        MERR("strdup(%p)=%p (%s:%d) <%d>", s, res, file, line, alloc_count);
      }
    }
    return res;
  }
}

#ifdef USE_FAIL_MALLOC
int
grn_fail_malloc_check(size_t size, const char *file, int line, const char *func)
{
  if ((grn_fmalloc_file && strcmp(file, grn_fmalloc_file)) ||
      (grn_fmalloc_line && line != grn_fmalloc_line) ||
      (grn_fmalloc_func && strcmp(func, grn_fmalloc_func))) {
    return 1;
  }
  if (grn_fmalloc_prob && grn_fmalloc_prob >= rand()) {
    return 0;
  }
  return 1;
}

void *
grn_malloc_fail(grn_ctx *ctx, size_t size, const char* file, int line, const char *func)
{
  if (grn_fail_malloc_check(size, file, line, func)) {
    return grn_malloc_default(ctx, size, file, line, func);
  } else {
    MERR("fail_malloc (%d) (%s:%d@%s) <%d>", size, file, line, func, alloc_count);
    return NULL;
  }
}

void *
grn_calloc_fail(grn_ctx *ctx, size_t size, const char* file, int line, const char *func)
{
  if (grn_fail_malloc_check(size, file, line, func)) {
    return grn_calloc_default(ctx, size, file, line, func);
  } else {
    MERR("fail_calloc (%d) (%s:%d@%s) <%d>", size, file, line, func, alloc_count);
    return NULL;
  }
}

void *
grn_realloc_fail(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line,
                 const char *func)
{
  if (grn_fail_malloc_check(size, file, line, func)) {
    return grn_realloc_default(ctx, ptr, size, file, line, func);
  } else {
    MERR("fail_realloc (%p,%zu) (%s:%d@%s) <%d>", ptr, size, file, line, func, alloc_count);
    return NULL;
  }
}

char *
grn_strdup_fail(grn_ctx *ctx, const char *s, const char* file, int line, const char *func)
{
  if (grn_fail_malloc_check(strlen(s), file, line, func)) {
    return grn_strdup_default(ctx, s, file, line, func);
  } else {
    MERR("fail_strdup(%p) (%s:%d@%s) <%d>", s, file, line, func, alloc_count);
    return NULL;
  }
}
#endif /* USE_FAIL_MALLOC */

grn_cell *
grn_cell_new(grn_ctx *ctx)
{
  grn_cell *o = NULL;
  if (ctx && ctx->impl) {
    grn_array_add(ctx, ctx->impl->objects, (void **)&o);
    if (o) {
      o->header.impl_flags = 0;
      ctx->impl->n_entries++;
    }
  }
  return o;
}

grn_cell *
grn_cell_cons(grn_ctx *ctx, grn_cell *a, grn_cell *b)
{
  if (!ctx) { return NULL; }
  {
    grn_cell *o;
    GRN_CELL_NEW(ctx, o);
    if (o) {
      o->header.type = GRN_CELL_LIST;
      o->header.impl_flags = 0;
      o->u.l.car = a;
      o->u.l.cdr = b;
    }
    return o;
  }
}

grn_cell *
grn_cell_alloc(grn_ctx *ctx, uint32_t size)
{
  if (!ctx) { return NULL; }
  {
    void *value = GRN_MALLOC(size + 1);
    if (value) {
      grn_cell *o = grn_cell_new(ctx);
      if (!ERRP(ctx, GRN_ERROR)) {
        o->header.impl_flags = GRN_OBJ_ALLOCATED;
        o->header.type = GRN_CELL_STR;
        o->u.b.size = size;
        o->u.b.value = value;
        return o;
      }
      GRN_FREE(value);
    } else {
      MERR("malloc(%d) failed", size + 1);
    }
    return NULL;
  }
}

void
grn_cell_clear(grn_ctx *ctx, grn_cell *o)
{
  if (!ctx || !ctx->impl) { return; }
  if (o->header.impl_flags & GRN_OBJ_ALLOCATED) {
    switch (o->header.type) {
    case GRN_SNIP :
      if (o->u.p.value) { grn_snip_close(ctx, (grn_snip *)o->u.p.value); }
      break;
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_PAT_KEY :
    case GRN_TABLE_NO_KEY :
      grn_obj_close(ctx, grn_ctx_at(ctx, o->u.o.id));
      break;
    case GRN_CELL_STR :
      if (o->u.b.value) {
        GRN_FREE(o->u.b.value);
      }
      break;
    case GRN_QUERY :
      if (o->u.p.value) { grn_query_close(ctx, (grn_query *)o->u.p.value); }
      break;
    case GRN_UVECTOR :
    case GRN_VECTOR :
      if (o->u.p.value) { grn_obj_close(ctx, o->u.p.value); }
      break;
    case GRN_PATSNIP :
      grn_obj_patsnip_spec_close(ctx, (patsnip_spec *)o->u.p.value);
      break;
    default :
      GRN_LOG(ctx, GRN_LOG_WARNING, "obj_clear: invalid type(%x)", o->header.type);
      break;
    }
    o->header.impl_flags &= ~GRN_OBJ_ALLOCATED;
  }
}

/* don't handle error inside logger functions */

void
grn_ctx_log(grn_ctx *ctx, char *fmt, ...)
{
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(ctx->errbuf, GRN_CTX_MSGSIZE - 1, fmt, argp);
  va_end(argp);
  ctx->errbuf[GRN_CTX_MSGSIZE - 1] = '\0';
}

void
grn_logger_fin(void)
{
  if (default_logger_fp) {
    fclose(default_logger_fp);
    default_logger_fp = NULL;
  }
}

grn_rc
grn_logger_info_set(grn_ctx *ctx, const grn_logger_info *info)
{
  if (info) {
    grn_logger = info;
  } else {
    grn_logger = &default_logger;
  }
  return GRN_SUCCESS;
}

int
grn_logger_pass(grn_ctx *ctx, grn_log_level level)
{
  return level <= grn_logger->max_level;
}

#define TBUFSIZE GRN_TIMEVAL_STR_SIZE
#define MBUFSIZE 0x1000
#define LBUFSIZE 0x400

void
grn_logger_put(grn_ctx *ctx, grn_log_level level,
               const char *file, int line, const char *func, const char *fmt, ...)
{
  if (level <= grn_logger->max_level) {
    char tbuf[TBUFSIZE];
    char mbuf[MBUFSIZE];
    char lbuf[LBUFSIZE];
    tbuf[0] = '\0';
    if (grn_logger->flags & GRN_LOG_TIME) {
      grn_timeval tv;
      grn_timeval_now(ctx, &tv);
      grn_timeval2str(ctx, &tv, tbuf);
    }
    if (grn_logger->flags & GRN_LOG_MESSAGE) {
      va_list argp;
      va_start(argp, fmt);
      vsnprintf(mbuf, MBUFSIZE - 1, fmt, argp);
      va_end(argp);
      mbuf[MBUFSIZE - 1] = '\0';
    } else {
      mbuf[0] = '\0';
    }
    if (grn_logger->flags & GRN_LOG_LOCATION) {
      snprintf(lbuf, LBUFSIZE - 1, "%d %s:%d %s()", getpid(), file, line, func);
      lbuf[LBUFSIZE - 1] = '\0';
    } else {
      lbuf[0] = '\0';
    }
    if (grn_logger->func) {
      grn_logger->func(level, tbuf, "", mbuf, lbuf, grn_logger->func_arg);
    } else {
      default_logger_func(level, tbuf, "", mbuf, lbuf, grn_logger->func_arg);
    }
  }
}

void
grn_assert(grn_ctx *ctx, int cond, const char* file, int line, const char* func)
{
  if (!cond) {
    GRN_LOG(ctx, GRN_LOG_WARNING, "ASSERT fail on %s %s:%d", func, file, line);
  }
}
