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

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#define GRN_BREAK_POINT raise(SIGTRAP)
#endif /* HAVE_SIGNAL_H */

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
  if ((ctx)->seqno & 1) {\
    (ctx)->subno++;\
  } else {\
    (ctx)->errlvl = GRN_OK;\
    (ctx)->rc = GRN_SUCCESS;\
    (ctx)->seqno++;\
  }\
  GRN_TEST_YIELD();\
}

/* CAUTION!! : pass only variables or constants as r */
#define GRN_API_RETURN(r) \
{\
  if (ctx->subno) {\
    ctx->subno--;\
  } else {\
    ctx->seqno++;\
  }\
  GRN_TEST_YIELD();\
  return r;\
}

/**** error handling ****/

#define GRN_OP_T0LVL 0
#define GRN_OP_ERR0  1

#define  GRN_EMERG  GRN_LOG_EMERG
#define  GRN_ALERT  GRN_LOG_ALERT
#define  GRN_CRIT   GRN_LOG_CRIT
#define  GRN_ERROR  GRN_LOG_ERROR
#define  GRN_WARN   GRN_LOG_WARNING
#define  GRN_OK     GRN_LOG_NOTICE

#define ERRCLR(ctx) do {\
  if (ctx) {\
    ((grn_ctx *)ctx)->errlvl = GRN_OK;\
    ((grn_ctx *)ctx)->rc = GRN_SUCCESS;\
  }\
  errno = 0;\
  grn_gctx.errlvl = GRN_OK;\
  grn_gctx.rc = GRN_SUCCESS;\
} while (0)

#ifdef HAVE_EXECINFO_H
#define BACKTRACE(ctx) ((ctx)->ntrace = (unsigned char)backtrace((ctx)->trace, 16))
#else /* HAVE_EXECINFO_H */
#define BACKTRACE(ctx)
#endif /* HAVE_EXECINFO_H */

void grn_ctx_impl_err(grn_ctx *ctx);

#ifdef HAVE_EXECINFO_H
#define LOGTRACE(ctx,lvl) {\
  int i;\
  char **p;\
  grn_obj buf;\
  BACKTRACE(ctx);\
  p = backtrace_symbols((ctx)->trace, (ctx)->ntrace);\
  GRN_TEXT_INIT(&buf, 0);\
  for (i = 0; i < (ctx)->ntrace; i++) {\
    if (i) GRN_TEXT_PUTS((ctx), &buf, " <= ");\
    GRN_TEXT_PUTS((ctx), &buf, p[i]);\
  }\
  GRN_TEXT_PUTC((ctx), &buf, '\0');\
  free(p);\
  GRN_LOG((ctx), lvl, "%s", GRN_BULK_HEAD(&buf));\
  grn_obj_close((ctx), &buf);\
}
#else  /* HAVE_EXECINFO_H */
#define LOGTRACE(ctx,msg)
#endif /* HAVE_EXECINFO_H */

#define ERRSET(ctx,lvl,r,...) do {\
  grn_ctx *ctx_ = (grn_ctx *)ctx;\
  ctx_->errlvl = (lvl);\
  ctx_->rc = (r);\
  ctx_->errfile = __FILE__;\
  ctx_->errline = __LINE__;\
  ctx_->errfunc = __FUNCTION__;\
  grn_ctx_impl_err(ctx);\
  grn_ctx_log(ctx, __VA_ARGS__);\
  GRN_LOG(ctx, lvl, __VA_ARGS__);\
  BACKTRACE(ctx);\
  if (lvl <= GRN_LOG_ERROR) { LOGTRACE(ctx, lvl); }\
} while (0)

#define ERRP(ctx,lvl) (((ctx) && ((grn_ctx *)(ctx))->errlvl <= (lvl)) || (grn_gctx.errlvl <= (lvl)))

#define QLERR(...) do {\
  ERRSET(ctx, GRN_WARN, GRN_INVALID_ARGUMENT, __VA_ARGS__);\
  return F;\
} while (0)

#define QLASSERT(expr) do {\
  if (!(expr)) { QLERR("syntax error"); }\
} while (0)

#define ERR(rc,...) ERRSET(ctx, GRN_ERROR, (rc),  __VA_ARGS__)
#define WARN(rc,...) ERRSET(ctx, GRN_WARN, (rc),  __VA_ARGS__)
#define MERR(...) ERRSET(ctx, GRN_ALERT, GRN_NO_MEMORY_AVAILABLE,  __VA_ARGS__)
#define ALERT(...) ERRSET(ctx, GRN_ALERT, GRN_SUCCESS,  __VA_ARGS__)

