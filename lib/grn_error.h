/*
  Copyright (C) 2013-2016  Brazil
  Copyright (C) 2020-2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "grn.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_EMERG GRN_LOG_EMERG
#define GRN_ALERT GRN_LOG_ALERT
#define GRN_CRIT  GRN_LOG_CRIT
#define GRN_ERROR GRN_LOG_ERROR
#define GRN_WARN  GRN_LOG_WARNING
#define GRN_OK    GRN_LOG_NOTICE

#define ERRCLR(ctx)                                                            \
  do {                                                                         \
    if (ctx) {                                                                 \
      ((grn_ctx *)ctx)->errlvl = GRN_OK;                                       \
      if (((grn_ctx *)ctx)->rc != GRN_CANCEL) {                                \
        ((grn_ctx *)ctx)->rc = GRN_SUCCESS;                                    \
        ((grn_ctx *)ctx)->errbuf[0] = '\0';                                    \
      }                                                                        \
    }                                                                          \
    errno = 0;                                                                 \
    grn_gctx.errlvl = GRN_OK;                                                  \
    grn_gctx.rc = GRN_SUCCESS;                                                 \
  } while (0)

void
grn_error_init_from_env(void);
void
grn_error_cancel(grn_ctx *ctx);

static inline grn_rc
grn_ctx_rc_propagate(grn_ctx *ctx, grn_rc rc)
{
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return ctx->rc;
}

GRN_API bool
grn_ctx_impl_should_log(grn_ctx *ctx);
GRN_API void
grn_ctx_impl_set_current_error_message(grn_ctx *ctx);

GRN_API void
grn_ctx_log(grn_ctx *ctx, const char *fmt, ...) GRN_ATTRIBUTE_PRINTF(2);
GRN_API void
grn_ctx_logv(grn_ctx *ctx, const char *fmt, va_list ap);
GRN_API void
grn_ctx_log_back_trace(grn_ctx *ctx, grn_log_level level);

static inline void
grn_error_setv(grn_ctx *ctx,
               grn_log_level level,
               grn_rc rc,
               const char *file,
               uint32_t line,
               const char *function,
               const char *format,
               va_list args)
{
  ctx->errlvl = level;
  if (ctx->rc != GRN_CANCEL) {
    ctx->rc = rc;
  }
  ctx->errfile = file;
  ctx->errline = line;
  ctx->errfunc = function;
  va_list logger_putv_args;
  va_copy(logger_putv_args, args);
  grn_ctx_logv(ctx, format, args);
  if (grn_ctx_impl_should_log(ctx)) {
    grn_ctx_impl_set_current_error_message(ctx);
    if (grn_logger_pass(ctx, level)) {
      grn_logger_putv(ctx,
                      level,
                      file,
                      line,
                      function,
                      format,
                      logger_putv_args);
    }
    if (level <= GRN_LOG_ERROR) {
      grn_ctx_log_back_trace(ctx, level);
    }
  }
  va_end(logger_putv_args);
}

static inline void
grn_error_set(grn_ctx *ctx,
              grn_log_level level,
              grn_rc rc,
              const char *file,
              uint32_t line,
              const char *function,
              const char *format,
              ...) GRN_ATTRIBUTE_PRINTF(7);
static inline void
grn_error_set(grn_ctx *ctx,
              grn_log_level level,
              grn_rc rc,
              const char *file,
              uint32_t line,
              const char *function,
              const char *format,
              ...)
{
  va_list args;
  va_start(args, format);
  grn_error_setv(ctx, level, rc, file, line, function, format, args);
  va_end(args);
}

#define ERRSET(ctx, lvl, r, ...)                                               \
  grn_error_set((ctx),                                                         \
                (lvl),                                                         \
                (r),                                                           \
                __FILE__,                                                      \
                __LINE__,                                                      \
                __FUNCTION__,                                                  \
                __VA_ARGS__)

#define ERRP(ctx, lvl)                                                         \
  (((ctx) && ((grn_ctx *)(ctx))->errlvl <= (lvl)) || (grn_gctx.errlvl <= (lvl)))

#ifdef ERR
#  undef ERR
#endif /* ERR */
#define CRIT(rc, ...)     ERRSET(ctx, GRN_CRIT, (rc), __VA_ARGS__)
#define ERR(rc, ...)      ERRSET(ctx, GRN_ERROR, (rc), __VA_ARGS__)
#define WARN(rc, ...)     ERRSET(ctx, GRN_WARN, (rc), __VA_ARGS__)
#define MERR(...)         ERRSET(ctx, GRN_ALERT, GRN_NO_MEMORY_AVAILABLE, __VA_ARGS__)
#define ALERT(...)        ERRSET(ctx, GRN_ALERT, GRN_SUCCESS, __VA_ARGS__)

