/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017-2018 Brazil
  Copyright(C) 2018-2020 Sutou Kouhei <kou@clear-code.com>

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
#include "grn_ctx.h"
#include "grn_expr_executor.h"

grn_rc
grn_table_apply_expr(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *output_column,
                     grn_obj *expr)
{
  grn_expr_executor executor;

  GRN_API_ENTER;

  if (!grn_obj_is_data_column(ctx, output_column)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, output_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply-expr] output column isn't data column: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  if (!grn_obj_is_expr(ctx, expr)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, expr);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply-expr] expr is invalid: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  grn_expr_executor_init(ctx, &executor, expr);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_API_RETURN(ctx->rc);
  }
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, table, cursor, id, GRN_CURSOR_BY_ID) {
    grn_obj *value;
    value = grn_expr_executor_exec(ctx, &executor, id);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    if (value) {
      grn_obj_set_value(ctx, output_column, id, value, GRN_OBJ_SET);
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  grn_expr_executor_fin(ctx, &executor);

  GRN_API_RETURN(ctx->rc);
}

grn_id
grn_table_find_reference_object(grn_ctx *ctx, grn_obj *table)
{
  grn_id table_id;
  grn_id reference_object_id = GRN_ID_NIL;

  GRN_API_ENTER;

  if (!grn_obj_is_table(ctx, table)) {
    GRN_API_RETURN(GRN_ID_NIL);
  }

  table_id = DB_OBJ(table)->id;

  GRN_DB_EACH_SPEC_BEGIN(ctx, cursor, id, spec) {
    if (id == table_id) {
      continue;
    }

    switch (spec->header.type) {
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_PAT_KEY :
    case GRN_TABLE_DAT_KEY :
      if (spec->header.domain == table_id) {
        reference_object_id = id;
      }
      break;
    case GRN_COLUMN_VAR_SIZE :
    case GRN_COLUMN_FIX_SIZE :
      if (spec->header.domain == table_id) {
        break;
      }
      if (spec->range == table_id) {
        reference_object_id = id;
      }
      break;
    default :
      break;
    }

    if (reference_object_id != GRN_ID_NIL) {
      break;
    }
  } GRN_DB_EACH_SPEC_END(ctx, cursor);

  GRN_API_RETURN(reference_object_id);
}

static void
grn_table_copy_same_key_type(grn_ctx *ctx, grn_obj *from, grn_obj *to)
{
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, from, cursor, from_id,
                             GRN_CURSOR_BY_KEY | GRN_CURSOR_ASCENDING) {
    void   *key;
    int     key_size;
    grn_id  to_id;

    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    to_id    = grn_table_add(ctx, to, key, key_size, NULL);
    if (to_id == GRN_ID_NIL) {
      char from_name[GRN_TABLE_MAX_KEY_SIZE];
      int from_name_size;
      char to_name[GRN_TABLE_MAX_KEY_SIZE];
      int to_name_size;
      grn_obj key_buffer;
      grn_obj inspected_key;

      from_name_size = grn_obj_name(ctx, from, from_name, sizeof(from_name));
      to_name_size = grn_obj_name(ctx, to, to_name, sizeof(to_name));
      if (from->header.domain == GRN_DB_SHORT_TEXT) {
        GRN_SHORT_TEXT_INIT(&key_buffer, 0);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&key_buffer, 0, from->header.domain);
      }
      grn_bulk_write(ctx, &key_buffer, key, key_size);
      GRN_TEXT_INIT(&inspected_key, 0);
      grn_inspect(ctx, &inspected_key, &key_buffer);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][copy] failed to copy key: <%.*s>: "
          "<%.*s> -> <%.*s>",
          (int)GRN_TEXT_LEN(&inspected_key),
          GRN_TEXT_VALUE(&inspected_key),
          from_name_size, from_name,
          to_name_size, to_name);
      GRN_OBJ_FIN(ctx, &inspected_key);
      GRN_OBJ_FIN(ctx, &key_buffer);
      break;
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
}

