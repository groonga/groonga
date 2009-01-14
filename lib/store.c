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
#include "str.h"
#include "store.h"
#include "ctx.h"
#include "ql.h"
#include <string.h>

/* rectangular arrays */

#define GRN_RA_IDSTR "GROONGA:RA:0001"
#define GRN_RA_SEGMENT_SIZE (1 << 22)

grn_ra *
grn_ra_create(grn_ctx *ctx, const char *path, unsigned int element_size)
{
  grn_io *io;
  int max_segments, n_elm, w_elm;
  grn_ra *ra = NULL;
  struct grn_ra_header *header;
  unsigned actual_size;
  if (element_size > GRN_RA_SEGMENT_SIZE) {
    GRN_LOG(grn_log_error, "element_size too large (%d)", element_size);
    return NULL;
  }
  for (actual_size = 1; actual_size < element_size; actual_size *= 2) ;
  max_segments = ((GRN_ID_MAX + 1) / GRN_RA_SEGMENT_SIZE) * actual_size;
  io = grn_io_create(ctx, path, sizeof(struct grn_ra_header),
                     GRN_RA_SEGMENT_SIZE, max_segments, grn_io_auto, 0);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  memcpy(header->idstr, GRN_RA_IDSTR, 16);
  grn_io_set_type(io, GRN_COLUMN_FIX_SIZE);
  header->element_size = actual_size;
  header->curr_max = 0;
  if (!(ra = GRN_GMALLOC(sizeof(grn_ra)))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  n_elm = GRN_RA_SEGMENT_SIZE / header->element_size;
  for (w_elm = 22; (1 << w_elm) > n_elm; w_elm--);
  GRN_DB_OBJ_SET_TYPE(ra, GRN_COLUMN_FIX_SIZE);
  ra->io = io;
  ra->header = header;
  ra->element_mask =  n_elm - 1;
  ra->element_width = w_elm;
  return ra;
}

grn_ra *
grn_ra_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  int n_elm, w_elm;
  grn_ra *ra = NULL;
  struct grn_ra_header *header;
  io = grn_io_open(ctx, path, grn_io_auto);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  if (memcmp(header->idstr, GRN_RA_IDSTR, 16)) {
    GRN_LOG(grn_log_error, "ra_idstr (%s)", header->idstr);
    grn_io_close(ctx, io);
    return NULL;
  }
  if (!(ra = GRN_GMALLOC(sizeof(grn_ra)))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  n_elm = GRN_RA_SEGMENT_SIZE / header->element_size;
  for (w_elm = 22; (1 << w_elm) > n_elm; w_elm--);
  GRN_DB_OBJ_SET_TYPE(ra, GRN_COLUMN_FIX_SIZE);
  ra->io = io;
  ra->header = header;
  ra->element_mask =  n_elm - 1;
  ra->element_width = w_elm;
  return ra;
}

grn_rc
grn_ra_info(grn_ctx *ctx, grn_ra *ra, unsigned int *element_size, grn_id *curr_max)
{
  if (!ra) { return grn_invalid_argument; }
  if (element_size) { *element_size = ra->header->element_size; }
  if (curr_max) { *curr_max = ra->header->curr_max; }
  return grn_success;
}

grn_rc
grn_ra_close(grn_ctx *ctx, grn_ra *ra)
{
  grn_rc rc;
  if (!ra) { return grn_invalid_argument; }
  rc = grn_io_close(ctx, ra->io);
  GRN_GFREE(ra);
  return rc;
}

grn_rc
grn_ra_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return grn_invalid_argument; }
  return grn_io_remove(ctx, path);
}

void *
grn_ra_get(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  void *p;
  uint16_t seg;
  if (id > GRN_ID_MAX) { return NULL; }
  seg = id >> ra->element_width;
  GRN_IO_SEG_MAP(ra->io, seg, p);
  if (!p) { return NULL; }
  if (id > ra->header->curr_max) { ra->header->curr_max = id; }
  return (void *)(((byte *)p) + ((id & ra->element_mask) * ra->header->element_size));
}

void *
grn_ra_at(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  void *p;
  uint16_t seg;
  static char buf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  if (id > ra->header->curr_max) { return (void *) buf; }
  seg = id >> ra->element_width;
  GRN_IO_SEG_MAP(ra->io, seg, p);
  if (!p) { return NULL; }
  return (void *)(((byte *)p) + ((id & ra->element_mask) * ra->header->element_size));
}

/**** jagged arrays ****/

#define USE_JA01

#define SEG_NOT_ASSIGNED 0xffffffff

#ifdef USE_JA01

#define GRN_JA_IDSTR "GROONGA:JA:0001"

#define W_OF_JA_MAX 38
#define W_OF_JA_SEGMENT 22
#define W_OF_JA_MAX_SEGMENTS (W_OF_JA_MAX - W_OF_JA_SEGMENT)

#define W_OF_JA_EINFO 3
#define W_OF_JA_EINFO_IN_A_SEGMENT (W_OF_JA_SEGMENT - W_OF_JA_EINFO)
#define N_OF_JA_EINFO_IN_A_SEGMENT (1U << W_OF_JA_EINFO_IN_A_SEGMENT)
#define JA_EINFO_MASK (N_OF_JA_EINFO_IN_A_SEGMENT - 1)

#define N_ELEMENT_VARIATION (W_OF_JA_SEGMENT - W_OF_JA_EINFO + 1)

#define JA_SEGMENT_SIZE (1U << W_OF_JA_SEGMENT)
#define JA_MAX_SEGMENTS (1U << W_OF_JA_MAX_SEGMENTS)

#define JA_N_BSEGMENTS (1U << (W_OF_JA_MAX_SEGMENTS - 7))
#define JA_N_ESEGMENTS (1U << 9)

typedef struct _grn_ja_einfo grn_ja_einfo;

struct _grn_ja_einfo {
  uint16_t seg;
  uint16_t pos;
  uint16_t size;
  uint8_t tail[2];
};

#define EINFO_SET(e,_seg,_pos,_size) {\
  (e)->seg = _seg;\
  (e)->pos = (_pos) >> 4;\
  (e)->size = _size;\
  (e)->tail[0] = (((_pos) >> 14) & 0xc0) + ((_size) >> 16);\
  (e)->tail[1] = 0;\
}

#define EINFO_GET(e,_seg,_pos,_size) {\
  _seg = (e)->seg;\
  _pos = ((e)->pos + (((e)->tail[0] & 0xc0) << 10)) << 4;\
  _size = (e)->size + (((e)->tail[0] & 0x3f) << 16);\
}

