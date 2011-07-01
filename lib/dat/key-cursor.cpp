#include "key-cursor.hpp"

#include <algorithm>
#include <cstring>

#include "trie.hpp"

namespace grn {
namespace dat {

KeyCursor::KeyCursor()
    : trie_(NULL),
      offset_(0),
      limit_(UINT32_MAX),
      flags_(KEY_RANGE_CURSOR),
      buf_(),
      count_(0),
      max_count_(0),
      end_(false),
      end_ptr_(NULL),
      end_length_(0) {}

KeyCursor::~KeyCursor() {
  if ((end_ptr_ != NULL) && (end_length_ != 0)) {
    delete [] end_ptr_;
  }
}

void KeyCursor::open(const Trie &trie,
                     const void *min_ptr, UInt32 min_length,
                     const void *max_ptr, UInt32 max_length,
                     UInt32 offset,
                     UInt32 limit,
                     UInt32 flags) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (min_ptr == NULL) && (min_length != 0));
  GRN_DAT_THROW_IF(PARAM_ERROR, (max_ptr == NULL) && (max_length != 0));

  flags = fix_flags(flags);
  KeyCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(static_cast<const UInt8 *>(min_ptr), min_length,
                  static_cast<const UInt8 *>(max_ptr), max_length);
  new_cursor.swap(this);
}

void KeyCursor::close() {
  KeyCursor new_cursor;
  new_cursor.swap(this);
}

bool KeyCursor::next(Key *key) {
  if (end_ || (count_ >= max_count_)) {
    return false;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    return ascending_next(key);
  } else {
    return descending_next(key);
  }
}

UInt32 KeyCursor::fix_flags(UInt32 flags) const {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_type != 0) &&
                                (cursor_type != KEY_RANGE_CURSOR));
  flags |= KEY_RANGE_CURSOR;

  const UInt32 cursor_order = flags & CURSOR_ORDER_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_order != 0) &&
                                (cursor_order != ASCENDING_CURSOR) &&
                                (cursor_order != DESCENDING_CURSOR));
  if (cursor_order == 0) {
    flags |= ASCENDING_CURSOR;
  }

  const UInt32 cursor_options = flags & CURSOR_OPTIONS_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR,
      cursor_options & ~(EXCEPT_LOWER_BOUND | EXCEPT_UPPER_BOUND));

  return flags;
}

KeyCursor::KeyCursor(const Trie &trie,
                     UInt32 offset, UInt32 limit, UInt32 flags)
    : trie_(&trie),
      offset_(offset),
      limit_(limit),
      flags_(flags),
      buf_(),
      count_(0),
      max_count_(0),
      end_(false),
      end_ptr_(NULL),
      end_length_(0) {}

void KeyCursor::init(const UInt8 *min_ptr, UInt32 min_length,
                     const UInt8 *max_ptr, UInt32 max_length) {
  if (offset_ > (UINT32_MAX - limit_)) {
    max_count_ = UINT32_MAX;
  } else {
    max_count_ = offset_ + limit_;
  }

  if (limit_ == 0) {
    return;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    ascending_init(min_ptr, min_length, max_ptr, max_length);
  } else {
    descending_init(min_ptr, min_length, max_ptr, max_length);
  }
}

void KeyCursor::ascending_init(const UInt8 *min_ptr, UInt32 min_length,
                               const UInt8 *max_ptr, UInt32 max_length) {
  if (max_ptr != NULL) {
    if (max_length != 0) {
      end_ptr_ = new UInt8[max_length];
      std::memcpy(end_ptr_, max_ptr, max_length);
    } else {
      end_ptr_ = reinterpret_cast<UInt8 *>(const_cast<char *>(""));
    }
  }
  end_length_ = max_length;

  UInt32 node_id = ROOT_NODE_ID;
  Node node = trie_->ith_node(node_id);
  for (UInt32 i = 0; i < min_length; ++i) {
    if (node.is_terminal()) {
      Key key;
      trie_->ith_key(node.key_id(), &key);
      const int result = compare(key, min_ptr, min_length, i);
      if ((result > 0) || ((result == 0) &&
          ((flags_ & EXCEPT_LOWER_BOUND) != EXCEPT_LOWER_BOUND))) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
      }
      return;
    }

    node_id = node.offset() ^ min_ptr[i];
    if (trie_->ith_node(node_id).label() != min_ptr[i]) {
      UInt16 label = node.child();
      if (label == TERMINAL_LABEL) {
        label = trie_->ith_node(node.offset() ^ label).sibling();
      }
      while (label != INVALID_LABEL) {
        if (label > min_ptr[i]) {
          GRN_DAT_THROW_IF(MEMORY_ERROR,
                           !buf_.push_back(node.offset() ^ label));
          break;
        }
        label = trie_->ith_node(node.offset() ^ label).sibling();
      }
      return;
    }

    node = trie_->ith_node(node_id);
    if (node.sibling() != INVALID_LABEL) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(node_id ^ min_ptr[i] ^ node.sibling()));
    }
  }

  const Base base = trie_->ith_node(node_id).base();
  if (base.is_terminal()) {
    Key key;
    trie_->ith_key(base.key_id(), &key);
    if ((key.length() != min_length) ||
        ((flags_ & EXCEPT_LOWER_BOUND) != EXCEPT_LOWER_BOUND)) {
      GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
    }
    return;
  }

  UInt16 label = trie_->ith_node(node_id).child();
  if ((label == TERMINAL_LABEL) &&
      ((flags_ & EXCEPT_LOWER_BOUND) == EXCEPT_LOWER_BOUND)) {
    label = trie_->ith_node(base.offset() ^ label).sibling();
  }
  if (label != INVALID_LABEL) {
    GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(base.offset() ^ label));
  }
}

