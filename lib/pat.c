/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2023  Sutou Kouhei <kou@clear-code.com>

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
#include <string.h>
#include <limits.h>
#include "grn_ctx_impl.h"
#include "grn_pat.h"
#include "grn_obj.h"
#include "grn_output.h"
#include "grn_util.h"
#include "grn_normalizer.h"
#include "grn_wal.h"

#define GRN_PAT_DELETED       (GRN_ID_MAX + 1)

#define GRN_PAT_SEGMENT_SIZE  0x400000
#define W_OF_KEY_IN_A_SEGMENT 22
#define W_OF_PAT_IN_A_SEGMENT 18
#define W_OF_SIS_IN_A_SEGMENT 19
#define KEY_MASK_IN_A_SEGMENT 0x3fffff
#define PAT_MASK_IN_A_SEGMENT 0x3ffff
#define SIS_MASK_IN_A_SEGMENT 0x7ffff
#define SEG_NOT_ASSIGNED      0xffff
#define GRN_PAT_MAX_SEGMENT   0x1000
#define GRN_PAT_MDELINFOS     (GRN_PAT_NDELINFOS - 1)

#define GRN_PAT_BIN_KEY       0x70000

typedef enum {
  DIRECTION_LEFT = 0,
  DIRECTION_RIGHT = 1,
} pat_node_direction;

typedef struct {
  grn_id lr[2];
  /*
    lr[0]: the left node.
    lr[1]: the right node.

    The left node has 0 at the nth bit at the nth byte.
    The right node has 1 at the nth bit at the nth byte.
    'check' value indicate 'at the nth bit at the nth byte'.

    The both available nodes has larger check value rather
    than the current node.

    The first node (PAT_AT(pat, GRN_ID_NIL, node)) has only
    the right node and the node is the start point.
   */
  uint32_t key;
  /*
    PAT_IMD(node) == 0: key bytes offset in memory map.
    PAT_IMD(node) == 1: the key bytes.
   */
  uint16_t check;
  /*
    nth byte: 12, nth bit: 3, terminated: 1

    nth byte is different in key bytes: (check >> 4): max == 4095
    the left most byte is the 0th byte and the right most byte is the 11th byte.

    nth bit is different in nth byte: ((check >> 1) & 0b111)
    the left most bit is the 0th bit and the right most bit is the 7th bit.

    terminated: (check & 0b1)
    terminated == 1: key is terminated.
   */
  uint16_t bits;
  /* length: 13, immediate: 1, deleting: 1 */
} pat_node;

#define PAT_DELETING  (1 << 1)
#define PAT_IMMEDIATE (1 << 2)

grn_inline static bool
pat_key_is_embeddable(uint32_t key_size)
{
  return key_size <= sizeof(uint32_t);
}

grn_inline static uint32_t
pat_key_storage_size(uint32_t key_size)
{
  return pat_key_is_embeddable(key_size) ? 0 : key_size;
}

#define PAT_DEL(x)     ((x)->bits & PAT_DELETING)
#define PAT_IMD(x)     ((x)->bits & PAT_IMMEDIATE)
#define PAT_LEN(x)     (uint32_t)(((x)->bits >> 3) + 1)
#define PAT_CHK(x)     ((x)->check)
#define PAT_DEL_ON(x)  ((x)->bits |= PAT_DELETING)
#define PAT_IMD_ON(x)  ((x)->bits |= PAT_IMMEDIATE)
#define PAT_DEL_OFF(x) ((x)->bits &= ~PAT_DELETING)
#define PAT_IMD_OFF(x) ((x)->bits &= ~PAT_IMMEDIATE)
#define PAT_LEN_SET(x, v)                                                      \
  ((x)->bits = ((x)->bits & ((1 << 3) - 1)) | (((v)-1) << 3))
#define PAT_CHK_SET(x, v)                ((x)->check = (v))

#define PAT_CHECK_BYTE_DIFFERENCES_SHIFT 4
#define PAT_CHECK_BIT_DIFFERENCES_SHIFT  1
#define PAT_CHECK_PACK(byte_differences, bit_differences, terminated)          \
  (((byte_differences) << PAT_CHECK_BYTE_DIFFERENCES_SHIFT) +                  \
   ((bit_differences) << PAT_CHECK_BIT_DIFFERENCES_SHIFT) +                    \
   ((terminated) ? 1 : 0))
#define PAT_CHECK_BYTE_DIFFERENCES(check)                                      \
  ((check) >> PAT_CHECK_BYTE_DIFFERENCES_SHIFT)
#define PAT_CHECK_BIT_DIFFERENCES(check)                                       \
  (((check) >> PAT_CHECK_BIT_DIFFERENCES_SHIFT) & 0b111)
#define PAT_CHECK_IS_TERMINATED(check) ((check)&1)
#define PAT_CHECK_ADD_BIT_DIFFERENCES(check, n)                                \
  ((check) + ((n) << PAT_CHECK_BIT_DIFFERENCES_SHIFT))

typedef struct {
  grn_id children;
  grn_id sibling;
} sis_node;

enum {
  SEGMENT_KEY = 0,
  SEGMENT_PAT = 1,
  SEGMENT_SIS = 2
};

void
grn_p_pat_node(grn_ctx *ctx, grn_pat *pat, pat_node *node);

/* WAL operation */

typedef struct {
  grn_pat *pat;
  uint64_t wal_id;
  const char *tag;
  grn_wal_event event;
  grn_id record_id;
  pat_node_direction record_direction;
  const uint8_t *key;
  uint32_t key_size;
  uint32_t key_offset;
  uint32_t shared_key_offset;
  bool is_shared;
  uint16_t check;
  grn_id parent_record_id;
  pat_node_direction parent_record_direction;
  uint16_t parent_check;
  grn_id grandparent_record_id;
  grn_id otherside_record_id;
  uint16_t otherside_check;
  grn_id left_record_id;
  grn_id right_record_id;
  grn_id next_garbage_record_id;
  uint32_t n_garbages;
  uint32_t n_entries;
  uint32_t delete_info_index;
  uint32_t delete_info_phase1_index;
  uint32_t delete_info_phase2_index;
  uint32_t delete_info_phase3_index;
  grn_id previous_record_id;
} grn_pat_wal_add_entry_data;

typedef struct {
  bool record_id;
  bool record_direction;
  bool key;
  bool key_size;
  bool key_offset;
  bool shared_key_offset;
  bool is_shared;
  bool check;
  bool parent_record_id;
  bool parent_record_direction;
  bool parent_check;
  bool grandparent_record_id;
  bool otherside_record_id;
  bool otherside_check;
  bool left_record_id;
  bool right_record_id;
  bool next_garbage_record_id;
  bool n_garbages;
  bool n_entries;
  bool delete_info_index;
  bool delete_info_phase1_index;
  bool delete_info_phase2_index;
  bool delete_info_phase3_index;
  bool previous_record_id;
} grn_pat_wal_add_entry_used;

