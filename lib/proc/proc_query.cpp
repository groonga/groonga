/*
  Copyright(C) 2016  Brazil
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_ctx.hpp"
#include "../grn_expr.h"
#include "../grn_proc.h"
#include "../grn_hash.h"
#include "../grn_table_selector.h"

#include <groonga/plugin.h>
#include <groonga.hpp>

#ifdef GRN_WITH_APACHE_ARROW
# include "../grn_arrow.hpp"
# include <arrow/util/thread_pool.h>
# include <mutex>
# include <thread>
#endif

#include <vector>

static bool grn_query_min_id_skip_enable = false;
static int grn_query_parallel_or_n_conditions_threshold = 4;
static int grn_query_parallel_or_n_threads_limit = -1;

extern "C" void
grn_proc_query_init_from_env(void)
{
  {
    char env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_QUERY_MIN_ID_SKIP_ENABLE",
               env,
               GRN_ENV_BUFFER_SIZE);
    if (std::string(env) == "yes") {
      grn_query_min_id_skip_enable = true;
    }
  }

  {
    char env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_QUERY_PARALLEL_OR_N_CONDITIONS_THRESHOLD",
               env,
               GRN_ENV_BUFFER_SIZE);
    if (env[0]) {
      grn_query_parallel_or_n_conditions_threshold = atoi(env);
    }
  }

  {
#ifdef GRN_WITH_APACHE_ARROW
    grn_query_parallel_or_n_threads_limit =
      std::thread::hardware_concurrency() / 3;
#endif
    char env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_QUERY_PARALLEL_OR_N_THREADS_LIMIT",
               env,
               GRN_ENV_BUFFER_SIZE);
    if (env[0]) {
      grn_query_parallel_or_n_threads_limit = atoi(env);
    }
  }
}

namespace {
  class BaseQueryExecutor {
  protected:
    BaseQueryExecutor(grn_ctx *ctx,
                      grn_obj *table,
                      int n_args,
                      grn_obj **args,
                      grn_obj *res,
                      grn_operator op,
                      grn_selector_data *selector_data,
                      const char *tag) :
      ctx_(ctx),
      table_(table),
      n_args_(n_args),
      args_(args),
      res_(res),
      op_(op),
      selector_data_(selector_data),
      tag_(tag),
      match_columns_raw_(nullptr),
      query_expander_name_(nullptr),
      query_options_(nullptr),
      default_mode_(GRN_OP_MATCH),
      default_operator_(GRN_OP_AND),
      flags_(GRN_EXPR_SYNTAX_QUERY),
      flags_specified_(-1),
      enough_filtered_ratio_(-1),
      max_n_enough_filtered_records_(-1),
      match_columns_(nullptr) {
    }

    virtual ~BaseQueryExecutor() {
      if (match_columns_) {
        grn_obj_close(ctx_, match_columns_);
      }
    }

  public:
    grn_rc
    execute() {
      if (!prepare()) {
        return ctx_->rc;
      }
      if (op_ == GRN_OP_OR || grn_table_size(ctx_, res_) > 0) {
        if (!run()) {
          return ctx_->rc;
        }
      }
      return ctx_->rc;
    }

  private:
    virtual bool prepare() = 0;
    virtual bool run() = 0;

  protected:
    grn_id
    get_min_id() {
      if (op_ == GRN_OP_OR) {
        return GRN_ID_NIL;
      }
      if (!grn_query_min_id_skip_enable) {
        return GRN_ID_NIL;
      }
      if (grn_table_size(ctx_, res_) > 10000) {
        return GRN_ID_NIL;
      }
      return grn_result_set_get_min_id(ctx_,
                                       reinterpret_cast<grn_hash *>(res_));
    }

    float
    get_weight_factor() {
      if (selector_data_) {
        return grn_selector_data_get_weight_factor(ctx_, selector_data_);
      } else {
        return 1.0;
      }
    }

    void
    init_table_selector(grn_ctx *ctx,
                        grn_table_selector *table_selector,
                        grn_obj *condition,
                        grn_operator op,
                        grn_id min_id) {
      grn_table_selector_init(ctx,
                              table_selector,
                              table_,
                              condition,
                              op);
      grn_table_selector_set_min_id(ctx,
                                    table_selector,
                                    min_id);
      grn_table_selector_set_weight_factor(ctx,
                                           table_selector,
                                           get_weight_factor());
      if (enough_filtered_ratio_ >= 0) {
        grn_table_selector_set_enough_filtered_ratio(ctx_,
                                                     table_selector,
                                                     enough_filtered_ratio_);
      }
      if (max_n_enough_filtered_records_ >= 0) {
        grn_table_selector_set_max_n_enough_filtered_records(
          ctx_,
          table_selector,
          max_n_enough_filtered_records_);
      }
    }

    bool
    parse_match_columns_arg() {
      match_columns_raw_ = args_[0];
      if (!grn_obj_is_text_family_bulk(ctx_, match_columns_raw_) &&
          !grn_obj_is_vector(ctx_, match_columns_raw_)) {
        grn::TextBulk inspected(ctx_);
        grn_inspect(ctx_, *inspected, match_columns_raw_);
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "%s 1st argument must be string or array: <%.*s>",
                         tag_,
                         (int)GRN_TEXT_LEN(*inspected),
                         GRN_TEXT_VALUE(*inspected));
        return false;
      }
      return true;
    }

    bool
    parse_options(grn_obj *options) {
      grn_obj *query_options_ptr = NULL;
#define OPTIONS                                                         \
      "expander",                                                       \
        GRN_PROC_OPTION_VALUE_RAW,                                      \
        &query_expander_name_,                                          \
        "default_mode",                                                 \
        GRN_PROC_OPTION_VALUE_MODE,                                     \
        &default_mode_,                                                 \
        "default_operator",                                             \
        GRN_PROC_OPTION_VALUE_OPERATOR,                                 \
        &default_operator_,                                             \
        "flags",                                                        \
        GRN_PROC_OPTION_VALUE_EXPR_FLAGS,                               \
        &flags_specified_,                                              \
        "options",                                                      \
        GRN_PROC_OPTION_VALUE_RAW,                                      \
        &query_options_ptr,                                             \
        "enough_filtered_ratio",                                        \
        GRN_PROC_OPTION_VALUE_DOUBLE,                                   \
        &enough_filtered_ratio_,                                        \
        "max_n_enough_filtered_records",                                \
        GRN_PROC_OPTION_VALUE_INT64,                                    \
        &max_n_enough_filtered_records_
      if (selector_data_) {
        grn_selector_data_parse_options(ctx_,
                                        selector_data_,
                                        options,
                                        tag_,
                                        OPTIONS,
                                        NULL);
      } else {
        grn_proc_options_parse(ctx_,
                               options,
                               tag_,
                               OPTIONS,
                               NULL);
      }
#undef OPTIONS
      if (ctx_->rc != GRN_SUCCESS) {
        return false;
      }
      if (query_expander_name_ &&
          !grn_obj_is_text_family_bulk(ctx_, query_expander_name_)) {
        grn::TextBulk inspected(ctx_);
        grn_inspect(ctx_, *inspected, query_expander_name_);
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "%s query expander name must be string: <%.*s>",
                         tag_,
                         (int)GRN_TEXT_LEN(*inspected),
                         GRN_TEXT_VALUE(*inspected));
        return false;
      }
      if (query_options_ptr) {
        query_options_ = GRN_PTR_VALUE(query_options_ptr);
      }
      return true;
    }

    bool
    prepare_flags() {
      if (flags_specified_ == static_cast<grn_expr_flags>(-1)) {
        flags_ |= GRN_EXPR_ALLOW_PRAGMA | GRN_EXPR_ALLOW_COLUMN;
      } else {
        flags_ |= flags_specified_;
      }
      return true;
    }

    bool
    prepare_match_columns() {
      if (grn_obj_is_text_family_bulk(ctx_, match_columns_raw_)) {
        return prepare_match_columns_bulk();
      } else {
        return prepare_match_columns_vector();
      }
    }

    bool
    prepare_match_columns_one(const char *match_columns_string,
                              size_t match_columns_string_length,
                              grn_obj **match_columns_inout) {
      grn_obj *match_columns = *match_columns_inout;
      if (!match_columns) {
        grn_obj *dummy_variable;
        GRN_EXPR_CREATE_FOR_QUERY(ctx_,
                                  table_,
                                  match_columns,
                                  dummy_variable);
        *match_columns_inout = match_columns;
        if (!match_columns) {
          return false;
        }
      }

      grn_expr_parse(ctx_,
                     match_columns,
                     match_columns_string,
                     match_columns_string_length,
                     NULL,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT);
      return ctx_->rc == GRN_SUCCESS;
    }

    virtual bool
    prepare_match_columns_bulk() {
      if (GRN_TEXT_LEN(match_columns_raw_) == 0) {
        return true;
      }

      return prepare_match_columns_one(GRN_TEXT_VALUE(match_columns_raw_),
                                       GRN_TEXT_LEN(match_columns_raw_),
                                       &match_columns_);
    }

    virtual bool
    prepare_match_columns_vector() {
      auto n_match_columns = grn_vector_size(ctx_, match_columns_raw_);
      if (n_match_columns == 0) {
        return true;
      }

      uint32_t n_sub_match_columns = 0;
      for (unsigned int i = 0; i < n_match_columns; ++i) {
        const char *sub_match_columns_string;
        auto sub_match_columns_string_length =
          grn_vector_get_element(ctx_,
                                 match_columns_raw_,
                                 i,
                                 &sub_match_columns_string,
                                 NULL,
                                 NULL);
        if (sub_match_columns_string_length == 0) {
          continue;
        }
        if (!prepare_match_columns_one(sub_match_columns_string,
                                       sub_match_columns_string_length,
                                       &match_columns_)) {
          return false;
        }
        n_sub_match_columns++;
        if (n_sub_match_columns >= 2) {
          grn_expr_append_op(ctx_, match_columns_, GRN_OP_OR, 2);
        }
      }
      return true;
    }

    bool
    expand_query(grn_obj *query, grn::TextBulk &expanded_query)
    {
      if (!query_expander_name_ ||
          GRN_TEXT_LEN(query_expander_name_) == 0) {
        GRN_TEXT_SET(ctx_,
                     *expanded_query,
                     GRN_TEXT_VALUE(query),
                     GRN_TEXT_LEN(query));
        return true;
      }
      grn_proc_syntax_expand_query(ctx_,
                                   GRN_TEXT_VALUE(query),
                                   GRN_TEXT_LEN(query),
                                   flags_,
                                   GRN_TEXT_VALUE(query_expander_name_),
                                   GRN_TEXT_LEN(query_expander_name_),
                                   NULL, 0,
                                   NULL, 0,
                                   *expanded_query,
                                   tag_);
      return ctx_->rc == GRN_SUCCESS;
    }

    grn_obj *
    build_condition(grn_ctx *ctx,
                    grn_obj *match_columns,
                    const char *query,
                    size_t query_length) {
      grn_obj *condition;
      grn_obj *dummy_variable;
      GRN_EXPR_CREATE_FOR_QUERY(ctx,
                                table_,
                                condition,
                                dummy_variable);
      if (!condition) {
        return nullptr;
      }
      grn_expr_parse(ctx,
                     condition,
                     query,
                     query_length,
                     match_columns,
                     default_mode_,
                     default_operator_,
                     flags_);
      if (ctx->rc != GRN_SUCCESS) {
        grn_obj_close(ctx, condition);
        return nullptr;
      }
      if (query_options_) {
        grn_expr_set_query_options(ctx, condition, query_options_);
      }
      return condition;
    }

    bool
    select_one(grn_ctx *ctx,
               grn_obj *match_columns,
               const char *query,
               size_t query_length,
               grn_operator op,
               grn_id min_id,
               grn_obj **result_set) {
      if (ctx->rc != GRN_SUCCESS) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s[select-one] "
                "ctx->rc != GRN_SUCCESS before build_condition(): "
                "<%.*s>: %d: %s:%d: %s",
                tag_,
                static_cast<int>(query_length),
                query,
                ctx->rc,
                ctx->errfile,
                ctx->errline,
                ctx->errbuf);
      }
      auto condition = build_condition(ctx,
                                       match_columns,
                                       query,
                                       query_length);
      if (!condition) {
        grn_rc rc = ctx->rc;
        if (rc == GRN_SUCCESS) {
          rc = GRN_UNKNOWN_ERROR;
        }
        GRN_PLUGIN_ERROR(ctx,
                         rc,
                         "%s failed to build a condition: <%.*s>",
                         tag_,
                         static_cast<int>(query_length),
                         query);
        return false;
      }
      if (ctx->rc != GRN_SUCCESS) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s[select-one] "
                "ctx->rc != GRN_SUCCESS before init_table_selector(): "
                "<%.*s>: %d: %s:%d: %s",
                tag_,
                static_cast<int>(query_length),
                query,
                ctx->rc,
                ctx->errfile,
                ctx->errline,
                ctx->errbuf);
      }
      grn::UniqueObj unique_condition(ctx, condition);
      grn_table_selector table_selector;
      init_table_selector(ctx,
                          &table_selector,
                          condition,
                          op,
                          min_id);
      if (ctx->rc != GRN_SUCCESS) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s[select-one] "
                "ctx->rc != GRN_SUCCESS before grn_table_selector_select(): "
                "<%.*s>: %d: %s:%d: %s",
                tag_,
                static_cast<int>(query_length),
                query,
                ctx->rc,
                ctx->errfile,
                ctx->errline,
                ctx->errbuf);
      }
      auto new_result_set = grn_table_selector_select(ctx,
                                                      &table_selector,
                                                      *result_set);
      if (new_result_set) {
        if (ctx->rc == GRN_SUCCESS) {
          *result_set = new_result_set;
        } else {
          grn_obj_close(ctx, new_result_set);
        }
      }
      if (ctx->rc != GRN_SUCCESS) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s[select-one] "
                "ctx->rc != GRN_SUCCESS before grn_table_selector_fin(): "
                "<%.*s>: %d: %s:%d: %s",
                tag_,
                static_cast<int>(query_length),
                query,
                ctx->rc,
                ctx->errfile,
                ctx->errline,
                ctx->errbuf);
      }
      grn_table_selector_fin(ctx, &table_selector);
      return ctx->rc == GRN_SUCCESS;
    }

    grn_ctx *ctx_;
    grn_obj *table_;
    int n_args_;
    grn_obj **args_;
    grn_obj *res_;
    grn_operator op_;
    grn_selector_data *selector_data_;
    const char *tag_;

    grn_obj *match_columns_raw_;
    grn_obj *query_expander_name_;
    grn_obj *query_options_;
    grn_operator default_mode_;
    grn_operator default_operator_;
    grn_expr_flags flags_;
    grn_expr_flags flags_specified_;
    double enough_filtered_ratio_;
    int64_t max_n_enough_filtered_records_;
    grn_obj *match_columns_;
  };

  class QueryExecutor : public BaseQueryExecutor {
  public:
    QueryExecutor(grn_ctx *ctx,
                  grn_obj *table,
                  int n_args,
                  grn_obj **args,
                  grn_obj *res,
                  grn_operator op,
                  grn_selector_data *selector_data) :
      BaseQueryExecutor(ctx,
                        table,
                        n_args,
                        args,
                        res,
                        op,
                        selector_data,
                        "[query]"),
      query_(nullptr) {
    }

    ~QueryExecutor() override {
    }

  private:
    bool
    prepare() override {
      if (!validate_args()) {
        return false;
      }
      if (!parse_match_columns_arg()) {
        return false;
      }
      if (!parse_query_arg()) {
        return false;
      }
      if (n_args_ == 3) {
        grn_obj *options = args_[2];
        switch (options->header.type) {
        case GRN_BULK :
          query_expander_name_ = options;
          break;
        case GRN_TABLE_HASH_KEY :
          if (!parse_options(options)) {
            return false;
          }
          break;
        default :
          {
            grn::TextBulk inspected(ctx_);
            grn_inspect(ctx_, *inspected, options);
            GRN_PLUGIN_ERROR(ctx_,
                             GRN_INVALID_ARGUMENT,
                             "%s "
                             "3rd argument must be string "
                             "or object literal: <%.*s>",
                             tag_,
                             (int)GRN_TEXT_LEN(*inspected),
                             GRN_TEXT_VALUE(*inspected));
            return false;
          }
          break;
        }
      }
      if (!prepare_flags()) {
        return false;
      }
      if (!prepare_match_columns()) {
        return false;
      }
      return true;
    }

    bool
    validate_args() {
      if (!(2 <= n_args_ && n_args_ <= 3)) {
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "%s wrong number of arguments (%d for 2..3)",
                         tag_,
                         n_args_);
        return false;
      }
      return true;
    }

    bool
    parse_query_arg() {
      query_ = args_[1];
      if (!grn_obj_is_text_family_bulk(ctx_, query_)) {
        grn::TextBulk inspected(ctx_);
        grn_inspect(ctx_, *inspected, query_);
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "%s 2nd argument must be string: <%.*s>",
                         tag_,
                         (int)GRN_TEXT_LEN(*inspected),
                         GRN_TEXT_VALUE(*inspected));
        return false;
      }
      return true;
    }

    bool
    run() override {
      if (GRN_TEXT_LEN(query_) == 0) {
        return true;
      }

      grn::TextBulk expanded_query(ctx_);
      if (!expand_query(query_, expanded_query)) {
        return false;
      }

      return select_one(ctx_,
                        match_columns_,
                        GRN_TEXT_VALUE(*expanded_query),
                        GRN_TEXT_LEN(*expanded_query),
                        op_,
                        get_min_id(),
                        &res_);
    }

    grn_obj *query_;
  };

  class QueryParallelOrExecutor : public BaseQueryExecutor {
  public:
    QueryParallelOrExecutor(grn_ctx *ctx,
                            grn_obj *table,
                            int n_args,
                            grn_obj **args,
                            grn_obj *res,
                            grn_operator op,
                            grn_selector_data *selector_data) :
      BaseQueryExecutor(ctx,
                        table,
                        n_args,
                        args,
                        res,
                        op,
                        selector_data,
                        "[query-parallel-or]"),
      expanded_queries_(),
      sub_match_columns_() {
      GRN_PTR_INIT(&sub_match_columns_,
                   GRN_OBJ_VECTOR | GRN_OBJ_OWN,
                   GRN_ID_NIL);
    }

    ~QueryParallelOrExecutor() override {
      GRN_OBJ_FIN(ctx_, &sub_match_columns_);
    }

  private:
    bool
    prepare_match_columns_bulk() override {
      if (GRN_TEXT_LEN(match_columns_raw_) > 0) {
        grn_obj *sub_match_columns = nullptr;
        if (!prepare_match_columns_one(GRN_TEXT_VALUE(match_columns_raw_),
                                       GRN_TEXT_LEN(match_columns_raw_),
                                       &sub_match_columns)) {
          return false;
        }
        GRN_PTR_PUT(ctx_, &sub_match_columns_, sub_match_columns);
      } else {
        GRN_PTR_PUT(ctx_, &sub_match_columns_, NULL);
      }
      return true;
    }

    bool
    prepare_match_columns_vector() override {
      auto n_match_columns = grn_vector_size(ctx_, match_columns_raw_);
      if (n_match_columns == 0) {
        return true;
      }

      for (unsigned int i = 0; i < n_match_columns; ++i) {
        const char *sub_match_columns_string;
        auto sub_match_columns_string_length =
          grn_vector_get_element(ctx_,
                                 match_columns_raw_,
                                 i,
                                 &sub_match_columns_string,
                                 NULL,
                                 NULL);
        if (sub_match_columns_string_length == 0) {
          continue;
        }
        grn_obj *sub_match_columns = nullptr;
        if (!prepare_match_columns_one(sub_match_columns_string,
                                       sub_match_columns_string_length,
                                       &sub_match_columns)) {
          return false;
        }
        GRN_PTR_PUT(ctx_, &sub_match_columns_, sub_match_columns);
      }
      return true;
    }

    bool
    prepare() override {
      if (!validate_args()) {
        return false;
      }
      if (!parse_match_columns_arg()) {
        return false;
      }
      auto n_queries_ = n_args_ - 1;
      if (n_args_ > 2 && args_[n_args_ - 1]->header.type == GRN_TABLE_HASH_KEY) {
        n_queries_--;
        auto options = args_[n_args_ - 1];
        if (!parse_options(options)) {
          return false;
        }
      }
      if (!prepare_flags()) {
        return false;
      }
      if (!parse_query_args(args_ + 1, n_queries_)) {
        return false;
      }
      if (!prepare_match_columns()) {
        return false;
      }
      return true;
    }

    bool
    validate_args() {
      if (n_args_ < 2) {
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "%s wrong number of arguments (%d for 2..)",
                         tag_,
                         n_args_);
        return false;
      }
      return true;
    }

    bool
    parse_query_args(grn_obj **queries, int n_queries) {
      grn::TextBulk expanded_query(ctx_);
      for (int i = 0; i < n_queries; ++i) {
        auto query = queries[i];
        if (!grn_obj_is_text_family_bulk(ctx_, query)) {
          grn::TextBulk inspected(ctx_);
          grn_inspect(ctx_, *inspected, query);
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_INVALID_ARGUMENT,
                           "%s %dth argument must be string: <%.*s>",
                           tag_,
                           static_cast<int>(queries - args_ + i),
                           static_cast<int>(GRN_TEXT_LEN(*inspected)),
                           GRN_TEXT_VALUE(*inspected));
          return false;
        }
        if (GRN_TEXT_LEN(query) == 0) {
          continue;
        }
        GRN_BULK_REWIND(*expanded_query);
        if (!expand_query(query, expanded_query)) {
          return false;
        }
        expanded_queries_.emplace_back(GRN_TEXT_VALUE(*expanded_query),
                                       GRN_TEXT_LEN(*expanded_query));
      }
      return true;
    }

    bool
    run() override {
      if (expanded_queries_.empty()) {
        return true;
      }
      if (!select()) {
        return false;
      }
      return ctx_->rc == GRN_SUCCESS;
    }

    bool
    select() {
      bool processed = false;
#ifdef GRN_WITH_APACHE_ARROW
      int n_conditions =
        GRN_PTR_VECTOR_SIZE(&sub_match_columns_) *
        expanded_queries_.size();
      if (n_conditions >= grn_query_parallel_or_n_conditions_threshold) {
        auto capacity = grn_query_parallel_or_n_threads_limit;
        if (capacity < 0) {
          capacity = ::arrow::internal::ThreadPool::DefaultCapacity();
        }
        auto pool = ::arrow::internal::ThreadPool::MakeEternal(capacity);
        if (pool.ok()) {
          if (!select_parallel(*pool)) {
            return false;
          }
          processed = true;
        }
      }
#endif
      if (!processed) {
        if (!select_sequential()) {
          return false;
        }
      }
      return true;
    }

#ifdef GRN_WITH_APACHE_ARROW
    bool
    select_parallel(std::shared_ptr<::arrow::internal::ThreadPool> pool) {
      grn_id min_id = get_min_id();
      // merge isn't parallel. This is just used for queue.
      auto merge_pool_result = ::arrow::internal::ThreadPool::MakeEternal(1);
      if (!grnarrow::check(ctx_,
                           merge_pool_result,
                           "%s failed to create a thread pool for merging",
                           tag_)) {
        return false;
      }
      auto merge_pool = *merge_pool_result;
      std::mutex mutex;
      std::vector<grn::UniqueObj> unique_sub_results;
      auto merge = [&](grn_obj *sub_result) {
        std::lock_guard<std::mutex> lock(mutex);
        if (unique_sub_results.empty()) {
          unique_sub_results.emplace_back(ctx_, sub_result);
        } else {
          if (ctx_->rc == GRN_SUCCESS) {
            grn_table_setoperation(ctx_,
                                   unique_sub_results[0].get(),
                                   sub_result,
                                   unique_sub_results[0].get(),
                                   GRN_OP_OR);
          }
          unique_sub_results.emplace_back(ctx_, sub_result);
        }
        return arrow::Status::OK();
      };
      std::vector<::arrow::Future<>> merge_futures;
      auto select = [&](grn_obj *match_columns, const std::string &query) {
        auto sub_ctx = grn_ctx_pull_child(ctx_);
        grn::ChildCtxReleaser releaser(ctx_, sub_ctx);
        grn_obj *sub_result = nullptr;
        {
          std::lock_guard<std::mutex> lock(mutex);
          auto n_unique_sub_results = unique_sub_results.size();
          if (n_unique_sub_results < 2) {
            sub_result =
              grn_table_create(ctx_,
                               nullptr, 0,
                               nullptr,
                               GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                               table_,
                               nullptr);
          } else {
            sub_result =
              unique_sub_results[n_unique_sub_results - 1].release();
            unique_sub_results.pop_back();
            clear_result_set(ctx_, sub_result);
          }
        }
        if (!select_one(sub_ctx,
                        match_columns,
                        query.data(),
                        query.length(),
                        GRN_OP_OR,
                        min_id,
                        &sub_result)) {
          std::lock_guard<std::mutex> lock(mutex);
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_UNKNOWN_ERROR,
                           "%s failed to select a query: <%s>: %u: %s:%d: %s",
                           tag_,
                           query.c_str(),
                           sub_ctx->rc,
                           sub_ctx->errfile,
                           sub_ctx->errline,
                           sub_ctx->errbuf);
          return ::arrow::Status::OK();
        }
        auto merge_future = merge_pool->Submit(merge, sub_result);
        if (!merge_future.ok()) {
          std::lock_guard<std::mutex> lock(mutex);
          if (!grnarrow::check(ctx_,
                               merge_future,
                               "%s failed to submit a merge job",
                               tag_)) {
            return merge_future.status();
          }
        }
        {
          std::lock_guard<std::mutex> lock(mutex);
          merge_futures.push_back(*merge_future);
        }
        return ::arrow::Status::OK();
      };

      {
        std::vector<::arrow::Future<>> select_futures;
        auto n_sub_match_columns = GRN_PTR_VECTOR_SIZE(&sub_match_columns_);
        for (size_t i = 0; i < n_sub_match_columns; ++i) {
          grn_obj *sub_match_column = GRN_PTR_VALUE_AT(&sub_match_columns_, i);
          for (const auto &expanded_query : expanded_queries_) {
            auto select_future = pool->Submit(select,
                                              sub_match_column,
                                              expanded_query);
            if (!select_future.ok()) {
              std::lock_guard<std::mutex> lock(mutex);
              if (!grnarrow::check(ctx_,
                                   select_future,
                                   "%s failed to submit a select job",
                                   tag_)) {
                break;
              }
            }
            select_futures.push_back(*select_future);
          }
          if (ctx_->rc != GRN_SUCCESS) {
            break;
          }
        }
        auto status = ::arrow::Status::OK();
        for (auto& select_future : select_futures) {
          status &= select_future.status();
        }
        if (!grnarrow::check(ctx_,
                             status,
                             "%s failed to complete select jobs",
                             tag_)) {
          return false;
        }
        for (auto& merge_future : merge_futures) {
          status &= merge_future.status();
        }
        if (!grnarrow::check(ctx_,
                             status,
                             "%s failed to complete merge jobs",
                             tag_)) {
          return false;
        }
      }
      if (ctx_->rc == GRN_SUCCESS && !unique_sub_results.empty()) {
        grn_table_setoperation(ctx_,
                               res_,
                               unique_sub_results[0].get(),
                               res_,
                               op_);
       }
      return ctx_->rc == GRN_SUCCESS;
    }
#endif

    bool
    select_sequential() {
      grn_id min_id = get_min_id();
      grn::UniqueObj unique_sub_result(ctx_, nullptr);
      grn::UniqueObj unique_working_sub_result(ctx_, nullptr);
      auto n_sub_match_columns = GRN_PTR_VECTOR_SIZE(&sub_match_columns_);
      for (size_t i = 0; i < n_sub_match_columns; ++i) {
        grn_obj *sub_match_column = GRN_PTR_VALUE_AT(&sub_match_columns_, i);
        for (const auto &expanded_query : expanded_queries_) {
          auto sub_result = unique_working_sub_result.get();
          if (!select_one(ctx_,
                          sub_match_column,
                          expanded_query.data(),
                          expanded_query.length(),
                          GRN_OP_OR,
                          min_id,
                          &sub_result)) {
            return false;
          }
          if (!unique_sub_result.get()) {
            unique_sub_result.reset(sub_result);
          } else {
            grn_table_setoperation(ctx_,
                                   unique_sub_result.get(),
                                   sub_result,
                                   unique_sub_result.get(),
                                   GRN_OP_OR);
            if (!unique_working_sub_result.get()) {
              unique_working_sub_result.reset(sub_result);
            }
            clear_result_set(ctx_, unique_working_sub_result.get());
          }
        }
      }
      grn_table_setoperation(ctx_, res_, unique_sub_result.get(), res_, op_);
      return true;
    }

    void
    clear_result_set(grn_ctx *ctx, grn_obj *result_set) {
      // This is faster than grn_hash_truncate() or grn_table_create().
      GRN_HASH_EACH_BEGIN(ctx,
                          reinterpret_cast<grn_hash *>(result_set),
                          cursor,
                          id) {
        grn_hash_cursor_delete(ctx, cursor, nullptr);
      } GRN_HASH_EACH_END(ctx, cursor);
    }

    std::vector<std::string> expanded_queries_;
    grn_obj sub_match_columns_;
  };
}


static grn_rc
run_query(grn_ctx *ctx,
          grn_obj *table,
          int n_args,
          grn_obj **args,
          grn_obj *res,
          grn_operator op,
          grn_selector_data *selector_data)
{
  QueryExecutor executor(ctx,
                         table,
                         n_args,
                         args,
                         res,
                         op,
                         selector_data);
  return executor.execute();
}

static grn_obj *
func_query(grn_ctx *ctx, int n_args, grn_obj **args, grn_user_data *user_data)
{
  grn_proc_selector_to_function_data data;

  if (grn_proc_selector_to_function_data_init(ctx, &data, user_data)) {
    grn_rc rc;
    rc = run_query(ctx, data.table, n_args, args, data.records, GRN_OP_AND, NULL);
    if (rc == GRN_SUCCESS) {
      grn_proc_selector_to_function_data_selected(ctx, &data);
    }
  }
  grn_proc_selector_to_function_data_fin(ctx, &data);

  return data.found;
}

static grn_rc
selector_query(grn_ctx *ctx, grn_obj *table, grn_obj *index,
               int n_args, grn_obj **args,
               grn_obj *res, grn_operator op)
{
  return run_query(ctx,
                   table,
                   n_args - 1,
                   args + 1,
                   res,
                   op,
                   grn_selector_data_get(ctx));
}

void
grn_proc_init_query(grn_ctx *ctx)
{
  grn_obj *selector_proc = grn_proc_create(ctx,
                                           "query", -1,
                                           GRN_PROC_FUNCTION,
                                           func_query,
                                           NULL,
                                           NULL,
                                           0,
                                           NULL);
  grn_proc_set_selector(ctx, selector_proc, selector_query);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
}

static grn_rc
run_query_parallel_or(grn_ctx *ctx,
                      grn_obj *table,
                      int n_args,
                      grn_obj **args,
                      grn_obj *res,
                      grn_operator op,
                      grn_selector_data *selector_data)
{
  QueryParallelOrExecutor executor(ctx,
                                   table,
                                   n_args,
                                   args,
                                   res,
                                   op,
                                   selector_data);
  return executor.execute();
}

static grn_obj *
func_query_parallel_or(grn_ctx *ctx,
                       int n_args,
                       grn_obj **args,
                       grn_user_data *user_data)
{
  grn_proc_selector_to_function_data data;

  if (grn_proc_selector_to_function_data_init(ctx, &data, user_data)) {
    grn_rc rc;
    rc = run_query_parallel_or(ctx,
                               data.table,
                               n_args,
                               args,
                               data.records,
                               GRN_OP_AND,
                               NULL);
    if (rc == GRN_SUCCESS) {
      grn_proc_selector_to_function_data_selected(ctx, &data);
    }
  }
  grn_proc_selector_to_function_data_fin(ctx, &data);

  return data.found;
}

static grn_rc
selector_query_parallel_or(grn_ctx *ctx,
                           grn_obj *table,
                           grn_obj *index,
                           int n_args,
                           grn_obj **args,
                           grn_obj *res,
                           grn_operator op)
{
  return run_query_parallel_or(ctx,
                               table,
                               n_args - 1,
                               args + 1,
                               res,
                               op,
                               grn_selector_data_get(ctx));
}

void
grn_proc_init_query_parallel_or(grn_ctx *ctx)
{
  grn_obj *selector_proc = grn_proc_create(ctx,
                                           "query_parallel_or", -1,
                                           GRN_PROC_FUNCTION,
                                           func_query_parallel_or,
                                           NULL,
                                           NULL,
                                           0,
                                           NULL);
  grn_proc_set_selector(ctx, selector_proc, selector_query_parallel_or);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
}

static grn_obj *
command_query_expand(grn_ctx *ctx, int n_args, grn_obj **args,
                     grn_user_data *user_data)
{
  const char *expander;
  size_t expander_size;
  const char *query;
  size_t query_size;
  const char *flags_raw;
  size_t flags_raw_size;
  grn_expr_flags flags = GRN_EXPR_SYNTAX_QUERY;
  const char *term_column;
  size_t term_column_size;
  const char *expanded_term_column;
  size_t expanded_term_column_size;
  grn_obj expanded_query;

  expander = grn_plugin_proc_get_var_string(ctx,
                                            user_data,
                                            "expander",
                                            -1,
                                            &expander_size);
  query = grn_plugin_proc_get_var_string(ctx,
                                         user_data,
                                         "query",
                                         -1,
                                         &query_size);
  flags_raw = grn_plugin_proc_get_var_string(ctx,
                                             user_data,
                                             "flags",
                                             -1,
                                             &flags_raw_size);
  term_column = grn_plugin_proc_get_var_string(ctx,
                                            user_data,
                                            "term_column",
                                            -1,
                                            &term_column_size);
  expanded_term_column =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "expanded_term_column",
                                   -1,
                                   &expanded_term_column_size);

  if (flags_raw_size > 0) {
    grn_obj flags_bulk;
    GRN_TEXT_INIT(&flags_bulk, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET(ctx,
                 &flags_bulk,
                 flags_raw,
                 flags_raw_size);
    flags |= grn_proc_expr_query_flags_parse(ctx,
                                             &flags_bulk,
                                             "[query][expand]");
  } else {
    flags |= GRN_EXPR_ALLOW_PRAGMA | GRN_EXPR_ALLOW_COLUMN;
  }

  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  GRN_TEXT_INIT(&expanded_query, 0);
  grn_proc_syntax_expand_query(ctx,
                               query,
                               query_size,
                               flags,
                               expander,
                               expander_size,
                               term_column,
                               term_column_size,
                               expanded_term_column,
                               expanded_term_column_size,
                               &expanded_query,
                               "[query][expand]");
  if (ctx->rc == GRN_SUCCESS) {
    grn_ctx_output_str(ctx,
                       GRN_TEXT_VALUE(&expanded_query),
                       GRN_TEXT_LEN(&expanded_query));
  }
  GRN_OBJ_FIN(ctx, &expanded_query);

  return NULL;
}

void
grn_proc_init_query_expand(grn_ctx *ctx)
{
  grn_expr_var vars[5];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "expander", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "query", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "term_column", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "expanded_term_column", -1);
  grn_plugin_command_create(ctx,
                            "query_expand", -1,
                            command_query_expand,
                            5,
                            vars);
}
