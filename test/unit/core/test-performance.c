/* -*- c-basic-offset: 2; coding: utf-8 -*- */
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

#include <groonga.h>

#include <cutter.h>

#include "../lib/grn-assertions.h"

void data_read_write(void);
void test_read_write(gconstpointer test_data);

typedef struct _grn_test_data
{
  gchar *type_name;
  grn_obj_flags flags;
  gint n_processes;
  gboolean multi_thread;
} grn_test_data;

static GList *sub_processes;
static grn_test_data *base_data;

static grn_ctx *context;
static grn_obj *space;
static grn_obj *type;
static grn_obj *table;
static gchar *base_dir;
static gchar *env_table_path;
static gchar *env_table_type;
static gchar *env_multi_thread;
static gchar *env_n_processes;
static gchar *env_process_number;

static grn_test_data *
test_data_new(const gchar *type_name, grn_obj_flags flags,
              gint n_processes, gboolean multi_thread)
{
  grn_test_data *data;

  data = g_new0(grn_test_data, 1);
  data->type_name = g_strdup(type_name);
  data->flags = flags;
  data->n_processes = n_processes;
  data->multi_thread = multi_thread;

  return data;
}

static void
test_data_free(grn_test_data *data)
{
  g_free(data->type_name);
  g_free(data);
}

void
cut_setup(void)
{
  const gchar *tmp_dir;

  sub_processes = NULL;
  base_data = NULL;

  context = g_new0(grn_ctx, 1);
  space = NULL;
  table = NULL;

#define SAVE_ENV_VALUE(var_name, macro_name)                            \
  env_ ## var_name = g_strdup(g_getenv(GRN_TEST_ENV_ ## macro_name))

  SAVE_ENV_VALUE(table_path, TABLE_PATH);
  SAVE_ENV_VALUE(table_type, TABLE_TYPE);
  SAVE_ENV_VALUE(multi_thread, MULTI_THREAD);
  SAVE_ENV_VALUE(n_processes, N_PROCESSES);
  SAVE_ENV_VALUE(process_number, PROCESS_NUMBER);

#undef SAVE_ENV_VALUE

  tmp_dir = grn_test_get_tmp_dir();
  cut_remove_path(tmp_dir, NULL);

  base_dir = g_build_filename(tmp_dir, "performance", NULL);

  g_mkdir_with_parents(base_dir, 0755);
  cut_assert_path_exist(base_dir);
}

void
cut_teardown(void)
{
  if (sub_processes)
    g_list_free(sub_processes);

  if (base_data)
    test_data_free(base_data);

  if (context) {
    if (table)
      grn_obj_close(context, table);
    if (space)
      grn_obj_close(context, space);
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

  RESTORE_ENV_VALUE(table_path, TABLE_PATH);
  RESTORE_ENV_VALUE(table_type, TABLE_TYPE);
  RESTORE_ENV_VALUE(multi_thread, MULTI_THREAD);
  RESTORE_ENV_VALUE(n_processes, N_PROCESSES);
  RESTORE_ENV_VALUE(process_number, PROCESS_NUMBER);

#undef RESTORE_ENV_VALUE

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
    g_free(base_dir);
  }
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

    sub_processes = g_list_append(sub_processes, sub_process);
  }
  return cut_sub_process_group_wait(group);
}

static gboolean
run_test(const gchar **test_case_names, const grn_test_data *data)
{
  const gchar *type_name, *table_name;
  gchar *path;

  grn_test_assert(grn_ctx_init(context, 0));

  GRN_CTX_SET_ENCODING(context, GRN_ENC_UTF8);

  type_name = "name";
  type = grn_type_create(context, type_name, strlen(type_name),
                         GRN_OBJ_KEY_UINT, sizeof(grn_id));

  path = g_build_filename(base_dir, "table", NULL);
  g_setenv(GRN_TEST_ENV_TABLE_PATH, path, TRUE);

  table_name = cut_take_printf("%s: performance-read-write", data->type_name);
  g_setenv(GRN_TEST_ENV_TABLE_TYPE, data->type_name, TRUE);
  table = grn_table_create(context,
                           table_name, strlen(table_name),
                           path, GRN_OBJ_PERSISTENT | data->flags,
                           type, NULL);
  g_free(path);
  cut_assert_not_null(table);

  return run(test_case_names, data);
}


static void
add_read_write_data(const gchar *type_name, grn_obj_flags flags)
{
  cut_add_data(cut_take_printf("%s - single process - single thread", type_name),
               test_data_new(type_name, flags, 1, FALSE), test_data_free,

               cut_take_printf("%s - single process - multi thread", type_name),
               test_data_new(type_name, flags, 1, TRUE), test_data_free,

               cut_take_printf("%s - multi process - single thread", type_name),
               test_data_new(type_name, flags, 10, FALSE), test_data_free,

               cut_take_printf("%s - multi process - multi thread", type_name),
               test_data_new(type_name, flags, 10, TRUE), test_data_free);
}

void
data_read_write(void)
{
  add_read_write_data("hash", GRN_OBJ_TABLE_HASH_KEY);
  add_read_write_data("patricia tree", GRN_OBJ_TABLE_PAT_KEY);
}

void
test_read_write(gconstpointer test_data)
{
  const grn_test_data *data = test_data;
  const gchar *test_case_names[] = {"test_read_write", NULL};
  CutSubProcess *target_sub_process;
  CutSubProcess *base_sub_process;
  gdouble target_elapsed, base_elapsed;

  cut_assert_true(run_test(test_case_names, data));
  target_sub_process = g_list_last(sub_processes)->data;

  base_data = test_data_new(cut_take_printf("%s - base", data->type_name),
                            data->flags, 1, FALSE);
  cut_assert_true(run_test(test_case_names, base_data));
  base_sub_process = g_list_last(sub_processes)->data;

  target_elapsed = cut_sub_process_get_total_elapsed(target_sub_process);
  base_elapsed = cut_sub_process_get_total_elapsed(base_sub_process);

  /* TODO: should use cut_assert_operator_double() in Cutter 1.0.5 */
  cut_assert_operator(target_elapsed / (data->multi_thread ? 100 : 1),
                      <,
                      base_elapsed);
}
