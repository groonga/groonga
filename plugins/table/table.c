/* -*- c-basic-offset: 2; indent-tabs-mode: nil -*- */
/* Copyright(C) 2012 Brazil

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

#include "ctx.h"
#include "db.h"
#include "output.h"
#include "util.h"
#include <groonga/plugin.h>

#define VAR GRN_PROC_GET_VAR_BY_OFFSET
#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define TEXT_VALUE_LEN(x) GRN_TEXT_VALUE(x), GRN_TEXT_LEN(x)

static grn_obj *
grn_ctx_get_table_by_name_or_id(grn_ctx *ctx,
                                const char *name, unsigned int name_len)
{
  grn_obj *table;
  const char *end = name + name_len;
  const char *rest = NULL;
  grn_id id = grn_atoui(name, end, &rest);
  if (rest == end) {
    table = grn_ctx_at(ctx, id);
  } else {
    table = grn_ctx_get(ctx, name, name_len);
  }
  if (!GRN_OBJ_TABLEP(table)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid table name: <%.*s>", name_len, name);
    if (table) {
      grn_obj_unlink(ctx, table);
      table = NULL;
    }
  }
  return table;
}

static void
grn_output_table_name_or_id(grn_ctx *ctx, grn_obj *table)
{
  if (table) {
    if (((grn_db_obj *)table)->id & GRN_OBJ_TMP_OBJECT) {
      GRN_OUTPUT_INT64(((grn_db_obj *)table)->id);
    } else {
      int name_len;
      char name_buf[GRN_TABLE_MAX_KEY_SIZE];
      name_len = grn_obj_name(ctx, table, name_buf, GRN_TABLE_MAX_KEY_SIZE);
      GRN_OUTPUT_STR(name_buf, name_len);
    }
  } else {
    GRN_OUTPUT_INT64(0);
  }
}

static grn_bool
parse_bool_value(grn_ctx *ctx, grn_obj *text)
{
  grn_bool value = GRN_FALSE;
  if (GRN_TEXT_LEN(text) == 3 &&
      memcmp("yes", GRN_TEXT_VALUE(text), 3) == 0) {
    value = GRN_TRUE;
  }
  return value;
}

static grn_operator
parse_set_operator_value(grn_ctx *ctx, grn_obj *text)
{
  grn_operator value = GRN_OP_OR;
  if (GRN_TEXT_LEN(text) == 3) {
    if (memcmp("and", GRN_TEXT_VALUE(text), 3) == 0) {
      value = GRN_OP_AND;
    } else if (memcmp("but", GRN_TEXT_VALUE(text), 3) == 0) {
      value = GRN_OP_BUT;
    }
  } else if (GRN_TEXT_LEN(text) == 6 &&
             memcmp("adjust", GRN_TEXT_VALUE(text), 6) == 0) {
    value = GRN_OP_ADJUST;
  }
  return value;
}

static grn_obj *
command_match(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *result_set = NULL;
  grn_obj *table = grn_ctx_get_table_by_name_or_id(ctx, TEXT_VALUE_LEN(VAR(0)));
  if (table) {
    grn_expr_flags flags = GRN_EXPR_SYNTAX_QUERY;
    grn_obj *v, *query, *columns = NULL;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, v);
    if (query) {
      if (GRN_TEXT_LEN(VAR(1))) {
        GRN_EXPR_CREATE_FOR_QUERY(ctx, table, columns, v);
        if (columns) {
          grn_expr_parse(ctx, columns, TEXT_VALUE_LEN(VAR(1)),
                         NULL, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT);
        }
      }
      if (parse_bool_value(ctx, VAR(5))) {
        flags |= GRN_EXPR_ALLOW_COLUMN;
      }
      if (parse_bool_value(ctx, VAR(6))) {
        flags |= GRN_EXPR_ALLOW_PRAGMA;
      }
      grn_expr_parse(ctx, query, TEXT_VALUE_LEN(VAR(2)),
                     columns, GRN_OP_MATCH, GRN_OP_AND, flags);
      if (GRN_TEXT_LEN(VAR(3))) {
        result_set = grn_ctx_get_table_by_name_or_id(ctx, TEXT_VALUE_LEN(VAR(3)));
      } else {
        result_set = grn_table_create(ctx, NULL, 0, NULL,
                                      GRN_TABLE_HASH_KEY|
                                      GRN_OBJ_WITH_SUBREC,
                                      table, NULL);
      }
      if (result_set) {
        grn_table_select(ctx, table, query, result_set,
                         parse_set_operator_value(ctx, VAR(4)));
      }
      grn_obj_unlink(ctx, columns);
      grn_obj_unlink(ctx, query);
    }
  }
  grn_output_table_name_or_id(ctx, result_set);
  return NULL;
}

static grn_obj *
command_filter_by_script(grn_ctx *ctx, int nargs,
                         grn_obj **args, grn_user_data *user_data)
{
  grn_obj *result_set = NULL;
  grn_obj *table = grn_ctx_get_table_by_name_or_id(ctx, TEXT_VALUE_LEN(VAR(0)));
  if (table) {
    grn_expr_flags flags = GRN_EXPR_SYNTAX_SCRIPT;
    grn_obj *v, *query;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, v);
    if (query) {
      if (parse_bool_value(ctx, VAR(4))) {
        flags |= GRN_EXPR_ALLOW_UPDATE;
      }
      grn_expr_parse(ctx, query, TEXT_VALUE_LEN(VAR(1)),
                     NULL, GRN_OP_MATCH, GRN_OP_AND, flags);
      if (GRN_TEXT_LEN(VAR(2))) {
        result_set = grn_ctx_get_table_by_name_or_id(ctx, TEXT_VALUE_LEN(VAR(2)));
      } else {
        result_set = grn_table_create(ctx, NULL, 0, NULL,
                                      GRN_TABLE_HASH_KEY|
                                      GRN_OBJ_WITH_SUBREC,
                                      table, NULL);
      }
      if (result_set) {
        grn_table_select(ctx, table, query, result_set,
                         parse_set_operator_value(ctx, VAR(3)));
      }
      grn_obj_unlink(ctx, query);
    }
  }
  grn_output_table_name_or_id(ctx, result_set);
  return NULL;
}

static grn_obj *
command_group(grn_ctx *ctx, int nargs, grn_obj **args,
              grn_user_data *user_data)
{
  const char *table = GRN_TEXT_VALUE(VAR(0));
  unsigned int table_len = GRN_TEXT_LEN(VAR(0));
  const char *key = GRN_TEXT_VALUE(VAR(1));
  unsigned int key_len = GRN_TEXT_LEN(VAR(1));
  const char *set = GRN_TEXT_VALUE(VAR(2));
  unsigned int set_len = GRN_TEXT_LEN(VAR(2));
  grn_obj *table_ = grn_ctx_get_table_by_name_or_id(ctx, table, table_len);
  grn_obj *set_ = NULL;
  if (table_) {
    uint32_t ngkeys;
    grn_table_sort_key *gkeys;
    gkeys = grn_table_sort_key_from_str(ctx, key, key_len, table_, &ngkeys);
    if (gkeys) {
      if (set_len) {
        set_ = grn_ctx_get_table_by_name_or_id(ctx, set, set_len);
      } else {
        set_ = grn_table_create_for_group(ctx, NULL, 0, NULL,
                                          GRN_TABLE_HASH_KEY|
                                          GRN_OBJ_WITH_SUBREC,
                                          gkeys[0].key, NULL);
      }
      if (set_) {
        grn_table_group_result g = {
          set_, 0, 0, 1,
          GRN_TABLE_GROUP_CALC_COUNT, 0
        };
        grn_table_group(ctx, table_, gkeys, 1, &g, 1);
      }
      grn_table_sort_key_close(ctx, gkeys, ngkeys);
    }
  }
  grn_output_table_name_or_id(ctx, set_);
  return NULL;
}

#define DEFAULT_LIMIT           10

static grn_obj *
command_sort(grn_ctx *ctx, int nargs, grn_obj **args,
             grn_user_data *user_data)
{
  const char *table = GRN_TEXT_VALUE(VAR(0));
  unsigned int table_len = GRN_TEXT_LEN(VAR(0));
  const char *keys = GRN_TEXT_VALUE(VAR(1));
  unsigned int keys_len = GRN_TEXT_LEN(VAR(1));
  int offset = GRN_TEXT_LEN(VAR(2))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(2)), GRN_BULK_CURR(VAR(2)), NULL)
    : 0;
  int limit = GRN_TEXT_LEN(VAR(3))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(3)), GRN_BULK_CURR(VAR(3)), NULL)
    : DEFAULT_LIMIT;
  grn_obj *table_ = grn_ctx_get_table_by_name_or_id(ctx, table, table_len);
  grn_obj *sorted = NULL;
  if (table_) {
    uint32_t nkeys;
    grn_table_sort_key *keys_;
    if (keys_len &&
        (keys_ = grn_table_sort_key_from_str(ctx, keys, keys_len,
                                             table_, &nkeys))) {
      if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                     GRN_OBJ_TABLE_NO_KEY, NULL, table_))) {
        int table_size = (int)grn_table_size(ctx, table_);
        grn_normalize_offset_and_limit(ctx, table_size, &offset, &limit);
        grn_table_sort(ctx, table_, offset, limit, sorted, keys_, nkeys);
        grn_table_sort_key_close(ctx, keys_, nkeys);
      }
    }
  }
  grn_output_table_name_or_id(ctx, sorted);
  return NULL;
}

static grn_obj *
command_output(grn_ctx *ctx, int nargs, grn_obj **args,
               grn_user_data *user_data)
{
  const char *table = GRN_TEXT_VALUE(VAR(0));
  unsigned int table_len = GRN_TEXT_LEN(VAR(0));
  const char *columns = GRN_TEXT_VALUE(VAR(1));
  unsigned int columns_len = GRN_TEXT_LEN(VAR(1));
  int offset = GRN_TEXT_LEN(VAR(2))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(2)), GRN_BULK_CURR(VAR(2)), NULL)
    : 0;
  int limit = GRN_TEXT_LEN(VAR(3))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(3)), GRN_BULK_CURR(VAR(3)), NULL)
    : DEFAULT_LIMIT;
  grn_obj *table_ = grn_ctx_get_table_by_name_or_id(ctx, table, table_len);
  if (table_) {
    grn_obj_format format;
    int table_size = (int)grn_table_size(ctx, table_);
    GRN_OBJ_FORMAT_INIT(&format, table_size, 0, limit, offset);
    format.flags =
      GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
      GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
    /* TODO: accept only comma separated expr as columns */
    grn_obj_columns(ctx, table_, columns, columns_len, &format.columns);
    GRN_OUTPUT_OBJ(table_, &format);
    GRN_OBJ_FORMAT_FIN(ctx, &format);
  }
  return NULL;
}

