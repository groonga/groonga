/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_default(void);

void
cut_setup(void)
{
  grn_set_default_encoding(GRN_ENC_DEFAULT);
}

void
cut_teardown(void)
{
  grn_set_default_encoding(GRN_ENC_DEFAULT);
}

void
test_default(void)
{
  grn_encoding default_encoding;

  default_encoding = grn_get_default_encoding();
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_set_default_encoding(999));
  grn_test_assert_equal_encoding(default_encoding,
                                 grn_get_default_encoding());

  grn_test_assert(grn_set_default_encoding(GRN_ENC_SJIS));
  grn_test_assert_equal_encoding(GRN_ENC_SJIS,
                                 grn_get_default_encoding());
}
