#ifndef GRN_DAT_KEY_CURSOR_HPP_
#define GRN_DAT_KEY_CURSOR_HPP_

#include "cursor.hpp"
#include "trie.hpp"

#include <vector>

namespace grn {
namespace dat {

class KeyCursor : public Cursor {
 public:
  KeyCursor();
  ~KeyCursor();

  void open(const Trie &trie,
            const void *min_ptr, UInt32 min_length,
            const void *max_ptr, UInt32 max_length,
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
  UInt32 count_;
  UInt32 max_count_;
  bool end_;
  UInt8 *end_ptr_;
  UInt32 end_length_;

  UInt32 fix_flags(UInt32 flags) const;
  KeyCursor(const Trie &trie,
                         UInt32 offset, UInt32 limit, UInt32 flags);
  void init(const UInt8 *min_ptr, UInt32 min_length,
            const UInt8 *max_ptr, UInt32 max_length);
  void ascending_init(const UInt8 *min_ptr, UInt32 min_length,
                      const UInt8 *max_ptr, UInt32 max_length);
  void descending_init(const UInt8 *min_ptr, UInt32 min_length,
                       const UInt8 *max_ptr, UInt32 max_length);
  void swap(KeyCursor *cursor);

  bool ascending_next(Key *key);
  bool descending_next(Key *key);

  int compare(const Key &key,
              const UInt8 *ptr, UInt32 length,
              UInt32 offset) const;

  static const UInt32 POST_ORDER_FLAG = 0x80000000U;

  // Disallows copy and assignment.
  KeyCursor(const KeyCursor &);
  KeyCursor &operator=(const KeyCursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_KEY_CURSOR_HPP_
