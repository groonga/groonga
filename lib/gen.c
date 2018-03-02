/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn.h"
#include "grn_ctx.h"
#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_gen.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOCK_COUNT_MASK   ((1 << (31 - 11)) - 1) // 20 bits
#define GEN_ID_MASK       (0xFFF00000) // 11 bits and sign bit

void
gen_flock(grn_gen *gen)
{
#ifdef _WIN32
  LockFileEx((gen)->hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, TESTSTRLEN, 0, 0);
#else
  flock((gen)->fd, LOCK_EX);
#endif
}

int
gen_flock_nb(grn_gen *gen)
{
#ifdef _WIN32
  return LockFileEx((gen)->hFile,
      LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY, 0, TESTSTRLEN, 0, 0);
#else
  return flock((gen)->fd, LOCK_EX|LOCK_NB) != -1;
#endif
}

int
gen_open_file(grn_gen *gen, const char *path)
{
#ifdef _WIN32
    gen->hFile = OpenFile(path, NULL, OF_READWRITE);
    return gen->hFile != HFILE_ERROR;
#else
    gen->fd = open(path, O_RDWR);
    return gen->fd != -1;
#endif
}

void
gen_close_file(grn_gen *gen)
{
#ifdef _WIN32
  CloseHandle(gen->hFile);
#else
  close(gen->fd);
#endif
}

int
gen_new_file(grn_gen *gen, const char *path)
{
#ifdef _WIN32
    gen->hFile = CreateFile(path, GENERIC_READ|GENERIC_WRITE, 0,
        NULL, CREATE_NEW, 0, FILE_FLAG_DELETE_ON_CLOSE, 0, NULL);
    return gen->hFile != INVALID_HANDLE_VALUE;
#else
    gen->fd = open(path, O_RDWR|O_CREAT|O_EXCL);
    return gen->fd != -1;
#endif
}

int
gen_is_valid_file(grn_gen *gen)
{
#ifdef _WIN32
  return gen->hFile != INVALID_HANDLE_VALUE;
#else
  return gen->fd != -1;
#endif
}

void
grn_gen_path_fill(grn_ctx *ctx, grn_db *db, int id, char *path)
{
  const char *db_path = grn_obj_path(ctx, (grn_obj*)db);
  grn_snprintf(path, PATH_MAX, PATH_MAX, "%s.gen.%03X", db_path, id);
}

int
grn_gen_is_alive(grn_ctx *ctx, grn_db *db, int id)
{
  grn_gen gen;
  char gen_path[PATH_MAX];
  grn_gen_path_fill(ctx, db, id, gen_path);
  if (!gen_open_file(&gen, gen_path)) return 0;
  if (gen_flock_nb(&gen)) {
    gen_close_file(&gen);
    return 0;
  }
  return 1;
}

grn_rc
grn_gen_init(grn_ctx *ctx, grn_db *db)
{
  if (db->gen) {
    return GRN_SUCCESS;
  }

  grn_gen *gen = GRN_MALLOC(sizeof(grn_gen));
  char gen_path[PATH_MAX];
  uint16_t min = GRN_GEN_SIZE - 1, max = 0;
  for(int i = 0; i < GRN_GEN_SIZE; i++){
    int n = i>>3, m = i&0x7, o = 1<<m;
    if (m == 0) {
      gen->table[n] = 0;
    }
    grn_gen_path_fill(ctx, db, i, gen_path);
    if (gen_open_file(gen, gen_path)) {
      if (i < min) min = i;
      if (i > max) max = i;
      if (!gen_flock_nb(gen)) { // alive
        gen->table[n] |= o;
      } else { // crashed
        gen_close_file(gen);
        unlink(gen_path);
      }
    }
  }
  gen->id = (max - min < GRN_GEN_SIZE/2) ? max : min;
  for(int i = 0; i < GRN_GEN_SIZE/2; i++) {
    gen->id = (gen->id + 1) % GRN_GEN_SIZE;
    if (gen->id == 0) gen->id = 1; // gen id is in [1, GRN_GEN_SIZE-1]
    grn_gen_path_fill(ctx, db, gen->id, gen_path);
    if (gen_new_file(gen, gen_path)) break;
  }
  if (!gen_is_valid_file(gen)) {
    GRN_FREE(gen);
    return GRN_TOO_MANY_OPEN_FILES;
  }
  gen_flock(gen);
  db->gen = gen;
  return GRN_SUCCESS;
}

grn_rc
grn_gen_fin(grn_ctx *ctx, grn_db *db)
{
  if (db->gen) {
    char gen_path[PATH_MAX];
    grn_gen_path_fill(ctx, db, db->gen->id, gen_path);
    gen_close_file(db->gen);
    unlink(gen_path);
    GRN_FREE(db->gen);
    db->gen = NULL;
  }
  return GRN_SUCCESS;
}

int
grn_gen_lock(grn_ctx *ctx, grn_io *io)
{
  grn_db *db = (grn_db*)ctx->impl->db;
  uint32_t lock;
  GRN_ATOMIC_ADD_EX(io->lock, 1, lock);
  if (lock) { // failed
    GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
    if (db && db->gen) {
      int id = (lock & GEN_ID_MASK) >> 20;
      int n = id>>3, m = id%7, o = 1<<m;
      if (db->gen->table[n]&o) { // has been active
        if (!grn_gen_is_alive(ctx, db, id)) { // now is not alive
          db->gen->table[n] &= ~o; // remove entry
        }
      } else { // locked by crashed gen
        grn_io_clear_lock(io);
      }
    }
    return 0;
  }
  // set gen id to lock
  if (db && db->gen) {
    GRN_ATOMIC_ADD_EX(io->lock, db->gen->id << 20, lock); // 11 bits of gen id
  }
  return 1;
}

int
grn_gen_unlock(grn_io *io)
{
  uint32_t lock;
  GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
  if ((lock & LOCK_COUNT_MASK) == 1) {
    // remove gen id
    grn_db *db = (grn_db*)io->locker->impl->db;
    if (db && db->gen) {
      GRN_ATOMIC_ADD_EX(io->lock, -(db->gen->id << 20), lock);
    }
    return 1;
  }
  return 0;
}
