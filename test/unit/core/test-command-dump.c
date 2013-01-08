/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010 Brazil
  Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
  Copyright (C) 2009-2013  Kouhei Sutou <kou@clear-code.com>

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

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

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
void test_nil_reference(void);
void test_load_with_vector_int32_reference_key(void);
void test_tables_argument(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-dump",
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

  database_path = cut_build_path(tmp_directory, "dump.db", NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  if (context) {
    grn_obj_unlink(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
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
           "table_create Blog TABLE_HASH_KEY ShortText",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY,
           "ShortText",
           NULL);
  ADD_DATA("hash - without key",
           "table_create Blog TABLE_HASH_KEY",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY,
           NULL,
           NULL);
  ADD_DATA("hash - key normalize",
           "table_create Blog TABLE_HASH_KEY ShortText --normalizer NormalizerAuto",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_NORMALIZE,
           "ShortText",
           NULL);
  ADD_DATA("hash - key normalize - value",
           "table_create Blog TABLE_HASH_KEY ShortText Int32 --normalizer NormalizerAuto",
           "Blog",
           GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_NORMALIZE,
           "ShortText",
           "Int32");
}

static void
data_patricia_trie_create(void)
{
  ADD_DATA("patricia trie",
           "table_create Blog TABLE_PAT_KEY ShortText",
           "Blog",
           GRN_OBJ_TABLE_PAT_KEY,
           "ShortText",
           NULL);
}

static void
data_double_array_trie_create(void)
{
  ADD_DATA("double-array trie",
           "table_create Blog TABLE_DAT_KEY ShortText",
           "Blog",
           GRN_OBJ_TABLE_DAT_KEY,
           "ShortText",
           NULL);
}

static void
data_array_create(void)
{
  ADD_DATA("array",
           "table_create Blog TABLE_NO_KEY",
           "Blog",
           GRN_OBJ_TABLE_NO_KEY,
           NULL,
           NULL);
  ADD_DATA("array with value",
           "table_create Blog TABLE_NO_KEY --value_type Int32",
           "Blog",
           GRN_OBJ_TABLE_NO_KEY,
           NULL,
           "Int32");
}

void
data_table_create(void)
{
  data_hash_table_create();
  data_patricia_trie_create();
  data_double_array_trie_create();
  data_array_create();
}
#undef ADD_DATA

void
test_table_create(gconstpointer data)
{
  table_create(gcut_data_get_string(data, "name"),
               gcut_data_get_uint(data, "flags"),
               gcut_data_get_string(data, "key_type_name"),
               gcut_data_get_string(data, "value_type_name"));
  cut_assert_equal_string(gcut_data_get_string(data, "expected"),
                          send_command("dump"));
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
           "column_create Blog body COLUMN_SCALAR Text",
           "Blog",
           "body",
           GRN_OBJ_COLUMN_SCALAR,
           "Text",
           NULL);
  ADD_DATA("vector",
           "column_create Blog body COLUMN_VECTOR Text",
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
  cut_assert_equal_string(
    cut_take_printf("table_create Blog TABLE_HASH_KEY\n%s", expected),
    send_command("dump"));
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
                             "column_create Table Column COLUMN_VECTOR %s\n"
                             "load --table Table\n"
                             "[\n"
                             "[\"_id\",\"Column\"],\n"
                             "[1,%s]\n"
                             "]",
                             type_name,
                             gcut_data_get_string(data, "expected"));
  cut_assert_equal_string(expected, send_command("dump"));
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
                             "column_create Table Column COLUMN_VECTOR %s\n"
                             "load --table Table\n"
                             "[\n"
                             "[\"_id\",\"Column\"],\n"
                             "[1,%s]\n"
                             "]",
                             type_name,
                             gcut_data_get_string(data, "expected"));
  cut_assert_equal_string(expected, send_command("dump"));
  GRN_OBJ_FIN(context, &vector);
}

