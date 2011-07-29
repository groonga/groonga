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

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_MEMORY_MAPPED_FILE_HPP_
