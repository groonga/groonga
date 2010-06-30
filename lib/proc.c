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
#include "util.h"
#include "output.h"

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

/**** globals for procs ****/
const char *grn_admin_html_path = NULL;

#define VAR GRN_PROC_GET_VAR_BY_OFFSET

/**** procs ****/

#define DEFAULT_LIMIT           10
#define DEFAULT_OUTPUT_COLUMNS  "_id _key _value *"
#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key _nsubrecs"

#define LAP(msg) {\
  uint64_t et;\
  grn_timeval tv;\
  grn_timeval_now(ctx, &tv);\
  et = (tv.tv_sec - ctx->impl->tv.tv_sec) * GRN_TIME_USEC_PER_SEC\
    + (tv.tv_usec - ctx->impl->tv.tv_usec);\
  GRN_LOG(ctx, GRN_LOG_NONE, "%08x|:%012llu %s", (intptr_t)ctx, et, msg);\
}

grn_rc
grn_select(grn_ctx *ctx, const char *table, unsigned table_len,
           const char *match_columns, unsigned match_columns_len,
           const char *query, unsigned query_len,
           const char *filter, unsigned filter_len,
           const char *scorer, unsigned scorer_len,
           const char *sortby, unsigned sortby_len,
           const char *output_columns, unsigned output_columns_len,
           int offset, int limit,
           const char *drilldown, unsigned drilldown_len,
           const char *drilldown_sortby, unsigned drilldown_sortby_len,
           const char *drilldown_output_columns, unsigned drilldown_output_columns_len,
           int drilldown_offset, int drilldown_limit,
           const char *cache, unsigned cache_len)
{
  uint32_t nkeys, nhits;
  uint16_t cacheable = 1, taintable = 0;
  grn_obj_format format;
  grn_table_sort_key *keys;
  grn_obj *outbuf = ctx->impl->outbuf;
  grn_content_type output_type = ctx->impl->output_type;
  grn_obj *table_, *match_columns_ = NULL, *cond, *scorer_, *res = NULL, *sorted;
  char cache_key[GRN_TABLE_MAX_KEY_SIZE];
  uint32_t cache_key_size = table_len + 1 + match_columns_len + 1 + query_len + 1 +
    filter_len + 1 + scorer_len + 1 + sortby_len + 1 + output_columns_len + 1 +
    drilldown_len + 1 + drilldown_sortby_len + 1 + drilldown_output_columns_len +
    sizeof(grn_content_type) + sizeof(int) * 4;
  if (cache_key_size <= GRN_TABLE_MAX_KEY_SIZE) {
    grn_obj *cache;
    char *cp = cache_key;
    memcpy(cp, table, table_len);
    cp += table_len; *cp++ = '\0';
    memcpy(cp, match_columns, match_columns_len);
    cp += match_columns_len; *cp++ = '\0';
    memcpy(cp, query, query_len);
    cp += query_len; *cp++ = '\0';
    memcpy(cp, filter, filter_len);
    cp += filter_len; *cp++ = '\0';
    memcpy(cp, scorer, scorer_len);
    cp += scorer_len; *cp++ = '\0';
    memcpy(cp, sortby, sortby_len);
    cp += sortby_len; *cp++ = '\0';
    memcpy(cp, output_columns, output_columns_len);
    cp += output_columns_len; *cp++ = '\0';
    memcpy(cp, drilldown, drilldown_len);
    cp += drilldown_len; *cp++ = '\0';
    memcpy(cp, drilldown_sortby, drilldown_sortby_len);
    cp += drilldown_sortby_len; *cp++ = '\0';
    memcpy(cp, drilldown_output_columns, drilldown_output_columns_len);
    cp += drilldown_output_columns_len; *cp++ = '\0';
    memcpy(cp, &output_type, sizeof(grn_content_type)); cp += sizeof(grn_content_type);
    memcpy(cp, &offset, sizeof(int)); cp += sizeof(int);
    memcpy(cp, &limit, sizeof(int)); cp += sizeof(int);
    memcpy(cp, &drilldown_offset, sizeof(int)); cp += sizeof(int);
    memcpy(cp, &drilldown_limit, sizeof(int)); cp += sizeof(int);
    if ((cache = grn_cache_fetch(ctx, cache_key, cache_key_size))) {
      GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(cache), GRN_TEXT_LEN(cache));
      grn_cache_unref(cache_key, cache_key_size);
      LAP("cache");
      return ctx->rc;
    }
  }
  if ((table_ = grn_ctx_get(ctx, table, table_len))) {
    // match_columns_ = grn_obj_column(ctx, table_, match_columns, match_columns_len);
    if (query_len || filter_len) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, cond, v);
      if (cond) {
        if (match_columns_len) {
          GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, match_columns_, v);
          if (match_columns_) {
            grn_expr_parse(ctx, match_columns_, match_columns, match_columns_len,
                           NULL, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
          } else {
            /* todo */
          }
        }
        if (query_len) {
          grn_expr_parse(ctx, cond, query, query_len,
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
          if (!ctx->rc && filter_len) {
            grn_expr_parse(ctx, cond, filter, filter_len,
                           match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
            if (!ctx->rc) { grn_expr_append_op(ctx, cond, GRN_OP_AND, 2); }
          }
        } else {
          grn_expr_parse(ctx, cond, filter, filter_len,
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT);
        }
        cacheable *= ((grn_expr *)cond)->cacheable;
        taintable += ((grn_expr *)cond)->taintable;
        /*
        grn_obj strbuf;
        GRN_TEXT_INIT(&strbuf, 0);
        grn_expr_inspect(ctx, &strbuf, cond);
        GRN_TEXT_PUTC(ctx, &strbuf, '\0');
        GRN_LOG(ctx, GRN_LOG_NOTICE, "query=(%s)", GRN_TEXT_VALUE(&strbuf));
        GRN_OBJ_FIN(ctx, &strbuf);
        */
        if (!ctx->rc) { res = grn_table_select(ctx, table_, cond, NULL, GRN_OP_OR); }
        if (match_columns_) { grn_obj_unlink(ctx, match_columns_); }
        grn_obj_unlink(ctx, cond);
      } else {
        /* todo */
        ERRCLR(ctx);
      }
    } else {
      res = table_;
    }
    LAP("select");
    GRN_OUTPUT_ARRAY_OPEN("RESULTPAGE", -1);
    if (res) {
      if (scorer && scorer_len) {
        grn_obj *v;
        GRN_EXPR_CREATE_FOR_QUERY(ctx, res, scorer_, v);
        if (scorer_ && v) {
          grn_table_cursor *tc;
          grn_expr_parse(ctx, scorer_, scorer, scorer_len, NULL, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
          cacheable *= ((grn_expr *)scorer_)->cacheable;
          taintable += ((grn_expr *)scorer_)->taintable;
          if ((tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, 0))) {
            while (!grn_table_cursor_next_o(ctx, tc, v)) {
              grn_expr_exec(ctx, scorer_, 0);
            }
            grn_table_cursor_close(ctx, tc);
          }
          grn_obj_unlink(ctx, scorer_);
        }
        LAP("score");
      }
      nhits = grn_table_size(ctx, res);

      grn_normalize_offset_and_limit(ctx, nhits, &offset, &limit);
      ERRCLR(ctx);

      if (sortby_len) {
        if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
          if ((keys = grn_table_sort_key_from_str(ctx, sortby, sortby_len, res, &nkeys))) {
            grn_table_sort(ctx, res, offset, limit, sorted, keys, nkeys);
            LAP("sort");
            GRN_OBJ_FORMAT_INIT(&format, nhits, 0, limit, offset);
            format.flags =
              GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
              GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
            grn_obj_columns(ctx, sorted, output_columns, output_columns_len, &format.columns);
            GRN_OUTPUT_OBJ(sorted, &format);
            GRN_OBJ_FORMAT_FIN(ctx, &format);
            grn_table_sort_key_close(ctx, keys, nkeys);
          }
          grn_obj_unlink(ctx, sorted);
        }
      } else {
        GRN_OBJ_FORMAT_INIT(&format, nhits, offset, limit, offset);
        format.flags =
          GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
          GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
        grn_obj_columns(ctx, res, output_columns, output_columns_len, &format.columns);
        GRN_OUTPUT_OBJ(res, &format);
        GRN_OBJ_FORMAT_FIN(ctx, &format);
      }
      LAP("output");
      if (!ctx->rc && drilldown_len) {
        uint32_t i, ngkeys;
        grn_table_sort_key *gkeys;
        grn_table_group_result g = {NULL, 0, 0, 1, GRN_TABLE_GROUP_CALC_COUNT, 0};
        if ((gkeys = grn_table_sort_key_from_str(ctx, drilldown,
                                                 drilldown_len, res, &ngkeys))) {
          for (i = 0; i < ngkeys; i++) {
            if ((g.table = grn_table_create_for_group(ctx, NULL, 0, NULL,
                                                      GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                                      gkeys[i].key, NULL))) {
              int n_drilldown_offset = drilldown_offset,
                  n_drilldown_limit = drilldown_limit;

              grn_table_group(ctx, res, &gkeys[i], 1, &g, 1);
              nhits = grn_table_size(ctx, g.table);

              grn_normalize_offset_and_limit(ctx, nhits,
                                             &n_drilldown_offset, &n_drilldown_limit);
              ERRCLR(ctx);

              if (drilldown_sortby_len) {
                if ((keys = grn_table_sort_key_from_str(ctx,
                                                        drilldown_sortby, drilldown_sortby_len,
                                                        g.table, &nkeys))) {
                  if ((sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                                 NULL, g.table))) {
                    grn_table_sort(ctx, g.table, n_drilldown_offset, n_drilldown_limit,
                                   sorted, keys, nkeys);
                    GRN_OBJ_FORMAT_INIT(&format, nhits, 0,
                                        n_drilldown_limit, n_drilldown_offset);
                    format.flags =
                      GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
                      GRN_OBJ_FORMAT_XML_ELEMENT_NAVIGATIONENTRY;
                    grn_obj_columns(ctx, sorted,
                                    drilldown_output_columns, drilldown_output_columns_len,
                                    &format.columns);
                    GRN_OUTPUT_OBJ(sorted, &format);
                    GRN_OBJ_FORMAT_FIN(ctx, &format);
                    grn_obj_unlink(ctx, sorted);
                  }
                  grn_table_sort_key_close(ctx, keys, nkeys);
                }
              } else {
                GRN_OBJ_FORMAT_INIT(&format, nhits, n_drilldown_offset,
                                    n_drilldown_limit, n_drilldown_offset);
                format.flags =
                  GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
                  GRN_OBJ_FORMAT_XML_ELEMENT_NAVIGATIONENTRY;
                grn_obj_columns(ctx, g.table, drilldown_output_columns,
                                drilldown_output_columns_len, &format.columns);
                GRN_OUTPUT_OBJ(g.table, &format);
                GRN_OBJ_FORMAT_FIN(ctx, &format);
              }
              grn_obj_unlink(ctx, g.table);
            }
            LAP("drilldown");
          }
          grn_table_sort_key_close(ctx, gkeys, ngkeys);
        }
      }
      if (res != table_) { grn_obj_unlink(ctx, res); }
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    if (!ctx->rc && cacheable && cache_key_size <= GRN_TABLE_MAX_KEY_SIZE
        && (!cache || cache_len != 2 || *cache != 'n' || *(cache + 1) != 'o')) {
      grn_cache_update(ctx, cache_key, cache_key_size, outbuf);
    }
    if (taintable) { grn_db_touch(ctx, DB_OBJ(table_)->db); }
    grn_obj_unlink(ctx, table_);
  }
  /* GRN_LOG(ctx, GRN_LOG_NONE, "%d", ctx->seqno); */
  return ctx->rc;
}

