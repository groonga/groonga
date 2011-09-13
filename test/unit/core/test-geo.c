/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>

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

void test_in_circle(void);
void test_in_rectangle(void);
void data_distance(void);
void test_distance(gconstpointer data);
void test_distance2(void);
void test_distance3(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

#define DEFINE_GEO_POINT(name) \
  static grn_obj *name, *name ## _tokyo, *name ## _wgs84, *name ## _text
DEFINE_GEO_POINT(nedu_no_taiyaki);
DEFINE_GEO_POINT(takane);
DEFINE_GEO_POINT(sazare);
DEFINE_GEO_POINT(yanagi_ya);
DEFINE_GEO_POINT(hiiragi);
DEFINE_GEO_POINT(tokyo);
DEFINE_GEO_POINT(shinjuku);
#undef DEFINE_GEO_POINT

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "geo",
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
tokyo_geo_point_open(int latitude, int longitude)
{
  grn_obj *point;

  point = grn_obj_open(context, GRN_BULK, 0, GRN_DB_TOKYO_GEO_POINT);
  /* TODO: latitude and longitude are wgs84 format. They
   * should be converted to tokyo from wgs84. */
  GRN_GEO_POINT_SET(context, point, latitude, longitude);
  return point;
}

static grn_obj *
wgs84_geo_point_open(int latitude, int longitude)
{
  grn_obj *point;

  point = grn_obj_open(context, GRN_BULK, 0, GRN_DB_WGS84_GEO_POINT);
  GRN_GEO_POINT_SET(context, point, latitude, longitude);
  return point;
}

static grn_obj *
text_geo_point_open(int latitude, int longitude)
{
  grn_obj *point;

  point = grn_obj_open(context, GRN_BULK, 0, GRN_DB_SHORT_TEXT);
  GRN_TEXT_PUTS(context, point, cut_take_printf("%d,%d", latitude, longitude));
  return point;
}

static void
setup_values(void)
{
#define SETUP_GEO_POINT(name, latitude, longitude)              \
  name ## _tokyo = tokyo_geo_point_open(latitude, longitude);   \
  name ## _wgs84 = wgs84_geo_point_open(latitude, longitude);   \
  name ## _text = text_geo_point_open(latitude, longitude)

  SETUP_GEO_POINT(nedu_no_taiyaki, 130322053, 504985073);
  SETUP_GEO_POINT(takane, 130226001, 503769013);
  SETUP_GEO_POINT(sazare, 130306053, 504530043);
  SETUP_GEO_POINT(yanagi_ya, 130133052, 505120058);
  SETUP_GEO_POINT(hiiragi, 129917001, 504675017);

  SETUP_GEO_POINT(tokyo, 130101399, 505020000);
  SETUP_GEO_POINT(shinjuku, 130158300, 504604000);

#undef SETUP_GEO_POINT
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

  setup_values();
}

static void
teardown_values(void)
{
#define UNLINK_GEO_POINT(name)                  \
  grn_obj_unlink(context, name ## _tokyo);      \
  grn_obj_unlink(context, name ## _wgs84);      \
  grn_obj_unlink(context, name ## _text)

  UNLINK_GEO_POINT(nedu_no_taiyaki);
  UNLINK_GEO_POINT(takane);
  UNLINK_GEO_POINT(sazare);
  UNLINK_GEO_POINT(yanagi_ya);
  UNLINK_GEO_POINT(hiiragi);

  UNLINK_GEO_POINT(tokyo);
  UNLINK_GEO_POINT(shinjuku);

#undef UNLINK_GEO_POINT
}

void
cut_teardown(void)
{
  teardown_values();

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

void
test_in_circle(void)
{
  cut_assert_true(grn_geo_in_circle(context,
                                    hiiragi_wgs84,
                                    shinjuku_wgs84,
                                    tokyo_wgs84));
  cut_assert_false(grn_geo_in_circle(context,
                                     takane_wgs84,
                                     shinjuku_wgs84,
                                     tokyo_wgs84));
}

void
test_in_rectangle(void)
{
  cut_assert_true(grn_geo_in_rectangle(context,
                                       shinjuku_wgs84,
                                       sazare_wgs84,
                                       hiiragi_wgs84));
  cut_assert_false(grn_geo_in_rectangle(context,
                                        tokyo_wgs84,
                                        sazare_wgs84,
                                        hiiragi_wgs84));
}

static void
assign_shinjuku_and_takane(gconstpointer data)
{
  switch (gcut_data_get_int(data, "shinjuku-geographic-coordinate-system")) {
  case GRN_DB_TOKYO_GEO_POINT:
    shinjuku = shinjuku_tokyo;
    break;
  case GRN_DB_WGS84_GEO_POINT:
    shinjuku = shinjuku_wgs84;
    break;
  default:
    shinjuku = shinjuku_text;
    break;
  }

  switch (gcut_data_get_int(data, "takane-geographic-coordinate-system")) {
  case GRN_DB_TOKYO_GEO_POINT:
    takane = takane_tokyo;
    break;
  case GRN_DB_WGS84_GEO_POINT:
    takane = takane_wgs84;
    break;
  default:
    takane = takane_text;
    break;
  }
}

void
data_distance(void)
{
#define ADD_DATUM(label, shinjuku, takane)                      \
  gcut_add_datum(label,                                         \
                 "shinjuku-geographic-coordinate-system",       \
                 G_TYPE_INT, shinjuku,                          \
                 "takane-geographic-coordinate-system",         \
                 G_TYPE_INT, takane,                            \
                 NULL)

/*
  ADD_DATUM("tokyo - tokyo",
            GRN_DB_TOKYO_GEO_POINT, GRN_DB_TOKYO_GEO_POINT);
  ADD_DATUM("tokyo - wgs84",
            GRN_DB_TOKYO_GEO_POINT, GRN_DB_WGS84_GEO_POINT);
  ADD_DATUM("tokyo - text",
            GRN_DB_TOKYO_GEO_POINT, GRN_DB_SHORT_TEXT);
  ADD_DATUM("wgs84 - tokyo",
            GRN_DB_WGS84_GEO_POINT, GRN_DB_TOKYO_GEO_POINT);
*/
  ADD_DATUM("wgs84 - wgs84",
            GRN_DB_WGS84_GEO_POINT, GRN_DB_WGS84_GEO_POINT);
  ADD_DATUM("wgs84 - text",
            GRN_DB_WGS84_GEO_POINT, GRN_DB_SHORT_TEXT);
/*
  ADD_DATUM("text - tokyo",
            GRN_DB_SHORT_TEXT, GRN_DB_TOKYO_GEO_POINT);
*/
  ADD_DATUM("text - wgs84",
            GRN_DB_SHORT_TEXT, GRN_DB_WGS84_GEO_POINT);
  ADD_DATUM("text - text",
            GRN_DB_SHORT_TEXT, GRN_DB_SHORT_TEXT);

#undef ADD_DATUM
}

void
test_distance(gconstpointer data)
{
  assign_shinjuku_and_takane(data);
  cut_assert_equal_double(20881.0, 10,
                          grn_geo_distance(context, shinjuku, takane));
}

void
test_distance2(void)
{
  cut_assert_equal_double(20881.0, 10,
                          grn_geo_distance2(context,
                                            shinjuku_wgs84,
                                            takane_wgs84));
}

void
test_distance3(void)
{
  cut_assert_equal_double(20973.0, 10,
                          grn_geo_distance3(context,
                                            shinjuku_wgs84,
                                            takane_wgs84));
}
