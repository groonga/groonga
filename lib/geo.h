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

#define GRN_GEO_KEY_MAX_BITS 64

typedef enum {
  GRN_GEO_APPROXIMATE_RECTANGLE,
  GRN_GEO_APPROXIMATE_SPHERE,
  GRN_GEO_APPROXIMATE_ELLIPSOID
} grn_geo_approximate_type;

typedef enum {
  GRN_GEO_CURSOR_ENTRY_STATUS_NONE            = 0,
  GRN_GEO_CURSOR_ENTRY_STATUS_TOP_INCLUDED    = 1 << 0,
  GRN_GEO_CURSOR_ENTRY_STATUS_BOTTOM_INCLUDED = 1 << 1,
  GRN_GEO_CURSOR_ENTRY_STATUS_LEFT_INCLUDED   = 1 << 2,
  GRN_GEO_CURSOR_ENTRY_STATUS_RIGHT_INCLUDED  = 1 << 3,
  GRN_GEO_CURSOR_ENTRY_STATUS_LATITUDE_INNER  = 1 << 4,
  GRN_GEO_CURSOR_ENTRY_STATUS_LONGITUDE_INNER = 1 << 5
} grn_geo_cursor_entry_status_flag;

typedef struct {
  uint8_t key[sizeof(grn_geo_point)];
  int target_bit;
  int status_flags;
} grn_geo_cursor_entry;

typedef struct {
  grn_db_obj obj;
  grn_obj *pat;
  grn_obj *index;
  grn_geo_point top_left;
  grn_geo_point bottom_right;
  uint8_t top_left_key[sizeof(grn_geo_point)];
  uint8_t bottom_right_key[sizeof(grn_geo_point)];
  grn_geo_point current;
  grn_table_cursor *pat_cursor;
  grn_ii_cursor *ii_cursor;
  int offset;
  int rest;
  grn_geo_cursor_entry entries[GRN_GEO_KEY_MAX_BITS];
  int current_entry;
  int minimum_reduce_bit;
} grn_geo_cursor_in_rectangle;

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
double grn_geo_distance(grn_ctx *ctx, grn_obj *point1, grn_obj *point2,
                        grn_geo_approximate_type type);
double grn_geo_distance_rectangle(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance_sphere(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance_ellipsoid(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance_rectangle_raw(grn_ctx *ctx,
                                      grn_geo_point *point1,
                                      grn_geo_point *point2);
double grn_geo_distance_sphere_raw(grn_ctx *ctx,
                                   grn_geo_point *point1,
                                   grn_geo_point *point2);
double grn_geo_distance_ellipsoid_raw(grn_ctx *ctx,
                                      grn_geo_point *point1,
                                      grn_geo_point *point2,
                                      int c1, int c2, double c3);

#ifdef __cplusplus
}
#endif

#endif /* GRN_GEO_H */
