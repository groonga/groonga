/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2023  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_cache.h"
#include "../grn_ctx.hpp"
#include "../grn_ctx_impl.h"
#include "../grn_expr.h"
#include "../grn_group.h"
#include "../grn_ii.h"
#include "../grn_output.h"
#include "../grn_posting.h"
#include "../grn_proc.h"
#include "../grn_str.h"
#include "../grn_table_selector.h"
#include "../grn_util.h"
#include "../grn_window_function_executor.h"

#include <groonga/plugin.h>
#include <groonga.hpp>

#include <atomic>
#include <unordered_map>
#include <vector>

#ifdef GRN_WITH_APACHE_ARROW
#  include "../grn_arrow.hpp"
#  include <arrow/util/thread_pool.h>
#  include <mutex>
#  include <unordered_map>
#endif

#define GRN_SELECT_INTERNAL_VAR_MATCH_COLUMNS "$match_columns"

static int32_t grn_select_n_workers_default = 0;

extern "C" void
grn_proc_select_init_from_env(void)
{
  {
    char env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_SELECT_N_WORKERS_DEFAULT", env, GRN_ENV_BUFFER_SIZE);
    if (env[0]) {
      grn_select_n_workers_default = atoi(env);
    }
  }
}

namespace {
  enum class DynamicColumnStage;
  struct DynamicColumns;
  class Drilldowns;
  struct Filter;
  struct Slice;
  class Slices;
  struct SelectExecutor;
} // namespace

static void
grn_select_apply_dynamic_columns(grn_ctx *ctx,
                                 SelectExecutor *data,
                                 grn_obj *table,
                                 DynamicColumnStage stage,
                                 grn_hash *dynamic_columns,
                                 grn_obj *condition,
                                 const char *log_tag_prefix,
                                 const char *query_log_tag_prefix);

static grn_bool
grn_select_apply_adjuster(grn_ctx *ctx,
                          SelectExecutor *data,
                          grn_raw_string *adjuster_string,
                          grn_obj *target,
                          grn_obj *result,
                          const char *log_tag_prefix,
                          const char *query_log_tag_prefix);

static grn_obj *
grn_select_create_all_selected_result_table(grn_ctx *ctx, grn_obj *table);

static bool
grn_filter_execute(grn_ctx *ctx,
                   SelectExecutor *data,
                   Filter *filter,
                   grn_obj *table,
                   DynamicColumns *dynamic_columns,
                   const char *log_tag_prefix,
                   const char *query_log_tag_prefix);

static bool
grn_filter_execute_post_filter(grn_ctx *ctx,
                               SelectExecutor *data,
                               Filter *filter,
                               grn_obj *table,
                               const char *log_tag_prefix,
                               const char *query_log_tag_prefix);

namespace {
  template <typename Class>
  void
  close_objects(grn_ctx *ctx, grn_hash *objects)
  {
    GRN_HASH_EACH_BEGIN(ctx, objects, cursor, id)
    {
      void *value;
      grn_hash_cursor_get_value(ctx, cursor, &value);
      auto object = static_cast<Class *>(value);
      object->~Class();
    }
    GRN_HASH_EACH_END(ctx, cursor);
    grn_hash_close(ctx, objects);
  }

  enum class DynamicColumnStage {
    INITIAL,
    RESULT_SET,
    FILTERED,
    OUTPUT,
    GROUP,
  };

  const char *
  grn_dynamic_column_stage_name(DynamicColumnStage stage)
  {
    switch (stage) {
    case DynamicColumnStage::INITIAL:
      return "initial";
    case DynamicColumnStage::RESULT_SET:
      return "result_set";
    case DynamicColumnStage::FILTERED:
      return "filtered";
    case DynamicColumnStage::OUTPUT:
      return "output";
    case DynamicColumnStage::GROUP:
      return "group";
    default:
      return "unknown";
    }
  }

  struct DynamicColumn {
    DynamicColumn(grn_ctx *ctx, grn_raw_string *name, DynamicColumnStage stage)
      : ctx_(ctx),
        label(*name),
        stage(stage),
        type(grn_ctx_at(ctx, GRN_DB_TEXT)),
        flags(GRN_OBJ_COLUMN_SCALAR),
        value({nullptr, 0}),
        window({{nullptr, 0}, {nullptr, 0}}),
        dependency_names()
    {
      GRN_TEXT_INIT(&dependency_names, GRN_OBJ_VECTOR);
    }

    ~DynamicColumn()
    {
      if (grn_obj_is_table(ctx_, type)) {
        grn_obj_unref(ctx_, type);
      }
      GRN_OBJ_FIN(ctx_, &dependency_names);
    }

    bool
    fill(grn::CommandArguments *args, const char *prefix)
    {
      char name_prefix[GRN_TABLE_MAX_KEY_SIZE];
      grn_snprintf(name_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%scolumns[%.*s].",
                   prefix ? prefix : "",
                   static_cast<int>(label.length),
                   label.value);
      char fallback_name_prefix[GRN_TABLE_MAX_KEY_SIZE];
      grn_snprintf(fallback_name_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%scolumn[%.*s].",
                   prefix ? prefix : "",
                   static_cast<int>(label.length),
                   label.value);

      auto type_name =
        args->get_string(name_prefix, fallback_name_prefix, "type", nullptr);
      if (type_name.length > 0) {
        auto new_type = grn_ctx_get(ctx_,
                                    type_name.value,
                                    static_cast<int>(type_name.length));
        if (!new_type) {
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_INVALID_ARGUMENT,
                           "[select][columns][%s][%.*s] unknown type: <%.*s>",
                           grn_dynamic_column_stage_name(stage),
                           static_cast<int>(label.length),
                           label.value,
                           static_cast<int>(type_name.length),
                           type_name.value);
          return false;
        }
        if (!(grn_obj_is_type(ctx_, new_type) ||
              grn_obj_is_table(ctx_, new_type))) {
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          grn_inspect(ctx_, &inspected, new_type);
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_INVALID_ARGUMENT,
                           "[select][columns][%s][%.*s] invalid type: %.*s",
                           grn_dynamic_column_stage_name(stage),
                           static_cast<int>(label.length),
                           label.value,
                           static_cast<int>(GRN_TEXT_LEN(&inspected)),
                           GRN_TEXT_VALUE(&inspected));
          GRN_OBJ_FIN(ctx_, &inspected);
          grn_obj_unlink(ctx_, new_type);
          return false;
        }
        if (type != new_type) {
          if (grn_obj_is_table(ctx_, type)) {
            grn_obj_unref(ctx_, type);
          }
          type = new_type;
        }
      }

      auto flag_names =
        args->get_string(name_prefix, fallback_name_prefix, "flags", nullptr);
      if (flag_names.length > 0) {
        char error_message_tag[GRN_TABLE_MAX_KEY_SIZE];
        grn_snprintf(error_message_tag,
                     GRN_TABLE_MAX_KEY_SIZE,
                     GRN_TABLE_MAX_KEY_SIZE,
                     "[select][columns][%s][%.*s]",
                     grn_dynamic_column_stage_name(stage),
                     static_cast<int>(label.length),
                     label.value);
        flags =
          grn_proc_column_parse_flags(ctx_,
                                      error_message_tag,
                                      flag_names.value,
                                      flag_names.value + flag_names.length);
        if (ctx_->rc != GRN_SUCCESS) {
          return false;
        }
      }

      value =
        args->get_string(name_prefix, fallback_name_prefix, "value", nullptr);
      window.sort_keys = args->get_string(name_prefix,
                                          fallback_name_prefix,
                                          "window.sort_keys",
                                          nullptr);
      window.group_keys = args->get_string(name_prefix,
                                           fallback_name_prefix,
                                           "window.group_keys",
                                           nullptr);

      GRN_BULK_REWIND(&dependency_names);
      if (!extract_dependency_column_names(&window.sort_keys)) {
        return false;
      }
      if (!extract_dependency_column_names(&window.group_keys)) {
        return false;
      }

