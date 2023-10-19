/*
  Copyright (C) 2009-2017  Brazil
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

#include "grn_cast.h"
#include "grn_column.h"
#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_load.h"
#include "grn_obj.h"
#include "grn_table.h"
#include "grn_util.h"

#include <stdio.h>

void
grn_loader_save_error(grn_ctx *ctx, grn_loader *loader)
{
  loader->rc = ctx->rc;
  grn_strcpy(loader->errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
}

void
grn_loader_on_record_added(grn_ctx *ctx, grn_loader *loader, grn_id id)
{
  if (id == GRN_ID_NIL) {
    grn_loader_save_error(ctx, loader);
    loader->n_record_errors++;
  } else {
    loader->n_records++;
  }
  if (loader->output_ids) {
    GRN_UINT32_PUT(ctx, &(loader->ids), id);
  }
  if (loader->output_errors) {
    GRN_INT32_PUT(ctx, &(loader->return_codes), ctx->rc);
    grn_vector_add_element(ctx,
                           &(loader->error_messages),
                           ctx->errbuf,
                           (uint32_t)strlen(ctx->errbuf),
                           0,
                           GRN_DB_TEXT);
  }
  ERRCLR(ctx);
}

void
grn_loader_on_column_set(grn_ctx *ctx,
                         grn_loader *loader,
                         grn_loader_add_record_data *data)
{
  if (ctx->rc == GRN_SUCCESS) {
    return;
  }

  grn_loader_save_error(ctx, loader);

  grn_obj key_inspected;
  grn_obj value_inspected;
  GRN_TEXT_INIT(&key_inspected, 0);
  GRN_TEXT_INIT(&value_inspected, 0);
  if (data->key) {
    grn_inspect_limited(ctx, &key_inspected, data->key);
  } else {
    if (grn_obj_is_table_with_key(ctx, data->table)) {
      grn_obj key_value;
      GRN_OBJ_INIT(&key_value, GRN_BULK, 0, data->table->header.domain);
      grn_table_get_key2(ctx, data->table, data->id, &key_value);
      grn_inspect_limited(ctx, &key_inspected, &key_value);
      GRN_OBJ_FIN(ctx, &key_value);
    }
  }
  grn_inspect_limited(ctx, &value_inspected, data->current.value);
  if (GRN_TEXT_LEN(&key_inspected) > 0) {
    GRN_DEFINE_NAME(data->table);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "[table][load][%.*s][%.*s] failed to set column value: %s: "
            "key: <%.*s>, value: <%.*s>",
            name_size,
            name,
            (int)(data->current.column_name_size),
            data->current.column_name,
            ctx->errbuf,
            (int)GRN_TEXT_LEN(&key_inspected),
            GRN_TEXT_VALUE(&key_inspected),
            (int)GRN_TEXT_LEN(&value_inspected),
            GRN_TEXT_VALUE(&value_inspected));
  } else {
    GRN_DEFINE_NAME(data->table);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "[table][load][%.*s][%.*s] failed to set column value: %s: "
            "id: <%u>: value: <%.*s>",
            name_size,
            name,
            (int)(data->current.column_name_size),
            data->current.column_name,
            ctx->errbuf,
            data->id,
            (int)GRN_TEXT_LEN(&value_inspected),
            GRN_TEXT_VALUE(&value_inspected));
  }
  GRN_OBJ_FIN(ctx, &key_inspected);
  GRN_OBJ_FIN(ctx, &value_inspected);

  loader->n_column_errors++;
  ERRCLR(ctx);
}

void
grn_loader_on_no_identifier_error(grn_ctx *ctx,
                                  grn_loader *loader,
                                  grn_obj *table)
{
  if (!table) {
    table = loader->table;
  }
  GRN_DEFINE_NAME(table);
  if (table->header.type == GRN_TABLE_NO_KEY) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][load][%.*s] <_id> isn't specified",
        name_size,
        name);
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][load][%.*s] neither <_key> nor <_id> is specified",
        name_size,
        name);
  }
}

grn_obj *
grn_loader_get_column(grn_ctx *ctx,
                      grn_loader *loader,
                      const char *name,
                      size_t name_length)
{
  if (!loader->columns) {
    loader->columns = grn_hash_create(ctx,
                                      NULL,
                                      GRN_TABLE_MAX_KEY_SIZE,
                                      sizeof(grn_obj *),
                                      GRN_OBJ_TABLE_HASH_KEY |
                                        GRN_OBJ_KEY_VAR_SIZE | GRN_HASH_TINY);
  }

  void *value;
  grn_id id =
    grn_hash_get(ctx, loader->columns, name, (unsigned int)name_length, &value);
  if (id != GRN_ID_NIL) {
    return *((grn_obj **)value);
  }

  grn_obj *column =
    grn_obj_column(ctx, loader->table, name, (uint32_t)name_length);
  if (!column) {
    return NULL;
  }

  id = grn_hash_add(ctx,
                    loader->columns,
                    name,
                    (unsigned int)name_length,
                    &value,
                    NULL);
  grn_memcpy(value, &column, sizeof(grn_obj *));
  grn_obj *range = grn_ctx_at(ctx, DB_OBJ(column)->range);
  if (grn_obj_is_table(ctx, range)) {
    GRN_PTR_PUT(ctx, &(loader->ranges), range);
    grn_column_get_all_index_columns(ctx, range, &(loader->indexes));
  }
  grn_column_get_all_index_columns(ctx, column, &(loader->indexes));

  return column;
}

static grn_obj *
values_add(grn_ctx *ctx, grn_loader *loader)
{
  grn_obj *res;
  uint32_t curr_size = loader->values_size * sizeof(grn_obj);
  if (curr_size < GRN_TEXT_LEN(&loader->values)) {
    res = (grn_obj *)(GRN_TEXT_VALUE(&loader->values) + curr_size);
    res->header.domain = GRN_DB_TEXT;
    GRN_BULK_REWIND(res);
  } else {
    if (grn_bulk_space(ctx, &loader->values, sizeof(grn_obj))) {
      return NULL;
    }
    res = (grn_obj *)(GRN_TEXT_VALUE(&loader->values) + curr_size);
    GRN_TEXT_INIT(res, 0);
  }
  loader->values_size++;
  loader->last = res;
  return res;
}

static grn_obj *
values_next(grn_ctx *ctx, grn_obj *value)
{
  if (value->header.domain == GRN_JSON_LOAD_OPEN_BRACKET ||
      value->header.domain == GRN_JSON_LOAD_OPEN_BRACE) {
    value += GRN_UINT32_VALUE(value);
  }
  return value + 1;
}

static uint32_t
grn_loader_bracket_count(grn_ctx *ctx, grn_obj *bracket_value)
{
  uint32_t size;
  grn_obj *head = bracket_value + 1;
  grn_obj *tail = head + GRN_UINT32_VALUE(bracket_value);
  for (size = 0; head < tail; head = values_next(ctx, head), size++) {
  }
  return size;
}

static grn_obj *
grn_loader_bracket_get(grn_ctx *ctx, grn_obj *bracket_value, uint32_t offset)
{
  uint32_t i;
  grn_obj *head = bracket_value + 1;
  grn_obj *tail = head + GRN_UINT32_VALUE(bracket_value);
  for (i = 0; head < tail; head = values_next(ctx, head), i++) {
    if (i == offset) {
      return head;
    }
  }
  return NULL;
}

grn_id
grn_loader_add_record(grn_ctx *ctx,
                      grn_loader *loader,
                      grn_loader_add_record_data *data)
{
  if (data->id != GRN_ID_NIL) {
    if (grn_table_at(ctx, data->table, data->id) == GRN_ID_NIL) {
      if (ctx->rc == GRN_SUCCESS) {
        data->id = grn_table_add(ctx, data->table, NULL, 0, NULL);
      }
    }
  } else if (data->key) {
    int added = 0;
    data->id = grn_table_add_by_key(ctx, data->table, data->key, &added);
    if (data->id == GRN_ID_NIL) {
      return GRN_ID_NIL;
    }
    if (!added && loader->ifexists) {
      grn_obj *v = grn_expr_get_var_by_offset(ctx, loader->ifexists, 0);
      GRN_RECORD_SET(ctx, v, data->id);
      grn_obj *result = grn_expr_exec(ctx, loader->ifexists, 0);
      if (!grn_obj_is_true(ctx, result)) {
        data->id = GRN_ID_NIL;
      }
    }
  } else {
    data->id = grn_table_add(ctx, data->table, NULL, 0, NULL);
  }
  return data->id;
}

static void
grn_loader_brace_add_weight_vector_element(grn_ctx *ctx,
                                           grn_loader *loader,
                                           grn_loader_add_record_data *data,
                                           grn_obj *vector,
                                           grn_obj *brace_value)
{
  grn_obj weight_buffer;
  GRN_FLOAT32_INIT(&weight_buffer, 0);

  grn_obj *value = brace_value + 1;
  grn_obj *value_end = value + GRN_UINT32_VALUE(brace_value);
  for (; value < value_end; value = values_next(ctx, value)) {
    grn_obj *key = value;
    if (value >= value_end) {
      /* TODO: Report no weight error. */
      break;
    }
    value = values_next(ctx, value);
    grn_obj *weight = value;

    GRN_BULK_REWIND(&weight_buffer);
    grn_rc rc = grn_obj_cast(ctx, weight, &weight_buffer, true);
    if (rc != GRN_SUCCESS) {
      grn_obj *range;
      range = grn_ctx_at(ctx, weight_buffer.header.domain);
      ERR_CAST(data->current.column, range, weight);
      grn_obj_unlink(ctx, range);
      break;
    }
    grn_vector_add_element_float(ctx,
                                 vector,
                                 GRN_BULK_HEAD(key),
                                 (uint32_t)GRN_BULK_VSIZE(key),
                                 GRN_FLOAT32_VALUE(&weight_buffer),
                                 key->header.domain);
  }
  GRN_OBJ_FIN(ctx, &weight_buffer);
}

