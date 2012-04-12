# Copyright(C) 2012 Brazil
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/dat
  )

set(LIBGROONGA_SOURCES
  com.c
  com.h
  ctx.c
  ctx.h
  ctx_impl.h
  dat.cpp
  dat.h
  db.c
  db.h
  expr.c
  geo.c
  geo.h
  groonga_in.h
  hash.c
  hash.h
  ii.c
  ii.h
  io.c
  io.h
  nfkc.c
  nfkc.h
  output.c
  output.h
  pat.c
  pat.h
  plugin.c
  plugin_in.h
  proc.c
  proc.h
  snip.c
  snip.h
  store.c
  store.h
  str.c
  str.h
  token.c
  token.h
  tokenizer.c
  util.c
  util.h
  )

add_library(groonga SHARED ${LIBGROONGA_SOURCES})
