/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/* Copyright(C) 2009 Brazil

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

void test_nil_column_reference_value(void);
void test_output_columns_with_space(void);
void test_vector_geo_point(void);
void test_vector_geo_point_with_query(void);
void test_unmatched_output_columns(void);
void test_vector_int32(void);
void test_vector_text(void);
void test_vector_reference_id(void);
void test_nonexistent_id(void);
void test_bigram_split_symbol_tokenizer(void);
void test_nonexistent_table(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-select",
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
test_nil_column_reference_value(void)
{
  const gchar *actual;

  assert_send_commands("table_create Sites TABLE_PAT_KEY ShortText Int32\n"
                       "column_create Sites link COLUMN_SCALAR Sites\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\",\"_value\"],\n"
                       "[\"groonga.org\",0],\n"
                       "[\"razil.jp\",0]\n"
                       "]");
  actual = send_command("select Sites");
  cut_assert_equal_string("[[[2],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"],"
                           "[\"_value\",\"Int32\"],"
                           "[\"link\",\"Sites\"]],"
                          "[1,\"groonga.org\",0,\"\"],"
                          "[2,\"razil.jp\",0,\"\"]]]", actual);
}

void
test_output_columns_with_space(void)
{
  assert_send_commands("table_create Sites TABLE_HASH_KEY ShortText\n"
                       "column_create Sites uri COLUMN_SCALAR ShortText\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\",\"uri\"],\n"
                       "[\"groonga\",\"http://groonga.org/\"],\n"
                       "[\"razil\",\"http://razil.jp/\"]\n"
                       "]");
  cut_assert_equal_string("[[[2],"
                          "[[\"_key\",\"ShortText\"],"
                           "[\"uri\",\"ShortText\"]],"
                          "[\"groonga\",\"http://groonga.org/\"],"
                          "[\"razil\",\"http://razil.jp/\"]]]",
                          send_command("select Sites "
                                       "--output_columns '_key, uri'"));
}

void
test_vector_geo_point(void)
{
  assert_send_command("table_create Shops TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Shops places COLUMN_VECTOR WGS84GeoPoint");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"places\"],"
                      "[\"daruma\","
                      "[\"130094061x505025099\",\"130185500x505009000\"]]"
                      "]' "
                      "Shops");
  cut_assert_equal_string("[["
                          "[1],"
                          "["
                          "[\"_id\",\"UInt32\"],"
                          "[\"_key\",\"ShortText\"],"
                          "[\"places\",\"WGS84GeoPoint\"]"
                          "],"
                          "[1,\"daruma\","
                          "[\"130094061x505025099\",\"130185500x505009000\"]]"
                          "]]",
                          send_command("select Shops"));
}

void
test_vector_geo_point_with_query(void)
{
  assert_send_command("table_create Shops TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Shops places COLUMN_VECTOR WGS84GeoPoint");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"places\"],"
                      "[\"daruma\","
                      "[\"130094061x505025099\",\"130185500x505009000\"]]"
                      "]' "
                      "Shops");
  cut_assert_equal_string("[["
                          "[1],"
                          "["
                          "[\"_id\",\"UInt32\"],"
                          "[\"_key\",\"ShortText\"],"
                          "[\"places\",\"WGS84GeoPoint\"]"
                          "],"
                          "[1,\"daruma\","
                          "[\"130094061x505025099\",\"130185500x505009000\"]]"
                          "]]",
                          send_command("select Shops --query _key:daruma"));
}

void
test_unmatched_output_columns(void)
{
  assert_send_command("table_create Answer TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Answer value COLUMN_SCALAR UInt32");
  assert_send_command("table_create Question TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Question num COLUMN_SCALAR UInt32");
  assert_send_command("column_create Question answer COLUMN_SCALAR Answer");
  assert_send_command("load '"
                      "["
                      "[\"_key\",\"value\"],"
                      "[\"ultimate\",42]"
                      "]' "
                      "Answer");
  assert_send_command("load '"
                      "["
                      "[\"_key\",\"answer\"],"
                      "[\"universe\",\"ultimate\"]"
                      "[\"mankind\",\"\"],"
                      "]' "
                      "Question");
  cut_assert_equal_string("[["
                          "[2],"
                          "["
                          "[\"_key\",\"ShortText\"],"
                          "[\"num\",\"UInt32\"],"
                          "[\"answer.value\",\"UInt32\"]"
                          "],"
                          "[\"universe\",0,42],"
                          "[\"mankind\",0,0]"
                          "]]",
                          send_command("select Question"
                                       " --output_columns"
                                       " \"_key, num, answer.value\""));
}

void
test_vector_int32(void)
{
  assert_send_command("table_create Students TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Students scores COLUMN_VECTOR Int32");

  cut_assert_equal_string("1",
                          send_command("load --table Students\n"
                                       "[{\"_key\": \"Daijiro MORI\", "
                                         "\"scores\": [5, 5, 5]}]"));
  cut_assert_equal_string("[[[1],"
                           "[[\"_id\",\"UInt32\"],"
                            "[\"_key\",\"ShortText\"],"
                            "[\"scores\",\"Int32\"]],"
                           "[1,\"Daijiro MORI\",[5,5,5]]]]",
                          send_command("select Students"));
}

void
test_vector_text(void)
{
  assert_send_command("table_create Blogs TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Blogs articles COLUMN_VECTOR Text");
  assert_send_command("table_create Terms "
                      "TABLE_PAT_KEY|KEY_NORMALIZE ShortText "
                      "--default_tokenizer TokenBigram");
  assert_send_command("column_create Terms Blogs_articles "
                      "COLUMN_INDEX|WITH_POSITION Blogs articles");

  assert_send_command("load --table Blogs --columns '_key, articles' \n"
                      "[\n"
                      " [\"gunya-gunya\", "
                      "  [\"hello all!\", "
                      "   \"hello groonga!\", "
                      "   \"hello Senna!\"]],\n"
                      " [\"groonga\", "
                      "  [\"Release!\", "
                      "   \"My name is groonga!\"]],\n"
                      " [\"Senna\", "
                      "  [\"Release!\", "
                      "   \"My name is Senna!\"]]\n"
                      "]");
  cut_assert_equal_string("[[[2],"
                            "[[\"_key\",\"ShortText\"],"
                             "[\"articles\",\"Text\"]],"
                            "[\"gunya-gunya\","
                             "[\"hello all!\","
                              "\"hello groonga!\","
                              "\"hello Senna!\"]],"
                            "[\"groonga\","
                             "[\"Release!\","
                              "\"My name is groonga!\"]]]]",
                          send_command("select Blogs "
                                       "--output_columns _key,articles "
                                       "--match_columns articles "
                                       "--query groonga"));
}

void
test_vector_reference_id(void)
{
  assert_send_command("table_create Users TABLE_NO_KEY");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("table_create Comments TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Comments text COLUMN_SCALAR ShortText");
  assert_send_command("column_create Comments authors COLUMN_VECTOR Users");

  cut_assert_equal_string("2",
                          send_command("load --table Users\n"
                                       "[\n"
                                       " [\"_id\", \"name\"],\n"
                                       " [1, \"ryoqun\"],\n"
                                       " [2, \"hayamiz\"]\n"
                                       "]"));
  cut_assert_equal_string("1",
                          send_command("load --table Comments\n"
                                       "[\n"
                                       " [\"_key\", \"text\", \"authors\"],\n"
                                       " [\"groonga\", \"it is fast\", [1, 2]]\n"
                                       "]"));
  cut_assert_equal_string("[[[1],"
                           "[[\"_id\",\"UInt32\"],"
                            "[\"_key\",\"ShortText\"],"
                            "[\"text\",\"ShortText\"],"
                            "[\"authors\",\"Users\"]],"
                           "[1,\"groonga\",\"it is fast\",[1,2]]]]",
                          send_command("select Comments"));
}

void
test_nonexistent_id(void)
{
  assert_send_commands("table_create Sites TABLE_PAT_KEY ShortText\n"
                       "column_create Sites link COLUMN_SCALAR Sites\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\"],\n"
                       "[\"groonga.org\"],\n"
                       "[\"razil.jp\"]\n"
                       "]");
  cut_assert_equal_string("[[[0],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"]]]]",
                          send_command("select Sites "
                                       "--output_columns '_id _key' "
                                       "--filter '_id == 100'"));
}

void
test_bigram_split_symbol_tokenizer(void)
{
  assert_send_commands("table_create Softwares TABLE_HASH_KEY ShortText\n"
                       "column_create Softwares comment "
                       "COLUMN_SCALAR ShortText\n"
                       "table_create Terms TABLE_PAT_KEY ShortText "
                       "--default_tokenizer TokenBigramSplitSymbol\n"
                       "column_create Terms Softwares_comment "
                       "COLUMN_INDEX|WITH_POSITION Softwares comment\n"
                       "load --table Softwares\n"
                       "[\n"
                       "[\"_key\", \"comment\"],\n"
                       "[\"groonga\", \"うむ上々な仕上がりだね。\"],\n"
                       "[\"Cutter\", \"使いやすいじゃないか。\"]\n"
                       "[\"Senna\", \"悪くないね。上々だよ。\"]\n"
                       "]");
  cut_assert_equal_string("[[[2],"
                           "[[\"_key\",\"ShortText\"]],"
                           "[\"groonga\"],"
                           "[\"Senna\"]]]",
                          send_command("select Softwares "
                                       "--output_columns '_key' "
                                       "--match_columns comment "
                                       "--query 上々"));
}

void
test_nonexistent_table(void)
{
  const gchar *command = "select nonexistent";

  grn_ctx_send(context, command, strlen(command), 0);
  grn_test_assert_error(GRN_INVALID_ARGUMENT,
                        "invalid table name: nonexistent",
                        context);
}
