#ifndef GRN_DAT_KEY_CURSOR_HPP_
#define GRN_DAT_KEY_CURSOR_HPP_

#include "cursor.hpp"
#include "vector.hpp"

namespace grn {
namespace dat {

class Trie;

class KeyCursor : public Cursor {
 public:
  KeyCursor();
  ~KeyCursor();

  void open(const Trie &trie,
            const String &min_str,
            const String &max_str,
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

  Vector<UInt32> buf_;
  UInt32 count_;
  UInt32 max_count_;
  bool finished_;
  UInt8 *end_buf_;
  String end_str_;

  KeyCursor(const Trie &trie,
            UInt32 offset, UInt32 limit, UInt32 flags);

  UInt32 fix_flags(UInt32 flags) const;
  void init(const String &min_str, const String &max_str);
  void ascending_init(const String &min_str, const String &max_str);
  void descending_init(const String &min_str, const String &max_str);
  void swap(KeyCursor *cursor);

  bool ascending_next(Key *key);
  bool descending_next(Key *key);

  static const UInt32 POST_ORDER_FLAG = 0x80000000U;

  // Disallows copy and assignment.
  KeyCursor(const KeyCursor &);
  KeyCursor &operator=(const KeyCursor &);
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_KEY_CURSOR_HPP_
