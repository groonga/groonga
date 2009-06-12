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
    GRN_CTX_FIN, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }

#define GRN_CTX_CLOSED(ctx) ((ctx)->stat == GRN_CTX_FIN)

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

  ctx->impl->qe = grn_hash_create(ctx, NULL, sizeof(grn_id), sizeof(void *), 0);
  ctx->impl->stack_curr = 0;

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
  // if (ctx->stat != GRN_QL_FIN) { return GRN_INVALID_ARGUMENT; }
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
  MUTEX_LOCK(grn_glock);
  ctx->next = grn_gctx.next;
  ctx->prev = &grn_gctx;
  grn_gctx.next->prev = ctx;
  grn_gctx.next = ctx;
  MUTEX_UNLOCK(grn_glock);
  return ctx->rc;
}

grn_rc
grn_ctx_fin(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  if (ctx->stat == GRN_QL_FIN) { return GRN_INVALID_ARGUMENT; }
  MUTEX_LOCK(grn_glock);
  ctx->next->prev = ctx->prev;
  ctx->prev->next = ctx->next;
  MUTEX_UNLOCK(grn_glock);
  if (ctx->impl) {
    if (ctx->impl->objects) {
      grn_cell *o;
      GRN_ARRAY_EACH(ctx->impl->objects, 0, 0, id, &o, {
        grn_cell_clear(ctx, o);
      });
      grn_array_close(ctx, ctx->impl->objects);
    }
    if (ctx->impl->values) {
      grn_tmp_db_obj *o;
      GRN_ARRAY_EACH(ctx->impl->values, 0, 0, id, &o, {
        grn_obj_close(ctx, (grn_obj *)o->obj);
      });
      grn_array_close(ctx, ctx->impl->values);
    }
    if (ctx->impl->symbols) {
      grn_hash_close(ctx, ctx->impl->symbols);
    }
    if (ctx->impl->com) {
      if (ctx->stat != GRN_QL_QUIT) {
        int flags;
        char *str;
        unsigned int str_len;
        grn_ql_send(ctx, "(quit)", 6, GRN_QL_HEAD);
        grn_ql_recv(ctx, &str, &str_len, &flags);
      }
      grn_ql_send(ctx, "ACK", 3, GRN_QL_HEAD);
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
    grn_hash_close(ctx, ctx->impl->qe);
    GRN_FREE(ctx->impl);
  }
  ctx->stat = GRN_QL_FIN;
  return rc;
}

grn_rc
grn_init(void)
{
  grn_rc rc;
  grn_ctx *ctx = &grn_gctx;
  MUTEX_INIT(grn_glock);
  grn_gtick = 0;
  grn_ql_init_const();
  ctx->next = ctx;
  ctx->prev = ctx;
  grn_ctx_init(ctx, 0);
  ctx->encoding = grn_strtoenc(GROONGA_DEFAULT_ENCODING);
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
  grn_ctx *ctx = &grn_gctx;
  grn_rc rc = GRN_SUCCESS;
  grn_io_fin();
  grn_ctx_fin(ctx);
  grn_token_fin();
  grn_com_fin();
  GRN_LOG(ctx, GRN_LOG_NOTICE, "grn_fin (%d)", alloc_count);
  grn_logger_fin();
  MUTEX_DESTROY(grn_glock);
  return rc;
}

/*
grn_ctx *
grn_ctx_open(grn_obj *db, int flags)
{
  grn_ctx *ctx = GRN_GMALLOCN(grn_ctx, 1);
  if (ctx) {
    grn_ctx_init(ctx, flags);
    if (ERRP(ctx, GRN_ERROR)) {
      grn_ctx_close(ctx);
      return NULL;
    }
    if (ctx->impl && (ctx->impl->db = db) && ctx->impl->symbols) {
      if (grn_ql_def_db_funcs(ctx)) {
        grn_ctx_close(ctx);
        return NULL;
      }
    }
  }
  return ctx;
}
*/

grn_rc
grn_ql_connect(grn_ctx *ctx, const char *host, int port, int flags)
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

grn_obj *grn_ctx_qe_exec(grn_ctx *ctx, const char *str, uint32_t str_size);

grn_rc
grn_ql_send(grn_ctx *ctx, char *str, unsigned int str_len, int flags)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  ERRCLR(ctx);
  if (ctx->impl) {
    if (ctx->impl->com) {
      grn_rc rc;
      grn_com_header sheader;
      if ((flags & GRN_QL_MORE)) { flags |= GRN_QL_QUIET; }
      if (ctx->stat == GRN_QL_QUIT) { flags |= GRN_QL_QUIT; }
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
        if (str_len && *str == '/') {
          grn_ctx_qe_exec(ctx, str, str_len);
        } else {
          grn_ql_feed(ctx, str, str_len, flags);
        }
        if (ctx->stat == GRN_QL_QUITTING) { ctx->stat = GRN_QL_QUIT; }
        if (!ERRP(ctx, GRN_CRIT)) {
          if (!(flags & GRN_QL_QUIET) && ctx->impl->output) {
            ctx->impl->output(ctx, 0, ctx->impl->data.ptr);
          }
        }
        goto exit;
      }
    }
  }
  ERR(GRN_INVALID_ARGUMENT, "invalid ctx assigned");
