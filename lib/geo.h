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
#ifndef GRN_GEO_H
#define GRN_GEO_H

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

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
#define GRN_GEO_RAD2INT(x)   (((GRN_GEO_RESOLUTION * 180) / M_PI) * (x))

#define GRN_GEO_POINT_VALUE_RAW(obj) (grn_geo_point *)GRN_BULK_HEAD(obj)
#define GRN_GEO_POINT_VALUE_RADIUS(obj,_latitude,_longitude) do {\
  grn_geo_point *_val = (grn_geo_point *)GRN_BULK_HEAD(obj);\
  _latitude = GRN_GEO_INT2RAD(_val->latitude);\
  _longitude = GRN_GEO_INT2RAD(_val->longitude);\
} while (0)

grn_rc grn_geo_search_in_circle(grn_ctx *ctx, grn_obj *obj, grn_obj **args,
                                int nargs, grn_obj *res, grn_operator op);
grn_rc grn_geo_search_in_rectangle(grn_ctx *ctx, grn_obj *obj, grn_obj **args,
                                   int nargs, grn_obj *res, grn_operator op);
int grn_geo_table_sort(grn_ctx *ctx, grn_obj *table, int offset, int limit,
                       grn_obj *result, grn_table_sort_key *keys, int n_keys);

unsigned grn_geo_in_circle(grn_ctx *ctx, grn_obj *point, grn_obj *center,
                           grn_obj *radius_or_point);
unsigned grn_geo_in_rectangle(grn_ctx *ctx, grn_obj *point,
                              grn_obj *top_left, grn_obj *bottom_right);
unsigned grn_geo_in_rectangle_raw(grn_ctx *ctx, grn_geo_point *point,
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
