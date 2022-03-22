/*
  Copyright(C) 2009-2016  Brazil
  Copyright(C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_time.h"
#include "grn_ctx.h"
#include "grn_str.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#if defined(HAVE__LOCALTIME64_S) && defined(__GNUC__)
# ifdef _WIN64
#  define localtime_s(tm, time) _localtime64_s(tm, time)
# else /* _WIN64 */
#  define localtime_s(tm, time) _localtime32_s(tm, time)
# endif /* _WIN64 */
#endif /* defined(HAVE__LOCALTIME64_S) && defined(__GNUC__) */

/* fixme by 2038 */

grn_rc
grn_timeval_now(grn_ctx *ctx, grn_timeval *tv)
{
#ifdef WIN32
  struct __timeb64 tb;
  _ftime64_s(&tb);
  tv->tv_sec = tb.time;
  tv->tv_nsec = tb.millitm * (GRN_TIME_NSEC_PER_SEC / 1000);
  return GRN_SUCCESS;
#else /* WIN32 */
# ifdef HAVE_CLOCK_GETTIME
  struct timespec t;
  if (clock_gettime(CLOCK_REALTIME, &t)) {
    SERR("clock_gettime");
  } else {
    tv->tv_sec = t.tv_sec;
    tv->tv_nsec = t.tv_nsec;
  }
  return ctx->rc;
# else /* HAVE_CLOCK_GETTIME */
  struct timeval t;
  if (gettimeofday(&t, NULL)) {
    SERR("gettimeofday");
  } else {
    tv->tv_sec = t.tv_sec;
    tv->tv_nsec = GRN_TIME_USEC_TO_NSEC(t.tv_usec);
  }
  return ctx->rc;
# endif /* HAVE_CLOCK_GETTIME */
#endif /* WIN32 */
}

grn_timeval
grn_timeval_from_double(grn_ctx *ctx, double value)
{
  grn_timeval timeval;
  timeval.tv_sec = (int64_t)(trunc(value));
  timeval.tv_nsec =
    (int64_t)((value - timeval.tv_sec) * GRN_TIME_NSEC_PER_SEC) %
    GRN_TIME_NSEC_PER_SEC;
  return timeval;
}

void
grn_time_now(grn_ctx *ctx, grn_obj *obj)
{
  grn_timeval tv;
  grn_timeval_now(ctx, &tv);
  GRN_TIME_SET(ctx, obj, GRN_TIME_PACK(tv.tv_sec,
                                       GRN_TIME_NSEC_TO_USEC(tv.tv_nsec)));
}

static grn_bool
grn_time_t_to_tm(grn_ctx *ctx, const grn_time_t time, struct tm *tm)
{
  grn_bool success;
  const char *function_name;
#ifdef HAVE__LOCALTIME64_S
  function_name = "localtime_s";
  success = (_localtime64_s(tm, &time) == 0);
#else /* HAVE__LOCALTIME64_S */
# ifdef HAVE_LOCALTIME_R
  function_name = "localtime_r";
  success = (localtime_r(&time, tm) != NULL);
# else /* HAVE_LOCALTIME_R */
  function_name = "localtime";
  {
    struct tm *local_tm;
    local_tm = localtime(&time);
    if (local_tm) {
      success = GRN_TRUE;
      memcpy(tm, local_tm, sizeof(struct tm));
    } else {
      success = GRN_FALSE;
    }
  }
# endif /* HAVE_LOCALTIME_R */
#endif /* HAVE__LOCALTIME64_S */
  if (!success) {
    SERR("%s: failed to convert time_t to struct tm: <%" GRN_FMT_INT64D ">",
         function_name,
         (int64_t)time);
  }
  return success;
}

struct tm *
grn_timeval2tm(grn_ctx *ctx, grn_timeval *tv, struct tm *tm)
{
  if (grn_time_t_to_tm(ctx, tv->tv_sec, tm)) {
    return tm;
  } else {
    return NULL;
  }
}

grn_bool
grn_time_to_tm(grn_ctx *ctx, int64_t time, struct tm *tm)
{
  int64_t sec;
  int32_t usec;

  GRN_TIME_UNPACK(time, sec, usec);
  return grn_time_t_to_tm(ctx, sec, tm);
}

static grn_bool
grn_time_t_from_tm(grn_ctx *ctx, grn_time_t *time, struct tm *tm)
{
  grn_bool success;

  tm->tm_yday = -1;
  *time = grn_mktime(tm);
  /* We can't use (*time != -1) because -1 is a valid UNIX time
   * (1969-12-31T23:59:59Z). */
  success = (tm->tm_yday != -1);
  if (!success) {
    ERR(GRN_INVALID_ARGUMENT,
        "mktime: failed to convert struct tm to time_t: "
        "<%04d-%02d-%02dT%02d:%02d:%02d>(%d)",
        1900 + tm->tm_year,
        tm->tm_mon + 1,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec,
        tm->tm_isdst);
  }
  return success;
}

