/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2010 Brazil

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

#include <string.h>
#include <stdlib.h>
#include "geo.h"
#include "ii.h"
#include "db.h"
#include "pat.h"
#include "util.h"

typedef struct {
  grn_id id;
  double d;
} geo_entry;

typedef struct
{
  grn_geo_point key;
  int key_size;
} mesh_entry;

static int
compute_diff_bit(uint8_t *geo_key1, uint8_t *geo_key2)
{
  int i, j, diff_bit = 8;

  for (i = 0; i < sizeof(grn_geo_point); i++) {
    if (geo_key1[i] != geo_key2[i]) {
      for (j = 0; j < 8; j++) {
        if ((geo_key1[i] & (1 << (7 - j))) != (geo_key2[i] & (1 << (7 - j)))) {
          diff_bit = j;
          break;
        }
      }
      break;
    }
  }

  return i * 8 + diff_bit;
}

static void
compute_min_and_max(grn_geo_point *base_point, int diff_bit,
                    grn_geo_point *geo_min, grn_geo_point *geo_max)
{
  int diff_byte, diff_bit_mask;
  uint8_t geo_key_base[sizeof(grn_geo_point)];
  uint8_t geo_key_min[sizeof(grn_geo_point)];
  uint8_t geo_key_max[sizeof(grn_geo_point)];

  diff_byte = diff_bit / 8;
  diff_bit_mask = 0xff >> (diff_bit % 8);
  grn_gton(geo_key_base, base_point, sizeof(grn_geo_point));

  if (diff_byte == sizeof(grn_geo_point)) {
    memcpy(geo_key_min, geo_key_base, diff_byte);
    memcpy(geo_key_max, geo_key_base, diff_byte);
  } else {
    memcpy(geo_key_min, geo_key_base, diff_byte + 1);
    geo_key_min[diff_byte] &= ~diff_bit_mask;
    memset(geo_key_min + diff_byte + 1, 0,
           sizeof(grn_geo_point) - diff_byte - 1);

    memcpy(geo_key_max, geo_key_base, diff_byte + 1);
    geo_key_max[diff_byte] |= diff_bit_mask;
    memset(geo_key_max + diff_byte + 1, 0xff,
           sizeof(grn_geo_point) - diff_byte - 1);
  }

  grn_ntog((uint8_t *)geo_min, geo_key_min, sizeof(grn_geo_point));
  grn_ntog((uint8_t *)geo_max, geo_key_max, sizeof(grn_geo_point));
}

/* #define GEO_DEBUG */

#ifdef GEO_DEBUG
#include <stdio.h>

static void
inspect_mesh(grn_ctx *ctx, grn_geo_point *key, int key_size, int n)
{
  grn_geo_point min, max;

  printf("mesh: %d:%d\n", n, key_size);

  printf("key: ");
  grn_p_geo_point(ctx, key);

  compute_min_and_max(key, key_size, &min, &max);
  printf("min: ");
  grn_p_geo_point(ctx, &min);
  printf("max: ");
  grn_p_geo_point(ctx, &max);
}

static void
inspect_mesh_entry(grn_ctx *ctx, mesh_entry *entries, int n)
{
  mesh_entry *entry;

  entry = entries + n;
  inspect_mesh(ctx, &(entry->key), entry->key_size, n);
}

static void
inspect_tid(grn_ctx *ctx, grn_id tid, grn_geo_point *point, double d)
{
  printf("tid: %d:%g", tid, d);
  grn_p_geo_point(ctx, point);
}
#else
#  define inspect_mesh(...)
#  define inspect_mesh_entry(...)
#  define inspect_tid(...)
#endif

