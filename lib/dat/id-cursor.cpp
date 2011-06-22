#include "id-cursor.hpp"

namespace grn {
namespace dat {

void IdCursor::open(const Trie &trie,
                    const void *min_ptr,
                    UInt32 min_length,
                    const void *max_ptr,
                    UInt32 max_length,
                    UInt32 offset,
                    UInt32 limit,
                    bool is_ascending,
                    bool ignores_min,
                    bool ignores_max) {
  GRN_DAT_PARAM_ERROR_IF((min_ptr == NULL) && (min_length != 0));
  GRN_DAT_PARAM_ERROR_IF((max_ptr == NULL) && (max_length != 0));

  trie_ = &trie;
  step_ = is_ascending ? 1 : -1;
  cur_ = INVALID_KEY_ID;
  end_ = INVALID_KEY_ID;

  Key min_key;
  if (min_ptr == NULL) {
    min_key.set_id((UInt32)MAX_KEY_ID + 1);
  } else if (!trie.search(min_ptr, min_length, &min_key)) {
    return;
  }

  Key max_key;
  if (max_ptr == NULL) {
    max_key.set_id((UInt32)MAX_KEY_ID + 1);
  } else if (!trie.search(max_ptr, max_length, &max_key)) {
    return;
  }

  open(trie, min_key.id(), max_key.id(),
       offset, limit, is_ascending, ignores_min, ignores_max);
}

bool IdCursor::next(Key *key) {
  if (cur_ == end_) {
    return false;
  }
  cur_ += step_;
  trie_->ith_key(cur_, key);
  return true;
}

void IdCursor::open(const Trie &trie,
                    UInt32 min_id,
                    UInt32 max_id,
                    UInt32 offset,
                    UInt32 limit,
                    bool is_ascending,
                    bool ignores_min,
                    bool ignores_max) {
  if (min_id == INVALID_KEY_ID) {
    min_id = KEY_ID_OFFSET;  // MIN_KEY_ID
  } else if (ignores_min) {
    ++min_id;
  }

  if (max_id == INVALID_KEY_ID) {
    max_id = trie.num_keys();
  } else if (ignores_max) {
    --max_id;
  }

  // Overflow and underflow...
  if (is_ascending) {
    min_id += offset;
    max_id -= limit;
    ++max_id;
    if (min_id < max_id) {
      cur_ = min_id;
      end_ = max_id;
    }
  } else {
    min_id += limit;
    max_id -= offset;
    --min_id;
    if (min_id < max_id) {
      cur_ = max_id;
      end_ = min_id;
    }
  }
}

}  // namespace grn
}  // namespace dat
