#ifndef GRN_DAT_CURSOR_H
#define GRN_DAT_CURSOR_H

#include "key.hpp"

namespace grn {
namespace dat {

class Cursor {
 public:
  Cursor() {}
  virtual ~Cursor() {}

//  virtual void open(const CursorQuery &query) = 0;
//  virtual void close() = 0;

  virtual bool next(Key *key) = 0;

 private:
  // Disallows copy and assignment.
  Cursor(const Cursor &);
  Cursor &operator=(const Cursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_CURSOR_H
