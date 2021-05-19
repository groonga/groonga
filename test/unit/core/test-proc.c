/*
  Copyright (C) 2013  Kouhei Sutou <kou@clear-code.com>

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

void data_path(void);
void test_path(gconstpointer data);

static gchar *tmp_directory;

static grn_logger_info *logger;
static grn_ctx *context;
static grn_obj *database, *proc;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-proc",
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

  context = NULL;
  logger = setup_grn_logger();

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database = grn_db_create(context,
                           cut_build_path(tmp_directory, "proc.db", NULL),
                           NULL);

  proc = NULL;
}

void
cut_teardown(void)
{
  if (proc) {
    grn_obj_unlink(context, proc);
  }

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  teardown_grn_logger(logger);
  cut_remove_path(tmp_directory, NULL);
}

void
data_path(void)
{
#define ADD_DATA(label, expected, name)                 \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_STRING, expected,   \
                 "name", G_TYPE_STRING, name,           \
                 NULL)

  ADD_DATA("built-in", NULL, "select");

#undef ADD_DATA
}

void
test_path(gconstpointer data)
{
  const gchar *expected;
  const gchar *name;

  expected = gcut_data_get_string(data, "expected");
  name = gcut_data_get_string(data, "name");
  proc = grn_ctx_get(context, name, -1);
  cut_assert_equal_string(expected, grn_obj_path(context, proc));
}
