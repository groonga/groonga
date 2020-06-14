/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018-2020 Sutou Kouhei <kou@clear-code.com>

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
#include "grn_pat.h"

#include <stdio.h>

static int
grn_table_sort_index(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *index,
                     int offset,
                     int limit,
                     grn_obj *result,
                     grn_table_sort_key *keys,
                     int n_keys)
{
  int n_sorted_records = 0;
  int e = offset + limit;
  grn_id tid;
  grn_pat *lexicon = (grn_pat *)grn_ctx_at(ctx, index->header.domain);
  grn_pat_cursor *pc = grn_pat_cursor_open(ctx, lexicon, NULL, 0, NULL, 0,
                                           0 /* offset : can be used in unique index */,
                                           -1 /* limit : can be used in unique index */,
                                           (keys->flags & GRN_TABLE_SORT_DESC)
                                           ? GRN_CURSOR_DESCENDING
                                           : GRN_CURSOR_ASCENDING);
  if (pc) {
    while (n_sorted_records < e && (tid = grn_pat_cursor_next(ctx, pc))) {
      grn_ii_cursor *ic = grn_ii_cursor_open(ctx, (grn_ii *)index, tid, 0, 0, 1, 0);
      if (ic) {
        grn_posting *posting;
        while (n_sorted_records < e && (posting = grn_ii_cursor_next(ctx, ic))) {
          if (offset <= n_sorted_records) {
            grn_id *v;
            if (!grn_array_add(ctx, (grn_array *)result, (void **)&v)) { break; }
            *v = posting->rid;
          }
          n_sorted_records++;
        }
        grn_ii_cursor_close(ctx, ic);
      }
    }
    grn_pat_cursor_close(ctx, pc);
  }
  return n_sorted_records;
}

typedef enum {
  KEY_ID = 0,
  KEY_BULK,
  KEY_INT8,
  KEY_INT16,
  KEY_INT32,
  KEY_INT64,
  KEY_UINT8,
  KEY_UINT16,
  KEY_UINT32,
  KEY_UINT64,
  KEY_FLOAT32,
  KEY_FLOAT64,
} sort_key_offset;

typedef struct sort_key_ sort_key;
typedef union sort_entry_ sort_entry;
typedef struct sort_data_ sort_data;
typedef struct sort_compare_data_ sort_compare_data;

typedef const uint8_t *(*sort_value_get_func)(grn_ctx *ctx,
                                              const sort_key *key,
                                              sort_entry *entry,
                                              sort_compare_data *compare_data,
                                              uint32_t *size,
                                              sort_data *data);

struct sort_key_ {
  uint32_t index;
  uint32_t index_refer;
  uint32_t index_value;
  grn_obj *key;
  grn_table_sort_flags flags;
  bool can_refer;
  sort_key_offset offset;
  grn_column_cache *column_cache;
  sort_value_get_func get_value;
};

grn_inline static size_t
grn_sort_key_size(sort_key_offset offset)
{
  if (offset == KEY_BULK) {
    return sizeof(uint32_t);
  } else {
    return sizeof(uint8_t);
  }
}

grn_inline static uint32_t
grn_sort_key_get_size(const uint8_t *size_raw,
                      sort_key_offset key_offset)
{
  if (key_offset == KEY_BULK) {
    return *((const uint32_t *)size_raw);
  } else {
    return *((const uint8_t *)size_raw);
  }
}

grn_inline static void
grn_sort_key_set_size(uint8_t *size_raw,
                      sort_key_offset key_offset,
                      uint32_t size)
{
  if (key_offset == KEY_BULK) {
    *((uint32_t *)size_raw) = size;
  } else {
    *((uint8_t *)size_raw) = size;
  }
}

typedef struct {
  grn_obj *values;
  uint8_t *sizes;
} sort_value_copy;

