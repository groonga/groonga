#include "cursor-factory.hpp"
#include "id-cursor.hpp"
#include "key-cursor.hpp"
#include "common-prefix-cursor.hpp"
#include "predictive-cursor.hpp"

#include <new>

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
      IdCursor *cursor = new (std::nothrow) IdCursor;
      GRN_DAT_THROW_IF(MEMORY_ERROR, cursor == NULL);
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
      KeyCursor *cursor = new (std::nothrow) KeyCursor;
      GRN_DAT_THROW_IF(MEMORY_ERROR, cursor == NULL);
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
      CommonPrefixCursor *cursor = new (std::nothrow) CommonPrefixCursor;
      GRN_DAT_THROW_IF(MEMORY_ERROR, cursor == NULL);
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
      PredictiveCursor *cursor = new (std::nothrow) PredictiveCursor;
      GRN_DAT_THROW_IF(MEMORY_ERROR, cursor == NULL);
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
}

}  // namespace grn
}  // namespace dat
