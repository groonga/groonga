/*
  Copyright(C) 2009-2018  Brazil
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

#include "../grn_cache.h"
#include "../grn_ctx_impl.h"
#include "../grn_expr.h"
#include "../grn_ii.h"
#include "../grn_output.h"
#include "../grn_posting.h"
#include "../grn_proc.h"
#include "../grn_str.h"
#include "../grn_util.h"
#include "../grn_window_function_executor.h"

#include <groonga/plugin.h>

#define GRN_SELECT_INTERNAL_VAR_MATCH_COLUMNS "$match_columns"

#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key, _nsubrecs"

typedef enum {
  GRN_COLUMN_STAGE_INITIAL,
  GRN_COLUMN_STAGE_RESULT_SET,
  GRN_COLUMN_STAGE_FILTERED,
  GRN_COLUMN_STAGE_OUTPUT,
  GRN_COLUMN_STAGE_GROUP,
} grn_column_stage;

typedef struct {
  grn_raw_string label;
  grn_column_stage stage;
  grn_obj *type;
  grn_column_flags flags;
  grn_raw_string value;
  struct {
    grn_raw_string sort_keys;
    grn_raw_string group_keys;
  } window;
  grn_obj dependency_column_names;
} grn_column_data;

typedef struct {
  grn_hash *initial;
  grn_hash *result_set;
  grn_hash *filtered;
  grn_hash *output;
  grn_hash *group;
} grn_columns;

typedef struct {
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
} grn_filter_data;

typedef struct {
  grn_raw_string label;
  grn_filter_data filter;
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
  grn_columns columns;
} grn_slice_data;

typedef struct {
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
  grn_columns columns;
  grn_table_group_result result;
  grn_obj *filtered_result;
} grn_drilldown_data;

typedef struct _grn_select_output_formatter grn_select_output_formatter;

typedef struct {
  /* inputs */
  grn_raw_string table;
  grn_filter_data filter;
  grn_raw_string scorer;
  grn_raw_string sort_keys;
  grn_raw_string output_columns;
  grn_raw_string default_output_columns;
  int offset;
  int limit;
  grn_hash *slices;
  grn_drilldown_data drilldown;
  grn_hash *drilldowns;
  grn_raw_string cache;
  grn_raw_string match_escalation_threshold;
  grn_raw_string adjuster;
  grn_raw_string match_escalation;
  grn_columns columns;

  /* for processing */
  struct {
    grn_obj *target;
    grn_obj *initial;
    grn_obj *result;
    grn_obj *sorted;
    grn_obj *output;
  } tables;
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
} grn_select_data;

typedef void grn_select_output_slices_label_func(grn_ctx *ctx,
                                                 grn_select_data *data);
typedef void grn_select_output_slices_open_func(grn_ctx *ctx,
                                                grn_select_data *data,
                                                unsigned int n_result_sets);
typedef void grn_select_output_slices_close_func(grn_ctx *ctx,
                                                 grn_select_data *data);
typedef void grn_select_output_slice_label_func(grn_ctx *ctx,
                                                grn_select_data *data,
                                                grn_slice_data *slice);
typedef void grn_select_output_drilldowns_label_func(grn_ctx *ctx,
                                                     grn_select_data *data);
typedef void grn_select_output_drilldowns_open_func(grn_ctx *ctx,
                                                    grn_select_data *data,
                                                    unsigned int n_result_sets);
typedef void grn_select_output_drilldowns_close_func(grn_ctx *ctx,
                                                     grn_select_data *data);
typedef void grn_select_output_drilldown_label_func(grn_ctx *ctx,
                                                    grn_select_data *data,
                                                    grn_drilldown_data *drilldown);

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
  if (((calc_types_end - calc_types) >= (sizeof(#name) - 1)) &&\
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
grn_column_stage_name(grn_column_stage stage)
{
  switch (stage) {
  case GRN_COLUMN_STAGE_INITIAL :
    return "initial";
  case GRN_COLUMN_STAGE_RESULT_SET :
    return "result_set";
  case GRN_COLUMN_STAGE_FILTERED :
    return "filtered";
  case GRN_COLUMN_STAGE_OUTPUT :
    return "output";
  case GRN_COLUMN_STAGE_GROUP :
    return "group";
  default :
    return "unknown";
  }
}

static grn_bool
grn_column_data_init(grn_ctx *ctx,
                     const char *label,
                     size_t label_len,
                     grn_column_stage stage,
                     grn_hash **columns)
{
  void *column_raw;
  grn_column_data *column;
  int added;

  if (!*columns) {
    *columns = grn_hash_create(ctx,
                               NULL,
                               GRN_TABLE_MAX_KEY_SIZE,
                               sizeof(grn_column_data),
                               GRN_OBJ_TABLE_HASH_KEY |
                               GRN_OBJ_KEY_VAR_SIZE |
                               GRN_HASH_TINY);
  }
  if (!*columns) {
    return GRN_FALSE;
  }

  grn_hash_add(ctx,
               *columns,
               label,
               label_len,
               &column_raw,
               &added);
  if (!added) {
    return GRN_TRUE;
  }

  column = column_raw;
  column->label.value = label;
  column->label.length = label_len;
  column->stage = stage;
  column->type = grn_ctx_at(ctx, GRN_DB_TEXT);
  column->flags = GRN_OBJ_COLUMN_SCALAR;
  GRN_RAW_STRING_INIT(column->value);
  GRN_RAW_STRING_INIT(column->window.sort_keys);
  GRN_RAW_STRING_INIT(column->window.group_keys);
  GRN_TEXT_INIT(&(column->dependency_column_names), GRN_OBJ_VECTOR);

  return GRN_TRUE;
}

static void
grn_column_data_fin(grn_ctx *ctx,
                    grn_column_data *column)
{
  if (grn_enable_reference_count) {
    grn_obj_unlink(ctx, column->type);
  }

  GRN_OBJ_FIN(ctx, &(column->dependency_column_names));
}

static bool
grn_column_data_extract_dependency_column_names(grn_ctx *ctx,
                                                grn_column_data *column,
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
                             &(column->dependency_column_names),
                             keys->value + start,
                             current - start,
                             0,
                             GRN_DB_SHORT_TEXT);
      start = current + 1;
    }
  }

  if (start != current) {
    grn_vector_add_element(ctx,
                           &(column->dependency_column_names),
                           keys->value + start,
                           current - start,
                           0,
                           GRN_DB_SHORT_TEXT);
  }

  return true;
}

static grn_bool
grn_column_data_fill(grn_ctx *ctx,
                     grn_column_data *column,
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
                       grn_column_stage_name(column->stage),
                       (int)(column->label.length),
                       column->label.value,
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
                       grn_column_stage_name(column->stage),
                       (int)(column->label.length),
                       column->label.value,
                       (int)(GRN_TEXT_LEN(&inspected)),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      grn_obj_unlink(ctx, type);
      return GRN_FALSE;
    }
    if (column->type && grn_enable_reference_count) {
      grn_obj_unlink(ctx, column->type);
    }
    column->type = type;
  }

  if (flags && GRN_TEXT_LEN(flags) > 0) {
    char error_message_tag[GRN_TABLE_MAX_KEY_SIZE];

    grn_snprintf(error_message_tag,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[select][columns][%s][%.*s]",
                 grn_column_stage_name(column->stage),
                 (int)(column->label.length),
                 column->label.value);
    column->flags =
      grn_proc_column_parse_flags(ctx,
                                  error_message_tag,
                                  GRN_TEXT_VALUE(flags),
                                  GRN_TEXT_VALUE(flags) + GRN_TEXT_LEN(flags));
    if (ctx->rc != GRN_SUCCESS) {
      return GRN_FALSE;
    }
  }

  GRN_RAW_STRING_FILL(column->value, value);
  GRN_RAW_STRING_FILL(column->window.sort_keys, window_sort_keys);
  GRN_RAW_STRING_FILL(column->window.group_keys, window_group_keys);

  GRN_BULK_REWIND(&(column->dependency_column_names));
  if (!grn_column_data_extract_dependency_column_names(
        ctx, column, &(column->window.sort_keys))) {
    return false;
  }
  if (!grn_column_data_extract_dependency_column_names(
        ctx, column, &(column->window.group_keys))) {
    return false;
  }

  return GRN_TRUE;
}

