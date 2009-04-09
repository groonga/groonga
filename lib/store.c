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
    GRN_LOG(ctx, GRN_LOG_ERROR, "element_size too large (%d)", element_size);
    return NULL;
  }
  for (actual_size = 1; actual_size < element_size; actual_size *= 2) ;
  max_segments = ((GRN_ID_MAX + 1) / GRN_RA_SEGMENT_SIZE) * actual_size;
  io = grn_io_create(ctx, path, sizeof(struct grn_ra_header),
                     GRN_RA_SEGMENT_SIZE, max_segments, grn_io_auto,
                     GRN_IO_EXPIRE_SEGMENT);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  grn_io_set_type(io, GRN_COLUMN_FIX_SIZE);
  header->element_size = actual_size;
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
  if (grn_io_get_type(io) != GRN_COLUMN_FIX_SIZE) {
    ERR(GRN_INVALID_FORMAT, "file type unmatch");
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
grn_ra_info(grn_ctx *ctx, grn_ra *ra, unsigned int *element_size)
{
  if (!ra) { return GRN_INVALID_ARGUMENT; }
  if (element_size) { *element_size = ra->header->element_size; }
  return GRN_SUCCESS;
}

grn_rc
grn_ra_close(grn_ctx *ctx, grn_ra *ra)
{
  grn_rc rc;
  if (!ra) { return GRN_INVALID_ARGUMENT; }
  rc = grn_io_close(ctx, ra->io);
  GRN_GFREE(ra);
  return rc;
}

grn_rc
grn_ra_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return GRN_INVALID_ARGUMENT; }
  return grn_io_remove(ctx, path);
}

void *
grn_ra_ref(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  void *p;
  uint16_t seg;
  if (id > GRN_ID_MAX) { return NULL; }
  seg = id >> ra->element_width;
  GRN_IO_SEG_REF(ra->io, seg, p);
  if (!p) { return NULL; }
  return (void *)(((byte *)p) + ((id & ra->element_mask) * ra->header->element_size));
}

grn_rc
grn_ra_unref(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  uint16_t seg;
  if (id > GRN_ID_MAX) { return GRN_INVALID_ARGUMENT; }
  seg = id >> ra->element_width;
  GRN_IO_SEG_UNREF(ra->io, seg);
  return GRN_SUCCESS;
}

/**** jagged arrays ****/

#define GRN_JA_W_SEGREGATE_THRESH      7
#define GRN_JA_W_CAPACITY              38
#define GRN_JA_W_SEGMENT               22

#define JA_ESEG_VOID                   (0xffffffffU)
#define JA_SEGMENT_SIZE                (1U << GRN_JA_W_SEGMENT)
#define JA_W_EINFO                     3
#define JA_W_SEGMENTS_MAX              (GRN_JA_W_CAPACITY - GRN_JA_W_SEGMENT)
#define JA_W_EINFO_IN_A_SEGMENT        (GRN_JA_W_SEGMENT - JA_W_EINFO)
#define JA_N_EINFO_IN_A_SEGMENT        (1U << JA_W_EINFO_IN_A_SEGMENT)
#define JA_M_EINFO_IN_A_SEGMENT        (JA_N_EINFO_IN_A_SEGMENT - 1)
#define JA_N_GARBAGES_IN_A_SEGMENT     ((1U << (GRN_JA_W_SEGMENT - 3)) - 2)
#define JA_N_ELEMENT_VARIATION         (GRN_JA_W_SEGREGATE_THRESH - JA_W_EINFO + 1)
#define JA_N_DSEGMENTS                 (1U << JA_W_SEGMENTS_MAX)
#define JA_N_ESEGMENTS                 (1U << (GRN_ID_WIDTH - JA_W_EINFO_IN_A_SEGMENT))

typedef struct _grn_ja_einfo grn_ja_einfo;

struct _grn_ja_einfo {
  union {
    struct {
      uint16_t seg;
      uint16_t pos;
      uint16_t size;
      uint8_t c1;
      uint8_t c2;
    } n;
    struct {
      uint32_t size;
      uint16_t seg;
      uint8_t c1;
      uint8_t c2;
    } h;
    uint8_t c[8];
  } u;
};

