/*
  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>

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

#include "test-patricia-trie.h"

void data_next_with_no_entry(void);
void test_next_with_no_entry(gconstpointer data);
void data_next_with_one_entry(void);
void test_next_with_one_entry(gconstpointer data);
void data_next_with_multi_entries(void);
void test_next_with_multi_entries(gconstpointer data);
void test_by_key_descending_max(void);
void test_by_id_over_offset(void);
void data_value(void);
void test_value(gconstpointer data);
void data_delete(void);
void test_delete(gconstpointer data);

static GList *keys;
static GList *keys_and_values;

void
cut_setup(void)
{
  setup_trie_common("patricia-trie-cursor");

  keys = NULL;
  keys_and_values = NULL;

  default_encoding = GRN_ENC_UTF8;

  sample_value = NULL;
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
keys_and_values_free(void)
{
  if (keys_and_values) {
    gcut_list_string_free(keys_and_values);
    keys_and_values = NULL;
  }
}

void
cut_teardown(void)
{
  keys_free();
  keys_and_values_free();
  teardown_trie_common();
}

static GList *
retrieve_all_keys(void)
{
  keys_free();
  keys = grn_test_pat_cursor_get_keys(context, (grn_table_cursor *)cursor);
  return keys;
}

static GList *
retrieve_all_keys_and_values(void)
{
  keys_and_values_free();
  keys_and_values = grn_test_pat_cursor_get_pairs(context,
                                                  (grn_table_cursor *)cursor);
  return keys_and_values;
}

static grn_trie_test_data *test_data_new(GList *expected_strings,
                                         grn_test_set_parameters_func set_parameters,
                                         ...) G_GNUC_NULL_TERMINATED;
static grn_trie_test_data *
test_data_new(GList *expected_strings,
              grn_test_set_parameters_func set_parameters,
              ...)
{
  grn_trie_test_data *test_data;
  va_list args;

  va_start(args, set_parameters);
  test_data = trie_test_data_newv(NULL, NULL, NULL, GRN_SUCCESS,
                                  expected_strings, NULL,
                                  set_parameters, &args);
  va_end(args);

  return test_data;
}

static void
test_data_free(grn_trie_test_data *test_data)
{
  trie_test_data_free(test_data);
}

static void
set_ascending(void)
{
  default_cursor_flags |= GRN_CURSOR_ASCENDING;
}

static void
set_descending(void)
{
  default_cursor_flags |= GRN_CURSOR_DESCENDING;
}

void
data_next_with_no_entry(void)
{
  cut_add_data("ascending",
               test_data_new(NULL, set_ascending, NULL),
               test_data_free,
               "ascending - sis",
               test_data_new(NULL, set_ascending, set_sis, NULL),
               test_data_free,
               "descending",
               test_data_new(NULL, set_descending, NULL),
               test_data_free,
               "descending - sis",
               test_data_new(NULL, set_descending, set_sis, NULL),
               test_data_free);
}

void
test_next_with_no_entry(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();
  cut_assert_open_cursor();
  gcut_assert_equal_list_string(NULL, retrieve_all_keys());
}

void
data_next_with_one_entry(void)
{
  cut_add_data("ascending",
               test_data_new(gcut_list_string_new("セナ", NULL),
                             set_ascending, NULL),
               test_data_free,
               "ascending - sis",
               test_data_new(gcut_list_string_new("セナ", "ナ", NULL),
                             set_ascending, set_sis, NULL),
               test_data_free,
               "descending",
               test_data_new(gcut_list_string_new("セナ", NULL),
                             set_descending, NULL),
               test_data_free,
               "descending - sis",
               test_data_new(gcut_list_string_new("ナ", "セナ", NULL),
                             set_descending, set_sis, NULL),
               test_data_free);
}

void
test_next_with_one_entry(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  const gchar key[] = "セナ";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key);

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

static void
set_max(void)
{
  default_cursor_max = g_strdup("セナセナ");
  default_cursor_max_size = strlen(default_cursor_max);
}

static void
set_max_low(void)
{
  default_cursor_max = g_strdup("セナ");
  default_cursor_max_size = strlen(default_cursor_max);
}

static void
set_min(void)
{
  default_cursor_min = g_strdup("セナ");
  default_cursor_min_size = strlen(default_cursor_min);
}

static void
set_min_high(void)
{
  default_cursor_min = g_strdup("セナセナ");
  default_cursor_min_size = strlen(default_cursor_min);
}

static void
set_gt(void)
{
  default_cursor_flags |= GRN_CURSOR_GT;
}

static void
set_lt(void)
{
  default_cursor_flags |= GRN_CURSOR_LT;
}

static void
add_data_ascending(void)
{
  cut_add_data("ascending",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, NULL),
               test_data_free,
               "ascending - max",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, NULL),
               test_data_free,
               "ascending - max - gt",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_gt, NULL),
               test_data_free,
               "ascending - max - lt",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_lt, NULL),
               test_data_free,
               "ascending - max - gt - lt",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_gt, set_lt, NULL),
               test_data_free,
               "ascending - min",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, NULL),
               test_data_free,
               "ascending - min - gt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_gt, NULL),
               test_data_free,
               "ascending - min - lt",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_lt, NULL),
               test_data_free,
               "ascending - min - gt - lt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_gt, set_lt, NULL),
               test_data_free,
               "ascending - max - min",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_min, NULL),
               test_data_free,
               "ascending - max - min - gt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_min, set_gt,
                             NULL),
               test_data_free,
               "ascending - max - min - lt",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_min, set_lt,
                             NULL),
               test_data_free,
               "ascending - max - min - gt - lt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_min, set_gt,
                             set_lt, NULL),
               test_data_free,
               "ascending - high-min",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min_high, NULL),
               test_data_free,
               "ascending - low-max",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ",
                                                  NULL),
                             set_ascending, set_max_low, NULL),
               test_data_free,
               "ascending - high-min - low-max",
               test_data_new(NULL,
                             set_ascending, set_min_high, set_max_low, NULL),
               test_data_free);
}

static void
add_data_ascending_sis(void)
{
  cut_add_data("ascending - sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  "ナ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_sis, NULL),
               test_data_free,
               "ascending - max - sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_sis, NULL),
               test_data_free,
               "ascending - max - gt - sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_gt, set_sis, NULL),
               test_data_free,
               "ascending - max - lt - sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  NULL),
                             set_ascending, set_max, set_lt, set_sis, NULL),
               test_data_free,
               "ascending - max - gt - lt - sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  NULL),
                             set_ascending, set_max, set_gt, set_lt, set_sis,
                             NULL),
               test_data_free,
               "ascending - min - sis",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  "ナ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_sis, NULL),
               test_data_free,
               "ascending - min - gt - sis",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  "ナ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_gt, set_sis, NULL),
               test_data_free,
               "ascending - min - lt - sis",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  "ナ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_lt, set_sis, NULL),
               test_data_free,
               "ascending - min - gt - lt - sis",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  "ナ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min, set_gt, set_lt,
                             set_sis, NULL),
               test_data_free,
               "ascending - max - min - sis",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_min, set_sis,
                             NULL),
               test_data_free,
               "ascending - max - min - gt - sis",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセ",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_max, set_min, set_gt,
                             set_sis, NULL),
               test_data_free,
               "ascending - max - min - lt - sis",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  NULL),
                             set_ascending, set_max, set_min, set_lt,
                             set_sis, NULL),
               test_data_free,
               "ascending - max - min - gt - lt - sis",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセ",
                                                  NULL),
                             set_ascending, set_max, set_min, set_gt,
                             set_lt, set_sis, NULL),
               test_data_free,
               "ascending - high-min - sis",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "ナ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_min_high, set_sis, NULL),
               test_data_free,
               "ascending - low-max - sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ",
                                                  NULL),
                             set_ascending, set_max_low, set_sis, NULL),
               test_data_free,
               "ascending - high-min - low-max - sis",
               test_data_new(NULL,
                             set_ascending, set_min_high, set_max_low, set_sis,
                             NULL),
               test_data_free);
}

static void
add_data_descending(void)
{
  cut_add_data("descending",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナセナ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, NULL),
               test_data_free,
               "descending - max",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, NULL),
               test_data_free,
               "descending - max - gt",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_gt, NULL),
               test_data_free,
               "descending - max - lt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_lt, NULL),
               test_data_free,
               "descending - max - gt - lt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_gt, set_lt, NULL),
               test_data_free,
               "descending - min",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナセナ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_min, NULL),
               test_data_free,
               "descending - min - gt",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナセナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_min, set_gt, NULL),
               test_data_free,
               "descending - min - lt",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナセナ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_min, set_lt, NULL),
               test_data_free,
               "descending - min - gt - lt",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナセナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_min, set_gt, set_lt, NULL),
               test_data_free,
               "descending - max - min",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_min, NULL),
               test_data_free,
               "descending - max - min - gt",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_max, set_min, set_gt,
                             NULL),
               test_data_free,
               "descending - max - min - lt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_min, set_lt,
                             NULL),
               test_data_free,
               "descending - max - min - gt - lt",
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  NULL),
                             set_descending, set_max, set_min, set_gt,
                             set_lt, NULL),
               test_data_free,
               "descending - high-min",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナセナ",
                                                  NULL),
                             set_descending, set_min_high, NULL),
               test_data_free,
               "descending - low-max",
               test_data_new(gcut_list_string_new("セナ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max_low, NULL),
               test_data_free,
               "descending - high-min - low-max",
               test_data_new(NULL,
                             set_descending, set_min_high, set_max_low, NULL),
               test_data_free);
}

static void
add_data_descending_sis(void)
{
  cut_add_data("descending - sis",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "ナセナ",
                                                  "ナセ",
                                                  "ナ + Ruby",
                                                  "ナ",
                                                  "セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "セ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_sis, NULL),
               test_data_free,
               "descending - max - sis",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "セ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_sis, NULL),
               test_data_free,
               "descending - max - gt - sis",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "セ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_gt, set_sis,
                             NULL),
               test_data_free,
               "descending - max - lt - sis",
               test_data_new(gcut_list_string_new("セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "セ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_lt, set_sis,
                             NULL),
               test_data_free,
               "descending - max - gt - lt - sis",
               test_data_new(gcut_list_string_new("セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  "セ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max, set_gt, set_lt,
                             set_sis, NULL),
               test_data_free,
               "descending - min - sis",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "ナセナ",
                                                  "ナセ",
                                                  "ナ + Ruby",
                                                  "ナ",
                                                  "セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_min, set_sis, NULL),
               test_data_free,
               "descending - min - gt - sis",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "ナセナ",
                                                  "ナセ",
                                                  "ナ + Ruby",
                                                  "ナ",
                                                  "セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_min, set_gt, set_sis,
                             NULL),
               test_data_free,
               "descending - min - lt - sis",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "ナセナ",
                                                  "ナセ",
                                                  "ナ + Ruby",
                                                  "ナ",
                                                  "セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_min, set_lt, set_sis,
                             NULL),
               test_data_free,
               "descending - min - gt - lt - sis",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "ナセナ",
                                                  "ナセ",
                                                  "ナ + Ruby",
                                                  "ナ",
                                                  "セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_min, set_gt, set_lt,
                             set_sis, NULL),
               test_data_free,
               "descending - max - min - sis",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_min,
                             set_sis, NULL),
               test_data_free,
               "descending - max - min - gt - sis",
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナセ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_max, set_min, set_gt,
                             set_sis, NULL),
               test_data_free,
               "descending - max - min - lt - sis",
               test_data_new(gcut_list_string_new("セナセ",
                                                  "セナ + Ruby",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_min, set_lt,
                             set_sis, NULL),
               test_data_free,
               "descending - max - min - gt - lt - sis",
               test_data_new(gcut_list_string_new("セナセ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_max, set_min, set_gt,
                             set_lt, set_sis, NULL),
               test_data_free,
               "descending - high-min - sis",
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "ナセナ",
                                                  "ナセ",
                                                  "ナ + Ruby",
                                                  "ナ",
                                                  "セナセナ",
                                                  NULL),
                             set_descending, set_min_high, set_sis, NULL),
               test_data_free,
               "descending - low-max - sis",
               test_data_new(gcut_list_string_new("セナ",
                                                  "セ",
                                                  "Groonga",
                                                  NULL),
                             set_descending, set_max_low, set_sis, NULL),
               test_data_free,
               "descending - high-min - low-max - sis",
               test_data_new(NULL,
                             set_descending, set_min_high, set_max_low, set_sis,
                             NULL),
               test_data_free);
}

void
data_next_with_multi_entries(void)
{
  add_data_ascending();
  add_data_ascending_sis();
  add_data_descending();
  add_data_descending_sis();
}

void
test_next_with_multi_entries(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Groonga";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

void
test_by_key_descending_max(void)
{
  default_cursor_min = g_strdup("0");
  default_cursor_min_size = strlen(default_cursor_min);
  default_cursor_max = g_strdup("9989");
  default_cursor_max_size = strlen(default_cursor_max);
  default_cursor_flags |= GRN_CURSOR_DESCENDING;

  cut_assert_create_trie();

  cut_assert_lookup_add("997");
  cut_assert_lookup_add("999");
  cut_assert_lookup_add("9998");

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(gcut_take_new_list_string("997", NULL),
                                retrieve_all_keys());
}

void
test_by_id_over_offset(void)
{
  default_cursor_flags |= GRN_CURSOR_BY_ID;
  default_cursor_offset = 1;

  cut_assert_create_trie();

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(NULL,
                                retrieve_all_keys());
}

static void
set_value_size(void)
{
  default_value_size = strlen("上書きされた値 -");
}

void
data_value(void)
{
  cut_add_data("default",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "",
                                                  "セナ",
                                                  "",
                                                  "セナ + Ruby",
                                                  "",
                                                  /* should be set two values */
                                                  "セナセナ",
                                                  "上書きされた値 -",
                                                  "ナセナセ",
                                                  "VALUE2",
                                                  NULL),
                             set_ascending, set_value_size, NULL),
               test_data_free,

               "sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "",
                                                  "セ",
                                                  "",
                                                  "セナ",
                                                  "",
                                                  "セナ + Ruby",
                                                  "",
                                                  /* should be set two values */
                                                  "セナセ",
                                                  "",
                                                  "セナセナ",
                                                  "上書きされた値 -",
                                                  "ナ",
                                                  "",
                                                  "ナ + Ruby",
                                                  "",
                                                  "ナセ",
                                                  "",
                                                  "ナセナ",
                                                  "",
                                                  "ナセナセ",
                                                  "VALUE2",
                                                  NULL),
                             set_ascending, set_value_size, set_sis, NULL),
               test_data_free);
}

