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

#include "grn_distance.hpp"

#include <groonga/bulk.hpp>

#include "grn_ctx.hpp"
#include "grn_db.h"

namespace grn {
  namespace distance {
#ifdef GRN_WITH_SIMSIMD
    bool use_simsimd = true;
    namespace simsimd {
      simsimd_capability_t capabilities = simsimd_cap_serial_k;
    }
#else
    bool use_simsimd = false;
#endif

#ifdef GRN_WITH_XSIMD
    bool use_xsimd = true;
#else
    bool use_xsimd = false;
#endif
  } // namespace distance
} // namespace grn

namespace {
  bool
  validate_vectors(grn_ctx *ctx,
                   grn_obj *vector1,
                   grn_obj *vector2,
                   const char *tag)
  {
    if (!grn_obj_is_number_family_vector(ctx, vector1)) {
      grn::TextBulk inspected(ctx);
      grn_inspect(ctx, *inspected, vector1);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the 1st argument must be number family vector: %.*s",
          tag,
          static_cast<int>(GRN_TEXT_LEN(*inspected)),
          GRN_TEXT_VALUE(*inspected));
      return false;
    }

    if (!grn_obj_is_number_family_vector(ctx, vector2)) {
      grn::TextBulk inspected(ctx);
      grn_inspect(ctx, *inspected, vector2);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the 2nd argument must be number family vector: %.*s",
          tag,
          static_cast<int>(GRN_TEXT_LEN(*inspected)),
          GRN_TEXT_VALUE(*inspected));
      return false;
    }

    if (vector1->header.domain != vector2->header.domain) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s both vectors must be the same type: %s: %s",
          tag,
          grn_type_id_to_string_builtin(ctx, vector1->header.domain),
          grn_type_id_to_string_builtin(ctx, vector2->header.domain));
      return false;
    }

    auto n_elements1 = grn_vector_size(ctx, vector1);
    auto n_elements2 = grn_vector_size(ctx, vector2);
    if (n_elements1 != n_elements2) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s both vectors must be the same size: %d: %d",
          tag,
          n_elements1,
          n_elements2);
      return false;
    }

    return true;
  }

  template <typename ElementType>
  float
  compute_distance_cosine(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
  {
    return grn::distance::compute_distance_cosine<ElementType>(vector1,
                                                               vector2);
  }

  template <typename ElementType>
  float
  compute_distance_inner_product(grn_ctx *ctx,
                                 grn_obj *vector1,
                                 grn_obj *vector2)
  {
    return grn::distance::compute_distance_inner_product<ElementType>(vector1,
                                                                      vector2);
  }

  template <typename ElementType>
  float
  compute_distance_l1_norm(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
  {
    return grn::distance::compute_difference_l1_norm<ElementType>(vector1,
                                                                  vector2);
  }

  template <typename ElementType>
  float
  compute_distance_l2_norm_squared(grn_ctx *ctx,
                                   grn_obj *vector1,
                                   grn_obj *vector2)
  {
    return grn::distance::compute_difference_l2_norm_squared<ElementType>(
      vector1,
      vector2);
  }

  enum class Method {
    Cosine,
    InnerProduct,
    L1Norm,
    L2NormSquared,
  };

  template <typename ElementType, typename DistanceFunc>
  void
  distance_apply(grn_ctx *ctx,
                 grn_applier_data *data,
                 grn_obj *table,
                 grn_obj *output_column,
                 grn_obj *input_column,
                 grn_id input_column_range,
                 grn_obj *literal,
                 const char *function_name,
                 DistanceFunc distance_func)
  {
    auto n_records = grn_table_size(ctx, table);
    auto n_elements = GRN_BULK_VSIZE(literal) / sizeof(ElementType);
    auto task_executor = grn_ctx_get_task_executor(ctx);
    auto n_workers = task_executor->get_n_workers();
    if (n_workers == 0) {
      n_workers = 1;
    }
    int limit = (n_records / n_workers) + 1;
    for (size_t i = 0; i < n_workers; i++) {
      auto execute = [&, i]() {
        grn_obj input;
        GRN_VALUE_FIX_SIZE_INIT(&input,
                                GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY,
                                input_column_range);
        grn_obj output;
        GRN_FLOAT_INIT(&output, 0);
        int offset = limit * i;
        grn_ctx *task_ctx = ctx;
        grn_ctx *child_ctx = nullptr;
        if (task_executor->is_parallel()) {
          task_ctx = child_ctx = grn_ctx_pull_child(ctx);
        }
        grn::ChildCtxReleaser releaser(ctx, child_ctx);
        auto cursor = grn_table_cursor_open(task_ctx,
                                            table,
                                            nullptr,
                                            0,
                                            nullptr,
                                            0,
                                            offset,
                                            limit,
                                            GRN_CURSOR_BY_ID);
        if (cursor) {
          grn_id id;
          while ((id = grn_table_cursor_next(task_ctx, cursor)) != GRN_ID_NIL) {
            uint32_t size;
            auto value = grn_obj_get_value_(task_ctx, input_column, id, &size);
            GRN_TEXT_SET(task_ctx, &input, value, size);
            if ((GRN_BULK_VSIZE(&input) / sizeof(ElementType)) != n_elements) {
              continue;
            }
            auto distance_raw = distance_func(task_ctx, &input, literal);
            if (ctx->rc != GRN_SUCCESS) {
              continue;
            }
            GRN_FLOAT_SET(task_ctx, &output, distance_raw);
            grn_obj_set_value(task_ctx,
                              output_column,
                              id,
                              &output,
                              GRN_OBJ_SET);
            if (ctx->rc != GRN_SUCCESS) {
              continue;
            }
          }
          grn_table_cursor_close(task_ctx, cursor);
        }
        GRN_OBJ_FIN(task_ctx, &input);
        GRN_OBJ_FIN(task_ctx, &output);
        return true;
      };
      task_executor->execute(i, execute, function_name);
    }
    task_executor->wait_all();
  }

  grn_rc
  distance_applier(grn_ctx *ctx,
                   grn_applier_data *data,
                   Method method,
                   const char *function_name)
  {
    auto table = grn_applier_data_get_table(ctx, data);
    auto output_column = grn_applier_data_get_output_column(ctx, data);
    size_t n_args;
    auto args = grn_applier_data_get_args(ctx, data, &n_args);

    if (n_args != 2) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s: wrong number of arguments (%" GRN_FMT_SIZE " for 2)",
          function_name,
          n_args);
      return ctx->rc;
    }

    auto input_column = args[0];
    if (!(grn_obj_is_vector_column(ctx, input_column) ||
          grn_obj_is_vector_accessor(ctx, input_column))) {
      grn::TextBulk inspected(ctx);
      grn_inspect(ctx, *inspected, input_column);
      ERR(GRN_INVALID_ARGUMENT,
          "%s: 1st argument must be a vector column or accessor: %.*s",
          function_name,
          static_cast<int>(GRN_TEXT_LEN(*inspected)),
          GRN_TEXT_VALUE(*inspected));
      return ctx->rc;
    }
    auto input_column_range = grn_obj_get_range(ctx, input_column);
    switch (input_column_range) {
    case GRN_DB_FLOAT32:
    case GRN_DB_FLOAT:
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT,
          "%s: 1st argument must be Float or Float32: %s",
          function_name,
          grn_type_id_to_string_builtin(ctx, input_column_range));
      return ctx->rc;
    }

    auto literal = args[1];
    grn_obj casted_literal_buffer;
    grn_obj *casted_literal = NULL;
    if (grn_obj_is_uvector(ctx, literal) &&
        literal->header.domain == input_column_range) {
      /* We can use this as-is. */
    } else {
      GRN_VALUE_FIX_SIZE_INIT(&casted_literal_buffer,
                              GRN_OBJ_VECTOR,
                              input_column_range);
      if (grn_obj_cast(ctx, literal, &casted_literal_buffer, false) !=
          GRN_SUCCESS) {
        GRN_OBJ_FIN(ctx, &casted_literal_buffer);
        grn::TextBulk inspected(ctx);
        grn_inspect(ctx, *inspected, literal);
        ERR(GRN_INVALID_ARGUMENT,
            "%s: 2nd argument must be a Float or Float32 vector literal: %.*s",
            function_name,
            static_cast<int>(GRN_TEXT_LEN(*inspected)),
            GRN_TEXT_VALUE(*inspected));
        return ctx->rc;
      }
      casted_literal = &casted_literal_buffer;
      literal = casted_literal;
    }

    switch (input_column_range) {
    case GRN_DB_FLOAT32:
      {
        auto apply = [&](auto distance_func) {
          return distance_apply<float>(ctx,
                                       data,
                                       table,
                                       output_column,
                                       input_column,
                                       input_column_range,
                                       literal,
                                       function_name,
                                       distance_func);
        };
        switch (method) {
        case Method::Cosine:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_cosine<float>(ctx, vector1, vector2);
          });
          break;
        case Method::InnerProduct:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_inner_product<float>(ctx, vector1, vector2);
          });
          break;
        case Method::L1Norm:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_l1_norm<float>(ctx, vector1, vector2);
          });
          break;
        case Method::L2NormSquared:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_l2_norm_squared<float>(ctx,
                                                           vector1,
                                                           vector2);
          });
          break;
        }
        break;
      }
    case GRN_DB_FLOAT:
      {
        auto apply = [&](auto distance_func) {
          distance_apply<double>(ctx,
                                 data,
                                 table,
                                 output_column,
                                 input_column,
                                 input_column_range,
                                 literal,
                                 function_name,
                                 distance_func);
        };
        switch (method) {
        case Method::Cosine:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_cosine<double>(ctx, vector1, vector2);
          });
          break;
        case Method::InnerProduct:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_inner_product<double>(ctx,
                                                          vector1,
                                                          vector2);
          });
          break;
        case Method::L1Norm:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_l1_norm<double>(ctx, vector1, vector2);
          });
          break;
        case Method::L2NormSquared:
          apply([](grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2) {
            return compute_distance_l2_norm_squared<double>(ctx,
                                                            vector1,
                                                            vector2);
          });
          break;
        }
        break;
      }
    default:
      break;
    }

    if (literal == casted_literal) {
      GRN_OBJ_FIN(ctx, casted_literal);
    }

    return ctx->rc;
  }
} // namespace