typedef struct {
  uint32_t seg;
  uint32_t pos;
} ja_pos;

#define N_GARBAGES 524280

typedef struct {
  uint32_t head;
  uint32_t tail;
  uint32_t nrecs;
  uint32_t next;
  ja_pos recs[N_GARBAGES];
} grn_ja_ginfo;

struct grn_ja_header {
  char idstr[16];
  unsigned max_element_size;
  unsigned max_segments;
  ja_pos free_elements[N_ELEMENT_VARIATION];
  uint32_t garbages[N_ELEMENT_VARIATION];
  uint32_t ngarbages[N_ELEMENT_VARIATION];
  uint8_t segments[JA_MAX_SEGMENTS >> 3];
  uint32_t esegs[JA_N_ESEGMENTS];
  //  uint32_t bsegs[JA_N_BSEGMENTS];
};

#define JA_SEG_ESEG 1 /* entry info */
#define JA_SEG_GSEG 2 /* garbage info */
#define JA_SEG_HSEG 3 /* huge records */

#define JA_IMMEDIATE 1
#define JA_HUGE      2

#define HEADER_SEGMENTS_AT(ja,offset) \
  ((((ja)->header->segments[((offset) >> 3)]) >> ((offset) & 7)) & 1)

#define HEADER_SEGMENTS_ON(ja,offset) \
  (((ja)->header->segments[((offset) >> 3)]) |= (1 << ((offset) & 7)))

#define HEADER_SEGMENTS_OFF(ja,offset) \
  (((ja)->header->segments[((offset) >> 3)]) &= ~(1 << ((offset) & 7)))

grn_ja *
grn_ja_create(grn_ctx *ctx, const char *path, unsigned int max_element_size, uint32_t flags)
{
  int i;
  grn_io *io;
  int max_segments;
  grn_ja *ja = NULL;
  struct grn_ja_header *header;
  max_segments = max_element_size * 128;
  if (max_segments > JA_MAX_SEGMENTS) { max_segments = JA_MAX_SEGMENTS; }
  io = grn_io_create(ctx, path, sizeof(struct grn_ja_header),
                     JA_SEGMENT_SIZE, max_segments, grn_io_auto, 0);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  memcpy(header->idstr, GRN_JA_IDSTR, 16);
  grn_io_set_type(io, GRN_COLUMN_VAR_SIZE);
  for (i = 0; i < JA_N_ESEGMENTS; i++) { header->esegs[i] = SEG_NOT_ASSIGNED; }
  //  for (i = 0; i < JA_N_BSEGMENTS; i++) { header->bsegs[i] = SEG_NOT_ASSIGNED; }
  if (!(ja = GRN_GMALLOC(sizeof(grn_ja)))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ja, GRN_COLUMN_VAR_SIZE);
  ja->io = io;
  ja->header = header;
  header->max_element_size = max_element_size;
  header->max_segments = max_segments;
  HEADER_SEGMENTS_ON(ja, 0);
  header->esegs[0] = 0;
  return ja;
}

grn_ja *
grn_ja_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  grn_ja *ja = NULL;
  struct grn_ja_header *header;
  io = grn_io_open(ctx, path, grn_io_auto);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  if (memcmp(header->idstr, GRN_JA_IDSTR, 16)) {
    GRN_LOG(grn_log_error, "ja_idstr (%s)", header->idstr);
    grn_io_close(ctx, io);
    return NULL;
  }
  if (!(ja = GRN_GMALLOC(sizeof(grn_ja)))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ja, GRN_COLUMN_VAR_SIZE);
  ja->io = io;
  ja->header = header;
  return ja;
}

grn_rc
grn_ja_info(grn_ctx *ctx, grn_ja *ja, unsigned int *max_element_size)
{
  if (!ja) { return grn_invalid_argument; }
  if (max_element_size) { *max_element_size = ja->header->max_element_size; }
  return grn_success;
}

grn_rc
grn_ja_close(grn_ctx *ctx, grn_ja *ja)
{
  grn_rc rc;
  if (!ja) { return grn_invalid_argument; }
  rc = grn_io_close(ctx, ja->io);
  GRN_GFREE(ja);
  return rc;
}

grn_rc
grn_ja_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return grn_invalid_argument; }
  return grn_io_remove(ctx, path);
}

void *
grn_ja_ref(grn_ctx *ctx, grn_ja *ja, grn_id id, uint32_t *value_len)
{
  grn_ja_einfo *einfo;
  uint32_t lseg, *pseg, pos;
  lseg = id >> W_OF_JA_EINFO_IN_A_SEGMENT;
  pos = id & JA_EINFO_MASK;
  pseg = &ja->header->esegs[lseg];
  if (*pseg == SEG_NOT_ASSIGNED) { *value_len = 0; return NULL; }
  GRN_IO_SEG_MAP(ja->io, *pseg, einfo);
  if (!einfo) { *value_len = 0; return NULL; }
  if (einfo[pos].tail[1] & JA_IMMEDIATE) {
    *value_len = einfo[pos].tail[1] >> 1;
    return (void *) &einfo[pos];
  }
  {
    void *value;
    grn_io_win iw;
    uint32_t jag, vpos, vsize;
    EINFO_GET(&einfo[pos], jag, vpos, vsize);
    value = grn_io_win_map2(ja->io, ctx, &iw, jag, vpos, vsize, grn_io_rdonly);
    // printf("at id=%d value=%p jag=%d vpos=%d ei=%p(%d:%d)\n", id, value, jag, vpos, &einfo[pos], einfo[pos].pos, einfo[pos].tail[0]);
    if (!value) { *value_len = 0; return NULL; }
    *value_len = vsize;
    return value;
  }
}

grn_rc
grn_ja_unref(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, uint32_t value_len)
{
  if (!value) { return grn_invalid_argument; }
  // todo : depend on grn_io_win_map2 implementation. unmap2 should be used
  if (value_len > JA_SEGMENT_SIZE) { GRN_FREE(value); }
  return grn_success;
}

