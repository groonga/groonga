/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2011 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "memory-mapped-file.hpp"

#include <new>

#include "memory-mapped-file-impl.hpp"

namespace grn {
namespace dat {

MemoryMappedFile::MemoryMappedFile() : impl_(NULL) {}

MemoryMappedFile::~MemoryMappedFile() {
  delete impl_;
}

void MemoryMappedFile::create(const char *path, UInt64 size) {
  MemoryMappedFileImpl *new_impl = new (std::nothrow) MemoryMappedFileImpl;
  GRN_DAT_THROW_IF(MEMORY_ERROR, new_impl == NULL);
  try {
    new_impl->create(path, size);
  } catch (...) {
    delete new_impl;
    throw;
  }
  delete impl_;
  impl_ = new_impl;
}

void MemoryMappedFile::open(const char *path) {
  MemoryMappedFileImpl *new_impl = new (std::nothrow) MemoryMappedFileImpl;
  GRN_DAT_THROW_IF(MEMORY_ERROR, new_impl == NULL);
  try {
    new_impl->open(path);
  } catch (...) {
    delete new_impl;
    throw;
  }
  delete impl_;
  impl_ = new_impl;
}

void MemoryMappedFile::close() {
  delete impl_;
  impl_ = NULL;
}

void *MemoryMappedFile::ptr() const {
  return (impl_ != NULL) ? impl_->ptr() : NULL;
}

UInt64 MemoryMappedFile::size() const {
  return (impl_ != NULL) ? impl_->size() : 0;
}

void MemoryMappedFile::swap(MemoryMappedFile *rhs) {
  MemoryMappedFileImpl * const temp = impl_;
  impl_ = rhs->impl_;
  rhs->impl_ = temp;
}

}  // namespace dat
}  // namespace grn
