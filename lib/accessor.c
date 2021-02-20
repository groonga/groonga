/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018  Brazil
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

#include "grn.h"
#include "grn_accessor.h"
#include "grn_db.h"
#include "grn_ii.h"
#include "grn_report.h"
#include "grn_posting.h"

#include <stdio.h>

grn_accessor *
grn_accessor_new(grn_ctx *ctx)
{
  grn_accessor *res = GRN_MALLOCN(grn_accessor, 1);
  if (!res) {
    return NULL;
  }

  res->header.type = GRN_ACCESSOR;
  res->header.impl_flags = GRN_OBJ_ALLOCATED;
  res->header.flags = 0;
  res->header.domain = GRN_ID_NIL;
  res->range = GRN_ID_NIL;
  res->action = GRN_ACCESSOR_VOID;
  res->offset = 0;
  res->obj = NULL;
  res->next = NULL;
  res->reference_count = 1;
  grn_log_reference_count("%p: accessor: %p: %u\n",
                          ctx, res, res->reference_count);
  return res;
}

grn_obj *
grn_accessor_copy(grn_ctx *ctx, grn_obj *accessor)
{
  grn_accessor *new_accessor = grn_accessor_new(ctx);
  grn_accessor *a = (grn_accessor *)accessor;
  grn_accessor *new_a = new_accessor;
  for (; a; a = a->next) {
    new_a->action = a->action;
    new_a->obj = a->obj;
    if (new_a->obj) {
      grn_obj_refer(ctx, new_a->obj);
    }
    if (a->next) {
      new_a->next = grn_accessor_new(ctx);
      new_a = new_a->next;
    }
  }
  return (grn_obj *)new_accessor;
}

static grn_rc
grn_accessor_resolve_one_index_column(grn_ctx *ctx, grn_accessor *accessor,
                                      grn_obj *current_res, grn_obj **next_res)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *column = NULL;
  grn_id next_res_domain_id = GRN_ID_NIL;

  {
    grn_obj *index;
    grn_obj source_ids;
    unsigned int i, n_ids;

    index = accessor->obj;
    next_res_domain_id = index->header.domain;

    GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
    grn_obj_get_info(ctx, index, GRN_INFO_SOURCE, &source_ids);
    n_ids = GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id);
    for (i = 0; i < n_ids; i++) {
      grn_id source_id;
      grn_obj *source;

      source_id = GRN_UINT32_VALUE_AT(&source_ids, i);
      source = grn_ctx_at(ctx, source_id);
      if (DB_OBJ(source)->range == next_res_domain_id) {
        column = source;
        break;
      }
      grn_obj_unlink(ctx, source);
    }

    if (!column) {
      return GRN_INVALID_ARGUMENT;
    }
  }

  {
    grn_rc rc;
    grn_obj *next_res_domain = grn_ctx_at(ctx, next_res_domain_id);
    *next_res = grn_table_create(ctx, NULL, 0, NULL,
                                 GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                 next_res_domain, NULL);
    rc = ctx->rc;
    grn_obj_unlink(ctx, next_res_domain);
    if (!*next_res) {
      return rc;
    }
  }

  {
    grn_obj_flags column_value_flags = 0;
    grn_obj column_value;
    grn_posting_internal add_posting = {0};

    if (column->header.type == GRN_COLUMN_VAR_SIZE) {
      column_value_flags |= GRN_OBJ_VECTOR;
    }
    GRN_VALUE_FIX_SIZE_INIT(&column_value,
                            column_value_flags,
                            next_res_domain_id);

    add_posting.sid = 0;
    add_posting.pos = 0;
    add_posting.weight_float = 0;

    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)current_res, cursor, id) {
      void *key;
      void *value;
      grn_hash_cursor_get_key_value(ctx, cursor, &key, NULL, &value);

      grn_id *tid = key;
      grn_rset_recinfo *recinfo = value;

      add_posting.weight_float = recinfo->score;

      GRN_BULK_REWIND(&column_value);
      grn_obj_get_value(ctx, column, *tid, &column_value);

      int n_elements = GRN_BULK_VSIZE(&column_value) / sizeof(grn_id);
      for (int i = 0; i < n_elements; i++) {
        add_posting.rid = GRN_RECORD_VALUE_AT(&column_value, i);
        rc = grn_ii_posting_add_float(ctx,
                                      (grn_posting *)(&add_posting),
                                      (grn_hash *)*next_res,
                                      GRN_OP_OR);
        if (rc != GRN_SUCCESS) {
          break;
        }
      }
      if (rc != GRN_SUCCESS) {
        break;
      }
    } GRN_HASH_EACH_END(ctx, cursor);

    GRN_OBJ_FIN(ctx, &column_value);
  }

  if (rc != GRN_SUCCESS) {
    grn_obj_unlink(ctx, *next_res);
  }

  return rc;
}

