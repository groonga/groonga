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

/*
 * bit layout of generational lock
 *
 * [sggggggg ggggcccc cccccccc cccccccc] uint32_t
 *
 * s: sign 1 bit
 * g: generation id 11 bits
 * c: lock count 20 bits
 */

#define LOCK_COUNT_MASK   ((1 << (31 - 11)) - 1) // 20 bits
#define GEN_ID_MASK       (0xFFF00000) // 11 bits and sign bit
#define GEN_ID_SHIFT      (20)
#define GEN_ID_IDX(id)    ((id) >> 3)
#define GEN_ID_BIT(id)    (1 << ((id) & 0x7))

int
gen_file_is_valid(grn_gen *gen)
{
#ifdef WIN32
  return gen->hFile != INVALID_HANDLE_VALUE;
#else
  return gen->fd != -1;
#endif
}

int
gen_id_is_newer(grn_gen *gen, int id)
{
  if (id > gen->id) {
    return (id - gen->id < GRN_GEN_SIZE/2) ? 1 : 0;
  } else {
    return (gen->id - id > GRN_GEN_SIZE/2) ? 1 : 0;
  }
}

void
gen_flock(grn_gen *gen)
{
#ifdef WIN32
  LockFileEx((gen)->hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, TESTSTRLEN, 0, 0);
#else
  flock((gen)->fd, LOCK_EX);
#endif
}

int
gen_file_open(grn_gen *gen, const char *path)
{
#ifdef WIN32
    gen->hFile = OpenFile(path, NULL, OF_READWRITE);
#else
    gen->fd = open(path, O_RDWR);
#endif
    if (!gen_file_is_valid(gen)) return 0;
#ifdef WIN32
    return LockFileEx(gen->hFile,
        LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,
        0, TESTSTRLEN, 0, 0);
#else
    return flock(gen->fd, LOCK_EX|LOCK_NB) == 0;
#endif
}

void
gen_file_close(grn_gen *gen)
{
#ifdef WIN32
  CloseHandle(gen->hFile);
#else
  close(gen->fd);
#endif
}

int
gen_file_exist(const char *path)
{
#ifdef WIN32
  return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
#else
  return access(path, F_OK) == 0;
#endif
}

int
gen_file_new(grn_gen *gen, const char *path)
{
#ifdef WIN32
    gen->hFile = CreateFile(path, GENERIC_READ|GENERIC_WRITE, 0,
        NULL, CREATE_NEW, 0, 0, 0, NULL);
    return gen->hFile != INVALID_HANDLE_VALUE;
#else
    gen->fd = open(path, O_RDWR|O_CREAT|O_EXCL);
    return gen->fd != -1;
#endif
}

void
gen_path_fill(grn_ctx *ctx, grn_db *db, int id, char *path)
{
  const char *db_path = grn_obj_path(ctx, (grn_obj*)db);
  grn_snprintf(path, PATH_MAX, PATH_MAX, "%s.gen.%03X", db_path, id);
}

int
gen_is_alive(grn_ctx *ctx, grn_db *db, int id)
{
  char gen_path[PATH_MAX];
  gen_path_fill(ctx, db, id, gen_path);
  if (!gen_file_exist(gen_path)) return 0;
  grn_gen gen;
  if (gen_file_open(&gen, gen_path)) {
    gen_file_close(&gen);
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
  memset(gen->table, 0, GRN_GEN_SIZE/8);
  char gen_path[PATH_MAX];
  uint16_t min = GRN_GEN_SIZE - 1, max = 0;
  for(int i = 1; i < GRN_GEN_SIZE; i++){
    gen_path_fill(ctx, db, i, gen_path);
    if (gen_file_exist(gen_path)) {
      if (i < min) min = i;
      if (i > max) max = i;
      if (!gen_file_open(gen, gen_path)) {
        GRN_LOG(ctx, GRN_OK, "found generation: id=%03x", i);
        gen->table[GEN_ID_IDX(i)] |= GEN_ID_BIT(i);
      } else {
        GRN_LOG(ctx, GRN_WARN, "found crashed generation: id=%03x", i);
        gen_file_close(gen);
        unlink(gen_path);
      }
    }
  }
  gen->id = (max - min < GRN_GEN_SIZE/2) ? max : min;
  for(int i = 0; i < GRN_GEN_SIZE/2; i++) {
    gen->id = (gen->id + 1) % GRN_GEN_SIZE;
    if (gen->id == 0) gen->id = 1; // gen id is in [1, GRN_GEN_SIZE-1]
    gen_path_fill(ctx, db, gen->id, gen_path);
    if (gen_file_new(gen, gen_path)) break;
  }
  if (!gen_file_is_valid(gen)) {
    GRN_FREE(gen);
    return GRN_TOO_MANY_OPEN_FILES;
  }
  gen->table[GEN_ID_IDX(gen->id)] |= GEN_ID_BIT(gen->id);
  gen_flock(gen);
  db->gen = gen;
  GRN_LOG(ctx, GRN_OK, "generation: id=%03x", gen->id);
  return GRN_SUCCESS;
}

grn_rc
grn_gen_fin(grn_ctx *ctx, grn_db *db)
{
  if (db->gen) {
    char gen_path[PATH_MAX];
    gen_path_fill(ctx, db, db->gen->id, gen_path);
    gen_file_close(db->gen);
    unlink(gen_path);
    GRN_FREE(db->gen);
    db->gen = NULL;
  }
  return GRN_SUCCESS;
}

int
grn_gen_lock(grn_ctx *ctx, grn_io *io, uint32_t count)
{
  grn_db *db = (grn_db*)ctx->impl->db;
  uint32_t count_check_border = 100;
  uint32_t lock;
  GRN_ATOMIC_ADD_EX(io->lock, 1, lock);
  if (lock & LOCK_COUNT_MASK) {
    GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
    if (db && db->gen) {
      int id = (lock & GEN_ID_MASK) >> GEN_ID_SHIFT;
      int idx = GEN_ID_IDX(id), bit = GEN_ID_BIT(id);
      if (db->gen->table[idx] & bit) { // known generation
        if (count % count_check_border == count_check_border - 1) {
          if (!gen_is_alive(ctx, db, id)) {
            GRN_LOG(ctx, GRN_WARN, "lost generation: id=%03X", id);
            db->gen->table[idx] &= ~bit; // forget it
          }
        }
      } else {
        if (gen_id_is_newer(db->gen, id)) { // check new commer
          if (gen_is_alive(ctx, db, id)) {
            GRN_LOG(ctx, GRN_OK, "found newer generation: id=%03X", id);
            db->gen->table[idx] |= bit;
            return 0;
          }
        }
        GRN_LOG(ctx, GRN_WARN, "crashed generation found: id=%03X", id);
        grn_io_clear_lock(io);
      }
    }
    return 0;
  }
  if (db && db->gen) {
    GRN_ATOMIC_ADD_EX(io->lock, db->gen->id << GEN_ID_SHIFT, lock);
  }
  return 1;
}

int
grn_gen_unlock(grn_io *io)
{
  uint32_t lock;
  GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
  if ((lock & LOCK_COUNT_MASK) == 1) {
    GRN_ATOMIC_ADD_EX(io->lock, -(lock & GEN_ID_MASK), lock);
    return 1;
  }
  return 0;
}
