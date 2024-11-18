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
#include "grn_db.h"

#include "groonga/smart_obj.hpp"

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
    } // namespace
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

    static bool initialized = false;
    static std::once_flag initialize_once;

    namespace {
      void
      init_external_libraries(void)
      {
        llama_log_set(log_callback, &grn_gctx);
        llama_backend_init();
        initialized = true;
      }

      void
      ensure_init_external_libraries(void)
      {
        std::call_once(initialize_once, init_external_libraries);
      }
    } // namespace
#else
    namespace {
      void
      ensure_init_external_libraries(void)
      {
      }
    } // namespace
#endif

    void
    fin_external_libraries(void)
    {
#ifdef GRN_WITH_LLAMA_CPP
      if (!initialized) return;

      model_cache.clear();
      llama_backend_free();
      llama_log_set(nullptr, nullptr);
#endif
    }

#ifdef GRN_WITH_LLAMA_CPP
#  ifdef _WIN32
    static char *windows_language_models_dir = NULL;
    static char windows_language_models_dir_buffer[PATH_MAX];
    namespace {
      const char *
      default_system_language_models_dir(void)
      {
        if (!windows_language_models_dir) {
          const char *base_dir;
          const char *relative_path = GRN_RELATIVE_LANGUAGE_MODELS_DIR;

          base_dir = grn_windows_base_dir();
          grn_strcpy(windows_language_models_dir_buffer, PATH_MAX, base_dir);
          grn_strcat(windows_language_models_dir_buffer, PATH_MAX, "/");
          grn_strcat(windows_language_models_dir_buffer,
                     PATH_MAX,
                     relative_path);
          windows_language_models_dir = windows_language_models_dir_buffer;
        }
        return windows_language_models_dir;
      }
    } // namespace
#  else
    namespace {
      const char *
      default_system_language_models_dir(void)
      {
        return GRN_LANGUAGE_MODELS_DIR;
      }
    } // namespace
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
#ifdef GRN_WITH_LLAMA_CPP
    struct BatchReleaser {
      llama_batch *batch_;

      BatchReleaser(llama_batch *batch) : batch_(batch) {}
      ~BatchReleaser()
      {
        if (batch_) {
          llama_batch_free(*batch_);
        }
      }
    };
#endif
  public:
#ifdef GRN_WITH_LLAMA_CPP
    Impl(grn_ctx *ctx,
         std::shared_ptr<LanguageModel> model,
         llama_context *llama_ctx)
      : ctx_(ctx),
        model_(std::move(model)),
        llama_ctx_(llama_ctx),
        llama_model_(llama_get_model(llama_ctx_)),
        n_dimentions_(llama_n_embd(llama_model_)),
        has_encoder_(llama_model_has_encoder(llama_model_)),
        has_decoder_(llama_model_has_decoder(llama_model_)),
        pooling_type_(llama_pooling_type(llama_ctx_))
    {
    }

    ~Impl() { llama_free(llama_ctx_); }
