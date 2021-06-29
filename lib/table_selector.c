/*
  Copyright(C) 2010-2018  Brazil
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

#include "grn.h"
#include "grn_expr.h"
#include "grn_expr_code.h"
#include "grn_expr_executor.h"
#include "grn_ii.h"
#include "grn_posting.h"
#include "grn_report.h"
#include "grn_scan_info.h"
#include "grn_selector.h"
#include "grn_table.h"
#include "grn_table_selector.h"
#include "grn_util.h"

#include <stdio.h>

static double grn_table_select_enough_filtered_ratio = 0.01;
static int64_t grn_table_select_max_n_enough_filtered_records = 1000;
static bool grn_table_select_min_id_skip_enable = true;
static bool grn_query_log_show_condition = true;

void
grn_table_selector_init_from_env(void)
{
  {
    char grn_table_select_enough_filtered_ratio_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO",
               grn_table_select_enough_filtered_ratio_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_table_select_enough_filtered_ratio_env[0]) {
      grn_table_select_enough_filtered_ratio =
        atof(grn_table_select_enough_filtered_ratio_env);
    }
  }

  {
    char grn_table_select_max_n_enough_filtered_records_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_SELECT_MAX_N_ENOUGH_FILTERED_RECORDS",
               grn_table_select_max_n_enough_filtered_records_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_table_select_max_n_enough_filtered_records_env[0]) {
      grn_table_select_max_n_enough_filtered_records =
        atoi(grn_table_select_max_n_enough_filtered_records_env);
    }
  }

  {
    char env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_SELECT_MIN_ID_SKIP_ENABLE",
               env,
               GRN_ENV_BUFFER_SIZE);
    if (env[0]) {
      if (strcmp(env, "no") == 0) {
        grn_table_select_min_id_skip_enable = false;
      } else {
        grn_table_select_min_id_skip_enable = true;
      }
    } else {
      grn_getenv("GRN_TABLE_SELECT_AND_MIN_SKIP_ENABLE",
                 env,
                 GRN_ENV_BUFFER_SIZE);
      if (env[0]) {
        if (strcmp(env, "no") == 0) {
          grn_table_select_min_id_skip_enable = false;
        } else {
          grn_table_select_min_id_skip_enable = true;
        }
      }
    }
  }

  {
    char grn_query_log_show_condition_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_QUERY_LOG_SHOW_CONDITION",
               grn_query_log_show_condition_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_query_log_show_condition_env, "no") == 0) {
      grn_query_log_show_condition = false;
    } else {
      grn_query_log_show_condition = true;
    }
  }
}

void
grn_table_selector_init(grn_ctx *ctx,
                        grn_table_selector *table_selector,
                        grn_obj *table,
                        grn_obj *expr,
                        grn_operator op)
{
  table_selector->table = table;
  table_selector->expr = expr;
  table_selector->op = op;
  table_selector->min_id = GRN_ID_NIL;
  table_selector->use_sequential_scan = false;
  table_selector->query_options = grn_expr_get_query_options(ctx, expr);
  table_selector->weight_factor = 1.0;
  table_selector->enough_filtered_ratio =
    grn_table_select_enough_filtered_ratio;
  table_selector->max_n_enough_filtered_records =
    grn_table_select_max_n_enough_filtered_records;
  grn_table_selector_data data = {0};
  table_selector->data = data;
}

void
grn_table_selector_fin(grn_ctx *ctx,
                       grn_table_selector *table_selector)
{
  if (table_selector->data.scanner) {
    grn_scanner_close(ctx, table_selector->data.scanner);
  }
}

grn_table_selector *
grn_table_selector_open(grn_ctx *ctx,
                        grn_obj *table,
                        grn_obj *expr,
                        grn_operator op)
{
  GRN_API_ENTER;
  grn_table_selector *table_selector = GRN_MALLOC(sizeof(grn_table_selector));
  if (table_selector) {
    grn_table_selector_init(ctx, table_selector, table, expr, op);
  }
  GRN_API_RETURN(table_selector);
}

grn_rc
grn_table_selector_close(grn_ctx *ctx,
                         grn_table_selector *table_selector)
{
  GRN_API_ENTER;
  if (table_selector) {
    grn_table_selector_fin(ctx, table_selector);
    GRN_FREE(table_selector);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_id
grn_table_selector_get_min_id(grn_ctx *ctx,
                              grn_table_selector *table_selector)
{
  return table_selector->min_id;
}

grn_rc
grn_table_selector_set_min_id(grn_ctx *ctx,
                              grn_table_selector *table_selector,
                              grn_id min_id)
{
  GRN_API_ENTER;
  table_selector->min_id = min_id;
  GRN_API_RETURN(ctx->rc);
}

bool
grn_table_selector_get_use_sequential_scan(grn_ctx *ctx,
                                           grn_table_selector *table_selector)
{
  return table_selector->use_sequential_scan;
}

grn_rc
grn_table_selector_set_use_sequential_scan(grn_ctx *ctx,
                                           grn_table_selector *table_selector,
                                           bool use)
{
  GRN_API_ENTER;
  table_selector->use_sequential_scan = use;
  GRN_API_RETURN(ctx->rc);
}

float
grn_table_selector_get_weight_factor(grn_ctx *ctx,
                                     grn_table_selector *table_selector)
{
  return table_selector->weight_factor;
}

grn_rc
grn_table_selector_set_weight_factor(grn_ctx *ctx,
                                     grn_table_selector *table_selector,
                                     float factor)
{
  GRN_API_ENTER;
  table_selector->weight_factor = factor;
  GRN_API_RETURN(ctx->rc);
}

double
grn_table_selector_get_enough_filtered_ratio(grn_ctx *ctx,
                                             grn_table_selector *table_selector)
{
  return table_selector->enough_filtered_ratio;
}

grn_rc
grn_table_selector_set_enough_filtered_ratio(grn_ctx *ctx,
                                             grn_table_selector *table_selector,
                                             double ratio)
{
  GRN_API_ENTER;
  table_selector->enough_filtered_ratio = ratio;
  GRN_API_RETURN(ctx->rc);
}

int64_t
grn_table_selector_get_max_n_enough_filtered_records(
  grn_ctx *ctx,
  grn_table_selector *table_selector)
{
  return table_selector->max_n_enough_filtered_records;
}

grn_rc
grn_table_selector_set_max_n_enough_filtered_records(
  grn_ctx *ctx,
  grn_table_selector *table_selector,
  int64_t n)
{
  GRN_API_ENTER;
  table_selector->max_n_enough_filtered_records = n;
  GRN_API_RETURN(ctx->rc);
}

static grn_obj *
selector_create_result_set(grn_ctx *ctx,
                           grn_table_selector *table_selector)
{
  grn_obj *result_set =
    grn_table_create(ctx,
                     NULL, 0,
                     NULL,
                     GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                     table_selector->table,
                     NULL);
  if (!result_set) {
    return NULL;
  }

  grn_obj *initial_result_set = table_selector->data.initial_result_set;
  grn_hash *columns = grn_table_all_columns(ctx, initial_result_set);
  if (!columns) {
    return result_set;
  }
  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    grn_id column_id = *((grn_id *)key);
    grn_obj *column = grn_ctx_at(ctx, column_id);
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size = grn_column_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
    grn_obj *range = grn_ctx_at(ctx, DB_OBJ(column)->range);
    grn_column_create(ctx,
                      result_set,
                      name,
                      name_size,
                      NULL,
                      grn_column_get_flags(ctx, column),
                      range);
    grn_obj_unref(ctx, range);
    grn_obj_unref(ctx, column);
  } GRN_HASH_EACH_END(ctx, cursor);
  return result_set;
}

static grn_inline int32_t
exec_result_to_score(grn_ctx *ctx, grn_obj *result, grn_obj *score_buffer)
{
  if (!result) {
    return 0;
  }

  switch (result->header.type) {
  case GRN_VOID :
    return 0;
  case GRN_BULK :
    switch (result->header.domain) {
    case GRN_DB_BOOL :
      return GRN_BOOL_VALUE(result) ? 1 : 0;
    case GRN_DB_INT32 :
      return GRN_INT32_VALUE(result);
    default :
      GRN_BULK_REWIND(score_buffer);
      if (grn_obj_cast(ctx, result, score_buffer, false) != GRN_SUCCESS) {
        return 1;
      }
      return GRN_INT32_VALUE(score_buffer);
    }
  case GRN_UVECTOR :
  case GRN_PVECTOR :
  case GRN_VECTOR :
    return 1;
  default :
    return 1; /* TODO: 1 is reasonable? */
  }
}

