/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

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
#include "groonga_in.h"
#include <string.h>
#include <limits.h>
#include "pat.h"

#define GRN_PAT_DELETED (GRN_ID_MAX + 1)

#define GRN_PAT_SEGMENT_SIZE 0x400000
#define W_OF_KEY_IN_A_SEGMENT 22
#define W_OF_PAT_IN_A_SEGMENT 18
#define W_OF_SIS_IN_A_SEGMENT 19
#define KEY_MASK_IN_A_SEGMENT 0x3fffff
#define PAT_MASK_IN_A_SEGMENT 0x3ffff
#define SIS_MASK_IN_A_SEGMENT 0x7ffff
#define SEG_NOT_ASSIGNED 0xffff
#define GRN_PAT_MAX_SEGMENT 0x1000
#define GRN_PAT_MDELINFOS (GRN_PAT_NDELINFOS - 1)

#define GRN_PAT_BIN_KEY 0x70000

typedef struct {
  grn_id lr[2];
  uint32_t key;
  uint16_t check;
  uint16_t bits;
  /* length : 13, deleting : 1, immediate : 1 */
} pat_node;

#define PAT_DELETING  (1<<1)
#define PAT_IMMEDIATE (1<<2)

#define PAT_DEL(x) ((x)->bits & PAT_DELETING)
#define PAT_IMD(x) ((x)->bits & PAT_IMMEDIATE)
#define PAT_LEN(x) (((x)->bits >> 3) + 1)
#define PAT_CHK(x) ((x)->check)
#define PAT_DEL_ON(x) ((x)->bits |= PAT_DELETING)
#define PAT_IMD_ON(x) ((x)->bits |= PAT_IMMEDIATE)
#define PAT_DEL_OFF(x) ((x)->bits &= ~PAT_DELETING)
#define PAT_IMD_OFF(x) ((x)->bits &= ~PAT_IMMEDIATE)
#define PAT_LEN_SET(x,v) ((x)->bits = ((x)->bits & ((1<<3) - 1))|(((v) - 1) << 3))
#define PAT_CHK_SET(x,v) ((x)->check = (v))

typedef struct {
  grn_id children;
  grn_id sibling;
} sis_node;

enum {
  segment_key = 0,
  segment_pat = 1,
  segment_sis = 2
};

/* bit operation */

#define nth_bit(key,n,l) ((((key)[(n)>>4]) >> (7 - (((n)>>1) & 7))) & 1)

/* segment operation */

/* patricia array operation */

#define PAT_AT(pat,id,n) {\
  int flags = 0;\
  GRN_IO_ARRAY_AT(pat->io, segment_pat, id, &flags, n);\
}

inline static pat_node *
pat_get(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  pat_node *res;
  int flags = GRN_TABLE_ADD;
  if (id > GRN_ID_MAX) { return NULL; }
  GRN_IO_ARRAY_AT(pat->io, segment_pat, id, &flags, res);
  return res;
}

inline static pat_node *
pat_node_new(grn_ctx *ctx, grn_pat *pat, grn_id *id)
{
  uint32_t n = pat->header->curr_rec + 1;
  pat_node *res;
  if (n > GRN_ID_MAX) { return NULL; }
  if ((res = pat_get(ctx, pat, n))) {
    pat->header->curr_rec = n;
    pat->header->n_entries++;
  }
  if (id) { *id = n; }
  return res;
}

/* sis operation */

inline static sis_node *
sis_at(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  sis_node *res;
  int flags = 0;
  if (id > GRN_ID_MAX) { return NULL; }
  GRN_IO_ARRAY_AT(pat->io, segment_sis, id, &flags, res);
  return res;
}

inline static sis_node *
sis_get(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
  sis_node *res;
  int flags = GRN_TABLE_ADD;
  if (id > GRN_ID_MAX) { return NULL; }
  GRN_IO_ARRAY_AT(pat->io, segment_sis, id, &flags, res);
  return res;
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
      if (grn_hash_add(ctx, h, &sid, sizeof(grn_id), (void **) &offset, NULL)) {
        *offset = level;
        if (level < MAX_LEVEL) { sis_collect(ctx, pat, h, sid, level + 1); }
        if (!(sl = sis_at(ctx, pat, sid))) { break; }
        sid = sl->sibling;
      } else {
        /* todo : must be handled */
      }
    }
  }
}

/* key operation */

#define KEY_AT(pat,pos,ptr,addp)\
{\
  int flags = addp;\
  GRN_IO_ARRAY_AT(pat->io, segment_key, pos, &flags, ptr);\
}

inline static uint32_t
key_put(grn_ctx *ctx, grn_pat *pat, const uint8_t *key, int len)
{
  uint32_t res, ts;
//  if (len >= GRN_PAT_SEGMENT_SIZE) { return 0; /* error */ }
  res = pat->header->curr_key;
  ts = (res + len) >> W_OF_KEY_IN_A_SEGMENT;
  if (res >> W_OF_KEY_IN_A_SEGMENT != ts) {
    res = pat->header->curr_key = ts << W_OF_KEY_IN_A_SEGMENT;
  }
  {
    uint8_t *dest;
    KEY_AT(pat, res, dest, GRN_TABLE_ADD);
    if (!dest) { return 0; }
    memcpy(dest, key, len);
  }
  pat->header->curr_key += len;
  return res;
}

inline static uint8_t *
pat_node_get_key(grn_ctx *ctx, grn_pat *pat, pat_node *n)
{
  if (PAT_IMD(n)) {
    return (uint8_t *) &n->key;
  } else {
    uint8_t *res;
    KEY_AT(pat, n->key, res, 0);
    return res;
  }
}

inline static grn_rc
pat_node_set_key(grn_ctx *ctx, grn_pat *pat, pat_node *n, const uint8_t *key, unsigned int len)
{
  if (!key || !len) { return GRN_INVALID_ARGUMENT; }
  PAT_LEN_SET(n, len);
  if (len <= sizeof(uint32_t)) {
    PAT_IMD_ON(n);
    memcpy(&n->key, key, len);
  } else {
    PAT_IMD_OFF(n);
    n->key = key_put(ctx, pat, key, len);
  }
  return GRN_SUCCESS;
}

/* delinfo operation */

enum {
  DL_EMPTY = 0,
  DL_PHASE1,
  DL_PHASE2
};

inline static grn_pat_delinfo *
delinfo_search(grn_pat *pat, grn_id id)
{
  int i;
  grn_pat_delinfo *di;
  for (i = (pat->header->curr_del2) & GRN_PAT_MDELINFOS;
       i != pat->header->curr_del;
       i = (i + 1) & GRN_PAT_MDELINFOS) {
    di = &pat->header->delinfos[i];
    if (di->stat != DL_PHASE1) { continue; }
    if (di->ld == id) { return di; }
    if (di->d == id) { return di; }
  }
  return NULL;
}

