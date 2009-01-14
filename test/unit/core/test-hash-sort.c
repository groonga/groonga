/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <store.h>
#include "test-hash.h"

void data_sort_by_uint_key(void);
void test_sort_by_uint_key(gconstpointer data);
void data_sort_by_int_key(void);
void test_sort_by_int_key(gconstpointer data);
void data_sort_by_variable_size_key(void);
void test_sort_by_variable_size_key(gconstpointer data);
void data_sort_by_value(void);
void test_sort_by_value(gconstpointer data);

static GList *keys;
static GList *values;
static sen_array *sort_result;

void
startup(void)
{
  startup_hash_common();
}

void
shutdown(void)
{
  shutdown_hash_common();
}

void
setup(void)
{
  setup_hash_common("hash");
  keys = NULL;
  values = NULL;
  sort_result = NULL;
}

static void
keys_free(void)
{
  if (keys) {
    gcut_list_string_free(keys);
    keys = NULL;
  }
}

static void
values_free(void)
{
  if (values) {
    gcut_list_string_free(values);
    values = NULL;
  }
}

void
teardown(void)
{
  keys_free();
  values_free();
  if (sort_result)
    sen_array_close(context, sort_result);
  teardown_hash_common();
}


typedef int (*compare_func)(sen_ctx *ctx,
                            sen_obj *hash1, void *target1, uint32_t target1_size,
                            sen_obj *hash2, void *target2, uint32_t target2_size,
                            void *compare_arg);

typedef struct _sen_sort_test_data
{
  GList *expected_strings;
  int limit;
  sen_table_sort_optarg *options;
  sen_test_set_parameters_func set_parameters;
  const GList *strings;
} sen_sort_test_data;

static sen_table_sort_optarg *
sort_options_new(int flags, compare_func _compare, void *compare_arg, int offset)
{
  sen_table_sort_optarg *options;

  options = g_new0(sen_table_sort_optarg, 1);
  options->flags = flags;
  options->compar = _compare;
  options->compar_arg = compare_arg;
  options->offset = offset;

  return options;
}

static void
sort_options_free(sen_table_sort_optarg *options)
{
  g_free(options);
}

static sen_sort_test_data *
sort_test_data_new(GList *expected_strings,
                   int limit,
                   sen_table_sort_optarg *options,
                   sen_test_set_parameters_func set_parameters,
                   const GList *strings)
{
  sen_sort_test_data *test_data;

  test_data = g_new0(sen_sort_test_data, 1);
  test_data->expected_strings = expected_strings;
  test_data->limit = limit;
  test_data->options = options;
  test_data->set_parameters = set_parameters;
  test_data->strings = strings;

  return test_data;
}

static void
sort_test_data_free(sen_sort_test_data *test_data)
{
  if (test_data->options)
    sort_options_free(test_data->options);
  g_free(test_data);
}

static GList *
retrieve_all_keys (sen_array *array, sen_id n_entries)
{
  sen_id id;
  sen_array_cursor *cursor;

  keys_free();

  cursor = sen_array_cursor_open(context, array, 0, 0, SEN_CURSOR_ASCENDING);
  id = sen_array_cursor_next(context, cursor);
  while (id != SEN_ID_NIL) {
    sen_id *hash_id;
    void *array_value;
    gchar key[SEN_HASH_MAX_KEY_SIZE];
    int size;

    sen_array_cursor_get_value(context, cursor, &array_value);
    hash_id = array_value;
    size = sen_hash_get_key(context, hash, *hash_id, key, SEN_HASH_MAX_KEY_SIZE);
    key[size] = '\0';
    keys = g_list_append(keys, g_strdup(key));
    id = sen_array_cursor_next(context, cursor);
  }
  sen_array_cursor_close(context, cursor);

  return keys;
}

