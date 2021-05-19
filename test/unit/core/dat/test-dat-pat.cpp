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
#include <grn_pat.h>

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

namespace test_dat_pat
{
  const gchar *base_dir = NULL;
  grn_ctx ctx;

  void cut_setup(void)
  {
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    base_dir = grn_test_get_tmp_dir();
    cut_remove_path(base_dir, NULL);
    g_mkdir_with_parents(base_dir, 0755);

    grn_ctx_init(&ctx, 0);
  }

  void cut_teardown(void)
  {
    grn_ctx_fin(&ctx);

    if (base_dir) {
      cut_remove_path(base_dir, NULL);
    }
  }

  void generate_pat_path(const char *filename, char *path)
  {
    std::sprintf(path, "%s/%s.pat", base_dir, filename);
  }

  void generate_dat_path(const char *filename, char *path)
  {
    std::sprintf(path, "%s/%s.dat", base_dir, filename);
  }

  grn_pat *create_pat(const char *filename,
                      const std::vector<std::string> &keys)
  {
    char pat_path[PATH_MAX];
    generate_pat_path(filename, pat_path);
    grn_pat * const pat =
        grn_pat_create(&ctx, pat_path, 1024, 0, GRN_OBJ_KEY_VAR_SIZE);
    cppcut_assert_not_null(pat);

    for (std::size_t i = 0; i < keys.size(); ++i) {
      grn_pat_add(&ctx, pat, keys[i].c_str(), keys[i].length(), NULL, NULL);
    }
    return pat;
  }

  grn_dat *create_dat(const char *filename,
                      const std::vector<std::string> &keys)
  {
    char dat_path[PATH_MAX];
    generate_dat_path(filename, dat_path);
    grn_dat * const dat =
        grn_dat_create(&ctx, dat_path, 1024, 0, GRN_OBJ_KEY_VAR_SIZE);
    cppcut_assert_not_null(dat);

    for (std::size_t i = 0; i < keys.size(); ++i) {
      grn_dat_add(&ctx, dat, keys[i].c_str(), keys[i].length(), NULL, NULL);
    }
    return dat;
  }

  void test_open(void)
  {
    const char * const filename = "test_open";

    char pat_path[PATH_MAX];
    char dat_path[PATH_MAX];

    generate_pat_path(filename, pat_path);
    generate_dat_path(filename, dat_path);

    cut_assert_not_exist_path(pat_path);
    cut_assert_not_exist_path(dat_path);

    grn_pat *pat = grn_pat_open(&ctx, pat_path);
    grn_dat *dat = grn_dat_open(&ctx, dat_path);

    cppcut_assert_null(pat);
    cppcut_assert_null(dat);

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    pat = create_pat(filename, keys);
    dat = create_dat(filename, keys);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    cut_assert_exist_path(pat_path);
    cut_assert_exist_path(dat_path);

    pat = grn_pat_open(&ctx, pat_path);
    dat = grn_dat_open(&ctx, dat_path);

    cppcut_assert_not_null(pat);
    cppcut_assert_not_null(dat);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_remove(void)
  {
    const char * const filename = "test_remove";

    std::vector<std::string> keys;
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));

    char pat_path[PATH_MAX];
    char dat_path[PATH_MAX];

    generate_pat_path(filename, pat_path);
    generate_dat_path(filename, dat_path);

    grn_test_assert(grn_pat_remove(&ctx, pat_path));
    grn_test_assert(grn_dat_remove(&ctx, dat_path));

    cut_assert_not_exist_path(pat_path);
    cut_assert_not_exist_path(dat_path);

