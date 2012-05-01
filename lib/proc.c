/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2012 Brazil

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

#include "proc.h"
#include "ii.h"
#include "db.h"
#include "util.h"
#include "output.h"
#include "pat.h"
#include "geo.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

/**** globals for procs ****/
const char *grn_document_root = NULL;

#define VAR GRN_PROC_GET_VAR_BY_OFFSET

/* bulk must be initialized grn_bulk or grn_msg */
static int
grn_bulk_put_from_file(grn_ctx *ctx, grn_obj *bulk, const char *path)
{
  /* FIXME: implement more smartly with grn_bulk */
  int fd, ret = 0;
  struct stat stat;
  if ((fd = GRN_OPEN(path, O_RDONLY|O_NOFOLLOW)) == -1) {
    switch (errno) {
    case EACCES :
      ERR(GRN_OPERATION_NOT_PERMITTED, "request is not allowed: <%s>", path);
      break;
    case ENOENT :
      ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY, "no such file: <%s>", path);
      break;
#ifndef WIN32
    case ELOOP :
      ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY,
          "symbolic link is not allowed: <%s>", path);
      break;
#endif /* WIN32 */
    default :
      ERR(GRN_UNKNOWN_ERROR, "GRN_OPEN() failed(errno: %d): <%s>", errno, path);
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
        if ((ss = GRN_READ(fd, bp, rest)) == -1) { goto exit; }
      }
      GRN_TEXT_PUT(ctx, bulk, buf, stat.st_size);
      ret = 1;
    }
    GRN_FREE(buf);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "cannot stat file: <%s>", path);
  }
exit :
  GRN_CLOSE(fd);
  return ret;
}

#ifdef stat
#  undef stat
#endif /* stat */

/**** query expander ****/

static grn_rc
substitute_query(grn_ctx *ctx, grn_obj *table, grn_obj *column,
                 const char *key, size_t key_size, grn_obj *dest)
{
  grn_id id;
  grn_rc rc = GRN_END_OF_DATA;
  if ((id = grn_table_get(ctx, table, (const void *)key, (unsigned int)key_size))) {
    if ((column->header.type == GRN_COLUMN_VAR_SIZE) &&
        ((column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR)) {
      unsigned int i, n;
      grn_obj values;
      GRN_TEXT_INIT(&values, GRN_OBJ_VECTOR);
      grn_obj_get_value(ctx, column, id, &values);
      n = grn_vector_size(ctx, &values);
      if (n > 1) { GRN_TEXT_PUTC(ctx, dest, '('); }
      for (i = 0; i < n; i++) {
        const char *value;
        unsigned int length;
        if (i > 0) {
          GRN_TEXT_PUTS(ctx, dest, " OR ");
        }
        if (n > 1) { GRN_TEXT_PUTC(ctx, dest, '('); }
        length = grn_vector_get_element(ctx, &values, i, &value, NULL, NULL);
        GRN_TEXT_PUT(ctx, dest, value, length);
        if (n > 1) { GRN_TEXT_PUTC(ctx, dest, ')'); }
      }
      if (n > 1) { GRN_TEXT_PUTC(ctx, dest, ')'); }
      GRN_OBJ_FIN(ctx, &values);
    } else {
      grn_obj_get_value(ctx, column, id, dest);
    }
    rc = GRN_SUCCESS;
  }
  return rc;
}

static grn_rc
expand_query(grn_ctx *ctx, grn_obj *table, grn_obj *column, grn_expr_flags flags,
             const char *str, unsigned int str_len, grn_obj *dest)
{
  grn_obj buf;
  unsigned int len;
  const char *start, *cur = str, *str_end = str + (size_t)str_len;
  GRN_TEXT_INIT(&buf, 0);
  for (;;) {
    while (cur < str_end && grn_isspace(cur, ctx->encoding)) {
      if (!(len = grn_charlen(ctx, cur, str_end))) { goto exit; }
      GRN_TEXT_PUT(ctx, dest, cur, len);
      cur += len;
    }
    if (str_end <= cur) { break; }
    switch (*cur) {
    case '\0' :
      goto exit;
      break;
    case GRN_QUERY_AND :
    case GRN_QUERY_ADJ_INC :
    case GRN_QUERY_ADJ_DEC :
    case GRN_QUERY_ADJ_NEG :
    case GRN_QUERY_BUT :
    case GRN_QUERY_PARENL :
    case GRN_QUERY_PARENR :
    case GRN_QUERY_PREFIX :
      GRN_TEXT_PUTC(ctx, dest, *cur);
      cur++;
      break;
    case GRN_QUERY_QUOTEL :
      GRN_BULK_REWIND(&buf);
      for (start = cur++; cur < str_end; cur += len) {
        if (!(len = grn_charlen(ctx, cur, str_end))) {
          goto exit;
        } else if (len == 1) {
          if (*cur == GRN_QUERY_QUOTER) {
            cur++;
            break;
          } else if (cur + 1 < str_end && *cur == GRN_QUERY_ESCAPE) {
            cur++;
            len = grn_charlen(ctx, cur, str_end);
          }
        }
        GRN_TEXT_PUT(ctx, &buf, cur, len);
      }
      if (substitute_query(ctx, table, column, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf), dest)) {
        GRN_TEXT_PUT(ctx, dest, start, cur - start);
      }
      break;
    case 'O' :
      if (cur + 2 <= str_end && cur[1] == 'R' &&
          (cur + 2 == str_end || grn_isspace(cur + 2, ctx->encoding))) {
        GRN_TEXT_PUT(ctx, dest, cur, 2);
        cur += 2;
        break;
      }
      /* fallthru */
    default :
      for (start = cur; cur < str_end; cur += len) {
        if (!(len = grn_charlen(ctx, cur, str_end))) {
          goto exit;
        } else if (grn_isspace(cur, ctx->encoding)) {
          break;
        } else if (len == 1) {
          if (*cur == GRN_QUERY_PARENL ||
              *cur == GRN_QUERY_PARENR ||
              *cur == GRN_QUERY_PREFIX) {
            break;
          } else if (flags & GRN_EXPR_ALLOW_COLUMN && *cur == GRN_QUERY_COLUMN) {
            if (cur + 1 < str_end) {
              switch (cur[1]) {
              case '!' :
              case '@' :
              case '^' :
              case '$' :
                cur += 2;
                break;
              case '=' :
                cur += (flags & GRN_EXPR_ALLOW_UPDATE) ? 2 : 1;
                break;
              case '<' :
              case '>' :
                cur += (cur + 2 < str_end && cur[2] == '=') ? 3 : 2;
                break;
              default :
                cur += 1;
                break;
              }
            } else {
              cur += 1;
            }
            GRN_TEXT_PUT(ctx, dest, start, cur - start);
            start = cur;
            break;
          }
        }
      }
      if (start < cur) {
        if (substitute_query(ctx, table, column, start, cur - start, dest)) {
          GRN_TEXT_PUT(ctx, dest, start, cur - start);
        }
      }
      break;
    }
  }
exit :
  GRN_OBJ_FIN(ctx, &buf);
  return GRN_SUCCESS;
}

