/*
  Copyright (C) 2008-2012  Kouhei Sutou <kou@clear-code.com>

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

void data_lcp_search(void);
void test_lcp_search(gconstpointer data);
void data_prefix_search(void);
void test_prefix_search(gconstpointer data);
void data_suffix_search(void);
void test_suffix_search(gconstpointer data);
void data_fuzzy_search(void);
void test_fuzzy_search(gconstpointer data);

static GList *keys;

static grn_hash *hash;

void
cut_setup(void)
{
  setup_trie_common("patricia-trie-search");

  hash = NULL;

  keys = NULL;

  default_encoding = GRN_ENC_UTF8;
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
hash_free(void)
{
  if (context && hash) {
    grn_hash_close(context, hash);
    hash = NULL;
  }
}

void
cut_teardown(void)
{
  keys_free();
  hash_free();
  teardown_trie_common();
}

#define create_hash()                                   \
  hash = grn_hash_create(context, NULL, sizeof(grn_id), \
                         0, GRN_HASH_TINY)

#define cut_assert_create_hash() do                     \
{                                                       \
  clear_messages();                                     \
  hash_free();                                          \
  create_hash();                                        \
  cut_assert_equal_g_list_string(NULL, messages());     \
  cut_assert(hash);                                     \
} while (0)


static GList *
retrieve_all_keys(void)
{
  grn_id hash_id;
  grn_hash_cursor *cursor;

  keys_free();

  cursor = grn_hash_cursor_open(context, hash,
                                NULL, 0, NULL, 0, 0, -1,
                                GRN_CURSOR_DESCENDING);
  hash_id = grn_hash_cursor_next(context, cursor);
  while (hash_id != GRN_ID_NIL) {
    grn_id *trie_id;
    void *hash_key;
    GString *null_terminated_key;
    gchar key[GRN_PAT_MAX_KEY_SIZE];
    int size;

    grn_hash_cursor_get_key(context, cursor, &hash_key);
    trie_id = hash_key;
    size = grn_pat_get_key(context, trie, *trie_id, key, sizeof(key));
    null_terminated_key = g_string_new_len(key, size);
    keys = g_list_append(keys, g_string_free(null_terminated_key, FALSE));
    hash_id = grn_hash_cursor_next(context, cursor);
  }
  grn_hash_cursor_close(context, cursor);

  return keys;
}

static grn_trie_test_data *lcp_test_data_new(const gchar *expected_key,
                                             const gchar *search_key,
                                             grn_test_set_parameters_func set_parameters,
                                             ...) G_GNUC_NULL_TERMINATED;
static grn_trie_test_data *
lcp_test_data_new(const gchar *expected_key, const gchar *search_key,
                  grn_test_set_parameters_func set_parameters, ...)
{
  grn_trie_test_data *test_data;
  va_list args;

  va_start(args, set_parameters);
  test_data = trie_test_data_newv(NULL, search_key, expected_key,
                                  GRN_SUCCESS, NULL, NULL,
                                  set_parameters, &args);
  va_end(args);

  return test_data;
}

static void
lcp_test_data_free(grn_trie_test_data *test_data)
{
  trie_test_data_free(test_data);
}

void
data_lcp_search(void)
{
  cut_add_data("default - nonexistence",
               lcp_test_data_new(NULL, "カッター", NULL, NULL),
               lcp_test_data_free,
               "default - short",
               lcp_test_data_new(NULL, "セ", NULL, NULL),
               lcp_test_data_free,
               "default - exact",
               lcp_test_data_new("セナ", "セナ", NULL, NULL),
               lcp_test_data_free,
               "default - long",
               lcp_test_data_new("セナセナ", "セナセナセナ", NULL, NULL),
               lcp_test_data_free,
               "sis - nonexistence",
               lcp_test_data_new(NULL, "カッター", set_sis, NULL),
               lcp_test_data_free,
               "sis - short",
               lcp_test_data_new("セ", "セ", set_sis, NULL),
               lcp_test_data_free,
               "sis - exact",
               lcp_test_data_new("セナ", "セナ", set_sis, NULL),
               lcp_test_data_free,
               "sis - long",
               lcp_test_data_new("セナセナ", "セナセナセナ",
                                 set_sis, NULL),
               lcp_test_data_free);
}

void
test_lcp_search(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  gchar key[GRN_PAT_MAX_KEY_SIZE];
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

  id = grn_pat_lcp_search(context, trie,
                          test_data->search_key,
                          strlen(test_data->search_key));
  if (test_data->expected_key) {
    int size;
    const gchar *null_terminated_key;

    grn_test_assert_not_nil(id);
    size = grn_pat_get_key(context, trie, id, key, sizeof(key));
    null_terminated_key = cut_take_strndup(key, size);
    cut_assert_equal_string(test_data->expected_key, null_terminated_key);
  } else {
    grn_test_assert_nil(id);
  }
}

static grn_trie_test_data *xfix_test_data_new(grn_rc expected_rc,
                                              GList *expected_keys,
                                              const gchar *search_key,
                                              grn_test_set_parameters_func set_parameters,
                                              ...) G_GNUC_NULL_TERMINATED;
static grn_trie_test_data *
xfix_test_data_new(grn_rc expected_rc, GList *expected_keys,
                   const gchar *search_key,
                   grn_test_set_parameters_func set_parameters, ...)
{
  grn_trie_test_data *test_data;
  va_list args;

  va_start(args, set_parameters);
  test_data = trie_test_data_newv(NULL, search_key, NULL, expected_rc,
                                  expected_keys, NULL,
                                  set_parameters, &args);
  va_end(args);

  return test_data;
}

static void
xfix_test_data_free(grn_trie_test_data *test_data)
{
  trie_test_data_free(test_data);
}

void
data_prefix_search(void)
{
  cut_add_data("default - nonexistence",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "カッター", NULL, NULL),
               xfix_test_data_free,
               "default - short",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セナ", "セナ + Ruby",
                                                       "セナセナ", NULL),
                                  "セ", NULL, NULL),
               xfix_test_data_free,
               "default - exact",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セナ", "セナ + Ruby",
                                                       "セナセナ", NULL),
                                  "セナ", NULL, NULL),
               xfix_test_data_free,
               "default - long",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "セナセナセナ",
                                  NULL, NULL),
               xfix_test_data_free,
               "sis - nonexistence",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "カッター",
                                  set_sis, NULL),
               xfix_test_data_free,
               "sis - short",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セ", "セナ",
                                                       "セナ + Ruby",
                                                       "セナセ", "セナセナ",
                                                       NULL),
                                  "セ", set_sis, NULL),
               xfix_test_data_free,
               "sis - exact",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セナ",
                                                       "セナ + Ruby",
                                                       "セナセ", "セナセナ",
                                                       NULL),
                                  "セナ", set_sis, NULL),
               xfix_test_data_free,
               "sis - long",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "セナセナセナ",
                                  set_sis, NULL),
               xfix_test_data_free);
}

void
test_prefix_search(gconstpointer data)
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

  cut_assert_create_hash();
  grn_test_assert_equal_rc(test_data->expected_rc,
                           grn_pat_prefix_search(context, trie,
                                                 test_data->search_key,
                                                 strlen(test_data->search_key),
                                                 hash));
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

void
data_suffix_search(void)
{
  cut_add_data("default - nonexistence",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "カッター", NULL, NULL),
               xfix_test_data_free,
               "default - short",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "ナ", NULL, NULL),
               xfix_test_data_free,
               "default - exact",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セナ", NULL),
                                  "セナ", NULL, NULL),
               xfix_test_data_free,
               "default - long",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "セナセナセナ",
                                  NULL, NULL),
               xfix_test_data_free,
               "sis - nonexistence",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "カッター",
                                  set_sis, NULL),
               xfix_test_data_free,
               "sis - short",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セナセナ",
                                                       "ナセナ",
                                                       "セナ",
                                                       "ナ",
                                                       NULL),
                                  "ナ", set_sis, NULL),
               xfix_test_data_free,
               "sis - exact",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("セナセナ",
                                                       "ナセナ",
                                                       "セナ",
                                                       NULL),
                                  "セナ", set_sis, NULL),
               xfix_test_data_free,
               "sis - long",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "セナセナセナ",
                                  set_sis, NULL),
               xfix_test_data_free);
}

void
test_suffix_search(gconstpointer data)
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

  cut_assert_create_hash();
  grn_test_assert_equal_rc(test_data->expected_rc,
                           grn_pat_suffix_search(context, trie,
                                                 test_data->search_key,
                                                 strlen(test_data->search_key),
                                                 hash));
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

void
data_fuzzy_search(void)
{
  cut_add_data("nonexistence",
               xfix_test_data_new(GRN_END_OF_DATA, NULL, "cccc", NULL, NULL),
               xfix_test_data_free,
               "insertion",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("ああ", "あ", NULL),
               "あ", NULL, NULL),
               xfix_test_data_free,
               "deletion",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("bbbbb", NULL),
               "bbbbbb", NULL, NULL),
               xfix_test_data_free,
               "substitution",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("cdefg", NULL),
               "cdefh", NULL, NULL),
               xfix_test_data_free,
               "transposition",
               xfix_test_data_new(GRN_SUCCESS,
                                  gcut_list_string_new("cdefg", NULL),
               "cdegf", NULL, NULL),
               xfix_test_data_free);
}

void
test_fuzzy_search(gconstpointer data)
{
  const grn_trie_test_data *test_data = data;
  const gchar key1[]  = "あ";
  const gchar key2[]  = "ああ";
  const gchar key3[]  = "あああ";
  const gchar key4[]  = "bbbb";
  const gchar key5[]  = "bbbbb";
  const gchar key6[]  = "cdefg";
  grn_fuzzy_search_optarg args;
  args.prefix_match_size = 0;
  args.max_distance = 1;
  args.max_expansion = 0;
  args.flags = GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION;

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);
  cut_assert_lookup_add(key6);

  cut_assert_create_hash();
  grn_test_assert_equal_rc(test_data->expected_rc,
                           grn_pat_fuzzy_search(context, trie,
                                                test_data->search_key,
                                                strlen(test_data->search_key),
                                                &args, hash));
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}
