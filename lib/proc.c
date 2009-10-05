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
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "proc.h"
#include "ql.h"

/**** globals for procs ****/
const char *admin_html_path = NULL;

/**** procs ****/

#define DEFAULT_LIMIT           10
#define DEFAULT_OUTPUT_COLUMNS  "_id _key _value *"

static grn_obj *
proc_select(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = GRN_INT32_VALUE(&vars[14].value);

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
    grn_search(ctx, outbuf, ct,
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
  return outbuf;
}

static grn_obj *
proc_define_selector(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_obj *outbuf = args[0];
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (grn_proc_create(ctx,
                      GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
                      NULL, GRN_PROC_PROCEDURE, proc_select, NULL, NULL, nvars - 1, vars + 1)) {
    GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value));
  }
  return outbuf;
}

static grn_obj *
proc_load(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];
  grn_expr_var *vars;
  grn_obj *proc = grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);

  /* ct = GRN_INT32_VALUE(&vars[4].value); */
  ct = GRN_CONTENT_JSON;

  if (nvars == 5) {
    grn_load(ctx, ct,
             GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
             GRN_TEXT_VALUE(&vars[2].value), GRN_TEXT_LEN(&vars[2].value),
             GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
             GRN_TEXT_VALUE(&vars[3].value), GRN_TEXT_LEN(&vars[3].value));
    if (ctx->impl->loader.stat == GRN_LOADER_BEGIN) {
      grn_text_itoa(ctx, outbuf, ctx->impl->loader.nrecords);
    } else {
      grn_ctx_set_next_expr(ctx, proc);
    }
  }
  return outbuf;
}

static grn_obj *
proc_status(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = args[0];
  grn_expr_var *vars;

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);

  if (nvars == 1) {
    grn_timeval now;
    grn_content_type ct;
    grn_timeval_now(ctx, &now);

    ct = GRN_INT32_VALUE(&vars[0].value);
    switch (ct) {
    case GRN_CONTENT_TSV:
      /* TODO: implement */
      break;
    case GRN_CONTENT_JSON:
      GRN_TEXT_PUTS(ctx, outbuf, "{\"alloc_count\":");
      grn_text_itoa(ctx, outbuf, grn_alloc_count());
      GRN_TEXT_PUTS(ctx, outbuf, ",\"starttime\":");
      grn_text_itoa(ctx, outbuf, grn_starttime.tv_sec);
      GRN_TEXT_PUTS(ctx, outbuf, ",\"uptime\":");
      grn_text_itoa(ctx, outbuf, now.tv_sec - grn_starttime.tv_sec);
      GRN_TEXT_PUTC(ctx, outbuf, '}');
      break;
    }
  }
  return outbuf;
}

static grn_obj *
proc_table_create(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_expr_var *vars;

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);

  if (nvars == 6) {
    grn_obj *table;
    grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(&vars[1].value),
                                   GRN_BULK_CURR(&vars[1].value), NULL);
    if (GRN_TEXT_LEN(&vars[0].value)) { flags |= GRN_OBJ_PERSISTENT; }
    table = grn_table_create(ctx,
                             GRN_TEXT_VALUE(&vars[0].value),
                             GRN_TEXT_LEN(&vars[0].value),
                             NULL, flags,
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
  return buf;
}

static grn_obj *
proc_column_create(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 6) {
    grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(&vars[2].value),
                                   GRN_BULK_CURR(&vars[2].value), NULL);
    grn_obj *column, *table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                                          GRN_TEXT_LEN(&vars[0].value));
    grn_obj *type = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[3].value),
                                GRN_TEXT_LEN(&vars[3].value));
    if (GRN_TEXT_LEN(&vars[1].value)) { flags |= GRN_OBJ_PERSISTENT; }
    column = grn_column_create(ctx, table,
                               GRN_TEXT_VALUE(&vars[1].value),
                               GRN_TEXT_LEN(&vars[1].value),
                               NULL, flags, type);
    if (column) {
      if (GRN_TEXT_LEN(&vars[4].value)) {
        grn_obj sources, source_ids, **p, **pe;
        GRN_PTR_INIT(&sources, GRN_OBJ_VECTOR, GRN_ID_NIL);
        GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
        grn_obj_columns(ctx, type,
                        GRN_TEXT_VALUE(&vars[4].value),
                        GRN_TEXT_LEN(&vars[4].value),
                        &sources);
        p = (grn_obj **)GRN_BULK_HEAD(&sources);
        pe = (grn_obj **)GRN_BULK_CURR(&sources);
        for (; p < pe; p++) {
          grn_id source_id = grn_obj_id(ctx, *p);
          if ((*p)->header.type == GRN_ACCESSOR) {
            /* todo : if "_key" assigned */
            source_id = grn_obj_id(ctx, type);
          }
          if (source_id) {
            GRN_UINT32_PUT(ctx, &source_ids, source_id);
          }
          grn_obj_unlink(ctx, *p);
        }
        if (GRN_BULK_VSIZE(&source_ids)) {
          grn_obj_set_info(ctx, column, GRN_INFO_SOURCE, &source_ids);
        }
        GRN_OBJ_FIN(ctx, &source_ids);
        GRN_OBJ_FIN(ctx, &sources);
      }
      grn_obj_unlink(ctx, column);
    }
    GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
  }
  return buf;
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
  name_len = grn_column_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
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
  case GRN_TABLE_VIEW:
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

