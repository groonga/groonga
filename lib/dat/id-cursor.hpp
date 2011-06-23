#ifndef GRN_DAT_ID_CURSOR_H
#define GRN_DAT_ID_CURSOR_H

#include "cursor.hpp"
#include "trie.hpp"

namespace grn {
namespace dat {

class IdCursor : public Cursor {
 public:
  IdCursor();
  ~IdCursor();

  void open(const Trie &trie,
            const void *min_ptr, UInt32 min_length,
            const void *max_ptr, UInt32 max_length,
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

  UInt32 fix_flags(UInt32 flags) const;
  IdCursor(const Trie &trie, UInt32 offset, UInt32 limit, UInt32 flags);
  void init(UInt32 min_id, UInt32 max_id);
  void swap(IdCursor *cursor);

  // Disallows copy and assignment.
  IdCursor(const IdCursor &);
  IdCursor &operator=(const IdCursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_ID_CURSOR_H
