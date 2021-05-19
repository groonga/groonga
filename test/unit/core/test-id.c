/*
  Copyright (C) 2016  Kouhei Sutou <kou@clear-code.com>

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

#include "../../../config.h"

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_is_builtin(void);
void test_is_builtin(gconstpointer data);
void data_is_builtin_type(void);
void test_is_builtin_type(gconstpointer data);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "id",
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
data_is_builtin(void)
{
#define ADD_DATUM(expected, id)                                         \
  gcut_add_datum((expected ? "built-in - " #id : "custom - " #id),      \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "id", G_TYPE_UINT, id,                                 \
                 NULL)

  ADD_DATUM(FALSE, GRN_ID_NIL);
  ADD_DATUM(TRUE, GRN_DB_BIGRAM);
  ADD_DATUM(TRUE, GRN_DB_MECAB);
  ADD_DATUM(TRUE, 255);
  ADD_DATUM(FALSE, 256);

#undef ADD_DATUM
}

void
test_is_builtin(gconstpointer data)
{
  grn_id id;

  id = gcut_data_get_uint(data, "id");
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_id_is_builtin(context, id));
  } else {
    cut_assert_false(grn_id_is_builtin(context, id));
  }
}

void
data_is_builtin_type(void)
{
#define ADD_DATUM(expected, id)                                         \
  gcut_add_datum((expected ? "built-in - " #id : "custom - " #id),      \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "id", G_TYPE_UINT, id,                                 \
                 NULL)

  ADD_DATUM(FALSE, GRN_DB_DB);
  ADD_DATUM(TRUE, GRN_DB_OBJECT);
  ADD_DATUM(TRUE, GRN_DB_BOOL);
  ADD_DATUM(TRUE, GRN_DB_INT8);
  ADD_DATUM(TRUE, GRN_DB_UINT8);
  ADD_DATUM(TRUE, GRN_DB_INT16);
  ADD_DATUM(TRUE, GRN_DB_UINT16);
  ADD_DATUM(TRUE, GRN_DB_INT32);
  ADD_DATUM(TRUE, GRN_DB_UINT32);
  ADD_DATUM(TRUE, GRN_DB_INT64);
  ADD_DATUM(TRUE, GRN_DB_UINT64);
  ADD_DATUM(TRUE, GRN_DB_FLOAT);
  ADD_DATUM(TRUE, GRN_DB_TIME);
  ADD_DATUM(TRUE, GRN_DB_SHORT_TEXT);
  ADD_DATUM(TRUE, GRN_DB_TEXT);
  ADD_DATUM(TRUE, GRN_DB_LONG_TEXT);
  ADD_DATUM(TRUE, GRN_DB_TOKYO_GEO_POINT);
  ADD_DATUM(TRUE, GRN_DB_WGS84_GEO_POINT);
  ADD_DATUM(FALSE, GRN_DB_MECAB);

#undef ADD_DATUM
}

void
test_is_builtin_type(gconstpointer data)
{
  grn_id id;

  id = gcut_data_get_uint(data, "id");
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_id_is_builtin_type(context, id));
  } else {
    cut_assert_false(grn_id_is_builtin_type(context, id));
  }
}
