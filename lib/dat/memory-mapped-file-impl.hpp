#ifndef GRN_DAT_MEMORY_MAPPED_FILE_IMPL_H_
#define GRN_DAT_MEMORY_MAPPED_FILE_IMPL_H_

#include "dat.hpp"

#ifdef WIN32
#include <Windows.h>
#endif  // WIN32

namespace grn {
namespace dat {

class MemoryMappedFileImpl {
 public:
  MemoryMappedFileImpl();
  ~MemoryMappedFileImpl();

  void create(const char *path, UInt64 size);
  void open(const char *path);
  void close();

  void *ptr() const {
    return ptr_;
  }
  UInt64 size() const {
    return size_;
  }

  void swap(MemoryMappedFileImpl *rhs);

 private:
  void *ptr_;
  UInt64 size_;

#ifdef WIN32
  HANDLE file_;
  HANDLE map_;
  LPVOID addr_;
#else  // WIN32
  int fd_;
  void *addr_;
  ::size_t length_;
#endif  // WIN32

  void create_(const char *path, UInt64 size);
  void open_(const char *path);

  template <typename T>
  void swap(T &lhs, T &rhs) const;

  // Disallows copy and assignment.
  MemoryMappedFileImpl(const MemoryMappedFileImpl &);
  MemoryMappedFileImpl &operator=(const MemoryMappedFileImpl &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_MEMORY_MAPPED_FILE_IMPL_H_
