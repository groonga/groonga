/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2025  Sutou Kouhei <kou@clear-code.com>
  Copyright (C) 2021  Horimoto Yasuhiro <horimoto@clear-code.com>

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

#include "grn_cache.h"
#include "grn_ctx.h"
#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_expr.h"
#include "grn_float.h"
#include "grn_geo.h"
#include "grn_ii.h"
#include "grn_load.h"
#include "grn_output.h"
#include "grn_pat.h"
#include "grn_posting.h"
#include "grn_proc.h"
#include "grn_report.h"
#include "grn_str.h"
#include "grn_util.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef WIN32
#  include <io.h>
#  include <share.h>
#endif /* WIN32 */

#ifdef GRN_WITH_APACHE_ARROW
#  include <arrow/util/config.h>
#endif

#ifndef O_NOFOLLOW
#  define O_NOFOLLOW 0
#endif

/**** globals for procs ****/
const char *grn_document_root = NULL;

#define VAR GRN_PROC_GET_VAR_BY_OFFSET

static double grn_between_too_many_index_match_ratio = 0.01;
static double grn_in_values_too_many_index_match_ratio = 0.01;
static int32_t grn_sub_filter_pre_filter_threshold = 10;

void
grn_proc_init_from_env(void)
{
  {
    char grn_between_too_many_index_match_ratio_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO",
               grn_between_too_many_index_match_ratio_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_between_too_many_index_match_ratio_env[0]) {
      grn_between_too_many_index_match_ratio =
        atof(grn_between_too_many_index_match_ratio_env);
    }
  }

  {
    char grn_in_values_too_many_index_match_ratio_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO",
               grn_in_values_too_many_index_match_ratio_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_in_values_too_many_index_match_ratio_env[0]) {
      grn_in_values_too_many_index_match_ratio =
        atof(grn_in_values_too_many_index_match_ratio_env);
    }
  }

  {
    char env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_SUB_FILTER_PRE_FILTER_THRESHOLD", env, GRN_ENV_BUFFER_SIZE);
    if (env[0]) {
      grn_sub_filter_pre_filter_threshold =
        grn_atoi(env, env + strlen(env), NULL);
    }
  }
}

/* bulk must be initialized grn_bulk or grn_msg */
static int
grn_bulk_put_from_file(grn_ctx *ctx, grn_obj *bulk, const char *path)
{
  /* FIXME: implement more smartly with grn_bulk */
  int fd, ret = 0;
  struct stat stat;
  grn_open(fd, path, O_RDONLY | O_NOFOLLOW | GRN_OPEN_FLAG_BINARY);
  if (fd == -1) {
    switch (errno) {
    case EACCES:
      ERR(GRN_OPERATION_NOT_PERMITTED, "request is not allowed: <%s>", path);
      break;
    case ENOENT:
      ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY, "no such file: <%s>", path);
      break;
#ifndef WIN32
    case ELOOP:
      ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY,
          "symbolic link is not allowed: <%s>",
          path);
      break;
#endif /* WIN32 */
    default:
      ERRNO_ERR("failed to open file: <%s>", path);
      break;
    }
    return 0;
  }
  if (fstat(fd, &stat) != -1) {
    char *buf, *bp;
    off_t rest = stat.st_size;
    if ((buf = GRN_MALLOC(rest))) {
      ssize_t ss;
      for (bp = buf; rest; rest -= ss, bp += ss) {
        if ((ss = grn_read(fd, bp, rest)) == -1) {
          goto exit;
        }
      }
      GRN_TEXT_PUT(ctx, bulk, buf, stat.st_size);
      ret = 1;
    }
    GRN_FREE(buf);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "cannot stat file: <%s>", path);
  }
exit:
  grn_close(fd);
  return ret;
}

#ifdef stat
#  undef stat
#endif /* stat */

grn_rc
grn_proc_prefixed_options_parse(grn_ctx *ctx,
                                grn_obj *options,
                                const char *prefix,
                                const char *tag,
                                const char *name,
                                ...)
{
  va_list args;

  va_start(args, name);
  grn_rc rc =
    grn_proc_prefixed_options_parsev(ctx, options, prefix, tag, name, args);
  va_end(args);

  return rc;
}

