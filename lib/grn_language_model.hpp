/*
  Copyright (C) 2024-2025  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include "grn_ctx.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace grn {
  namespace language_model {
    void
    init_from_env();
    void
    fin_external_libraries();
  } // namespace language_model

  class LanguageModelInferencer;
  class LanguageModelLoader;

  class LanguageModel : public std::enable_shared_from_this<LanguageModel> {
  public:
    class Impl;

    LanguageModel(Impl *impl);
    ~LanguageModel();

    std::unique_ptr<LanguageModelInferencer>
    make_inferencer(grn_ctx *ctx);
    uint32_t
    get_n_embedding_dimensions(grn_ctx *ctx);

  private:
    std::unique_ptr<Impl> impl_;
  };

  class LanguageModelLoader {
  public:
    LanguageModelLoader(grn_ctx *ctx) : ctx_(ctx) {}

    std::shared_ptr<LanguageModel>
    load();

    std::string model_path;
    int32_t n_gpu_layers = GRN_LANGUAGE_MODEL_LOADER_N_GPU_LAYERS_DEFAULT;

  private:
    grn_ctx *ctx_;
  };

  class LanguageModelInferencer {
  public:
    class Impl;

    LanguageModelInferencer(Impl *impl);
    ~LanguageModelInferencer();

    /// \brief Prepend `prefix` to all values of `input_column` in
    ///        \ref vectorize_in_batch.
    ///
    /// \param prefix The prefix for all values of `input_column.
    ///
    /// \since 15.1.9
    void
    set_input_column_value_prefix(std::string prefix);

    /// \brief Set progress callback that is called on each batch
    ///        vectorization is completed.
    ///
    /// \param callback The callback.
    /// \param user_data The data that is passed to the `callback` when
    ///                  `callback` is called.
    ///
    /// \since 15.2.1
    void
    set_progress_callback(grn_progress_callback_func callback, void *user_data);

    /// \brief Generate an embedding for a record
    ///
    /// \param ctx A \ref grn_ctx.
    /// \param text The text to be vectorized.
    /// \param output_vector It must be an Float32 vector. This
    ///                      function doesn't validate it.
    void
    vectorize(grn_ctx *ctx, std::string_view text, grn_obj *output_vector);

    /// \brief Generate embeddings for multiple records in batch
    ///
    /// \param ctx A \ref grn_ctx.
    /// \param cursor It must be a valid \ref grn_table_cursor.
    ///               This function doesn't validate it.
    /// \param input_column It must be an text-family scalar
    ///                     column. This function doesn't validate it.

    /// \param output It must be a `Float32` vector column or a
    ///               `Float32` vector. This function doesn't validate
    ///               it.
    ///
    ///               If it's a `Float32` vector, output order is same
    ///               as IDs returned by the `cursor`.
    void
    vectorize_in_batch(grn_ctx *ctx,
                       grn_table_cursor *cursor,
                       grn_obj *input_column,
                       grn_obj *output);

  private:
    std::unique_ptr<Impl> impl_;
  };
} // namespace grn
