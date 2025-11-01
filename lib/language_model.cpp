// Copyright (C) 2024-2025  Sutou Kouhei <kou@clear-code.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

#include "grn_db.h"
#include "grn_http_client.h"
#include "grn_language_model.hpp"
#include "grn_util.h"

#include <groonga/smart_obj.hpp>

#ifdef GRN_WITH_LLAMA_CPP
#  include <ggml-backend.h>
#  include <llama.h>
#endif

#ifdef GRN_WITH_SIMDJSON
#  include <simdjson.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>

#define GRN_LM_ERROR(default_rc, message)                                      \
  do {                                                                         \
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

    static char ggml_backends_dir[GRN_ENV_BUFFER_SIZE];
    static char language_models_dir[GRN_ENV_BUFFER_SIZE];
    static char language_model_download_cache_dir[GRN_ENV_BUFFER_SIZE];

    void
    init_from_env()
    {
      grn_getenv("GRN_GGML_BACKENDS_DIR",
                 ggml_backends_dir,
                 GRN_ENV_BUFFER_SIZE);
      grn_getenv("GRN_LANGUAGE_MODELS_DIR",
                 language_models_dir,
                 GRN_ENV_BUFFER_SIZE);
      grn_getenv("GRN_LANGUAGE_MODEL_DOWNLOAD_CACHE_DIR",
                 language_model_download_cache_dir,
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
#  ifdef GRN_WITH_LLAMA_CPP_BUNDLED
#    ifdef _WIN32
      const char *
      get_default_ggml_backends_dir()
      {
        static char *windows_ggml_backends_dir = nullptr;
        static char windows_ggml_backends_dir_buffer[PATH_MAX];
        if (!windows_ggml_backends_dir) {
          auto base_dir = grn_windows_base_dir();
          auto base_dir_length = strlen(base_dir);
          grn_strcpy(windows_ggml_backends_dir_buffer, PATH_MAX, base_dir);
          grn_strcat(windows_ggml_backends_dir_buffer, PATH_MAX, "/");
          grn_strcat(windows_ggml_backends_dir_buffer,
                     PATH_MAX,
                     GRN_RELATIVE_GGML_BACKENDS_DIR);
          windows_ggml_backends_dir = windows_ggml_backends_dir_buffer;
        }
        return windows_ggml_backends_dir;
      }
#    else
      const char *
      get_default_ggml_backends_dir()
      {
        return GRN_GGML_BACKENDS_DIR;
      }
#    endif

      const char *
      get_ggml_backends_dir()
      {
        if (ggml_backends_dir[0]) {
          return ggml_backends_dir;
        } else {
          return get_default_ggml_backends_dir();
        }
      }
#  endif

      void
      init_external_libraries()
      {
        llama_log_set(log_callback, &grn_gctx);
#  ifdef GRN_WITH_LLAMA_CPP_BUNDLED
        ggml_backend_load_all_from_path(get_ggml_backends_dir());
#  endif
        llama_backend_init();
        initialized = true;
      }

      void
      ensure_init_external_libraries()
      {
        std::call_once(initialize_once, init_external_libraries);
      }
    } // namespace
#else
    namespace {
      void
      ensure_init_external_libraries()
      {
      }
    } // namespace
#endif

    void
    fin_external_libraries()
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
      default_system_language_models_dir()
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
      default_system_language_models_dir()
      {
        return GRN_LANGUAGE_MODELS_DIR;
      }
    } // namespace
#  endif

    const char *
    system_language_models_dir()
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

      ~CaptureError() { llama_log_set(log_callback, &grn_gctx); }

    private:
      std::lock_guard<std::mutex> lock_;
    };
