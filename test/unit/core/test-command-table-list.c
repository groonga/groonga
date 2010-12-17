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

void test_xml(void);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-list",
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
test_xml(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("table_create Tweets TABLE_NO_KEY");
  cut_assert_equal_string(
      cut_take_printf("<TABLE_LIST>\n"
                      "<HEADER>\n"
                      "<PROPERTY>\n"
                      "<TEXT>id</TEXT>\n"
                      "<TEXT>UInt32</TEXT></PROPERTY>\n"
                      "<PROPERTY>\n"
                      "<TEXT>name</TEXT>\n"
                      "<TEXT>ShortText</TEXT></PROPERTY>\n"
                      "<PROPERTY>\n"
                      "<TEXT>path</TEXT>\n"
                      "<TEXT>ShortText</TEXT></PROPERTY>\n"
                      "<PROPERTY>\n"
                      "<TEXT>flags</TEXT>\n"
                      "<TEXT>ShortText</TEXT></PROPERTY>\n"
                      "<PROPERTY>\n"
                      "<TEXT>domain</TEXT>\n"
                      "<TEXT>ShortText</TEXT></PROPERTY>\n"
                      "<PROPERTY>\n"
                      "<TEXT>range</TEXT>\n"
                      "<TEXT>ShortText</TEXT></PROPERTY></HEADER>\n"
                      "<TABLE>\n"
                      "<INT>%u</INT>\n"
                      "<TEXT>Sites</TEXT>\n"
                      "<TEXT>%s.0000101</TEXT>\n"
                      "<TEXT>TABLE_PAT_KEY|PERSISTENT</TEXT>\n"
                      "<TEXT>ShortText</TEXT>\n"
                      "<TEXT>null</TEXT></TABLE>\n"
                      "<TABLE>\n"
                      "<INT>%u</INT>\n"
                      "<TEXT>Tweets</TEXT>\n"
                      "<TEXT>%s.0000102</TEXT>\n"
                      "<TEXT>TABLE_NO_KEY|PERSISTENT</TEXT>\n"
                      "<TEXT>null</TEXT>\n"
                      "<TEXT>null</TEXT></TABLE>\n"
                      "<TABLE>\n"
                      "<INT>%u</INT>\n"
                      "<TEXT>Users</TEXT>\n"
                      "<TEXT>%s.0000100</TEXT>\n"
                      "<TEXT>TABLE_HASH_KEY|PERSISTENT</TEXT>\n"
                      "<TEXT>ShortText</TEXT>\n"
                      "<TEXT>null</TEXT></TABLE></TABLE_LIST>",
                      grn_obj_id(context, get("Sites")), database_path,
                      grn_obj_id(context, get("Tweets")), database_path,
                      grn_obj_id(context, get("Users")), database_path),
      send_command("table_list --output_type xml"));
}