static int
grn_geo_table_sort_detect_far_point(grn_ctx *ctx, grn_obj *table, grn_obj *index,
                                    grn_pat *pat, geo_entry *entries,
                                    grn_pat_cursor *pc, int n, int accessorp,
                                    grn_geo_point *base_point,
                                    double *d_far, int *diff_bit)
{
  int i = 0, diff_bit_prev, diff_bit_current;
  grn_id tid;
  geo_entry *ep, *p;
  double d;
  uint8_t geo_key_prev[sizeof(grn_geo_point)];
  uint8_t geo_key_curr[sizeof(grn_geo_point)];
  grn_geo_point point;

  *d_far = 0.0;
  grn_gton(geo_key_curr, base_point, sizeof(grn_geo_point));
  *diff_bit = sizeof(grn_geo_point) * 8;
  diff_bit_current = sizeof(grn_geo_point) * 8;
  memcpy(&point, base_point, sizeof(grn_geo_point));
  ep = entries;
  inspect_mesh(ctx, &point, *diff_bit, -1);
  while ((tid = grn_pat_cursor_next(ctx, pc))) {
    grn_ii_cursor *ic = grn_ii_cursor_open(ctx, (grn_ii *)index, tid, 0, 0, 1, 0);
    if (ic) {
      grn_ii_posting *posting;
      grn_gton(geo_key_prev, &point, sizeof(grn_geo_point));
      grn_pat_get_key(ctx, pat, tid, &point, sizeof(grn_geo_point));
      grn_gton(geo_key_curr, &point, sizeof(grn_geo_point));
      d = grn_geo_distance_raw(ctx, base_point, &point);
      inspect_tid(ctx, tid, &point, d);

      diff_bit_prev = diff_bit_current;
      diff_bit_current = compute_diff_bit(geo_key_curr, geo_key_prev);
#ifdef GEO_DEBUG
      printf("diff: %d:%d:%d\n", *diff_bit, diff_bit_prev, diff_bit_current);
#endif
      if ((diff_bit_current % 2) == 1) {
        diff_bit_current--;
      }
      if (diff_bit_current < diff_bit_prev && *diff_bit > diff_bit_current) {
        if (i == n) {
          break;
        }
        *diff_bit = diff_bit_current;
      }

      if (d > *d_far) {
        *d_far = d;
      }
      while ((posting = grn_ii_cursor_next(ctx, ic))) {
        grn_id rid = accessorp
          ? grn_table_get(ctx, table, &posting->rid, sizeof(grn_id))
          : posting->rid;
        if (rid) {
          for (p = ep; entries < p && p[-1].d > d; p--) {
            p->id = p[-1].id;
            p->d = p[-1].d;
          }
          p->id = rid;
          p->d = d;
          if (i < n) {
            ep++;
            i++;
          }
        }
      }
      grn_ii_cursor_close(ctx, ic);
    }
  }

  return i;
}

typedef enum {
  MESH_LEFT_TOP,
  MESH_RIGHT_TOP,
  MESH_RIGHT_BOTTOM,
  MESH_LEFT_BOTTOM
} mesh_position;

