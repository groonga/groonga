/*
  Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_progress.h"

grn_progress_type
grn_progress_get_type(grn_ctx *ctx, grn_progress *progress)
{
  return progress->type;
}

grn_progress_index_phase
grn_progress_index_get_phase(grn_ctx *ctx, grn_progress *progress)
{
  if (progress->type != GRN_PROGRESS_INDEX) {
    return 0;
  }
  return progress->value.index.phase;
}

uint32_t
grn_progress_index_get_n_target_records(grn_ctx *ctx, grn_progress *progress)
{
  if (progress->type != GRN_PROGRESS_INDEX) {
    return 0;
  }
  return progress->value.index.n_target_records;
}

uint32_t
grn_progress_index_get_n_processed_records(grn_ctx *ctx, grn_progress *progress)
{
  if (progress->type != GRN_PROGRESS_INDEX) {
    return 0;
  }
  return progress->value.index.n_processed_records;
}

uint32_t
grn_progress_index_get_n_target_terms(grn_ctx *ctx, grn_progress *progress)
{
  if (progress->type != GRN_PROGRESS_INDEX) {
    return 0;
  }
  return progress->value.index.n_target_terms;
}

uint32_t
grn_progress_index_get_n_processed_terms(grn_ctx *ctx, grn_progress *progress)
{
  if (progress->type != GRN_PROGRESS_INDEX) {
    return 0;
  }
  return progress->value.index.n_processed_terms;
}
