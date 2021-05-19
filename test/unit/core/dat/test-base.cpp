/*
  Copyright (C) 2011  Brazil

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

#include <gcutter.h>
#include <cppcutter.h>

#include <grn-assertions.h>
#include <dat/base.hpp>

namespace test_dat_base
{
  void test_initial_values(void)
  {
    grn::dat::Base base;

    cppcut_assert_equal(false, base.is_linker());
    cppcut_assert_equal(grn::dat::UInt32(0), base.offset());
  }

  void test_linker(void)
  {
    grn::dat::Base base;

    base.set_key_pos(100);
    cppcut_assert_equal(true, base.is_linker());
    cppcut_assert_equal(grn::dat::UInt32(100), base.key_pos());
  }

  void test_nonlinker(void)
  {
    grn::dat::Base base;

    base.set_offset(1000);
    cppcut_assert_equal(false, base.is_linker());
    cppcut_assert_equal(grn::dat::UInt32(1000), base.offset());
  }
}
