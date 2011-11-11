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
#include <dat/predictive-cursor.hpp>
#include <dat/trie.hpp>

#include <cstring>

namespace
{
  void create_trie(grn::dat::Trie *trie)
  {
    trie->create();
    trie->insert("北斗", std::strlen("北斗"));                      // ID: 1, 2nd
    trie->insert("北斗神拳", std::strlen("北斗神拳"));              // ID: 2, 3rd
    trie->insert("北斗神拳伝承者", std::strlen("北斗神拳伝承者"));  // ID: 3, 4th
    trie->insert("南斗聖拳", std::strlen("南斗聖拳"));              // ID: 4, 6th
    trie->insert("南斗孤鷲拳", std::strlen("南斗孤鷲拳"));          // ID: 5, 5th
    trie->insert("元斗皇拳", std::strlen("元斗皇拳"));              // ID: 6, 1st
  }
}

namespace test_dat_predictive_cursor
{
  void test_null(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;
    cursor.open(trie, grn::dat::String());
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_str(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;

    cursor.open(trie, grn::dat::String("北斗"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗神拳"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗神拳伝承者"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("南斗"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("南斗聖拳"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("南斗水鳥拳"));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("元斗"));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_offset(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;

    cursor.open(trie, grn::dat::String(), 0);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 3);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 2);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 5);
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_limit(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;

    cursor.open(trie, grn::dat::String(), 0, grn::dat::UINT32_MAX);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 0, 4);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 2, 3);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, 2);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 1, 1);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("南斗"), 0, 0);
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 1, 100);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_ascending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;

    cursor.open(trie, grn::dat::String(), 0, grn::dat::UINT32_MAX,
                grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 3, grn::dat::UINT32_MAX,
                grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 3, 2,
                grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, grn::dat::UINT32_MAX,
                grn::dat::ASCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_descending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;

    cursor.open(trie, grn::dat::String(), 0, grn::dat::UINT32_MAX,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 3, grn::dat::UINT32_MAX,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String(), 3, 2,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, grn::dat::UINT32_MAX,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 2, grn::dat::UINT32_MAX,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, 1,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 3, grn::dat::UINT32_MAX,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, 0,
                grn::dat::DESCENDING_CURSOR);
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }

  void test_except_boundary(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::PredictiveCursor cursor;

    cursor.open(trie, grn::dat::String(), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(6));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(5));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(4));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北"), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(1));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗神"), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗神拳"), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗神拳伝承"), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗神拳伝承者"), 0, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 1, grn::dat::UINT32_MAX,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(3));
    cppcut_assert_equal(cursor.next().is_valid(), false);

    cursor.open(trie, grn::dat::String("北斗"), 0, 1,
                grn::dat::EXCEPT_EXACT_MATCH);
    cppcut_assert_equal(cursor.next().id(), grn::dat::UInt32(2));
    cppcut_assert_equal(cursor.next().is_valid(), false);
  }
}
