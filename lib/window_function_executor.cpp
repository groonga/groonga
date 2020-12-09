/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019-2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx.h"
#include "grn_window_function_executor.h"

#ifdef GRN_WITH_APACHE_ARROW
# include "grn_arrow.hpp"
# include <groonga/arrow.hpp>
# include <arrow/compute/api.h>
# include <arrow/util/make_unique.h>
#endif

extern "C" {
static grn_bool grn_window_function_executor_all_tables_at_once_enable = true;

void
grn_window_function_executor_init_from_env(void)
{
  {
    char grn_window_function_executor_all_tables_at_once_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_WINDOW_FUNCTION_EXECUTOR_ALL_TABLES_AT_ONCE_ENABLE",
               grn_window_function_executor_all_tables_at_once_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_window_function_executor_all_tables_at_once_enable_env, "no") == 0) {
      grn_window_function_executor_all_tables_at_once_enable = false;
    } else {
      grn_window_function_executor_all_tables_at_once_enable = true;
    }
  }
}

grn_rc
grn_window_function_executor_init(grn_ctx *ctx,
                                  grn_window_function_executor *executor)
{
  GRN_API_ENTER;

  GRN_TEXT_INIT(&(executor->tag), 0);
  GRN_PTR_INIT(&(executor->tables), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_BOOL_INIT(&(executor->is_context_tables), 0);
  GRN_TEXT_INIT(&(executor->source), 0);
  GRN_TEXT_INIT(&(executor->sort_keys), 0);
  GRN_TEXT_INIT(&(executor->group_keys), 0);
  GRN_TEXT_INIT(&(executor->output_column_name), 0);
  executor->context.sort_keys = NULL;
  executor->context.n_sort_keys = 0;
  executor->context.group_keys = NULL;
  executor->context.n_group_keys = 0;
  executor->context.window_sort_keys = NULL;
  executor->context.n_window_sort_keys = 0;
  executor->context.sorted = NULL;
  executor->values.n = 0;
  executor->values.previous = NULL;
  executor->values.current = NULL;
  GRN_PTR_INIT(&(executor->window_function_calls), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(&(executor->output_columns), GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_window_init(ctx, &(executor->window));

  GRN_API_RETURN(ctx->rc);
}

static void
grn_window_function_executor_rewind(grn_ctx *ctx,
                                    grn_window_function_executor *executor)
{
  grn_obj *window_function_calls = &(executor->window_function_calls);
  const size_t n_calls = GRN_PTR_VECTOR_SIZE(window_function_calls);
  for (size_t i = 0; i < n_calls; i++) {
    grn_obj *window_function_call = GRN_PTR_VALUE_AT(window_function_calls, i);
    if (window_function_call) {
      grn_obj_close(ctx, window_function_call);
    }
  }
  GRN_BULK_REWIND(window_function_calls);

  grn_obj *output_columns = &(executor->output_columns);
  const size_t n_output_columns = GRN_PTR_VECTOR_SIZE(output_columns);
  for (size_t i = 0; i < n_output_columns; i++) {
    grn_obj *output_column = GRN_PTR_VALUE_AT(output_columns, i);
    if (grn_obj_is_accessor(ctx, output_column)) {
      grn_obj_close(ctx, output_column);
    }
  }
  GRN_BULK_REWIND(output_columns);
}

grn_rc
grn_window_function_executor_fin(grn_ctx *ctx,
                                 grn_window_function_executor *executor)
{
  GRN_API_ENTER;

  if (!executor) {
    GRN_API_RETURN(GRN_SUCCESS);
  }

  grn_window_fin(ctx, &(executor->window));

  grn_window_function_executor_rewind(ctx, executor);
  GRN_OBJ_FIN(ctx, &(executor->output_columns));
  GRN_OBJ_FIN(ctx, &(executor->window_function_calls));

  if (executor->values.n > 0) {
    for (size_t i = 0; i < executor->values.n; i++) {
      GRN_OBJ_FIN(ctx, &(executor->values.previous[i]));
      GRN_OBJ_FIN(ctx, &(executor->values.current[i]));
    }
    GRN_FREE(executor->values.previous);
    GRN_FREE(executor->values.current);
  }

  if (executor->context.sorted) {
    grn_obj_close(ctx, executor->context.sorted);
  }
  if (executor->context.window_sort_keys) {
    GRN_FREE(executor->context.window_sort_keys);
  }
  if (executor->context.group_keys) {
    grn_table_sort_key_close(ctx,
                             executor->context.group_keys,
                             executor->context.n_group_keys);
  }
  if (executor->context.sort_keys) {
    grn_table_sort_key_close(ctx,
                             executor->context.sort_keys,
                             executor->context.n_sort_keys);
  }

  GRN_OBJ_FIN(ctx, &(executor->output_column_name));
  GRN_OBJ_FIN(ctx, &(executor->group_keys));
  GRN_OBJ_FIN(ctx, &(executor->sort_keys));
  GRN_OBJ_FIN(ctx, &(executor->source));
  GRN_OBJ_FIN(ctx, &(executor->is_context_tables));
  GRN_OBJ_FIN(ctx, &(executor->tables));
  GRN_OBJ_FIN(ctx, &(executor->tag));

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_window_function_executor *
grn_window_function_executor_open(grn_ctx *ctx)
{
  GRN_API_ENTER;

  grn_window_function_executor *executor;
  executor = static_cast<grn_window_function_executor *>(
    GRN_CALLOC(sizeof(grn_window_function_executor)));
  if (!executor) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc,
        "[window-function-executor][open] failed to allocate: %s",
        errbuf);
    GRN_API_RETURN(NULL);
  }

  grn_window_function_executor_init(ctx, executor);

  if (ctx->rc != GRN_SUCCESS) {
    GRN_FREE(executor);
    executor = NULL;
  }

  GRN_API_RETURN(executor);
}

grn_rc
grn_window_function_executor_close(grn_ctx *ctx,
                                   grn_window_function_executor *executor)
{
  GRN_API_ENTER;

  if (!executor) {
    GRN_API_RETURN(GRN_SUCCESS);
  }

  grn_window_function_executor_fin(ctx, executor);
  GRN_FREE(executor);

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_window_function_executor_set_tag(grn_ctx *ctx,
                                     grn_window_function_executor *executor,
                                     const char *tag,
                                     size_t tag_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "[window-function-executor][tag][set] executor is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->tag), tag, tag_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_add_table(grn_ctx *ctx,
                                       grn_window_function_executor *executor,
                                       grn_obj *table)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][table][add] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_function_executor_rewind(ctx, executor);

  GRN_PTR_PUT(ctx, &(executor->tables), table);
  GRN_BOOL_PUT(ctx, &(executor->is_context_tables), false);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_add_context_table(grn_ctx *ctx,
                                               grn_window_function_executor *executor,
                                               grn_obj *table)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][context-table][add] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_function_executor_rewind(ctx, executor);

  GRN_PTR_PUT(ctx, &(executor->tables), table);
  GRN_BOOL_PUT(ctx, &(executor->is_context_tables), true);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_source(grn_ctx *ctx,
                                        grn_window_function_executor *executor,
                                        const char *source,
                                        size_t source_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][source][set] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->source), source, source_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_sort_keys(grn_ctx *ctx,
                                           grn_window_function_executor *executor,
                                           const char *sort_keys,
                                           size_t sort_keys_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][sort-keys][set] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->sort_keys), sort_keys, sort_keys_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_group_keys(grn_ctx *ctx,
                                            grn_window_function_executor *executor,
                                            const char *group_keys,
                                            size_t group_keys_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][group-keys][set] executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->group_keys), group_keys, group_keys_size);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_window_function_executor_set_output_column_name(grn_ctx *ctx,
                                                    grn_window_function_executor *executor,
                                                    const char *name,
                                                    size_t name_size)
{
  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s[window-function-executor][output-column-name][set] "
        "executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)));
    GRN_API_RETURN(ctx->rc);
  }

  GRN_TEXT_SET(ctx, &(executor->output_column_name), name, name_size);

  GRN_API_RETURN(ctx->rc);
}