      return true;
    }

    grn_ctx *ctx_;

    grn_raw_string label;
    DynamicColumnStage stage;
    grn_obj *type;
    grn_column_flags flags;
    grn_raw_string value;
    struct {
      grn_raw_string sort_keys;
      grn_raw_string group_keys;
    } window;
    grn_obj dependency_names;

  private:
    bool
    extract_dependency_column_names(grn_raw_string *keys)
    {
      if (keys->length == 0) {
        return true;
      }

      // TODO: Improve this logic.
      size_t start = 0;
      size_t current;
      for (current = 0; current < keys->length; current++) {
        char c = keys->value[current];
        if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
            ('a' <= c && c <= 'z') || (c == '_') || (c == '.')) {
          continue;
        }
        if (start == current) {
          start++;
        } else {
          grn_vector_add_element(ctx_,
                                 &dependency_names,
                                 keys->value + start,
                                 static_cast<uint32_t>(current - start),
                                 0,
                                 GRN_DB_SHORT_TEXT);
          start = current + 1;
        }
      }

      if (start != current) {
        grn_vector_add_element(ctx_,
                               &dependency_names,
                               keys->value + start,
                               static_cast<uint32_t>(current - start),
                               0,
                               GRN_DB_SHORT_TEXT);
      }

      return true;
    }
  };

  struct DynamicColumns {
  public:
    DynamicColumns(grn_ctx *ctx, grn::CommandArguments *args)
      : ctx_(ctx),
        args_(args),
        initial(nullptr),
        result_set(nullptr),
        filtered(nullptr),
        output(nullptr),
        group(nullptr)
    {
    }

    ~DynamicColumns() { close(); }

    bool
    fill(const char *prefix)
    {
      size_t prefix_length = 0;
      if (prefix) {
        prefix_length += strlen(prefix);
      }
      constexpr auto name_suffix = "].stage";
      for (const auto &arg : *args_) {
        auto name_prefix_length = prefix_length;
        constexpr auto name_prefix = "columns[";
        // For backward compatibility
        constexpr auto name_prefix_compat = "column[";
        if (!GRN_RAW_STRING_START_WITH_CSTRING(arg.name, prefix)) {
          continue;
        }
        auto subname =
          grn_raw_string_substring(ctx_, &(arg.name), prefix_length, -1);
        if (GRN_RAW_STRING_START_WITH_CSTRING(subname, name_prefix)) {
          name_prefix_length += strlen(name_prefix);
        } else if (GRN_RAW_STRING_START_WITH_CSTRING(subname,
                                                     name_prefix_compat)) {
          name_prefix_length += strlen(name_prefix_compat);
        } else {
          continue;
        }
        if (!GRN_RAW_STRING_END_WITH_CSTRING(subname, name_suffix)) {
          continue;
        }

        DynamicColumnStage stage;
        grn_hash **target_columns;
        if (GRN_TEXT_EQUAL_CSTRING(arg.value, "initial")) {
          stage = DynamicColumnStage::INITIAL;
          target_columns = &initial;
        } else if (GRN_TEXT_EQUAL_CSTRING(arg.value, "result_set")) {
          stage = DynamicColumnStage::RESULT_SET;
          target_columns = &result_set;
        } else if (GRN_TEXT_EQUAL_CSTRING(arg.value, "filtered")) {
          stage = DynamicColumnStage::FILTERED;
          target_columns = &filtered;
        } else if (GRN_TEXT_EQUAL_CSTRING(arg.value, "output")) {
          stage = DynamicColumnStage::OUTPUT;
          target_columns = &output;
        } else if (GRN_TEXT_EQUAL_CSTRING(arg.value, "group")) {
          stage = DynamicColumnStage::GROUP;
          target_columns = &group;
        } else {
          continue;
        }

        auto column_name_length =
          arg.name.length - name_prefix_length - strlen(name_suffix);
        auto column_name =
          grn_raw_string_substring(ctx_,
                                   &(arg.name),
                                   name_prefix_length,
                                   static_cast<int64_t>(column_name_length));
        if (!add(prefix, &column_name, stage, target_columns)) {
          return false;
        }
      }
      return true;
    }

    void
    close()
    {
      close_stage(initial);
      close_stage(result_set);
      close_stage(filtered);
      close_stage(output);
      close_stage(group);
    }

  private:
    void
    close_stage(grn_hash *&stage)
    {
      if (!stage) {
        return;
      }
      close_objects<DynamicColumn>(ctx_, stage);
      stage = nullptr;
    }

    bool
    add(const char *prefix,
        grn_raw_string *name,
        DynamicColumnStage stage,
        grn_hash **dynamic_columns)
    {
      if (!*dynamic_columns) {
        *dynamic_columns = grn_hash_create(
          ctx_,
          NULL,
          GRN_TABLE_MAX_KEY_SIZE,
          sizeof(DynamicColumn),
          GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_VAR_SIZE | GRN_HASH_TINY);
      }
      if (!*dynamic_columns) {
        return false;
      }

      void *value;
      int added;
      grn_hash_add(ctx_,
                   *dynamic_columns,
                   name->value,
                   static_cast<unsigned int>(name->length),
                   &value,
                   &added);
      if (added) {
        auto dynamic_column = static_cast<DynamicColumn *>(value);
        new (dynamic_column) DynamicColumn(ctx_, name, stage);
        return dynamic_column->fill(args_, prefix);
      } else {
        return true;
      }
    }

  public:
    grn_ctx *ctx_;
    grn::CommandArguments *args_;

    grn_hash *initial;
    grn_hash *result_set;
    grn_hash *filtered;
    grn_hash *output;
    grn_hash *group;
  };

  struct Drilldown {
    static constexpr int DEFAULT_LIMIT = 10;

    Drilldown(grn_ctx *ctx,
              SelectExecutor *executor,
              grn::CommandArguments *args,
              const char *prefix,
              const char *fallback_prefix,
              grn_raw_string label)
      : ctx_(ctx),
        executor_(executor),
        args_(args),
        prefix_(prefix),
        fallback_prefix_(fallback_prefix),
        label(std::move(label)),
        keys(keys_arg()),
        nth_key(-1),
        sort_keys(
          args_->get_string(prefix_, fallback_prefix_, "sort_keys", "sortby")),
        output_columns(output_columns_arg()),
        offset(
          args_->get_int32(prefix_, fallback_prefix_, "offset", nullptr, 0)),
        limit(args_->get_int32(
          prefix_, fallback_prefix_, "limit", nullptr, DEFAULT_LIMIT)),
        calc_types(calc_types_arg()),
        calc_target_name(
          args_->get_string(prefix_, fallback_prefix_, "calc_target", nullptr)),
        filter(args_->get_string(prefix_, fallback_prefix_, "filter", nullptr)),
        adjuster(
          args_->get_string(prefix_, fallback_prefix_, "adjuster", nullptr)),
        table_name(table_arg()),
        max_n_target_records(args_->get_int32(
          prefix_, fallback_prefix_, "max_n_target_records", nullptr, -1)),
        key_vector_expansion(args_->get_string(
          prefix_, fallback_prefix_, "key_vector_expansion", nullptr)),
        dynamic_columns(ctx, args),
        result(),
        filtered_result(nullptr)
    {
      if (strcmp(prefix_, "drilldown_") != 0) {
        dynamic_columns.fill(prefix_);
        if (fallback_prefix_) {
          dynamic_columns.fill(fallback_prefix_);
        }
      }
    }

    ~Drilldown() { close(); }

    void
    close()
    {
      dynamic_columns.close();

      if (filtered_result) {
        grn_obj_close(ctx_, filtered_result);
        filtered_result = nullptr;
      }

      if (result.table) {
        if (result.calc_target) {
          grn_obj_unlink(ctx_, result.calc_target);
          result.calc_target = nullptr;
        }
        if (result.table) {
          grn_obj_unlink(ctx_, result.table);
          result.table = nullptr;
        }
        if (result.n_aggregators > 0) {
          uint32_t i;
          for (i = 0; i < result.n_aggregators; i++) {
            auto *aggregator = result.aggregators[i];
            grn_table_group_aggregator_close(ctx_, aggregator);
          }
          auto ctx = ctx_;
          GRN_FREE(result.aggregators);
          result.aggregators = nullptr;
          result.n_aggregators = 0;
        }
      }
    }

    bool
    execute(Drilldowns *drilldowns,
            grn_obj *table,
            Slices *slices,
            grn_obj *condition,
            const char *log_tag_context,
            const char *query_log_tag_prefix);

    grn_ctx *ctx_;
    SelectExecutor *executor_;
    grn::CommandArguments *args_;
    const char *prefix_;
    const char *fallback_prefix_;

    grn_raw_string label;
    grn_raw_string keys;
    int32_t nth_key;
    grn_raw_string sort_keys;
    grn_raw_string output_columns;
    int offset;
    int limit;
    grn_table_group_flags calc_types;
    grn_raw_string calc_target_name;
    grn_raw_string filter;
    grn_raw_string adjuster;
    grn_raw_string table_name;
    int32_t max_n_target_records;
    grn_raw_string key_vector_expansion;
    DynamicColumns dynamic_columns;
    grn_table_group_result result;
    grn_obj *filtered_result;

  private:
    grn_raw_string
    keys_arg()
    {
      if (strcmp(prefix_, "drilldown_") == 0) {
        return args_->get_string(nullptr, nullptr, "drilldown");
      } else {
        return args_->get_string(prefix_, fallback_prefix_, "keys", nullptr);
      }
    }

    grn_raw_string
    output_columns_arg()
    {
      grn_raw_string default_output_columns;
      GRN_RAW_STRING_SET_CSTRING(default_output_columns, "_key, _nsubrecs");
      return args_->get_string(prefix_,
                               fallback_prefix_,
                               "output_columns",
                               nullptr,
                               default_output_columns);
    }

    grn_table_group_flags
    parse_table_group_calc_types(grn_raw_string calc_types)
    {
      grn_table_group_flags flags = 0;
      const char *calc_types_end = calc_types.value + calc_types.length;

      while (calc_types.value < calc_types_end) {
        if (calc_types.value[0] == ',' || calc_types.value[0] == ' ') {
          ++(calc_types.value);
          --(calc_types.length);
          continue;
        }

#define CHECK_TABLE_GROUP_CALC_TYPE(name)                                      \
  {                                                                            \
    const char *name_cstring = #name;                                          \
    if (GRN_RAW_STRING_START_WITH_CSTRING(calc_types, name_cstring)) {         \
      flags |= GRN_TABLE_GROUP_CALC_##name;                                    \
      calc_types.value += sizeof(#name) - 1;                                   \
      calc_types.length += sizeof(#name) - 1;                                  \
      continue;                                                                \
    }                                                                          \
  }

        CHECK_TABLE_GROUP_CALC_TYPE(COUNT);
        CHECK_TABLE_GROUP_CALC_TYPE(MAX);
        CHECK_TABLE_GROUP_CALC_TYPE(MIN);
        CHECK_TABLE_GROUP_CALC_TYPE(SUM);
        CHECK_TABLE_GROUP_CALC_TYPE(AVG);
        CHECK_TABLE_GROUP_CALC_TYPE(MEAN);

#define GRN_TABLE_GROUP_CALC_NONE 0
        CHECK_TABLE_GROUP_CALC_TYPE(NONE);
#undef GRN_TABLE_GROUP_CALC_NONE

        GRN_PLUGIN_ERROR(ctx_,
                         GRN_INVALID_ARGUMENT,
                         "invalid table group calc type: <%.*s>",
                         static_cast<int>(calc_types.length),
                         calc_types.value);
        return 0;
#undef CHECK_TABLE_GROUP_CALC_TYPE
      }

      return flags;
    }

    grn_table_group_flags
    calc_types_arg()
    {
      auto calc_types =
        args_->get_string(prefix_, fallback_prefix_, "calc_types", nullptr);
      if (calc_types.length == 0) {
        return 0;
      } else {
        return parse_table_group_calc_types(calc_types);
      }
    }

    grn_raw_string
    table_arg()
    {
      if (strcmp(prefix_, "drilldown_") == 0) {
        return grn_raw_string{nullptr, 0};
      } else {
        return args_->get_string(prefix_, fallback_prefix_, "table", nullptr);
      }
    }

    bool
    execute_internal(grn_ctx *ctx,
                     Drilldowns *drilldowns,
                     grn_obj *table,
                     Slices *slices,
                     grn_obj *condition,
                     const char *log_tag_context,
                     const char *query_log_tag_prefix);
  };

  template <typename Object> class ObjectCursor {
  public:
    ObjectCursor(grn_ctx *ctx, grn_hash_cursor *cursor)
      : ctx_(ctx),
        cursor_(cursor),
        id_(cursor ? grn_hash_cursor_next(ctx_, cursor_) : GRN_ID_NIL)
    {
    }

    ~ObjectCursor()
    {
      if (cursor_) {
        grn_hash_cursor_close(ctx_, cursor_);
      }
    }

    ObjectCursor &
    operator++()
    {
      id_ = grn_hash_cursor_next(ctx_, cursor_);
      return *this;
    }

    Object *
    operator*() const
    {
      void *value;
      grn_hash_cursor_get_value(ctx_, cursor_, &value);
      return static_cast<Object *>(value);
    }

    bool
    operator==(ObjectCursor &other)
    {
      return id_ == other.id_;
    }

    bool
    operator!=(ObjectCursor &other)
    {
      return id_ != other.id_;
    }

  private:
    grn_ctx *ctx_;
    grn_hash_cursor *cursor_;
    grn_id id_;
  };

  class Drilldowns {
  public:
    Drilldowns(grn_ctx *ctx,
               SelectExecutor *executor,
               grn::CommandArguments *args,
               const char *prefix,
               bool need_backward_compatibility,
               const char *log_tag)
      : ctx_(ctx),
        executor_(executor),
        args_(args), // TODO: executor->args()
        prefix_(prefix),
        need_backward_compatibility_(need_backward_compatibility),
        log_tag_(log_tag),
        drilldowns_(nullptr),
        keys_({nullptr, 0}),
        labels_(),
        n_executing_drilldowns_(0),
        n_executed_drilldowns_(0)
    {
      fill();
    }

    ~Drilldowns()
    {
      close();
      for (auto label : labels_) {
        delete label;
      }
    }

    void
    close()
    {
      if (drilldowns_) {
        close_objects<Drilldown>(ctx_, drilldowns_);
        drilldowns_ = nullptr;
      }
    }

    ObjectCursor<Drilldown>
    begin()
    {
      if (drilldowns_) {
        auto cursor = grn_hash_cursor_open(ctx_,
                                           drilldowns_,
                                           NULL,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           -1,
                                           GRN_CURSOR_ASCENDING);
        return ObjectCursor<Drilldown>(ctx_, cursor);
      } else {
        return ObjectCursor<Drilldown>(ctx_, nullptr);
      }
    }

    ObjectCursor<Drilldown>
    end()
    {
      return ObjectCursor<Drilldown>(ctx_, nullptr);
    }

    Drilldown *
    operator[](grn_raw_string &name)
    {
      if (drilldowns_) {
        void *value = nullptr;
        auto id = grn_hash_get(ctx_,
                               drilldowns_,
                               name.value,
                               static_cast<unsigned int>(name.length),
                               &value);
        if (id == GRN_ID_NIL) {
          return nullptr;
        }
        return static_cast<Drilldown *>(value);
      } else {
        return nullptr;
      }
    }

    Drilldown *
    operator[](grn_id id)
    {
      if (drilldowns_) {
        return const_cast<Drilldown *>(reinterpret_cast<const Drilldown *>(
          grn_hash_get_value_(ctx_, drilldowns_, id, nullptr)));
      } else {
        return nullptr;
      }
    }

    bool
    empty()
    {
      return drilldowns_ == nullptr;
    }

    bool
    is_labeled()
    {
      return keys_.length == 0;
    }

    void
    executed(Drilldown *executed_drilldown);

    bool
    execute(grn_obj *table,
            Slices *slices,
            grn_obj *condition,
            const char *log_tag_context,
            const char *query_log_tag_prefix);

  private:
    grn_ctx *ctx_;
    SelectExecutor *executor_;
    grn::CommandArguments *args_;
    const char *prefix_;
    bool need_backward_compatibility_;
    const char *log_tag_;
    grn_hash *drilldowns_;
    grn_raw_string keys_;
    std::vector<std::string *> labels_;
    uint32_t n_executing_drilldowns_;
    std::atomic<uint32_t> n_executed_drilldowns_;

    void
    fill()
    {
      if (!prefix_) {
        keys_ = args_->get_string("drilldown");
        if (keys_.length > 0) {
          return;
        }
      }

      size_t prefix_length = 0;
      if (prefix_) {
        prefix_length = strlen(prefix_);
      }
      for (const auto &arg : *args_) {
        auto name_prefix_length = prefix_length;
        constexpr auto name_prefix = "drilldowns[";
        // For backward compatibility
        constexpr auto name_prefix_compat = "drilldown[";
        if (!GRN_RAW_STRING_START_WITH_CSTRING(arg.name, prefix_)) {
          continue;
        }
        auto subname =
          grn_raw_string_substring(ctx_, &(arg.name), prefix_length, -1);
        if (GRN_RAW_STRING_START_WITH_CSTRING(subname, name_prefix)) {
          name_prefix_length += strlen(name_prefix);
        } else if (need_backward_compatibility_ &&
                   GRN_RAW_STRING_START_WITH_CSTRING(subname,
                                                     name_prefix_compat)) {
          name_prefix_length += strlen(name_prefix_compat);
        } else {
          continue;
        }
        auto label_end = memchr(subname.value, ']', subname.length);
        if (!label_end) {
          continue;
        }
        auto label_length =
          static_cast<size_t>(static_cast<const char *>(label_end) -
                              arg.name.value) -
          name_prefix_length;
        auto label =
          grn_raw_string_substring(ctx_,
                                   &(arg.name),
                                   name_prefix_length,
                                   static_cast<int64_t>(label_length));
        char full_name_prefix[GRN_TABLE_MAX_KEY_SIZE];
        grn_snprintf(full_name_prefix,
                     GRN_TABLE_MAX_KEY_SIZE,
                     GRN_TABLE_MAX_KEY_SIZE,
                     "%sdrilldowns[%.*s].",
                     prefix_ ? prefix_ : "",
                     static_cast<int>(label.length),
                     label.value);
        char *full_fallback_name_prefix = nullptr;
        char full_fallback_name_prefix_buffer[GRN_TABLE_MAX_KEY_SIZE];
        if (need_backward_compatibility_) {
          grn_snprintf(full_fallback_name_prefix_buffer,
                       GRN_TABLE_MAX_KEY_SIZE,
                       GRN_TABLE_MAX_KEY_SIZE,
                       "%sdrilldown[%.*s].",
                       prefix_ ? prefix_ : "",
                       static_cast<int>(label.length),
                       label.value);
          full_fallback_name_prefix = full_fallback_name_prefix_buffer;
        }
        if (!add(full_name_prefix, full_fallback_name_prefix, &label)) {
          return;
        }
      }
    }

    Drilldown *
    add(const char *prefix, const char *fallback_prefix, grn_raw_string *label)
    {
      if (!drilldowns_) {
        drilldowns_ = grn_hash_create(ctx_,
                                      NULL,
                                      GRN_TABLE_MAX_KEY_SIZE,
                                      sizeof(Drilldown),
                                      GRN_OBJ_TABLE_HASH_KEY |
                                        GRN_OBJ_KEY_VAR_SIZE | GRN_HASH_TINY);
        if (!drilldowns_) {
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_INVALID_ARGUMENT,
                           "[select]%s[drilldowns] "
                           "failed to allocate drilldowns data: %s",
                           log_tag_,
                           ctx_->errbuf);
          return nullptr;
        }
      }

      void *value;
      int added;
      grn_hash_add(ctx_,
                   drilldowns_,
                   label->value,
                   static_cast<unsigned int>(label->length),
                   &value,
                   &added);
      auto drilldown = static_cast<Drilldown *>(value);
      if (added) {
        new (drilldown)
          Drilldown(ctx_, executor_, args_, prefix, fallback_prefix, *label);
        if (ctx_->rc != GRN_SUCCESS) {
          return nullptr;
        }
      }
      return drilldown;
    }

    bool
    process_keys(grn_obj *table)
    {
      if (keys_.length == 0) {
        return true;
      }

      uint32_t n_parsed_keys;
      auto parsed_keys =
        grn_table_group_keys_parse(ctx_,
                                   table,
                                   keys_.value,
                                   static_cast<int32_t>(keys_.length),
                                   &n_parsed_keys);
      if (!parsed_keys) {
        return false;
      }

      grn::TextBulk buffer(ctx_);
      for (uint32_t i = 0; i < n_parsed_keys; i++) {
        auto key = parsed_keys[i].key;
        if (grn_obj_is_expr(ctx_, key)) {
          grn_expr_to_script_syntax(ctx_, key, *buffer);
        } else {
          char name[GRN_TABLE_MAX_KEY_SIZE];
          int name_len;
          if (grn_obj_is_accessor(ctx_, key)) {
            grn_accessor *a = reinterpret_cast<grn_accessor *>(key);
            while (a->next) {
              a = a->next;
            }
            key = a->obj;
          }
          if (grn_obj_is_column(ctx_, key)) {
            name_len = grn_column_name(ctx_, key, name, GRN_TABLE_MAX_KEY_SIZE);
          } else {
            name_len = grn_obj_name(ctx_, key, name, GRN_TABLE_MAX_KEY_SIZE);
          }
          GRN_TEXT_SET(ctx_, *buffer, name, name_len);
        }
        labels_.push_back(
          new std::string(GRN_TEXT_VALUE(*buffer), GRN_TEXT_LEN(*buffer)));
        grn_raw_string label = {labels_[i]->data(), labels_[i]->length()};
        auto drilldown = add("drilldown_", nullptr, &label);
        if (!drilldown) {
          continue;
        }
        drilldown->nth_key = static_cast<int32_t>(i);
      }
      grn_table_sort_key_close(ctx_, parsed_keys, n_parsed_keys);

      return true;
    }

    enum class TSortStatus {
      NOT_VISITED,
      VISITING,
      VISITED,
    };

    bool
    tsort(std::vector<grn_id> &ids)
    {
      std::vector<TSortStatus> statuses(grn_hash_size(ctx_, drilldowns_),
                                        TSortStatus::NOT_VISITED);
      bool succeeded = true;
      GRN_HASH_EACH_BEGIN(ctx_, drilldowns_, cursor, id)
      {
        if (tsort_visit(statuses, ids, id)) {
          succeeded = false;
          break;
        }
      }
      GRN_HASH_EACH_END(ctx_, cursor);
      return succeeded;
    }

    bool
    tsort_visit(std::vector<TSortStatus> &statuses,
                std::vector<grn_id> &ids,
                grn_id id)
    {
      bool cycled = true;
      uint32_t index = id - 1;

      switch (statuses[index]) {
      case TSortStatus::VISITING:
        cycled = true;
        break;
      case TSortStatus::VISITED:
        cycled = false;
        break;
      case TSortStatus::NOT_VISITED:
        cycled = false;
        statuses[index] = TSortStatus::VISITING;
        {
          auto drilldown = reinterpret_cast<const Drilldown *>(
            grn_hash_get_value_(ctx_, drilldowns_, id, nullptr));
          if (drilldown->table_name.length > 0) {
            auto dependent_id = grn_hash_get(
              ctx_,
              drilldowns_,
              drilldown->table_name.value,
              static_cast<unsigned int>(drilldown->table_name.length),
              nullptr);
            if (dependent_id != GRN_ID_NIL) {
              cycled = tsort_visit(statuses, ids, dependent_id);
              if (cycled) {
                GRN_PLUGIN_ERROR(ctx_,
                                 GRN_INVALID_ARGUMENT,
                                 "[select][drilldowns][%.*s][table] "
                                 "cycled dependency: <%.*s>",
                                 static_cast<int>(drilldown->label.length),
                                 drilldown->label.value,
                                 static_cast<int>(drilldown->table_name.length),
                                 drilldown->table_name.value);
              }
            }
          }
        }
        if (!cycled) {
          statuses[index] = TSortStatus::VISITED;
          ids.push_back(id);
        }
        break;
      }

      return cycled;
    }
  };

  struct Filter {
    Filter(grn_ctx *ctx,
           grn::CommandArguments *args,
           const char *prefix = nullptr)
      : ctx_(ctx),
        match_columns(args->get_string(prefix, "match_columns")),
        query(args->get_string(prefix, "query")),
        query_expander(args->get_string(
          prefix, nullptr, "query_expander", "query_expansion")),
        query_flags(args->get_string(prefix, "query_flags")),
        query_options(args->get_string(prefix, "query_options")),
        filter(args->get_string(prefix, "filter")),
        post_filter(args->get_string(prefix, "post_filter")),
        fuzzy_max_distance(args->get_uint32(prefix, "fuzzy_max_distance", 0)),
        fuzzy_max_expansions(
          args->get_uint32(prefix,
                           "fuzzy_max_expansions",
                           GRN_TABLE_SELECTOR_FUZZY_MAX_EXPANSIONS_DEFAULT)),
        fuzzy_prefix_length(args->get_uint32(prefix, "fuzzy_prefix_length", 0)),
        fuzzy_max_distance_ratio(
          args->get_float(prefix, "fuzzy_max_distance_ratio", 0)),
        fuzzy_with_transposition(
          args->get_bool(prefix, "fuzzy_with_transposition", true)),
        condition({nullptr, nullptr, nullptr}),
        post_condition({nullptr}),
        filtered(nullptr),
        post_filtered(nullptr)
    {
    }

    ~Filter() { close(); }

    void
    close()
    {
      if (post_filtered) {
        grn_obj_unlink(ctx_, post_filtered);
        post_filtered = nullptr;
      }
      if (post_condition.expression) {
        grn_obj_close(ctx_, post_condition.expression);
        post_condition.expression = nullptr;
      }
      if (filtered) {
        grn_obj_unlink(ctx_, filtered);
        filtered = nullptr;
      }
      if (condition.expression) {
        grn_obj_close(ctx_, condition.expression);
        condition.expression = nullptr;
      }
      if (condition.match_columns) {
        grn_obj_close(ctx_, condition.match_columns);
        condition.match_columns = nullptr;
      }
      if (condition.query_options_expression) {
        grn_obj_close(ctx_, condition.query_options_expression);
        condition.query_options_expression = nullptr;
      }
    }

    void
    set_fuzzy_options(grn_ctx *ctx, grn_table_selector *table_selector)
    {
      grn_table_selector_set_fuzzy_max_distance(ctx,
                                                table_selector,
                                                fuzzy_max_distance);
      grn_table_selector_set_fuzzy_max_expansions(ctx,
                                                  table_selector,
                                                  fuzzy_max_expansions);
      grn_table_selector_set_fuzzy_prefix_length(ctx,
                                                 table_selector,
                                                 fuzzy_prefix_length);
      grn_table_selector_set_fuzzy_max_distance_ratio(ctx,
                                                      table_selector,
                                                      fuzzy_max_distance_ratio);
      grn_table_selector_set_fuzzy_with_transposition(ctx,
                                                      table_selector,
                                                      fuzzy_with_transposition);
    }

    grn_ctx *ctx_;

    grn_raw_string match_columns;
    grn_raw_string query;
    grn_raw_string query_expander;
    grn_raw_string query_flags;
    grn_raw_string query_options;
    grn_raw_string filter;
    grn_raw_string post_filter;
    uint32_t fuzzy_max_distance;
    uint32_t fuzzy_max_expansions;
    uint32_t fuzzy_prefix_length;
    float fuzzy_max_distance_ratio;
    bool fuzzy_with_transposition;
    struct {
      grn_obj *match_columns;
      grn_obj *expression;
      grn_obj *query_options_expression;
    } condition;
    struct {
      grn_obj *expression;
    } post_condition;
    grn_obj *filtered;
    grn_obj *post_filtered;
  };

  struct Slice {
    Slice(grn_ctx *ctx,
          SelectExecutor *executor,
          grn::CommandArguments *args,
          const char *label,
          size_t label_len,
          const char *prefix,
          const char *log_tag)
      : ctx_(ctx),
        executor_(executor),
        label({label, label_len}),
        filter(ctx, args, prefix),
        sort_keys(args->get_string(prefix, "sort_keys")),
        output_columns(args->get_string(prefix, "output_columns")),
        offset(args->get_int32(prefix, "offset", 0)),
        limit(args->get_int32(prefix, "limit", GRN_SELECT_DEFAULT_LIMIT)),
        tables({nullptr, nullptr, nullptr, nullptr, nullptr}),
        drilldowns(ctx, executor, args, prefix, false, log_tag),
        dynamic_columns(ctx, args)
    {
      if (ctx->rc != GRN_SUCCESS) {
        return;
      }
      if (!dynamic_columns.fill(prefix)) {
        return;
      }
    }

    ~Slice()
    {
      filter.close();
      drilldowns.close();
      dynamic_columns.close();
      if (tables.sorted) {
        grn_obj_unlink(ctx_, tables.sorted);
      }
      if (tables.initial) {
        grn_obj_unlink(ctx_, tables.initial);
      }
    }

    bool
    execute(Slices *slices, grn_obj *table);

    grn_ctx *ctx_;
    SelectExecutor *executor_;

    grn_raw_string label;
    Filter filter;
    grn_raw_string sort_keys;
    grn_raw_string output_columns;
    int offset;
    int limit;
    struct {
      grn_obj *target;
      grn_obj *initial;
      grn_obj *result;
      grn_obj *sorted;
      grn_obj *output;
    } tables;
    Drilldowns drilldowns;
    DynamicColumns dynamic_columns;

  private:
    bool
    execute_internal(grn_ctx *ctx, Slices *slices, grn_obj *table);
  };

  class Slices {
  public:
    Slices(grn_ctx *ctx, SelectExecutor *executor, grn::CommandArguments *args)
      : ctx_(ctx),
        executor_(executor),
        args_(args), // TODO: executor->args()
        slices_(nullptr),
        n_executing_slices_(0),
        n_executed_slices_(0)
    {
      fill();
    }

    ~Slices() { close(); }

    void
    close()
    {
      if (slices_) {
        close_objects<Slice>(ctx_, slices_);
        slices_ = nullptr;
      }
    }

    ObjectCursor<Slice>
    begin()
    {
      if (slices_) {
        auto cursor = grn_hash_cursor_open(ctx_,
                                           slices_,
                                           NULL,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           -1,
                                           GRN_CURSOR_ASCENDING);
        return ObjectCursor<Slice>(ctx_, cursor);
      } else {
        return ObjectCursor<Slice>(ctx_, nullptr);
      }
    }

    ObjectCursor<Slice>
    end()
    {
      return ObjectCursor<Slice>(ctx_, nullptr);
    }

    Slice *
    operator[](grn_raw_string &name)
    {
      if (slices_) {
        void *value = nullptr;
        auto id = grn_hash_get(ctx_,
                               slices_,
                               name.value,
                               static_cast<unsigned int>(name.length),
                               &value);
        if (id == GRN_ID_NIL) {
          return nullptr;
        } else {
          return static_cast<Slice *>(value);
        }
      } else {
        return nullptr;
      }
    }

    bool
    empty()
    {
      return slices_ == nullptr;
    }

    void
    executed(Slice *executed_slice);

    bool
    execute(grn_obj *table);

  private:
    grn_ctx *ctx_;
    SelectExecutor *executor_;
    grn::CommandArguments *args_;
    grn_hash *slices_;
    uint32_t n_executing_slices_;
    std::atomic<uint32_t> n_executed_slices_;

    void
    fill()
    {
      for (const auto &arg : *args_) {
        constexpr auto name_prefix = "slices[";
        auto name_prefix_length = strlen(name_prefix);
        if (!GRN_RAW_STRING_START_WITH_CSTRING(arg.name, name_prefix)) {
          continue;
        }
        auto label_end = memchr(arg.name.value, ']', arg.name.length);
        if (!label_end) {
          continue;
        }
        auto label_length =
          static_cast<size_t>(static_cast<const char *>(label_end) -
                              arg.name.value) -
          name_prefix_length;
        auto label =
          grn_raw_string_substring(ctx_,
                                   &(arg.name),
                                   name_prefix_length,
                                   static_cast<int64_t>(label_length));
        if (!add(&label)) {
          return;
        }
      }
    }

    Slice *
    add(grn_raw_string *label)
    {
      if (!slices_) {
        slices_ = grn_hash_create(ctx_,
                                  NULL,
                                  GRN_TABLE_MAX_KEY_SIZE,
                                  sizeof(Slice),
                                  GRN_OBJ_TABLE_HASH_KEY |
                                    GRN_OBJ_KEY_VAR_SIZE | GRN_HASH_TINY);
        if (!slices_) {
          GRN_PLUGIN_ERROR(ctx_,
                           GRN_INVALID_ARGUMENT,
                           "[select][slices] "
                           "failed to allocate slices data: %s",
                           ctx_->errbuf);
          return nullptr;
        }
      }

      void *value;
      int added;
      grn_hash_add(ctx_,
                   slices_,
                   label->value,
                   static_cast<unsigned int>(label->length),
                   &value,
                   &added);
      auto slice = static_cast<Slice *>(value);
      if (added) {
        char slice_prefix[GRN_TABLE_MAX_KEY_SIZE];
        grn_snprintf(slice_prefix,
                     GRN_TABLE_MAX_KEY_SIZE,
                     GRN_TABLE_MAX_KEY_SIZE,
                     "slices[%.*s].",
                     static_cast<int>(label->length),
                     label->value);
        char slice_log_tag[GRN_TABLE_MAX_KEY_SIZE];
        grn_snprintf(slice_log_tag,
                     GRN_TABLE_MAX_KEY_SIZE,
                     GRN_TABLE_MAX_KEY_SIZE,
                     "[slices][%.*s]",
                     static_cast<int>(label->length),
                     label->value);
        new (slice) Slice(ctx_,
                          executor_,
                          args_,
                          label->value,
                          label->length,
                          slice_prefix,
                          slice_log_tag);
        if (ctx_->rc != GRN_SUCCESS) {
          return nullptr;
        }
      }
      return slice;
    }
  };
} // namespace

