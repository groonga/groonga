/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

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

#include "test-patricia-trie.h"

void deta_create(void);
void test_create(gconstpointer data);
void data_open_success(void);
void test_open_success(gconstpointer data);
void data_open_without_path(void);
void test_open_without_path(gconstpointer data);
void test_remove(void);
void test_remove_with_null_as_path(void);
void data_lookup_add(void);
void test_lookup_add(gconstpointer data);
void data_delete_by_id(void);
void test_delete_by_id(gconstpointer data);
void test_delete_by_id_sis_short(void);
void attributes_delete_by_id_sis_long(void);
void test_delete_by_id_sis_long(void);
void data_delete(void);
void test_delete(gconstpointer data);
void data_lookup_and_delete_for_same_prefix_key(void);
void test_lookup_and_delete_for_same_prefix_key(gconstpointer data);
void test_get_key(void);
void data_get_value(void);
void test_get_value(gconstpointer data);
void test_set_value(void);
void test_set_value_with_null_value(void);
void data_add_and_delete(void);
void test_add_and_delete(gconstpointer data);

static GArray *ids;
static GList *expected_keys, *actual_keys;

void
cut_setup(void)
{
  setup_trie_common("patricia-trie");

  ids = NULL;
  expected_keys = NULL;
  actual_keys = NULL;
}

void
cut_teardown(void)
{
  if (ids)
    g_array_free(ids, TRUE);
  if (expected_keys)
    gcut_list_string_free(expected_keys);
  if (actual_keys)
    gcut_list_string_free(actual_keys);
  teardown_trie_common();
}


static void
set_key_size_to_zero(void)
{
  default_key_size = 0;
}

static void
set_value_size_to_zero(void)
{
  default_value_size = 0;
}

static void
set_sis_and_utf8_encoding(void)
{
  set_sis();
  default_encoding = GRN_ENC_UTF8;
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

  cut_assert_create_trie();
}

void
data_open_success(void)
{
  cut_add_data("default", NULL, NULL,
               "sis", set_sis, NULL);
}

void
test_open_success(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;

  if (set_parameters)
    set_parameters();

  cut_assert_create_trie();
  cut_assert_open_trie();
}

void
data_open_without_path(void)
{
  cut_add_data("default", NULL, NULL,
               "sis", set_sis, NULL);
}

void
test_open_without_path(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;
  const gchar *saved_default_path;

  if (set_parameters)
    set_parameters();

  saved_default_path = cut_take_string(default_path);
  default_path = NULL;

  cut_assert_path_not_exist(saved_default_path);
  cut_assert_create_trie();
  cut_assert_path_not_exist(saved_default_path);
  cut_assert_fail_open_trie();
  cut_assert_path_not_exist(saved_default_path);
}

void
test_remove(void)
{
  cut_assert_path_not_exist(default_path);
  cut_assert_create_trie();
  cut_assert_path_exist(default_path);
  grn_test_assert(grn_pat_remove(context, default_path));
  cut_assert_path_not_exist(default_path);
}

void
test_remove_with_null_as_path(void)
{
  cut_assert_open_context();
  gcut_assert_equal_list_string(NULL, messages());
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_remove(context, NULL));
  expected_messages = gcut_list_string_new("path is null", NULL);
  cut_assert_equal_g_list_string(expected_messages, messages());
}

static grn_trie_test_data *
test_data_new(const gchar *key, grn_test_set_parameters_func set_parameters)
{
  return trie_test_data_newv(key, NULL, NULL, GRN_SUCCESS, NULL, NULL,
                             set_parameters, NULL);
}

static void
test_data_free(grn_trie_test_data *data)
{
  trie_test_data_free(data);
}

void
data_lookup_add(void)
{
  cut_add_data("default",
               test_data_new("Cutter", NULL),
               test_data_free,
               "sis",
               test_data_new("Groonga", set_sis),
               test_data_free,
               "sis - multi byte key",
               test_data_new("セナ", set_sis_and_utf8_encoding),
               test_data_free);
}

void
test_lookup_add(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();
  cut_assert_lookup_add(test_data->key);
}

void
data_delete_by_id(void)
{
  cut_add_data("default",
               test_data_new("Cutter", NULL),
               test_data_free,
               "sis",
               test_data_new("Groonga", set_sis),
               test_data_free,
               "sis - multi byte key",
               test_data_new("セナ", set_sis_and_utf8_encoding),
               test_data_free);
}

