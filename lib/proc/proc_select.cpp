/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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
#include "../grn_ctx_impl.h"
#include "../grn_expr.h"
#include "../grn_group.h"
#include "../grn_ii.h"
#include "../grn_output.h"
#include "../grn_posting.h"
#include "../grn_proc.h"
#include "../grn_str.h"
#include "../grn_util.h"
#include "../grn_window_function_executor.h"

#include <groonga/plugin.h>
#include <groonga.hpp>

#define GRN_SELECT_INTERNAL_VAR_MATCH_COLUMNS "$match_columns"

#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key, _nsubrecs"

namespace {
  template <typename Class>
  void
  close_objects(grn_ctx *ctx, grn_hash *objects)
  {
    GRN_HASH_EACH_BEGIN(ctx, objects, cursor, id) {
      void *value;
      grn_hash_cursor_get_value(ctx, cursor, &value);
      auto object = static_cast<Class *>(value);
      object->~Class();
    } GRN_HASH_EACH_END(ctx, cursor);
    grn_hash_close(ctx, objects);
  }

  enum class DynamicColumnStage {
    INITIAL,
    RESULT_SET,
    FILTERED,
    OUTPUT,
    GROUP,
  };

  struct DynamicColumn {
    DynamicColumn(grn_ctx *ctx,
                  const char *label,
                  size_t label_len,
                  DynamicColumnStage stage)
      : ctx_(ctx),
        label({label, label_len}),
        stage(stage),
        type(grn_ctx_at(ctx, GRN_DB_TEXT)),
        flags(GRN_OBJ_COLUMN_SCALAR),
        value({nullptr, 0}),
        window({{nullptr, 0}, {nullptr, 0}}),
        dependency_names() {
      GRN_TEXT_INIT(&dependency_names, GRN_OBJ_VECTOR);
    }

    ~DynamicColumn() {
      grn_obj_unref(ctx_, type);
      auto ctx = ctx_;
      GRN_OBJ_FIN(ctx, &dependency_names);
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
  };

  struct DynamicColumns {
  public:
    DynamicColumns(grn_ctx *ctx)
        : ctx_(ctx),
          initial(nullptr),
          result_set(nullptr),
          filtered(nullptr),
          output(nullptr),
          group(nullptr) {
    }

    ~DynamicColumns() {
      close_stage(initial);
      close_stage(result_set);
      close_stage(filtered);
      close_stage(output);
      close_stage(group);
    }

  private:
    void close_stage(grn_hash *stage) {
      if (!stage) {
        return;
      }
      close_objects<DynamicColumn>(ctx_, stage);
    }

  public:
    grn_ctx *ctx_;

    grn_hash *initial;
    grn_hash *result_set;
    grn_hash *filtered;
    grn_hash *output;
    grn_hash *group;
  };


  struct Drilldown {
    Drilldown(grn_ctx *ctx, const char *label, size_t label_len)
      : ctx_(ctx),
        label({label, label_len}),
        keys({nullptr, 0}),
        parsed_keys(nullptr),
        n_parsed_keys(0),
        sort_keys({nullptr, 0}),
        output_columns({nullptr, 0}),
        offset(0),
        limit(DEFAULT_DRILLDOWN_LIMIT),
        calc_types(0),
        calc_target_name({nullptr, 0}),
        filter({nullptr, 0}),
        adjuster({nullptr, 0}),
        table_name({nullptr, 0}),
        max_n_target_records(-1),
        dynamic_columns(ctx),
        result(),
        filtered_result(nullptr) {
    }

    ~Drilldown() {
      if (filtered_result) {
        grn_obj_close(ctx_, filtered_result);
      }

      if (result.table) {
        if (result.calc_target) {
          grn_obj_unlink(ctx_, result.calc_target);
        }
        if (result.table) {
          grn_obj_unlink(ctx_, result.table);
        }
        if (result.n_aggregators > 0) {
          uint32_t i;
          for (i = 0; i < result.n_aggregators; i++) {
            auto *aggregator = result.aggregators[i];
            grn_table_group_aggregator_close(ctx_, aggregator);
          }
          auto ctx = ctx_;
          GRN_FREE(result.aggregators);
        }
      }
    }

    grn_ctx *ctx_;

    grn_raw_string label;
    grn_raw_string keys;
    grn_table_sort_key *parsed_keys;
    int n_parsed_keys;
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
    DynamicColumns dynamic_columns;
    grn_table_group_result result;
    grn_obj *filtered_result;
  };

  struct Filter {
    Filter(grn_ctx *ctx)
      : ctx_(ctx),
        match_columns({nullptr, 0}),
        query({nullptr, 0}),
        query_expander({nullptr, 0}),
        query_flags({nullptr, 0}),
        query_options({nullptr, 0}),
        filter({nullptr, 0}),
        post_filter({nullptr, 0}),
        condition({nullptr, nullptr, nullptr}),
        post_condition({nullptr}),
        filtered(nullptr),
        post_filtered(nullptr) {
    }

    ~Filter() {
      if (post_filtered) {
        grn_obj_unlink(ctx_, post_filtered);
      }
      if (post_condition.expression) {
        grn_obj_close(ctx_, post_condition.expression);
      }
      if (filtered) {
        grn_obj_unlink(ctx_, filtered);
      }
      if (condition.expression) {
        grn_obj_close(ctx_, condition.expression);
      }
      if (condition.match_columns) {
        grn_obj_close(ctx_, condition.match_columns);
      }
      if (condition.query_options_expression) {
        grn_obj_close(ctx_, condition.query_options_expression);
      }
    }

    grn_ctx *ctx_;

    grn_raw_string match_columns;
    grn_raw_string query;
    grn_raw_string query_expander;
    grn_raw_string query_flags;
    grn_raw_string query_options;
    grn_raw_string filter;
    grn_raw_string post_filter;
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
    Slice(grn_ctx *ctx, const char *label, size_t label_len)
      : ctx_(ctx),
        label({label, label_len}),
        filter(ctx),
        sort_keys({nullptr, 0}),
        output_columns({nullptr, 0}),
        offset(0),
        limit(GRN_SELECT_DEFAULT_LIMIT),
        tables({nullptr, nullptr, nullptr, nullptr, nullptr}),
        drilldowns(nullptr),
        dynamic_columns(ctx) {
    }

    ~Slice() {
      if (drilldowns) {
        close_objects<Drilldown>(ctx_, drilldowns);
      }
      if (tables.sorted) {
        grn_obj_unlink(ctx_, tables.sorted);
      }
      if (tables.initial) {
        grn_obj_unlink(ctx_, tables.initial);
      }
    }

    grn_ctx *ctx_;

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
    grn_hash *drilldowns;
    DynamicColumns dynamic_columns;
  };
}

typedef struct _grn_select_output_formatter grn_select_output_formatter;

namespace {
  struct Tables {
    Tables(grn_ctx *ctx)
      : ctx_(ctx),
        target(nullptr),
        initial(nullptr),
        result(nullptr),
        sorted(nullptr),
        output(nullptr) {
    }

    ~Tables() {
      if (sorted) {
        grn_obj_unlink(ctx_, sorted);
      }

      if (result &&
          result != initial &&
          result != target) {
        grn_obj_unlink(ctx_, result);
      }

      if (initial && initial != target) {
        grn_obj_unlink(ctx_, initial);
      }

      if (target) {
        grn_obj_unlink(ctx_, target);
      }
    }

    grn_ctx *ctx_;

    grn_obj *target;
    grn_obj *initial;
    grn_obj *result;
    grn_obj *sorted;
    grn_obj *output;
  };

  struct SelectExecutor {
    SelectExecutor(grn_ctx *ctx,
                   int n_args,
                   grn_obj **args,
                   grn_user_data *user_data)
      : ctx_(ctx),
        n_args_(n_args),
        args_(args),
        user_data_(user_data),

        tables(ctx),
        cacheable(0),
        taintable(0),
        output({0, nullptr}),
        load({{nullptr, 0}, {nullptr, 0}, {nullptr, 0}}),

        table({nullptr, 0}),
        filter(ctx),
        scorer({nullptr, 0}),
        sort_keys({nullptr, 0}),
        output_columns({nullptr, 0}),
        default_output_columns({nullptr, 0}),
        offset(0),
        limit(0),
        slices(nullptr),
        drilldown(ctx, nullptr, 0),
        drilldowns(nullptr),
        cache({nullptr, 0}),
        match_escalation_threshold({nullptr, 0}),
        adjuster({nullptr, 0}),
        match_escalation({nullptr, 0}),
        dynamic_columns(ctx) {
    }

    ~SelectExecutor() {
      if (drilldowns) {
        close_objects<Drilldown>(ctx_, drilldowns);
      }

      if (slices) {
        close_objects<Slice>(ctx_, slices);
      }

      if (tables.result == filter.post_filtered) {
        tables.result = nullptr;
      }
      if (tables.result == filter.filtered) {
        tables.result = nullptr;
      }
    }

    grn_ctx *ctx_;
    int n_args_;
    grn_obj **args_;
    grn_user_data *user_data_;

    /* for processing */
    Tables tables;
    uint16_t cacheable;
    uint16_t taintable;
    struct {
      int n_elements;
      grn_select_output_formatter *formatter;
    } output;
    struct {
      grn_raw_string table;
      grn_raw_string columns;
      grn_raw_string values;
    } load;

    /* inputs */
    grn_raw_string table;
    Filter filter;
    grn_raw_string scorer;
    grn_raw_string sort_keys;
    grn_raw_string output_columns;
    grn_raw_string default_output_columns;
    int offset;
    int limit;
    grn_hash *slices;
    Drilldown drilldown;
    grn_hash *drilldowns;
    grn_raw_string cache;
    grn_raw_string match_escalation_threshold;
    grn_raw_string adjuster;
    grn_raw_string match_escalation;
    DynamicColumns dynamic_columns;
  };
}

using grn_select_data = SelectExecutor;


typedef void grn_select_output_slices_label_func(grn_ctx *ctx,
                                                 grn_select_data *data);
typedef void grn_select_output_slices_open_func(grn_ctx *ctx,
                                                grn_select_data *data,
                                                unsigned int n_result_sets);
typedef void grn_select_output_slices_close_func(grn_ctx *ctx,
                                                 grn_select_data *data);
typedef void grn_select_output_slice_label_func(grn_ctx *ctx,
                                                grn_select_data *data,
                                                Slice *slice);
typedef void grn_select_output_drilldowns_label_func(grn_ctx *ctx,
                                                     grn_select_data *data);
typedef void grn_select_output_drilldowns_open_func(grn_ctx *ctx,
                                                    grn_select_data *data,
                                                    unsigned int n_result_sets);
typedef void grn_select_output_drilldowns_close_func(grn_ctx *ctx,
                                                     grn_select_data *data);
typedef void grn_select_output_drilldown_label_func(grn_ctx *ctx,
                                                    grn_select_data *data,
                                                    Drilldown *drilldown);

struct _grn_select_output_formatter {
  grn_select_output_slices_label_func      *slices_label;
  grn_select_output_slices_open_func       *slices_open;
  grn_select_output_slices_close_func      *slices_close;
  grn_select_output_slice_label_func       *slice_label;
  grn_select_output_drilldowns_label_func  *drilldowns_label;
  grn_select_output_drilldowns_open_func   *drilldowns_open;
  grn_select_output_drilldowns_close_func  *drilldowns_close;
  grn_select_output_drilldown_label_func   *drilldown_label;
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

