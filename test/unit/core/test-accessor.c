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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define get(name) grn_ctx_get(context, name, strlen(name))

void data_column_name(void);
void test_column_name(gconstpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "accessor",
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
}

void
cut_teardown(void)
{
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

void
data_column_name(void)
{
#define ADD_DATA(accessor_name)                                 \
  gcut_add_datum(accessor_name,                                 \
                 "accessor_name", G_TYPE_STRING, accessor_name, \
                 NULL)

  ADD_DATA("_id");
  ADD_DATA("_key");
  ADD_DATA("_value");
  ADD_DATA("_score");

#undef ADD_DATA
}

void
test_column_name(gconstpointer data)
{
  const gchar *table_name = "Bookmarks";
  const gchar *accessor_name;
  grn_obj *bookmarks, *accessor;
  gchar name[256];
  gint length;

  bookmarks = grn_table_create(context, table_name, strlen(table_name),
                               NULL,
                               GRN_TABLE_HASH_KEY |
                               GRN_OBJ_PERSISTENT |
                               GRN_OBJ_WITH_SUBREC,
                               get("ShortText"), get("Int32"));
  accessor_name = gcut_data_get_string(data, "accessor_name");
  accessor = grn_obj_column(context, bookmarks,
                            accessor_name, strlen(accessor_name));
  length = grn_column_name(context, accessor, name, sizeof(name));
  name[length] = '\0';
  grn_obj_unlink(context, accessor);
  cut_assert_equal_string(accessor_name, name);
}
