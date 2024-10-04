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

#pragma once

#include "grn_ctx.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace grn {
  namespace language_model {
    void
    init_from_env(void);
    void
    init_external_libraries(void);
    void
    fin_external_libraries(void);
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

  private:
    std::unique_ptr<Impl> impl_;
  };

  class LanguageModelLoader {
  public:
    LanguageModelLoader(grn_ctx *ctx) : ctx_(ctx) {}

    std::shared_ptr<LanguageModel>
    load(void);

    std::string model_path;
    int32_t n_gpu_layers = 0;

  private:
    grn_ctx *ctx_;
  };

  class LanguageModelInferencer {
  public:
    class Impl;

    LanguageModelInferencer(Impl *impl);
    ~LanguageModelInferencer();

    void
    vectorize(std::string_view text, grn_obj *output_vector);

  private:
    std::unique_ptr<Impl> impl_;
  };
} // namespace grn
