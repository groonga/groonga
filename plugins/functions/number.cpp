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
    return number < 0 ? (((number + 1) / interval) - 1) : ((number / interval));
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
  classify(grn_ctx *ctx,
           grn_obj *number,
           grn_obj *interval,
           grn_obj *classed_number)
  {
    auto number_raw = grn::bulk::get<TYPE>(ctx, number, 0);
    auto interval_raw = grn::bulk::get<TYPE>(ctx, interval, 0);
    auto class_raw = classify_raw<TYPE>(ctx, number_raw, interval_raw);
    auto classed_number_raw = class_raw * interval_raw;
    grn::bulk::set<TYPE>(ctx, classed_number, classed_number_raw);
  }

  grn_obj *
  func_number_classify(grn_ctx *ctx,
                       int n_args,
                       grn_obj **args,
                       grn_user_data *user_data)
  {
#define FUNCTION_NAME "number_classify"

    grn_obj *number;
    grn_obj *interval;
    grn_obj casted_interval;
    grn_obj *classed_number;

    if (n_args != 2) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
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
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
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
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s(): the second argument must be a number: %.*s",
                       FUNCTION_NAME,
                       static_cast<int>(GRN_TEXT_LEN(&inspected)),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }

    classed_number =
      grn_plugin_proc_alloc(ctx, user_data, number->header.domain, 0);
    if (!classed_number) {
      return NULL;
    }

    GRN_VALUE_FIX_SIZE_INIT(&casted_interval, 0, number->header.domain);
    grn_obj_cast(ctx, interval, &casted_interval, false);

    switch (number->header.domain) {
    case GRN_DB_INT8:
      classify<int8_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_UINT8:
      classify<uint8_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_INT16:
      classify<int16_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_UINT16:
      classify<uint16_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_INT32:
      classify<int32_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_UINT32:
      classify<uint32_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_INT64:
      classify<int64_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_UINT64:
      classify<uint64_t>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_FLOAT32:
      classify<float>(ctx, number, &casted_interval, classed_number);
      break;
    case GRN_DB_FLOAT:
      classify<double>(ctx, number, &casted_interval, classed_number);
      break;
    default:
      {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, number);
        GRN_PLUGIN_ERROR(
          ctx,
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

  template <typename TYPE>
  std::enable_if_t<std::is_integral_v<TYPE>, TYPE>
  round_raw(TYPE value, int32_t n_digits)
  {
    if (n_digits >= 0) {
      return value;
    }
    auto scale = std::pow(10, -n_digits);
    return std::round(value / scale) * scale;
  }

  template <typename TYPE>
  std::enable_if_t<std::is_floating_point_v<TYPE>, TYPE>
  round_raw(TYPE value, int32_t n_digits)
  {
    if (n_digits == 0) {
      return std::round(value);
    }
    if (n_digits > 0) {
      auto scale = std::pow(10, n_digits);
      return std::round(value * scale) / scale;
    } else {
      auto scale = std::pow(10, -n_digits);
      return std::round(value / scale) * scale;
    }
  }

  template <typename TYPE>
  void
  round(grn_ctx *ctx, grn_obj *value, int32_t n_digits_raw, grn_obj *rounded)
  {
    TYPE value_raw = grn::bulk::get<TYPE>(ctx, value, 0);
    TYPE rounded_raw = round_raw<TYPE>(value_raw, n_digits_raw);
    grn::bulk::set(ctx, rounded, rounded_raw);
  }

  grn_obj *
  func_number_round(grn_ctx *ctx,
                    int n_args,
                    grn_obj **args,
                    grn_user_data *user_data)
  {
#define FUNCTION_NAME "round"
    if (n_args == 0 || n_args > 2) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s(): wrong number of arguments (%d for 1..2)",
                       FUNCTION_NAME,
                       n_args);
      return grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
    }

    grn_obj *target = args[0];
    if (!grn_obj_is_number_family_bulk(ctx, target)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, target);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s(): target value must be a number family value: %.*s",
                       FUNCTION_NAME,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
    }

    int32_t n_digits = 0;
    if (n_args >= 2) {
      grn_obj *n_digits_object = args[1];
      n_digits = grn_plugin_proc_get_value_int32(ctx,
                                                 n_digits_object,
                                                 n_digits,
                                                 FUNCTION_NAME "(): n_digits");
      if (ctx->rc != GRN_SUCCESS) {
        return grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
      }
    }

    auto rounded =
      grn_plugin_proc_alloc(ctx, user_data, target->header.domain, 0);
    switch (target->header.domain) {
    case GRN_DB_UINT8:
      round<uint8_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_INT8:
      round<int8_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_UINT16:
      round<uint16_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_INT16:
      round<int16_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_UINT32:
      round<uint32_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_INT32:
      round<int32_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_UINT64:
      round<uint64_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_INT64:
      round<int64_t>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_FLOAT32:
      round<float>(ctx, target, n_digits, rounded);
      break;
    case GRN_DB_FLOAT:
      round<double>(ctx, target, n_digits, rounded);
      break;
    default:
      {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, target);
        GRN_PLUGIN_ERROR(
          ctx,
          GRN_FUNCTION_NOT_IMPLEMENTED,
          "%s(): unsupported type: %s",
          FUNCTION_NAME,
          grn_type_id_to_string_builtin(ctx, target->header.domain));
        GRN_OBJ_FIN(ctx, &inspected);
      }
      break;
    }
    return rounded;
#undef FUNCTION_NAME
  }
} // namespace

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
                  "number_classify",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_number_classify,
                  NULL,
                  NULL,
                  0,
                  NULL);

  grn_proc_create(ctx,
                  "number_round",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_number_round,
                  NULL,
                  NULL,
                  0,
                  NULL);

  return rc;
}

extern "C" grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