static void
grn_loader_brace_add_record(grn_ctx *ctx,
                            grn_loader *loader,
                            grn_loader_add_record_data *data);

static void
grn_loader_bracket_set_value_reference(grn_ctx *ctx,
                                       grn_loader *loader,
                                       grn_loader_add_record_data *data)
{
  grn_obj *bracket_element = data->current.value;
  grn_obj *element = bracket_element + 1;
  grn_obj *element_end = element + GRN_UINT32_VALUE(bracket_element);

  grn_id range_id = DB_OBJ(data->current.column)->range;
  grn_obj *range = grn_ctx_at(ctx, range_id);

  grn_obj vector;
  GRN_RECORD_INIT(&vector, GRN_OBJ_VECTOR, range_id);
  for (; element < element_end; element = values_next(ctx, element)) {
    if (element->header.domain == GRN_JSON_LOAD_OPEN_BRACE) {
      grn_loader_add_record_data sub_data;
      sub_data.table = range;
      sub_data.depth = data->depth + 1;
      sub_data.record_value = element;
      sub_data.id = GRN_ID_NIL;
      sub_data.key = NULL;
      grn_loader_brace_add_record(ctx, loader, &sub_data);
      if (sub_data.id == GRN_ID_NIL) {
        goto exit;
      }
      GRN_RECORD_PUT(ctx, &vector, sub_data.id);
    } else if (element->header.domain == GRN_JSON_LOAD_OPEN_BRACKET) {
      GRN_DEFINE_NAME(data->table);
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, element);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s][%.*s] can't use [...] for reference column: "
          "%.*s",
          name_size,
          name,
          data->current.column_name_size,
          data->current.column_name,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    } else if (element->header.domain == range_id) {
      GRN_RECORD_PUT(ctx, &vector, GRN_RECORD_VALUE(element));
    } else {
      grn_obj values;
      if (grn_obj_is_text_family_bulk(ctx, element)) {
        GRN_TEXT_INIT(&values, GRN_OBJ_VECTOR);
        grn_vector_add_element_float(ctx,
                                     &values,
                                     GRN_TEXT_VALUE(element),
                                     (uint32_t)GRN_TEXT_LEN(element),
                                     0.0,
                                     element->header.domain);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&values,
                                GRN_OBJ_VECTOR,
                                element->header.domain);
        grn_bulk_write(ctx,
                       &values,
                       GRN_BULK_HEAD(element),
                       GRN_BULK_VSIZE(element));
      }
      grn_obj records;
      GRN_RECORD_INIT(&records, 0, range_id);
      grn_obj *casted_records = grn_column_cast_value(ctx,
                                                      data->current.column,
                                                      &values,
                                                      &records,
                                                      GRN_OBJ_SET);
      if (casted_records) {
        grn_uvector_copy(ctx, &records, &vector);
      }
      GRN_OBJ_FIN(ctx, &values);
      GRN_OBJ_FIN(ctx, &records);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }
  }

  grn_obj_set_value(ctx, data->current.column, data->id, &vector, GRN_OBJ_SET);

exit:
  grn_obj_unlink(ctx, range);
  GRN_OBJ_FIN(ctx, &vector);
}

static void
grn_loader_bracket_set_value_text(grn_ctx *ctx,
                                  grn_loader *loader,
                                  grn_loader_add_record_data *data)
{
  grn_obj *bracket_element = data->current.value;
  grn_obj *element = bracket_element + 1;
  grn_obj *element_end = element + GRN_UINT32_VALUE(bracket_element);

  grn_id range_id = DB_OBJ(data->current.column)->range;

  grn_obj vector;
  GRN_TEXT_INIT(&vector, GRN_OBJ_VECTOR);
  for (; element < element_end; element = values_next(ctx, element)) {
    if (element->header.domain == GRN_JSON_LOAD_OPEN_BRACE) {
      grn_loader_brace_add_weight_vector_element(ctx,
                                                 loader,
                                                 data,
                                                 &vector,
                                                 element);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    } else if (element->header.domain == GRN_JSON_LOAD_OPEN_BRACKET) {
      GRN_DEFINE_NAME(data->table);
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, element);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s][%.*s] "
          "can't use [...] for variable size data column: "
          "%.*s",
          name_size,
          name,
          data->current.column_name_size,
          data->current.column_name,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    } else if (grn_obj_is_text_family_bulk(ctx, element)) {
      grn_vector_add_element(ctx,
                             &vector,
                             GRN_TEXT_VALUE(element),
                             (uint32_t)GRN_TEXT_LEN(element),
                             0,
                             range_id);
    } else {
      grn_obj values;
      GRN_VALUE_FIX_SIZE_INIT(&values, GRN_OBJ_VECTOR, element->header.domain);
      grn_bulk_write(ctx,
                     &values,
                     GRN_BULK_HEAD(element),
                     GRN_BULK_VSIZE(element));
      grn_obj buffer;
      GRN_VOID_INIT(&buffer);
      grn_obj *casted_values = grn_column_cast_value(ctx,
                                                     data->current.column,
                                                     &values,
                                                     &buffer,
                                                     GRN_OBJ_SET);
      if (casted_values) {
        grn_vector_copy(ctx, casted_values, &vector);
      }
      GRN_OBJ_FIN(ctx, &values);
      GRN_OBJ_FIN(ctx, &buffer);
      if (ctx->rc != GRN_SUCCESS) {
        grn_loader_save_error(ctx, loader);
        ERRCLR(ctx);
      }
    }
  }

  grn_obj_set_value(ctx, data->current.column, data->id, &vector, GRN_OBJ_SET);