static grn_obj *
proc_select(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int offset = GRN_TEXT_LEN(VAR(7))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(7)), GRN_BULK_CURR(VAR(7)), NULL)
    : 0;
  int limit = GRN_TEXT_LEN(VAR(8))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(8)), GRN_BULK_CURR(VAR(8)), NULL)
    : DEFAULT_LIMIT;
  char *output_columns = GRN_TEXT_VALUE(VAR(6));
  uint32_t output_columns_len = GRN_TEXT_LEN(VAR(6));
  char *drilldown_output_columns = GRN_TEXT_VALUE(VAR(11));
  uint32_t drilldown_output_columns_len = GRN_TEXT_LEN(VAR(11));
  int drilldown_offset = GRN_TEXT_LEN(VAR(12))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(12)), GRN_BULK_CURR(VAR(12)), NULL)
    : 0;
  int drilldown_limit = GRN_TEXT_LEN(VAR(13))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(13)), GRN_BULK_CURR(VAR(13)), NULL)
    : DEFAULT_DRILLDOWN_LIMIT;
  if (!output_columns_len) {
    output_columns = DEFAULT_OUTPUT_COLUMNS;
    output_columns_len = strlen(DEFAULT_OUTPUT_COLUMNS);
  }
  if (!drilldown_output_columns_len) {
    drilldown_output_columns = DEFAULT_DRILLDOWN_OUTPUT_COLUMNS;
    drilldown_output_columns_len = strlen(DEFAULT_DRILLDOWN_OUTPUT_COLUMNS);
  }
  if (grn_select(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)),
                 GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)),
                 GRN_TEXT_VALUE(VAR(2)), GRN_TEXT_LEN(VAR(2)),
                 GRN_TEXT_VALUE(VAR(3)), GRN_TEXT_LEN(VAR(3)),
                 GRN_TEXT_VALUE(VAR(4)), GRN_TEXT_LEN(VAR(4)),
                 GRN_TEXT_VALUE(VAR(5)), GRN_TEXT_LEN(VAR(5)),
                 output_columns, output_columns_len,
                 offset, limit,
                 GRN_TEXT_VALUE(VAR(9)), GRN_TEXT_LEN(VAR(9)),
                 GRN_TEXT_VALUE(VAR(10)), GRN_TEXT_LEN(VAR(10)),
                 drilldown_output_columns, drilldown_output_columns_len,
                 drilldown_offset, drilldown_limit,
                 GRN_TEXT_VALUE(VAR(14)), GRN_TEXT_LEN(VAR(14)))) {
  }
  return NULL;
}

