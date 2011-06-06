/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>

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

void test_invalid_name(void);
void test_missing_name(void);
void test_nonexistent_table(void);
void test_nonexistent_type(void);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "column-create",
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
test_invalid_name(void)
{
  assert_send_command("table_create Users");
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][create]: name can't start with '_' and 0-9, "
    "and contains only 0-9, A-Z, a-z, or _: <_name>",
    "column_create Users _name COLUMN_SCALAR ShortText");
}

void
test_missing_name(void)
{
  assert_send_command("table_create Users");
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][create]: name is missing",
    "column_create Users --flags COLUMN_SCALAR --type ShortText");
}

void
test_nonexistent_table(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][create]: table doesn't exist: <Users>",
    "column_create Users name COLUMN_SCALAR ShortText");
}

void
test_nonexistent_type(void)
{
  assert_send_command("table_create Users");
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[column][create]: type doesn't exist: <VeryShortText>",
    "column_create Users name COLUMN_SCALAR VeryShortText");
}
