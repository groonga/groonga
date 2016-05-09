/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil

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

#include "../grn_proc.h"
#include "../grn_expr.h"
#include "../grn_str.h"
#include "../grn_output.h"
#include "../grn_util.h"
#include "../grn_cache.h"
#include "../grn_ii.h"

#include "../grn_ts.h"

#include <groonga/plugin.h>

#define GRN_SELECT_INTERNAL_VAR_MATCH_COLUMNS "$match_columns"

#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key, _nsubrecs"

#define GRN_SELECT_FILL_STRING(string, bulk)     \
  if (bulk && GRN_TEXT_LEN(bulk) > 0) {          \
    string.value = GRN_TEXT_VALUE(bulk);         \
    string.length = GRN_TEXT_LEN(bulk);          \
  } else {                                       \
    string.value = NULL;                         \
    string.length = 0;                           \
  }                                              \

#define GRN_BULK_EQUAL_STRING(bulk, string)                             \
  (GRN_TEXT_LEN(bulk) == strlen(string) &&                              \
   memcmp(GRN_TEXT_VALUE(bulk), string, GRN_TEXT_LEN(bulk)) == 0)

typedef struct {
  const char *value;
  size_t length;
} grn_select_string;

typedef struct {
  grn_select_string label;
  grn_select_string keys;
  grn_select_string sortby;
  grn_select_string output_columns;
  int offset;
  int limit;
  grn_table_group_flags calc_types;
  grn_select_string calc_target_name;
  grn_select_string table_name;
  struct {
    grn_hash *initial;
  } columns;
} grn_drilldown_data;

typedef enum {
  GRN_COLUMN_STAGE_UNKNOWN,
  GRN_COLUMN_STAGE_INITIAL,
  GRN_COLUMN_STAGE_FILTERED
} grn_column_stage;

typedef struct {
  grn_select_string label;
  grn_column_stage stage;
  grn_obj *type;
  grn_obj_flags flags;
  grn_select_string value;
  grn_select_string sortby;
} grn_column_data;

typedef struct {
  grn_select_string table;
  grn_select_string match_columns;
  grn_select_string query;
  grn_select_string filter;
  grn_select_string scorer;
  grn_select_string sortby;
  grn_select_string output_columns;
  int offset;
  int limit;
  grn_drilldown_data *drilldowns;
  size_t n_drilldowns;
  grn_obj *drilldown_labels;
  grn_select_string cache;
  grn_select_string match_escalation_threshold;
  grn_select_string query_expander;
  grn_select_string query_flags;
  grn_select_string adjuster;
  struct {
    grn_hash *filtered;
  } columns;
} grn_select_data;

grn_rc
grn_proc_syntax_expand_query(grn_ctx *ctx,
                             const char *query,
                             unsigned int query_len,
                             grn_expr_flags flags,
                             const char *query_expander_name,
                             unsigned int query_expander_name_len,
                             grn_obj *expanded_query)
{
  grn_obj *query_expander;

  query_expander = grn_ctx_get(ctx,
                               query_expander_name,
                               query_expander_name_len);
  if (!query_expander) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "nonexistent query expansion column: <%.*s>",
                     query_expander_name_len, query_expander_name);
    return ctx->rc;
  }

  return grn_expr_syntax_expand_query(ctx, query, query_len, flags,
                                      query_expander, expanded_query);
}

