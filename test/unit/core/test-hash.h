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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <grn_hash.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"
#include "../lib/grn-test-hash-factory.h"
#include "../lib/grn-test-hash-assertions.h"

static GList *expected_messages;
static GrnTestHashFactory *factory;

static grn_logger_info *logger;

static grn_hash *hash;
static grn_hash_cursor *cursor;
static grn_id id;
static void *value;

static uint32_t sample_key;
static const gchar *sample_value;
static grn_id sample_id;

static const gchar *base_dir;

static uint32_t key_size;
static gchar *not_uint32_size_key;
static uint32_t not_uint32_key_size;

static void
startup_hash_common(void)
{
  not_uint32_size_key = g_strdup("uint32_t size must be 4. "
                                 "So this key should have more 4 bytes length. "
                                 "And this key should have enough length to "
                                 "increment 10000 times. "
                                 "But this is too difficult because we need "
                                 "to write meaningless grntences that is "
                                 "what you are reading now!");
  not_uint32_key_size = strlen(not_uint32_size_key);
}

static void
shutdown_hash_common(void)
{
  if (not_uint32_size_key) {
    g_free(not_uint32_size_key);
  }
}

static void
setup_hash_common(const gchar *default_path_component)
{
  gchar *default_path;

  logger = setup_grn_logger();

  factory = grn_test_hash_factory_new();
  grn_test_hash_factory_set_logger(factory, logger);

  expected_messages = NULL;
  hash = NULL;
  cursor = NULL;
  id = GRN_ID_NIL;

  sample_key = 2929;
  sample_value = cut_take_string(g_strdup("hash test"));
  sample_id = GRN_ID_NIL;

  base_dir = grn_test_get_tmp_dir();
  default_path = g_build_filename(base_dir, default_path_component, NULL);
  grn_test_hash_factory_set_path(factory, default_path);
  g_free(default_path);

  key_size = grn_test_hash_factory_get_key_size(factory);

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
teardown_hash_common(void)
{
  expected_messages_free();

  if (factory)
    g_object_unref(factory);

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
  }

  teardown_grn_logger(logger);
}

#define context (grn_test_hash_factory_get_context(factory))

#define clear_messages()                        \
  grn_collect_logger_clear_messages(logger)

#define messages()                              \
  grn_collect_logger_get_messages(logger)

#define cut_assert_create_hash()                \
  grn_test_assert_create_hash(&hash, factory)

#define cut_assert_open_hash()                  \
  grn_test_assert_open_hash(&hash, factory)

#define cut_assert_fail_open_hash()                     \
  grn_test_assert_fail_open_hash(&hash, factory)

typedef int grn_search_flags;

#define GRN_TABLE_ADD                  (0x01<<6)

#define lookup(key, flags)                                      \
  (((*(flags) & GRN_TABLE_ADD))                                 \
   ? grn_hash_add(context, hash, key, key_size, &value, flags)  \
   : grn_hash_get(context, hash, key, key_size, &value))

#define cut_assert_lookup(key, flags) do                                \
{                                                       		\
  grn_test_assert_not_nil((id = lookup(key, (flags))),                  \
                          cut_message("flags: <%d>", *(flags)));        \
} while (0)

#define cut_assert_lookup_failed(key, flags) do                 \
{                                               		\
  grn_test_assert_nil(lookup(key, (flags)),                     \
                      cut_message("flags: <%d>", *(flags)));    \
} while (0)

#define cut_assert_lookup_add(key) do                                   \
{                                                                       \
  const void *_key;                                                     \
  grn_search_flags flags;                                               \
  grn_id found_id;                                                      \
                                                                        \
  _key = (key);                                                         \
  if (grn_test_hash_factory_get_flags(factory) & GRN_OBJ_KEY_VAR_SIZE)  \
    key_size = strlen(_key);                                            \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup_failed(_key, &flags);                               \
                                                                        \
  flags = GRN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, &flags);                                      \
  cut_assert_equal_uint(1, flags & 1);                                  \
  found_id = id;                                                        \
  if (sample_value) {                                                   \
    strcpy(value, sample_value);                                        \
    value = NULL;                                                       \
  }                                                                     \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup(_key, &flags);                                      \
  cut_assert_equal_uint(found_id, id);                                  \
  if (sample_value) {                                                   \
    cut_assert_equal_string(sample_value, value);                       \
    value = NULL;                                                       \
  }                                                                     \
                                                                        \
  flags = GRN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, &flags);                                      \
  cut_assert_equal_uint(0, flags & 1);                                  \
  cut_assert_equal_uint(found_id, id);                                  \
  if (sample_value) {                                                   \
    cut_assert_equal_string(sample_value, value);                       \
  }                                                                     \
} while (0)

#define cut_assert_lookup_add_with_value(key, value) do                 \
{                                                                       \
  const gchar *_sample_value = sample_value;                            \
  sample_value = cut_take_string(g_strdup(value));                      \
  cut_assert_lookup_add(key);                                           \
  sample_value = _sample_value;                                         \
} while (0)

#define open_cursor() do                                                \
{                                                                       \
  GError *error = NULL;                                                 \
                                                                        \
  cursor = grn_test_hash_factory_open_cursor(factory, &error);          \
  gcut_assert_error(error);                                             \
} while (0)

#define cut_assert_open_cursor() do                     \
{                                                       \
  clear_messages();                                     \
  open_cursor();                                        \
  cut_assert_equal_g_list_string(NULL, messages());     \
  cut_assert(cursor);                                   \
} while (0)


static void
set_tiny_flags(void)
{
  grn_test_hash_factory_set_path(factory, NULL);
  grn_test_hash_factory_add_flags(factory, GRN_HASH_TINY);
}
