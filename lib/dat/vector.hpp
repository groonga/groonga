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