static grn_inline void
select_sequential_scan(grn_ctx *ctx,
                       grn_table_selector *table_selector,
                       grn_obj *result_set)
{
  grn_expr_executor executor;
  grn_expr_executor_init(ctx, &executor, table_selector->expr);
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  grn_obj score_buffer;
  GRN_INT32_INIT(&score_buffer, 0);
  switch (table_selector->op) {
  case GRN_OP_OR :
    GRN_TABLE_EACH_BEGIN(ctx, table_selector->table, cursor, id) {
      grn_obj *result = grn_expr_executor_exec(ctx, &executor, id);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      int32_t score = exec_result_to_score(ctx, result, &score_buffer);
      if (score > 0) {
        void *value;
        if (grn_hash_add(ctx,
                         (grn_hash *)result_set,
                         &id,
                         sizeof(grn_id),
                         &value,
                         NULL) != GRN_ID_NIL) {
          grn_rset_recinfo *ri = value;
          grn_table_add_subrec(ctx,
                               result_set,
                               ri,
                               score,
                               (grn_rset_posinfo *)&id,
                               1);
        }
      }
    } GRN_TABLE_EACH_END(ctx,cursor);
    break;
  case GRN_OP_AND :
    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)result_set, cursor, id) {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id record_id = *((grn_id *)key);
      grn_obj *result = grn_expr_executor_exec(ctx, &executor, record_id);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      int32_t score = exec_result_to_score(ctx, result, &score_buffer);
      if (score > 0) {
        void *value;
        grn_hash_cursor_get_value(ctx, cursor, &value);
        grn_rset_recinfo *ri = value;
        grn_table_add_subrec(ctx,
                             result_set,
                             ri,
                             score,
                             (grn_rset_posinfo *)&record_id,
                             1);
      } else {
        grn_hash_cursor_delete(ctx, cursor, NULL);
      }
    } GRN_HASH_EACH_END(ctx, cursor);
    break;
  case GRN_OP_AND_NOT :
    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)result_set, cursor, id) {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id record_id = *((grn_id *)key);
      grn_obj *result = grn_expr_executor_exec(ctx, &executor, record_id);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      int32_t score = exec_result_to_score(ctx, result, &score_buffer);
      if (score > 0) {
        grn_hash_cursor_delete(ctx, cursor, NULL);
      }
    } GRN_HASH_EACH_END(ctx, cursor);
    break;
  case GRN_OP_ADJUST :
    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)result_set, cursor, id) {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id record_id = *((grn_id *)key);
      grn_obj *result = grn_expr_executor_exec(ctx, &executor, record_id);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      int32_t score = exec_result_to_score(ctx, result, &score_buffer);
      if (score > 0) {
        void *value;
        grn_hash_cursor_get_value(ctx, cursor, &value);
        grn_rset_recinfo *ri = value;
        grn_table_add_subrec(ctx,
                             result_set,
                             ri,
                             score,
                             (grn_rset_posinfo *)&record_id,
                             1);
      }
    } GRN_HASH_EACH_END(ctx, cursor);
    break;
  default :
    break;
  }
  GRN_OBJ_FIN(ctx, &score_buffer);
  grn_expr_executor_fin(ctx, &executor);
}

static grn_inline void
select_index_report(grn_ctx *ctx, const char *tag, grn_obj *index)
{
  grn_report_index(ctx, "[table-selector][select]", tag, index);
}

static grn_inline void
select_index_not_used_report(grn_ctx *ctx,
                             const char *tag,
                             grn_obj *index,
                             const char *reason)
{
  grn_report_index_not_used(ctx, "[table-selector][select]", tag, index, reason);
}

static grn_inline bool
select_index_use_sequential_search(grn_ctx *ctx,
                                   grn_table_selector *table_selector,
                                   grn_obj *result_set,
                                   grn_operator logical_op,
                                   const char *tag,
                                   grn_obj *index)
{
  if (logical_op != GRN_OP_AND) {
    return false;
  }

  grn_obj *table = grn_ctx_at(ctx, result_set->header.domain);
  int n_records = grn_table_size(ctx, table);
  int n_filtered_records = grn_table_size(ctx, result_set);
  grn_obj_unref(ctx, table);
  double filtered_ratio;
  if (n_records == 0) {
    filtered_ratio = 1.0;
  } else {
    filtered_ratio = (double)n_filtered_records / (double)n_records;
  }

  if (filtered_ratio >= table_selector->enough_filtered_ratio) {
    return false;
  }

  if (n_filtered_records > table_selector->max_n_enough_filtered_records) {
    return false;
  }

  grn_obj reason;
  GRN_TEXT_INIT(&reason, 0);
  grn_text_printf(ctx, &reason,
                  "enough filtered: "
                  "%.2f%%(%d/%d) < %.2f%% && %d <= %" GRN_FMT_INT64D,
                  filtered_ratio * 100,
                  n_filtered_records,
                  n_records,
                  table_selector->enough_filtered_ratio * 100,
                  n_filtered_records,
                  table_selector->max_n_enough_filtered_records);
  GRN_TEXT_PUTC(ctx, &reason, '\0');
  select_index_not_used_report(ctx,
                               tag,
                               index,
                               GRN_TEXT_VALUE(&reason));
  GRN_OBJ_FIN(ctx, &reason);
  return true;
}

static grn_inline grn_id
select_index_resolve_key(grn_ctx *ctx,
                         grn_obj *domain,
                         grn_obj *key)
{
  if (GRN_OBJ_GET_DOMAIN(key) == DB_OBJ(domain)->id) {
    return GRN_RECORD_VALUE(key);
  } else if (GRN_OBJ_GET_DOMAIN(key) == domain->header.domain) {
    return grn_table_get(ctx,
                         domain,
                         GRN_BULK_HEAD(key),
                         GRN_BULK_VSIZE(key));
  } else {
    grn_id id = GRN_ID_NIL;
    grn_obj casted_key;
    GRN_OBJ_INIT(&casted_key, GRN_BULK, 0, domain->header.domain);
    if (grn_obj_cast(ctx, key, &casted_key, false) == GRN_SUCCESS) {
      id = grn_table_get(ctx,
                         domain,
                         GRN_BULK_HEAD(&casted_key),
                         GRN_BULK_VSIZE(&casted_key));
    }
    GRN_OBJ_FIN(ctx, &casted_key);
    return id;
  }
}