/**** procs ****/

#define DEFAULT_LIMIT           10
#define DEFAULT_OUTPUT_COLUMNS  "_id, _key, *"
#define DEFAULT_DRILLDOWN_LIMIT           10
#define DEFAULT_DRILLDOWN_OUTPUT_COLUMNS  "_key, _nsubrecs"
#define DUMP_COLUMNS            "_id, _key, _value, *"

grn_rc
grn_select(grn_ctx *ctx, const char *table, unsigned int table_len,
           const char *match_columns, unsigned int match_columns_len,
           const char *query, unsigned int query_len,
           const char *filter, unsigned int filter_len,
           const char *scorer, unsigned int scorer_len,
           const char *sortby, unsigned int sortby_len,
           const char *output_columns, unsigned int output_columns_len,
           int offset, int limit,
           const char *drilldown, unsigned int drilldown_len,
           const char *drilldown_sortby, unsigned int drilldown_sortby_len,
           const char *drilldown_output_columns, unsigned int drilldown_output_columns_len,
           int drilldown_offset, int drilldown_limit,
           const char *cache, unsigned int cache_len,
           const char *match_escalation_threshold, unsigned int match_escalation_threshold_len,
           const char *query_expansion, unsigned int query_expansion_len)
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
    match_escalation_threshold_len + 1 + query_expansion_len + 1 +
    sizeof(grn_content_type) + sizeof(int) * 4;
  long long int threshold, original_threshold = 0;
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
    memcpy(cp, match_escalation_threshold, match_escalation_threshold_len);
    cp += match_escalation_threshold_len; *cp++ = '\0';
    memcpy(cp, query_expansion, query_expansion_len);
    cp += query_expansion_len; *cp++ = '\0';
    memcpy(cp, &output_type, sizeof(grn_content_type)); cp += sizeof(grn_content_type);
    memcpy(cp, &offset, sizeof(int)); cp += sizeof(int);
    memcpy(cp, &limit, sizeof(int)); cp += sizeof(int);
    memcpy(cp, &drilldown_offset, sizeof(int)); cp += sizeof(int);
    memcpy(cp, &drilldown_limit, sizeof(int)); cp += sizeof(int);
    if ((cache = grn_cache_fetch(ctx, cache_key, cache_key_size))) {
      GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(cache), GRN_TEXT_LEN(cache));
      grn_cache_unref(cache_key, cache_key_size);
      LAP(":", "cache(%" GRN_FMT_LLD ")", (long long int)GRN_TEXT_LEN(cache));
      return ctx->rc;
    }
  }
  if (match_escalation_threshold_len) {
    const char *end, *rest;
    original_threshold = grn_ctx_get_match_escalation_threshold(ctx);
    end = match_escalation_threshold + match_escalation_threshold_len;
    threshold = grn_atoll(match_escalation_threshold, end, &rest);
    if (end == rest) {
      grn_ctx_set_match_escalation_threshold(ctx, threshold);
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
          grn_expr_flags flags;
          grn_obj query_expansion_buf;
          GRN_TEXT_INIT(&query_expansion_buf, 0);
          flags = GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN;
          if (query_expansion_len) {
            grn_obj *query_expansion_column;
            query_expansion_column = grn_ctx_get(ctx, query_expansion, query_expansion_len);
            if (query_expansion_column) {
              grn_obj *query_expansion_table;
              query_expansion_table = grn_column_table(ctx, query_expansion_column);
              if (query_expansion_table) {
                expand_query(ctx, query_expansion_table, query_expansion_column, flags,
                             query, query_len, &query_expansion_buf);
                query = GRN_TEXT_VALUE(&query_expansion_buf);
                query_len = GRN_TEXT_LEN(&query_expansion_buf);
                grn_obj_unlink(ctx, query_expansion_table);
              }
              grn_obj_unlink(ctx, query_expansion_column);
            } else {
              ERR(GRN_INVALID_ARGUMENT,
                  "nonexistent query expansion column: <%.*s>",
                  query_expansion_len, query_expansion);
              grn_obj_unlink(ctx, cond);
              GRN_OBJ_FIN(ctx, &query_expansion_buf);
              goto exit;
            }
          }
          grn_expr_parse(ctx, cond, query, query_len,
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND, flags);
          GRN_OBJ_FIN(ctx, &query_expansion_buf);
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
    nhits = res ? grn_table_size(ctx, res) : 0;
    LAP(":", "select(%d)", nhits);

    if (res) {
      uint32_t ngkeys;
      grn_table_sort_key *gkeys = NULL;
      int result_size = 1;
      if (!ctx->rc && drilldown_len) {
        gkeys = grn_table_sort_key_from_str(ctx,
                                            drilldown, drilldown_len,
                                            res, &ngkeys);
        if (gkeys) {
          result_size += ngkeys;
        }
      }
      GRN_OUTPUT_ARRAY_OPEN("RESULT", result_size);

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
        LAP(":", "score(%d)", nhits);
      }

      grn_normalize_offset_and_limit(ctx, nhits, &offset, &limit);

      if (sortby_len &&
          (keys = grn_table_sort_key_from_str(ctx, sortby, sortby_len, res, &nkeys))) {
        if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
          grn_table_sort(ctx, res, offset, limit, sorted, keys, nkeys);
          LAP(":", "sort(%d)", limit);
          GRN_OBJ_FORMAT_INIT(&format, nhits, 0, limit, offset);
          format.flags =
            GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
            GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
          grn_obj_columns(ctx, sorted, output_columns, output_columns_len, &format.columns);
          GRN_OUTPUT_OBJ(sorted, &format);
          GRN_OBJ_FORMAT_FIN(ctx, &format);
          grn_obj_unlink(ctx, sorted);
        }
        grn_table_sort_key_close(ctx, keys, nkeys);
      } else {
        if (!ctx->rc) {
          GRN_OBJ_FORMAT_INIT(&format, nhits, offset, limit, offset);
          format.flags =
            GRN_OBJ_FORMAT_WITH_COLUMN_NAMES|
            GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
          grn_obj_columns(ctx, res, output_columns, output_columns_len, &format.columns);
          GRN_OUTPUT_OBJ(res, &format);
          GRN_OBJ_FORMAT_FIN(ctx, &format);
        }
      }
      LAP(":", "output(%d)", limit);
      if (!ctx->rc && drilldown_len) {
        uint32_t i;
        grn_table_group_result g = {NULL, 0, 0, 1, GRN_TABLE_GROUP_CALC_COUNT, 0};
        if (gkeys) {
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
            LAP(":", "drilldown(%d)", nhits);
          }
          grn_table_sort_key_close(ctx, gkeys, ngkeys);
        }
      }
      if (res != table_) { grn_obj_unlink(ctx, res); }
    } else {
      GRN_OUTPUT_ARRAY_OPEN("RESULT", 0);
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    if (!ctx->rc && cacheable && cache_key_size <= GRN_TABLE_MAX_KEY_SIZE
        && (!cache || cache_len != 2 || *cache != 'n' || *(cache + 1) != 'o')) {
      grn_cache_update(ctx, cache_key, cache_key_size, outbuf);
    }
    if (taintable) { grn_db_touch(ctx, DB_OBJ(table_)->db); }
    grn_obj_unlink(ctx, table_);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid table name: <%.*s>", table_len, table);
  }