grn_rc
grn_proc_prefixed_options_parsev(grn_ctx *ctx,
                                 grn_obj *options,
                                 const char *prefix,
                                 const char *tag,
                                 const char *name,
                                 va_list args)
{
  GRN_API_ENTER;

  grn_obj func_tag_buffer;
  GRN_TEXT_INIT(&func_tag_buffer, 0);
  grn_text_printf(ctx, &func_tag_buffer, "%s[options][parse]", tag);
  GRN_TEXT_PUTC(ctx, &func_tag_buffer, '\0');
  const char *func_tag = GRN_TEXT_VALUE(&func_tag_buffer);

  grn_obj full_name;
  GRN_TEXT_INIT(&full_name, 0);

  grn_obj used_ids;
  GRN_RECORD_INIT(&used_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);

  if (options->header.type != GRN_TABLE_HASH_KEY) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, options);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s options must be a hash table: %.*s",
                     func_tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }

  const size_t n_specified_options = grn_table_size(ctx, options);
  while (name && GRN_RECORD_VECTOR_SIZE(&used_ids) < n_specified_options) {
    grn_proc_option_value_type type = va_arg(args, grn_proc_option_value_type);
    void *value_raw = NULL;
    GRN_TEXT_SETS(ctx, &full_name, prefix);
    GRN_TEXT_PUTS(ctx, &full_name, name);
    GRN_TEXT_PUTC(ctx, &full_name, '\0');
    grn_id id = grn_hash_get(ctx,
                             (grn_hash *)options,
                             GRN_TEXT_VALUE(&full_name),
                             GRN_TEXT_LEN(&full_name) - 1,
                             &value_raw);
    grn_obj *value = value_raw;
    switch (type) {
    case GRN_PROC_OPTION_VALUE_RAW:
      {
        grn_obj **raw = va_arg(args, grn_obj **);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *raw = value;
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_MODE:
      {
        grn_operator *mode = va_arg(args, grn_operator *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *mode = grn_proc_get_value_mode(ctx, value, *mode, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_OPERATOR:
      {
        grn_operator *op = va_arg(args, grn_operator *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *op = grn_proc_get_value_operator(ctx, value, *op, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_EXPR_FLAGS:
      {
        grn_expr_flags *flags = va_arg(args, grn_expr_flags *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *flags = grn_proc_expr_query_flags_parse(ctx, value, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_INT32:
      {
        int32_t *number = va_arg(args, int32_t *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *number = grn_proc_get_value_int32(ctx, value, *number, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_UINT32:
      {
        uint32_t *number = va_arg(args, uint32_t *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *number = grn_proc_get_value_uint32(ctx, value, *number, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_INT64:
      {
        int64_t *number = va_arg(args, int64_t *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *number = grn_proc_get_value_int64(ctx, value, *number, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_BOOL:
      {
        bool *b = va_arg(args, bool *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *b = grn_proc_get_value_bool(ctx, value, *b, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_FUNC:
      {
        grn_proc_option_value_parse_func func =
          va_arg(args, grn_proc_option_value_parse_func);
        void *user_data = va_arg(args, void *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          grn_rc rc =
            func(ctx, GRN_TEXT_VALUE(&full_name), value, func_tag, user_data);
          if (rc != GRN_SUCCESS) {
            if (ctx->rc == GRN_SUCCESS) {
              grn_obj inspected;
              GRN_TEXT_INIT(&inspected, 0);
              grn_inspect(ctx, &inspected, value);
              GRN_PLUGIN_ERROR(ctx,
                               rc,
                               "%s[%s] failed to parse: <%.*s>",
                               func_tag,
                               name,
                               (int)GRN_TEXT_LEN(&inspected),
                               GRN_TEXT_VALUE(&inspected));
              GRN_OBJ_FIN(ctx, &inspected);
            }
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_TOKENIZE_MODE:
      {
        grn_tokenize_mode *mode = va_arg(args, grn_tokenize_mode *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *mode = grn_proc_get_value_tokenize_mode(ctx, value, *mode, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_TOKEN_CURSOR_FLAGS:
      {
        uint32_t *flags = va_arg(args, uint32_t *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *flags =
            grn_proc_get_value_token_cursor_flags(ctx, value, *flags, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_DOUBLE:
      {
        double *number = va_arg(args, double *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *number = grn_proc_get_value_double(ctx, value, *number, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    case GRN_PROC_OPTION_VALUE_RAW_STRING:
      {
        grn_raw_string *string = va_arg(args, grn_raw_string *);
        if (id != GRN_ID_NIL) {
          GRN_RECORD_PUT(ctx, &used_ids, id);
          *string = grn_proc_get_value_raw_string(ctx, value, string, func_tag);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
      break;
    default:
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s[%s] invalid option value type: %d",
                       func_tag,
                       GRN_TEXT_VALUE(&full_name),
                       type);
      goto exit;
    }
    name = va_arg(args, const char *);
  }

  const size_t n_used_options = GRN_RECORD_VECTOR_SIZE(&used_ids);
  if (n_used_options == n_specified_options) {
    goto exit;
  }

  {
    grn_obj message;
    GRN_TEXT_INIT(&message, 0);
    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)options, cursor, id)
    {
      bool is_used_option = false;
      size_t i;
      for (i = 0; i < n_used_options; i++) {
        if (GRN_RECORD_VALUE_AT(&used_ids, i) == id) {
          is_used_option = true;
          break;
        }
      }
      if (is_used_option) {
        continue;
      }

      void *key;
      int key_size = grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_raw_string key_string;
      key_string.value = key;
      key_string.length = key_size;
      if (!GRN_RAW_STRING_START_WITH_CSTRING(key_string, prefix)) {
        continue;
      }
      if (GRN_TEXT_LEN(&message) > 0) {
        GRN_TEXT_PUTS(ctx, &message, ", ");
      }
      grn_text_printf(ctx, &message, "<%.*s>", key_size, (char *)key);
    }
    GRN_HASH_EACH_END(ctx, cursor);
    if (GRN_TEXT_LEN(&message) > 0) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, options);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s unknown option names: %.*s: %.*s",
                       func_tag,
                       (int)GRN_TEXT_LEN(&message),
                       GRN_TEXT_VALUE(&message),
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    GRN_OBJ_FIN(ctx, &message);
  }

exit:
  GRN_OBJ_FIN(ctx, &used_ids);
  GRN_OBJ_FIN(ctx, &full_name);
  GRN_OBJ_FIN(ctx, &func_tag_buffer);
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_proc_options_parse(
  grn_ctx *ctx, grn_obj *options, const char *tag, const char *name, ...)
{
  va_list args;

  va_start(args, name);
  grn_rc rc = grn_proc_options_parsev(ctx, options, tag, name, args);
  va_end(args);

  return rc;
}

grn_rc
grn_proc_options_parsev(grn_ctx *ctx,
                        grn_obj *options,
                        const char *tag,
                        const char *name,
                        va_list args)
{
  return grn_proc_prefixed_options_parsev(ctx, options, "", tag, name, args);
}

static void
grn_proc_func_generate_cache_key_add(grn_ctx *ctx,
                                     grn_obj *cache_key,
                                     grn_obj *arg)
{
  switch (arg->header.type) {
  case GRN_TABLE_HASH_KEY:
    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)arg, cursor, id)
    {
      void *key;
      unsigned int key_size;
      void *value;
      int value_size =
        grn_hash_cursor_get_key_value(ctx, cursor, &key, &key_size, &value);
      GRN_TEXT_PUT(ctx, cache_key, key, key_size);
      GRN_TEXT_PUTC(ctx, cache_key, '\1');
      if (value_size == sizeof(grn_obj)) {
        grn_proc_func_generate_cache_key_add(ctx, cache_key, (grn_obj *)value);
      }
      GRN_TEXT_PUTC(ctx, cache_key, '\2');
    }
    GRN_HASH_EACH_END(ctx, cursor);
    break;
  default:
    GRN_TEXT_PUT(ctx, cache_key, GRN_BULK_HEAD(arg), GRN_BULK_VSIZE(arg));
    break;
  }
}

grn_rc
grn_proc_func_generate_cache_key(grn_ctx *ctx,
                                 const char *function_name,
                                 grn_obj **args,
                                 int n_args,
                                 grn_obj *cache_key)
{
  GRN_API_ENTER;
  GRN_TEXT_PUTS(ctx, cache_key, function_name);
  GRN_TEXT_PUTC(ctx, cache_key, '\0');
  int i;
  for (i = 0; i < n_args; i++) {
    grn_proc_func_generate_cache_key_add(ctx, cache_key, args[i]);
    GRN_TEXT_PUTC(ctx, cache_key, '\0');
  }
  GRN_API_RETURN(ctx->rc);
}

/**** procs ****/

static grn_obj *
proc_load(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_load_input input;

  input.type = grn_plugin_proc_get_var_content_type(ctx,
                                                    user_data,
                                                    "input_type",
                                                    -1,
                                                    GRN_CONTENT_JSON);
#define INIT_STRING_ARGUMENT(member_name, arg_name)                            \
  input.member_name.value =                                                    \
    grn_plugin_proc_get_var_string(ctx,                                        \
                                   user_data,                                  \
                                   arg_name,                                   \
                                   -1,                                         \
                                   &(input.member_name.length))

  INIT_STRING_ARGUMENT(table, "table");
  INIT_STRING_ARGUMENT(columns, "columns");
  INIT_STRING_ARGUMENT(values, "values");
  INIT_STRING_ARGUMENT(if_exists, "ifexists");
  INIT_STRING_ARGUMENT(each, "each");

#undef INIT_STRING_ARGUMENT

  input.output_ids =
    grn_plugin_proc_get_var_bool(ctx, user_data, "output_ids", -1, false);
  input.output_errors =
    grn_plugin_proc_get_var_bool(ctx, user_data, "output_errors", -1, false);
  input.lock_table =
    grn_plugin_proc_get_var_bool(ctx, user_data, "lock_table", -1, false);
  input.emit_level = 1;

  grn_load_internal(ctx, &input);
  if (ctx->rc != GRN_SUCCESS) {
    ctx->impl->loader.stat = GRN_LOADER_END;
    if (ctx->rc == GRN_CANCEL) {
      ctx->impl->loader.error.rc = GRN_SUCCESS;
    }
  }
  bool accept_more_data = true;
  if (ctx->impl->loader.stat == GRN_LOADER_END) {
    accept_more_data = false;
  }
  if (ctx->impl->command.flags & GRN_CTX_TAIL) {
    accept_more_data = false;
  }
  if (accept_more_data) {
    grn_obj *command = grn_proc_get_info(ctx, user_data, NULL, NULL, NULL);
    grn_ctx_set_keep_command(ctx, command);
    return NULL;
  }

  if (ctx->impl->loader.error.rc != GRN_SUCCESS) {
    ctx->rc = ctx->impl->loader.error.rc;
    grn_strcpy(ctx->errbuf, GRN_CTX_MSGSIZE, ctx->impl->loader.error.buffer);
    ctx->errline = ctx->impl->loader.error.line;
    ctx->errfile = ctx->impl->loader.error.file;
    ctx->errfunc = ctx->impl->loader.error.func;
  }
  {
    unsigned int n_records;
    if (ctx->impl->loader.table) {
      n_records = grn_table_size(ctx, ctx->impl->loader.table);
    } else {
      n_records = 0;
    }
    GRN_QUERY_LOG(ctx,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "load(%d): [%d][%d][%d]",
                  ctx->impl->loader.n_records,
                  ctx->impl->loader.n_record_errors,
                  ctx->impl->loader.n_column_errors,
                  n_records);
  }
  if (grn_ctx_get_command_version(ctx) >= GRN_COMMAND_VERSION_3) {
    int n_elements = 1;
    if (ctx->impl->loader.output_ids) {
      n_elements++;
    }
    if (ctx->impl->loader.output_errors) {
      n_elements++;
    }
    GRN_OUTPUT_MAP_OPEN("result", n_elements);
    GRN_OUTPUT_CSTR("n_loaded_records");
    GRN_OUTPUT_INT64(ctx->impl->loader.n_records);
    if (ctx->impl->loader.output_ids) {
      grn_obj *ids = &(ctx->impl->loader.ids);
      int i, n_ids;

      GRN_OUTPUT_CSTR("loaded_ids");
      n_ids = GRN_BULK_VSIZE(ids) / sizeof(uint32_t);
      GRN_OUTPUT_ARRAY_OPEN("loaded_ids", n_ids);
      for (i = 0; i < n_ids; i++) {
        GRN_OUTPUT_UINT64(GRN_UINT32_VALUE_AT(ids, i));
      }
      GRN_OUTPUT_ARRAY_CLOSE();
    }
    if (ctx->impl->loader.output_errors) {
      grn_obj *return_codes = &(ctx->impl->loader.return_codes);
      grn_obj *error_messages = &(ctx->impl->loader.error_messages);
      int i, n;

      GRN_OUTPUT_CSTR("errors");
      n = GRN_BULK_VSIZE(return_codes) / sizeof(int32_t);
      GRN_OUTPUT_ARRAY_OPEN("errors", n);
      for (i = 0; i < n; i++) {
        const char *message;
        unsigned int message_size;

        message_size =
          grn_vector_get_element(ctx, error_messages, i, &message, NULL, NULL);

        GRN_OUTPUT_MAP_OPEN("error", 2);
        GRN_OUTPUT_CSTR("return_code");
        GRN_OUTPUT_INT64(GRN_INT32_VALUE_AT(return_codes, i));
        GRN_OUTPUT_CSTR("message");
        if (message_size == 0) {
          GRN_OUTPUT_NULL();
        } else {
          GRN_OUTPUT_STR(message, message_size);
        }
        GRN_OUTPUT_MAP_CLOSE();
      }
      GRN_OUTPUT_ARRAY_CLOSE();
    }
    GRN_OUTPUT_MAP_CLOSE();
  } else {
    GRN_OUTPUT_INT64(ctx->impl->loader.n_records);
  }
  if (ctx->impl->loader.table) {
    grn_db_touch(ctx, DB_OBJ(ctx->impl->loader.table)->db);
  }
  grn_ctx_loader_clear(ctx);

  return NULL;
}

static grn_obj *
proc_status(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_timeval now;
  grn_cache *cache;
  grn_cache_statistics statistics;
  int n_elements = 17;
#ifdef GRN_WITH_APACHE_ARROW
  n_elements++;
#endif

  grn_timeval_now(ctx, &now);
  cache = grn_cache_current_get(ctx);
  grn_cache_get_statistics(ctx, cache, &statistics);
  GRN_OUTPUT_MAP_OPEN("RESULT", n_elements);
  GRN_OUTPUT_CSTR("alloc_count");
  GRN_OUTPUT_INT64(grn_alloc_count());
  GRN_OUTPUT_CSTR("starttime");
  GRN_OUTPUT_INT64(grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("start_time");
  GRN_OUTPUT_INT64(grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("uptime");
  GRN_OUTPUT_INT64(now.tv_sec - grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("version");
  GRN_OUTPUT_CSTR(grn_get_version());
  GRN_OUTPUT_CSTR("n_queries");
  GRN_OUTPUT_INT64(statistics.nfetches);
  GRN_OUTPUT_CSTR("cache_hit_rate");
  if (statistics.nfetches == 0) {
    GRN_OUTPUT_FLOAT(0.0);
  } else {
    double cache_hit_rate;
    cache_hit_rate = (double)statistics.nhits / (double)statistics.nfetches;
    GRN_OUTPUT_FLOAT(cache_hit_rate * 100.0);
  }
  GRN_OUTPUT_CSTR("command_version");
  GRN_OUTPUT_INT32(grn_ctx_get_command_version(ctx));
  GRN_OUTPUT_CSTR("default_command_version");
  GRN_OUTPUT_INT32(grn_get_default_command_version());
  GRN_OUTPUT_CSTR("max_command_version");
  GRN_OUTPUT_INT32(GRN_COMMAND_VERSION_MAX);
  {
    grn_com_queue *job_queue;
    job_queue = grn_job_queue_current_get(ctx);
    GRN_OUTPUT_CSTR("n_jobs");
    if (job_queue) {
      GRN_OUTPUT_UINT64(grn_com_queue_size(ctx, job_queue));
    } else {
      GRN_OUTPUT_UINT64(0);
    }
  }
  GRN_OUTPUT_CSTR("features");
  {
    const int n_features = 20;
    GRN_OUTPUT_MAP_OPEN("features", n_features);

    GRN_OUTPUT_CSTR("nfkc");
#ifndef NO_NFKC
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("mecab");
#ifdef GRN_WITH_MECAB
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("message_pack");
#ifdef GRN_WITH_MESSAGE_PACK
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("mruby");
#ifdef GRN_WITH_MRUBY
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("onigmo");
#ifdef GRN_WITH_ONIGMO
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("zlib");
#ifdef GRN_WITH_ZLIB
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("lz4");
#ifdef GRN_WITH_LZ4
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("zstandard");
#ifdef GRN_WITH_ZSTD
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("kqueue");
#ifdef USE_KQUEUE
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("epoll");
#ifdef USE_EPOLL
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("poll");
#ifdef USE_POLL
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("rapidjson");
#ifdef GRN_WITH_RAPIDJSON
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("apache_arrow");
#ifdef GRN_WITH_APACHE_ARROW
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("xxhash");
#ifdef GRN_WITH_XXHASH
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("blosc");
#ifdef GRN_WITH_BLOSC
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("bfloat16");
#ifdef GRN_HAVE_BFLOAT16
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("h3");
#ifdef GRN_WITH_H3
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("simdjson");
#ifdef GRN_WITH_SIMDJSON
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("llama.cpp");
#ifdef GRN_WITH_LLAMA_CPP
    GRN_OUTPUT_BOOL(true);
#else
    GRN_OUTPUT_BOOL(false);
#endif

    GRN_OUTPUT_CSTR("back_trace");
    GRN_OUTPUT_BOOL(grn_is_back_trace_enable());

    GRN_OUTPUT_CSTR("reference_count");
    GRN_OUTPUT_BOOL(grn_is_reference_count_enable());

    GRN_OUTPUT_MAP_CLOSE();
  }
#ifdef GRN_WITH_APACHE_ARROW
  GRN_OUTPUT_CSTR("apache_arrow");
  {
    const int n_apache_arrow_elements = 4;
    GRN_OUTPUT_MAP_OPEN("apache_arrow", n_apache_arrow_elements);

    GRN_OUTPUT_CSTR("version_major");
    GRN_OUTPUT_INT32(ARROW_VERSION_MAJOR);

    GRN_OUTPUT_CSTR("version_minor");
    GRN_OUTPUT_INT32(ARROW_VERSION_MINOR);

    GRN_OUTPUT_CSTR("version_patch");
    GRN_OUTPUT_INT32(ARROW_VERSION_PATCH);

    GRN_OUTPUT_CSTR("version");
    GRN_OUTPUT_CSTR(ARROW_VERSION_STRING);

    GRN_OUTPUT_MAP_CLOSE();
  }
#endif
  GRN_OUTPUT_CSTR("memory_map_size");
  GRN_OUTPUT_UINT64(grn_get_memory_map_size());
  GRN_OUTPUT_CSTR("n_workers");
  GRN_OUTPUT_INT32(grn_ctx_get_n_workers(ctx));
  GRN_OUTPUT_CSTR("default_n_workers");
  GRN_OUTPUT_INT32(grn_get_default_n_workers());
  GRN_OUTPUT_CSTR("os");
#ifdef HOST_OS
  GRN_OUTPUT_CSTR(HOST_OS);
#else
  GRN_OUTPUT_CSTR("Unknown");
#endif
  GRN_OUTPUT_CSTR("cpu");
#ifdef HOST_CPU
  GRN_OUTPUT_CSTR(HOST_CPU);
#else
  GRN_OUTPUT_CSTR("unknown");
#endif
  GRN_OUTPUT_MAP_CLOSE();

#ifdef GRN_WITH_MEMORY_DEBUG
  if (grn_plugin_proc_get_var_bool(ctx,
                                   user_data,
                                   "dump_alloc_info",
                                   -1,
                                   false)) {
    grn_alloc_info_dump(&grn_gctx);
  }
#endif

  return NULL;
}

#define GRN_STRLEN(s) ((s) ? strlen(s) : 0)

void
grn_proc_output_object_name(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj bulk;
  int name_len;
  char name[GRN_TABLE_MAX_KEY_SIZE];

  if (obj) {
    GRN_TEXT_INIT(&bulk, GRN_OBJ_DO_SHALLOW_COPY);
    name_len = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
    GRN_TEXT_SET(ctx, &bulk, name, name_len);
  } else {
    GRN_VOID_INIT(&bulk);
  }

  GRN_OUTPUT_OBJ(&bulk, NULL);
  GRN_OBJ_FIN(ctx, &bulk);
}

void
grn_proc_output_object_id_name(grn_ctx *ctx, grn_id id)
{
  grn_obj *obj = NULL;

  if (id != GRN_ID_NIL) {
    obj = grn_ctx_at(ctx, id);
  }

  grn_proc_output_object_name(ctx, obj);
}

static grn_obj *
proc_missing(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t plen;
  grn_obj *outbuf = ctx->impl->output.buf;
  static int grn_document_root_len = -1;
  if (!grn_document_root) {
    return NULL;
  }
  if (grn_document_root_len < 0) {
    size_t l;
    if ((l = strlen(grn_document_root)) > PATH_MAX) {
      return NULL;
    }
    grn_document_root_len = (int)l;
    if (l > 0 && grn_document_root[l - 1] == '/') {
      grn_document_root_len--;
    }
  }
  if ((plen = GRN_TEXT_LEN(VAR(0))) + grn_document_root_len < PATH_MAX) {
    char path[PATH_MAX];
    grn_memcpy(path, grn_document_root, grn_document_root_len);
    path[grn_document_root_len] = '/';
    grn_str_url_path_normalize(ctx,
                               GRN_TEXT_VALUE(VAR(0)),
                               GRN_TEXT_LEN(VAR(0)),
                               path + grn_document_root_len + 1,
                               PATH_MAX - grn_document_root_len - 1);
    grn_bulk_put_from_file(ctx, outbuf, path);
  } else {
    uint32_t abbrlen = 32;
    ERR(GRN_INVALID_ARGUMENT,
        "too long path name: <%s/%.*s...> %u(%u)",
        grn_document_root,
        abbrlen < plen ? abbrlen : plen,
        GRN_TEXT_VALUE(VAR(0)),
        plen + grn_document_root_len,
        PATH_MAX);
  }
  return NULL;
}

static grn_obj *
proc_quit(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  ctx->stat = GRN_CTX_QUITTING;
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_shutdown(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  const char *mode;
  size_t mode_size;

  mode = grn_plugin_proc_get_var_string(ctx, user_data, "mode", -1, &mode_size);
#define MODE_EQUAL(name)                                                       \
  (mode_size == strlen(name) && memcmp(mode, name, mode_size) == 0)
  if (mode_size == 0 || MODE_EQUAL("graceful")) {
    /* Do nothing. This is the default. */
  } else if (MODE_EQUAL("immediate")) {
    grn_request_canceler_cancel_all();
    if (ctx->rc == GRN_INTERRUPTED_FUNCTION_CALL) {
      ctx->rc = GRN_SUCCESS;
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "[shutdown] mode must be <graceful> or <immediate>: <%.*s>",
        (int)mode_size,
        mode);
  }
#undef MODE_EQUAL

  if (ctx->rc == GRN_SUCCESS) {
    grn_gctx.stat = GRN_CTX_QUIT;
    ctx->stat = GRN_CTX_QUITTING;
  }

  GRN_OUTPUT_BOOL(!ctx->rc);

  return NULL;
}

static grn_obj *
proc_defrag(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  int olen, threshold;
  olen = GRN_TEXT_LEN(VAR(0));

  if (olen) {
    obj = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), olen);
  } else {
    obj = ctx->impl->db;
  }

  threshold = GRN_TEXT_LEN(VAR(1))
                ? grn_atoi(GRN_TEXT_VALUE(VAR(1)), GRN_BULK_CURR(VAR(1)), NULL)
                : 0;

  if (obj) {
    grn_obj_defrag(ctx, obj, threshold);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "defrag object not found");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_rc
proc_delete_validate_selector(grn_ctx *ctx,
                              grn_obj *table,
                              grn_obj *table_name,
                              grn_obj *key,
                              grn_obj *id,
                              grn_obj *filter)
{
  grn_rc rc = GRN_SUCCESS;

  if (!table) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] table doesn't exist: <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name));
    return rc;
  }

  if (GRN_TEXT_LEN(key) == 0 && GRN_TEXT_LEN(id) == 0 &&
      GRN_TEXT_LEN(filter) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] either key, id or filter must be specified: "
        "table: <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name));
    return rc;
  }

  if (GRN_TEXT_LEN(key) && GRN_TEXT_LEN(id) && GRN_TEXT_LEN(filter)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "record selector must be one of key, id and filter: "
        "table: <%.*s>, key: <%.*s>, id: <%.*s>, filter: <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(key),
        GRN_TEXT_VALUE(key),
        (int)GRN_TEXT_LEN(id),
        GRN_TEXT_VALUE(id),
        (int)GRN_TEXT_LEN(filter),
        GRN_TEXT_VALUE(filter));
    return rc;
  }

  if (GRN_TEXT_LEN(key) && GRN_TEXT_LEN(id) && GRN_TEXT_LEN(filter) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "can't use both key and id: table: <%.*s>, key: <%.*s>, id: <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(key),
        GRN_TEXT_VALUE(key),
        (int)GRN_TEXT_LEN(id),
        GRN_TEXT_VALUE(id));
    return rc;
  }

  if (GRN_TEXT_LEN(key) && GRN_TEXT_LEN(id) == 0 && GRN_TEXT_LEN(filter)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "can't use both key and filter: "
        "table: <%.*s>, key: <%.*s>, filter: <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(key),
        GRN_TEXT_VALUE(key),
        (int)GRN_TEXT_LEN(filter),
        GRN_TEXT_VALUE(filter));
    return rc;
  }

  if (GRN_TEXT_LEN(key) == 0 && GRN_TEXT_LEN(id) && GRN_TEXT_LEN(filter)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "can't use both id and filter: "
        "table: <%.*s>, id: <%.*s>, filter: <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(id),
        GRN_TEXT_VALUE(id),
        (int)GRN_TEXT_LEN(filter),
        GRN_TEXT_VALUE(filter));
    return rc;
  }

  return rc;
}

static grn_obj *
proc_delete(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_obj *table_name = VAR(0);
  grn_obj *key = VAR(1);
  grn_obj *id = VAR(2);
  grn_obj *filter = VAR(3);
  grn_obj *table = NULL;
  uint32_t n_deleted = 0;
  uint32_t n_errors = 0;
  int32_t limit;

  if (GRN_TEXT_LEN(table_name) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc, "[table][record][delete] table name isn't specified");
    goto exit;
  }

  table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(table_name), GRN_TEXT_LEN(table_name));
  rc = proc_delete_validate_selector(ctx, table, table_name, key, id, filter);
  if (rc != GRN_SUCCESS) {
    goto exit;
  }

  limit = grn_plugin_proc_get_var_int32(ctx, user_data, "limit", -1, -1);
  if (limit == 0) {
    goto exit;
  }

  if (GRN_TEXT_LEN(key)) {
    grn_obj casted_key;
    if (key->header.domain != table->header.domain) {
      GRN_OBJ_INIT(&casted_key, GRN_BULK, 0, table->header.domain);
      grn_obj_cast(ctx, key, &casted_key, false);
      key = &casted_key;
    }
    if (ctx->rc) {
      rc = ctx->rc;
    } else {
      if (limit < 0) {
        const int32_t n_records = 1;
        limit += n_records + 1;
      }
      if (limit >= 1) {
        rc =
          grn_table_delete(ctx, table, GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key));
        if (rc == GRN_SUCCESS) {
          n_deleted++;
        } else {
          n_errors++;
        }
      }
    }
    if (key == &casted_key) {
      GRN_OBJ_FIN(ctx, &casted_key);
    }
  } else if (GRN_TEXT_LEN(id)) {
    const char *end;
    grn_id parsed_id = grn_atoui(GRN_TEXT_VALUE(id), GRN_BULK_CURR(id), &end);
    if (end == GRN_BULK_CURR(id)) {
      if (limit < 0) {
        const int32_t n_records = 1;
        limit += n_records + 1;
      }
      if (limit >= 1) {
        rc = grn_table_delete_by_id(ctx, table, parsed_id);
        if (rc == GRN_SUCCESS) {
          n_deleted++;
        } else {
          n_errors++;
        }
      }
    } else {
      rc = GRN_INVALID_ARGUMENT;
      ERR(rc,
          "[table][record][delete] id should be number: "
          "table: <%.*s>, id: <%.*s>, detail: <%.*s|%c|%.*s>",
          (int)GRN_TEXT_LEN(table_name),
          GRN_TEXT_VALUE(table_name),
          (int)GRN_TEXT_LEN(id),
          GRN_TEXT_VALUE(id),
          (int)(end - GRN_TEXT_VALUE(id)),
          GRN_TEXT_VALUE(id),
          end[0],
          (int)(GRN_TEXT_VALUE(id) - end - 1),
          end + 1);
    }
  } else if (GRN_TEXT_LEN(filter)) {
    grn_obj *cond, *v;

    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, cond, v);
    grn_expr_parse(ctx,
                   cond,
                   GRN_TEXT_VALUE(filter),
                   GRN_TEXT_LEN(filter),
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc) {
      char errbuf[GRN_CTX_MSGSIZE];
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      rc = ctx->rc;
      ERR(rc,
          "[table][record][delete] failed to parse filter: "
          "table: <%.*s>, filter: <%.*s>, detail: <%s>",
          (int)GRN_TEXT_LEN(table_name),
          GRN_TEXT_VALUE(table_name),
          (int)GRN_TEXT_LEN(filter),
          GRN_TEXT_VALUE(filter),
          errbuf);
    } else {
      grn_obj *records;

      records = grn_table_select(ctx, table, cond, NULL, GRN_OP_OR);
      if (records) {
        if (limit < 0) {
          limit += grn_table_size(ctx, records) + 1;
        }
        GRN_TABLE_EACH_BEGIN(ctx, records, cursor, result_id)
        {
          void *key;
          grn_id id;
          grn_rc sub_rc;

          if (grn_table_cursor_get_key(ctx, cursor, &key) == 0) {
            continue;
          }

          if (n_deleted >= (uint32_t)limit) {
            break;
          }

          id = *(grn_id *)key;
          sub_rc = grn_table_delete_by_id(ctx, table, id);
          if (sub_rc == GRN_SUCCESS) {
            n_deleted++;
          } else {
            n_errors++;
          }
          if (rc == GRN_SUCCESS) {
            rc = sub_rc;
          }
          if (ctx->rc == GRN_CANCEL) {
            break;
          }
          if (ctx->rc != GRN_SUCCESS) {
            ERRCLR(ctx);
          }
        }
        GRN_TABLE_EACH_END(ctx, cursor);
        grn_obj_unlink(ctx, records);
      }
    }
    grn_obj_unlink(ctx, cond);
  }

exit:
  {
    unsigned int n_rest_records = 0;
    if (table) {
      n_rest_records = grn_table_size(ctx, table);
    }
    GRN_QUERY_LOG(ctx,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "delete(%u): [%u][%u]",
                  n_deleted,
                  n_errors,
                  n_rest_records);
  }
  if (table) {
    grn_obj_unlink(ctx, table);
  }
  GRN_OUTPUT_BOOL(rc == GRN_SUCCESS);
  return NULL;
}

bool
grn_proc_option_value_bool(grn_ctx *ctx, grn_obj *option, bool default_value)
{
  if (!option) {
    return default_value;
  }

  grn_raw_string value;
  GRN_RAW_STRING_SET(value, option);
  if (value.length == 0) {
    return default_value;
  }

  if (GRN_RAW_STRING_EQUAL_CSTRING(value, "yes") ||
      GRN_RAW_STRING_EQUAL_CSTRING(value, "true")) {
    return true;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(value, "no") ||
             GRN_RAW_STRING_EQUAL_CSTRING(value, "false")) {
    return false;
  } else {
    return default_value;
  }
}

int32_t
grn_proc_option_value_int32(grn_ctx *ctx,
                            grn_obj *option,
                            int32_t default_value)
{
  const char *value;
  size_t value_length;
  int32_t int32_value;
  const char *rest;

  if (!option) {
    return default_value;
  }

  value = GRN_TEXT_VALUE(option);
  value_length = GRN_TEXT_LEN(option);

  if (value_length == 0) {
    return default_value;
  }

  int32_value = grn_atoi(value, value + value_length, &rest);
  if (rest == value + value_length) {
    return int32_value;
  } else {
    return default_value;
  }
}

uint32_t
grn_proc_option_value_uint32(grn_ctx *ctx,
                             grn_obj *option,
                             uint32_t default_value)
{
  const char *value;
  size_t value_length;
  uint32_t uint32_value;
  const char *rest;

  if (!option) {
    return default_value;
  }

  value = GRN_TEXT_VALUE(option);
  value_length = GRN_TEXT_LEN(option);

  if (value_length == 0) {
    return default_value;
  }

  uint32_value = grn_atoui(value, value + value_length, &rest);
  if (rest == value + value_length) {
    return uint32_value;
  } else {
    return default_value;
  }
}

double
grn_proc_option_value_double(grn_ctx *ctx,
                             grn_obj *option,
                             double default_value)
{
  if (!option) {
    return default_value;
  }

  if (GRN_TEXT_LEN(option) == 0) {
    return default_value;
  }

  double value = default_value;
  grn_obj buffer;
  GRN_FLOAT_INIT(&buffer, 0);
  grn_rc rc = grn_obj_cast(ctx, option, &buffer, false);
  if (rc == GRN_SUCCESS) {
    value = GRN_FLOAT_VALUE(&buffer);
  }
  GRN_OBJ_FIN(ctx, &buffer);

  return value;
}

float
grn_proc_option_value_float(grn_ctx *ctx, grn_obj *option, float default_value)
{
  if (!option) {
    return default_value;
  }

  if (GRN_TEXT_LEN(option) == 0) {
    return default_value;
  }

  float value = default_value;
  grn_obj buffer;
  GRN_FLOAT_INIT(&buffer, 0);
  grn_rc rc = grn_obj_cast(ctx, option, &buffer, false);
  if (rc == GRN_SUCCESS) {
    value = GRN_FLOAT_VALUE(&buffer);
  }
  GRN_OBJ_FIN(ctx, &buffer);

  return value;
}

const char *
grn_proc_option_value_string(grn_ctx *ctx, grn_obj *option, size_t *size)
{
  const char *value;
  size_t value_length;

  if (!option) {
    if (size) {
      *size = 0;
    }
    return NULL;
  }

  value = GRN_TEXT_VALUE(option);
  value_length = GRN_TEXT_LEN(option);

  if (size) {
    *size = value_length;
  }

  if (value_length == 0) {
    return NULL;
  } else {
    return value;
  }
}

grn_content_type
grn_proc_option_value_content_type(grn_ctx *ctx,
                                   grn_obj *option,
                                   grn_content_type default_value)
{
  if (!option) {
    return default_value;
  }

  return grn_content_type_parse(ctx, option, default_value);
}

grn_log_level
grn_proc_option_value_log_level(grn_ctx *ctx,
                                grn_obj *option,
                                grn_log_level default_value)
{
  if (!option) {
    return default_value;
  }

  grn_obj value;
  GRN_TEXT_INIT(&value, 0);
  GRN_TEXT_SET(ctx, &value, GRN_TEXT_VALUE(option), GRN_TEXT_LEN(option));
  GRN_TEXT_PUTC(ctx, &value, '\0');
  grn_log_level level = default_value;
  if (!grn_log_level_parse(GRN_TEXT_VALUE(&value), &level)) {
    level = default_value;
  }
  GRN_OBJ_FIN(ctx, &value);
  return level;
}

bool
grn_proc_get_value_bool(grn_ctx *ctx,
                        grn_obj *value,
                        bool default_value,
                        const char *tag)
{
  if (!value) {
    return default_value;
  }

  if (value->header.domain == GRN_DB_BOOL) {
    return GRN_BOOL_VALUE(value);
  }

  if (grn_type_id_is_text_family(ctx, value->header.domain)) {
    return grn_proc_option_value_bool(ctx, value, default_value);
  }

  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, value);
  GRN_PLUGIN_ERROR(ctx,
                   GRN_INVALID_ARGUMENT,
                   "%s value must be a bool or string: <%.*s>",
                   tag,
                   (int)GRN_TEXT_LEN(&inspected),
                   GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
  return default_value;
}

int32_t
grn_proc_get_value_int32(grn_ctx *ctx,
                         grn_obj *value,
                         int32_t default_value_raw,
                         const char *tag)
{
  int32_t value_raw;

  if (!value) {
    return default_value_raw;
  }

  if (!grn_type_id_is_number_family(ctx, value->header.domain)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s value must be a number: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_value_raw;
  }

  if (value->header.domain == GRN_DB_INT32) {
    value_raw = GRN_INT32_VALUE(value);
  } else if (value->header.domain == GRN_DB_INT64) {
    value_raw = (int32_t)GRN_INT64_VALUE(value);
  } else {
    grn_obj buffer;
    grn_rc rc;

    GRN_INT32_INIT(&buffer, 0);
    rc = grn_obj_cast(ctx, value, &buffer, false);
    if (rc == GRN_SUCCESS) {
      value_raw = GRN_INT32_VALUE(&buffer);
    }
    GRN_OBJ_FIN(ctx, &buffer);

    if (rc != GRN_SUCCESS) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      GRN_PLUGIN_ERROR(ctx,
                       rc,
                       "%s "
                       "failed to cast value to number: <%.*s>",
                       tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      value_raw = default_value_raw;
    }
  }

  return value_raw;
}

uint32_t
grn_proc_get_value_uint32(grn_ctx *ctx,
                          grn_obj *value,
                          uint32_t default_value_raw,
                          const char *tag)
{
  uint32_t value_raw;

  if (!value) {
    return default_value_raw;
  }

  if (!grn_type_id_is_number_family(ctx, value->header.domain)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s value must be a number: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_value_raw;
  }

  if (value->header.domain == GRN_DB_UINT32) {
    value_raw = GRN_UINT32_VALUE(value);
  } else if (value->header.domain == GRN_DB_INT32) {
    value_raw = (uint32_t)GRN_INT32_VALUE(value);
  } else if (value->header.domain == GRN_DB_UINT64) {
    value_raw = (uint32_t)GRN_UINT64_VALUE(value);
  } else if (value->header.domain == GRN_DB_INT64) {
    value_raw = (uint32_t)GRN_INT64_VALUE(value);
  } else {
    grn_obj buffer;
    grn_rc rc;

    GRN_UINT32_INIT(&buffer, 0);
    rc = grn_obj_cast(ctx, value, &buffer, false);
    if (rc == GRN_SUCCESS) {
      value_raw = GRN_UINT32_VALUE(&buffer);
    }
    GRN_OBJ_FIN(ctx, &buffer);

    if (rc != GRN_SUCCESS) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      GRN_PLUGIN_ERROR(ctx,
                       rc,
                       "%s "
                       "failed to cast value to number: <%.*s>",
                       tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      value_raw = default_value_raw;
    }
  }

  return value_raw;
}

int64_t
grn_proc_get_value_int64(grn_ctx *ctx,
                         grn_obj *value,
                         int64_t default_value_raw,
                         const char *tag)
{
  int64_t value_raw;

  if (!value) {
    return default_value_raw;
  }

  if (!grn_type_id_is_number_family(ctx, value->header.domain)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s value must be a number: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_value_raw;
  }

  if (value->header.domain == GRN_DB_INT32) {
    value_raw = GRN_INT32_VALUE(value);
  } else if (value->header.domain == GRN_DB_INT64) {
    value_raw = GRN_INT64_VALUE(value);
  } else {
    grn_obj buffer;
    grn_rc rc;

    GRN_INT64_INIT(&buffer, 0);
    rc = grn_obj_cast(ctx, value, &buffer, false);
    if (rc == GRN_SUCCESS) {
      value_raw = GRN_INT64_VALUE(&buffer);
    }
    GRN_OBJ_FIN(ctx, &buffer);

    if (rc != GRN_SUCCESS) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      GRN_PLUGIN_ERROR(ctx,
                       rc,
                       "%s "
                       "failed to cast value to number: <%.*s>",
                       tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      value_raw = default_value_raw;
    }
  }

  return value_raw;
}

grn_operator
grn_proc_get_value_mode(grn_ctx *ctx,
                        grn_obj *value,
                        grn_operator default_mode,
                        const char *tag)
{
  if (!value) {
    return default_mode;
  }

  if (value->header.domain != GRN_DB_TEXT) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s mode must be text: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_mode;
  }

  if (GRN_TEXT_LEN(value) == 0) {
    return default_mode;
  }

#define EQUAL_MODE(name)                                                       \
  (GRN_TEXT_LEN(value) == strlen(name) &&                                      \
   memcmp(GRN_TEXT_VALUE(value), name, strlen(name)) == 0)

  if (EQUAL_MODE("==") || EQUAL_MODE("EQUAL")) {
    return GRN_OP_EQUAL;
  } else if (EQUAL_MODE("!=") || EQUAL_MODE("NOT_EQUAL")) {
    return GRN_OP_NOT_EQUAL;
  } else if (EQUAL_MODE("<") || EQUAL_MODE("LESS")) {
    return GRN_OP_LESS;
  } else if (EQUAL_MODE(">") || EQUAL_MODE("GREATER")) {
    return GRN_OP_GREATER;
  } else if (EQUAL_MODE("<=") || EQUAL_MODE("LESS_EQUAL")) {
    return GRN_OP_LESS_EQUAL;
  } else if (EQUAL_MODE(">=") || EQUAL_MODE("GREATER_EQUAL")) {
    return GRN_OP_GREATER_EQUAL;
  } else if (EQUAL_MODE("@") || EQUAL_MODE("MATCH")) {
    return GRN_OP_MATCH;
  } else if (EQUAL_MODE("*N") || EQUAL_MODE("NEAR")) {
    return GRN_OP_NEAR;
  } else if (EQUAL_MODE("*NP") || EQUAL_MODE("NEAR_PHRASE")) {
    return GRN_OP_NEAR_PHRASE;
  } else if (EQUAL_MODE("*ONP") || EQUAL_MODE("ORDERED_NEAR_PHRASE")) {
    return GRN_OP_ORDERED_NEAR_PHRASE;
  } else if (EQUAL_MODE("*NPP") || EQUAL_MODE("NEAR_PHRASE_PRODUCT")) {
    return GRN_OP_NEAR_PHRASE_PRODUCT;
  } else if (EQUAL_MODE("*ONPP") || EQUAL_MODE("ORDERED_NEAR_PHRASE_PRODUCT")) {
    return GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT;
  } else if (EQUAL_MODE("*S") || EQUAL_MODE("SIMILAR")) {
    return GRN_OP_SIMILAR;
  } else if (EQUAL_MODE("^") || EQUAL_MODE("@^") || EQUAL_MODE("PREFIX")) {
    return GRN_OP_PREFIX;
  } else if (EQUAL_MODE("$") || EQUAL_MODE("@$") || EQUAL_MODE("SUFFIX")) {
    return GRN_OP_SUFFIX;
  } else if (EQUAL_MODE("~") || EQUAL_MODE("@~") || EQUAL_MODE("REGEXP")) {
    return GRN_OP_REGEXP;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s mode must be one of them: "
                     "["
                     "\"==\", \"EQUAL\", "
                     "\"!=\", \"NOT_EQUAL\", "
                     "\"<\", \"LESS\", "
                     "\">\", \"GREATER\", "
                     "\"<=\", \"LESS_EQUAL\", "
                     "\">=\", \"GREATER_EQUAL\", "
                     "\"@\", \"MATCH\", "
                     "\"*N\", \"NEAR\", "
                     "\"*NP\", \"NEAR_PHRASE\", "
                     "\"*ONP\", \"ORDERED_NEAR_PHRASE\", "
                     "\"*NPP\", \"NEAR_PHRASE_PRODUCT\", "
                     "\"*ONPP\", \"ORDERED_NEAR_PHRASE_PRODUCT\", "
                     "\"*S\", \"SIMILAR\", "
                     "\"^\", \"@^\", \"PREFIX\", "
                     "\"$\", \"@$\", \"SUFFIX\", "
                     "\"~\", \"@~\", \"REGEXP\""
                     "]: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(value),
                     GRN_TEXT_VALUE(value));
    return default_mode;
  }

#undef EQUAL_MODE
}

grn_operator
grn_proc_get_value_operator(grn_ctx *ctx,
                            grn_obj *value,
                            grn_operator default_operator,
                            const char *tag)
{
  if (!value) {
    return default_operator;
  }

  if (value->header.domain != GRN_DB_TEXT) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s operator must be text: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_operator;
  }

  if (GRN_TEXT_LEN(value) == 0) {
    return default_operator;
  }

  grn_raw_string operator_string;
  GRN_RAW_STRING_SET(operator_string, value);

  if (GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "&&") ||
      GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "+") ||
      GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "AND")) {
    return GRN_OP_AND;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "||") ||
             GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "OR")) {
    return GRN_OP_OR;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "!") ||
             GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "NOT")) {
    return GRN_OP_NOT;
  } else if (GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "&!") ||
             GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "-") ||
             GRN_RAW_STRING_EQUAL_CSTRING(operator_string, "AND_NOT")) {
    return GRN_OP_AND_NOT;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s operator must be one of them: "
                     "["
                     "\"&&\", \"+\", \"AND\", "
                     "\"||\", \"OR\", "
                     "\"!\" \"NOT\", "
                     "\"&!\", \"-\", \"AND_NOT\""
                     "]: <%.*s>",
                     tag,
                     (int)operator_string.length,
                     operator_string.value);
    return default_operator;
  }
}

grn_tokenize_mode
grn_proc_get_value_tokenize_mode(grn_ctx *ctx,
                                 grn_obj *value,
                                 grn_tokenize_mode default_mode,
                                 const char *tag)
{
  if (!value) {
    return default_mode;
  }

  if (!grn_obj_is_text_family_bulk(ctx, value)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s tokenize mode must be text bulk: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_mode;
  }

  if (GRN_TEXT_LEN(value) == 0) {
    return default_mode;
  }

#define EQUAL_MODE(name)                                                       \
  (GRN_TEXT_LEN(value) == strlen(name) &&                                      \
   memcmp(GRN_TEXT_VALUE(value), name, strlen(name)) == 0)

  if (EQUAL_MODE("GET")) {
    return GRN_TOKENIZE_GET;
  } else if (EQUAL_MODE("ADD")) {
    return GRN_TOKENIZE_ADD;
  } else if (EQUAL_MODE("ONLY")) {
    return GRN_TOKENIZE_ONLY;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s tokenize mode must be one of them: "
                     "["
                     "\"GET\", "
                     "\"ADD\", "
                     "\"ONLY\""
                     "]: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(value),
                     GRN_TEXT_VALUE(value));
    return default_mode;
  }

#undef EQUAL_MODE
}

uint32_t
grn_proc_get_value_token_cursor_flags(grn_ctx *ctx,
                                      grn_obj *value,
                                      uint32_t default_flags,
                                      const char *tag)
{
  if (!value) {
    return default_flags;
  }

  if (!grn_obj_is_text_family_bulk(ctx, value)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s token cursor flags must be text bulk: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_flags;
  }

  if (GRN_TEXT_LEN(value) == 0) {
    return default_flags;
  }

  uint32_t flags = 0;
  const char *names, *names_end;
  names = GRN_TEXT_VALUE(value);
  names_end = names + GRN_TEXT_LEN(value);
  while (names < names_end) {
    if (*names == '|' || *names == ' ') {
      names += 1;
      continue;
    }

#define CHECK_FLAG(name)                                                       \
  if (((size_t)(names_end - names) >= (sizeof(#name) - 1)) &&                  \
      (!memcmp(names, #name, sizeof(#name) - 1))) {                            \
    flags |= GRN_TOKEN_CURSOR_##name;                                          \
    names += sizeof(#name) - 1;                                                \
    continue;                                                                  \
  }

    CHECK_FLAG(ENABLE_TOKENIZED_DELIMITER);

#define GRN_TOKEN_CURSOR_NONE 0
    CHECK_FLAG(NONE);
#undef GRN_TOKEN_CURSOR_NONE

    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s token cursor flag must be one of them: "
                     "["
                     "\"ENABLE_TOKENIZED_DELIMITER\", "
                     "\"NONE\""
                     "]: <%.*s>",
                     tag,
                     (int)(names_end - names),
                     names);
    return default_flags;
#undef CHECK_FLAG
  }

  return flags;
}

double
grn_proc_get_value_double(grn_ctx *ctx,
                          grn_obj *value,
                          double default_value_raw,
                          const char *tag)
{
  double value_raw;

  if (!value) {
    return default_value_raw;
  }

  if (!grn_type_id_is_number_family(ctx, value->header.domain)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, value);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s value must be a number: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return default_value_raw;
  }

  if (value->header.domain == GRN_DB_FLOAT) {
    value_raw = GRN_FLOAT_VALUE(value);
  } else if (value->header.domain == GRN_DB_FLOAT32) {
    value_raw = GRN_FLOAT32_VALUE(value);
#ifdef GRN_HAVE_BFLOAT16
  } else if (value->header.domain == GRN_DB_BFLOAT16) {
    value_raw = grn_bfloat16_to_float32(GRN_BFLOAT16_VALUE(value));
#endif
  } else {
    grn_obj buffer;
    grn_rc rc;

    GRN_FLOAT_INIT(&buffer, 0);
    rc = grn_obj_cast(ctx, value, &buffer, false);
    if (rc == GRN_SUCCESS) {
      value_raw = GRN_FLOAT_VALUE(&buffer);
    }
    GRN_OBJ_FIN(ctx, &buffer);

    if (rc != GRN_SUCCESS) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      GRN_PLUGIN_ERROR(ctx,
                       rc,
                       "%s "
                       "failed to cast value to number: <%.*s>",
                       tag,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      value_raw = default_value_raw;
    }
  }

  return value_raw;
}

grn_obj *
grn_proc_get_value_object(grn_ctx *ctx, grn_obj *value, const char *tag)
{
  if (!value) {
    return NULL;
  }

  if (!grn_obj_is_text_family_bulk(ctx, value)) {
    return value;
  }

  if (GRN_TEXT_LEN(value) == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s object name isn't specified",
                     tag);
    return NULL;
  }

  grn_obj *object =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
  if (!object) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s object doesn't exist: <%.*s>",
                     tag,
                     (int)GRN_TEXT_LEN(value),
                     GRN_TEXT_VALUE(value));
    return NULL;
  }

  return object;
}

grn_obj *
grn_proc_get_value_column(grn_ctx *ctx,
                          grn_obj *value,
                          grn_obj *table,
                          const char *tag)
{
  if (!value) {
    return NULL;
  }

  if (!grn_obj_is_text_family_bulk(ctx, value)) {
    return value;
  }

  if (GRN_TEXT_LEN(value) == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s column name isn't specified",
                     tag);
    return NULL;
  }

  grn_obj *column =
    grn_obj_column(ctx, table, GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
  if (!column) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, table);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s column doesn't exist: <%.*s>: %.*s",
                     tag,
                     (int)GRN_TEXT_LEN(value),
                     GRN_TEXT_VALUE(value),
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  return column;
}

grn_raw_string
grn_proc_get_value_raw_string(grn_ctx *ctx,
                              grn_obj *value,
                              grn_raw_string *default_value,
                              const char *tag)
{
  if (!value) {
    return (grn_raw_string){NULL, 0};
  }

  if (!grn_obj_is_text_family_bulk(ctx, value)) {
    return *default_value;
  }

  return (grn_raw_string){GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value)};
}

static grn_obj *
proc_cache_limit(grn_ctx *ctx,
                 int nargs,
                 grn_obj **args,
                 grn_user_data *user_data)
{
  grn_cache *cache;
  unsigned int current_max_n_entries;

  cache = grn_cache_current_get(ctx);
  current_max_n_entries = grn_cache_get_max_n_entries(ctx, cache);
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *rest;
    uint32_t max =
      grn_atoui(GRN_TEXT_VALUE(VAR(0)), GRN_BULK_CURR(VAR(0)), &rest);
    if (GRN_BULK_CURR(VAR(0)) == rest) {
      grn_cache_set_max_n_entries(ctx, cache, max);
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "max value is invalid unsigned integer format: <%.*s>",
          (int)GRN_TEXT_LEN(VAR(0)),
          GRN_TEXT_VALUE(VAR(0)));
    }
  }
  if (ctx->rc == GRN_SUCCESS) {
    GRN_OUTPUT_INT64(current_max_n_entries);
  }
  return NULL;
}

static grn_obj *
proc_register(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *name;
    GRN_TEXT_PUTC(ctx, VAR(0), '\0');
    name = GRN_TEXT_VALUE(VAR(0));
    grn_plugin_register(ctx, name);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "path is required");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

void
grn_ii_buffer_check(grn_ctx *ctx, grn_ii *ii, uint32_t seg);

static grn_obj *
proc_check(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT,
        "no such object: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)),
        GRN_TEXT_VALUE(VAR(0)));
    GRN_OUTPUT_BOOL(!ctx->rc);
  } else {
    switch (obj->header.type) {
    case GRN_DB:
      GRN_OUTPUT_BOOL(!ctx->rc);
      break;
    case GRN_TABLE_PAT_KEY:
      grn_pat_check(ctx, (grn_pat *)obj);
      break;
    case GRN_TABLE_HASH_KEY:
      grn_hash_check(ctx, (grn_hash *)obj);
      break;
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
    case GRN_COLUMN_FIX_SIZE:
      GRN_OUTPUT_BOOL(!ctx->rc);
      break;
    case GRN_COLUMN_VAR_SIZE:
      grn_ja_check(ctx, (grn_ja *)obj);
      break;
    case GRN_COLUMN_INDEX:
      {
        grn_ii *ii = (grn_ii *)obj;
        grn_ii_header_common *h = ii->header.common;
        char buf[8];
        GRN_OUTPUT_ARRAY_OPEN("RESULT", 8);
        {
          uint32_t i, j, g = 0, a = 0, b = 0;
          uint32_t max = 0;
          for (i = h->bgqtail; i != h->bgqhead;
               i = ((i + 1) & (GRN_II_BGQSIZE - 1))) {
            j = h->bgqbody[i];
            g++;
            if (j > max) {
              max = j;
            }
          }
          for (i = 0; i < GRN_II_MAX_LSEG; i++) {
            j = h->binfo[i];
            if (j != GRN_II_PSEG_NOT_ASSIGNED) {
              if (j > max) {
                max = j;
              }
              b++;
            }
          }
          for (i = 0; i < GRN_II_MAX_LSEG; i++) {
            j = h->ainfo[i];
            if (j != GRN_II_PSEG_NOT_ASSIGNED) {
              if (j > max) {
                max = j;
              }
              a++;
            }
          }
          GRN_OUTPUT_MAP_OPEN("SUMMARY", 12);
          GRN_OUTPUT_CSTR("flags");
          grn_itoh(h->flags, buf, 8);
          GRN_OUTPUT_STR(buf, 8);
          GRN_OUTPUT_CSTR("max sid");
          GRN_OUTPUT_INT64(h->smax);
          GRN_OUTPUT_CSTR("number of garbage segments");
          GRN_OUTPUT_INT64(g);
          GRN_OUTPUT_CSTR("number of array segments");
          GRN_OUTPUT_INT64(a);
          GRN_OUTPUT_CSTR("max id of array segment");
          GRN_OUTPUT_INT64(h->amax);
          GRN_OUTPUT_CSTR("number of buffer segments");
          GRN_OUTPUT_INT64(b);
          GRN_OUTPUT_CSTR("max id of buffer segment");
          GRN_OUTPUT_INT64(h->bmax);
          GRN_OUTPUT_CSTR("max id of physical segment in use");
          GRN_OUTPUT_INT64(max);
          GRN_OUTPUT_CSTR("number of unmanaged segments");
          GRN_OUTPUT_INT64(h->pnext - a - b - g);
          GRN_OUTPUT_CSTR("total chunk size");
          GRN_OUTPUT_INT64(h->total_chunk_size);
          for (max = 0, i = 0; i < (GRN_II_MAX_CHUNK >> 3); i++) {
            if ((j = h->chunks[i])) {
              int k;
              for (k = 0; k < 8; k++) {
                if ((j & (1 << k))) {
                  max = (i << 3) + j;
                }
              }
            }
          }
          GRN_OUTPUT_CSTR("max id of chunk segments in use");
          GRN_OUTPUT_INT64(max);
          GRN_OUTPUT_CSTR("number of garbage chunk");
          GRN_OUTPUT_ARRAY_OPEN("NGARBAGES", GRN_II_N_CHUNK_VARIATION);
          for (i = 0; i <= GRN_II_N_CHUNK_VARIATION; i++) {
            GRN_OUTPUT_INT64(h->ngarbages[i]);
          }
          GRN_OUTPUT_ARRAY_CLOSE();
          GRN_OUTPUT_MAP_CLOSE();
          const uint32_t n_logical_segments = grn_ii_n_logical_segments(ii);
          for (uint32_t lseg = 0; lseg < n_logical_segments; lseg++) {
            const uint32_t pseg = grn_ii_get_buffer_pseg(ii, lseg);
            if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
              grn_ii_buffer_check(ctx, ii, lseg);
            }
          }
        }
        GRN_OUTPUT_ARRAY_CLOSE();
      }
      break;
    }
  }
  return NULL;
}

static grn_obj *
proc_truncate(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  const char *target_name;
  int target_name_len;

  target_name_len = GRN_TEXT_LEN(VAR(0));
  if (target_name_len > 0) {
    target_name = GRN_TEXT_VALUE(VAR(0));
  } else {
    target_name_len = GRN_TEXT_LEN(VAR(1));
    if (target_name_len == 0) {
      ERR(GRN_INVALID_ARGUMENT, "[truncate] table name is missing");
      goto exit;
    }
    target_name = GRN_TEXT_VALUE(VAR(1));
  }

  {
    grn_obj *target = grn_ctx_get(ctx, target_name, target_name_len);
    if (!target) {
      ERR(GRN_INVALID_ARGUMENT,
          "[truncate] no such target: <%.*s>",
          target_name_len,
          target_name);
      goto exit;
    }

    switch (target->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
      grn_table_truncate(ctx, target);
      break;
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE:
    case GRN_COLUMN_INDEX:
      grn_column_truncate(ctx, target);
      break;
    default:
      {
        grn_obj buffer;
        GRN_TEXT_INIT(&buffer, 0);
        grn_inspect(ctx, &buffer, target);
        ERR(GRN_INVALID_ARGUMENT,
            "[truncate] not a table nor column object: <%.*s>",
            (int)GRN_TEXT_LEN(&buffer),
            GRN_TEXT_VALUE(&buffer));
        GRN_OBJ_FIN(ctx, &buffer);
      }
      break;
    }
  }

exit:
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static void
list_proc(grn_ctx *ctx,
          grn_proc_type target_proc_type,
          const char *name,
          const char *plural_name)
{
  grn_obj *db;
  grn_table_cursor *cursor;
  grn_obj target_procs;

  db = grn_ctx_db(ctx);
  cursor =
    grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
  if (!cursor) {
    return;
  }

  GRN_PTR_INIT(&target_procs, GRN_OBJ_VECTOR, GRN_ID_NIL);
  {
    grn_id id;

    while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
      grn_obj *obj;
      grn_proc_type proc_type;

      obj = grn_ctx_at(ctx, id);
      if (!obj) {
        continue;
      }

      if (obj->header.type != GRN_PROC) {
        grn_obj_unlink(ctx, obj);
        continue;
      }

      proc_type = grn_proc_get_type(ctx, obj);
      if (proc_type != target_proc_type) {
        grn_obj_unlink(ctx, obj);
        continue;
      }

      GRN_PTR_PUT(ctx, &target_procs, obj);
    }
    grn_table_cursor_close(ctx, cursor);

    {
      int i, n_procs;

      n_procs = GRN_BULK_VSIZE(&target_procs) / sizeof(grn_obj *);
      GRN_OUTPUT_ARRAY_OPEN(plural_name, n_procs);
      for (i = 0; i < n_procs; i++) {
        grn_obj *proc;
        char name[GRN_TABLE_MAX_KEY_SIZE];
        int name_size;

        proc = GRN_PTR_VALUE_AT(&target_procs, i);
        name_size = grn_obj_name(ctx, proc, name, GRN_TABLE_MAX_KEY_SIZE);
        GRN_OUTPUT_MAP_OPEN(name, 1);
        GRN_OUTPUT_CSTR("name");
        GRN_OUTPUT_STR(name, name_size);
        GRN_OUTPUT_MAP_CLOSE();

        grn_obj_unlink(ctx, proc);
      }
      GRN_OUTPUT_ARRAY_CLOSE();
    }

    grn_obj_unlink(ctx, &target_procs);
  }
}

static grn_obj *
proc_tokenizer_list(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  list_proc(ctx, GRN_PROC_TOKENIZER, "tokenizer", "tokenizers");
  return NULL;
}

static grn_obj *
proc_normalizer_list(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  list_proc(ctx, GRN_PROC_NORMALIZER, "normalizer", "normalizers");
  return NULL;
}

static grn_obj *
func_rand(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int val;
  grn_obj *obj;
  if (nargs > 0) {
    int max = GRN_INT32_VALUE(args[0]);
    val = (int)(1.0 * max * rand() / (RAND_MAX + 1.0));
  } else {
    val = rand();
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_INT32, 0))) {
    GRN_INT32_SET(ctx, obj, val);
  }
  return obj;
}

