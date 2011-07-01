#include "memory-mapped-file-impl.hpp"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
# ifdef min
#  undef min
# endif  // min
# ifdef max
#  undef max
# endif  // max
#else  // WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif  // WIN32

#include <algorithm>
#include <limits>

namespace grn {
namespace dat {

#ifdef WIN32

MemoryMappedFileImpl::MemoryMappedFileImpl()
    : ptr_(NULL),
      size_(0),
      file_(INVALID_HANDLE_VALUE),
      map_(INVALID_HANDLE_VALUE),
      addr_(NULL) {}

MemoryMappedFileImpl::~MemoryMappedFileImpl() {
  if (addr_ != NULL) {
    ::UnmapViewOfFile(addr_);
  }

  if (map_ != INVALID_HANDLE_VALUE) {
    ::CloseHandle(map_);
  }

  if (file_ != INVALID_HANDLE_VALUE) {
    ::CloseHandle(file_);
  }
}

#else  // WIN32

MemoryMappedFileImpl::MemoryMappedFileImpl()
    : ptr_(NULL),
      size_(0),
      fd_(-1),
      addr_(MAP_FAILED),
      length_(0) {}

MemoryMappedFileImpl::~MemoryMappedFileImpl() {
  if (addr_ != MAP_FAILED) {
    ::munmap(addr_, length_);
  }

  if (fd_ != -1) {
    ::close(fd_);
  }
}

#endif  // WIN32

void MemoryMappedFileImpl::create(const char *path, UInt64 size) {
  GRN_DAT_THROW_IF(PARAM_ERROR, size == 0);
  GRN_DAT_THROW_IF(PARAM_ERROR,
      size > static_cast<UInt64>(std::numeric_limits< ::size_t>::max()));

  MemoryMappedFileImpl new_impl;
  new_impl.create_(path, size);
  new_impl.swap(this);
}

void MemoryMappedFileImpl::open(const char *path) {
  GRN_DAT_THROW_IF(PARAM_ERROR, path == NULL);
  GRN_DAT_THROW_IF(PARAM_ERROR, path[0] == '\0');

  MemoryMappedFileImpl new_impl;
  new_impl.open_(path);
  new_impl.swap(this);
}

void MemoryMappedFileImpl::close() {
  MemoryMappedFileImpl new_impl;
  new_impl.swap(this);
}

#ifdef WIN32

void MemoryMappedFileImpl::swap(MemoryMappedFileImpl *rhs) {
  std::swap(ptr_, rhs->ptr_);
  std::swap(size_, rhs->size_);
  std::swap(file_, rhs->file_);
  std::swap(map_, rhs->map_);
  std::swap(addr_, rhs->addr_);
}

void MemoryMappedFileImpl::create_(const char *path, UInt64 size) {
  if ((path != NULL) && (path[0] != '\0')) {
    file_ = ::CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    GRN_DAT_THROW_IF(IO_ERROR, file_ == INVALID_HANDLE_VALUE);

    const LONG size_low = static_cast<LONG>(size & 0xFFFFFFFFU);
    LONG size_high = static_cast<LONG>(size >> 32);
    const DWORD file_pos = ::SetFilePointer(file_, size_low, &size_high,
                                            FILE_BEGIN);
    GRN_DAT_THROW_IF(IO_ERROR, (file_pos == INVALID_SET_FILE_POINTER) &&
                               (::GetLastError() != 0));
    GRN_DAT_THROW_IF(IO_ERROR, ::SetEndOfFile(file_) == 0);

    map_ = ::CreateFileMapping(file_, NULL, PAGE_READWRITE, 0, 0, NULL);
    GRN_DAT_THROW_IF(IO_ERROR, map_ == INVALID_HANDLE_VALUE);
  } else {
    const DWORD size_low = static_cast<DWORD>(size & 0xFFFFFFFFU);
    const DWORD size_high = static_cast<DWORD>(size >> 32);

    map_ = ::CreateFileMapping(file_, NULL, PAGE_READWRITE,
                               size_high, size_low, NULL);
    GRN_DAT_THROW_IF(IO_ERROR, map_ == INVALID_HANDLE_VALUE);
  }

  addr_ = ::MapViewOfFile(map_, FILE_MAP_WRITE, 0, 0, 0);
  GRN_DAT_THROW_IF(IO_ERROR, addr_ == NULL);

  ptr_ = addr_;
  size_ = static_cast< ::size_t>(size);
}

void MemoryMappedFileImpl::open_(const char *path) {
  struct __stat64 st;
  GRN_DAT_THROW_IF(IO_ERROR, ::_stat64(path, &st) == -1);
  GRN_DAT_THROW_IF(IO_ERROR, st.st_size == 0);
  GRN_DAT_THROW_IF(IO_ERROR,
      static_cast<UInt64>(st.st_size) > std::numeric_limits< ::size_t>::max());

  file_ = ::CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  GRN_DAT_THROW_IF(IO_ERROR, file_ == NULL);

  map_ = ::CreateFileMapping(file_, NULL, PAGE_READWRITE, 0, 0, NULL);
  GRN_DAT_THROW_IF(IO_ERROR, map_ == NULL);

  addr_ = ::MapViewOfFile(map_, FILE_MAP_READ, 0, 0, 0);
  GRN_DAT_THROW_IF(IO_ERROR, addr_ == NULL);

  ptr_ = addr_;
  size_ = static_cast< ::size_t>(st.st_size);
}

#else  // WIN32

void MemoryMappedFileImpl::swap(MemoryMappedFileImpl *rhs) {
  std::swap(ptr_, rhs->ptr_);
  std::swap(size_, rhs->size_);
  std::swap(fd_, rhs->fd_);
  std::swap(addr_, rhs->addr_);
  std::swap(length_, rhs->length_);
}

void MemoryMappedFileImpl::create_(const char *path, UInt64 size) {
  GRN_DAT_THROW_IF(PARAM_ERROR,
      size > static_cast<UInt64>(std::numeric_limits< ::off_t>::max()));

  if ((path != NULL) && (path[0] != '\0')) {
    fd_ = ::open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    GRN_DAT_THROW_IF(IO_ERROR, fd_ == -1);

    const ::off_t file_size = static_cast< ::off_t>(size);
    GRN_DAT_THROW_IF(IO_ERROR, ::ftruncate(fd_, file_size) == -1);
  }

#ifdef MAP_ANONYMOUS
  const int flags = (fd_ == -1) ? (MAP_PRIVATE | MAP_ANONYMOUS) : MAP_SHARED;
#else  // MAP_ANONYMOUS
  const int flags = (fd_ == -1) ? (MAP_PRIVATE | MAP_ANON) : MAP_SHARED;
#endif  // MAP_ANONYMOUS

  length_ = static_cast< ::size_t>(size);
  addr_ = ::mmap(NULL, length_, PROT_READ | PROT_WRITE, flags, fd_, 0);
  GRN_DAT_THROW_IF(IO_ERROR, addr_ == MAP_FAILED);

  ptr_ = addr_;
  size_ = length_;
}

void MemoryMappedFileImpl::open_(const char *path) {
  struct stat st;
  GRN_DAT_THROW_IF(IO_ERROR, ::stat(path, &st) == -1);
  GRN_DAT_THROW_IF(IO_ERROR, (st.st_mode & S_IFMT) != S_IFREG);
  GRN_DAT_THROW_IF(IO_ERROR, st.st_size == 0);
  GRN_DAT_THROW_IF(IO_ERROR,
      static_cast<UInt64>(st.st_size) > std::numeric_limits< ::size_t>::max());

  fd_ = ::open(path, O_RDWR);
  GRN_DAT_THROW_IF(IO_ERROR, fd_ == -1);

  length_ = static_cast<std::size_t>(st.st_size);
  addr_ = ::mmap(NULL, length_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  GRN_DAT_THROW_IF(IO_ERROR, addr_ == MAP_FAILED);

  ptr_ = addr_;
  size_ = length_;
}

#endif  // WIN32

}  // namespace grn
}  // namespace dat
