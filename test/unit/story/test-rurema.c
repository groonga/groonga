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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_complete_prefix_rk_search(void);
void test_complete_prefix_rk_search(gconstpointer data);
void test_complete_prefix_rk_search_threshold_found(void);
void test_complete_prefix_rk_search_threshold_not_found(void);
void test_complete_cooccurrence(void);
void data_complete_prefix_search(void);
void test_complete_prefix_search(gconstpointer data);
void test_complete_prefix_search_threshold_found(void);
void test_complete_prefix_search_threshold_not_found(void);
void test_complete_prefix_search_upcase(void);
void test_complete_prefix_search_downcase(void);
void test_correct_cooccurrence(void);
void test_correct_similar(void);
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

  ADD_DATUM("romaji", "saku");
  ADD_DATUM("katakana", "サク");
  ADD_DATUM("hiragana", "さく");
  ADD_DATUM("hiragana + romaji", "さｋ");

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
      "[\"削除\",1]]}",
    send_command(
      cut_take_printf(
        "suggest "
        "--table item_rurema "
        "--column kana "
        "--types complete "
        "--frequency_threshold 1 "
        "--query '%s'",
        gcut_data_get_string(data, "query"))));
}

void
test_complete_prefix_rk_search_threshold_found(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"変数\",101]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--frequency_threshold 101 "
      "--query 'hen'"));
}

void
test_complete_prefix_rk_search_threshold_not_fuond(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[0],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--frequency_threshold 102 "
      "--query 'hen'"));
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
        "--frequency_threshold 1 "
        "--conditional_probability_threshold 0.1 "
        "--query 'ｓｔりん'"));
}

void
test_complete_prefix_search_force(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"置換\",1]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--prefix_search yes "
      "--frequency_threshold 1 "
      "--query '置'"));
}

void
test_complete_prefix_search_disable(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[0],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--prefix_search no "
      "--frequency_threshold 1 "
      "--query '置'"));
}

void
test_complete_prefix_search_threshold_found(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"変数\",101]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--frequency_threshold 101 "
      "--query '変'"));
}

void
test_complete_prefix_search_threshold_not_fuond(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[0],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--frequency_threshold 102 "
      "--query '変'"));
}

void
test_complete_prefix_search_upcase(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"ssh\",101]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--frequency_threshold 1 "
      "--query 'SSH'"));
}

void
test_complete_prefix_search_downcase(void)
{
  cut_assert_equal_string(
    "{\"complete\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"ssh\",101]]}",
    send_command(
      "suggest "
      "--table item_rurema "
      "--column kana "
      "--types complete "
      "--frequency_threshold 1 "
      "--query 'ssh'"));
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
        "--frequency_threshold 1 "
        "--query 'avg'"));
}

void
test_correct_similar(void)
{
  cut_assert_equal_string(
    "{\"correct\":"
     "[[1],"
      "[[\"_key\",\"ShortText\"],"
       "[\"_score\",\"Int32\"]],"
      "[\"kernel.\",1]]}",
    send_command(
        "suggest "
        "--table item_rurema "
        "--column kana "
        "--types correct "
        "--frequency_threshold 1 "
        "--query 'kernel'"));
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
        "--frequency_threshold 1 "
        "--query 'CSV'"));
}
