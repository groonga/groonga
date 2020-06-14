/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_ctx.h"

#include <groonga/plugin.h>

static grn_obj *
command_reference_acquire(grn_ctx *ctx,
                          int nargs,
                          grn_obj **args,
                          grn_user_data *user_data)
{
  grn_raw_string target_name;
  target_name.value =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "target_name", -1,
                                   &(target_name.length));
  grn_raw_string recursive;
  recursive.value =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "recursive", -1,
                                   &(recursive.length));

  grn_obj *obj;
  bool need_unref = false;
  if (target_name.length > 0) {
    obj = grn_ctx_get(ctx, target_name.value, target_name.length);
    need_unref = true;
  } else {
    obj = grn_ctx_db(ctx);
  }

  if (obj) {
    if (GRN_RAW_STRING_EQUAL_CSTRING(recursive, "dependent")) {
      grn_obj_refer_recursive_dependent(ctx, obj);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(recursive, "no")) {
      grn_obj_refer(ctx, obj);
    } else {
      grn_obj_refer_recursive(ctx, obj);
    }
    if (need_unref) {
      grn_obj_unref(ctx, obj);
    }
  } else {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[reference][acquire] target object not found: <%.*s>",
                     (int)(target_name.length),
                     target_name.value);
  }

  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  return NULL;
}

void
grn_proc_init_reference_acquire(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "target_name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "recursive", -1);
  grn_plugin_command_create(ctx,
                            "reference_acquire", -1,
                            command_reference_acquire,
                            2,
                            vars);
}

static grn_obj *
command_reference_release(grn_ctx *ctx,
                          int nargs,
                          grn_obj **args,
                          grn_user_data *user_data)
{
  grn_raw_string target_name;
  target_name.value =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "target_name", -1,
                                   &(target_name.length));
  grn_raw_string recursive;
  recursive.value =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "recursive", -1,
                                   &(recursive.length));

  grn_obj *obj;
  bool need_unref = false;
  if (target_name.length > 0) {
    obj = grn_ctx_get(ctx, target_name.value, target_name.length);
    need_unref = true;
  } else {
    obj = grn_ctx_db(ctx);
  }

  if (obj) {
    if (GRN_RAW_STRING_EQUAL_CSTRING(recursive, "dependent")) {
      grn_obj_unref_recursive_dependent(ctx, obj);
    } else if (GRN_RAW_STRING_EQUAL_CSTRING(recursive, "no")) {
      grn_obj_unref(ctx, obj);
    } else {
      grn_obj_unref_recursive(ctx, obj);
    }
    if (need_unref) {
      grn_obj_unref(ctx, obj);
    }
  } else {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[reference][release] target object not found: <%.*s>",
                     (int)(target_name.length),
                     target_name.value);
  }

  grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);

  return NULL;
}

void
grn_proc_init_reference_release(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "target_name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "recursive", -1);
  grn_plugin_command_create(ctx,
                            "reference_release", -1,
                            command_reference_release,
                            2,
                            vars);
}
