#ifndef GRN_DAT_HPP_EADER_HPP_
#define GRN_DAT_HPP_EADER_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

class Header {
 public:
  Header()
      : file_size_(0),
        num_keys_(0),
        max_num_keys_(0),
        num_phantoms_(0),
        num_zombies_(0),
        num_blocks_(0),
        max_num_blocks_(0),
        key_buf_size_(0),
        entries_() {
    for (UInt32 i = 0; i <= MAX_BLOCK_LEVEL; ++i) {
      entries_[i] = INVALID_ENTRY;
    }
  }

  UInt64 file_size() const {
    return file_size_;
  }
  UInt32 min_key_id() const {
    return MIN_KEY_ID;
  }
  UInt32 max_key_id() const {
    return num_keys();
  }
  UInt32 num_keys() const {
    return num_keys_;
  }
  UInt32 max_num_keys() const {
    return max_num_keys_;
  }
  UInt32 num_nodes() const {
    return num_blocks() * BLOCK_SIZE;
  }
  UInt32 num_phantoms() const {
    return num_phantoms_;
  }
  UInt32 num_zombies() const {
    return num_zombies_;
  }
  UInt32 max_num_nodes() const {
    return max_num_blocks() * BLOCK_SIZE;
  }
  UInt32 num_blocks() const {
    return num_blocks_;
  }
  UInt32 max_num_blocks() const {
    return max_num_blocks_;
  }
  UInt32 key_buf_size() const {
    return key_buf_size_;
  }
  UInt32 ith_entry(UInt32 i) const {
    GRN_DAT_DEBUG_THROW_IF(i > MAX_BLOCK_LEVEL);
    return entries_[i];
  }

  void set_file_size(UInt64 x) {
    file_size_ = x;
  }
  void set_num_keys(UInt32 x) {
    num_keys_ = x;
  }
  void set_max_num_keys(UInt32 x) {
    max_num_keys_ = x;
  }
  void set_num_phantoms(UInt32 x) {
    num_phantoms_ = x;
  }
  void set_num_zombies(UInt32 x) {
    num_zombies_ = x;
  }
  void set_num_blocks(UInt32 x) {
    num_blocks_ = x;
  }
  void set_max_num_blocks(UInt32 x) {
    max_num_blocks_ = x;
  }
  void set_key_buf_size(UInt32 x) {
    key_buf_size_ = x;
  }
  void set_ith_entry(UInt32 i, UInt32 x) {
    GRN_DAT_DEBUG_THROW_IF(i > MAX_BLOCK_LEVEL);
    GRN_DAT_DEBUG_THROW_IF((x != INVALID_ENTRY) && (x >= num_blocks()));
    entries_[i] = x;
  }

 private:
  UInt64 file_size_;
  UInt32 num_keys_;
  UInt32 max_num_keys_;
  UInt32 num_phantoms_;
  UInt32 num_zombies_;
  UInt32 num_blocks_;
  UInt32 max_num_blocks_;
  UInt32 key_buf_size_;
  UInt32 entries_[MAX_BLOCK_LEVEL + 1];
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_HPP_EADER_HPP_
