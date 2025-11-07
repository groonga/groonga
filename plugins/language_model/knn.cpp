/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG language_model_knn
#endif

#include <groonga.h>
#include <groonga/tokenizer.h>

#include <faiss/Clustering.h>
#include <faiss/IndexFlat.h>
#include <faiss/VectorTransform.h>

#define FAISS_VERSION_OR_LATER(major, minor, micro)                            \
  (FAISS_VERSION_MAJOR > (major) ||                                            \
   (FAISS_VERSION_MAJOR == (major) && FAISS_VERSION_MINOR > (minor)) ||        \
   (FAISS_VERSION_MAJOR == (major) && FAISS_VERSION_MINOR == (minor) &&        \
    FAISS_VERSION_MICRO >= (micro)))

#if FAISS_VERSION_OR_LATER(1, 7, 3)
#  define GRN_FAISS_HAVE_FLAT_CODES_DISTANCE_COMPUTER
#endif

#if !FAISS_VERSION_OR_LATER(1, 7, 4)
namespace faiss {
  using idx_t = Index::idx_t;
};
#endif

#if FAISS_VERSION_OR_LATER(1, 11, 0)
#  define GRN_FAISS_HAVE_RABITQ
#  define GRN_FAISS_HAVE_MAYBE_OWNED_VECTOR
#  include <faiss/impl/RaBitQuantizer.h>
#endif

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifndef _WIN32
#  include <unistd.h>
#endif

/* IVF based approximate nearest neighbor search implementation. */

namespace {
  const char *TAG = "[language-model][knn]";
  const int SEED = 29;

  enum class Quantizer {
    NONE,
    RABITQ,
  };

  struct Options {
    /* Input */
    std::string model_name;
    uint32_t n_clusters;
    Quantizer quantizer;
    std::string code_column_name;
    std::string passage_prefix;
    std::string query_prefix;

    /* Others are cached data for performance. */

    grn_language_model *model;
    grn_language_model_inferencer *inferencer;
    uint32_t n_dimensions;
    std::vector<float> centroids;
    std::vector<grn_id> centroid_index_to_record_id;
    faiss::IndexFlatIP centroid_searcher;

    /* For Quantization */
    grn_obj *code_column;
#ifdef GRN_FAISS_HAVE_RABITQ
    /* For RaBitQ */
    faiss::RandomRotationMatrix random_rotation_matrix; /* P in RaBitQ */
    faiss::RaBitQuantizer rabitq;
#endif
  };

  void
  options_init(grn_ctx *ctx, Options *options)
  {
    new (&options->model_name) std::string;
    new (&options->code_column_name) std::string;
    new (&options->passage_prefix) std::string;
    new (&options->query_prefix) std::string;
    options->n_clusters = 0;
#ifdef GRN_FAISS_HAVE_RABITQ
    options->quantizer = Quantizer::RABITQ;
#else
    options->quantizer = Quantizer::NONE;
#endif

    options->model = nullptr;
    options->inferencer = nullptr;
    options->n_dimensions = 0;
    new (&options->centroids) std::vector<float>;
    new (&options->centroid_index_to_record_id) std::vector<grn_id>;
    new (&options->centroid_searcher) faiss::IndexFlatIP;

    options->code_column = nullptr;
#ifdef GRN_FAISS_HAVE_RABITQ
    new (&options->random_rotation_matrix) faiss::RandomRotationMatrix;
    new (&options->rabitq) faiss::RaBitQuantizer;
#endif
  }

  void
  options_fin(grn_ctx *ctx, Options *options)
  {
    if (options->inferencer) {
      grn_language_model_inferencer_close(ctx, options->inferencer);
    }
    if (options->model) {
      grn_language_model_close(ctx, options->model);
    }
    options->model_name.~basic_string();

    // This may not work when this options is closed by grn_fin()
    // because code_column may be alread closed.
    // if (options->code_column) {
    //   grn_obj_unref(ctx, options->code_column);
    // }
    options->code_column_name.~basic_string();

    options->passage_prefix.~basic_string();
    options->query_prefix.~basic_string();

#ifdef GRN_FAISS_HAVE_RABITQ
    options->rabitq.~RaBitQuantizer();
    options->random_rotation_matrix.~RandomRotationMatrix();
#endif

    options->centroid_searcher.~IndexFlatIP();
    options->centroid_index_to_record_id.~vector();
    options->centroids.~vector();
  }

  const char *TOKENIZER_TAG = "[tokenizer][language-model]";
  struct Tokenizer {
    grn_obj *lexicon;
    grn_obj *source_table;
    Options *options;
    std::string_view query;
    grn_obj embedding;
    grn_obj transformed_embedding;
    grn_obj code;
  };