static void
grn_table_copy_different(grn_ctx *ctx, grn_obj *from, grn_obj *to)
{
  grn_obj from_key_buffer;
  grn_obj to_key_buffer;

  if (from->header.domain == GRN_DB_SHORT_TEXT) {
    GRN_SHORT_TEXT_INIT(&from_key_buffer, 0);
  } else {
    GRN_VALUE_FIX_SIZE_INIT(&from_key_buffer, 0, from->header.domain);
  }
  if (to->header.domain == GRN_DB_SHORT_TEXT) {
    GRN_SHORT_TEXT_INIT(&to_key_buffer, 0);
  } else {
    GRN_VALUE_FIX_SIZE_INIT(&to_key_buffer, 0, to->header.domain);
  }

  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, from, cursor, from_id,
                             GRN_CURSOR_BY_KEY | GRN_CURSOR_ASCENDING) {
    void *key;
    int key_size;
    grn_rc cast_rc;
    grn_id to_id;

    GRN_BULK_REWIND(&from_key_buffer);
    GRN_BULK_REWIND(&to_key_buffer);

    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    grn_bulk_write(ctx, &from_key_buffer, key, key_size);
    cast_rc = grn_obj_cast(ctx, &from_key_buffer, &to_key_buffer, GRN_FALSE);
    if (cast_rc != GRN_SUCCESS) {
      char from_name[GRN_TABLE_MAX_KEY_SIZE];
      int from_name_size;
      char to_name[GRN_TABLE_MAX_KEY_SIZE];
      int to_name_size;
      grn_obj *to_key_type;
      grn_obj inspected_key;
      grn_obj inspected_to_key_type;

      from_name_size = grn_obj_name(ctx, from, from_name, sizeof(from_name));
      to_name_size = grn_obj_name(ctx, to, to_name, sizeof(to_name));
      to_key_type = grn_ctx_at(ctx, to->header.domain);
      GRN_TEXT_INIT(&inspected_key, 0);
      GRN_TEXT_INIT(&inspected_to_key_type, 0);
      grn_inspect(ctx, &inspected_key, &from_key_buffer);
      grn_inspect(ctx, &inspected_to_key_type, to_key_type);
      ERR(cast_rc,
          "[table][copy] failed to cast key: <%.*s> -> %.*s: "
          "<%.*s> -> <%.*s>",
          (int)GRN_TEXT_LEN(&inspected_key),
          GRN_TEXT_VALUE(&inspected_key),
          (int)GRN_TEXT_LEN(&inspected_to_key_type),
          GRN_TEXT_VALUE(&inspected_to_key_type),
          from_name_size, from_name,
          to_name_size, to_name);
      GRN_OBJ_FIN(ctx, &inspected_key);
      GRN_OBJ_FIN(ctx, &inspected_to_key_type);
      break;
    }

    to_id = grn_table_add(ctx, to,
                          GRN_BULK_HEAD(&to_key_buffer),
                          GRN_BULK_VSIZE(&to_key_buffer),
                          NULL);
    if (to_id == GRN_ID_NIL) {
      char from_name[GRN_TABLE_MAX_KEY_SIZE];
      int from_name_size;
      char to_name[GRN_TABLE_MAX_KEY_SIZE];
      int to_name_size;
      grn_obj inspected_from_key;
      grn_obj inspected_to_key;

      from_name_size = grn_obj_name(ctx, from, from_name, sizeof(from_name));
      to_name_size = grn_obj_name(ctx, to, to_name, sizeof(to_name));
      GRN_TEXT_INIT(&inspected_from_key, 0);
      GRN_TEXT_INIT(&inspected_to_key, 0);
      grn_inspect(ctx, &inspected_from_key, &from_key_buffer);
      grn_inspect(ctx, &inspected_to_key, &to_key_buffer);
      ERR(GRN_INVALID_ARGUMENT,
          "[table][copy] failed to copy key: <%.*s> -> <%.*s>: "
          "<%.*s> -> <%.*s>",
          (int)GRN_TEXT_LEN(&inspected_from_key),
          GRN_TEXT_VALUE(&inspected_from_key),
          (int)GRN_TEXT_LEN(&inspected_to_key),
          GRN_TEXT_VALUE(&inspected_to_key),
          from_name_size, from_name,
          to_name_size, to_name);
      GRN_OBJ_FIN(ctx, &inspected_from_key);
      GRN_OBJ_FIN(ctx, &inspected_to_key);
      break;
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &from_key_buffer);
  GRN_OBJ_FIN(ctx, &to_key_buffer);
}