static bool
grn_window_function_executor_is_ascending(grn_ctx *ctx,
                                          grn_window_function_executor *executor)
{
  const size_t sort_keys_len = GRN_TEXT_LEN(&(executor->sort_keys));
  if (sort_keys_len == 0) {
    return true;
  }

  const char *sort_keys = GRN_TEXT_VALUE(&(executor->sort_keys));
  for (size_t i = 0; i < sort_keys_len; i++) {
    switch (sort_keys[i]) {
    case ' ' :
      break;
    case '-' :
      return false;
    default :
      return true;
    }
  }

  return true;
}

#if ARROW_VERSION_MAJOR >= 3
namespace {
  struct GrnSortKeys {
    grn_table_sort_key *keys_;
    size_t n_keys_;
    GrnSortKeys(grn_table_sort_key *keys,
                size_t n_keys) : keys_(keys),
                                 n_keys_(n_keys) {
    }
  };

  constexpr uint64_t pack_source_id(uint32_t table_index, grn_id record_id) {
    return (static_cast<uint64_t>(table_index) << 32) | record_id;
  }

  constexpr uint32_t unpack_source_id_table_index(uint64_t source_id) {
    return source_id >> 32;
  }

  constexpr uint32_t unpack_source_id_record_id(uint64_t source_id) {
    return source_id & ((1UL << 32) - 1);
  }