void
test_value(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Groonga";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";
  gchar value2[] = "VALUE2";
  gchar value4_1[] = "Groonga";
  gchar value4_2[] = "るびい";
  gchar value5_1[] = "上書きされる値 - overridden value";
  gchar value5_2[] = "上書きされた値 - override value";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_open_cursor();
  while (grn_pat_cursor_next(context, cursor) != GRN_ID_NIL) {
    void *key;
    gchar *null_terminated_key;
    int size;

    size = grn_pat_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_free(g_string_new_len(key, size), FALSE);
    if (g_str_equal(null_terminated_key, key2)) {
      grn_pat_cursor_set_value(context, cursor, value2, GRN_OBJ_SET);
    } else if (g_str_equal(null_terminated_key, key4)) {
      grn_pat_cursor_set_value(context, cursor, value4_1, GRN_OBJ_INCR);
      grn_pat_cursor_set_value(context, cursor, value4_2, GRN_OBJ_INCR);
    } else if (g_str_equal(null_terminated_key, key5)) {
      grn_pat_cursor_set_value(context, cursor, value5_1, GRN_OBJ_SET);
      grn_pat_cursor_set_value(context, cursor, value5_2, GRN_OBJ_SET);
    }
  }
  cursor_free();

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys_and_values());
}

void
data_delete(void)
{
  cut_add_data("default",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セナ + Ruby",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, NULL),
               test_data_free,
               "sis",
               test_data_new(gcut_list_string_new("Groonga",
                                                  "セ",
                                                  "セナ + Ruby",
                                                  "セナセ",
                                                  "ナ + Ruby",
                                                  "ナセ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_sis, NULL),
               test_data_free);
}

void
test_delete(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Groonga";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_open_cursor();
  while (grn_pat_cursor_next(context, cursor) != GRN_ID_NIL) {
    void *key;
    gchar *null_terminated_key;
    int size;

    size = grn_pat_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_free(g_string_new_len(key, size), FALSE);
    if (g_str_equal(null_terminated_key, key1) ||
        g_str_equal(null_terminated_key, key5)) {
      grn_pat_cursor_delete(context, cursor, NULL);
    }
  }
  cursor_free();

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}
