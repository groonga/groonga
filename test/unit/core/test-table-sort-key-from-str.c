/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009-2010  Nobuyoshi Nakada <nakada@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_valid(void);
void test_valid(gconstpointer data);
void data_invalid(void);
void test_invalid(gconstpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;
static grn_obj *table;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-sort-key-from-str",
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

  assert_send_command("table_create Test TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Test name COLUMN_SCALAR Text");
  table = get_object("Test");
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
data_valid(void)
{
#define ADD_DATUM(str, count)                   \
  gcut_add_datum("[" str "] == " #count,        \
                 "keys", G_TYPE_STRING, str,    \
                 "count", G_TYPE_UINT, count,   \
                 NULL)

  ADD_DATUM("name", 1);
  ADD_DATUM(" name ", 1);
  ADD_DATUM("_key", 1);
  ADD_DATUM("_key, name", 2);
  ADD_DATUM(" _key, name ", 2);
  ADD_DATUM("+name", 1);
  ADD_DATUM(" +name ", 1);
  ADD_DATUM("+_key", 1);
  ADD_DATUM("+_key, +name", 2);
  ADD_DATUM(" +_key, +name ", 2);
  ADD_DATUM("-name", 1);
  ADD_DATUM(" -name ", 1);
  ADD_DATUM("-_key", 1);
  ADD_DATUM("-_key, -name", 2);
  ADD_DATUM(" -_key, -name ", 2);

#undef ADD_DATUM
}

void
test_valid(gconstpointer data)
{
  unsigned i, nkeys;
  const char *str = gcut_data_get_string(data, "keys");
  grn_table_sort_key *keys = grn_table_sort_key_from_str(context, str, strlen(str),
                                                         table, &nkeys);
  cut_assert_not_null(keys);
  cut_assert_equal_uint(gcut_data_get_uint(data, "count"), nkeys);
  for (i = 0; i < nkeys; ++i) {
    cut_assert_not_null(keys[i].key);
  }
}

void
data_invalid(void)
{
#define ADD_DATUM(str)                          \
  gcut_add_datum("[" str "] is invalid",        \
                 "keys", G_TYPE_STRING, str,    \
                 NULL)

  ADD_DATUM("foo");
  ADD_DATUM("_key, foo");
  ADD_DATUM("foo, name");
  ADD_DATUM("name, foo");
  ADD_DATUM("+ name");
  ADD_DATUM("- name");

#undef ADD_DATUM
}

void
test_invalid(gconstpointer data)
{
  unsigned nkeys;
  const char *str = gcut_data_get_string(data, "keys");
  grn_table_sort_key *keys = grn_table_sort_key_from_str(context, str, strlen(str),
                                                         table, &nkeys);
  cut_assert_null(keys);
  cut_assert_equal_uint(0, nkeys);
}