static grn_obj *
func_now(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  if ((obj = GRN_PROC_ALLOC(GRN_DB_TIME, 0))) {
    GRN_TIME_NOW(ctx, obj);
  }
  return obj;
}

static inline bool
is_comparable_number_type(grn_id type)
{
  return GRN_DB_INT8 <= type && type <= GRN_DB_TIME;
}

static inline grn_id
larger_number_type(grn_id type1, grn_id type2)
{
  if (type1 == type2) {
    return type1;
  }

  switch (type1) {
  case GRN_DB_BFLOAT16:
    if (type2 == GRN_DB_FLOAT32 || type2 == GRN_DB_FLOAT ||
        type2 == GRN_DB_TIME) {
      return type2;
    } else {
      return type1;
    }
  case GRN_DB_FLOAT32:
    if (type2 == GRN_DB_FLOAT || type2 == GRN_DB_TIME) {
      return type2;
    } else {
      return type1;
    }
  case GRN_DB_FLOAT:
    return type1;
  case GRN_DB_TIME:
    if (type2 == GRN_DB_FLOAT) {
      return type2;
    } else {
      return type1;
    }
  default:
    if (type2 > type1) {
      return type2;
    } else {
      return type1;
    }
  }
}

static inline grn_id
smaller_number_type(grn_id type1, grn_id type2)
{
  if (type1 == type2) {
    return type1;
  }

  switch (type1) {
  case GRN_DB_BFLOAT16:
    if (type2 == GRN_DB_FLOAT32 || type2 == GRN_DB_FLOAT ||
        type2 == GRN_DB_TIME) {
      return type1;
    } else {
      return type2;
    }
  case GRN_DB_FLOAT32:
    if (type2 == GRN_DB_FLOAT || type2 == GRN_DB_TIME) {
      return type1;
    } else {
      return type2;
    }
  case GRN_DB_FLOAT:
    return type1;
  case GRN_DB_TIME:
    if (type2 == GRN_DB_FLOAT) {
      return type1;
    } else {
      return type2;
    }
  default:
    {
      grn_id smaller_number_type;
      if (type2 > type1) {
        smaller_number_type = type2;
      } else {
        smaller_number_type = type1;
      }
      switch (smaller_number_type) {
      case GRN_DB_UINT8:
        return GRN_DB_INT8;
      case GRN_DB_UINT16:
        return GRN_DB_INT16;
      case GRN_DB_UINT32:
        return GRN_DB_INT32;
      case GRN_DB_UINT64:
        return GRN_DB_INT64;
      default:
        return smaller_number_type;
      }
    }
  }
}