typedef struct _grn_select_output_formatter grn_select_output_formatter;

namespace {
  struct Tables {
    Tables(grn_ctx *ctx)
      : ctx_(ctx),
        target(nullptr),
        initial(nullptr),
        result(nullptr),
        sorted(nullptr),
        output(nullptr)
    {
    }

    ~Tables() { close(); }

    void
    close()
    {
      close_sorted();

      if (result && result != initial && result != target) {
        grn_obj_unlink(ctx_, result);
      }
      result = nullptr;

      if (initial && initial != target) {
        grn_obj_unlink(ctx_, initial);
      }
      initial = nullptr;

      if (target) {
        grn_obj_unlink(ctx_, target);
      }
      target = nullptr;
    }

    void
    close_sorted()
    {
      if (!sorted) {
        return;
      }
      grn_obj_unlink(ctx_, sorted);
      sorted = nullptr;
    }

    grn_ctx *ctx_;

    grn_obj *target;
    grn_obj *initial;
    grn_obj *result;
    grn_obj *sorted;
    grn_obj *output;
  };

  class TaskExecutor {
  private:
    grn_ctx *ctx_;
    int32_t n_workers_;
#ifdef GRN_WITH_APACHE_ARROW
    std::shared_ptr<::arrow::internal::ThreadPool> thread_pool_;
    std::unordered_map<uintptr_t, ::arrow::Future<bool>> futures_;
    std::mutex futures_mutex_;
#endif

  public:
    TaskExecutor(grn_ctx *ctx, int32_t n_workers)
      : ctx_(ctx),
        n_workers_(n_workers)
#ifdef GRN_WITH_APACHE_ARROW
        ,
        thread_pool_(nullptr),
        futures_(),
        futures_mutex_()
#endif
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ < 0) {
        n_workers_ = ::arrow::internal::ThreadPool::DefaultCapacity();
      }
      if (n_workers_ > 1) {
        auto thread_pool_result =
          ::arrow::internal::ThreadPool::MakeEternal(n_workers_);
        if (thread_pool_result.ok()) {
          thread_pool_ = *thread_pool_result;
        } else {
          n_workers_ = 0;
        }
      }
#else
      n_workers_ = 0;
#endif
    }

    bool
    is_parallel()
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        return true;
      }
#endif
      return false;
    }

    template <typename Function>
    bool
    execute(void *object, Function &&func, const char *tag)
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        auto future_result = thread_pool_->Submit(func);
        if (!grnarrow::check(ctx_,
                             future_result,
                             tag,
                             " failed to submit a job")) {
          return false;
        }
        {
          std::unique_lock<std::mutex> lock(futures_mutex_);
          futures_.emplace(reinterpret_cast<uintptr_t>(object), *future_result);
        }
        return true;
      }
#endif
      return func();
    }

    bool
    wait(void *object, const char *tag)
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        try {
          auto id = reinterpret_cast<uintptr_t>(object);
          std::unique_lock<std::mutex> lock(futures_mutex_);
          auto future = futures_.at(id);
          lock.unlock();
          auto status = future.status();
          lock.lock();
          futures_.erase(id);
          lock.unlock();
          return grnarrow::check(ctx_,
                                 status,
                                 tag,
                                 " failed to wait a job: ",
                                 id);
        } catch (std::out_of_range &) {
          return true;
        }
      }
#endif
      return true;
    }

    bool
    wait_all()
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        thread_pool_->WaitForIdle();
        return ctx_->rc == GRN_SUCCESS;
      }
