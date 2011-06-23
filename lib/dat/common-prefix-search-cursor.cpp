#include "common-prefix-search-cursor.hpp"

#include <algorithm>
#include <cstring>

namespace grn {
namespace dat {

CommonPrefixSearchCursor::CommonPrefixSearchCursor()
    : trie_(NULL),
      offset_(0),
      limit_(UINT32_MAX),
      flags_(COMMON_PREFIX_CURSOR),
      buf_(),
      count_(0) {}

CommonPrefixSearchCursor::~CommonPrefixSearchCursor() {
  close();
}

void CommonPrefixSearchCursor::open(const Trie &trie,
                                    const void *ptr,
                                    UInt32 min_length,
                                    UInt32 max_length,
                                    UInt32 offset,
                                    UInt32 limit,
                                    UInt32 flags) {
  GRN_DAT_PARAM_ERROR_IF((ptr == NULL) && (max_length != 0));
  GRN_DAT_PARAM_ERROR_IF(min_length > max_length);

  flags = fix_flags(flags);
  CommonPrefixSearchCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(static_cast<const UInt8 *>(ptr), min_length, max_length);
  new_cursor.swap(this);
}

void CommonPrefixSearchCursor::close() {
  trie_ = NULL;
  offset_ = 0;
  limit_ = UINT32_MAX;
  flags_ = COMMON_PREFIX_CURSOR;
  buf_.clear();
  count_ = 0;
}

bool CommonPrefixSearchCursor::next(Key *key) {
  if (count_ >= buf_.size()) {
    return false;
  }
  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    trie_->ith_key(buf_[count_++], key);
  } else {
    trie_->ith_key(buf_[buf_.size() - ++count_], key);
  }
  return true;
}

UInt32 CommonPrefixSearchCursor::fix_flags(UInt32 flags) const {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  GRN_DAT_PARAM_ERROR_IF((cursor_type != 0) &&
                         (cursor_type != COMMON_PREFIX_CURSOR));
  flags |= COMMON_PREFIX_CURSOR;

  const UInt32 cursor_order = flags & CURSOR_ORDER_MASK;
  GRN_DAT_PARAM_ERROR_IF((cursor_order != 0) &&
                         (cursor_order != ASCENDING_CURSOR) &&
                         (cursor_order != DESCENDING_CURSOR));
  if (cursor_order == 0) {
    flags |= ASCENDING_CURSOR;
  }

  const UInt32 cursor_options = flags & CURSOR_OPTIONS_MASK;
  GRN_DAT_PARAM_ERROR_IF(cursor_options & ~EXCEPT_EXACT_MATCH);

  return flags;
}

CommonPrefixSearchCursor::CommonPrefixSearchCursor(const Trie &trie,
                                                   UInt32 offset,
                                                   UInt32 limit,
                                                   UInt32 flags)
    : trie_(&trie),
      offset_(offset),
      limit_(limit),
      flags_(flags),
      buf_(),
      count_(0) {}

void CommonPrefixSearchCursor::init(const UInt8 *ptr,
                                    UInt32 min_length,
                                    UInt32 max_length) {
  if ((limit_ == 0) || (offset_ > (max_length - min_length))) {
    return;
  }

  UInt32 node_id = ROOT_NODE_ID;
  UInt32 skip_count = 0;
  for (UInt32 i = 0; i < max_length; ++i) {
    if (trie_->ith_node(node_id).child() == TERMINAL_LABEL) {
      if (i >= min_length) {
        if (skip_count < offset_) {
          ++skip_count;
        } else {
          const UInt32 terminal =
              trie_->ith_node(node_id).offset() ^ TERMINAL_LABEL;
          buf_.push_back(trie_->ith_node(terminal).key_id());
          if (buf_.size() >= limit_) {
            return;
          }
        }
      }
    }

    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      Key key;
      trie_->ith_key(base.key_id(), &key);
      if ((key.length() >= min_length) && (key.length() <= max_length) &&
          (std::memcmp(ptr + i, key.ptr() + i, key.length() - i) == 0)) {
        if (skip_count < offset_) {
          ++skip_count;
        } else {
          buf_.push_back(key.id());
        }
      }
      return;
    }

    node_id = base.offset() ^ ptr[i];
    if (trie_->ith_node(node_id).label() != ptr[i]) {
      return;
    }
  }

  if (skip_count < offset_) {
    return;
  }

  const Base base = trie_->ith_node(node_id).base();
  if (base.is_terminal()) {
    Key key;
    trie_->ith_key(base.key_id(), &key);
    if (key.length() <= max_length) {
      buf_.push_back(key.id());
    }
  } else if (trie_->ith_node(node_id).child() == TERMINAL_LABEL) {
    const UInt32 terminal = base.offset() ^ TERMINAL_LABEL;
    Key key;
    trie_->ith_key(trie_->ith_node(terminal).key_id(), &key);
    if ((key.length() >= min_length) && (key.length() <= max_length)) {
      buf_.push_back(key.id());
    }
  }
}

void CommonPrefixSearchCursor::swap(CommonPrefixSearchCursor *cursor) {
  std::swap(trie_, cursor->trie_);
  std::swap(offset_, cursor->offset_);
  std::swap(limit_, cursor->limit_);
  std::swap(flags_, cursor->flags_);
  buf_.swap(cursor->buf_);
  std::swap(count_, cursor->count_);
}

}  // namespace grn
}  // namespace dat
