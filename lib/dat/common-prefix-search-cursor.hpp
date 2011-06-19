#ifndef GRN_DAT_COMMON_PREFIX_SEARCH_CURSOR_H
#define GRN_DAT_COMMON_PREFIX_SEARCH_CURSOR_H

#include "cursor.hpp"
#include "trie.hpp"

#include <vector>

namespace grn {
namespace dat {

class CommonPrefixSearchCursor : public Cursor {
 public:
  CommonPrefixSearchCursor()
      : trie_(NULL),
        buf_(),
        count_(0) {}
  ~CommonPrefixSearchCursor() {}

  void open(const Trie &trie,
            const void *ptr,
            UInt32 min_length,
            UInt32 max_length,
            UInt32 offset,
            UInt32 max_count);

  void close();

  bool next(Key *key);

 private:
  const Trie *trie_;
  std::vector<UInt32> buf_;
  UInt32 count_;

  // Disallows copy and assignment.
  CommonPrefixSearchCursor(const CommonPrefixSearchCursor &);
  CommonPrefixSearchCursor &operator=(const CommonPrefixSearchCursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_COMMON_PREFIX_SEARCH_CURSOR_H