static grn_obj *
command_each(grn_ctx *ctx, int nargs, grn_obj **args,
             grn_user_data *user_data)
{
  const char *table = GRN_TEXT_VALUE(VAR(0));
  unsigned int table_len = GRN_TEXT_LEN(VAR(0));
  const char *expr = GRN_TEXT_VALUE(VAR(1));
  unsigned int expr_len = GRN_TEXT_LEN(VAR(1));
  grn_obj *table_ = grn_ctx_get_table_by_name_or_id(ctx, table, table_len);
  if (table_) {
    grn_obj *v, *expr_;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, expr_, v);
    if (expr_ && v) {
      grn_table_cursor *tc;
      grn_expr_parse(ctx, expr_, expr, expr_len,
                     NULL, GRN_OP_MATCH, GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
      if ((tc = grn_table_cursor_open(ctx, table_, NULL, 0,
                                      NULL, 0, 0, -1, 0))) {
        while (!grn_table_cursor_next_o(ctx, tc, v)) {
          grn_expr_exec(ctx, expr_, 0);
        }
        grn_table_cursor_close(ctx, tc);
      }
      grn_obj_unlink(ctx, expr_);
    }
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
command_unlink(grn_ctx *ctx, int nargs, grn_obj **args,
               grn_user_data *user_data)
{
  const char *table = GRN_TEXT_VALUE(VAR(0));
  unsigned int table_len = GRN_TEXT_LEN(VAR(0));
  grn_obj *table_ = grn_ctx_get_table_by_name_or_id(ctx, table, table_len);
  if (table_) {
    grn_obj_unlink(ctx, table_);
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
command_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_load_(ctx, GRN_CONTENT_JSON,
            GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)),
            NULL, 0,
            GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)),
            NULL, 0, NULL, 0, 0);
  GRN_OUTPUT_BOOL(ctx->impl->loader.nrecords);
  if (ctx->impl->loader.table) {
    grn_db_touch(ctx, DB_OBJ(ctx->impl->loader.table)->db);
  }
  return NULL;
}