static grn_rc
grn_pat_wal_add_entry_add_entry(grn_ctx *ctx,
                                grn_pat_wal_add_entry_data *data,
                                grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->record_direction = true;
  used->key = true;
  used->key_size = true;
  used->key_offset = true;
  used->check = true;
  used->parent_record_id = true;
  used->n_entries = true;
  used->previous_record_id = true;
  const void *key = data->key;
  size_t key_size = data->key_size;
  uint32_t record_direction = data->record_direction;
  uint32_t check = data->check;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_RECORD_DIRECTION,
                           GRN_WAL_VALUE_UINT32,
                           record_direction,

                           GRN_WAL_KEY_KEY,
                           GRN_WAL_VALUE_BINARY,
                           key,
                           key_size,

                           GRN_WAL_KEY_KEY_OFFSET,
                           GRN_WAL_VALUE_UINT32,
                           data->key_offset,

                           GRN_WAL_KEY_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           check,

                           GRN_WAL_KEY_PARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->parent_record_id,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_PREVIOUS_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->previous_record_id,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_add_shared_entry(grn_ctx *ctx,
                                       grn_pat_wal_add_entry_data *data,
                                       grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->record_direction = true;
  used->key = true;
  used->key_size = true;
  used->shared_key_offset = true;
  used->check = true;
  used->parent_record_id = true;
  used->n_entries = true;
  used->previous_record_id = true;
  const void *key = data->key;
  size_t key_size = data->key_size;
  uint32_t check = data->check;
  uint32_t record_direction = data->record_direction;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_RECORD_DIRECTION,
                           GRN_WAL_VALUE_UINT32,
                           record_direction,

                           GRN_WAL_KEY_KEY,
                           GRN_WAL_VALUE_BINARY,
                           key,
                           key_size,

                           GRN_WAL_KEY_SHARED_KEY_OFFSET,
                           GRN_WAL_VALUE_UINT32,
                           data->shared_key_offset,

                           GRN_WAL_KEY_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           check,

                           GRN_WAL_KEY_PARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->parent_record_id,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_PREVIOUS_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->previous_record_id,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_reuse_entry(grn_ctx *ctx,
                                  grn_pat_wal_add_entry_data *data,
                                  grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->record_direction = true;
  used->key = true;
  used->key_size = true;
  used->check = true;
  used->parent_record_id = true;
  used->next_garbage_record_id = true;
  used->n_garbages = true;
  used->n_entries = true;
  used->previous_record_id = true;
  const void *key = data->key;
  size_t key_size = data->key_size;
  uint32_t record_direction = data->record_direction;
  uint32_t check = data->check;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_RECORD_DIRECTION,
                           GRN_WAL_VALUE_UINT32,
                           record_direction,

                           GRN_WAL_KEY_KEY,
                           GRN_WAL_VALUE_BINARY,
                           key,
                           key_size,

                           GRN_WAL_KEY_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           check,

                           GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->next_garbage_record_id,

                           GRN_WAL_KEY_PARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->parent_record_id,

                           GRN_WAL_KEY_N_GARBAGES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_garbages,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_PREVIOUS_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->previous_record_id,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_reuse_shared_entry(grn_ctx *ctx,
                                         grn_pat_wal_add_entry_data *data,
                                         grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->record_direction = true;
  used->key = true;
  used->key_size = true;
  used->shared_key_offset = true;
  used->check = true;
  used->parent_record_id = true;
  used->next_garbage_record_id = true;
  used->n_garbages = true;
  used->n_entries = true;
  used->previous_record_id = true;
  const void *key = data->key;
  size_t key_size = data->key_size;
  uint32_t check = data->check;
  uint32_t record_direction = data->record_direction;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_RECORD_DIRECTION,
                           GRN_WAL_VALUE_UINT32,
                           record_direction,

                           GRN_WAL_KEY_KEY,
                           GRN_WAL_VALUE_BINARY,
                           key,
                           key_size,

                           GRN_WAL_KEY_SHARED_KEY_OFFSET,
                           GRN_WAL_VALUE_UINT32,
                           data->shared_key_offset,

                           GRN_WAL_KEY_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           check,

                           GRN_WAL_KEY_PARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->parent_record_id,

                           GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->next_garbage_record_id,

                           GRN_WAL_KEY_N_GARBAGES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_garbages,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_PREVIOUS_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->previous_record_id,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_delete_info_phase1(grn_ctx *ctx,
                                         grn_pat_wal_add_entry_data *data,
                                         grn_pat_wal_add_entry_used *used)
{
  used->is_shared = true;
  used->delete_info_phase1_index = true;
  int is_shared = data->is_shared;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_IS_SHARED,
                           GRN_WAL_VALUE_BOOLEAN,
                           is_shared,

                           GRN_WAL_KEY_DELETE_INFO_PHASE1_INDEX,
                           GRN_WAL_VALUE_UINT32,
                           data->delete_info_phase1_index,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_delete_info_phase2(grn_ctx *ctx,
                                         grn_pat_wal_add_entry_data *data,
                                         grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->key_size = true;
  used->check = true;
  used->parent_record_id = true;
  used->left_record_id = true;
  used->right_record_id = true;
  used->delete_info_phase2_index = true;
  uint32_t check = data->check;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_KEY_SIZE,
                           GRN_WAL_VALUE_UINT32,
                           data->key_size,

                           GRN_WAL_KEY_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           check,

                           GRN_WAL_KEY_PARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->parent_record_id,

                           GRN_WAL_KEY_LEFT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->left_record_id,

                           GRN_WAL_KEY_RIGHT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->right_record_id,

                           GRN_WAL_KEY_DELETE_INFO_PHASE2_INDEX,
                           GRN_WAL_VALUE_UINT32,
                           data->delete_info_phase2_index,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_delete_info_phase3(grn_ctx *ctx,
                                         grn_pat_wal_add_entry_data *data,
                                         grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->is_shared = true;
  used->check = true;
  used->next_garbage_record_id = true;
  used->delete_info_phase3_index = true;
  int is_shared = data->is_shared;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_IS_SHARED,
                           GRN_WAL_VALUE_BOOLEAN,
                           is_shared,

                           GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->next_garbage_record_id,

                           GRN_WAL_KEY_DELETE_INFO_PHASE3_INDEX,
                           GRN_WAL_VALUE_UINT32,
                           data->delete_info_phase3_index,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_pat_wal_add_entry_delete_entry(grn_ctx *ctx,
                                   grn_pat_wal_add_entry_data *data,
                                   grn_pat_wal_add_entry_used *used)
{
  used->record_id = true;
  used->record_direction = true;
  used->check = true;
  used->parent_record_id = true;
  used->parent_record_direction = true;
  used->parent_check = true;
  used->grandparent_record_id = true;
  used->otherside_record_id = true;
  used->otherside_check = true;
  used->left_record_id = true;
  used->right_record_id = true;
  used->n_garbages = true;
  used->n_entries = true;
  used->delete_info_index = true;
  used->delete_info_phase1_index = true;
  used->delete_info_phase2_index = true;
  uint32_t check = data->check;
  uint32_t parent_check = data->parent_check;
  uint32_t otherside_check = data->otherside_check;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->pat),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_RECORD_DIRECTION,
                           GRN_WAL_VALUE_UINT32,
                           data->record_direction,

                           GRN_WAL_KEY_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           check,

                           GRN_WAL_KEY_PARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->parent_record_id,

                           GRN_WAL_KEY_PARENT_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           parent_check,

                           GRN_WAL_KEY_PARENT_RECORD_DIRECTION,
                           GRN_WAL_VALUE_UINT32,
                           data->parent_record_direction,

                           GRN_WAL_KEY_GRANDPARENT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->grandparent_record_id,

                           GRN_WAL_KEY_OTHERSIDE_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->otherside_record_id,

                           GRN_WAL_KEY_OTHERSIDE_CHECK,
                           GRN_WAL_VALUE_UINT32,
                           otherside_check,

                           GRN_WAL_KEY_LEFT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->left_record_id,

                           GRN_WAL_KEY_RIGHT_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->right_record_id,

                           GRN_WAL_KEY_N_GARBAGES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_garbages,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_DELETE_INFO_INDEX,
                           GRN_WAL_VALUE_UINT32,
                           data->delete_info_index,

                           GRN_WAL_KEY_DELETE_INFO_PHASE1_INDEX,
                           GRN_WAL_VALUE_UINT32,
                           data->delete_info_phase1_index,

                           GRN_WAL_KEY_DELETE_INFO_PHASE2_INDEX,
                           GRN_WAL_VALUE_UINT32,
                           data->delete_info_phase2_index,

                           GRN_WAL_KEY_END);
}

static void
grn_pat_wal_add_entry_format_deatils(grn_ctx *ctx,
                                     grn_pat_wal_add_entry_data *data,
                                     grn_pat_wal_add_entry_used *used,
                                     grn_obj *details)
{
  grn_text_printf(ctx,
                  details,
                  "event:%u<%s> ",
                  data->event,
                  grn_wal_event_to_string(data->event));
  if (used->record_id) {
    grn_text_printf(ctx, details, "record-id:%u ", data->record_id);
  }
  if (used->record_direction) {
    grn_text_printf(ctx,
                    details,
                    "record-direction:%s ",
                    data->record_direction == DIRECTION_LEFT ? "left"
                                                             : "right");
  }
  if (used->key_size) {
    grn_text_printf(ctx, details, "key-size:%u ", data->key_size);
  }
  if (used->key_offset) {
    grn_text_printf(ctx, details, "key-offset:%u ", data->key_offset);
  }
  if (used->shared_key_offset) {
    grn_text_printf(ctx,
                    details,
                    "shared-key-offset:%u ",
                    data->shared_key_offset);
  }
  if (used->is_shared) {
    grn_text_printf(ctx,
                    details,
                    "is-shared:%s ",
                    data->is_shared ? "true" : "false");
  }
  if (used->check) {
    uint16_t check = data->check;
    grn_text_printf(ctx,
                    details,
                    "check:<%u|%u|%s>(%u) ",
                    PAT_CHECK_BYTE_DIFFERENCES(check),
                    PAT_CHECK_BIT_DIFFERENCES(check),
                    PAT_CHECK_IS_TERMINATED(check) ? "true" : "false",
                    check);
  }
  if (used->parent_record_id) {
    grn_text_printf(ctx,
                    details,
                    "parent-record-id:%u ",
                    data->parent_record_id);
  }
  if (used->parent_record_direction) {
    grn_text_printf(ctx,
                    details,
                    "parent-record-direction:%s ",
                    data->parent_record_direction == DIRECTION_LEFT ? "left"
                                                                    : "right");
  }
  if (used->grandparent_record_id) {
    grn_text_printf(ctx,
                    details,
                    "grandparent-record-id:%u ",
                    data->grandparent_record_id);
  }
  if (used->parent_check) {
    uint16_t check = data->parent_check;
    grn_text_printf(ctx,
                    details,
                    "parent-check:<%u|%u|%s>(%u) ",
                    PAT_CHECK_BYTE_DIFFERENCES(check),
                    PAT_CHECK_BIT_DIFFERENCES(check),
                    PAT_CHECK_IS_TERMINATED(check) ? "true" : "false",
                    check);
  }
  if (used->otherside_record_id) {
    grn_text_printf(ctx,
                    details,
                    "otherside-record-id:%u ",
                    data->otherside_record_id);
  }
  if (used->otherside_check) {
    uint16_t check = data->otherside_check;
    grn_text_printf(ctx,
                    details,
                    "otherside-check:<%u|%u|%s>(%u) ",
                    PAT_CHECK_BYTE_DIFFERENCES(check),
                    PAT_CHECK_BIT_DIFFERENCES(check),
                    PAT_CHECK_IS_TERMINATED(check) ? "true" : "false",
                    check);
  }
  if (used->left_record_id) {
    grn_text_printf(ctx, details, "left-record-id:%u ", data->left_record_id);
  }
  if (used->right_record_id) {
    grn_text_printf(ctx, details, "right-record-id:%u ", data->right_record_id);
  }
  if (used->next_garbage_record_id) {
    grn_text_printf(ctx,
                    details,
                    "next-garbage-record-id:%u ",
                    data->next_garbage_record_id);
  }
  if (used->n_garbages) {
    grn_text_printf(ctx, details, "n-garbages:%u ", data->n_garbages);
  }
  if (used->n_entries) {
    grn_text_printf(ctx, details, "n-entries:%u ", data->n_entries);
  }
  if (used->delete_info_index) {
    grn_text_printf(ctx,
                    details,
                    "delete-info-index:%u ",
                    data->delete_info_index);
  }
  if (used->delete_info_phase1_index) {
    grn_text_printf(ctx,
                    details,
                    "delete-info-phase1-index:%u ",
                    data->delete_info_phase1_index);
  }
  if (used->delete_info_phase2_index) {
    grn_text_printf(ctx,
                    details,
                    "delete-info-phase2-index:%u ",
                    data->delete_info_phase2_index);
  }
  if (used->delete_info_phase3_index) {
    grn_text_printf(ctx,
                    details,
                    "delete-info-phase3-index:%u ",
                    data->delete_info_phase3_index);
  }
  if (used->previous_record_id) {
    grn_text_printf(ctx,
                    details,
                    "previous-record-id:%u ",
                    data->previous_record_id);
  }
}

grn_inline static grn_rc
grn_pat_wal_add_entry(grn_ctx *ctx, grn_pat_wal_add_entry_data *data)
{
  if (GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_NONE) {
    return GRN_SUCCESS;
  }

  if (data->pat->io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  grn_rc rc = GRN_SUCCESS;
  const char *usage = "";
  grn_pat_wal_add_entry_used used = {0};

  switch (data->event) {
  case GRN_WAL_EVENT_ADD_ENTRY:
    usage = "adding entry";
    rc = grn_pat_wal_add_entry_add_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_ADD_SHARED_ENTRY:
    usage = "adding shared entry";
    rc = grn_pat_wal_add_entry_add_shared_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_REUSE_ENTRY:
    usage = "reusing entry";
    rc = grn_pat_wal_add_entry_reuse_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_REUSE_SHARED_ENTRY:
    usage = "reusing shared entry";
    rc = grn_pat_wal_add_entry_reuse_shared_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_DELETE_INFO_PHASE1:
    usage = "deleting info phase1";
    rc = grn_pat_wal_add_entry_delete_info_phase1(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_DELETE_INFO_PHASE2:
    usage = "deleting info phase2";
    rc = grn_pat_wal_add_entry_delete_info_phase2(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_DELETE_INFO_PHASE3:
    usage = "deleting info phase3";
    rc = grn_pat_wal_add_entry_delete_info_phase3(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_DELETE_ENTRY:
    usage = "deleting entry";
    rc = grn_pat_wal_add_entry_delete_entry(ctx, data, &used);
    break;
  default:
    usage = "not implemented event";
    rc = GRN_FUNCTION_NOT_IMPLEMENTED;
    break;
  }

  if (rc != GRN_SUCCESS) {
    grn_obj details;
    GRN_TEXT_INIT(&details, 0);
    grn_pat_wal_add_entry_format_deatils(ctx, data, &used, &details);
    grn_obj_set_error(ctx,
                      (grn_obj *)(data->pat),
                      rc,
                      data->record_id,
                      data->tag,
                      "failed to add WAL entry for %s: %.*s",
                      usage,
                      (int)GRN_TEXT_LEN(&details),
                      GRN_TEXT_VALUE(&details));
    GRN_OBJ_FIN(ctx, &details);
  }

  return rc;
}

grn_inline static void
grn_pat_wal_add_entry_data_set_record_direction(
  grn_ctx *ctx,
  grn_pat_wal_add_entry_data *data,
  grn_id parent_record_id,
  pat_node *parent_node,
  grn_id *id_location)
{
  data->grandparent_record_id = data->parent_record_id;
  data->parent_record_direction = data->record_direction;
  data->parent_record_id = parent_record_id;
  if (id_location == &(parent_node->lr[DIRECTION_LEFT])) {
    data->record_direction = DIRECTION_LEFT;
  } else {
    data->record_direction = DIRECTION_RIGHT;
  }
  data->previous_record_id = *id_location;
}

/* bit operation */

#define nth_bit(key, check)                                                    \
  ((((key)[PAT_CHECK_BYTE_DIFFERENCES((check))]) >>                            \
    (0b111 - PAT_CHECK_BIT_DIFFERENCES((check)))) &                            \
   1)

/* segment operation */

/* patricia array operation */

#define PAT_AT(pat, id, n)                                                     \
  do {                                                                         \
    int flags = 0;                                                             \
    n = grn_io_array_at(ctx, pat->io, SEGMENT_PAT, id, &flags);                \
  } while (0)

grn_inline static pat_node *
pat_get(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  int flags = GRN_TABLE_ADD;
  if (id > GRN_ID_MAX) {
    return NULL;
  }
  return grn_io_array_at(ctx, pat->io, SEGMENT_PAT, id, &flags);
}

/* sis operation */

grn_inline static sis_node *
sis_at(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  int flags = 0;
  if (id > GRN_ID_MAX) {
    return NULL;
  }
  return grn_io_array_at(ctx, pat->io, SEGMENT_SIS, id, &flags);
}

grn_inline static sis_node *
sis_get(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  int flags = GRN_TABLE_ADD;
  if (id > GRN_ID_MAX) {
    return NULL;
  }
  return grn_io_array_at(ctx, pat->io, SEGMENT_SIS, id, &flags);
}

#define MAX_LEVEL 16

static void
sis_collect(grn_ctx *ctx, grn_pat *pat, grn_hash *h, grn_id id, uint32_t level)
{
  uint32_t *offset;
  sis_node *sl = sis_at(ctx, pat, id);
  if (sl) {
    grn_id sid = sl->children;
    while (sid && sid != id) {
      if (grn_hash_add(ctx, h, &sid, sizeof(grn_id), (void **)&offset, NULL)) {
        *offset = level;
        if (level < MAX_LEVEL) {
          sis_collect(ctx, pat, h, sid, level + 1);
        }
        if (!(sl = sis_at(ctx, pat, sid))) {
          break;
        }
        sid = sl->sibling;
      } else {
        /* todo : must be handled */
      }
    }
  }
}

/* key operation */

#define KEY_AT(pat, pos, ptr, addp)                                            \
  do {                                                                         \
    int flags = addp;                                                          \
    ptr = grn_io_array_at(ctx, pat->io, SEGMENT_KEY, pos, &flags);             \
  } while (0)

grn_inline static uint32_t
key_put(grn_ctx *ctx, grn_pat *pat, const uint8_t *key, uint32_t len)
{
  uint32_t res, ts;
  //  if (len >= GRN_PAT_SEGMENT_SIZE) { return 0; /* error */ }
  res = pat->header->curr_key;
  if (res < GRN_PAT_MAX_TOTAL_KEY_SIZE &&
      len > GRN_PAT_MAX_TOTAL_KEY_SIZE - res) {
    GRN_DEFINE_NAME(pat);
    ERR(GRN_NOT_ENOUGH_SPACE,
        "[pat][key][put] total key size is over: <%.*s>: "
        "max=%u: current=%u: new key size=%u",
        name_size,
        name,
        GRN_PAT_MAX_TOTAL_KEY_SIZE,
        res,
        len);
    return 0;
  }

  ts = (res + len) >> W_OF_KEY_IN_A_SEGMENT;
  if (res >> W_OF_KEY_IN_A_SEGMENT != ts) {
    res = pat->header->curr_key = ts << W_OF_KEY_IN_A_SEGMENT;
  }
  {
    uint8_t *dest;
    KEY_AT(pat, res, dest, GRN_TABLE_ADD);
    if (!dest) {
      GRN_DEFINE_NAME(pat);
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[pat][key][put] failed to allocate memory for new key: <%.*s>: "
          "new offset:%u key size:%u",
          name_size,
          name,
          res,
          len);
      return 0;
    }
    grn_memcpy(dest, key, len);
  }
  pat->header->curr_key += len;
  return res;
}

grn_inline static uint8_t *
pat_node_get_key(grn_ctx *ctx, grn_pat *pat, pat_node *n)
{
  if (PAT_IMD(n)) {
    return (uint8_t *)&(n->key);
  } else {
    uint8_t *res;
    KEY_AT(pat, n->key, res, 0);
    return res;
  }
}

grn_inline static grn_rc
pat_node_set_key(
  grn_ctx *ctx, grn_pat *pat, pat_node *n, const uint8_t *key, uint32_t len)
{
  grn_rc rc;
  if (!key || !len) {
    return GRN_INVALID_ARGUMENT;
  }
  PAT_LEN_SET(n, len);
  if (pat_key_is_embeddable(len)) {
    PAT_IMD_ON(n);
    grn_memcpy(&n->key, key, len);
    rc = GRN_SUCCESS;
  } else {
    PAT_IMD_OFF(n);
    n->key = key_put(ctx, pat, key, len);
    rc = ctx->rc;
  }
  return rc;
}

grn_inline static void
pat_node_set_shared_key(grn_ctx *ctx,
                        grn_pat *pat,
                        pat_node *node,
                        uint32_t key_size,
                        uint32_t shared_key_offset)
{
  PAT_IMD_OFF(node);
  PAT_LEN_SET(node, key_size);
  node->key = shared_key_offset;
}

/* delinfo operation */

enum {
  /* The delinfo is currently not used. */
  DL_EMPTY = 0,
  /*
   * stat->d refers to a deleting node (in a tree).
   * The deletion requires an additional operation.
   */
  DL_PHASE1,
  /*
   * stat->d refers to a deleted node (not in a tree).
   * The node is pending for safety.
   */
  DL_PHASE2
};

grn_inline static grn_id *
grn_pat_next_location(grn_ctx *ctx,
                      pat_node *node,
                      const uint8_t *key,
                      uint16_t check,
                      uint16_t check_max)
{
  if (PAT_CHECK_IS_TERMINATED(check)) {
    /* check + 1: delete terminated flag and increment bit differences */
    if (check + 1 < check_max) {
      return &(node->lr[DIRECTION_RIGHT]);
    } else {
      return &(node->lr[DIRECTION_LEFT]);
    }
  } else {
    return &(node->lr[nth_bit(key, check)]);
  }
}

grn_inline static grn_pat_delinfo *
delinfo_search(grn_pat *pat, grn_id id, uint32_t *index)
{
  uint32_t i;
  grn_pat_delinfo *di;
  for (i = (pat->header->curr_del2) & GRN_PAT_MDELINFOS;
       i != pat->header->curr_del;
       i = (i + 1) & GRN_PAT_MDELINFOS) {
    di = &pat->header->delinfos[i];
    if (di->stat != DL_PHASE1) {
      continue;
    }
    if (di->ld == id || di->d == id) {
      if (index) {
        *index = i;
      }
      return di;
    }
  }
  return NULL;
}

grn_inline static uint32_t
delinfo_turn_1_pre(grn_ctx *ctx, grn_pat *pat, grn_pat_delinfo **di)
{
  *di = &pat->header->delinfos[pat->header->curr_del];
  return (pat->header->curr_del + 1) & GRN_PAT_MDELINFOS;
}

grn_inline static void
delinfo_turn_1_post(grn_ctx *ctx, grn_pat *pat, uint32_t next_delete_info_index)
{
  pat->header->curr_del = next_delete_info_index;
}

grn_inline static grn_rc
delinfo_turn_2_internal(
  grn_ctx *ctx, grn_pat *pat, grn_pat_delinfo *di, pat_node *ln, pat_node *dn)
{
  grn_id d = di->d;
  grn_id *p = NULL;
  PAT_DEL_OFF(ln);
  PAT_DEL_OFF(dn);
  {
    grn_id *p0;
    pat_node *rn;
    int32_t c0 = -1, c;
    int32_t check_max = PAT_CHECK_PACK(PAT_LEN(dn), 0, false);
    const uint8_t *key = pat_node_get_key(ctx, pat, dn);
    if (!key) {
      return GRN_INVALID_ARGUMENT;
    }
    PAT_AT(pat, 0, rn);
    p0 = &rn->lr[1];
    for (;;) {
      grn_id r = *p0;
      if (!r) {
        break;
      }
      if (r == d) {
        p = p0;
        break;
      }
      PAT_AT(pat, r, rn);
      if (!rn) {
        return GRN_FILE_CORRUPT;
      }
      c = PAT_CHK(rn);
      if (c <= c0 || check_max <= c) {
        break;
      }
      p0 = grn_pat_next_location(ctx, rn, key, c, check_max);
      c0 = c;
    }
  }
  if (p) {
    PAT_CHK_SET(ln, PAT_CHK(dn));
    ln->lr[1] = dn->lr[1];
    ln->lr[0] = dn->lr[0];
    *p = di->ld;
  } else {
    /* debug */
    uint32_t j;
    grn_id dd;
    grn_pat_delinfo *ddi;
    GRN_LOG(ctx, GRN_LOG_DEBUG, "failed to find d=%d", d);
    for (j = (pat->header->curr_del2 + 1) & GRN_PAT_MDELINFOS;
         j != pat->header->curr_del;
         j = (j + 1) & GRN_PAT_MDELINFOS) {
      ddi = &pat->header->delinfos[j];
      if (ddi->stat != DL_PHASE1) {
        continue;
      }
      PAT_AT(pat, ddi->ld, ln);
      if (!ln) {
        continue;
      }
      if (!(dd = ddi->d)) {
        continue;
      }
      if (d == ddi->ld) {
        GRN_LOG(ctx, GRN_LOG_DEBUG, "found!!!, d(%d) become ld of (%d)", d, dd);
      }
    }
    /* debug */
  }
  di->stat = DL_PHASE2;
  di->d = d;
  // grn_log("delinfo_turn_2< di->d=%d di->ld=%d", di->d, di->ld);
  return GRN_SUCCESS;
}

grn_inline static grn_rc
delinfo_turn_2(grn_ctx *ctx,
               grn_pat *pat,
               grn_pat_wal_add_entry_data *base_wal_data,
               uint32_t di_index)
{
  grn_pat_delinfo *di = &pat->header->delinfos[di_index];
  pat_node *ln, *dn;
  // grn_log("delinfo_turn_2> di->d=%d di->ld=%d stat=%d", di->d, di->ld,
  // di->stat);
  if (di->stat != DL_PHASE1) {
    return GRN_SUCCESS;
  }
  PAT_AT(pat, di->ld, ln);
  if (!ln) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_id d = di->d;
  if (!d) {
    return GRN_INVALID_ARGUMENT;
  }
  PAT_AT(pat, d, dn);
  if (!dn) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_pat_wal_add_entry_data wal_data = *base_wal_data;
  wal_data.event = GRN_WAL_EVENT_DELETE_INFO_PHASE2;
  wal_data.record_id = d;
  wal_data.key_size = PAT_LEN(dn);
  wal_data.check = PAT_CHK(dn);
  wal_data.parent_record_id = di->ld;
  wal_data.left_record_id = dn->lr[0];
  wal_data.right_record_id = dn->lr[1];
  wal_data.delete_info_phase2_index = di_index;
  if (grn_pat_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return ctx->rc;
  }
  grn_rc rc = delinfo_turn_2_internal(ctx, pat, di, ln, dn);
  if (rc == GRN_SUCCESS) {
    pat->header->wal_id = wal_data.wal_id;
  }
  return rc;
}

grn_inline static void
delinfo_turn_2_post(grn_ctx *ctx, grn_pat *pat)
{
  pat->header->curr_del2 = (pat->header->curr_del2 + 1) & GRN_PAT_MDELINFOS;
}

grn_inline static uint32_t
delinfo_compute_storage_size(grn_ctx *ctx, grn_pat_delinfo *di, pat_node *dn)
{
  if (di->shared) {
    return 0;
  } else {
    if (PAT_IMD(dn)) {
      return 0;
    } else {
      return PAT_LEN(dn);
    }
  }
}

grn_inline static grn_rc
delinfo_turn_3_internal(grn_ctx *ctx,
                        grn_pat *pat,
                        grn_pat_delinfo *di,
                        pat_node *dn)
{
  if (di->shared) {
    PAT_IMD_ON(dn);
  }
  di->stat = DL_EMPTY;
  //  dn->lr[1] = GRN_PAT_DELETED;
  uint32_t size = delinfo_compute_storage_size(ctx, di, dn);
  dn->lr[0] = pat->header->garbages[size];
  pat->header->garbages[size] = di->d;
  return GRN_SUCCESS;
}

grn_inline static grn_rc
delinfo_turn_3(grn_ctx *ctx,
               grn_pat *pat,
               grn_pat_wal_add_entry_data *base_wal_data,
               uint32_t di_index)
{
  grn_pat_delinfo *di = &pat->header->delinfos[di_index];
  pat_node *dn;
  if (di->stat != DL_PHASE2) {
    return GRN_SUCCESS;
  }
  PAT_AT(pat, di->d, dn);
  if (!dn) {
    return GRN_INVALID_ARGUMENT;
  }
  uint32_t size = delinfo_compute_storage_size(ctx, di, dn);
  grn_pat_wal_add_entry_data wal_data = *base_wal_data;
  wal_data.event = GRN_WAL_EVENT_DELETE_INFO_PHASE3;
  wal_data.record_id = di->d;
  wal_data.is_shared = di->shared;
  wal_data.next_garbage_record_id = pat->header->garbages[size];
  wal_data.delete_info_phase3_index = di_index;
  if (grn_pat_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return ctx->rc;
  }
  grn_rc rc = delinfo_turn_3_internal(ctx, pat, di, dn);
  if (rc == GRN_SUCCESS) {
    pat->header->wal_id = wal_data.wal_id;
  }
  return rc;
}

grn_inline static void
delinfo_turn_3_post(grn_ctx *ctx, grn_pat *pat)
{
  pat->header->curr_del3 = (pat->header->curr_del3 + 1) & GRN_PAT_MDELINFOS;
}

grn_inline static grn_pat_delinfo *
delinfo_new(grn_ctx *ctx,
            grn_pat *pat,
            grn_pat_wal_add_entry_data *base_wal_data)
{
  base_wal_data->delete_info_index = pat->header->curr_del;
  grn_pat_wal_add_entry_data wal_data = *base_wal_data;
  wal_data.event = GRN_WAL_EVENT_DELETE_INFO_PHASE1;
  wal_data.delete_info_phase1_index = pat->header->curr_del;
  if (grn_pat_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return NULL;
  }
  grn_pat_delinfo *di;
  uint32_t next_delete_info_index = delinfo_turn_1_pre(ctx, pat, &di);
  di->shared = wal_data.is_shared;
  int gap =
    ((next_delete_info_index + GRN_PAT_NDELINFOS - pat->header->curr_del2) &
     GRN_PAT_MDELINFOS) -
    (GRN_PAT_NDELINFOS / 2);
  while (gap-- > 0) {
    if (delinfo_turn_2(ctx, pat, base_wal_data, pat->header->curr_del2)) {
      GRN_LOG(ctx,
              GRN_LOG_CRIT,
              "d2 failed: %d",
              pat->header->delinfos[pat->header->curr_del2].ld);
    }
    delinfo_turn_2_post(ctx, pat);
  }
  if (next_delete_info_index == pat->header->curr_del3) {
    if (delinfo_turn_3(ctx, pat, base_wal_data, pat->header->curr_del3)) {
      GRN_LOG(ctx,
              GRN_LOG_CRIT,
              "d3 failed: %d",
              pat->header->delinfos[pat->header->curr_del3].ld);
    }
    delinfo_turn_3_post(ctx, pat);
  }
  delinfo_turn_1_post(ctx, pat, next_delete_info_index);
  /* We can't do this because this overwrites WAL ID for phase2 and phase3. */
  /* pat->header->wal_id = wal_data.wal_id; */
  return di;
}

/* pat operation */

grn_inline static grn_pat *
_grn_pat_create(grn_ctx *ctx,
                grn_pat *pat,
                const char *path,
                uint32_t key_size,
                uint32_t value_size,
                uint32_t flags)
{
  grn_io *io;
  pat_node *node0;
  struct grn_pat_header *header;
  uint32_t entry_size, w_of_element;
  grn_encoding encoding = ctx->encoding;
  if (flags & GRN_OBJ_KEY_WITH_SIS) {
    entry_size = sizeof(sis_node) + value_size;
  } else {
    entry_size = value_size;
  }
  for (w_of_element = 0; (uint32_t)(1 << w_of_element) < entry_size;
       w_of_element++) {
    /* nop */
  }
  {
    grn_io_array_spec array_spec[3];
    array_spec[SEGMENT_KEY].w_of_element = 0;
    array_spec[SEGMENT_KEY].max_n_segments = 0x400;
    array_spec[SEGMENT_PAT].w_of_element = 4;
    array_spec[SEGMENT_PAT].max_n_segments = 1 << (30 - (22 - 4));
    array_spec[SEGMENT_SIS].w_of_element = w_of_element;
    array_spec[SEGMENT_SIS].max_n_segments = 1 << (30 - (22 - w_of_element));
    io = grn_io_create_with_array(ctx,
                                  path,
                                  sizeof(struct grn_pat_header),
                                  GRN_PAT_SEGMENT_SIZE,
                                  GRN_IO_AUTO,
                                  3,
                                  array_spec);
  }
  if (!io) {
    return NULL;
  }
  if (encoding == GRN_ENC_DEFAULT) {
    encoding = grn_gctx.encoding;
  }
  header = grn_io_header(io);
  grn_io_set_type(io, GRN_TABLE_PAT_KEY);
  header->flags = flags;
  header->encoding = encoding;
  header->key_size = key_size;
  header->value_size = value_size;
  header->n_entries = 0;
  header->curr_rec = 0;
  header->curr_key = 0;
  header->curr_del = 0;
  header->curr_del2 = 0;
  header->curr_del3 = 0;
  header->n_garbages = 0;
  header->tokenizer = GRN_ID_NIL;
  grn_table_modules_init(ctx, &(pat->normalizers));
  if (header->flags & GRN_OBJ_KEY_NORMALIZE) {
    header->flags &= ~GRN_OBJ_KEY_NORMALIZE;
    header->normalizer = GRN_ID_NIL;
    grn_obj *normalizer = grn_ctx_get(ctx, GRN_NORMALIZER_AUTO_NAME, -1);
    grn_table_modules_add(ctx, &(pat->normalizers), normalizer);
  } else {
    header->normalizer = GRN_ID_NIL;
  }
  header->truncated = GRN_FALSE;
  GRN_TEXT_INIT(&(pat->token_filters), 0);
  GRN_PTR_INIT(&(pat->token_filter_procs), GRN_OBJ_VECTOR, GRN_ID_NIL);
  pat->io = io;
  pat->header = header;
  pat->key_size = key_size;
  pat->value_size = value_size;
  grn_table_module_init(ctx, &(pat->tokenizer), GRN_ID_NIL);
  pat->encoding = encoding;
  pat->obj.header.flags = header->flags;
  if (!(node0 = pat_get(ctx, pat, 0))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  node0->lr[1] = 0;
  node0->lr[0] = 0;
  node0->key = 0;
  return pat;
}

grn_pat *
grn_pat_create(grn_ctx *ctx,
               const char *path,
               uint32_t key_size,
               uint32_t value_size,
               uint32_t flags)
{
  grn_pat *pat;
  if (!(pat = GRN_CALLOC(sizeof(grn_pat)))) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(pat, GRN_TABLE_PAT_KEY);
  if (!_grn_pat_create(ctx, pat, path, key_size, value_size, flags)) {
    GRN_FREE(pat);
    return NULL;
  }
  pat->cache = NULL;
  pat->cache_size = 0;
  pat->is_dirty = GRN_FALSE;
  CRITICAL_SECTION_INIT(pat->lock);
  return pat;
}

/*
 grn_pat_cache_enable() and grn_pat_cache_disable() are not thread-safe.
 So far, they can be used only from single threaded programs.
 */

grn_rc
grn_pat_cache_enable(grn_ctx *ctx, grn_pat *pat, uint32_t cache_size)
{
  grn_id *cache;

  if (cache_size & (cache_size - 1)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[pat][cache][enable] size must be a power of two: %u",
        cache_size);
    return ctx->rc;
  }
  if (cache_size <= pat->cache_size) {
    return GRN_SUCCESS;
  }
  if (!(cache = GRN_CALLOC(cache_size * sizeof(grn_id)))) {
    return ctx->rc;
  }
  if (pat->cache) {
    GRN_FREE(pat->cache);
  }
  pat->cache = cache;
  pat->cache_size = cache_size;

  return GRN_SUCCESS;
}

void
grn_pat_cache_disable(grn_ctx *ctx, grn_pat *pat)
{
  if (pat->cache) {
    GRN_FREE(pat->cache);
    pat->cache_size = 0;
    pat->cache = NULL;
  }
}

grn_pat *
grn_pat_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  grn_pat *pat;
  pat_node *node0;
  struct grn_pat_header *header;
  uint32_t io_type;
  io = grn_io_open(ctx, path, GRN_IO_AUTO);
  if (!io) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_UNKNOWN_ERROR;
    }
    ERR(rc,
        "[pat][open] failed to open grn_io: <%s>",
        path ? path : "(temporary)");
    return NULL;
  }
  header = grn_io_header(io);
  io_type = grn_io_get_type(io);
  if (io_type != GRN_TABLE_PAT_KEY) {
    ERR(GRN_INVALID_FORMAT,
        "[pat] file type must be %#04x: <%#04x>: <%s>",
        GRN_TABLE_PAT_KEY,
        io_type,
        path ? path : "(temporary)");
    grn_io_close(ctx, io);
    return NULL;
  }
  if (!(pat = GRN_CALLOC(sizeof(grn_pat)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[pat][open] failed to allocate memory: <%s>",
        path ? path : "(temporary)");
    grn_io_close(ctx, io);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(pat, GRN_TABLE_PAT_KEY);
  pat->io = io;
  pat->header = header;
  pat->key_size = header->key_size;
  pat->value_size = header->value_size;
  pat->encoding = header->encoding;
  grn_table_module_init(ctx, &(pat->tokenizer), header->tokenizer);
  grn_table_modules_init(ctx, &(pat->normalizers));
  if (header->flags & GRN_OBJ_KEY_NORMALIZE) {
    header->flags &= ~GRN_OBJ_KEY_NORMALIZE;
    header->normalizer = GRN_ID_NIL;
    grn_obj *normalizer = grn_ctx_get(ctx, GRN_NORMALIZER_AUTO_NAME, -1);
    grn_table_modules_add(ctx, &(pat->normalizers), normalizer);
  } else if (header->normalizer != GRN_ID_NIL) {
    grn_obj *normalizer = grn_ctx_at(ctx, header->normalizer);
    grn_table_modules_add(ctx, &(pat->normalizers), normalizer);
  }
  grn_table_modules_init(ctx, &(pat->token_filters));
  GRN_PTR_INIT(&(pat->token_filter_procs), GRN_OBJ_VECTOR, GRN_ID_NIL);
  pat->obj.header.flags = header->flags;
  PAT_AT(pat, 0, node0);
  if (!node0) {
    ERR(GRN_INVALID_FORMAT,
        "[pat][open] failed to get the root node: <%s>",
        path ? path : "(temporary)");
    grn_io_close(ctx, io);
    GRN_FREE(pat);
    return NULL;
  }
  pat->cache = NULL;
  pat->cache_size = 0;
  pat->is_dirty = GRN_FALSE;
  CRITICAL_SECTION_INIT(pat->lock);
  return pat;
}

/*
 * grn_pat_error_if_truncated() logs an error and returns its error code if
 * a pat is truncated by another process.
 * Otherwise, this function returns GRN_SUCCESS.
 * Note that `ctx` and `pat` must be valid.
 *
 * FIXME: A pat should be reopened if possible.
 */
static grn_rc
grn_pat_error_if_truncated(grn_ctx *ctx, grn_pat *pat)
{
  if (pat->header->truncated) {
    ERR(GRN_FILE_CORRUPT,
        "pat is truncated, please unmap or reopen the database");
    return GRN_FILE_CORRUPT;
  }
  return GRN_SUCCESS;
}

static void
grn_pat_close_token_filters(grn_ctx *ctx, grn_pat *pat)
{
  grn_table_modules_fin(ctx, &(pat->token_filters));
  GRN_OBJ_FIN(ctx, &(pat->token_filter_procs));
}

grn_rc
grn_pat_close(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc;

  CRITICAL_SECTION_FIN(pat->lock);

  if (pat->is_dirty) {
    uint32_t n_dirty_opens;
    GRN_ATOMIC_ADD_EX(&(pat->header->n_dirty_opens), -1, n_dirty_opens);
  }

  if (pat->io->path[0] != '\0' &&
      GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_PRIMARY) {
    grn_obj_flush(ctx, (grn_obj *)pat);
  }
  rc = grn_io_close(ctx, pat->io);
  if (rc != GRN_SUCCESS) {
    ERR(rc, "[pat][close] failed to close IO");
  }
  grn_table_module_fin(ctx, &(pat->tokenizer));
  grn_table_modules_fin(ctx, &(pat->normalizers));
  grn_pat_close_token_filters(ctx, pat);
  if (pat->cache) {
    grn_pat_cache_disable(ctx, pat);
  }
  GRN_FREE(pat);

  return rc;
}

grn_rc
grn_pat_remove(grn_ctx *ctx, const char *path)
{
  if (!path) {
    ERR(GRN_INVALID_ARGUMENT, "path is null");
    return GRN_INVALID_ARGUMENT;
  }
  grn_rc wal_rc = grn_wal_remove(ctx, path, "[pat]");
  grn_rc io_rc = grn_io_remove(ctx, path);
  grn_rc rc = wal_rc;
  if (rc == GRN_SUCCESS) {
    rc = io_rc;
  }
  return rc;
}

grn_rc
grn_pat_truncate(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc;
  const char *io_path;
  char *path;
  uint32_t key_size, value_size, flags;

  rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if ((io_path = grn_io_path(pat->io)) && *io_path != '\0') {
    if (!(path = GRN_STRDUP(io_path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_path);
      return GRN_NO_MEMORY_AVAILABLE;
    }
  } else {
    path = NULL;
  }
  key_size = pat->key_size;
  value_size = pat->value_size;
  flags = pat->obj.header.flags;
  if (path) {
    pat->header->truncated = GRN_TRUE;
  }
  if ((rc = grn_io_close(ctx, pat->io))) {
    goto exit;
  }
  grn_table_module_fin(ctx, &(pat->tokenizer));
  grn_table_modules_fin(ctx, &(pat->normalizers));
  grn_pat_close_token_filters(ctx, pat);
  pat->io = NULL;
  if (path && (rc = grn_pat_remove(ctx, path))) {
    goto exit;
  }
  if (!_grn_pat_create(ctx, pat, path, key_size, value_size, flags)) {
    rc = GRN_UNKNOWN_ERROR;
  }
  if (pat->cache && pat->cache_size) {
    memset(pat->cache, 0, pat->cache_size * sizeof(grn_id));
  }
exit:
  if (path) {
    GRN_FREE(path);
  }
  return rc;
}

static uint32_t
grn_pat_cache_compute_id(grn_ctx *ctx,
                         grn_pat *pat,
                         const uint8_t *key,
                         uint32_t size)
{
  const uint8_t *p = key;
  uint32_t length = size;
  uint32_t cache_id = 0;
  for (cache_id = 0; length--; p++) {
    cache_id = (cache_id * 37) + *p;
  }
  cache_id &= (pat->cache_size - 1);
  return cache_id;
}

typedef struct {
  grn_pat_wal_add_entry_data wal_data;
  bool added;
  uint32_t shared_key_offset;
  uint32_t cache_id;
  grn_id found_id;
  grn_id *last_id_location;
  int32_t check_max;
} grn_pat_add_data;

grn_inline static bool
grn_pat_add_internal_find(grn_ctx *ctx, grn_pat_add_data *data)
{
  grn_pat *pat = data->wal_data.pat;
  const uint8_t *key = data->wal_data.key;
  uint32_t key_size = data->wal_data.key_size;
  int32_t check_max = data->check_max;

  grn_id id = GRN_ID_NIL;
  pat_node *node;
  PAT_AT(pat, id, node);
  grn_id *id_location_previous = NULL;
  grn_id *id_location = &(node->lr[DIRECTION_RIGHT]);
  data->wal_data.record_direction = DIRECTION_RIGHT;
  grn_pat_wal_add_entry_data_set_record_direction(ctx,
                                                  &(data->wal_data),
                                                  id,
                                                  node,
                                                  id_location);
  if (*id_location == GRN_ID_NIL) {
    data->last_id_location = id_location;
    data->wal_data.check = PAT_CHECK_PACK(key_size - 1, 7, false);
    return true;
  }

  const uint8_t *found_key = NULL;
  uint32_t found_key_size = 0;
  grn_id id_previous = GRN_ID_NIL;
  pat_node *node_previous = NULL;
  int check_node = -1;
  int check_node_previous = -1;
  for (;;) {
    id_previous = id;
    id = *id_location;
    if (id == GRN_ID_NIL) {
      found_key = pat_node_get_key(ctx, pat, node);
      if (!found_key) {
        grn_obj_set_error(ctx,
                          (grn_obj *)pat,
                          GRN_INVALID_ARGUMENT,
                          id_previous,
                          data->wal_data.tag,
                          "failed to get key from node");
        return false;
      }
      found_key_size = PAT_LEN(node);
      break;
    }
    node_previous = node;
    PAT_AT(pat, id, node);
    if (!node) {
      grn_obj_set_error(ctx,
                        (grn_obj *)pat,
                        GRN_INVALID_ARGUMENT,
                        id,
                        data->wal_data.tag,
                        "failed to get node");
      return false;
    }
    if (check_node < node->check && node->check < check_max) {
      check_node_previous = check_node;
      check_node = node->check;
      id_location_previous = id_location;
      id_location =
        grn_pat_next_location(ctx, node, key, check_node, check_max);
      grn_pat_wal_add_entry_data_set_record_direction(ctx,
                                                      &(data->wal_data),
                                                      id,
                                                      node,
                                                      id_location);
    } else {
      found_key = pat_node_get_key(ctx, pat, node);
      if (!found_key) {
        grn_obj_set_error(ctx,
                          (grn_obj *)pat,
                          GRN_INVALID_ARGUMENT,
                          id,
                          data->wal_data.tag,
                          "failed to get key from node: "
                          "check_node:%d "
                          "node->check:%u "
                          "check_max:%u",
                          check_node,
                          node->check,
                          check_max);
        return false;
      }
      found_key_size = PAT_LEN(node);
      if (key_size == found_key_size && memcmp(found_key, key, key_size) == 0) {
        if (pat->cache) {
          pat->cache[data->cache_id] = id;
        }
        data->found_id = id;
        return true;
      }
      break;
    }
  }

  int32_t check;
  {
    uint32_t min = key_size > found_key_size ? found_key_size : key_size;
    uint16_t byte_differences = 0;
    const uint8_t *k1 = key;
    const uint8_t *k2 = found_key;
    for (; min > 0 && *k1 == *k2; byte_differences++, k1++, k2++, min--) {
    }
    if (min == 0) {
      check = PAT_CHECK_PACK(byte_differences - 1, 7, true);
    } else {
      uint8_t bit_differences = 0;
      int xor = *k1 ^ *k2;
      int mask = 0x80;
      for (; !(xor&mask); mask >>= 1, bit_differences++) {
      }
      check = PAT_CHECK_PACK(byte_differences, bit_differences, false);
    }
  }
  if (check == check_node && *id_location == GRN_ID_NIL) {
    if (check < (check_max - (1 << PAT_CHECK_BIT_DIFFERENCES_SHIFT))) {
      check = PAT_CHECK_ADD_BIT_DIFFERENCES(check, 1);
    }
  } else {
    if (check < check_node) {
      if (check > check_node_previous) {
        data->wal_data.record_direction =
          data->wal_data.parent_record_direction;
        data->wal_data.parent_record_id = data->wal_data.grandparent_record_id;
        id = id_previous;
        data->wal_data.previous_record_id = id_previous;
        id_location = id_location_previous;
      } else {
        id = GRN_ID_NIL;
        PAT_AT(pat, id, node);
        id_location = &(node->lr[DIRECTION_RIGHT]);
        data->wal_data.parent_record_direction = DIRECTION_RIGHT;
        grn_pat_wal_add_entry_data_set_record_direction(ctx,
                                                        &(data->wal_data),
                                                        id,
                                                        node,
                                                        id_location);
        while ((id = *id_location) != GRN_ID_NIL) {
          PAT_AT(pat, id, node);
          if (!node) {
            grn_obj_set_error(ctx,
                              (grn_obj *)pat,
                              GRN_INVALID_ARGUMENT,
                              id,
                              data->wal_data.tag,
                              "failed to get node to detect position");
            return false;
          }
          check_node = PAT_CHK(node);
          if (check < check_node) {
            break;
          }
          id_location =
            grn_pat_next_location(ctx, node, key, check_node, check_max);
          grn_pat_wal_add_entry_data_set_record_direction(ctx,
                                                          &(data->wal_data),
                                                          id,
                                                          node,
                                                          id_location);
        }
      }
    }
  }
  if (check >= check_max) {
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      GRN_INVALID_ARGUMENT,
                      id,
                      data->wal_data.tag,
                      "BUG: computed check is too large: "
                      "check:%u "
                      "check_max:%u "
                      "key_size:%u",
                      check,
                      check_max,
                      key_size);
    return false;
  }
  data->last_id_location = id_location;
  data->wal_data.check = check;
  return true;
}

grn_inline static void
grn_pat_enable_node(grn_ctx *ctx,
                    grn_pat *pat,
                    pat_node *node,
                    grn_id id,
                    const uint8_t *key,
                    uint16_t check,
                    uint16_t check_max,
                    grn_id *id_location)
{
  PAT_CHK_SET(node, check);
  PAT_DEL_OFF(node);
  if (PAT_CHECK_IS_TERMINATED(check)
        ?
        /* check + 1:
         * delete terminated flag and increment bit differences */
        (check + 1 < check_max)
        : nth_bit(key, check)) {
    node->lr[DIRECTION_RIGHT] = id;
    node->lr[DIRECTION_LEFT] = *id_location;
  } else {
    node->lr[DIRECTION_RIGHT] = *id_location;
    node->lr[DIRECTION_LEFT] = id;
  }
  // smp_wmb();
  *id_location = id;
}

grn_inline static void
grn_pat_reuse_shared_node(grn_ctx *ctx,
                          grn_pat *pat,
                          pat_node *node,
                          grn_id id,
                          const uint8_t *key,
                          uint32_t key_size,
                          uint32_t shared_key_offset,
                          uint16_t check,
                          uint16_t check_max,
                          grn_id *id_location)
{
  pat->header->garbages[0] = node->lr[0];
  pat->header->n_garbages--;
  pat->header->n_entries++;
  pat_node_set_shared_key(ctx, pat, node, key_size, shared_key_offset);
  grn_pat_enable_node(ctx, pat, node, id, key, check, check_max, id_location);
}

grn_inline static void
grn_pat_add_shared_node(grn_ctx *ctx,
                        grn_pat *pat,
                        pat_node *node,
                        grn_id id,
                        const uint8_t *key,
                        uint32_t key_size,
                        uint32_t shared_key_offset,
                        uint16_t check,
                        uint16_t check_max,
                        grn_id *id_location)
{
  pat->header->curr_rec = id;
  pat->header->n_entries++;
  pat_node_set_shared_key(ctx, pat, node, key_size, shared_key_offset);
  grn_pat_enable_node(ctx, pat, node, id, key, check, check_max, id_location);
}

grn_inline static grn_rc
grn_pat_reuse_node(grn_ctx *ctx,
                   grn_pat *pat,
                   pat_node *node,
                   grn_id id,
                   const uint8_t *key,
                   uint32_t key_size,
                   uint16_t check,
                   uint16_t check_max,
                   grn_id *id_location,
                   const char *tag)
{
  uint8_t *key_buffer;
  key_buffer = pat_node_get_key(ctx, pat, node);
  if (!key_buffer) {
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      GRN_FILE_CORRUPT,
                      id,
                      tag,
                      "failed to get key from node: "
                      "size:%u",
                      key_size);
    return ctx->rc;
  }
  uint32_t key_storage_size = pat_key_storage_size(key_size);
  pat->header->garbages[key_storage_size] = node->lr[0];
  PAT_LEN_SET(node, key_size);
  grn_memcpy(key_buffer, key, key_size);
  pat->header->n_garbages--;
  pat->header->n_entries++;
  grn_pat_enable_node(ctx, pat, node, id, key, check, check_max, id_location);
  return GRN_SUCCESS;
}

grn_inline static grn_rc
grn_pat_add_node(grn_ctx *ctx,
                 grn_pat *pat,
                 pat_node *node,
                 grn_id id,
                 const uint8_t *key,
                 uint32_t key_size,
                 uint16_t check,
                 uint16_t check_max,
                 grn_id *id_location,
                 const char *tag)
{
  grn_rc rc = pat_node_set_key(ctx, pat, node, key, key_size);
  if (rc != GRN_SUCCESS) {
    grn_obj inspected_key;
    GRN_TEXT_INIT(&inspected_key, 0);
    grn_inspect_key(ctx, &inspected_key, (grn_obj *)pat, key, key_size);
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      rc,
                      id,
                      tag,
                      "failed to set key: %.*s",
                      (int)GRN_TEXT_LEN(&inspected_key),
                      GRN_TEXT_VALUE(&inspected_key));
    GRN_OBJ_FIN(ctx, &inspected_key);
    return ctx->rc;
  }
  pat->header->curr_rec = id;
  pat->header->n_entries++;
  grn_pat_enable_node(ctx, pat, node, id, key, check, check_max, id_location);
  return GRN_SUCCESS;
}

grn_inline static grn_id
grn_pat_add_internal(grn_ctx *ctx, grn_pat_add_data *data)
{
  grn_pat *pat = data->wal_data.pat;
  const uint8_t *key = data->wal_data.key;
  uint32_t key_size = data->wal_data.key_size;

  data->added = false;
  data->cache_id = 0;
  if (pat->cache) {
    data->cache_id = grn_pat_cache_compute_id(ctx, pat, key, key_size);
    if (pat->cache[data->cache_id] != GRN_ID_NIL) {
      pat_node *node;
      PAT_AT(pat, pat->cache[data->cache_id], node);
      if (node) {
        const uint8_t *k = pat_node_get_key(ctx, pat, node);
        if (k && key_size == PAT_LEN(node) && memcmp(k, key, key_size) == 0) {
          return pat->cache[data->cache_id];
        }
      }
    }
  }

  data->check_max = PAT_CHECK_PACK(key_size, 0, false);
  data->found_id = GRN_ID_NIL;
  if (!grn_pat_add_internal_find(ctx, data)) {
    return GRN_ID_NIL;
  }
  if (data->found_id != GRN_ID_NIL) {
    return data->found_id;
  }

  data->wal_data.key_offset = pat->header->curr_key;
  pat_node *node = NULL;
  {
    uint32_t key_storage_size = pat_key_storage_size(key_size);
    if (data->shared_key_offset > 0 && key_storage_size > 0) {
      data->wal_data.shared_key_offset = data->shared_key_offset;
      if (pat->header->garbages[0] != GRN_ID_NIL) {
        data->wal_data.event = GRN_WAL_EVENT_REUSE_SHARED_ENTRY;
        data->wal_data.record_id = pat->header->garbages[0];
        PAT_AT(pat, data->wal_data.record_id, node);
        if (!node) {
          grn_obj_set_error(ctx,
                            (grn_obj *)pat,
                            GRN_INVALID_ARGUMENT,
                            data->wal_data.record_id,
                            data->wal_data.tag,
                            "failed to get node from garbage: "
                            "shared-key-offset:%u",
                            data->shared_key_offset);
          return GRN_ID_NIL;
        }
        data->wal_data.next_garbage_record_id = node->lr[0];
        data->wal_data.n_garbages = pat->header->n_garbages;
        data->wal_data.n_entries = pat->header->n_entries;
        if (grn_pat_wal_add_entry(ctx, &(data->wal_data)) != GRN_SUCCESS) {
          return GRN_ID_NIL;
        }
        grn_pat_reuse_shared_node(ctx,
                                  pat,
                                  node,
                                  data->wal_data.record_id,
                                  data->wal_data.key,
                                  data->wal_data.key_size,
                                  data->wal_data.shared_key_offset,
                                  data->wal_data.check,
                                  data->check_max,
                                  data->last_id_location);
        pat->header->wal_id = data->wal_data.wal_id;
      } else {
        data->wal_data.event = GRN_WAL_EVENT_ADD_SHARED_ENTRY;
        data->wal_data.record_id = pat->header->curr_rec + 1;
        data->wal_data.n_entries = pat->header->n_entries;
        if (grn_pat_wal_add_entry(ctx, &(data->wal_data)) != GRN_SUCCESS) {
          return GRN_ID_NIL;
        }
        node = pat_get(ctx, pat, data->wal_data.record_id);
        if (!node) {
          grn_obj_set_error(ctx,
                            (grn_obj *)pat,
                            GRN_INVALID_ARGUMENT,
                            data->wal_data.record_id,
                            data->wal_data.tag,
                            "failed to get node: "
                            "shared-key-offset:%u",
                            data->shared_key_offset);
          return GRN_ID_NIL;
        }
        grn_pat_add_shared_node(ctx,
                                pat,
                                node,
                                data->wal_data.record_id,
                                data->wal_data.key,
                                data->wal_data.key_size,
                                data->wal_data.shared_key_offset,
                                data->wal_data.check,
                                data->check_max,
                                data->last_id_location);
        pat->header->wal_id = data->wal_data.wal_id;
      }
    } else {
      if (pat->header->garbages[key_storage_size] != GRN_ID_NIL) {
        data->wal_data.event = GRN_WAL_EVENT_REUSE_ENTRY;
        data->wal_data.record_id = pat->header->garbages[key_storage_size];
        data->wal_data.n_garbages = pat->header->n_garbages;
        data->wal_data.n_entries = pat->header->n_entries;
        PAT_AT(pat, data->wal_data.record_id, node);
        if (!node) {
          grn_obj_set_error(ctx,
                            (grn_obj *)pat,
                            GRN_INVALID_ARGUMENT,
                            data->wal_data.record_id,
                            data->wal_data.tag,
                            "failed to get node from garbage");
          return GRN_ID_NIL;
        }
        data->wal_data.next_garbage_record_id = node->lr[0];
        if (grn_pat_wal_add_entry(ctx, &(data->wal_data)) != GRN_SUCCESS) {
          return GRN_ID_NIL;
        }
        if (grn_pat_reuse_node(ctx,
                               pat,
                               node,
                               data->wal_data.record_id,
                               data->wal_data.key,
                               data->wal_data.key_size,
                               data->wal_data.check,
                               data->check_max,
                               data->last_id_location,
                               data->wal_data.tag) != GRN_SUCCESS) {
          return GRN_ID_NIL;
        }
        pat->header->wal_id = data->wal_data.wal_id;
      } else {
        data->wal_data.event = GRN_WAL_EVENT_ADD_ENTRY;
        data->wal_data.record_id = pat->header->curr_rec + 1;
        data->wal_data.n_entries = pat->header->n_entries;
        if (grn_pat_wal_add_entry(ctx, &(data->wal_data)) != GRN_SUCCESS) {
          return GRN_ID_NIL;
        }
        node = pat_get(ctx, pat, data->wal_data.record_id);
        if (!node) {
          grn_obj_set_error(ctx,
                            (grn_obj *)pat,
                            GRN_INVALID_ARGUMENT,
                            data->wal_data.record_id,
                            data->wal_data.tag,
                            "failed to get node");
          return GRN_ID_NIL;
        }
        if (grn_pat_add_node(ctx,
                             pat,
                             node,
                             data->wal_data.record_id,
                             data->wal_data.key,
                             data->wal_data.key_size,
                             data->wal_data.check,
                             data->check_max,
                             data->last_id_location,
                             data->wal_data.tag) != GRN_SUCCESS) {
          return GRN_ID_NIL;
        }
        pat->header->wal_id = data->wal_data.wal_id;
      }
      data->shared_key_offset = node->key;
    }
  }
  data->added = true;
  if (pat->cache) {
    pat->cache[data->cache_id] = data->wal_data.record_id;
  }
  return data->wal_data.record_id;
}

grn_inline static bool
chop(grn_ctx *ctx,
     grn_pat *pat,
     const char **key,
     const char *end,
     uint32_t *offset)
{
  size_t len = grn_charlen(ctx, *key, end);
  if (len > 0) {
    *offset += len;
    *key += len;
    return (end - *key) > 0;
  } else {
    return false;
  }
}

#define MAX_FIXED_KEY_SIZE (sizeof(int64_t))

#define KEY_NEEDS_CONVERT(pat, size)                                           \
  (!((pat)->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) &&                        \
   (size_t)(size) <= MAX_FIXED_KEY_SIZE)

#define KEY_ENC(pat, keybuf, key, size)                                        \
  do {                                                                         \
    switch ((pat)->obj.header.flags & GRN_OBJ_KEY_MASK) {                      \
    case GRN_OBJ_KEY_UINT:                                                     \
      if (((pat)->obj.header.domain != GRN_DB_TOKYO_GEO_POINT) &&              \
          ((pat)->obj.header.domain != GRN_DB_WGS84_GEO_POINT)) {              \
        grn_hton((keybuf), (key), (size));                                     \
        break;                                                                 \
      }                                                                        \
    case GRN_OBJ_KEY_GEO_POINT:                                                \
      grn_gton((keybuf), (key), (size));                                       \
      break;                                                                   \
    case GRN_OBJ_KEY_INT:                                                      \
      grn_hton((keybuf), (key), (size));                                       \
      *((uint8_t *)(keybuf)) ^= 0x80;                                          \
      break;                                                                   \
    case GRN_OBJ_KEY_FLOAT:                                                    \
      if ((size) == sizeof(int64_t)) {                                         \
        int64_t v = *(int64_t *)(key);                                         \
        v ^= ((v >> 63) | (1ULL << 63));                                       \
        grn_hton((keybuf), &v, (size));                                        \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
  } while (0)

#define KEY_DEC(pat, keybuf, key, size)                                        \
  do {                                                                         \
    switch ((pat)->obj.header.flags & GRN_OBJ_KEY_MASK) {                      \
    case GRN_OBJ_KEY_UINT:                                                     \
      if (((pat)->obj.header.domain != GRN_DB_TOKYO_GEO_POINT) &&              \
          ((pat)->obj.header.domain != GRN_DB_WGS84_GEO_POINT)) {              \
        grn_ntoh((keybuf), (key), (size));                                     \
        break;                                                                 \
      }                                                                        \
    case GRN_OBJ_KEY_GEO_POINT:                                                \
      grn_ntog((keybuf), (key), (size));                                       \
      break;                                                                   \
    case GRN_OBJ_KEY_INT:                                                      \
      grn_ntohi((keybuf), (key), (size));                                      \
      break;                                                                   \
    case GRN_OBJ_KEY_FLOAT:                                                    \
      if ((size) == sizeof(int64_t)) {                                         \
        int64_t v;                                                             \
        grn_hton(&v, (key), (size));                                           \
        *((int64_t *)(keybuf)) =                                               \
          v ^ ((((int64_t)(v ^ (1ULL << 63))) >> 63) | (1ULL << 63));          \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
  } while (0)

#define KEY_ENCODE(pat, keybuf, key, size)                                     \
  do {                                                                         \
    if (KEY_NEEDS_CONVERT(pat, size)) {                                        \
      KEY_ENC((pat), (keybuf), (key), (size));                                 \
      (key) = (keybuf);                                                        \
    }                                                                          \
  } while (0)

grn_id
grn_pat_add(grn_ctx *ctx,
            grn_pat *pat,
            const void *key,
            uint32_t key_size,
            void **value,
            int *added)
{
  grn_pat_add_data data = {0};
  data.wal_data.pat = pat;
  data.wal_data.tag = "[pat][add]";
  data.wal_data.key = key;
  data.wal_data.key_size = key_size;
  data.added = false;
  data.shared_key_offset = 0;
  grn_id r0;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (!key) {
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      GRN_INVALID_ARGUMENT,
                      GRN_ID_NIL,
                      data.wal_data.tag,
                      "key must not NULL");
    return GRN_ID_NIL;
  }
  if (!key_size) {
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      GRN_INVALID_ARGUMENT,
                      GRN_ID_NIL,
                      data.wal_data.tag,
                      "key size must not zero");
    return GRN_ID_NIL;
  }
  if (key_size > GRN_TABLE_MAX_KEY_SIZE) {
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      GRN_INVALID_ARGUMENT,
                      GRN_ID_NIL,
                      data.wal_data.tag,
                      "too long key: <%u>",
                      key_size);
    return GRN_ID_NIL;
  }
  KEY_ENCODE(pat, keybuf, data.wal_data.key, data.wal_data.key_size);
  r0 = grn_pat_add_internal(ctx, &data);
  if (r0 == GRN_ID_NIL) {
    grn_obj_set_error(ctx,
                      (grn_obj *)pat,
                      GRN_INVALID_ARGUMENT,
                      GRN_ID_NIL,
                      data.wal_data.tag,
                      "failed to add a key");
    return GRN_ID_NIL;
  }
  if (added) {
    *added = data.added;
  }
  if (r0 && (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) &&
      (*(data.wal_data.key) & 0x80)) { // todo: refine!!
    sis_node *sl, *sr;
    grn_id l = r0, r;
    if (data.added && (sl = sis_get(ctx, pat, l))) {
      const char *sis = data.wal_data.key;
      const char *end = sis + data.wal_data.key_size;
      sl->children = l;
      sl->sibling = 0;
      while (chop(ctx, pat, &sis, end, &(data.shared_key_offset))) {
        if (!(*sis & 0x80)) {
          break;
        }
        data.wal_data.key = sis;
        data.wal_data.key_size = end - sis;
        if (!(r = grn_pat_add_internal(ctx, &data))) {
          break;
        }
        if (!(sr = sis_get(ctx, pat, r))) {
          break;
        }
        if (data.added) {
          sl->sibling = r;
          sr->children = l;
          sr->sibling = 0;
        } else {
          sl->sibling = sr->children;
          sr->children = l;
          break;
        }
        l = r;
        sl = sr;
      }
    }
  }
  if (r0 && value) {
    byte *v = (byte *)sis_get(ctx, pat, r0);
    if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
      *value = v + sizeof(sis_node);
    } else {
      *value = v;
    }
  }
  return r0;
}

grn_inline static grn_id
_grn_pat_get(
  grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size, void **value)
{
  grn_id r;
  pat_node *rn;
  int32_t c0 = -1, c;
  int32_t len = key_size * 16;
  PAT_AT(pat, 0, rn);
  for (r = rn->lr[1]; r;) {
    PAT_AT(pat, r, rn);
    if (!rn) {
      break; /* corrupt? */
    }
    c = PAT_CHK(rn);
    if (len <= c) {
      break;
    }
    if (c <= c0) {
      const uint8_t *k = pat_node_get_key(ctx, pat, rn);
      if (k && key_size == PAT_LEN(rn) && !memcmp(k, key, key_size)) {
        if (value) {
          byte *v = (byte *)sis_get(ctx, pat, r);
          if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
            *value = v + sizeof(sis_node);
          } else {
            *value = v;
          }
        }
        return r;
      }
      break;
    }
    r = *grn_pat_next_location(ctx, rn, key, c, len);
    c0 = c;
  }
  return GRN_ID_NIL;
}

grn_id
grn_pat_get(
  grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size, void **value)
{
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  KEY_ENCODE(pat, keybuf, key, key_size);
  return _grn_pat_get(ctx, pat, key, key_size, value);
}

grn_id
grn_pat_nextid(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size)
{
  grn_id r = GRN_ID_NIL;
  if (pat && key) {
    if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
      return GRN_ID_NIL;
    }
    uint32_t key_storage_size = pat_key_storage_size(key_size);
    if (!(r = pat->header->garbages[key_storage_size])) {
      r = pat->header->curr_rec + 1;
    }
  }
  return r;
}

static void
get_tc(grn_ctx *ctx, grn_pat *pat, grn_hash *h, pat_node *rn)
{
  grn_id id;
  pat_node *node;
  id = rn->lr[1];
  if (id) {
    PAT_AT(pat, id, node);
    if (node) {
      if (PAT_CHK(node) > PAT_CHK(rn)) {
        get_tc(ctx, pat, h, node);
      } else {
        grn_hash_add(ctx, h, &id, sizeof(grn_id), NULL, NULL);
      }
    }
  }
  id = rn->lr[0];
  if (id) {
    PAT_AT(pat, id, node);
    if (node) {
      if (PAT_CHK(node) > PAT_CHK(rn)) {
        get_tc(ctx, pat, h, node);
      } else {
        grn_hash_add(ctx, h, &id, sizeof(grn_id), NULL, NULL);
      }
    }
  }
}

grn_rc
grn_pat_prefix_search(
  grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size, grn_hash *h)
{
  int32_t c0 = -1, c;
  const uint8_t *k;
  int32_t len = key_size * 16;
  grn_id r;
  pat_node *rn;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  grn_rc rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (key_size == 0) {
    return GRN_END_OF_DATA;
  }
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, rn);
  r = rn->lr[1];
  while (r) {
    PAT_AT(pat, r, rn);
    if (!rn) {
      return GRN_FILE_CORRUPT;
    }
    c = PAT_CHK(rn);
    if (c0 < c && c < len - 1) {
      r = *grn_pat_next_location(ctx, rn, key, c, len);
      c0 = c;
      continue;
    }
    if (!(k = pat_node_get_key(ctx, pat, rn))) {
      break;
    }
    if (PAT_LEN(rn) < key_size) {
      break;
    }
    if (!memcmp(k, key, key_size)) {
      if (c >= len - 1) {
        get_tc(ctx, pat, h, rn);
      } else {
        grn_hash_add(ctx, h, &r, sizeof(grn_id), NULL, NULL);
      }
      return GRN_SUCCESS;
    }
    break;
  }
  return GRN_END_OF_DATA;
}

grn_hash *
grn_pat_prefix_search2(grn_ctx *ctx,
                       grn_pat *pat,
                       const void *key,
                       uint32_t key_size)
{
  grn_hash *h;
  if (!pat || !key) {
    return NULL;
  }
  if ((h = grn_hash_create(ctx, NULL, sizeof(grn_id), 0, 0))) {
    if (grn_pat_prefix_search(ctx, pat, key, key_size, h)) {
      grn_hash_close(ctx, h);
      h = NULL;
    }
  }
  return h;
}

grn_rc
grn_pat_suffix_search(
  grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size, grn_hash *h)
{
  grn_id r;
  if ((r = grn_pat_get(ctx, pat, key, key_size, NULL))) {
    uint32_t *offset;
    if (grn_hash_add(ctx, h, &r, sizeof(grn_id), (void **)&offset, NULL)) {
      *offset = 0;
      if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
        sis_collect(ctx, pat, h, r, 1);
      }
      return GRN_SUCCESS;
    }
  }
  return GRN_END_OF_DATA;
}

grn_hash *
grn_pat_suffix_search2(grn_ctx *ctx,
                       grn_pat *pat,
                       const void *key,
                       uint32_t key_size)
{
  grn_hash *h;
  if (!pat || !key) {
    return NULL;
  }
  if ((h = grn_hash_create(ctx, NULL, sizeof(grn_id), sizeof(uint32_t), 0))) {
    if (grn_pat_suffix_search(ctx, pat, key, key_size, h)) {
      grn_hash_close(ctx, h);
      h = NULL;
    }
  }
  return h;
}

grn_id
grn_pat_lcp_search(grn_ctx *ctx,
                   grn_pat *pat,
                   const void *key,
                   uint32_t key_size)
{
  pat_node *rn;
  grn_id r, r2 = GRN_ID_NIL;
  int32_t len = key_size * 16;
  int32_t c0 = -1, c;
  if (!pat || !key) {
    return GRN_ID_NIL;
  }
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (!(pat->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)) {
    return GRN_ID_NIL;
  }
  PAT_AT(pat, 0, rn);
  for (r = rn->lr[1]; r;) {
    PAT_AT(pat, r, rn);
    if (!rn) {
      break; /* corrupt? */
    }
    c = PAT_CHK(rn);
    if (c <= c0) {
      if (PAT_LEN(rn) <= key_size) {
        uint8_t *p = pat_node_get_key(ctx, pat, rn);
        if (!p) {
          break;
        }
        if (!memcmp(p, key, PAT_LEN(rn))) {
          return r;
        }
      }
      break;
    }
    if (len <= c) {
      break;
    }
    if (PAT_CHECK_IS_TERMINATED(c)) {
      uint8_t *p;
      pat_node *rn0;
      grn_id r0 = rn->lr[0];
      PAT_AT(pat, r0, rn0);
      if (!rn0) {
        break; /* corrupt? */
      }
      p = pat_node_get_key(ctx, pat, rn0);
      if (!p) {
        break;
      }
      if (PAT_LEN(rn0) <= key_size && !memcmp(p, key, PAT_LEN(rn0))) {
        r2 = r0;
      }
    }
    r = *grn_pat_next_location(ctx, rn, key, c, len);
    c0 = c;
  }
  return r2;
}

static grn_id
grn_pat_fuzzy_search_find_prefixed_start_node_id(grn_ctx *ctx,
                                                 grn_pat *pat,
                                                 const void *key,
                                                 uint32_t key_size)
{
  int32_t c0 = -1, c;
  const uint8_t *k;
  int32_t len = key_size * 16;
  grn_id r;
  pat_node *rn;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];

  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, rn);
  r = rn->lr[1];
  while (r) {
    PAT_AT(pat, r, rn);
    if (!rn) {
      return GRN_ID_NIL;
    }
    c = PAT_CHK(rn);
    if (c0 < c && c < len - 1) {
      r = *grn_pat_next_location(ctx, rn, key, c, len);
      c0 = c;
      continue;
    }
    if (!(k = pat_node_get_key(ctx, pat, rn))) {
      break;
    }
    if (PAT_LEN(rn) < key_size) {
      break;
    }
    if (!memcmp(k, key, key_size)) {
      return r;
    }
    break;
  }
  return GRN_ID_NIL;
}

