/*
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_slow_log.h"

#ifdef _WIN32
static int64_t grn_slow_log_performance_frequency = 1;
#endif

static double grn_slow_log_threshold = 0.0;

void
grn_slow_log_init(void)
{
#ifdef _WIN32
  {
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency)) {
      grn_slow_log_performance_frequency = frequency.QuadPart;
    }
  }
#endif
}

void
grn_slow_log_init_from_env(void)
{
  {
    char grn_slow_log_threshold_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_SLOW_LOG_THRESHOLD",
               grn_slow_log_threshold_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_slow_log_threshold_env[0]) {
      grn_slow_log_threshold = atof(grn_slow_log_threshold_env);
    }
  }
}

grn_rc
grn_slow_log_push(grn_ctx *ctx)
{
  const char *tag = "[slow-log][push]";

  if (grn_slow_log_threshold <= 0.0) {
    return GRN_SUCCESS;
  }

  GRN_API_ENTER;

  if (!ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT, "%s not initialized grn_ctx", tag);
    GRN_API_RETURN(ctx->rc);
  }

#ifdef _WIN32
  {
    int64_t start_counter = 0;
    LARGE_INTEGER counter;
    if (QueryPerformanceCounter(&counter)) {
      start_counter = counter.QuadPart;
    }
    GRN_INT64_PUT(ctx, &(ctx->impl->slow_log.start_times), start_counter);
  }
#else
  {
    grn_timeval now;
    grn_timeval_now(ctx, &now);
    int64_t packed_start_time =
       GRN_TIME_PACK(now.tv_sec,
                     GRN_TIME_NSEC_TO_USEC(now.tv_nsec));
    GRN_INT64_PUT(ctx, &(ctx->impl->slow_log.start_times), packed_start_time);
  }
#endif

  GRN_API_RETURN(ctx->rc);
}

double
grn_slow_log_pop(grn_ctx *ctx)
{
  const char *tag = "[slow-log][pop]";

  if (grn_slow_log_threshold <= 0.0) {
    return 0.0;
  }

  GRN_API_ENTER;

  if (!ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT, "%s not initialized grn_ctx", tag);
    GRN_API_RETURN(0.0);
  }

  grn_obj *start_times = &(ctx->impl->slow_log.start_times);
  if (GRN_TIME_VECTOR_SIZE(start_times) == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s unbalanced grn_slow_log_push()/grn_slow_log_pop() calls",
        tag);
    GRN_API_RETURN(0.0);
  }

  double elapsed_time;
#ifdef _WIN32
  {
    int64_t start_counter;
    GRN_INT64_POP(start_times, start_counter);
    int64_t end_counter = 0;
    LARGE_INTEGER counter;
    if (QueryPerformanceCounter(&counter)) {
      end_counter = counter.QuadPart;
    }
    elapsed_time =
      (double)(end_counter - start_counter) /
      (double)grn_slow_log_performance_frequency;
  }
#else
  {
    int64_t packed_start_time;
    GRN_INT64_POP(start_times, packed_start_time);
    grn_timeval now;
    grn_timeval_now(ctx, &now);
    int64_t packed_end_time = GRN_TIME_PACK(now.tv_sec,
                                            GRN_TIME_NSEC_TO_USEC(now.tv_nsec));
    int64_t packed_elapsed_time = packed_end_time - packed_start_time;
    int64_t elapsed_time_sec;
    int32_t elapsed_time_usec;
    GRN_TIME_UNPACK(packed_elapsed_time, elapsed_time_sec, elapsed_time_usec);
    elapsed_time =
      (double)elapsed_time_sec +
      GRN_TIME_USEC_TO_SEC((double)(elapsed_time_usec));
  }
#endif

  GRN_API_RETURN(elapsed_time);
}

bool
grn_slow_log_is_slow(grn_ctx *ctx, double elapsed_time)
{
  return grn_slow_log_threshold > 0.0 && elapsed_time >= grn_slow_log_threshold;
}
