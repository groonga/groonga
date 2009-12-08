/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define GET(name) grn_ctx_get(&context, name, strlen(name))

void data_table_create(void);
void test_table_create(gconstpointer data);

static GCutEgg *egg = NULL;
static GString *dumped = NULL;

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj *database;
static gchar *tmp_directory;
static gchar *database_path;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "test-database",
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
  egg = NULL;
  dumped = NULL;

  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  database = grn_db_create(&context, database_path, NULL);
  grn_test_assert_context(&context);
}

void
cut_teardown(void)
{
  grn_ctx_fin(&context);
  teardown_grn_logger(logger);

  if (egg) {
    g_object_unref(egg);
  }

  if (dumped) {
    g_string_free(dumped, TRUE);
  }

  remove_tmp_directory();
}

static void
cb_output_received(GCutEgg *egg, gchar *chunk, guint64 size, gpointer user_data)
{
  g_string_append_len(dumped, chunk, size);
}

static const gchar *
groonga_path(void)
{
  const gchar *path;

  path = g_getenv("GROONGA");
  if (path)
    return path;

  return GROONGA;
}

static void
grn_test_assert_dump(const gchar *expected)
{
  GError *error = NULL;

  dumped = g_string_new(NULL);
  egg = gcut_egg_new(groonga_path(),
                     "-e", "utf8",
                     database_path,
                     "dump",
                     NULL);
  g_signal_connect(egg, "output-received",
                   G_CALLBACK(cb_output_received), NULL);

  gcut_egg_hatch(egg, &error);
  gcut_assert_error(error);
  gcut_egg_wait(egg, 1000, &error);
  gcut_assert_error(error);

  cut_assert_equal_string(expected, dumped->str);

  g_object_unref(egg);
  egg = NULL;

  g_string_free(dumped, TRUE);
  dumped = NULL;
}

static void
table_create(const gchar *name, grn_obj_flags flags,
             const gchar *key_type_name, const gchar *value_type_name)
{
  grn_obj *key_type = NULL, *value_type = NULL;

  if (key_type_name)
    key_type = GET(key_type_name);
  if (value_type_name)
    value_type = GET(value_type_name);

  grn_table_create(&context,
                   name, strlen(name),
                   NULL,
                   flags | GRN_OBJ_PERSISTENT,
                   key_type, value_type);
  grn_test_assert_context(&context);
}

#define ADD_DATA(label, expected, name, flags,                          \
                 key_type_name, value_type_name)                        \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "name", G_TYPE_STRING, name,                           \
                 "flags", G_TYPE_UINT, flags,                           \
                 "key_type_name", G_TYPE_STRING, key_type_name,         \
                 "value_type_name", G_TYPE_STRING, value_type_name,     \
                 NULL)

static void
data_hash_table_create(void)
{
  ADD_DATA("hash",
           "table_create Blog 0 ShortText\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY,
           "ShortText",
           NULL);
  ADD_DATA("hash - without key",
           "table_create Blog 0\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY,
           NULL,
           NULL);
  ADD_DATA("hash - key normalize",
           "table_create Blog 128 ShortText\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_NORMALIZE,
           "ShortText",
           NULL);
  ADD_DATA("hash - key normalize - value",
           "table_create Blog 128 ShortText Int32\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_NORMALIZE,
           "ShortText",
           "Int32");
  ADD_DATA("hash - escaped name",
           "table_create \"Blog\\\"\" 0 ShortText\n",
           "Blog\"",
           GRN_OBJ_TABLE_HASH_KEY,
           "ShortText",
           NULL);
}

static void
data_patricia_trie_create(void)
{
  ADD_DATA("patricia trie",
           "table_create Blog 1 ShortText\n",
           "Blog",
           GRN_OBJ_TABLE_PAT_KEY,
           "ShortText",
           NULL);
}

static void
data_array_create(void)
{
  ADD_DATA("array",
           "table_create Blog 3\n",
           "Blog",
           GRN_OBJ_TABLE_NO_KEY,
           NULL,
           NULL);
}

static void
data_view_create(void)
{
  ADD_DATA("view",
           "table_create Blog 4\n",
           "Blog",
           GRN_OBJ_TABLE_VIEW,
           NULL,
           NULL);
}

void
data_table_create(void)
{
  data_hash_table_create();
  data_patricia_trie_create();
  data_array_create();
  data_view_create();
}
#undef ADD_DATA

void
test_table_create(gconstpointer data)
{
  table_create(gcut_data_get_string(data, "name"),
               gcut_data_get_uint(data, "flags"),
               gcut_data_get_string(data, "key_type_name"),
               gcut_data_get_string(data, "value_type_name"));
  grn_test_assert_dump(gcut_data_get_string(data, "expected"));
}