exit:
  if (match_escalation_threshold_len) {
    grn_ctx_set_match_escalation_threshold(ctx, original_threshold);
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
  const char *output_columns = GRN_TEXT_VALUE(VAR(6));
  uint32_t output_columns_len = GRN_TEXT_LEN(VAR(6));
  const char *drilldown_output_columns = GRN_TEXT_VALUE(VAR(11));
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
                 GRN_TEXT_VALUE(VAR(14)), GRN_TEXT_LEN(VAR(14)),
                 GRN_TEXT_VALUE(VAR(15)), GRN_TEXT_LEN(VAR(15)),
                 GRN_TEXT_VALUE(VAR(16)), GRN_TEXT_LEN(VAR(16)))) {
  }
  return NULL;
}

static grn_obj *
proc_define_selector(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t i, nvars;
  grn_expr_var *vars;
  grn_proc_get_info(ctx, user_data, &vars, &nvars, NULL);
  for (i = 1; i < nvars; i++) {
    GRN_TEXT_SET(ctx, &((vars + i)->value),
                 GRN_TEXT_VALUE(VAR(i)), GRN_TEXT_LEN(VAR(i)));
  }
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
           GRN_TEXT_VALUE(VAR(3)), GRN_TEXT_LEN(VAR(3)),
           GRN_TEXT_VALUE(VAR(5)), GRN_TEXT_LEN(VAR(5)));
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
  grn_cache_statistics statistics;
  grn_timeval_now(ctx, &now);
  grn_cache_get_statistics(ctx, &statistics);
  GRN_OUTPUT_MAP_OPEN("RESULT", 18);
  GRN_OUTPUT_CSTR("alloc_count");
  GRN_OUTPUT_INT32(grn_alloc_count());
  GRN_OUTPUT_CSTR("starttime");
  GRN_OUTPUT_INT32(grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("uptime");
  GRN_OUTPUT_INT32(now.tv_sec - grn_starttime.tv_sec);
  GRN_OUTPUT_CSTR("version");
  GRN_OUTPUT_CSTR(grn_get_version());
  GRN_OUTPUT_CSTR("n_queries");
  GRN_OUTPUT_INT64(statistics.nfetches);
  GRN_OUTPUT_CSTR("cache_hit_rate");
  if (statistics.nfetches == 0) {
    GRN_OUTPUT_FLOAT(0.0);
  } else {
    double cache_hit_rate;
    cache_hit_rate = (double)statistics.nhits / (double)statistics.nfetches;
    GRN_OUTPUT_FLOAT(cache_hit_rate * 100.0);
  }
  GRN_OUTPUT_CSTR("command_version");
  GRN_OUTPUT_INT32(grn_ctx_get_command_version(ctx));
  GRN_OUTPUT_CSTR("default_command_version");
  GRN_OUTPUT_INT32(grn_get_default_command_version());
  GRN_OUTPUT_CSTR("max_command_version");
  GRN_OUTPUT_INT32(GRN_COMMAND_VERSION_MAX);
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
    } else if (!memcmp(nptr, "TABLE_DAT_KEY", 13)) {
      flags |= GRN_OBJ_TABLE_DAT_KEY;
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
      ERR(GRN_INVALID_ARGUMENT, "invalid flags option: %.*s",
          (int)(end - nptr), nptr);
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
    } else if (!memcmp(nptr, "RING_BUFFER", 11)) {
      flags |= GRN_OBJ_RING_BUFFER;
      nptr += 11;
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid flags option: %.*s",
          (int)(end - nptr), nptr);
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
  case GRN_OBJ_TABLE_DAT_KEY:
    GRN_TEXT_PUTS(ctx, buf, "TABLE_DAT_KEY");
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
    grn_obj *key_type = NULL, *value_type = NULL;
    if (GRN_TEXT_LEN(VAR(2)) > 0) {
      key_type = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(2)),
                             GRN_TEXT_LEN(VAR(2)));
      if (!key_type) {
        ERR(GRN_INVALID_ARGUMENT,
            "[table][create] key type doesn't exist: <%.*s> (%.*s)",
            (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
            (int)GRN_TEXT_LEN(VAR(2)), GRN_TEXT_VALUE(VAR(2)));
        return NULL;
      }
    }
    if (GRN_TEXT_LEN(VAR(3)) > 0) {
      value_type = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(3)),
                               GRN_TEXT_LEN(VAR(3)));
      if (!value_type) {
        ERR(GRN_INVALID_ARGUMENT,
            "[table][create] value type doesn't exist: <%.*s> (%.*s)",
            (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
            (int)GRN_TEXT_LEN(VAR(3)), GRN_TEXT_VALUE(VAR(3)));
        return NULL;
      }
    }
    flags |= GRN_OBJ_PERSISTENT;
    table = grn_table_create(ctx,
                             GRN_TEXT_VALUE(VAR(0)),
                             GRN_TEXT_LEN(VAR(0)),
                             NULL, flags,
                             key_type,
                             value_type);
    if (table) {
      grn_obj_set_info(ctx, table,
                       GRN_INFO_DEFAULT_TOKENIZER,
                       grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(4)),
                                   GRN_TEXT_LEN(VAR(4))));
      grn_obj_unlink(ctx, table);
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][create] should not create anonymous table");
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
proc_table_rename(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *table = NULL;
  if (GRN_TEXT_LEN(VAR(0)) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc, "[table][rename] table name isn't specified");
    goto exit;
  }
  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (!table) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][rename] table isn't found: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    goto exit;
  }
  if (GRN_TEXT_LEN(VAR(1)) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][rename] new table name isn't specified: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    goto exit;
  }
  rc = grn_table_rename(ctx, table,
                        GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)));
  if (rc != GRN_SUCCESS && ctx->rc == GRN_SUCCESS) {
    ERR(rc,
        "[table][rename] failed to rename: <%.*s> -> <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
        (int)GRN_TEXT_LEN(VAR(1)), GRN_TEXT_VALUE(VAR(1)));
  }
exit:
  GRN_OUTPUT_BOOL(!rc);
  if (table) { grn_obj_unlink(ctx, table); }
  return NULL;
}

static grn_obj *
proc_column_create(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *column, *table = NULL, *type = NULL;
  const char *rest;
  grn_obj_flags flags = grn_atoi(GRN_TEXT_VALUE(VAR(2)),
                                 GRN_BULK_CURR(VAR(2)), &rest);
  if (GRN_TEXT_VALUE(VAR(2)) == rest) {
    flags = grn_parse_column_create_flags(ctx, GRN_TEXT_VALUE(VAR(2)),
                                          GRN_BULK_CURR(VAR(2)));
    if (ctx->rc) { goto exit; }
  }
  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT,
        "[column][create] table doesn't exist: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    goto exit;
  }
  type = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(3)),
                     GRN_TEXT_LEN(VAR(3)));
  if (!type) {
    ERR(GRN_INVALID_ARGUMENT,
        "[column][create] type doesn't exist: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(3)), GRN_TEXT_VALUE(VAR(3))) ;
    goto exit;
  }
  if (GRN_TEXT_LEN(VAR(1))) {
    flags |= GRN_OBJ_PERSISTENT;
  } else {
    ERR(GRN_INVALID_ARGUMENT, "[column][create] name is missing");
    goto exit;
  }
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
  if (table) { grn_obj_unlink(ctx, table); }
  if (type) { grn_obj_unlink(ctx, type); }
  return NULL;
}

