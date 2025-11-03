/*
  Copyright (C) 2019-2025  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_progress_logger.h"

#include "grn_float.h"

#include <math.h>

static const grn_log_level grn_progress_logger_log_level_default =
  GRN_LOG_DEBUG;

void
grn_progress_logger_init(grn_ctx *ctx,
                         grn_progress_logger *logger,
                         const char *tag,
                         const char *targets_label,
                         uint32_t n_targets)
{
  logger->tag = tag;                     /* Don't own this. */
  logger->targets_label = targets_label; /* Don't own this. */
  logger->n_targets = n_targets;
  logger->n_processed_targets = 0;
  logger->previous_n_processed_targets = 0;
  logger->last_logged_n_processed_targets = 0;
  logger->interval = 1000;
  logger->log_level = grn_progress_logger_log_level_default;
  double n_targets_digits = ceil(log10(logger->n_targets + 1));
  logger->n_targets_digits = (uint32_t)n_targets_digits;
  grn_timeval_now(ctx, &(logger->start_time));
  logger->previous_time = logger->start_time;
}

void
grn_progress_logger_fin(grn_ctx *ctx, grn_progress_logger *logger)
{
}

static double
grn_progress_logger_format_time(grn_ctx *ctx, double seconds, const char **unit)
{
  if (seconds < 60) {
    *unit = "s";
    return seconds;
  } else if (seconds < (60 * 60)) {
    *unit = "m";
    return seconds / 60;
  } else if (seconds < (60 * 60 * 24)) {
    *unit = "h";
    return seconds / 60 / 60;
  } else {
    *unit = "d";
    return seconds / 60 / 60 / 24;
  }
}

static double
grn_progress_logger_format_memory(grn_ctx *ctx,
                                  uint64_t usage,
                                  const char **unit)
{
  if (usage < 1024) {
    *unit = "B";
    return (double)usage;
  } else if (usage < (1024 * 1024)) {
    *unit = "KiB";
    return (double)usage / 1024.0;
  } else if (usage < (1024 * 1024 * 1024)) {
    *unit = "MiB";
    return (double)usage / 1024.0 / 1024.0;
  } else {
    *unit = "GiB";
    return (double)usage / 1024.0 / 1024.0 / 1024.0;
  }
}

static void
grn_progress_logger_adjust_interval(grn_ctx *ctx, grn_progress_logger *logger)
{
  if (logger->n_processed_targets >= 100000) {
    logger->interval = 100000;
  } else if (logger->n_processed_targets >= 10000) {
    logger->interval = 10000;
  } else if (logger->n_processed_targets >= 5000) {
    logger->interval = 5000;
  }
}

void
grn_progress_logger_log(grn_ctx *ctx, grn_progress_logger *logger)
{
  if (!grn_logger_pass(ctx, logger->log_level)) {
    return;
  }

  const uint32_t n_processed_targets = logger->n_processed_targets;
  const uint32_t previous_n_processed_targets =
    logger->previous_n_processed_targets;
  const uint32_t interval = logger->interval;
  const uint32_t n_targets = logger->n_targets;
  logger->previous_n_processed_targets = logger->n_processed_targets;
  if (n_processed_targets != n_targets) {
    if (n_processed_targets == previous_n_processed_targets + 1) {
      /* Step by step logging. */
      if ((n_processed_targets % interval) != 0) {
        return;
      }
    } else {
      /* Bulk logging. */
      if (n_processed_targets <
          logger->last_logged_n_processed_targets + interval) {
        return;
      }
    }
  }

  grn_timeval current_time;
  grn_timeval_now(ctx, &current_time);
  const grn_timeval *start_time = &(logger->start_time);
  const grn_timeval *previous_time = &(logger->previous_time);
  const double elapsed_seconds =
    ((double)(current_time.tv_sec) +
     current_time.tv_nsec / GRN_TIME_NSEC_PER_SEC_F) -
    ((double)(start_time->tv_sec) +
     start_time->tv_nsec / GRN_TIME_NSEC_PER_SEC_F);
  const double current_interval_seconds =
    ((double)(current_time.tv_sec) +
     current_time.tv_nsec / GRN_TIME_NSEC_PER_SEC_F) -
    ((double)(previous_time->tv_sec) +
     previous_time->tv_nsec / GRN_TIME_NSEC_PER_SEC_F);
  double throughput;
  if (grn_float_is_zero(current_interval_seconds)) {
    throughput = interval;
  } else {
    throughput = interval / current_interval_seconds;
  }
  const double remained_seconds =
    elapsed_seconds + ((n_targets - n_processed_targets) / throughput);
  const char *elapsed_unit = NULL;
  const double elapsed_time =
    grn_progress_logger_format_time(ctx, elapsed_seconds, &elapsed_unit);
  const char *remained_unit = NULL;
  const double remained_time =
    grn_progress_logger_format_time(ctx, remained_seconds, &remained_unit);
  const char *interval_unit = NULL;
  const double interval_time =
    grn_progress_logger_format_time(ctx,
                                    current_interval_seconds,
                                    &interval_unit);
  const char *memory_unit = NULL;
  const double memory_usage =
    grn_progress_logger_format_memory(ctx,
                                      grn_memory_get_usage(ctx),
                                      &memory_unit);

  GRN_LOG(ctx,
          logger->log_level,
          "%s %*u/%u %3.0f%% %.2f%s/%.2f%s %.2f%s(%.2f%s/s) %.2f%s",
          logger->tag,
          logger->n_targets_digits,
          n_processed_targets,
          n_targets,
          ((double)n_processed_targets / (double)n_targets) * 100,
          elapsed_time,
          elapsed_unit,
          remained_time,
          remained_unit,
          interval_time,
          interval_unit,
          throughput,
          logger->targets_label,
          memory_usage,
          memory_unit);
  logger->previous_time = current_time;
  logger->last_logged_n_processed_targets = n_processed_targets;
  grn_progress_logger_adjust_interval(ctx, logger);
}
