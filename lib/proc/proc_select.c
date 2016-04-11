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

#include "../grn_ts.h"

#include <groonga/plugin.h>

#define GRN_SELECT_INTERNAL_VAR_MATCH_COLUMNS "$match_columns"

#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key, _nsubrecs"

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

  if (format.expression) {
    grn_obj *condition_ptr;
    condition_ptr =
      grn_expr_get_or_add_var(ctx, format.expression,
                              GRN_SELECT_INTERNAL_VAR_CONDITION,
                              strlen(GRN_SELECT_INTERNAL_VAR_CONDITION));
    GRN_PTR_INIT(condition_ptr, 0, GRN_DB_OBJECT);
    GRN_PTR_SET(ctx, condition_ptr, condition);
  }
  GRN_OUTPUT_OBJ(res, &format);
  GRN_OBJ_FORMAT_FIN(ctx, &format);
}

typedef struct {
  const char *label;
  unsigned int label_len;
  const char *keys;
  unsigned int keys_len;
  const char *sortby;
  unsigned int sortby_len;
  const char *output_columns;
  unsigned int output_columns_len;
  int offset;
  int limit;
  grn_table_group_flags calc_types;
  const char *calc_target_name;
  unsigned int calc_target_name_len;
  const char *table_name;
  unsigned int table_name_len;
} drilldown_info;

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
drilldown_info_fill(grn_ctx *ctx,
                    drilldown_info *drilldown,
                    grn_obj *keys,
                    grn_obj *sortby,
                    grn_obj *output_columns,
                    grn_obj *offset,
                    grn_obj *limit,
                    grn_obj *calc_types,
                    grn_obj *calc_target,
                    grn_obj *table)
{
  if (keys) {
    drilldown->keys = GRN_TEXT_VALUE(keys);
    drilldown->keys_len = GRN_TEXT_LEN(keys);
  } else {
    drilldown->keys = NULL;
    drilldown->keys_len = 0;
  }

  if (sortby) {
    drilldown->sortby = GRN_TEXT_VALUE(sortby);
    drilldown->sortby_len = GRN_TEXT_LEN(sortby);
  } else {
    drilldown->sortby = NULL;
    drilldown->sortby_len = 0;
  }

  if (output_columns) {
    drilldown->output_columns = GRN_TEXT_VALUE(output_columns);
    drilldown->output_columns_len = GRN_TEXT_LEN(output_columns);
  } else {
    drilldown->output_columns = NULL;
    drilldown->output_columns_len = 0;
  }
  if (!drilldown->output_columns_len) {
    drilldown->output_columns = DEFAULT_DRILLDOWN_OUTPUT_COLUMNS;
    drilldown->output_columns_len = strlen(DEFAULT_DRILLDOWN_OUTPUT_COLUMNS);
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

  if (calc_target && GRN_TEXT_LEN(calc_target)) {
    drilldown->calc_target_name = GRN_TEXT_VALUE(calc_target);
    drilldown->calc_target_name_len = GRN_TEXT_LEN(calc_target);
  } else {
    drilldown->calc_target_name = NULL;
    drilldown->calc_target_name_len = 0;
  }

  if (table && GRN_TEXT_LEN(table)) {
    drilldown->table_name = GRN_TEXT_VALUE(table);
    drilldown->table_name_len = GRN_TEXT_LEN(table);
  } else {
    drilldown->table_name = NULL;
    drilldown->table_name_len = 0;
  }
}

static void
grn_select_drilldown(grn_ctx *ctx, grn_obj *table,
                     grn_table_sort_key *keys, uint32_t n_keys,
                     drilldown_info *drilldown)
{
  uint32_t i;
  for (i = 0; i < n_keys; i++) {
    grn_table_group_result g = {NULL, 0, 0, 1, GRN_TABLE_GROUP_CALC_COUNT, 0};
    uint32_t n_hits;
    int offset;
    int limit;

    if (drilldown->calc_target_name) {
      g.calc_target = grn_obj_column(ctx, table,
                                     drilldown->calc_target_name,
                                     drilldown->calc_target_name_len);
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

    if (drilldown->sortby_len) {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_key_from_str(ctx,
                                              drilldown->sortby,
                                              drilldown->sortby_len,
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
                          drilldown->output_columns,
                          drilldown->output_columns_len,
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
                      drilldown->output_columns,
                      drilldown->output_columns_len,
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
drilldown_info_tsort_visit(grn_ctx *ctx, grn_obj *labels,
                           tsort_status *statuses,
                           drilldown_info *drilldowns,
                           uint32_t index, grn_obj *indexes)
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
      drilldown_info *drilldown = &(drilldowns[index]);
      if (drilldown->table_name) {
        grn_id dependent_id;
        dependent_id = grn_table_get(ctx, labels,
                                     drilldown->table_name,
                                     drilldown->table_name_len);
        if (dependent_id != GRN_ID_NIL) {
          uint32_t dependent_index = dependent_id - 1;
          cycled = drilldown_info_tsort_visit(ctx, labels, statuses, drilldowns,
                                              dependent_index, indexes);
          if (cycled) {
            GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                             "[select][drilldown][%.*s][table] "
                             "cycled dependency: <%.*s>",
                             (int)(drilldown->label_len),
                             drilldown->label,
                             (int)(drilldown->table_name_len),
                             drilldown->table_name);
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
drilldown_info_tsort_body(grn_ctx *ctx, grn_obj *labels,
                          tsort_status *statuses,
                          drilldown_info *drilldowns, unsigned int n_drilldowns,
                          grn_obj *indexes)
{
  grn_bool succeeded = GRN_TRUE;
  unsigned int i;
  for (i = 0; i < n_drilldowns; i++) {
    drilldown_info *drilldown = &(drilldowns[i]);
    grn_id id;
    id = grn_table_get(ctx, labels,
                       drilldown->label, drilldown->label_len);
    if (id != GRN_ID_NIL) {
      uint32_t index = id - 1;
      if (drilldown_info_tsort_visit(ctx, labels, statuses, drilldowns,
                                     index, indexes)) {
        succeeded = GRN_FALSE;
        break;
      }
    }
  }
  return succeeded;
}

static void
drilldown_info_tsort_init(grn_ctx *ctx, grn_obj *labels,
                          tsort_status *statuses,
                          drilldown_info *drilldowns, unsigned int n_drilldowns)
{
  unsigned int i;
  for (i = 0; i < n_drilldowns; i++) {
    statuses[i] = TSORT_STATUS_NOT_VISITED;
  }
}

static grn_bool
drilldown_info_tsort(grn_ctx *ctx, grn_obj *labels,
                     drilldown_info *drilldowns, unsigned int n_drilldowns,
                     grn_obj *indexes)
{
  tsort_status *statuses;
  grn_bool succeeded;

  statuses = GRN_PLUGIN_MALLOCN(ctx, tsort_status, n_drilldowns);
  if (!statuses) {
    return GRN_FALSE;
  }

  drilldown_info_tsort_init(ctx, labels, statuses, drilldowns, n_drilldowns);
  succeeded = drilldown_info_tsort_body(ctx, labels, statuses,
                                        drilldowns, n_drilldowns, indexes);
  GRN_PLUGIN_FREE(ctx, statuses);
  return succeeded;
}

static grn_table_group_result *
grn_select_drilldowns_execute(grn_ctx *ctx,
                              grn_obj *table,
                              drilldown_info *drilldowns,
                              unsigned int n_drilldowns,
                              grn_obj *labels,
                              grn_obj *condition)
{
  grn_table_group_result *results = NULL;
  grn_obj tsorted_indexes;
  unsigned int i;

  if (!labels) {
    return NULL;
  }

  GRN_UINT32_INIT(&tsorted_indexes, GRN_OBJ_VECTOR);
  if (!drilldown_info_tsort(ctx, labels,
                            drilldowns, n_drilldowns, &tsorted_indexes)) {
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
    drilldown_info *drilldown;
    grn_table_group_result *result;

    index = GRN_UINT32_VALUE_AT(&tsorted_indexes, i);
    drilldown = drilldowns + index;
    result = results + index;

    result->limit = 1;
    result->flags = GRN_TABLE_GROUP_CALC_COUNT;
    result->op = 0;
    result->max_n_subrecs = 0;
    result->calc_target = NULL;

    if (drilldown->table_name) {
      grn_id dependent_id;
      dependent_id = grn_table_get(ctx,
                                   labels,
                                   drilldown->table_name,
                                   drilldown->table_name_len);
      if (dependent_id == GRN_ID_NIL) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[select][drilldown][%.*s][table] "
                         "nonexistent label: <%.*s>",
                         (int)(drilldown->label_len),
                         drilldown->label,
                         (int)(drilldown->table_name_len),
                         drilldown->table_name);
        break;
      } else {
        uint32_t dependent_index = dependent_id - 1;
        target_table = results[dependent_index].table;
      }
    }

    keys = grn_table_sort_key_from_str(ctx,
                                       drilldown->keys,
                                       drilldown->keys_len,
                                       target_table, &n_keys);

    if (!keys && !drilldown->calc_target_name) {
      GRN_PLUGIN_CLEAR_ERROR(ctx);
      continue;
    }

    if (n_keys > 1) {
      result->max_n_subrecs = 1;
      result->key_begin = 0;
      result->key_end = n_keys - 1;
    }
    if (drilldown->calc_target_name) {
      result->calc_target = grn_obj_column(ctx, target_table,
                                           drilldown->calc_target_name,
                                           drilldown->calc_target_name_len);
    }
    if (result->calc_target) {
      result->flags |= drilldown->calc_types;
    }

    grn_table_group(ctx, target_table, keys, n_keys, result, 1);
    grn_table_sort_key_close(ctx, keys, n_keys);
  }

exit :
  GRN_OBJ_FIN(ctx, &tsorted_indexes);

  return results;
}

static void
grn_select_drilldowns_output(grn_ctx *ctx,
                             grn_obj *table,
                             drilldown_info *drilldowns,
                             unsigned int n_drilldowns,
                             grn_obj *condition,
                             grn_table_group_result *results)
{
  unsigned int i;
  unsigned int n_available_results = 0;

  for (i = 0; i < n_drilldowns; i++) {
    grn_table_group_result *result = results + i;

    if (result->table) {
      n_available_results++;
    }
  }

  GRN_OUTPUT_MAP_OPEN("DRILLDOWNS", n_available_results);
  for (i = 0; i < n_drilldowns; i++) {
    drilldown_info *drilldown = drilldowns + i;
    grn_table_group_result *result = results + i;
    uint32_t n_hits;
    int offset;
    int limit;

    if (!result->table) {
      continue;
    }

    GRN_OUTPUT_STR(drilldown->label, drilldown->label_len);

    n_hits = grn_table_size(ctx, result->table);

    offset = drilldown->offset;
    limit = drilldown->limit;
    grn_normalize_offset_and_limit(ctx, n_hits, &offset, &limit);

    if (drilldown->sortby_len) {
      grn_table_sort_key *sort_keys;
      uint32_t n_sort_keys;
      sort_keys = grn_table_sort_key_from_str(ctx,
                                              drilldown->sortby,
                                              drilldown->sortby_len,
                                              result->table, &n_sort_keys);
      if (sort_keys) {
        grn_obj *sorted;
        sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                  NULL, result->table);
        if (sorted) {
          grn_table_sort(ctx, result->table, offset, limit,
                         sorted, sort_keys, n_sort_keys);
          grn_proc_select_output_columns(ctx, sorted, n_hits, 0, limit,
                                         drilldown->output_columns,
                                         drilldown->output_columns_len,
                                         condition);
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, sort_keys, n_sort_keys);
      }
    } else {
      grn_proc_select_output_columns(ctx, result->table, n_hits, offset, limit,
                                     drilldown->output_columns,
                                     drilldown->output_columns_len,
                                     condition);
    }

    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "drilldown(%d)[%.*s]", n_hits,
                  (int)(drilldown->label_len), drilldown->label);
  }
  GRN_OUTPUT_MAP_CLOSE();
}

static void
grn_select_drilldowns(grn_ctx *ctx, grn_obj *table,
                      drilldown_info *drilldowns, unsigned int n_drilldowns,
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
    unsigned int i;

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
grn_select(grn_ctx *ctx, const char *table, unsigned int table_len,
           const char *match_columns, unsigned int match_columns_len,
           const char *query, unsigned int query_len,
           const char *filter, unsigned int filter_len,
           const char *scorer, unsigned int scorer_len,
           const char *sortby, unsigned int sortby_len,
           const char *output_columns, unsigned int output_columns_len,
           int offset, int limit,
           drilldown_info *drilldowns,
           unsigned int n_drilldowns,
           grn_obj *drilldown_labels,
           const char *cache, unsigned int cache_len,
           const char *match_escalation_threshold, unsigned int match_escalation_threshold_len,
           const char *query_expander, unsigned int query_expander_len,
           const char *query_flags, unsigned int query_flags_len,
           const char *adjuster, unsigned int adjuster_len)
{
  uint32_t nkeys, nhits;
  uint16_t cacheable = 1, taintable = 0;
  grn_table_sort_key *keys;
  grn_obj *outbuf = ctx->impl->output.buf;
  grn_content_type output_type = ctx->impl->output.type;
  grn_obj *table_, *match_columns_ = NULL, *cond = NULL, *scorer_, *res = NULL, *sorted;
  char cache_key[GRN_CACHE_MAX_KEY_SIZE];
  uint32_t cache_key_size;
  long long int threshold, original_threshold = 0;
  grn_cache *cache_obj = grn_cache_current_get(ctx);

  {
    const char *query_end = query + query_len;
    int space_len;
    while (query < query_end) {
      space_len = grn_isspace(query, ctx->encoding);
      if (space_len == 0) {
        break;
      }
      query += space_len;
      query_len -= space_len;
    }
  }

  cache_key_size = table_len + 1 + match_columns_len + 1 + query_len + 1 +
    filter_len + 1 + scorer_len + 1 + sortby_len + 1 + output_columns_len + 1 +
    match_escalation_threshold_len + 1 +
    query_expander_len + 1 + query_flags_len + 1 + adjuster_len + 1 +
    sizeof(grn_content_type) + sizeof(int) * 2 +
    sizeof(grn_command_version) +
    sizeof(grn_bool);
  {
    unsigned int i;
    for (i = 0; i < n_drilldowns; i++) {
      drilldown_info *drilldown = &(drilldowns[i]);
      cache_key_size +=
        drilldown->keys_len + 1 +
        drilldown->sortby_len + 1 +
        drilldown->output_columns_len + 1 +
        sizeof(int) * 2;
    }
  }
  if (cache_key_size <= GRN_CACHE_MAX_KEY_SIZE) {
    grn_obj *cache_value;
    char *cp = cache_key;
    grn_memcpy(cp, table, table_len);
    cp += table_len; *cp++ = '\0';
    grn_memcpy(cp, match_columns, match_columns_len);
    cp += match_columns_len; *cp++ = '\0';
    grn_memcpy(cp, query, query_len);
    cp += query_len; *cp++ = '\0';
    grn_memcpy(cp, filter, filter_len);
    cp += filter_len; *cp++ = '\0';
    grn_memcpy(cp, scorer, scorer_len);
    cp += scorer_len; *cp++ = '\0';
    grn_memcpy(cp, sortby, sortby_len);
    cp += sortby_len; *cp++ = '\0';
    grn_memcpy(cp, output_columns, output_columns_len);
    cp += output_columns_len; *cp++ = '\0';
    {
      unsigned int i;
      for (i = 0; i < n_drilldowns; i++) {
        drilldown_info *drilldown = &(drilldowns[i]);
        grn_memcpy(cp, drilldown->keys, drilldown->keys_len);
        cp += drilldown->keys_len; *cp++ = '\0';
        grn_memcpy(cp, drilldown->sortby, drilldown->sortby_len);
        cp += drilldown->sortby_len; *cp++ = '\0';
        grn_memcpy(cp, drilldown->output_columns, drilldown->output_columns_len);
        cp += drilldown->output_columns_len; *cp++ = '\0';
      }
    }
    grn_memcpy(cp, match_escalation_threshold, match_escalation_threshold_len);
    cp += match_escalation_threshold_len; *cp++ = '\0';
    grn_memcpy(cp, query_expander, query_expander_len);
    cp += query_expander_len; *cp++ = '\0';
    grn_memcpy(cp, query_flags, query_flags_len);
    cp += query_flags_len; *cp++ = '\0';
    grn_memcpy(cp, adjuster, adjuster_len);
    cp += adjuster_len; *cp++ = '\0';
    grn_memcpy(cp, &output_type, sizeof(grn_content_type));
    cp += sizeof(grn_content_type);
    grn_memcpy(cp, &offset, sizeof(int));
    cp += sizeof(int);
    grn_memcpy(cp, &limit, sizeof(int));
    cp += sizeof(int);
    grn_memcpy(cp, &(ctx->impl->command_version), sizeof(grn_command_version));
    cp += sizeof(grn_command_version);
    grn_memcpy(cp, &(ctx->impl->output.is_pretty), sizeof(grn_bool));
    cp += sizeof(grn_bool);
    {
      unsigned int i;
      for (i = 0; i < n_drilldowns; i++) {
        drilldown_info *drilldown = &(drilldowns[i]);
        grn_memcpy(cp, &(drilldown->offset), sizeof(int));
        cp += sizeof(int);
        grn_memcpy(cp, &(drilldown->limit), sizeof(int));
        cp += sizeof(int);
      }
    }
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
  if (match_escalation_threshold_len) {
    const char *end, *rest;
    original_threshold = grn_ctx_get_match_escalation_threshold(ctx);
    end = match_escalation_threshold + match_escalation_threshold_len;
    threshold = grn_atoll(match_escalation_threshold, end, &rest);
    if (end == rest) {
      grn_ctx_set_match_escalation_threshold(ctx, threshold);
    }
  }
  if ((table_ = grn_ctx_get(ctx, table, table_len))) {
    // match_columns_ = grn_obj_column(ctx, table_, match_columns, match_columns_len);
    if (filter_len && (filter[0] == '?') &&
        (ctx->impl->output.type == GRN_CONTENT_JSON)) {
      ctx->rc = grn_ts_select(ctx, table_, filter + 1, filter_len - 1,
                              scorer, scorer_len,
                              sortby, sortby_len,
                              output_columns, output_columns_len,
                              offset, limit);
      if (!ctx->rc && cacheable && cache_key_size <= GRN_CACHE_MAX_KEY_SIZE &&
          (!cache || cache_len != 2 || cache[0] != 'n' || cache[1] != 'o')) {
        grn_cache_update(ctx, cache_obj, cache_key, cache_key_size, outbuf);
      }
      goto exit;
    }
    if (query_len || filter_len) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, cond, v);
      if (cond) {
        if (match_columns_len) {
          GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, match_columns_, v);
          if (match_columns_) {
            grn_expr_parse(ctx, match_columns_, match_columns, match_columns_len,
                           NULL, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
            if (ctx->rc) {
              goto exit;
            }
          } else {
            /* todo */
          }
        }
        if (query_len) {
          grn_expr_flags flags;
          grn_obj query_expander_buf;
          flags = GRN_EXPR_SYNTAX_QUERY;
          if (query_flags_len) {
            flags |= grn_parse_query_flags(ctx, query_flags, query_flags_len);
            if (ctx->rc) {
              goto exit;
            }
          } else {
            flags |= GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN;
          }
          GRN_TEXT_INIT(&query_expander_buf, 0);
          if (query_expander_len) {
            grn_rc rc;
            rc = grn_proc_syntax_expand_query(ctx,
                                              query, query_len, flags,
                                              query_expander, query_expander_len,
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
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND, flags);
          GRN_OBJ_FIN(ctx, &query_expander_buf);
          if (!ctx->rc && filter_len) {
            grn_expr_parse(ctx, cond, filter, filter_len,
                           match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
            if (!ctx->rc) { grn_expr_append_op(ctx, cond, GRN_OP_AND, 2); }
          }
        } else {
          grn_expr_parse(ctx, cond, filter, filter_len,
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND,
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
        if (!ctx->rc) { res = grn_table_select(ctx, table_, cond, NULL, GRN_OP_OR); }
      } else {
        /* todo */
        ERRCLR(ctx);
      }
    } else {
      res = table_;
    }
    nhits = res ? grn_table_size(ctx, res) : 0;
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                  ":", "select(%d)", nhits);

    if (res) {
      uint32_t ngkeys;
      grn_table_sort_key *gkeys = NULL;
      int result_size = 1;
      if (!ctx->rc && n_drilldowns > 0) {
        if (n_drilldowns == 1 && !drilldowns[0].label) {
          gkeys = grn_table_sort_key_from_str(ctx,
                                              drilldowns[0].keys,
                                              drilldowns[0].keys_len,
                                              res, &ngkeys);
          if (gkeys) {
            result_size += ngkeys;
          }
        } else {
          result_size += 1;
        }
      }

      if (adjuster && adjuster_len) {
        grn_obj *adjuster_;
        grn_obj *v;
        GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, adjuster_, v);
        if (adjuster_ && v) {
          grn_rc rc;
          rc = grn_expr_parse(ctx, adjuster_, adjuster, adjuster_len, NULL,
                              GRN_OP_MATCH, GRN_OP_ADJUST,
                              GRN_EXPR_SYNTAX_ADJUSTER);
          if (rc) {
            grn_obj_unlink(ctx, adjuster_);
            goto exit;
          }
          cacheable *= ((grn_expr *)adjuster_)->cacheable;
          taintable += ((grn_expr *)adjuster_)->taintable;
          grn_select_apply_adjuster(ctx, table_, res, adjuster_);
          grn_obj_unlink(ctx, adjuster_);
        }
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                      ":", "adjust(%d)", nhits);
      }

      if (scorer && scorer_len) {
        grn_obj *v;
        GRN_EXPR_CREATE_FOR_QUERY(ctx, res, scorer_, v);
        if (scorer_ && v) {
          grn_table_cursor *tc;
          grn_expr_parse(ctx, scorer_, scorer, scorer_len, NULL, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
          cacheable *= ((grn_expr *)scorer_)->cacheable;
          taintable += ((grn_expr *)scorer_)->taintable;
          if ((tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, 0))) {
            grn_id id;
            while ((id = grn_table_cursor_next(ctx, tc)) != GRN_ID_NIL) {
              GRN_RECORD_SET(ctx, v, id);
              grn_expr_exec(ctx, scorer_, 0);
              if (ctx->rc) {
                break;
              }
            }
            grn_table_cursor_close(ctx, tc);
          }
          grn_obj_unlink(ctx, scorer_);
        }
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                      ":", "score(%d)", nhits);
      }

      GRN_OUTPUT_ARRAY_OPEN("RESULT", result_size);

      grn_normalize_offset_and_limit(ctx, nhits, &offset, &limit);

      if (sortby_len &&
          (keys = grn_table_sort_key_from_str(ctx, sortby, sortby_len, res, &nkeys))) {
        if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
          grn_table_sort(ctx, res, offset, limit, sorted, keys, nkeys);
          GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                        ":", "sort(%d)", limit);
          grn_proc_select_output_columns(ctx, sorted, nhits, 0, limit,
                                         output_columns, output_columns_len,
                                         cond);
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, keys, nkeys);
      } else {
        if (!ctx->rc) {
          grn_proc_select_output_columns(ctx, res, nhits, offset, limit,
                                         output_columns, output_columns_len,
                                         cond);
        }
      }
      GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                    ":", "output(%d)", limit);
      if (!ctx->rc) {
        if (gkeys) {
          drilldown_info *drilldown = &(drilldowns[0]);
          grn_select_drilldown(ctx, res, gkeys, ngkeys, drilldown);
        } else if (n_drilldowns > 0) {
          grn_select_drilldowns(ctx, res, drilldowns, n_drilldowns,
                                drilldown_labels, cond);
        }
      }
      if (gkeys) {
        grn_table_sort_key_close(ctx, gkeys, ngkeys);
      }
      if (res != table_) { grn_obj_unlink(ctx, res); }
    } else {
      GRN_OUTPUT_ARRAY_OPEN("RESULT", 0);
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    if (!ctx->rc && cacheable && cache_key_size <= GRN_CACHE_MAX_KEY_SIZE
        && (!cache || cache_len != 2 || *cache != 'n' || *(cache + 1) != 'o')) {
      grn_cache_update(ctx, cache_obj, cache_key, cache_key_size, outbuf);
    }
    if (taintable) { grn_db_touch(ctx, DB_OBJ(table_)->db); }
    grn_obj_unlink(ctx, table_);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid table name: <%.*s>", table_len, table);
  }
exit :
  if (match_escalation_threshold_len) {
    grn_ctx_set_match_escalation_threshold(ctx, original_threshold);
  }
  if (match_columns_) {
    grn_obj_unlink(ctx, match_columns_);
  }
  if (cond) {
    grn_obj_unlink(ctx, cond);
  }
  /* GRN_LOG(ctx, GRN_LOG_NONE, "%d", ctx->seqno); */
  return ctx->rc;
}

static void
proc_select_find_all_drilldown_labels(grn_ctx *ctx, grn_user_data *user_data,
                                      grn_obj *labels)
{
  grn_obj *vars = GRN_PROC_GET_VARS();
  grn_table_cursor *cursor;
  cursor = grn_table_cursor_open(ctx, vars, NULL, 0, NULL, 0, 0, -1, 0);
  if (cursor) {
#define N_SUFFIXES 3
    const char *prefix = "drilldown[";
    int prefix_len = strlen(prefix);
    const char *suffixes[N_SUFFIXES] = {"].keys", "].table", "].calc_target"};
    int suffix_len;
    while (grn_table_cursor_next(ctx, cursor)) {
      void *key;
      char *name;
      int name_len;
      name_len = grn_table_cursor_get_key(ctx, cursor, &key);
      name = key;
      suffix_len = 0;
      if (name_len >= prefix_len &&
          strncmp(prefix, name, prefix_len) == 0) {
        int i;
        for (i = 0; i < N_SUFFIXES; i++) {
          int len = strlen(suffixes[i]);
          if (name_len >= (prefix_len + 1 + len) &&
              strncmp(suffixes[i], name + name_len - len, len) == 0) {
            suffix_len = len;
            break;
          }
        }
        if (suffix_len > 0) {
          grn_table_add(ctx, labels,
                        name + prefix_len,
                        name_len - prefix_len - suffix_len,
                        NULL);
        }
      }
#undef N_SUFFIXES
    }
    grn_table_cursor_close(ctx, cursor);
  }
}

static grn_obj *
command_select(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
#define MAX_N_DRILLDOWNS 10
  const char *table;
  size_t table_len;
  const char *match_columns;
  size_t match_columns_len;
  const char *query;
  size_t query_len;
  const char *filter;
  size_t filter_len;
  const char *scorer;
  size_t scorer_len;
  const char *sortby;
  size_t sortby_len;
  const char *output_columns;
  size_t output_columns_len;
  int32_t offset;
  int32_t limit;
  grn_obj *drilldown;
  drilldown_info drilldowns[MAX_N_DRILLDOWNS];
  unsigned int n_drilldowns = 0;
  grn_obj *drilldown_labels = NULL;
  const char *cache;
  size_t cache_len;
  const char *match_escalation_threshold;
  size_t match_escalation_threshold_len;
  const char *query_flags;
  size_t query_flags_len;
  const char *query_expander;
  size_t query_expander_len;
  const char *adjuster;
  size_t adjuster_len;

  table = grn_plugin_proc_get_var_string(ctx, user_data,
                                         "table", -1,
                                         &table_len);
  match_columns = grn_plugin_proc_get_var_string(ctx, user_data,
                                                 "match_columns", -1,
                                                 &match_columns_len);
  query = grn_plugin_proc_get_var_string(ctx, user_data,
                                         "query", -1,
                                         &query_len);
  filter = grn_plugin_proc_get_var_string(ctx, user_data,
                                          "filter", -1,
                                          &filter_len);
  scorer = grn_plugin_proc_get_var_string(ctx, user_data,
                                          "scorer", -1,
                                          &scorer_len);
  sortby = grn_plugin_proc_get_var_string(ctx, user_data,
                                          "sortby", -1,
                                          &sortby_len);
  output_columns = grn_plugin_proc_get_var_string(ctx, user_data,
                                                  "output_columns", -1,
                                                  &output_columns_len);
  if (!output_columns) {
    output_columns = GRN_SELECT_DEFAULT_OUTPUT_COLUMNS;
    output_columns_len = strlen(GRN_SELECT_DEFAULT_OUTPUT_COLUMNS);
  }
  offset = grn_plugin_proc_get_var_int32(ctx, user_data,
                                         "offset", -1,
                                         0);
  limit = grn_plugin_proc_get_var_int32(ctx, user_data,
                                        "limit", -1,
                                        GRN_SELECT_DEFAULT_LIMIT);

  cache = grn_plugin_proc_get_var_string(ctx, user_data,
                                         "cache", -1,
                                         &cache_len);
  match_escalation_threshold =
    grn_plugin_proc_get_var_string(ctx, user_data,
                                   "match_escalation_threshold", -1,
                                   &match_escalation_threshold_len);
  query_expander = grn_plugin_proc_get_var_string(ctx, user_data,
                                                  "query_expander", -1,
                                                  &query_expander_len);
  if (!query_expander) {
    query_expander = grn_plugin_proc_get_var_string(ctx, user_data,
                                                    "query_expansion", -1,
                                                    &query_expander_len);
  }
  query_flags = grn_plugin_proc_get_var_string(ctx, user_data,
                                               "query_flags", -1,
                                               &query_flags_len);

  adjuster = grn_plugin_proc_get_var_string(ctx, user_data,
                                            "adjuster", -1,
                                            &adjuster_len);

  drilldown = grn_plugin_proc_get_var(ctx, user_data,
                                      "drilldown", -1);
  if (GRN_TEXT_LEN(drilldown) > 0) {
    drilldown_info *drilldown_info = &(drilldowns[0]);
    drilldown_info->label = NULL;
    drilldown_info->label_len = 0;
    drilldown_info_fill(ctx,
                        drilldown_info,
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
    n_drilldowns++;
  } else {
    unsigned int i;
    grn_table_cursor *cursor = NULL;
    drilldown_labels = grn_table_create(ctx, NULL, 0, NULL,
                                        GRN_OBJ_TABLE_HASH_KEY,
                                        grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                        NULL);
    if (drilldown_labels) {
      proc_select_find_all_drilldown_labels(ctx, user_data, drilldown_labels);
      cursor = grn_table_cursor_open(ctx, drilldown_labels,
                                     NULL, 0, NULL, 0, 0, -1, 0);
    }
    if (cursor) {
      i = 0;
      n_drilldowns = grn_table_size(ctx, drilldown_labels);
      while (grn_table_cursor_next(ctx, cursor)) {
        drilldown_info *drilldown = &(drilldowns[i]);
        const char *label;
        int label_len;
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
        drilldown->label = label;
        drilldown->label_len = label_len;

#define GET_VAR(name)\
        grn_snprintf(key_name,                                            \
                     GRN_TABLE_MAX_KEY_SIZE,                              \
                     GRN_TABLE_MAX_KEY_SIZE,                              \
                     "drilldown[%.*s]." # name, label_len, label);        \
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

        drilldown_info_fill(ctx, drilldown,
                            keys, sortby, output_columns, offset, limit,
                            calc_types, calc_target, table);
        i++;
      }
      grn_table_cursor_close(ctx, cursor);
    }
  }
  if (grn_select(ctx,
                 table, table_len,
                 match_columns, match_columns_len,
                 query, query_len,
                 filter, filter_len,
                 scorer, scorer_len,
                 sortby, sortby_len,
                 output_columns, output_columns_len,
                 offset, limit,
                 drilldowns, n_drilldowns,
                 drilldown_labels,
                 cache, cache_len,
                 match_escalation_threshold,
                 match_escalation_threshold_len,
                 query_expander, query_expander_len,
                 query_flags, query_flags_len,
                 adjuster, adjuster_len)) {
  }
  if (drilldown_labels) {
    grn_obj_unlink(ctx, drilldown_labels);
  }
#undef MAX_N_DRILLDOWNS

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
