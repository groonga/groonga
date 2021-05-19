/*
  Copyright (C) 2012  Kouhei Sutou <kou@clear-code.com>

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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <grn_str.h>

#define get(name) grn_ctx_get(context, name, strlen(name))

void test_success(void);
void test_error_no_argument(void);
void test_error_nonexistent_table_name(void);
void test_error_missing_column_name(void);
void test_error_nonexistent_column_name(void);
void test_error_missing_new_column_name(void);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "column-rename",
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

  database_path = cut_build_path(tmp_directory,
                                 "command-column-rename",
                                 NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

static void
populate(void)
{
  assert_send_commands("table_create Users TABLE_HASH_KEY ShortText\n"
                       "column_create Users name COLUMN_SCALAR ShortText\n"
                       "load --table Users\n"
                       "[\n"
                       "[\"_key\",\"name\"],\n"
                       "[\"morita\",\"Daijiro MORI\"],\n"
                       "[\"yata\",\"Susumu Yata\"]\n"
                       "]");
}

void
test_success(void)
{
  populate();
  assert_send_command("column_rename Users name full_name");
  cut_assert_equal_string(
      "table_create Users TABLE_HASH_KEY ShortText\n"
      "column_create Users full_name COLUMN_SCALAR ShortText\n"
      "\n"
      "load --table Users\n"
      "[\n"
      "[\"_key\",\"full_name\"],\n"
      "[\"morita\",\"Daijiro MORI\"],\n"
      "[\"yata\",\"Susumu Yata\"]\n"
      "]",
      send_command("dump"));
}

void
test_error_no_argument(void)
{
  populate();
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][rename] table name isn't specified",
    "column_rename");
}

void
test_error_nonexistent_table_name(void)
{
  populate();
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][rename] table isn't found: <nonexistent>",
    "column_rename nonexistent");
}

void
test_error_missing_column_name(void)
{
  populate();
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][rename] column name isn't specified: <Users>",
    "column_rename Users");
}

void
test_error_nonexistent_column_name(void)
{
  populate();
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][rename] column isn't found: <Users.nonexistent>",
    "column_rename Users nonexistent");
}

void
test_error_missing_new_column_name(void)
{
  populate();
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][rename] new column name isn't specified: <Users.name>",
    "column_rename Users name");
}
