/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2011 Brazil

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

#include "geo.h"
#include "pat.h"
#include "util.h"

#include <string.h>
#include <stdlib.h>

typedef struct {
  grn_id id;
  double d;
} geo_entry;

typedef struct
{
  grn_geo_point key;
  int key_size;
} mesh_entry;

typedef struct {
  grn_obj *pat;
  grn_obj top_left_point_buffer;
  grn_obj bottom_right_point_buffer;
  grn_geo_point *top_left;
  grn_geo_point *bottom_right;
  grn_geo_point base;
  grn_geo_point min;
  grn_geo_point max;
  int start;
  int end;
  int distance;
  int diff_bit;
  int rectangle_common_bit;
  grn_geo_mesh_direction direction;
} in_rectangle_data;

static int
compute_diff_bit(uint8_t *geo_key1, uint8_t *geo_key2)
{
  int i, j, diff_bit = 0;

  for (i = 0; i < sizeof(grn_geo_point); i++) {
    if (geo_key1[i] != geo_key2[i]) {
      diff_bit = 8;
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

static void
inspect_key(grn_ctx *ctx, uint8_t *key)
{
  int i;
  for (i = 0; i < 8; i++) {
    int j;
    for (j = 0; j < 8; j++) {
      printf("%d", (key[i] & (1 << (7 - j))) >> (7 - j));
    }
    printf(" ");
  }
  printf("\n");
}

static void
print_key_mark(grn_ctx *ctx, int target_bit)
{
  int i;

  for (i = 0; i < target_bit; i++) {
    printf(" ");
    if (i > 0 && i % 8 == 0) {
      printf(" ");
    }
  }
  if (i > 0 && i % 8 == 0) {
    printf(" ");
  }
  printf("^\n");
}

static void
inspect_cursor_entry(grn_ctx *ctx, grn_geo_cursor_entry *entry)
{
  grn_geo_point point;

  printf("entry: ");
  grn_ntog((uint8_t *)&point, entry->base_key, sizeof(grn_geo_point));
  grn_p_geo_point(ctx, &point);
  inspect_key(ctx, entry->base_key);
  print_key_mark(ctx, entry->target_bit);
  printf("     target bit:    %d\n", entry->target_bit);
  printf("   top included:    %s\n", entry->top_included ? "true" : "false");
  printf("bottom included:    %s\n", entry->bottom_included ? "true" : "false");
  printf("  left included:    %s\n", entry->left_included ? "true" : "false");
  printf(" right included:    %s\n", entry->right_included ? "true" : "false");
  printf(" latitude inner:    %s\n", entry->latitude_inner ? "true" : "false");
  printf("longitude inner:    %s\n", entry->longitude_inner ? "true" : "false");
}

static void
inspect_cursor_entry_targets(grn_ctx *ctx, grn_geo_cursor_entry *entry,
                             uint8_t *top_left_key, uint8_t *bottom_right_key,
                             grn_geo_cursor_entry *next_entry0,
                             grn_geo_cursor_entry *next_entry1)
{
  printf("entry:        ");
  inspect_key(ctx, entry->base_key);
  printf("top-left:     ");
  inspect_key(ctx, top_left_key);
  printf("bottom-right: ");
  inspect_key(ctx, bottom_right_key);
  printf("next-entry-0: ");
  inspect_key(ctx, next_entry0->base_key);
  printf("next-entry-1: ");
  inspect_key(ctx, next_entry1->base_key);
  printf("              ");
  print_key_mark(ctx, entry->target_bit + 1);
}
#else
#  define inspect_mesh(...)
#  define inspect_mesh_entry(...)
#  define inspect_tid(...)
#  define inspect_key(...)
#  define print_key_mark(...)
#  define inspect_cursor_entry(...)
#  define inspect_cursor_entry_targets(...)
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
          grn_ii_cursor_close(ctx, ic);
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
grn_selector_geo_in_circle(grn_ctx *ctx, grn_obj *obj, grn_obj **args, int nargs,
                           grn_obj *res, grn_operator op)
{
  if (nargs == 4) {
    grn_obj *center_point, *distance;
    center_point = args[2];
    distance = args[3];
    grn_geo_select_in_circle(ctx, obj, center_point, distance, res, op);
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "geo_in_circle(): requires 3 arguments but was <%d> arguments",
        nargs - 1);
  }
  return ctx->rc;
}

grn_rc
grn_geo_select_in_circle(grn_ctx *ctx, grn_obj *index,
                         grn_obj *center_point, grn_obj *distance,
                         grn_obj *res, grn_operator op)
{
  grn_id domain;
  double center_longitude, center_latitude;
  double on_circle_longitude, on_circle_latitude;
  double x, y, d;
  grn_obj *pat, *point_on_circle = NULL, center_point_, point_on_circle_;
  grn_geo_point *center, on_circle;
  pat = grn_ctx_at(ctx, index->header.domain);
  domain = pat->header.domain;
  if (domain != GRN_DB_TOKYO_GEO_POINT && domain != GRN_DB_WGS84_GEO_POINT) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size = 0;
    grn_obj *domain_object;
    domain_object = grn_ctx_at(ctx, domain);
    if (domain_object) {
      name_size = grn_obj_name(ctx, domain_object, name, GRN_TABLE_MAX_KEY_SIZE);
      grn_obj_unlink(ctx, domain_object);
    } else {
      strcpy(name, "(null)");
      name_size = strlen(name);
    }
    ERR(GRN_INVALID_ARGUMENT,
        "geo_in_circle(): index table must be "
        "TokyoGeoPoint or WGS84GeoPoint key type table: <%.*s>",
        name_size, name);
    goto exit;
  }

  if (center_point->header.domain != domain) {
    GRN_OBJ_INIT(&center_point_, GRN_BULK, 0, domain);
    if (grn_obj_cast(ctx, center_point, &center_point_, 0)) { goto exit; }
    center_point = &center_point_;
  }
  center = GRN_GEO_POINT_VALUE_RAW(center_point);
  center_longitude = GRN_GEO_INT2RAD(center->longitude);
  center_latitude = GRN_GEO_INT2RAD(center->latitude);
  switch (distance->header.domain) {
  case GRN_DB_INT32 :
    d = GRN_INT32_VALUE(distance) / (double)GRN_GEO_RADIUS;
    on_circle.latitude = center->latitude + GRN_GEO_RAD2INT(d);
    on_circle.longitude = center->longitude;
    d = d * d;
    break;
  case GRN_DB_UINT32 :
    d = GRN_UINT32_VALUE(distance) / (double)GRN_GEO_RADIUS;
    on_circle.latitude = center->latitude + GRN_GEO_RAD2INT(d);
    on_circle.longitude = center->longitude;
    d = d * d;
    break;
  case GRN_DB_INT64 :
    d = GRN_INT64_VALUE(distance) / (double)GRN_GEO_RADIUS;
    on_circle.latitude = center->latitude + GRN_GEO_RAD2INT(d);
    on_circle.longitude = center->longitude;
    d = d * d;
    break;
  case GRN_DB_UINT64 :
    d = GRN_UINT64_VALUE(distance) / (double)GRN_GEO_RADIUS;
    on_circle.latitude = center->latitude + GRN_GEO_RAD2INT(d);
    on_circle.longitude = center->longitude;
    d = d * d;
    break;
  case GRN_DB_FLOAT :
    d = GRN_FLOAT_VALUE(distance) / (double)GRN_GEO_RADIUS;
    on_circle.latitude = center->latitude + GRN_GEO_RAD2INT(d);
    on_circle.longitude = center->longitude;
    d = d * d;
    break;
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    GRN_OBJ_INIT(&point_on_circle_, GRN_BULK, 0, domain);
    if (grn_obj_cast(ctx, point_on_circle, &point_on_circle_, 0)) { goto exit; }
    point_on_circle = &point_on_circle_;
    /* fallthru */
  case GRN_DB_TOKYO_GEO_POINT :
  case GRN_DB_WGS84_GEO_POINT :
    if (domain != distance->header.domain) { /* todo */ goto exit; }
    if (!point_on_circle) { point_on_circle = distance; }
    GRN_GEO_POINT_VALUE(point_on_circle,
                        on_circle.latitude, on_circle.longitude);
    on_circle_longitude = GRN_GEO_INT2RAD(on_circle.longitude);
    on_circle_latitude = GRN_GEO_INT2RAD(on_circle.latitude);
    x = (on_circle_longitude - center_longitude) *
      cos((center_latitude + on_circle_latitude) * 0.5);
    y = (on_circle_latitude - center_latitude);
    d = ((x * x) + (y * y));
    if (point_on_circle == &point_on_circle_) {
      grn_obj_unlink(ctx, point_on_circle);
    }
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

    d_far = grn_geo_distance_raw(ctx, center, &on_circle);
    grn_gton(geo_key1, center, sizeof(grn_geo_point));
    grn_gton(geo_key2, &on_circle, sizeof(grn_geo_point));
    diff_bit = compute_diff_bit(geo_key1, geo_key2);
    diff_bit = compute_diff_bit(geo_key1, geo_key2);
#ifdef GEO_DEBUG
    printf("center point: ");
    grn_p_geo_point(ctx, center);
    printf("point on circle: ");
    grn_p_geo_point(ctx, &on_circle);
    printf("diff:   %d\n", diff_bit);
#endif
    if ((diff_bit % 2) == 1) {
      diff_bit--;
    }
    n_meshes = grn_geo_get_meshes_for_circle(ctx, center,
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
        grn_geo_point point;
        double longitude, latitude;
        while ((tid = grn_table_cursor_next(ctx, tc))) {
          grn_table_get_key(ctx, pat, tid, &point, sizeof(grn_geo_point));
          longitude = GRN_GEO_INT2RAD(point.longitude);
          latitude = GRN_GEO_INT2RAD(point.latitude);
          x = (center_longitude - longitude) *
            cos((latitude + center_latitude) * 0.5);
          y = (center_latitude - latitude);
          if (((x * x) + (y * y)) <= d) {
            inspect_tid(ctx, tid, &point, (x * x) + (y * y));
            grn_ii_at(ctx, (grn_ii *)index, tid, (grn_hash *)res, op);
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

grn_rc
grn_selector_geo_in_rectangle(grn_ctx *ctx, grn_obj *obj,
                              grn_obj **args, int nargs,
                              grn_obj *res, grn_operator op)
{
  if (nargs == 4) {
    grn_obj *top_left_point, *bottom_right_point;
    top_left_point = args[2];
    bottom_right_point = args[3];
    grn_geo_select_in_rectangle(ctx, obj,
                                top_left_point, bottom_right_point,
                                res, op);
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "geo_in_rectangle(): requires 3 arguments but was <%d> arguments",
        nargs - 1);
  }
  return ctx->rc;
}

static grn_rc
in_rectangle_data_prepare(grn_ctx *ctx, grn_obj *index,
                          grn_obj *top_left_point,
                          grn_obj *bottom_right_point,
                          const char *process_name,
                          in_rectangle_data *data)
{
  grn_id domain;

  if (!index) {
    ERR(GRN_INVALID_ARGUMENT, "%s: index column is missing", process_name);
    goto exit;
  }

  data->pat = grn_ctx_at(ctx, index->header.domain);
  domain = data->pat->header.domain;
  if (domain != GRN_DB_TOKYO_GEO_POINT && domain != GRN_DB_WGS84_GEO_POINT) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size = 0;
    grn_obj *domain_object;
    domain_object = grn_ctx_at(ctx, domain);
    if (domain_object) {
      name_size = grn_obj_name(ctx, domain_object, name, GRN_TABLE_MAX_KEY_SIZE);
      grn_obj_unlink(ctx, domain_object);
    } else {
      strcpy(name, "(null)");
      name_size = strlen(name);
    }
    ERR(GRN_INVALID_ARGUMENT,
        "%s: index table must be "
        "TokyoGeoPoint or WGS84GeoPoint key type table: <%.*s>",
        process_name, name_size, name);
    goto exit;
  }

  if (top_left_point->header.domain != domain) {
    grn_obj_reinit(ctx, &(data->top_left_point_buffer), domain, GRN_BULK);
    if (grn_obj_cast(ctx, top_left_point, &(data->top_left_point_buffer),
                     GRN_FALSE)) {
      goto exit;
    }
    top_left_point = &(data->top_left_point_buffer);
  }
  data->top_left = GRN_GEO_POINT_VALUE_RAW(top_left_point);

  if (bottom_right_point->header.domain != domain) {
    grn_obj_reinit(ctx, &(data->bottom_right_point_buffer), domain, GRN_BULK);
    if (grn_obj_cast(ctx, bottom_right_point, &(data->bottom_right_point_buffer),
                     GRN_FALSE)) {
      goto exit;
    }
    bottom_right_point = &(data->bottom_right_point_buffer);
  }
  data->bottom_right = GRN_GEO_POINT_VALUE_RAW(bottom_right_point);

  {
    grn_geo_point *top_left, *bottom_right;

    top_left = data->top_left;
    bottom_right = data->bottom_right;

    if (top_left->latitude < 0 || top_left->longitude < 0 ||
        bottom_right->latitude < 0 || bottom_right->longitude < 0) {
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "%s: negative coordinate is not implemented.", process_name);
      goto exit;
    }

    if (top_left->latitude >= GRN_GEO_MAX_LATITUDE) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s: top left point's latitude is too big: "
          "<%d>(max:%d): (%d,%d) (%d,%d)",
          process_name,
          GRN_GEO_MAX_LATITUDE, top_left->latitude,
          top_left->latitude, top_left->longitude,
          bottom_right->latitude, bottom_right->longitude);
      goto exit;
    }

    if (top_left->longitude >= GRN_GEO_MAX_LONGITUDE) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s: top left point's longitude is too big: "
          "<%d>(max:%d): (%d,%d) (%d,%d)",
          process_name,
          GRN_GEO_MAX_LONGITUDE, top_left->longitude,
          top_left->latitude, top_left->longitude,
          bottom_right->latitude, bottom_right->longitude);
      goto exit;
    }

    if (bottom_right->latitude >= GRN_GEO_MAX_LATITUDE) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s: bottom right point's latitude is too big: "
          "<%d>(max:%d): (%d,%d) (%d,%d)",
          process_name,
          GRN_GEO_MAX_LATITUDE, bottom_right->latitude,
          top_left->latitude, top_left->longitude,
          bottom_right->latitude, bottom_right->longitude);
      goto exit;
    }

    if (bottom_right->longitude >= GRN_GEO_MAX_LONGITUDE) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s: bottom right point's longitude is too big: "
          "<%d>(max:%d): (%d,%d) (%d,%d)",
          process_name,
          GRN_GEO_MAX_LONGITUDE, bottom_right->longitude,
          top_left->latitude, top_left->longitude,
          bottom_right->latitude, bottom_right->longitude);
      goto exit;
    }
  }

  {
    int latitude_distance, longitude_distance;
    grn_geo_point *top_left, *bottom_right;
    grn_geo_point *geo_point_input;
    uint8_t geo_key_input[sizeof(grn_geo_point)];
    uint8_t geo_key_base[sizeof(grn_geo_point)];
    uint8_t geo_key_top_left[sizeof(grn_geo_point)];
    uint8_t geo_key_bottom_right[sizeof(grn_geo_point)];

    top_left = data->top_left;
    bottom_right = data->bottom_right;

    latitude_distance = top_left->latitude - bottom_right->latitude;
    longitude_distance = bottom_right->longitude - top_left->longitude;
    if (latitude_distance > longitude_distance) {
      data->direction = GRN_GEO_MESH_LATITUDE;
      geo_point_input = bottom_right;
      data->base.latitude = bottom_right->latitude;
      data->base.longitude = bottom_right->longitude - longitude_distance;
    } else {
      data->direction = GRN_GEO_MESH_LONGITUDE;
      geo_point_input = top_left;
      data->base.latitude = top_left->latitude - latitude_distance;
      data->base.longitude = top_left->longitude;
    }
    grn_gton(geo_key_input, geo_point_input, sizeof(grn_geo_point));
    grn_gton(geo_key_base, &(data->base), sizeof(grn_geo_point));
    data->diff_bit = compute_diff_bit(geo_key_input, geo_key_base) - 1;
    grn_gton(geo_key_top_left, top_left, sizeof(grn_geo_point));
    grn_gton(geo_key_bottom_right, bottom_right, sizeof(grn_geo_point));
    data->rectangle_common_bit =
      compute_diff_bit(geo_key_top_left, geo_key_bottom_right) - 1;
    compute_min_and_max(&(data->base), data->diff_bit,
                        &(data->min), &(data->max));
    switch (data->direction) {
    case GRN_GEO_MESH_LATITUDE :
      data->distance = data->max.latitude - data->min.latitude + 1;
      data->start = data->min.latitude;
      data->end = top_left->latitude;
      break;
    case GRN_GEO_MESH_LONGITUDE :
      data->distance = data->max.longitude - data->min.longitude + 1;
      data->start = data->min.longitude;
      data->end = bottom_right->longitude;
      break;
    }
#ifdef GEO_DEBUG
    printf("direction: %s\n",
           data->direction == GRN_GEO_MESH_LATITUDE ? "latitude" : "longitude");
    printf("base:         ");
    grn_p_geo_point(ctx, &(data->base));
    printf("input:        ");
    grn_p_geo_point(ctx, geo_point_input);
    printf("min:          ");
    grn_p_geo_point(ctx, &(data->min));
    printf("max:          ");
    grn_p_geo_point(ctx, &(data->max));
    printf("top-left:     ");
    grn_p_geo_point(ctx, top_left);
    printf("bottom-right: ");
    grn_p_geo_point(ctx, bottom_right);
    printf("diff-bit:            %10d\n", data->diff_bit);
    printf("rectangle-common-bit:%10d\n", data->rectangle_common_bit);
    printf("start:               %10d\n", data->start);
    printf("end:                 %10d\n", data->end);
    printf("distance:            %10d\n", data->distance);
    printf("distance(latitude):  %10d\n", latitude_distance);
    printf("distance(longitude): %10d\n", longitude_distance);
#endif
    memcpy(&(data->base), &(data->min), sizeof(grn_geo_point));
  }