#define ETINY (0x80)
#define EHUGE (0x40)
#define ETINY_P(e) ((e)->u.c[7] & ETINY)
#define ETINY_ENC(e,_size) ((e)->u.c[7] = (_size) + ETINY)
#define ETINY_DEC(e,_size) ((_size) = (e)->u.c[7] & ~(ETINY|EHUGE))
#define EHUGE_P(e) ((e)->u.c[7] & EHUGE)
#define EHUGE_ENC(e,_seg,_size) {\
  (e)->u.h.c1 = 0;\
  (e)->u.h.c2 = EHUGE;\
  (e)->u.h.seg = (_seg);\
  (e)->u.h.size = (_size);\
}
#define EHUGE_DEC(e,_seg,_size) {\
  (_seg) = (e)->u.h.seg;\
  (_size) = (e)->u.h.size;\
}
#define EINFO_ENC(e,_seg,_pos,_size) {\
  (e)->u.n.c1 = (_pos) >> 16;\
  (e)->u.n.c2 = ((_size) >> 16);\
  (e)->u.n.seg = (_seg);\
  (e)->u.n.pos = (_pos);\
  (e)->u.n.size = (_size);\
}
#define EINFO_DEC(e,_seg,_pos,_size) {\
  (_seg) = (e)->u.n.seg;\
  (_pos) = ((e)->u.n.c1 << 16) + (e)->u.n.pos;\
  (_size) = ((e)->u.n.c2 << 16) + (e)->u.n.size;\
}

typedef struct {
  uint32_t seg;
  uint32_t pos;
} ja_pos;

typedef struct {
  uint32_t head;
  uint32_t tail;
  uint32_t nrecs;
  uint32_t next;
  ja_pos recs[JA_N_GARBAGES_IN_A_SEGMENT];
} grn_ja_ginfo;

struct grn_ja_header {
  uint32_t flags;
  uint32_t curr_seg;
  uint32_t curr_pos;
  uint32_t max_element_size;
  ja_pos free_elements[JA_N_ELEMENT_VARIATION];
  uint32_t garbages[JA_N_ELEMENT_VARIATION];
  uint32_t ngarbages[JA_N_ELEMENT_VARIATION];
  uint32_t dsegs[JA_N_DSEGMENTS];
  uint32_t esegs[JA_N_ESEGMENTS];
};

#define SEG_SEQ        (0x10000000U)
#define SEG_HUGE       (0x20000000U)
#define SEG_EINFO      (0x30000000U)
#define SEG_GINFO      (0x40000000U)
#define SEG_MASK       (0xf0000000U)

#define SEGMENTS_AT(ja,seg) ((ja)->header->dsegs[seg])
#define SEGMENTS_SEGRE_ON(ja,seg,width) (SEGMENTS_AT(ja,seg) = width)
#define SEGMENTS_SEQ_ON(ja,seg) (SEGMENTS_AT(ja,seg) = SEG_SEQ)
#define SEGMENTS_HUGE_ON(ja,seg) (SEGMENTS_AT(ja,seg) = SEG_HUGE)
#define SEGMENTS_EINFO_ON(ja,seg,lseg) (SEGMENTS_AT(ja,seg) = SEG_EINFO|(lseg))
#define SEGMENTS_GINFO_ON(ja,seg,width) (SEGMENTS_AT(ja,seg) = SEG_GINFO|(width))
#define SEGMENTS_OFF(ja,seg) (SEGMENTS_AT(ja,seg) = 0)

grn_ja *
grn_ja_create(grn_ctx *ctx, const char *path, unsigned int max_element_size, uint32_t flags)
{
  int i;
  grn_io *io;
  grn_ja *ja = NULL;
  struct grn_ja_header *header;
  io = grn_io_create(ctx, path, sizeof(struct grn_ja_header),
                     JA_SEGMENT_SIZE, JA_N_DSEGMENTS, grn_io_auto,
                     GRN_IO_EXPIRE_SEGMENT);
  if (!io) { return NULL; }
  grn_io_set_type(io, GRN_COLUMN_VAR_SIZE);
  header = grn_io_header(io);
  header->curr_pos = JA_SEGMENT_SIZE;
  header->flags = flags;
  for (i = 0; i < JA_N_ESEGMENTS; i++) { header->esegs[i] = JA_ESEG_VOID; }
  if (!(ja = GRN_GMALLOC(sizeof(grn_ja)))) {
    grn_io_close(ctx, io);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ja, GRN_COLUMN_VAR_SIZE);
  ja->io = io;
  ja->header = header;
  header->max_element_size = max_element_size;
  SEGMENTS_EINFO_ON(ja, 0, 0);
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
  if (grn_io_get_type(io) != GRN_COLUMN_VAR_SIZE) {
    ERR(GRN_INVALID_FORMAT, "file type unmatch");
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
  if (!ja) { return GRN_INVALID_ARGUMENT; }
  if (max_element_size) { *max_element_size = ja->header->max_element_size; }
  return GRN_SUCCESS;
}

grn_rc
grn_ja_close(grn_ctx *ctx, grn_ja *ja)
{
  grn_rc rc;
  if (!ja) { return GRN_INVALID_ARGUMENT; }
  rc = grn_io_close(ctx, ja->io);
  GRN_GFREE(ja);
  return rc;
}

grn_rc
grn_ja_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return GRN_INVALID_ARGUMENT; }
  return grn_io_remove(ctx, path);
}