typedef struct {
  grn_id id;
  uint16_t distance;
} fuzzy_heap_node;

typedef struct {
  uint32_t n_entries;
  uint32_t limit;
  fuzzy_heap_node *nodes;
} fuzzy_heap;

static grn_inline fuzzy_heap *
fuzzy_heap_open(grn_ctx *ctx, uint32_t max)
{
  fuzzy_heap *h = GRN_CALLOC(sizeof(fuzzy_heap));
  if (!h) {
    return NULL;
  }
  h->nodes = GRN_MALLOC(sizeof(fuzzy_heap_node) * max);
  if (!h->nodes) {
    GRN_FREE(h);
    return NULL;
  }
  h->n_entries = 0;
  h->limit = max;
  return h;
}

static grn_inline grn_bool
fuzzy_heap_push(grn_ctx *ctx, fuzzy_heap *h, grn_id id, uint16_t distance)
{
  int n, n2;
  fuzzy_heap_node node = {id, distance};
  fuzzy_heap_node node2;
  if (h->n_entries >= h->limit) {
    uint32_t max = h->limit * 2;
    fuzzy_heap_node *nodes = GRN_REALLOC(h->nodes, sizeof(fuzzy_heap) * max);
    if (!h) {
      return GRN_FALSE;
    }
    h->limit = max;
    h->nodes = nodes;
  }
  h->nodes[h->n_entries] = node;
  n = h->n_entries++;
  while (n) {
    n2 = (n - 1) >> 1;
    if (h->nodes[n2].distance <= h->nodes[n].distance) {
      break;
    }
    node2 = h->nodes[n];
    h->nodes[n] = h->nodes[n2];
    h->nodes[n2] = node2;
    n = n2;
  }
  return GRN_TRUE;
}

