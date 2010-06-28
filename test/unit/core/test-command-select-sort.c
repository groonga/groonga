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

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_int(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "command-select-sort",
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
test_int(void)
{
  assert_send_commands("table_create Sites TABLE_PAT_KEY ShortText\n"
                       "column_create Sites score COLUMN_SCALAR Int32\n"
                       "column_create Sites age COLUMN_SCALAR Int32\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\", \"score\", \"age\"],\n"
                       "[\"groonga.org\", 100, 2],\n"
                       "[\"qwik.jp/senna/FrontPageJ.html\", 100, 5],\n"
                       "[\"2ch.net\", 10, 11]\n"
                       "]");
  cut_assert_equal_string(
      "[[[3],"
       "[[\"_key\",\"ShortText\"],"
        "[\"score\",\"Int32\"],"
        "[\"age\",\"Int32\"]],"
       "[\"qwik.jp/senna/FrontPageJ.html\",100,5],"
       "[\"groonga.org\",100,2],"
       "[\"2ch.net\",10,11]]]",
      send_command("select Sites "
                   "--sortby \"-score -age\" "
                   "--output_columns \"_key score age\""));
}