static grn_obj *
proc_define_selector(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  grn_proc_create(ctx,
                  GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)),
                  GRN_PROC_COMMAND, proc_select, NULL, NULL, nvars - 1, vars + 1);
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_load(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_load(ctx, grn_get_ctype(VAR(4)),
           GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)),
           GRN_TEXT_VALUE(VAR(2)), GRN_TEXT_LEN(VAR(2)),
           GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)),
           GRN_TEXT_VALUE(VAR(3)), GRN_TEXT_LEN(VAR(3)));
  if (ctx->impl->loader.stat != GRN_LOADER_END) {
    grn_ctx_set_next_expr(ctx, grn_proc_get_info(ctx, user_data, NULL, NULL, NULL));
  } else {
    GRN_OUTPUT_INT64(ctx->impl->loader.nrecords);
    if (ctx->impl->loader.table) {
      grn_db_touch(ctx, DB_OBJ(ctx->impl->loader.table)->db);
    }
    /* maybe necessary : grn_ctx_loader_clear(ctx); */
  }
  return NULL;
}

static grn_obj *
proc_status(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_timeval now;
  grn_timeval_now(ctx, &now);
  GRN_OUTPUT_MAP_OPEN("STATUS", 8);
  GRN_OUTPUT_CSTR("alloc_count");
  GRN_OUTPUT_INT32(grn_alloc_count());
  GRN_OUTPUT_CSTR("starttime");
  GRN_OUTPUT_INT32(grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("uptime");
  GRN_OUTPUT_INT32(now.tv_sec - grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("version");
  GRN_OUTPUT_CSTR(grn_get_version());
  GRN_OUTPUT_MAP_CLOSE();
  return NULL;
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
  grn_obj *table;
  const char *rest;
  grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(VAR(1)),
                                 GRN_BULK_CURR(VAR(1)), &rest);
  if (GRN_TEXT_VALUE(VAR(1)) == rest) {
    flags = grn_parse_table_create_flags(ctx, GRN_TEXT_VALUE(VAR(1)),
                                         GRN_BULK_CURR(VAR(1)));
    if (ctx->rc) { goto exit; }
  }
  if (GRN_TEXT_LEN(VAR(0))) {
    flags |= GRN_OBJ_PERSISTENT;
    table = grn_table_create(ctx,
                             GRN_TEXT_VALUE(VAR(0)),
                             GRN_TEXT_LEN(VAR(0)),
                             NULL, flags,
                             grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(2)),
                                         GRN_TEXT_LEN(VAR(2))),
                             grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(3)),
                                         GRN_TEXT_LEN(VAR(3))));
    if (table) {
      grn_obj_set_info(ctx, table,
                       GRN_INFO_DEFAULT_TOKENIZER,
                       grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(4)),
                                   GRN_TEXT_LEN(VAR(4))));
      grn_obj_unlink(ctx, table);
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "should not create anonymous table");
  }
