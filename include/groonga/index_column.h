/*
  Copyright (C) 2019  Brazil
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Compute differences between expected index content and
 *        actual index content.
 *
 * See \ref grn_index_column_diff_full() for details. This uses the
 * default options.
 *
 * \param ctx The context object.
 * \param index_column The target index column.
 * \param diff The result. This is an output parameter.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 */
GRN_API grn_rc
grn_index_column_diff(grn_ctx *ctx, grn_obj *index_column, grn_obj **diff);

/**
 * \brief Options for \ref grn_index_column_diff().
 *
 * \since 14.1.3
 */
typedef struct grn_index_column_diff_options grn_index_column_diff_options;

/**
 * \brief Create a new options.
 *
 * \param ctx The context object.
 *
 * \return The newly created options on success, `NULL` on error.
 *
 *         See `ctx->rc` for error details.
 *
 * \since 14.1.3
 */
GRN_API grn_index_column_diff_options *
grn_index_column_diff_options_open(grn_ctx *ctx);

/**
 * \brief Free the options.
 *
 * \param ctx The context object.
 * \param options The options to be freed.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 14.1.3
 */
GRN_API grn_rc
grn_index_column_diff_options_close(grn_ctx *ctx,
                                    grn_index_column_diff_options *options);

/**
 * \brief Get the log level that is used for progress logs.
 *
 * \param ctx The context object.
 * \param options The target options.
 *
 * \return The log level to be used for progress logs. If `options` is
 *         `NULL`, the default log level is returned.
 *
 * \since 14.1.3
 */
GRN_API grn_log_level
grn_index_column_diff_options_get_progress_log_level(
  grn_ctx *ctx, grn_index_column_diff_options *options);

/**
 * \brief Set the log level that is used for progress logs.
 *
 * \param ctx The context object.
 * \param options The target options. Must not `NULL`.
 * \param level The log level for progress logs.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 14.1.3
 */
GRN_API grn_rc
grn_index_column_diff_options_set_progress_log_level(
  grn_ctx *ctx, grn_index_column_diff_options *options, grn_log_level level);

/**
 * \brief Compute differences between expected index content and
 *        actual index content with options.
 *
 * You can use this to check whether the target index column is broken
 * or not.
 *
 * Expected index content is computed from the sources. So you can't
 * update the sources while this is executing. If you update the
 * sources, the result of this may be incorrect.
 *
 * \param ctx The context object.
 * \param index_column The target index column.
 * \param options The target options.
 * \param diff The result. This is an output parameter.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 14.1.3
 */
GRN_API grn_rc
grn_index_column_diff_full(grn_ctx *ctx,
                           grn_obj *index_column,
                           grn_index_column_diff_options *options,
                           grn_obj **diff);

GRN_API bool
grn_index_column_is_usable(grn_ctx *ctx,
                           grn_obj *index_column,
                           grn_operator op);

#ifdef __cplusplus
}
#endif