union sort_entry_ {
  struct {
    grn_id id;
  } common;
  struct {
    grn_id id;
    uint32_t size;
    const void *value;
  } refer;
  struct {
    grn_id id;
    uint32_t n_cached_values;
    sort_value_copy *copy;
  } value;
};

struct sort_data_ {
  int offset;
  int limit;
  grn_obj *result;
  sort_key *keys;
  uint32_t n_keys;
  sort_entry *entries;
};

struct sort_compare_data_ {
  size_t values_offset;
  size_t sizes_offset;
};

#define CMPNUM(type) do {\
  if (GRN_LIKELY(as > 0)) {\
    if (GRN_LIKELY(bs > 0)) {\
      type va = *((type *)(ap));\
      type vb = *((type *)(bp));\
      if (va != vb) { return va > vb; }\
    } else {\
      return 1;\
    }\
  } else {\
    if (bs) { return 0; }\
  }\
} while (0)

#define CMPFLOAT(type) do {\
  if (GRN_LIKELY(as > 0)) {\
    if (GRN_LIKELY(bs > 0)) {\
      type va = *((type *)(ap));\
      type vb = *((type *)(bp));\
      if (va < vb || va > vb) { return va > vb; }\
    } else {\
      return 1;\
    }\
  } else {\
    if (GRN_LIKELY(bs > 0)) { return 0; }\
  }\
} while (0)


static const uint8_t *
sort_value_get_refer(grn_ctx *ctx,
                     const sort_key *key,
                     sort_entry *entry,
                     sort_compare_data *compare_data,
                     uint32_t *size,
                     sort_data *data)
{
  return grn_obj_get_value_(ctx, key->key, entry->common.id, size);
}

static const uint8_t *
sort_value_get_value(grn_ctx *ctx,
                     const sort_key *key,
                     sort_entry *entry,
                     sort_compare_data *compare_data,
                     uint32_t *size,
                     sort_data *data)
{
  sort_value_copy *copy = entry->value.copy;
  if (entry->value.n_cached_values <= key->index_value) {
    if (key->column_cache) {
      void *value;
      size_t value_size;
      value = grn_column_cache_ref(ctx,
                                   key->column_cache,
                                   entry->value.id,
                                   &value_size);
      GRN_TEXT_PUT(ctx, copy->values, value, value_size);
      grn_sort_key_set_size(copy->sizes + compare_data->sizes_offset,
                            key->offset,
                            value_size);
    } else {
      size_t value_size_before = GRN_BULK_VSIZE(copy->values);
      grn_obj_get_value(ctx, key->key, entry->value.id, copy->values);
      size_t value_size = GRN_BULK_VSIZE(copy->values) - value_size_before;
      grn_sort_key_set_size(copy->sizes + compare_data->sizes_offset,
                            key->offset,
                            value_size);
    }
    entry->value.n_cached_values++;
  }
  const uint8_t *value =
    (const uint8_t *)GRN_BULK_HEAD(copy->values) +
    compare_data->values_offset;
  *size =
    grn_sort_key_get_size(copy->sizes + compare_data->sizes_offset,
                          key->offset);
  compare_data->values_offset += *size;
  compare_data->sizes_offset += grn_sort_key_size(key->offset);
  return value;
}