#ifdef WIN32
#define SERR(str) {\
  grn_rc rc;\
  const char *m;\
  int e = WSAGetLastError();\
  switch (e) {\
  case WSANOTINITIALISED :\
    rc = GRN_SOCKET_NOT_INITIALIZED;\
    m = "please call grn_com_init first";\
    break;\
  case WSAEFAULT :\
    rc = GRN_BAD_ADDRESS;\
    m = "bad address";\
    break;\
  case WSAEINVAL :\
    rc = GRN_INVALID_ARGUMENT;\
    m = "invalid argument";\
    break;\
  case WSAEMFILE :\
    rc = GRN_TOO_MANY_OPEN_FILES;\
    m = "too many sockets";\
    break;\
  case WSAEWOULDBLOCK :\
    rc = GRN_OPERATION_WOULD_BLOCK;\
    m = "operation would block";\
    break;\
  case WSAENOTSOCK :\
    rc = GRN_NOT_SOCKET;\
    m = "given fd is not socket fd";\
    break;\
  case WSAEOPNOTSUPP :\
    rc = GRN_OPERATION_NOT_SUPPORTED;\
    m = "operation is not supported";\
    break;\
  case WSAEADDRINUSE :\
    rc = GRN_ADDRESS_IS_IN_USE;\
    m = "address is already in use";\
    break;\
  case WSAEADDRNOTAVAIL :\
    rc = GRN_ADDRESS_IS_NOT_AVAILABLE;\
    m = "address is not available";\
    break;\
  case WSAENETDOWN :\
    rc = GRN_NETWORK_IS_DOWN;\
    m = "network is down";\
    break;\
  case WSAENOBUFS :\
    rc = GRN_NO_BUFFER;\
    m = "no buffer";\
    break;\
  case WSAEISCONN :\
    rc = GRN_SOCKET_IS_ALREADY_CONNECTED;\
    m = "socket is already connected";\
    break;\
  case WSAENOTCONN :\
    rc = GRN_SOCKET_IS_NOT_CONNECTED;\
    m = "socket is not connected";\
    break;\
  case WSAESHUTDOWN :\
    rc = GRN_SOCKET_IS_ALREADY_SHUTDOWNED;\
    m = "socket is already shutdowned";\
    break;\
  case WSAETIMEDOUT :\
    rc = GRN_OPERATION_TIMEOUT;\
    m = "connection time out";\
    break;\
  case WSAECONNREFUSED :\
    rc = GRN_CONNECTION_REFUSED;\
    m = "connection refused";\
    break;\
  case WSAEINTR :\
    rc = GRN_INTERRUPTED_FUNCTION_CALL;\
    m = "interrupted function call";\
    break;\
  default:\
    rc = GRN_UNKNOWN_ERROR;\
    m = "unknown error";\
    break;\
  }\
  ERR(rc, "syscall error '%s' (%s)", str, m);\
}
#else /* WIN32 */
#define SERR(str) {\
  grn_rc rc;\
  switch (errno) {\
  case ELOOP : rc = GRN_TOO_MANY_SYMBOLIC_LINKS; break;\
  case ENAMETOOLONG : rc = GRN_FILENAME_TOO_LONG; break;\
  case ENOENT : rc = GRN_NO_SUCH_FILE_OR_DIRECTORY; break;\
  case ENOMEM : rc = GRN_NO_MEMORY_AVAILABLE; break;\
  case ENOTDIR : rc = GRN_NOT_A_DIRECTORY; break;\
  case EPERM : rc = GRN_OPERATION_NOT_PERMITTED; break;\
  case ESRCH : rc = GRN_NO_SUCH_PROCESS; break;\
  case EINTR : rc = GRN_INTERRUPTED_FUNCTION_CALL; break;\
  case EIO : rc = GRN_INPUT_OUTPUT_ERROR; break;\
  case ENXIO : rc = GRN_NO_SUCH_DEVICE_OR_ADDRESS; break;\
  case E2BIG : rc = GRN_ARG_LIST_TOO_LONG; break;\
  case ENOEXEC : rc = GRN_EXEC_FORMAT_ERROR; break;\
  case EBADF : rc = GRN_BAD_FILE_DESCRIPTOR; break;\
  case ECHILD : rc = GRN_NO_CHILD_PROCESSES; break;\
  case EACCES : rc = GRN_PERMISSION_DENIED; break;\
  case EFAULT : rc = GRN_BAD_ADDRESS; break;\
  case EBUSY : rc = GRN_RESOURCE_BUSY; break;\
  case EEXIST : rc = GRN_FILE_EXISTS; break;\
  case ENODEV : rc = GRN_NO_SUCH_DEVICE; break;\
  case EISDIR : rc = GRN_IS_A_DIRECTORY; break;\
  case EINVAL : rc = GRN_INVALID_ARGUMENT; break;\
  case EMFILE : rc = GRN_TOO_MANY_OPEN_FILES; break;\
  case EFBIG : rc = GRN_FILE_TOO_LARGE; break;\
  case ENOSPC : rc = GRN_NO_SPACE_LEFT_ON_DEVICE; break;\
  case EROFS : rc = GRN_READ_ONLY_FILE_SYSTEM; break;\
  case EMLINK : rc = GRN_TOO_MANY_LINKS; break;\
  case EPIPE : rc = GRN_BROKEN_PIPE; break;\
  case EDOM : rc = GRN_DOMAIN_ERROR; break;\
  case ERANGE : rc = GRN_RANGE_ERROR; break;\
  case ENOTSOCK : rc = GRN_NOT_SOCKET; break;\
  case EADDRINUSE : rc = GRN_ADDRESS_IS_IN_USE; break;\
  case ENETDOWN : rc = GRN_NETWORK_IS_DOWN; break;\
  case ENOBUFS : rc = GRN_NO_BUFFER; break;\
  case EISCONN : rc = GRN_SOCKET_IS_ALREADY_CONNECTED; break;\
  case ENOTCONN : rc = GRN_SOCKET_IS_NOT_CONNECTED; break;\
    /*\
  case ESOCKTNOSUPPORT :\
  case EOPNOTSUPP :\
  case EPFNOSUPPORT :\
    */\
  case EPROTONOSUPPORT : rc = GRN_OPERATION_NOT_SUPPORTED; break;\
  case ESHUTDOWN : rc = GRN_SOCKET_IS_ALREADY_SHUTDOWNED; break;\
  case ETIMEDOUT : rc = GRN_OPERATION_TIMEOUT; break;\
  case ECONNREFUSED: rc = GRN_CONNECTION_REFUSED; break;\
  case EAGAIN: rc = GRN_OPERATION_WOULD_BLOCK; break;\
  default : rc = GRN_UNKNOWN_ERROR; break;\
  }\
  ERR(rc, "syscall error '%s' (%s)", str, strerror(errno));\
}
#endif /* WIN32 */