void
test_unsequantial_records_in_table_with_keys(void)
{
  grn_obj *table;
  grn_id id, expected_id = 1;
  const gchar *keys[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  int i, n_keys = sizeof(keys) / sizeof(keys[0]);

  table = table_create("Weekdays", GRN_OBJ_TABLE_HASH_KEY, "ShortText", NULL);
  grn_test_assert_context(context);

  for (i = 0; i < n_keys; ++i) {
    id = grn_table_add(context, table, keys[i], strlen(keys[i]), NULL);
    cut_assert_equal_int(expected_id++, id);
    grn_test_assert_context(context);
  }

  grn_table_delete_by_id(context, table, 3);
  grn_table_delete_by_id(context, table, 6);

  cut_assert_equal_string("table_create Weekdays TABLE_HASH_KEY ShortText\n"
                          "load --table Weekdays\n"
                          "[\n"
                          "[\"_key\"],\n"
                          "[\"Sun\"],\n"
                          "[\"Mon\"],\n"
                          "[\"Wed\"],\n"
                          "[\"Thu\"],\n"
                          "[\"Sat\"]\n"
                          "]",
                          send_command("dump"));
}

void
test_nil_reference(void)
{
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Initials TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Users initial COLUMN_SCALAR Initials");

  cut_assert_equal_string(
    "2",
    send_command("load --table Initials --columns '_key'\n"
                 "[\n"
                 "  [\"U\"],\n"
                 "  [\"ア\"]\n"
                 "]"));
  cut_assert_equal_string(
    "1",
    send_command("load --table Users --columns '_key'\n"
                 "[\n"
                 "  [\"mori\"]\n"
                 "]"));

  cut_assert_equal_string("table_create Users TABLE_HASH_KEY ShortText\n"
                          "table_create Initials TABLE_PAT_KEY ShortText\n"
                          "column_create Users initial COLUMN_SCALAR Initials\n"
                          "load --table Users\n"
                          "[\n"
                          "[\"_key\",\"initial\"],\n"
                          "[\"mori\",\"\"]\n"
                          "]\n"
                          "load --table Initials\n"
                          "[\n"
                          "[\"_key\"],\n"
                          "[\"U\"],\n"
                          "[\"ア\"]\n"
                          "]",
                          send_command("dump"));
}

void
test_load_with_vector_int32_reference_key(void)
{
  const gchar *commands =
    "table_create users TABLE_HASH_KEY Int32\n"
    "column_create users name COLUMN_SCALAR ShortText\n"
    "table_create comments TABLE_PAT_KEY ShortText\n"
    "column_create comments text COLUMN_SCALAR ShortText\n"
    "column_create comments author COLUMN_VECTOR users\n"
    "load --table users\n"
    "[\n"
    "[\"_key\",\"name\"],\n"
    "[1000,\"ryoqun\"],\n"
    "[1001,\"hayamiz\"]\n"
    "]\n"
    "load --table comments\n"
    "[\n"
    "[\"_key\",\"author\",\"text\"],\n"
    "[\"groonga\",[1000,1001],\"it is fast\"]\n"
    "]";

  assert_send_commands(commands);
  cut_assert_equal_string(commands, send_command("dump"));
}

void
test_tables_argument(void)
{
  const gchar *define_schema_commands =
    "table_create users TABLE_HASH_KEY Int32\n"
    "column_create users name COLUMN_SCALAR ShortText\n"
    "table_create comments TABLE_PAT_KEY ShortText\n"
    "column_create comments text COLUMN_SCALAR ShortText\n"
    "table_create sites TABLE_NO_KEY\n"
    "column_create sites url COLUMN_SCALAR ShortText\n"
    "column_create comments author COLUMN_VECTOR users";
  const gchar *load_users_commands =
    "load --table users\n"
    "[\n"
    "[\"_key\",\"name\"],\n"
    "[1000,\"ryoqun\"],\n"
    "[1001,\"hayamiz\"]\n"
    "]";
  const gchar *load_comments_commands =
    "load --table comments\n"
    "[\n"
    "[\"_key\",\"text\",\"author\"],\n"
    "[\"groonga\",\"it is fast\",[1000,1001]]\n"
    "]";
  const gchar *load_sites_commands =
    "load --table sites\n"
    "[\n"
    "[\"_id\",\"url\"],\n"
    "[1,\"http://groonga.org/\"],\n"
    "[2,\"http://qwik.jp/senna/\"]\n"
    "]";

  assert_send_commands(define_schema_commands);
  assert_send_commands(load_users_commands);
  assert_send_commands(load_comments_commands);
  assert_send_commands(load_sites_commands);
  cut_assert_equal_string(cut_take_printf("%s\n"
                                          "%s\n"
                                          "%s",
                                          define_schema_commands,
                                          load_users_commands,
                                          load_sites_commands),
                          send_command("dump --tables users,sites"));
}
