/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <pat.h>
#include <gcutter.h>
#include "../lib/sen-assertions.h"

void data_read_write(void);
void test_read_write(gconstpointer *data);

#define N_THREADS 10

static sen_ctx *contexts[N_THREADS];
static sen_pat *tries[N_THREADS];

void
startup(void)
{
  int i;

  for (i = 0; i < N_THREADS; i++) {
    contexts[i] = NULL;
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
      sen_pat *trie;

      trie = tries[i];
      if (trie)
        sen_pat_close(context, trie);
      sen_ctx_close(context);
    }
  }
}


void
data_read_write(void)
{
  gint i;
  const gchar *n_processes, *process_number, *thread_type;

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
  sen_search_flags flags;
  sen_ctx *context;
  sen_pat *trie;
  const gchar *path;
  const gchar *value_string;
  gint process_number = 0;
  const gchar *process_number_string;
  void *value;
  sen_id id = SEN_ID_NIL;
  sen_rc rc;

  i = GPOINTER_TO_INT(data);
  process_number_string = g_getenv(SEN_TEST_ENV_PROCESS_NUMBER);
  if (process_number_string)
    process_number = atoi(process_number_string);

  key = cut_take_printf("key: %d (%d:%d)", i, process_number, N_THREADS);

  contexts[i] = sen_ctx_open(NULL, SEN_CTX_USE_QL);
  cut_assert_not_null(contexts[i], "context: %d (%d)", i, process_number);
  context = contexts[i];

  path = g_getenv(SEN_TEST_ENV_PATRICIA_TRIE_PATH);
  cut_assert_not_null(path);
  tries[i] = sen_pat_open(context, path);
  cut_assert_not_null(tries[i], "patricia trie: %d (%d)", i, process_number);
  trie = tries[i];

  flags = 0;
  sen_test_assert_nil(sen_pat_lookup(context, trie, key, strlen(key),
                                     &value, &flags),
                      "lookup - fail: %d (%d:%d)", key, i, process_number);

  value_string = cut_take_printf("value: [%s] (%d:%d)", key, i, process_number);
  flags = SEN_TABLE_ADD;
  rc = sen_io_lock(context, trie->io, -1);
  if (rc != sen_success)
    sen_test_assert(rc);
  id = sen_pat_lookup(context, trie, key, strlen(key), &value, &flags);
  sen_io_unlock(trie->io);
  sen_test_assert_not_nil(id);
  cut_assert_equal_uint(SEN_TABLE_ADDED, flags & SEN_TABLE_ADDED);
  strcpy(value, value_string);

  flags = 0;
  value = NULL;
  id = sen_pat_lookup(context, trie, key, strlen(key), &value, &flags);
  sen_test_assert_not_nil(id,
                          "lookup - success: %s (%d:%d)",
                          key, i, process_number);
  cut_assert_equal_string(value_string, value);

  tries[i] = NULL;
  sen_test_assert(sen_pat_close(context, trie));

  contexts[i] = NULL;
  sen_test_assert(sen_ctx_close(context));
}
