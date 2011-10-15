/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
  groonga: 3ad91b868909444f66a36dbcbdbe2292ed14bd72
  CFLAGS: -O0 -g
  CPU: Intel(R) Core(TM) i5 CPU       U 470  @ 1.33GHz stepping 05

  % (cd test/benchmark; make --quiet run-bench-geo-select)
  run-bench-geo-select:
                         (time)
    select_in_rectangle: (1.84038)
*/

#include <string.h>

#include <db.h>
#include <groonga.h>

#include "lib/benchmark.h"

#define GET(context, name) (grn_ctx_get(context, name, strlen(name)))

typedef struct _BenchmarkData
{
  gboolean report_result;

  grn_ctx *context;
  grn_obj *database;
  grn_obj *table;
  grn_obj *index_column;
  grn_obj *result;

  grn_obj top_left_point;
  grn_obj bottom_right_point;
} BenchmarkData;

static void
set_geo_point(grn_ctx *context, grn_obj *geo_point, const gchar *geo_point_text)
{
  grn_obj point_text;

  GRN_TEXT_INIT(&point_text, 0);
  GRN_TEXT_PUTS(context, &point_text, geo_point_text);
  grn_obj_cast(context, &point_text, geo_point, GRN_FALSE);
  grn_obj_unlink(context, &point_text);
}

static void
bench_setup(gpointer user_data)
{
  BenchmarkData *data = user_data;
  const gchar *tokyo_station = "35.68136,139.76609";
  const gchar *ikebukuro_station = "35.72890,139.71036";

  data->result = grn_table_create(data->context, NULL, 0, NULL,
                                  GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                  data->table, NULL);

  set_geo_point(data->context, &(data->top_left_point),
                ikebukuro_station);
  set_geo_point(data->context, &(data->bottom_right_point),
                tokyo_station);
}

static void
bench_geo_select_in_rectangle(gpointer user_data)
{
  BenchmarkData *data = user_data;

  grn_geo_select_in_rectangle(data->context,
                              data->index_column,
                              &(data->top_left_point),
                              &(data->bottom_right_point),
                              data->result,
                              GRN_OP_OR);
}

static void
bench_teardown(gpointer user_data)
{
  BenchmarkData *data = user_data;

  if (data->report_result) {
    g_print("result: %d\n", grn_table_size(data->context, data->result));
  }

  grn_obj_unlink(data->context, data->result);
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
  gchar *tmp_dir;
  gchar *database_path;

  tmp_dir = get_tmp_dir();
  database_path = g_build_filename(tmp_dir, "geo-select", "db", NULL);
  data->database = grn_db_open(data->context, database_path);

  data->table = GET(data->context, "Addresses");
  data->index_column = GET(data->context, "Locations.address");

  g_free(database_path);
}

static void
teardown_database(BenchmarkData *data)
{
  grn_obj_unlink(data->context, data->index_column);
  grn_obj_unlink(data->context, data->table);
  grn_obj_unlink(data->context, data->database);
}

int
main(int argc, gchar **argv)
{
  BenchmarkData data;
  BenchReporter *reporter;
  gint n = 100;

  grn_init();
  bench_init(&argc, &argv);

  data.report_result = g_getenv("GROONGA_BENCH_REPORT_RESULT") != NULL;

  data.context = g_new(grn_ctx, 1);
  grn_ctx_init(data.context, 0);

  setup_database(&data);
  GRN_WGS84_GEO_POINT_INIT(&(data.top_left_point), 0);
  GRN_WGS84_GEO_POINT_INIT(&(data.bottom_right_point), 0);

  {
    const gchar *groonga_bench_n;
    groonga_bench_n = g_getenv("GROONGA_BENCH_N");
    if (groonga_bench_n) {
      n = atoi(groonga_bench_n);
    }
  }

  reporter = bench_reporter_new();

#define REGISTER(label, type)                           \
  bench_reporter_register(reporter, label, n,           \
                          bench_setup,                  \
                          bench_geo_select_ ## type,    \
                          bench_teardown,               \
                          &data)
  REGISTER("select_in_rectangle", in_rectangle);
#undef REGISTER

  bench_reporter_run(reporter);
  g_object_unref(reporter);

  grn_obj_unlink(data.context, &(data.top_left_point));
  grn_obj_unlink(data.context, &(data.bottom_right_point));
  teardown_database(&data);

  grn_ctx_fin(data.context);
  g_free(data.context);

  bench_quit();
  grn_fin();

  return 0;
}
