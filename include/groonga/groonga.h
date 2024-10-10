/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GRN_API
#  ifdef GRN_STATIC
#    define GRN_API
#  else
#    if defined(_WIN32) || defined(_WIN64)
#      define GRN_API __declspec(dllimport)
#    else
#      define GRN_API
#    endif /* defined(_WIN32) || defined(_WIN64) */
#  endif
#endif /* GRN_API */

typedef uint32_t grn_id;
/* Deprecated since 9.0.2. Use bool directly. */
typedef bool grn_bool;

#ifdef GRN_HAVE_BFLOAT16
typedef __bf16 grn_bfloat16;
#endif

/**
 * \brief Means "does not exist"
 */
#define GRN_ID_NIL (0x00)
#define GRN_ID_MAX (0x3fffffff)

#define GRN_TRUE   true
#define GRN_FALSE  false

typedef enum {
  /// Success (0)
  GRN_SUCCESS = 0,
  GRN_END_OF_DATA = 1,
  GRN_UNKNOWN_ERROR = -1,
  GRN_OPERATION_NOT_PERMITTED = -2,
  GRN_NO_SUCH_FILE_OR_DIRECTORY = -3,
  GRN_NO_SUCH_PROCESS = -4,
  GRN_INTERRUPTED_FUNCTION_CALL = -5,
  GRN_INPUT_OUTPUT_ERROR = -6,
  GRN_NO_SUCH_DEVICE_OR_ADDRESS = -7,
  GRN_ARG_LIST_TOO_LONG = -8,
  GRN_EXEC_FORMAT_ERROR = -9,
  GRN_BAD_FILE_DESCRIPTOR = -10,
  GRN_NO_CHILD_PROCESSES = -11,
  GRN_RESOURCE_TEMPORARILY_UNAVAILABLE = -12,
  GRN_NOT_ENOUGH_SPACE = -13,
  GRN_PERMISSION_DENIED = -14,
  GRN_BAD_ADDRESS = -15,
  GRN_RESOURCE_BUSY = -16,
  GRN_FILE_EXISTS = -17,
  GRN_IMPROPER_LINK = -18,
  GRN_NO_SUCH_DEVICE = -19,
  GRN_NOT_A_DIRECTORY = -20,
  GRN_IS_A_DIRECTORY = -21,
  /// Invalid function argument (-22)
  GRN_INVALID_ARGUMENT = -22,
  GRN_TOO_MANY_OPEN_FILES_IN_SYSTEM = -23,
  GRN_TOO_MANY_OPEN_FILES = -24,
  GRN_INAPPROPRIATE_I_O_CONTROL_OPERATION = -25,
  GRN_FILE_TOO_LARGE = -26,
  GRN_NO_SPACE_LEFT_ON_DEVICE = -27,
  GRN_INVALID_SEEK = -28,
  GRN_READ_ONLY_FILE_SYSTEM = -29,
  GRN_TOO_MANY_LINKS = -30,
  GRN_BROKEN_PIPE = -31,
  GRN_DOMAIN_ERROR = -32,
  GRN_RESULT_TOO_LARGE = -33,
  GRN_RESOURCE_DEADLOCK_AVOIDED = -34,
  /// Not enough memory available (-35)
  GRN_NO_MEMORY_AVAILABLE = -35,
  GRN_FILENAME_TOO_LONG = -36,
  GRN_NO_LOCKS_AVAILABLE = -37,
  GRN_FUNCTION_NOT_IMPLEMENTED = -38,
  GRN_DIRECTORY_NOT_EMPTY = -39,
  GRN_ILLEGAL_BYTE_SEQUENCE = -40,
  GRN_SOCKET_NOT_INITIALIZED = -41,
  GRN_OPERATION_WOULD_BLOCK = -42,
  GRN_ADDRESS_IS_NOT_AVAILABLE = -43,
  GRN_NETWORK_IS_DOWN = -44,
  GRN_NO_BUFFER = -45,
  GRN_SOCKET_IS_ALREADY_CONNECTED = -46,
  GRN_SOCKET_IS_NOT_CONNECTED = -47,
  GRN_SOCKET_IS_ALREADY_SHUTDOWNED = -48,
  GRN_OPERATION_TIMEOUT = -49,
  GRN_CONNECTION_REFUSED = -50,
  GRN_RANGE_ERROR = -51,
  GRN_TOKENIZER_ERROR = -52,
  GRN_FILE_CORRUPT = -53,
  GRN_INVALID_FORMAT = -54,
  GRN_OBJECT_CORRUPT = -55,
  GRN_TOO_MANY_SYMBOLIC_LINKS = -56,
  GRN_NOT_SOCKET = -57,
  GRN_OPERATION_NOT_SUPPORTED = -58,
  GRN_ADDRESS_IS_IN_USE = -59,
  GRN_ZLIB_ERROR = -60,
  GRN_LZ4_ERROR = -61,
/* Just for backward compatibility. We'll remove it at 5.0.0. */
#define GRN_LZO_ERROR GRN_LZ4_ERROR
  GRN_STACK_OVER_FLOW = -62,
  GRN_SYNTAX_ERROR = -63,
  GRN_RETRY_MAX = -64,
  GRN_INCOMPATIBLE_FILE_FORMAT = -65,
  GRN_UPDATE_NOT_ALLOWED = -66,
  GRN_TOO_SMALL_OFFSET = -67,
  GRN_TOO_LARGE_OFFSET = -68,
  GRN_TOO_SMALL_LIMIT = -69,
  GRN_CAS_ERROR = -70,
  /// When a version not supported by the default version set is specified (-71)
  GRN_UNSUPPORTED_COMMAND_VERSION = -71,
  GRN_NORMALIZER_ERROR = -72,
  GRN_TOKEN_FILTER_ERROR = -73,
  GRN_COMMAND_ERROR = -74,
  GRN_PLUGIN_ERROR = -75,
  GRN_SCORER_ERROR = -76,
  GRN_CANCEL = -77,
  GRN_WINDOW_FUNCTION_ERROR = -78,
  GRN_ZSTD_ERROR = -79,
  GRN_CONNECTION_RESET = -80,
  GRN_BLOSC_ERROR = -81,
} grn_rc;

GRN_API grn_rc
grn_init(void);
GRN_API grn_rc
grn_fin(void);

GRN_API const char *
grn_get_global_error_message(void);

typedef enum {
  /// The default encoding specified at build time (0).
  /// The default at build time is "UTF-8".
  GRN_ENC_DEFAULT = 0,
  /// Process strings as binary data (1)
  GRN_ENC_NONE,
  /// EUC-JP (2)
  GRN_ENC_EUC_JP,
  /// UTF-8 (3)
  GRN_ENC_UTF8,
  /// Shift_JIS (4)
  GRN_ENC_SJIS,
  /// Latin-1 (5)
  GRN_ENC_LATIN1,
  /// KOI8-R (6)
  GRN_ENC_KOI8R
} grn_encoding;

typedef enum {
  GRN_COMMAND_VERSION_DEFAULT = 0,
  GRN_COMMAND_VERSION_1,
  GRN_COMMAND_VERSION_2,
  GRN_COMMAND_VERSION_3
} grn_command_version;

#define GRN_COMMAND_VERSION_MIN    GRN_COMMAND_VERSION_1
#define GRN_COMMAND_VERSION_STABLE GRN_COMMAND_VERSION_1
#define GRN_COMMAND_VERSION_MAX    GRN_COMMAND_VERSION_3

typedef enum {
  GRN_LOG_NONE = 0,
  GRN_LOG_EMERG,
  GRN_LOG_ALERT,
  GRN_LOG_CRIT,
  GRN_LOG_ERROR,
  GRN_LOG_WARNING,
  GRN_LOG_NOTICE,
  GRN_LOG_INFO,
  GRN_LOG_DEBUG,
  GRN_LOG_DUMP
} grn_log_level;

GRN_API const char *
grn_log_level_to_string(grn_log_level level);
GRN_API bool
grn_log_level_parse(const char *string, grn_log_level *level);

/* query log flags */
#define GRN_QUERY_LOG_NONE        (0x00)
#define GRN_QUERY_LOG_COMMAND     (0x01 << 0)
#define GRN_QUERY_LOG_RESULT_CODE (0x01 << 1)
#define GRN_QUERY_LOG_DESTINATION (0x01 << 2)
#define GRN_QUERY_LOG_CACHE       (0x01 << 3)
#define GRN_QUERY_LOG_SIZE        (0x01 << 4)
#define GRN_QUERY_LOG_SCORE       (0x01 << 5)
#define GRN_QUERY_LOG_ALL                                                      \
  (GRN_QUERY_LOG_COMMAND | GRN_QUERY_LOG_RESULT_CODE |                         \
   GRN_QUERY_LOG_DESTINATION | GRN_QUERY_LOG_CACHE | GRN_QUERY_LOG_SIZE |      \
   GRN_QUERY_LOG_SCORE)
#define GRN_QUERY_LOG_DEFAULT GRN_QUERY_LOG_ALL

typedef enum {
  GRN_CONTENT_NONE = 0,
  GRN_CONTENT_TSV,
  GRN_CONTENT_JSON,
  GRN_CONTENT_XML,
  GRN_CONTENT_MSGPACK,
  GRN_CONTENT_GROONGA_COMMAND_LIST,
  GRN_CONTENT_APACHE_ARROW
} grn_content_type;

typedef struct _grn_obj grn_obj;
typedef struct _grn_ctx grn_ctx;

#define GRN_CTX_MSGSIZE (0x80)
#define GRN_CTX_FIN     (0xff)

typedef void (*grn_close_func)(grn_ctx *ctx, void *data);

typedef union {
  int int_value;
  grn_id id;
  void *ptr;
} grn_user_data;

typedef grn_obj *
grn_proc_func(grn_ctx *ctx,
              int nargs,
              grn_obj **args,
              grn_user_data *user_data);

struct _grn_ctx {
  grn_rc rc;
  int flags;
  grn_encoding encoding;
  unsigned char ntrace;
  unsigned char errlvl;
  unsigned char stat;
  unsigned int seqno;
  unsigned int subno;
  unsigned int seqno2;
  unsigned int errline;
  grn_user_data user_data;
  grn_ctx *prev;
  grn_ctx *next;
  const char *errfile;
  const char *errfunc;
  struct _grn_ctx_impl *impl;
  void *trace[16];
  char errbuf[GRN_CTX_MSGSIZE];
};

#define GRN_CTX_USER_DATA(ctx) (&((ctx)->user_data))

/* Deprecated since 4.0.3. Don't use it. */
#define GRN_CTX_USE_QL (0x03)
/* Deprecated since 4.0.3. Don't use it. */
#define GRN_CTX_BATCH_MODE (0x04)
#define GRN_CTX_PER_DB     (0x08)

GRN_API grn_rc
grn_ctx_init(grn_ctx *ctx, int flags);
GRN_API grn_rc
grn_ctx_fin(grn_ctx *ctx);
GRN_API grn_ctx *
grn_ctx_open(int flags);
GRN_API grn_rc
grn_ctx_close(grn_ctx *ctx);
GRN_API grn_rc
grn_ctx_set_finalizer(grn_ctx *ctx, grn_proc_func *func);

GRN_API grn_rc
grn_ctx_push_temporary_open_space(grn_ctx *ctx);
GRN_API grn_rc
grn_ctx_pop_temporary_open_space(grn_ctx *ctx);
GRN_API grn_rc
grn_ctx_merge_temporary_open_space(grn_ctx *ctx);

/**
 * \return The default encoding
 */
GRN_API grn_encoding
grn_get_default_encoding(void);
/**
 * \brief Set the default encoding
 *
 * \param encoding New encoding
 *
 * \return \ref GRN_SUCCESS on success, \ref GRN_INVALID_ARGUMENT on
 *         error
 */
GRN_API grn_rc
grn_set_default_encoding(grn_encoding encoding);

#define GRN_CTX_GET_ENCODING(ctx) ((ctx)->encoding)
#define GRN_CTX_SET_ENCODING(ctx, enc)                                         \
  ((ctx)->encoding =                                                           \
     (enc == GRN_ENC_DEFAULT) ? grn_get_default_encoding() : enc)

GRN_API const char *
grn_get_version(void);
GRN_API uint32_t
grn_get_version_major(void);
GRN_API uint32_t
grn_get_version_minor(void);
GRN_API uint32_t
grn_get_version_micro(void);
GRN_API const char *
grn_get_package(void);
GRN_API const char *
grn_get_package_label(void);

/**
 * \return Default command version
 */
GRN_API grn_command_version
grn_get_default_command_version(void);
/**
 * \brief Set default command version
 *
 * \param version New command version
 *
 * \return \ref GRN_SUCCESS on success, \ref GRN_UNSUPPORTED_COMMAND_VERSION on
 *         error
 */
GRN_API grn_rc
grn_set_default_command_version(grn_command_version version);
GRN_API grn_command_version
grn_ctx_get_command_version(grn_ctx *ctx);
GRN_API grn_rc
grn_ctx_set_command_version(grn_ctx *ctx, grn_command_version version);
/**
 * \param ctx The context object.
 *
 * \return The threshold to determine whether search strategy escalation is used
 *         or not.
 *
 * \see
 * https://groonga.org/docs/reference/commands/select.html#match-escalation-threshold
 */