void
test_delete_by_id(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  grn_search_flags flags;
  uint32_t key_size;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_delete_by_id(context, trie, 0, NULL));

  cut_assert_lookup_add(test_data->key);

  flags = 0;
  key_size = strlen(test_data->key);
  cut_assert_lookup(test_data->key, key_size, &flags);

  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_delete_by_id(context, NULL, id, NULL));
  grn_test_assert(grn_pat_delete_by_id(context, trie, id, NULL));
  flags = 0;
  cut_assert_lookup_failed(test_data->key, key_size, &flags);

  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_delete_by_id(context, trie, id, NULL));
}

void
test_delete_by_id_sis_short(void)
{
  grn_id short_term_id, long_term_id;

  set_sis_and_utf8_encoding();

  cut_assert_create_trie();

  cut_assert_lookup_add("セナ");
  short_term_id = id;
  cut_assert_lookup_add("セナセナ");
  long_term_id = id;

  expected_keys = gcut_list_string_new("セナ", "セナセナ", "ナ", "ナセナ", NULL);
  actual_keys = grn_test_pat_get_keys(context, (grn_obj *)trie);
  gcut_assert_equal_list_string(expected_keys, actual_keys);

  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_delete_by_id(context, trie, short_term_id, NULL));

  gcut_list_string_free(actual_keys);
  actual_keys = grn_test_pat_get_keys(context, (grn_obj *)trie);
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}

void
attributes_delete_by_id_sis_long(void)
{
  cut_set_attributes("ML", "1010",
                     NULL);
}

void
test_delete_by_id_sis_long(void)
{
  grn_id short_term_id, long_term_id;

  set_sis_and_utf8_encoding();

  cut_assert_create_trie();

  cut_assert_lookup_add("セナ");
  short_term_id = id;
  cut_assert_lookup_add("セナセナ");
  long_term_id = id;

  expected_keys = gcut_list_string_new("セナ", "セナセナ", "ナ", "ナセナ", NULL);
  actual_keys = grn_test_pat_get_keys(context, (grn_obj *)trie);
  gcut_assert_equal_list_string(expected_keys, actual_keys);

  grn_test_assert(grn_pat_delete_by_id(context, trie, long_term_id, NULL));

  gcut_list_string_free(actual_keys);
  actual_keys = grn_test_pat_get_keys(context, (grn_obj *)trie);
  gcut_assert_equal_list_string(NULL, actual_keys);
}

#define cut_assert_delete(key) do                                       \
{                                                                       \
  const gchar *_key;                                                    \
  uint32_t key_size = 0;                                                \
  grn_search_flags flags;                                         \
                                                                        \
  _key = (key);                                                         \
  if (_key)                                                             \
    key_size = strlen(_key);                                            \
                                                                        \
  grn_test_assert(grn_pat_delete(context, trie, _key, key_size, NULL));  \
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,                        \
                           grn_pat_delete(context, trie,                \
                                         _key, key_size, NULL));        \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup_failed(_key, key_size, &flags);                     \
} while (0)

void
data_delete(void)
{
  cut_add_data("default",
               test_data_new("29", NULL),
               test_data_free,
               "sis",
               test_data_new("29", set_sis),
               test_data_free,
               "sis - multi byte key",
               test_data_new("肉ニク", set_sis_and_utf8_encoding),
               test_data_free);
}

void
test_delete(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();
  cut_assert_lookup_add(test_data->key);
  cut_assert_delete(test_data->key);
}

void
data_lookup_and_delete_for_same_prefix_key(void)
{
  cut_add_data("default", NULL, NULL,
               "sis", set_sis, NULL);
}

void
test_lookup_and_delete_for_same_prefix_key(gconstpointer data)
{
  const grn_test_set_parameters_func set_parameters = data;
  grn_search_flags flags;
  const gchar key1[] = "セナ + PostgreSQL";
  const gchar key2[] = "セナ + MySQL";
  const gchar key3[] = "セナ + Ruby";

  if (set_parameters)
    set_parameters();

  default_encoding = GRN_ENC_UTF8;

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_delete(key3);

  flags = 0;
  cut_assert_lookup(key1, strlen(key1), &flags);

  flags = 0;
  cut_assert_lookup(key2, strlen(key2), &flags);

  flags = 0;
  cut_assert_lookup_failed(key3, strlen(key3), &flags);
}

