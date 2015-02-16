/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2011-2015  Kouhei Sutou <kou@clear-code.com>

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

#include "../../../config.h"

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_is_builtin(void);
void test_is_builtin(gconstpointer data);
void data_is_table(void);
void test_is_table(gconstpointer data);
void data_is_function_proc(void);
void test_is_function_proc(gconstpointer data);
void data_is_selector_proc(void);
void test_is_selector_proc(gconstpointer data);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "object",
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
data_is_builtin(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ? "built-in - " name : "custom - " name),    \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "TokenBigram");
#ifdef GRN_WITH_MECAB
  ADD_DATUM(TRUE, "TokenMecab");
#endif
  ADD_DATUM(FALSE, "Users");
  ADD_DATUM(FALSE, "Users.name");
  ADD_DATUM(FALSE, "suggest");

#undef ADD_DATUM
}

void
test_is_builtin(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("register suggest/suggest");
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_obj_is_builtin(context, object));
  } else {
    cut_assert_false(grn_obj_is_builtin(context, object));
  }
}

void
data_is_table(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ? "table - " name : "column - " name),       \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "Users");
  ADD_DATUM(FALSE, "Users.name");

#undef ADD_DATUM
}

void
test_is_table(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_obj_is_table(context, object));
  } else {
    cut_assert_false(grn_obj_is_table(context, object));
  }
}

void
data_is_function_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "function-proc - " name :                             \
                  "not function-proc - " name),                         \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "now");
  ADD_DATUM(FALSE, "status");

#undef ADD_DATUM
}

void
test_is_function_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_obj_is_function_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_function_proc(context, object));
  }
}

void
data_is_selector_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "selector-proc - " name :                             \
                  "not selector-proc - " name),                         \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "geo_in_circle");
  ADD_DATUM(FALSE, "now");

#undef ADD_DATUM
}

void
test_is_selector_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_obj_is_selector_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_selector_proc(context, object));
  }
}