GRN_API int64_t
grn_ctx_get_match_escalation_threshold(grn_ctx *ctx);
/**
 * \brief Set the threshold to determine whether search strategy escalation is
 *        used or not.
 *
 * \param ctx The context object.
 * \param threshold New threshold.
 *
 * \return \ref GRN_SUCCESS.
 *
 * \see
 * https://groonga.org/docs/reference/commands/select.html#match-escalation-threshold
 */
GRN_API grn_rc
grn_ctx_set_match_escalation_threshold(grn_ctx *ctx, int64_t threshold);
GRN_API grn_bool
grn_ctx_get_force_match_escalation(grn_ctx *ctx);
GRN_API grn_rc
grn_ctx_set_force_match_escalation(grn_ctx *ctx, grn_bool force);
GRN_API int32_t
grn_ctx_get_n_workers(grn_ctx *ctx);
GRN_API grn_rc
grn_ctx_set_n_workers(grn_ctx *ctx, int32_t n_workers);
/**
 * \return The default threshold to determine whether search strategy escalation
 *         is used or not.
 *
 * \see
 * https://groonga.org/docs/reference/commands/select.html#match-escalation-threshold
 */
GRN_API int64_t
grn_get_default_match_escalation_threshold(void);
/**
 * \brief Set the default threshold to determine whether search strategy
 *        escalation is used or not.
 *
 * \param threshold New default threshold.
 *
 * \return \ref GRN_SUCCESS.
 *
 * \see
 * https://groonga.org/docs/reference/commands/select.html#match-escalation-threshold
 */
GRN_API grn_rc
grn_set_default_match_escalation_threshold(int64_t threshold);
GRN_API int32_t
grn_get_default_n_workers(void);
GRN_API grn_rc
grn_set_default_n_workers(int32_t n_workers);
GRN_API bool
grn_is_back_trace_enable(void);
GRN_API grn_rc
grn_set_back_trace_enable(bool enable);
GRN_API bool
grn_is_reference_count_enable(void);
GRN_API grn_rc
grn_set_reference_count_enable(bool enable);

GRN_API grn_rc
grn_ctx_set_variable(grn_ctx *ctx,
                     const char *name,
                     int name_size,
                     void *data,
                     grn_close_func close_func);
GRN_API void *
grn_ctx_get_variable(grn_ctx *ctx, const char *name, int name_size);
GRN_API grn_rc
grn_unset_variable(const char *name, int name_size);

GRN_API int
grn_get_lock_timeout(void);
GRN_API grn_rc
grn_set_lock_timeout(int timeout);

GRN_API size_t
grn_get_memory_map_size(void);

/* grn_encoding */
/**
 * \brief Return string representation for the encoding. For example,
 *        `grn_encoding_to_string(GRN_ENC_UTF8)` returns `"utf8"`.
 *
 * \param encoding The encoding
 *
 * \return String representation for the encoding on success, "unknown"
 *         on invalid encoding.
 */
GRN_API const char *
grn_encoding_to_string(grn_encoding encoding);
/**
 * \brief Parse encoding name and return \ref grn_encoding. For example,
 *        `grn_encoding_parse("UTF8")` returns \ref GRN_ENC_UTF8.
 *
 * \param name The encoding name
 *
 * \return \ref grn_encoding matching `name` on success, \ref GRN_ENC_UTF8 on
 *         invalid encoding name.
 */
GRN_API grn_encoding
grn_encoding_parse(const char *name);

/* obj */

typedef uint16_t grn_obj_flags;
typedef uint32_t grn_table_flags;
typedef uint32_t grn_column_flags;

/* flags for grn_obj_flags and grn_table_flags */

#define GRN_OBJ_FLAGS_MASK      (0xffff)

#define GRN_OBJ_TABLE_TYPE_MASK (0x07)
#define GRN_OBJ_TABLE_HASH_KEY  (0x00)
#define GRN_OBJ_TABLE_PAT_KEY   (0x01)
#define GRN_OBJ_TABLE_DAT_KEY   (0x02)
#define GRN_OBJ_TABLE_NO_KEY    (0x03)

/// Mask of `GRN_OBJ_KEY_*`
#define GRN_OBJ_KEY_MASK (0x07 << 3)
/// Unsigned integer
/// (Used to create data types. Used to determine sorting method.)
#define GRN_OBJ_KEY_UINT (0x00 << 3)
/// Signed integer
/// (Used to create data types. Used to determine sorting method.)
#define GRN_OBJ_KEY_INT (0x01 << 3)
/// Float
/// (Used to create data types. Used to determine sorting method.)
#define GRN_OBJ_KEY_FLOAT (0x02 << 3)
/// Latitude and longitude (\ref grn_geo_point)
/// (Used to create data types. Used to determine sorting method.)
#define GRN_OBJ_KEY_GEO_POINT (0x03 << 3)
/// Enable semi-infinite string support.
/// This is only available with \ref GRN_OBJ_TABLE_PAT_KEY.
/// You can use efficient suffix search with this but it requires more
/// storage/memory size. Because it generates many additional data for efficient
/// suffix search implicitly.
#define GRN_OBJ_KEY_WITH_SIS (0x01 << 6)
/**
 * \deprecated This was used when only NormalizerAuto was available.
 *             Now you can specify a normalizer, so there is no need
 *             to use it.
 */
#define GRN_OBJ_KEY_NORMALIZE (0x01 << 7)

/* flags for grn_obj_flags and grn_column_flags */

#define GRN_OBJ_COLUMN_TYPE_MASK (0x07)
#define GRN_OBJ_COLUMN_SCALAR    (0x00)
#define GRN_OBJ_COLUMN_VECTOR    (0x01)
#define GRN_OBJ_COLUMN_INDEX     (0x02)

#define GRN_OBJ_COMPRESS_MASK    (0x07 << 4)
#define GRN_OBJ_COMPRESS_NONE    (0x00 << 4)
#define GRN_OBJ_COMPRESS_ZLIB    (0x01 << 4)
#define GRN_OBJ_COMPRESS_LZ4     (0x02 << 4)
/* Just for backward compatibility. We'll remove it at 5.0.0. */
#define GRN_OBJ_COMPRESS_LZO           GRN_OBJ_COMPRESS_LZ4
#define GRN_OBJ_COMPRESS_ZSTD          (0x03 << 4)

#define GRN_OBJ_WITH_SECTION           (0x01 << 7)
#define GRN_OBJ_WITH_WEIGHT            (0x01 << 8)
#define GRN_OBJ_WITH_POSITION          (0x01 << 9)
#define GRN_OBJ_RING_BUFFER            (0x01 << 10)
#define GRN_OBJ_WEIGHT_BFLOAT16        (0x01 << 11)

#define GRN_OBJ_UNIT_MASK              (0x0f << 8)
#define GRN_OBJ_UNIT_DOCUMENT_NONE     (0x00 << 8)
#define GRN_OBJ_UNIT_DOCUMENT_SECTION  (0x01 << 8)
#define GRN_OBJ_UNIT_DOCUMENT_POSITION (0x02 << 8)
#define GRN_OBJ_UNIT_SECTION_NONE      (0x03 << 8)
#define GRN_OBJ_UNIT_SECTION_POSITION  (0x04 << 8)
#define GRN_OBJ_UNIT_POSITION_NONE     (0x05 << 8)
#define GRN_OBJ_UNIT_USERDEF_DOCUMENT  (0x06 << 8)
#define GRN_OBJ_UNIT_USERDEF_SECTION   (0x07 << 8)
#define GRN_OBJ_UNIT_USERDEF_POSITION  (0x08 << 8)

/* Don't use (0x01<<12) because it's used internally. */

#define GRN_OBJ_NO_SUBREC   (0x00 << 13)
#define GRN_OBJ_WITH_SUBREC (0x01 << 13)

/// It shows that the key is variable size not fixed size.
/// If you're using DB API such as \ref grn_table_create(), you don't need to
/// use this.
/// You can use type object such as `ShortText` instead.
/// If you're using a low-level table such as \ref grn_hash and \ref grn_pat
/// directly, you can use this to show that the table uses variable size key.
#define GRN_OBJ_KEY_VAR_SIZE (0x01 << 14)

#define GRN_OBJ_TEMPORARY    (0x00 << 15)
#define GRN_OBJ_PERSISTENT   (0x01 << 15)

/* flags only for grn_table_flags */

#define GRN_OBJ_KEY_LARGE (0x01 << 16)

/* flags only for grn_column_flags */

#define GRN_OBJ_INDEX_SMALL                               (0x01 << 16)
#define GRN_OBJ_INDEX_MEDIUM                              (0x01 << 17)
#define GRN_OBJ_INDEX_LARGE                               (0x01 << 18)
#define GRN_OBJ_WEIGHT_FLOAT32                            (0x01 << 19)

#define GRN_OBJ_MISSING_MASK                              (0x03 << 20)
#define GRN_OBJ_MISSING_ADD                               (0x00 << 20)
#define GRN_OBJ_MISSING_IGNORE                            (0x01 << 20)
#define GRN_OBJ_MISSING_NIL                               (0x02 << 20)

#define GRN_OBJ_INVALID_MASK                              (0x03 << 22)
#define GRN_OBJ_INVALID_ERROR                             (0x00 << 22)
#define GRN_OBJ_INVALID_WARN                              (0x01 << 22)
#define GRN_OBJ_INVALID_IGNORE                            (0x02 << 22)

#define GRN_OBJ_COMPRESS_FILTER_SHUFFLE                   (0x01 << 24)
#define GRN_OBJ_COMPRESS_FILTER_BYTE_DELTA                (0x01 << 25)
#define GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE  (0x01 << 26)
#define GRN_OBJ_COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES (0x01 << 27)

/* flags only for grn_table_flags and grn_column_flags */

/* GRN_COLUMN_INDEX only uses this for now */
#define GRN_OBJ_VISIBLE   ((uint32_t)(0x00 << 31))
#define GRN_OBJ_INVISIBLE ((uint32_t)(0x01 << 31))

/* obj types */

#define GRN_VOID (0x00)
/**
 * Auto-extendable buffer that can be used for storing a number and variable
 * size string.
 */
#define GRN_BULK (0x02)
/**
 * Buffer that has a `grn_obj *`. The hold `grn_obj *` can be closed
 * automatically when this is closed by specifying \ref GRN_OBJ_OWN flag.
 */
#define GRN_PTR (0x03)
/* vector of fixed size (uniform) data especially grn_id */
#define GRN_UVECTOR                 (0x04)
#define GRN_PVECTOR                 (0x05) /* vector of grn_obj* */
#define GRN_VECTOR                  (0x06) /* vector of arbitrary data */
#define GRN_MSG                     (0x07)
#define GRN_QUERY                   (0x08)
#define GRN_ACCESSOR                (0x09)
#define GRN_SNIP                    (0x0b)
#define GRN_PATSNIP                 (0x0c)
#define GRN_STRING                  (0x0d)
#define GRN_HIGHLIGHTER             (0x0e)
#define GRN_CURSOR_TABLE_HASH_KEY   (0x10)
#define GRN_CURSOR_TABLE_PAT_KEY    (0x11)
#define GRN_CURSOR_TABLE_DAT_KEY    (0x12)
#define GRN_CURSOR_TABLE_NO_KEY     (0x13)
#define GRN_CURSOR_COLUMN_INDEX     (0x18)
#define GRN_CURSOR_COLUMN_GEO_INDEX (0x1a)
#define GRN_CURSOR_CONFIG           (0x1f)
#define GRN_TYPE                    (0x20)
#define GRN_PROC                    (0x21)
#define GRN_EXPR                    (0x22)
#define GRN_TABLE_HASH_KEY          (0x30)
#define GRN_TABLE_PAT_KEY           (0x31)
#define GRN_TABLE_DAT_KEY           (0x32)
#define GRN_TABLE_NO_KEY            (0x33)
#define GRN_DB                      (0x37)
#define GRN_COLUMN_FIX_SIZE         (0x40)
#define GRN_COLUMN_VAR_SIZE         (0x41)
#define GRN_COLUMN_INDEX            (0x48)

typedef struct _grn_section {
  uint32_t offset;
  uint32_t length;
  float weight;
  grn_id domain;
} grn_section;

typedef struct _grn_obj_header {
  uint8_t type;
  uint8_t impl_flags;
  grn_obj_flags flags;
  grn_id domain;
} grn_obj_header;

struct _grn_obj {
  grn_obj_header header;
  union {
    struct {
      char *head;
      char *curr;
      char *tail;
    } b;
    struct {
      grn_obj *body;
      grn_section *sections;
      uint32_t n_sections;
    } v;
  } u;
};

#define GRN_OBJ_REFER    (0x01 << 0)
#define GRN_OBJ_OUTPLACE (0x01 << 1)
/**
 * A flag to represent that \ref GRN_PTR or \ref GRN_PVECTOR owns associated
 * \ref grn_obj. When this flag is set, the associated \ref grn_obj will be
 * automatically closed using \ref grn_obj_close when owning \ref GRN_PTR or
 * \ref GRN_PVECTOR is closed.
 *
 * You can use this flag only with \ref GRN_PTR_INIT.
 */
#define GRN_OBJ_OWN (0x01 << 5)

#define GRN_OBJ_INIT(obj, obj_type, obj_flags, obj_domain)                     \
  do {                                                                         \
    (obj)->header.type = (obj_type);                                           \
    (obj)->header.impl_flags = (obj_flags);                                    \
    (obj)->header.flags = 0;                                                   \
    (obj)->header.domain = (obj_domain);                                       \
    (obj)->u.b.head = NULL;                                                    \
    (obj)->u.b.curr = NULL;                                                    \
    (obj)->u.b.tail = NULL;                                                    \
  } while (0)

