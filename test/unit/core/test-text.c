/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright(C) 2009  Brazil
  Copyright(C) 2011  Kouhei Sutou <kou@clear-code.com>

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

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_time2rfc1123(void);
void test_atoi_padded(void);
void test_urldec(void);

static grn_ctx context;

void
cut_startup(void)
{
}

void
cut_shutdown(void)
{
}

void
cut_setup(void)
{
  grn_ctx_init(&context, 0);
}

void
cut_teardown(void)
{
  grn_ctx_fin(&context);
}

void
test_time2rfc1123(void)
{
  grn_obj rfc1123;
  const gchar *dupped_rfc1123;

  GRN_TEXT_INIT(&rfc1123, 0);
  grn_text_time2rfc1123(&context, &rfc1123, 1243433233);
  dupped_rfc1123 = cut_take_strndup(GRN_TEXT_VALUE(&rfc1123),
                                    GRN_TEXT_LEN(&rfc1123));
  grn_obj_unlink(&context, &rfc1123);
  cut_assert_equal_string("Wed, 27 May 2009 14:07:13 GMT", dupped_rfc1123);
}

void
test_atoi_padded(void)
{
  grn_obj t;
  GRN_TEXT_INIT(&t, 0);
  grn_text_itoa_padded(&context, &t, 543, '*', 5);
  cut_assert_equal_memory("**543", 5, GRN_TEXT_VALUE(&t), GRN_TEXT_LEN(&t));

  GRN_BULK_REWIND(&t);
  grn_text_itoa_padded(&context, &t, 0, '-', 5);
  cut_assert_equal_memory("----0", 5, GRN_TEXT_VALUE(&t), GRN_TEXT_LEN(&t));

  GRN_BULK_REWIND(&t);
  grn_text_itoa_padded(&context, &t, -123, ' ', 5);
  cut_assert_equal_memory("- 123", 5, GRN_TEXT_VALUE(&t), GRN_TEXT_LEN(&t));

  GRN_BULK_REWIND(&t);
  grn_text_itoa_padded(&context, &t, 123, ' ', 0);
  cut_assert_equal_memory("", 0, GRN_TEXT_VALUE(&t), GRN_TEXT_LEN(&t));
}

void
test_urldec(void)
{
  grn_obj t;
  const char *test_str1 = "/+test%20/u_hihi%00desu?yo-da:test";
  GRN_TEXT_INIT(&t, 0);
  grn_text_urldec(&context, &t, test_str1, test_str1 + strlen(test_str1), ':');
  cut_assert_equal_memory("/+test /u_hihi\0desu?yo-da", 25, GRN_TEXT_VALUE(&t), GRN_TEXT_LEN(&t));
}
