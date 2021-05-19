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
#include <dat/key.hpp>

#include <cstring>
#include <iostream>

#include "test-string.hpp"

namespace test_dat_key
{
  void test_size_estimation(void)
  {
    cppcut_assert_equal(grn::dat::UInt32(2), grn::dat::Key::estimate_size(0));
    cppcut_assert_equal(grn::dat::UInt32(2), grn::dat::Key::estimate_size(3));

    cppcut_assert_equal(grn::dat::UInt32(3), grn::dat::Key::estimate_size(4));
    cppcut_assert_equal(grn::dat::UInt32(3), grn::dat::Key::estimate_size(7));
  }

  void test_invalid_key(void)
  {
    const grn::dat::Key &key = grn::dat::Key::invalid_key();

    cppcut_assert_equal(grn::dat::INVALID_KEY_ID, key.id());
    cppcut_assert_equal(grn::dat::UInt32(0), key.length());
    cppcut_assert_not_null(key.ptr());
  }

  void test_creation(void)
  {
    grn::dat::UInt32 buf[16];
    const grn::dat::Key &key = grn::dat::Key::create(buf, 123, "groonga", 7);

    cppcut_assert_equal(grn::dat::String("groonga"), key.str());
    cppcut_assert_equal(grn::dat::UInt32(123), key.id());
    cppcut_assert_equal(grn::dat::UInt32(7), key.length());
    cppcut_assert_equal(0, std::memcmp(key.ptr(), "groonga", 7));
    cppcut_assert_equal(reinterpret_cast<const char *>(buf) + 5,
                        static_cast<const char *>(key.ptr()));
  }
}
