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

void test_in(void);
void test_not_in(void);
void test_select(void);
void data_cursor(void);
void test_cursor(gconstpointer data);
void test_estimate(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *shops, *location_index, *result;

#define DEFINE_GEO_POINT(name) \
  static grn_obj *name ## _wgs84, *name ## _text
DEFINE_GEO_POINT(nezu_no_taiyaki);
DEFINE_GEO_POINT(takane);
DEFINE_GEO_POINT(sazare);
DEFINE_GEO_POINT(yanagi_ya);
DEFINE_GEO_POINT(hiiragi);
DEFINE_GEO_POINT(tokyo);
DEFINE_GEO_POINT(shinjuku);
DEFINE_GEO_POINT(nerima);
#undef DEFINE_GEO_POINT

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "geo-in-rectangle",
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

static grn_obj *
text_geo_point_open(double latitude, double longitude)
{
  grn_obj *point;

  point = grn_obj_open(context, GRN_BULK, 0, GRN_DB_SHORT_TEXT);
  GRN_TEXT_PUTS(context, point, cut_take_printf("%f,%f", latitude, longitude));
  return point;
}

static grn_obj *
wgs84_geo_point_open(double latitude, double longitude)
{
  grn_obj *point, *point_text;

  point_text = text_geo_point_open(latitude, longitude);
  point = grn_obj_open(context, GRN_BULK, 0, GRN_DB_WGS84_GEO_POINT);
  grn_obj_cast(context, point_text, point, GRN_FALSE);
  grn_obj_unlink(context, point_text);
  return point;
}

static void
setup_values(void)
{
#define SETUP_GEO_POINT(name, latitude, longitude)              \
  name ## _wgs84 = wgs84_geo_point_open(latitude, longitude);   \
  name ## _text = text_geo_point_open(latitude, longitude)

  SETUP_GEO_POINT(nezu_no_taiyaki, 35.720253, 139.762573);
  SETUP_GEO_POINT(takane, 35.698601, 139.560913);
  SETUP_GEO_POINT(sazare, 35.714653, 139.685043);
  SETUP_GEO_POINT(yanagi_ya, 35.685341, 139.783981);
  SETUP_GEO_POINT(hiiragi, 35.647701, 139.711517);

  SETUP_GEO_POINT(tokyo, 35.6814, 139.7660);
  SETUP_GEO_POINT(shinjuku, 35.6908, 139.7002);
  SETUP_GEO_POINT(nerima, 35.7377, 139.6535);

#undef SETUP_GEO_POINT
}

static void
load_data(void)
{
  assert_send_commands(cut_get_fixture_data_string("ddl.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("shops.grn", NULL));
}

void
cut_setup(void)
{
  const gchar *database_path;

  cut_set_fixture_data_dir(grn_test_get_base_dir(),
                           "fixtures",
                           "story",
                           "taiyaki",
                           NULL);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);

  setup_values();
  load_data();

  shops = get("Shops");
  location_index = get("Locations.shop");

  result = grn_table_create(context,
                            NULL, 0,
                            NULL,
                            GRN_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                            shops, NULL);
}

static void
teardown_values(void)
{
#define UNLINK_GEO_POINT(name)                  \
  grn_obj_unlink(context, name ## _wgs84);      \
  grn_obj_unlink(context, name ## _text)

  UNLINK_GEO_POINT(nezu_no_taiyaki);
  UNLINK_GEO_POINT(takane);
  UNLINK_GEO_POINT(sazare);
  UNLINK_GEO_POINT(yanagi_ya);
  UNLINK_GEO_POINT(hiiragi);

  UNLINK_GEO_POINT(tokyo);
  UNLINK_GEO_POINT(shinjuku);
  UNLINK_GEO_POINT(nerima);

#undef UNLINK_GEO_POINT
}

void
cut_teardown(void)
{
  teardown_values();

  grn_obj_unlink(context, result);
  grn_obj_unlink(context, location_index);
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

void
test_in(void)
{
  cut_assert_true(grn_geo_in_rectangle(context,
                                       shinjuku_wgs84,
                                       sazare_wgs84,
                                       hiiragi_wgs84));
}

void
test_not_in(void)
{
  cut_assert_false(grn_geo_in_rectangle(context,
                                        tokyo_wgs84,
                                        sazare_wgs84,
                                        hiiragi_wgs84));
}

static GList *
result_to_list(void)
{
  GList *list = NULL;
  grn_table_cursor *cursor;

  cursor = grn_table_cursor_open(context, result,
                                 NULL, 0,
                                 NULL, 0,
                                 0, -1,
                                 GRN_CURSOR_ASCENDING | GRN_CURSOR_BY_ID);
  while ((grn_table_cursor_next(context, cursor))) {
    void *result_key;
    gint result_key_size;
    grn_id shop_id;
    gchar key[GRN_TABLE_MAX_KEY_SIZE];
    gint key_size;

    result_key_size = grn_table_cursor_get_key(context, cursor, &result_key);
    memcpy(&shop_id, result_key, result_key_size);
    key_size = grn_table_get_key(context, shops, shop_id,
                                 &key, GRN_TABLE_MAX_KEY_SIZE);
    list = g_list_append(list, g_strndup(key, key_size));
  }
  gcut_take_list(list, g_free);

  return list;
}

void
test_select(void)
{
  grn_test_assert(grn_geo_select_in_rectangle(context,
                                              location_index,
                                              nerima_wgs84, tokyo_wgs84,
                                              result, GRN_OP_OR));
  gcut_assert_equal_list_string(
    gcut_take_new_list_string("soba-taiyaki-ku",
                              "sazare",
                              "hirose-ya",
                              "taiyaki-kataoka",
                              "kuruma",
                              "nezu-no-taiyaki",
                              NULL),
    result_to_list());
}

void
data_cursor(void)
{
#define ADD_DATA(label, expected, offset, limit)                        \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER, expected, gcut_list_string_free, \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 NULL)

  ADD_DATA("all",
           gcut_list_string_new("soba-taiyaki-ku",
                                "sazare",
                                "hirose-ya",
                                "taiyaki-kataoka",
                                "kuruma",
                                "nezu-no-taiyaki",
                                NULL),
           0, -1);
  ADD_DATA("offset",
           gcut_list_string_new("hirose-ya",
                                "taiyaki-kataoka",
                                "kuruma",
                                "nezu-no-taiyaki",
                                NULL),
           2, -1);
  ADD_DATA("limit",
           gcut_list_string_new("soba-taiyaki-ku",
                                "sazare",
                                "hirose-ya",
                                NULL),
           0, 3);
  ADD_DATA("offset - limit",
           gcut_list_string_new("hirose-ya",
                                "taiyaki-kataoka",
                                "kuruma",
                                NULL),
           2, 3);
  ADD_DATA("over offset",
           NULL,
           100, -1);
  ADD_DATA("0 limit",
           NULL,
           0, 0);

#undef ADD_DATA
}

void
test_cursor(gconstpointer data)
{
  GList *expected, *records = NULL;
  gint offset, limit;
  grn_obj *geo_cursor;
  grn_posting *posting;

  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  geo_cursor = grn_geo_cursor_open_in_rectangle(context,
                                                location_index,
                                                nerima_wgs84, tokyo_wgs84,
                                                offset, limit);
  while ((posting = grn_geo_cursor_next(context, geo_cursor))) {
    grn_id shop_id = posting->rid;
    gchar key[GRN_TABLE_MAX_KEY_SIZE];
    gint key_size;

    key_size = grn_table_get_key(context, shops, shop_id,
                                 &key, GRN_TABLE_MAX_KEY_SIZE);
    records = g_list_append(records, g_strndup(key, key_size));
  }
  grn_obj_unlink(context, geo_cursor);

  expected = (GList *)gcut_data_get_pointer(data, "expected");
  gcut_take_list(records, g_free);
  gcut_assert_equal_list_string(expected, records);
}

void
test_estimate_size(void)
{
  cut_assert_equal_uint(4,
                        grn_geo_estimate_size_in_rectangle(context,
                                                           location_index,
                                                           sazare_wgs84,
                                                           tokyo_wgs84));
}
