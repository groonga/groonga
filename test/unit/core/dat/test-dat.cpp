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

#define __STDC_LIMIT_MACROS

#include <gcutter.h>
#include <glib/gstdio.h>
#include <cppcutter.h>

#include <grn-assertions.h>
#include <dat.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <set>
#include <string>
#include <vector>

namespace
{
  void create_key(std::string *key, std::size_t min_length, std::size_t max_length)
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

namespace test_dat
{
  const char *base_dir;
  grn_ctx ctx;

  void cut_setup(void)
  {
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    base_dir = grn_test_get_tmp_dir();
    cut_remove_path(base_dir, NULL);
    g_mkdir_with_parents(base_dir, 0755);

    grn_init();
    grn_ctx_init(&ctx, 0);
  }

  void cut_teardown(void)
  {
    grn_ctx_fin(&ctx);
    grn_fin();

    if (base_dir) {
      cut_remove_path(base_dir, NULL);
    }
  }

  void test_create_without_path(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_id added_key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, NULL);
      cppcut_assert_equal(static_cast<grn_id>(i + 1), added_key_id);
      const grn_id gotten_key_id = grn_dat_get(&ctx, dat, ptr, length, NULL);
      cppcut_assert_equal(added_key_id, gotten_key_id);
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_create_with_path(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_create_with_path.tmp");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, dat_path, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_id added_key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, NULL);
      cppcut_assert_equal(static_cast<grn_id>(i + 1), added_key_id);
      const grn_id gotten_key_id = grn_dat_get(&ctx, dat, ptr, length, NULL);
      cppcut_assert_equal(added_key_id, gotten_key_id);
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    cut_assert_null(grn_dat_create(&ctx, "", 0, 0, 0));
  }

  void test_open(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_open.tmp");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, dat_path, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    grn_dat * const dat2 = grn_dat_open(&ctx, dat_path);
    cut_assert_not_null(dat2);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_get(&ctx, dat2, ptr, length, NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat2));

    cut_assert_null(grn_dat_open(&ctx, NULL));
    cut_assert_null(grn_dat_open(&ctx, ""));
  }

  void test_remove(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_remove.tmp");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, dat_path, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    const uint32_t last_file_id = dat->file_id;
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    cppcut_assert_equal(GRN_SUCCESS, grn_dat_remove(&ctx, dat_path));
    cut_assert_not_exist_path(dat_path);
    char trie_path[PATH_MAX];
    for (uint32_t i = 1; i <= last_file_id; ++i) {
      sprintf(trie_path, "%s.%03d", dat_path, i);
      cut_assert_not_exist_path(trie_path);
    }

    cppcut_assert_equal(GRN_INVALID_ARGUMENT, grn_dat_remove(&ctx, NULL));
  }

  void test_get(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_id added_key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, NULL);
      const grn_id gotten_key_id = grn_dat_get(&ctx, dat, ptr, length, NULL);
      cppcut_assert_equal(added_key_id, gotten_key_id);
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_add(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_get_key(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_id key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, NULL);
      cppcut_assert_equal(static_cast<grn_id>(i + 1), key_id);
      cppcut_assert_equal(length,
                          static_cast<uint32_t>(grn_dat_get_key(&ctx, dat, key_id, NULL, 0)));
      char key_buf[16];
      const int key_length = grn_dat_get_key(&ctx, dat, key_id, key_buf, sizeof(key_buf));
      cppcut_assert_equal(length, static_cast<uint32_t>(key_length));
      cppcut_assert_equal(keys[i], std::string(key_buf, key_length));
    }
    cppcut_assert_equal(0, grn_dat_get_key(&ctx, dat, GRN_ID_NIL, NULL, 0));
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_get_key2(void)
  {
    cut_omit("Not implemented yet.");

    grn_obj bulk;
    GRN_OBJ_INIT(&bulk, 0, 0, 0);

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_id key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, NULL);
      cppcut_assert_equal(static_cast<grn_id>(i + 1), key_id);

      uint32_t key_length;
      const char * const key_ptr = _grn_dat_key(&ctx, dat, key_id, &key_length);
      cut_assert_not_null(key_ptr);
      cppcut_assert_equal(length, key_length);

