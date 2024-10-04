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

#include "grn_language_model.hpp"

#ifdef GRN_WITH_LLAMA_CPP
#  include <llama.h>
#endif

#include <cmath>
#include <functional>
#include <map>
#include <mutex>

#define GRN_LM_ERROR(ctx_, default_rc, message)                                \
  do {                                                                         \
    grn_ctx *ctx = (ctx_);                                                     \
    grn_rc rc = ctx->rc == GRN_SUCCESS ? (default_rc) : ctx->rc;               \
    char errbuf[GRN_CTX_MSGSIZE];                                              \
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);                          \
    ERR(rc, "%s: %s", (message), errbuf);                                      \
  } while (false)

namespace grn {
  namespace language_model {
#ifdef GRN_WITH_LLAMA_CPP
    namespace {
      void
      log_callback(ggml_log_level level, const char *text, void *user_data)
      {
        grn_ctx *ctx = static_cast<grn_ctx *>(user_data);
        switch (level) {
        case GGML_LOG_LEVEL_ERROR:
          ERR(GRN_UNKNOWN_ERROR, "%s", text);
          break;
        case GGML_LOG_LEVEL_WARN:
          if (grn_logger_pass(ctx, GRN_LOG_WARNING)) {
            grn_logger_put(ctx,
                           GRN_LOG_WARNING,
                           __FILE__,
                           __LINE__,
                           __FUNCTION__,
                           "%s",
                           text);
          }
          break;
        case GGML_LOG_LEVEL_INFO:
          if (grn_logger_pass(ctx, GRN_LOG_INFO)) {
            grn_logger_put(ctx,
                           GRN_LOG_INFO,
                           __FILE__,
                           __LINE__,
                           __FUNCTION__,
                           "%s",
                           text);
          }
          break;
        case GGML_LOG_LEVEL_DEBUG:
          if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
            grn_logger_put(ctx,
                           GRN_LOG_DEBUG,
                           __FILE__,
                           __LINE__,
                           __FUNCTION__,
                           "%s",
                           text);
          }
          break;
        default:
          ERR(GRN_UNKNOWN_ERROR, "%s", text);
          break;
        }
      }
    }; // namespace
#endif

    static char language_models_dir[GRN_ENV_BUFFER_SIZE];

    void
    init_from_env(void)
    {
      grn_getenv("GRN_LANGUAGE_MODELS_DIR",
                 language_models_dir,
                 GRN_ENV_BUFFER_SIZE);
    }

#ifdef GRN_WITH_LLAMA_CPP
    struct ModelCache {
    public:
      ModelCache() = default;
      ~ModelCache() = default;

      std::shared_ptr<LanguageModel>
      get(const std::string &path,
          std::function<std::shared_ptr<LanguageModel>()> load)
      {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = models_.find(path);
        if (it != models_.end()) {
          return it->second;
        }

        auto model = load();
        if (model) {
          models_[path] = model;
        }
        return model;
      }

      void
      clear()
      {
        models_.clear();
      }

    private:
      std::map<std::string, std::shared_ptr<LanguageModel>> models_;
      std::mutex mutex_;
    };

    static ModelCache model_cache;
#endif

    void
    init_external_libraries(void)
    {
#ifdef GRN_WITH_LLAMA_CPP
      llama_log_set(log_callback, &grn_gctx);
      llama_backend_init();
#endif
    }

    void
    fin_external_libraries(void)
    {
#ifdef GRN_WITH_LLAMA_CPP
      model_cache.clear();
      llama_backend_free();
      llama_log_set(nullptr, nullptr);
#endif
    }

#ifdef GRN_WITH_LLAMA_CPP
#  ifdef _WIN32
    static char *windows_language_models_dir = NULL;
    static char windows_language_models_dir_buffer[PATH_MAX];
    static const char *
    default_system_language_models_dir(void)
    {
      if (!windows_language_models_dir) {
        const char *base_dir;
        const char *relative_path = GRN_RELATIVE_LANGUAGE_MODELS_DIR;

        base_dir = grn_windows_base_dir();
        grn_strcpy(windows_language_models_dir_buffer, PATH_MAX, base_dir);
        grn_strcat(windows_language_models_dir_buffer, PATH_MAX, "/");
        grn_strcat(windows_language_models_dir_buffer, PATH_MAX, relative_path);
        windows_language_models_dir = windows_language_models_dir_buffer;
      }
      return windows_language_models_dir;
    }
#  else
    static const char *
    default_system_language_models_dir(void)
    {
      return GRN_LANGUAGE_MODELS_DIR;
    }
#  endif