exit:
  GRN_OBJ_FIN(ctx, &vector);
}

static void
grn_loader_bracket_set_value_fix_size(grn_ctx *ctx,
                                      grn_loader *loader,
                                      grn_loader_add_record_data *data)
{
  grn_obj *bracket_element = data->current.value;
  grn_obj *element = bracket_element + 1;
  grn_obj *element_end = element + GRN_UINT32_VALUE(bracket_element);

  grn_id range_id = DB_OBJ(data->current.column)->range;

  grn_obj vector;
  GRN_VALUE_FIX_SIZE_INIT(&vector, GRN_OBJ_VECTOR, range_id);
  for (; element < element_end; element = values_next(ctx, element)) {
    if (element->header.domain == GRN_JSON_LOAD_OPEN_BRACE ||
        element->header.domain == GRN_JSON_LOAD_OPEN_BRACKET) {
      GRN_DEFINE_NAME(data->table);
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, element);
      const char *example;
      if (element->header.domain == GRN_JSON_LOAD_OPEN_BRACE) {
        example = "{...}";
      } else {
        example = "[...]";
      }
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s][%.*s] "
          "can't use %s for fixed size data column: "
          "%.*s",
          name_size,
          name,
          data->current.column_name_size,
          data->current.column_name,
          example,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    } else if (element->header.domain == range_id) {
      grn_bulk_write(ctx,
                     &vector,
                     GRN_BULK_HEAD(element),
                     GRN_BULK_VSIZE(element));
    } else {
      grn_obj values;
      if (grn_obj_is_text_family_bulk(ctx, element)) {
        GRN_TEXT_INIT(&values, GRN_OBJ_VECTOR);
        grn_vector_add_element_float(ctx,
                                     &values,
                                     GRN_TEXT_VALUE(element),
                                     (uint32_t)GRN_TEXT_LEN(element),
                                     0.0,
                                     element->header.domain);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&values,
                                GRN_OBJ_VECTOR,
                                element->header.domain);
        grn_bulk_write(ctx,
                       &values,
                       GRN_BULK_HEAD(element),
                       GRN_BULK_VSIZE(element));
      }
      grn_obj buffer;
      GRN_VOID_INIT(&buffer);
      grn_obj *casted_values = grn_column_cast_value(ctx,
                                                     data->current.column,
                                                     &values,
                                                     &buffer,
                                                     GRN_OBJ_SET);
      if (casted_values) {
        grn_uvector_copy(ctx, casted_values, &vector);
      }
      GRN_OBJ_FIN(ctx, &values);
      GRN_OBJ_FIN(ctx, &buffer);
      if (ctx->rc != GRN_SUCCESS) {
        grn_loader_save_error(ctx, loader);
        ERRCLR(ctx);
      }
    }
  }

  grn_obj_set_value(ctx, data->current.column, data->id, &vector, GRN_OBJ_SET);

exit:
  GRN_OBJ_FIN(ctx, &vector);
}

static void
grn_loader_bracket_set_value(grn_ctx *ctx,
                             grn_loader *loader,
                             grn_loader_add_record_data *data)
{
  if (!grn_obj_is_vector_column(ctx, data->current.column)) {
    GRN_DEFINE_NAME(data->table);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][load][%.*s][%.*s] [...] is only for vector column",
        name_size,
        name,
        (int)(data->current.column_name_size),
        data->current.column_name);
    return;
  }

  grn_id range_id = DB_OBJ(data->current.column)->range;
  if (grn_id_maybe_table(ctx, range_id)) {
    grn_loader_bracket_set_value_reference(ctx, loader, data);
  } else if (grn_type_id_is_text_family(ctx, range_id)) {
    grn_loader_bracket_set_value_text(ctx, loader, data);
  } else {
    grn_loader_bracket_set_value_fix_size(ctx, loader, data);
  }
}

static void
grn_loader_brace_set_value(grn_ctx *ctx,
                           grn_loader *loader,
                           grn_loader_add_record_data *data)
{
  if (grn_obj_is_weight_vector_column(ctx, data->current.column)) {
    grn_obj vector;
    GRN_TEXT_INIT(&vector, GRN_OBJ_VECTOR);
    grn_loader_brace_add_weight_vector_element(ctx,
                                               loader,
                                               data,
                                               &vector,
                                               data->current.value);
    if (ctx->rc == GRN_SUCCESS) {
      grn_obj_set_value(ctx,
                        data->current.column,
                        data->id,
                        &vector,
                        GRN_OBJ_SET);
    }
    GRN_OBJ_FIN(ctx, &vector);
  } else if (grn_obj_is_reference_column(ctx, data->current.column) &&
             !grn_obj_is_vector_column(ctx, data->current.column)) {
    grn_id range_id = DB_OBJ(data->current.column)->range;
    grn_loader_add_record_data sub_data;
    sub_data.table = grn_ctx_at(ctx, range_id);
    sub_data.depth = data->depth + 1;
    sub_data.record_value = data->current.value;
    sub_data.id = GRN_ID_NIL;
    sub_data.key = NULL;
    grn_loader_brace_add_record(ctx, loader, &sub_data);
    if (sub_data.id != GRN_ID_NIL) {
      grn_obj record;
      GRN_RECORD_INIT(&record, 0, range_id);
      GRN_RECORD_SET(ctx, &record, sub_data.id);
      grn_obj_set_value(ctx,
                        data->current.column,
                        data->id,
                        &record,
                        GRN_OBJ_SET);
      GRN_OBJ_FIN(ctx, &record);
    }
    grn_obj_unlink(ctx, sub_data.table);
  } else {
    GRN_DEFINE_NAME(data->table);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][load][%.*s][%.*s] {...} is only for "
        "weight vector column or reference column",
        name_size,
        name,
        (int)(data->current.column_name_size),
        data->current.column_name);
  }
}

static grn_inline int
name_equal(const char *p, size_t size, const char *name)
{
  if (strlen(name) != size) {
    return 0;
  }
  if (*p != GRN_DB_PSEUDO_COLUMN_PREFIX) {
    return 0;
  }
  return !memcmp(p + 1, name + 1, size - 1);
}

