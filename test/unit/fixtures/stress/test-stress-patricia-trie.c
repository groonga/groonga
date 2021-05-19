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

#include <grn_pat.h>
#include <gcutter.h>
#include "../lib/grn-assertions.h"

void data_read_write(void);
void test_read_write(gconstpointer *data);

#define N_THREADS 10

static grn_ctx *contexts[N_THREADS];
static grn_pat *tries[N_THREADS];

void
cut_startup(void)
{
  int i;

  for (i = 0; i < N_THREADS; i++) {
    contexts[i] = NULL;
  }
}

void
cut_shutdown(void)
{
  int i;

  for (i = 0; i < N_THREADS; i++) {
    grn_ctx *context;

    context = contexts[i];
    if (context) {
      grn_pat *trie;

      trie = tries[i];
      if (trie)
        grn_pat_close(context, trie);
      grn_ctx_fin(context);
    }
  }
}


void
data_read_write(void)
{
  gint i;
  const gchar *n_processes, *process_number, *thread_type;

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
    cut_add_data(cut_take_printf("%s process(es)[%s] - %s[%d]",
                                 n_processes, process_number, thread_type, i),
                 GINT_TO_POINTER(i), NULL);
  }
}

void
test_read_write(gconstpointer *data)
{
  gint i;
  const gchar *key;
  grn_ctx *context;
  grn_pat *trie;
  const gchar *path;
  const gchar *value_string;
  gint process_number = 0;
  const gchar *process_number_string;
  void *value;
  int added;
  grn_id id = GRN_ID_NIL;
  grn_rc rc;

  i = GPOINTER_TO_INT(data);
  process_number_string = g_getenv(GRN_TEST_ENV_PROCESS_NUMBER);
  if (process_number_string)
    process_number = atoi(process_number_string);

  key = cut_take_printf("key: %d (%d:%d)", i, process_number, N_THREADS);

  rc = grn_ctx_init(contexts[i], GRN_CTX_USE_QL);
  grn_test_assert(rc, cut_message("context: %d (%d)", i, process_number));
  context = contexts[i];

  path = g_getenv(GRN_TEST_ENV_PATRICIA_TRIE_PATH);
  cut_assert_not_null(path);
  tries[i] = grn_pat_open(context, path);
  cut_assert_not_null(tries[i],
                      cut_message("patricia trie: %d (%d)", i, process_number));
  trie = tries[i];

  grn_test_assert_nil(
    grn_pat_get(context, trie, key, strlen(key), &value),
    cut_message("lookup - fail: %s (%d:%d)", key, i, process_number));

  value_string = cut_take_printf("value: [%s] (%d:%d)", key, i, process_number);
  rc = grn_io_lock(context, trie->io, -1);
  if (rc != GRN_SUCCESS)
    grn_test_assert(rc);
  added = 0;
  id = grn_pat_add(context, trie, key, strlen(key), &value, &added);
  grn_io_unlock(trie->io);
  grn_test_assert_not_nil(id);
  cut_assert_equal_uint(1, added);
  strcpy(value, value_string);

  value = NULL;
  id = grn_pat_get(context, trie, key, strlen(key), &value);
  grn_test_assert_not_nil(
    id,
    cut_message("lookup - success: %s (%d:%d)", key, i, process_number));
  cut_assert_equal_string(value_string, value);

  tries[i] = NULL;
  grn_test_assert(grn_pat_close(context, trie));

  contexts[i] = NULL;
  grn_test_assert(grn_ctx_fin(context));
}
