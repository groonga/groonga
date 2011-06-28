#ifndef GRN_DAT_COMMON_PREFIX_CURSOR_H
#define GRN_DAT_COMMON_PREFIX_CURSOR_H

#include "cursor.hpp"
#include "trie.hpp"

#include <vector>

namespace grn {
namespace dat {

class CommonPrefixCursor : public Cursor {
 public:
  CommonPrefixCursor();
  ~CommonPrefixCursor();

  void open(const Trie &trie,
            const void *ptr,
            UInt32 min_length,
            UInt32 max_length,
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

  UInt32 fix_flags(UInt32 flags) const;
  CommonPrefixCursor(const Trie &trie,
                     UInt32 offset, UInt32 limit, UInt32 flags);
  void init(const UInt8 *ptr, UInt32 min_length, UInt32 max_length);
  void swap(CommonPrefixCursor *cursor);

  // Disallows copy and assignment.
  CommonPrefixCursor(const CommonPrefixCursor &);
  CommonPrefixCursor &operator=(const CommonPrefixCursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_COMMON_PREFIX_CURSOR_H