/*
  meshes should have
    86 >= spaces when include_base_point_hash == GRN_FALSE,
    87 >= spaces when include_base_point_hash == GRN_TRUE.
*/
static int
grn_geo_get_meshes_for_circle(grn_ctx *ctx, grn_geo_point *base_point,
                              double d_far, int diff_bit,
                              int include_base_point_mesh,
                              mesh_entry *meshes)
{
  double d;
  int n_meshes;
  int lat_diff, lng_diff;
  mesh_position position;
  grn_geo_point geo_base, geo_min, geo_max;

  compute_min_and_max(base_point, diff_bit - 2, &geo_min, &geo_max);

  lat_diff = (geo_max.latitude - geo_min.latitude + 1) / 2;
  lng_diff = (geo_max.longitude - geo_min.longitude + 1) / 2;
  geo_base.latitude = geo_min.latitude + lat_diff;
  geo_base.longitude = geo_min.longitude + lng_diff;
  if (base_point->latitude >= geo_base.latitude) {
    if (base_point->longitude >= geo_base.longitude) {
      position = MESH_RIGHT_TOP;
    } else {
      position = MESH_LEFT_TOP;
    }
  } else {
    if (base_point->longitude >= geo_base.longitude) {
      position = MESH_RIGHT_BOTTOM;
    } else {
      position = MESH_LEFT_BOTTOM;
    }
  }
  /*
    base_point: b
    geo_min: i
    geo_max: a
    geo_base: x: must be at the left bottom in the top right mesh.

    e.g.: base_point is at the left bottom mesh case:
              +------+------+
              |      |     a|
              |      |x     |
             ^+------+------+
             ||      |      |
    lng_diff || b    |      |
            \/i------+------+
              <------>
              lat_diff

    grn_min + lat_diff -> the right mesh.
    grn_min + lng_diff -> the top mesh.
   */
#ifdef GEO_DEBUG
  grn_p_geo_point(ctx, base_point);
  printf("base: ");
  grn_p_geo_point(ctx, &geo_base);
  printf("min:  ");
  grn_p_geo_point(ctx, &geo_min);
  printf("max:  ");
  grn_p_geo_point(ctx, &geo_max);
  printf("diff: %d (%d, %d)\n", diff_bit, lat_diff, lng_diff);
  switch (position) {
  case MESH_LEFT_TOP :
    printf("position: left-top\n");
    break;
  case MESH_RIGHT_TOP :
    printf("position: right-top\n");
    break;
  case MESH_RIGHT_BOTTOM :
    printf("position: right-bottom\n");
    break;
  case MESH_LEFT_BOTTOM :
    printf("position: left-bottom\n");
    break;
  }
#endif

  n_meshes = 0;

#define add_mesh(lat_diff_,lng_diff_,key_size_)\
  meshes[n_meshes].key.latitude = geo_base.latitude + (lat_diff_);\
  meshes[n_meshes].key.longitude = geo_base.longitude + (lng_diff_);\
  meshes[n_meshes].key_size = key_size_;\
  n_meshes++;

  if (include_base_point_mesh || position != MESH_LEFT_TOP) {
    add_mesh(0, -lng_diff, diff_bit);
  }
  if (include_base_point_mesh || position != MESH_RIGHT_TOP) {
    add_mesh(0, 0, diff_bit);
  }
  if (include_base_point_mesh || position != MESH_RIGHT_BOTTOM) {
    add_mesh(-lat_diff, 0, diff_bit);
  }
  if (include_base_point_mesh || position != MESH_LEFT_BOTTOM) {
    add_mesh(-lat_diff, -lng_diff, diff_bit);
  }

  /*
    b: base_point
    x: geo_base
    0-83: sub meshes. 0-83 are added order.

  j: -5  -4  -3  -2  -1   0   1   2   3   4
    +---+---+---+---+---+---+---+---+---+---+
    |74 |75 |76 |77 |78 |79 |80 |81 |82 |83 | 4
    +---+---+---+---+---+---+---+---+---+---+
    |64 |65 |66 |67 |68 |69 |70 |71 |72 |73 | 3
    +---+---+---+---+---+---+---+---+---+---+
    |54 |55 |56 |57 |58 |59 |60 |61 |62 |63 | 2
    +---+---+---+---+---+---+---+---+---+---+
    |48 |49 |50 |  b    |       |51 |52 |53 | 1
    +---+---+---+       |       +---+---+---+
    |42 |43 |44 |       |x      |45 |46 |47 | 0
    +---+---+---+-------+-------+---+---+---+
    |36 |37 |38 |       |       |39 |40 |41 | -1
    +---+---+---+  base meshes  +---+---+---+
    |30 |31 |32 |       |       |33 |34 |35 | -2
    +---+---+---+---+---+---+---+---+---+---+
    |20 |21 |22 |23 |24 |25 |26 |27 |28 |29 | -3
    +---+---+---+---+---+---+---+---+---+---+
    |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 | -4
    +---+---+---+---+---+---+---+---+---+---+
    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | -5
    +---+---+---+---+---+---+---+---+---+---+
                                              i
  */
  {
    int i, j, n_sub_meshes, lat, lat_min, lat_max, lng, lng_min, lng_max;
    n_sub_meshes = 0;
    for (i = -5; i < 5; i++) {
      lat_min = ((lat_diff + 1) / 2) * i;
      lat_max = ((lat_diff + 1) / 2) * (i + 1) - 1;
      for (j = -5; j < 5; j++) {
        if (-3 < i && i < 2 && -3 < j && j < 2) {
          continue;
        }
        lng_min = ((lng_diff + 1) / 2) * j;
        lng_max = ((lng_diff + 1) / 2) * (j + 1) - 1;
        if (base_point->latitude <= geo_base.latitude + lat_min) {
          lat = geo_base.latitude + lat_min;
        } else if (geo_base.latitude + lat_max < base_point->latitude) {
          lat = geo_base.latitude + lat_max;
        } else {
          lat = base_point->latitude;
        }
        if (base_point->longitude <= geo_base.longitude + lng_min) {
          lng = geo_base.longitude + lng_min;
        } else if (geo_base.longitude + lng_max < base_point->longitude) {
          lng = geo_base.longitude + lng_max;
        } else {
          lng = base_point->longitude;
        }
        meshes[n_meshes].key.latitude = lat;
        meshes[n_meshes].key.longitude = lng;
        d = grn_geo_distance_raw(ctx, base_point, &(meshes[n_meshes].key));
        if (d < d_far) {
#ifdef GEO_DEBUG
          printf("sub-mesh: %d: (%d,%d): (%d,%d;%d,%d)\n",
                 n_sub_meshes, base_point->latitude, base_point->longitude,
                 geo_base.latitude + lat_min,
                 geo_base.latitude + lat_max,
                 geo_base.longitude + lng_min,
                 geo_base.longitude + lng_max);
          grn_p_geo_point(ctx, &(meshes[n_meshes].key));
#endif
          meshes[n_meshes].key_size = diff_bit + 2;
          n_meshes++;
        }
        n_sub_meshes++;
      }
    }
  }

#undef add_mesh

  return n_meshes;
}