static grn_rc
grn_accessor_resolve_one_table(grn_ctx *ctx, grn_accessor *accessor,
                               grn_obj *current_res, grn_obj **next_res)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *table;

  table = accessor->obj;
  *next_res = grn_table_create(ctx, NULL, 0, NULL,
                               GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                               table, NULL);
  if (!*next_res) {
    return ctx->rc;
  }

  grn_report_table(ctx,
                   "[accessor][resolve]",
                   "",
                   table);

  {
    grn_posting_internal posting = {0};

    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)current_res, cursor, id) {
      void *key;
      void *value;
      grn_id *record_id;
      grn_rset_recinfo *recinfo;
      grn_id next_record_id;

      grn_hash_cursor_get_key_value(ctx, cursor, &key, NULL, &value);
      record_id = key;
      recinfo = value;
      next_record_id = grn_table_get(ctx,
                                     table,
                                     record_id,
                                     sizeof(grn_id));
      if (next_record_id == GRN_ID_NIL) {
        continue;
      }

      posting.rid = next_record_id;
      posting.weight_float = recinfo->score;
      rc = grn_ii_posting_add_float(ctx,
                                    (grn_posting *)(&posting),
                                    (grn_hash *)*next_res,
                                    GRN_OP_OR);
      if (rc != GRN_SUCCESS) {
        break;
      }
    } GRN_HASH_EACH_END(ctx, cursor);
  }

  if (rc != GRN_SUCCESS) {
    grn_obj_unlink(ctx, *next_res);
  }

  return rc;
}

static grn_rc
grn_accessor_resolve_one_data_column_sequential(grn_ctx *ctx,
                                                grn_accessor *accessor,
                                                grn_obj *current_res,
                                                grn_obj **next_res)
{
  grn_obj *column = accessor->obj;
  grn_report_column(ctx,
                    "[accessor][resolve][data-column]",
                    "",
                    column);

  grn_id next_res_domain_id = column->header.domain;
  grn_obj *next_res_domain = grn_ctx_at(ctx, next_res_domain_id);
  *next_res = grn_table_create(ctx, NULL, 0, NULL,
                               GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                               next_res_domain, NULL);
  if (!*next_res) {
    return ctx->rc;
  }

  {
    grn_obj value;
    GRN_VOID_INIT(&value);
    GRN_TABLE_EACH_BEGIN(ctx, next_res_domain, cursor, id) {
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, column, id, &value);
      bool found = false;
      switch (value.header.type) {
      case GRN_BULK :
        found = (grn_table_get(ctx,
                               current_res,
                               GRN_BULK_HEAD(&value),
                               GRN_BULK_VSIZE(&value)) != GRN_ID_NIL);

        break;
      case GRN_UVECTOR :
        {
          size_t i, n_elements;
          n_elements = GRN_BULK_VSIZE(&value) / sizeof(grn_id);
          for (i = 0; i < n_elements; i++) {
            grn_id id = GRN_RECORD_VALUE_AT(&value, i);
            found = (grn_table_get(ctx,
                                   current_res,
                                   &id,
                                   sizeof(grn_id)) != GRN_ID_NIL);
            if (found) {
              break;
            }
          }
        }
        break;
      default :
        break;
      }
      if (found) {
        grn_posting_internal posting = {0};
        posting.rid = id;
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)*next_res,
                                 GRN_OP_OR);
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &value);
  }

  return ctx->rc;
}

