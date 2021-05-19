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
#include <glib/gstdio.h>
#include <cppcutter.h>

#include <grn-assertions.h>
#include <dat/trie.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "test-string.hpp"

namespace
{
  class Thread {
   public:
    Thread() : thread_(NULL) {}
    ~Thread() {
      join();
    }

    void create(GThreadFunc func, gpointer data) {
      join();

      thread_ = g_thread_new("test-trie", func, data);
    }

    void join() {
      if (thread_) {
        g_thread_join(thread_);
        thread_ = NULL;
      }
    }

   private:
    GThread *thread_;

    // Disallows copy and assignment.
    Thread(const Thread &);
    Thread &operator=(const Thread &);
  };

  struct Context {
    CutTestContext *cut_test_context;
    grn::dat::Trie *trie;
    bool continue_flag;
  };

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

namespace test_dat_trie
{
  const gchar *base_dir = NULL;

  void cut_setup(void)
  {
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    base_dir = grn_test_get_tmp_dir();
    cut_remove_path(base_dir, NULL);
    g_mkdir_with_parents(base_dir, 0755);
  }

  void cut_teardown(void)
  {
    if (base_dir) {
      cut_remove_path(base_dir, NULL);
    }
  }

  void test_empty_trie(void)
  {
    grn::dat::Trie trie;
    trie.create();

    cppcut_assert_equal(grn::dat::DEFAULT_FILE_SIZE, trie.file_size());
    cppcut_assert_equal(grn::dat::UInt64(4236), trie.virtual_size());
    cppcut_assert_equal(grn::dat::UInt32(0), trie.total_key_length());
    cppcut_assert_equal(grn::dat::UInt32(0), trie.num_keys());
    cppcut_assert_equal(grn::dat::UInt32(1), trie.min_key_id());
    cppcut_assert_equal(grn::dat::UInt32(1), trie.next_key_id());
    cppcut_assert_equal(grn::dat::UInt32(0), trie.max_key_id());
    cppcut_assert_equal(grn::dat::UInt32(0), trie.num_keys());
    cppcut_assert_equal(grn::dat::UInt32(17893), trie.max_num_keys());
    cppcut_assert_equal(grn::dat::UInt32(512), trie.num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(511), trie.num_phantoms());
    cppcut_assert_equal(grn::dat::UInt32(0), trie.num_zombies());
    cppcut_assert_equal(grn::dat::UInt32(71680), trie.max_num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(1), trie.num_blocks());
    cppcut_assert_equal(grn::dat::UInt32(140), trie.max_num_blocks());
    cppcut_assert_equal(grn::dat::UInt32(0), trie.next_key_pos());
    cppcut_assert_equal(grn::dat::UInt32(100439), trie.key_buf_size());
  }

  void test_trie_on_memory(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create();

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(true,
          trie.search(keys[i].c_str(), keys[i].length()));
    }
  }

  void test_trie_on_file(void)
  {
    char trie_path[PATH_MAX];
    std::strcpy(trie_path, base_dir);
    std::strcat(trie_path, "/test_trie_on_file.dat");

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create(trie_path);

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(true,
          trie.search(keys[i].c_str(), keys[i].length()));
    }

    grn::dat::Trie new_trie;
    new_trie.open(trie_path);

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(false,
          new_trie.insert(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(true,
          new_trie.search(keys[i].c_str(), keys[i].length()));
    }
  }

  void test_insert(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create();

    std::size_t total_key_length = 0;
    for (std::size_t i = 0; i < keys.size(); ++i) {
      grn::dat::UInt32 key_pos;
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length(), &key_pos));

      const grn::dat::Key &key = trie.get_key(key_pos);
      cppcut_assert_equal(true, key.is_valid());
      cppcut_assert_equal(static_cast<grn::dat::UInt32>(i + 1), key.id());
      cppcut_assert_equal(static_cast<grn::dat::UInt32>(keys[i].length()),
                          key.length());
      cppcut_assert_equal(0,
          std::memcmp(key.ptr(), keys[i].c_str(), key.length()));