static grn_inline int
sort_value_compare(grn_ctx *ctx,
                   sort_entry *a,
                   sort_entry *b,
                   sort_data *data)
{
  const sort_key *keys = data->keys;
  const uint32_t n_keys = data->n_keys;
  uint32_t i;
  const uint8_t *ap;
  const uint8_t *bp;
  uint32_t as;
  uint32_t bs;
  sort_compare_data a_data = {0};
  sort_compare_data b_data = {0};
  for (i = 0; i < n_keys; i++, keys++) {
    if (keys->get_value) {
      if (keys->flags & GRN_TABLE_SORT_DESC) {
        bp = keys->get_value(ctx, keys, a, &a_data, &bs, data);
        ap = keys->get_value(ctx, keys, b, &b_data, &as, data);
      } else {
        ap = keys->get_value(ctx, keys, a, &a_data, &as, data);
        bp = keys->get_value(ctx, keys, b, &b_data, &bs, data);
      }
    } else {
      if (keys->flags & GRN_TABLE_SORT_DESC) {
        bp = a->refer.value;
        bs = a->refer.size;
        ap = b->refer.value;
        as = b->refer.size;
      } else {
        ap = a->refer.value;
        as = a->refer.size;
        bp = b->refer.value;
        bs = b->refer.size;
      }
    }
    switch (keys->offset) {
    case KEY_ID :
      if (keys->can_refer) {
        if (ap != bp) { return ap > bp; }
      } else {
        CMPNUM(grn_id);
      }
      break;
    case KEY_BULK :
      for (;; ap++, bp++, as--, bs--) {
        if (!as) { if (bs) { return 0; } else { break; } }
        if (!bs) { return 1; }
        if (*ap < *bp) { return 0; }
        if (*ap > *bp) { return 1; }
      }
      break;
    case KEY_INT8 :
      CMPNUM(int8_t);
      break;
    case KEY_INT16 :
      CMPNUM(int16_t);
      break;
    case KEY_INT32 :
      CMPNUM(int32_t);
      break;
    case KEY_INT64 :
      CMPNUM(int64_t);
      break;
    case KEY_UINT8 :
      CMPNUM(uint8_t);
      break;
    case KEY_UINT16 :
      CMPNUM(uint16_t);
      break;
    case KEY_UINT32 :
      CMPNUM(uint32_t);
      break;
    case KEY_UINT64 :
      CMPNUM(uint64_t);
      break;
    case KEY_FLOAT32 :
      CMPFLOAT(float);
      break;
    case KEY_FLOAT64 :
      CMPFLOAT(double);
      break;
    }
  }
  return 0;
}

static grn_inline void
sort_value_swap(grn_ctx *ctx, sort_entry *a, sort_entry *b, sort_data *data)
{
  sort_entry c = *a;
  *a = *b;
  *b = c;
}

static grn_inline sort_entry *
sort_value_part(grn_ctx *ctx,
                sort_entry *b,
                sort_entry *e,
                sort_data *data)
{
  sort_entry *c;
  intptr_t d = e - b;
  if (sort_value_compare(ctx, b, e, data)) {
    sort_value_swap(ctx, b, e, data);
  }
  if (d < 2) { return NULL; }
  c = b + (d >> 1);
  if (sort_value_compare(ctx, b, c, data)) {
    sort_value_swap(ctx, b, c, data);
  } else {
    if (sort_value_compare(ctx, c, e, data)) {
      sort_value_swap(ctx, c, e, data);
    }
  }
  if (d < 3) { return NULL; }
  b++;
  sort_value_swap(ctx, b, c, data);
  c = b;
  for (;;) {
    do {
      b++;
    } while (sort_value_compare(ctx, c, b, data));
    do {
      e--;
    } while (sort_value_compare(ctx, e, c, data));
    if (b >= e) { break; }
    sort_value_swap(ctx, b, e, data);
  }
  sort_value_swap(ctx, c, e, data);
  return e;
}

static grn_inline void
sort_value_body(grn_ctx *ctx,
                sort_entry *head,
                sort_entry *tail,
                int from,
                int to,
                sort_data *data)
{
  sort_entry *c;
  if (head < tail && (c = sort_value_part(ctx, head, tail, data))) {
    intptr_t m = c - head + 1;
    if (from < m - 1) {
      sort_value_body(ctx, head, c - 1, from, to, data);
    }
    if (m < to) {
      sort_value_body(ctx, c + 1, tail, from - m, to - m, data);
    }
  }
}

