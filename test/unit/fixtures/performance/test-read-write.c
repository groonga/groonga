/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <groonga.h>
#include <gcutter.h>
#include "../lib/sen-assertions.h"

void data_read_write(void);
void test_read_write(gconstpointer *data);

#define VALUE_SIZE 1024
#define N_THREADS 100

static sen_ctx *contexts[N_THREADS];
static sen_obj *spaces[N_THREADS];
static sen_obj *tables[N_THREADS];

void
startup(void)
{
  int i;

  for (i = 0; i < N_THREADS; i++) {
    contexts[i] = NULL;
    spaces[i] = NULL;
    tables[i] = NULL;
  }
}

void
shutdown(void)
{
  int i;

  for (i = 0; i < N_THREADS; i++) {
    sen_ctx *context;

    context = contexts[i];
    if (context) {
      sen_obj *space;
      sen_obj *table;

      space = spaces[i];
      table = tables[i];
#if 0
      if (table)
        sen_table_close(context, table);
      if (space)
        sen_space_close(context, space);
#endif
      sen_ctx_close(context);
    }
  }
}


void
data_read_write(void)
{
  gint i;
  const gchar *table_type, *n_processes, *process_number, *thread_type;

  table_type = g_getenv(SEN_TEST_ENV_TABLE_TYPE);
  if (!table_type)
    table_type = "?";

  n_processes = g_getenv(SEN_TEST_ENV_N_PROCESSES);
  if (!n_processes)
    n_processes = "?";

  process_number = g_getenv(SEN_TEST_ENV_PROCESS_NUMBER);
  if (!process_number)
    process_number = "?";

  if (g_getenv(SEN_TEST_ENV_MULTI_THREAD))
    thread_type = "multi thread";
  else
    thread_type = "single thread";

  for (i = 0; i < N_THREADS; i++) {
    cut_add_data(cut_take_printf("%s - %s process(es)(%s) - %s - %d",
                                 table_type,
                                 n_processes, process_number, thread_type, i),
                 GINT_TO_POINTER(i), NULL);
  }
}

void
test_read_write(gconstpointer *data)
{
  gint i;
  sen_search_flags flags;
  sen_ctx *context;
  sen_obj *table;
  sen_obj *space;
  const gchar *path;
  const gchar *value_string;
  gint process_number = 0;
  const gchar *process_number_string;
  const gchar table_name[] = "performance-read-write";
  gchar value[VALUE_SIZE];
  gint size;
  sen_id id;

  i = GPOINTER_TO_INT(data);
  process_number_string = g_getenv(SEN_TEST_ENV_PROCESS_NUMBER);
  if (process_number_string)
    process_number = atoi(process_number_string);

  contexts[i] = sen_ctx_open(NULL, SEN_CTX_USE_QL);
  cut_assert_not_null(contexts[i], "context: %d (%d)", i, process_number);
  context = contexts[i];

  path = g_getenv(SEN_TEST_ENV_SPACE_PATH);
  cut_assert_not_null(path);
  spaces[i] = sen_space_open(context, path);
  space = spaces[i];

  path = g_getenv(SEN_TEST_ENV_TABLE_PATH);
  cut_assert_not_null(path);
  tables[i] = sen_table_open(context, table_name, strlen(table_name),
                             path);
  cut_assert_not_null(tables[i], "table: %d (%d)", i, process_number);
  table = tables[i];

  flags = 0;
  sen_test_assert_nil(sen_table_lookup(context, table,
                                       &i, sizeof(sen_id), &flags),
                      "lookup - fail: (%d:%d)", i, process_number);

  value_string = cut_take_printf("value: (%d:%d)", i, process_number);
  flags = SEN_TABLE_ADD;
  id = sen_table_lookup(context, table, &i, sizeof(sen_id), &flags);
  sen_test_assert_not_nil(id);
  cut_assert_equal_uint(SEN_TABLE_ADDED, flags & SEN_TABLE_ADDED);

  sen_table_set_value(context, table, id, (gchar *)value_string, SEN_OBJ_SET);

  size = sen_table_get_value(context, table, id, value);
  value[size] = '\0';
  sen_test_assert_not_nil(id,
                          "lookup - success: (%d:%d)",
                          i, process_number);
  cut_assert_equal_string(value_string, value);

#if 0
  tables[i] = NULL;
  sen_test_assert(sen_table_close(context, table));

  spaces[i] = NULL;
  sen_test_assert(sen_space_close(context, space));
#endif

  contexts[i] = NULL;
  sen_test_assert(sen_ctx_close(context));
}
