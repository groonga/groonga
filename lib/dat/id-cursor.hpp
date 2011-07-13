#ifndef GRN_DAT_ID_CURSOR_HPP_
#define GRN_DAT_ID_CURSOR_HPP_

#include "cursor.hpp"

namespace grn {
namespace dat {

class Trie;

class IdCursor : public Cursor {
 public:
  IdCursor();
  ~IdCursor();

  void open(const Trie &trie,
            const String &min_str,
            const String &max_str,
            UInt32 offset = 0,
            UInt32 limit = UINT32_MAX,
            UInt32 flags = 0);

  void open(const Trie &trie,
            UInt32 min_id,
            UInt32 max_id,
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

  UInt32 cur_;
  UInt32 end_;

  IdCursor(const Trie &trie, UInt32 offset, UInt32 limit, UInt32 flags);

  UInt32 fix_flags(UInt32 flags) const;
  void init(UInt32 min_id, UInt32 max_id);
  void swap(IdCursor *cursor);

  // Disallows copy and assignment.
  IdCursor(const IdCursor &);
  IdCursor &operator=(const IdCursor &);
};

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_ID_CURSOR_HPP_
