/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>
#include "proc.h"
#include "ql.h"

/**** procs ****/

#define GET_OTYPE(var) \
  ((GRN_TEXT_LEN(var) && *(GRN_TEXT_VALUE(var)) == 't') ? GRN_CONTENT_TSV : GRN_CONTENT_JSON)

#define DEFAULT_LIMIT           10
#define DEFAULT_OUTPUT_COLUMNS  ":id :key :value *"

static grn_rc
proc_select(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 15) {
    int offset = GRN_TEXT_LEN(&vars[7].value)
      ? grn_atoi(GRN_TEXT_VALUE(&vars[7].value), GRN_BULK_CURR(&vars[7].value), NULL)
      : 0;
    int limit = GRN_TEXT_LEN(&vars[8].value)
      ? grn_atoi(GRN_TEXT_VALUE(&vars[8].value), GRN_BULK_CURR(&vars[8].value), NULL)
      : DEFAULT_LIMIT;
    char *output_columns = GRN_TEXT_VALUE(&vars[6].value);
    uint32_t output_columns_len = GRN_TEXT_LEN(&vars[6].value);
    if (!output_columns_len) {
      output_columns = DEFAULT_OUTPUT_COLUMNS;
      output_columns_len = strlen(DEFAULT_OUTPUT_COLUMNS);
    }
    grn_search(ctx, outbuf, GET_OTYPE(&vars[14].value),
               GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
               GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
               GRN_TEXT_VALUE(&vars[2].value), GRN_TEXT_LEN(&vars[2].value),
               GRN_TEXT_VALUE(&vars[3].value), GRN_TEXT_LEN(&vars[3].value),
               GRN_TEXT_VALUE(&vars[4].value), GRN_TEXT_LEN(&vars[4].value),
               GRN_TEXT_VALUE(&vars[5].value), GRN_TEXT_LEN(&vars[5].value),
               output_columns, output_columns_len,
               offset, limit,
               GRN_TEXT_VALUE(&vars[9].value), GRN_TEXT_LEN(&vars[9].value),
               GRN_TEXT_VALUE(&vars[10].value), GRN_TEXT_LEN(&vars[10].value),
               GRN_TEXT_VALUE(&vars[11].value), GRN_TEXT_LEN(&vars[11].value),
               grn_atoi(GRN_TEXT_VALUE(&vars[12].value), GRN_BULK_CURR(&vars[12].value), NULL),
               grn_atoi(GRN_TEXT_VALUE(&vars[13].value), GRN_BULK_CURR(&vars[13].value), NULL));
  }
  grn_ctx_push(ctx, outbuf);
  return ctx->rc;
}

static grn_rc
proc_define_selector(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  grn_obj *outbuf = grn_ctx_pop(ctx);
  if (grn_proc_create(ctx,
                      GRN_TEXT_VALUE(&vars[0].value),
                      GRN_TEXT_LEN(&vars[0].value),
                      NULL, proc_select, NULL, NULL, nvars - 1, vars + 1)) {
    GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value));
  }
  grn_ctx_push(ctx, outbuf);
  return ctx->rc;
}

static grn_rc
proc_load(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 4) {
    grn_load(ctx, GET_OTYPE(&vars[3].value),
             GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
             GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
             GRN_TEXT_VALUE(&vars[2].value), GRN_TEXT_LEN(&vars[2].value));
    if (ctx->impl->loader.stat == GRN_LOADER_BEGIN) {
      grn_text_itoa(ctx, outbuf, ctx->impl->loader.nrecords);
    } else {
      grn_ctx_set_next_expr(ctx, obj);
    }
  }
  grn_ctx_push(ctx, outbuf);
  return ctx->rc;
}

static grn_rc
proc_status(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 1) {
    grn_timeval now;
    grn_content_type otype = GET_OTYPE(&vars[0].value);
    grn_timeval_now(ctx, &now);
    switch (otype) {
    case GRN_CONTENT_TSV:
      /* TODO: implement */
      break;
    case GRN_CONTENT_JSON:
      GRN_TEXT_PUTS(ctx, buf, "{\"starttime\":");
      grn_text_itoa(ctx, buf, grn_starttime.tv_sec);
      GRN_TEXT_PUTS(ctx, buf, ",\"uptime\":");
      grn_text_itoa(ctx, buf, now.tv_sec - grn_starttime.tv_sec);
      GRN_TEXT_PUTC(ctx, buf, '}');
      break;
    }
  }
  grn_ctx_push(ctx, buf);
  return ctx->rc;
}

