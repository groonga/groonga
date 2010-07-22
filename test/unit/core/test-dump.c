/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
  Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>

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

void data_table_create(void);
void test_table_create(gconstpointer data);
void data_column_create(void);
void test_column_create(gconstpointer data);
void data_uvector_column(void);
void data_vector_column(void);
void test_uvector_column(gconstpointer data);
void test_vector_column(gconstpointer data);
void test_unsequantial_records_in_table_with_keys(void);

static GCutEgg *egg = NULL;
static GString *dumped = NULL, *error_output = NULL;

static grn_logger_info *logger;
static grn_ctx *context;
static grn_obj *database;
static gchar *tmp_directory;
static gchar *database_path;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
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
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  database = grn_db_create(context, database_path, NULL);
  grn_test_assert_context(context);
}

void
cut_teardown(void)
{
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);
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
  gcut_egg_wait(egg, 10000, &error);
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
    key_type = get_object(key_type_name);
  if (value_type_name)
    value_type = get_object(value_type_name);

  table = grn_table_create(context,
                           name, strlen(name),
                           NULL,
                           flags | GRN_OBJ_PERSISTENT,
                           key_type, value_type);
  grn_test_assert_context(context);
  return table;
}

static grn_obj *
column_create(const gchar *table_name, const gchar *name, grn_obj_flags flags,
              const gchar *type_name, const gchar *sources)
{
  grn_obj *column;
  grn_obj *table = NULL, *type = NULL;

  if (table_name)
    table = get_object(table_name);
  if (type_name)
    type = get_object(type_name);

  column = grn_column_create(context,
                             table,
                             name, strlen(name),
                             NULL,
                             flags | GRN_OBJ_PERSISTENT,
                             type);
  grn_test_assert_context(context);
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
           "table_create Blog TABLE_HASH_KEY ShortText\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY,
           "ShortText",
           NULL);
  ADD_DATA("hash - without key",
           "table_create Blog TABLE_HASH_KEY\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY,
           NULL,
           NULL);
  ADD_DATA("hash - key normalize",
           "table_create Blog TABLE_HASH_KEY|KEY_NORMALIZE ShortText\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_NORMALIZE,
           "ShortText",
           NULL);
  ADD_DATA("hash - key normalize - value",
           "table_create Blog TABLE_HASH_KEY|KEY_NORMALIZE ShortText Int32\n",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_NORMALIZE,
           "ShortText",
           "Int32");
}

static void
data_patricia_trie_create(void)
{
  ADD_DATA("patricia trie",
           "table_create Blog TABLE_PAT_KEY ShortText\n",
           "Blog",
           GRN_OBJ_TABLE_PAT_KEY,
           "ShortText",
           NULL);
}

static void
data_array_create(void)
{
  ADD_DATA("array",
           "table_create Blog TABLE_NO_KEY\n",
           "Blog",
           GRN_OBJ_TABLE_NO_KEY,
           NULL,
           NULL);
  ADD_DATA("array with value",
           "table_create Blog TABLE_NO_KEY --value_type Int32\n",
           "Blog",
           GRN_OBJ_TABLE_NO_KEY,
           NULL,
           "Int32");
}

static void
data_view_create(void)
{
  ADD_DATA("view",
           "table_create Blog TABLE_VIEW\n",
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
  ADD_DATA("scalar",
           "column_create Blog body COLUMN_SCALAR|COMPRESS_NONE Text\n",
           "Blog",
           "body",
           GRN_OBJ_COLUMN_SCALAR,
           "Text",
           NULL);
  ADD_DATA("vector",
           "column_create Blog body COLUMN_VECTOR|COMPRESS_NONE Text\n",
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
  grn_test_assert_dump(cut_take_printf("table_create Blog TABLE_HASH_KEY\n%s", expected));
}

#define ADD_DATA(label, expected, type_name, element_type,              \
                 first_element, second_element, third_element) do {     \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "type_name", G_TYPE_STRING, type_name,                 \
                 "first_element", element_type, first_element,          \
                 "second_element", element_type, second_element,        \
                 "third_element", element_type, third_element,          \
                 NULL);                                                 \
} while(0)

void
data_uvector_column(void)
{
  ADD_DATA("int32", "[-322,7,9270]", "Int32",
           G_TYPE_INT, -322, 7, 9270);
  ADD_DATA("float", "[0.5,12.5,-1.0]", "Float",
           G_TYPE_DOUBLE, 0.5, 12.5, -1.0);
  ADD_DATA("bool", "[true,false,true]", "Bool",
           G_TYPE_BOOLEAN, TRUE, FALSE, TRUE);
}

void
data_vector_column(void)
{
  ADD_DATA("text", "[\"groonga\",\"is\",\"cool\"]", "Text",
           G_TYPE_STRING, "groonga", "is", "cool");
}

#undef ADD_DATA

