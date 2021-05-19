/*
  Copyright(C) 2011 Kouhei Sutou <kou@clear-code.com>

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

#include "grn_str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_no_columns(void);
void test_have_columns(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-truncate",
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

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  if (context) {
    grn_obj_unlink(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_no_columns(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("load --table Users\n"
                      "[\n"
                      "{\"_key\":\"mori\"},\n"
                      "{\"_key\":\"gunyara-kun\"},\n"
                      "{\"_key\":\"yu\"}\n"
                      "]");
  cut_assert_equal_string(
      "[[[3],"
       "[[\"_id\",\"UInt32\"],[\"_key\",\"ShortText\"]],"
       "[1,\"mori\"],"
       "[2,\"gunyara-kun\"],"
       "[3,\"yu\"]]]",
    send_command("select Users"));
  cut_assert_equal_string("true",
                          send_command("truncate Users"));
  cut_assert_equal_string(
      "[[[0],"
       "[[\"_id\",\"UInt32\"],[\"_key\",\"ShortText\"]]"
       "]]",
    send_command("select Users"));
}

void
test_have_columns(void)
{
  assert_send_command("table_create Users TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("load --table Users\n"
                      "[\n"
                      "{\"_key\":\"mori\", \"name\":\"Daijiro MORI\"},\n"
                      "{\"_key\":\"gunyara-kun\", \"name\":\"Tasuku SUENAGA\"},\n"
                      "{\"_key\":\"yu\", \"name\":\"Yutaro Shimamura\"}\n"
                      "]");
  cut_assert_equal_string(
      "[[[3],"
       "[[\"_id\",\"UInt32\"],"
        "[\"_key\",\"ShortText\"],"
        "[\"name\",\"ShortText\"]],"
       "[1,\"mori\",\"Daijiro MORI\"],"
       "[2,\"gunyara-kun\",\"Tasuku SUENAGA\"],"
       "[3,\"yu\",\"Yutaro Shimamura\"]]]",
    send_command("select Users --sort_keys _id"));
  cut_assert_equal_string("true",
                          send_command("truncate Users"));
  cut_assert_equal_string(
      "[[[0],"
       "[[\"_id\",\"UInt32\"],"
        "[\"_key\",\"ShortText\"],"
        "[\"name\",\"ShortText\"]]"
       "]]",
    send_command("select Users"));
}