grn_rc
grn_ja_free(grn_ctx *ctx, grn_ja *ja, grn_ja_einfo *einfo)
{
  grn_ja_ginfo *ginfo;
  uint32_t seg, pos, element_size, size, m, *gseg;
  if (!einfo->size) { return grn_success; }
  if (einfo->tail[1] & JA_IMMEDIATE) { return grn_success; }
  EINFO_GET(einfo, seg, pos, element_size);
  if (element_size > JA_SEGMENT_SIZE) {
    int n = (pos + element_size + JA_SEGMENT_SIZE - 1) >> W_OF_JA_SEGMENT;
    for (; n--; seg++) { HEADER_SEGMENTS_OFF(ja, seg); }
    return grn_success;
  }
  {
    int es = element_size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
    size = 1 << m;
  }
  gseg = &ja->header->garbages[m - W_OF_JA_EINFO];
  while (*gseg) {
    GRN_IO_SEG_MAP(ja->io, *gseg, ginfo);
    if (!ginfo) { return grn_memory_exhausted; }
    if (ginfo->nrecs < N_GARBAGES) { break; }
    gseg = &ginfo->next;
  }
  if (!*gseg) {
    uint32_t i = 0;
    while (HEADER_SEGMENTS_AT(ja, i)) {
      if (++i >= ja->header->max_segments) { return grn_memory_exhausted; }
    }
    HEADER_SEGMENTS_ON(ja, i);
    *gseg = i;
    GRN_IO_SEG_MAP(ja->io, *gseg, ginfo);
    if (!ginfo) { return grn_memory_exhausted; }
    ginfo->head = 0;
    ginfo->tail = 0;
    ginfo->nrecs = 0;
    ginfo->next = 0;
  }
  ginfo->recs[ginfo->head].seg = seg;
  ginfo->recs[ginfo->head].pos = pos;
  if (++ginfo->head == N_GARBAGES) { ginfo->head = 0; }
  ginfo->nrecs++;
  ja->header->ngarbages[m - W_OF_JA_EINFO]++;
  return grn_success;
}

grn_rc
grn_ja_replace(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_ja_einfo *ei)
{
  uint32_t lseg, *pseg, pos;
  grn_ja_einfo *einfo, eback;
  lseg = id >> W_OF_JA_EINFO_IN_A_SEGMENT;
  pos = id & JA_EINFO_MASK;
  pseg = &ja->header->esegs[lseg];
  if (*pseg == SEG_NOT_ASSIGNED) {
    int i = 0;
    while (HEADER_SEGMENTS_AT(ja, i)) {
      if (++i >= ja->header->max_segments) { return grn_memory_exhausted; }
    }
    HEADER_SEGMENTS_ON(ja, i);
    *pseg = i;
  }
  GRN_IO_SEG_MAP(ja->io, *pseg, einfo);
  if (!einfo) { return grn_memory_exhausted; }
  eback = einfo[pos];
  einfo[pos] = *ei;
  GRN_SET_64BIT(&einfo[pos], *ei);
  grn_ja_free(ctx, ja, &eback);
  return grn_success;
}

#define N_GARBAGES_TH 10

// todo : grn_io_win_map2 cause verbose copy when nseg > 1, it should be copied directly.
grn_rc
grn_ja_alloc(grn_ctx *ctx, grn_ja *ja, int element_size, grn_ja_einfo *einfo, grn_io_win *iw)
{
  void *addr;
  iw->io = ja->io;
  iw->ctx = ctx;
  iw->cached = 1;
  if (element_size < 8) {
    einfo->tail[1] = element_size * 2 + JA_IMMEDIATE;
    iw->addr = (void *)einfo;
    return grn_success;
  }
  if (element_size > JA_SEGMENT_SIZE) {
    int i, j, n = (element_size + JA_SEGMENT_SIZE - 1) >> W_OF_JA_SEGMENT;
    for (i = 0, j = -1; i < ja->header->max_segments; i++) {
      if (HEADER_SEGMENTS_AT(ja, i)) {
        j = i;
      } else {
        if (i == j + n) {
          j++;
          addr = grn_io_win_map2(ja->io, ctx, iw, j, 0, element_size, grn_io_wronly);
          if (!addr) { return grn_memory_exhausted; }
          EINFO_SET(einfo, j, 0, element_size);
          // einfo->tail[1] = JA_HUGE;
          for (; j <= i; j++) { HEADER_SEGMENTS_ON(ja, j); }
          return grn_success;
        }
      }
    }
    GRN_LOG(grn_log_crit, "ja full. requested element_size=%d.", element_size);
    return grn_memory_exhausted;
  } else {
    ja_pos *vp;
    int m, aligned_size, es = element_size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
    aligned_size = 1 << m;
    if (ja->header->ngarbages[m - W_OF_JA_EINFO] > N_GARBAGES_TH) {
      grn_ja_ginfo *ginfo;
      uint32_t seg, pos, *gseg;
      gseg = &ja->header->garbages[m - W_OF_JA_EINFO];
      while (*gseg) {
        GRN_IO_SEG_MAP(ja->io, *gseg, ginfo);
        if (!ginfo) { return grn_memory_exhausted; }
        if (ginfo->next || ginfo->nrecs > N_GARBAGES_TH) {
          seg = ginfo->recs[ginfo->tail].seg;
          pos = ginfo->recs[ginfo->tail].pos;
          GRN_IO_SEG_MAP(ja->io, seg, addr);
          if (!addr) { return grn_memory_exhausted; }
          EINFO_SET(einfo, seg, pos, element_size);
          iw->addr = (byte *)addr + pos;
          if (++ginfo->tail == N_GARBAGES) { ginfo->tail = 0; }
          ginfo->nrecs--;
          ja->header->ngarbages[m - W_OF_JA_EINFO]--;
          if (!ginfo->nrecs) {
            HEADER_SEGMENTS_OFF(ja, *gseg);
            *gseg = ginfo->next;
          }
          return grn_success;
        }
        gseg = &ginfo->next;
      }
    }
    vp = &ja->header->free_elements[m - W_OF_JA_EINFO];
    if (!vp->seg) {
      int i = 0;
      while (HEADER_SEGMENTS_AT(ja, i)) {
        if (++i >= ja->header->max_segments) { return grn_memory_exhausted; }
      }
      HEADER_SEGMENTS_ON(ja, i);
      vp->seg = i;
      vp->pos = 0;
    }
    EINFO_SET(einfo, vp->seg, vp->pos, element_size);
    GRN_IO_SEG_MAP(ja->io, vp->seg, addr);
    // printf("addr=%p seg=%d pos=%d\n", addr, vp->seg, vp->pos);
    if (!addr) { return grn_memory_exhausted; }
    iw->addr = (byte *)addr + vp->pos;
    if ((vp->pos += aligned_size) == JA_SEGMENT_SIZE) {
      vp->seg = 0;
      vp->pos = 0;
    }
    return grn_success;
  }
}