#define GRN_OBJ_FIN(ctx, obj) (grn_obj_close((ctx), (obj)))

GRN_API grn_rc
grn_ctx_use(grn_ctx *ctx, grn_obj *db);
GRN_API grn_obj *
grn_ctx_db(grn_ctx *ctx);
GRN_API grn_obj *
grn_ctx_get(grn_ctx *ctx, const char *name, int name_size);
GRN_API grn_rc
grn_ctx_get_all_tables(grn_ctx *ctx, grn_obj *tables_buffer);
GRN_API grn_rc
grn_ctx_get_all_types(grn_ctx *ctx, grn_obj *types_buffer);
GRN_API grn_rc
grn_ctx_get_all_tokenizers(grn_ctx *ctx, grn_obj *tokenizers_buffer);
GRN_API grn_rc
grn_ctx_get_all_normalizers(grn_ctx *ctx, grn_obj *normalizers_buffer);
GRN_API grn_rc
grn_ctx_get_all_token_filters(grn_ctx *ctx, grn_obj *token_filters_buffer);

typedef enum {
  GRN_DB_VOID = 0,
  GRN_DB_DB,
  GRN_DB_OBJECT,
  GRN_DB_BOOL,
  GRN_DB_INT8,
  GRN_DB_UINT8,
  GRN_DB_INT16,
  GRN_DB_UINT16,
  GRN_DB_INT32,
  GRN_DB_UINT32,
  GRN_DB_INT64,
  GRN_DB_UINT64,
  GRN_DB_FLOAT,
  GRN_DB_TIME,
  GRN_DB_SHORT_TEXT,
  GRN_DB_TEXT,
  GRN_DB_LONG_TEXT,
  GRN_DB_TOKYO_GEO_POINT,
  GRN_DB_WGS84_GEO_POINT,
  GRN_DB_FLOAT32,
  GRN_DB_BFLOAT16,
} grn_builtin_type;

typedef enum {
  GRN_DB_MECAB = 64,
  GRN_DB_DELIMIT,
  GRN_DB_UNIGRAM,
  GRN_DB_BIGRAM,
  GRN_DB_TRIGRAM
} grn_builtin_tokenizer;

GRN_API grn_obj *
grn_ctx_at(grn_ctx *ctx, grn_id id);
GRN_API bool
grn_ctx_is_opened(grn_ctx *ctx, grn_id id);

GRN_API grn_rc
grn_plugin_register(grn_ctx *ctx, const char *name);
GRN_API grn_rc
grn_plugin_unregister(grn_ctx *ctx, const char *name);
GRN_API grn_rc
grn_plugin_register_by_path(grn_ctx *ctx, const char *path);
GRN_API grn_rc
grn_plugin_unregister_by_path(grn_ctx *ctx, const char *path);
GRN_API const char *
grn_plugin_get_system_plugins_dir(void);
GRN_API const char *
grn_plugin_get_suffix(void);
GRN_API const char *
grn_plugin_get_ruby_suffix(void);
GRN_API grn_rc
grn_plugin_get_names(grn_ctx *ctx, grn_obj *names);

typedef struct {
  const char *name;
  unsigned int name_size;
  grn_obj value;
} grn_expr_var;

typedef grn_rc (*grn_plugin_func)(grn_ctx *ctx);

typedef grn_obj grn_table_cursor;

typedef enum {
  GRN_OP_PUSH = 0,
  GRN_OP_POP,
  GRN_OP_NOP,
  GRN_OP_CALL,
  GRN_OP_INTERN,
  GRN_OP_GET_REF,
  GRN_OP_GET_VALUE,
  GRN_OP_AND,
  GRN_OP_AND_NOT,
  /* Deprecated. Just for backward compatibility. */
#define GRN_OP_BUT GRN_OP_AND_NOT
  GRN_OP_OR,
  GRN_OP_ASSIGN,
  GRN_OP_STAR_ASSIGN,
  GRN_OP_SLASH_ASSIGN,
  GRN_OP_MOD_ASSIGN,
  GRN_OP_PLUS_ASSIGN,
  GRN_OP_MINUS_ASSIGN,
  GRN_OP_SHIFTL_ASSIGN,
  GRN_OP_SHIFTR_ASSIGN,
  GRN_OP_SHIFTRR_ASSIGN,
  GRN_OP_AND_ASSIGN,
  GRN_OP_XOR_ASSIGN,
  GRN_OP_OR_ASSIGN,
  GRN_OP_JUMP,
  GRN_OP_CJUMP,
  GRN_OP_COMMA,
  GRN_OP_BITWISE_OR,
  GRN_OP_BITWISE_XOR,
  GRN_OP_BITWISE_AND,
  GRN_OP_BITWISE_NOT,
  GRN_OP_EQUAL,
  GRN_OP_NOT_EQUAL,
  GRN_OP_LESS,
  GRN_OP_GREATER,
  GRN_OP_LESS_EQUAL,
  GRN_OP_GREATER_EQUAL,
  GRN_OP_IN,
  GRN_OP_MATCH,
  GRN_OP_NEAR,
  GRN_OP_NEAR_NO_OFFSET,
  /* Deprecated. Just for backward compatibility. */
#define GRN_OP_NEAR2 GRN_OP_NEAR_NO_OFFSET
  GRN_OP_SIMILAR,
  GRN_OP_TERM_EXTRACT,
  GRN_OP_SHIFTL,
  GRN_OP_SHIFTR,
  GRN_OP_SHIFTRR,
  GRN_OP_PLUS,
  GRN_OP_MINUS,
  GRN_OP_STAR,
  GRN_OP_SLASH,
  GRN_OP_MOD,
  GRN_OP_DELETE,
  GRN_OP_INCR,
  GRN_OP_DECR,
  GRN_OP_INCR_POST,
  GRN_OP_DECR_POST,
  GRN_OP_NOT,
  GRN_OP_ADJUST,
  GRN_OP_EXACT,
  GRN_OP_LCP,
  GRN_OP_PARTIAL,
  GRN_OP_UNSPLIT,
  GRN_OP_PREFIX,
  GRN_OP_SUFFIX,
  GRN_OP_GEO_DISTANCE1,
  GRN_OP_GEO_DISTANCE2,
  GRN_OP_GEO_DISTANCE3,
  GRN_OP_GEO_DISTANCE4,
  GRN_OP_GEO_WITHINP5,
  GRN_OP_GEO_WITHINP6,
  GRN_OP_GEO_WITHINP8,
  GRN_OP_OBJ_SEARCH,
  GRN_OP_EXPR_GET_VAR,
  GRN_OP_TABLE_CREATE,
  GRN_OP_TABLE_SELECT,
  GRN_OP_TABLE_SORT,
  GRN_OP_TABLE_GROUP,
  GRN_OP_JSON_PUT,
  GRN_OP_GET_MEMBER,
  GRN_OP_REGEXP,
  GRN_OP_FUZZY,
  GRN_OP_QUORUM,
  GRN_OP_NEAR_PHRASE,
  GRN_OP_ORDERED_NEAR_PHRASE,
  GRN_OP_NEAR_PHRASE_PRODUCT,
  GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT,
} grn_operator;

/**
 * \brief Retrieve a column or an accessor from a specified table or accessor.
 *
 *        This function returns a column corresponding to the given name from
 *        the specified table. If the name does not correspond to any column, it
 *        returns NULL. If the name is an accessor string, it returns the
 *        corresponding accessor. Accessor strings are dot-concatenated column
 *        names. Column names that are started with `_` such as `_id` and `_key`
 *        are pseudo column names. This function returns an accessor for a
 *        pseudo column name.
 *        See https://groonga.org/docs/reference/columns/pseudo.html for pseudo
 *        column.
 *
 *        Column name examples: `name`, `age`
 *
 *        Pseudo column name examples: `_key`, `_score`, `_nsubrecs`
 *
 *        Accessor string examples: `tag.name`, `user.bookmarks.url`
 *
 *        If this function returns an accessor, you must call `grn_obj_unlink()`
 *        with it when it's no longer needed. You can use
 *        `grn_obj_is_accessor()` to detect whether it's an accessor or not.
 *
 * \param ctx The context object
 * \param table The target table or accessor from which the column or accessor
 *              is retrieved.
 * \param name The name of the column or an accessor string.
 * \param name_size The length of the `name` string.
 *
 * \return The column or accessor, or NULL if not found.
 */
GRN_API grn_obj *
grn_obj_column(grn_ctx *ctx,
               grn_obj *table,
               const char *name,
               uint32_t name_size);

/*-------------------------------------------------------------
 * API for db, table and/or column
 */

typedef enum {
  GRN_INFO_ENCODING = 0,
  GRN_INFO_SOURCE,
  GRN_INFO_DEFAULT_TOKENIZER,
  GRN_INFO_ELEMENT_SIZE,
  GRN_INFO_CURR_MAX,
  GRN_INFO_MAX_ELEMENT_SIZE,
  GRN_INFO_SEG_SIZE,
  GRN_INFO_CHUNK_SIZE,
  GRN_INFO_MAX_SECTION,
  GRN_INFO_HOOK_LOCAL_DATA,
  GRN_INFO_ELEMENT_A,
  GRN_INFO_ELEMENT_CHUNK,
  GRN_INFO_ELEMENT_CHUNK_SIZE,
  GRN_INFO_ELEMENT_BUFFER_FREE,
  GRN_INFO_ELEMENT_NTERMS,
  GRN_INFO_ELEMENT_NTERMS_VOID,
  GRN_INFO_ELEMENT_SIZE_IN_CHUNK,
  GRN_INFO_ELEMENT_POS_IN_CHUNK,
  GRN_INFO_ELEMENT_SIZE_IN_BUFFER,
  GRN_INFO_ELEMENT_POS_IN_BUFFER,
  GRN_INFO_ELEMENT_ESTIMATE_SIZE,
  GRN_INFO_NGRAM_UNIT_SIZE,
  /*
  GRN_INFO_VERSION,
  GRN_INFO_CONFIGURE_OPTIONS,
  GRN_INFO_CONFIG_PATH,
  */
  GRN_INFO_PARTIAL_MATCH_THRESHOLD,
  GRN_INFO_II_SPLIT_THRESHOLD,
  GRN_INFO_SUPPORT_ZLIB,
  GRN_INFO_SUPPORT_LZ4,
/* Just for backward compatibility. We'll remove it at 5.0.0. */
#define GRN_INFO_SUPPORT_LZO GRN_INFO_SUPPORT_LZ4
  GRN_INFO_NORMALIZER,
  GRN_INFO_TOKEN_FILTERS,
  GRN_INFO_SUPPORT_ZSTD,
/* Just for backward compatibility. */
#define GRN_INFO_SUPPORT_ARROW GRN_INFO_SUPPORT_APACHE_ARROW
  GRN_INFO_SUPPORT_APACHE_ARROW,
  GRN_INFO_NORMALIZERS,
} grn_info_type;

GRN_API grn_obj *
grn_obj_get_info(grn_ctx *ctx,
                 grn_obj *obj,
                 grn_info_type type,
                 grn_obj *valuebuf);
GRN_API grn_rc
grn_obj_set_info(grn_ctx *ctx,
                 grn_obj *obj,
                 grn_info_type type,
                 grn_obj *value);
GRN_API grn_obj *
grn_obj_get_element_info(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_info_type type, grn_obj *value);
GRN_API grn_rc
grn_obj_set_element_info(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_info_type type, grn_obj *value);

/**
 * \brief Retrieve the value of the record corresponding to the given ID
 *        in the specified object.
 *
 * \param ctx The context object
 * \param obj The target object from which to retrieve the value
 * \param id The ID of the target record
 * \param value The buffer to store the retrieved value (must be prepared by the
 *              caller)
 *
 * \return The value of the specified record in the object.
 */
GRN_API grn_obj *
grn_obj_get_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value);
/**
 * \brief Retrieve an array of fixed-size column values starting from a
 *        specified record ID.
 *
 *        This function retrieves values from a fixed-size column (`obj`),
 *        starting at the record ID given by `offset`. The retrieved values are
 *        stored in the array pointed to by `values`, and the number of records
 *        that can be retrieved is returned.
 *
 * \attention It is not guaranteed that all record IDs within the specified
 *            range are valid. For tables where records may have been deleted,
 *            you must use functions such as \ref grn_table_at to check the
 *            existence of each record.
 *
 * \note If an error occurs and the return value is `-1`, check `ctx->rc` for
 *       the specific error code (e.g., \ref GRN_NO_MEMORY_AVAILABLE,
 *       \ref GRN_INVALID_ARGUMENT). Additional details might be available in
 *       `ctx->errbuf`.
 *
 * \param ctx The context object
 * \param obj The target fixed-size column
 * \param offset The starting record ID for retrieving values
 * \param values A pointer to an array where the values will be stored
 *
 * \return The number of records retrieved, or `-1` if an error occurred.
 */
GRN_API int
grn_obj_get_values(grn_ctx *ctx, grn_obj *obj, grn_id offset, void **values);

#define GRN_COLUMN_EACH(ctx, column, id, value, block)                         \
  do {                                                                         \
    int _n;                                                                    \
    grn_id id = 1;                                                             \
    while ((_n = grn_obj_get_values(ctx, column, id, (void **)&value)) > 0) {  \
      for (; _n; _n--, id++, value++) {                                        \
        block                                                                  \
      }                                                                        \
    }                                                                          \
  } while (0)