exit :
  return ctx->rc;
}

#define SAME_BIT_P(a, b, n_bit)\
  ((((uint8_t *)(a))[(n_bit) / 8] & (1 << (7 - ((n_bit) % 8)))) ==\
   (((uint8_t *)(b))[(n_bit) / 8] & (1 << (7 - ((n_bit) % 8)))))

#define ENTRY_CHECK_KEY(entry, key)\
  SAME_BIT_P((entry)->base_key, (key), (entry)->target_bit)

#define SET_N_BIT(a, n_bit)\
  ((uint8_t *)(a))[((n_bit) / 8)] ^= (1 << (7 - ((n_bit) % 8)))

#define N_BIT(a, n_bit)\
  ((((uint8_t *)(a))[((n_bit) / 8)] &\
    (1 << (7 - ((n_bit) % 8)))) >> (1 << (7 - ((n_bit) % 8))))

grn_obj *
grn_geo_cursor_open_in_rectangle(grn_ctx *ctx,
                                 grn_obj *index,
                                 grn_obj *top_left_point,
                                 grn_obj *bottom_right_point,
                                 int offset,
                                 int limit)
{
  grn_geo_cursor_in_rectangle *cursor = NULL;
  in_rectangle_data data;

  GRN_VOID_INIT(&(data.top_left_point_buffer));
  GRN_VOID_INIT(&(data.bottom_right_point_buffer));
  if (in_rectangle_data_prepare(ctx, index, top_left_point, bottom_right_point,
                                "geo_in_rectangle()", &data)) {
    goto exit;
  }

  cursor = GRN_MALLOCN(grn_geo_cursor_in_rectangle, 1);
  if (!cursor) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[geo][cursor][in-rectangle] failed to allocate memory for geo cursor");
    goto exit;
  }

  GRN_DB_OBJ_SET_TYPE(cursor, GRN_CURSOR_COLUMN_GEO_INDEX);
  cursor->pat = data.pat;
  cursor->index = index;
  cursor->diff_bit = data.diff_bit;
  cursor->start_mesh_point = data.start;
  cursor->end_mesh_point = data.end;
  cursor->distance = data.distance;
  cursor->direction = data.direction;
  memcpy(&(cursor->top_left), data.top_left, sizeof(grn_geo_point));
  memcpy(&(cursor->bottom_right), data.bottom_right, sizeof(grn_geo_point));
  grn_gton(cursor->top_left_key, data.top_left, sizeof(grn_geo_point));
  grn_gton(cursor->bottom_right_key, data.bottom_right, sizeof(grn_geo_point));
  memcpy(&(cursor->base), &(data.base), sizeof(grn_geo_point));
  cursor->pat_cursor = NULL;
  cursor->ii_cursor = NULL;
  cursor->offset = offset;
  cursor->rest = limit;

  cursor->current_entry = 0;
  {
    grn_geo_cursor_entry *entry;
    entry = &(cursor->entries[cursor->current_entry]);
    entry->target_bit = data.rectangle_common_bit;
    entry->top_included = GRN_TRUE;
    entry->bottom_included = GRN_TRUE;
    entry->left_included = GRN_TRUE;
    entry->right_included = GRN_TRUE;
    entry->latitude_inner = GRN_FALSE;
    entry->longitude_inner = GRN_FALSE;
    grn_gton(entry->base_key, &(data.base), sizeof(grn_geo_point));
  }