static grn_rc
proc_table_create(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 6) {
    grn_obj *table = grn_table_create(ctx,
                                      GRN_TEXT_VALUE(&vars[0].value),
                                      GRN_TEXT_LEN(&vars[0].value),
                                      NULL,
                                      grn_atoi(GRN_TEXT_VALUE(&vars[1].value),
                                               GRN_BULK_CURR(&vars[1].value), NULL),
                                      grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[2].value),
                                                  GRN_TEXT_LEN(&vars[2].value)),
                                      grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[3].value),
                                                  GRN_TEXT_LEN(&vars[3].value)));
    if (table) {
      grn_obj_set_info(ctx, table,
                       GRN_INFO_DEFAULT_TOKENIZER,
                       grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[4].value),
                                   GRN_TEXT_LEN(&vars[4].value)));
      grn_obj_unlink(ctx, table);
    }
    GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
  }
  grn_ctx_push(ctx, buf);
  return ctx->rc;
}

static grn_rc
proc_column_create(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 5) {
    grn_obj *column = grn_column_create(ctx,
                                        grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                                                    GRN_TEXT_LEN(&vars[0].value)),
                                        GRN_TEXT_VALUE(&vars[1].value),
                                        GRN_TEXT_LEN(&vars[1].value),
                                        NULL,
                                        grn_atoi(GRN_TEXT_VALUE(&vars[2].value),
                                                 GRN_BULK_CURR(&vars[2].value), NULL),
                                        grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[3].value),
                                                    GRN_TEXT_LEN(&vars[3].value)));
    if (column) { grn_obj_unlink(ctx, column); }
    GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
  }
  grn_ctx_push(ctx, buf);
  return ctx->rc;
}

#define GRN_STRLEN(s) ((s) ? strlen(s) : 0)

static int
print_columninfo(grn_ctx *ctx, grn_obj *column, grn_obj *buf, grn_content_type otype)
{
  grn_id id;
  char *type, name[GRN_TABLE_MAX_KEY_SIZE];
  const char *path;
  int name_len;

  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE:
    type = "\"fix\"";
    break;
  case GRN_COLUMN_VAR_SIZE:
    type = "\"var\"";
    break;
  case GRN_COLUMN_INDEX:
    type = "\"index\"";
    break;
  default:
    GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid header type %d\n", column->header.type);
    return 0;
  }

  id = grn_obj_id(ctx, column);
  name_len = grn_obj_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
  path = grn_obj_path(ctx, column);

  switch (otype) {
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_esc(ctx, buf, name, name_len);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    GRN_TEXT_PUTS(ctx, buf, type);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_itoa(ctx, buf, column->header.flags);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_itoa(ctx, buf, column->header.domain);
    /* TODO: flags to str, domain to str */
    break;
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, buf, '[');
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_esc(ctx, buf, name, name_len);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, ',');
    GRN_TEXT_PUTS(ctx, buf, type);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_itoa(ctx, buf, column->header.flags);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_itoa(ctx, buf, column->header.domain);
    /* TODO: flags to str, domain to str */
    GRN_TEXT_PUTC(ctx, buf, ']');
    break;
  }
  return 1;
}

static int
print_tableinfo(grn_ctx *ctx, grn_obj *table, grn_obj *buf, grn_content_type otype)
{
  grn_id id;
  char name[GRN_TABLE_MAX_KEY_SIZE];
  const char *path;
  int name_len;

  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_NO_KEY:
    break;
  default:
    return 0;
  }

  id = grn_obj_id(ctx, table);
  name_len = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
  path = grn_obj_path(ctx, table);

  switch (otype) {
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_esc(ctx, buf, name, name_len);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_itoa(ctx, buf, table->header.flags);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_itoa(ctx, buf, table->header.domain);
    /* TODO: domain to str */
    break;
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, buf, '[');
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_esc(ctx, buf, name, name_len);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_itoa(ctx, buf, table->header.flags);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_itoa(ctx, buf, table->header.domain);
    /* TODO: domain to str */
    GRN_TEXT_PUTC(ctx, buf, ']');
    break;
  }
  return 1;
}

static grn_rc
proc_column_list(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 2) {
    grn_obj *table;
    grn_content_type otype = GET_OTYPE(&vars[1].value);

    if ((table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                             GRN_TEXT_LEN(&vars[0].value)))) {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)cols) >= 0) {
          grn_id *key;
          char line_delimiter, column_delimiter;

          switch (otype) {
          case GRN_CONTENT_TSV:
            line_delimiter = '\n';
            column_delimiter = '\t';
            GRN_TEXT_PUTS(ctx, buf, "id\tname\tpath\ttype\tflags\tdomain");
            break;
          case GRN_CONTENT_JSON:
            line_delimiter = ',';
            column_delimiter = ',';
            GRN_TEXT_PUTS(ctx, buf, "[[\"id\",\"name\",\"path\",\"type\",\"flags\",\"domain\"]");
            break;
          }

          GRN_HASH_EACH(cols, id, &key, NULL, NULL, {
            grn_obj *col;
            if ((col = grn_ctx_at(ctx, *key))) {
              GRN_TEXT_PUTC(ctx, buf, line_delimiter);
              if (!print_columninfo(ctx, col, buf, otype)) {
                grn_bulk_truncate(ctx, buf, GRN_BULK_VSIZE(buf) - 1);
              }
              grn_obj_unlink(ctx, col);
            }
          });
          if (otype == GRN_CONTENT_JSON) {
            GRN_TEXT_PUTC(ctx, buf, ']');
          }
        }
        grn_hash_close(ctx, cols);
      }
      grn_obj_unlink(ctx, table);
    }
  }

  grn_ctx_push(ctx, buf);
  return ctx->rc;
}