static void *
grn_ja_ref_raw(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  uint32_t pseg = ja->header->esegs[id >> JA_W_EINFO_IN_A_SEGMENT];
  iw->size = 0;
  iw->addr = NULL;
  iw->pseg = pseg;
  if (pseg != JA_ESEG_VOID) {
    grn_ja_einfo *einfo;
    GRN_IO_SEG_REF(ja->io, pseg, einfo);
    if (einfo) {
      grn_ja_einfo *ei = &einfo[id & JA_M_EINFO_IN_A_SEGMENT];
      if (ETINY_P(ei)) {
        iw->tiny_p = 1;
        ETINY_DEC(ei, iw->size);
        iw->io = ja->io;
        iw->ctx = ctx;
        iw->addr = (void *)ei;
      } else {
        uint32_t jag, vpos, vsize;
        iw->tiny_p = 0;
        if (EHUGE_P(ei)) {
          EHUGE_DEC(ei, jag, vsize);
          vpos = 0;
        } else {
          EINFO_DEC(ei, jag, vpos, vsize);
        }
        grn_io_win_map2(ja->io, ctx, iw, jag, vpos, vsize, grn_io_rdonly);
      }
      if (!iw->addr) { GRN_IO_SEG_UNREF(ja->io, pseg); }
    }
  }
  *value_len = iw->size;
  return iw->addr;
}

grn_rc
grn_ja_unref(grn_ctx *ctx, grn_io_win *iw)
{
  if (!iw->addr) { return GRN_INVALID_ARGUMENT; }
  GRN_IO_SEG_UNREF(iw->io, iw->pseg);
  if (!iw->tiny_p) { grn_io_win_unmap2(iw); }
  return GRN_SUCCESS;
}

#define DELETED 0x80000000