exit :
  grn_obj_unlink(ctx, &(data.top_left_point_buffer));
  grn_obj_unlink(ctx, &(data.bottom_right_point_buffer));
  return (grn_obj *)cursor;
}

static inline grn_bool
grn_geo_cursor_entry_next_push(grn_ctx *ctx,
                               grn_geo_cursor_in_rectangle *cursor,
                               grn_geo_cursor_entry *entry)
{
  grn_geo_cursor_entry *next_entry;
  grn_geo_point entry_base;
  grn_table_cursor *pat_cursor;
  grn_bool pushed = GRN_FALSE;

  grn_ntog((uint8_t*)(&entry_base), entry->base_key, sizeof(grn_geo_point));
  pat_cursor = grn_table_cursor_open(ctx,
                                     cursor->pat,
                                     &entry_base,
                                     entry->target_bit + 1,
                                     NULL, 0,
                                     0, -1,
                                     GRN_CURSOR_PREFIX|GRN_CURSOR_SIZE_BY_BIT);
  if (pat_cursor) {
    if (grn_table_cursor_next(ctx, pat_cursor)) {
      next_entry = &(cursor->entries[++cursor->current_entry]);
      memcpy(next_entry, entry, sizeof(grn_geo_cursor_entry));
      pushed = GRN_TRUE;
    }
    grn_table_cursor_close(ctx, pat_cursor);
  }

  return pushed;
}