static void
grn_loader_brace_set_values(grn_ctx *ctx,
                            grn_loader *loader,
                            grn_loader_add_record_data *data)
{
  grn_obj *brace_value = data->record_value;
  grn_obj *value = brace_value + 1;
  grn_obj *value_end = value + GRN_UINT32_VALUE(brace_value);

  for (; value < value_end; value = values_next(ctx, value)) {
    const char *name = data->current.column_name = GRN_TEXT_VALUE(value);
    size_t name_size = GRN_TEXT_LEN(value);
    data->current.column_name_size = (uint32_t)name_size;
    value++;
    data->current.value = value;
    if (name_equal(name, name_size, GRN_COLUMN_NAME_ID) ||
        name_equal(name, name_size, GRN_COLUMN_NAME_KEY)) {
      /* Skip _id and _key, because it's already used to get id. */
      continue;
    }
    grn_obj *column = data->current.column =
      grn_obj_column(ctx, data->table, name, (uint32_t)name_size);
    if (!column) {
      GRN_DEFINE_NAME(data->table);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s] nonexistent column: <%.*s>",
          (int)name_size,
          name,
          (int)(data->current.column_name_size),
          data->current.column_name);
      grn_loader_on_column_set(ctx, loader, data);
      continue;
      /* Automatic column creation is disabled. */
      /*
      if (value->header.domain == GRN_JSON_LOAD_OPEN_BRACKET) {
        grn_obj *v = value + 1;
        col = grn_column_create(ctx, loader->table, name, name_size,
                                NULL, GRN_OBJ_PERSISTENT|GRN_OBJ_COLUMN_VECTOR,
                                grn_ctx_at(ctx, v->header.domain));
      } else {
        col = grn_column_create(ctx, loader->table, name, name_size,
                                NULL, GRN_OBJ_PERSISTENT,
                                grn_ctx_at(ctx, value->header.domain));
      }
      */
    }

    if (value->header.domain == GRN_JSON_LOAD_OPEN_BRACKET) {
      grn_loader_bracket_set_value(ctx, loader, data);
    } else if (value->header.domain == GRN_JSON_LOAD_OPEN_BRACE) {
      grn_loader_brace_set_value(ctx, loader, data);
    } else {
      grn_obj_set_value(ctx, column, data->id, value, GRN_OBJ_SET);
    }
    grn_loader_on_column_set(ctx, loader, data);
    grn_obj_unlink(ctx, column);
  }
}

static void
grn_loader_parse_id_value(grn_ctx *ctx,
                          grn_loader *loader,
                          grn_loader_add_record_data *data,
                          grn_obj *value)
{
  switch (value->header.type) {
  case GRN_DB_UINT32:
    data->id = GRN_UINT32_VALUE(value);
    break;
  case GRN_DB_INT32:
    data->id = (grn_id)GRN_INT32_VALUE(value);
    break;
  default:
    {
      grn_obj casted_value;
      GRN_UINT32_INIT(&casted_value, 0);
      if (grn_obj_cast(ctx, value, &casted_value, false) != GRN_SUCCESS) {
        GRN_DEFINE_NAME(data->table);
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, value);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][load][%.*s][%s] failed to cast to <UInt32>: <%.*s>",
            name_size,
            name,
            GRN_COLUMN_NAME_ID,
            (int)GRN_TEXT_LEN(&inspected),
            GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
      } else {
        data->id = GRN_UINT32_VALUE(&casted_value);
      }
      GRN_OBJ_FIN(ctx, &casted_value);
      break;
    }
  }
}

static grn_inline void
grn_loader_brace_add_record_internal(grn_ctx *ctx,
                                     grn_loader *loader,
                                     grn_loader_add_record_data *data)
{
  grn_obj *brace_value = data->record_value;
  grn_obj *value = brace_value + 1;
  grn_obj *value_end = value + GRN_UINT32_VALUE(brace_value);
  grn_obj *id_bulk = NULL;
  grn_obj *key = NULL;

  /* Scan values to find _id or _key. */
  for (; value < value_end; value = values_next(ctx, value)) {
    if (!grn_obj_is_text_family_bulk(ctx, value)) {
      GRN_DEFINE_NAME(data->table);
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s] column name must be string: <%.*s>",
          name_size,
          name,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return;
    }

    const char *name = GRN_TEXT_VALUE(value);
    size_t name_size = GRN_TEXT_LEN(value);
    value = values_next(ctx, value);
    if (name_equal(name, name_size, GRN_COLUMN_NAME_ID)) {
      if (id_bulk || key) {
        if (data->table->header.type == GRN_TABLE_NO_KEY) {
          GRN_DEFINE_NAME(data->table);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][load][%.*s] duplicated <%s> columns",
              name_size,
              name,
              GRN_COLUMN_NAME_ID);
          return;
        } else {
          GRN_DEFINE_NAME(data->table);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][load][%.*s] duplicated identifier columns: <%s>:<%s>",
              name_size,
              name,
              id_bulk ? GRN_COLUMN_NAME_ID : GRN_COLUMN_NAME_KEY,
              GRN_COLUMN_NAME_ID);
          return;
        }
      }
      id_bulk = value;
    } else if (name_equal(name, name_size, GRN_COLUMN_NAME_KEY)) {
      if (data->table->header.type == GRN_TABLE_NO_KEY) {
        GRN_DEFINE_NAME(data->table);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][load][%.*s] TABLE_NO_KEY doesn't accept <%s> column",
            name_size,
            name,
            GRN_COLUMN_NAME_KEY);
        return;
      }
      if (id_bulk || key) {
        GRN_DEFINE_NAME(data->table);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][load][%.*s] duplicated identifier columns: <%s>:<%s>",
            name_size,
            name,
            id_bulk ? GRN_COLUMN_NAME_ID : GRN_COLUMN_NAME_KEY,
            GRN_COLUMN_NAME_KEY);
        return;
      }
      key = value;
    }
  }

  switch (data->table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    /* The target table requires _id or _key. */
    if (!id_bulk && !key) {
      grn_loader_on_no_identifier_error(ctx, loader, data->table);
      return;
    }
    break;
  default:
    break;
  }

  if (id_bulk) {
    grn_loader_parse_id_value(ctx, loader, data, id_bulk);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  } else if (key) {
    data->key = key;
  }
  grn_loader_add_record(ctx, loader, data);
  if (data->id == GRN_ID_NIL) {
    /* Target record is not available. */
    return;
  }

  /* TODO: support recursive lock */
  if (data->depth == 0 && loader->lock_table) {
    GRN_TABLE_LOCK_BEGIN(ctx, data->table)
    {
      if (grn_table_at(ctx, data->table, data->id) == data->id) {
        grn_loader_brace_set_values(ctx, loader, data);
        if (data->depth == 0) {
          grn_loader_apply_each(ctx, loader, data->id);
        }
      } else {
        data->id = GRN_ID_NIL;
      }
    }
    GRN_TABLE_LOCK_END(ctx);
  } else {
    grn_loader_brace_set_values(ctx, loader, data);
    if (data->depth == 0) {
      grn_loader_apply_each(ctx, loader, data->id);
    }
  }

  return;
}

static void
grn_loader_brace_add_record(grn_ctx *ctx,
                            grn_loader *loader,
                            grn_loader_add_record_data *data)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_loader_brace_add_record_internal(ctx, loader, data);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(loader->table);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[loader][brace][add-record][slow][%f] "
            "<%.*s>(%u)",
            elapsed_time,
            name_size,
            name,
            data->id);
  }
  GRN_SLOW_LOG_POP_END(ctx);
}

void
grn_loader_apply_each(grn_ctx *ctx, grn_loader *loader, grn_id id)
{
  grn_obj *var;

  if (!loader->each) {
    return;
  }

  var = grn_expr_get_var_by_offset(ctx, loader->each, 0);
  GRN_RECORD_SET(ctx, var, id);
  grn_expr_exec(ctx, loader->each, 0);
}