static int
grn_geo_table_sort_collect_points(grn_ctx *ctx, grn_obj *table, grn_obj *index,
                                  grn_pat *pat,
                                  geo_entry *entries, int n_entries,
                                  int n, int accessorp,
                                  grn_geo_point *base_point,
                                  double d_far, int diff_bit)
{
  int n_meshes;
  mesh_entry meshes[86];
  geo_entry *ep, *p;

  n_meshes = grn_geo_get_meshes_for_circle(ctx, base_point, d_far, diff_bit,
                                           GRN_FALSE, meshes);

  ep = entries + n_entries;
  while (n_meshes--) {
    grn_id tid;
    grn_pat_cursor *pc = grn_pat_cursor_open(ctx, pat,
                                             &(meshes[n_meshes].key),
                                             meshes[n_meshes].key_size,
                                             NULL, 0,
                                             0, -1,
                                             GRN_CURSOR_PREFIX|GRN_CURSOR_SIZE_BY_BIT);
    inspect_mesh_entry(ctx, meshes, n_meshes);
    if (pc) {
      while ((tid = grn_pat_cursor_next(ctx, pc))) {
        grn_ii_cursor *ic = grn_ii_cursor_open(ctx, (grn_ii *)index, tid, 0, 0, 1, 0);
        if (ic) {
          double d;
          grn_geo_point pos;
          grn_ii_posting *posting;
          grn_pat_get_key(ctx, pat, tid, &pos, sizeof(grn_geo_point));
          d = grn_geo_distance_raw(ctx, base_point, &pos);
          inspect_tid(ctx, tid, &pos, d);
          while ((posting = grn_ii_cursor_next(ctx, ic))) {
            grn_id rid = accessorp
              ? grn_table_get(ctx, table, &posting->rid, sizeof(grn_id))
              : posting->rid;
            if (rid) {
              for (p = ep; entries < p && p[-1].d > d; p--) {
                p->id = p[-1].id;
                p->d = p[-1].d;
              }
              p->id = rid;
              p->d = d;
              if (n_entries < n) {
                ep++;
                n_entries++;
              }
            }
          }
          grn_ii_cursor_close(ctx, ic);
        }
      }
      grn_pat_cursor_close(ctx, pc);
    }
  }
  return n_entries;
}

int
grn_geo_table_sort(grn_ctx *ctx, grn_obj *table, int offset, int limit,
                   grn_obj *result, grn_table_sort_key *keys, int n_keys)
{
  grn_obj *index;
  int i = 0, e = offset + limit, sid, accessorp = GRN_ACCESSORP(keys->key);
  if (n_keys == 2 && grn_column_index(ctx, keys->key, GRN_OP_LESS, &index, 1, &sid)) {
    grn_id tid;
    grn_obj *arg = keys[1].key;
    grn_pat *pat = (grn_pat *)grn_ctx_at(ctx, index->header.domain);
    grn_id domain = pat->obj.header.domain;
    grn_pat_cursor *pc = grn_pat_cursor_open(ctx, pat, NULL, 0,
                                             GRN_BULK_HEAD(arg), GRN_BULK_VSIZE(arg),
                                             0, -1, GRN_CURSOR_PREFIX);
    if (pc) {
      if (domain != GRN_DB_TOKYO_GEO_POINT && domain != GRN_DB_WGS84_GEO_POINT) {
        while (i < e && (tid = grn_pat_cursor_next(ctx, pc))) {
          grn_ii_cursor *ic = grn_ii_cursor_open(ctx, (grn_ii *)index, tid, 0, 0, 1, 0);
          if (ic) {
            grn_ii_posting *posting;
            while (i < e && (posting = grn_ii_cursor_next(ctx, ic))) {
              if (offset <= i) {
                grn_id *v;
                if (!grn_array_add(ctx, (grn_array *)result, (void **)&v)) { break; }
                *v = posting->rid;
              }
              i++;
            }
            grn_ii_cursor_close(ctx, ic);
          }
        }
        grn_pat_cursor_close(ctx, pc);
      } else {
        geo_entry *entries;

        if ((entries = GRN_MALLOC(sizeof(geo_entry) * (e + 1)))) {
          int n, diff_bit;
          double d_far;
          grn_id *v;
          grn_geo_point *base_point;
          geo_entry *ep;

          base_point = (grn_geo_point *)GRN_BULK_HEAD(arg);
          n = grn_geo_table_sort_detect_far_point(ctx, table, index, pat,
                                                  entries, pc, e, accessorp,
                                                  base_point,
                                                  &d_far, &diff_bit);
          grn_pat_cursor_close(ctx, pc);
          if (diff_bit > 0) {
            n += grn_geo_table_sort_collect_points(ctx, table, index, pat,
                                                   entries, n, e, accessorp,
                                                   base_point, d_far, diff_bit);
          }
          for (i = 0, ep = entries + offset; i < limit && ep < entries + n; i++, ep++) {
            if (!grn_array_add(ctx, (grn_array *)result, (void **)&v)) { break; }
            *v = ep->id;
          }
          GRN_FREE(entries);
        }
      }
    }
  }
  return i;
}