static GList *
retrieve_all_values (sen_array *array, sen_id n_entries)
{
  sen_id id;
  sen_array_cursor *cursor;

  values_free();

  cursor = sen_array_cursor_open(context, array, 0, 0, SEN_CURSOR_ASCENDING);
  id = sen_array_cursor_next(context, cursor);
  while (id != SEN_ID_NIL) {
    sen_id *hash_id;
    void *array_value;
    gchar value[SEN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE];
    int size;

    sen_array_cursor_get_value(context, cursor, &array_value);
    hash_id = array_value;
    size = sen_hash_get_value(context, hash, *hash_id, value);
    value[size]= '\0';
    values = g_list_append(values, g_strdup(value));
    id = sen_array_cursor_next(context, cursor);
  }
  sen_array_cursor_close(context, cursor);

  return values;
}

static void
add_sort_by_uint_key_data(const gchar *additional_label,
                          sen_test_set_parameters_func set_parameters_func)
{
  int sort_by_unsigned_key_flags;

  sort_by_unsigned_key_flags = SEN_TABLE_SORT_AS_NUMBER;
  sort_by_unsigned_key_flags |= SEN_TABLE_SORT_AS_UNSIGNED;

  cut_add_data(cut_take_printf("ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "セナ",
                                                       "ナセナセ",
                                                       "Senna",
                                                       "セナ + Ruby",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   sort_by_unsigned_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("ascending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "セナ",
                                                       "ナセナセ",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   sort_by_unsigned_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "Senna",
                                                       "ナセナセ",
                                                       "セナ",
                                                       "セナセナ",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   sort_by_unsigned_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "Senna",
                                                       "ナセナセ",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   sort_by_unsigned_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ナセナセ",
                                                       "セナ + Ruby",
                                                       "セナ",
                                                       "Senna",
                                                       "セナセナ",
                                                       NULL),
                                  0,
                                  NULL,
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ナセナセ",
                                                       "セナ + Ruby",
                                                       "セナ",
                                                       NULL),
                                  3,
                                  NULL,
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);
}

void
data_sort_by_uint_key(void)
{
  add_sort_by_uint_key_data("", NULL);
  add_sort_by_uint_key_data(" - tiny", set_tiny_flags);
}

void
test_sort_by_uint_key(gconstpointer data)
{
  const sen_sort_test_data *test_data = data;
  const uint32_t key1 = 100;
  const uint32_t key2 = 2000000;
  const uint32_t key3 = 30000;
  const uint32_t key4 = 4000;
  const uint32_t key5 = 5;
  gchar value1[] = "セナ";
  gchar value2[] = "セナ + Ruby";
  gchar value3[] = "Senna";
  gchar value4[] = "ナセナセ";
  gchar value5[] = "セナセナ";
  int n_entries;

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  cut_assert_lookup_add_with_value(&key1, value1);
  cut_assert_lookup_add_with_value(&key2, value2);
  cut_assert_lookup_add_with_value(&key3, value3);
  cut_assert_lookup_add_with_value(&key4, value4);
  cut_assert_lookup_add_with_value(&key5, value5);

  sort_result = sen_array_create(context, NULL, sizeof(sen_id), SEN_ARRAY_TINY);
  n_entries = sen_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_values(sort_result, n_entries));
}

static void
add_sort_by_int_key_data(const gchar *additional_label,
                         sen_test_set_parameters_func set_parameters_func)
{
  int sort_by_number_key_flags;

  sort_by_number_key_flags = SEN_TABLE_SORT_AS_NUMBER;

  cut_add_data(cut_take_printf("ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       "セナ",
                                                       "Senna",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   sort_by_number_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("ascending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   sort_by_number_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Senna",
                                                       "セナ",
                                                       "セナセナ",
                                                       "ナセナセ",
                                                       "セナ + Ruby",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   sort_by_number_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Senna",
                                                       "セナ",
                                                       "セナセナ",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   sort_by_number_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "セナ",
                                                       "ナセナセ",
                                                       "Senna",
                                                       "セナセナ",
                                                       NULL),
                                  0,
                                  NULL,
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "セナ",
                                                       "ナセナセ",
                                                       NULL),
                                  3,
                                  NULL,
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);
}

void
data_sort_by_int_key(void)
{
  add_sort_by_int_key_data("", NULL);
  add_sort_by_int_key_data(" - tiny", set_tiny_flags);
}