exit:
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_table_remove(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table;
  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)),
                           GRN_TEXT_LEN(VAR(0)));
  if (table) {
    grn_obj_remove(ctx,table);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "table not found.");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_column_create(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *column, *table, *type;
  const char *rest;
  grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(VAR(2)),
                                 GRN_BULK_CURR(VAR(2)), &rest);
  if (GRN_TEXT_VALUE(VAR(2)) == rest) {
    flags = grn_parse_column_create_flags(ctx, GRN_TEXT_VALUE(VAR(2)),
                                          GRN_BULK_CURR(VAR(2)));
    if (ctx->rc) { goto exit; }
  }
  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)),
                      GRN_TEXT_LEN(VAR(0)));
  type = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(3)),
                     GRN_TEXT_LEN(VAR(3)));
  if (GRN_TEXT_LEN(VAR(1))) { flags |= GRN_OBJ_PERSISTENT; }
  column = grn_column_create(ctx, table,
                             GRN_TEXT_VALUE(VAR(1)),
                             GRN_TEXT_LEN(VAR(1)),
                             NULL, flags, type);
  if (column) {
    if (GRN_TEXT_LEN(VAR(4))) {
      grn_obj sources, source_ids, **p, **pe;
      GRN_PTR_INIT(&sources, GRN_OBJ_VECTOR, GRN_ID_NIL);
      GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
      grn_obj_columns(ctx, type,
                      GRN_TEXT_VALUE(VAR(4)),
                      GRN_TEXT_LEN(VAR(4)),
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
exit:
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_column_remove(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table, *col;
  char *colname,fullname[GRN_TABLE_MAX_KEY_SIZE];
  unsigned colname_len,fullname_len;

  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)),
                           GRN_TEXT_LEN(VAR(0)));

  colname = GRN_TEXT_VALUE(VAR(1));
  colname_len = GRN_TEXT_LEN(VAR(1));

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
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
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

static int
print_columninfo(grn_ctx *ctx, grn_obj *column)
{
  grn_obj o;
  grn_id id;
  char *type;
  const char *path;

  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE:
    type = "fix";
    break;
  case GRN_COLUMN_VAR_SIZE:
    type = "var";
    break;
  case GRN_COLUMN_INDEX:
    type = "index";
    break;
  default:
    GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid header type %d\n", column->header.type);
    return 0;
  }
  id = grn_obj_id(ctx, column);
  path = grn_obj_path(ctx, column);
  GRN_TEXT_INIT(&o, 0);
  GRN_OUTPUT_ARRAY_OPEN("", 8);
  GRN_OUTPUT_INT64(id);
  column2name(ctx, column, &o);
  GRN_OUTPUT_OBJ(&o, NULL);
  GRN_OUTPUT_CSTR(path);
  GRN_OUTPUT_CSTR(type);
  grn_column_create_flags_to_text(ctx, &o, column->header.flags);
  GRN_OUTPUT_OBJ(&o, NULL);
  objid2name(ctx, column->header.domain, &o);
  GRN_OUTPUT_OBJ(&o, NULL);
  objid2name(ctx, grn_obj_get_range(ctx, column), &o);
  GRN_OUTPUT_OBJ(&o, NULL);
  {
    grn_db_obj *obj = (grn_db_obj *)column;
    grn_id *s = obj->source;
    int i = 0, n = obj->source_size / sizeof(grn_id);
    GRN_OUTPUT_ARRAY_OPEN("", n);
    for (i = 0; i < n; i++, s++) {
      objid2name(ctx, *s, &o);
      GRN_OUTPUT_OBJ(&o, NULL);
    }
    GRN_OUTPUT_ARRAY_CLOSE();

  }
  //  print_obj_source(ctx, (grn_db_obj *)column);
  GRN_OUTPUT_ARRAY_CLOSE();
  GRN_OBJ_FIN(ctx, &o);
  return 1;
}

