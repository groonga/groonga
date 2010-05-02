/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

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

#include "proc.h"
#include "ql.h"
#include "db.h"

#define GEO_RESOLUTION   3600000
#define GEO_RADIOUS      6357303
#define GEO_BES_C1       6334834
#define GEO_BES_C2       6377397
#define GEO_BES_C3       0.006674
#define GEO_GRS_C1       6335439
#define GEO_GRS_C2       6378137
#define GEO_GRS_C3       0.006694
#define GEO_INT2RAD(x)   ((M_PI / (GEO_RESOLUTION * 180)) * x)

static grn_obj *
func_geo_in_circle(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  unsigned char r = GRN_FALSE;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 3) {
    grn_obj *pos = args[0], *pos1 = args[1], *pos2 = args[2], pos1_, pos2_;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      double lng0, lat0, lng1, lat1, lng2, lat2, x, y, d;
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
      lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
      lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
      lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
      x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
      y = (lat1 - lat0);
      d = (x * x) + (y * y);
      switch (pos2->header.domain) {
      case GRN_DB_INT32 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_INT32_VALUE(pos2);
        break;
      case GRN_DB_UINT32 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_UINT32_VALUE(pos2);
        break;
      case GRN_DB_INT64 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_INT64_VALUE(pos2);
        break;
      case GRN_DB_UINT64 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_UINT64_VALUE(pos2);
        break;
      case GRN_DB_FLOAT :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_FLOAT_VALUE(pos2);
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
        lng2 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos2))->longitude);
        lat2 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos2))->latitude);
        x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
        y = (lat2 - lat1);
        r = d <= (x * x) + (y * y);
        break;
      default :
        goto exit;
      }
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_in_rectangle(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  unsigned char r = GRN_FALSE;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 3) {
    grn_obj *pos = args[0], *pos1 = args[1], *pos2 = args[2], pos1_, pos2_;
    grn_geo_point *p, *p1, *p2;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      if (pos2->header.domain != domain) {
        GRN_OBJ_INIT(&pos2_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos2, &pos2_, 0)) { goto exit; }
        pos2 = &pos2_;
      }
      p = ((grn_geo_point *)GRN_BULK_HEAD(pos));
      p1 = ((grn_geo_point *)GRN_BULK_HEAD(pos1));
      p2 = ((grn_geo_point *)GRN_BULK_HEAD(pos2));
      r = ((p1->longitude <= p->longitude) && (p->longitude <= p2->longitude) &&
           (p1->latitude <= p->latitude) && (p->latitude <= p2->latitude));
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_distance(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  double d = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 2) {
    grn_obj *pos = args[0], *pos1 = args[1], pos1_;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      double lng0, lat0, lng1, lat1, x, y;
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
      lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
      lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
      lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
      x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
      y = (lat1 - lat0);
      d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_geo_distance2(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  double d = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 2) {
    grn_obj *pos = args[0], *pos1 = args[1], pos1_;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      double lng0, lat0, lng1, lat1, x, y;
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
      lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
      lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
      lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
      x = sin(fabs(lng1 - lng0) * 0.5);
      y = sin(fabs(lat1 - lat0) * 0.5);
      d = asin(sqrt((y * y) + cos(lat0) * cos(lat1) * x * x)) * 2 * GEO_RADIOUS;
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_geo_distance3(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  double d = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);

  if (nargs == 2) {
    grn_obj *pos = args[0], *pos1 = args[1], pos1_;
    grn_id domain = pos->header.domain;
    switch (domain) {
    case GRN_DB_TOKYO_GEO_POINT :
      {
        double lng0, lat0, lng1, lat1, p, q, r, m, n, x, y;
        if (pos1->header.domain != domain) {
          GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
          if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
          pos1 = &pos1_;
        }
        lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
        lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
        lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
        lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
        p = (lat0 + lat1) * 0.5;
        q = (1 - GEO_BES_C3 * sin(p) * sin(p));
        r = sqrt(q);
        m = GEO_BES_C1 / (q * r);
        n = GEO_BES_C2 / r;
        x = n * cos(p) * fabs(lng0 - lng1);
        y = m * fabs(lat0 - lat1);
        d = sqrt((x * x) + (y * y));
      }
      break;
    case  GRN_DB_WGS84_GEO_POINT :
      {
        double lng0, lat0, lng1, lat1, p, q, r, m, n, x, y;
        if (pos1->header.domain != domain) {
          GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
          if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
          pos1 = &pos1_;
        }
        lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
        lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
        lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
        lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
        p = (lat0 + lat1) * 0.5;
        q = (1 - GEO_GRS_C3 * sin(p) * sin(p));
        r = sqrt(q);
        m = GEO_GRS_C1 / (q * r);
        n = GEO_GRS_C2 / r;
        x = n * cos(p) * fabs(lng0 - lng1);
        y = m * fabs(lat0 - lat1);
        d = sqrt((x * x) + (y * y));
      }
      break;
    default :
      /* todo */
      break;
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

grn_rc
grn_module_init_geo(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
grn_module_register_geo(grn_ctx *ctx)
{
  grn_proc_create(ctx, "geo_in_circle", 13, GRN_PROC_FUNCTION,
                  func_geo_in_circle, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_in_rectangle", 16, GRN_PROC_FUNCTION,
                  func_geo_in_rectangle, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance", 12, GRN_PROC_FUNCTION,
                  func_geo_distance, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance2", 13, GRN_PROC_FUNCTION,
                  func_geo_distance2, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance3", 13, GRN_PROC_FUNCTION,
                  func_geo_distance3, NULL, NULL, 0, NULL);

  return ctx->rc;
}

grn_rc
grn_module_fin_geo(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