  class AllTablesExecutor {
  public:
    AllTablesExecutor(grn_ctx *ctx,
                      grn_window_function_executor *executor,
                      const char *tag)
      : ctx_(ctx),
        executor_(executor),
        executor_tag_(GRN_TEXT_VALUE(&(executor->tag)),
                      GRN_TEXT_LEN(&(executor->tag))),

        tag_(tag),
        sort_keys_builders_(),
        group_keys_builders_(),
        arrow_sort_keys_() {
    }

    void execute() {
      auto ctx = ctx_;

      auto record_batch = build_record_batch();
      if (!record_batch) {
        return;
      }
      ::arrow::compute::SortOptions options(arrow_sort_keys_);
      ::arrow::Datum datum(record_batch);
      auto sort_result = ::arrow::compute::CallFunction("sort_indices",
                                                        {datum},
                                                        &options);
      if (!grnarrow::check(ctx_,
                           sort_result,
                           executor_tag_,
                           tag_,
                           " failed to sort")) {
        return;
      }

      grn_window_set_is_sorted(ctx_,
                               &(executor_->window),
                               !sort_keys_builders_.empty());

      auto source_ids =
        std::static_pointer_cast<::arrow::UInt64Array>(record_batch->column(0));
      auto raw_source_ids = source_ids->raw_values();
      auto indices =
        std::static_pointer_cast<::arrow::UInt64Array>((*sort_result).make_array());
      auto n_rows = indices->length();
      auto raw_indices = indices->raw_values();

      auto n_group_keys = group_keys_builders_.size();
      if (n_group_keys == 0) {
        for (int64_t i = 0; i < n_rows; ++i) {
          const auto index = raw_indices[i];
          const auto source_id = raw_source_ids[index];
          const auto table_index = unpack_source_id_table_index(source_id);
          const auto record_id = unpack_source_id_record_id(source_id);
          auto table = GRN_PTR_VALUE_AT(&(executor_->tables), table_index);
          const auto is_context_table =
            GRN_BOOL_VALUE_AT(&(executor_->is_context_tables), table_index);
          auto window_function_call =
            GRN_PTR_VALUE_AT(&(executor_->window_function_calls), table_index);
          auto output_column =
            GRN_PTR_VALUE_AT(&(executor_->output_columns), table_index);
          grn_window_add_record(ctx_,
                                &(executor_->window),
                                table,
                                is_context_table,
                                record_id,
                                window_function_call,
                                output_column);
          if (ctx_->rc != GRN_SUCCESS) {
            break;
          }
        }
        if (ctx_->rc == GRN_SUCCESS &&
            !grn_window_is_empty(ctx, &(executor_->window))) {
          grn_window_execute(ctx, &(executor_->window));
          grn_window_reset(ctx, &(executor_->window));
        }
        return;
      }

      if (executor_->values.n == 0) {
        executor_->values.n = n_group_keys;
        executor_->values.previous = GRN_MALLOCN(grn_obj, n_group_keys);
        executor_->values.current = GRN_MALLOCN(grn_obj, n_group_keys);
        for (size_t i = 0; i < n_group_keys; ++i) {
          GRN_VOID_INIT(&(executor_->values.previous[i]));
          GRN_VOID_INIT(&(executor_->values.current[i]));
        }
      }
      if (n_group_keys != executor_->values.n) {
        ERR(GRN_INVALID_ARGUMENT,
            "%.*s%s the number of group keys in tables is erratic: "
            "<%" GRN_FMT_SIZE ">: <%" GRN_FMT_SIZE ">",
            static_cast<int>(GRN_TEXT_LEN(&(executor_->tag))),
            GRN_TEXT_VALUE(&(executor_->tag)),
            tag_,
            n_group_keys,
            executor_->values.n);
        return;
      }

      for (int64_t i = 0; i < n_rows; ++i) {
        const auto index = raw_indices[i];
        const auto source_id = raw_source_ids[index];
        const auto table_index = unpack_source_id_table_index(source_id);
        const auto record_id = unpack_source_id_record_id(source_id);

        auto table = GRN_PTR_VALUE_AT(&(executor_->tables), table_index);
        const auto is_context_table =
          GRN_BOOL_VALUE_AT(&(executor_->is_context_tables), table_index);
        auto window_function_call =
          GRN_PTR_VALUE_AT(&(executor_->window_function_calls), table_index);
        auto output_column =
          GRN_PTR_VALUE_AT(&(executor_->output_columns), table_index);

        auto is_group_key_changed = false;
        for (size_t j = 0; j < n_group_keys; j++) {
          auto reverse_j = n_group_keys - j - 1;
          auto previous_value = &(executor_->values.previous[reverse_j]);
          auto current_value = &(executor_->values.current[reverse_j]);
          // +1 is for the first "_source_id" column
          auto group_key = record_batch->column(reverse_j + 1).get();

          if (is_group_key_changed) {
            GRN_BULK_REWIND(previous_value);
            grn::arrow::get_value(ctx_, group_key, index, previous_value);
          } else {
            GRN_BULK_REWIND(current_value);
            grn::arrow::get_value(ctx_, group_key, index, current_value);
            if ((GRN_BULK_VSIZE(current_value) !=
                 GRN_BULK_VSIZE(previous_value)) ||
                (memcmp(GRN_BULK_HEAD(current_value),
                        GRN_BULK_HEAD(previous_value),
                        GRN_BULK_VSIZE(current_value)) != 0)) {
              is_group_key_changed = true;
              grn_bulk_write_from(ctx,
                                  previous_value,
                                  GRN_BULK_HEAD(current_value),
                                  0,
                                  GRN_BULK_VSIZE(current_value));
            }
          }
        }

        if (is_group_key_changed &&
            !grn_window_is_empty(ctx, &(executor_->window))) {
          grn_window_execute(ctx, &(executor_->window));
          if (ctx_->rc != GRN_SUCCESS) {
            break;
          }
          grn_window_reset(ctx, &(executor_->window));
        }
        grn_window_add_record(ctx,
                              &(executor_->window),
                              table,
                              is_context_table,
                              record_id,
                              window_function_call,
                              output_column);
        if (ctx_->rc != GRN_SUCCESS) {
          break;
        }
      }
      if (ctx_->rc == GRN_SUCCESS &&
          !grn_window_is_empty(ctx, &(executor_->window))) {
        grn_window_execute(ctx, &(executor_->window));
        grn_window_reset(ctx, &(executor_->window));
      }
    }

