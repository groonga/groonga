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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gcutter.h>
#include <cppcutter.h>

#include <grn-assertions.h>
#include <dat/header.hpp>

namespace test_dat_header
{
  void test_initial_values(void)
  {
    const grn::dat::Header header;

    cppcut_assert_equal(grn::dat::UInt64(0), header.file_size());
    cppcut_assert_equal(grn::dat::UInt32(0), header.total_key_length());
    cppcut_assert_equal(grn::dat::MIN_KEY_ID, header.min_key_id());
    cppcut_assert_equal(grn::dat::MIN_KEY_ID, header.next_key_id());
    cppcut_assert_equal(grn::dat::UInt32(0), header.max_key_id());
    cppcut_assert_equal(grn::dat::UInt32(0), header.num_keys());
    cppcut_assert_equal(grn::dat::UInt32(0), header.max_num_keys());
    cppcut_assert_equal(grn::dat::UInt32(0), header.num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(0), header.num_phantoms());
    cppcut_assert_equal(grn::dat::UInt32(0), header.num_zombies());
    cppcut_assert_equal(grn::dat::UInt32(0), header.max_num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(0), header.num_blocks());
    cppcut_assert_equal(grn::dat::UInt32(0), header.max_num_blocks());
    cppcut_assert_equal(grn::dat::UInt32(0), header.next_key_pos());
    cppcut_assert_equal(grn::dat::UInt32(0), header.key_buf_size());
    for (grn::dat::UInt32 i = 0; i <= grn::dat::MAX_BLOCK_LEVEL; ++i) {
      cppcut_assert_equal(grn::dat::INVALID_LEADER, header.ith_leader(i));
    }
  }

  void test_immutable_values(void)
  {
    grn::dat::Header header;

    header.set_file_size(10000);
    header.set_max_num_keys(30);
    header.set_max_num_blocks(20);
    header.set_key_buf_size(800);

    cppcut_assert_equal(grn::dat::UInt64(10000), header.file_size());
    cppcut_assert_equal(grn::dat::UInt32(30), header.max_num_keys());
    cppcut_assert_equal(grn::dat::BLOCK_SIZE * 20, header.max_num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(20), header.max_num_blocks());
    cppcut_assert_equal(grn::dat::UInt32(800), header.key_buf_size());
  }

  void test_mutable_values(void)
  {
    grn::dat::Header header;

    header.set_file_size(1000000);
    header.set_max_num_keys(100);
    header.set_max_num_blocks(50);
    header.set_key_buf_size(100000);

    header.set_total_key_length(500);
    header.set_next_key_id(15);
    header.set_max_key_id(14);
    header.set_num_keys(20);
    header.set_num_phantoms(200);
    header.set_num_zombies(300);
    header.set_num_blocks(10);
    header.set_next_key_pos(400);

    cppcut_assert_equal(grn::dat::UInt32(500), header.total_key_length());
    cppcut_assert_equal(grn::dat::MIN_KEY_ID, header.min_key_id());
    cppcut_assert_equal(grn::dat::UInt32(15), header.next_key_id());
    cppcut_assert_equal(grn::dat::UInt32(14), header.max_key_id());
    cppcut_assert_equal(grn::dat::UInt32(20), header.num_keys());
    cppcut_assert_equal(grn::dat::BLOCK_SIZE * 10, header.num_nodes());
    cppcut_assert_equal(grn::dat::UInt32(200), header.num_phantoms());
    cppcut_assert_equal(grn::dat::UInt32(300), header.num_zombies());
    cppcut_assert_equal(grn::dat::UInt32(10), header.num_blocks());
    cppcut_assert_equal(grn::dat::UInt32(400), header.next_key_pos());

    for (grn::dat::UInt32 i = 0; i <= grn::dat::MAX_BLOCK_LEVEL; ++i) {
      header.set_ith_leader(i, i + 1);
    }

    for (grn::dat::UInt32 i = 0; i <= grn::dat::MAX_BLOCK_LEVEL; ++i) {
      cppcut_assert_equal(i + 1, header.ith_leader(i));
    }
  }
}
