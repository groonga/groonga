/*
  Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>

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
#include <grn_db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_old_value_zero(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *table, *column, *index_table, *result;
static grn_table_cursor *cursor;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-hook",
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
  table = NULL;
  column = NULL;
  index_table = NULL;
  result = NULL;
  cursor = NULL;
}

void
cut_teardown(void)
{
  if (cursor) {
    grn_obj_unlink(context, cursor);
  }

  if (result) {
    grn_obj_unlink(context, result);
  }

  if (column) {
    grn_obj_unlink(context, column);
  }

  if (table) {
    grn_obj_unlink(context, table);
  }

  if (index_table) {
    grn_obj_unlink(context, index_table);
  }

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

static void
create_int32_table(const gchar *load_data)
{
  const char *table_name = "Data";
  const char *column_name = "number";
  const char *index_table_name = "Index";

  assert_send_commands(
    cut_take_printf("table_create %s TABLE_NO_KEY", table_name));
  assert_send_commands(
    cut_take_printf("column_create %s %s COLUMN_SCALAR Int32",
                    table_name, column_name));
  assert_send_commands(
    cut_take_printf("table_create %s TABLE_PAT_KEY Int32",
                    index_table_name));
  assert_send_commands(
    cut_take_printf("column_create %s %s_%s COLUMN_INDEX %s %s",
                    index_table_name,
                    table_name, column_name,
                    table_name, column_name));
  assert_send_commands(
    cut_take_printf("load --table %s\n"
                    "[\n"
                    " [\"%s\"],\n"
                    "%s\n"
                    "]",
                    table_name,
                    column_name,
                    load_data));

  table = grn_ctx_get(context, table_name, strlen(table_name));
  column = grn_obj_column(context, table, column_name, strlen(column_name));
  index_table = grn_ctx_get(context, index_table_name, strlen(index_table_name));
}

static GList *
int_list_new(gint n, gint value, ...)
{
  GList *list = NULL;
  va_list args;
  gint i;

  va_start(args, value);
  for (i = 0; i < n; i++) {
    list = g_list_prepend(list, GINT_TO_POINTER(value));
    value = va_arg(args, gint);
  }
  va_end(args);

  return g_list_reverse(list);
}

void
test_old_value_zero(void)
{
  grn_id id;
  GList *expected_keys, *actual_keys = NULL;

  cut_omit("null value support is required.");
  create_int32_table(" [0],\n"
                     " [1],\n"
                     " [2],\n"
                     " [3],\n"
                     " [4]");

  cursor = grn_table_cursor_open(context, index_table,
                                 NULL, 0, NULL, 0, 0, -1,
                                 GRN_CURSOR_ASCENDING);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    gint32 *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, GINT_TO_POINTER(*key));
  }
  gcut_take_list(actual_keys, NULL);

  expected_keys = int_list_new(5, 0, 1, 2, 3, 4);
  gcut_take_list(expected_keys, NULL);
  gcut_assert_equal_list_int(expected_keys, actual_keys);
}
