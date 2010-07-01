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

#include "../../lib/grn-assertions.h"

void test_in_circle(void);
void test_filter_by_tag_and_sort_by_distance_from_tokyo_tocho(void);
void test_but_white(void);
void test_drilldown(void);
void test_drilldown_with_broken_reference(void);
void test_weight_match(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "taiyaki-geo",
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
test_in_circle(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[6],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"銀座 かずや\",795],"
    "[\"たいやき神田達磨 八重洲店\",1079],"
    "[\"たい焼き鉄次 大丸東京店\",1390],"
    "[\"築地 さのきや\",1723],"
    "[\"にしみや 甘味処\",2000],"
    "[\"しげ田\",2272]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer '_score=geo_distance(location, \"%s\")'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_filter_by_tag_and_sort_by_distance_from_tokyo_tocho(void)
{
  gdouble tokyo_tocho_latitude = 35.689444;
  gdouble tokyo_tocho_longitude = 139.691667;

  cut_assert_equal_string(
    "[[[31],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"さざれ\",4110],"
    "[\"広瀬屋\",4149],"
    "[\"そばたいやき空\",4507],"
    "[\"たい焼 カタオカ\",5210],"
    "[\"車\",5600],"
    "[\"吾妻屋\",6099],"
    "[\"たいやき工房白家 阿佐ヶ谷店\",7646],"
    "[\"たいやき本舗 藤家 阿佐ヶ谷店\",7861],"
    "[\"たいやきひいらぎ\",8463],"
    "[\"代官山たい焼き黒鯛\",8668]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance(location, \"%s\")'",
        grn_test_location_string(tokyo_tocho_latitude, tokyo_tocho_longitude))));
}

void
test_in_circle_and_tag(void)
{
  gdouble tokyo_tocho_latitude = 35.689444;
  gdouble tokyo_tocho_longitude = 139.691667;
  gint distance = 5 * 1000;

  cut_assert_equal_string(
    "[[[3],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"さざれ\",4110],"
    "[\"広瀬屋\",4149],"
    "[\"そばたいやき空\",4507]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d) && tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance(location, \"%s\")'",
        grn_test_location_string(tokyo_tocho_latitude, tokyo_tocho_longitude),
        distance,
        grn_test_location_string(tokyo_tocho_latitude, tokyo_tocho_longitude))));
}

void
test_but_white(void)
{
  gdouble asagaya_latitude = 35.70452;
  gdouble asagaya_longitude = 139.6351;
  gint distance = 10 * 1000;

  cut_assert_equal_string(
    "[[[5],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"吾妻屋\",2385],"
    "[\"そばたいやき空\",5592],"
    "[\"たい焼き / たつみや\",7148],"
    "[\"さざれ\",7671],"
    "[\"広瀬屋\",7830]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter '" \
        "geo_in_circle(location, \"%s\", %d) && " \
        "tags @ \"たいやき\" &! tags @ \"白\"' "
        "--scorer '_score=geo_distance(location, \"%s\")'",
        grn_test_location_string(asagaya_latitude, asagaya_longitude),
        distance,
        grn_test_location_string(asagaya_latitude, asagaya_longitude))));
}

void
test_drilldown(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 10 * 1000;

  cut_assert_equal_string(
    "[[[13],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"たいやき神田達磨 八重洲店\",1079],"
    "[\"たい焼き鉄次 大丸東京店\",1390],"
    "[\"築地 さのきや\",1723],"
    "[\"にしみや 甘味処\",2000],"
    "[\"しげ田\",2272],"
    "[\"柳屋 たい焼き\",3686],"
    "[\"根津のたいやき\",7812],"
    "[\"尾長屋 錦糸町店\",8314],"
    "[\"横浜 くりこ庵 浅草店\",8394],"
    "[\"たいやきひいらぎ\",9242]],\n"
    "[[6],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"おでん\",\"\",1],"
     "[\"たいやき\",\"\",13],"
     "[\"マグロ\",\"\",1],"
     "[\"和菓子\",\"\",1],"
     "[\"天然\",\"\",3],"
     "[\"白\",\"\",1]],\n"
    "[[2],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"category0001\",\"和食\",1],"
     "[\"category0003\",\"おやつ\",1]],\n"
    "[[3],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"area0002\",\"東京都中央区\",3],"
     "[\"area0005\",\"東京都文京区\",1],"
     "[\"area0013\",\"東京都渋谷区\",1]]"
     "]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d) && tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance(location, \"%s\")' "
        "--drilldown 'tags categories area' "
        "--drilldown_output_columns '_key, name, _nsubrecs' "
        "--drilldown_sortby '_key'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_drilldown_with_broken_reference(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 10 * 1000;

  assert_send_commands("delete Areas area0002");
  assert_send_commands("delete Areas area0005");
  cut_assert_equal_string(
    "[[[13],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"たいやき神田達磨 八重洲店\",1079],"
    "[\"たい焼き鉄次 大丸東京店\",1390],"
    "[\"築地 さのきや\",1723],"
    "[\"にしみや 甘味処\",2000],"
    "[\"しげ田\",2272],"
    "[\"柳屋 たい焼き\",3686],"
    "[\"根津のたいやき\",7812],"
    "[\"尾長屋 錦糸町店\",8314],"
    "[\"横浜 くりこ庵 浅草店\",8394],"
    "[\"たいやきひいらぎ\",9242]],\n"
    "[[1],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"area0013\",\"東京都渋谷区\",1]]"
     "]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d) && tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance(location, \"%s\")' "
        "--drilldown 'area' "
        "--drilldown_output_columns '_key, name, _nsubrecs' "
        "--drilldown_sortby '_key'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_weight_match(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 10 * 1000;

  cut_assert_equal_string(
    "[[[13],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"たいやき神田達磨 八重洲店\",9922],"
    "[\"たい焼き鉄次 大丸東京店\",8611],"
    "[\"築地 さのきや\",8278],"
    "[\"にしみや 甘味処\",8001],"
    "[\"しげ田\",7729],"
    "[\"柳屋 たい焼き\",6315],"
    "[\"たいやきひいらぎ\",1759],"
    "[\"尾長屋 錦糸町店\",1687],"
    "[\"横浜 くりこ庵 浅草店\",1607],"
    "[\"たい焼き写楽\",651]],\n"
    "[[6],"
    "[[\"_key\",\"ShortText\"],[\"_nsubrecs\",\"Int32\"]],"
    "[\"たいやき\",13],"
    "[\"天然\",3],"
    "[\"白\",1],"
    "[\"マグロ\",1],"
    "[\"和菓子\",1],"
    "[\"おでん\",1]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sortby '-_score, +name' "
        "--output_columns 'name, _score' "
        "--match_columns 'name * 1000 || tags * 10000' "
        "--query たいやき "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer '_score -= geo_distance(location, \"%s\")' "
        "--drilldown_output_columns '_key, _nsubrecs' "
        "--drilldown_sortby '-_nsubrecs' "
        "--drilldown 'tags' ",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}