static grn_obj *
proc_column_remove(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table, *col;
  char *colname,fullname[GRN_TABLE_MAX_KEY_SIZE];
  unsigned int colname_len,fullname_len;

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

static grn_obj *
proc_column_rename(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *table = NULL;
  grn_obj *column = NULL;
  if (GRN_TEXT_LEN(VAR(0)) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc, "[column][rename] table name isn't specified");
    goto exit;
  }
  table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (!table) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[column][rename] table isn't found: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    goto exit;
  }
  if (GRN_TEXT_LEN(VAR(1)) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[column][rename] column name isn't specified: <%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    goto exit;
  }
  column = grn_obj_column(ctx, table,
                          GRN_TEXT_VALUE(VAR(1)), GRN_TEXT_LEN(VAR(1)));
  if (!column) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[column][rename] column isn't found: <%.*s.%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
        (int)GRN_TEXT_LEN(VAR(1)), GRN_TEXT_VALUE(VAR(1)));
    goto exit;
  }
  if (GRN_TEXT_LEN(VAR(2)) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[column][rename] new column name isn't specified: <%.*s.%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
        (int)GRN_TEXT_LEN(VAR(1)), GRN_TEXT_VALUE(VAR(1)));
    goto exit;
  }
  rc = grn_column_rename(ctx, column,
                         GRN_TEXT_VALUE(VAR(2)), GRN_TEXT_LEN(VAR(2)));
  if (rc != GRN_SUCCESS && ctx->rc == GRN_SUCCESS) {
    ERR(rc,
        "[column][rename] failed to rename: <%.*s.%.*s> -> <%.*s.%.*s>",
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
        (int)GRN_TEXT_LEN(VAR(1)), GRN_TEXT_VALUE(VAR(1)),
        (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)),
        (int)GRN_TEXT_LEN(VAR(2)), GRN_TEXT_VALUE(VAR(2)));
  }
exit:
  GRN_OUTPUT_BOOL(!rc);
  if (column) { grn_obj_unlink(ctx, column); }
  if (table) { grn_obj_unlink(ctx, table); }
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
  const char *type;
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
  GRN_OUTPUT_ARRAY_OPEN("COLUMN", 8);
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
    GRN_OUTPUT_ARRAY_OPEN("SOURCES", n);
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
    grn_obj *col;
    int column_list_size = -1;
#ifdef WITH_MESSAGE_PACK
    column_list_size = 1; /* [header, (key), (COLUMNS)] */
    if ((col = grn_obj_column(ctx, table, KEY_NAME, sizeof(KEY_NAME)-1))) {
      column_list_size++;
      grn_obj_unlink(ctx, col);
    }
    if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
      column_list_size += grn_table_columns(ctx, table, NULL, 0,
                                            (grn_obj *)cols);
      grn_hash_close(ctx, cols);
    }
#endif
    if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
      GRN_OUTPUT_ARRAY_OPEN("COLUMN_LIST", column_list_size);
      GRN_OUTPUT_ARRAY_OPEN("HEADER", 8);
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("id");
      GRN_OUTPUT_CSTR("UInt32");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("name");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("path");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("type");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("flags");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("domain");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("range");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
      GRN_OUTPUT_CSTR("source");
      GRN_OUTPUT_CSTR("ShortText");
      GRN_OUTPUT_ARRAY_CLOSE();
      GRN_OUTPUT_ARRAY_CLOSE();
      if ((col = grn_obj_column(ctx, table, KEY_NAME, sizeof(KEY_NAME)-1))) {
        int name_len;
        char name_buf[GRN_TABLE_MAX_KEY_SIZE];
        grn_id id;
        grn_obj buf;
        GRN_TEXT_INIT(&buf, 0);
        GRN_OUTPUT_ARRAY_OPEN("COLUMN", 8);
        id = grn_obj_id(ctx, table);
        GRN_OUTPUT_INT64(id);
        GRN_OUTPUT_CSTR(KEY_NAME);
        GRN_OUTPUT_CSTR("");
        GRN_OUTPUT_CSTR("");
        grn_column_create_flags_to_text(ctx, &buf, 0);
        GRN_OUTPUT_OBJ(&buf, NULL);
        name_len = grn_obj_name(ctx, table, name_buf, GRN_TABLE_MAX_KEY_SIZE);
        GRN_OUTPUT_STR(name_buf, name_len);
        objid2name(ctx, table->header.domain, &buf);
        GRN_OUTPUT_OBJ(&buf, NULL);
        GRN_OUTPUT_ARRAY_OPEN("SOURCES", 0);
        GRN_OUTPUT_ARRAY_CLOSE();
        GRN_OUTPUT_ARRAY_CLOSE();
        GRN_OBJ_FIN(ctx, &buf);
        grn_obj_unlink(ctx, col);
      }
      if (grn_table_columns(ctx, table, NULL, 0, (grn_obj *)cols) >= 0) {
        grn_id *key;
        GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
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
        (int)GRN_TEXT_LEN(VAR(0)),
        GRN_TEXT_VALUE(VAR(0)));
  }
  return NULL;
}

static grn_bool
is_table(grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
  case GRN_TABLE_VIEW:
    return GRN_TRUE;
  default:
    return GRN_FALSE;
  }
}

static int
print_tableinfo(grn_ctx *ctx, grn_obj *table)
{
  grn_id id;
  grn_obj o;
  const char *path;

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
  int table_list_length = -1;

#ifdef WITH_MESSAGE_PACK
  if (ctx->impl->output_type == GRN_CONTENT_MSGPACK) {
    table_list_length = 1; /* header */
    if ((cur = grn_table_cursor_open(ctx, ctx->impl->db, NULL, 0, NULL, 0, 0, -1, 0))) {
      grn_id id;
      while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
        grn_obj *o;
        if ((o = grn_ctx_at(ctx, id))) {
          if (is_table(o)) {
            table_list_length++;
          }
          grn_obj_unlink(ctx, o);
        }
      }
    }
  }
