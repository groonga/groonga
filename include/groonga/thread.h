/*
  Copyright(C) 2015-2016  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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
 * \brief Get the max number of threads.
 *
 * If \ref grn_thread_get_limit_func isn't set by
 * \ref grn_thread_set_get_limit_func, it always returns `0`.
 *
 * \return The max number of threads or `0`.
 */
GRN_API uint32_t
grn_thread_get_limit(void);
/**
 * \brief Set the max number of threads.
 *
 * If \ref grn_thread_set_limit_func isn't set by
 * \ref grn_thread_set_set_limit_func, it does nothing.
 *
 * \param new_limit The new max number of threads.
 */
GRN_API void
grn_thread_set_limit(uint32_t new_limit);

GRN_API uint32_t
grn_thread_get_limit_with_ctx(grn_ctx *ctx);
GRN_API void
grn_thread_set_limit_with_ctx(grn_ctx *ctx, uint32_t new_limit);

/**
 * \brief The type of function that returns the max number of threads.
 *
 * \since 5.0.7
 *
 * This is the function pointer type for custom functions that return the
 * maximum number of threads. When set via \ref grn_thread_set_get_limit_func,
 * this function will be called by \ref grn_thread_get_limit.
 *
 * \param data User data passed when the function was registered.
 *
 * \return The max number of threads.
 */
typedef uint32_t (*grn_thread_get_limit_func)(void *data);
/**
 * \brief Set the custom function that returns the max number of threads.
 *
 * \since 5.0.7
 *
 * This function sets a custom function that will be called when
 * \ref grn_thread_get_limit is invoked. The custom function allows
 * applications to dynamically control the thread limit.
 *
 * \p data is passed to \p func when \p func is called from
 * \ref grn_thread_get_limit.
 *
 * For example usage:
 * ```c
 * static uint32_t
 * my_get_thread_limit(void *data)
 * {
 *   uint32_t *max_threads = (uint32_t *)data;
 *   return *max_threads;
 * }
 *
 * uint32_t max_threads = 100;
 * grn_thread_set_get_limit_func(my_get_thread_limit, &max_threads);
 * ```
 *
 * \param func The custom function that returns the max number of threads.
 * \param data User data to be passed to \p func when \p func is called.
 */
GRN_API void
grn_thread_set_get_limit_func(grn_thread_get_limit_func func, void *data);
typedef void (*grn_thread_set_limit_func)(uint32_t new_limit, void *data);
GRN_API void
grn_thread_set_set_limit_func(grn_thread_set_limit_func func, void *data);

typedef uint32_t (*grn_thread_get_limit_with_ctx_func)(grn_ctx *ctx,
                                                       void *data);
GRN_API void
grn_thread_set_get_limit_with_ctx_func(grn_thread_get_limit_with_ctx_func func,
                                       void *data);
typedef void (*grn_thread_set_limit_with_ctx_func)(grn_ctx *ctx,
                                                   uint32_t new_limit,
                                                   void *data);
GRN_API void
grn_thread_set_set_limit_with_ctx_func(grn_thread_set_limit_with_ctx_func func,
                                       void *data);

GRN_API grn_rc
grn_thread_dump(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif
