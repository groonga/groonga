#ifndef GRN_DAT_CURSOR_H
#define GRN_DAT_CURSOR_H

#include "key.hpp"

namespace grn {
namespace dat {

class Cursor {
 public:
  Cursor() {}
  virtual ~Cursor() {}

  virtual bool next(Key *key) = 0;

 private:
  // Disallows copy and assignment.
  Cursor(const Cursor &);
  Cursor &operator=(const Cursor &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_CURSOR_H