static void
grn_loader_bracket_set_columns(grn_ctx *ctx,
                               grn_loader *loader,
                               grn_obj *bracket_value)
{
  grn_obj *value = bracket_value + 1;
  grn_obj *value_end = value + GRN_UINT32_VALUE(bracket_value);
  uint32_t i = 0;
  for (; value < value_end; value = values_next(ctx, value), i++) {
    if (!grn_obj_is_text_family_bulk(ctx, value)) {
      GRN_DEFINE_NAME(loader->table);
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s] column name must be string: <%.*s>",
          name_size,
          name,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      grn_loader_save_error(ctx, loader);
      GRN_OBJ_FIN(ctx, &inspected);
      loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
      return;
    }

    const char *column_name = GRN_TEXT_VALUE(value);
    size_t column_name_size = GRN_TEXT_LEN(value);
    if (name_equal(column_name, column_name_size, GRN_COLUMN_NAME_ID)) {
      if (loader->id_offset != -1 || loader->key_offset != -1) {
        /* _id and _key must not appear more than once. */
        if (loader->table->header.type == GRN_TABLE_NO_KEY) {
          GRN_DEFINE_NAME(loader->table);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][load][%.*s] duplicated <%s> columns: %d and %d",
              name_size,
              name,
              GRN_COLUMN_NAME_ID,
              loader->id_offset,
              i);
        } else {
          GRN_DEFINE_NAME(loader->table);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][load][%.*s] duplicated identifier columns: <%s>:<%s>",
              name_size,
              name,
              loader->id_offset != -1 ? GRN_COLUMN_NAME_ID
                                      : GRN_COLUMN_NAME_KEY,
              GRN_COLUMN_NAME_ID);
        }
        grn_loader_save_error(ctx, loader);
        loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
        return;
      }
      loader->id_offset = (int32_t)i;
    } else if (name_equal(column_name, column_name_size, GRN_COLUMN_NAME_KEY)) {
      if (loader->table->header.type == GRN_TABLE_NO_KEY) {
        GRN_DEFINE_NAME(loader->table);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][load][%.*s] TABLE_NO_KEY doesn't accept <%s> column",
            name_size,
            name,
            GRN_COLUMN_NAME_KEY);
        grn_loader_save_error(ctx, loader);
        loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
        return;
      }
      if (loader->id_offset != -1 || loader->key_offset != -1) {
        /* _id and _key must not appear more than once. */
        GRN_DEFINE_NAME(loader->table);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][load][%.*s] duplicated identifier columns: <%s>:<%s>",
            name_size,
            name,
            loader->id_offset != -1 ? GRN_COLUMN_NAME_ID : GRN_COLUMN_NAME_KEY,
            GRN_COLUMN_NAME_KEY);
        grn_loader_save_error(ctx, loader);
        loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
        return;
      }
      loader->key_offset = (int32_t)i;
    }
    grn_obj *column =
      grn_loader_get_column(ctx, loader, column_name, column_name_size);
    if (!column) {
      GRN_DEFINE_NAME(loader->table);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][load][%.*s] nonexistent column: <%.*s>",
          name_size,
          name,
          (int)column_name_size,
          column_name);
      grn_loader_save_error(ctx, loader);
      loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
      return;
    }
  }

  switch (loader->table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    if (loader->id_offset == -1 && loader->key_offset == -1) {
      grn_loader_on_no_identifier_error(ctx, loader, loader->table);
      grn_loader_save_error(ctx, loader);
      loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
      return;
    }
    break;
  default:
    break;
  }

  loader->columns_status = GRN_LOADER_COLUMNS_SET;
}

static void
grn_loader_bracket_set_values(grn_ctx *ctx,
                              grn_loader *loader,
                              grn_loader_add_record_data *data)
{
  grn_obj *bracket_value = data->record_value;
  grn_obj *value = bracket_value + 1;
  grn_obj *value_end = value + GRN_UINT32_VALUE(bracket_value);
  int32_t i = 0;
  GRN_HASH_EACH_BEGIN(ctx, loader->columns, cursor, cursor_id)
  {
    if (value >= value_end) {
      break;
    }

    if (i == loader->id_offset || i == loader->key_offset) {
      i++;
      value = values_next(ctx, value);
      continue;
    }

    void *cursor_value;
    grn_hash_cursor_get_value(ctx, cursor, &cursor_value);
    data->current.column = *((grn_obj **)cursor_value);
    GRN_DEFINE_NAME(data->current.column);
    const char *column_only_name = name;
    for (; column_only_name < name + name_size; column_only_name++) {
      if (column_only_name[0] == '.') {
        column_only_name++;
        break;
      }
    }
    data->current.column_name = column_only_name;
    data->current.column_name_size =
      (uint32_t)(name_size - (column_only_name - name));
    data->current.value = value;

    if (value->header.domain == GRN_JSON_LOAD_OPEN_BRACKET) {
      grn_loader_bracket_set_value(ctx, loader, data);
    } else if (value->header.domain == GRN_JSON_LOAD_OPEN_BRACE) {
      grn_loader_brace_set_value(ctx, loader, data);
    } else {
      grn_obj_set_value(ctx,
                        data->current.column,
                        data->id,
                        value,
                        GRN_OBJ_SET);
    }
    grn_loader_on_column_set(ctx, loader, data);

    i++;
    value = values_next(ctx, value);
  }
  GRN_HASH_EACH_END(ctx, cursor);
}

static grn_inline void
grn_loader_bracket_add_record_internal(grn_ctx *ctx,
                                       grn_loader *loader,
                                       grn_loader_add_record_data *data)
{
  grn_obj *bracket_value = data->record_value;
  uint32_t n_values = grn_loader_bracket_count(ctx, bracket_value);
  if (n_values == 0) {
    switch (loader->table->header.type) {
    case GRN_TABLE_NO_KEY:
      /*
       * Accept empty arrays because a dump command may output a load command
       * which contains empty arrays for a table with deleted records.
       */
      data->id = grn_table_add(ctx, loader->table, NULL, 0, NULL);
      return;
    default:
      grn_loader_on_no_identifier_error(ctx, loader, data->table);
      return;
    }
  }

  uint32_t expected_n_values = grn_hash_size(ctx, loader->columns);
  if (n_values != expected_n_values) {
    GRN_DEFINE_NAME(data->table);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][load][%*.s] "
        "unexpected the number of values: expected:%u, actual:%u",
        name_size,
        name,
        expected_n_values,
        n_values);
    return;
  }

  grn_obj *value = bracket_value + 1;
  if (loader->id_offset != -1) {
    grn_obj *id_bulk =
      grn_loader_bracket_get(ctx, bracket_value, (uint32_t)(loader->id_offset));
    grn_loader_parse_id_value(ctx, loader, data, id_bulk);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  } else if (loader->key_offset != -1) {
    data->key = value + loader->key_offset;
  }
  grn_loader_add_record(ctx, loader, data);
  if (data->id == GRN_ID_NIL) {
    /* Target record is not available. */
    return;
  }

  /* TODO: support recursive lock */
  if (data->depth == 0 && loader->lock_table) {
    GRN_TABLE_LOCK_BEGIN(ctx, data->table)
    {
      if (grn_table_at(ctx, data->table, data->id) == data->id) {
        grn_loader_bracket_set_values(ctx, loader, data);
        if (data->depth == 0) {
          grn_loader_apply_each(ctx, loader, data->id);
        }
      }
    }
    GRN_TABLE_LOCK_END(ctx);
  } else {
    grn_loader_bracket_set_values(ctx, loader, data);
    if (data->depth == 0) {
      grn_loader_apply_each(ctx, loader, data->id);
    }
  }
}

static void
grn_loader_bracket_add_record(grn_ctx *ctx,
                              grn_loader *loader,
                              grn_loader_add_record_data *data)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_loader_bracket_add_record_internal(ctx, loader, data);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(loader->table);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[loader][bracket][add-record][slow][%f] "
            "<%.*s>(%u)",
            elapsed_time,
            name_size,
            name,
            data->id);
  }
  GRN_SLOW_LOG_POP_END(ctx);
}