static inline bool
is_negative_value(grn_obj *number)
{
  switch (number->header.domain) {
  case GRN_DB_INT8:
    return GRN_INT8_VALUE(number) < 0;
  case GRN_DB_INT16:
    return GRN_INT16_VALUE(number) < 0;
  case GRN_DB_INT32:
    return GRN_INT32_VALUE(number) < 0;
  case GRN_DB_INT64:
    return GRN_INT64_VALUE(number) < 0;
  case GRN_DB_TIME:
    return GRN_TIME_VALUE(number) < 0;
#ifdef GRN_HAVE_BFLOAT16
  case GRN_DB_BFLOAT16:
    return GRN_BFLOAT16_VALUE(number) < 0;
#endif
  case GRN_DB_FLOAT32:
    return GRN_FLOAT32_VALUE(number) < 0;
  case GRN_DB_FLOAT:
    return GRN_FLOAT_VALUE(number) < 0;
  default:
    return false;
  }
}

static inline bool
number_safe_cast(grn_ctx *ctx, grn_obj *src, grn_obj *dest, grn_id type)
{
  grn_obj_reinit(ctx, dest, type, 0);
  if (src->header.domain == type) {
    GRN_TEXT_SET(ctx, dest, GRN_TEXT_VALUE(src), GRN_TEXT_LEN(src));
    return true;
  }

  switch (type) {
  case GRN_DB_UINT8:
    if (is_negative_value(src)) {
      GRN_UINT8_SET(ctx, dest, 0);
      return true;
    }
    break;
  case GRN_DB_UINT16:
    if (is_negative_value(src)) {
      GRN_UINT16_SET(ctx, dest, 0);
      return true;
    }
    break;
  case GRN_DB_UINT32:
    if (is_negative_value(src)) {
      GRN_UINT32_SET(ctx, dest, 0);
      return true;
    }
    break;
  case GRN_DB_UINT64:
    if (is_negative_value(src)) {
      GRN_UINT64_SET(ctx, dest, 0);
      return true;
    }
    break;
  }
  return grn_obj_cast(ctx, src, dest, false) == GRN_SUCCESS;
}

static inline int
compare_number(grn_ctx *ctx, grn_obj *number1, grn_obj *number2, grn_id type)
{
#define COMPARE_AND_RETURN(type, value1, value2)                               \
  {                                                                            \
    type computed_value1 = value1;                                             \
    type computed_value2 = value2;                                             \
    if (computed_value1 > computed_value2) {                                   \
      return 1;                                                                \
    } else if (computed_value1 < computed_value2) {                            \
      return -1;                                                               \
    } else {                                                                   \
      return 0;                                                                \
    }                                                                          \
  }

  switch (type) {
  case GRN_DB_INT8:
    COMPARE_AND_RETURN(int8_t,
                       GRN_INT8_VALUE(number1),
                       GRN_INT8_VALUE(number2));
  case GRN_DB_UINT8:
    COMPARE_AND_RETURN(uint8_t,
                       GRN_UINT8_VALUE(number1),
                       GRN_UINT8_VALUE(number2));
  case GRN_DB_INT16:
    COMPARE_AND_RETURN(int16_t,
                       GRN_INT16_VALUE(number1),
                       GRN_INT16_VALUE(number2));
  case GRN_DB_UINT16:
    COMPARE_AND_RETURN(uint16_t,
                       GRN_UINT16_VALUE(number1),
                       GRN_UINT16_VALUE(number2));
  case GRN_DB_INT32:
    COMPARE_AND_RETURN(int32_t,
                       GRN_INT32_VALUE(number1),
                       GRN_INT32_VALUE(number2));
  case GRN_DB_UINT32:
    COMPARE_AND_RETURN(uint32_t,
                       GRN_UINT32_VALUE(number1),
                       GRN_UINT32_VALUE(number2));
  case GRN_DB_INT64:
    COMPARE_AND_RETURN(int64_t,
                       GRN_INT64_VALUE(number1),
                       GRN_INT64_VALUE(number2));
  case GRN_DB_UINT64:
    COMPARE_AND_RETURN(uint64_t,
                       GRN_UINT64_VALUE(number1),
                       GRN_UINT64_VALUE(number2));
#ifdef GRN_HAVE_BFLOAT16
  case GRN_DB_BFLOAT16:
    COMPARE_AND_RETURN(grn_bfloat16,
                       GRN_BFLOAT16_VALUE(number1),
                       GRN_BFLOAT16_VALUE(number2));
#endif
  case GRN_DB_FLOAT32:
    COMPARE_AND_RETURN(float,
                       GRN_FLOAT32_VALUE(number1),
                       GRN_FLOAT32_VALUE(number2));
  case GRN_DB_FLOAT:
    COMPARE_AND_RETURN(double,
                       GRN_FLOAT_VALUE(number1),
                       GRN_FLOAT_VALUE(number2));
  case GRN_DB_TIME:
    COMPARE_AND_RETURN(int64_t,
                       GRN_TIME_VALUE(number1),
                       GRN_TIME_VALUE(number2));
  default:
    return 0;
  }

#undef COMPARE_AND_RETURN
}

static inline void
get_number_in_grn_uvector(grn_ctx *ctx,
                          grn_obj *uvector,
                          unsigned int offset,
                          grn_obj *buf)
{
#define GET_UVECTOR_ELEMENT_AS(type)                                           \
  do {                                                                         \
    GRN_##type##_SET(ctx, buf, GRN_##type##_VALUE_AT(uvector, offset));        \
  } while (false)
  switch (uvector->header.domain) {
  case GRN_DB_BOOL:
    GET_UVECTOR_ELEMENT_AS(BOOL);
    break;
  case GRN_DB_INT8:
    GET_UVECTOR_ELEMENT_AS(INT8);
    break;
  case GRN_DB_UINT8:
    GET_UVECTOR_ELEMENT_AS(UINT8);
    break;
  case GRN_DB_INT16:
    GET_UVECTOR_ELEMENT_AS(INT16);
    break;
  case GRN_DB_UINT16:
    GET_UVECTOR_ELEMENT_AS(UINT16);
    break;
  case GRN_DB_INT32:
    GET_UVECTOR_ELEMENT_AS(INT32);
    break;
  case GRN_DB_UINT32:
    GET_UVECTOR_ELEMENT_AS(UINT32);
    break;
  case GRN_DB_INT64:
    GET_UVECTOR_ELEMENT_AS(INT64);
    break;
  case GRN_DB_UINT64:
    GET_UVECTOR_ELEMENT_AS(UINT64);
    break;
#ifdef GRN_HAVE_BFLOAT16
  case GRN_DB_BFLOAT16:
    GET_UVECTOR_ELEMENT_AS(BFLOAT16);
    break;
#endif
  case GRN_DB_FLOAT32:
    GET_UVECTOR_ELEMENT_AS(FLOAT32);
    break;
  case GRN_DB_FLOAT:
    GET_UVECTOR_ELEMENT_AS(FLOAT);
    break;
  case GRN_DB_TIME:
    GET_UVECTOR_ELEMENT_AS(TIME);
    break;
  default:
    GET_UVECTOR_ELEMENT_AS(RECORD);
    break;
  }
#undef GET_UVECTOR_ELEMENT_AS
}