static inline grn_bool
grn_geo_cursor_entry_next(grn_ctx *ctx,
                          grn_geo_cursor_in_rectangle *cursor,
                          grn_geo_cursor_entry *entry)
{
  uint8_t *top_left_key = cursor->top_left_key;
  uint8_t *bottom_right_key = cursor->bottom_right_key;

  if (cursor->current_entry < 0) {
    return GRN_FALSE;
  }

  memcpy(entry,
         &(cursor->entries[cursor->current_entry--]),
         sizeof(grn_geo_cursor_entry));
  while (GRN_TRUE) {
    grn_geo_cursor_entry next_entry0, next_entry1;
    grn_bool pushed = GRN_FALSE;

#ifdef GEO_DEBUG
    inspect_cursor_entry(ctx, entry);
#endif

    if (entry->target_bit >= 55) {
#ifdef GEO_DEBUG
      printf("only 1 entry is remained\n");
#endif
      break;
    }

    if (entry->latitude_inner && entry->longitude_inner) {
#ifdef GEO_DEBUG
      printf("%d: inner entries\n", entry->target_bit);
#endif
      break;
    }

    memcpy(&next_entry0, entry, sizeof(grn_geo_cursor_entry));
    next_entry0.target_bit++;
    memcpy(&next_entry1, entry, sizeof(grn_geo_cursor_entry));
    next_entry1.target_bit++;
    SET_N_BIT(next_entry1.base_key, next_entry1.target_bit);

#ifdef GEO_DEBUG
    inspect_cursor_entry_targets(ctx, entry, top_left_key, bottom_right_key,
                                 &next_entry0, &next_entry1);
#endif

    if ((entry->target_bit + 1) % 2 == 0) {
      if (entry->top_included) {
        next_entry0.top_included = ENTRY_CHECK_KEY(&next_entry0, top_left_key);
        next_entry1.top_included = ENTRY_CHECK_KEY(&next_entry1, top_left_key);
      }
      if (entry->bottom_included) {
        next_entry0.bottom_included =
          ENTRY_CHECK_KEY(&next_entry0, bottom_right_key);
        next_entry1.bottom_included =
          ENTRY_CHECK_KEY(&next_entry1, bottom_right_key);
      }

      if (entry->top_included && !entry->bottom_included &&
          next_entry1.top_included) {
        next_entry0.latitude_inner = GRN_TRUE;
      } else if (!entry->top_included && entry->bottom_included &&
                 next_entry0.bottom_included) {
        next_entry1.latitude_inner = GRN_TRUE;
      }

      if (next_entry0.latitude_inner ||
          next_entry0.top_included || next_entry0.bottom_included) {
        if (grn_geo_cursor_entry_next_push(ctx, cursor, &next_entry0)) {
          pushed = GRN_TRUE;
#ifdef GEO_DEBUG
          printf("%d: latitude: push 0\n", next_entry0.target_bit);
#endif
        }
      }
      if (next_entry1.latitude_inner ||
          next_entry1.top_included || next_entry1.bottom_included) {
        if (grn_geo_cursor_entry_next_push(ctx, cursor, &next_entry1)) {
          pushed = GRN_TRUE;
#ifdef GEO_DEBUG
          printf("%d: latitude: push 1\n", next_entry1.target_bit);
#endif
        }
      }
    } else {
      if (entry->right_included) {
        next_entry0.right_included =
          ENTRY_CHECK_KEY(&next_entry0, bottom_right_key);
        next_entry1.right_included =
          ENTRY_CHECK_KEY(&next_entry1, bottom_right_key);
      }
      if (entry->left_included) {
        next_entry0.left_included = ENTRY_CHECK_KEY(&next_entry0, top_left_key);
        next_entry1.left_included = ENTRY_CHECK_KEY(&next_entry1, top_left_key);
      }

      if (entry->left_included && !entry->right_included &&
          next_entry0.left_included) {
        next_entry1.longitude_inner = GRN_TRUE;
      } else if (!entry->left_included && entry->right_included &&
                 next_entry1.right_included) {
        next_entry0.longitude_inner = GRN_TRUE;
      }

      if (next_entry0.longitude_inner ||
          next_entry0.left_included || next_entry0.right_included) {
        if (grn_geo_cursor_entry_next_push(ctx, cursor, &next_entry0)) {
          pushed = GRN_TRUE;
#ifdef GEO_DEBUG
          printf("%d: longitude: push 0\n", next_entry0.target_bit);
#endif
        }
      }
      if (next_entry1.longitude_inner ||
          next_entry1.left_included || next_entry1.right_included) {
        if (grn_geo_cursor_entry_next_push(ctx, cursor, &next_entry1)) {
          pushed = GRN_TRUE;
#ifdef GEO_DEBUG
          printf("%d: longitude: push 1\n", next_entry1.target_bit);
#endif
        }
      }
    }

    if (pushed) {
#ifdef GEO_DEBUG
      int i;

      printf("%d: pushed\n", entry->target_bit);
      printf("stack:\n");
      for (i = cursor->current_entry; i >= 0; i--) {
        grn_geo_cursor_entry *stack_entry;
        stack_entry = &(cursor->entries[i]);
        printf("%2d: ", i);
        inspect_key(ctx, stack_entry->base_key);
        printf("    ");
        print_key_mark(ctx, stack_entry->target_bit);
      }
#endif
      memcpy(entry,
             &(cursor->entries[cursor->current_entry--]),
             sizeof(grn_geo_cursor_entry));
#ifdef GEO_DEBUG
      printf("%d: pop entry\n", entry->target_bit);
#endif
    } else {
      break;
    }
  }

#ifdef GEO_DEBUG
  printf("found:\n");
  inspect_cursor_entry(ctx, entry);
#endif

  return GRN_TRUE;
}

