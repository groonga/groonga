/*
  Copyright (C) 2011-2012  Brazil
  Copyright (C) 2019  Sutou Kouhei <kou@clear-code.com>

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
  void enter_api(grn_ctx *ctx) {
    GRN_API_ENTER;
  }

  int leave_api(grn_ctx *ctx) {
    GRN_API_RETURN(0);
  }

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

#define ENABLE_API APIScope api_scope(&ctx);

namespace test_dat
{
  const char *base_dir;
  grn_ctx ctx;
  grn_obj *database;

  void cut_setup(void)
  {
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    base_dir = grn_test_get_tmp_dir();
    cut_remove_path(base_dir, NULL);
    g_mkdir_with_parents(base_dir, 0755);

    grn_ctx_init(&ctx, 0);
    database = grn_db_create(&ctx, NULL, NULL);
    enter_api(&ctx);
  }

  void cut_teardown(void)
  {
    leave_api(&ctx);
    grn_obj_close(&ctx, database);
    grn_ctx_fin(&ctx);

    if (base_dir) {
      cut_remove_path(base_dir, NULL);
    }
  }

  grn_dat *create_trie(const std::vector<std::string> &keys, const char *path,
                       unsigned int flags = 0) {
    flags |= GRN_OBJ_KEY_VAR_SIZE;
    grn_dat * const dat = grn_dat_create(&ctx, path, 0, 0, flags);
    cppcut_assert_not_null(dat);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      int added;
      const grn_id key_id = grn_dat_add(&ctx, dat, ptr, length, NULL, &added);
      cppcut_assert_not_equal(static_cast<const grn_id>(GRN_ID_NIL),
                              key_id);
      cut_assert_true(added);
    }
    return dat;
  }

  void test_create_without_path(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat *dat = create_trie(keys, NULL);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    dat = create_trie(keys, "");
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_create_with_path(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_create_with_path.tmp");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, dat_path);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_open(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_open.tmp");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, dat_path);
    grn_dat * const dat2 = grn_dat_open(&ctx, dat_path);
    cppcut_assert_not_null(dat2);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_get(&ctx, dat2, ptr, length, NULL));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat2));

    cppcut_assert_null(grn_dat_open(&ctx, NULL));
    cppcut_assert_null(grn_dat_open(&ctx, ""));
  }

  void test_remove(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_remove.tmp");

    grn_test_assert_equal_rc(GRN_NO_SUCH_FILE_OR_DIRECTORY,
                             grn_dat_remove(&ctx, dat_path));
    grn_test_assert_equal_rc(GRN_NO_SUCH_FILE_OR_DIRECTORY, ctx.rc);
    ctx.rc = GRN_SUCCESS;

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, dat_path);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_repair(&ctx, dat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_repair(&ctx, dat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_repair(&ctx, dat));
    const uint32_t last_file_id = dat->file_id;
    cppcut_assert_equal(static_cast<uint32_t>(4), last_file_id);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_remove(&ctx, dat_path));
    cut_assert_not_exist_path(dat_path);
    char trie_path[PATH_MAX];
    for (uint32_t i = 1; i <= last_file_id; ++i) {
      grn_snprintf(trie_path, PATH_MAX, PATH_MAX, "%s.%03d", dat_path, i);
      cut_assert_not_exist_path(trie_path);
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, ctx.rc);

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_remove(&ctx, NULL));
  }

  void test_get(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_add(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_add(&ctx, dat, "", 0, NULL, NULL));

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_get_key(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const int length = static_cast<int>(keys[i].length());
      cppcut_assert_equal(length, grn_dat_get_key(&ctx, dat, key_id, NULL, 0));
      char key_buf[16];
      const int key_length =
          grn_dat_get_key(&ctx, dat, key_id, key_buf, sizeof(key_buf));
      cppcut_assert_equal(length, key_length);
      cppcut_assert_equal(keys[i], std::string(key_buf, key_length));
    }
    cppcut_assert_equal(0, grn_dat_get_key(&ctx, dat, GRN_ID_NIL, NULL, 0));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_get_key2_with_refer(void)
  {
    grn_obj bulk;
    GRN_OBJ_INIT(&bulk, 0, GRN_OBJ_REFER, 0);

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const int length = static_cast<int>(keys[i].length());

      uint32_t key_length;
      const char * const key_ptr =
          _grn_dat_key(&ctx, dat, key_id, &key_length);
      cppcut_assert_not_null(key_ptr);
      cppcut_assert_equal(length, static_cast<int>(key_length));

      cppcut_assert_equal(length, grn_dat_get_key2(&ctx, dat, key_id, &bulk));
      cppcut_assert_equal(key_ptr, bulk.u.b.head);
      cppcut_assert_equal(length,
                          static_cast<int>(bulk.u.b.curr - bulk.u.b.head));
    }
    cppcut_assert_equal(0, grn_dat_get_key2(&ctx, dat, GRN_ID_NIL, &bulk));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    GRN_OBJ_FIN(&ctx, &bulk);
  }

  void test_get_key2_with_outplace(void)
  {
    grn_obj bulk;
    GRN_TEXT_INIT(&bulk, 0);

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const int length = static_cast<int>(keys[i].length());

      uint32_t key_length;
      const char * const key_ptr =
          _grn_dat_key(&ctx, dat, key_id, &key_length);
      cppcut_assert_not_null(key_ptr);
      cppcut_assert_equal(length, static_cast<int>(key_length));

      GRN_BULK_REWIND(&bulk);
      cppcut_assert_equal(length, grn_dat_get_key2(&ctx, dat, key_id, &bulk));
      cppcut_assert_equal(length, static_cast<int>(GRN_TEXT_LEN(&bulk)));
      cppcut_assert_equal(keys[i],
          std::string(GRN_TEXT_VALUE(&bulk), GRN_TEXT_LEN(&bulk)));
    }
    cppcut_assert_equal(0, grn_dat_get_key2(&ctx, dat, GRN_ID_NIL, &bulk));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    GRN_OBJ_FIN(&ctx, &bulk);
  }

  void test_delete_by_id(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      int added;
      cppcut_assert_equal(static_cast<grn_id>(keys.size() - i),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, &added));
      cut_assert_true(added);
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_delete(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete(&ctx, dat, ptr, length, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                               grn_dat_delete(&ctx, dat, ptr, length, NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      int added;
      cppcut_assert_equal(static_cast<grn_id>(keys.size() - i),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, &added));
      cut_assert_true(added);
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete(&ctx, dat, ptr, length, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                               grn_dat_delete(&ctx, dat, ptr, length, NULL));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_update_by_id(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat =
        grn_dat_create(&ctx, NULL, 0, 0, GRN_OBJ_KEY_VAR_SIZE);
    cppcut_assert_not_null(dat);
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_update_by_id(&ctx, dat, 1, "", 0));
    for (std::size_t i = (keys.size() / 2); i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1 - (keys.size() / 2));
      const char * const src_ptr = keys[i - (keys.size() / 2)].c_str();
      const uint32_t src_length =
          static_cast<uint32_t>(keys[i - (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
    }
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const char *src_ptr = keys[i + (keys.size() / 2)].c_str();
      const uint32_t src_length =
          static_cast<uint32_t>(keys[i + (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
          grn_dat_update_by_id(&ctx, dat, key_id, dest_ptr, dest_length));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_update(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat =
        grn_dat_create(&ctx, NULL, 0, 0, GRN_OBJ_KEY_VAR_SIZE);
    cppcut_assert_not_null(dat);
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, NULL));
    }
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
        grn_dat_update(&ctx, dat, keys[1].c_str(), keys[1].length(), "", 0));
    for (std::size_t i = (keys.size() / 2); i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1 - (keys.size() / 2));
      const char * const src_ptr = keys[i - (keys.size() / 2)].c_str();
      const uint32_t src_length =
          static_cast<uint32_t>(keys[i - (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
          grn_dat_update(&ctx, dat, src_ptr, src_length,
                         dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
          grn_dat_update(&ctx, dat, src_ptr, src_length,
                         dest_ptr, dest_length));
    }
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const char *src_ptr = keys[i + (keys.size() / 2)].c_str();
      const uint32_t src_length =
          static_cast<uint32_t>(keys[i + (keys.size() / 2)].length());
      const char * const dest_ptr = keys[i].c_str();
      const uint32_t dest_length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
          grn_dat_update(&ctx, dat, src_ptr, src_length,
                         dest_ptr, dest_length));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
          grn_dat_get(&ctx, dat, src_ptr, src_length, NULL));
      cppcut_assert_equal(key_id,
          grn_dat_get(&ctx, dat, dest_ptr, dest_length, NULL));
      grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
          grn_dat_update(&ctx, dat, src_ptr, src_length,
                         dest_ptr, dest_length));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_scan(void)
  {
    {
      std::vector<std::string> keys;
      keys.push_back("12");
      keys.push_back("234");
      keys.push_back("45");
      keys.push_back("789");

      const char text[] = "0123456789X";
      const unsigned int text_size = sizeof(text) - 1;
      grn_dat_scan_hit scan_hits[4];
      const unsigned int max_num_scan_hits =
          sizeof(scan_hits) / sizeof(scan_hits[0]);

      grn_dat * const dat = create_trie(keys, NULL);

      const char *text_rest;
      cppcut_assert_equal(3,
                          grn_dat_scan(&ctx, dat, text, text_size,
                          scan_hits, max_num_scan_hits, &text_rest));
      cppcut_assert_equal(text + text_size, text_rest);
      cppcut_assert_equal(static_cast<grn_id>(1), scan_hits[0].id);
      cppcut_assert_equal(1U, scan_hits[0].offset);
      cppcut_assert_equal(2U, scan_hits[0].length);
      cppcut_assert_equal(static_cast<grn_id>(3), scan_hits[1].id);
      cppcut_assert_equal(4U, scan_hits[1].offset);
      cppcut_assert_equal(2U, scan_hits[1].length);
      cppcut_assert_equal(static_cast<grn_id>(4), scan_hits[2].id);
      cppcut_assert_equal(7U, scan_hits[2].offset);
      cppcut_assert_equal(3U, scan_hits[2].length);

      cppcut_assert_equal(1,
          grn_dat_scan(&ctx, dat, text, text_size, scan_hits, 1, &text_rest));
      cppcut_assert_equal(static_cast<std::ptrdiff_t>(3), text_rest - text);
      cppcut_assert_equal(1,
          grn_dat_scan(&ctx, dat, text_rest, text_size - (text_rest - text),
                       scan_hits, 1, &text_rest));
      cppcut_assert_equal(static_cast<std::ptrdiff_t>(6), text_rest - text);
      cppcut_assert_equal(1,
          grn_dat_scan(&ctx, dat, text_rest, text_size - (text_rest - text),
                       scan_hits, 1, &text_rest));
      cppcut_assert_equal(static_cast<std::ptrdiff_t>(10), text_rest - text);
      cppcut_assert_equal(0,
          grn_dat_scan(&ctx, dat, text_rest, text_size - (text_rest - text),
                       scan_hits, 1, &text_rest));

      grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
    }

    {
      std::vector<std::string> keys;
      keys.push_back("ユニグラム");
      keys.push_back("グラム");

      const char text[] = "ユニ㌘…ハ゛イク゛ラム";
      const unsigned int text_size = sizeof(text) - 1;
      grn_dat_scan_hit scan_hits[4];
      const unsigned int max_num_scan_hits =
          sizeof(scan_hits) / sizeof(scan_hits[0]);

      grn_dat * const dat = create_trie(keys, NULL, GRN_OBJ_KEY_NORMALIZE);

      const char *text_rest;
      cppcut_assert_equal(2,
          grn_dat_scan(&ctx, dat, text, text_size,
                       scan_hits, max_num_scan_hits, &text_rest));
      cppcut_assert_equal(text + text_size, text_rest);
      cppcut_assert_equal(static_cast<grn_id>(1), scan_hits[0].id);
      cppcut_assert_equal(0U, scan_hits[0].offset);
      cppcut_assert_equal(9U, scan_hits[0].length);
      cppcut_assert_equal(static_cast<grn_id>(2), scan_hits[1].id);
      cppcut_assert_equal(21U, scan_hits[1].offset);
      cppcut_assert_equal(12U, scan_hits[1].length);

      cppcut_assert_equal(1,
          grn_dat_scan(&ctx, dat, text, text_size, scan_hits, 1, &text_rest));
      cppcut_assert_equal(static_cast<std::ptrdiff_t>(9), text_rest - text);
      cppcut_assert_equal(1,
          grn_dat_scan(&ctx, dat, text_rest, text_size - (text_rest - text),
                       scan_hits, 1, &text_rest));
      cppcut_assert_equal(static_cast<std::ptrdiff_t>(33), text_rest - text);
      cppcut_assert_equal(0,
          grn_dat_scan(&ctx, dat, text_rest, text_size - (text_rest - text),
                       scan_hits, 1, &text_rest));

      grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
    }

    {
      std::vector<std::string> keys;
      create_keys(&keys, 1000, 6, 15);

      grn_dat * const dat = create_trie(keys, NULL);
      for (std::size_t i = 0; i < keys.size(); ++i) {
        const grn_id key_id = static_cast<grn_id>(i + 1);
        const char * const ptr = keys[i].c_str();
        const uint32_t length = static_cast<uint32_t>(keys[i].length());
        grn_dat_scan_hit scan_hits[2];
        cppcut_assert_equal(1,
            grn_dat_scan(&ctx, dat, ptr, length, scan_hits, 2, NULL));
        cppcut_assert_equal(key_id, scan_hits[0].id);
        cppcut_assert_equal(0U, scan_hits[0].offset);
        cppcut_assert_equal(length, scan_hits[0].length);
      }
      grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
    }
  }

  void test_lcp_search(void)
  {
    {
      std::vector<std::string> keys;
      keys.push_back("012");
      keys.push_back("01234");
      keys.push_back("0123456");

      grn_dat * const dat = create_trie(keys, NULL);
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_lcp_search(&ctx, dat, "01", 2));
      cppcut_assert_equal(static_cast<grn_id>(1),
                          grn_dat_lcp_search(&ctx, dat, "012", 3));
      cppcut_assert_equal(static_cast<grn_id>(1),
                          grn_dat_lcp_search(&ctx, dat, "0123", 4));
      cppcut_assert_equal(static_cast<grn_id>(2),
                          grn_dat_lcp_search(&ctx, dat, "01234", 5));
      cppcut_assert_equal(static_cast<grn_id>(2),
                          grn_dat_lcp_search(&ctx, dat, "012345", 6));
      cppcut_assert_equal(static_cast<grn_id>(3),
                          grn_dat_lcp_search(&ctx, dat, "0123456", 7));
      cppcut_assert_equal(static_cast<grn_id>(3),
                          grn_dat_lcp_search(&ctx, dat, "0123456789", 10));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_lcp_search(&ctx, dat, "013", 3));
    }

    {
      std::vector<std::string> keys;
      create_keys(&keys, 1000, 6, 15);

      grn_dat * const dat = create_trie(keys, NULL);
      for (std::size_t i = 0; i < keys.size(); ++i) {
        const grn_id key_id = static_cast<grn_id>(i + 1);
        const char * const ptr = keys[i].c_str();
        const uint32_t length = static_cast<uint32_t>(keys[i].length());
        cppcut_assert_equal(key_id,
                            grn_dat_lcp_search(&ctx, dat, ptr, length));
      }
      grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
    }
  }

  void test_size(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    cppcut_assert_equal(static_cast<unsigned int>(keys.size()),
                        grn_dat_size(&ctx, dat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_curr_id(void)
  {
    grn_dat * const dat =
        grn_dat_create(&ctx, NULL, 0, 0, GRN_OBJ_KEY_VAR_SIZE);
    cppcut_assert_not_null(dat);

    grn_id max_key_id = 0;
    std::string key;
    for (uint32_t i = 1; i <= 1000; ++i) {
      cppcut_assert_equal(max_key_id, grn_dat_curr_id(&ctx, dat));
      create_key(&key, 1, 3);
      int added;
      const grn_id key_id =
          grn_dat_add(&ctx, dat, key.c_str(), key.length(), NULL, &added);
      if (added) {
        ++max_key_id;
        cppcut_assert_equal(max_key_id, key_id);
      }
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_truncate(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_truncate");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, dat_path);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_truncate(&ctx, dat));
    cppcut_assert_equal(static_cast<unsigned int>(0), grn_dat_size(&ctx, dat));
    cppcut_assert_equal(static_cast<grn_id>(0), grn_dat_curr_id(&ctx, dat));
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_key(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      uint32_t key_length;
      const char * const key_ptr =
          _grn_dat_key(&ctx, dat, key_id, &key_length);
      cppcut_assert_not_null(key_ptr);
      cppcut_assert_equal(length, key_length);
      cppcut_assert_equal(keys[i], std::string(key_ptr, key_length));
    }
    uint32_t key_length;
    cppcut_assert_null(_grn_dat_key(&ctx, dat, GRN_ID_NIL, &key_length));
    cppcut_assert_equal(static_cast<uint32_t>(0), key_length);
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_next(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); i += 2) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
    }
    for (std::size_t i = 0; i < (keys.size() - 1); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      if (!(i & 1)) {
        cppcut_assert_equal(key_id + 1, grn_dat_next(&ctx, dat, key_id));
      } else {
        cppcut_assert_equal(key_id + 2, grn_dat_next(&ctx, dat, key_id));
      }
    }
    cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                        grn_dat_next(&ctx, dat, keys.size()));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_at(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, NULL);
    for (std::size_t i = 0; i < keys.size(); i += 2) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      if (!(i & 1)) {
        cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                            grn_dat_at(&ctx, dat, key_id));
      } else {
        cppcut_assert_equal(key_id, grn_dat_at(&ctx, dat, key_id));
      }
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_repair(void)
  {
    char dat_path[PATH_MAX];
    std::sprintf(dat_path, "%s/%s", base_dir, "test_repair");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 6, 15);

    grn_dat * const dat = create_trie(keys, dat_path);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_repair(&ctx, dat));
    cppcut_assert_equal(static_cast<unsigned int>(keys.size()),
                        grn_dat_size(&ctx, dat));
    cppcut_assert_equal(static_cast<grn_id>(keys.size()),
                        grn_dat_curr_id(&ctx, dat));

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }

    for (std::size_t i = 0; i < keys.size(); i += 2) {
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete_by_id(&ctx, dat, i + 1, NULL));
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_repair(&ctx, dat));
    cppcut_assert_equal(static_cast<unsigned int>(keys.size() / 2),
                        grn_dat_size(&ctx, dat));

    for (std::size_t i = 0; i < keys.size(); i += 2) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      int added;
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_add(&ctx, dat, ptr, length, NULL, &added));
      cppcut_assert_equal(1, added);
    }
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_repair(&ctx, dat));
    cppcut_assert_equal(static_cast<unsigned int>(keys.size()),
                        grn_dat_size(&ctx, dat));

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(i + 1),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }
}
