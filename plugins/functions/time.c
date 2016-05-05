/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016 Brazil

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
#  define GRN_PLUGIN_FUNCTION_TAG functions_time
#endif

#include <groonga/plugin.h>

#include <math.h>

typedef enum {
  GRN_TIME_CLASSIFY_UNIT_SECOND,
  GRN_TIME_CLASSIFY_UNIT_MINUTE
} grn_time_classify_unit;

static grn_obj *
func_time_classify_raw(grn_ctx *ctx,
                       int n_args,
                       grn_obj **args,
                       grn_user_data *user_data,
                       const char *function_name,
                       grn_time_classify_unit unit)
{
  grn_obj *time;
  uint32_t interval_raw = 1;
  grn_obj *classed_time;

  if (!(n_args == 1 || n_args == 2)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): "
                     "wrong number of arguments (%d for 1..2)",
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
  }

  {
    int64_t time_raw;
    struct tm tm;
    int64_t classed_time_raw;

    time_raw = GRN_TIME_VALUE(time);
    if (!grn_time_to_tm(ctx, time_raw, &tm)) {
      return NULL;
    }

    switch (unit) {
    case GRN_TIME_CLASSIFY_UNIT_SECOND :
      tm.tm_sec = (tm.tm_sec / interval_raw) * interval_raw;
      break;
    case GRN_TIME_CLASSIFY_UNIT_MINUTE :
      tm.tm_min = (tm.tm_min / interval_raw) * interval_raw;
      tm.tm_sec = 0;
      break;
    }

    if (!grn_time_from_tm(ctx, &classed_time_raw, &tm)) {
      return NULL;
    }

    classed_time = grn_plugin_proc_alloc(ctx,
                                         user_data,
                                         time->header.domain,
                                         0);
    if (!classed_time) {
      return NULL;
    }
    GRN_TIME_SET(ctx, classed_time, classed_time_raw);

    return classed_time;
  }
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

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
