/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>

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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <str.h>

void test_invalid_char(void);

static grn_logger_info *logger;
static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "log",
                                   NULL);
}

void
cut_shutdown(void)
{
  g_free(tmp_directory);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

void
cut_setup(void)
{
  const gchar *database_path;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  logger = setup_grn_logger();

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  teardown_grn_logger(logger);

  remove_tmp_directory();
}

void
test_invalid_char(void)
{
  GList *log = NULL;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("column_create Users desc COLUMN_SCALAR ShortText");
  grn_collect_logger_clear_messages(logger);
  assert_send_command("load --table Users --input_type json\n"
                      "{\"name\": \"groonga\" @ \"desc\" \"search engine\"}\n"
                      "");
  log = (GList *)grn_collect_logger_get_messages(logger);
  cut_assert_equal_string("ignored invalid char('@') at",
                          g_list_nth_data(log, 0));
  cut_assert_equal_string("{\"name\": \"groonga\" @",
                          g_list_nth_data(log, 1));
  cut_assert_equal_string("                   ^",
                          g_list_nth_data(log, 2));
}

void
test_no_key(void)
{
  GList *log = NULL;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users desc COLUMN_SCALAR ShortText");
  grn_collect_logger_clear_messages(logger);
  assert_send_command("load --table Users --input_type json\n"
                      "{\"info\" \"search engine\"}\n"
                      "");
  log = (GList *)grn_collect_logger_get_messages(logger);
  cut_assert_equal_string("neither _key nor _id is assigned",
                          g_list_nth_data(log, 0));
}

void
test_duplicated_key(void)
{
  GList *log = NULL;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users desc COLUMN_SCALAR ShortText");
  grn_collect_logger_clear_messages(logger);
  assert_send_command("load --table Users --input_type json\n"
                      "{\"_key\": \"groonga\", \"_id\": 1}\n"
                      "");
  log = (GList *)grn_collect_logger_get_messages(logger);
  cut_assert_equal_string("duplicated key columns: _key and _id",
                          g_list_nth_data(log, 0));
}

void
test_invalid_column(void)
{
  GList *log = NULL;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users desc COLUMN_SCALAR ShortText");
  grn_collect_logger_clear_messages(logger);
  assert_send_command("load --table Users --input_type json\n"
                      "{\"_key\": \"groonga\", \"info\" \"search engine\"}\n"
                      "");
  log = (GList *)grn_collect_logger_get_messages(logger);
  cut_assert_equal_string("invalid column('info')", g_list_nth_data(log, 0));
}