grn_rc
grn_geo_search_in_circle(grn_ctx *ctx, grn_obj *obj, grn_obj **args, int nargs,
                         grn_obj *res, grn_operator op)
{
  grn_id domain;
  double lng0, lat0, lng1, lat1, lng2, lat2, x, y, d;
  grn_obj *proc, *pat, *pos1, *pos2, pos1_, pos2_;
  grn_geo_point *geo_point1, geo_point2;
  if (nargs != 4) { goto exit; }
  pat = grn_ctx_at(ctx, obj->header.domain);
  proc = args[0];
  pos1 = args[2];
  pos2 = args[3];
  domain = pat->header.domain;
  if (domain != GRN_DB_TOKYO_GEO_POINT && domain != GRN_DB_WGS84_GEO_POINT) { goto exit; }
  if (pos1->header.domain != domain) {
    GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
    if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
    pos1 = &pos1_;
  }
  geo_point1 = GRN_GEO_POINT_VALUE_RAW(pos1);
  lng1 = GRN_GEO_INT2RAD(geo_point1->longitude);
  lat1 = GRN_GEO_INT2RAD(geo_point1->latitude);
  switch (pos2->header.domain) {
  case GRN_DB_INT32 :
    d = GRN_INT32_VALUE(pos2) / (double)GRN_GEO_RADIUS;
    geo_point2.latitude = geo_point1->latitude + GRN_GEO_RAD2INT(d);
    geo_point2.longitude = geo_point1->longitude;
    d = d * d;
    break;
  case GRN_DB_UINT32 :
    d = GRN_UINT32_VALUE(pos2) / (double)GRN_GEO_RADIUS;
    geo_point2.latitude = geo_point1->latitude + GRN_GEO_RAD2INT(d);
    geo_point2.longitude = geo_point1->longitude;
    d = d * d;
    break;
  case GRN_DB_INT64 :
    d = GRN_INT64_VALUE(pos2) / (double)GRN_GEO_RADIUS;
    geo_point2.latitude = geo_point1->latitude + GRN_GEO_RAD2INT(d);
    geo_point2.longitude = geo_point1->longitude;
    d = d * d;
    break;
  case GRN_DB_UINT64 :
    d = GRN_UINT64_VALUE(pos2) / (double)GRN_GEO_RADIUS;
    geo_point2.latitude = geo_point1->latitude + GRN_GEO_RAD2INT(d);
    geo_point2.longitude = geo_point1->longitude;
    d = d * d;
    break;
  case GRN_DB_FLOAT :
    d = GRN_FLOAT_VALUE(pos2) / (double)GRN_GEO_RADIUS;
    geo_point2.latitude = geo_point1->latitude + GRN_GEO_RAD2INT(d);
    geo_point2.longitude = geo_point1->longitude;
    d = d * d;
    break;
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    GRN_OBJ_INIT(&pos2_, GRN_BULK, 0, domain);
    if (grn_obj_cast(ctx, pos2, &pos2_, 0)) { goto exit; }
    pos2 = &pos2_;
    /* fallthru */
  case GRN_DB_TOKYO_GEO_POINT :
  case GRN_DB_WGS84_GEO_POINT :
    if (domain != pos2->header.domain) { /* todo */ goto exit; }
    GRN_GEO_POINT_VALUE(pos2, geo_point2.latitude, geo_point2.longitude);
    lng2 = GRN_GEO_INT2RAD(geo_point2.longitude);
    lat2 = GRN_GEO_INT2RAD(geo_point2.latitude);
    x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
    y = (lat2 - lat1);
    d = ((x * x) + (y * y));
    break;
  default :
    goto exit;
  }
  {
    int n_meshes, diff_bit;
    double d_far;
    mesh_entry meshes[87];
    uint8_t geo_key1[sizeof(grn_geo_point)];
    uint8_t geo_key2[sizeof(grn_geo_point)];

    d_far = grn_geo_distance_raw(ctx, geo_point1, &geo_point2);
    grn_gton(geo_key1, geo_point1, sizeof(grn_geo_point));
    grn_gton(geo_key2, &geo_point2, sizeof(grn_geo_point));
    diff_bit = compute_diff_bit(geo_key1, geo_key2);
#ifdef GEO_DEBUG
    printf("point1: ");
    grn_p_geo_point(ctx, geo_point1);
    printf("point2: ");
    grn_p_geo_point(ctx, &geo_point2);
    printf("diff:   %d\n", diff_bit);
#endif
    if ((diff_bit % 2) == 1) {
      diff_bit--;
    }
    n_meshes = grn_geo_get_meshes_for_circle(ctx, geo_point1,
                                             d_far, diff_bit, GRN_TRUE,
                                             meshes);
    while (n_meshes--) {
      grn_table_cursor *tc;
      tc = grn_table_cursor_open(ctx, pat,
                                 &(meshes[n_meshes].key),
                                 meshes[n_meshes].key_size,
                                 NULL, 0,
                                 0, -1,
                                 GRN_CURSOR_PREFIX|GRN_CURSOR_SIZE_BY_BIT);
      inspect_mesh_entry(ctx, meshes, n_meshes);
      if (tc) {
        grn_id tid;
        grn_geo_point pos;
        while ((tid = grn_table_cursor_next(ctx, tc))) {
          grn_table_get_key(ctx, pat, tid, &pos, sizeof(grn_geo_point));
          lng0 = GRN_GEO_INT2RAD(pos.longitude);
          lat0 = GRN_GEO_INT2RAD(pos.latitude);
          x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
          y = (lat1 - lat0);
          if (((x * x) + (y * y)) <= d) {
            inspect_tid(ctx, tid, &pos, (x * x) + (y * y));
            grn_ii_at(ctx, (grn_ii *)obj, tid, (grn_hash *)res, op);
          }
        }
        grn_table_cursor_close(ctx, tc);
      }
    }
  }
exit :
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  return ctx->rc;
}

