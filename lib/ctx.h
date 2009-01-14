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
#ifndef GRN_CTX_H
#define GRN_CTX_H

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif /* HAVE_EXECINFO_H */

#ifndef GRN_IO_H
#include "io.h"
#endif /* GRN_IO_H */

#ifndef GRN_COM_H
#include "com.h"
#endif /* GRN_COM_H */

#ifdef __cplusplus
extern "C" {
#endif

/**** api in/out ****/

#define GRN_API_ENTER \
{\
  (ctx)->errlvl = GRN_OK;\
  (ctx)->rc = grn_success;\
  if ((ctx)->seqno & 1) {\
    (ctx)->subno++;\
  } else {\
    (ctx)->seqno++;\
  }\
}

/* CAUTION!! : pass only variables or constants as r */
#define GRN_API_RETURN(r) \
{\
  if (ctx->subno) {\
    ctx->subno--;\
  } else {\
    ctx->seqno++;\
  }\
  return r;\
}

/**** error handling ****/

#define GRN_OP_T0LVL 0
#define GRN_OP_ERR0  1

#define  GRN_EMERG  grn_log_emerg
#define  GRN_ALERT  grn_log_alert
#define  GRN_CRIT   grn_log_crit
#define  GRN_ERROR  grn_log_error
#define  GRN_WARN   grn_log_warning
#define  GRN_OK     grn_log_notice

#define ERRCLR(ctx) do {\
  if (ctx) {\
    ((grn_ctx *)ctx)->errlvl = GRN_OK;\
    ((grn_ctx *)ctx)->rc = grn_success;\
  }\
  grn_gctx.errlvl = GRN_OK;\
  grn_gctx.rc = grn_success;\
} while (0)

#ifdef HAVE_EXECINFO_H
#define BACKTRACE(ctx) ((ctx)->ntrace = (unsigned char)backtrace((ctx)->trace, 16))
#else /* HAVE_EXECINFO_H */
#define BACKTRACE(ctx)
#endif /* HAVE_EXECINFO_H */

void grn_ctx_impl_err(grn_ctx *ctx);

#define ERRSET(ctx,lvl,r,...) do {\
  grn_ctx *ctx_ = (grn_ctx *)ctx;\
  ctx_->errlvl = (lvl);\
  ctx_->rc = (r);\
  ctx_->errfile = __FILE__;\
  ctx_->errline = __LINE__;\
  ctx_->errfunc = __FUNCTION__;\
  grn_ctx_impl_err(ctx);\
  GRN_LOG(lvl, __VA_ARGS__);\
  grn_ctx_log((grn_ctx *)ctx, __VA_ARGS__);\
  BACKTRACE(ctx);\
} while (0)

#define ERRP(ctx,lvl) (((ctx) && ((grn_ctx *)(ctx))->errlvl <= (lvl)) || (grn_gctx.errlvl <= (lvl)))

#define QLERR(...) do {\
  ERRSET(ctx, GRN_WARN, grn_invalid_argument, __VA_ARGS__);\
  return F;\
} while (0)

#define QLASSERT(expr) do {\
  if (!(expr)) { QLERR("syntax error"); }\
} while (0)

#define ERR(rc,...) ERRSET(ctx, GRN_ERROR, (rc),  __VA_ARGS__)
#define MERR(...) ERRSET(ctx, GRN_ALERT, grn_memory_exhausted,  __VA_ARGS__)
#define SERR(str) ERR(grn_other_error, "syscall error '%s' (%s)", str, strerror(errno))

#define GERR(rc,...) ERRSET(&grn_gctx, GRN_ERROR, (rc),  __VA_ARGS__)
#define GMERR(...) ERRSET(&grn_gctx, GRN_ALERT, grn_memory_exhausted,  __VA_ARGS__)
#define GSERR(str) GERR(grn_other_error, "syscall error '%s' (%s)", str, strerror(errno))

#define GRN_MALLOC(s) grn_malloc(ctx,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_CALLOC(s) grn_calloc(ctx,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_REALLOC(p,s) grn_realloc(ctx,p,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_STRDUP(s) grn_strdup(ctx,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_GMALLOC(s) grn_malloc(&grn_gctx,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_GCALLOC(s) grn_calloc(&grn_gctx,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_GREALLOC(p,s) grn_realloc(&grn_gctx,p,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_GSTRDUP(s) grn_strdup(&grn_gctx,s,__FILE__,__LINE__,__FUNCTION__)
#define GRN_FREE(p) grn_free(ctx,p,__FILE__,__LINE__)
#define GRN_MALLOCN(t,n) ((t *)(GRN_MALLOC(sizeof(t) * (n))))
#define GRN_GFREE(p) grn_free(&grn_gctx,p,__FILE__,__LINE__)
#define GRN_GMALLOCN(t,n) ((t *)(GRN_GMALLOC(sizeof(t) * (n))))

#ifdef DEBUG
#define GRN_ASSERT(s) grn_assert(ctx,(s),__FILE__,__LINE__,__FUNCTION__)
#else
#define GRN_ASSERT(s)
#endif

