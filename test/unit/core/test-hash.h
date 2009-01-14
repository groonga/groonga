/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <hash.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/sen-assertions.h"
#include "../lib/sen-test-hash-factory.h"
#include "../lib/sen-test-hash-assertions.h"

static GList *expected_messages;
static SenTestHashFactory *factory;

static sen_logger_info *logger;

static sen_hash *hash;
static sen_hash_cursor *cursor;
static sen_id id;
static void *value;

static uint32_t sample_key;
static const gchar *sample_value;
static sen_id sample_id;

static gchar *base_dir;

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
                                 "to write meaningless sentences that is "
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

  logger = setup_sen_logger();

  factory = sen_test_hash_factory_new();
  sen_test_hash_factory_set_logger(factory, logger);

  expected_messages = NULL;
  hash = NULL;
  cursor = NULL;
  id = SEN_ID_NIL;

  sample_key = 2929;
  sample_value = cut_take_string(g_strdup("hash test"));
  sample_id = SEN_ID_NIL;

  base_dir = g_build_filename(sen_test_get_base_dir(), "tmp", NULL);
  default_path = g_build_filename(base_dir, "hash", NULL);
  sen_test_hash_factory_set_path(factory, default_path);
  g_free(default_path);

  key_size = sen_test_hash_factory_get_key_size(factory);

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
    g_free(base_dir);
  }

  teardown_sen_logger(logger);
}

#define context (sen_test_hash_factory_get_context(factory))

#define clear_messages()                        \
  sen_collect_logger_clear_messages(logger)

#define messages()                              \
  sen_collect_logger_get_messages(logger)

#define cut_assert_create_hash()                \
  sen_test_assert_create_hash(&hash, factory)

#define cut_assert_open_hash()                  \
  sen_test_assert_open_hash(&hash, factory)

#define cut_assert_fail_open_hash()                     \
  sen_test_assert_fail_open_hash(&hash, factory)

#define lookup(key, flags)                                      \
  sen_hash_lookup(context, hash, key, key_size, &value, flags)

#define cut_assert_lookup(key, flags)                   \
  sen_test_assert_not_nil((id = lookup(key, (flags))),  \
                          "flags: <%d>", *(flags))

#define cut_assert_lookup_failed(key, flags)                            \
  sen_test_assert_nil(lookup(key, (flags)),  "flags: <%d>", *(flags))

#define cut_assert_lookup_add(key) do                                   \
{                                                                       \
  const void *_key;                                                     \
  sen_search_flags flags;                                               \
  sen_id found_id;                                                      \
                                                                        \
  _key = (key);                                                         \
  if (sen_test_hash_factory_get_flags(factory) & SEN_OBJ_KEY_VAR_SIZE)  \
    key_size = strlen(_key);                                            \
                                                                        \
  flags = 0;                                                            \
  cut_assert_lookup_failed(_key, &flags);                               \
                                                                        \
  flags = SEN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, &flags);                                      \
  cut_assert_equal_uint(SEN_TABLE_ADDED, flags & SEN_TABLE_ADDED);      \
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
  flags = SEN_TABLE_ADD;                                                \
  cut_assert_lookup(_key, &flags);                                      \
  cut_assert_equal_uint(0, flags & SEN_TABLE_ADDED);                    \
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
  cursor = sen_test_hash_factory_open_cursor(factory, &error);          \
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
  sen_test_hash_factory_set_path(factory, NULL);
  sen_test_hash_factory_add_flags(factory, SEN_HASH_TINY);
}