typedef grn_bool (*grn_geo_cursor_callback)(grn_ctx *ctx, grn_ii_posting *posting, void *user_data);

static void
grn_geo_cursor_each_strictly(grn_ctx *ctx, grn_obj *geo_cursor,
                             grn_geo_cursor_callback callback, void *user_data)
{
  grn_geo_cursor_in_rectangle *cursor;
  grn_obj *pat;
  grn_table_cursor *pat_cursor;
  grn_ii *ii;
  grn_ii_cursor *ii_cursor;
  grn_ii_posting *posting = NULL;
  grn_geo_point *current, *base, *top_left, *bottom_right;
  int diff_bit, distance, end_mesh_point;
  grn_geo_mesh_direction direction;
  grn_id index_id;

  cursor = (grn_geo_cursor_in_rectangle *)geo_cursor;
  if (cursor->rest == 0) {
    return;
  }

  pat = cursor->pat;
  pat_cursor = cursor->pat_cursor;
  ii = (grn_ii *)(cursor->index);
  ii_cursor = cursor->ii_cursor;
  current = &(cursor->current);
  base = &(cursor->base);
  top_left = &(cursor->top_left);
  bottom_right = &(cursor->bottom_right);
  diff_bit = cursor->diff_bit;
  distance = cursor->distance;
  end_mesh_point = cursor->end_mesh_point;
  direction = cursor->direction;

  while (GRN_TRUE) {
    if (!pat_cursor) {
      grn_geo_cursor_entry entry;
      grn_geo_point entry_base;
      if (!grn_geo_cursor_entry_next(ctx, cursor, &entry)) {
        cursor->rest = 0;
        return;
      }
      grn_ntog((uint8_t*)(&entry_base), entry.base_key, sizeof(grn_geo_point));
      if (!(cursor->pat_cursor = pat_cursor =
            grn_table_cursor_open(ctx,
                                  pat,
                                  &entry_base,
                                  entry.target_bit + 1,
                                  NULL, 0,
                                  0, -1,
                                  GRN_CURSOR_PREFIX|GRN_CURSOR_SIZE_BY_BIT))) {
        cursor->rest = 0;
        return;
      }
#ifdef GEO_DEBUG
      {
        inspect_mesh(ctx, &entry_base, entry.target_bit, 0);
      }
#endif
    }

    while (ii_cursor || (index_id = grn_table_cursor_next(ctx, pat_cursor))) {
      if (!ii_cursor) {
        grn_table_get_key(ctx, pat, index_id, current, sizeof(grn_geo_point));
        if (grn_geo_in_rectangle_raw(ctx, current, top_left, bottom_right)) {
          inspect_tid(ctx, index_id, current, 0);
          if (!(cursor->ii_cursor = ii_cursor =
                grn_ii_cursor_open(ctx,
                                   ii,
                                   index_id,
                                   GRN_ID_NIL,
                                   GRN_ID_MAX,
                                   ii->n_elements,
                                   0))) {
            continue;
          }
        } else {
          continue;
        }
      }

      while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
        if (cursor->offset == 0) {
          grn_bool keep_each;
          keep_each = callback(ctx, posting, user_data);
          if (cursor->rest > 0) {
            if (--(cursor->rest) == 0) {
              keep_each = GRN_FALSE;
            }
          }
          if (!keep_each) {
            return;
          }
        } else {
          cursor->offset--;
        }
      }
      grn_ii_cursor_close(ctx, ii_cursor);
      cursor->ii_cursor = ii_cursor = NULL;
    }
    grn_table_cursor_close(ctx, pat_cursor);
    cursor->pat_cursor = pat_cursor = NULL;
  }
}