  private:
    std::shared_ptr<::arrow::RecordBatch> build_record_batch() {
      std::vector<std::shared_ptr<::arrow::Field>> fields;
      fields.push_back(::arrow::field("_source_id",
                                      ::arrow::uint64(),
                                      false));
      std::unique_ptr<::arrow::ArrayBuilder> id_builder;
      auto status = ::arrow::MakeBuilder(::arrow::default_memory_pool(),
                                         ::arrow::uint64(),
                                         &id_builder);
      if (!grnarrow::check(ctx_,
                           status,
                           executor_tag_,
                           tag_,
                           " failed to make builder for source id")) {
        return nullptr;
      }

      auto id_builder_raw =
        static_cast<::arrow::UInt64Builder *>(id_builder.get());
      std::vector<std::shared_ptr<::arrow::Array>> arrays;
      const auto n_tables = GRN_PTR_VECTOR_SIZE(&(executor_->tables));
      for (size_t i = 0; i < n_tables; ++i) {
        grn_obj *table = GRN_PTR_VALUE_AT(&(executor_->tables), i);
        GRN_TABLE_EACH_BEGIN(ctx_, table, cursor, id) {
          uint64_t source_id = pack_source_id(static_cast<uint32_t>(i), id);
          id_builder_raw->Append(source_id);
        } GRN_TABLE_EACH_END(ctx_, cursor);
        if (!parse_keys(table,
                        &(executor_->group_keys),
                        "group",
                        group_keys_builders_)) {
          return nullptr;
        }
        if (!parse_keys(table,
                        &(executor_->sort_keys),
                        "sort",
                        sort_keys_builders_)) {
          return nullptr;
        }
      }
      auto id_array_result = id_builder_raw->Finish();
      if (!grnarrow::check(ctx_,
                           id_array_result,
                           executor_tag_,
                           tag_,
                           " failed to build source id array")) {
        return nullptr;
      }
      arrays.push_back(*id_array_result);
      if (!build_keys(group_keys_builders_,
                      0,
                      "group",
                      arrays,
                      fields)) {
        return nullptr;
      }
      if (!build_keys(sort_keys_builders_,
                      group_keys_builders_.size(),
                      "sort",
                      arrays,
                      fields)) {
        return nullptr;
      }
      return ::arrow::RecordBatch::Make(::arrow::schema(fields),
                                        arrays[0]->length(),
                                        arrays);
    }

