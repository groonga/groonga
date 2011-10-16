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
  groonga: e65e87f0039f6bb2da3d9b959423774d3f66b567
  CFLAGS: -O0 -ggdb3
  CPU: Intel(R) Core(TM) i7 CPU         860  @ 2.80GHz stepping 05

  % (cd test/benchmark; make --quiet run-bench-geo-select)
  run-bench-geo-select:
                                        (time)
    select_in_rectangle (1st)   (loop): (0.943075)
    select_in_rectangle (1st) (cursor): (0.937281)
    select_in_rectangle (2nd)   (loop): (0.928663)
    select_in_rectangle (2nd) (cursor): (0.947011)
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

  const gchar *original_in_rectangle_implementation;
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
bench_setup_common(gpointer user_data)
{
  BenchmarkData *data = user_data;
  const gchar *tokyo_station = "35.68136,139.76609";
  const gchar *ikebukuro_station = "35.72890,139.71036";

  data->original_in_rectangle_implementation =
    g_getenv("GRN_GEO_SELECT_IN_RECTANGLE");

  data->result = grn_table_create(data->context, NULL, 0, NULL,
                                  GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                  data->table, NULL);

  set_geo_point(data->context, &(data->top_left_point),
                ikebukuro_station);
  set_geo_point(data->context, &(data->bottom_right_point),
                tokyo_station);
}

static void
bench_setup_implementation_loop(gpointer user_data)
{
  g_setenv("GRN_GEO_SELECT_IN_RECTANGLE", "loop", TRUE);
}

static void
bench_setup_implementation_cursor(gpointer user_data)
{
  g_setenv("GRN_GEO_SELECT_IN_RECTANGLE", "cursor", TRUE);
}

static void
bench_setup_query_partial(gpointer user_data)
{
  BenchmarkData *data = user_data;
  const gchar *tokyo_station = "35.68136,139.76609";
  const gchar *ikebukuro_station = "35.72890,139.71036";

  set_geo_point(data->context, &(data->top_left_point),
                ikebukuro_station);
  set_geo_point(data->context, &(data->bottom_right_point),
                tokyo_station);
}

static void
bench_setup_query_all(gpointer user_data)
{
  BenchmarkData *data = user_data;
  const gchar *tokyo_station = "35.0,140.0";
  const gchar *ikebukuro_station = "36.0,139.0";

  set_geo_point(data->context, &(data->top_left_point),
                ikebukuro_station);
  set_geo_point(data->context, &(data->bottom_right_point),
                tokyo_station);
}

static void
bench_setup_in_rectangle_loop_partial(gpointer user_data)
{
  bench_setup_common(user_data);
  bench_setup_implementation_loop(user_data);
  bench_setup_query_partial(user_data);
}

static void
bench_setup_in_rectangle_cursor_partial(gpointer user_data)
{
  bench_setup_common(user_data);
  bench_setup_implementation_cursor(user_data);
  bench_setup_query_partial(user_data);
}

static void
bench_setup_in_rectangle_loop_all(gpointer user_data)
{
  bench_setup_common(user_data);
  bench_setup_implementation_loop(user_data);
  bench_setup_query_all(user_data);
}

static void
bench_setup_in_rectangle_cursor_all(gpointer user_data)
{
  bench_setup_common(user_data);
  bench_setup_implementation_cursor(user_data);
  bench_setup_query_all(user_data);
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

  if (data->original_in_rectangle_implementation) {
    g_setenv("GRN_GEO_SELECT_IN_RECTANGLE",
             data->original_in_rectangle_implementation,
             TRUE);
  } else {
    g_unsetenv("GRN_GEO_SELECT_IN_RECTANGLE");
  }
  data->original_in_rectangle_implementation = NULL;
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

#define REGISTER(label, type, implementation, area)                     \
  bench_reporter_register(                                              \
    reporter,                                                           \
    label,                                                              \
    n,                                                                  \
    bench_setup_ ## type ## _ ## implementation ## _ ## area,           \
    bench_geo_select_ ## type,                                          \
    bench_teardown,                                                     \
    &data)
  REGISTER("1st: select_in_rectangle   (loop) (partial)",
           in_rectangle, loop, partial);
  REGISTER("1st: select_in_rectangle (cursor) (partial)",
           in_rectangle, cursor, partial);
  REGISTER("2nd: select_in_rectangle   (loop) (partial)",
           in_rectangle, loop, partial);
  REGISTER("2nd: select_in_rectangle (cursor) (partial)",
           in_rectangle, cursor, partial);
  REGISTER("1st: select_in_rectangle   (loop)     (all)",
           in_rectangle, loop, all);
  REGISTER("1st: select_in_rectangle (cursor)     (all)",
           in_rectangle, cursor, all);
  REGISTER("2nd: select_in_rectangle   (loop)     (all)",
           in_rectangle, loop, all);
  REGISTER("2nd: select_in_rectangle (cursor)     (all)",
           in_rectangle, cursor, all);
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
