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

#ifdef __cplusplus
extern "C" {
#endif

unsigned grn_geo_in_circle(grn_ctx *ctx, grn_obj *point, grn_obj *center,
                           grn_obj *radius_or_point);
unsigned grn_geo_in_rectangle(grn_ctx *ctx, grn_obj *point,
                              grn_obj *top_left, grn_obj *bottom_right);
double grn_geo_distance(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance2(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);
double grn_geo_distance3(grn_ctx *ctx, grn_obj *point1, grn_obj *point2);

#ifdef __cplusplus
}
#endif

#endif /* GRN_GEO_H */