#endif
      return true;
    }
  };

  struct SelectExecutor {
    SelectExecutor(grn_ctx *ctx, grn::CommandArguments *args)
      : ctx_(ctx),
        args_(args),

        tables(ctx),
        cacheable(0),
        taintable(0),
        output({0, nullptr}),
        load({
          args->get_string("load_table"),
          args->get_string("load_columns"),
          args->get_string("load_values"),
        }),

        table(args->get_string("table")),
        filter(ctx, args),
        scorer(args->get_string("scorer")),
        sort_keys(args->get_string(nullptr, nullptr, "sort_keys", "sortby")),
        output_columns(args->get_string("output_columns")),
        default_output_columns({nullptr, 0}),
        offset(args->get_int32("offset", 0)),
        limit(args->get_int32("limit", GRN_SELECT_DEFAULT_LIMIT)),
        slices(ctx, this, args),
        drilldowns(ctx, this, args, nullptr, true, ""),
        cache(args->get_string("cache")),
        match_escalation_threshold(
          args->get_string("match_escalation_threshold")),
        adjuster(args->get_string("adjuster")),
        match_escalation(args->get_string("match_escalation")),
        dynamic_columns(ctx, args),
        task_executor_(
          ctx, args->get_int32("n_workers", grn_select_n_workers_default))
    {
      if (ctx_->rc != GRN_SUCCESS) {
        return;
      }

      dynamic_columns.fill(nullptr);
    }

    ~SelectExecutor()
    {
      drilldowns.close();

      slices.close();

      tables.close_sorted();

      if (tables.result == filter.post_filtered) {
        tables.result = nullptr;
      }
      if (tables.result == filter.filtered) {
        tables.result = nullptr;
      }
      filter.close();

      tables.close();

      dynamic_columns.close();
    }

    grn::CommandArguments *
    args()
    {
      return args_;
    }

    TaskExecutor &
    task_executor()
    {
      return task_executor_;
    }

    grn_rc
    execute();

    bool
    execute_olap_operations()
    {
      if (!slices.execute(tables.result)) {
        return false;
      }
      if (!drilldowns.execute(tables.result,
                              &slices,
                              filter.condition.expression,
                              "",
                              "")) {
        return false;
      }
      if (!task_executor_.wait_all()) {
        return false;
      }
      return ctx_->rc == GRN_SUCCESS;
    }

    void
    executed(Slice *executed_slice)
    {
    }

    void
    executed(Slices *executed_slices)
    {
      ++(output.n_elements);
    }

    void
    executed(Drilldown *executed_drilldown)
    {
      if (ctx_->rc != GRN_SUCCESS) {
        return;
      }
      if (!drilldowns.is_labeled()) {
        ++(output.n_elements);
      }
    }

    void
    executed(Drilldowns *executed_drilldowns)
    {
      if (drilldowns.is_labeled()) {
        ++(output.n_elements);
      }
    }

    grn_ctx *ctx_;
    grn::CommandArguments *args_;

    /* for processing */
    Tables tables;
    uint16_t cacheable;
    uint16_t taintable;
    struct {
      int n_elements;
      grn_select_output_formatter *formatter;
    } output;

    /* inputs */
    struct {
      grn_raw_string table;
      grn_raw_string columns;
      grn_raw_string values;
    } load;
    grn_raw_string table;
    Filter filter;
    grn_raw_string scorer;
    grn_raw_string sort_keys;
    grn_raw_string output_columns;
    grn_raw_string default_output_columns;
    int offset;
    int limit;
    Slices slices;
    Drilldowns drilldowns;
    grn_raw_string cache;
    grn_raw_string match_escalation_threshold;
    grn_raw_string adjuster;
    grn_raw_string match_escalation;
    DynamicColumns dynamic_columns;

  private:
    TaskExecutor task_executor_;
  };

  bool
  Drilldown::execute_internal(grn_ctx *ctx,
                              Drilldowns *drilldowns,
                              grn_obj *table,
                              Slices *slices,
                              grn_obj *condition,
                              const char *log_tag_context,
                              const char *query_log_tag_prefix)
  {
    grn_obj *target_table = table;

    grn::TextBulk log_tag_prefix(ctx);
    grn_text_printf(ctx,
                    *log_tag_prefix,
                    "[select]%s[drilldowns]%s%.*s%s",
                    log_tag_context,
                    label.length > 0 ? "[" : "",
                    static_cast<int>(label.length),
                    label.value,
                    label.length > 0 ? "]" : "");
    GRN_TEXT_PUTC(ctx, *log_tag_prefix, '\0');
    grn::TextBulk full_query_log_tag_prefix(ctx);
    if (drilldowns->is_labeled()) {
      grn_text_printf(ctx,
                      *full_query_log_tag_prefix,
                      "%sdrilldowns[%.*s].",
                      query_log_tag_prefix,
                      static_cast<int>(label.length),
                      label.value);
    } else {
      grn_text_printf(ctx,
                      *full_query_log_tag_prefix,
                      "%sdrilldown.",
                      query_log_tag_prefix);
    }
    GRN_TEXT_PUTC(ctx, *full_query_log_tag_prefix, '\0');

    result.limit = max_n_target_records;
    result.flags = GRN_TABLE_GROUP_CALC_COUNT | GRN_TABLE_GROUP_LIMIT;
    result.op = GRN_OP_NOP;
    result.max_n_subrecs = 0;
    result.key_begin = 0;
    result.key_end = 0;
    if (result.calc_target) {
      grn_obj_unlink(ctx, result.calc_target);
    }
    result.calc_target = NULL;

    if (table_name.length > 0) {
      auto dependent_drilldown = (*drilldowns)[table_name];
      auto &task_executor = executor_->task_executor();
      if (dependent_drilldown) {
        if (!task_executor.wait(dependent_drilldown,
                                GRN_TEXT_VALUE(*log_tag_prefix))) {
          return false;
        }
        target_table = dependent_drilldown->result.table;
      } else {
        auto slice = slices ? (*slices)[table_name] : nullptr;
        if (slice) {
          if (!task_executor.wait(slice, GRN_TEXT_VALUE(*log_tag_prefix))) {
            return false;
          }
          target_table = slice->tables.result;
        } else {
          GRN_PLUGIN_ERROR(ctx,
                           GRN_INVALID_ARGUMENT,
                           "%s[table] "
                           "nonexistent label: <%.*s>",
                           GRN_TEXT_VALUE(*log_tag_prefix),
                           static_cast<int>(table_name.length),
                           table_name.value);
          return false;
        }
      }
    }

    {
      struct ParsedKeysCloser {
        ParsedKeysCloser(grn_ctx *ctx)
          : ctx(ctx),
            parsed_keys(nullptr),
            n_parsed_keys(0)
        {
        }

        ~ParsedKeysCloser()
        {
          if (!parsed_keys) {
            return;
          }
          grn_table_sort_key_close(ctx, parsed_keys, n_parsed_keys);
        }

        grn_ctx *ctx;
        grn_table_sort_key *parsed_keys;
        uint32_t n_parsed_keys;
      } parsed_keys_closer(ctx);

      grn_table_sort_key *parsed_keys = NULL;
      uint32_t n_parsed_keys = 0;
      if (keys.length > 0) {
        parsed_keys =
          grn_table_group_keys_parse(ctx,
                                     target_table,
                                     keys.value,
                                     static_cast<int32_t>(keys.length),
                                     &n_parsed_keys);
        if (!parsed_keys) {
          GRN_PLUGIN_CLEAR_ERROR(ctx);
          return true;
        }
        parsed_keys_closer.parsed_keys = parsed_keys;
        parsed_keys_closer.n_parsed_keys = n_parsed_keys;

        if (nth_key >= 0) {
          parsed_keys += nth_key;
          n_parsed_keys = 1;
          result.key_end = 1;
        } else {
          result.key_end = static_cast<uint8_t>(n_parsed_keys - 1);
        }
        if (n_parsed_keys > 1) {
          result.max_n_subrecs = 1;
        }
      }

      if (calc_target_name.length > 0) {
        result.calc_target =
          grn_obj_column(ctx,
                         target_table,
                         calc_target_name.value,
                         static_cast<uint32_t>(calc_target_name.length));
      }
      if (result.calc_target) {
        result.flags |= calc_types;
      }

      if (GRN_RAW_STRING_EQUAL_CSTRING(key_vector_expansion, "POWER_SET")) {
        result.flags |= GRN_TABLE_GROUP_KEY_VECTOR_EXPANSION_POWER_SET;
      }

      if (dynamic_columns.group) {
        result.n_aggregators = grn_hash_size(ctx, dynamic_columns.group);
        if (result.n_aggregators > 0) {
          result.aggregators =
            GRN_MALLOCN(grn_table_group_aggregator *, result.n_aggregators);
          if (!result.aggregators) {
            GRN_PLUGIN_ERROR(ctx,
                             GRN_INVALID_ARGUMENT,
                             "%s[filter] "
                             "failed to allocate aggregators: %s",
                             GRN_TEXT_VALUE(*log_tag_prefix),
                             ctx->errbuf);
            return false;
          }
          uint32_t i = 0;
          bool success = true;
          GRN_HASH_EACH_BEGIN(ctx, dynamic_columns.group, cursor, id)
          {
            void *value;
            grn_hash_cursor_get_value(ctx, cursor, &value);
            auto dynamic_column = static_cast<DynamicColumn *>(value);
            result.aggregators[i] = grn_table_group_aggregator_open(ctx);
            if (!result.aggregators[i]) {
              GRN_PLUGIN_ERROR(ctx,
                               GRN_INVALID_ARGUMENT,
                               "%s[filter] "
                               "failed to open aggregator: %s",
                               GRN_TEXT_VALUE(*log_tag_prefix),
                               ctx->errbuf);
              success = false;
              break;
            }
            grn_table_group_aggregator_set_output_column_name(
              ctx,
              result.aggregators[i],
              dynamic_column->label.value,
              static_cast<int32_t>(dynamic_column->label.length));
            grn_table_group_aggregator_set_output_column_type(
              ctx,
              result.aggregators[i],
              dynamic_column->type);
            grn_table_group_aggregator_set_output_column_flags(
              ctx,
              result.aggregators[i],
              dynamic_column->flags);
            grn_table_group_aggregator_set_expression(
              ctx,
              result.aggregators[i],
              dynamic_column->value.value,
              static_cast<int32_t>(dynamic_column->value.length));
            i++;
          }
          GRN_HASH_EACH_END(ctx, cursor);
          if (!success) {
            return false;
          }
          result.flags |= GRN_TABLE_GROUP_CALC_AGGREGATOR;
        }
      }

      if (n_parsed_keys == 0 && !target_table) {
        /* For backward compatibility and consistency. Ignore
         * nonexistent table case with warning like we did for sort_keys
         * and drilldown[LABEL].keys. */
        GRN_LOG(ctx,
                GRN_WARN,
                "%s[table] doesn't exist: <%.*s>",
                GRN_TEXT_VALUE(*log_tag_prefix),
                static_cast<int>(table_name.length),
                table_name.value);
        return true;
      }
      grn_table_group(ctx,
                      target_table,
                      parsed_keys,
                      static_cast<int>(n_parsed_keys),
                      &result,
                      1);
    }

    if (!result.table) {
      return false;
    }

    if (grn_query_logger_pass(ctx, GRN_QUERY_LOG_SIZE)) {
      grn_obj keys_inspected;
      GRN_TEXT_INIT(&keys_inspected, GRN_OBJ_DO_SHALLOW_COPY);
      if (nth_key >= 0) {
        GRN_TEXT_SET(ctx, &keys_inspected, label.value, label.length);
      } else {
        GRN_TEXT_SET(ctx, &keys_inspected, keys.value, keys.length);
      }
      GRN_QUERY_LOG(
        ctx,
        GRN_QUERY_LOG_SIZE,
        ":",
        "%.*s(%u): %.*s",
        static_cast<int>(GRN_TEXT_LEN(*full_query_log_tag_prefix) - 2),
        GRN_TEXT_VALUE(*full_query_log_tag_prefix),
        grn_table_size(ctx, result.table),
        static_cast<int>(GRN_TEXT_LEN(&keys_inspected)),
        GRN_TEXT_VALUE(&keys_inspected));
      GRN_OBJ_FIN(ctx, &keys_inspected);
    }

    if (dynamic_columns.initial) {
      grn_select_apply_dynamic_columns(
        ctx,
        executor_,
        result.table,
        DynamicColumnStage::INITIAL,
        dynamic_columns.initial,
        condition,
        GRN_TEXT_VALUE(*log_tag_prefix),
        GRN_TEXT_VALUE(*full_query_log_tag_prefix));
    }

    if (filter.length > 0) {
      grn_obj *expression;
      grn_obj *record;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, result.table, expression, record);
      if (!expression) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s[filter] "
                         "failed to create expression for filter: %s",
                         GRN_TEXT_VALUE(*log_tag_prefix),
                         ctx->errbuf);
        return false;
      }
      grn::UniqueObj unique_expression(ctx, expression);
      grn_expr_parse(ctx,
                     expression,
                     filter.value,
                     filter.length,
                     NULL,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT);
      if (ctx->rc != GRN_SUCCESS) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s[filter] "
                         "failed to parse filter: <%.*s>: %s",
                         GRN_TEXT_VALUE(*log_tag_prefix),
                         static_cast<int>(filter.length),
                         filter.value,
                         ctx->errbuf);
        return false;
      }
      filtered_result =
        grn_table_select(ctx, result.table, expression, NULL, GRN_OP_OR);
      if (ctx->rc != GRN_SUCCESS) {
        if (filtered_result) {
          grn_obj_close(ctx, filtered_result);
          filtered_result = nullptr;
        }
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s[filter] "
                         "failed to execute filter: <%.*s>: %s",
                         GRN_TEXT_VALUE(*log_tag_prefix),
                         static_cast<int>(filter.length),
                         filter.value,
                         ctx->errbuf);
        return false;
      }

      GRN_QUERY_LOG(ctx,
                    GRN_QUERY_LOG_SIZE,
                    ":",
                    "%sfilter(%u)",
                    GRN_TEXT_VALUE(*full_query_log_tag_prefix),
                    grn_table_size(ctx, filtered_result));
    }

    if (dynamic_columns.filtered) {
      grn_select_apply_dynamic_columns(
        ctx,
        executor_,
        filtered_result,
        DynamicColumnStage::FILTERED,
        dynamic_columns.filtered,
        condition,
        GRN_TEXT_VALUE(*log_tag_prefix),
        GRN_TEXT_VALUE(*full_query_log_tag_prefix));
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }

    {
      grn_obj *adjuster_result_table;
      if (filtered_result) {
        adjuster_result_table = filtered_result;
      } else {
        adjuster_result_table = result.table;
      }
      grn_select_apply_adjuster(ctx,
                                executor_,
                                &adjuster,
                                result.table,
                                adjuster_result_table,
                                GRN_TEXT_VALUE(*log_tag_prefix),
                                GRN_TEXT_VALUE(*full_query_log_tag_prefix));
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }

    drilldowns->executed(this);
    executor_->executed(this);

    return true;
  }

  bool
  Drilldown::execute(Drilldowns *drilldowns,
                     grn_obj *table,
                     Slices *slices,
                     grn_obj *condition,
                     const char *log_tag_context,
                     const char *query_log_tag_prefix)
  {
    auto ctx = ctx_;
    const auto is_parallel = executor_->task_executor().is_parallel();
    if (is_parallel) {
      ctx = grn_ctx_pull_child(ctx_);
    }
    grn::ChildCtxReleaser releaser(ctx_, is_parallel ? ctx : nullptr);

    GRN_API_ENTER;
    auto success = execute_internal(ctx,
                                    drilldowns,
                                    table,
                                    slices,
                                    condition,
                                    log_tag_context,
                                    query_log_tag_prefix);
    GRN_API_RETURN(success);
  }

  void
  Drilldowns::executed(Drilldown *executed_drilldown)
  {
    if (ctx_->rc != GRN_SUCCESS) {
      return;
    }
    if (++n_executed_drilldowns_ == 1) {
      executor_->executed(this);
    }
  }

  bool
  Drilldowns::execute(grn_obj *table,
                      Slices *slices,
                      grn_obj *condition,
                      const char *log_tag_context,
                      const char *query_log_tag_prefix)
  {
    if (!process_keys(table)) {
      return false;
    }

    if (!drilldowns_) {
      return true;
    }

    std::vector<grn_id> tsorted_ids;
    if (!tsort(tsorted_ids)) {
      return false;
    }

    n_executing_drilldowns_ = static_cast<uint32_t>(tsorted_ids.size());
    for (const auto &id : tsorted_ids) {
      auto drilldowns = this;
      auto drilldown = (*drilldowns)[id];
      auto execute = [=]() {
        return drilldown->execute(drilldowns,
                                  table,
                                  slices,
                                  condition,
                                  log_tag_context,
                                  query_log_tag_prefix);
      };
      if (!executor_->task_executor().execute(drilldown,
                                              execute,
                                              log_tag_context)) {
        return false;
      }
    }
    return true;
  }

  bool
  Slice::execute_internal(grn_ctx *ctx, Slices *slices, grn_obj *table)
  {
    char log_tag[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(log_tag,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[select][slices][%.*s]",
                 static_cast<int>(label.length),
                 label.value);
    if (filter.query.length == 0 && filter.filter.length == 0) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s slice requires query or filter",
                       log_tag);
      return false;
    }

    char query_log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(query_log_tag_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "slices[%.*s].",
                 static_cast<int>(label.length),
                 label.value);

    tables.target = table;
    if (dynamic_columns.initial) {
      tables.initial =
        grn_select_create_all_selected_result_table(ctx, tables.target);
      if (!tables.initial) {
        return false;
      }
      grn_select_apply_dynamic_columns(ctx,
                                       executor_,
                                       tables.initial,
                                       DynamicColumnStage::INITIAL,
                                       dynamic_columns.initial,
                                       filter.condition.expression,
                                       log_tag,
                                       query_log_tag_prefix);
      if (ctx_->rc != GRN_SUCCESS) {
        return false;
      }
      tables.target = tables.initial;
    }

    if (!grn_filter_execute(ctx,
                            executor_,
                            &filter,
                            tables.target,
                            &dynamic_columns,
                            log_tag,
                            query_log_tag_prefix)) {
      return false;
    }
    grn_expr_set_parent(ctx,
                        filter.condition.expression,
                        executor_->filter.condition.expression);

    tables.result = filter.filtered;
    GRN_QUERY_LOG(ctx_,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "%.*s(%d)",
                  static_cast<int>(strlen(query_log_tag_prefix) - 1),
                  query_log_tag_prefix,
                  grn_table_size(ctx_, tables.result));

    if (dynamic_columns.filtered) {
      grn_select_apply_dynamic_columns(ctx,
                                       executor_,
                                       tables.result,
                                       DynamicColumnStage::FILTERED,
                                       dynamic_columns.filtered,
                                       filter.condition.expression,
                                       log_tag,
                                       query_log_tag_prefix);
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }

    if (!grn_filter_execute_post_filter(ctx,
                                        executor_,
                                        &filter,
                                        tables.result,
                                        log_tag,
                                        query_log_tag_prefix)) {
      return false;
    }

    if (filter.post_filtered) {
      tables.result = filter.post_filtered;
    }

    if (!drilldowns.execute(tables.result,
                            nullptr,
                            filter.condition.expression,
                            log_tag,
                            query_log_tag_prefix)) {
      return false;
    }

    slices->executed(this);
    executor_->executed(this);

    return true;
  }

  bool
  Slice::execute(Slices *slices, grn_obj *table)
  {
    auto ctx = ctx_;
    const auto is_parallel = executor_->task_executor().is_parallel();
    if (is_parallel) {
      ctx = grn_ctx_pull_child(ctx_);
    }
    grn::ChildCtxReleaser releaser(ctx_, is_parallel ? ctx : nullptr);

    GRN_API_ENTER;
    auto success = execute_internal(ctx, slices, table);
    GRN_API_RETURN(success);
  }

  void
  Slices::executed(Slice *executed_slice)
  {
    {
      ++n_executed_slices_;
      if (n_executed_slices_ == n_executing_slices_) {
        executor_->executed(this);
      }
    }
  }

  bool
  Slices::execute(grn_obj *table)
  {
    if (!slices_) {
      return true;
    }

    n_executing_slices_ = grn_hash_size(ctx_, slices_);
    for (auto slice : *this) {
      auto slices = this;
      auto execute = [=]() { return slice->execute(slices, table); };
      if (!executor_->task_executor().execute(slice, execute, "[slices]")) {
        return false;
      }
    }

    return true;
  }
} // namespace

using grn_select_data = SelectExecutor;

typedef void
grn_select_output_slices_label_func(grn_ctx *ctx, grn_select_data *data);
typedef void
grn_select_output_slices_open_func(grn_ctx *ctx,
                                   grn_select_data *data,
                                   unsigned int n_result_sets);
typedef void
grn_select_output_slices_close_func(grn_ctx *ctx, grn_select_data *data);
typedef void
grn_select_output_slice_label_func(grn_ctx *ctx,
                                   grn_select_data *data,
                                   Slice *slice);
typedef void
grn_select_output_drilldowns_label_func(grn_ctx *ctx, grn_select_data *data);
typedef void
grn_select_output_drilldowns_open_func(grn_ctx *ctx,
                                       grn_select_data *data,
                                       unsigned int n_result_sets);
typedef void
grn_select_output_drilldowns_close_func(grn_ctx *ctx, grn_select_data *data);
typedef void
grn_select_output_drilldown_label_func(grn_ctx *ctx,
                                       grn_select_data *data,
                                       Drilldown *drilldown);

struct _grn_select_output_formatter {
  grn_select_output_slices_label_func *slices_label;
  grn_select_output_slices_open_func *slices_open;
  grn_select_output_slices_close_func *slices_close;
  grn_select_output_slice_label_func *slice_label;
  grn_select_output_drilldowns_label_func *drilldowns_label;
  grn_select_output_drilldowns_open_func *drilldowns_open;
  grn_select_output_drilldowns_close_func *drilldowns_close;
  grn_select_output_drilldown_label_func *drilldown_label;
};

grn_rc
grn_proc_syntax_expand_query(grn_ctx *ctx,
                             const char *query,
                             unsigned int query_len,
                             grn_expr_flags flags,
                             const char *query_expander_name,
                             unsigned int query_expander_name_len,
                             const char *term_column_name,
                             unsigned int term_column_name_len,
                             const char *expanded_term_column_name,
                             unsigned int expanded_term_column_name_len,
                             grn_obj *expanded_query,
                             const char *error_message_tag)
{
  grn_obj *query_expander;

  query_expander =
    grn_ctx_get(ctx, query_expander_name, query_expander_name_len);
  if (!query_expander) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s nonexistent query expander: <%.*s>",
                     error_message_tag,
                     (int)query_expander_name_len,
                     query_expander_name);
    return ctx->rc;
  }

  if (expanded_term_column_name_len == 0) {
    return grn_expr_syntax_expand_query(ctx,
                                        query,
                                        query_len,
                                        flags,
                                        query_expander,
                                        expanded_query);
  }

  if (!grn_obj_is_table(ctx, query_expander)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, query_expander);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s query expander with expanded term column "
                     "must be table: <%.*s>",
                     error_message_tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return ctx->rc;
  }

  {
    grn_obj *term_column = NULL;
    grn_obj *expanded_term_column;

    expanded_term_column = grn_obj_column(ctx,
                                          query_expander,
                                          expanded_term_column_name,
                                          expanded_term_column_name_len);
    if (!expanded_term_column) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, query_expander);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s nonexistent expanded term column: <%.*s>: "
                       "query expander: <%.*s>",
                       error_message_tag,
                       (int)expanded_term_column_name_len,
                       expanded_term_column_name,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return ctx->rc;
    }

    if (term_column_name_len > 0) {
      term_column = grn_obj_column(ctx,
                                   query_expander,
                                   term_column_name,
                                   term_column_name_len);
      if (!term_column) {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, query_expander);
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s nonexistent term column: <%.*s>: "
                         "query expander: <%.*s>",
                         error_message_tag,
                         (int)term_column_name_len,
                         term_column_name,
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        if (grn_obj_is_accessor(ctx, expanded_term_column)) {
          grn_obj_unlink(ctx, expanded_term_column);
        }
        return ctx->rc;
      }
    }

    grn_expr_syntax_expand_query_by_table(ctx,
                                          query,
                                          query_len,
                                          flags,
                                          term_column,
                                          expanded_term_column,
                                          expanded_query);
    if (grn_obj_is_accessor(ctx, term_column)) {
      grn_obj_unlink(ctx, term_column);
    }
    if (grn_obj_is_accessor(ctx, expanded_term_column)) {
      grn_obj_unlink(ctx, expanded_term_column);
    }
    return ctx->rc;
  }
}

static void
grn_select_expression_set_condition(grn_ctx *ctx,
                                    grn_obj *expression,
                                    grn_obj *condition)
{
  if (!expression) {
    return;
  }

  grn_expr_set_condition(ctx, expression, condition);
}

static grn_obj *
grn_select_create_all_selected_result_table(grn_ctx *ctx, grn_obj *table)
{
  grn_obj *result;

  result = grn_table_create(ctx,
                            NULL,
                            0,
                            NULL,
                            GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                            table,
                            NULL);
  if (!result) {
    return NULL;
  }

  grn_result_set_add_table(ctx, (grn_hash *)result, table, 0.0, GRN_OP_OR);
  return result;
}

static grn_obj *
grn_select_create_no_sort_keys_sorted_table(grn_ctx *ctx,
                                            grn_select_data *data,
                                            grn_obj *table)
{
  grn_obj *sorted;
  grn_table_cursor *cursor;

  sorted =
    grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY, NULL, table);

  if (!sorted) {
    return NULL;
  }

  cursor = grn_table_cursor_open(ctx,
                                 table,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 data->offset,
                                 data->limit,
                                 GRN_CURSOR_ASCENDING);
  if (cursor) {
    grn_id id;
    while ((id = grn_table_cursor_next(ctx, cursor))) {
      grn_id *value;
      if (grn_array_add(ctx, (grn_array *)sorted, (void **)&value)) {
        *value = id;
      }
    }
    grn_table_cursor_close(ctx, cursor);
  }

  return sorted;
}

typedef enum {
  TSORT_STATUS_NOT_VISITED,
  TSORT_STATUS_VISITING,
  TSORT_STATUS_VISITED
} tsort_status;

static bool
dynamic_columns_tsort_visit(grn_ctx *ctx,
                            grn_hash *dynamic_columns,
                            tsort_status *statuses,
                            grn_obj *ids,
                            grn_id id,
                            const char *log_tag_prefix)
{
  bool cycled = true;
  uint32_t index = id - 1;

  switch (statuses[index]) {
  case TSORT_STATUS_VISITING:
    cycled = true;
    break;
  case TSORT_STATUS_VISITED:
    cycled = false;
    break;
  case TSORT_STATUS_NOT_VISITED:
    cycled = false;
    statuses[index] = TSORT_STATUS_VISITING;
    {
      auto *dynamic_column =
        const_cast<DynamicColumn *>(reinterpret_cast<const DynamicColumn *>(
          grn_hash_get_value_(ctx, dynamic_columns, id, NULL)));
      uint32_t i;
      uint32_t n_dependencies =
        grn_vector_size(ctx, &(dynamic_column->dependency_names));
      for (i = 0; i < n_dependencies; i++) {
        const char *name;
        unsigned int name_length =
          grn_vector_get_element(ctx,
                                 &(dynamic_column->dependency_names),
                                 i,
                                 &name,
                                 NULL,
                                 NULL);
        grn_id dependent_id;
        dependent_id =
          grn_hash_get(ctx, dynamic_columns, name, name_length, NULL);
        if (dependent_id != GRN_ID_NIL) {
          cycled = dynamic_columns_tsort_visit(ctx,
                                               dynamic_columns,
                                               statuses,
                                               ids,
                                               dependent_id,
                                               log_tag_prefix);
          if (cycled) {
            GRN_PLUGIN_ERROR(ctx,
                             GRN_INVALID_ARGUMENT,
                             "%s[column][%.*s] cycled dependency: <%.*s>",
                             log_tag_prefix,
                             (int)(dynamic_column->label.length),
                             dynamic_column->label.value,
                             (int)name_length,
                             name);
          }
        }
      }
    }
    if (!cycled) {
      statuses[index] = TSORT_STATUS_VISITED;
      GRN_RECORD_PUT(ctx, ids, id);
    }
    break;
  }

  return cycled;
}