    const char *
    system_language_models_dir(void)
    {
      if (language_models_dir[0]) {
        return language_models_dir;
      } else {
        return default_system_language_models_dir();
      }
    }

    static std::mutex capture_error_mutex;
    class CaptureError {
    public:
      CaptureError(grn_ctx *ctx) : lock_(capture_error_mutex)
      {
        llama_log_set(log_callback, ctx);
      }

      ~CaptureError(void) { llama_log_set(log_callback, &grn_gctx); }

    private:
      std::lock_guard<std::mutex> lock_;
    };
#endif
  }; // namespace language_model

  class LanguageModel::Impl {
#ifdef GRN_WITH_LLAMA_CPP
  public:
    Impl(llama_model *model) : model_(model) {}

    ~Impl(void) { llama_free_model(model_); }

    llama_model *
    get_raw()
    {
      return model_;
    }

  private:
    llama_model *model_;
#endif
  };

  LanguageModel::LanguageModel(Impl *impl) : impl_(impl) {}

  LanguageModel::~LanguageModel() = default;

  std::shared_ptr<LanguageModel>
  LanguageModelLoader::load(void)
  {
    auto ctx = ctx_;

#ifdef GRN_WITH_LLAMA_CPP
    if (model_path.empty()) {
      ERR(GRN_INVALID_ARGUMENT,
          "[language-model-loader][load] model path is missing");
      return nullptr;
    }

    auto model = language_model::model_cache.get(
      model_path,
      [this]() -> std::shared_ptr<LanguageModel> {
        auto ctx = ctx_;
        auto params = llama_model_default_params();
        params.n_gpu_layers = n_gpu_layers;
        params.progress_callback = [](float progress, void *ctx) {
          return true;
        };

        llama_model *raw_model;
        {
          language_model::CaptureError capture(ctx_);
          raw_model = llama_load_model_from_file(model_path.c_str(), params);
          if (!raw_model) {
            GRN_LM_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "[language-model-loader][load] failed to load model");
            return nullptr;
          }

          if (llama_model_has_encoder(raw_model) &&
              !llama_model_has_decoder(raw_model)) {
            ERR(GRN_INVALID_ARGUMENT,
                "[language-model-loader][load] encoder-decoder model isn't "
                "supported yet: <%s>",
                model_path.c_str());
            llama_free_model(raw_model);
            return nullptr;
          }
        }

        return std::make_shared<LanguageModel>(
          new LanguageModel::Impl(raw_model));
      });
    return model;
#else
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[language-model-loader][load] llama.cpp isn't enabled");
    return nullptr;