#define GRN_OBJ_SET_MASK (0x07)
/**
 * \brief Replace the value of the record/column with the specified value
 */
#define GRN_OBJ_SET (0x01)
/**
 * \brief Add the specified value to the record/column value
 */
#define GRN_OBJ_INCR (0x02)
/**
 * \brief Subtract the specified value from the record/column value
 */
#define GRN_OBJ_DECR (0x03)
/**
 * \brief Append the specified value to the column value.
 */
#define GRN_OBJ_APPEND (0x04)
/**
 * \brief Prepend the specified value to the column value.
 */
#define GRN_OBJ_PREPEND (0x05)
#define GRN_OBJ_GET     (0x01 << 4)
#define GRN_OBJ_COMPARE (0x01 << 5)
#define GRN_OBJ_LOCK    (0x01 << 6)
#define GRN_OBJ_UNLOCK  (0x01 << 7)

/**
 * \brief Update the value of a record identified by the given ID in the
 *        specified object.
 *
 *        This function updates the value of the record specified by `id` in the
 *        given object (`obj`). If the corresponding record does not exist, it
 *        returns \ref GRN_INVALID_ARGUMENT.
 *
 * \param ctx The context object.
 * \param obj The target object where the value will be updated.
 * \param id The ID of the record to be updated.
 * \param value The value to be stored in the record.
 * \param flags Only one flag can be specified, depending on the type of the
 *              object (`obj`).
 *              - For a table and a scalar column, use either \ref GRN_OBJ_SET,
 *                \ref GRN_OBJ_INCR, or \ref GRN_OBJ_DECR.
 *              - For a vector column, use either \ref GRN_OBJ_SET, \ref
 *                GRN_OBJ_INCR, \ref GRN_OBJ_DECR \ref GRN_OBJ_APPEND or
 *                \ref GRN_OBJ_PREPEND.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if the
 *         record does not exist.
 */
GRN_API grn_rc
grn_obj_set_value(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags);

#define GRN_OBJ_REMOVE_DEPENDENT (0x01 << 0)
#define GRN_OBJ_REMOVE_ENSURE    (0x01 << 1)

/**
 * \brief Remove the specified object.
 *
 *        This function frees the specified object (`obj`) from memory. If the
 *        object is a persistent object, it also removes the associated files
 *        from disk.
 *
 * \param ctx The context object.
 * \param obj The target object to be removed.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_obj_remove(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_remove_dependent(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_remove_flags(grn_ctx *ctx, grn_obj *obj, uint32_t flags);
/* Deprecated since 14.0.5. Use grn_ctx_remove(...,
 * GRN_OBJ_REMOVE_ENSURE) instead. */
GRN_API grn_rc
grn_obj_remove_force(grn_ctx *ctx, const char *name, int name_size);
GRN_API grn_rc
grn_ctx_remove(grn_ctx *ctx, const char *name, int name_size, uint32_t flags);
GRN_API grn_rc
grn_ctx_remove_by_id(grn_ctx *ctx, grn_id id, uint32_t flags);

/**
 * \brief Rename a persistent object in the database used by the context.
 *
 *        This function updates the name of the specified object (`obj`) to the
 *        new name provided. The object must be a persistent object.
 *
 * \param ctx The context object.
 * \param obj The target object to be renamed.
 * \param name The new name for the given object.
 * \param name_size The size of the `name` in bytes.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_obj_rename(grn_ctx *ctx,
               grn_obj *obj,
               const char *name,
               unsigned int name_size);

GRN_API grn_rc
grn_column_rename(grn_ctx *ctx,
                  grn_obj *column,
                  const char *name,
                  unsigned int name_size);

/**
 * \brief Close an object.
 *
 *        This function frees all resources used by the specified object (`obj`)
 *        from memory. All resources include other associated objects.
 *
 *        In general, you must close temporary objects explicitly. You don't
 *        need to close persistent objects explicitly because you can close
 *        persistent objects implicitly by closing a DB object.
 *
 *        You can use \ref grn_obj_unlink instead. It closes temporary objects
 *        but does nothing for most persistent objects. It's useful for normal
 *        use cases.
 *
 * \attention In general, you should not close persistent objects such as tables
 *            and columns for performance reasons. If you close persistent
 *            objects, you need to re-open them when they are needed again. This
 *            is inefficient.
 *
 * \param ctx The context object.
 * \param obj The object to be closed.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_obj_close(grn_ctx *ctx, grn_obj *obj);
/**
 * \brief Reinitialize an object.
 *
 *        Buffer objects, \ref GRN_BULK, \ref GRN_PTR, \ref GRN_UVECTOR,
 *        \ref GRN_PVECTOR, and \ref GRN_VECTOR, are only target objects
 *        of this function. You can't use other objects such as table and
 *        column for this function.
 *
 *        This function frees the current data in the specified object
 *        (`obj`) and initializes the specified `obj` for the specified
 *        `domain` and `flags`.
 *
 *        Before calling this function, the object must have been initialized.
 *        You can use `GRN_XXX_INIT()` macros such as \ref GRN_TEXT_INIT to
 *        initialize a buffer object.
 *
 * \param ctx The context object.
 * \param obj The object to be reinitialized. It must be a buffer object.
 * \param domain The new type that the object can hold.
 * \param flags `0` or \ref GRN_OBJ_VECTOR. If \ref GRN_OBJ_VECTOR is
 *              specified, the object will be configured to store a vector of
 *              values of the specified `domain`.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_obj_reinit(grn_ctx *ctx, grn_obj *obj, grn_id domain, uint8_t flags);
/* On non reference count mode (default):
 * This closes the following objects immediately:
 *
 *   * Acceessor
 *   * Bulk
 *   * DB
 *   * Temporary column
 *   * Temporary table
 *
 * This does nothing for other objects such as persisted tables and
 * columns.
 *
 * On reference count mode (GRN_ENABLE_REFERENCE_COUNT=yes):
 * This closes the following objects immediately:
 *
 *   * Bulk
 *   * DB
 *
 * This decreases the reference count of the following objects:
 *
 *   * Acceessor
 *   * Column (both persisted and temporary)
 *   * Table (both persisted and temporary)
 *
 * If the decreased reference count is zero, the object is closed.
 */
