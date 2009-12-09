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
void data_column_create(void);
void test_column_create(gconstpointer data);

static GCutEgg *egg = NULL;
static GString *dumped = NULL, *error_output = NULL;

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
  error_output = NULL;

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

  if (error_output) {
    g_string_free(error_output, TRUE);
  }

  remove_tmp_directory();
}

static void
cb_output_received(GCutEgg *egg, gchar *chunk, guint64 size, gpointer user_data)
{
  g_string_append_len(dumped, chunk, size);
}

static void
cb_error_received(GCutEgg *egg, gchar *chunk, guint64 size, gpointer user_data)
{
  g_string_append_len(error_output, chunk, size);
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
  error_output = g_string_new(NULL);
  egg = gcut_egg_new(groonga_path(),
                     "-e", "utf8",
                     database_path,
                     "dump",
                     NULL);
  g_signal_connect(egg, "output-received",
                   G_CALLBACK(cb_output_received), NULL);
  g_signal_connect(egg, "error-received",
                   G_CALLBACK(cb_error_received), NULL);

  gcut_egg_hatch(egg, &error);
  gcut_assert_error(error);
  gcut_egg_wait(egg, 1000, &error);
  gcut_assert_error(error);

  fprintf(stderr, "%s", error_output->str);
  cut_assert_equal_string(expected, dumped->str);

  g_object_unref(egg);
  egg = NULL;

  g_string_free(dumped, TRUE);
  g_string_free(error_output, TRUE);
  dumped = NULL;
  error_output = NULL;
}

static grn_obj *
table_create(const gchar *name, grn_obj_flags flags,
             const gchar *key_type_name, const gchar *value_type_name)
{
  grn_obj *table;
  grn_obj *key_type = NULL, *value_type = NULL;

  if (key_type_name)
    key_type = GET(key_type_name);
  if (value_type_name)
    value_type = GET(value_type_name);

  table = grn_table_create(&context,
                           name, strlen(name),
                           NULL,
                           flags | GRN_OBJ_PERSISTENT,
                           key_type, value_type);
  grn_test_assert_context(&context);
  return table;
}

static grn_obj *
column_create(const gchar *table_name, const gchar *name, grn_obj_flags flags,
              const gchar *type_name, const gchar *sources)
{
  grn_obj *column;
  grn_obj *table = NULL, *type = NULL;

  if (table_name)
    table = GET(table_name);
  if (type_name)
    type = GET(type_name);

  column = grn_column_create(&context,
                             table,
                             name, strlen(name),
                             NULL,
                             flags | GRN_OBJ_PERSISTENT,
                             type);
  grn_test_assert_context(&context);
  return column;
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
  ADD_DATA("array with value",
           "table_create Blog 3 --value_type Int32\n",
           "Blog",
           GRN_OBJ_TABLE_NO_KEY,
           NULL,
           "Int32");
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

#define ADD_DATA(label, expected, table, name, flags, type_name, source)\
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "table", G_TYPE_STRING, table,                         \
                 "name", G_TYPE_STRING, name,                           \
                 "flags", G_TYPE_UINT, flags,                           \
                 "type_name", G_TYPE_STRING, type_name,                 \
                 "source", G_TYPE_STRING, source,                       \
                 NULL)

void
data_column_create(void)
{
  ADD_DATA("column - scalar",
           "column_create Blog body 0 Text\n",
           "Blog",
           "body",
           GRN_OBJ_COLUMN_SCALAR,
           "Text",
           NULL);
  ADD_DATA("column - vector",
           "column_create Blog body 1 Text\n",
           "Blog",
           "body",
           GRN_OBJ_COLUMN_VECTOR,
           "Text",
           NULL);
}
#undef ADD_DATA

