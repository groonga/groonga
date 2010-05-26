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
#define __USE_GNU
#include <fcntl.h>
#include <sys/stat.h>
#include "proc.h"
#include "ql.h"
#include "db.h"

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

/**** globals for procs ****/
const char *grn_admin_html_path = NULL;

/**** procs ****/

#define DEFAULT_LIMIT           10
#define DEFAULT_OUTPUT_COLUMNS  "_id _key _value *"
#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key _nsubrecs"

static void
print_return_code_with_body(grn_ctx *ctx, grn_obj *buf, grn_content_type ct,
                            grn_obj *body)
{
  switch (ct) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTS(ctx, buf, "[[");
    grn_text_itoa(ctx, buf, ctx->rc);
    {
      double dv;
      grn_timeval tv;
      grn_timeval_now(ctx, &tv);
      dv = ctx->impl->tv.tv_sec;
      dv += ctx->impl->tv.tv_usec / 1000000.0;
      GRN_TEXT_PUTC(ctx, buf, ',');
      grn_text_ftoa(ctx, buf, dv);
      dv = (tv.tv_sec - ctx->impl->tv.tv_sec);
      dv += (tv.tv_usec - ctx->impl->tv.tv_usec) / 1000000.0;
      GRN_TEXT_PUTC(ctx, buf, ',');
      grn_text_ftoa(ctx, buf, dv);
    }
    if (ctx->rc != GRN_SUCCESS) {
      GRN_TEXT_PUTS(ctx, buf, ",");
      grn_text_esc(ctx, buf, ctx->errbuf, strlen(ctx->errbuf));
    }
    if (body && GRN_TEXT_LEN(body)) {
      GRN_TEXT_PUTS(ctx, buf, "],");
      GRN_TEXT_PUT(ctx, buf, GRN_TEXT_VALUE(body), GRN_TEXT_LEN(body));
      GRN_TEXT_PUTS(ctx, buf, "]");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "]]");
    }
    break;
  case GRN_CONTENT_TSV:
  case GRN_CONTENT_XML:
    if (body) {
      GRN_TEXT_PUT(ctx, buf, GRN_TEXT_VALUE(body), GRN_TEXT_LEN(body));
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
}

static void
print_return_code(grn_ctx *ctx, grn_obj *buf, grn_content_type ct)
{
  print_return_code_with_body(ctx, buf, ct, NULL);
}

static grn_obj *
proc_select(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 16) ? grn_get_ctype(&vars[15].value) : GRN_CONTENT_JSON;

  if (nvars == 16) {
    grn_obj body; /* FIXME: double buffering! */
    int offset = GRN_TEXT_LEN(&vars[7].value)
      ? grn_atoi(GRN_TEXT_VALUE(&vars[7].value), GRN_BULK_CURR(&vars[7].value), NULL)
      : 0;
    int limit = GRN_TEXT_LEN(&vars[8].value)
      ? grn_atoi(GRN_TEXT_VALUE(&vars[8].value), GRN_BULK_CURR(&vars[8].value), NULL)
      : DEFAULT_LIMIT;
    char *output_columns = GRN_TEXT_VALUE(&vars[6].value);
    uint32_t output_columns_len = GRN_TEXT_LEN(&vars[6].value);
    char *drilldown_output_columns = GRN_TEXT_VALUE(&vars[11].value);
    uint32_t drilldown_output_columns_len = GRN_TEXT_LEN(&vars[11].value);
    int drilldown_offset = GRN_TEXT_LEN(&vars[12].value)
      ? grn_atoi(GRN_TEXT_VALUE(&vars[12].value), GRN_BULK_CURR(&vars[12].value), NULL)
      : 0;
    int drilldown_limit = GRN_TEXT_LEN(&vars[13].value)
      ? grn_atoi(GRN_TEXT_VALUE(&vars[13].value), GRN_BULK_CURR(&vars[13].value), NULL)
      : DEFAULT_DRILLDOWN_LIMIT;
    if (!output_columns_len) {
      output_columns = DEFAULT_OUTPUT_COLUMNS;
      output_columns_len = strlen(DEFAULT_OUTPUT_COLUMNS);
    }
    if (!drilldown_output_columns_len) {
      drilldown_output_columns = DEFAULT_DRILLDOWN_OUTPUT_COLUMNS;
      drilldown_output_columns_len = strlen(DEFAULT_DRILLDOWN_OUTPUT_COLUMNS);
    }

    GRN_TEXT_INIT(&body, 0);
    if (grn_select(ctx, &body, ct,
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
                   drilldown_output_columns, drilldown_output_columns_len,
                   drilldown_offset, drilldown_limit,
                   GRN_TEXT_VALUE(&vars[14].value), GRN_TEXT_LEN(&vars[14].value))) {
      print_return_code(ctx, outbuf, ct);
    } else {
      print_return_code_with_body(ctx, outbuf, ct, &body);
    }
    grn_obj_unlink(ctx, &body);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 15);
    print_return_code(ctx, outbuf, ct);
  }
  return outbuf;
}

static grn_obj *
proc_define_selector(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 17) ? grn_get_ctype(&vars[16].value) : GRN_CONTENT_JSON;

  if (nvars == 17) {
    grn_proc_create(ctx,
                    GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
                    GRN_PROC_COMMAND, proc_select, NULL, NULL, nvars - 1, vars + 1);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 16);
  }
  print_return_code(ctx, outbuf, ct);
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

  ct = (nvars >= 6) ? grn_get_ctype(&vars[5].value) : GRN_CONTENT_JSON;

  if (nvars == 6) {
    grn_load(ctx, grn_get_ctype(&vars[4].value),
             GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
             GRN_TEXT_VALUE(&vars[2].value), GRN_TEXT_LEN(&vars[2].value),
             GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
             GRN_TEXT_VALUE(&vars[3].value), GRN_TEXT_LEN(&vars[3].value));
    if (ctx->impl->loader.stat != GRN_LOADER_END) {
      grn_ctx_set_next_expr(ctx, proc);
    } else {
      grn_obj body;
      GRN_TEXT_INIT(&body, 0);
      grn_text_itoa(ctx, &body, ctx->impl->loader.nrecords);
      print_return_code_with_body(ctx, outbuf, ct, &body);
      if (ctx->impl->loader.table) {
        grn_db_touch(ctx, DB_OBJ(ctx->impl->loader.table)->db);
      }
      /* maybe necessary : grn_ctx_loader_clear(ctx); */
      grn_obj_unlink(ctx, &body);
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 6);
    print_return_code(ctx, outbuf, ct);
  }
  return outbuf;
}

static grn_obj *
proc_status(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 1) ? grn_get_ctype(&vars[0].value) : GRN_CONTENT_JSON;

  if (nvars == 1) {
    grn_obj body;
    grn_timeval now;
    grn_timeval_now(ctx, &now);

    switch (ct) {
    case GRN_CONTENT_TSV:
      /* TODO: implement */
      break;
    case GRN_CONTENT_JSON:
      {
        GRN_TEXT_INIT(&body, 0);
        GRN_TEXT_PUTS(ctx, &body, "{\"alloc_count\":");
        grn_text_itoa(ctx, &body, grn_alloc_count());
        GRN_TEXT_PUTS(ctx, &body, ",\"starttime\":");
        grn_text_itoa(ctx, &body, grn_starttime.tv_sec);
        GRN_TEXT_PUTS(ctx, &body, ",\"uptime\":");
        grn_text_itoa(ctx, &body, now.tv_sec - grn_starttime.tv_sec);
        GRN_TEXT_PUTS(ctx, &body, ",\"version\":\"");
        GRN_TEXT_PUTS(ctx, &body, grn_get_version());
        GRN_TEXT_PUTC(ctx, &body, '"');
        GRN_TEXT_PUTC(ctx, &body, '}');
        print_return_code_with_body(ctx, outbuf, ct, &body);
        grn_obj_unlink(ctx, &body);
      }
      break;
    case GRN_CONTENT_XML:
    case GRN_CONTENT_NONE:
      break;
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 1);
    print_return_code(ctx, outbuf, ct);
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
    } else if (!memcmp(nptr, "TABLE_VIEW", 10)) {
      flags |= GRN_OBJ_TABLE_VIEW;
      nptr += 10;
    } else if (!memcmp(nptr, "KEY_NORMALIZE", 13)) {
      flags |= GRN_OBJ_KEY_NORMALIZE;
      nptr += 13;
    } else if (!memcmp(nptr, "KEY_WITH_SIS", 12)) {
      flags |= GRN_OBJ_KEY_WITH_SIS;
      nptr += 12;
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid flags option: %.*s", end - nptr, nptr);
      return 0;
    }
  }
  return flags;
}

