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
const char *grn_admin_html_path = NULL;

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
    grn_select(ctx, outbuf, ct,
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

  ct = grn_get_ctype(&vars[4].value);

  if (nvars == 6) {
    grn_load(ctx, ct,
             GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
             GRN_TEXT_VALUE(&vars[2].value), GRN_TEXT_LEN(&vars[2].value),
             GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
             GRN_TEXT_VALUE(&vars[3].value), GRN_TEXT_LEN(&vars[3].value));
    if (ctx->impl->loader.stat == GRN_LOADER_END) {
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
    case GRN_CONTENT_XML:
    case GRN_CONTENT_NONE:
      break;
    }
  }
  return outbuf;
}

static grn_obj_flags
grn_parse_table_create_flags(grn_ctx *ctx, const char *nptr, const char *end)
{
  grn_obj_flags flags = 0;
  while (nptr < end) {
    if (*nptr == '|' || *nptr == ' ') {
      nptr += 1;
      continue;
    }
    if (!memcmp(nptr, "TABLE_HASH_KEY", 14)) {
      flags |= GRN_OBJ_TABLE_HASH_KEY;
      nptr += 14;
    } else if (!memcmp(nptr, "TABLE_PAT_KEY", 13)) {
      flags |= GRN_OBJ_TABLE_PAT_KEY;
      nptr += 13;
    } else if (!memcmp(nptr, "TABLE_NO_KEY", 12)) {
      flags |= GRN_OBJ_TABLE_NO_KEY;
      nptr += 12;
    } else if (!memcmp(nptr, "KEY_NORMALIZE", 13)) {
      flags |= GRN_OBJ_KEY_NORMALIZE;
      nptr += 13;
    } else if (!memcmp(nptr, "KEY_WITH_SIS", 12)) {
      flags |= GRN_OBJ_KEY_WITH_SIS;
      nptr += 12;
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid flags option: %*s", end - nptr, nptr);
      return 0;
    }
  }
  return flags;
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
    const char *rest;
    grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(&vars[1].value),
                                   GRN_BULK_CURR(&vars[1].value), &rest);
    if (GRN_TEXT_VALUE(&vars[1].value) == rest) {
      flags = grn_parse_table_create_flags(ctx, GRN_TEXT_VALUE(&vars[1].value),
                                           GRN_BULK_CURR(&vars[1].value));
      if (ctx->rc) {
        GRN_TEXT_PUTS(ctx, buf, "false");
        return buf;
      }
    }
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
  case GRN_CONTENT_XML:
  case GRN_CONTENT_NONE:
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
  case GRN_CONTENT_XML:
  case GRN_CONTENT_NONE:
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
          case GRN_CONTENT_XML:
          case GRN_CONTENT_NONE:
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
    if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, 0))) {
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
      case GRN_CONTENT_XML:
      case GRN_CONTENT_NONE:
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
  static int grn_admin_html_path_len = -1;
  if (!grn_admin_html_path) { return buf; }
  if (grn_admin_html_path_len < 0) {
    size_t l;
    if ((l = strlen(grn_admin_html_path)) > PATH_MAX) {
      return buf;
    }
    grn_admin_html_path_len = (int)l;
    if (l > 0 && grn_admin_html_path[l - 1] == PATH_SEPARATOR[0]) { grn_admin_html_path_len--; }
  }
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 2 &&
      (plen = GRN_TEXT_LEN(&vars[0].value)) + grn_admin_html_path_len < PATH_MAX) {
    char path[PATH_MAX];
    memcpy(path, grn_admin_html_path, grn_admin_html_path_len);
    path[grn_admin_html_path_len] = PATH_SEPARATOR[0];
    grn_str_url_path_normalize(GRN_TEXT_VALUE(&vars[0].value),
                               GRN_TEXT_LEN(&vars[0].value),
                               path + grn_admin_html_path_len + 1,
                               PATH_MAX - grn_admin_html_path_len - 1);
    grn_bulk_put_from_file(ctx, buf, path);
  }
  return buf;
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

