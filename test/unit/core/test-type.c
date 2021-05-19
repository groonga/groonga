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

void data_id_is_builtin(void);
void test_id_is_builtin(gconstpointer data);
void data_id_is_number_family(void);
void test_id_is_number_family(gconstpointer data);
void data_id_is_text_family(void);
void test_id_is_text_family(gconstpointer data);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "type",
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
data_id_is_builtin(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("built-in - Bool",          TRUE, "Bool");
  ADD_DATUM("built-in - WGS84GeoPoint", TRUE, "WGS84GeoPoint");
  ADD_DATUM("custom - Users",           FALSE, "Users");
  ADD_DATUM("not type - TokenBigram",   FALSE, "TokenBigrm");

#undef ADD_DATUM
}

void
test_id_is_builtin(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;
  grn_id id;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  id = grn_obj_id(context, object);
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_type_id_is_builtin(context, id));
  } else {
    cut_assert_false(grn_type_id_is_builtin(context, id));
  }
}

void
data_id_is_number_family(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ? "number-family - " name : "column - " name), \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE,  "Int8");
  ADD_DATUM(TRUE, "UInt8");
  ADD_DATUM(TRUE,  "Int16");
  ADD_DATUM(TRUE, "UInt16");
  ADD_DATUM(TRUE,  "Int32");
  ADD_DATUM(TRUE, "UInt32");
  ADD_DATUM(TRUE,  "Int64");
  ADD_DATUM(TRUE, "UInt64");
  ADD_DATUM(TRUE,  "Float");
  ADD_DATUM(FALSE, "Time");

#undef ADD_DATUM
}

void
test_id_is_number_family(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;
  grn_id id;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  id = grn_obj_id(context, object);
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_type_id_is_number_family(context, id));
  } else {
    cut_assert_false(grn_type_id_is_number_family(context, id));
  }
}

void
data_id_is_text_family(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ? "text-family - " name : "column - " name), \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "ShortText");
  ADD_DATUM(TRUE, "Text");
  ADD_DATUM(TRUE, "LongText");
  ADD_DATUM(FALSE, "Time");
  ADD_DATUM(FALSE, "TokyoGeoPoint");

#undef ADD_DATUM
}

void
test_id_is_text_family(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;
  grn_id id;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  id = grn_obj_id(context, object);
  if (gcut_data_get_string(data, "expected")) {
    cut_assert_true(grn_type_id_is_text_family(context, id));
  } else {
    cut_assert_false(grn_type_id_is_text_family(context, id));
  }
}
