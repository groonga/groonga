/*
  Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
  GRN_PROGRESS_INDEX,
} grn_progress_type;

typedef enum {
  GRN_PROGRESS_INDEX_INVALID,
  GRN_PROGRESS_INDEX_INITIALIZE,
  GRN_PROGRESS_INDEX_LOAD,
  GRN_PROGRESS_INDEX_COMMIT,
  GRN_PROGRESS_INDEX_FINALIZE,
  GRN_PROGRESS_INDEX_DONE,
} grn_progress_index_phase;

typedef struct _grn_progress grn_progress;

GRN_API grn_progress_type
grn_progress_get_type(grn_ctx *ctx,
                      grn_progress *progress);
GRN_API grn_progress_index_phase
grn_progress_index_get_phase(grn_ctx *ctx,
                             grn_progress *progress);
GRN_API uint32_t
grn_progress_index_get_n_target_records(grn_ctx *ctx,
                                        grn_progress *progress);
GRN_API uint32_t
grn_progress_index_get_n_processed_records(grn_ctx *ctx,
                                           grn_progress *progress);
GRN_API uint32_t
grn_progress_index_get_n_target_terms(grn_ctx *ctx,
                                      grn_progress *progress);
GRN_API uint32_t
grn_progress_index_get_n_processed_terms(grn_ctx *ctx,
                                         grn_progress *progress);

typedef void (*grn_progress_callback_func)(grn_ctx *ctx,
                                           grn_progress *progress,
                                           void *user_data);

GRN_API grn_rc
grn_ctx_set_progress_callback(grn_ctx *ctx,
                              grn_progress_callback_func func,
                              void *user_data);
GRN_API grn_progress_callback_func
grn_ctx_get_progress_callback(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif
