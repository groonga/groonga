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
#include <dat/prefix-cursor.hpp>
#include <dat/trie.hpp>

#include <cstring>

namespace
{
  void create_trie(grn::dat::Trie *trie)
  {
    trie->create();
    trie->insert("東京", std::strlen("東京"));          // 3rd
    trie->insert("京都", std::strlen("京都"));          // 1st
    trie->insert("東京都", std::strlen("東京都"));      // 4th
    trie->insert("京都府", std::strlen("京都府"));      // 2nd
    trie->insert("東京都庁", std::strlen("東京都庁"));  // 5th
  }
}

namespace test_dat_prefix_cursor
{
  void test_null(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;
    cursor.open(trie, grn::dat::String());
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_str(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("京都"));
    cppcut_assert_equal(grn::dat::UInt32(2), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("京都府"));
    cppcut_assert_equal(grn::dat::UInt32(2), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(4), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京"));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都"));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都議会"));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_min_length(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen(""));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東"));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京"));
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京都"));
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"),
                std::strlen("東京都庁"));
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"),
                std::strlen("東京都庁ビル"));
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_offset(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 1);
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 2);
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 3);
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_limit(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0,
                grn::dat::MAX_UINT32);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 3);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 2);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 1);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 0);
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 2, 100);
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 1, 1);
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_ascending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("京都府"), 0,
                0, grn::dat::MAX_UINT32, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(2), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(4), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("京都府"), std::strlen("京都"),
                0, grn::dat::MAX_UINT32, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(2), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(4), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("京都府"), 0,
                1, grn::dat::MAX_UINT32, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(4), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("京都府"), 0,
                0, 1, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(2), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_descending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁"), 0,
                0, grn::dat::MAX_UINT32, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京都"),
                0, grn::dat::MAX_UINT32, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京都"),
                1, grn::dat::MAX_UINT32, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京都"),
                0, 1, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"), 0,
                1, grn::dat::MAX_UINT32, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"), 0,
                0, 2, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_except_boundary(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0,
                0, grn::dat::MAX_UINT32, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京"),
                0, grn::dat::MAX_UINT32, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(5), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京"),
                0, grn::dat::MAX_UINT32, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(grn::dat::UInt32(1), cursor.next().id());
    cppcut_assert_equal(grn::dat::UInt32(3), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::String("京都府"), 0,
                0, grn::dat::MAX_UINT32, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(grn::dat::UInt32(2), cursor.next().id());
    cppcut_assert_equal(false, cursor.next().is_valid());
  }
}
