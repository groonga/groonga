/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

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

#include <groonga.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void attributes_open_broken_utf8(void);
void test_open_broken_utf8(void);

static grn_ctx *context;
static grn_query *query;

void
cut_setup(void)
{
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, GRN_CTX_USE_QL);
  query = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    if (query)
      grn_obj_close(context, (grn_obj *)query);
    grn_ctx_fin(context);
    g_free(context);
  }
}

void
attributes_open_broken_utf8(void)
{
  cut_set_attributes("ML", "senna-dev:1070",
                     NULL);
}

void
test_open_broken_utf8(void)
{
  gchar utf8[] = "\"あ\"";
  GRN_CTX_SET_ENCODING(context, GRN_ENC_UTF8);
  query = grn_query_open(context, utf8, 2, GRN_SEL_OR, 10);
  cut_assert_not_null(query);
}
