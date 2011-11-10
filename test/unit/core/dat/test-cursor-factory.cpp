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
#include <dat/cursor-factory.hpp>
#include <dat/trie.hpp>

#include <memory>

namespace
{
  void create_trie(grn::dat::Trie *trie)
  {
    trie->create();
    cppcut_assert_equal(trie->insert("apple", 5), true);
    cppcut_assert_equal(trie->insert("orange", 6), true);
    cppcut_assert_equal(trie->insert("banana", 6), true);
    cppcut_assert_equal(trie->insert("melon", 5), true);
  }
}

namespace test_dat_cursor_factory
{
  void test_key_range_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    std::auto_ptr<grn::dat::Cursor> cursor(grn::dat::CursorFactory::open(
        trie, "apple", 5, "melon", 5, 1, 2,
        grn::dat::KEY_RANGE_CURSOR | grn::dat::EXCEPT_LOWER_BOUND |
        grn::dat::EXCEPT_UPPER_BOUND));
    cut_assert(cursor.get() != static_cast<grn::dat::Cursor *>(NULL));

    cppcut_assert_equal(cursor->offset(), static_cast<grn::dat::UInt32>(1));
    cppcut_assert_equal(cursor->limit(), static_cast<grn::dat::UInt32>(2));
    cppcut_assert_equal(cursor->flags(),
                        grn::dat::KEY_RANGE_CURSOR | grn::dat::ASCENDING_CURSOR |
                        grn::dat::EXCEPT_LOWER_BOUND | grn::dat::EXCEPT_UPPER_BOUND);
  }

  void test_id_range_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    std::auto_ptr<grn::dat::Cursor> cursor(grn::dat::CursorFactory::open(
        trie, "apple", 5, "melon", 5, 1, 2,
        grn::dat::ID_RANGE_CURSOR | grn::dat::ASCENDING_CURSOR));
    cut_assert(cursor.get() != static_cast<grn::dat::Cursor *>(NULL));

    cppcut_assert_equal(cursor->offset(), static_cast<grn::dat::UInt32>(1));
    cppcut_assert_equal(cursor->limit(), static_cast<grn::dat::UInt32>(2));
    cppcut_assert_equal(cursor->flags(),
                        grn::dat::ID_RANGE_CURSOR | grn::dat::ASCENDING_CURSOR);
  }

  void test_prefix_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    std::auto_ptr<grn::dat::Cursor> cursor(grn::dat::CursorFactory::open(
        trie, NULL, 3, "apple", 5, 0, 1,
        grn::dat::PREFIX_CURSOR | grn::dat::DESCENDING_CURSOR));
    cut_assert(cursor.get() != static_cast<grn::dat::Cursor *>(NULL));

    cppcut_assert_equal(cursor->offset(), static_cast<grn::dat::UInt32>(0));
    cppcut_assert_equal(cursor->limit(), static_cast<grn::dat::UInt32>(1));
    cppcut_assert_equal(cursor->flags(),
                        grn::dat::PREFIX_CURSOR | grn::dat::DESCENDING_CURSOR);
  }

  void test_predictive_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    std::auto_ptr<grn::dat::Cursor> cursor(grn::dat::CursorFactory::open(
        trie, "apple", 5, NULL, 0, 1, 2,
        grn::dat::PREDICTIVE_CURSOR | grn::dat::EXCEPT_EXACT_MATCH));
    cut_assert(cursor.get() != static_cast<grn::dat::Cursor *>(NULL));

    cppcut_assert_equal(cursor->offset(), static_cast<grn::dat::UInt32>(1));
    cppcut_assert_equal(cursor->limit(), static_cast<grn::dat::UInt32>(2));
    cppcut_assert_equal(cursor->flags(),
                        grn::dat::PREDICTIVE_CURSOR |
                        grn::dat::ASCENDING_CURSOR | grn::dat::EXCEPT_EXACT_MATCH);
  }
}
