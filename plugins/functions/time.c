/*
  Copyright(C) 2016-2018  Brazil
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
# define GRN_PLUGIN_FUNCTION_TAG functions_time
#endif /* GRN_EMBEDDED */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <groonga/plugin.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
# define timegm _mkgmtime
# define HAVE_TIMEGM
#else /* WIN32 */
# define HAVE_TIMEGM
#endif /* WIN32 */

typedef enum {
  GRN_TIME_CLASSIFY_UNIT_SECOND,
  GRN_TIME_CLASSIFY_UNIT_MINUTE,
  GRN_TIME_CLASSIFY_UNIT_HOUR,
  GRN_TIME_CLASSIFY_UNIT_DAY,
  GRN_TIME_CLASSIFY_UNIT_WEEK,
  GRN_TIME_CLASSIFY_UNIT_MONTH,
  GRN_TIME_CLASSIFY_UNIT_YEAR
} grn_time_classify_unit;

static bool
func_time_classify_raw_compute(grn_ctx *ctx,
                               grn_obj *time,
                               grn_time_classify_unit unit,
                               uint32_t interval_raw,
                               int64_t *classed_time_raw,
                               const char *function_name)
{
  int64_t time_raw;
  struct tm tm;

  if (time->header.domain != GRN_DB_TIME) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, time);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): "
                     "the first argument must be a time: "
                     "<%.*s>",
                     function_name,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return false;
  }

  time_raw = GRN_TIME_VALUE(time);
  if (!grn_time_to_tm(ctx, time_raw, &tm)) {
    return false;
  }

  struct tm tm_timezone_offset;
  grn_time_to_tm(ctx, 0, &tm_timezone_offset);
  int64_t timezone_offset = 0;

  switch (unit) {
  case GRN_TIME_CLASSIFY_UNIT_SECOND :
    tm.tm_sec = (tm.tm_sec / interval_raw) * interval_raw;
    break;
  case GRN_TIME_CLASSIFY_UNIT_MINUTE :
    tm.tm_min = (tm.tm_min / interval_raw) * interval_raw;
    tm.tm_sec = 0;
    break;
  case GRN_TIME_CLASSIFY_UNIT_HOUR :
    tm.tm_hour = (tm.tm_hour / interval_raw) * interval_raw;
    tm.tm_min = tm_timezone_offset.tm_min;
    tm.tm_sec = 0;
    timezone_offset = GRN_TIME_PACK((tm_timezone_offset.tm_min * 60),
                                    0);
    break;
  case GRN_TIME_CLASSIFY_UNIT_DAY :
    tm.tm_hour = tm_timezone_offset.tm_hour;
    tm.tm_min = tm_timezone_offset.tm_min;
    tm.tm_sec = 0;
    timezone_offset = GRN_TIME_PACK((tm_timezone_offset.tm_hour * 60 * 60) +
                                    (tm_timezone_offset.tm_min * 60),
                                    0);
    break;
  case GRN_TIME_CLASSIFY_UNIT_WEEK :
    if ((tm.tm_mday - tm.tm_wday) >= 0) {
      tm.tm_mday -= tm.tm_wday;
    } else {
      int n_underflowed_mday = -(tm.tm_mday - tm.tm_wday);
      int mday;
      int max_mday = 31;

      if (tm.tm_mon == 0) {
        tm.tm_year--;
        tm.tm_mon = 11;
      } else {
        tm.tm_mon--;
      }

      for (mday = max_mday; mday > n_underflowed_mday; mday--) {
        int64_t unused;
        tm.tm_mday = mday;
        if (grn_time_from_tm(ctx, &unused, &tm)) {
          break;
        }
      }
      tm.tm_mday -= n_underflowed_mday;
    }
    tm.tm_hour = tm_timezone_offset.tm_hour;
    tm.tm_min = tm_timezone_offset.tm_min;
    tm.tm_sec = 0;
    timezone_offset = GRN_TIME_PACK((tm_timezone_offset.tm_hour * 60 * 60) +
                                    (tm_timezone_offset.tm_min * 60),
                                    0);
    break;
  case GRN_TIME_CLASSIFY_UNIT_MONTH :
    tm.tm_mon = (tm.tm_mon / interval_raw) * interval_raw;
    tm.tm_mday = 1;
    tm.tm_hour = tm_timezone_offset.tm_hour;
    tm.tm_min = tm_timezone_offset.tm_min;
    tm.tm_sec = 0;
    timezone_offset = GRN_TIME_PACK((tm_timezone_offset.tm_hour * 60 * 60) +
                                    (tm_timezone_offset.tm_min * 60),
                                    0);
    break;
  case GRN_TIME_CLASSIFY_UNIT_YEAR :
    tm.tm_year = (((1900 + tm.tm_year) / interval_raw) * interval_raw) - 1900;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = tm_timezone_offset.tm_hour;
    tm.tm_min = tm_timezone_offset.tm_min;
    tm.tm_sec = 0;
    timezone_offset = GRN_TIME_PACK((tm_timezone_offset.tm_hour * 60 * 60) +
                                    (tm_timezone_offset.tm_min * 60),
                                    0);
    break;
  }

  if (!grn_time_from_tm(ctx, classed_time_raw, &tm)) {
    return false;
  }
  *classed_time_raw -= timezone_offset;
  return true;
}