    grn_test_assert_equal_rc(GRN_NO_SUCH_FILE_OR_DIRECTORY,
                             grn_pat_remove(&ctx, pat_path));
    grn_test_assert_equal_rc(GRN_NO_SUCH_FILE_OR_DIRECTORY,
                             grn_dat_remove(&ctx, dat_path));

  }

  void test_get(void)
  {
    const char * const filename = "test_get";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string key;
    for (int i = 0; i < 1000; ++i) {
      create_key(&key, 3, 5);

      const grn_id pat_id =
          grn_pat_get(&ctx, pat, key.c_str(), key.length(), NULL);
      const grn_id dat_id =
          grn_dat_get(&ctx, dat, key.c_str(), key.length(), NULL);
      cppcut_assert_equal(pat_id, dat_id);
      if (pat_id != GRN_ID_NIL) {
        char pat_key[1024];
        const int pat_length =
            grn_pat_get_key(&ctx, pat, pat_id, pat_key, sizeof(pat_key));
        char dat_key[1024];
        const int dat_length =
            grn_dat_get_key(&ctx, dat, dat_id, dat_key, sizeof(dat_key));
        cut_assert_equal_memory(pat_key, pat_length, dat_key, dat_length);
      }
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_add(void)
  {
    const char * const filename = "test_add";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string key;
    for (int i = 0; i < 1000; ++i) {
      create_key(&key, 3, 5);
      int pat_added = -1;
      const grn_id pat_id =
          grn_pat_add(&ctx, pat, key.c_str(), key.length(), NULL, &pat_added);
      int dat_added = -2;
      const grn_id dat_id =
          grn_dat_add(&ctx, dat, key.c_str(), key.length(), NULL, &dat_added);
      cppcut_assert_equal(pat_id, dat_id);
      cppcut_assert_equal(pat_added, dat_added);

      cppcut_assert_equal(grn_pat_size(&ctx, pat),
                          grn_dat_size(&ctx, dat));
      cppcut_assert_equal(grn_pat_curr_id(&ctx, pat),
                          grn_dat_curr_id(&ctx, dat));
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_delete_by_id(void)
  {
    const char * const filename = "test_delete_by_id";

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_pat_delete_by_id(&ctx, NULL, 1, NULL));
    ctx.rc = GRN_SUCCESS; // TODO: We should use different ctx instead of reset
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_delete_by_id(&ctx, NULL, 1, NULL));
    ctx.rc = GRN_SUCCESS; // TODO: We should use different ctx instead of reset

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
        grn_pat_delete_by_id(&ctx, pat, GRN_ID_NIL, NULL));
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
        grn_dat_delete_by_id(&ctx, dat, GRN_ID_NIL, NULL));

    for (std::size_t i = 0; i < keys.size(); i += 2) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_pat_delete_by_id(&ctx, pat, key_id, NULL));
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
    }

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_pat_delete_by_id(&ctx, pat, 1, NULL));
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_delete_by_id(&ctx, dat, 1, NULL));

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const grn_id key_id = static_cast<grn_id>(i + 1);
      const grn_rc rc = (i & 1) ? GRN_SUCCESS : GRN_INVALID_ARGUMENT;
      grn_test_assert_equal_rc(rc,
          grn_pat_delete_by_id(&ctx, pat, key_id, NULL));
      grn_test_assert_equal_rc(rc,
          grn_dat_delete_by_id(&ctx, dat, key_id, NULL));
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_delete(void)
  {
    const char * const filename = "test_delete";

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_pat_delete(&ctx, NULL, "XYZ", 3, NULL));
    ctx.rc = GRN_SUCCESS; // TODO: We should use different ctx instead of reset
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_delete(&ctx, NULL, "XYZ", 3, NULL));
    ctx.rc = GRN_SUCCESS; // TODO: We should use different ctx instead of reset

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_pat_delete(&ctx, pat, NULL, 1, NULL));
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_delete(&ctx, dat, NULL, 1, NULL));

    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_pat_delete(&ctx, pat, "XYZ", 0, NULL));
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                             grn_dat_delete(&ctx, dat, "XYZ", 0, NULL));

    for (std::size_t i = 0; i < keys.size(); i += 2) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_pat_delete(&ctx, pat, ptr, length, NULL));
      grn_test_assert_equal_rc(GRN_SUCCESS,
                               grn_dat_delete(&ctx, dat, ptr, length, NULL));
    }

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      const grn_rc rc = (i & 1) ? GRN_SUCCESS : GRN_INVALID_ARGUMENT;
      grn_test_assert_equal_rc(rc,
          grn_pat_delete(&ctx, pat, ptr, length, NULL));
      grn_test_assert_equal_rc(rc,
          grn_dat_delete(&ctx, dat, ptr, length, NULL));
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_scan(void)
  {
    const char * const filename = "test_scan";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string query;
    for (int i = 0; i < 1000; ++i) {
      create_key(&query, 8, 16);

      grn_pat_scan_hit pat_hits[4];
      const char *pat_rest;
      const int pat_num_hits =
          grn_pat_scan(&ctx, pat, query.c_str(), query.length(),
                       pat_hits, 4, &pat_rest);

      grn_dat_scan_hit dat_hits[4];
      const char *dat_rest;
      const int dat_num_hits =
          grn_dat_scan(&ctx, dat, query.c_str(), query.length(),
                       dat_hits, 4, &dat_rest);

      cppcut_assert_equal(pat_num_hits, dat_num_hits);
      for (int j = 0; j < pat_num_hits; ++j) {
        cppcut_assert_equal(pat_hits[j].id, dat_hits[j].id);
        cppcut_assert_equal(pat_hits[j].offset, dat_hits[j].offset);
        cppcut_assert_equal(pat_hits[j].length, dat_hits[j].length);
      }
      cppcut_assert_equal(pat_rest, dat_rest);
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_lcp_search(void)
  {
    const char * const filename = "test_lcp_search";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string query;
    for (int i = 0; i < 1000; ++i) {
      create_key(&query, 3, 6);

      const grn_id pat_id =
          grn_pat_lcp_search(&ctx, pat, query.c_str(), query.length());
      const grn_id dat_id =
          grn_dat_lcp_search(&ctx, dat, query.c_str(), query.length());
      cppcut_assert_equal(pat_id, dat_id);
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_id_cursor(void)
  {
    const char * const filename = "test_id_cursor";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string key;
    for (int i = 0; i < 1000; ++i) {
      grn_id min_id = static_cast<grn_id>(std::rand() % (keys.size() + 1));
      grn_id max_id = static_cast<grn_id>(std::rand() % (keys.size() + 1));
      if (!min_id) {
        min_id = GRN_ID_NIL;
      }
      if (!max_id) {
        max_id = GRN_ID_NIL;
      }
      if ((min_id != GRN_ID_NIL) && (max_id != GRN_ID_NIL) &&
          (min_id > max_id)) {
        std::swap(min_id, max_id);
      }

      const char * const min_key =
          (min_id != GRN_ID_NIL) ? keys[min_id - 1].c_str() : NULL;
      const int min_length =
          min_key ? static_cast<int>(keys[min_id - 1].length()) : 0;
      const char * const max_key =
          (max_id != GRN_ID_NIL) ? keys[max_id - 1].c_str() : NULL;
      const int max_length =
          max_key ? static_cast<int>(keys[max_id - 1].length()) : 0;

      const int temp = std::rand();
      const int offset = temp & 0x0F;
      const int limit = ((temp & 0xF0) == 0xF0) ? -1 : ((temp & 0xF0) >> 4);
      const int flags = GRN_CURSOR_BY_ID |
          (((temp & 0x100) == 0x100) ? GRN_CURSOR_LT : GRN_CURSOR_LE) |
          (((temp & 0x200) == 0x200) ? GRN_CURSOR_GT : GRN_CURSOR_GE) |
          (((temp & 0x400) == 0x400) ?
              GRN_CURSOR_DESCENDING : GRN_CURSOR_ASCENDING);

      grn_pat_cursor * const pat_cursor = grn_pat_cursor_open(
          &ctx, pat, min_key, min_length, max_key, max_length,
          offset, limit, flags);
      cppcut_assert_not_null(pat_cursor);

      grn_dat_cursor * const dat_cursor = grn_dat_cursor_open(
          &ctx, dat, min_key, min_length, max_key, max_length,
          offset, limit, flags);
      cppcut_assert_not_null(dat_cursor);

      grn_id pat_id;
      grn_id dat_id;
      do {
        pat_id = grn_pat_cursor_next(&ctx, pat_cursor);
        dat_id = grn_dat_cursor_next(&ctx, dat_cursor);
        cppcut_assert_equal(pat_id, dat_id);
      } while (pat_id != GRN_ID_NIL);

      grn_pat_cursor_close(&ctx, pat_cursor);
      grn_dat_cursor_close(&ctx, dat_cursor);
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_key_cursor(void)
  {
    const char * const filename = "test_key_cursor";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string min_str;
    std::string max_str;
    for (int i = 0; i < 1000; ++i) {
      create_key(&min_str, 3, 5);
      create_key(&max_str, 3, 5);
      if (min_str > max_str) {
        min_str.swap(max_str);
      }

      const int temp = std::rand();
      const int offset = temp & 0x0F;
      const int limit = ((temp & 0xF0) == 0xF0) ? -1 : ((temp & 0xF0) >> 4);
      const int flags = GRN_CURSOR_BY_KEY |
          (((temp & 0x100) == 0x100) ? GRN_CURSOR_LT : GRN_CURSOR_LE) |
          (((temp & 0x200) == 0x200) ? GRN_CURSOR_GT : GRN_CURSOR_GE) |
          (((temp & 0x400) == 0x400) ?
              GRN_CURSOR_DESCENDING : GRN_CURSOR_ASCENDING);

      const bool disables_min = !(rand() % 32);
      const bool disables_max = !(rand() % 32);

      grn_pat_cursor * const pat_cursor =
          grn_pat_cursor_open(&ctx, pat, disables_min ? NULL : min_str.c_str(),
                              disables_min ? 0 : min_str.length(),
                              disables_max ? NULL : max_str.c_str(),
                              disables_max ? 0 : max_str.length(),
                              offset, limit, flags);
      cppcut_assert_not_null(pat_cursor);

      grn_dat_cursor * const dat_cursor =
          grn_dat_cursor_open(&ctx, dat, disables_min ? NULL : min_str.c_str(),
                              disables_min ? 0 : min_str.length(),
                              disables_max ? NULL : max_str.c_str(),
                              disables_max ? 0 : max_str.length(),
                              offset, limit, flags);
      cppcut_assert_not_null(dat_cursor);

      grn_id pat_id;
      grn_id dat_id;
      do {
        pat_id = grn_pat_cursor_next(&ctx, pat_cursor);
        dat_id = grn_dat_cursor_next(&ctx, dat_cursor);
        cppcut_assert_equal(pat_id, dat_id);
      } while (pat_id != GRN_ID_NIL);

      grn_pat_cursor_close(&ctx, pat_cursor);
      grn_dat_cursor_close(&ctx, dat_cursor);
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_prefix_cursor(void)
  {
    const char * const filename = "test_prefix_cursor";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string max_str;
    for (int i = 0; i < 1000; ++i) {
      create_key(&max_str, 3, 5);

      const int temp = std::rand();
      const int offset = temp & 0x03;
      const int limit = ((temp & 0xF0) == 0xF0) ? -1 : ((temp & 0xF0) >> 4);
      const int flags = GRN_CURSOR_PREFIX |
          (((temp & 0x100) == 0x100) ? GRN_CURSOR_LT : GRN_CURSOR_LE) |
          (((temp & 0x200) == 0x200) ? GRN_CURSOR_GT : GRN_CURSOR_GE) |
          (((temp & 0x400) == 0x400) ?
              GRN_CURSOR_DESCENDING : GRN_CURSOR_ASCENDING);

      const int min_length = std::rand() % (max_str.length() + 1);

      grn_pat_cursor * const pat_cursor =
          grn_pat_cursor_open(&ctx, pat, NULL, min_length, max_str.c_str(),
                              max_str.length(), offset, limit, flags);
      cppcut_assert_not_null(pat_cursor);

      grn_dat_cursor * const dat_cursor =
          grn_dat_cursor_open(&ctx, dat, NULL, min_length, max_str.c_str(),
                              max_str.length(), offset, limit, flags);
      cppcut_assert_not_null(dat_cursor);

      grn_id pat_id;
      grn_id dat_id;
      do {
        pat_id = grn_pat_cursor_next(&ctx, pat_cursor);
        dat_id = grn_dat_cursor_next(&ctx, dat_cursor);
        cppcut_assert_equal(pat_id, dat_id);
      } while (pat_id != GRN_ID_NIL);

      grn_pat_cursor_close(&ctx, pat_cursor);
      grn_dat_cursor_close(&ctx, dat_cursor);
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_predictive_cursor(void)
  {
    const char * const filename = "test_predictive_cursor";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    std::string min_str;
    for (int i = 0; i < 1000; ++i) {
      create_key(&min_str, 3, 5);

      const int temp = std::rand();
      const int offset = temp & 0x0F;
      const int limit = ((temp & 0xF0) == 0xF0) ? -1 : ((temp & 0xF0) >> 4);
      const int flags = GRN_CURSOR_PREFIX |
          (((temp & 0x100) == 0x100) ? GRN_CURSOR_LT : GRN_CURSOR_LE) |
          (((temp & 0x200) == 0x200) ? GRN_CURSOR_GT : GRN_CURSOR_GE) |
          (((temp & 0x400) == 0x400) ?
              GRN_CURSOR_DESCENDING : GRN_CURSOR_ASCENDING);

      grn_pat_cursor * const pat_cursor =
          grn_pat_cursor_open(&ctx, pat, min_str.c_str(), min_str.length(),
                              NULL, 0, offset, limit, flags);
      cppcut_assert_not_null(pat_cursor);

      grn_dat_cursor *dat_cursor =
          grn_dat_cursor_open(&ctx, dat, min_str.c_str(), min_str.length(),
                              NULL, 0, offset, limit, flags);
      cppcut_assert_not_null(dat_cursor);

      grn_id pat_id;
      grn_id dat_id;
      do {
        pat_id = grn_pat_cursor_next(&ctx, pat_cursor);
        dat_id = grn_dat_cursor_next(&ctx, dat_cursor);
        cppcut_assert_equal(pat_id, dat_id);
      } while (pat_id != GRN_ID_NIL);

      grn_pat_cursor_close(&ctx, pat_cursor);
      grn_dat_cursor_close(&ctx, dat_cursor);
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }

  void test_truncate(void)
  {
    const char * const filename = "test_truncate";

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 3, 5);
    grn_pat * const pat = create_pat(filename, keys);
    grn_dat * const dat = create_dat(filename, keys);

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_truncate(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_truncate(&ctx, dat));

    cppcut_assert_equal(static_cast<unsigned int>(0), grn_pat_size(&ctx, pat));
    cppcut_assert_equal(static_cast<unsigned int>(0), grn_dat_size(&ctx, dat));

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_pat_get(&ctx, pat, ptr, length, NULL));
      cppcut_assert_equal(static_cast<grn_id>(GRN_ID_NIL),
                          grn_dat_get(&ctx, dat, ptr, length, NULL));
    }

    grn_test_assert_equal_rc(GRN_SUCCESS, grn_pat_close(&ctx, pat));
    grn_test_assert_equal_rc(GRN_SUCCESS, grn_dat_close(&ctx, dat));
  }
}