static grn_obj *
proc_column_list(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 2) {
    grn_obj *table;
    grn_content_type ct;

    grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
    ct = GRN_INT32_VALUE(&vars[1].value);

    if ((table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                             GRN_TEXT_LEN(&vars[0].value)))) {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)cols) >= 0) {
          grn_id *key;
          char line_delimiter, column_delimiter;

          switch (ct) {
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

          GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col;
            if ((col = grn_ctx_at(ctx, *key))) {
              GRN_TEXT_PUTC(ctx, buf, line_delimiter);
              if (!print_columninfo(ctx, col, buf, ct)) {
                grn_bulk_truncate(ctx, buf, GRN_BULK_VSIZE(buf) - 1);
              }
              grn_obj_unlink(ctx, col);
            }
          });
          if (ct == GRN_CONTENT_JSON) {
            GRN_TEXT_PUTC(ctx, buf, ']');
          }
        }
        grn_hash_close(ctx, cols);
      }
      grn_obj_unlink(ctx, table);
    }
  }
  return buf;
}

static grn_obj *
proc_table_list(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_obj *db = ctx->impl->db;
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 1) {
    grn_table_cursor *cur;
    if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, 0, 0))) {
      grn_id id;
      grn_content_type ct;
      char line_delimiter, column_delimiter;

      grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
      ct = GRN_INT32_VALUE(&vars[0].value);

      switch (ct) {
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
          if (!print_tableinfo(ctx, o, buf, ct)) {
            grn_bulk_truncate(ctx, buf, GRN_BULK_VSIZE(buf) - 1);
          }
          grn_obj_unlink(ctx, o);
        }
      }
      if (ct == GRN_CONTENT_JSON) {
        GRN_TEXT_PUTC(ctx, buf, ']');
      }
      grn_table_cursor_close(ctx, cur);
    }
  }
  return buf;
}

/* bulk must be initialized grn_bulk or grn_msg */
static int
grn_bulk_put_from_file(grn_ctx *ctx, grn_obj *bulk, const char *path)
{
  /* FIXME: implement more smartly with grn_bulk */
  int fd, ret = 0;
  struct stat stat;
  if ((fd = open(path, O_RDONLY)) == -1) { return ret; }
  if (fstat(fd, &stat) != -1) {
    char *buf, *bp;
    off_t rest = stat.st_size;
    if ((buf = GRN_MALLOC(rest))) {
      ssize_t ss;
      for (bp = buf; rest; rest -= ss, bp += ss) {
        if ((ss = read(fd, bp, rest)) == -1) { goto exit; }
      }
      GRN_TEXT_PUT(ctx, bulk, buf, stat.st_size);
      ret = 1;
    }
    GRN_FREE(buf);
  }
exit :
  close(fd);
  return ret;
}

static grn_obj *
proc_missing(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars, plen;
  grn_obj *buf = args[0];
  grn_expr_var *vars;
  static int admin_html_path_len = -1;
  if (!admin_html_path) { return buf; }
  if (admin_html_path_len < 0) {
    size_t l;
    if ((l = strlen(admin_html_path)) > PATH_MAX) {
      return buf;
    }
    admin_html_path_len = (int)l;
    if (l > 0 && admin_html_path[l - 1] == PATH_SEPARATOR[0]) { admin_html_path_len--; }
  }
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 2 &&
      (plen = GRN_TEXT_LEN(&vars[0].value)) + admin_html_path_len < PATH_MAX) {
    char path[PATH_MAX];
    memcpy(path, admin_html_path, admin_html_path_len);
    path[admin_html_path_len] = PATH_SEPARATOR[0];
    grn_str_url_path_normalize(GRN_TEXT_VALUE(&vars[0].value),
                               GRN_TEXT_LEN(&vars[0].value),
                               path + admin_html_path_len + 1,
                               PATH_MAX - admin_html_path_len - 1);
    grn_bulk_put_from_file(ctx, buf, path);
  }
  return buf;
}

