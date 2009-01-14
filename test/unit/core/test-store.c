/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/sen-assertions.h"

void test_create_simple(void);
void test_create_with_many_flags(void);
void test_create_with_encoding_utf8(void);
void test_create_with_encoding_euc_jp(void);

static sen_logger_info *logger;
static GList *error_messages;

static sen_db *db;
static gchar *base_dir;
static gchar *default_path;
static int default_flags;
static sen_encoding default_encoding;

void
setup(void)
{
  logger = setup_sen_logger();
  error_messages = NULL;

  db = NULL;
  base_dir = g_build_filename(sen_test_get_base_dir(), "tmp", NULL);
  default_path = g_build_filename(base_dir, "store", NULL);
  default_flags = 0;
  default_encoding = sen_enc_default;

  cut_remove_path(base_dir, NULL);
  g_mkdir_with_parents(base_dir, 0755);
}

void
clear_error_messages(void)
{
  if (error_messages) {
    gcut_list_string_free(error_messages);
    error_messages = NULL;
  }
}

void
teardown(void)
{
  if (db) {
    sen_db_close(db);
  }

  if (default_path) {
    g_free(default_path);
  }

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
    g_free(base_dir);
  }

  clear_error_messages();
  teardown_sen_logger(logger);
}

#define clear_messages()                        \
  sen_collect_logger_clear_messages(logger)

#define messages()                              \
  sen_collect_logger_get_messages(logger)

#define create_db()                                                     \
  db = sen_db_create(default_path, default_flags, default_encoding)

#define cut_assert_create_db() do               \
{                                               \
  create_db();                                  \
  cut_assert(db);                               \
} while (0)

#define cut_assert_fail_create_db(error_message, ...) do        \
{                                                               \
  clear_messages();                                             \
  create_db();                                                  \
  clear_error_messages();                                       \
  error_messages = gcut_list_string_new(error_message,          \
                                        ## __VA_ARGS__,         \
                                        NULL);                  \
  cut_assert_equal_g_list_string(error_messages, messages());   \
  cut_assert_null(db);                                          \
} while (0)

void
test_create_simple(void)
{
  cut_assert_create_db();
}

void
test_create_with_long_path(void)
{
  gssize max_path = PATH_MAX - 14;
  GString *long_path;
  const gchar last_component[] = G_DIR_SEPARATOR_S "db";

  long_path = sen_long_path_new(default_path,
                                max_path - strlen(last_component));
  g_free(default_path);

  g_mkdir_with_parents(long_path->str, 0700);
  g_string_append(long_path, last_component);

  default_path = g_string_free(long_path, FALSE);
  cut_assert_create_db();
  sen_db_close(db);
  db = NULL;

  long_path = g_string_new(default_path);
  g_free(default_path);

  g_string_append(long_path, "X");
  default_path = g_string_free(long_path, FALSE);
  cut_assert_fail_create_db("too long path");
}

void
test_create_with_many_flags(void)
{
  default_flags = SEN_INDEX_NORMALIZE | SEN_INDEX_SPLIT_ALPHA |
    SEN_INDEX_SPLIT_DIGIT | SEN_INDEX_SPLIT_SYMBOL |
    SEN_INDEX_NGRAM | SEN_INDEX_DELIMITED;
  cut_assert_create_db();
}

void
test_create_with_encoding_utf8(void)
{
  default_encoding = sen_enc_utf8;
  cut_assert_create_db();
}

void
test_create_with_encoding_euc_jp(void)
{
  default_encoding = sen_enc_euc_jp;
  cut_assert_create_db();
}