  Tokenizer *
  tokenizer_open(grn_ctx *ctx, Options *options, std::string_view query)
  {
    auto tokenizer =
      static_cast<Tokenizer *>(GRN_PLUGIN_CALLOC(ctx, sizeof(Tokenizer)));
    if (!tokenizer) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_NO_MEMORY_AVAILABLE,
                       "%s failed to allocate tokenizer",
                       TOKENIZER_TAG);
      return nullptr;
    }

    tokenizer->options = options;
    tokenizer->query = query;
    GRN_FLOAT32_INIT(&(tokenizer->embedding), GRN_OBJ_VECTOR);
    GRN_FLOAT32_INIT(&(tokenizer->transformed_embedding), GRN_OBJ_VECTOR);
    if (options->quantizer == Quantizer::NONE) {
      GRN_SHORT_BINARY_INIT(&(tokenizer->code), GRN_OBJ_DO_SHALLOW_COPY);
    } else {
      GRN_SHORT_BINARY_INIT(&(tokenizer->code), 0);
    }

    return tokenizer;
  }

  void
  tokenizer_close(grn_ctx *ctx, Tokenizer *tokenizer)
  {
    GRN_OBJ_FIN(ctx, &(tokenizer->embedding));
    GRN_OBJ_FIN(ctx, &(tokenizer->transformed_embedding));
    GRN_OBJ_FIN(ctx, &(tokenizer->code));
    GRN_PLUGIN_FREE(ctx, tokenizer);
  }

  struct OpenOptionsData {
    grn_obj *lexicon;
    grn_obj *source_table;
  };

  void
  close_options(grn_ctx *ctx, void *data)
  {
    Options *options = static_cast<Options *>(data);
    options_fin(ctx, options);
    GRN_PLUGIN_FREE(ctx, options);
  }

  void *
  open_options(grn_ctx *ctx,
               grn_obj *tokenizer,
               grn_obj *raw_options,
               void *user_data)
  {
    auto data = static_cast<OpenOptionsData *>(user_data);
    auto options =
      static_cast<Options *>(GRN_PLUGIN_CALLOC(ctx, sizeof(Options)));
    if (!options) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_NO_MEMORY_AVAILABLE,
                       "%s failed to allocate memory for options",
                       TOKENIZER_TAG);
      return nullptr;
    }

    auto set_string =
      [&](std::string &target, grn_obj *raw_options, unsigned int i) {
        const char *raw_value = nullptr;
        grn_id domain = GRN_ID_NIL;
        auto length = grn_vector_get_element(ctx,
                                             raw_options,
                                             i,
                                             &raw_value,
                                             nullptr,
                                             &domain);
        if (grn_type_id_is_text_family(ctx, domain)) {
          target = std::string(raw_value, length);
        }
      };

    options_init(ctx, options);
    GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length)
    {
      std::string_view name_raw(name, name_length);

      if (name_raw == "model") {
        set_string(options->model_name, raw_options, i);
      } else if (name_raw == "n_clusters") {
        options->n_clusters =
          grn_vector_get_element_uint32(ctx,
                                        raw_options,
                                        i,
                                        options->n_clusters);
      } else if (name_raw == "quantizer") {
        // TODO: RaBitQ and no quantization are only supported for now
      } else if (name_raw == "code_column") {
        set_string(options->code_column_name, raw_options, i);
      } else if (name_raw == "passage_prefix") {
        set_string(options->passage_prefix, raw_options, i);
      } else if (name_raw == "query_prefix") {
        set_string(options->query_prefix, raw_options, i);
      }
    }
    GRN_OPTION_VALUES_EACH_END();

    if (options->model_name.empty()) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s model isn't specified",
                       TOKENIZER_TAG);
      close_options(ctx, options);
      return nullptr;
    }

    if (options->code_column_name.empty()) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s code_column isn't specified",
                       TOKENIZER_TAG);
      close_options(ctx, options);
      return nullptr;
    }

    options->code_column = grn_obj_column(ctx,
                                          data->source_table,
                                          options->code_column_name.data(),
                                          options->code_column_name.size());
    if (!options->code_column) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s couldn't find code_column: <%s>",
                       TOKENIZER_TAG,
                       options->code_column_name.data());
      close_options(ctx, options);
      return nullptr;
    }

    grn_language_model_loader *loader = grn_language_model_loader_open(ctx);
    grn_language_model_loader_set_model(ctx,
                                        loader,
                                        options->model_name.data(),
                                        options->model_name.size());
    options->model = grn_language_model_loader_load(ctx, loader);
    grn_language_model_loader_close(ctx, loader);
    if (!options->model) {
      std::string message(ctx->errbuf);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s couldn't load model: <%s>: %s",
                       TOKENIZER_TAG,
                       options->model_name.data(),
                       message.data());
      close_options(ctx, options);
      return nullptr;
    }

    options->inferencer =
      grn_language_model_open_inferencer(ctx, options->model);
    if (!options->inferencer) {
      std::string message(ctx->errbuf);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s failed to create model inferencer: %s",
                       TOKENIZER_TAG,
                       message.data());
      close_options(ctx, options);
      return nullptr;
    }
    if (!options->passage_prefix.empty()) {
      grn_language_model_inferencer_set_input_column_value_prefix(
        ctx,
        options->inferencer,
        options->passage_prefix.data(),
        static_cast<int64_t>(options->passage_prefix.size()));
    }

    options->n_dimensions =
      grn_language_model_get_n_embedding_dimensions(ctx, options->model);
    options->centroid_searcher.d = options->n_dimensions;
    options->centroid_searcher.metric_type =
      faiss::MetricType::METRIC_INNER_PRODUCT;
    options->centroid_searcher.code_size =
      sizeof(float) * options->n_dimensions;
    options->n_clusters = grn_table_size(ctx, data->lexicon);
    if (options->n_clusters > 0) {
      options->centroids.resize(options->n_dimensions * options->n_clusters);
      options->centroid_index_to_record_id.resize(options->n_clusters);
      size_t i = 0;
      GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                                 data->lexicon,
                                 cursor,
                                 id,
                                 GRN_CURSOR_BY_ID)
      {
        void *key;
        auto key_size = grn_table_cursor_get_key(ctx, cursor, &key);
        memcpy(options->centroids.data() + (options->n_dimensions * i),
               key,
               key_size);
        options->centroid_index_to_record_id[i] = id;
        ++i;
      }
      GRN_TABLE_EACH_END(ctx, cursor);
    }
