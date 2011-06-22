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