void
test_get_key(void)
{
  const gchar key[] = "Groonga";
  const gchar initial_key[] = "Ludia";
  gchar got_key[GRN_PAT_MAX_KEY_SIZE];
  grn_id nonexistence_id = 12345;
  int got_key_size;
  grn_search_flags flags;

  cut_assert_create_trie();

  strcpy(got_key, initial_key);
  cut_assert_equal_int(0, grn_pat_get_key(context, trie, nonexistence_id,
                                          &got_key, GRN_PAT_MAX_KEY_SIZE));
  cut_assert_equal_string(initial_key, got_key);

  flags = GRN_TABLE_ADD;
  cut_assert_lookup(key, strlen(key), &flags);

  strcpy(got_key, initial_key);
  got_key_size = grn_pat_get_key(context, trie, nonexistence_id,
                                 &got_key, GRN_PAT_MAX_KEY_SIZE);
  /* don't need to show the below. */
  /*
  cut_notify("grn_pat_get_key() with nonexistence ID is undefined:\n"
             "   got_key_size: %d (may != 0)\n"
             "        got_key: <%s> (may != initial_key)\n"
             "    initial_key: <%s>",
             got_key_size,
             got_key,
             initial_key);
  */

  got_key_size = grn_pat_get_key(context, trie, id,
                                 &got_key, GRN_PAT_MAX_KEY_SIZE);
  cut_assert_equal_int(strlen(key), got_key_size);
  got_key[got_key_size] = '\0';
  cut_assert_equal_string(key, got_key);
}

void
data_get_value(void)
{
  cut_add_data("default",
               test_data_new("Cutter", NULL),
               test_data_free,
               "sis",
               test_data_new("Groonga", set_sis),
               test_data_free,
               "sis - multi byte",
               test_data_new("セナ", set_sis_and_utf8_encoding),
               test_data_free);
}

void
test_get_value(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  const gchar *set_value;
  const gchar *initial_value;
  gchar got_value[DEFAULT_VALUE_SIZE];
  grn_id nonexistence_id = 12345;
  int got_value_size;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  initial_value = cut_take_string(g_strjoin(" - ",
                                            test_data->key,
                                            test_data->key,
                                            NULL));
  strcpy(got_value, initial_value);
  cut_assert_equal_int(0, grn_pat_get_value(context, trie,
                                            nonexistence_id, got_value));
  cut_assert_equal_string(initial_value, got_value);

  cut_assert_lookup_add(test_data->key);
  set_value = cut_take_string(g_strjoin(" - ",
                                        test_data->key,
                                        test_data->key,
                                        test_data->key,
                                        NULL));
  strcpy(value, set_value);

  strcpy(got_value, initial_value);
  got_value_size = grn_pat_get_value(context, trie,
                                     nonexistence_id, got_value);
  /* don't need to show the below. */
  /*
  cut_notify("grn_pat_get_value() with nonexistence ID is undefined:\n"
             "   got_value_size: %d (may != 0)\n"
             "        got_value: <%s> (may != initial_value)\n"
             "    initial_value: <%s>",
             got_value_size,
             got_value,
             initial_value);
  */

  cut_assert_equal_int(DEFAULT_VALUE_SIZE,
                       grn_pat_get_value(context, trie, id, got_value));
  cut_assert_equal_string(set_value, got_value);
}

void
test_set_value(void)
{
  gchar got_value[DEFAULT_VALUE_SIZE];

  cut_assert_create_trie();

  put_sample_entry();

  grn_test_assert(grn_pat_set_value(context, trie,
                                   sample_id, "XXX", GRN_OBJ_SET));
  cut_assert_equal_int(DEFAULT_VALUE_SIZE,
                       grn_pat_get_value(context, trie, sample_id, got_value));
  cut_assert_equal_string("XXX", got_value);
}

void
test_set_value_with_null_value(void)
{
  cut_assert_create_trie();

  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_set_value(context, trie,
                                             999, NULL, GRN_OBJ_SET));

  put_sample_entry();
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_pat_set_value(context, trie,
                                             sample_id, NULL, GRN_OBJ_SET));
}

