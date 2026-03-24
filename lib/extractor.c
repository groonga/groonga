/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>

#include "grn.h"
#include "grn_db.h"
#include "grn_extractor.h"

grn_rc
grn_extract_data_init(grn_ctx *ctx, grn_extract_data *data)
{
  data->table = NULL;
  data->index = 0;
  data->value = NULL;
  return GRN_SUCCESS;
}

grn_obj *
grn_extract_data_get_value(grn_ctx *ctx, grn_extract_data *data)
{
  return data->value;
}

grn_obj *
grn_extract_data_get_table(grn_ctx *ctx, grn_extract_data *data)
{
  return data->table;
}

uint32_t
grn_extract_data_get_index(grn_ctx *ctx, grn_extract_data *data)
{
  return data->index;
}

grn_obj *
grn_extractor_create(grn_ctx *ctx, const char *name, int length)
{
  GRN_API_ENTER;
  grn_obj *extractor = grn_proc_create(ctx,
                                       name,
                                       length,
                                       GRN_PROC_EXTRACTOR,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       NULL);
  if (!extractor) {
    if (length < 0) {
      length = (int)strlen(name);
    }
    GRN_PLUGIN_ERROR(ctx,
                     GRN_EXTRACTOR_ERROR,
                     "[extractor][create] failed to create: <%.*s>",
                     length,
                     name);
  }

  GRN_API_RETURN(extractor);
}

grn_rc
grn_extractor_set_extract_func(grn_ctx *ctx,
                               grn_obj *extractor,
                               grn_extractor_extract_func *extract)
{
  GRN_API_ENTER;
  if (extractor) {
    ((grn_proc *)extractor)->callbacks.extractor.extract = extract;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[extractor][extract][set] extractor is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_extractor_extract(grn_ctx *ctx, grn_obj *extractor, grn_extract_data *data)
{
  GRN_API_ENTER;
  grn_obj *value =
    ((grn_proc *)extractor)->callbacks.extractor.extract(ctx, extractor, data);
  GRN_API_RETURN(value);
}