#ifdef GRN_FAISS_HAVE_MAYBE_OWNED_VECTOR
    options->centroid_searcher.codes =
      faiss::MaybeOwnedVector<uint8_t>::create_view(options->centroids.data(),
                                                    sizeof(float) *
                                                      options->centroids.size(),
                                                    nullptr);
#else
    options->centroid_searcher.codes.resize(sizeof(float) *
                                            options->centroids.size());
    memcpy(options->centroid_searcher.codes.data(),
           options->centroids.data(),
           options->centroid_searcher.codes.size());
#endif
    options->centroid_searcher.ntotal =
      options->centroids.size() / options->n_dimensions;

#ifdef GRN_FAISS_HAVE_RABITQ
    options->random_rotation_matrix.d_in = options->n_dimensions;
    options->random_rotation_matrix.d_out = options->n_dimensions;
    options->random_rotation_matrix.init(SEED);

    // We can use inner product here because we normalize generated
    // embeddings.
    options->rabitq =
      faiss::RaBitQuantizer(options->n_dimensions,
                            faiss::MetricType::METRIC_INNER_PRODUCT);
#endif

    return options;
  }

  class Builder {
  public:
    Builder(grn_ctx *ctx, grn_tokenizer_build_data *data)
      : ctx_(ctx),
        data_(data),
        lexicon_(grn_tokenizer_build_data_get_lexicon(ctx_, data_)),
        source_table_(grn_tokenizer_build_data_get_source_table(ctx_, data_)),
        n_source_records_(grn_table_size(ctx, source_table_)),
        source_columns_(
          grn_tokenizer_build_data_get_source_columns(ctx_, data_)),
        source_column_(GRN_PTR_VALUE_AT(source_columns_, 0)),
        index_column_(grn_tokenizer_build_data_get_index_column(ctx_, data_)),
        options_(nullptr),
        embeddings_path_(),
        embeddings_map_(nullptr),
        embeddings_(),
        embeddings_raw_(),
        transformed_embeddings_raw_(nullptr)
    {
      GRN_FLOAT32_INIT(&embeddings_, GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY);
    }

    ~Builder()
    {
      GRN_OBJ_FIN(ctx_, &embeddings_);
      if (embeddings_map_) {
        grn_memory_map_close(ctx_, embeddings_map_);
        grn_unlink(embeddings_path_.data());
      }
    }

    bool
    build()
    {
      if (!prepare_options()) {
        return false;
      }
      faiss::IndexFlatIP index(options_->n_dimensions);
      if (!build_clusters(index)) {
        return false;
      }
      if (!build_inverted_index(index)) {
        return false;
      }
      return true;
    }

  private:
    grn_ctx *ctx_;
    grn_tokenizer_build_data *data_;
    grn_obj *lexicon_;
    grn_obj *source_table_;
    uint32_t n_source_records_;
    grn_obj *source_columns_;
    grn_obj *source_column_;
    grn_obj *index_column_;
    Options *options_;
    std::string embeddings_path_;
    grn_memory_map *embeddings_map_;
    grn_obj embeddings_;
    float *embeddings_raw_;
    float *transformed_embeddings_raw_;

    bool
    prepare_options()
    {
      OpenOptionsData open_options_data;
      open_options_data.lexicon = lexicon_;
      open_options_data.source_table = source_table_;
      options_ = static_cast<Options *>(
        grn_table_cache_default_tokenizer_options(ctx_,
                                                  lexicon_,
                                                  open_options,
                                                  close_options,
                                                  &open_options_data));
      return ctx_->rc == GRN_SUCCESS;
    }

    bool
    build_clusters(faiss::IndexFlatIP &index)
    {
      if (options_->n_clusters == 0) {
        // Heuristic default
        if (n_source_records_ >= 1000000) { // 1M records
          options_->n_clusters =
            static_cast<uint32_t>(std::sqrt(n_source_records_));
        } else {
          options_->n_clusters = (n_source_records_ / 1000) + 1;
        }
      }

      auto cursor = grn_table_cursor_open(ctx_,
                                          source_table_,
                                          nullptr,
                                          0,
                                          nullptr,
                                          0,
                                          0,
                                          -1,
                                          GRN_CURSOR_BY_ID);
      if (!cursor) {
        if (ctx_->rc == GRN_SUCCESS) {
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_UNKNOWN_ERROR,
                           "%s failed to open source table cursor",
                           TOKENIZER_TAG);
        } else {
          std::string message(ctx_->errbuf);
          GRN_PLUGIN_ERROR(ctx_,
                           ctx_->rc,
                           "%s failed to open source table cursor: %s",
                           TOKENIZER_TAG,
                           message.data());
        }
        return false;
      }

      grn_tokenizer_build_data_start_vectorize(ctx_, data_, n_source_records_);
      embeddings_path_ = grn_obj_path(ctx_, index_column_);
      embeddings_path_ += ".embeddings";
      size_t embeddings_size =
        sizeof(float) * options_->n_dimensions * n_source_records_;
      embeddings_map_ =
        grn_memory_map_open(ctx_,
                            embeddings_path_.data(),
                            GRN_MEMORY_MAP_READ | GRN_MEMORY_MAP_WRITE,
                            0,
                            // The former part is for raw embeddings and
                            // the latter part is for transformed embeddings.
                            embeddings_size * 2);
      embeddings_raw_ =
        static_cast<float *>(grn_memory_map_get_address(ctx_, embeddings_map_));
      transformed_embeddings_raw_ =
        embeddings_raw_ + (embeddings_size / sizeof(float));
      GRN_BINARY_SET_REF(&embeddings_, embeddings_raw_, embeddings_size);
      GRN_BULK_REWIND(&embeddings_);
      {
        bool need_progress = grn_ctx_get_progress_callback(ctx_);
        struct UserData {
          grn_tokenizer_build_data *data;
        } user_data;
        user_data.data = data_;
        auto progress_callback =
          [](grn_ctx *ctx, grn_progress *progress, void *user_data) {
            auto user_data_ = static_cast<UserData *>(user_data);
            auto n_processed_records =
              grn_progress_language_model_inferencer_get_n_processed_records(
                ctx,
                progress);
            grn_tokenizer_build_data_processed_n_records(ctx,
                                                         user_data_->data,
                                                         n_processed_records);
          };
        if (need_progress) {
          grn_language_model_inferencer_set_progress_callback(
            ctx_,
            options_->inferencer,
            progress_callback,
            &user_data);
        }
        grn_language_model_inferencer_vectorize_in_batch(ctx_,
                                                         options_->inferencer,
                                                         cursor,
                                                         source_column_,
                                                         &embeddings_);
        if (need_progress) {
          grn_language_model_inferencer_set_progress_callback(
            ctx_,
            options_->inferencer,
            nullptr,
            nullptr);
        }
      }
      grn_table_cursor_close(ctx_, cursor);

      if (GRN_BULK_VSIZE(&embeddings_) != embeddings_size) {
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_UNKNOWN_ERROR,
                         "%s failed to generate "
                         "float[%u] * %u (%" PRIu64 " bytes) embeddings: "
                         "%" PRIu64 " bytes are only generated",
                         TOKENIZER_TAG,
                         options_->n_dimensions,
                         n_source_records_,
                         static_cast<uint64_t>(embeddings_size),
                         static_cast<uint64_t>(GRN_BULK_VSIZE(&embeddings_)));
        return false;
      }

      grn_tokenizer_build_data_start_cluster(ctx_, data_, n_source_records_);
      size_t n_centroids = options_->n_clusters;
      faiss::Clustering clustering(options_->n_dimensions, n_centroids);
      if (options_->quantizer == Quantizer::RABITQ) {
#ifdef GRN_FAISS_HAVE_RABITQ
        options_->random_rotation_matrix.apply_noalloc(
          n_source_records_,
          embeddings_raw_,
          transformed_embeddings_raw_);
        clustering.train(n_source_records_, transformed_embeddings_raw_, index);
#endif
      } else {
        clustering.train(n_source_records_, embeddings_raw_, index);
      }
      options_->centroid_index_to_record_id.reserve(n_centroids);
      for (size_t i = 0; i < n_centroids; ++i) {
        grn_id id = grn_table_add(ctx_,
                                  lexicon_,
                                  clustering.centroids.data() +
                                    (options_->n_dimensions * i),
                                  sizeof(float) * options_->n_dimensions,
                                  nullptr);
        options_->centroid_index_to_record_id.push_back(id);
      }
      options_->centroids = std::move(clustering.centroids);
