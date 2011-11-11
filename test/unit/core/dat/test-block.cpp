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
#include <dat/block.hpp>

namespace test_dat_block
{
  void test_initial_values(void)
  {
    const grn::dat::Block block;

    cppcut_assert_equal(block.next(), grn::dat::UInt32(0));
    cppcut_assert_equal(block.prev(), grn::dat::UInt32(0));
    cppcut_assert_equal(block.level(), grn::dat::UInt32(0));
    cppcut_assert_equal(block.failure_count(), grn::dat::UInt32(0));
    cppcut_assert_equal(block.first_phantom(), grn::dat::UInt32(0));
    cppcut_assert_equal(block.num_phantoms(), grn::dat::UInt32(0));
  }

  void test_link_management(void)
  {
    grn::dat::Block block;

    block.set_next(101);
    block.set_prev(99);
    cppcut_assert_equal(block.next(), grn::dat::UInt32(101));
    cppcut_assert_equal(block.prev(), grn::dat::UInt32(99));
  }

  void test_level_management(void)
  {
    grn::dat::Block block;

    block.set_level(grn::dat::MAX_BLOCK_LEVEL);
    block.set_failure_count(grn::dat::MAX_FAILURE_COUNT);
    cppcut_assert_equal(block.level(), grn::dat::MAX_BLOCK_LEVEL);
    cppcut_assert_equal(block.failure_count(), grn::dat::MAX_FAILURE_COUNT);
  }

  void test_phantoms_management(void)
  {
    grn::dat::Block block;

    block.set_first_phantom(37);
    block.set_num_phantoms(89);
    cppcut_assert_equal(block.first_phantom(), grn::dat::UInt32(37));
    cppcut_assert_equal(block.num_phantoms(), grn::dat::UInt32(89));
  }
}