static void
bracket_close(grn_ctx *ctx, grn_loader *loader)
{
  uint32_t begin;
  GRN_UINT32_POP(&loader->level, begin);
  grn_obj *values = (grn_obj *)GRN_TEXT_VALUE(&loader->values);
  grn_obj *bracket_value = values + begin;
  GRN_ASSERT(bracket_value->header.domain == GRN_JSON_LOAD_OPEN_BRACKET);
  GRN_UINT32_SET(ctx, bracket_value, loader->values_size - begin - 1);
  size_t depth = GRN_UINT32_VECTOR_SIZE(&(loader->level));
  if (depth > loader->emit_level) {
    return;
  }

  if (depth == 0 || !loader->table ||
      loader->columns_status == GRN_LOADER_COLUMNS_BROKEN) {
    goto exit;
  }

  if (loader->columns_status == GRN_LOADER_COLUMNS_UNSET) {
    /*
     * Target columns and _id or _key are not specified yet and values are
     * handled as column names and "_id" or "_key".
     */
    grn_loader_bracket_set_columns(ctx, loader, bracket_value);
    goto exit;
  }

  grn_loader_add_record_data data;
  data.table = loader->table;
  data.depth = 0;
  data.record_value = bracket_value;
  data.id = GRN_ID_NIL;
  data.key = NULL;
  grn_loader_bracket_add_record(ctx, loader, &data);
  grn_loader_on_record_added(ctx, loader, data.id);

exit:
  loader->values_size = begin;
  ERRCLR(ctx);
}

static void
brace_close(grn_ctx *ctx, grn_loader *loader)
{
  uint32_t begin;
  GRN_UINT32_POP(&loader->level, begin);
  grn_obj *values = (grn_obj *)GRN_TEXT_VALUE(&loader->values);
  grn_obj *brace_value = values + begin;
  GRN_ASSERT(brace_value->header.domain == GRN_JSON_LOAD_OPEN_BRACE);
  GRN_UINT32_SET(ctx, brace_value, loader->values_size - begin - 1);
  size_t depth = GRN_UINT32_VECTOR_SIZE(&(loader->level));
  if (depth > loader->emit_level) {
    return;
  }

  grn_id id = GRN_ID_NIL;
  if (loader->table) {
    grn_loader_add_record_data data;
    data.table = loader->table;
    data.depth = 0;
    data.record_value = brace_value;
    data.id = GRN_ID_NIL;
    data.key = NULL;
    grn_loader_brace_add_record(ctx, loader, &data);
    id = data.id;
  }
  grn_loader_on_record_added(ctx, loader, id);
  loader->values_size = begin;
  ERRCLR(ctx);
}

#define JSON_READ_OPEN_BRACKET()                                               \
  do {                                                                         \
    GRN_UINT32_PUT(ctx, &loader->level, loader->values_size);                  \
    values_add(ctx, loader);                                                   \
    loader->last->header.domain = GRN_JSON_LOAD_OPEN_BRACKET;                  \
    loader->stat = GRN_LOADER_TOKEN;                                           \
    str++;                                                                     \
  } while (0)

#define JSON_READ_OPEN_BRACE()                                                 \
  do {                                                                         \
    GRN_UINT32_PUT(ctx, &loader->level, loader->values_size);                  \
    values_add(ctx, loader);                                                   \
    loader->last->header.domain = GRN_JSON_LOAD_OPEN_BRACE;                    \
    loader->stat = GRN_LOADER_TOKEN;                                           \
    str++;                                                                     \
  } while (0)