#endif

  if ((cur = grn_table_cursor_open(ctx, ctx->impl->db, NULL, 0, NULL, 0, 0, -1, 0))) {
    grn_id id;
    GRN_OUTPUT_ARRAY_OPEN("TABLE_LIST", table_list_length);
    GRN_OUTPUT_ARRAY_OPEN("HEADER", 6);
    GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
    GRN_OUTPUT_CSTR("id");
    GRN_OUTPUT_CSTR("UInt32");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
    GRN_OUTPUT_CSTR("name");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
    GRN_OUTPUT_CSTR("path");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
    GRN_OUTPUT_CSTR("flags");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
    GRN_OUTPUT_CSTR("domain");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_OPEN("PROPERTY", 2);
    GRN_OUTPUT_CSTR("range");
    GRN_OUTPUT_CSTR("ShortText");
    GRN_OUTPUT_ARRAY_CLOSE();
    GRN_OUTPUT_ARRAY_CLOSE();
    while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *o;
      if ((o = grn_ctx_at(ctx, id))) {
        if (is_table(o)) {
          print_tableinfo(ctx, o);
        }
        grn_obj_unlink(ctx, o);
      }
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    grn_table_cursor_close(ctx, cur);
  }
  return NULL;
}

static grn_obj *
proc_missing(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  uint32_t plen;
  grn_obj *outbuf = ctx->impl->outbuf;
  static int grn_document_root_len = -1;
  if (!grn_document_root) { return NULL; }
  if (grn_document_root_len < 0) {
    size_t l;
    if ((l = strlen(grn_document_root)) > PATH_MAX) {
      return NULL;
    }
    grn_document_root_len = (int)l;
    if (l > 0 && grn_document_root[l - 1] == '/') { grn_document_root_len--; }
  }
  if ((plen = GRN_TEXT_LEN(VAR(0))) + grn_document_root_len < PATH_MAX) {
    char path[PATH_MAX];
    memcpy(path, grn_document_root, grn_document_root_len);
    path[grn_document_root_len] = '/';
    grn_str_url_path_normalize(ctx,
                               GRN_TEXT_VALUE(VAR(0)),
                               GRN_TEXT_LEN(VAR(0)),
                               path + grn_document_root_len + 1,
                               PATH_MAX - grn_document_root_len - 1);
    grn_bulk_put_from_file(ctx, outbuf, path);
  } else {
    uint32_t abbrlen = 32;
    ERR(GRN_INVALID_ARGUMENT,
        "too long path name: <%s/%.*s...> %u(%u)",
        grn_document_root,
        abbrlen < plen ? abbrlen : plen, GRN_TEXT_VALUE(VAR(0)),
        plen + grn_document_root_len, PATH_MAX);
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
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

static grn_obj *
proc_shutdown(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_gctx.stat = GRN_CTX_QUIT;
  ctx->stat = GRN_CTX_QUITTING;
  GRN_OUTPUT_BOOL(!ctx->rc);
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

static grn_obj *
proc_defrag(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  int olen, threshold;
  olen = GRN_TEXT_LEN(VAR(0));

  if (olen) {
    obj = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), olen);
  } else {
    obj = ctx->impl->db;
  }

  threshold = GRN_TEXT_LEN(VAR(1))
    ? grn_atoi(GRN_TEXT_VALUE(VAR(1)), GRN_BULK_CURR(VAR(1)), NULL)
    : 0;

  if (obj) {
    grn_obj_defrag(ctx, obj, threshold);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "defrag object not found");
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
  id_text = GRN_TEXT_VALUE(VAR(3));
  id_length = GRN_TEXT_LEN(VAR(3));
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
  case GRN_TABLE_DAT_KEY:
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
  grn_id id = GRN_ID_NIL;
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

static grn_rc
proc_delete_validate_selector(grn_ctx *ctx, grn_obj *table, grn_obj *table_name,
                              grn_obj *key, grn_obj *id, grn_obj *filter)
{
  grn_rc rc = GRN_SUCCESS;

  if (!table) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] table doesn't exist: <%.*s>",
        (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name));
    return rc;
  }

  if (GRN_TEXT_LEN(key) == 0 &&
      GRN_TEXT_LEN(id) == 0 &&
      GRN_TEXT_LEN(filter) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] either key, id or filter must be specified: "
        "table: <%.*s>",
        (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name));
    return rc;
  }

  if (GRN_TEXT_LEN(key) && GRN_TEXT_LEN(id) && GRN_TEXT_LEN(filter)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "record selector must be one of key, id and filter: "
        "table: <%.*s>, key: <%.*s>, id: <%.*s>, filter: <%.*s>",
        (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(key), GRN_TEXT_VALUE(key),
        (int)GRN_TEXT_LEN(id), GRN_TEXT_VALUE(id),
        (int)GRN_TEXT_LEN(filter), GRN_TEXT_VALUE(filter));
    return rc;
  }

  if (GRN_TEXT_LEN(key) && GRN_TEXT_LEN(id) && GRN_TEXT_LEN(filter) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "can't use both key and id: table: <%.*s>, key: <%.*s>, id: <%.*s>",
        (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(key), GRN_TEXT_VALUE(key),
        (int)GRN_TEXT_LEN(id), GRN_TEXT_VALUE(id));
    return rc;
  }

  if (GRN_TEXT_LEN(key) && GRN_TEXT_LEN(id) == 0 && GRN_TEXT_LEN(filter)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "can't use both key and filter: "
        "table: <%.*s>, key: <%.*s>, filter: <%.*s>",
        (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(key), GRN_TEXT_VALUE(key),
        (int)GRN_TEXT_LEN(filter), GRN_TEXT_VALUE(filter));
    return rc;
  }

  if (GRN_TEXT_LEN(key) == 0 && GRN_TEXT_LEN(id) && GRN_TEXT_LEN(filter)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][record][delete] "
        "can't use both id and filter: "
        "table: <%.*s>, id: <%.*s>, filter: <%.*s>",
        (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name),
        (int)GRN_TEXT_LEN(id), GRN_TEXT_VALUE(id),
        (int)GRN_TEXT_LEN(filter), GRN_TEXT_VALUE(filter));
    return rc;
  }

  return rc;
}

