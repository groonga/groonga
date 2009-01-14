/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <groonga.h>

#include <cutter.h>

#include "../lib/sen-test-utils.h"

void data_read_write(void);
void test_read_write(gconstpointer test_data);

#define VALUE_SIZE 1024

typedef struct _sen_test_data
{
  gchar *type_name;
  sen_obj_flags flags;
  gint n_processes;
  gboolean multi_thread;
} sen_test_data;

static GList *sub_processes;
static sen_test_data *base_data;

static sen_ctx *context;
static sen_obj *space;
static sen_obj *type;
static sen_obj *table;
static gchar *base_dir;
static gchar *env_space_path;
static gchar *env_table_path;
static gchar *env_table_type;
static gchar *env_multi_thread;
static gchar *env_n_processes;
static gchar *env_process_number;

static sen_test_data *
test_data_new(const gchar *type_name, sen_obj_flags flags,
              gint n_processes, gboolean multi_thread)
{
  sen_test_data *data;

  data = g_new0(sen_test_data, 1);
  data->type_name = g_strdup(type_name);
  data->flags = flags;
  data->n_processes = n_processes;
  data->multi_thread = multi_thread;

  return data;
}

static void
test_data_free(sen_test_data *data)
{
  g_free(data->type_name);
  g_free(data);
}

void
setup(void)
{
  gchar *tmp_dir;

  sub_processes = NULL;
  base_data = NULL;

  context = NULL;
  space = NULL;
  table = NULL;

#define SAVE_ENV_VALUE(var_name, macro_name)                            \
  env_ ## var_name = g_strdup(g_getenv(SEN_TEST_ENV_ ## macro_name))

  SAVE_ENV_VALUE(space_path, SPACE_PATH);
  SAVE_ENV_VALUE(table_path, TABLE_PATH);
  SAVE_ENV_VALUE(table_type, TABLE_TYPE);
  SAVE_ENV_VALUE(multi_thread, MULTI_THREAD);
  SAVE_ENV_VALUE(n_processes, N_PROCESSES);
  SAVE_ENV_VALUE(process_number, PROCESS_NUMBER);

#undef SAVE_ENV_VALUE

  tmp_dir = g_build_filename(sen_test_get_base_dir(), "tmp", NULL);
  cut_remove_path(tmp_dir, NULL);

  base_dir = g_build_filename(tmp_dir, "performance", NULL);
  g_free(tmp_dir);

  g_mkdir_with_parents(base_dir, 0755);
  cut_assert_path_exist(base_dir);
}

void
teardown(void)
{
  if (sub_processes)
    g_list_free(sub_processes);

  if (base_data)
    test_data_free(base_data);

  if (context) {
    if (table)
      sen_obj_close(context, table);
    if (space)
      sen_obj_close(context, space);
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

  RESTORE_ENV_VALUE(space_path, SPACE_PATH);
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

    sub_processes = g_list_append(sub_processes, sub_process);
  }
  return cut_sub_process_group_wait(group);
}

static gboolean
run_test(const gchar **test_case_names, const sen_test_data *data)
{
  const gchar *type_name, *table_name;
  gchar *path;

  context = sen_ctx_open(NULL, SEN_CTX_USE_QL);
  cut_assert_not_null(context);

  path = g_build_filename(base_dir, "space", NULL);
  g_setenv(SEN_TEST_ENV_SPACE_PATH, path, TRUE);
  space = sen_space_create(context, path, sen_enc_utf8);
  g_free(path);

  type_name = "name";
  type = sen_type_create(context, type_name, strlen(type_name),
                         SEN_OBJ_KEY_UINT, sizeof(sen_id));

  path = g_build_filename(base_dir, "table", NULL);
  g_setenv(SEN_TEST_ENV_TABLE_PATH, path, TRUE);

  table_name = cut_take_printf("%s: performance-read-write", data->type_name);
  g_setenv(SEN_TEST_ENV_TABLE_TYPE, data->type_name, TRUE);
  table = sen_table_create(context,
                           table_name, strlen(table_name),
                           path, SEN_OBJ_PERSISTENT | data->flags,
                           type, VALUE_SIZE, sen_enc_utf8);
  g_free(path);
  cut_assert_not_null(table);

  return run(test_case_names, data);
}


static void
add_read_write_data(const gchar *type_name, sen_obj_flags flags)
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
  add_read_write_data("hash", SEN_OBJ_TABLE_HASH_KEY);
  add_read_write_data("patricia tree", SEN_OBJ_TABLE_PAT_KEY);
}

void
test_read_write(gconstpointer test_data)
{
  const sen_test_data *data = test_data;
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
