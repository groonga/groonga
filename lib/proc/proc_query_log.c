/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017 Brazil

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

#include "../grn_proc.h"

#include <groonga/plugin.h>

static grn_obj *
command_query_log_flags(grn_ctx *ctx,
                        int nargs,
                        grn_obj **args,
                        grn_user_data *user_data)
{
  unsigned int previous_flags;
  grn_obj *flags_text;

  previous_flags = grn_query_logger_get_flags(ctx);
  flags_text = grn_plugin_proc_get_var(ctx, user_data, "flags", -1);
  if (GRN_TEXT_LEN(flags_text) > 0) {
    unsigned int flags = 0;
    grn_obj *mode_value;

    if (!grn_query_log_flags_parse(GRN_TEXT_VALUE(flags_text),
                                   GRN_TEXT_LEN(flags_text),
                                   &flags)) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[query-log][flags] invalid query log flags: <%.*s>",
                       (int)GRN_TEXT_LEN(flags_text),
                       GRN_TEXT_VALUE(flags_text));
      grn_ctx_output_null(ctx);
      return NULL;
    }

    mode_value = grn_plugin_proc_get_var(ctx, user_data, "mode", -1);
    if (GRN_TEXT_LEN(mode_value) == 0 ||
        GRN_TEXT_EQUAL_CSTRING(mode_value, "SET")) {
      grn_query_logger_set_flags(ctx, flags);
    } else if (GRN_TEXT_EQUAL_CSTRING(mode_value, "ADD")) {
      grn_query_logger_add_flags(ctx, flags);
    } else if (GRN_TEXT_EQUAL_CSTRING(mode_value, "REMOVE")) {
      grn_query_logger_remove_flags(ctx, flags);
    } else {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[query-log][flags] "
                       "mode must be <SET>, <ADD> or <REMOVE>: <%.*s>",
                       (int)GRN_TEXT_LEN(mode_value),
                       GRN_TEXT_VALUE(mode_value));
      grn_ctx_output_null(ctx);
      return NULL;
    }
  }

  {
    unsigned int current_flags;
    grn_obj inspected_flags;

    current_flags = grn_query_logger_get_flags(ctx);
    GRN_TEXT_INIT(&inspected_flags, 0);

    grn_ctx_output_map_open(ctx, "query_log_flags", 2);

    grn_inspect_query_log_flags(ctx, &inspected_flags, previous_flags);
    grn_ctx_output_cstr(ctx, "previous");
    grn_ctx_output_str(ctx,
                       GRN_TEXT_VALUE(&inspected_flags),
                       GRN_TEXT_LEN(&inspected_flags));

    GRN_BULK_REWIND(&inspected_flags);
    grn_inspect_query_log_flags(ctx, &inspected_flags, current_flags);
    grn_ctx_output_cstr(ctx, "current");
    grn_ctx_output_str(ctx,
                       GRN_TEXT_VALUE(&inspected_flags),
                       GRN_TEXT_LEN(&inspected_flags));

    grn_ctx_output_map_close(ctx);

    GRN_OBJ_FIN(ctx, &inspected_flags);
  }

  return NULL;
}

void
grn_proc_init_query_log_flags(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "mode", -1);
  grn_plugin_command_create(ctx,
                            "query_log_flags", -1,
                            command_query_log_flags,
                            2,
                            vars);
}