static bool
dynamic_columns_tsort_body(grn_ctx *ctx,
                           grn_hash *dynamic_columns,
                           tsort_status *statuses,
                           grn_obj *ids,
                           const char *log_tag_prefix)
{
  grn_bool succeeded = GRN_TRUE;

  GRN_HASH_EACH_BEGIN(ctx, dynamic_columns, cursor, id)
  {
    if (dynamic_columns_tsort_visit(ctx,
                                    dynamic_columns,
                                    statuses,
                                    ids,
                                    id,
                                    log_tag_prefix)) {
      succeeded = true;
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);

  return succeeded;
}

static void
dynamic_columns_tsort_init(grn_ctx *ctx,
                           tsort_status *statuses,
                           size_t n_statuses)
{
  size_t i;
  for (i = 0; i < n_statuses; i++) {
    statuses[i] = TSORT_STATUS_NOT_VISITED;
  }
}

static bool
dynamic_columns_tsort(grn_ctx *ctx,
                      grn_hash *dynamic_columns,
                      grn_obj *ids,
                      const char *log_tag_prefix)
{
  size_t n_statuses = grn_hash_size(ctx, dynamic_columns);
  tsort_status *statuses = GRN_PLUGIN_MALLOCN(ctx, tsort_status, n_statuses);
  if (!statuses) {
    return false;
  }

  dynamic_columns_tsort_init(ctx, statuses, n_statuses);
  auto succeeded = dynamic_columns_tsort_body(ctx,
                                              dynamic_columns,
                                              statuses,
                                              ids,
                                              log_tag_prefix);
  GRN_PLUGIN_FREE(ctx, statuses);
  return succeeded;
}

static bool
grn_select_apply_dynamic_column(grn_ctx *ctx,
                                grn_select_data *data,
                                grn_obj *table,
                                DynamicColumnStage stage,
                                DynamicColumn *dynamic_column,
                                grn_obj *condition,
                                const char *log_tag_prefix,
                                const char *query_log_tag_prefix)
{
  bool succeeded = false;
  grn_obj tag;
  GRN_TEXT_INIT(&tag, 0);
  grn_text_printf(ctx,
                  &tag,
                  "%s[columns][%s][%.*s]",
                  log_tag_prefix,
                  grn_dynamic_column_stage_name(dynamic_column->stage),
                  (int)(dynamic_column->label.length),
                  dynamic_column->label.value);
  grn_obj *column = grn_column_create(ctx,
                                      table,
                                      dynamic_column->label.value,
                                      dynamic_column->label.length,
                                      NULL,
                                      dynamic_column->flags,
                                      dynamic_column->type);
  if (!column) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%.*s failed to create column: %s",
                     (int)GRN_TEXT_LEN(&tag),
                     GRN_TEXT_VALUE(&tag),
                     ctx->errbuf);
    goto exit;
  }

  if (stage == DynamicColumnStage::RESULT_SET) {
    succeeded = true;
    goto exit;
  }

  if (dynamic_column->window.sort_keys.length > 0 ||
      dynamic_column->window.group_keys.length > 0) {
    grn_window_function_executor executor;
    grn_window_function_executor_init(ctx, &executor);
    if (ctx->rc == GRN_SUCCESS) {
      grn_window_function_executor_set_tag(ctx,
                                           &executor,
                                           GRN_TEXT_VALUE(&tag),
                                           GRN_TEXT_LEN(&tag));
    }
    if (ctx->rc == GRN_SUCCESS) {
      grn_window_function_executor_add_table(ctx, &executor, table);
    }
    if (ctx->rc == GRN_SUCCESS) {
      grn_window_function_executor_set_source(ctx,
                                              &executor,
                                              dynamic_column->value.value,
                                              dynamic_column->value.length);
    }
    if (ctx->rc == GRN_SUCCESS) {
      grn_window_function_executor_set_output_column_name(
        ctx,
        &executor,
        dynamic_column->label.value,
        dynamic_column->label.length);
    }
    if (ctx->rc == GRN_SUCCESS && dynamic_column->window.sort_keys.length > 0) {
      grn_window_function_executor_set_sort_keys(
        ctx,
        &executor,
        dynamic_column->window.sort_keys.value,
        dynamic_column->window.sort_keys.length);
    }
    if (ctx->rc == GRN_SUCCESS &&
        dynamic_column->window.group_keys.length > 0) {
      grn_window_function_executor_set_group_keys(
        ctx,
        &executor,
        dynamic_column->window.group_keys.value,
        dynamic_column->window.group_keys.length);
    }
    if (ctx->rc == GRN_SUCCESS) {
      grn_window_function_executor_execute(ctx, &executor);
    }
    grn_window_function_executor_fin(ctx, &executor);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  } else {
    grn_obj *expression;
    grn_obj *record;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expression, record);
    if (!expression) {
      grn_obj_close(ctx, column);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%.*s failed to create expression to compute value: %s",
                       (int)GRN_TEXT_LEN(&tag),
                       GRN_TEXT_VALUE(&tag),
                       ctx->errbuf);
      succeeded = false;
      goto exit;
    }
    grn_expr_parse(ctx,
                   expression,
                   dynamic_column->value.value,
                   dynamic_column->value.length,
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, expression);
      grn_obj_close(ctx, column);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%.*s failed to parse value: <%.*s>: %s",
                       (int)GRN_TEXT_LEN(&tag),
                       GRN_TEXT_VALUE(&tag),
                       (int)(dynamic_column->value.length),
                       dynamic_column->value.value,
                       ctx->errbuf);
      succeeded = false;
      goto exit;
    }
    grn_select_expression_set_condition(ctx, expression, condition);

    grn_rc rc = grn_table_apply_expr(ctx, table, column, expression);
    if (rc != GRN_SUCCESS) {
      grn_obj_close(ctx, expression);
      grn_obj_close(ctx, column);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%.*s "
                       "failed to apply expression to generate column values: "
                       "%s",
                       (int)GRN_TEXT_LEN(&tag),
                       GRN_TEXT_VALUE(&tag),
                       ctx->errbuf);
      succeeded = false;
      goto exit;
    }
    grn_obj_close(ctx, expression);
  }

  succeeded = true;

exit:
  GRN_OBJ_FIN(ctx, &tag);

  return succeeded;
}

static void
grn_select_apply_dynamic_columns(grn_ctx *ctx,
                                 grn_select_data *data,
                                 grn_obj *table,
                                 DynamicColumnStage stage,
                                 grn_hash *dynamic_columns,
                                 grn_obj *condition,
                                 const char *log_tag_prefix,
                                 const char *query_log_tag_prefix)
{
  grn_obj tsorted_ids;
  GRN_RECORD_INIT(&tsorted_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  if (!dynamic_columns_tsort(ctx,
                             dynamic_columns,
                             &tsorted_ids,
                             log_tag_prefix)) {
    GRN_OBJ_FIN(ctx, &tsorted_ids);
    return;
  }

  size_t i;
  size_t n_columns = GRN_RECORD_VECTOR_SIZE(&tsorted_ids);
  for (i = 0; i < n_columns; i++) {
    grn_id id = GRN_RECORD_VALUE_AT(&tsorted_ids, i);
    auto dynamic_column =
      const_cast<DynamicColumn *>(reinterpret_cast<const DynamicColumn *>(
        grn_hash_get_value_(ctx, dynamic_columns, id, NULL)));
    if (!grn_select_apply_dynamic_column(ctx,
                                         data,
                                         table,
                                         stage,
                                         dynamic_column,
                                         condition,
                                         log_tag_prefix,
                                         query_log_tag_prefix)) {
      break;
    }

    GRN_QUERY_LOG(ctx,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "%scolumns[%.*s](%d)",
                  query_log_tag_prefix ? query_log_tag_prefix : "",
                  (int)(dynamic_column->label.length),
                  dynamic_column->label.value,
                  grn_table_size(ctx, table));
  }

  GRN_OBJ_FIN(ctx, &tsorted_ids);
}

static bool
grn_filter_execute(grn_ctx *ctx,
                   grn_select_data *data,
                   Filter *filter,
                   grn_obj *table,
                   DynamicColumns *dynamic_columns,
                   const char *log_tag_prefix,
                   const char *query_log_tag_prefix)
{
  grn_obj *variable;

  if (filter->query.length == 0 && filter->filter.length == 0) {
    return true;
  }

  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, filter->condition.expression, variable);
  if (!filter->condition.expression) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "%s[condition] "
                     "failed to create expression for condition: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return false;
  }

  if (filter->query.length > 0) {
    if (filter->match_columns.length > 0) {
      GRN_EXPR_CREATE_FOR_QUERY(ctx,
                                table,
                                filter->condition.match_columns,
                                variable);
      if (!filter->condition.match_columns) {
        grn_rc rc = ctx->rc;
        if (rc == GRN_SUCCESS) {
          rc = GRN_NO_MEMORY_AVAILABLE;
        }
        GRN_PLUGIN_ERROR(ctx,
                         rc,
                         "%s[match_columns] "
                         "failed to create expression for match columns: "
                         "<%.*s>: %s",
                         log_tag_prefix,
                         (int)(filter->match_columns.length),
                         filter->match_columns.value,
                         ctx->errbuf);
        return false;
      }

      grn_expr_parse(ctx,
                     filter->condition.match_columns,
                     filter->match_columns.value,
                     filter->match_columns.length,
                     NULL,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT);
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }

    {
      grn_expr_flags flags;
      grn_obj query_expander_buf;
      const char *query = filter->query.value;
      unsigned int query_len = filter->query.length;

      flags = GRN_EXPR_SYNTAX_QUERY;
      if (filter->query_flags.length) {
        grn_obj query_flags;
        GRN_TEXT_INIT(&query_flags, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET(ctx,
                     &query_flags,
                     filter->query_flags.value,
                     filter->query_flags.length);
        flags |=
          grn_proc_expr_query_flags_parse(ctx, &query_flags, log_tag_prefix);
        GRN_OBJ_FIN(ctx, &query_flags);
        if (ctx->rc != GRN_SUCCESS) {
          return false;
        }
      } else {
        flags |= GRN_EXPR_ALLOW_PRAGMA | GRN_EXPR_ALLOW_COLUMN;
      }

      GRN_TEXT_INIT(&query_expander_buf, 0);
      if (filter->query_expander.length > 0) {
        grn_rc rc;
        rc = grn_proc_syntax_expand_query(ctx,
                                          filter->query.value,
                                          filter->query.length,
                                          flags,
                                          filter->query_expander.value,
                                          filter->query_expander.length,
                                          NULL,
                                          0,
                                          NULL,
                                          0,
                                          &query_expander_buf,
                                          log_tag_prefix);
        if (rc == GRN_SUCCESS) {
          query = GRN_TEXT_VALUE(&query_expander_buf);
          query_len = GRN_TEXT_LEN(&query_expander_buf);
        } else {
          GRN_OBJ_FIN(ctx, &query_expander_buf);
          return false;
        }
      }

      grn_expr_parse(ctx,
                     filter->condition.expression,
                     query,
                     query_len,
                     filter->condition.match_columns,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     flags);
      GRN_OBJ_FIN(ctx, &query_expander_buf);

      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }
  }

  if (filter->filter.length > 0) {
    grn_expr_parse(ctx,
                   filter->condition.expression,
                   filter->filter.value,
                   filter->filter.length,
                   filter->condition.match_columns,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }

    if (filter->query.length > 0) {
      grn_expr_append_op(ctx, filter->condition.expression, GRN_OP_AND, 2);
    }

    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
  }

  grn_expr_set_query_log_tag_prefix(ctx,
                                    filter->condition.expression,
                                    query_log_tag_prefix,
                                    -1);
  filter->filtered =
    grn_table_create(ctx,
                     NULL,
                     0,
                     NULL,
                     GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                     table,
                     NULL);
  if (!filter->filtered) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s[filter] failed to create result set table: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return false;
  }

  if (dynamic_columns->result_set) {
    grn_select_apply_dynamic_columns(ctx,
                                     data,
                                     filter->filtered,
                                     DynamicColumnStage::RESULT_SET,
                                     dynamic_columns->result_set,
                                     filter->condition.expression,
                                     log_tag_prefix,
                                     query_log_tag_prefix);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
  }

  if (filter->query_options.length > 0) {
    GRN_EXPR_CREATE_FOR_QUERY(ctx,
                              table,
                              filter->condition.query_options_expression,
                              variable);
    if (!filter->condition.expression) {
      grn_rc rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_NO_MEMORY_AVAILABLE;
      }
      GRN_PLUGIN_ERROR(ctx,
                       rc,
                       "%s[query-options] "
                       "failed to create expression for query options: %s",
                       log_tag_prefix,
                       ctx->errbuf);
      return false;
    }
    grn_expr_parse(ctx,
                   filter->condition.query_options_expression,
                   filter->query_options.value,
                   filter->query_options.length,
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_OPTIONS);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
    grn_expr *query_options_expression =
      (grn_expr *)(filter->condition.query_options_expression);
    grn_expr_set_query_options(ctx,
                               filter->condition.expression,
                               query_options_expression->codes[0].value);
  }

  {
    grn_table_selector table_selector;
    grn_table_selector_init(ctx,
                            &table_selector,
                            table,
                            filter->condition.expression,
                            GRN_OP_OR);
    filter->set_fuzzy_options(ctx, &table_selector);
    grn_table_selector_select(ctx, &table_selector, filter->filtered);
    grn_table_selector_fin(ctx, &table_selector);
  }

  return ctx->rc == GRN_SUCCESS;
}

static bool
grn_filter_execute_post_filter(grn_ctx *ctx,
                               grn_select_data *data,
                               Filter *filter,
                               grn_obj *table,
                               const char *log_tag_prefix,
                               const char *query_log_tag_prefix)
{
  if (filter->post_filter.length == 0) {
    return true;
  }

  grn_obj *variable;
  GRN_EXPR_CREATE_FOR_QUERY(ctx,
                            table,
                            filter->post_condition.expression,
                            variable);
  if (!filter->post_condition.expression) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "%s[post-condition] "
                     "failed to create expression for post condition: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return false;
  }

  grn_expr_parse(ctx,
                 filter->post_condition.expression,
                 filter->post_filter.value,
                 filter->post_filter.length,
                 NULL,
                 GRN_OP_MATCH,
                 GRN_OP_AND,
                 GRN_EXPR_SYNTAX_SCRIPT);
  if (ctx->rc != GRN_SUCCESS) {
    return false;
  }

  grn_expr_set_query_log_tag_prefix(ctx,
                                    filter->post_condition.expression,
                                    query_log_tag_prefix,
                                    -1);
  filter->post_filtered =
    grn_table_create(ctx,
                     NULL,
                     0,
                     NULL,
                     GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                     table,
                     NULL);
  if (!filter->post_filtered) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s[post-filter] failed to create result set table: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return false;
  }

  grn_table_select(ctx,
                   table,
                   filter->post_condition.expression,
                   filter->post_filtered,
                   GRN_OP_OR);

  return ctx->rc == GRN_SUCCESS;
}