GRN_API grn_rc
grn_obj_unlink(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_refer(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_refer_recursive(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_refer_recursive_dependent(grn_ctx *ctx, grn_obj *obj);
/* This calls grn_obj_unlink() only on reference count mode
 * (GRN_ENABLE_REFERENCE_COUNT=yes) */
GRN_API void
grn_obj_unref(grn_ctx *ctx, grn_obj *obj);
GRN_API void
grn_obj_unref_recursive(grn_ctx *ctx, grn_obj *obj);
GRN_API void
grn_obj_unref_recursive_dependent(grn_ctx *ctx, grn_obj *obj);

/**
 * \brief Returns a pointer to user data that can be registered in the object
 *
 * \param ctx The context object
 * \param obj Only table, column, proc, and expr can be specified
 * \return Pointer to user data
 */
GRN_API grn_user_data *
grn_obj_user_data(grn_ctx *ctx, grn_obj *obj);

/**
 * \brief Set the callback function when finalizing the object.
 *
 * \param ctx The context object.
 * \param obj Target object. Table, table cursor, column, procedure, and
 *            expression can be specified.
 * \param func Callback function when finalizing.
 *
 * \return \ref GRN_SUCCESS on success, \ref GRN_INVALID_ARGUMENT if `obj`
 *         is not table, table cursor, column, procedure, or expression.
 */
GRN_API grn_rc
grn_obj_set_finalizer(grn_ctx *ctx, grn_obj *obj, grn_proc_func *func);
/**
 * \brief Return the file path associated with an object.
 *
 * \param ctx The context object.
 * \param obj The object whose file path is to be retrieved.
 *
 * \return The file path on success, `NULL` if the object is temporary.
 */
GRN_API const char *
grn_obj_path(grn_ctx *ctx, grn_obj *obj);
/**
 * \brief Return an object's name by storing it in `namebuf`.
 *
 *        If the object has a name and `buf_size` is greater than or equal to
 *        the length of the name, the name is copied into `namebuf`.
 *
 *        If the length of the name exceeds `buf_size`, the name is not copied
 *        into `namebuf`, but the length of the name is still returned.
 *
 *        The maximum possible length of the name is limited by
 *        \ref GRN_TABLE_MAX_KEY_SIZE.
 *
 * \param ctx The context object.
 * \param obj The object whose name is to be retrieved.
 * \param namebuf A buffer allocated by the caller to store the object's name.
 * \param buf_size The size of `namebuf` in bytes.
 *
 * \return The length of the object's name. Returns `0` if the object is unnamed
 *         or `NULL`.
 */
GRN_API int
grn_obj_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size);

GRN_API int
grn_column_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size);

/**
 * \brief Retrieve the value range based on the ID associated with an object.
 *
 *        For example, it may return \ref GRN_DB_INT32 from \ref
 *        grn_builtin_type to indicate that the object operates within the
 *        integer range.
 *
 * \param ctx The context object.
 * \param obj The target object whose range ID is to be returned.
 *
 * \return The ID of the range object. Returns \ref GRN_ID_NIL if no range is
 *         associated or the object is `NULL`.
 */
GRN_API grn_id
grn_obj_get_range(grn_ctx *ctx, grn_obj *obj);

#define GRN_OBJ_GET_DOMAIN(obj)                                                \
  ((obj)->header.type == GRN_TABLE_NO_KEY ? GRN_ID_NIL : (obj)->header.domain)

/**
 * \brief Free allocatable memory occupied by an object based on a threshold.
 *
 * \note This function is not implemented yet.
 *       The \ref grn_obj_expire function is intended to serve as a
 *       generic version of grn_ii_expire() and grn_io_expire().
 *
 * \param ctx The context object.
 * \param obj The target object whose memory is to be managed.
 * \param threshold The threshold value used as a benchmark for freeing memory
 *                  spaces.
 *
 * \return \ref GRN_SUCCESS on success.
 */
GRN_API int
grn_obj_expire(grn_ctx *ctx, grn_obj *obj, int threshold);
/**
 * \brief Check the integrity of the file corresponding to an object.
 *
 * \note This function is not implemented yet.
 *
 * \param ctx The context object.
 * \param obj The target object whose file is to be checked.
 *
 * \return \ref GRN_SUCCESS on success.
 */
GRN_API int
grn_obj_check(grn_ctx *ctx, grn_obj *obj);
/**
 * \brief Lock an object with a specified timeout.
 *
 * \param ctx The context object.
 * \param obj The target object to lock.
 * \param id The ID of the target object.
 * \param timeout The maximum time to wait for the lock, in seconds.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_RESOURCE_DEADLOCK_AVOIDED is returned if the
 *         lock could not be acquired within the specified timeout.
 */
GRN_API grn_rc
grn_obj_lock(grn_ctx *ctx, grn_obj *obj, grn_id id, int timeout);
/**
 * \brief Unlock an object.
 *
 * Unlike \ref grn_obj_clear_lock, which forcefully resets the lock count to
 * zero, this function decrements the lock count of an object by one.
 *
 * \note Locks are managed using a counter mechanism where each lock acquisition
 *       increments the count by one, and each unlock operation decrements it by
 *       one. When the lock count reaches zero, the object is considered
 *       unlocked.
 *
 * \param ctx The context object.
 * \param obj The target object to unlock.
 * \param id The ID of the target object.
 *
 * \return \ref GRN_SUCCESS on success.
 */
GRN_API grn_rc
grn_obj_unlock(grn_ctx *ctx, grn_obj *obj, grn_id id);
/**
 * \brief Forcefully clear locks on an object.
 *
 * Unlike \ref grn_obj_unlock, which decrements the lock count by one,
 * this function forcefully resets the lock count of an object to zero,
 * unlocking it regardless of the current lock count.
 *
 * \attention In general you should not use this function. Forcefully
 *            clearing locks might lead to data corruption or
 *            inconsistencies within the database. Use it only
 *            when absolutely necessary and ensure that you understand
 *            the potential consequences.
 *
 * \param ctx The context object.
 * \param obj The target object whose lock is to be cleared.
 *
 * \return \ref GRN_SUCCESS on success.
 */
GRN_API grn_rc
grn_obj_clear_lock(grn_ctx *ctx, grn_obj *obj);
/**
 * \brief Check if an object is currently locked.
 *
 * \param ctx The context object.
 * \param obj The target object whose lock status is to be checked.
 *
 * \return `0` if there are no locks. Return the number of acquired locks
 *         if the object or any of its related sub-objects are currently locked.
 */
GRN_API unsigned int
grn_obj_is_locked(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_flush(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_flush_recursive(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_flush_recursive_dependent(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_rc
grn_obj_flush_only_opened(grn_ctx *ctx, grn_obj *obj);
GRN_API int
grn_obj_defrag(grn_ctx *ctx, grn_obj *obj, int threshold);

/**
 * \brief Return the database to which `obj` belongs.
 *
 * \param ctx The context object.
 * \param obj Target object.
 *
 * \return The database object to which `obj` belongs, `NULL` if `obj` doesn't
 *         belong to any database.
 */
GRN_API grn_obj *
grn_obj_db(grn_ctx *ctx, grn_obj *obj);

GRN_API grn_id
grn_obj_id(grn_ctx *ctx, grn_obj *obj);

/* Flags for grn_fuzzy_search_optarg::flags. */
#define GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION     (0x01 << 0)
#define GRN_TABLE_FUZZY_SEARCH_USE_PREFIX_LENGTH      (0x01 << 1)
#define GRN_TABLE_FUZZY_SEARCH_USE_MAX_DISTANCE_RATIO (0x01 << 2)
/* We want to use GRN_TABLE_FUZZY_SEARCH_TOKENIZE but it breaks
 * backward compatibility. So we use this inverted name. */
#define GRN_TABLE_FUZZY_SEARCH_SKIP_TOKENIZE (0x01 << 3)

typedef struct _grn_fuzzy_search_optarg grn_fuzzy_search_optarg;

struct _grn_fuzzy_search_optarg {
  uint32_t max_distance;
  /* We want to name this max_expansions but can't change it to keep
   * backward compatibility. */
  uint32_t max_expansion;
  /* Unit is byte. */
  uint32_t prefix_match_size;
  uint32_t flags;
  /* Unit is character. This is used only when
   * GRN_TABLE_FUZZY_SEARCH_USE_PREFIX_LENGTH flag is set. */
  uint32_t prefix_length;
  /* This is used only when
   * GRN_TABLE_FUZZY_SEARCH_USE_MAX_DISTANCE_RATIO flag is set. */
  float max_distance_ratio;
};

#define GRN_MATCH_INFO_GET_MIN_RECORD_ID (0x01)
#define GRN_MATCH_INFO_ONLY_SKIP_TOKEN   (0x02)

typedef struct _grn_match_info grn_match_info;

struct _grn_match_info {
  int flags;
  grn_id min;
};

typedef struct _grn_search_optarg grn_search_optarg;

struct _grn_search_optarg {
  grn_operator mode;
  int similarity_threshold;
  int max_interval;
  int *weight_vector;
  int vector_size;
  grn_obj *proc;
  int max_size;
  grn_obj *scorer;
  grn_obj *scorer_args_expr;
  unsigned int scorer_args_expr_offset;
  grn_fuzzy_search_optarg fuzzy;
  grn_match_info match_info;
  int quorum_threshold;
  int additional_last_interval;
  float *weight_vector_float;
  float weight_float;
  grn_obj *query_options;
  grn_obj *max_element_intervals;
  int *min_interval;
  int32_t *start_position;
  grn_id *query_domain;
};

/**
 * \brief Search for `obj` in `query`.
 *
 * \param ctx The context object.
 * \param obj The object to be searched.
 * \param query Search query.
 * \param res The table to store results.
 * \param op Logical operation of the table stored in `res` and the table of
 *           the search result.
 *           - \ref GRN_OP_OR
 *           - \ref GRN_OP_AND
 *           - \ref GRN_OP_AND_NOT
 *           - \ref GRN_OP_ADJUST
 * \param optarg Search options. For example, you can specify the search mode in
 *               \ref grn_search_optarg::mode.
 *               If \ref GRN_OP_EQUAL is specified for
 *               \ref grn_search_optarg::mode, it searching for equal record.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if `obj` is
 *         `NULL`.
 */
GRN_API grn_rc
grn_obj_search(grn_ctx *ctx,
               grn_obj *obj,
               grn_obj *query,
               grn_obj *res,
               grn_operator op,
               grn_search_optarg *optarg);

GRN_API grn_rc
grn_proc_set_is_stable(grn_ctx *ctx, grn_obj *proc, grn_bool is_stable);
GRN_API grn_bool
grn_proc_is_stable(grn_ctx *ctx, grn_obj *proc);

/*-------------------------------------------------------------
 * API for hook
 */

GRN_API int
grn_proc_call_next(grn_ctx *ctx, grn_obj *exec_info, grn_obj *in, grn_obj *out);
GRN_API void *
grn_proc_get_ctx_local_data(grn_ctx *ctx, grn_obj *exec_info);
GRN_API void *
grn_proc_get_hook_local_data(grn_ctx *ctx, grn_obj *exec_info);

typedef enum {
  GRN_HOOK_SET = 0,
  GRN_HOOK_GET,
  GRN_HOOK_INSERT,
  GRN_HOOK_DELETE,
  GRN_HOOK_SELECT
} grn_hook_entry;

#define GRN_N_HOOK_ENTRIES 5 /* (GRN_HOOK_SELECT + 1) */

GRN_API const char *
grn_hook_entry_to_string(grn_hook_entry entry);

GRN_API grn_rc
grn_obj_add_hook(grn_ctx *ctx,
                 grn_obj *obj,
                 grn_hook_entry entry,
                 int offset,
                 grn_obj *proc,
                 grn_obj *data);
GRN_API int
grn_obj_get_nhooks(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry);
GRN_API grn_obj *
grn_obj_get_hook(
  grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset, grn_obj *data);
GRN_API grn_rc
grn_obj_delete_hook(grn_ctx *ctx,
                    grn_obj *obj,
                    grn_hook_entry entry,
                    int offset);

GRN_API grn_obj *
grn_obj_open(grn_ctx *ctx,
             unsigned char type,
             grn_obj_flags flags,
             grn_id domain);

/* Deprecated since 5.0.1. Use grn_column_find_index_data() instead. */
GRN_API int
grn_column_index(grn_ctx *ctx,
                 grn_obj *column,
                 grn_operator op,
                 grn_obj **indexbuf,
                 int buf_size,
                 int *section);

/* @since 5.0.1. */
typedef struct _grn_index_datum {
  grn_obj *index;
  unsigned int section;
} grn_index_datum;

/* @since 5.0.1. */
GRN_API unsigned int
grn_column_find_index_data(grn_ctx *ctx,
                           grn_obj *column,
                           grn_operator op,
                           grn_index_datum *index_data,
                           unsigned int n_index_data);
/* @since 5.1.2. */
GRN_API uint32_t
grn_column_get_all_index_data(grn_ctx *ctx,
                              grn_obj *column,
                              grn_index_datum *index_data,
                              uint32_t n_index_data);
/* @since 9.1.2. */
GRN_API grn_rc
grn_column_get_all_index_columns(grn_ctx *ctx,
                                 grn_obj *column,
                                 grn_obj *index_columns);

GRN_API grn_rc
grn_obj_delete_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, bool remove_p);
GRN_API grn_rc
grn_obj_path_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, char *buffer);

/* query & snippet */

#ifndef GRN_QUERY_AND
#  define GRN_QUERY_AND '+'
#endif /* GRN_QUERY_AND */
#ifndef GRN_QUERY_AND_NOT
#  ifdef GRN_QUERY_BUT
/* Deprecated. Just for backward compatibility. */
#    define GRN_QUERY_AND_NOT GRN_QUERY_BUT
#  else
#    define GRN_QUERY_AND_NOT '-'
#  endif /* GRN_QUERY_BUT */
#endif   /* GRN_QUERY_AND_NOT */
#ifndef GRN_QUERY_ADJ_INC
#  define GRN_QUERY_ADJ_INC '>'
#endif /* GRN_QUERY_ADJ_POS2 */
#ifndef GRN_QUERY_ADJ_DEC
#  define GRN_QUERY_ADJ_DEC '<'
#endif /* GRN_QUERY_ADJ_POS1 */
#ifndef GRN_QUERY_ADJ_NEG
#  define GRN_QUERY_ADJ_NEG '~'
#endif /* GRN_QUERY_ADJ_NEG */
#ifndef GRN_QUERY_PREFIX
#  define GRN_QUERY_PREFIX '*'
#endif /* GRN_QUERY_PREFIX */
#ifndef GRN_QUERY_PARENL
#  define GRN_QUERY_PARENL '('
#endif /* GRN_QUERY_PARENL */
#ifndef GRN_QUERY_PARENR
#  define GRN_QUERY_PARENR ')'
#endif /* GRN_QUERY_PARENR */
#ifndef GRN_QUERY_QUOTEL
#  define GRN_QUERY_QUOTEL '"'
#endif /* GRN_QUERY_QUOTEL */
#ifndef GRN_QUERY_QUOTER
#  define GRN_QUERY_QUOTER '"'
#endif /* GRN_QUERY_QUOTER */
#ifndef GRN_QUERY_ESCAPE
#  define GRN_QUERY_ESCAPE '\\'
#endif /* GRN_QUERY_ESCAPE */
#ifndef GRN_QUERY_COLUMN
#  define GRN_QUERY_COLUMN ':'
#endif /* GRN_QUERY_COLUMN */

typedef struct _grn_snip_mapping grn_snip_mapping;

struct _grn_snip_mapping {
  void *dummy;
};

#define GRN_SNIP_NORMALIZE           (0x01 << 0)
#define GRN_SNIP_COPY_TAG            (0x01 << 1)
#define GRN_SNIP_SKIP_LEADING_SPACES (0x01 << 2)

#define GRN_SNIP_MAPPING_HTML_ESCAPE ((grn_snip_mapping *)-1)

GRN_API grn_obj *
grn_snip_open(grn_ctx *ctx,
              int flags,
              unsigned int width,
              unsigned int max_results,
              const char *defaultopentag,
              unsigned int defaultopentag_len,
              const char *defaultclosetag,
              unsigned int defaultclosetag_len,
              grn_snip_mapping *mapping);
GRN_API grn_rc
grn_snip_add_cond(grn_ctx *ctx,
                  grn_obj *snip,
                  const char *keyword,
                  unsigned int keyword_len,
                  const char *opentag,
                  unsigned int opentag_len,
                  const char *closetag,
                  unsigned int closetag_len);
GRN_API grn_rc
grn_snip_set_normalizer(grn_ctx *ctx, grn_obj *snip, grn_obj *normalizer);
GRN_API grn_obj *
grn_snip_get_normalizer(grn_ctx *ctx, grn_obj *snip);
GRN_API grn_rc
grn_snip_set_normalizers(grn_ctx *ctx, grn_obj *snip, grn_obj *normalizers);
GRN_API grn_obj *
grn_snip_get_normalizers(grn_ctx *ctx, grn_obj *snip);
GRN_API grn_rc
grn_snip_set_delimiter_regexp(grn_ctx *ctx,
                              grn_obj *snip,
                              const char *pattern,
                              int pattern_length);
GRN_API const char *
grn_snip_get_delimiter_regexp(grn_ctx *ctx,
                              grn_obj *snip,
                              size_t *pattern_length);
GRN_API grn_rc
grn_snip_exec(grn_ctx *ctx,
              grn_obj *snip,
              const char *string,
              unsigned int string_len,
              unsigned int *nresults,
              unsigned int *max_tagged_len);
GRN_API grn_rc
grn_snip_get_result(grn_ctx *ctx,
                    grn_obj *snip,
                    const unsigned int index,
                    char *result,
                    unsigned int *result_len);

/* log */

#define GRN_LOG_NONE       (0x00 << 0)
#define GRN_LOG_TIME       (0x01 << 0)
#define GRN_LOG_TITLE      (0x01 << 1)
#define GRN_LOG_MESSAGE    (0x01 << 2)
#define GRN_LOG_LOCATION   (0x01 << 3)
#define GRN_LOG_PID        (0x01 << 4)
#define GRN_LOG_PROCESS_ID GRN_LOG_PID
#define GRN_LOG_THREAD_ID  (0x01 << 5)
#define GRN_LOG_ALL                                                            \
  (GRN_LOG_TIME | GRN_LOG_TITLE | GRN_LOG_MESSAGE | GRN_LOG_LOCATION |         \
   GRN_LOG_PROCESS_ID | GRN_LOG_THREAD_ID)
#define GRN_LOG_DEFAULT (GRN_LOG_TIME | GRN_LOG_MESSAGE)

/* Deprecated since 2.1.2. Use grn_logger instead. */
typedef struct _grn_logger_info grn_logger_info;

/* Deprecated since 2.1.2. Use grn_logger instead. */
struct _grn_logger_info {
  grn_log_level max_level;
  int flags;
  void (*func)(
    int, const char *, const char *, const char *, const char *, void *);
  void *func_arg;
};

/* Deprecated since 2.1.2. Use grn_logger_set() instead. */
GRN_API grn_rc
grn_logger_info_set(grn_ctx *ctx, const grn_logger_info *info);

typedef struct _grn_logger grn_logger;

struct _grn_logger {
  grn_log_level max_level;
  int flags;
  void *user_data;
  void (*log)(grn_ctx *ctx,
              grn_log_level level,
              const char *timestamp,
              const char *title,
              const char *message,
              const char *location,
              void *user_data);
  void (*reopen)(grn_ctx *ctx, void *user_data);
  void (*fin)(grn_ctx *ctx, void *user_data);
};

GRN_API bool
grn_log_flags_parse(const char *string, int string_size, int *flags);

GRN_API grn_rc
grn_logger_set(grn_ctx *ctx, const grn_logger *logger);

GRN_API void
grn_logger_set_max_level(grn_ctx *ctx, grn_log_level max_level);
GRN_API grn_log_level
grn_logger_get_max_level(grn_ctx *ctx);

#ifdef __GNUC__
#  define GRN_ATTRIBUTE_PRINTF(fmt_pos)                                        \
    __attribute__((format(printf, fmt_pos, fmt_pos + 1)))
#else
#  define GRN_ATTRIBUTE_PRINTF(fmt_pos)
#endif /* __GNUC__ */

#if defined(__clang__)
#  if __has_attribute(__alloc_size__)
#    define HAVE_ALLOC_SIZE_ATTRIBUTE
#  endif /* __has_attribute(__alloc_size__) */
#elif defined(__GNUC__) &&                                                     \
  ((__GNUC__ >= 5) || (__GNUC__ > 4 && __GNUC_MINOR__ >= 3))
#  define HAVE_ALLOC_SIZE_ATTRIBUTE
#endif /* __clang__ */

#ifdef HAVE_ALLOC_SIZE_ATTRIBUTE
#  define GRN_ATTRIBUTE_ALLOC_SIZE(size) __attribute__((alloc_size(size)))
#  define GRN_ATTRIBUTE_ALLOC_SIZE_N(n, size)                                  \
    __attribute__((alloc_size(n, size)))
#else
#  define GRN_ATTRIBUTE_ALLOC_SIZE(size)
#  define GRN_ATTRIBUTE_ALLOC_SIZE_N(n, size)
#endif /* HAVE_ALLOC_SIZE_ATTRIBUTE */

GRN_API void
grn_logger_put(grn_ctx *ctx,
               grn_log_level level,
               const char *file,
               int line,
               const char *func,
               const char *fmt,
               ...) GRN_ATTRIBUTE_PRINTF(6);
GRN_API void
grn_logger_putv(grn_ctx *ctx,
                grn_log_level level,
                const char *file,
                int line,
                const char *func,
                const char *fmt,
                va_list ap);
GRN_API void
grn_logger_reopen(grn_ctx *ctx);

GRN_API bool
grn_logger_pass(grn_ctx *ctx, grn_log_level level);

GRN_API bool
grn_logger_is_default_logger(grn_ctx *ctx);

#ifndef GRN_LOG_DEFAULT_LEVEL
#  define GRN_LOG_DEFAULT_LEVEL GRN_LOG_NOTICE
#endif /* GRN_LOG_DEFAULT_LEVEL */

GRN_API void
grn_default_logger_set_max_level(grn_log_level level);
GRN_API grn_log_level
grn_default_logger_get_max_level(void);
GRN_API void
grn_default_logger_set_flags(int flags);
GRN_API int
grn_default_logger_get_flags(void);
GRN_API void
grn_default_logger_set_path(const char *path);
GRN_API const char *
grn_default_logger_get_path(void);
GRN_API void
grn_default_logger_set_rotate_threshold_size(off_t threshold);
GRN_API off_t
grn_default_logger_get_rotate_threshold_size(void);

#define GRN_LOG(ctx, level, ...)                                               \
  do {                                                                         \
    if (grn_logger_pass(ctx, level)) {                                         \
      grn_logger_put(ctx,                                                      \
                     (level),                                                  \
                     __FILE__,                                                 \
                     __LINE__,                                                 \
                     __FUNCTION__,                                             \
                     __VA_ARGS__);                                             \
    }                                                                          \
  } while (0)

GRN_API grn_rc
grn_slow_log_push(grn_ctx *ctx);
GRN_API double
grn_slow_log_pop(grn_ctx *ctx);
GRN_API bool
grn_slow_log_is_slow(grn_ctx *ctx, double elapsed_time);

#define GRN_SLOW_LOG_PUSH(ctx, level)                                          \
  do {                                                                         \
    if (grn_logger_pass(ctx, level)) {                                         \
      grn_slow_log_push(ctx);                                                  \
    }                                                                          \
  } while (false)

#define GRN_SLOW_LOG_POP_BEGIN(ctx, level, elapsed_time)                       \
  do {                                                                         \
    if (grn_logger_pass(ctx, level)) {                                         \
      double elapsed_time = grn_slow_log_pop(ctx);                             \
      if (grn_slow_log_is_slow(ctx, elapsed_time)) {

#define GRN_SLOW_LOG_POP_END(ctx)                                              \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  while (false)

typedef struct _grn_query_logger grn_query_logger;

struct _grn_query_logger {
  unsigned int flags;
  void *user_data;
  void (*log)(grn_ctx *ctx,
              unsigned int flag,
              const char *timestamp,
              const char *info,
              const char *message,
              void *user_data);
  void (*reopen)(grn_ctx *ctx, void *user_data);
  void (*fin)(grn_ctx *ctx, void *user_data);
};

GRN_API bool
grn_query_log_flags_parse(const char *string,
                          int string_size,
                          unsigned int *flags);

GRN_API grn_rc
grn_query_logger_set(grn_ctx *ctx, const grn_query_logger *logger);
GRN_API void
grn_query_logger_set_flags(grn_ctx *ctx, unsigned int flags);
GRN_API void
grn_query_logger_add_flags(grn_ctx *ctx, unsigned int flags);
GRN_API void
grn_query_logger_remove_flags(grn_ctx *ctx, unsigned int flags);
GRN_API unsigned int
grn_query_logger_get_flags(grn_ctx *ctx);

GRN_API void
grn_query_logger_put(
  grn_ctx *ctx, unsigned int flag, const char *mark, const char *format, ...)
  GRN_ATTRIBUTE_PRINTF(4);
GRN_API void
grn_query_logger_reopen(grn_ctx *ctx);

GRN_API bool
grn_query_logger_pass(grn_ctx *ctx, unsigned int flag);

GRN_API void
grn_default_query_logger_set_flags(unsigned int flags);
GRN_API unsigned int
grn_default_query_logger_get_flags(void);
GRN_API void
grn_default_query_logger_set_path(const char *path);
GRN_API const char *
grn_default_query_logger_get_path(void);
GRN_API void
grn_default_query_logger_set_rotate_threshold_size(off_t threshold);
GRN_API off_t
grn_default_query_logger_get_rotate_threshold_size(void);

#define GRN_QUERY_LOG(ctx, flag, mark, format, ...)                            \
  do {                                                                         \
    if (grn_query_logger_pass(ctx, flag)) {                                    \
      grn_query_logger_put(ctx, (flag), (mark), format, __VA_ARGS__);          \
    }                                                                          \
  } while (0)

/* grn_bulk */

#define GRN_BULK_BUFSIZE (sizeof(grn_obj) - sizeof(grn_obj_header))
/* This assumes that GRN_BULK_BUFSIZE is less than 32 (= 0x20). */
#define GRN_BULK_BUFSIZE_MAX          0x1f
#define GRN_BULK_SIZE_IN_FLAGS(flags) (size_t)((flags)&GRN_BULK_BUFSIZE_MAX)
#define GRN_BULK_OUTP(bulk)           ((bulk)->header.impl_flags & GRN_OBJ_OUTPLACE)
#define GRN_BULK_REWIND(bulk)                                                  \
  do {                                                                         \
    if ((bulk)->header.type == GRN_VECTOR) {                                   \
      grn_obj *_body = (bulk)->u.v.body;                                       \
      if (_body) {                                                             \
        if (GRN_BULK_OUTP(_body)) {                                            \
          (_body)->u.b.curr = (_body)->u.b.head;                               \
        } else {                                                               \
          (_body)->header.flags &= (grn_obj_flags)(~GRN_BULK_BUFSIZE_MAX);     \
        }                                                                      \
      }                                                                        \
      (bulk)->u.v.n_sections = 0;                                              \
    } else {                                                                   \
      if (GRN_BULK_OUTP(bulk)) {                                               \
        (bulk)->u.b.curr = (bulk)->u.b.head;                                   \
      } else {                                                                 \
        (bulk)->header.flags &= (grn_obj_flags)(~GRN_BULK_BUFSIZE_MAX);        \
      }                                                                        \
    }                                                                          \
  } while (0)
#define GRN_BULK_SET_CURR(buf, p)                                              \
  do {                                                                         \
    if (GRN_BULK_OUTP(buf)) {                                                  \
      (buf)->u.b.curr = (char *)(p);                                           \
    } else {                                                                   \
      (buf)->header.flags = (grn_obj_flags)((char *)(p)-GRN_BULK_HEAD(buf));   \
    }                                                                          \
  } while (0)
#define GRN_BULK_INCR_LEN(bulk, len)                                           \
  do {                                                                         \
    if (GRN_BULK_OUTP(bulk)) {                                                 \
      (bulk)->u.b.curr += (len);                                               \
    } else {                                                                   \
      (bulk)->header.flags += (grn_obj_flags)(len);                            \
    }                                                                          \
  } while (0)
#define GRN_BULK_WSIZE(bulk)                                                   \
  (GRN_BULK_OUTP(bulk) ? (size_t)((bulk)->u.b.tail - (bulk)->u.b.head)         \
                       : GRN_BULK_BUFSIZE)
#define GRN_BULK_REST(bulk)                                                    \
  (GRN_BULK_OUTP(bulk)                                                         \
     ? (size_t)((bulk)->u.b.tail - (bulk)->u.b.curr)                           \
     : GRN_BULK_BUFSIZE - GRN_BULK_SIZE_IN_FLAGS((bulk)->header.flags))
#define GRN_BULK_VSIZE(bulk)                                                   \
  (GRN_BULK_OUTP(bulk) ? (size_t)((bulk)->u.b.curr - (bulk)->u.b.head)         \
                       : GRN_BULK_SIZE_IN_FLAGS((bulk)->header.flags))
#define GRN_BULK_EMPTYP(bulk)                                                  \
  (GRN_BULK_OUTP(bulk) ? ((bulk)->u.b.curr == (bulk)->u.b.head)                \
                       : !(GRN_BULK_SIZE_IN_FLAGS((bulk)->header.flags)))
#define GRN_BULK_HEAD(bulk)                                                    \
  (GRN_BULK_OUTP(bulk) ? ((bulk)->u.b.head) : (char *)&((bulk)->u.b.head))
#define GRN_BULK_CURR(bulk)                                                    \
  (GRN_BULK_OUTP(bulk) ? ((bulk)->u.b.curr)                                    \
                       : (char *)&((bulk)->u.b.head) +                         \
                           GRN_BULK_SIZE_IN_FLAGS((bulk)->header.flags))
#define GRN_BULK_TAIL(bulk)                                                    \
  (GRN_BULK_OUTP(bulk) ? ((bulk)->u.b.tail) : (char *)&((bulk)[1]))

GRN_API grn_rc
grn_bulk_reinit(grn_ctx *ctx, grn_obj *bulk, size_t size);
GRN_API grn_rc
grn_bulk_resize(grn_ctx *ctx, grn_obj *bulk, size_t new_size);
GRN_API grn_rc
grn_bulk_write(grn_ctx *ctx, grn_obj *bulk, const char *str, size_t len);
GRN_API grn_rc
grn_bulk_write_from(
  grn_ctx *ctx, grn_obj *bulk, const char *str, size_t from, size_t len);
GRN_API grn_rc
grn_bulk_reserve(grn_ctx *ctx, grn_obj *bulk, size_t len);
GRN_API grn_rc
grn_bulk_space(grn_ctx *ctx, grn_obj *bulk, size_t len);
GRN_API grn_rc
grn_bulk_truncate(grn_ctx *ctx, grn_obj *bulk, size_t len);
GRN_API char *
grn_bulk_detach(grn_ctx *ctx, grn_obj *bulk);
GRN_API grn_rc
grn_bulk_fin(grn_ctx *ctx, grn_obj *bulk);

/* grn_text */

GRN_API grn_rc
grn_text_itoa(grn_ctx *ctx, grn_obj *bulk, int i);
GRN_API grn_rc
grn_text_itoa_padded(grn_ctx *ctx, grn_obj *bulk, int i, char ch, size_t len);
GRN_API grn_rc
grn_text_lltoa(grn_ctx *ctx, grn_obj *bulk, long long int i);
#ifdef GRN_HAVE_BFLOAT16
GRN_API grn_rc
grn_text_bf16toa(grn_ctx *ctx, grn_obj *bulk, grn_bfloat16 value);
#endif
GRN_API grn_rc
grn_text_f32toa(grn_ctx *ctx, grn_obj *bulk, float f);
GRN_API grn_rc
grn_text_ftoa(grn_ctx *ctx, grn_obj *bulk, double d);
GRN_API grn_rc
grn_text_itoh(grn_ctx *ctx, grn_obj *bulk, unsigned int i, size_t len);
GRN_API grn_rc
grn_text_itob(grn_ctx *ctx, grn_obj *bulk, grn_id id);
GRN_API grn_rc
grn_text_lltob32h(grn_ctx *ctx, grn_obj *bulk, long long int i);
GRN_API grn_rc
grn_text_benc(grn_ctx *ctx, grn_obj *bulk, unsigned int v);
GRN_API grn_rc
grn_text_esc(grn_ctx *ctx, grn_obj *bulk, const char *s, size_t len);
GRN_API grn_rc
grn_text_code_point(grn_ctx *ctx, grn_obj *buffer, uint32_t code_point);
GRN_API grn_rc
grn_text_urlenc(grn_ctx *ctx, grn_obj *buf, const char *str, size_t len);
GRN_API const char *
grn_text_urldec(
  grn_ctx *ctx, grn_obj *buf, const char *s, const char *e, char d);
GRN_API grn_rc
grn_text_escape_xml(grn_ctx *ctx, grn_obj *buf, const char *s, size_t len);
GRN_API grn_rc
grn_text_time2rfc1123(grn_ctx *ctx, grn_obj *bulk, int sec);
GRN_API grn_rc
grn_text_printf(grn_ctx *ctx, grn_obj *bulk, const char *format, ...)
  GRN_ATTRIBUTE_PRINTF(3);
GRN_API grn_rc
grn_text_printfv(grn_ctx *ctx, grn_obj *bulk, const char *format, va_list args);
/* Deprecated since 10.0.3. Use grn_text_printfv() instead. */
GRN_API grn_rc
grn_text_vprintf(grn_ctx *ctx, grn_obj *bulk, const char *format, va_list args);

typedef void (*grn_recv_handler_func)(grn_ctx *ctx, int flags, void *user_data);

GRN_API void
grn_ctx_recv_handler_set(grn_ctx *,
                         grn_recv_handler_func func,
                         void *user_data);

/* various values exchanged via grn_obj */

#define GRN_OBJ_DO_SHALLOW_COPY (GRN_OBJ_REFER | GRN_OBJ_OUTPLACE)
/* A flag to initialize a vector object. */
#define GRN_OBJ_VECTOR       (0x01 << 7)

#define GRN_OBJ_MUTABLE(obj) ((obj) && (obj)->header.type <= GRN_VECTOR)

#define GRN_VALUE_FIX_SIZE_INIT(obj, flags, domain)                            \
  GRN_OBJ_INIT((obj),                                                          \
               ((flags)&GRN_OBJ_VECTOR) ? GRN_UVECTOR : GRN_BULK,              \
               ((flags)&GRN_OBJ_DO_SHALLOW_COPY),                              \
               (domain))
#define GRN_VALUE_VAR_SIZE_INIT(obj, flags, domain)                            \
  GRN_OBJ_INIT((obj),                                                          \
               ((flags)&GRN_OBJ_VECTOR) ? GRN_VECTOR : GRN_BULK,               \
               ((flags)&GRN_OBJ_DO_SHALLOW_COPY),                              \
               (domain))

#define GRN_VOID_INIT(obj) GRN_OBJ_INIT((obj), GRN_VOID, 0, GRN_DB_VOID)
#define GRN_TEXT_INIT(obj, flags)                                              \
  GRN_VALUE_VAR_SIZE_INIT(obj, flags, GRN_DB_TEXT)
#define GRN_SHORT_TEXT_INIT(obj, flags)                                        \
  GRN_VALUE_VAR_SIZE_INIT(obj, flags, GRN_DB_SHORT_TEXT)
#define GRN_LONG_TEXT_INIT(obj, flags)                                         \
  GRN_VALUE_VAR_SIZE_INIT(obj, flags, GRN_DB_LONG_TEXT)
#define GRN_TEXT_SET_REF(obj, str, len)                                        \
  do {                                                                         \
    (obj)->u.b.head = (char *)(str);                                           \
    (obj)->u.b.curr = (char *)(str) + (len);                                   \
  } while (0)
#define GRN_TEXT_SET(ctx, obj, str, len)                                       \
  do {                                                                         \
    if ((obj)->header.impl_flags & GRN_OBJ_REFER) {                            \
      GRN_TEXT_SET_REF((obj), (str), (len));                                   \
    } else {                                                                   \
      grn_bulk_write_from((ctx),                                               \
                          (obj),                                               \
                          (const char *)(str),                                 \
                          0,                                                   \
                          (unsigned int)(len));                                \
    }                                                                          \
  } while (0)
#define GRN_TEXT_PUT(ctx, obj, str, len)                                       \
  grn_bulk_write((ctx), (obj), (const char *)(str), (unsigned int)(len))
#define GRN_TEXT_PUTC(ctx, obj, c)                                             \
  do {                                                                         \
    char _c = (c);                                                             \
    grn_bulk_write((ctx), (obj), &_c, 1);                                      \
  } while (0)

#define GRN_TEXT_PUTS(ctx, obj, str)                                           \
  GRN_TEXT_PUT((ctx), (obj), (str), strlen(str))
#define GRN_TEXT_SETS(ctx, obj, str)                                           \
  GRN_TEXT_SET((ctx), (obj), (str), strlen(str))
#define GRN_TEXT_VALUE(obj) GRN_BULK_HEAD(obj)
#define GRN_TEXT_LEN(obj)   GRN_BULK_VSIZE(obj)

#define GRN_TEXT_EQUAL_CSTRING(bulk, string)                                   \
  (GRN_TEXT_LEN(bulk) == strlen(string) &&                                     \
   memcmp(GRN_TEXT_VALUE(bulk), string, GRN_TEXT_LEN(bulk)) == 0)

#define GRN_BOOL_INIT(obj, flags)                                              \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_BOOL)
#define GRN_INT8_INIT(obj, flags)                                              \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_INT8)
#define GRN_UINT8_INIT(obj, flags)                                             \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_UINT8)
#define GRN_INT16_INIT(obj, flags)                                             \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_INT16)
#define GRN_UINT16_INIT(obj, flags)                                            \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_UINT16)
#define GRN_INT32_INIT(obj, flags)                                             \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_INT32)
#define GRN_UINT32_INIT(obj, flags)                                            \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_UINT32)
#define GRN_INT64_INIT(obj, flags)                                             \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_INT64)
#define GRN_UINT64_INIT(obj, flags)                                            \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_UINT64)
#define GRN_BFLOAT16_INIT(obj, flags)                                          \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_BFLOAT16)
#define GRN_FLOAT32_INIT(obj, flags)                                           \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_FLOAT32)
#define GRN_FLOAT_INIT(obj, flags)                                             \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_FLOAT)
#define GRN_TIME_INIT(obj, flags)                                              \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_TIME)
#define GRN_RECORD_INIT GRN_VALUE_FIX_SIZE_INIT
#define GRN_PTR_INIT(obj, flags, domain)                                       \
  GRN_OBJ_INIT((obj),                                                          \
               ((flags)&GRN_OBJ_VECTOR) ? GRN_PVECTOR : GRN_PTR,               \
               ((flags) & (GRN_OBJ_DO_SHALLOW_COPY | GRN_OBJ_OWN)),            \
               (domain))
#define GRN_TOKYO_GEO_POINT_INIT(obj, flags)                                   \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_TOKYO_GEO_POINT)
#define GRN_WGS84_GEO_POINT_INIT(obj, flags)                                   \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_WGS84_GEO_POINT)

#define GRN_BOOL_SET(ctx, obj, val)                                            \
  do {                                                                         \
    bool _val = (bool)(val);                                                   \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(bool));         \
  } while (0)
#define GRN_INT8_SET(ctx, obj, val)                                            \
  do {                                                                         \
    int8_t _val = (int8_t)(val);                                               \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int8_t));       \
  } while (0)
#define GRN_UINT8_SET(ctx, obj, val)                                           \
  do {                                                                         \
    uint8_t _val = (uint8_t)(val);                                             \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(uint8_t));      \
  } while (0)
#define GRN_INT16_SET(ctx, obj, val)                                           \
  do {                                                                         \
    int16_t _val = (int16_t)(val);                                             \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int16_t));      \
  } while (0)
