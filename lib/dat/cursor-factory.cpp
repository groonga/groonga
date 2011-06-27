#include "cursor-factory.hpp"
#include "id-cursor.hpp"
#include "key-cursor.hpp"
#include "common-prefix-search-cursor.hpp"
#include "predictive-search-cursor.hpp"

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
      CommonPrefixSearchCursor *cursor = new CommonPrefixSearchCursor;
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
      PredictiveSearchCursor *cursor = new PredictiveSearchCursor;
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
      GRN_DAT_PARAM_ERROR_IF((cursor_type != ID_RANGE_CURSOR) &&
                             (cursor_type != KEY_RANGE_CURSOR) &&
                             (cursor_type != COMMON_PREFIX_CURSOR) &&
                             (cursor_type != PREDICTIVE_CURSOR));
    }
  }
  return NULL;
}

}  // namespace grn
}  // namespace dat