static grn_obj_flags
grn_parse_column_create_flags(grn_ctx *ctx, const char *nptr, const char *end)
{
  grn_obj_flags flags = 0;
  while (nptr < end) {
    if (*nptr == '|' || *nptr == ' ') {
      nptr += 1;
      continue;
    }
    if (!memcmp(nptr, "COLUMN_SCALAR", 13)) {
      flags |= GRN_OBJ_COLUMN_SCALAR;
      nptr += 13;
    } else if (!memcmp(nptr, "COLUMN_VECTOR", 13)) {
      flags |= GRN_OBJ_COLUMN_VECTOR;
      nptr += 13;
    } else if (!memcmp(nptr, "COLUMN_INDEX", 12)) {
      flags |= GRN_OBJ_COLUMN_INDEX;
      nptr += 12;
    } else if (!memcmp(nptr, "WITH_SECTION", 12)) {
      flags |= GRN_OBJ_WITH_SECTION;
      nptr += 12;
    } else if (!memcmp(nptr, "WITH_WEIGHT", 11)) {
      flags |= GRN_OBJ_WITH_WEIGHT;
      nptr += 11;
    } else if (!memcmp(nptr, "WITH_POSITION", 13)) {
      flags |= GRN_OBJ_WITH_POSITION;
      nptr += 13;
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid flags option: %.*s", end - nptr, nptr);
      return 0;
    }
  }
  return flags;
}

static void
grn_table_create_flags_to_text(grn_ctx *ctx, grn_obj *buf, grn_obj_flags flags)
{
  GRN_BULK_REWIND(buf);
  switch (flags & GRN_OBJ_TABLE_TYPE_MASK) {
  case GRN_OBJ_TABLE_HASH_KEY:
    GRN_TEXT_PUTS(ctx, buf, "TABLE_HASH_KEY");
    break;
  case GRN_OBJ_TABLE_PAT_KEY:
    GRN_TEXT_PUTS(ctx, buf, "TABLE_PAT_KEY");
    break;
  case GRN_OBJ_TABLE_NO_KEY:
    GRN_TEXT_PUTS(ctx, buf, "TABLE_NO_KEY");
    break;
  case GRN_OBJ_TABLE_VIEW:
    GRN_TEXT_PUTS(ctx, buf, "TABLE_VIEW");
    break;
  }
  if (flags & GRN_OBJ_KEY_WITH_SIS) {
    GRN_TEXT_PUTS(ctx, buf, "|KEY_WITH_SIS");
  }
  if (flags & GRN_OBJ_KEY_NORMALIZE) {
    GRN_TEXT_PUTS(ctx, buf, "|KEY_NORMALIZE");
  }
  if (flags & GRN_OBJ_PERSISTENT) {
    GRN_TEXT_PUTS(ctx, buf, "|PERSISTENT");
  }
}

static void
grn_column_create_flags_to_text(grn_ctx *ctx, grn_obj *buf, grn_obj_flags flags)
{
  GRN_BULK_REWIND(buf);
  switch (flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_SCALAR:
    GRN_TEXT_PUTS(ctx, buf, "COLUMN_SCALAR");
    break;
  case GRN_OBJ_COLUMN_VECTOR:
    GRN_TEXT_PUTS(ctx, buf, "COLUMN_VECTOR");
    break;
  case GRN_OBJ_COLUMN_INDEX:
    GRN_TEXT_PUTS(ctx, buf, "COLUMN_INDEX");
    if (flags & GRN_OBJ_WITH_SECTION) {
      GRN_TEXT_PUTS(ctx, buf, "|WITH_SECTION");
    }
    if (flags & GRN_OBJ_WITH_WEIGHT) {
      GRN_TEXT_PUTS(ctx, buf, "|WITH_WEIGHT");
    }
    if (flags & GRN_OBJ_WITH_POSITION) {
      GRN_TEXT_PUTS(ctx, buf, "|WITH_POSITION");
    }
    break;
  }
  switch (flags & GRN_OBJ_COMPRESS_MASK) {
  case GRN_OBJ_COMPRESS_NONE:
    GRN_TEXT_PUTS(ctx, buf, "|COMPRESS_NONE");
    break;
  case GRN_OBJ_COMPRESS_ZLIB:
    GRN_TEXT_PUTS(ctx, buf, "|COMPRESS_ZLIB");
    break;
  case GRN_OBJ_COMPRESS_LZO:
    GRN_TEXT_PUTS(ctx, buf, "|COMPRESS_LZO");
    break;
  }
  if (flags & GRN_OBJ_PERSISTENT) {
    GRN_TEXT_PUTS(ctx, buf, "|PERSISTENT");
  }
}

static grn_obj *
proc_table_create(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 6) ? grn_get_ctype(&vars[5].value) : GRN_CONTENT_JSON;

  if (nvars == 6) {
    grn_obj *table;
    const char *rest;
    grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(&vars[1].value),
                                   GRN_BULK_CURR(&vars[1].value), &rest);
    if (GRN_TEXT_VALUE(&vars[1].value) == rest) {
      flags = grn_parse_table_create_flags(ctx, GRN_TEXT_VALUE(&vars[1].value),
                                           GRN_BULK_CURR(&vars[1].value));
      if (ctx->rc) {
        print_return_code(ctx, buf, ct);
        return buf;
      }
    }
    if (GRN_TEXT_LEN(&vars[0].value)) {
      flags |= GRN_OBJ_PERSISTENT;
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
    } else {
      ERR(GRN_INVALID_ARGUMENT, "should not create anonymous table");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 6);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_table_remove(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 2) ? grn_get_ctype(&vars[1].value) : GRN_CONTENT_JSON;

  if (nvars == 2) {
    grn_obj *table;

    table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                             GRN_TEXT_LEN(&vars[0].value));

    if (table) {
      grn_obj_remove(ctx,table);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "table not found.");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 2);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_column_create(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 6) ? grn_get_ctype(&vars[5].value) : GRN_CONTENT_JSON;

  if (nvars == 6) {
    grn_obj *column, *table, *type;
    const char *rest;
    grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(&vars[2].value),
                                   GRN_BULK_CURR(&vars[2].value), &rest);
    if (GRN_TEXT_VALUE(&vars[2].value) == rest) {
      flags = grn_parse_column_create_flags(ctx, GRN_TEXT_VALUE(&vars[2].value),
                                            GRN_BULK_CURR(&vars[2].value));
      if (ctx->rc) {
        print_return_code(ctx, buf, ct);
        return buf;
      }
    }
    table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                        GRN_TEXT_LEN(&vars[0].value));
    type = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[3].value),
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
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 6);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}


