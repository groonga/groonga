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

void test_id(void);
void test_filter(void);
void test_key(void);
void test_uint64(void);
void test_last_token(void);
void test_no_key_twice(void);
void test_no_key_by_id(void);
void test_corrupt_jagged_array(void);
void test_no_table_name(void);
void test_nonexistent_table(void);
void test_no_selector(void);
void test_all_selector(void);
void test_key_and_id(void);
void test_key_and_filter(void);
void test_id_and_filter(void);
void test_invalid_id(void);
void test_invalid_filter(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-delete",
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
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Bookmarks TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Bigram TABLE_PAT_KEY ShortText "
                      "--default_tokenizer TokenBigram");

  assert_send_command("column_create Bookmarks user COLUMN_SCALAR Users");
  assert_send_command("column_create Users bookmarks COLUMN_INDEX "
                      "Bookmarks user");
  assert_send_command("column_create Bigram user COLUMN_INDEX|WITH_POSITION "
                      "Users _key");

  assert_send_command("load --table Users --columns '_key'\n"
                      "[\n"
                      "  [\"mori\"],\n"
                      "  [\"tapo\"]\n"
                      "]");
  assert_send_command("load --table Bookmarks --columns '_key, user'\n"
                      "[\n"
                      "  [\"http://groonga.org/\", \"yu\"],\n"
                      "  [\"http://cutter.sourceforge.net/\", \"tasukuchan\"]\n"
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
    grn_obj_close(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_id(void)
{
  assert_send_command("delete Users --id 1");
  cut_assert_equal_string("[[[3],"
                            "[[\"_key\",\"ShortText\"]],"
                            "[\"tapo\"],"
                            "[\"yu\"],"
                            "[\"tasukuchan\"]]]",
                          send_command("select Users "
                                       "--output_columns _key"));
}

void
test_filter(void)
{
  assert_send_command("delete Users --filter "
                        "\"_key == \\\"tapo\\\"\"");
  cut_assert_equal_string("[[[3],"
                            "[[\"_key\",\"ShortText\"]],"
                            "[\"mori\"],"
                            "[\"yu\"],"
                            "[\"tasukuchan\"]]]",
                          send_command("select Users "
                                       "--output_columns _key"));
}

void
test_key(void)
{
  assert_send_command("delete Users tapo");
  cut_assert_equal_string("[[[3],"
                            "[[\"_key\",\"ShortText\"]],"
                            "[\"mori\"],"
                            "[\"yu\"],"
                            "[\"tasukuchan\"]]]",
                          send_command("select Users "
                                       "--output_columns _key"));
}

void
test_uint64(void)
{
  assert_send_command("table_create Students TABLE_HASH_KEY UInt64");
  assert_send_command("load --table Students --columns '_key'\n"
                      "[\n"
                      "  [29],\n"
                      "  [2929]\n"
                      "]");
  assert_send_command("delete Students 2929");
  cut_assert_equal_string("[[[1],"
                            "[[\"_key\",\"UInt64\"]],"
                            "[29]]]",
                          send_command("select Students "
                                       "--output_columns _key"));
}

void
test_last_token(void)
{
  cut_assert_equal_string("[[[2],"
                            "[[\"_key\",\"ShortText\"]],"
                            "[\"mori\"],"
                            "[\"tapo\"]]]",
                          send_command("select Users "
                                       "--match_columns _key "
                                       "--query o "
                                       "--output_columns _key"));
  assert_send_command("delete Users tapo");
  cut_assert_equal_string("[[[1],"
                            "[[\"_key\",\"ShortText\"]],"
                            "[\"mori\"]]]",
                          send_command("select Users "
                                       "--match_columns _key "
                                       "--query o "
                                       "--output_columns _key"));
}

void
test_no_key_twice(void)
{
  assert_send_command("table_create Sites TABLE_NO_KEY");
  assert_send_command("column_create Sites title COLUMN_SCALAR ShortText");
  assert_send_command("delete Sites --id 999");
  assert_send_command("delete Sites --id 999");
  cut_assert_equal_string("3",
                          send_command("load --table Sites\n"
                                       "[\n"
                                       "{\"title\": \"groonga\"},\n"
                                       "{\"title\": \"Ruby\"},\n"
                                       "{\"title\": \"Cutter\"}\n"
                                       "]"));
  cut_assert_equal_string("[[[3],"
                           "[[\"_id\",\"UInt32\"],"
                            "[\"title\",\"ShortText\"]],"
                           "[1,\"groonga\"],"
                           "[2,\"Ruby\"],"
                           "[3,\"Cutter\"]]]",
                          send_command("select Sites"));
}

void
test_no_key_by_id(void)
{
  assert_send_command("table_create Sites TABLE_NO_KEY");
  assert_send_command("column_create Sites title COLUMN_SCALAR ShortText");
  cut_assert_equal_string("3",
                          send_command("load --table Sites\n"
                                       "[\n"
                                       "{\"title\": \"groonga\"},\n"
                                       "{\"title\": \"Ruby\"},\n"
                                       "{\"title\": \"Cutter\"}\n"
                                       "]"));
  assert_send_command("delete Sites --id 2");
  cut_assert_equal_string("[[[2],"
                           "[[\"_id\",\"UInt32\"],"
                            "[\"title\",\"ShortText\"]],"
                           "[1,\"groonga\"],"
                           "[3,\"Cutter\"]]]",
                          send_command("select Sites"));
}

void
test_corrupt_jagged_array(void)
{
  const gchar *text_65bytes =
    "65bytes text "
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  const gchar *text_129bytes =
    "129bytes text "
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

  assert_send_command("table_create Sites TABLE_NO_KEY");
  assert_send_command("column_create Sites description COLUMN_SCALAR ShortText");
  cut_assert_equal_string(
    "1",
    send_command(cut_take_printf("load --table Sites\n"
                                 "[[\"description\"],\n"
                                 "[\"%s\"]\n"
                                 "]",
                                 text_129bytes)));
  assert_send_command("delete Sites --id 1");

  cut_assert_equal_string(
    "3",
    send_command(cut_take_printf("load --table Sites\n"
                                 "[[\"description\"],\n"
                                 "[\"%s\"],\n"
                                 "[\"%s\"],\n"
                                 "[\"%s\"]"
                                 "]",
                                 text_65bytes,
                                 text_65bytes,
                                 text_129bytes)));
  cut_assert_equal_string(
    cut_take_printf("[[[3],"
                    "[[\"_id\",\"UInt32\"],"
                     "[\"description\",\"ShortText\"]],"
                     "[1,\"%s\"],"
                     "[2,\"%s\"],"
                     "[3,\"%s\"]]]",
                    text_65bytes,
                    text_65bytes,
                    text_129bytes),
                    send_command("select Sites"));
}

void
test_no_table_name(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] table name isn't specified",
    "delete");
}