#ifdef GRN_FAISS_HAVE_MAYBE_OWNED_VECTOR
      options_->centroid_searcher.codes =
        faiss::MaybeOwnedVector<uint8_t>::create_view(
          options_->centroids.data(),
          sizeof(float) * options_->centroids.size(),
          nullptr);
#else
      options_->centroid_searcher.codes.resize(sizeof(float) *
                                               options_->centroids.size());
      memcpy(options_->centroid_searcher.codes.data(),
             options_->centroids.data(),
             options_->centroid_searcher.codes.size());
#endif
      options_->centroid_searcher.ntotal = n_centroids;
      grn_tokenizer_build_data_processed_n_records(ctx_, data_, n_centroids);

      return true;
    }

    bool
    build_inverted_index(faiss::IndexFlatIP &index)
    {
      grn_obj tokens;
      GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, grn_obj_id(ctx_, lexicon_));
      grn_obj code;
      if (options_->quantizer == Quantizer::NONE) {
        GRN_SHORT_BINARY_INIT(&code, GRN_OBJ_DO_SHALLOW_COPY);
      } else if (options_->quantizer == Quantizer::RABITQ) {
#ifdef GRN_FAISS_HAVE_RABITQ
        GRN_SHORT_BINARY_INIT(&code, 0);
        grn_bulk_space(ctx_, &code, options_->rabitq.code_size);
#endif
      }
      grn_tokenizer_build_data_start_load(ctx_, data_, n_source_records_);
      size_t i = 0;
      GRN_TABLE_EACH_BEGIN_FLAGS(ctx_,
                                 source_table_,
                                 cursor,
                                 id,
                                 GRN_CURSOR_BY_ID)
      {
        grn_tokenizer_build_data_start_record(ctx_, data_, id);
        grn_tokenizer_build_data_start_section(ctx_, data_, 1);

        GRN_BULK_REWIND(&tokens);
        const float *target_embedding = nullptr;
        if (options_->quantizer == Quantizer::RABITQ) {
          target_embedding =
            transformed_embeddings_raw_ + (options_->n_dimensions * i);
        } else {
          target_embedding = embeddings_raw_ + (options_->n_dimensions * i);
        }
        float distances[1];
        faiss::idx_t indexes[1];
        index.search(1, target_embedding, 1, distances, indexes);
        if (options_->quantizer == Quantizer::NONE) {
          GRN_BINARY_SET_REF(&code,
                             target_embedding,
                             sizeof(float) * options_->n_dimensions);
        } else if (options_->quantizer == Quantizer::RABITQ) {
#ifdef GRN_FAISS_HAVE_RABITQ
          const float *centroid =
            options_->centroids.data() + (options_->n_dimensions * indexes[0]);
          options_->rabitq.compute_codes_core(
            target_embedding,
            reinterpret_cast<uint8_t *>(GRN_BULK_HEAD(&code)),
            1,
            centroid);
#endif
        }
        grn_obj_set_value(ctx_, options_->code_column, id, &code, GRN_OBJ_SET);
        grn_id centroid_id = options_->centroid_index_to_record_id[indexes[0]];
        grn_uvector_add_element_record(ctx_, &tokens, centroid_id, 0.0);
        grn_tokenizer_build_data_append_tokens(ctx_, data_, &tokens);

        grn_tokenizer_build_data_finish_section(ctx_, data_);
        grn_tokenizer_build_data_finish_record(ctx_, data_);

        ++i;
      }
      GRN_TABLE_EACH_END(ctx_, cursor);
      GRN_OBJ_FIN(ctx_, &code);
      GRN_OBJ_FIN(ctx_, &tokens);

      return true;
    }
  };

  grn_rc
  build(grn_ctx *ctx, grn_tokenizer_build_data *data)
  {
    Builder builder(ctx, data);
    builder.build();
    return ctx->rc;
  }

  void *
  init(grn_ctx *ctx, grn_tokenizer_query *query)
  {
    size_t query_size;
    grn_id query_domain;
    const char *raw_query =
      grn_tokenizer_query_get_data(ctx, query, &query_size, &query_domain);
    if (!grn_type_id_is_text_family(ctx, query_domain)) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s query must be text: %s(%u)",
                       TAG,
                       grn_type_id_to_string_builtin(ctx, query_domain),
                       query_domain);
      return nullptr;
    }

    auto lexicon = grn_tokenizer_query_get_lexicon(ctx, query);
    auto source_column = grn_tokenizer_query_get_source_column(ctx, query);
    auto source_table = grn_ctx_at(ctx, source_column->header.domain);
    OpenOptionsData open_options_data;
    open_options_data.lexicon = lexicon;
    open_options_data.source_table = source_table;
    auto options = static_cast<Options *>(
      grn_table_cache_default_tokenizer_options(ctx,
                                                lexicon,
                                                open_options,
                                                close_options,
                                                &open_options_data));
    grn_obj_unref(ctx, source_table);
    if (ctx->rc != GRN_SUCCESS) {
      return nullptr;
    }

    auto tokenizer =
      tokenizer_open(ctx, options, std::string_view(raw_query, query_size));
    if (!tokenizer) {
      return nullptr;
    }

    return tokenizer;
  }

  void
  next(grn_ctx *ctx,
       grn_tokenizer_query *query,
       grn_token *token,
       void *user_data)
  {
    auto tokenizer = static_cast<Tokenizer *>(user_data);

    if (tokenizer->options->passage_prefix.empty()) {
      grn_language_model_inferencer_vectorize(ctx,
                                              tokenizer->options->inferencer,
                                              tokenizer->query.data(),
                                              tokenizer->query.size(),
                                              &(tokenizer->embedding));
    } else {
      auto prefixed_query = tokenizer->options->passage_prefix;
      prefixed_query.append(tokenizer->query);
      grn_language_model_inferencer_vectorize(ctx,
                                              tokenizer->options->inferencer,
                                              prefixed_query.data(),
                                              prefixed_query.size(),
                                              &(tokenizer->embedding));
    }
    if (ctx->rc != GRN_SUCCESS) {
      GRN_PLUGIN_ERROR(ctx,
                       ctx->rc,
                       "%s failed to vectorize: <%.*s>",
                       TAG,
                       static_cast<int>(tokenizer->query.size()),
                       tokenizer->query.data());
      return;
    }

    auto embedding =
      reinterpret_cast<const float *>(GRN_BULK_HEAD(&(tokenizer->embedding)));
    const float *target_embedding = nullptr;
    if (tokenizer->options->quantizer == Quantizer::RABITQ) {
#ifdef GRN_FAISS_HAVE_RABITQ
      grn_bulk_space(ctx,
                     &(tokenizer->transformed_embedding),
                     GRN_BULK_VSIZE(&(tokenizer->embedding)));
      auto transformed_embedding = reinterpret_cast<float *>(
        GRN_BULK_HEAD(&(tokenizer->transformed_embedding)));
      tokenizer->options->random_rotation_matrix.apply_noalloc(
        1,
        embedding,
        transformed_embedding);
      target_embedding = transformed_embedding;
#endif
    } else {
      target_embedding = embedding;
    }
    size_t k = 1;
    float distances[1];
    faiss::idx_t indexes[1];
    tokenizer->options->centroid_searcher.search(1,
                                                 target_embedding,
                                                 k,
                                                 distances,
                                                 indexes);
    const float *centroid = tokenizer->options->centroids.data() +
                            (tokenizer->options->n_dimensions * indexes[0]);
    grn_token_set_data(ctx,
                       token,
                       reinterpret_cast<const char *>(centroid),
                       sizeof(float) * tokenizer->options->n_dimensions);
    grn_token_set_domain(ctx, token, GRN_DB_SHORT_BINARY);
    grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
    if (grn_tokenizer_query_get_mode(ctx, query) == GRN_TOKENIZE_ADD) {
      if (tokenizer->options->quantizer == Quantizer::NONE) {
        GRN_BINARY_SET_REF(&(tokenizer->code),
                           target_embedding,
                           sizeof(float) * tokenizer->options->n_dimensions);
      } else if (tokenizer->options->quantizer == Quantizer::RABITQ) {
#ifdef GRN_FAISS_HAVE_RABITQ
        grn_bulk_space(ctx,
                       &(tokenizer->code),
                       tokenizer->options->rabitq.code_size);
        tokenizer->options->rabitq.compute_codes_core(
          target_embedding,
          reinterpret_cast<uint8_t *>(GRN_BULK_HEAD(&(tokenizer->code))),
          1,
          centroid);
#endif
      }
      grn_id source_id = grn_tokenizer_query_get_source_id(ctx, query);
      grn_obj_set_value(ctx,
                        tokenizer->options->code_column,
                        source_id,
                        &(tokenizer->code),
                        GRN_OBJ_SET);
    }
  }

  void
  fin(grn_ctx *ctx, void *user_data)
  {
    auto tokenizer = static_cast<Tokenizer *>(user_data);

    if (!tokenizer) {
      return;
    }

    tokenizer_close(ctx, tokenizer);
  }

  struct Candidate {
    grn_id record_id = GRN_ID_NIL;
    float similarity = 0.0;

    Candidate(grn_id record_id, float similarity)
      : record_id(record_id),
        similarity(similarity)
    {
    }
  };

  class Searcher {
  public:
    Searcher(grn_ctx *ctx,
             grn_obj *table,
             grn_obj *index,
             std::string_view query,
             uint32_t n_probes,
             uint32_t k)
      : ctx_(ctx),
        table_(table),
        index_(index),
        query_(query),
        n_probes_(n_probes),
        k_(k),
        query_embedding_(),
        transformed_query_embedding_(),
        code_()
    {
      GRN_FLOAT32_INIT(&query_embedding_, GRN_OBJ_VECTOR);
      GRN_FLOAT32_INIT(&transformed_query_embedding_, GRN_OBJ_VECTOR);
      GRN_VOID_INIT(&code_);
    }

    ~Searcher()
    {
      GRN_OBJ_FIN(ctx_, &query_embedding_);
      GRN_OBJ_FIN(ctx_, &transformed_query_embedding_);
      GRN_OBJ_FIN(ctx_, &code_);
    }

    template <typename Filter>
    std::optional<std::vector<Candidate>>
    search(Filter filter, bool ascending)
    {
      auto lexicon = grn_ctx_at(ctx_, index_->header.domain);
      OpenOptionsData open_options_data;
      open_options_data.lexicon = lexicon;
      open_options_data.source_table = table_;
      auto options = static_cast<Options *>(
        grn_table_cache_default_tokenizer_options(ctx_,
                                                  lexicon,
                                                  open_options,
                                                  close_options,
                                                  &open_options_data));

      if (options->query_prefix.empty()) {
        grn_language_model_inferencer_vectorize(ctx_,
                                                options->inferencer,
                                                query_.data(),
                                                query_.size(),
                                                &query_embedding_);
      } else {
        auto prefixed_query = options->query_prefix;
        prefixed_query.append(query_);
        grn_language_model_inferencer_vectorize(ctx_,
                                                options->inferencer,
                                                prefixed_query.data(),
                                                prefixed_query.size(),
                                                &query_embedding_);
      }
      auto raw_query_embedding =
        reinterpret_cast<const float *>(GRN_BULK_HEAD(&query_embedding_));
      const float *raw_target_embedding = nullptr;
      if (options->quantizer == Quantizer::RABITQ) {
#ifdef GRN_FAISS_HAVE_RABITQ
        grn_bulk_space(ctx_,
                       &transformed_query_embedding_,
                       GRN_BULK_VSIZE(&query_embedding_));
        auto raw_transformed_query_embedding = reinterpret_cast<float *>(
          GRN_BULK_HEAD(&transformed_query_embedding_));
        options->random_rotation_matrix.apply_noalloc(
          1,
          raw_query_embedding,
          raw_transformed_query_embedding);
        raw_target_embedding = raw_transformed_query_embedding;
#endif
      } else {
        raw_target_embedding = raw_query_embedding;
      }

      auto n_probes =
        std::min(n_probes_,
                 static_cast<uint32_t>(options->centroid_searcher.ntotal));
      std::vector<float> centroid_distances(n_probes);
      std::vector<faiss::idx_t> centroid_indexes(n_probes, -1);
      options->centroid_searcher.search(1,
                                        raw_target_embedding,
                                        n_probes,
                                        centroid_distances.data(),
                                        centroid_indexes.data());

      std::vector<Candidate> candidates;
      for (auto centroid_index : centroid_indexes) {
        if (centroid_index == -1) {
          break;
        }

        auto lexicon_record_id =
          options->centroid_index_to_record_id[centroid_index];
        auto ii = reinterpret_cast<grn_ii *>(index_);
        auto cursor = grn_ii_cursor_open(ctx_,
                                         ii,
                                         lexicon_record_id,
                                         GRN_ID_NIL,
                                         GRN_ID_MAX,
                                         0,
                                         0);
        if (!cursor) {
          continue;
        }

        // TODO: Bulk similarity computation for performance.
#ifdef GRN_FAISS_HAVE_FLAT_CODES_DISTANCE_COMPUTER
        faiss::FlatCodesDistanceComputer *distance_computer = nullptr;
        if (options->quantizer == Quantizer::NONE) {
          distance_computer =
            options->centroid_searcher.get_FlatCodesDistanceComputer();
        } else if (options->quantizer == Quantizer::RABITQ) {
#  ifdef GRN_FAISS_HAVE_RABITQ
          uint8_t n_scalar_quantization_bits = 8;
          const float *centroid = options->centroids.data() +
                                  (options->n_dimensions * centroid_index);
          distance_computer =
            options->rabitq.get_distance_computer(n_scalar_quantization_bits,
                                                  centroid);
#  endif
        }
        distance_computer->set_query(raw_target_embedding);
#else
        grn_obj target_embedding;
        GRN_FLOAT32_INIT(&target_embedding,
                         GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET_REF(&target_embedding,
                         raw_target_embedding,
                         sizeof(float) * options->n_dimensions);
        grn_obj candidate_embedding;
        GRN_FLOAT32_INIT(&candidate_embedding,
                         GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY);
#endif

        while (true) {
          auto posting = grn_ii_cursor_next(ctx_, cursor);
          if (!posting) {
            break;
          }
          auto source_record_id = posting->rid;

          if (!filter(source_record_id)) {
            continue;
          }

          GRN_BULK_REWIND(&code_);
          grn_obj_get_value(ctx_,
                            options->code_column,
                            source_record_id,
                            &code_);
#ifdef GRN_FAISS_HAVE_FLAT_CODES_DISTANCE_COMPUTER
          auto similarity = distance_computer->distance_to_code(
            reinterpret_cast<const uint8_t *>(GRN_BULK_HEAD(&code_)));
#else
          GRN_TEXT_SET_REF(&candidate_embedding,
                           GRN_BULK_HEAD(&code_),
                           GRN_BULK_VSIZE(&code_));
          auto similarity =
            1 - grn_distance_inner_product(ctx_,
                                           &target_embedding,
                                           &candidate_embedding);
#endif
          candidates.emplace_back(source_record_id, similarity);
        }
#ifdef GRN_FAISS_HAVE_FLAT_CODES_DISTANCE_COMPUTER
        delete distance_computer;
#else
        GRN_OBJ_FIN(ctx_, &target_embedding);
        GRN_OBJ_FIN(ctx_, &candidate_embedding);
#endif
        grn_ii_cursor_close(ctx_, cursor);
      }

      auto k = std::min(k_, static_cast<uint32_t>(candidates.size()));
      if (ascending) {
        std::partial_sort(candidates.begin(),
                          candidates.begin() + k,
                          candidates.end(),
                          [](const Candidate &a, const Candidate &b) {
                            return b.similarity > a.similarity;
                          });
      } else {
        std::partial_sort(candidates.begin(),
                          candidates.begin() + k,
                          candidates.end(),
                          [](const Candidate &a, const Candidate &b) {
                            return a.similarity > b.similarity;
                          });
      }
      return candidates;
    }

  private:
    grn_ctx *ctx_;
    grn_obj *table_;
    grn_obj *index_;
    std::string_view query_;
    uint32_t n_probes_;
    uint32_t k_;
    grn_obj query_embedding_;
    grn_obj transformed_query_embedding_;
    grn_obj code_;
  };

  grn_rc
  language_model_knn_selector(grn_ctx *ctx,
                              grn_obj *table,
                              grn_obj *index,
                              int n_args,
                              grn_obj **args,
                              grn_obj *res,
                              grn_operator op)
  {
    const char *tag = "language_model_knn():";

    if (!(n_args == 3 || n_args == 4)) {
      /* args[0] is function. */
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s wrong number of arguments (%d for 2..3)",
                       tag,
                       n_args - 1);
      return ctx->rc;
    }

    grn_obj *query = args[2];
    uint32_t n_probes = 10;
    uint32_t k = 10;
    if (n_args == 4 && args[3]->header.type == GRN_TABLE_HASH_KEY) {
      grn_rc rc = grn_proc_options_parse(ctx,
                                         args[3],
                                         tag,
                                         "k",
                                         GRN_PROC_OPTION_VALUE_UINT32,
                                         &k,
                                         NULL);
      if (rc != GRN_SUCCESS) {
        return ctx->rc;
      }
      if (k == 0) {
        return GRN_SUCCESS;
      }
    }

    if (GRN_TEXT_LEN(query) == 0) {
      grn_ii_resolve_sel_and(ctx, reinterpret_cast<grn_hash *>(res), op);
      return ctx->rc;
    }

    Searcher searcher(
      ctx,
      table,
      index,
      std::string_view{GRN_TEXT_VALUE(query), GRN_TEXT_LEN(query)},
      n_probes,
      k);
    const bool ascending = false;
    std::optional<std::vector<Candidate>> maybe_candidates;
    if (op == GRN_OP_AND) {
      maybe_candidates = searcher.search(
        [&](grn_id id) {
          return grn_table_get(ctx, res, &id, sizeof(grn_id)) != GRN_ID_NIL;
        },
        ascending);
    } else {
      maybe_candidates =
        searcher.search([](grn_id id) { return true; }, ascending);
    }
    if (!maybe_candidates) {
      return ctx->rc;
    }

    auto candidates = *maybe_candidates;
    k = std::min(k, static_cast<uint32_t>(candidates.size()));
    auto posting = grn_posting_open(ctx);
    posting->sid = 0;
    posting->pos = 0;
    for (size_t i = 0; i < k; ++i) {
      const auto &candidate = candidates[i];
      posting->rid = candidate.record_id;
      grn_posting_set_weight_float(ctx, posting, candidate.similarity);
      grn_result_set_add_record(ctx,
                                reinterpret_cast<grn_hash *>(res),
                                posting,
                                op);
    }
    grn_ii_resolve_sel_and(ctx, reinterpret_cast<grn_hash *>(res), op);
    grn_posting_close(ctx, posting);

    return ctx->rc;
  }

  grn_rc
  language_model_knn_sorter(grn_ctx *ctx, grn_sorter_data *data)
  {
    const char *tag = "language_model_knn():";

    size_t n_args;
    grn_obj **args = grn_sorter_data_get_args(ctx, data, &n_args);
    if (!(n_args == 2 || n_args == 3)) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s wrong number of arguments (%u for 2..3)",
                       tag,
                       static_cast<uint32_t>(n_args));
      return ctx->rc;
    }

    grn_obj *table = grn_sorter_data_get_table(ctx, data);
    size_t offset = grn_sorter_data_get_offset(ctx, data);
    size_t limit = grn_sorter_data_get_limit(ctx, data);
    grn_obj *result = grn_sorter_data_get_result(ctx, data);
    bool ascending = grn_sorter_data_is_ascending(ctx, data);

    grn_obj *target = args[0];
    grn_obj *query = args[1];
    uint32_t n_probes = 10;
    uint32_t k = offset + limit;
    if (n_args == 3 && args[2]->header.type == GRN_TABLE_HASH_KEY) {
      // TODO: Parse options by grn_proc_options_parse()
    }

    if (grn_obj_is_column(ctx, target)) {
      grn_index_datum index_data;
      unsigned int n_indexes =
        grn_column_find_index_data(ctx, target, GRN_OP_SIMILAR, &index_data, 1);
      if (n_indexes == 0) {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, target);
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s no index: <%.*s>",
                         tag,
                         static_cast<int>(GRN_TEXT_LEN(&inspected)),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        return ctx->rc;
      }

      Searcher searcher(
        ctx,
        table,
        index_data.index,
        std::string_view(GRN_TEXT_VALUE(query), GRN_TEXT_LEN(query)),
        n_probes,
        k);
      auto maybe_candidates =
        searcher.search([](grn_id id) { return true; }, ascending);
      if (!maybe_candidates) return ctx->rc;

      auto candidates = *maybe_candidates;
      for (size_t i = offset; i < k; ++i) {
        const auto &candidate = candidates[i];
        void *value;
        auto id =
          grn_array_add(ctx, reinterpret_cast<grn_array *>(result), &value);
        if (id == GRN_ID_NIL) {
          break;
        }
        auto sorted_id = static_cast<grn_id *>(value);
        *sorted_id = candidate.record_id;
      }
    } else if (grn_obj_is_accessor(ctx, target)) {
      /* Filtered result set. */
      std::vector<grn_obj *> accessor_stack;
      grn_obj *leaf_target = target;
      while (true) {
        grn_obj *next_target = grn_accessor_get_next(ctx, leaf_target);
        if (!next_target) {
          leaf_target = grn_accessor_get_obj(ctx, leaf_target);
          break;
        }
        accessor_stack.push_back(leaf_target);
        leaf_target = next_target;
      }

      grn_index_datum index_data;
      unsigned int n_indexes = grn_column_find_index_data(ctx,
                                                          leaf_target,
                                                          GRN_OP_SIMILAR,
                                                          &index_data,
                                                          1);
      if (n_indexes == 0) {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, target);
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s no index: <%.*s>",
                         tag,
                         static_cast<int>(GRN_TEXT_LEN(&inspected)),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        return ctx->rc;
      }

      auto resolve_id = [&](grn_id id) -> grn_id {
        for (auto i = accessor_stack.size(); i > 0; --i) {
          auto accessor = accessor_stack[i - 1];
          auto obj = grn_accessor_get_obj(ctx, accessor);
          auto next_id = grn_table_get(ctx, obj, &id, sizeof(grn_id));
          if (next_id == GRN_ID_NIL) {
            return GRN_ID_NIL;
          }
          id = next_id;
        }
        return id;
      };

      grn_obj *leaf_table = grn_ctx_at(ctx, leaf_target->header.domain);
      Searcher searcher(
        ctx,
        leaf_table,
        index_data.index,
        std::string_view(GRN_TEXT_VALUE(query), GRN_TEXT_LEN(query)),
        n_probes,
        k);
      grn_obj_unref(ctx, leaf_table);
      auto maybe_candidates =
        searcher.search([&](grn_id id) { return resolve_id(id) != GRN_ID_NIL; },
                        ascending);
      if (!maybe_candidates) return ctx->rc;

      auto candidates = *maybe_candidates;
      for (size_t i = offset; i < k; ++i) {
        const auto &candidate = candidates[i];
        void *value;
        auto id =
          grn_array_add(ctx, reinterpret_cast<grn_array *>(result), &value);
        if (id == GRN_ID_NIL) {
          break;
        }
        auto sorted_id = static_cast<grn_id *>(value);
        *sorted_id = resolve_id(candidate.record_id);
      }
    }

    return ctx->rc;
  }
} // namespace

extern "C" {
grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  {
    grn_obj *tokenizer = grn_tokenizer_create(ctx, "TokenLanguageModelKNN", -1);
    if (tokenizer) {
      grn_tokenizer_set_build_func(ctx, tokenizer, build);
      grn_tokenizer_set_init_func(ctx, tokenizer, init);
      grn_tokenizer_set_next_func(ctx, tokenizer, next);
      grn_tokenizer_set_fin_func(ctx, tokenizer, fin);
    }
  }

  {
    grn_obj *proc = grn_proc_create(ctx,
                                    "language_model_knn",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    nullptr);
    grn_proc_set_selector(ctx, proc, language_model_knn_selector);
    grn_proc_set_selector_operator(ctx, proc, GRN_OP_SIMILAR);
    grn_proc_set_sorter(ctx, proc, language_model_knn_sorter);
  }
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
}