static grn_obj *
proc_delete(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_obj *table_name = VAR(0);
  grn_obj *key = VAR(1);
  grn_obj *id = VAR(2);
  grn_obj *filter = VAR(3);
  grn_obj *table = NULL;

  if (GRN_TEXT_LEN(table_name) == 0) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc, "[table][record][delete] table name isn't specified");
    goto exit;
  }

  table = grn_ctx_get(ctx,
                      GRN_TEXT_VALUE(table_name),
                      GRN_TEXT_LEN(table_name));
  rc = proc_delete_validate_selector(ctx, table, table_name, key, id, filter);
  if (rc != GRN_SUCCESS) { goto exit; }

  if (GRN_TEXT_LEN(key)) {
    grn_obj casted_key;
    if (key->header.domain != table->header.domain) {
      GRN_OBJ_INIT(&casted_key, GRN_BULK, 0, table->header.domain);
      grn_obj_cast(ctx, key, &casted_key, GRN_FALSE);
      key = &casted_key;
    }
    if (ctx->rc) {
      rc = ctx->rc;
    } else {
      rc = grn_table_delete(ctx, table, GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key));
      if (key == &casted_key) {
        GRN_OBJ_FIN(ctx, &casted_key);
      }
    }
  } else if (GRN_TEXT_LEN(id)) {
    const char *end;
    grn_id parsed_id = grn_atoui(GRN_TEXT_VALUE(id), GRN_BULK_CURR(id), &end);
    if (end == GRN_BULK_CURR(id)) {
      rc = grn_table_delete_by_id(ctx, table, parsed_id);
    } else {
      rc = GRN_INVALID_ARGUMENT;
      ERR(rc,
          "[table][record][delete] id should be number: "
          "table: <%.*s>, id: <%.*s>, detail: <%.*s|%c|%.*s>",
          (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name),
          (int)GRN_TEXT_LEN(id), GRN_TEXT_VALUE(id),
          (int)(end - GRN_TEXT_VALUE(id)), GRN_TEXT_VALUE(id),
          end[0],
          (int)(GRN_TEXT_VALUE(id) - end - 1), end + 1);
    }
  } else if (GRN_TEXT_LEN(filter)) {
    grn_obj *cond, *v;

    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, cond, v);
    grn_expr_parse(ctx, cond,
                   GRN_TEXT_VALUE(filter),
                   GRN_TEXT_LEN(filter),
                   NULL, GRN_OP_MATCH, GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc) {
      char original_error_message[GRN_CTX_MSGSIZE];
      strcpy(original_error_message, ctx->errbuf);
      rc = ctx->rc;
      ERR(rc,
          "[table][record][delete] failed to parse filter: "
          "table: <%.*s>, filter: <%.*s>, detail: <%s>",
          (int)GRN_TEXT_LEN(table_name), GRN_TEXT_VALUE(table_name),
          (int)GRN_TEXT_LEN(filter), GRN_TEXT_VALUE(filter),
          original_error_message);
    } else {
      grn_obj *records;

      records = grn_table_select(ctx, table, cond, NULL, GRN_OP_OR);
      if (records) {
        void *key;
        GRN_TABLE_EACH(ctx, records, GRN_ID_NIL, GRN_ID_NIL,
                       result_id, &key, NULL, NULL, {
          grn_id id = *(grn_id *)key;
          grn_table_delete_by_id(ctx, table, id);
        });
        grn_obj_unlink(ctx, records);
      }
    }
    grn_obj_unlink(ctx, cond);
  }

exit :
  if (table) {
    grn_obj_unlink(ctx, table);
  }
  GRN_OUTPUT_BOOL(!rc);
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
      case GRN_TABLE_DAT_KEY:
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
  grn_obj buf;

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
  GRN_TEXT_INIT(&buf, 0);
  grn_column_create_flags_to_text(ctx, &buf, column->header.flags & ~default_flags);
  GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
  GRN_OBJ_FIN(ctx, &buf);
  GRN_TEXT_PUTC(ctx, outbuf, ' ');
  dump_obj_name(ctx, outbuf, type);
  if (column->header.flags & GRN_OBJ_COLUMN_INDEX) {
    dump_index_column_sources(ctx, outbuf, column);
  }
  GRN_TEXT_PUTC(ctx, outbuf, '\n');

  grn_obj_unlink(ctx, type);
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
  case GRN_TABLE_DAT_KEY:
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
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    break;
  case GRN_TABLE_VIEW:
    dump_view(ctx, outbuf, table);
    return;
  default:
    return;
  }

  if (grn_table_size(ctx, table) == 0) {
    return;
  }

  GRN_TEXT_INIT(&delete_commands, 0);

  GRN_TEXT_PUTS(ctx, outbuf, "load --table ");
  dump_obj_name(ctx, outbuf, table);
  GRN_TEXT_PUTS(ctx, outbuf, "\n[\n");

  GRN_PTR_INIT(&columnbuf, GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_obj_columns(ctx, table, DUMP_COLUMNS, strlen(DUMP_COLUMNS), &columnbuf);
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
          table->header.type == GRN_TABLE_PAT_KEY ||
          table->header.type == GRN_TABLE_DAT_KEY) &&
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
  grn_obj buf;

  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
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
  GRN_TEXT_INIT(&buf, 0);
  grn_table_create_flags_to_text(ctx, &buf, table->header.flags & ~default_flags);
  GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
  GRN_OBJ_FIN(ctx, &buf);
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
#define GRN_PTR_POP(obj,value) do {\
  if (GRN_BULK_VSIZE(obj) >= sizeof(grn_obj *)) {\
    GRN_BULK_INCR_LEN((obj), -(sizeof(grn_obj *)));\
    value = *(grn_obj **)(GRN_BULK_CURR(obj));\
  } else {\
    value = NULL;\
  }\
} while (0)

static void
dump_schema(grn_ctx *ctx, grn_obj *outbuf)
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
        case GRN_TABLE_DAT_KEY:
        case GRN_TABLE_NO_KEY:
        case GRN_TABLE_VIEW:
          dump_table(ctx, outbuf, object, &pending_columns);
          break;
        default:
          break;
        }
        grn_obj_unlink(ctx, object);
      } else {
        /* XXX: this clause is executed when MeCab tokenizer is enabled in
           database but the groonga isn't supported MeCab.
           We should return error mesage about it and error exit status
           but it's too difficult for this architecture. :< */
        ERRCLR(ctx);
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
dump_selected_tables_records(grn_ctx *ctx, grn_obj *outbuf, grn_obj *tables)
{
  const char *p, *e;

  p = GRN_TEXT_VALUE(tables);
  e = p + GRN_TEXT_LEN(tables);
  while (p < e) {
    int len;
    grn_obj *table;
    const char *token, *token_e;

    if ((len = grn_isspace(p, ctx->encoding))) {
      p += len;
      continue;
    }

    token = p;
    if (!(('a' <= *p && *p <= 'z') ||
          ('A' <= *p && *p <= 'Z') ||
          (*p == '_'))) {
      while (p < e && !grn_isspace(p, ctx->encoding)) {
        p++;
      }
      GRN_LOG(ctx, GRN_LOG_WARNING, "invalid table name is ignored: <%.*s>\n",
              (int)(p - token), token);
      continue;
    }
    while (p < e &&
           (('a' <= *p && *p <= 'z') ||
            ('A' <= *p && *p <= 'Z') ||
            ('0' <= *p && *p <= '9') ||
            (*p == '_'))) {
      p++;
    }
    token_e = p;
    while (p < e && (len = grn_isspace(p, ctx->encoding))) {
      p += len;
      continue;
    }
    if (p < e && *p == ',') {
      p++;
    }

    if ((table = grn_ctx_get(ctx, token, token_e - token))) {
      dump_records(ctx, outbuf, table);
      grn_obj_unlink(ctx, table);
    } else {
      GRN_LOG(ctx, GRN_LOG_WARNING,
              "nonexistent table name is ignored: <%.*s>\n",
              (int)(token_e - token), token);
    }
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
      } else {
        /* XXX: this clause is executed when MeCab tokenizer is enabled in
           database but the groonga isn't supported MeCab.
           We should return error mesage about it and error exit status
           but it's too difficult for this architecture. :< */
        ERRCLR(ctx);
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
}

static grn_obj *
proc_dump(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *outbuf = ctx->impl->outbuf;
  ctx->impl->output_type = GRN_CONTENT_NONE;
  dump_schema(ctx, outbuf);
  /* To update index columns correctly, we first create the whole schema, then
     load non-derivative records, while skipping records of index columns. That
     way, groonga will silently do the job of updating index columns for us. */
  if (GRN_TEXT_LEN(VAR(0)) > 0) {
    dump_selected_tables_records(ctx, outbuf, VAR(0));
  } else {
    dump_all_records(ctx, outbuf);
  }

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
          (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    }
  }
  return NULL;
}

static grn_obj *
proc_register(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  if (GRN_TEXT_LEN(VAR(0))) {
    const char *name;
    GRN_TEXT_PUTC(ctx, VAR(0), '\0');
    name = GRN_TEXT_VALUE(VAR(0));
    grn_plugin_register(ctx, name);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "path is required");
  }
  GRN_OUTPUT_BOOL(!ctx->rc);
  return NULL;
}