static grn_obj *
command_set(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  /* TODO: implement */
  grn_obj *table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (table) {
    grn_id id;
    if (GRN_TEXT_LEN(VAR(1))) {
      if ((id = grn_table_get(ctx, table, GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1))))) {
        grn_obj obj;
        grn_obj_format format;
        GRN_RECORD_INIT(&obj, 0, ((grn_db_obj *)table)->id);
        GRN_OBJ_FORMAT_INIT(&format, 1, 0, 1, 0);
        GRN_RECORD_SET(ctx, &obj, id);
        grn_obj_columns(ctx, table,
                        GRN_TEXT_VALUE(VAR(4)),
                        GRN_TEXT_LEN(VAR(4)), &format.columns);
        format.flags = 0 /* GRN_OBJ_FORMAT_WITH_COLUMN_NAMES */;
        GRN_OUTPUT_OBJ(&obj, &format);
        GRN_OBJ_FORMAT_FIN(ctx, &format);
      } else {
        /* todo : error handling */
      }
    } else {
      /* todo : get_by_id */
    }
  } else {
    /* todo : error handling */
  }
  return NULL;
}

uint32_t
grn_table_queue_size(grn_table_queue *queue)
{
  return (queue->head < queue->tail)
    ? 2 * queue->cap + queue->head - queue->tail
    : queue->head - queue->tail;
}

