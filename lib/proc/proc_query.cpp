/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_expr.h"
#include "../grn_proc.h"
#include "../grn_hash.h"

#include <groonga/plugin.h>
#include <groonga.hpp>

#ifdef GRN_WITH_APACHE_ARROW
# include "../grn_arrow.hpp"
# include <arrow/util/thread_pool.h>
# include <mutex>
# include <thread>
#endif

#include <vector>

static int grn_query_parallel_or_n_conditions_threshold = 4;
static int grn_query_parallel_or_n_threads_limit = -1;

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
      match_columns_string_(nullptr),
      query_expander_name_(nullptr),
      default_mode_(GRN_OP_MATCH),
      default_operator_(GRN_OP_AND),
      flags_(GRN_EXPR_SYNTAX_QUERY),
      flags_specified_(-1),
      match_columns_(nullptr) {
    }

    ~BaseQueryExecutor() {
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
      if (!run()) {
        return ctx_->rc;
      }
      return ctx_->rc;
    }

  private:
    virtual bool prepare() = 0;
    virtual bool run() = 0;

  protected:
    bool
    parse_match_columns_arg() {
      match_columns_string_ = args_[0];
      if (!grn_obj_is_text_family_bulk(ctx_, match_columns_string_)) {
        grn::TextBulk inspected(ctx_);
        grn_inspect(ctx_, *inspected, match_columns_string_);
        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "%s 1st argument must be string: <%.*s>",
                         tag_,
                         (int)GRN_TEXT_LEN(*inspected),
                         GRN_TEXT_VALUE(*inspected));
        return false;
      }
      return true;
    }

    bool
    parse_options(grn_obj *options) {
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
        &flags_specified_
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
      if (GRN_TEXT_LEN(match_columns_string_) == 0) {
        return true;
      }

      grn_obj *dummy_variable;
      GRN_EXPR_CREATE_FOR_QUERY(ctx_,
                                table_,
                                match_columns_,
                                dummy_variable);
      if (!match_columns_) {
        return false;
      }

      grn_expr_parse(ctx_,
                     match_columns_,
                     GRN_TEXT_VALUE(match_columns_string_),
                     GRN_TEXT_LEN(match_columns_string_),
                     NULL,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT);
      return ctx_->rc == GRN_SUCCESS;
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

    grn_ctx *ctx_;
    grn_obj *table_;
    int n_args_;
    grn_obj **args_;
    grn_obj *res_;
    grn_operator op_;
    grn_selector_data *selector_data_;
    const char *tag_;

    grn_obj *match_columns_string_;
    grn_obj *query_expander_name_;
    grn_operator default_mode_;
    grn_operator default_operator_;
    grn_expr_flags flags_;
    grn_expr_flags flags_specified_;
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
      query_(nullptr),
      condition_(nullptr) {
    }

    ~QueryExecutor() {
      if (condition_) {
        grn_obj_close(ctx_, condition_);
      }
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
      if (!prepare_condition()) {
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
    prepare_condition() {
      if (GRN_TEXT_LEN(query_) == 0) {
        return true;
      }

      grn_obj *dummy_variable;
      GRN_EXPR_CREATE_FOR_QUERY(ctx_,
                                table_,
                                condition_,
                                dummy_variable);
      if (!condition_) {
        return false;
      }

      grn::TextBulk expanded_query(ctx_);
      if (!expand_query(query_, expanded_query)) {
        return false;
      }
      grn_expr_parse(ctx_,
                     condition_,
                     GRN_TEXT_VALUE(*expanded_query),
                     GRN_TEXT_LEN(*expanded_query),
                     match_columns_,
                     default_mode_,
                     default_operator_,
                     flags_);
      return ctx_->rc == GRN_SUCCESS;
    }

    bool
    run() override {
      if (!condition_) {
        return true;
      }
      grn_table_select(ctx_, table_, condition_, res_, op_);
      return ctx_->rc == GRN_SUCCESS;
    }

    grn_obj *query_;
    grn_obj *condition_;
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

    ~QueryParallelOrExecutor() {
      GRN_OBJ_FIN(ctx_, &sub_match_columns_);
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
#ifdef GRN_WITH_APACHE_ARROW
      if (match_columns_) {
        grn_expr_match_columns_split(ctx_, match_columns_, &sub_match_columns_);
      } else {
        GRN_PTR_PUT(ctx_, &sub_match_columns_, NULL);
      }
#endif
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
      std::mutex mutex;
      std::vector<grn::UniqueObj> unique_sub_results;
      auto select = [&](grn_obj *match_columns, const std::string &query) {
        auto sub_ctx = grn_ctx_pull_child(ctx_);
        grn_obj *condition = build_condition(sub_ctx,
                                             match_columns,
                                             query.data(),
                                             query.length());
        grn_obj *sub_result = nullptr;
        if (condition) {
          grn::UniqueObj unique_condition(sub_ctx, condition);
          {
            std::lock_guard<std::mutex> lock(mutex);
            auto n_unique_sub_results = unique_sub_results.size();
            if (n_unique_sub_results == 0) {
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
            }
          }
          if (sub_result) {
            grn_table_select(sub_ctx,
                             table_,
                             condition,
                             sub_result,
                             GRN_OP_OR);
            {
              std::lock_guard<std::mutex> lock(mutex);
              unique_sub_results.emplace_back(ctx_, sub_result);
            }
          }
        }
        grn_ctx_release_child(ctx_, sub_ctx);
        return ::arrow::Status::OK();
      };

      {
        std::vector<::arrow::Future<>> futures;
        auto n_sub_match_columns = GRN_PTR_VECTOR_SIZE(&sub_match_columns_);
        for (size_t i = 0; i < n_sub_match_columns; ++i) {
          grn_obj *sub_match_column = GRN_PTR_VALUE_AT(&sub_match_columns_, i);
          for (const auto &expanded_query : expanded_queries_) {
            auto future = pool->Submit(select,
                                       sub_match_column,
                                       expanded_query);
            if (!future.ok()) {
              std::lock_guard<std::mutex> lock(mutex);
              if (!grnarrow::check(ctx_,
                                   future,
                                   "%s failed to submit a job",
                                   tag_)) {
                break;
              }
            }
            futures.push_back(*future);
          }
          if (ctx_->rc != GRN_SUCCESS) {
            break;
          }
        }
        auto status = ::arrow::Status::OK();
        for (auto& future : futures) {
          status &= future.status();
        }
        if (!grnarrow::check(ctx_,
                             status,
                             "%s failed to complete a job",
                             tag_)) {
          return false;
        }
      }
      if (ctx_->rc != GRN_SUCCESS) {
        return false;
      }
      auto sub_result = unique_sub_results[0].get();
      auto n_sub_results = unique_sub_results.size();
      for (size_t i = 1; i < n_sub_results; ++i) {
        grn_table_setoperation(ctx_,
                               sub_result,
                               unique_sub_results[i].get(),
                               sub_result,
                               GRN_OP_OR);
        if (ctx_->rc != GRN_SUCCESS) {
          break;
        }
      }
      if (ctx_->rc == GRN_SUCCESS) {
        grn_table_setoperation(ctx_, res_, sub_result, res_, op_);
      }
      return ctx_->rc == GRN_SUCCESS;
    }
#endif

    bool
    select_sequential() {
      grn::UniqueObj unique_sub_result(ctx_, nullptr);
      for (const auto &expanded_query : expanded_queries_) {
        grn_obj *condition = build_condition(ctx_,
                                             match_columns_,
                                             expanded_query.data(),
                                             expanded_query.length());
        if (!condition) {
          return false;
        }
        grn::UniqueObj unique_condition(ctx_, condition);
        auto sub_result = grn_table_select(ctx_,
                                           table_,
                                           condition,
                                           unique_sub_result.get(),
                                           GRN_OP_OR);
        if (ctx_->rc != GRN_SUCCESS) {
          return false;
        }
        unique_sub_result.reset(sub_result);
      }
      grn_table_setoperation(ctx_, res_, unique_sub_result.get(), res_, op_);
      return true;
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
      return condition;
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
