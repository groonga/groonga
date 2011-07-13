#ifndef GRN_DAT_KEY_HPP_
#define GRN_DAT_KEY_HPP_

#include "string.hpp"

namespace grn {
namespace dat {

class Key {
 public:
  Key()
      : str_(),
        id_(INVALID_KEY_ID) {}

  const UInt8 &operator[](UInt32 i) const {
    GRN_DAT_DEBUG_THROW_IF(i >= str_.length());
    return str_[i];
  }

  const String &str() const {
    return str_;
  }
  const void *ptr() const {
    return str_.ptr();
  }
  UInt32 length() const {
    return str_.length();
  }
  UInt32 id() const {
    return id_;
  }

  void set_str(const void *ptr, UInt32 length) {
    str_.assign(ptr, length);
  }
  void set_ptr(const void *x) {
    str_.set_ptr(x);
  }
  void set_length(UInt32 x) {
    str_.set_length(x);
  }
  void set_id(UInt32 x) {
    id_ = x;
  }

 private:
  String str_;
  UInt32 id_;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_KEY_HPP_
