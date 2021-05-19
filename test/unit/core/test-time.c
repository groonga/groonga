/*
  Copyright (C) 2016  Kouhei Sutou <kou@clear-code.com>

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

#include "../../../config.h"

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_to_tm(void);
void test_from_tm(void);

static grn_ctx *context;

void
cut_setup(void)
{
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);
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
test_to_tm(void)
{
  int64_t time;
  struct tm tm;

  time = GRN_TIME_PACK(1462453129, 997984);
  cut_assert_true(grn_time_to_tm(context, time, &tm));
  cut_assert_equal_string("2016-05-05T21:58:49",
                          cut_take_printf("%04d-%02d-%02dT%02d:%02d:%02d",
                                          1900 + tm.tm_year,
                                          tm.tm_mon + 1,
                                          tm.tm_mday,
                                          tm.tm_hour,
                                          tm.tm_min,
                                          tm.tm_sec));
}

void
test_from_tm(void)
{
  int64_t time;
  struct tm tm;

  tm.tm_year = 116;
  tm.tm_mon = 4;
  tm.tm_mday = 5;
  tm.tm_hour = 21;
  tm.tm_min = 58;
  tm.tm_sec = 49;
  tm.tm_isdst = -1;

  cut_assert_true(grn_time_from_tm(context, &time, &tm));
  gcut_assert_equal_int64(1462453129000000,
                          time);
}
