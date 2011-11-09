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
#include <dat/string.hpp>

namespace test_dat_string
{
  void test_empty_string(void)
  {
    const grn::dat::String str;

    cppcut_assert_equal(str.ptr(), static_cast<const void *>(NULL));
    cppcut_assert_equal(str.length(), static_cast<grn::dat::UInt32>(0));
  }

  void test_constructor_with_length(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str(str_buf, sizeof(str_buf) - 1);

    cppcut_assert_equal(str.ptr(), static_cast<const void *>(str_buf));
    cppcut_assert_equal(str.length(), static_cast<grn::dat::UInt32>(sizeof(str_buf) - 1));
  }

  void test_constructor_without_length(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str(str_buf);

    cppcut_assert_equal(str.ptr(), static_cast<const void *>(str_buf));
    cppcut_assert_equal(str.length(), static_cast<grn::dat::UInt32>(sizeof(str_buf) - 1));
  }

  void test_copy_constructor(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str_origin(str_buf);
    const grn::dat::String str_copy(str_origin);

    cppcut_assert_equal(str_origin.ptr(), str_copy.ptr());
    cppcut_assert_equal(str_origin.length(), str_copy.length());
  }

  void test_index_access(void)
  {
    const char str_buf[] = "日本語";
    const grn::dat::String str(str_buf);

    for (grn::dat::UInt32 i = 0; i < str.length(); ++i) {
      cppcut_assert_equal(str[i], static_cast<grn::dat::UInt8>(str_buf[i]));
    }
  }

  void test_assign(void)
  {
    const char str_buf[] = "日本語";

    grn::dat::String str;
    str.assign(str_buf, sizeof(str_buf) - 1);

    cppcut_assert_equal(str.ptr(), static_cast<const void *>(str_buf));
    cppcut_assert_equal(str.length(), static_cast<grn::dat::UInt32>(sizeof(str_buf) - 1));
  }

  void test_substr(void)
  {
    const grn::dat::String str("apple");

    cut_assert(str.substr(3) == grn::dat::String("le"));
    cut_assert(str.substr(0, 3) == grn::dat::String("app"));
    cut_assert(str.substr(1, 3) == grn::dat::String("ppl"));
  }

  void test_compare(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(str.compare(grn::dat::String("apple")), 0);
    cut_assert(str.compare(grn::dat::String("appl")) > 0);
    cut_assert(str.compare(grn::dat::String("appleX")) < 0);
    cut_assert(str.compare(grn::dat::String("banana")) < 0);
    cut_assert(str.compare(grn::dat::String("and")) > 0);
  }

  void test_starts_with(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(str.starts_with(grn::dat::String("")), true);
    cppcut_assert_equal(str.starts_with(grn::dat::String("app")), true);
    cppcut_assert_equal(str.starts_with(grn::dat::String("apple")), true);
    cppcut_assert_equal(str.starts_with(grn::dat::String("X")), false);
  }

  void test_ends_with(void)
  {
    const grn::dat::String str("apple");

    cppcut_assert_equal(str.ends_with(grn::dat::String("")), true);
    cppcut_assert_equal(str.ends_with(grn::dat::String("ple")), true);
    cppcut_assert_equal(str.ends_with(grn::dat::String("apple")), true);
    cppcut_assert_equal(str.ends_with(grn::dat::String("X")), false);
  }
}