extern "C" void
grn_distance_init_external_libraries(void)
{
#ifdef GRN_WITH_SIMSIMD
  grn::distance::simsimd::capabilities = simsimd_capabilities();
#endif
}

extern "C" void
grn_distance_init_from_env(void)
{
  {
    char grn_distance_simd_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_DISTANCE_SIMD", grn_distance_simd_env, GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_distance_simd_env, "simsimd") == 0) {
      grn::distance::use_simsimd = true;
      grn::distance::use_xsimd = false;
    } else if (strcmp(grn_distance_simd_env, "xsimd") == 0) {
      grn::distance::use_simsimd = false;
      grn::distance::use_xsimd = true;
    } else if (strcmp(grn_distance_simd_env, "no") == 0) {
      grn::distance::use_simsimd = false;
      grn::distance::use_xsimd = false;
    }
  }
}

extern "C" float
grn_distance_cosine(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
{
  const char *tag = "[distance][cosine]";
  float distance = 0.0;

  if (!validate_vectors(ctx, vector1, vector2, tag)) {
    return distance;
  }

  switch (vector1->header.domain) {
  case GRN_DB_FLOAT32:
    distance = compute_distance_cosine<float>(ctx, vector1, vector2);
    break;
  case GRN_DB_FLOAT:
    distance = compute_distance_cosine<double>(ctx, vector1, vector2);
    break;
  default:
    // TODO: We should add support for all integer types
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "%s unsupported type: %s",
        tag,
        grn_type_id_to_string_builtin(ctx, vector1->header.domain));
    break;
  }

  return distance;
}

