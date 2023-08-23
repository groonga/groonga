/*
  Copyright (C) 2015-2018  Brazil
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

#include "grn.h"
#include "grn_accessor.h"
#include "grn_ctx_impl.h"
#include "grn_expr.h"
#include "grn_dat.h"
#include "grn_float.h"
#include "grn_hash.h"
#include "grn_ii.h"
#include "grn_index_column.h"
#include "grn_pat.h"
#include "grn_store.h"
#include "grn_table.h"
#include "grn_token_column.h"

const char *
grn_obj_set_flag_to_string(int flags)
{
  const char *message = "invalid set flag";

  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_SET:
    message = "set";
    break;
  case GRN_OBJ_INCR:
    message = "increment";
    break;
  case GRN_OBJ_DECR:
    message = "decrement";
    break;
  case GRN_OBJ_APPEND:
    message = "append";
    break;
  case GRN_OBJ_PREPEND:
    message = "prepend";
    break;
  default:
    break;
  }

  return message;
}

bool
grn_obj_is_true(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  switch (obj->header.type) {
  case GRN_BULK:
    switch (obj->header.domain) {
    case GRN_DB_BOOL:
      return GRN_BOOL_VALUE(obj);
    case GRN_DB_INT32:
      return GRN_INT32_VALUE(obj) != 0;
    case GRN_DB_UINT32:
      return GRN_UINT32_VALUE(obj) != 0;
    case GRN_DB_FLOAT32:
      {
        float float_value;
        float_value = GRN_FLOAT32_VALUE(obj);
        return grn_float32_is_zero(float_value);
      }
    case GRN_DB_FLOAT:
      {
        double float_value;
        float_value = GRN_FLOAT_VALUE(obj);
        return grn_float_is_zero(float_value);
      }
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT:
      return GRN_TEXT_LEN(obj) != 0;
    default:
      if (grn_id_maybe_table(ctx, obj->header.domain)) {
        return GRN_BULK_VSIZE(obj) > 0 && GRN_PTR_VALUE(obj) != GRN_ID_NIL;
      } else {
        return false;
      }
    }
    break;
  case GRN_VECTOR:
    if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
      return true;
    } else {
      return grn_vector_size(ctx, obj) > 0;
    }
  case GRN_UVECTOR:
    if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
      return true;
    } else {
      return grn_uvector_size(ctx, obj) > 0;
    }
  default:
    return false;
  }
}

bool
grn_obj_is_temporary(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  if (!GRN_DB_OBJP(obj)) {
    return false;
  }

  if (DB_OBJ(obj)->id & GRN_OBJ_TMP_OBJECT) {
    return true;
  }

  if (DB_OBJ(obj)->id == GRN_ID_NIL) {
    return true;
  }

  return false;
}

grn_bool
grn_obj_is_builtin(grn_ctx *ctx, grn_obj *obj)
{
  grn_id id;

  if (!obj) {
    return GRN_FALSE;
  }

  id = grn_obj_id(ctx, obj);
  return grn_id_is_builtin(ctx, id);
}

grn_bool
grn_obj_is_bulk(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_BULK;
}

grn_bool
grn_obj_is_text_family_bulk(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_bulk(ctx, obj)) {
    return GRN_FALSE;
  }

  return GRN_TYPE_IS_TEXT_FAMILY(obj->header.domain);
}

grn_bool
grn_obj_is_number_family_bulk(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_bulk(ctx, obj)) {
    return GRN_FALSE;
  }

  return grn_type_id_is_number_family(ctx, obj->header.domain);
}

bool
grn_obj_is_vector(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  return obj->header.type == GRN_VECTOR;
}

bool
grn_obj_is_text_family_vector(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_vector(ctx, obj)) {
    return GRN_FALSE;
  }

  return grn_type_id_is_text_family(ctx, obj->header.domain);
}

bool
grn_obj_is_weight_vector(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_vector(ctx, obj)) {
    return false;
  }

  return obj->header.flags & GRN_OBJ_WITH_WEIGHT;
}

bool
grn_obj_is_uvector(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  return obj->header.type == GRN_UVECTOR;
}

bool
grn_obj_is_weight_uvector(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_uvector(ctx, obj)) {
    return false;
  }

  return obj->header.flags & GRN_OBJ_WITH_WEIGHT;
}

bool
grn_obj_is_db(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  return obj->header.type == GRN_DB;
}

grn_bool
grn_obj_is_table(grn_ctx *ctx, grn_obj *obj)
{
  grn_bool is_table = GRN_FALSE;

  if (!obj) {
    return GRN_FALSE;
  }

  switch (obj->header.type) {
  case GRN_TABLE_NO_KEY:
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    is_table = GRN_TRUE;
    break;
  default:
    break;
  }

  return is_table;
}

bool
grn_obj_is_table_with_key(grn_ctx *ctx, grn_obj *obj)
{
  grn_bool is_table_with_key = false;

  if (!obj) {
    return false;
  }

  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    is_table_with_key = true;
  default:
    break;
  }

  return is_table_with_key;
}

bool
grn_obj_is_table_with_value(grn_ctx *ctx, grn_obj *obj)
{
  grn_bool is_table_with_value = false;

  if (!obj) {
    return false;
  }

  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    is_table_with_value = DB_OBJ(obj)->range != GRN_ID_NIL;
  default:
    break;
  }

  return is_table_with_value;
}

bool
grn_obj_is_lexicon(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_table_with_key(ctx, obj)) {
    return false;
  }

  grn_hash *columns = grn_hash_create(ctx,
                                      NULL,
                                      sizeof(grn_id),
                                      0,
                                      GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    return false;
  }
  grn_table_columns(ctx, obj, "", 0, (grn_obj *)columns);
  bool is_lexicon = false;
  if (grn_hash_size(ctx, columns) > 0) {
    GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id)
    {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id column_id = *((grn_id *)key);
      grn_obj *column = grn_ctx_at(ctx, column_id);
      if (grn_obj_is_index_column(ctx, column)) {
        is_lexicon = true;
      }
      grn_obj_unref(ctx, column);
      if (is_lexicon) {
        break;
      }
    }
    GRN_HASH_EACH_END(ctx, cursor);
  }
  grn_hash_close(ctx, columns);
  return is_lexicon;
}

bool
grn_obj_is_lexicon_without_data_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_table_with_key(ctx, obj)) {
    return false;
  }

  grn_hash *columns = grn_hash_create(ctx,
                                      NULL,
                                      sizeof(grn_id),
                                      0,
                                      GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    return false;
  }
  grn_table_columns(ctx, obj, "", 0, (grn_obj *)columns);
  bool is_lexicon_without_data_column;
  if (grn_hash_size(ctx, columns) == 0) {
    is_lexicon_without_data_column = false;
  } else {
    is_lexicon_without_data_column = true;
    GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id)
    {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id column_id = *((grn_id *)key);
      grn_obj *column = grn_ctx_at(ctx, column_id);
      if (!grn_obj_is_index_column(ctx, column)) {
        is_lexicon_without_data_column = false;
      }
      grn_obj_unref(ctx, column);
      if (!is_lexicon_without_data_column) {
        break;
      }
    }
    GRN_HASH_EACH_END(ctx, cursor);
  }
  grn_hash_close(ctx, columns);
  return is_lexicon_without_data_column;
}

bool
grn_obj_is_tiny_hash_table(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  if (obj->header.type != GRN_TABLE_HASH_KEY) {
    return false;
  }

  return obj->header.flags & GRN_HASH_TINY;
}

bool
grn_obj_is_patricia_trie(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  return obj->header.type == GRN_TABLE_PAT_KEY;
}

bool
grn_obj_is_result_set(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_temporary(ctx, obj)) {
    return false;
  }

  if (obj->header.type != GRN_TABLE_HASH_KEY) {
    return false;
  }

  grn_table_flags flags = 0;
  grn_table_get_info(ctx, obj, &flags, NULL, NULL, NULL, NULL);
  return flags & GRN_OBJ_WITH_SUBREC;
}

grn_bool
grn_obj_is_column(grn_ctx *ctx, grn_obj *obj)
{
  grn_bool is_column = GRN_FALSE;

  if (!obj) {
    return GRN_FALSE;
  }

  switch (obj->header.type) {
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_INDEX:
    is_column = GRN_TRUE;
  default:
    break;
  }

  return is_column;
}

bool
grn_obj_is_number_family_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_column(ctx, obj)) {
    return false;
  }

  return grn_type_id_is_number_family(ctx, grn_obj_get_range(ctx, obj));
}

grn_bool
grn_obj_is_scalar_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_column(ctx, obj)) {
    return GRN_FALSE;
  }

  return (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) ==
         GRN_OBJ_COLUMN_SCALAR;
}

bool
grn_obj_is_text_family_scalar_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_scalar_column(ctx, obj)) {
    return false;
  }

  return grn_type_id_is_text_family(ctx, grn_obj_get_range(ctx, obj));
}

bool
grn_obj_is_number_family_scalar_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_scalar_column(ctx, obj)) {
    return false;
  }

  return grn_type_id_is_number_family(ctx, grn_obj_get_range(ctx, obj));
}

grn_bool
grn_obj_is_vector_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_column(ctx, obj)) {
    return GRN_FALSE;
  }

  return (
    (obj->header.type == GRN_COLUMN_VAR_SIZE) &&
    ((obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR));
}

bool
grn_obj_is_text_family_vector_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_vector_column(ctx, obj)) {
    return false;
  }

  return grn_type_id_is_text_family(ctx, grn_obj_get_range(ctx, obj));
}

grn_bool
grn_obj_is_weight_vector_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_vector_column(ctx, obj)) {
    return GRN_FALSE;
  }

  return (obj->header.flags & GRN_OBJ_WITH_WEIGHT) == GRN_OBJ_WITH_WEIGHT;
}

bool
grn_obj_is_reference_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_column(ctx, obj)) {
    return GRN_FALSE;
  }

  const grn_id range_id = grn_obj_get_range(ctx, obj);
  if (grn_id_is_builtin(ctx, range_id)) {
    return GRN_FALSE;
  }

  grn_obj *range = range = grn_ctx_at(ctx, range_id);
  if (!range) {
    return GRN_FALSE;
  }

  uint8_t range_type = range->header.type;
  grn_obj_unref(ctx, range);

  switch (range_type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    return true;
  default:
    return false;
  }
}

grn_bool
grn_obj_is_data_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_column(ctx, obj)) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_COLUMN_FIX_SIZE ||
         obj->header.type == GRN_COLUMN_VAR_SIZE;
}

grn_bool
grn_obj_is_index_column(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_column(ctx, obj)) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_COLUMN_INDEX;
}

grn_bool
grn_obj_is_accessor(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_ACCESSOR;
}

grn_bool
grn_obj_is_id_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_ID;
}

grn_bool
grn_obj_is_key_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_KEY;
}

bool
grn_obj_is_value_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_VALUE;
}

bool
grn_obj_is_score_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_SCORE;
}

bool
grn_obj_is_referable_score_accessor(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_score_accessor(ctx, obj)) {
    return false;
  }

  grn_accessor *accessor = (grn_accessor *)obj;
  grn_id parent_domain = accessor->obj->header.domain;
  if (!(parent_domain & GRN_OBJ_TMP_OBJECT)) {
    return true;
  }

  bool need_recursive_collect = false;
  grn_obj *parent = grn_ctx_at(ctx, parent_domain);
  if (parent) {
    if (parent->header.flags & GRN_OBJ_WITH_SUBREC) {
      need_recursive_collect = true;
    }
    grn_obj_unref(ctx, parent);
  }

  return !need_recursive_collect;
}

bool
grn_obj_is_nsubrecs_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_NSUBRECS;
}

bool
grn_obj_is_max_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_MAX;
}

bool
grn_obj_is_min_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_MIN;
}

bool
grn_obj_is_sum_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_SUM;
}

bool
grn_obj_is_avg_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_AVG;
}

bool
grn_obj_is_mean_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return false;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return false;
  }

  return accessor->action == GRN_ACCESSOR_GET_MEAN;
}

bool
grn_obj_is_column_value_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_COLUMN_VALUE;
}

bool
grn_obj_is_number_family_scalar_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return false;
  }

  accessor = (grn_accessor *)obj;
  while (accessor->next) {
    accessor = accessor->next;
  }

  switch (accessor->action) {
  case GRN_ACCESSOR_GET_ID:
  case GRN_ACCESSOR_GET_SCORE:
  case GRN_ACCESSOR_GET_NSUBRECS:
  case GRN_ACCESSOR_GET_MAX:
  case GRN_ACCESSOR_GET_MIN:
  case GRN_ACCESSOR_GET_SUM:
  case GRN_ACCESSOR_GET_AVG:
  case GRN_ACCESSOR_GET_MEAN:
    return true;
  case GRN_ACCESSOR_GET_VALUE:
    return grn_type_id_is_number_family(ctx,
                                        grn_obj_get_range(ctx, accessor->obj));
  case GRN_ACCESSOR_GET_COLUMN_VALUE:
    return grn_obj_is_number_family_scalar_column(ctx, accessor->obj);
  default:
    return false;
  }
}

grn_bool
grn_obj_is_type(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_TYPE;
}

grn_bool
grn_obj_is_text_family_type(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_obj_is_type(ctx, obj)) {
    return GRN_FALSE;
  }

  return GRN_TYPE_IS_TEXT_FAMILY(grn_obj_id(ctx, obj));
}

grn_bool
grn_obj_is_proc(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_PROC;
}

grn_bool
grn_obj_is_tokenizer_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_TOKENIZER;
}

grn_bool
grn_obj_is_function_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_FUNCTION;
}

grn_bool
grn_obj_is_selector_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_function_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->callbacks.function.selector != NULL;
}

grn_bool
grn_obj_is_selector_only_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_selector_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->funcs[PROC_INIT] == NULL;
}

grn_bool
grn_obj_is_normalizer_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_NORMALIZER;
}

grn_bool
grn_obj_is_token_filter_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_TOKEN_FILTER;
}

grn_bool
grn_obj_is_scorer_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_SCORER;
}

grn_bool
grn_obj_is_window_function_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_WINDOW_FUNCTION;
}

bool
grn_obj_is_aggregator_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return false;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_AGGREGATOR;
}

grn_bool
grn_obj_is_expr(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_EXPR;
}

bool
grn_obj_is_visible(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  uint32_t flags = 0;
  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
    flags = ((grn_hash *)obj)->header.common->flags;
    break;
  case GRN_TABLE_PAT_KEY:
    flags = ((grn_pat *)obj)->header->flags;
    break;
  case GRN_TABLE_DAT_KEY:
    flags = ((grn_dat *)obj)->header->flags;
    break;
  case GRN_TABLE_NO_KEY:
    flags = grn_array_get_flags(ctx, ((grn_array *)obj));
    break;
  case GRN_COLUMN_FIX_SIZE:
    flags = ((grn_ra *)obj)->header->flags;
    break;
  case GRN_COLUMN_VAR_SIZE:
    flags = grn_ja_get_flags(ctx, (grn_ja *)obj);
    break;
  case GRN_COLUMN_INDEX:
    flags = ((grn_ii *)obj)->header.common->flags;
    break;
  default:
    return false;
    break;
  }

  return !(flags & GRN_OBJ_INVISIBLE);
}

grn_rc
grn_obj_set_visibility(grn_ctx *ctx, grn_obj *obj, bool is_visible)
{
  GRN_API_ENTER;

  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "[obj][set-visibility] must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (!(grn_obj_is_index_column(ctx, obj) ||
        grn_obj_is_token_column(ctx, obj))) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, obj);
    ERR(GRN_INVALID_ARGUMENT,
        "[obj][set-visibility] "
        "must be an index column or a token column: <%.*s>",
        (int)(GRN_TEXT_LEN(&inspected)),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  if (grn_obj_is_index_column(ctx, obj)) {
    grn_ii_set_visibility(ctx, (grn_ii *)obj, is_visible);
  } else if (grn_obj_is_token_column(ctx, obj)) {
    grn_ja_set_visibility(ctx, (grn_ja *)obj, is_visible);
  }

  GRN_API_RETURN(GRN_SUCCESS);
}

bool
grn_obj_have_source(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  if (!GRN_DB_OBJP(obj)) {
    return false;
  }

  return DB_OBJ(obj)->source_size > 0;
}

bool
grn_obj_is_token_column(grn_ctx *ctx, grn_obj *obj)
{
  return grn_obj_is_vector_column(ctx, obj) && grn_obj_have_source(ctx, obj);
}

static void
grn_db_reindex(grn_ctx *ctx, grn_obj *db)
{
  grn_table_cursor *cursor;
  grn_id id;

  cursor =
    grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
  if (!cursor) {
    return;
  }

  while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    grn_obj *object;

    object = grn_ctx_at(ctx, id);
    if (!object) {
      ERRCLR(ctx);
      continue;
    }

    switch (object->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      grn_obj_reindex(ctx, object);
      break;
    default:
      break;
    }

    grn_obj_unlink(ctx, object);

    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  grn_table_cursor_close(ctx, cursor);
}

static void
grn_table_reindex(grn_ctx *ctx, grn_obj *table)
{
  grn_hash *columns;

  columns = grn_hash_create(ctx,
                            NULL,
                            sizeof(grn_id),
                            0,
                            GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[table][reindex] failed to create a table to store columns");
    return;
  }

  if (grn_table_columns(ctx, table, "", 0, (grn_obj *)columns) > 0) {
    grn_id *key;
    GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
      grn_obj *column = grn_ctx_at(ctx, *key);
      if (column && column->header.type == GRN_COLUMN_INDEX) {
        grn_obj_reindex(ctx, column);
      }
    });
  }
  grn_hash_close(ctx, columns);
}

static void
grn_data_column_reindex(grn_ctx *ctx, grn_obj *data_column)
{
  grn_hook *hooks;

  if (grn_obj_is_token_column(ctx, data_column)) {
    grn_token_column_build(ctx, data_column);
  }

  for (hooks = DB_OBJ(data_column)->hooks[GRN_HOOK_SET]; hooks;
       hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    if (target->header.type != GRN_COLUMN_INDEX) {
      continue;
    }
    grn_obj_reindex(ctx, target);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
}

grn_rc
grn_obj_reindex(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;

  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "[object][reindex] object must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }

  switch (obj->header.type) {
  case GRN_DB:
    grn_db_reindex(ctx, obj);
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    grn_table_reindex(ctx, obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
    grn_data_column_reindex(ctx, obj);
    break;
  case GRN_COLUMN_INDEX:
    grn_index_column_rebuild(ctx, obj);
    break;
  default:
    {
      grn_obj type_name;
      GRN_TEXT_INIT(&type_name, 0);
      grn_inspect_type(ctx, &type_name, obj->header.type);
      ERR(GRN_INVALID_ARGUMENT,
          "[object][reindex] object must be TABLE_HASH_KEY, "
          "TABLE_PAT_KEY, TABLE_DAT_KEY or COLUMN_INDEX: <%.*s>",
          (int)GRN_TEXT_LEN(&type_name),
          GRN_TEXT_VALUE(&type_name));
      GRN_OBJ_FIN(ctx, &type_name);
      GRN_API_RETURN(ctx->rc);
    }
    break;
  }

  GRN_API_RETURN(ctx->rc);
}

const char *
grn_obj_type_to_string(uint8_t type)
{
  switch (type) {
  case GRN_VOID:
    return "void";
  case GRN_BULK:
    return "bulk";
  case GRN_PTR:
    return "ptr";
  case GRN_UVECTOR:
    return "uvector";
  case GRN_PVECTOR:
    return "pvector";
  case GRN_VECTOR:
    return "vector";
  case GRN_MSG:
    return "msg";
  case GRN_QUERY:
    return "query";
  case GRN_ACCESSOR:
    return "accessor";
  case GRN_SNIP:
    return "snip";
  case GRN_PATSNIP:
    return "patsnip";
  case GRN_STRING:
    return "string";
  case GRN_CURSOR_TABLE_HASH_KEY:
    return "cursor:table:hash_key";
  case GRN_CURSOR_TABLE_PAT_KEY:
    return "cursor:table:pat_key";
  case GRN_CURSOR_TABLE_DAT_KEY:
    return "cursor:table:dat_key";
  case GRN_CURSOR_TABLE_NO_KEY:
    return "cursor:table:no_key";
  case GRN_CURSOR_COLUMN_INDEX:
    return "cursor:column:index";
  case GRN_CURSOR_COLUMN_GEO_INDEX:
    return "cursor:column:geo_index";
  case GRN_CURSOR_CONFIG:
    return "cursor:config";
  case GRN_TYPE:
    return "type";
  case GRN_PROC:
    return "proc";
  case GRN_EXPR:
    return "expr";
  case GRN_TABLE_HASH_KEY:
    return "table:hash_key";
  case GRN_TABLE_PAT_KEY:
    return "table:pat_key";
  case GRN_TABLE_DAT_KEY:
    return "table:dat_key";
  case GRN_TABLE_NO_KEY:
    return "table:no_key";
  case GRN_DB:
    return "db";
  case GRN_COLUMN_FIX_SIZE:
    return "column:fix_size";
  case GRN_COLUMN_VAR_SIZE:
    return "column:var_size";
  case GRN_COLUMN_INDEX:
    return "column:index";
  default:
    return "unknown";
  }
}

grn_bool
grn_obj_name_is_column(grn_ctx *ctx, const char *name, int name_len)
{
  if (!name) {
    return GRN_FALSE;
  }

  if (name_len < 0) {
    name_len = (int)strlen(name);
  }

  return memchr(name, GRN_DB_DELIMITER, (size_t)name_len) != NULL;
}

grn_io *
grn_obj_get_io(grn_ctx *ctx, grn_obj *obj)
{
  grn_io *io = NULL;

  if (!obj) {
    return NULL;
  }

  if (obj->header.type == GRN_DB) {
    obj = ((grn_db *)obj)->keys;
  }

  switch (obj->header.type) {
  case GRN_TABLE_PAT_KEY:
    io = ((grn_pat *)obj)->io;
    break;
  case GRN_TABLE_DAT_KEY:
    io = ((grn_dat *)obj)->io;
    break;
  case GRN_TABLE_HASH_KEY:
    io = ((grn_hash *)obj)->io;
    break;
  case GRN_TABLE_NO_KEY:
    io = ((grn_array *)obj)->io;
    break;
  case GRN_COLUMN_VAR_SIZE:
    io = ((grn_ja *)obj)->io;
    break;
  case GRN_COLUMN_FIX_SIZE:
    io = ((grn_ra *)obj)->io;
    break;
  case GRN_COLUMN_INDEX:
    io = ((grn_ii *)obj)->seg;
    break;
  }

  return io;
}

size_t
grn_obj_get_disk_usage(grn_ctx *ctx, grn_obj *obj)
{
  size_t usage = 0;

  GRN_API_ENTER;

  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "[object][disk-usage] object must not be NULL");
    GRN_API_RETURN(0);
  }

  switch (obj->header.type) {
  case GRN_DB:
    {
      grn_db *db = (grn_db *)obj;
      usage = grn_obj_get_disk_usage(ctx, db->keys);
      if (db->specs) {
        usage += grn_obj_get_disk_usage(ctx, (grn_obj *)(db->specs));
      }
      usage += grn_obj_get_disk_usage(ctx, (grn_obj *)(db->config));
    }
    break;
  case GRN_TABLE_DAT_KEY:
    usage = grn_dat_get_disk_usage(ctx, (grn_dat *)obj);
    break;
  case GRN_COLUMN_INDEX:
    usage = grn_ii_get_disk_usage(ctx, (grn_ii *)obj);
    break;
  default:
    {
      grn_io *io;
      io = grn_obj_get_io(ctx, obj);
      if (io) {
        usage = grn_io_get_disk_usage(ctx, io);
      }
    }
    break;
  }

  GRN_API_RETURN(usage);
}

grn_rc
grn_obj_set_option_values(grn_ctx *ctx,
                          grn_obj *obj,
                          const char *name,
                          int name_length,
                          grn_obj *values)
{
  grn_id id;

  GRN_API_ENTER;

  id = grn_obj_id(ctx, obj);
  if (id & GRN_OBJ_TMP_OBJECT) {
    grn_options_set(ctx,
                    ctx->impl->temporary_options,
                    id & ~GRN_OBJ_TMP_OBJECT,
                    name,
                    name_length,
                    values);
  } else {
    grn_db_set_option_values(ctx,
                             grn_ctx_db(ctx),
                             id,
                             name,
                             name_length,
                             values);
  }

  GRN_API_RETURN(ctx->rc);
}

grn_option_revision
grn_obj_get_option_values(grn_ctx *ctx,
                          grn_obj *obj,
                          const char *name,
                          int name_length,
                          grn_option_revision revision,
                          grn_obj *values)
{
  grn_id id;
  grn_option_revision returned_revision;

  GRN_API_ENTER;

  id = grn_obj_id(ctx, obj);
  if (id & GRN_OBJ_TMP_OBJECT) {
    returned_revision = grn_options_get(ctx,
                                        ctx->impl->temporary_options,
                                        id & ~GRN_OBJ_TMP_OBJECT,
                                        name,
                                        name_length,
                                        revision,
                                        values);
  } else {
    returned_revision = grn_db_get_option_values(ctx,
                                                 grn_ctx_db(ctx),
                                                 id,
                                                 name,
                                                 name_length,
                                                 revision,
                                                 values);
  }

  GRN_API_RETURN(returned_revision);
}

grn_rc
grn_obj_clear_option_values(grn_ctx *ctx, grn_obj *obj)
{
  grn_id id;
  grn_rc rc;

  GRN_API_ENTER;

  id = grn_obj_id(ctx, obj);
  if (id & GRN_OBJ_TMP_OBJECT) {
    rc = grn_options_clear(ctx,
                           ctx->impl->temporary_options,
                           id & ~GRN_OBJ_TMP_OBJECT);
  } else {
    rc = grn_db_clear_option_values(ctx, grn_ctx_db(ctx), id);
  }

  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_to_script_syntax(grn_ctx *ctx, grn_obj *obj, grn_obj *buffer)
{
  GRN_API_ENTER;
  switch (obj->header.type) {
  case GRN_VOID:
    GRN_TEXT_PUTS(ctx, buffer, "null");
    break;
  case GRN_BULK:
    grn_inspect(ctx, buffer, obj);
    break;
  case GRN_PTR:
    if (GRN_BULK_VSIZE(obj) == 0) {
      GRN_TEXT_PUTS(ctx, buffer, "null");
    } else {
      grn_obj_to_script_syntax(ctx, GRN_PTR_VALUE(obj), buffer);
    }
    break;
  case GRN_UVECTOR:
  case GRN_PVECTOR:
  case GRN_VECTOR:
    grn_inspect(ctx, buffer, obj);
    break;
  case GRN_ACCESSOR:
    grn_accessor_to_script_syntax(ctx, obj, buffer);
    break;
  case GRN_TYPE:
  case GRN_PROC:
    {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_length = grn_obj_name(ctx, obj, name, sizeof(name));
      GRN_TEXT_PUT(ctx, buffer, name, name_length);
    }
    break;
  case GRN_EXPR:
    grn_expr_to_script_syntax(ctx, obj, buffer);
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    if (grn_obj_is_tiny_hash_table(ctx, obj)) {
      GRN_TEXT_PUTC(ctx, buffer, '{');
      uint32_t i = 0;
      grn_obj key;
      GRN_TEXT_INIT(&key, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TABLE_EACH_BEGIN_FLAGS(ctx, obj, cursor, id, GRN_CURSOR_BY_ID)
      {
        if (i > 0) {
          GRN_TEXT_PUTS(ctx, buffer, ", ");
        }
        void *raw_key;
        int raw_key_size = grn_table_cursor_get_key(ctx, cursor, &raw_key);
        GRN_TEXT_SET(ctx, &key, raw_key, raw_key_size);
        grn_obj_to_script_syntax(ctx, &key, buffer);
        GRN_TEXT_PUTS(ctx, buffer, ": ");
        void *raw_value;
        grn_table_cursor_get_value(ctx, cursor, &raw_value);
        grn_obj *value = raw_value;
        grn_obj_to_script_syntax(ctx, value, buffer);
        i++;
      }
      GRN_TABLE_EACH_END(ctx, cursor);
      GRN_OBJ_FIN(ctx, &key);
      GRN_TEXT_PUTC(ctx, buffer, '}');
    } else {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_length = grn_obj_name(ctx, obj, name, sizeof(name));
      GRN_TEXT_PUT(ctx, buffer, name, name_length);
    }
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_INDEX:
    grn_column_name_(ctx, obj, buffer);
    break;
  default:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[obj][to-script-syntax] unsupported type: %s",
        grn_obj_type_to_string(obj->header.type));
    break;
  }
  GRN_API_RETURN(ctx->rc);
}

static void
grn_obj_warm_dispatch(grn_ctx *ctx, grn_obj *obj);

static void
grn_db_warm(grn_ctx *ctx, grn_obj *obj)
{
  grn_db *db = (grn_db *)obj;
  grn_obj_warm_dispatch(ctx, db->keys);
  if (grn_ja_warm(ctx, db->specs) != GRN_SUCCESS) {
    return;
  }
  if (grn_hash_warm(ctx, db->config) != GRN_SUCCESS) {
    return;
  }
  if (grn_options_warm(ctx, db->options) != GRN_SUCCESS) {
    return;
  }

  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, obj, cursor, id, GRN_CURSOR_BY_ID)
  {
    if (id < GRN_N_RESERVED_TYPES) {
      continue;
    }

    grn_obj *object = grn_ctx_at(ctx, id);
    if (!object) {
      continue;
    }
    if (grn_obj_is_table(ctx, object)) {
      grn_obj_warm_dispatch(ctx, object);
    }
    grn_obj_unref(ctx, object);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static void
grn_table_warm(grn_ctx *ctx, grn_obj *obj)
{
  grn_hash *columns = grn_table_all_columns(ctx, obj);
  if (!columns) {
    return;
  }

  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id)
  {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    grn_id column_id = *((grn_id *)key);
    grn_obj *column = grn_ctx_at(ctx, column_id);
    grn_obj_warm_dispatch(ctx, column);
    grn_obj_unref(ctx, column);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
}

static void
grn_obj_warm_dispatch(grn_ctx *ctx, grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_DB:
    grn_db_warm(ctx, obj);
    break;
  case GRN_TABLE_HASH_KEY:
    if (grn_hash_warm(ctx, (grn_hash *)obj) != GRN_SUCCESS) {
      return;
    }
    grn_table_warm(ctx, obj);
    break;
  case GRN_TABLE_PAT_KEY:
    if (grn_pat_warm(ctx, (grn_pat *)obj) != GRN_SUCCESS) {
      return;
    }
    grn_table_warm(ctx, obj);
    break;
  case GRN_TABLE_DAT_KEY:
    if (grn_dat_warm(ctx, (grn_dat *)obj) != GRN_SUCCESS) {
      return;
    }
    grn_table_warm(ctx, obj);
    break;
  case GRN_TABLE_NO_KEY:
    if (grn_array_warm(ctx, (grn_array *)obj) != GRN_SUCCESS) {
      return;
    }
    grn_table_warm(ctx, obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
    grn_ra_warm(ctx, (grn_ra *)obj);
    break;
  case GRN_COLUMN_VAR_SIZE:
    grn_ja_warm(ctx, (grn_ja *)obj);
    break;
  case GRN_COLUMN_INDEX:
    grn_ii_warm(ctx, (grn_ii *)obj);
    break;
  default:
    break;
  }
}

grn_rc
grn_obj_warm(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;
  grn_obj_warm_dispatch(ctx, obj);
  GRN_API_RETURN(ctx->rc);
}

void
grn_obj_set_error(grn_ctx *ctx,
                  grn_obj *obj,
                  grn_rc rc,
                  grn_id id,
                  const char *tag,
                  const char *format,
                  ...)
{
  grn_obj message;
  GRN_TEXT_INIT(&message, 0);
  va_list args;
  va_start(args, format);
  grn_text_printfv(ctx, &message, format, args);
  va_end(args);

  grn_io *io = grn_obj_get_io(ctx, obj);
  bool have_path = (io->path[0] != '\0');
  GRN_DEFINE_NAME(obj);
  if (id == GRN_ID_NIL) {
    ERR(rc,
        "%s[%.*s] %.*s%s%s%s",
        tag,
        name_size,
        name,
        (int)GRN_TEXT_LEN(&message),
        GRN_TEXT_VALUE(&message),
        have_path ? ": path:<" : "",
        have_path ? io->path : "",
        have_path ? ">" : "");
  } else {
    ERR(rc,
        "%s[%.*s][%u] %.*s%s%s%s",
        tag,
        name_size,
        name,
        id,
        (int)GRN_TEXT_LEN(&message),
        GRN_TEXT_VALUE(&message),
        have_path ? ": path:<" : "",
        have_path ? io->path : "",
        have_path ? ">" : "");
  }

  GRN_OBJ_FIN(ctx, &message);
}

void
grn_obj_log(grn_ctx *ctx,
            grn_obj *obj,
            grn_log_level level,
            grn_id id,
            const char *tag,
            const char *format,
            ...)
{
  if (!grn_logger_pass(ctx, level)) {
    return;
  }

  grn_obj message;
  GRN_TEXT_INIT(&message, 0);
  va_list args;
  va_start(args, format);
  grn_text_printfv(ctx, &message, format, args);
  va_end(args);

  grn_io *io = grn_obj_get_io(ctx, obj);
  bool have_path = (io->path[0] != '\0');
  GRN_DEFINE_NAME(obj);
  if (id == GRN_ID_NIL) {
    GRN_LOG(ctx,
            level,
            "%s[%.*s] %.*s%s%s%s",
            tag,
            name_size,
            name,
            (int)GRN_TEXT_LEN(&message),
            GRN_TEXT_VALUE(&message),
            have_path ? ": path:<" : "",
            have_path ? io->path : "",
            have_path ? ">" : "");
  } else {
    GRN_LOG(ctx,
            level,
            "%s[%.*s][%u] %.*s%s%s%s",
            tag,
            name_size,
            name,
            id,
            (int)GRN_TEXT_LEN(&message),
            GRN_TEXT_VALUE(&message),
            have_path ? ": path:<" : "",
            have_path ? io->path : "",
            have_path ? ">" : "");
  }

  GRN_OBJ_FIN(ctx, &message);
}
