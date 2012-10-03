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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void data_expand_OR_quoted(void);
void test_expand_OR_quoted(gconstpointer data);
void data_expand_column_value(void);
void test_expand_column_value(gconstpointer data);
void data_expand_column_value_with_space(void);
void test_expand_column_value_with_space(gconstpointer data);
void data_expand_equal(void);
void test_expand_equal(gconstpointer data);
void data_expand_prefix(void);
void test_expand_prefix(gconstpointer data);
void data_not_expand_OR(void);
void test_not_expand_OR(gconstpointer data);
void data_not_expand_OR_at_the_end(void);
void test_not_expand_OR_at_the_end(gconstpointer data);
void data_not_expand_OR_with_leading_space(void);
void test_not_expand_OR_with_leading_space(gconstpointer data);
void data_not_expand_and(void);
void test_not_expand_and(gconstpointer data);
void data_not_expand_but(void);
void test_not_expand_but(gconstpointer data);
void data_not_expand_paren(void);
void test_not_expand_paren(gconstpointer data);
void data_no_expand(void);
void test_no_expand(gconstpointer data);
void data_no_expand_word_with_space(void);
void test_no_expand_word_with_space(gconstpointer data);
void data_nonexistent_expansion_column(void);
void test_nonexistent_expansion_column(gconstpointer data);
void data_key_normalize(void);
void test_key_normalize(gconstpointer data);

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
  assert_send_command("column_create Synonyms words_scalar "
                      "COLUMN_SCALAR ShortText");
  assert_send_command("column_create Synonyms words_vector "
                      "COLUMN_VECTOR ShortText");
  assert_send_command("load --table Diaries\n"
                      "[\n"
                      "[\"_key\", \"content\"],\n"
                      "[\"2011-09-11 00:00:00\", \"Start groonga!\"],\n"
                      "[\"2011-09-12 00:00:00\", \"Start mroonga!\"],\n"
                      "[\"2011-09-13 00:00:00\", \"Start rroonga!\"],\n"
                      "[\"2011-09-14 00:00:00\", \"Start Ruby!\"],\n"
                      "[\"2011-09-15 00:00:00\", \"Start MySQL!\"],\n"
                      "[\"2011-09-16 00:00:00\", "
                       "\"Setup groonga storage engine!\"],\n"
                      "[\"2011-09-17 00:00:00\", \"Learning MySQL...\"],\n"
                      "[\"2011-09-18 00:00:00\", "
                       "\"Learning MySQL and groonga...\"],\n"
                      "[\"2011-09-19 00:00:00\", "
                       "\"Learning Ruby and groonga...\"],\n"
                      "[\"2011-09-20 00:00:00\", "
                       "\"明日は日本語あるいは中国語を勉強します。\"],\n"
                      "[\"2011-09-21 00:00:00\", "
                       "\"かつ 差集合 より重要 重要度を下げる "
                         "補集合 前方一致 かっこ こっか\"]\n"
                      "]");
  assert_send_command("load --table Synonyms\n"
                      "[\n"
                      "[\"_key\", \"words_scalar\", \"words_vector\"],\n"
                      "[\"groonga\", "
                       "\"(groonga OR rroonga OR mroonga)\", "
                       "[\"groonga\", \"rroonga\", \"mroonga\"]],\n"
                      "[\"rroonga\", "
                       "\"(rroonga OR (Ruby groonga))\", "
                       "[\"rroonga\", \"Ruby groonga\"]],\n"
                      "[\"mroonga\", "
                       "\"(mroonga OR (groonga MySQL) OR "
                          "\\\"groonga storage engine\\\")\", "
                       "[\"mroonga\", "
                        "\"groonga MySQL\", "
                        "\"\\\"groonga storage engine\\\"\"]],\n"
                      "[\"groonga storage engine\", "
                       "\"(\\\"groonga storage engine\\\" OR mroonga)\", "
                       "[\"\\\"groonga storage engine\\\"\", \"mroonga\"]],\n"
                      "[\"OR\", \"あるいは\", [\"あるいは\"]],\n"
                      "[\"+\", \"かつ\", [\"かつ\"]],\n"
                      "[\"-\", \"差集合\", [\"差集合\"]],\n"
                      "[\">\", \"より重要\", [\"より重要\"]],\n"
                      "[\"<\", \"重要度を下げる\", [\"重要度を下げる\"]],\n"
                      "[\"~\", \"補集合\", [\"補集合\"]],\n"
                      "[\"*\", \"前方一致\", [\"前方一致\"]],\n"
                      "[\"(\", \"かっこ\", [\"かっこ\"]],\n"
                      "[\")\", \"こっか\", [\"こっか\"]]\n"
                      "[\"=start-rroonga\", "
                       "\"\\\"Start rroonga!\\\"\", "
                       "[\"\\\"Start rroonga!\\\"\"]],\n"
                      "[\"Japan\", \"日本\", [\"日本\"]]\n"
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

static void
data_scalar_and_vector(void)
{
#define ADD_DATA(label, column_name)                            \
  gcut_add_datum(label,                                         \
                 "column-name", G_TYPE_STRING, column_name,     \
                 NULL)

  ADD_DATA("scalar", "words_scalar");
  ADD_DATA("vector", "words_vector");

#undef ADD_DATA
}

void
data_expand_OR_quoted(void)
{
  data_scalar_and_vector();
}

void
test_expand_OR_quoted(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
     "[10,1316444400.0,\"明日は日本語あるいは中国語を勉強します。\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query '\"OR\"' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_expand_column_value(void)
{
  data_scalar_and_vector();
}

void
test_expand_column_value(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[2],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
      "[3,1315839600.0,\"Start rroonga!\"],"
      "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query 'content:@rroonga' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_expand_column_value_with_space(void)
{
  data_scalar_and_vector();
}

void
test_expand_column_value_with_space(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[2],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
     "[2,1315753200.0,\"Start mroonga!\"],"
     "[6,1316098800.0,\"Setup groonga storage engine!\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content "
                      "--query 'content:@\"groonga storage engine\"' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_expand_equal(void)
{
  data_scalar_and_vector();
}

void
test_expand_equal(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
      "[3,1315839600.0,\"Start rroonga!\"]]]",
    send_command(
      cut_take_printf(
        "select Diaries --sortby _id "
        "--match_columns content --query 'content:=start-rroonga' "
        "--query_expansion Synonyms.%s",
        gcut_data_get_string(data, "column-name"))));
}

void
data_expand_prefix(void)
{
  data_scalar_and_vector();
}

void
test_expand_prefix(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
      "[10,1316444400.0,\"明日は日本語あるいは中国語を勉強します。\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query 'Japan*' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_not_expand_OR(void)
{
  data_scalar_and_vector();
}

void
test_not_expand_OR(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[5],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
     "[4,1315926000.0,\"Start Ruby!\"],"
     "[5,1316012400.0,\"Start MySQL!\"],"
     "[7,1316185200.0,\"Learning MySQL...\"],"
     "[8,1316271600.0,\"Learning MySQL and groonga...\"],"
     "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query 'Ruby OR MySQL' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_not_expand_OR_at_the_end(void)
{
  data_scalar_and_vector();
}

void
test_not_expand_OR_at_the_end(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[0],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query OR "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_not_expand_OR_with_leading_space(void)
{
  data_scalar_and_vector();
}

void
test_not_expand_OR_with_leading_space(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[0],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query '\"OR \"' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_not_expand_and(void)
{
  data_scalar_and_vector();
}

void
test_not_expand_and(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
      "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query 'Ruby + groonga' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_not_expand_but(void)
{
  data_scalar_and_vector();
}

void
test_not_expand_but(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
      "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query 'Ruby - Start' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_not_expand_paren(void)
{
  data_scalar_and_vector();
}

void
test_not_expand_paren(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[2],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
      "[4,1315926000.0,\"Start Ruby!\"],"
      "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries --sortby _id "
                      "--match_columns content --query '(Ruby)' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_no_expand(void)
{
  data_scalar_and_vector();
}

void
test_no_expand(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[2],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
     "[4,1315926000.0,\"Start Ruby!\"],"
     "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries --match_columns content --query Ruby "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_no_expand_word_with_space(void)
{
  data_scalar_and_vector();
}

void
test_no_expand_word_with_space(gconstpointer data)
{
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
     "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries "
                      "--match_columns content --query '\"Ruby and groonga\"' "
                      "--query_expansion Synonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}

void
data_nonexistent_expansion_column(void)
{
  data_scalar_and_vector();
}

void
test_nonexistent_expansion_column(gconstpointer data)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "nonexistent query expansion column: <Synonyms.nonexistent>",
    "select Diaries --match_columns content --query groonga "
    "--query_expansion Synonyms.nonexistent");
}

void
data_key_normalize(void)
{
  data_scalar_and_vector();
}

void
test_key_normalize(gconstpointer data)
{
  assert_send_command("table_create NormalizedSynonyms "
                      "TABLE_PAT_KEY|KEY_NORMALIZE ShortText");
  assert_send_command("column_create NormalizedSynonyms words_scalar "
                      "COLUMN_SCALAR ShortText");
  assert_send_command("column_create NormalizedSynonyms words_vector "
                      "COLUMN_VECTOR ShortText");
  assert_send_command("load --table NormalizedSynonyms\n"
                      "[\n"
                      "[\"_key\", \"words_scalar\", \"words_vector\"],\n"
                      "[\"Ruby\", "
                       "\"(Ruby OR rroonga)\", "
                       "[\"Ruby\", \"rroonga\"]]\n"
                      "]");

  cut_assert_equal_string(
    "[[[3],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"Time\"],"
      "[\"content\",\"Text\"]],"
     "[3,1315839600.0,\"Start rroonga!\"],"
     "[4,1315926000.0,\"Start Ruby!\"],"
     "[9,1316358000.0,\"Learning Ruby and groonga...\"]]]",
    send_command(
      cut_take_printf("select Diaries "
                      "--sortby _id "
                      "--match_columns content --query ruby "
                      "--query_expansion NormalizedSynonyms.%s",
                      gcut_data_get_string(data, "column-name"))));
}