static grn_obj *
proc_clearlock(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *buf = args[0];
  grn_obj *db = ctx->impl->db;
  if (db) {
    grn_id id;
    grn_pat *keys = (grn_pat *)grn_db_keys(db);
    grn_pat_cursor *pc = grn_pat_cursor_open(ctx, keys, NULL, 0, NULL, 0, 0, -1, 0);
    while ((id = grn_pat_cursor_next(ctx, pc))) {
      grn_obj *obj = grn_ctx_at(ctx, id);
      grn_obj_clear_lock(ctx, obj);
    }
    grn_pat_cursor_close(ctx, pc);
    grn_obj_clear_lock(ctx, db);
    GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
  }
  return buf;
}

static char slev[] = " EACewnid-";

static grn_logger_info info;

static grn_obj *
proc_log_level(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 1) {
    char *p;
    if (GRN_TEXT_LEN(&vars[0].value) &&
        (p = strchr(slev, GRN_TEXT_VALUE(&vars[0].value)[0]))) {
      info.max_level = (int)(p - slev);
      info.flags = GRN_LOG_TIME|GRN_LOG_MESSAGE;
      info.func = NULL;
      info.func_arg = NULL;
      grn_logger_info_set(ctx, &info);
      GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "invalid level");
    }
  }
  return buf;
}

static grn_obj *
proc_log_put(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *buf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 2) {
    char *p;
    if (GRN_TEXT_LEN(&vars[0].value) &&
        (p = strchr(slev, GRN_TEXT_VALUE(&vars[0].value)[0]))) {
      GRN_TEXT_PUTC(ctx, &vars[1].value, '\0');
      GRN_LOG(ctx, (int)(p - slev), "%s", GRN_TEXT_VALUE(&vars[1].value));
      GRN_TEXT_PUTS(ctx, buf, ctx->rc ? "false" : "true");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "invalid level");
    }
  }
  return buf;
}

static grn_obj *
proc_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  /* todo */
  return NULL;
}

static grn_obj *
proc_set(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 6) {
    grn_content_type ct = GRN_INT32_VALUE(&vars[5].value);
    grn_obj *table = grn_ctx_get(ctx,
                                 GRN_TEXT_VALUE(&vars[0].value),
                                 GRN_TEXT_LEN(&vars[0].value));
    if (table) {
      grn_id id;
      if (GRN_TEXT_LEN(&vars[1].value)) {
        if ((id = grn_table_get(ctx, table,
                                GRN_TEXT_VALUE(&vars[1].value),
                                GRN_TEXT_LEN(&vars[1].value)))) {
          /* todo */
          {
            grn_obj obj;
            grn_obj_format format;
            GRN_RECORD_INIT(&obj, 0, ((grn_db_obj *)table)->id);
            GRN_OBJ_FORMAT_INIT(&format, 1, 0, 1);
            GRN_RECORD_SET(ctx, &obj, id);
            grn_obj_columns(ctx, table,
                            GRN_TEXT_VALUE(&vars[4].value),
                            GRN_TEXT_LEN(&vars[4].value), &format.columns);
            switch (ct) {
            case GRN_CONTENT_JSON:
              format.flags = 0 /* GRN_OBJ_FORMAT_WITH_COLUMN_NAMES */;
              GRN_TEXT_PUTS(ctx, outbuf, "[[");
              grn_text_itoa(ctx, outbuf, ctx->rc);
              if (ctx->rc) {
                GRN_TEXT_PUTC(ctx, outbuf, ',');
                grn_text_esc(ctx, outbuf, ctx->errbuf, strlen(ctx->errbuf));
              }
              GRN_TEXT_PUTC(ctx, outbuf, ']');
              GRN_TEXT_PUTC(ctx, outbuf, ',');
              grn_text_otoj(ctx, outbuf, &obj, &format);
              GRN_TEXT_PUTC(ctx, outbuf, ']');
              break;
            case GRN_CONTENT_TSV:
              GRN_TEXT_PUTC(ctx, outbuf, '\n');
              /* TODO: implement */
              break;
            case GRN_CONTENT_XML:
              format.flags = GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
              grn_text_otoxml(ctx, outbuf, &obj, &format);
              break;
            case GRN_CONTENT_NONE:
              break;
            }
            GRN_OBJ_FORMAT_FIN(ctx, &format);
          }
        } else {
          /* todo : error handling */
        }
      } else {
        /* todo : get_by_id */
      }
    } else {
      /* todo : error handling */
    }
  }
  return outbuf;
}