static bool
func_time_classify_raw_validate_arg0(grn_ctx *ctx,
                                     grn_obj *arg0,
                                     const char *function_name)
{
  if (arg0->header.domain == GRN_DB_TIME) {
    switch (arg0->header.type) {
    case GRN_BULK :
    case GRN_UVECTOR :
      return true;
    default :
      break;
    }
  }

  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, arg0);
  GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                   "%s(): "
                   "the first argument must be a time or a time vector: "
                   "<%.*s>",
                   function_name,
                   (int)GRN_TEXT_LEN(&inspected),
                   GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
  return false;
}

static grn_obj *
func_time_classify_raw(grn_ctx *ctx,
                       int n_args,
                       grn_obj **args,
                       grn_user_data *user_data,
                       const char *function_name,
                       grn_time_classify_unit unit)
{
  uint32_t interval_raw = 1;
  bool accept_interval = GRN_TRUE;

  switch (unit) {
  case GRN_TIME_CLASSIFY_UNIT_SECOND :
  case GRN_TIME_CLASSIFY_UNIT_MINUTE :
  case GRN_TIME_CLASSIFY_UNIT_HOUR :
    accept_interval = GRN_TRUE;
    break;
  case GRN_TIME_CLASSIFY_UNIT_DAY :
  case GRN_TIME_CLASSIFY_UNIT_WEEK :
    accept_interval = GRN_FALSE;
    break;
  case GRN_TIME_CLASSIFY_UNIT_MONTH :
  case GRN_TIME_CLASSIFY_UNIT_YEAR :
    accept_interval = GRN_TRUE;
    break;
  }

  if (accept_interval) {
    if (!(n_args == 1 || n_args == 2)) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s(): "
                       "wrong number of arguments (%d for 1..2)",
                       function_name,
                       n_args);
      return NULL;
    }
  } else {
    if (n_args != 1) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s(): "
                       "wrong number of arguments (%d for 1)",
                       function_name,
                       n_args);
      return NULL;
    }
  }

  grn_obj *arg0 = args[0];
  if (!func_time_classify_raw_validate_arg0(ctx, arg0, function_name)) {
    return NULL;
  }

  if (n_args == 2) {
    grn_obj *interval;
    grn_obj casted_interval;

    interval = args[1];
    if (!(interval->header.type == GRN_BULK &&
          grn_type_id_is_number_family(ctx, interval->header.domain))) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, interval);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s(): "
                       "the second argument must be a number: "
                       "<%.*s>",
                       function_name,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }

    GRN_VALUE_FIX_SIZE_INIT(&casted_interval, 0, GRN_DB_UINT32);
    grn_obj_cast(ctx, interval, &casted_interval, GRN_FALSE);
    interval_raw = GRN_UINT32_VALUE(&casted_interval);
    GRN_OBJ_FIN(ctx, &casted_interval);

    if (interval_raw == 0) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, interval);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s(): "
                       "the second argument must not be zero: "
                       "<%.*s>",
                       function_name,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
  }

  switch (arg0->header.type) {
  case GRN_BULK :
    {
      grn_obj *time = arg0;
      int64_t classed_time_raw;
      bool success = func_time_classify_raw_compute(ctx,
                                                    time,
                                                    unit,
                                                    interval_raw,
                                                    &classed_time_raw,
                                                    function_name);
      if (!success) {
        return NULL;
      }
      grn_obj *classed_time = grn_plugin_proc_alloc(ctx,
                                                    user_data,
                                                    time->header.domain,
                                                    0);
      if (!classed_time) {
        return NULL;
      }
      GRN_TIME_SET(ctx, classed_time, classed_time_raw);

      return classed_time;
    }
    break;
  case GRN_UVECTOR :
    if (arg0->header.domain == GRN_DB_TIME) {
      grn_obj *times = arg0;
      grn_obj *classed_times = grn_plugin_proc_alloc(ctx,
                                                     user_data,
                                                     times->header.domain,
                                                     GRN_OBJ_VECTOR);
      if (!classed_times) {
        return NULL;
      }

      {
        size_t i;
        size_t n_elements = GRN_TIME_VECTOR_SIZE(times);
        grn_obj time;
        GRN_TIME_INIT(&time, 0);
        for (i = 0; i < n_elements; i++) {
          GRN_BULK_REWIND(&time);
          GRN_TIME_SET(ctx, &time, GRN_TIME_VALUE_AT(times, i));

          int64_t classed_time_raw;
          bool success = func_time_classify_raw_compute(ctx,
                                                        &time,
                                                        unit,
                                                        interval_raw,
                                                        &classed_time_raw,
                                                        function_name);
          if (!success) {
            GRN_OBJ_FIN(ctx, &time);
            return NULL;
          }

          GRN_TIME_PUT(ctx, classed_times, classed_time_raw);
        }
        GRN_OBJ_FIN(ctx, &time);

        return classed_times;
      }
    }
    break;
  }

  return NULL;
}

