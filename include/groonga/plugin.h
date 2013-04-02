/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2010-2012 Brazil

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
#ifndef GRN_PLUGIN_H
#define GRN_PLUGIN_H

#include <stddef.h>

#include <groonga.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_PLUGIN_INIT grn_plugin_impl_init
#define GRN_PLUGIN_REGISTER grn_plugin_impl_register
#define GRN_PLUGIN_FIN grn_plugin_impl_fin

#if defined(_WIN32) || defined(_WIN64)
#  define GRN_PLUGIN_EXPORT __declspec(dllexport)
#else /* defined(_WIN32) || defined(_WIN64) */
#  define GRN_PLUGIN_EXPORT
#endif /* defined(_WIN32) || defined(_WIN64) */

GRN_PLUGIN_EXPORT grn_rc GRN_PLUGIN_INIT(grn_ctx *ctx);
GRN_PLUGIN_EXPORT grn_rc GRN_PLUGIN_REGISTER(grn_ctx *ctx);
GRN_PLUGIN_EXPORT grn_rc GRN_PLUGIN_FIN(grn_ctx *ctx);

/*
  Don't call these functions directly. Use GRN_PLUGIN_MALLOC(),
  GRN_PLUGIN_REALLOC() and GRN_PLUGIN_FREE() instead.
 */
GRN_API void *grn_plugin_malloc(grn_ctx *ctx, size_t size, const char *file,
                                int line, const char *func);
GRN_API void *grn_plugin_realloc(grn_ctx *ctx, void *ptr, size_t size,
                                 const char *file, int line, const char *func);
GRN_API void grn_plugin_free(grn_ctx *ctx, void *ptr, const char *file,
                             int line, const char *func);

/*
  GRN_PLUGIN_MALLOC() allocates `size' bytes and returns a pointer to the
  allocated memory space. Note that the memory space is associated with `ctx'.
 */
#define GRN_PLUGIN_MALLOC(ctx, size) \
  grn_plugin_malloc((ctx), (size), __FILE__, __LINE__, __FUNCTION__)
/*
  GRN_PLUGIN_REALLOC() resizes the memory space pointed to by `ptr' or
  allocates a new memory space of `size' bytes. GRN_PLUGIN_REALLOC() returns
  a pointer to the memory space. The contents is unchanged or copied from the
  old memory space to the new memory space.
 */
#define GRN_PLUGIN_REALLOC(ctx, ptr, size) \
  grn_plugin_realloc((ctx), (ptr), (size), __FILE__, __LINE__, __FUNCTION__)
/*
  GRN_PLUGIN_FREE() frees a memory space allocated by GRN_PLUGIN_MALLOC() or
  GRN_PLUGIN_REALLOC(). This means that `ptr' must be a pointer returned by
  GRN_PLUGIN_MALLOC() or GRN_PLUGIN_REALLOC().
 */
#define GRN_PLUGIN_FREE(ctx, ptr) \
  grn_plugin_free((ctx), (ptr), __FILE__, __LINE__, __FUNCTION__)

/*
  GRN_PLUGIN_LOG() reports a log of `level'. Its error message is generated
  from the varying number of arguments, in which the first one is the format
  string and the rest are its arguments. See grn_log_level in "groonga.h" for
  more details of `level'.
 */
#define GRN_PLUGIN_LOG(ctx, level, ...) \
  GRN_LOG((ctx), (level), __VA_ARGS__)

/*
  Don't call grn_plugin_set_error() directly. This function is used in
  GRN_PLUGIN_SET_ERROR().
 */
GRN_API void grn_plugin_set_error(grn_ctx *ctx, grn_log_level level,
                                  grn_rc error_code,
                                  const char *file, int line, const char *func,
                                  const char *format, ...) GRN_ATTRIBUTE_PRINTF(7);

/*
  Don't call these functions directly. grn_plugin_backtrace() and
  grn_plugin_logtrace() are used in GRN_PLUGIN_SET_ERROR().
 */
GRN_API void grn_plugin_backtrace(grn_ctx *ctx);
GRN_API void grn_plugin_logtrace(grn_ctx *ctx, grn_log_level level);

/*
  Don't use GRN_PLUGIN_SET_ERROR() directly. This macro is used in
  GRN_PLUGIN_ERROR().
 */
