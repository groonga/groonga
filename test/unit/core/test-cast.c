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

#include <db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define OBJECT(name) (grn_ctx_get(&context, (name), strlen(name)))

void test_text(void);

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj src, dest;

void
cut_setup(void)
{
  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);
  GRN_VOID_INIT(&src);
  GRN_VOID_INIT(&dest);
}

void
cut_teardown(void)
{
  grn_obj_remove(&context, &src);
  grn_obj_remove(&context, &dest);
  grn_ctx_fin(&context);
  teardown_grn_logger(logger);
}

void
test_text(void)
{
  grn_obj_reinit(&context, &src, GRN_DB_TEXT, 0);
  grn_obj_reinit(&context, &dest, GRN_DB_INT32, 0);

  GRN_TEXT_PUTS(&context, &src, "29");
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
  cut_assert_equal_int(29, GRN_INT32_VALUE(&dest));
}
