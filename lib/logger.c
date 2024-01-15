/*
  Copyright(C) 2009-2017  Brazil
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_logger.h"
#include "grn_ctx.h"
#include "grn_ctx_impl.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
# include <share.h>
#else /* WIN32 */
# include <sys/file.h>
#endif /* WIN32 */

static const char *log_level_names[] = {
  "none",
  "emergency",
  "alert",
  "critical",
  "error",
  "warning",
  "notice",
  "info",
  "debug",
  "dump"
};

#define GRN_LOG_LAST GRN_LOG_DUMP

typedef enum {
  GRN_FLAGS_OPERATOR_ADD,
  GRN_FLAGS_OPERATOR_REMOVE,
  GRN_FLAGS_OPERATOR_REPLACE,
} grn_flags_operator;

#define INITIAL_LOGGER {                        \
  GRN_LOG_DEFAULT_LEVEL,                        \
  GRN_LOG_DEFAULT,                              \
  NULL,                                         \
  NULL,                                         \
  NULL,                                         \
  NULL                                          \
}

static grn_logger current_logger = INITIAL_LOGGER;

const char *
grn_log_level_to_string(grn_log_level level)
{
  if (level <= GRN_LOG_LAST) {
    return log_level_names[level];
  } else {
    return "unknown";
  }
}

bool
grn_log_level_parse(const char *string, grn_log_level *level)
{
  if (strcmp(string, " ") == 0 ||
      grn_strcasecmp(string, "none") == 0) {
    *level = GRN_LOG_NONE;
    return true;
  } else if (strcmp(string, "E") == 0 ||
             grn_strcasecmp(string, "emerg") == 0 ||
             grn_strcasecmp(string, "emergency") == 0) {
    *level = GRN_LOG_EMERG;
    return true;
  } else if (strcmp(string, "A") == 0 ||
             grn_strcasecmp(string, "alert") == 0) {
    *level = GRN_LOG_ALERT;
    return true;
  } else if (strcmp(string, "C") == 0 ||
             grn_strcasecmp(string, "crit") == 0 ||
             grn_strcasecmp(string, "critical") == 0) {
    *level = GRN_LOG_CRIT;
    return true;
  } else if (strcmp(string, "e") == 0 ||
             grn_strcasecmp(string, "error") == 0) {
    *level = GRN_LOG_ERROR;
    return true;
  } else if (strcmp(string, "w") == 0 ||
             grn_strcasecmp(string, "warn") == 0 ||
             grn_strcasecmp(string, "warning") == 0) {
    *level = GRN_LOG_WARNING;
    return true;
  } else if (strcmp(string, "n") == 0 ||
             grn_strcasecmp(string, "notice") == 0) {
    *level = GRN_LOG_NOTICE;
    return true;
  } else if (strcmp(string, "i") == 0 ||
             grn_strcasecmp(string, "info") == 0) {
    *level = GRN_LOG_INFO;
    return true;
  } else if (strcmp(string, "d") == 0 ||
             grn_strcasecmp(string, "debug") == 0) {
    *level = GRN_LOG_DEBUG;
    return true;
  } else if (strcmp(string, "-") == 0 ||
             grn_strcasecmp(string, "dump") == 0) {
    *level = GRN_LOG_DUMP;
    return true;
  }
  return false;
}