    bool parse_keys(grn_obj *table,
                    grn_obj *text,
                    const char *type_name,
                    std::vector<std::unique_ptr<grn::arrow::ArrayBuilder>>& builders) {
      auto ctx = ctx_;
      if (GRN_TEXT_LEN(text) == 0) {
        return true;
      }
      unsigned int n_sort_keys = 0;
      auto sort_keys = grn_table_sort_key_from_str(ctx_,
                                                   GRN_TEXT_VALUE(text),
                                                   GRN_TEXT_LEN(text),
                                                   table,
                                                   &n_sort_keys);
      if (!sort_keys) {
        ERR(ctx->rc,
            "%.*s%s failed to parse %s keys: <%.*s>",
            static_cast<int>(GRN_TEXT_LEN(&(executor_->tag))),
            GRN_TEXT_VALUE(&(executor_->tag)),
            tag_,
            type_name,
            static_cast<int>(GRN_TEXT_LEN(text)),
            GRN_TEXT_VALUE(text));
        return false;
      }
      for (unsigned int i = 0; i < n_sort_keys; ++i) {
        const auto& sort_key = sort_keys[i];
        if (builders.size() < n_sort_keys) {
          builders.push_back(
            ::arrow::internal::make_unique<grn::arrow::ArrayBuilder>(ctx));
          grn::TextBulk name(ctx);
          grn_obj_to_script_syntax(ctx, sort_key.key, *name);
          arrow_sort_keys_.emplace_back(
            std::string("_") + type_name + "_" + name.value(),
            sort_key.flags & GRN_TABLE_SORT_DESC ?
            ::arrow::compute::SortOrder::Descending :
            ::arrow::compute::SortOrder::Ascending);
        }
        auto& builder = builders[i];
        auto cursor = grn_table_cursor_open(ctx, table,
                                            NULL, 0,
                                            NULL, 0,
                                            0, -1,
                                            GRN_CURSOR_ASCENDING);
        auto status = builder->add_column(sort_key.key, cursor);
        if (!grnarrow::check(ctx,
                             status,
                             executor_tag_,
                             tag_,
                             " failed to add ",
                             type_name,
                             " key values: <",
                             arrow_sort_keys_[i].name,
                             ">")) {
          return false;
        }
        grn_table_cursor_close(ctx, cursor);
      }
      grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
      return true;
    }

    bool build_keys(std::vector<std::unique_ptr<grn::arrow::ArrayBuilder>> &builders,
                    size_t arrow_sort_keys_offset,
                    const char *type_name,
                    std::vector<std::shared_ptr<::arrow::Array>> &arrays,
                    std::vector<std::shared_ptr<::arrow::Field>> &fields) {
      for (size_t i = 0; i < builders.size(); ++i) {
        auto &builder = builders[i];
        const auto &arrow_sort_key =
          arrow_sort_keys_[arrow_sort_keys_offset + i];
        auto array_result = builder->finish();
        if (!grnarrow::check(ctx_,
                             array_result,
                             executor_tag_,
                             tag_,
                             " failed to build ",
                             type_name,
                             " key array: <",
                             arrow_sort_key.name,
                             ">")) {
          return false;
        }
        auto array = *array_result;
        arrays.push_back(array);
        fields.push_back(::arrow::field(arrow_sort_key.name,
                                        array->type(),
                                        false));
      }
      return true;
    }