static inline void
apply_max(grn_ctx *ctx,
          grn_obj *number,
          grn_obj *max,
          grn_obj *casted_number,
          grn_obj *casted_max,
          grn_id cast_type)
{
  grn_id domain = number->header.domain;
  if (!is_comparable_number_type(domain)) {
    return;
  }
  cast_type = larger_number_type(cast_type, domain);
  if (!number_safe_cast(ctx, number, casted_number, cast_type)) {
    return;
  }
  if (max->header.domain == GRN_DB_VOID) {
    grn_obj_reinit(ctx, max, cast_type, 0);
    GRN_TEXT_SET(ctx,
                 max,
                 GRN_TEXT_VALUE(casted_number),
                 GRN_TEXT_LEN(casted_number));
    return;
  }

  if (max->header.domain != cast_type) {
    if (!number_safe_cast(ctx, max, casted_max, cast_type)) {
      return;
    }
    grn_obj_reinit(ctx, max, cast_type, 0);
    GRN_TEXT_SET(ctx,
                 max,
                 GRN_TEXT_VALUE(casted_max),
                 GRN_TEXT_LEN(casted_max));
  }
  if (compare_number(ctx, casted_number, max, cast_type) > 0) {
    grn_obj_reinit(ctx, max, cast_type, 0);
    GRN_TEXT_SET(ctx,
                 max,
                 GRN_TEXT_VALUE(casted_number),
                 GRN_TEXT_LEN(casted_number));
  }
}

static grn_obj *
func_max(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *max;
  grn_id cast_type = GRN_DB_INT8;
  grn_obj casted_max, casted_number;
  int i;

  max = GRN_PROC_ALLOC(GRN_DB_VOID, 0);
  if (!max) {
    return max;
  }

  GRN_VOID_INIT(&casted_max);
  GRN_VOID_INIT(&casted_number);

  for (i = 0; i < nargs; i++) {
    switch (args[i]->header.type) {
    case GRN_BULK:
      apply_max(ctx, args[i], max, &casted_number, &casted_max, cast_type);
      break;
    case GRN_UVECTOR:
      {
        unsigned int j;
        unsigned int n_elements;
        grn_obj number_in_uvector;
        grn_obj *domain;

        domain = grn_ctx_at(ctx, args[i]->header.domain);
        GRN_OBJ_INIT(&number_in_uvector, GRN_BULK, 0, args[i]->header.domain);
        n_elements = grn_uvector_size(ctx, args[i]);
        for (j = 0; j < n_elements; j++) {
          get_number_in_grn_uvector(ctx, args[i], j, &number_in_uvector);
          if (grn_obj_is_table(ctx, domain)) {
            grn_obj_reinit(ctx, &number_in_uvector, domain->header.domain, 0);
            grn_table_get_key2(ctx,
                               domain,
                               GRN_RECORD_VALUE(&number_in_uvector),
                               &number_in_uvector);
          }
          apply_max(ctx,
                    &number_in_uvector,
                    max,
                    &casted_number,
                    &casted_max,
                    cast_type);
        }
        GRN_OBJ_FIN(ctx, &number_in_uvector);
      }
      break;
    default:
      continue;
    }
  }
  GRN_OBJ_FIN(ctx, &casted_max);
  GRN_OBJ_FIN(ctx, &casted_number);

  return max;
}

static void
apply_min(grn_ctx *ctx,
          grn_obj *number,
          grn_obj *min,
          grn_obj *casted_number,
          grn_obj *casted_min,
          grn_id cast_type)
{
  grn_id domain = number->header.domain;
  if (!is_comparable_number_type(domain)) {
    return;
  }
  cast_type = smaller_number_type(cast_type, domain);
  if (!number_safe_cast(ctx, number, casted_number, cast_type)) {
    return;
  }
  if (min->header.domain == GRN_DB_VOID) {
    grn_obj_reinit(ctx, min, cast_type, 0);
    GRN_TEXT_SET(ctx,
                 min,
                 GRN_TEXT_VALUE(casted_number),
                 GRN_TEXT_LEN(casted_number));
    return;
  }

  if (min->header.domain != cast_type) {
    if (!number_safe_cast(ctx, min, casted_min, cast_type)) {
      return;
    }
    grn_obj_reinit(ctx, min, cast_type, 0);
    GRN_TEXT_SET(ctx,
                 min,
                 GRN_TEXT_VALUE(casted_min),
                 GRN_TEXT_LEN(casted_min));
  }
  if (compare_number(ctx, casted_number, min, cast_type) < 0) {
    grn_obj_reinit(ctx, min, cast_type, 0);
    GRN_TEXT_SET(ctx,
                 min,
                 GRN_TEXT_VALUE(casted_number),
                 GRN_TEXT_LEN(casted_number));
  }
}

static grn_obj *
func_min(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *min;
  grn_id cast_type = GRN_DB_INT8;
  grn_obj casted_min, casted_number;
  int i;

  min = GRN_PROC_ALLOC(GRN_DB_VOID, 0);
  if (!min) {
    return min;
  }

  GRN_VOID_INIT(&casted_min);
  GRN_VOID_INIT(&casted_number);
  for (i = 0; i < nargs; i++) {
    switch (args[i]->header.type) {
    case GRN_BULK:
      apply_min(ctx, args[i], min, &casted_number, &casted_min, cast_type);
      break;
    case GRN_UVECTOR:
      {
        unsigned int j;
        unsigned int n_elements;
        grn_obj number_in_uvector;
        grn_obj *domain;

        domain = grn_ctx_at(ctx, args[i]->header.domain);
        GRN_OBJ_INIT(&number_in_uvector, GRN_BULK, 0, args[i]->header.domain);
        n_elements = grn_uvector_size(ctx, args[i]);
        for (j = 0; j < n_elements; j++) {
          get_number_in_grn_uvector(ctx, args[i], j, &number_in_uvector);
          if (grn_obj_is_table(ctx, domain)) {
            grn_obj_reinit(ctx, &number_in_uvector, domain->header.domain, 0);
            grn_table_get_key2(ctx,
                               domain,
                               GRN_RECORD_VALUE(&number_in_uvector),
                               &number_in_uvector);
          }
          apply_min(ctx,
                    &number_in_uvector,
                    min,
                    &casted_number,
                    &casted_min,
                    cast_type);
        }
        GRN_OBJ_FIN(ctx, &number_in_uvector);
      }
      break;
    default:
      continue;
    }
  }
  GRN_OBJ_FIN(ctx, &casted_min);
  GRN_OBJ_FIN(ctx, &casted_number);

  return min;
}

static grn_obj *
func_geo_in_circle(grn_ctx *ctx,
                   int nargs,
                   grn_obj **args,
                   grn_user_data *user_data)
{
  grn_obj *obj;
  bool r = false;
  grn_geo_approximate_type type = GRN_GEO_APPROXIMATE_RECTANGLE;
  switch (nargs) {
  case 4:
    if (grn_geo_resolve_approximate_type(ctx, args[3], &type) != GRN_SUCCESS) {
      break;
    }
    /* fallthru */
  case 3:
    r = grn_geo_in_circle(ctx, args[0], args[1], args[2], type);
    break;
  default:
    break;
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_BOOL, 0))) {
    GRN_BOOL_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_in_rectangle(grn_ctx *ctx,
                      int nargs,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  grn_obj *obj;
  bool r = false;
  if (nargs == 3) {
    r = grn_geo_in_rectangle(ctx, args[0], args[1], args[2]);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_BOOL, 0))) {
    GRN_BOOL_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_distance(grn_ctx *ctx,
                  int nargs,
                  grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0.0;
  grn_geo_approximate_type type = GRN_GEO_APPROXIMATE_RECTANGLE;
  switch (nargs) {
  case 3:
    if (grn_geo_resolve_approximate_type(ctx, args[2], &type) != GRN_SUCCESS) {
      break;
    }
    /* fallthru */
  case 2:
    d = grn_geo_distance(ctx, args[0], args[1], type);
    break;
  default:
    break;
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

/* deprecated. */
static grn_obj *
func_geo_distance2(grn_ctx *ctx,
                   int nargs,
                   grn_obj **args,
                   grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
  if (nargs == 2) {
    d = grn_geo_distance_sphere(ctx, args[0], args[1]);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

/* deprecated. */
static grn_obj *
func_geo_distance3(grn_ctx *ctx,
                   int nargs,
                   grn_obj **args,
                   grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
  if (nargs == 2) {
    d = grn_geo_distance_ellipsoid(ctx, args[0], args[1]);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_all_records(grn_ctx *ctx,
                 int nargs,
                 grn_obj **args,
                 grn_user_data *user_data)
{
  grn_obj *true_value;
  if ((true_value = GRN_PROC_ALLOC(GRN_DB_BOOL, 0))) {
    GRN_BOOL_SET(ctx, true_value, true);
  }
  return true_value;
}

static grn_rc
selector_all_records(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *index,
                     int nargs,
                     grn_obj **args,
                     grn_obj *res,
                     grn_operator op)
{
  const double score = 1.0;
  return grn_result_set_add_table(ctx, (grn_hash *)res, table, score, op);
}

bool
grn_proc_selector_to_function_data_init(
  grn_ctx *ctx,
  grn_proc_selector_to_function_data *data,
  grn_user_data *user_data)
{
  grn_obj *condition = NULL;
  grn_obj *variable;

  data->table = NULL;
  data->records = NULL;

  data->found = GRN_PROC_ALLOC(GRN_DB_BOOL, 0);
  if (!data->found) {
    return false;
  }
  GRN_BOOL_SET(ctx, data->found, false);

  grn_proc_get_info(ctx, user_data, NULL, NULL, &condition);
  if (!condition) {
    return false;
  }

  variable = grn_expr_get_var_by_offset(ctx, condition, 0);
  if (!variable) {
    return false;
  }

  data->table = grn_ctx_at(ctx, variable->header.domain);
  if (!data->table) {
    return false;
  }

  data->records = grn_table_create(ctx,
                                   NULL,
                                   0,
                                   NULL,
                                   GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                   data->table,
                                   NULL);
  if (!data->records) {
    return false;
  }

  {
    grn_rset_posinfo pi;
    unsigned int key_size;
    memset(&pi, 0, sizeof(grn_rset_posinfo));
    pi.rid = GRN_RECORD_VALUE(variable);
    key_size = ((grn_hash *)(data->records))->key_size;
    if (grn_table_add(ctx, data->records, &pi, key_size, NULL) == GRN_ID_NIL) {
      return false;
    }
  }

  return true;
}

void
grn_proc_selector_to_function_data_selected(
  grn_ctx *ctx, grn_proc_selector_to_function_data *data)
{
  GRN_BOOL_SET(ctx, data->found, grn_table_size(ctx, data->records) > 0);
}

void
grn_proc_selector_to_function_data_fin(grn_ctx *ctx,
                                       grn_proc_selector_to_function_data *data)
{
  if (data->records) {
    grn_obj_unlink(ctx, data->records);
  }
  if (data->table) {
    grn_obj_unref(ctx, data->table);
  }
}

static bool
sub_filter_pre_filter_accessor_is_available(grn_ctx *ctx,
                                            grn_accessor *accessor)
{
  if (grn_obj_is_scalar_column(ctx, accessor->obj) ||
      grn_obj_is_table(ctx, accessor->obj)) {
    if (accessor->next) {
      return sub_filter_pre_filter_accessor_is_available(ctx, accessor->next);
    } else {
      return true;
    }
  } else if (grn_obj_is_vector_column(ctx, accessor->obj)) {
    if (accessor->next) {
      return sub_filter_pre_filter_accessor_is_available(ctx, accessor->next);
    } else {
      return true;
    }
  } else if (grn_obj_is_index_column(ctx, accessor->obj)) {
    if (accessor->next) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

static void
sub_filter_pre_filter_accessor(grn_ctx *ctx,
                               grn_accessor *accessor,
                               grn_id id,
                               grn_obj *base_res)
{
  if (grn_obj_is_scalar_column(ctx, accessor->obj) ||
      grn_obj_is_table(ctx, accessor->obj)) {
    grn_posting_internal posting = {0};
    posting.weight_float = 1;
    if (grn_obj_is_scalar_column(ctx, accessor->obj)) {
      grn_obj value;
      GRN_RECORD_INIT(&value, 0, DB_OBJ(accessor->obj)->range);
      grn_obj_get_value(ctx, accessor->obj, id, &value);
      if (GRN_BULK_VSIZE(&value) > 0) {
        posting.rid = GRN_RECORD_VALUE(&value);
      }
      GRN_OBJ_FIN(ctx, &value);
    } else {
      grn_table_get_key(ctx, accessor->obj, id, &(posting.rid), sizeof(grn_id));
    }
    if (posting.rid != GRN_ID_NIL) {
      if (accessor->next) {
        sub_filter_pre_filter_accessor(ctx,
                                       accessor->next,
                                       posting.rid,
                                       base_res);
      } else {
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)base_res,
                                 GRN_OP_OR);
      }
    }
  } else if (grn_obj_is_vector_column(ctx, accessor->obj)) {
    grn_posting_internal posting = {0};
    grn_obj values;
    unsigned int i, n;

    posting.weight_float = 1;
    GRN_RECORD_INIT(&values, GRN_OBJ_VECTOR, DB_OBJ(accessor->obj)->range);
    grn_obj_get_value(ctx, accessor->obj, id, &values);
    n = grn_vector_size(ctx, &values);
    for (i = 0; i < n; i++) {
      posting.rid = grn_uvector_get_element(ctx, &values, i, &(posting.weight));
      if (accessor->next) {
        sub_filter_pre_filter_accessor(ctx,
                                       accessor->next,
                                       posting.rid,
                                       base_res);
      } else {
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)base_res,
                                 GRN_OP_OR);
      }
    }
    GRN_OBJ_FIN(ctx, &values);
  } else if (grn_obj_is_index_column(ctx, accessor->obj)) {
    grn_ii_at(ctx,
              (grn_ii *)(accessor->obj),
              id,
              (grn_hash *)base_res,
              GRN_OP_OR);
  }
}

static bool
sub_filter_need_pre_filter(grn_ctx *ctx,
                           grn_obj *res,
                           int32_t pre_filter_threshold)
{
  if (pre_filter_threshold < 0) {
    return true;
  }

  if ((int32_t)(grn_table_size(ctx, res)) <= pre_filter_threshold) {
    return true;
  }

  return false;
}

static bool
sub_filter_pre_filter(grn_ctx *ctx,
                      grn_obj *res,
                      grn_obj *scope,
                      grn_obj *base_res,
                      int32_t pre_filter_threshold)
{
  const char *action = "[sub-filter][pre-filter]";

  if (!sub_filter_need_pre_filter(ctx, res, pre_filter_threshold)) {
    return false;
  }

  if (grn_obj_is_scalar_column(ctx, scope)) {
    grn_posting_internal posting = {0};
    grn_obj value;

    posting.weight_float = 1;
    GRN_RECORD_INIT(&value, 0, grn_obj_get_range(ctx, scope));
    GRN_TABLE_EACH_BEGIN(ctx, res, cursor, id)
    {
      grn_id *matched_id;

      grn_table_cursor_get_key(ctx, cursor, (void **)&matched_id);
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, scope, *matched_id, &value);
      if (GRN_BULK_VSIZE(&value) > 0) {
        posting.rid = GRN_RECORD_VALUE(&value);
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)base_res,
                                 GRN_OP_OR);
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &value);

    grn_report_column(ctx, action, "[scalar]", scope);
    return true;
  } else if (grn_obj_is_vector_column(ctx, scope)) {
    grn_posting_internal posting = {0};
    grn_obj values;

    posting.weight_float = 1;
    GRN_RECORD_INIT(&values, GRN_OBJ_VECTOR, grn_obj_get_range(ctx, scope));
    GRN_TABLE_EACH_BEGIN(ctx, res, cursor, id)
    {
      grn_id *matched_id;
      unsigned int i, n;

      grn_table_cursor_get_key(ctx, cursor, (void **)&matched_id);
      GRN_BULK_REWIND(&values);
      grn_obj_get_value(ctx, scope, *matched_id, &values);
      n = grn_vector_size(ctx, &values);
      for (i = 0; i < n; i++) {
        posting.rid =
          grn_uvector_get_element(ctx, &values, i, &(posting.weight));
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)base_res,
                                 GRN_OP_OR);
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &values);

    grn_report_column(ctx, action, "[vector]", scope);
    return true;
  } else if (grn_obj_is_index_column(ctx, scope)) {
    GRN_TABLE_EACH_BEGIN(ctx, res, cursor, id)
    {
      grn_id *matched_id;
      grn_table_cursor_get_key(ctx, cursor, (void **)&matched_id);
      grn_ii_at(ctx,
                (grn_ii *)scope,
                *matched_id,
                (grn_hash *)base_res,
                GRN_OP_OR);
    }
    GRN_TABLE_EACH_END(ctx, cursor);

    grn_report_column(ctx, action, "[index]", scope);
    return true;
  } else if (grn_obj_is_accessor(ctx, scope)) {
    grn_accessor *accessor = (grn_accessor *)scope;
    if (!sub_filter_pre_filter_accessor_is_available(ctx, accessor)) {
      return false;
    }
    GRN_TABLE_EACH_BEGIN(ctx, res, cursor, id)
    {
      grn_id *matched_id;
      grn_table_cursor_get_key(ctx, cursor, (void **)&matched_id);
      sub_filter_pre_filter_accessor(ctx, accessor, *matched_id, base_res);
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    grn_report_accessor(ctx, action, "", scope);
    return true;
  } else {
    return false;
  }
}

static grn_rc
run_sub_filter(grn_ctx *ctx,
               grn_obj *table,
               int nargs,
               grn_obj **args,
               grn_obj *res,
               grn_operator op)
{
  const char *tag = "[sub-filter]";
  grn_rc rc = GRN_SUCCESS;
  grn_obj *scope;
  grn_obj *sub_filter_string;
  int32_t pre_filter_threshold = grn_sub_filter_pre_filter_threshold;
  grn_obj *scope_domain = NULL;
  grn_obj *sub_filter = NULL;
  grn_obj *dummy_variable = NULL;

  if (!(nargs >= 2 && nargs <= 3)) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s wrong number of arguments (%d for 2..3)",
        tag,
        nargs);
    rc = ctx->rc;
    goto exit;
  }

  scope = args[0];
  sub_filter_string = args[1];
  if (nargs == 3) {
    grn_proc_options_parse(ctx,
                           args[2],
                           tag,
                           "pre_filter_threshold",
                           GRN_PROC_OPTION_VALUE_INT32,
                           &pre_filter_threshold,
                           NULL);
  }

  switch (scope->header.type) {
  case GRN_ACCESSOR:
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_INDEX:
    break;
  default:
    /* TODO: put inspected the 1st argument to message */
    ERR(GRN_INVALID_ARGUMENT,
        "%s the 1st argument must be column or accessor",
        tag);
    rc = ctx->rc;
    goto exit;
    break;
  }

  scope_domain = grn_ctx_at(ctx, grn_obj_get_range(ctx, scope));

  if (sub_filter_string->header.domain != GRN_DB_TEXT) {
    /* TODO: put inspected the 2nd argument to message */
    ERR(GRN_INVALID_ARGUMENT, "%s the 2nd argument must be String", tag);
    rc = ctx->rc;
    goto exit;
  }
  if (GRN_TEXT_LEN(sub_filter_string) == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s the 2nd argument must not be empty String",
        tag);
    rc = ctx->rc;
    goto exit;
  }

  GRN_EXPR_CREATE_FOR_QUERY(ctx, scope_domain, sub_filter, dummy_variable);
  if (!sub_filter) {
    rc = ctx->rc;
    goto exit;
  }

  grn_expr_parse(ctx,
                 sub_filter,
                 GRN_TEXT_VALUE(sub_filter_string),
                 GRN_TEXT_LEN(sub_filter_string),
                 NULL,
                 GRN_OP_MATCH,
                 GRN_OP_AND,
                 GRN_EXPR_SYNTAX_SCRIPT);
  if (ctx->rc != GRN_SUCCESS) {
    rc = ctx->rc;
    goto exit;
  }

  {
    grn_obj *base_res = NULL;
    grn_operator select_op = GRN_OP_OR;

    base_res = grn_table_create(ctx,
                                NULL,
                                0,
                                NULL,
                                GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                scope_domain,
                                NULL);
    if (op == GRN_OP_AND && sub_filter_pre_filter(ctx,
                                                  res,
                                                  scope,
                                                  base_res,
                                                  pre_filter_threshold)) {
      if (base_res) {
        void *key = NULL, *value = NULL;
        uint32_t key_size = 0;

        GRN_TABLE_EACH(ctx, base_res, 0, 0, id, &key, &key_size, &value, {
          grn_rset_recinfo *ri = value;
          ri->score = 0;
        });
      }
      select_op = GRN_OP_AND;
    }
    grn_table_select(ctx, scope_domain, sub_filter, base_res, select_op);
    if (scope->header.type == GRN_ACCESSOR) {
      rc = grn_accessor_resolve(ctx, scope, -1, base_res, res, op);
    } else {
      grn_accessor accessor;
      accessor.header.type = GRN_ACCESSOR;
      accessor.obj = scope;
      accessor.action = GRN_ACCESSOR_GET_COLUMN_VALUE;
      accessor.next = NULL;
      rc =
        grn_accessor_resolve(ctx, (grn_obj *)&accessor, -1, base_res, res, op);
    }
    grn_obj_unlink(ctx, base_res);
  }

exit:
  if (scope_domain) {
    grn_obj_unlink(ctx, scope_domain);
  }
  if (sub_filter) {
    grn_obj_unlink(ctx, sub_filter);
  }

  return rc;
}

