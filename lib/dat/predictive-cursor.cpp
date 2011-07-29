/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2011 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "predictive-cursor.hpp"

#include <algorithm>
#include <cstring>

#include "trie.hpp"

namespace grn {
namespace dat {

PredictiveCursor::PredictiveCursor()
    : trie_(NULL),
      offset_(0),
      limit_(UINT32_MAX),
      flags_(PREDICTIVE_CURSOR),
      buf_(),
      cur_(0),
      end_(0),
      min_length_(0) {}

PredictiveCursor::~PredictiveCursor() {}

void PredictiveCursor::open(const Trie &trie,
                            const String &str,
                            UInt32 offset,
                            UInt32 limit,
                            UInt32 flags) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (str.ptr() == NULL) && (str.length() != 0));

  flags = fix_flags(flags);
  PredictiveCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(str);
  new_cursor.swap(this);
}

void PredictiveCursor::close() {
  PredictiveCursor new_cursor;
  new_cursor.swap(this);
}

bool PredictiveCursor::next(Key *key) {
  if (cur_ == end_) {
    return false;
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    return ascending_next(key);
  } else {
    return descending_next(key);
  }
}

PredictiveCursor::PredictiveCursor(const Trie &trie,
                                   UInt32 offset, UInt32 limit, UInt32 flags)
    : trie_(&trie),
      offset_(offset),
      limit_(limit),
      flags_(flags),
      buf_(),
      cur_(0),
      end_(0),
      min_length_(0) {}

UInt32 PredictiveCursor::fix_flags(UInt32 flags) const {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_type != 0) &&
                                (cursor_type != PREDICTIVE_CURSOR));
  flags |= PREDICTIVE_CURSOR;

  const UInt32 cursor_order = flags & CURSOR_ORDER_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_order != 0) &&
                                (cursor_order != ASCENDING_CURSOR) &&
                                (cursor_order != DESCENDING_CURSOR));
  if (cursor_order == 0) {
    flags |= ASCENDING_CURSOR;
  }

  const UInt32 cursor_options = flags & CURSOR_OPTIONS_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, cursor_options & ~(EXCEPT_EXACT_MATCH));

  return flags;
}

void PredictiveCursor::init(const String &str) {
  if (limit_ == 0) {
    return;
  }

  min_length_ = str.length();
  if ((flags_ & EXCEPT_EXACT_MATCH) == EXCEPT_EXACT_MATCH) {
    ++min_length_;
  }
  end_ = (offset_ > (UINT32_MAX - limit_)) ? UINT32_MAX : (offset_ + limit_);

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < str.length(); ++i) {
    const Base base = trie_->ith_node(node_id).base();
    if (base.is_terminal()) {
      if (offset_ == 0) {
        Key key;
        trie_->ith_key(base.key_id(), &key);
        if ((key.length() >= str.length()) &&
            (key.str().substr(0, str.length()).compare(str, i) == 0)) {
          if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
            node_id |= IS_ROOT_FLAG;
          }
          GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
        }
      }
      return;
    }

    node_id = base.offset() ^ str[i];
    if (trie_->ith_node(node_id).label() != str[i]) {
      return;
    }
  }

  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    node_id |= IS_ROOT_FLAG;
  }
  GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(node_id));
}

void PredictiveCursor::swap(PredictiveCursor *cursor) {
  std::swap(trie_, cursor->trie_);
  std::swap(offset_, cursor->offset_);
  std::swap(limit_, cursor->limit_);
  std::swap(flags_, cursor->flags_);
  buf_.swap(&cursor->buf_);
  std::swap(cur_, cursor->cur_);
  std::swap(end_, cursor->end_);
  std::swap(min_length_, cursor->min_length_);
}

bool PredictiveCursor::ascending_next(Key *key) {
  while (!buf_.empty()) {
    const bool is_root = (buf_.back() & IS_ROOT_FLAG) == IS_ROOT_FLAG;
    const UInt32 node_id = buf_.back() & ~IS_ROOT_FLAG;
    buf_.pop_back();

    const Node node = trie_->ith_node(node_id);
    if (!is_root && (node.sibling() != INVALID_LABEL)) {
      GRN_DAT_THROW_IF(MEMORY_ERROR,
          !buf_.push_back(node_id ^ node.label() ^ node.sibling()));
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
      GRN_DAT_THROW_IF(MEMORY_ERROR,
                       !buf_.push_back(node.offset() ^ node.child()));
    }
  }
  return false;
}

bool PredictiveCursor::descending_next(Key *key) {
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
        GRN_DAT_THROW_IF(MEMORY_ERROR, !buf_.push_back(base.offset() ^ label));
        label = trie_->ith_node(base.offset() ^ label).sibling();
      }
    }
  }
  return false;
}

}  // namespace dat
}  // namespace grn
