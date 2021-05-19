/*
  Copyright (C) 2008-2015  Kouhei Sutou <kou@clear-code.com>

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

#include <math.h>

#include "test-hash.h"

void data_create(void);
void test_create(gconstpointer data);
void data_open(void);
void test_open(gconstpointer data);
void test_open_without_path(void);
void test_open_tiny_hash(void);
void data_lookup_add(void);
void test_lookup_add(gconstpointer data);
void data_delete_by_id(void);
void test_delete_by_id(gconstpointer data);
void data_delete(void);
void test_delete(gconstpointer data);
void data_get_key(void);
void test_get_key(gconstpointer data);
void data_get_value(void);
void test_get_value(gconstpointer data);
void data_set_value(void);
void test_set_value(gconstpointer data);
void data_set_value_with_null_value(void);
void test_set_value_with_null_value(gconstpointer data);
void data_add_and_delete(void);
void test_add_and_delete(gconstpointer data);
void data_truncate(void);
void test_truncate(gconstpointer data);

static GArray *ids;

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
  setup_hash_common("hash");
  ids = NULL;
}

static void
ids_free(void)
{
  if (ids)
    g_array_free(ids, TRUE);
  ids = NULL;
}

void
cut_teardown(void)
{
  ids_free();
  teardown_hash_common();
}

static void
set_key_size_to_zero(void)
{
  grn_test_hash_factory_set_key_size(factory, 0);
}

static void
set_value_size_to_zero(void)
{
  grn_test_hash_factory_set_value_size(factory, 0);
}

static void
set_variable_size(void)
{
  grn_test_hash_factory_set_key_size(factory, GRN_HASH_MAX_KEY_SIZE_NORMAL);
  grn_test_hash_factory_add_flags(factory, GRN_OBJ_KEY_VAR_SIZE);
}

static void
set_tiny_flags_and_variable_size(void)
{
  set_tiny_flags();
  set_variable_size();
}

static void
set_not_uint32_key_size(void)
{
  grn_test_hash_factory_set_key_size(factory, not_uint32_key_size);
  key_size = not_uint32_key_size;
}

static void
set_tiny_flags_and_not_uint32_key_size(void)
{
  set_tiny_flags();
  set_not_uint32_key_size();
}


void
data_create(void)
{
  cut_add_data("default", NULL, NULL,
               "zero key size", set_key_size_to_zero, NULL,
               "zero value size", set_value_size_to_zero, NULL);
}

void
test_create(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;

  if (set_parameters)
    set_parameters();
  cut_assert_create_hash();
}

void
data_open(void)
{
  cut_add_data("default", NULL, NULL,
               "zero key size", set_key_size_to_zero, NULL,
               "zero value size", set_value_size_to_zero, NULL);
}

void
test_open(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;

  if (set_parameters)
    set_parameters();
  cut_assert_create_hash();
  cut_assert_open_hash();
}

void
test_open_without_path(void)
{
  const gchar *saved_default_path;

  grn_test_hash_factory_set_flags(factory, 0);

  saved_default_path = cut_take_strdup(grn_test_hash_factory_get_path(factory));
  grn_test_hash_factory_set_path(factory, NULL);

  cut_assert_path_not_exist(saved_default_path);
  cut_assert_create_hash();
  cut_assert_path_not_exist(saved_default_path);
  cut_assert_fail_open_hash();
  cut_assert_path_not_exist(saved_default_path);
}

void
test_open_tiny_hash(void)
{
  set_tiny_flags();

  cut_assert_create_hash();
  cut_assert_fail_open_hash();
}

typedef struct _grn_test_data grn_test_data;

typedef void (*increment_key_func) (grn_test_data *test_data);

struct _grn_test_data
{
  gpointer key;
  increment_key_func increment;
  grn_test_set_parameters_func set_parameters;
};

static grn_test_data *
test_data_new(gpointer key, increment_key_func increment,
              grn_test_set_parameters_func set_parameters)
{
  grn_test_data *test_data;

  test_data = g_new0(grn_test_data, 1);
  test_data->key = key;
  test_data->increment = increment;
  test_data->set_parameters = set_parameters;

  return test_data;
}

static void
test_data_free(grn_test_data *test_data)
{
  g_free(test_data->key);
  g_free(test_data);
}

static void
test_data_add_n_data(guint n, grn_test_data *test_data)
{
  guint i;

  ids_free();
  ids = g_array_new(TRUE, TRUE, sizeof(grn_id));
  for (i = 0; i < n; i++) {
    if (grn_test_hash_factory_get_flags(factory) & GRN_OBJ_KEY_VAR_SIZE)
      key_size = strlen(test_data->key);
    cut_assert_lookup_add(test_data->key);
    test_data->increment((grn_test_data *)test_data);
    g_array_append_val(ids, id);
  }
}

static gpointer
uint32_key_new(uint32_t key)
{
  uint32_t *key_pointer;

  key_pointer = g_new0(uint32_t, 1);
  *key_pointer = key;

  return key_pointer;
}

static void
uint32_increment(grn_test_data *test_data)
{
  uint32_t *key_pointer = test_data->key;
  (*key_pointer)++;
}

static void
string_increment(grn_test_data *test_data)
{
  gchar *original_string = test_data->key;
  gchar *string;
  gint last;

  last = strlen(original_string);
  if (original_string[last - 1] < 'X') {
    original_string[last - 1]++;
  } else {
    string = g_strconcat(test_data->key, "A", NULL);
    g_free(test_data->key);
    test_data->key = string;
  }
}

static void
not_uint32_size_key_increment(grn_test_data *test_data)
{
  gchar *string = test_data->key;
  gint i;

  for (i = 0; i < not_uint32_key_size; i++) {
    if (string[i] < '~') {
      string[i]++;
      return;
    }
  }
  cut_error("can't increment more!: %s", string);
}

static void
add_variable_key_size_test_data(void)
{
  cut_add_data("uint32 - default",
               test_data_new(uint32_key_new(29), uint32_increment,
                             NULL),
               test_data_free,
               "uint32 - tiny",
               test_data_new(uint32_key_new(29), uint32_increment,
                             set_tiny_flags),
               test_data_free,
               "variable size - short - default",
               test_data_new(g_strdup("X"), string_increment,
                             set_variable_size),
               test_data_free,
               "variable size - short - tiny",
               test_data_new(g_strdup("X"), string_increment,
                             set_tiny_flags_and_variable_size),
               test_data_free,
               "variable size - long - default",
               test_data_new(g_strdup("must be long rather than sizeof(char *)"),
                             string_increment,
                             set_variable_size),
               test_data_free,
               "variable size - long - tiny",
               test_data_new(g_strdup("must be long rather than sizeof(char *)"),
                             string_increment,
                             set_tiny_flags_and_variable_size),
               test_data_free,
               "not uint32 size - default",
               test_data_new(g_strdup(not_uint32_size_key),
                             not_uint32_size_key_increment,
                             set_not_uint32_key_size),
               test_data_free,
               "not uint32 size - tiny",
               test_data_new(g_strdup(not_uint32_size_key),
                             not_uint32_size_key_increment,
                             set_tiny_flags_and_not_uint32_key_size),
               test_data_free);
}

void
data_lookup_add(void)
{
  add_variable_key_size_test_data();
}

void
test_lookup_add(gconstpointer data)
{
  const grn_test_data *test_data = data;

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  if (grn_test_hash_factory_get_flags(factory) & GRN_OBJ_KEY_VAR_SIZE)
    key_size = strlen(test_data->key);
  cut_assert_lookup_add(test_data->key);
}


#define put_sample_entry() do                   \
{                                               \
  grn_search_flags flags;                 \
                                                \
  flags = GRN_TABLE_ADD;                        \
  cut_assert_lookup(&sample_key, &flags);       \
  cut_assert(flags & 1);                        \
  sample_id = id;                               \
} while (0)

#define cut_assert_delete_by_id() do                                    \
{                                                                       \
  grn_search_flags flags;                                               \
                                                                        \
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,                        \
                           grn_hash_delete_by_id(context, hash, 0, NULL)); \
                                                                        \
  put_sample_entry();                                                   \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup(&sample_key, &flags);                               \
                                                                        \
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,                        \
                           grn_hash_delete_by_id(context,               \
                                                 NULL, sample_id, NULL)); \
  grn_test_assert(grn_hash_delete_by_id(context, hash, sample_id, NULL)); \
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,                        \
                           grn_hash_delete_by_id(context,               \
                                                 hash, sample_id, NULL)); \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup_failed(&sample_key, &flags);                        \
} while (0)

void
data_delete_by_id(void)
{
  cut_add_data("default", NULL, NULL,
               "tiny", set_tiny_flags, NULL);
}

void
test_delete_by_id(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;

  if (set_parameters)
    set_parameters();

  cut_assert_create_hash();
  cut_assert_delete_by_id();
}


#define cut_assert_delete(key) do                                       \
{                                                                       \
  const void *_key;                                                     \
  grn_search_flags flags;                                               \
                                                                        \
  _key = (key);                                                         \
                                                                        \
  flags = GRN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, &flags);                                      \
                                                                        \
  grn_test_assert(grn_hash_delete(context, hash, _key, key_size, NULL)); \
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,                         \
                           grn_hash_delete(context,                     \
                                           hash, _key, key_size, NULL)); \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup_failed(_key, &flags);                               \
} while (0)

void
data_delete(void)
{
  add_variable_key_size_test_data();
}

void
test_delete(gconstpointer data)
{
  const grn_test_data *test_data = data;

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  if (grn_test_hash_factory_get_flags(factory) & GRN_OBJ_KEY_VAR_SIZE)
    key_size = strlen(test_data->key);
  cut_assert_delete(test_data->key);
}

void
data_get_key(void)
{
  cut_add_data("default", NULL, NULL,
               "tiny", set_tiny_flags, NULL);
}

void
test_get_key(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;
  uint32_t key = 29;
  uint32_t initial_key = 999999;
  uint32_t got_key;
  grn_search_flags flags;

  if (set_parameters)
    set_parameters();

  cut_assert_create_hash();

  flags = GRN_TABLE_ADD;
  cut_assert_lookup(&key, &flags);

  got_key = initial_key;
  cut_assert_equal_int(key_size,
                       grn_hash_get_key(context, hash, id, &got_key, key_size));
  cut_assert_equal_uint(key, got_key);
}

void
data_get_value(void)
{
  cut_add_data("default", NULL, NULL,
               "tiny", set_tiny_flags, NULL);
}

void
test_get_value(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;
  uint32_t key = 29;
  gchar set_value[] = "ABCDE";
  gchar initial_value[] = "XYZ";
  gchar got_value[GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE];
  grn_search_flags flags;

  if (set_parameters)
    set_parameters();

  cut_assert_create_hash();

  flags = GRN_TABLE_ADD;
  cut_assert_lookup(&key, &flags);
  strcpy(value, set_value);

  strcpy(got_value, initial_value);
  cut_assert_equal_int(GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE,
                       grn_hash_get_value(context, hash, id, got_value));
  cut_assert_equal_string(set_value, got_value);
}

void
data_set_value(void)
{
  cut_add_data("default", NULL, NULL,
               "tiny", set_tiny_flags, NULL);
}

void
test_set_value(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;
  gchar got_value[GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE];

  if (set_parameters)
    set_parameters();

  cut_assert_create_hash();

  put_sample_entry();

  grn_test_assert(grn_hash_set_value(context, hash,
                                     sample_id, "XXX", GRN_OBJ_SET));
  cut_assert_equal_int(GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE,
                       grn_hash_get_value(context, hash, sample_id, got_value));
  cut_assert_equal_string("XXX", got_value);
}

void
data_set_value_with_null_value(void)
{
  cut_add_data("default", NULL, NULL,
               "tiny", set_tiny_flags, NULL);
}

void
test_set_value_with_null_value(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;
  grn_id nonexistence_id = 999;

  if (set_parameters)
    set_parameters();
  cut_assert_create_hash();

  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_hash_set_value(context, hash, nonexistence_id,
                                              NULL, GRN_OBJ_SET));

  put_sample_entry();
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_hash_set_value(context, hash, sample_id,
                                              NULL, GRN_OBJ_SET));
}

void
data_add_and_delete(void)
{
  add_variable_key_size_test_data();
}

void
test_add_and_delete(gconstpointer data)
{
  grn_test_data *test_data;
  guint i;
  const guint n_operations = 750;

  test_data = (grn_test_data *)data;

  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  test_data_add_n_data(n_operations, test_data);

  cut_assert_equal_int(n_operations, GRN_HASH_SIZE(hash));
  for (i = 0; i < ids->len; i++) {
    grn_id delete_id;

    delete_id = g_array_index(ids, grn_id, i);
    grn_test_assert(grn_hash_delete_by_id(context, hash, delete_id, NULL),
                    cut_message("i = %d; id = %d", i, delete_id));
  }
  cut_assert_equal_int(0, GRN_HASH_SIZE(hash));
}

void
data_truncate(void)
{
  add_variable_key_size_test_data();
}

void
test_truncate(gconstpointer data)
{
  grn_test_data *test_data;
  guint n_data = 100;

  test_data = (grn_test_data *)data;
  if (test_data->set_parameters)
    test_data->set_parameters();

  cut_assert_create_hash();

  cut_assert_equal_uint(0, GRN_HASH_SIZE(hash));
  test_data_add_n_data(n_data, test_data);
  cut_assert_equal_uint(n_data, GRN_HASH_SIZE(hash));
  grn_test_assert(grn_hash_truncate(context, hash));
  cut_assert_equal_uint(0, GRN_HASH_SIZE(hash));
}