exit :
  return ctx->rc;
}

grn_rc
grn_ql_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags)
{
  if (!ctx) { return GRN_INVALID_ARGUMENT; }
  ERRCLR(ctx);
  if (ctx->stat == GRN_QL_QUIT) {
    *flags = GRN_QL_QUIT;
    return ctx->rc;
  }
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
        if (header.flags & GRN_QL_QUIT) {
          ctx->stat = GRN_QL_QUIT;
          *flags = GRN_QL_QUIT;
        } else {
          *flags = (header.flags & GRN_QL_TAIL) ? 0 : GRN_QL_MORE;
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
        if (npackets < ctx->impl->bufcur) { return GRN_INVALID_ARGUMENT; }
        head = ctx->impl->bufcur ? offsets[ctx->impl->bufcur - 1] : 0;
        tail = ctx->impl->bufcur < npackets ? offsets[ctx->impl->bufcur] : GRN_BULK_VSIZE(buf);
        *str = GRN_BULK_HEAD(buf) + head;
        *str_len = tail - head;
        *flags = ctx->impl->bufcur++ < npackets ? GRN_QL_MORE : 0;
        goto exit;
      }
    }
  }
  ERR(GRN_INVALID_ARGUMENT, "invalid ctx assigned");
exit :
  return ctx->rc;
}

void
grn_ctx_concat_func(grn_ctx *ctx, int flags, void *dummy)
{
  if (ctx && ctx->impl && (flags & GRN_QL_MORE)) {
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
grn_ql_recv_handler_set(grn_ctx *ctx, void (*func)(grn_ctx *, int, void *), void *func_arg)
{
  if (ctx && ctx->impl) {
    ctx->impl->output = func;
    ctx->impl->data.ptr = func_arg;
  }
}

grn_rc
grn_ql_info_get(grn_ctx *ctx, grn_ql_info *info)
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
          ERR(GRN_INVALID_ARGUMENT,"invalid ptr passed. ptr=%p seg=%d", ptr, i);
          return;
        }
        //GRN_LOG(ctx, GRN_LOG_NOTICE, "umap i=%d (%d)", i, mi->nref * grn_pagesize);
        grn_io_anon_unmap(ctx, mi, mi->nref * grn_pagesize);
        mi->map = NULL;
      } else {
        if (!mi->map) {
          ERR(GRN_INVALID_ARGUMENT,"invalid ptr passed. ptr=%p seg=%d", ptr, i);
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
      alloc_count++;
    } else {
      if (!(res = malloc(size))) {
        MERR("malloc fail (%d)=%p (%s:%d) <%d>", size, res, file, line, alloc_count);
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
      alloc_count++;
    } else {
      if (!(res = calloc(size, 1))) {
        MERR("calloc fail (%d)=%p (%s:%d) <%d>", size, res, file, line, alloc_count);
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
      alloc_count--;
    } else {
      GRN_LOG(ctx, GRN_LOG_ALERT, "free fail (%p) (%s:%d) <%d>", ptr, file, line, alloc_count);
    }
  }
}

void *
grn_realloc_default(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func)
{
  if (!ctx) { return NULL; }
  {
    void *res;
    if (!size) {
      alloc_count--;
#if defined __FreeBSD__
      free(ptr);
      return NULL;
#endif /* __FreeBSD__ */
    }
    res = realloc(ptr, size);
    if (!ptr && res) { alloc_count++; }
    if (size && !res) {
      if (!(res = realloc(ptr, size))) {
        MERR("realloc fail (%p,%zu)=%p (%s:%d) <%d>", ptr, size, res, file, line, alloc_count);
      }
    }
    if (!size && res) {
      GRN_LOG(ctx, GRN_LOG_ALERT, "realloc(%p,%zu)=%p (%s:%d) <%d>", ptr, size, res, file, line, alloc_count);
      // grn_free(ctx, res, file, line);
    }
    return res;
  }
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
      alloc_count++;
    } else  {
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
    fprintf(default_logger_fp, "%s|%c|%s %s %s\n",
            time, *(slev + level), title, msg, location);
    fflush(default_logger_fp);
  }
}