void
test_sort_by_int_key(gconstpointer data)
{
  const sen_sort_test_data *test_data = data;
  const int32_t key1 = 100;
  const int32_t key2 = -2000000;
  const int32_t key3 = 30000;
  const int32_t key4 = -4000;
  const int32_t key5 = 5;
  gchar value1[] = "セナ";
  gchar value2[] = "セナ + Ruby";
  gchar value3[] = "Senna";
  gchar value4[] = "ナセナセ";
  gchar value5[] = "セナセナ";
  int n_entries;

  key_size = sizeof(int32_t);
  sen_test_hash_factory_set_key_size(factory, key_size);

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  cut_assert_lookup_add_with_value(&key1, value1);
  cut_assert_lookup_add_with_value(&key2, value2);
  cut_assert_lookup_add_with_value(&key3, value3);
  cut_assert_lookup_add_with_value(&key4, value4);
  cut_assert_lookup_add_with_value(&key5, value5);

  sort_result = sen_array_create(context, NULL, sizeof(sen_id), SEN_ARRAY_TINY);
  n_entries = sen_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_values(sort_result, n_entries));
}

static int
compare_string(sen_ctx *ctx,
               sen_obj *hash1, void *target1, uint32_t target1_size,
               sen_obj *hash2, void *target2, uint32_t target2_size,
               void *user_data)
{
  gchar *null_terminated_target1;
  gchar *null_terminated_target2;

  null_terminated_target1 =
    g_string_free(g_string_new_len(target1, target1_size), FALSE);
  null_terminated_target2 =
    g_string_free(g_string_new_len(target2, target2_size), FALSE);
  return g_utf8_collate(cut_take_string(null_terminated_target1),
                        cut_take_string(null_terminated_target2)) > 0;
}

