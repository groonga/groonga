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

#include <grn_hash.h>
#include <gcutter.h>
#include "../lib/grn-assertions.h"

void data_read_write(void);
void test_read_write(gconstpointer *data);

#define N_THREADS 10

static grn_ctx *contexts[N_THREADS];
static grn_hash *hashes[N_THREADS];

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
      grn_hash *hash;

      hash = hashes[i];
      if (hash)
        grn_hash_close(context, hash);
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
  gint i, key;
  int added;
  grn_ctx *context;
  grn_hash *hash;
  const gchar *path;
  const gchar *value_string;
  gint process_number = 0;
  const gchar *process_number_string;
  void *value;
  grn_id id = GRN_ID_NIL;
  grn_rc rc;

  i = GPOINTER_TO_INT(data);
  process_number_string = g_getenv(GRN_TEST_ENV_PROCESS_NUMBER);
  if (process_number_string)
    process_number = atoi(process_number_string);

  key = i + process_number * N_THREADS;

  rc = grn_ctx_init(contexts[i], GRN_CTX_USE_QL);
  grn_test_assert(rc, cut_message("context: %d (%d)", i, process_number));
  context = contexts[i];

  path = g_getenv(GRN_TEST_ENV_HASH_PATH);
  cut_assert_not_null(path);
  hashes[i] = grn_hash_open(context, path);
  cut_assert_not_null(hashes[i],
                      cut_message("hash: %d (%d)", i, process_number));
  hash = hashes[i];

  grn_test_assert_nil(
    grn_hash_get(context, hash, &key, sizeof(key), &value),
    cut_message("lookup - fail: %d (%d:%d)", key, i, process_number));

  value_string = cut_take_printf("value: %d (%d:%d)", key, i, process_number);
  rc = grn_io_lock(context, hash->io, -1);
  if (rc != GRN_SUCCESS)
    grn_test_assert(rc);
  id = grn_hash_add(context, hash, &key, sizeof(key), &value, &added);
  grn_io_unlock(hash->io);
  grn_test_assert_not_nil(id);
  cut_assert_equal_int(1, added);
  strcpy(value, value_string);

  value = NULL;
  id = grn_hash_get(context, hash, &key, sizeof(key), &value);
  grn_test_assert_not_nil(
    id,
    cut_message("lookup - success: %d (%d:%d)", key, i, process_number));
  cut_assert_equal_string(value_string, value);

  hashes[i] = NULL;
  grn_test_assert(grn_hash_close(context, hash));

  contexts[i] = NULL;
  grn_test_assert(grn_ctx_fin(context));
}