static grn_rc
selector_sub_filter(grn_ctx *ctx,
                    grn_obj *table,
                    grn_obj *index,
                    int nargs,
                    grn_obj **args,
                    grn_obj *res,
                    grn_operator op)
{
  return run_sub_filter(ctx, table, nargs - 1, args + 1, res, op);
}

static grn_obj *
func_html_untag(grn_ctx *ctx,
                int nargs,
                grn_obj **args,
                grn_user_data *user_data)
{
  grn_obj *html_arg;
  int html_arg_domain;
  grn_obj html;
  grn_obj *text;
  const char *html_raw;
  int i, length;
  bool in_tag = false;

  if (nargs != 1) {
    ERR(GRN_INVALID_ARGUMENT, "HTML is missing");
    return NULL;
  }

  html_arg = args[0];
  html_arg_domain = html_arg->header.domain;
  switch (html_arg_domain) {
  case GRN_DB_SHORT_TEXT:
  case GRN_DB_TEXT:
  case GRN_DB_LONG_TEXT:
    GRN_VALUE_VAR_SIZE_INIT(&html, GRN_OBJ_DO_SHALLOW_COPY, html_arg_domain);
    GRN_TEXT_SET(ctx, &html, GRN_TEXT_VALUE(html_arg), GRN_TEXT_LEN(html_arg));
    break;
  default:
    GRN_TEXT_INIT(&html, 0);
    if (grn_obj_cast(ctx, html_arg, &html, false)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, html_arg);
      ERR(GRN_INVALID_ARGUMENT,
          "failed to cast to text: <%.*s>",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      GRN_OBJ_FIN(ctx, &html);
      return NULL;
    }
    break;
  }

  text = GRN_PROC_ALLOC(html.header.domain, 0);
  if (!text) {
    GRN_OBJ_FIN(ctx, &html);
    return NULL;
  }

  html_raw = GRN_TEXT_VALUE(&html);
  length = GRN_TEXT_LEN(&html);
  for (i = 0; i < length; i++) {
    switch (html_raw[i]) {
    case '<':
      in_tag = true;
      break;
    case '>':
      if (in_tag) {
        in_tag = false;
      } else {
        GRN_TEXT_PUTC(ctx, text, html_raw[i]);
      }
      break;
    default:
      if (!in_tag) {
        GRN_TEXT_PUTC(ctx, text, html_raw[i]);
      }
      break;
    }
  }

  GRN_OBJ_FIN(ctx, &html);

  return text;
}

static bool
grn_text_equal_cstr(grn_ctx *ctx, grn_obj *text, const char *cstr)
{
  size_t cstr_len = strlen(cstr);
  return (GRN_TEXT_LEN(text) == cstr_len &&
          strncmp(GRN_TEXT_VALUE(text), cstr, cstr_len) == 0);
}

typedef enum {
  BETWEEN_BORDER_INVALID,
  BETWEEN_BORDER_INCLUDE,
  BETWEEN_BORDER_EXCLUDE
} between_border_type;

typedef struct {
  grn_obj *value;
  grn_obj *min;
  grn_obj casted_min;
  between_border_type min_border_type;
  grn_obj *max;
  grn_obj casted_max;
  between_border_type max_border_type;
  int cursor_flags;
  double too_many_index_match_ratio;
  const char *tag;
} between_data;

static void
between_data_init(grn_ctx *ctx, between_data *data)
{
  GRN_VOID_INIT(&(data->casted_min));
  GRN_VOID_INIT(&(data->casted_max));
  data->cursor_flags = 0;
  data->too_many_index_match_ratio = grn_between_too_many_index_match_ratio;
  data->tag = "[between]";
}

static void
between_data_fin(grn_ctx *ctx, between_data *data)
{
  GRN_OBJ_FIN(ctx, &(data->casted_min));
  GRN_OBJ_FIN(ctx, &(data->casted_max));
}

static between_border_type
between_parse_border(grn_ctx *ctx,
                     grn_obj *border,
                     const char *argument_description)
{
  grn_obj inspected;

  /* TODO: support other text types */
  if (border->header.domain == GRN_DB_TEXT) {
    if (grn_text_equal_cstr(ctx, border, "include")) {
      return BETWEEN_BORDER_INCLUDE;
    } else if (grn_text_equal_cstr(ctx, border, "exclude")) {
      return BETWEEN_BORDER_EXCLUDE;
    }
  }

  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, border);
  ERR(GRN_INVALID_ARGUMENT,
      "between(): %s must be \"include\" or \"exclude\": <%.*s>",
      argument_description,
      (int)GRN_TEXT_LEN(&inspected),
      GRN_TEXT_VALUE(&inspected));
  grn_obj_unlink(ctx, &inspected);

  return BETWEEN_BORDER_INVALID;
}

static grn_rc
between_cast(grn_ctx *ctx,
             grn_obj *source,
             grn_obj *destination,
             grn_id domain,
             const char *target_argument_name)
{
  grn_rc rc;

  GRN_OBJ_INIT(destination, GRN_BULK, 0, domain);
  rc = grn_obj_cast(ctx, source, destination, false);
  if (rc != GRN_SUCCESS) {
    grn_obj inspected_source;
    grn_obj *domain_object;
    char domain_name[GRN_TABLE_MAX_KEY_SIZE];
    int domain_name_length;

    GRN_TEXT_INIT(&inspected_source, 0);
    grn_inspect(ctx, &inspected_source, source);

    domain_object = grn_ctx_at(ctx, domain);
    domain_name_length =
      grn_obj_name(ctx, domain_object, domain_name, GRN_TABLE_MAX_KEY_SIZE);

    ERR(rc,
        "between(): failed to cast %s: <%.*s> -> <%.*s>",
        target_argument_name,
        (int)GRN_TEXT_LEN(&inspected_source),
        GRN_TEXT_VALUE(&inspected_source),
        domain_name_length,
        domain_name);

    grn_obj_unlink(ctx, &inspected_source);
    grn_obj_unlink(ctx, domain_object);
  }

  return rc;
}

static void
between_parse_options(grn_ctx *ctx, grn_obj *options, between_data *data)
{
  grn_proc_options_parse(ctx,
                         options,
                         data->tag,
                         "too_many_index_match_ratio",
                         GRN_PROC_OPTION_VALUE_DOUBLE,
                         &(data->too_many_index_match_ratio),
                         NULL);
}

static grn_rc
between_parse_args(grn_ctx *ctx, int nargs, grn_obj **args, between_data *data)
{
  grn_rc rc = GRN_SUCCESS;

  data->value = args[0];
  data->min = args[1];
  switch (nargs) {
  case 3:
  case 4:
    data->min_border_type = BETWEEN_BORDER_INCLUDE;
    data->max = args[2];
    data->max_border_type = BETWEEN_BORDER_INCLUDE;
    if (nargs == 4) {
      between_parse_options(ctx, args[3], data);
      if (ctx->rc != GRN_SUCCESS) {
        rc = ctx->rc;
        goto exit;
      }
    }
    break;
  case 5:
  case 6:
    data->min_border_type =
      between_parse_border(ctx, args[2], "the 3rd argument (min_border)");
    if (data->min_border_type == BETWEEN_BORDER_INVALID) {
      rc = ctx->rc;
      goto exit;
    }
    data->max = args[3];
    data->max_border_type =
      between_parse_border(ctx, args[4], "the 5th argument (max_border)");
    if (data->max_border_type == BETWEEN_BORDER_INVALID) {
      rc = ctx->rc;
      goto exit;
    }
    if (nargs == 6) {
      between_parse_options(ctx, args[5], data);
      if (ctx->rc != GRN_SUCCESS) {
        rc = ctx->rc;
        goto exit;
      }
    }
    break;
  default:
    ERR(GRN_INVALID_ARGUMENT,
        "between(): wrong number of arguments (%d for 3 or 5)",
        nargs);
    rc = ctx->rc;
    goto exit;
  }

  {
    grn_id value_type;
    switch (data->value->header.type) {
    case GRN_BULK:
      value_type = data->value->header.domain;
      break;
    case GRN_COLUMN_INDEX:
      {
        grn_obj *domain_object;
        domain_object = grn_ctx_at(ctx, data->value->header.domain);
        value_type = domain_object->header.domain;
      }
      break;
    default:
      value_type = grn_obj_get_range(ctx, data->value);
      break;
    }
    if (value_type != data->min->header.domain) {
      rc = between_cast(ctx, data->min, &data->casted_min, value_type, "min");
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
      data->min = &(data->casted_min);
    }

    if (value_type != data->max->header.domain) {
      rc = between_cast(ctx, data->max, &data->casted_max, value_type, "max");
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
      data->max = &(data->casted_max);
    }
  }

exit:
  return rc;
}

static bool
between_create_expr(grn_ctx *ctx,
                    grn_obj *table,
                    between_data *data,
                    grn_obj **expr,
                    grn_obj **variable)
{
  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, *expr, *variable);
  if (!*expr) {
    return false;
  }
  if (!*variable) {
    return false;
  }

  if (data->value->header.type == GRN_BULK) {
    grn_expr_append_obj(ctx, *expr, data->value, GRN_OP_PUSH, 1);
  } else {
    grn_expr_append_obj(ctx, *expr, data->value, GRN_OP_GET_VALUE, 1);
  }
  grn_expr_append_obj(ctx, *expr, data->min, GRN_OP_PUSH, 1);
  if (data->min_border_type == BETWEEN_BORDER_INCLUDE) {
    grn_expr_append_op(ctx, *expr, GRN_OP_GREATER_EQUAL, 2);
  } else {
    grn_expr_append_op(ctx, *expr, GRN_OP_GREATER, 2);
  }

  if (data->value->header.type == GRN_BULK) {
    grn_expr_append_obj(ctx, *expr, data->value, GRN_OP_PUSH, 1);
  } else {
    grn_expr_append_obj(ctx, *expr, data->value, GRN_OP_GET_VALUE, 1);
  }
  grn_expr_append_obj(ctx, *expr, data->max, GRN_OP_PUSH, 1);
  if (data->max_border_type == BETWEEN_BORDER_INCLUDE) {
    grn_expr_append_op(ctx, *expr, GRN_OP_LESS_EQUAL, 2);
  } else {
    grn_expr_append_op(ctx, *expr, GRN_OP_LESS, 2);
  }

  grn_expr_append_op(ctx, *expr, GRN_OP_AND, 2);

  return true;
}

static grn_obj *
func_between(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *found;
  between_data data;
  grn_obj *condition = NULL;
  grn_obj *variable;
  grn_obj *table = NULL;
  grn_obj *between_expr;
  grn_obj *between_variable;
  grn_obj *result;

  found = GRN_PROC_ALLOC(GRN_DB_BOOL, 0);
  if (!found) {
    return NULL;
  }
  GRN_BOOL_SET(ctx, found, false);

  grn_proc_get_info(ctx, user_data, NULL, NULL, &condition);
  if (!condition) {
    return found;
  }

  variable = grn_expr_get_var_by_offset(ctx, condition, 0);
  if (!variable) {
    return found;
  }

  between_data_init(ctx, &data);
  rc = between_parse_args(ctx, nargs, args, &data);
  if (rc != GRN_SUCCESS) {
    goto exit;
  }

  table = grn_ctx_at(ctx, variable->header.domain);
  if (!table) {
    goto exit;
  }
  if (!between_create_expr(ctx,
                           table,
                           &data,
                           &between_expr,
                           &between_variable)) {
    goto exit;
  }

  GRN_RECORD_SET(ctx, between_variable, GRN_RECORD_VALUE(variable));
  result = grn_expr_exec(ctx, between_expr, 0);
  if (grn_obj_is_true(ctx, result)) {
    GRN_BOOL_SET(ctx, found, true);
  }

  grn_obj_unlink(ctx, between_expr);

exit:
  between_data_fin(ctx, &data);
  if (table) {
    grn_obj_unref(ctx, table);
  }

  return found;
}

static bool
selector_between_sequential_search_should_use(grn_ctx *ctx,
                                              grn_obj *table,
                                              grn_obj *index,
                                              grn_obj *index_table,
                                              between_data *data,
                                              grn_obj *res,
                                              grn_operator op)
{
  if (data->too_many_index_match_ratio < 0.0) {
    return false;
  }

  if (op != GRN_OP_AND) {
    return false;
  }

  if (!index) {
    return false;
  }

  if (index->header.flags & GRN_OBJ_WITH_WEIGHT) {
    return false;
  }

  if (data->value->header.type == GRN_COLUMN_INDEX) {
    return false;
  }

  grn_table_cursor *cursor = grn_table_cursor_open(ctx,
                                                   index_table,
                                                   GRN_BULK_HEAD(data->min),
                                                   GRN_BULK_VSIZE(data->min),
                                                   GRN_BULK_HEAD(data->max),
                                                   GRN_BULK_VSIZE(data->max),
                                                   0,
                                                   -1,
                                                   data->cursor_flags);
  if (!cursor) {
    return false;
  }
  uint32_t estimated_size =
    grn_ii_estimate_size_for_lexicon_cursor(ctx, (grn_ii *)index, cursor);
  grn_table_cursor_close(ctx, cursor);

  if (estimated_size == 0) {
    return false;
  }

  uint32_t n_existing_records = grn_table_size(ctx, res);
  double too_many_index_match_threshold =
    (estimated_size * data->too_many_index_match_ratio);
  bool use_sequential_search =
    (n_existing_records < too_many_index_match_threshold);
  if (use_sequential_search) {
    grn_obj reason;
    GRN_TEXT_INIT(&reason, 0);
    grn_text_printf(ctx,
                    &reason,
                    "too many index match: "
                    "%d < %f (%u * %f)",
                    n_existing_records,
                    too_many_index_match_threshold,
                    estimated_size,
                    data->too_many_index_match_ratio);
    GRN_TEXT_PUTC(ctx, &reason, '\0');
    grn_report_index_not_used(ctx,
                              data->tag,
                              "",
                              index,
                              GRN_TEXT_VALUE(&reason));
    GRN_OBJ_FIN(ctx, &reason);
  }
  return use_sequential_search;
}

static grn_rc
selector_between_sequential_search(grn_ctx *ctx,
                                   grn_obj *table,
                                   between_data *data,
                                   grn_obj *res,
                                   grn_operator op)
{
  {
    int offset = 0;
    int limit = -1;
    int flags = 0;
    grn_obj *target_table;
    grn_obj *target_column;
    grn_operator_exec_func *greater;
    grn_operator_exec_func *less;
    grn_table_cursor *cursor;
    grn_id id;
    grn_obj value;

    if (op == GRN_OP_AND) {
      target_table = res;
    } else {
      target_table = table;
    }
    cursor = grn_table_cursor_open(ctx,
                                   target_table,
                                   NULL,
                                   0,
                                   NULL,
                                   0,
                                   offset,
                                   limit,
                                   flags);
    if (!cursor) {
      return ctx->rc;
    }

    if (data->value->header.type == GRN_BULK) {
      target_column = grn_obj_column(ctx,
                                     table,
                                     GRN_TEXT_VALUE(data->value),
                                     GRN_TEXT_LEN(data->value));
    } else {
      target_column = data->value;
    }
    if (data->min_border_type == BETWEEN_BORDER_INCLUDE) {
      greater = grn_operator_exec_greater_equal;
    } else {
      greater = grn_operator_exec_greater;
    }
    if (data->max_border_type == BETWEEN_BORDER_INCLUDE) {
      less = grn_operator_exec_less_equal;
    } else {
      less = grn_operator_exec_less;
    }

    GRN_VOID_INIT(&value);
    while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
      grn_id record_id;

      if (target_table == res) {
        grn_id *key;
        grn_table_cursor_get_key(ctx, cursor, (void **)&key);
        record_id = *key;
      } else {
        record_id = id;
      }

      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, target_column, record_id, &value);
      if (greater(ctx, &value, data->min) && less(ctx, &value, data->max)) {
        grn_posting_internal posting = {0};
        posting.rid = record_id;
        posting.sid = 1;
        posting.pos = 0;
        posting.weight_float = 1;
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)res,
                                 op);
      }
    }

    GRN_OBJ_FIN(ctx, &value);

    if (target_column != data->value &&
        target_column->header.type == GRN_ACCESSOR) {
      grn_obj_unlink(ctx, target_column);
    }

    grn_table_cursor_close(ctx, cursor);

    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  }

  return GRN_SUCCESS;
}

