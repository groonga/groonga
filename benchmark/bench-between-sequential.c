/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2016  Kouhei Sutou <kou@clear-code.com>

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

#include <stdio.h>
#include <string.h>

#include <grn_db.h>
#include <groonga.h>

#include "lib/benchmark.h"

#define GET(context, name) (grn_ctx_get(context, name, strlen(name)))

typedef struct _BenchmarkData
{
  grn_ctx context;
  grn_obj *database;
  guint n_records;
  const gchar *command;
} BenchmarkData;

static void
run_command(grn_ctx *context, const gchar *command)
{
  gchar *response;
  unsigned int response_length;
  int flags;

  grn_ctx_send(context, command, strlen(command), 0);
  grn_ctx_recv(context, &response, &response_length, &flags);
}

static void
bench(gpointer user_data)
{
  BenchmarkData *data = user_data;
  grn_ctx *context = &(data->context);

  run_command(context, data->command);
}

static gchar *
get_tmp_dir(void)
{
  gchar *current_dir;
  gchar *tmp_dir;

  current_dir = g_get_current_dir();
  tmp_dir = g_build_filename(current_dir, "tmp", NULL);
  g_free(current_dir);

  return tmp_dir;
}

static void
setup_database(BenchmarkData *data)
{
  grn_ctx *context = &(data->context);
  gchar *tmp_dir;
  gchar *database_last_component_name;
  gchar *database_path;
  guint i;

  tmp_dir = get_tmp_dir();
  database_last_component_name = g_strdup_printf("db-%d", data->n_records);
  database_path = g_build_filename(tmp_dir,
                                   "between-sequential",
                                   database_last_component_name,
                                   NULL);
  g_free(database_last_component_name);

  if (g_file_test(database_path, G_FILE_TEST_EXISTS)) {
    data->database = grn_db_open(context, database_path);
    run_command(context, "dump");
  } else {
    data->database = grn_db_create(context, database_path, NULL);

    run_command(context, "table_create Entries TABLE_NO_KEY");
    run_command(context, "column_create Entries rank COLUMN_SCALAR Int32");

    run_command(context, "load --table Entries");
    run_command(context, "[");
    for (i = 0; i < data->n_records; i++) {
#define BUFFER_SIZE 4096
      gchar buffer[BUFFER_SIZE];
      const gchar *separator;
      if (i == (data->n_records - 1)) {
        separator = "";
      } else {
        separator = ",";
      }
      snprintf(buffer, BUFFER_SIZE, "{\"rank\": %u}%s", i, separator);
      run_command(context, buffer);
#undef BUFFER_SIZE
    }
    run_command(context, "]");
  }

  g_free(database_path);
}

static void
bench_startup(BenchmarkData *data)
{
  grn_ctx_init(&(data->context), 0);
  setup_database(data);
}

static void
bench_shutdown(BenchmarkData *data)
{
  grn_ctx *context = &(data->context);

  grn_obj_close(context, data->database);
  grn_ctx_fin(context);
}

int
main(int argc, gchar **argv)
{
  grn_rc rc;
  BenchReporter *reporter;
  gint n = 10;

  rc = grn_init();
  if (rc != GRN_SUCCESS) {
    g_print("failed to initialize Groonga: <%d>: %s\n",
            rc, grn_get_global_error_message());
    return EXIT_FAILURE;
  }

  g_print("Process %d times in each pattern\n", n);

  bench_init(&argc, &argv);
  reporter = bench_reporter_new();

  {
    BenchmarkData data_small;
    BenchmarkData data_medium;
    BenchmarkData data_large;
    BenchmarkData data_very_large;

#define REGISTER(data, n_records_, min, max)                    \
    do {                                                        \
      gchar *label;                                             \
      label = g_strdup_printf("(%6d, %6d] (%7d)",               \
                              min, max, n_records_);            \
      data.n_records = n_records_;                              \
      data.command =                                            \
        "select Entries --cache no "                            \
        "--filter "                                             \
        "'between(rank, " #min ", \"exclude\","                 \
                      " " #max ", \"include\")'";               \
      bench_startup(&data);                                     \
      bench_reporter_register(reporter, label,                  \
                              n,                                \
                              NULL,                             \
                              bench,                            \
                              NULL,                             \
                              &data);                           \
      g_free(label);                                            \
    } while(FALSE)

    REGISTER(data_small,
             1000,
             500, 600);
    REGISTER(data_medium,
             10000,
             5000, 5100);
    REGISTER(data_large,
             100000,
             50000, 50100);
    REGISTER(data_very_large,
             1000000,
             500000, 500100);

#undef REGISTER

    bench_reporter_run(reporter);

    bench_shutdown(&data_small);
    bench_shutdown(&data_medium);
    bench_shutdown(&data_large);
    bench_shutdown(&data_very_large);
  }
  g_object_unref(reporter);

  grn_fin();

  return EXIT_SUCCESS;
}