static grn_obj *
proc_column_list(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table;
  if ((table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)),
                           GRN_TEXT_LEN(VAR(0))))) {
    grn_hash *cols;
    if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
      GRN_OUTPUT_ARRAY_OPEN("", -1);
      GRN_OUTPUT_ARRAY_OPEN("", 8);
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("id");
      GRN_OUTPUT_CSTR("UInt32");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("name");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("path");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("type");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("flags");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("domain");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("range");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("", 2);
      GRN_OUTPUT_CSTR("source");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_CLOSE();
      if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)cols) >= 0) {
        grn_id *key;
        GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
          grn_obj *col;
          if ((col = grn_ctx_at(ctx, *key))) {
            print_columninfo(ctx, col);
            grn_obj_unlink(ctx, col);
          }
        });
      }
      GRN_OUTPUT_ARRAY_CLOSE();
      grn_hash_close(ctx, cols);
    }
    grn_obj_unlink(ctx, table);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "table '%.*s' does not exist.",
        GRN_TEXT_LEN(VAR(0)),
        GRN_TEXT_VALUE(VAR(0)));
  }
  return NULL;
}

static int
print_tableinfo(grn_ctx *ctx, grn_obj *table)
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
  GRN_OUTPUT_ARRAY_OPEN("TABLE", 6);
  GRN_OUTPUT_INT64(id);
  objid2name(ctx, id, &o);
  GRN_OUTPUT_OBJ(&o, NULL);
  GRN_OUTPUT_CSTR(path);
  grn_table_create_flags_to_text(ctx, &o, table->header.flags);
  GRN_OUTPUT_OBJ(&o, NULL);
  objid2name(ctx, table->header.domain, &o);
  GRN_OUTPUT_OBJ(&o, NULL);
  objid2name(ctx, grn_obj_get_range(ctx, table), &o);
  GRN_OUTPUT_OBJ(&o, NULL);
  GRN_OUTPUT_ARRAY_CLOSE();
  GRN_OBJ_FIN(ctx, &o);
  return 1;
}

