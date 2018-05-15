/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn.h"
#include "grn_db.h"

grn_rc
grn_accessor_name(grn_ctx *ctx, grn_obj *accessor, grn_obj *name)
{
  grn_accessor *accessor_;
  GRN_API_ENTER;

  if (!grn_obj_is_accessor(ctx, accessor)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    ERR(GRN_INVALID_ARGUMENT,
        "[accessor][name] not accessor: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  for (accessor_ = (grn_accessor *)accessor;
       accessor_;
       accessor_ = accessor_->next) {
    grn_bool show_obj_name = GRN_FALSE;
    grn_bool show_obj_domain_name = GRN_FALSE;

    if (accessor_ != (grn_accessor *)accessor) {
      GRN_TEXT_PUTS(ctx, name, ".");
    }
    switch (accessor_->action) {
    case GRN_ACCESSOR_GET_ID :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_ID,
                   GRN_COLUMN_NAME_ID_LEN);
      show_obj_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_KEY :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_KEY,
                   GRN_COLUMN_NAME_KEY_LEN);
      show_obj_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_VALUE :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_VALUE,
                   GRN_COLUMN_NAME_VALUE_LEN);
      show_obj_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_SCORE :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_SCORE,
                   GRN_COLUMN_NAME_SCORE_LEN);
      break;
    case GRN_ACCESSOR_GET_NSUBRECS :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_NSUBRECS,
                   GRN_COLUMN_NAME_NSUBRECS_LEN);
      break;
    case GRN_ACCESSOR_GET_MAX :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_MAX,
                   GRN_COLUMN_NAME_MAX_LEN);
      break;
    case GRN_ACCESSOR_GET_MIN :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_MIN,
                   GRN_COLUMN_NAME_MIN_LEN);
      break;
    case GRN_ACCESSOR_GET_SUM :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_SUM,
                   GRN_COLUMN_NAME_SUM_LEN);
      break;
    case GRN_ACCESSOR_GET_AVG :
      GRN_TEXT_PUT(ctx,
                   name,
                   GRN_COLUMN_NAME_AVG,
                   GRN_COLUMN_NAME_AVG_LEN);
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      grn_column_name_(ctx, accessor_->obj, name);
      show_obj_domain_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_DB_OBJ :
      grn_text_printf(ctx, name, "(_db)");
      break;
    case GRN_ACCESSOR_LOOKUP :
      grn_text_printf(ctx, name, "(_lookup)");
      break;
    case GRN_ACCESSOR_FUNCALL :
      grn_text_printf(ctx, name, "(_funcall)");
      break;
    default :
      grn_text_printf(ctx, name, "(unknown:%u)", accessor_->action);
      break;
    }

    if (show_obj_name || show_obj_domain_name) {
      grn_obj *target = accessor_->obj;
      char target_name[GRN_TABLE_MAX_KEY_SIZE];
      int target_name_size;

      if (show_obj_domain_name) {
        target = grn_ctx_at(ctx, target->header.domain);
      }

      target_name_size = grn_obj_name(ctx,
                                      target,
                                      target_name,
                                      GRN_TABLE_MAX_KEY_SIZE);
      GRN_TEXT_PUTS(ctx, name, "(");
      if (target_name_size == 0) {
        GRN_TEXT_PUTS(ctx, name, "anonymous");
      } else {
        GRN_TEXT_PUT(ctx, name, target_name, target_name_size);
      }
      GRN_TEXT_PUTS(ctx, name, ")");
    }
  }

  GRN_API_RETURN(GRN_SUCCESS);
}