void
grn_logger_fin(void)
{
  if (default_logger_fp) {
    fclose(default_logger_fp);
    default_logger_fp = NULL;
  }
}

static grn_logger_info default_logger = {
  GRN_LOG_DEFAULT_LEVEL,
  GRN_LOG_TIME|GRN_LOG_MESSAGE,
  default_logger_func
};

static const grn_logger_info *grn_logger = &default_logger;

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

/**** grn_ctx_qe ****/

typedef struct _grn_ctx_qe grn_ctx_qe;
typedef struct _grn_ctx_qe_list grn_ctx_qe_list;
typedef struct _grn_ctx_qe_source grn_ctx_qe_source;

struct _grn_ctx_qe_list {
  grn_ctx_qe *qe;
  grn_ctx_qe_list *next;
};

struct _grn_ctx_qe_source {
  grn_proc_func *func;
  uint32_t nargs;
  grn_ctx_qe *args[16];
};

struct _grn_ctx_qe {
  grn_obj *value;
  grn_ctx_qe_list *deps;
  grn_ctx_qe *dest;
  grn_ctx_qe_source *source;
  grn_id id;
};

static void
grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)
{
  if (obj && (!GRN_DB_OBJP(obj) || (((grn_db_obj *)obj)->id & GRN_OBJ_TMP_OBJECT))) {
    grn_obj_close(ctx, obj);
  }
}

static grn_rc
disp(grn_ctx *ctx, grn_obj *qe, grn_proc_data *user_data)
{
  grn_obj *table = grn_ctx_pop(ctx);
  grn_obj *str = grn_ctx_pop(ctx);
  if (table && str && str->header.type == GRN_BULK) {
    grn_obj_format format;
    grn_obj *cols[256];
    char *p = GRN_BULK_HEAD(str), *tokbuf[256];
    int i, n = grn_str_tok(p, GRN_BULK_VSIZE(str), ' ', tokbuf, 256, NULL);
    for (i = 0; i < n; i++) {
      cols[i] = grn_obj_column(ctx, table, p, tokbuf[i] - p);
      p = tokbuf[i] + 1;
    }
    format.ncolumns = n;
    format.columns = cols;
    grn_text_otoj(ctx, ctx->impl->outbuf, table, &format);
    //  ctx->impl->output(ctx, GRN_QL_MORE, ctx->impl->data.ptr);
    for (i = 0; i < n; i++) {
      grn_obj_unlink(ctx, cols[i]);
    }
    return GRN_SUCCESS;
  }
  return GRN_INVALID_ARGUMENT;
}

