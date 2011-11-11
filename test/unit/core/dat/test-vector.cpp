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
#include <dat/vector.hpp>

namespace
{
  class Counter {
   public:
    Counter() {
      ++constructor_count;
    }
    Counter(const Counter &) {
      ++copy_count;
    }
    ~Counter() {
      ++destructor_count;
    }

    static int constructor_count;
    static int copy_count;
    static int destructor_count;
  };

  int Counter::constructor_count = 0;
  int Counter::copy_count = 0;
  int Counter::destructor_count = 0;
}

namespace test_dat_vector
{
  void test_empty_vector(void)
  {
    const grn::dat::Vector<grn::dat::UInt32> vec;

    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(0));
  }

  void test_reserve(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.reserve(1);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(1));

    vec.reserve(2);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(2));

    vec.reserve(3);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(4));

    vec.reserve(100);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(100));

    vec.reserve(101);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(200));

    vec.reserve(0);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(200));
  }

  void test_resize(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.resize(1);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(1));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(1));

    vec.resize(2);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(2));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(2));

    vec.resize(3);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(3));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(4));

    vec.resize(100);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(100));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(100));

    vec.resize(101);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(101));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(200));

    vec.resize(0);
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(200));
  }

  void test_push_pop(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.push_back();
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(1));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(1));

    vec.pop_back();
    cppcut_assert_equal(vec.empty(), true);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(0));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(1));

    vec.push_back(5);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(1));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(1));

    cppcut_assert_equal(vec.front(), grn::dat::UInt32(5));
    cppcut_assert_equal(vec.back(), grn::dat::UInt32(5));

    vec.push_back(123);
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(2));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(2));

    cppcut_assert_equal(vec.front(), grn::dat::UInt32(5));
    cppcut_assert_equal(vec.back(), grn::dat::UInt32(123));

    vec.pop_back();
    cppcut_assert_equal(vec.empty(), false);
    cppcut_assert_equal(vec.size(), grn::dat::UInt32(1));
    cppcut_assert_equal(vec.capacity(), grn::dat::UInt32(2));

    cppcut_assert_equal(vec.front(), grn::dat::UInt32(5));
    cppcut_assert_equal(vec.back(), grn::dat::UInt32(5));

    vec.clear();

    for (grn::dat::UInt32 i = 0; i < 1000; ++i) {
      vec.push_back(i);
      cppcut_assert_equal(vec.back(), i);
      cppcut_assert_equal(vec.size(), i + 1);
    }
    for (grn::dat::UInt32 i = 0; i < 1000; ++i) {
      cppcut_assert_equal(vec.size(), 1000 - i);
      cppcut_assert_equal(vec.back(), 999 - i);
      vec.pop_back();
    }
  }

  void test_index_access(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.resize(100);
    for (grn::dat::UInt32 i = 0; i < vec.size(); ++i) {
      vec[i] = i;
    }
    for (grn::dat::UInt32 i = 0; i < vec.size(); ++i) {
      cppcut_assert_equal(vec[i], i);
      cppcut_assert_equal(const_cast<const grn::dat::Vector<grn::dat::UInt32> &>(vec)[i], i);
      cppcut_assert_equal(&vec[i], vec.begin() + i);
      cppcut_assert_equal(&vec[i], vec.end() - vec.size() + i);
    }
  }

  void test_object_management(void)
  {
    grn::dat::Vector<Counter> vec;

    cppcut_assert_equal(Counter::constructor_count, 0);
    cppcut_assert_equal(Counter::copy_count, 0);
    cppcut_assert_equal(Counter::destructor_count, 0);

    vec.push_back();

    cppcut_assert_equal(Counter::constructor_count, 1);
    cppcut_assert_equal(Counter::copy_count, 0);
    cppcut_assert_equal(Counter::destructor_count, 0);

    vec.pop_back();

    cppcut_assert_equal(Counter::constructor_count, 1);
    cppcut_assert_equal(Counter::copy_count, 0);
    cppcut_assert_equal(Counter::destructor_count, 1);

    vec.resize(10);

    cppcut_assert_equal(Counter::constructor_count, 11);
    cppcut_assert_equal(Counter::copy_count, 0);
    cppcut_assert_equal(Counter::destructor_count, 1);

    vec.pop_back();

    cppcut_assert_equal(Counter::constructor_count, 11);
    cppcut_assert_equal(Counter::copy_count, 0);
    cppcut_assert_equal(Counter::destructor_count, 2);

    vec.resize(11);

    cppcut_assert_equal(Counter::constructor_count, 13);
    cppcut_assert_equal(Counter::copy_count, 9);
    cppcut_assert_equal(Counter::destructor_count, 11);

    vec.clear();

    cppcut_assert_equal(Counter::constructor_count, 13);
    cppcut_assert_equal(Counter::copy_count, 9);
    cppcut_assert_equal(Counter::destructor_count, 22);
  }
}
