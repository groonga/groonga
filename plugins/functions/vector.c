/*
  Copyright (C) 2015-2018  Brazil
  Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_vector
#endif

#include <groonga/plugin.h>

static grn_obj *
func_vector_size(grn_ctx *ctx, int n_args, grn_obj **args,
                 grn_user_data *user_data)
{
  grn_obj *target;
  unsigned int size;
  grn_obj *grn_size;

  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "vector_size(): wrong number of arguments (%d for 1)",
                     n_args);
    return NULL;
  }

  target = args[0];
  switch (target->header.type) {
  case GRN_VECTOR :
  case GRN_PVECTOR :
  case GRN_UVECTOR :
    size = grn_vector_size(ctx, target);
    break;
  default :
    {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, target, &inspected);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "vector_size(): target object must be vector: <%.*s>",
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
    break;
  }

  grn_size = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_UINT32, 0);
  if (!grn_size) {
    return NULL;
  }

  GRN_UINT32_SET(ctx, grn_size, size);

  return grn_size;
}

static grn_obj *
func_vector_slice(grn_ctx *ctx, int n_args, grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *target;
  grn_obj *from_raw = NULL;
  grn_obj *length_raw = NULL;
  int64_t from = 0;
  int64_t length = -1;
  uint32_t to = 0;
  uint32_t size = 0;
  grn_obj *slice;

  if (n_args < 2 || n_args > 3) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "vector_slice(): wrong number of arguments (%d for 2..3)",
                     n_args);
    return NULL;
  }

  target = args[0];
  from_raw = args[1];
  if (n_args == 3) {
    length_raw = args[2];
  }
  switch (target->header.type) {
  case GRN_VECTOR :
  case GRN_PVECTOR :
  case GRN_UVECTOR :
    size = grn_vector_size(ctx, target);
    break;
  default :
    {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, target, &inspected);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "vector_slice(): target object must be vector: <%.*s>",
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
    break;
  }

  if (!grn_type_id_is_number_family(ctx, from_raw->header.domain)) {
    grn_obj inspected;

    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, from_raw);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "vector_slice(): from must be a number: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }
  if (from_raw->header.domain == GRN_DB_INT32) {
    from = GRN_INT32_VALUE(from_raw);
  } else if (from_raw->header.domain == GRN_DB_INT64) {
    from = GRN_INT64_VALUE(from_raw);
  } else {
    grn_obj buffer;
    grn_rc rc;

    GRN_INT64_INIT(&buffer, 0);
    rc = grn_obj_cast(ctx, from_raw, &buffer, GRN_FALSE);
    if (rc == GRN_SUCCESS) {
      from = GRN_INT64_VALUE(&buffer);
    }
    GRN_OBJ_FIN(ctx, &buffer);

    if (rc != GRN_SUCCESS) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, from_raw);
      GRN_PLUGIN_ERROR(ctx, rc,
                       "vector_slice(): "
                       "failed to cast from value to number: <%.*s>",
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
  }

  if (length_raw) {
    if (!grn_type_id_is_number_family(ctx, length_raw->header.domain)) {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, length_raw);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "vector_slice(): length must be a number: <%.*s>",
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
    if (length_raw->header.domain == GRN_DB_INT32) {
      length = GRN_INT32_VALUE(length_raw);
    } else if (length_raw->header.domain == GRN_DB_INT64) {
      length = GRN_INT64_VALUE(length_raw);
    } else {
      grn_obj buffer;
      grn_rc rc;

      GRN_INT64_INIT(&buffer, 0);
      rc = grn_obj_cast(ctx, length_raw, &buffer, GRN_FALSE);
      if (rc == GRN_SUCCESS) {
        length = GRN_INT64_VALUE(&buffer);
      }
      GRN_OBJ_FIN(ctx, &buffer);

      if (rc != GRN_SUCCESS) {
        grn_obj inspected;

        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, length_raw);
        GRN_PLUGIN_ERROR(ctx, rc,
                         "vector_slice(): "
                         "failed to cast length value to number: <%.*s>",
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        return NULL;
      }
    }
  }

  slice = grn_plugin_proc_alloc(ctx, user_data, target->header.domain, GRN_OBJ_VECTOR);
  if (!slice) {
    return NULL;
  }

  if (target->header.flags & GRN_OBJ_WITH_WEIGHT) {
    slice->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }

  if (length < 0) {
    length = size + length + 1;
  }

  if (length > size) {
    length = size;
  }

  if (length <= 0) {
    return slice;
  }

  while (from < 0) {
    from += size;
  }

  to = from + length;
  if (to > size) {
    to = size;
  }

  switch (target->header.type) {
  case GRN_VECTOR :
    {
      unsigned int i;
      for (i = from; i < to; i++) {
        const char *content;
        unsigned int content_length;
        float weight;
        grn_id domain;
        content_length = grn_vector_get_element_float(ctx, target, i,
                                                      &content, &weight, &domain);
        grn_vector_add_element_float(ctx, slice,
                                     content, content_length, weight, domain);
      }
    }
    break;
  case GRN_PVECTOR :
    {
      unsigned int i;
      for (i = from; i < to; i++) {
        grn_obj *element = GRN_PTR_VALUE_AT(target, i);
        GRN_PTR_PUT(ctx, slice, element);
      }
    }
    break;
  case GRN_UVECTOR :
    {
      grn_obj *domain;

      domain = grn_ctx_at(ctx, target->header.domain);
      if (grn_obj_is_table(ctx, domain)) {
        unsigned int i;
        for (i = from; i < to; i++) {
          grn_id id;
          float weight;
          id = grn_uvector_get_element_record(ctx, target, i, &weight);
          grn_uvector_add_element_record(ctx, slice, id, weight);
        }
      } else {
#define PUT_SLICE_VALUES(type) do {                                     \
          unsigned int i;                                               \
          for (i = from; i < to; i++) {                                 \
            GRN_ ## type ## _PUT(ctx,                                   \
                                 slice,                                 \
                                 GRN_ ## type ## _VALUE_AT(target, i)); \
          }                                                             \
        } while (GRN_FALSE)
        switch (target->header.domain) {
        case GRN_DB_BOOL :
          PUT_SLICE_VALUES(BOOL);
          break;
        case GRN_DB_INT8 :
          PUT_SLICE_VALUES(INT8);
          break;
        case GRN_DB_UINT8 :
          PUT_SLICE_VALUES(UINT8);
          break;
        case GRN_DB_INT16 :
          PUT_SLICE_VALUES(INT16);
          break;
        case GRN_DB_UINT16 :
          PUT_SLICE_VALUES(UINT16);
          break;
        case GRN_DB_INT32 :
          PUT_SLICE_VALUES(INT32);
          break;
        case GRN_DB_UINT32 :
          PUT_SLICE_VALUES(UINT32);
          break;
        case GRN_DB_INT64 :
          PUT_SLICE_VALUES(INT64);
          break;
        case GRN_DB_UINT64 :
          PUT_SLICE_VALUES(UINT64);
          break;
        case GRN_DB_FLOAT :
          PUT_SLICE_VALUES(FLOAT);
          break;
        case GRN_DB_FLOAT32 :
          PUT_SLICE_VALUES(FLOAT32);
          break;
        case GRN_DB_TIME :
          PUT_SLICE_VALUES(TIME);
          break;
        }
      }
    }
    break;
#undef PUT_SLICE_VALUES
  }

  return slice;
}

static grn_obj *
func_vector_new(grn_ctx *ctx, int n_args, grn_obj **args,
                grn_user_data *user_data)
{
  grn_obj *vector = NULL;
  int i;

  if (n_args == 0) {
    return grn_plugin_proc_alloc(ctx, user_data, GRN_DB_UINT32, GRN_OBJ_VECTOR);
  }

  vector = grn_plugin_proc_alloc(ctx,
                                 user_data,
                                 args[0]->header.domain,
                                 GRN_OBJ_VECTOR);
  if (!vector) {
    return NULL;
  }

  for (i = 0; i < n_args; i++) {
    grn_obj *element = args[i];
    switch (vector->header.type) {
    case GRN_VECTOR :
      grn_vector_add_element(ctx,
                             vector,
                             GRN_BULK_HEAD(element),
                             GRN_BULK_VSIZE(element),
                             0,
                             element->header.domain);
      break;
    case GRN_UVECTOR :
      grn_bulk_write(ctx,
                     vector,
                     GRN_BULK_HEAD(element),
                     GRN_BULK_VSIZE(element));
      break;
    case GRN_PVECTOR :
      GRN_PTR_PUT(ctx, vector, element);
      break;
    default :
      break;
    }
  }

  return vector;
}

static grn_obj *
func_vector_find_vector(grn_ctx *ctx,
                        grn_obj *target,
                        grn_obj *query,
                        grn_operator mode,
                        grn_user_data *user_data)
{
  grn_obj *found_element = NULL;
  grn_operator_exec_func *exec;
  grn_obj element;
  unsigned int i;
  unsigned int n_elements = 0;

  exec = grn_operator_to_exec_func(mode);
  if (!exec) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "vector_find(): unsupported mode: <%s>",
                     grn_operator_to_string(mode));
    return NULL;
  }
  GRN_VOID_INIT(&element);
  n_elements = grn_vector_size(ctx, target);
  for (i = 0; i < n_elements; i++) {
    const char *content;
    unsigned int content_size;
    grn_id domain;

    content_size = grn_vector_get_element(ctx,
                                          target,
                                          i,
                                          &content,
                                          NULL,
                                          &domain);
    grn_obj_reinit(ctx, &element, domain, 0);
    grn_bulk_write(ctx, &element, content, content_size);
    if (exec(ctx, &element, query)) {
      found_element = grn_plugin_proc_alloc(ctx, user_data, domain, 0);
      grn_bulk_write(ctx, found_element, content, content_size);
      break;
    }
  }
  GRN_OBJ_FIN(ctx, &element);

  return found_element;
}

static grn_obj *
func_vector_find_uvector_number(grn_ctx *ctx,
                                grn_obj *target,
                                grn_obj *query,
                                grn_operator mode,
                                grn_user_data *user_data)
{
  grn_obj *found_element = NULL;
  grn_obj query_number_raw;
  grn_obj *query_number = NULL;
  size_t i, n_elements;
  unsigned int element_size;

  if (query->header.domain == target->header.domain) {
    query_number = query;
  } else {
    GRN_VALUE_FIX_SIZE_INIT(&query_number_raw, 0, target->header.domain);
    if (grn_obj_cast(ctx, query, &query_number_raw, GRN_FALSE) != GRN_SUCCESS) {
      GRN_OBJ_FIN(ctx, &query_number_raw);
      return NULL;
    }
    query_number = &query_number_raw;
  }
  element_size = grn_uvector_element_size(ctx, target);
  n_elements = GRN_BULK_VSIZE(target) / element_size;
  if (mode == GRN_OP_EQUAL) {
    for (i = 0; i < n_elements; i++) {
      if (memcmp(GRN_BULK_HEAD(target) + (i * element_size),
                 GRN_BULK_HEAD(query_number),
                 element_size) == 0) {
        found_element = grn_plugin_proc_alloc(ctx,
                                              user_data,
                                              target->header.domain,
                                              0);
        grn_bulk_write(ctx,
                       found_element,
                       GRN_BULK_HEAD(target) + (i * element_size),
                       element_size);
        break;
      }
    }
  } else {
    grn_operator_exec_func *exec;
    grn_obj element;

    exec = grn_operator_to_exec_func(mode);
    if (!exec) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "vector_find(): unsupported mode: <%s>",
                       grn_operator_to_string(mode));
      return NULL;
    }
    GRN_VALUE_FIX_SIZE_INIT(&element, 0, target->header.domain);
    for (i = 0; i < n_elements; i++) {
      GRN_BULK_REWIND(&element);
      grn_bulk_write(ctx,
                     &element,
                     GRN_BULK_HEAD(target) + (i * element_size),
                     element_size);
      if (exec(ctx, &element, query_number)) {
        found_element = grn_plugin_proc_alloc(ctx,
                                              user_data,
                                              target->header.domain,
                                              0);
        grn_bulk_write(ctx,
                       found_element,
                       GRN_BULK_HEAD(target) + (i * element_size),
                       element_size);
        break;
      }
    }
    GRN_OBJ_FIN(ctx, &element);
  }
  if (query_number == &query_number_raw) {
    GRN_OBJ_FIN(ctx, &query_number_raw);
  }

  return found_element;
}

static grn_obj *
func_vector_find_uvector_record(grn_ctx *ctx,
                                grn_obj *target,
                                grn_obj *query,
                                grn_operator mode,
                                grn_user_data *user_data)
{
  grn_obj *found_element = NULL;
  grn_id query_id = GRN_ID_NIL;
  grn_id *ids;
  size_t i, n_elements;

  if (mode != GRN_OP_EQUAL) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "vector_find(): unsupported mode: <%s>",
                     grn_operator_to_string(mode));
    return NULL;
  }

  if (query->header.domain == target->header.domain) {
    query_id = GRN_RECORD_VALUE(query);
  } else {
    grn_obj query_id_raw;
    GRN_RECORD_INIT(&query_id_raw, 0, target->header.domain);
    if (grn_obj_cast(ctx, query, &query_id_raw, GRN_FALSE) != GRN_SUCCESS) {
      GRN_OBJ_FIN(ctx, &query_id_raw);
      return NULL;
    }
    query_id = GRN_RECORD_VALUE(&query_id_raw);
    GRN_OBJ_FIN(ctx, &query_id_raw);
  }
  ids = (grn_id *)GRN_BULK_HEAD(target);
  n_elements = GRN_BULK_VSIZE(target) / sizeof(grn_id);
  for (i = 0; i < n_elements; i++) {
    if (ids[i] == query_id) {
      found_element = grn_plugin_proc_alloc(ctx,
                                            user_data,
                                            target->header.domain,
                                            0);
      GRN_RECORD_SET(ctx, found_element, ids[i]);
      break;
    }
  }

  return found_element;
}

static grn_obj *
func_vector_find_uvector(grn_ctx *ctx,
                         grn_obj *target,
                         grn_obj *query,
                         grn_operator mode,
                         grn_user_data *user_data)
{
  if (grn_type_id_is_number_family(ctx, target->header.domain)) {
    return func_vector_find_uvector_number(ctx,
                                           target,
                                           query,
                                           mode,
                                           user_data);
  } else if (grn_type_id_is_builtin(ctx, target->header.domain)) {
    return NULL;
  } else {
    return func_vector_find_uvector_record(ctx,
                                           target,
                                           query,
                                           mode,
                                           user_data);
  }
}

static grn_obj *
func_vector_find_pvector(grn_ctx *ctx,
                         grn_obj *target,
                         grn_obj *query,
                         grn_operator mode,
                         grn_user_data *user_data)
{
  grn_obj *found_element = NULL;
  grn_operator_exec_func *exec;
  unsigned int i;
  unsigned int n_elements = 0;

  exec = grn_operator_to_exec_func(mode);
  if (!exec) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "vector_find(): unsupported mode: <%s>",
                     grn_operator_to_string(mode));
    return NULL;
  }
  n_elements = GRN_PTR_VECTOR_SIZE(target);
  for (i = 0; i < n_elements; i++) {
    grn_obj *element = GRN_PTR_VALUE_AT(target, i);
    if (exec(ctx, element, query)) {
      found_element =
        grn_plugin_proc_alloc(ctx, user_data, element->header.domain, 0);
      grn_bulk_write(ctx,
                     found_element,
                     GRN_BULK_HEAD(element),
                     GRN_BULK_VSIZE(element));
      break;
    }
  }

  return found_element;
}

static grn_obj *
func_vector_find(grn_ctx *ctx, int n_args, grn_obj **args,
                 grn_user_data *user_data)
{
  const char *context = "vector_find()";
  grn_obj *target;
  grn_obj *query = NULL;
  grn_operator mode = GRN_OP_EQUAL;
  grn_obj *found_element = NULL;

  if (n_args < 2 || n_args > 3) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s: wrong number of arguments (%d for 2..3)",
                     context,
                     n_args);
    return NULL;
  }

  target = args[0];
  query = args[1];
  if (n_args == 3) {
    mode = grn_plugin_proc_get_value_mode(ctx, args[2], mode, context);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
  }

  switch (target->header.type) {
  case GRN_VECTOR :
  case GRN_PVECTOR :
  case GRN_UVECTOR :
    break;
  default :
    {
      grn_obj inspected;

      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, target, &inspected);
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s: target object must be vector: <%.*s>",
                       context,
                       (int)GRN_TEXT_LEN(&inspected),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return NULL;
    }
    break;
  }

  if (target->header.type == GRN_VECTOR) {
    found_element =
      func_vector_find_vector(ctx, target, query, mode, user_data);
  } else if (target->header.type == GRN_UVECTOR) {
    found_element =
      func_vector_find_uvector(ctx, target, query, mode, user_data);
  } else if (target->header.type == GRN_PVECTOR) {
    found_element =
      func_vector_find_pvector(ctx, target, query, mode, user_data);
  }

  if (!found_element) {
    found_element = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return found_element;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  grn_proc_create(ctx, "vector_size", -1, GRN_PROC_FUNCTION, func_vector_size,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "vector_slice", -1, GRN_PROC_FUNCTION, func_vector_slice,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "vector_new", -1, GRN_PROC_FUNCTION, func_vector_new,
                  NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "vector_find", -1, GRN_PROC_FUNCTION, func_vector_find,
                  NULL, NULL, 0, NULL);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
