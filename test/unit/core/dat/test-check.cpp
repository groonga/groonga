/*
  Copyright (C) 2011-2012  Brazil

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
#include <dat/check.hpp>

namespace test_dat_check
{
  void test_initial_values(void)
  {
    const grn::dat::Check check;

    cppcut_assert_equal(false, check.is_offset());
    cppcut_assert_equal(grn::dat::UInt32(0), check.except_is_offset());
    cppcut_assert_equal(false, check.is_phantom());
    cppcut_assert_equal(grn::dat::UInt32(0), check.label());
    cppcut_assert_equal(grn::dat::UInt32(0), check.child());
    cppcut_assert_equal(grn::dat::UInt32(0), check.sibling());
  }

  void test_phantomize(void)
  {
    grn::dat::Check check;

    check.set_is_phantom(true);
    cppcut_assert_equal(true, check.is_phantom());
    cppcut_assert_equal(grn::dat::UInt32(0), check.next());
    cppcut_assert_equal(grn::dat::UInt32(0), check.prev());

    check.set_next(101);
    check.set_prev(99);
    cppcut_assert_equal(grn::dat::UInt32(101), check.next());
    cppcut_assert_equal(grn::dat::UInt32(99), check.prev());
  }

  void test_unphantomize(void)
  {
    grn::dat::Check check;

    check.set_is_phantom(true);
    check.set_is_phantom(false);
    cppcut_assert_equal(false, check.is_phantom());
    cppcut_assert_equal(grn::dat::INVALID_LABEL, check.child());
    cppcut_assert_equal(grn::dat::INVALID_LABEL, check.sibling());
  }

  void test_nonphantom(void)
  {
    grn::dat::Check check;

    check.set_is_offset(true);
    cppcut_assert_equal(true, check.is_offset());

    check.set_label('a');
    cppcut_assert_equal(grn::dat::UInt32('a'), check.label());

    check.set_child('b');
    cppcut_assert_equal(grn::dat::UInt32('b'), check.child());

    check.set_sibling('c');
    cppcut_assert_equal(grn::dat::UInt32('c'), check.sibling());

    cppcut_assert_equal(true, check.is_offset());
    const grn::dat::UInt32 expected_except_is_offset =
        'a' | grn::dat::UInt32('b' << 9) | grn::dat::UInt32('c' << 18);
    cppcut_assert_equal(expected_except_is_offset, check.except_is_offset());
    cppcut_assert_equal(false, check.is_phantom());
  }
}
