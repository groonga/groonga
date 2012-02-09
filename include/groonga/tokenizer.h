/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012 Brazil

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
#ifndef GRN_PLUGIN_TOKENIZER_H
#define GRN_PLUGIN_TOKENIZER_H

#include <stddef.h>

#include <groonga/plugin.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
  Don't call these functions directly. Use GRN_TOKENIZER_MALLOC() and
  GRN_TOKENIZER_FREE() instead.
 */
void *grn_tokenizer_malloc(grn_ctx *ctx, size_t size, const char *file,
                           int line, const char *func);
void grn_tokenizer_free(grn_ctx *ctx, void *ptr, const char *file,
                        int line, const char *func);

/*
  GRN_TOKENIZER_MALLOC() allocates `size' bytes and returns a pointer to the
  allocated memory space. Note that the memory space is associated with `ctx'.
 */
#define GRN_TOKENIZER_MALLOC(ctx, size) \
  grn_tokenizer_malloc((ctx), (size), __FILE__, __LINE__, __FUNCTION__)
/*
  GRN_TOKENIZER_FREE() frees a memory space allocated by
  GRN_TOKENIZER_MALLOC(). This means that `ptr' must be a pointer returned by
  GRN_TOKENIZER_MALLOC().
 */
#define GRN_TOKENIZER_FREE(ctx, ptr) \
  grn_tokenizer_free((ctx), (ptr), __FILE__, __LINE__, __FUNCTION__)

/*
  GRN_TOKENIZER_LOG() reports a log of `level'. Its error message is generated
  from `format' and the varying number of arguments. See grn_log_level in
  "groonga.h" for more details of `level'.
 */