static grn_obj *
proc_get(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  if (nvars == 5) {
    grn_content_type ct = GRN_INT32_VALUE(&vars[3].value);
    grn_obj *table = grn_ctx_get(ctx,
                                 GRN_TEXT_VALUE(&vars[0].value),
                                 GRN_TEXT_LEN(&vars[0].value));
    if (table) {
      grn_id id;
      if (GRN_TEXT_LEN(&vars[1].value)) {
        if ((id = grn_table_get(ctx, table,
                                GRN_TEXT_VALUE(&vars[1].value),
                                GRN_TEXT_LEN(&vars[1].value)))) {
          grn_obj obj;
          grn_obj_format format;
          GRN_RECORD_INIT(&obj, 0, ((grn_db_obj *)table)->id);
          GRN_OBJ_FORMAT_INIT(&format, 1, 0, 1);
          GRN_RECORD_SET(ctx, &obj, id);
          grn_obj_columns(ctx, table,
                          GRN_TEXT_VALUE(&vars[2].value),
                          GRN_TEXT_LEN(&vars[2].value), &format.columns);
          switch (ct) {
          case GRN_CONTENT_JSON:
            format.flags = 0 /* GRN_OBJ_FORMAT_WITH_COLUMN_NAMES */;
            GRN_TEXT_PUTS(ctx, outbuf, "[[");
            grn_text_itoa(ctx, outbuf, ctx->rc);
            if (ctx->rc) {
              GRN_TEXT_PUTC(ctx, outbuf, ',');
              grn_text_esc(ctx, outbuf, ctx->errbuf, strlen(ctx->errbuf));
            }
            GRN_TEXT_PUTC(ctx, outbuf, ']');
            GRN_TEXT_PUTC(ctx, outbuf, ',');
            grn_text_otoj(ctx, outbuf, &obj, &format);
            GRN_TEXT_PUTC(ctx, outbuf, ']');
            break;
          case GRN_CONTENT_TSV:
            GRN_TEXT_PUTC(ctx, outbuf, '\n');
            /* TODO: implement */
            break;
          case GRN_CONTENT_XML:
            format.flags = GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
            grn_text_otoxml(ctx, outbuf, &obj, &format);
            break;
          case GRN_CONTENT_NONE:
            break;
          }
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
  }
  return outbuf;
}

static grn_obj *
func_rand(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
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
func_now(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
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

#define GEO_RESOLUTION   3600000
#define GEO_RADIOUS      6357303
#define GEO_BES_C1       6334834
#define GEO_BES_C2       6377397
#define GEO_BES_C3       0.006674
#define GEO_GRS_C1       6335439
#define GEO_GRS_C2       6378137
#define GEO_GRS_C3       0.006694
#define GEO_INT2RAD(x)   ((M_PI * x) / (GEO_RESOLUTION * 180))

static grn_obj *
func_geo_in_circle(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  unsigned char r = GRN_FALSE;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 3) {
    grn_obj *pos = args[0], *pos1 = args[1], *pos2 = args[2], pos1_, pos2_;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      double lng0, lat0, lng1, lat1, lng2, lat2, x, y, d;
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
      lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
      lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
      lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
      x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
      y = (lat1 - lat0);
      d = (x * x) + (y * y);
      switch (pos2->header.domain) {
      case GRN_DB_INT32 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_INT32_VALUE(pos2);
        break;
      case GRN_DB_UINT32 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_UINT32_VALUE(pos2);
        break;
      case GRN_DB_INT64 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_INT64_VALUE(pos2);
        break;
      case GRN_DB_UINT64 :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_UINT64_VALUE(pos2);
        break;
      case GRN_DB_FLOAT :
        r = (sqrt(d) * GEO_RADIOUS) <= GRN_FLOAT_VALUE(pos2);
        break;
      case GRN_DB_SHORT_TEXT :
      case GRN_DB_TEXT :
      case GRN_DB_LONG_TEXT :
        GRN_OBJ_INIT(&pos2_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos2, &pos2_, 0)) { goto exit; }
        pos2 = &pos2_;
        /* fallthru */
      case GRN_DB_TOKYO_GEO_POINT :
      case GRN_DB_WGS84_GEO_POINT :
        if (domain != pos2->header.domain) { /* todo */ goto exit; }
        lng2 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos2))->longitude);
        lat2 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos2))->latitude);
        x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
        y = (lat2 - lat1);
        r = d <= (x * x) + (y * y);
        break;
      default :
        goto exit;
      }
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_BOOL, 0))) {
    GRN_BOOL_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_in_rectangle(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  unsigned char r = GRN_FALSE;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 3) {
    grn_obj *pos = args[0], *pos1 = args[1], *pos2 = args[2], pos1_, pos2_;
    grn_geo_point *p, *p1, *p2;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      if (pos2->header.domain != domain) {
        GRN_OBJ_INIT(&pos2_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos2, &pos2_, 0)) { goto exit; }
        pos2 = &pos2_;
      }
      p = ((grn_geo_point *)GRN_BULK_HEAD(pos));
      p1 = ((grn_geo_point *)GRN_BULK_HEAD(pos1));
      p2 = ((grn_geo_point *)GRN_BULK_HEAD(pos2));
      r = ((p1->longitude <= p->longitude) && (p->longitude <= p2->longitude) &&
           (p1->latitude <= p->latitude) && (p->latitude <= p2->latitude));
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_BOOL, 0))) {
    GRN_BOOL_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_distance(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  double d = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 2) {
    grn_obj *pos = args[0], *pos1 = args[1], pos1_;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      double lng0, lat0, lng1, lat1, x, y;
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
      lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
      lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
      lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
      x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
      y = (lat1 - lat0);
      d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_geo_distance2(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  double d = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);
  if (nargs == 2) {
    grn_obj *pos = args[0], *pos1 = args[1], pos1_;
    grn_id domain = pos->header.domain;
    if (domain == GRN_DB_TOKYO_GEO_POINT || domain == GRN_DB_WGS84_GEO_POINT) {
      double lng0, lat0, lng1, lat1, x, y;
      if (pos1->header.domain != domain) {
        GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
        if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
        pos1 = &pos1_;
      }
      lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
      lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
      lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
      lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
      x = sin(fabs(lng1 - lng0) * 0.5);
      y = sin(fabs(lat1 - lat0) * 0.5);
      d = asin(sqrt((y * y) + cos(lat0) * cos(lat1) * x * x)) * 2 * GEO_RADIOUS;
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_geo_distance3(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj, *caller;
  uint32_t nvars;
  grn_expr_var *vars;
  double d = 0;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, &caller);

  if (nargs == 2) {
    grn_obj *pos = args[0], *pos1 = args[1], pos1_;
    grn_id domain = pos->header.domain;
    switch (domain) {
    case GRN_DB_TOKYO_GEO_POINT :
      {
        double lng0, lat0, lng1, lat1, p, q, m, n, x, y;
        if (pos1->header.domain != domain) {
          GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
          if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
          pos1 = &pos1_;
        }
        lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
        lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
        lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
        lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
        p = (lat0 + lat1) * 0.5;
        q = (1 - GEO_BES_C3 * sin(p) * sin(p));
        m = GEO_BES_C1 / sqrt(q * q * q);
        n = GEO_BES_C2 / sqrt(q);
        x = n * cos(p) * fabs(lng0 - lng1);
        y = m * fabs(lat0 - lat1);
        d = sqrt((x * x) + (y * y));
      }
      break;
    case  GRN_DB_WGS84_GEO_POINT :
      {
        double lng0, lat0, lng1, lat1, p, q, m, n, x, y, d;
        if (pos1->header.domain != domain) {
          GRN_OBJ_INIT(&pos1_, GRN_BULK, 0, domain);
          if (grn_obj_cast(ctx, pos1, &pos1_, 0)) { goto exit; }
          pos1 = &pos1_;
        }
        lng0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->longitude);
        lat0 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos))->latitude);
        lng1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->longitude);
        lat1 = GEO_INT2RAD(((grn_geo_point *)GRN_BULK_HEAD(pos1))->latitude);
        p = (lat0 + lat1) * 0.5;
        q = (1 - GEO_GRS_C3 * sin(p) * sin(p));
        m = GEO_GRS_C1 / sqrt(q * q * q);
        n = GEO_GRS_C2 / sqrt(q);
        x = n * cos(p) * fabs(lng0 - lng1);
        y = m * fabs(lat0 - lat1);
        d = sqrt((x * x) + (y * y));
      }
      break;
    default :
      /* todo */
      break;
    }
  }
