#ifndef GRN_DAT_KEY_HPP_
#define GRN_DAT_KEY_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

class Key {
 public:
  const char &operator[](UInt32 i) const {
    GRN_DAT_DEBUG_THROW_IF(i > length_);
    return ptr_[i];
  }

  const char *ptr() const {
    return ptr_;
  }
  UInt32 length() const {
    return length_;
  }
  UInt32 id() const {
    return id_;
  }

  void set_ptr(const char *x) {
    ptr_ = x;
  }
  void set_length(UInt32 x) {
    length_ = x;
  }
  void set_id(UInt32 x) {
    id_ = x;
  }

 private:
  const char *ptr_;
  UInt32 length_;
  UInt32 id_;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_KEY_HPP_
