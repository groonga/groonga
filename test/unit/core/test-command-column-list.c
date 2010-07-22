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

#define get(name) grn_ctx_get(context, name, strlen(name))

void test_columns(void);
void test_index_columns(void);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "column-list",
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
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_columns(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt32");
  assert_send_command("column_create Users comment COLUMN_SCALAR Text");
  cut_assert_equal_string(
      cut_take_printf("["
                      "["
                      "[\"id\",\"UInt32\"],"
                      "[\"name\",\"ShortText\"],"
                      "[\"path\",\"ShortText\"],"
                      "[\"type\",\"ShortText\"],"
                      "[\"flags\",\"ShortText\"],"
                      "[\"domain\",\"ShortText\"],"
                      "[\"range\",\"ShortText\"],"
                      "[\"source\",\"ShortText\"]"
                      "],"
                      "[%d,"
                      "\"_key\","
                      "\"\","
                      "\"\","
                      "\"COLUMN_SCALAR|COMPRESS_NONE\","
                      "\"Users\","
                      "\"ShortText\","
                      "[]"
                      "],"
                      "[%d,"
                      "\"comment\","
                      "\"%s.0000102\","
                      "\"var\","
                      "\"COLUMN_SCALAR|COMPRESS_NONE|PERSISTENT\","
                      "\"Users\","
                      "\"Text\","
                      "[]"
                      "],"
                      "[%d,"
                      "\"age\","
                      "\"%s.0000101\","
                      "\"fix\","
                      "\"COLUMN_SCALAR|COMPRESS_NONE|PERSISTENT\","
                      "\"Users\","
                      "\"UInt32\","
                      "[]"
                      "]"
                      "]",
                      grn_obj_id(context, get("Users")),
                      grn_obj_id(context, get("Users.comment")), database_path,
                      grn_obj_id(context, get("Users.age")), database_path),
      send_command("column_list Users"));
}

void
test_index_columns(void)
{
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Terms TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Terms Sites_key "
                      "COLUMN_INDEX|WITH_POSITION Sites _key");
  assert_send_command("load '[[\"_key\"],[\"groonga.org\"]]' Sites");
  cut_assert_equal_string(
      cut_take_printf("["
                      "["
                      "[\"id\",\"UInt32\"],"
                      "[\"name\",\"ShortText\"],"
                      "[\"path\",\"ShortText\"],"
                      "[\"type\",\"ShortText\"],"
                      "[\"flags\",\"ShortText\"],"
                      "[\"domain\",\"ShortText\"],"
                      "[\"range\",\"ShortText\"],"
                      "[\"source\",\"ShortText\"]"
                      "],"
                      "[%d,"
                      "\"_key\","
                      "\"\","
                      "\"\","
                      "\"COLUMN_SCALAR|COMPRESS_NONE\","
                      "\"Terms\","
                      "\"ShortText\","
                      "[]"
                      "],"
                      "[%d,"
                      "\"Sites_key\","
                      "\"%s.0000102\","
                      "\"index\",\"COLUMN_INDEX|WITH_POSITION|COMPRESS_NONE|PERSISTENT\","
                      "\"Terms\","
                      "\"Sites\","
                      "[\"Sites\"]"
                      "]"
                      "]",
                      grn_obj_id(context, get("Terms")),
                      grn_obj_id(context, get("Terms.Sites_key")),
                      database_path),
      send_command("column_list Terms"));
}