void grn_ii_buffer_check(grn_ctx *ctx, grn_ii *ii, uint32_t seg);

static grn_obj *
proc_check(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)), GRN_TEXT_LEN(VAR(0)));
  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT,
        "no such object: <%.*s>", (int)GRN_TEXT_LEN(VAR(0)), GRN_TEXT_VALUE(VAR(0)));
    GRN_OUTPUT_BOOL(!ctx->rc);
  } else {
    switch (obj->header.type) {
    case GRN_DB :
      GRN_OUTPUT_BOOL(!ctx->rc);
      break;
    case GRN_TABLE_PAT_KEY :
      grn_pat_check(ctx, (grn_pat *)obj);
      break;
    case GRN_TABLE_HASH_KEY :
      grn_hash_check(ctx, (grn_hash *)obj);
      break;
    case GRN_TABLE_DAT_KEY :
    case GRN_TABLE_NO_KEY :
    case GRN_COLUMN_FIX_SIZE :
      GRN_OUTPUT_BOOL(!ctx->rc);
      break;
    case GRN_COLUMN_VAR_SIZE :
      grn_ja_check(ctx, (grn_ja *)obj);
      break;
    case GRN_COLUMN_INDEX :
      {
        grn_ii *ii = (grn_ii *)obj;
        struct grn_ii_header *h = ii->header;
        char buf[8];
        GRN_OUTPUT_ARRAY_OPEN("RESULT", 8);
        {
          uint32_t i, j, g =0, a = 0, b = 0;
          uint32_t max = 0;
          for (i = h->bgqtail; i != h->bgqhead; i = ((i + 1) & (GRN_II_BGQSIZE - 1))) {
            j = h->bgqbody[i];
            g++;
            if (j > max) { max = j; }
          }
          for (i = 0; i < GRN_II_MAX_LSEG; i++) {
            j = h->binfo[i];
            if (j < 0x20000) {
              if (j > max) { max = j; }
              b++;
            }
          }
          for (i = 0; i < GRN_II_MAX_LSEG; i++) {
            j = h->ainfo[i];
            if (j < 0x20000) {
              if (j > max) { max = j; }
              a++;
            }
          }
          GRN_OUTPUT_MAP_OPEN("SUMMARY", 8);
          GRN_OUTPUT_CSTR("flags");
          grn_itoh(h->flags, buf, 8);
          GRN_OUTPUT_STR(buf, 8);
          GRN_OUTPUT_CSTR("max sid");
          GRN_OUTPUT_INT64(h->smax);
          GRN_OUTPUT_CSTR("number of garbage segments");
          GRN_OUTPUT_INT64(g);
          GRN_OUTPUT_CSTR("number of array segments");
          GRN_OUTPUT_INT64(a);
          GRN_OUTPUT_CSTR("max id of array segment");
          GRN_OUTPUT_INT64(h->amax);
          GRN_OUTPUT_CSTR("number of buffer segments");
          GRN_OUTPUT_INT64(b);
          GRN_OUTPUT_CSTR("max id of buffer segment");
          GRN_OUTPUT_INT64(h->bmax);
          GRN_OUTPUT_CSTR("max id of physical segment in use");
          GRN_OUTPUT_INT64(max);
          GRN_OUTPUT_CSTR("number of unmanaged segments");
          GRN_OUTPUT_INT64(h->pnext - a - b - g);
          GRN_OUTPUT_CSTR("total chunk size");
          GRN_OUTPUT_INT64(h->total_chunk_size);
          for (max = 0, i = 0; i < (GRN_II_MAX_CHUNK >> 3); i++) {
            if ((j = h->chunks[i])) {
              int k;
              for (k = 0; k < 8; k++) {
                if ((j & (1 << k))) { max = (i << 3) + j; }
              }
            }
          }
          GRN_OUTPUT_CSTR("max id of chunk segments in use");
          GRN_OUTPUT_INT64(max);
          GRN_OUTPUT_CSTR("number of garbage chunk");
          GRN_OUTPUT_ARRAY_OPEN("NGARBAGES", GRN_II_N_CHUNK_VARIATION);
          for (i = 0; i <= GRN_II_N_CHUNK_VARIATION; i++) {
            GRN_OUTPUT_INT64(h->ngarbages[i]);
          }
          GRN_OUTPUT_ARRAY_CLOSE();
          GRN_OUTPUT_MAP_CLOSE();
          for (i = 0; i < GRN_II_MAX_LSEG; i++) {
            if (h->binfo[i] < 0x20000) { grn_ii_buffer_check(ctx, ii, i); }
          }
        }
        GRN_OUTPUT_ARRAY_CLOSE();
      }
      break;
    }
  }
  return NULL;
}