static grn_inline grn_rc
select_index_equal(grn_ctx *ctx,
                   grn_table_selector *table_selector,
                   grn_obj *index,
                   grn_operator op,
                   grn_obj *result_set,
                   grn_operator logical_op)
{
  scan_info *si = table_selector->data.scan_info;

  if (si->query->header.type != GRN_BULK) {
    /* We can't use index for non bulk value such as vector. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  if (GRN_BULK_VSIZE(si->query) == 0) {
    /* We can't use index for empty value. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  if (grn_obj_is_accessor(ctx, index)) {
    grn_search_optarg optarg;
    memset(&optarg, 0, sizeof(grn_search_optarg));
    optarg.mode = GRN_OP_EQUAL;
    optarg.vector_size = 1;
    rc = grn_obj_search(ctx,
                        index,
                        si->query,
                        result_set,
                        logical_op,
                        &optarg);
  } else {
    const char *tag = "[equal]";

    grn_obj *domain = grn_ctx_at(ctx, index->header.domain);
    if (domain) {
      bool optimizable = false;

      if (domain->header.domain == GRN_DB_SHORT_TEXT) {
        grn_obj *normalizer = NULL;
        grn_table_get_info(ctx, domain, NULL, NULL, NULL, &normalizer, NULL);
        if (normalizer == grn_ctx_get(ctx, "NormalizerAuto", -1)) {
          optimizable = true;
        }
      } else {
        optimizable = true;
      }
      if (optimizable &&
          select_index_use_sequential_search(ctx,
                                             table_selector,
                                             result_set,
                                             logical_op,
                                             tag,
                                             index)) {
        grn_obj_unref(ctx, domain);
        domain = NULL;
      }
    }

    if (domain) {
      select_index_report(ctx, tag, index);

      grn_id tid = select_index_resolve_key(ctx, domain, si->query);
      if (tid == GRN_ID_NIL) {
        rc = GRN_SUCCESS;
      } else {
        uint32_t sid = GRN_UINT32_VALUE_AT(&(si->sections), 0);
        float weight = GRN_FLOAT32_VALUE_AT(&(si->weights), 0);
        weight *= table_selector->weight_factor * si->weight_factor;
        grn_obj *index_cursor = grn_index_cursor_open(ctx,
                                                      NULL,
                                                      index,
                                                      GRN_ID_NIL,
                                                      GRN_ID_MAX,
                                                      0);
        if (index_cursor) {
          rc = grn_index_cursor_set_term_id(ctx, index_cursor, tid);
          if (rc == GRN_SUCCESS) {
            rc = grn_index_cursor_set_section_id(ctx, index_cursor, sid);
          }
          if (si->position.specified) {
            if (rc == GRN_SUCCESS) {
              rc = grn_index_cursor_set_start_position(ctx,
                                                       index_cursor,
                                                       si->position.start);
            }
          }
          if (rc == GRN_SUCCESS) {
            rc = grn_result_set_add_index_cursor(ctx,
                                                 (grn_hash *)result_set,
                                                 index_cursor,
                                                 1,
                                                 weight,
                                                 logical_op);
          }
          grn_obj_close(ctx, index_cursor);
        }
      }

      grn_obj_unref(ctx, domain);
    }
  }

  if (rc == GRN_SUCCESS) {
    grn_ii_resolve_sel_and(ctx, (grn_hash *)result_set, logical_op);
  }

  return rc;
}

static grn_inline grn_rc
select_index_not_equal(grn_ctx *ctx,
                       grn_table_selector *table_selector,
                       grn_obj *index,
                       grn_operator op,
                       grn_obj *result_set,
                       grn_operator logical_op)
{
  scan_info *si = table_selector->data.scan_info;

  if (GRN_BULK_VSIZE(si->query) == 0) {
    /* We can't use index for empty value. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  if (logical_op != GRN_OP_AND) {
    /* We can't use index for OR and AND_NOT. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  if (grn_obj_is_accessor(ctx, index)) {
    grn_obj dest;
    grn_accessor *a = (grn_accessor *)index;
    grn_id id;
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID :
      select_index_report(ctx,
                          "[not-equal][accessor][id]",
                          a->obj);
      GRN_UINT32_INIT(&dest, 0);
      if (!grn_obj_cast(ctx, si->query, &dest, false)) {
        id = GRN_UINT32_VALUE(&dest);
        if (id != GRN_ID_NIL) {
          if (id == grn_table_at(ctx, a->obj, id)) {
            grn_hash_delete(ctx,
                            (grn_hash *)result_set,
                            &id,
                            sizeof(grn_id),
                            NULL);
          }
        }
        rc = GRN_SUCCESS;
      }
      GRN_OBJ_FIN(ctx, &dest);
      break;
    case GRN_ACCESSOR_GET_KEY :
      select_index_report(ctx,
                          "[not-equal][accessor][key]",
                          a->obj);
      GRN_OBJ_INIT(&dest, GRN_BULK, 0, a->obj->header.domain);
      if (!grn_obj_cast(ctx, si->query, &dest, false)) {
        id = grn_table_get(ctx,
                           a->obj,
                           GRN_BULK_HEAD(&dest),
                           GRN_BULK_VSIZE(&dest));
        if (id != GRN_ID_NIL) {
          grn_hash_delete(ctx,
                          (grn_hash *)result_set,
                          &id,
                          sizeof(grn_id),
                          NULL);
        }
        rc = GRN_SUCCESS;
      }
      GRN_OBJ_FIN(ctx, &dest);
      break;
    }
  } else {
    grn_obj *domain = grn_ctx_at(ctx, index->header.domain);
    if (domain) {
      grn_id tid;

      select_index_report(ctx, "[not-equal]", index);

      tid = select_index_resolve_key(ctx, domain, si->query);
      if (tid == GRN_ID_NIL) {
        rc = GRN_SUCCESS;
      } else {
        grn_ii *ii = (grn_ii *)index;
        grn_ii_cursor *ii_cursor;

        uint32_t sid = GRN_UINT32_VALUE_AT(&(si->sections), 0);
        ii_cursor = grn_ii_cursor_open(ctx, ii, tid,
                                       GRN_ID_NIL, GRN_ID_MAX,
                                       ii->n_elements, 0);
        if (ii_cursor) {
          grn_posting *posting;
          while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
            if (!(sid == 0 || posting->sid == sid)) {
              continue;
            }

            if (si->position.specified) {
              while ((posting = grn_ii_cursor_next_pos(ctx, ii_cursor))) {
                if (posting->pos == si->position.start) {
                  break;
                }
              }
              if (!posting) {
                continue;
              }
            }

            grn_hash_delete(ctx, (grn_hash *)result_set,
                            &(posting->rid), sizeof(grn_id),
                            NULL);
          }
          grn_ii_cursor_close(ctx, ii_cursor);
          rc = GRN_SUCCESS;
        }
      }
    }
  }

  return rc;
}

static grn_inline grn_rc
select_index_fix_column_prefix(grn_ctx *ctx,
                               grn_table_selector *table_selector,
                               grn_obj *index,
                               grn_operator op,
                               grn_obj *result_set,
                               grn_operator logical_op)
{
  const char *tag = "[table][select][index][fix][column][prefix]";
  grn_obj *lexicon = grn_ctx_at(ctx, index->header.domain);
  if (!lexicon) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, index);
    ERR(GRN_OBJECT_CORRUPT,
        "%s index's domain is corrupt: %.*s",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return ctx->rc;
  }

  scan_info *si = table_selector->data.scan_info;
  grn_obj query_buffer;
  grn_obj *query;
  grn_table_cursor *table_cursor = NULL;
  grn_obj *index_cursor = NULL;
  if (lexicon->header.domain == si->query->header.domain ||
      (grn_type_id_is_text_family(ctx, lexicon->header.domain) &&
       grn_type_id_is_text_family(ctx, si->query->header.domain))) {
    query = si->query;
  } else {
    GRN_OBJ_INIT(&query_buffer,
                 GRN_BULK,
                 0,
                 lexicon->header.domain);
    query = &query_buffer;
    grn_rc rc = grn_obj_cast(ctx, si->query, &query_buffer, false);
    if (rc != GRN_SUCCESS) {
      grn_obj query_inspected;
      grn_obj domain_inspected;
      GRN_TEXT_INIT(&query_inspected, 0);
      GRN_TEXT_INIT(&domain_inspected, 0);
      grn_inspect(ctx, &query_inspected, si->query);
      grn_obj *domain = grn_ctx_at(ctx, query_buffer.header.domain);
      grn_inspect(ctx, &domain_inspected, domain);
      grn_obj_unref(ctx, domain);
      ERR(rc,
          "%s failed to cast query: "
          "<%.*s> -> <%.*s>",
          tag,
          (int)GRN_TEXT_LEN(&query_inspected),
          GRN_TEXT_VALUE(&query_inspected),
          (int)GRN_TEXT_LEN(&domain_inspected),
          GRN_TEXT_VALUE(&domain_inspected));
      GRN_OBJ_FIN(ctx, &domain_inspected);
      GRN_OBJ_FIN(ctx, &query_inspected);
      goto exit;
    }
  }

  table_cursor = grn_table_cursor_open(ctx,
                                       lexicon,
                                       GRN_BULK_HEAD(si->query),
                                       GRN_BULK_VSIZE(si->query),
                                       NULL,
                                       0,
                                       0,
                                       -1,
                                       GRN_CURSOR_PREFIX|GRN_CURSOR_GE);
  if (!table_cursor) {
    grn_obj query_inspected;
    GRN_TEXT_INIT(&query_inspected, 0);
    grn_inspect(ctx, &query_inspected, si->query);
    ERR(ctx->rc,
        "%s failed to create table cursor: <%.*s>",
        tag,
        (int)GRN_TEXT_LEN(&query_inspected),
        GRN_TEXT_VALUE(&query_inspected));
    GRN_OBJ_FIN(ctx, &query_inspected);
    goto exit;
  }

  index_cursor = grn_index_cursor_open(ctx,
                                       table_cursor,
                                       index,
                                       GRN_ID_NIL,
                                       GRN_ID_MAX,
                                       0);
  if (!index_cursor) {
    grn_obj query_inspected;
    GRN_TEXT_INIT(&query_inspected, 0);
    grn_inspect(ctx, &query_inspected, si->query);
    ERR(ctx->rc,
        "%s failed to create table cursor: <%.*s>",
        tag,
        (int)GRN_TEXT_LEN(&query_inspected),
        GRN_TEXT_VALUE(&query_inspected));
    GRN_OBJ_FIN(ctx, &query_inspected);
    goto exit;
  }
  select_index_report(ctx, "[prefix]", index);
  grn_result_set_add_index_cursor(ctx,
                                  (grn_hash *)result_set,
                                  index_cursor,
                                  1,
                                  1,
                                  logical_op);
exit :
  if (index_cursor) {
    grn_obj_close(ctx, index_cursor);
  }
  if (table_cursor) {
    grn_table_cursor_close(ctx, table_cursor);
  }
  if (query == &query_buffer) {
    GRN_OBJ_FIN(ctx, &query_buffer);
  }
  grn_obj_unref(ctx, lexicon);
  return ctx->rc;
}

static grn_inline grn_rc
select_index_fix(grn_ctx *ctx,
                 grn_table_selector *table_selector,
                 grn_obj *index,
                 grn_operator op,
                 grn_obj *result_set,
                 grn_operator logical_op)
{
  scan_info *si = table_selector->data.scan_info;
  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;

  if (grn_obj_is_accessor(ctx, index)) {
    grn_accessor *a = (grn_accessor *)index;
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID :
      /* todo */
      break;
    case GRN_ACCESSOR_GET_KEY :
      index = a->obj;
      break;
    }
  }

  if (grn_obj_is_table(ctx, index)) {
    grn_obj *table = index;
    if (op == GRN_OP_SUFFIX) {
      select_index_report(ctx, "[suffix][accessor][key]", table);
    } else {
      select_index_report(ctx, "[prefix][accessor][key]", table);
    }
    grn_obj dest;
    GRN_OBJ_INIT(&dest, GRN_BULK, 0, table->header.domain);
    if (!grn_obj_cast(ctx, si->query, &dest, false)) {
      grn_hash *pres;
      if ((pres = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY))) {
        grn_table_search(ctx,
                         index,
                         GRN_BULK_HEAD(&dest),
                         GRN_BULK_VSIZE(&dest),
                         op,
                         (grn_obj *)pres,
                         GRN_OP_OR);
        grn_posting_internal posting = {0};
        posting.sid = 1;
        posting.pos = 0;
        posting.weight_float = 1;
        GRN_HASH_EACH_BEGIN(ctx, pres, cursor, id) {
          void *key;
          grn_hash_cursor_get_key(ctx, cursor, &key);
          posting.rid = *((grn_id *)key);
          grn_ii_posting_add_float(ctx,
                                   (grn_posting *)(&posting),
                                   (grn_hash *)result_set,
                                   logical_op);
        } GRN_HASH_EACH_END(ctx, cursor);
        grn_hash_close(ctx, pres);
      }
      rc = GRN_SUCCESS;
      grn_ii_resolve_sel_and(ctx, (grn_hash *)result_set, logical_op);
    }
    GRN_OBJ_FIN(ctx, &dest);
  } else {
    if (op == GRN_OP_PREFIX) {
      return select_index_fix_column_prefix(ctx,
                                            table_selector,
                                            index,
                                            op,
                                            result_set,
                                            logical_op);
    }
    grn_obj *lexicon = grn_ctx_at(ctx, index->header.domain);
    if (lexicon) {
      grn_hash *keys;
      if ((keys = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY))) {
        select_index_report(ctx, "[suffix]", index);
        grn_table_search(ctx, lexicon,
                         GRN_BULK_HEAD(si->query),
                         GRN_BULK_VSIZE(si->query),
                         op, (grn_obj *)keys, GRN_OP_OR);
        GRN_HASH_EACH_BEGIN(ctx, keys, cursor, id) {
          void *key;
          grn_hash_cursor_get_key(ctx, cursor, &key);
          grn_id id = *((grn_id *)key);
          grn_ii_at(ctx, (grn_ii *)index, id, (grn_hash *)result_set, logical_op);
        } GRN_HASH_EACH_END(ctx, cursor);
        grn_hash_close(ctx, keys);
      }
      grn_obj_unref(ctx, lexicon);
    }
    rc = GRN_SUCCESS;
  }
  return rc;
}

