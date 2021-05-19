/*
  Copyright (C) 2010-2012  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_text_to_table(void);
void test_text_to_table(gconstpointer data);

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj *database;
static grn_obj src, dest;

static gchar *tmp_directory;
static grn_id users, daijiro;

static void
setup_database(void)
{
  gchar *database_path;

  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   NULL);
  database_path = g_build_filename(tmp_directory,
                                   "cast-table.db",
                                   NULL);

  g_mkdir_with_parents(tmp_directory, 0700);
  database = grn_db_create(&context, database_path, NULL);
  g_free(database_path);
}

static void
teardown_database(void)
{
  grn_obj_close(&context, database);
  cut_remove_path(tmp_directory, NULL);
  g_free(tmp_directory);
}

static grn_id
create_table(const gchar *name, grn_obj_flags flags, const gchar *key_type_name)
{
  grn_obj *key_type = NULL;
  grn_obj *table;

  if (key_type_name) {
    key_type = grn_ctx_get(&context, key_type_name, strlen(key_type_name));
  }
  table = grn_table_create(&context, name, strlen(name),
                           NULL, GRN_OBJ_PERSISTENT | flags,
                           key_type, NULL);
  if (key_type) {
    grn_obj_unlink(&context, key_type);
  }

  return grn_obj_id(&context, table);
}

static grn_id
add_record(const gchar *table_name, const gchar *key)
{
  grn_obj *table;
  grn_id record_id;

  table = grn_ctx_get(&context, table_name, strlen(table_name));
  record_id = grn_table_add(&context, table, key, key ? strlen(key) : 0, NULL);
  grn_obj_unlink(&context, table);

  grn_test_assert_not_nil(record_id);
  return record_id;
}

static void
setup_tables(void)
{
  users = create_table("Users", GRN_OBJ_TABLE_HASH_KEY, "ShortText");
  daijiro = add_record("Users", "daijiro");
}

void
cut_setup(void)
{
  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);
  GRN_VOID_INIT(&src);
  GRN_VOID_INIT(&dest);

  setup_database();
  setup_tables();
}

void
cut_teardown(void)
{
  grn_obj_remove(&context, &src);
  grn_obj_remove(&context, &dest);
  teardown_database();

  grn_ctx_fin(&context);
  teardown_grn_logger(logger);
}

static void
cast_text(grn_rc expected_rc, const gchar *text)
{
  grn_obj_reinit(&context, &src, GRN_DB_TEXT, 0);
  if (text) {
    GRN_TEXT_PUTS(&context, &src, text);
  }
  grn_test_assert_equal_rc(expected_rc,
                           grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
data_text_to_table(void)
{
#define ADD_DATA(label, expected, expected_size, text)                  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_UINT, expected,                     \
                 "expected-size", GCUT_TYPE_SIZE, expected_size,        \
                 "text", G_TYPE_STRING, text,                           \
                 NULL)

  ADD_DATA("existence", 1, sizeof(grn_id), "daijiro");
  ADD_DATA("nonexistence", GRN_ID_NIL, 0, "yu");
  ADD_DATA("empty key", GRN_ID_NIL, sizeof(grn_id), "");

#undef ADD_DATA
}

void
test_text_to_table(gconstpointer data)
{
  gsize expected_size;

  grn_obj_reinit(&context, &dest, users, 0);
  expected_size = gcut_data_get_size(data, "expected-size");
  if (expected_size == 0) {
    cast_text(GRN_INVALID_ARGUMENT, gcut_data_get_string(data, "text"));
    cut_assert_equal_uint(0, GRN_BULK_VSIZE(&dest));
  } else {
    cast_text(GRN_SUCCESS, gcut_data_get_string(data, "text"));
    grn_test_assert_equal_record_id(&context,
                                    grn_ctx_at(&context, users),
                                    gcut_data_get_uint(data, "expected"),
                                    GRN_RECORD_VALUE(&dest));
    cut_assert_equal_uint(expected_size, GRN_BULK_VSIZE(&dest));
  }
}
