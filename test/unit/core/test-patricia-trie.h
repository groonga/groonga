/*
  Copyright (C) 2008-2014  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_pat.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define DEFAULT_VALUE_SIZE 64

static grn_logger_info *logger;

static GList *expected_messages;

static grn_ctx *context;
static grn_obj *database;
static grn_pat *trie;
static grn_pat_cursor *cursor;
static grn_id id;
static void *value;

static gchar *sample_key;
static const gchar *sample_value;
static grn_id sample_id;

static const gchar *base_dir;

static gchar *default_path;
static uint32_t default_key_size;
static uint32_t default_value_size;
static uint32_t default_flags;
static grn_encoding default_encoding;

static gchar *default_cursor_min;
static uint32_t default_cursor_min_size;
static gchar *default_cursor_max;
static uint32_t default_cursor_max_size;
static int default_cursor_offset;
static int default_cursor_limit;
static int default_cursor_flags;

static uint32_t default_context_flags;

static void
setup_trie_common(const gchar *default_path_component)
{
  logger = setup_grn_logger();

  expected_messages = NULL;

  context = NULL;
  database = NULL;
  trie = NULL;
  cursor = NULL;
  id = GRN_ID_NIL;

  sample_key = g_strdup("sample-key");
  sample_value = cut_take_string(g_strdup("patricia trie test"));
  sample_id = GRN_ID_NIL;

  base_dir = grn_test_get_tmp_dir();
  default_path = g_build_filename(base_dir, default_path_component, NULL);
  default_key_size = GRN_PAT_MAX_KEY_SIZE / 2;
  default_value_size = DEFAULT_VALUE_SIZE;
  default_flags = GRN_OBJ_KEY_VAR_SIZE;
  default_encoding = GRN_ENC_DEFAULT;

  default_cursor_min = NULL;
  default_cursor_min_size = 0;
  default_cursor_max = NULL;
  default_cursor_max_size = 0;
  default_cursor_offset = 0;
  default_cursor_limit = -1;
  default_cursor_flags = 0;

  default_context_flags = 0;

  cut_remove_path(base_dir, NULL);
  g_mkdir_with_parents(base_dir, 0755);
}

static void
expected_messages_free(void)
{
  if (expected_messages) {
    gcut_list_string_free(expected_messages);
    expected_messages = NULL;
  }
}

static void
cursor_free(void)
{
  if (context && cursor) {
    grn_pat_cursor_close(context, cursor);
    cursor = NULL;
  }
}

static void
trie_free(void)
{
  if (context && trie) {
    grn_pat_close(context, trie);
    trie = NULL;
  }
}

static void
database_free(void)
{
  if (context && database) {
    grn_obj_close(context, database);
    database = NULL;
  }
}

static void
context_free(void)
{
  if (context) {
    cursor_free();
    trie_free();
    database_free();
    grn_ctx_fin(context);
    context = NULL;
  }
}

static void
teardown_trie_common(void)
{
  expected_messages_free();
  context_free();

  if (sample_key) {
    g_free(sample_key);
  }

  if (default_path) {
    g_free(default_path);
  }

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
  }

  teardown_grn_logger(logger);
}

#define clear_messages()                        \
  grn_collect_logger_clear_messages(logger)

#define messages()                              \
  grn_collect_logger_get_messages(logger)

#define open_context() do                               \
{                                                       \
  context = g_new0(grn_ctx, 1);                         \
  grn_test_assert(grn_ctx_init(context,                 \
                               default_context_flags)); \
  GRN_CTX_SET_ENCODING(context, default_encoding);      \
} while (0)

#define cut_assert_open_context() do                    \
{                                                       \
  context_free();                                       \
  open_context();                                       \
  cut_assert(context);                                  \
  database = grn_db_create(context, NULL, NULL);        \
  cut_assert(database);                                 \
} while (0)

#define create_trie() do                                                \
{                                                                       \
  GRN_CTX_SET_ENCODING(context, default_encoding);                      \
  trie = grn_pat_create(context, default_path, default_key_size,        \
                        default_value_size, default_flags);             \
} while (0)

#define cut_assert_create_trie() do             \
{                                               \
  cut_assert_open_context();                    \
  trie_free();                                  \
  create_trie();                                \
  grn_test_assert_context(context);             \
  cut_assert(trie);                             \
} while (0)

#define open_trie()                             \
  trie = grn_pat_open(context, default_path)

#define cut_assert_open_trie() do                                       \
{                                                                       \
  clear_messages();                                                     \
  cut_assert_open_context();                                            \
  trie_free();                                                          \
  open_trie();                                                          \
  gcut_assert_equal_list_string(NULL, messages());                      \
  cut_assert(trie);                                                     \
} while (0)

#define cut_assert_fail_open_trie() do                  \
{                                                       \
  cut_assert_open_context();                            \
  trie_free();                                          \
  open_trie();                                          \
  cut_assert_null(trie);                                \
} while (0)

typedef int grn_search_flags;

#define GRN_TABLE_ADD                  (0x01<<6)

#define lookup(key, key_size, flags)                            \
  (((*(flags) & GRN_TABLE_ADD))                                 \
   ? grn_pat_add(context, trie, key, key_size, &value, flags)   \
   : grn_pat_get(context, trie, key, key_size, &value))

#define cut_assert_lookup(key, key_size, flags) do                      \
{                                                                       \
  grn_test_assert_not_nil((id = lookup(key, key_size, (flags))),        \
                          cut_message("flags: <%d>", *(flags)));        \
} while (0)

#define cut_assert_lookup_failed(key, key_size, flags) do       \
{                                                               \
  grn_test_assert_nil(lookup(key, key_size, (flags)),           \
                      cut_message("flags: <%d>", *(flags)));    \
} while (0)

#define cut_assert_lookup_add(key) do                                   \
{                                                                       \
  const gchar *_key;                                                    \
  uint32_t key_size;                                                    \
  grn_search_flags flags;                                               \
  grn_id found_id;                                                      \
                                                                        \
  _key = (key);                                                         \
  if (_key) {                                                           \
    key_size = strlen(_key);                                            \
  } else {                                                              \
    key_size = 0;                                                       \
  }                                                                     \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup_failed(_key, key_size, &flags);                     \
                                                                        \
  flags = GRN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, key_size, &flags);                            \
  cut_assert_equal_int(1, flags & 1);                                   \
  found_id = id;                                                        \
  if (sample_value) {                                                   \
    strcpy(value, sample_value);                                        \
    value = NULL;                                                       \
  }                                                                     \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup(_key, key_size, &flags);                            \
  cut_assert_equal_int(found_id, id);                                   \
  if (sample_value) {                                                   \
    cut_assert_equal_string(sample_value, value);                       \
    value = NULL;                                                       \
  }                                                                     \
                                                                        \
  flags = GRN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, key_size, &flags);                            \
  cut_assert_equal_uint(0, flags & 1);                                  \
  cut_assert_equal_uint(found_id, id);                                  \
  if (sample_value) {                                                   \
    cut_assert_equal_string(sample_value, value);                       \
  }                                                                     \
} while (0)

#define put_sample_entry() do                   \
{                                               \
  cut_assert_lookup_add(sample_key);            \
  sample_id = id;                               \
} while (0)

#define open_cursor()                                   \
  cursor = grn_pat_cursor_open(context, trie,           \
                               default_cursor_min,      \
                               default_cursor_min_size, \
                               default_cursor_max,      \
                               default_cursor_max_size, \
                               default_cursor_offset,   \
                               default_cursor_limit,    \
                               default_cursor_flags)

#define cut_assert_open_cursor() do                     \
{                                                       \
  clear_messages();                                     \
  cursor_free();                                        \
  open_cursor();                                        \
  cut_assert_equal_g_list_string(NULL, messages());     \
  cut_assert(cursor);                                   \
} while (0)

typedef struct _grn_trie_test_data grn_trie_test_data;
typedef void (*increment_key_func) (grn_trie_test_data *test_data);

struct _grn_trie_test_data {
  gchar *key;
  gchar *search_key;
  grn_rc expected_rc;
  gchar *expected_key;
  GList *expected_strings;
  increment_key_func increment;
  GList *set_parameters_funcs;
};

static grn_trie_test_data *
trie_test_data_newv(const gchar *key,
                    const gchar *search_key,
                    const gchar *expected_key,
                    grn_rc expected_rc,
                    GList *expected_strings,
                    increment_key_func increment,
                    grn_test_set_parameters_func set_parameters,
                    va_list *args)
{
  grn_trie_test_data *test_data;

  test_data = g_new0(grn_trie_test_data, 1);
  test_data->key = g_strdup(key);
  test_data->search_key = g_strdup(search_key);
  test_data->expected_key = g_strdup(expected_key);
  test_data->expected_rc = expected_rc;
  test_data->expected_strings = expected_strings;
  test_data->increment = increment;
  test_data->set_parameters_funcs = NULL;
  while (set_parameters) {
    test_data->set_parameters_funcs =
      g_list_append(test_data->set_parameters_funcs, set_parameters);
    if (args && *args)
      set_parameters = va_arg(*args, grn_test_set_parameters_func);
    else
      set_parameters = NULL;
  }

  return test_data;
}

static void
trie_test_data_free(grn_trie_test_data *test_data)
{
  if (test_data->key)
    g_free(test_data->key);
  if (test_data->search_key)
    g_free(test_data->search_key);
  if (test_data->expected_key)
    g_free(test_data->expected_key);
  if (test_data->expected_strings)
    gcut_list_string_free(test_data->expected_strings);
  g_list_free(test_data->set_parameters_funcs);
  g_free(test_data);
}

static void
trie_test_data_set_parameters(const grn_trie_test_data *data)
{
  GList *node;

  for (node = data->set_parameters_funcs; node; node = g_list_next(node)) {
    grn_test_set_parameters_func set_parameters = node->data;
    set_parameters();
  }
}

static void
set_sis(void)
{
  default_flags |= GRN_OBJ_KEY_WITH_SIS;
}
