/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010-2012  Kouhei Sutou <kou@clear-code.com>

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
#include <db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define COORDINATE(hours, minutes, seconds) \
  ((hours) * 3600 + (minutes) * 60 + (seconds)) * 1000

#define POINT(latitude_hours, latitude_minutes, latitude_seconds,       \
              longitude_hours, longitude_minutes, longitude_seconds)    \
  g_strdup_printf(                                                      \
    "%dx%d",                                                            \
    COORDINATE(latitude_hours, latitude_minutes, latitude_seconds),     \
    COORDINATE(longitude_hours, longitude_minutes, longitude_seconds))

#define TAKEN_POINT(latitude_hours, latitude_minutes, latitude_seconds, \
                    longitude_hours, longitude_minutes, longitude_seconds) \
  cut_take_string(POINT(latitude_hours, latitude_minutes, latitude_seconds, \
                        longitude_hours, longitude_minutes, longitude_seconds))

#define INSPECTED_POINT(latitude_hours, latitude_minutes, latitude_seconds, \
                        longitude_hours, longitude_minutes, longitude_seconds) \
  inspect_point(                                                        \
    COORDINATE(latitude_hours, latitude_minutes, latitude_seconds),     \
    COORDINATE(longitude_hours, longitude_minutes, longitude_seconds))

#define TAKEN_INSPECTED_POINT(latitude_hours,           \
                              latitude_minutes,         \
                              latitude_seconds,         \
                              longitude_hours,          \
                              longitude_minutes,        \
                              longitude_seconds)        \
  cut_take_string(INSPECTED_POINT(latitude_hours,       \
                                  latitude_minutes,     \
                                  latitude_seconds,     \
                                  longitude_hours,      \
                                  longitude_minutes,    \
                                  longitude_seconds))