static void
json_read(grn_ctx *ctx, grn_loader *loader, const char *str, size_t str_len)
{
  const char *const beg = str;
  char c;
  int len;
  const char *se = str + str_len;
  while (str < se) {
    c = *str;
    switch (loader->stat) {
    case GRN_LOADER_BEGIN:
      if ((len = grn_isspace(str, ctx->encoding))) {
        str += len;
        continue;
      }
      switch (c) {
      case '[':
        JSON_READ_OPEN_BRACKET();
        break;
      case '{':
        JSON_READ_OPEN_BRACE();
        break;
      default:
        ERR(GRN_INVALID_ARGUMENT,
            "JSON must start with '[' or '{': <%.*s>",
            (int)str_len,
            beg);
        loader->stat = GRN_LOADER_END;
        break;
      }
      break;
    case GRN_LOADER_TOKEN:
      if ((len = grn_isspace(str, ctx->encoding))) {
        str += len;
        continue;
      }
      switch (c) {
      case '"':
        loader->stat = GRN_LOADER_STRING;
        values_add(ctx, loader);
        str++;
        break;
      case '[':
        JSON_READ_OPEN_BRACKET();
        break;
      case '{':
        JSON_READ_OPEN_BRACE();
        break;
      case ':':
        str++;
        break;
      case ',':
        str++;
        break;
      case ']':
        bracket_close(ctx, loader);
        loader->stat =
          GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        if (ctx->rc == GRN_CANCEL) {
          loader->stat = GRN_LOADER_END;
        }
        str++;
        break;
      case '}':
        brace_close(ctx, loader);
        loader->stat =
          GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        if (ctx->rc == GRN_CANCEL) {
          loader->stat = GRN_LOADER_END;
        }
        str++;
        break;
      case '+':
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        loader->stat = GRN_LOADER_NUMBER;
        values_add(ctx, loader);
        break;
      default:
        if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('_' == c)) {
          loader->stat = GRN_LOADER_SYMBOL;
          values_add(ctx, loader);
        } else {
          if ((len = grn_charlen(ctx, str, se))) {
            GRN_LOG(ctx, GRN_LOG_ERROR, "ignored invalid char('%c') at", c);
            GRN_LOG(ctx, GRN_LOG_ERROR, "%.*s", (int)(str - beg) + len, beg);
            GRN_LOG(ctx, GRN_LOG_ERROR, "%*s", (int)(str - beg) + 1, "^");
            str += len;
          } else {
            GRN_LOG(ctx,
                    GRN_LOG_ERROR,
                    "ignored invalid char(\\x%.2x) after",
                    c);
            GRN_LOG(ctx, GRN_LOG_ERROR, "%.*s", (int)(str - beg), beg);
            str = se;
          }
        }
        break;
      }
      break;
    case GRN_LOADER_SYMBOL:
      if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
          ('0' <= c && c <= '9') || ('_' == c)) {
        GRN_TEXT_PUTC(ctx, loader->last, c);
        str++;
      } else {
        char *v = GRN_TEXT_VALUE(loader->last);
        switch (*v) {
        case 'n':
          if (GRN_TEXT_LEN(loader->last) == 4 && !memcmp(v, "null", 4)) {
            loader->last->header.domain = GRN_DB_VOID;
            GRN_BULK_REWIND(loader->last);
          }
          break;
        case 't':
          if (GRN_TEXT_LEN(loader->last) == 4 && !memcmp(v, "true", 4)) {
            loader->last->header.domain = GRN_DB_BOOL;
            GRN_BOOL_SET(ctx, loader->last, GRN_TRUE);
          }
          break;
        case 'f':
          if (GRN_TEXT_LEN(loader->last) == 5 && !memcmp(v, "false", 5)) {
            loader->last->header.domain = GRN_DB_BOOL;
            GRN_BOOL_SET(ctx, loader->last, GRN_FALSE);
          }
          break;
        default:
          break;
        }
        loader->stat =
          GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
      }
      break;
    case GRN_LOADER_NUMBER:
      switch (c) {
      case '+':
      case '-':
      case '.':
      case 'e':
      case 'E':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        GRN_TEXT_PUTC(ctx, loader->last, c);
        str++;
        break;
      default:
        {
          const char *cur, *str = GRN_BULK_HEAD(loader->last);
          const char *str_end = GRN_BULK_CURR(loader->last);
          int64_t i = grn_atoll(str, str_end, &cur);
          if (cur == str_end) {
            loader->last->header.domain = GRN_DB_INT64;
            GRN_INT64_SET(ctx, loader->last, i);
          } else if (cur != str) {
            uint64_t i = grn_atoull(str, str_end, &cur);
            if (cur == str_end) {
              loader->last->header.domain = GRN_DB_UINT64;
              GRN_UINT64_SET(ctx, loader->last, i);
            } else if (cur != str) {
              double d;
              char *end;
              grn_obj buf;
              GRN_TEXT_INIT(&buf, 0);
              GRN_TEXT_PUT(ctx, &buf, str, GRN_BULK_VSIZE(loader->last));
              GRN_TEXT_PUTC(ctx, &buf, '\0');
              errno = 0;
              d = strtod(GRN_TEXT_VALUE(&buf), &end);
              if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
                loader->last->header.domain = GRN_DB_FLOAT;
                GRN_FLOAT_SET(ctx, loader->last, d);
              }
              GRN_OBJ_FIN(ctx, &buf);
            }
          }
        }
        loader->stat =
          GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        break;
      }
      break;
    case GRN_LOADER_STRING:
      switch (c) {
      case '\\':
        loader->stat = GRN_LOADER_STRING_ESC;
        str++;
        break;
      case '"':
        str++;
        loader->stat =
          GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        /*
        *(GRN_BULK_CURR(loader->last)) = '\0';
        GRN_LOG(ctx, GRN_LOG_ALERT, "read str(%s)",
        GRN_TEXT_VALUE(loader->last));
        */
        break;
      default:
        if ((len = grn_charlen(ctx, str, se))) {
          GRN_TEXT_PUT(ctx, loader->last, str, len);
          str += len;
        } else {
          GRN_LOG(ctx, GRN_LOG_ERROR, "ignored invalid char(\\x%.2x) after", c);
          GRN_LOG(ctx, GRN_LOG_ERROR, "%.*s", (int)(str - beg), beg);
          str = se;
        }
        break;
      }
      break;
    case GRN_LOADER_STRING_ESC:
      switch (c) {
      case 'b':
        GRN_TEXT_PUTC(ctx, loader->last, '\b');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'f':
        GRN_TEXT_PUTC(ctx, loader->last, '\f');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'n':
        GRN_TEXT_PUTC(ctx, loader->last, '\n');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'r':
        GRN_TEXT_PUTC(ctx, loader->last, '\r');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 't':
        GRN_TEXT_PUTC(ctx, loader->last, '\t');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'u':
        loader->stat = GRN_LOADER_UNICODE0;
        break;
      default:
        GRN_TEXT_PUTC(ctx, loader->last, c);
        loader->stat = GRN_LOADER_STRING;
        break;
      }
      str++;
      break;
    case GRN_LOADER_UNICODE0:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        loader->unichar = (uint32_t)((c - '0') * 0x1000);
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        loader->unichar = (uint32_t)((c - 'a' + 10) * 0x1000);
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        loader->unichar = (uint32_t)((c - 'A' + 10) * 0x1000);
        break;
      default:; // todo : error
      }
      loader->stat = GRN_LOADER_UNICODE1;
      str++;
      break;
    case GRN_LOADER_UNICODE1:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        loader->unichar += (uint32_t)((c - '0') * 0x100);
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        loader->unichar += (uint32_t)((c - 'a' + 10) * 0x100);
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        loader->unichar += (uint32_t)((c - 'A' + 10) * 0x100);
        break;
      default:; // todo : error
      }
      loader->stat = GRN_LOADER_UNICODE2;
      str++;
      break;
    case GRN_LOADER_UNICODE2:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        loader->unichar += (uint32_t)((c - '0') * 0x10);
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        loader->unichar += (uint32_t)((c - 'a' + 10) * 0x10);
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        loader->unichar += (uint32_t)((c - 'A' + 10) * 0x10);
        break;
      default:; // todo : error
      }
      loader->stat = GRN_LOADER_UNICODE3;
      str++;
      break;
    case GRN_LOADER_UNICODE3:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        loader->unichar += (uint32_t)(c - '0');
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        loader->unichar += (uint32_t)(c - 'a' + 10);
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        loader->unichar += (uint32_t)(c - 'A' + 10);
        break;
      default:; // todo : error
      }
      {
        uint32_t u = loader->unichar;
        if (u >= 0xd800 && u <= 0xdbff) { /* High-surrogate code points */
          loader->unichar_hi = u;
          loader->stat = GRN_LOADER_STRING;
          str++;
          break;
        }
        if (u >= 0xdc00 && u <= 0xdfff) { /* Low-surrogate code points */
          u = 0x10000 + (loader->unichar_hi - 0xd800) * 0x400 + u - 0xdc00;
        }
        if (u < 0x80) {
          GRN_TEXT_PUTC(ctx, loader->last, (char)u);
        } else {
          if (u < 0x800) {
            GRN_TEXT_PUTC(ctx, loader->last, (char)((u >> 6) | 0xc0));
          } else {
            if (u < 0x10000) {
              GRN_TEXT_PUTC(ctx, loader->last, (char)((u >> 12) | 0xe0));
            } else {
              GRN_TEXT_PUTC(ctx, loader->last, (char)((u >> 18) | 0xf0));
              GRN_TEXT_PUTC(ctx,
                            loader->last,
                            (char)(((u >> 12) & 0x3f) | 0x80));
            }
            GRN_TEXT_PUTC(ctx, loader->last, (char)(((u >> 6) & 0x3f) | 0x80));
          }
          GRN_TEXT_PUTC(ctx, loader->last, (char)((u & 0x3f) | 0x80));
        }
      }
      loader->stat = GRN_LOADER_STRING;
      str++;
      break;
    case GRN_LOADER_END:
      str = se;
      break;
    }
  }
}

#undef JSON_READ_OPEN_BRACKET
#undef JSON_READ_OPEN_BRACE

/*
 * grn_loader_parse_columns parses a columns parameter.
 * Columns except _id and _key are appended to loader->columns.
 * If it contains _id or _key, loader->id_offset or loader->key_offset is set.
 */
static grn_rc
grn_loader_parse_columns(grn_ctx *ctx,
                         grn_loader *loader,
                         const char *str,
                         unsigned int str_size)
{
  const char *ptr = str, *ptr_end = ptr + str_size, *rest;
  const char *tokens[256], *token_end;
  while (ptr < ptr_end) {
    int i, n = grn_tokenize(ptr, (size_t)(ptr_end - ptr), tokens, 256, &rest);
    for (i = 0; i < n; i++) {
      grn_obj *column;
      token_end = tokens[i];
      while (ptr < token_end && (' ' == *ptr || ',' == *ptr)) {
        ptr++;
      }
      if (name_equal(ptr, (size_t)(token_end - ptr), GRN_COLUMN_NAME_ID)) {
        if (loader->id_offset != -1 || loader->key_offset != -1) {
          /* _id and _key must not appear more than once. */
          if (loader->id_offset != -1) {
            ERR(GRN_INVALID_ARGUMENT,
                "duplicated id and key columns: <%s> at %d and <%s> at %d",
                GRN_COLUMN_NAME_ID,
                i,
                GRN_COLUMN_NAME_ID,
                loader->id_offset);
          } else {
            ERR(GRN_INVALID_ARGUMENT,
                "duplicated id and key columns: <%s> at %d and <%s> at %d",
                GRN_COLUMN_NAME_ID,
                i,
                GRN_COLUMN_NAME_KEY,
                loader->key_offset);
          }
          return ctx->rc;
        }
        loader->id_offset = i;
      } else if (name_equal(ptr,
                            (size_t)(token_end - ptr),
                            GRN_COLUMN_NAME_KEY)) {
        if (loader->id_offset != -1 || loader->key_offset != -1) {
          /* _id and _key must not appear more than once. */
          if (loader->id_offset != -1) {
            ERR(GRN_INVALID_ARGUMENT,
                "duplicated id and key columns: <%s> at %d and <%s> at %d",
                GRN_COLUMN_NAME_KEY,
                i,
                GRN_COLUMN_NAME_ID,
                loader->id_offset);
          } else {
            ERR(GRN_INVALID_ARGUMENT,
                "duplicated id and key columns: <%s> at %d and <%s> at %d",
                GRN_COLUMN_NAME_KEY,
                i,
                GRN_COLUMN_NAME_KEY,
                loader->key_offset);
          }
          return ctx->rc;
        }
        loader->key_offset = i;
      }
      column =
        grn_loader_get_column(ctx, loader, ptr, (size_t)(token_end - ptr));
      if (!column) {
        ERR(GRN_INVALID_ARGUMENT,
            "nonexistent column: <%.*s>",
            (int)(token_end - ptr),
            ptr);
        return ctx->rc;
      }
      ptr = token_end;
    }
    ptr = rest;
  }
  switch (loader->table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    if (loader->id_offset == -1 && loader->key_offset == -1) {
      ERR(GRN_INVALID_ARGUMENT, "missing id or key column");
      return ctx->rc;
    }
    break;
  }
  return ctx->rc;
}