static void
grn_geo_cursor_each_loose(grn_ctx *ctx, grn_obj *geo_cursor,
                          grn_geo_cursor_callback callback, void *user_data)
{
  grn_geo_cursor_in_rectangle *cursor;
  grn_obj *pat;
  grn_table_cursor *pat_cursor;
  grn_ii *ii;
  grn_ii_cursor *ii_cursor;
  grn_ii_posting *posting = NULL;
  grn_geo_point *current, *base, *top_left, *bottom_right;
  int diff_bit, distance, end_mesh_point;
  grn_geo_mesh_direction direction;
  int mesh_point = 0;
  grn_id index_id;

  cursor = (grn_geo_cursor_in_rectangle *)geo_cursor;
  if (cursor->rest == 0) {
    return;
  }

  pat = cursor->pat;
  pat_cursor = cursor->pat_cursor;
  ii = (grn_ii *)(cursor->index);
  ii_cursor = cursor->ii_cursor;
  current = &(cursor->current);
  base = &(cursor->base);
  top_left = &(cursor->top_left);
  bottom_right = &(cursor->bottom_right);
  diff_bit = cursor->diff_bit;
  distance = cursor->distance;
  end_mesh_point = cursor->end_mesh_point;
  direction = cursor->direction;

  while (GRN_TRUE) {
    if (!pat_cursor) {
      if (!(cursor->pat_cursor = pat_cursor =
            grn_table_cursor_open(ctx,
                                  pat,
                                  base,
                                  diff_bit,
                                  NULL, 0,
                                  0, -1,
                                  GRN_CURSOR_PREFIX|GRN_CURSOR_SIZE_BY_BIT))) {
        cursor->rest = 0;
        return;
      }
#ifdef GEO_DEBUG
      {
        switch (direction) {
        case GRN_GEO_MESH_LATITUDE :
          mesh_point = base->latitude;
          break;
        case GRN_GEO_MESH_LONGITUDE :
          mesh_point = base->longitude;
          break;
        }
        printf("mesh-point:          %10d\n", mesh_point);
        inspect_mesh(ctx, base, diff_bit,
                     (mesh_point - cursor->start_mesh_point) /
                     distance);
      }
#endif
    }

    while (ii_cursor || (index_id = grn_table_cursor_next(ctx, pat_cursor))) {
      if (!ii_cursor) {
        grn_table_get_key(ctx, pat, index_id, current, sizeof(grn_geo_point));
        if (grn_geo_in_rectangle_raw(ctx, current, top_left, bottom_right)) {
          inspect_tid(ctx, index_id, current, 0);
          if (!(cursor->ii_cursor = ii_cursor =
                grn_ii_cursor_open(ctx,
                                   ii,
                                   index_id,
                                   GRN_ID_NIL,
                                   GRN_ID_MAX,
                                   ii->n_elements,
                                   0))) {
            continue;
          }
        } else {
          continue;
        }
      }

      while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
        if (cursor->offset == 0) {
          grn_bool keep_each;
          keep_each = callback(ctx, posting, user_data);
          if (cursor->rest > 0) {
            if (--(cursor->rest) == 0) {
              keep_each = GRN_FALSE;
            }
          }
          if (!keep_each) {
            return;
          }
        } else {
          cursor->offset--;
        }
      }
      grn_ii_cursor_close(ctx, ii_cursor);
      cursor->ii_cursor = ii_cursor = NULL;
    }
    grn_table_cursor_close(ctx, pat_cursor);
    cursor->pat_cursor = pat_cursor = NULL;

    switch (direction) {
    case GRN_GEO_MESH_LATITUDE :
      mesh_point = (base->latitude += distance);
      break;
    case GRN_GEO_MESH_LONGITUDE :
      mesh_point = (base->longitude += distance);
      break;
    }
    if (mesh_point > end_mesh_point + distance) {
      cursor->rest = 0;
      return;
    }
  }
}