void
grn_table_queue_head_increment(grn_table_queue *queue)
{
  if (queue->head == 2 * queue->cap) {
    queue->head = 1;
  } else {
    queue->head++;
  }
}

void
grn_table_queue_tail_increment(grn_table_queue *queue)
{
  if (queue->tail == 2 * queue->cap) {
    queue->tail = 1;
  } else {
    queue->tail++;
  }
}

grn_id
grn_table_queue_head(grn_table_queue *queue)
{
  return queue->head > queue->cap
    ? queue->head - queue->cap
    : queue->head;
}

grn_id
grn_table_queue_tail(grn_table_queue *queue)
{
  return queue->tail > queue->cap
    ? queue->tail - queue->cap
    : queue->tail;
}

static grn_obj *
command_push(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_NO_KEY:
      {
        grn_array *array = (grn_array *)table;
        grn_table_queue *queue = grn_array_queue(ctx, array);
        if (queue) {
          MUTEX_LOCK(queue->mutex);
          if (grn_table_queue_head(queue) == queue->cap) {
            grn_array_clear_curr_rec(ctx, array);
          }
          grn_load_(ctx, GRN_CONTENT_JSON,
                    GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)),
                    NULL, 0,
                    GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)),
                    NULL, 0, NULL, 0, 0);
          if (grn_table_queue_size(queue) == queue->cap) {
            grn_table_queue_tail_increment(queue);
          }
          grn_table_queue_head_increment(queue);
          COND_SIGNAL(queue->cond);
          MUTEX_UNLOCK(queue->mutex);
          GRN_OUTPUT_BOOL(ctx->impl->loader.nrecords);
          if (ctx->impl->loader.table) {
            grn_db_touch(ctx, DB_OBJ(ctx->impl->loader.table)->db);
          }
        } else {
          ERR(GRN_OPERATION_NOT_SUPPORTED, "table '%.*s' doesn't support push",
              (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
        }
      }
      break;
    default :
      ERR(GRN_OPERATION_NOT_SUPPORTED, "table '%.*s' doesn't support push",
          (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "table '%.*s' does not exist.",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
  }
  return NULL;
}

static grn_obj *
command_pull(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_NO_KEY:
      {
        grn_array *array = (grn_array *)table;
        grn_table_queue *queue = grn_array_queue(ctx, array);
        if (queue) {
          MUTEX_LOCK(queue->mutex);
          while (grn_table_queue_size(queue) == 0) {
            if (GRN_TEXT_LEN(VAR(2))) {
              MUTEX_UNLOCK(queue->mutex);
              GRN_OUTPUT_BOOL(0);
              return NULL;
            }
            COND_WAIT(queue->cond, queue->mutex);
          }
          grn_table_queue_tail_increment(queue);
          {
            grn_obj obj;
            grn_obj_format format;
            GRN_RECORD_INIT(&obj, 0, ((grn_db_obj *)table)->id);
            GRN_OBJ_FORMAT_INIT(&format, 1, 0, 1, 0);
            GRN_RECORD_SET(ctx, &obj, grn_table_queue_tail(queue));
            grn_obj_columns(ctx, table, GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)),
                            &format.columns);
            format.flags = 0 /* GRN_OBJ_FORMAT_WITH_COLUMN_NAMES */;
            GRN_OUTPUT_OBJ(&obj, &format);
            GRN_OBJ_FORMAT_FIN(ctx, &format);
          }
          MUTEX_UNLOCK(queue->mutex);
        } else {
          ERR(GRN_OPERATION_NOT_SUPPORTED, "table '%.*s' doesn't support pull",
              (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
        }
      }
      break;
    default :
      ERR(GRN_OPERATION_NOT_SUPPORTED, "table '%.*s' doesn't support pull",
          (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "table '%.*s' does not exist.",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
  }
  return NULL;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

#define DEF_VAR(v,x) do {\
  (v).name = (x);\
  (v).name_size = (x) ? sizeof(x) - 1 : 0;\
  GRN_TEXT_INIT(&(v).value, 0);\
} while (0)

#define DEF_COMMAND(name,func,nvars,vars)\
  (grn_proc_create(ctx, CONST_STR_LEN(name),\
                   GRN_PROC_COMMAND, (func), NULL, NULL, (nvars), (vars)))

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_expr_var vars[18];

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "expression");
  DEF_VAR(vars[2], "result_set");
  DEF_VAR(vars[3], "set_operation");
  DEF_VAR(vars[4], "allow_update");
  DEF_COMMAND("filter_by_script", command_filter_by_script, 5, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "result_set");
  DEF_COMMAND("group", command_group, 3, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "keys");
  DEF_VAR(vars[2], "offset");
  DEF_VAR(vars[3], "limit");
  DEF_COMMAND("sort", command_sort, 4, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "columns");
  DEF_VAR(vars[2], "offset");
  DEF_VAR(vars[3], "limit");
  DEF_COMMAND("output", command_output, 4, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "expression");
  DEF_COMMAND("each", command_each, 2, vars);

  DEF_VAR(vars[0], "table");
  DEF_COMMAND("unlink", command_unlink, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "values");
  DEF_VAR(vars[2], "key");
  DEF_VAR(vars[3], "columns");
  DEF_VAR(vars[4], "output_columns");
  DEF_VAR(vars[5], "id");
  DEF_COMMAND("add", command_add, 2, vars);
  DEF_COMMAND("push", command_push, 2, vars);
  DEF_COMMAND("set", command_set, 6, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "output_columns");
  DEF_VAR(vars[2], "non_block");
  DEF_COMMAND("pull", command_pull, 3, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "columns");
  DEF_VAR(vars[2], "query");
  DEF_VAR(vars[3], "result_set");
  DEF_VAR(vars[4], "set_operation");
  DEF_VAR(vars[5], "allow_column_expression");
  DEF_VAR(vars[6], "allow_pragma");
  DEF_COMMAND("match", command_match, 7, vars);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