static grn_rc
proc_table_list(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = grn_ctx_pop(ctx);
  grn_obj *db = ctx->impl->db;
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 1) {
    grn_table_cursor *cur;
    if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0))) {
      grn_id id;
      char line_delimiter, column_delimiter;
      grn_content_type otype = GET_OTYPE(&vars[0].value);

      switch (otype) {
      case GRN_CONTENT_TSV:
        line_delimiter = '\n';
        column_delimiter = '\t';
        GRN_TEXT_PUTS(ctx, buf, "id\tname\tpath\tflags\tdomain");
        break;
      case GRN_CONTENT_JSON:
        line_delimiter = ',';
        column_delimiter = ',';
        GRN_TEXT_PUTS(ctx, buf, "[[\"id\",\"name\",\"path\",\"flags\",\"domain\"]");
        break;
      }
      while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
        grn_obj *o;

        if ((o = grn_ctx_at(ctx, id))) {
          GRN_TEXT_PUTC(ctx, buf, line_delimiter);
          if (!print_tableinfo(ctx, o, buf, otype)) {
            grn_bulk_truncate(ctx, buf, GRN_BULK_VSIZE(buf) - 1);
          }
          grn_obj_unlink(ctx, o);
        }
      }
      if (otype == GRN_CONTENT_JSON) {
        GRN_TEXT_PUTC(ctx, buf, ']');
      }
      grn_table_cursor_close(ctx, cur);
    }
  }

  grn_ctx_push(ctx, buf);
  return ctx->rc;
}

#define DEF_VAR(v,name_str) {\
  (v).name = (name_str);\
  (v).name_size = GRN_STRLEN(name_str);\
  GRN_TEXT_INIT(&(v).value, 0);\
}

void
grn_db_init_builtin_query(grn_ctx *ctx)
{
  grn_expr_var vars[16];
  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "match_column");
  DEF_VAR(vars[3], "query");
  DEF_VAR(vars[4], "filter");
  DEF_VAR(vars[5], "foreach");
  DEF_VAR(vars[6], "sortby");
  DEF_VAR(vars[7], "output_columns");
  DEF_VAR(vars[8], "offset");
  DEF_VAR(vars[9], "limit");
  DEF_VAR(vars[10], "drilldown");
  DEF_VAR(vars[11], "drilldown_sortby");
  DEF_VAR(vars[12], "drilldown_output_columns");
  DEF_VAR(vars[13], "drilldown_offset");
  DEF_VAR(vars[14], "drilldown_limit");
  DEF_VAR(vars[15], "output_type");
  grn_proc_create(ctx, "define_selector", 15, NULL, proc_define_selector, NULL, NULL, 16, vars);

  grn_proc_create(ctx, "select", 6, NULL, proc_select, NULL, NULL, 15, vars + 1);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "columns");
  DEF_VAR(vars[2], "values");
  DEF_VAR(vars[3], "input_type");
  grn_proc_create(ctx, "load", 4, NULL, proc_load, NULL, NULL, 4, vars);

  DEF_VAR(vars[0], "output_type");
  grn_proc_create(ctx, "status", 6, NULL, proc_status, NULL, NULL, 1, vars);

  DEF_VAR(vars[0], "output_type");
  grn_proc_create(ctx, "table_list", 10, NULL, proc_table_list, NULL, NULL, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "output_type");
  grn_proc_create(ctx, "column_list", 11, NULL, proc_column_list, NULL, NULL, 2, vars);

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "flags");
  DEF_VAR(vars[2], "key_type");
  DEF_VAR(vars[3], "value_type");
  DEF_VAR(vars[4], "default_tokenizer");
  DEF_VAR(vars[5], "output_type");
  grn_proc_create(ctx, "table_create", 12, NULL, proc_table_create, NULL, NULL, 6, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "flags");
  DEF_VAR(vars[3], "type");
  DEF_VAR(vars[4], "output_type");
  grn_proc_create(ctx, "column_create", 13, NULL, proc_column_create, NULL, NULL, 5, vars);
}