static grn_rc
grn_ja_free(grn_ctx *ctx, grn_ja *ja, grn_ja_einfo *einfo)
{
  grn_ja_ginfo *ginfo;
  uint32_t seg, pos, element_size, aligned_size, m, *gseg;
  if (ETINY_P(einfo)) { return GRN_SUCCESS; }
  if (EHUGE_P(einfo)) {
    uint32_t n;
    EHUGE_DEC(einfo, seg, element_size);
    n = ((element_size + JA_SEGMENT_SIZE - 1) >> GRN_JA_W_SEGMENT);
    for (; n--; seg++) { SEGMENTS_OFF(ja, seg); }
    return GRN_SUCCESS;
  }
  EINFO_DEC(einfo, seg, pos, element_size);
  if (!element_size) { return GRN_SUCCESS; }
  {
    int es = element_size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
  }
  if (m > GRN_JA_W_SEGREGATE_THRESH) {
    byte *addr;
    GRN_IO_SEG_REF(ja->io, seg, addr);
    if (!addr) { return GRN_NO_MEMORY_AVAILABLE; }
    aligned_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
    *(uint32_t *)(addr + pos - sizeof(grn_id)) = DELETED|aligned_size;
    SEGMENTS_AT(ja, seg) -= (aligned_size + sizeof(grn_id));
    if (SEGMENTS_AT(ja, seg) == SEG_SEQ) {
      /* reuse the segment */
      SEGMENTS_AT(ja, seg) = 0;
    }
    GRN_IO_SEG_UNREF(ja->io, seg);
  } else {
    uint32_t lseg = 0, lseg_;
    gseg = &ja->header->garbages[m - JA_W_EINFO];
    while ((lseg_ = *gseg)) {
      if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
      GRN_IO_SEG_REF(ja->io, lseg_, ginfo);
      if (!ginfo) { return GRN_NO_MEMORY_AVAILABLE; }
      lseg = lseg_;
      if (ginfo->nrecs < JA_N_GARBAGES_IN_A_SEGMENT) { break; }
      gseg = &ginfo->next;
    }
    if (!lseg_) {
      uint32_t i = 0;
      while (SEGMENTS_AT(ja, i)) {
        if (++i >= JA_N_DSEGMENTS) {
          if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
          return GRN_NO_MEMORY_AVAILABLE;
        }
      }
      SEGMENTS_GINFO_ON(ja, i, m - JA_W_EINFO);
      *gseg = i;
      lseg_ = *gseg;
      if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
      GRN_IO_SEG_REF(ja->io, lseg_, ginfo);
      lseg = lseg_;
      if (!ginfo) { return GRN_NO_MEMORY_AVAILABLE; }
      ginfo->head = 0;
      ginfo->tail = 0;
      ginfo->nrecs = 0;
      ginfo->next = 0;
    }
    ginfo->recs[ginfo->head].seg = seg;
    ginfo->recs[ginfo->head].pos = pos;
    if (++ginfo->head == JA_N_GARBAGES_IN_A_SEGMENT) { ginfo->head = 0; }
    ginfo->nrecs++;
    ja->header->ngarbages[m - JA_W_EINFO]++;
    if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ja_replace(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_ja_einfo *ei)
{
  uint32_t lseg, *pseg, pos;
  grn_ja_einfo *einfo, eback;
  lseg = id >> JA_W_EINFO_IN_A_SEGMENT;
  pos = id & JA_M_EINFO_IN_A_SEGMENT;
  pseg = &ja->header->esegs[lseg];
  if (*pseg == JA_ESEG_VOID) {
    int i = 0;
    while (SEGMENTS_AT(ja, i)) {
      if (++i >= JA_N_DSEGMENTS) { return GRN_NO_MEMORY_AVAILABLE; }
    }
    SEGMENTS_EINFO_ON(ja, i, lseg);
    *pseg = i;
  }
  GRN_IO_SEG_REF(ja->io, *pseg, einfo);
  if (!einfo) { return GRN_NO_MEMORY_AVAILABLE; }
  eback = einfo[pos];
  GRN_SET_64BIT(&einfo[pos], *ei);
  GRN_IO_SEG_UNREF(ja->io, *pseg);
  grn_ja_free(ctx, ja, &eback);
  return GRN_SUCCESS;
}

#define JA_N_GARBAGES_TH 10

// todo : grn_io_win_map2 cause verbose copy when nseg > 1, it should be copied directly.
static grn_rc
grn_ja_alloc(grn_ctx *ctx, grn_ja *ja, grn_id id,
             uint32_t element_size, grn_ja_einfo *einfo, grn_io_win *iw)
{
  byte *addr;
  iw->io = ja->io;
  iw->ctx = ctx;
  iw->cached = 1;
  if (element_size < 8) {
    ETINY_ENC(einfo, element_size);
    iw->tiny_p = 1;
    iw->addr = (void *)einfo;
    return GRN_SUCCESS;
  }
  iw->tiny_p = 0;
  if (element_size + sizeof(grn_id) > JA_SEGMENT_SIZE) {
    int i, j, n = (element_size + JA_SEGMENT_SIZE - 1) >> GRN_JA_W_SEGMENT;
    for (i = 0, j = -1; i < JA_N_DSEGMENTS; i++) {
      if (SEGMENTS_AT(ja, i)) {
        j = i;
      } else {
        if (i == j + n) {
          j++;
          addr = grn_io_win_map2(ja->io, ctx, iw, j, 0, element_size, grn_io_wronly);
          if (!addr) { return GRN_NO_MEMORY_AVAILABLE; }
          EHUGE_ENC(einfo, j, element_size);
          for (; j <= i; j++) { SEGMENTS_HUGE_ON(ja, j); }
          return GRN_SUCCESS;
        }
      }
    }
    GRN_LOG(ctx, GRN_LOG_CRIT, "ja full. requested element_size=%d.", element_size);
    return GRN_NO_MEMORY_AVAILABLE;
  } else {
    ja_pos *vp;
    int m, aligned_size, es = element_size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
    if (m > GRN_JA_W_SEGREGATE_THRESH) {
      uint32_t seg = ja->header->curr_seg, pos = ja->header->curr_pos;
      if (pos + element_size + sizeof(grn_id) > JA_SEGMENT_SIZE) {
        seg = 0;
        while (SEGMENTS_AT(ja, seg)) {
          if (++seg >= JA_N_DSEGMENTS) { return GRN_NO_MEMORY_AVAILABLE; }
        }
        SEGMENTS_SEQ_ON(ja, seg);
        ja->header->curr_seg = seg;
        pos = 0;
      }
      GRN_IO_SEG_REF(ja->io, seg, addr);
      if (!addr) { return GRN_NO_MEMORY_AVAILABLE; }
      *(grn_id *)(addr + pos) = id;
      aligned_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
      SEGMENTS_AT(ja, seg) += aligned_size + sizeof(grn_id);
      pos += sizeof(grn_id);
      EINFO_ENC(einfo, seg, pos, element_size);
      iw->segment = seg;
      iw->addr = addr + pos;
      ja->header->curr_pos = pos + aligned_size;
      return GRN_SUCCESS;
    } else {
      uint32_t lseg = 0, lseg_;
      aligned_size = 1 << m;
      if (ja->header->ngarbages[m - JA_W_EINFO] > JA_N_GARBAGES_TH) {
        grn_ja_ginfo *ginfo;
        uint32_t seg, pos, *gseg;
        gseg = &ja->header->garbages[m - JA_W_EINFO];
        while ((lseg_ = *gseg)) {
          GRN_IO_SEG_REF(ja->io, lseg_, ginfo);
          if (!ginfo) {
            if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
            return GRN_NO_MEMORY_AVAILABLE;
          }
          if (ginfo->next || ginfo->nrecs > JA_N_GARBAGES_TH) {
            seg = ginfo->recs[ginfo->tail].seg;
            pos = ginfo->recs[ginfo->tail].pos;
            GRN_IO_SEG_REF(ja->io, seg, addr);
            if (!addr) {
              if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
              return GRN_NO_MEMORY_AVAILABLE;
            }
            EINFO_ENC(einfo, seg, pos, element_size);
            iw->segment = seg;
            iw->addr = addr + pos;
            if (++ginfo->tail == JA_N_GARBAGES_IN_A_SEGMENT) { ginfo->tail = 0; }
            ginfo->nrecs--;
            ja->header->ngarbages[m - JA_W_EINFO]--;
            if (!ginfo->nrecs) {
              SEGMENTS_OFF(ja, *gseg);
              *gseg = ginfo->next;
            }
            if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
            return GRN_SUCCESS;
          }
          if (lseg) { GRN_IO_SEG_UNREF(ja->io, lseg); }
          lseg = lseg_;
          gseg = &ginfo->next;
        }
      }
      vp = &ja->header->free_elements[m - JA_W_EINFO];
      if (!vp->seg) {
        int i = 0;
        while (SEGMENTS_AT(ja, i)) {
          if (++i >= JA_N_DSEGMENTS) { return GRN_NO_MEMORY_AVAILABLE; }
        }
        SEGMENTS_SEGRE_ON(ja, i, m);
        vp->seg = i;
        vp->pos = 0;
      }
    }
    EINFO_ENC(einfo, vp->seg, vp->pos, element_size);
    GRN_IO_SEG_REF(ja->io, vp->seg, addr);
    if (!addr) { return GRN_NO_MEMORY_AVAILABLE; }
    iw->segment = vp->seg;
    iw->addr = addr + vp->pos;
    if ((vp->pos += aligned_size) == JA_SEGMENT_SIZE) {
      vp->seg = 0;
      vp->pos = 0;
    }
    return GRN_SUCCESS;
  }
}

grn_rc
grn_ja_put_raw(grn_ctx *ctx, grn_ja *ja, grn_id id,
               void *value, uint32_t value_len, int flags)
{
  int rc;
  grn_io_win iw;
  grn_ja_einfo einfo;
  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_APPEND :
    if (value_len) {
      grn_io_win jw;
      uint32_t old_len;
      void *oldvalue = grn_ja_ref(ctx, ja, id, &jw, &old_len);
      if (oldvalue) {
        if ((rc = grn_ja_alloc(ctx, ja, id, value_len + old_len, &einfo, &iw))) { return rc; }
        memcpy(iw.addr, oldvalue, old_len);
        memcpy((byte *)iw.addr + old_len, value, value_len);
        grn_ja_unref(ctx, &jw);
        grn_io_win_unmap2(&iw);
      } else {
        if ((rc = grn_ja_alloc(ctx, ja, id, value_len, &einfo, &iw))) { return rc; }
        memcpy(iw.addr, value, value_len);
        grn_io_win_unmap2(&iw);
      }
    }
    break;
  case GRN_OBJ_PREPEND :
    if (value_len) {
      grn_io_win jw;
      uint32_t old_len;
      void *oldvalue = grn_ja_ref(ctx, ja, id, &jw, &old_len);
      if (oldvalue) {
        if ((rc = grn_ja_alloc(ctx, ja, id, value_len + old_len, &einfo, &iw))) { return rc; }
        memcpy(iw.addr, value, value_len);
        memcpy((byte *)iw.addr + value_len, oldvalue, old_len);
        grn_ja_unref(ctx, &jw);
        grn_io_win_unmap2(&iw);
      } else {
        if ((rc = grn_ja_alloc(ctx, ja, id, value_len, &einfo, &iw))) { return rc; }
        memcpy(iw.addr, value, value_len);
        grn_io_win_unmap2(&iw);
      }
    }
    break;
  case GRN_OBJ_SET :
    if (value_len) {
      if ((rc = grn_ja_alloc(ctx, ja, id, value_len, &einfo, &iw))) { return rc; }
      // printf("put id=%d, value_len=%d value=%p ei=%p(%d:%d)\n", id, value_len, buf, &einfo, einfo.pos, einfo.tail[0]);
      memcpy(iw.addr, value, value_len);
      grn_io_win_unmap2(&iw);
    } else {
      memset(&einfo, 0, sizeof(grn_ja_einfo));
    }
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT, "grn_ja_put_raw called with illegal flags value");
    return ctx->rc;
  }
  return grn_ja_replace(ctx, ja, id, &einfo);
}