static grn_obj *
construct_elements(gconstpointer data)
{
  const int n_of_elements = 3;
  grn_obj *elements;
  const gchar *type_name;

  elements = g_new0(grn_obj, n_of_elements);
  type_name = gcut_data_get_string(data, "type_name");

#define SET_VALUE(index, name)                          \
  if (g_str_equal(type_name, "Int32")) {                \
    GRN_INT32_INIT(&elements[index], 0);                \
    GRN_INT32_SET(context, &elements[index],            \
                  gcut_data_get_int(data, name));       \
  } if (g_str_equal(type_name, "Float")) {              \
    GRN_FLOAT_INIT(&elements[index], 0);                \
    GRN_FLOAT_SET(context, &elements[index],            \
                  gcut_data_get_double(data, name));    \
  } if (g_str_equal(type_name, "Bool")) {               \
    GRN_BOOL_INIT(&elements[index], 0);                 \
    GRN_BOOL_SET(context, &elements[index],             \
                 gcut_data_get_boolean(data, name));    \
  } if (g_str_equal(type_name, "Text")) {               \
    GRN_TEXT_INIT(&elements[index], 0);                 \
    GRN_TEXT_SETS(context, &elements[index],            \
                  gcut_data_get_string(data, name));    \
  }

  SET_VALUE(0, "first_element");
  SET_VALUE(1, "second_element");
  SET_VALUE(2, "third_element");

#undef SET_VALUE

  cut_take_memory(elements);

  return elements;
}

void
test_uvector_column(gconstpointer data)
{
  const gchar *expected;
  grn_id id, type_id;
  grn_obj uvector;
  grn_obj *elements;
  grn_obj *table, *column;
  const gchar *type_name;
  type_name = gcut_data_get_string(data, "type_name");
  type_id = grn_obj_id(context, get_object(type_name));

  table = table_create("Table", GRN_OBJ_TABLE_NO_KEY, NULL, NULL);
  grn_test_assert_context(context);
  column = column_create("Table", "Column", GRN_OBJ_COLUMN_VECTOR,
                         type_name, NULL);
  grn_test_assert_context(context);
  id = grn_table_add(context, table, NULL, 0, NULL);
  grn_test_assert_context(context);
  cut_assert_equal_int(1, id);
  elements = construct_elements(data);

  GRN_OBJ_INIT(&uvector, GRN_UVECTOR, 0, type_id);
  grn_bulk_write(context, &uvector,
                 GRN_BULK_HEAD(&elements[0]), GRN_BULK_VSIZE(&elements[0]));
  grn_bulk_write(context, &uvector,
                 GRN_BULK_HEAD(&elements[1]), GRN_BULK_VSIZE(&elements[1]));
  grn_bulk_write(context, &uvector,
                 GRN_BULK_HEAD(&elements[2]), GRN_BULK_VSIZE(&elements[2]));
  grn_obj_set_value(context, column, id, &uvector, GRN_OBJ_SET);

  expected = cut_take_printf("table_create Table TABLE_NO_KEY\n"
                             "column_create Table Column COLUMN_VECTOR|COMPRESS_NONE %s\n"
                             "load --table Table\n"
                             "[\n"
                             "[\"_id\",\"Column\"],\n"
                             "[1,%s]\n"
                             "]\n",
                             type_name,
                             gcut_data_get_string(data, "expected"));
  grn_test_assert_dump(expected);
  GRN_OBJ_FIN(context, &uvector);
}

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
  type_id = grn_obj_id(context, get_object(type_name));

  table = table_create("Table", GRN_OBJ_TABLE_NO_KEY, NULL, NULL);
  grn_test_assert_context(context);
  column = column_create("Table", "Column", GRN_OBJ_COLUMN_VECTOR,
                         type_name, NULL);
  grn_test_assert_context(context);
  id = grn_table_add(context, table, NULL, 0, NULL);
  grn_test_assert_context(context);
  cut_assert_equal_int(1, id);
  elements = construct_elements(data);

  GRN_TEXT_INIT(&vector, GRN_OBJ_VECTOR);
  grn_vector_add_element(context, &vector,
                         GRN_TEXT_VALUE(&elements[0]),
                         GRN_TEXT_LEN(&elements[0]), 0, type_id);
  grn_vector_add_element(context, &vector,
                         GRN_TEXT_VALUE(&elements[1]),
                         GRN_TEXT_LEN(&elements[1]), 0, type_id);
  grn_vector_add_element(context, &vector,
                         GRN_TEXT_VALUE(&elements[2]),
                         GRN_TEXT_LEN(&elements[2]), 0, type_id);
  grn_obj_set_value(context, column, id, &vector, GRN_OBJ_SET);

  expected = cut_take_printf("table_create Table TABLE_NO_KEY\n"
                             "column_create Table Column COLUMN_VECTOR|COMPRESS_NONE %s\n"
                             "load --table Table\n"
                             "[\n"
                             "[\"_id\",\"Column\"],\n"
                             "[1,%s]\n"
                             "]\n",
                             type_name,
                             gcut_data_get_string(data, "expected"));
  grn_test_assert_dump(expected);
  GRN_OBJ_FIN(context, &vector);
}

void
test_unsequantial_records_in_table_with_keys(void)
{
  grn_obj *table;
  grn_id id, expected_id = 1;
  char *keys[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  int i, n_keys = sizeof(keys)/sizeof(keys[0]);

  table = table_create("Weekdays", GRN_OBJ_TABLE_HASH_KEY, "ShortText", NULL);
  grn_test_assert_context(context);

  for (i = 0; i < n_keys; ++i) {
    id = grn_table_add(context, table, keys[i], strlen(keys[i]), NULL);
    cut_assert_equal_int(expected_id++, id);
    grn_test_assert_context(context);
  }

  grn_table_delete_by_id(context, table, 3);
  grn_table_delete_by_id(context, table, 6);

  grn_test_assert_dump("table_create Weekdays TABLE_HASH_KEY ShortText\n"
                       "load --table Weekdays\n"
                       "[\n"
                       "[\"_key\"],\n"
                       "[\"Sun\"],\n"
                       "[\"Mon\"],\n"
                       "[\"Wed\"],\n"
                       "[\"Thu\"],\n"
                       "[\"Sat\"]\n"
                       "]\n");
}