grn_rc
grn_table_copy(grn_ctx *ctx, grn_obj *from, grn_obj *to)
{
  GRN_API_ENTER;

  if (from->header.type == GRN_TABLE_NO_KEY ||
      to->header.type == GRN_TABLE_NO_KEY) {
    char from_name[GRN_TABLE_MAX_KEY_SIZE];
    int from_name_size;
    char to_name[GRN_TABLE_MAX_KEY_SIZE];
    int to_name_size;

    from_name_size = grn_obj_name(ctx, from, from_name, sizeof(from_name));
    to_name_size = grn_obj_name(ctx, to, to_name, sizeof(to_name));
    ERR(GRN_OPERATION_NOT_SUPPORTED,
        "[table][copy] copy from/to TABLE_NO_KEY isn't supported: "
        "<%.*s> -> <%.*s>",
        from_name_size, from_name,
        to_name_size, to_name);
    goto exit;
  }

  if (from == to) {
    char from_name[GRN_TABLE_MAX_KEY_SIZE];
    int from_name_size;

    from_name_size = grn_obj_name(ctx, from, from_name, sizeof(from_name));
    ERR(GRN_OPERATION_NOT_SUPPORTED,
        "[table][copy] from table and to table is the same: "
        "<%.*s>",
        from_name_size, from_name);
    goto exit;
  }

  if (from->header.domain == to->header.domain) {
    grn_table_copy_same_key_type(ctx, from, to);
  } else {
    grn_table_copy_different(ctx, from, to);
  }

exit :
  GRN_API_RETURN(ctx->rc);
}

double
grn_table_get_score(grn_ctx *ctx, grn_obj *table, grn_id id)
{
  if (id == GRN_ID_NIL) {
    return 0.0;
  }

  uint32_t value_size;
  grn_rset_recinfo *ri =
    (grn_rset_recinfo *)grn_obj_get_value_(ctx, table, id, &value_size);
  double score = ri->score;
  grn_obj *parent = NULL;
  grn_id parent_record_id;
  if (grn_table_get_key(ctx,
                        table,
                        id,
                        (void *)&parent_record_id,
                        sizeof(grn_id)) == 0) {
    parent_record_id = GRN_ID_NIL;
  }
  if (parent_record_id == GRN_ID_NIL) {
    return score;
  }

  parent = grn_ctx_at(ctx, table->header.domain);
  while (parent && (parent->header.flags & GRN_OBJ_WITH_SUBREC)) {
    uint32_t parent_value_size;
    grn_rset_recinfo *parent_ri =
      (grn_rset_recinfo *)grn_obj_get_value_(ctx,
                                             parent,
                                             parent_record_id,
                                             &parent_value_size);
    if (parent_value_size == 0) {
      break;
    }
    score += parent_ri->score;
    if (grn_table_get_key(ctx,
                          parent,
                          parent_record_id,
                          (void *)&parent_record_id,
                          sizeof(grn_id)) == 0) {
      break;
    }
    if (parent_record_id == GRN_ID_NIL) {
      break;
    }

    grn_id next_parent_domain = parent->header.domain;
    if (grn_enable_reference_count) {
      grn_obj_unlink(ctx, parent);
    }
    parent = grn_ctx_at(ctx, next_parent_domain);
  }
  if (parent && grn_enable_reference_count) {
    grn_obj_unlink(ctx, parent);
    parent = NULL;
  }

  return score;
}

