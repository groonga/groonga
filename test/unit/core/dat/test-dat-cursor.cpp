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

#include <grn_dat.h>

#include <gcutter.h>
#include <glib/gstdio.h>
#include <cppcutter.h>

#include <grn-assertions.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <set>
#include <string>
#include <vector>

namespace
{
  void create_key(std::string *key, std::size_t min_length,
                  std::size_t max_length)
  {
    key->resize(min_length + (std::rand() % (max_length - min_length + 1)));
    for (std::size_t i = 0; i < key->size(); ++i) {
      (*key)[i] = '0' + (std::rand() % 10);
    }
  }

  void create_keys(std::vector<std::string> *keys, std::size_t num_keys,
                   std::size_t min_length, std::size_t max_length)
  {
    std::string key;
    std::set<std::string> keyset;
    while (keyset.size() < num_keys) {
      create_key(&key, min_length, max_length);
      keyset.insert(key);
    }
    std::vector<std::string>(keyset.begin(), keyset.end()).swap(*keys);
    std::random_shuffle(keys->begin(), keys->end());
  }
}

namespace test_dat_cursor
{
  grn_ctx ctx;

  void cut_setup(void)
  {
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    grn_ctx_init(&ctx, 0);
  }

  void cut_teardown(void)
  {
    grn_ctx_fin(&ctx);
  }

