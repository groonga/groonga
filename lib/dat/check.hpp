#ifndef GRN_DAT_CHECK_HPP_
#define GRN_DAT_CHECK_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

class Check {
 public:
  Check()
      : check_(0) {}

  UInt32 check() const {
    return check_;
  }

  // The most significant bit (MSB) represents whether or not the node ID is
  // used as an offset. Note that the MSB is independent of the other bits.
  bool is_offset() const {
    return (check_ & IS_OFFSET_FLAG) == IS_OFFSET_FLAG;
  }

  UInt32 except_is_offset() const {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    return check_ & ~IS_OFFSET_FLAG;
  }

  // A phantom node is a node that has never been used, and such a node is also
  // called an empty element. Phantom nodes form a doubly linked list in each
  // block, and the linked list is represented by next() and prev().
  bool is_phantom() const {
    return (check_ & IS_PHANTOM_FLAG) == IS_PHANTOM_FLAG;
  }

  UInt32 next() const {
    GRN_DAT_DEBUG_THROW_IF(!is_phantom());
    return (check_ >> NEXT_SHIFT) & BLOCK_MASK;
  }
  UInt32 prev() const {
    GRN_DAT_DEBUG_THROW_IF(!is_phantom());
    return (check_ >> PREV_SHIFT) & BLOCK_MASK;
  }

  // A label is attached to each non-phantom node. A label is represented by
  // a byte except for a terminal label '\256'. Note that a phantom node always
  // returns an invalid label with its phantom bit flag so as to reject a
  // transition to the phantom node.
  UInt32 label() const {
    return check_ & (IS_PHANTOM_FLAG | LABEL_MASK);
  }

  UInt32 child() const {
    return (check_ >> CHILD_SHIFT) & LABEL_MASK;
  }
  UInt32 sibling() const {
    return (check_ >> SIBLING_SHIFT) & LABEL_MASK;
  }

  void set_check(UInt32 x) {
    check_ = x;
  }

  void set_is_offset(bool x) {
    if (x) {
      GRN_DAT_DEBUG_THROW_IF(is_offset());
      check_ |= IS_OFFSET_FLAG;
    } else {
      GRN_DAT_DEBUG_THROW_IF(!is_offset());
      check_ &= ~IS_OFFSET_FLAG;
    }
  }

  void set_except_is_offset(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    GRN_DAT_DEBUG_THROW_IF((x & IS_OFFSET_FLAG) == IS_OFFSET_FLAG);
    check_ = (check_ & IS_OFFSET_FLAG) | x;
  }

  // To reject a transition to an incomplete node, set_is_phantom() overwrites
  // its label with an invalid label when a node becomes a non-phantom node.
  void set_is_phantom(bool x) {
    if (x) {
      GRN_DAT_DEBUG_THROW_IF(is_phantom());
      check_ |= IS_PHANTOM_FLAG;
    } else {
      GRN_DAT_DEBUG_THROW_IF(!is_phantom());
      check_ = (check_ & IS_OFFSET_FLAG) | (INVALID_LABEL << CHILD_SHIFT) |
          (INVALID_LABEL << SIBLING_SHIFT) | INVALID_LABEL;
    }
  }

  void set_next(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(!is_phantom());
    GRN_DAT_DEBUG_THROW_IF(x > BLOCK_MASK);
    check_ = (check_ & ~(BLOCK_MASK << NEXT_SHIFT)) | (x << NEXT_SHIFT);
  }
  void set_prev(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(!is_phantom());
    GRN_DAT_DEBUG_THROW_IF(x > BLOCK_MASK);
    check_ = (check_ & ~(BLOCK_MASK << PREV_SHIFT)) | (x << PREV_SHIFT);
  }

  void set_label(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    GRN_DAT_DEBUG_THROW_IF(x > MAX_LABEL);
    check_ = (check_ & ~LABEL_MASK) | x;
  }

  void set_child(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    GRN_DAT_DEBUG_THROW_IF(x > MAX_LABEL);
    check_ = (check_ & ~(LABEL_MASK << CHILD_SHIFT)) | (x << CHILD_SHIFT);
  }
  void set_sibling(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    GRN_DAT_DEBUG_THROW_IF(label() > MAX_LABEL);
    GRN_DAT_DEBUG_THROW_IF((sibling() != INVALID_LABEL) && (x == INVALID_LABEL));
    check_ = (check_ & ~(LABEL_MASK << SIBLING_SHIFT)) | (x << SIBLING_SHIFT);
  }

 private:
  UInt32 check_;

  static const UInt32 IS_OFFSET_FLAG = 1U << 31;
  static const UInt32 IS_PHANTOM_FLAG = 1U << 30;
  static const UInt32 NEXT_SHIFT = 9;
  static const UInt32 PREV_SHIFT = 18;
  static const UInt32 CHILD_SHIFT = 9;
  static const UInt32 SIBLING_SHIFT = 18;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_CHECK_HPP_
