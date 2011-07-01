#include "id-cursor.hpp"

#include <algorithm>

#include "trie.hpp"

namespace grn {
namespace dat {

IdCursor::IdCursor()
    : trie_(NULL),
      offset_(0),
      limit_(UINT32_MAX),
      flags_(ID_RANGE_CURSOR),
      cur_(INVALID_KEY_ID),
      end_(INVALID_KEY_ID) {}

IdCursor::~IdCursor() {}

void IdCursor::open(const Trie &trie,
                    const void *min_ptr, UInt32 min_length,
                    const void *max_ptr, UInt32 max_length,
                    UInt32 offset,
                    UInt32 limit,
                    UInt32 flags) {
  GRN_DAT_THROW_IF(PARAM_ERROR, (min_ptr == NULL) && (min_length != 0));
  GRN_DAT_THROW_IF(PARAM_ERROR, (max_ptr == NULL) && (max_length != 0));

  UInt32 min_id = INVALID_KEY_ID;
  if (min_ptr != NULL) {
    Key key;
    GRN_DAT_THROW_IF(PARAM_ERROR, !trie.search(min_ptr, min_length, &key));
    min_id = key.id();
  }

  UInt32 max_id = INVALID_KEY_ID;
  if (max_ptr != NULL) {
    Key key;
    GRN_DAT_THROW_IF(PARAM_ERROR, !trie.search(max_ptr, max_length, &key));
    max_id = key.id();
  }

  open(trie, min_id, max_id, offset, limit, flags);
}

void IdCursor::open(const Trie &trie,
                    UInt32 min_id,
                    UInt32 max_id,
                    UInt32 offset,
                    UInt32 limit,
                    UInt32 flags) {
  flags = fix_flags(flags);

  IdCursor new_cursor(trie, offset, limit, flags);
  new_cursor.init(min_id, max_id);
  new_cursor.swap(this);
}

void IdCursor::close() {
  IdCursor new_cursor;
  new_cursor.swap(this);
}

bool IdCursor::next(Key *key) {
  if (cur_ == end_) {
    return false;
  }
  trie_->ith_key(cur_, key);
  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    ++cur_;
  } else {
    --cur_;
  }
  return true;
}

UInt32 IdCursor::fix_flags(UInt32 flags) const {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  GRN_DAT_THROW_IF(PARAM_ERROR, (cursor_type != 0) &&
                                (cursor_type != ID_RANGE_CURSOR));
  flags |= ID_RANGE_CURSOR;

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

IdCursor::IdCursor(const Trie &trie,
                   UInt32 offset, UInt32 limit, UInt32 flags)
    : trie_(&trie),
      offset_(offset),
      limit_(limit),
      flags_(flags),
      cur_(INVALID_KEY_ID),
      end_(INVALID_KEY_ID) {}

void IdCursor::init(UInt32 min_id, UInt32 max_id) {
  if (min_id == INVALID_KEY_ID) {
    min_id = trie_->min_key_id();
  } else if ((flags_ & EXCEPT_LOWER_BOUND) == EXCEPT_LOWER_BOUND) {
    ++min_id;
  }

  if (max_id == INVALID_KEY_ID) {
    max_id = trie_->max_key_id();
  } else if ((flags_ & EXCEPT_UPPER_BOUND) == EXCEPT_UPPER_BOUND) {
    --max_id;
  }

  if (max_id < min_id) {
    return;
  }

  UInt32 diff = max_id - min_id;
  if (diff < offset_) {
    return;
  }
  diff -= offset_;

  const UInt32 temp_limit = (limit_ > diff) ? (diff + 1) : limit_;
  if ((flags_ & ASCENDING_CURSOR) == ASCENDING_CURSOR) {
    min_id += offset_;
    max_id = min_id + temp_limit;
    if (min_id < max_id) {
      cur_ = min_id;
      end_ = max_id;
    }
  } else {
    max_id -= offset_;
    min_id = max_id - temp_limit;
    if (min_id < max_id) {
      cur_ = max_id;
      end_ = min_id;
    }
  }
}

void IdCursor::swap(IdCursor *cursor) {
  std::swap(trie_, cursor->trie_);
  std::swap(offset_, cursor->offset_);
  std::swap(limit_, cursor->limit_);
  std::swap(flags_, cursor->flags_);
  std::swap(cur_, cursor->cur_);
  std::swap(end_, cursor->end_);
}

}  // namespace grn
}  // namespace dat
