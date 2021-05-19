/*
  Copyright(C) 2009-2010 Brazil
  Copyright(C) 2011-2019 Sutou Kouhei <kou@clear-code.com>

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

void test_unmatched_output_columns(void);
void test_vector_int32(void);
void test_vector_text(void);
void test_vector_reference_id(void);
void test_vector_int32_reference_key(void);
void test_tautology(void);
void test_contradiction(void);
void test_filter_null(void);
void test_bigram_split_symbol_tokenizer(void);
void test_nonexistent_table(void);
void test_boolean(void);
void test_equal_index(void);
void data_less_than(void);
void test_less_than(gconstpointer data);
void data_less_than_or_equal(void);
void test_less_than_or_equal(gconstpointer data);
void data_equal_numeric(void);
void test_equal_numeric(gconstpointer data);
void data_not_tokenize_by_full_width_space(void);
void test_not_tokenize_by_full_width_space(gconstpointer data);

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
                      "[\"universe\",\"ultimate\"],"
                      "[\"mankind\",\"\"]"
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
                      "COLUMN_INDEX|WITH_POSITION|WITH_SECTION Blogs articles");

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
                            "[\"authors\",\"Users\"],"
                            "[\"text\",\"ShortText\"]],"
                           "[1,\"groonga\",[1,2],\"it is fast\"]]]",
                          send_command("select Comments"));
}

void
test_vector_int32_reference_key(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY Int32");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("table_create Comments TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Comments text COLUMN_SCALAR ShortText");
  assert_send_command("column_create Comments authors COLUMN_VECTOR Users");

  cut_assert_equal_string("2",
                          send_command("load --table Users\n"
                                       "[\n"
                                       "[\"_key\",\"name\"],\n"
                                       "[1000,\"ryoqun\"],\n"
                                       "[1001,\"hayamiz\"]\n"
                                       "]"));

  cut_assert_equal_string(
    "1",
    send_command("load --table Comments\n"
                 "[\n"
                 "[\"_key\",\"text\",\"authors\"],\n"
                 "[\"groonga\",\"it is fast\",[1000,1001]]\n"
                 "]"));

  cut_assert_equal_string("[[[1],"
                           "[[\"_id\",\"UInt32\"],"
                            "[\"_key\",\"ShortText\"],"
                            "[\"authors\",\"Users\"],"
                            "[\"text\",\"ShortText\"]],"
                           "[1,\"groonga\",[1000,1001],\"it is fast\"]]]",
                          send_command("select Comments"));
}

void
test_tautology(void)
{
  assert_send_commands("table_create Sites TABLE_PAT_KEY ShortText\n"
                       "column_create Sites link COLUMN_SCALAR Sites\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\"],\n"
                       "[\"groonga.org\"],\n"
                       "[\"razil.jp\"]\n"
                       "]");
  cut_assert_equal_string("[[[2],"
                          "[[\"_id\",\"UInt32\"],"
                           "[\"_key\",\"ShortText\"]],"
                           "[1,\"groonga.org\"],[2,\"razil.jp\"]]]",
                          send_command("select Sites "
                                       "--output_columns '_id _key' "
                                       "--filter true"));
}

void
test_contradiction(void)
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
                                       "--filter false"));
}

void
test_null(void)
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
                                       "--filter null"));
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
                       "[\"Cutter\", \"使いやすいじゃないか。\"],\n"
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
                        "[select][table] invalid name: <nonexistent>",
                        context);
}

void
test_boolean(void)
{
  assert_send_command("table_create Blogs TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Blogs public COLUMN_SCALAR Bool");

  assert_send_command("load --table Blogs --columns '_key, public' \n"
                      "[\n"
                      " [\"groonga 1.0\", true],\n"
                      " [\"groonga 2.0\", false]\n"
                      "]");
  cut_assert_equal_string("[[[1],"
                           "[[\"_key\",\"ShortText\"],"
                            "[\"public\",\"Bool\"]],"
                           "[\"groonga 1.0\",true]]]",
                          send_command("select Blogs "
                                       "--output_columns _key,public "
                                       "--filter public==true"));
}

void
test_equal_index(void)
{
  assert_send_command("table_create Blogs TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Titles TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Blogs title COLUMN_SCALAR Titles");
  assert_send_command("column_create Titles Blogs_title "
                      "COLUMN_INDEX Blogs title");

  assert_send_command("load --table Blogs --columns '_key, title' \n"
                      "[\n"
                      " [\"groonga-1-0\", \"groonga 1.0 release!\"],\n"
                      " [\"groonga-2-0\", \"groonga 2.0 release!\"]\n"
                      "]");
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_key\",\"ShortText\"],"
      "[\"title\",\"Titles\"]],"
      "[\"groonga-1-0\",\"groonga 1.0 release!\"]]]",
    send_command("select Blogs "
                 "--output_columns _key,title "
                 "--filter 'title == \"groonga 1.0 release!\"'"));
}

void
data_less_than(void)
{
#define ADD_DATA(type)                          \
  gcut_add_datum(type,                          \
                 "type", G_TYPE_STRING, type,   \
                 NULL)

  ADD_DATA("Int8");
  ADD_DATA("UInt8");
  ADD_DATA("Int16");
  ADD_DATA("UInt16");
  ADD_DATA("Int32");
  ADD_DATA("UInt32");
  ADD_DATA("Int64");
  ADD_DATA("UInt64");

#undef ADD_DATA
}

void
test_less_than(gconstpointer data)
{
  const gchar *type;

  type = gcut_data_get_string(data, "type");

  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command(
    cut_take_printf("column_create Sites score COLUMN_SCALAR %s", type));

  cut_assert_equal_string(
    "3",
    send_command("load --table Sites --columns '_key, score' \n"
                 "[\n"
                 " [\"groonga.org\", 5],\n"
                 " [\"ruby-lang.org\", 4],\n"
                 " [\"qwik.jp/senna/\", 3]\n"
                 "]"));
  cut_assert_equal_string(
    cut_take_printf("[[[1],"
                     "[[\"_key\",\"ShortText\"],"
                      "[\"score\",\"%s\"]],"
                     "[\"groonga.org\",5]]]",
                    type),
    send_command("select Sites "
                 "--sort_keys -score "
                 "--output_columns _key,score "
                 "--filter 'score > 4'"));
}

void
data_less_than_or_equal(void)
{
#define ADD_DATA(type)                          \
  gcut_add_datum(type,                          \
                 "type", G_TYPE_STRING, type,   \
                 NULL)

  ADD_DATA("Int8");
  ADD_DATA("UInt8");
  ADD_DATA("Int16");
  ADD_DATA("UInt16");
  ADD_DATA("Int32");
  ADD_DATA("UInt32");
  ADD_DATA("Int64");
  ADD_DATA("UInt64");

#undef ADD_DATA
}

void
test_less_than_or_equal(gconstpointer data)
{
  const gchar *type;

  type = gcut_data_get_string(data, "type");

  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command(
    cut_take_printf("column_create Sites score COLUMN_SCALAR %s", type));

  cut_assert_equal_string(
    "3",
    send_command("load --table Sites --columns '_key, score' \n"
                 "[\n"
                 " [\"groonga.org\", 5],\n"
                 " [\"ruby-lang.org\", 4],\n"
                 " [\"qwik.jp/senna/\", 3]\n"
                 "]"));
  cut_assert_equal_string(
    cut_take_printf("[[[2],"
                     "[[\"_key\",\"ShortText\"],"
                      "[\"score\",\"%s\"]],"
                     "[\"groonga.org\",5],"
                     "[\"ruby-lang.org\",4]]]",
                    type),
    send_command("select Sites "
                 "--sort_keys -score "
                 "--output_columns _key,score "
                 "--filter 'score >= 4'"));
}

void
data_equal_numeric(void)
{
#define ADD_DATA(type)                          \
  gcut_add_datum(type,                          \
                 "type", G_TYPE_STRING, type,   \
                 NULL)

  ADD_DATA("Int8");
  ADD_DATA("UInt8");
  ADD_DATA("Int16");
  ADD_DATA("UInt16");
  ADD_DATA("Int32");
  ADD_DATA("UInt32");
  ADD_DATA("Int64");
  ADD_DATA("UInt64");

#undef ADD_DATA
}

void
test_equal_numeric(gconstpointer data)
{
  const gchar *type;

  type = gcut_data_get_string(data, "type");

  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command(
    cut_take_printf("column_create Sites score COLUMN_SCALAR %s", type));

  cut_assert_equal_string(
    "3",
    send_command("load --table Sites --columns '_key, score' \n"
                 "[\n"
                 " [\"groonga.org\", 5],\n"
                 " [\"ruby-lang.org\", 4],\n"
                 " [\"qwik.jp/senna/\", 3]\n"
                 "]"));
  cut_assert_equal_string(
    cut_take_printf("[[[1],"
                     "[[\"_key\",\"ShortText\"],"
                      "[\"score\",\"%s\"]],"
                     "[\"ruby-lang.org\",4]]]",
                    type),
    send_command("select Sites "
                 "--sort_keys -score "
                 "--output_columns _key,score "
                 "--filter 'score == 4'"));
}

void
data_not_tokenize_by_full_width_space(void)
{
#define ADD_DATA(label, error_message, command)                 \
  gcut_add_datum(label,                                         \
                 "error-message", G_TYPE_STRING, error_message, \
                 "command", G_TYPE_STRING, command,             \
                 NULL)

  ADD_DATA("separator",
           "[select][table] invalid name: <Sites　--output_columns>",
           "select Sites"
           "　"
           "--output_columns _key");

  ADD_DATA("prepend",
           "[select][table] invalid name: <　Sites>",
           "select "
           " 　Sites"
           " "
           "--output_columns _key");

#undef ADD_DATA
}

void
test_not_tokenize_by_full_width_space(gconstpointer data)
{
  const gchar *error_message;
  const gchar *command;

  error_message = gcut_data_get_string(data, "error-message");
  command = gcut_data_get_string(data, "command");

  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  cut_assert_equal_string(
    "3",
    send_command("load --table Sites --columns '_key' \n"
                 "[\n"
                 " [\"groonga.org\"],\n"
                 " [\"ruby-lang.org\"],\n"
                 " [\"qwik.jp/senna/\"]\n"
                 "]"));

  assert_send_command_error(GRN_INVALID_ARGUMENT,
                            error_message,
                            command);
}