grn_rc
grn_ja_put(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, int value_len, int flags)
{
  int rc;
  grn_io_win iw;
  grn_ja_einfo einfo;
  if ((flags & GRN_ST_APPEND)) {
    if (value_len) {
      uint32_t old_len;
      void *oldvalue = grn_ja_ref(ctx, ja, id, &old_len);
      if (oldvalue) {
        if ((rc = grn_ja_alloc(ctx, ja, value_len + old_len, &einfo, &iw))) { return rc; }
        memcpy(iw.addr, oldvalue, old_len);
        memcpy((byte *)iw.addr + old_len, value, value_len);
        grn_ja_unref(ctx, ja, id, oldvalue, old_len);
        grn_io_win_unmap2(&iw);
      } else {
        if ((rc = grn_ja_alloc(ctx, ja, value_len, &einfo, &iw))) { return rc; }
        memcpy(iw.addr, value, value_len);
        grn_io_win_unmap2(&iw);
      }
    }
  } else {
    if (value_len) {
      if ((rc = grn_ja_alloc(ctx, ja, value_len, &einfo, &iw))) { return rc; }
      // printf("put id=%d, value_len=%d value=%p ei=%p(%d:%d)\n", id, value_len, buf, &einfo, einfo.pos, einfo.tail[0]);
      memcpy(iw.addr, value, value_len);
      grn_io_win_unmap2(&iw);
    } else {
      memset(&einfo, 0, sizeof(grn_ja_einfo));
    }
  }
  return grn_ja_replace(ctx, ja, id, &einfo);
}

grn_rc
grn_ja_putv(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_obj *vector, int flags)
{
  grn_obj header;
  uint32_t last = 0;
  grn_rc rc = grn_success;
  grn_vector *v = (grn_vector *)vector;
  int i, n = grn_vector_size(ctx, vector);
  GRN_OBJ_INIT(&header, GRN_BULK, 0);
  grn_bulk_benc(ctx, &header, n);
  for (i = 0; i < n; i++) {
    grn_bulk_benc(ctx, &header, v->offsets[i] - last);
    last = v->offsets[i];
  }
  {
    grn_io_win iw;
    grn_ja_einfo einfo;
    size_t sizeh = GRN_BULK_VSIZE(&header);
    size_t sizev = GRN_BULK_VSIZE(vector);
    if ((rc = grn_ja_alloc(ctx, ja, sizeh + sizev, &einfo, &iw))) { goto exit; }
    memcpy(iw.addr, header.u.b.head, sizeh);
    memcpy((char *)iw.addr + sizeh, vector->u.b.head, sizev);
    grn_io_win_unmap2(&iw);
    rc = grn_ja_replace(ctx, ja, id, &einfo);
  }
exit :
  GRN_OBJ_FIN(ctx, &header);
  return rc;
}

int
grn_ja_size(grn_ctx *ctx, grn_ja *ja, grn_id id)
{
  grn_ja_einfo *einfo;
  uint32_t lseg, *pseg, pos;
  lseg = id >> W_OF_JA_EINFO_IN_A_SEGMENT;
  pos = id & JA_EINFO_MASK;
  pseg = &ja->header->esegs[lseg];
  if (*pseg == SEG_NOT_ASSIGNED) { return -1; }
  GRN_IO_SEG_MAP(ja->io, *pseg, einfo);
  if (!einfo) { return -1; }
  if (einfo[pos].tail[1] & JA_IMMEDIATE) {
    return einfo[pos].tail[1] >> 1;
  } else {
    return einfo[pos].size + ((einfo[pos].tail[0] & 0x3f) << 16);
  }
}

int
grn_ja_defrag(grn_ctx *ctx, grn_ja *ja, int threshold)
{
  return 0;
}

#else /* USE_JA01 */

#define GRN_JA_IDSTR "GROONGA:JA:0001"

struct grn_ja_header {
  char idstr[16];
  uint32_t flags;
  uint32_t align_width;
  uint32_t segment_width;
  uint32_t element_size;
  uint32_t curr_pos;
};

#define GRN_JA_DEFAULT_ALIGN_WIDTH    4
#define GRN_JA_DEFAULT_SEGMENT_WIDTH 20

#define JA_SEGMENT_SIZE (1 << GRN_JA_DEFAULT_SEGMENT_WIDTH)

#define JA_ALIGN_WIDTH (ja->header->align_width)
#define JA_ALIGN_MASK ((1U << JA_ALIGN_WIDTH) - 1)

#define JA_DSEG_WIDTH (ja->header->segment_width)
#define JA_DSEG_MASK ((1U << JA_DSEG_WIDTH) - 1)

#define JA_ESEG_WIDTH (JA_DSEG_WIDTH - 3)
#define JA_ESEG_MASK ((1U << JA_ESEG_WIDTH) - 1)

#define JA_EPOS_WIDTH (JA_DSEG_WIDTH - JA_ALIGN_WIDTH)
#define JA_EPOS_MASK ((1U << JA_EPOS_WIDTH) - 1)

// segment_width, align_width, flags
grn_ja *
grn_ja_create(grn_ctx *ctx, const char *path,
              unsigned int max_element_size, uint32_t flags)
{
  int i;
  grn_io *io;
  grn_ja *ja;
  struct grn_ja_header *header;
  uint32_t align_width = GRN_JA_DEFAULT_ALIGN_WIDTH;
  uint32_t segment_width = GRN_JA_DEFAULT_SEGMENT_WIDTH;
  uint32_t max_dsegs = 1 << (32 - (segment_width - align_width));
  uint32_t max_esegs = 1 << (31 - segment_width);
  if (align_width > segment_width || align_width + 32 < segment_width) {
    GRN_LOG(grn_log_error, "invalid align_width, segment_width value");
    return NULL;
  }
  io = grn_io_create(path,
                     sizeof(struct grn_ja_header) +
                     max_dsegs * sizeof(int32_t) +
                     max_esegs * sizeof(int32_t),
                     1 << segment_width, max_dsegs, grn_io_auto, 0);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  memcpy(header->idstr, GRN_JA_IDSTR, 16);
  grn_io_set_type(io, GRN_COLUMN_VAR_SIZE);
  header->flags = flags;
  header->align_width = align_width;
  header->segment_width = segment_width;
  header->element_size = max_element_size;
  header->curr_pos = 0;
  if (!(ja = GRN_GMALLOC(sizeof(grn_ja)))) {
    grn_io_close(io);
    return NULL;
  }
  ja->io = io;
  ja->obj.header.type = GRN_COLUMN_VAR_SIZE;
  ja->header = header;
  ja->dsegs = (uint32_t *)(((uintptr_t) header) + sizeof(struct grn_ja_header));
  ja->esegs = &ja->dsegs[max_dsegs];
  for (i = 0; i < max_esegs; i++) { ja->esegs[i] = SEG_NOT_ASSIGNED; }
  return ja;
}