  query_expander = grn_ctx_get(ctx,
                               query_expander_name,
                               query_expander_name_len);
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
    return grn_expr_syntax_expand_query(ctx, query, query_len, flags,
                                        query_expander, expanded_query);
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
                                          query, query_len,
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

static grn_table_group_flags
grn_parse_table_group_calc_types(grn_ctx *ctx,
                                 const char *calc_types,
                                 unsigned int calc_types_len)
{
  grn_table_group_flags flags = 0;
  const char *calc_types_end = calc_types + calc_types_len;

  while (calc_types < calc_types_end) {
    if (*calc_types == ',' || *calc_types == ' ') {
      calc_types += 1;
      continue;
    }

#define CHECK_TABLE_GROUP_CALC_TYPE(name)\
  if (((size_t)(calc_types_end - calc_types) >= (sizeof(#name) - 1)) &&\
      (!memcmp(calc_types, #name, sizeof(#name) - 1))) {\
    flags |= GRN_TABLE_GROUP_CALC_ ## name;\
    calc_types += sizeof(#name) - 1;\
    continue;\
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

    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "invalid table group calc type: <%.*s>",
                     (int)(calc_types_end - calc_types),
                     calc_types);
    return 0;
#undef CHECK_TABLE_GROUP_CALC_TYPE
  }

  return flags;
}

static const char *
grn_dynamic_column_stage_name(DynamicColumnStage stage)
{
  switch (stage) {
  case DynamicColumnStage::INITIAL :
    return "initial";
  case DynamicColumnStage::RESULT_SET :
    return "result_set";
  case DynamicColumnStage::FILTERED :
    return "filtered";
  case DynamicColumnStage::OUTPUT :
    return "output";
  case DynamicColumnStage::GROUP :
    return "group";
  default :
    return "unknown";
  }
}

static bool
grn_dynamic_columns_add(grn_ctx *ctx,
                        const char *label,
                        size_t label_len,
                        DynamicColumnStage stage,
                        grn_hash **dynamic_columns)
{
  if (!*dynamic_columns) {
    *dynamic_columns = grn_hash_create(ctx,
                                       NULL,
                                       GRN_TABLE_MAX_KEY_SIZE,
                                       sizeof(DynamicColumn),
                                       GRN_OBJ_TABLE_HASH_KEY |
                                       GRN_OBJ_KEY_VAR_SIZE |
                                       GRN_HASH_TINY);
  }
  if (!*dynamic_columns) {
    return false;
  }

  void *dynamic_column;
  int added;
  grn_hash_add(ctx,
               *dynamic_columns,
               label,
               static_cast<unsigned int>(label_len),
               &dynamic_column,
               &added);
  if (!added) {
    return GRN_TRUE;
  }

  new(dynamic_column) DynamicColumn(ctx, label, label_len, stage);

  return true;
}

static bool
grn_dynamic_column_extract_dependency_column_names(grn_ctx *ctx,
                                                   DynamicColumn *dynamic_column,
                                                   grn_raw_string *keys)
{
  if (keys->length == 0) {
    return true;
  }

  // TODO: Improve this logic.
  size_t start = 0;
  size_t current;
  for (current = 0; current < keys->length; current++) {
    char c = keys->value[current];
    if (('0' <= c && c <= '9') ||
        ('A' <= c && c <= 'Z') ||
        ('a' <= c && c <= 'z') ||
        (c == '_') ||
        (c == '.')) {
      continue;
    }
    if (start == current) {
      start++;
    } else {
      grn_vector_add_element(ctx,
                             &(dynamic_column->dependency_names),
                             keys->value + start,
                             current - start,
                             0,
                             GRN_DB_SHORT_TEXT);
      start = current + 1;
    }
  }

  if (start != current) {
    grn_vector_add_element(ctx,
                           &(dynamic_column->dependency_names),
                           keys->value + start,
                           current - start,
                           0,
                           GRN_DB_SHORT_TEXT);
  }

  return true;
}

static grn_bool
grn_dynamic_column_fill(grn_ctx *ctx,
                        DynamicColumn *dynamic_column,
                        grn_obj *type_raw,
                        grn_obj *flags,
                        grn_obj *value,
                        grn_obj *window_sort_keys,
                        grn_obj *window_group_keys)
{
  if (type_raw && GRN_TEXT_LEN(type_raw) > 0) {
    grn_obj *type;

    type = grn_ctx_get(ctx, GRN_TEXT_VALUE(type_raw), GRN_TEXT_LEN(type_raw));
    if (!type) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][columns][%s][%.*s] unknown type: <%.*s>",
                       grn_dynamic_column_stage_name(dynamic_column->stage),
                       (int)(dynamic_column->label.length),
                       dynamic_column->label.value,
                       (int)(GRN_TEXT_LEN(type_raw)),
                       GRN_TEXT_VALUE(type_raw));
      return GRN_FALSE;
    }
    if (!(grn_obj_is_type(ctx, type) || grn_obj_is_table(ctx, type))) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, type);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][columns][%s][%.*s] invalid type: %.*s",
                       grn_dynamic_column_stage_name(dynamic_column->stage),
                       (int)(dynamic_column->label.length),
                       dynamic_column->label.value,
                       (int)(GRN_TEXT_LEN(&inspected)),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      grn_obj_unlink(ctx, type);
      return GRN_FALSE;
    }
    if (dynamic_column->type) {
      grn_obj_unref(ctx, dynamic_column->type);
    }
    dynamic_column->type = type;
  }

  if (flags && GRN_TEXT_LEN(flags) > 0) {
    char error_message_tag[GRN_TABLE_MAX_KEY_SIZE];

    grn_snprintf(error_message_tag,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[select][columns][%s][%.*s]",
                 grn_dynamic_column_stage_name(dynamic_column->stage),
                 (int)(dynamic_column->label.length),
                 dynamic_column->label.value);
    dynamic_column->flags =
      grn_proc_column_parse_flags(ctx,
                                  error_message_tag,
                                  GRN_TEXT_VALUE(flags),
                                  GRN_TEXT_VALUE(flags) + GRN_TEXT_LEN(flags));
    if (ctx->rc != GRN_SUCCESS) {
      return GRN_FALSE;
    }
  }

  GRN_RAW_STRING_FILL(dynamic_column->value, value);
  GRN_RAW_STRING_FILL(dynamic_column->window.sort_keys, window_sort_keys);
  GRN_RAW_STRING_FILL(dynamic_column->window.group_keys, window_group_keys);

  GRN_BULK_REWIND(&(dynamic_column->dependency_names));
  if (!grn_dynamic_column_extract_dependency_column_names(
        ctx, dynamic_column, &(dynamic_column->window.sort_keys))) {
    return false;
  }
  if (!grn_dynamic_column_extract_dependency_column_names(
        ctx, dynamic_column, &(dynamic_column->window.group_keys))) {
    return false;
  }

  return GRN_TRUE;
}

static grn_bool
grn_dynamic_column_collect(grn_ctx *ctx,
                           grn_user_data *user_data,
                           grn_hash *columns,
                           const char *prefix_label,
                           size_t prefix_label_len)
{
  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {
    char key_name[GRN_TABLE_MAX_KEY_SIZE];
    grn_obj *type = NULL;
    grn_obj *flags = NULL;
    grn_obj *value = NULL;
    struct {
      grn_obj *sort_keys;
      grn_obj *group_keys;
    } window;

    window.sort_keys = NULL;
    window.group_keys = NULL;

    DynamicColumn *dynamic_column;
    grn_hash_cursor_get_value(ctx,
                              cursor,
                              reinterpret_cast<void **>(&dynamic_column));

#define GET_VAR_RAW(parameter_key, name)                                \
    if (!name) {                                                        \
      grn_snprintf(key_name,                                            \
                   GRN_TABLE_MAX_KEY_SIZE,                              \
                   GRN_TABLE_MAX_KEY_SIZE,                              \
                   "%.*s%s[%.*s]." # name,                              \
                   (int)prefix_label_len,                               \
                   prefix_label,                                        \
                   parameter_key,                                       \
                   (int)(dynamic_column->label.length),                 \
                   dynamic_column->label.value);                        \
      name = grn_plugin_proc_get_var(ctx, user_data, key_name, -1);     \
    }

#define GET_VAR(name) do {                      \
      GET_VAR_RAW("columns", name);             \
      /* For backward compatibility */          \
      GET_VAR_RAW("column", name);              \
    } while (GRN_FALSE)

    GET_VAR(type);
    GET_VAR(flags);
    GET_VAR(value);
    GET_VAR(window.sort_keys);
    GET_VAR(window.group_keys);

#undef GET_VAR

#undef GET_VAR_RAW

    grn_dynamic_column_fill(ctx, dynamic_column,
                            type, flags, value,
                            window.sort_keys,
                            window.group_keys);
  } GRN_HASH_EACH_END(ctx, cursor);

  return true;
}

static bool
grn_dynamic_columns_collect(grn_ctx *ctx,
                            grn_user_data *user_data,
                            DynamicColumns *dynamic_columns,
                            const char *prefix,
                            const char *base_prefix,
                            size_t base_prefix_len)
{
  grn_obj *vars;
  grn_table_cursor *cursor;
  size_t prefix_len;
  const char *suffix = "].stage";
  size_t suffix_len;

  vars = grn_plugin_proc_get_vars(ctx, user_data);
  cursor = grn_table_cursor_open(ctx, vars, NULL, 0, NULL, 0, 0, -1, 0);
  if (!cursor) {
    return false;
  }

  prefix_len = strlen(prefix);
  suffix_len = strlen(suffix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    auto variable_name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    auto variable_name = static_cast<char *>(key);
    if (static_cast<size_t>(variable_name_len) <
        base_prefix_len + prefix_len + suffix_len + 1) {
      continue;
    }

    if (base_prefix_len > 0) {
      if (memcmp(base_prefix, variable_name, base_prefix_len) != 0) {
        continue;
      }
    }

    if (memcmp(prefix, variable_name + base_prefix_len, prefix_len) != 0) {
      continue;
    }

    if (memcmp(suffix,
               variable_name +
               (static_cast<size_t>(variable_name_len) - suffix_len),
               suffix_len) != 0) {
      continue;
    }

    void *value_raw;
    grn_table_cursor_get_value(ctx, cursor, &value_raw);
    auto value = static_cast<grn_obj *>(value_raw);
    DynamicColumnStage stage;
    grn_hash **target_columns;
    if (GRN_TEXT_EQUAL_CSTRING(value, "initial")) {
      stage = DynamicColumnStage::INITIAL;
      target_columns = &(dynamic_columns->initial);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "result_set")) {
      stage = DynamicColumnStage::RESULT_SET;
      target_columns = &(dynamic_columns->result_set);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "filtered")) {
      stage = DynamicColumnStage::FILTERED;
      target_columns = &(dynamic_columns->filtered);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "output")) {
      stage = DynamicColumnStage::OUTPUT;
      target_columns = &(dynamic_columns->output);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "group")) {
      stage = DynamicColumnStage::GROUP;
      target_columns = &(dynamic_columns->group);
    } else {
      continue;
    }

    auto column_name = variable_name + base_prefix_len + prefix_len;
    auto column_name_len =
      static_cast<size_t>(variable_name_len) -
      base_prefix_len -
      prefix_len -
      suffix_len;
    if (!grn_dynamic_columns_add(ctx,
                                 column_name,
                                 column_name_len,
                                 stage,
                                 target_columns)) {
      grn_table_cursor_close(ctx, cursor);
      return false;
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return true;
}