static grn_rc
grn_accessor_resolve_one_data_column_index(grn_ctx *ctx,
                                           grn_index_datum *index_datum,
                                           grn_obj *current_res,
                                           grn_obj **next_res)
{
  if (index_datum->index->header.domain != current_res->header.domain) {
    char index_name[GRN_TABLE_MAX_KEY_SIZE];
    int index_name_size;
    grn_obj *expected;
    char expected_name[GRN_TABLE_MAX_KEY_SIZE];
    int expected_name_size;

    index_name_size = grn_obj_name(ctx,
                                   index_datum->index,
                                   index_name,
                                   GRN_TABLE_MAX_KEY_SIZE);
    expected = grn_ctx_at(ctx, current_res->header.domain);
    expected_name_size = grn_obj_name(ctx,
                                      expected,
                                      expected_name,
                                      GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_INVALID_ARGUMENT,
        "[accessor][resolve][data-column] lexicon mismatch index: "
        "<%.*s> "
        "expected:<%.*s>",
        index_name_size,
        index_name,
        expected_name_size,
        expected_name);
    return ctx->rc;
  }

  grn_report_index(ctx,
                   "[accessor][resolve][data-column]",
                   "",
                   index_datum->index);
  {
    grn_rc rc;
    grn_id next_res_domain_id = DB_OBJ(index_datum->index)->range;
    grn_obj *next_res_domain = grn_ctx_at(ctx, next_res_domain_id);
    *next_res = grn_table_create(ctx, NULL, 0, NULL,
                                 GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                 next_res_domain, NULL);
    rc = ctx->rc;
    grn_obj_unlink(ctx, next_res_domain);
    if (!*next_res) {
      return rc;
    }
  }

  {
    grn_rc rc = GRN_SUCCESS;
    grn_obj *index_column = index_datum->index;
    grn_obj *index_cursor = grn_index_cursor_open(ctx,
                                                  NULL,
                                                  index_column,
                                                  GRN_ID_NIL,
                                                  GRN_ID_MAX,
                                                  0);
    if (!index_cursor) {
      grn_obj_unlink(ctx, *next_res);
      return ctx->rc;
    }
    if (index_datum->section > 0) {
      rc = grn_index_cursor_set_section_id(ctx,
                                           index_cursor,
                                           index_datum->section);
    }
    if (rc != GRN_SUCCESS) {
      grn_obj_close(ctx, index_cursor);
      grn_obj_unlink(ctx, *next_res);
      return ctx->rc;
    }

    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)current_res, cursor, id) {
      void *key;
      void *value;
      grn_hash_cursor_get_key_value(ctx, cursor, &key, NULL, &value);

      grn_id *term_id = key;
      grn_rset_recinfo *recinfo = value;

      rc = grn_index_cursor_set_term_id(ctx, index_cursor, *term_id);
      if (rc == GRN_SUCCESS) {
        rc = grn_result_set_add_index_cursor(ctx,
                                             (grn_hash *)(*next_res),
                                             index_cursor,
                                             recinfo->score,
                                             1,
                                             GRN_OP_OR);
      }

      if (rc != GRN_SUCCESS) {
        break;
      }
    } GRN_HASH_EACH_END(ctx, cursor);
    grn_obj_close(ctx, index_cursor);

    if (rc != GRN_SUCCESS) {
      grn_obj_unlink(ctx, *next_res);
    }

    return rc;
  }
}

static grn_rc
grn_accessor_resolve_one_data_column(grn_ctx *ctx, grn_accessor *accessor,
                                     grn_obj *current_res, grn_obj **next_res)
{
  grn_rc rc = GRN_SUCCESS;
  grn_index_datum index_datum;
  unsigned int n_index_data;

  n_index_data = grn_column_get_all_index_data(ctx,
                                               accessor->obj,
                                               &index_datum,
                                               1);
  if (n_index_data == 0) {
    rc = grn_accessor_resolve_one_data_column_sequential(ctx,
                                                         accessor,
                                                         current_res,
                                                         next_res);
  } else {
    rc = grn_accessor_resolve_one_data_column_index(ctx,
                                                    &index_datum,
                                                    current_res,
                                                    next_res);
    grn_obj_unref(ctx, index_datum.index);
  }

  return rc;
}

