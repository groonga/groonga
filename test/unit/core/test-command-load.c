/*
  Copyright (C) 2010-2012  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_str.h>

void test_columns(void);
void attributes_bool(void);
void data_bool(void);
void test_bool(gconstpointer data);
void test_int32_key(void);
void test_time_float(void);
void data_null(void);
void test_null(gconstpointer data);
void test_index_geo_point(void);
void test_no_key_table(void);
void test_two_bigram_indexes_to_key(void);
void test_invalid_start_with_symbol(void);
void test_nonexistent_table_name(void);
void test_invalid_table_name(void);
void data_each(void);
void test_each(gconstpointer data);
void test_vector_reference_column(void);
void test_vector_domain(void);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "load",
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
    grn_obj_close(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_columns(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("column_create Users desc COLUMN_SCALAR ShortText");
  cut_assert_equal_string(
    "2",
    send_command("load --table Users --columns '_key,name,desc'\n"
                 "[\n"
                 "  [\"mori\", \"モリ\", \"タポ\"],\n"
                 "  [\"tapo\", \"タポ\", \"モリモリモリタポ\"]\n"
                 "]"));
  cut_assert_equal_string("[[[2],"
                          "["
                          "[\"_id\",\"UInt32\"],"
                          "[\"_key\",\"ShortText\"],"
                          "[\"desc\",\"ShortText\"],"
                          "[\"name\",\"ShortText\"]"
                          "],"
                          "[1,\"mori\",\"タポ\",\"モリ\"],"
                          "[2,\"tapo\",\"モリモリモリタポ\",\"タポ\"]"
                          "]]",
                          send_command("select Users"));
}

void
attributes_bool(void)
{
  cut_set_attributes("bug", "123, 304",
                     NULL);
}

void
data_bool(void)
{
#define ADD_DATUM(label, load_command)                          \
  gcut_add_datum(label,                                         \
                 "load-command", G_TYPE_STRING, load_command,   \
                 NULL)

  ADD_DATUM("symbol",
            "load --table Users --columns '_key,enabled'\n"
            "[\n"
            "  [\"mori\",true],\n"
            "  [\"tapo\",false]\n"
            "]");
  ADD_DATUM("number",
            "load --table Users --columns '_key,enabled'\n"
            "[\n"
            "  [\"mori\",1],\n"
            "  [\"tapo\",0]\n"
            "]");
  ADD_DATUM("string",
            "load --table Users --columns '_key,enabled'\n"
            "[\n"
            "  [\"mori\",\"1\"],\n"
            "  [\"tapo\",\"\"]\n"
            "]");
  ADD_DATUM("string (null)",
            "load --table Users --columns '_key,enabled'\n"
            "[\n"
            "  [\"mori\",\"0\"],\n"
            "  [\"tapo\",null]\n"
            "]");

#undef ADD_DATUM
}

void
test_bool(gconstpointer data)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users enabled COLUMN_SCALAR Bool");
  cut_assert_equal_string("2",
                          send_command(gcut_data_get_string(data,
                                                            "load-command")));
  cut_assert_equal_string("[[[2],"
                          "["
                          "[\"_id\",\"UInt32\"],"
                          "[\"_key\",\"ShortText\"],"
                          "[\"enabled\",\"Bool\"]"
                          "],"
                          "[1,\"mori\",true],"
                          "[2,\"tapo\",false]"
                          "]]",
                          send_command("select Users"));
}

void
test_int32_key(void)
{
  assert_send_command("table_create Students TABLE_HASH_KEY Int32");
  assert_send_command("column_create Students name COLUMN_SCALAR ShortText");
  cut_assert_equal_string(
    "1",
    send_command("load --table Students\n"
                 "[{\"_key\": 1, \"name\": \"morita\"}]"));
  cut_assert_equal_string("[[[1],"
                          "["
                          "[\"_id\",\"UInt32\"],"
                          "[\"_key\",\"Int32\"],"
                          "[\"name\",\"ShortText\"]"
                          "],"
                          "[1,1,\"morita\"]"
                          "]]",
                          send_command("select Students"));
}

void
test_time_float(void)
{
  assert_send_command("table_create Logs TABLE_NO_KEY");
  assert_send_command("column_create Logs time_stamp COLUMN_SCALAR Time");
  cut_assert_equal_string("1",
                          send_command("load --table Logs\n"
                                       "[{\"time_stamp\": 1295851581.41798}]"));
  cut_assert_equal_string("[[[1],"
                          "["
                          "[\"_id\",\"UInt32\"],"
                          "[\"time_stamp\",\"Time\"]"
                          "],"
                          "[1,1295851581.41798]"
                          "]]",
                          send_command("select Logs"));
}

void
data_null(void)
{
#define ADD_DATUM(label, expected, load)                        \
  gcut_add_datum(label,                                         \
                 "expected", G_TYPE_STRING, expected,           \
                 "load", G_TYPE_STRING, load,                   \
                 NULL)

  ADD_DATUM("string - null",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"nick\",\"ShortText\"],"
               "[\"scores\",\"Int32\"]],"
             "[1,\"Daijiro MORI\",\"\",[5,5,5]]]]",
            "load --table Students --columns '_key, nick'\n"
            "[\n"
            "  [\"Daijiro MORI\", null]\n"
            "]");
  ADD_DATUM("string - empty string",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"nick\",\"ShortText\"],"
               "[\"scores\",\"Int32\"]],"
             "[1,\"Daijiro MORI\",\"\",[5,5,5]]]]",
            "load --table Students --columns '_key, nick'\n"
            "[\n"
            "  [\"Daijiro MORI\", \"\"]\n"
            "]");

  ADD_DATUM("vector - empty null",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"nick\",\"ShortText\"],"
               "[\"scores\",\"Int32\"]],"
             "[1,\"Daijiro MORI\",\"morita\",[]]]]",
            "load --table Students --columns '_key, scores'\n"
            "[\n"
            "  [\"Daijiro MORI\", null]\n"
            "]");
  ADD_DATUM("vector - empty string",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"nick\",\"ShortText\"],"
               "[\"scores\",\"Int32\"]],"
             "[1,\"Daijiro MORI\",\"morita\",[]]]]",
            "load --table Students --columns '_key, scores'\n"
            "[\n"
            "  [\"Daijiro MORI\", \"\"]\n"
            "]");
  ADD_DATUM("vector - empty array",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"nick\",\"ShortText\"],"
               "[\"scores\",\"Int32\"]],"
             "[1,\"Daijiro MORI\",\"morita\",[]]]]",
            "load --table Students --columns '_key, scores'\n"
            "[\n"
            "  [\"Daijiro MORI\", []]\n"
            "]");

#undef ADD_DATUM
}

void
test_null(gconstpointer data)
{
  assert_send_command("table_create Students TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Students nick COLUMN_SCALAR ShortText");
  assert_send_command("column_create Students scores COLUMN_VECTOR Int32");

  cut_assert_equal_string("1",
                          send_command("load --table Students\n"
                                       "[{\"_key\": \"Daijiro MORI\", "
                                         "\"nick\": \"morita\", "
                                         "\"scores\": [5, 5, 5]}]"));
  cut_assert_equal_string("1",
                          send_command(gcut_data_get_string(data, "load")));
  cut_assert_equal_string(gcut_data_get_string(data, "expected"),
                          send_command("select Students"));
}

void
test_no_key_table(void)
{
  assert_send_command("table_create Users TABLE_NO_KEY");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("column_create Users desc COLUMN_SCALAR ShortText");
  cut_assert_equal_string("2",
                          send_command("load "
                                       "--table Users "
                                       "--columns 'name, desc' "
                                       "--values "
                                       "'[[\"mori\", \"the author of groonga\"],"
                                       "[\"gunyara-kun\", \"co-author\"]]'"));
}

void
test_two_bigram_indexes_to_key(void)
{
  cut_omit("crashed!!!");
  assert_send_command("table_create Books TABLE_PAT_KEY ShortText");
  assert_send_command("table_create Authors TABLE_PAT_KEY ShortText");
  assert_send_command("table_create Bigram "
                      "TABLE_PAT_KEY|KEY_NORMALIZE ShortText "
                      "--default_tokenizer TokenBigram");
  assert_send_command("table_create BigramLoose "
                      "TABLE_PAT_KEY|KEY_NORMALIZE ShortText "
                      "--default_tokenizer TokenBigramIgnoreBlankSplitSymbolAlphaDigit");
  assert_send_command("column_create Books author COLUMN_SCALAR Authors");
  assert_send_command("column_create Bigram author "
                      "COLUMN_INDEX|WITH_POSITION Authors _key");
  assert_send_command("column_create BigramLoose author "
                      "COLUMN_INDEX|WITH_POSITION Authors _key");
  cut_assert_equal_string(
    "2",
    send_command("load "
                 "--table Books "
                 "--columns '_key, author' "
                 "--values "
                 "'["
                 "[\"The groonga book\", \"mori\"],"
                 "[\"The first groonga\", \"morita\"]"
                 "]'"));
  cut_assert_equal_string(
    "[[[2],"
    "[[\"_key\",\"ShortText\"],"
    "[\"author\",\"Authors\"],"
    "[\"_score\",\"Int32\"]],"
    "[\"The groonga book\",\"mori\",11],"
    "[\"The first groonga\",\"morita\",1]]]",
    send_command("select Books "
                 "--match_columns "
                 "'Bigram.author * 10 || BigramLoose.author * 1' "
                 "--query 'mori' "
                 "--output_columns '_key, author, _score'"));
}

void
test_invalid_start_with_symbol(void)
{
  const gchar *table_list_result;

  table_list_result =
    cut_take_printf("["
                    "[[\"id\",\"UInt32\"],"
                     "[\"name\",\"ShortText\"],"
                     "[\"path\",\"ShortText\"],"
                     "[\"flags\",\"ShortText\"],"
                     "[\"domain\",\"ShortText\"],"
                     "[\"range\",\"ShortText\"],"
                     "[\"default_tokenizer\",\"ShortText\"],"
                     "[\"normalizer\",\"ShortText\"]],"
                    "[256,"
                     "\"Authors\","
                     "\"%s.0000100\","
                     "\"TABLE_PAT_KEY|PERSISTENT\","
                     "\"ShortText\","
                     "null,"
                     "null,"
                     "null]]",
                    database_path);

  assert_send_command("table_create Authors TABLE_PAT_KEY ShortText");
  cut_assert_equal_string(table_list_result, send_command("table_list"));
  grn_test_assert_send_command_error(context,
                                     GRN_INVALID_ARGUMENT,
                                     "JSON must start with '[' or '{': <invalid>",
                                     "load "
                                     "--table Authors "
                                     "--columns '_key' "
                                     "--values 'invalid'");
  cut_assert_equal_string(table_list_result, send_command("table_list"));
}

void
test_nonexistent_table_name(void)
{
  grn_test_assert_send_command_error(context,
                                     GRN_INVALID_ARGUMENT,
                                     "nonexistent table: <Users>",
                                     "load --table Users\n"
                                     "[\n"
                                     "[\"_key\": \"alice\"]\n"
                                     "]");
}

void
test_invalid_table_name(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][load] name can't start with '_' "
    "and contains only 0-9, A-Z, a-z, #, @, - or _: <_Users>",
    "load --table _Users\n"
    "[\n"
    "{\"_key\": \"alice\"}\n"
    "]");
}


void
test_vector_reference_column(void)
{
  assert_send_command("table_create Tags TABLE_HASH_KEY ShortText "
                      "--default_tokenizer TokenDelimit");
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users tags COLUMN_VECTOR Tags");
  cut_assert_equal_string(
    "1",
    send_command(
      "load "
      "--table Users "
      "--values '[{\"_key\": \"mori\", \"tags\": \"groonga search engine\"}]'"));
  cut_assert_equal_string(
    "[[[1],"
     "[[\"_id\",\"UInt32\"],"
      "[\"_key\",\"ShortText\"],"
      "[\"tags\",\"Tags\"]],"
     "[1,\"mori\",[\"groonga\",\"search\",\"engine\"]]]]",
    send_command("select Users"));
}

void
test_vector_domain(void)
{
  assert_send_command("table_create Posts TABLE_NO_KEY");
  assert_send_command("column_create Posts tags COLUMN_VECTOR ShortText");
  cut_assert_equal_string(
    "1",
    send_command(
      "load "
      "--table Posts "
      "--values '[{\"tags\": [\"groonga\", \"search engine\"]}]'"));

  {
    grn_obj *tags_column;
    grn_obj tags;
    const char *tag;
    grn_id domain;

    tags_column = grn_ctx_get(context, "Posts.tags", strlen("Posts.tags"));
    GRN_SHORT_TEXT_INIT(&tags, GRN_OBJ_VECTOR);
    grn_obj_get_value(context, tags_column, 1, &tags);
    cut_assert_equal_int(2, grn_vector_size(context, &tags));
    grn_vector_get_element(context, &tags, 0, &tag, NULL, &domain);
    GRN_OBJ_FIN(context, &tags);
    grn_obj_unlink(context, tags_column);

    grn_test_assert_equal_id(context, GRN_DB_SHORT_TEXT, domain);
  }
}