grn_expr_flags
grn_proc_expr_query_flags_parse(grn_ctx *ctx,
                                grn_obj *query_flags,
                                const char *error_message_tag)
{
  grn_expr_flags flags = 0;
  if (grn_obj_is_text_family_bulk(ctx, query_flags)) {
    const char *current = GRN_TEXT_VALUE(query_flags);
    const char *end = current + GRN_TEXT_LEN(query_flags);

    while (current < end) {
      if (*current == '|' || *current == ' ') {
        current += 1;
        continue;
      }

#define CHECK_EXPR_FLAG(name)                                                  \
  if (((size_t)(end - current) >= (sizeof(#name) - 1)) &&                      \
      (memcmp(current, #name, sizeof(#name) - 1) == 0) &&                      \
      (((end - current) == (sizeof(#name) - 1)) ||                             \
       (current[sizeof(#name) - 1] == '|') ||                                  \
       (current[sizeof(#name) - 1] == ' '))) {                                 \
    flags |= GRN_EXPR_##name;                                                  \
    current += sizeof(#name) - 1;                                              \
    continue;                                                                  \
  }

      CHECK_EXPR_FLAG(ALLOW_PRAGMA);
      CHECK_EXPR_FLAG(ALLOW_COLUMN);
      CHECK_EXPR_FLAG(ALLOW_UPDATE);
      CHECK_EXPR_FLAG(ALLOW_LEADING_NOT);
      CHECK_EXPR_FLAG(QUERY_NO_SYNTAX_ERROR);
      CHECK_EXPR_FLAG(DISABLE_PREFIX_SEARCH);
      CHECK_EXPR_FLAG(DISABLE_AND_NOT);

#define GRN_EXPR_NONE 0
      CHECK_EXPR_FLAG(NONE);
#undef GNR_EXPR_NONE

      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s invalid query flag: <%.*s>",
                       error_message_tag,
                       (int)(end - current),
                       current);
      return 0;
#undef CHECK_EXPR_FLAG
    }
  } else if (grn_obj_is_vector(ctx, query_flags)) {
    uint32_t n_elements = grn_vector_size(ctx, query_flags);
    uint32_t i;
    for (i = 0; i < n_elements; i++) {
      const char *flag;
      grn_id domain;
      uint32_t flag_size =
        grn_vector_get_element_float(ctx, query_flags, i, &flag, NULL, &domain);
      if (!grn_type_id_is_text_family(ctx, domain)) {
        grn_obj value;
        GRN_VALUE_FIX_SIZE_INIT(&value, 0, domain);
        GRN_TEXT_SET(ctx, &value, flag, flag_size);
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, &value);
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s query flag must be string: %.*s",
                         error_message_tag,
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        GRN_OBJ_FIN(ctx, &value);
        return 0;
      }

#define CHECK_EXPR_FLAG(name)                                                  \
  if ((flag_size == (sizeof(#name) - 1)) &&                                    \
      (memcmp(flag, #name, sizeof(#name) - 1) == 0)) {                         \
    flags |= GRN_EXPR_##name;                                                  \
    continue;                                                                  \
  }

      CHECK_EXPR_FLAG(ALLOW_PRAGMA);
      CHECK_EXPR_FLAG(ALLOW_COLUMN);
      CHECK_EXPR_FLAG(ALLOW_UPDATE);
      CHECK_EXPR_FLAG(ALLOW_LEADING_NOT);
      CHECK_EXPR_FLAG(QUERY_NO_SYNTAX_ERROR);
      CHECK_EXPR_FLAG(DISABLE_PREFIX_SEARCH);
      CHECK_EXPR_FLAG(DISABLE_AND_NOT);

#define GRN_EXPR_NONE 0
      CHECK_EXPR_FLAG(NONE);
#undef GNR_EXPR_NONE

      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s invalid query flag: <%.*s>",
                       error_message_tag,
                       (int)flag_size,
                       flag);
      return 0;
#undef CHECK_EXPR_FLAG
    }
  } else {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, query_flags);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s must be string or string vector: %.*s",
                     error_message_tag,
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
  }

  return flags;
}

grn_bool
grn_proc_select_format_init(grn_ctx *ctx,
                            grn_obj_format *format,
                            grn_obj *result_set,
                            int n_hits,
                            int offset,
                            int limit,
                            const char *columns,
                            unsigned int columns_len,
                            grn_obj *condition)
{
  grn_rc rc;

  GRN_OBJ_FORMAT_INIT(format, n_hits, offset, limit, offset);
  format->flags =
    GRN_OBJ_FORMAT_WITH_COLUMN_NAMES | GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
  rc =
    grn_obj_format_set_columns(ctx, format, result_set, columns, columns_len);
  if (rc != GRN_SUCCESS) {
    grn_obj_format_fin(ctx, format);
    return GRN_FALSE;
  }

  grn_select_expression_set_condition(ctx, format->expression, condition);

  return ctx->rc == GRN_SUCCESS;
}

grn_bool
grn_proc_select_format_fin(grn_ctx *ctx, grn_obj_format *format)
{
  grn_obj_format_fin(ctx, format);

  return ctx->rc == GRN_SUCCESS;
}

grn_bool
grn_proc_select_output_columns_open(grn_ctx *ctx,
                                    grn_obj_format *format,
                                    grn_obj *res,
                                    int n_hits,
                                    int offset,
                                    int limit,
                                    const char *columns,
                                    unsigned int columns_len,
                                    grn_obj *condition,
                                    uint32_t n_additional_elements)
{
  grn_bool succeeded;

  if (!grn_proc_select_format_init(ctx,
                                   format,
                                   res,
                                   n_hits,
                                   offset,
                                   limit,
                                   columns,
                                   columns_len,
                                   condition)) {
    GRN_OUTPUT_RESULT_SET_OPEN(res, NULL, 0);
    GRN_OUTPUT_RESULT_SET_CLOSE(res, NULL);
    return GRN_FALSE;
  }

  GRN_OUTPUT_RESULT_SET_OPEN(res, format, n_additional_elements);
  succeeded = (ctx->rc == GRN_SUCCESS);
  if (!succeeded) {
    GRN_OUTPUT_RESULT_SET_CLOSE(res, format);
    grn_proc_select_format_fin(ctx, format);
  }

  return succeeded;
}

grn_bool
grn_proc_select_output_columns_close(grn_ctx *ctx,
                                     grn_obj_format *format,
                                     grn_obj *result_set)
{
  GRN_OUTPUT_RESULT_SET_CLOSE(result_set, format);

  return grn_proc_select_format_fin(ctx, format);
}

grn_bool
grn_proc_select_output_columns(grn_ctx *ctx,
                               grn_obj *res,
                               int n_hits,
                               int offset,
                               int limit,
                               const char *columns,
                               unsigned int columns_len,
                               grn_obj *condition)
{
  grn_obj_format format;
  uint32_t n_additional_elements = 0;

  if (!grn_proc_select_output_columns_open(ctx,
                                           &format,
                                           res,
                                           n_hits,
                                           offset,
                                           limit,
                                           columns,
                                           columns_len,
                                           condition,
                                           n_additional_elements)) {
    return GRN_FALSE;
  }

  return grn_proc_select_output_columns_close(ctx, &format, res);
}

static grn_bool
grn_select_output_columns_open(grn_ctx *ctx,
                               grn_select_data *data,
                               grn_obj_format *format,
                               grn_obj *res,
                               int n_hits,
                               int offset,
                               int limit,
                               const char *columns,
                               int columns_len,
                               grn_obj *condition,
                               uint32_t n_additional_elements)
{
  if (!grn_proc_select_output_columns_open(ctx,
                                           format,
                                           res,
                                           n_hits,
                                           offset,
                                           limit,
                                           columns,
                                           columns_len,
                                           condition,
                                           n_additional_elements)) {
    return GRN_FALSE;
  }

  if (format->expression) {
    data->cacheable *= ((grn_expr *)format->expression)->cacheable;
    data->taintable += ((grn_expr *)format->expression)->taintable;
  }

  return GRN_TRUE;
}

static grn_bool
grn_select_output_columns(grn_ctx *ctx,
                          grn_select_data *data,
                          grn_obj *res,
                          int n_hits,
                          int offset,
                          int limit,
                          const char *columns,
                          int columns_len,
                          grn_obj *condition)
{
  grn_obj_format format;
  uint32_t n_additional_elements = 0;

  if (!grn_select_output_columns_open(ctx,
                                      data,
                                      &format,
                                      res,
                                      n_hits,
                                      offset,
                                      limit,
                                      columns,
                                      columns_len,
                                      condition,
                                      n_additional_elements)) {
    return GRN_FALSE;
  }

  return grn_proc_select_output_columns_close(ctx, &format, res);
}

static grn_bool
grn_select_apply_initial_columns(grn_ctx *ctx, grn_select_data *data)
{
  if (!data->dynamic_columns.initial) {
    return GRN_TRUE;
  }

  data->tables.initial =
    grn_select_create_all_selected_result_table(ctx, data->tables.target);
  if (!data->tables.initial) {
    return GRN_FALSE;
  }

  grn_select_apply_dynamic_columns(ctx,
                                   data,
                                   data->tables.initial,
                                   DynamicColumnStage::INITIAL,
                                   data->dynamic_columns.initial,
                                   data->filter.condition.expression,
                                   "[select]",
                                   NULL);

  return ctx->rc == GRN_SUCCESS;
}

static grn_bool
grn_select_filter(grn_ctx *ctx, grn_select_data *data)
{
  if (!grn_filter_execute(ctx,
                          data,
                          &(data->filter),
                          data->tables.initial,
                          &(data->dynamic_columns),
                          "[select]",
                          "")) {
    return GRN_FALSE;
  }

  data->tables.result = data->filter.filtered;
  if (!data->tables.result) {
    data->tables.result = data->tables.initial;
  }

  {
    grn_expr *expression;
    expression = (grn_expr *)(data->filter.condition.expression);
    if (expression) {
      data->cacheable *= expression->cacheable;
      data->taintable += expression->taintable;
    }
  }

  return GRN_TRUE;
}

static grn_bool
grn_select_apply_filtered_dynamic_columns(grn_ctx *ctx, grn_select_data *data)
{
  if (!data->dynamic_columns.filtered) {
    return GRN_TRUE;
  }

  if (data->tables.result == data->tables.initial) {
    data->tables.result =
      grn_select_create_all_selected_result_table(ctx, data->tables.initial);
    if (!data->tables.result) {
      return GRN_FALSE;
    }
  }

  grn_select_apply_dynamic_columns(ctx,
                                   data,
                                   data->tables.result,
                                   DynamicColumnStage::FILTERED,
                                   data->dynamic_columns.filtered,
                                   data->filter.condition.expression,
                                   "[select]",
                                   NULL);

  return ctx->rc == GRN_SUCCESS;
}

static bool
grn_select_post_filter(grn_ctx *ctx, grn_select_data *data)
{
  if (!grn_filter_execute_post_filter(ctx,
                                      data,
                                      &(data->filter),
                                      data->tables.result,
                                      "[select]",
                                      NULL)) {
    return false;
  }

  if (data->filter.post_filtered) {
    data->tables.result = data->filter.post_filtered;
  }

  return true;
}

static int
grn_select_apply_adjuster_execute_ensure_factor(grn_ctx *ctx,
                                                grn_obj *factor_object)
{
  if (!factor_object) {
    return 1;
  } else if (factor_object->header.domain == GRN_DB_INT32) {
    return GRN_INT32_VALUE(factor_object);
  } else {
    grn_rc rc;
    grn_obj int32_object;
    int factor;
    GRN_INT32_INIT(&int32_object, 0);
    rc = grn_obj_cast(ctx, factor_object, &int32_object, GRN_FALSE);
    if (rc == GRN_SUCCESS) {
      factor = GRN_INT32_VALUE(&int32_object);
    } else {
      /* TODO: Log or return error? */
      factor = 1;
    }
    GRN_OBJ_FIN(ctx, &int32_object);
    return factor;
  }
}

static void
grn_select_apply_adjuster_execute_adjust(grn_ctx *ctx,
                                         grn_obj *table,
                                         grn_obj *column,
                                         grn_obj *value,
                                         grn_obj *factor)
{
  grn_obj *index;
  unsigned int n_indexes;
  int factor_value;

  n_indexes = grn_column_index(ctx, column, GRN_OP_MATCH, &index, 1, NULL);
  if (n_indexes == 0) {
    char column_name[GRN_TABLE_MAX_KEY_SIZE];
    int column_name_size;
    column_name_size =
      grn_obj_name(ctx, column, column_name, GRN_TABLE_MAX_KEY_SIZE);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "adjuster requires index column for the target column: "
                     "<%.*s>",
                     column_name_size,
                     column_name);
    return;
  }

  factor_value = grn_select_apply_adjuster_execute_ensure_factor(ctx, factor);

  {
    grn_search_optarg options;
    memset(&options, 0, sizeof(grn_search_optarg));

    options.mode = GRN_OP_EXACT;
    options.similarity_threshold = 0;
    options.max_interval = 0;
    options.additional_last_interval = 0;
    options.weight_vector = NULL;
    options.vector_size = factor_value;
    options.proc = NULL;
    options.max_size = 0;
    options.scorer = NULL;
    options.query_options = NULL;
    options.min_interval = NULL;

    grn_obj_search(ctx, index, value, table, GRN_OP_ADJUST, &options);
  }

  grn_obj_unref(ctx, index);
}

static void
grn_select_apply_adjuster_execute(grn_ctx *ctx,
                                  grn_obj *table,
                                  grn_obj *adjuster)
{
  grn_expr *expr = (grn_expr *)adjuster;
  grn_expr_code *code, *code_end;

  code = expr->codes;
  code_end = expr->codes + expr->codes_curr;
  while (code < code_end) {
    grn_obj *column, *value, *factor;

    if (code->op == GRN_OP_PLUS) {
      code++;
      continue;
    }

    column = code->value;
    code++;
    value = code->value;
    code++;
    code++; /* op == GRN_OP_MATCH */
    if ((code_end - code) >= 2 && code[1].op == GRN_OP_STAR) {
      factor = code->value;
      code++;
      code++; /* op == GRN_OP_STAR */
    } else {
      factor = NULL;
    }
    grn_select_apply_adjuster_execute_adjust(ctx, table, column, value, factor);
  }
}

static grn_bool
grn_select_apply_adjuster(grn_ctx *ctx,
                          grn_select_data *data,
                          grn_raw_string *adjuster_string,
                          grn_obj *target,
                          grn_obj *result,
                          const char *log_tag_prefix,
                          const char *query_log_tag_prefix)
{
  grn_obj *adjuster;
  grn_obj *record;
  grn_rc rc;

  if (adjuster_string->length == 0) {
    return GRN_TRUE;
  }

  GRN_EXPR_CREATE_FOR_QUERY(ctx, target, adjuster, record);
  if (!adjuster) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s[adjuster] "
                     "failed to create expression: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return GRN_FALSE;
  }

  rc = grn_expr_parse(ctx,
                      adjuster,
                      adjuster_string->value,
                      adjuster_string->length,
                      NULL,
                      GRN_OP_MATCH,
                      GRN_OP_ADJUST,
                      GRN_EXPR_SYNTAX_ADJUSTER);
  if (rc != GRN_SUCCESS) {
    grn_obj_unlink(ctx, adjuster);
    GRN_PLUGIN_ERROR(ctx,
                     rc,
                     "%s[adjuster] "
                     "failed to parse: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return GRN_FALSE;
  }

  data->cacheable *= ((grn_expr *)adjuster)->cacheable;
  data->taintable += ((grn_expr *)adjuster)->taintable;
  grn_select_apply_adjuster_execute(ctx, result, adjuster);
  grn_obj_unlink(ctx, adjuster);

  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_SIZE,
                ":",
                "%sadjust(%d)",
                query_log_tag_prefix ? query_log_tag_prefix : "",
                grn_table_size(ctx, result));

  return GRN_TRUE;
}

static grn_bool
grn_select_apply_scorer(grn_ctx *ctx, grn_select_data *data)
{
  grn_obj *scorer;
  grn_obj *record;
  grn_rc rc = GRN_SUCCESS;

  if (data->scorer.length == 0) {
    return GRN_TRUE;
  }

  GRN_EXPR_CREATE_FOR_QUERY(ctx, data->tables.result, scorer, record);
  if (!scorer) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[select][scorer] "
                     "failed to create expression: %s",
                     ctx->errbuf);
    return GRN_FALSE;
  }

  rc = grn_expr_parse(ctx,
                      scorer,
                      data->scorer.value,
                      data->scorer.length,
                      NULL,
                      GRN_OP_MATCH,
                      GRN_OP_AND,
                      GRN_EXPR_SYNTAX_SCRIPT | GRN_EXPR_ALLOW_UPDATE);
  if (rc != GRN_SUCCESS) {
    grn_obj_unlink(ctx, scorer);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[select][scorer] "
                     "failed to parse: %s",
                     ctx->errbuf);
    return GRN_FALSE;
  }

  data->cacheable *= ((grn_expr *)scorer)->cacheable;
  data->taintable += ((grn_expr *)scorer)->taintable;

  grn_expr_executor executor;
  rc = grn_expr_executor_init(ctx, &executor, scorer);
  if (rc == GRN_SUCCESS) {
    GRN_TABLE_EACH_BEGIN(ctx, data->tables.result, cursor, id)
    {
      grn_expr_executor_exec(ctx, &executor, id);
      if (ctx->rc) {
        rc = ctx->rc;
        GRN_PLUGIN_ERROR(ctx,
                         rc,
                         "[select][scorer] "
                         "failed to execute: <%.*s>: %s",
                         (int)(data->scorer.length),
                         data->scorer.value,
                         ctx->errbuf);
        break;
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    grn_expr_executor_fin(ctx, &executor);
  }
  grn_obj_unlink(ctx, scorer);

  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_SIZE,
                ":",
                "score(%d): %.*s",
                grn_table_size(ctx, data->tables.result),
                (int)(data->scorer.length),
                data->scorer.value);

  return rc == GRN_SUCCESS;
}

static grn_bool
grn_select_sort(grn_ctx *ctx, grn_select_data *data)
{
  grn_table_sort_key *keys;
  uint32_t n_keys;

  if (data->sort_keys.length == 0) {
    return GRN_TRUE;
  }

  keys = grn_table_sort_keys_parse(ctx,
                                   data->tables.result,
                                   data->sort_keys.value,
                                   data->sort_keys.length,
                                   &n_keys);
  if (!keys) {
    if (ctx->rc == GRN_SUCCESS) {
      return GRN_TRUE;
    } else {
      GRN_PLUGIN_ERROR(ctx,
                       ctx->rc,
                       "[select][sort] "
                       "failed to parse: <%.*s>: %s",
                       (int)(data->sort_keys.length),
                       data->sort_keys.value,
                       ctx->errbuf);
      return GRN_FALSE;
    }
  }

  data->tables.sorted = grn_table_create(ctx,
                                         NULL,
                                         0,
                                         NULL,
                                         GRN_OBJ_TABLE_NO_KEY,
                                         NULL,
                                         data->tables.result);
  if (!data->tables.sorted) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "[select][sort] "
                     "failed to create table to store sorted record: "
                     "<%.*s>: %s",
                     (int)(data->sort_keys.length),
                     data->sort_keys.value,
                     ctx->errbuf);
    return GRN_FALSE;
  }

  grn_table_sort(ctx,
                 data->tables.result,
                 data->offset,
                 data->limit,
                 data->tables.sorted,
                 keys,
                 n_keys);

  grn_table_sort_key_close(ctx, keys, n_keys);

  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_SIZE,
                ":",
                "sort(%d): %.*s",
                data->limit,
                (int)(data->sort_keys.length),
                data->sort_keys.value);

  return ctx->rc == GRN_SUCCESS;
}

static bool
grn_select_load(grn_ctx *ctx, grn_select_data *data)
{
  grn_obj *table = NULL;
  grn_obj columns;
  grn_obj ranges;
  grn_obj indexes;
  grn_obj *output_columns = NULL;

  if (data->load.table.length == 0) {
    return true;
  }
  if (data->load.columns.length == 0) {
    return true;
  }
  if (data->load.values.length == 0) {
    return true;
  }

  table = grn_ctx_get(ctx, data->load.table.value, data->load.table.length);
  if (!table) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "[select][load] "
                     "nonexistent load table: <%.*s>",
                     (int)(data->load.table.length),
                     data->load.table.value);
    return false;
  }

  GRN_PTR_INIT(&columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(&ranges, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(&indexes, GRN_OBJ_VECTOR, GRN_ID_NIL);

  grn_table_parse_load_columns(ctx,
                               table,
                               data->load.columns.value,
                               data->load.columns.length,
                               &columns);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "[select][load] "
                     "failed to parse columns: <%.*s>: %s",
                     (int)(data->load.columns.length),
                     data->load.columns.value,
                     ctx->errbuf);
    goto exit;
  }

  {
    size_t i;
    size_t n_columns = GRN_PTR_VECTOR_SIZE(&columns);
    for (i = 0; i < n_columns; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(&columns, i);
      grn_obj *range = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));
      if (grn_obj_is_table(ctx, range)) {
        GRN_PTR_PUT(ctx, &ranges, range);
        grn_column_get_all_index_columns(ctx, range, &indexes);
      } else {
        grn_obj_unref(ctx, range);
      }
      grn_column_get_all_index_columns(ctx, column, &indexes);
    }
  }

  output_columns = grn_output_columns_parse(ctx,
                                            data->tables.result,
                                            data->load.values.value,
                                            data->load.values.length);
  if (!output_columns) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "[select][load] "
                     "failed to parse values: <%.*s>: %s",
                     (int)(data->load.values.length),
                     data->load.values.value,
                     ctx->errbuf);
    goto exit;
  }

  grn_output_columns_apply(ctx, output_columns, &columns);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "[select][load] "
                     "failed to load: <%.*s>: %s",
                     (int)(data->load.values.length),
                     data->load.values.value,
                     ctx->errbuf);
  }

  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_SIZE,
                ":",
                "load(%d): [%.*s][%d]",
                grn_table_size(ctx, data->tables.result),
                (int)(data->load.table.length),
                data->load.table.value,
                grn_table_size(ctx, table));

exit:
  if (output_columns) {
    grn_obj_close(ctx, output_columns);
  }
  {
    size_t i;
    size_t n_ranges = GRN_PTR_VECTOR_SIZE(&ranges);
    for (i = 0; i < n_ranges; i++) {
      grn_obj *range = GRN_PTR_VALUE_AT(&ranges, i);
      grn_obj_unref(ctx, range);
    }
    GRN_OBJ_FIN(ctx, &ranges);
  }
  {
    size_t i;
    size_t n_indexes = GRN_PTR_VECTOR_SIZE(&indexes);
    for (i = 0; i < n_indexes; i++) {
      grn_obj *index = GRN_PTR_VALUE_AT(&indexes, i);
      grn_obj_unref(ctx, index);
    }
    GRN_OBJ_FIN(ctx, &indexes);
  }
  {
    size_t i;
    size_t n_columns = GRN_PTR_VECTOR_SIZE(&columns);
    for (i = 0; i < n_columns; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(&columns, i);
      if (grn_obj_is_accessor(ctx, column)) {
        grn_obj_close(ctx, column);
      } else {
        grn_obj_unref(ctx, column);
      }
    }
    GRN_OBJ_FIN(ctx, &columns);
  }
  if (table) {
    grn_obj_unref(ctx, table);
  }

  return ctx->rc == GRN_SUCCESS;
}

static grn_bool
grn_select_apply_output_dynamic_columns(grn_ctx *ctx, grn_select_data *data)
{
  if (!data->dynamic_columns.output) {
    return GRN_TRUE;
  }

  if (!data->tables.sorted) {
    data->tables.sorted =
      grn_select_create_no_sort_keys_sorted_table(ctx,
                                                  data,
                                                  data->tables.result);
    if (!data->tables.sorted) {
      return GRN_FALSE;
    }
  }

  grn_select_apply_dynamic_columns(ctx,
                                   data,
                                   data->tables.sorted,
                                   DynamicColumnStage::OUTPUT,
                                   data->dynamic_columns.output,
                                   data->filter.condition.expression,
                                   "[select]",
                                   NULL);

  return ctx->rc == GRN_SUCCESS;
}

static grn_bool
grn_select_output_match_open(grn_ctx *ctx,
                             grn_select_data *data,
                             grn_obj_format *format,
                             uint32_t n_additional_elements)
{
  grn_bool succeeded = GRN_TRUE;
  int offset;
  grn_obj *output_table;

  if (!data->output_columns.value) {
    data->output_columns = data->default_output_columns;
  }
  if (data->tables.sorted) {
    offset = 0;
    output_table = data->tables.sorted;
  } else {
    offset = data->offset;
    output_table = data->tables.result;
  }
  succeeded =
    grn_select_output_columns_open(ctx,
                                   data,
                                   format,
                                   output_table,
                                   grn_table_size(ctx, data->tables.result),
                                   offset,
                                   data->limit,
                                   data->output_columns.value,
                                   data->output_columns.length,
                                   data->filter.condition.expression,
                                   n_additional_elements);
  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE, ":", "output(%d)", data->limit);

  return succeeded;
}