static grn_inline grn_rc
select_index_match(grn_ctx *ctx,
                   grn_table_selector *table_selector,
                   grn_obj *index,
                   grn_operator op,
                   grn_obj *result_set,
                   grn_operator logical_op)
{
  grn_table_selector_data *data = &(table_selector->data);
  scan_info *si = data->scan_info;
  grn_search_optarg *options = &(data->search_options);
  return grn_obj_search(ctx, index, si->query, result_set, logical_op, options);
}

static grn_inline grn_rc
select_index_extract(grn_ctx *ctx,
                     grn_table_selector *table_selector,
                     grn_obj *index,
                     grn_operator op,
                     grn_obj *result_set,
                     grn_operator logical_op)
{
  if (!grn_obj_is_accessor(ctx, index)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = table_selector->data.scan_info;
  grn_accessor *a = (grn_accessor *)index;
  switch (a->action) {
  case GRN_ACCESSOR_GET_KEY :
    select_index_report(ctx,
                        "[term-extract][accessor][key]",
                        a->obj);
    grn_table_search(ctx,
                     a->obj,
                     GRN_TEXT_VALUE(si->query),
                     GRN_TEXT_LEN(si->query),
                     GRN_OP_TERM_EXTRACT,
                     result_set,
                     logical_op);
    rc = GRN_SUCCESS;
    break;
  }

  return rc;
}

static grn_inline grn_rc
select_index_call(grn_ctx *ctx,
                  grn_table_selector *table_selector,
                  grn_obj *index,
                  grn_operator op,
                  grn_obj *result_set,
                  grn_operator logical_op)
{
  grn_table_selector_data *data = &(table_selector->data);
  scan_info *si = data->scan_info;
  grn_obj *selector = si->args[0];

  if (!grn_obj_is_selector_proc(ctx, selector)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;

  grn_operator selector_op = grn_proc_get_selector_operator(ctx, selector);

  grn_index_datum index_datum;
  unsigned int n_index_datum;
  if (grn_obj_is_index_column(ctx, index)) {
    index_datum.index = index;
    n_index_datum = 1;
  } else if (grn_obj_is_key_accessor(ctx, index)) {
    index_datum.index = ((grn_accessor *)index)->obj;
    n_index_datum = 1;
  } else if (grn_obj_is_accessor(ctx, index)) {
    n_index_datum = grn_column_find_index_data(ctx,
                                               ((grn_accessor *)index)->obj,
                                               selector_op,
                                               &index_datum,
                                               1);
  } else {
    n_index_datum = grn_column_find_index_data(ctx,
                                               index,
                                               selector_op,
                                               &index_datum,
                                               1);
  }
  if (n_index_datum > 0) {
    if (grn_logger_pass(ctx, GRN_REPORT_INDEX_LOG_LEVEL)) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      char tag[GRN_TABLE_MAX_KEY_SIZE];
      name_size = grn_obj_name(ctx, selector, name, GRN_TABLE_MAX_KEY_SIZE);
      grn_snprintf(tag, GRN_TABLE_MAX_KEY_SIZE, GRN_TABLE_MAX_KEY_SIZE,
                   "[selector][%.*s]",
                   name_size, name);
      select_index_report(ctx, tag, index_datum.index);
    }

    grn_obj *table;
    bool table_is_referred = false;
    if (grn_obj_is_key_accessor(ctx, index)) {
      table = index_datum.index;
    } else {
      grn_id range = grn_obj_get_range(ctx, index_datum.index);
      table = grn_ctx_at(ctx, range);
      table_is_referred = true;
    }
    rc = grn_selector_run(ctx,
                          selector,
                          data->expr,
                          table,
                          index_datum.index,
                          si->nargs,
                          si->args,
                          si->weight_factor,
                          result_set,
                          logical_op);
    if (table_is_referred) {
      grn_obj_unref(ctx, table);
    }
  }

  return rc;
}

static grn_inline grn_rc
select_index_range_id(grn_ctx *ctx,
                      grn_table_selector *table_selector,
                      grn_obj *table,
                      grn_operator op,
                      grn_obj *result_set,
                      grn_operator logical_op)
{
  const char *tag = "[range][id]";

  if (select_index_use_sequential_search(ctx,
                                         table_selector,
                                         result_set,
                                         logical_op,
                                         tag,
                                         table)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = table_selector->data.scan_info;
  grn_obj id;
  GRN_UINT32_INIT(&id, 0);
  if (grn_obj_cast(ctx, si->query, &id, false) == GRN_SUCCESS) {
    grn_table_cursor *cursor;
    grn_id min = GRN_ID_NIL;
    grn_id max = GRN_ID_MAX;
    int offset = 0;
    int limit = -1;
    int flags = GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING;

    select_index_report(ctx, tag, table);

    switch (op) {
    case GRN_OP_LESS :
      max = GRN_UINT32_VALUE(&id);
      if (max > GRN_ID_NIL) {
        max--;
      }
      break;
    case GRN_OP_GREATER :
      min = GRN_UINT32_VALUE(&id) + 1;
      break;
    case GRN_OP_LESS_EQUAL :
      max = GRN_UINT32_VALUE(&id);
      break;
    case GRN_OP_GREATER_EQUAL :
      min = GRN_UINT32_VALUE(&id);
      break;
    default :
      break;
    }
    cursor = grn_table_cursor_open(ctx, table,
                                   NULL, 0,
                                   NULL, 0,
                                   offset, limit, flags);
    if (cursor) {
      uint32_t sid = GRN_UINT32_VALUE_AT(&(si->sections), 0);
      float weight = GRN_FLOAT32_VALUE_AT(&(si->weights), 0);
      weight *= table_selector->weight_factor * si->weight_factor;

      if (sid == 0) {
        grn_posting_internal posting = {0};

        posting.weight_float = weight;
        while ((posting.rid = grn_table_cursor_next(ctx, cursor))) {
          if (posting.rid < min) {
            continue;
          }
          if (posting.rid > max) {
            break;
          }
          grn_ii_posting_add_float(ctx,
                                   (grn_posting *)(&posting),
                                   (grn_hash *)result_set,
                                   logical_op);
        }
      }
      rc = GRN_SUCCESS;
      grn_table_cursor_close(ctx, cursor);
    }

    grn_ii_resolve_sel_and(ctx, (grn_hash *)result_set, logical_op);
  }
  GRN_OBJ_FIN(ctx, &id);

  return rc;
}

static grn_inline grn_rc
select_index_range_key(grn_ctx *ctx,
                       grn_table_selector *table_selector,
                       grn_obj *table,
                       grn_operator op,
                       grn_obj *result_set,
                       grn_operator logical_op)
{
  const char *tag = "[range][key]";

  if (select_index_use_sequential_search(ctx,
                                         table_selector,
                                         result_set,
                                         logical_op,
                                         tag,
                                         table)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = table_selector->data.scan_info;
  grn_obj key;
  GRN_OBJ_INIT(&key, GRN_BULK, 0, table->header.domain);
  if (grn_obj_cast(ctx, si->query, &key, false) == GRN_SUCCESS) {
    grn_table_cursor *cursor;
    const void *min = NULL, *max = NULL;
    unsigned int min_size = 0, max_size = 0;
    int offset = 0;
    int limit = -1;
    int flags = GRN_CURSOR_ASCENDING;

    select_index_report(ctx, tag, table);

    switch (op) {
    case GRN_OP_LESS :
      flags |= GRN_CURSOR_LT;
      max = GRN_BULK_HEAD(&key);
      max_size = GRN_BULK_VSIZE(&key);
      break;
    case GRN_OP_GREATER :
      flags |= GRN_CURSOR_GT;
      min = GRN_BULK_HEAD(&key);
      min_size = GRN_BULK_VSIZE(&key);
      break;
    case GRN_OP_LESS_EQUAL :
      flags |= GRN_CURSOR_LE;
      max = GRN_BULK_HEAD(&key);
      max_size = GRN_BULK_VSIZE(&key);
      break;
    case GRN_OP_GREATER_EQUAL :
      flags |= GRN_CURSOR_GE;
      min = GRN_BULK_HEAD(&key);
      min_size = GRN_BULK_VSIZE(&key);
      break;
    default :
      break;
    }
    cursor = grn_table_cursor_open(ctx, table,
                                   min, min_size, max, max_size,
                                   offset, limit, flags);
    if (cursor) {
      uint32_t sid = GRN_UINT32_VALUE_AT(&(si->sections), 0);
      float weight = GRN_FLOAT32_VALUE_AT(&(si->weights), 0);
      weight *= table_selector->weight_factor * si->weight_factor;

      if (sid == 0) {
        rc = grn_result_set_add_table_cursor(ctx,
                                             (grn_hash *)result_set,
                                             cursor,
                                             weight,
                                             logical_op);
      } else {
        rc = GRN_SUCCESS;
      }
      rc = GRN_SUCCESS;
      grn_table_cursor_close(ctx, cursor);
    }

    grn_ii_resolve_sel_and(ctx, (grn_hash *)result_set, logical_op);
  }
  GRN_OBJ_FIN(ctx, &key);

  return rc;
}

static grn_inline grn_rc
select_index_range_column(grn_ctx *ctx,
                          grn_table_selector *table_selector,
                          grn_obj *index,
                          grn_operator op,
                          grn_obj *result_set,
                          grn_operator logical_op)
{
  const char *tag = "[range]";

  grn_obj *lexicon = grn_ctx_at(ctx, index->header.domain);
  if (!lexicon) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  if (select_index_use_sequential_search(ctx,
                                         table_selector,
                                         result_set,
                                         logical_op,
                                         tag,
                                         lexicon)) {
    grn_obj_unref(ctx, lexicon);
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = table_selector->data.scan_info;
  grn_obj range;
  GRN_OBJ_INIT(&range, GRN_BULK, 0, lexicon->header.domain);
  if (grn_obj_cast(ctx, si->query, &range, false) == GRN_SUCCESS) {
    grn_table_cursor *cursor;
    const void *min = NULL, *max = NULL;
    unsigned int min_size = 0, max_size = 0;
    int offset = 0;
    int limit = -1;
    int flags = GRN_CURSOR_ASCENDING;

    select_index_report(ctx, "[range]", index);

    switch (op) {
    case GRN_OP_LESS :
      flags |= GRN_CURSOR_LT;
      max = GRN_BULK_HEAD(&range);
      max_size = GRN_BULK_VSIZE(&range);
      break;
    case GRN_OP_GREATER :
      flags |= GRN_CURSOR_GT;
      min = GRN_BULK_HEAD(&range);
      min_size = GRN_BULK_VSIZE(&range);
      break;
    case GRN_OP_LESS_EQUAL :
      flags |= GRN_CURSOR_LE;
      max = GRN_BULK_HEAD(&range);
      max_size = GRN_BULK_VSIZE(&range);
      break;
    case GRN_OP_GREATER_EQUAL :
      flags |= GRN_CURSOR_GE;
      min = GRN_BULK_HEAD(&range);
      min_size = GRN_BULK_VSIZE(&range);
      break;
    default :
      break;
    }
    cursor = grn_table_cursor_open(ctx, lexicon,
                                   min, min_size, max, max_size,
                                   offset, limit, flags);
    if (cursor) {
      uint32_t sid = GRN_UINT32_VALUE_AT(&(si->sections), 0);
      float weight = GRN_FLOAT32_VALUE_AT(&(si->weights), 0);
      weight *= table_selector->weight_factor * si->weight_factor;
      grn_obj *index_cursor = grn_index_cursor_open(ctx,
                                                    cursor,
                                                    index,
                                                    GRN_ID_NIL,
                                                    GRN_ID_MAX,
                                                    0);
      if (index_cursor) {
        grn_index_cursor_set_section_id(ctx, index_cursor, sid);
        if (si->position.specified) {
          grn_index_cursor_set_start_position(ctx,
                                              index_cursor,
                                              si->position.start);
        }
        rc = grn_result_set_add_index_cursor(ctx,
                                             (grn_hash *)result_set,
                                             index_cursor,
                                             1,
                                             weight,
                                             logical_op);
        grn_obj_close(ctx, index_cursor);
      }
      grn_table_cursor_close(ctx, cursor);
    }

    grn_ii_resolve_sel_and(ctx, (grn_hash *)result_set, logical_op);
  }
  GRN_OBJ_FIN(ctx, &range);
  grn_obj_unref(ctx, lexicon);

  return rc;
}

static grn_inline grn_rc
select_index_range(grn_ctx *ctx,
                   grn_table_selector *table_selector,
                   grn_obj *index,
                   grn_operator op,
                   grn_obj *result_set,
                   grn_operator logical_op)
{
  grn_obj *target = index;

  if (grn_obj_is_accessor(ctx, index)) {
    target = ((grn_accessor *)index)->obj;
    if (grn_obj_is_id_accessor(ctx, index)) {
      return select_index_range_id(ctx,
                                   table_selector,
                                   target,
                                   op,
                                   result_set,
                                   logical_op);
    }
  }

  switch (target->header.type) {
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
    /* target == table */
    return select_index_range_key(ctx,
                                  table_selector,
                                  target,
                                  op,
                                  result_set,
                                  logical_op);
  default :
    return select_index_range_column(ctx,
                                     table_selector,
                                     target,
                                     op,
                                     result_set,
                                     logical_op);
  }
}

static grn_rc
select_index_dispatch(grn_ctx *ctx,
                      grn_obj *index,
                      grn_operator op,
                      grn_obj *result_set,
                      grn_operator logical_op,
                      void *user_data)
{
  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  grn_table_selector *table_selector = user_data;

  switch (op) {
  case GRN_OP_EQUAL :
    rc = select_index_equal(ctx,
                            table_selector,
                            index,
                            op,
                            result_set,
                            logical_op);
    break;
  case GRN_OP_NOT_EQUAL :
    rc = select_index_not_equal(ctx,
                                table_selector,
                                index,
                                op,
                                result_set,
                                logical_op);
    break;
  case GRN_OP_MATCH :
  case GRN_OP_NEAR :
  case GRN_OP_NEAR2 :
  case GRN_OP_NEAR_PHRASE :
  case GRN_OP_ORDERED_NEAR_PHRASE :
  case GRN_OP_SIMILAR :
  case GRN_OP_REGEXP :
  case GRN_OP_QUORUM :
    rc = select_index_match(ctx,
                            table_selector,
                            index,
                            op,
                            result_set,
                            logical_op);
    break;
  case GRN_OP_PREFIX :
  case GRN_OP_SUFFIX :
    rc = select_index_fix(ctx,
                          table_selector,
                          index,
                          op,
                          result_set,
                          logical_op);
    break;
  case GRN_OP_TERM_EXTRACT :
    rc = select_index_extract(ctx,
                              table_selector,
                              index,
                              op,
                              result_set,
                              logical_op);
    break;
  case GRN_OP_CALL :
    rc = select_index_call(ctx,
                           table_selector,
                           index,
                           op,
                           result_set,
                           logical_op);
    break;
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
    rc = select_index_range(ctx,
                            table_selector,
                            index,
                            op,
                            result_set,
                            logical_op);
    break;
  default :
    /* todo : implement */
    /* todo : handle SCAN_PRE_CONST */
    break;
  }

  return rc;
}

static grn_inline bool
select_index(grn_ctx *ctx,
             grn_table_selector *table_selector)
{
  grn_table_selector_data *data = &(table_selector->data);
  scan_info *si = data->scan_info;

  if (!si->query) {
    if (si->op != GRN_OP_CALL || !grn_obj_is_selector_proc(ctx, si->args[0])) {
      return false;
    }
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  size_t n_indexes = GRN_PTR_VECTOR_SIZE(&(si->index));
  if (n_indexes > 0) {
    unsigned int previous_n_hits = grn_table_size(ctx, data->result_set);

    grn_id minimum_min_id = GRN_ID_NIL;
    bool minimum_min_id_is_set = false;

    grn_obj weights_buffer;
    GRN_FLOAT32_INIT(&weights_buffer, GRN_OBJ_VECTOR);

    int original_flags = ctx->flags;
    ctx->flags |= GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND;
    size_t i;
    for (i = 0; i < n_indexes; i++) {
      grn_obj *index = GRN_PTR_VALUE_AT(&(si->index), i);

      grn_search_optarg *options = &(data->search_options);
      memset(options, 0, sizeof(*options));
      if (si->op == GRN_OP_MATCH) {
        options->mode = GRN_OP_EXACT;
      } else {
        options->mode = si->op;
      }
      options->match_info.flags = GRN_MATCH_INFO_GET_MIN_RECORD_ID;
      options->max_interval = si->max_interval;
      options->additional_last_interval = si->additional_last_interval;
      options->similarity_threshold = si->similarity_threshold;
      options->quorum_threshold = si->quorum_threshold;
      options->query_options = table_selector->query_options;
      uint32_t section = GRN_UINT32_VALUE_AT(&(si->sections), i);
      float weight = GRN_FLOAT32_VALUE_AT(&(si->weights), i);
      weight *= table_selector->weight_factor * si->weight_factor;
      if (section == 0)  {
        options->weight_vector = NULL;
        options->weight_vector_float = NULL;
        options->vector_size = -1;
        options->weight_float = weight;
      } else {
        int weight_index = section - 1;
        int current_vector_size;
        current_vector_size = GRN_FLOAT32_VECTOR_SIZE(&weights_buffer);
        if (weight_index < current_vector_size) {
          GRN_FLOAT32_VALUE_AT(&weights_buffer, weight_index) = weight;
        } else {
          GRN_FLOAT32_SET_AT(ctx, &weights_buffer, weight_index, weight);
        }
        options->weight_vector = NULL;
        options->weight_vector_float = &GRN_FLOAT32_VALUE(&weights_buffer);
        options->vector_size = GRN_FLOAT32_VECTOR_SIZE(&weights_buffer);
        options->weight_float = 0.0;
      }
      options->scorer = GRN_PTR_VALUE_AT(&(si->scorers), i);
      options->scorer_args_expr =
        GRN_PTR_VALUE_AT(&(si->scorer_args_exprs), i);
      options->scorer_args_expr_offset =
        GRN_UINT32_VALUE_AT(&(si->scorer_args_expr_offsets), i);

      bool use_min_id_skip_enable;
      use_min_id_skip_enable =
        (grn_table_select_min_id_skip_enable && !GRN_ACCESSORP(index));
      if (use_min_id_skip_enable) {
        options->match_info.min = data->min_id;
      } else {
        options->match_info.min = GRN_ID_NIL;
      }
      if (n_indexes == 1 && grn_obj_is_accessor(ctx, index)) {
        rc = grn_accessor_execute(ctx,
                                  index,
                                  select_index_dispatch,
                                  table_selector,
                                  si->op,
                                  data->result_set,
                                  si->logical_op);
      } else {
        if (i < n_indexes - 1) {
          if (section > 0 &&
              index == GRN_PTR_VALUE_AT(&(si->index), i + 1) &&
              !options->scorer &&
              !GRN_PTR_VALUE_AT(&(si->scorers), i + 1)) {
            continue;
          }
        }
        rc = select_index_dispatch(ctx,
                                   index,
                                   si->op,
                                   data->result_set,
                                   si->logical_op,
                                   table_selector);
      }
      if (rc != GRN_SUCCESS) {
        break;
      }
      if (options->weight_vector_float) {
        int i;
        for (i = 0; i < options->vector_size; i++) {
          options->weight_vector_float[i] = 0;
        }
      }
      GRN_BULK_REWIND(&weights_buffer);
      if (use_min_id_skip_enable &&
          (!minimum_min_id_is_set ||
           options->match_info.min < minimum_min_id)) {
        minimum_min_id_is_set = true;
        minimum_min_id = options->match_info.min;
      }
      if (options->match_info.flags & GRN_MATCH_INFO_ONLY_SKIP_TOKEN) {
        data->is_skipped = true;
      }
    }
    ctx->flags = original_flags;

    GRN_OBJ_FIN(ctx, &weights_buffer);

    if (rc == GRN_SUCCESS) {
      bool need_resolve_sel_and = !data->is_skipped;
      if (si->op == GRN_OP_NOT_EQUAL) {
        need_resolve_sel_and = false;
      }
      if (need_resolve_sel_and) {
        grn_ii_resolve_sel_and(ctx,
                               (grn_hash *)(data->result_set),
                               si->logical_op);
      }
      if ((si->logical_op == GRN_OP_AND) ||
          (si->logical_op == GRN_OP_OR && previous_n_hits == 0)) {
        data->min_id = minimum_min_id;
      } else {
        data->min_id = table_selector->min_id;
      }
    }
  } else {
    if (si->op == GRN_OP_CALL && grn_obj_is_selector_proc(ctx, si->args[0])) {
      grn_obj *selector = si->args[0];
      grn_proc *proc = (grn_proc *)(selector);
      grn_operator selector_op = grn_proc_get_selector_operator(ctx, selector);
      bool callable = false;
      if (selector_op == GRN_OP_NOP) {
        callable = true;
      } else if (si->nargs > 1) {
        grn_obj *first_arg = si->args[1];
        if (grn_obj_is_index_column(ctx, first_arg)) {
          callable = grn_index_column_is_usable(ctx, first_arg, selector_op);
        }
      }
      if (callable) {
        if (grn_logger_pass(ctx, GRN_REPORT_INDEX_LOG_LEVEL)) {
          char proc_name[GRN_TABLE_MAX_KEY_SIZE];
          int proc_name_size;
          char tag[GRN_TABLE_MAX_KEY_SIZE];
          proc_name_size = grn_obj_name(ctx, (grn_obj *)proc,
                                        proc_name, GRN_TABLE_MAX_KEY_SIZE);
          proc_name[proc_name_size] = '\0';
          grn_snprintf(tag, GRN_TABLE_MAX_KEY_SIZE, GRN_TABLE_MAX_KEY_SIZE,
                       "[selector][no-index][%s]", proc_name);
          select_index_report(ctx, tag, table_selector->table);
        }
        rc = grn_selector_run(ctx,
                              selector,
                              data->expr,
                              table_selector->table,
                              NULL,
                              si->nargs,
                              si->args,
                              si->weight_factor,
                              data->result_set,
                              si->logical_op);
      }
    }
  }

  switch (rc) {
  case GRN_SUCCESS :
    return true;
    break;
  case GRN_FUNCTION_NOT_IMPLEMENTED :
    ERRCLR(ctx);
    return false;
  default :
    /* TODO: report error */
    return false;
  }
}

static void
inspect_condition_argument(grn_ctx *ctx,
                           grn_obj *buffer,
                           grn_obj *argument)
{
  switch (argument->header.type) {
  case GRN_BULK :
    if (grn_type_id_is_builtin(ctx, argument->header.domain)) {
      grn_inspect(ctx, buffer, argument);
    } else {
      grn_record_inspect_without_columns(ctx, buffer, argument);
    }
    break;
  case GRN_UVECTOR :
    if (grn_type_id_is_builtin(ctx, argument->header.domain)) {
      grn_inspect(ctx, buffer, argument);
    } else {
      grn_uvector_record_inspect_without_columns(ctx, buffer, argument);
    }
    break;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_NO_KEY :
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
  case GRN_COLUMN_INDEX :
    grn_inspect_name(ctx, buffer, argument);
    break;
  default :
    grn_inspect(ctx, buffer, argument);
    break;
  }
}

static void
inspect_condition_call(grn_ctx *ctx,
                       grn_obj *buffer,
                       grn_expr_code *codes,
                       uint32_t start,
                       uint32_t end)
{
  uint32_t i;

  for (i = start; i <= end; i++) {
    grn_expr_code *code = codes + i;
    if (i == start) {
      if (grn_obj_is_proc(ctx, code->value)) {
        grn_inspect_name(ctx, buffer, code->value);
      } else {
        inspect_condition_argument(ctx, buffer, code->value);
      }
      GRN_TEXT_PUTC(ctx, buffer, '(');
    } else if (code->value) {
      if (i > start + 1) {
        GRN_TEXT_PUTS(ctx, buffer, ", ");
      }
      inspect_condition_argument(ctx, buffer, code->value);
    }
  }
  GRN_TEXT_PUTC(ctx, buffer, ')');
}

static const char *
inspect_condition(grn_ctx *ctx,
                  grn_obj *buffer,
                  scan_info *si,
                  grn_expr *expr)
{
  uint32_t i;
  uint32_t n_codes;
  grn_operator last_operator;

  if (!grn_query_log_show_condition) {
    return "";
  }

  GRN_BULK_REWIND(buffer);

  GRN_TEXT_PUTS(ctx, buffer, ": ");

  if (si->flags & SCAN_POP) {
    GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(si->logical_op));
    GRN_TEXT_PUTC(ctx, buffer, '\0');
    return GRN_TEXT_VALUE(buffer);
  }

  n_codes = si->end - si->start + 1;
  last_operator = expr->codes[si->end].op;

  switch (last_operator) {
  case GRN_OP_CALL :
    inspect_condition_call(ctx,
                           buffer,
                           expr->codes,
                           si->start,
                           si->end);
    break;
  case GRN_OP_EQUAL :
  case GRN_OP_NOT_EQUAL :
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
  case GRN_OP_MATCH :
  case GRN_OP_NEAR :
  case GRN_OP_NEAR2 :
  case GRN_OP_NEAR_PHRASE :
  case GRN_OP_ORDERED_NEAR_PHRASE :
  case GRN_OP_PREFIX :
    if (n_codes == 3) {
      grn_expr_code *arg1 = expr->codes + si->start;
      grn_expr_code *arg2 = expr->codes + si->start + 1;

      if (arg1->value->header.type == GRN_EXPR) {
        GRN_TEXT_PUTS(ctx, buffer, "(match columns)");
      } else {
        inspect_condition_argument(ctx, buffer, arg1->value);
      }
      GRN_TEXT_PUTC(ctx, buffer, ' ');
      GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(last_operator));
      GRN_TEXT_PUTC(ctx, buffer, ' ');
      inspect_condition_argument(ctx, buffer, arg2->value);
    } else {
      GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(last_operator));
      GRN_TEXT_PUTC(ctx, buffer, '(');
      for (i = si->start; i < si->end; i++) {
        grn_expr_code *code = expr->codes + i;
        if (i != si->start) {
          GRN_TEXT_PUTS(ctx, buffer, ", ");
        }
        if (code->modify > 0 && code[code->modify].op == GRN_OP_CALL) {
          inspect_condition_call(ctx,
                                 buffer,
                                 code,
                                 0,
                                 code->modify);
          if (i + code->modify < si->end &&
              code[code->modify + 1].op == GRN_OP_PUSH) {
            i += code->modify;
          } else {
            i += code->modify - 1;
          }
        } else {
          if (code->value) {
            if (i == si->start && code->value->header.type == GRN_EXPR) {
              GRN_TEXT_PUTS(ctx, buffer, "(match columns)");
            } else {
              inspect_condition_argument(ctx,
                                         buffer,
                                         code->value);
            }
          } else {
            GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(code->op));
          }
        }
      }
      GRN_TEXT_PUTC(ctx, buffer, ')');
    }
    break;
  default :
    for (i = si->start; i <= si->end; i++) {
      grn_expr_code *code = expr->codes + i;
      if (i > si->start) {
        GRN_TEXT_PUTC(ctx, buffer, ' ');
      }
      if (code->value) {
        inspect_condition_argument(ctx, buffer, code->value);
      } else {
        GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(code->op));
      }
    }
    break;
  }

  GRN_TEXT_PUTC(ctx, buffer, '\0');
  return GRN_TEXT_VALUE(buffer);
}