static grn_obj *
proc_rand(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int val;
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs > 0) {
    int max = GRN_INT32_VALUE(args[0]);
    val = (int) (1.0 * max * rand() / (RAND_MAX + 1.0));
  } else {
    val = rand();
  }
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_INT32, 0))) {
    GRN_INT32_SET(ctx, obj, val);
  }
  return obj;
}

static grn_obj *
proc_now(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_obj *obj, *caller;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_TIME, 0))) {
    GRN_TIME_NOW(ctx, obj);
  }
  return obj;
}

static grn_obj *
proc_view_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 2) {
    grn_obj *view = grn_ctx_get(ctx,
                                GRN_TEXT_VALUE(&vars[0].value),
                                GRN_TEXT_LEN(&vars[0].value));
    grn_obj *table = grn_ctx_get(ctx,
                                GRN_TEXT_VALUE(&vars[1].value),
                                GRN_TEXT_LEN(&vars[1].value));
    grn_view_add(ctx, view, table);
    GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
  }
  return buf;
}

static grn_obj *
proc_quit(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *buf = args[0];
  ctx->stat = GRN_CTX_QUITTING;
  return buf;
}

static grn_obj *
proc_shutdown(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *buf = args[0];
  grn_gctx.stat = GRN_CTX_QUIT;
  ctx->stat = GRN_CTX_QUITTING;
  return buf;
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
  grn_proc_create(ctx, "define_selector", 15, NULL, GRN_PROC_PROCEDURE,
                  proc_define_selector, NULL, NULL, 16, vars);

  grn_proc_create(ctx, "select", 6, NULL, GRN_PROC_PROCEDURE,
                  proc_select, NULL, NULL, 15, vars + 1);

  DEF_VAR(vars[0], "values");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "ifexists");
  DEF_VAR(vars[4], "input_type");
  grn_proc_create(ctx, "load", 4, NULL, GRN_PROC_PROCEDURE,
                  proc_load, NULL, NULL, 5, vars);

  DEF_VAR(vars[0], "output_type");
  grn_proc_create(ctx, "status", 6, NULL, GRN_PROC_PROCEDURE,
                  proc_status, NULL, NULL, 1, vars);

  DEF_VAR(vars[0], "output_type");
  grn_proc_create(ctx, "table_list", 10, NULL, GRN_PROC_PROCEDURE,
                  proc_table_list, NULL, NULL, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "output_type");
  grn_proc_create(ctx, "column_list", 11, NULL, GRN_PROC_PROCEDURE,
                  proc_column_list, NULL, NULL, 2, vars);

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "flags");
  DEF_VAR(vars[2], "key_type");
  DEF_VAR(vars[3], "value_type");
  DEF_VAR(vars[4], "default_tokenizer");
  DEF_VAR(vars[5], "output_type");
  grn_proc_create(ctx, "table_create", 12, NULL, GRN_PROC_PROCEDURE,
                  proc_table_create, NULL, NULL, 6, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "flags");
  DEF_VAR(vars[3], "type");
  DEF_VAR(vars[4], "source");
  DEF_VAR(vars[5], "output_type");
  grn_proc_create(ctx, "column_create", 13, NULL, GRN_PROC_PROCEDURE,
                  proc_column_create, NULL, NULL, 6, vars);

  DEF_VAR(vars[0], "path");
  DEF_VAR(vars[1], "output_type");
  grn_proc_create(ctx, GRN_EXPR_MISSING_NAME, strlen(GRN_EXPR_MISSING_NAME),
                  NULL, GRN_PROC_PROCEDURE, proc_missing, NULL, NULL, 2, vars);

  DEF_VAR(vars[0], "view");
  DEF_VAR(vars[1], "table");
  grn_proc_create(ctx, "view_add", 8, NULL, GRN_PROC_PROCEDURE,
                  proc_view_add, NULL, NULL, 2, vars);

  grn_proc_create(ctx, "quit", 4, NULL, GRN_PROC_PROCEDURE,
                  proc_quit, NULL, NULL, 0, vars);
  grn_proc_create(ctx, "shutdown", 8, NULL, GRN_PROC_PROCEDURE,
                  proc_shutdown, NULL, NULL, 0, vars);

  DEF_VAR(vars[0], "seed");
  grn_proc_create(ctx, "rand", 4, NULL, GRN_PROC_FUNCTION, proc_rand, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "now", 3, NULL, GRN_PROC_FUNCTION, proc_now, NULL, NULL, 0, vars);
}