static grn_rc
selector_between(grn_ctx *ctx,
                 grn_obj *table,
                 grn_obj *index,
                 int nargs,
                 grn_obj **args,
                 grn_obj *res,
                 grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  int offset = 0;
  int limit = -1;
  between_data data;
  bool use_sequential_search;
  bool index_table_need_unref = false;
  grn_obj *index_table = NULL;
  grn_table_cursor *cursor;

  between_data_init(ctx, &data);
  data.cursor_flags = GRN_CURSOR_ASCENDING | GRN_CURSOR_BY_KEY;
  rc = between_parse_args(ctx, nargs - 1, args + 1, &data);
  if (rc != GRN_SUCCESS) {
    goto exit;
  }

  if (data.min_border_type == BETWEEN_BORDER_EXCLUDE) {
    data.cursor_flags |= GRN_CURSOR_GT;
  }
  if (data.max_border_type == BETWEEN_BORDER_EXCLUDE) {
    data.cursor_flags |= GRN_CURSOR_LT;
  }

  if (data.value->header.type == GRN_COLUMN_INDEX) {
    index = data.value;
  }

  if (index) {
    switch (index->header.type) {
    case GRN_TABLE_NO_KEY:
    case GRN_TABLE_HASH_KEY:
      break;
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      index_table = index;
      index = NULL;
      break;
    default:
      index_table = grn_ctx_at(ctx, index->header.domain);
      if (index_table) {
        index_table_need_unref = true;
      }
      break;
    }
  }

  if (index_table) {
    use_sequential_search =
      selector_between_sequential_search_should_use(ctx,
                                                    table,
                                                    index,
                                                    index_table,
                                                    &data,
                                                    res,
                                                    op);
  } else {
    use_sequential_search = true;
  }
  if (use_sequential_search) {
    rc = selector_between_sequential_search(ctx, table, &data, res, op);
    goto exit;
  }

  cursor = grn_table_cursor_open(ctx,
                                 index_table,
                                 GRN_BULK_HEAD(data.min),
                                 GRN_BULK_VSIZE(data.min),
                                 GRN_BULK_HEAD(data.max),
                                 GRN_BULK_VSIZE(data.max),
                                 offset,
                                 limit,
                                 data.cursor_flags);
  if (!cursor) {
    rc = ctx->rc;
    goto exit;
  }

  if (index) {
    grn_obj *index_cursor =
      grn_index_cursor_open(ctx, cursor, index, GRN_ID_NIL, GRN_ID_MAX, 0);
    if (index_cursor) {
      grn_result_set_add_index_cursor(ctx,
                                      (grn_hash *)res,
                                      index_cursor,
                                      1,
                                      1,
                                      op);
      grn_obj_close(ctx, index_cursor);
    }
  } else {
    grn_result_set_add_table_cursor(ctx, (grn_hash *)res, cursor, 1, op);
  }
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  grn_table_cursor_close(ctx, cursor);

exit:
  between_data_fin(ctx, &data);

  if (index_table_need_unref) {
    grn_obj_unref(ctx, index_table);
  }

  return rc;
}

static grn_obj *
func_in_values(grn_ctx *ctx,
               int nargs,
               grn_obj **args,
               grn_user_data *user_data)
{
  grn_obj *found;
  grn_obj *target_value;
  int i;

  found = GRN_PROC_ALLOC(GRN_DB_BOOL, 0);
  if (!found) {
    return NULL;
  }
  GRN_BOOL_SET(ctx, found, false);

  if (nargs < 1) {
    ERR(GRN_INVALID_ARGUMENT,
        "in_values(): wrong number of arguments (%d for 1..)",
        nargs);
    return found;
  }

  target_value = args[0];
  for (i = 1; i < nargs; i++) {
    grn_obj *value = args[i];

    bool result = grn_operator_exec_equal(ctx, target_value, value);
    if (ctx->rc) {
      break;
    }

    if (result) {
      GRN_BOOL_SET(ctx, found, true);
      break;
    }
  }

  return found;
}

static bool
is_reference_type_column(grn_ctx *ctx, grn_obj *column)
{
  bool is_reference_type;
  grn_obj *range;

  range = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));
  switch (range->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    is_reference_type = true;
    break;
  default:
    is_reference_type = false;
    break;
  }
  grn_obj_unlink(ctx, range);

  return is_reference_type;
}

static grn_obj *
selector_in_values_find_source(grn_ctx *ctx, grn_obj *index, grn_obj *res)
{
  grn_id source_id = GRN_ID_NIL;
  grn_obj source_ids;
  unsigned int n_source_ids;

  GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
  grn_obj_get_info(ctx, index, GRN_INFO_SOURCE, &source_ids);
  n_source_ids = GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id);
  if (n_source_ids == 1) {
    source_id = GRN_UINT32_VALUE_AT(&source_ids, 0);
  }
  GRN_OBJ_FIN(ctx, &source_ids);

  if (source_id == GRN_ID_NIL) {
    return NULL;
  } else {
    return grn_ctx_at(ctx, source_id);
  }
}

static bool
selector_in_values_sequential_search(grn_ctx *ctx,
                                     grn_obj *table,
                                     grn_obj *index,
                                     int n_values,
                                     grn_obj **values,
                                     grn_obj *res,
                                     grn_operator op,
                                     double too_many_index_match_ratio,
                                     const char *tag)
{
  grn_obj *source;
  int n_existing_records;

  if (too_many_index_match_ratio < 0.0) {
    return false;
  }

  if (op != GRN_OP_AND) {
    return false;
  }

  if (index->header.flags & GRN_OBJ_WITH_WEIGHT) {
    return false;
  }

  n_existing_records = grn_table_size(ctx, res);
  if (n_existing_records == 0) {
    return true;
  }

  source = selector_in_values_find_source(ctx, index, res);
  if (!source) {
    return false;
  }

  if (!is_reference_type_column(ctx, source)) {
    grn_obj_unlink(ctx, source);
    return false;
  }

  uint32_t n_indexed_records = 0;
  {
    grn_obj value_ids;
    int i, n_value_ids;

    {
      grn_id range_id;
      grn_obj *range;

      range_id = grn_obj_get_range(ctx, source);
      range = grn_ctx_at(ctx, range_id);
      if (!range) {
        grn_obj_unlink(ctx, source);
        return false;
      }

      GRN_RECORD_INIT(&value_ids, GRN_OBJ_VECTOR, range_id);
      for (i = 0; i < n_values; i++) {
        grn_obj *value = values[i];
        grn_id value_id;

        value_id =
          grn_table_get(ctx, range, GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
        if (value_id == GRN_ID_NIL) {
          continue;
        }
        GRN_RECORD_PUT(ctx, &value_ids, value_id);
      }
      grn_obj_unlink(ctx, range);
    }

    n_value_ids = GRN_BULK_VSIZE(&value_ids) / sizeof(grn_id);
    for (i = 0; i < n_value_ids; i++) {
      grn_id value_id = GRN_RECORD_VALUE_AT(&value_ids, i);
      n_indexed_records += grn_ii_estimate_size(ctx, (grn_ii *)index, value_id);
    }

    /*
     * Same as:
     * ((n_existing_record / n_indexed_records) >
     *  grn_in_values_too_many_index_match_ratio)
     */
    if (n_existing_records > (n_indexed_records * too_many_index_match_ratio)) {
      grn_obj_unlink(ctx, &value_ids);
      grn_obj_unlink(ctx, source);
      return false;
    }

    {
      grn_obj *accessor;
      char local_source_name[GRN_TABLE_MAX_KEY_SIZE];
      int local_source_name_length;

      local_source_name_length =
        grn_column_name(ctx, source, local_source_name, GRN_TABLE_MAX_KEY_SIZE);
      accessor =
        grn_obj_column(ctx, res, local_source_name, local_source_name_length);
      {
        grn_table_cursor *cursor;
        grn_id id;
        grn_obj record_value;

        GRN_VOID_INIT(&record_value);
        cursor = grn_table_cursor_open(ctx,
                                       res,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       0,
                                       -1,
                                       GRN_CURSOR_ASCENDING);
        while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
          grn_id *record_id;
          grn_table_cursor_get_key(ctx, cursor, (void **)&record_id);
          GRN_BULK_REWIND(&record_value);
          grn_obj_get_value(ctx, accessor, id, &record_value);
          for (i = 0; i < n_value_ids; i++) {
            grn_id value_id = GRN_RECORD_VALUE_AT(&value_ids, i);
            switch (record_value.header.type) {
            case GRN_BULK:
              if (value_id == GRN_RECORD_VALUE(&record_value)) {
                grn_posting_internal posting = {0};
                posting.rid = *record_id;
                posting.sid = 1;
                posting.pos = 0;
                posting.weight_float = 1;
                grn_ii_posting_add_float(ctx,
                                         (grn_posting *)(&posting),
                                         (grn_hash *)res,
                                         op);
              }
              break;
            case GRN_UVECTOR:
              {
                int j, n_elements;
                n_elements = GRN_BULK_VSIZE(&record_value) / sizeof(grn_id);
                for (j = 0; j < n_elements; j++) {
                  if (value_id == GRN_RECORD_VALUE_AT(&record_value, j)) {
                    grn_posting_internal posting = {0};
                    posting.rid = *record_id;
                    posting.sid = 1;
                    posting.pos = 0;
                    posting.weight_float = 1;
                    grn_ii_posting_add_float(ctx,
                                             (grn_posting *)(&posting),
                                             (grn_hash *)res,
                                             op);
                  }
                }
              }
              break;
            default:
              break;
            }
          }
        }
        grn_table_cursor_close(ctx, cursor);
        grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
        GRN_OBJ_FIN(ctx, &record_value);
      }
      grn_obj_unlink(ctx, accessor);
    }
    grn_obj_unlink(ctx, &value_ids);
  }
  grn_obj_unlink(ctx, source);

  {
    grn_obj reason;
    GRN_TEXT_INIT(&reason, 0);
    grn_text_printf(ctx,
                    &reason,
                    "too many index match: "
                    "%d < %f (%u * %f)",
                    n_existing_records,
                    (n_indexed_records * too_many_index_match_ratio),
                    n_indexed_records,
                    too_many_index_match_ratio);
    GRN_TEXT_PUTC(ctx, &reason, '\0');
    grn_report_index_not_used(ctx, tag, "", index, GRN_TEXT_VALUE(&reason));
    GRN_OBJ_FIN(ctx, &reason);
  }

  return true;
}

static void
in_values_parse_options(grn_ctx *ctx,
                        grn_obj *options,
                        double *too_many_index_match_ratio,
                        const char *tag)
{
  grn_proc_options_parse(ctx,
                         options,
                         tag,
                         "too_many_index_match_ratio",
                         GRN_PROC_OPTION_VALUE_DOUBLE,
                         too_many_index_match_ratio,
                         NULL);
}

static grn_rc
selector_in_values(grn_ctx *ctx,
                   grn_obj *table,
                   grn_obj *index,
                   int nargs,
                   grn_obj **args,
                   grn_obj *res,
                   grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  int i, n_values;
  grn_obj **values;
  double too_many_index_match_ratio = grn_in_values_too_many_index_match_ratio;
  const char *tag = "[in_values]";

  if (!index) {
    return GRN_INVALID_ARGUMENT;
  }

  if (nargs < 2) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s wrong number of arguments (%d for 1..)",
        tag,
        nargs);
    return ctx->rc;
  }

  n_values = nargs - 2;
  if (args[nargs - 1]->header.type == GRN_TABLE_HASH_KEY) {
    in_values_parse_options(ctx,
                            args[nargs - 1],
                            &too_many_index_match_ratio,
                            tag);
    if (ctx->rc != GRN_SUCCESS) {
      return ctx->rc;
    }
    n_values--;
  }
  values = args + 2;

  if (n_values == 0) {
    return rc;
  }

  if (selector_in_values_sequential_search(ctx,
                                           table,
                                           index,
                                           n_values,
                                           values,
                                           res,
                                           op,
                                           too_many_index_match_ratio,
                                           tag)) {
    return ctx->rc;
  }

  int original_flags = ctx->flags;
  ctx->flags |= GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND;
  for (i = 0; i < n_values; i++) {
    grn_obj *value = values[i];
    grn_search_optarg search_options;
    memset(&search_options, 0, sizeof(grn_search_optarg));
    search_options.mode = GRN_OP_EXACT;
    search_options.similarity_threshold = 0;
    search_options.max_interval = 0;
    search_options.additional_last_interval = 0;
    search_options.weight_vector = NULL;
    search_options.vector_size = 0;
    search_options.proc = NULL;
    search_options.max_size = 0;
    search_options.scorer = NULL;
    search_options.query_options = NULL;
    search_options.min_interval = NULL;
    search_options.start_position = NULL;
    if (i == n_values - 1) {
      ctx->flags = original_flags;
    }
    rc = grn_obj_search(ctx, index, value, res, op, &search_options);
    if (rc != GRN_SUCCESS) {
      break;
    }
  }

  return rc;
}

static grn_obj *
proc_range_filter(grn_ctx *ctx,
                  int nargs,
                  grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *table_name = VAR(0);
  grn_obj *column_name = VAR(1);
  grn_obj *min = VAR(2);
  grn_obj *min_border = VAR(3);
  grn_obj *max = VAR(4);
  grn_obj *max_border = VAR(5);
  grn_obj *offset = VAR(6);
  grn_obj *limit = VAR(7);
  grn_obj *filter = VAR(8);
  grn_obj *output_columns = VAR(9);
  grn_obj *table;
  grn_obj *res = NULL;
  grn_obj *filter_expr = NULL;
  grn_obj *filter_variable = NULL;
  int real_offset;
  int real_limit;

  table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(table_name), GRN_TEXT_LEN(table_name));
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT,
        "[range_filter] nonexistent table <%.*s>",
        (int)GRN_TEXT_LEN(table_name),
        GRN_TEXT_VALUE(table_name));
    return NULL;
  }

  if (GRN_TEXT_LEN(filter) > 0) {
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, filter_expr, filter_variable);
    if (!filter_expr) {
      ERR(GRN_INVALID_ARGUMENT, "[range_filter] failed to create expression");
      goto exit;
    }

    grn_expr_parse(ctx,
                   filter_expr,
                   GRN_TEXT_VALUE(filter),
                   GRN_TEXT_LEN(filter),
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

  res = grn_table_create(ctx,
                         NULL,
                         0,
                         NULL,
                         GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                         table,
                         NULL);
  if (!res) {
    ERR(GRN_INVALID_ARGUMENT, "[range_filter] failed to result table");
    goto exit;
  }

  {
    grn_obj int32_value;

    GRN_INT32_INIT(&int32_value, 0);

    if (GRN_TEXT_LEN(offset) > 0) {
      if (grn_obj_cast(ctx, offset, &int32_value, false) != GRN_SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "[range_filter] invalid offset format: <%.*s>",
            (int)GRN_TEXT_LEN(offset),
            GRN_TEXT_VALUE(offset));
        GRN_OBJ_FIN(ctx, &int32_value);
        goto exit;
      }
      real_offset = GRN_INT32_VALUE(&int32_value);
    } else {
      real_offset = 0;
    }

    GRN_BULK_REWIND(&int32_value);

    if (GRN_TEXT_LEN(limit) > 0) {
      if (grn_obj_cast(ctx, limit, &int32_value, false) != GRN_SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "[range_filter] invalid limit format: <%.*s>",
            (int)GRN_TEXT_LEN(limit),
            GRN_TEXT_VALUE(limit));
        GRN_OBJ_FIN(ctx, &int32_value);
        goto exit;
      }
      real_limit = GRN_INT32_VALUE(&int32_value);
    } else {
      real_limit = GRN_SELECT_DEFAULT_LIMIT;
    }

    GRN_OBJ_FIN(ctx, &int32_value);
  }
  {
    grn_rc rc;
    int original_offset = real_offset;
    int original_limit = real_limit;
    rc = grn_output_range_normalize(ctx,
                                    grn_table_size(ctx, table),
                                    &real_offset,
                                    &real_limit);
    switch (rc) {
    case GRN_TOO_SMALL_OFFSET:
      ERR(GRN_INVALID_ARGUMENT,
          "[range_filter] too small offset: <%d>",
          original_offset);
      goto exit;
    case GRN_TOO_LARGE_OFFSET:
      ERR(GRN_INVALID_ARGUMENT,
          "[range_filter] too large offset: <%d>",
          original_offset);
      goto exit;
    case GRN_TOO_SMALL_LIMIT:
      ERR(GRN_INVALID_ARGUMENT,
          "[range_filter] too small limit: <%d>",
          original_limit);
      goto exit;
    default:
      break;
    }
  }

  if (real_limit != 0) {
    grn_table_sort_key *sort_keys;
    unsigned int n_sort_keys;
    sort_keys = grn_table_sort_key_from_str(ctx,
                                            GRN_TEXT_VALUE(column_name),
                                            GRN_TEXT_LEN(column_name),
                                            table,
                                            &n_sort_keys);
    if (n_sort_keys == 1) {
      grn_table_sort_key *sort_key;
      grn_obj *index;
      int n_indexes;
      grn_operator op = GRN_OP_OR;

      sort_key = &(sort_keys[0]);
      n_indexes =
        grn_column_index(ctx, sort_key->key, GRN_OP_LESS, &index, 1, NULL);
      if (n_indexes > 0) {
        grn_obj *lexicon;
        grn_table_cursor *table_cursor;
        int table_cursor_flags = 0;
        between_border_type min_border_type;
        between_border_type max_border_type;
        grn_obj real_min;
        grn_obj real_max;
        int n_records = 0;
        grn_obj *index_cursor;
        int index_cursor_flags = 0;
        grn_posting *posting;

        lexicon = grn_ctx_at(ctx, index->header.domain);
        if (sort_key->flags & GRN_TABLE_SORT_DESC) {
          table_cursor_flags |= GRN_CURSOR_DESCENDING;
        } else {
          table_cursor_flags |= GRN_CURSOR_ASCENDING;
        }
        if (GRN_TEXT_LEN(min_border) > 0) {
          min_border_type = between_parse_border(ctx, min_border, "min_border");
        } else {
          min_border_type = BETWEEN_BORDER_INCLUDE;
        }
        if (GRN_TEXT_LEN(max_border) > 0) {
          max_border_type = between_parse_border(ctx, max_border, "max_border");
        } else {
          max_border_type = BETWEEN_BORDER_INCLUDE;
        }
        if (min_border_type == BETWEEN_BORDER_EXCLUDE) {
          table_cursor_flags |= GRN_CURSOR_GT;
        }
        if (max_border_type == BETWEEN_BORDER_EXCLUDE) {
          table_cursor_flags |= GRN_CURSOR_LT;
        }
        GRN_OBJ_INIT(&real_min, GRN_BULK, 0, lexicon->header.domain);
        GRN_OBJ_INIT(&real_max, GRN_BULK, 0, lexicon->header.domain);
        if (GRN_TEXT_LEN(min) > 0) {
          grn_obj_cast(ctx, min, &real_min, false);
        }
        if (GRN_TEXT_LEN(max) > 0) {
          grn_obj_cast(ctx, max, &real_max, false);
        }
        table_cursor = grn_table_cursor_open(ctx,
                                             lexicon,
                                             GRN_BULK_HEAD(&real_min),
                                             GRN_BULK_VSIZE(&real_min),
                                             GRN_BULK_HEAD(&real_max),
                                             GRN_BULK_VSIZE(&real_max),
                                             0,
                                             -1,
                                             table_cursor_flags);
        index_cursor = grn_index_cursor_open(ctx,
                                             table_cursor,
                                             index,
                                             GRN_ID_NIL,
                                             GRN_ID_NIL,
                                             index_cursor_flags);
        while ((posting = grn_index_cursor_next(ctx, index_cursor, NULL))) {
          bool result_boolean = false;

          if (filter_expr) {
            grn_obj *result;
            GRN_RECORD_SET(ctx, filter_variable, posting->rid);
            result = grn_expr_exec(ctx, filter_expr, 0);
            if (ctx->rc) {
              break;
            }
            result_boolean = grn_obj_is_true(ctx, result);
          } else {
            result_boolean = true;
          }

          if (result_boolean) {
            if (n_records >= real_offset) {
              grn_posting_internal add_posting =
                *((grn_posting_internal *)posting);
              add_posting.weight_float += 1;
              grn_ii_posting_add_float(ctx,
                                       (grn_posting *)(&add_posting),
                                       (grn_hash *)res,
                                       op);
            }
            n_records++;
            if (n_records == real_limit) {
              break;
            }
          }
        }
        grn_obj_unlink(ctx, index_cursor);
        grn_table_cursor_close(ctx, table_cursor);

        GRN_OBJ_FIN(ctx, &real_min);
        GRN_OBJ_FIN(ctx, &real_max);
      }
      grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
    }
    grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
  }

  if (ctx->rc == GRN_SUCCESS) {
    const char *raw_output_columns;
    int raw_output_columns_len;

    raw_output_columns = GRN_TEXT_VALUE(output_columns);
    raw_output_columns_len = GRN_TEXT_LEN(output_columns);
    if (raw_output_columns_len == 0) {
      if (grn_obj_is_table_with_key(ctx, table)) {
        raw_output_columns = GRN_SELECT_DEFAULT_OUTPUT_COLUMNS_FOR_WITH_KEY;
      } else {
        raw_output_columns = GRN_SELECT_DEFAULT_OUTPUT_COLUMNS_FOR_NO_KEY;
      }
      raw_output_columns_len = strlen(raw_output_columns);
    }
    grn_proc_select_output_columns(ctx,
                                   res,
                                   -1,
                                   real_offset,
                                   real_limit,
                                   raw_output_columns,
                                   raw_output_columns_len,
                                   filter_expr);
  }