static grn_obj *
proc_column_remove(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 3) ? grn_get_ctype(&vars[2].value) : GRN_CONTENT_JSON;

  if (nvars == 3) {
    grn_obj *table, *col;
    char *colname,fullname[GRN_TABLE_MAX_KEY_SIZE];
    unsigned colname_len,fullname_len;

    table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                             GRN_TEXT_LEN(&vars[0].value));

    colname = GRN_TEXT_VALUE(&vars[1].value);
    colname_len = GRN_TEXT_LEN(&vars[1].value);

    if ((fullname_len = grn_obj_name(ctx, table, fullname, GRN_TABLE_MAX_KEY_SIZE))) {
      fullname[fullname_len] = GRN_DB_DELIMITER;
      memcpy((fullname + fullname_len + 1), colname, colname_len);
      fullname_len += colname_len + 1;
      //TODO:check fullname_len < GRN_TABLE_MAX_KEY_SIZE
      col = grn_ctx_get(ctx, fullname, fullname_len);
      if (col) {
        grn_obj_remove(ctx, col);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "column not found.");
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT, "table not found.");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 3);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}


#define GRN_STRLEN(s) ((s) ? strlen(s) : 0)

static void
objid2name(grn_ctx *ctx, grn_id id, grn_obj *bulk)
{
  GRN_BULK_REWIND(bulk);
  if (id == GRN_ID_NIL) {
    GRN_TEXT_PUTS(ctx, bulk, "null");
  } else {
    grn_obj *obj;
    int name_len;
    char name_buf[GRN_TABLE_MAX_KEY_SIZE];

    obj = grn_ctx_at(ctx, id);
    name_len = grn_obj_name(ctx, obj, name_buf, GRN_TABLE_MAX_KEY_SIZE);
    GRN_TEXT_PUT(ctx, bulk, name_buf, name_len);
  }
}

static void
column2name(grn_ctx *ctx, grn_obj *obj, grn_obj *bulk)
{
  int name_len;
  char name_buf[GRN_TABLE_MAX_KEY_SIZE];

  name_len = grn_column_name(ctx, obj, name_buf, GRN_TABLE_MAX_KEY_SIZE);
  GRN_TEXT_PUT(ctx, bulk, name_buf, name_len);
}

static void
obj_source_to_json(grn_ctx *ctx, grn_db_obj *obj, grn_obj *bulk)
{
  grn_obj o;
  grn_id *s = obj->source;
  int i = 0, n = obj->source_size / sizeof(grn_id);

  GRN_TEXT_INIT(&o, 0);
  GRN_TEXT_PUTC(ctx, bulk, '[');
  for (i = 0; i < n; i++, s++) {
    if (i) { GRN_TEXT_PUTC(ctx, bulk, ','); }
    objid2name(ctx, *s, &o);
    grn_text_otoj(ctx, bulk, &o, NULL);
  }
  GRN_TEXT_PUTC(ctx, bulk, ']');
  grn_obj_close(ctx, &o);
}

static int
print_columninfo(grn_ctx *ctx, grn_obj *column, grn_obj *buf, grn_content_type otype)
{
  grn_obj o;
  grn_id id;
  char *type;
  const char *path;

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
  path = grn_obj_path(ctx, column);
  GRN_TEXT_INIT(&o, 0);

  switch (otype) {
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    column2name(ctx, column, &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    GRN_TEXT_PUTS(ctx, buf, type);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_column_create_flags_to_text(ctx, &o, column->header.flags);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    objid2name(ctx, column->header.domain, &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    objid2name(ctx, grn_obj_get_range(ctx, column), &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    objid2name(ctx, grn_obj_get_range(ctx, column), &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    break;
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, buf, '[');
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, ',');
    column2name(ctx, column, &o);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, ',');
    GRN_TEXT_PUTS(ctx, buf, type);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_column_create_flags_to_text(ctx, &o, column->header.flags);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    objid2name(ctx, column->header.domain, &o);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    objid2name(ctx, grn_obj_get_range(ctx, column), &o);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    obj_source_to_json(ctx, (grn_db_obj *)column, buf);
    GRN_TEXT_PUTC(ctx, buf, ']');
    break;
  case GRN_CONTENT_XML:
  case GRN_CONTENT_NONE:
    break;
  }
  grn_obj_close(ctx, &o);
  return 1;
}

static int
print_tableinfo(grn_ctx *ctx, grn_obj *table, grn_obj *buf, grn_content_type otype)
{
  grn_id id;
  grn_obj o;
  const char *path;

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
  path = grn_obj_path(ctx, table);
  GRN_TEXT_INIT(&o, 0);

  switch (otype) {
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, '\t');
    objid2name(ctx, id, &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    grn_table_create_flags_to_text(ctx, &o, table->header.flags);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    objid2name(ctx, table->header.domain, &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    GRN_TEXT_PUTC(ctx, buf, '\t');
    objid2name(ctx, grn_obj_get_range(ctx, table), &o);
    grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&o), GRN_TEXT_LEN(&o));
    break;
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, buf, '[');
    grn_text_itoa(ctx, buf, id);
    GRN_TEXT_PUTC(ctx, buf, ',');
    objid2name(ctx, id, &o);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_text_esc(ctx, buf, path, GRN_STRLEN(path));
    GRN_TEXT_PUTC(ctx, buf, ',');
    grn_table_create_flags_to_text(ctx, &o, table->header.flags);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    objid2name(ctx, table->header.domain, &o);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ',');
    objid2name(ctx, grn_obj_get_range(ctx, table), &o);
    grn_text_otoj(ctx, buf, &o, NULL);
    GRN_TEXT_PUTC(ctx, buf, ']');
    break;
  case GRN_CONTENT_XML:
  case GRN_CONTENT_NONE:
    break;
  }
  grn_obj_close(ctx, &o);
  return 1;
}

static grn_obj *
proc_column_list(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 2) ? grn_get_ctype(&vars[1].value) : GRN_CONTENT_JSON;

  if (nvars == 2) {
    grn_obj *table;

    grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);

    if ((table = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value),
                             GRN_TEXT_LEN(&vars[0].value)))) {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)cols) >= 0) {
          grn_id *key;
          grn_obj body;
          char line_delimiter, column_delimiter;

          GRN_TEXT_INIT(&body, 0);

          switch (ct) {
          case GRN_CONTENT_TSV:
            line_delimiter = '\n';
            column_delimiter = '\t';
            GRN_TEXT_PUTS(ctx, &body, "id\tname\tpath\ttype\tflags\tdomain\trange");
            break;
          case GRN_CONTENT_JSON:
            line_delimiter = ',';
            column_delimiter = ',';
            GRN_TEXT_PUTS(ctx, &body, "[[[\"id\", \"UInt32\"],[\"name\",\"ShortText\"],[\"path\",\"ShortText\"],[\"type\",\"ShortText\"],[\"flags\",\"ShortText\"],[\"domain\", \"ShortText\"],[\"range\", \"ShortText\"],[\"source\",\"ShortText\"]]");
            break;
          case GRN_CONTENT_XML:
          case GRN_CONTENT_NONE:
            break;
          }

          GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col;
            if ((col = grn_ctx_at(ctx, *key))) {
              GRN_TEXT_PUTC(ctx, &body, line_delimiter);
              if (!print_columninfo(ctx, col, &body, ct)) {
                grn_bulk_truncate(ctx, &body, GRN_BULK_VSIZE(&body) - 1);
              }
              grn_obj_unlink(ctx, col);
            }
          });
          if (ct == GRN_CONTENT_JSON) {
            GRN_TEXT_PUTC(ctx, &body, ']');
          }
          print_return_code_with_body(ctx, buf, ct, &body);
          grn_obj_unlink(ctx, &body);
        }
        grn_hash_close(ctx, cols);
      }
      grn_obj_unlink(ctx, table);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "table '%.*s' is not exist.",
        GRN_TEXT_LEN(&vars[0].value),
        GRN_TEXT_VALUE(&vars[0].value));
      print_return_code(ctx, buf, ct);
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 2);
    print_return_code(ctx, buf, ct);
  }
  return buf;
}

