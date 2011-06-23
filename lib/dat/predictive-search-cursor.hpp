#ifndef GRN_DAT_PREDICTIVE_SEARCH_CURSOR_HPP_
#define GRN_DAT_PREDICTIVE_SEARCH_CURSOR_HPP_

#include "cursor.hpp"
#include "trie.hpp"

#include <vector>

namespace grn {
namespace dat {

class PredictiveSearchCursor : public Cursor {
 public:
  PredictiveSearchCursor();
  ~PredictiveSearchCursor();

  void open(const Trie &trie,
            const void *ptr,
            UInt32 length,
            UInt32 offset = 0,
            UInt32 limit = UINT32_MAX,
            UInt32 flags = 0);

  void close();

  bool next(Key *key);

  UInt32 offset() const {
    return offset_;
  }
  UInt32 limit() const {
    return limit_;
  }
  UInt32 flags() const {
    return flags_;
  }

 private:
  const Trie *trie_;
  UInt32 offset_;
  UInt32 limit_;
  UInt32 flags_;

  std::vector<UInt32> buf_;
  UInt32 cur_;
  UInt32 end_;
  UInt32 min_length_;

  UInt32 fix_flags(UInt32 flags) const;
  PredictiveSearchCursor(const Trie &trie,
                         UInt32 offset, UInt32 limit, UInt32 flags);
  void init(const UInt8 *ptr, UInt32 length);
  void swap(PredictiveSearchCursor *cursor);

  bool ascending_next(Key *key);
  bool descending_next(Key *key);
  void push_next_nodes(UInt32 node_id);

  static const UInt32 POST_ORDER_FLAG = 0x80000000U;

  // Disallows copy and assignment.
  PredictiveSearchCursor(const PredictiveSearchCursor &);
  PredictiveSearchCursor &operator=(const PredictiveSearchCursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_PREDICTIVE_SEARCH_CURSOR_HPP_