#endif
  }

  class LanguageModelInferencer::Impl {
  public:
#ifdef GRN_WITH_LLAMA_CPP
    Impl(grn_ctx *ctx,
         std::shared_ptr<LanguageModel> model,
         llama_context *llama_ctx)
      : ctx_(ctx),
        model_(std::move(model)),
        llama_ctx_(llama_ctx)
    {
    }

    ~Impl() { llama_free(llama_ctx_); }
#endif

    void
    vectorize(std::string_view text, grn_obj *output_vector)
    {
#ifdef GRN_WITH_LLAMA_CPP
      language_model::CaptureError capture(ctx_);

      const auto model = llama_get_model(llama_ctx_);
      auto tokens = tokenize(text);
      auto batch = llama_batch_init(tokens.size(), 0, 1);
      batch.n_tokens = tokens.size();
      memcpy(batch.token, tokens.data(), sizeof(llama_token) * batch.n_tokens);
      for (int i = 0; i < batch.n_tokens; ++i) {
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = true;
      }
      auto n_dimentions = llama_n_embd(model);

      llama_kv_cache_clear(llama_ctx_);

      if (llama_model_has_encoder(model) && !llama_model_has_decoder(model)) {
        // encoder-only model
        if (llama_encode(llama_ctx_, batch) < 0) {
          GRN_LM_ERROR(
            ctx_,
            GRN_UNKNOWN_ERROR,
            "[language-model-inferencer][vectorize] failed to encode");
        }
      } else if (!llama_model_has_encoder(model) &&
                 llama_model_has_decoder(model)) {
        // decoder-only model
        if (llama_decode(llama_ctx_, batch) < 0) {
          GRN_LM_ERROR(
            ctx_,
            GRN_UNKNOWN_ERROR,
            "[language-model-inferencer][vectorize] failed to decode");
        }
      }

      auto pooling_type = llama_pooling_type(llama_ctx_);
      float *raw_embeddings;
      if (pooling_type == LLAMA_POOLING_TYPE_NONE) {
        raw_embeddings =
          llama_get_embeddings_ith(llama_ctx_, batch.n_tokens - 1);
      } else {
        raw_embeddings =
          llama_get_embeddings_seq(llama_ctx_,
                                   batch.seq_id[batch.n_tokens - 1][0]);
      }
      if (!raw_embeddings) {
        GRN_LM_ERROR(ctx_,
                     GRN_UNKNOWN_ERROR,
                     "[language-model-inferencer][vectorize] "
                     "failed to get embeddings");
      }

      // TODO: grn::distance::compute_l2_norm()
      float square_sum = 0.0;
      for (int32_t i = 0; i < n_dimentions; ++i) {
        square_sum += raw_embeddings[i] * raw_embeddings[i];
      }
      auto magnitude = std::sqrt(square_sum);
      const float normalize = magnitude > 0.0 ? 1.0 / magnitude : 0.0f;
      for (int i = 0; i < n_dimentions; ++i) {
        auto normalized_value = raw_embeddings[i] * normalize;
        GRN_FLOAT32_PUT(ctx_, output_vector, normalized_value);
      }
#else
      auto ctx = ctx_;
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "[language-model-inferencer][vectorize] llama.cpp isn't enabled");
#endif
    }

  private:
    grn_ctx *ctx_;
#ifdef GRN_WITH_LLAMA_CPP
    std::shared_ptr<LanguageModel> model_;
    llama_context *llama_ctx_;

    std::vector<llama_token>
    tokenize(std::string_view text)
    {
      auto model = llama_get_model(llama_ctx_);
      constexpr auto add_special = true;
      constexpr auto parse_special = false;
      // Guess enough size
      int n_tokens = text.length() + 2 * add_special;
      std::vector<llama_token> tokens(n_tokens);
      n_tokens = llama_tokenize(model,
                                text.data(),
                                text.length(),
                                tokens.data(),
                                tokens.size(),
                                add_special,
                                parse_special);
      if (n_tokens < 0) {
        // If guessed size isn't enough, use the real size.
        tokens.resize(-n_tokens);
        llama_tokenize(model,
                       text.data(),
                       text.length(),
                       tokens.data(),
                       tokens.size(),
                       add_special,
                       parse_special);
      } else {
        tokens.resize(n_tokens);
      }
      return tokens;
    }
#endif
  };

  LanguageModelInferencer::LanguageModelInferencer(Impl *impl)
    : impl_(std::unique_ptr<Impl>(impl))
  {
  }

  LanguageModelInferencer::~LanguageModelInferencer() = default;

  void
  LanguageModelInferencer::vectorize(std::string_view text,
                                     grn_obj *output_vector)
  {
    return impl_->vectorize(text, output_vector);
  }

  std::unique_ptr<LanguageModelInferencer>
  LanguageModel::make_inferencer(grn_ctx *ctx)
  {
#ifdef GRN_WITH_LLAMA_CPP
    auto params = llama_context_default_params();
    params.embeddings = true;
    // We want document vector not token vectors. We want to use the
    // default pooling type in a model but it seems that most models
    // don't provide the default pooling type (LLAMA_POOLING_TYPE_NONE
    // is used). If LLAMA_POOLING_TYPE_NONE is used, token vectors are
    // generated. So we force to use LLAMA_POOLING_TYPE_MEAN here.
    params.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    return std::make_unique<LanguageModelInferencer>(
      new LanguageModelInferencer::Impl(
        ctx,
        shared_from_this(),
        llama_new_context_with_model(impl_->get_raw(), params)));
#else
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[language-model][make-inferencer] llama.cpp isn't enabled");
    return nullptr;
#endif
  }
}; // namespace grn