#define GRN_CTX_ALLOC(ctx,s,f) grn_ctx_alloc(ctx,s,f,__FILE__,__LINE__,__FUNCTION__)
#define GRN_CTX_FREE(ctx,p) grn_ctx_free(ctx,p,__FILE__,__LINE__,__FUNCTION__)
#define GRN_CTX_ALLOC_L(ctx,s,f) grn_ctx_alloc_lifo(ctx,s,f,__FILE__,__LINE__,__FUNCTION__)
#define GRN_CTX_FREE_L(ctx,p) grn_ctx_free_lifo(ctx,p,__FILE__,__LINE__,__FUNCTION__)

void *grn_ctx_alloc(grn_ctx *ctx, size_t size, int flags,
                     const char* file, int line, const char *func);
void grn_ctx_free(grn_ctx *ctx, void *ptr,
                  const char* file, int line, const char *func);
void *grn_ctx_alloc_lifo(grn_ctx *ctx, size_t size, int flags,
                         const char* file, int line, const char *func);
void grn_ctx_free_lifo(grn_ctx *ctx, void *ptr,
                       const char* file, int line, const char *func);

#ifdef USE_DYNAMIC_MALLOC_CHANGE
typedef void *(*grn_malloc_func) (grn_ctx *ctx, size_t size,
                                  const char *file, int line, const char *func);
typedef void *(*grn_calloc_func) (grn_ctx *ctx, size_t size,
                                  const char *file, int line, const char *func);
typedef void *(*grn_realloc_func) (grn_ctx *ctx, void *ptr, size_t size,
                                   const char *file, int line, const char *func);
typedef char *(*grn_strdup_func) (grn_ctx *ctx, const char *string,
                                  const char *file, int line, const char *func);
grn_malloc_func grn_ctx_get_malloc(grn_ctx *ctx);
void grn_ctx_set_malloc(grn_ctx *ctx, grn_malloc_func malloc_func);
grn_calloc_func grn_ctx_get_calloc(grn_ctx *ctx);
void grn_ctx_set_calloc(grn_ctx *ctx, grn_calloc_func calloc_func);
grn_realloc_func grn_ctx_get_realloc(grn_ctx *ctx);
void grn_ctx_set_realloc(grn_ctx *ctx, grn_realloc_func realloc_func);
grn_strdup_func grn_ctx_get_strdup(grn_ctx *ctx);
void grn_ctx_set_strdup(grn_ctx *ctx, grn_strdup_func strdup_func);

void *grn_malloc(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_calloc(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_realloc(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func);
char *grn_strdup(grn_ctx *ctx, const char *s, const char* file, int line, const char *func);
#else
#  define grn_malloc grn_malloc_default
#  define grn_calloc grn_calloc_default
#  define grn_realloc grn_realloc_default
#  define grn_strdup grn_strdup_default
#endif

void *grn_malloc_default(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_calloc_default(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_realloc_default(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func);
char *grn_strdup_default(grn_ctx *ctx, const char *s, const char* file, int line, const char *func);

#ifdef USE_FAIL_MALLOC
void *grn_malloc_fail(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_calloc_fail(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_realloc_fail(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func);
char *grn_strdup_fail(grn_ctx *ctx, const char *s, const char* file, int line, const char *func);
#endif

void grn_free(grn_ctx *ctx, void *ptr, const char* file, int line);

void grn_assert(grn_ctx *ctx, int cond, const char* file, int line, const char* func);

void grn_index_expire(void);

/**** grn_ctx ****/

extern grn_ctx grn_gctx;
extern int grn_pagesize;
extern grn_mutex grn_glock;
extern uint32_t grn_gtick;

typedef struct {
  int32_t tv_sec;
  int32_t tv_usec;
} grn_timeval;

#ifndef GRN_TIMEVAL_STR_SIZE
#define GRN_TIMEVAL_STR_SIZE 0x100
#endif /* GRN_TIMEVAL_STR_SIZE */
#ifndef GRN_TIMEVAL_STR_FORMAT
#define GRN_TIMEVAL_STR_FORMAT "%04d-%02d-%02d %02d:%02d:%02d.%06d"
#endif /* GRN_TIMEVAL_STR_FORMAT */

grn_rc grn_timeval_now(grn_timeval *tv);
grn_rc grn_timeval2str(grn_timeval *tv, char *buf);
grn_rc grn_str2timeval(const char *str, uint32_t str_len, grn_timeval *tv);

void grn_ctx_log(grn_ctx *ctx, char *fmt, ...);

/**** receive handler ****/

void grn_ql_recv_handler_set(grn_ctx *c, void (*func)(grn_ctx *, int, void *),
                              void *func_arg);

void grn_ctx_concat_func(grn_ctx *ctx, int flags, void *dummy);
void grn_ctx_stream_out_func(grn_ctx *c, int flags, void *stream);

#ifdef __cplusplus
}
#endif

#endif /* GRN_CTX_H */