static grn_obj *
func_time_classify_second(grn_ctx *ctx, int n_args, grn_obj **args,
                          grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_second",
                                GRN_TIME_CLASSIFY_UNIT_SECOND);
}

static grn_obj *
func_time_classify_minute(grn_ctx *ctx, int n_args, grn_obj **args,
                          grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_minute",
                                GRN_TIME_CLASSIFY_UNIT_MINUTE);
}

static grn_obj *
func_time_classify_hour(grn_ctx *ctx, int n_args, grn_obj **args,
                        grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_hour",
                                GRN_TIME_CLASSIFY_UNIT_HOUR);
}

static grn_obj *
func_time_classify_day(grn_ctx *ctx, int n_args, grn_obj **args,
                       grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_day",
                                GRN_TIME_CLASSIFY_UNIT_DAY);
}

static grn_obj *
func_time_classify_week(grn_ctx *ctx, int n_args, grn_obj **args,
                        grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_week",
                                GRN_TIME_CLASSIFY_UNIT_WEEK);
}

static grn_obj *
func_time_classify_month(grn_ctx *ctx, int n_args, grn_obj **args,
                         grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_month",
                                GRN_TIME_CLASSIFY_UNIT_MONTH);
}

static grn_obj *
func_time_classify_year(grn_ctx *ctx, int n_args, grn_obj **args,
                        grn_user_data *user_data)
{
  return func_time_classify_raw(ctx,
                                n_args,
                                args,
                                user_data,
                                "time_classify_year",
                                GRN_TIME_CLASSIFY_UNIT_YEAR);
}

static grn_obj *
func_time_classify_day_of_week(grn_ctx *ctx, int n_args, grn_obj **args,
                               grn_user_data *user_data)
{
  const char *function_name = "time_classify_day_of_week";
  grn_obj *time;

  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): "
                     "wrong number of arguments (%d for 1)",
                     function_name,
                     n_args);
    return NULL;
  }

  time = args[0];
  if (!(time->header.type == GRN_BULK &&
        time->header.domain == GRN_DB_TIME)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, time);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): "
                     "the first argument must be a time: "
                     "<%.*s>",
                     function_name,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  {
    int64_t time_raw;
    struct tm tm;
    grn_obj *day_of_week;

    time_raw = GRN_TIME_VALUE(time);
    if (!grn_time_to_tm(ctx, time_raw, &tm)) {
      return NULL;
    }

    day_of_week = grn_plugin_proc_alloc(ctx,
                                        user_data,
                                        GRN_DB_UINT8,
                                        0);
    if (!day_of_week) {
      return NULL;
    }
    GRN_UINT8_SET(ctx, day_of_week, tm.tm_wday);

    return day_of_week;
  }
}