static grn_expr_flags
grn_parse_query_flags(grn_ctx *ctx, const char *query_flags,
                      unsigned int query_flags_len)
{
  grn_expr_flags flags = 0;
  const char *query_flags_end = query_flags + query_flags_len;

  while (query_flags < query_flags_end) {
    if (*query_flags == '|' || *query_flags == ' ') {
      query_flags += 1;
      continue;
    }

#define CHECK_EXPR_FLAG(name)\
  if (((query_flags_end - query_flags) >= (sizeof(#name) - 1)) &&\
      (!memcmp(query_flags, #name, sizeof(#name) - 1))) {\
    flags |= GRN_EXPR_ ## name;\
    query_flags += sizeof(#name) - 1;\
    continue;\
  }

    CHECK_EXPR_FLAG(ALLOW_PRAGMA);
    CHECK_EXPR_FLAG(ALLOW_COLUMN);
    CHECK_EXPR_FLAG(ALLOW_UPDATE);
    CHECK_EXPR_FLAG(ALLOW_LEADING_NOT);

#define GRN_EXPR_NONE 0
    CHECK_EXPR_FLAG(NONE);
#undef GNR_EXPR_NONE

    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "invalid query flag: <%.*s>",
                     (int)(query_flags_end - query_flags),
                     query_flags);
    return 0;
#undef CHECK_EXPR_FLAG
  }

  return flags;
}

static int
grn_select_apply_adjuster_ensure_factor(grn_ctx *ctx, grn_obj *factor_object)
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
grn_select_apply_adjuster_adjust(grn_ctx *ctx, grn_obj *table, grn_obj *res,
                                 grn_obj *column, grn_obj *value,
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
    ERR(GRN_INVALID_ARGUMENT,
        "adjuster requires index column for the target column: <%.*s>",
        column_name_size, column_name);
    return;
  }

  factor_value = grn_select_apply_adjuster_ensure_factor(ctx, factor);

  {
    grn_search_optarg options;
    memset(&options, 0, sizeof(grn_search_optarg));

    options.mode = GRN_OP_EXACT;
    options.similarity_threshold = 0;
    options.max_interval = 0;
    options.weight_vector = NULL;
    options.vector_size = factor_value;
    options.proc = NULL;
    options.max_size = 0;
    options.scorer = NULL;

    grn_obj_search(ctx, index, value, res, GRN_OP_ADJUST, &options);
  }
}

static void
grn_select_apply_adjuster(grn_ctx *ctx, grn_obj *table, grn_obj *res,
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
    grn_select_apply_adjuster_adjust(ctx, table, res, column, value, factor);
  }
}

static void
grn_select_expression_set_condition(grn_ctx *ctx,
                                    grn_obj *expression,
                                    grn_obj *condition)
{
  grn_obj *condition_ptr;

  if (!expression) {
    return;
  }

  condition_ptr =
    grn_expr_get_or_add_var(ctx, expression,
                            GRN_SELECT_INTERNAL_VAR_CONDITION,
                            strlen(GRN_SELECT_INTERNAL_VAR_CONDITION));
  GRN_PTR_INIT(condition_ptr, 0, GRN_DB_OBJECT);
  GRN_PTR_SET(ctx, condition_ptr, condition);
}

void
grn_proc_select_output_columns(grn_ctx *ctx, grn_obj *res,
                               int n_hits, int offset, int limit,
                               const char *columns, int columns_len,
                               grn_obj *condition)
{
  grn_rc rc;
  grn_obj_format format;

  GRN_OBJ_FORMAT_INIT(&format, n_hits, offset, limit, offset);
  format.flags =
    GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
    GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
  rc = grn_output_format_set_columns(ctx, &format, res, columns, columns_len);
  if (rc != GRN_SUCCESS) {
    GRN_OBJ_FORMAT_FIN(ctx, &format);
    return;
  }

  grn_select_expression_set_condition(ctx, format.expression, condition);
  GRN_OUTPUT_OBJ(res, &format);
  GRN_OBJ_FORMAT_FIN(ctx, &format);
}

static const char *
grn_column_stage_name(grn_column_stage stage)
{
  switch (stage) {
  case GRN_COLUMN_STAGE_INITIAL :
    return "initial";
  case GRN_COLUMN_STAGE_FILTERED :
    return "filtered";
  default :
    return "unknown";
  }
}

static void
grn_select_apply_columns(grn_ctx *ctx,
                         grn_obj *table,
                         grn_hash *columns,
                         grn_obj *condition)
{
  grn_hash_cursor *columns_cursor;

  columns_cursor = grn_hash_cursor_open(ctx, columns,
                                        NULL, 0, NULL, 0, 0, -1, 0);
  if (!columns_cursor) {
    return;
  }

  while (grn_hash_cursor_next(ctx, columns_cursor) != GRN_ID_NIL) {
    grn_column_data *column_data;
    grn_obj *target_table = table;
    grn_obj *column;
    grn_obj *expression;
    grn_obj *record;
    grn_table_cursor *table_cursor;
    grn_id id;

    grn_hash_cursor_get_value(ctx, columns_cursor, (void **)&column_data);

    column = grn_column_create(ctx,
                               table,
                               column_data->label.value,
                               column_data->label.length,
                               NULL,
                               column_data->flags,
                               column_data->type);
    if (!column) {
      char error_message[GRN_CTX_MSGSIZE];
      grn_memcpy(error_message, ctx->errbuf, GRN_CTX_MSGSIZE);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][column][%s][%.*s] failed to create column: %s",
                       grn_column_stage_name(column_data->stage),
                       (int)(column_data->label.length),
                       column_data->label.value,
                       error_message);
      break;
    }

    if (column_data->sortby.length > 0) {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_key_from_str(ctx,
                                              column_data->sortby.value,
                                              column_data->sortby.length,
                                              table, &n_sort_keys);
      if (!sort_keys) {
        char error_message[GRN_CTX_MSGSIZE];
        grn_memcpy(error_message, ctx->errbuf, GRN_CTX_MSGSIZE);
        grn_obj_close(ctx, column);
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[select][column][%s][%.*s] failed to parse sort key: %s",
                         grn_column_stage_name(column_data->stage),
                         (int)(column_data->label.length),
                         column_data->label.value,
                         error_message);
        break;
      }

      target_table = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                      NULL, table);
      if (!target_table) {
        char error_message[GRN_CTX_MSGSIZE];
        grn_memcpy(error_message, ctx->errbuf, GRN_CTX_MSGSIZE);
        grn_obj_close(ctx, column);
        grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
        GRN_PLUGIN_ERROR(ctx,
                         GRN_INVALID_ARGUMENT,
                         "[select][column][%s][%.*s] failed to create sorted table: %s",
                         grn_column_stage_name(column_data->stage),
                         (int)(column_data->label.length),
                         column_data->label.value,
                         error_message);
        break;
      }
      grn_table_sort(ctx, table, 0, -1,
                     target_table, sort_keys, n_sort_keys);
      grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
    }

    GRN_EXPR_CREATE_FOR_QUERY(ctx, target_table, expression, record);
    if (!expression) {
      char error_message[GRN_CTX_MSGSIZE];
      grn_memcpy(error_message, ctx->errbuf, GRN_CTX_MSGSIZE);
      grn_obj_close(ctx, column);
      if (column_data->sortby.length > 0) {
        grn_obj_unlink(ctx, target_table);
      }
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][column][%s][%.*s] "
                       "failed to create expression to compute value: %s",
                       grn_column_stage_name(column_data->stage),
                       (int)(column_data->label.length),
                       column_data->label.value,
                       error_message);
      break;
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
      char error_message[GRN_CTX_MSGSIZE];
      grn_memcpy(error_message, ctx->errbuf, GRN_CTX_MSGSIZE);
      grn_obj_close(ctx, expression);
      grn_obj_close(ctx, column);
      if (column_data->sortby.length > 0) {
        grn_obj_unlink(ctx, target_table);
      }
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][column][%s][%.*s] "
                       "failed to parse value: <%.*s>: %s",
                       grn_column_stage_name(column_data->stage),
                       (int)(column_data->label.length),
                       column_data->label.value,
                       (int)(column_data->value.length),
                       column_data->value.value,
                       error_message);
      break;
    }
    grn_select_expression_set_condition(ctx, expression, condition);

    table_cursor = grn_table_cursor_open(ctx, target_table,
                                         NULL, 0,
                                         NULL, 0,
                                         0, -1, GRN_CURSOR_BY_ID);
    if (!table_cursor) {
      char error_message[GRN_CTX_MSGSIZE];
      grn_memcpy(error_message, ctx->errbuf, GRN_CTX_MSGSIZE);
      grn_obj_close(ctx, expression);
      grn_obj_close(ctx, column);
      if (column_data->sortby.length > 0) {
        grn_obj_unlink(ctx, target_table);
      }
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][column][%s][%.*s] "
                       "failed to create cursor for getting records: %s",
                       grn_column_stage_name(column_data->stage),
                       (int)(column_data->label.length),
                       column_data->label.value,
                       error_message);
      break;
    }

    while ((id = grn_table_cursor_next(ctx, table_cursor)) != GRN_ID_NIL) {
      grn_obj *value;

      GRN_RECORD_SET(ctx, record, id);
      value = grn_expr_exec(ctx, expression, 0);
      if (column_data->sortby.length > 0) {
        void *buf;
        grn_table_cursor_get_value(ctx, table_cursor, &buf);
        id = *((grn_id *)buf);
      }
      if (value) {
        grn_obj_set_value(ctx, column, id, value, GRN_OBJ_SET);
      }
    }

    if (column_data->sortby.length > 0) {
      grn_obj_unlink(ctx, target_table);
    }
    grn_obj_close(ctx, expression);
  }

  grn_hash_cursor_close(ctx, columns_cursor);
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

#define GRN_TABLE_GROUP_CALC_NONE 0
    CHECK_TABLE_GROUP_CALC_TYPE(NONE);
#undef GRN_TABLE_GROUP_CALC_NONE

    ERR(GRN_INVALID_ARGUMENT, "invalid table group calc type: <%.*s>",
        (int)(calc_types_end - calc_types), calc_types);
    return 0;
#undef CHECK_TABLE_GROUP_CALC_TYPE
  }

  return flags;
}

static void
grn_drilldown_data_fill(grn_ctx *ctx,
                        grn_drilldown_data *drilldown,
                        grn_obj *keys,
                        grn_obj *sortby,
                        grn_obj *output_columns,
                        grn_obj *offset,
                        grn_obj *limit,
                        grn_obj *calc_types,
                        grn_obj *calc_target,
                        grn_obj *table)
{
  GRN_SELECT_FILL_STRING(drilldown->keys, keys);

  GRN_SELECT_FILL_STRING(drilldown->sortby, sortby);

  GRN_SELECT_FILL_STRING(drilldown->output_columns, output_columns);
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

  GRN_SELECT_FILL_STRING(drilldown->calc_target_name, calc_target);

  GRN_SELECT_FILL_STRING(drilldown->table_name, table);
}

