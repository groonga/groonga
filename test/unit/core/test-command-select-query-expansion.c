/* -*- c-basic-offset: 2; coding: utf-8 -*- */
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_expand(void);
void test_no_expand(void);
void test_nonexistent_expansion_column(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-select-query-expansion",
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

static void
setup_data(void)
{
  assert_send_command("table_create Diaries TABLE_HASH_KEY Time");
  assert_send_command("column_create Diaries content COLUMN_SCALAR Text");
  assert_send_command("table_create Lexicon TABLE_PAT_KEY ShortText "
                      "--default_tokenizer TokenBigram");
  assert_send_command("column_create Lexicon diary_content "
                      "COLUMN_INDEX|WITH_POSITION Diaries content");
  assert_send_command("table_create Synonyms TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Synonyms words COLUMN_SCALAR ShortText");
  assert_send_command("load --table Diaries\n"
                      "[\n"
                      "[\"_key\", \"content\"],\n"
                      "[\"2011-09-11 00:00:00\", \"Start groonga!\"],\n"
                      "[\"2011-09-12 00:00:00\", \"Start mroonga!\"],\n"
                      "[\"2011-09-13 00:00:00\", \"Start rroonga!\"]\n"
                      "]");
  assert_send_command("load --table Synonyms\n"
                      "[\n"
                      "[\"_key\", \"words\"],\n"
                      "[\"groonga\", \"(groonga OR mroonga)\"]\n"
                      "]");
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

  setup_data();
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
test_expand(void)
{
  cut_assert_equal_string(
      "[[[2],"
       "[[\"_id\",\"UInt32\"],"
        "[\"_key\",\"Time\"],"
        "[\"content\",\"Text\"]],"
       "[1,1315666800.0,\"Start groonga!\"],"
       "[2,1315753200.0,\"Start mroonga!\"]]]",
    send_command("select Diaries --match_columns content --query groonga "
                 "--query_expand Synonyms.words"));
}

void
test_no_expand(void)
{
  cut_assert_equal_string(
      "[[[1],"
       "[[\"_id\",\"UInt32\"],"
        "[\"_key\",\"Time\"],"
        "[\"content\",\"Text\"]],"
       "[3,1315839600.0,\"Start rroonga!\"]]]",
    send_command("select Diaries --match_columns content --query rroonga "
                 "--query_expand Synonyms.words"));
}

void
test_nonexistent_expansion_column(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "nonexistent query expansion column: <Synonyms.nonexistent>",
    "select Diaries --match_columns content --query groonga "
    "--query_expand Synonyms.nonexistent");
}
