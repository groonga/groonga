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

#include <store.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_create_simple(void);
void test_create_with_many_flags(void);
void test_create_with_encoding_utf8(void);
void test_create_with_encoding_euc_jp(void);

static grn_logger_info *logger;
static GList *error_messages;

static grn_ctx *context;
static grn_obj *db;
static gchar *base_dir;
static gchar *default_path;
static grn_db_create_optarg options;

void
cut_setup(void)
{
  logger = setup_grn_logger();
  error_messages = NULL;

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, GRN_CTX_USE_QL);
  db = NULL;
  base_dir = g_build_filename(grn_test_get_base_dir(), "tmp", NULL);
  default_path = g_build_filename(base_dir, "store", NULL);
  options.encoding = GRN_ENC_DEFAULT;
  options.

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
cut_teardown(void)
{
  if (db) {
    grn_db_close(context, db);
  }

  if (default_path) {
    g_free(default_path);
  }

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
    g_free(base_dir);
  }

  if (context) {
    grn_ctx_fin(context);
  }

  clear_error_messages();
  teardown_grn_logger(logger);
}

#define clear_messages()                        \
  grn_collect_logger_clear_messages(logger)

#define messages()                              \
  grn_collect_logger_get_messages(logger)

#define create_db()                                                     \
  db = grn_db_create(context, default_path, default_flags, default_encoding)

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

  long_path = grn_long_path_new(default_path,
                                max_path - strlen(last_component));
  g_free(default_path);

  g_mkdir_with_parents(long_path->str, 0700);
  g_string_append(long_path, last_component);

  default_path = g_string_free(long_path, FALSE);
  cut_assert_create_db();
  grn_db_close(db);
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
  default_flags = GRN_INDEX_NORMALIZE | GRN_INDEX_SPLIT_ALPHA |
    GRN_INDEX_SPLIT_DIGIT | GRN_INDEX_SPLIT_SYMBOL |
    GRN_INDEX_NGRAM | GRN_INDEX_DELIMITED;
  cut_assert_create_db();
}

void
test_create_with_encoding_utf8(void)
{
  default_encoding = grn_enc_utf8;
  cut_assert_create_db();
}

void
test_create_with_encoding_euc_jp(void)
{
  default_encoding = grn_enc_euc_jp;
  cut_assert_create_db();
}
