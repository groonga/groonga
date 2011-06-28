#ifndef GRN_DAT_NODE_HPP_
#define GRN_DAT_NODE_HPP_

// See base.hpp and check.hpp for details.
#include "base.hpp"
#include "check.hpp"

namespace grn {
namespace dat {

class Node {
 public:
  Node() : base_(), check_() {}

  Base base() const {
    return base_;
  }
  bool is_terminal() const {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    return base_.is_terminal();
  }
  UInt32 offset() const {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    return base_.offset();
  }
  UInt32 key_id() const {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    return base_.key_id();
  }

  Check check() const {
    return check_;
  }
  bool is_offset() const {
    return check_.is_offset();
  }
  UInt32 except_is_offset() const {
    return check_.except_is_offset();
  }
  bool is_phantom() const {
    return check_.is_phantom();
  }
  UInt32 next() const {
    return check_.next();
  }
  UInt32 prev() const {
    return check_.prev();
  }
  UInt32 label() const {
    return check_.label();
  }
  UInt32 child() const {
    return check_.child();
  }
  UInt32 sibling() const {
    return check_.sibling();
  }

  void set_base(Base x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    base_ = x;
  }
  void set_offset(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    base_.set_offset(x);
  }
  void set_key_id(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(is_phantom());
    base_.set_key_id(x);
  }

  void set_check(Check x) {
    check_ = x;
  }
  void set_is_offset(bool x) {
    check_.set_is_offset(x);
  }
  void set_except_is_offset(UInt32 x) {
    check_.set_except_is_offset(x);
  }
  void set_is_phantom(bool x) {
    GRN_DAT_DEBUG_THROW_IF(base_.offset() != INVALID_OFFSET);
    check_.set_is_phantom(x);
  }
  void set_next(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(base_.offset() != INVALID_OFFSET);
    check_.set_next(x);
  }
  void set_prev(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(base_.offset() != INVALID_OFFSET);
    check_.set_prev(x);
  }
  void set_label(UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(offset() != INVALID_OFFSET);
    check_.set_label(x);
  }
  void set_child(UInt32 x) {
    check_.set_child(x);
  }
  void set_sibling(UInt32 x) {
    check_.set_sibling(x);
  }

 private:
  Base base_;
  Check check_;
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_NODE_HPP_
