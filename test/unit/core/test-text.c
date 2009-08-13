/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/* Copyright(C) 2009 Brazil

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
  grn_obj t;
  GRN_TEXT_INIT(&t, 0);
  grn_text_time2rfc1123(&context, &t, 1243433233);
  cut_assert_equal_memory("Wed, 27 May 2009 14:07:13 GMT", 29, GRN_TEXT_VALUE(&t), GRN_TEXT_LEN(&t));
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