#endif
  }; // namespace language_model

  class LanguageModel::Impl {
#ifdef GRN_WITH_LLAMA_CPP
  public:
    Impl(llama_model *model)
      : model_(model),
        default_pooling_type_(LLAMA_POOLING_TYPE_NONE)
    {
      auto params = llama_context_default_params();
      params.n_ctx = 0;
      params.embeddings = true;
      auto llama_ctx = llama_init_from_model(model_, params);
      default_pooling_type_ = llama_pooling_type(llama_ctx);
      llama_free(llama_ctx);
    }

    ~Impl() { llama_model_free(model_); }

    llama_model *
    get_raw()
    {
      return model_;
    }

    enum llama_pooling_type
    default_pooling_type()
    {
      return default_pooling_type_;
    }

  private:
    llama_model *model_;
    enum llama_pooling_type default_pooling_type_;
#endif
  };

  LanguageModel::LanguageModel(Impl *impl) : impl_(impl) {}

  LanguageModel::~LanguageModel() = default;

  std::shared_ptr<LanguageModel>
  LanguageModelLoader::load()
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
        params.progress_callback = [](float progress, void *ctx) {
          return true;
        };

        llama_model *raw_model;
        {
          language_model::CaptureError capture(ctx_);
          raw_model = llama_model_load_from_file(model_path.c_str(), params);
          if (!raw_model) {
            GRN_LM_ERROR(GRN_INVALID_ARGUMENT,
                         "[language-model-loader][load] failed to load model");
            return nullptr;
          }

          if (llama_model_has_encoder(raw_model) &&
              !llama_model_has_decoder(raw_model)) {
            ERR(GRN_INVALID_ARGUMENT,
                "[language-model-loader][load] encoder-decoder model isn't "
                "supported yet: <%s>",
                model_path.c_str());
            llama_model_free(raw_model);
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

#  ifdef GRN_WITH_LLAMA_CPP
    class Vectorizer {
    public:
      Vectorizer(llama_model *model_raw,
                 enum llama_pooling_type default_pooling_type)
        : llama_ctx_(nullptr),
          llama_model_(model_raw),
          n_dimensions_(llama_model_n_embd(llama_model_)),
          has_encoder_(llama_model_has_encoder(llama_model_)),
          has_decoder_(llama_model_has_decoder(llama_model_)),
          // We want document vector not token vectors. We want to use the
          // default pooling type in a model but it seems that most models
          // don't provide the default pooling type (LLAMA_POOLING_TYPE_NONE
          // is used). If LLAMA_POOLING_TYPE_NONE is used, token vectors are
          // generated. So we force to use LLAMA_POOLING_TYPE_MEAN here.
          pooling_type_(default_pooling_type == LLAMA_POOLING_TYPE_NONE
                          ? LLAMA_POOLING_TYPE_MEAN
                          : default_pooling_type),
          max_n_tokens_limit_(llama_model_n_ctx_train(llama_model_)),
          max_n_tokens_(llama_context_default_params().n_ubatch),
          pooling_buffer_(n_dimensions_)
      {
      }

      ~Vectorizer()
      {
        if (llama_ctx_) {
          llama_free(llama_ctx_);
        }
      }

      void
      vectorize(grn_ctx *ctx,
                std::string_view text,
                grn_obj *output_vector,
                const char *tag)
      {
        language_model::CaptureError capture(ctx);

        std::vector<llama_token> tokens;
        tokenize(text, tokens);
        auto n_tokens = static_cast<uint32_t>(tokens.size());
        adjust_max_n_tokens(ctx, GRN_ID_NIL, n_tokens, tag);
        auto batch = llama_batch_init(max_n_tokens_, 0, 1);
        BatchReleaser batch_releaser(&batch);
        if (n_tokens > max_n_tokens_) {
          const llama_seq_id sequence_id = 0;
          auto n_chunks = ((n_tokens - 1) / max_n_tokens_) + 1;
          for (size_t i = 0; i < n_chunks; ++i) {
            auto offset = max_n_tokens_ * i;
            auto size =
              std::min(n_tokens - offset, static_cast<size_t>(max_n_tokens_));
            add_tokens(batch, tokens, offset, size, sequence_id);

            if (!vectorize_batch(ctx, batch, sequence_id + 1)) {
              return;
            }

            if (!pool_embedding(ctx, batch, sequence_id, i, tag)) {
              return;
            }

            batch.n_tokens = 0;
            offset += size;
          }

          normalize_embedding(
            pooling_buffer_.data(),
            [&](int32_t dimension, float normalized_value) {
              GRN_FLOAT32_PUT(ctx, output_vector, normalized_value);
            });
        } else {
          const llama_seq_id sequence_id = 0;
          add_tokens(batch, tokens, 0, tokens.size(), sequence_id);

          if (!vectorize_batch(ctx, batch, sequence_id + 1)) {
            return;
          }

          if (!store_embedding(ctx, batch, sequence_id, output_vector, tag)) {
            return;
          }
        }
      }

      template <typename Output>
      void
      vectorize_in_batch(grn_ctx *ctx,
                         grn_table_cursor *cursor,
                         grn_obj *input_column,
                         Output output,
                         const char *tag)
      {
        language_model::CaptureError capture(ctx);

        struct Target {
          grn_id id;
          size_t current_chunk_index;
          size_t n_chunks;
        };

        std::vector<Target> targets;
        auto batch = llama_batch_init(max_n_tokens_, 0, 1);
        BatchReleaser batch_releaser(&batch);

        grn_obj embedding;
        GRN_FLOAT32_INIT(&embedding, GRN_OBJ_VECTOR);
        grn::UniqueObj smart_embedding(ctx, &embedding);

        auto flush_batch = [&]() {
          const auto n_sequences = targets.size();
          if (!vectorize_batch(ctx, batch, n_sequences)) {
            return false;
          }
          for (size_t i = 0; i < n_sequences; ++i) {
            const auto &target = targets[i];
            const auto sequence_id = static_cast<llama_seq_id>(i);
            if (target.n_chunks == 1) {
              GRN_BULK_REWIND(&embedding);
              if (!store_embedding(ctx, batch, sequence_id, &embedding, tag)) {
                return false;
              }
              output(ctx, target.id, &embedding);
            } else {
              if (!pool_embedding(ctx,
                                  batch,
                                  sequence_id,
                                  target.current_chunk_index,
                                  tag)) {
                return false;
              }
              if (target.current_chunk_index == target.n_chunks - 1) {
                GRN_BULK_REWIND(&embedding);
                normalize_embedding(
                  pooling_buffer_.data(),
                  [&](int32_t dimension, float normalized_value) {
                    GRN_FLOAT32_PUT(ctx, &embedding, normalized_value);
                  });
                output(ctx, target.id, &embedding);
              }
            }
          }
          targets.clear();
          batch.n_tokens = 0;
          return true;
        };

        std::vector<llama_token> tokens;

        auto batch_is_full = [&](uint32_t n_tokens) {
          auto next_n_tokens = batch.n_tokens + n_tokens;
          if (next_n_tokens > max_n_tokens_) {
            return true;
          }

          auto n_sequences = static_cast<uint32_t>(targets.size());
          if (n_sequences == 0) {
            return false;
          }

          auto actual_next_n_tokens =
            normalize_n_tokens(next_n_tokens, n_sequences);
          return actual_next_n_tokens < max_n_tokens_;
        };

        grn_id id;
        while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
          uint32_t input_size = 0;
          auto input = grn_obj_get_value_(ctx, input_column, id, &input_size);
          tokenize(input, tokens);
          auto n_tokens = static_cast<uint32_t>(tokens.size());
          if (n_tokens == 0) {
            continue;
          }
          if (batch_is_full(n_tokens)) {
            if (batch.n_tokens > 0) {
              if (!flush_batch()) {
                return;
              }
            }
            if (adjust_max_n_tokens(ctx, id, n_tokens, tag)) {
              llama_batch_free(batch);
              batch_releaser.batch_ = nullptr;
              batch = llama_batch_init(max_n_tokens_, 0, 1);
              batch_releaser.batch_ = &batch;
            }
          }
          auto n_chunks = ((n_tokens - 1) / max_n_tokens_) + 1;
          for (size_t i = 0; i < n_chunks; ++i) {
            auto offset = max_n_tokens_ * i;
            auto size =
              std::min(n_tokens - offset, static_cast<size_t>(max_n_tokens_));
            if (batch.n_tokens > 0 && batch_is_full(size)) {
              if (!flush_batch()) {
                return;
              }
            }
            add_tokens(batch, tokens, offset, size, targets.size());
            targets.push_back({id, i, n_chunks});
          }
        }
        if (targets.size() > 0) {
          if (!flush_batch()) {
            return;
          }
        }
      }

    private:
      llama_context *llama_ctx_;
      llama_model *llama_model_;
      const int32_t n_dimensions_;
      const bool has_encoder_;
      const bool has_decoder_;
      const enum llama_pooling_type pooling_type_;
      const uint32_t max_n_tokens_limit_;
      uint32_t max_n_tokens_;
      std::vector<float> pooling_buffer_;

      void
      tokenize(std::string_view text, std::vector<llama_token> &tokens)
      {
        constexpr auto add_special = true;
        constexpr auto parse_special = false;
        // Guess enough size
        int n_tokens = text.length() + 2 * add_special;
        if (tokens.capacity() < static_cast<size_t>(n_tokens)) {
          tokens.reserve(n_tokens);
        }
        auto vocab = llama_model_get_vocab(llama_model_);
        n_tokens = llama_tokenize(vocab,
                                  text.data(),
                                  text.length(),
                                  tokens.data(),
                                  tokens.size(),
                                  add_special,
                                  parse_special);
        if (n_tokens < 0) {
          // If guessed size isn't enough, use the real size.
          tokens.resize(-n_tokens);
          llama_tokenize(vocab,
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

      // Return true when max_n_tokens_ is updated.
      bool
      adjust_max_n_tokens(grn_ctx *ctx,
                          grn_id id,
                          size_t n_tokens,
                          const char *tag)
      {
        bool max_n_tokens_updated = false;
        if (n_tokens == 0) {
          return max_n_tokens_updated;
        }

        if (max_n_tokens_ != max_n_tokens_limit_) {
          while (max_n_tokens_ < n_tokens) {
            max_n_tokens_ *= 2;
          }
          if (max_n_tokens_ > max_n_tokens_limit_) {
            max_n_tokens_ = max_n_tokens_limit_;
          }
          max_n_tokens_updated = true;
        }

        return max_n_tokens_updated;
      }

      // TODO: We want to use std::span instead of std::vector +
      // tokens_star + toekns_size. It requires C++20 but llama.cpp
      // isn't C++20 ready.
      void
      add_tokens(llama_batch &batch,
                 const std::vector<llama_token> &tokens,
                 size_t tokens_start,
                 size_t n_target_tokens,
                 llama_seq_id sequence_id)
      {
        const auto offset = batch.n_tokens;
        memcpy(batch.token + offset,
               tokens.data() + tokens_start,
               sizeof(llama_token) * n_target_tokens);
        for (size_t i = 0; i < n_target_tokens; ++i) {
          const auto offset_i = offset + i;
          batch.pos[offset_i] = i;
          batch.n_seq_id[offset_i] = 1;
          batch.seq_id[offset_i][0] = sequence_id;
          batch.logits[offset_i] = true;
        }
        batch.n_tokens += n_target_tokens;
      }

      // n_tokens must be multiple of n_sequences.
      uint32_t
      normalize_n_tokens(uint32_t n_tokens, uint32_t n_sequences)
      {
        if (n_tokens < GGML_KQ_MASK_PAD) {
          n_tokens = GGML_KQ_MASK_PAD;
        }
        return ((n_tokens + (n_sequences - 1)) / n_sequences) * n_sequences;
      }

      bool
      vectorize_batch(grn_ctx *ctx,
                      llama_batch &batch,
                      uint32_t max_n_sequences)
      {
        if (llama_ctx_ && llama_n_ubatch(llama_ctx_) == max_n_tokens_ &&
            llama_n_seq_max(llama_ctx_) == max_n_sequences) {
          auto memory = llama_get_memory(llama_ctx_);
          if (memory) {
            llama_memory_clear(memory, true);
          }
        } else {
          if (llama_ctx_) {
            llama_free(llama_ctx_);
          }
          auto params = llama_context_default_params();
          params.n_ctx =
            llama_model_n_ctx_train(llama_model_) * max_n_sequences;
          params.embeddings = true;
          params.n_batch = normalize_n_tokens(batch.n_tokens, max_n_sequences);
          params.n_ubatch = params.n_batch;
          params.n_seq_max = max_n_sequences;
          params.pooling_type = pooling_type_;
          llama_ctx_ = llama_init_from_model(llama_model_, params);
        }

        if (has_encoder_ && !has_decoder_) {
          // encoder-only model
          if (llama_encode(llama_ctx_, batch) < 0) {
            GRN_LM_ERROR(
              GRN_UNKNOWN_ERROR,
              "[language-model-inferencer][vectorize-batch] failed to encode");
            return false;
          }
        } else if (!has_encoder_ && has_decoder_) {
          // decoder-only model
          if (llama_decode(llama_ctx_, batch) < 0) {
            GRN_LM_ERROR(
              GRN_UNKNOWN_ERROR,
              "[language-model-inferencer][vectorize-batch] failed to decode");
            return false;
          }
        } else {
          GRN_LM_ERROR(
            GRN_FUNCTION_NOT_IMPLEMENTED,
            "[language-model-inferencer][vectorize-batch] "
            "model that has both of encoder and docoder isn't supported yet");
          return false;
        }

        return true;
      }

      const float *
      get_embedding(grn_ctx *ctx,
                    llama_batch &batch,
                    llama_seq_id id,
                    const char *tag)
      {
        // pooling_type_ must not be LLAMA_POOLING_TYPE_NONE.
        auto raw_embedding = llama_get_embeddings_seq(llama_ctx_, id);
        if (!raw_embedding) {
          std::string message(tag);
          message += " failed to get embedding";
          GRN_LM_ERROR(GRN_UNKNOWN_ERROR, message.data());
          return nullptr;
        }
        return raw_embedding;
      }

      template <typename Output>
      void
      normalize_embedding(const float *raw_embedding, Output output)
      {
        // TODO: grn::distance::compute_l2_norm()
        float square_sum = 0.0;
        for (int32_t dimension = 0; dimension < n_dimensions_; ++dimension) {
          square_sum += raw_embedding[dimension] * raw_embedding[dimension];
        }
        auto magnitude = std::sqrt(square_sum);
        const float normalize = magnitude > 0.0 ? 1.0 / magnitude : 0.0f;
        for (int32_t dimension = 0; dimension < n_dimensions_; ++dimension) {
          auto normalized_value = raw_embedding[dimension] * normalize;
          output(dimension, normalized_value);
        }
      }

      bool
      pool_embedding(grn_ctx *ctx,
                     llama_batch &batch,
                     llama_seq_id id,
                     size_t n_pooled_embeddings,
                     const char *tag)
      {
        auto raw_embedding = get_embedding(ctx, batch, id, tag);
        if (!raw_embedding) {
          return false;
        }
        for (int32_t dimension = 0; dimension < n_dimensions_; ++dimension) {
          // Use average pooling to generate document embedding from
          // split embeddings. This is a SWEM-aver like approach.
          //
          // See also: Baseline Needs More Love: On Simple
          // Word-Embedding-Based Models and Associated Pooling
          // Mechanisms: https://arxiv.org/abs/1805.09843
          //
          // TODO: We can improve error of incremental/online average
          // computation. For example, merging averages, using Kahan
          // summation algorithm and so on.
          if (n_pooled_embeddings == 0) {
            pooling_buffer_[dimension] = raw_embedding[dimension];
          } else {
            pooling_buffer_[dimension] +=
              (raw_embedding[dimension] - pooling_buffer_[dimension]) /
              (n_pooled_embeddings + 1);
          }
        }
        return true;
      }

      bool
      store_embedding(grn_ctx *ctx,
                      llama_batch &batch,
                      llama_seq_id id,
                      grn_obj *output_vector,
                      const char *tag)
      {
        auto raw_embedding = get_embedding(ctx, batch, id, tag);
        if (!raw_embedding) {
          return false;
        }
        auto output = [&](int32_t dimension, float normalized_value) {
          GRN_FLOAT32_PUT(ctx, output_vector, normalized_value);
        };
        normalize_embedding(raw_embedding, output);
        return true;
      }
    };
#  endif

#endif
  public:
#ifdef GRN_WITH_LLAMA_CPP
    Impl(std::shared_ptr<LanguageModel> model,
         llama_model *model_raw,
         enum llama_pooling_type default_pooling_type)
      : model_(std::move(model)),
        llama_model_(model_raw),
        default_pooling_type_(default_pooling_type),
        vectorizer_(llama_model_, default_pooling_type_)
    {
    }

    ~Impl() {}
#endif

    void
    vectorize(grn_ctx *ctx, std::string_view text, grn_obj *output_vector)
    {
      const char *tag = "[language-model-inferencer][vectorize]";
#ifdef GRN_WITH_LLAMA_CPP
      vectorizer_.vectorize(ctx, text, output_vector, tag);
#else
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s llama.cpp isn't enabled", tag);
#endif
    }

    template <typename Output>
    void
    vectorize_in_batch(grn_ctx *ctx,
                       grn_table_cursor *cursor,
                       grn_obj *input_column,
                       Output output)
    {
      const char *tag = "[language-model-inferencer][vectorize-in-batch]";
#ifdef GRN_WITH_LLAMA_CPP
      vectorizer_.vectorize_in_batch(ctx, cursor, input_column, output, tag);
#else
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s llama.cpp isn't enabled", tag);
#endif
    }

  private:
#ifdef GRN_WITH_LLAMA_CPP
    std::shared_ptr<LanguageModel> model_;
    llama_model *llama_model_;
    enum llama_pooling_type default_pooling_type_;
    Vectorizer vectorizer_;
#endif
  };

  LanguageModelInferencer::LanguageModelInferencer(Impl *impl)
    : impl_(std::unique_ptr<Impl>(impl))
  {
  }

  LanguageModelInferencer::~LanguageModelInferencer() = default;

  void
  LanguageModelInferencer::vectorize(grn_ctx *ctx,
                                     std::string_view text,
                                     grn_obj *output_vector)
  {
    return impl_->vectorize(ctx, text, output_vector);
  }

  void
  LanguageModelInferencer::vectorize_in_batch(grn_ctx *ctx,
                                              grn_table_cursor *cursor,
                                              grn_obj *input_column,
                                              grn_obj *output)
  {
    if (grn_obj_is_column(ctx, output)) {
      return impl_->vectorize_in_batch(
        ctx,
        cursor,
        input_column,
        [&](grn_ctx *ctx, grn_id id, grn_obj *embedding) {
          grn_obj_set_value(ctx, output, id, embedding, GRN_OBJ_SET);
        });
    } else {
      return impl_->vectorize_in_batch(
        ctx,
        cursor,
        input_column,
        [&](grn_ctx *ctx, grn_id id, grn_obj *embedding) {
          grn_bulk_write(ctx,
                         output,
                         GRN_BULK_HEAD(embedding),
                         GRN_BULK_VSIZE(embedding));
        });
    }
  }

  std::unique_ptr<LanguageModelInferencer>
  LanguageModel::make_inferencer(grn_ctx *ctx)
  {
#ifdef GRN_WITH_LLAMA_CPP
    return std::make_unique<LanguageModelInferencer>(
      new LanguageModelInferencer::Impl(shared_from_this(),
                                        impl_->get_raw(),
                                        impl_->default_pooling_type()));
#else
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[language-model][make-inferencer] llama.cpp isn't enabled");
    return nullptr;
#endif
  }

  uint32_t
  LanguageModel::get_n_embedding_dimensions(grn_ctx *ctx)
  {
#ifdef GRN_WITH_LLAMA_CPP
    return llama_model_n_embd(impl_->get_raw());
#else
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[language-model][get-n-embedding-dimensions] llama.cpp isn't enabled");
    return 0;
#endif
  }

  class LanguageModelDownloader {
  public:
    LanguageModelDownloader(grn_ctx *ctx,
                            std::string_view hf_repo,
                            std::string_view tag)
      : ctx_(ctx),
        hf_repo_(hf_repo),
        tag_(tag),
        client_(grn_http_client_open(ctx_)),
        db_path_(grn_obj_path(ctx, grn_ctx_db(ctx_))),
        manifest_path_(build_manifest_path()),
        endpoint_url_("https://huggingface.co/"),
        model_path_()
    {
      // We need to use "llama-cpp" as User-Agent to get "ggufFile"
      // information by manifest API.
      grn_http_client_set_user_agent(ctx_, client_, "llama-cpp");
      // We may need to set "Accept:" explicitly in the future.
      // grn_http_client_add_header(ctx_, client_, "Accept: application/json");
    }

    ~LanguageModelDownloader() { grn_http_client_close(ctx_, client_); }

    bool
    download()
    {
      auto ctx = ctx_;
#ifdef GRN_WITH_SIMDJSON
      if (!ensure_manifest()) {
        return false;
      }

      auto manifest_result = simdjson::padded_string::load(manifest_path_);
      if (manifest_result.error() != simdjson::SUCCESS) {
        // TODO: Convert simdjson::error_code to grn_rc
        ERR(GRN_UNKNOWN_ERROR,
            "%s can't read manifest: <%s>: <%s>: <%s>",
            TAG,
            hf_repo_.data(),
            tag_.data(),
            simdjson::error_message(manifest_result.error()));
        return false;
      }
      auto manifest = std::move(manifest_result.value());
      simdjson::ondemand::parser parser;
      auto doc = parser.iterate(manifest);
      auto model_file_name_result = doc["ggufFile"]["rfilename"].get_string();
      if (model_file_name_result.error() != simdjson::SUCCESS) {
        // TODO: add support multi modal projects ("mmprojFile.rfilename")
        ERR(GRN_INVALID_ARGUMENT,
            "%s GGUF file can't be detected: <%s>: <%s>: <%.*s>",
            TAG,
            hf_repo_.data(),
            tag_.data(),
            static_cast<int>(manifest.size()),
            manifest.data());
        return false;
      }
      auto model_file_name = model_file_name_result.value();

      return ensure_model(model_file_name);
#else
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s simdjson isn't enabled", TAG);
      return false;
#endif
    }

    const std::string &
    model_path()
    {
      return model_path_;
    }

  private:
    static constexpr const char *TAG = "[language-model-downloader]";

    grn_ctx *ctx_;
    std::string_view hf_repo_;
    std::string_view tag_;
    grn_http_client *client_;
    std::string db_path_;
    std::string manifest_path_;
    std::string endpoint_url_;
    std::string model_path_;

    bool
    enable_cache()
    {
      return language_model::language_model_download_cache_dir[0] != '\0';
    }

    std::string
    build_base_path()
    {
      std::string safe_hf_repo = std::string(hf_repo_);
      std::replace(safe_hf_repo.begin(), safe_hf_repo.end(), '/', '_');
      return std::string("lm.") + safe_hf_repo + "_" + std::string(tag_);
    }

    std::string
    build_cache_path_prefix()
    {
      std::string cache_dir(language_model::language_model_download_cache_dir);
      if (cache_dir.back() == '/') {
        cache_dir.pop_back();
      }
      return cache_dir + "/" + build_base_path();
    }

    std::string
    build_path_prefix()
    {
      return db_path_ + "." + build_base_path();
    }

    std::string
    build_cache_manifest_path()
    {
      return build_cache_path_prefix() + ".manifest";
    }

    std::string
    build_manifest_path()
    {
      return build_path_prefix() + ".manifest";
    }

    std::string
    build_model_base_path(std::string_view model_file_name)
    {
      std::string safe_model_file_name = std::string(model_file_name);
      std::replace(safe_model_file_name.begin(),
                   safe_model_file_name.end(),
                   '/',
                   '_');
      return std::string("model.") + safe_model_file_name;
    }

    std::string
    build_cache_model_path(std::string_view model_file_name)
    {
      return build_cache_path_prefix() + "." +
             build_model_base_path(model_file_name);
    }

    std::string
    build_model_path(std::string_view model_file_name)
    {
      return build_path_prefix() + "." + build_model_base_path(model_file_name);
    }

    std::string
    build_manifest_url()
    {
      return endpoint_url_ + "v2/" + std::string(hf_repo_) + "/manifests/" +
             std::string(tag_);
    }

    std::string
    build_model_url(std::string_view model_file_name)
    {
      return endpoint_url_ + std::string(hf_repo_) + "/resolve/main/" +
             std::string(model_file_name);
    }

    bool
    ensure_manifest()
    {
      if (grn_path_exist(manifest_path_.data())) {
        return true;
      }

      std::string cache_manifest_path;
      if (enable_cache()) {
        cache_manifest_path = build_cache_manifest_path();
        if (grn_path_exist(cache_manifest_path.data()) &&
            grn_path_copy(ctx_,
                          cache_manifest_path.data(),
                          manifest_path_.data()) == GRN_SUCCESS) {
          return true;
        }
      }

      auto url = build_manifest_url();
      grn_http_client_set_url(ctx_, client_, url.data());
      if (grn_http_client_download(ctx_, client_) != GRN_SUCCESS) {
        return false;
      }
      auto manifest = grn_http_client_get_output(ctx_, client_);
      auto tmp_manifest_path = manifest_path_ + ".tmp";
      {
        std::ofstream tmp_manifest;
        tmp_manifest.open(tmp_manifest_path,
                          std::ios_base::binary | std::ios_base::trunc);
        if (!tmp_manifest) {
          auto ctx = ctx_;
          ERR(GRN_INVALID_ARGUMENT,
              "%s failed to save manifest: <%s>: <%s>: <%s>",
              TAG,
              url.data(),
              tmp_manifest_path.data(),
              std::strerror(errno));
          return false;
        }
        tmp_manifest.write(GRN_TEXT_VALUE(manifest), GRN_TEXT_LEN(manifest));
      }
      if (rename(tmp_manifest_path.data(), manifest_path_.data()) != 0) {
        auto ctx = ctx_;
        SERR("%s failed to rename downloaded manifest: <%s>: <%s> -> <%s>",
             TAG,
             url.data(),
             tmp_manifest_path.data(),
             manifest_path_.data());
        if (grn_path_exist(tmp_manifest_path.data())) {
          grn_unlink(tmp_manifest_path.data());
        }
        if (grn_path_exist(manifest_path_.data())) {
          grn_unlink(manifest_path_.data());
        }
        return false;
      }

      if (enable_cache()) {
        grn_path_copy(ctx_, manifest_path_.data(), cache_manifest_path.data());
      }

      return true;
    }

    bool
    ensure_model(std::string_view model_file_name)
    {
      model_path_ = build_model_path(model_file_name);
      if (grn_path_exist(model_path_.data())) {
        return true;
      }

      std::string cache_model_path;
      if (enable_cache()) {
        cache_model_path = build_cache_model_path(model_file_name);
        if (grn_path_exist(cache_model_path.data()) &&
            grn_path_copy(ctx_, cache_model_path.data(), model_path_.data()) ==
              GRN_SUCCESS) {
          return true;
        }
      }

      auto url = build_model_url(model_file_name);
      grn_http_client_set_url(ctx_, client_, url.data());
      auto tmp_model_path = model_path_ + ".tmp";
      grn_http_client_set_output_path(ctx_, client_, tmp_model_path.data());
      if (grn_http_client_download(ctx_, client_) != GRN_SUCCESS) {
        grn_io_remove_if_exist(ctx_, tmp_model_path.data());
        return false;
      }
      if (rename(tmp_model_path.data(), model_path_.data()) != 0) {
        auto ctx = ctx_;
        SERR("%s failed to rename downloaded manifest: <%s>: <%s> -> <%s>",
             TAG,
             url.data(),
             tmp_model_path.data(),
             manifest_path_.data());
        grn_io_remove_if_exist(ctx_, tmp_model_path.data());
        grn_io_remove_if_exist(ctx_, model_path_.data());
        return false;
      }

      if (enable_cache()) {
        grn_path_copy(ctx_, model_path_.data(), cache_model_path.data());
      }

      return true;
    }
  };
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
    const std::string_view hf_url_prefix(
      "hf:///"); // TODO: Add support for custom endpoint
    // TODO: We can use starts_with() with C++20.
    if (loader->loader.model_path.substr(0, hf_url_prefix.size()) ==
        hf_url_prefix) {
      auto hf_repo = loader->loader.model_path.substr(hf_url_prefix.size());
      auto tag_separator_position = hf_repo.find("#");
      std::string_view tag("latest");
      if (tag_separator_position != std::string_view::npos) {
        auto specified_tag = hf_repo.substr(tag_separator_position + 1);
        if (!specified_tag.empty()) {
          tag = specified_tag;
        }
      }
      grn::LanguageModelDownloader downloader(ctx, hf_repo, tag);
      if (!downloader.download()) {
        GRN_API_RETURN(ctx->rc);
      }
      loader->loader.model_path = downloader.model_path();
    } else {
      if (loader->loader.model_path[0] != '/') {
        std::string model_path =
          grn::language_model::system_language_models_dir();
        model_path += "/" + loader->loader.model_path + ".gguf";
        loader->loader.model_path = std::move(model_path);
      }
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

uint32_t
grn_language_model_get_n_embedding_dimensions(grn_ctx *ctx,
                                              grn_language_model *model)
{
  GRN_API_ENTER;
  uint32_t n_dimensions = model->model->get_n_embedding_dimensions(ctx);
  GRN_API_RETURN(n_dimensions);
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
    inferencer->inferencer->vectorize(ctx,
                                      std::string_view(text, text_length),
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
    grn_language_model_inferencer_vectorize_in_batch(ctx,
                                                     inferencer,
                                                     cursor,
                                                     input_column,
                                                     output_column);
    grn_table_cursor_close(ctx, cursor);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_language_model_inferencer_vectorize_in_batch(
  grn_ctx *ctx,
  grn_language_model_inferencer *inferencer,
  grn_table_cursor *cursor,
  grn_obj *input_column,
  grn_obj *output)
{
  GRN_API_ENTER;
  if (!inferencer) {
    ERR(GRN_INVALID_ARGUMENT,
        "[language-model-inferencer][vectorize-in-batch] "
        "inferencer must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }
  inferencer->inferencer->vectorize_in_batch(ctx, cursor, input_column, output);
  GRN_API_RETURN(ctx->rc);
}
}