static grn_inline void
fuzzy_heap_close(grn_ctx *ctx, fuzzy_heap *h)
{
  GRN_FREE(h->nodes);
  GRN_FREE(h);
}

typedef struct {
  uint16_t *dists;
  uint32_t x_length;
} fuzzy_search_data;

#define DIST(data, ox, oy)                                                     \
  ((data)->dists[(((data)->x_length + 1) * (oy)) + (ox)])

grn_inline static uint16_t
grn_pat_fuzzy_search_calc_edit_distance(grn_ctx *ctx,
                                        fuzzy_search_data *data,
                                        const char *sx,
                                        const char *ex,
                                        const char *sy,
                                        const char *ey,
                                        uint32_t offset,
                                        uint32_t max_distance,
                                        bool *can_transition,
                                        uint32_t flags)
{
  uint32_t cx, cy, x, y;
  const char *px, *py;

  /*
   * Continue from grn_pat_fuzzy_search().
   *
   * Skip already calculated rows.
   *
   * If the previous y is "data" and the new y is "day" and offset is
   * 2, we can reuse the current "da" result.
   *
   * Current dists:
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | d | 1 | 0 | 1 | 2 | 3 |
   * | a | 2 | 1 | 0 | 1 | 2 |
   * | t | 3 | 2 | 1 | 0 | 1 |
   * | a | 4 | 3 | 2 | 1 | 1 |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   *
   * Reused dists:
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | d | 1 | 0 | 1 | 2 | 3 |
   * | a | 2 | 1 | 0 | 1 | 2 |
   * | y | 3 | ? | ? | ? | ? |
   * | ? | 4 | ? | ? | ? | ? |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   */
  for (py = sy, y = 1; py < ey && (cy = grn_charlen(ctx, py, ey));
       py += cy, y++) {
    if ((uint32_t)(py - sy) >= offset) {
      break;
    }
  }
  /*
   * Continue from grn_pat_fuzzy_search().
   *
   * y: data
   *
   * 1: d
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | d | 1 | 0 | 1 | 2 | 3 |
   * | ? | 2 | ? | ? | ? | ? |
   * | ? | 3 | ? | ? | ? | ? |
   * | ? | 4 | ? | ? | ? | ? |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   *
   * 2: a
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | d | 1 | 0 | 1 | 2 | 3 |
   * | a | 2 | 1 | 0 | 1 | 3 |
   * | t | 3 | ? | ? | ? | ? |
   * | a | 4 | ? | ? | ? | ? |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   *
   * 3: t
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | d | 1 | 0 | 1 | 2 | 3 |
   * | a | 2 | 1 | 0 | 1 | 3 |
   * | t | 3 | 2 | 1 | 0 | 1 |
   * | a | 4 | ? | ? | ? | ? |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   *
   * 4: a
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | d | 1 | 0 | 1 | 2 | 3 |
   * | a | 2 | 1 | 0 | 1 | 2 |
   * | t | 3 | 2 | 1 | 0 | 1 |
   * | a | 4 | 3 | 2 | 1 | 1 |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   */
  for (; py < ey && (cy = grn_charlen(ctx, py, ey)); py += cy, y++) {
    /* children nodes will be no longer smaller than max distance
     * with only insertion costs.
     * This is end of row on allocated memory. */
    if (y > data->x_length + max_distance) {
      *can_transition = false;
      return max_distance + 1;
    }

    for (px = sx, x = 1; px < ex && (cx = grn_charlen(ctx, px, ex));
         px += cx, x++) {
      if (cx == cy && !memcmp(px, py, cx)) {
        /* Use the upper left cell value as-is when the target
         * characters equal. */
        DIST(data, x, y) = DIST(data, x - 1, y - 1);
      } else {
        uint32_t a, b, c;
        a = DIST(data, x - 1, y) + 1;
        b = DIST(data, x, y - 1) + 1;
        c = DIST(data, x - 1, y - 1) + 1;
        /* Use the minimum value in a, b and c. */
        DIST(data, x, y) = ((a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c));
        if (flags & GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION && x > 1 &&
            y > 1 && cx == cy && memcmp(px, py - cy, cx) == 0 &&
            memcmp(px - cx, py, cx) == 0) {
          /* Use 1 distance for transposition. */
          uint32_t t = DIST(data, x - 2, y - 2) + 1;
          if (t < (DIST(data, x, y))) {
            DIST(data, x, y) = t;
          }
        }
      }
    }
  }
  if (data->x_length > 0) {
    /* If there is no cell which is smaller than equal to max distance on end of
     * row, children nodes will be no longer smaller than max distance */
    *can_transition = false;
    for (x = 1; x <= data->x_length; x++) {
      if (DIST(data, x, y - 1) <= max_distance) {
        *can_transition = true;
        break;
      }
    }
  }
  /* The bottom right cell is the distance of this input. */
  return DIST(data, data->x_length, y - 1);
}

typedef struct {
  const char *key;
  uint32_t key_length;
  bool can_transition;
} fuzzy_node;

/* id must not be GRN_ID_NIL. Caller must check it. */
grn_inline static void
grn_pat_fuzzy_search_recursive(grn_ctx *ctx,
                               grn_pat *pat,
                               fuzzy_search_data *data,
                               grn_id id,
                               const char *key,
                               uint32_t key_size,
                               int last_check,
                               fuzzy_node *last_node,
                               uint32_t max_distance,
                               uint32_t prefix_match_size,
                               uint32_t flags,
                               fuzzy_heap *heap)
{
  pat_node *node = NULL;
  int check;
  const char *k;

  PAT_AT(pat, id, node);
  if (!node) {
    return;
  }
  check = PAT_CHK(node);
  uint32_t len = PAT_LEN(node);
  k = pat_node_get_key(ctx, pat, node);

  /* There are sub nodes. */
  if (check > last_check) {
    if (len >= last_node->key_length &&
        !memcmp(k, last_node->key, last_node->key_length)) {
      if (!last_node->can_transition) {
        return;
      }
    }
    if (node->lr[0] != GRN_ID_NIL) {
      grn_pat_fuzzy_search_recursive(ctx,
                                     pat,
                                     data,
                                     node->lr[0],
                                     key,
                                     key_size,
                                     check,
                                     last_node,
                                     max_distance,
                                     prefix_match_size,
                                     flags,
                                     heap);
    }
    if (node->lr[1] != GRN_ID_NIL) {
      grn_pat_fuzzy_search_recursive(ctx,
                                     pat,
                                     data,
                                     node->lr[1],
                                     key,
                                     key_size,
                                     check,
                                     last_node,
                                     max_distance,
                                     prefix_match_size,
                                     flags,
                                     heap);
    }
  } else {
    if (prefix_match_size > 0) {
      if (len < prefix_match_size) {
        return;
      }
      if (memcmp(k, key, prefix_match_size) != 0) {
        return;
      }
    }
    /* This is an optimization to avoid re-calculated dists. See also
     * a comment in grn_pat_fuzzy_search_calc_edit_distance().
     *
     * Set already calculated common prefix length. */
    uint32_t offset = 0;
    if (len >= last_node->key_length &&
        !memcmp(k, last_node->key, last_node->key_length)) {
      if (!last_node->can_transition) {
        return;
      }
      offset = last_node->key_length;
    } else {
      if (!last_node->can_transition) {
        last_node->can_transition = true;
      }
      if (last_node->key_length) {
        const char *kp = k;
        const char *ke = k + len;
        const char *p = last_node->key;
        const char *e = last_node->key + last_node->key_length;
        int lp;
        for (; p < e && kp < ke && (lp = grn_charlen(ctx, p, e));
             p += lp, kp += lp) {
          if (p + lp <= e && kp + lp <= ke && memcmp(p, kp, lp)) {
            break;
          }
        }
        offset = kp - k;
      }
    }
    if (len - offset) {
      uint16_t distance;
      distance =
        grn_pat_fuzzy_search_calc_edit_distance(ctx,
                                                data,
                                                key,
                                                key + key_size,
                                                k,
                                                k + len,
                                                offset,
                                                max_distance,
                                                &(last_node->can_transition),
                                                flags);
      if (distance <= max_distance) {
        fuzzy_heap_push(ctx, heap, id, distance);
      }
    }
    last_node->key = k;
    last_node->key_length = len;
  }
  return;
}

#define HEAP_SIZE 256

grn_rc
grn_pat_fuzzy_search(grn_ctx *ctx,
                     grn_pat *pat,
                     const void *key,
                     uint32_t key_size,
                     grn_fuzzy_search_optarg *args,
                     grn_hash *h)
{
  pat_node *node;
  grn_id id;
  fuzzy_search_data data;
  uint32_t len, x, y, i;
  const char *s = key;
  const char *e = (const char *)key + key_size;
  fuzzy_node last_node;
  fuzzy_heap *heap;
  uint32_t max_distance = 1;
  uint32_t max_expansion = 0;
  uint32_t prefix_match_size = 0;
  uint32_t flags = 0;
  grn_rc rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (args) {
    max_distance = args->max_distance;
    max_expansion = args->max_expansion;
    prefix_match_size = args->prefix_match_size;
    flags = args->flags;
  }
  if (key_size > GRN_TABLE_MAX_KEY_SIZE ||
      max_distance > GRN_TABLE_MAX_KEY_SIZE || prefix_match_size > key_size) {
    return GRN_INVALID_ARGUMENT;
  }

  if (prefix_match_size > 0) {
    id = grn_pat_fuzzy_search_find_prefixed_start_node_id(ctx,
                                                          pat,
                                                          key,
                                                          prefix_match_size);
  } else {
    PAT_AT(pat, GRN_ID_NIL, node);
    id = node->lr[1];
  }
  if (id == GRN_ID_NIL) {
    return GRN_END_OF_DATA;
  }

  heap = fuzzy_heap_open(ctx, HEAP_SIZE);
  if (!heap) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  for (data.x_length = 0; s < e && (len = grn_charlen(ctx, s, e)); s += len) {
    data.x_length++;
  }
  data.dists =
    GRN_MALLOC((data.x_length + 1) * (data.x_length + max_distance + 1) *
               sizeof(uint16_t));
  if (!data.dists) {
    fuzzy_heap_close(ctx, heap);
    return GRN_NO_MEMORY_AVAILABLE;
  }

  /*
   * x: each character of the given key
   * lx: the number of x in character
   * y: each character of a key in this pat
   *
   * Example:
   *
   * x: date
   * max_distance: 2
   *
   * Initial:
   * |   |   | d | a | t | e |
   * |   | 0 | 1 | 2 | 3 | 4 |
   * | ? | 1 | ? | ? | ? | ? |
   * | ? | 2 | ? | ? | ? | ? |
   * | ? | 3 | ? | ? | ? | ? |
   * | ? | 4 | ? | ? | ? | ? |
   * | ? | 5 | ? | ? | ? | ? |
   * | ? | 6 | ? | ? | ? | ? |
   * | ? | 7 | ? | ? | ? | ? |
   */
  for (x = 0; x <= data.x_length; x++) {
    DIST(&data, x, 0) = x;
  }
  for (y = 0; y <= data.x_length + max_distance; y++) {
    DIST(&data, 0, y) = y;
  }

  last_node.key = NULL;
  last_node.key_length = 0;
  last_node.can_transition = true;
  /*
   * Fill dists and find matched IDs recursively. See
   * grn_pat_fuzzy_search_calc_edit_distance() how to fill dists.
   */
  grn_pat_fuzzy_search_recursive(ctx,
                                 pat,
                                 &data,
                                 id,
                                 key,
                                 key_size,
                                 -1,
                                 &last_node,
                                 max_distance,
                                 prefix_match_size,
                                 flags,
                                 heap);
  GRN_FREE(data.dists);
  for (i = 0; i < heap->n_entries; i++) {
    if (max_expansion > 0 && i >= max_expansion) {
      break;
    }
    if (DB_OBJ(h)->header.flags & GRN_OBJ_WITH_SUBREC) {
      grn_rset_recinfo *ri;
      if (grn_hash_add(ctx,
                       h,
                       &(heap->nodes[i].id),
                       sizeof(grn_id),
                       (void **)&ri,
                       NULL)) {
        ri->score = max_distance - heap->nodes[i].distance + 1;
      }
    } else {
      grn_hash_add(ctx, h, &(heap->nodes[i].id), sizeof(grn_id), NULL, NULL);
    }
  }
  fuzzy_heap_close(ctx, heap);
  if (grn_hash_size(ctx, h)) {
    return GRN_SUCCESS;
  } else {
    return GRN_END_OF_DATA;
  }
}

typedef struct {
  grn_pat_delinfo *di;
  grn_id id;
  uint16_t check;
  uint16_t check0;
  pat_node *rn;
  pat_node *rn0;
  pat_node *rno;
  grn_id otherside;
  grn_id *proot;
  grn_id *p;
  grn_id *p0;
  uint32_t n_garbages;
  uint32_t n_entries;
} grn_pat_del_data;

grn_inline static grn_rc
grn_pat_del_internal(grn_ctx *ctx, grn_pat *pat, grn_pat_del_data *data)
{
  grn_pat_delinfo *di = data->di;
  grn_id id = data->id;
  uint16_t check = data->check;
  uint16_t check0 = data->check0;
  pat_node *rn = data->rn;
  pat_node *rn0 = data->rn0;
  pat_node *rno = data->rno;
  grn_id otherside = data->otherside;
  grn_id *proot = data->proot;
  grn_id *p = data->p;
  grn_id *p0 = data->p0;
  if (rn == rn0) {
    /* The last transition (p) is a self-loop. */
    di->stat = DL_PHASE2;
    di->d = id;
    if (otherside) {
      if (check0 < PAT_CHK(rno) && PAT_CHK(rno) <= check) {
        /* To keep rno as an output node, its check is set to zero. */
        if (!delinfo_search(pat, otherside, NULL)) {
          GRN_LOG(ctx, GRN_LOG_DEBUG, "no delinfo found %d", otherside);
        }
        PAT_CHK_SET(rno, 0);
      }
      if (proot == p0 && !rno->check) {
        /*
         * Update rno->lr because the first node, rno becomes the new first
         * node, is not an output node even if its check is zero.
         */
        const uint8_t *k = pat_node_get_key(ctx, pat, rno);
        int direction = k ? (*k >> 7) : 1;
        rno->lr[direction] = otherside;
        rno->lr[!direction] = 0;
      }
    }
    *p0 = otherside;
  } else if ((rn->lr[0] == GRN_ID_NIL && rn->lr[1] == id) ||
             (rn->lr[1] == GRN_ID_NIL && rn->lr[0] == id)) {
    /* The output node has only a disabled self-loop. */
    di->stat = DL_PHASE2;
    di->d = id;
    *p = GRN_ID_NIL;
  } else {
    /* The last transition (p) is not a self-loop. */
    grn_pat_delinfo *ldi = NULL, *ddi = NULL;
    if (PAT_DEL(rn)) {
      ldi = delinfo_search(pat, id, NULL);
    }
    if (PAT_DEL(rn0)) {
      ddi = delinfo_search(pat, *p0, NULL);
    }
    if (ldi) {
      PAT_DEL_OFF(rn);
      di->stat = DL_PHASE2;
      if (ddi) {
        PAT_DEL_OFF(rn0);
        ddi->stat = DL_PHASE2;
        if (ddi == ldi) {
          if (id != ddi->ld) {
            GRN_LOG(ctx, GRN_LOG_ERROR, "id(%u) != ddi->ld(%u)", id, ddi->ld);
          }
          di->d = id;
        } else {
          ldi->ld = ddi->ld;
          di->d = id;
        }
      } else {
        PAT_DEL_ON(rn0);
        ldi->ld = *p0;
        di->d = id;
      }
    } else {
      PAT_DEL_ON(rn);
      if (ddi) {
        if (ddi->d != *p0) {
          GRN_LOG(ctx, GRN_LOG_ERROR, "ddi->d(%d) != *p0(%d)", ddi->d, *p0);
        }
        PAT_DEL_OFF(rn0);
        ddi->stat = DL_PHASE2;
        di->stat = DL_PHASE1;
        di->ld = ddi->ld;
        di->d = id;
        /*
        PAT_DEL_OFF(rn0);
        ddi->d = r;
        di->stat = DL_PHASE2;
        di->d = *p0;
        */
      } else {
        PAT_DEL_ON(rn0);
        di->stat = DL_PHASE1;
        di->ld = *p0;
        di->d = id;
        // grn_log("pat_del d=%d ld=%d stat=%d", r, *p0, DL_PHASE1);
      }
    }
    if (*p0 == otherside) {
      /* The previous node (*p0) has a self-loop (rn0 == rno). */
      PAT_CHK_SET(rno, 0);
      if (proot == p0) {
        /*
         * Update rno->lr because the first node, rno becomes the new first
         * node, is not an output node even if its check is zero.
         */
        const uint8_t *k = pat_node_get_key(ctx, pat, rno);
        int direction = k ? (*k >> 7) : 1;
        rno->lr[direction] = otherside;
        rno->lr[!direction] = 0;
      }
    } else {
      if (otherside) {
        if (check0 < PAT_CHK(rno) && PAT_CHK(rno) <= check) {
          /* To keep rno as an output node, its check is set to zero. */
          if (!delinfo_search(pat, otherside, NULL)) {
            GRN_LOG(ctx, GRN_LOG_ERROR, "no delinfo found %d", otherside);
          }
          PAT_CHK_SET(rno, 0);
        }
        if (proot == p0 && !rno->check) {
          /*
           * Update rno->lr because the first node, rno becomes the new first
           * node, is not an output node even if its check is zero.
           */
          const uint8_t *k = pat_node_get_key(ctx, pat, rno);
          int direction = k ? (*k >> 7) : 1;
          rno->lr[direction] = otherside;
          rno->lr[!direction] = 0;
        }
      }
      *p0 = otherside;
    }
  }
  pat->header->n_garbages = data->n_garbages;
  pat->header->n_entries = data->n_entries;
  return GRN_SUCCESS;
}

grn_inline static grn_rc
_grn_pat_del(grn_ctx *ctx,
             grn_pat *pat,
             const char *key,
             uint32_t key_size,
             int shared,
             grn_table_delete_optarg *optarg)
{
  pat_node *rn, *rn0 = NULL, *rno = NULL;
  int32_t c = -1, c0 = -1, ch;
  int32_t len = key_size * 16;
  grn_id otherside, *proot, *p, *p0 = NULL;

  grn_pat_wal_add_entry_data wal_data = {0};
  wal_data.pat = pat;
  wal_data.tag = "[pat][delete]";
  wal_data.key = key;
  wal_data.key_size = key_size;
  wal_data.is_shared = shared;

  /* delinfo_new() must be called before searching for rn. */
  grn_pat_delinfo *di = delinfo_new(ctx, pat, &wal_data);
  if (!di) {
    return ctx->rc;
  }

  /*
   * Search a patricia tree for a given key.
   * If the key exists, get its output node.
   *
   * rn, rn0: the output node and its previous node.
   * rno: the other side of rn (the other destination of rn0).
   * c, c0: checks of rn0 and its previous node.
   * p, p0: pointers to transitions (IDs) that refer to rn and rn0.
   * id, id0: ID of p and p0.
   */
  grn_id id = GRN_ID_NIL;
  grn_id id0 = GRN_ID_NIL;
  PAT_AT(pat, id, rn);
  proot = p = &rn->lr[1];
  wal_data.record_direction = DIRECTION_RIGHT;
  grn_pat_wal_add_entry_data_set_record_direction(ctx, &wal_data, id, rn, p);
  for (;;) {
    grn_id next_id = *p;
    if (next_id == GRN_ID_NIL) {
      return GRN_INVALID_ARGUMENT;
    }
    id0 = id;
    id = next_id;
    PAT_AT(pat, id, rn);
    if (!rn) {
      return GRN_FILE_CORRUPT;
    }
    ch = PAT_CHK(rn);
    if (len <= ch) {
      return GRN_INVALID_ARGUMENT;
    }
    if (c >= ch) {
      /* Output node found. */
      const uint8_t *k = pat_node_get_key(ctx, pat, rn);
      if (!k) {
        return GRN_INVALID_ARGUMENT;
      }
      if (key_size != PAT_LEN(rn) || memcmp(k, key, key_size)) {
        return GRN_INVALID_ARGUMENT;
      }
      /* Given key found. */
      break;
    }
    c0 = c;
    p0 = p;
    c = ch;
    p = grn_pat_next_location(ctx, rn, key, c, len);
    grn_pat_wal_add_entry_data_set_record_direction(ctx, &wal_data, id, rn, p);
    rn0 = rn;
  }
  if (optarg && optarg->func &&
      !optarg->func(ctx, (grn_obj *)pat, id, optarg->func_arg)) {
    return GRN_SUCCESS;
  }
  if (rn0->lr[0] == rn0->lr[1]) {
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "*p0 (%d), rn0->lr[0] == rn0->lr[1] (%d)",
            *p0,
            rn0->lr[0]);
    return GRN_FILE_CORRUPT;
  }
  otherside = (rn0->lr[1] == id) ? rn0->lr[0] : rn0->lr[1];
  if (otherside) {
    PAT_AT(pat, otherside, rno);
    if (!rno) {
      return GRN_FILE_CORRUPT;
    }
  }

  if (pat->cache) {
    uint32_t cache_id;
    cache_id = grn_pat_cache_compute_id(ctx, pat, key, key_size);
    if (pat->cache[cache_id]) {
      pat->cache[cache_id] = GRN_ID_NIL;
    }
  }

  wal_data.event = GRN_WAL_EVENT_DELETE_ENTRY;
  wal_data.record_id = id;
  wal_data.check = c;
  wal_data.parent_check = c0;
  wal_data.otherside_record_id = otherside;
  if (rno) {
    wal_data.otherside_check = PAT_CHK(rno);
  }
  wal_data.left_record_id = rn->lr[0];
  wal_data.right_record_id = rn->lr[1];
  wal_data.n_garbages = pat->header->n_garbages + 1;
  wal_data.n_entries = pat->header->n_entries - 1;
  wal_data.delete_info_phase1_index = pat->header->curr_del;
  wal_data.delete_info_phase2_index = pat->header->curr_del2;
  if (grn_pat_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return ctx->rc;
  }

  grn_pat_del_data data = {0};
  data.di = di;
  data.id = id;
  data.check = c;
  data.check0 = c0;
  data.rn = rn;
  data.rn0 = rn0;
  data.rno = rno;
  data.otherside = otherside;
  data.proot = proot;
  data.p = p;
  data.p0 = p0;
  data.n_garbages = wal_data.n_garbages;
  data.n_entries = wal_data.n_entries;
  grn_rc rc = grn_pat_del_internal(ctx, pat, &data);
  if (rc == GRN_SUCCESS) {
    pat->header->wal_id = wal_data.wal_id;
  }
  return rc;
}