static grn_inline sort_entry *
sort_value_pack(grn_ctx *ctx,
                grn_obj *table,
                sort_entry *head,
                sort_entry *tail,
                sort_data *data)
{
  sort_entry *entries = head;
  size_t n_ids = 0;
  if (data->keys[0].get_value) {
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
      entries[n_ids].common.id = id;
      n_ids++;
    } GRN_TABLE_EACH_END(ctx, cursor);
  } else {
    grn_obj *key = data->keys[0].key;
    grn_accessor *key_accessor = (grn_accessor *)key;
    grn_column_cache *column_cache;
    /* _key->cachable_column */
    if (grn_obj_is_accessor(ctx, key) &&
        key_accessor->action == GRN_ACCESSOR_GET_KEY &&
        key_accessor->next &&
        key_accessor->next->action == GRN_ACCESSOR_GET_COLUMN_VALUE &&
        !key_accessor->next->next &&
        (column_cache = grn_column_cache_open(ctx, key_accessor->next->obj))) {
      GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
        sort_entry *entry = &(entries[n_ids]);
        entry->common.id = id;
        void *key;
        int key_size = grn_table_cursor_get_key(ctx, cursor, &key);
        if (key_size == 0) {
          entry->refer.value = NULL;
          entry->refer.size = 0;
        } else {
          grn_id resolved_id = *((grn_id *)key);
          size_t value_size;
          entry->refer.value =
            grn_column_cache_ref(ctx,
                                 column_cache,
                                 resolved_id,
                                 &value_size);
          entry->refer.size = value_size;
        }
        n_ids++;
      } GRN_TABLE_EACH_END(ctx, cursor);
      grn_column_cache_close(ctx, column_cache);
    } else {
      GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id) {
        sort_entry *entry = &(entries[n_ids]);
        entry->common.id = id;
        entry->refer.value =
          grn_obj_get_value_(ctx,
                             key,
                             entry->refer.id,
                             &(entry->refer.size));
        n_ids++;
      } GRN_TABLE_EACH_END(ctx, cursor);
    }
  }

  /* We can use "return sort_value_part(ctx, head, tail, data)"
   * here but we implement custom part logic here to use the same
   * logic before. It's easy to test that we use the same logic before
   * here because we can get the same sort order for the same sort key
   * value case. We will use the different logic when we use Apache
   * Arrow or something for sorting. */
  {
    grn_obj entry_queue;
    GRN_TEXT_INIT(&entry_queue, 0);
    sort_entry first = *head;
    sort_entry *target;
    for (target = head + 1; target < tail; target++) {
      if (sort_value_compare(ctx, &first, target, data)) {
        sort_value_swap(ctx, head, target, data);
        head++;
      } else {
        grn_bulk_write(ctx,
                       &entry_queue,
                       (const char *)tail,
                       sizeof(sort_entry));
        *tail-- = *target;
      }
    }
    if (target == tail) {
      if (sort_value_compare(ctx, &first, target, data)) {
        sort_value_swap(ctx, head, target, data);
        head++;
      } else {
        grn_bulk_write(ctx,
                       &entry_queue,
                       (const char *)tail,
                       sizeof(sort_entry));
      }
    }
    size_t i;
    const size_t n_queued_entries =
      GRN_BULK_VSIZE(&entry_queue) / sizeof(sort_entry);
    sort_entry *entry_queue_raw = (sort_entry *)GRN_BULK_HEAD(&entry_queue);
    for (i = 0; i < n_queued_entries; i++) {
      sort_entry *entry = entry_queue_raw + (n_queued_entries - i - 1);
      if (sort_value_compare(ctx, &first, entry, data)) {
        *head++ = *entry;
      } else {
        *tail-- = *entry;
      }
    }
    GRN_OBJ_FIN(ctx, &entry_queue);
    *head = first;
  }

  return n_ids > 2 ? head : NULL;
}