static grn_obj *
proc_table_list(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, ctx->impl->db, NULL, 0, NULL, 0, 0, -1, 0))) {
    grn_id id;
    GRN_OUTPUT_ARRAY_OPEN("TABLE_LIST", -1);
    GRN_OUTPUT_ARRAY_OPEN("COLUMNS", 6);
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_OUTPUT_CSTR("id");
    GRN_OUTPUT_CSTR("UInt32");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_OUTPUT_CSTR("name");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_OUTPUT_CSTR("path");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_OUTPUT_CSTR("flags");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_OUTPUT_CSTR("domain");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    GRN_OUTPUT_CSTR("range");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_CLOSE();
    while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *o;
      if ((o = grn_ctx_at(ctx, id))) {
        print_tableinfo(ctx, o);
        grn_obj_unlink(ctx, o);
      }
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    grn_table_cursor_close(ctx, cur);
  }
  return NULL;
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
  uint32_t plen;
  grn_obj *outbuf = ctx->impl->outbuf;
  static int grn_admin_html_path_len = -1;
  if (!grn_admin_html_path) { return NULL; }
  if (grn_admin_html_path_len < 0) {
    size_t l;
    if ((l = strlen(grn_admin_html_path)) > PATH_MAX) {
      return NULL;
    }
    grn_admin_html_path_len = (int)l;
    if (l > 0 && grn_admin_html_path[l - 1] == PATH_SEPARATOR[0]) { grn_admin_html_path_len--; }
  }
  if ((plen = GRN_TEXT_LEN(VAR(0))) + grn_admin_html_path_len < PATH_MAX) {
    char path[PATH_MAX];
    memcpy(path, grn_admin_html_path, grn_admin_html_path_len);
    path[grn_admin_html_path_len] = PATH_SEPARATOR[0];
    grn_str_url_path_normalize(ctx,
                               GRN_TEXT_VALUE(VAR(0)),
                               GRN_TEXT_LEN(VAR(0)),
                               path + grn_admin_html_path_len + 1,
                               PATH_MAX - grn_admin_html_path_len - 1);
    grn_bulk_put_from_file(ctx, outbuf, path);
  } else {
    uint32_t abbrlen = 32;
    ERR(GRN_INVALID_ARGUMENT,
        "too long path name: <%s%c%.*s...> %u(%u)",
        grn_admin_html_path, PATH_SEPARATOR[0],
        abbrlen < plen ? abbrlen : plen, GRN_TEXT_VALUE(VAR(0)),
        plen + grn_admin_html_path_len, PATH_MAX);
  }
  return NULL;
}

static grn_obj *
proc_view_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *view = grn_ctx_get(ctx,
                              GRN_TEXT_VALUE(VAR(0)),
                              GRN_TEXT_LEN(VAR(0)));
  grn_obj *table = grn_ctx_get(ctx,
                              GRN_TEXT_VALUE(VAR(1)),
                              GRN_TEXT_LEN(VAR(1)));
  grn_view_add(ctx, view, table);
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_quit(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  ctx->stat = GRN_CTX_QUITTING;
  return NULL;
}