extern "C" {
struct grn_language_model_ {
  std::shared_ptr<grn::LanguageModel> model;

  grn_language_model_() : model(nullptr) {}
  ~grn_language_model_() = default;
};

struct grn_language_model_inferencer_ {
  std::shared_ptr<grn::LanguageModelInferencer> inferencer;

  grn_language_model_inferencer_() : inferencer(nullptr) {}
  ~grn_language_model_inferencer_() = default;
};
struct grn_language_model_loader_ {
  grn::LanguageModelLoader loader;

  grn_language_model_loader_(grn_ctx *ctx) : loader(ctx) {}
  ~grn_language_model_loader_() = default;
};

grn_language_model_loader *
grn_language_model_loader_open(grn_ctx *ctx)
{
  auto loader = new grn_language_model_loader_(ctx);
  return loader;
}

grn_rc
grn_language_model_loader_close(grn_ctx *ctx, grn_language_model_loader *loader)
{
  delete loader;
  return GRN_SUCCESS;
}

grn_rc
grn_language_model_loader_set_model(grn_ctx *ctx,
                                    grn_language_model_loader *loader,
                                    const char *model,
                                    int64_t model_length)
{
  GRN_API_ENTER;
  if (!loader) {
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model-loader][set-model] loader must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
#ifdef GRN_WITH_LLAMA_CPP
  if (model_length < 0) {
    loader->loader.model_path = std::string(model, strlen(model));
  } else {
    loader->loader.model_path = std::string(model, model_length);
  }
  if (!loader->loader.model_path.empty()) {
    if (loader->loader.model_path[0] != '/') {
      std::string model_path =
        grn::language_model::system_language_models_dir();
      model_path += "/" + loader->loader.model_path + ".gguf";
      loader->loader.model_path = std::move(model_path);
    }
  }
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
      "[language-model-loader][set-model] llama.cpp isn't enabled");
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_language_model *
grn_language_model_loader_load(grn_ctx *ctx, grn_language_model_loader *loader)
{
  GRN_API_ENTER;
  if (!loader) {
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model-loader][loader] loader must not be NULL");
    GRN_API_RETURN(NULL);
  }
  auto model = new grn_language_model();
  model->model = loader->loader.load();
  if (!model->model) {
    delete model;
    GRN_API_RETURN(NULL);
  }
  GRN_API_RETURN(model);
}

grn_rc
grn_language_model_close(grn_ctx *ctx, grn_language_model *model)
{
  delete model;
  return GRN_SUCCESS;
}

grn_language_model_inferencer *
grn_language_model_open_inferencer(grn_ctx *ctx, grn_language_model *model)
{
  GRN_API_ENTER;
  if (!model) {
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model][open-inferencer] model must not be NULL");
    GRN_API_RETURN(NULL);
  }
  auto inferencer = new grn_language_model_inferencer_();
  inferencer->inferencer = model->model->make_inferencer(ctx);
  if (!inferencer->inferencer) {
    delete inferencer;
    GRN_API_RETURN(NULL);
  }
  GRN_API_RETURN(inferencer);
}

grn_rc
grn_language_model_inferencer_close(grn_ctx *ctx,
                                    grn_language_model_inferencer *inferencer)
{
  delete inferencer;
  return GRN_SUCCESS;
}

grn_rc
grn_language_model_inferencer_vectorize(
  grn_ctx *ctx,
  grn_language_model_inferencer *inferencer,
  const char *text,
  int64_t text_length,
  grn_obj *output_vector)
{
  GRN_API_ENTER;
  if (!inferencer) {
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model-inferencer][vectorize] inferencer must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  if (!output_vector) {
    ERR(
      GRN_INVALID_ARGUMENT,
      "[language-model-inferencer][vectorize] output vector must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  if (text_length < 0) {
    text_length = static_cast<int64_t>(strlen(text));
  }
  if (text_length > 0) {
    inferencer->inferencer->vectorize(std::string_view(text, text_length),
                                      output_vector);
  }
  GRN_API_RETURN(ctx->rc);
}
}
