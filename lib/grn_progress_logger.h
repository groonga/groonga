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

#pragma once

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct grn_progress_logger {
  const char *tag;
  const char *targets_label;
  uint32_t n_targets;
  uint32_t n_processed_targets;
  uint32_t previous_n_processed_targets;
  uint32_t last_logged_n_processed_targets;
  uint32_t interval;
  uint32_t n_targets_digits;
  grn_log_level log_level;
  grn_timeval start_time;
  grn_timeval previous_time;
} grn_progress_logger;

void
grn_progress_logger_init(grn_ctx *ctx,
                         grn_progress_logger *logger,
                         const char *tag,
                         const char *targets_label,
                         uint32_t n_targets);
void
grn_progress_logger_fin(grn_ctx *ctx, grn_progress_logger *logger);
void
grn_progress_logger_log(grn_ctx *ctx, grn_progress_logger *logger);

#ifdef __cplusplus
}
#endif