static grn_rc
_grn_pat_delete(grn_ctx *ctx,
                grn_pat *pat,
                const void *key,
                uint32_t key_size,
                grn_table_delete_optarg *optarg)
{
  if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
    grn_id id = grn_pat_get(ctx, pat, key, key_size, NULL);
    if (id && grn_pat_delete_with_sis(ctx, pat, id, optarg)) {
      return GRN_SUCCESS;
    }
    return GRN_INVALID_ARGUMENT;
  }
  return _grn_pat_del(ctx, pat, key, key_size, 0, optarg);
}

grn_rc
grn_pat_delete(grn_ctx *ctx,
               grn_pat *pat,
               const void *key,
               uint32_t key_size,
               grn_table_delete_optarg *optarg)
{
  grn_rc rc;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  if (!pat || !key || !key_size) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  KEY_ENCODE(pat, keybuf, key, key_size);
  return _grn_pat_delete(ctx, pat, key, key_size, optarg);
}

uint32_t
grn_pat_size(grn_ctx *ctx, grn_pat *pat)
{
  if (!pat) {
    return GRN_INVALID_ARGUMENT;
  }
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return 0;
  }
  return pat->header->n_entries;
}

const char *
_grn_pat_key(grn_ctx *ctx, grn_pat *pat, grn_id id, uint32_t *key_size)
{
  pat_node *node;
  uint8_t *key;
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    *key_size = 0;
    return NULL;
  }
  PAT_AT(pat, id, node);
  if (!node) {
    *key_size = 0;
    return NULL;
  }
  key = pat_node_get_key(ctx, pat, node);
  if (key) {
    *key_size = PAT_LEN(node);
  } else {
    *key_size = 0;
  }
  return (const char *)key;
}

grn_rc
grn_pat_delete_by_id(grn_ctx *ctx,
                     grn_pat *pat,
                     grn_id id,
                     grn_table_delete_optarg *optarg)
{
  grn_rc rc;
  if (!pat || !id) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  {
    uint32_t key_size;
    const char *key = _grn_pat_key(ctx, pat, id, &key_size);
    return _grn_pat_delete(ctx, pat, key, key_size, optarg);
  }
}

int
grn_pat_get_key(
  grn_ctx *ctx, grn_pat *pat, grn_id id, void *keybuf, int bufsize)
{
  int len;
  uint8_t *key;
  pat_node *node;
  if (!pat) {
    return 0;
  }
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return 0;
  }
  if (!id) {
    return 0;
  }
  PAT_AT(pat, id, node);
  if (!node) {
    return 0;
  }
  if (!(key = pat_node_get_key(ctx, pat, node))) {
    return 0;
  }
  len = PAT_LEN(node);
  if (keybuf && bufsize >= len) {
    if (KEY_NEEDS_CONVERT(pat, len)) {
      KEY_DEC(pat, keybuf, key, len);
    } else {
      grn_memcpy(keybuf, key, len);
    }
  }
  return len;
}

int
grn_pat_get_key2(grn_ctx *ctx, grn_pat *pat, grn_id id, grn_obj *bulk)
{
  uint32_t len;
  uint8_t *key;
  pat_node *node;
  if (!pat) {
    return GRN_INVALID_ARGUMENT;
  }
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return 0;
  }
  if (!id) {
    return 0;
  }
  PAT_AT(pat, id, node);
  if (!node) {
    return 0;
  }
  if (!(key = pat_node_get_key(ctx, pat, node))) {
    return 0;
  }
  len = PAT_LEN(node);
  if (KEY_NEEDS_CONVERT(pat, len)) {
    if (bulk->header.impl_flags & GRN_OBJ_REFER) {
      GRN_TEXT_INIT(bulk, 0);
    }
    if (!grn_bulk_reserve(ctx, bulk, len)) {
      char *curr = GRN_BULK_CURR(bulk);
      KEY_DEC(pat, curr, key, len);
      GRN_BULK_INCR_LEN(bulk, len);
    }
  } else {
    if (bulk->header.impl_flags & GRN_OBJ_REFER) {
      bulk->u.b.head = (char *)key;
      bulk->u.b.curr = (char *)key + len;
    } else {
      grn_bulk_write(ctx, bulk, (char *)key, len);
    }
  }
  return len;
}

int
grn_pat_get_value(grn_ctx *ctx, grn_pat *pat, grn_id id, void *valuebuf)
{
  int value_size;
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return 0;
  }
  value_size = (int)pat->value_size;
  if (value_size) {
    byte *v = (byte *)sis_at(ctx, pat, id);
    if (v) {
      if (valuebuf) {
        if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
          grn_memcpy(valuebuf, v + sizeof(sis_node), value_size);
        } else {
          grn_memcpy(valuebuf, v, value_size);
        }
      }
      return value_size;
    }
  }
  return 0;
}

const char *
grn_pat_get_value_(grn_ctx *ctx, grn_pat *pat, grn_id id, uint32_t *size)
{
  const char *value = NULL;
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return NULL;
  }
  if ((*size = pat->value_size)) {
    if ((value = (const char *)sis_at(ctx, pat, id)) &&
        (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS)) {
      value += sizeof(sis_node);
    }
  }
  return value;
}

grn_rc
grn_pat_set_value(
  grn_ctx *ctx, grn_pat *pat, grn_id id, const void *value, int flags)
{
  grn_rc rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (value) {
    uint32_t value_size = pat->value_size;
    if (value_size) {
      byte *v = (byte *)sis_get(ctx, pat, id);
      if (v) {
        if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
          v += sizeof(sis_node);
        }
        switch ((flags & GRN_OBJ_SET_MASK)) {
        case GRN_OBJ_SET:
          grn_memcpy(v, value, value_size);
          return GRN_SUCCESS;
        case GRN_OBJ_INCR:
          switch (value_size) {
          case sizeof(int32_t):
            *((int32_t *)v) += *((int32_t *)value);
            return GRN_SUCCESS;
          case sizeof(int64_t):
            *((int64_t *)v) += *((int64_t *)value);
            return GRN_SUCCESS;
          default:
            return GRN_INVALID_ARGUMENT;
          }
          break;
        case GRN_OBJ_DECR:
          switch (value_size) {
          case sizeof(int32_t):
            *((int32_t *)v) -= *((int32_t *)value);
            return GRN_SUCCESS;
          case sizeof(int64_t):
            *((int64_t *)v) -= *((int64_t *)value);
            return GRN_SUCCESS;
          default:
            return GRN_INVALID_ARGUMENT;
          }
          break;
        default:
          // todo : support other types.
          return GRN_INVALID_ARGUMENT;
        }
      } else {
        return GRN_NO_MEMORY_AVAILABLE;
      }
    }
  }
  return GRN_INVALID_ARGUMENT;
}

grn_rc
grn_pat_info(grn_ctx *ctx,
             grn_pat *pat,
             int *key_size,
             unsigned int *flags,
             grn_encoding *encoding,
             unsigned int *n_entries,
             unsigned int *file_size)
{
  grn_rc rc;
  ERRCLR(NULL);
  if (!pat) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_pat_error_if_truncated(ctx, pat);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (key_size) {
    *key_size = pat->key_size;
  }
  if (flags) {
    *flags = pat->obj.header.flags;
  }
  if (encoding) {
    *encoding = pat->encoding;
  }
  if (n_entries) {
    *n_entries = pat->header->n_entries;
  }
  if (file_size) {
    uint64_t tmp = 0;
    if ((rc = grn_io_size(ctx, pat->io, &tmp))) {
      return rc;
    }
    *file_size = (unsigned int)tmp; /* FIXME: inappropriate cast */
  }
  return GRN_SUCCESS;
}

int
grn_pat_delete_with_sis(grn_ctx *ctx,
                        grn_pat *pat,
                        grn_id id,
                        grn_table_delete_optarg *optarg)
{
  int level = 0, shared;
  const char *key = NULL, *_key;
  sis_node *sp, *ss = NULL, *si;
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return 0;
  }
  si = sis_at(ctx, pat, id);
  while (id) {
    pat_node *rn;
    uint32_t key_size;
    if ((si && si->children && si->children != id) ||
        (optarg && optarg->func &&
         !optarg->func(ctx, (grn_obj *)pat, id, optarg->func_arg))) {
      break;
    }
    PAT_AT(pat, id, rn);
    if (!(_key = (char *)pat_node_get_key(ctx, pat, rn))) {
      return 0;
    }
    if (_key == key) {
      shared = 1;
    } else {
      key = _key;
      shared = 0;
    }
    key_size = PAT_LEN(rn);
    if (key && key_size) {
      _grn_pat_del(ctx, pat, key, key_size, shared, NULL);
    }
    if (si) {
      grn_id *p, sid;
      uint32_t offset = 0;
      if ((*key & 0x80) && chop(ctx, pat, &key, key + key_size, &offset)) {
        if ((sid = grn_pat_get(ctx, pat, key, key_size - offset, NULL)) &&
            (ss = sis_at(ctx, pat, sid))) {
          for (p = &ss->children; *p && *p != sid; p = &sp->sibling) {
            if (*p == id) {
              *p = si->sibling;
              break;
            }
            if (!(sp = sis_at(ctx, pat, *p))) {
              break;
            }
          }
        }
      } else {
        sid = GRN_ID_NIL;
      }
      si->sibling = 0;
      si->children = 0;
      id = sid;
      si = ss;
    } else {
      id = GRN_ID_NIL;
    }
    level++;
  }
  if (level) {
    uint32_t shared_key_offset = 0;
    while (id && key) {
      uint32_t key_size;
      if (_grn_pat_key(ctx, pat, id, &key_size) != key) {
        break;
      }
      {
        pat_node *rn;
        PAT_AT(pat, id, rn);
        if (!rn) {
          break;
        }
        if (shared_key_offset > 0) {
          rn->key = shared_key_offset;
        } else {
          pat_node_set_key(ctx, pat, rn, (uint8_t *)key, key_size);
          shared_key_offset = rn->key;
        }
      }
      {
        const char *end = key + key_size;
        if (!((*key & 0x80) && chop(ctx, pat, &key, end, &shared_key_offset))) {
          break;
        }
        id = grn_pat_get(ctx, pat, key, end - key, NULL);
      }
    }
  }
  return level;
}

grn_id
grn_pat_next(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  while (++id <= pat->header->curr_rec) {
    uint32_t key_size;
    const char *key = _grn_pat_key(ctx, pat, id, &key_size);
    if (id == grn_pat_get(ctx, pat, key, key_size, NULL)) {
      return id;
    }
  }
  return GRN_ID_NIL;
}

grn_id
grn_pat_at(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  uint32_t key_size;
  const char *key = _grn_pat_key(ctx, pat, id, &key_size);
  if (key && (id == _grn_pat_get(ctx, pat, key, key_size, NULL))) {
    return id;
  }
  return GRN_ID_NIL;
}

grn_id
grn_pat_curr_id(grn_ctx *ctx, grn_pat *pat)
{
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  return pat->header->curr_rec;
}

int
grn_pat_scan(grn_ctx *ctx,
             grn_pat *pat,
             const char *str,
             unsigned int str_len,
             grn_pat_scan_hit *sh,
             unsigned int sh_size,
             const char **rest)
{
  int n = 0;
  grn_id tid;
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return 0;
  }
  if (GRN_BULK_VSIZE(&(pat->normalizers)) > 0) {
    int flags =
      GRN_STRING_REMOVE_BLANK | GRN_STRING_WITH_TYPES | GRN_STRING_WITH_CHECKS;
    grn_obj *nstr = grn_string_open(ctx, str, str_len, (grn_obj *)pat, flags);
    if (nstr) {
      const short *cp = grn_string_get_checks(ctx, nstr);
      const unsigned char *tp = grn_string_get_types(ctx, nstr);
      unsigned int offset = 0, offset0 = 0;
      unsigned int normalized_length_in_bytes;
      const char *sp, *se;
      grn_string_get_normalized(ctx,
                                nstr,
                                &sp,
                                &normalized_length_in_bytes,
                                NULL);
      se = sp + normalized_length_in_bytes;
      while ((unsigned int)n < sh_size) {
        if ((tid = grn_pat_lcp_search(ctx, pat, sp, se - sp))) {
          const char *key;
          uint32_t len;
          int first_key_char_len;
          key = _grn_pat_key(ctx, pat, tid, &len);
          sh[n].id = tid;
          sh[n].offset = (*cp > 0) ? offset : offset0;
          first_key_char_len = grn_charlen(ctx, key, key + len);
          if (sh[n].offset > 0 && GRN_CHAR_IS_BLANK(tp[-1]) &&
              ((first_key_char_len == 1 && key[0] != ' ') ||
               first_key_char_len > 1)) {
            /* Remove leading spaces. */
            const char *original_str = str + sh[n].offset;
            while (true) {
              int char_len = grn_charlen(ctx, original_str, str + str_len);
              if (char_len == 0) {
                break;
              }
              if (!grn_isspace(original_str, ctx->encoding)) {
                break;
              }
              original_str += char_len;
              sh[n].offset += char_len;
            }
          }
          {
            grn_bool blank_in_alnum = GRN_FALSE;
            const unsigned char *start_tp = tp;
            const unsigned char *blank_in_alnum_check_tp;
            while (len--) {
              if (*cp > 0) {
                offset0 = offset;
                offset += *cp;
                tp++;
              }
              sp++;
              cp++;
            }
            sh[n].length = offset - sh[n].offset;
            for (blank_in_alnum_check_tp = start_tp + 1;
                 blank_in_alnum_check_tp < tp;
                 blank_in_alnum_check_tp++) {
#define GRN_CHAR_IS_ALNUM(char_type)                                           \
  (GRN_CHAR_TYPE(char_type) == GRN_CHAR_ALPHA ||                               \
   GRN_CHAR_TYPE(char_type) == GRN_CHAR_DIGIT)
              if (GRN_CHAR_IS_BLANK(blank_in_alnum_check_tp[0]) &&
                  GRN_CHAR_IS_ALNUM(blank_in_alnum_check_tp[-1]) &&
                  (blank_in_alnum_check_tp + 1) < tp &&
                  GRN_CHAR_IS_ALNUM(blank_in_alnum_check_tp[1])) {
                blank_in_alnum = GRN_TRUE;
              }
#undef GRN_CHAR_IS_ALNUM
            }
            if (!blank_in_alnum) {
              n++;
            }
          }
        } else {
          if (*cp > 0) {
            offset0 = offset;
            offset += *cp;
            tp++;
          }
          do {
            sp++;
            cp++;
          } while (sp < se && !*cp);
        }
        if (se <= sp) {
          offset = str_len;
          break;
        }
      }
      if (rest) {
        grn_string_get_original(ctx, nstr, rest, NULL);
        *rest += offset;
      }
      grn_obj_close(ctx, nstr);
    } else {
      n = -1;
      if (rest) {
        *rest = str;
      }
    }
  } else {
    uint32_t len;
    const char *sp, *se = str + str_len;
    for (sp = str; sp < se && (unsigned int)n < sh_size; sp += len) {
      if ((tid = grn_pat_lcp_search(ctx, pat, sp, se - sp))) {
        _grn_pat_key(ctx, pat, tid, &len);
        sh[n].id = tid;
        sh[n].offset = sp - str;
        sh[n].length = len;
        n++;
      } else {
        len = grn_charlen(ctx, sp, se);
      }
      if (!len) {
        break;
      }
    }
    if (rest) {
      *rest = sp;
    }
  }
  return n;
}

#define INITIAL_SIZE 512

grn_inline static void
push(grn_pat_cursor *c, grn_id id, uint16_t check)
{
  grn_ctx *ctx = c->ctx;
  grn_pat_cursor_entry *se;
  if (c->size <= c->sp) {
    if (c->ss) {
      uint32_t size = c->size * 4;
      grn_pat_cursor_entry *ss =
        GRN_REALLOC(c->ss, sizeof(grn_pat_cursor_entry) * size);
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "[pat][cursor][push][realloc] "
              "%p: %p -> %p: <%" GRN_FMT_INT32U ">",
              c,
              c->ss,
              ss,
              size);
      if (!ss) {
        return; /* give up */
      }
      c->ss = ss;
      c->size = size;
    } else {
      uint32_t size = INITIAL_SIZE;
      if (!(c->ss = GRN_MALLOC(sizeof(grn_pat_cursor_entry) * size))) {
        return; /* give up */
      }
      c->size = size;
    }
  }
  se = &c->ss[c->sp++];
  se->id = id;
  se->check = check;
}

grn_inline static grn_pat_cursor_entry *
pop(grn_pat_cursor *c)
{
  return c->sp ? &c->ss[--c->sp] : NULL;
}

static grn_id
grn_pat_cursor_next_by_id(grn_ctx *ctx, grn_pat_cursor *c)
{
  grn_pat *pat = c->pat;
  int dir = (c->obj.header.flags & GRN_CURSOR_DESCENDING) ? -1 : 1;
  while (c->curr_rec != c->tail) {
    c->curr_rec += dir;
    if (pat->header->n_garbages) {
      uint32_t key_size;
      const void *key = _grn_pat_key(ctx, pat, c->curr_rec, &key_size);
      if (_grn_pat_get(ctx, pat, key, key_size, NULL) != c->curr_rec) {
        continue;
      }
    }
    c->rest--;
    return c->curr_rec;
  }
  return GRN_ID_NIL;
}

grn_id
grn_pat_cursor_next(grn_ctx *ctx, grn_pat_cursor *c)
{
  pat_node *node;
  grn_pat_cursor_entry *se;
  if (!c->rest) {
    return GRN_ID_NIL;
  }
  if ((c->obj.header.flags & GRN_CURSOR_BY_ID)) {
    return grn_pat_cursor_next_by_id(ctx, c);
  }
  while ((se = pop(c))) {
    grn_id id = se->id;
    int check = se->check, ch;
    while (id) {
      PAT_AT(c->pat, id, node);
      if (!node) {
        break;
      }
      ch = PAT_CHK(node);
      if (ch > check) {
        if (c->obj.header.flags & GRN_CURSOR_DESCENDING) {
          push(c, node->lr[0], ch);
          id = node->lr[1];
        } else {
          push(c, node->lr[1], ch);
          id = node->lr[0];
        }
        check = ch;
        continue;
      } else {
        if (id == c->tail) {
          c->sp = 0;
        } else {
          if (!c->curr_rec && c->tail) {
            uint32_t lmin, lmax;
            pat_node *nmin, *nmax;
            const uint8_t *kmin, *kmax;
            if (c->obj.header.flags & GRN_CURSOR_DESCENDING) {
              PAT_AT(c->pat, c->tail, nmin);
              PAT_AT(c->pat, id, nmax);
            } else {
              PAT_AT(c->pat, id, nmin);
              PAT_AT(c->pat, c->tail, nmax);
            }
            lmin = PAT_LEN(nmin);
            lmax = PAT_LEN(nmax);
            kmin = pat_node_get_key(ctx, c->pat, nmin);
            kmax = pat_node_get_key(ctx, c->pat, nmax);
            if ((lmin < lmax) ? (memcmp(kmin, kmax, lmin) > 0)
                              : (memcmp(kmin, kmax, lmax) >= 0)) {
              c->sp = 0;
              break;
            }
          }
        }
        c->curr_rec = id;
        c->rest--;
        return id;
      }
    }
  }
  return GRN_ID_NIL;
}

void
grn_pat_cursor_close(grn_ctx *ctx, grn_pat_cursor *c)
{
  GRN_ASSERT(c->ctx == ctx);
  if (c->ss) {
    GRN_FREE(c->ss);
  }
  GRN_FREE(c);
}

grn_inline static int
bitcmp(const void *s1, const void *s2, int offset, int length)
{
  int r, rest = length + (offset & 7) - 8, bl = offset >> 3,
         mask = 0xff >> (offset & 7);
  unsigned char *a = (unsigned char *)s1 + bl, *b = (unsigned char *)s2 + bl;
  if (rest <= 0) {
    mask &= 0xff << -rest;
    return (*a & mask) - (*b & mask);
  }
  if ((r = (*a & mask) - (*b & mask))) {
    return r;
  }
  a++;
  b++;
  if ((bl = rest >> 3)) {
    if ((r = memcmp(a, b, bl))) {
      return r;
    }
    a += bl;
    b += bl;
  }
  mask = 0xff << (8 - (rest & 7));
  return (*a & mask) - (*b & mask);
}

grn_inline static grn_rc
set_cursor_prefix(grn_ctx *ctx,
                  grn_pat *pat,
                  grn_pat_cursor *c,
                  const void *key,
                  uint32_t key_size,
                  int flags)
{
  int32_t c0 = -1, ch;
  const uint8_t *k;
  int32_t len;
  uint32_t byte_len;
  grn_id id;
  pat_node *node;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  if (flags & GRN_CURSOR_SIZE_BY_BIT) {
    len = key_size * 2;
    byte_len = key_size >> 3;
  } else {
    len = key_size * 16;
    byte_len = key_size;
  }
  KEY_ENCODE(pat, keybuf, key, byte_len);
  PAT_AT(pat, 0, node);
  id = node->lr[1];
  while (id) {
    PAT_AT(pat, id, node);
    if (!node) {
      return GRN_FILE_CORRUPT;
    }
    ch = PAT_CHK(node);
    if (c0 < ch && ch < len - 1) {
      id = *grn_pat_next_location(ctx, node, key, ch, len);
      c0 = ch;
      continue;
    }
    if (!(k = pat_node_get_key(ctx, pat, node))) {
      break;
    }
    if (PAT_LEN(node) < byte_len) {
      break;
    }
    if ((flags & GRN_CURSOR_SIZE_BY_BIT) ? !bitcmp(k, key, 0, key_size)
                                         : !memcmp(k, key, key_size)) {
      if (c0 < ch) {
        if (flags & GRN_CURSOR_DESCENDING) {
          if ((ch > len - 1) || !(flags & GRN_CURSOR_GT)) {
            push(c, node->lr[0], ch);
          }
          push(c, node->lr[1], ch);
        } else {
          push(c, node->lr[1], ch);
          if ((ch > len - 1) || !(flags & GRN_CURSOR_GT)) {
            push(c, node->lr[0], ch);
          }
        }
      } else {
        if (PAT_LEN(node) * 16 > (uint32_t)len || !(flags & GRN_CURSOR_GT)) {
          push(c, id, ch);
        }
      }
    }
    break;
  }
  return GRN_SUCCESS;
}

grn_inline static grn_rc
set_cursor_near(grn_ctx *ctx,
                grn_pat *pat,
                grn_pat_cursor *c,
                uint32_t min_size,
                const void *key,
                int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int32_t r, check = -1, ch;
  int32_t min = min_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, pat->key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) {
      return GRN_FILE_CORRUPT;
    }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (check >= min) {
        push(c, id, check);
      }
      break;
    }
    if ((check += 2) < ch) {
      if (!(k = pat_node_get_key(ctx, pat, node))) {
        return GRN_FILE_CORRUPT;
      }
      if ((r = bitcmp(key, k, check >> 1, (ch - check) >> 1))) {
        if (ch >= min) {
          push(c, node->lr[1], ch);
          push(c, node->lr[0], ch);
        }
        break;
      }
    }
    check = ch;
    if (nth_bit((uint8_t *)key, check)) {
      if (check >= min) {
        push(c, node->lr[0], check);
      }
      id = node->lr[1];
    } else {
      if (check >= min) {
        push(c, node->lr[1], check);
      }
      id = node->lr[0];
    }
  }
  return GRN_SUCCESS;
}

grn_inline static grn_rc
set_cursor_common_prefix(grn_ctx *ctx,
                         grn_pat *pat,
                         grn_pat_cursor *c,
                         uint32_t min_size,
                         const void *key,
                         uint32_t key_size,
                         int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int32_t check = -1, ch;
  int32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) {
      return GRN_FILE_CORRUPT;
    }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (!(k = pat_node_get_key(ctx, pat, node))) {
        return GRN_FILE_CORRUPT;
      }
      {
        uint32_t l = PAT_LEN(node);
        if (min_size <= l && l <= key_size) {
          if (!memcmp(key, k, l)) {
            push(c, id, check);
          }
        }
      }
      break;
    }
    check = ch;
    if (len <= check) {
      break;
    }
    if (PAT_CHECK_IS_TERMINATED(check)) {
      grn_id id0 = node->lr[0];
      pat_node *node0;
      PAT_AT(pat, id0, node0);
      if (!node0) {
        return GRN_FILE_CORRUPT;
      }
      if (!(k = pat_node_get_key(ctx, pat, node0))) {
        return GRN_FILE_CORRUPT;
      }
      {
        uint32_t l = PAT_LEN(node0);
        if (memcmp(key, k, l)) {
          break;
        }
        if (min_size <= l) {
          push(c, id0, check);
        }
      }
      id = node->lr[1];
    } else {
      id = node->lr[nth_bit((uint8_t *)key, check)];
    }
  }
  return GRN_SUCCESS;
}

grn_inline static grn_rc
set_cursor_ascend(grn_ctx *ctx,
                  grn_pat *pat,
                  grn_pat_cursor *c,
                  const void *key,
                  uint32_t key_size,
                  int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int32_t r, check = -1, ch, c2;
  int32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) {
      return GRN_FILE_CORRUPT;
    }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (!(k = pat_node_get_key(ctx, pat, node))) {
        return GRN_FILE_CORRUPT;
      }
      {
        uint32_t l = PAT_LEN(node);
        if (l == key_size) {
          if (flags & GRN_CURSOR_GT) {
            if (memcmp(key, k, l) < 0) {
              push(c, id, check);
            }
          } else {
            if (memcmp(key, k, l) <= 0) {
              push(c, id, check);
            }
          }
        } else if (l < key_size) {
          if (memcmp(key, k, l) < 0) {
            push(c, id, check);
          }
        } else {
          if (memcmp(key, k, key_size) <= 0) {
            push(c, id, check);
          }
        }
      }
      break;
    }
    c2 = len < ch ? len : ch;
    if ((check += 2) < c2) {
      if (!(k = pat_node_get_key(ctx, pat, node))) {
        return GRN_FILE_CORRUPT;
      }
      if ((r = bitcmp(key, k, check >> 1, ((c2 + 1) >> 1) - (check >> 1)))) {
        if (r < 0) {
          push(c, node->lr[1], ch);
          push(c, node->lr[0], ch);
        }
        break;
      }
    }
    check = ch;
    if (len <= check) {
      push(c, node->lr[1], ch);
      push(c, node->lr[0], ch);
      break;
    }
    if (PAT_CHECK_IS_TERMINATED(check)) {
      if (check + 1 < len) {
        id = node->lr[1];
      } else {
        push(c, node->lr[1], check);
        id = node->lr[0];
      }
    } else {
      if (nth_bit((uint8_t *)key, check)) {
        id = node->lr[1];
      } else {
        push(c, node->lr[1], check);
        id = node->lr[0];
      }
    }
  }
  return GRN_SUCCESS;
}