#define GRN_TOKENIZER_LOG(ctx, level, format, ...) \
  GRN_LOG((ctx), (level), (format), ## __VA_ARGS__)

/*
  Don't call grn_tokenizer_set_error() directly. This function is used in
  GRN_TOKENIZER_SET_ERROR().
 */
void grn_tokenizer_set_error(grn_ctx *ctx, grn_log_level level,
                             grn_rc error_code,
                             const char *file, int line, const char *func,
                             const char *format, ...);

/*
  Don't call these functions directly. grn_tokenizer_backtrace() and
  grn_tokenizer_logtrace() are used in GRN_TOKENIZER_SET_ERROR().
 */
void grn_tokenizer_backtrace(grn_ctx *ctx);
void grn_tokenizer_logtrace(grn_ctx *ctx, grn_log_level level);

/*
  Don't use GRN_TOKENIZER_SET_ERROR() directly. This macro is used in
  GRN_TOKENIZER_ERROR().
 */
#define GRN_TOKENIZER_SET_ERROR(ctx, level, error_code, format, ...) do { \
  grn_tokenizer_set_error(ctx, level, error_code, \
                          __FILE__, __LINE__, __FUNCTION__, \
                          format, ## __VA_ARGS__); \
  GRN_LOG(ctx, level, format, ## __VA_ARGS__); \
  grn_tokenizer_backtrace(ctx); \
  grn_tokenizer_logtrace(ctx, level); \
} while (0)

/*
  GRN_TOKENIZER_ERROR() reports an error of `error_code'. Its error message is
  generated from `format' and the varying number of arguments. See grn_rc in
  "groonga.h" for more details of `error_code'.
 */
#define GRN_TOKENIZER_ERROR(ctx, error_code, format, ...) \
  GRN_TOKENIZER_SET_ERROR(ctx, GRN_LOG_ERROR, error_code, \
                          format, ## __VA_ARGS__)

/*
  grn_tokenizer_mutex is available to make a critical section. See the
  following functions.
 */
typedef struct _grn_tokenizer_mutex grn_tokenizer_mutex;

/*
  grn_tokenizer_mutex_create() returns a pointer to a new object of
  grn_tokenizer_mutex. Memory for the new object is obtained with
  GRN_TOKENIZER_MALLOC(). grn_tokenizer_mutex_create() returns NULL if
  sufficient memory is not available.
 */
grn_tokenizer_mutex *grn_tokenizer_mutex_create(grn_ctx *ctx);

/*
  grn_tokenizer_mutex_destroy() finalizes an object of grn_tokenizer_mutex
  and then frees memory allocated for that object.
 */
void grn_tokenizer_mutex_destroy(grn_ctx *ctx, grn_tokenizer_mutex *mutex);

/*
  grn_tokenizer_mutex_lock() locks a mutex object. If the object is already
  locked, the calling thread waits until the object will be unlocked.
 */
void grn_tokenizer_mutex_lock(grn_ctx *ctx, grn_tokenizer_mutex *mutex);

/*
  grn_tokenizer_mutex_unlock() unlocks a mutex object.
  grn_tokenizer_mutex_unlock() should not be called for an unlocked object.
 */
void grn_tokenizer_mutex_unlock(grn_ctx *ctx, grn_tokenizer_mutex *mutex);

/*
  grn_tokenizer_charlen() returns the length (#bytes) of the first character
  in the string specified by `str_ptr' and `str_length'. If the starting bytes
  are invalid as a character, grn_tokenizer_charlen() returns 0. See
  grn_encoding in "groonga.h" for more details of `encoding'
 */
int grn_tokenizer_charlen(grn_ctx *ctx, const char *str_ptr,
                          unsigned int str_length, grn_encoding encoding);

/*
  grn_tokenizer_isspace() returns the length (#bytes) of the first character
  in the string specified by `str_ptr' and `str_length' if it is a space
  character. Otherwise, grn_tokenizer_isspace() returns 0.
 */
int grn_tokenizer_isspace(grn_ctx *ctx, const char *str_ptr,
                          unsigned int str_length, grn_encoding encoding);

/*
  grn_tokenizer_query is a structure for storing a query. See the following
  functions.
 */
typedef struct _grn_tokenizer_query grn_tokenizer_query;

struct _grn_tokenizer_query {
  grn_str *str;
  const char *ptr;
  unsigned int length;
  grn_encoding encoding;
};

/*
  grn_tokenizer_query_create() parses `args' and returns a new object of
  grn_tokenizer_query. The new object stores information of the query.
  grn_tokenizer_query_create() normalizes the query if the target table
  requires normalization. grn_tokenizer_query_create() returns NULL if
  something goes wrong. Note that grn_tokenizer_query_create() must be called
  just once in the function that initializes a tokenizer.
 */
grn_tokenizer_query *grn_tokenizer_query_create(grn_ctx *ctx,
                                                int num_args, grn_obj **args);

/*
  grn_tokenizer_mutex_destroy() finalizes an object of grn_tokenizer_mutex
  and then frees memory allocated for that object.
 */
void grn_tokenizer_query_destroy(grn_ctx *ctx, grn_tokenizer_query *query);

/*
  grn_tokenizer_token is needed to return tokens. A grn_tokenizer_token object
  stores a token to be returned and it must be maintained until a request for
  next token or finalization comes.
 */
typedef struct _grn_tokenizer_token grn_tokenizer_token;

struct _grn_tokenizer_token {
  grn_obj str;
  grn_obj status;
};

/*
  grn_tokenizer_token_init() initializes `token'. Note that an initialized
  object must be finalized by grn_tokenizer_token_fin().
 */
void grn_tokenizer_token_init(grn_ctx *ctx, grn_tokenizer_token *token);

/*
  grn_tokenizer_token_fin() finalizes `token' that has been initialized by
  grn_tokenizer_token_init().
 */
void grn_tokenizer_token_fin(grn_ctx *ctx, grn_tokenizer_token *token);

/*
  grn_tokenizer_status provides a list of tokenizer status codes.
  GRN_TOKENIZER_CONTINUE means that the next token is not the last one and
  GRN_TOKENIZER_LAST means that the next token is the last one. If a document
  or query contains no tokens, please push an empty string with
  GRN_TOKENIZER_LAST as a token.
 */
typedef enum _grn_tokenizer_status grn_tokenizer_status;

enum _grn_tokenizer_status {
  GRN_TOKENIZER_CONTINUE = 0,
  GRN_TOKENIZER_LAST     = 1
};

/*
  grn_tokenizer_token_push() pushes the next token in `*token'. Note that
  grn_tokenizer_token_push() does not make a copy of the given string. This
  means that you have to maintain a memory space allocated to the string.
  Also note that the grn_tokenizer_token object must be maintained until the
  request for the next token or finalization comes. See grn_tokenizer_status in
  this header for more details of `status'.
 */
void grn_tokenizer_token_push(grn_ctx *ctx, grn_tokenizer_token *token,
                              const char *str_ptr, unsigned int str_length,
                              grn_tokenizer_status status);

/*
  grn_tokenizer_register() registers a plugin to the database which is
  associated with `ctx'. `plugin_name_ptr' and `plugin_name_length' specify the
  plugin name. Alphabetic letters ('A'-'Z' and 'a'-'z'), digits ('0'-'9') and
  an underscore ('_') are capable characters. `init', `next' and `fin' specify
  the plugin functions. `init' is called for initializing a tokenizer for a
  document or query. `next' is called for extracting tokens one by one. `fin'
  is called for finalizing a tokenizer. grn_tokenizer_register() returns
  GRN_SUCCESS on success, an error code on failure. See "groonga.h" for more
  details of grn_proc_func and grn_user_data, that is used as an argument of
  grn_proc_func.
 */
grn_rc grn_tokenizer_register(grn_ctx *ctx, const char *plugin_name_ptr,
                              unsigned int plugin_name_length,
                              grn_proc_func *init, grn_proc_func *next,
                              grn_proc_func *fin);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif  /* GRN_PLUGIN_TOKENIZER_H */
