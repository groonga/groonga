/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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

void test_domain(void);
void test_range(void);
void test_cursor(void);
void test_get_persistent_object_from_opened_database(void);
void test_recreate_temporary_object_on_opened_database(void);
void test_size(void);

static gchar *tmp_directory;

static grn_ctx *context, *context2;
static grn_obj *database, *database2;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "test-database",
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
  database = NULL;

  context2 = g_new0(grn_ctx, 1);
  grn_ctx_init(context2, 0);
  database2 = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  if (context2) {
    grn_ctx_fin(context2);
    g_free(context2);
  }

  remove_tmp_directory();
}

void
test_domain(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_nil(database->header.domain);
}

void
test_range(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_nil(grn_obj_get_range(context, database));
}

void
test_cursor(void)
{
  grn_table_cursor *c;
  database = grn_db_create(context, NULL, NULL);
  c = grn_table_cursor_open(context, database, NULL, 0, NULL, 0, 0, -1, 0);
  cut_assert_true(grn_table_cursor_next(context, c));
  grn_table_cursor_close(context, c);
}

void
test_get_persistent_object_from_opened_database(void)
{
  const gchar table_name[] = "Users";
  const gchar *path;

  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, path, NULL);
  grn_test_assert_not_null(context, database);
  grn_test_assert_not_null(context,
                           grn_table_create(context,
                                            table_name,
                                            strlen(table_name),
                                            NULL,
                                            GRN_OBJ_TABLE_HASH_KEY |
                                            GRN_OBJ_PERSISTENT,
                                            grn_ctx_at(context, GRN_DB_UINT32),
                                            NULL));

  database2 = grn_db_open(context2, path);
  grn_test_assert_not_null(context2, database2);
  grn_test_assert_not_null(context2,
                           grn_ctx_get(context2,
                                       table_name,
                                       strlen(table_name)));
}

void
test_recreate_temporary_object_on_opened_database(void)
{
  const gchar table_name[] = "<users>";
  const gchar *path;

  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, path, NULL);
  grn_test_assert_not_null(context, database);
  grn_test_assert_null(context,
                           grn_table_create(context,
                                            table_name,
                                            strlen(table_name),
                                            NULL,
                                            GRN_OBJ_TABLE_HASH_KEY,
                                            grn_ctx_at(context, GRN_DB_UINT32),
                                            NULL));

  database2 = grn_db_open(context2, path);
  grn_test_assert_not_null(context2, database2);
  grn_test_assert_null(context2,
                       grn_ctx_get(context2,
                                   table_name,
                                   strlen(table_name)));
  grn_test_assert_null(context2,
                           grn_table_create(context,
                                            table_name,
                                            strlen(table_name),
                                            NULL,
                                            GRN_OBJ_TABLE_HASH_KEY,
                                            grn_ctx_at(context, GRN_DB_UINT32),
                                            NULL));
}

void
test_size(void)
{
  guint n_builtin_objects = 255;
  const gchar table_name[] = "bookmarks";

  database = grn_db_create(context, NULL, NULL);

  cut_assert_equal_uint(n_builtin_objects, grn_table_size(context, database));
  grn_test_assert_context(context);

  grn_table_create(context,
                   table_name,
                   strlen(table_name),
                   NULL,
                   GRN_OBJ_TABLE_HASH_KEY,
                   grn_ctx_at(context, GRN_DB_UINT32),
                   NULL);
  cut_assert_equal_uint(n_builtin_objects + 1,
                        grn_table_size(context, database));
  grn_test_assert_context(context);
}