void
test_column_create(gconstpointer data)
{
  const gchar *expected;
  table_create("Blog", 0, NULL, NULL);
  column_create(gcut_data_get_string(data, "table"),
                gcut_data_get_string(data, "name"),
                gcut_data_get_uint(data, "flags"),
                gcut_data_get_string(data, "type_name"),
                gcut_data_get_string(data, "source"));
  expected = gcut_data_get_string(data, "expected");
  grn_test_assert_dump(cut_take_printf("table_create Blog 0\n%s", expected));
}

#define ADD_DATA(label, expected, type_name, OBJ_INIT, OBJ_SET,         \
                 first_element, second_element, third_element) do {     \
  const int n_of_elements = 3;                                          \
  grn_obj *elements;                                                    \
  elements = g_new0(grn_obj, n_of_elements);                            \
  OBJ_INIT(&elements[0], 0);                                            \
  OBJ_SET(&context, &elements[0], first_element);                       \
  OBJ_INIT(&elements[1], 0);                                            \
  OBJ_SET(&context, &elements[1], second_element);                      \
  OBJ_INIT(&elements[2], 0);                                            \
  OBJ_SET(&context, &elements[2], third_element);                       \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "type_name", G_TYPE_STRING, type_name,                 \
                 "elements", G_TYPE_POINTER, elements, g_free,          \
                 NULL);                                                 \
} while(0)

void
data_vector_column(void)
{
  ADD_DATA("vector records - text", "[\"groonga\",\"is\",\"cool\"]", "Text",
           GRN_TEXT_INIT, GRN_TEXT_PUTS, "groonga", "is", "cool");
  ADD_DATA("vector records - int32", "[-322,7,9270]", "Int32",
           GRN_INT32_INIT, GRN_INT32_SET, -322, 7, 9270);
  ADD_DATA("vector records - float", "[0.5,12.5,-1.0]", "Float",
           GRN_FLOAT_INIT, GRN_FLOAT_SET, 0.5, 12.5, -1.0);
  ADD_DATA("vector records - bool", "[true,false,true]", "Bool",
           GRN_BOOL_INIT, GRN_BOOL_SET, TRUE, FALSE, TRUE);
}
#undef ADD_DATA

void
test_vector_column(gconstpointer data)
{
  const gchar *expected;
  grn_id id, type_id;
  grn_obj vector;
  grn_obj *elements;
  grn_obj *table, *column;
  const gchar *type_name;
  type_name = gcut_data_get_string(data, "type_name");
  type_id = grn_obj_id(&context, GET(type_name));

  table = table_create("Table", GRN_OBJ_TABLE_NO_KEY, NULL, NULL);
  grn_test_assert_context(&context);
  column = column_create("Table", "Column", GRN_OBJ_COLUMN_VECTOR,
                         type_name, NULL);
  grn_test_assert_context(&context);
  id = grn_table_add(&context, table, NULL, 0, NULL);
  grn_test_assert_context(&context);
  cut_assert_equal_int(1, id);
  elements = (grn_obj *)gcut_data_get_pointer(data, "elements");

  GRN_TEXT_INIT(&vector, GRN_OBJ_VECTOR);
  grn_vector_add_element(&context, &vector,
                         GRN_TEXT_VALUE(&elements[0]),
                         GRN_TEXT_LEN(&elements[0]), 0, type_id);
  grn_vector_add_element(&context, &vector,
                         GRN_TEXT_VALUE(&elements[1]),
                         GRN_TEXT_LEN(&elements[1]), 0, type_id);
  grn_vector_add_element(&context, &vector,
                         GRN_TEXT_VALUE(&elements[2]),
                         GRN_TEXT_LEN(&elements[2]), 0, type_id);
  grn_obj_set_value(&context, column, id, &vector, GRN_OBJ_SET);

  expected = cut_take_printf("table_create Table 3\n"
                             "column_create Table Column 1 %s\n"
                             "load --table Table\n"
                             "[\n"
                             "{\"_id\":1,\"Column\":%s}\n"
                             "]\n",
                             type_name,
                             gcut_data_get_string(data, "expected"));
  grn_test_assert_dump(expected);
}
