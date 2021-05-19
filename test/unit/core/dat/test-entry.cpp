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
#include <dat/entry.hpp>

namespace test_dat_entry
{
  void test_initial_values(void)
  {
    const grn::dat::Entry entry;

    cppcut_assert_equal(false, entry.is_valid());
    cppcut_assert_equal(grn::dat::UInt32(0), entry.next());
  }

  void test_valid_entry(void)
  {
    grn::dat::Entry entry;

    entry.set_key_pos(100);
    cppcut_assert_equal(true, entry.is_valid());
    cppcut_assert_equal(grn::dat::UInt32(100), entry.key_pos());
  }

  void test_invalid_entry(void)
  {
    grn::dat::Entry entry;

    entry.set_next(200);
    cppcut_assert_equal(false, entry.is_valid());
    cppcut_assert_equal(grn::dat::UInt32(200), entry.next());
  }
}