grn_rc
grn_ja_putv(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_obj *vector, int flags)
{
  grn_obj header, footer;
  grn_rc rc = GRN_SUCCESS;
  grn_section *vp;
  int i, f = 0, n = grn_vector_size(ctx, vector);
  GRN_OBJ_INIT(&header, GRN_BULK, 0);
  GRN_OBJ_INIT(&footer, GRN_BULK, 0);
  grn_bulk_benc(ctx, &header, n);
  for (i = 0, vp = vector->u.v.sections; i < n; i++, vp++) {
    grn_bulk_benc(ctx, &header, vp->length);
    if (vp->weight || vp->domain) { f = 1; }
  }
  if (f) {
    for (i = 0, vp = vector->u.v.sections; i < n; i++, vp++) {
      grn_bulk_benc(ctx, &footer, vp->weight);
      grn_bulk_benc(ctx, &footer, vp->domain);
    }
  }
  {
    grn_io_win iw;
    grn_ja_einfo einfo;
    grn_obj *body = vector->u.v.body;
    size_t sizeh = GRN_BULK_VSIZE(&header);
    size_t sizev = GRN_BULK_VSIZE(body);
    size_t sizef = GRN_BULK_VSIZE(&footer);
    if ((rc = grn_ja_alloc(ctx, ja, id, sizeh + sizev + sizef, &einfo, &iw))) { goto exit; }
    memcpy(iw.addr, GRN_BULK_HEAD(&header), sizeh);
    memcpy((char *)iw.addr + sizeh, GRN_BULK_HEAD(body), sizev);
    if (f) { memcpy((char *)iw.addr + sizeh + sizev, GRN_BULK_HEAD(&footer), sizef); }
    grn_io_win_unmap2(&iw);
    rc = grn_ja_replace(ctx, ja, id, &einfo);
  }
exit :
  GRN_OBJ_FIN(ctx, &footer);
  GRN_OBJ_FIN(ctx, &header);
  return rc;
}