#define GRN_UINT16_SET(ctx, obj, val)                                          \
  do {                                                                         \
    uint16_t _val = (uint16_t)(val);                                           \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(uint16_t));     \
  } while (0)
#define GRN_INT32_SET(ctx, obj, val)                                           \
  do {                                                                         \
    int32_t _val = (int32_t)(val);                                             \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int32_t));      \
  } while (0)
#define GRN_UINT32_SET(ctx, obj, val)                                          \
  do {                                                                         \
    uint32_t _val = (uint32_t)(val);                                           \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(uint32_t));     \
  } while (0)
#define GRN_INT64_SET(ctx, obj, val)                                           \
  do {                                                                         \
    int64_t _val = (int64_t)(val);                                             \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int64_t));      \
  } while (0)
#define GRN_UINT64_SET(ctx, obj, val)                                          \
  do {                                                                         \
    uint64_t _val = (uint64_t)(val);                                           \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(uint64_t));     \
  } while (0)
#define GRN_BFLOAT16_SET(ctx, obj, val)                                        \
  do {                                                                         \
    grn_bfloat16 _val = (grn_bfloat16)(val);                                   \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(grn_bfloat16)); \
  } while (0)
#define GRN_FLOAT32_SET(ctx, obj, val)                                         \
  do {                                                                         \
    float _val = (float)(val);                                                 \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(float));        \
  } while (0)
