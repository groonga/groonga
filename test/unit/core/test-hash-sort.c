/*
  Copyright (C) 2008-2015  Kouhei Sutou <kou@cozmixng.org>

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

#include <grn_store.h>
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
static grn_array *sort_result;

void
cut_startup(void)
{
  startup_hash_common();
}

void
cut_shutdown(void)
{
  shutdown_hash_common();
}

void
cut_setup(void)
{
  setup_hash_common("hash-sort");
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
cut_teardown(void)
{
  keys_free();
  values_free();
  if (sort_result)
    grn_array_close(context, sort_result);
  teardown_hash_common();
}


typedef int (*compare_func)(grn_ctx *ctx,
                            grn_obj *hash1, void *target1, uint32_t target1_size,
                            grn_obj *hash2, void *target2, uint32_t target2_size,
                            void *compare_arg);

typedef struct _grn_sort_test_data
{
  GList *expected_strings;
  int limit;
  grn_table_sort_optarg *options;
  grn_test_set_parameters_func set_parameters;
  const GList *strings;
} grn_sort_test_data;

static grn_table_sort_optarg *
sort_options_new(int flags, compare_func _compare, void *compare_arg, int offset)
{
  grn_table_sort_optarg *options;

  options = g_new0(grn_table_sort_optarg, 1);
  options->flags = flags;
  options->compar = _compare;
  options->compar_arg = compare_arg;
  options->offset = offset;

  return options;
}

static void
sort_options_free(grn_table_sort_optarg *options)
{
  g_free(options);
}

static grn_sort_test_data *
sort_test_data_new(GList *expected_strings,
                   int limit,
                   grn_table_sort_optarg *options,
                   grn_test_set_parameters_func set_parameters,
                   const GList *strings)
{
  grn_sort_test_data *test_data;

  test_data = g_new0(grn_sort_test_data, 1);
  test_data->expected_strings = expected_strings;
  test_data->limit = limit;
  test_data->options = options;
  test_data->set_parameters = set_parameters;
  test_data->strings = strings;

  return test_data;
}

static void
sort_test_data_free(grn_sort_test_data *test_data)
{
  if (test_data->options)
    sort_options_free(test_data->options);
  g_free(test_data);
}

static GList *
retrieve_all_keys (grn_array *array, grn_id n_entries)
{
  grn_id id;
  grn_array_cursor *cursor;

  keys_free();

  cursor = grn_array_cursor_open(context, array, 0, 0, 0, -1, GRN_CURSOR_ASCENDING);
  id = grn_array_cursor_next(context, cursor);
  while (id != GRN_ID_NIL) {
    grn_id *hash_id;
    void *array_value;
    gchar key[GRN_HASH_MAX_KEY_SIZE_NORMAL];
    int size;

    grn_array_cursor_get_value(context, cursor, &array_value);
    hash_id = array_value;
    size = grn_hash_get_key(context, hash, *hash_id,
                            key, GRN_HASH_MAX_KEY_SIZE_NORMAL);
    key[size] = '\0';
    keys = g_list_append(keys, g_strdup(key));
    id = grn_array_cursor_next(context, cursor);
  }
  grn_array_cursor_close(context, cursor);

  return keys;
}

static GList *
retrieve_all_values (grn_array *array, grn_id n_entries)
{
  grn_id id;
  grn_array_cursor *cursor;

  values_free();

  cursor = grn_array_cursor_open(context, array, 0, 0, 0, -1, GRN_CURSOR_ASCENDING);
  id = grn_array_cursor_next(context, cursor);
  while (id != GRN_ID_NIL) {
    grn_id *hash_id;
    void *array_value;
    gchar value[GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE];
    int size;

    grn_array_cursor_get_value(context, cursor, &array_value);
    hash_id = array_value;
    size = grn_hash_get_value(context, hash, *hash_id, value);
    values = g_list_append(values, g_strdup(value));
    id = grn_array_cursor_next(context, cursor);
  }
  grn_array_cursor_close(context, cursor);

  return values;
}

static void
add_sort_by_uint_key_data(const gchar *additional_label,
                          grn_test_set_parameters_func set_parameters_func)
{
  int sort_by_unsigned_key_flags;

  sort_by_unsigned_key_flags = GRN_TABLE_SORT_AS_NUMBER;
  sort_by_unsigned_key_flags |= GRN_TABLE_SORT_AS_UNSIGNED;

  cut_add_data(cut_take_printf("ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "セナ",
                                                       "ナセナセ",
                                                       "Senna",
                                                       "セナ + Ruby",
                                                       NULL),
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_ASC |
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
                                  sort_options_new(GRN_TABLE_SORT_ASC |
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_DESC |
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
                                  sort_options_new(GRN_TABLE_SORT_DESC |
                                                   sort_by_unsigned_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "Senna",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       "ナセナセ",
                                                       NULL),
                                  -1,
                                  NULL,
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "Senna",
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
  const grn_sort_test_data *test_data = data;
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

  sort_result = grn_array_create(context, NULL, sizeof(grn_id), GRN_ARRAY_TINY);
  n_entries = grn_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_values(sort_result, n_entries));
}

static void
add_sort_by_int_key_data(const gchar *additional_label,
                         grn_test_set_parameters_func set_parameters_func)
{
  int sort_by_number_key_flags;

  sort_by_number_key_flags = GRN_TABLE_SORT_AS_NUMBER;

  cut_add_data(cut_take_printf("ascending%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナ + Ruby",
                                                       "ナセナセ",
                                                       "セナセナ",
                                                       "セナ",
                                                       "Senna",
                                                       NULL),
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_ASC |
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
                                  sort_options_new(GRN_TABLE_SORT_ASC |
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_DESC |
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
                                  sort_options_new(GRN_TABLE_SORT_DESC |
                                                   sort_by_number_key_flags,
                                                   NULL, NULL,
                                                   0),
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "Senna",
                                                       "ナセナセ",
                                                       "セナ",
                                                       "セナ + Ruby",
                                                       NULL),
                                  -1,
                                  NULL,
                                  set_parameters_func,
                                  NULL),
               sort_test_data_free);

  cut_add_data(cut_take_printf("no options - limit%s", additional_label),
               sort_test_data_new(gcut_list_string_new("セナセナ",
                                                       "Senna",
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
  const grn_sort_test_data *test_data = data;
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
  grn_test_hash_factory_set_key_size(factory, key_size);

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  cut_assert_lookup_add_with_value(&key1, value1);
  cut_assert_lookup_add_with_value(&key2, value2);
  cut_assert_lookup_add_with_value(&key3, value3);
  cut_assert_lookup_add_with_value(&key4, value4);
  cut_assert_lookup_add_with_value(&key5, value5);

  sort_result = grn_array_create(context, NULL, sizeof(grn_id), GRN_ARRAY_TINY);
  n_entries = grn_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_values(sort_result, n_entries));
}

static int
compare_string(grn_ctx *ctx,
               grn_obj *hash1, void *target1, uint32_t target1_size,
               grn_obj *hash2, void *target2, uint32_t target2_size,
               void *user_data)
{
  gchar *null_terminated_target1;
  gchar *null_terminated_target2;

  null_terminated_target1 =
    g_string_free(g_string_new_len(target1, target1_size), FALSE);
  null_terminated_target2 =
    g_string_free(g_string_new_len(target2, target2_size), FALSE);
  return strcmp(cut_take_string(null_terminated_target1),
                cut_take_string(null_terminated_target2)) > 0;
}

static void
add_sort_by_variable_size_key_data(const gchar *additional_label,
                                   grn_test_set_parameters_func set_parameters_func)
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_ASC,
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
                                  sort_options_new(GRN_TABLE_SORT_ASC,
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_DESC,
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
                                  sort_options_new(GRN_TABLE_SORT_DESC,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  keys),
               sort_test_data_free);
}

static void
add_sort_by_variable_size_key_data_many(const gchar *additional_label,
                                        grn_test_set_parameters_func set_parameters_func)
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_ASC,
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
                                  sort_options_new(GRN_TABLE_SORT_ASC,
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_DESC,
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
                                  sort_options_new(GRN_TABLE_SORT_DESC,
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
  const grn_sort_test_data *test_data = data;
  const GList *node;
  int n_entries;

  grn_test_hash_factory_set_key_size(factory, GRN_HASH_MAX_KEY_SIZE_NORMAL);
  grn_test_hash_factory_add_flags(factory, GRN_OBJ_KEY_VAR_SIZE);

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  for (node = test_data->strings; node; node = g_list_next(node)) {
    gchar *key = node->data;
    cut_assert_lookup_add(key);
  }

  sort_result = grn_array_create(context, NULL, sizeof(grn_id), GRN_ARRAY_TINY);
  n_entries = grn_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys(sort_result, n_entries));
}

static void
add_sort_by_value_data(const gchar *additional_label,
                       grn_test_set_parameters_func set_parameters_func)
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_ASC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
                                  sort_options_new(GRN_TABLE_SORT_ASC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_DESC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
                                  sort_options_new(GRN_TABLE_SORT_DESC |
                                                   GRN_TABLE_SORT_BY_VALUE,
                                                   compare_string, NULL,
                                                   0),
                                  set_parameters_func,
                                  values),
               sort_test_data_free);
}

static void
add_sort_by_value_data_many(const gchar *additional_label,
                            grn_test_set_parameters_func set_parameters_func)
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_ASC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
                                  sort_options_new(GRN_TABLE_SORT_ASC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
                                  -1,
                                  sort_options_new(GRN_TABLE_SORT_DESC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
                                  sort_options_new(GRN_TABLE_SORT_DESC |
                                                   GRN_TABLE_SORT_BY_VALUE,
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
  const grn_sort_test_data *test_data = data;
  const GList *node;
  int32_t key;
  int n_entries;

  key_size = sizeof(key);
  grn_test_hash_factory_set_key_size(factory, key_size);

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  key = 1;
  for (node = test_data->strings; node; node = g_list_next(node)) {
    gchar *_value = node->data;
    cut_assert_lookup_add_with_value(&key, _value);
    key++;
  }

  sort_result = grn_array_create(context, NULL, sizeof(grn_id), GRN_ARRAY_TINY);
  n_entries = grn_hash_sort(context, hash, test_data->limit,
                            sort_result, test_data->options);
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_values(sort_result, n_entries));
}