bool
grn_log_flags_parse(const char *string,
                    int string_size,
                    int *flags)
{
  const char *string_end;

  *flags = GRN_LOG_DEFAULT;

  if (!string) {
    return true;
  }

  if (string_size < 0) {
    string_size = (int)strlen(string);
  }

  string_end = string + string_size;

  while (string < string_end) {
    grn_flags_operator operator = GRN_FLAGS_OPERATOR_REPLACE;

    if (*string == '|' || *string == ' ') {
      string += 1;
      continue;
    }

    if (*string == '+') {
      operator = GRN_FLAGS_OPERATOR_ADD;
      string++;
    } else if (*string == '-') {
      operator = GRN_FLAGS_OPERATOR_REMOVE;
      string++;
    }

#define CHECK_FLAG(name)                                                \
    if (((size_t)(string_end - string) >= (sizeof(#name) - 1)) &&       \
        (grn_strncasecmp(string, #name, sizeof(#name) - 1) == 0) &&     \
        (((size_t)(string_end - string) == (sizeof(#name) - 1)) ||      \
         (string[sizeof(#name) - 1] == '|') ||                          \
         (string[sizeof(#name) - 1] == ' ') ||                          \
         (string[sizeof(#name) - 1] == '+') ||                          \
         (string[sizeof(#name) - 1] == '-'))) {                         \
      if (operator == GRN_FLAGS_OPERATOR_ADD) {                         \
        *flags |= GRN_LOG_ ## name;                                     \
      } else if (operator == GRN_FLAGS_OPERATOR_REMOVE) {               \
        *flags &= ~GRN_LOG_ ## name;                                    \
      } else {                                                          \
        *flags = GRN_LOG_ ## name;                                      \
      }                                                                 \
      string += sizeof(#name) - 1;                                      \
      continue;                                                         \
    }

    CHECK_FLAG(NONE);
    CHECK_FLAG(TIME);
    CHECK_FLAG(TITLE);
    CHECK_FLAG(MESSAGE);
    CHECK_FLAG(LOCATION);
    CHECK_FLAG(PID);
    CHECK_FLAG(PROCESS_ID);
    CHECK_FLAG(THREAD_ID);
    CHECK_FLAG(ALL);
    CHECK_FLAG(DEFAULT);

#undef CHECK_FLAG

    return false;
  }

  return true;
}

typedef struct {
  char *path;
  FILE *file;
  grn_critical_section lock;
  off_t size;
  off_t rotate_threshold_size;
  bool need_flock;
} grn_logger_output;

static void
grn_logger_output_init(grn_logger_output *output)
{
  CRITICAL_SECTION_INIT(output->lock);
}

static void
grn_logger_output_close(grn_ctx *ctx, grn_logger_output *output)
{
  CRITICAL_SECTION_ENTER(output->lock);
  if (output->file) {
    if (output->file != stdout &&
        output->file != stderr) {
      fclose(output->file);
    }
    output->file = NULL;
  }
  CRITICAL_SECTION_LEAVE(output->lock);
}

static void
grn_logger_output_fin(grn_ctx *ctx, grn_logger_output *output)
{
  grn_logger_output_close(ctx, output);
  if (output->path) {
    free(output->path);
    output->path = NULL;
  }
  CRITICAL_SECTION_FIN(output->lock);
}

static bool
grn_logger_output_start(grn_ctx *ctx,
                        grn_logger_output *output,
                        bool need_flock)
{
  if (!output->path) {
    return false;
  }

  CRITICAL_SECTION_ENTER(output->lock);

  if (!output->file) {
    output->size = 0;
    if (strcmp(output->path, "-") == 0) {
      output->file = stdout;
    } else if (strcmp(output->path, "+") == 0) {
      output->file = stderr;
    } else {
      output->file = grn_fopen(output->path, "a");
      if (output->file) {
        struct stat stat;
        if (fstat(grn_fileno(output->file), &stat) != -1) {
          output->size = stat.st_size;
        }
      }
    }
  }

  if (!output->file) {
    CRITICAL_SECTION_LEAVE(output->lock);
    return false;
  }

  output->need_flock = need_flock;
  if (output->need_flock) {
    if (output->file == stdout ||
        output->file == stderr) {
      output->need_flock = false;
#ifndef _WIN32
    } else if (flock(fileno(output->file), LOCK_EX) == -1) {
      output->need_flock = false;
#endif
    }
  }

  return true;
}

static void
grn_logger_output_rotate(grn_ctx *ctx, grn_logger_output *output)
{
  char rotated_path[PATH_MAX];
  grn_timeval now;
  struct tm tm_buffer;
  struct tm *tm;

  grn_timeval_now(ctx, &now);
  tm = grn_timeval2tm(ctx, &now, &tm_buffer);
  grn_snprintf(rotated_path, PATH_MAX, PATH_MAX,
               "%s.%04d-%02d-%02d-%02d-%02d-%02d-%06d",
               output->path,
               tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
               tm->tm_hour, tm->tm_min, tm->tm_sec,
               (int)(GRN_TIME_NSEC_TO_USEC(now.tv_nsec)));
  rename(output->path, rotated_path);
}

static void
grn_logger_output_end(grn_ctx *ctx,
                      grn_logger_output *output,
                      int written)
{
  if (written > 0) {
    output->size += written;
    if (output->file != stdout &&
        output->file != stderr &&
        (output->rotate_threshold_size > 0 &&
         output->size >= output->rotate_threshold_size)) {
      fclose(output->file);
      output->file = NULL;
      grn_logger_output_rotate(ctx, output);
    } else {
      fflush(output->file);
    }
  }
#ifndef _WIN32
  if (output->need_flock) {
    flock(fileno(output->file), LOCK_UN);
  }
#endif

  CRITICAL_SECTION_LEAVE(output->lock);
}

static bool logger_inited = false;
static grn_logger_output default_logger_output = {0};

static void
default_logger_log(grn_ctx *ctx, grn_log_level level,
                   const char *timestamp, const char *title,
                   const char *message, const char *location, void *user_data)
{
  if (!grn_logger_output_start(ctx,
                               &default_logger_output,
                               (current_logger.flags & GRN_LOG_PID))) {
    return;
  }

  const char slev[] = " EACewnid-";
  char label = *(slev + level);
  int written = 0;
  if (location && *location) {
    if (title && *title) {
      written = fprintf(default_logger_output.file, "%s|%c|%s: %s %s\n",
                        timestamp, label, location, title, message);
    } else {
      written = fprintf(default_logger_output.file, "%s|%c|%s: %s\n",
                        timestamp, label, location, message);
    }
  } else {
    written = fprintf(default_logger_output.file, "%s|%c|%s %s\n",
                      timestamp, label, title, message);
  }

  grn_logger_output_end(ctx, &default_logger_output, written);
}

static void
default_logger_reopen(grn_ctx *ctx, void *user_data)
{
  GRN_LOG(ctx, GRN_LOG_DEBUG, "log will be closed.");
  grn_logger_output_close(ctx, &default_logger_output);
  GRN_LOG(ctx, GRN_LOG_DEBUG, "log opened.");
}

static void
default_logger_fin(grn_ctx *ctx, void *user_data)
{
  grn_logger_output_close(ctx, &default_logger_output);
}

static grn_logger default_logger = {
  GRN_LOG_DEFAULT_LEVEL,
  GRN_LOG_DEFAULT,
  NULL,
  default_logger_log,
  default_logger_reopen,
  default_logger_fin
};

void
grn_default_logger_set_max_level(grn_log_level max_level)
{
  default_logger.max_level = max_level;
  if (current_logger.log == default_logger_log) {
    current_logger.max_level = max_level;
  }
}

grn_log_level
grn_default_logger_get_max_level(void)
{
  return default_logger.max_level;
}

void
grn_default_logger_set_flags(int flags)
{
  default_logger.flags = flags;
  if (current_logger.log == default_logger_log) {
    current_logger.flags = flags;
  }
}

int
grn_default_logger_get_flags(void)
{
  return default_logger.flags;
}

void
grn_default_logger_set_path(const char *path)
{
  if (logger_inited) {
    CRITICAL_SECTION_ENTER(default_logger_output.lock);
  }

  if (default_logger_output.path) {
    free(default_logger_output.path);
  }

  if (path) {
    default_logger_output.path = grn_strdup_raw(path);
  } else {
    default_logger_output.path = NULL;
  }

  if (logger_inited) {
    CRITICAL_SECTION_LEAVE(default_logger_output.lock);
  }
}

const char *
grn_default_logger_get_path(void)
{
  return default_logger_output.path;
}

void
grn_default_logger_set_rotate_threshold_size(off_t threshold)
{
  default_logger_output.rotate_threshold_size = threshold;
}

off_t
grn_default_logger_get_rotate_threshold_size(void)
{
  return default_logger_output.rotate_threshold_size;
}

void
grn_logger_reopen(grn_ctx *ctx)
{
  if (current_logger.reopen) {
    current_logger.reopen(ctx, current_logger.user_data);
  }
}

static void
current_logger_fin(grn_ctx *ctx)
{
  if (current_logger.fin) {
    current_logger.fin(ctx, current_logger.user_data);
  }
  {
    grn_logger initial_logger = INITIAL_LOGGER;
    current_logger = initial_logger;
  }
}

static void
logger_info_func_wrapper(grn_ctx *ctx, grn_log_level level,
                         const char *timestamp, const char *title,
                         const char *message, const char *location,
                         void *user_data)
{
  grn_logger_info *info = user_data;
  info->func(level, timestamp, title, message, location, info->func_arg);
}

/* Deprecated since 2.1.2. */
grn_rc
grn_logger_info_set(grn_ctx *ctx, const grn_logger_info *info)
{
  if (info) {
    grn_logger logger;

    memset(&logger, 0, sizeof(grn_logger));
    logger.max_level = info->max_level;
    logger.flags = info->flags;
    if (info->func) {
      logger.log       = logger_info_func_wrapper;
      logger.user_data = (grn_logger_info *)info;
    } else {
      logger.log    = default_logger_log;
      logger.reopen = default_logger_reopen;
      logger.fin    = default_logger_fin;
    }
    return grn_logger_set(ctx, &logger);
  } else {
    return grn_logger_set(ctx, NULL);
  }
}

grn_rc
grn_logger_set(grn_ctx *ctx, const grn_logger *logger)
{
  current_logger_fin(ctx);
  if (logger) {
    current_logger = *logger;
  } else {
    current_logger = default_logger;
  }
  return GRN_SUCCESS;
}

bool
grn_logger_is_default_logger(grn_ctx *ctx)
{
  return current_logger.log == default_logger.log;
}

void
grn_logger_set_max_level(grn_ctx *ctx, grn_log_level max_level)
{
  current_logger.max_level = max_level;
}

grn_log_level
grn_logger_get_max_level(grn_ctx *ctx)
{
  return current_logger.max_level;
}

bool
grn_logger_pass(grn_ctx *ctx, grn_log_level level)
{
  return level <= current_logger.max_level;
}

#define TBUFSIZE GRN_TIMEVAL_STR_SIZE
#define MBUFSIZE 0x1000
#define LBUFSIZE 0x400

void
grn_logger_put(grn_ctx *ctx,
               grn_log_level level,
               const char *file,
               int line,
               const char *func,
               const char *fmt,
               ...)
{
  va_list ap;
  va_start(ap, fmt);
  grn_logger_putv(ctx, level, file, line, func, fmt, ap);
  va_end(ap);
}

void
grn_logger_putv(grn_ctx *ctx,
                grn_log_level level,
                const char *file,
                int line,
                const char *func,
                const char *fmt,
                va_list ap)
{
  if (!ctx) {
    ctx = &grn_gctx;
  }

  if (level <= current_logger.max_level && current_logger.log) {
    char tbuf[TBUFSIZE];
    char mbuf[MBUFSIZE];
    char lbuf[LBUFSIZE];
    tbuf[0] = '\0';
    if (current_logger.flags & GRN_LOG_TIME) {
      grn_timeval tv;
      grn_timeval_now(ctx, &tv);
      grn_timeval2str(ctx, &tv, tbuf, TBUFSIZE);
    }
    if (current_logger.flags & GRN_LOG_MESSAGE) {
      grn_vsnprintf(mbuf, MBUFSIZE, fmt, ap);
    } else {
      mbuf[0] = '\0';
    }
    {
      char *lbuf_current = lbuf;
      size_t lbuf_rest_size = LBUFSIZE;
      lbuf[0] = '\0';
      lbuf_current = lbuf;
      if ((current_logger.flags & GRN_LOG_PID) ||
          /* For backward compatibility. GRN_LOG_LOCATION implies GRN_LOG_PID. */
          (current_logger.flags & GRN_LOG_LOCATION)) {
        grn_snprintf(lbuf_current, lbuf_rest_size, lbuf_rest_size,
                     "%d", grn_getpid());
        const size_t lbuf_size = strlen(lbuf_current);
        lbuf_current += lbuf_size;
        lbuf_rest_size -= lbuf_size;
      }
      if (current_logger.flags & GRN_LOG_THREAD_ID) {
        const char *prefix = "";
        if (lbuf_current != lbuf) {
          prefix = "|";
        }
#ifdef HAVE_PTHREAD_H
        grn_snprintf(lbuf_current, lbuf_rest_size, lbuf_rest_size,
                     "%s%08lx", prefix, (uintptr_t)(pthread_self()));
#elif defined(WIN32) /* HAVE_PTHREAD_H */
        grn_snprintf(lbuf_current, lbuf_rest_size, lbuf_rest_size,
                     "%s%08ld", prefix, GetCurrentThreadId());
#endif /* HAVE_PTHREAD_H */
        const size_t lbuf_size = strlen(lbuf_current);
        lbuf_current += lbuf_size;
        lbuf_rest_size -= lbuf_size;
      }
      if (current_logger.flags & GRN_LOG_LOCATION) {
        const char *prefix = "";
        if (lbuf_current != lbuf) {
          prefix = " ";
        }
        grn_snprintf(lbuf_current, lbuf_rest_size, lbuf_rest_size,
                     "%s%s:%d %s()", prefix, file, line, func);
        const size_t lbuf_size = strlen(lbuf_current);
        lbuf_current += lbuf_size;
        lbuf_rest_size -= lbuf_size;
      }
    }

    if (mbuf[0] == '\0') {
      current_logger.log(ctx, level, tbuf, "", mbuf, lbuf,
                         current_logger.user_data);
    } else {
      const char *mbuf_line_start = mbuf;
      const char *mbuf_end = mbuf + strlen(mbuf);
      int mbuf_char_length = 0;
      for (char *mbuf_current = mbuf;
           mbuf_current < mbuf_end;
           mbuf_current += mbuf_char_length) {
        mbuf_char_length = grn_charlen(ctx, mbuf_current, mbuf_end);
        if (mbuf_char_length == 0) {
          break;
        } else if (mbuf_char_length == 1 && mbuf_current[0] == '\n') {
          mbuf_current[0] = '\0';
          current_logger.log(ctx, level, tbuf, "", mbuf_line_start, lbuf,
                             current_logger.user_data);
          mbuf_line_start = mbuf_current + 1;
        }
      }
      if (mbuf_line_start < mbuf_end) {
        current_logger.log(ctx, level, tbuf, "", mbuf_line_start, lbuf,
                           current_logger.user_data);
      }
    }
  }
}

void
grn_logger_init(void)
{
  grn_logger_output_init(&default_logger_output);
  if (!current_logger.log) {
    current_logger = default_logger;
  }

  logger_inited = true;
}

void
grn_logger_fin(grn_ctx *ctx)
{
  current_logger_fin(ctx);
  grn_logger_output_fin(ctx, &default_logger_output);
  logger_inited = false;
}


static bool query_logger_inited = false;
static grn_logger_output default_query_logger_output = {0};

bool
grn_query_log_flags_parse(const char *string,
                          int string_size,
                          unsigned int *flags)
{
  const char *string_end;

  *flags = GRN_QUERY_LOG_NONE;

  if (!string) {
    return true;
  }

  if (string_size < 0) {
    string_size = (int)strlen(string);
  }

  string_end = string + string_size;

  while (string < string_end) {
    if (*string == '|' || *string == ' ') {
      string += 1;
      continue;
    }

#define CHECK_FLAG(name)                                            \
    if (((size_t)(string_end - string) >= (sizeof(#name) - 1)) &&   \
        (memcmp(string, #name, sizeof(#name) - 1) == 0) &&          \
        (((size_t)(string_end - string) == (sizeof(#name) - 1)) ||  \
         (string[sizeof(#name) - 1] == '|') ||                      \
         (string[sizeof(#name) - 1] == ' '))) {                     \
      *flags |= GRN_QUERY_LOG_ ## name;                             \
      string += sizeof(#name) - 1;                                  \
      continue;                                                     \
    }

    CHECK_FLAG(NONE);
    CHECK_FLAG(COMMAND);
    CHECK_FLAG(RESULT_CODE);
    CHECK_FLAG(DESTINATION);
    CHECK_FLAG(CACHE);
    CHECK_FLAG(SIZE);
    CHECK_FLAG(SCORE);
    CHECK_FLAG(ALL);
    CHECK_FLAG(DEFAULT);

#undef CHECK_FLAG

    return false;
  }

  return true;
}

static void
default_query_logger_log(grn_ctx *ctx, unsigned int flag,
                         const char *timestamp, const char *info,
                         const char *message, void *user_data)
{
  if (!grn_logger_output_start(ctx,
                               &default_query_logger_output,
                               (current_logger.flags & GRN_LOG_PID))) {
    return;
  }
  int written = fprintf(default_query_logger_output.file, "%s|%s%s\n",
                        timestamp, info, message);
  grn_logger_output_end(ctx, &default_query_logger_output, written);
}

static void
default_query_logger_close(grn_ctx *ctx, void *user_data)
{
  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_DESTINATION, " ",
                "query log will be closed: <%s>",
                default_query_logger_output.path);
  grn_logger_output_close(ctx, &default_query_logger_output);
}

static void
default_query_logger_reopen(grn_ctx *ctx, void *user_data)
{
  default_query_logger_close(ctx, user_data);
  if (default_query_logger_output.path) {
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_DESTINATION, " ",
                  "query log is opened: <%s>",
                  default_query_logger_output.path);
  }
}

static void
default_query_logger_fin(grn_ctx *ctx, void *user_data)
{
  default_query_logger_close(ctx, user_data);
}

static grn_query_logger default_query_logger = {
  GRN_QUERY_LOG_DEFAULT,
  NULL,
  default_query_logger_log,
  default_query_logger_reopen,
  default_query_logger_fin
};

#define INITIAL_QUERY_LOGGER {                  \
  GRN_QUERY_LOG_DEFAULT,                        \
  NULL,                                         \
  NULL,                                         \
  NULL,                                         \
  NULL                                          \
}

static grn_query_logger current_query_logger = INITIAL_QUERY_LOGGER;

void
grn_default_query_logger_set_flags(unsigned int flags)
{
  default_query_logger.flags = flags;
  if (current_query_logger.log == default_query_logger_log) {
    current_query_logger.flags = flags;
  }
}

unsigned int
grn_default_query_logger_get_flags(void)
{
  return default_query_logger.flags;
}

void
grn_default_query_logger_set_path(const char *path)
{
  if (query_logger_inited) {
    CRITICAL_SECTION_ENTER(default_query_logger_output.lock);
  }

  if (default_query_logger_output.path) {
    free(default_query_logger_output.path);
  }

  if (path) {
    default_query_logger_output.path = grn_strdup_raw(path);
  } else {
    default_query_logger_output.path = NULL;
  }

  if (query_logger_inited) {
    CRITICAL_SECTION_LEAVE(default_query_logger_output.lock);
  }
}

const char *
grn_default_query_logger_get_path(void)
{
  return default_query_logger_output.path;
}

void
grn_default_query_logger_set_rotate_threshold_size(off_t threshold)
{
  default_query_logger_output.rotate_threshold_size = threshold;
}

off_t
grn_default_query_logger_get_rotate_threshold_size(void)
{
  return default_query_logger_output.rotate_threshold_size;
}

void
grn_query_logger_reopen(grn_ctx *ctx)
{
  if (current_query_logger.reopen) {
    current_query_logger.reopen(ctx, current_query_logger.user_data);
  }
}

static void
current_query_logger_fin(grn_ctx *ctx)
{
  if (current_query_logger.fin) {
    current_query_logger.fin(ctx, current_query_logger.user_data);
  }
  {
    grn_query_logger initial_query_logger = INITIAL_QUERY_LOGGER;
    current_query_logger = initial_query_logger;
  }
}

grn_rc
grn_query_logger_set(grn_ctx *ctx, const grn_query_logger *logger)
{
  current_query_logger_fin(ctx);
  if (logger) {
    current_query_logger = *logger;
  } else {
    current_query_logger = default_query_logger;
  }
  return GRN_SUCCESS;
}

void
grn_query_logger_set_flags(grn_ctx *ctx, unsigned int flags)
{
  current_query_logger.flags = flags;
}

void
grn_query_logger_add_flags(grn_ctx *ctx, unsigned int flags)
{
  current_query_logger.flags |= flags;
}

void
grn_query_logger_remove_flags(grn_ctx *ctx, unsigned int flags)
{
  current_query_logger.flags &= ~flags;
}

unsigned int
grn_query_logger_get_flags(grn_ctx *ctx)
{
  return current_query_logger.flags;
}

bool
grn_query_logger_pass(grn_ctx *ctx, unsigned int flag)
{
  return current_query_logger.flags & flag;
}

#define TIMESTAMP_BUFFER_SIZE    TBUFSIZE
/* 8+a(%p) + 1(|) + 1(mark) + 15(elapsed time) = 25+a */
#define INFO_BUFFER_SIZE         40

void
grn_query_logger_put(grn_ctx *ctx, unsigned int flag, const char *mark,
                     const char *format, ...)
{
  char timestamp[TIMESTAMP_BUFFER_SIZE];
  char info[INFO_BUFFER_SIZE];
  grn_obj *message = &ctx->impl->query_log_buf;

  if (!current_query_logger.log) {
    return;
  }

  {
    grn_timeval tv;
    timestamp[0] = '\0';
    grn_timeval_now(ctx, &tv);
    grn_timeval2str(ctx, &tv, timestamp, TIMESTAMP_BUFFER_SIZE);
  }

  grn_ctx *target_ctx = ctx;
  while (target_ctx->impl->parent) {
    target_ctx = target_ctx->impl->parent;
  }
  if (flag & (GRN_QUERY_LOG_COMMAND | GRN_QUERY_LOG_DESTINATION)) {
    grn_snprintf(info, INFO_BUFFER_SIZE, INFO_BUFFER_SIZE,
                 "%p|%s", target_ctx, mark);
    info[INFO_BUFFER_SIZE - 1] = '\0';
  } else {
    grn_timeval tv;
    uint64_t elapsed_time;
    grn_timeval_now(ctx, &tv);
    elapsed_time =
      (uint64_t)(tv.tv_sec - target_ctx->impl->tv.tv_sec) *
      GRN_TIME_NSEC_PER_SEC +
      (uint64_t)(tv.tv_nsec - target_ctx->impl->tv.tv_nsec);

    grn_snprintf(info, INFO_BUFFER_SIZE, INFO_BUFFER_SIZE,
                 "%p|%s%015" GRN_FMT_INT64U " ", target_ctx, mark, elapsed_time);
    info[INFO_BUFFER_SIZE - 1] = '\0';
  }

  {
    va_list args;

    va_start(args, format);
    GRN_BULK_REWIND(message);
    grn_text_printfv(ctx, message, format, args);
    va_end(args);
    GRN_TEXT_PUTC(ctx, message, '\0');
  }

  current_query_logger.log(ctx, flag, timestamp, info, GRN_TEXT_VALUE(message),
                           current_query_logger.user_data);
}

void
grn_query_logger_init(void)
{
  current_query_logger = default_query_logger;
  grn_logger_output_init(&default_query_logger_output);
  query_logger_inited = true;
}

void
grn_query_logger_fin(grn_ctx *ctx)
{
  current_query_logger_fin(ctx);
  grn_logger_output_fin(ctx, &default_query_logger_output);
  query_logger_inited = false;
}

void
grn_log_reopen(grn_ctx *ctx)
{
  grn_logger_reopen(ctx);
  grn_query_logger_reopen(ctx);
}
