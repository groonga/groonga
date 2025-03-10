/*
  Copyright(C) 2013-2017 Brazil

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

#define GRN_CACHE_DEFAULT_MAX_N_ENTRIES 100
typedef struct _grn_cache grn_cache;

GRN_API void
grn_set_default_cache_base_path(const char *base_path);
GRN_API const char *
grn_get_default_cache_base_path(void);

/**
 * \brief Create a new cache object.
 *
 * It initializes a cache based on the current configuration or environment
 * settings. The cache operates in either memory mode or persistent mode.
 *
 * ## Cache Modes
 *
 * Memory Cache:
 * - The same cache is shared within the same process.
 * - The same cache isn't shared across multiple processes.
 *
 * Persistent Cache:
 * - The same cache is shared across multiple processes.
 * - Persistent cache is slightly slower than memory cache due to filesystem
 *   I/O.
 *
 * ## Cache Mode Configuration
 *
 * Cache mode can be configured in three ways:
 *
 * ### Server/Daemon Option
 *
 * When starting Groonga, use the `--cache-base-path` option to specify the
 * cache base path. Setting this option to a valid path enables persistent
 * cache mode.
 *
 * ### Environment Variable
 *
 * If the `--cache-base-path` option is not set when you start Groonga, the
 * function checks the `GRN_CACHE_TYPE` environment variable.
 * - If `GRN_CACHE_TYPE` is set to `persistent`, it enables persistent cache
 *   mode without a specified base path.
 * - If `GRN_CACHE_TYPE` is set to any other value or is unset, it uses
 *   memory cache mode.
 *
 * ### C API
 *
 * Use \ref grn_set_default_cache_base_path to override the configuration or
 * environment settings at runtime.
 *
 * \param ctx The context object used for memory allocation and error handling.
 *
 * \return A newly created \ref grn_cache object on success, `NULL` on error.
 *         The returned \ref grn_cache object must be freed by
 *         \ref grn_cache_close.
 *         See `ctx->rc` for error details.
 */
GRN_API grn_cache *
grn_cache_open(grn_ctx *ctx);
GRN_API grn_cache *
grn_persistent_cache_open(grn_ctx *ctx, const char *base_path);
/**
 * \brief Free resourses of the `cache`.
 *
 * \param ctx The context object.
 * \param cache The cache object to be freed.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_cache_close(grn_ctx *ctx, grn_cache *cache);

GRN_API grn_rc
grn_cache_current_set(grn_ctx *ctx, grn_cache *cache);
/**
 * \brief Retrieve the current \ref grn_cache object used in the select command.
 *
 * \note For more details about the select command, please see
 * \htmlonly
 *   <a href="https://groonga.org/docs/reference/commands/select.html">
 *     Groonga command select documentation
 *   </a>.
 * \endhtmlonly
 *
 * \param ctx The context object.
 *
 * \return The \ref grn_cache object used in the select command on success,
 *         `NULL` on error.
 */
GRN_API grn_cache *
grn_cache_current_get(grn_ctx *ctx);

GRN_API grn_rc
grn_cache_default_reopen(void);

/**
 * \brief Set the maximum number of entries in the \ref grn_cache object.
 *
 * \param ctx The context object.
 * \param cache The cache object whose maximum entry count is updated.
 * \param n The new maximum number of entries.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_cache_set_max_n_entries(grn_ctx *ctx, grn_cache *cache, unsigned int n);
/**
 * \brief Retrieve the maximum number of entries in the \ref grn_cache object.
 *
 * \param ctx The context object.
 * \param cache The cache object whose maximum entry count is to be retrieved.
 *
 * \return The maximum number of entries for \ref grn_cache object.
 */
GRN_API unsigned int
grn_cache_get_max_n_entries(grn_ctx *ctx, grn_cache *cache);

#ifdef __cplusplus
}
#endif
