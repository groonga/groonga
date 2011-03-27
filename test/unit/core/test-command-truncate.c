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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_no_columns(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-truncate",
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
test_no_columns(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("load --table Users\n"
                      "[\n"
                      "{\"_key\":\"mori\"}\n"
                      "{\"_key\":\"gunyara-kun\"}\n"
                      "{\"_key\":\"yu\"}\n"
                      "]");
  cut_assert_equal_string(
      "[[[3],"
       "[[\"_id\",\"UInt32\"],[\"_key\",\"ShortText\"]],"
       "[1,\"mori\"],"
       "[2,\"gunyara-kun\"],"
       "[3,\"yu\"]]]",
    send_command("select Users"));
  assert_send_command("truncate Users");
  cut_assert_equal_string(
      "[[[0],"
       "[[\"_id\",\"UInt32\"],[\"_key\",\"ShortText\"]]"
       "]]",
    send_command("select Users"));
}