static int
grn_table_sort_value_body(grn_ctx *ctx,
                          grn_obj *table,
                          sort_data *data)
{
  const int offset = data->offset;
  const int limit = data->limit;
  int n_sorted_records = 0;
  uint32_t e = offset + limit;
  uint32_t n = grn_table_size(ctx, table);

  const sort_key *keys = data->keys;
  const uint32_t n_keys = data->n_keys;

  size_t n_copies = 0;
  size_t copy_size_per_entry = 0;
  {
    uint32_t i;
    for (i = 0; i < n_keys; i++) {
      if (!keys[i].can_refer) {
        n_copies++;
        copy_size_per_entry += grn_sort_key_size(keys[i].offset);
      }
    }
  }

  sort_entry *entries = NULL;
  grn_obj *bulks = NULL;
  uint8_t *sizes = NULL;
  sort_value_copy *copies = NULL;
  if (!(entries = GRN_MALLOC(sizeof(sort_entry) * n))) {
    goto exit;
  }
  data->entries = entries;
  if (n_copies > 0) {
    bulks = GRN_MALLOC(sizeof(grn_obj) * n);
    if (!bulks) {
      goto exit;
    }
    {
      uint32_t i;
      for (i = 0; i < n; i++) {
        grn_obj *values = &(bulks[i]);
        GRN_TEXT_INIT(values, 0);
      }
    }
    sizes = GRN_MALLOC(copy_size_per_entry * n);
    if (!sizes) {
      goto exit;
    }
    copies = GRN_MALLOC(sizeof(sort_value_copy) * n);
    if (!copies) {
      goto exit;
    }
    {
      uint32_t i;
      for (i = 0; i < n; i++) {
        copies[i].values = &(bulks[i]);
        copies[i].sizes = sizes + copy_size_per_entry * i;
        entries[i].value.n_cached_values = 0;
        entries[i].value.copy = &(copies[i]);
      }
    }
  }

  {
    sort_entry *ep;
    if ((ep = sort_value_pack(ctx, table, entries, entries + n - 1, data))) {
      intptr_t m = ep - entries + 1;
      if (offset < m - 1) {
        sort_value_body(ctx,
                        entries,
                        ep - 1,
                        offset,
                        e,
                        data);
      }
      if (m < e) {
        sort_value_body(ctx,
                        ep + 1,
                        entries + n - 1,
                        offset - m,
                        e - m,
                        data);
      }
    }
  }
  {
    int i;
    sort_entry *ep;
    grn_id *v;
    grn_array *result = (grn_array *)(data->result);
    for (i = 0, ep = entries + offset;
         i < limit && ep < entries + n;
         i++, ep++) {
      if (!grn_array_add(ctx, result, (void **)&v)) { break; }
      *v = ep->common.id;
    }
    n_sorted_records = i;
  }

exit :
  if (entries) {
    GRN_FREE(entries);
  }
  if (copies) {
    GRN_FREE(copies);
  }
  if (bulks) {
    uint32_t i;
    for (i = 0; i < n; i++) {
      GRN_OBJ_FIN(ctx, &(bulks[i]));
    }
    GRN_FREE(bulks);
  }
  if (sizes) {
    GRN_FREE(sizes);
  }
  return n_sorted_records;
}

static grn_bool
is_compressed_column(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj *target_obj;

  if (!obj) {
    return GRN_FALSE;
  }

  if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a = (grn_accessor *)obj;
    while (a->next) {
      a = a->next;
    }
    target_obj = a->obj;
  } else {
    target_obj = obj;
  }

  if (target_obj->header.type != GRN_COLUMN_VAR_SIZE) {
    return GRN_FALSE;
  }

  switch (target_obj->header.flags & GRN_OBJ_COMPRESS_MASK) {
  case GRN_OBJ_COMPRESS_ZLIB :
  case GRN_OBJ_COMPRESS_LZ4 :
  case GRN_OBJ_COMPRESS_ZSTD :
    return GRN_TRUE;
  default :
    return GRN_FALSE;
  }
}

