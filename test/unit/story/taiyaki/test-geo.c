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

#include <str.h>

void test_in_circle(void);

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

  assert_send_commands(cut_get_fixture_data_string("ddl.grn", NULL));
  assert_send_command(cut_get_fixture_data_string("shops.grn", NULL));
  cut_assert_equal_string(
    "[[[6],"
    "[[\"_key\",\"ShortText\"],[\"_score\",\"Int32\"]],"
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
        "--sortby '+_score, +_key' "
        "--output_columns '_key, _score' "
        "--filter 'geo_in_circle(location, \"%s\", %d)' "
        "--scorer '_score=geo_distance(location, \"%s\")",
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude),
        distance,
        grn_test_location_string(yurakucho_latitude, yurakucho_longitude))));
}
