/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>

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
#include <gcutter.h>
#include "../lib/grn-assertions.h"
#include <db.h>

void data_read_write(void);
void test_read_write(gconstpointer *data);

#define VALUE_SIZE 1024
#define N_THREADS 100

static grn_ctx contexts[N_THREADS];
static grn_obj *tables[N_THREADS];

void
cut_startup(void)
{
  int i;
  grn_init();
  for (i = 0; i < N_THREADS; i++) {
    grn_ctx *context;

    context = &contexts[i];
    grn_ctx_init(context, 0);
    // contexts[i] = NULL;
    // grn_ctx_fin(context);
    tables[i] = NULL;
  }
}

void
cut_shutdown(void)
{
  int i;

  for (i = 0; i < N_THREADS; i++) {
    grn_ctx *context;

    context = &contexts[i];
    if (context) {
      grn_obj *table;

      table = tables[i];
#if 0
      if (table)
        grn_table_close(context, table);
#endif
      grn_ctx_fin(context);
    }
  }
  grn_fin();
}


void
data_read_write(void)
{
  gint i;
  const gchar *table_type, *n_processes, *process_number, *thread_type;

  table_type = g_getenv(GRN_TEST_ENV_TABLE_TYPE);
  if (!table_type)
    table_type = "?";

  n_processes = g_getenv(GRN_TEST_ENV_N_PROCESSES);
  if (!n_processes)
    n_processes = "?";

  process_number = g_getenv(GRN_TEST_ENV_PROCESS_NUMBER);
  if (!process_number)
    process_number = "?";

  if (g_getenv(GRN_TEST_ENV_MULTI_THREAD))
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
  int added;
  grn_ctx *context;
  grn_obj *table;
  const gchar *path;
  const gchar *value_string;
  gint process_number = 0;
  const gchar *process_number_string;
  const gchar table_name[] = "performance-read-write";
  grn_obj value;
  grn_obj *retrieved_value;
  grn_id id;
  grn_rc rc;

  i = GPOINTER_TO_INT(data);
  process_number_string = g_getenv(GRN_TEST_ENV_PROCESS_NUMBER);
  if (process_number_string)
    process_number = atoi(process_number_string);

  rc = grn_ctx_init(&contexts[i], 0);
  grn_test_assert(rc, cut_set_message("context: %d (%d)", i, process_number));
  context = &contexts[i];

  path = g_getenv(GRN_TEST_ENV_TABLE_PATH);
  cut_assert_not_null(path);
  tables[i] = grn_table_open(context, table_name, strlen(table_name),
                             path);
  cut_assert_not_null(tables[i],
                      cut_message("table: %d (%d)", i, process_number));
  table = tables[i];

  grn_test_assert_nil(grn_table_get(context, table, &i, sizeof(grn_id)),
                      cut_message("lookup - fail: (%d:%d)", i, process_number));

  value_string = cut_take_printf("value: (%d:%d)", i, process_number);
  id = grn_table_add(context, table, &i, sizeof(grn_id), &added);
  grn_test_assert_not_nil(id);
  cut_assert_equal_int(1, added);

  GRN_TEXT_INIT(&value, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET_REF(&value, value_string, strlen(value_string));
  grn_obj_set_value(context, table, id, &value, GRN_OBJ_SET);

  retrieved_value = grn_obj_get_value(context, table, id, NULL);
  grn_test_assert_not_nil(
    id,
    cut_message("lookup - success: (%d:%d)", i, process_number));
  GRN_TEXT_PUTC(context, retrieved_value, '\0');
  cut_assert_equal_string(value_string, GRN_BULK_HEAD(retrieved_value));

  tables[i] = NULL;
  grn_test_assert(grn_obj_close(context, table));

  //  contexts[i] = NULL;
  grn_test_assert(grn_ctx_fin(context));
}