static grn_obj *
proc_table_list(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 1) ? grn_get_ctype(&vars[0].value) : GRN_CONTENT_JSON;

  if (nvars == 1) {
    grn_table_cursor *cur;

    if ((cur = grn_table_cursor_open(ctx, ctx->impl->db, NULL, 0, NULL, 0, 0, -1, 0))) {
      grn_id id;
      grn_obj body;
      char line_delimiter, column_delimiter;

      GRN_TEXT_INIT(&body, 0);

      grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);

      switch (ct) {
      case GRN_CONTENT_TSV:
        line_delimiter = '\n';
        column_delimiter = '\t';
        GRN_TEXT_PUTS(ctx, &body, "id\tname\tpath\tflags\tdomain\trange");
        break;
      case GRN_CONTENT_JSON:
        line_delimiter = ',';
        column_delimiter = ',';
        GRN_TEXT_PUTS(ctx, &body, "[[[\"id\", \"UInt32\"],[\"name\",\"ShortText\"],[\"path\",\"ShortText\"],[\"flags\",\"ShortText\"],[\"domain\", \"ShortText\"],[\"range\",\"ShortText\"]]");
        break;
      case GRN_CONTENT_XML:
      case GRN_CONTENT_NONE:
        break;
      }
      while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
        grn_obj *o;

        if ((o = grn_ctx_at(ctx, id))) {
          GRN_TEXT_PUTC(ctx, &body, line_delimiter);
          if (!print_tableinfo(ctx, o, &body, ct)) {
            grn_bulk_truncate(ctx, &body, GRN_BULK_VSIZE(&body) - 1);
          }
          grn_obj_unlink(ctx, o);
        }
      }
      if (ct == GRN_CONTENT_JSON) {
        GRN_TEXT_PUTC(ctx, &body, ']');
      }
      grn_table_cursor_close(ctx, cur);
      print_return_code_with_body(ctx, buf, ct, &body);
      grn_obj_unlink(ctx, &body);
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 1);
    print_return_code(ctx, buf, ct);
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
  if ((fd = open(path, O_RDONLY|O_NOFOLLOW)) == -1) {
    switch (errno) {
    case EACCES :
      ERR(GRN_OPERATION_NOT_PERMITTED, "request is not allowed.");
      break;
    case ENOENT :
      ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY, "no such file.");
      break;
#ifndef WIN32
    case ELOOP :
      ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY, "symbolic link is not allowed.");
      break;
#endif /* WIN32 */
    default :
      ERR(GRN_UNKNOWN_ERROR, "open() failed(errno: %d).", errno);
      break;
    }
    return 0;
  }
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
  } else {
    ERR(GRN_INVALID_ARGUMENT, "cannot stat file");
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
    grn_str_url_path_normalize(ctx,
                               GRN_TEXT_VALUE(&vars[0].value),
                               GRN_TEXT_LEN(&vars[0].value),
                               path + grn_admin_html_path_len + 1,
                               PATH_MAX - grn_admin_html_path_len - 1);
    grn_bulk_put_from_file(ctx, buf, path);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 2);
    print_return_code(ctx, buf, GRN_CONTENT_JSON);
  }
  return buf;
}

static grn_obj *
proc_view_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 3) ? grn_get_ctype(&vars[2].value) : GRN_CONTENT_JSON;

  if (nvars == 3) {
    grn_obj *view = grn_ctx_get(ctx,
                                GRN_TEXT_VALUE(&vars[0].value),
                                GRN_TEXT_LEN(&vars[0].value));
    grn_obj *table = grn_ctx_get(ctx,
                                GRN_TEXT_VALUE(&vars[1].value),
                                GRN_TEXT_LEN(&vars[1].value));
    grn_view_add(ctx, view, table);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 3);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_quit(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 1) ? grn_get_ctype(&vars[0].value) : GRN_CONTENT_JSON;

  if (nvars == 1) {
    ctx->stat = GRN_CTX_QUITTING;
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 1);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_shutdown(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 1) ? grn_get_ctype(&vars[0].value) : GRN_CONTENT_JSON;

  if (nvars == 1) {
    grn_gctx.stat = GRN_CTX_QUIT;
    ctx->stat = GRN_CTX_QUITTING;
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 1);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_clearlock(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int olen;
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0], *obj;

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 2) ? grn_get_ctype(&vars[1].value) : GRN_CONTENT_JSON;

  if (nvars == 2) {
    olen = GRN_TEXT_LEN(&vars[0].value);

    if (olen) {
      obj = grn_ctx_get(ctx, GRN_TEXT_VALUE(&vars[0].value), olen);
    } else {
      obj = ctx->impl->db;
    }

    if (obj) {
      grn_obj_clear_lock(ctx, obj);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "clear object not found");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 2);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static char slev[] = " EACewnid-";

static grn_logger_info info;

static grn_obj *
proc_log_level(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 2) ? grn_get_ctype(&vars[1].value) : GRN_CONTENT_JSON;

  if (nvars == 2) {
    char *p;
    if (GRN_TEXT_LEN(&vars[0].value) &&
        (p = strchr(slev, GRN_TEXT_VALUE(&vars[0].value)[0]))) {
      info.max_level = (int)(p - slev);
      info.flags = GRN_LOG_TIME|GRN_LOG_MESSAGE;
      info.func = NULL;
      info.func_arg = NULL;
      grn_logger_info_set(ctx, &info);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid log level.");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 2);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_log_put(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 3) ? grn_get_ctype(&vars[2].value) : GRN_CONTENT_JSON;

  if (nvars == 3) {
    char *p;
    if (GRN_TEXT_LEN(&vars[0].value) &&
        (p = strchr(slev, GRN_TEXT_VALUE(&vars[0].value)[0]))) {
      GRN_TEXT_PUTC(ctx, &vars[1].value, '\0');
      GRN_LOG(ctx, (int)(p - slev), "%s", GRN_TEXT_VALUE(&vars[1].value));
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid log level.");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 3);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_log_reopen(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 1) ? grn_get_ctype(&vars[0].value) : GRN_CONTENT_JSON;

  if (nvars == 1) {
    grn_log_reopen(ctx);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 1);
  }
  print_return_code(ctx, buf, ct);
  return buf;
}

static grn_obj *
proc_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *buf = args[0];
  /* TODO: implement */
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "proc_add is not implemented.");
  print_return_code(ctx, buf, GRN_CONTENT_JSON);
  return buf;
}

static grn_obj *
proc_set(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 6) ? grn_get_ctype(&vars[5].value) : GRN_CONTENT_JSON;

  if (nvars == 6) {
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
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 6);
  }
  return outbuf;
}