static grn_bool
grn_select_output_match_close(grn_ctx *ctx,
                              grn_select_data *data,
                              grn_obj_format *format)
{
  grn_obj *output_table;

  if (data->tables.sorted) {
    output_table = data->tables.sorted;
  } else {
    output_table = data->tables.result;
  }

  return grn_proc_select_output_columns_close(ctx, format, output_table);
}

static grn_bool
grn_select_output_match(grn_ctx *ctx, grn_select_data *data)
{
  grn_obj_format format;
  uint32_t n_additional_elements = 0;

  if (!grn_select_output_match_open(ctx,
                                    data,
                                    &format,
                                    n_additional_elements)) {
    return GRN_FALSE;
  }

  return grn_select_output_match_close(ctx, data, &format);
}

static bool
grn_select_output_drilldowns(grn_ctx *ctx,
                             grn_select_data *data,
                             Drilldowns &drilldowns,
                             const bool is_labeled,
                             grn_obj *condition,
                             const char *log_tag_prefix,
                             const char *query_log_tag_prefix)
{
  bool succeeded = true;
  unsigned int n_available_results = 0;

  if (drilldowns.empty()) {
    return true;
  }

  data->output.formatter->drilldowns_label(ctx, data);

  for (auto drilldown : drilldowns) {
    if (drilldown->result.table) {
      n_available_results++;
    }
  }

  data->output.formatter->drilldowns_open(ctx, data, n_available_results);

  for (auto drilldown : drilldowns) {
    if (!drilldown->result.table) {
      continue;
    }

    grn_obj *target_table;
    if (drilldown->filtered_result) {
      target_table = drilldown->filtered_result;
    } else {
      target_table = drilldown->result.table;
    }

    auto n_hits = grn_table_size(ctx, target_table);

    auto offset = drilldown->offset;
    auto limit = drilldown->limit;
    grn_output_range_normalize(ctx, n_hits, &offset, &limit);

    char drilldown_log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    char drilldown_query_log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    char drilldown_output_query_log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    if (is_labeled) {
      grn_snprintf(drilldown_log_tag_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%s[drilldowns][%.*s]",
                   log_tag_prefix,
                   (int)(drilldown->label.length),
                   drilldown->label.value);
      grn_snprintf(drilldown_query_log_tag_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%sdrilldowns[%.*s].",
                   query_log_tag_prefix,
                   (int)(drilldown->label.length),
                   drilldown->label.value);
      grn_snprintf(drilldown_output_query_log_tag_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%soutput.drilldowns[%.*s]",
                   query_log_tag_prefix,
                   (int)(drilldown->label.length),
                   drilldown->label.value);
    } else {
      grn_snprintf(drilldown_log_tag_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%s[drilldown]",
                   log_tag_prefix);
      grn_snprintf(drilldown_query_log_tag_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%sdrilldown.",
                   query_log_tag_prefix);
      grn_snprintf(drilldown_output_query_log_tag_prefix,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%soutput.drilldown",
                   query_log_tag_prefix);
    }

    if (drilldown->sort_keys.length > 0) {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_keys_parse(ctx,
                                            target_table,
                                            drilldown->sort_keys.value,
                                            drilldown->sort_keys.length,
                                            &n_sort_keys);
      if (sort_keys) {
        grn_obj *sorted;
        sorted = grn_table_create(ctx,
                                  NULL,
                                  0,
                                  NULL,
                                  GRN_OBJ_TABLE_NO_KEY,
                                  NULL,
                                  target_table);
        if (sorted) {
          grn_table_sort(ctx,
                         target_table,
                         offset,
                         limit,
                         sorted,
                         sort_keys,
                         n_sort_keys);
          GRN_QUERY_LOG(ctx,
                        GRN_QUERY_LOG_SIZE,
                        ":",
                        "%ssort(%d): %.*s",
                        drilldown_query_log_tag_prefix,
                        limit,
                        (int)(drilldown->sort_keys.length),
                        drilldown->sort_keys.value);

          if (drilldown->dynamic_columns.output) {
            grn_select_apply_dynamic_columns(ctx,
                                             data,
                                             sorted,
                                             DynamicColumnStage::OUTPUT,
                                             drilldown->dynamic_columns.output,
                                             condition,
                                             drilldown_log_tag_prefix,
                                             drilldown_query_log_tag_prefix);
            succeeded = (ctx->rc == GRN_SUCCESS);
          }

          if (succeeded) {
            data->output.formatter->drilldown_label(ctx, data, drilldown);
            succeeded =
              grn_select_output_columns(ctx,
                                        data,
                                        sorted,
                                        n_hits,
                                        0,
                                        limit,
                                        drilldown->output_columns.value,
                                        drilldown->output_columns.length,
                                        condition);
          }
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
      } else {
        succeeded = false;
      }
    } else {
      grn_obj *sorted = NULL;
      if (drilldown->dynamic_columns.output) {
        sorted =
          grn_select_create_no_sort_keys_sorted_table(ctx, data, target_table);
        if (!sorted) {
          succeeded = false;
        } else {
          grn_select_apply_dynamic_columns(ctx,
                                           data,
                                           sorted,
                                           DynamicColumnStage::OUTPUT,
                                           drilldown->dynamic_columns.output,
                                           condition,
                                           drilldown_log_tag_prefix,
                                           drilldown_query_log_tag_prefix);
          succeeded = (ctx->rc == GRN_SUCCESS);
        }
        target_table = sorted;
      }

      if (succeeded) {
        data->output.formatter->drilldown_label(ctx, data, drilldown);
        succeeded = grn_select_output_columns(ctx,
                                              data,
                                              target_table,
                                              n_hits,
                                              offset,
                                              limit,
                                              drilldown->output_columns.value,
                                              drilldown->output_columns.length,
                                              condition);
      }
      if (sorted) {
        grn_obj_unlink(ctx, sorted);
      }
    }

    if (!succeeded) {
      break;
    }

    GRN_QUERY_LOG(ctx,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "%s(%d)",
                  drilldown_output_query_log_tag_prefix,
                  n_hits);
  }

  data->output.formatter->drilldowns_close(ctx, data);

  return succeeded;
}

static grn_bool
grn_select_output_slices(grn_ctx *ctx, grn_select_data *data)
{
  grn_bool succeeded = GRN_TRUE;
  unsigned int n_available_results = 0;

  if (data->slices.empty()) {
    return GRN_TRUE;
  }

  data->output.formatter->slices_label(ctx, data);

  for (auto slice : data->slices) {
    if (slice->tables.result) {
      n_available_results++;
    }
  }

  data->output.formatter->slices_open(ctx, data, n_available_results);

  for (auto slice : data->slices) {
    if (!slice->tables.result) {
      continue;
    }

    auto n_hits = grn_table_size(ctx, slice->tables.result);

    auto offset = slice->offset;
    auto limit = slice->limit;
    grn_output_range_normalize(ctx, n_hits, &offset, &limit);

    char log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(log_tag_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[select][slices][%.*s]",
                 static_cast<int>(slice->label.length),
                 slice->label.value);
    char query_log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(query_log_tag_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "slices[%.*s].",
                 static_cast<int>(slice->label.length),
                 slice->label.value);

    if (slice->sort_keys.length == 0) {
      slice->tables.output = slice->tables.result;
    } else {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_keys_parse(ctx,
                                            slice->tables.result,
                                            slice->sort_keys.value,
                                            slice->sort_keys.length,
                                            &n_sort_keys);
      if (sort_keys) {
        slice->tables.sorted = grn_table_create(ctx,
                                                NULL,
                                                0,
                                                NULL,
                                                GRN_OBJ_TABLE_NO_KEY,
                                                NULL,
                                                slice->tables.result);
        if (slice->tables.sorted) {
          grn_table_sort(ctx,
                         slice->tables.result,
                         offset,
                         limit,
                         slice->tables.sorted,
                         sort_keys,
                         n_sort_keys);
          GRN_QUERY_LOG(ctx,
                        GRN_QUERY_LOG_SIZE,
                        ":",
                        "%ssort(%d): %.*s",
                        query_log_tag_prefix,
                        limit,
                        (int)(slice->sort_keys.length),
                        slice->sort_keys.value);
          offset = 0;
          slice->tables.output = slice->tables.sorted;
        } else {
          succeeded = false;
        }
        grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
      } else {
        succeeded = false;
      }

      if (!succeeded) {
        break;
      }
    }

    if (slice->dynamic_columns.output) {
      grn_select_apply_dynamic_columns(ctx,
                                       data,
                                       slice->tables.output,
                                       DynamicColumnStage::OUTPUT,
                                       slice->dynamic_columns.output,
                                       slice->filter.condition.expression,
                                       log_tag_prefix,
                                       query_log_tag_prefix);
      if (ctx->rc != GRN_SUCCESS) {
        succeeded = false;
        break;
      }
    }

    data->output.formatter->slice_label(ctx, data, slice);
    grn_obj_format format;
    uint32_t n_additional_elements = 0;
    if (!slice->drilldowns.empty()) {
      n_additional_elements++;
    }
    if (slice->output_columns.length == 0) {
      slice->output_columns = data->default_output_columns;
    }
    succeeded =
      grn_select_output_columns_open(ctx,
                                     data,
                                     &format,
                                     slice->tables.output,
                                     n_hits,
                                     offset,
                                     limit,
                                     slice->output_columns.value,
                                     slice->output_columns.length,
                                     slice->filter.condition.expression,
                                     n_additional_elements);
    if (!succeeded) {
      break;
    }
    succeeded = grn_select_output_drilldowns(ctx,
                                             data,
                                             slice->drilldowns,
                                             true,
                                             slice->filter.condition.expression,
                                             log_tag_prefix,
                                             query_log_tag_prefix);
    if (!succeeded) {
      break;
    }

    succeeded =
      grn_proc_select_output_columns_close(ctx, &format, slice->tables.output);
    if (!succeeded) {
      break;
    }

    GRN_QUERY_LOG(ctx,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "slices[%.*s].output(%d)",
                  (int)(slice->label.length),
                  slice->label.value,
                  limit);
  }

  data->output.formatter->slices_close(ctx, data);

  return succeeded;
}

static bool
grn_select_data_output_drilldowns(grn_ctx *ctx, grn_select_data *data)
{
  return grn_select_output_drilldowns(ctx,
                                      data,
                                      data->drilldowns,
                                      data->drilldowns.is_labeled(),
                                      data->filter.condition.expression,
                                      "[select]",
                                      "");
}

static grn_bool
grn_select_output(grn_ctx *ctx, grn_select_data *data)
{
  bool succeeded = true;

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    GRN_OUTPUT_ARRAY_OPEN("RESULT", data->output.n_elements);
    succeeded = grn_select_output_match(ctx, data);
    if (succeeded) {
      succeeded = grn_select_output_slices(ctx, data);
    }
    if (succeeded) {
      succeeded = grn_select_data_output_drilldowns(ctx, data);
    }
    GRN_OUTPUT_ARRAY_CLOSE();
  } else {
    grn_obj_format format;
    uint32_t n_additional_elements = 0;

    if (!data->slices.empty()) {
      n_additional_elements++;
    }
    if (!data->drilldowns.empty()) {
      n_additional_elements++;
    }

    succeeded =
      grn_select_output_match_open(ctx, data, &format, n_additional_elements);
    if (succeeded) {
      succeeded = grn_select_output_slices(ctx, data);
      if (succeeded) {
        succeeded = grn_select_data_output_drilldowns(ctx, data);
      }
      if (!grn_select_output_match_close(ctx, data, &format)) {
        succeeded = GRN_FALSE;
      }
    }
  }

  return succeeded;
}

static void
grn_select_output_slices_label_v1(grn_ctx *ctx, grn_select_data *data)
{
}

static void
grn_select_output_slices_open_v1(grn_ctx *ctx,
                                 grn_select_data *data,
                                 unsigned int n_result_sets)
{
  GRN_OUTPUT_MAP_OPEN("SLICES", n_result_sets);
}

static void
grn_select_output_slices_close_v1(grn_ctx *ctx, grn_select_data *data)
{
  GRN_OUTPUT_MAP_CLOSE();
}

static void
grn_select_output_slice_label_v1(grn_ctx *ctx,
                                 grn_select_data *data,
                                 Slice *slice)
{
  GRN_OUTPUT_STR(slice->label.value, slice->label.length);
}

static void
grn_select_output_drilldowns_label_v1(grn_ctx *ctx, grn_select_data *data)
{
}

static void
grn_select_output_drilldowns_open_v1(grn_ctx *ctx,
                                     grn_select_data *data,
                                     unsigned int n_result_sets)
{
  if (data->drilldowns.is_labeled()) {
    GRN_OUTPUT_MAP_OPEN("DRILLDOWNS", n_result_sets);
  }
}

static void
grn_select_output_drilldowns_close_v1(grn_ctx *ctx, grn_select_data *data)
{
  if (data->drilldowns.is_labeled()) {
    GRN_OUTPUT_MAP_CLOSE();
  }
}

static void
grn_select_output_drilldown_label_v1(grn_ctx *ctx,
                                     grn_select_data *data,
                                     Drilldown *drilldown)
{
  if (data->drilldowns.is_labeled()) {
    GRN_OUTPUT_STR(drilldown->label.value, drilldown->label.length);
  }
}

static grn_select_output_formatter grn_select_output_formatter_v1 = {
  grn_select_output_slices_label_v1,
  grn_select_output_slices_open_v1,
  grn_select_output_slices_close_v1,
  grn_select_output_slice_label_v1,
  grn_select_output_drilldowns_label_v1,
  grn_select_output_drilldowns_open_v1,
  grn_select_output_drilldowns_close_v1,
  grn_select_output_drilldown_label_v1};

static void
grn_select_output_slices_label_v3(grn_ctx *ctx, grn_select_data *data)
{
  GRN_OUTPUT_CSTR("slices");
}

static void
grn_select_output_slices_open_v3(grn_ctx *ctx,
                                 grn_select_data *data,
                                 unsigned int n_result_sets)
{
  GRN_OUTPUT_MAP_OPEN("slices", n_result_sets);
}

static void
grn_select_output_slices_close_v3(grn_ctx *ctx, grn_select_data *data)
{
  GRN_OUTPUT_MAP_CLOSE();
}

static void
grn_select_output_slice_label_v3(grn_ctx *ctx,
                                 grn_select_data *data,
                                 Slice *slice)
{
  GRN_OUTPUT_STR(slice->label.value, slice->label.length);
}

static void
grn_select_output_drilldowns_label_v3(grn_ctx *ctx, grn_select_data *data)
{
  GRN_OUTPUT_CSTR("drilldowns");
}

