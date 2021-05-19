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

#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_table(void);
void test_table_with_no_column(void);
void test_column(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *table, *column;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "rename",
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

  table = NULL;
  column = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    grn_obj_unlink(context, column);
    grn_obj_unlink(context, table);
    grn_obj_unlink(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_table(void)
{
  const gchar *actual;

  assert_send_commands("table_create Sites TABLE_PAT_KEY ShortText\n"
                       "column_create Sites link COLUMN_SCALAR Sites\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\",\"link\"],\n"
                       "[\"groonga.org\",\"razil.jp\"],\n"
                       "[\"razil.jp\",\"qwik.jp/senna/\"]\n"
                       "]");
  actual = send_command("select Sites");
  cut_assert_equal_string("[[[3],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"],"
                           "[\"link\",\"Sites\"]],"
                          "[1,\"groonga.org\",\"razil.jp\"],"
                          "[3,\"qwik.jp/senna/\",\"\"],"
                          "[2,\"razil.jp\",\"qwik.jp/senna/\"]]]",
                          actual);

  table = grn_ctx_get(context, "Sites", strlen("Sites"));
  grn_test_assert(grn_table_rename(context, table, "URLs", strlen("URLs")));
  grn_test_assert_context(context);

  actual = send_command("select URLs");
  cut_assert_equal_string("[[[3],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"],"
                           "[\"link\",\"URLs\"]],"
                          "[1,\"groonga.org\",\"razil.jp\"],"
                          "[3,\"qwik.jp/senna/\",\"\"],"
                          "[2,\"razil.jp\",\"qwik.jp/senna/\"]]]",
                          actual);
}

void
test_table_with_no_column(void)
{
  const gchar *actual;

  assert_send_commands("table_create Sites TABLE_PAT_KEY ShortText\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\"],\n"
                       "[\"groonga.org\"],\n"
                       "[\"razil.jp\"]\n"
                       "]");
  actual = send_command("select Sites");
  cut_assert_equal_string("[[[2],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"]],"
                          "[1,\"groonga.org\"],"
                          "[2,\"razil.jp\"]]]",
                          actual);

  table = grn_ctx_get(context, "Sites", strlen("Sites"));
  grn_test_assert(grn_table_rename(context, table, "URLs", strlen("URLs")));
  grn_test_assert_context(context);

  actual = send_command("select URLs");
  cut_assert_equal_string("[[[2],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"]],"
                          "[1,\"groonga.org\"],"
                          "[2,\"razil.jp\"]]]",
                          actual);
}

void
test_column(void)
{
  const gchar *actual;

  assert_send_commands("table_create Users TABLE_PAT_KEY ShortText\n"
                       "column_create Users nick COLUMN_SCALAR ShortText\n"
                       "load --table Users\n"
                       "[\n"
                       "[\"_key\",\"nick\"],\n"
                       "[\"Daijiro MORI\",\"daijiro\"],\n"
                       "[\"Kouhei Sutou\",\"kou\"]\n"
                       "]");
  actual = send_command("select Users");
  cut_assert_equal_string("[[[2],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"],"
                           "[\"nick\",\"ShortText\"]],"
                          "[1,\"Daijiro MORI\",\"daijiro\"],"
                          "[2,\"Kouhei Sutou\",\"kou\"]]]",
                          actual);

  column = grn_ctx_get(context, "Users.nick", strlen("Users.nick"));
  grn_test_assert(grn_column_rename(context, column,
                                    "account", strlen("account")));
  grn_test_assert_context(context);

  actual = send_command("select Users");
  cut_assert_equal_string("[[[2],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"],"
                           "[\"account\",\"ShortText\"]],"
                          "[1,\"Daijiro MORI\",\"daijiro\"],"
                          "[2,\"Kouhei Sutou\",\"kou\"]]]",
                          actual);
}
