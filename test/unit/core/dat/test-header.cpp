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
#include <dat/header.hpp>

namespace test_dat_header
{
  void test_initial_values(void)
  {
    const grn::dat::Header header;

    cppcut_assert_equal(header.file_size(), grn::dat::UInt64(0));
    cppcut_assert_equal(header.total_key_length(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.min_key_id(), grn::dat::MIN_KEY_ID);
    cppcut_assert_equal(header.next_key_id(), grn::dat::MIN_KEY_ID);
    cppcut_assert_equal(header.max_key_id(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.num_keys(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.max_num_keys(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.num_nodes(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.num_phantoms(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.num_zombies(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.max_num_nodes(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.num_blocks(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.max_num_blocks(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.next_key_pos(), grn::dat::UInt32(0));
    cppcut_assert_equal(header.key_buf_size(), grn::dat::UInt32(0));
    for (grn::dat::UInt32 i = 0; i <= grn::dat::MAX_BLOCK_LEVEL; ++i) {
      cppcut_assert_equal(header.ith_leader(i), grn::dat::INVALID_LEADER);
    }
  }

  void test_immutable_values(void)
  {
    grn::dat::Header header;

    header.set_file_size(10000);
    header.set_max_num_keys(30);
    header.set_max_num_blocks(20);
    header.set_key_buf_size(800);

    cppcut_assert_equal(header.file_size(), grn::dat::UInt64(10000));
    cppcut_assert_equal(header.max_num_keys(), grn::dat::UInt32(30));
    cppcut_assert_equal(header.max_num_nodes(), (grn::dat::BLOCK_SIZE * 20));
    cppcut_assert_equal(header.max_num_blocks(), grn::dat::UInt32(20));
    cppcut_assert_equal(header.key_buf_size(), grn::dat::UInt32(800));
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

    cppcut_assert_equal(header.total_key_length(), grn::dat::UInt32(500));
    cppcut_assert_equal(header.min_key_id(), grn::dat::MIN_KEY_ID);
    cppcut_assert_equal(header.next_key_id(), grn::dat::UInt32(15));
    cppcut_assert_equal(header.max_key_id(), grn::dat::UInt32(14));
    cppcut_assert_equal(header.num_keys(), grn::dat::UInt32(20));
    cppcut_assert_equal(header.num_nodes(), (grn::dat::BLOCK_SIZE * 10));
    cppcut_assert_equal(header.num_phantoms(), grn::dat::UInt32(200));
    cppcut_assert_equal(header.num_zombies(), grn::dat::UInt32(300));
    cppcut_assert_equal(header.num_blocks(), grn::dat::UInt32(10));
    cppcut_assert_equal(header.next_key_pos(), grn::dat::UInt32(400));

    for (grn::dat::UInt32 i = 0; i <= grn::dat::MAX_BLOCK_LEVEL; ++i) {
      header.set_ith_leader(i, i + 1);
    }

    for (grn::dat::UInt32 i = 0; i <= grn::dat::MAX_BLOCK_LEVEL; ++i) {
      cppcut_assert_equal(header.ith_leader(i), (i + 1));
    }
  }
}
