/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

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

#include "../lib/sen-assertions.h"

void attributes_open_broken_utf8(void);
void test_open_broken_utf8(void);

static sen_query *query;

void
setup(void)
{
  query = NULL;
}

void
teardown(void)
{
  if (query)
    sen_query_close(query);
}

void
attributes_open_broken_utf8(void)
{
  cut_set_attributes("ML", "senna-dev:1070");
}

void
test_open_broken_utf8(void)
{
  gchar utf8[] = "\"あ\"";
  query = sen_query_open(utf8, 2, sen_sel_or, 10, sen_enc_utf8);
  cut_assert_not_null(query);
}
