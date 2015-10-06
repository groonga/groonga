/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2015  Kouhei Sutou <kou@clear-code.com>

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

#include <string.h>

#include <groonga.h>

#include "lib/benchmark.h"

typedef struct _BenchmarkData
{
  gchar *base_dir;
  grn_ctx *context;
  grn_obj *result_set;
} BenchmarkData;

static void
bench_n(BenchmarkData *data, gint64 n)
{
  gint64 i;
  grn_ctx *ctx;
  grn_hash *result_set;

  ctx = data->context;
  result_set = (grn_hash *)data->result_set;
  for (i = 0; i < n; i++) {
    grn_id id = i;
    grn_hash_add(ctx, result_set, &id, sizeof(grn_id), NULL, NULL);
  }
}

static void
bench_1000(gpointer user_data)
{
  BenchmarkData *data = user_data;
  bench_n(data, 1000);
}

static void
bench_10000(gpointer user_data)
{
  BenchmarkData *data = user_data;
  bench_n(data, 10000);
}

static void
bench_100000(gpointer user_data)
{
  BenchmarkData *data = user_data;
  bench_n(data, 100000);
}

static void
bench_setup(gpointer user_data)
{
  BenchmarkData *data = user_data;
  gchar *database_path;
  grn_obj *source_table;
  const gchar *source_table_name = "Sources";

  bench_utils_remove_path_recursive_force(data->base_dir);
  g_mkdir_with_parents(data->base_dir, 0755);

  grn_ctx_init(data->context, 0);
  database_path = g_build_filename(data->base_dir, "db", NULL);
  grn_db_create(data->context, database_path, NULL);
  g_free(database_path);

  source_table = grn_table_create(data->context,
                                  source_table_name,
                                  strlen(source_table_name),
                                  NULL,
                                  GRN_TABLE_PAT_KEY | GRN_OBJ_PERSISTENT,
                                  grn_ctx_at(data->context, GRN_DB_SHORT_TEXT),
                                  NULL);
  data->result_set = grn_table_create(data->context,
                                      NULL, 0, NULL,
                                      GRN_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                      source_table,
                                      NULL);

}

static void
bench_teardown(gpointer user_data)
{
  BenchmarkData *data = user_data;

  grn_ctx_fin(data->context);
  bench_utils_remove_path_recursive_force(data->base_dir);
}

int
main(int argc, gchar **argv)
{
  BenchmarkData data;
  BenchReporter *reporter;
  gint n = 1;

  grn_init();
  bench_init(&argc, &argv);

  data.context = g_new(grn_ctx, 1);
  data.base_dir = g_build_filename(g_get_tmp_dir(), "groonga-bench", NULL);

  reporter = bench_reporter_new();
  bench_reporter_register(reporter, "1000", n,
                          bench_setup, bench_1000,   bench_teardown, &data);
  bench_reporter_register(reporter, "10000", n,
                          bench_setup, bench_10000,  bench_teardown, &data);
  bench_reporter_register(reporter, "100000", n,
                          bench_setup, bench_100000, bench_teardown, &data);
  bench_reporter_run(reporter);
  g_object_unref(reporter);

  bench_utils_remove_path_recursive_force(data.base_dir);

  g_free(data.context);

  bench_quit();
  grn_fin();

  return 0;
}