static grn_rc
proc_get_resolve_parameters(grn_ctx *ctx, grn_expr_var *vars, grn_obj *outbuf,
                            grn_obj **table, grn_id *id)
{
  const char *table_text, *id_text, *key_text;
  int table_length, id_length, key_length;

  table_text = GRN_TEXT_VALUE(&vars[0].value);
  table_length = GRN_TEXT_LEN(&vars[0].value);
  if (table_length == 0) {
    ERR(GRN_INVALID_ARGUMENT, "table isn't specified");
    return ctx->rc;
  }

  *table = grn_ctx_get(ctx, table_text, table_length);
  if (!*table) {
    ERR(GRN_INVALID_ARGUMENT,
        "table doesn't exist: <%.*s>", table_length, table_text);
    return ctx->rc;
  }

  key_text = GRN_TEXT_VALUE(&vars[1].value);
  key_length = GRN_TEXT_LEN(&vars[1].value);
  id_text = GRN_TEXT_VALUE(&vars[4].value);
  id_length = GRN_TEXT_LEN(&vars[4].value);
  switch ((*table)->header.type) {
  case GRN_TABLE_NO_KEY:
    if (key_length) {
      ERR(GRN_INVALID_ARGUMENT,
          "should not specify key for NO_KEY table: <%.*s>: table: <%.*s>",
          key_length, key_text,
          table_length, table_text);
      return ctx->rc;
    }
    if (id_length) {
      const char *rest = NULL;
      *id = grn_atoi(id_text, id_text + id_length, &rest);
      if (rest == id_text) {
        ERR(GRN_INVALID_ARGUMENT,
            "ID should be a number: <%.*s>: table: <%.*s>",
            id_length, id_text,
            table_length, table_text);
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "ID isn't specified: table: <%.*s>",
          table_length, table_text);
    }
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_VIEW:
    if (key_length && id_length) {
      ERR(GRN_INVALID_ARGUMENT,
          "should not specify both key and ID: "
          "key: <%.*s>: ID: <%.*s>: table: <%.*s>",
          key_length, key_text,
          id_length, id_text,
          table_length, table_text);
      return ctx->rc;
    }
    if (key_length) {
      *id = grn_table_get(ctx, *table, key_text, key_length);
      if (!*id) {
        ERR(GRN_INVALID_ARGUMENT,
            "nonexistent key: <%.*s>: table: <%.*s>",
            key_length, key_text,
            table_length, table_text);
      }
    } else {
      if (id_length) {
        const char *rest = NULL;
        *id = grn_atoi(id_text, id_text + id_length, &rest);
        if (rest == id_text) {
          ERR(GRN_INVALID_ARGUMENT,
              "ID should be a number: <%.*s>: table: <%.*s>",
              id_length, id_text,
              table_length, table_text);
        }
      } else {
        ERR(GRN_INVALID_ARGUMENT,
            "key nor ID isn't specified: table: <%.*s>",
            table_length, table_text);
      }
    }
    break;
  default:
    ERR(GRN_INVALID_ARGUMENT, "not a table: <%.*s>", table_length, table_text);
    break;
  }

  return ctx->rc;
}

static grn_obj *
proc_get(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];
  grn_obj *table = NULL;
  grn_id id;

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 4) ? grn_get_ctype(&vars[3].value) : GRN_CONTENT_JSON;

  if (nvars != 5) {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 5);
    print_return_code(ctx, outbuf, ct);
    return outbuf;
  }

  if (proc_get_resolve_parameters(ctx, vars, outbuf, &table, &id)) {
    print_return_code(ctx, outbuf, ct);
  } else {
    grn_obj obj, body;
    grn_obj_format format;
    GRN_RECORD_INIT(&obj, 0, ((grn_db_obj *)table)->id);
    GRN_OBJ_FORMAT_INIT(&format, 1, 0, 1);
    GRN_RECORD_SET(ctx, &obj, id);
    grn_obj_columns(ctx, table,
                    GRN_TEXT_VALUE(&vars[2].value),
                    GRN_TEXT_LEN(&vars[2].value), &format.columns);
    switch (ct) {
    case GRN_CONTENT_JSON:
      GRN_TEXT_INIT(&body, 0);
      format.flags = 0 /* GRN_OBJ_FORMAT_WITH_COLUMN_NAMES */;
      grn_text_otoj(ctx, &body, &obj, &format);
      print_return_code_with_body(ctx, outbuf, ct, &body);
      grn_obj_unlink(ctx, &body);
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

  return outbuf;
}

static grn_obj *
proc_delete(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc;
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *outbuf = args[0];

  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 4) ? grn_get_ctype(&vars[3].value) : GRN_CONTENT_JSON;

  if (nvars == 4) {
    grn_obj *table = grn_ctx_get(ctx,
                                 GRN_TEXT_VALUE(&vars[0].value),
                                 GRN_TEXT_LEN(&vars[0].value));
    if (table) {
      if (GRN_TEXT_LEN(&vars[1].value) && GRN_TEXT_LEN(&vars[3].value)) {
        ERR(GRN_INVALID_ARGUMENT, "both id and key are specified");
      } else if (GRN_TEXT_LEN(&vars[1].value)) {
        rc = grn_table_delete(ctx, table, GRN_TEXT_VALUE(&vars[1].value),
                                          GRN_TEXT_LEN(&vars[1].value));
      } else if (GRN_TEXT_LEN(&vars[3].value)) {
        const char *end;
        grn_id id = grn_atoui(GRN_TEXT_VALUE(&vars[3].value),
                              GRN_BULK_CURR(&vars[3].value), &end);
        if (end == GRN_BULK_CURR(&vars[3].value)) {
          rc = grn_table_delete_by_id(ctx, table, id);
        } else {
          ERR(GRN_INVALID_ARGUMENT, "invalid id");
        }
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT, "unknown table name");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument number. %d for %d", nvars, 4);
  }
  print_return_code(ctx, outbuf, ct);
  return outbuf;
}

static void
dump_name(grn_ctx *ctx, grn_obj *outbuf, const char *name, int name_len)
{
  grn_obj escaped_name;
  GRN_TEXT_INIT(&escaped_name, 0);
  grn_text_esc(ctx, &escaped_name, name, name_len);
  /* is no character escaped? */
  /* TODO false positive with spaces inside names */
  if (GRN_TEXT_LEN(&escaped_name) == name_len + 2) {
    GRN_TEXT_PUT(ctx, outbuf, name, name_len);
  } else {
    GRN_TEXT_PUT(ctx, outbuf,
                 GRN_TEXT_VALUE(&escaped_name), GRN_TEXT_LEN(&escaped_name));
  }
  grn_obj_close(ctx, &escaped_name);
}

static void
dump_obj_name(grn_ctx *ctx, grn_obj *outbuf, grn_obj *obj)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_len;
  name_len = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
  dump_name(ctx, outbuf, name, name_len);
}

static void
dump_column_name(grn_ctx *ctx, grn_obj *outbuf, grn_obj *column)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_len;
  name_len = grn_column_name(ctx, column, name, GRN_TABLE_MAX_KEY_SIZE);
  dump_name(ctx, outbuf, name, name_len);
}

static void
dump_index_column_sources(grn_ctx *ctx, grn_obj *outbuf, grn_obj *column)
{
  grn_obj sources;
  grn_id *source_ids;
  int i, n;

  GRN_OBJ_INIT(&sources, GRN_BULK, 0, GRN_ID_NIL);
  grn_obj_get_info(ctx, column, GRN_INFO_SOURCE, &sources);

  n = GRN_BULK_VSIZE(&sources) / sizeof(grn_id);
  source_ids = (grn_id *)GRN_BULK_HEAD(&sources);
  if (n > 0) {
    GRN_TEXT_PUTC(ctx, outbuf, ' ');
  }
  for (i = 0; i < n; i++) {
    grn_obj *source;
    if ((source = grn_ctx_at(ctx, *source_ids))) {
      if (i) { GRN_TEXT_PUTC(ctx, outbuf, ','); }
      switch (source->header.type) {
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_VIEW:
        GRN_TEXT_PUTS(ctx, outbuf, "_key");
        break;
      default:
        dump_column_name(ctx, outbuf, source);
        break;
      }
    }
    source_ids++;
  }
  grn_obj_close(ctx, &sources);
}