static grn_obj *
func_time_format(grn_ctx *ctx, int n_args, grn_obj **args,
                 grn_user_data *user_data)
{
  grn_obj *time;
  grn_obj *format;

  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "time_format(): "
                     "wrong number of arguments (%d for 2)",
                     n_args);
    return NULL;
  }

  time = args[0];
  format = args[1];

  if (!(time->header.type == GRN_BULK &&
        time->header.domain == GRN_DB_TIME)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, time);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "time_format(): "
                     "the first argument must be a time: "
                     "<%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  if (!grn_obj_is_text_family_bulk(ctx, format)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, format);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "time_format(): "
                     "the second argument must be a string: "
                     "<%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  {
    int64_t time_raw;
    struct tm tm;
    grn_obj nul_terminated_format;
    grn_obj *formatted_time;
    char formatted_time_buffer[4096];
    size_t formatted_time_size;

    time_raw = GRN_TIME_VALUE(time);
    if (!grn_time_to_tm(ctx, time_raw, &tm)) {
      return NULL;
    }

    GRN_TEXT_INIT(&nul_terminated_format, 0);
    GRN_TEXT_SET(ctx,
                 &nul_terminated_format,
                 GRN_TEXT_VALUE(format),
                 GRN_TEXT_LEN(format));
    GRN_TEXT_PUTC(ctx, &nul_terminated_format, '\0');

    formatted_time_size = strftime(formatted_time_buffer,
                                   sizeof(formatted_time_buffer),
                                   GRN_TEXT_VALUE(&nul_terminated_format),
                                   &tm);
    GRN_OBJ_FIN(ctx, &nul_terminated_format);

    formatted_time = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
    GRN_TEXT_SET(ctx,
                 formatted_time,
                 formatted_time_buffer,
                 formatted_time_size);

    return formatted_time;
  }
}

static grn_obj *
func_time_format_iso8601(grn_ctx *ctx, int n_args, grn_obj **args,
                         grn_user_data *user_data)
{
  const char *function_name = "time_format_iso8601";
  grn_obj *time;

  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): "
                     "wrong number of arguments (%d for 1)",
                     function_name,
                     n_args);
    return NULL;
  }

  time = args[0];

  if (!(time->header.type == GRN_BULK &&
        time->header.domain == GRN_DB_TIME)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, time);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): "
                     "the first argument must be a time: "
                     "<%.*s>",
                     function_name,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  {
    int64_t time_raw;
    struct tm tm;
    int64_t time_sec;
    int32_t time_usec;
    grn_obj *formatted_time;

    time_raw = GRN_TIME_VALUE(time);
    if (!grn_time_to_tm(ctx, time_raw, &tm)) {
      return NULL;
    }

    GRN_TIME_UNPACK(time_raw, time_sec, time_usec);

    formatted_time = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
    if (!formatted_time) {
      return NULL;
    }

    grn_text_printf(ctx,
                    formatted_time,
                    "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec,
                    time_usec);
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    grn_text_printf(ctx,
                    formatted_time,
                    "%+03d:%02d",
                    (int32_t)(tm.tm_gmtoff / 3600),
                    abs(tm.tm_gmtoff % 3600));
#elif defined(HAVE_TIMEGM) /* HAVE_STRUCT_TM_TM_GMTOFF */
    {
      time_t gmtoff = timegm(&tm) - time_sec;
      grn_text_printf(ctx,
                      formatted_time,
                      "%+03d:%02d",
                      (int32_t)(gmtoff / 3600),
                      abs(gmtoff % 3600));
    }
#endif /* HAVE_STRUCT_TM_TM_GMTOFF */
    return formatted_time;
  }
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  grn_proc_create(ctx,
                  "time_classify_second", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_second,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_minute", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_minute,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_hour", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_hour,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_day", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_day,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_week", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_week,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_month", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_month,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_year", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_year,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_classify_day_of_week", -1,
                  GRN_PROC_FUNCTION,
                  func_time_classify_day_of_week,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx,
                  "time_format", -1,
                  GRN_PROC_FUNCTION,
                  func_time_format,
                  NULL, NULL, 0, NULL);
  grn_proc_create(ctx,
                  "time_format_iso8601", -1,
                  GRN_PROC_FUNCTION,
                  func_time_format_iso8601,
                  NULL, NULL, 0, NULL);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
