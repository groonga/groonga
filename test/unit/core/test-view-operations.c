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
#include <db.h>

void test_sort(void);

static grn_logger_info *logger;
static grn_ctx *context;
static grn_obj *database, *entries, *users, *dogs;

static gchar *tmp_directory;
static gchar *database_path;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "test-view",
                                   NULL);
  database_path = g_build_filename(tmp_directory, "database.groonga", NULL);
}

void
cut_shutdown(void)
{
  g_free(database_path);
  g_free(tmp_directory);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

static void
setup_ddl(void)
{
  assert_send_command("table_create Entries TABLE_VIEW");
  assert_send_command("table_create Users --key_type ShortText");
  assert_send_command("table_create Dogs --key_type ShortText");

  entries = get_object("Entries");
  users = get_object("Users");
  dogs = get_object("Dogs");

  grn_view_add(context, entries, users);
  grn_test_assert_context(context);
  grn_view_add(context, entries, dogs);
  grn_test_assert_context(context);
}

static void
setup_data(void)
{
  assert_send_command("load '[[\"_key\"],[\"morita\"],[\"gunyara-kun\"],[\"yu\"]]' "
                      "Users");
  assert_send_command("load '[[\"_key\"],[\"pochi\"],[\"bob\"],[\"taro\"]]' Dogs");
}

static void
setup_database(void)
{
  database = grn_db_create(context, database_path, NULL);
  grn_test_assert_context(context);

  setup_ddl();
  setup_data();
}

void
cut_setup(void)
{
  entries = NULL;
  users = NULL;
  dogs = NULL;

  logger = setup_grn_logger();

  context = g_new(grn_ctx, 1);
  grn_ctx_init(context, 0);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  setup_database();
}

void
cut_teardown(void)
{
  if (entries) {
    grn_obj_unlink(context, entries);
  }

  if (users) {
    grn_obj_unlink(context, users);
  }

  if (dogs) {
    grn_obj_unlink(context, dogs);
  }

  if (database) {
    grn_obj_unlink(context, database);
  }

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  teardown_grn_logger(logger);

  remove_tmp_directory();
}

void
test_sort(void)
{
  grn_obj *result;
  grn_table_sort_key keys[1];
  gint limit, n_records;

  result = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_VIEW, NULL, NULL);
  grn_view_add(context, result,
               grn_table_create(context, NULL, 0, NULL, GRN_TABLE_NO_KEY,
                                NULL, users));
  grn_view_add(context, result,
               grn_table_create(context, NULL, 0, NULL, GRN_TABLE_NO_KEY,
                                NULL, dogs));

  keys[0].key = grn_obj_column(context, entries, "_key", strlen("_key"));
  keys[0].flags = GRN_TABLE_SORT_DESC;
  limit = 2;
  n_records = grn_table_sort(context, entries, 0, limit, result,
                             keys, sizeof(keys[0]) / sizeof(keys));
  grn_test_assert_equal_view(context,
                             gcut_take_new_list_string("yu",
                                                       "taro",
                                                       NULL),
                             result,
                             "_key");
  cut_assert_equal_int(limit, n_records);
}