static grn_obj *
proc_shutdown(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_gctx.stat = GRN_CTX_QUIT;
  ctx->stat = GRN_CTX_QUITTING;
  return NULL;
}

static grn_obj *
proc_clearlock(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int olen;
  grn_obj *obj;
  olen = GRN_TEXT_LEN(VAR(0));

  if (olen) {
    obj = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), olen);
  } else {
    obj = ctx->impl->db;
  }

  if (obj) {
    grn_obj_clear_lock(ctx, obj);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "clear object not found");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static char slev[] = " EACewnid-";

static grn_logger_info info;

static grn_obj *
proc_log_level(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  char *p;
  if (GRN_TEXT_LEN(VAR(0)) &&
      (p = strchr(slev, GRN_TEXT_VALUE(VAR(0))[0]))) {
    info.max_level = (int)(p - slev);
    info.flags = GRN_LOG_TIME|GRN_LOG_MESSAGE;
    info.func = NULL;
    info.func_arg = NULL;
    grn_logger_info_set(ctx, &info);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid log level.");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_log_put(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  char *p;
  if (GRN_TEXT_LEN(VAR(0)) &&
      (p = strchr(slev, GRN_TEXT_VALUE(VAR(0))[0]))) {
    GRN_TEXT_PUTC(ctx, VAR(1), '\0');
    GRN_LOG(ctx, (int)(p - slev), "%s", GRN_TEXT_VALUE(VAR(1)));
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid log level.");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_log_reopen(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_log_reopen(ctx);
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_add(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  /* TODO: implement */
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "proc_add is not implemented.");
  return NULL;
}

static grn_obj *
proc_set(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
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

static grn_rc
proc_get_resolve_parameters(grn_ctx *ctx, grn_user_data *user_data, grn_obj **table, grn_id *id)
{
  const char *table_text, *id_text, *key_text;
  int table_length, id_length, key_length;

  table_text = GRN_TEXT_VALUE(VAR(0));
  table_length = GRN_TEXT_LEN(VAR(0));
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

  key_text = GRN_TEXT_VALUE(VAR(1));
  key_length = GRN_TEXT_LEN(VAR(1));
  id_text = GRN_TEXT_VALUE(VAR(4));
  id_length = GRN_TEXT_LEN(VAR(4));
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
  grn_id id;
  grn_obj *table = NULL;
  if (!proc_get_resolve_parameters(ctx, user_data, &table, &id)) {
    grn_obj obj;
    grn_obj_format format;
    GRN_RECORD_INIT(&obj, 0, ((grn_db_obj *)table)->id);
    GRN_OBJ_FORMAT_INIT(&format, 1, 0, 1, 0);
    GRN_RECORD_SET(ctx, &obj, id);
    grn_obj_columns(ctx, table, GRN_TEXT_VALUE(VAR(2)), GRN_TEXT_LEN(VAR(2)), &format.columns);
    format.flags = 0 /* GRN_OBJ_FORMAT_WITH_COLUMN_NAMES */;
    GRN_OUTPUT_OBJ(&obj, &format);
    GRN_OBJ_FORMAT_FIN(ctx, &format);
  }
  return NULL;
}

static grn_obj *
proc_delete(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc;
  grn_obj *table = grn_ctx_get(ctx,
                               GRN_TEXT_VALUE(VAR(0)),
                               GRN_TEXT_LEN(VAR(0)));
  if (table) {
    if (GRN_TEXT_LEN(VAR(1)) && GRN_TEXT_LEN(VAR(2))) {
      ERR(GRN_INVALID_ARGUMENT, "both id and key are specified");
    } else if (GRN_TEXT_LEN(VAR(1))) {
      rc = grn_table_delete(ctx, table, GRN_TEXT_VALUE(VAR(1)),
                                        GRN_TEXT_LEN(VAR(1)));
    } else if (GRN_TEXT_LEN(VAR(2))) {
      const char *end;
      grn_id id = grn_atoui(GRN_TEXT_VALUE(VAR(2)),
                            GRN_BULK_CURR(VAR(2)), &end);
      if (end == GRN_BULK_CURR(VAR(2))) {
        rc = grn_table_delete_by_id(ctx, table, id);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "invalid id");
      }
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "unknown table name");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
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
  grn_obj *outbuf = ctx->impl->outbuf;
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
  return NULL;
}

static grn_obj *
proc_cache_limit(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t *mp = grn_cache_max_nentries();
  GRN_OUTPUT_INT64(*mp);
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *rest;
    uint32_t max = grn_atoui(GRN_TEXT_VALUE(VAR(0)),
                             GRN_BULK_CURR(VAR(0)), &rest);
    if (GRN_BULK_CURR(VAR(0)) == rest) {
      *mp = max;
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "max value is invalid unsigned integer format: <%.*s>",
          GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    }
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_register(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *name;
    name = GRN_TEXT_VALUE(VAR(0));
    grn_db_register_by_name(ctx, name);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "path is required");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
func_rand(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int val;
  grn_obj *obj;
  if (nargs > 0) {
    int max = GRN_INT32_VALUE(args[0]);
    val = (int) (1.0 * max * rand() / (RAND_MAX + 1.0));
  } else {
    val = rand();
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_INT32, 0))) {
    GRN_INT32_SET(ctx, obj, val);
  }
  return obj;
}

static grn_obj *
func_now(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  if ((obj = GRN_PROC_ALLOC(GRN_DB_TIME, 0))) {
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
  grn_obj *obj;
  unsigned char r = GRN_FALSE;
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
  if ((obj = GRN_PROC_ALLOC(GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_in_rectangle(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  unsigned char r = GRN_FALSE;
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
  if ((obj = GRN_PROC_ALLOC(GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_distance(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
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
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_geo_distance2(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
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
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

static grn_obj *
func_geo_distance3(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
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
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
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
  DEF_COMMAND("define_selector", proc_define_selector, 16, vars);
  DEF_COMMAND("select", proc_select, 15, vars + 1);

  DEF_VAR(vars[0], "values");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "ifexists");
  DEF_VAR(vars[4], "input_type");
  DEF_COMMAND("load", proc_load, 5, vars);

  DEF_COMMAND("status", proc_status, 0, vars);

  DEF_COMMAND("table_list", proc_table_list, 0, vars);

  DEF_VAR(vars[0], "table");
  DEF_COMMAND("column_list", proc_column_list, 1, vars);

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "flags");
  DEF_VAR(vars[2], "key_type");
  DEF_VAR(vars[3], "value_type");
  DEF_VAR(vars[4], "default_tokenizer");
  DEF_COMMAND("table_create", proc_table_create, 5, vars);

  DEF_VAR(vars[0], "name");
  DEF_COMMAND("table_remove", proc_table_remove, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "flags");
  DEF_VAR(vars[3], "type");
  DEF_VAR(vars[4], "source");
  DEF_COMMAND("column_create", proc_column_create, 5, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_COMMAND("column_remove", proc_column_remove, 2, vars);

  DEF_VAR(vars[0], "path");
  DEF_COMMAND(GRN_EXPR_MISSING_NAME, proc_missing, 1, vars);

  DEF_VAR(vars[0], "view");
  DEF_VAR(vars[1], "table");
  DEF_COMMAND("view_add", proc_view_add, 2, vars);

  DEF_COMMAND("quit", proc_quit, 0, vars);

  DEF_COMMAND("shutdown", proc_shutdown, 0, vars);

  DEF_VAR(vars[0], "target_name");
  DEF_COMMAND("clearlock", proc_clearlock, 1, vars);

  DEF_VAR(vars[0], "level");
  DEF_COMMAND("log_level", proc_log_level, 1, vars);

  DEF_VAR(vars[0], "level");
  DEF_VAR(vars[1], "message");
  DEF_COMMAND("log_put", proc_log_put, 2, vars);

  DEF_COMMAND("log_reopen", proc_log_reopen, 0, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "values");
  DEF_VAR(vars[4], "output_columns");
  DEF_VAR(vars[5], "id");
  DEF_COMMAND("add", proc_add, 5, vars);
  DEF_COMMAND("set", proc_set, 6, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "output_columns");
  DEF_VAR(vars[3], "id");
  DEF_COMMAND("get", proc_get, 4, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "key");
  DEF_VAR(vars[2], "id");
  DEF_COMMAND("delete", proc_delete, 3, vars);

  DEF_VAR(vars[0], "max");
  DEF_COMMAND("cache_limit", proc_cache_limit, 1, vars);

  DEF_COMMAND("dump", proc_dump, 0, vars);

  DEF_VAR(vars[0], "path");
  DEF_COMMAND("register", proc_register, 1, vars);

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
