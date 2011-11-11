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
#include <dat/prefix-cursor.hpp>
#include <dat/trie.hpp>

#include <cstring>

namespace
{
  void create_trie(grn::dat::Trie *trie)
  {
    trie->create();
    trie->insert("東京", std::strlen("東京"));          // ID: 1, 3rd
    trie->insert("京都", std::strlen("京都"));          // ID: 2, 1st
    trie->insert("東京都", std::strlen("東京都"));      // ID: 3, 4th
    trie->insert("京都府", std::strlen("京都府"));      // ID: 4, 2nd
    trie->insert("東京都庁", std::strlen("東京都庁"));  // ID: 5, 5th
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
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_str(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("京都"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("京都府"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都議会"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_min_length(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen(""));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京都"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京都庁"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京都庁ビル"));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_offset(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 1);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 2);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 3);
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_limit(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, grn::dat::UINT32_MAX);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 3);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 2);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 1);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 0, 0);
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 2, 100);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0, 1, 1);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_ascending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("京都府"), 0,
                0, grn::dat::UINT32_MAX, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("京都府"), std::strlen("京都"),
                0, grn::dat::UINT32_MAX, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("京都府"), 0,
                1, grn::dat::UINT32_MAX, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("京都府"), 0,
                0, 1, grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_descending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁"), 0,
                0, grn::dat::UINT32_MAX, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京都"),
                0, grn::dat::UINT32_MAX, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京都"),
                1, grn::dat::UINT32_MAX, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京都"),
                0, 1, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"), 0,
                1, grn::dat::UINT32_MAX, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"), 0,
                0, 2, grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_except_boundary(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PrefixCursor cursor;

    cursor.open(trie, grn::dat::String("東京都庁ビル"), 0,
                0, grn::dat::UINT32_MAX, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁ビル"), std::strlen("東京"),
                0, grn::dat::UINT32_MAX, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("東京都庁"), std::strlen("東京"),
                0, grn::dat::UINT32_MAX, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("京都府"), 0,
                0, grn::dat::UINT32_MAX, grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }
}