static void
dump_column(grn_ctx *ctx, grn_obj *outbuf , grn_obj *table, grn_obj *column)
{
  grn_obj *type;
  grn_obj_flags default_flags = GRN_OBJ_PERSISTENT;

  type = grn_ctx_at(ctx, ((grn_db_obj *)column)->range);
  if (!type) {
    // ERR(GRN_RANGE_ERROR, "couldn't get column's type object");
    return;
  }

  GRN_TEXT_PUTS(ctx, outbuf, "column_create ");
  dump_obj_name(ctx, outbuf, table);
  GRN_TEXT_PUTC(ctx, outbuf, ' ');
  dump_column_name(ctx, outbuf, column);
  GRN_TEXT_PUTC(ctx, outbuf, ' ');
  if (type->header.type == GRN_TYPE) {
    default_flags |= type->header.flags;
  }
  grn_text_itoa(ctx, outbuf, column->header.flags & ~default_flags);
  GRN_TEXT_PUTC(ctx, outbuf, ' ');
  dump_obj_name(ctx, outbuf, type);
  if (column->header.flags & GRN_OBJ_COLUMN_INDEX) {
    dump_index_column_sources(ctx, outbuf, column);
  }
  GRN_TEXT_PUTC(ctx, outbuf, '\n');

  grn_obj_unlink(ctx, type);
}

static int
have_index_column(grn_ctx *ctx, grn_obj *table)
{
  int n_index_columns = 0;
  grn_hash *columns;
  columns = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                            GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY);
  if (!columns) {
    ERR(GRN_ERROR, "couldn't create a hash to hold columns");
    return 0;
  }

  if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)columns) >= 0) {
    grn_id *key;

    GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
      grn_obj *column;
      if ((column = grn_ctx_at(ctx, *key))) {
        if (column->header.flags & GRN_OBJ_COLUMN_INDEX) {
          n_index_columns++;
        }
        grn_obj_unlink(ctx, column);
      }
    });
  }
  grn_hash_close(ctx, columns);
  return n_index_columns;
}

static int
reference_column_p(grn_ctx *ctx, grn_obj *column)
{
  grn_obj *range;

  range = grn_ctx_at(ctx, grn_obj_get_range(ctx, column));
  if (!range) {
    return GRN_FALSE;
  }

  switch (range->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_NO_KEY:
  case GRN_TABLE_VIEW:
    return GRN_TRUE;
  default:
    return GRN_FALSE;
  }
}

static void
dump_columns(grn_ctx *ctx, grn_obj *outbuf, grn_obj *table,
             grn_obj *pending_columns)
{
  grn_hash *columns;
  columns = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                            GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY);
  if (!columns) {
    ERR(GRN_ERROR, "couldn't create a hash to hold columns");
    return;
  }

  if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)columns) >= 0) {
    grn_id *key;

    GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
      grn_obj *column;
      if ((column = grn_ctx_at(ctx, *key))) {
        if (reference_column_p(ctx, column)) {
          GRN_PTR_PUT(ctx, pending_columns, column);
        } else {
          dump_column(ctx, outbuf, table, column);
          grn_obj_unlink(ctx, column);
        }
      }
    });
  }
  grn_hash_close(ctx, columns);
}

static void
dump_view(grn_ctx *ctx, grn_obj *outbuf, grn_obj *table)
{
  grn_view *view = (grn_view *)table;
  grn_id id;
  grn_hash_cursor *cursor;

  cursor = grn_hash_cursor_open(ctx, view->hash, NULL, 0, NULL, 0, 0, -1, 0);
  while ((id = grn_hash_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    grn_id *table_id;
    int key_size = grn_hash_cursor_get_key(ctx, cursor, ((void **)&table_id));
    if (key_size != 4) {
      ERR(GRN_ERROR, "corrupted view table");
    }
    GRN_TEXT_PUTS(ctx, outbuf, "view_add ");
    dump_obj_name(ctx, outbuf, table);
    GRN_TEXT_PUTC(ctx, outbuf, ' ');
    dump_obj_name(ctx, outbuf, grn_ctx_at(ctx, *table_id));
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
  }
  grn_hash_cursor_close(ctx, cursor);
}

static void
dump_records(grn_ctx *ctx, grn_obj *outbuf, grn_obj *table)
{
  grn_obj **columns;
  grn_id old_id = 0, id;
  grn_table_cursor *cursor;
  int i, ncolumns, n_use_columns;
  grn_obj columnbuf, delete_commands, use_columns, column_name;

  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_NO_KEY:
    break;
  case GRN_TABLE_VIEW:
    dump_view(ctx, outbuf, table);
    return;
  default:
    return;
  }

  if (grn_table_size(ctx, table) == 0 ||
      (grn_obj_get_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER, NULL) &&
       have_index_column(ctx, table))) {
    return;
  }

  GRN_TEXT_INIT(&delete_commands, 0);

  GRN_TEXT_PUTS(ctx, outbuf, "load --table ");
  dump_obj_name(ctx, outbuf, table);
  GRN_TEXT_PUTS(ctx, outbuf, "\n[\n");

  GRN_PTR_INIT(&columnbuf, GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_obj_columns(ctx, table, DEFAULT_OUTPUT_COLUMNS,
                  strlen(DEFAULT_OUTPUT_COLUMNS), &columnbuf);
  columns = (grn_obj **)GRN_BULK_HEAD(&columnbuf);
  ncolumns = GRN_BULK_VSIZE(&columnbuf)/sizeof(grn_obj *);

  GRN_PTR_INIT(&use_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_TEXT_INIT(&column_name, 0);
  for (i = 0; i < ncolumns; i++) {
    if (columns[i]->header.type == GRN_COLUMN_INDEX) {
      continue;
    }
    GRN_BULK_REWIND(&column_name);
    grn_column_name_(ctx, columns[i], &column_name);
    if (((table->header.type == GRN_TABLE_HASH_KEY ||
          table->header.type == GRN_TABLE_PAT_KEY) &&
         GRN_TEXT_LEN(&column_name) == 3 &&
         !memcmp(GRN_TEXT_VALUE(&column_name), "_id", 3)) ||
        (table->header.type == GRN_TABLE_NO_KEY &&
         GRN_TEXT_LEN(&column_name) == 4 &&
         !memcmp(GRN_TEXT_VALUE(&column_name), "_key", 4))) {
      continue;
    }
    GRN_PTR_PUT(ctx, &use_columns, columns[i]);
  }

  n_use_columns = GRN_BULK_VSIZE(&use_columns) / sizeof(grn_obj *);
  GRN_TEXT_PUTC(ctx, outbuf, '[');
  for (i = 0; i < n_use_columns; i++) {
    grn_obj *column;
    column = *((grn_obj **)GRN_BULK_HEAD(&use_columns) + i);
    if (i) { GRN_TEXT_PUTC(ctx, outbuf, ','); }
    GRN_BULK_REWIND(&column_name);
    grn_column_name_(ctx, column, &column_name);
    grn_text_otoj(ctx, outbuf, &column_name, NULL);
  }
  GRN_TEXT_PUTS(ctx, outbuf, "],\n");

  cursor = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1,
                                 GRN_CURSOR_BY_KEY);
  for (i = 0; (id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL;
       ++i, old_id = id) {
    int is_value_column;
    int j;
    grn_obj buf;
    if (i) { GRN_TEXT_PUTS(ctx, outbuf, ",\n"); }
    if (table->header.type == GRN_TABLE_NO_KEY && old_id + 1 < id) {
      grn_id current_id;
      for (current_id = old_id + 1; current_id < id; current_id++) {
        GRN_TEXT_PUTS(ctx, outbuf, "[],\n");
        GRN_TEXT_PUTS(ctx, &delete_commands, "delete --table ");
        dump_obj_name(ctx, &delete_commands, table);
        GRN_TEXT_PUTS(ctx, &delete_commands, " --id ");
        grn_text_lltoa(ctx, &delete_commands, current_id);
        GRN_TEXT_PUTC(ctx, &delete_commands, '\n');
      }
    }
    GRN_TEXT_PUTC(ctx, outbuf, '[');
    for (j = 0; j < n_use_columns; j++) {
      grn_id range;
      grn_obj *column;
      column = *((grn_obj **)GRN_BULK_HEAD(&use_columns) + j);
      GRN_BULK_REWIND(&column_name);
      grn_column_name_(ctx, column, &column_name);
      if (GRN_TEXT_LEN(&column_name) == 6 &&
          !memcmp(GRN_TEXT_VALUE(&column_name), "_value", 6)) {
        is_value_column = 1;
      } else {
        is_value_column = 0;
      }
      range = grn_obj_get_range(ctx, column);

      if (j) { GRN_TEXT_PUTC(ctx, outbuf, ','); }
      switch (column->header.type) {
      case GRN_COLUMN_VAR_SIZE:
      case GRN_COLUMN_FIX_SIZE:
        switch (column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
        case GRN_OBJ_COLUMN_VECTOR:
          /* TODO: We assume that if |range| is GRN_OBJ_KEY_VAR_SIZE, a vector
                   is GRN_VECTOR, otherwise GRN_UVECTOR. This is not always
                   the case, especially by using GRNAPI with C, it's possible
                   to create GRN_VECTOR with values of constant-size type. */
          if (((struct _grn_type *)grn_ctx_at(ctx, range))->obj.header.flags &
              GRN_OBJ_KEY_VAR_SIZE) {
            GRN_OBJ_INIT(&buf, GRN_VECTOR, 0, range);
            grn_obj_get_value(ctx, column, id, &buf);
            grn_text_otoj(ctx, outbuf, &buf, NULL);
            grn_obj_unlink(ctx, &buf);
          } else {
            GRN_OBJ_INIT(&buf, GRN_UVECTOR, 0, range);
            grn_obj_get_value(ctx, column, id, &buf);
            grn_text_otoj(ctx, outbuf, &buf, NULL);
            grn_obj_unlink(ctx, &buf);
          }
          break;
        case GRN_OBJ_COLUMN_SCALAR:
          {
            GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
            grn_obj_get_value(ctx, column, id, &buf);
            grn_text_otoj(ctx, outbuf, &buf, NULL);
            grn_obj_unlink(ctx, &buf);
          }
          break;
        case GRN_OBJ_COLUMN_INDEX:
          break;
        default:
          ERR(GRN_ERROR, "invalid column type");
          break;
        }
        break;
      case GRN_ACCESSOR:
        {
          GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
          grn_obj_get_value(ctx, column, id, &buf);
          /* XXX maybe, grn_obj_get_range() should not unconditionally return
             GRN_DB_INT32 when column is GRN_ACCESSOR and
             GRN_ACCESSOR_GET_VALUE */
          if (is_value_column) {
            buf.header.domain = ((grn_db_obj *)table)->range;
          }
          grn_text_otoj(ctx, outbuf, &buf, NULL);
          grn_obj_unlink(ctx, &buf);
        }
        break;
      default:
        ERR(GRN_ERROR, "invalid header type %d", column->header.type);
        break;
      }
    }
    GRN_TEXT_PUTC(ctx, outbuf, ']');
  }
  GRN_TEXT_PUTS(ctx, outbuf, "\n]\n");
  GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(&delete_commands),
                            GRN_TEXT_LEN(&delete_commands));
  grn_obj_unlink(ctx, &delete_commands);
  grn_obj_unlink(ctx, &column_name);
  grn_obj_unlink(ctx, &use_columns);

  grn_table_cursor_close(ctx, cursor);
  for (i = 0; i < ncolumns; i++) {
    grn_obj_unlink(ctx, columns[i]);
  }
  grn_obj_unlink(ctx, &columnbuf);
}