grn_rc
grn_accessor_resolve(grn_ctx *ctx, grn_obj *accessor, int depth,
                     grn_obj *base_res, grn_obj *res,
                     grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  grn_accessor *a;
  grn_obj accessor_stack;
  int i, n_accessors;
  grn_obj *current_res = base_res;

  GRN_PTR_INIT(&accessor_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
  n_accessors = 0;
  for (a = (grn_accessor *)accessor; a; a = a->next) {
    if (depth == n_accessors) {
      break;
    }
    GRN_PTR_PUT(ctx, &accessor_stack, a);
    n_accessors++;
  }

  for (i = n_accessors; i > 0; i--) {
    grn_obj *next_res = NULL;

    a = (grn_accessor *)GRN_PTR_VALUE_AT(&accessor_stack, i - 1);
    if (a->obj->header.type == GRN_COLUMN_INDEX) {
      rc = grn_accessor_resolve_one_index_column(ctx, a,
                                                 current_res, &next_res);
    } else if (grn_obj_is_table(ctx, a->obj)) {
      rc = grn_accessor_resolve_one_table(ctx, a,
                                          current_res, &next_res);
    } else {
      rc = grn_accessor_resolve_one_data_column(ctx, a,
                                                current_res, &next_res);
    }

    if (current_res != base_res) {
      grn_obj_unlink(ctx, current_res);
    }

    if (rc != GRN_SUCCESS) {
      break;
    }

    current_res = next_res;
  }

  if (rc == GRN_SUCCESS && current_res != base_res) {
    GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)current_res, cursor, id) {
      void *key;
      void *value;
      grn_id *record_id;
      grn_rset_recinfo *recinfo;
      grn_posting_internal posting = {0};

      grn_hash_cursor_get_key_value(ctx, cursor, &key, NULL, &value);
      record_id = key;
      recinfo = value;
      posting.rid = *record_id;
      posting.sid = 1;
      posting.pos = 0;
      posting.weight_float = recinfo->score;
      grn_ii_posting_add_float(ctx,
                               (grn_posting *)(&posting),
                               (grn_hash *)res,
                               op);
    } GRN_HASH_EACH_END(ctx, cursor);
    grn_obj_unlink(ctx, current_res);
    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  } else {
    if (rc == GRN_SUCCESS) {
      rc = GRN_INVALID_ARGUMENT;
    }
  }

  GRN_OBJ_FIN(ctx, &accessor_stack);
  return rc;
}

grn_id
grn_accessor_resolve_id(grn_ctx *ctx, grn_obj *accessor, grn_id id)
{
  GRN_API_ENTER;

  grn_accessor *a;
  for (a = (grn_accessor *)accessor;
       a->next;
       a = a->next) {
    switch (a->action) {
    case GRN_ACCESSOR_GET_KEY :
      {
        grn_id next_id;
        int key_size = grn_table_get_key(ctx,
                                         a->obj,
                                         id,
                                         &next_id,
                                         sizeof(grn_id));
        if (key_size != sizeof(grn_id)) {
          GRN_API_RETURN(id);
        }
        id = next_id;
      }
      break;
    case GRN_ACCESSOR_GET_VALUE :
      {
        grn_obj value;
        GRN_VOID_INIT(&value);
        grn_obj_get_value(ctx, a->obj, id, &value);
        if (GRN_BULK_VSIZE(&value) < sizeof(grn_id)) {
          GRN_OBJ_FIN(ctx, &value);
          GRN_API_RETURN(id);
        }
        id = GRN_RECORD_VALUE(&value);
        GRN_OBJ_FIN(ctx, &value);
      }
      break;
    default :
      GRN_API_RETURN(id);
    }
  }

  GRN_API_RETURN(id);
}