static grn_rc
search(grn_ctx *ctx, grn_obj *qe, grn_proc_data *user_data)
{
  grn_obj *op = grn_ctx_pop(ctx);
  grn_obj *index = grn_ctx_pop(ctx);
  grn_obj *query = grn_ctx_pop(ctx);
  grn_obj *res = grn_ctx_pop(ctx);
  if (op) {
    /* todo */
  }
  if (index->header.type == GRN_BULK) {
    index = grn_ctx_get(ctx, GRN_BULK_HEAD(index), GRN_BULK_VSIZE(index));
  }
  if (!query) { return GRN_INVALID_ARGUMENT; }
  if (!index || index->header.type != GRN_COLUMN_INDEX) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!res) {
    grn_obj *table;
    if (!(table = grn_ctx_at(ctx, ((grn_db_obj *)index)->range))) {
      return GRN_INVALID_ARGUMENT;
    }
    res = grn_table_create(ctx, NULL, 0, NULL,
                           GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, 0);
  }
  grn_ctx_push(ctx, res);
  return grn_obj_search(ctx, index, query, res, GRN_SEL_OR, NULL);
}

static grn_rc
scan(grn_ctx *ctx, grn_obj *qe, grn_proc_data *user_data)
{
  grn_id id;
  grn_table_cursor *c;
  grn_obj *table, *val1, *op, *val2, *res, *opt;
  opt = grn_ctx_pop(ctx);
  table = grn_ctx_pop(ctx);
  val1 = grn_ctx_pop(ctx);
  op = grn_ctx_pop(ctx);
  val2 = grn_ctx_pop(ctx);
  res = grn_ctx_pop(ctx);
  if (!res) {
    res = grn_table_create(ctx, NULL, 0, NULL,
                           GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, 0);
  }
  if ((c = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0))) {
    int o = 0, n =0, l = -1;
    grn_obj buf1, buf2, *v1, *v2;
    GRN_TEXT_INIT(&buf1, 0);
    GRN_TEXT_INIT(&buf2, 0);
    // todo : support op
    while ((id = grn_table_cursor_next(ctx, c))) {
      if (val1->header.type == GRN_BULK) {
        v1 = val1;
      } else {
        GRN_BULK_REWIND(&buf1);
        v1 = grn_obj_get_value(ctx, val1, id, &buf1);
      }
      if (val2->header.type == GRN_BULK) {
        v2 = val2;
      } else {
        GRN_BULK_REWIND(&buf2);
        v2 = grn_obj_get_value(ctx, val2, id, &buf2);
      }
      if ((GRN_BULK_VSIZE(v1) == GRN_BULK_VSIZE(v2)) &&
          !memcmp(GRN_BULK_HEAD(v1), GRN_BULK_HEAD(v2), GRN_BULK_VSIZE(v1))) {
        if (n++ >= o) {
          /* todo : use GRN_SET_INT_ADD if !n_entries */
          grn_rset_recinfo *ri;
          grn_table_add_v(ctx, res, &id, sizeof(grn_id), (void **)&ri, NULL);
          {
            int score = 1;
            grn_table_add_subrec(res, ri, score, NULL, 0);
          }
          if (!--l) { break; }
        }
      }
    }
    grn_table_cursor_close(ctx, c);
  }
  grn_ctx_push(ctx, res);
  return ctx->rc;
}

grn_rc
grn_db_init_builtin_procs(grn_ctx *ctx)
{
  grn_obj res;
  GRN_INT32_INIT(&res, 0);
  grn_proc_create(ctx, "<proc:disp>", 11, NULL, disp, NULL, NULL, 2, 0, NULL);
  grn_proc_create(ctx, "<proc:search>", 13, NULL, search, NULL, NULL, 4, 1, &res);
  grn_proc_create(ctx, "<proc:scan>", 11, NULL, scan, NULL, NULL, 6, 1, &res);
  return ctx->rc;
}

static grn_rc
grn_ctx_qe_init(grn_ctx *ctx)
{
  if (!ctx->impl) {
    grn_ctx_impl_init(ctx);
  }
  if (ctx->impl && !ctx->impl->qe) {
    ctx->impl->qe = grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE,
                                    sizeof(grn_ctx_qe),
                                    GRN_OBJ_KEY_VAR_SIZE|GRN_HASH_TINY);
    {
      grn_proc *init = (grn_proc *)grn_ctx_get(ctx, "<proc:init>", 11);
      if (init) {
        init->funcs[PROC_INIT](ctx, (grn_obj *)ctx->impl->qe, NULL);
      }
    }
  }
  return ctx->rc;
}

