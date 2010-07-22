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

void data_create(void);
void test_create(gconstpointer data);
void test_add(void);

static grn_logger_info *logger;
static grn_ctx *context;
static grn_obj *database, *view;
static grn_obj *entries, *users, *dogs;

static gchar *tmp_directory;
static gchar *database_path;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
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

void
cut_setup(void)
{
  view = NULL;
  entries = NULL;
  users = NULL;
  dogs = NULL;

  logger = setup_grn_logger();

  context = g_new(grn_ctx, 1);
  grn_ctx_init(context, 0);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  database = grn_db_create(context, database_path, NULL);
  grn_test_assert_context(context);
}

static void
teardown_database(void)
{
  if (view) {
    grn_obj_unlink(context, view);
  }

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
}

void
cut_teardown(void)
{
  teardown_database();

  teardown_grn_logger(logger);

  remove_tmp_directory();
}

void
data_create(void)
{
#define ADD_DATA(label, name, path)             \
  gcut_add_datum(label,                         \
                 "name", G_TYPE_STRING, name,   \
                 "path", G_TYPE_STRING, path,   \
                 NULL)

  ADD_DATA("anonymous", NULL, NULL);
  ADD_DATA("named", "Entries", NULL);
  ADD_DATA("named - explicit path", "Entries",
           cut_build_path(tmp_directory, "view.db", NULL));

#undef ADD_DATA
}

void
test_create(gconstpointer data)
{
  const gchar *name, *path;
  grn_obj_flags flags = GRN_OBJ_TABLE_VIEW;

  name = gcut_data_get_string(data, "name");
  path = gcut_data_get_string(data, "path");

  if (name || path) {
    flags |= GRN_OBJ_PERSISTENT;
  }

  view = grn_table_create(context,
                          name, name ? strlen(name) : 0,
                          path,
                          flags,
                          NULL, NULL);
  grn_test_assert_not_null(context, view);
}

void
test_add(void)
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

  cut_assert_equal_uint(0, grn_table_size(context, entries));
  assert_send_command("load '[[\"_key\"],[\"morita\"]]' Users");
  cut_assert_equal_uint(1, grn_table_size(context, entries));
  assert_send_command("load '[[\"_key\"],[\"pochi\"]]' Dogs");
  cut_assert_equal_uint(2, grn_table_size(context, entries));
}
