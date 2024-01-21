/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_ctx.h"

#include <groonga/plugin.h>

static grn_obj *
func_distance_cosine(grn_ctx *ctx,
                     int n_args,
                     grn_obj **args,
                     grn_user_data *user_data)
{
  const char *function_name = "distance_cosine";

  grn_obj *distance = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_FLOAT32, 0);
  GRN_FLOAT32_SET(ctx, distance, 0.0);

  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s(): wrong number of arguments (%d for 2)",
                     function_name,
                     n_args);
    return distance;
  }

  grn_obj *vector1 = args[0];
  grn_obj *vector2 = args[1];
  float distance_raw = grn_distance_cosine(ctx, vector1, vector2);
  GRN_FLOAT32_SET(ctx, distance, distance_raw);

  return distance;
}

void
grn_proc_init_distance_cosine(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "distance_cosine",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_distance_cosine,
                  NULL,
                  NULL,
                  0,
                  NULL);
}

static grn_obj *
func_distance_inner_product(grn_ctx *ctx,
                            int n_args,
                            grn_obj **args,
                            grn_user_data *user_data)
{
  const char *function_name = "distance_inner_product";

  grn_obj *distance = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_FLOAT32, 0);
  GRN_FLOAT32_SET(ctx, distance, 0.0);

  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s(): wrong number of arguments (%d for 2)",
                     function_name,
                     n_args);
    return distance;
  }

  grn_obj *vector1 = args[0];
  grn_obj *vector2 = args[1];
  float distance_raw = grn_distance_inner_product(ctx, vector1, vector2);
  GRN_FLOAT32_SET(ctx, distance, distance_raw);

  return distance;
}

void
grn_proc_init_distance_inner_product(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "distance_inner_product",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_distance_inner_product,
                  NULL,
                  NULL,
                  0,
                  NULL);
}

static grn_obj *
func_distance_l1_norm(grn_ctx *ctx,
                      int n_args,
                      grn_obj **args,
                      grn_user_data *user_data)
{
  const char *function_name = "distance_l1_norm";

  grn_obj *distance = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_FLOAT32, 0);
  GRN_FLOAT32_SET(ctx, distance, 0.0);

  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s(): wrong number of arguments (%d for 2)",
                     function_name,
                     n_args);
    return distance;
  }

  grn_obj *vector1 = args[0];
  grn_obj *vector2 = args[1];
  float distance_raw = grn_distance_l1_norm(ctx, vector1, vector2);
  GRN_FLOAT32_SET(ctx, distance, distance_raw);

  return distance;
}

void
grn_proc_init_distance_l1_norm(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "distance_l1_norm",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_distance_l1_norm,
                  NULL,
                  NULL,
                  0,
                  NULL);
}

static grn_obj *
func_distance_l2_norm_squared(grn_ctx *ctx,
                              int n_args,
                              grn_obj **args,
                              grn_user_data *user_data)
{
  const char *function_name = "distance_l2_norm_squared";

  grn_obj *distance = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_FLOAT32, 0);
  GRN_FLOAT32_SET(ctx, distance, 0.0);

  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s(): wrong number of arguments (%d for 2)",
                     function_name,
                     n_args);
    return distance;
  }

  grn_obj *vector1 = args[0];
  grn_obj *vector2 = args[1];
  float distance_raw = grn_distance_l2_norm_squared(ctx, vector1, vector2);
  GRN_FLOAT32_SET(ctx, distance, distance_raw);

  return distance;
}

void
grn_proc_init_distance_l2_norm_squared(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "distance_l2_norm_squared",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_distance_l2_norm_squared,
                  NULL,
                  NULL,
                  0,
                  NULL);
}
