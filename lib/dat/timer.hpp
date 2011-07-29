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

#ifndef GRN_DAT_TIMER_HPP_
#define GRN_DAT_TIMER_HPP_

#include "dat.hpp"

#include <sys/time.h>

namespace grn {
namespace dat {

class Timer {
 public:
  Timer()
      : base_(get_time()) {}

  double elapsed() const {
    return get_time() - base_;
  }

  void reset() {
    base_ = get_time();
  }

 private:
  double base_;

  static double get_time() {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec * 0.000001);
  }

  // Disallows copy and assignment.
  Timer(const Timer &);
  Timer &operator=(const Timer &);
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_TIMER_HPP_