static grn_bool
grn_column_data_collect(grn_ctx *ctx,
                        grn_user_data *user_data,
                        grn_hash *columns,
                        const char *prefix_label,
                        size_t prefix_label_len)
{
  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {
    grn_column_data *column;
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

    grn_hash_cursor_get_value(ctx, cursor, (void **)&column);

#define GET_VAR_RAW(parameter_key, name)                                \
    if (!name) {                                                        \
      grn_snprintf(key_name,                                            \
                   GRN_TABLE_MAX_KEY_SIZE,                              \
                   GRN_TABLE_MAX_KEY_SIZE,                              \
                   "%.*s%s[%.*s]." # name,                              \
                   (int)prefix_label_len,                               \
                   prefix_label,                                        \
                   parameter_key,                                       \
                   (int)(column->label.length),                         \
                   column->label.value);                                \
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

    grn_column_data_fill(ctx, column,
                         type, flags, value,
                         window.sort_keys,
                         window.group_keys);
  } GRN_HASH_EACH_END(ctx, cursor);

  return true;
}

static void
grn_columns_init(grn_ctx *ctx, grn_columns *columns)
{
  columns->initial = NULL;
  columns->result_set = NULL;
  columns->filtered = NULL;
  columns->output = NULL;
  columns->group = NULL;
}

static void
grn_columns_stage_fin(grn_ctx *ctx, grn_hash *columns_stage)
{
  GRN_HASH_EACH_BEGIN(ctx, columns_stage, cursor, id) {
    grn_column_data *column_data;
    grn_hash_cursor_get_value(ctx, cursor, (void **)&column_data);
    grn_column_data_fin(ctx, column_data);
  } GRN_HASH_EACH_END(ctx, cursor);
  grn_hash_close(ctx, columns_stage);
}

static void
grn_columns_fin(grn_ctx *ctx, grn_columns *columns)
{
  if (columns->initial) {
    grn_columns_stage_fin(ctx, columns->initial);
  }
  if (columns->result_set) {
    grn_columns_stage_fin(ctx, columns->result_set);
  }
  if (columns->filtered) {
    grn_columns_stage_fin(ctx, columns->filtered);
  }
  if (columns->output) {
    grn_columns_stage_fin(ctx, columns->output);
  }
  if (columns->group) {
    grn_columns_stage_fin(ctx, columns->group);
  }
}

static grn_bool
grn_columns_collect(grn_ctx *ctx,
                    grn_user_data *user_data,
                    grn_columns *columns,
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
    return GRN_FALSE;
  }

  prefix_len = strlen(prefix);
  suffix_len = strlen(suffix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    char *variable_name;
    int variable_name_len;
    char *column_name;
    size_t column_name_len;
    void *value_raw;
    grn_obj *value;
    grn_column_stage stage;
    grn_hash **target_columns;

    variable_name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    variable_name = key;
    if (variable_name_len < base_prefix_len + prefix_len + suffix_len + 1) {
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
               variable_name + (variable_name_len - suffix_len),
               suffix_len) != 0) {
      continue;
    }

    grn_table_cursor_get_value(ctx, cursor, &value_raw);
    value = value_raw;
    if (GRN_TEXT_EQUAL_CSTRING(value, "initial")) {
      stage = GRN_COLUMN_STAGE_INITIAL;
      target_columns = &(columns->initial);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "result_set")) {
      stage = GRN_COLUMN_STAGE_RESULT_SET;
      target_columns = &(columns->result_set);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "filtered")) {
      stage = GRN_COLUMN_STAGE_FILTERED;
      target_columns = &(columns->filtered);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "output")) {
      stage = GRN_COLUMN_STAGE_OUTPUT;
      target_columns = &(columns->output);
    } else if (GRN_TEXT_EQUAL_CSTRING(value, "group")) {
      stage = GRN_COLUMN_STAGE_GROUP;
      target_columns = &(columns->group);
    } else {
      continue;
    }

    column_name = variable_name + base_prefix_len + prefix_len;
    column_name_len =
      variable_name_len - base_prefix_len - prefix_len - suffix_len;
    if (!grn_column_data_init(ctx,
                              column_name,
                              column_name_len,
                              stage,
                              target_columns)) {
      grn_table_cursor_close(ctx, cursor);
      return GRN_FALSE;
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return GRN_TRUE;
}

