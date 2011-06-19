#include "common-prefix-search-cursor.hpp"

#include <cstring>

namespace grn {
namespace dat {

void CommonPrefixSearchCursor::open(const Trie &trie,
                                    const void *ptr,
                                    UInt32 min_length,
                                    UInt32 max_length,
                                    UInt32 offset,
                                    UInt32 max_count) {
  DA_PARAM_ERROR_IF((ptr == NULL) && (max_length != 0));
  DA_PARAM_ERROR_IF(min_length > max_length);

  const UInt8 * const uptr = static_cast<const UInt8 *>(ptr);

  trie_ = &trie;
  buf_.clear();
  count_ = 0;
  if ((max_count == 0) || (offset > (max_length - min_length))) {
    return;
  }

  UInt32 node_id = ROOT_NODE_ID;
  for (UInt32 i = 0; i < max_length; ++i) {
    if (trie.ith_node(node_id).child() == TERMINAL_LABEL) {
      if (i >= min_length) {
        if (offset) {
          --offset;
        } else {
          const UInt32 terminal = trie.ith_node(node_id).offset() ^ TERMINAL_LABEL;
          buf_.push_back(trie.ith_node(terminal).key_id());
          if (buf_.size() >= max_count) {
            return;
          }
        }
      }
    }

    const Base base = trie.ith_node(node_id).base();
    if (base.is_terminal()) {
      Key key;
      trie.ith_key(base.key_id(), &key);
      if ((key.length() >= min_length) && (key.length() <= max_length) &&
          (std::memcmp(uptr + i, key.ptr() + i, key.length() - i) == 0)) {
        if (offset) {
          --offset;
        } else {
          buf_.push_back(key.id());
        }
      }
      return;
    }

    node_id = base.offset() ^ uptr[i];
    if (trie.ith_node(node_id).label() != uptr[i]) {
      return;
    }
  }

  if (offset) {
    return;
  }

  const Base base = trie.ith_node(node_id).base();
  if (base.is_terminal()) {
    Key key;
    trie.ith_key(base.key_id(), &key);
    if (key.length() <= max_length) {
      buf_.push_back(key.id());
    }
  } else if (trie.ith_node(node_id).child() == TERMINAL_LABEL) {
    const UInt32 terminal = base.offset() ^ TERMINAL_LABEL;
    Key key;
    trie.ith_key(trie.ith_node(terminal).key_id(), &key);
    if ((key.length() >= min_length) && (key.length() <= max_length)) {
      buf_.push_back(key.id());
    }
  }
}

void CommonPrefixSearchCursor::close() {
  trie_ = NULL;
  buf_.clear();
  count_ = 0;
}

bool CommonPrefixSearchCursor::next(Key *key) {
  if (count_ >= buf_.size()) {
    return false;
  }
  trie_->ith_key(buf_[count_++], key);
  return true;
}

}  // namespace grn
}  // namespace dat
