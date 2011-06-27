#include "predictive-search-cursor.hpp"

#include <algorithm>
#include <cstring>

namespace grn {
namespace dat {

PredictiveSearchCursor::PredictiveSearchCursor()
    : trie_(NULL),
      offset_(0),
      limit_(UINT32_MAX),
      flags_(PREDICTIVE_CURSOR),
      buf_(),
      cur_(0),
      end_(0),
      min_length_(0) {}

PredictiveSearchCursor::~PredictiveSearchCursor() {
  close();
}

void PredictiveSearchCursor::open(const Trie &trie,
                                  const void *ptr,
                                  UInt32 length,
                                  UInt32 offset,
                                  UInt32 limit,
                                  UInt32 flags) {
  GRN_DAT_PARAM_ERROR_IF((ptr == NULL) && (length != 0));

  flags = fix_flags(flags);
  PredictiveSearchCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(static_cast<const UInt8 *>(ptr), length);
  new_cursor.swap(this);
}

void PredictiveSearchCursor::close() {
  trie_ = NULL;
  offset_ = 0;
  limit_ = UINT32_MAX;
  flags_ = PREDICTIVE_CURSOR;
  buf_.clear();
  cur_ = 0;
  end_ = 0;
  min_length_ = 0;
}

bool PredictiveSearchCursor::next(Key *key) {
  if (cur_ == end_) {
    return false;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    return ascending_next(key);
  } else {
    return descending_next(key);
  }
}

UInt32 PredictiveSearchCursor::fix_flags(UInt32 flags) const {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  GRN_DAT_PARAM_ERROR_IF((cursor_type != 0) &&
                         (cursor_type != PREDICTIVE_CURSOR));
  flags |= PREDICTIVE_CURSOR;

  const UInt32 cursor_order = flags & CURSOR_ORDER_MASK;
  GRN_DAT_PARAM_ERROR_IF((cursor_order != 0) &&
                         (cursor_order != ASCENDING_CURSOR) &&
                         (cursor_order != DESCENDING_CURSOR));
  if (cursor_order == 0) {
    flags |= ASCENDING_CURSOR;
  }

  const UInt32 cursor_options = flags & CURSOR_OPTIONS_MASK;
  GRN_DAT_PARAM_ERROR_IF(cursor_options & ~(EXCEPT_EXACT_MATCH));

  return flags;
}

PredictiveSearchCursor::PredictiveSearchCursor(const Trie &trie,
                                               UInt32 offset,
                                               UInt32 limit,
                                               UInt32 flags)
    : trie_(&trie),
      offset_(offset),
      limit_(limit),
      flags_(flags),
      buf_(),
      cur_(0),
      end_(0),
      min_length_(0) {}

void PredictiveSearchCursor::init(const UInt8 *ptr, UInt32 length) {
  if (limit_ == 0) {
    return;
  }

  min_length_ = length;
  if ((flags_ & EXCEPT_EXACT_MATCH) == EXCEPT_EXACT_MATCH) {
    ++min_length_;
  }
  end_ = (offset_ > (UINT32_MAX - limit_)) ? UINT32_MAX : (offset_ + limit_);

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < length; ++i) {
    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      if (offset_ == 0) {
        Key key;
        trie_->ith_key(base.key_id(), &key);
        if ((key.length() >= length) &&
            (std::memcmp(ptr + i, key.ptr() + i, length - i) == 0)) {
          buf_.push_back(node_id);
        }
      }
      return;
    }

    node_id = base.offset() ^ ptr[i];
    if (trie_->ith_node(node_id).label() != ptr[i]) {
      return;
    }
  }
  buf_.push_back(node_id);
}

void PredictiveSearchCursor::swap(PredictiveSearchCursor *cursor) {
  std::swap(trie_, cursor->trie_);
  std::swap(offset_, cursor->offset_);
  std::swap(limit_, cursor->limit_);
  std::swap(flags_, cursor->flags_);
  buf_.swap(cursor->buf_);
  std::swap(cur_, cursor->cur_);
  std::swap(end_, cursor->end_);
  std::swap(min_length_, cursor->min_length_);
}

bool PredictiveSearchCursor::ascending_next(Key *key) {
  while (!buf_.empty()) {
    const UInt32 node_id = buf_.back();
    buf_.pop_back();

    const Node node = trie_->ith_node(node_id);
    if (node.sibling() != INVALID_LABEL) {
      buf_.push_back(node_id ^ node.label() ^ node.sibling());
    }

    if (node.is_terminal()) {
      Key temp_key;
      trie_->ith_key(node.key_id(), &temp_key);
      if (temp_key.length() >= min_length_) {
        if (cur_++ >= offset_) {
          *key = temp_key;
          return true;
        }
      }
    } else if (node.child() != INVALID_LABEL) {
      buf_.push_back(node.offset() ^ node.child());
    }
  }
  return false;
}

bool PredictiveSearchCursor::descending_next(Key *key) {
  while (!buf_.empty()) {
    const bool post_order = (buf_.back() & POST_ORDER_FLAG) == POST_ORDER_FLAG;
    const UInt32 node_id = buf_.back() & ~POST_ORDER_FLAG;

    const Base base = trie_->ith_node(node_id).base();
    if (post_order) {
      buf_.pop_back();
      if (base.is_terminal()) {
        Key temp_key;
        trie_->ith_key(base.key_id(), &temp_key);
        if (temp_key.length() >= min_length_) {
          if (cur_++ >= offset_) {
            *key = temp_key;
            return true;
          }
        }
      }
    } else {
      buf_.back() |= POST_ORDER_FLAG;
      UInt16 label = trie_->ith_node(node_id).child();
      while (label != INVALID_LABEL) {
        buf_.push_back(base.offset() ^ label);
        label = trie_->ith_node(base.offset() ^ label).sibling();
      }
    }
  }
  return false;
}

}  // namespace grn
}  // namespace dat