static bool
grn_columns_fill(grn_ctx *ctx,
                 grn_user_data *user_data,
                 grn_columns *columns,
                 const char *prefix,
                 size_t prefix_length)
{
  if (!grn_columns_collect(ctx, user_data, columns,
                           "columns[", prefix, prefix_length)) {
    return false;
  }

  /* For backward compatibility */
  if (!grn_columns_collect(ctx, user_data, columns,
                           "column[", prefix, prefix_length)) {
    return false;
  }

  if (columns->initial) {
    if (!grn_column_data_collect(ctx,
                                 user_data,
                                 columns->initial,
                                 prefix,
                                 prefix_length)) {
      return false;
    }
  }

  if (columns->result_set) {
    if (!grn_column_data_collect(ctx,
                                 user_data,
                                 columns->result_set,
                                 prefix,
                                 prefix_length)) {
      return false;
    }
  }

  if (columns->filtered) {
    if (!grn_column_data_collect(ctx,
                                 user_data,
                                 columns->filtered,
                                 prefix,
                                 prefix_length)) {
      return false;
    }
  }

  if (columns->output) {
    if (!grn_column_data_collect(ctx,
                                 user_data,
                                 columns->output,
                                 prefix,
                                 prefix_length)) {
      return false;
    }
  }

  if (columns->group) {
    if (!grn_column_data_collect(ctx,
                                 user_data,
                                 columns->group,
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
columns_tsort_visit(grn_ctx *ctx,
                    grn_hash *columns,
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
      grn_column_data *column =
        (grn_column_data *)grn_hash_get_value_(ctx, columns, id, NULL);
      size_t i;
      size_t n_dependencies =
        grn_vector_size(ctx, &(column->dependency_column_names));
      for (i = 0; i < n_dependencies; i++) {
        const char *name;
        unsigned int name_length =
          grn_vector_get_element(ctx,
                                 &(column->dependency_column_names),
                                 i,
                                 &name,
                                 NULL,
                                 NULL);
        grn_id dependent_id;
        dependent_id = grn_hash_get(ctx,
                                    columns,
                                    name,
                                    name_length,
                                    NULL);
        if (dependent_id != GRN_ID_NIL) {
          cycled = columns_tsort_visit(ctx,
                                       columns,
                                       statuses,
                                       ids,
                                       dependent_id,
                                       log_tag_prefix);
          if (cycled) {
            GRN_PLUGIN_ERROR(ctx,
                             GRN_INVALID_ARGUMENT,
                             "%s[column][%.*s] cycled dependency: <%.*s>",
                             log_tag_prefix,
                             (int)(column->label.length),
                             column->label.value,
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
columns_tsort_body(grn_ctx *ctx,
                   grn_hash *columns,
                   tsort_status *statuses,
                   grn_obj *ids,
                   const char *log_tag_prefix)
{
  grn_bool succeeded = GRN_TRUE;

  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {
    if (columns_tsort_visit(ctx, columns, statuses, ids, id, log_tag_prefix)) {
      succeeded = true;
      break;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  return succeeded;
}

static void
columns_tsort_init(grn_ctx *ctx,
                   tsort_status *statuses,
                   size_t n_statuses)
{
  size_t i;
  for (i = 0; i < n_statuses; i++) {
    statuses[i] = TSORT_STATUS_NOT_VISITED;
  }
}

static bool
columns_tsort(grn_ctx *ctx,
              grn_hash *columns,
              grn_obj *ids,
              const char *log_tag_prefix)
{
  size_t n_statuses = grn_hash_size(ctx, columns);
  tsort_status *statuses = GRN_PLUGIN_MALLOCN(ctx, tsort_status, n_statuses);
  if (!statuses) {
    return false;
  }

  columns_tsort_init(ctx, statuses, n_statuses);
  bool succeeded = columns_tsort_body(ctx,
                                      columns,
                                      statuses,
                                      ids,
                                      log_tag_prefix);
  GRN_PLUGIN_FREE(ctx, statuses);
  return succeeded;
}

static bool
grn_select_apply_column(grn_ctx *ctx,
                        grn_select_data *data,
                        grn_obj *table,
                        grn_column_stage stage,
                        grn_column_data *column_data,
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
                  grn_column_stage_name(column_data->stage),
                  (int)(column_data->label.length),
                  column_data->label.value);
  grn_obj *column = grn_column_create(ctx,
                                      table,
                                      column_data->label.value,
                                      column_data->label.length,
                                      NULL,
                                      column_data->flags,
                                      column_data->type);
  if (!column) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%.*s failed to create column: %s",
                     (int)GRN_TEXT_LEN(&tag),
                     GRN_TEXT_VALUE(&tag),
                     ctx->errbuf);
    goto exit;
  }

  if (stage == GRN_COLUMN_STAGE_RESULT_SET) {
    succeeded = true;
    goto exit;
  }

  if (column_data->window.sort_keys.length > 0 ||
      column_data->window.group_keys.length > 0) {
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
                                              column_data->value.value,
                                              column_data->value.length);
    }
    if (ctx->rc == GRN_SUCCESS) {
      grn_window_function_executor_set_output_column_name(
        ctx,
        &executor,
        column_data->label.value,
        column_data->label.length);
    }
    if (ctx->rc == GRN_SUCCESS &&
        column_data->window.sort_keys.length > 0) {
      grn_window_function_executor_set_sort_keys(
        ctx,
        &executor,
        column_data->window.sort_keys.value,
        column_data->window.sort_keys.length);
    }
    if (ctx->rc == GRN_SUCCESS &&
        column_data->window.group_keys.length > 0) {
      grn_window_function_executor_set_group_keys(
        ctx,
        &executor,
        column_data->window.group_keys.value,
        column_data->window.group_keys.length);
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
                   column_data->value.value,
                   column_data->value.length,
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
                       (int)(column_data->value.length),
                       column_data->value.value,
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
grn_select_apply_columns(grn_ctx *ctx,
                         grn_select_data *data,
                         grn_obj *table,
                         grn_column_stage stage,
                         grn_hash *columns,
                         grn_obj *condition,
                         const char *log_tag_prefix,
                         const char *query_log_tag_prefix)
{
  grn_obj tsorted_ids;
  GRN_RECORD_INIT(&tsorted_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  if (!columns_tsort(ctx, columns, &tsorted_ids, log_tag_prefix)) {
    GRN_OBJ_FIN(ctx, &tsorted_ids);
    return;
  }

  size_t i;
  size_t n_columns = GRN_RECORD_VECTOR_SIZE(&tsorted_ids);
  for (i = 0; i < n_columns; i++) {
    grn_id id = GRN_RECORD_VALUE_AT(&tsorted_ids, i);
    grn_column_data *column_data =
      (grn_column_data *)grn_hash_get_value_(ctx, columns, id, NULL);
    if (!grn_select_apply_column(ctx,
                                 data,
                                 table,
                                 stage,
                                 column_data,
                                 condition,
                                 log_tag_prefix,
                                 query_log_tag_prefix)) {
      break;
    }

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "%scolumns[%.*s](%d)",
                  query_log_tag_prefix ? query_log_tag_prefix : "",
                  (int)(column_data->label.length),
                  column_data->label.value,
                  grn_table_size(ctx, table));
  }

  GRN_OBJ_FIN(ctx, &tsorted_ids);
}

static void
grn_filter_data_init(grn_ctx *ctx, grn_filter_data *data)
{
  GRN_RAW_STRING_INIT(data->match_columns);
  GRN_RAW_STRING_INIT(data->query);
  GRN_RAW_STRING_INIT(data->query_expander);
  GRN_RAW_STRING_INIT(data->query_flags);
  GRN_RAW_STRING_INIT(data->query_options);
  GRN_RAW_STRING_INIT(data->filter);
  GRN_RAW_STRING_INIT(data->post_filter);
  data->condition.match_columns = NULL;
  data->condition.expression = NULL;
  data->condition.query_options_expression = NULL;
  data->post_condition.expression = NULL;
  data->filtered = NULL;
  data->post_filtered = NULL;
}

static void
grn_filter_data_fin(grn_ctx *ctx, grn_filter_data *data)
{
  if (data->post_filtered) {
    grn_obj_unlink(ctx, data->post_filtered);
  }
  if (data->post_condition.expression) {
    grn_obj_close(ctx, data->post_condition.expression);
  }
  if (data->filtered) {
    grn_obj_unlink(ctx, data->filtered);
  }
  if (data->condition.expression) {
    grn_obj_close(ctx, data->condition.expression);
  }
  if (data->condition.match_columns) {
    grn_obj_close(ctx, data->condition.match_columns);
  }
  if (data->condition.query_options_expression) {
    grn_obj_close(ctx, data->condition.query_options_expression);
  }
}

static void
grn_filter_data_fill(grn_ctx *ctx,
                     grn_filter_data *data,
                     grn_obj *match_columns,
                     grn_obj *query,
                     grn_obj *query_expander,
                     grn_obj *query_flags,
                     grn_obj *query_options,
                     grn_obj *filter,
                     grn_obj *post_filter)
{
  GRN_RAW_STRING_FILL(data->match_columns, match_columns);
  GRN_RAW_STRING_FILL(data->query, query);
  GRN_RAW_STRING_FILL(data->query_expander, query_expander);
  GRN_RAW_STRING_FILL(data->query_flags, query_flags);
  GRN_RAW_STRING_FILL(data->query_options, query_options);
  GRN_RAW_STRING_FILL(data->filter, filter);
  GRN_RAW_STRING_FILL(data->post_filter, post_filter);
}

static bool
grn_filter_data_execute(grn_ctx *ctx,
                        grn_select_data *data,
                        grn_filter_data *filter_data,
                        grn_obj *table,
                        grn_columns *columns,
                        const char *log_tag_prefix,
                        const char *query_log_tag_prefix)
{
  grn_obj *variable;

  if (filter_data->query.length == 0 && filter_data->filter.length == 0) {
    return true;
  }

  GRN_EXPR_CREATE_FOR_QUERY(ctx,
                            table,
                            filter_data->condition.expression,
                            variable);
  if (!filter_data->condition.expression) {
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

  if (filter_data->query.length > 0) {
    if (filter_data->match_columns.length > 0) {
      GRN_EXPR_CREATE_FOR_QUERY(ctx,
                                table,
                                filter_data->condition.match_columns,
                                variable);
      if (!filter_data->condition.match_columns) {
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
                         (int)(filter_data->match_columns.length),
                         filter_data->match_columns.value,
                         ctx->errbuf);
        return false;
      }

      grn_expr_parse(ctx,
                     filter_data->condition.match_columns,
                     filter_data->match_columns.value,
                     filter_data->match_columns.length,
                     NULL, GRN_OP_MATCH, GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT);
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }

    {
      grn_expr_flags flags;
      grn_obj query_expander_buf;
      const char *query = filter_data->query.value;
      unsigned int query_len = filter_data->query.length;

      flags = GRN_EXPR_SYNTAX_QUERY;
      if (filter_data->query_flags.length) {
        grn_obj query_flags;
        GRN_TEXT_INIT(&query_flags, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET(ctx,
                     &query_flags,
                     filter_data->query_flags.value,
                     filter_data->query_flags.length);
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
      if (filter_data->query_expander.length > 0) {
        grn_rc rc;
        rc = grn_proc_syntax_expand_query(ctx,
                                          filter_data->query.value,
                                          filter_data->query.length,
                                          flags,
                                          filter_data->query_expander.value,
                                          filter_data->query_expander.length,
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
                     filter_data->condition.expression,
                     query,
                     query_len,
                     filter_data->condition.match_columns,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     flags);
      GRN_OBJ_FIN(ctx, &query_expander_buf);

      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }
  }

  if (filter_data->filter.length > 0) {
    grn_expr_parse(ctx,
                   filter_data->condition.expression,
                   filter_data->filter.value,
                   filter_data->filter.length,
                   filter_data->condition.match_columns,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }

    if (filter_data->query.length > 0) {
      grn_expr_append_op(ctx, filter_data->condition.expression, GRN_OP_AND, 2);
    }

    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
  }

  grn_expr_set_query_log_tag_prefix(ctx,
                                    filter_data->condition.expression,
                                    query_log_tag_prefix,
                                    -1);
  filter_data->filtered =
    grn_table_create(ctx,
                     NULL, 0,
                     NULL,
                     GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                     table,
                     NULL);
  if (!filter_data->filtered) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s[filter] failed to create result set table: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return false;
  }

  if (columns->result_set) {
    grn_select_apply_columns(ctx,
                             data,
                             filter_data->filtered,
                             GRN_COLUMN_STAGE_RESULT_SET,
                             columns->result_set,
                             filter_data->condition.expression,
                             log_tag_prefix,
                             query_log_tag_prefix);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
  }

  if (filter_data->query_options.length > 0) {
    GRN_EXPR_CREATE_FOR_QUERY(ctx,
                              table,
                              filter_data->condition.query_options_expression,
                              variable);
    if (!filter_data->condition.expression) {
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
                   filter_data->condition.query_options_expression,
                   filter_data->query_options.value,
                   filter_data->query_options.length,
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_OPTIONS);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
    grn_expr *query_options_expression =
      (grn_expr *)(filter_data->condition.query_options_expression);
    grn_expr_set_query_options(ctx,
                               filter_data->condition.expression,
                               query_options_expression->codes[0].value);
  }

  grn_table_select(ctx,
                   table,
                   filter_data->condition.expression,
                   filter_data->filtered,
                   GRN_OP_OR);

  return ctx->rc == GRN_SUCCESS;
}

static bool
grn_filter_data_execute_post_filter(grn_ctx *ctx,
                                    grn_select_data *data,
                                    grn_filter_data *filter_data,
                                    grn_obj *table,
                                    const char *log_tag_prefix,
                                    const char *query_log_tag_prefix)
{
  if (filter_data->post_filter.length == 0) {
    return true;
  }

  grn_obj *variable;
  GRN_EXPR_CREATE_FOR_QUERY(ctx,
                            table,
                            filter_data->post_condition.expression,
                            variable);
  if (!filter_data->post_condition.expression) {
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
                 filter_data->post_condition.expression,
                 filter_data->post_filter.value,
                 filter_data->post_filter.length,
                 NULL,
                 GRN_OP_MATCH,
                 GRN_OP_AND,
                 GRN_EXPR_SYNTAX_SCRIPT);
  if (ctx->rc != GRN_SUCCESS) {
    return false;
  }

  grn_expr_set_query_log_tag_prefix(ctx,
                                    filter_data->post_condition.expression,
                                    query_log_tag_prefix,
                                    -1);
  filter_data->post_filtered =
    grn_table_create(ctx,
                     NULL, 0,
                     NULL,
                     GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                     table,
                     NULL);
  if (!filter_data->post_filtered) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s[post-filter] failed to create result set table: %s",
                     log_tag_prefix,
                     ctx->errbuf);
    return false;
  }

  grn_table_select(ctx,
                   table,
                   filter_data->post_condition.expression,
                   filter_data->post_filtered,
                   GRN_OP_OR);

  return ctx->rc == GRN_SUCCESS;
}

static void
grn_drilldown_data_init(grn_ctx *ctx,
                        grn_drilldown_data *drilldown,
                        const char *label,
                        size_t label_len)
{
  drilldown->label.value = label;
  drilldown->label.length = label_len;
  GRN_RAW_STRING_INIT(drilldown->keys);
  drilldown->parsed_keys = NULL;
  drilldown->n_parsed_keys = 0;
  GRN_RAW_STRING_INIT(drilldown->sort_keys);
  GRN_RAW_STRING_INIT(drilldown->output_columns);
  drilldown->offset = 0;
  drilldown->limit = DEFAULT_DRILLDOWN_LIMIT;
  drilldown->calc_types = 0;
  GRN_RAW_STRING_INIT(drilldown->calc_target_name);
  GRN_RAW_STRING_INIT(drilldown->filter);
  GRN_RAW_STRING_INIT(drilldown->adjuster);
  GRN_RAW_STRING_INIT(drilldown->table_name);
  grn_columns_init(ctx, &(drilldown->columns));
  drilldown->result.table = NULL;
  drilldown->filtered_result = NULL;
}

static void
grn_drilldown_data_fin(grn_ctx *ctx, grn_drilldown_data *drilldown)
{
  grn_table_group_result *result;

  grn_columns_fin(ctx, &(drilldown->columns));

  if (drilldown->filtered_result) {
    grn_obj_close(ctx, drilldown->filtered_result);
  }

  result = &(drilldown->result);
  if (result->table) {
    if (result->calc_target) {
      grn_obj_unlink(ctx, result->calc_target);
    }
    if (result->table) {
      grn_obj_unlink(ctx, result->table);
    }
    if (result->n_aggregators > 0) {
      uint32_t i;
      for (i = 0; i < result->n_aggregators; i++) {
        grn_table_group_aggregator *aggregator = result->aggregators[i];
        grn_table_group_aggregator_close(ctx, aggregator);
      }
      GRN_FREE(result->aggregators);
    }
  }
}

static void
grn_drilldowns_fin(grn_ctx *ctx, grn_hash *drilldowns)
{
  GRN_HASH_EACH_BEGIN(ctx, drilldowns, cursor, id) {
    grn_drilldown_data *drilldown;
    grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
    grn_drilldown_data_fin(ctx, drilldown);
  } GRN_HASH_EACH_END(ctx, cursor);
  grn_hash_close(ctx, drilldowns);
}

static void
grn_drilldown_data_fill(grn_ctx *ctx,
                        grn_drilldown_data *drilldown,
                        grn_obj *keys,
                        grn_obj *sort_keys,
                        grn_obj *output_columns,
                        grn_obj *offset,
                        grn_obj *limit,
                        grn_obj *calc_types,
                        grn_obj *calc_target,
                        grn_obj *filter,
                        grn_obj *adjuster,
                        grn_obj *table)
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
}

static void
grn_slice_data_init(grn_ctx *ctx,
                    grn_slice_data *slice,
                    const char *label,
                    size_t label_len)
{
  slice->label.value = label;
  slice->label.length = label_len;
  grn_filter_data_init(ctx, &(slice->filter));
  GRN_RAW_STRING_INIT(slice->sort_keys);
  GRN_RAW_STRING_INIT(slice->output_columns);
  slice->offset = 0;
  slice->limit = GRN_SELECT_DEFAULT_LIMIT;
  slice->tables.target = NULL;
  slice->tables.initial = NULL;
  slice->tables.result = NULL;
  slice->tables.sorted = NULL;
  slice->tables.output = NULL;
  slice->drilldowns = NULL;
  grn_columns_init(ctx, &(slice->columns));
}

static void
grn_slice_data_fin(grn_ctx *ctx, grn_slice_data *slice)
{
  grn_filter_data_fin(ctx, &(slice->filter));
  if (slice->drilldowns) {
    grn_drilldowns_fin(ctx, slice->drilldowns);
  }
  grn_columns_fin(ctx, &(slice->columns));
  if (slice->tables.sorted) {
    grn_obj_unlink(ctx, slice->tables.sorted);
  }
  if (slice->tables.initial) {
    grn_obj_unlink(ctx, slice->tables.initial);
  }
}

static void
grn_slices_fin(grn_ctx *ctx, grn_hash *slices)
{
  GRN_HASH_EACH_BEGIN(ctx, slices, cursor, id) {
    grn_slice_data *slice;
    grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);
    grn_slice_data_fin(ctx, slice);
  } GRN_HASH_EACH_END(ctx, cursor);
  grn_hash_close(ctx, slices);
}

static void
grn_slice_data_fill(grn_ctx *ctx,
                    grn_slice_data *slice,
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
  grn_filter_data_fill(ctx,
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
      if (((end - current) >= (sizeof(#name) - 1)) &&                   \
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
                            int columns_len,
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
                                    int columns_len,
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
                               int columns_len,
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
  if (!data->columns.initial) {
    return GRN_TRUE;
  }

  data->tables.initial =
    grn_select_create_all_selected_result_table(ctx, data->tables.target);
  if (!data->tables.initial) {
    return GRN_FALSE;
  }

  grn_select_apply_columns(ctx,
                           data,
                           data->tables.initial,
                           GRN_COLUMN_STAGE_INITIAL,
                           data->columns.initial,
                           data->filter.condition.expression,
                           "[select]",
                           NULL);

  return ctx->rc == GRN_SUCCESS;
}

static grn_bool
grn_select_filter(grn_ctx *ctx,
                  grn_select_data *data)
{
  if (!grn_filter_data_execute(ctx,
                               data,
                               &(data->filter),
                               data->tables.initial,
                               &(data->columns),
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
grn_select_apply_filtered_columns(grn_ctx *ctx,
                                  grn_select_data *data)
{
  if (!data->columns.filtered) {
    return GRN_TRUE;
  }

  if (data->tables.result == data->tables.initial) {
    data->tables.result =
      grn_select_create_all_selected_result_table(ctx, data->tables.initial);
    if (!data->tables.result) {
      return GRN_FALSE;
    }
  }

  grn_select_apply_columns(ctx,
                           data,
                           data->tables.result,
                           GRN_COLUMN_STAGE_FILTERED,
                           data->columns.filtered,
                           data->filter.condition.expression,
                           "[select]",
                           NULL);

  return ctx->rc == GRN_SUCCESS;
}

static bool
grn_select_post_filter(grn_ctx *ctx,
                       grn_select_data *data)
{
  if (!grn_filter_data_execute_post_filter(ctx,
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
grn_select_apply_output_columns(grn_ctx *ctx,
                                grn_select_data *data)
{
  if (!data->columns.output) {
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

  grn_select_apply_columns(ctx,
                           data,
                           data->tables.sorted,
                           GRN_COLUMN_STAGE_OUTPUT,
                           data->columns.output,
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
  unsigned int n_keys = 0;
  grn_obj *target_table = table;
  grn_drilldown_data *drilldown;
  grn_table_group_result *result;
  grn_obj log_tag_prefix;
  grn_obj full_query_log_tag_prefix;

  drilldown =
    (grn_drilldown_data *)grn_hash_get_value_(ctx, drilldowns, id, NULL);

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

  result = &(drilldown->result);
  result->limit = 1;
  result->flags = GRN_TABLE_GROUP_CALC_COUNT;
  result->op = 0;
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
        grn_slice_data *slice;
        dependent_id = grn_hash_get(ctx,
                                    slices,
                                    drilldown->table_name.value,
                                    drilldown->table_name.length,
                                    NULL);
        if (dependent_id) {
          slice =
            (grn_slice_data *)grn_hash_get_value_(ctx,
                                                  slices,
                                                  dependent_id,
                                                  NULL);
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
      grn_drilldown_data *dependent_drilldown;
      grn_table_group_result *dependent_result;

      dependent_drilldown =
        (grn_drilldown_data *)grn_hash_get_value_(ctx,
                                                  drilldowns,
                                                  dependent_id,
                                                  NULL);
      dependent_result = &(dependent_drilldown->result);
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

  if (drilldown->columns.group) {
    result->n_aggregators = grn_hash_size(ctx, drilldown->columns.group);
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
      GRN_HASH_EACH_BEGIN(ctx, drilldown->columns.group, cursor, id) {
        void *value;
        grn_hash_cursor_get_value(ctx, cursor, &value);
        grn_column_data *column_data = value;
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
        grn_table_group_aggregator_set_output_column_name(ctx,
                                                          result->aggregators[i],
                                                          column_data->label.value,
                                                          column_data->label.length);
        grn_table_group_aggregator_set_output_column_type(ctx,
                                                          result->aggregators[i],
                                                          column_data->type);
        grn_table_group_aggregator_set_output_column_flags(ctx,
                                                           result->aggregators[i],
                                                           column_data->flags);
        grn_table_group_aggregator_set_expression(ctx,
                                                  result->aggregators[i],
                                                  column_data->value.value,
                                                  column_data->value.length);
        i++;
      } GRN_HASH_EACH_END(ctx, cursor);
      if (!success) {
        goto exit;
      }
      result->flags |= GRN_TABLE_GROUP_CALC_AGGREGATOR;
    }
  }

  if (drilldown->parsed_keys) {
    grn_table_group(ctx,
                    target_table,
                    drilldown->parsed_keys,
                    drilldown->n_parsed_keys,
                    result,
                    1);
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
    grn_table_group(ctx, target_table, keys, n_keys, result, 1);
  }

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

  if (drilldown->columns.initial) {
    grn_select_apply_columns(ctx,
                             data,
                             result->table,
                             GRN_COLUMN_STAGE_INITIAL,
                             drilldown->columns.initial,
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

  if (drilldown->columns.filtered) {
    grn_select_apply_columns(ctx,
                             data,
                             drilldown->filtered_result,
                             GRN_COLUMN_STAGE_FILTERED,
                             drilldown->columns.filtered,
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
      grn_drilldown_data *drilldown;
      drilldown =
        (grn_drilldown_data *)grn_hash_get_value_(ctx, drilldowns, id, NULL);
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

static grn_drilldown_data *
grn_select_drilldowns_add(grn_ctx *ctx,
                          grn_hash **drilldowns,
                          const char *label,
                          size_t label_len,
                          const char *log_tag)
{
  grn_drilldown_data *drilldown = NULL;
  int added;

  if (!*drilldowns) {
    *drilldowns = grn_hash_create(ctx,
                                  NULL,
                                  GRN_TABLE_MAX_KEY_SIZE,
                                  sizeof(grn_drilldown_data),
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
    grn_drilldown_data_init(ctx, drilldown, label, label_len);
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
        grn_drilldown_data *drilldown;

        GRN_BULK_REWIND(&buffer);
        grn_text_printf(ctx, &buffer, "drilldown%d", i);
        drilldown = grn_select_drilldowns_add(ctx,
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
      grn_drilldown_data *drilldown;
      grn_table_group_result *result;

      grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
      result = &(drilldown->result);
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
    grn_drilldown_data *drilldown;
    grn_table_group_result *result;

    grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
    result = &(drilldown->result);
    if (result->table) {
      n_available_results++;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  data->output.formatter->drilldowns_open(ctx, data, n_available_results);

  GRN_HASH_EACH_BEGIN(ctx, drilldowns, cursor, id) {
    grn_drilldown_data *drilldown;
    grn_table_group_result *result;
    grn_obj *target_table;
    uint32_t n_hits;
    int offset;
    int limit;

    grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
    result = &(drilldown->result);

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
      uint32_t n_sort_keys;
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

          if (drilldown->columns.output) {
            grn_select_apply_columns(ctx,
                                     data,
                                     sorted,
                                     GRN_COLUMN_STAGE_OUTPUT,
                                     drilldown->columns.output,
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
      if (drilldown->columns.output) {
        sorted =
          grn_select_create_no_sort_keys_sorted_table(ctx,
                                                      data,
                                                      target_table);
        if (!sorted) {
          succeeded = false;
        } else {
          grn_select_apply_columns(ctx,
                                   data,
                                   sorted,
                                   GRN_COLUMN_STAGE_OUTPUT,
                                   drilldown->columns.output,
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
                         grn_slice_data *slice)
{
  char log_tag[GRN_TABLE_MAX_KEY_SIZE];
  grn_filter_data *filter;

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
  if (slice->columns.initial) {
    slice->tables.initial =
      grn_select_create_all_selected_result_table(ctx, slice->tables.target);
    if (!slice->tables.initial) {
      return false;
    }
    grn_select_apply_columns(ctx,
                             data,
                             slice->tables.initial,
                             GRN_COLUMN_STAGE_INITIAL,
                             slice->columns.initial,
                             filter->condition.expression,
                             log_tag,
                             query_log_tag_prefix);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
    slice->tables.target = slice->tables.initial;
  }

  if (!grn_filter_data_execute(ctx,
                               data,
                               filter,
                               slice->tables.target,
                               &(slice->columns),
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

  if (slice->columns.filtered) {
    grn_select_apply_columns(ctx,
                             data,
                             slice->tables.result,
                             GRN_COLUMN_STAGE_FILTERED,
                             slice->columns.filtered,
                             slice->filter.condition.expression,
                             log_tag,
                             query_log_tag_prefix);
    if (ctx->rc != GRN_SUCCESS) {
      return false;
    }
  }

  if (!grn_filter_data_execute_post_filter(ctx,
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
    grn_slice_data *slice;

    grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);
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
    grn_slice_data *slice;

    grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);
    if (slice->tables.result) {
      n_available_results++;
    }
  } GRN_HASH_EACH_END(ctx, cursor);

  data->output.formatter->slices_open(ctx, data, n_available_results);

  GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
    grn_slice_data *slice;
    uint32_t n_hits;
    int offset;
    int limit;

    grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);
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
      uint32_t n_sort_keys;
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

    if (slice->columns.output) {
      grn_select_apply_columns(ctx,
                               data,
                               slice->tables.output,
                               GRN_COLUMN_STAGE_OUTPUT,
                               slice->columns.output,
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
                                 grn_slice_data *slice)
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
                                     grn_drilldown_data *drilldown)
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
                                 grn_slice_data *slice)
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
                                     grn_drilldown_data *drilldown)
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

static grn_rc
grn_select(grn_ctx *ctx, grn_select_data *data)
{
  uint32_t nhits;
  grn_obj *outbuf = ctx->impl->output.buf;
  grn_content_type output_type = ctx->impl->output.type;
  char cache_key[GRN_CACHE_MAX_KEY_SIZE];
  uint32_t cache_key_size;
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

  cache_key_size =
    data->table.length + 1 +
    data->filter.match_columns.length + 1 +
    data->filter.query.length + 1 +
    data->filter.filter.length + 1 +
    data->scorer.length + 1 +
    data->sort_keys.length + 1 +
    data->output_columns.length + 1 +
    data->match_escalation_threshold.length + 1 +
    data->filter.query_expander.length + 1 +
    data->filter.query_flags.length + 1 +
    data->filter.query_options.length + 1 +
    data->filter.post_filter.length + 1 +
    data->adjuster.length + 1 +
    data->match_escalation.length + 1 +
    sizeof(grn_content_type) +
    sizeof(int) * 2 +
    sizeof(grn_command_version) +
    sizeof(grn_bool);
  if (data->slices) {
    GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
      grn_slice_data *slice;
      grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);
      grn_raw_string_lstrip(ctx, &(slice->filter.query));
      cache_key_size +=
        slice->filter.match_columns.length + 1 +
        slice->filter.query.length + 1 +
        slice->filter.query_expander.length + 1 +
        slice->filter.query_flags.length + 1 +
        slice->filter.query_options.length + 1 +
        slice->filter.filter.length + 1 +
        slice->filter.post_filter.length + 1 +
        slice->sort_keys.length + 1 +
        slice->output_columns.length + 1 +
        slice->label.length + 1 +
        sizeof(int) * 2;
    } GRN_HASH_EACH_END(ctx, cursor);
  }
#define ADD_COLUMNS_CACHE_SIZE(columns) do {                     \
    GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {              \
      grn_column_data *column;                                   \
      grn_hash_cursor_get_value(ctx, cursor, (void **)&column);  \
      cache_key_size += column->label.length + 1;                \
      cache_key_size += sizeof(grn_column_stage);                \
      cache_key_size += sizeof(grn_id);                          \
      cache_key_size += sizeof(grn_column_flags);                \
      cache_key_size += column->value.length + 1;                \
      cache_key_size += column->window.sort_keys.length + 1;     \
      cache_key_size += column->window.group_keys.length + 1;    \
    } GRN_HASH_EACH_END(ctx, cursor);                            \
  } while (false)
  if (data->columns.initial) {
    ADD_COLUMNS_CACHE_SIZE(data->columns.initial);
  }
  if (data->columns.filtered) {
    ADD_COLUMNS_CACHE_SIZE(data->columns.filtered);
  }
  if (data->columns.output) {
    ADD_COLUMNS_CACHE_SIZE(data->columns.output);
  }
#undef ADD_COLUMNS_CACHE_SIZE

#define DRILLDOWN_CACHE_SIZE(drilldown)         \
  drilldown->keys.length + 1 +                  \
    drilldown->sort_keys.length + 1 +           \
    drilldown->output_columns.length + 1 +      \
    drilldown->label.length + 1 +               \
    drilldown->calc_target_name.length + 1 +    \
    drilldown->filter.length + 1 +              \
    drilldown->adjuster.length + 1 +            \
    drilldown->table_name.length + 1 +          \
    sizeof(int) * 2 +                           \
    sizeof(grn_table_group_flags)
  if (data->drilldown.keys.length > 0) {
    grn_drilldown_data *drilldown = &(data->drilldown);
    cache_key_size += DRILLDOWN_CACHE_SIZE(drilldown);
  }
  if (data->drilldowns) {
    GRN_HASH_EACH_BEGIN(ctx, data->drilldowns, cursor, id) {
      grn_drilldown_data *drilldown;
      grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
      cache_key_size += DRILLDOWN_CACHE_SIZE(drilldown);
    } GRN_HASH_EACH_END(ctx, cursor);
  }
#undef DRILLDOWN_CACHE_SIZE
  if (cache_key_size <= GRN_CACHE_MAX_KEY_SIZE) {
    char *cp = cache_key;

#define PUT_CACHE_KEY(string)                                   \
    grn_memcpy(cp, (string).value, (string).length);            \
    cp += (string).length;                                      \
    *cp++ = '\0'

    PUT_CACHE_KEY(data->table);
    PUT_CACHE_KEY(data->filter.match_columns);
    PUT_CACHE_KEY(data->filter.query);
    PUT_CACHE_KEY(data->filter.filter);
    PUT_CACHE_KEY(data->filter.post_filter);
    PUT_CACHE_KEY(data->scorer);
    PUT_CACHE_KEY(data->sort_keys);
    PUT_CACHE_KEY(data->output_columns);
#define PUT_CACHE_KEY_DRILLDOWN(drilldown) do {                 \
      PUT_CACHE_KEY(drilldown->keys);                           \
      PUT_CACHE_KEY(drilldown->sort_keys);                      \
      PUT_CACHE_KEY(drilldown->output_columns);                 \
      PUT_CACHE_KEY(drilldown->label);                          \
      PUT_CACHE_KEY(drilldown->calc_target_name);               \
      PUT_CACHE_KEY(drilldown->filter);                         \
      PUT_CACHE_KEY(drilldown->adjuster);                       \
      PUT_CACHE_KEY(drilldown->table_name);                     \
      grn_memcpy(cp, &(drilldown->offset), sizeof(int));        \
      cp += sizeof(int);                                        \
      grn_memcpy(cp, &(drilldown->limit), sizeof(int));         \
      cp += sizeof(int);                                        \
      grn_memcpy(cp,                                            \
                 &(drilldown->calc_types),                      \
                 sizeof(grn_table_group_flags));                \
      cp += sizeof(grn_table_group_flags);                      \
    } while (false)
    if (data->slices) {
      GRN_HASH_EACH_BEGIN(ctx, data->slices, cursor, id) {
        grn_slice_data *slice;
        grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);
        PUT_CACHE_KEY(slice->filter.match_columns);
        PUT_CACHE_KEY(slice->filter.query);
        PUT_CACHE_KEY(slice->filter.query_expander);
        PUT_CACHE_KEY(slice->filter.query_flags);
        PUT_CACHE_KEY(slice->filter.filter);
        PUT_CACHE_KEY(slice->filter.post_filter);
        PUT_CACHE_KEY(slice->sort_keys);
        PUT_CACHE_KEY(slice->output_columns);
        PUT_CACHE_KEY(slice->label);
        grn_memcpy(cp, &(slice->offset), sizeof(int));
        cp += sizeof(int);
        grn_memcpy(cp, &(slice->limit), sizeof(int));
        cp += sizeof(int);
        if (slice->drilldowns) {
          GRN_HASH_EACH_BEGIN(ctx, slice->drilldowns, cursor, id) {
            grn_drilldown_data *drilldown;
            grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
            PUT_CACHE_KEY_DRILLDOWN(drilldown);
          } GRN_HASH_EACH_END(ctx, cursor);
        }
      } GRN_HASH_EACH_END(ctx, cursor);
    }
#define PUT_CACHE_KEY_COLUMNS(columns) do {                          \
      GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {                \
        grn_column_data *column;                                     \
        grn_hash_cursor_get_value(ctx, cursor, (void **)&column);    \
        PUT_CACHE_KEY(column->label);                                \
        grn_memcpy(cp, &(column->stage), sizeof(grn_column_stage));  \
        cp += sizeof(grn_column_stage);                              \
        grn_memcpy(cp, &(DB_OBJ(column->type)->id), sizeof(grn_id)); \
        cp += sizeof(grn_id);                                        \
        grn_memcpy(cp, &(column->flags), sizeof(grn_column_flags));  \
        cp += sizeof(grn_column_flags);                              \
        PUT_CACHE_KEY(column->value);                                \
        PUT_CACHE_KEY(column->window.sort_keys);                     \
        PUT_CACHE_KEY(column->window.group_keys);                    \
      } GRN_HASH_EACH_END(ctx, cursor);                              \
    } while (false)
    if (data->columns.initial) {
      PUT_CACHE_KEY_COLUMNS(data->columns.initial);
    }
    if (data->columns.filtered) {
      PUT_CACHE_KEY_COLUMNS(data->columns.filtered);
    }
    if (data->columns.output) {
      PUT_CACHE_KEY_COLUMNS(data->columns.output);
    }
#undef PUT_CACHE_KEY_COLUMNS
    if (data->drilldown.keys.length > 0) {
      grn_drilldown_data *drilldown = &(data->drilldown);
      PUT_CACHE_KEY_DRILLDOWN(drilldown);
    }
    if (data->drilldowns) {
      GRN_HASH_EACH_BEGIN(ctx, data->drilldowns, cursor, id) {
        grn_drilldown_data *drilldown;
        grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);
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
    grn_memcpy(cp, &output_type, sizeof(grn_content_type));
    cp += sizeof(grn_content_type);
    grn_memcpy(cp, &(data->offset), sizeof(int));
    cp += sizeof(int);
    grn_memcpy(cp, &(data->limit), sizeof(int));
    cp += sizeof(int);
    grn_memcpy(cp, &(ctx->impl->command.version), sizeof(grn_command_version));
    cp += sizeof(grn_command_version);
    grn_memcpy(cp, &(ctx->impl->output.is_pretty), sizeof(grn_bool));
    cp += sizeof(grn_bool);
#undef PUT_CACHE_KEY

    {
      grn_rc rc;
      rc = grn_cache_fetch(ctx, cache_obj, cache_key, cache_key_size, outbuf);
      if (rc == GRN_SUCCESS) {
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_CACHE,
                      ":", "cache(%" GRN_FMT_LLD ")",
                      (long long int)GRN_TEXT_LEN(outbuf));
        return ctx->rc;
      }
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

    if (!grn_select_apply_filtered_columns(ctx, data)) {
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

      if (!grn_select_apply_output_columns(ctx, data)) {
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
        cache_key_size <= GRN_CACHE_MAX_KEY_SIZE &&
        (!data->cache.value ||
         data->cache.length != 2 ||
         data->cache.value[0] != 'n' ||
         data->cache.value[1] != 'o')) {
      grn_cache_update(ctx, cache_obj, cache_key, cache_key_size, outbuf);
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

static grn_bool
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
    return GRN_FALSE;
  }

  prefix_len = strlen(prefix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    char *name;
    int name_len;
    name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    name = key;
    if (name_len > prefix_len + 2 &&
        strncmp(prefix, name, prefix_len) == 0 &&
        name[prefix_len] == '[') {
      const char *label_end;
      size_t label_len;
      label_end = memchr(name + prefix_len + 2,
                         ']',
                         name_len - prefix_len - 2);
      if (!label_end) {
        continue;
      }
      label_len = (label_end - name) - prefix_len - 1;
      grn_select_drilldowns_add(ctx,
                                drilldowns,
                                name + prefix_len + 1,
                                label_len,
                                log_tag);
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return GRN_TRUE;
}

static grn_bool
grn_select_fill_drilldown_columns(grn_ctx *ctx,
                                  grn_user_data *user_data,
                                  grn_drilldown_data *drilldown,
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
  return grn_columns_fill(ctx,
                          user_data,
                          &(drilldown->columns),
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
    grn_drilldown_data *drilldown;
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

    grn_hash_cursor_get_value(ctx, cursor, (void **)&drilldown);

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
                            table);
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
    grn_drilldown_data_fill(ctx,
                            &(data->drilldown),
                            drilldown,
                            sort_keys,
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_output_columns",
                                                    -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_offset", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_limit", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_calc_types", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_calc_target", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_filter", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_adjuster", -1),
                            NULL);
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

static grn_slice_data *
grn_select_data_slices_add(grn_ctx *ctx,
                           grn_select_data *data,
                           const char *label,
                           size_t label_len)
{
  grn_slice_data *slice = NULL;
  int added;

  if (!data->slices) {
    data->slices = grn_hash_create(ctx,
                                   NULL,
                                   GRN_TABLE_MAX_KEY_SIZE,
                                   sizeof(grn_slice_data),
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
               (void **)&slice,
               &added);
  if (added) {
    grn_slice_data_init(ctx, slice, label, label_len);
  }

  return slice;
}

static grn_bool
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
    return GRN_FALSE;
  }

  prefix_len = strlen(prefix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    char *name;
    int name_len;
    name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    name = key;
    if (name_len > prefix_len + 1 &&
        strncmp(prefix, name, prefix_len) == 0) {
      const char *label_end;
      size_t label_len;
      label_end = memchr(name + prefix_len + 1,
                         ']',
                         name_len - prefix_len - 1);
      if (!label_end) {
        continue;
      }
      label_len = (label_end - name) - prefix_len;
      grn_select_data_slices_add(ctx,
                                 data,
                                 name + prefix_len,
                                 label_len);
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return GRN_TRUE;
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
    grn_slice_data *slice;
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

    grn_hash_cursor_get_value(ctx, cursor, (void **)&slice);

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

    grn_slice_data_fill(ctx,
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

    success = grn_columns_fill(ctx,
                               user_data,
                               &(slice->columns),
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
  grn_select_data data;

  grn_columns_init(ctx, &(data.columns));
  grn_filter_data_init(ctx, &(data.filter));

  data.tables.target = NULL;
  data.tables.initial = NULL;
  data.tables.result = NULL;
  data.tables.sorted = NULL;

  data.slices = NULL;
  grn_drilldown_data_init(ctx, &(data.drilldown), NULL, 0);
  data.drilldowns = NULL;

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

    grn_filter_data_fill(ctx,
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

  if (!grn_columns_fill(ctx, user_data, &(data.columns), NULL, 0)) {
    goto exit;
  }

  grn_select(ctx, &data);

exit :
  if (data.drilldowns) {
    grn_drilldowns_fin(ctx, data.drilldowns);
  }

  if (data.drilldown.parsed_keys) {
    grn_table_sort_key_close(ctx,
                             data.drilldown.parsed_keys,
                             data.drilldown.n_parsed_keys);
  }
  grn_drilldown_data_fin(ctx, &(data.drilldown));

  if (data.slices) {
    grn_slices_fin(ctx, data.slices);
  }

  if (data.tables.sorted) {
    grn_obj_unlink(ctx, data.tables.sorted);
  }

  if (data.tables.result == data.filter.post_filtered) {
    data.tables.result = NULL;
  }
  if (data.tables.result == data.filter.filtered) {
    data.tables.result = NULL;
  }
  grn_filter_data_fin(ctx, &(data.filter));

  if (data.tables.result &&
      data.tables.result != data.tables.initial &&
      data.tables.result != data.tables.target) {
    grn_obj_unlink(ctx, data.tables.result);
  }

  if (data.tables.initial && data.tables.initial != data.tables.target) {
    grn_obj_unlink(ctx, data.tables.initial);
  }

  if (data.tables.target) {
    grn_obj_unlink(ctx, data.tables.target);
  }

  grn_columns_fin(ctx, &(data.columns));

  return NULL;
}

#define N_VARS 33
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
