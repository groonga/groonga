#include "cursor-factory.hpp"
#include "id-cursor.hpp"
#include "key-cursor.hpp"
#include "common-prefix-cursor.hpp"
#include "predictive-cursor.hpp"

namespace grn {
namespace dat {

Cursor *CursorFactory::open(const Trie &trie,
                            const void *min_ptr, UInt32 min_length,
                            const void *max_ptr, UInt32 max_length,
                            UInt32 offset,
                            UInt32 limit,
                            UInt32 flags) {
  const UInt32 cursor_type = flags & CURSOR_TYPE_MASK;
  switch (cursor_type) {
    case ID_RANGE_CURSOR: {
      IdCursor *cursor = new IdCursor;
      try {
        cursor->open(trie, min_ptr, min_length, max_ptr, max_length,
                     offset, limit, flags);
      } catch (...) {
        delete cursor;
        throw;
      }
      return cursor;
    }
    case KEY_RANGE_CURSOR: {
      KeyCursor *cursor = new KeyCursor;
      try {
        cursor->open(trie, min_ptr, min_length, max_ptr, max_length,
                     offset, limit, flags);
      } catch (...) {
        delete cursor;
        throw;
      }
      return cursor;
    }
    case COMMON_PREFIX_CURSOR: {
      CommonPrefixCursor *cursor = new CommonPrefixCursor;
      try {
        cursor->open(trie, max_ptr, min_length, max_length,
                     offset, limit, flags);
      } catch (...) {
        delete cursor;
        throw;
      }
      return cursor;
    }
    case PREDICTIVE_CURSOR: {
      PredictiveCursor *cursor = new PredictiveCursor;
      try {
        cursor->open(trie, min_ptr, min_length,
                     offset, limit, flags);
      } catch (...) {
        delete cursor;
        throw;
      }
      return cursor;
    }
    default: {
      GRN_DAT_THROW(PARAM_ERROR, "unknown cursor type");
    }
  }
  return NULL;
}

}  // namespace grn
}  // namespace dat
