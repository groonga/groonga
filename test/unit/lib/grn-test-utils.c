/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "grn-test-utils.h"

const gchar *
grn_rc_to_string(grn_rc rc)
{
  switch (rc) {
  case GRN_SUCCESS:
    return "GRN_SUCCESS";
  case GRN_END_OF_DATA:
    return "GRN_END_OF_DATA";
  case GRN_UNKNOWN_ERROR:
    return "GRN_UNKNOWN_ERROR";
  case GRN_OPERATION_NOT_PERMITTED:
    return "GRN_OPERATION_NOT_PERMITTED";
  case GRN_NO_SUCH_FILE_OR_DIRECTORY:
    return "GRN_NO_SUCH_FILE_OR_DIRECTORY";
  case GRN_NO_SUCH_PROCESS:
    return "GRN_NO_SUCH_PROCESS";
  case GRN_INTERRUPTED_FUNCTION_CALL:
    return "GRN_INTERRUPTED_FUNCTION_CALL";
  case GRN_INPUT_OUTPUT_ERROR:
    return "GRN_INPUT_OUTPUT_ERROR";
  case GRN_NO_SUCH_DEVICE_OR_ADDRESS:
    return "GRN_NO_SUCH_DEVICE_OR_ADDRESS";
  case GRN_ARG_LIST_TOO_LONG:
    return "GRN_ARG_LIST_TOO_LONG";
  case GRN_EXEC_FORMAT_ERROR:
    return "GRN_EXEC_FORMAT_ERROR";
  case GRN_BAD_FILE_DESCRIPTOR:
    return "GRN_BAD_FILE_DESCRIPTOR";
  case GRN_NO_CHILD_PROCESSES:
    return "GRN_NO_CHILD_PROCESSES";
  case GRN_RESOURCE_TEMPORARILY_UNAVAILABLE:
    return "GRN_RESOURCE_TEMPORARILY_UNAVAILABLE";
  case GRN_NOT_ENOUGH_SPACE:
    return "GRN_NOT_ENOUGH_SPACE";
  case GRN_PERMISSION_DENIED:
    return "GRN_PERMISSION_DENIED";
  case GRN_BAD_ADDRESS:
    return "GRN_BAD_ADDRESS";
  case GRN_RESOURCE_BUSY:
    return "GRN_RESOURCE_BUSY";
  case GRN_FILE_EXISTS:
    return "GRN_FILE_EXISTS";
  case GRN_IMPROPER_LINK:
    return "GRN_IMPROPER_LINK";
  case GRN_NO_SUCH_DEVICE:
    return "GRN_NO_SUCH_DEVICE";
  case GRN_NOT_A_DIRECTORY:
    return "GRN_NOT_A_DIRECTORY";
  case GRN_IS_A_DIRECTORY:
    return "GRN_IS_A_DIRECTORY";
  case GRN_INVALID_ARGUMENT:
    return "GRN_INVALID_ARGUMENT";
  case GRN_TOO_MANY_OPEN_FILES_IN_SYSTEM:
    return "GRN_TOO_MANY_OPEN_FILES_IN_SYSTEM";
  case GRN_TOO_MANY_OPEN_FILES:
    return "GRN_TOO_MANY_OPEN_FILES";
  case GRN_INAPPROPRIATE_I_O_CONTROL_OPERATION:
    return "GRN_INAPPROPRIATE_I_O_CONTROL_OPERATION";
  case GRN_FILE_TOO_LARGE:
    return "GRN_FILE_TOO_LARGE";
  case GRN_NO_SPACE_LEFT_ON_DEVICE:
    return "GRN_NO_SPACE_LEFT_ON_DEVICE";
  case GRN_INVALID_SEEK:
    return "GRN_INVALID_SEEK";
  case GRN_READ_ONLY_FILE_SYSTEM:
    return "GRN_READ_ONLY_FILE_SYSTEM";
  case GRN_TOO_MANY_LINKS:
    return "GRN_TOO_MANY_LINKS";
  case GRN_BROKEN_PIPE:
    return "GRN_BROKEN_PIPE";
  case GRN_DOMAIN_ERROR:
    return "GRN_DOMAIN_ERROR";
  case GRN_RESULT_TOO_LARGE:
    return "GRN_RESULT_TOO_LARGE";
  case GRN_RESOURCE_DEADLOCK_AVOIDED:
    return "GRN_RESOURCE_DEADLOCK_AVOIDED";
  case GRN_NO_MEMORY_AVAILABLE:
    return "GRN_NO_MEMORY_AVAILABLE";
  case GRN_FILENAME_TOO_LONG:
    return "GRN_FILENAME_TOO_LONG";
  case GRN_NO_LOCKS_AVAILABLE:
    return "GRN_NO_LOCKS_AVAILABLE";
  case GRN_FUNCTION_NOT_IMPLEMENTED:
    return "GRN_FUNCTION_NOT_IMPLEMENTED";
  case GRN_DIRECTORY_NOT_EMPTY:
    return "GRN_DIRECTORY_NOT_EMPTY";
  case GRN_ILLEGAL_BYTE_SEQUENCE:
    return "GRN_ILLEGAL_BYTE_SEQUENCE";
  case GRN_SOCKET_NOT_INITIALIZED:
    return "GRN_SOCKET_NOT_INITIALIZED";
  case GRN_OPERATION_WOULD_BLOCK:
    return "GRN_OPERATION_WOULD_BLOCK";
  case GRN_ADDRESS_IS_NOT_AVAILABLE:
    return "GRN_ADDRESS_IS_NOT_AVAILABLE";
  case GRN_NETWORK_IS_DOWN:
    return "GRN_NETWORK_IS_DOWN";
  case GRN_NO_BUFFER:
    return "GRN_NO_BUFFER";
  case GRN_SOCKET_IS_ALREADY_CONNECTED:
    return "GRN_SOCKET_IS_ALREADY_CONNECTED";
  case GRN_SOCKET_IS_NOT_CONNECTED:
    return "GRN_SOCKET_IS_NOT_CONNECTED";
  case GRN_SOCKET_IS_ALREADY_SHUTDOWNED:
    return "GRN_SOCKET_IS_ALREADY_SHUTDOWNED";
  case GRN_OPERATION_TIMEOUT:
    return "GRN_OPERATION_TIMEOUT";
  case GRN_CONNECTION_REFUSED:
    return "GRN_CONNECTION_REFUSED";
  case GRN_RANGE_ERROR:
    return "GRN_RANGE_ERROR";
  case GRN_TOKENIZER_ERROR:
    return "GRN_TOKENIZER_ERROR";
  case GRN_FILE_CORRUPT:
    return "GRN_FILE_CORRUPT";
  case GRN_INVALID_FORMAT:
    return "GRN_INVALID_FORMAT";
  case GRN_OBJECT_CORRUPT:
    return "GRN_OBJECT_CORRUPT";
  case GRN_TOO_MANY_SYMBOLIC_LINKS:
    return "GRN_TOO_MANY_SYMBOLIC_LINKS";
  case GRN_NOT_SOCKET:
    return "GRN_NOT_SOCKET";
  case GRN_OPERATION_NOT_SUPPORTED:
    return "GRN_OPERATION_NOT_SUPPORTED";
  case GRN_ADDRESS_IS_IN_USE:
    return "GRN_ADDRESS_IS_IN_USE";
  case GRN_ZLIB_ERROR:
    return "GRN_ZLIB_ERROR";
  case GRN_LZO_ERROR:
    return "GRN_LZO_ERROR";
  case GRN_STACK_OVER_FLOW:
    return "GRN_STACK_OVER_FLOW";
  case GRN_SYNTAX_ERROR:
    return "GRN_SYNTAX_ERROR";
  case GRN_RETRY_MAX:
    return "GRN_RETRY_MAX";
  case GRN_INCOMPATIBLE_FILE_FORMAT:
    return "GRN_INCOMPATIBLE_FILE_FORMAT";
  default:
    return "GRN_UNKNOWN_STATUS";
  }
}