static grn_bool
is_sub_record_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!obj) {
    return GRN_FALSE;
  }

  if (obj->header.type != GRN_ACCESSOR) {
    return GRN_FALSE;
  }

  for (accessor = (grn_accessor *)obj; accessor; accessor = accessor->next) {
    switch (accessor->action) {
    case GRN_ACCESSOR_GET_VALUE :
      if (GRN_TABLE_IS_MULTI_KEYS_GROUPED(accessor->obj)) {
        return GRN_TRUE;
      }
      break;
    default :
      break;
    }
  }

  return GRN_FALSE;
}

static grn_bool
is_encoded_pat_key_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  while (accessor->next) {
    accessor = accessor->next;
  }

  if (accessor->action != GRN_ACCESSOR_GET_KEY) {
    return GRN_FALSE;
  }

  if (accessor->obj->header.type != GRN_TABLE_PAT_KEY) {
    return GRN_FALSE;
  }

  return grn_pat_is_key_encoded(ctx, (grn_pat *)(accessor->obj));
}

static bool
range_is_idp(grn_obj *obj)
{
  if (obj && obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      if (a->action == GRN_ACCESSOR_GET_ID) { return true; }
    }
  }
  return false;
}

static int
grn_table_sort_value(grn_ctx *ctx,
                     grn_obj *table,
                     int offset,
                     int limit,
                     grn_obj *result,
                     grn_table_sort_key *raw_keys,
                     int n_keys)
{
  int n_sorted_records = 0;
  sort_key *keys = NULL;

  keys = GRN_CALLOC(sizeof(sort_key) * n_keys);
  if (!keys) {
    goto exit;
  }

  int i;
  uint32_t index_refer = 0;
  uint32_t index_value = 0;
  for (i = 0; i < n_keys; i++) {
    grn_obj *key = raw_keys[i].key;
    keys[i].index = i;
    keys[i].key = key;
    keys[i].flags = raw_keys[i].flags;

    bool can_refer = true;
    if (is_compressed_column(ctx, key)) {
      can_refer = false;
    }
    if (is_sub_record_accessor(ctx, key)) {
      can_refer = false;
    }
    if (is_encoded_pat_key_accessor(ctx, key)) {
      can_refer = false;
    }
    if (grn_obj_is_score_accessor(ctx, key)) {
      can_refer = false;
    }

    sort_key_offset offset = KEY_ID;
    if (range_is_idp(key)) {
      offset = KEY_ID;
    } else {
      grn_obj *range = grn_ctx_at(ctx, grn_obj_get_range(ctx, key));
      if (range->header.type == GRN_TYPE) {
        if (range->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
          offset = KEY_BULK;
        } else {
          uint8_t key_type = range->header.flags & GRN_OBJ_KEY_MASK;
          switch (key_type) {
          case GRN_OBJ_KEY_UINT :
          case GRN_OBJ_KEY_GEO_POINT :
            switch (GRN_TYPE_SIZE(DB_OBJ(range))) {
            case 1 :
              offset = KEY_UINT8;
              break;
            case 2 :
              offset = KEY_UINT16;
              break;
            case 4 :
              offset = KEY_UINT32;
              break;
            case 8 :
              offset = KEY_UINT64;
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "[table][sort] unsupported uint value");
              goto exit;
            }
            break;
          case GRN_OBJ_KEY_INT :
            switch (GRN_TYPE_SIZE(DB_OBJ(range))) {
            case 1 :
              offset = KEY_INT8;
              break;
            case 2 :
              offset = KEY_INT16;
              break;
            case 4 :
              offset = KEY_INT32;
              break;
            case 8 :
              offset = KEY_INT64;
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "[table][sort] unsupported int value");
              goto exit;
            }
            break;
          case GRN_OBJ_KEY_FLOAT :
            switch (GRN_TYPE_SIZE(DB_OBJ(range))) {
            case 4 :
              offset = KEY_FLOAT32;
              break;
            case 8 :
              offset = KEY_FLOAT64;
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "[table][sort] unsupported float value");
              goto exit;
            }
            break;
          }
        }
      } else {
        switch (key->header.type) {
        case GRN_ACCESSOR :
        {
          grn_accessor *accessor = (grn_accessor *)key;
          while (accessor->next) {
            accessor = accessor->next;
          }
          if (accessor->action == GRN_ACCESSOR_GET_COLUMN_VALUE &&
              accessor->obj->header.type == GRN_COLUMN_INDEX) {
            can_refer = false;
          }
        }
        break;
        case GRN_COLUMN_INDEX :
          can_refer = false;
          break;
        default :
          break;
        }
        offset = KEY_UINT32;
      }
      grn_obj_unref(ctx, range);
    }

    keys[i].can_refer = can_refer;
    if (keys[i].can_refer) {
      keys[i].index_refer = index_refer;
      keys[i].get_value = sort_value_get_refer;
      index_refer++;
    } else {
      keys[i].index_value = index_value;
      keys[i].get_value = sort_value_get_value;
      index_value++;
    }
    keys[i].offset = offset;
    keys[i].column_cache = grn_column_cache_open(ctx, key);
  }
  if (index_value == 0) {
    keys[0].get_value = NULL;
  }
  {
    sort_data data = {0};
    data.offset = offset;
    data.limit = limit;
    data.result = result;
    data.keys = keys;
    data.n_keys = n_keys;
    n_sorted_records = grn_table_sort_value_body(ctx, table, &data);
  }