      grn::dat::UInt32 key_pos_again;
      cppcut_assert_equal(false,
          trie.insert(keys[i].c_str(), keys[i].length(), &key_pos_again));
      cppcut_assert_equal(key_pos, key_pos_again);

      total_key_length += keys[i].length();
      cppcut_assert_equal(total_key_length,
                          static_cast<std::size_t>(trie.total_key_length()));
    }
  }

  void test_ith_key(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create();

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(false, trie.ith_key(i + 1).is_valid());

      grn::dat::UInt32 key_pos;
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length(), &key_pos));

      const grn::dat::Key &key_by_pos = trie.get_key(key_pos);
      const grn::dat::Key &key_by_id = trie.ith_key(i + 1);
      cppcut_assert_equal(&key_by_pos, &key_by_id);
    }
  }

  void test_search(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create();

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(false,
          trie.search(keys[i].c_str(), keys[i].length()));

      grn::dat::UInt32 key_pos_inserted;
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length(), &key_pos_inserted));

      grn::dat::UInt32 key_pos_found;
      cppcut_assert_equal(true,
          trie.search(keys[i].c_str(), keys[i].length(), &key_pos_found));
      cppcut_assert_equal(key_pos_inserted, key_pos_found);
    }
  }

  void test_lcp_search(void)
  {
    grn::dat::Trie trie;
    trie.create();

    cppcut_assert_equal(true, trie.insert("012", 3));
    cppcut_assert_equal(true, trie.insert("01234", 5));
    cppcut_assert_equal(true, trie.insert("0123456", 7));

    cppcut_assert_equal(false, trie.lcp_search("01", 2));
    cppcut_assert_equal(true, trie.lcp_search("012", 3));
    cppcut_assert_equal(true, trie.lcp_search("0123", 4));
    cppcut_assert_equal(false, trie.lcp_search("12345", 5));

    grn::dat::UInt32 key_pos = grn::dat::MAX_UINT32;

    cppcut_assert_equal(false, trie.lcp_search("01", 2, &key_pos));
    cppcut_assert_equal(grn::dat::MAX_UINT32, key_pos);

    cppcut_assert_equal(true, trie.lcp_search("012", 3, &key_pos));
    cppcut_assert_equal(true, trie.get_key(key_pos).is_valid());
    cppcut_assert_equal(static_cast<grn::dat::UInt32>(1),
                        trie.get_key(key_pos).id());

    cppcut_assert_equal(true, trie.lcp_search("012345", 6, &key_pos));
    cppcut_assert_equal(true, trie.get_key(key_pos).is_valid());
    cppcut_assert_equal(static_cast<grn::dat::UInt32>(2),
                        trie.get_key(key_pos).id());

    cppcut_assert_equal(true, trie.lcp_search("0123456789", 10, &key_pos));
    cppcut_assert_equal(true, trie.get_key(key_pos).is_valid());
    cppcut_assert_equal(static_cast<grn::dat::UInt32>(3),
                        trie.get_key(key_pos).id());

    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const char * const ptr = keys[i].c_str();
      const uint32_t length = static_cast<uint32_t>(keys[i].length());
      grn::dat::UInt32 key_pos_inserted;
      trie.insert(ptr, length, &key_pos_inserted);

      grn::dat::UInt32 key_pos_found;
      cppcut_assert_equal(true,
                          trie.lcp_search(ptr, length, &key_pos_found));
      cppcut_assert_equal(key_pos_inserted, key_pos_found);
    }
  }

  void test_remove(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create();

    std::size_t total_key_length = 0;
    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length()));
      total_key_length += keys[i].length();
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(static_cast<grn::dat::UInt32>(keys.size() - i),
                          trie.num_keys());
      cppcut_assert_equal(true, trie.remove(i + 1));
      cppcut_assert_equal(false, trie.ith_key(i + 1).is_valid());
      cppcut_assert_equal(false,
          trie.search(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(false, trie.remove(i + 1));

      total_key_length -= keys[i].length();
      cppcut_assert_equal(total_key_length, static_cast<std::size_t>(trie.total_key_length()));
    }

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length()));
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          trie.remove(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(false, trie.ith_key(keys.size() - i).is_valid());
      cppcut_assert_equal(false,
          trie.search(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(false,
          trie.remove(keys[i].c_str(), keys[i].length()));
    }
  }

  void test_update(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie trie;
    trie.create();

    std::size_t total_key_length = 0;
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      cppcut_assert_equal(true,
          trie.insert(keys[i].c_str(), keys[i].length()));
      total_key_length += keys[i].length();
    }
    for (std::size_t i = (keys.size() / 2); i < keys.size(); ++i) {
      const grn::dat::UInt32 key_id = i + 1 - (keys.size() / 2);
      const std::string &src_key = keys[i - (keys.size() / 2)];
      cppcut_assert_equal(true,
          trie.update(key_id, keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(true, trie.ith_key(key_id).is_valid());
      cppcut_assert_equal(true,
          trie.search(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(false,
          trie.search(src_key.c_str(), src_key.length()));

      total_key_length += keys[i].length() - src_key.length();
      cppcut_assert_equal(total_key_length, static_cast<std::size_t>(trie.total_key_length()));
    }
    for (std::size_t i = 0; i < (keys.size() / 2); ++i) {
      const std::string &src_key = keys[i + (keys.size() / 2)];
      cppcut_assert_equal(true,
          trie.update(src_key.c_str(), src_key.length(),
                      keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(true, trie.ith_key(i + 1).is_valid());
      cppcut_assert_equal(true,
          trie.search(keys[i].c_str(), keys[i].length()));
      cppcut_assert_equal(false,
          trie.search(src_key.c_str(), src_key.length()));

      total_key_length += keys[i].length() - src_key.length();
      cppcut_assert_equal(total_key_length,
                          static_cast<std::size_t>(trie.total_key_length()));
    }
  }

  void test_create(void)
  {
    std::vector<std::string> keys;
    create_keys(&keys, 1000, 1, 16);

    grn::dat::Trie src_trie;
    src_trie.create();

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          src_trie.insert(keys[i].c_str(), keys[i].length()));
    }

    grn::dat::Trie dest_trie;
    dest_trie.create(src_trie);

    for (std::size_t i = 0; i < keys.size(); ++i) {
      cppcut_assert_equal(true,
          dest_trie.search(keys[i].c_str(), keys[i].length()));
    }

    cppcut_assert_equal(src_trie.file_size(), dest_trie.file_size());
    cppcut_assert_equal(src_trie.total_key_length(),
                        dest_trie.total_key_length());
    cppcut_assert_equal(src_trie.min_key_id(), dest_trie.min_key_id());
    cppcut_assert_equal(src_trie.next_key_id(), dest_trie.next_key_id());
    cppcut_assert_equal(src_trie.max_key_id(), dest_trie.max_key_id());
    cppcut_assert_equal(src_trie.num_keys(), dest_trie.num_keys());
    cppcut_assert_equal(src_trie.next_key_pos(), dest_trie.next_key_pos());

    cppcut_assert_operator(dest_trie.num_nodes(), <, src_trie.num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(0), dest_trie.num_zombies());
    cppcut_assert_operator(dest_trie.num_blocks(), <, src_trie.num_nodes());
  }

  void test_random_queries(void)
  {
    typedef std::stack<grn::dat::UInt32> Stack;
    typedef std::map<std::string, grn::dat::UInt32> Keyset;

    grn::dat::Trie trie;
    trie.create(NULL, 1 << 16);

    Keyset keyset;
    Stack stack;
    std::string str;
    for (std::size_t i = 0; i < 1000; ++i) {
      create_key(&str, 2, 3);
      switch (static_cast<int>(4.0 * std::rand() / RAND_MAX)) {
        case 0: {
          const Keyset::const_iterator it = keyset.find(str);
          const bool to_be_found = (it != keyset.end());
          grn::dat::UInt32 key_pos;
          const bool is_found =
              trie.search(str.c_str(), str.length(), &key_pos);
          cppcut_assert_equal(to_be_found, is_found);
          if (is_found) {
            const grn::dat::Key &key = trie.get_key(key_pos);
            cppcut_assert_equal(it->second, key.id());
            cppcut_assert_equal(static_cast<grn::dat::UInt32>(str.length()),
                                key.length());
            cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                                key.str());
          }
          break;
        }
        case 1: {
          const Keyset::iterator it = keyset.find(str);
          const bool to_be_removed = (it != keyset.end());
          const bool is_removed = trie.remove(str.c_str(), str.length());
          cppcut_assert_equal(to_be_removed, is_removed);
          if (is_removed) {
            stack.push(it->second);
            keyset.erase(it);
          }
          break;
        }
        case 2: {
          const grn::dat::UInt32 key_id =
              stack.empty() ? (keyset.size() + 1) : stack.top();
          const std::pair<Keyset::iterator, bool> it_bool_pair =
              keyset.insert(std::make_pair(str, key_id));
          const bool to_be_inserted = it_bool_pair.second;
          if (!stack.empty() && to_be_inserted) {
            stack.pop();
          }
          grn::dat::UInt32 key_pos;
          bool is_inserted = !to_be_inserted;
          try {
            is_inserted = trie.insert(str.c_str(), str.length(), &key_pos);
          } catch (const grn::dat::SizeError &ex) {
            trie.create(trie, NULL, trie.file_size() * 2);
            is_inserted = trie.insert(str.c_str(), str.length(), &key_pos);
          }
          cppcut_assert_equal(to_be_inserted, is_inserted);
          const grn::dat::Key &key = trie.get_key(key_pos);
          cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                              key.str());
          break;
        }
        default: {
          const grn::dat::UInt32 key_id = (trie.max_key_id()) ?
              ((std::rand() % trie.max_key_id()) + 1) : 0;
          const grn::dat::Key &src_key = trie.ith_key(key_id);
          const Keyset::iterator src_it = !src_key.is_valid() ? keyset.end() :
              keyset.find(std::string(
                  static_cast<const char *>(src_key.ptr()), src_key.length()));
          cppcut_assert_equal(src_it != keyset.end(), src_key.is_valid());
          const bool to_be_updated = src_key.is_valid() &&
              (keyset.find(str) == keyset.end());
          grn::dat::UInt32 key_pos;
          bool is_updated = !to_be_updated;
          try {
            is_updated = trie.update(key_id, str.c_str(), str.length(),
                                     &key_pos);
          } catch (const grn::dat::SizeError &ex) {
            trie.create(trie, NULL, trie.file_size() * 2);
            is_updated = trie.update(key_id, str.c_str(), str.length(),
                                     &key_pos);
          }
          cppcut_assert_equal(to_be_updated, is_updated);
          if (is_updated) {
            const grn::dat::Key &key = trie.get_key(key_pos);
            cppcut_assert_equal(key_id, key.id());
            cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                                key.str());
            keyset.erase(src_it);
            keyset.insert(std::make_pair(str, key_id));
          }
          break;
        }
      }
    }
  }

  gpointer sub_test_multi_threaded_random_queries(gpointer data)
  {
    volatile Context * const context = static_cast<Context *>(data);
    cut_set_current_test_context(context->cut_test_context);

    std::string str;
    while (context->continue_flag) try {
      const grn::dat::Trie * const trie = context->trie;
      create_key(&str, 2, 3);
      grn::dat::UInt32 key_pos;
      if (trie->search(str.c_str(), str.length(), &key_pos)) {
        const grn::dat::Key &key = trie->get_key(key_pos);
        cppcut_assert_equal(str.length(),
                            static_cast<std::size_t>(key.length()));
        cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                            key.str());
      }
    } catch (...) {
      cut_fail("sub_test_multi_threaded_random_queries() failed.");
    }
    return NULL;
  }

  void test_multi_threaded_random_queries(void)
  {
    typedef std::stack<grn::dat::UInt32> Stack;
    typedef std::map<std::string, grn::dat::UInt32> Keyset;

    grn::dat::Trie tries[32];
    grn::dat::Trie *trie = &tries[0];
    trie->create(NULL, 1 << 16);

    Context context;
    context.cut_test_context = cut_get_current_test_context();
    context.trie = trie;
    context.continue_flag = true;

    Thread threads[2];
    for (std::size_t i = 0; i < (sizeof(threads) / sizeof(*threads)); ++i) {
      threads[i].create(sub_test_multi_threaded_random_queries, &context);
    }

    Keyset keyset;
    Stack stack;
    std::string str;
    for (std::size_t i = 0; i < 1000; ++i) {
      create_key(&str, 2, 3);
      switch (static_cast<int>(4.0 * std::rand() / RAND_MAX)) {
        case 0: {
          const Keyset::const_iterator it = keyset.find(str);
          const bool to_be_found = (it != keyset.end());
          grn::dat::UInt32 key_pos;
          const bool is_found =
              trie->search(str.c_str(), str.length(), &key_pos);
          cppcut_assert_equal(to_be_found, is_found);
          if (is_found) {
            const grn::dat::Key &key = trie->get_key(key_pos);
            cppcut_assert_equal(it->second, key.id());
            cppcut_assert_equal(static_cast<grn::dat::UInt32>(str.length()),
                                key.length());
            cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                                key.str());
          }
          break;
        }
        case 1: {
          const Keyset::iterator it = keyset.find(str);
          const bool to_be_removed = (it != keyset.end());
          const bool is_removed = trie->remove(str.c_str(), str.length());
          cppcut_assert_equal(to_be_removed, is_removed);
          if (is_removed) {
            stack.push(it->second);
            keyset.erase(it);
          }
          break;
        }
        case 2: {
          const grn::dat::UInt32 key_id =
              stack.empty() ? (keyset.size() + 1) : stack.top();
          const std::pair<Keyset::iterator, bool> it_bool_pair =
              keyset.insert(std::make_pair(str, key_id));
          const bool to_be_inserted = it_bool_pair.second;
          if (!stack.empty() && to_be_inserted) {
            stack.pop();
          }
          grn::dat::UInt32 key_pos;
          bool is_inserted = !to_be_inserted;
          try {
            is_inserted = trie->insert(str.c_str(), str.length(), &key_pos);
          } catch (const grn::dat::SizeError &ex) {
            (trie + 1)->create(*trie, NULL, trie->file_size() * 2);
            context.trie = ++trie;
            is_inserted = trie->insert(str.c_str(), str.length(), &key_pos);
          }
          cppcut_assert_equal(to_be_inserted, is_inserted);
          const grn::dat::Key &key = trie->get_key(key_pos);
          cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                              key.str());
          break;
        }
        default: {
          const grn::dat::UInt32 key_id = (trie->max_key_id()) ?
              ((std::rand() % trie->max_key_id()) + 1) : 0;
          const grn::dat::Key &src_key = trie->ith_key(key_id);
          const Keyset::iterator src_it = !src_key.is_valid() ? keyset.end() :
              keyset.find(std::string(
                  static_cast<const char *>(src_key.ptr()), src_key.length()));
          cppcut_assert_equal(src_it != keyset.end(), src_key.is_valid());
          const bool to_be_updated = src_key.is_valid() &&
              (keyset.find(str) == keyset.end());
          grn::dat::UInt32 key_pos;
          bool is_updated = !to_be_updated;
          try {
            is_updated = trie->update(key_id, str.c_str(), str.length(),
                                      &key_pos);
          } catch (const grn::dat::SizeError &ex) {
            (trie + 1)->create(*trie, NULL, trie->file_size() * 2);
            context.trie = ++trie;
            is_updated = trie->update(key_id, str.c_str(), str.length(),
                                      &key_pos);
          }
          cppcut_assert_equal(to_be_updated, is_updated);
          if (is_updated) {
            const grn::dat::Key &key = trie->get_key(key_pos);
            cppcut_assert_equal(key_id, key.id());
            cppcut_assert_equal(grn::dat::String(str.c_str(), str.length()),
                                key.str());
            keyset.erase(src_it);
            keyset.insert(std::make_pair(str, key_id));
          }
          break;
        }
      }
    }
    context.continue_flag = false;
  }
}
