#ifndef GRN_DAT_BLOCK_HPP_
#define GRN_DAT_BLOCK_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

class Block {
 public:
  Block()
      : next_(0),
        prev_(0),
        first_phantom_(0),
        num_phantoms_(0) {}

  UInt32 next() const {
    return next_ / BLOCK_SIZE;
  }
  UInt32 prev() const {
    return prev_ / BLOCK_SIZE;
  }

  UInt32 level() const {
    return next_ & BLOCK_MASK;
  }
  UInt32 fail_count() const {
    return prev_ & BLOCK_MASK;
  }

  UInt32 first_phantom() const {
    return first_phantom_;
  }
  UInt32 num_phantoms() const {
    return num_phantoms_;
  }

  void set_next(UInt32 x) {
    DA_DEBUG_THROW_IF(x > MAX_BLOCK_ID);
    next_ = (next_ & BLOCK_MASK) | (x * BLOCK_SIZE);
  }
  void set_prev(UInt32 x) {
    DA_DEBUG_THROW_IF(x > MAX_BLOCK_ID);
    prev_ = (prev_ & BLOCK_MASK) | (x * BLOCK_SIZE);
  }

  void set_level(UInt32 x) {
    DA_DEBUG_THROW_IF(x > MAX_BLOCK_LEVEL);
    DA_DEBUG_THROW_IF(x > BLOCK_MASK);
    next_ = (next_ & ~BLOCK_MASK) | x;
  }
  void set_fail_count(UInt32 x) {
    DA_DEBUG_THROW_IF(x > MAX_FAIL_COUNT);
    DA_DEBUG_THROW_IF(x > BLOCK_MASK);
    prev_ = (prev_ & ~BLOCK_MASK) | x;
  }

  void set_first_phantom(UInt32 x) {
    DA_DEBUG_THROW_IF(x > BLOCK_MASK);
    first_phantom_ = (UInt16)x;
  }
  void set_num_phantoms(UInt32 x) {
    DA_DEBUG_THROW_IF(x > BLOCK_SIZE);
    num_phantoms_ = (UInt16)x;
  }

 private:
  UInt32 next_;
  UInt32 prev_;
  UInt16 first_phantom_;
  UInt16 num_phantoms_;

  static const UInt32 SHIFT = 9;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_BLOCK_HPP_