  grn_dat *create_trie(const std::vector<std::string> &keys) {
    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0,
                                         GRN_OBJ_KEY_VAR_SIZE);
    cppcut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    return dat;
  }

  void test_cursor_open(void)
  {
    grn_dat *dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cppcut_assert_not_null(dat);
    grn_dat_cursor *cursor = grn_dat_cursor_open(
        &ctx, dat, "ABC", 0, "XYZ", 0, 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    grn_dat_cursor_close(&ctx, cursor);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    std::vector<std::string> keys;
    create_keys(&keys, 100, 3, 5);
    dat = create_trie(keys);
    cursor = grn_dat_cursor_open(
        &ctx, dat, "ABC", 0, "XYZ", 0, 0, -1, GRN_CURSOR_BY_KEY);
    cppcut_assert_not_null(cursor);
    grn_dat_cursor_close(&ctx, cursor);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_next(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor * const cursor = grn_dat_cursor_open(
        &ctx, dat, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    for (std::size_t i = 1; i <= keys.size(); ++i) {
      cppcut_assert_equal(static_cast<grn_id>(i),
                          grn_dat_cursor_next(&ctx, cursor));
    }
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_get_key(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor * const cursor = grn_dat_cursor_open(
        &ctx, dat, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const int length = static_cast<int>(keys[i].length());
      cppcut_assert_equal(key_id, grn_dat_cursor_next(&ctx, cursor));
      const void *key_ptr;
      cppcut_assert_equal(length,
                          grn_dat_cursor_get_key(&ctx, cursor, &key_ptr));
      uint32_t key_length;
      cppcut_assert_equal(_grn_dat_key(&ctx, dat, key_id, &key_length),
                          static_cast<const char *>(key_ptr));
    }
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_delete(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor *cursor = grn_dat_cursor_open(
        &ctx, dat, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    for (std::size_t i = 1; i <= keys.size(); ++i) {
      cppcut_assert_equal(static_cast<grn_id>(i),
                          grn_dat_cursor_next(&ctx, cursor));
      if (i & 1) {
        grn_test_assert_equal_rc(GRN_SUCCESS,
                                 grn_dat_cursor_delete(&ctx, cursor, NULL));
      }
    }
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0, 0, -1,
                                 GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    for (std::size_t i = 1; i <= (keys.size() / 2); ++i) {
      cppcut_assert_equal(static_cast<grn_id>(i * 2),
                          grn_dat_cursor_next(&ctx, cursor));
    }
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_by_id(void)
  {
    std::vector<std::string> keys;
    keys.push_back("mandarin");
    keys.push_back("grapefruit");
    keys.push_back("orange");
    keys.push_back("citron");
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor *cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0,
                                                 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(4),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, keys[2].c_str(), keys[2].length(),
                                 NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(4),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, keys[1].c_str(),
                                 keys[1].length(), 0, -1, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0,
                                 1, 2, GRN_CURSOR_BY_ID);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0, 1, 2,
                                 GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0, 1, 2,
                                 GRN_CURSOR_BY_ID | GRN_CURSOR_DESCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, keys[1].c_str(), keys[1].length(),
                                 keys[2].c_str(), keys[2].length(),
                                 0, -1, GRN_CURSOR_BY_ID | GRN_CURSOR_LT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, keys[1].c_str(), keys[1].length(),
                                 keys[2].c_str(), keys[2].length(),
                                 0, -1, GRN_CURSOR_BY_ID | GRN_CURSOR_GT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_by_key(void)
  {
    std::vector<std::string> keys;
    keys.push_back("mandarin");
    keys.push_back("grapefruit");
    keys.push_back("orange");
    keys.push_back("citron");
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor *cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0,
                                                 0, -1, GRN_CURSOR_BY_KEY);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(4),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, keys[0].c_str(), keys[0].length(),
                                 NULL, 0, 0, -1, GRN_CURSOR_BY_KEY);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, keys[1].c_str(),
                                 keys[1].length(), 0, -1, GRN_CURSOR_BY_KEY);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(4),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0,
                                 1, 2, GRN_CURSOR_BY_KEY);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0, 1, 2,
                                 GRN_CURSOR_BY_KEY | GRN_CURSOR_ASCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0, 1, 2,
                                 GRN_CURSOR_BY_KEY | GRN_CURSOR_DESCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, keys[1].c_str(), keys[1].length(),
                                 keys[0].c_str(), keys[0].length(),
                                 0, -1, GRN_CURSOR_BY_KEY | GRN_CURSOR_LT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, keys[1].c_str(), keys[1].length(),
                                 keys[0].c_str(), keys[0].length(),
                                 0, -1, GRN_CURSOR_BY_KEY | GRN_CURSOR_GT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_prefix_without_max(void)
  {
    std::vector<std::string> keys;
    keys.push_back("minimum");
    keys.push_back("mind");
    keys.push_back("minute");
    keys.push_back("mild");
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor *cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0,
                                                 0, -1, GRN_CURSOR_PREFIX);
    cut_assert_null(cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "", 0, NULL, 0, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cut_assert_null(cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "m", 1, NULL, 0, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(4),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "min", 3, NULL, 0, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "m", 1, NULL, 0, 1, 2,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "m", 1, NULL, 0, 1, 2,
                                 GRN_CURSOR_PREFIX | GRN_CURSOR_ASCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "m", 1, NULL, 0, 1, 2,
                                 GRN_CURSOR_PREFIX | GRN_CURSOR_DESCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "mind", 4, NULL, 0,
                                 0, -1, GRN_CURSOR_PREFIX | GRN_CURSOR_LT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "mind", 4, NULL, 0,
                                 0, -1, GRN_CURSOR_PREFIX | GRN_CURSOR_GT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_cursor_prefix_with_max(void)
  {
    std::vector<std::string> keys;
    keys.push_back("not");
    keys.push_back("no");
    keys.push_back("note");
    keys.push_back("nice");
    grn_dat * const dat = create_trie(keys);

    grn_dat_cursor *cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, NULL, 0,
                                                 0, -1, GRN_CURSOR_PREFIX);
    cut_assert_null(cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "", 0, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cut_assert_null(cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "note", 4, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "not", 3, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 3, "note", 4, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, "XYZ", 3, "note", 4, 0, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "note", 4, 1, -1,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "note", 4, 0, 2,
                                 GRN_CURSOR_PREFIX);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "note", 4, 0, 2,
                                 GRN_CURSOR_PREFIX | GRN_CURSOR_ASCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 0, "note", 4, 0, 2,
                                 GRN_CURSOR_PREFIX | GRN_CURSOR_DESCENDING);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 2, "note", 4,
                                 0, -1, GRN_CURSOR_PREFIX | GRN_CURSOR_LT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    cursor = grn_dat_cursor_open(&ctx, dat, NULL, 2, "note", 4,
                                 0, -1, GRN_CURSOR_PREFIX | GRN_CURSOR_GT);
    cppcut_assert_not_null(cursor);
    cppcut_assert_equal(static_cast<grn_id>(3),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(1),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(2),
                        grn_dat_cursor_next(&ctx, cursor));
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_cursor_next(&ctx, cursor));
    grn_dat_cursor_close(&ctx, cursor);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }
}