static grn_trie_test_data *
increment_test_data_new(const gchar *key, increment_key_func increment,
                        grn_test_set_parameters_func set_parameters)
{
  return trie_test_data_newv(key, NULL, NULL, GRN_SUCCESS, NULL, increment,
                             set_parameters, NULL);
}

static void
increment_test_data_free(grn_trie_test_data *data)
{
  trie_test_data_free(data);
}

static void
string_increment(grn_trie_test_data *data)
{
  gchar *original_string = data->key;
  gchar *string;
  gint last;

  last = strlen(original_string);
  if (original_string[last - 1] < 'X') {
    original_string[last - 1]++;
  } else {
    string = g_strconcat(data->key, "A", NULL);
    g_free(data->key);
    data->key = string;
  }
}

static void
utf8_string_increment(grn_trie_test_data *data)
{
  gchar *original_string = data->key;
  gchar *character;

  for (character = original_string;
       *character;
       character = g_utf8_next_char(character)) {
    gunichar unicode;

    if (g_random_int_range(0, 100) < 99)
      continue;

    unicode = g_utf8_get_char(character);
    if (unicode < g_utf8_get_char("ン")) {
      gchar buffer[6];
      gint i, length;

      length = g_unichar_to_utf8(unicode + 1, buffer);
      for (i = 0; i < length; i++) {
        character[i] = buffer[i];
      }
      return;
    }
  }

  {
    gchar *string;
    gunichar new_unicode;
    gchar new_character[6];
    gint length;

    new_unicode = g_random_int_range(g_utf8_get_char("ア"),
                                     g_utf8_get_char("ン"));
    length = g_unichar_to_utf8(new_unicode, new_character);
    new_character[length] = '\0';
    string = g_strconcat(data->key, new_character, NULL);
    g_free(data->key);
    data->key = string;
  }
}

static void
utf8_string_same_prefix_increment(grn_trie_test_data *data)
{
  gchar *original_string = data->key;
  gchar *character;

  for (character = original_string;
       *character;
       character = g_utf8_next_char(character)) {
    gunichar unicode;

    unicode = g_utf8_get_char(character);
    if (unicode < g_utf8_get_char("ン")) {
      gchar buffer[6];
      gint i, length;

      length = g_unichar_to_utf8(unicode + 1, buffer);
      for (i = 0; i < length; i++) {
        character[i] = buffer[i];
      }
      return;
    }
  }

  {
    gchar *string;
    string = g_strconcat(data->key, "ア", NULL);
    g_free(data->key);
    data->key = string;
  }
}

void
data_add_and_delete(void)
{
  cut_add_data("default",
               increment_test_data_new("Cutter", string_increment, NULL),
               increment_test_data_free,
               "sis",
               increment_test_data_new("Groonga", string_increment, set_sis),
               increment_test_data_free,
               "sis - multi byte key (katakana)",
               increment_test_data_new("セナ", utf8_string_increment,
                                       set_sis_and_utf8_encoding),
               increment_test_data_free,
               "sis - multi byte key (katakana) - many same prefix",
               increment_test_data_new("セナ", utf8_string_same_prefix_increment,
                                       set_sis_and_utf8_encoding),
               increment_test_data_free);
}

void
test_add_and_delete(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  guint i;
  const guint n_operations = 7500;
  gboolean sis_utf8_data = FALSE;

  if (test_data->increment == utf8_string_increment ||
      test_data->increment == utf8_string_same_prefix_increment)
    sis_utf8_data = TRUE;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  ids = g_array_new(TRUE, TRUE, sizeof(grn_id));
  for (i = 0; i < n_operations; i++) {
    cut_assert_lookup_add(test_data->key);
    test_data->increment((grn_trie_test_data *)test_data);
    g_array_append_val(ids, id);
  }

  if (sis_utf8_data)
    cut_assert_operator_int(n_operations, <, grn_pat_size(context, trie));
  else
    cut_assert_equal_int(n_operations, grn_pat_size(context, trie));

  for (i = 0; i < ids->len; i++) {
    grn_id delete_id;

    delete_id = g_array_index(ids, grn_id, i);
    if (sis_utf8_data) {
      grn_pat_delete_by_id(context, trie, delete_id, NULL);
    } else {
      cut_set_message("i = %d; id = %d", i, delete_id);
      grn_test_assert(grn_pat_delete_by_id(context, trie, delete_id, NULL));
    }
  }

  cut_assert_equal_int(0, grn_pat_size(context, trie));
}
