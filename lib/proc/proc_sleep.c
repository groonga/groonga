/*
  Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>

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
command_sleep(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  double second = grn_plugin_proc_get_var_double(ctx, user_data,
                                                 "second", -1,
                                                 1);
  grn_nanosleep((uint64_t)(second * 1000000000));
  grn_ctx_output_bool(ctx, true);
  return NULL;
}

void
grn_proc_init_sleep(grn_ctx *ctx)
{
  grn_expr_var vars[1];
  grn_plugin_expr_var_init(ctx, &(vars[0]), "second", -1);
  grn_plugin_command_create(ctx,
                            "sleep", -1,
                            command_sleep,
                            sizeof(vars) / sizeof(vars[0]),
                            vars);
}