exit :
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

#define DEF_VAR(v,name_str) {\
  (v).name = (name_str);\
  (v).name_size = GRN_STRLEN(name_str);\
  GRN_TEXT_INIT(&(v).value, 0);\
}

#define DEF_PROC(name, func, nvars, vars)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1), NULL,\
                   GRN_PROC_PROCEDURE, (func), NULL, NULL, (nvars), (vars)))

void
grn_db_init_builtin_query(grn_ctx *ctx)
{
  grn_expr_var vars[16];
  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "match_column");
  DEF_VAR(vars[3], "query");
  DEF_VAR(vars[4], "filter");
  DEF_VAR(vars[5], "scorer");
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
  DEF_PROC("define_selector", proc_define_selector, 16, vars);
  DEF_PROC("select", proc_select, 15, vars + 1);

  DEF_VAR(vars[0], "values");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "ifexists");
  DEF_VAR(vars[4], "input_type");
  DEF_VAR(vars[5], "output_type");
  DEF_PROC("load", proc_load, 6, vars);

  DEF_VAR(vars[0], "output_type");
  DEF_PROC("status", proc_status, 1, vars);

  DEF_VAR(vars[0], "output_type");
  DEF_PROC("table_list", proc_table_list, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "output_type");
  DEF_PROC("column_list", proc_column_list, 2, vars);

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "flags");
  DEF_VAR(vars[2], "key_type");
  DEF_VAR(vars[3], "value_type");
  DEF_VAR(vars[4], "default_tokenizer");
  DEF_VAR(vars[5], "output_type");
  DEF_PROC("table_create", proc_table_create, 6, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "flags");
  DEF_VAR(vars[3], "type");
  DEF_VAR(vars[4], "source");
  DEF_VAR(vars[5], "output_type");
  DEF_PROC("column_create", proc_column_create, 6, vars);

  DEF_VAR(vars[0], "path");
  DEF_VAR(vars[1], "output_type");
  DEF_PROC(GRN_EXPR_MISSING_NAME, proc_missing, 2, vars);

  DEF_VAR(vars[0], "view");
  DEF_VAR(vars[1], "table");
  DEF_PROC("view_add", proc_view_add, 2, vars);

  DEF_PROC("quit", proc_quit, 0, vars);
  DEF_PROC("shutdown", proc_shutdown, 0, vars);
  DEF_PROC("clearlock", proc_clearlock, 0, vars);

  DEF_VAR(vars[0], "level");
  DEF_PROC("log_level", proc_log_level, 1, vars);

  DEF_VAR(vars[0], "level");
  DEF_VAR(vars[1], "message");
  DEF_PROC("log_put", proc_log_put, 2, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "values");
  DEF_VAR(vars[4], "output_columns");
  DEF_VAR(vars[5], "output_type");
  DEF_VAR(vars[6], "id");
  DEF_PROC("add", proc_add, 6, vars);
  DEF_PROC("set", proc_set, 7, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "output_columns");
  DEF_VAR(vars[3], "output_type");
  DEF_VAR(vars[4], "id");
  DEF_PROC("get", proc_get, 5, vars);

  DEF_VAR(vars[0], "seed");
  grn_proc_create(ctx, "rand", 4, NULL, GRN_PROC_FUNCTION, func_rand, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "now", 3, NULL, GRN_PROC_FUNCTION, func_now, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_in_circle", 13, NULL, GRN_PROC_FUNCTION,
                  func_geo_in_circle, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_in_rectanble", 16, NULL, GRN_PROC_FUNCTION,
                  func_geo_in_rectangle, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_distance", 12, NULL, GRN_PROC_FUNCTION,
                  func_geo_distance, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_distance2", 13, NULL, GRN_PROC_FUNCTION,
                  func_geo_distance2, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_distance3", 13, NULL, GRN_PROC_FUNCTION,
                  func_geo_distance3, NULL, NULL, 0, vars);
}
