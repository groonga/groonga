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

void test_default(void);
void test_rectangle(void);
void test_sphere(void);
void test_ellipsoid(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-select-geo-in-rectangle",
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

  assert_send_commands(cut_get_fixture_data_string("ddl.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("areas.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("categories.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("shops.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("synonyms.grn", NULL));
}

void
cut_teardown(void)
{
  if (context) {
    grn_obj_unlink(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_default(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[7],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"],"
    "[\"location\",\"WGS84GeoPoint\"]],"
    "[\"柳屋 たい焼き\",-2147483648,\"128467228x503222332\"],"
    "[\"銀座 かずや\",280743810,\"128424629x503139222\"],"
    "[\"たい焼き鉄次 大丸東京店\",810303031,\"128451283x503166852\"],"
    "[\"たいやき神田達磨 八重洲店\",970517026,\"128453260x503174156\"],"
    "[\"にしみや 甘味処\",1056698886,\"128418570x503188661\"],"
    "[\"築地 さのきや\",1186376492,\"128397312x503174596\"],"
    "[\"しげ田\",1530425643,\"128421454x503208983\"]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score, location' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer "
          "'_score = geo_distance(location, \"%s\") * 1000 * 1000'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_rectangle(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[7],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"],"
    "[\"location\",\"WGS84GeoPoint\"]],"
    "[\"柳屋 たい焼き\",-2147483648,\"128467228x503222332\"],"
    "[\"銀座 かずや\",280743810,\"128424629x503139222\"],"
    "[\"たい焼き鉄次 大丸東京店\",810303031,\"128451283x503166852\"],"
    "[\"たいやき神田達磨 八重洲店\",970517026,\"128453260x503174156\"],"
    "[\"にしみや 甘味処\",1056698886,\"128418570x503188661\"],"
    "[\"築地 さのきや\",1186376492,\"128397312x503174596\"],"
    "[\"しげ田\",1530425643,\"128421454x503208983\"]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score, location' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer "
          "'_score = geo_distance(location, \"%s\", \"rectangle\") * 1000 * 1000'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_sphere(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[7],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"],"
    "[\"location\",\"WGS84GeoPoint\"]],"
    "[\"柳屋 たい焼き\",-2147483648,\"128467228x503222332\"],"
    "[\"銀座 かずや\",280743810,\"128424629x503139222\"],"
    "[\"たい焼き鉄次 大丸東京店\",810303031,\"128451283x503166852\"],"
    "[\"たいやき神田達磨 八重洲店\",970517025,\"128453260x503174156\"],"
    "[\"にしみや 甘味処\",1056698886,\"128418570x503188661\"],"
    "[\"築地 さのきや\",1186376491,\"128397312x503174596\"],"
    "[\"しげ田\",1530425641,\"128421454x503208983\"]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score, location' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer "
          "'_score = geo_distance(location, \"%s\", \"sphere\") * 1000 * 1000'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_ellipsoid(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[7],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"],"
    "[\"location\",\"WGS84GeoPoint\"]],"
    "[\"柳屋 たい焼き\",-2147483648,\"128467228x503222332\"],"
    "[\"銀座 かずや\",281623827,\"128424629x503139222\"],"
    "[\"たい焼き鉄次 大丸東京店\",811420890,\"128451283x503166852\"],"
    "[\"たいやき神田達磨 八重洲店\",972359708,\"128453260x503174156\"],"
    "[\"にしみや 甘味処\",1060891168,\"128418570x503188661\"],"
    "[\"築地 さのきや\",1187926579,\"128397312x503174596\"],"
    "[\"しげ田\",1537012099,\"128421454x503208983\"]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score, location' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer "
          "'_score = geo_distance(location, \"%s\", \"ellipsoid\") * 1000 * 1000'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}