#define USER_MESSAGE_SIZE 1024

#ifdef WIN32

#  define SERR(...)                                                            \
    do {                                                                       \
      grn_rc rc;                                                               \
      int error_code;                                                          \
      const char *system_message;                                              \
      char user_message[USER_MESSAGE_SIZE];                                    \
      error_code = GetLastError();                                             \
      system_message = grn_error_get_current_system_message();                 \
      rc = grn_windows_error_code_to_rc(error_code);                           \
      grn_snprintf(user_message,                                               \
                   USER_MESSAGE_SIZE,                                          \
                   USER_MESSAGE_SIZE,                                          \
                   __VA_ARGS__);                                               \
      ERR(rc,                                                                  \
          "system error[%d]: %s: %s",                                          \
          error_code,                                                          \
          system_message,                                                      \
          user_message);                                                       \
    } while (0)

#  define SOERR(...)                                                           \
    do {                                                                       \
      grn_rc rc;                                                               \
      const char *m;                                                           \
      char user_message[USER_MESSAGE_SIZE];                                    \
      int e = WSAGetLastError();                                               \
      switch (e) {                                                             \
      case WSANOTINITIALISED:                                                  \
        rc = GRN_SOCKET_NOT_INITIALIZED;                                       \
        m = "please call grn_com_init first";                                  \
        break;                                                                 \
      case WSAEFAULT:                                                          \
        rc = GRN_BAD_ADDRESS;                                                  \
        m = "bad address";                                                     \
        break;                                                                 \
      case WSAEINVAL:                                                          \
        rc = GRN_INVALID_ARGUMENT;                                             \
        m = "invalid argument";                                                \
        break;                                                                 \
      case WSAEMFILE:                                                          \
        rc = GRN_TOO_MANY_OPEN_FILES;                                          \
        m = "too many sockets";                                                \
        break;                                                                 \
      case WSAEWOULDBLOCK:                                                     \
        rc = GRN_OPERATION_WOULD_BLOCK;                                        \
        m = "operation would block";                                           \
        break;                                                                 \
      case WSAENOTSOCK:                                                        \
        rc = GRN_NOT_SOCKET;                                                   \
        m = "given fd is not socket fd";                                       \
        break;                                                                 \
      case WSAEOPNOTSUPP:                                                      \
        rc = GRN_OPERATION_NOT_SUPPORTED;                                      \
        m = "operation is not supported";                                      \
        break;                                                                 \
      case WSAEADDRINUSE:                                                      \
        rc = GRN_ADDRESS_IS_IN_USE;                                            \
        m = "address is already in use";                                       \
        break;                                                                 \
      case WSAEADDRNOTAVAIL:                                                   \
        rc = GRN_ADDRESS_IS_NOT_AVAILABLE;                                     \
        m = "address is not available";                                        \
        break;                                                                 \
      case WSAENETDOWN:                                                        \
        rc = GRN_NETWORK_IS_DOWN;                                              \
        m = "network is down";                                                 \
        break;                                                                 \
      case WSAENOBUFS:                                                         \
        rc = GRN_NO_BUFFER;                                                    \
        m = "no buffer";                                                       \
        break;                                                                 \
      case WSAEISCONN:                                                         \
        rc = GRN_SOCKET_IS_ALREADY_CONNECTED;                                  \
        m = "socket is already connected";                                     \
        break;                                                                 \
      case WSAENOTCONN:                                                        \
        rc = GRN_SOCKET_IS_NOT_CONNECTED;                                      \
        m = "socket is not connected";                                         \
        break;                                                                 \
      case WSAESHUTDOWN:                                                       \
        rc = GRN_SOCKET_IS_ALREADY_SHUTDOWNED;                                 \
        m = "socket is already shutdowned";                                    \
        break;                                                                 \
      case WSAETIMEDOUT:                                                       \
        rc = GRN_OPERATION_TIMEOUT;                                            \
        m = "connection time out";                                             \
        break;                                                                 \
      case WSAECONNREFUSED:                                                    \
        rc = GRN_CONNECTION_REFUSED;                                           \
        m = "connection refused";                                              \
        break;                                                                 \
      case WSAEINTR:                                                           \
        rc = GRN_INTERRUPTED_FUNCTION_CALL;                                    \
        m = "interrupted function call";                                       \
        break;                                                                 \
      case WSAECONNRESET:                                                      \
        rc = GRN_CONNECTION_RESET;                                             \
        m = "connection reset by peer";                                        \
        break;                                                                 \
      default:                                                                 \
        rc = GRN_UNKNOWN_ERROR;                                                \
        m = "unknown error";                                                   \
        break;                                                                 \
      }                                                                        \
      grn_snprintf(user_message,                                               \
                   USER_MESSAGE_SIZE,                                          \
                   USER_MESSAGE_SIZE,                                          \
                   __VA_ARGS__);                                               \
      ERR(rc, "socket error[%d]: %s: %s", e, m, user_message);                 \
    } while (0)

