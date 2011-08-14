/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>

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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_complete_prefix_rk_search(void);
void test_complete_prefix_rk_search(gconstpointer data);
void test_complete_cooccurrence(void);
void test_correct_cooccurrence(void);
void test_suggest_cooccurrence(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "rurema",
                                   NULL);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = NULL;
}

void
cut_shutdown(void)
{
  if (context) {
    grn_obj_unlink(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();

  g_free(tmp_directory);
}

void
cut_setup(void)
{
  const gchar *database_path;

  if (context) {
    return;
  }

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  cut_set_fixture_data_dir(grn_test_get_base_dir(),
                           "fixtures",
                           "story",
                           "rurema",
                           NULL);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);

  assert_send_commands(cut_get_fixture_data_string("ddl.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("learn.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("items.grn", NULL));
}

void
cut_teardown(void)
{
}

void
data_complete_prefix_rk_search(void)
{
#define ADD_DATUM(label, query)                 \
  gcut_add_datum(label " - " query,             \
                 "query", G_TYPE_STRING, query, \
                 NULL)

  ADD_DATUM("romaji", "tan");
  ADD_DATUM("katakana", "タン");
  ADD_DATUM("hiragana", "たん");
  ADD_DATUM("hiragana + romaji", "たｎ");

#undef ADD_DATUM
}

void
test_complete_prefix_rk_search(gconstpointer data)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"短縮\",1]]}",
    send_command(
      cut_take_printf(
        "suggest "
        "--table item_rurema "
        "--column kana "
        "--types complete "
        "--threshold 1 "
        "--query '%s'",
        gcut_data_get_string(data, "query"))));
}

void
test_complete_coocurrence(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"string\",1]]}",
    send_command(
        "suggest "
        "--table item_rurema "
        "--column kana "
        "--types complete "
        "--threshold 1 "
        "--query 'ｓｔりん'"));
}

void
test_correct_coocurrence(void)
{
  cut_assert_equal_string(
    "{\"correct\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"sum\",1]]}",
    send_command(
        "suggest "
        "--table item_rurema "
        "--column kana "
        "--types correct "
        "--threshold 1 "
        "--query 'avg'"));
}

void
test_suggest_coocurrence(void)
{
  cut_assert_equal_string(
    "{\"suggest\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"csv generate\",1]]}",
    send_command(
        "suggest "
        "--table item_rurema "
        "--column kana "
        "--types suggest "
        "--threshold 1 "
        "--query 'CSV'"));
}