static void
dump_table(grn_ctx *ctx, grn_obj *outbuf, grn_obj *table,
           grn_obj *pending_columns)
{
  grn_obj *domain = NULL, *range = NULL;
  grn_obj_flags default_flags = GRN_OBJ_PERSISTENT;
  grn_obj *default_tokenizer;

  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
    domain = grn_ctx_at(ctx, table->header.domain);
    if (domain) {
      default_flags |= domain->header.flags;
    }
    break;
  default:
    break;
  }

  GRN_TEXT_PUTS(ctx, outbuf, "table_create ");
  dump_obj_name(ctx, outbuf, table);
  GRN_TEXT_PUTC(ctx, outbuf, ' ');
  grn_text_itoa(ctx, outbuf, table->header.flags & ~default_flags);
  if (domain) {
    GRN_TEXT_PUTC(ctx, outbuf, ' ');
    dump_obj_name(ctx, outbuf, domain);
  }
  if (((grn_db_obj *)table)->range != GRN_ID_NIL) {
    range = grn_ctx_at(ctx, ((grn_db_obj *)table)->range);
    if (!range) {
      // ERR(GRN_RANGE_ERROR, "couldn't get table's value_type object");
      return;
    }
    if (table->header.type != GRN_TABLE_NO_KEY) {
      GRN_TEXT_PUTC(ctx, outbuf, ' ');
    } else {
      GRN_TEXT_PUTS(ctx, outbuf, " --value_type ");
    }
    dump_obj_name(ctx, outbuf, range);
    grn_obj_unlink(ctx, range);
  }
  default_tokenizer = grn_obj_get_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER,
                                       NULL);
  if (default_tokenizer) {
    GRN_TEXT_PUTS(ctx, outbuf, " --default_tokenizer ");
    dump_obj_name(ctx, outbuf, default_tokenizer);
  }

  GRN_TEXT_PUTC(ctx, outbuf, '\n');

  if (domain) {
    grn_obj_unlink(ctx, domain);
  }

  dump_columns(ctx, outbuf, table, pending_columns);
}

/* can we move this to groonga.h? */
#define GRN_PTR_POP(obj,value) {\
  if (GRN_BULK_VSIZE(obj) >= sizeof(grn_obj *)) {\
    GRN_BULK_INCR_LEN((obj), -(sizeof(grn_obj *)));\
    value = *(grn_obj **)(GRN_BULK_CURR(obj));\
  } else {\
    value = NULL;\
  }\
}

static void
dump_scheme(grn_ctx *ctx, grn_obj *outbuf)
{
  grn_obj *db = ctx->impl->db;
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1,
                                   GRN_CURSOR_BY_ID))) {
    grn_id id;
    grn_obj pending_columns;

    GRN_PTR_INIT(&pending_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
    while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *object;

      if ((object = grn_ctx_at(ctx, id))) {
        switch (object->header.type) {
        case GRN_TABLE_HASH_KEY:
        case GRN_TABLE_PAT_KEY:
        case GRN_TABLE_NO_KEY:
        case GRN_TABLE_VIEW:
          dump_table(ctx, outbuf, object, &pending_columns);
          break;
        default:
          break;
        }
        grn_obj_unlink(ctx, object);
      }
    }
    grn_table_cursor_close(ctx, cur);

    while (GRN_TRUE) {
      grn_obj *table, *column;
      GRN_PTR_POP(&pending_columns, column);
      if (!column) {
        break;
      }
      table = grn_ctx_at(ctx, column->header.domain);
      dump_column(ctx, outbuf, table, column);
      grn_obj_unlink(ctx, column);
      grn_obj_unlink(ctx, table);
    }
    grn_obj_close(ctx, &pending_columns);
  }
}

