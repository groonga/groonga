/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <hash.h>
#include <pat.h>

#include <cutter.h>

#include "../lib/sen-test-utils.h"

void data_hash(void);
void test_hash(gconstpointer test_data);
void data_patricia_trie(void);
void test_patricia_trie(gconstpointer test_data);

#define VALUE_SIZE 1024

static sen_ctx *context;
static sen_hash *hash;
static sen_pat *trie;
static gchar *base_dir;
static gchar *env_hash_path;
static gchar *env_patricia_trie_path;
static gchar *env_multi_thread;
static gchar *env_n_processes;
static gchar *env_process_number;

void
setup(void)
{
  gchar *tmp_dir;

  context = NULL;
  hash = NULL;
  trie = NULL;

#define SAVE_ENV_VALUE(var_name, macro_name)                            \
  env_ ## var_name = g_strdup(g_getenv(SEN_TEST_ENV_ ## macro_name))

  SAVE_ENV_VALUE(hash_path, HASH_PATH);
  SAVE_ENV_VALUE(patricia_trie_path, PATRICIA_TRIE_PATH);
  SAVE_ENV_VALUE(multi_thread, MULTI_THREAD);
  SAVE_ENV_VALUE(n_processes, N_PROCESSES);
  SAVE_ENV_VALUE(process_number, PROCESS_NUMBER);

#undef SAVE_ENV_VALUE

  tmp_dir = g_build_filename(sen_test_get_base_dir(), "tmp", NULL);
  cut_remove_path(tmp_dir, NULL);

  base_dir = g_build_filename(tmp_dir, "stress", NULL);
  g_free(tmp_dir);

  g_mkdir_with_parents(base_dir, 0755);
  cut_assert_path_exist(base_dir);
}

void
teardown(void)
{
  if (context) {
    if (hash)
      sen_hash_close(context, hash);
    if (trie)
      sen_pat_close(context, trie);
    sen_ctx_close(context);
  }

#define RESTORE_ENV_VALUE(var_name, macro_name) do                      \
  {                                                                     \
    if (env_ ## var_name) {                                             \
      g_setenv(SEN_TEST_ENV_ ## macro_name, env_ ## var_name, TRUE);    \
      g_free(env_ ## var_name);                                         \
    } else {                                                            \
      g_unsetenv(SEN_TEST_ENV_ ## macro_name);                          \
    }                                                                   \
  } while(0)

  RESTORE_ENV_VALUE(hash_path, HASH_PATH);
  RESTORE_ENV_VALUE(patricia_trie_path, PATRICIA_TRIE_PATH);
  RESTORE_ENV_VALUE(multi_thread, MULTI_THREAD);
  RESTORE_ENV_VALUE(n_processes, N_PROCESSES);
  RESTORE_ENV_VALUE(process_number, PROCESS_NUMBER);

#undef RESTORE_ENV_VALUE

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
    g_free(base_dir);
  }
}

typedef struct _sen_test_data
{
  gint n_processes;
  gboolean multi_thread;
} sen_test_data;

static sen_test_data *
test_data_new(gint n_processes, gboolean multi_thread)
{
  sen_test_data *data;

  data = g_new0(sen_test_data, 1);
  data->n_processes = n_processes;
  data->multi_thread = multi_thread;

  return data;
}

static void
test_data_free(sen_test_data *data)
{
  g_free(data);
}

static gboolean
run(const gchar **test_case_names, const sen_test_data *data)
{
  gint i;
  const gchar *test_dir;
  CutSubProcessGroup *group;

  test_dir = cut_take_string(g_build_filename(sen_test_get_base_dir(),
                                              "fixtures",
                                              NULL));

  group = cut_take_new_sub_process_group();
  for (i = 0; i < data->n_processes; i++) {
    CutSubProcess *sub_process;

    sub_process = cut_take_new_sub_process(test_dir);
    cut_sub_process_set_multi_thread(sub_process, data->multi_thread);
    cut_sub_process_set_fatal_failures(sub_process, TRUE);
    cut_sub_process_set_target_test_case_names(sub_process, test_case_names);

    cut_sub_process_group_add(group, sub_process);
    if (data->multi_thread)
      g_setenv(SEN_TEST_ENV_MULTI_THREAD, "TRUE", TRUE);
    else
      g_unsetenv(SEN_TEST_ENV_MULTI_THREAD);
    g_setenv(SEN_TEST_ENV_N_PROCESSES,
             cut_take_printf("%d", data->n_processes), TRUE);
    g_setenv(SEN_TEST_ENV_PROCESS_NUMBER, cut_take_printf("%d", i), TRUE);
    cut_sub_process_run_async(sub_process);
  }
  return cut_sub_process_group_wait(group);
}

void
data_hash(void)
{
  cut_add_data("single process - single thread",
               test_data_new(1, FALSE), test_data_free,
               "single process - multi thread",
               test_data_new(1, TRUE), test_data_free,
               "multi process - single thread",
               test_data_new(10, FALSE), test_data_free,
               "multi process - multi thread",
               test_data_new(10, TRUE), test_data_free);
}

void
test_hash(gconstpointer test_data)
{
  const sen_test_data *data = test_data;
  gchar *path;
  const gchar *test_case_names[] = {"test_stress_hash", NULL};

  context = sen_ctx_open(NULL, SEN_CTX_USE_QL);
  cut_assert_not_null(context);

  path = g_build_filename(base_dir, "hash", NULL);
  g_setenv(SEN_TEST_ENV_HASH_PATH, path, TRUE);
  hash = sen_hash_create(context, path, sizeof(gint), VALUE_SIZE,
                         0, sen_enc_utf8);
  g_free(path);
  cut_assert_not_null(hash);

  cut_assert_true(run(test_case_names, data));
}

void
data_patricia_trie(void)
{
  cut_add_data("single process - single thread",
               test_data_new(1, FALSE), test_data_free,
               "single process - multi thread",
               test_data_new(1, TRUE), test_data_free,
               "multi process - single thread",
               test_data_new(10, FALSE), test_data_free,
               "multi process - multi thread",
               test_data_new(10, TRUE), test_data_free);
}

void
test_patricia_trie(gconstpointer test_data)
{
  const sen_test_data *data = test_data;
  gchar *path;
  const gchar *test_case_names[] = {"test_stress_patricia_trie", NULL};

  context = sen_ctx_open(NULL, SEN_CTX_USE_QL);
  cut_assert_not_null(context);

  path = g_build_filename(base_dir, "patricia-trie", NULL);
  g_setenv(SEN_TEST_ENV_PATRICIA_TRIE_PATH, path, TRUE);
  trie = sen_pat_create(context, path, SEN_PAT_MAX_KEY_SIZE / 2, VALUE_SIZE,
                        0, sen_enc_utf8);
  g_free(path);
  cut_assert_not_null(trie);

  cut_assert_true(run(test_case_names, data));
}
