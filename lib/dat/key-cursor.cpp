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
      finished_(false),
      end_buf_(NULL),
      end_str_() {}

KeyCursor::~KeyCursor() {
  if (end_buf_ != NULL) {
    delete [] end_buf_;
  }
}

void KeyCursor::open(const Trie &trie,
                     const String &min_str,
                     const String &max_str,
                     UInt32 offset,
                     UInt32 limit,
                     UInt32 flags) {
  GRN_DAT_THROW_IF(PARAM_ERROR,
                   (min_str.ptr() == NULL) && (min_str.length() != 0));
  GRN_DAT_THROW_IF(PARAM_ERROR,
                   (max_str.ptr() == NULL) && (max_str.length() != 0));

  flags = fix_flags(flags);
  KeyCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(min_str, max_str);
  new_cursor.swap(this);
}

void KeyCursor::close() {
  KeyCursor new_cursor;
  new_cursor.swap(this);
}

bool KeyCursor::next(Key *key) {
  if (finished_ || (count_ >= max_count_)) {
    return false;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    return ascending_next(key);
  } else {
    return descending_next(key);
  }
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
      finished_(false),
      end_buf_(NULL),
      end_str_() {}

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

void KeyCursor::init(const String &min_str, const String &max_str) {
  if (offset_ > (UINT32_MAX - limit_)) {
    max_count_ = UINT32_MAX;
  } else {
    max_count_ = offset_ + limit_;
  }

  if (limit_ == 0) {
    return;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    ascending_init(min_str, max_str);
  } else {
    descending_init(min_str, max_str);
  }
}

void KeyCursor::ascending_init(const String &min_str, const String &max_str) {
  if (max_str.ptr() != NULL) {
    if (max_str.length() != 0) {
      end_buf_ = new UInt8[max_str.length()];
      std::memcpy(end_buf_, max_str.ptr(), max_str.length());
      end_str_.assign(end_buf_, max_str.length());
    }
  }

  if ((min_str.ptr() == NULL) || (min_str.length() == 0)) {
    GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(ROOT_NODE_ID));
    return;
  }

  UInt32 node_id = ROOT_NODE_ID;
  Node node;
  for (UInt32 i = 0; i < min_str.length(); ++i) {
    node = trie_->ith_node(node_id);
    if (node.is_terminal()) {
      Key key;
      trie_->ith_key(node.key_id(), &key);
      const int result = key.str().compare(min_str, i);
      if ((result > 0) || ((result == 0) &&
          ((flags_ & EXCEPT_LOWER_BOUND) != EXCEPT_LOWER_BOUND))) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
      } else if (node.sibling() != INVALID_LABEL) {
        GRN_DAT_THROW_IF(MEMORY_ERROR,
            !buf_.push_back(node_id ^ node.label() ^ node.sibling()));
      }
      return;
    } else if (node.sibling() != INVALID_LABEL) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
          !buf_.push_back(node_id ^ node.label() ^ node.sibling()));
    }

    node_id = node.offset() ^ min_str[i];
    if (trie_->ith_node(node_id).label() != min_str[i]) {
      UInt16 label = node.child();
      if (label == TERMINAL_LABEL) {
        label = trie_->ith_node(node.offset() ^ label).sibling();
      }
      while (label != INVALID_LABEL) {
        if (label > min_str[i]) {
          GRN_DAT_THROW_IF(MEMORY_ERROR,
                           !buf_.push_back(node.offset() ^ label));
          break;
        }
        label = trie_->ith_node(node.offset() ^ label).sibling();
      }
      return;
    }
  }

  node = trie_->ith_node(node_id);
  if (node.is_terminal()) {
    Key key;
    trie_->ith_key(node.key_id(), &key);
    if ((key.length() != min_str.length()) ||
        ((flags_ & EXCEPT_LOWER_BOUND) != EXCEPT_LOWER_BOUND)) {
      GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
    } else if (node.sibling() != INVALID_LABEL) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
          !buf_.push_back(node_id ^ node.label() ^ node.sibling()));
    }
    return;
  }

  UInt16 label = node.child();
  if ((label == TERMINAL_LABEL) &&
      ((flags_ & EXCEPT_LOWER_BOUND) == EXCEPT_LOWER_BOUND)) {
    label = trie_->ith_node(node.offset() ^ label).sibling();
  }
  if (label != INVALID_LABEL) {
    GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node.offset() ^ label));
  }
}

void KeyCursor::descending_init(const String &min_str, const String &max_str) {
  if (min_str.ptr() != NULL) {
    if (min_str.length() != 0) {
      end_buf_ = new UInt8[min_str.length()];
      std::memcpy(end_buf_, min_str.ptr(), min_str.length());
      end_str_.assign(end_buf_, min_str.length());
    }
  }

  if ((max_str.ptr() == NULL) || (max_str.length() == 0)) {
    GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(ROOT_NODE_ID));
    return;
  }

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < max_str.length(); ++i) {
    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      Key key;
      trie_->ith_key(base.key_id(), &key);
      const int result = key.str().compare(max_str, i);
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
      if (label < max_str[i]) {
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
      } else if (label > max_str[i]) {
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
    if ((key.length() == max_str.length()) &&
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
  std::swap(finished_, cursor->finished_);
  std::swap(end_buf_, cursor->end_buf_);
  end_str_.swap(&cursor->end_str_);
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

    if (node.is_terminal()) {
      Key temp_key;
      trie_->ith_key(node.key_id(), &temp_key);
      if (end_buf_ != NULL) {
        const int result = temp_key.str().compare(end_str_);
        if ((result > 0) || ((result == 0) &&
            ((flags_ & EXCEPT_UPPER_BOUND) == EXCEPT_UPPER_BOUND))) {
          finished_ = true;
          return false;
        }
      }
      if (count_++ >= offset_) {
        *key = temp_key;
        return true;
      }
    } else if (node.child() != INVALID_LABEL) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(node.offset() ^ node.child()));
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
        if (end_buf_ != NULL) {
          const int result =
              temp_key.str().compare(end_str_);
          if ((result < 0) || ((result == 0) &&
              ((flags_ & EXCEPT_LOWER_BOUND) == EXCEPT_LOWER_BOUND))) {
            finished_ = true;
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

}  // namespace dat
}  // namespace grn