#endif

    void
    vectorize(std::string_view text, grn_obj *output_vector)
    {
#ifdef GRN_WITH_LLAMA_CPP
      language_model::CaptureError capture(ctx_);

      std::vector<llama_token> tokens;
      tokenize(text, tokens);
      auto batch = llama_batch_init(tokens.size(), 0, 1);
      BatchReleaser batch_releaser(&batch);
      const llama_seq_id sequence_id = 0;
      add_tokens(batch, tokens, sequence_id);

      if (!vectorize_batch(batch)) {
        return;
      }

      if (!store_embeddings(batch, sequence_id, output_vector)) {
        return;
      }
#else
      auto ctx = ctx_;
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "[language-model-inferencer][vectorize] llama.cpp isn't enabled");
#endif
    }

    void
    vectorize_in_batch(grn_table_cursor *cursor,
                       grn_obj *input_column,
                       grn_obj *output_column)
    {
#ifdef GRN_WITH_LLAMA_CPP
      language_model::CaptureError capture(ctx_);

      std::vector<grn_id> target_ids;
      int32_t max_n_tokens = 2048;
      auto batch = llama_batch_init(max_n_tokens, 0, 1);
      BatchReleaser batch_releaser(&batch);

      grn_obj embeddings;
      GRN_FLOAT32_INIT(&embeddings, GRN_OBJ_VECTOR);
      grn::UniqueObj smart_embeddings(ctx_, &embeddings);

      auto flush_batch = [&]() {
        if (!vectorize_batch(batch)) {
          return false;
        }
        const auto n_targets = target_ids.size();
        for (size_t i = 0; i < n_targets; ++i) {
          const auto sequence_id = static_cast<llama_seq_id>(i);
          GRN_BULK_REWIND(&embeddings);
          if (!store_embeddings(batch, sequence_id, &embeddings)) {
            return false;
          }
          const auto target_id = target_ids[i];
          grn_obj_set_value(ctx_,
                            output_column,
                            target_id,
                            &embeddings,
                            GRN_OBJ_SET);
        }
        target_ids.clear();
        batch.n_tokens = 0;
        return true;
      };

      std::vector<llama_token> tokens;
      grn_id id;
      while ((id = grn_table_cursor_next(ctx_, cursor)) != GRN_ID_NIL) {
        uint32_t input_size = 0;
        auto input = grn_obj_get_value_(ctx_, input_column, id, &input_size);
        tokenize(input, tokens);
        const auto n_tokens = static_cast<int32_t>(tokens.size());
        if (n_tokens == 0) {
          continue;
        }
        const auto batch_is_full = (batch.n_tokens + n_tokens > max_n_tokens);
        if (batch_is_full) {
          const auto batch_is_small = (batch.n_tokens == 0);
          if (batch_is_small) {
            llama_batch_free(batch);
            batch_releaser.batch_ = nullptr;
            max_n_tokens = tokens.size();
            batch = llama_batch_init(max_n_tokens, 0, 1);
            batch_releaser.batch_ = &batch;
            add_tokens(batch, tokens, target_ids.size());
            target_ids.push_back(id);
          }
          if (!flush_batch()) {
            return;
          }
          if (batch_is_small) {
            continue;
          }
        }
        add_tokens(batch, tokens, target_ids.size());
        target_ids.push_back(id);
      }
      if (target_ids.size() > 0) {
        if (!flush_batch()) {
          return;
        }
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
    const llama_model *llama_model_;
    const int32_t n_dimentions_;
    const bool has_encoder_;
    const bool has_decoder_;
    const enum llama_pooling_type pooling_type_;

    void
    tokenize(std::string_view text, std::vector<llama_token> &tokens)
    {
      auto model = llama_get_model(llama_ctx_);
      constexpr auto add_special = true;
      constexpr auto parse_special = false;
      // Guess enough size
      int n_tokens = text.length() + 2 * add_special;
      if (tokens.capacity() < static_cast<size_t>(n_tokens)) {
        tokens.reserve(n_tokens);
      }
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
    }

    void
    add_tokens(llama_batch &batch,
               const std::vector<llama_token> &tokens,
               llama_seq_id sequence_id)
    {
      const auto offset = batch.n_tokens;
      const auto n_tokens = static_cast<int32_t>(tokens.size());
      memcpy(batch.token + offset,
             tokens.data(),
             sizeof(llama_token) * n_tokens);
      for (int32_t i = 0; i < n_tokens; ++i) {
        const auto offset_i = offset + i;
        batch.pos[offset_i] = i;
        batch.n_seq_id[offset_i] = 1;
        batch.seq_id[offset_i][0] = sequence_id;
        batch.logits[offset_i] = true;
      }
      batch.n_tokens += n_tokens;
    }

    bool
    vectorize_batch(llama_batch &batch)
    {
      llama_kv_cache_clear(llama_ctx_);

      if (has_encoder_ && !has_decoder_) {
        // encoder-only model
        if (llama_encode(llama_ctx_, batch) < 0) {
          GRN_LM_ERROR(
            ctx_,
            GRN_UNKNOWN_ERROR,
            "[language-model-inferencer][vectorize-batch] failed to encode");
          return false;
        }
      } else if (!has_encoder_ && has_decoder_) {
        // decoder-only model
        if (llama_decode(llama_ctx_, batch) < 0) {
          GRN_LM_ERROR(
            ctx_,
            GRN_UNKNOWN_ERROR,
            "[language-model-inferencer][vectorize-batch] failed to decode");
          return false;
        }
      } else {
        GRN_LM_ERROR(
          ctx_,
          GRN_FUNCTION_NOT_IMPLEMENTED,
          "[language-model-inferencer][vectorize-batch] "
          "model that has both of encoder and docoder isn't supported yet");
        return false;
      }

      return true;
    }

    bool
    store_embeddings(llama_batch &batch,
                     llama_seq_id id,
                     grn_obj *output_vector)
    {
      // pooling_type_ must not be LLAMA_POOLING_TYPE_NONE.
      auto raw_embeddings = llama_get_embeddings_seq(llama_ctx_, id);
      if (!raw_embeddings) {
        GRN_LM_ERROR(ctx_,
                     GRN_UNKNOWN_ERROR,
                     "[language-model-inferencer][store-embeddings] "
                     "failed to get embeddings");
        return false;
      }

      // TODO: grn::distance::compute_l2_norm()
      float square_sum = 0.0;
      for (int32_t i = 0; i < n_dimentions_; ++i) {
        square_sum += raw_embeddings[i] * raw_embeddings[i];
      }
      auto magnitude = std::sqrt(square_sum);
      const float normalize = magnitude > 0.0 ? 1.0 / magnitude : 0.0f;
      for (int i = 0; i < n_dimentions_; ++i) {
        auto normalized_value = raw_embeddings[i] * normalize;
        GRN_FLOAT32_PUT(ctx_, output_vector, normalized_value);
      }
      return true;
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

  void
  LanguageModelInferencer::vectorize_in_batch(grn_table_cursor *cursor,
                                              grn_obj *input_column,
                                              grn_obj *output_column)
  {
    return impl_->vectorize_in_batch(cursor, input_column, output_column);
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
  grn::language_model::ensure_init_external_libraries();
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
  grn::language_model::ensure_init_external_libraries();

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

grn_rc
grn_language_model_inferencer_vectorize_applier(
  grn_ctx *ctx,
  grn_language_model_inferencer *inferencer,
  grn_obj *input_column,
  grn_applier_data *data)
{
  GRN_API_ENTER;
  if (!inferencer) {
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model-inferencer][vectorize-applier] "
        "inferencer must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  grn_obj *table = grn_applier_data_get_table(ctx, data);
  grn_obj *output_column = grn_applier_data_get_output_column(ctx, data);
  if (!(grn_obj_is_vector_column(ctx, output_column) &&
        DB_OBJ(output_column)->range == GRN_DB_FLOAT32)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, output_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model-inferencer][vectorize-applier] "
        "output column must be a Float32 vector column: %.*s",
        static_cast<int>(GRN_TEXT_LEN(&inspected)),
        GRN_TEXT_VALUE(&inspected));
    GRN_API_RETURN(ctx->rc);
  }
  auto cursor =
    grn_table_cursor_open(ctx, table, nullptr, 0, nullptr, 0, 0, -1, 0);
  if (cursor) {
    inferencer->inferencer->vectorize_in_batch(cursor,
                                               input_column,
                                               output_column);
    grn_table_cursor_close(ctx, cursor);
  }
  GRN_API_RETURN(ctx->rc);
}
}