static bool
grn_dynamic_columns_fill(grn_ctx *ctx,
                         grn_user_data *user_data,
                         DynamicColumns *dynamic_columns,
                         const char *prefix,
                         size_t prefix_length)
{
  if (!grn_dynamic_columns_collect(ctx, user_data, dynamic_columns,
                                   "columns[", prefix, prefix_length)) {
    return false;
  }

  /* For backward compatibility */
  if (!grn_dynamic_columns_collect(ctx, user_data, dynamic_columns,
                                   "column[", prefix, prefix_length)) {
    return false;
  }

  if (dynamic_columns->initial) {
    if (!grn_dynamic_column_collect(ctx,
                                    user_data,
                                    dynamic_columns->initial,
                                    prefix,
                                    prefix_length)) {
      return false;
    }
  }

  if (dynamic_columns->result_set) {
    if (!grn_dynamic_column_collect(ctx,
                                    user_data,
                                    dynamic_columns->result_set,
                                    prefix,
                                    prefix_length)) {
      return false;
    }
  }

  if (dynamic_columns->filtered) {
    if (!grn_dynamic_column_collect(ctx,
                                    user_data,
                                    dynamic_columns->filtered,
                                    prefix,
                                    prefix_length)) {
      return false;
    }
  }

  if (dynamic_columns->output) {
    if (!grn_dynamic_column_collect(ctx,
                                    user_data,
                                    dynamic_columns->output,
                                    prefix,
                                    prefix_length)) {
      return false;
    }
  }

  if (dynamic_columns->group) {
    if (!grn_dynamic_column_collect(ctx,
                                    user_data,
                                    dynamic_columns->group,
                                    prefix,
                                    prefix_length)) {
      return false;
    }
  }

  return true;
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
grn_select_create_all_selected_result_table(grn_ctx *ctx,
                                            grn_obj *table)
{
  grn_obj *result;

  result = grn_table_create(ctx, NULL, 0, NULL,
                            GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                            table, NULL);
  if (!result) {
    return NULL;
  }

  grn_result_set_add_table(ctx,
                           (grn_hash *)result,
                           table,
                           0.0,
                           GRN_OP_OR);
  return result;
}

static grn_obj *
grn_select_create_no_sort_keys_sorted_table(grn_ctx *ctx,
                                            grn_select_data *data,
                                            grn_obj *table)
{
  grn_obj *sorted;
  grn_table_cursor *cursor;

  sorted = grn_table_create(ctx, NULL, 0, NULL,
                            GRN_OBJ_TABLE_NO_KEY,
                            NULL,
                            table);

  if (!sorted) {
    return NULL;
  }

  cursor = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0,
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
  case TSORT_STATUS_VISITING :
    cycled = true;
    break;
  case TSORT_STATUS_VISITED :
    cycled = false;
    break;
  case TSORT_STATUS_NOT_VISITED :
    cycled = false;
    statuses[index] = TSORT_STATUS_VISITING;
    {
      auto *dynamic_column =
        const_cast<DynamicColumn *>(
          reinterpret_cast<const DynamicColumn *>(
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
        dependent_id = grn_hash_get(ctx,
                                    dynamic_columns,
                                    name,
                                    name_length,
                                    NULL);
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

  GRN_HASH_EACH_BEGIN(ctx, dynamic_columns, cursor, id) {
    if (dynamic_columns_tsort_visit(ctx,
                                    dynamic_columns,
                                    statuses,
                                    ids,
                                    id,
                                    log_tag_prefix)) {
      succeeded = true;
      break;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

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
    if (ctx->rc == GRN_SUCCESS &&
        dynamic_column->window.sort_keys.length > 0) {
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

exit :
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
      const_cast<DynamicColumn *>(
        reinterpret_cast<const DynamicColumn *>(
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

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "%scolumns[%.*s](%d)",
                  query_log_tag_prefix ? query_log_tag_prefix : "",
                  (int)(dynamic_column->label.length),
                  dynamic_column->label.value,
                  grn_table_size(ctx, table));
  }

  GRN_OBJ_FIN(ctx, &tsorted_ids);
}

static void
grn_filter_fill(grn_ctx *ctx,
                Filter *filter,
                grn_obj *match_columns,
                grn_obj *query,
                grn_obj *query_expander,
                grn_obj *query_flags,
                grn_obj *query_options,
                grn_obj *filter_raw,
                grn_obj *post_filter)
{
  GRN_RAW_STRING_FILL(filter->match_columns, match_columns);
  GRN_RAW_STRING_FILL(filter->query, query);
  GRN_RAW_STRING_FILL(filter->query_expander, query_expander);
  GRN_RAW_STRING_FILL(filter->query_flags, query_flags);
  GRN_RAW_STRING_FILL(filter->query_options, query_options);
  GRN_RAW_STRING_FILL(filter->filter, filter_raw);
  GRN_RAW_STRING_FILL(filter->post_filter, post_filter);
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

  GRN_EXPR_CREATE_FOR_QUERY(ctx,
                            table,
                            filter->condition.expression,
                            variable);
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
                     NULL, GRN_OP_MATCH, GRN_OP_AND,
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
        flags |= grn_proc_expr_query_flags_parse(ctx,
                                                 &query_flags,
                                                 log_tag_prefix);
        GRN_OBJ_FIN(ctx, &query_flags);
        if (ctx->rc != GRN_SUCCESS) {
          return false;
        }
      } else {
        flags |= GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN;
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
                                          NULL, 0,
                                          NULL, 0,
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
                     NULL, 0,
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

  grn_table_select(ctx,
                   table,
                   filter->condition.expression,
                   filter->filtered,
                   GRN_OP_OR);

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
                     NULL, 0,
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

static void
grn_drilldown_data_fill(grn_ctx *ctx,
                        Drilldown *drilldown,
                        grn_obj *keys,
                        grn_obj *sort_keys,
                        grn_obj *output_columns,
                        grn_obj *offset,
                        grn_obj *limit,
                        grn_obj *calc_types,
                        grn_obj *calc_target,
                        grn_obj *filter,
                        grn_obj *adjuster,
                        grn_obj *table,
                        grn_obj *max_n_target_records)
{
  GRN_RAW_STRING_FILL(drilldown->keys, keys);

  GRN_RAW_STRING_FILL(drilldown->sort_keys, sort_keys);

  GRN_RAW_STRING_FILL(drilldown->output_columns, output_columns);
  if (drilldown->output_columns.length == 0) {
    drilldown->output_columns.value = DEFAULT_DRILLDOWN_OUTPUT_COLUMNS;
    drilldown->output_columns.length = strlen(DEFAULT_DRILLDOWN_OUTPUT_COLUMNS);
  }

  if (offset && GRN_TEXT_LEN(offset)) {
    drilldown->offset =
      grn_atoi(GRN_TEXT_VALUE(offset), GRN_BULK_CURR(offset), NULL);
  } else {
    drilldown->offset = 0;
  }

  if (limit && GRN_TEXT_LEN(limit)) {
    drilldown->limit =
      grn_atoi(GRN_TEXT_VALUE(limit), GRN_BULK_CURR(limit), NULL);
  } else {
    drilldown->limit = DEFAULT_DRILLDOWN_LIMIT;
  }

  if (calc_types && GRN_TEXT_LEN(calc_types)) {
    drilldown->calc_types =
      grn_parse_table_group_calc_types(ctx,
                                       GRN_TEXT_VALUE(calc_types),
                                       GRN_TEXT_LEN(calc_types));
  } else {
    drilldown->calc_types = 0;
  }

  GRN_RAW_STRING_FILL(drilldown->calc_target_name, calc_target);

  GRN_RAW_STRING_FILL(drilldown->filter, filter);

  GRN_RAW_STRING_FILL(drilldown->adjuster, adjuster);

  GRN_RAW_STRING_FILL(drilldown->table_name, table);

  drilldown->max_n_target_records =
    grn_proc_option_value_int32(ctx,
                                max_n_target_records,
                                drilldown->max_n_target_records);
}

static void
grn_slice_fill(grn_ctx *ctx,
               Slice *slice,
               grn_obj *match_columns,
               grn_obj *query,
               grn_obj *query_expander,
               grn_obj *query_flags,
               grn_obj *query_options,
               grn_obj *filter,
               grn_obj *post_filter,
               grn_obj *sort_keys,
               grn_obj *output_columns,
               grn_obj *offset,
               grn_obj *limit)
{
  grn_filter_fill(ctx,
                  &(slice->filter),
                  match_columns,
                  query,
                  query_expander,
                  query_flags,
                  query_options,
                  filter,
                  post_filter);

  GRN_RAW_STRING_FILL(slice->sort_keys, sort_keys);

  GRN_RAW_STRING_FILL(slice->output_columns, output_columns);

  slice->offset = grn_proc_option_value_int32(ctx, offset, 0);
  slice->limit = grn_proc_option_value_int32(ctx,
                                             limit,
                                             GRN_SELECT_DEFAULT_LIMIT);
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

#define CHECK_EXPR_FLAG(name)                                           \
      if (((size_t)(end - current) >= (sizeof(#name) - 1)) &&           \
          (memcmp(current, #name, sizeof(#name) - 1) == 0) &&           \
          (((end - current) == (sizeof(#name) - 1)) ||                  \
           (current[sizeof(#name) - 1] == '|') ||                       \
           (current[sizeof(#name) - 1] == ' '))) {                      \
        flags |= GRN_EXPR_ ## name;                                     \
        current += sizeof(#name) - 1;                                   \
        continue;                                                       \
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

      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
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
      uint32_t flag_size = grn_vector_get_element_float(ctx,
                                                        query_flags,
                                                        i,
                                                        &flag,
                                                        NULL,
                                                        &domain);
      if (!grn_type_id_is_text_family(ctx, domain)) {
        grn_obj value;
        GRN_VALUE_FIX_SIZE_INIT(&value, 0, domain);
        GRN_TEXT_SET(ctx, &value, flag, flag_size);
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, &value);
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "%s query flag must be string: %.*s",
                         error_message_tag,
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        GRN_OBJ_FIN(ctx, &value);
        return 0;
      }

#define CHECK_EXPR_FLAG(name)                                           \
      if ((flag_size == (sizeof(#name) - 1)) &&                         \
          (memcmp(flag, #name, sizeof(#name) - 1) == 0)) {              \
        flags |= GRN_EXPR_ ## name;                                     \
        continue;                                                       \
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

      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
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
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
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
    GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
    GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
  rc = grn_obj_format_set_columns(ctx,
                                  format,
                                  result_set,
                                  columns,
                                  columns_len);
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
grn_select_apply_initial_columns(grn_ctx *ctx,
                                 grn_select_data *data)
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
grn_select_filter(grn_ctx *ctx,
                  grn_select_data *data)
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
grn_select_apply_filtered_dynamic_columns(grn_ctx *ctx,
                                          grn_select_data *data)
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
grn_select_post_filter(grn_ctx *ctx,
                       grn_select_data *data)
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
    column_name_size = grn_obj_name(ctx, column,
                                    column_name, GRN_TABLE_MAX_KEY_SIZE);
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

    grn_obj_search(ctx, index, value, table, GRN_OP_ADJUST, &options);
  }

  if (grn_enable_reference_count) {
    grn_obj_unlink(ctx, index);
  }
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

  rc = grn_expr_parse(ctx, adjuster,
                      adjuster_string->value,
                      adjuster_string->length,
                      NULL,
                      GRN_OP_MATCH, GRN_OP_ADJUST,
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

  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                ":", "%sadjust(%d)",
                query_log_tag_prefix ? query_log_tag_prefix : "",
                grn_table_size(ctx, result));

  return GRN_TRUE;
}

static grn_bool
grn_select_apply_scorer(grn_ctx *ctx,
                        grn_select_data *data)
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
                      GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
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
    GRN_TABLE_EACH_BEGIN(ctx, data->tables.result, cursor, id) {
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
    } GRN_TABLE_EACH_END(ctx, cursor);
    grn_expr_executor_fin(ctx, &executor);
  }
  grn_obj_unlink(ctx, scorer);

  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                ":", "score(%d): %.*s",
                grn_table_size(ctx, data->tables.result),
                (int)(data->scorer.length),
                data->scorer.value);

  return rc == GRN_SUCCESS;
}

static grn_bool
grn_select_sort(grn_ctx *ctx,
                grn_select_data *data)
{
  grn_table_sort_key *keys;
  int n_keys;

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

  data->tables.sorted = grn_table_create(ctx, NULL, 0, NULL,
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

  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                ":",
                "sort(%d): %.*s",
                data->limit,
                (int)(data->sort_keys.length),
                data->sort_keys.value);

  return ctx->rc == GRN_SUCCESS;
}

static bool
grn_select_load(grn_ctx *ctx,
                grn_select_data *data)
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

  table = grn_ctx_get(ctx,
                      data->load.table.value,
                      data->load.table.length);
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

  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                ":",
                "load(%d): [%.*s][%d]",
                grn_table_size(ctx, data->tables.result),
                (int)(data->load.table.length),
                data->load.table.value,
                grn_table_size(ctx, table));

exit :
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
grn_select_apply_output_dynamic_columns(grn_ctx *ctx,
                                        grn_select_data *data)
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
  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                ":", "output(%d)", data->limit);

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

  if (!grn_select_output_match_open(ctx, data, &format, n_additional_elements)) {
    return GRN_FALSE;
  }

  return grn_select_output_match_close(ctx, data, &format);
}

static bool
grn_select_drilldown_execute(grn_ctx *ctx,
                             grn_select_data *data,
                             grn_hash *drilldowns,
                             grn_obj *table,
                             grn_hash *slices,
                             grn_obj *condition,
                             grn_id id,
                             const bool is_labeled,
                             const char *log_tag_context,
                             const char *query_log_tag_prefix)
{
  bool success = true;
  grn_table_sort_key *keys = NULL;
  int n_keys = 0;
  grn_obj *target_table = table;
  grn_obj log_tag_prefix;
  grn_obj full_query_log_tag_prefix;

  auto drilldown =
    const_cast<Drilldown *>(
      reinterpret_cast<const Drilldown *>(
        grn_hash_get_value_(ctx, drilldowns, id, NULL)));

  GRN_TEXT_INIT(&log_tag_prefix, 0);
  grn_text_printf(ctx, &log_tag_prefix,
                  "[select]%s[drilldowns]%s%.*s%s",
                  log_tag_context,
                  drilldown->label.length > 0 ? "[" : "",
                  (int)(drilldown->label.length),
                  drilldown->label.value,
                  drilldown->label.length > 0 ? "]" : "");
  GRN_TEXT_PUTC(ctx, &log_tag_prefix, '\0');
  GRN_TEXT_INIT(&full_query_log_tag_prefix, 0);
  if (is_labeled) {
    grn_text_printf(ctx, &full_query_log_tag_prefix,
                    "%sdrilldowns[%.*s].",
                    query_log_tag_prefix,
                    (int)(drilldown->label.length),
                    drilldown->label.value);
  } else {
    grn_text_printf(ctx, &full_query_log_tag_prefix,
                    "%sdrilldown.",
                    query_log_tag_prefix);
  }
  GRN_TEXT_PUTC(ctx, &full_query_log_tag_prefix, '\0');

  auto result = &(drilldown->result);
  result->limit = drilldown->max_n_target_records;
  result->flags =
    GRN_TABLE_GROUP_CALC_COUNT |
    GRN_TABLE_GROUP_LIMIT;
  result->op = GRN_OP_NOP;
  result->max_n_subrecs = 0;
  result->key_begin = 0;
  result->key_end = 0;
  if (result->calc_target) {
    grn_obj_unlink(ctx, result->calc_target);
  }
  result->calc_target = NULL;

  if (drilldown->table_name.length > 0) {
    grn_id dependent_id;
    dependent_id = grn_hash_get(ctx,
                                drilldowns,
                                drilldown->table_name.value,
                                drilldown->table_name.length,
                                NULL);
    if (dependent_id == GRN_ID_NIL) {
      if (slices) {
        dependent_id = grn_hash_get(ctx,
                                    slices,
                                    drilldown->table_name.value,
                                    drilldown->table_name.length,
                                    NULL);
        if (dependent_id) {
          auto slice =
            reinterpret_cast<const Slice *>(
              grn_hash_get_value_(ctx, slices, dependent_id, NULL));
          target_table = slice->tables.result;
        }
      }
      if (dependent_id == GRN_ID_NIL) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "%s[table] "
                         "nonexistent label: <%.*s>",
                         GRN_TEXT_VALUE(&log_tag_prefix),
                         (int)(drilldown->table_name.length),
                         drilldown->table_name.value);
        success = false;
        goto exit;
      }
    } else {
      auto dependent_drilldown =
        reinterpret_cast<const Drilldown *>(
          grn_hash_get_value_(ctx, drilldowns, dependent_id, NULL));
      auto dependent_result = &(dependent_drilldown->result);
      target_table = dependent_result->table;
    }
  }

  if (drilldown->parsed_keys) {
    result->key_end = drilldown->n_parsed_keys;
  } else if (drilldown->keys.length > 0) {
    keys = grn_table_group_keys_parse(ctx,
                                      target_table,
                                      drilldown->keys.value,
                                      drilldown->keys.length,
                                      &n_keys);
    if (!keys) {
      GRN_PLUGIN_CLEAR_ERROR(ctx);
      success = false;
      goto exit;
    }

    result->key_end = n_keys - 1;
    if (n_keys > 1) {
      result->max_n_subrecs = 1;
    }
  }

  if (drilldown->calc_target_name.length > 0) {
    result->calc_target = grn_obj_column(ctx, target_table,
                                         drilldown->calc_target_name.value,
                                         drilldown->calc_target_name.length);
  }
  if (result->calc_target) {
    result->flags |= drilldown->calc_types;
  }

  if (drilldown->dynamic_columns.group) {
    result->n_aggregators = grn_hash_size(ctx, drilldown->dynamic_columns.group);
    if (result->n_aggregators > 0) {
      result->aggregators =
        GRN_MALLOCN(grn_table_group_aggregator *, result->n_aggregators);
      if (!result->aggregators) {
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "%s[filter] "
                         "failed to allocate aggregators: %s",
                         GRN_TEXT_VALUE(&log_tag_prefix),
                         ctx->errbuf);
        success = false;
        goto exit;
      }
      uint32_t i = 0;
      GRN_HASH_EACH_BEGIN(ctx, drilldown->dynamic_columns.group, cursor, id) {
        void *value;
        grn_hash_cursor_get_value(ctx, cursor, &value);
        auto dynamic_column = static_cast<DynamicColumn *>(value);
        result->aggregators[i] = grn_table_group_aggregator_open(ctx);
        if (!result->aggregators[i]) {
          GRN_PLUGIN_ERROR(ctx,
                           GRN_INVALID_ARGUMENT,
                           "%s[filter] "
                           "failed to open aggregator: %s",
                           GRN_TEXT_VALUE(&log_tag_prefix),
                           ctx->errbuf);
          success = false;
          break;
        }
        grn_table_group_aggregator_set_output_column_name(
          ctx,
          result->aggregators[i],
          dynamic_column->label.value,
          dynamic_column->label.length);
        grn_table_group_aggregator_set_output_column_type(
          ctx,
          result->aggregators[i],
          dynamic_column->type);
        grn_table_group_aggregator_set_output_column_flags(
          ctx,
          result->aggregators[i],
          dynamic_column->flags);
        grn_table_group_aggregator_set_expression(ctx,
                                                  result->aggregators[i],
                                                  dynamic_column->value.value,
                                                  dynamic_column->value.length);
        i++;
      } GRN_HASH_EACH_END(ctx, cursor);
      if (!success) {
        goto exit;
      }
      result->flags |= GRN_TABLE_GROUP_CALC_AGGREGATOR;
    }
  }

  grn_table_sort_key *group_keys;
  unsigned int n_group_keys;
  if (drilldown->parsed_keys) {
    group_keys = drilldown->parsed_keys;
    n_group_keys = drilldown->n_parsed_keys;
  } else {
    if (n_keys == 0 && !target_table) {
      /* For backward compatibility and consistency. Ignore
       * nonexistent table case with warning like we did for sort_keys
       * and drilldown[LABEL].keys. */
      GRN_LOG(ctx, GRN_WARN,
              "%s[table] doesn't exist: <%.*s>",
              GRN_TEXT_VALUE(&log_tag_prefix),
              (int)(drilldown->table_name.length),
              drilldown->table_name.value);
      goto exit;
    }
    group_keys = keys;
    n_group_keys = n_keys;
  }
  grn_table_group(ctx, target_table, group_keys, n_group_keys, result, 1);

  if (keys) {
    grn_table_sort_key_close(ctx, keys, n_keys);
  }

  if (!result->table) {
    success = false;
    goto exit;
  }

  if (grn_query_logger_pass(ctx, GRN_QUERY_LOG_SIZE)) {
    grn_obj keys_inspected;
    if (drilldown->parsed_keys) {
      GRN_TEXT_INIT(&keys_inspected, 0);
      int i;
      for (i = 0; i < drilldown->n_parsed_keys; i++) {
        if (i > 0) {
          GRN_TEXT_PUTS(ctx, &keys_inspected, ",");
        }
        grn_obj *key = drilldown->parsed_keys[i].key;
        if (grn_obj_is_table(ctx, key) || grn_obj_is_column(ctx, key)) {
          grn_inspect_name(ctx, &keys_inspected, key);
        } else {
          grn_inspect(ctx, &keys_inspected, key);
        }
      }
    } else {
      GRN_TEXT_INIT(&keys_inspected, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_SET(ctx,
                   &keys_inspected,
                   drilldown->keys.value,
                   drilldown->keys.length);
    }
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "%.*s(%u): %.*s",
                  (int)(GRN_TEXT_LEN(&full_query_log_tag_prefix) - 2),
                  GRN_TEXT_VALUE(&full_query_log_tag_prefix),
                  grn_table_size(ctx, result->table),
                  (int)(GRN_TEXT_LEN(&keys_inspected)),
                  GRN_TEXT_VALUE(&keys_inspected));
    GRN_OBJ_FIN(ctx, &keys_inspected);
  }

  if (drilldown->dynamic_columns.initial) {
    grn_select_apply_dynamic_columns(ctx,
                                     data,
                                     result->table,
                                     DynamicColumnStage::INITIAL,
                                     drilldown->dynamic_columns.initial,
                                     condition,
                                     GRN_TEXT_VALUE(&log_tag_prefix),
                                     GRN_TEXT_VALUE(&full_query_log_tag_prefix));
  }

  if (drilldown->filter.length > 0) {
    grn_obj *expression;
    grn_obj *record;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, result->table, expression, record);
    if (!expression) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s[filter] "
                       "failed to create expression for filter: %s",
                       GRN_TEXT_VALUE(&log_tag_prefix),
                       ctx->errbuf);
      success = false;
      goto exit;
    }
    grn_expr_parse(ctx,
                   expression,
                   drilldown->filter.value,
                   drilldown->filter.length,
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, expression);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s[filter] "
                       "failed to parse filter: <%.*s>: %s",
                       GRN_TEXT_VALUE(&log_tag_prefix),
                       (int)(drilldown->filter.length),
                       drilldown->filter.value,
                       ctx->errbuf);
      success = false;
      goto exit;
    }
    drilldown->filtered_result = grn_table_select(ctx,
                                                  result->table,
                                                  expression,
                                                  NULL,
                                                  GRN_OP_OR);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, expression);
      if (drilldown->filtered_result) {
        grn_obj_close(ctx, drilldown->filtered_result);
        drilldown->filtered_result = NULL;
      }
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s[filter] "
                       "failed to execute filter: <%.*s>: %s",
                       GRN_TEXT_VALUE(&log_tag_prefix),
                       (int)(drilldown->filter.length),
                       drilldown->filter.value,
                       ctx->errbuf);
      success = false;
      goto exit;
    }
    grn_obj_close(ctx, expression);

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "%sfilter(%u)",
                  GRN_TEXT_VALUE(&full_query_log_tag_prefix),
                  grn_table_size(ctx, drilldown->filtered_result));
  }

  if (drilldown->dynamic_columns.filtered) {
    grn_select_apply_dynamic_columns(ctx,
                                     data,
                                     drilldown->filtered_result,
                                     DynamicColumnStage::FILTERED,
                                     drilldown->dynamic_columns.filtered,
                                     condition,
                                     GRN_TEXT_VALUE(&log_tag_prefix),
                                     GRN_TEXT_VALUE(&full_query_log_tag_prefix));
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

  {
    grn_obj *adjuster_result_table;
    if (drilldown->filtered_result) {
      adjuster_result_table = drilldown->filtered_result;
    } else {
      adjuster_result_table = result->table;
    }
    grn_select_apply_adjuster(ctx,
                              data,
                              &(drilldown->adjuster),
                              result->table,
                              adjuster_result_table,
                              GRN_TEXT_VALUE(&log_tag_prefix),
                              GRN_TEXT_VALUE(&full_query_log_tag_prefix));
  }

exit :
  GRN_OBJ_FIN(ctx, &log_tag_prefix);
  GRN_OBJ_FIN(ctx, &full_query_log_tag_prefix);

  return success;
}
static grn_bool
drilldown_tsort_visit(grn_ctx *ctx,
                      grn_hash *drilldowns,
                      tsort_status *statuses,
                      grn_obj *ids,
                      grn_id id)
{
  grn_bool cycled = GRN_TRUE;
  uint32_t index = id - 1;

  switch (statuses[index]) {
  case TSORT_STATUS_VISITING :
    cycled = GRN_TRUE;
    break;
  case TSORT_STATUS_VISITED :
    cycled = GRN_FALSE;
    break;
  case TSORT_STATUS_NOT_VISITED :
    cycled = GRN_FALSE;
    statuses[index] = TSORT_STATUS_VISITING;
    {
      auto drilldown =
        reinterpret_cast<const Drilldown *>(
          grn_hash_get_value_(ctx, drilldowns, id, NULL));
      if (drilldown->table_name.length > 0) {
        grn_id dependent_id;
        dependent_id = grn_hash_get(ctx, drilldowns,
                                    drilldown->table_name.value,
                                    drilldown->table_name.length,
                                    NULL);
        if (dependent_id != GRN_ID_NIL) {
          cycled = drilldown_tsort_visit(ctx,
                                         drilldowns,
                                         statuses,
                                         ids,
                                         dependent_id);
          if (cycled) {
            GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                             "[select][drilldowns][%.*s][table] "
                             "cycled dependency: <%.*s>",
                             (int)(drilldown->label.length),
                             drilldown->label.value,
                             (int)(drilldown->table_name.length),
                             drilldown->table_name.value);
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

static grn_bool
drilldown_tsort_body(grn_ctx *ctx,
                     grn_hash *drilldowns,
                     tsort_status *statuses,
                     grn_obj *ids)
{
  grn_bool succeeded = GRN_TRUE;

  GRN_HASH_EACH_BEGIN(ctx, drilldowns, cursor, id) {
    if (drilldown_tsort_visit(ctx, drilldowns, statuses, ids, id)) {
      succeeded = GRN_FALSE;
      break;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  return succeeded;
}

static void
drilldown_tsort_init(grn_ctx *ctx,
                     tsort_status *statuses,
                     size_t n_statuses)
{
  size_t i;
  for (i = 0; i < n_statuses; i++) {
    statuses[i] = TSORT_STATUS_NOT_VISITED;
  }
}

static grn_bool
drilldown_tsort(grn_ctx *ctx,
                grn_hash *drilldowns,
                grn_obj *ids)
{
  tsort_status *statuses;
  size_t n_statuses;
  grn_bool succeeded;

  n_statuses = grn_hash_size(ctx, drilldowns);
  statuses = GRN_PLUGIN_MALLOCN(ctx, tsort_status, n_statuses);
  if (!statuses) {
    return GRN_FALSE;
  }

  drilldown_tsort_init(ctx, statuses, n_statuses);
  succeeded = drilldown_tsort_body(ctx, drilldowns, statuses, ids);
  GRN_PLUGIN_FREE(ctx, statuses);
  return succeeded;
}

static bool
grn_select_drilldowns_execute(grn_ctx *ctx,
                              grn_select_data *data,
                              grn_hash *drilldowns,
                              grn_obj *table,
                              grn_hash *slices,
                              grn_obj *condition,
                              const bool is_labeled,
                              const char *log_tag_context,
                              const char *query_log_tag_prefix)
{
  grn_bool succeeded = GRN_TRUE;
  grn_obj tsorted_ids;
  size_t i;
  size_t n_drilldowns;

  GRN_RECORD_INIT(&tsorted_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  if (!drilldown_tsort(ctx, drilldowns, &tsorted_ids)) {
    succeeded = GRN_FALSE;
    goto exit;
  }

  n_drilldowns = GRN_BULK_VSIZE(&tsorted_ids) / sizeof(grn_id);
  for (i = 0; i < n_drilldowns; i++) {
    grn_id id = GRN_RECORD_VALUE_AT(&tsorted_ids, i);
    if (!grn_select_drilldown_execute(ctx,
                                      data,
                                      drilldowns,
                                      table,
                                      slices,
                                      condition,
                                      id,
                                      is_labeled,
                                      log_tag_context,
                                      query_log_tag_prefix)) {
      if (ctx->rc != GRN_SUCCESS) {
        succeeded = GRN_FALSE;
        break;
      }
    }
  }

exit :
  GRN_OBJ_FIN(ctx, &tsorted_ids);

  return succeeded;
}

static Drilldown *
grn_select_drilldowns_add(grn_ctx *ctx,
                          grn_hash **drilldowns,
                          const char *label,
                          size_t label_len,
                          const char *log_tag)
{
  Drilldown *drilldown = NULL;
  int added;

  if (!*drilldowns) {
    *drilldowns = grn_hash_create(ctx,
                                  NULL,
                                  GRN_TABLE_MAX_KEY_SIZE,
                                  sizeof(Drilldown),
                                  GRN_OBJ_TABLE_HASH_KEY |
                                  GRN_OBJ_KEY_VAR_SIZE |
                                  GRN_HASH_TINY);
    if (!*drilldowns) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select]%s[drilldowns] "
                       "failed to allocate drilldowns data: %s",
                       log_tag,
                       ctx->errbuf);
      return NULL;
    }
  }

  grn_hash_add(ctx,
               *drilldowns,
               label,
               label_len,
               (void **)&drilldown,
               &added);
  if (added) {
    new(drilldown) Drilldown(ctx, label, label_len);
  }

  return drilldown;
}

static bool
grn_select_prepare_drilldowns(grn_ctx *ctx,
                              grn_select_data *data)
{
  if (data->drilldown.keys.length > 0) {
    data->drilldown.parsed_keys =
      grn_table_group_keys_parse(ctx,
                                 data->tables.result,
                                 data->drilldown.keys.value,
                                 data->drilldown.keys.length,
                                 &(data->drilldown.n_parsed_keys));
    if (data->drilldown.parsed_keys) {
      int i;
      grn_obj buffer;

      GRN_TEXT_INIT(&buffer, 0);
      for (i = 0; i < data->drilldown.n_parsed_keys; i++) {
        GRN_BULK_REWIND(&buffer);
        grn_text_printf(ctx, &buffer, "drilldown%d", i);
        auto drilldown = grn_select_drilldowns_add(ctx,
                                                   &(data->drilldowns),
                                                   GRN_TEXT_VALUE(&buffer),
                                                   GRN_TEXT_LEN(&buffer),
                                                   "");
        if (!drilldown) {
          continue;
        }

        drilldown->parsed_keys = data->drilldown.parsed_keys + i;
        drilldown->n_parsed_keys = 1;

#define COPY(field)                                     \
        drilldown->field = data->drilldown.field

        COPY(sort_keys);
        COPY(output_columns);
        COPY(offset);
        COPY(limit);
        COPY(calc_types);
        COPY(calc_target_name);
        COPY(filter);
        COPY(adjuster);
        COPY(max_n_target_records);

#undef COPY
      }
    }
  }

  if (!data->drilldowns) {
    return true;
  }

  {
    const bool is_labeled = (data->drilldown.keys.length == 0);
    if (!grn_select_drilldowns_execute(ctx,
                                       data,
                                       data->drilldowns,
                                       data->tables.result,
                                       data->slices,
                                       data->filter.condition.expression,
                                       is_labeled,
                                       "",
                                       "")) {
      return false;
    }
  }

  {
    unsigned int n_available_results = 0;

    GRN_HASH_EACH_BEGIN(ctx, data->drilldowns, cursor, id) {
      Drilldown *drilldown;
      grn_hash_cursor_get_value(ctx,
                                cursor,
                                reinterpret_cast<void **>(&drilldown));
      auto result = &(drilldown->result);
      if (result->table) {
        n_available_results++;
      }
    } GRN_HASH_EACH_END(ctx, cursor);

    if (data->drilldown.keys.length > 0) {
      data->output.n_elements += n_available_results;
    } else {
      if (n_available_results > 0) {
        data->output.n_elements += 1;
      }
    }
  }

  return true;
}

static bool
grn_select_output_drilldowns(grn_ctx *ctx,
                             grn_select_data *data,
                             grn_hash *drilldowns,
                             const bool is_labeled,
                             grn_obj *condition,
                             const char *log_tag_prefix,
                             const char *query_log_tag_prefix)
{
  bool succeeded = true;
  unsigned int n_available_results = 0;

  if (!drilldowns) {
    return true;
  }

  data->output.formatter->drilldowns_label(ctx, data);

  GRN_HASH_EACH_BEGIN(ctx, drilldowns, cursor, id) {
    Drilldown *drilldown;
    grn_hash_cursor_get_value(ctx,
                              cursor,
                              reinterpret_cast<void **>(&drilldown));
    auto result = &(drilldown->result);
    if (result->table) {
      n_available_results++;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  data->output.formatter->drilldowns_open(ctx, data, n_available_results);

  GRN_HASH_EACH_BEGIN(ctx, drilldowns, cursor, id) {
    grn_obj *target_table;
    uint32_t n_hits;
    int offset;
    int limit;

    Drilldown *drilldown;
    grn_hash_cursor_get_value(ctx,
                              cursor,
                              reinterpret_cast<void **>(&drilldown));
    auto result = &(drilldown->result);

    if (!result->table) {
      continue;
    }

    if (drilldown->filtered_result) {
      target_table = drilldown->filtered_result;
    } else {
      target_table = result->table;
    }

    n_hits = grn_table_size(ctx, target_table);

    offset = drilldown->offset;
    limit = drilldown->limit;
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
      int n_sort_keys;
      sort_keys = grn_table_sort_keys_parse(ctx,
                                            target_table,
                                            drilldown->sort_keys.value,
                                            drilldown->sort_keys.length,
                                            &n_sort_keys);
      if (sort_keys) {
        grn_obj *sorted;
        sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                  NULL, target_table);
        if (sorted) {
          grn_table_sort(ctx, target_table, offset, limit,
                         sorted, sort_keys, n_sort_keys);
          GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                        ":", "%ssort(%d): %.*s",
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
          grn_select_create_no_sort_keys_sorted_table(ctx,
                                                      data,
                                                      target_table);
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

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "%s(%d)",
                  drilldown_output_query_log_tag_prefix,
                  n_hits);
  } GRN_HASH_EACH_END(ctx, cursor);

  data->output.formatter->drilldowns_close(ctx, data);

  return succeeded;
}

static bool
grn_select_slice_execute(grn_ctx *ctx,
                         grn_select_data *data,
                         grn_obj *table,
                         Slice *slice)
{
  char log_tag[GRN_TABLE_MAX_KEY_SIZE];
  Filter *filter;

  grn_snprintf(log_tag, GRN_TABLE_MAX_KEY_SIZE, GRN_TABLE_MAX_KEY_SIZE,
               "[select][slices][%.*s]",
               (int)(slice->label.length),
               slice->label.value);
  filter = &(slice->filter);
  if (filter->query.length == 0 && filter->filter.length == 0) {
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
               (int)(slice->label.length),
               slice->label.value);

  slice->tables.target = table;
  if (slice->dynamic_columns.initial) {
    slice->tables.initial =
      grn_select_create_all_selected_result_table(ctx, slice->tables.target);
    if (!slice->tables.initial) {
      return false;
    }
    grn_select_apply_dynamic_columns(ctx,
                                     data,
                                     slice->tables.initial,
                                     DynamicColumnStage::INITIAL,
                                     slice->dynamic_columns.initial,
                                     filter->condition.expression,
                                     log_tag,
                                     query_log_tag_prefix);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
    slice->tables.target = slice->tables.initial;
  }

  if (!grn_filter_execute(ctx,
                          data,
                          filter,
                          slice->tables.target,
                          &(slice->dynamic_columns),
                          log_tag,
                          query_log_tag_prefix)) {
    return false;
  }
  grn_expr_set_parent(ctx,
                      filter->condition.expression,
                      data->filter.condition.expression);

  slice->tables.result = filter->filtered;
  GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                ":", "%.*s(%d)",
                (int)(strlen(query_log_tag_prefix) - 1),
                query_log_tag_prefix,
                grn_table_size(ctx, slice->tables.result));

  if (slice->dynamic_columns.filtered) {
    grn_select_apply_dynamic_columns(ctx,
                                     data,
                                     slice->tables.result,
                                     DynamicColumnStage::FILTERED,
                                     slice->dynamic_columns.filtered,
                                     slice->filter.condition.expression,
                                     log_tag,
                                     query_log_tag_prefix);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
  }

  if (!grn_filter_execute_post_filter(ctx,
                                      data,
                                      &(slice->filter),
                                      slice->tables.result,
                                      log_tag,
                                      query_log_tag_prefix)) {
    return false;
  }

  if (slice->filter.post_filtered) {
    slice->tables.result = slice->filter.post_filtered;
  }

  if (slice->drilldowns) {
    if (!grn_select_drilldowns_execute(ctx,
                                       data,
                                       slice->drilldowns,
                                       slice->tables.result,
                                       NULL,
                                       slice->filter.condition.expression,
                                       true,
                                       log_tag,
                                       query_log_tag_prefix)) {
      return false;
    }
  }

  return true;
}

static grn_bool
grn_select_slices_execute(grn_ctx *ctx,
                          grn_select_data *data,
                          grn_obj *table,
                          grn_hash *slices)
{
  grn_bool succeeded = GRN_TRUE;

  GRN_HASH_EACH_BEGIN(ctx, slices, cursor, id) {
    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    auto slice = static_cast<Slice *>(value);
    if (!grn_select_slice_execute(ctx, data, table, slice)) {
      succeeded = GRN_FALSE;
      break;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  return succeeded;
}

static grn_bool
grn_select_prepare_slices(grn_ctx *ctx,
                          grn_select_data *data)
{
  if (!data->slices) {
    return GRN_TRUE;
  }

  if (!grn_select_slices_execute(ctx, data, data->tables.result, data->slices)) {
    return GRN_FALSE;
  }

  data->output.n_elements += 1;

  return GRN_TRUE;
}

static grn_bool
grn_select_output_slices(grn_ctx *ctx,
                         grn_select_data *data)
{
  grn_bool succeeded = GRN_TRUE;
  unsigned int n_available_results = 0;

  if (!data->slices) {
    return GRN_TRUE;
  }

  data->output.formatter->slices_label(ctx, data);

  GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    auto slice = static_cast<Slice *>(value);
    if (slice->tables.result) {
      n_available_results++;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  data->output.formatter->slices_open(ctx, data, n_available_results);

  GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
    uint32_t n_hits;
    int offset;
    int limit;

    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    auto slice = static_cast<Slice *>(value);
    if (!slice->tables.result) {
      continue;
    }

    n_hits = grn_table_size(ctx, slice->tables.result);

    offset = slice->offset;
    limit = slice->limit;
    grn_output_range_normalize(ctx, n_hits, &offset, &limit);

    char log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(log_tag_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[select][slices][%.*s]",
                 (int)(slice->label.length),
                 slice->label.value);
    char query_log_tag_prefix[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(query_log_tag_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "slices[%.*s].",
                 (int)(slice->label.length),
                 slice->label.value);

    if (slice->sort_keys.length == 0) {
      slice->tables.output = slice->tables.result;
    } else {
      grn_table_sort_key *sort_keys;
      int n_sort_keys;
      sort_keys = grn_table_sort_keys_parse(ctx,
                                            slice->tables.result,
                                            slice->sort_keys.value,
                                            slice->sort_keys.length,
                                            &n_sort_keys);
      if (sort_keys) {
        slice->tables.sorted =
          grn_table_create(ctx,
                           NULL, 0,
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
                         sort_keys, n_sort_keys);
          GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                        ":", "%ssort(%d): %.*s",
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
    if (slice->drilldowns) {
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

    succeeded = grn_proc_select_output_columns_close(ctx,
                                                     &format,
                                                     slice->tables.output);
    if (!succeeded) {
      break;
    }

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "slices[%.*s].output(%d)",
                  (int)(slice->label.length),
                  slice->label.value,
                  limit);
  } GRN_HASH_EACH_END(ctx, cursor);

  data->output.formatter->slices_close(ctx, data);

  return succeeded;
}

static bool
grn_select_data_output_drilldowns(grn_ctx *ctx,
                                  grn_select_data *data)
{
  return grn_select_output_drilldowns(ctx,
                                      data,
                                      data->drilldowns,
                                      (data->drilldown.keys.length == 0),
                                      data->filter.condition.expression,
                                      "[select]",
                                      "");
}

static grn_bool
grn_select_output(grn_ctx *ctx, grn_select_data *data)
{
  grn_bool succeeded = GRN_TRUE;

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

    if (data->slices) {
      n_additional_elements++;
    }
    if (data->drilldowns) {
      n_additional_elements++;
    }

    succeeded = grn_select_output_match_open(ctx,
                                             data,
                                             &format,
                                             n_additional_elements);
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
  if (data->drilldown.keys.length == 0) {
    GRN_OUTPUT_MAP_OPEN("DRILLDOWNS", n_result_sets);
  }
}

static void
grn_select_output_drilldowns_close_v1(grn_ctx *ctx, grn_select_data *data)
{
  if (data->drilldown.keys.length == 0) {
    GRN_OUTPUT_MAP_CLOSE();
  }
}

static void
grn_select_output_drilldown_label_v1(grn_ctx *ctx,
                                     grn_select_data *data,
                                     Drilldown *drilldown)
{
  if (data->drilldown.keys.length == 0) {
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
  grn_select_output_drilldown_label_v1
};

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
  if (data->drilldown.keys.length == 0) {
    GRN_OUTPUT_STR(drilldown->label.value, drilldown->label.length);
  } else {
    grn_obj *key;
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_len;

    key = drilldown->parsed_keys[0].key;
    if (grn_obj_is_accessor(ctx, key)) {
      grn_accessor *a = (grn_accessor *)key;
      while (a->next) {
        a = a->next;
      }
      key = a->obj;
    }
    if (grn_obj_is_column(ctx, key)) {
      name_len = grn_column_name(ctx, key, name, GRN_TABLE_MAX_KEY_SIZE);
    } else {
      name_len = grn_obj_name(ctx, key, name, GRN_TABLE_MAX_KEY_SIZE);
    }
    GRN_OUTPUT_STR(name, name_len);
  }
}

static grn_select_output_formatter grn_select_output_formatter_v3 = {
  grn_select_output_slices_label_v3,
  grn_select_output_slices_open_v3,
  grn_select_output_slices_close_v3,
  grn_select_output_slice_label_v3,
  grn_select_output_drilldowns_label_v3,
  grn_select_output_drilldowns_open_v3,
  grn_select_output_drilldowns_close_v3,
  grn_select_output_drilldown_label_v3
};

static void
grn_select_prepare_cache_key(grn_ctx *ctx,
                             grn_select_data *data,
                             grn::TextBulk &cache_key)
{
#define PUT_CACHE_KEY(string)                                           \
  if ((string).length > 0) {                                            \
    GRN_TEXT_PUT(ctx, *cache_key, (string).value, (string).length);     \
  }                                                                     \
  GRN_TEXT_PUTC(ctx, *cache_key, '\0');                                 \
  if (GRN_TEXT_LEN(*cache_key) > GRN_CACHE_MAX_KEY_SIZE) {              \
    return;                                                             \
  }

  PUT_CACHE_KEY(data->table);
  PUT_CACHE_KEY(data->filter.match_columns);
  PUT_CACHE_KEY(data->filter.query);
  PUT_CACHE_KEY(data->filter.filter);
  PUT_CACHE_KEY(data->filter.post_filter);
  PUT_CACHE_KEY(data->scorer);
  PUT_CACHE_KEY(data->sort_keys);
  PUT_CACHE_KEY(data->output_columns);
#define PUT_CACHE_KEY_DRILLDOWN(drilldown)                              \
  do {                                                                  \
    PUT_CACHE_KEY(drilldown->keys);                                     \
    PUT_CACHE_KEY(drilldown->sort_keys);                                \
    PUT_CACHE_KEY(drilldown->output_columns);                           \
    PUT_CACHE_KEY(drilldown->label);                                    \
    PUT_CACHE_KEY(drilldown->calc_target_name);                         \
    PUT_CACHE_KEY(drilldown->filter);                                   \
    PUT_CACHE_KEY(drilldown->adjuster);                                 \
    PUT_CACHE_KEY(drilldown->table_name);                               \
    GRN_INT32_PUT(ctx, *cache_key, drilldown->offset);                  \
    GRN_INT32_PUT(ctx, *cache_key, drilldown->limit);                   \
    GRN_INT32_PUT(ctx, *cache_key, drilldown->max_n_target_records);    \
    GRN_TEXT_PUT(ctx,                                                   \
                 *cache_key,                                            \
                 &(drilldown->calc_types),                              \
                 sizeof(grn_table_group_flags));                        \
  } while (false)
  if (data->slices) {
    GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
      void *value;
      grn_hash_cursor_get_value(ctx, cursor, &value);
      auto slice = static_cast<Slice *>(value);
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
      if (slice->drilldowns) {
        GRN_HASH_EACH_BEGIN(ctx, slice->drilldowns, cursor, id) {
          Drilldown *drilldown;
          grn_hash_cursor_get_value(ctx,
                                    cursor,
                                    reinterpret_cast<void **>(&drilldown));
          PUT_CACHE_KEY_DRILLDOWN(drilldown);
        } GRN_HASH_EACH_END(ctx, cursor);
      }
    } GRN_HASH_EACH_END(ctx, cursor);
  }
#define PUT_CACHE_KEY_COLUMNS(dynamic_columns)                          \
  do {                                                                  \
    GRN_HASH_EACH_BEGIN(ctx, dynamic_columns, cursor, id) {             \
      void *value;                                                      \
      grn_hash_cursor_get_value(ctx, cursor, &value);                   \
      auto dynamic_column = static_cast<DynamicColumn *>(value);        \
      PUT_CACHE_KEY(dynamic_column->label);                             \
      GRN_TEXT_PUT(ctx,                                                 \
                   *cache_key,                                          \
                   &(dynamic_column->stage),                            \
                   sizeof(DynamicColumnStage));                         \
      GRN_RECORD_PUT(ctx,                                               \
                     *cache_key,                                        \
                     DB_OBJ(dynamic_column->type)->id);                 \
      GRN_TEXT_PUT(ctx,                                                 \
                   *cache_key,                                          \
                   &(dynamic_column->flags),                            \
                   sizeof(grn_column_flags));                           \
      PUT_CACHE_KEY(dynamic_column->value);                             \
      PUT_CACHE_KEY(dynamic_column->window.sort_keys);                  \
      PUT_CACHE_KEY(dynamic_column->window.group_keys);                 \
    } GRN_HASH_EACH_END(ctx, cursor);                                   \
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
  if (data->drilldown.keys.length > 0) {
    auto drilldown = &(data->drilldown);
    PUT_CACHE_KEY_DRILLDOWN(drilldown);
  }
  if (data->drilldowns) {
    GRN_HASH_EACH_BEGIN(ctx, data->drilldowns, cursor, id) {
      Drilldown *drilldown;
      grn_hash_cursor_get_value(ctx,
                                cursor,
                                reinterpret_cast<void **>(&drilldown));
      PUT_CACHE_KEY_DRILLDOWN(drilldown);
    } GRN_HASH_EACH_END(ctx, cursor);
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
  GRN_TEXT_PUT(ctx, *cache_key,
               &(ctx->impl->command.version), sizeof(grn_command_version));
  GRN_BOOL_PUT(ctx, *cache_key, &(ctx->impl->output.is_pretty));
#undef PUT_CACHE_KEY
}

static grn_rc
grn_select(grn_ctx *ctx, grn_select_data *data)
{
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
      GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_CACHE,
                    ":", "cache(%" GRN_FMT_LLD ")",
                    (long long int)GRN_TEXT_LEN(outbuf));
      return ctx->rc;
    }
  }

  original_match_escalation_threshold =
    grn_ctx_get_match_escalation_threshold(ctx);
  original_force_match_escalation =
    grn_ctx_get_force_match_escalation(ctx);

  if (data->match_escalation_threshold.length > 0) {
    const char *end, *rest;
    long long int threshold;
    end =
      data->match_escalation_threshold.value +
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

  data->tables.target = grn_ctx_get(ctx, data->table.value, data->table.length);
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
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "select(%d)", nhits);

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

      grn_output_range_normalize(ctx, nhits,
                                     &(data->offset), &(data->limit));

      if (!grn_select_sort(ctx, data)) {
        goto exit;
      }

      if (!grn_select_load(ctx, data)) {
        goto exit;
      }

      if (!grn_select_apply_output_dynamic_columns(ctx, data)) {
        goto exit;
      }

      if (!grn_select_prepare_slices(ctx, data)) {
        goto exit;
      }

      if (!grn_select_prepare_drilldowns(ctx, data)) {
        goto exit;
      }

      succeeded = grn_select_output(ctx, data);
      if (!succeeded) {
        goto exit;
      }
    }
    if (!ctx->rc &&
        data->cacheable &&
        GRN_TEXT_LEN(*cache_key) <= GRN_CACHE_MAX_KEY_SIZE &&
        (!data->cache.value ||
         data->cache.length != 2 ||
         data->cache.value[0] != 'n' ||
         data->cache.value[1] != 'o')) {
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

exit :
  grn_ctx_set_match_escalation_threshold(ctx,
                                         original_match_escalation_threshold);
  grn_ctx_set_force_match_escalation(ctx, original_force_match_escalation);

  /* GRN_LOG(ctx, GRN_LOG_NONE, "%d", ctx->seqno); */

  return ctx->rc;
}

static bool
grn_select_fill_drilldown_labels(grn_ctx *ctx,
                                 grn_user_data *user_data,
                                 grn_hash **drilldowns,
                                 const char *prefix,
                                 const char *log_tag)
{
  grn_obj *vars;
  grn_table_cursor *cursor;
  int prefix_len;

  vars = grn_plugin_proc_get_vars(ctx, user_data);

  cursor = grn_table_cursor_open(ctx, vars, NULL, 0, NULL, 0, 0, -1, 0);
  if (!cursor) {
    return false;
  }

  prefix_len = strlen(prefix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    auto name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    auto name = static_cast<const char *>(key);
    if (name_len > prefix_len + 2 &&
        strncmp(prefix, name, prefix_len) == 0 &&
        name[prefix_len] == '[') {
      auto label_end =
        static_cast<const char *>(memchr(name + prefix_len + 2,
                                         ']',
                                         name_len - prefix_len - 2));
      if (!label_end) {
        continue;
      }
      auto label_len = (label_end - name) - prefix_len - 1;
      grn_select_drilldowns_add(ctx,
                                drilldowns,
                                name + prefix_len + 1,
                                label_len,
                                log_tag);
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return true;
}

static bool
grn_select_fill_drilldown_columns(grn_ctx *ctx,
                                  grn_user_data *user_data,
                                  Drilldown *drilldown,
                                  const char *parameter_key)
{
  char prefix[GRN_TABLE_MAX_KEY_SIZE];

  grn_snprintf(prefix,
               GRN_TABLE_MAX_KEY_SIZE,
               GRN_TABLE_MAX_KEY_SIZE,
               "%s[%.*s].",
               parameter_key,
               (int)(drilldown->label.length),
               drilldown->label.value);
  return grn_dynamic_columns_fill(ctx,
                                  user_data,
                                  &(drilldown->dynamic_columns),
                                  prefix,
                                  strlen(prefix));
}

static bool
grn_select_fill_drilldowns(grn_ctx *ctx,
                           grn_user_data *user_data,
                           grn_hash **drilldowns,
                           const char *prefix,
                           bool need_backward_compatibility,
                           const char *log_tag)
{
  bool succeeded = true;

  char drilldowns_prefix[GRN_TABLE_MAX_KEY_SIZE];
  grn_snprintf(drilldowns_prefix,
               GRN_TABLE_MAX_KEY_SIZE,
               GRN_TABLE_MAX_KEY_SIZE,
               "%sdrilldowns",
               prefix);
  if (!grn_select_fill_drilldown_labels(ctx,
                                        user_data,
                                        drilldowns,
                                        drilldowns_prefix,
                                        log_tag)) {
    return false;
  }

  char drilldown_prefix[GRN_TABLE_MAX_KEY_SIZE];
  if (need_backward_compatibility) {
    grn_snprintf(drilldown_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "%sdrilldown",
                 prefix);
    if (!grn_select_fill_drilldown_labels(ctx,
                                          user_data,
                                          drilldowns,
                                          drilldown_prefix,
                                          log_tag)) {
      return false;
    }
  }

  GRN_HASH_EACH_BEGIN(ctx, *drilldowns, cursor, id) {
    grn_obj *keys = NULL;
    grn_obj *sort_keys = NULL;
    grn_obj *output_columns = NULL;
    grn_obj *offset = NULL;
    grn_obj *limit = NULL;
    grn_obj *calc_types = NULL;
    grn_obj *calc_target = NULL;
    grn_obj *filter = NULL;
    grn_obj *adjuster = NULL;
    grn_obj *table = NULL;
    grn_obj *max_n_target_records = NULL;

    Drilldown *drilldown;
    grn_hash_cursor_get_value(ctx,
                              cursor,
                              reinterpret_cast<void **>(&drilldown));

    succeeded = grn_select_fill_drilldown_columns(ctx,
                                                  user_data,
                                                  drilldown,
                                                  drilldowns_prefix);
    if (!succeeded) {
      break;
    }

    if (need_backward_compatibility) {
      succeeded = grn_select_fill_drilldown_columns(ctx,
                                                    user_data,
                                                    drilldown,
                                                    drilldown_prefix);
      if (!succeeded) {
        break;
      }
    }

#define GET_VAR_RAW(parameter_key, name) do {                           \
      if (!name) {                                                      \
        char key_name[GRN_TABLE_MAX_KEY_SIZE];                          \
        grn_snprintf(key_name,                                          \
                     GRN_TABLE_MAX_KEY_SIZE,                            \
                     GRN_TABLE_MAX_KEY_SIZE,                            \
                     "%s%s[%.*s].%s",                                   \
                     prefix,                                            \
                     (parameter_key),                                   \
                     (int)(drilldown->label.length),                    \
                     drilldown->label.value,                            \
                     #name);                                            \
        name = grn_plugin_proc_get_var(ctx, user_data, key_name, -1);   \
      }                                                                 \
    } while (GRN_FALSE)

#define GET_VAR(name) do {                                              \
      GET_VAR_RAW("drilldowns", name);                                  \
      if (need_backward_compatibility) {                                \
        GET_VAR_RAW("drilldown", name);                                 \
      }                                                                 \
    } while (GRN_FALSE)

    GET_VAR(keys);
    GET_VAR(sort_keys);
    if (!sort_keys) {
      grn_obj *sortby = NULL;
      GET_VAR(sortby);
      sort_keys = sortby;
    }
    GET_VAR(output_columns);
    GET_VAR(offset);
    GET_VAR(limit);
    GET_VAR(calc_types);
    GET_VAR(calc_target);
    GET_VAR(filter);
    GET_VAR(adjuster);
    GET_VAR(table);
    GET_VAR(max_n_target_records);

#undef GET_VAR

#undef GET_VAR_RAW

    grn_drilldown_data_fill(ctx,
                            drilldown,
                            keys,
                            sort_keys,
                            output_columns,
                            offset,
                            limit,
                            calc_types,
                            calc_target,
                            filter,
                            adjuster,
                            table,
                            max_n_target_records);
  } GRN_HASH_EACH_END(ctx, cursor);

  return succeeded;
}

static grn_bool
grn_select_data_fill_drilldowns(grn_ctx *ctx,
                                grn_user_data *user_data,
                                grn_select_data *data)
{
  grn_obj *drilldown;

  drilldown = grn_plugin_proc_get_var(ctx, user_data, "drilldown", -1);
  if (GRN_TEXT_LEN(drilldown) > 0) {
    grn_obj *sort_keys;

    sort_keys = grn_plugin_proc_get_var(ctx, user_data,
                                        "drilldown_sort_keys", -1);
    if (GRN_TEXT_LEN(sort_keys) == 0) {
      /* For backward compatibility */
      sort_keys = grn_plugin_proc_get_var(ctx, user_data,
                                          "drilldown_sortby", -1);
    }
    grn_drilldown_data_fill(
      ctx,
      &(data->drilldown),
      drilldown,
      sort_keys,
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_output_columns", -1),
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_offset", -1),
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_limit", -1),
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_calc_types", -1),
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_calc_target", -1),
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_filter", -1),
      grn_plugin_proc_get_var(ctx, user_data, "drilldown_adjuster", -1),
      NULL,
      grn_plugin_proc_get_var(ctx, user_data,
                              "drilldown_max_n_target_records", -1));
    return GRN_TRUE;
  } else {
    return grn_select_fill_drilldowns(ctx,
                                      user_data,
                                      &(data->drilldowns),
                                      "",
                                      true,
                                      "");
  }
}

static Slice *
grn_select_data_slices_add(grn_ctx *ctx,
                           grn_select_data *data,
                           const char *label,
                           size_t label_len)
{
  Slice *slice = NULL;
  int added;

  if (!data->slices) {
    data->slices = grn_hash_create(ctx,
                                   NULL,
                                   GRN_TABLE_MAX_KEY_SIZE,
                                   sizeof(Slice),
                                   GRN_OBJ_TABLE_HASH_KEY |
                                   GRN_OBJ_KEY_VAR_SIZE |
                                   GRN_HASH_TINY);
    if (!data->slices) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][slices] "
                       "failed to allocate slices data: %s",
                       ctx->errbuf);
      return NULL;
    }
  }

  grn_hash_add(ctx,
               data->slices,
               label,
               label_len,
               reinterpret_cast<void **>(&slice),
               &added);
  if (added) {
    new(slice) Slice(ctx, label, label_len);
  }

  return slice;
}

static bool
grn_select_data_fill_slice_labels(grn_ctx *ctx,
                                  grn_user_data *user_data,
                                  grn_select_data *data)
{
  grn_obj *vars;
  grn_table_cursor *cursor;
  const char *prefix = "slices[";
  int prefix_len;

  vars = grn_plugin_proc_get_vars(ctx, user_data);

  cursor = grn_table_cursor_open(ctx, vars, NULL, 0, NULL, 0, 0, -1, 0);
  if (!cursor) {
    return false;
  }

  prefix_len = strlen(prefix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    auto name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    auto name = static_cast<const char *>(key);
    if (name_len > prefix_len + 1 &&
        strncmp(prefix, name, prefix_len) == 0) {
      auto label_end =
        static_cast<const char *>(memchr(name + prefix_len + 1,
                                         ']',
                                         name_len - prefix_len - 1));
      if (!label_end) {
        continue;
      }
      auto label_len = (label_end - name) - prefix_len;
      grn_select_data_slices_add(ctx,
                                 data,
                                 name + prefix_len,
                                 label_len);
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return true;
}

static bool
grn_select_data_fill_slices(grn_ctx *ctx,
                            grn_user_data *user_data,
                            grn_select_data *data)
{
  if (!grn_select_data_fill_slice_labels(ctx, user_data, data)) {
    return false;
  }

  bool success = true;
  GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
    char slice_prefix[GRN_TABLE_MAX_KEY_SIZE];
    char key_name[GRN_TABLE_MAX_KEY_SIZE];
    grn_obj *match_columns;
    grn_obj *query;
    grn_obj *query_expander;
    grn_obj *query_flags;
    grn_obj *query_options;
    grn_obj *filter;
    grn_obj *post_filter;
    grn_obj *sort_keys;
    grn_obj *output_columns;
    grn_obj *offset;
    grn_obj *limit;

    void *value;
    grn_hash_cursor_get_value(ctx, cursor, &value);
    auto slice = static_cast<Slice *>(value);

    grn_snprintf(slice_prefix,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "slices[%.*s].",
                 (int)(slice->label.length),
                 slice->label.value);

#define GET_VAR(name)                                                   \
    grn_snprintf(key_name,                                              \
                 GRN_TABLE_MAX_KEY_SIZE,                                \
                 GRN_TABLE_MAX_KEY_SIZE,                                \
                 "%s%s", slice_prefix, #name);                          \
    name = grn_plugin_proc_get_var(ctx, user_data, key_name, -1);

    GET_VAR(match_columns);
    GET_VAR(query);
    GET_VAR(query_expander);
    GET_VAR(query_flags);
    GET_VAR(query_options);
    GET_VAR(filter);
    GET_VAR(post_filter);
    GET_VAR(sort_keys);
    GET_VAR(output_columns);
    GET_VAR(offset);
    GET_VAR(limit);

#undef GET_VAR

    grn_slice_fill(ctx,
                   slice,
                   match_columns,
                   query,
                   query_expander,
                   query_flags,
                   query_options,
                   filter,
                   post_filter,
                   sort_keys,
                   output_columns,
                   offset,
                   limit);

    char log_tag[GRN_TABLE_MAX_KEY_SIZE];
    grn_snprintf(log_tag,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[slices][%.*s]",
                 (int)(slice->label.length),
                 slice->label.value);
    success = grn_select_fill_drilldowns(ctx,
                                         user_data,
                                         &(slice->drilldowns),
                                         slice_prefix,
                                         false,
                                         log_tag);
    if (!success) {
      break;
    }

    success = grn_dynamic_columns_fill(ctx,
                                       user_data,
                                       &(slice->dynamic_columns),
                                       slice_prefix,
                                       strlen(slice_prefix));
    if (!success) {
      break;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  return success;
}

static grn_obj *
command_select(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  SelectExecutor data(ctx, nargs, args, user_data);

  data.table.value = grn_plugin_proc_get_var_string(ctx, user_data,
                                                    "table", -1,
                                                    &(data.table.length));
#define GET_VAR(name)                                           \
  grn_plugin_proc_get_var(ctx, user_data, name, strlen(name))

  {
    grn_obj *query_expander;

    query_expander = GET_VAR("query_expander");
    if (GRN_TEXT_LEN(query_expander) == 0) {
      query_expander = GET_VAR("query_expansion");
    }

    grn_filter_fill(ctx,
                    &(data.filter),
                    GET_VAR("match_columns"),
                    GET_VAR("query"),
                    query_expander,
                    GET_VAR("query_flags"),
                    GET_VAR("query_options"),
                    GET_VAR("filter"),
                    GET_VAR("post_filter"));
  }
#undef GET_VAR

  data.scorer.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "scorer", -1,
                                   &(data.scorer.length));
  data.sort_keys.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "sort_keys", -1,
                                   &(data.sort_keys.length));
  if (data.sort_keys.length == 0) {
    /* For backward compatibility */
    data.sort_keys.value =
      grn_plugin_proc_get_var_string(ctx, user_data,
                                     "sortby", -1,
                                     &(data.sort_keys.length));
  }
  data.output_columns.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "output_columns", -1,
                                   &(data.output_columns.length));
  data.offset = grn_plugin_proc_get_var_int32(ctx, user_data,
                                              "offset", -1,
                                              0);
  data.limit = grn_plugin_proc_get_var_int32(ctx, user_data,
                                             "limit", -1,
                                             GRN_SELECT_DEFAULT_LIMIT);

  data.cache.value = grn_plugin_proc_get_var_string(ctx, user_data,
                                                    "cache", -1,
                                                    &(data.cache.length));
  data.match_escalation_threshold.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "match_escalation_threshold", -1,
                                   &(data.match_escalation_threshold.length));

  data.adjuster.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "adjuster", -1,
                                   &(data.adjuster.length));
  data.match_escalation.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "match_escalation", -1,
                                   &(data.match_escalation.length));
  data.load.table.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "load_table", -1,
                                   &(data.load.table.length));
  data.load.columns.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "load_columns", -1,
                                   &(data.load.columns.length));
  data.load.values.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "load_values", -1,
                                   &(data.load.values.length));

  if (!grn_select_data_fill_slices(ctx, user_data, &data)) {
    goto exit;
  }

  if (!grn_select_data_fill_drilldowns(ctx, user_data, &data)) {
    goto exit;
  }

  if (!grn_dynamic_columns_fill(ctx,
                                user_data,
                                &(data.dynamic_columns),
                                NULL,
                                0)) {
    goto exit;
  }

  grn_select(ctx, &data);

exit :
  if (data.drilldown.parsed_keys) {
    grn_table_sort_key_close(ctx,
                             data.drilldown.parsed_keys,
                             data.drilldown.n_parsed_keys);
  }

  return NULL;
}

#define N_VARS 34
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
  grn_plugin_expr_var_init(ctx, &(vars[33]), "drilldown_max_n_target_records", -1);
}

void
grn_proc_init_select(grn_ctx *ctx)
{
  DEFINE_VARS;

  init_vars(ctx, vars);
  grn_plugin_command_create(ctx,
                            "select", -1,
                            command_select,
                            N_VARS - 1,
                            vars + 1);
}

static grn_obj *
command_define_selector(grn_ctx *ctx, int nargs, grn_obj **args,
                        grn_user_data *user_data)
{
  uint32_t i, nvars;
  grn_expr_var *vars;

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  for (i = 1; i < nvars; i++) {
    grn_obj *var;
    var = grn_plugin_proc_get_var_by_offset(ctx, user_data, i);
    GRN_TEXT_SET(ctx, &((vars + i)->value),
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

void
grn_proc_init_define_selector(grn_ctx *ctx)
{
  DEFINE_VARS;

  init_vars(ctx, vars);
  grn_plugin_command_create(ctx,
                            "define_selector", -1,
                            command_define_selector,
                            N_VARS,
                            vars);
}
