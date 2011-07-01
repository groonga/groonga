#include "common-prefix-cursor.hpp"

#include <algorithm>
#include <cstring>

#include "trie.hpp"

namespace grn {
namespace dat {

CommonPrefixCursor::CommonPrefixCursor()
    : trie_(NULL),
      offset_(0),
      limit_(UINT32_MAX),
      flags_(COMMON_PREFIX_CURSOR),
      buf_(),
      cur_(0),
      end_(0) {}

CommonPrefixCursor::~CommonPrefixCursor() {}

void CommonPrefixCursor::open(const Trie &trie,
                              const void *ptr,
                              UInt32 min_length,
                              UInt32 max_length,
                              UInt32 offset,
                              UInt32 limit,
                              UInt32 flags) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (ptr == NULL) && (max_length != 0));
  GRN_DAT_THROW_IF(PARAM_ERROR, min_length > max_length);

  flags = fix_flags(flags);
  CommonPrefixCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(static_cast<const UInt8 *>(ptr), min_length, max_length);
  new_cursor.swap(this);
}

void CommonPrefixCursor::close() {
  CommonPrefixCursor new_cursor;
  new_cursor.swap(this);
}

bool CommonPrefixCursor::next(Key *key) {
  if (cur_ == end_) {
    return false;
  }
  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    trie_->ith_key(buf_[cur_++], key);
  } else {
    trie_->ith_key(buf_[--cur_], key);
  }
  return true;
}

UInt32 CommonPrefixCursor::fix_flags(UInt32 flags) const {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_type != 0) &&
                                (cursor_type != COMMON_PREFIX_CURSOR));
  flags |= COMMON_PREFIX_CURSOR;

  const UInt32 cursor_order = flags & CURSOR_ORDER_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_order != 0) &&
                                (cursor_order != ASCENDING_CURSOR) &&
                                (cursor_order != DESCENDING_CURSOR));
  if (cursor_order == 0) {
    flags |= ASCENDING_CURSOR;
  }

  const UInt32 cursor_options = flags & CURSOR_OPTIONS_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, cursor_options & ~EXCEPT_EXACT_MATCH);

  return flags;
}

CommonPrefixCursor::CommonPrefixCursor(const Trie &trie,
                                        UInt32 offset,
                                        UInt32 limit,
                                        UInt32 flags)
    : trie_(&trie),
      offset_(offset),
      limit_(limit),
      flags_(flags),
      buf_(),
      cur_(0),
      end_(0) {}

void CommonPrefixCursor::init(const UInt8 *ptr,
                              UInt32 min_length,
                              UInt32 max_length) {
  if ((limit_ == 0) || (offset_ > (max_length - min_length))) {
    return;
  }

  UInt32 node_id = ROOT_NODE_ID;
  UInt32 i;
  for (i = 0; i < max_length; ++i) {
    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      Key key;
      trie_->ith_key(base.key_id(), &key);
      if ((key.length() >= min_length) && (key.length() <= max_length) &&
          (std::memcmp(ptr + i, key.ptr() + i, key.length() - i) == 0) &&
          ((key.length() < max_length) ||
           ((flags_ & EXCEPT_EXACT_MATCH) != EXCEPT_EXACT_MATCH))) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(key.id()));
      }
      break;
    }

    if ((i >= min_length) &&
        (trie_->ith_node(node_id).child() == TERMINAL_LABEL)) {
      const UInt32 terminal = base.offset() ^ TERMINAL_LABEL;
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(trie_->ith_node(terminal).key_id()));
    }

    node_id = base.offset() ^ ptr[i];
    if (trie_->ith_node(node_id).label() != ptr[i]) {
      break;
    }
  }

  if ((i == max_length) &&
      (flags_ & EXCEPT_EXACT_MATCH) != EXCEPT_EXACT_MATCH) {
    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      Key key;
      trie_->ith_key(base.key_id(), &key);
      if ((key.length() >= min_length) && (key.length() <= max_length)) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(key.id()));
      }
    } else if (trie_->ith_node(node_id).child() == TERMINAL_LABEL) {
      const UInt32 terminal = base.offset() ^ TERMINAL_LABEL;
      Key key;
      trie_->ith_key(trie_->ith_node(terminal).key_id(), &key);
      if ((key.length() >= min_length) && (key.length() <= max_length)) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(key.id()));
      }
    }
  }

  if (buf_.size() <= offset_) {
    return;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    cur_ = offset_;
    end_ = (limit_ < (buf_.size() - cur_)) ? (cur_ + limit_) : buf_.size();
  } else {
    cur_ = buf_.size() - offset_;
    end_ = (limit_ < cur_) ? (cur_ - limit_) : 0;
  }
}

void CommonPrefixCursor::swap(CommonPrefixCursor *cursor) {
  std::swap(trie_, cursor->trie_);
  std::swap(offset_, cursor->offset_);
  std::swap(limit_, cursor->limit_);
  std::swap(flags_, cursor->flags_);
  buf_.swap(&cursor->buf_);
  std::swap(cur_, cursor->cur_);
  std::swap(end_, cursor->end_);
}

}  // namespace grn
}  // namespace dat
