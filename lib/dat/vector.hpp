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

#ifndef GRN_DAT_VECTOR_HPP_
#define GRN_DAT_VECTOR_HPP_

#include "dat.hpp"

#include <vector>

namespace grn {
namespace dat {

template <typename T>
class Vector {
 public:
  Vector()
      : buf_() {}
  ~Vector() {}

  const T &operator[](UInt32 i) const {
    return buf_[i];
  }
  T &operator[](UInt32 i) {
    return buf_[i];
  }

  const T &back() const {
    return buf_.back();
  }
  T &back() {
    return buf_.back();
  }

  bool push_back(const T &x) try {
    buf_.push_back(x);
    return true;
  } catch (...) {
    return false;
  }

  void pop_back() {
    buf_.pop_back();
  }

  void swap(Vector *rhs) {
    buf_.swap(rhs->buf_);
  }

  bool empty() const {
    return buf_.empty();
  }

  UInt32 size() const {
    return static_cast<UInt32>(buf_.size());
  }

 private:
  std::vector<T> buf_;

  // Disallows copy and assignment.
  Vector(const Vector &);
  Vector &operator=(const Vector &);
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_VECTOR_HPP_
