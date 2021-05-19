/*
  Copyright(C) 2009  Brazil
  Copyright(C) 2011-2014  Kouhei Sutou <kou@clear-code.com>

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

#include "grn_str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_time2rfc1123(void);
void test_atoi_padded(void);
void test_urldec(void);

static grn_ctx context;
static grn_obj buffer;

void
cut_setup(void)
{
  grn_ctx_init(&context, 0);
  GRN_TEXT_INIT(&buffer, 0);
}

void
cut_teardown(void)
{
  GRN_OBJ_FIN(&context, &buffer);
  grn_ctx_fin(&context);
}

void
test_time2rfc1123(void)
{
  const char *expected = "Wed, 27 May 2009 14:07:13 GMT";

  grn_text_time2rfc1123(&context, &buffer, 1243433233);
  cut_assert_equal_memory(expected,
                          strlen(expected),
                          GRN_TEXT_VALUE(&buffer),
                          GRN_TEXT_LEN(&buffer));
}

void
test_atoi_padded(void)
{
  grn_text_itoa_padded(&context, &buffer, 543, '*', 5);
  cut_assert_equal_memory("**543",
                          5,
                          GRN_TEXT_VALUE(&buffer),
                          GRN_TEXT_LEN(&buffer));

  GRN_BULK_REWIND(&buffer);
  grn_text_itoa_padded(&context, &buffer, 0, '-', 5);
  cut_assert_equal_memory("----0",
                          5,
                          GRN_TEXT_VALUE(&buffer),
                          GRN_TEXT_LEN(&buffer));

  GRN_BULK_REWIND(&buffer);
  grn_text_itoa_padded(&context, &buffer, -123, ' ', 5);
  cut_assert_equal_memory("- 123",
                          5,
                          GRN_TEXT_VALUE(&buffer),
                          GRN_TEXT_LEN(&buffer));

  GRN_BULK_REWIND(&buffer);
  grn_text_itoa_padded(&context, &buffer, 123, ' ', 0);
  cut_assert_equal_memory("",
                          0,
                          GRN_TEXT_VALUE(&buffer),
                          GRN_TEXT_LEN(&buffer));
}

void
test_urldec(void)
{
  const gchar *url = "/+test%20/u_hihi%00desu?yo-da:test";
  const gchar decoded_url[] = "/+test /u_hihi\0desu?yo-da";

  grn_text_urldec(&context,
                  &buffer,
                  url, url + strlen(url),
                  ':');
  cut_assert_equal_memory(decoded_url,
                          sizeof(decoded_url) - 1,
                          GRN_TEXT_VALUE(&buffer),
                          GRN_TEXT_LEN(&buffer));
}

void
test_printf_inplace_size(void)
{
  const char *inplace_size_string = "inplace size is <= 24  ";
  grn_text_printf(&context, &buffer, "%s", inplace_size_string);
  cut_assert_equal_memory(inplace_size_string,
                          strlen(inplace_size_string),
                          GRN_TEXT_VALUE(&buffer),
                          GRN_BULK_VSIZE(&buffer));
}

void
test_printf_outplace_size(void)
{
  const char *outplace_size_string = "outplace size is > 24   ";
  grn_text_printf(&context, &buffer, "%s", outplace_size_string);
  cut_assert_equal_memory(outplace_size_string,
                          strlen(outplace_size_string),
                          GRN_TEXT_VALUE(&buffer),
                          GRN_BULK_VSIZE(&buffer));
}
