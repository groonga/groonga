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

#include <grn_hash.h>
#include <grn_pat.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void data_hash(void);
void test_hash(gconstpointer test_data);
void data_patricia_trie(void);
void test_patricia_trie(gconstpointer test_data);

#define VALUE_SIZE 1024

static grn_ctx *context;
static grn_hash *hash;
static grn_pat *trie;
static gchar *base_dir;
static gchar *env_hash_path;
static gchar *env_patricia_trie_path;
static gchar *env_multi_thread;
static gchar *env_n_processes;
static gchar *env_process_number;

void
cut_setup(void)
{
  const gchar *tmp_dir;

  context = g_new0(grn_ctx, 1);
  hash = NULL;
  trie = NULL;

#define SAVE_ENV_VALUE(var_name, macro_name)                            \
  env_ ## var_name = g_strdup(g_getenv(GRN_TEST_ENV_ ## macro_name))

  SAVE_ENV_VALUE(hash_path, HASH_PATH);
  SAVE_ENV_VALUE(patricia_trie_path, PATRICIA_TRIE_PATH);
  SAVE_ENV_VALUE(multi_thread, MULTI_THREAD);
  SAVE_ENV_VALUE(n_processes, N_PROCESSES);
  SAVE_ENV_VALUE(process_number, PROCESS_NUMBER);

#undef SAVE_ENV_VALUE

  tmp_dir = grn_test_get_tmp_dir();
  cut_remove_path(tmp_dir, NULL);

  base_dir = g_build_filename(tmp_dir, "stress", NULL);

  g_mkdir_with_parents(base_dir, 0755);
  cut_assert_path_exist(base_dir);
}

void
cut_teardown(void)
{
  if (context) {
    if (hash)
      grn_hash_close(context, hash);
    if (trie)
      grn_pat_close(context, trie);
    grn_ctx_fin(context);
    g_free(context);
  }

#define RESTORE_ENV_VALUE(var_name, macro_name) do                      \
  {                                                                     \
    if (env_ ## var_name) {                                             \
      g_setenv(GRN_TEST_ENV_ ## macro_name, env_ ## var_name, TRUE);    \
      g_free(env_ ## var_name);                                         \
    } else {                                                            \
      g_unsetenv(GRN_TEST_ENV_ ## macro_name);                          \
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

typedef struct _grn_test_data
{
  gint n_processes;
  gboolean multi_thread;
} grn_test_data;

static grn_test_data *
test_data_new(gint n_processes, gboolean multi_thread)
{
  grn_test_data *data;

  data = g_new0(grn_test_data, 1);
  data->n_processes = n_processes;
  data->multi_thread = multi_thread;

  return data;
}

static void
test_data_free(grn_test_data *data)
{
  g_free(data);
}

static gboolean
run(const gchar **test_case_names, const grn_test_data *data)
{
  gint i;
  const gchar *test_dir;
  CutSubProcessGroup *group;

  test_dir = cut_take_string(g_build_filename(grn_test_get_base_dir(),
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
      g_setenv(GRN_TEST_ENV_MULTI_THREAD, "TRUE", TRUE);
    else
      g_unsetenv(GRN_TEST_ENV_MULTI_THREAD);
    g_setenv(GRN_TEST_ENV_N_PROCESSES,
             cut_take_printf("%d", data->n_processes), TRUE);
    g_setenv(GRN_TEST_ENV_PROCESS_NUMBER, cut_take_printf("%d", i), TRUE);
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
  const grn_test_data *data = test_data;
  gchar *path;
  const gchar *test_case_names[] = {"test_stress_hash", NULL};

  grn_test_assert(grn_ctx_init(context, 0));

  path = g_build_filename(base_dir, "hash", NULL);
  g_setenv(GRN_TEST_ENV_HASH_PATH, path, TRUE);
  GRN_CTX_SET_ENCODING(context, GRN_ENC_UTF8);
  hash = grn_hash_create(context, path, sizeof(gint), VALUE_SIZE, 0);
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
  const grn_test_data *data = test_data;
  gchar *path;
  const gchar *test_case_names[] = {"test_stress_patricia_trie", NULL};

  grn_test_assert(grn_ctx_init(context, 0));

  path = g_build_filename(base_dir, "patricia-trie", NULL);
  g_setenv(GRN_TEST_ENV_PATRICIA_TRIE_PATH, path, TRUE);
  GRN_CTX_SET_ENCODING(context, GRN_ENC_UTF8);
  trie = grn_pat_create(context, path, GRN_PAT_MAX_KEY_SIZE / 2, VALUE_SIZE, 0);
  g_free(path);
  cut_assert_not_null(trie);

  cut_assert_true(run(test_case_names, data));
}