static void
add_sort_by_variable_size_key_data(const gchar *additional_label,
                                   sen_test_set_parameters_func set_parameters_func)
{
  const GList *keys;

  keys = gcut_take_list(gcut_list_string_new("セナ",
                                             "セナ + Ruby",
                                             "Senna",
                                             "ナセナセ",
                                             "セナセナ",
                                             NULL),
                        g_free);
  cut_add_data(cut_take_printf("ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Senna",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       "セナセナ",
                                                       "ナセナセ",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_ASC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);

  cut_add_data(cut_take_printf("ascending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Senna",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_ASC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ナセナセ",
                                                       "セナセナ",
                                                       "セナ + Ruby",
                                                       "セナ",
                                                       "Senna",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_DESC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ナセナセ",
                                                       "セナセナ",
                                                       "セナ + Ruby",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_DESC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);
}

static void
add_sort_by_variable_size_key_data_many(const gchar *additional_label,
                                        sen_test_set_parameters_func set_parameters_func)
{
  const GList *keys;

  keys = gcut_take_list(gcut_list_string_new("セナ",
                                             "セナ + Ruby",
                                             "Senna",
                                             "ナセナセ",
                                             "セナセナ",
                                             "Ruby",
                                             "Tritton",
                                             "ブラジル",
                                             "Ludia",
                                             NULL),
                        g_free);
  cut_add_data(cut_take_printf("many - ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Ludia",
                                                       "Ruby",
                                                       "Senna",
                                                       "Tritton",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       "セナセナ",
                                                       "ナセナセ",
                                                       "ブラジル",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_ASC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);

  cut_add_data(cut_take_printf("many - ascending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Ludia",
                                                       "Ruby",
                                                       "Senna",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_ASC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);

  cut_add_data(cut_take_printf("many - descending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ブラジル",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       "セナ + Ruby",
                                                       "セナ",
                                                       "Tritton",
                                                       "Senna",
                                                       "Ruby",
                                                       "Ludia",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_DESC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);

  cut_add_data(cut_take_printf("many - descending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ブラジル",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_DESC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);
}

void
data_sort_by_variable_size_key(void)
{
  add_sort_by_variable_size_key_data("", NULL);
  add_sort_by_variable_size_key_data(" - tiny", set_tiny_flags);

  add_sort_by_variable_size_key_data_many("", NULL);
  add_sort_by_variable_size_key_data_many(" - tiny", set_tiny_flags);
}

void
test_sort_by_variable_size_key(gconstpointer data)
{
  const sen_sort_test_data *test_data = data;
  const GList *node;
  int n_entries;

  sen_test_hash_factory_set_key_size(factory, SEN_HASH_MAX_KEY_SIZE);
  sen_test_hash_factory_add_flags(factory, SEN_OBJ_KEY_VAR_SIZE);

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  for (node = test_data->strings; node; node = g_list_next(node)) {
    gchar *key = node->data;
    cut_assert_lookup_add(key);
  }

  sort_result = sen_array_create(context, NULL, sizeof(sen_id), SEN_ARRAY_TINY);
  n_entries = sen_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys(sort_result, n_entries));
}

static void
add_sort_by_value_data(const gchar *additional_label,
                       sen_test_set_parameters_func set_parameters_func)
{
  const GList *values;

  values = gcut_take_list(gcut_list_string_new("セナ",
                                               "セナ + Ruby",
                                               "Senna",
                                               "ナセナセ",
                                               "セナセナ",
                                               NULL),
                          g_free);

  cut_add_data(cut_take_printf("ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Senna",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       "セナセナ",
                                                       "ナセナセ",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);

  cut_add_data(cut_take_printf("ascending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Senna",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ナセナセ",
                                                       "セナセナ",
                                                       "セナ + Ruby",
                                                       "セナ",
                                                       "Senna",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);

  cut_add_data(cut_take_printf("descending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ナセナセ",
                                                       "セナセナ",
                                                       "セナ + Ruby",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);
}

static void
add_sort_by_value_data_many(const gchar *additional_label,
                            sen_test_set_parameters_func set_parameters_func)
{
  const GList *values;

  values = gcut_take_list(gcut_list_string_new("セナ",
                                               "セナ + Ruby",
                                               "Senna",
                                               "ナセナセ",
                                               "セナセナ",
                                               "Ruby",
                                               "Tritton",
                                               "ブラジル",
                                               "Ludia",
                                               NULL),
                          g_free);

  cut_add_data(cut_take_printf("many - ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Ludia",
                                                       "Ruby",
                                                       "Senna",
                                                       "Tritton",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       "セナセナ",
                                                       "ナセナセ",
                                                       "ブラジル",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);

  cut_add_data(cut_take_printf("many - ascending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("Ludia",
                                                       "Ruby",
                                                       "Senna",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_ASC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);

  cut_add_data(cut_take_printf("many - descending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ブラジル",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       "セナ + Ruby",
                                                       "セナ",
                                                       "Tritton",
                                                       "Senna",
                                                       "Ruby",
                                                       "Ludia",
                                                       NULL),
                                  0,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);

  cut_add_data(cut_take_printf("many - descending - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("ブラジル",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       NULL),
                                  3,
                                  sort_options_new(SEN_TABLE_SORT_DESC |
                                                   SEN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);
}

void
data_sort_by_value(void)
{
  add_sort_by_value_data("", NULL);
  add_sort_by_value_data(" - tiny", set_tiny_flags);

  add_sort_by_value_data_many("", NULL);
  add_sort_by_value_data_many(" - tiny", set_tiny_flags);
}

void
test_sort_by_value(gconstpointer data)
{
  const sen_sort_test_data *test_data = data;
  const GList *node;
  int32_t key;
  int n_entries;

  key_size = sizeof(key);
  sen_test_hash_factory_set_key_size(factory, key_size);

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  key = 1;
  for (node = test_data->strings; node; node = g_list_next(node)) {
    gchar *_value = node->data;
    cut_assert_lookup_add_with_value(&key, _value);
    key++;
  }

  sort_result = sen_array_create(context, NULL, sizeof(sen_id), SEN_ARRAY_TINY);
  n_entries = sen_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_values(sort_result, n_entries));
}