exit :
  if (keys) {
    for (i = 0; i < n_keys; i++) {
      if (keys[i].column_cache) {
        grn_column_cache_close(ctx, keys[i].column_cache);
      }
    }
    GRN_FREE(keys);
  }
  return n_sorted_records;
}

int
grn_table_sort(grn_ctx *ctx, grn_obj *table, int offset, int limit,
               grn_obj *result, grn_table_sort_key *keys, int n_keys)
{
  grn_rc rc;
  int n;
  int n_sorted_records = 0;
  GRN_API_ENTER;
  if (!n_keys || !keys) {
    WARN(GRN_INVALID_ARGUMENT, "keys is null");
    goto exit;
  }
  if (!table) {
    WARN(GRN_INVALID_ARGUMENT, "table is null");
    goto exit;
  }
  if (!(result && result->header.type == GRN_TABLE_NO_KEY)) {
    WARN(GRN_INVALID_ARGUMENT, "result is not a array");
    goto exit;
  }
  n = grn_table_size(ctx, table);
  if ((rc = grn_output_range_normalize(ctx, n, &offset, &limit))) {
    ERR(rc, "grn_output_range_normalize failed");
    goto exit;
  }
  if (keys->flags & GRN_TABLE_SORT_GEO) {
    if (n_keys == 2) {
      n_sorted_records = grn_geo_table_sort(ctx,
                                            table,
                                            offset,
                                            limit,
                                            result,
                                            keys[0].key,
                                            keys[1].key);
    } else {
      n_sorted_records = 0;
    }
    goto exit;
  }
  if (n_keys == 1 && !GRN_ACCESSORP(keys->key)) {
    grn_obj *index = 0;
    int n_indexes = grn_column_index(ctx,
                                     keys->key,
                                     GRN_OP_LESS,
                                     &index,
                                     1,
                                     NULL);
    if (n_indexes > 0) {
      n_sorted_records = grn_table_sort_index(ctx,
                                              table,
                                              index,
                                              offset,
                                              limit,
                                              result,
                                              keys,
                                              n_keys);
      grn_obj_unref(ctx, index);
      goto exit;
    }
  }
  if (offset + limit == 0) {
    n_sorted_records = 0;
  } else {
    n_sorted_records = grn_table_sort_value(ctx,
                                            table,
                                            offset,
                                            limit,
                                            result,
                                            keys,
                                            n_keys);
  }
exit :
  GRN_API_RETURN(n_sorted_records);
}