static gchar *base_dir = NULL;
const gchar *
grn_test_get_base_dir(void)
{
  const gchar *dir;

  if (base_dir)
    return base_dir;

  dir = g_getenv("BASE_DIR");
  if (!dir)
    dir = ".";

  if (g_path_is_absolute(dir)) {
    base_dir = g_strdup(dir);
  } else {
    gchar *current_dir;

    current_dir = g_get_current_dir();
    base_dir = g_build_filename(current_dir, dir, NULL);
    g_free(current_dir);
  }

  return base_dir;
}

typedef struct _grn_log
{
  gint level;
  gchar *time;
  gchar *title;
  gchar *message;
  gchar *location;
} grn_log;

static grn_log *
grn_log_new(gint level, const gchar *time, const gchar *title,
            const gchar *message, const gchar *location)
{
  grn_log *log;

  log = g_new0(grn_log, 1);
  log->level = level;
  log->time = g_strdup(time);
  log->title = g_strdup(title);
  log->message = g_strdup(message);
  log->location = g_strdup(location);

  return log;
}

static void
grn_log_free(grn_log *log)
{
  if (!log) {
    return;
  }

  g_free(log->time);
  g_free(log->title);
  g_free(log->message);
  g_free(log->location);

  g_free(log);
}

typedef struct _grn_logger_context
{
  grn_logger_info *logger;
  GList *logs;
  GList *messages;
} grn_logger_context;