#define GRN_FLOAT_SET(ctx, obj, val)                                           \
  do {                                                                         \
    double _val = (double)(val);                                               \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(double));       \
  } while (0)
#define GRN_TIME_SET GRN_INT64_SET
#define GRN_RECORD_SET(ctx, obj, val)                                          \
  do {                                                                         \
    grn_id _val = (grn_id)(val);                                               \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(grn_id));       \
  } while (0)
#define GRN_PTR_SET(ctx, obj, val)                                             \
  do {                                                                         \
    grn_obj *_val = (grn_obj *)(val);                                          \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(grn_obj *));    \
  } while (0)

#define GRN_GEO_DEGREE2MSEC(degree)                                            \
  ((int32_t)((degree)*3600 * 1000 + ((degree) > 0 ? 0.5 : -0.5)))
#define GRN_GEO_MSEC2DEGREE(msec) ((((int32_t)(msec)) / 3600.0) * 0.001)
#define GRN_GEO_RESOLUTION        3600000
#define GRN_GEO_M_PI              3.14159265358979323846
#define GRN_GEO_RADIAN2MSEC(radian)                                            \
  ((int32_t)(((GRN_GEO_RESOLUTION * 180) / GRN_GEO_M_PI) * (radian)))
#define GRN_GEO_MSEC2RADIAN(msec)                                              \
  ((GRN_GEO_M_PI / (GRN_GEO_RESOLUTION * 180)) * (msec))

#define GRN_GEO_POINT_SET(ctx, obj, _latitude, _longitude)                     \
  do {                                                                         \
    grn_geo_point _val;                                                        \
    _val.latitude = (int)(_latitude);                                          \
    _val.longitude = (int)(_longitude);                                        \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        0,                                                     \
                        sizeof(grn_geo_point));                                \
  } while (0)

#define GRN_BOOL_SET_AT(ctx, obj, offset, val)                                 \
  do {                                                                         \
    bool _val = (bool)(val);                                                   \
    grn_bulk_write_from((ctx), (obj), (char *)&_val, (offset), sizeof(bool));  \
  } while (0)
#define GRN_INT8_SET_AT(ctx, obj, offset, val)                                 \
  do {                                                                         \
    int8_t _val = (int8_t)(val);                                               \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(int8_t),                             \
                        sizeof(int8_t));                                       \
  } while (0)
#define GRN_UINT8_SET_AT(ctx, obj, offset, val)                                \
  do {                                                                         \
    uint8_t _val = (uint8_t)(val);                                             \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(uint8_t),                            \
                        sizeof(uint8_t));                                      \
  } while (0)
#define GRN_INT16_SET_AT(ctx, obj, offset, val)                                \
  do {                                                                         \
    int16_t _val = (int16_t)(val);                                             \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(int16_t),                            \
                        sizeof(int16_t));                                      \
  } while (0)
#define GRN_UINT16_SET_AT(ctx, obj, offset, val)                               \
  do {                                                                         \
    uint16_t _val = (uint16_t)(val);                                           \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(uint16_t),                           \
                        sizeof(uint16_t));                                     \
  } while (0)
#define GRN_INT32_SET_AT(ctx, obj, offset, val)                                \
  do {                                                                         \
    int32_t _val = (int32_t)(val);                                             \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(int32_t),                            \
                        sizeof(int32_t));                                      \
  } while (0)
#define GRN_UINT32_SET_AT(ctx, obj, offset, val)                               \
  do {                                                                         \
    uint32_t _val = (uint32_t)(val);                                           \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(uint32_t),                           \
                        sizeof(uint32_t));                                     \
  } while (0)
#define GRN_INT64_SET_AT(ctx, obj, offset, val)                                \
  do {                                                                         \
    int64_t _val = (int64_t)(val);                                             \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(int64_t),                            \
                        sizeof(int64_t));                                      \
  } while (0)
#define GRN_UINT64_SET_AT(ctx, obj, offset, val)                               \
  do {                                                                         \
    uint64_t _val = (uint64_t)(val);                                           \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(uint64_t),                           \
                        sizeof(uint64_t));                                     \
  } while (0)
#define GRN_BFLOAT16_SET_AT(ctx, obj, offset, val)                             \
  do {                                                                         \
    grn_bfloat16 _val = (grn_bfloat16)(val);                                   \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(grn_bfloat16),                       \
                        sizeof(grn_bfloat16));                                 \
  } while (0)
#define GRN_FLOAT32_SET_AT(ctx, obj, offset, val)                              \
  do {                                                                         \
    float _val = (float)(val);                                                 \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(float),                              \
                        sizeof(float));                                        \
  } while (0)
#define GRN_FLOAT_SET_AT(ctx, obj, offset, val)                                \
  do {                                                                         \
    double _val = (double)(val);                                               \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(double),                             \
                        sizeof(double));                                       \
  } while (0)
#define GRN_TIME_SET_AT GRN_INT64_SET_AT
#define GRN_RECORD_SET_AT(ctx, obj, offset, val)                               \
  do {                                                                         \
    grn_id _val = (grn_id)(val);                                               \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(grn_id),                             \
                        sizeof(grn_id));                                       \
  } while (0)
#define GRN_PTR_SET_AT(ctx, obj, offset, val)                                  \
  do {                                                                         \
    grn_obj *_val = (grn_obj *)(val);                                          \
    grn_bulk_write_from((ctx),                                                 \
                        (obj),                                                 \
                        (char *)&_val,                                         \
                        (offset) * sizeof(grn_obj *),                          \
                        sizeof(grn_obj *));                                    \
  } while (0)

