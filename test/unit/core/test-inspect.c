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
#include <util.h>

void test_null(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;
static grn_obj *inspected;


void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "inspect",
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

  inspected = NULL;

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
  if (inspected) {
    grn_obj_close(context, inspected);
  }

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_null(void)
{
  inspected = grn_inspect(context, NULL, NULL);
  cut_assert_equal_string("(NULL)",
                          cut_take_printf("%.*s",
                                          (int)GRN_TEXT_LEN(inspected),
                                          GRN_TEXT_VALUE(inspected)));
}