static grn_logger_context *
grn_logger_context_new(grn_logger_info *logger)
{
  grn_logger_context *context;

  context = g_new0(grn_logger_context, 1);
  context->logger = logger;
  context->logs = NULL;
  context->messages = NULL;

  return context;
}

static void
grn_logger_context_clear_messages(grn_logger_context *context)
{
  g_list_foreach(context->messages, (GFunc)g_free, NULL);
  g_list_free(context->messages);
  context->messages = NULL;
}

static void
grn_logger_context_free(grn_logger_context *context)
{
  if (!context) {
    return;
  }

  g_list_foreach(context->logs, (GFunc)grn_log_free, NULL);
  g_list_free(context->logs);

  grn_logger_context_clear_messages(context);

  g_free(context);
}

static void
grn_collect_logger_log_func(int level, const char *time, const char *title,
                            const char *message, const char *location,
                            void *func_arg)
{
  grn_logger_context *context = func_arg;
  grn_log *log;

  log = grn_log_new(level, time, title, message, location);
  context->logs = g_list_prepend(context->logs, log);
  context->messages = g_list_prepend(context->messages, g_strdup(message));
}

grn_logger_info *
grn_collect_logger_new(void)
{
  grn_logger_info *logger;

  logger = g_new(grn_logger_info, 1);
  logger->max_level = GRN_LOG_DUMP;
  logger->flags = GRN_LOG_TIME | GRN_LOG_MESSAGE | GRN_LOG_LOCATION;
  logger->func = grn_collect_logger_log_func;
  logger->func_arg = grn_logger_context_new(logger);

  return logger;
}

void
grn_collect_logger_clear_messages(grn_logger_info *logger)
{
  grn_logger_context *context = logger->func_arg;

  grn_logger_context_clear_messages(context);
}

const GList *
grn_collect_logger_get_messages(grn_logger_info *logger)
{
  grn_logger_context *context = logger->func_arg;

  return context->messages;
}

gchar *
grn_collect_logger_to_string(grn_logger_info *logger)
{
  GString *string;
  const GList *messages;

  string = g_string_new(NULL);
  for (messages = grn_collect_logger_get_messages(logger);
       messages;
       messages = g_list_next(messages)) {
    const gchar *message = messages->data;
    g_string_append_printf(string, "%s\n", message);
  }

  return g_string_free(string, FALSE);
}

void
grn_collect_logger_print_messages(grn_logger_info *logger)
{
  const GList *messages;

  for (messages = grn_collect_logger_get_messages(logger);
       messages;
       messages = g_list_next(messages)) {
    const gchar *message = messages->data;
    g_print("%s\n", message);
  }
}

void
grn_collect_logger_free(grn_logger_info *logger)
{
  if (!logger) {
    return;
  }

  grn_logger_context_free(logger->func_arg);
  g_free(logger);
}


grn_logger_info *
setup_grn_logger(void)
{
  grn_logger_info *logger;

  logger = grn_collect_logger_new();
  grn_logger_info_set(NULL, logger);
  return logger;
}

void
teardown_grn_logger(grn_logger_info *logger)
{
  grn_logger_info_set(NULL, NULL);
  if (logger) {
    grn_collect_logger_free(logger);
  }
}

GString *
grn_long_path_new(const gchar *base_path, gssize max_size)
{
  GString *long_path;

  long_path = g_string_new(base_path);
  while (long_path->len < max_size) {
    g_string_append(long_path, G_DIR_SEPARATOR_S "XXXXXXXXXX");
  }
  g_string_set_size(long_path, max_size);

  return long_path;
}

GList *
grn_test_pat_cursor_get_keys(grn_ctx *context, grn_table_cursor *cursor)
{
  GList *keys = NULL;
  grn_id id;

  id = grn_table_cursor_next(context, cursor);
  while (id != GRN_ID_NIL) {
    void *key;
    GString *null_terminated_key;
    int size;

    size = grn_table_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_new_len(key, size);
    keys = g_list_append(keys, g_string_free(null_terminated_key, FALSE));
    id = grn_table_cursor_next(context, cursor);
  }

  return keys;
}