      bulk.header.impl_flags |= GRN_OBJ_REFER;
      cppcut_assert_equal(length,
                          static_cast<uint32_t>(grn_dat_get_key2(&ctx, dat, key_id, &bulk)));
      cppcut_assert_equal(key_ptr, bulk.u.b.head);
      cppcut_assert_equal(length, static_cast<uint32_t>(bulk.u.b.curr - bulk.u.b.head));
    }
    cppcut_assert_equal(0, grn_dat_get_key2(&ctx, dat, GRN_ID_NIL, &bulk));
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    GRN_OBJ_FIN(&ctx, &bulk);
  }

  void test_delete_by_id(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_delete_by_id(&ctx, dat, static_cast<grn_id>(i + 1), NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_delete_by_id(&ctx, dat, static_cast<grn_id>(i + 1), NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      int added;
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
      cppcut_assert_equal(static_cast<grn_id>(keys.size() - i),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, &added));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_delete_by_id(&ctx, dat, static_cast<grn_id>(i + 1), NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_delete_by_id(&ctx, dat, static_cast<grn_id>(i + 1), NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_delete(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_delete(&ctx, dat, ptr, length, NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_delete(&ctx, dat, ptr, length, NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      int added;
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
      cppcut_assert_equal(static_cast<grn_id>(keys.size() - i),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, &added));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_delete(&ctx, dat, ptr, length, NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_delete(&ctx, dat, ptr, length, NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_update_by_id(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    for (std::size_t i = (keys.size() / 2); i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1 - (keys.size() / 2));
      const char * const src_ptr = keys[i - (keys.size() / 2)].c_str();
      const uint32_t src_length = static_cast<uint32_t>(keys[i - (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
                          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
    }
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const char *src_ptr = keys[i + (keys.size() / 2)].c_str();
      const uint32_t src_length = static_cast<uint32_t>(keys[i + (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
                          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_update(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    for (std::size_t i = (keys.size() / 2); i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1 - (keys.size() / 2));
      const char * const src_ptr = keys[i - (keys.size() / 2)].c_str();
      const uint32_t src_length = static_cast<uint32_t>(keys[i - (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_update(&ctx, dat, src_ptr, src_length, dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
                          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_update(&ctx, dat, src_ptr, src_length, dest_ptr, dest_length));
    }
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const char *src_ptr = keys[i + (keys.size() / 2)].c_str();
      const uint32_t src_length = static_cast<uint32_t>(keys[i + (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(GRN_SUCCESS,
                          grn_dat_update(&ctx, dat, src_ptr, src_length, dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
                          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      cppcut_assert_equal(GRN_INVALID_ARGUMENT,
                          grn_dat_update(&ctx, dat, src_ptr, src_length, dest_ptr, dest_length));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_size(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
      cppcut_assert_equal(static_cast<unsigned int>(i + 1),
                          grn_dat_size(&ctx, dat));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_curr_id(void)
  {
    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);

    grn_id max_key_id = 0;
    std::string key;
    for (uint32_t i = 1; i <= 1000; ++i) {
      cppcut_assert_equal(max_key_id, grn_dat_curr_id(&ctx, dat));
      create_key(&key, 1, 3);
      int added;
      const grn_id key_id = grn_dat_add(&ctx, dat, key.c_str(), key.length(), NULL, &added);
      if (added) {
        cppcut_assert_equal(++max_key_id, key_id);
      }
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_truncate(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_truncate(&ctx, dat));
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_key(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = grn_dat_create(&ctx, NULL, 0, 0, 0);
    cut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_id key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, NULL);
      cppcut_assert_equal(static_cast<grn_id>(i + 1), key_id);
      cppcut_assert_equal(length,
                          static_cast<uint32_t>(grn_dat_get_key(&ctx, dat, key_id, NULL, 0)));
      uint32_t key_length;
      const char * const key_ptr = _grn_dat_key(&ctx, dat, key_id, &key_length);
      cut_assert_not_null(key_ptr);
      cppcut_assert_equal(length, static_cast<uint32_t>(key_length));
      cppcut_assert_equal(keys[i], std::string(key_ptr, key_length));
    }
    cut_assert_null(_grn_dat_key(&ctx, dat, GRN_ID_NIL, NULL));
    cppcut_assert_equal(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }
}