grn_inline static grn_rc
set_cursor_descend(grn_ctx *ctx,
                   grn_pat *pat,
                   grn_pat_cursor *c,
                   const void *key,
                   uint32_t key_size,
                   int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int32_t r, check = -1, ch, c2;
  int32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) {
      return GRN_FILE_CORRUPT;
    }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (!(k = pat_node_get_key(ctx, pat, node))) {
        return GRN_FILE_CORRUPT;
      }
      {
        uint32_t l = PAT_LEN(node);
        if (l <= key_size) {
          if ((flags & GRN_CURSOR_LT) && l == key_size) {
            if (memcmp(key, k, l) > 0) {
              push(c, id, check);
            }
          } else {
            if (memcmp(key, k, l) >= 0) {
              push(c, id, check);
            }
          }
        } else {
          if (memcmp(key, k, key_size) > 0) {
            push(c, id, check);
          }
        }
      }
      break;
    }
    c2 = len < ch ? len : ch;
    if ((check += 2) < c2) {
      if (!(k = pat_node_get_key(ctx, pat, node))) {
        return GRN_FILE_CORRUPT;
      }
      if ((r = bitcmp(key, k, check >> 1, ((c2 + 1) >> 1) - (check >> 1)))) {
        if (r >= 0) {
          push(c, node->lr[0], ch);
          push(c, node->lr[1], ch);
        }
        break;
      }
    }
    check = ch;
    if (len <= check) {
      break;
    }
    if (PAT_CHECK_IS_TERMINATED(check)) {
      if (check + 1 < len) {
        push(c, node->lr[0], check);
        id = node->lr[1];
      } else {
        id = node->lr[0];
      }
    } else {
      if (nth_bit((uint8_t *)key, check)) {
        push(c, node->lr[0], check);
        id = node->lr[1];
      } else {
        id = node->lr[0];
      }
    }
  }
  return GRN_SUCCESS;
}

static grn_pat_cursor *
grn_pat_cursor_open_by_id(grn_ctx *ctx,
                          grn_pat *pat,
                          const void *min,
                          uint32_t min_size,
                          const void *max,
                          uint32_t max_size,
                          int offset,
                          int limit,
                          int flags)
{
  int dir;
  grn_pat_cursor *c;
  if (!pat || !ctx) {
    return NULL;
  }
  if (!(c = GRN_CALLOC(sizeof(grn_pat_cursor)))) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(c, GRN_CURSOR_TABLE_PAT_KEY);
  c->pat = pat;
  c->ctx = ctx;
  c->obj.header.flags = flags;
  c->obj.header.domain = GRN_ID_NIL;
  c->size = 0;
  c->sp = 0;
  c->ss = NULL;
  c->tail = 0;
  if (flags & GRN_CURSOR_DESCENDING) {
    dir = -1;
    if (max) {
      if (!(c->curr_rec = grn_pat_get(ctx, pat, max, max_size, NULL))) {
        c->tail = GRN_ID_NIL;
        goto exit;
      }
      if (!(flags & GRN_CURSOR_LT)) {
        c->curr_rec++;
      }
    } else {
      c->curr_rec = pat->header->curr_rec + 1;
    }
    if (min) {
      if (!(c->tail = grn_pat_get(ctx, pat, min, min_size, NULL))) {
        c->curr_rec = GRN_ID_NIL;
        goto exit;
      }
      if ((flags & GRN_CURSOR_GT)) {
        c->tail++;
      }
    } else {
      c->tail = GRN_ID_NIL + 1;
    }
    if (c->curr_rec < c->tail) {
      c->tail = c->curr_rec;
    }
  } else {
    dir = 1;
    if (min) {
      if (!(c->curr_rec = grn_pat_get(ctx, pat, min, min_size, NULL))) {
        c->tail = GRN_ID_NIL;
        goto exit;
      }
      if (!(flags & GRN_CURSOR_GT)) {
        c->curr_rec--;
      }
    } else {
      c->curr_rec = GRN_ID_NIL;
    }
    if (max) {
      if (!(c->tail = grn_pat_get(ctx, pat, max, max_size, NULL))) {
        c->curr_rec = GRN_ID_NIL;
        goto exit;
      }
      if ((flags & GRN_CURSOR_LT)) {
        c->tail--;
      }
    } else {
      c->tail = pat->header->curr_rec;
    }
    if (c->tail < c->curr_rec) {
      c->tail = c->curr_rec;
    }
  }
  if (pat->header->n_garbages) {
    while (offset && c->curr_rec != c->tail) {
      uint32_t key_size;
      const void *key;
      c->curr_rec += dir;
      key = _grn_pat_key(ctx, pat, c->curr_rec, &key_size);
      if (_grn_pat_get(ctx, pat, key, key_size, NULL) == c->curr_rec) {
        offset--;
      }
    }
  } else {
    if ((int)(dir * (c->tail - c->curr_rec)) < offset) {
      c->curr_rec = c->tail;
    } else {
      c->curr_rec += dir * offset;
    }
  }
  c->rest = (limit < 0) ? GRN_ID_MAX : limit;
exit:
  return c;
}

static grn_rc
set_cursor_rk(grn_ctx *ctx,
              grn_pat *pat,
              grn_pat_cursor *c,
              const void *key,
              uint32_t key_size,
              int flags);

grn_pat_cursor *
grn_pat_cursor_open(grn_ctx *ctx,
                    grn_pat *pat,
                    const void *min,
                    uint32_t min_size,
                    const void *max,
                    uint32_t max_size,
                    int offset,
                    int limit,
                    int flags)
{
  grn_id id;
  pat_node *node;
  grn_pat_cursor *c;
  if (!pat || !ctx) {
    return NULL;
  }
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return NULL;
  }
  if ((flags & GRN_CURSOR_BY_ID)) {
    return grn_pat_cursor_open_by_id(ctx,
                                     pat,
                                     min,
                                     min_size,
                                     max,
                                     max_size,
                                     offset,
                                     limit,
                                     flags);
  }
  if (!(c = GRN_CALLOC(sizeof(grn_pat_cursor)))) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(c, GRN_CURSOR_TABLE_PAT_KEY);
  c->pat = pat;
  c->ctx = ctx;
  c->size = 0;
  c->sp = 0;
  c->ss = NULL;
  c->tail = 0;
  c->rest = GRN_ID_MAX;
  c->curr_rec = GRN_ID_NIL;
  c->obj.header.domain = GRN_ID_NIL;
  if (flags & GRN_CURSOR_PREFIX) {
    if (max && max_size) {
      if ((pat->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)) {
        set_cursor_common_prefix(ctx, pat, c, min_size, max, max_size, flags);
      } else {
        set_cursor_near(ctx, pat, c, min_size, max, flags);
      }
      goto exit;
    } else {
      if (min && min_size) {
        if (flags & GRN_CURSOR_RK) {
          set_cursor_rk(ctx, pat, c, min, min_size, flags);
        } else {
          set_cursor_prefix(ctx, pat, c, min, min_size, flags);
        }
        goto exit;
      }
    }
  }
  if (flags & GRN_CURSOR_DESCENDING) {
    if (min && min_size) {
      set_cursor_ascend(ctx, pat, c, min, min_size, flags);
      c->obj.header.flags = GRN_CURSOR_ASCENDING;
      c->tail = grn_pat_cursor_next(ctx, c);
      c->sp = 0;
      if (!c->tail) {
        goto exit;
      }
    }
    if (max && max_size) {
      set_cursor_descend(ctx, pat, c, max, max_size, flags);
    } else {
      PAT_AT(pat, 0, node);
      if (!node) {
        grn_pat_cursor_close(ctx, c);
        return NULL;
      }
      if ((id = node->lr[1])) {
        PAT_AT(pat, id, node);
        if (node) {
          int ch = PAT_CHK(node);
          push(c, node->lr[0], ch);
          push(c, node->lr[1], ch);
        }
      }
    }
  } else {
    if (max && max_size) {
      set_cursor_descend(ctx, pat, c, max, max_size, flags);
      c->obj.header.flags = GRN_CURSOR_DESCENDING;
      c->tail = grn_pat_cursor_next(ctx, c);
      c->sp = 0;
      if (!c->tail) {
        goto exit;
      }
    }
    if (min && min_size) {
      set_cursor_ascend(ctx, pat, c, min, min_size, flags);
    } else {
      PAT_AT(pat, 0, node);
      if (!node) {
        grn_pat_cursor_close(ctx, c);
        return NULL;
      }
      if ((id = node->lr[1])) {
        PAT_AT(pat, id, node);
        if (node) {
          int ch = PAT_CHK(node);
          push(c, node->lr[1], ch);
          push(c, node->lr[0], ch);
        }
      }
    }
  }
exit:
  c->obj.header.flags = flags;
  c->curr_rec = GRN_ID_NIL;
  while (offset--) {
    grn_pat_cursor_next(ctx, c);
  }
  c->rest = (limit < 0) ? GRN_ID_MAX : limit;
  return c;
}

int
grn_pat_cursor_get_key(grn_ctx *ctx, grn_pat_cursor *c, void **key)
{
  *key = c->curr_key;
  return grn_pat_get_key(ctx,
                         c->pat,
                         c->curr_rec,
                         *key,
                         GRN_TABLE_MAX_KEY_SIZE);
}

int
grn_pat_cursor_get_value(grn_ctx *ctx, grn_pat_cursor *c, void **value)
{
  int value_size = (int)c->pat->value_size;
  if (value_size) {
    byte *v = (byte *)sis_at(ctx, c->pat, c->curr_rec);
    if (v) {
      if (c->pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
        *value = v + sizeof(sis_node);
      } else {
        *value = v;
      }
    } else {
      *value = NULL;
    }
  }
  return value_size;
}

int
grn_pat_cursor_get_key_value(
  grn_ctx *ctx, grn_pat_cursor *c, void **key, uint32_t *key_size, void **value)
{
  int value_size = (int)c->pat->value_size;
  if (key_size) {
    *key_size = (uint32_t)grn_pat_get_key(ctx,
                                          c->pat,
                                          c->curr_rec,
                                          c->curr_key,
                                          GRN_TABLE_MAX_KEY_SIZE);
    if (key) {
      *key = c->curr_key;
    }
  }
  if (value && value_size) {
    byte *v = (byte *)sis_at(ctx, c->pat, c->curr_rec);
    if (v) {
      if (c->pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
        *value = v + sizeof(sis_node);
      } else {
        *value = v;
      }
    } else {
      *value = NULL;
    }
  }
  return value_size;
}

grn_rc
grn_pat_cursor_set_value(grn_ctx *ctx,
                         grn_pat_cursor *c,
                         const void *value,
                         int flags)
{
  return grn_pat_set_value(ctx, c->pat, c->curr_rec, value, flags);
}

grn_rc
grn_pat_cursor_delete(grn_ctx *ctx,
                      grn_pat_cursor *c,
                      grn_table_delete_optarg *optarg)
{
  return grn_pat_delete_by_id(ctx, c->pat, c->curr_rec, optarg);
}

void
grn_pat_check(grn_ctx *ctx, grn_pat *pat)
{
  char buf[8];
  struct grn_pat_header *h = pat->header;
  if (grn_pat_error_if_truncated(ctx, pat) != GRN_SUCCESS) {
    return;
  }
  GRN_OUTPUT_ARRAY_OPEN("RESULT", 1);
  GRN_OUTPUT_MAP_OPEN("SUMMARY", 23);
  GRN_OUTPUT_CSTR("flags");
  grn_itoh(h->flags, buf, 8);
  GRN_OUTPUT_STR(buf, 8);
  GRN_OUTPUT_CSTR("key size");
  GRN_OUTPUT_INT64(h->key_size);
  GRN_OUTPUT_CSTR("value_size");
  GRN_OUTPUT_INT64(h->value_size);
  GRN_OUTPUT_CSTR("tokenizer");
  GRN_OUTPUT_INT64(h->tokenizer);
  GRN_OUTPUT_CSTR("normalizer");
  GRN_OUTPUT_INT64(h->normalizer);
  GRN_OUTPUT_CSTR("n_entries");
  GRN_OUTPUT_INT64(h->n_entries);
  GRN_OUTPUT_CSTR("curr_rec");
  GRN_OUTPUT_INT64(h->curr_rec);
  GRN_OUTPUT_CSTR("curr_key");
  GRN_OUTPUT_INT64(h->curr_key);
  GRN_OUTPUT_CSTR("curr_del");
  GRN_OUTPUT_INT64(h->curr_del);
  GRN_OUTPUT_CSTR("curr_del2");
  GRN_OUTPUT_INT64(h->curr_del2);
  GRN_OUTPUT_CSTR("curr_del3");
  GRN_OUTPUT_INT64(h->curr_del3);
  GRN_OUTPUT_CSTR("n_garbages");
  GRN_OUTPUT_INT64(h->n_garbages);
  GRN_OUTPUT_MAP_CLOSE();
  GRN_OUTPUT_ARRAY_CLOSE();
}

/* utilities */
void
grn_p_pat_node(grn_ctx *ctx, grn_pat *pat, pat_node *node)
{
  uint8_t *key = NULL;

  if (!node) {
    printf("#<pat_node:(null)>\n");
    return;
  }

  if (PAT_IMD(node)) {
    key = (uint8_t *)&(node->key);
  } else {
    KEY_AT(pat, node->key, key, 0);
  }

  printf("#<pat_node:%p "
         "left:%u "
         "right:%u "
         "deleting:%s "
         "immediate:%s "
         "length:%u "
         "nth-byte:%u "
         "nth-bit:%u "
         "terminated:%s "
         "key:<%.*s>"
         ">\n",
         node,
         node->lr[0],
         node->lr[1],
         PAT_DEL(node) ? "true" : "false",
         PAT_IMD(node) ? "true" : "false",
         PAT_LEN(node),
         PAT_CHK(node) >> 4,
         (PAT_CHK(node) >> 1) & 0x7,
         (PAT_CHK(node) & 0x1) ? "true" : "false",
         PAT_LEN(node),
         (char *)key);
}

static void
grn_pat_inspect_check(grn_ctx *ctx, grn_obj *buf, int check)
{
  GRN_TEXT_PUTS(ctx, buf, "{");
  grn_text_lltoa(ctx, buf, PAT_CHECK_BYTE_DIFFERENCES(check));
  GRN_TEXT_PUTS(ctx, buf, ",");
  grn_text_lltoa(ctx, buf, PAT_CHECK_BIT_DIFFERENCES(check));
  GRN_TEXT_PUTS(ctx, buf, ",");
  if (PAT_CHECK_IS_TERMINATED(check)) {
    GRN_TEXT_PUTS(ctx, buf, "true");
  } else {
    GRN_TEXT_PUTS(ctx, buf, "false");
  }
  GRN_TEXT_PUTS(ctx, buf, "}");
}

static void
grn_pat_inspect_node(grn_ctx *ctx,
                     grn_pat *pat,
                     grn_id id,
                     int check,
                     grn_obj *key_buf,
                     int indent,
                     const char *prefix,
                     grn_obj *buf)
{
  pat_node *node = NULL;
  int i, c;

  PAT_AT(pat, id, node);
  c = PAT_CHK(node);

  for (i = 0; i < indent; i++) {
    GRN_TEXT_PUTC(ctx, buf, ' ');
  }
  GRN_TEXT_PUTS(ctx, buf, prefix);
  grn_text_lltoa(ctx, buf, id);
  grn_pat_inspect_check(ctx, buf, c);

  if (c > check) {
    GRN_TEXT_PUTS(ctx, buf, "\n");
    grn_pat_inspect_node(ctx,
                         pat,
                         node->lr[0],
                         c,
                         key_buf,
                         indent + 2,
                         "L:",
                         buf);
    GRN_TEXT_PUTS(ctx, buf, "\n");
    grn_pat_inspect_node(ctx,
                         pat,
                         node->lr[1],
                         c,
                         key_buf,
                         indent + 2,
                         "R:",
                         buf);
  } else if (id != GRN_ID_NIL) {
    int key_size;
    uint8_t *key;

    key_size = PAT_LEN(node);
    GRN_BULK_REWIND(key_buf);
    grn_bulk_space(ctx, key_buf, key_size);
    grn_pat_get_key(ctx, pat, id, GRN_BULK_HEAD(key_buf), key_size);
    GRN_TEXT_PUTS(ctx, buf, "(");
    grn_inspect(ctx, buf, key_buf);
    GRN_TEXT_PUTS(ctx, buf, ")");

    GRN_TEXT_PUTS(ctx, buf, "[");
    key = pat_node_get_key(ctx, pat, node);
    for (i = 0; i < key_size; i++) {
      int j;
      uint8_t byte = key[i];
      if (i != 0) {
        GRN_TEXT_PUTS(ctx, buf, " ");
      }
      for (j = 0; j < 8; j++) {
        grn_text_lltoa(ctx, buf, (byte >> (7 - j)) & 1);
      }
    }
    GRN_TEXT_PUTS(ctx, buf, "]");
  }
}

void
grn_pat_inspect_nodes(grn_ctx *ctx, grn_pat *pat, grn_obj *buf)
{
  pat_node *node;
  grn_obj key_buf;

  GRN_TEXT_PUTS(ctx, buf, "{");
  PAT_AT(pat, GRN_ID_NIL, node);
  if (node->lr[1] != GRN_ID_NIL) {
    GRN_TEXT_PUTS(ctx, buf, "\n");
    GRN_OBJ_INIT(&key_buf, GRN_BULK, 0, pat->obj.header.domain);
    grn_pat_inspect_node(ctx, pat, node->lr[1], -1, &key_buf, 0, "", buf);
    GRN_OBJ_FIN(ctx, &key_buf);
    GRN_TEXT_PUTS(ctx, buf, "\n");
  }
  GRN_TEXT_PUTS(ctx, buf, "}");
}

static void
grn_pat_cursor_inspect_entries(grn_ctx *ctx, grn_pat_cursor *c, grn_obj *buf)
{
  uint32_t i;
  GRN_TEXT_PUTS(ctx, buf, "[");
  for (i = 0; i < c->sp; i++) {
    grn_pat_cursor_entry *e = c->ss + i;
    if (i != 0) {
      GRN_TEXT_PUTS(ctx, buf, ", ");
    }
    GRN_TEXT_PUTS(ctx, buf, "[");
    grn_text_lltoa(ctx, buf, e->id);
    GRN_TEXT_PUTS(ctx, buf, ",");
    grn_pat_inspect_check(ctx, buf, e->check);
    GRN_TEXT_PUTS(ctx, buf, "]");
  }
  GRN_TEXT_PUTS(ctx, buf, "]");
}

void
grn_pat_cursor_inspect(grn_ctx *ctx, grn_pat_cursor *c, grn_obj *buf)
{
  GRN_TEXT_PUTS(ctx, buf, "#<cursor:pat:");
  grn_inspect_name(ctx, buf, (grn_obj *)(c->pat));

  GRN_TEXT_PUTS(ctx, buf, " ");
  GRN_TEXT_PUTS(ctx, buf, "current:");
  grn_text_lltoa(ctx, buf, c->curr_rec);

  GRN_TEXT_PUTS(ctx, buf, " ");
  GRN_TEXT_PUTS(ctx, buf, "tail:");
  grn_text_lltoa(ctx, buf, c->tail);

  GRN_TEXT_PUTS(ctx, buf, " ");
  GRN_TEXT_PUTS(ctx, buf, "flags:");
  if (c->obj.header.flags & GRN_CURSOR_PREFIX) {
    GRN_TEXT_PUTS(ctx, buf, "prefix");
  } else {
    if (c->obj.header.flags & GRN_CURSOR_DESCENDING) {
      GRN_TEXT_PUTS(ctx, buf, "descending");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "ascending");
    }
    GRN_TEXT_PUTS(ctx, buf, "|");
    if (c->obj.header.flags & GRN_CURSOR_GT) {
      GRN_TEXT_PUTS(ctx, buf, "greater-than");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "greater");
    }
    GRN_TEXT_PUTS(ctx, buf, "|");
    if (c->obj.header.flags & GRN_CURSOR_LT) {
      GRN_TEXT_PUTS(ctx, buf, "less-than");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "less");
    }
    if (c->obj.header.flags & GRN_CURSOR_BY_ID) {
      GRN_TEXT_PUTS(ctx, buf, "|by-id");
    }
    if (c->obj.header.flags & GRN_CURSOR_BY_KEY) {
      GRN_TEXT_PUTS(ctx, buf, "|by-key");
    }
  }

  GRN_TEXT_PUTS(ctx, buf, " ");
  GRN_TEXT_PUTS(ctx, buf, "rest:");
  grn_text_lltoa(ctx, buf, c->rest);

  GRN_TEXT_PUTS(ctx, buf, " ");
  GRN_TEXT_PUTS(ctx, buf, "entries:");
  grn_pat_cursor_inspect_entries(ctx, c, buf);

  GRN_TEXT_PUTS(ctx, buf, ">");
}

typedef struct {
  uint8_t code;
  uint8_t next;
  uint8_t emit;
  uint8_t attr;
} rk_tree_node;

static uint16_t rk_str_idx[] = {
  0x0003, 0x0006, 0x0009, 0x000c, 0x0012, 0x0015, 0x0018, 0x001e, 0x0024,
  0x002a, 0x0030, 0x0036, 0x003c, 0x0042, 0x0048, 0x004e, 0x0054, 0x005a,
  0x0060, 0x0066, 0x006c, 0x0072, 0x0078, 0x007e, 0x0084, 0x008a, 0x0090,
  0x0096, 0x009c, 0x00a2, 0x00a8, 0x00ae, 0x00b4, 0x00ba, 0x00c0, 0x00c3,
  0x00c6, 0x00c9, 0x00cc, 0x00cf, 0x00d2, 0x00d5, 0x00db, 0x00e1, 0x00e7,
  0x00ea, 0x00f0, 0x00f6, 0x00fc, 0x00ff, 0x0105, 0x0108, 0x010e, 0x0111,
  0x0114, 0x0117, 0x011a, 0x011d, 0x0120, 0x0123, 0x0129, 0x012f, 0x0135,
  0x013b, 0x013e, 0x0144, 0x014a, 0x0150, 0x0156, 0x0159, 0x015c, 0x015f,
  0x0162, 0x0165, 0x0168, 0x016b, 0x016e, 0x0171, 0x0177, 0x017d, 0x0183,
  0x0189, 0x018c, 0x0192, 0x0198, 0x019e, 0x01a1, 0x01a4, 0x01aa, 0x01b0,
  0x01b6, 0x01bc, 0x01bf, 0x01c2, 0x01c8, 0x01ce, 0x01d1, 0x01d7, 0x01dd,
  0x01e0, 0x01e6, 0x01e9, 0x01ef, 0x01f2, 0x01f5, 0x01fb, 0x0201, 0x0207,
  0x020d, 0x0213, 0x0216, 0x0219, 0x021c, 0x021f, 0x0222, 0x0225, 0x0228,
  0x022e, 0x0234, 0x023a, 0x023d, 0x0243, 0x0249, 0x024f, 0x0252, 0x0258,
  0x025e, 0x0264, 0x0267, 0x026d, 0x0273, 0x0279, 0x027f, 0x0285, 0x0288,
  0x028b, 0x028e, 0x0291, 0x0294, 0x0297, 0x029a, 0x029d, 0x02a0, 0x02a3,
  0x02a9, 0x02af, 0x02b5, 0x02b8, 0x02bb, 0x02be, 0x02c1, 0x02c4, 0x02c7,
  0x02ca, 0x02cd, 0x02d0, 0x02d3, 0x02d6, 0x02dc, 0x02e2, 0x02e8, 0x02eb,
  0x02ee, 0x02f1, 0x02f4, 0x02f7, 0x02fa, 0x02fd, 0x0300, 0x0303, 0x0309,
  0x030c, 0x0312, 0x0318, 0x031e, 0x0324, 0x0327, 0x032a, 0x032d};