grn_obj *
grn_table_selector_select(grn_ctx *ctx,
                          grn_table_selector *table_selector,
                          grn_obj *result_set)
{
  bool result_set_created = false;
  GRN_API_ENTER;
  if (result_set) {
    if (result_set->header.type != GRN_TABLE_HASH_KEY ||
        (result_set->header.domain != DB_OBJ(table_selector->table)->id)) {
      ERR(GRN_INVALID_ARGUMENT,
          "[table-selector][select] result set must be a hash table");
      GRN_API_RETURN(NULL);
    }
  } else {
    result_set = grn_table_create(ctx,
                                  NULL, 0,
                                  NULL,
                                  GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                  table_selector->table,
                                  NULL);
    if (!result_set) {
      GRN_API_RETURN(NULL);
    }
    result_set_created = true;
  }

  uint32_t result_set_size = GRN_HASH_SIZE((grn_hash *)result_set);
  if (table_selector->op != GRN_OP_OR && result_set_size == 0) {
    GRN_API_RETURN(result_set);
  }

  if (table_selector->use_sequential_scan) {
    select_sequential_scan(ctx, table_selector, result_set);
    if (ctx->rc != GRN_SUCCESS) {
      if (result_set_created) {
        grn_obj_close(ctx, result_set);
      }
      result_set = NULL;
    }
    GRN_API_RETURN(result_set);
  }

  grn_table_selector_data *data = &(table_selector->data);
  if (data->scanner) {
    grn_scanner_close(ctx, data->scanner);
  }
  data->scanner = grn_scanner_open(ctx,
                                   table_selector->expr,
                                   table_selector->op,
                                   result_set_size > 0);
  if (!data->scanner) {
    if (ctx->rc == GRN_SUCCESS) {
      grn_table_selector sequential_table_selector;
      grn_table_selector_init(ctx,
                              &sequential_table_selector,
                              table_selector->table,
                              table_selector->expr,
                              table_selector->op);
      grn_table_selector_set_use_sequential_scan(ctx,
                                                 &sequential_table_selector,
                                                 true);
      grn_table_selector_select(ctx,
                                &sequential_table_selector,
                                result_set);
      grn_table_selector_fin(ctx, &sequential_table_selector);
      if (ctx->rc) {
        if (result_set_created) {
          grn_obj_close(ctx, result_set);
        }
        result_set = NULL;
      }
    }
    GRN_API_RETURN(result_set);
  }

  grn_obj result_set_stack;
  GRN_PTR_INIT(&result_set_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);

  grn_obj *base_result_set = NULL;

  data->initial_result_set = result_set;
  data->result_set = result_set;
  data->min_id = table_selector->min_id;
  data->expr = data->scanner->expr;
  data->variable = grn_expr_get_var_by_offset(ctx, data->expr, 0);
  data->is_skipped = false;
  data->is_first_unskipped_scan_info = true;
  if (result_set_size > 0 && table_selector->op == GRN_OP_AND) {
    bool have_push = false;
    int i;
    for (i = 0; i < data->scanner->n_sis; i++) {
      scan_info *si = data->scanner->sis[i];
      if (si->flags & SCAN_PUSH) {
        have_push = true;
        break;
      }
    }
    if (have_push) {
      base_result_set = selector_create_result_set(ctx, table_selector);
      if (base_result_set) {
        GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)result_set, cursor, id) {
          void *key = NULL;
          uint32_t key_size = 0;
          void *value = NULL;
          grn_hash_cursor_get_key_value(ctx, cursor, &key, &key_size, &value);
          void *base_value = NULL;
          if (grn_table_add_v(ctx,
                              base_result_set,
                              key,
                              key_size,
                              &base_value,
                              NULL)) {
            grn_rset_recinfo *ri = value;
            grn_rset_recinfo *base_ri = base_value;
            grn_memcpy(base_ri, ri, ((grn_hash *)base_result_set)->value_size);
            base_ri->score = 0;
          }
        } GRN_HASH_EACH_END(ctx, cursor);
      }
    }
  }

  grn_expr *e = (grn_expr *)(data->scanner->expr);
  grn_expr_code *codes = e->codes;
  uint32_t codes_curr = e->codes_curr;
  grn_obj condition_inspect_buffer;
  GRN_TEXT_INIT(&condition_inspect_buffer, 0);
  {
    int i;
    for (i = 0; i < data->scanner->n_sis; i++) {
      scan_info *si = data->scanner->sis[i];
      data->nth_scan_info = i;
      data->scan_info = si;
      if (i > 0 && data->is_first_unskipped_scan_info) {
        if (data->is_skipped) {
          if (si->logical_op == GRN_OP_AND) {
            si->logical_op = GRN_OP_OR;
          }
        } else {
          data->is_first_unskipped_scan_info = false;
        }
      }
      data->is_skipped = false;
      if (si->flags & SCAN_POP) {
        grn_obj *result_set_;
        GRN_PTR_POP(&result_set_stack, result_set_);
        grn_table_setoperation_with_weight_factor(ctx,
                                                  result_set_,
                                                  data->result_set,
                                                  result_set_,
                                                  si->logical_op,
                                                  si->weight_factor);
        grn_obj_close(ctx, data->result_set);
        data->result_set = result_set_;
        data->min_id = table_selector->min_id;
      } else {
        bool processed = false;
        if (si->flags & SCAN_PUSH) {
          grn_obj *new_result_set =
            selector_create_result_set(ctx, table_selector);
          if (!new_result_set) {
            break;
          }
          if (base_result_set && si->logical_op == GRN_OP_OR) {
            GRN_LOG(ctx, GRN_REPORT_INDEX_LOG_LEVEL,
                    "[table-selector][select][push][initial] <%u>",
                    grn_table_size(ctx, base_result_set));
            grn_table_setoperation(ctx,
                                   new_result_set,
                                   base_result_set,
                                   new_result_set,
                                   GRN_OP_OR);
            si->logical_op = GRN_OP_AND;
          }
          GRN_PTR_PUT(ctx, &result_set_stack, data->result_set);
          data->result_set = new_result_set;
          data->min_id = table_selector->min_id;
        }
        if (si->logical_op != GRN_OP_AND) {
          data->min_id = table_selector->min_id;
        }
        processed = select_index(ctx, table_selector);
        if (!processed) {
          if (ctx->rc != GRN_SUCCESS) {
            break;
          }
          e->codes = codes + si->start;
          e->codes_curr = si->end - si->start + 1;
          grn_table_selector sequential_table_selector;
          grn_table_selector_init(ctx,
                                  &sequential_table_selector,
                                  table_selector->table,
                                  data->scanner->expr,
                                  si->logical_op);
          grn_table_selector_set_use_sequential_scan(ctx,
                                                     &sequential_table_selector,
                                                     true);
          grn_table_selector_select(ctx,
                                    &sequential_table_selector,
                                    data->result_set);
          grn_table_selector_fin(ctx, &sequential_table_selector);
          e->codes = codes;
          e->codes_curr = codes_curr;
          data->min_id = table_selector->min_id;
        }
      }
      GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                    ":", "%sfilter(%d)%s",
                    grn_expr_get_query_log_tag_prefix(ctx, table_selector->expr),
                    grn_table_size(ctx, data->result_set),
                    inspect_condition(ctx,
                                      &condition_inspect_buffer,
                                      si,
                                      e));
      if (ctx->rc != GRN_SUCCESS) {
        if (result_set_created) {
          grn_obj_close(ctx, data->result_set);
        }
        data->result_set = NULL;
        break;
      }
    }
  }
  GRN_OBJ_FIN(ctx, &condition_inspect_buffer);
  e->codes = codes;
  e->codes_curr = codes_curr;

  result_set = data->result_set;

  if (base_result_set) {
    grn_obj_close(ctx, base_result_set);
  }

  {
    int i = 0;
    if (!result_set_created) { i++; }
    int n_result_sets = GRN_PTR_VECTOR_SIZE(&result_set_stack);
    for (; i < n_result_sets; i++) {
      grn_obj *result_set_ = GRN_PTR_VALUE_AT(&result_set_stack, i);
      grn_obj_close(ctx, result_set_);
    }
  }
  GRN_OBJ_FIN(ctx, &result_set_stack);

  grn_scanner_close(ctx, data->scanner);
  data->scanner = NULL;

  GRN_API_RETURN(result_set);
}
