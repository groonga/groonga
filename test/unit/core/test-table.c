/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
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

#define OBJECT(name) (grn_ctx_get(&context, (name), strlen(name)))

void test_array_set_data(void);
void data_temporary_table_no_path(void);
void test_temporary_table_no_path(gpointer data);
void data_temporary_table_default_tokenizer(void);
void test_temporary_table_default_tokenizer(gpointer data);
void data_temporary_table_add(void);
void test_temporary_table_add(gpointer data);
void test_nonexistent_column(void);

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj *database;

void
cut_setup(void)
{
  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, NULL, NULL);
}

void
cut_teardown(void)
{
  grn_ctx_fin(&context);
  teardown_grn_logger(logger);
}

void
test_array_set_data(void)
{
  grn_obj *table;
  grn_id record_id;
  gchar value[] = "sample value";
  grn_obj *record_value;
  grn_obj *retrieved_record_value;
  grn_obj *value_type = grn_type_create(&context, "<value_type>", 12, 0, sizeof(value));
  table = grn_table_create(&context, NULL, 0, NULL,
                           GRN_OBJ_TABLE_NO_KEY,
                           NULL, value_type);
  record_id = grn_table_add(&context, table, NULL, 0, NULL);

  record_value = grn_obj_open(&context, GRN_BULK, 0, 0);
  grn_bulk_write(&context, record_value, value, sizeof(value));
  grn_test_assert(grn_obj_set_value(&context, table, record_id,
                                    record_value, GRN_OBJ_SET));

  retrieved_record_value = grn_obj_get_value(&context, table, record_id, NULL);
  cut_assert_equal_string(value, GRN_BULK_HEAD(retrieved_record_value));
}

void
data_temporary_table_no_path(void)
{
#define ADD_DATA(label, flags)                                          \
  cut_add_data(label, GINT_TO_POINTER(flags), NULL, NULL)

  ADD_DATA("no-key", GRN_OBJ_TABLE_NO_KEY);
  ADD_DATA("hash", GRN_OBJ_TABLE_HASH_KEY);
  ADD_DATA("patricia trie", GRN_OBJ_TABLE_PAT_KEY);

#undef ADD_DATA
}

void
test_temporary_table_no_path(gpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = GPOINTER_TO_INT(data);

  table = grn_table_create(&context, NULL, 0, NULL,
                           flags,
                           NULL, NULL);
  cut_assert_equal_string(NULL, grn_obj_path(&context, table));
}

void
data_temporary_table_default_tokenizer(void)
{
#define ADD_DATA(label, flags)                                          \
  cut_add_data(label, GINT_TO_POINTER(flags), NULL, NULL)

  ADD_DATA("hash", GRN_OBJ_TABLE_HASH_KEY);
  ADD_DATA("patricia trie", GRN_OBJ_TABLE_PAT_KEY);

#undef ADD_DATA
}

void
test_temporary_table_default_tokenizer(gpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = GPOINTER_TO_INT(data);
  grn_obj *tokenizer = NULL;
  char name[1024];
  int name_size;

  table = grn_table_create(&context, NULL, 0, NULL,
                           flags,
                           NULL, NULL);
  grn_obj_set_info(&context, table, GRN_INFO_DEFAULT_TOKENIZER,
                   OBJECT("TokenTrigram"));
  tokenizer = grn_obj_get_info(&context, table, GRN_INFO_DEFAULT_TOKENIZER, NULL);
  name_size = grn_obj_name(&context, tokenizer, name, sizeof(name));
  name[name_size] = '\0';
  cut_assert_equal_string("TokenTrigram", name);
}

void
data_temporary_table_add(void)
{
#define ADD_DATA(label, flags)                                          \
  cut_add_data(label, GINT_TO_POINTER(flags), NULL, NULL)

  ADD_DATA("no-key", GRN_OBJ_TABLE_NO_KEY);
  ADD_DATA("hash", GRN_OBJ_TABLE_HASH_KEY);
  ADD_DATA("patricia trie", GRN_OBJ_TABLE_PAT_KEY);

#undef ADD_DATA
}

void
test_temporary_table_add(gpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = GPOINTER_TO_INT(data);
  gchar key[] = "key";

  if ((flags & GRN_OBJ_TABLE_TYPE_MASK) == GRN_OBJ_TABLE_NO_KEY) {
    table = grn_table_create(&context, NULL, 0, NULL,
                             flags,
                             NULL,
                             NULL);
    grn_table_add(&context, table, NULL, 0, NULL);
  } else {
    table = grn_table_create(&context, NULL, 0, NULL,
                             flags,
                             OBJECT("ShortText"),
                             NULL);
    grn_table_add(&context, table, key, strlen(key), NULL);
  }

  cut_assert_equal_int(1, grn_table_size(&context, table));
}

typedef struct _ArraySortTestData
{
  int offset, limit;
  gint32 expected_values[1000];
  int n_expected_values;
} ArraySortTestData;