grn_ja *
grn_ja_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  grn_ja *ja = NULL;
  struct grn_ja_header *header;
  io = grn_io_open(path, grn_io_auto);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  if (memcmp(header->idstr, GRN_JA_IDSTR, 16)) {
    GRN_LOG(grn_log_error, "ja_idstr (%s)", header->idstr);
    grn_io_close(io);
    return NULL;
  }
  if (!(ja = GRN_GMALLOC(sizeof(grn_ja)))) {
    grn_io_close(io);
    return NULL;
  }
  ja->io = io;
  ja->obj.header.type = GRN_COLUMN_VAR_SIZE;
  ja->header = header;
  ja->dsegs = (uint32_t *)(((uintptr_t) header) + sizeof(struct grn_ja_header));
  ja->esegs = &ja->dsegs[1 << (32 - JA_EPOS_WIDTH)];
  return ja;
}

grn_rc
grn_ja_info(grn_ctx *ctx, grn_ja *ja, unsigned int *max_element_size)
{
  if (!ja) { return grn_invalid_argument; }
  if (max_element_size) { *max_element_size = 4294967295U; }
  return grn_success;
}

grn_rc
grn_ja_close(grn_ctx *ctx, grn_ja *ja)
{
  grn_rc rc;
  if (!ja) { return grn_invalid_argument; }
  rc = grn_io_close(ja->io);
  GRN_GFREE(ja);
  return rc;
}

grn_rc
grn_ja_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return grn_invalid_argument; }
  return grn_io_remove(path);
}

inline static void *
grn_ja_ref_raw(grn_ctx *ctx, grn_ja *ja, grn_id id, uint32_t *value_len)
{
  grn_io_ja_einfo *ep;
  uint32_t lseg = id >> JA_ESEG_WIDTH;
  uint32_t lpos = id & JA_ESEG_MASK;
  uint32_t *pseg = &ja->esegs[lseg];
  if (*pseg == SEG_NOT_ASSIGNED) { *value_len = 0; return NULL; }
  GRN_IO_SEG_MAP(ja->io, *pseg, ep);
  if (!ep) { *value_len = 0; return NULL; }
  ep += lpos;
  for (;;) {
    void *value;
    uint32_t epos = ep->pos;
    uint32_t dseg = epos >> JA_EPOS_WIDTH;
    uint32_t doff = (epos & JA_EPOS_MASK) << JA_ALIGN_WIDTH;
    if (!(*value_len = ep->size)) { return NULL; }
    if (grn_io_read_ja(ja->io, ctx, ep, epos, id, dseg, doff, &value, value_len)
        != grn_internal_error) {
      return value;
    }
  }
}

#ifndef NO_ZLIB
#include <zlib.h>

inline static void *
grn_ja_ref_zlib(grn_ctx *ctx, grn_ja *ja, grn_id id, uint32_t *value_len)
{
  grn_ctx *ctx = &grn_gctx; /* todo : replace it with the local ctx */
  z_stream zstream;
  void *value, *zvalue;
  uint32_t zvalue_len = *value_len + sizeof (uint64_t);
  if (!(zvalue = grn_ja_ref_raw(ctx, ja, id, &zvalue_len))) {
    *value_len = 0; return NULL;
  }
  zstream.next_in = (Bytef *)((uint64_t *)zvalue + 1);
  zstream.avail_in = zvalue_len - sizeof (uint64_t);
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  if (inflateInit2(&zstream, 15 /* windowBits */) != Z_OK) {
    GRN_FREE((grn_io_ja_ehead *)zvalue - 1);
    *value_len = 0;
    return NULL;
  }
  if (!(value = GRN_MALLOC(*(uint64_t *)zvalue + sizeof (grn_io_ja_ehead)))) {
    inflateEnd(&zstream);
    GRN_FREE((grn_io_ja_ehead *)zvalue - 1);
    *value_len = 0;
    return NULL;
  }
  zstream.next_out = (Bytef *)((grn_io_ja_ehead *)value + 1);
  zstream.avail_out = *(uint64_t *)zvalue;
  if (inflate(&zstream, Z_FINISH) != Z_STREAM_END) {
    inflateEnd(&zstream);
    GRN_FREE(value);
    GRN_FREE((grn_io_ja_ehead *)zvalue - 1);
    *value_len = 0;
    return NULL;
  }
  *value_len = zstream.total_out;
  if (inflateEnd(&zstream) != Z_OK) {
    GRN_FREE(value);
    GRN_FREE((grn_io_ja_ehead *)zvalue - 1);
    *value_len = 0;
    return NULL;
  }
  *(grn_io_ja_ehead *)value = ((grn_io_ja_ehead *)zvalue)[-1];
  GRN_FREE((grn_io_ja_ehead *)zvalue - 1);
  return (grn_io_ja_ehead *)value + 1;
}
#endif /* NO_ZLIB */

#ifndef NO_LZO
#include <lzo/lzo1x.h>

inline static void *
grn_ja_ref_lzo(grn_ctx *ctx, grn_ja *ja, grn_id id, uint32_t *value_len)
{
  grn_ctx *ctx = &grn_gctx; /* todo : replace it with the local ctx */
  void *value, *lvalue;
  uint32_t lvalue_len = *value_len + sizeof (uint64_t);
  lzo_uint loutlen;
  if (!(lvalue = grn_ja_ref_raw(ctx, ja, id, &lvalue_len))) {
    *value_len = 0; return NULL;
  }
  if (!(value = GRN_MALLOC(*(uint64_t *)lvalue + sizeof (grn_io_ja_ehead)))) {
    GRN_FREE((grn_io_ja_ehead *)lvalue - 1);
    *value_len = 0;
    return NULL;
  }
  loutlen = *(uint64_t *)lvalue;
  switch (lzo1x_decompress((lzo_bytep)((uint64_t *)lvalue + 1), lvalue_len - sizeof (uint64_t),
                           (lzo_bytep)((grn_io_ja_ehead *)value + 1), &loutlen, NULL)) {
  case LZO_E_OK :
  case LZO_E_INPUT_NOT_CONSUMED :
    break;
  default :
    GRN_FREE(value);
    GRN_FREE((grn_io_ja_ehead *)lvalue - 1);
    *value_len = 0;
    return NULL;
  }
  *value_len = loutlen;
  *(grn_io_ja_ehead *)value = ((grn_io_ja_ehead *)lvalue)[-1];
  GRN_FREE((grn_io_ja_ehead *)lvalue - 1);
  return (grn_io_ja_ehead *)value + 1;
}
#endif /* NO_LZO */

