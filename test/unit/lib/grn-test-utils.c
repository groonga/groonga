/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sen-test-utils.h"

const gchar *
sen_rc_to_string(sen_rc rc)
{
  switch (rc) {
  case sen_success:
    return "sen_success";
  case sen_memory_exhausted:
    return "sen_memory_exhausted";
  case sen_invalid_format:
    return "sen_invalid_format";
  case sen_file_operation_error:
    return "sen_file_operation_error";
  case sen_invalid_argument:
    return "sen_invalid_argument";
  case sen_other_error:
    return "sen_other_error";
  case sen_external_error:
    return "sen_external_error";
  case sen_internal_error:
    return "sen_internal_error";
  case sen_abnormal_error:
    return "sen_abnormal_error";
  case sen_end_of_data:
    return "sen_end_of_data";
  default:
    return "sen_unknown_status";
  }
}

static gchar *base_dir = NULL;
const gchar *
sen_test_get_base_dir(void)
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

typedef struct _sen_log
{
  gint level;
  gchar *time;
  gchar *title;
  gchar *message;
  gchar *location;
} sen_log;

static sen_log *
sen_log_new(gint level, const gchar *time, const gchar *title,
            const gchar *message, const gchar *location)
{
  sen_log *log;

  log = g_new0(sen_log, 1);
  log->level = level;
  log->time = g_strdup(time);
  log->title = g_strdup(title);
  log->message = g_strdup(message);
  log->location = g_strdup(location);

  return log;
}

static void
sen_log_free(sen_log *log)
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

typedef struct _sen_logger_context
{
  sen_logger_info *logger;
  GList *logs;
  GList *messages;
} sen_logger_context;

static sen_logger_context *
sen_logger_context_new(sen_logger_info *logger)
{
  sen_logger_context *context;

  context = g_new0(sen_logger_context, 1);
  context->logger = logger;
  context->logs = NULL;
  context->messages = NULL;

  return context;
}

static void
sen_logger_context_clear_messages(sen_logger_context *context)
{
  g_list_foreach(context->messages, (GFunc)g_free, NULL);
  g_list_free(context->messages);
  context->messages = NULL;
}

static void
sen_logger_context_free(sen_logger_context *context)
{
  if (!context) {
    return;
  }

  g_list_foreach(context->logs, (GFunc)sen_log_free, NULL);
  g_list_free(context->logs);

  sen_logger_context_clear_messages(context);

  g_free(context);
}

static void
sen_collect_logger_log_func(int level, const char *time, const char *title,
                            const char *message, const char *location,
                            void *func_arg)
{
  sen_logger_context *context = func_arg;
  sen_log *log;

  log = sen_log_new(level, time, title, message, location);
  context->logs = g_list_prepend(context->logs, log);
  context->messages = g_list_prepend(context->messages, g_strdup(message));
}

sen_logger_info *
sen_collect_logger_new(void)
{
  sen_logger_info *logger;

  logger = g_new(sen_logger_info, 1);
  logger->max_level = sen_log_dump;
  logger->flags = SEN_LOG_TIME | SEN_LOG_MESSAGE | SEN_LOG_LOCATION;
  logger->func = sen_collect_logger_log_func;
  logger->func_arg = sen_logger_context_new(logger);

  return logger;
}

void
sen_collect_logger_clear_messages(sen_logger_info *logger)
{
  sen_logger_context *context = logger->func_arg;

  sen_logger_context_clear_messages(context);
}

const GList *
sen_collect_logger_get_messages(sen_logger_info *logger)
{
  sen_logger_context *context = logger->func_arg;

  return context->messages;
}

void
sen_collect_logger_free(sen_logger_info *logger)
{
  if (!logger) {
    return;
  }

  sen_logger_context_free(logger->func_arg);
  g_free(logger);
}


sen_logger_info *
setup_sen_logger(void)
{
  sen_logger_info *logger;

  logger = sen_collect_logger_new();
  sen_logger_info_set(logger);
  return logger;
}

void
teardown_sen_logger(sen_logger_info *logger)
{
  sen_logger_info_set(NULL);
  if (logger) {
    sen_collect_logger_free(logger);
  }
}

GString *
sen_long_path_new(const gchar *base_path, gssize max_size)
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
sen_test_pat_cursor_get_keys(sen_ctx *context, sen_table_cursor *cursor)
{
  GList *keys = NULL;
  sen_id id;

  id = sen_table_cursor_next(context, cursor);
  while (id != SEN_ID_NIL) {
    void *key;
    GString *null_terminated_key;
    int size;

    size = sen_table_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_new_len(key, size);
    keys = g_list_append(keys, g_string_free(null_terminated_key, FALSE));
    id = sen_table_cursor_next(context, cursor);
  }

  return keys;
}

GList *
sen_test_pat_get_keys(sen_ctx *context, sen_obj *patricia_trie)
{
  GList *keys;
  sen_table_cursor *cursor;

  cursor = sen_table_cursor_open(context, patricia_trie,
                                 NULL, 0, NULL, 0, SEN_CURSOR_ASCENDING);
  keys = sen_test_pat_cursor_get_keys(context, cursor);
  sen_table_cursor_close(context, cursor);

  return keys;
}

GList *
sen_test_pat_cursor_get_pairs(sen_ctx *context, sen_table_cursor *cursor)
{
  sen_id id;
  GList *pairs = NULL;

  id = sen_table_cursor_next(context, cursor);
  while (id != SEN_ID_NIL) {
    int length;
    void *key, *value;
    GString *null_terminated_key, *null_terminated_value;

    length = sen_table_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_new_len(key, length);
    pairs = g_list_append(pairs,
                          g_string_free(null_terminated_key, FALSE));

    length = sen_table_cursor_get_value(context, cursor, &value);
    null_terminated_value = g_string_new_len(value, length);
    pairs = g_list_append(pairs,
                          g_string_free(null_terminated_value, FALSE));

    id = sen_table_cursor_next(context, cursor);
  }

  return pairs;
}

GHashTable *
sen_test_pat_get_pairs(sen_ctx *context, sen_obj *patricia_trie)
{
  GList *node, *ordered_pairs;
  GHashTable *pairs;
  sen_table_cursor *cursor;

  cursor = sen_table_cursor_open(context, patricia_trie,
                                 NULL, 0, NULL, 0, SEN_CURSOR_ASCENDING);
  ordered_pairs = sen_test_pat_cursor_get_pairs(context, cursor);
  sen_table_cursor_close(context, cursor);

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