static ArraySortTestData sort_data_no_offset_no_limit = {
  0, -1,
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
   40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
   59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
   78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
   97, 98, 99}, 100
};

static ArraySortTestData sort_data_offset = {
  20, -1,
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
   39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
   58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
   77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
   96, 97, 98, 99}, 80
};

static ArraySortTestData sort_data_limit = {
  0, 20,
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}, 20
};

static ArraySortTestData sort_data_offset_limit = {
  20, 20,
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
   39}, 20
};

void
data_array_sort_offset_and_limit(void)
{
#define ADD_DATA(label, p)                                          \
  cut_add_data(label, p, NULL, NULL)

  ADD_DATA("no offset, no limit", &sort_data_no_offset_no_limit);
  ADD_DATA("offset", &sort_data_offset);
  ADD_DATA("limit", &sort_data_limit);
  ADD_DATA("offset, limit", &sort_data_offset_limit);

#undef ADD_DATA
}

void
test_array_sort_offset_and_limit(ArraySortTestData *data)
{
  const gint32 values[] = {
    80, 37, 1, 95, 29, 21, 27, 11, 70, 0, 73, 79, 16, 17, 23, 43, 20, 55, 15,
    72, 92, 69, 74, 82, 8, 41, 65, 57, 39, 75, 46, 83, 4, 67, 48, 32, 25, 62,
    40, 30, 94, 99, 98, 91, 61, 5, 7, 31, 33, 13, 38, 36, 34, 24, 59, 10, 45,
    63, 22, 93, 85, 58, 19, 44, 53, 6, 3, 18, 78, 49, 90, 47, 28, 54, 56, 51,
    71, 66, 2, 52, 77, 64, 76, 35, 97, 68, 42, 84, 86, 14, 87, 96, 50, 26, 9,
    60, 88, 12, 89, 81
  };
  const int n_values = sizeof(values)/sizeof(values[0]);

  grn_obj *grn_type_int32 = grn_ctx_get(&context, "Int32", strlen("Int32"));
  grn_obj *table, *column;
  const gchar column_name[] = "sample_column";

  grn_table_sort_key keys[1];
  const int n_keys = 1;

  int i;
  grn_obj *result;
  int n_results;
  grn_table_cursor *cursor;

  table = grn_table_create(&context, NULL, 0, NULL,
                           GRN_OBJ_TABLE_NO_KEY,
                           NULL,
                           NULL);
  column = grn_column_create(&context,
                             table,
                             column_name,
                             strlen(column_name),
                             NULL, 0,
                             grn_type_int32);

  keys[0].key = column;
  keys[0].flags = GRN_TABLE_SORT_ASC;

  for(i = 0; i < n_values; ++i) {
    grn_obj record_value;
    grn_id record_id;
    record_id = grn_table_add(&context, table, NULL, 0, NULL);

    GRN_INT32_INIT(&record_value, 0);
    GRN_INT32_SET(&context, &record_value, values[i]);
    grn_test_assert(grn_obj_set_value(&context, column, record_id,
                                      &record_value, GRN_OBJ_SET));
    GRN_OBJ_FIN(&context, &record_value);
  }
  cut_assert_equal_int(n_values, grn_table_size(&context, table));

  result = grn_table_create(&context, NULL, 0, NULL, GRN_TABLE_NO_KEY,
                            NULL, table);
  n_results = grn_table_sort(&context, table, data->offset, data->limit,
                             result, keys, n_keys);
  cut_assert_equal_int(data->n_expected_values, n_results);
  cut_assert_equal_int(data->n_expected_values, grn_table_size(&context, result));

  i = 0;
  cursor = grn_table_cursor_open(&context, result, NULL, 0, NULL, 0,
                                 0, 0, GRN_CURSOR_ASCENDING);
  while (grn_table_cursor_next(&context, cursor) != GRN_ID_NIL) {
    void *value;
    grn_id *id;
    grn_obj record_value;

    grn_table_cursor_get_value(&context, cursor, &value);
    id = value;

    GRN_INT32_INIT(&record_value, 0);
    grn_obj_get_value(&context, column, *id, &record_value);
    cut_assert_equal_int(data->expected_values[i++],
                         GRN_INT32_VALUE(&record_value));
    GRN_OBJ_FIN(&context, &record_value);
  }
  cut_assert_equal_int(data->n_expected_values, i);
  grn_table_cursor_close(&context, cursor);
  grn_obj_close(&context, result);
}

void
test_nonexistent_column(void)
{
  grn_obj *table;
  char table_name[] = "users";
  char nonexistent_column_name[] = "nonexistent";

  table = grn_table_create(&context, table_name, strlen(table_name),
                           NULL,
                           GRN_OBJ_TABLE_NO_KEY,
                           NULL, NULL);
  grn_test_assert_null(&context,
                       grn_obj_column(&context, table,
                                      nonexistent_column_name,
                                      strlen(nonexistent_column_name)));
}
