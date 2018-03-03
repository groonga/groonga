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
 * [sGGGGGGG Ggggcccc cccccccc cccccccc] uint32_t
 *
 * s: sign 1 bit
 * G, g: generation id 11 bits
 *   G: table index 8 bits
 *   g: position in the table 3 bits
 * c: lock count 20 bits
 */

#define LOCK_COUNT_MASK   ((1 << (31 - 11)) - 1) // 20 bits
#define GEN_ID_MASK       (0xFFF00000) // 11 bits and sign bit
#define GEN_ID_SHIFT      (20)
#define GEN_ID_IDX(id)    ((id) >> 3)
#define GEN_ID_BIT(id)    (1 << ((id) & 0x7))

static grn_bool grn_gen_enabled = GRN_FALSE;
static uint32_t grn_gen_lock_count_mask = 0xFFFFFFFF;

void
grn_gen_enable()
{
  grn_gen_enabled = GRN_TRUE;
  grn_gen_lock_count_mask = LOCK_COUNT_MASK;
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
gen_path_fill(grn_ctx *ctx, grn_db *db, int id, char *path)
{
  const char *db_path = grn_obj_path(ctx, (grn_obj*)db);
  grn_snprintf(path, PATH_MAX, PATH_MAX, "%s.gen.%03X", db_path, id);
}

grn_bool
gen_is_alive(grn_ctx *ctx, grn_db *db, int id)
{
  char gen_path[PATH_MAX];
  gen_path_fill(ctx, db, id, gen_path);
  grn_file_lock file_lock;
  grn_file_lock_init(ctx, &file_lock, gen_path);
  if (!grn_file_lock_exist(ctx, &file_lock)) goto inactive;
  if (grn_file_lock_takeover(ctx, &file_lock)) {
    grn_file_lock_close(ctx, &file_lock);
    goto inactive;
  }
  grn_file_lock_fin(ctx, &file_lock);
  return GRN_TRUE;
inactive:
  grn_file_lock_fin(ctx, &file_lock);
  return GRN_FALSE;
}

grn_rc
grn_gen_init(grn_ctx *ctx, grn_db *db)
{
  if (!grn_gen_enabled) return GRN_SUCCESS;
  if (db->gen) return GRN_SUCCESS;

  grn_gen *gen = GRN_MALLOC(sizeof(grn_gen));
  memset(gen->table, 0, GRN_GEN_SIZE/8);
  char gen_path[PATH_MAX];
  uint16_t min = GRN_GEN_SIZE - 1, max = 0;
  for(int i = 1; i < GRN_GEN_SIZE; i++){
    gen_path_fill(ctx, db, i, gen_path);
    grn_file_lock file_lock;
    grn_file_lock_init(ctx, &file_lock, gen_path);
    if (grn_file_lock_exist(ctx, &file_lock)) {
      if (i < min) min = i;
      if (i > max) max = i;
      if (grn_file_lock_takeover(ctx, &file_lock)) {
        GRN_LOG(ctx, GRN_WARN, "found crashed generation: id=%03x", i);
        grn_file_lock_release(ctx, &file_lock);
      } else {
        GRN_LOG(ctx, GRN_LOG_INFO, "found generation: id=%03x", i);
        gen->table[GEN_ID_IDX(i)] |= GEN_ID_BIT(i);
      }
    }
    grn_file_lock_fin(ctx, &file_lock);
  }
  if (max == 0) { // you are the first generation to use this db
    grn_obj_clear_lock(ctx, db);
  }
  gen->id = (max - min < GRN_GEN_SIZE/2) ? max : min;
  for(int i = 0; i < GRN_GEN_SIZE/2; i++) {
    gen->id = (gen->id + 1) % GRN_GEN_SIZE;
    if (gen->id == 0) gen->id = 1; // gen id is in [1, GRN_GEN_SIZE-1]
    gen_path_fill(ctx, db, gen->id, gen_path);
    grn_file_lock_init(ctx, &gen->file_lock, gen_path);
    if (grn_file_lock_acquire(ctx, &gen->file_lock, 1000,
          "generational lock")) break;
    grn_file_lock_fin(ctx, &gen->file_lock);
  }
  gen->table[GEN_ID_IDX(gen->id)] |= GEN_ID_BIT(gen->id);
  grn_file_lock_exclusive(ctx, &gen->file_lock);
  db->gen = gen;
  GRN_LOG(ctx, GRN_LOG_INFO, "generation: id=%03x", gen->id);
  return GRN_SUCCESS;
}

grn_rc
grn_gen_fin(grn_ctx *ctx, grn_db *db)
{
  if (db->gen) {
    grn_file_lock_fin(ctx, &db->gen->file_lock);
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
  if (lock & grn_gen_lock_count_mask) {
    GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
    int id = (lock & GEN_ID_MASK) >> GEN_ID_SHIFT;
    if (id && db && db->gen) {
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
            GRN_LOG(ctx, GRN_LOG_INFO, "found newer generation: id=%03X", id);
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
  if ((lock & grn_gen_lock_count_mask) == 1) {
    GRN_ATOMIC_ADD_EX(io->lock, -(lock & GEN_ID_MASK), lock);
    return 1;
  }
  return 0;
}
