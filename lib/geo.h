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
#ifndef GRN_GEO_H
#define GRN_GEO_H

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

#include "ii.h"
#include "db.h"

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif /* WIN32 */
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_GEO_RESOLUTION   3600000
#define GRN_GEO_RADIUS       6357303
#define GRN_GEO_BES_C1       6334834
#define GRN_GEO_BES_C2       6377397
#define GRN_GEO_BES_C3       0.006674
#define GRN_GEO_GRS_C1       6335439
#define GRN_GEO_GRS_C2       6378137
#define GRN_GEO_GRS_C3       0.006694
#define GRN_GEO_INT2RAD(x)   ((M_PI / (GRN_GEO_RESOLUTION * 180)) * (x))
#define GRN_GEO_RAD2INT(x)   ((int)(((GRN_GEO_RESOLUTION * 180) / M_PI) * (x)))

#define GRN_GEO_MAX_LATITUDE  324000000 /*  90 * 60 * 60 * 1000 */
#define GRN_GEO_MAX_LONGITUDE 648000000 /* 180 * 60 * 60 * 1000 */

#define GRN_GEO_POINT_VALUE_RAW(obj) (grn_geo_point *)GRN_BULK_HEAD(obj)
#define GRN_GEO_POINT_VALUE_RADIUS(obj,_latitude,_longitude) do {\
  grn_geo_point *_val = (grn_geo_point *)GRN_BULK_HEAD(obj);\
  _latitude = GRN_GEO_INT2RAD(_val->latitude);\
  _longitude = GRN_GEO_INT2RAD(_val->longitude);\
} while (0)

typedef enum _grn_geo_mesh_direction grn_geo_mesh_direction;
enum _grn_geo_mesh_direction {
  GRN_GEO_MESH_LATITUDE,
  GRN_GEO_MESH_LONGITUDE
};

typedef struct _grn_geo_cursor_entry grn_geo_cursor_entry;
struct _grn_geo_cursor_entry {
  uint8_t base_key[sizeof(grn_geo_point)];
  grn_bool top_included;
  grn_bool bottom_included;
  grn_bool left_included;
  grn_bool right_included;
  grn_bool latitude_inner;
  grn_bool longitude_inner;
  int target_bit;
};

typedef struct _grn_geo_cursor_in_rectangle grn_geo_cursor_in_rectangle;
struct _grn_geo_cursor_in_rectangle {
  grn_db_obj obj;
  grn_obj *pat;
  grn_obj *index;
  int diff_bit;
  int start_mesh_point;
  int end_mesh_point;
  int distance;
  grn_geo_mesh_direction direction;
  grn_geo_point top_left;
  grn_geo_point bottom_right;
  uint8_t top_left_key[sizeof(grn_geo_point)];
  uint8_t bottom_right_key[sizeof(grn_geo_point)];
  grn_geo_point base;
  grn_geo_point current;
  grn_table_cursor *pat_cursor;
  grn_ii_cursor *ii_cursor;
  int offset;
  int rest;
  grn_geo_cursor_entry entries[64];
  int current_entry;
  int minimum_reduce_bit;
};

grn_rc grn_geo_cursor_close(grn_ctx *ctx, grn_obj *geo_cursor);


int grn_geo_table_sort(grn_ctx *ctx, grn_obj *table, int offset, int limit,
                       grn_obj *result, grn_table_sort_key *keys, int n_keys);

grn_rc grn_selector_geo_in_circle(grn_ctx *ctx, grn_obj *obj, grn_obj **args,
                                  int nargs, grn_obj *res, grn_operator op);
grn_rc grn_selector_geo_in_rectangle(grn_ctx *ctx, grn_obj *obj, grn_obj **args,
                                     int nargs, grn_obj *res, grn_operator op);

grn_bool grn_geo_in_circle(grn_ctx *ctx, grn_obj *point, grn_obj *center,
                           grn_obj *radius_or_point);
grn_bool grn_geo_in_rectangle(grn_ctx *ctx, grn_obj *point,
                              grn_obj *top_left, grn_obj *bottom_right);
grn_bool grn_geo_in_rectangle_raw(grn_ctx *ctx, grn_geo_point *point,
                                  grn_geo_point *top_left,
                                  grn_geo_point *bottom_right);
double grn_geo_distance(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance2(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance3(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance_raw(grn_ctx *ctx, grn_geo_point *point1, grn_geo_point *point2);
double grn_geo_distance2_raw(grn_ctx *ctx, grn_geo_point *point1, grn_geo_point *point2);
double grn_geo_distance3_raw(grn_ctx *ctx, grn_geo_point *point1, grn_geo_point *point2,
                             int c1, int c2, double c3);

#ifdef __cplusplus
}
#endif

#endif /* GRN_GEO_H */
