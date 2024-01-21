/*
  Copyright (C) 2016  Brazil
  Copyright (C) 2022-2024  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_number
#endif

#include <groonga/plugin.h>

#include <groonga/bulk.hpp>

#include <cmath>

namespace {
  template <typename TYPE>
  std::enable_if_t<std::is_integral_v<TYPE> && std::is_signed_v<TYPE>, TYPE>
  classify_raw(grn_ctx *ctx, TYPE number, TYPE interval)
  {
    return number < 0 ?
      (((number + 1) / interval) - 1) :
      ((number / interval));
  }

  template <typename TYPE>
  std::enable_if_t<std::is_integral_v<TYPE> && !std::is_signed_v<TYPE>, TYPE>
  classify_raw(grn_ctx *ctx, TYPE number, TYPE interval)
  {
    return number / interval;
  }

  template <typename TYPE>
  std::enable_if_t<std::is_floating_point_v<TYPE>, TYPE>
  classify_raw(grn_ctx *ctx, TYPE number, TYPE interval)
  {
    return std::floor(number / interval);
  }

  template <typename TYPE>
  void
  classify(grn_ctx *ctx, grn_obj *number, grn_obj *interval, grn_obj *classed_number)
  {
    auto number_raw = grn::bulk::get<TYPE>(ctx, number, 0);
    auto interval_raw = grn::bulk::get<TYPE>(ctx, interval, 0);
    auto class_raw = classify_raw<TYPE>(ctx, number_raw, interval_raw);
    auto classed_number_raw = class_raw * interval_raw;
    grn::bulk::set<TYPE>(ctx, classed_number, classed_number_raw);
  }

  grn_obj *
  func_number_classify(grn_ctx *ctx, int n_args, grn_obj **args,
                       grn_user_data *user_data)
  {
#define FUNCTION_NAME "number_classify"

  grn_obj *number;
  grn_obj *interval;
  grn_obj casted_interval;
  grn_obj *classed_number;

  if (n_args != 2) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): wrong number of arguments (%d for 2)",
                     FUNCTION_NAME,
                     n_args);
    return NULL;
  }

  number = args[0];
  if (!grn_obj_is_number_family_bulk(ctx, number)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, number);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): the first argument must be a number: %.*s",
                     FUNCTION_NAME,
                     static_cast<int>(GRN_TEXT_LEN(&inspected)),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  interval = args[1];
  if (!grn_obj_is_number_family_bulk(ctx, interval)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, interval);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s(): the second argument must be a number: %.*s",
                     FUNCTION_NAME,
                     static_cast<int>(GRN_TEXT_LEN(&inspected)),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  classed_number = grn_plugin_proc_alloc(ctx,
                                         user_data,
                                         number->header.domain,
                                         0);
  if (!classed_number) {
    return NULL;
  }

  GRN_VALUE_FIX_SIZE_INIT(&casted_interval, 0, number->header.domain);
  grn_obj_cast(ctx, interval, &casted_interval, false);

  switch (number->header.domain) {
  case GRN_DB_INT8 :
    classify<int8_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_UINT8 :
    classify<uint8_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_INT16 :
    classify<int16_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_UINT16 :
    classify<uint16_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_INT32 :
    classify<int32_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_UINT32 :
    classify<uint32_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_INT64 :
    classify<int64_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_UINT64 :
    classify<uint64_t>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_FLOAT32 :
    classify<float>(ctx, number, &casted_interval, classed_number);
    break;
  case GRN_DB_FLOAT :
    classify<double>(ctx, number, &casted_interval, classed_number);
    break;
  default :
    {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, number);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_FUNCTION_NOT_IMPLEMENTED,
                       "%s(): unsupported type: %s",
                       FUNCTION_NAME,
                       grn_type_id_to_string_builtin(ctx, number->header.domain));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    break;
  }

  GRN_OBJ_FIN(ctx, &casted_interval);

  return classed_number;
#undef FUNCTION_NAME
}
}

extern "C" grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

extern "C" grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  grn_proc_create(ctx,
                  "number_classify", -1,
                  GRN_PROC_FUNCTION,
                  func_number_classify,
                  NULL, NULL, 0, NULL);

  return rc;
}

extern "C" grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