#  define ERRNO_ERR(...)                                                       \
    do {                                                                       \
      grn_rc rc;                                                               \
      int errno_keep = errno;                                                  \
      bool show_errno = false;                                                 \
      const char *system_message;                                              \
      char user_message[USER_MESSAGE_SIZE];                                    \
      system_message = grn_strerror(errno);                                    \
      switch (errno_keep) {                                                    \
      case EPERM:                                                              \
        rc = GRN_OPERATION_NOT_PERMITTED;                                      \
        break;                                                                 \
      case ENOENT:                                                             \
        rc = GRN_NO_SUCH_FILE_OR_DIRECTORY;                                    \
        break;                                                                 \
      case ESRCH:                                                              \
        rc = GRN_NO_SUCH_PROCESS;                                              \
        break;                                                                 \
      case EINTR:                                                              \
        rc = GRN_INTERRUPTED_FUNCTION_CALL;                                    \
        break;                                                                 \
      case EIO:                                                                \
        rc = GRN_INPUT_OUTPUT_ERROR;                                           \
        break;                                                                 \
      case E2BIG:                                                              \
        rc = GRN_ARG_LIST_TOO_LONG;                                            \
        break;                                                                 \
      case ENOEXEC:                                                            \
        rc = GRN_EXEC_FORMAT_ERROR;                                            \
        break;                                                                 \
      case EBADF:                                                              \
        rc = GRN_BAD_FILE_DESCRIPTOR;                                          \
        break;                                                                 \
      case ECHILD:                                                             \
        rc = GRN_NO_CHILD_PROCESSES;                                           \
        break;                                                                 \
      case EAGAIN:                                                             \
        rc = GRN_OPERATION_WOULD_BLOCK;                                        \
        break;                                                                 \
      case ENOMEM:                                                             \
        rc = GRN_NO_MEMORY_AVAILABLE;                                          \
        break;                                                                 \
      case EACCES:                                                             \
        rc = GRN_PERMISSION_DENIED;                                            \
        break;                                                                 \
      case EFAULT:                                                             \
        rc = GRN_BAD_ADDRESS;                                                  \
        break;                                                                 \
      case EEXIST:                                                             \
        rc = GRN_FILE_EXISTS;                                                  \
        break;                                                                 \
      /* case EXDEV : */                                                       \
      case ENODEV:                                                             \
        rc = GRN_NO_SUCH_DEVICE;                                               \
        break;                                                                 \
      case ENOTDIR:                                                            \
        rc = GRN_NOT_A_DIRECTORY;                                              \
        break;                                                                 \
      case EISDIR:                                                             \
        rc = GRN_IS_A_DIRECTORY;                                               \
        break;                                                                 \
      case EINVAL:                                                             \
        rc = GRN_INVALID_ARGUMENT;                                             \
        break;                                                                 \
      case EMFILE:                                                             \
        rc = GRN_TOO_MANY_OPEN_FILES;                                          \
        break;                                                                 \
      case ENOTTY:                                                             \
        rc = GRN_INAPPROPRIATE_I_O_CONTROL_OPERATION;                          \
        break;                                                                 \
      case EFBIG:                                                              \
        rc = GRN_FILE_TOO_LARGE;                                               \
        break;                                                                 \
      case ENOSPC:                                                             \
        rc = GRN_NO_SPACE_LEFT_ON_DEVICE;                                      \
        break;                                                                 \
      case ESPIPE:                                                             \
        rc = GRN_INVALID_SEEK;                                                 \
        break;                                                                 \
      case EROFS:                                                              \
        rc = GRN_READ_ONLY_FILE_SYSTEM;                                        \
        break;                                                                 \
      case EMLINK:                                                             \
        rc = GRN_TOO_MANY_LINKS;                                               \
        break;                                                                 \
      case EPIPE:                                                              \
        rc = GRN_BROKEN_PIPE;                                                  \
        break;                                                                 \
      case EDOM:                                                               \
        rc = GRN_DOMAIN_ERROR;                                                 \
        break;                                                                 \
      case ERANGE:                                                             \
        rc = GRN_RANGE_ERROR;                                                  \
        break;                                                                 \
      case EDEADLOCK:                                                          \
        rc = GRN_RESOURCE_DEADLOCK_AVOIDED;                                    \
        break;                                                                 \
      case ENAMETOOLONG:                                                       \
        rc = GRN_FILENAME_TOO_LONG;                                            \
        break;                                                                 \
      case EILSEQ:                                                             \
        rc = GRN_ILLEGAL_BYTE_SEQUENCE;                                        \
        break;                                                                 \
      case ECONNRESET:                                                         \
        rc = GRN_CONNECTION_RESET;                                             \
        break;                                                                 \
      /* case STRUNCATE : */                                                   \
      default:                                                                 \
        rc = GRN_UNKNOWN_ERROR;                                                \
        show_errno = true;                                                     \
        break;                                                                 \
      }                                                                        \
      grn_snprintf(user_message,                                               \
                   USER_MESSAGE_SIZE,                                          \
                   USER_MESSAGE_SIZE,                                          \
                   __VA_ARGS__);                                               \
      if (show_errno) {                                                        \
        ERR(rc,                                                                \
            "system call error[%d]: %s: %s",                                   \
            errno_keep,                                                        \
            system_message,                                                    \
            user_message);                                                     \
      } else {                                                                 \
        ERR(rc, "system call error: %s: %s", system_message, user_message);    \
      }                                                                        \
    } while (0)

