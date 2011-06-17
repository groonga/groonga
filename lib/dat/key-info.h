#ifndef GRN_DAT_KEY_INFO_H
#define GRN_DAT_KEY_INFO_H

#include "./common.h"

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

#endif  // GRN_DAT_KEY_INFO_H