grn_bool
grn_time_from_tm(grn_ctx *ctx, int64_t *time, struct tm *tm)
{
  grn_time_t sec_time_t;
  int64_t sec;
  int32_t usec = 0;

  if (!grn_time_t_from_tm(ctx, &sec_time_t, tm)) {
    return GRN_FALSE;
  }

  sec = sec_time_t;
  *time = GRN_TIME_PACK(sec, usec);
  return GRN_TRUE;
}

grn_rc
grn_timeval2str(grn_ctx *ctx, grn_timeval *tv, char *buf, size_t buf_size)
{
  struct tm tm;
  struct tm *ltm;
  ltm = grn_timeval2tm(ctx, tv, &tm);
  grn_snprintf(buf, buf_size, GRN_TIMEVAL_STR_SIZE,
               GRN_TIMEVAL_STR_FORMAT,
               ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday,
               ltm->tm_hour, ltm->tm_min, ltm->tm_sec,
               (int)(GRN_TIME_NSEC_TO_USEC(tv->tv_nsec)));
  if (buf_size > GRN_TIMEVAL_STR_SIZE) {
    buf[GRN_TIMEVAL_STR_SIZE - 1] = '\0';
  } else {
    buf[buf_size - 1] = '\0';
  }
  return ctx->rc;
}

static grn_rc
grn_str2timeval_offset_sec(const char *start, const char *end, bool *have_offset, int32_t *gm_offset_sec)
{
  const char *position = start;
  int32_t hours, minutes = 0, sign = 1;

  *have_offset = false;
  *gm_offset_sec = 0;

  if (position >= end) { return GRN_SUCCESS; }

  switch (*position) {
  case '+' :
    sign = 1;
    *have_offset = true;
    break;
  case '-' :
    sign = -1;
    *have_offset = true;
    break;
  case 'Z' :
  case 'z' :
    *have_offset = true;
    return GRN_SUCCESS;
  default :
    return GRN_INVALID_ARGUMENT;
  }

  position++;
  if (position >= end) { return GRN_INVALID_ARGUMENT; }

  hours = grn_atoi(position, end, &position);
  if (hours < 0 || hours >= 24) { return GRN_INVALID_ARGUMENT; }

  if (position < end && *position == ':') {
    position++;
    if (position == end) { return GRN_INVALID_ARGUMENT; }

    minutes = grn_atoi(position, end, &position);
    if (minutes < 0 || minutes >= 60) { return GRN_INVALID_ARGUMENT; }
  }

  *gm_offset_sec = sign * (hours * 3600 + minutes * 60);

  return GRN_SUCCESS;
}

grn_rc
grn_str2timeval(const char *str, uint32_t str_len, grn_timeval *tv)
{
  struct tm tm;
  const char *r1, *r2, *rend = str + str_len;
  uint32_t uv = 0;

  memset(&tm, 0, sizeof(struct tm));

  tm.tm_year = (int)grn_atoui(str, rend, &r1) - 1900;
  if ((r1 + 1) >= rend || (*r1 != '/' && *r1 != '-')) {
    return GRN_INVALID_ARGUMENT;
  }
  r1++;
  tm.tm_mon = (int)grn_atoui(r1, rend, &r1) - 1;
  if ((r1 + 1) >= rend || (*r1 != '/' && *r1 != '-') ||
      tm.tm_mon < 0 || tm.tm_mon >= 12) { return GRN_INVALID_ARGUMENT; }
  r1++;
  tm.tm_mday = (int)grn_atoui(r1, rend, &r1);
  if ((r1 + 1) >= rend || (*r1 != ' ' && *r1 != 'T' && *r1 != 't') ||
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

  if ((r1 + 1) < rend && *r1 == '.') {
    uv = grn_atoi(++r1, rend, &r2);

    for (int i = 0; r2 + i < r1 + 6; i++) {
      uv *= 10;
    }
    if (uv >= GRN_TIME_USEC_PER_SEC) { return GRN_INVALID_ARGUMENT; }
  }

  tm.tm_yday = -1;
  tm.tm_isdst = -1;

  {
    bool have_offset = false;
    int32_t gm_offset_sec = 0;
    grn_rc rc = GRN_SUCCESS;

    rc = grn_str2timeval_offset_sec(r2, rend, &have_offset, &gm_offset_sec);

    if (rc != GRN_SUCCESS) { return rc; }

    if (have_offset) {
      tv->tv_sec = grn_timegm(&tm);
      tv->tv_sec -= gm_offset_sec;
    } else {
      tv->tv_sec = grn_mktime(&tm);
    }
    /* tm_yday is set appropriately (0-365) on successful completion. */
    if (tm.tm_yday == -1) { return GRN_INVALID_ARGUMENT; }
  }

  tv->tv_nsec = GRN_TIME_USEC_TO_NSEC(uv);
  return GRN_SUCCESS;
}