static void
grn_select_drilldown(grn_ctx *ctx, grn_obj *table,
                     grn_table_sort_key *keys, uint32_t n_keys,
                     grn_drilldown_data *drilldown)
{
  uint32_t i;
  for (i = 0; i < n_keys; i++) {
    grn_table_group_result g = {NULL, 0, 0, 1, GRN_TABLE_GROUP_CALC_COUNT, 0};
    uint32_t n_hits;
    int offset;
    int limit;

    if (drilldown->calc_target_name.length > 0) {
      g.calc_target = grn_obj_column(ctx, table,
                                     drilldown->calc_target_name.value,
                                     drilldown->calc_target_name.length);
    }
    if (g.calc_target) {
      g.flags |= drilldown->calc_types;
    }

    grn_table_group(ctx, table, &keys[i], 1, &g, 1);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    n_hits = grn_table_size(ctx, g.table);

    offset = drilldown->offset;
    limit = drilldown->limit;
    grn_normalize_offset_and_limit(ctx, n_hits, &offset, &limit);

    if (drilldown->sortby.length > 0) {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_key_from_str(ctx,
                                              drilldown->sortby.value,
                                              drilldown->sortby.length,
                                              g.table, &n_sort_keys);
      if (sort_keys) {
        grn_obj *sorted;
        sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                  NULL, g.table);
        if (sorted) {
          grn_obj_format format;
          grn_table_sort(ctx, g.table, offset, limit,
                         sorted, sort_keys, n_sort_keys);
          GRN_OBJ_FORMAT_INIT(&format, n_hits, 0, limit, offset);
          format.flags =
            GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
            GRN_OBJ_FORMAT_XML_ELEMENT_NAVIGATIONENTRY;
          grn_obj_columns(ctx, sorted,
                          drilldown->output_columns.value,
                          drilldown->output_columns.length,
                          &format.columns);
          GRN_OUTPUT_OBJ(sorted, &format);
          GRN_OBJ_FORMAT_FIN(ctx, &format);
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
      }
    } else {
      grn_obj_format format;
      GRN_OBJ_FORMAT_INIT(&format, n_hits, offset, limit, offset);
      format.flags =
        GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
        GRN_OBJ_FORMAT_XML_ELEMENT_NAVIGATIONENTRY;
      grn_obj_columns(ctx, g.table,
                      drilldown->output_columns.value,
                      drilldown->output_columns.length,
                      &format.columns);
      GRN_OUTPUT_OBJ(g.table, &format);
      GRN_OBJ_FORMAT_FIN(ctx, &format);
    }
    grn_obj_unlink(ctx, g.table);
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "drilldown(%d)", n_hits);
  }
}

typedef enum {
  TSORT_STATUS_NOT_VISITED,
  TSORT_STATUS_VISITING,
  TSORT_STATUS_VISITED
} tsort_status;

static grn_bool
drilldown_tsort_visit(grn_ctx *ctx,
                      grn_obj *labels,
                      tsort_status *statuses,
                      grn_drilldown_data *drilldowns,
                      uint32_t index,
                      grn_obj *indexes)
{
  grn_bool cycled = GRN_TRUE;

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
      grn_drilldown_data *drilldown = &(drilldowns[index]);
      if (drilldown->table_name.length > 0) {
        grn_id dependent_id;
        dependent_id = grn_table_get(ctx, labels,
                                     drilldown->table_name.value,
                                     drilldown->table_name.length);
        if (dependent_id != GRN_ID_NIL) {
          uint32_t dependent_index = dependent_id - 1;
          cycled = drilldown_tsort_visit(ctx,
                                         labels,
                                         statuses,
                                         drilldowns,
                                         dependent_index,
                                         indexes);
          if (cycled) {
            GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                             "[select][drilldown][%.*s][table] "
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
      GRN_UINT32_PUT(ctx, indexes, index);
    }
    break;
  }

  return cycled;
}

static grn_bool
drilldown_tsort_body(grn_ctx *ctx,
                     grn_obj *labels,
                     tsort_status *statuses,
                     grn_drilldown_data *drilldowns,
                     size_t n_drilldowns,
                     grn_obj *indexes)
{
  grn_bool succeeded = GRN_TRUE;
  size_t i;
  for (i = 0; i < n_drilldowns; i++) {
    grn_drilldown_data *drilldown = &(drilldowns[i]);
    grn_id id;
    id = grn_table_get(ctx, labels,
                       drilldown->label.value,
                       drilldown->label.length);
    if (id != GRN_ID_NIL) {
      uint32_t index = id - 1;
      if (drilldown_tsort_visit(ctx, labels, statuses, drilldowns,
                                index, indexes)) {
        succeeded = GRN_FALSE;
        break;
      }
    }
  }
  return succeeded;
}

static void
drilldown_tsort_init(grn_ctx *ctx,
                     grn_obj *labels,
                     tsort_status *statuses,
                     grn_drilldown_data *drilldowns,
                     size_t n_drilldowns)
{
  size_t i;
  for (i = 0; i < n_drilldowns; i++) {
    statuses[i] = TSORT_STATUS_NOT_VISITED;
  }
}

static grn_bool
drilldown_tsort(grn_ctx *ctx,
                grn_obj *labels,
                grn_drilldown_data *drilldowns,
                size_t n_drilldowns,
                grn_obj *indexes)
{
  tsort_status *statuses;
  grn_bool succeeded;

  statuses = GRN_PLUGIN_MALLOCN(ctx, tsort_status, n_drilldowns);
  if (!statuses) {
    return GRN_FALSE;
  }

  drilldown_tsort_init(ctx, labels, statuses, drilldowns, n_drilldowns);
  succeeded = drilldown_tsort_body(ctx,
                                   labels,
                                   statuses,
                                   drilldowns,
                                   n_drilldowns,
                                   indexes);
  GRN_PLUGIN_FREE(ctx, statuses);
  return succeeded;
}

static grn_table_group_result *
grn_select_drilldowns_execute(grn_ctx *ctx,
                              grn_obj *table,
                              grn_drilldown_data *drilldowns,
                              size_t n_drilldowns,
                              grn_obj *labels,
                              grn_obj *condition)
{
  grn_table_group_result *results = NULL;
  grn_obj tsorted_indexes;
  size_t i;

  if (!labels) {
    return NULL;
  }

  GRN_UINT32_INIT(&tsorted_indexes, GRN_OBJ_VECTOR);
  if (!drilldown_tsort(ctx, labels, drilldowns, n_drilldowns, &tsorted_indexes)) {
    goto exit;
  }

  results = GRN_PLUGIN_MALLOCN(ctx, grn_table_group_result, n_drilldowns);
  if (!results) {
    goto exit;
  }

  for (i = 0; i < n_drilldowns; i++) {
    grn_table_group_result *result = results + i;
    result->table = NULL;
  }

  for (i = 0; i < n_drilldowns; i++) {
    grn_table_sort_key *keys = NULL;
    unsigned int n_keys = 0;
    grn_obj *target_table = table;
    unsigned int index;
    grn_drilldown_data *drilldown;
    grn_table_group_result *result;

    index = GRN_UINT32_VALUE_AT(&tsorted_indexes, i);
    drilldown = drilldowns + index;
    result = results + index;

    result->limit = 1;
    result->flags = GRN_TABLE_GROUP_CALC_COUNT;
    result->op = 0;
    result->max_n_subrecs = 0;
    result->key_begin = 0;
    result->key_end = 0;
    result->calc_target = NULL;

    if (drilldown->table_name.length > 0) {
      grn_id dependent_id;
      dependent_id = grn_table_get(ctx,
                                   labels,
                                   drilldown->table_name.value,
                                   drilldown->table_name.length);
      if (dependent_id == GRN_ID_NIL) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[select][drilldown][%.*s][table] "
                         "nonexistent label: <%.*s>",
                         (int)(drilldown->label.length),
                         drilldown->label.value,
                         (int)(drilldown->table_name.length),
                         drilldown->table_name.value);
        break;
      } else {
        uint32_t dependent_index = dependent_id - 1;
        target_table = results[dependent_index].table;
      }
    }

    if (drilldown->keys.length > 0) {
      keys = grn_table_sort_key_from_str(ctx,
                                         drilldown->keys.value,
                                         drilldown->keys.length,
                                         target_table, &n_keys);
      if (!keys) {
        GRN_PLUGIN_CLEAR_ERROR(ctx);
        continue;
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

    grn_table_group(ctx, target_table, keys, n_keys, result, 1);
    if (keys) {
      grn_table_sort_key_close(ctx, keys, n_keys);
    }
    if (drilldown->columns.initial) {
      grn_select_apply_columns(ctx, result->table, drilldown->columns.initial,
                               condition);
    }
  }

exit :
  GRN_OBJ_FIN(ctx, &tsorted_indexes);

  return results;
}

static void
grn_select_drilldowns_output(grn_ctx *ctx,
                             grn_obj *table,
                             grn_drilldown_data *drilldowns,
                             size_t n_drilldowns,
                             grn_obj *condition,
                             grn_table_group_result *results)
{
  size_t i;
  unsigned int n_available_results = 0;

  for (i = 0; i < n_drilldowns; i++) {
    grn_table_group_result *result = results + i;

    if (result->table) {
      n_available_results++;
    }
  }

  GRN_OUTPUT_MAP_OPEN("DRILLDOWNS", n_available_results);
  for (i = 0; i < n_drilldowns; i++) {
    grn_drilldown_data *drilldown = drilldowns + i;
    grn_table_group_result *result = results + i;
    uint32_t n_hits;
    int offset;
    int limit;

    if (!result->table) {
      continue;
    }

    GRN_OUTPUT_STR(drilldown->label.value, drilldown->label.length);

    n_hits = grn_table_size(ctx, result->table);

    offset = drilldown->offset;
    limit = drilldown->limit;
    grn_normalize_offset_and_limit(ctx, n_hits, &offset, &limit);

    if (drilldown->sortby.length > 0) {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_key_from_str(ctx,
                                              drilldown->sortby.value,
                                              drilldown->sortby.length,
                                              result->table, &n_sort_keys);
      if (sort_keys) {
        grn_obj *sorted;
        sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                  NULL, result->table);
        if (sorted) {
          grn_table_sort(ctx, result->table, offset, limit,
                         sorted, sort_keys, n_sort_keys);
          grn_proc_select_output_columns(ctx, sorted, n_hits, 0, limit,
                                         drilldown->output_columns.value,
                                         drilldown->output_columns.length,
                                         condition);
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
      }
    } else {
      grn_proc_select_output_columns(ctx, result->table, n_hits, offset, limit,
                                     drilldown->output_columns.value,
                                     drilldown->output_columns.length,
                                     condition);
    }

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "drilldown(%d)[%.*s]",
                  n_hits,
                  (int)(drilldown->label.length),
                  drilldown->label.value);
  }
  GRN_OUTPUT_MAP_CLOSE();
}

