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

void test_hash_key(void);
void test_pat_key(void);
void test_dat_key(void);
void test_no_key(void);

void test_invalid_name(void);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-create",
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

static void
grn_test_assert_users_table_type(unsigned char type)
{
  static const char *users = "Users";
  grn_obj *object = grn_ctx_get(context, users, strlen(users));
  cut_assert_equal_int(type, object->header.type);
}

void
test_hash_key(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  grn_test_assert_users_table_type(GRN_TABLE_HASH_KEY);
}

void
test_pat_key(void)
{
  assert_send_command("table_create Users TABLE_PAT_KEY ShortText");
  grn_test_assert_users_table_type(GRN_TABLE_PAT_KEY);
}

void
test_dat_key(void)
{
  assert_send_command("table_create Users TABLE_DAT_KEY ShortText");
  grn_test_assert_users_table_type(GRN_TABLE_DAT_KEY);
}

void
test_no_key(void)
{
  assert_send_command("table_create Users TABLE_NO_KEY");
  grn_test_assert_users_table_type(GRN_TABLE_NO_KEY);
}

void
test_invalid_name(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "[table][create]: name can't start with '_' and "
    "contains only 0-9, A-Z, a-z, #, - or _: <_Users>",
    "table_create _Users");
}