static grn_obj *
proc_truncate(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int table_name_len = GRN_TEXT_LEN(VAR(0));
  if (table_name_len == 0) {
      ERR(GRN_INVALID_ARGUMENT, "table name is missing");
  } else {
    const char *table_name = GRN_TEXT_VALUE(VAR(0));
    grn_obj *table = grn_ctx_get(ctx, table_name, table_name_len);
    if (!table) {
      ERR(GRN_INVALID_ARGUMENT,
          "no such table: <%.*s>", table_name_len, table_name);
    } else {
      switch (table->header.type) {
      case GRN_TABLE_HASH_KEY :
      case GRN_TABLE_PAT_KEY :
      case GRN_TABLE_DAT_KEY :
      case GRN_TABLE_NO_KEY :
        grn_table_truncate(ctx, table);
        break;
      default:
        {
          grn_obj buffer;
          GRN_TEXT_INIT(&buffer, 0);
          grn_inspect(ctx, &buffer, table);
          ERR(GRN_INVALID_ARGUMENT,
              "not a table object: %.*s",
              (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
        }
        break;
      }
    }
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

static grn_obj *
func_geo_in_circle(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  unsigned char r = GRN_FALSE;
  grn_geo_approximate_type type = GRN_GEO_APPROXIMATE_RECTANGLE;
  switch (nargs) {
  case 4 :
    if (grn_geo_resolve_approximate_type(ctx, args[3], &type) != GRN_SUCCESS) {
      break;
    }
    /* fallthru */
  case 3 :
    r = grn_geo_in_circle(ctx, args[0], args[1], args[2], type);
    break;
  default :
    break;
  }
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
    r = grn_geo_in_rectangle(ctx, args[0], args[1], args[2]);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, r);
  }
  return obj;
}

static grn_obj *
func_geo_distance(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0.0;
  grn_geo_approximate_type type = GRN_GEO_APPROXIMATE_RECTANGLE;
  switch (nargs) {
  case 3 :
    if (grn_geo_resolve_approximate_type(ctx, args[2], &type) != GRN_SUCCESS) {
      break;
    }
    /* fallthru */
  case 2 :
    d = grn_geo_distance(ctx, args[0], args[1], type);
    break;
  default:
    break;
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

/* deprecated. */
static grn_obj *
func_geo_distance2(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
  if (nargs == 2) {
    d = grn_geo_distance_sphere(ctx, args[0], args[1]);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

/* deprecated. */
static grn_obj *
func_geo_distance3(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  double d = 0;
  if (nargs == 2) {
    d = grn_geo_distance_ellipsoid(ctx, args[0], args[1]);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_FLOAT, 0))) {
    GRN_FLOAT_SET(ctx, obj, d);
  }
  return obj;
}

#define DIST(ox,oy) (dists[((lx + 1) * (oy)) + (ox)])

static grn_obj *
func_edit_distance(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int d = 0;
  grn_obj *obj;
  if (nargs == 2) {
    uint32_t cx, lx, cy, ly, *dists;
    char *px, *sx = GRN_TEXT_VALUE(args[0]), *ex = GRN_BULK_CURR(args[0]);
    char *py, *sy = GRN_TEXT_VALUE(args[1]), *ey = GRN_BULK_CURR(args[1]);
    for (px = sx, lx = 0; px < ex && (cx = grn_charlen(ctx, px, ex)); px += cx, lx++);
    for (py = sy, ly = 0; py < ey && (cy = grn_charlen(ctx, py, ey)); py += cy, ly++);
    if ((dists = GRN_MALLOC((lx + 1) * (ly + 1) * sizeof(uint32_t)))) {
      uint32_t x, y;
      for (x = 0; x <= lx; x++) { DIST(x, 0) = x; }
      for (y = 0; y <= ly; y++) { DIST(0, y) = y; }
      for (x = 1, px = sx; x <= lx; x++, px += cx) {
        cx = grn_charlen(ctx, px, ex);
        for (y = 1, py = sy; y <= ly; y++, py += cy) {
          cy = grn_charlen(ctx, py, ey);
          if (cx == cy && !memcmp(px, py, cx)) {
            DIST(x, y) = DIST(x - 1, y - 1);
          } else {
            uint32_t a = DIST(x - 1, y) + 1;
            uint32_t b = DIST(x, y - 1) + 1;
            uint32_t c = DIST(x - 1, y - 1) + 1;
            DIST(x, y) = ((a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c));
          }
        }
      }
      d = DIST(lx, ly);
      GRN_FREE(dists);
    }
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, d);
  }
  return obj;
}

#define DEF_VAR(v,name_str) do {\
  (v).name = (name_str);\
  (v).name_size = GRN_STRLEN(name_str);\
  GRN_TEXT_INIT(&(v).value, 0);\
} while (0)

#define DEF_COMMAND(name, func, nvars, vars)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1),\
                   GRN_PROC_COMMAND, (func), NULL, NULL, (nvars), (vars)))

void
grn_db_init_builtin_query(grn_ctx *ctx)
{
  grn_expr_var vars[18];

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
  DEF_VAR(vars[16], "match_escalation_threshold");
  DEF_VAR(vars[17], "query_expansion");
  DEF_COMMAND("define_selector", proc_define_selector, 18, vars);
  DEF_COMMAND("select", proc_select, 17, vars + 1);

  DEF_VAR(vars[0], "values");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "ifexists");
  DEF_VAR(vars[4], "input_type");
  DEF_VAR(vars[5], "each");
  DEF_COMMAND("load", proc_load, 6, vars);

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

  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "new_name");
  DEF_COMMAND("table_rename", proc_table_rename, 2, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "flags");
  DEF_VAR(vars[3], "type");
  DEF_VAR(vars[4], "source");
  DEF_COMMAND("column_create", proc_column_create, 5, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_COMMAND("column_remove", proc_column_remove, 2, vars);

  DEF_VAR(vars[0], "table");
  DEF_VAR(vars[1], "name");
  DEF_VAR(vars[2], "new_name");
  DEF_COMMAND("column_rename", proc_column_rename, 3, vars);

  DEF_VAR(vars[0], "path");
  DEF_COMMAND(GRN_EXPR_MISSING_NAME, proc_missing, 1, vars);

  DEF_VAR(vars[0], "view");
  DEF_VAR(vars[1], "table");
  DEF_COMMAND("view_add", proc_view_add, 2, vars);

  DEF_COMMAND("quit", proc_quit, 0, vars);

  DEF_COMMAND("shutdown", proc_shutdown, 0, vars);

  DEF_VAR(vars[0], "target_name");
  DEF_COMMAND("clearlock", proc_clearlock, 1, vars);

  DEF_VAR(vars[0], "target_name");
  DEF_VAR(vars[1], "threshold");
  DEF_COMMAND("defrag", proc_defrag, 2, vars);

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
  DEF_VAR(vars[3], "filter");
  DEF_COMMAND("delete", proc_delete, 4, vars);

  DEF_VAR(vars[0], "max");
  DEF_COMMAND("cache_limit", proc_cache_limit, 1, vars);

  DEF_VAR(vars[0], "tables");
  DEF_COMMAND("dump", proc_dump, 1, vars);

  DEF_VAR(vars[0], "path");
  DEF_COMMAND("register", proc_register, 1, vars);

  DEF_VAR(vars[0], "obj");
  DEF_COMMAND("check", proc_check, 1, vars);

  DEF_VAR(vars[0], "table");
  DEF_COMMAND("truncate", proc_truncate, 1, vars);

  DEF_VAR(vars[0], "seed");
  grn_proc_create(ctx, "rand", 4, GRN_PROC_FUNCTION, func_rand, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "now", 3, GRN_PROC_FUNCTION, func_now, NULL, NULL, 0, vars);

  grn_proc_create(ctx, "geo_in_circle", 13, GRN_PROC_FUNCTION,
                  func_geo_in_circle, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_in_rectangle", 16, GRN_PROC_FUNCTION,
                  func_geo_in_rectangle, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "geo_distance", 12, GRN_PROC_FUNCTION,
                  func_geo_distance, NULL, NULL, 0, NULL);

  /* deprecated. */
  grn_proc_create(ctx, "geo_distance2", 13, GRN_PROC_FUNCTION,
                  func_geo_distance2, NULL, NULL, 0, NULL);

  /* deprecated. */
  grn_proc_create(ctx, "geo_distance3", 13, GRN_PROC_FUNCTION,
                  func_geo_distance3, NULL, NULL, 0, NULL);

  grn_proc_create(ctx, "edit_distance", 13, GRN_PROC_FUNCTION,
                  func_edit_distance, NULL, NULL, 0, NULL);
}