static void
grn_select_drilldowns(grn_ctx *ctx, grn_obj *table,
                      grn_drilldown_data *drilldowns, size_t n_drilldowns,
                      grn_obj *drilldown_labels, grn_obj *condition)
{
  grn_table_group_result *results;

  results = grn_select_drilldowns_execute(ctx,
                                          table,
                                          drilldowns,
                                          n_drilldowns,
                                          drilldown_labels,
                                          condition);
  if (!results) {
    return;
  }

  grn_select_drilldowns_output(ctx,
                               table,
                               drilldowns,
                               n_drilldowns,
                               condition,
                               results);

  {
    size_t i;

    for (i = 0; i < n_drilldowns; i++) {
      grn_table_group_result *result = results + i;

      if (!result->table) {
        continue;
      }

      if (result->calc_target) {
        grn_obj_unlink(ctx, result->calc_target);
      }
      if (result->table) {
        grn_obj_close(ctx, result->table);
      }

    }
  }
  GRN_PLUGIN_FREE(ctx, results);
}

static grn_rc
grn_select(grn_ctx *ctx, grn_select_data *data)
{
  uint32_t nkeys, nhits;
  uint16_t cacheable = 1, taintable = 0;
  grn_table_sort_key *keys;
  grn_obj *outbuf = ctx->impl->output.buf;
  grn_content_type output_type = ctx->impl->output.type;
  grn_obj *table;
  grn_obj *match_columns = NULL;
  grn_obj *cond = NULL;
  grn_obj *scorer;
  grn_obj *res = NULL;
  grn_obj *sorted;
  char cache_key[GRN_CACHE_MAX_KEY_SIZE];
  uint32_t cache_key_size;
  long long int threshold, original_threshold = 0;
  grn_cache *cache_obj = grn_cache_current_get(ctx);

  {
    const char *query_end = data->query.value + data->query.length;
    int space_len;
    while (data->query.value < query_end) {
      space_len = grn_isspace(data->query.value, ctx->encoding);
      if (space_len == 0) {
        break;
      }
      data->query.value += space_len;
      data->query.length -= space_len;
    }
  }

  cache_key_size =
    data->table.length + 1 +
    data->match_columns.length + 1 +
    data->query.length + 1 +
    data->filter.length + 1 +
    data->scorer.length + 1 +
    data->sortby.length + 1 +
    data->output_columns.length + 1 +
    data->match_escalation_threshold.length + 1 +
    data->query_expander.length + 1 +
    data->query_flags.length + 1 +
    data->adjuster.length + 1 +
    sizeof(grn_content_type) +
    sizeof(int) * 2 +
    sizeof(grn_command_version) +
    sizeof(grn_bool);
  {
    size_t i;
    for (i = 0; i < data->n_drilldowns; i++) {
      grn_drilldown_data *drilldown = &(data->drilldowns[i]);
      cache_key_size +=
        drilldown->keys.length + 1 +
        drilldown->sortby.length + 1 +
        drilldown->output_columns.length + 1 +
        drilldown->label.length + 1 +
        drilldown->calc_target_name.length + 1 +
        drilldown->table_name.length + 1 +
        sizeof(int) * 2 +
        sizeof(grn_table_group_flags);
    }
  }
  if (cache_key_size <= GRN_CACHE_MAX_KEY_SIZE) {
    grn_obj *cache_value;
    char *cp = cache_key;

#define PUT_CACHE_KEY(string)                                   \
    grn_memcpy(cp, (string).value, (string).length);            \
    cp += (string).length;                                      \
    *cp++ = '\0'

    PUT_CACHE_KEY(data->table);
    PUT_CACHE_KEY(data->match_columns);
    PUT_CACHE_KEY(data->query);
    PUT_CACHE_KEY(data->filter);
    PUT_CACHE_KEY(data->scorer);
    PUT_CACHE_KEY(data->sortby);
    PUT_CACHE_KEY(data->output_columns);
    {
      size_t i;
      for (i = 0; i < data->n_drilldowns; i++) {
        grn_drilldown_data *drilldown = &(data->drilldowns[i]);
        PUT_CACHE_KEY(drilldown->keys);
        PUT_CACHE_KEY(drilldown->sortby);
        PUT_CACHE_KEY(drilldown->output_columns);
        PUT_CACHE_KEY(drilldown->label);
        PUT_CACHE_KEY(drilldown->calc_target_name);
        PUT_CACHE_KEY(drilldown->table_name);
      }
    }
    PUT_CACHE_KEY(data->match_escalation_threshold);
    PUT_CACHE_KEY(data->query_expander);
    PUT_CACHE_KEY(data->query_flags);
    PUT_CACHE_KEY(data->adjuster);
    grn_memcpy(cp, &output_type, sizeof(grn_content_type));
    cp += sizeof(grn_content_type);
    grn_memcpy(cp, &(data->offset), sizeof(int));
    cp += sizeof(int);
    grn_memcpy(cp, &(data->limit), sizeof(int));
    cp += sizeof(int);
    grn_memcpy(cp, &(ctx->impl->command_version), sizeof(grn_command_version));
    cp += sizeof(grn_command_version);
    grn_memcpy(cp, &(ctx->impl->output.is_pretty), sizeof(grn_bool));
    cp += sizeof(grn_bool);
    {
      size_t i;
      for (i = 0; i < data->n_drilldowns; i++) {
        grn_drilldown_data *drilldown = &(data->drilldowns[i]);
        grn_memcpy(cp, &(drilldown->offset), sizeof(int));
        cp += sizeof(int);
        grn_memcpy(cp, &(drilldown->limit), sizeof(int));
        cp += sizeof(int);
        grn_memcpy(cp, &(drilldown->calc_types), sizeof(grn_table_group_flags));
        cp += sizeof(grn_table_group_flags);
      }
    }
#undef PUT_CACHE_KEY

    cache_value = grn_cache_fetch(ctx, cache_obj, cache_key, cache_key_size);
    if (cache_value) {
      GRN_TEXT_PUT(ctx, outbuf,
                   GRN_TEXT_VALUE(cache_value),
                   GRN_TEXT_LEN(cache_value));
      grn_cache_unref(ctx, cache_obj, cache_key, cache_key_size);
      GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_CACHE,
                    ":", "cache(%" GRN_FMT_LLD ")",
                    (long long int)GRN_TEXT_LEN(cache_value));
      return ctx->rc;
    }
  }
  if (data->match_escalation_threshold.length) {
    const char *end, *rest;
    original_threshold = grn_ctx_get_match_escalation_threshold(ctx);
    end =
      data->match_escalation_threshold.value +
      data->match_escalation_threshold.length;
    threshold = grn_atoll(data->match_escalation_threshold.value, end, &rest);
    if (end == rest) {
      grn_ctx_set_match_escalation_threshold(ctx, threshold);
    }
  }
  if ((table = grn_ctx_get(ctx, data->table.value, data->table.length))) {
    if (data->filter.length > 0 && (data->filter.value[0] == '?') &&
        (ctx->impl->output.type == GRN_CONTENT_JSON)) {
      ctx->rc = grn_ts_select(ctx, table,
                              data->filter.value + 1,
                              data->filter.length - 1,
                              data->scorer.value,
                              data->scorer.length,
                              data->sortby.value,
                              data->sortby.length,
                              data->output_columns.value,
                              data->output_columns.length,
                              data->offset,
                              data->limit);
      if (!ctx->rc && cacheable && cache_key_size <= GRN_CACHE_MAX_KEY_SIZE &&
          (!data->cache.value ||
           data->cache.length != 2 ||
           data->cache.value[0] != 'n' ||
           data->cache.value[1] != 'o')) {
        grn_cache_update(ctx, cache_obj, cache_key, cache_key_size, outbuf);
      }
      goto exit;
    }
    if (data->query.length > 0 || data->filter.length > 0) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, table, cond, v);
      if (cond) {
        if (data->match_columns.length) {
          GRN_EXPR_CREATE_FOR_QUERY(ctx, table, match_columns, v);
          if (match_columns) {
            grn_expr_parse(ctx, match_columns,
                           data->match_columns.value,
                           data->match_columns.length,
                           NULL, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
            if (ctx->rc) {
              goto exit;
            }
          } else {
            /* todo */
          }
        }
        if (data->query.length > 0) {
          grn_expr_flags flags;
          grn_obj query_expander_buf;
          const char *query = data->query.value;
          unsigned int query_len = data->query.length;

          flags = GRN_EXPR_SYNTAX_QUERY;
          if (data->query_flags.length) {
            flags |= grn_parse_query_flags(ctx,
                                           data->query_flags.value,
                                           data->query_flags.length);
            if (ctx->rc) {
              goto exit;
            }
          } else {
            flags |= GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN;
          }
          GRN_TEXT_INIT(&query_expander_buf, 0);
          if (data->query_expander.length) {
            grn_rc rc;
            rc = grn_proc_syntax_expand_query(ctx,
                                              data->query.value,
                                              data->query.length,
                                              flags,
                                              data->query_expander.value,
                                              data->query_expander.length,
                                              &query_expander_buf);
            if (rc == GRN_SUCCESS) {
              query = GRN_TEXT_VALUE(&query_expander_buf);
              query_len = GRN_TEXT_LEN(&query_expander_buf);
            } else {
              GRN_OBJ_FIN(ctx, &query_expander_buf);
              goto exit;
            }
          }
          grn_expr_parse(ctx, cond, query, query_len,
                         match_columns, GRN_OP_MATCH, GRN_OP_AND, flags);
          GRN_OBJ_FIN(ctx, &query_expander_buf);
          if (!ctx->rc && data->filter.length > 0) {
            grn_expr_parse(ctx, cond, data->filter.value, data->filter.length,
                           match_columns, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
            if (!ctx->rc) { grn_expr_append_op(ctx, cond, GRN_OP_AND, 2); }
          }
        } else {
          grn_expr_parse(ctx, cond, data->filter.value, data->filter.length,
                         match_columns, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT);
        }
        cacheable *= ((grn_expr *)cond)->cacheable;
        taintable += ((grn_expr *)cond)->taintable;
        /*
        grn_obj strbuf;
        GRN_TEXT_INIT(&strbuf, 0);
        grn_expr_inspect(ctx, &strbuf, cond);
        GRN_TEXT_PUTC(ctx, &strbuf, '\0');
        GRN_LOG(ctx, GRN_LOG_NOTICE, "query=(%s)", GRN_TEXT_VALUE(&strbuf));
        GRN_OBJ_FIN(ctx, &strbuf);
        */
        if (!ctx->rc) { res = grn_table_select(ctx, table, cond, NULL, GRN_OP_OR); }
      } else {
        /* todo */
        ERRCLR(ctx);
      }
    } else {
      res = table;
    }
    nhits = res ? grn_table_size(ctx, res) : 0;
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "select(%d)", nhits);

    if (res && data->columns.filtered) {
      if (res == table) {
        grn_posting posting;

        memset(&posting, 0, sizeof(grn_posting));
        res = grn_table_create(ctx, NULL, 0, NULL,
                               GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                               table, NULL);
        GRN_TABLE_EACH(ctx, table, 0, 0, id, NULL, NULL, NULL, {
          posting.rid = id;
          grn_ii_posting_add(ctx, &posting, (grn_hash *)res, GRN_OP_OR);
        });
      }
      grn_select_apply_columns(ctx, res, data->columns.filtered, cond);
    }

    if (res) {
      uint32_t ngkeys;
      grn_table_sort_key *gkeys = NULL;
      int result_size = 1;
      if (!ctx->rc && data->n_drilldowns > 0) {
        if (data->n_drilldowns == 1 && data->drilldowns[0].label.length == 0) {
          gkeys = grn_table_sort_key_from_str(ctx,
                                              data->drilldowns[0].keys.value,
                                              data->drilldowns[0].keys.length,
                                              res, &ngkeys);
          if (gkeys) {
            result_size += ngkeys;
          }
        } else {
          result_size += 1;
        }
      }

      if (data->adjuster.length > 0) {
        grn_obj *adjuster;
        grn_obj *v;
        GRN_EXPR_CREATE_FOR_QUERY(ctx, table, adjuster, v);
        if (adjuster && v) {
          grn_rc rc;
          rc = grn_expr_parse(ctx, adjuster,
                              data->adjuster.value,
                              data->adjuster.length,
                              NULL,
                              GRN_OP_MATCH, GRN_OP_ADJUST,
                              GRN_EXPR_SYNTAX_ADJUSTER);
          if (rc) {
            grn_obj_unlink(ctx, adjuster);
            goto exit;
          }
          cacheable *= ((grn_expr *)adjuster)->cacheable;
          taintable += ((grn_expr *)adjuster)->taintable;
          grn_select_apply_adjuster(ctx, table, res, adjuster);
          grn_obj_unlink(ctx, adjuster);
        }
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                      ":", "adjust(%d)", nhits);
      }

      if (data->scorer.length > 0) {
        grn_obj *v;
        GRN_EXPR_CREATE_FOR_QUERY(ctx, res, scorer, v);
        if (scorer && v) {
          grn_table_cursor *tc;
          grn_expr_parse(ctx, scorer,
                         data->scorer.value,
                         data->scorer.length, NULL, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
          cacheable *= ((grn_expr *)scorer)->cacheable;
          taintable += ((grn_expr *)scorer)->taintable;
          if ((tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, 0))) {
            grn_id id;
            while ((id = grn_table_cursor_next(ctx, tc)) != GRN_ID_NIL) {
              GRN_RECORD_SET(ctx, v, id);
              grn_expr_exec(ctx, scorer, 0);
              if (ctx->rc) {
                break;
              }
            }
            grn_table_cursor_close(ctx, tc);
          }
          grn_obj_unlink(ctx, scorer);
        }
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                      ":", "score(%d)", nhits);
      }

      GRN_OUTPUT_ARRAY_OPEN("RESULT", result_size);

      grn_normalize_offset_and_limit(ctx, nhits,
                                     &(data->offset), &(data->limit));

      if (data->sortby.length > 0 &&
          (keys = grn_table_sort_key_from_str(ctx,
                                              data->sortby.value,
                                              data->sortby.length,
                                              res,
                                              &nkeys))) {
        if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
          grn_table_sort(ctx, res, data->offset, data->limit,
                         sorted, keys, nkeys);
          GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                        ":", "sort(%d)", data->limit);
          grn_proc_select_output_columns(ctx, sorted, nhits, 0, data->limit,
                                         data->output_columns.value,
                                         data->output_columns.length,
                                         cond);
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, keys, nkeys);
      } else {
        if (!ctx->rc) {
          grn_proc_select_output_columns(ctx, res, nhits,
                                         data->offset, data->limit,
                                         data->output_columns.value,
                                         data->output_columns.length,
                                         cond);
        }
      }
      GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                    ":", "output(%d)", data->limit);
      if (!ctx->rc) {
        if (gkeys) {
          grn_drilldown_data *drilldown = &(data->drilldowns[0]);
          grn_select_drilldown(ctx, res, gkeys, ngkeys, drilldown);
        } else if (data->n_drilldowns > 0) {
          grn_select_drilldowns(ctx, res,
                                data->drilldowns,
                                data->n_drilldowns,
                                data->drilldown_labels,
                                cond);
        }
      }
      if (gkeys) {
        grn_table_sort_key_close(ctx, gkeys, ngkeys);
      }
      if (res != table) { grn_obj_unlink(ctx, res); }
    } else {
      GRN_OUTPUT_ARRAY_OPEN("RESULT", 0);
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    if (!ctx->rc && cacheable && cache_key_size <= GRN_CACHE_MAX_KEY_SIZE &&
        (!data->cache.value ||
         data->cache.length != 2 ||
         data->cache.value[0] != 'n' ||
         data->cache.value[1] != 'o')) {
      grn_cache_update(ctx, cache_obj, cache_key, cache_key_size, outbuf);
    }
    if (taintable) { grn_db_touch(ctx, DB_OBJ(table)->db); }
    grn_obj_unlink(ctx, table);
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "invalid table name: <%.*s>",
        (int)(data->table.length),
        data->table.value);
  }
