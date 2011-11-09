/* -*- c-basic-offset: 2; coding: utf-8 -*- */
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

    cppcut_assert_equal(check.is_offset(), false);
    cppcut_assert_equal(check.except_is_offset(), static_cast<grn::dat::UInt32>(0));
    cppcut_assert_equal(check.is_phantom(), false);
    cppcut_assert_equal(check.label(), static_cast<grn::dat::UInt32>(0));
    cppcut_assert_equal(check.child(), static_cast<grn::dat::UInt32>(0));
    cppcut_assert_equal(check.sibling(), static_cast<grn::dat::UInt32>(0));
  }

  void test_phantomize(void)
  {
    grn::dat::Check check;

    check.set_is_phantom(true);
    cppcut_assert_equal(check.is_phantom(), true);
    cppcut_assert_equal(check.next(), static_cast<grn::dat::UInt32>(0));
    cppcut_assert_equal(check.prev(), static_cast<grn::dat::UInt32>(0));

    check.set_next(101);
    check.set_prev(99);
    cppcut_assert_equal(check.next(), static_cast<grn::dat::UInt32>(101));
    cppcut_assert_equal(check.prev(), static_cast<grn::dat::UInt32>(99));
  }

  void test_unphantomize(void)
  {
    grn::dat::Check check;

    check.set_is_phantom(true);
    check.set_is_phantom(false);
    cppcut_assert_equal(check.is_phantom(), false);
    cppcut_assert_equal(check.child(), grn::dat::INVALID_LABEL);
    cppcut_assert_equal(check.sibling(), grn::dat::INVALID_LABEL);
  }

  void test_nonphantom(void)
  {
    grn::dat::Check check;

    check.set_is_offset(true);
    cppcut_assert_equal(check.is_offset(), true);

    check.set_label('a');
    cppcut_assert_equal(check.label(), static_cast<grn::dat::UInt32>('a'));

    check.set_child('b');
    cppcut_assert_equal(check.child(), static_cast<grn::dat::UInt32>('b'));

    check.set_sibling('c');
    cppcut_assert_equal(check.sibling(), static_cast<grn::dat::UInt32>('c'));

    cppcut_assert_equal(check.is_offset(), true);
    cppcut_assert_equal(check.except_is_offset(),
                        'a' | (static_cast<grn::dat::UInt32>('b' << 9)) | (static_cast<grn::dat::UInt32>('c' << 18)));
    cppcut_assert_equal(check.is_phantom(), false);
  }
}