void
grn_ctx_qe_fin(grn_ctx *ctx)
{
  if (ctx->impl->qe) {
    grn_ctx_qe *qe;
    grn_ctx_qe_list *l, *l_;
    GRN_HASH_EACH(ctx->impl->qe, id, NULL, NULL, &qe, {
      if (qe->source) { GRN_FREE(qe->source); }
      grn_obj_unlink(ctx, qe->value);
      for (l = qe->deps; l; l = l_) {
        l_ = l->next;
        GRN_FREE(l);
      }
    });
    grn_hash_close(ctx, ctx->impl->qe);
    ctx->impl->qe = NULL;
  }
}

static void
grn_ctx_qe_deps_push(grn_ctx *ctx, grn_ctx_qe *s, grn_ctx_qe *d)
{
  grn_ctx_qe_list *l;
  if ((l = GRN_MALLOCN(grn_ctx_qe_list, 1))) {
    l->qe = d;
    l->next = s->deps;
    s->deps = l;
  }
}

static void
grn_ctx_qe_deps_delete(grn_ctx *ctx, grn_ctx_qe *s, grn_ctx_qe *d)
{
  grn_ctx_qe_list **l, *l_;
  for (l = &s->deps; *l; l = &(*l)->next) {
    if ((*l)->qe == d) {
      l_ = *l;
      *l = l_->next;
      GRN_FREE(l_);
      break;
    }
  }
}

static grn_ctx_qe *
qe_get(grn_ctx *ctx, const char *key, int key_size)
{
  grn_ctx_qe *qe = NULL;
  grn_hash_get(ctx, ctx->impl->qe, key, key_size, (void **)&qe);
  return qe;
}

static grn_ctx_qe *
qe_add(grn_ctx *ctx, const char *key, int key_size)
{
  grn_id id;
  grn_ctx_qe *qe;
  int added;
  if (!(id = grn_hash_add(ctx, ctx->impl->qe, key, key_size, (void **)&qe, &added))) {
    return NULL;
  }
  if (added) { qe->id = id; }
  return qe;
}

static grn_ctx_qe *
grn_ctx_qe_parse(grn_ctx *ctx, const char *key, int key_size)
{
  grn_obj *str;
  grn_ctx_qe *s, *d;
  if (!key_size || *key != '/') { return NULL; }
  if (!(s = qe_get(ctx, key + 1, key_size - 1))) { return NULL; }
  if (!(d = qe_add(ctx, key, key_size))) { return NULL; }
  s->dest = d;
  str = s->value;
  if (str && str->header.type == GRN_BULK) {
    char *head = GRN_BULK_HEAD(str);
    char *tokbuf[17 + 1];
    int i, n = grn_str_tok(head, GRN_BULK_VSIZE(str), ' ', tokbuf, 16, NULL);
    if (n <= 17) {
      grn_obj *obj = grn_ctx_get(ctx, head, tokbuf[0] - head);
      if (obj && obj->header.type == GRN_PROC) {
        if ((d->source = GRN_MALLOCN(grn_ctx_qe_source, 1))) {
          d->source->func = ((grn_proc *)obj)->funcs[PROC_INIT];
          d->source->nargs = n - 1;
          for (i = 0; i < n - 1; i++) {
            const char *key = tokbuf[i] + 1;
            if ((d->source->args[i] = qe_add(ctx, key, tokbuf[i + 1] - key))) {
              grn_ctx_qe_deps_push(ctx, d->source->args[i], d);
            }
          }
        }
      }
    }
  }
  return d;
}

static grn_obj *grn_ctx_qe_get_(grn_ctx *ctx, grn_ctx_qe *qe);

static grn_obj *
qe_exec(grn_ctx *ctx, grn_ctx_qe_source *source)
{
  uint32_t i;
  grn_proc_ctx pctx;
  pctx.user_data.ptr = NULL;
  pctx.data[0].ptr = NULL;
  if (source) {
    for (i = 0; i < source->nargs; i++) {
      pctx.data[i + 1].ptr = grn_ctx_qe_get_(ctx, source->args[i]);
    }
    source->func(ctx, (grn_obj *)ctx->impl->qe, &pctx.user_data);
  }
  return (grn_obj *)pctx.data[0].ptr;
}