exit :
  if (data->match_escalation_threshold.length > 0) {
    grn_ctx_set_match_escalation_threshold(ctx, original_threshold);
  }
  if (match_columns) {
    grn_obj_unlink(ctx, match_columns);
  }
  if (cond) {
    grn_obj_unlink(ctx, cond);
  }
  /* GRN_LOG(ctx, GRN_LOG_NONE, "%d", ctx->seqno); */
  return ctx->rc;
}

static grn_bool
grn_column_data_init(grn_ctx *ctx, const char *label, size_t label_len,
                     grn_obj *value, grn_hash **columns)
{
  grn_column_stage stage;

  if (GRN_BULK_EQUAL_STRING(value, "filtered")) {
    stage = GRN_COLUMN_STAGE_FILTERED;
  } else if (GRN_BULK_EQUAL_STRING(value, "initial")) {
    stage = GRN_COLUMN_STAGE_INITIAL;
  } else {
    stage = GRN_COLUMN_STAGE_UNKNOWN;
  }

  if (stage != GRN_COLUMN_STAGE_UNKNOWN) {
    void *column_raw;
    grn_column_data *column;

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
                 NULL);
    column = column_raw;
    column->label.value = label;
    column->label.length = label_len;
    column->stage = stage;
    column->type = grn_ctx_at(ctx, GRN_DB_TEXT);
    column->flags = GRN_OBJ_COLUMN_SCALAR;
    column->value.value = NULL;
    column->value.length = 0;
  }
  return GRN_TRUE;
}