static grn_com_addr *addr;

void
grn_load_internal(grn_ctx *ctx, grn_load_input *input)
{
  grn_loader *loader = &ctx->impl->loader;

  loader->emit_level = input->emit_level;
  if (ctx->impl->edge) {
    grn_edge *edge = grn_edges_add_communicator(ctx, addr);
    grn_obj *msg = grn_msg_open(ctx, edge->com, &ctx->impl->edge->send_old);
    /* build msg */
    grn_edge_dispatch(ctx, edge, msg);
  }
  if (input->table.length > 0) {
    grn_ctx_loader_clear(ctx);
    loader->input_type = input->type;
    if (grn_db_check_name(ctx,
                          input->table.value,
                          (unsigned int)(input->table.length))) {
      GRN_DB_CHECK_NAME_ERR("[table][load]",
                            input->table.value,
                            (int)(input->table.length));
      loader->stat = GRN_LOADER_END;
      return;
    }
    loader->table =
      grn_ctx_get(ctx, input->table.value, (int)(input->table.length));
    if (!loader->table) {
      ERR(GRN_INVALID_ARGUMENT,
          "nonexistent table: <%.*s>",
          (int)(input->table.length),
          input->table.value);
      loader->stat = GRN_LOADER_END;
      return;
    }
    if (input->columns.length > 0) {
      grn_rc rc =
        grn_loader_parse_columns(ctx,
                                 loader,
                                 input->columns.value,
                                 (unsigned int)(input->columns.length));
      if (rc != GRN_SUCCESS) {
        loader->columns_status = GRN_LOADER_COLUMNS_BROKEN;
        loader->stat = GRN_LOADER_END;
        return;
      }
      loader->columns_status = GRN_LOADER_COLUMNS_SET;
    }
    if (input->if_exists.length > 0) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, loader->table, loader->ifexists, v);
      if (loader->ifexists && v) {
        grn_expr_parse(ctx,
                       loader->ifexists,
                       input->if_exists.value,
                       (unsigned int)(input->if_exists.length),
                       NULL,
                       GRN_OP_EQUAL,
                       GRN_OP_AND,
                       GRN_EXPR_SYNTAX_SCRIPT | GRN_EXPR_ALLOW_UPDATE);
      }
    }
    if (input->each.length > 0) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, loader->table, loader->each, v);
      if (loader->each && v) {
        grn_expr_parse(ctx,
                       loader->each,
                       input->each.value,
                       (unsigned int)(input->each.length),
                       NULL,
                       GRN_OP_EQUAL,
                       GRN_OP_AND,
                       GRN_EXPR_SYNTAX_SCRIPT | GRN_EXPR_ALLOW_UPDATE);
      }
    }
    loader->output_ids = input->output_ids;
    loader->output_errors = input->output_errors;
    loader->lock_table = input->lock_table;
  } else {
    if (!loader->table) {
      ERR(GRN_INVALID_ARGUMENT, "mandatory \"table\" parameter is absent");
      loader->stat = GRN_LOADER_END;
      return;
    }
  }
  switch (loader->input_type) {
  case GRN_CONTENT_JSON:
    json_read(ctx, loader, input->values.value, input->values.length);
    break;
  case GRN_CONTENT_APACHE_ARROW:
#ifdef GRN_WITH_APACHE_ARROW
    if (!ctx->impl->arrow_stream_loader) {
      ctx->impl->arrow_stream_loader =
        grn_arrow_stream_loader_open(ctx, &(ctx->impl->loader));
    }
    grn_arrow_stream_loader_consume(ctx,
                                    ctx->impl->arrow_stream_loader,
                                    input->values.value,
                                    input->values.length);
#else
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[load][arrow] Apache Arrow support isn't enabled");
    loader->stat = GRN_LOADER_END;
#endif
    break;
  case GRN_CONTENT_NONE:
  case GRN_CONTENT_TSV:
  case GRN_CONTENT_XML:
  case GRN_CONTENT_MSGPACK:
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "unsupported input_type");
    loader->stat = GRN_LOADER_END;
    // todo
    break;
  }
}

grn_rc
grn_load(grn_ctx *ctx,
         grn_content_type input_type,
         const char *table,
         unsigned int table_len,
         const char *columns,
         unsigned int columns_len,
         const char *values,
         unsigned int values_len,
         const char *ifexists,
         unsigned int ifexists_len,
         const char *each,
         unsigned int each_len)
{
  if (!ctx || !ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return ctx->rc;
  }
  GRN_API_ENTER;
  {
    grn_load_input input;
    input.type = input_type;
    input.table.value = table;
    input.table.length = table_len;
    input.columns.value = columns;
    input.columns.length = columns_len;
    input.values.value = values;
    input.values.length = values_len;
    input.if_exists.value = ifexists;
    input.if_exists.length = ifexists_len;
    input.each.value = each;
    input.each.length = each_len;
    input.output_ids = GRN_FALSE;
    input.output_errors = GRN_FALSE;
    input.lock_table = GRN_FALSE;
    input.emit_level = 1;
    grn_load_internal(ctx, &input);
  }
  GRN_API_RETURN(ctx->rc);
}

void
grn_p_loader(grn_ctx *ctx, grn_loader *loader)
{
  printf("#<loader %p\n", loader);
  grn_obj buffer;
  GRN_TEXT_INIT(&buffer, 0);
  grn_inspect(ctx, &buffer, &(loader->level));
  printf("  levels:%.*s\n",
         (int)GRN_TEXT_LEN(&buffer),
         GRN_TEXT_VALUE(&buffer));
  printf("  values:[\n");
  size_t n_values = GRN_BULK_VSIZE(&(loader->values)) / sizeof(grn_obj);
  size_t i;
  for (i = 0; i < n_values; i++) {
    grn_obj *value = ((grn_obj *)GRN_BULK_HEAD(&(loader->values))) + i;
    GRN_BULK_REWIND(&buffer);
    grn_inspect(ctx, &buffer, value);
    printf("    %" GRN_FMT_SIZE ": %.*s,\n",
           i,
           (int)GRN_TEXT_LEN(&buffer),
           GRN_TEXT_VALUE(&buffer));
  }
  GRN_OBJ_FIN(ctx, &buffer);
  printf("  ]\n");
  printf(">\n");
}