#define GRN_BOOL_VALUE(obj)          (*((bool *)GRN_BULK_HEAD(obj)))
#define GRN_INT8_VALUE(obj)          (*((int8_t *)GRN_BULK_HEAD(obj)))
#define GRN_UINT8_VALUE(obj)         (*((uint8_t *)GRN_BULK_HEAD(obj)))
#define GRN_INT16_VALUE(obj)         (*((int16_t *)GRN_BULK_HEAD(obj)))
#define GRN_UINT16_VALUE(obj)        (*((uint16_t *)GRN_BULK_HEAD(obj)))
#define GRN_INT32_VALUE(obj)         (*((int32_t *)GRN_BULK_HEAD(obj)))
#define GRN_UINT32_VALUE(obj)        (*((uint32_t *)GRN_BULK_HEAD(obj)))
#define GRN_INT64_VALUE(obj)         (*((int64_t *)GRN_BULK_HEAD(obj)))
#define GRN_UINT64_VALUE(obj)        (*((uint64_t *)GRN_BULK_HEAD(obj)))
#define GRN_BFLOAT16_VALUE(obj)      (*((grn_bfloat16 *)GRN_BULK_HEAD(obj)))
#define GRN_FLOAT32_VALUE(obj)       (*((float *)GRN_BULK_HEAD(obj)))
#define GRN_FLOAT_VALUE(obj)         (*((double *)GRN_BULK_HEAD(obj)))
#define GRN_TIME_VALUE               GRN_INT64_VALUE
#define GRN_RECORD_VALUE(obj)        (*((grn_id *)GRN_BULK_HEAD(obj)))
#define GRN_PTR_VALUE(obj)           (*((grn_obj **)GRN_BULK_HEAD(obj)))
#define GRN_GEO_POINT_VALUE_RAW(obj) (grn_geo_point *)GRN_BULK_HEAD(obj)
#define GRN_GEO_POINT_VALUE(obj, _latitude, _longitude)                        \
  do {                                                                         \
    grn_geo_point *_val = GRN_GEO_POINT_VALUE_RAW(obj);                        \
    _latitude = _val->latitude;                                                \
    _longitude = _val->longitude;                                              \
  } while (0)
#define GRN_GEO_POINT_VALUE_RADIAN(obj, _latitude, _longitude)                 \
  do {                                                                         \
    grn_geo_point *_val = GRN_GEO_POINT_VALUE_RAW(obj);                        \
    _latitude = GRN_GEO_MSEC2RADIAN(_val->latitude);                           \
    _longitude = GRN_GEO_MSEC2RADIAN(_val->longitude);                         \
  } while (0)

#define GRN_BOOL_VALUE_AT(obj, offset) (((bool *)GRN_BULK_HEAD(obj))[offset])
#define GRN_INT8_VALUE_AT(obj, offset) (((int8_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_UINT8_VALUE_AT(obj, offset)                                        \
  (((uint8_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_INT16_VALUE_AT(obj, offset)                                        \
  (((int16_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_UINT16_VALUE_AT(obj, offset)                                       \
  (((uint16_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_INT32_VALUE_AT(obj, offset)                                        \
  (((int32_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_UINT32_VALUE_AT(obj, offset)                                       \
  (((uint32_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_INT64_VALUE_AT(obj, offset)                                        \
  (((int64_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_UINT64_VALUE_AT(obj, offset)                                       \
  (((uint64_t *)GRN_BULK_HEAD(obj))[offset])
#define GRN_BFLOAT16_VALUE_AT(obj, offset)                                     \
  (((grn_bfloat16 *)GRN_BULK_HEAD(obj))[offset])
#define GRN_FLOAT32_VALUE_AT(obj, offset)                                      \
  (((float *)GRN_BULK_HEAD(obj))[offset])
#define GRN_FLOAT_VALUE_AT(obj, offset) (((double *)GRN_BULK_HEAD(obj))[offset])
#define GRN_TIME_VALUE_AT               GRN_INT64_VALUE_AT
#define GRN_RECORD_VALUE_AT(obj, offset)                                       \
  (((grn_id *)GRN_BULK_HEAD(obj))[offset])
#define GRN_PTR_VALUE_AT(obj, offset) (((grn_obj **)GRN_BULK_HEAD(obj))[offset])

#define GRN_BOOL_PUT(ctx, obj, val)                                            \
  do {                                                                         \
    bool _val = (bool)(val);                                                   \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(bool));                 \
  } while (0)
#define GRN_INT8_PUT(ctx, obj, val)                                            \
  do {                                                                         \
    int8_t _val = (int8_t)(val);                                               \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(int8_t));               \
  } while (0)
#define GRN_UINT8_PUT(ctx, obj, val)                                           \
  do {                                                                         \
    uint8_t _val = (uint8_t)(val);                                             \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(uint8_t));              \
  } while (0)
#define GRN_INT16_PUT(ctx, obj, val)                                           \
  do {                                                                         \
    int16_t _val = (int16_t)(val);                                             \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(int16_t));              \
  } while (0)
#define GRN_UINT16_PUT(ctx, obj, val)                                          \
  do {                                                                         \
    uint16_t _val = (uint16_t)(val);                                           \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(uint16_t));             \
  } while (0)
#define GRN_INT32_PUT(ctx, obj, val)                                           \
  do {                                                                         \
    int32_t _val = (int32_t)(val);                                             \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(int32_t));              \
  } while (0)
#define GRN_UINT32_PUT(ctx, obj, val)                                          \
  do {                                                                         \
    uint32_t _val = (uint32_t)(val);                                           \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(unsigned int));         \
  } while (0)
#define GRN_INT64_PUT(ctx, obj, val)                                           \
  do {                                                                         \
    int64_t _val = (int64_t)(val);                                             \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(int64_t));              \
  } while (0)
#define GRN_UINT64_PUT(ctx, obj, val)                                          \
  do {                                                                         \
    uint64_t _val = (uint64_t)(val);                                           \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(uint64_t));             \
  } while (0)
#define GRN_BFLOAT16_PUT(ctx, obj, val)                                        \
  do {                                                                         \
    grn_bfloat16 _val = (grn_bfloat16)(val);                                   \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(grn_bfloat16));         \
  } while (0)
#define GRN_FLOAT32_PUT(ctx, obj, val)                                         \
  do {                                                                         \
    float _val = (float)(val);                                                 \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(float));                \
  } while (0)
#define GRN_FLOAT_PUT(ctx, obj, val)                                           \
  do {                                                                         \
    double _val = (double)(val);                                               \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(double));               \
  } while (0)
#define GRN_TIME_PUT GRN_INT64_PUT
#define GRN_RECORD_PUT(ctx, obj, val)                                          \
  do {                                                                         \
    grn_id _val = (grn_id)(val);                                               \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(grn_id));               \
  } while (0)
#define GRN_PTR_PUT(ctx, obj, val)                                             \
  do {                                                                         \
    grn_obj *_val = (grn_obj *)(val);                                          \
    grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(grn_obj *));            \
  } while (0)

#define GRN_BULK_POP(obj, value, type, default)                                \
  do {                                                                         \
    if (GRN_BULK_VSIZE(obj) >= sizeof(type)) {                                 \
      ssize_t value_size = (ssize_t)(sizeof(type));                            \
      GRN_BULK_INCR_LEN((obj), -value_size);                                   \
      value = *(type *)(GRN_BULK_CURR(obj));                                   \
    } else {                                                                   \
      value = default;                                                         \
    }                                                                          \
  } while (0)
#define GRN_BOOL_POP(obj, value)        GRN_BULK_POP(obj, value, bool, 0)
#define GRN_INT8_POP(obj, value)        GRN_BULK_POP(obj, value, int8_t, 0)
#define GRN_UINT8_POP(obj, value)       GRN_BULK_POP(obj, value, uint8_t, 0)
#define GRN_INT16_POP(obj, value)       GRN_BULK_POP(obj, value, int16_t, 0)
#define GRN_UINT16_POP(obj, value)      GRN_BULK_POP(obj, value, uint16_t, 0)
#define GRN_INT32_POP(obj, value)       GRN_BULK_POP(obj, value, int32_t, 0)
#define GRN_UINT32_POP(obj, value)      GRN_BULK_POP(obj, value, uint32_t, 0)
#define GRN_INT64_POP(obj, value)       GRN_BULK_POP(obj, value, int64_t, 0)
#define GRN_UINT64_POP(obj, value)      GRN_BULK_POP(obj, value, uint64_t, 0)
#define GRN_BFLOAT16_POP(obj, value)    GRN_BULK_POP(obj, value, grn_bfloat16, 0.0)
#define GRN_FLOAT32_POP(obj, value)     GRN_BULK_POP(obj, value, float, 0.0)
#define GRN_FLOAT_POP(obj, value)       GRN_BULK_POP(obj, value, double, 0.0)
#define GRN_TIME_POP                    GRN_INT64_POP
#define GRN_RECORD_POP(obj, value)      GRN_BULK_POP(obj, value, grn_id, GRN_ID_NIL)
#define GRN_PTR_POP(obj, value)         GRN_BULK_POP(obj, value, grn_obj *, NULL)

#define GRN_BULK_VECTOR_SIZE(obj, type) (GRN_BULK_VSIZE(obj) / sizeof(type))
#define GRN_BOOL_VECTOR_SIZE(obj)       GRN_BULK_VECTOR_SIZE(obj, bool)
#define GRN_INT8_VECTOR_SIZE(obj)       GRN_BULK_VECTOR_SIZE(obj, int8_t)
#define GRN_UINT8_VECTOR_SIZE(obj)      GRN_BULK_VECTOR_SIZE(obj, uint8_t)
#define GRN_INT16_VECTOR_SIZE(obj)      GRN_BULK_VECTOR_SIZE(obj, int16_t)
#define GRN_UINT16_VECTOR_SIZE(obj)     GRN_BULK_VECTOR_SIZE(obj, uint16_t)
#define GRN_INT32_VECTOR_SIZE(obj)      GRN_BULK_VECTOR_SIZE(obj, int32_t)
#define GRN_UINT32_VECTOR_SIZE(obj)     GRN_BULK_VECTOR_SIZE(obj, uint32_t)
#define GRN_INT64_VECTOR_SIZE(obj)      GRN_BULK_VECTOR_SIZE(obj, int64_t)
#define GRN_UINT64_VECTOR_SIZE(obj)     GRN_BULK_VECTOR_SIZE(obj, uint64_t)
#define GRN_BFLOAT16_VECTOR_SIZE(obj)   GRN_BULK_VECTOR_SIZE(obj, grn_bfloat16)
#define GRN_FLOAT32_VECTOR_SIZE(obj)    GRN_BULK_VECTOR_SIZE(obj, float)
#define GRN_FLOAT_VECTOR_SIZE(obj)      GRN_BULK_VECTOR_SIZE(obj, double)
#define GRN_TIME_VECTOR_SIZE            GRN_INT64_VECTOR_SIZE
#define GRN_RECORD_VECTOR_SIZE(obj)     GRN_BULK_VECTOR_SIZE(obj, grn_id)
#define GRN_PTR_VECTOR_SIZE(obj)        GRN_BULK_VECTOR_SIZE(obj, grn_obj *)

GRN_API grn_rc
grn_ctx_push(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_obj *
grn_ctx_pop(grn_ctx *ctx);

GRN_API grn_rc
grn_obj_columns(grn_ctx *ctx,
                grn_obj *table,
                const char *str,
                unsigned int str_size,
                grn_obj *res);

GRN_API grn_rc
grn_load(grn_ctx *ctx,
         grn_content_type input_type,
         const char *table,
         unsigned int table_len,
         const char *columns,
         unsigned int columns_len,
         const char *values,
         unsigned int values_len,
         const char *ifexists,
         unsigned int ifexists_len,
         const char *each,
         unsigned int each_len);

#define GRN_CTX_MORE  (0x01 << 0)
#define GRN_CTX_TAIL  (0x01 << 1)
#define GRN_CTX_HEAD  (0x01 << 2)
#define GRN_CTX_QUIET (0x01 << 3)
#define GRN_CTX_QUIT  (0x01 << 4)

GRN_API grn_rc
grn_ctx_connect(grn_ctx *ctx, const char *host, int port, int flags);
GRN_API unsigned int
grn_ctx_send(grn_ctx *ctx, const char *str, unsigned int str_len, int flags);
GRN_API grn_rc
grn_ctx_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags);

typedef struct _grn_ctx_info grn_ctx_info;

struct _grn_ctx_info {
  int fd;
  unsigned int com_status;
  grn_obj *outbuf;
  unsigned char stat;
};

GRN_API grn_rc
grn_ctx_info_get(grn_ctx *ctx, grn_ctx_info *info);

GRN_API grn_rc
grn_set_segv_handler(void);
GRN_API grn_rc
grn_set_abrt_handler(void);
GRN_API grn_rc
grn_set_int_handler(void);
GRN_API grn_rc
grn_set_term_handler(void);

typedef struct _grn_table_delete_optarg grn_table_delete_optarg;

struct _grn_table_delete_optarg {
  int flags;
  int (*func)(grn_ctx *ctx, grn_obj *, grn_id, void *);
  void *func_arg;
};

struct _grn_table_scan_hit {
  grn_id id;
  unsigned int offset;
  unsigned int length;
};

typedef struct {
  int64_t tv_sec;
  int32_t tv_nsec;
} grn_timeval;

#ifdef __cplusplus
}
#endif
