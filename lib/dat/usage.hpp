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

#ifndef GRN_DAT_USAGE_HPP_
#define GRN_DAT_USAGE_HPP_

#include "dat.hpp"

#include <sys/time.h>
#include <sys/resource.h>

namespace grn {
namespace dat {

class Usage {
 public:
  Usage()
      : usage_() {
    ::getrusage(RUSAGE_SELF, &usage_);
  }

  double user_time() const {
    return usage_.ru_utime.tv_sec + (usage_.ru_utime.tv_usec * 0.000001);
  }
  double system_time() const {
    return usage_.ru_stime.tv_sec + (usage_.ru_stime.tv_usec * 0.000001);
  }
  long max_resident_set_size() const {
    return usage_.ru_maxrss;
  }
  long minor_page_faults() const {
    return usage_.ru_minflt;
  }
  long major_page_faults() const {
    return usage_.ru_majflt;
  }
  long file_system_inputs() const {
    return usage_.ru_inblock;
  }
  long file_system_outputs() const {
    return usage_.ru_oublock;
  }

 private:
  struct rusage usage_;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_USAGE_HPP_
