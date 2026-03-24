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

#include "../grn_proc.h"
#include "../grn_ctx.h"
#include "../grn_extractor.h"

#include <groonga/plugin.h>

static grn_obj *
command_extract(grn_ctx *ctx,
                int nargs,
                grn_obj **args,
                grn_user_data *user_data)
{
  const char *context_tag = "[extract]";

  grn_raw_string extractors_raw;
  extractors_raw.value =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "extractors",
                                   -1,
                                   &(extractors_raw.length));
  grn_obj *value = grn_plugin_proc_get_var(ctx, user_data, "value", -1);

  if (extractors_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s extractor names are missing",
                     context_tag);
    return NULL;
  }

  {
    grn_obj *lexicon = grn_proc_lexicon_open(ctx,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &extractors_raw,
                                             context_tag);
    if (!lexicon) {
      return NULL;
    }

    int n_elements = 1;
    grn_ctx_output_map_open(ctx, "RESULT", n_elements);
    {
      grn_obj extractors;
      GRN_PTR_INIT(&extractors, GRN_OBJ_VECTOR, 0);
      grn_obj_get_info(ctx, lexicon, GRN_INFO_EXTRACTORS, &extractors);

      grn_extract_data data;
      grn_extract_data_init(ctx, &data);
      data.table = lexicon;
      data.value = value;

      size_t i;
      size_t n = GRN_PTR_VECTOR_SIZE(&extractors);
      for (i = 0; i < n; i++) {
        data.index = i;

        grn_obj *extractor = GRN_PTR_VALUE_AT(&extractors, i);
        grn_obj *extracted = grn_extractor_extract(ctx, extractor, &data);
        if (extracted) {
          if (data.value != value) {
            grn_obj_close(ctx, data.value);
          }
          data.value = extracted;
        }
      }
      GRN_OBJ_FIN(ctx, &extractors);

      grn_ctx_output_cstr(ctx, "extracted");
      grn_ctx_output_obj(ctx, data.value, NULL);
      if (data.value != value) {
        GRN_OBJ_FIN(ctx, data.value);
      }
    }
    grn_ctx_output_map_close(ctx);

    grn_obj_unlink(ctx, lexicon);
  }

  return NULL;
}

void
grn_proc_init_extract(grn_ctx *ctx)
{
  grn_expr_var vars[2];
  grn_plugin_expr_var_init(ctx, &(vars[0]), "extractors", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "value", -1);
  grn_plugin_command_create(ctx, "extract", -1, command_extract, 2, vars);
}