inline static grn_rc
delinfo_turn_2(grn_ctx *ctx, grn_pat *pat, grn_pat_delinfo *di)
{
  grn_id d, *p = NULL;
  pat_node *ln, *dn;
  // grn_log("delinfo_turn_2> di->d=%d di->ld=%d stat=%d", di->d, di->ld, di->stat);
  if (di->stat != DL_PHASE1) { return GRN_SUCCESS; }
  PAT_AT(pat, di->ld, ln);
  if (!ln) { return GRN_INVALID_ARGUMENT; }
  if (!(d = di->d)) { return GRN_INVALID_ARGUMENT; }
  PAT_AT(pat, d, dn);
  if (!dn) { return GRN_INVALID_ARGUMENT; }
  PAT_DEL_OFF(ln);
  PAT_DEL_OFF(dn);
  {
    grn_id r, *p0;
    pat_node *rn;
    int c0 = -1, c;
    uint32_t len = PAT_LEN(dn) * 16;
    const uint8_t *key = pat_node_get_key(ctx, pat, dn);
    if (!key) { return GRN_INVALID_ARGUMENT; }
    PAT_AT(pat, 0, rn);
    p0 = &rn->lr[1];
    while ((r = *p0)) {
      if (r == d) {
        p = p0;
        break;
      }
      PAT_AT(pat, r, rn);
      if (!rn) { return GRN_FILE_CORRUPT; }
      c = PAT_CHK(rn);
      if (c <= c0 || len <= c) { break; }
      if (c & 1) {
        p0 = (c + 1 < len) ? &rn->lr[1] : &rn->lr[0];
      } else {
        p0 = &rn->lr[nth_bit((uint8_t *)key, c, len)];
      }
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
    int j;
    grn_id dd;
    grn_pat_delinfo *ddi;
    GRN_LOG(ctx, GRN_LOG_DEBUG, "failed to find d=%d", d);
    for (j = (pat->header->curr_del2 + 1) & GRN_PAT_MDELINFOS;
         j != pat->header->curr_del;
         j = (j + 1) & GRN_PAT_MDELINFOS) {
      ddi = &pat->header->delinfos[j];
      if (ddi->stat != DL_PHASE1) { continue; }
      PAT_AT(pat, ddi->ld, ln);
      if (!ln) { continue; }
      if (!(dd = ddi->d)) { continue; }
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

inline static grn_rc
delinfo_turn_3(grn_ctx *ctx, grn_pat *pat, grn_pat_delinfo *di)
{
  pat_node *dn;
  uint32_t size;
  if (di->stat != DL_PHASE2) { return GRN_SUCCESS; }
  PAT_AT(pat, di->d, dn);
  if (!dn) { return GRN_INVALID_ARGUMENT; }
  if (di->shared) {
    PAT_IMD_ON(dn);
    size = 0;
  } else {
    if (PAT_IMD(dn)) {
      size = 0;
    } else {
      size = PAT_LEN(dn);
    }
  }
  di->stat = DL_EMPTY;
  //  dn->lr[1] = GRN_PAT_DELETED;
  dn->lr[0] = pat->header->garbages[size];
  pat->header->garbages[size] = di->d;
  return GRN_SUCCESS;
}

inline static grn_pat_delinfo *
delinfo_new(grn_ctx *ctx, grn_pat *pat)
{
  grn_pat_delinfo *res = &pat->header->delinfos[pat->header->curr_del];
  uint32_t n = (pat->header->curr_del + 1) & GRN_PAT_MDELINFOS;
  int gap = ((n + GRN_PAT_NDELINFOS - pat->header->curr_del2) & GRN_PAT_MDELINFOS)
            - (GRN_PAT_NDELINFOS / 2);
  while (gap-- > 0) {
    if (delinfo_turn_2(ctx, pat, &pat->header->delinfos[pat->header->curr_del2])) {
      GRN_LOG(ctx, GRN_LOG_CRIT, "d2 failed: %d", pat->header->delinfos[pat->header->curr_del2].ld);
    }
    pat->header->curr_del2 = (pat->header->curr_del2 + 1) & GRN_PAT_MDELINFOS;
  }
  if (n == pat->header->curr_del3) {
    if (delinfo_turn_3(ctx, pat, &pat->header->delinfos[pat->header->curr_del3])) {
      GRN_LOG(ctx, GRN_LOG_CRIT, "d3 failed: %d", pat->header->delinfos[pat->header->curr_del3].ld);
    }
    pat->header->curr_del3 = (pat->header->curr_del3 + 1) & GRN_PAT_MDELINFOS;
  }
  pat->header->curr_del = n;
  return res;
}

/* pat operation */

inline static grn_pat *
_grn_pat_create(grn_ctx *ctx, grn_pat *pat,
                const char *path, uint32_t key_size,
                uint32_t value_size, uint32_t flags) {
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
  for (w_of_element = 0; (1 << w_of_element) < entry_size; w_of_element++);
  {
    grn_io_array_spec array_spec[3];
    array_spec[segment_key].w_of_element = 0;
    array_spec[segment_key].max_n_segments = 0x400;
    array_spec[segment_pat].w_of_element = 4;
    array_spec[segment_pat].max_n_segments = 1 << (30 - (22 - 4));
    array_spec[segment_sis].w_of_element = w_of_element;
    array_spec[segment_sis].max_n_segments = 1 << (30 - (22 - w_of_element));
    io = grn_io_create_with_array(ctx, path, sizeof(struct grn_pat_header),
                                  GRN_PAT_SEGMENT_SIZE, grn_io_auto, 3, array_spec);
  }
  if (!io) { return NULL; }
  if (encoding == GRN_ENC_DEFAULT) { encoding = grn_gctx.encoding; }
  header = grn_io_header(io);
  grn_io_set_type(io, GRN_TABLE_PAT_KEY);
  header->flags = flags;
  header->encoding = encoding;
  header->key_size = key_size;
  header->value_size = value_size;
  header->n_entries = 0;
  header->curr_rec = 0;
  header->curr_key = -1;
  header->curr_del = 0;
  header->curr_del2 = 0;
  header->curr_del3 = 0;
  header->n_garbages = 0;
  header->tokenizer = 0;
  GRN_DB_OBJ_SET_TYPE(pat, GRN_TABLE_PAT_KEY);
  pat->io = io;
  pat->header = header;
  pat->key_size = key_size;
  pat->value_size = value_size;
  pat->tokenizer = grn_ctx_at(ctx, header->tokenizer);
  pat->encoding = encoding;
  pat->obj.header.flags = flags;
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
grn_pat_create(grn_ctx *ctx, const char *path, uint32_t key_size,
               uint32_t value_size, uint32_t flags)
{
  grn_pat *pat;
  if (!(pat = GRN_MALLOC(sizeof(grn_pat)))) {
    return NULL;
  }
  if (!_grn_pat_create(ctx, pat, path, key_size, value_size, flags)) {
    GRN_FREE(pat);
    return NULL;
  }
  return pat;
}

grn_pat *
grn_pat_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  grn_pat *pat;
  pat_node *node0;
  struct grn_pat_header *header;
  io = grn_io_open(ctx, path, grn_io_auto);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  if (grn_io_get_type(io) != GRN_TABLE_PAT_KEY) {
    ERR(GRN_INVALID_FORMAT, "file type unmatch");
    grn_io_close(ctx, io);
    return NULL;
  }
  if (!(pat = GRN_MALLOC(sizeof(grn_pat)))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(pat, GRN_TABLE_PAT_KEY);
  pat->io = io;
  pat->header = header;
  pat->key_size = header->key_size;
  pat->value_size = header->value_size;
  pat->encoding = header->encoding;
  pat->obj.header.flags = header->flags;
  pat->tokenizer = grn_ctx_at(ctx, header->tokenizer);
  PAT_AT(pat, 0, node0);
  if (!node0) {
    grn_io_close(ctx, io);
    GRN_GFREE(pat);
    return NULL;
  }
  return pat;
}

grn_rc
grn_pat_close(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc;
  if ((rc = grn_io_close(ctx, pat->io))) {
    ERR(rc, "grn_io_close failed");
  } else {
    GRN_FREE(pat);
  }
  return rc;
}

grn_rc
grn_pat_remove(grn_ctx *ctx, const char *path)
{
  if (!path) {
    ERR(GRN_INVALID_ARGUMENT, "path is null");
    return GRN_INVALID_ARGUMENT;
  }
  return grn_io_remove(ctx, path);
}

grn_rc
grn_pat_truncate(grn_ctx *ctx, grn_pat *pat)
{
  grn_rc rc;
  char *path;
  uint32_t key_size, value_size, flags;

  if ((path = (char *)grn_io_path(pat->io)) && *path != '\0') {
    if (!(path = GRN_STRDUP(path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path.");
      return GRN_NO_MEMORY_AVAILABLE;
    }
  } else {
    path = NULL;
  }
  key_size = pat->key_size;
  value_size = pat->value_size;
  flags = pat->obj.header.flags;

  if ((rc = grn_io_close(ctx, pat->io))) { goto exit; }
  pat->io = NULL;
  if (path && (rc = grn_io_remove(ctx, path))) { goto exit; }
  if (!_grn_pat_create(ctx, pat, path, key_size, value_size, flags)) {
    rc = GRN_UNKNOWN_ERROR;
  }
exit:
  if (path) { GRN_FREE(path); }
  return rc;
}

inline static grn_id
_grn_pat_add(grn_ctx *ctx, grn_pat *pat, const uint8_t *key, uint32_t size, uint32_t *new, uint32_t *lkey)
{
  grn_id r, r0, *p0, *p1 = NULL;
  pat_node *rn, *rn0;
  int c, c0 = -1, c1 = -1, len;
  *new = 0;
  len = (int)size * 16;
  PAT_AT(pat, 0, rn0);
  p0 = &rn0->lr[1];
  if (*p0) {
    uint32_t size2;
    int xor, mask;
    const uint8_t *s, *d;
    for (;;) {
      if (!(r0 = *p0)) {
        if (!(s = pat_node_get_key(ctx, pat, rn0))) { return 0; }
        size2 = PAT_LEN(rn0);
        break;
      }
      PAT_AT(pat, r0, rn0);
      if (!rn0) { return GRN_ID_NIL; }
      if (c0 < rn0->check && rn0->check < len) {
        c1 = c0; c0 = rn0->check;
        p1 = p0;
        if (c0 & 1) {
          p0 = (c0 + 1 < len) ? &rn0->lr[1] : &rn0->lr[0];
        } else {
          p0 = &rn0->lr[nth_bit(key, c0, len)];
        }
      } else {
        if (!(s = pat_node_get_key(ctx, pat, rn0))) { return 0; }
        size2 = PAT_LEN(rn0);
        if (size == size2 && !memcmp(s, key, size)) { return r0; }
        break;
      }
    }
    {
      uint32_t min = size > size2 ? size2 : size;
      for (c = 0, d = key; min && *s == *d; c += 16, s++, d++, min--);
      if (min) {
        for (xor = *s ^ *d, mask = 0x80; !(xor & mask); mask >>= 1, c += 2);
      } else {
        c--;
      }
    }
    if (c == c0 && !*p0) {
      if (c < len - 2) { c += 2; }
    } else {
      if (c < c0) {
        if (c > c1) {
          p0 = p1;
        } else {
          PAT_AT(pat, 0, rn0);
          p0 = &rn0->lr[1];
          while ((r0 = *p0)) {
            PAT_AT(pat, r0, rn0);
            if (!rn0) { return 0; }
            c0 = PAT_CHK(rn0);
            if (c < c0) { break; }
            if (c0 & 1) {
              p0 = (c0 + 1 < len) ? &rn0->lr[1] : &rn0->lr[0];
            } else {
              p0 = &rn0->lr[nth_bit(key, c0, len)];
            }
          }
        }
      }
    }
    if (c >= len) { return 0; }
  } else {
    c = len - 2;
  }
  {
    uint32_t size2 = size > sizeof(uint32_t) ? size : 0;
    if (*lkey && size2) {
      if (pat->header->garbages[0]) {
        r = pat->header->garbages[0];
        pat->header->n_entries++;
        pat->header->n_garbages--;
        PAT_AT(pat, r, rn);
        if (!rn) { return 0; }
        pat->header->garbages[0] = rn->lr[0];
      } else {
        if (!(rn = pat_node_new(ctx, pat, &r))) { return 0; }
      }
      PAT_IMD_OFF(rn);
      PAT_LEN_SET(rn, size);
      rn->key = *lkey;
    } else {
      if (pat->header->garbages[size2]) {
        uint8_t *keybuf;
        r = pat->header->garbages[size2];
        pat->header->n_entries++;
        pat->header->n_garbages--;
        PAT_AT(pat, r, rn);
        if (!rn) { return 0; }
        pat->header->garbages[size2] = rn->lr[0];
        if (!(keybuf = pat_node_get_key(ctx, pat, rn))) { return 0; }
        PAT_LEN_SET(rn, size);
        memcpy(keybuf, key, size);
      } else {
        if (!(rn = pat_node_new(ctx, pat, &r))) { return 0; }
        pat_node_set_key(ctx, pat, rn, key, size);
      }
      *lkey = rn->key;
    }
  }
  PAT_CHK_SET(rn, c);
  PAT_DEL_OFF(rn);
  if ((c & 1) ? (c + 1 < len) : nth_bit(key, c, len)) {
    rn->lr[1] = r;
    rn->lr[0] = *p0;
  } else {
    rn->lr[1] = *p0;
    rn->lr[0] = r;
  }
  // smp_wmb();
  *p0 = r;
  *new = 1;
  return r;
}

inline static int
chop(grn_ctx *ctx, grn_pat *pat, const char **key, const char *end, uint32_t *lkey)
{
  size_t len = grn_charlen(ctx, *key, end);
  if (len) {
    *lkey += len;
    *key += len;
    return **key;
  } else {
    return 0;
  }
}

static void
grn_gton(uint8_t *keybuf, const void *key, uint32_t size)
{
  int la = ((grn_geo_point *)key)->latitude;
  int lo = ((grn_geo_point *)key)->longitude;
  uint8_t *p = keybuf;
  int i = 32;
  while (i) {
    i -= 4;
    *p++ = ((((la >> i) & 8) << 4) + (((lo >> i) & 8) << 3) +
            (((la >> i) & 4) << 3) + (((lo >> i) & 4) << 2) +
            (((la >> i) & 2) << 2) + (((lo >> i) & 2) << 1) +
            (((la >> i) & 1) << 1) + (((lo >> i) & 1) << 0));
  }
}

static void
grn_ntog(uint8_t *keybuf, uint8_t *key, uint32_t size)
{
  int la = 0, lo = 0;
  uint8_t v, *p = key;
  int i = 32;
  while (size--) {
    i -= 4;
    v = *p++;
    la += (((v & 128) >> 4) + ((v &  32) >> 3) +
           ((v &   8) >> 2) + ((v &   2) >> 1)) << i;
    lo += (((v &  64) >> 3) + ((v &  16) >> 2) +
           ((v &   4) >> 1) + ((v &   1) >> 0)) << i;
  }
  ((grn_geo_point *)keybuf)->latitude = la;
  ((grn_geo_point *)keybuf)->longitude = lo;
}

#define MAX_FIXED_KEY_SIZE (sizeof(int64_t))

#define KEY_NEEDS_CONVERT(pat,size) \
  (!((pat)->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) && (size) <= MAX_FIXED_KEY_SIZE)

#define KEY_ENC(pat,keybuf,key,size) {\
  switch ((pat)->obj.header.flags & GRN_OBJ_KEY_MASK) {\
  case GRN_OBJ_KEY_UINT :\
    if (((pat)->obj.header.domain != GRN_DB_TOKYO_GEO_POINT) &&\
        ((pat)->obj.header.domain != GRN_DB_WGS84_GEO_POINT)) {\
      grn_hton((keybuf), (key), (size));\
      break;\
    }\
  case GRN_OBJ_KEY_GEO_POINT :\
    grn_gton((keybuf), (key), (size));\
    break;\
  case GRN_OBJ_KEY_INT :\
    grn_hton((keybuf), (key), (size));\
    *((uint8_t *)(keybuf)) ^= 0x80;\
    break;\
  case GRN_OBJ_KEY_FLOAT :\
    if ((size) == sizeof(int64_t)) {\
      int64_t v = *(int64_t *)(key);\
      v ^= ((v >> 63)|(1LL << 63));\
      grn_hton((keybuf), &v, (size));\
    }\
    break;\
  }\
}

#define KEY_DEC(pat,keybuf,key,size) {\
  switch ((pat)->obj.header.flags & GRN_OBJ_KEY_MASK) {\
  case GRN_OBJ_KEY_UINT :\
    if (((pat)->obj.header.domain != GRN_DB_TOKYO_GEO_POINT) &&\
        ((pat)->obj.header.domain != GRN_DB_WGS84_GEO_POINT)) {\
      grn_ntoh((keybuf), (key), (size));\
      break;\
    }\
  case GRN_OBJ_KEY_GEO_POINT :\
    grn_ntog((keybuf), (key), (size));\
    break;\
  case GRN_OBJ_KEY_INT :\
    grn_ntohi((keybuf), (key), (size));\
    break;\
  case GRN_OBJ_KEY_FLOAT :\
    if ((size) == sizeof(int64_t)) {\
      int64_t v;\
      grn_hton(&v, (key), (size));\
      *((int64_t *)(keybuf)) = v ^ (((v^(1LL<<63))>> 63)|(1LL<<63));  \
    }\
    break;\
  }\
}

#define KEY_ENCODE(pat,keybuf,key,size) \
if (KEY_NEEDS_CONVERT(pat,size)) {\
  KEY_ENC((pat), (keybuf), (key), (size));\
  (key) = (keybuf);\
}

grn_id
grn_pat_add(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size,
            void **value, int *added)
{
  uint32_t new, lkey = 0;
  grn_id r0;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  if (!key || !key_size) { return GRN_ID_NIL; }
  if (key_size > GRN_TABLE_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT, "too long key");
    return GRN_ID_NIL;
  }
  KEY_ENCODE(pat, keybuf, key, key_size);
  r0 = _grn_pat_add(ctx, pat, (uint8_t *)key, key_size, &new, &lkey);
  if (added) { *added = new; }
  if (r0 && (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) &&
      (*((uint8_t *)key) & 0x80)) { // todo: refine!!
    sis_node *sl, *sr;
    grn_id l = r0, r;
    if (new && (sl = sis_get(ctx, pat, l))) {
      const char *sis = key, *end = sis + key_size;
      sl->children = l;
      sl->sibling = 0;
      while (chop(ctx, pat, &sis, end, &lkey)) {
        if (!(*sis & 0x80)) { break; }
        if (!(r = _grn_pat_add(ctx, pat, (uint8_t *)sis, end - sis, &new, &lkey))) {
          break;
        }
        if (!(sr = sis_get(ctx, pat, r))) { break; }
        if (new) {
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

grn_id
grn_pat_get(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size, void **value)
{
  grn_id r;
  pat_node *rn;
  int c0 = -1, c;
  uint32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, rn);
  for (r = rn->lr[1]; r;) {
    PAT_AT(pat, r, rn);
    if (!rn) { break; /* corrupt? */ }
    c = PAT_CHK(rn);
    if (len <= c) { break; }
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
    if (c & 1) {
      r = (c + 1 < len) ? rn->lr[1] : rn->lr[0];
    } else {
      r = rn->lr[nth_bit((uint8_t *)key, c, len)];
    }
    c0 = c;
  }
  return GRN_ID_NIL;
}

grn_id
grn_pat_nextid(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size)
{
  grn_id r = GRN_ID_NIL;
  if (pat && key) {
    if (!(r = pat->header->garbages[key_size > sizeof(uint32_t) ? key_size : 0])) {
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
grn_pat_prefix_search(grn_ctx *ctx, grn_pat *pat,
                      const void *key, uint32_t key_size, grn_hash *h)
{
  int c0 = -1, c;
  const uint8_t *k;
  uint32_t len = key_size * 16;
  grn_id r;
  pat_node *rn;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, rn);
  r = rn->lr[1];
  while (r) {
    PAT_AT(pat, r, rn);
    if (!rn) { return GRN_FILE_CORRUPT; }
    c = PAT_CHK(rn);
    if (c0 < c && c < len - 1) {
      if (c & 1) {
        r = (c + 1 < len) ? rn->lr[1] : rn->lr[0];
      } else {
        r = rn->lr[nth_bit((uint8_t *)key, c, len)];
      }
      c0 = c;
      continue;
    }
    if (!(k = pat_node_get_key(ctx, pat, rn))) { break; }
    if (PAT_LEN(rn) < key_size) { break; }
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
grn_pat_prefix_search2(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size)
{
  grn_hash *h;
  if (!pat || !key) { return NULL; }
  if ((h = grn_hash_create(ctx, NULL, sizeof(grn_id), 0, 0))) {
    if (grn_pat_prefix_search(ctx, pat, key, key_size, h)) {
      grn_hash_close(ctx, h);
      h = NULL;
    }
  }
  return h;
}

grn_rc
grn_pat_suffix_search(grn_ctx *ctx, grn_pat *pat,
                      const void *key, uint32_t key_size, grn_hash *h)
{
  grn_id r;
  if ((r = grn_pat_get(ctx, pat, key, key_size, NULL))) {
    uint32_t *offset;
    if (grn_hash_add(ctx, h, &r, sizeof(grn_id), (void **) &offset, NULL)) {
      *offset = 0;
      if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) { sis_collect(ctx, pat, h, r, 1); }
      return GRN_SUCCESS;
    }
  }
  return GRN_END_OF_DATA;
}

grn_hash *
grn_pat_suffix_search2(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size)
{
  grn_hash *h;
  if (!pat || !key) { return NULL; }
  if ((h = grn_hash_create(ctx, NULL, sizeof(grn_id), sizeof(uint32_t), 0))) {
    if (grn_pat_suffix_search(ctx, pat, key, key_size, h)) {
      grn_hash_close(ctx, h);
      h = NULL;
    }
  }
  return h;
}

grn_id
grn_pat_lcp_search(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size)
{
  pat_node *rn;
  grn_id r, r2 = GRN_ID_NIL;
  uint32_t len = key_size * 16;
  int c0 = -1, c;
  if (!pat || !key || !(pat->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)) { return GRN_ID_NIL; }
  PAT_AT(pat, 0, rn);
  for (r = rn->lr[1]; r;) {
    PAT_AT(pat, r, rn);
    if (!rn) { break; /* corrupt? */ }
    c = PAT_CHK(rn);
    if (c <= c0) {
      if (PAT_LEN(rn) <= key_size) {
        uint8_t *p = pat_node_get_key(ctx, pat, rn);
        if (!p) { break; }
        if (!memcmp(p, key, PAT_LEN(rn))) { return r; }
      }
      break;
    }
    if (len <= c) { break; }
    if (c & 1) {
      uint8_t *p;
      pat_node *rn0;
      grn_id r0 = rn->lr[0];
      PAT_AT(pat, r0, rn0);
      if (!rn0) { break; /* corrupt? */ }
      p = pat_node_get_key(ctx, pat, rn0);
      if (!p) { break; }
      if (PAT_LEN(rn0) <= key_size && !memcmp(p, key, PAT_LEN(rn0))) { r2 = r0; }
      r = (c + 1 < len) ? rn->lr[1] : rn->lr[0];
    } else {
      r = rn->lr[nth_bit((uint8_t *)key, c, len)];
    }
    c0 = c;
  }
  return r2;
}

inline static grn_rc
__grn_pat_del(grn_ctx *ctx, grn_pat *pat, const char *key, uint32_t key_size, int shared,
             grn_table_delete_optarg *optarg)
{
  grn_pat_delinfo *di;
  uint8_t direction;
  pat_node *rn, *rn0 = NULL, *rno;
  int c, c0 = -1, ch;
  uint32_t len = key_size * 16;
  grn_id r, otherside, *p, *p0 = NULL;
  di = delinfo_new(ctx, pat); /* must be called before find rn */
  di->shared = shared;
  PAT_AT(pat, 0, rn);
  c = -1;
  p = &rn->lr[1];
  for (;;) {
    if (!(r = *p)) { return GRN_INVALID_ARGUMENT; }
    PAT_AT(pat, r, rn);
    if (!rn) { return GRN_FILE_CORRUPT; }
    ch = PAT_CHK(rn);
    if (len <= ch) { return GRN_INVALID_ARGUMENT; }
    if (c >= ch) {
      const uint8_t *k = pat_node_get_key(ctx, pat, rn);
      if (!k) { return GRN_INVALID_ARGUMENT; }
      if (key_size == PAT_LEN(rn) && !memcmp(k, key, key_size)) {
        break; /* found */
      } else {  return GRN_INVALID_ARGUMENT; }
    }
    c0 = c;
    p0 = p;
    if ((c = ch) & 1) {
      p = (c + 1 < len) ? &rn->lr[1] : &rn->lr[0];
    } else {
      p = &rn->lr[nth_bit((uint8_t *)key, c, len)];
    }
    rn0 = rn;
  }
  if (optarg && optarg->func &&
      !optarg->func(ctx, (grn_obj *)pat, r, optarg->func_arg)) {
    return GRN_SUCCESS;
  }
  direction = (rn0->lr[1] == r);
  otherside = direction ? rn0->lr[0] : rn0->lr[1];
  if (rn == rn0) {
    di->stat = DL_PHASE2;
    di->d = r;
    if (otherside) {
      PAT_AT(pat, otherside, rno);
      if (rno && c0 < PAT_CHK(rno) && PAT_CHK(rno) <= c) {
        if (!delinfo_search(pat, otherside)) {
          GRN_LOG(ctx, GRN_LOG_ERROR, "no delinfo found %d", otherside);
        }
        PAT_CHK_SET(rno, 0);
      }
    }
    *p0 = otherside;
  } else {
    grn_pat_delinfo *ldi = NULL, *ddi = NULL;
    if (PAT_DEL(rn)) { ldi = delinfo_search(pat, r); }
    if (PAT_DEL(rn0)) { ddi = delinfo_search(pat, *p0); }
    if (ldi) {
      PAT_DEL_OFF(rn);
      di->stat = DL_PHASE2;
      if (ddi) {
        PAT_DEL_OFF(rn0);
        ddi->stat = DL_PHASE2;
        if (ddi == ldi) {
          if (r != ddi->ld) {
            GRN_LOG(ctx, GRN_LOG_ERROR, "r(%d) != ddi->ld(%d)", r, ddi->ld);
          }
          di->d = r;
        } else {
          ldi->ld = ddi->ld;
          di->d = r;
        }
      } else {
        PAT_DEL_ON(rn0);
        ldi->ld = *p0;
        di->d = r;
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
        di->d = r;
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
        di->d = r;
        // grn_log("pat_del d=%d ld=%d stat=%d", r, *p0, DL_PHASE1);
      }
    }
    if (*p0 == otherside) {
      PAT_CHK_SET(rn0, 0);
    } else {
      if (otherside) {
        PAT_AT(pat, otherside, rno);
        if (rno && c0 < PAT_CHK(rno) && PAT_CHK(rno) <= c) {
          if (!delinfo_search(pat, otherside)) {
            GRN_LOG(ctx, GRN_LOG_ERROR, "no delinfo found %d", otherside);
          }
          PAT_CHK_SET(rno, 0);
        }
      }
      *p0 = otherside;
    }
  }
  pat->header->n_entries--;
  pat->header->n_garbages++;
  return GRN_SUCCESS;
}

static grn_rc
_grn_pat_delete(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size,
                grn_table_delete_optarg *optarg)
{
  if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
    grn_id id = grn_pat_get(ctx, pat, key, key_size, NULL);
    if (id && grn_pat_delete_with_sis(ctx, pat, id, optarg)) {
      return GRN_SUCCESS;
    }
    return GRN_INVALID_ARGUMENT;
  }
  return __grn_pat_del(ctx, pat, key, key_size, 0, optarg);
}

grn_rc
grn_pat_delete(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size,
               grn_table_delete_optarg *optarg)
{
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  if (!pat || !key || !key_size) { return GRN_INVALID_ARGUMENT; }
  KEY_ENCODE(pat, keybuf, key, key_size);
  return _grn_pat_delete(ctx, pat, key, key_size, optarg);
}

uint32_t
grn_pat_size(grn_ctx *ctx, grn_pat *pat)
{
  if (!pat) { return GRN_INVALID_ARGUMENT; }
  return pat->header->n_entries;
}

const char *
_grn_pat_key(grn_ctx *ctx, grn_pat *pat, grn_id id, uint32_t *key_size)
{
  pat_node *node;
  PAT_AT(pat, id, node);
  if (!node) { return NULL; }
  *key_size = PAT_LEN(node);
  return (const char *) pat_node_get_key(ctx, pat, node);
}

grn_rc
grn_pat_delete_by_id(grn_ctx *ctx, grn_pat *pat, grn_id id,
                     grn_table_delete_optarg *optarg)
{
  if (!pat || !id) { return GRN_INVALID_ARGUMENT; }
  {
    uint32_t key_size;
    const char *key = _grn_pat_key(ctx, pat, id, &key_size);
    return _grn_pat_delete(ctx, pat, key, key_size, optarg);
  }
}

int
grn_pat_get_key(grn_ctx *ctx, grn_pat *pat, grn_id id, void *keybuf, int bufsize)
{
  int len;
  uint8_t *key;
  pat_node *node;
  if (!pat) { return GRN_INVALID_ARGUMENT; }
  PAT_AT(pat, id, node);
  if (!node) { return 0; }
  if (!(key = pat_node_get_key(ctx, pat, node))) { return 0; }
  len = PAT_LEN(node);
  if (keybuf && bufsize >= len) {
    if (KEY_NEEDS_CONVERT(pat, len)) {
      KEY_DEC(pat, keybuf, key, len);
    } else {
      memcpy(keybuf, key, len);
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
  if (!pat) { return GRN_INVALID_ARGUMENT; }
  PAT_AT(pat, id, node);
  if (!node) { return 0; }
  if (!(key = pat_node_get_key(ctx, pat, node))) { return 0; }
  len = PAT_LEN(node);
  if (KEY_NEEDS_CONVERT(pat, len)) {
    if (bulk->header.impl_flags & GRN_OBJ_REFER) {
      GRN_TEXT_INIT(bulk, 0);
    }
    if (!grn_bulk_reserve(ctx, bulk, len)) {
      char *curr = GRN_BULK_CURR(bulk);
      KEY_DEC(pat, curr, key, len);
      grn_bulk_truncate(ctx, bulk, GRN_BULK_VSIZE(bulk) + len);
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
  int value_size = (int)pat->value_size;
  if (value_size) {
    byte *v = (byte *)sis_at(ctx, pat, id);
    if (v) {
      if (valuebuf) {
        if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) {
          memcpy(valuebuf, v + sizeof(sis_node), value_size);
        } else {
          memcpy(valuebuf, v, value_size);
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
  if ((*size = pat->value_size)) {
    if ((value = (const char *)sis_at(ctx, pat, id))
        && (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS)) {
      value += sizeof(sis_node);
    }
  }
  return value;
}

grn_rc
grn_pat_set_value(grn_ctx *ctx, grn_pat *pat, grn_id id, void *value, int flags)
{
  if (value) {
    uint32_t value_size = pat->value_size;
    if (value_size) {
      byte *v = (byte *)sis_get(ctx, pat, id);
      if (v) {
        if (pat->obj.header.flags & GRN_OBJ_KEY_WITH_SIS) { v += sizeof(sis_node); }
        switch ((flags & GRN_OBJ_SET_MASK)) {
        case GRN_OBJ_SET :
          memcpy(v, value, value_size);
          return GRN_SUCCESS;
        case GRN_OBJ_INCR :
          switch (value_size) {
          case sizeof(int32_t) :
            *((int32_t *)v) += *((int32_t *)value);
            return GRN_SUCCESS;
          case sizeof(int64_t) :
            *((int64_t *)v) += *((int64_t *)value);
            return GRN_SUCCESS;
          default :
            return GRN_INVALID_ARGUMENT;
          }
          break;
        case GRN_OBJ_DECR :
          switch (value_size) {
          case sizeof(int32_t) :
            *((int32_t *)v) -= *((int32_t *)value);
            return GRN_SUCCESS;
          case sizeof(int64_t) :
            *((int64_t *)v) -= *((int64_t *)value);
            return GRN_SUCCESS;
          default :
            return GRN_INVALID_ARGUMENT;
          }
          break;
        default :
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
grn_pat_info(grn_ctx *ctx, grn_pat *pat, int *key_size, unsigned *flags,
             grn_encoding *encoding, unsigned *n_entries, unsigned *file_size)
{
  ERRCLR(NULL);
  if (!pat) { return GRN_INVALID_ARGUMENT; }
  if (key_size) { *key_size = pat->key_size; }
  if (flags) { *flags = pat->obj.header.flags; }
  if (encoding) { *encoding = pat->encoding; }
  if (n_entries) { *n_entries = pat->header->n_entries; }
  if (file_size) {
    grn_rc rc;
    uint64_t tmp = 0;
    if ((rc = grn_io_size(ctx, pat->io, &tmp))) {
      return rc;
    }
    *file_size = (unsigned) tmp; /* FIXME: inappropriate cast */
  }
  return GRN_SUCCESS;
}

int
grn_pat_delete_with_sis(grn_ctx *ctx, grn_pat *pat, grn_id id,
                        grn_table_delete_optarg *optarg)
{
  int level = 0, shared;
  const char *key = NULL, *_key;
  sis_node *sp, *ss = NULL, *si = sis_at(ctx, pat, id);
  while (id) {
    pat_node *rn;
    uint32_t key_size;
    if ((si && si->children && si->children != id) ||
        (optarg && optarg->func &&
         !optarg->func(ctx, (grn_obj *)pat, id, optarg->func_arg))) {
      break;
    }
    PAT_AT(pat, id, rn);
    if (!(_key = (char *)pat_node_get_key(ctx, pat, rn))) { return 0; }
    if (_key == key) {
      shared = 1;
    } else {
      key = _key;
      shared = 0;
    }
    key_size = PAT_LEN(rn);
    if (key && key_size) { __grn_pat_del(ctx, pat, key, key_size, shared, NULL); }
    if (si) {
      grn_id *p, sid;
      uint32_t lkey = 0;
      if ((*key & 0x80) && chop(ctx, pat, &key, key + key_size, &lkey)) {
        if ((sid = grn_pat_get(ctx, pat, key, key_size - lkey, NULL)) &&
            (ss = sis_at(ctx, pat, sid))) {
          for (p = &ss->children; *p && *p != sid; p = &sp->sibling) {
            if (*p == id) {
              *p = si->sibling;
              break;
            }
            if (!(sp = sis_at(ctx, pat, *p))) { break; }
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
    uint32_t lkey = 0;
    while (id && key) {
      uint32_t key_size;
      if (_grn_pat_key(ctx, pat, id, &key_size) != key) { break; }
      {
        pat_node *rn;
        PAT_AT(pat, id, rn);
        if (!rn) { break; }
        if (lkey) {
          rn->key = lkey;
        } else {
          pat_node_set_key(ctx, pat, rn, (uint8_t *)key, key_size);
          lkey = rn->key;
        }
      }
      {
        const char *end = key + key_size;
        if (!((*key & 0x80) && chop(ctx, pat, &key, end, &lkey))) { break; }
        id = grn_pat_get(ctx, pat, key, end - key, NULL);
      }
    }
  }
  return level;
}

grn_id
grn_pat_next(grn_ctx *ctx, grn_pat *pat, grn_id id)
{
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
  if (id == grn_pat_get(ctx, pat, key, key_size, NULL)) { return id; }
  return GRN_ID_NIL;
}

grn_id
grn_pat_curr_id(grn_ctx *ctx, grn_pat *pat)
{
  return pat->header->curr_rec;
}

int
grn_pat_scan(grn_ctx *ctx, grn_pat *pat, const char *str, unsigned int str_len,
             grn_pat_scan_hit *sh, unsigned int sh_size, const char **rest)
{
  int n = 0;
  grn_id tid;
  if (pat->obj.header.flags & GRN_OBJ_KEY_NORMALIZE) {
    grn_str *nstr = grn_str_open(ctx, str, str_len, GRN_STR_NORMALIZE|GRN_STR_WITH_CHECKS);
    if (nstr) {
      int16_t *cp = nstr->checks;
      unsigned int offset = 0, offset0 = 0;
      const char *sp = nstr->norm, *se = nstr->norm + nstr->norm_blen;
      while (n < sh_size) {
        if ((tid = grn_pat_lcp_search(ctx, pat, sp, se - sp))) {
          uint32_t len;
          _grn_pat_key(ctx, pat, tid, &len);
          sh[n].id = tid;
          sh[n].offset = (*cp > 0) ? offset : offset0;
          while (len--) {
            if (*cp > 0) { offset0 = offset; offset += *cp; }
            sp++; cp++;
          }
          sh[n].length = offset - sh[n].offset;
          n++;
        } else {
          if (*cp > 0) { offset0 = offset; offset += *cp; }
          do {
            sp++; cp++;
          } while (sp < se && !*cp);
        }
        if (se <= sp) { offset = str_len; break; }
      }
      if (rest) { *rest = nstr->orig + offset; }
      grn_str_close(ctx, nstr);
    } else {
      n = -1;
      if (rest) { *rest = str; }
    }
  } else {
    uint32_t len;
    const char *sp, *se = str + str_len;
    for (sp = str; sp < se && n < sh_size; sp += len) {
      if ((tid = grn_pat_lcp_search(ctx, pat, sp, se - sp))) {
        _grn_pat_key(ctx, pat, tid, &len);
        sh[n].id = tid;
        sh[n].offset = sp - str;
        sh[n].length = len;
        n++;
      } else {
        len = grn_charlen(ctx, sp, se);
      }
      if (!len) { break; }
    }
    if (rest) { *rest = sp; }
  }
  return n;
}

#define INITIAL_SIZE 512

inline static void
push(grn_pat_cursor *c, grn_id id, uint16_t check)
{
  grn_ctx *ctx = c->ctx;
  grn_pat_cursor_entry *se;
  if (c->size <= c->sp) {
    if (c->ss) {
      uint32_t size = c->size * 4;
      grn_pat_cursor_entry *ss = GRN_REALLOC(c->ss, size);
      if (!ss) { return; /* give up */ }
      c->ss = ss;
      c->size = size;
    } else {
      if (!(c->ss = GRN_MALLOC(sizeof(grn_pat_cursor_entry) * INITIAL_SIZE))) {
        return; /* give up */
      }
      c->size = INITIAL_SIZE;
    }
  }
  se = &c->ss[c->sp++];
  se->id = id;
  se->check = check;
}

inline static grn_pat_cursor_entry *
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
      if (grn_pat_get(ctx, pat, key, key_size, NULL) != c->curr_rec) {
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
  if (!c->rest) { return GRN_ID_NIL; }
  if ((c->obj.header.flags & GRN_CURSOR_BY_ID)) {
    return grn_pat_cursor_next_by_id(ctx, c);
  }
  while ((se = pop(c))) {
    grn_id id = se->id;
    int check = se->check, ch;
    for (;;) {
      if (id) {
        PAT_AT(c->pat, id, node);
        if (node) {
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
                if ((lmin < lmax) ?
                    (memcmp(kmin, kmax, lmin) > 0) :
                    (memcmp(kmin, kmax, lmax) >= 0)) {
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
      } else {
        break;
      }
    }
  }
  return GRN_ID_NIL;
}

void
grn_pat_cursor_close(grn_ctx *ctx, grn_pat_cursor *c)
{
  GRN_ASSERT(c->ctx == ctx);
  if (c->ss) { GRN_FREE(c->ss); }
  GRN_FREE(c);
}

inline static int
bitcmp(const void *s1, const void *s2, int offset, int length)
{
  int r, rest = length + (offset & 7) - 8, bl = offset >> 3, mask = 0xff >> (offset & 7);
  unsigned char *a = (unsigned char *)s1 + bl, *b = (unsigned char *)s2 + bl;
  if (rest <= 0) {
    mask &= 0xff << -rest;
    return (*a & mask) - (*b & mask);
  }
  if ((r = (*a & mask) - (*b & mask))) { return r; }
  a++; b++;
  if ((bl = rest >> 3)) {
    if ((r = memcmp(a, b, bl))) { return r; }
    a += bl; b += bl;
  }
  mask = 0xff << (8 - (rest & 7));
  return (*a & mask) - (*b & mask);
}

inline static grn_rc
set_cursor_prefix(grn_ctx *ctx, grn_pat *pat, grn_pat_cursor *c,
                  const void *key, uint32_t key_size, int flags)
{
  int c0 = -1, ch;
  const uint8_t *k;
  uint32_t len = key_size * 16;
  grn_id id;
  pat_node *node;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  id = node->lr[1];
  while (id) {
    PAT_AT(pat, id, node);
    if (!node) { return GRN_FILE_CORRUPT; }
    ch = PAT_CHK(node);
    if (c0 < ch && ch < len - 1) {
      if (ch & 1) {
        id = (ch + 1 < len) ? node->lr[1] : node->lr[0];
      } else {
        id = node->lr[nth_bit((uint8_t *)key, ch, len)];
      }
      c0 = ch;
      continue;
    }
    if (!(k = pat_node_get_key(ctx, pat, node))) { break; }
    if (PAT_LEN(node) < key_size) { break; }
    if (!memcmp(k, key, key_size)) {
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
    }
    break;
  }
  return GRN_SUCCESS;
}

inline static grn_rc
set_cursor_near(grn_ctx *ctx, grn_pat *pat, grn_pat_cursor *c,
                uint32_t min_size, const void *key, int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int r, check = -1, ch;
  uint32_t min = min_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, pat->key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) { return GRN_FILE_CORRUPT; }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (check >= min) { push(c, id, check); }
      break;
    }
    if ((check += 2) < ch) {
      if (!(k = pat_node_get_key(ctx, pat, node))) { return GRN_FILE_CORRUPT; }
      if ((r = bitcmp(key, k, check >> 1, (ch - check) >> 1))) {
        if (ch >= min) {
          push(c, node->lr[1], ch);
          push(c, node->lr[0], ch);
        }
        break;
      }
    }
    check = ch;
    if (nth_bit((uint8_t *)key, check, pat->key_size)) {
      if (check >= min) { push(c, node->lr[0], check); }
      id = node->lr[1];
    } else {
      if (check >= min) { push(c, node->lr[1], check); }
      id = node->lr[0];
    }
  }
  return GRN_SUCCESS;
}

inline static grn_rc
set_cursor_common_prefix(grn_ctx *ctx, grn_pat *pat, grn_pat_cursor *c,
                         uint32_t min_size, const void *key, uint32_t key_size, int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int check = -1, ch;
  uint32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) { return GRN_FILE_CORRUPT; }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (!(k = pat_node_get_key(ctx, pat, node))) { return GRN_FILE_CORRUPT; }
      {
        uint32_t l = PAT_LEN(node);
        if (min_size <= l && l <= key_size) {
          if (!memcmp(key, k, l)) { push(c, id, check); }
        }
      }
      break;
    }
    check = ch;
    if (len <= check) { break; }
    if (check & 1) {
      grn_id id0 = node->lr[0];
      pat_node *node0;
      PAT_AT(pat, id0, node0);
      if (!node0) { return GRN_FILE_CORRUPT; }
      if (!(k = pat_node_get_key(ctx, pat, node0))) { return GRN_FILE_CORRUPT; }
      {
        uint32_t l = PAT_LEN(node);
        if (memcmp(key, k, l)) { break; }
        if (min_size <= l) {
          push(c, id0, check);
        }
      }
      id = node->lr[1];
    } else {
      id = node->lr[nth_bit((uint8_t *)key, check, len)];
    }
  }
  return GRN_SUCCESS;
}

inline static grn_rc
set_cursor_ascend(grn_ctx *ctx, grn_pat *pat, grn_pat_cursor *c,
                  const void *key, uint32_t key_size, int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int r, check = -1, ch, c2;
  uint32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) { return GRN_FILE_CORRUPT; }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (!(k = pat_node_get_key(ctx, pat, node))) { return GRN_FILE_CORRUPT; }
      {
        uint32_t l = PAT_LEN(node);
        if (l == key_size) {
          if (flags & GRN_CURSOR_GT) {
            if (memcmp(key, k, l) < 0) { push(c, id, check); }
          } else {
            if (memcmp(key, k, l) <= 0) { push(c, id, check); }
          }
        } else if (l < key_size) {
          if (memcmp(key, k, l) < 0) { push(c, id, check); }
        } else {
          if (memcmp(key, k, key_size) <= 0) { push(c, id, check); }
        }
      }
      break;
    }
    c2 = len < ch ? len : ch;
    if ((check += 2) < c2) {
      if (!(k = pat_node_get_key(ctx, pat, node))) { return GRN_FILE_CORRUPT; }
      if ((r = bitcmp(key, k, check >> 1, (c2 - check) >> 1))) {
        if (r < 0) {
          push(c, node->lr[1], ch);
          push(c, node->lr[0], ch);
        }
        break;
      }
    }
    check = ch;
    if (len <= check) { break; }
    if (check & 1) {
      if (check + 1 < len) {
        id = node->lr[1];
      } else {
        push(c, node->lr[1], check);
        id = node->lr[0];
      }
    } else {
      if (nth_bit((uint8_t *)key, check, len)) {
        id = node->lr[1];
      } else {
        push(c, node->lr[1], check);
        id = node->lr[0];
      }
    }
  }
  return GRN_SUCCESS;
}

inline static grn_rc
set_cursor_descend(grn_ctx *ctx, grn_pat *pat, grn_pat_cursor *c,
                   const void *key, uint32_t key_size, int flags)
{
  grn_id id;
  pat_node *node;
  const uint8_t *k;
  int r, check = -1, ch, c2;
  uint32_t len = key_size * 16;
  uint8_t keybuf[MAX_FIXED_KEY_SIZE];
  KEY_ENCODE(pat, keybuf, key, key_size);
  PAT_AT(pat, 0, node);
  for (id = node->lr[1]; id;) {
    PAT_AT(pat, id, node);
    if (!node) { return GRN_FILE_CORRUPT; }
    ch = PAT_CHK(node);
    if (ch <= check) {
      if (!(k = pat_node_get_key(ctx, pat, node))) { return GRN_FILE_CORRUPT; }
      {
        uint32_t l = PAT_LEN(node);
        if (l <= key_size) {
          if ((flags & GRN_CURSOR_LT) && l == key_size) {
            if (memcmp(key, k, l) > 0) { push(c, id, check); }
          } else {
            if (memcmp(key, k, l) >= 0) { push(c, id, check); }
          }
        } else {
          if (memcmp(key, k, key_size) > 0) { push(c, id, check); }
        }
      }
      break;
    }
    c2 = len < ch ? len : ch;
    if ((check += 2) < c2) {
      if (!(k = pat_node_get_key(ctx, pat, node))) { return GRN_FILE_CORRUPT; }
      if ((r = bitcmp(key, k, check >> 1, (c2 - check) >> 1))) {
        if (r >= 0) {
          push(c, node->lr[0], ch);
          push(c, node->lr[1], ch);
        }
        break;
      }
    }
    check = ch;
    if (len <= check) {
      push(c, node->lr[0], check);
      push(c, node->lr[1], check);
      break;
    }
    if (check & 1) {
      if (check + 1 < len) {
        push(c, node->lr[0], check);
        id = node->lr[1];
      } else {
        id = node->lr[0];
      }
    } else {
      if (nth_bit((uint8_t *)key, check, len)) {
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
grn_pat_cursor_open_by_id(grn_ctx *ctx, grn_pat *pat,
                          const void *min, uint32_t min_size,
                          const void *max, uint32_t max_size,
                          int offset, int limit, int flags)
{
  int dir;
  grn_pat_cursor *c;
  if (!pat || !ctx) { return NULL; }
  if (!(c = GRN_MALLOCN(grn_pat_cursor, 1))) { return NULL; }
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
      if (!(flags & GRN_CURSOR_LT)) { c->curr_rec++; }
    } else {
      c->curr_rec = pat->header->curr_rec + 1;
    }
    if (min) {
      if (!(c->tail = grn_pat_get(ctx, pat, min, min_size, NULL))) {
        c->curr_rec = GRN_ID_NIL;
        goto exit;
      }
      if ((flags & GRN_CURSOR_GT)) { c->tail++; }
    } else {
      c->tail = GRN_ID_NIL + 1;
    }
    if (c->curr_rec < c->tail) { c->tail = c->curr_rec; }
  } else {
    dir = 1;
    if (min) {
      if (!(c->curr_rec = grn_pat_get(ctx, pat, min, min_size, NULL))) {
        c->tail = GRN_ID_NIL;
        goto exit;
      }
      if (!(flags & GRN_CURSOR_GT)) { c->curr_rec--; }
    } else {
      c->curr_rec = GRN_ID_NIL;
    }
    if (max) {
      if (!(c->tail = grn_pat_get(ctx, pat, max, max_size, NULL))) {
        c->curr_rec = GRN_ID_NIL;
        goto exit;
      }
      if ((flags & GRN_CURSOR_LT)) { c->tail--; }
    } else {
      c->tail = pat->header->curr_rec;
    }
    if (c->tail < c->curr_rec) { c->tail = c->curr_rec; }
  }
  if (pat->header->n_garbages) {
    while (offset && c->curr_rec != c->tail) {
      uint32_t key_size;
      const void *key = _grn_pat_key(ctx, pat, c->curr_rec, &key_size);
      c->curr_rec += dir;
      if (grn_pat_get(ctx, pat, key, key_size, NULL) == c->curr_rec) {
        offset--;
      }
    }
  } else {
    c->curr_rec += dir * offset;
  }
  c->rest = (limit < 0) ? GRN_ID_MAX : limit;
exit :
  return c;
}

grn_pat_cursor *
grn_pat_cursor_open(grn_ctx *ctx, grn_pat *pat,
                    const void *min, uint32_t min_size,
                    const void *max, uint32_t max_size,
                    int offset, int limit, int flags)
{
  grn_id id;
  pat_node *node;
  grn_pat_cursor *c;
  if (!pat || !ctx) { return NULL; }
  if ((flags & GRN_CURSOR_BY_ID)) {
    return grn_pat_cursor_open_by_id(ctx, pat, min, min_size, max, max_size,
                                     offset, limit, flags);
  }
  if (!(c = GRN_MALLOCN(grn_pat_cursor, 1))) { return NULL; }
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
    if (max_size) {
      if ((pat->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)) {
        set_cursor_common_prefix(ctx, pat, c, min_size, max, max_size, flags);
      } else {
        set_cursor_near(ctx, pat, c, min_size, max, flags);
      }
    } else {
      set_cursor_prefix(ctx, pat, c, min, min_size, flags);
    }
  } else {
    if (flags & GRN_CURSOR_DESCENDING) {
      if (min) {
        set_cursor_ascend(ctx, pat, c, min, min_size, flags);
        c->obj.header.flags = GRN_CURSOR_ASCENDING;
        c->tail = grn_pat_cursor_next(ctx, c);
        c->sp = 0;
        if (!c->tail) { goto exit; }
      }
      if (max) {
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
      if (max) {
        set_cursor_descend(ctx, pat, c, max, max_size, flags);
        c->obj.header.flags = GRN_CURSOR_DESCENDING;
        c->tail = grn_pat_cursor_next(ctx, c);
        c->sp = 0;
        if (!c->tail) { goto exit; }
      }
      if (min) {
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
  }
exit :
  c->obj.header.flags = flags;
  c->curr_rec = GRN_ID_NIL;
  while (offset--) { grn_pat_cursor_next(ctx, c); }
  c->rest = (limit < 0) ? GRN_ID_MAX : limit;
  return c;
}

int
grn_pat_cursor_get_key(grn_ctx *ctx, grn_pat_cursor *c, void **key)
{
  *key = c->curr_key;
  return grn_pat_get_key(ctx, c->pat, c->curr_rec, *key, GRN_TABLE_MAX_KEY_SIZE);
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
grn_pat_cursor_get_key_value(grn_ctx *ctx, grn_pat_cursor *c,
                             void **key, uint32_t *key_size, void **value)
{
  int value_size = (int)c->pat->value_size;
  if (key_size) {
    *key_size = (uint32_t) grn_pat_get_key(ctx, c->pat, c->curr_rec, c->curr_key,
                                           GRN_TABLE_MAX_KEY_SIZE);
    if (key) { *key = c->curr_key; }
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
grn_pat_cursor_set_value(grn_ctx *ctx, grn_pat_cursor *c, void *value, int flags)
{
  return grn_pat_set_value(ctx, c->pat, c->curr_rec, value, flags);
}

grn_rc
grn_pat_cursor_delete(grn_ctx *ctx, grn_pat_cursor *c,
                      grn_table_delete_optarg *optarg)
{
  return grn_pat_delete_by_id(ctx, c->pat, c->curr_rec, optarg);
}
