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

static double grn_slow_log_threshold = 0.0;

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

  grn_timeval start_time;
  grn_timeval_now(ctx, &start_time);
  GRN_TIME_PUT(ctx,
               &(ctx->impl->slow_log.start_times),
               GRN_TIME_PACK(start_time.tv_sec,
                             GRN_TIME_NSEC_TO_USEC(start_time.tv_nsec)));

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

  int64_t packed_start_time;
  GRN_TIME_POP(start_times, packed_start_time);
  grn_timeval end_time;
  grn_timeval_now(ctx, &end_time);
  int64_t packed_end_time =
    GRN_TIME_PACK(end_time.tv_sec,
                  GRN_TIME_NSEC_TO_USEC(end_time.tv_nsec));
  int64_t packed_elapsed_time = packed_end_time - packed_start_time;
  int64_t elapsed_time_sec;
  int32_t elapsed_time_usec;
  GRN_TIME_UNPACK(packed_elapsed_time, elapsed_time_sec, elapsed_time_usec);
  double elapsed_time =
    (double)elapsed_time_sec +
    GRN_TIME_USEC_TO_SEC((double)(elapsed_time_usec));

  GRN_API_RETURN(elapsed_time);
}

bool
grn_slow_log_is_slow(grn_ctx *ctx, double elapsed_time)
{
  return grn_slow_log_threshold > 0.0 && elapsed_time >= grn_slow_log_threshold;
}
