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
#include <dat/node.hpp>

#include <iostream>

namespace grn
{
  namespace dat
  {
    std::ostream &operator<<(std::ostream &stream, const grn::dat::Base &base)
    {
      if (base.is_linker()) {
        stream << "linker: " << base.key_pos();
      } else {
        stream << "non-linker: " << base.offset();
      }
      return stream;
    }

    std::ostream &operator<<(std::ostream &stream, const grn::dat::Check &check)
    {
      if (check.is_offset()) {
        stream << "offset: " << check.except_is_offset() << "; ";
      } else {
        stream << "not offset: " << check.except_is_offset() << "; ";
      }

      if (check.is_phantom()) {
        stream << "phantom: " << check.next() << ", " << check.prev();
      } else {
        stream << "non-phantom: " << check.label()
               << ", " << check.child() << ", " << check.sibling();
      }
      return stream;
    }
  }
}

namespace test_dat_node
{
  void test_base(void)
  {
    grn::dat::Node node;
    grn::dat::Base base;

    cppcut_assert_equal(base, node.base());

    node.set_key_pos(100);
    base.set_key_pos(100);
    cppcut_assert_equal(base, node.base());
    cppcut_assert_equal(base.is_linker(), node.is_linker());
    cppcut_assert_equal(base.key_pos(), node.key_pos());

    node.set_offset(1000);
    base.set_offset(1000);
    cppcut_assert_equal(base, node.base());
    cppcut_assert_equal(base.is_linker(), node.is_linker());
    cppcut_assert_equal(base.offset(), node.offset());
  }

  void test_check(void)
  {
    grn::dat::Node node;
    grn::dat::Check check;

    cppcut_assert_equal(check, node.check());

    node.set_is_offset(true);
    check.set_is_offset(true);
    cppcut_assert_equal(check, node.check());
    cppcut_assert_equal(check.is_offset(), node.is_offset());

    node.set_offset(grn::dat::INVALID_OFFSET);

    node.set_is_phantom(true);
    check.set_is_phantom(true);
    cppcut_assert_equal(check, node.check());
    cppcut_assert_equal(check.is_phantom(), node.is_phantom());

    node.set_next(101);
    node.set_prev(99);
    check.set_next(101);
    check.set_prev(99);
    cppcut_assert_equal(check, node.check());
    cppcut_assert_equal(check.next(), node.next());
    cppcut_assert_equal(check.prev(), node.prev());

    node.set_is_phantom(false);
    check.set_is_phantom(false);
    cppcut_assert_equal(check, node.check());
    cppcut_assert_equal(check.is_phantom(), node.is_phantom());
    cppcut_assert_equal(check.label(), node.label());
    cppcut_assert_equal(check.child(), node.child());
    cppcut_assert_equal(check.sibling(), node.sibling());

    node.set_label('a');
    check.set_label('a');
    cppcut_assert_equal(check, node.check());
    cppcut_assert_equal(check.label(), node.label());
  }
}
