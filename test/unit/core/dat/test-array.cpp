/*
  Copyright (C) 2011-2012  Brazil
  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>

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
#include <dat/array.hpp>

namespace test_dat_array
{
  void test_assign_with_length(void)
  {
    char buf[] = "This is a pen.";

    grn::dat::Array<char> array;

    array.assign(buf, sizeof(buf));
    cppcut_assert_equal(buf, array.ptr());
    cppcut_assert_equal(sizeof(buf), static_cast<size_t>(array.size()));

    for (std::size_t i = 0; i < sizeof(buf); ++i) {
      cppcut_assert_equal(buf[i], array[i]);
      cppcut_assert_equal(buf[i],
          static_cast<const grn::dat::Array<char> &>(array)[i]);
    }
    cppcut_assert_equal(buf, array.begin());
    cppcut_assert_equal(buf,
        static_cast<const grn::dat::Array<char> &>(array).begin());
    cppcut_assert_equal((buf + sizeof(buf)), array.end());
    cppcut_assert_equal((buf + sizeof(buf)),
        static_cast<const grn::dat::Array<char> &>(array).end());
  }

  void test_assign_without_length(void)
  {
    char buf[] = "This is a pen.";

    grn::dat::Array<char> array;

    array.assign(buf);
    cppcut_assert_equal(buf, array.ptr());
    cppcut_assert_equal(sizeof(buf), static_cast<size_t>(array.size()));
  }

  void test_copy(void)
  {
    char buf[] = "This is a pen.";

    grn::dat::Array<char> clone(buf);
    cppcut_assert_equal(buf, clone.ptr());
    cppcut_assert_equal(sizeof(buf), static_cast<size_t>(clone.size()));
  }

  void test_copy_with_length(void)
  {
    char buf[] = "This is a pen.";

    grn::dat::Array<char> clone(buf, sizeof(buf));
    cppcut_assert_equal(buf, clone.ptr());
    cppcut_assert_equal(sizeof(buf), static_cast<size_t>(clone.size()));
  }
}