typedef enum {
  MESH_LATITUDE,
  MESH_LONGITUDE
} mesh_direction;

grn_rc
grn_geo_search_in_rectangle(grn_ctx *ctx, grn_obj *obj, grn_obj **args, int nargs,
                            grn_obj *res, grn_operator op)
{
  grn_id domain;
  grn_obj *proc, *pat, *pos1, *pos2, pos1_, pos2_;
  grn_geo_point *geo_point1, *geo_point2;
  if (nargs != 4) { goto exit; }
  pat = grn_ctx_at(ctx, obj->header.domain);
  proc = args[0];
  pos1 = args[2];
  pos2 = args[3];
  domain = pat->header.domain;
  if (domain != GRN_DB_TOKYO_GEO_POINT && domain != GRN_DB_WGS84_GEO_POINT) { goto exit; }
  if (pos1->header.domain != domain) {
    GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
    if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
    pos1 = &pos1_;
  }
  geo_point1 = GRN_GEO_POINT_VALUE_RAW(pos1);
  if (pos2->header.domain != domain) {
    GRN_OBJ_INIT(&pos2_, GRN_BULK, 0, domain);
    if (grn_obj_cast(ctx, pos2, &pos2_, 0)) { goto exit; }
    pos2 = &pos2_;
  }
  geo_point2 = GRN_GEO_POINT_VALUE_RAW(pos2);
  {
    mesh_direction direction;
    int distance, latitude_distance, longitude_distance;
    int i, start, end, diff_bit;
    grn_geo_point *geo_point_input, geo_point_base, geo_point_min, geo_point_max;
    uint8_t geo_key_input[sizeof(grn_geo_point)];
    uint8_t geo_key_base[sizeof(grn_geo_point)];

    if (geo_point1->latitude < 0 || geo_point1->longitude < 0 ||
        geo_point2->latitude < 0 || geo_point2->longitude < 0) {
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "geo_in_rectangle() with negative coordinate is not implemented.");
      goto exit;
    }

    if (geo_point1->latitude >= GRN_GEO_MAX_LATITUDE) {
      ERR(GRN_INVALID_ARGUMENT,
          "geo_in_rectangle(): geo point1's latitude is too big: <%d>(max:%d): "
          "(%d,%d) (%d,%d)",
          GRN_GEO_MAX_LATITUDE, geo_point1->latitude,
          geo_point1->latitude, geo_point1->longitude,
          geo_point2->latitude, geo_point2->longitude);
      goto exit;
    }

    latitude_distance = geo_point1->latitude - geo_point2->latitude;
    longitude_distance = geo_point2->longitude - geo_point1->longitude;
    if (latitude_distance > longitude_distance) {
      direction = MESH_LATITUDE;
      geo_point_input = geo_point2;
      geo_point_base.latitude = geo_point2->latitude;
      geo_point_base.longitude = geo_point2->longitude - longitude_distance;
    } else {
      direction = MESH_LONGITUDE;
      geo_point_input = geo_point1;
      geo_point_base.latitude = geo_point1->latitude - latitude_distance;
      geo_point_base.longitude = geo_point1->longitude;
    }
    grn_gton(geo_key_input, geo_point_input, sizeof(grn_geo_point));
    grn_gton(geo_key_base, &geo_point_base, sizeof(grn_geo_point));
    diff_bit = compute_diff_bit(geo_key_input, geo_key_base);
    compute_min_and_max(&geo_point_base, diff_bit,
                        &geo_point_min, &geo_point_max);
    if (direction == MESH_LATITUDE) {
      distance = geo_point_max.latitude - geo_point_min.latitude + 1;
      start = geo_point_min.latitude;
      end = geo_point1->latitude;
    } else {
      distance = geo_point_max.longitude - geo_point_min.longitude + 1;
      start = geo_point_min.longitude;
      end = geo_point2->longitude;
    }
#ifdef GEO_DEBUG
    printf("direction: %s\n",
           direction == MESH_LATITUDE ? "latitude" : "longitude");
    printf("base:         ");
    grn_p_geo_point(ctx, &geo_point_base);
    printf("input:        ");
    grn_p_geo_point(ctx, geo_point_input);
    printf("min:          ");
    grn_p_geo_point(ctx, &geo_point_min);
    printf("max:          ");
    grn_p_geo_point(ctx, &geo_point_max);
    printf("top-left:     ");
    grn_p_geo_point(ctx, geo_point1);
    printf("bottom-right: ");
    grn_p_geo_point(ctx, geo_point2);
    printf("diff-bit:            %10d\n", diff_bit);
    printf("start:               %10d\n", start);
    printf("end:                 %10d\n", end);
    printf("distance:            %10d\n", distance);
    printf("distance(latitude):  %10d\n", latitude_distance);
    printf("distance(longitude): %10d\n", longitude_distance);
#endif
    memcpy(&geo_point_base, &geo_point_min, sizeof(grn_geo_point));

    for (i = start; i < end + distance; i += distance) {
      grn_table_cursor *tc;
      tc = grn_table_cursor_open(ctx, pat,
                                 &geo_point_base, diff_bit,
                                 NULL, 0,
                                 0, -1,
                                 GRN_CURSOR_PREFIX|GRN_CURSOR_SIZE_BY_BIT);
#ifdef GEO_DEBUG
      printf("i:                   %10d\n", i);
#endif
      inspect_mesh(ctx, &geo_point_base, diff_bit, (i - start) / distance);
      if (tc) {
        grn_id tid;
        grn_geo_point pos;
        while ((tid = grn_table_cursor_next(ctx, tc))) {
          grn_table_get_key(ctx, pat, tid, &pos, sizeof(grn_geo_point));
          if (!grn_geo_in_rectangle_raw(ctx, &pos, geo_point1, geo_point2)) {
            continue;
          }
          inspect_tid(ctx, tid, &pos, 0);
          grn_ii_at(ctx, (grn_ii *)obj, tid, (grn_hash *)res, op);
        }
        grn_table_cursor_close(ctx, tc);
      }
      if (direction == MESH_LATITUDE) {
        geo_point_base.latitude += distance;
      } else {
        geo_point_base.longitude += distance;
      }
    }
  }