static grn_bool
grn_column_data_fill(grn_ctx *ctx,
                     grn_column_data *column,
                     grn_obj *type_raw,
                     grn_obj *flags,
                     grn_obj *value,
                     grn_obj *sortby)
{
  if (type_raw && GRN_TEXT_LEN(type_raw) > 0) {
    grn_obj *type;

    type = grn_ctx_get(ctx, GRN_TEXT_VALUE(type_raw), GRN_TEXT_LEN(type_raw));
    if (!type) {
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "[select][column][%s][%.*s] unknown type: <%.*s>",
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
                       "[select][column][%s][%.*s] invalid type: %.*s",
                       grn_column_stage_name(column->stage),
                       (int)(column->label.length),
                       column->label.value,
                       (int)(GRN_TEXT_LEN(&inspected)),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      grn_obj_unlink(ctx, type);
      return GRN_FALSE;
    }
    column->type = type;
  }

  if (flags && GRN_TEXT_LEN(flags) > 0) {
    char error_message_tag[GRN_TABLE_MAX_KEY_SIZE];

    grn_snprintf(error_message_tag,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "[select][column][%s][%.*s]",
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

  GRN_SELECT_FILL_STRING(column->value, value);
  GRN_SELECT_FILL_STRING(column->sortby, sortby);

  return GRN_TRUE;
}

static grn_bool
grn_column_data_collect(grn_ctx *ctx,
                        grn_user_data *user_data,
                        grn_hash *columns,
                        const char *prefix_label,
                        size_t prefix_label_len)
{
  grn_hash_cursor *cursor = NULL;
  cursor = grn_hash_cursor_open(ctx, columns,
                                NULL, 0, NULL, 0, 0, -1, 0);
  if (!cursor) {
    return GRN_FALSE;
  }

  while (grn_hash_cursor_next(ctx, cursor)) {
    grn_column_data *column;
    char key_name[GRN_TABLE_MAX_KEY_SIZE];
    grn_obj *type;
    grn_obj *flags;
    grn_obj *value;
    grn_obj *sortby;

    grn_hash_cursor_get_value(ctx, cursor, (void **)&column);

#define GET_VAR(name)                                                   \
    grn_snprintf(key_name,                                              \
                 GRN_TABLE_MAX_KEY_SIZE,                                \
                 GRN_TABLE_MAX_KEY_SIZE,                                \
                 "%.*scolumn[%.*s]." # name,                            \
                 (int)prefix_label_len,                                 \
                 prefix_label,                                          \
                 (int)(column->label.length),                           \
                 column->label.value);                                  \
    name = grn_plugin_proc_get_var(ctx, user_data, key_name, -1);

    GET_VAR(type);
    GET_VAR(flags);
    GET_VAR(value);
    GET_VAR(sortby);

#undef GET_VAR

    grn_column_data_fill(ctx, column,
                         type, flags, value, sortby);
  }
  grn_hash_cursor_close(ctx, cursor);
  return GRN_TRUE;
}

static grn_bool
grn_select_data_fill_columns_collect(grn_ctx *ctx,
                                     grn_user_data *user_data,
                                     grn_select_data *data)
{
  grn_obj *vars;
  grn_table_cursor *cursor;
  const char *prefix = "column[";
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
    char *name;
    int name_len;
    void *value_raw;
    grn_obj *value;

    name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    name = key;
    if (name_len < prefix_len + suffix_len + 1) {
      continue;
    }

    if (memcmp(prefix, name, prefix_len) != 0) {
      continue;
    }

    if (memcmp(suffix, name + (name_len - suffix_len), suffix_len) != 0) {
      continue;
    }

    grn_table_cursor_get_value(ctx, cursor, &value_raw);
    value = value_raw;
    if (!grn_column_data_init(ctx,
                              name + prefix_len,
                              name_len - prefix_len - suffix_len,
                              value, &(data->columns.filtered))) {
      grn_table_cursor_close(ctx, cursor);
      return GRN_FALSE;
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return GRN_TRUE;
}

static grn_bool
grn_select_data_fill_columns(grn_ctx *ctx,
                             grn_user_data *user_data,
                             grn_select_data *data)
{
  if (!grn_select_data_fill_columns_collect(ctx, user_data, data)) {
    return GRN_FALSE;
  }

  if (!data->columns.filtered) {
    return GRN_TRUE;
  }

  if (!grn_column_data_collect(ctx, user_data, data->columns.filtered, NULL, 0)) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_bool
grn_drilldown_data_fill_columns_collect(grn_ctx *ctx,
                                        grn_user_data *user_data,
                                        grn_drilldown_data *data,
                                        const char *drilldown_label)
{
  grn_obj *vars;
  grn_table_cursor *cursor;
  const char *prefix = "column[";
  size_t prefix_len;
  const char *suffix = "].stage";
  size_t suffix_len;
  size_t drilldown_label_len = strlen(drilldown_label);

  vars = grn_plugin_proc_get_vars(ctx, user_data);
  cursor = grn_table_cursor_open(ctx, vars, NULL, 0, NULL, 0, 0, -1, 0);
  if (!cursor) {
    return GRN_FALSE;
  }

  prefix_len = strlen(prefix);
  suffix_len = strlen(suffix);
  while (grn_table_cursor_next(ctx, cursor)) {
    void *key;
    char *name;
    int name_len;
    void *value_raw;
    grn_obj *value;

    name_len = grn_table_cursor_get_key(ctx, cursor, &key);
    name = key;

    name += drilldown_label_len;
    name_len -= drilldown_label_len;

    if (name_len < prefix_len + suffix_len + 1) {
      continue;
    }

    if (memcmp(prefix, name, prefix_len) != 0) {
      continue;
    }

    if (memcmp(suffix, name + (name_len - suffix_len), suffix_len) != 0) {
      continue;
    }

    grn_table_cursor_get_value(ctx, cursor, &value_raw);
    value = value_raw;

    if (!grn_column_data_init(ctx,
                              name + prefix_len,
                              name_len - prefix_len - suffix_len,
                              value, &(data->columns.initial))) {
      grn_table_cursor_close(ctx, cursor);
      return GRN_FALSE;
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return GRN_TRUE;
}

static grn_bool
grn_drilldown_data_fill_columns(grn_ctx *ctx,
                                grn_user_data *user_data,
                                grn_drilldown_data *data,
                                const char *drilldown_label)
{
  if (!grn_drilldown_data_fill_columns_collect(ctx, user_data, data,
                                               drilldown_label)) {
    return GRN_FALSE;
  }

  if (!data->columns.initial) {
    return GRN_TRUE;
  }

  if (!grn_column_data_collect(ctx, user_data, data->columns.initial,
                               drilldown_label, strlen(drilldown_label))) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_bool
grn_select_data_fill_drilldown_labels(grn_ctx *ctx,
                                      grn_user_data *user_data,
                                      grn_select_data *data)
{
  grn_obj *vars;
  grn_table_cursor *cursor;
  const char *prefix = "drilldown[";
  int prefix_len;

  vars = grn_plugin_proc_get_vars(ctx, user_data);
  data->drilldown_labels = grn_table_create(ctx, NULL, 0, NULL,
                                            GRN_OBJ_TABLE_HASH_KEY,
                                            grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                            NULL);
  if (!data->drilldown_labels) {
    return GRN_FALSE;
  }

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
      grn_table_add(ctx,
                    data->drilldown_labels,
                    name + prefix_len,
                    label_len,
                    NULL);
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return GRN_TRUE;
}

static grn_bool
grn_select_data_fill_drilldowns(grn_ctx *ctx,
                                grn_user_data *user_data,
                                grn_select_data *data)
{
  grn_obj *drilldown;

  drilldown = grn_plugin_proc_get_var(ctx, user_data, "drilldown", -1);
  if (GRN_TEXT_LEN(drilldown) > 0) {
    grn_drilldown_data *drilldown_data;
    data->n_drilldowns = 1;
    data->drilldowns = GRN_PLUGIN_MALLOCN(ctx,
                                          grn_drilldown_data,
                                          data->n_drilldowns);
    if (!data->drilldowns) {
      return GRN_FALSE;
    }
    drilldown_data = &(data->drilldowns[0]);
    drilldown_data->label.value = NULL;
    drilldown_data->label.length = 0;
    drilldown_data->columns.initial = NULL;
    grn_drilldown_data_fill(ctx,
                            drilldown_data,
                            drilldown,
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_sortby", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_output_columns", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_offset", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_limit", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_calc_types", -1),
                            grn_plugin_proc_get_var(ctx, user_data,
                                                    "drilldown_calc_target", -1),
                            NULL);
  } else {
    unsigned int i;
    grn_table_cursor *cursor = NULL;

    if (!grn_select_data_fill_drilldown_labels(ctx, user_data, data)) {
      return GRN_FALSE;
    }

    cursor = grn_table_cursor_open(ctx, data->drilldown_labels,
                                   NULL, 0, NULL, 0, 0, -1, 0);
    if (!cursor) {
      return GRN_FALSE;
    }

    data->n_drilldowns = grn_table_size(ctx, data->drilldown_labels);
    data->drilldowns = GRN_PLUGIN_MALLOCN(ctx,
                                          grn_drilldown_data,
                                          data->n_drilldowns);
    if (!data->drilldowns) {
      grn_table_cursor_close(ctx, cursor);
      return GRN_FALSE;
    }

    i = 0;
    while (grn_table_cursor_next(ctx, cursor)) {
      grn_drilldown_data *drilldown = &(data->drilldowns[i]);
        const char *label;
        int label_len;
        char drilldown_label[GRN_TABLE_MAX_KEY_SIZE];
        char key_name[GRN_TABLE_MAX_KEY_SIZE];
        grn_obj *keys;
        grn_obj *sortby;
        grn_obj *output_columns;
        grn_obj *offset;
        grn_obj *limit;
        grn_obj *calc_types;
        grn_obj *calc_target;
        grn_obj *table;

        label_len = grn_table_cursor_get_key(ctx, cursor, (void **)&label);
        drilldown->label.value = label;
        drilldown->label.length = label_len;
        grn_snprintf(drilldown_label,
                     GRN_TABLE_MAX_KEY_SIZE,
                     GRN_TABLE_MAX_KEY_SIZE,
                     "drilldown[%.*s].", label_len, label);

#define GET_VAR(name)                                                   \
        grn_snprintf(key_name,                                          \
                     GRN_TABLE_MAX_KEY_SIZE,                            \
                     GRN_TABLE_MAX_KEY_SIZE,                            \
                     "%s" # name, drilldown_label);                     \
        name = grn_plugin_proc_get_var(ctx, user_data, key_name, -1);

        GET_VAR(keys);
        GET_VAR(sortby);
        GET_VAR(output_columns);
        GET_VAR(offset);
        GET_VAR(limit);
        GET_VAR(calc_types);
        GET_VAR(calc_target);
        GET_VAR(table);

#undef GET_VAR

        grn_drilldown_data_fill_columns(ctx, user_data, drilldown, drilldown_label);
        grn_drilldown_data_fill(ctx, drilldown,
                                keys, sortby, output_columns, offset, limit,
                                calc_types, calc_target, table);
        i++;
    }
    grn_table_cursor_close(ctx, cursor);
  }

  return GRN_TRUE;
}

static grn_obj *
command_select(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_select_data data;

  data.drilldowns = NULL;
  data.n_drilldowns = 0;
  data.drilldown_labels = NULL;

  data.columns.filtered = NULL;

  data.table.value = grn_plugin_proc_get_var_string(ctx, user_data,
                                                    "table", -1,
                                                    &(data.table.length));
  data.match_columns.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "match_columns", -1,
                                   &(data.match_columns.length));
  data.query.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "query", -1,
                                   &(data.query.length));
  data.filter.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "filter", -1,
                                   &(data.filter.length));
  data.scorer.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "scorer", -1,
                                   &(data.scorer.length));
  data.sortby.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "sortby", -1,
                                   &(data.sortby.length));
  data.output_columns.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "output_columns", -1,
                                   &(data.output_columns.length));
  if (!data.output_columns.value) {
    data.output_columns.value = GRN_SELECT_DEFAULT_OUTPUT_COLUMNS;
    data.output_columns.length = strlen(GRN_SELECT_DEFAULT_OUTPUT_COLUMNS);
  }
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
  data.query_expander.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "query_expander", -1,
                                   &(data.query_expander.length));
  if (!data.query_expander.value) {
    data.query_expander.value =
      grn_plugin_proc_get_var_string(ctx, user_data,
                                     "query_expansion", -1,
                                     &(data.query_expander.length));
  }
  data.query_flags.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "query_flags", -1,
                                   &(data.query_flags.length));

  data.adjuster.value =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "adjuster", -1,
                                   &(data.adjuster.length));

  if (!grn_select_data_fill_drilldowns(ctx, user_data, &data)) {
    goto exit;
  }

  if (!grn_select_data_fill_columns(ctx, user_data, &data)) {
    goto exit;
  }

  grn_select(ctx, &data);

exit :
  if (data.columns.filtered) {
    grn_hash_close(ctx, data.columns.filtered);
  }

  if (data.drilldowns) {
    int i;
    for (i = 0; i < data.n_drilldowns; i++) {
      grn_drilldown_data *drilldown = &(data.drilldowns[i]);
      if (drilldown->columns.initial) {
        grn_hash_close(ctx, drilldown->columns.initial);
      }
    }
    GRN_PLUGIN_FREE(ctx, data.drilldowns);
  }
  if (data.drilldown_labels) {
    grn_obj_unlink(ctx, data.drilldown_labels);
  }

  return NULL;
}

#define N_VARS 23
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
  grn_plugin_expr_var_init(ctx, &(vars[6]), "sortby", -1);
  grn_plugin_expr_var_init(ctx, &(vars[7]), "output_columns", -1);
  grn_plugin_expr_var_init(ctx, &(vars[8]), "offset", -1);
  grn_plugin_expr_var_init(ctx, &(vars[9]), "limit", -1);
  grn_plugin_expr_var_init(ctx, &(vars[10]), "drilldown", -1);
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
