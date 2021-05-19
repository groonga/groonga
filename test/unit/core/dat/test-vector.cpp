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

    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.capacity());
  }

  void test_reserve(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.reserve(1);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.capacity());

    vec.reserve(2);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(2), vec.capacity());

    vec.reserve(3);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(4), vec.capacity());

    vec.reserve(100);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(100), vec.capacity());

    vec.reserve(101);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(200), vec.capacity());

    vec.reserve(0);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(200), vec.capacity());
  }

  void test_resize(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.resize(1);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.capacity());

    vec.resize(2);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(2), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(2), vec.capacity());

    vec.resize(3);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(3), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(4), vec.capacity());

    vec.resize(100);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(100), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(100), vec.capacity());

    vec.resize(101);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(101), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(200), vec.capacity());

    vec.resize(0);
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(200), vec.capacity());
  }

  void test_push_pop(void)
  {
    grn::dat::Vector<grn::dat::UInt32> vec;

    vec.push_back();
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.capacity());

    vec.pop_back();
    cppcut_assert_equal(true, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(0), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.capacity());

    vec.push_back(5);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.capacity());

    cppcut_assert_equal(grn::dat::UInt32(5), vec.front());
    cppcut_assert_equal(grn::dat::UInt32(5), vec.back());

    vec.push_back(123);
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(2), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(2), vec.capacity());

    cppcut_assert_equal(grn::dat::UInt32(5), vec.front());
    cppcut_assert_equal(grn::dat::UInt32(123), vec.back());

    vec.pop_back();
    cppcut_assert_equal(false, vec.empty());
    cppcut_assert_equal(grn::dat::UInt32(1), vec.size());
    cppcut_assert_equal(grn::dat::UInt32(2), vec.capacity());

    cppcut_assert_equal(grn::dat::UInt32(5), vec.front());
    cppcut_assert_equal(grn::dat::UInt32(5), vec.back());

    vec.clear();

    for (grn::dat::UInt32 i = 0; i < 1000; ++i) {
      vec.push_back(i);
      cppcut_assert_equal(i, vec.back());
      cppcut_assert_equal(i + 1, vec.size());
    }
    for (grn::dat::UInt32 i = 0; i < 1000; ++i) {
      cppcut_assert_equal(1000 - i, vec.size());
      cppcut_assert_equal(999 - i, vec.back());
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
      cppcut_assert_equal(i, vec[i]);
      cppcut_assert_equal(i,
          const_cast<const grn::dat::Vector<grn::dat::UInt32> &>(vec)[i]);
      cppcut_assert_equal(vec.begin() + i, &vec[i]);
      cppcut_assert_equal(vec.end() - vec.size() + i, &vec[i]);
    }
  }

  void test_object_management(void)
  {
    grn::dat::Vector<Counter> vec;

    cppcut_assert_equal(0, Counter::constructor_count);
    cppcut_assert_equal(0, Counter::copy_count);
    cppcut_assert_equal(0, Counter::destructor_count);

    vec.push_back();

    cppcut_assert_equal(1, Counter::constructor_count);
    cppcut_assert_equal(0, Counter::copy_count);
    cppcut_assert_equal(0, Counter::destructor_count);

    vec.pop_back();

    cppcut_assert_equal(1, Counter::constructor_count);
    cppcut_assert_equal(0, Counter::copy_count);
    cppcut_assert_equal(1, Counter::destructor_count);

    vec.resize(10);

    cppcut_assert_equal(11, Counter::constructor_count);
    cppcut_assert_equal(0, Counter::copy_count);
    cppcut_assert_equal(1, Counter::destructor_count);

    vec.pop_back();

    cppcut_assert_equal(11, Counter::constructor_count);
    cppcut_assert_equal(0, Counter::copy_count);
    cppcut_assert_equal(2, Counter::destructor_count);

    vec.resize(11);

    cppcut_assert_equal(13, Counter::constructor_count);
    cppcut_assert_equal(9, Counter::copy_count);
    cppcut_assert_equal(11, Counter::destructor_count);

    vec.clear();

    cppcut_assert_equal(13, Counter::constructor_count);
    cppcut_assert_equal(9, Counter::copy_count);
    cppcut_assert_equal(22, Counter::destructor_count);
  }
}
