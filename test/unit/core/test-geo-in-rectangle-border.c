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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <grn_str.h>
#include <grn_geo.h>

#define get(name) grn_ctx_get(context, name, strlen(name))

void data_cursor(void);
void test_cursor(gconstpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *points, *short_degree_column;
static grn_obj *location_index_column, *result;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "geo-in-rectangle-border",
                                   NULL);
}

void
cut_shutdown(void)
{
  g_free(tmp_directory);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

static void
load_data(void)
{
  assert_send_commands(cut_get_fixture_data_string("ddl.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("data.grn", NULL));
}

void
cut_setup(void)
{
  const gchar *database_path;

  cut_set_fixture_data_dir(grn_test_get_base_dir(),
                           "fixtures",
                           "geo",
                           NULL);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);

  load_data();

  points = get("Points");
  short_degree_column = get("Points.short_degree");
  location_index_column = get("Locations.point");

  result = grn_table_create(context,
                            NULL, 0,
                            NULL,
                            GRN_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                            points, NULL);
}

void
cut_teardown(void)
{
  grn_obj_unlink(context, result);
  grn_obj_unlink(context, location_index_column);
  grn_obj_unlink(context, short_degree_column);
  grn_obj_unlink(context, points);
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

#define ADD_DATA(label, expected, top, left, bottom, right)             \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER, expected, gcut_list_string_free, \
                 "top", G_TYPE_INT, top,                                \
                 "left", G_TYPE_INT, left,                              \
                 "bottom", G_TYPE_INT, bottom,                          \
                 "right", G_TYPE_INT, right,                            \
                 NULL)

static void
data_cursor_all_bottom_left(void)
{
  ADD_DATA("all - bottom left - bottom left",
           gcut_list_string_new(
             "(03,03)", "(03,04)", "(03,05)", "(03,06)", "(03,07)", "(03,08)",
             "(04,03)", "(04,04)", "(04,05)", "(04,06)", "(04,07)", "(04,08)",
             "(05,03)", "(05,04)", "(05,05)", "(05,06)", "(05,07)", "(05,08)",
             "(06,03)", "(06,04)", "(06,05)", "(06,06)", "(06,07)", "(06,08)",
             "(07,03)", "(07,04)", "(07,05)", "(07,06)", "(07,07)", "(07,08)",
             "(08,03)", "(08,04)", "(08,05)", "(08,06)", "(08,07)", "(08,08)",
             NULL),
           8, 3,
           3, 8);

  ADD_DATA("all - bottom left - top left",
           gcut_list_string_new(
             "(07,03)", "(07,04)", "(07,05)", "(07,06)", "(07,07)", "(07,08)",
             "(08,03)", "(08,04)", "(08,05)", "(08,06)", "(08,07)", "(08,08)",
             NULL),
           8, 3,
           7, 8);

  ADD_DATA("all - bottom left - bottom right",
           gcut_list_string_new("(03,07)", "(03,08)",
                                "(04,07)", "(04,08)",
                                "(05,07)", "(05,08)",
                                "(06,07)", "(06,08)",
                                "(07,07)", "(07,08)",
                                "(08,07)", "(08,08)",
                                NULL),
           8, 7,
           3, 8);
}

static void
data_cursor_all_top_left(void)
{
  ADD_DATA("all - top left - top left",
           gcut_list_string_new(
             "(07,03)", "(07,04)", "(07,05)", "(07,06)", "(07,07)", "(07,08)",
             "(08,03)", "(08,04)", "(08,05)", "(08,06)", "(08,07)", "(08,08)",
             "(09,03)", "(09,04)", "(09,05)", "(09,06)", "(09,07)", "(09,08)",
             "(10,03)", "(10,04)", "(10,05)", "(10,06)", "(10,07)", "(10,08)",
             "(11,03)", "(11,04)", "(11,05)", "(11,06)", "(11,07)", "(11,08)",
             "(12,03)", "(12,04)", "(12,05)", "(12,06)", "(12,07)", "(12,08)",
             NULL),
           12, 3,
           7, 8);

  ADD_DATA("all - top left - top right",
           gcut_list_string_new("(07,07)", "(07,08)",
                                "(08,07)", "(08,08)",
                                "(09,07)", "(09,08)",
                                "(10,07)", "(10,08)",
                                "(11,07)", "(11,08)",
                                "(12,07)", "(12,08)",
                                NULL),
           12, 7,
           7, 8);
}

static void
data_cursor_all_bottom_right(void)
{
  ADD_DATA("all - bottom right - bottom right",
           gcut_list_string_new(
             "(03,07)", "(03,08)", "(03,09)", "(03,10)", "(03,11)", "(03,12)",
             "(04,07)", "(04,08)", "(04,09)", "(04,10)", "(04,11)", "(04,12)",
             "(05,07)", "(05,08)", "(05,09)", "(05,10)", "(05,11)", "(05,12)",
             "(06,07)", "(06,08)", "(06,09)", "(06,10)", "(06,11)", "(06,12)",
             "(07,07)", "(07,08)", "(07,09)", "(07,10)", "(07,11)", "(07,12)",
             "(08,07)", "(08,08)", "(08,09)", "(08,10)", "(08,11)", "(08,12)",
             NULL),
           8, 7,
           3, 12);

  ADD_DATA("all - bottom right - top right",
           gcut_list_string_new(
             "(07,07)", "(07,08)", "(07,09)", "(07,10)", "(07,11)", "(07,12)",
             "(08,07)", "(08,08)", "(08,09)", "(08,10)", "(08,11)", "(08,12)",
             NULL),
           8, 7,
           7, 12);
}

static void
data_cursor_all_top_right(void)
{
  ADD_DATA("all - bottom right - top right",
           gcut_list_string_new(
             "(07,07)", "(07,08)", "(07,09)", "(07,10)", "(07,11)", "(07,12)",
             "(08,07)", "(08,08)", "(08,09)", "(08,10)", "(08,11)", "(08,12)",
             "(09,07)", "(09,08)", "(09,09)", "(09,10)", "(09,11)", "(09,12)",
             "(10,07)", "(10,08)", "(10,09)", "(10,10)", "(10,11)", "(10,12)",
             "(11,07)", "(11,08)", "(11,09)", "(11,10)", "(11,11)", "(11,12)",
             "(12,07)", "(12,08)", "(12,09)", "(12,10)", "(12,11)", "(12,12)",
             NULL),
           12, 7,
           7, 12);
}

static void
data_cursor_all(void)
{
  ADD_DATA("all - minimum",
           gcut_list_string_new("(07,07)", "(07,08)",
                                "(08,07)", "(08,08)",
                                NULL),
           8, 7,
           7, 8);

#define ALL_LONGITUDES(latitude)                        \
             "(" latitude ",00)", "(" latitude ",01)",  \
             "(" latitude ",02)", "(" latitude ",03)",  \
             "(" latitude ",04)", "(" latitude ",05)",  \
             "(" latitude ",06)", "(" latitude ",07)",  \
             "(" latitude ",08)", "(" latitude ",09)",  \
             "(" latitude ",10)", "(" latitude ",11)",  \
             "(" latitude ",12)", "(" latitude ",13)",  \
             "(" latitude ",14)", "(" latitude ",15)"

  ADD_DATA("all - maximum",
           gcut_list_string_new(
             ALL_LONGITUDES("00"), ALL_LONGITUDES("01"), ALL_LONGITUDES("02"),
             ALL_LONGITUDES("03"), ALL_LONGITUDES("04"), ALL_LONGITUDES("05"),
             ALL_LONGITUDES("06"), ALL_LONGITUDES("07"), ALL_LONGITUDES("08"),
             ALL_LONGITUDES("09"), ALL_LONGITUDES("10"), ALL_LONGITUDES("11"),
             ALL_LONGITUDES("12"), ALL_LONGITUDES("13"), ALL_LONGITUDES("14"),
             ALL_LONGITUDES("15"),
             NULL),
           15, 0,
           0, 15);

#undef ALL_LONGITUDES

  data_cursor_all_bottom_left();
  data_cursor_all_top_left();
  data_cursor_all_bottom_right();
  data_cursor_all_top_right();
}

static void
data_cursor_bottom(void)
{
  ADD_DATA("bottom - minimum",
           gcut_list_string_new("(03,07)", "(03,08)",
                                "(04,07)", "(04,08)",
                                NULL),
           4, 7,
           3, 8);

  ADD_DATA("bottom - left",
           gcut_list_string_new(
             "(03,03)", "(03,04)", "(03,05)", "(03,06)", "(03,07)", "(03,08)",
             "(04,03)", "(04,04)", "(04,05)", "(04,06)", "(04,07)", "(04,08)",
             NULL),
           4, 3,
           3, 8);

  ADD_DATA("bottom - right",
           gcut_list_string_new(
             "(03,07)", "(03,08)", "(03,09)", "(03,10)", "(03,11)", "(03,12)",
             "(04,07)", "(04,08)", "(04,09)", "(04,10)", "(04,11)", "(04,12)",
             NULL),
           4, 7,
           3, 12);
}

static void
data_cursor_top(void)
{
  ADD_DATA("top - minimum",
           gcut_list_string_new("(11,07)", "(11,08)",
                                "(12,07)", "(12,08)",
                                NULL),
           12, 7,
           11, 8);

  ADD_DATA("top - left",
           gcut_list_string_new(
             "(11,03)", "(11,04)", "(11,05)", "(11,06)", "(11,07)", "(11,08)",
             "(12,03)", "(12,04)", "(12,05)", "(12,06)", "(12,07)", "(12,08)",
             NULL),
           12, 3,
           11, 8);

  ADD_DATA("top - right",
           gcut_list_string_new(
             "(11,07)", "(11,08)", "(11,09)", "(11,10)", "(11,11)", "(11,12)",
             "(12,07)", "(12,08)", "(12,09)", "(12,10)", "(12,11)", "(12,12)",
             NULL),
           12, 7,
           11, 12);
}

static void
data_cursor_left(void)
{
  ADD_DATA("left - minimum",
           gcut_list_string_new("(07,03)", "(07,04)",
                                "(08,03)", "(08,04)",
                                NULL),
           8, 3,
           7, 4);

  ADD_DATA("left - bottom",
           gcut_list_string_new("(03,03)", "(03,04)",
                                "(04,03)", "(04,04)",
                                "(05,03)", "(05,04)",
                                "(06,03)", "(06,04)",
                                "(07,03)", "(07,04)",
                                "(08,03)", "(08,04)",
                                NULL),
           8, 3,
           3, 4);

  ADD_DATA("left - top",
           gcut_list_string_new("(07,03)", "(07,04)",
                                "(08,03)", "(08,04)",
                                "(09,03)", "(09,04)",
                                "(10,03)", "(10,04)",
                                "(11,03)", "(11,04)",
                                "(12,03)", "(12,04)",
                                NULL),
           12, 3,
           7, 4);
}

static void
data_cursor_right(void)
{
  ADD_DATA("right - minimum",
           gcut_list_string_new("(07,11)", "(07,12)",
                                "(08,11)", "(08,12)",
                                NULL),
           8, 11,
           7, 12);

  ADD_DATA("right - bottom",
           gcut_list_string_new("(03,11)", "(03,12)",
                                "(04,11)", "(04,12)",
                                "(05,11)", "(05,12)",
                                "(06,11)", "(06,12)",
                                "(07,11)", "(07,12)",
                                "(08,11)", "(08,12)",
                                NULL),
           8, 11,
           3, 12);

  ADD_DATA("right - top",
           gcut_list_string_new("(07,11)", "(07,12)",
                                "(08,11)", "(08,12)",
                                "(09,11)", "(09,12)",
                                "(10,11)", "(10,12)",
                                "(11,11)", "(11,12)",
                                "(12,11)", "(12,12)",
                                NULL),
           12, 11,
           7, 12);
}

void
data_cursor(void)
{
  data_cursor_all();
  data_cursor_bottom();
  data_cursor_top();
  data_cursor_left();
  data_cursor_right();
}

#undef ADD_DATA

static void
set_geo_point(grn_obj *geo_point,
              gint relative_latitude, gint relative_longitude)
{
  gint latitude, longitude;
  gint base_latitude = (45 * 60 * 60 + 0 * 60) * 1000;
  gint base_longitude = (90 * 60 * 60 + 0 * 60) * 1000;

  GRN_WGS84_GEO_POINT_INIT(geo_point, 0);
  latitude = base_latitude + relative_latitude;
  longitude = base_longitude + relative_longitude;
  GRN_GEO_POINT_SET(context, geo_point, latitude, longitude);
}

void
test_cursor(gconstpointer data)
{
  GList *expected, *records = NULL;
  gint offset = 0;
  gint limit = -1;
  grn_obj top_left, bottom_right;
  grn_obj *geo_cursor;
  grn_posting *posting;
  grn_obj short_degree;

  set_geo_point(&top_left,
                gcut_data_get_int(data, "top"),
                gcut_data_get_int(data, "left"));
  set_geo_point(&bottom_right,
                gcut_data_get_int(data, "bottom"),
                gcut_data_get_int(data, "right"));
  geo_cursor = grn_geo_cursor_open_in_rectangle(context,
                                                location_index_column,
                                                &top_left,
                                                &bottom_right,
                                                offset, limit);
  grn_obj_unlink(context, &top_left);
  grn_obj_unlink(context, &bottom_right);

  GRN_TEXT_INIT(&short_degree, 0);
  while ((posting = grn_geo_cursor_next(context, geo_cursor))) {
    grn_id point_id = posting->rid;

    GRN_BULK_REWIND(&short_degree);
    grn_obj_get_value(context, short_degree_column, point_id, &short_degree);
    records = g_list_append(records,
                            g_strndup(GRN_TEXT_VALUE(&short_degree),
                                      GRN_TEXT_LEN(&short_degree)));
  }
  grn_obj_unlink(context, &short_degree);
  grn_obj_unlink(context, geo_cursor);

  records = g_list_sort(records, (GCompareFunc)strcmp);
  gcut_take_list(records, g_free);

  expected = (GList *)gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected, records);
}