void *
grn_ja_ref(grn_ctx *ctx, grn_ja *ja, grn_id id, uint32_t *value_len)
{
#ifndef NO_ZLIB
  if (ja->header->flags & GRN_OBJ_COMPRESS_ZLIB) {
    return grn_ja_ref_zlib(ctx, ja, id, value_len);
  }
#endif /* NO_ZLIB */
#ifndef NO_LZO
  if (ja->header->flags & GRN_OBJ_COMPRESS_LZO) {
    return grn_ja_ref_lzo(ctx, ja, id, value_len);
  }
#endif /* NO_LZO */
  return grn_ja_ref_raw(ctx, ja, id, value_len);
}

grn_rc
grn_ja_unref(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, uint32_t value_len)
{
  GRN_FREE((void *)(((uintptr_t) value) - sizeof(grn_io_ja_ehead)));
  return grn_success;
}

#define SEG_ESEG 0xffffffff

grn_rc
assign_eseg(grn_ja *ja, uint32_t lseg)
{
  int i;
  uint32_t max_dsegs = 1 << (32 - JA_EPOS_WIDTH);
  for (i = 0; i < max_dsegs; i++) {
    if (ja->dsegs[i] == 0) {
      if (ja->header->curr_pos && i == (ja->header->curr_pos >> JA_EPOS_WIDTH)) {
        // todo : curr_pos must be moved.
      }
      ja->dsegs[i] = SEG_ESEG;
      ja->esegs[lseg] = i;
      return grn_success;
    }
  }
  return grn_internal_error;
}

static grn_rc
assign_rec(grn_ja *ja, int value_len)
{
  if (ja->header->curr_pos) {
    uint32_t doff = (ja->header->curr_pos & JA_EPOS_MASK) << JA_ALIGN_WIDTH;
    if (doff + value_len + sizeof(grn_io_ja_ehead) <= (1 << JA_DSEG_WIDTH)) {
      return grn_success;
    }
  }
  {
    uint32_t max_dsegs = 1 << (32 - JA_EPOS_WIDTH);
    /* if (value_len <= JA_DSEG_MASK) { todo : } else */
    {
      uint32_t i, j;
      uint32_t nseg = (value_len + sizeof(grn_io_ja_ehead) + JA_DSEG_MASK) >> JA_DSEG_WIDTH;
      for (i = 0, j = 0; i < max_dsegs; i++) {
        if (ja->dsegs[i] == 0) {
          if (++j == nseg) {
            ja->header->curr_pos = (i + 1 - j) << JA_EPOS_WIDTH;
            return grn_success;
          }
        } else {
          j = 0;
        }
      }
    }
    return grn_other_error;
  }
}

inline static grn_rc
grn_ja_put_raw(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, int value_len, int flags)
{
  grn_rc rc = grn_success;
  uint32_t newpos = 0;
  grn_io_ja_einfo *ep;
  {
    uint32_t lseg = id >> JA_ESEG_WIDTH;
    uint32_t lpos = id & JA_ESEG_MASK;
    uint32_t *pseg = &ja->esegs[lseg];
    if (*pseg == SEG_NOT_ASSIGNED && (rc = assign_eseg(ja, lseg))) { return rc; }
    GRN_IO_SEG_MAP(ja->io, *pseg, ep);
    if (!ep) { return grn_other_error; }
    ep += lpos;
  }
  if (value_len) {
    uint32_t dseg, doff, inc;
    if ((rc = assign_rec(ja, value_len))) { return rc; }
    newpos = ja->header->curr_pos;
    dseg = newpos >> JA_EPOS_WIDTH;
    doff = (newpos & JA_EPOS_MASK) << JA_ALIGN_WIDTH;
    rc = grn_io_write_ja(ja->io, ctx, id, dseg, doff, value, value_len);
    if (rc) { return rc; }
    inc = ((value_len + sizeof(grn_io_ja_ehead) + JA_ALIGN_MASK) >> JA_ALIGN_WIDTH);
    ja->header->curr_pos = ((newpos + inc) & JA_EPOS_MASK) ? (newpos + inc) : 0;
    {
      uint32_t max = 1U << JA_EPOS_WIDTH;
      while (ja->dsegs[dseg] + inc > max) {
        inc -= max - ja->dsegs[dseg];
        ja->dsegs[dseg++] = max;
      }
      ja->dsegs[dseg] += inc;
    }
  }
  {
    grn_io_ja_einfo ne, oe;
    oe = *ep;
    ne.pos = newpos;
    ne.size = value_len;
    *ep = ne; // atomic_set64 maybe more suitable.
    if (oe.size) {
      uint32_t dseg, off, dec, max = 1U << JA_EPOS_WIDTH;
      dseg = oe.pos >> JA_EPOS_WIDTH;
      off = (oe.pos & JA_EPOS_MASK);
      dec = ((oe.size + sizeof(grn_io_ja_ehead) + JA_ALIGN_MASK) >> JA_ALIGN_WIDTH);
      if (off + dec > max) {
        dec -= max - off;
        ja->dsegs[dseg++] -= max - off;
        while (dec > max) {
          dec -= max;
          ja->dsegs[dseg++] = 0;
        }
        {
          uint32_t vsize = (dec << JA_ALIGN_WIDTH) - sizeof(grn_io_ja_ehead);
          rc = grn_io_write_ja_ehead(ja->io, ctx, 0, dseg, 0, vsize);
          // todo : handle rc..
        }
      }
      ja->dsegs[dseg] -= dec;
    }
  }
  return rc;
}

#ifndef NO_ZLIB
inline static grn_rc
grn_ja_put_zlib(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, int value_len, int flags)
{
  grn_rc rc;
  grn_ctx *ctx = &grn_gctx; /* todo : replace it with the local ctx */
  z_stream zstream;
  void *zvalue;
  int zvalue_len;
  zstream.next_in = value;
  zstream.avail_in = value_len;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  if (deflateInit2(&zstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 /* windowBits */,
                   8 /* memLevel */, Z_DEFAULT_STRATEGY) != Z_OK) { return grn_other_error; }
  zvalue_len = deflateBound(&zstream, value_len);
  if (!(zvalue = GRN_MALLOC(zvalue_len + sizeof (uint64_t)))) { deflateEnd(&zstream); return grn_memory_exhausted; }
  zstream.next_out = (Bytef *)((uint64_t *)zvalue + 1);
  zstream.avail_out = zvalue_len;
  if (deflate(&zstream, Z_FINISH) != Z_STREAM_END) {
    deflateEnd(&zstream);
    GRN_FREE(zvalue);
    return grn_other_error;
  }
  zvalue_len = zstream.total_out;
  if (deflateEnd(&zstream) != Z_OK) { GRN_FREE(zvalue); return grn_other_error; }
  *(uint64_t *)zvalue = value_len;
  rc = grn_ja_put_raw(ctx, ja, id, zvalue, zvalue_len + sizeof (uint64_t), flags);
  GRN_FREE(zvalue);
  return rc;
}
#endif /* NO_ZLIB */