exit:
  if (filter_expr) {
    grn_obj_unlink(ctx, filter_expr);
  }
  if (res) {
    grn_obj_unlink(ctx, res);
  }

  return NULL;
}

static grn_obj *
proc_request_cancel(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *id = VAR(0);
  bool canceled;

  if (GRN_TEXT_LEN(id) == 0) {
    ERR(GRN_INVALID_ARGUMENT, "[request_cancel] ID is missing");
    return NULL;
  }

  canceled = grn_request_canceler_cancel(GRN_TEXT_VALUE(id), GRN_TEXT_LEN(id));

  GRN_OUTPUT_MAP_OPEN("result", 2);
  GRN_OUTPUT_CSTR("id");
  GRN_OUTPUT_STR(GRN_TEXT_VALUE(id), GRN_TEXT_LEN(id));
  GRN_OUTPUT_CSTR("canceled");
  GRN_OUTPUT_BOOL(canceled);
  GRN_OUTPUT_MAP_CLOSE();

  return NULL;
}

static grn_obj *
proc_plugin_register(grn_ctx *ctx,
                     int nargs,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *name;
    GRN_TEXT_PUTC(ctx, VAR(0), '\0');
    name = GRN_TEXT_VALUE(VAR(0));
    grn_plugin_register(ctx, name);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "[plugin_register] name is missing");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_plugin_unregister(grn_ctx *ctx,
                       int nargs,
                       grn_obj **args,
                       grn_user_data *user_data)
{
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *name;
    GRN_TEXT_PUTC(ctx, VAR(0), '\0');
    name = GRN_TEXT_VALUE(VAR(0));
    grn_plugin_unregister(ctx, name);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "[plugin_unregister] name is missing");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_io_flush(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *db = grn_ctx_db(ctx);

  grn_raw_string target_name;
  target_name.value = grn_plugin_proc_get_var_string(ctx,
                                                     user_data,
                                                     "target_name",
                                                     -1,
                                                     &(target_name.length));
  grn_obj *target;
  if (target_name.length > 0) {
    target = grn_ctx_get(ctx, target_name.value, target_name.length);
    if (!target) {
      ERR(GRN_INVALID_ARGUMENT,
          "[io_flush] unknown target: <%.*s>",
          (int)target_name.length,
          target_name.value);
      GRN_OUTPUT_BOOL(false);
      return NULL;
    }
  } else {
    target = db;
  }

  grn_raw_string recursive;
  recursive.value = grn_plugin_proc_get_var_string(ctx,
                                                   user_data,
                                                   "recursive",
                                                   -1,
                                                   &(recursive.length));
  bool is_only_opened =
    grn_plugin_proc_get_var_bool(ctx, user_data, "only_opened", -1, false);
  grn_rc rc = GRN_SUCCESS;
  if (is_only_opened) {
    rc = grn_obj_flush_only_opened(ctx, target);
  } else {
    if (GRN_RAW_STRING_EQUAL_CSTRING(recursive, "dependent")) {
      rc = grn_obj_flush_recursive_dependent(ctx, target);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(recursive, "no")) {
      rc = grn_obj_flush(ctx, target);
    } else {
      rc = grn_obj_flush_recursive(ctx, target);
    }
  }

  if (target->header.type != GRN_DB) {
    grn_obj_unlink(ctx, target);
  }

  GRN_OUTPUT_BOOL(rc == GRN_SUCCESS);

  return NULL;
}

static grn_obj *
proc_database_unmap(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_rc rc;
  uint32_t current_limit;

  current_limit = grn_thread_get_limit_with_ctx(ctx);
  if (current_limit != 1) {
    ERR(GRN_OPERATION_NOT_PERMITTED,
        "[database_unmap] the max number of threads must be 1: <%u>",
        current_limit);
    GRN_OUTPUT_BOOL(false);
    return NULL;
  }

  rc = grn_db_unmap(ctx, grn_ctx_db(ctx));
  GRN_OUTPUT_BOOL(rc == GRN_SUCCESS);

  return NULL;
}

static grn_obj *
proc_reindex(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *target_name;
  grn_obj *target;

  target_name = VAR(0);
  if (GRN_TEXT_LEN(target_name) == 0) {
    target = grn_ctx_db(ctx);
  } else {
    target =
      grn_ctx_get(ctx, GRN_TEXT_VALUE(target_name), GRN_TEXT_LEN(target_name));
    if (!target) {
      ERR(GRN_INVALID_ARGUMENT,
          "[reindex] nonexistent target: <%.*s>",
          (int)GRN_TEXT_LEN(target_name),
          GRN_TEXT_VALUE(target_name));
      GRN_OUTPUT_BOOL(false);
      return NULL;
    }
  }

  grn_obj_reindex(ctx, target);

  GRN_OUTPUT_BOOL(ctx->rc == GRN_SUCCESS);

  return NULL;
}

static grn_rc
selector_prefix_rk_search_key(
  grn_ctx *ctx, grn_obj *table, grn_obj *query, grn_obj *res, grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;

  if (table->header.type != GRN_TABLE_PAT_KEY) {
    grn_obj inspected_table;
    GRN_TEXT_INIT(&inspected_table, 0);
    grn_inspect(ctx, &inspected_table, table);
    ERR(GRN_INVALID_ARGUMENT,
        "prefix_rk_serach(): table of _key must TABLE_PAT_KEY: %.*s",
        (int)GRN_TEXT_LEN(&inspected_table),
        GRN_TEXT_VALUE(&inspected_table));
    rc = ctx->rc;
    GRN_OBJ_FIN(ctx, &inspected_table);
    goto exit;
  }

  GRN_TABLE_EACH_BEGIN_MIN(ctx,
                           table,
                           cursor,
                           id,
                           GRN_TEXT_VALUE(query),
                           GRN_TEXT_LEN(query),
                           GRN_CURSOR_PREFIX | GRN_CURSOR_RK)
  {
    grn_posting_internal posting = {0};
    posting.rid = id;
    posting.sid = 1;
    posting.pos = 0;
    posting.weight_float = 1;
    grn_ii_posting_add_float(ctx,
                             (grn_posting *)(&posting),
                             (grn_hash *)res,
                             op);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);

exit:
  return rc;
}

static grn_rc
selector_prefix_rk_search_index(
  grn_ctx *ctx, grn_obj *index, grn_obj *query, grn_obj *res, grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *table;

  table = grn_column_table(ctx, index);

  GRN_TABLE_EACH_BEGIN_MIN(ctx,
                           table,
                           cursor,
                           id,
                           GRN_TEXT_VALUE(query),
                           GRN_TEXT_LEN(query),
                           GRN_CURSOR_PREFIX | GRN_CURSOR_RK)
  {
    grn_ii_at(ctx, (grn_ii *)index, id, (grn_hash *)res, op);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);

  return rc;
}

static grn_rc
selector_prefix_rk_search(grn_ctx *ctx,
                          grn_obj *table,
                          grn_obj *index,
                          int nargs,
                          grn_obj **args,
                          grn_obj *res,
                          grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *column;
  grn_obj *query;

  if ((nargs - 1) != 2) {
    ERR(GRN_INVALID_ARGUMENT,
        "prefix_rk_serach(): wrong number of arguments (%d for 2)",
        nargs - 1);
    return ctx->rc;
  }

  column = args[1];
  query = args[2];

  if (grn_obj_is_table(ctx, index)) {
    rc = selector_prefix_rk_search_key(ctx, index, query, res, op);
  } else {
    rc = selector_prefix_rk_search_index(ctx, index, query, res, op);
  }
  return rc;
}

#define DEF_VAR(v, name_str)                                                   \
  do {                                                                         \
    (v).name = (name_str);                                                     \
    (v).name_size = GRN_STRLEN(name_str);                                      \
    GRN_TEXT_INIT(&(v).value, 0);                                              \
  } while (0)

#define DEF_COMMAND(name, func, nvars, vars)                                   \
  (grn_proc_create(ctx,                                                        \
                   (name),                                                     \
                   (sizeof(name) - 1),                                         \
                   GRN_PROC_COMMAND,                                           \
                   (func),                                                     \
                   NULL,                                                       \
                   NULL,                                                       \
                   (nvars),                                                    \
                   (vars)))

void
grn_db_init_builtin_commands(grn_ctx *ctx)
{
  grn_expr_var vars[10];

  grn_proc_init_define_selector(ctx);
  grn_proc_init_select(ctx);

  DEF_VAR(vars[0], "values");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "ifexists");
  DEF_VAR(vars[4], "input_type");
  DEF_VAR(vars[5], "each");
  DEF_VAR(vars[6], "output_ids");
  DEF_VAR(vars[7], "output_errors");
  DEF_VAR(vars[8], "lock_table");
  DEF_COMMAND("load", proc_load, 9, vars);

  DEF_COMMAND("status", proc_status, 0, vars);

  grn_proc_init_table_list(ctx);

  grn_proc_init_column_list(ctx);

  grn_proc_init_table_create(ctx);

  grn_proc_init_table_remove(ctx);

  grn_proc_init_table_rename(ctx);

  grn_proc_init_column_create(ctx);

  grn_proc_init_column_remove(ctx);

  grn_proc_init_column_rename(ctx);

  DEF_VAR(vars[0], "path");
  DEF_COMMAND(GRN_EXPR_MISSING_NAME, proc_missing, 1, vars);

  DEF_COMMAND("quit", proc_quit, 0, vars);

  DEF_VAR(vars[0], "mode");
  DEF_COMMAND("shutdown", proc_shutdown, 1, vars);

  grn_proc_init_clearlock(ctx);
  grn_proc_init_lock_clear(ctx);

  DEF_VAR(vars[0], "target_name");
  DEF_VAR(vars[1], "threshold");
  DEF_COMMAND("defrag", proc_defrag, 2, vars);

  grn_proc_init_log_level(ctx);
  grn_proc_init_log_put(ctx);
  grn_proc_init_log_reopen(ctx);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "id");
  DEF_VAR(vars[3], "filter");
  DEF_COMMAND("delete", proc_delete, 4, vars);

  DEF_VAR(vars[0], "max");
  DEF_COMMAND("cache_limit", proc_cache_limit, 1, vars);

  grn_proc_init_dump(ctx);

  /* Deprecated. Use "plugin_register" instead. */
  DEF_VAR(vars[0], "path");
  DEF_COMMAND("register", proc_register, 1, vars);

  DEF_VAR(vars[0], "obj");
  DEF_COMMAND("check", proc_check, 1, vars);

  DEF_VAR(vars[0], "target_name");
  DEF_VAR(vars[1], "table");
  DEF_COMMAND("truncate", proc_truncate, 2, vars);

  grn_proc_init_normalize(ctx);

  grn_proc_init_tokenize(ctx);
  grn_proc_init_table_tokenize(ctx);

  DEF_COMMAND("tokenizer_list", proc_tokenizer_list, 0, vars);

  DEF_COMMAND("normalizer_list", proc_normalizer_list, 0, vars);

  {
    grn_obj *proc;
    proc = grn_proc_create(ctx,
                           "rand",
                           -1,
                           GRN_PROC_FUNCTION,
                           func_rand,
                           NULL,
                           NULL,
                           0,
                           NULL);
    grn_proc_set_is_stable(ctx, proc, false);
  }

  {
    grn_obj *proc;
    proc = grn_proc_create(ctx,
                           "now",
                           -1,
                           GRN_PROC_FUNCTION,
                           func_now,
                           NULL,
                           NULL,
                           0,
                           NULL);
    grn_proc_set_is_stable(ctx, proc, false);
  }

  grn_proc_create(ctx,
                  "max",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_max,
                  NULL,
                  NULL,
                  0,
                  NULL);
  grn_proc_create(ctx,
                  "min",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_min,
                  NULL,
                  NULL,
                  0,
                  NULL);

  {
    grn_obj *selector_proc;

    selector_proc = grn_proc_create(ctx,
                                    "geo_in_circle",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_geo_in_circle,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, grn_selector_geo_in_circle);
    /* We may need GRN_OP_GEO_IN_CIRCLE. */
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_MATCH);

    selector_proc = grn_proc_create(ctx,
                                    "geo_in_rectangle",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_geo_in_rectangle,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, grn_selector_geo_in_rectangle);
    /* We may need GRN_OP_GEO_IN_RECTANGLE. */
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_MATCH);
  }

  grn_proc_create(ctx,
                  "geo_distance",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_geo_distance,
                  NULL,
                  NULL,
                  0,
                  NULL);

  /* deprecated. */
  grn_proc_create(ctx,
                  "geo_distance2",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_geo_distance2,
                  NULL,
                  NULL,
                  0,
                  NULL);

  /* deprecated. */
  grn_proc_create(ctx,
                  "geo_distance3",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_geo_distance3,
                  NULL,
                  NULL,
                  0,
                  NULL);

  grn_proc_init_edit_distance(ctx);

  {
    grn_obj *selector_proc;

    selector_proc = grn_proc_create(ctx,
                                    "all_records",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_all_records,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, selector_all_records);
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
  }

  /* experimental */
  grn_proc_init_snippet_html(ctx);

  grn_proc_init_query(ctx);

  {
    grn_obj *selector_proc;

    selector_proc = grn_proc_create(ctx,
                                    "sub_filter",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    NULL,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, selector_sub_filter);
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
  }

  grn_proc_create(ctx,
                  "html_untag",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_html_untag,
                  NULL,
                  NULL,
                  0,
                  NULL);

  {
    grn_obj *selector_proc;

    selector_proc = grn_proc_create(ctx,
                                    "between",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_between,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, selector_between);
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_LESS);
  }

  grn_proc_init_highlight_html(ctx);
  grn_proc_init_highlight_full(ctx);

  {
    grn_obj *selector_proc;

    selector_proc = grn_proc_create(ctx,
                                    "in_values",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_in_values,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, selector_in_values);
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_EQUAL);
  }

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "column");
  DEF_VAR(vars[2], "min");
  DEF_VAR(vars[3], "min_border");
  DEF_VAR(vars[4], "max");
  DEF_VAR(vars[5], "max_border");
  DEF_VAR(vars[6], "offset");
  DEF_VAR(vars[7], "limit");
  DEF_VAR(vars[8], "filter");
  DEF_VAR(vars[9], "output_columns");
  DEF_COMMAND("range_filter", proc_range_filter, 10, vars);

  DEF_VAR(vars[0], "id");
  DEF_COMMAND("request_cancel", proc_request_cancel, 1, vars);

  DEF_VAR(vars[0], "name");
  DEF_COMMAND("plugin_register", proc_plugin_register, 1, vars);

  DEF_VAR(vars[0], "name");
  DEF_COMMAND("plugin_unregister", proc_plugin_unregister, 1, vars);

  DEF_VAR(vars[0], "target_name");
  DEF_VAR(vars[1], "recursive");
  DEF_VAR(vars[2], "only_opened");
  DEF_COMMAND("io_flush", proc_io_flush, 3, vars);

  grn_proc_init_object_exist(ctx);

  grn_proc_init_thread_limit(ctx);

  DEF_COMMAND("database_unmap", proc_database_unmap, 0, vars);

  grn_proc_init_column_copy(ctx);

  grn_proc_init_schema(ctx);

  DEF_VAR(vars[0], "target_name");
  DEF_COMMAND("reindex", proc_reindex, 1, vars);

  {
    grn_obj *selector_proc;

    selector_proc = grn_proc_create(ctx,
                                    "prefix_rk_search",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    NULL,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_selector(ctx, selector_proc, selector_prefix_rk_search);
    grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_PREFIX);
  }

  grn_proc_init_config_get(ctx);
  grn_proc_init_config_set(ctx);
  grn_proc_init_config_delete(ctx);

  grn_proc_init_lock_acquire(ctx);
  grn_proc_init_lock_release(ctx);

  grn_proc_init_object_inspect(ctx);

  grn_proc_init_fuzzy_search(ctx);

  grn_proc_init_object_remove(ctx);

  grn_proc_init_snippet(ctx);
  grn_proc_init_highlight(ctx);

  grn_proc_init_query_expand(ctx);

  grn_proc_init_object_list(ctx);

  grn_proc_init_table_copy(ctx);

  grn_proc_init_in_records(ctx);

  grn_proc_init_query_log_flags_get(ctx);
  grn_proc_init_query_log_flags_set(ctx);
  grn_proc_init_query_log_flags_add(ctx);
  grn_proc_init_query_log_flags_remove(ctx);

  grn_proc_init_cast_loose(ctx);

  grn_proc_init_index_column_diff(ctx);

  grn_proc_init_object_set_visibility(ctx);

  grn_proc_init_reference_acquire(ctx);
  grn_proc_init_reference_release(ctx);

  grn_proc_init_query_parallel_or(ctx);

  grn_proc_init_object_warm(ctx);

  grn_proc_init_table_create_similar(ctx);

  grn_proc_init_column_create_similar(ctx);

  grn_proc_init_thread_dump(ctx);

  grn_proc_init_escalate(ctx);

  grn_proc_init_sleep(ctx);

  grn_proc_init_distance_cosine(ctx);
  grn_proc_init_distance_inner_product(ctx);
  grn_proc_init_distance_l1_norm(ctx);
  grn_proc_init_distance_l2_norm_squared(ctx);
}