static char rk_str[] = {
  0xe3, 0x82, 0xa1, 0xe3, 0x82, 0xa2, 0xe3, 0x82, 0xa3, 0xe3, 0x82, 0xa4, 0xe3,
  0x82, 0xa4, 0xe3, 0x82, 0xa7, 0xe3, 0x82, 0xa5, 0xe3, 0x82, 0xa6, 0xe3, 0x82,
  0xa6, 0xe3, 0x82, 0xa2, 0xe3, 0x82, 0xa6, 0xe3, 0x82, 0xa3, 0xe3, 0x82, 0xa6,
  0xe3, 0x82, 0xa4, 0xe3, 0x82, 0xa6, 0xe3, 0x82, 0xa6, 0xe3, 0x82, 0xa6, 0xe3,
  0x82, 0xa7, 0xe3, 0x82, 0xa6, 0xe3, 0x82, 0xa8, 0xe3, 0x82, 0xa6, 0xe3, 0x82,
  0xaa, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa0, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa1,
  0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa2, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa3, 0xe3,
  0x82, 0xa6, 0xe3, 0x83, 0xa4, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa5, 0xe3, 0x82,
  0xa6, 0xe3, 0x83, 0xa6, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa7, 0xe3, 0x82, 0xa6,
  0xe3, 0x83, 0xa8, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xa9, 0xe3, 0x82, 0xa6, 0xe3,
  0x83, 0xaa, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xab, 0xe3, 0x82, 0xa6, 0xe3, 0x83,
  0xac, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xad, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xae,
  0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xaf, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xb0, 0xe3,
  0x82, 0xa6, 0xe3, 0x83, 0xb1, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xb2, 0xe3, 0x82,
  0xa6, 0xe3, 0x83, 0xb3, 0xe3, 0x82, 0xa6, 0xe3, 0x83, 0xbc, 0xe3, 0x82, 0xa7,
  0xe3, 0x82, 0xa8, 0xe3, 0x82, 0xa9, 0xe3, 0x82, 0xaa, 0xe3, 0x82, 0xab, 0xe3,
  0x82, 0xac, 0xe3, 0x82, 0xad, 0xe3, 0x82, 0xad, 0xe3, 0x83, 0xa3, 0xe3, 0x82,
  0xad, 0xe3, 0x83, 0xa5, 0xe3, 0x82, 0xad, 0xe3, 0x83, 0xa7, 0xe3, 0x82, 0xae,
  0xe3, 0x82, 0xae, 0xe3, 0x83, 0xa3, 0xe3, 0x82, 0xae, 0xe3, 0x83, 0xa5, 0xe3,
  0x82, 0xae, 0xe3, 0x83, 0xa7, 0xe3, 0x82, 0xaf, 0xe3, 0x82, 0xaf, 0xe3, 0x82,
  0xa1, 0xe3, 0x82, 0xb0, 0xe3, 0x82, 0xb0, 0xe3, 0x82, 0xa1, 0xe3, 0x82, 0xb1,
  0xe3, 0x82, 0xb2, 0xe3, 0x82, 0xb3, 0xe3, 0x82, 0xb4, 0xe3, 0x82, 0xb5, 0xe3,
  0x82, 0xb6, 0xe3, 0x82, 0xb7, 0xe3, 0x82, 0xb7, 0xe3, 0x82, 0xa7, 0xe3, 0x82,
  0xb7, 0xe3, 0x83, 0xa3, 0xe3, 0x82, 0xb7, 0xe3, 0x83, 0xa5, 0xe3, 0x82, 0xb7,
  0xe3, 0x83, 0xa7, 0xe3, 0x82, 0xb8, 0xe3, 0x82, 0xb8, 0xe3, 0x82, 0xa7, 0xe3,
  0x82, 0xb8, 0xe3, 0x83, 0xa3, 0xe3, 0x82, 0xb8, 0xe3, 0x83, 0xa5, 0xe3, 0x82,
  0xb8, 0xe3, 0x83, 0xa7, 0xe3, 0x82, 0xb9, 0xe3, 0x82, 0xba, 0xe3, 0x82, 0xbb,
  0xe3, 0x82, 0xbc, 0xe3, 0x82, 0xbd, 0xe3, 0x82, 0xbe, 0xe3, 0x82, 0xbf, 0xe3,
  0x83, 0x80, 0xe3, 0x83, 0x81, 0xe3, 0x83, 0x81, 0xe3, 0x82, 0xa7, 0xe3, 0x83,
  0x81, 0xe3, 0x83, 0xa3, 0xe3, 0x83, 0x81, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x81,
  0xe3, 0x83, 0xa7, 0xe3, 0x83, 0x82, 0xe3, 0x83, 0x82, 0xe3, 0x83, 0xa3, 0xe3,
  0x83, 0x82, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x82, 0xe3, 0x83, 0xa7, 0xe3, 0x83,
  0x83, 0xe3, 0x83, 0x84, 0xe3, 0x83, 0x84, 0xe3, 0x82, 0xa1, 0xe3, 0x83, 0x84,
  0xe3, 0x82, 0xa3, 0xe3, 0x83, 0x84, 0xe3, 0x82, 0xa7, 0xe3, 0x83, 0x84, 0xe3,
  0x82, 0xa9, 0xe3, 0x83, 0x85, 0xe3, 0x83, 0x86, 0xe3, 0x83, 0x86, 0xe3, 0x82,
  0xa3, 0xe3, 0x83, 0x86, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x87, 0xe3, 0x83, 0x87,
  0xe3, 0x82, 0xa3, 0xe3, 0x83, 0x87, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x88, 0xe3,
  0x83, 0x88, 0xe3, 0x82, 0xa5, 0xe3, 0x83, 0x89, 0xe3, 0x83, 0x89, 0xe3, 0x82,
  0xa5, 0xe3, 0x83, 0x8a, 0xe3, 0x83, 0x8b, 0xe3, 0x83, 0x8b, 0xe3, 0x82, 0xa3,
  0xe3, 0x83, 0x8b, 0xe3, 0x82, 0xa7, 0xe3, 0x83, 0x8b, 0xe3, 0x83, 0xa3, 0xe3,
  0x83, 0x8b, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x8b, 0xe3, 0x83, 0xa7, 0xe3, 0x83,
  0x8c, 0xe3, 0x83, 0x8d, 0xe3, 0x83, 0x8e, 0xe3, 0x83, 0x8f, 0xe3, 0x83, 0x90,
  0xe3, 0x83, 0x91, 0xe3, 0x83, 0x92, 0xe3, 0x83, 0x92, 0xe3, 0x83, 0xa3, 0xe3,
  0x83, 0x92, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x92, 0xe3, 0x83, 0xa7, 0xe3, 0x83,
  0x93, 0xe3, 0x83, 0x93, 0xe3, 0x83, 0xa3, 0xe3, 0x83, 0x93, 0xe3, 0x83, 0xa5,
  0xe3, 0x83, 0x93, 0xe3, 0x83, 0xa7, 0xe3, 0x83, 0x94, 0xe3, 0x83, 0x94, 0xe3,
  0x83, 0xa3, 0xe3, 0x83, 0x94, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x94, 0xe3, 0x83,
  0xa7, 0xe3, 0x83, 0x95, 0xe3, 0x83, 0x95, 0xe3, 0x82, 0xa1, 0xe3, 0x83, 0x95,
  0xe3, 0x82, 0xa3, 0xe3, 0x83, 0x95, 0xe3, 0x82, 0xa7, 0xe3, 0x83, 0x95, 0xe3,
  0x82, 0xa9, 0xe3, 0x83, 0x95, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0x96, 0xe3, 0x83,
  0x97, 0xe3, 0x83, 0x98, 0xe3, 0x83, 0x99, 0xe3, 0x83, 0x9a, 0xe3, 0x83, 0x9b,
  0xe3, 0x83, 0x9c, 0xe3, 0x83, 0x9d, 0xe3, 0x83, 0x9e, 0xe3, 0x83, 0x9f, 0xe3,
  0x83, 0x9f, 0xe3, 0x83, 0xa3, 0xe3, 0x83, 0x9f, 0xe3, 0x83, 0xa5, 0xe3, 0x83,
  0x9f, 0xe3, 0x83, 0xa7, 0xe3, 0x83, 0xa0, 0xe3, 0x83, 0xa1, 0xe3, 0x83, 0xa2,
  0xe3, 0x83, 0xa3, 0xe3, 0x83, 0xa4, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0xa6, 0xe3,
  0x83, 0xa7, 0xe3, 0x83, 0xa8, 0xe3, 0x83, 0xa9, 0xe3, 0x83, 0xaa, 0xe3, 0x83,
  0xaa, 0xe3, 0x83, 0xa3, 0xe3, 0x83, 0xaa, 0xe3, 0x83, 0xa5, 0xe3, 0x83, 0xaa,
  0xe3, 0x83, 0xa7, 0xe3, 0x83, 0xab, 0xe3, 0x83, 0xac, 0xe3, 0x83, 0xad, 0xe3,
  0x83, 0xae, 0xe3, 0x83, 0xaf, 0xe3, 0x83, 0xb0, 0xe3, 0x83, 0xb1, 0xe3, 0x83,
  0xb2, 0xe3, 0x83, 0xb3, 0xe3, 0x83, 0xb3, 0xe3, 0x83, 0xbc, 0xe3, 0x83, 0xb4,
  0xe3, 0x83, 0xb4, 0xe3, 0x82, 0xa1, 0xe3, 0x83, 0xb4, 0xe3, 0x82, 0xa3, 0xe3,
  0x83, 0xb4, 0xe3, 0x82, 0xa7, 0xe3, 0x83, 0xb4, 0xe3, 0x82, 0xa9, 0xe3, 0x83,
  0xb5, 0xe3, 0x83, 0xb6, 0xe3, 0x83, 0xbc};
static uint16_t rk_tree_idx[] = {
  0x001b, 0x0022, 0x0025, 0x0028, 0x002d, 0x0030, 0x0039, 0x003b, 0x003c,
  0x003f, 0x0046, 0x0047, 0x004f, 0x0050, 0x0053, 0x005a, 0x005d, 0x0064,
  0x0067, 0x006f, 0x0070, 0x0073, 0x007d, 0x007f, 0x0081, 0x0082, 0x0083,
  0x0088, 0x008f, 0x0092, 0x00af, 0x00b5, 0x00bc, 0x00bf, 0x00c6, 0x00c9,
  0x00d1, 0x00d6, 0x00da, 0x00e4, 0x00e6, 0x00eb, 0x00ec, 0x00f0, 0x00f6,
  0x00fc, 0x00fe, 0x0108, 0x010a, 0x010c, 0x010d, 0x010e, 0x0113, 0x0118,
  0x011f, 0x0123, 0x0125, 0x0164, 0x0180, 0x0183, 0x0199, 0x01ad};
static rk_tree_node rk_tree[] = {
  {0x2d, 0x00, 0xb2, 0x01}, {0x61, 0x00, 0x01, 0x01}, {0x62, 0x01, 0xff, 0x01},
  {0x63, 0x03, 0xff, 0x01}, {0x64, 0x06, 0xff, 0x01}, {0x65, 0x00, 0x24, 0x01},
  {0x66, 0x0a, 0xff, 0x01}, {0x67, 0x0c, 0xff, 0x01}, {0x68, 0x0f, 0xff, 0x01},
  {0x69, 0x00, 0x03, 0x01}, {0x6a, 0x11, 0xff, 0x01}, {0x6b, 0x13, 0xff, 0x01},
  {0x6c, 0x16, 0xff, 0x01}, {0x6d, 0x1c, 0xff, 0x01}, {0x6e, 0x1e, 0xff, 0x01},
  {0x6f, 0x00, 0x26, 0x01}, {0x70, 0x20, 0xff, 0x01}, {0x72, 0x22, 0xff, 0x01},
  {0x73, 0x24, 0xff, 0x01}, {0x74, 0x27, 0xff, 0x01}, {0x75, 0x00, 0x06, 0x01},
  {0x76, 0x2c, 0xff, 0x01}, {0x77, 0x2d, 0xff, 0x01}, {0x78, 0x2f, 0xff, 0x01},
  {0x79, 0x35, 0xff, 0x01}, {0x7a, 0x36, 0xff, 0x01}, {0xe3, 0x38, 0xff, 0x01},
  {0x61, 0x00, 0x72, 0x01}, {0x62, 0x01, 0x56, 0x01}, {0x65, 0x00, 0x89, 0x01},
  {0x69, 0x00, 0x78, 0x01}, {0x6f, 0x00, 0x8c, 0x01}, {0x75, 0x00, 0x86, 0x01},
  {0x79, 0x02, 0xff, 0x00}, {0x61, 0x00, 0x79, 0x01}, {0x6f, 0x00, 0x7b, 0x01},
  {0x75, 0x00, 0x7a, 0x01}, {0x63, 0x03, 0x56, 0x01}, {0x68, 0x04, 0xff, 0x01},
  {0x79, 0x05, 0xff, 0x01}, {0x61, 0x00, 0x4f, 0x00}, {0x65, 0x00, 0x4e, 0x00},
  {0x69, 0x00, 0x4d, 0x01}, {0x6f, 0x00, 0x51, 0x00}, {0x75, 0x00, 0x50, 0x00},
  {0x61, 0x00, 0x4f, 0x01}, {0x6f, 0x00, 0x51, 0x01}, {0x75, 0x00, 0x50, 0x01},
  {0x61, 0x00, 0x4c, 0x01}, {0x64, 0x06, 0x56, 0x01}, {0x65, 0x00, 0x60, 0x01},
  {0x68, 0x07, 0xff, 0x00}, {0x69, 0x00, 0x61, 0x00}, {0x6f, 0x00, 0x65, 0x01},
  {0x75, 0x00, 0x5c, 0x01}, {0x77, 0x08, 0xff, 0x00}, {0x79, 0x09, 0xff, 0x01},
  {0x69, 0x00, 0x61, 0x01}, {0x75, 0x00, 0x62, 0x01}, {0x75, 0x00, 0x66, 0x01},
  {0x61, 0x00, 0x53, 0x01}, {0x6f, 0x00, 0x55, 0x01}, {0x75, 0x00, 0x54, 0x01},
  {0x61, 0x00, 0x81, 0x00}, {0x65, 0x00, 0x83, 0x00}, {0x66, 0x0a, 0x56, 0x01},
  {0x69, 0x00, 0x82, 0x00}, {0x6f, 0x00, 0x84, 0x00}, {0x75, 0x00, 0x80, 0x01},
  {0x79, 0x0b, 0xff, 0x00}, {0x75, 0x00, 0x85, 0x01}, {0x61, 0x00, 0x28, 0x01},
  {0x65, 0x00, 0x36, 0x01}, {0x67, 0x0c, 0x56, 0x01}, {0x69, 0x00, 0x2d, 0x01},
  {0x6f, 0x00, 0x38, 0x01}, {0x75, 0x00, 0x33, 0x01}, {0x77, 0x0d, 0xff, 0x00},
  {0x79, 0x0e, 0xff, 0x00}, {0x61, 0x00, 0x34, 0x01}, {0x61, 0x00, 0x2e, 0x01},
  {0x6f, 0x00, 0x30, 0x01}, {0x75, 0x00, 0x2f, 0x01}, {0x61, 0x00, 0x71, 0x01},
  {0x65, 0x00, 0x88, 0x01}, {0x68, 0x0f, 0x56, 0x01}, {0x69, 0x00, 0x74, 0x01},
  {0x6f, 0x00, 0x8b, 0x01}, {0x75, 0x00, 0x80, 0x01}, {0x79, 0x10, 0xff, 0x00},
  {0x61, 0x00, 0x75, 0x01}, {0x6f, 0x00, 0x77, 0x01}, {0x75, 0x00, 0x76, 0x01},
  {0x61, 0x00, 0x42, 0x00}, {0x65, 0x00, 0x41, 0x00}, {0x69, 0x00, 0x40, 0x01},
  {0x6a, 0x11, 0x56, 0x01}, {0x6f, 0x00, 0x44, 0x00}, {0x75, 0x00, 0x43, 0x00},
  {0x79, 0x12, 0xff, 0x00}, {0x61, 0x00, 0x42, 0x01}, {0x6f, 0x00, 0x44, 0x01},
  {0x75, 0x00, 0x43, 0x01}, {0x61, 0x00, 0x27, 0x01}, {0x65, 0x00, 0x35, 0x01},
  {0x69, 0x00, 0x29, 0x01}, {0x6b, 0x13, 0x56, 0x01}, {0x6f, 0x00, 0x37, 0x01},
  {0x75, 0x00, 0x31, 0x01}, {0x77, 0x14, 0xff, 0x00}, {0x79, 0x15, 0xff, 0x00},
  {0x61, 0x00, 0x32, 0x01}, {0x61, 0x00, 0x2a, 0x01}, {0x6f, 0x00, 0x2c, 0x01},
  {0x75, 0x00, 0x2b, 0x01}, {0x61, 0x00, 0x00, 0x01}, {0x65, 0x00, 0x23, 0x01},
  {0x69, 0x00, 0x02, 0x01}, {0x6b, 0x17, 0xff, 0x01}, {0x6c, 0x16, 0x56, 0x01},
  {0x6f, 0x00, 0x25, 0x01}, {0x74, 0x18, 0xff, 0x01}, {0x75, 0x00, 0x05, 0x01},
  {0x77, 0x1a, 0xff, 0x01}, {0x79, 0x1b, 0xff, 0x01}, {0x61, 0x00, 0xb0, 0x01},
  {0x65, 0x00, 0xb1, 0x01}, {0x73, 0x19, 0xff, 0x00}, {0x75, 0x00, 0x56, 0x01},
  {0x75, 0x00, 0x56, 0x01}, {0x61, 0x00, 0xa4, 0x01}, {0x61, 0x00, 0x96, 0x01},
  {0x65, 0x00, 0x23, 0x01}, {0x69, 0x00, 0x02, 0x01}, {0x6f, 0x00, 0x9a, 0x01},
  {0x75, 0x00, 0x98, 0x01}, {0x61, 0x00, 0x8e, 0x01}, {0x65, 0x00, 0x94, 0x01},
  {0x69, 0x00, 0x8f, 0x01}, {0x6d, 0x1c, 0x56, 0x01}, {0x6f, 0x00, 0x95, 0x01},
  {0x75, 0x00, 0x93, 0x01}, {0x79, 0x1d, 0xff, 0x00}, {0x61, 0x00, 0x90, 0x01},
  {0x6f, 0x00, 0x92, 0x01}, {0x75, 0x00, 0x91, 0x01}, {0x00, 0x00, 0xa9, 0x01},
  {0x27, 0x00, 0xa9, 0x00}, {0x2d, 0x00, 0xaa, 0x00}, {0x61, 0x00, 0x67, 0x01},
  {0x62, 0x01, 0xa9, 0x00}, {0x63, 0x03, 0xa9, 0x00}, {0x64, 0x06, 0xa9, 0x00},
  {0x65, 0x00, 0x6f, 0x01}, {0x66, 0x0a, 0xa9, 0x00}, {0x67, 0x0c, 0xa9, 0x00},
  {0x68, 0x0f, 0xa9, 0x00}, {0x69, 0x00, 0x68, 0x01}, {0x6a, 0x11, 0xa9, 0x00},
  {0x6b, 0x13, 0xa9, 0x00}, {0x6c, 0x16, 0xa9, 0x00}, {0x6d, 0x1c, 0xa9, 0x00},
  {0x6e, 0x00, 0xa9, 0x00}, {0x6f, 0x00, 0x70, 0x01}, {0x70, 0x20, 0xa9, 0x00},
  {0x72, 0x22, 0xa9, 0x00}, {0x73, 0x24, 0xa9, 0x00}, {0x74, 0x27, 0xa9, 0x00},
  {0x75, 0x00, 0x6e, 0x01}, {0x76, 0x2c, 0xa9, 0x00}, {0x77, 0x2d, 0xa9, 0x00},
  {0x78, 0x2f, 0xa9, 0x00}, {0x79, 0x1f, 0xff, 0x00}, {0x7a, 0x36, 0xa9, 0x00},
  {0xe3, 0x38, 0xa9, 0x00}, {0x00, 0x00, 0xa9, 0x01}, {0x61, 0x00, 0x6b, 0x01},
  {0x65, 0x00, 0x6a, 0x01}, {0x69, 0x00, 0x69, 0x01}, {0x6f, 0x00, 0x6d, 0x01},
  {0x75, 0x00, 0x6c, 0x01}, {0x61, 0x00, 0x73, 0x01}, {0x65, 0x00, 0x8a, 0x01},
  {0x69, 0x00, 0x7c, 0x01}, {0x6f, 0x00, 0x8d, 0x01}, {0x70, 0x20, 0x56, 0x01},
  {0x75, 0x00, 0x87, 0x01}, {0x79, 0x21, 0xff, 0x00}, {0x61, 0x00, 0x7d, 0x01},
  {0x6f, 0x00, 0x7f, 0x01}, {0x75, 0x00, 0x7e, 0x01}, {0x61, 0x00, 0x9c, 0x01},
  {0x65, 0x00, 0xa2, 0x01}, {0x69, 0x00, 0x9d, 0x01}, {0x6f, 0x00, 0xa3, 0x01},
  {0x72, 0x22, 0x56, 0x01}, {0x75, 0x00, 0xa1, 0x01}, {0x79, 0x23, 0xff, 0x00},
  {0x61, 0x00, 0x9e, 0x01}, {0x6f, 0x00, 0xa0, 0x01}, {0x75, 0x00, 0x9f, 0x01},
  {0x61, 0x00, 0x39, 0x01}, {0x65, 0x00, 0x47, 0x01}, {0x68, 0x25, 0xff, 0x00},
  {0x69, 0x00, 0x3b, 0x01}, {0x6f, 0x00, 0x49, 0x01}, {0x73, 0x24, 0x56, 0x01},
  {0x75, 0x00, 0x45, 0x01}, {0x79, 0x26, 0xff, 0x00}, {0x61, 0x00, 0x3d, 0x00},
  {0x65, 0x00, 0x3c, 0x00}, {0x69, 0x00, 0x3b, 0x01}, {0x6f, 0x00, 0x3f, 0x00},
  {0x75, 0x00, 0x3e, 0x00}, {0x61, 0x00, 0x3d, 0x01}, {0x65, 0x00, 0x3c, 0x01},
  {0x6f, 0x00, 0x3f, 0x01}, {0x75, 0x00, 0x3e, 0x01}, {0x61, 0x00, 0x4b, 0x01},
  {0x65, 0x00, 0x5d, 0x01}, {0x68, 0x28, 0xff, 0x00}, {0x69, 0x00, 0x4d, 0x01},
  {0x6f, 0x00, 0x63, 0x01}, {0x73, 0x29, 0xff, 0x00}, {0x74, 0x27, 0x56, 0x01},
  {0x75, 0x00, 0x57, 0x01}, {0x77, 0x2a, 0xff, 0x00}, {0x79, 0x2b, 0xff, 0x00},
  {0x69, 0x00, 0x5e, 0x01}, {0x75, 0x00, 0x5f, 0x01}, {0x61, 0x00, 0x58, 0x00},
  {0x65, 0x00, 0x5a, 0x00}, {0x69, 0x00, 0x59, 0x00}, {0x6f, 0x00, 0x5b, 0x00},
  {0x75, 0x00, 0x57, 0x01}, {0x75, 0x00, 0x64, 0x01}, {0x61, 0x00, 0x4f, 0x01},
  {0x65, 0x00, 0x4e, 0x01}, {0x6f, 0x00, 0x51, 0x01}, {0x75, 0x00, 0x50, 0x01},
  {0x61, 0x00, 0xac, 0x00}, {0x65, 0x00, 0xae, 0x00}, {0x69, 0x00, 0xad, 0x00},
  {0x6f, 0x00, 0xaf, 0x00}, {0x75, 0x00, 0xab, 0x01}, {0x76, 0x2c, 0x56, 0x01},
  {0x61, 0x00, 0xa5, 0x01}, {0x65, 0x00, 0x0b, 0x01}, {0x69, 0x00, 0x08, 0x01},
  {0x6f, 0x00, 0xa8, 0x01}, {0x77, 0x2d, 0x56, 0x01}, {0x79, 0x2e, 0xff, 0x01},
  {0x65, 0x00, 0xa7, 0x01}, {0x69, 0x00, 0xa6, 0x01}, {0x61, 0x00, 0x00, 0x01},
  {0x65, 0x00, 0x23, 0x01}, {0x69, 0x00, 0x02, 0x01}, {0x6b, 0x30, 0xff, 0x01},
  {0x6f, 0x00, 0x25, 0x01}, {0x74, 0x31, 0xff, 0x01}, {0x75, 0x00, 0x05, 0x01},
  {0x77, 0x33, 0xff, 0x01}, {0x78, 0x2f, 0x56, 0x01}, {0x79, 0x34, 0xff, 0x01},
  {0x61, 0x00, 0xb0, 0x01}, {0x65, 0x00, 0xb1, 0x01}, {0x73, 0x32, 0xff, 0x00},
  {0x75, 0x00, 0x56, 0x01}, {0x75, 0x00, 0x56, 0x01}, {0x61, 0x00, 0xa4, 0x01},
  {0x61, 0x00, 0x96, 0x01}, {0x65, 0x00, 0x23, 0x01}, {0x69, 0x00, 0x02, 0x01},
  {0x6f, 0x00, 0x9a, 0x01}, {0x75, 0x00, 0x98, 0x01}, {0x61, 0x00, 0x97, 0x01},
  {0x65, 0x00, 0x04, 0x01}, {0x6f, 0x00, 0x9b, 0x01}, {0x75, 0x00, 0x99, 0x01},
  {0x79, 0x35, 0x56, 0x01}, {0x61, 0x00, 0x3a, 0x01}, {0x65, 0x00, 0x48, 0x01},
  {0x69, 0x00, 0x40, 0x01}, {0x6f, 0x00, 0x4a, 0x01}, {0x75, 0x00, 0x46, 0x01},
  {0x79, 0x37, 0xff, 0x00}, {0x7a, 0x36, 0x56, 0x01}, {0x61, 0x00, 0x42, 0x01},
  {0x65, 0x00, 0x41, 0x01}, {0x6f, 0x00, 0x44, 0x01}, {0x75, 0x00, 0x43, 0x01},
  {0x81, 0x39, 0xff, 0x01}, {0x82, 0x3d, 0xff, 0x01}, {0x81, 0x00, 0x00, 0x01},
  {0x82, 0x00, 0x01, 0x01}, {0x83, 0x00, 0x02, 0x01}, {0x84, 0x00, 0x03, 0x01},
  {0x85, 0x00, 0x05, 0x01}, {0x86, 0x3a, 0xff, 0x01}, {0x87, 0x00, 0x23, 0x01},
  {0x88, 0x00, 0x24, 0x01}, {0x89, 0x00, 0x25, 0x01}, {0x8a, 0x00, 0x26, 0x01},
  {0x8b, 0x00, 0x27, 0x01}, {0x8c, 0x00, 0x28, 0x01}, {0x8d, 0x00, 0x29, 0x01},
  {0x8e, 0x00, 0x2d, 0x01}, {0x8f, 0x00, 0x31, 0x01}, {0x90, 0x00, 0x33, 0x01},
  {0x91, 0x00, 0x35, 0x01}, {0x92, 0x00, 0x36, 0x01}, {0x93, 0x00, 0x37, 0x01},
  {0x94, 0x00, 0x38, 0x01}, {0x95, 0x00, 0x39, 0x01}, {0x96, 0x00, 0x3a, 0x01},
  {0x97, 0x00, 0x3b, 0x01}, {0x98, 0x00, 0x40, 0x01}, {0x99, 0x00, 0x45, 0x01},
  {0x9a, 0x00, 0x46, 0x01}, {0x9b, 0x00, 0x47, 0x01}, {0x9c, 0x00, 0x48, 0x01},
  {0x9d, 0x00, 0x49, 0x01}, {0x9e, 0x00, 0x4a, 0x01}, {0x9f, 0x00, 0x4b, 0x01},
  {0xa0, 0x00, 0x4c, 0x01}, {0xa1, 0x00, 0x4d, 0x01}, {0xa2, 0x00, 0x52, 0x01},
  {0xa3, 0x00, 0x56, 0x01}, {0xa4, 0x00, 0x57, 0x01}, {0xa5, 0x00, 0x5c, 0x01},
  {0xa6, 0x00, 0x5d, 0x01}, {0xa7, 0x00, 0x60, 0x01}, {0xa8, 0x00, 0x63, 0x01},
  {0xa9, 0x00, 0x65, 0x01}, {0xaa, 0x00, 0x67, 0x01}, {0xab, 0x00, 0x68, 0x01},
  {0xac, 0x00, 0x6e, 0x01}, {0xad, 0x00, 0x6f, 0x01}, {0xae, 0x00, 0x70, 0x01},
  {0xaf, 0x00, 0x71, 0x01}, {0xb0, 0x00, 0x72, 0x01}, {0xb1, 0x00, 0x73, 0x01},
  {0xb2, 0x00, 0x74, 0x01}, {0xb3, 0x00, 0x78, 0x01}, {0xb4, 0x00, 0x7c, 0x01},
  {0xb5, 0x00, 0x80, 0x01}, {0xb6, 0x00, 0x86, 0x01}, {0xb7, 0x00, 0x87, 0x01},
  {0xb8, 0x00, 0x88, 0x01}, {0xb9, 0x00, 0x89, 0x01}, {0xba, 0x00, 0x8a, 0x01},
  {0xbb, 0x00, 0x8b, 0x01}, {0xbc, 0x00, 0x8c, 0x01}, {0xbd, 0x00, 0x8d, 0x01},
  {0xbe, 0x00, 0x8e, 0x01}, {0xbf, 0x00, 0x8f, 0x01}, {0x00, 0x00, 0x06, 0x00},
  {0x2d, 0x00, 0x22, 0x00}, {0x61, 0x00, 0x07, 0x00}, {0x62, 0x01, 0x06, 0x00},
  {0x63, 0x03, 0x06, 0x00}, {0x64, 0x06, 0x06, 0x00}, {0x65, 0x00, 0x0c, 0x00},
  {0x66, 0x0a, 0x06, 0x00}, {0x67, 0x0c, 0x06, 0x00}, {0x68, 0x0f, 0x06, 0x00},
  {0x69, 0x00, 0x09, 0x00}, {0x6a, 0x11, 0x06, 0x00}, {0x6b, 0x13, 0x06, 0x00},
  {0x6c, 0x16, 0x06, 0x00}, {0x6d, 0x1c, 0x06, 0x00}, {0x6e, 0x1e, 0x06, 0x00},
  {0x6f, 0x00, 0x0d, 0x00}, {0x70, 0x20, 0x06, 0x00}, {0x72, 0x22, 0x06, 0x00},
  {0x73, 0x24, 0x06, 0x00}, {0x74, 0x27, 0x06, 0x00}, {0x75, 0x00, 0x0a, 0x00},
  {0x76, 0x2c, 0x06, 0x00}, {0x77, 0x2d, 0x06, 0x00}, {0x78, 0x2f, 0x06, 0x00},
  {0x79, 0x35, 0x06, 0x00}, {0x7a, 0x36, 0x06, 0x00}, {0xe3, 0x3b, 0xff, 0x01},
  {0x00, 0x00, 0x06, 0x00}, {0x81, 0x39, 0x06, 0x00}, {0x82, 0x3c, 0xff, 0x01},
  {0x00, 0x00, 0x06, 0x01}, {0x80, 0x00, 0x0e, 0x00}, {0x81, 0x00, 0x0f, 0x00},
  {0x82, 0x00, 0x10, 0x00}, {0x83, 0x00, 0x11, 0x00}, {0x84, 0x00, 0x12, 0x00},
  {0x85, 0x00, 0x13, 0x00}, {0x86, 0x00, 0x14, 0x00}, {0x87, 0x00, 0x15, 0x00},
  {0x88, 0x00, 0x16, 0x00}, {0x89, 0x00, 0x17, 0x00}, {0x8a, 0x00, 0x18, 0x00},
  {0x8b, 0x00, 0x19, 0x00}, {0x8c, 0x00, 0x1a, 0x00}, {0x8d, 0x00, 0x1b, 0x00},
  {0x8e, 0x00, 0x1c, 0x00}, {0x8f, 0x00, 0x1d, 0x00}, {0x90, 0x00, 0x1e, 0x00},
  {0x91, 0x00, 0x1f, 0x00}, {0x92, 0x00, 0x20, 0x00}, {0x93, 0x00, 0x21, 0x00},
  {0x9b, 0x00, 0xab, 0x01}, {0x80, 0x00, 0x93, 0x01}, {0x81, 0x00, 0x94, 0x01},
  {0x82, 0x00, 0x95, 0x01}, {0x83, 0x00, 0x96, 0x01}, {0x84, 0x00, 0x97, 0x01},
  {0x85, 0x00, 0x98, 0x01}, {0x86, 0x00, 0x99, 0x01}, {0x87, 0x00, 0x9a, 0x01},
  {0x88, 0x00, 0x9b, 0x01}, {0x89, 0x00, 0x9c, 0x01}, {0x8a, 0x00, 0x9d, 0x01},
  {0x8b, 0x00, 0xa1, 0x01}, {0x8c, 0x00, 0xa2, 0x01}, {0x8d, 0x00, 0xa3, 0x01},
  {0x8e, 0x00, 0xa4, 0x01}, {0x8f, 0x00, 0xa5, 0x01}, {0x90, 0x00, 0xa6, 0x01},
  {0x91, 0x00, 0xa7, 0x01}, {0x92, 0x00, 0xa8, 0x01}, {0x93, 0x00, 0xa9, 0x01}};