void KeyCursor::descending_init(const UInt8 *min_ptr, UInt32 min_length,
                                const UInt8 *max_ptr, UInt32 max_length) {
  if (min_ptr != NULL) {
    if (min_length != 0) {
      end_ptr_ = new UInt8[min_length];
      std::memcpy(end_ptr_, min_ptr, min_length);
    } else {
      end_ptr_ = reinterpret_cast<UInt8 *>(const_cast<char *>(""));
    }
  }
  end_length_ = min_length;

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < max_length; ++i) {
    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      Key key;
      trie_->ith_key(base.key_id(), &key);
      const int result = compare(key, max_ptr, max_length, i);
      if ((result < 0) || ((result == 0) &&
          ((flags_ & EXCEPT_UPPER_BOUND) != EXCEPT_UPPER_BOUND))) {
        GRN_DAT_THROW_IF(MEMORY_ERROR,
                         !buf_.push_back(node_id | POST_ORDER_FLAG));
      }
      return;
    }

    UInt32 label = trie_->ith_node(node_id).child();
    if (label == TERMINAL_LABEL) {
      node_id = base.offset() ^ label;
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(node_id | POST_ORDER_FLAG));
      label = trie_->ith_node(node_id).sibling();
    }
    while (label != INVALID_LABEL) {
      node_id = base.offset() ^ label;
      if (label < max_ptr[i]) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
      } else if (label > max_ptr[i]) {
        return;
      } else {
        break;
      }
      label = trie_->ith_node(node_id).sibling();
    }
    if (label == INVALID_LABEL) {
      return;
    }
  }

  const Base base = trie_->ith_node(node_id).base();
  if (base.is_terminal()) {
    Key key;
    trie_->ith_key(base.key_id(), &key);
    if ((key.length() == max_length) &&
        ((flags_ & EXCEPT_UPPER_BOUND) != EXCEPT_UPPER_BOUND)) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(node_id | POST_ORDER_FLAG));
    }
    return;
  }

  UInt16 label = trie_->ith_node(node_id).child();
  if ((label == TERMINAL_LABEL) &&
      ((flags_ & EXCEPT_UPPER_BOUND) != EXCEPT_UPPER_BOUND)) {
    GRN_DAT_THROW_IF(MEMORY_ERROR,
        !buf_.push_back((base.offset() ^ label) | POST_ORDER_FLAG));
  }
}

void KeyCursor::swap(KeyCursor *cursor) {
  std::swap(trie_, cursor->trie_);
  std::swap(offset_, cursor->offset_);
  std::swap(limit_, cursor->limit_);
  std::swap(flags_, cursor->flags_);
  buf_.swap(&cursor->buf_);
  std::swap(count_, cursor->count_);
  std::swap(max_count_, cursor->max_count_);
  std::swap(end_, cursor->end_);
  std::swap(end_ptr_, cursor->end_ptr_);
  std::swap(end_length_, cursor->end_length_);
}

bool KeyCursor::ascending_next(Key *key) {
  while (!buf_.empty()) {
    const UInt32 node_id = buf_.back();
    buf_.pop_back();

    const Node node = trie_->ith_node(node_id);
    if (node.sibling() != INVALID_LABEL) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
          !buf_.push_back(node_id ^ node.label() ^ node.sibling()));
    }

    if (node.child() != INVALID_LABEL) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(node.offset() ^ node.child()));
    }

    if (node.is_terminal()) {
      Key temp_key;
      trie_->ith_key(node.key_id(), &temp_key);
      if (end_ptr_ != NULL) {
        const int result = compare(temp_key, end_ptr_, end_length_, 0);
        if ((result > 0) || ((result == 0) &&
            ((flags_ & EXCEPT_UPPER_BOUND) == EXCEPT_UPPER_BOUND))) {
          end_ = true;
          return false;
        }
      }
      if (count_++ >= offset_) {
        *key = temp_key;
        return true;
      }
    }
  }
  return false;
}

bool KeyCursor::descending_next(Key *key) {
  while (!buf_.empty()) {
    const bool post_order = (buf_.back() & POST_ORDER_FLAG) == POST_ORDER_FLAG;
    const UInt32 node_id = buf_.back() & ~POST_ORDER_FLAG;

    const Base base = trie_->ith_node(node_id).base();
    if (post_order) {
      buf_.pop_back();
      if (base.is_terminal()) {
        Key temp_key;
        trie_->ith_key(base.key_id(), &temp_key);
        if (end_ptr_ != NULL) {
          const int result = compare(temp_key, end_ptr_, end_length_, 0);
          if ((result < 0) || ((result == 0) &&
              ((flags_ & EXCEPT_LOWER_BOUND) == EXCEPT_LOWER_BOUND))) {
            end_ = true;
            return false;
          }
        }
        if (count_++ >= offset_) {
          trie_->ith_key(base.key_id(), key);
          return true;
        }
      }
    } else {
      buf_.back() |= POST_ORDER_FLAG;
      UInt16 label = trie_->ith_node(node_id).child();
      while (label != INVALID_LABEL) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(base.offset() ^ label));
        label = trie_->ith_node(base.offset() ^ label).sibling();
      }
    }
  }
  return false;
}

int KeyCursor::compare(const Key &key,
                       const UInt8 *ptr, UInt32 length,
                       UInt32 offset) const {
  const UInt32 min_length = (key.length() < length) ? key.length() : length;
  for (UInt32 i = offset; i < min_length; ++i) {
    if (static_cast<UInt8>(key[i]) != ptr[i]) {
      return static_cast<UInt8>(key[i]) - ptr[i];
    }
  }
  if (key.length() < length) {
    return -1;
  }
  return (key.length() > length) ? 1 : 0;
}

}  // namespace grn
}  // namespace dat
