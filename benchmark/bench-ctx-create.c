/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2013  Kouhei Sutou <kou@clear-code.com>

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

/*
  Groonga: 4851bf7c20fd37e7c40f350dade3716d3e210805
  CFLAGS: -O0 -g3
  % (cd benchmark/ && make --quiet run-bench-ctx-create)
  run-bench-ctx-create:
  ./bench-ctx-create
                   (time)
    with    mruby: 404KB (0.1260ms)
    without mruby:  36KB (0.0610ms)
*/

#include <stdlib.h>

#include <glib.h>

#include <groonga.h>

#include "lib/benchmark.h"

typedef struct _BenchmarkData {
  grn_ctx context;
  grn_obj *database;
  guint memory_usage_before;
} BenchmarkData;

static guint
get_memory_usage(void)
{
  GRegex *vm_rss_pattern;
  gchar *status;
  GMatchInfo *match_info;
  gchar *vm_rss_string;
  guint vm_rss;

  g_file_get_contents("/proc/self/status", &status, NULL, NULL);

  vm_rss_pattern = g_regex_new("VmRSS:\\s*(\\d*)\\s+kB", 0, 0, NULL);
  if (!g_regex_match(vm_rss_pattern, status, 0, &match_info)) {
    g_print("not match...: %s\n", status);
    return 0;
  }
  vm_rss_string = g_match_info_fetch(match_info, 1);
  vm_rss = atoi(vm_rss_string);
  g_free(vm_rss_string);
  g_match_info_free(match_info);
  g_regex_unref(vm_rss_pattern);
  g_free(status);

  return vm_rss;
}

static void
bench_with_mruby(gpointer user_data)
{
  BenchmarkData *data = user_data;

  g_setenv("GRN_MRUBY_ENABLED", "yes", TRUE);
  grn_ctx_init(&(data->context), 0);
  grn_ctx_use(&(data->context), data->database);
}

static void
bench_without_mruby(gpointer user_data)
{
  BenchmarkData *data = user_data;

  g_setenv("GRN_MRUBY_ENABLED", "no", TRUE);
  grn_ctx_init(&(data->context), 0);
  grn_ctx_use(&(data->context), data->database);
}

static void
bench_setup(gpointer user_data)
{
  BenchmarkData *data = user_data;

  data->memory_usage_before = get_memory_usage();
}

static void
bench_teardown(gpointer user_data)
{
  BenchmarkData *data = user_data;

  grn_ctx_fin(&(data->context));
  g_print("%3dKB ", get_memory_usage() - data->memory_usage_before);
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

static grn_obj *
setup_database(grn_ctx *context)
{
  gchar *tmp_dir;
  gchar *database_path;
  grn_obj *database;

  tmp_dir = get_tmp_dir();
  database_path = g_build_filename(tmp_dir, "ctx-create", "db", NULL);
  database = grn_db_open(context, database_path);

  g_free(database_path);

  return database;
}

static void
teardown_database(grn_ctx *context, grn_obj *database)
{
  grn_obj_close(context, database);
}

int
main(int argc, gchar **argv)
{
  grn_ctx context;
  BenchmarkData data;
  BenchReporter *reporter;
  gint n = 1;

  grn_init();
  bench_init(&argc, &argv);

  grn_ctx_init(&context, 0);

  data.database = setup_database(&context);

  reporter = bench_reporter_new();

#define REGISTER(label, bench_function)                 \
  bench_reporter_register(reporter, label, n,           \
                          bench_setup,                  \
                          bench_function,               \
                          bench_teardown,               \
                          &data)
  REGISTER("with    mruby1", bench_with_mruby);
  REGISTER("without mruby1", bench_without_mruby);
  REGISTER("with    mruby2", bench_with_mruby);
  REGISTER("without mruby2", bench_without_mruby);
#undef REGISTER

  bench_reporter_run(reporter);
  g_object_unref(reporter);

  teardown_database(&context, data.database);

  grn_ctx_fin(&context);

  grn_fin();

  return 0;
}
