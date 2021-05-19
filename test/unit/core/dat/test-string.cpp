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
#include <dat/string.hpp>

#include "test-string.hpp"

namespace test_dat_string
{
  void test_empty_string(void)
  {
    const grn::dat::String str;

    cut_assert_null(str.ptr());
    cppcut_assert_equal(grn::dat::UInt32(0), str.length());
  }

  void test_constructor_with_length(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str(str_buf, sizeof(str_buf) - 1);

    cppcut_assert_equal(static_cast<const void *>(str_buf), str.ptr());
    cppcut_assert_equal(static_cast<grn::dat::UInt32>(sizeof(str_buf) - 1),
                        str.length());
  }

  void test_constructor_without_length(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str(str_buf);

    cppcut_assert_equal(static_cast<const void *>(str_buf), str.ptr());
    cppcut_assert_equal(static_cast<grn::dat::UInt32>(sizeof(str_buf) - 1),
                        str.length());
  }

  void test_copy_constructor(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str_origin(str_buf);
    const grn::dat::String str_copy(str_origin);

    cppcut_assert_equal(str_copy.ptr(), str_origin.ptr());
    cppcut_assert_equal(str_copy.length(), str_origin.length());
  }

  void test_index_access(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str(str_buf);

    for (grn::dat::UInt32 i = 0; i < str.length(); ++i) {
      cppcut_assert_equal(static_cast<grn::dat::UInt8>(str_buf[i]), str[i]);
    }
  }

  void test_assign(void)
  {
    const char str_buf[] = "日本語";

    grn::dat::String str;
    str.assign(str_buf, sizeof(str_buf) - 1);

    cppcut_assert_equal(static_cast<const void *>(str_buf), str.ptr());
    cppcut_assert_equal(static_cast<grn::dat::UInt32>(sizeof(str_buf) - 1),
                        str.length());
  }

  void test_substr(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(grn::dat::String("le"), str.substr(3));
    cppcut_assert_equal(grn::dat::String("app"), str.substr(0, 3));
    cppcut_assert_equal(grn::dat::String("ppl"), str.substr(1, 3));
  }

  void test_compare(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(0, str.compare(grn::dat::String("apple")));
    cppcut_assert_operator(str.compare(grn::dat::String("appl")), >, 0);
    cppcut_assert_operator(str.compare(grn::dat::String("appleX")), <, 0);
    cppcut_assert_operator(str.compare(grn::dat::String("banana")), <, 0);
    cppcut_assert_operator(str.compare(grn::dat::String("and")), >, 0);
  }

  void test_starts_with(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(true, str.starts_with(grn::dat::String("")));
    cppcut_assert_equal(true, str.starts_with(grn::dat::String("app")));
    cppcut_assert_equal(true, str.starts_with(grn::dat::String("apple")));
    cppcut_assert_equal(false, str.starts_with(grn::dat::String("X")));
  }

  void test_ends_with(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(true, str.ends_with(grn::dat::String("")));
    cppcut_assert_equal(true, str.ends_with(grn::dat::String("ple")));
    cppcut_assert_equal(true, str.ends_with(grn::dat::String("apple")));
    cppcut_assert_equal(false, str.ends_with(grn::dat::String("X")));
  }
}