extern "C" grn_rc
grn_distance_cosine_applier(grn_ctx *ctx, grn_applier_data *data)
{
  return distance_applier(ctx, data, Method::Cosine, "distance_cosine()");
}

extern "C" float
grn_distance_inner_product(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
{
  const char *tag = "[distance][inner-product]";
  float distance = 0.0;

  if (!validate_vectors(ctx, vector1, vector2, tag)) {
    return distance;
  }

  switch (vector1->header.domain) {
  case GRN_DB_FLOAT32:
    distance = compute_distance_inner_product<float>(ctx, vector1, vector2);
    break;
  case GRN_DB_FLOAT:
    distance = compute_distance_inner_product<double>(ctx, vector1, vector2);
    break;
  default:
    // TODO: We should add support for all integer types
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "%s unsupported type: %s",
        tag,
        grn_type_id_to_string_builtin(ctx, vector1->header.domain));
    break;
  }

  return distance;
}

extern "C" grn_rc
grn_distance_inner_product_applier(grn_ctx *ctx, grn_applier_data *data)
{
  return distance_applier(ctx,
                          data,
                          Method::InnerProduct,
                          "distance_inner_product()");
}

extern "C" float
grn_distance_l1_norm(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
{
  const char *tag = "[distance][l1-norm]";
  float distance = 0.0;

  if (!validate_vectors(ctx, vector1, vector2, tag)) {
    return distance;
  }

  switch (vector1->header.domain) {
  case GRN_DB_FLOAT32:
    distance = compute_distance_l1_norm<float>(ctx, vector1, vector2);
    break;
  case GRN_DB_FLOAT:
    distance = compute_distance_l1_norm<double>(ctx, vector1, vector2);
    break;
  default:
    // TODO: We should add support for all integer types
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "%s unsupported type: %s",
        tag,
        grn_type_id_to_string_builtin(ctx, vector1->header.domain));
    break;
  }

  return distance;
}

extern "C" grn_rc
grn_distance_l1_norm_applier(grn_ctx *ctx, grn_applier_data *data)
{
  return distance_applier(ctx, data, Method::L1Norm, "distance_l1_norm()");
}

extern "C" float
grn_distance_l2_norm_squared(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
{
  const char *tag = "[distance][l2-norm-squared]";
  float distance = 0.0;

  if (!validate_vectors(ctx, vector1, vector2, tag)) {
    return distance;
  }

  switch (vector1->header.domain) {
  case GRN_DB_FLOAT32:
    distance = compute_distance_l2_norm_squared<float>(ctx, vector1, vector2);
    break;
  case GRN_DB_FLOAT:
    distance = compute_distance_l2_norm_squared<double>(ctx, vector1, vector2);
    break;
  default:
    // TODO: We should add support for all integer types
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "%s unsupported type: %s",
        tag,
        grn_type_id_to_string_builtin(ctx, vector1->header.domain));
    break;
  }

  return distance;
}

extern "C" grn_rc
grn_distance_l2_norm_squared_applier(grn_ctx *ctx, grn_applier_data *data)
{
  return distance_applier(ctx,
                          data,
                          Method::L2NormSquared,
                          "distance_l2_norm_squared()");
}
