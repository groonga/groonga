/*
  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_store.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_simple(void);
void test_with_long_path(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static gchar *path;
static grn_db_create_optarg *options;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "function-cast",
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

  path = NULL;
  options = NULL;
}

void
cut_teardown(void)
{
  if (path) {
    g_free(path);
  }

  if (options) {
    g_free(options);
  }

  if (database) {
    grn_obj_close(context, database);
  }

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_create_simple(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_context(context);
  cut_assert_not_null(database);
}

void
test_create_with_long_path(void)
{
  gssize max_path = PATH_MAX - 14;
  GString *long_path;
  const gchar last_component[] = G_DIR_SEPARATOR_S "db";

  if (getenv("TRAVIS")) {
    cut_omit("It is crashed on Travis CI. Why?");
  }

  long_path = grn_long_path_new(tmp_directory,
                                max_path - strlen(last_component));
  g_mkdir_with_parents(long_path->str, 0700);
  g_string_append(long_path, last_component);

  path = g_string_free(long_path, FALSE);
  database = grn_db_create(context, path, NULL);
  grn_test_assert_context(context);
  cut_assert_not_null(database);
  grn_obj_close(context, database);
  database = NULL;

  long_path = g_string_new(path);
  g_free(path);
  g_string_append(long_path, "X");
  path = g_string_free(long_path, FALSE);
  database = grn_db_create(context, path, NULL);
  grn_test_assert_error(GRN_INVALID_ARGUMENT, "too long path", context);
}