exit :
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  return ctx->rc;
}

unsigned
grn_geo_in_circle(grn_ctx *ctx, grn_obj *point, grn_obj *center,
                  grn_obj *radius_or_point)
{
  unsigned r = GRN_FALSE;
  grn_obj center_, radius_or_point_;
  grn_id domain = point->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    double lng0, lat0, lng1, lat1, lng2, lat2, x, y, d;
    if (center->header.domain != domain) {
      GRN_OBJ_INIT(&center_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, center, &center_, 0)) { goto exit; }
      center = &center_;
    }
    GRN_GEO_POINT_VALUE_RADIUS(point, lat0, lng0);
    GRN_GEO_POINT_VALUE_RADIUS(center, lat1, lng1);
    x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
    y = (lat1 - lat0);
    d = (x * x) + (y * y);
    switch (radius_or_point->header.domain) {
    case GRN_DB_INT32 :
      r = (sqrt(d) * GRN_GEO_RADIUS) <= GRN_INT32_VALUE(radius_or_point);
      break;
    case GRN_DB_UINT32 :
      r = (sqrt(d) * GRN_GEO_RADIUS) <= GRN_UINT32_VALUE(radius_or_point);
      break;
    case GRN_DB_INT64 :
      r = (sqrt(d) * GRN_GEO_RADIUS) <= GRN_INT64_VALUE(radius_or_point);
      break;
    case GRN_DB_UINT64 :
      r = (sqrt(d) * GRN_GEO_RADIUS) <= GRN_UINT64_VALUE(radius_or_point);
      break;
    case GRN_DB_FLOAT :
      r = (sqrt(d) * GRN_GEO_RADIUS) <= GRN_FLOAT_VALUE(radius_or_point);
      break;
    case GRN_DB_SHORT_TEXT :
    case GRN_DB_TEXT :
    case GRN_DB_LONG_TEXT :
      GRN_OBJ_INIT(&radius_or_point_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, radius_or_point, &radius_or_point_, 0)) { goto exit; }
      radius_or_point = &radius_or_point_;
      /* fallthru */
    case GRN_DB_TOKYO_GEO_POINT :
    case GRN_DB_WGS84_GEO_POINT :
      if (domain != radius_or_point->header.domain) { /* todo */ goto exit; }
      GRN_GEO_POINT_VALUE_RADIUS(radius_or_point, lat2, lng2);
      x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
      y = (lat2 - lat1);
      r = d <= (x * x) + (y * y);
      break;
    default :
      goto exit;
    }
  } else {
    /* todo */
  }
exit :
  return r;
}

unsigned
grn_geo_in_rectangle_raw(grn_ctx *ctx, grn_geo_point *point,
                         grn_geo_point *top_left, grn_geo_point *bottom_right)
{
    return ((top_left->longitude <= point->longitude) &&
            (point->longitude <= bottom_right->longitude) &&
            (bottom_right->latitude <= point->latitude) &&
            (point->latitude <= top_left->latitude));
}

unsigned
grn_geo_in_rectangle(grn_ctx *ctx, grn_obj *point,
                     grn_obj *top_left, grn_obj *bottom_right)
{
  unsigned r = GRN_FALSE;
  grn_obj top_left_, bottom_right_;
  grn_id domain = point->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    if (top_left->header.domain != domain) {
      GRN_OBJ_INIT(&top_left_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, top_left, &top_left_, 0)) { goto exit; }
      top_left = &top_left_;
    }
    if (bottom_right->header.domain != domain) {
      GRN_OBJ_INIT(&bottom_right_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, bottom_right, &bottom_right_, 0)) { goto exit; }
      bottom_right = &bottom_right_;
    }
    r = grn_geo_in_rectangle_raw(ctx,
                                 GRN_GEO_POINT_VALUE_RAW(point),
                                 GRN_GEO_POINT_VALUE_RAW(top_left),
                                 GRN_GEO_POINT_VALUE_RAW(bottom_right));
  } else {
    /* todo */
  }