static grn_obj *
grn_ctx_qe_get_(grn_ctx *ctx, grn_ctx_qe *qe)
{
  if (!qe->value) {
    uint32_t key_size;
    const char *key = _grn_hash_key(ctx, ctx->impl->qe, qe->id, &key_size);
    if (!qe->source) { grn_ctx_qe_parse(ctx, key, key_size); }
    if (qe->source) {
      qe->value = qe_exec(ctx, qe->source);
    } else {
      qe->value = grn_ctx_get(ctx, key, key_size);
    }
  }
  return qe->value;
}

static void
grn_ctx_qe_set_(grn_ctx *ctx, grn_ctx_qe *qe, grn_obj *value)
{
  grn_ctx_qe_list *l;
  grn_obj_unlink(ctx, qe->value);
  qe->value = value;
  if (qe->dest) {
    grn_ctx_qe_set_(ctx, qe->dest, NULL);
    if (qe->dest->source) {
      uint32_t i;
      for (i = 0; i < qe->dest->source->nargs; i++) {
        grn_ctx_qe_deps_delete(ctx, qe->dest->source->args[i], qe->dest);
      }
      GRN_FREE(qe->dest->source);
    }
    qe->dest->source = NULL;
  }
  for (l = qe->deps; l; l = l->next) { grn_ctx_qe_set_(ctx, l->qe, NULL); }
}

grn_rc
grn_ctx_qe_set(grn_ctx *ctx, const char *key, int key_size, grn_obj *value)
{
  grn_ctx_qe *qe;
  if (grn_ctx_qe_init(ctx)) { return ctx->rc; }
  if (!(qe = qe_add(ctx, key, key_size))) { return ctx->rc; }
  grn_ctx_qe_set_(ctx, qe, value);
  return GRN_SUCCESS;
}

grn_obj *
grn_ctx_qe_get(grn_ctx *ctx, const char *key, int key_size)
{
  grn_ctx_qe *qe;
  if (grn_ctx_qe_init(ctx)) { return NULL; }
  if (!(qe = qe_get(ctx, key, key_size))) {
    if (!(qe = grn_ctx_qe_parse(ctx, key, key_size))) {
      return grn_ctx_get(ctx, key, key_size);
    }
  }
  return grn_ctx_qe_get_(ctx, qe);
}

static const char *
get_token(grn_ctx *ctx, grn_obj *buf, const char *p, const char *e, char d)
{
  while (p < e) {
    if (*p == d) {
      p++; break;
    } else if (*p == '+') {
      GRN_TEXT_PUTC(ctx, buf, ' ');
      p++;
    } else if (*p == '%' && p + 3 <= e) {
      const char *r;
      unsigned int c = grn_htoui(p + 1, p + 3, &r);
      if (p + 3 == r) {
        GRN_TEXT_PUTC(ctx, buf, c);
      } else {
        GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid % sequence (%c%c)", p + 1, p + 2);
      }
      p += 3;
    } else {
      GRN_TEXT_PUTC(ctx, buf, *p);
      p++;
    }
  }
  return p;
}

grn_obj *
grn_ctx_qe_exec(grn_ctx *ctx, const char *str, uint32_t str_size)
{
  const char *p, *e;
  grn_obj key, *expr, *val = NULL;
  GRN_TEXT_INIT(&key, 0);
  p = str;
  e = p + str_size;
  p = get_token(ctx, &key, p, e, '?');
  if ((expr = grn_ctx_get(ctx, GRN_TEXT_VALUE(&key), GRN_TEXT_LEN(&key)))) {
    while (p < e) {
      GRN_BULK_REWIND(&key);
      p = get_token(ctx, &key, p, e, '=');
      if (!(val = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(&key), GRN_TEXT_LEN(&key)))) {
        val = &key;
      }
      p = get_token(ctx, val, p, e, '&');
    }
    val = grn_expr_exec(ctx, expr);
  }
  GRN_OBJ_FIN(ctx, &key);
  return val;
}