static void
dump_all_records(grn_ctx *ctx, grn_obj *outbuf)
{
  grn_obj *db = ctx->impl->db;
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1,
                                   GRN_CURSOR_BY_ID))) {
    grn_id id;

    while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *table;

      if ((table = grn_ctx_at(ctx, id))) {
        dump_records(ctx, outbuf, table);
        grn_obj_unlink(ctx, table);
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
}

static grn_obj *
proc_dump(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = args[0];
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);

  dump_scheme(ctx, outbuf);
  /* To update index columns correctly, we first create the whole scheme, then
     load non-derivative records, while skipping records of index columns. That
     way, groonga will silently do the job of updating index columns for us. */
  dump_all_records(ctx, outbuf);

  /* remove the last newline because another one will be added by the calller.
     maybe, the caller of proc functions currently doesn't consider the
     possibility of multiple-line output from proc functions. */
  if (GRN_BULK_VSIZE(outbuf) > 0) {
    grn_bulk_truncate(ctx, outbuf, GRN_BULK_VSIZE(outbuf) - 1);
  }
  return outbuf;
}

static grn_obj *
proc_cache_limit(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0], body;
  uint32_t *mp = grn_cach_max_nentries();
  GRN_TEXT_INIT(&body, 0);
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 2) ? grn_get_ctype(&vars[1].value) : GRN_CONTENT_JSON;
  grn_text_lltoa(ctx, &body, *mp);
  if (nvars == 2 && GRN_TEXT_LEN(&vars[0].value)) {
    const char *rest;
    uint32_t max = grn_atoui(GRN_TEXT_VALUE(&vars[0].value),
                             GRN_BULK_CURR(&vars[0].value), &rest);
    *mp = max;
  }
  print_return_code_with_body(ctx, buf, ct, &body);
  grn_obj_unlink(ctx, &body);
  return buf;
}

static grn_obj *
proc_register(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_content_type ct;
  grn_obj *buf = args[0];
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  ct = (nvars >= 2) ? grn_get_ctype(&vars[1].value) : GRN_CONTENT_JSON;
  if (GRN_TEXT_LEN(&vars[0].value)) {
    const char *name;
    name = GRN_TEXT_VALUE(&vars[0].value);
    grn_db_register_by_name(ctx, name);
  }
  print_return_code(ctx, buf, ct);
  return buf;
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
#define GEO_INT2RAD(x)   ((M_PI / (GEO_RESOLUTION * 180)) * x)

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
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
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
  if ((obj = grn_expr_alloc(ctx, caller, GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
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
        double lng0, lat0, lng1, lat1, p, q, r, m, n, x, y;
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
        r = sqrt(q);
        m = GEO_BES_C1 / (q * r);
        n = GEO_BES_C2 / r;
        x = n * cos(p) * fabs(lng0 - lng1);
        y = m * fabs(lat0 - lat1);
        d = sqrt((x * x) + (y * y));
      }
      break;
    case  GRN_DB_WGS84_GEO_POINT :
      {
        double lng0, lat0, lng1, lat1, p, q, r, m, n, x, y;
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
        r = sqrt(q);
        m = GEO_GRS_C1 / (q * r);
        n = GEO_GRS_C2 / r;
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

#define DEF_COMMAND(name, func, nvars, vars)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1),\
                   GRN_PROC_COMMAND, (func), NULL, NULL, (nvars), (vars)))

void
grn_db_init_builtin_query(grn_ctx *ctx)
{
  grn_expr_var vars[17];

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "match_columns");
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
  DEF_VAR(vars[15], "cache");
  DEF_VAR(vars[16], "output_type");
  DEF_COMMAND("define_selector", proc_define_selector, 17, vars);
  DEF_COMMAND("select", proc_select, 16, vars + 1);

  DEF_VAR(vars[0], "values");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "ifexists");
  DEF_VAR(vars[4], "input_type");
  DEF_VAR(vars[5], "output_type");
  DEF_COMMAND("load", proc_load, 6, vars);

  DEF_VAR(vars[0], "output_type");
  DEF_COMMAND("status", proc_status, 1, vars);

  DEF_VAR(vars[0], "output_type");
  DEF_COMMAND("table_list", proc_table_list, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND("column_list", proc_column_list, 2, vars);

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "flags");
  DEF_VAR(vars[2], "key_type");
  DEF_VAR(vars[3], "value_type");
  DEF_VAR(vars[4], "default_tokenizer");
  DEF_VAR(vars[5], "output_type");
  DEF_COMMAND("table_create", proc_table_create, 6, vars);

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND("table_remove", proc_table_remove, 2, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "flags");
  DEF_VAR(vars[3], "type");
  DEF_VAR(vars[4], "source");
  DEF_VAR(vars[5], "output_type");
  DEF_COMMAND("column_create", proc_column_create, 6, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "output_type");
  DEF_COMMAND("column_remove", proc_column_remove, 3, vars);

  DEF_VAR(vars[0], "path");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND(GRN_EXPR_MISSING_NAME, proc_missing, 2, vars);

  DEF_VAR(vars[0], "view");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "output_type");
  DEF_COMMAND("view_add", proc_view_add, 3, vars);

  DEF_VAR(vars[0], "output_type");
  DEF_COMMAND("quit", proc_quit, 1, vars);
  DEF_COMMAND("shutdown", proc_shutdown, 1, vars);

  DEF_VAR(vars[0], "target_name");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND("clearlock", proc_clearlock, 2, vars);

  DEF_VAR(vars[0], "level");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND("log_level", proc_log_level, 2, vars);

  DEF_VAR(vars[0], "level");
  DEF_VAR(vars[1], "message");
  DEF_VAR(vars[2], "output_type");
  DEF_COMMAND("log_put", proc_log_put, 3, vars);

  DEF_VAR(vars[0], "output_type");
  DEF_COMMAND("log_reopen", proc_log_reopen, 1, vars);


  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "values");
  DEF_VAR(vars[4], "output_columns");
  DEF_VAR(vars[5], "output_type");
  DEF_VAR(vars[6], "id");
  DEF_COMMAND("add", proc_add, 6, vars);
  DEF_COMMAND("set", proc_set, 7, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "output_columns");
  DEF_VAR(vars[3], "output_type");
  DEF_VAR(vars[4], "id");
  DEF_COMMAND("get", proc_get, 5, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "output_type");
  DEF_VAR(vars[3], "id");
  DEF_COMMAND("delete", proc_delete, 4, vars);

  DEF_VAR(vars[0], "max");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND("cache_limit", proc_cache_limit, 2, vars);

  /* TODO: Take "output_type" argument. Do we need GRN_CONTENT_GQTP? */
  DEF_COMMAND("dump", proc_dump, 0, vars);

  DEF_VAR(vars[0], "path");
  DEF_VAR(vars[1], "output_type");
  DEF_COMMAND("register", proc_register, 2, vars);

  DEF_VAR(vars[0], "seed");
  grn_proc_create(ctx, "rand", 4, GRN_PROC_FUNCTION, func_rand, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "now", 3, GRN_PROC_FUNCTION, func_now, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_in_circle", 13, GRN_PROC_FUNCTION,
                  func_geo_in_circle, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_in_rectangle", 16, GRN_PROC_FUNCTION,
                  func_geo_in_rectangle, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance", 12, GRN_PROC_FUNCTION,
                  func_geo_distance, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance2", 13, GRN_PROC_FUNCTION,
                  func_geo_distance2, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance3", 13, GRN_PROC_FUNCTION,
                  func_geo_distance3, NULL, NULL, 0, NULL);

}