GList *
grn_test_pat_get_keys(grn_ctx *context, grn_obj *patricia_trie)
{
  GList *keys;
  grn_table_cursor *cursor;

  cursor = grn_table_cursor_open(context, patricia_trie,
                                 NULL, 0, NULL, 0, 0, 0, GRN_CURSOR_ASCENDING);
  keys = grn_test_pat_cursor_get_keys(context, cursor);
  grn_table_cursor_close(context, cursor);

  return keys;
}

GList *
grn_test_pat_cursor_get_pairs(grn_ctx *context, grn_table_cursor *cursor)
{
  grn_id id;
  GList *pairs = NULL;

  id = grn_table_cursor_next(context, cursor);
  while (id != GRN_ID_NIL) {
    int length;
    void *key, *value;
    GString *null_terminated_key, *null_terminated_value;

    length = grn_table_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_new_len(key, length);
    pairs = g_list_append(pairs,
                          g_string_free(null_terminated_key, FALSE));

    length = grn_table_cursor_get_value(context, cursor, &value);
    null_terminated_value = g_string_new_len(value, length);
    pairs = g_list_append(pairs,
                          g_string_free(null_terminated_value, FALSE));

    id = grn_table_cursor_next(context, cursor);
  }

  return pairs;
}

GHashTable *
grn_test_pat_get_pairs(grn_ctx *context, grn_obj *patricia_trie)
{
  GList *node, *ordered_pairs;
  GHashTable *pairs;
  grn_table_cursor *cursor;

  cursor = grn_table_cursor_open(context, patricia_trie,
                                 NULL, 0, NULL, 0, 0, 0, GRN_CURSOR_ASCENDING);
  ordered_pairs = grn_test_pat_cursor_get_pairs(context, cursor);
  grn_table_cursor_close(context, cursor);

  pairs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  for (node = ordered_pairs; node; node = g_list_next(node)) {
    gchar *key, *value;

    key = node->data;
    node = g_list_next(node);
    if (!node)
      break;
    value = node->data;

    g_hash_table_insert(pairs, key, value);
  }
  g_list_free(ordered_pairs);

  return pairs;
}

const gchar *
grn_test_type_inspect (grn_ctx *context, unsigned char type)
{
  switch (type) {
  case GRN_VOID:
    return "void";
  case GRN_BULK:
    return "bulk";
  case GRN_PTR:
    return "ptr";
  case GRN_UVECTOR:
    return "uvector";
  case GRN_PVECTOR:
    return "pvector";
  case GRN_MSG:
    return "msg";
  case GRN_QUERY:
    return "query";
  case GRN_ACCESSOR:
    return "accessor";
  case GRN_SNIP:
    return "snip";
  case GRN_PATSNIP:
    return "patsnip";
  case GRN_CURSOR_TABLE_HASH_KEY:
    return "cursor-table-hash-key";
  case GRN_CURSOR_TABLE_PAT_KEY:
    return "cursor-table-pat-key";
  case GRN_CURSOR_TABLE_NO_KEY:
    return "cursor-table-no-key";
  case GRN_CURSOR_COLUMN_INDEX:
    return "cursor-column-index";
  case GRN_TYPE:
    return "type";
  case GRN_PROC:
    return "proc";
  case GRN_EXPR:
    return "expr";
  case GRN_TABLE_HASH_KEY:
    return "table-hash-key";
  case GRN_TABLE_PAT_KEY:
    return "table-pat-key";
  case GRN_TABLE_NO_KEY:
    return "table-no-key";
  case GRN_DB:
    return "db";
  case GRN_COLUMN_FIX_SIZE:
    return "column-fix-size";
  case GRN_COLUMN_VAR_SIZE:
    return "column-var-size";
  case GRN_COLUMN_INDEX:
    return "column-index";
  default:
    return "unknown";
  }
}

void
grn_test_object_inspect (GString *output, grn_ctx *context, grn_obj *object)
{
  grn_id domain;

  if (!object) {
    g_string_append(output, "<NULL>");
    return;
  }

  g_string_append(output, "#<");
  g_string_append_printf(output, "%s",
                         grn_test_type_inspect(context, object->header.type));
  g_string_append_printf(output, ":%p ", object);
  g_string_append_printf(output, "flags: 0x%x, ", object->header.flags);

  g_string_append(output, "domain: ");
  domain = object->header.domain;
  if (domain == GRN_ID_NIL) {
    g_string_append(output, "<nil>");
  } else {
    grn_obj *domain_object = NULL;

    if (context)
      domain_object = grn_ctx_at(context, domain);
    if (domain_object)
      grn_test_object_inspect(output, context, domain_object);
    else
      g_string_append_printf(output, "%u", domain);
  }

  g_string_append(output, ">");
}
