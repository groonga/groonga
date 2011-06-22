#ifndef GRN_DAT_ID_CURSOR_H
#define GRN_DAT_ID_CURSOR_H

#include "cursor.hpp"
#include "trie.hpp"

namespace grn {
namespace dat {

class IdCursor : public Cursor {
 public:
  IdCursor() 
      : trie_(NULL),
        step_(1),
        cur_(INVALID_KEY_ID),
        end_(INVALID_KEY_ID) {}
  ~IdCursor() {}

  void open(const Trie &trie,
            const void *min_ptr,
            UInt32 min_length,
            const void *max_ptr,
            UInt32 max_length,
            UInt32 offset,
            UInt32 limit,
            bool is_ascending = true,
            bool ignores_min = false,
            bool ignores_max = false);

  bool next(Key *key);

 private:
  const Trie *trie_;
  int step_;
  UInt32 cur_;
  UInt32 end_;

  void open(const Trie &trie,
            UInt32 min_id,
            UInt32 max_id,
            UInt32 offset,
            UInt32 limit,
            bool is_ascending = true,
            bool ignores_min = false,
            bool ignores_max = false);

  // Disallows copy and assignment.
  IdCursor(const IdCursor &);
  IdCursor &operator=(const IdCursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_ID_CURSOR_H
