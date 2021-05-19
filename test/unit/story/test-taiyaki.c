/*
  Copyright (C) 2010-2015  Kouhei Sutou <kou@clear-code.com>

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

void test_in_circle(void);
void test_sort(void);
void test_filter_by_tag_and_sort_by_distance_from_tokyo_tocho(void);
void test_in_circle_and_tag(void);
void test_but_white(void);
void test_drilldown(void);
void test_drilldown_with_broken_reference(void);
void test_query_expansion(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "taiyaki",
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
test_in_circle(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[7],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"],"
    "[\"location\",\"WGS84GeoPoint\"]],"
    "[\"銀座 かずや\",280,\"128424629x503139222\"],"
    "[\"たい焼き鉄次 大丸東京店\",810,\"128451283x503166852\"],"
    "[\"たいやき神田達磨 八重洲店\",970,\"128453260x503174156\"],"
    "[\"にしみや 甘味処\",1056,\"128418570x503188661\"],"
    "[\"築地 さのきや\",1186,\"128397312x503174596\"],"
    "[\"しげ田\",1530,\"128421454x503208983\"],"
    "[\"柳屋 たい焼き\",2179,\"128467228x503222332\"]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys '+_score, +name' "
        "--output_columns 'name, _score, location' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer '_score=geo_distance(location, \"%s\")'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_sort(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[7],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"銀座 かずや\",280],"
    "[\"たい焼き鉄次 大丸東京店\",810],"
    "[\"たいやき神田達磨 八重洲店\",970],"
    "[\"にしみや 甘味処\",1056],"
    "[\"築地 さのきや\",1186],"
    "[\"しげ田\",1530],"
    "[\"柳屋 たい焼き\",2179]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys 'geo_distance(location, \"%s\")' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer '_score=geo_distance(location, \"%s\")'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_filter_by_tag_and_sort_by_distance_from_tokyo_tocho(void)
{
  gdouble tokyo_tocho_latitude = 35.689444;
  gdouble tokyo_tocho_longitude = 139.69166701;

  cut_assert_equal_string(
    "[[[31],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"さざれ\",2860],"
    "[\"広瀬屋\",2870],"
    "[\"そばたいやき空\",3004],"
    "[\"たい焼 カタオカ\",3347],"
    "[\"車\",3792],"
    "[\"吾妻屋\",4166],"
    "[\"代官山たい焼き黒鯛\",4497],"
    "[\"たいやきひいらぎ\",4965],"
    "[\"たいやき工房白家 阿佐ヶ谷店\",5102],"
    "[\"たいやき本舗 藤家 阿佐ヶ谷店\",5172]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance2(location, \"%s\")'",
        grn_test_location_string(tokyo_tocho_latitude, tokyo_tocho_longitude))));
}

void
test_in_circle_and_tag(void)
{
  gdouble tokyo_tocho_latitude = 35.689444;
  gdouble tokyo_tocho_longitude = 139.691667;
  gint distance = 4 * 1000;

  cut_assert_equal_string(
    "[[[5],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"さざれ\",2860],"
    "[\"広瀬屋\",2871],"
    "[\"そばたいやき空\",3016],"
    "[\"たい焼 カタオカ\",3353],"
    "[\"車\",3794]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d) && tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance3(location, \"%s\")'",
        grn_test_location_string(tokyo_tocho_latitude, tokyo_tocho_longitude),
        distance,
        grn_test_location_string(tokyo_tocho_latitude, tokyo_tocho_longitude))));
}

void
test_but_white(void)
{
  gdouble asagaya_latitude = 35.70452;
  gdouble asagaya_longitude = 139.6351;
  gint distance = 5 * 1000;

  cut_assert_equal_string(
    "[[[5],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"吾妻屋\",1198],"
    "[\"そばたいやき空\",3162],"
    "[\"たい焼き / たつみや\",4341],"
    "[\"さざれ\",4637],"
    "[\"広瀬屋\",4692]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys '+_score, +name' "
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
    "[[[23],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"たい焼き鉄次 大丸東京店\",810],"
    "[\"たいやき神田達磨 八重洲店\",970],"
    "[\"にしみや 甘味処\",1056],"
    "[\"築地 さのきや\",1186],"
    "[\"しげ田\",1530],"
    "[\"柳屋 たい焼き\",2179],"
    "[\"尾長屋 錦糸町店\",5007],"
    "[\"根津のたいやき\",5036],"
    "[\"横浜 くりこ庵 浅草店\",5098],"
    "[\"たい焼き写楽\",5457]],"
    "[[7],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"おでん\",\"\",1],"
     "[\"たいやき\",\"\",23],"
     "[\"カレー\",\"\",1],"
     "[\"マグロ\",\"\",1],"
     "[\"和菓子\",\"\",1],"
     "[\"天然\",\"\",4],"
     "[\"白\",\"\",1]],"
    "[[2],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"category0001\",\"和食\",1],"
     "[\"category0003\",\"おやつ\",1]],"
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
        "--sort_keys '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d) && tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance2(location, \"%s\")' "
        "--drilldown 'tags categories area' "
        "--drilldown_output_columns '_key, name, _nsubrecs' "
        "--drilldown_sort_keys '_key'",
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
    "[[[23],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"たい焼き鉄次 大丸東京店\",811],"
    "[\"たいやき神田達磨 八重洲店\",972],"
    "[\"にしみや 甘味処\",1060],"
    "[\"築地 さのきや\",1187],"
    "[\"しげ田\",1537],"
    "[\"柳屋 たい焼き\",2186],"
    "[\"尾長屋 錦糸町店\",5024],"
    "[\"根津のたいやき\",5036],"
    "[\"横浜 くりこ庵 浅草店\",5106],"
    "[\"たい焼き写楽\",5464]],"
    "[[1],"
     "[[\"_key\",\"ShortText\"],"
      "[\"name\",\"ShortText\"],"
      "[\"_nsubrecs\",\"Int32\"]],"
     "[\"area0013\",\"東京都渋谷区\",1]]"
     "]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys '+_score, +name' "
        "--output_columns 'name, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d) && tags @ \"たいやき\"' "
        "--scorer '_score=geo_distance3(location, \"%s\")' "
        "--drilldown 'area' "
        "--drilldown_output_columns '_key, name, _nsubrecs' "
        "--drilldown_sort_keys '_key'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_multi_geo_in_circle(void)
{
  gdouble yurakucho_latitude = 35.67487;
  gdouble yurakucho_longitude = 139.76352;
  gint distance = 3 * 1000;

  cut_assert_equal_string(
    "[[[1],"
    "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
    "[\"たいやき神田達磨 八重洲店\",10032]"
    "]]",
    send_command(
      cut_take_printf(
        "select Shops "
        "--sort_keys '-_score, +name' "
        "--output_columns 'name, _score' "
        "--match_columns 'name * 1000 || tags * 10000' "
        "--query たいやき "
        "--filter '"
        "  (geo_in_circle(location1, \"%s\", %d) ||"
        "   geo_in_circle(location2, \"%s\", %d) ||"
        "   geo_in_circle(location3, \"%s\", %d))' "
        "--scorer '_score = _score - geo_distance(location, \"%s\")'",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}

void
test_query_expansion(void)
{
  cut_assert_equal_string(
    "[[[16],"
    "[[\"name\",\"ShortText\"]],"
    "[\"おめで鯛焼き本舗錦糸町東急店\"],"
    "[\"そばたいやき空\"],"
    "[\"たいやきひいらぎ\"]"
    "]]",
    send_command(
      "select Shops "
      "--sort_keys '+name' "
      "--output_columns 'name' "
      "--limit 3 "
      "--match_columns name "
      "--query たいやき "
      "--query_expansion Synonyms.words"));
}
