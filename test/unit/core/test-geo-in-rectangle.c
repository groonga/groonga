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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <str.h>
#include <geo.h>

#define get(name) grn_ctx_get(context, name, strlen(name))

void test_in(void);
void test_not_in(void);
void test_select(void);
void test_estimate(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *shops, *location_index, *result;

#define DEFINE_GEO_POINT(name) \
  static grn_obj *name, *name ## _wgs84, *name ## _text
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

void
test_select(void)
{
  grn_test_assert(grn_geo_select_in_rectangle(context,
                                              location_index,
                                              nerima_wgs84, tokyo_wgs84,
                                              result, GRN_OP_OR));
  cut_assert_equal_int(6,
                       grn_table_size(context, result));
}

void
test_estimate(void)
{
  cut_assert_equal_int(4,
                       grn_geo_estimate_in_rectangle(context,
                                                     location_index,
                                                     sazare_wgs84,
                                                     tokyo_wgs84));
}