uint32_t
grn_ja_size(grn_ctx *ctx, grn_ja *ja, grn_id id)
{
  grn_ja_einfo *einfo, *ei;
  uint32_t lseg, *pseg, pos, size;
  lseg = id >> JA_W_EINFO_IN_A_SEGMENT;
  pos = id & JA_M_EINFO_IN_A_SEGMENT;
  pseg = &ja->header->esegs[lseg];
  if (*pseg == JA_ESEG_VOID) {
    ctx->rc = GRN_INVALID_ARGUMENT;
    return 0;
  }
  GRN_IO_SEG_REF(ja->io, *pseg, einfo);
  if (!einfo) {
    ctx->rc = GRN_NO_MEMORY_AVAILABLE;
    return 0;
  }
  ei = &einfo[pos];
  if (ETINY_P(ei)) {
    ETINY_DEC(ei, size);
  } else {
    if (EHUGE_P(ei)) {
      size = ei->u.h.size;
    } else {
      size = (ei->u.n.c2 << 16) + ei->u.n.size;
    }
  }
  GRN_IO_SEG_UNREF(ja->io, *pseg);
  return size;
}

#ifndef NO_ZLIB
#include <zlib.h>

static void *
grn_ja_ref_zlib(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  z_stream zstream;
  void *value, *zvalue;
  uint32_t zvalue_len = *value_len + sizeof (uint64_t);
  if (!(zvalue = grn_ja_ref_raw(ctx, ja, id, iw, &zvalue_len))) {
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

static void *
grn_ja_ref_lzo(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  void *value, *lvalue;
  uint32_t lvalue_len = *value_len + sizeof (uint64_t);
  lzo_uint loutlen;
  if (!(lvalue = grn_ja_ref_raw(ctx, ja, id, iw, &lvalue_len))) {
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
grn_ja_ref(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
#ifndef NO_ZLIB
  if (ja->header->flags & GRN_OBJ_COMPRESS_ZLIB) {
    return grn_ja_ref_zlib(ctx, ja, id, iw, value_len);
  }
#endif /* NO_ZLIB */
#ifndef NO_LZO
  if (ja->header->flags & GRN_OBJ_COMPRESS_LZO) {
    return grn_ja_ref_lzo(ctx, ja, id, iw, value_len);
  }
#endif /* NO_LZO */
  return grn_ja_ref_raw(ctx, ja, id, iw, value_len);
}

#ifndef NO_ZLIB
inline static grn_rc
grn_ja_put_zlib(grn_ctx *ctx, grn_ja *ja, grn_id id,
                void *value, uint32_t value_len, int flags)
{
  grn_rc rc;
  z_stream zstream;
  void *zvalue;
  int zvalue_len;
  zstream.next_in = value;
  zstream.avail_in = value_len;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  if (deflateInit2(&zstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                   15 /* windowBits */,
                   8 /* memLevel */,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    ERR(GRN_ZLIB_ERROR, "deflateInit2 failed");
    return ctx->rc;
  }
  zvalue_len = deflateBound(&zstream, value_len);
  if (!(zvalue = GRN_MALLOC(zvalue_len + sizeof (uint64_t)))) { deflateEnd(&zstream); return GRN_NO_MEMORY_AVAILABLE; }
  zstream.next_out = (Bytef *)((uint64_t *)zvalue + 1);
  zstream.avail_out = zvalue_len;
  if (deflate(&zstream, Z_FINISH) != Z_STREAM_END) {
    deflateEnd(&zstream);
    GRN_FREE(zvalue);
    ERR(GRN_ZLIB_ERROR, "deflate failed");
    return ctx->rc;
  }
  zvalue_len = zstream.total_out;
  if (deflateEnd(&zstream) != Z_OK) {
    GRN_FREE(zvalue);
    ERR(GRN_ZLIB_ERROR, "deflateEnd failed");
    return ctx->rc;
  }
  *(uint64_t *)zvalue = value_len;
  rc = grn_ja_put_raw(ctx, ja, id, zvalue, zvalue_len + sizeof (uint64_t), flags);
  GRN_FREE(zvalue);
  return rc;
}
#endif /* NO_ZLIB */

#ifndef NO_LZO
inline static grn_rc
grn_ja_put_lzo(grn_ctx *ctx, grn_ja *ja, grn_id id,
               void *value, uint32_t value_len, int flags)
{
  grn_rc rc;
  void *lvalue, *lwork;
  lzo_uint lvalue_len = value_len + value_len / 16 + 64 + 3;
  if (!(lvalue = GRN_MALLOC(lvalue_len + sizeof (uint64_t)))) { return GRN_NO_MEMORY_AVAILABLE; }
  if (!(lwork = GRN_MALLOC(LZO1X_1_MEM_COMPRESS))) { GRN_FREE(lvalue); return GRN_NO_MEMORY_AVAILABLE; }
  if (lzo1x_1_compress(value, value_len, (lzo_bytep)((uint64_t *)lvalue + 1), &lvalue_len, lwork) != LZO_E_OK) {
    GRN_FREE(lwork);
    GRN_FREE(lvalue);
    ERR(GRN_LZO_ERROR, "lzo1x_1_compress");
    return ctx->rc;
  }
  GRN_FREE(lwork);
  *(uint64_t *)lvalue = value_len;
  rc = grn_ja_put_raw(ctx, ja, id, lvalue, lvalue_len + sizeof (uint64_t), flags);
  GRN_FREE(lvalue);
  return rc;
}
#endif /* NO_LZO */

grn_rc
grn_ja_put(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, uint32_t value_len, int flags)
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
grn_ja_defrag_seg(grn_ctx *ctx, grn_ja *ja, uint32_t seg)
{
  byte *v, *ve;
  uint32_t element_size, *segusage = &SEGMENTS_AT(ja,seg);
  GRN_IO_SEG_REF(ja->io, seg, v);
  if (!v) { return GRN_NO_MEMORY_AVAILABLE; }
  ve = v + JA_SEGMENT_SIZE;
  while (v < ve && (*segusage != SEG_SEQ)) {
    grn_id id = *((grn_id *)v);
    if (id & DELETED) {
      element_size = (id & ~DELETED);
    } else {
      element_size = grn_ja_size(ctx, ja, id) + sizeof(uint32_t);
      if (grn_ja_put(ctx, ja, id, v + sizeof(uint32_t), element_size, 0)) {
        return ctx->rc;
      }
      element_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
    }
    v += sizeof(uint32_t) + element_size;
  }
  if (*segusage != SEG_SEQ) {
    GRN_LOG(ctx, GRN_LOG_ERROR, "dseges[%d] = %d after defrag", seg, *segusage);
    return GRN_FILE_CORRUPT;
  }
  *segusage = 0;
  GRN_IO_SEG_UNREF(ja->io, seg);
  return GRN_SUCCESS;
}

int
grn_ja_defrag(grn_ctx *ctx, grn_ja *ja, int threshold)
{
  int nsegs = 0;
  uint32_t seg, ts = 1U << (GRN_JA_W_SEGMENT - threshold);
  for (seg = 0; seg < JA_N_DSEGMENTS; seg++) {
    if (seg == ja->header->curr_seg) { continue; }
    if (((SEGMENTS_AT(ja, seg) & SEG_MASK) == SEG_SEQ) &&
        ((SEGMENTS_AT(ja, seg) & ~SEG_MASK) < ts)) {
      if (!grn_ja_defrag_seg(ctx, ja, seg)) { nsegs++; }
    }
  }
  return nsegs;
}

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
  s->vgram = grn_sym_create(path, sizeof(grn_id) * 2, 0, GRN_ENC_NONE);
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
  return GRN_SUCCESS;
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
      if (!th) { return GRN_NO_MEMORY_AVAILABLE; }
      if (t0 == b->tvp) { GRN_LOG(ctx, GRN_LOG_DEBUG, "t0 == tvp"); }
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
              return GRN_NO_MEMORY_AVAILABLE;
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
            return GRN_NO_MEMORY_AVAILABLE;
          }
          (*tf)++;
        }
      }
      if (!th->n_entries) { GRN_LOG(ctx, GRN_LOG_DEBUG, "th->n_entries == 0"); }
      {
        int j = 0;
        int skip = 0;
        grn_set_eh *ehs, *ehp, *ehe;
        grn_set_sort_optarg arg;
        uint8_t *ps = GRN_GMALLOC(b->len * 2), *pp, *pe;
        if (!ps) {
          grn_set_close(th);
          return GRN_NO_MEMORY_AVAILABLE;
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
          return GRN_NO_MEMORY_AVAILABLE;
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
              GRN_LOG(ctx, GRN_LOG_ERROR, "lookup error (%d)", *t0);
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
  return GRN_SUCCESS;
}

grn_rc
grn_vgram_buf_close(grn_vgram_buf *b)
{
  if (!b) { return GRN_INVALID_ARGUMENT; }
  if (b->tvs) { GRN_GFREE(b->tvs); }
  if (b->vps) { GRN_GFREE(b->vps); }
  GRN_GFREE(b);
  return GRN_SUCCESS;
}

grn_rc
grn_vgram_close(grn_vgram *vgram)
{
  if (!vgram) { return GRN_INVALID_ARGUMENT; }
  GRN_LOG(ctx, GRN_LOG_DEBUG, "len=%d img=%d skip=%d simple=%d", len_sum, img_sum, skip_sum, simple_sum);
  grn_sym_close(vgram->vgram);
  GRN_GFREE(vgram);
  return GRN_SUCCESS;
}
*/