static void
grn_select_output_drilldowns_open_v3(grn_ctx *ctx,
                                     grn_select_data *data,
                                     unsigned int n_result_sets)
{
  GRN_OUTPUT_MAP_OPEN("drilldowns", n_result_sets);
}

static void
grn_select_output_drilldowns_close_v3(grn_ctx *ctx, grn_select_data *data)
{
  GRN_OUTPUT_MAP_CLOSE();
}

static void
grn_select_output_drilldown_label_v3(grn_ctx *ctx,
                                     grn_select_data *data,
                                     Drilldown *drilldown)
{
  GRN_OUTPUT_STR(drilldown->label.value, drilldown->label.length);
}

static grn_select_output_formatter grn_select_output_formatter_v3 = {
  grn_select_output_slices_label_v3,
  grn_select_output_slices_open_v3,
  grn_select_output_slices_close_v3,
  grn_select_output_slice_label_v3,
  grn_select_output_drilldowns_label_v3,
  grn_select_output_drilldowns_open_v3,
  grn_select_output_drilldowns_close_v3,
  grn_select_output_drilldown_label_v3};

static void
grn_select_prepare_cache_key(grn_ctx *ctx,
                             grn_select_data *data,
                             grn::TextBulk &cache_key)
{
#define PUT_CACHE_KEY(string)                                                  \
  if ((string).length > 0) {                                                   \
    GRN_TEXT_PUT(ctx, *cache_key, (string).value, (string).length);            \
  }                                                                            \
  GRN_TEXT_PUTC(ctx, *cache_key, '\0');                                        \
  if (GRN_TEXT_LEN(*cache_key) > GRN_CACHE_MAX_KEY_SIZE) {                     \
    return;                                                                    \
  }

  PUT_CACHE_KEY(data->table);
  PUT_CACHE_KEY(data->filter.match_columns);
  PUT_CACHE_KEY(data->filter.query);
  PUT_CACHE_KEY(data->filter.filter);
  PUT_CACHE_KEY(data->filter.post_filter);
  PUT_CACHE_KEY(data->scorer);
  PUT_CACHE_KEY(data->sort_keys);
  PUT_CACHE_KEY(data->output_columns);
#define PUT_CACHE_KEY_DRILLDOWN(drilldown)                                     \
  do {                                                                         \
    PUT_CACHE_KEY(drilldown->keys);                                            \
    PUT_CACHE_KEY(drilldown->sort_keys);                                       \
    PUT_CACHE_KEY(drilldown->output_columns);                                  \
    PUT_CACHE_KEY(drilldown->label);                                           \
    PUT_CACHE_KEY(drilldown->calc_target_name);                                \
    PUT_CACHE_KEY(drilldown->filter);                                          \
    PUT_CACHE_KEY(drilldown->adjuster);                                        \
    PUT_CACHE_KEY(drilldown->table_name);                                      \
    GRN_INT32_PUT(ctx, *cache_key, drilldown->offset);                         \
    GRN_INT32_PUT(ctx, *cache_key, drilldown->limit);                          \
    GRN_INT32_PUT(ctx, *cache_key, drilldown->max_n_target_records);           \
    GRN_TEXT_PUT(ctx,                                                          \
                 *cache_key,                                                   \
                 &(drilldown->calc_types),                                     \
                 sizeof(grn_table_group_flags));                               \
    PUT_CACHE_KEY(drilldown->key_vector_expansion);                            \
  } while (false)
  for (auto slice : data->slices) {
    PUT_CACHE_KEY(slice->filter.match_columns);
    PUT_CACHE_KEY(slice->filter.query);
    PUT_CACHE_KEY(slice->filter.query_expander);
    PUT_CACHE_KEY(slice->filter.query_flags);
    PUT_CACHE_KEY(slice->filter.filter);
    PUT_CACHE_KEY(slice->filter.post_filter);
    PUT_CACHE_KEY(slice->sort_keys);
    PUT_CACHE_KEY(slice->output_columns);
    PUT_CACHE_KEY(slice->label);
    GRN_INT32_PUT(ctx, *cache_key, slice->offset);
    GRN_INT32_PUT(ctx, *cache_key, slice->limit);
    for (auto drilldown : slice->drilldowns) {
      PUT_CACHE_KEY_DRILLDOWN(drilldown);
    }
  }
#define PUT_CACHE_KEY_COLUMNS(dynamic_columns)                                 \
  do {                                                                         \
    GRN_HASH_EACH_BEGIN(ctx, dynamic_columns, cursor, id)                      \
    {                                                                          \
      void *value;                                                             \
      grn_hash_cursor_get_value(ctx, cursor, &value);                          \
      auto dynamic_column = static_cast<DynamicColumn *>(value);               \
      PUT_CACHE_KEY(dynamic_column->label);                                    \
      GRN_TEXT_PUT(ctx,                                                        \
                   *cache_key,                                                 \
                   &(dynamic_column->stage),                                   \
                   sizeof(DynamicColumnStage));                                \
      GRN_RECORD_PUT(ctx, *cache_key, DB_OBJ(dynamic_column->type)->id);       \
      GRN_TEXT_PUT(ctx,                                                        \
                   *cache_key,                                                 \
                   &(dynamic_column->flags),                                   \
                   sizeof(grn_column_flags));                                  \
      PUT_CACHE_KEY(dynamic_column->value);                                    \
      PUT_CACHE_KEY(dynamic_column->window.sort_keys);                         \
      PUT_CACHE_KEY(dynamic_column->window.group_keys);                        \
    }                                                                          \
    GRN_HASH_EACH_END(ctx, cursor);                                            \
  } while (false)
  if (data->dynamic_columns.initial) {
    PUT_CACHE_KEY_COLUMNS(data->dynamic_columns.initial);
  }
  if (data->dynamic_columns.filtered) {
    PUT_CACHE_KEY_COLUMNS(data->dynamic_columns.filtered);
  }
  if (data->dynamic_columns.output) {
    PUT_CACHE_KEY_COLUMNS(data->dynamic_columns.output);
  }
#undef PUT_CACHE_KEY_COLUMNS
  if (!data->drilldowns.is_labeled()) {
    // TODO: Can we reuse parsed data here?
    Drilldown drilldown(ctx,
                        data,
                        data->args_,
                        "drilldown_",
                        nullptr,
                        {nullptr, 0});
    PUT_CACHE_KEY_DRILLDOWN((&drilldown));
  }
  for (auto drilldown : data->drilldowns) {
    PUT_CACHE_KEY_DRILLDOWN(drilldown);
  }
#undef PUT_CACHE_KEY_DRILLDOWN
  PUT_CACHE_KEY(data->match_escalation_threshold);
  PUT_CACHE_KEY(data->filter.query_expander);
  PUT_CACHE_KEY(data->filter.query_flags);
  PUT_CACHE_KEY(data->adjuster);
  PUT_CACHE_KEY(data->match_escalation);
  PUT_CACHE_KEY(data->load.table);
  PUT_CACHE_KEY(data->load.columns);
  PUT_CACHE_KEY(data->load.values);
  GRN_TEXT_PUT(ctx,
               *cache_key,
               &(ctx->impl->output.type),
               sizeof(grn_content_type));
  GRN_INT32_PUT(ctx, *cache_key, data->offset);
  GRN_INT32_PUT(ctx, *cache_key, data->limit);
  GRN_TEXT_PUT(ctx,
               *cache_key,
               &(ctx->impl->command.version),
               sizeof(grn_command_version));
  GRN_BOOL_PUT(ctx, *cache_key, &(ctx->impl->output.is_pretty));
#undef PUT_CACHE_KEY
}

namespace {
  grn_rc
  SelectExecutor::execute()
  {
    auto ctx = ctx_;
    auto data = this;

    uint32_t nhits;
    grn_obj *outbuf = ctx->impl->output.buf;
    grn::TextBulk cache_key(ctx);
    long long int original_match_escalation_threshold = 0;
    grn_bool original_force_match_escalation = GRN_FALSE;
    grn_cache *cache_obj = grn_cache_current_get(ctx);

    if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
      data->output.formatter = &grn_select_output_formatter_v1;
    } else {
      data->output.formatter = &grn_select_output_formatter_v3;
    }

    data->cacheable = 1;
    data->taintable = 0;

    data->output.n_elements = 0;

    grn_raw_string_lstrip(ctx, &(data->filter.query));

    grn_select_prepare_cache_key(ctx, data, cache_key);

    if (GRN_TEXT_LEN(*cache_key) <= GRN_CACHE_MAX_KEY_SIZE) {
      grn_rc rc;
      rc = grn_cache_fetch(ctx,
                           cache_obj,
                           GRN_TEXT_VALUE(*cache_key),
                           GRN_TEXT_LEN(*cache_key),
                           outbuf);
      if (rc == GRN_SUCCESS) {
        GRN_QUERY_LOG(ctx,
                      GRN_QUERY_LOG_CACHE,
                      ":",
                      "cache(%" GRN_FMT_LLD ")",
                      (long long int)GRN_TEXT_LEN(outbuf));
        return ctx->rc;
      }
    }

    original_match_escalation_threshold =
      grn_ctx_get_match_escalation_threshold(ctx);
    original_force_match_escalation = grn_ctx_get_force_match_escalation(ctx);

    if (data->match_escalation_threshold.length > 0) {
      const char *end, *rest;
      long long int threshold;
      end = data->match_escalation_threshold.value +
            data->match_escalation_threshold.length;
      threshold = grn_atoll(data->match_escalation_threshold.value, end, &rest);
      if (end == rest) {
        grn_ctx_set_match_escalation_threshold(ctx, threshold);
      }
    }
    if (data->match_escalation.length > 0) {
      if (GRN_RAW_STRING_EQUAL_CSTRING(data->match_escalation, "auto")) {
        grn_ctx_set_force_match_escalation(ctx, GRN_FALSE);
      } else if (GRN_RAW_STRING_EQUAL_CSTRING(data->match_escalation, "yes")) {
        grn_ctx_set_force_match_escalation(ctx, GRN_TRUE);
      } else if (GRN_RAW_STRING_EQUAL_CSTRING(data->match_escalation, "no")) {
        grn_ctx_set_force_match_escalation(ctx, GRN_FALSE);
        grn_ctx_set_match_escalation_threshold(ctx, -1);
      }
    }

    data->tables.target =
      grn_ctx_get(ctx, data->table.value, data->table.length);
    if (!data->tables.target) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][table] invalid name: <%.*s>",
                       (int)(data->table.length),
                       data->table.value);
      goto exit;
    }
    if (grn_obj_is_table_with_key(ctx, data->tables.target) ||
        grn_obj_is_table_with_value(ctx, data->tables.target)) {
      data->default_output_columns.value =
        GRN_SELECT_DEFAULT_OUTPUT_COLUMNS_FOR_WITH_KEY;
    } else {
      data->default_output_columns.value =
        GRN_SELECT_DEFAULT_OUTPUT_COLUMNS_FOR_NO_KEY;
    }
    data->default_output_columns.length =
      strlen(data->default_output_columns.value);

    {
      data->tables.initial = data->tables.target;
      if (!grn_select_apply_initial_columns(ctx, data)) {
        goto exit;
      }

      if (!grn_select_filter(ctx, data)) {
        goto exit;
      }

      nhits = grn_table_size(ctx, data->tables.result);
      GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE, ":", "select(%d)", nhits);

      if (!grn_select_apply_filtered_dynamic_columns(ctx, data)) {
        goto exit;
      }

      if (!grn_select_post_filter(ctx, data)) {
        goto exit;
      }

      {
        grn_bool succeeded;

        /* For select results */
        data->output.n_elements = 1;

        if (!grn_select_apply_adjuster(ctx,
                                       data,
                                       &(data->adjuster),
                                       data->tables.target,
                                       data->tables.result,
                                       "[select]",
                                       NULL)) {
          goto exit;
        }

        if (!grn_select_apply_scorer(ctx, data)) {
          goto exit;
        }

        grn_output_range_normalize(ctx, nhits, &(data->offset), &(data->limit));

        if (!grn_select_sort(ctx, data)) {
          goto exit;
        }

        if (!grn_select_load(ctx, data)) {
          goto exit;
        }

        if (!grn_select_apply_output_dynamic_columns(ctx, data)) {
          goto exit;
        }

        if (!data->execute_olap_operations()) {
          goto exit;
        }

        succeeded = grn_select_output(ctx, data);
        if (!succeeded) {
          goto exit;
        }
      }
      if (!ctx->rc && data->cacheable &&
          GRN_TEXT_LEN(*cache_key) <= GRN_CACHE_MAX_KEY_SIZE &&
          (!data->cache.value || data->cache.length != 2 ||
           data->cache.value[0] != 'n' || data->cache.value[1] != 'o')) {
        grn_cache_update(ctx,
                         cache_obj,
                         GRN_TEXT_VALUE(*cache_key),
                         GRN_TEXT_LEN(*cache_key),
                         outbuf);
      }
      if (data->taintable > 0) {
        grn_db_touch(ctx, DB_OBJ(data->tables.target)->db);
      }
    }

  exit:
    grn_ctx_set_match_escalation_threshold(ctx,
                                           original_match_escalation_threshold);
    grn_ctx_set_force_match_escalation(ctx, original_force_match_escalation);

    /* GRN_LOG(ctx, GRN_LOG_NONE, "%d", ctx->seqno); */

    return ctx->rc;
  }
} // namespace

static grn_obj *
command_select(grn_ctx *ctx,
               int nargs,
               grn_obj **args,
               grn_user_data *user_data)
{
  grn::CommandArguments command_args(ctx, user_data);
  SelectExecutor executor(ctx, &command_args);
  if (ctx->rc == GRN_SUCCESS) {
    executor.execute();
  }
  return NULL;
}

#define N_VARS      35
#define DEFINE_VARS grn_expr_var vars[N_VARS]

static void
init_vars(grn_ctx *ctx, grn_expr_var *vars)
{
  grn_plugin_expr_var_init(ctx, &(vars[0]), "name", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "match_columns", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "query", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "filter", -1);
  grn_plugin_expr_var_init(ctx, &(vars[5]), "scorer", -1);
  /* Deprecated since 6.0.3. Use sort_keys instead. */
  grn_plugin_expr_var_init(ctx, &(vars[6]), "sortby", -1);
  grn_plugin_expr_var_init(ctx, &(vars[7]), "output_columns", -1);
  grn_plugin_expr_var_init(ctx, &(vars[8]), "offset", -1);
  grn_plugin_expr_var_init(ctx, &(vars[9]), "limit", -1);
  grn_plugin_expr_var_init(ctx, &(vars[10]), "drilldown", -1);
  /* Deprecated since 6.0.3. Use drilldown_sort_keys instead. */
  grn_plugin_expr_var_init(ctx, &(vars[11]), "drilldown_sortby", -1);
  grn_plugin_expr_var_init(ctx, &(vars[12]), "drilldown_output_columns", -1);
  grn_plugin_expr_var_init(ctx, &(vars[13]), "drilldown_offset", -1);
  grn_plugin_expr_var_init(ctx, &(vars[14]), "drilldown_limit", -1);
  grn_plugin_expr_var_init(ctx, &(vars[15]), "cache", -1);
  grn_plugin_expr_var_init(ctx, &(vars[16]), "match_escalation_threshold", -1);
  /* Deprecated. Use query_expander instead. */
  grn_plugin_expr_var_init(ctx, &(vars[17]), "query_expansion", -1);
  grn_plugin_expr_var_init(ctx, &(vars[18]), "query_flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[19]), "query_expander", -1);
  grn_plugin_expr_var_init(ctx, &(vars[20]), "adjuster", -1);
  grn_plugin_expr_var_init(ctx, &(vars[21]), "drilldown_calc_types", -1);
  grn_plugin_expr_var_init(ctx, &(vars[22]), "drilldown_calc_target", -1);
  grn_plugin_expr_var_init(ctx, &(vars[23]), "drilldown_filter", -1);
  grn_plugin_expr_var_init(ctx, &(vars[24]), "sort_keys", -1);
  grn_plugin_expr_var_init(ctx, &(vars[25]), "drilldown_sort_keys", -1);
  grn_plugin_expr_var_init(ctx, &(vars[26]), "drilldown_adjuster", -1);
  grn_plugin_expr_var_init(ctx, &(vars[27]), "match_escalation", -1);
  grn_plugin_expr_var_init(ctx, &(vars[28]), "load_table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[29]), "load_columns", -1);
  grn_plugin_expr_var_init(ctx, &(vars[30]), "load_values", -1);
  grn_plugin_expr_var_init(ctx, &(vars[31]), "post_filter", -1);
  grn_plugin_expr_var_init(ctx, &(vars[32]), "query_options", -1);
  grn_plugin_expr_var_init(ctx,
                           &(vars[33]),
                           "drilldown_max_n_target_records",
                           -1);
  grn_plugin_expr_var_init(ctx, &(vars[34]), "n_workers", -1);
}

extern "C" void
grn_proc_init_select(grn_ctx *ctx)
{
  DEFINE_VARS;

  init_vars(ctx, vars);
  grn_plugin_command_create(ctx,
                            "select",
                            -1,
                            command_select,
                            N_VARS - 1,
                            vars + 1);
}

static grn_obj *
command_define_selector(grn_ctx *ctx,
                        int nargs,
                        grn_obj **args,
                        grn_user_data *user_data)
{
  uint32_t i, nvars;
  grn_expr_var *vars;

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  for (i = 1; i < nvars; i++) {
    grn_obj *var;
    var = grn_plugin_proc_get_var_by_offset(ctx, user_data, i);
    GRN_TEXT_SET(ctx,
                 &((vars + i)->value),
                 GRN_TEXT_VALUE(var),
                 GRN_TEXT_LEN(var));
  }
  {
    grn_obj *name;
    name = grn_plugin_proc_get_var(ctx, user_data, "name", -1);
    grn_plugin_command_create(ctx,
                              GRN_TEXT_VALUE(name),
                              GRN_TEXT_LEN(name),
                              command_select,
                              nvars - 1,
                              vars + 1);
  }
  GRN_OUTPUT_BOOL(!ctx->rc);

  return NULL;
}

extern "C" void
grn_proc_init_define_selector(grn_ctx *ctx)
{
  DEFINE_VARS;

  init_vars(ctx, vars);
  grn_plugin_command_create(ctx,
                            "define_selector",
                            -1,
                            command_define_selector,
                            N_VARS,
                            vars);
}