static void
grn_geo_cursor_each(grn_ctx *ctx, grn_obj *geo_cursor,
                    grn_geo_cursor_callback callback, void *user_data)
{
  if (getenv("GRN_GEO_CURSOR_STRICTLY")) {
    grn_geo_cursor_each_strictly(ctx, geo_cursor, callback, user_data);
  } else {
    grn_geo_cursor_each_loose(ctx, geo_cursor, callback, user_data);
  }
}

static grn_bool
grn_geo_cursor_next_callback(grn_ctx *ctx, grn_ii_posting *posting,
                             void *user_data)
{
  grn_ii_posting **return_posting = user_data;
  *return_posting = posting;
  return GRN_FALSE;
}

grn_posting *
grn_geo_cursor_next(grn_ctx *ctx, grn_obj *geo_cursor)
{
  grn_ii_posting *posting = NULL;
  grn_geo_cursor_each(ctx, geo_cursor, grn_geo_cursor_next_callback, &posting);
  return (grn_posting *)posting;
}

grn_rc
grn_geo_cursor_close(grn_ctx *ctx, grn_obj *geo_cursor)
{
  grn_geo_cursor_in_rectangle *cursor;

  if (!geo_cursor) { return GRN_INVALID_ARGUMENT; }

  cursor = (grn_geo_cursor_in_rectangle *)geo_cursor;
  if (cursor->pat) { grn_obj_unlink(ctx, cursor->pat); }
  if (cursor->index) { grn_obj_unlink(ctx, cursor->index); }
  if (cursor->pat_cursor) { grn_table_cursor_close(ctx, cursor->pat_cursor); }
  if (cursor->ii_cursor) { grn_ii_cursor_close(ctx, cursor->ii_cursor); }
  GRN_FREE(cursor);

  return GRN_SUCCESS;
}

typedef struct {
  grn_hash *res;
  grn_operator op;
} grn_geo_select_in_rectangle_data;

static grn_bool
grn_geo_select_in_rectangle_callback(grn_ctx *ctx, grn_ii_posting *posting,
                                     void *user_data)
{
  grn_geo_select_in_rectangle_data *data = user_data;
  grn_ii_posting_add(ctx, posting, data->res, data->op);
  return GRN_TRUE;
}

grn_rc
grn_geo_select_in_rectangle(grn_ctx *ctx, grn_obj *index,
                            grn_obj *top_left_point,
                            grn_obj *bottom_right_point,
                            grn_obj *res, grn_operator op)
{
  grn_obj *cursor;

  cursor = grn_geo_cursor_open_in_rectangle(ctx, index,
                                            top_left_point, bottom_right_point,
                                            0, -1);
  if (cursor) {
    grn_geo_select_in_rectangle_data data;
    data.res = (grn_hash *)res;
    data.op = op;
    grn_geo_cursor_each(ctx, cursor, grn_geo_select_in_rectangle_callback,
                        &data);
    grn_obj_unlink(ctx, cursor);
    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  }

  return ctx->rc;
}

static grn_rc
geo_point_get(grn_ctx *ctx, grn_obj *pat, int flags, grn_geo_point *geo_point)
{
  grn_rc rc = GRN_SUCCESS;
  grn_id id;
  grn_table_cursor *cursor = NULL;

  cursor = grn_table_cursor_open(ctx, pat,
                                 NULL, 0,
                                 NULL, 0,
                                 0, 1,
                                 GRN_CURSOR_BY_KEY | flags);
  if (!cursor) {
    rc = ctx->rc;
    goto exit;
  }

  id = grn_table_cursor_next(ctx, cursor);
  if (id == GRN_ID_NIL) {
    rc = GRN_END_OF_DATA;
  } else {
    void *key;
    int key_size;
    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    memcpy(geo_point, key, key_size);
  }

exit:
  if (cursor) {
    grn_table_cursor_close(ctx, cursor);
  }
  return rc;
}

int
grn_geo_estimate_in_rectangle(grn_ctx *ctx,
                              grn_obj *index,
                              grn_obj *top_left_point,
                              grn_obj *bottom_right_point)
{
  int n = 0;
  int total_records;
  grn_rc rc;
  in_rectangle_data data;

  GRN_VOID_INIT(&(data.top_left_point_buffer));
  GRN_VOID_INIT(&(data.bottom_right_point_buffer));
  if (in_rectangle_data_prepare(ctx, index, top_left_point, bottom_right_point,
                                "grn_geo_estimate_in_rectangle()", &data)) {
    n = -1;
    goto exit;
  }

  total_records = grn_table_size(ctx, data.pat);
  if (total_records > 0) {
    grn_geo_point min, max;
    int select_latitude_distance, select_longitude_distance;
    int total_latitude_distance, total_longitude_distance;
    double select_ratio;
    double estimated_n_records;

    rc = geo_point_get(ctx, data.pat, GRN_CURSOR_ASCENDING, &min);
    if (!rc) {
      rc = geo_point_get(ctx, data.pat, GRN_CURSOR_DESCENDING, &max);
    }
    if (rc) {
      if (rc == GRN_END_OF_DATA) {
        n = total_records;
        rc = GRN_SUCCESS;
      } else {
        n = -1;
      }
      goto exit;
    }

    select_latitude_distance = abs(data.max.latitude - data.min.latitude);
    select_longitude_distance = abs(data.max.longitude - data.min.longitude);
    total_latitude_distance = abs(max.latitude - min.latitude);
    total_longitude_distance = abs(max.longitude - min.longitude);

    select_ratio = 1.0;
    if (select_latitude_distance < total_latitude_distance) {
      select_ratio *= ((double)select_latitude_distance /
                       (double)total_latitude_distance);
    }
    if (select_longitude_distance < total_longitude_distance) {
      select_ratio *= ((double)select_longitude_distance /
                       (double)total_longitude_distance);
    }
    estimated_n_records = ceil(total_records * select_ratio);
    n = (int)estimated_n_records;
  }

exit :
  grn_obj_unlink(ctx, &(data.top_left_point_buffer));
  grn_obj_unlink(ctx, &(data.bottom_right_point_buffer));
  return n;
}