exit :
  return r;
}

double
grn_geo_distance_raw(grn_ctx *ctx, grn_geo_point *point1, grn_geo_point *point2)
{
  double lng1, lat1, lng2, lat2, x, y;

  lat1 = GRN_GEO_INT2RAD(point1->latitude);
  lng1 = GRN_GEO_INT2RAD(point1->longitude);
  lat2 = GRN_GEO_INT2RAD(point2->latitude);
  lng2 = GRN_GEO_INT2RAD(point2->longitude);
  x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
  y = (lat2 - lat1);
  return sqrt((x * x) + (y * y)) * GRN_GEO_RADIUS;
}

double
grn_geo_distance2_raw(grn_ctx *ctx, grn_geo_point *point1, grn_geo_point *point2)
{
  double lng1, lat1, lng2, lat2, x, y;

  lat1 = GRN_GEO_INT2RAD(point1->latitude);
  lng1 = GRN_GEO_INT2RAD(point1->longitude);
  lat2 = GRN_GEO_INT2RAD(point2->latitude);
  lng2 = GRN_GEO_INT2RAD(point2->longitude);
  x = sin(fabs(lng2 - lng1) * 0.5);
  y = sin(fabs(lat2 - lat1) * 0.5);
  return asin(sqrt((y * y) + cos(lat1) * cos(lat2) * x * x)) * 2 * GRN_GEO_RADIUS;
}

double
grn_geo_distance3_raw(grn_ctx *ctx, grn_geo_point *point1, grn_geo_point *point2,
                      int c1, int c2, double c3)
{
  double lng1, lat1, lng2, lat2, p, q, r, m, n, x, y;

  lat1 = GRN_GEO_INT2RAD(point1->latitude);
  lng1 = GRN_GEO_INT2RAD(point1->longitude);
  lat2 = GRN_GEO_INT2RAD(point2->latitude);
  lng2 = GRN_GEO_INT2RAD(point2->longitude);
  p = (lat1 + lat2) * 0.5;
  q = (1 - c3 * sin(p) * sin(p));
  r = sqrt(q);
  m = c1 / (q * r);
  n = c2 / r;
  x = n * cos(p) * fabs(lng1 - lng2);
  y = m * fabs(lat1 - lat2);
  return sqrt((x * x) + (y * y));
}

double
grn_geo_distance(grn_ctx *ctx, grn_obj *point1, grn_obj *point2)
{
  double d = 0;
  grn_obj point2_;
  grn_id domain = point1->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    if (point2->header.domain != domain) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
      point2 = &point2_;
    }
    d = grn_geo_distance_raw(ctx,
                             GRN_GEO_POINT_VALUE_RAW(point1),
                             GRN_GEO_POINT_VALUE_RAW(point2));
  } else {
    /* todo */
  }
exit :
  return d;
}

double
grn_geo_distance2(grn_ctx *ctx, grn_obj *point1, grn_obj *point2)
{
  double d = 0;
  grn_obj point2_;
  grn_id domain = point1->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    if (point2->header.domain != domain) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
      point2 = &point2_;
    }
    d = grn_geo_distance2_raw(ctx,
                              GRN_GEO_POINT_VALUE_RAW(point1),
                              GRN_GEO_POINT_VALUE_RAW(point2));
  } else {
    /* todo */
  }
exit :
  return d;
}

double
grn_geo_distance3(grn_ctx *ctx, grn_obj *point1, grn_obj *point2)
{
  double d = 0;
  grn_obj point2_;
  grn_id domain = point1->header.domain;
  switch (domain) {
  case GRN_DB_TOKYO_GEO_POINT :
    if (point2->header.domain != domain) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
      point2 = &point2_;
    }
    d = grn_geo_distance3_raw(ctx,
                              GRN_GEO_POINT_VALUE_RAW(point1),
                              GRN_GEO_POINT_VALUE_RAW(point2),
                              GRN_GEO_BES_C1, GRN_GEO_BES_C2, GRN_GEO_BES_C3);
    break;
  case GRN_DB_WGS84_GEO_POINT :
    if (point2->header.domain != domain) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain);
      if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
      point2 = &point2_;
    }
    d = grn_geo_distance3_raw(ctx,
                              GRN_GEO_POINT_VALUE_RAW(point1),
                              GRN_GEO_POINT_VALUE_RAW(point2),
                              GRN_GEO_GRS_C1, GRN_GEO_GRS_C2, GRN_GEO_GRS_C3);
    break;
  default :
    /* todo */
    break;
  }
exit :
  return d;
}
