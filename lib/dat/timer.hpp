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