grn_bool
grn_geo_in_circle(grn_ctx *ctx, grn_obj *point, grn_obj *center,
                  grn_obj *radius_or_point)
{
  grn_bool r = GRN_FALSE;
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

grn_bool
grn_geo_in_rectangle_raw(grn_ctx *ctx, grn_geo_point *point,
                         grn_geo_point *top_left, grn_geo_point *bottom_right)
{
    return ((top_left->longitude <= point->longitude) &&
            (point->longitude <= bottom_right->longitude) &&
            (bottom_right->latitude <= point->latitude) &&
            (point->latitude <= top_left->latitude));
}

grn_bool
grn_geo_in_rectangle(grn_ctx *ctx, grn_obj *point,
                     grn_obj *top_left, grn_obj *bottom_right)
{
  grn_bool r = GRN_FALSE;
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
  grn_bool point1_initialized = GRN_FALSE;
  grn_bool point2_initialized = GRN_FALSE;
  grn_obj point1_, point2_;
  grn_id domain1 = point1->header.domain;
  grn_id domain2 = point2->header.domain;
  if (domain1 == GRN_DB_TOKYO_GEO_POINT || domain1 == GRN_DB_WGS84_GEO_POINT) {
    if (domain1 != domain2) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain1);
      point2_initialized = GRN_TRUE;
      if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
      point2 = &point2_;
    }
  } else if (domain2 == GRN_DB_TOKYO_GEO_POINT ||
             domain2 == GRN_DB_WGS84_GEO_POINT) {
    GRN_OBJ_INIT(&point1_, GRN_BULK, 0, domain2);
    point1_initialized = GRN_TRUE;
    if (grn_obj_cast(ctx, point1, &point1_, 0)) { goto exit; }
    point1 = &point1_;
  } else if ((GRN_DB_SHORT_TEXT <= domain1 && domain1 <= GRN_DB_LONG_TEXT) &&
             (GRN_DB_SHORT_TEXT <= domain2 && domain2 <= GRN_DB_LONG_TEXT)) {
    GRN_OBJ_INIT(&point1_, GRN_BULK, 0, GRN_DB_WGS84_GEO_POINT);
    point1_initialized = GRN_TRUE;
    if (grn_obj_cast(ctx, point1, &point1_, 0)) { goto exit; }
    point1 = &point1_;

    GRN_OBJ_INIT(&point2_, GRN_BULK, 0, GRN_DB_WGS84_GEO_POINT);
    point2_initialized = GRN_TRUE;
    if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
    point2 = &point2_;
  } else {
    goto exit;
  }
  d = grn_geo_distance_raw(ctx,
                           GRN_GEO_POINT_VALUE_RAW(point1),
                           GRN_GEO_POINT_VALUE_RAW(point2));
exit :
  if (point1_initialized) {
    GRN_OBJ_FIN(ctx, &point1_);
  }
  if (point2_initialized) {
    GRN_OBJ_FIN(ctx, &point2_);
  }
  return d;
}

double
grn_geo_distance2(grn_ctx *ctx, grn_obj *point1, grn_obj *point2)
{
  double d = 0;
  grn_bool point2_initialized = GRN_FALSE;
  grn_obj point2_;
  grn_id domain = point1->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    if (point2->header.domain != domain) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain);
      point2_initialized = GRN_TRUE;
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
  if (point2_initialized) {
    GRN_OBJ_FIN(ctx, &point2_);
  }
  return d;
}

double
grn_geo_distance3(grn_ctx *ctx, grn_obj *point1, grn_obj *point2)
{
  double d = 0;
  grn_bool point2_initialized = GRN_FALSE;
  grn_obj point2_;
  grn_id domain = point1->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    if (point2->header.domain != domain) {
      GRN_OBJ_INIT(&point2_, GRN_BULK, 0, domain);
      point2_initialized = GRN_TRUE;
      if (grn_obj_cast(ctx, point2, &point2_, 0)) { goto exit; }
      point2 = &point2_;
    }
    if (domain == GRN_DB_TOKYO_GEO_POINT) {
      d = grn_geo_distance3_raw(ctx,
                                GRN_GEO_POINT_VALUE_RAW(point1),
                                GRN_GEO_POINT_VALUE_RAW(point2),
                                GRN_GEO_BES_C1, GRN_GEO_BES_C2, GRN_GEO_BES_C3);
    } else {
      d = grn_geo_distance3_raw(ctx,
                                GRN_GEO_POINT_VALUE_RAW(point1),
                                GRN_GEO_POINT_VALUE_RAW(point2),
                                GRN_GEO_GRS_C1, GRN_GEO_GRS_C2, GRN_GEO_GRS_C3);
    }
  } else {
    /* todo */
  }
exit :
  if (point2_initialized) {
    GRN_OBJ_FIN(ctx, &point2_);
  }
  return d;
}