void
test_nonexistent_table(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] table doesn't exist: <nonexistent>",
    "delete nonexistent");
}

void
test_no_selector(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] either key, id or filter must be specified: "
    "table: <Users>",
    "delete Users");
}

void
test_all_selector(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] "
    "record selector must be one of key, id and filter: "
    "table: <Users>, key: <mori>, id: <1>, filter: <true>",
    "delete Users --key mori --id 1 --filter \"true\"");
}

void
test_key_and_id(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] can't use both key and id: "
    "table: <Users>, key: <mori>, id: <1>",
    "delete Users --key mori --id 1");
}

void
test_key_and_filter(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] can't use both key and filter: "
    "table: <Users>, key: <mori>, filter: <true>",
    "delete Users --key mori --filter true");
}

void
test_id_and_filter(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] can't use both id and filter: "
    "table: <Users>, id: <1>, filter: <true>",
    "delete Users --id 1 --filter true");
}

void
test_invalid_id(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][record][delete] id should be number: "
    "table: <Users>, id: <1x2>, detail: <1|x|2>",
    "delete Users --id \"1x2\"");
}

void
test_invalid_filter(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_SYNTAX_ERROR,
    "[table][record][delete] failed to parse filter: "
    "table: <Users>, filter: <$>, detail: <Syntax error: <$||>: "
    "[expr][parse] unknow",
    "delete Users --filter \"$\"");
}