    grn_ctx *ctx_;
    grn_window_function_executor *executor_;
    ::arrow::util::string_view executor_tag_;
    const char *tag_;
    std::vector<std::unique_ptr<grn::arrow::ArrayBuilder>> sort_keys_builders_;
    std::vector<std::unique_ptr<grn::arrow::ArrayBuilder>> group_keys_builders_;
    std::vector<GrnSortKeys> grn_sort_keys_vector_;
    std::vector<GrnSortKeys> grn_group_keys_vector_;
    std::vector<::arrow::compute::SortKey> arrow_sort_keys_;
  };
}
#endif

static void
grn_window_function_executor_execute_all_tables(
  grn_ctx *ctx,
  grn_window_function_executor *executor,
  const char *tag)
{
#if ARROW_VERSION_MAJOR >= 3
  AllTablesExecutor all_tables_executor(ctx, executor, tag);
  all_tables_executor.execute();
#endif
}

static void
grn_window_function_executor_execute_per_table(
  grn_ctx *ctx,
  grn_window_function_executor *executor,
  const char *tag)
{
  const bool is_ascending =
    grn_window_function_executor_is_ascending(ctx, executor);
  const size_t n_tables = GRN_PTR_VECTOR_SIZE(&(executor->tables));
  bool in_before_context = true;
  bool in_after_context = false;
  for (size_t i = 0; i < n_tables; i++) {
    size_t nth_table = (is_ascending ? i : (n_tables - i - 1));
    grn_obj *table = GRN_PTR_VALUE_AT(&(executor->tables), nth_table);
    const bool is_context_table =
      GRN_BOOL_VALUE_AT(&(executor->is_context_tables), nth_table);
    if (in_before_context) {
      if (!is_context_table) {
        in_before_context = false;
      }
    } else if (!in_after_context) {
      if (is_context_table) {
        in_after_context = true;
      }
    }

    grn_obj *output_column =
      GRN_PTR_VALUE_AT(&(executor->output_columns), nth_table);
    grn_obj *window_function_call =
      GRN_PTR_VALUE_AT(&(executor->window_function_calls), nth_table);

    unsigned int n_sort_keys = 0;
    grn_table_sort_key *sort_keys = NULL;
    if (GRN_TEXT_LEN(&(executor->sort_keys)) > 0) {
      sort_keys =
        grn_table_sort_key_from_str(ctx,
                                    GRN_TEXT_VALUE(&(executor->sort_keys)),
                                    GRN_TEXT_LEN(&(executor->sort_keys)),
                                    table,
                                    &n_sort_keys);
      if (!sort_keys) {
        ERR(ctx->rc,
            "%.*s%s failed to parse sort keys: <%.*s>",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            (int)GRN_TEXT_LEN(&(executor->sort_keys)),
            GRN_TEXT_VALUE(&(executor->sort_keys)));
        return;
      }
      if (executor->context.sort_keys) {
        grn_table_sort_key_close(ctx,
                                 executor->context.sort_keys,
                                 executor->context.n_sort_keys);
      }
      executor->context.sort_keys = sort_keys;
      executor->context.n_sort_keys = n_sort_keys;
    }

    unsigned int n_group_keys = 0;
    grn_table_sort_key *group_keys = NULL;
    if (GRN_TEXT_LEN(&(executor->group_keys)) > 0) {
      group_keys =
        grn_table_sort_key_from_str(ctx,
                                    GRN_TEXT_VALUE(&(executor->group_keys)),
                                    GRN_TEXT_LEN(&(executor->group_keys)),
                                    table,
                                    &n_group_keys);
      if (!group_keys) {
        ERR(ctx->rc,
            "%.*s%s failed to parse group keys: <%.*s>",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            (int)GRN_TEXT_LEN(&(executor->group_keys)),
            GRN_TEXT_VALUE(&(executor->group_keys)));
        return;
      }
      if (executor->context.group_keys) {
        grn_table_sort_key_close(ctx,
                                 executor->context.group_keys,
                                 executor->context.n_group_keys);
      }
      executor->context.group_keys = group_keys;
      executor->context.n_group_keys = n_group_keys;
    }

    const size_t n_window_sort_keys = n_sort_keys + n_group_keys;
    if (executor->context.n_window_sort_keys < n_window_sort_keys) {
      if (executor->context.window_sort_keys) {
        GRN_FREE(executor->context.window_sort_keys);
      }
      executor->context.window_sort_keys =
        GRN_MALLOCN(grn_table_sort_key, n_window_sort_keys);
      if (!executor->context.window_sort_keys) {
        grn_rc rc = ctx->rc;
        char message[GRN_CTX_MSGSIZE];
        if (rc == GRN_SUCCESS) {
          rc = GRN_NO_MEMORY_AVAILABLE;
        }
        grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
        ERR(rc,
            "%.*s%s failed to allocate internal sort keys: %s",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            message);
        return;
      }
      executor->context.n_window_sort_keys = n_window_sort_keys;
    }
    grn_table_sort_key *window_sort_keys = executor->context.window_sort_keys;
    for (size_t j = 0; j < n_group_keys; j++) {
      window_sort_keys[j] = group_keys[j];
    }
    for (size_t j = 0; j < n_sort_keys; j++) {
      window_sort_keys[j + n_group_keys] = sort_keys[j];
    }

    grn_obj *sorted = grn_table_create(ctx,
                                       NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY,
                                       NULL,
                                       table);
    if (!sorted) {
      grn_rc rc = ctx->rc;
      char errbuf[GRN_CTX_MSGSIZE];
      if (rc == GRN_SUCCESS) {
        rc = GRN_NO_MEMORY_AVAILABLE;
      }
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(rc,
          "%.*s%s failed to allocate table to store sorted result: %s",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          errbuf);
      return;
    }
    if (executor->context.sorted) {
      grn_obj_close(ctx, executor->context.sorted);
    }
    executor->context.sorted = sorted;
    grn_table_sort(ctx,
                   table,
                   0, -1,
                   sorted,
                   window_sort_keys,
                   n_window_sort_keys);

    grn_window_set_is_sorted(ctx, &(executor->window), n_sort_keys > 0);
    if (n_group_keys > 0) {
      if (executor->values.n == 0) {
        executor->values.n = n_group_keys;
        executor->values.previous = GRN_MALLOCN(grn_obj, n_group_keys);
        executor->values.current = GRN_MALLOCN(grn_obj, n_group_keys);
        for (size_t j = 0; j < n_group_keys; j++) {
          GRN_VOID_INIT(&(executor->values.previous[j]));
          GRN_VOID_INIT(&(executor->values.current[j]));
        }
      }
      if (n_group_keys != executor->values.n) {
        ERR(GRN_INVALID_ARGUMENT,
            "%.*s%s the number of group keys in tables is erratic: "
            "<%u>: <%" GRN_FMT_SIZE ">",
            (int)GRN_TEXT_LEN(&(executor->tag)),
            GRN_TEXT_VALUE(&(executor->tag)),
            tag,
            n_group_keys,
            executor->values.n);
        return;
      }

      GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
        void *value;
        grn_table_cursor_get_value(ctx, cursor, &value);
        grn_id record_id = *((grn_id *)value);

        bool is_group_key_changed = false;
        for (size_t j = 0; j < n_group_keys; j++) {
          size_t reverse_j = n_group_keys - j - 1;
          grn_obj *previous_value = &(executor->values.previous[reverse_j]);
          grn_obj *current_value = &(executor->values.current[reverse_j]);
          grn_obj *group_key = group_keys[reverse_j].key;

          if (is_group_key_changed) {
            GRN_BULK_REWIND(previous_value);
            grn_obj_get_value(ctx, group_key, record_id, previous_value);
          } else {
            GRN_BULK_REWIND(current_value);
            grn_obj_get_value(ctx, group_key, record_id, current_value);
            if ((GRN_BULK_VSIZE(current_value) !=
                 GRN_BULK_VSIZE(previous_value)) ||
                (memcmp(GRN_BULK_HEAD(current_value),
                        GRN_BULK_HEAD(previous_value),
                        GRN_BULK_VSIZE(current_value)) != 0)) {
              is_group_key_changed = true;
              grn_bulk_write_from(ctx,
                                  previous_value,
                                  GRN_BULK_HEAD(current_value),
                                  0,
                                  GRN_BULK_VSIZE(current_value));
            }
          }
        }

        if (is_group_key_changed &&
            !grn_window_is_empty(ctx, &(executor->window))) {
          if (!in_before_context) {
            grn_window_execute(ctx, &(executor->window));
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
          grn_window_reset(ctx, &(executor->window));
          if (in_after_context) {
            break;
          }
        }
        grn_window_add_record(ctx,
                              &(executor->window),
                              table,
                              is_context_table,
                              record_id,
                              window_function_call,
                              output_column);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      } GRN_TABLE_EACH_END(ctx, cursor);
    } else {
      GRN_TABLE_EACH_BEGIN(ctx, sorted, cursor, id) {
        void *value;
        grn_id record_id;

        grn_table_cursor_get_value(ctx, cursor, &value);
        record_id = *((grn_id *)value);
        grn_window_add_record(ctx,
                              &(executor->window),
                              table,
                              is_context_table,
                              record_id,
                              window_function_call,
                              output_column);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      } GRN_TABLE_EACH_END(ctx, cursor);
    }
  }

  if (ctx->rc == GRN_SUCCESS &&
      !grn_window_is_empty(ctx, &(executor->window))) {
    grn_window_execute(ctx, &(executor->window));
    grn_window_reset(ctx, &(executor->window));
  }
}

