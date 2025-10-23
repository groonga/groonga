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
#include <faiss/impl/RaBitQuantizer.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
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
    RABITQ,
  };

  struct Options {
    /* Input */
    std::string model_name;
    uint32_t n_clusters;
    Quantizer quantizer;
    std::string code_column_name;

    /* Others are cached data for performance. */

    grn_language_model *model;
    grn_language_model_inferencer *inferencer;
    uint32_t n_dimensions;
    std::vector<float> centroids;
    std::vector<grn_id> centroid_index_to_record_id;
    faiss::IndexFlatIP centroid_searcher;

    /* For RaBitQ */
    grn_obj *code_column;
    faiss::RandomRotationMatrix random_rotation_matrix; /* P in RaBitQ */
    faiss::RaBitQuantizer rabitq;
  };

  void
  options_init(grn_ctx *ctx, Options *options)
  {
    new (&options->model_name) std::string;
    new (&options->code_column_name) std::string;
    options->n_clusters = 0;
    options->quantizer = Quantizer::RABITQ;

    options->model = nullptr;
    options->inferencer = nullptr;
    options->n_dimensions = 0;
    new (&options->centroids) std::vector<float>;
    new (&options->centroid_index_to_record_id) std::vector<grn_id>;
    new (&options->centroid_searcher) faiss::IndexFlatIP;

    options->code_column = nullptr;
    new (&options->random_rotation_matrix) faiss::RandomRotationMatrix;
    new (&options->rabitq) faiss::RaBitQuantizer;
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

    options->rabitq.~RaBitQuantizer();
    options->random_rotation_matrix.~RandomRotationMatrix();

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
    grn_obj embeddings;
    grn_obj transformed_embeddings;
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
    GRN_FLOAT32_INIT(&(tokenizer->embeddings), GRN_OBJ_VECTOR);
    GRN_FLOAT32_INIT(&(tokenizer->transformed_embeddings), GRN_OBJ_VECTOR);
    GRN_SHORT_BINARY_INIT(&(tokenizer->code), 0);

    return tokenizer;
  }

  void
  tokenizer_close(grn_ctx *ctx, Tokenizer *tokenizer)
  {
    GRN_OBJ_FIN(ctx, &(tokenizer->embeddings));
    GRN_OBJ_FIN(ctx, &(tokenizer->transformed_embeddings));
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

    options_init(ctx, options);
    GRN_OPTION_VALUES_EACH_BEGIN(ctx, raw_options, i, name, name_length)
    {
      std::string_view name_raw(name, name_length);
      if (name_raw == "model") {
        const char *raw_value = nullptr;
        grn_id domain = GRN_ID_NIL;
        auto length = grn_vector_get_element(ctx,
                                             raw_options,
                                             i,
                                             &raw_value,
                                             nullptr,
                                             &domain);
        if (grn_type_id_is_text_family(ctx, domain)) {
          options->model_name = std::string(raw_value, length);
        }
      } else if (name_raw == "n_clusters") {
        options->n_clusters =
          grn_vector_get_element_uint32(ctx,
                                        raw_options,
                                        i,
                                        options->n_clusters);
      } else if (name_raw == "quantizer") {
        // TODO: RaBitQ is only supported for now
      } else if (name_raw == "code_column") {
        const char *raw_value = nullptr;
        grn_id domain = GRN_ID_NIL;
        auto length = grn_vector_get_element(ctx,
                                             raw_options,
                                             i,
                                             &raw_value,
                                             nullptr,
                                             &domain);
        if (grn_type_id_is_text_family(ctx, domain)) {
          options->code_column_name = std::string(raw_value, length);
        }
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

    options->n_dimensions =
      grn_language_model_get_n_embedding_dimensions(ctx, options->model);
    options->centroid_searcher.d = options->n_dimensions;
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
    options->centroid_searcher.codes =
      faiss::MaybeOwnedVector<uint8_t>::create_view(options->centroids.data(),
                                                    sizeof(float) *
                                                      options->centroids.size(),
                                                    nullptr);
    options->centroid_searcher.ntotal = options->centroids.size();

    options->random_rotation_matrix.d_in = options->n_dimensions;
    options->random_rotation_matrix.d_out = options->n_dimensions;
    options->random_rotation_matrix.init(SEED);

    // We can use inner product here because we normalize generated
    // embeddings.
    options->rabitq =
      faiss::RaBitQuantizer(options->n_dimensions,
                            faiss::MetricType::METRIC_INNER_PRODUCT);

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
        embeddings_set_path_(),
        embeddings_set_map_(nullptr),
        embeddings_set_(),
        transformed_embeddings_set_raw_(nullptr)
    {
      GRN_FLOAT32_INIT(&embeddings_set_,
                       GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY);
    }

    ~Builder()
    {
      GRN_OBJ_FIN(ctx_, &embeddings_set_);
      if (embeddings_set_map_) {
        grn_memory_map_close(ctx_, embeddings_set_map_);
        grn_unlink(embeddings_set_path_.data());
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
    std::string embeddings_set_path_;
    grn_memory_map *embeddings_set_map_;
    grn_obj embeddings_set_;
    float *transformed_embeddings_set_raw_;

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

      faiss::Clustering clustering(options_->n_dimensions,
                                   options_->n_clusters);
      embeddings_set_path_ = grn_obj_path(ctx_, index_column_);
      embeddings_set_path_ += ".embeddings_set";
      size_t embeddings_set_size =
        sizeof(float) * options_->n_dimensions * n_source_records_;
      embeddings_set_map_ = grn_memory_map_open(
        ctx_,
        embeddings_set_path_.data(),
        GRN_MEMORY_MAP_READ | GRN_MEMORY_MAP_WRITE,
        0,
        // The former part is for raw embeddings set and
        // the latter part is for transformed embeddings set.
        embeddings_set_size * 2);
      auto embeddings_set_raw = static_cast<float *>(
        grn_memory_map_get_address(ctx_, embeddings_set_map_));
      transformed_embeddings_set_raw_ =
        embeddings_set_raw + (embeddings_set_size / sizeof(float));
      GRN_BINARY_SET_REF(&embeddings_set_,
                         embeddings_set_raw,
                         embeddings_set_size);
      GRN_BULK_REWIND(&embeddings_set_);
      grn_language_model_inferencer_vectorize_in_batch(ctx_,
                                                       options_->inferencer,
                                                       cursor,
                                                       source_column_,
                                                       &embeddings_set_);
      grn_table_cursor_close(ctx_, cursor);

      if (GRN_BULK_VSIZE(&embeddings_set_) != embeddings_set_size) {
        GRN_PLUGIN_ERROR(
          ctx_,
          GRN_UNKNOWN_ERROR,
          "%s failed to generate "
          "float[%u] * %u (%" PRIu64 " bytes) embeddings: "
          "%" PRIu64 " bytes are only generated",
          TOKENIZER_TAG,
          options_->n_dimensions,
          n_source_records_,
          static_cast<uint64_t>(embeddings_set_size),
          static_cast<uint64_t>(GRN_BULK_VSIZE(&embeddings_set_)));
        return false;
      }

      options_->random_rotation_matrix.apply_noalloc(
        n_source_records_,
        embeddings_set_raw,
        transformed_embeddings_set_raw_);
      clustering.train(n_source_records_,
                       transformed_embeddings_set_raw_,
                       index);
      size_t n_centroids = clustering.k;
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
      options_->centroid_searcher.codes =
        faiss::MaybeOwnedVector<uint8_t>::create_view(
          options_->centroids.data(),
          sizeof(float) * options_->centroids.size(),
          nullptr);
      options_->centroid_searcher.ntotal = n_centroids;

      return true;
    }

    bool
    build_inverted_index(faiss::IndexFlatIP &index)
    {
      grn_obj tokens;
      GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, grn_obj_id(ctx_, lexicon_));
      grn_obj code;
      GRN_SHORT_BINARY_INIT(&code, 0);
      grn_bulk_space(ctx_, &code, options_->rabitq.code_size);
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
        const float *transformed_embeddings =
          transformed_embeddings_set_raw_ + (options_->n_dimensions * i);
        float distances[1];
        faiss::idx_t indexes[1];
        index.search(1, transformed_embeddings, 1, distances, indexes);
        const float *centroid =
          options_->centroids.data() + (options_->n_dimensions * indexes[0]);
        options_->rabitq.compute_codes_core(
          transformed_embeddings,
          reinterpret_cast<uint8_t *>(GRN_BULK_HEAD(&code)),
          1,
          centroid);
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

    grn_language_model_inferencer_vectorize(ctx,
                                            tokenizer->options->inferencer,
                                            tokenizer->query.data(),
                                            tokenizer->query.size(),
                                            &(tokenizer->embeddings));
    if (ctx->rc != GRN_SUCCESS) {
      GRN_PLUGIN_ERROR(ctx,
                       ctx->rc,
                       "%s failed to vectorize: <%.*s>",
                       TAG,
                       static_cast<int>(tokenizer->query.size()),
                       tokenizer->query.data());
      return;
    }

    auto embeddings =
      reinterpret_cast<const float *>(GRN_BULK_HEAD(&(tokenizer->embeddings)));
    grn_bulk_space(ctx,
                   &(tokenizer->transformed_embeddings),
                   GRN_BULK_VSIZE(&(tokenizer->embeddings)));
    auto transformed_embeddings = reinterpret_cast<float *>(
      GRN_BULK_HEAD(&(tokenizer->transformed_embeddings)));
    tokenizer->options->random_rotation_matrix.apply_noalloc(
      1,
      embeddings,
      transformed_embeddings);
    size_t k = 1;
    float distances[1];
    faiss::idx_t indexes[1];
    tokenizer->options->centroid_searcher.search(1,
                                                 transformed_embeddings,
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
      grn_bulk_space(ctx,
                     &(tokenizer->code),
                     tokenizer->options->rabitq.code_size);
      tokenizer->options->rabitq.compute_codes_core(
        transformed_embeddings,
        reinterpret_cast<uint8_t *>(GRN_BULK_HEAD(&(tokenizer->code))),
        1,
        centroid);
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

    if (n_args != 4) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s wrong number of arguments (%d for 4)",
                       tag,
                       n_args);
      return ctx->rc;
    }

    auto lexicon = grn_ctx_at(ctx, index->header.domain);
    OpenOptionsData open_options_data;
    open_options_data.lexicon = lexicon;
    open_options_data.source_table = table;
    auto options = static_cast<Options *>(
      grn_table_cache_default_tokenizer_options(ctx,
                                                lexicon,
                                                open_options,
                                                close_options,
                                                &open_options_data));

    grn_obj embeddings;
    GRN_FLOAT32_INIT(&embeddings, GRN_OBJ_VECTOR);
    grn_language_model_inferencer_vectorize(ctx,
                                            options->inferencer,
                                            GRN_TEXT_VALUE(args[2]),
                                            GRN_TEXT_LEN(args[2]),
                                            &embeddings);
    auto raw_embeddings =
      reinterpret_cast<const float *>(GRN_BULK_HEAD(&embeddings));
    grn_obj transformed_embeddings;
    GRN_FLOAT32_INIT(&transformed_embeddings, GRN_OBJ_VECTOR);
    grn_bulk_space(ctx, &transformed_embeddings, GRN_BULK_VSIZE(&embeddings));
    auto raw_transformed_embeddings =
      reinterpret_cast<float *>(GRN_BULK_HEAD(&transformed_embeddings));
    options->random_rotation_matrix.apply_noalloc(1,
                                                  raw_embeddings,
                                                  raw_transformed_embeddings);
    GRN_OBJ_FIN(ctx, &embeddings);

    auto n_probes = std::min(
      static_cast<uint32_t>(10),
      static_cast<uint32_t>(options->centroids.size() / options->n_dimensions));
    std::vector<float> centroid_distances(n_probes);
    std::vector<faiss::idx_t> centroid_indexes(n_probes, -1);
    options->centroid_searcher.search(1,
                                      raw_transformed_embeddings,
                                      n_probes,
                                      centroid_distances.data(),
                                      centroid_indexes.data());

    std::vector<std::pair<grn_id, float>> candidates;
    for (auto centroid_index : centroid_indexes) {
      if (centroid_index == -1) {
        break;
      }

      auto lexicon_record_id =
        options->centroid_index_to_record_id[centroid_index];
      auto ii = reinterpret_cast<grn_ii *>(index);
      auto cursor = grn_ii_cursor_open(ctx,
                                       ii,
                                       lexicon_record_id,
                                       GRN_ID_NIL,
                                       GRN_ID_MAX,
                                       0,
                                       0);
      if (!cursor) {
        continue;
      }

      const float *centroid =
        options->centroids.data() + (options->n_dimensions * centroid_index);
      uint8_t n_scalar_quantization_bits = 8;
      auto distance_computer =
        options->rabitq.get_distance_computer(n_scalar_quantization_bits,
                                              centroid);
      distance_computer->set_query(raw_transformed_embeddings);
      grn_obj code;
      GRN_VOID_INIT(&code);
      while (true) {
        auto posting = grn_ii_cursor_next(ctx, cursor);
        if (!posting) {
          break;
        }
        auto source_record_id = posting->rid;

        // TODO: Skip nonexistent record when op == GRN_OP_AND

        GRN_BULK_REWIND(&code);
        grn_obj_get_value(ctx, options->code_column, source_record_id, &code);
        auto distance = distance_computer->distance_to_code(
          reinterpret_cast<const uint8_t *>(GRN_BULK_HEAD(&code)));
        candidates.emplace_back(source_record_id, distance);
      }
      GRN_OBJ_FIN(ctx, &code);
      grn_ii_cursor_close(ctx, cursor);
      delete distance_computer;
    }

    GRN_OBJ_FIN(ctx, &transformed_embeddings);

    auto k = std::min(static_cast<size_t>(GRN_UINT32_VALUE(args[3])),
                      candidates.size());
    std::partial_sort(
      candidates.begin(),
      candidates.begin() + k,
      candidates.end(),
      [](const std::pair<grn_id, float> &a, const std::pair<grn_id, float> &b) {
        return a.second > b.second;
      });

    auto posting = grn_posting_open(ctx);
    posting->sid = 0;
    posting->pos = 0;
    for (size_t i = 0; i < k; ++i) {
      const auto &candidate = candidates[i];
      posting->rid = candidate.first;
      grn_posting_set_weight_float(ctx, posting, candidate.second);
      grn_result_set_add_record(ctx,
                                reinterpret_cast<grn_hash *>(res),
                                posting,
                                op);
    }
    grn_ii_resolve_sel_and(ctx, reinterpret_cast<grn_hash *>(res), op);
    grn_posting_close(ctx, posting);

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
    grn_obj *tokenizer = grn_tokenizer_create(ctx, "TokenLanguageModel", -1);
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
  }
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
}