#define GRN_PLUGIN_SET_ERROR(ctx, level, error_code, ...) do { \
  grn_plugin_set_error(ctx, level, error_code, \
                       __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
  GRN_LOG(ctx, level, __VA_ARGS__); \
  grn_plugin_backtrace(ctx); \
  grn_plugin_logtrace(ctx, level); \
} while (0)

/*
  GRN_PLUGIN_ERROR() reports an error of `error_code'. Its error message is
  generated from the varying number of arguments, in which the first one is the
  format string and the rest are its arguments. See grn_rc in "groonga.h" for
  more details of `error_code'.
 */
#define GRN_PLUGIN_ERROR(ctx, error_code, ...) \
  GRN_PLUGIN_SET_ERROR(ctx, GRN_LOG_ERROR, error_code, __VA_ARGS__)

/*
  grn_plugin_mutex is available to make a critical section. See the
  following functions.
 */
typedef struct _grn_plugin_mutex grn_plugin_mutex;

/*
  grn_plugin_mutex_open() returns a pointer to a new object of
  grn_plugin_mutex. Memory for the new object is obtained with
  GRN_PLUGIN_MALLOC(). grn_plugin_mutex_open() returns NULL if sufficient
  memory is not available.
 */
GRN_API grn_plugin_mutex *grn_plugin_mutex_open(grn_ctx *ctx);

/*
  grn_plugin_mutex_create() is deprecated. Use grn_plugin_mutex_open()
  instead.
*/
GRN_API grn_plugin_mutex *grn_plugin_mutex_create(grn_ctx *ctx);

/*
  grn_plugin_mutex_close() finalizes an object of grn_plugin_mutex and then
  frees memory allocated for that object.
 */
GRN_API void grn_plugin_mutex_close(grn_ctx *ctx, grn_plugin_mutex *mutex);

/*
  grn_plugin_mutex_destroy() is deprecated. Use grn_plugin_mutex_close()
  instead.
*/
GRN_API void grn_plugin_mutex_destroy(grn_ctx *ctx, grn_plugin_mutex *mutex);

/*
  grn_plugin_mutex_lock() locks a mutex object. If the object is already
  locked, the calling thread waits until the object will be unlocked.
 */
GRN_API void grn_plugin_mutex_lock(grn_ctx *ctx, grn_plugin_mutex *mutex);

/*
  grn_plugin_mutex_unlock() unlocks a mutex object. grn_plugin_mutex_unlock()
  should not be called for an unlocked object.
 */
GRN_API void grn_plugin_mutex_unlock(grn_ctx *ctx, grn_plugin_mutex *mutex);

/*
  grn_plugin_proc_alloc() allocates a `grn_obj` object.
  You can use it in function that is registered as GRN_PROC_FUNCTION.
 */
GRN_API grn_obj *grn_plugin_proc_alloc(grn_ctx *ctx, grn_user_data *user_data,
                                       grn_id domain, grn_obj_flags flags);

/*
  grn_plugin_win32_base_dir() returns the groonga install directory.
  The install directory is computed from the directory that has
  `groonga.dll`. You can use the directory to generate install
  directory aware path.

  It only works on Windows. It returns `NULL` on other platforms.
 */
GRN_API const char *grn_plugin_win32_base_dir(void);

/*
  grn_plugin_charlen() returns the length (#bytes) of the first character
  in the string specified by `str_ptr' and `str_length'. If the starting bytes
  are invalid as a character, grn_plugin_charlen() returns 0. See
  grn_encoding in "groonga.h" for more details of `encoding'
 */
GRN_API int grn_plugin_charlen(grn_ctx *ctx, const char *str_ptr,
                               unsigned int str_length, grn_encoding encoding);

/*
  grn_plugin_isspace() returns the length (#bytes) of the first character
  in the string specified by `str_ptr' and `str_length' if it is a space
  character. Otherwise, grn_plugin_isspace() returns 0.
 */
GRN_API int grn_plugin_isspace(grn_ctx *ctx, const char *str_ptr,
                               unsigned int str_length, grn_encoding encoding);



#ifdef __cplusplus
}
#endif

#endif /* GRN_PLUGIN_H */
