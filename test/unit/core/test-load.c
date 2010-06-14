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
void test_bool(void);
void attributes_bool_0(void);
void test_bool_0(void);

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
  cut_set_attributes("bug", "304",
                     NULL);
}

void
test_bool(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users enabled COLUMN_SCALAR Bool");
  cut_assert_equal_string(
    "2",
    send_command("load --table Users --columns '_key,enabled'\n"
                 "[\n"
                 "  [\"mori\",true],\n"
                 "  [\"tapo\",false]\n"
                 "]"));
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
attributes_bool_0(void)
{
  cut_set_attributes("bug", "123",
                     NULL);
}

void
test_bool_0(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users enabled COLUMN_SCALAR Bool");
  cut_assert_equal_string(
    "2",
    send_command("load --table Users --columns '_key,enabled'\n"
                 "[\n"
                 "  [\"mori\",1],\n"
                 "  [\"tapo\",0]\n"
                 "]"));
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
