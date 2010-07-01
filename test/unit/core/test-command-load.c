/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>

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

#include <str.h>

void test_columns(void);
void attributes_bool(void);
void data_bool(void);
void test_bool(gconstpointer data);
void test_int32_key(void);
void data_null(void);
void test_null(gconstpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
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
                          "[\"name\",\"ShortText\"],"
                          "[\"desc\",\"ShortText\"]"
                          "],"
                          "[1,\"mori\",\"モリ\",\"タポ\"],"
                          "[2,\"tapo\",\"タポ\",\"モリモリモリタポ\"]"
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
               "[\"scores\",\"Int32\"],"
               "[\"nick\",\"ShortText\"]],"
             "[1,\"Daijiro MORI\",[5,5,5],\"\"]]]",
            "load --table Students --columns '_key, nick'\n"
            "[\n"
            "  [\"Daijiro MORI\", null]\n"
            "]");
  ADD_DATUM("string - empty string",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"scores\",\"Int32\"],"
               "[\"nick\",\"ShortText\"]],"
             "[1,\"Daijiro MORI\",[5,5,5],\"\"]]]",
            "load --table Students --columns '_key, nick'\n"
            "[\n"
            "  [\"Daijiro MORI\", \"\"]\n"
            "]");

  ADD_DATUM("vector - empty null",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"scores\",\"Int32\"],"
               "[\"nick\",\"ShortText\"]],"
             "[1,\"Daijiro MORI\",[],\"morita\"]]]",
            "load --table Students --columns '_key, scores'\n"
            "[\n"
            "  [\"Daijiro MORI\", null]\n"
            "]");
  ADD_DATUM("vector - empty string",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"scores\",\"Int32\"],"
               "[\"nick\",\"ShortText\"]],"
             "[1,\"Daijiro MORI\",[],\"morita\"]]]",
            "load --table Students --columns '_key, scores'\n"
            "[\n"
            "  [\"Daijiro MORI\", \"\"]\n"
            "]");
  ADD_DATUM("vector - empty array",
            "[[[1],"
              "[[\"_id\",\"UInt32\"],"
               "[\"_key\",\"ShortText\"],"
               "[\"scores\",\"Int32\"],"
               "[\"nick\",\"ShortText\"]],"
             "[1,\"Daijiro MORI\",[],\"morita\"]]]",
            "load --table Students --columns '_key, scores'\n"
            "[\n"
            "  [\"Daijiro MORI\", []]\n"
            "]");

#undef ADD_DATUM
}

void
test_null(gconstpointer data)
{
  cut_omit("not implemented yet");

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