#ifndef NO_LZO
inline static grn_rc
grn_ja_put_lzo(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, int value_len, int flags)
{
  grn_rc rc;
  grn_ctx *ctx = &grn_gctx; /* todo : replace it with the local ctx */
  void *lvalue, *lwork;
  lzo_uint lvalue_len = value_len + value_len / 16 + 64 + 3;
  if (!(lvalue = GRN_MALLOC(lvalue_len + sizeof (uint64_t)))) { return grn_memory_exhausted; }
  if (!(lwork = GRN_MALLOC(LZO1X_1_MEM_COMPRESS))) { GRN_FREE(lvalue); return grn_memory_exhausted; }
  if (lzo1x_1_compress(value, value_len, (lzo_bytep)((uint64_t *)lvalue + 1), &lvalue_len, lwork) != LZO_E_OK) {
    GRN_FREE(lwork);
    GRN_FREE(lvalue);
    return grn_other_error;
  }
  GRN_FREE(lwork);
  *(uint64_t *)lvalue = value_len;
  rc = grn_ja_put_raw(ctx, ja, id, lvalue, lvalue_len + sizeof (uint64_t), flags);
  GRN_FREE(lvalue);
  return rc;
}
#endif /* NO_LZO */

grn_rc
grn_ja_put(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, int value_len, int flags)
{
#ifndef NO_ZLIB
  if (ja->header->flags & GRN_OBJ_COMPRESS_ZLIB) {
    return grn_ja_put_zlib(ctx, ja, id, value, value_len, flags);
  }
#endif /* NO_ZLIB */
#ifndef NO_LZO
  if (ja->header->flags & GRN_OBJ_COMPRESS_LZO) {
    return grn_ja_put_lzo(ctx, ja, id, value, value_len, flags);
  }
#endif /* NO_LZO */
  return grn_ja_put_raw(ctx, ja, id, value, value_len, flags);
}

static grn_rc
grn_ja_defrag_seg(grn_ctx *ctx, grn_ja *ja, uint32_t dseg)
{
  grn_rc rc = grn_success;
  grn_io_win iw;
  uint32_t epos = dseg << JA_EPOS_WIDTH;
  uint32_t segsize = (1 << JA_DSEG_WIDTH);
  byte *v = grn_io_win_map(ja->io, ctx, &iw, dseg, 0, segsize, grn_io_rdonly);
  byte *ve = v + segsize;
  uint32_t *segusage = &ja->dsegs[dseg];
  if (!v) { return grn_internal_error; }
  while (v < ve && *segusage) {
    grn_io_ja_ehead *eh = (grn_io_ja_ehead *) v;
    uint32_t inc = ((eh->size + sizeof(grn_io_ja_ehead) + JA_ALIGN_MASK) >> JA_ALIGN_WIDTH);
    grn_io_ja_einfo *ep;
    uint32_t lseg = eh->key >> JA_ESEG_WIDTH;
    uint32_t lpos = eh->key & JA_ESEG_MASK;
    uint32_t *pseg = &ja->esegs[lseg];
    if (*pseg == SEG_NOT_ASSIGNED) {
      GRN_LOG(grn_log_error, "ep is not assigned (%x)", lseg);
      rc = grn_internal_error;
      goto exit;
    }
    GRN_IO_SEG_MAP(ja->io, *pseg, ep);
    if (!ep) {
      GRN_LOG(grn_log_error, "ep map failed (%x)", lseg);
      rc = grn_internal_error;
      goto exit;
    }
    ep += lpos;
    if (ep->pos == epos) {
      if (ep->size != eh->size) {
        GRN_LOG(grn_log_error, "ep->size(%d) != eh->size(%d)", ep->size, eh->size);
        rc = grn_internal_error;
        goto exit;
      }
      if ((rc = grn_ja_put(ctx, ja, eh->key, v + sizeof(grn_io_ja_ehead), eh->size, 0))) {
        goto exit;
      }
    }
    epos += inc;
    v += (inc << JA_ALIGN_WIDTH);
  }
  if (*segusage) {
    GRN_LOG(grn_log_error, "ja->dsegs[dseg] = %d after defrag", ja->dsegs[dseg]);
    rc = grn_internal_error;
  }
exit :
  grn_io_win_unmap(&iw);
  return rc;
}

int
grn_ja_defrag(grn_ctx *ctx, grn_ja *ja, int threshold)
{
  int nsegs = 0;
  uint32_t ts = 1U << (JA_EPOS_WIDTH - threshold);
  uint32_t dseg, max_dsegs = 1 << (32 - JA_EPOS_WIDTH);
  for (dseg = 0; dseg < max_dsegs; dseg++) {
    if (dseg == ja->header->curr_pos >> JA_EPOS_WIDTH) { continue; }
    if (ja->dsegs[dseg] && ja->dsegs[dseg] < ts) {
      if (!grn_ja_defrag_seg(ctx, ja, dseg)) { nsegs++; }
    }
  }
  return nsegs;
}

#endif /* USE_JA01 */

/**** vgram ****/