void data_near_geo_point(void);
void test_near_geo_point(gpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *table, *column, *result;
static grn_table_cursor *cursor;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-patricia-trie-sort",
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

void
cut_setup(void)
{
  const gchar *database_path;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
  table = NULL;
  column = NULL;
  result = NULL;
  cursor = NULL;
}

void
cut_teardown(void)
{
  if (cursor) {
    grn_obj_unlink(context, cursor);
  }

  if (result) {
    grn_obj_unlink(context, result);
  }

  if (column) {
    grn_obj_unlink(context, column);
  }

  if (table) {
    grn_obj_unlink(context, table);
  }

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

static void
create_geo_table(const gchar *load_data)
{
  const char *table_name = "Data";
  const char *column_name = "location";

  assert_send_commands(
    cut_take_printf("table_create %s TABLE_NO_KEY", table_name));
  assert_send_commands(
    cut_take_printf("column_create %s %s COLUMN_SCALAR WGS84GeoPoint",
                    table_name, column_name));
  assert_send_commands("table_create Index TABLE_PAT_KEY WGS84GeoPoint");
  assert_send_commands(
    cut_take_printf("column_create Index %s_%s COLUMN_INDEX %s %s",
                    table_name, column_name,
                    table_name, column_name));
  assert_send_commands(
    cut_take_printf("load --table %s\n"
                    "[\n"
                    " [\"%s\"],\n"
                    "%s\n"
                    "]",
                    table_name,
                    column_name,
                    load_data));

  table = grn_ctx_get(context, table_name, strlen(table_name));
  column = grn_obj_column(context, table, column_name, strlen(column_name));
}

static gchar *
inspect_point (gint latitude, gint longitude)
{
  return g_strdup_printf("((%d, %d, %d), (%d, %d, %d))",
                         latitude / 1000 / 3600 % 3600,
                         latitude / 1000 / 60 % 60,
                         latitude / 1000 % 60,
                         longitude / 1000 / 3600 % 3600,
                         longitude / 1000 / 60 % 60,
                         longitude / 1000 % 60);
}

void
data_near_geo_point(void)
{
#define ADD_DATA(label, expected, base, offset, limit)                  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, g_list_free,                                 \
                 "base", G_TYPE_INT, base,                              \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 NULL)

  ADD_DATA("no limit",
           gcut_list_string_new(TAKEN_INSPECTED_POINT(-1, -1, -1,
                                                      0, 0, 0),
                                TAKEN_INSPECTED_POINT(1, 0, 0,
                                                      1, 0, 0),
                                TAKEN_INSPECTED_POINT(-1, 0, 0,
                                                      1, 1, 1),
                                TAKEN_INSPECTED_POINT(1, 1, 0,
                                                      1, 1, 0),
                                TAKEN_INSPECTED_POINT(1, 1, 1,
                                                      1, 1, 1),
                                TAKEN_INSPECTED_POINT(1, 1, 10,
                                                      -1, -1, -1),
                                TAKEN_INSPECTED_POINT(-1, -2, -1,
                                                      -1, -1, -1),
                                TAKEN_INSPECTED_POINT(90, 0, 0,
                                                      0, 0, 0),
                                TAKEN_INSPECTED_POINT(-90, 0, 0,
                                                      1, 0, 0),
                                TAKEN_INSPECTED_POINT(-2, -1, -1,
                                                      -179, -59, -59),
                                TAKEN_INSPECTED_POINT(2, 1, 1,
                                                      180, 0, 0),
                                TAKEN_INSPECTED_POINT(1, 2, 1,
                                                      -179, -59, -59),
                                TAKEN_INSPECTED_POINT(-1, -1, -1,
                                                      180, 0, 0),
                                TAKEN_INSPECTED_POINT(0, 0, 0,
                                                      -179, -59, -59),
                                TAKEN_INSPECTED_POINT(0, 0, 0,
                                                      180, 0, 0),
                                NULL),
           TAKEN_POINT(0, 0, 0,
                       0, 0, 0),
           0, -1);

  ADD_DATA("limit",
           gcut_list_string_new(TAKEN_INSPECTED_POINT(-1, -1, -1,
                                                      0, 0, 0),
                                TAKEN_INSPECTED_POINT(1, 0, 0,
                                                      1, 0, 0),
                                TAKEN_INSPECTED_POINT(-1, 0, 0,
                                                      1, 1, 1),
                                NULL),
           TAKEN_POINT(0, 0, 0,
                       0, 0, 0),
           0, 3);

  ADD_DATA("offset - limit",
           gcut_list_string_new(TAKEN_INSPECTED_POINT(-1, 0, 0,
                                                      1, 1, 1),
                                TAKEN_INSPECTED_POINT(1, 1, 0,
                                                      1, 1, 0),
                                TAKEN_INSPECTED_POINT(1, 1, 1,
                                                      1, 1, 1),
                                NULL),
           TAKEN_POINT(0, 0, 0,
                       0, 0, 0),
           2, 3);

#undef ADD_DATA
}

void
test_near_geo_point(gpointer data)
{
  grn_id id;
  int offset, limit;
  const GList *expected_keys;
  GList *actual_keys = NULL;
  grn_table_sort_key keys[2];
  grn_obj base, base_string, location;

  create_geo_table(cut_take_printf(" [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"],\n"
                                   " [\"%s\"]",
                                   TAKEN_POINT(0, 0, 0,
                                               180, 0, 0),
                                   TAKEN_POINT(0, 0, 0,
                                               -179, -59, -59),
                                   TAKEN_POINT(-1, -1, -1,
                                               180, 0, 0),
                                   TAKEN_POINT(2, 1, 1,
                                               180, 0, 0),
                                   TAKEN_POINT(-2, -1, -1,
                                               -179, -59, -59),
                                   TAKEN_POINT(1, 2, 1,
                                               -179, -59, -59),
                                   TAKEN_POINT(90, 0, 0,
                                               0, 0, 0),
                                   TAKEN_POINT(-90, 0, 0,
                                               1, 0, 0),
                                   TAKEN_POINT(1, 0, 0,
                                               1, 0, 0),
                                   TAKEN_POINT(1, 1, 0,
                                               1, 1, 0),
                                   TAKEN_POINT(1, 1, 1,
                                               1, 1, 1),
                                   TAKEN_POINT(-1, 0, 0,
                                               1, 1, 1),
                                   TAKEN_POINT(-1, -1, -1,
                                               0, 0, 0),
                                   TAKEN_POINT(-1, -2, -1,
                                               -1, -1, -1),
                                   TAKEN_POINT(1, 1, 10,
                                               -1, -1, -1)));

  result = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_NO_KEY,
                            NULL, table);
  grn_test_assert_context(context);

  GRN_TEXT_INIT(&base_string, 0);
  GRN_TEXT_SETS(context, &base_string, gcut_data_get_string(data, "base"));
  GRN_WGS84_GEO_POINT_INIT(&base, 0);
  grn_obj_cast(context, &base_string, &base, FALSE);
  GRN_OBJ_FIN(context, &base_string);

  offset = gcut_data_get_int(data, "offset");
  if (offset > 0) {
    cut_omit("geo sort doesn't support offset yet.");
  }
  limit = gcut_data_get_int(data, "limit");
  keys[0].key = column;
  keys[0].flags = GRN_TABLE_SORT_GEO;
  keys[0].offset = 0;
  keys[1].key = &base;
  keys[1].flags = 0;
  keys[1].offset = 0;
  grn_table_sort(context, table, offset, limit, result, keys, 2);
  GRN_OBJ_FIN(context, &base);
  grn_test_assert_context(context);
  cursor = grn_table_cursor_open(context, result,
                                 NULL, 0, NULL, 0, 0, -1,
                                 GRN_CURSOR_ASCENDING);
  grn_test_assert_context(context);
  GRN_WGS84_GEO_POINT_INIT(&location, 0);
  while ((id = grn_table_cursor_next(context, cursor))) {
    gint32 *key;
    int key_size;
    gint latitude, longitude;

    key_size = grn_table_cursor_get_value(context, cursor, (void **)&key);
    GRN_BULK_REWIND(&location);
    grn_obj_get_value(context, column, *key, &location);
    GRN_GEO_POINT_VALUE(&location, latitude, longitude);
    actual_keys = g_list_append(actual_keys,
                                inspect_point(latitude, longitude));
  }
  GRN_OBJ_FIN(context, &location);
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}