grn_rc
grn_table_get_duplicated_keys(grn_ctx *ctx,
                              grn_obj *table,
                              grn_obj **duplicated_keys)
{
  GRN_API_ENTER;

  *duplicated_keys = NULL;

  const char *tag = "[table][get-duplicated-keys]";

  if (!grn_obj_is_table_with_key(ctx, table)) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be a table that has key: <%.*s>",
        tag,
        name_size,
        name);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj *keys;
  {
    grn_obj *domain = grn_ctx_at(ctx, table->header.domain);
    keys = grn_table_create(ctx,
                            NULL, 0,
                            NULL,
                            GRN_TABLE_HASH_KEY,
                            domain,
                            NULL);
    grn_obj_unref(ctx, domain);
  }
  if (!keys) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "%s failed to create output table: <%.*s>: %s",
        tag,
        name_size,
        name,
        message);
    goto exit;
  }

  const char *records_name = "records";
  grn_obj *records = grn_column_create(ctx,
                                       keys,
                                       records_name, strlen(records_name),
                                       NULL,
                                       GRN_OBJ_COLUMN_VECTOR,
                                       table);
  if (!records) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "%s failed to create output column: <%.*s>: %s",
        tag,
        name_size,
        name,
        message);
    goto exit;
  }

  grn_obj record_buffer;
  GRN_RECORD_INIT(&record_buffer, 0, DB_OBJ(table)->id);
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                             table,
                             cursor,
                             id,
                             GRN_CURSOR_ASCENDING | GRN_CURSOR_BY_ID) {
    void *key;
    int key_size;
    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    grn_id keys_id = grn_table_add(ctx, keys, key, key_size, NULL);
    if (keys_id == GRN_ID_NIL) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      grn_obj key_buffer;
      GRN_OBJ_INIT(&key_buffer,
                   GRN_BULK,
                   GRN_OBJ_DO_SHALLOW_COPY,
                   table->header.domain);
      GRN_TEXT_SET(ctx, &key_buffer, key, key_size);
      grn_obj inspected;
      grn_inspect(ctx, &inspected, &key_buffer);
      grn_rc rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_INVALID_ARGUMENT;
      }
      ERR(rc,
          "%s failed to add key: <%.*s>: %.*s: %s",
          tag,
          name_size,
          name,
          (int)GRN_TEXT_LEN(&key_buffer),
          GRN_TEXT_VALUE(&key_buffer),
          message);
      GRN_OBJ_FIN(ctx, &inspected);
      GRN_OBJ_FIN(ctx, &key_buffer);
      break;
    }

    GRN_RECORD_SET(ctx, &record_buffer, id);
    grn_obj_set_value(ctx, records, keys_id, &record_buffer, GRN_OBJ_APPEND);
    if (ctx->rc != GRN_SUCCESS) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(ctx->rc,
          "%s failed to add a record: <%.*s>: <%u>: <%u>: %s",
          tag,
          name_size,
          name,
          id,
          keys_id,
          message);
      break;
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &record_buffer);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  grn_obj records_buffer;
  GRN_RECORD_INIT(&records_buffer, GRN_OBJ_VECTOR, DB_OBJ(table)->id);
  GRN_TABLE_EACH_BEGIN(ctx, keys, cursor, id) {
    GRN_BULK_REWIND(&records_buffer);
    grn_obj_get_value(ctx, records, id, &records_buffer);
    if (ctx->rc != GRN_SUCCESS) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(ctx->rc,
          "%s failed to get records: <%.*s>: <%u>: %s",
          tag,
          name_size,
          name,
          id,
          message);
      break;
    }
    if (GRN_RECORD_VECTOR_SIZE(&records_buffer) == 1) {
      grn_table_cursor_delete(ctx, cursor);
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &records_buffer);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  *duplicated_keys = keys;
  keys = NULL;

exit :
  if (keys) {
    grn_obj_close(ctx, keys);
  }
  GRN_API_RETURN(ctx->rc);
}

bool
grn_table_have_duplicated_keys(grn_ctx *ctx, grn_obj *table)
{
  GRN_API_ENTER;

  bool have_duplicated_keys = false;

  const char *tag = "[table][have-duplicated-keys]";

  if (!grn_obj_is_table_with_key(ctx, table)) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be a table that has key: <%.*s>",
        tag,
        name_size,
        name);
    GRN_API_RETURN(have_duplicated_keys);
  }

  grn_obj *keys;
  {
    grn_obj *domain = grn_ctx_at(ctx, table->header.domain);
    keys = grn_table_create(ctx,
                            NULL, 0,
                            NULL,
                            GRN_TABLE_HASH_KEY,
                            domain,
                            NULL);
    grn_obj_unref(ctx, domain);
  }
  if (!keys) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "%s failed to create internal table: <%.*s>: %s",
        tag,
        name_size,
        name,
        message);
    goto exit;
  }

  GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                             table,
                             cursor,
                             id,
                             GRN_CURSOR_ASCENDING | GRN_CURSOR_BY_ID) {
    void *key;
    int key_size;
    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    int added = 0;
    grn_id keys_id = grn_table_add(ctx, keys, key, key_size, &added);
    if (keys_id == GRN_ID_NIL) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      grn_obj key_buffer;
      GRN_OBJ_INIT(&key_buffer,
                   GRN_BULK,
                   GRN_OBJ_DO_SHALLOW_COPY,
                   table->header.domain);
      GRN_TEXT_SET(ctx, &key_buffer, key, key_size);
      grn_obj inspected;
      grn_inspect(ctx, &inspected, &key_buffer);
      grn_rc rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_INVALID_ARGUMENT;
      }
      ERR(rc,
          "%s failed to add key: <%.*s>: %.*s: %s",
          tag,
          name_size,
          name,
          (int)GRN_TEXT_LEN(&key_buffer),
          GRN_TEXT_VALUE(&key_buffer),
          message);
      GRN_OBJ_FIN(ctx, &inspected);
      GRN_OBJ_FIN(ctx, &key_buffer);
      break;
    }
    if (!added) {
      have_duplicated_keys = true;
      break;
    }
  } GRN_TABLE_EACH_END(ctx, cursor);

exit :
  if (keys) {
    grn_obj_close(ctx, keys);
  }
  GRN_API_RETURN(have_duplicated_keys);
}