#else /* WIN32 */

#  define SERR(...)                                                            \
    do {                                                                       \
      grn_rc rc;                                                               \
      int errno_keep = errno;                                                  \
      bool show_errno = false;                                                 \
      const char *system_message = grn_error_get_current_system_message();     \
      char user_message[USER_MESSAGE_SIZE];                                    \
      switch (errno_keep) {                                                    \
      case ELOOP:                                                              \
        rc = GRN_TOO_MANY_SYMBOLIC_LINKS;                                      \
        break;                                                                 \
      case ENAMETOOLONG:                                                       \
        rc = GRN_FILENAME_TOO_LONG;                                            \
        break;                                                                 \
      case ENOENT:                                                             \
        rc = GRN_NO_SUCH_FILE_OR_DIRECTORY;                                    \
        break;                                                                 \
      case ENOMEM:                                                             \
        rc = GRN_NO_MEMORY_AVAILABLE;                                          \
        break;                                                                 \
      case ENOTDIR:                                                            \
        rc = GRN_NOT_A_DIRECTORY;                                              \
        break;                                                                 \
      case EPERM:                                                              \
        rc = GRN_OPERATION_NOT_PERMITTED;                                      \
        break;                                                                 \
      case ESRCH:                                                              \
        rc = GRN_NO_SUCH_PROCESS;                                              \
        break;                                                                 \
      case EINTR:                                                              \
        rc = GRN_INTERRUPTED_FUNCTION_CALL;                                    \
        break;                                                                 \
      case EIO:                                                                \
        rc = GRN_INPUT_OUTPUT_ERROR;                                           \
        break;                                                                 \
      case ENXIO:                                                              \
        rc = GRN_NO_SUCH_DEVICE_OR_ADDRESS;                                    \
        break;                                                                 \
      case E2BIG:                                                              \
        rc = GRN_ARG_LIST_TOO_LONG;                                            \
        break;                                                                 \
      case ENOEXEC:                                                            \
        rc = GRN_EXEC_FORMAT_ERROR;                                            \
        break;                                                                 \
      case EBADF:                                                              \
        rc = GRN_BAD_FILE_DESCRIPTOR;                                          \
        break;                                                                 \
      case ECHILD:                                                             \
        rc = GRN_NO_CHILD_PROCESSES;                                           \
        break;                                                                 \
      case EACCES:                                                             \
        rc = GRN_PERMISSION_DENIED;                                            \
        break;                                                                 \
      case EFAULT:                                                             \
        rc = GRN_BAD_ADDRESS;                                                  \
        break;                                                                 \
      case EBUSY:                                                              \
        rc = GRN_RESOURCE_BUSY;                                                \
        break;                                                                 \
      case EEXIST:                                                             \
        rc = GRN_FILE_EXISTS;                                                  \
        break;                                                                 \
      case ENODEV:                                                             \
        rc = GRN_NO_SUCH_DEVICE;                                               \
        break;                                                                 \
      case EISDIR:                                                             \
        rc = GRN_IS_A_DIRECTORY;                                               \
        break;                                                                 \
      case EINVAL:                                                             \
        rc = GRN_INVALID_ARGUMENT;                                             \
        break;                                                                 \
      case EMFILE:                                                             \
        rc = GRN_TOO_MANY_OPEN_FILES;                                          \
        break;                                                                 \
      case EFBIG:                                                              \
        rc = GRN_FILE_TOO_LARGE;                                               \
        break;                                                                 \
      case ENOSPC:                                                             \
        rc = GRN_NO_SPACE_LEFT_ON_DEVICE;                                      \
        break;                                                                 \
      case EROFS:                                                              \
        rc = GRN_READ_ONLY_FILE_SYSTEM;                                        \
        break;                                                                 \
      case EMLINK:                                                             \
        rc = GRN_TOO_MANY_LINKS;                                               \
        break;                                                                 \
      case EPIPE:                                                              \
        rc = GRN_BROKEN_PIPE;                                                  \
        break;                                                                 \
      case EDOM:                                                               \
        rc = GRN_DOMAIN_ERROR;                                                 \
        break;                                                                 \
      case ERANGE:                                                             \
        rc = GRN_RANGE_ERROR;                                                  \
        break;                                                                 \
      case ENOTSOCK:                                                           \
        rc = GRN_NOT_SOCKET;                                                   \
        break;                                                                 \
      case EADDRINUSE:                                                         \
        rc = GRN_ADDRESS_IS_IN_USE;                                            \
        break;                                                                 \
      case ENETDOWN:                                                           \
        rc = GRN_NETWORK_IS_DOWN;                                              \
        break;                                                                 \
      case ENOBUFS:                                                            \
        rc = GRN_NO_BUFFER;                                                    \
        break;                                                                 \
      case EISCONN:                                                            \
        rc = GRN_SOCKET_IS_ALREADY_CONNECTED;                                  \
        break;                                                                 \
      case ENOTCONN:                                                           \
        rc = GRN_SOCKET_IS_NOT_CONNECTED;                                      \
        break;                                                                 \
        /*                                                                     \
      case ESOCKTNOSUPPORT :                                                   \
      case EOPNOTSUPP :                                                        \
      case EPFNOSUPPORT :                                                      \
        */                                                                     \
      case EPROTONOSUPPORT:                                                    \
        rc = GRN_OPERATION_NOT_SUPPORTED;                                      \
        break;                                                                 \
      case ESHUTDOWN:                                                          \
        rc = GRN_SOCKET_IS_ALREADY_SHUTDOWNED;                                 \
        break;                                                                 \
      case ETIMEDOUT:                                                          \
        rc = GRN_OPERATION_TIMEOUT;                                            \
        break;                                                                 \
      case ECONNREFUSED:                                                       \
        rc = GRN_CONNECTION_REFUSED;                                           \
        break;                                                                 \
      case EAGAIN:                                                             \
        rc = GRN_OPERATION_WOULD_BLOCK;                                        \
        break;                                                                 \
      case ECONNRESET:                                                         \
        rc = GRN_CONNECTION_RESET;                                             \
        break;                                                                 \
      default:                                                                 \
        rc = GRN_UNKNOWN_ERROR;                                                \
        show_errno = true;                                                     \
        break;                                                                 \
      }                                                                        \
      grn_snprintf(user_message,                                               \
                   USER_MESSAGE_SIZE,                                          \
                   USER_MESSAGE_SIZE,                                          \
                   __VA_ARGS__);                                               \
      if (show_errno) {                                                        \
        ERR(rc,                                                                \
            "system call error[%d]: %s: %s",                                   \
            errno_keep,                                                        \
            system_message,                                                    \
            user_message);                                                     \
      } else {                                                                 \
        ERR(rc, "system call error: %s: %s", system_message, user_message);    \
      }                                                                        \
    } while (0)

#  define SOERR(...)     SERR(__VA_ARGS__)

#  define ERRNO_ERR(...) SERR(__VA_ARGS__)

#endif /* WIN32 */

#define GERR(rc, ...) ERRSET(&grn_gctx, GRN_ERROR, (rc), __VA_ARGS__)
#define GMERR(...)                                                             \
  ERRSET(&grn_gctx, GRN_ALERT, GRN_NO_MEMORY_AVAILABLE, __VA_ARGS__)

GRN_API const char *
grn_strerror(int error_code);

#ifdef __cplusplus
}
#endif