static rk_tree_node *
rk_lookup(uint8_t state, uint8_t code)
{
  if (state < sizeof(rk_tree_idx) / sizeof(uint16_t)) {
    uint16_t ns = state ? rk_tree_idx[state - 1] : 0;
    uint16_t ne = rk_tree_idx[state];
    while (ns < ne) {
      uint16_t m = (ns + ne) >> 1;
      rk_tree_node *rn = &rk_tree[m];
      if (rn->code == code) {
        return rn;
      }
      if (rn->code < code) {
        ns = m + 1;
      } else {
        ne = m;
      }
    }
  }
  return NULL;
}

static uint32_t
rk_emit(rk_tree_node *rn, char **str)
{
  if (rn && rn->emit != 0xff) {
    uint16_t pos = rn->emit ? rk_str_idx[rn->emit - 1] : 0;
    *str = &rk_str[pos];
    return (uint32_t)(rk_str_idx[rn->emit] - pos);
  } else {
    *str = NULL;
    return 0;
  }
}

#define RK_OUTPUT(e, l)                                                        \
  do {                                                                         \
    if (oc < oe) {                                                             \
      uint32_t l_ = (oc + (l) < oe) ? (l) : (oe - oc);                         \
      grn_memcpy(oc, (e), l_);                                                 \
      oc += l_;                                                                \
      ic_ = ic;                                                                \
    }                                                                          \
  } while (0)

static uint32_t
rk_conv(const char *str,
        uint32_t str_len,
        uint8_t *buf,
        uint32_t buf_size,
        uint8_t *statep)
{
  uint32_t l;
  uint8_t state = 0;
  rk_tree_node *rn;
  char *e;
  uint8_t *oc = buf, *oe = oc + buf_size;
  const uint8_t *ic = (uint8_t *)str, *ic_ = ic, *ie = ic + str_len;
  while (ic < ie) {
    if ((rn = rk_lookup(state, *ic))) {
      ic++;
      if ((l = rk_emit(rn, &e))) {
        RK_OUTPUT(e, l);
      }
      state = rn->next;
    } else {
      if (!state) {
        ic++;
      }
      if (ic_ < ic) {
        RK_OUTPUT(ic_, ic - ic_);
      }
      state = 0;
    }
  }
#ifdef FLUSH_UNRESOLVED_INPUT
  if ((rn = rk_lookup(state, 0))) {
    if ((l = rk_emit(rn, &e))) {
      RK_OUTPUT(e, l);
    }
    state = rn->next;
  } else {
    if (ic_ < ic) {
      RK_OUTPUT(ic_, ic - ic_);
    }
  }
#endif /* FLUSH_UNRESOLVED_INPUT */
  *statep = state;
  return oc - buf;
}

static grn_id
sub_search(grn_ctx *ctx,
           grn_pat *pat,
           grn_id id,
           int32_t *c0,
           uint8_t *key,
           uint32_t key_len)
{
  pat_node *pn;
  int32_t len = key_len * 16;
  if (!key_len) {
    return id;
  }
  PAT_AT(pat, id, pn);
  while (pn) {
    int32_t ch;
    ch = PAT_CHK(pn);
    if (*c0 < ch && ch < len - 1) {
      if (ch & 1) {
        id = (ch + 1 < len) ? pn->lr[1] : pn->lr[0];
      } else {
        id = pn->lr[nth_bit(key, ch)];
      }
      *c0 = ch;
      PAT_AT(pat, id, pn);
    } else {
      const uint8_t *k = pat_node_get_key(ctx, pat, pn);
      return (k && key_len <= PAT_LEN(pn) && !memcmp(k, key, key_len))
               ? id
               : GRN_ID_NIL;
    }
  }
  return GRN_ID_NIL;
}

static void
search_push(grn_ctx *ctx,
            grn_pat *pat,
            grn_pat_cursor *c,
            uint8_t *key,
            uint32_t key_len,
            uint8_t state,
            grn_id id,
            int c0,
            int flags)
{
  if (state) {
    int step;
    uint16_t ns, ne;
    if (flags & GRN_CURSOR_DESCENDING) {
      ns = rk_tree_idx[state - 1];
      ne = rk_tree_idx[state];
      step = 1;
    } else {
      ns = rk_tree_idx[state] - 1;
      ne = rk_tree_idx[state - 1] - 1;
      step = -1;
    }
    for (; ns != ne; ns += step) {
      rk_tree_node *rn = &rk_tree[ns];
      if (rn->attr) {
        char *e;
        uint32_t l = rk_emit(rn, &e);
        if (l) {
          if (l + key_len <= GRN_TABLE_MAX_KEY_SIZE) {
            int32_t ch = c0;
            grn_id i;
            grn_memcpy(key + key_len, e, l);
            if ((i = sub_search(ctx, pat, id, &ch, key, key_len + l))) {
              search_push(ctx,
                          pat,
                          c,
                          key,
                          key_len + l,
                          rn->next,
                          i,
                          ch,
                          flags);
            }
          }
        } else {
          search_push(ctx, pat, c, key, key_len, rn->next, id, c0, flags);
        }
      }
    }
  } else {
    pat_node *pn;
    PAT_AT(pat, id, pn);
    if (pn) {
      int32_t ch = PAT_CHK(pn);
      int32_t len = key_len * 16;
      if (c0 < ch) {
        if (flags & GRN_CURSOR_DESCENDING) {
          if ((ch > len - 1) || !(flags & GRN_CURSOR_GT)) {
            push(c, pn->lr[0], ch);
          }
          push(c, pn->lr[1], ch);
        } else {
          push(c, pn->lr[1], ch);
          if ((ch > len - 1) || !(flags & GRN_CURSOR_GT)) {
            push(c, pn->lr[0], ch);
          }
        }
      } else {
        if (PAT_LEN(pn) * 16 > (uint32_t)len || !(flags & GRN_CURSOR_GT)) {
          push(c, id, ch);
        }
      }
    }
  }
}

static grn_rc
set_cursor_rk(grn_ctx *ctx,
              grn_pat *pat,
              grn_pat_cursor *c,
              const void *key,
              uint32_t key_len,
              int flags)
{
  grn_id id;
  uint8_t state;
  pat_node *pn;
  int c0 = -1;
  uint32_t len, byte_len;
  uint8_t keybuf[GRN_TABLE_MAX_KEY_SIZE];
  if (flags & GRN_CURSOR_SIZE_BY_BIT) {
    return GRN_OPERATION_NOT_SUPPORTED;
  }
  byte_len = rk_conv(key, key_len, keybuf, GRN_TABLE_MAX_KEY_SIZE, &state);
  len = byte_len * 16;
  PAT_AT(pat, 0, pn);
  id = pn->lr[1];
  if ((id = sub_search(ctx, pat, id, &c0, keybuf, byte_len))) {
    search_push(ctx, pat, c, keybuf, byte_len, state, id, c0, flags);
  }
  return ctx->rc;
}

uint32_t
grn_pat_total_key_size(grn_ctx *ctx, grn_pat *pat)
{
  return pat->header->curr_key;
}

grn_bool
grn_pat_is_key_encoded(grn_ctx *ctx, grn_pat *pat)
{
  grn_obj *domain;
  uint32_t key_size;

  domain = grn_ctx_at(ctx, pat->obj.header.domain);
  if (grn_obj_is_type(ctx, domain)) {
    key_size = grn_type_size(ctx, domain);
  } else {
    key_size = sizeof(grn_id);
  }

  return KEY_NEEDS_CONVERT(pat, key_size);
}

grn_rc
grn_pat_dirty(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc = GRN_SUCCESS;

  CRITICAL_SECTION_ENTER(pat->lock);
  if (!pat->is_dirty) {
    uint32_t n_dirty_opens;
    pat->is_dirty = GRN_TRUE;
    GRN_ATOMIC_ADD_EX(&(pat->header->n_dirty_opens), 1, n_dirty_opens);
    rc = grn_io_flush(ctx, pat->io);
  }
  CRITICAL_SECTION_LEAVE(pat->lock);

  return rc;
}

grn_bool
grn_pat_is_dirty(grn_ctx *ctx, grn_pat *pat)
{
  return pat->header->n_dirty_opens > 0;
}

grn_rc
grn_pat_clean(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc = GRN_SUCCESS;

  CRITICAL_SECTION_ENTER(pat->lock);
  if (pat->is_dirty) {
    uint32_t n_dirty_opens;
    pat->is_dirty = GRN_FALSE;
    GRN_ATOMIC_ADD_EX(&(pat->header->n_dirty_opens), -1, n_dirty_opens);
    rc = grn_io_flush(ctx, pat->io);
  }
  CRITICAL_SECTION_LEAVE(pat->lock);

  return rc;
}

grn_rc
grn_pat_clear_dirty(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc = GRN_SUCCESS;

  CRITICAL_SECTION_ENTER(pat->lock);
  pat->is_dirty = GRN_FALSE;
  pat->header->n_dirty_opens = 0;
  rc = grn_io_flush(ctx, pat->io);
  CRITICAL_SECTION_LEAVE(pat->lock);

  return rc;
}

static void
grn_pat_wal_recover_add_entry(grn_ctx *ctx,
                              grn_pat *pat,
                              grn_wal_reader_entry *entry,
                              const char *tag,
                              const char *wal_error_tag)
{
  pat->header->curr_key = (uint32_t)(entry->key_offset);
  pat->header->n_entries = entry->n_entries;
  pat_node *node = pat_get(ctx, pat, entry->record_id);
  if (!node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  pat_node *parent_node = pat_get(ctx, pat, entry->parent_record_id);
  if (!parent_node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer parent node");
    return;
  }
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }
  grn_id *id_location = &(parent_node->lr[entry->record_direction]);
  *id_location = entry->previous_record_id;
  uint16_t check_max =
    (uint16_t)PAT_CHECK_PACK(entry->key.content.binary.size, 0, false);
  grn_pat_add_node(ctx,
                   pat,
                   node,
                   entry->record_id,
                   entry->key.content.binary.data,
                   entry->key.content.binary.size,
                   entry->check,
                   check_max,
                   id_location,
                   tag);
}

static void
grn_pat_wal_recover_add_shared_entry(grn_ctx *ctx,
                                     grn_pat *pat,
                                     grn_wal_reader_entry *entry,
                                     const char *wal_error_tag)
{
  pat->header->n_entries = entry->n_entries;
  pat_node *node = pat_get(ctx, pat, entry->record_id);
  if (!node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  pat_node *parent_node = pat_get(ctx, pat, entry->parent_record_id);
  if (!parent_node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer parent node");
    return;
  }
  grn_id *id_location = &(parent_node->lr[entry->record_direction]);
  *id_location = entry->previous_record_id;
  uint16_t check_max =
    (uint16_t)PAT_CHECK_PACK(entry->key.content.binary.size, 0, false);
  grn_pat_add_shared_node(ctx,
                          pat,
                          node,
                          entry->record_id,
                          entry->key.content.binary.data,
                          entry->key.content.binary.size,
                          (uint32_t)(entry->shared_key_offset),
                          entry->check,
                          check_max,
                          id_location);
}

static void
grn_pat_wal_recover_reuse_entry(grn_ctx *ctx,
                                grn_pat *pat,
                                grn_wal_reader_entry *entry,
                                const char *tag,
                                const char *wal_error_tag)
{
  pat->header->n_garbages = entry->n_garbages;
  pat->header->n_entries = entry->n_entries;
  pat_node *node;
  PAT_AT(pat, entry->record_id, node);
  if (!node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  pat_node *parent_node;
  PAT_AT(pat, entry->parent_record_id, parent_node);
  if (!parent_node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer parent node");
    return;
  }
  grn_id *id_location = &(parent_node->lr[entry->record_direction]);
  *id_location = entry->previous_record_id;
  uint16_t check_max =
    (uint16_t)PAT_CHECK_PACK(entry->key.content.binary.size, 0, false);
  grn_pat_reuse_node(ctx,
                     pat,
                     node,
                     entry->record_id,
                     entry->key.content.binary.data,
                     entry->key.content.binary.size,
                     entry->check,
                     check_max,
                     id_location,
                     tag);
}

static void
grn_pat_wal_recover_reuse_shared_entry(grn_ctx *ctx,
                                       grn_pat *pat,
                                       grn_wal_reader_entry *entry,
                                       const char *wal_error_tag)
{
  pat_node *node = pat_get(ctx, pat, entry->record_id);
  if (!node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  node->lr[0] = entry->next_garbage_record_id;
  pat_node *parent_node = pat_get(ctx, pat, entry->parent_record_id);
  if (!parent_node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer parent node");
    return;
  }
  grn_id *id_location = &(parent_node->lr[entry->record_direction]);
  *id_location = entry->previous_record_id;
  uint16_t check_max =
    (uint16_t)PAT_CHECK_PACK(entry->key.content.binary.size, 0, false);
  pat->header->n_garbages = entry->n_garbages;
  pat->header->n_entries = entry->n_entries;
  grn_pat_reuse_shared_node(ctx,
                            pat,
                            node,
                            entry->record_id,
                            entry->key.content.binary.data,
                            entry->key.content.binary.size,
                            (uint32_t)(entry->shared_key_offset),
                            entry->check,
                            check_max,
                            id_location);
}

static void
grn_pat_wal_recover_delete_info_phase1(grn_ctx *ctx,
                                       grn_pat *pat,
                                       grn_wal_reader_entry *entry,
                                       const char *wal_error_tag)
{
  pat->header->curr_del = entry->delete_info_phase1_index;
  grn_pat_delinfo *di;
  uint32_t next_delete_info_index = delinfo_turn_1_pre(ctx, pat, &di);
  di->shared = entry->is_shared;
  delinfo_turn_1_post(ctx, pat, next_delete_info_index);
}

static void
grn_pat_wal_recover_delete_info_phase2(grn_ctx *ctx,
                                       grn_pat *pat,
                                       grn_wal_reader_entry *entry,
                                       const char *wal_error_tag)
{
  pat->header->curr_del2 = entry->delete_info_phase2_index;
  grn_pat_delinfo *di = &(pat->header->delinfos[pat->header->curr_del2]);
  di->stat = DL_PHASE1;
  di->d = entry->record_id;
  pat_node *node;
  PAT_AT(pat, di->d, node);
  if (!node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  di->ld = entry->parent_record_id;
  pat_node *parent_node;
  PAT_AT(pat, di->ld, parent_node);
  if (!parent_node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer parent node");
    return;
  }
  PAT_LEN_SET(node, (uint16_t)(entry->key.content.uint64));
  PAT_CHK_SET(node, entry->check);
  node->lr[0] = entry->left_record_id;
  node->lr[1] = entry->right_record_id;
  delinfo_turn_2_internal(ctx, pat, di, parent_node, node);
  delinfo_turn_2_post(ctx, pat);
}

static void
grn_pat_wal_recover_delete_info_phase3(grn_ctx *ctx,
                                       grn_pat *pat,
                                       grn_wal_reader_entry *entry,
                                       const char *wal_error_tag)
{
  pat->header->curr_del3 = entry->delete_info_phase3_index;
  grn_pat_delinfo *di = &(pat->header->delinfos[pat->header->curr_del3]);
  di->stat = DL_PHASE2;
  di->d = entry->record_id;
  pat_node *node;
  PAT_AT(pat, di->d, node);
  if (!node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  di->shared = entry->is_shared;
  uint32_t size = delinfo_compute_storage_size(ctx, di, node);
  pat->header->garbages[size] = entry->next_garbage_record_id;
  delinfo_turn_3_internal(ctx, pat, di, node);
  delinfo_turn_3_post(ctx, pat);
}

static void
grn_pat_wal_recover_delete_entry(grn_ctx *ctx,
                                 grn_pat *pat,
                                 grn_wal_reader_entry *entry,
                                 const char *wal_error_tag)
{
  pat->header->curr_del = entry->delete_info_phase1_index;
  pat->header->curr_del2 = entry->delete_info_phase2_index;
  grn_pat_del_data data = {0};
  data.di = &(pat->header->delinfos[entry->delete_info_index]);
  data.id = entry->record_id;
  data.check = entry->check;
  data.check0 = entry->parent_check;
  PAT_AT(pat, data.id, data.rn);
  if (!data.rn) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer node");
    return;
  }
  PAT_AT(pat, entry->parent_record_id, data.rn0);
  if (!data.rn0) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer parent node");
    return;
  }
  pat_node *grandparent_node;
  PAT_AT(pat, entry->grandparent_record_id, grandparent_node);
  if (!grandparent_node) {
    grn_wal_set_recover_error(ctx,
                              GRN_NO_MEMORY_AVAILABLE,
                              (grn_obj *)pat,
                              entry,
                              wal_error_tag,
                              "failed to refer grandparent node");
    return;
  }
  data.otherside = entry->otherside_record_id;
  if (data.otherside != GRN_ID_NIL) {
    PAT_AT(pat, data.otherside, data.rno);
    if (!data.rno) {
      grn_wal_set_recover_error(ctx,
                                GRN_NO_MEMORY_AVAILABLE,
                                (grn_obj *)pat,
                                entry,
                                wal_error_tag,
                                "failed to refer otherside node");
      return;
    }
  }
  pat_node *root_node;
  PAT_AT(pat, GRN_ID_NIL, root_node);
  data.proot = &(root_node->lr[1]);
  data.p = &(data.rn0->lr[entry->record_direction]);
  data.p0 = &(grandparent_node->lr[entry->parent_record_direction]);
  data.n_garbages = entry->n_garbages;
  data.n_entries = entry->n_entries;
  grn_pat_del_internal(ctx, pat, &data);
}

grn_rc
grn_pat_wal_recover(grn_ctx *ctx, grn_pat *pat)
{
  if (ctx->rc != GRN_SUCCESS) {
    return ctx->rc;
  }

  const char *wal_error_tag = "[pat]";
  const char *tag = "[pat][recover]";
  grn_wal_reader *reader = grn_wal_reader_open(ctx, (grn_obj *)pat, tag);
  if (ctx->rc != GRN_SUCCESS) {
    return ctx->rc;
  }

  bool need_flush = false;
  if (grn_io_is_locked(pat->io)) {
    grn_io_clear_lock(pat->io);
    need_flush = true;
  }

  if (reader) {
    while (true) {
      grn_wal_reader_entry entry = {0};
      grn_rc rc = grn_wal_reader_read_entry(ctx, reader, &entry);
      if (rc != GRN_SUCCESS) {
        break;
      }
      switch (entry.event) {
      case GRN_WAL_EVENT_ADD_ENTRY:
        grn_pat_wal_recover_add_entry(ctx, pat, &entry, tag, wal_error_tag);
        break;
      case GRN_WAL_EVENT_ADD_SHARED_ENTRY:
        grn_pat_wal_recover_add_shared_entry(ctx, pat, &entry, wal_error_tag);
        break;
      case GRN_WAL_EVENT_REUSE_ENTRY:
        grn_pat_wal_recover_reuse_entry(ctx, pat, &entry, tag, wal_error_tag);
        break;
      case GRN_WAL_EVENT_REUSE_SHARED_ENTRY:
        grn_pat_wal_recover_reuse_shared_entry(ctx, pat, &entry, wal_error_tag);
        break;
      case GRN_WAL_EVENT_DELETE_INFO_PHASE1:
        grn_pat_wal_recover_delete_info_phase1(ctx, pat, &entry, wal_error_tag);
        break;
      case GRN_WAL_EVENT_DELETE_INFO_PHASE2:
        grn_pat_wal_recover_delete_info_phase2(ctx, pat, &entry, wal_error_tag);
        break;
      case GRN_WAL_EVENT_DELETE_INFO_PHASE3:
        grn_pat_wal_recover_delete_info_phase3(ctx, pat, &entry, wal_error_tag);
        break;
      case GRN_WAL_EVENT_DELETE_ENTRY:
        grn_pat_wal_recover_delete_entry(ctx, pat, &entry, wal_error_tag);
        break;
      default:
        grn_wal_set_recover_error(ctx,
                                  GRN_FUNCTION_NOT_IMPLEMENTED,
                                  (grn_obj *)pat,
                                  &entry,
                                  wal_error_tag,
                                  "not implemented yet");
        break;
      }
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      pat->header->wal_id = entry.id;
      need_flush = true;
    }
    grn_wal_reader_close(ctx, reader);
  }

  if (need_flush && ctx->rc == GRN_SUCCESS) {
    grn_obj_touch(ctx, (grn_obj *)pat, NULL);
    grn_obj_flush(ctx, (grn_obj *)pat);
  }

  return ctx->rc;
}

grn_rc
grn_pat_warm(grn_ctx *ctx, grn_pat *pat)
{
  return grn_io_warm(ctx, pat->io);
}
