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

#ifndef GRN_DAT_KEY_INFO_HPP_
#define GRN_DAT_KEY_INFO_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

class KeyInfo {
 public:
  KeyInfo()
      : offset_(0) {}

  UInt32 offset() const {
    return offset_;
  }

  void set_offset(UInt32 x) {
    offset_ = x;
  }

 private:
  UInt32 offset_;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_KEY_INFO_HPP_
