/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2011 Brazil

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

#ifndef GRN_DAT_BASE_HPP_
#define GRN_DAT_BASE_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

// The most significant bit represents whether or not the node is a terminal
// node. The BASE of a terminal node represents the ID of its associated key.
// On the other hand, the BASE of a non-terminal node represents the offset to
// its child nodes.
class Base {
 public:
  Base()
      : base_(0) {}

  UInt32 base() const {
    return base_;
  }
  bool is_terminal() const {
    return (base_ & IS_TERMINAL_FLAG) == IS_TERMINAL_FLAG;
  }
  UInt32 offset() const {
    GRN_DAT_DEBUG_THROW_IF(is_terminal());
    return base_;
  }
  UInt32 key_id() const {
    GRN_DAT_DEBUG_THROW_IF(!is_terminal());
    return base_ & ~IS_TERMINAL_FLAG;
  }

  void set_base(UInt32 x) {
    base_ = x;
  }
  void set_offset(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(x > MAX_OFFSET);
    base_ = x;
  }
  void set_key_id(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(x > MAX_KEY_ID);
    base_ = IS_TERMINAL_FLAG | x;
  }

 private:
  UInt32 base_;

  static const UInt32 IS_TERMINAL_FLAG = 0x80000000U;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_BASE_HPP_