#define GERR(rc,...) ERRSET(&grn_gctx, GRN_ERROR, (rc),  __VA_ARGS__)
#define GMERR(...) ERRSET(&grn_gctx, GRN_ALERT, GRN_NO_MEMORY_AVAILABLE,  __VA_ARGS__)

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
int grn_fail_malloc_check(size_t size, const char *file, int line, const char *func);
void *grn_malloc_fail(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_calloc_fail(grn_ctx *ctx, size_t size, const char* file, int line, const char *func);
void *grn_realloc_fail(grn_ctx *ctx, void *ptr, size_t size, const char* file, int line, const char *func);
char *grn_strdup_fail(grn_ctx *ctx, const char *s, const char* file, int line, const char *func);
#endif

void grn_free(grn_ctx *ctx, void *ptr, const char* file, int line);

void grn_assert(grn_ctx *ctx, int cond, const char* file, int line, const char* func);

/**** grn_ctx ****/

extern grn_ctx grn_gctx;
extern int grn_pagesize;
extern grn_mutex grn_glock;
extern uint32_t grn_gtick;
extern grn_obj *grn_true, *grn_false, *grn_null;

typedef struct {
  int32_t tv_sec;
  int32_t tv_usec;
} grn_timeval;

extern grn_timeval grn_starttime;

#ifndef GRN_TIMEVAL_STR_SIZE
#define GRN_TIMEVAL_STR_SIZE 0x100
#endif /* GRN_TIMEVAL_STR_SIZE */
#ifndef GRN_TIMEVAL_STR_FORMAT
#define GRN_TIMEVAL_STR_FORMAT "%04d-%02d-%02d %02d:%02d:%02d.%06d"
#endif /* GRN_TIMEVAL_STR_FORMAT */

grn_rc grn_timeval_now(grn_ctx *ctx, grn_timeval *tv);
grn_rc grn_timeval2str(grn_ctx *ctx, grn_timeval *tv, char *buf);
grn_rc grn_str2timeval(const char *str, uint32_t str_len, grn_timeval *tv);

void grn_ctx_log(grn_ctx *ctx, char *fmt, ...);
void grn_ctx_qe_fin(grn_ctx *ctx);
void grn_ctx_loader_clear(grn_ctx *ctx);

grn_rc grn_ctx_sendv(grn_ctx *ctx, int argc, char **argv, int flags);
void grn_ctx_set_next_expr(grn_ctx *ctx, grn_obj *expr);

int grn_alloc_count(void);

grn_content_type grn_get_ctype(grn_obj *var);

/**** receive handler ****/

void grn_ctx_recv_handler_set(grn_ctx *c, void (*func)(grn_ctx *, int, void *),
                              void *func_arg);

void grn_ctx_concat_func(grn_ctx *ctx, int flags, void *dummy);
void grn_ctx_stream_out_func(grn_ctx *c, int flags, void *stream);

grn_rc grn_db_init_builtin_procs(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* GRN_CTX_H */
