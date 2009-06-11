/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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

void test_domain(void);
void test_range(void);

static grn_ctx *context;
static grn_obj *database;

void
cut_setup(void)
{
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);
  database = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }
}

void
test_domain(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_nil(database->header.domain);
}

void
test_range(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_nil(grn_obj_get_range(context, database));
}

void
test_cursor(void)
{
  grn_table_cursor *c;
  database = grn_db_create(context, NULL, NULL);
  c = grn_table_cursor_open(context, database, NULL, 0, NULL, 0, 0);
  cut_assert_true(grn_table_cursor_next(context, c));
  grn_table_cursor_close(context, c);
}