grn_rc
grn_window_function_executor_execute(grn_ctx *ctx,
                                     grn_window_function_executor *executor)
{
  const char *tag = "[window-function-executor][execute]";

  GRN_API_ENTER;

  if (!executor) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s%s executor is NULL",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)),
        tag);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj *source = &(executor->source);
  if (GRN_TEXT_LEN(source) == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s%s no source",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)),
        tag);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj *output_column_name = &(executor->output_column_name);
  if (GRN_TEXT_LEN(output_column_name) == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%.*s%s no output column",
        (int)GRN_TEXT_LEN(&(executor->tag)),
        GRN_TEXT_VALUE(&(executor->tag)),
        tag);
    GRN_API_RETURN(ctx->rc);
  }

  const size_t n_tables = GRN_PTR_VECTOR_SIZE(&(executor->tables));
  if (n_tables == 0) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_window_function_executor_rewind(ctx, executor);

  for (size_t i = 0; i < n_tables; i++) {
    grn_obj *table = GRN_PTR_VALUE_AT(&(executor->tables), i);
    grn_obj *output_column = grn_obj_column(ctx,
                                            table,
                                            GRN_TEXT_VALUE(output_column_name),
                                            GRN_TEXT_LEN(output_column_name));
    if (!output_column) {
      char table_name[GRN_TABLE_MAX_KEY_SIZE];
      int table_name_size;
      table_name_size = grn_obj_name(ctx,
                                     table,
                                     table_name,
                                     GRN_TABLE_MAX_KEY_SIZE);
      if (table_name_size == 0) {
        grn_strcpy(table_name, GRN_TABLE_MAX_KEY_SIZE, "(anonymous)");
        table_name_size = strlen(table_name);
      }
      ERR(GRN_INVALID_ARGUMENT,
          "%.*s%s output column doesn't exist: <%.*s>: <%.*s>",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          table_name_size,
          table_name,
          (int)GRN_TEXT_LEN(output_column_name),
          GRN_TEXT_VALUE(output_column_name));
      GRN_API_RETURN(ctx->rc);
    }
    GRN_PTR_PUT(ctx, &(executor->output_columns), output_column);

    grn_obj *window_function_call;
    grn_obj *record;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, window_function_call, record);
    if (!window_function_call) {
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(GRN_INVALID_ARGUMENT,
          "%.*s%s failed to create expression to compile window function call: "
          "%s",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          message);
      GRN_API_RETURN(ctx->rc);
    }
    GRN_PTR_PUT(ctx, &(executor->window_function_calls), window_function_call);
    grn_expr_parse(ctx,
                   window_function_call,
                   GRN_TEXT_VALUE(source),
                   GRN_TEXT_LEN(source),
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(ctx->rc,
          "%.*s%s failed to parse window function call: <%.*s>: %s",
          (int)GRN_TEXT_LEN(&(executor->tag)),
          GRN_TEXT_VALUE(&(executor->tag)),
          tag,
          (int)GRN_TEXT_LEN(source),
          GRN_TEXT_VALUE(source),
          message);
      GRN_API_RETURN(ctx->rc);
    }
  }

  bool process_all_tables_at_once = false;
#if defined(GRN_WITH_APACHE_ARROW) && ARROW_VERSION_MAJOR >= 3
  process_all_tables_at_once =
    grn_window_function_executor_all_tables_at_once_enable &&
    (n_tables > 1);
#endif
  if (process_all_tables_at_once) {
    grn_window_function_executor_execute_all_tables(ctx, executor, tag);
  } else {
    grn_window_function_executor_execute_per_table(ctx, executor, tag);
  }

  GRN_API_RETURN(ctx->rc);
}
}
