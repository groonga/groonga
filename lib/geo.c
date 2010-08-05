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
      r = (sqrt(d) * GRN_GEO_RADIOUS) <= GRN_INT32_VALUE(radius_or_point);
      break;
    case GRN_DB_UINT32 :
      r = (sqrt(d) * GRN_GEO_RADIOUS) <= GRN_UINT32_VALUE(radius_or_point);
      break;
    case GRN_DB_INT64 :
      r = (sqrt(d) * GRN_GEO_RADIOUS) <= GRN_INT64_VALUE(radius_or_point);
      break;
    case GRN_DB_UINT64 :
      r = (sqrt(d) * GRN_GEO_RADIOUS) <= GRN_UINT64_VALUE(radius_or_point);
      break;
    case GRN_DB_FLOAT :
      r = (sqrt(d) * GRN_GEO_RADIOUS) <= GRN_FLOAT_VALUE(radius_or_point);
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
grn_geo_in_rectangle(grn_ctx *ctx, grn_obj *point,
                     grn_obj *top_left, grn_obj *bottom_right)
{
  unsigned r = GRN_FALSE;
  grn_obj top_left_, bottom_right_;
  grn_id domain = point->header.domain;
  if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
    int lat0, lng0, lat1, lng1, lat2, lng2;
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
    GRN_GEO_POINT_VALUE(point, lat0, lng0);
    GRN_GEO_POINT_VALUE(top_left, lat1, lng1);
    GRN_GEO_POINT_VALUE(bottom_right, lat2, lng2);
    r = ((lng1 <= lng0) && (lng0 <= lng2) &&
         (lat1 <= lat0) && (lat0 <= lat2));
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
  return sqrt((x * x) + (y * y)) * GRN_GEO_RADIOUS;
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
  return asin(sqrt((y * y) + cos(lat1) * cos(lat2) * x * x)) * 2 * GRN_GEO_RADIOUS;
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
