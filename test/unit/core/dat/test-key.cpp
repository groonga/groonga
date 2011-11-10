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
#include <dat/key.hpp>

#include <cstring>
#include <iostream>

namespace cut
{
  std::ostream &operator<<(std::ostream &stream, const grn::dat::String &str)
  {
    return stream.write(static_cast<const char *>(str.ptr()), str.length());
  }
}

namespace test_dat_key
{
  void test_size_estimation(void)
  {
    cppcut_assert_equal(grn::dat::Key::estimate_size(0),
                        static_cast<grn::dat::UInt32>(2));
    cppcut_assert_equal(grn::dat::Key::estimate_size(3),
                        static_cast<grn::dat::UInt32>(2));

    cppcut_assert_equal(grn::dat::Key::estimate_size(4),
                        static_cast<grn::dat::UInt32>(3));
    cppcut_assert_equal(grn::dat::Key::estimate_size(7),
                        static_cast<grn::dat::UInt32>(3));
  }

  void test_invalid_key(void)
  {
    const grn::dat::Key &key = grn::dat::Key::invalid_key();

    cppcut_assert_equal(key.id(), grn::dat::INVALID_KEY_ID);
    cppcut_assert_equal(key.length(), static_cast<grn::dat::UInt32>(0));
    cut_assert(key.ptr() != static_cast<const void *>(NULL));
  }

  void test_creation(void)
  {
    grn::dat::UInt32 buf[16];
    const grn::dat::Key &key = grn::dat::Key::create(buf, 123, "groonga", 7);

    cppcut_assert_equal(key.str(), grn::dat::String("groonga"));
    cppcut_assert_equal(key.id(), static_cast<grn::dat::UInt32>(123));
    cppcut_assert_equal(key.length(), static_cast<grn::dat::UInt32>(7));
    cppcut_assert_equal(std::memcmp(key.ptr(), "groonga", 7), 0);
    cppcut_assert_equal(key.ptr(), static_cast<const void *>(reinterpret_cast<char *>(buf) + 5));
  }
}
