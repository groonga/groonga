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
#include <dat/block.hpp>

namespace test_dat_block
{
  void test_initial_values(void)
  {
    const grn::dat::Block block;

    cppcut_assert_equal(grn::dat::UInt32(0), block.next());
    cppcut_assert_equal(grn::dat::UInt32(0), block.prev());
    cppcut_assert_equal(grn::dat::UInt32(0), block.level());
    cppcut_assert_equal(grn::dat::UInt32(0), block.failure_count());
    cppcut_assert_equal(grn::dat::UInt32(0), block.first_phantom());
    cppcut_assert_equal(grn::dat::UInt32(0), block.num_phantoms());
  }

  void test_link_management(void)
  {
    grn::dat::Block block;

    block.set_next(101);
    block.set_prev(99);
    cppcut_assert_equal(grn::dat::UInt32(101), block.next());
    cppcut_assert_equal(grn::dat::UInt32(99), block.prev());
  }

  void test_level_management(void)
  {
    grn::dat::Block block;

    block.set_level(grn::dat::MAX_BLOCK_LEVEL);
    block.set_failure_count(grn::dat::MAX_FAILURE_COUNT);
    cppcut_assert_equal(grn::dat::MAX_BLOCK_LEVEL, block.level());
    cppcut_assert_equal(grn::dat::MAX_FAILURE_COUNT, block.failure_count());
  }

  void test_phantoms_management(void)
  {
    grn::dat::Block block;

    block.set_first_phantom(37);
    block.set_num_phantoms(89);
    cppcut_assert_equal(grn::dat::UInt32(37), block.first_phantom());
    cppcut_assert_equal(grn::dat::UInt32(89), block.num_phantoms());
  }
}
