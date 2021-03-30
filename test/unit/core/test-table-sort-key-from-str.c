/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009-2010  Nobuyoshi Nakada <nakada@clear-code.com>
  Copyright (C) 2021  Sutou Kouhei <nakada@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_valid(void);
void test_valid(gconstpointer data);
void data_partial_invalid(void);
void test_partial_invalid(gconstpointer data);
void data_all_invalid(void);
void test_all_invalid(gconstpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;
static grn_obj *table;

static unsigned n_keys;
static grn_table_sort_key *keys;

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

  n_keys = 0;
  keys = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    if (keys)
      grn_table_sort_key_close(context, keys, n_keys);
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
  unsigned i;
  const char *str = gcut_data_get_string(data, "keys");

  keys = grn_table_sort_key_from_str(context, str, strlen(str), table, &n_keys);
  cut_assert_not_null(keys);
  cut_assert_equal_uint(gcut_data_get_uint(data, "count"), n_keys);
  for (i = 0; i < n_keys; ++i) {
    cut_assert_not_null(keys[i].key);
  }
}

void
data_partial_invalid(void)
{
#define ADD_DATUM(str, count)                   \
  gcut_add_datum("[" str "] == " #count,        \
                 "keys", G_TYPE_STRING, str,    \
                 "count", G_TYPE_UINT, count,   \
                 NULL)

  ADD_DATUM("_key, foo", 1);
  ADD_DATUM("foo, name", 1);
  ADD_DATUM("name, foo", 1);
  ADD_DATUM("+ name", 1);
  ADD_DATUM("- name", 1);

#undef ADD_DATUM
}

void
test_partial_invalid(gconstpointer data)
{
  unsigned i;
  const char *str = gcut_data_get_string(data, "keys");
  keys = grn_table_sort_key_from_str(context, str, strlen(str),
                                     table, &n_keys);
  cut_assert_not_null(keys);
  cut_assert_equal_uint(gcut_data_get_uint(data, "count"), n_keys);
  for (i = 0; i < n_keys; ++i) {
    cut_assert_not_null(keys[i].key);
  }
}

void
data_all_invalid(void)
{
#define ADD_DATUM(str)                          \
  gcut_add_datum("[" str "] is invalid",        \
                 "keys", G_TYPE_STRING, str,    \
                 NULL)

  ADD_DATUM("foo");

#undef ADD_DATUM
}

void
test_all_invalid(gconstpointer data)
{
  const char *str = gcut_data_get_string(data, "keys");
  keys = grn_table_sort_key_from_str(context, str, strlen(str),
                                     table, &n_keys);
  cut_assert_equal_uint(0, n_keys);
}