grn_rc
grn_accessor_execute(grn_ctx *ctx,
                     grn_obj *accessor,
                     grn_accessor_execute_func execute,
                     void *execute_data,
                     grn_operator execute_op,
                     grn_obj *res,
                     grn_operator logical_op)
{
  GRN_API_ENTER;

  if (!grn_obj_is_accessor(ctx, accessor)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, accessor);
    ERR(GRN_INVALID_ARGUMENT,
        "[accessor][execute] must be accessor: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  int depth = 0;
  grn_accessor *a;
  for (a = (grn_accessor *)accessor; a->next; a = a->next) {
    depth++;
  }

  grn_index_datum index_data;
  unsigned int n_index_datum;
  grn_obj *index;
  n_index_datum = grn_column_find_index_data(ctx,
                                             a->obj,
                                             execute_op,
                                             &index_data,
                                             1);
  if (n_index_datum == 0) {
    index = (grn_obj *)a;
  } else {
    index = index_data.index;
  }

  grn_rc rc;
  if (depth == 0) {
    rc = execute(ctx,
                 index,
                 execute_op,
                 res,
                 logical_op,
                 execute_data);
  } else {
    grn_obj *base_table = NULL;
    if (grn_obj_is_table(ctx, a->obj)) {
      base_table = a->obj;
    } else {
      base_table = grn_ctx_at(ctx, a->obj->header.domain);
    }

    grn_obj *base_res =
      grn_table_create(ctx, NULL, 0, NULL,
                       GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                       base_table, NULL);
    rc = execute(ctx,
                 index,
                 execute_op,
                 base_res,
                 GRN_OP_OR,
                 execute_data);
    if (rc == GRN_SUCCESS) {
      rc = grn_accessor_resolve(ctx,
                                accessor,
                                depth,
                                base_res,
                                res,
                                logical_op);
    }
    grn_obj_close(ctx, base_res);
  }

  if (n_index_datum > 0) {
    grn_obj_unref(ctx, index_data.index);
  }

  GRN_API_RETURN(rc);
}

typedef struct {
  grn_obj *accessor;
  grn_obj *query;
  grn_search_optarg *optarg;
  grn_obj *index;
  bool index_need_unref;
  grn_operator operator;
  grn_obj query_casted;
  const char *query_raw;
  uint32_t query_raw_len;
  uint32_t n_base_records;
  uint32_t n_target_records;
  uint32_t depth;
} grn_accessor_estimate_size_for_query_data;

static void
grn_accessor_estimate_size_for_query_data_fin(
  grn_ctx *ctx,
  grn_accessor_estimate_size_for_query_data *data)
{
  if (data->index_need_unref) {
    grn_obj_unref(ctx, data->index);
  }
  GRN_OBJ_FIN(ctx, &(data->query_casted));
}

static void
grn_accessor_estimate_size_for_query_cast_failed(
  grn_ctx *ctx,
  grn_accessor_estimate_size_for_query_data *data)
{
  grn_obj detail;
  GRN_TEXT_INIT(&detail, 0);
  GRN_TEXT_PUTS(ctx, &detail, "<");
  grn_inspect(ctx, &detail, data->query);
  GRN_TEXT_PUTS(ctx, &detail, "> -> ");
  grn_obj *domain = grn_ctx_at(ctx, data->query_casted.header.domain);
  grn_inspect(ctx, &detail, domain);
  grn_obj_unlink(ctx, domain);
  GRN_TEXT_PUTS(ctx, &detail, ": ");
  grn_inspect(ctx, &detail, data->accessor);
  ERR(GRN_INVALID_ARGUMENT,
      "[accessor][estimate-size][query][cast] "
      "failed: %.*s",
      (int)GRN_TEXT_LEN(&detail),
      GRN_TEXT_VALUE(&detail));
  GRN_OBJ_FIN(ctx, &detail);
}

static bool
grn_accessor_estimate_size_for_query_prepare(
  grn_ctx *ctx,
  grn_accessor_estimate_size_for_query_data *data)
{
  grn_accessor *a = (grn_accessor *)(data->accessor);
  if (grn_obj_is_table(ctx, a->obj)) {
    data->n_target_records = grn_table_size(ctx, a->obj);
  } else {
    grn_obj *target_table = grn_ctx_at(ctx, a->obj->header.domain);
    data->n_target_records = grn_table_size(ctx, target_table);
    grn_obj_unlink(ctx, target_table);
  }

  if (data->n_target_records == 0) {
    return false;
  }

  for (; a->next; a = a->next) {
    data->depth++;
  }

  if (grn_obj_is_table(ctx, a->obj)) {
    data->n_base_records = grn_table_size(ctx, a->obj);
  } else {
    grn_obj *base_table = grn_ctx_at(ctx, a->obj->header.domain);
    data->n_base_records = grn_table_size(ctx, base_table);
    grn_obj_unlink(ctx, base_table);
  }

  if (data->optarg) {
    if (data->optarg->mode == GRN_OP_EXACT) {
      data->operator = GRN_OP_MATCH;
    } else {
      data->operator = data->optarg->mode;
    }
  }
  grn_index_datum index_data;
  unsigned int n_index_datum;
  n_index_datum = grn_column_find_index_data(ctx,
                                             a->obj,
                                             data->operator,
                                             &index_data,
                                             1);
  if (n_index_datum == 0) {
    if (!grn_obj_is_table(ctx, a->obj)) {
      return false;
    }
    data->index = a->obj;
  } else {
    data->index = index_data.index;
    data->index_need_unref = true;
  }

  grn_obj *lexicon;
  bool lexicon_need_unref = false;
  if (grn_obj_is_table(ctx, data->index)) {
    lexicon = data->index;
  } else {
    lexicon = grn_ctx_at(ctx, data->index->header.domain);
    lexicon_need_unref = true;
  }

  if (data->query->header.domain == lexicon->header.domain) {
    data->query_raw = GRN_BULK_HEAD(data->query);
    data->query_raw_len = GRN_BULK_VSIZE(data->query);
  } else {
    grn_obj_reinit_for(ctx, &(data->query_casted), lexicon);
    grn_rc rc = grn_obj_cast(ctx, data->query, &(data->query_casted), false);
    if (rc == GRN_SUCCESS) {
      data->query_raw = GRN_BULK_HEAD(&(data->query_casted));
      data->query_raw_len = GRN_BULK_VSIZE(&(data->query_casted));
    } else {
      grn_accessor_estimate_size_for_query_cast_failed(ctx, data);
    }
  }

  if (lexicon_need_unref) {
    grn_obj_unref(ctx, lexicon);
  }

  return ctx->rc == GRN_SUCCESS;
}

uint32_t
grn_accessor_estimate_size_for_query(grn_ctx *ctx,
                                     grn_obj *accessor,
                                     grn_obj *query,
                                     grn_search_optarg *optarg)
{
  GRN_API_ENTER;

  if (!grn_obj_is_accessor(ctx, accessor)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, accessor);
    ERR(GRN_INVALID_ARGUMENT,
        "[accessor][estimate-size][query] must be accessor: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(0);
  }

  if (!query) {
    GRN_API_RETURN(0);
  }

  grn_accessor_estimate_size_for_query_data data;
  data.accessor = accessor;
  data.query = query;
  data.optarg = optarg;
  data.index = NULL;
  data.index_need_unref = false;
  data.operator = GRN_OP_MATCH;
  GRN_VOID_INIT(&(data.query_casted));
  data.query_raw = NULL;
  data.query_raw_len = 0;
  data.n_target_records = 0;
  data.depth = 0;

  if (!grn_accessor_estimate_size_for_query_prepare(ctx, &data)) {
    grn_accessor_estimate_size_for_query_data_fin(ctx, &data);
    GRN_API_RETURN(0);
  }

  uint32_t estimated_size = 0;
  if (data.n_base_records == 0) {
    estimated_size = 0;
  } else {
    uint32_t base_estimated_size;
    if (grn_obj_is_table(ctx, data.index)) {
      if (data.operator == GRN_OP_EQUAL) {
        if (grn_table_get(ctx,
                          data.index,
                          data.query_raw,
                          data.query_raw_len)) {
          base_estimated_size = 1;
        } else {
          base_estimated_size = 0;
        }
      } else {
        /* TODO */
        base_estimated_size = data.n_base_records;
      }
    } else {
      base_estimated_size =
        grn_ii_estimate_size_for_query(ctx,
                                       (grn_ii *)(data.index),
                                       data.query_raw,
                                       data.query_raw_len,
                                       optarg);
    }
    if (base_estimated_size >= data.n_base_records) {
      estimated_size = data.n_target_records;
    } else {
      estimated_size =
        data.n_target_records *
        (base_estimated_size / (double)(data.n_base_records));
    }
  }

  grn_accessor_estimate_size_for_query_data_fin(ctx, &data);

  GRN_API_RETURN(estimated_size);
}

typedef enum {
  GRN_ACCESSOR_TO_STRING_USE_CASE_NAME,
  GRN_ACCESSOR_TO_STRING_USE_CASE_SCRIPT_SYNTAX,
} GRN_ACCESSOR_TO_STRING_USE_CASE;

static grn_rc
grn_accessor_to_string(grn_ctx *ctx,
                       grn_obj *accessor,
                       grn_obj *buffer,
                       GRN_ACCESSOR_TO_STRING_USE_CASE use_case,
                       const char *tag)
{
  grn_accessor *accessor_;
  GRN_API_ENTER;

  bool need_context = true;
  bool need_preceding_keys = true;
  if (use_case == GRN_ACCESSOR_TO_STRING_USE_CASE_SCRIPT_SYNTAX) {
    need_context =false;
    need_preceding_keys = false;
  }

  if (!grn_obj_is_accessor(ctx, accessor)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, accessor);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be accessor: %.*s",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  uint32_t n_components = 0;
  for (accessor_ = (grn_accessor *)accessor;
       accessor_;
       accessor_ = accessor_->next) {
    bool need_continue = false;
    grn_bool show_obj_name = GRN_FALSE;
    grn_bool show_obj_domain_name = GRN_FALSE;

    if (n_components > 0) {
      GRN_TEXT_PUTS(ctx, buffer, ".");
    }
    switch (accessor_->action) {
    case GRN_ACCESSOR_GET_ID :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_ID,
                   GRN_COLUMN_NAME_ID_LEN);
      show_obj_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_KEY :
      if (!need_preceding_keys && accessor_->next) {
        need_continue = true;
      } else {
        GRN_TEXT_PUT(ctx,
                     buffer,
                     GRN_COLUMN_NAME_KEY,
                     GRN_COLUMN_NAME_KEY_LEN);
        show_obj_name = GRN_TRUE;
      }
      break;
    case GRN_ACCESSOR_GET_VALUE :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_VALUE,
                   GRN_COLUMN_NAME_VALUE_LEN);
      show_obj_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_SCORE :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_SCORE,
                   GRN_COLUMN_NAME_SCORE_LEN);
      break;
    case GRN_ACCESSOR_GET_NSUBRECS :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_NSUBRECS,
                   GRN_COLUMN_NAME_NSUBRECS_LEN);
      break;
    case GRN_ACCESSOR_GET_MAX :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_MAX,
                   GRN_COLUMN_NAME_MAX_LEN);
      break;
    case GRN_ACCESSOR_GET_MIN :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_MIN,
                   GRN_COLUMN_NAME_MIN_LEN);
      break;
    case GRN_ACCESSOR_GET_SUM :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_SUM,
                   GRN_COLUMN_NAME_SUM_LEN);
      break;
    case GRN_ACCESSOR_GET_AVG :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_AVG,
                   GRN_COLUMN_NAME_AVG_LEN);
      break;
    case GRN_ACCESSOR_GET_MEAN :
      GRN_TEXT_PUT(ctx,
                   buffer,
                   GRN_COLUMN_NAME_MEAN,
                   GRN_COLUMN_NAME_MEAN_LEN);
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      grn_column_name_(ctx, accessor_->obj, buffer);
      show_obj_domain_name = GRN_TRUE;
      break;
    case GRN_ACCESSOR_GET_DB_OBJ :
      grn_text_printf(ctx, buffer, "(_db)");
      break;
    case GRN_ACCESSOR_LOOKUP :
      grn_text_printf(ctx, buffer, "(_lookup)");
      break;
    case GRN_ACCESSOR_FUNCALL :
      grn_text_printf(ctx, buffer, "(_funcall)");
      break;
    default :
      grn_text_printf(ctx, buffer, "(unknown:%u)", accessor_->action);
      break;
    }

    if (need_continue) {
      continue;
    }

    if (need_context && (show_obj_name || show_obj_domain_name)) {
      grn_obj *target = accessor_->obj;
      char target_name[GRN_TABLE_MAX_KEY_SIZE];
      int target_name_size;

      if (show_obj_domain_name) {
        target = grn_ctx_at(ctx, target->header.domain);
      }

      target_name_size = grn_obj_name(ctx,
                                      target,
                                      target_name,
                                      GRN_TABLE_MAX_KEY_SIZE);
      GRN_TEXT_PUTS(ctx, buffer, "(");
      if (target_name_size == 0) {
        GRN_TEXT_PUTS(ctx, buffer, "anonymous");
      } else {
        GRN_TEXT_PUT(ctx, buffer, target_name, target_name_size);
      }
      GRN_TEXT_PUTS(ctx, buffer, ")");
      if (show_obj_domain_name && target) {
        grn_obj_unref(ctx, target);
      }
    }
    n_components++;
  }

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_accessor_name(grn_ctx *ctx, grn_obj *accessor, grn_obj *name)
{
  GRN_API_ENTER;
  grn_rc rc = grn_accessor_to_string(ctx,
                                     accessor,
                                     name,
                                     GRN_ACCESSOR_TO_STRING_USE_CASE_NAME,
                                     "[accessor][name]");
  GRN_API_RETURN(rc);
}

grn_rc
grn_accessor_to_script_syntax(grn_ctx *ctx, grn_obj *accessor, grn_obj *buffer)
{
  GRN_API_ENTER;
  grn_rc rc =
    grn_accessor_to_string(ctx,
                           accessor,
                           buffer,
                           GRN_ACCESSOR_TO_STRING_USE_CASE_SCRIPT_SYNTAX,
                           "[accessor][to-script-syntax]");
  GRN_API_RETURN(rc);
}
