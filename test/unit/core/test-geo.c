/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>

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
void test_distance(void);
void test_distance2(void);
void test_distance3(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static grn_obj *nedu_no_taiyaki, *takane, *sazare, *yanagi_ya, *hiiragi;
static grn_obj *tokyo, *shinjuku;

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
wgs84_geo_point_open(int latitude, int longitude)
{
  grn_obj *point;

  point = grn_obj_open(context, GRN_BULK, 0, GRN_DB_WGS84_GEO_POINT);
  GRN_GEO_POINT_SET(context, point, latitude, longitude);
  return point;
}

static void
setup_values(void)
{
  nedu_no_taiyaki = wgs84_geo_point_open(130322053, 504985073);
  takane = wgs84_geo_point_open(130226001, 503769013);
  sazare = wgs84_geo_point_open(130306053, 504530043);
  yanagi_ya = wgs84_geo_point_open(130133052, 505120058);
  hiiragi = wgs84_geo_point_open(129917001, 504675017);

  tokyo = wgs84_geo_point_open(130101399, 505020000);
  shinjuku = wgs84_geo_point_open(130158300, 504604000);
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
  grn_obj_unlink(context, nedu_no_taiyaki);
  grn_obj_unlink(context, takane);
  grn_obj_unlink(context, sazare);
  grn_obj_unlink(context, yanagi_ya);
  grn_obj_unlink(context, hiiragi);
  grn_obj_unlink(context, tokyo);
  grn_obj_unlink(context, shinjuku);
}

void
cut_teardown(void)
{
  teardown_values();

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_in_circle(void)
{
  cut_assert_true(grn_geo_in_circle(context,
                                    hiiragi,
                                    shinjuku,
                                    tokyo));
  cut_assert_false(grn_geo_in_circle(context,
                                     takane,
                                     shinjuku,
                                     tokyo));
}

void
test_in_rectangle(void)
{
  cut_assert_true(grn_geo_in_rectangle(context,
                                       shinjuku,
                                       sazare,
                                       hiiragi));
  cut_assert_false(grn_geo_in_rectangle(context,
                                        tokyo,
                                        sazare,
                                        hiiragi));
}

void
test_distance(void)
{
  cut_assert_equal_double(20881.0, 10,
                          grn_geo_distance(context, shinjuku, takane));
}

void
test_distance2(void)
{
  cut_assert_equal_double(20881.0, 10,
                          grn_geo_distance2(context, shinjuku, takane));
}

void
test_distance3(void)
{
  cut_assert_equal_double(20973.0, 10,
                          grn_geo_distance3(context, shinjuku, takane));
}
