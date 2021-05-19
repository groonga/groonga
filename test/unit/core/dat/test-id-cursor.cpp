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
#include <dat/id-cursor.hpp>
#include <dat/trie.hpp>

#include <cstring>

namespace
{
  void create_trie(grn::dat::Trie *trie)
  {
    trie->create();
    trie->insert("みかん", std::strlen("みかん"));
    trie->insert("オレンジ", std::strlen("オレンジ"));
    trie->insert("グレープフルーツ", std::strlen("グレープフルーツ"));
    trie->insert("柚子", std::strlen("柚子"));
    trie->insert("スダチ", std::strlen("スダチ"));
    trie->insert("伊予柑", std::strlen("伊予柑"));
    trie->insert("八朔", std::strlen("八朔"));
    trie->insert("文旦", std::strlen("文旦"));
  }
}

namespace test_dat_id_cursor
{
  void test_null(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;
    cursor.open(trie, grn::dat::String(), grn::dat::String());
    for (grn::dat::UInt32 i = trie.min_key_id(); i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_min_by_str(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::UInt32 key_pos;
    cppcut_assert_equal(true,
        trie.search("スダチ", std::strlen("スダチ"), &key_pos));
    const grn::dat::UInt32 min_key_id = trie.get_key(key_pos).id();

    grn::dat::IdCursor cursor;
    cursor.open(trie, grn::dat::String("スダチ"), grn::dat::String());
    for (grn::dat::UInt32 i = min_key_id; i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_max_by_str(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::UInt32 key_pos;
    cppcut_assert_equal(true,
        trie.search("オレンジ", std::strlen("オレンジ"), &key_pos));
    const grn::dat::UInt32 max_key_id = trie.get_key(key_pos).id();

    grn::dat::IdCursor cursor;
    cursor.open(trie, grn::dat::String(), grn::dat::String("オレンジ"));
    for (grn::dat::UInt32 i = trie.min_key_id(); i <= max_key_id; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_min_max_by_str(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::UInt32 key_pos;
    cppcut_assert_equal(true,
        trie.search("みかん", std::strlen("みかん"), &key_pos));
    const grn::dat::UInt32 min_key_id = trie.get_key(key_pos).id();
    cppcut_assert_equal(true,
        trie.search("八朔", std::strlen("八朔"), &key_pos));
    const grn::dat::UInt32 max_key_id = trie.get_key(key_pos).id();

    grn::dat::IdCursor cursor;
    cursor.open(trie, grn::dat::String("みかん"), grn::dat::String("八朔"));
    for (grn::dat::UInt32 i = min_key_id; i <= max_key_id; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_invalid_id(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;
    cursor.open(trie, grn::dat::INVALID_KEY_ID, grn::dat::INVALID_KEY_ID);
    for (grn::dat::UInt32 i = trie.min_key_id(); i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_min_by_id(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::UInt32 key_pos;
    cppcut_assert_equal(true,
        trie.search("伊予柑", std::strlen("伊予柑"), &key_pos));
    const grn::dat::UInt32 min_key_id = trie.get_key(key_pos).id();

    grn::dat::IdCursor cursor;
    cursor.open(trie, min_key_id, grn::dat::INVALID_KEY_ID);
    for (grn::dat::UInt32 i = min_key_id; i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_max_by_id(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::UInt32 key_pos;
    cppcut_assert_equal(true,
        trie.search("柚子", std::strlen("柚子"), &key_pos));
    const grn::dat::UInt32 max_key_id = trie.get_key(key_pos).id();

    grn::dat::IdCursor cursor;
    cursor.open(trie, grn::dat::INVALID_KEY_ID, max_key_id);
    for (grn::dat::UInt32 i = trie.min_key_id(); i <= max_key_id; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_min_max_by_id(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::UInt32 key_pos;
    cppcut_assert_equal(true,
        trie.search("グレープフルーツ", std::strlen("グレープフルーツ"), &key_pos));
    const grn::dat::UInt32 min_key_id = trie.get_key(key_pos).id();
    cppcut_assert_equal(true,
        trie.search("文旦", std::strlen("文旦"), &key_pos));
    const grn::dat::UInt32 max_key_id = trie.get_key(key_pos).id();

    grn::dat::IdCursor cursor;
    cursor.open(trie, min_key_id, max_key_id);
    for (grn::dat::UInt32 i = min_key_id; i <= max_key_id; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_offset(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;

    cursor.open(trie, 2, 5, 0);
    for (grn::dat::UInt32 i = 2; i <= 5; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::INVALID_KEY_ID, grn::dat::INVALID_KEY_ID, 5);
    for (grn::dat::UInt32 i = trie.min_key_id() + 5;
         i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 3, 7, 2);
    for (grn::dat::UInt32 i = 3 + 2; i <= 7; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 5, 100);
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_offset_after_delete(void) {
    grn::dat::Trie trie;
    create_trie(&trie);

    for (grn::dat::UInt32 i = 1; i <= trie.max_key_id(); i += 2) {
      trie.remove(i);
    }

    grn::dat::IdCursor cursor;

    cursor.open(trie, 0, 0, 1);
    for (grn::dat::UInt32 i = 4; i <= trie.max_key_id(); i += 2) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 0, 0, 2);
    for (grn::dat::UInt32 i = 6; i <= trie.max_key_id(); i += 2) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 0, 0, trie.max_key_id() / 2);
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_limit(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;

    cursor.open(trie, 3, 6, 0, grn::dat::MAX_UINT32);
    for (grn::dat::UInt32 i = 3; i <= 6; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, grn::dat::INVALID_KEY_ID,
                grn::dat::INVALID_KEY_ID, 0, 3);
    for (grn::dat::UInt32 i = trie.min_key_id();
         i < (trie.min_key_id() + 3); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 3, 7, 1, 2);
    for (grn::dat::UInt32 i = 3 + 1; i < (3 + 1 + 2); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 5, 0, 0);
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_limit_after_delete(void) {
    grn::dat::Trie trie;
    create_trie(&trie);

    for (grn::dat::UInt32 i = 1; i <= trie.max_key_id(); i += 2) {
      trie.remove(i);
    }

    grn::dat::IdCursor cursor;

    cursor.open(trie, 0, 0, 0, trie.max_key_id() / 2);
    for (grn::dat::UInt32 i = 2; i <= trie.max_key_id(); i += 2) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 0, 0, 0, 1);
    for (grn::dat::UInt32 i = 2; i <= 2; i += 2) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 0, 0, 0, 3);
    for (grn::dat::UInt32 i = 2; i <= (2 + 4); i += 2) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_ascending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;

    cursor.open(trie, grn::dat::INVALID_KEY_ID, grn::dat::INVALID_KEY_ID,
                0, grn::dat::MAX_UINT32, grn::dat::ASCENDING_CURSOR);
    for (grn::dat::UInt32 i = trie.min_key_id(); i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 7, 0, grn::dat::MAX_UINT32,
                grn::dat::ASCENDING_CURSOR);
    for (grn::dat::UInt32 i = 2; i <= 7; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 3, 6, 1, 5, grn::dat::ASCENDING_CURSOR);
    for (grn::dat::UInt32 i = 3 + 1; i <= 6; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_descending_cursor(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;

    cursor.open(trie, grn::dat::INVALID_KEY_ID, grn::dat::INVALID_KEY_ID,
                0, grn::dat::MAX_UINT32, grn::dat::DESCENDING_CURSOR);
    for (grn::dat::UInt32 i = trie.max_key_id(); i >= trie.min_key_id(); --i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 7, 0, grn::dat::MAX_UINT32,
                grn::dat::DESCENDING_CURSOR);
    for (grn::dat::UInt32 i = 7; i >= 2; --i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 8, 2, 3, grn::dat::DESCENDING_CURSOR);
    for (grn::dat::UInt32 i = 8 - 2; i > (8 - 2 - 3); --i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }

  void test_except_boundary(void)
  {
    grn::dat::Trie trie;
    create_trie(&trie);

    grn::dat::IdCursor cursor;

    cursor.open(trie, grn::dat::INVALID_KEY_ID, grn::dat::INVALID_KEY_ID,
                0, grn::dat::MAX_UINT32,
                grn::dat::EXCEPT_LOWER_BOUND | grn::dat::EXCEPT_UPPER_BOUND);
    for (grn::dat::UInt32 i = trie.min_key_id(); i <= trie.max_key_id(); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, trie.min_key_id(), trie.max_key_id(),
                0, grn::dat::MAX_UINT32,
                grn::dat::EXCEPT_LOWER_BOUND | grn::dat::EXCEPT_UPPER_BOUND);
    for (grn::dat::UInt32 i = trie.min_key_id() + 1;
         i <= (trie.max_key_id() - 1); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 7, 1, 100, grn::dat::EXCEPT_LOWER_BOUND);
    for (grn::dat::UInt32 i = 2 + 1 + 1; i <= 7; ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());

    cursor.open(trie, 2, 7, 1, 100, grn::dat::EXCEPT_UPPER_BOUND);
    for (grn::dat::UInt32 i = 2 + 1; i <= (7 - 1); ++i) {
      const grn::dat::Key &key = cursor.next();
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(i, key.id());
    }
    cppcut_assert_equal(false, cursor.next().is_valid());
  }
}
