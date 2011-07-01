#ifndef GRN_DAT_MEMORY_MAPPED_FILE_HPP_
#define GRN_DAT_MEMORY_MAPPED_FILE_HPP_

#include "dat.hpp"

namespace grn {
namespace dat {

class MemoryMappedFileImpl;

class MemoryMappedFile {
 public:
  MemoryMappedFile();
  ~MemoryMappedFile();

  void create(const char *path, UInt64 size);
  void open(const char *path);
  void close();

  void *ptr() const;
  UInt64 size() const;

  void swap(MemoryMappedFile *rhs);

 private:
  MemoryMappedFileImpl *impl_;

  // Disallows copy and assignment.
  MemoryMappedFile(const MemoryMappedFile &);
  MemoryMappedFile &operator=(const MemoryMappedFile &);
};

}  // namespace grn
}  // namespace dat

#endif  // GRN_DAT_MEMORY_MAPPED_FILE_HPP_