/*

static int len_sum = 0;
static int img_sum = 0;
static int simple_sum = 0;
static int skip_sum = 0;

grn_vgram *
grn_vgram_create(const char *path)
{
  grn_vgram *s;
  if (!(s = GRN_GMALLOC(sizeof(grn_vgram)))) { return NULL; }
  s->vgram = grn_sym_create(path, sizeof(grn_id) * 2, 0, grn_enc_none);
  if (!s->vgram) {
    GRN_GFREE(s);
    return NULL;
  }
  return s;
}

grn_vgram *
grn_vgram_open(const char *path)
{
  grn_vgram *s;
  if (!(s = GRN_GMALLOC(sizeof(grn_vgram)))) { return NULL; }
  s->vgram = grn_sym_open(path);
  if (!s->vgram) {
    GRN_GFREE(s);
    return NULL;
  }
  return s;
}

grn_vgram_buf *
grn_vgram_buf_open(size_t len)
{
  grn_vgram_buf *b;
  if (!(b = GRN_GMALLOC(sizeof(grn_vgram_buf)))) { return NULL; }
  b->len = len;
  b->tvs = b->tvp = GRN_GMALLOC(sizeof(grn_id) * len);
  if (!b->tvp) { GRN_GFREE(b); return NULL; }
  b->tve = b->tvs + len;
  b->vps = b->vpp = GRN_GMALLOC(sizeof(grn_vgram_vnode) * len * 2);
  if (!b->vpp) { GRN_GFREE(b->tvp); GRN_GFREE(b); return NULL; }
  b->vpe = b->vps + len;
  return b;
}

grn_rc
grn_vgram_buf_add(grn_vgram_buf *b, grn_id tid)
{
  uint8_t dummybuf[8], *dummyp;
  if (b->tvp < b->tve) { *b->tvp++ = tid; }
  dummyp = dummybuf;
  GRN_B_ENC(tid, dummyp);
  simple_sum += dummyp - dummybuf;
  return grn_success;
}

typedef struct {
  grn_id vid;
  grn_id tid;
} vgram_key;

grn_rc
grn_vgram_update(grn_vgram *vgram, grn_id rid, grn_vgram_buf *b, grn_hash *terms)
{
  grn_inv_updspec **u;
  if (b && b->tvs < b->tvp) {
    grn_id *t0, *tn;
    for (t0 = b->tvs; t0 < b->tvp - 1; t0++) {
      grn_vgram_vnode *v, **vp;
      if (grn_set_at(terms, t0, (void **) &u)) {
        vp = &(*u)->vnodes;
        for (tn = t0 + 1; tn < b->tvp; tn++) {
          for (v = *vp; v && v->tid != *tn; v = v->cdr) ;
          if (!v) {
            if (b->vpp < b->vpe) {
              v = b->vpp++;
            } else {
              // todo;
              break;
            }
            v->car = NULL;
            v->cdr = *vp;
            *vp = v;
            v->tid = *tn;
            v->vid = 0;
            v->freq = 0;
            v->len = tn - t0;
          }
          v->freq++;
          if (v->vid) {
            vp = &v->car;
          } else {
            break;
          }
        }
      }
    }
    {
      grn_set *th = grn_set_open(sizeof(grn_id), sizeof(int), 0);
      if (!th) { return grn_memory_exhausted; }
      if (t0 == b->tvp) { GRN_LOG(grn_log_debug, "t0 == tvp"); }
      for (t0 = b->tvs; t0 < b->tvp; t0++) {
        grn_id vid, vid0 = *t0, vid1 = 0;
        grn_vgram_vnode *v, *v2 = NULL, **vp;
        if (grn_set_at(terms, t0, (void **) &u)) {
          vp = &(*u)->vnodes;
          for (tn = t0 + 1; tn < b->tvp; tn++) {
            for (v = *vp; v; v = v->cdr) {
              if (!v->vid && (v->freq < 2 || v->freq * v->len < 4)) {
                *vp = v->cdr;
                v->freq = 0;
              }
              if (v->tid == *tn) { break; }
              vp = &v->cdr;
            }
            if (v) {
              if (v->freq) {
                v2 = v;
                vid1 = vid0;
                vid0 = v->vid;
              }
              if (v->vid) {
                vp = &v->car;
                continue;
              }
            }
            break;
          }
        }
        if (v2) {
          if (!v2->vid) {
            vgram_key key;
            key.vid = vid1;
            key.tid = v2->tid;
            if (!(v2->vid = grn_sym_get(vgram->vgram, (char *)&key))) {
              grn_set_close(th);
              return grn_memory_exhausted;
            }
          }
          vid = *t0 = v2->vid * 2 + 1;
          memset(t0 + 1, 0, sizeof(grn_id) * v2->len);
          t0 += v2->len;
        } else {
          vid = *t0 *= 2;
        }
        {
          int *tf;
          if (!grn_set_get(th, &vid, (void **) &tf)) {
            grn_set_close(th);
            return grn_memory_exhausted;
          }
          (*tf)++;
        }
      }
      if (!th->n_entries) { GRN_LOG(grn_log_debug, "th->n_entries == 0"); }
      {
        int j = 0;
        int skip = 0;
        grn_set_eh *ehs, *ehp, *ehe;
        grn_set_sort_optarg arg;
        uint8_t *ps = GRN_GMALLOC(b->len * 2), *pp, *pe;
        if (!ps) {
          grn_set_close(th);
          return grn_memory_exhausted;
        }
        pp = ps;
        pe = ps + b->len * 2;
        arg.mode = grn_sort_descending;
        arg.compar = NULL;
        arg.compar_arg = (void *)(intptr_t)sizeof(grn_id);
        ehs = grn_set_sort(th, 0, &arg);
        if (!ehs) {
          GRN_GFREE(ps);
          grn_set_close(th);
          return grn_memory_exhausted;
        }
        GRN_B_ENC(th->n_entries, pp);
        for (ehp = ehs, ehe = ehs + th->n_entries; ehp < ehe; ehp++, j++) {
          int *id = (int *)GRN_SET_INTVAL(*ehp);
          GRN_B_ENC(*GRN_SET_INTKEY(*ehp), pp);
          *id = j;
        }
        for (t0 = b->tvs; t0 < b->tvp; t0++) {
          if (*t0) {
            int *id;
            if (!grn_set_at(th, t0, (void **) &id)) {
              GRN_LOG(grn_log_error, "lookup error (%d)", *t0);
            }
            GRN_B_ENC(*id, pp);
          } else {
            skip++;
          }
        }
        len_sum += b->len;
        img_sum += pp - ps;
        skip_sum += skip;
        GRN_GFREE(ehs);
        GRN_GFREE(ps);
      }
      grn_set_close(th);
    }
  }
  return grn_success;
}

grn_rc
grn_vgram_buf_close(grn_vgram_buf *b)
{
  if (!b) { return grn_invalid_argument; }
  if (b->tvs) { GRN_GFREE(b->tvs); }
  if (b->vps) { GRN_GFREE(b->vps); }
  GRN_GFREE(b);
  return grn_success;
}

grn_rc
grn_vgram_close(grn_vgram *vgram)
{
  if (!vgram) { return grn_invalid_argument; }
  GRN_LOG(grn_log_debug, "len=%d img=%d skip=%d simple=%d", len_sum, img_sum, skip_sum, simple_sum);
  grn_sym_close(vgram->vgram);
  GRN_GFREE(vgram);
  return grn_success;
}
*/
