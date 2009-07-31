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

#include "lib/com.h"
#include "lib/ql.h"
#include <string.h>
#include <stdio.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */
#include <fcntl.h>
#include <sys/stat.h>

#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"
#define DEFAULT_MAX_NFTHREADS 8

static char hostname[HOST_NAME_MAX];
static int port = DEFAULT_PORT;
static int batchmode;
static int newdb;
static int useql;
grn_timeval starttime;

static void
usage(void)
{
  gethostname(hostname, HOST_NAME_MAX);
  fprintf(stderr,
          "Usage: groonga [options...] [dest]\n"
          "options:\n"
          "  -n:                 create new database\n"
          "  -a:                 run in standalone mode (default)\n"
          "  -c:                 run in client mode\n"
          "  -s:                 run in server mode\n"
          "  -d:                 run in daemon mode\n"
          "  -e:                 encoding for new database [none|euc|utf8|sjis|latin1|koi8r]\n"
          "  -l <log level>:     log level\n"
          "  -i <ip/hostname>:   server address to listen (default: %s)\n"
          "  -p <port number>:   server port number (default: %d)\n"
          "  -t <max threads>:   max number of free threads (default: %d)\n"
          "  -h, --help:         show usage\n"
          "dest: <db pathname> [<command>] or <dest hostname>\n"
          "  <db pathname> [<command>]: when standalone/server mode\n"
          "  <dest hostname>: when client mode (default: \"%s\")\n",
          hostname,
          DEFAULT_PORT, DEFAULT_MAX_NFTHREADS, DEFAULT_DEST);
}

inline static void
prompt(void)
{
  if (!batchmode) { fputs("> ", stderr); }
}

#define BUFSIZE 0x1000000

/* TODO: use struct which contains line_delimiter/column_delimiter */

static grn_hash *
parse_http_path(grn_ctx *ctx, char *path, int path_len)
{
  grn_obj buf;
  grn_hash *query;
  const char *p, *e;

  GRN_TEXT_INIT(&buf, 0);

  p = path;
  e = p + path_len;
  p = get_uri_token(ctx, &buf, p, e, '?');

  if ((query = grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE,
                               sizeof(grn_obj), GRN_OBJ_KEY_VAR_SIZE))) {
    while (p < e) {
      grn_id key_id;
      grn_obj *value;

      GRN_BULK_REWIND(&buf);
      p = get_uri_token(ctx, &buf, p, e, '=');
      if ((key_id = grn_hash_add(ctx, (grn_hash *)query,
                                 GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf),
                                 (void **)&value, NULL)) != GRN_ID_NIL) {
        /* TODO: if virtual table is able to have column, use it */
        GRN_TEXT_INIT(value, 0);
        p = get_uri_token(ctx, value, p, e, '&');
      } else {
        /* TODO: error handling */
      }
    }
  }
  return query;
}

void
release_query(grn_ctx *ctx, grn_hash *query)
{
  if (query) {
    grn_obj *value;
    GRN_HASH_EACH(query, id, NULL, NULL, &value, {
      GRN_OBJ_FIN(ctx, value);
    });
    grn_hash_close(ctx, query);
  }
}

/* bulk must be initialized grn_bulk or grn_msg */
static int
grn_bulk_from_file(grn_ctx *ctx, grn_obj *bulk, const char *path)
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
      GRN_TEXT_SET(ctx, bulk, buf, stat.st_size);
      ret = 1;
    }
    GRN_FREE(buf);
  }
exit :
  close(fd);
  return ret;
}

static int
print_columnvalue(grn_ctx *ctx, grn_obj *value, grn_obj *buf, grn_content_type otype)
{
  switch (otype) {
  case GRN_CONTENT_TSV:
    /* TODO: implement tsv */
    break;
  case GRN_CONTENT_JSON:
    {
      grn_text_otoj(ctx, buf, value, NULL);
    }
    break;
  }
  return 1;
}

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
    printf("%d\n", column->header.type);
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
    grn_text_esc(ctx, buf, path, strlen(path));
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
    grn_text_esc(ctx, buf, path, strlen(path));
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
    grn_text_esc(ctx, buf, path, strlen(path));
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
    grn_text_esc(ctx, buf, path, strlen(path));
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

static void
cmd_status(grn_ctx *ctx, grn_obj *buf, grn_content_type otype)
{
  grn_timeval now;
  grn_timeval_now(ctx, &now);

  switch (otype) {
  case GRN_CONTENT_TSV:
    /* TODO: implement */
    break;
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTS(ctx, buf, "{\"starttime\":");
    grn_text_itoa(ctx, buf, starttime.tv_sec);
    GRN_TEXT_PUTS(ctx, buf, ",\"uptime\":");
    grn_text_itoa(ctx, buf, now.tv_sec - starttime.tv_sec);
    GRN_TEXT_PUTC(ctx, buf, '}');
    break;
  }
}

/* TODO: support path */
static void
cmd_createcolumn(grn_ctx *ctx,
                 char *table_name, unsigned table_name_len,
                 char *column_name, unsigned column_name_len,
                 int flags,
                 char *type_name, unsigned type_name_len,
                 grn_obj *buf, grn_content_type otype)
{
  grn_obj *table;
  if ((table = grn_ctx_get(ctx, table_name, table_name_len))) {
    grn_obj *type;
    if ((type = grn_ctx_get(ctx, type_name, type_name_len))) {
      grn_obj *column;
      if ((column = grn_column_create(ctx, table,
                                      column_name, column_name_len,
                                      /* path */NULL, flags, type))) {
        grn_obj_unlink(ctx, column);
        GRN_TEXT_PUTS(ctx, buf, "true");
        return;
      } else {
        /* TODO: error handling */
      }
      grn_obj_unlink(ctx, type);
    }
    grn_obj_unlink(ctx, table);
  }
  GRN_TEXT_PUTS(ctx, buf, "false");
}

static void
cmd_createtable(grn_ctx *ctx,
                char *table_name, unsigned table_name_len,
                int flags,
                char *key_type_name, unsigned key_type_name_len,
                char *value_type_name, unsigned value_type_name_len,
                grn_obj *buf, grn_content_type otype)
{
  grn_obj *key_type;
  if ((key_type = grn_ctx_get(ctx, key_type_name, key_type_name_len))) {
    grn_obj *table;
    grn_obj *value_type = grn_ctx_get(ctx, value_type_name, value_type_name_len);
    if ((table = grn_table_create(ctx, table_name, table_name_len,
                                  NULL, flags,
                                  key_type, value_type))) {
      grn_obj_unlink(ctx, table);
      GRN_TEXT_PUTS(ctx, buf, "true");
      return;
    } else {
      /* TODO: error handling */
    }
    grn_obj_unlink(ctx, key_type);
  }
  GRN_TEXT_PUTS(ctx, buf, "false");
}

/* TODO: use column */
/* TODO: use expr */
/* TODO: use table_cursor flags */
/* if offset < 0, order desc. */
/* if count == -1, show all records */
static void
cmd_recordlist(grn_ctx *ctx, char *table_name, unsigned table_name_len,
               char *sort_column_name, unsigned sort_column_name_len,
               int offset, int count,
               grn_obj *buf, grn_content_type otype)
{
  grn_obj *table;
  if (!offset || count < -1) { return; }
  if ((table = grn_ctx_get(ctx, table_name, table_name_len))) {
    grn_hash *col_ids;
    if ((col_ids = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                   GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
      int ncols;
      if ((ncols = grn_table_columns(ctx, table, NULL, 0,
                                     (grn_obj *)col_ids)) >= 0) {
        grn_obj **cols;
        if ((cols = (grn_obj **)GRN_MALLOC(sizeof(grn_obj *) * (ncols + 2)))) {
          grn_obj *sort_col;
          if ((sort_col = grn_obj_column(ctx, table,
                                         sort_column_name,
                                         sort_column_name_len))) {
            grn_obj *sort;
            if ((sort = grn_table_create(ctx, NULL, 0, NULL,
                                         GRN_OBJ_TABLE_NO_KEY,
                                         NULL, table))) {
              int limit;
              grn_obj **last_cols = cols;
              char line_delimiter, column_delimiter;

              switch (otype) {
              case GRN_CONTENT_TSV:
                line_delimiter = '\n';
                column_delimiter = '\t';
                break;
              case GRN_CONTENT_JSON:
                line_delimiter = ',';
                column_delimiter = ',';
                GRN_TEXT_PUTC(ctx, buf, '[');
                break;
              }

              {
                unsigned int n = grn_table_size(ctx, table);
                grn_text_itoa(ctx, buf, n);
                GRN_TEXT_PUTC(ctx, buf, line_delimiter);
              }

              /* fetch columns */
              {
                grn_id *key;
                if (otype == GRN_CONTENT_JSON) {
                  GRN_TEXT_PUTC(ctx, buf, '[');
                }
                grn_text_esc(ctx, buf, ":id", 3);
                GRN_TEXT_PUTC(ctx, buf, column_delimiter);
                grn_text_esc(ctx, buf, ":key", 4);
                GRN_TEXT_PUTC(ctx, buf, column_delimiter);
                grn_text_esc(ctx, buf, ":value", 6);

                GRN_HASH_EACH(col_ids, id, &key, NULL, NULL, {
                  if ((*last_cols = grn_ctx_at(ctx, *key))) {
                    char name[GRN_TABLE_MAX_KEY_SIZE];
                    unsigned int name_len;
                    name_len = grn_obj_name(ctx, *last_cols, name,
                                            GRN_TABLE_MAX_KEY_SIZE);
                    GRN_TEXT_PUTC(ctx, buf, column_delimiter);
                    grn_text_esc(ctx, buf, name, name_len);
                    last_cols++;
                  } else {
                    /* TODO: handling error */
                  }
                });
                if (otype == GRN_CONTENT_JSON) {
                  GRN_TEXT_PUTC(ctx, buf, ']');
                }
              }

              /* sort records */
              {
                grn_table_sort_key keys;
                /* TODO: use grn_obj_column for sort :id/:key/column */
                if (offset < 0) {
                  keys.flags = GRN_TABLE_SORT_DESC;
                  offset = -offset; /* FIXME: INT_MIN */
                } else {
                  keys.flags = GRN_TABLE_SORT_ASC;
                }
                keys.key = sort_col;
                if (count == -1) {
                  grn_table_sort(ctx, table, limit, sort, &keys, 1);
                  limit = 0;
                } else {
                  limit = grn_table_sort(ctx, table, offset + count - 1, sort,
                                         &keys, 1) + 1;
                }
                /* NOTE: handling grn_table_sort return value */
              }

              /* show records */
              if (count) {
                grn_array_cursor *cur;
                if ((cur = grn_array_cursor_open(ctx, (grn_array *)sort, 0, 0,
                                                 (grn_id)offset,
                                                 (grn_id)limit, 0))) {
                  grn_id rec_count;
                  while ((rec_count = grn_array_cursor_next(ctx, cur)) != GRN_ID_NIL) {
                    grn_obj **c;
                    grn_id *rec_id;

                    GRN_TEXT_PUTC(ctx, buf, line_delimiter);
                    if (otype == GRN_CONTENT_JSON) {
                      GRN_TEXT_PUTC(ctx, buf, '[');
                    }

                    grn_array_cursor_get_value(ctx, cur, (void **)&rec_id);

                    grn_text_itoa(ctx, buf, (int)*rec_id);
                    GRN_TEXT_PUTC(ctx, buf, column_delimiter);
                    {
                      int key_len;
                      char key[GRN_TABLE_MAX_KEY_SIZE];
                      key_len = grn_table_get_key(ctx, table, *rec_id, key, GRN_TABLE_MAX_KEY_SIZE);
                      grn_text_esc(ctx, buf, key, key_len);
                    }
                    GRN_TEXT_PUTC(ctx, buf, column_delimiter);
                    {
                      grn_obj value;
                      GRN_TEXT_INIT(&value, 0);
                      grn_obj_get_value(ctx, table, *rec_id, &value);
                      grn_text_esc(ctx, buf, GRN_TEXT_VALUE(&value), GRN_TEXT_LEN(&value));
                    }

                    for (c = cols; c < last_cols; c++) {
                      grn_obj value;
                      GRN_TEXT_INIT(&value, 0); /* FIXME: to use GRN_VOID_INIT */
                      grn_obj_get_value(ctx, *c, *rec_id, &value);
                      GRN_TEXT_PUTC(ctx, buf, column_delimiter);
                      print_columnvalue(ctx, &value, buf, otype);
                      GRN_OBJ_FIN(ctx, &value);
                    }
                    if (otype == GRN_CONTENT_JSON) {
                      GRN_TEXT_PUTC(ctx, buf, ']');
                    }
                  }
                  grn_array_cursor_close(ctx, cur);
                }
              }
              if (otype == GRN_CONTENT_JSON) {
                GRN_TEXT_PUTC(ctx, buf, ']');
              }
              grn_obj_close(ctx, sort);
            }
            grn_obj_unlink(ctx, sort_col);
          }
          free(cols);
        }
      }
      grn_hash_close(ctx, col_ids);
    }
  }
}

static void
cmd_columnlist(grn_ctx *ctx, char *table_name, unsigned table_name_len, grn_obj *buf, grn_content_type otype)
{
  grn_obj *table;
  if ((table = grn_ctx_get(ctx, table_name, table_name_len))) {
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

/* FIXME: db with grn_ctx_db() */
static void
cmd_tablelist(grn_ctx *ctx, grn_obj *db, grn_obj *buf, grn_content_type otype)
{
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, 0, 0))) {
    grn_id id;
    char line_delimiter, column_delimiter;

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

static int
do_alone(int argc, char **argv)
{
  int rc = -1;
  char *path = NULL;
  grn_obj *db;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, (useql ? GRN_CTX_USE_QL : 0)|(batchmode ? GRN_CTX_BATCH_MODE : 0));
  grn_timeval_now(ctx, &starttime);
  if (argc > 0 && argv) { path = *argv++; argc--; }
  db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
  if (db) {
    grn_ql_recv_handler_set(ctx, grn_ctx_stream_out_func, stdout);
    if (!argc) {
      char *buf = GRN_MALLOC(BUFSIZE);
      if (buf) {
        grn_ql_load(ctx, NULL);
        while ((prompt(), fgets(buf, BUFSIZE, stdin))) {
          uint32_t size = strlen(buf) - 1;
          buf[size] = '\0';
          grn_ql_send(ctx, buf, size, 0);
          if (ctx->stat == GRN_QL_QUIT) { break; }
        }
        GRN_FREE(buf);
        rc = 0;
      } else {
        fprintf(stderr, "grn_malloc failed (%d)\n", BUFSIZE);
      }
    } else {
      grn_ql_sendv(ctx, argc, argv, 0);
    }
    grn_db_close(ctx, db);
  } else {
    fprintf(stderr, "db open failed (%s)\n", path);
  }
  grn_ctx_fin(ctx);
  return rc;
}

#define BATCHMODE(ctx) do {\
  int flags;\
  unsigned int str_len;\
  char *str, *query = "(batchmode #t)";\
  grn_ql_send(ctx, query, strlen(query), 0);\
  do {\
    if (grn_ql_recv(ctx, &str, &str_len, &flags)) {\
      fprintf(stderr, "grn_ql_recv failed\n");\
    }\
  } while ((flags & GRN_QL_MORE));\
} while (0)

static int
do_client(char *hostname)
{
  int rc = -1;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, (batchmode ? GRN_CTX_BATCH_MODE : 0));
  grn_timeval_now(ctx, &starttime);
  if (!grn_ql_connect(ctx, hostname, port, 0)) {
    char *buf = GRN_MALLOC(BUFSIZE);
    if (buf) {
      if (batchmode) { BATCHMODE(ctx); }
      while ((prompt(), fgets(buf, BUFSIZE, stdin))) {
        int flags;
        char *str;
        unsigned int str_len;
        uint32_t size = strlen(buf) - 1;
        if (grn_ql_send(ctx, buf, size, 0)) { break; }
        do {
          if (grn_ql_recv(ctx, &str, &str_len, &flags)) {
            fprintf(stderr, "grn_ql_recv failed\n");
            goto exit;
          }
          if (str_len) {
            fwrite(str, 1, str_len, stdout);
            putchar('\n');
            fflush(stdout);
          }
        } while ((flags & GRN_QL_MORE));
        if (ctx->stat == GRN_QL_QUIT) { break; }
      }
      GRN_FREE(buf);
      rc = 0;
    } else {
      fprintf(stderr, "grn_malloc failed (%d)\n", BUFSIZE);
    }
  } else {
    fprintf(stderr, "grn_ql_connect failed (%s:%d)\n", hostname, port);
  }
  grn_ctx_fin(ctx);
exit :
  return rc;
}

/* server */

typedef struct {
  grn_com_queue_entry eq;
  grn_ctx ctx;
  grn_com_queue recv_new;
  grn_com_queue send_old;
  grn_com *com;
  grn_com_addr *addr;
  grn_msg *msg;
  uint8_t stat;
  grn_id id;
} grn_edge;

static void
do_htreq(grn_ctx *ctx, grn_edge *edge)
{
  grn_msg *msg = edge->msg;
  grn_com_header *header = &msg->header;
  switch (header->qtype) {
  case 'G' : /* GET */
    {
      char *path = NULL;
      char *p = GRN_BULK_HEAD((grn_obj *)msg);
      char *e = GRN_BULK_CURR((grn_obj *)msg);
      for (;; p++) {
        if (e <= p + 6) {
          /* invalid request */
          goto exit;
        }
        if (*p == ' ') {
          if (!path) {
            path = p + 1;
          } else {
            if (!memcmp(p + 1, "HTTP/1", 6)) {
              break;
            }
          }
        }
      }
      if (path[0] == '/' && path[2] == '/') {
        switch (path[1]) {
        case 'a' :
          {
            grn_hash *query;
            grn_obj *re = grn_msg_open_for_reply(ctx, (grn_obj *)msg, &edge->send_old);
            ((grn_msg *)re)->header.qtype = header->qtype;
            *p = '\0';
            GRN_TEXT_PUTS(ctx, re, "HTTP/1.1 200 OK\r\n");
            GRN_TEXT_PUTS(ctx, re, "Connection: close\r\n");
            GRN_TEXT_PUTS(ctx, re, "Content-Type: text/javascript\r\n\r\n");
            if ((query = parse_http_path(ctx, path, p - path))) {
              path += 3;
              switch (*path) {
              case 't':
                /* tablelist */
                cmd_tablelist(ctx, grn_ctx_db(ctx), re, GRN_CONTENT_JSON);
                break;
              case 'c':
                /* columnlist */
                /* createtable */
                /* createcolumn */
                if (p - path > 10) {
                  switch (*(path + 6)) {
                  case 'l':
                    {
                      grn_obj *table;
                      if (grn_hash_get(ctx, query,
                                       "table", 5, (void **)&table)
                            != GRN_ID_NIL) {
                        cmd_columnlist(ctx,
                                       GRN_TEXT_VALUE(table),
                                       GRN_TEXT_LEN(table),
                                       re, GRN_CONTENT_JSON);
                      }
                    }
                    break;
                  case 't':
                    {
                      grn_obj *name, *flags_str, *key_type, *value_type;
                      if (grn_hash_get(ctx, query,
                                       "name", 4, (void **)&name)
                            != GRN_ID_NIL &&
                          grn_hash_get(ctx, query,
                                       "flags", 5, (void **)&flags_str)
                            != GRN_ID_NIL &&
                          grn_hash_get(ctx, query,
                                       "key_type", 8, (void **)&key_type)
                            != GRN_ID_NIL &&
                          grn_hash_get(ctx, query,
                                       "value_type", 10, (void **)&value_type)
                            != GRN_ID_NIL) {
                        int flags;
                        flags = grn_atoi(
                          GRN_TEXT_VALUE(flags_str),
                          GRN_TEXT_VALUE(flags_str) +
                            GRN_TEXT_LEN(flags_str),
                          NULL);
                        cmd_createtable(
                          ctx,
                          GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name),
                          flags,
                          GRN_TEXT_VALUE(key_type), GRN_TEXT_LEN(key_type),
                          GRN_TEXT_VALUE(value_type), GRN_TEXT_LEN(value_type),
                          re, GRN_CONTENT_JSON);
                      }
                    }
                  case 'c':
                    {
                      grn_obj *table, *name, *flags_str, *type;
                      if (grn_hash_get(ctx, query,
                                       "table", 5, (void **)&table)
                            != GRN_ID_NIL &&
                          grn_hash_get(ctx, query,
                                       "name", 4, (void **)&name)
                            != GRN_ID_NIL &&
                          grn_hash_get(ctx, query,
                                       "flags", 5, (void **)&flags_str)
                            != GRN_ID_NIL &&
                          grn_hash_get(ctx, query,
                                       "type", 4, (void **)&type)
                            != GRN_ID_NIL) {
                        int flags;
                        flags = grn_atoi(
                          GRN_TEXT_VALUE(flags_str),
                          GRN_TEXT_VALUE(flags_str) +
                            GRN_TEXT_LEN(flags_str),
                          NULL);
                        cmd_createcolumn(
                          ctx,
                          GRN_TEXT_VALUE(table), GRN_TEXT_LEN(table),
                          GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name),
                          flags,
                          GRN_TEXT_VALUE(type), GRN_TEXT_LEN(type),
                          re, GRN_CONTENT_JSON);
                      }
                    }
                    break;
                  }
                }
                break;
              case 'r':
                /* recordlist */
                {
                  grn_obj *table;
                  if (grn_hash_get(ctx, query,
                                   "table", 5,
                                   (void **)&table) != GRN_ID_NIL) {
                    int offset, count;
                    grn_obj *sort_column, *num_str;
                    if (grn_hash_get(ctx, query,
                                     "offset", 6,
                                     (void **)&num_str) != GRN_ID_NIL) {
                      offset = grn_atoi(
                        GRN_TEXT_VALUE(num_str),
                        GRN_TEXT_VALUE(num_str) + GRN_TEXT_LEN(num_str),
                        NULL);
                    } else {
                      offset = 1;
                    }

                    if (grn_hash_get(ctx, query,
                                     "count", 5,
                                     (void **)&num_str) != GRN_ID_NIL) {
                      count = grn_atoi(
                        GRN_TEXT_VALUE(num_str),
                        GRN_TEXT_VALUE(num_str) + GRN_TEXT_LEN(num_str),
                        NULL);
                    } else {
                      count = -1;
                    }

                    if (grn_hash_get(ctx, query,
                                     "sort_column", 11,
                                     (void **)&sort_column) != GRN_ID_NIL) {
                      cmd_recordlist(ctx,
                        GRN_TEXT_VALUE(table), GRN_TEXT_LEN(table),
                        GRN_TEXT_VALUE(sort_column), GRN_TEXT_LEN(sort_column),
                        offset, count, re, GRN_CONTENT_JSON);
                    } else {
                      cmd_recordlist(ctx,
                        GRN_TEXT_VALUE(table), GRN_TEXT_LEN(table),
                        ".:key", 5,
                        offset, count, re, GRN_CONTENT_JSON);
                    }
                  }
                }
                break;
              case 's':
                /* status */
                cmd_status(ctx, re, GRN_CONTENT_JSON);
                break;
              }
              release_query(ctx, query);
            }
            if (grn_msg_send(ctx, re, 0)) {
              /* TODO: error handling */
            }
          }
          break;
        case 's' :
          {
            char *q;
            grn_obj *re = grn_msg_open_for_reply(ctx, (grn_obj *)msg, &edge->send_old);
            ((grn_msg *)re)->header.qtype = header->qtype;
            *p = '\0';
            GRN_TEXT_PUTS(ctx, re, "HTTP/1.1 200 OK\r\n");
            GRN_TEXT_PUTS(ctx, re, "Connection: close\r\n");
            /* static file */
            /* FIXME: remove '..' for security ! */
            /* FIXME: follow symbolic link ? */
            for (q = p;; q--) {
              if (q <= path) {
                GRN_TEXT_PUTS(ctx, re, "Content-Type: text/plain\r\n\r\n");
                break;
              }
              if (*q == '.') {
                if (q + 5 == p && !memcmp(q, ".html", 5)) {
                  GRN_TEXT_PUTS(ctx, re, "Content-Type: text/html\r\n\r\n");
                  break;
                } else if (q + 4 == p && !memcmp(q, ".png", 4)) {
                  GRN_TEXT_PUTS(ctx, re, "Content-Type: image/png\r\n\r\n");
                  break;
                } else if (q + 4 == p && !memcmp(q, ".css", 4)) {
                  GRN_TEXT_PUTS(ctx, re, "Content-Type: text/css\r\n\r\n");
                  break;
                } else if (q + 3 == p && !memcmp(q, ".js", 3)) {
                  GRN_TEXT_PUTS(ctx, re, "Content-Type: text/javascript\r\n\r\n");
                  break;
                }
              }
            }
            grn_bulk_from_file(ctx, (grn_obj *)re, path + 1);
            if (grn_msg_send(ctx, re, 0)) {
              /* TODO: error handling */
            }
          }
          break;
        case 'q' :
          {
            grn_obj *head = ctx->impl->outbuf;
            GRN_TEXT_INIT(head, 0);
            GRN_TEXT_PUTS(ctx, head, "HTTP/1.1 200 OK\r\n");
            GRN_TEXT_PUTS(ctx, head, "Connection: close\r\n");
            /* todo : support tsv
            GRN_TEXT_PUTS(ctx, head, "Content-Type: text/plain\r\n\r\n");
            */
            GRN_TEXT_PUTS(ctx, head, "Content-Type: text/javascript\r\n\r\n");
            path += 2;
            grn_ql_send(ctx, path, p - path, header->flags);
          }
          break;
        }
      }
    }
    break;
  }
exit :
  // todo : support "Connection: keep-alive"
  ctx->stat = GRN_QL_QUIT;
}

enum {
  MBRES_SUCCESS = 0x00,
  MBRES_KEY_ENOENT = 0x01,
  MBRES_KEY_EEXISTS = 0x02,
  MBRES_E2BIG = 0x03,
  MBRES_EINVAL = 0x04,
  MBRES_NOT_STORED = 0x05,
  MBRES_UNKNOWN_COMMAND = 0x81,
  MBRES_ENOMEM = 0x82,
};

enum {
  MBCMD_GET = 0x00,
  MBCMD_SET = 0x01,
  MBCMD_ADD = 0x02,
  MBCMD_REPLACE = 0x03,
  MBCMD_DELETE = 0x04,
  MBCMD_INCREMENT = 0x05,
  MBCMD_DECREMENT = 0x06,
  MBCMD_QUIT = 0x07,
  MBCMD_FLUSH = 0x08,
  MBCMD_GETQ = 0x09,
  MBCMD_NOOP = 0x0a,
  MBCMD_VERSION = 0x0b,
  MBCMD_GETK = 0x0c,
  MBCMD_GETKQ = 0x0d,
  MBCMD_APPEND = 0x0e,
  MBCMD_PREPEND = 0x0f,
  MBCMD_STAT = 0x10,
  MBCMD_SETQ = 0x11,
  MBCMD_ADDQ = 0x12,
  MBCMD_REPLACEQ = 0x13,
  MBCMD_DELETEQ = 0x14,
  MBCMD_INCREMENTQ = 0x15,
  MBCMD_DECREMENTQ = 0x16,
  MBCMD_QUITQ = 0x17,
  MBCMD_FLUSHQ = 0x18,
  MBCMD_APPENDQ = 0x19,
  MBCMD_PREPENDQ = 0x1a
};

static grn_mutex cache_mutex;
static grn_obj *cache_table = NULL;
static grn_obj *cache_value = NULL;
static grn_obj *cache_flags = NULL;
static grn_obj *cache_expire = NULL;
static grn_obj *cache_cas = NULL;

#define CTX_GET(name) (grn_ctx_get(ctx, (name), strlen(name)))

static grn_obj *
cache_init(grn_ctx *ctx)
{
  if (cache_cas) { return cache_cas; }
  MUTEX_LOCK(cache_mutex);
  if (!cache_cas) {
    if ((cache_table = CTX_GET("<cache>"))) {
      cache_value = CTX_GET("<cache>.value");
      cache_flags = CTX_GET("<cache>.flags");
      cache_expire = CTX_GET("<cache>.expire");
      cache_cas = CTX_GET("<cache>.cas");
    } else {
      if (!cache_table) {
        grn_obj *uint_type = grn_ctx_at(ctx, GRN_DB_UINT32);
        grn_obj *int64_type = grn_ctx_at(ctx, GRN_DB_INT64);
        grn_obj *shorttext_type = grn_ctx_at(ctx, GRN_DB_SHORT_TEXT);
        if ((cache_table = grn_table_create(ctx, "<cache>", 7, NULL,
                                            GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                                            shorttext_type, NULL))) {
          cache_value = grn_column_create(ctx, cache_table, "value", 5, NULL,
                                          GRN_OBJ_PERSISTENT, shorttext_type);
          cache_flags = grn_column_create(ctx, cache_table, "flags", 5, NULL,
                                          GRN_OBJ_PERSISTENT, uint_type);
          cache_expire = grn_column_create(ctx, cache_table, "expire", 6, NULL,
                                           GRN_OBJ_PERSISTENT, uint_type);
          cache_cas = grn_column_create(ctx, cache_table, "cas", 3, NULL,
                                        GRN_OBJ_PERSISTENT, int64_type);
        }
      }
    }
  }
  MUTEX_UNLOCK(cache_mutex);
  return cache_cas;
}

#define RELATIVE_TIME_THRESH 1000000000

#define MBRES(ctx,re,status,key_len,extra_len,flags) {\
  grn_msg_set_property((ctx), (re), (status), (key_len), (extra_len));\
  grn_msg_send((ctx), (re), (flags));\
}

#define GRN_MSG_MBRES(block) \
do {\
  if (!quiet) {\
    grn_obj *re = grn_msg_open_for_reply(ctx, (grn_obj *)msg, &edge->send_old);\
    ((grn_msg *)re)->header.qtype = header->qtype;\
    block\
  }\
} while (0)

static uint64_t
get_mbreq_cas_id()
{
  /* FIXME: I think this logic have bugs.
            one is a race condition.
            another is a cyclic increment (cas_id must be non-zero).
            But memcached-1.2.8 do this... */
  static uint64_t cas_id = 0;
  return ++cas_id;
}

static void
do_mbreq(grn_ctx *ctx, grn_edge *edge)
{
  int quiet = 0;
  int flags = 0;
  grn_msg *msg = edge->msg;
  grn_com_header *header = &msg->header;

  switch (header->qtype) {
  case MBCMD_GETQ :
    flags = GRN_QL_MORE;
    /* fallthru */
  case MBCMD_GET :
    {
      grn_id rid;
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      cache_init(ctx);
      rid = grn_table_get(ctx, cache_table, key, keylen);
      if (!rid) {
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
        });
      } else {
        grn_obj buf;
        grn_timeval tv;
        uint32_t expire;
        GRN_TEXT_INIT(&buf, 0);
        grn_obj_get_value(ctx, cache_expire, rid, &buf);
        expire = *((uint32_t *)GRN_BULK_HEAD(&buf));
        grn_timeval_now(ctx, &tv);
        if (expire && expire < tv.tv_sec) {
          grn_table_delete_by_id(ctx, cache_table, rid);
          GRN_MSG_MBRES({
            MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
          });
        } else {
          GRN_BULK_REWIND(&buf);
          grn_obj_get_value(ctx, cache_cas, rid, &buf);
          GRN_MSG_MBRES({
            grn_obj_get_value(ctx, cache_flags, rid, re);
            grn_obj_get_value(ctx, cache_value, rid, re);
            ((grn_msg *)re)->header.cas = *((uint64_t *)GRN_BULK_HEAD(&buf));
            MBRES(ctx, re, MBRES_SUCCESS, 0, 4, flags);
          });
        }
        grn_obj_close(ctx, &buf);
      }
    }
    break;
  case MBCMD_SETQ :
  case MBCMD_ADDQ :
  case MBCMD_REPLACEQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_SET :
  case MBCMD_ADD :
  case MBCMD_REPLACE :
    {
      grn_id rid;
      uint32_t size = ntohl(header->size);
      uint16_t keylen = ntohs(header->keylen);
      uint8_t extralen = header->level;
      char *body = GRN_BULK_HEAD((grn_obj *)msg);
      uint32_t flags = *((uint32_t *)body);
      uint32_t expire = ntohl(*((uint32_t *)(body + 4)));
      uint32_t valuelen = size - keylen - extralen;
      char *key = body + 8;
      char *value = key + keylen;
      int added = 0;
      int f = (header->qtype == MBCMD_REPLACE ||
               header->qtype == MBCMD_REPLACEQ) ? 0 : GRN_TABLE_ADD;
      GRN_ASSERT(extralen == 8);
      cache_init(ctx);
      if (header->qtype == MBCMD_REPLACE || header->qtype == MBCMD_REPLACEQ) {
        rid = grn_table_get(ctx, cache_table, key, keylen);
      } else {
        rid = grn_table_add(ctx, cache_table, key, keylen, &added);
      }
      if (!rid) {
        GRN_MSG_MBRES({
          MBRES(ctx, re, (f & GRN_TABLE_ADD) ? MBRES_ENOMEM : MBRES_NOT_STORED, 0, 0, 0);
        });
      } else {
        if (added) {
          grn_obj buf;
          GRN_TEXT_INIT(&buf, GRN_OBJ_DO_SHALLOW_COPY);
          GRN_TEXT_SET_REF(&buf, value, valuelen);
          grn_obj_set_value(ctx, cache_value, rid, &buf, GRN_OBJ_SET);
          GRN_TEXT_SET_REF(&buf, &flags, 4);
          grn_obj_set_value(ctx, cache_flags, rid, &buf, GRN_OBJ_SET);
          if (expire && expire < RELATIVE_TIME_THRESH) {
            grn_timeval tv;
            grn_timeval_now(ctx, &tv);
            expire += tv.tv_sec;
          }
          GRN_TEXT_SET_REF(&buf, &expire, 4);
          grn_obj_set_value(ctx, cache_expire, rid, &buf, GRN_OBJ_SET);
          {
            uint64_t cas_id = get_mbreq_cas_id();
            GRN_TEXT_SET_REF(&buf, &cas_id, sizeof(uint64_t));
            grn_obj_set_value(ctx, cache_cas, rid, &buf, GRN_OBJ_SET);
            GRN_MSG_MBRES({
              ((grn_msg *)re)->header.cas = cas_id;
              MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
            });
          }
        } else {
          if (header->qtype != MBCMD_SET && header->qtype != MBCMD_SETQ) {
            grn_obj buf;
            grn_timeval tv;
            uint32_t oexpire;

            GRN_TEXT_INIT(&buf, 0);
            grn_obj_get_value(ctx, cache_expire, rid, &buf);
            oexpire = *((uint32_t *)GRN_BULK_HEAD(&buf));
            grn_timeval_now(ctx, &tv);

            if (oexpire && oexpire < tv.tv_sec) {
              if (header->qtype == MBCMD_REPLACE ||
                  header->qtype == MBCMD_REPLACEQ) {
                grn_table_delete_by_id(ctx, cache_table, rid);
                GRN_MSG_MBRES({
                  MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0);
                });
                break;
              }
            } else if (header->qtype == MBCMD_ADD ||
                       header->qtype == MBCMD_ADDQ) {
              GRN_MSG_MBRES({
                MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0);
              });
              break;
            }
            grn_obj_close(ctx, &buf);
          }
          {
            grn_obj cas;
            GRN_TEXT_INIT(&cas, 0);
            grn_obj_get_value(ctx, cache_cas, rid, &cas);
            if (header->cas && header->cas !=
                *((uint64_t *)GRN_BULK_HEAD(&cas))) {
              GRN_MSG_MBRES({
                MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0);
              });
            } else {
              grn_obj buf;
              GRN_TEXT_INIT(&buf, GRN_OBJ_DO_SHALLOW_COPY);
              GRN_TEXT_SET_REF(&buf, value, valuelen);
              grn_obj_set_value(ctx, cache_value, rid, &buf, GRN_OBJ_SET);
              GRN_TEXT_SET_REF(&buf, &flags, 4);
              grn_obj_set_value(ctx, cache_flags, rid, &buf, GRN_OBJ_SET);
              if (expire && expire < RELATIVE_TIME_THRESH) {
                grn_timeval tv;
                grn_timeval_now(ctx, &tv);
                expire += tv.tv_sec;
              }
              GRN_TEXT_SET_REF(&buf, &expire, 4);
              grn_obj_set_value(ctx, cache_expire, rid, &buf, GRN_OBJ_SET);
              {
                uint64_t cas_id = get_mbreq_cas_id();
                GRN_TEXT_SET_REF(&buf, &cas_id, sizeof(uint64_t));
                grn_obj_set_value(ctx, cache_cas, rid, &buf, GRN_OBJ_SET);
                GRN_MSG_MBRES({
                  ((grn_msg *)re)->header.cas = cas_id;
                  MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
                });
              }
            }
            grn_obj_close(ctx, &cas);
          }
        }
      }
    }
    break;
  case MBCMD_DELETEQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_DELETE :
    {
      grn_id rid;
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      cache_init(ctx);
      rid = grn_table_get(ctx, cache_table, key, keylen);
      if (!rid) {
        // GRN_LOG(ctx, GRN_LOG_NOTICE, "GET k=%d not found", keylen);
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
        });
      } else {
        grn_table_delete_by_id(ctx, cache_table, rid);
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_SUCCESS, 0, 4, 0);
        });
      }
    }
    break;
  case MBCMD_INCREMENTQ :
  case MBCMD_DECREMENTQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_INCREMENT :
  case MBCMD_DECREMENT :
    {
      grn_id rid;
      int added = 0;
      uint64_t delta, init;
      uint16_t keylen = ntohs(header->keylen);
      char *body = GRN_BULK_HEAD((grn_obj *)msg);
      char *key = body + 20;
      uint32_t expire = ntohl(*((uint32_t *)(body + 16)));
      grn_ntoh(&delta, body, 8);
      grn_ntoh(&init, body + 8, 8);
      GRN_ASSERT(header->level == 20); /* extralen */
      cache_init(ctx);
      if (expire == 0xffffffff) {
        rid = grn_table_get(ctx, cache_table, key, keylen);
      } else {
        rid = grn_table_add(ctx, cache_table, key, keylen, &added);
      }
      if (!rid) {
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
        });
      } else {
        grn_obj buf;
        GRN_TEXT_INIT(&buf, GRN_OBJ_DO_SHALLOW_COPY);
        if (added) {
          uint32_t flags = 0;
          GRN_TEXT_SET_REF(&buf, &init, 8);
          grn_obj_set_value(ctx, cache_value, rid, &buf, GRN_OBJ_SET);
          GRN_TEXT_SET_REF(&buf, &flags, 4);
          grn_obj_set_value(ctx, cache_flags, rid, &buf, GRN_OBJ_SET);
        } else {
          grn_timeval tv;
          uint32_t oexpire;

          grn_obj_get_value(ctx, cache_expire, rid, &buf);
          oexpire = *((uint32_t *)GRN_BULK_HEAD(&buf));
          grn_timeval_now(ctx, &tv);

          if (oexpire && oexpire < tv.tv_sec) {
            if (expire == 0xffffffffU) {
              GRN_MSG_MBRES({
                MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
              });
              grn_obj_close(ctx, &buf);
              break;
            } else {
              uint32_t flags = 0;
              GRN_TEXT_SET_REF(&buf, &init, 8);
              grn_obj_set_value(ctx, cache_value, rid, &buf, GRN_OBJ_SET);
              GRN_TEXT_SET_REF(&buf, &flags, 4);
              grn_obj_set_value(ctx, cache_flags, rid, &buf, GRN_OBJ_SET);
            }
          } else {
            GRN_TEXT_SET_REF(&buf, &delta, 8);
            grn_obj_set_value(ctx, cache_value, rid, &buf,
                              header->qtype == MBCMD_INCREMENT ||
                              header->qtype == MBCMD_INCREMENTQ
                              ? GRN_OBJ_INCR
                              : GRN_OBJ_DECR);
          }
        }
        if (expire && expire < RELATIVE_TIME_THRESH) {
          grn_timeval tv;
          grn_timeval_now(ctx, &tv);
          expire += tv.tv_sec;
        }
        GRN_TEXT_SET_REF(&buf, &expire, 4);
        grn_obj_set_value(ctx, cache_expire, rid, &buf, GRN_OBJ_SET);
        GRN_MSG_MBRES({
          /* TODO: get_mbreq_cas_id() */
          grn_obj_get_value(ctx, cache_value, rid, re);
          grn_hton(&delta, (uint64_t *)GRN_BULK_HEAD(re), 8);
          GRN_TEXT_SET(ctx, re, &delta, sizeof(uint64_t));
          MBRES(ctx, re, MBRES_SUCCESS, 0, sizeof(uint64_t), 0);
        });
        grn_obj_close(ctx, &buf);
      }
    }
    break;
  case MBCMD_FLUSHQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_FLUSH :
    {
      grn_obj buf;
      uint32_t expire;
      uint8_t extralen = header->level;
      GRN_TEXT_INIT(&buf, GRN_OBJ_DO_SHALLOW_COPY);
      if (extralen) {
        char *body = GRN_BULK_HEAD((grn_obj *)msg);
        GRN_ASSERT(extralen == 4);
        expire = ntohl(*((uint32_t *)(body)));
        if (expire < RELATIVE_TIME_THRESH) {
          grn_timeval tv;
          grn_timeval_now(ctx, &tv);
          if (expire) {
            expire += tv.tv_sec;
          } else {
            expire = tv.tv_sec - 1;
          }
        }
      } else {
        grn_timeval tv;
        grn_timeval_now(ctx, &tv);
        expire = tv.tv_sec - 1;
      }
      grn_obj_close(ctx, &buf);
      GRN_TEXT_SET_REF(&buf, &expire, 4);
      GRN_TABLE_EACH(ctx, cache_table, 0, 0, rid, NULL, NULL, NULL, {
        grn_obj_set_value(ctx, cache_expire, rid, &buf, GRN_OBJ_SET);
      });
      GRN_MSG_MBRES({
        MBRES(ctx, re, MBRES_SUCCESS, 0, 4, 0);
      });
    }
    break;
  case MBCMD_NOOP :
    GRN_MSG_MBRES({
      MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
    });
    break;
  case MBCMD_VERSION :
    GRN_MSG_MBRES({
      grn_bulk_write(ctx, re, PACKAGE_VERSION, strlen(PACKAGE_VERSION));
      MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
    });
    break;
  case MBCMD_GETKQ :
    flags = GRN_QL_MORE;
    /* fallthru */
  case MBCMD_GETK :
    {
      grn_id rid;
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      cache_init(ctx);
      rid = grn_table_get(ctx, cache_table, key, keylen);
      if (!rid) {
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
        });
      } else {
        grn_obj buf;
        grn_timeval tv;
        uint32_t expire;
        GRN_TEXT_INIT(&buf, 0);
        grn_obj_get_value(ctx, cache_expire, rid, &buf);
        expire = *((uint32_t *)GRN_BULK_HEAD(&buf));
        grn_timeval_now(ctx, &tv);
        if (expire && expire < tv.tv_sec) {
          grn_table_delete_by_id(ctx, cache_table, rid);
          GRN_MSG_MBRES({
            MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
          });
        } else {
          GRN_MSG_MBRES({
            grn_obj_get_value(ctx, cache_flags, rid, re);
            grn_bulk_write(ctx, re, key, keylen);
            grn_obj_get_value(ctx, cache_value, rid, re);
            GRN_BULK_REWIND(&buf);
            grn_obj_get_value(ctx, cache_cas, rid, &buf);
            ((grn_msg *)re)->header.cas = *((uint64_t *)GRN_BULK_HEAD(&buf));
            MBRES(ctx, re, MBRES_SUCCESS, keylen, 4, flags);
          });
        }
        grn_obj_close(ctx, &buf);
      }
    }
    break;
  case MBCMD_APPENDQ :
  case MBCMD_PREPENDQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_APPEND :
  case MBCMD_PREPEND :
    {
      grn_id rid;
      uint32_t size = ntohl(header->size);
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      char *value = key + keylen;
      uint32_t valuelen = size - keylen;
      cache_init(ctx);
      rid = grn_table_add(ctx, cache_table, key, keylen, NULL);
      if (!rid) {
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_ENOMEM, 0, 0, 0);
        });
      } else {
        /* FIXME: check expire */
        grn_obj buf;
        int flags = header->qtype == MBCMD_APPEND ? GRN_OBJ_APPEND : GRN_OBJ_PREPEND;
        GRN_TEXT_INIT(&buf, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET_REF(&buf, value, valuelen);
        grn_obj_set_value(ctx, cache_value, rid, &buf, flags);
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
        });
      }
    }
    break;
  case MBCMD_STAT :
    {
      pid_t pid = getpid();
      GRN_MSG_MBRES({
        grn_bulk_write(ctx, re, "pid", 3);
        grn_text_itoa(ctx, re, pid);
        MBRES(ctx, re, MBRES_SUCCESS, 3, 0, 0);
      });
    }
    break;
  case MBCMD_QUITQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_QUIT :
    GRN_MSG_MBRES({
      MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
    });
    /* fallthru */
  default :
    ctx->stat = GRN_QL_QUIT;
    break;
  }
}

/* worker thread */

enum {
  EDGE_IDLE = 0x00,
  EDGE_WAIT = 0x01,
  EDGE_DOING = 0x02,
  EDGE_ABORT = 0x03,
};

static grn_hash *edges;
static grn_com_queue ctx_new;
static grn_com_queue ctx_old;
static grn_mutex q_mutex;
static grn_cond q_cond;
static uint32_t nthreads = 0, nfthreads = 0,
                max_nfthreads = DEFAULT_MAX_NFTHREADS;

static void * CALLBACK
worker(void *arg)
{
  GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "thread start (%d/%d)", nfthreads, nthreads + 1);
  MUTEX_LOCK(q_mutex);
  do {
    grn_ctx *ctx;
    grn_edge *edge;
    nfthreads++;
    while (!(edge = (grn_edge *)grn_com_queue_deque(&grn_gctx, &ctx_new))) {
      COND_WAIT(q_cond, q_mutex);
      if (grn_gctx.stat == GRN_QL_QUIT) { goto exit; }
    }
    ctx = &edge->ctx;
    nfthreads--;
    if (edge->stat == EDGE_DOING) { continue; }
    if (edge->stat == EDGE_WAIT) {
      edge->stat = EDGE_DOING;
      while (!GRN_COM_QUEUE_EMPTYP(&edge->recv_new)) {
        grn_obj *msg;
        MUTEX_UNLOCK(q_mutex);
        while (ctx->stat != GRN_QL_QUIT &&
               (edge->msg = (grn_msg *)grn_com_queue_deque(ctx, &edge->recv_new))) {
          grn_com_header *header = &edge->msg->header;
          msg = (grn_obj *)edge->msg;
          switch (header->proto) {
          case GRN_COM_PROTO_HTTP :
            do_htreq(ctx, edge);
            break;
          case GRN_COM_PROTO_MBREQ :
            do_mbreq(ctx, edge);
            break;
          case GRN_COM_PROTO_GQTP :
            grn_ql_send(ctx, GRN_BULK_HEAD(msg), GRN_BULK_VSIZE(msg), header->flags);
            ERRCLR(ctx);
            break;
          default :
            ctx->stat = GRN_QL_QUIT;
            break;
          }
          grn_msg_close(ctx, msg);
        }
        while ((msg = (grn_obj *)grn_com_queue_deque(ctx, &edge->send_old))) {
          grn_msg_close(ctx, msg);
        }
        MUTEX_LOCK(q_mutex);
        if (ctx->stat == GRN_QL_QUIT || edge->stat == EDGE_ABORT) { break; }
      }
    }
    if (ctx->stat == GRN_QL_QUIT || edge->stat == EDGE_ABORT) {
      if (edge->com->has_sid) { grn_com_close_(ctx, edge->com); }
      grn_com_queue_enque(&grn_gctx, &ctx_old, (grn_com_queue_entry *)edge);
      edge->stat = EDGE_ABORT;
    } else {
      edge->stat = EDGE_IDLE;
    }
  } while (nfthreads < max_nfthreads && grn_gctx.stat != GRN_QL_QUIT);
exit :
  nthreads--;
  MUTEX_UNLOCK(q_mutex);
  GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "thread end (%d/%d)", nfthreads, nthreads);
  return NULL;
}

static void
output(grn_ctx *ctx, int flags, void *arg)
{
  grn_edge *edge = arg;
  grn_com *com = edge->com;
  grn_msg *req = edge->msg, *msg = (grn_msg *)ctx->impl->outbuf;
  msg->edge_id = req->edge_id;
  msg->header.proto = req->header.proto == GRN_COM_PROTO_MBREQ
    ? GRN_COM_PROTO_MBRES : req->header.proto;
  ERRCLR(ctx);
  if (grn_msg_send(ctx, (grn_obj *)msg,
                   (flags & GRN_QL_MORE) ? GRN_QL_MORE : GRN_QL_TAIL)) {
    edge->stat = EDGE_ABORT;
  }
  ctx->impl->outbuf = grn_msg_open(ctx, com, &edge->send_old);
}

static void
msg_handler(grn_ctx *ctx, grn_obj *msg)
{
  grn_edge *edge;
  grn_com *com = ((grn_msg *)msg)->peer;
  if (ctx->rc) {
    if (com->has_sid) {
      if ((edge = com->opaque)) {
        MUTEX_LOCK(q_mutex);
        if (edge->stat == EDGE_IDLE) {
          grn_com_queue_enque(ctx, &ctx_old, (grn_com_queue_entry *)edge);
        }
        edge->stat = EDGE_ABORT;
        MUTEX_UNLOCK(q_mutex);
      } else {
        grn_com_close(ctx, com);
      }
    }
    grn_msg_close(ctx, msg);
  } else {
    int added;
    grn_id id = grn_hash_add(ctx, edges, &((grn_msg *)msg)->edge_id, sizeof(grn_com_addr),
                             (void **)&edge, &added);
    if (added) {
      grn_ctx_init(&edge->ctx, (useql ? GRN_CTX_USE_QL : 0));
      GRN_COM_QUEUE_INIT(&edge->recv_new);
      GRN_COM_QUEUE_INIT(&edge->send_old);
      grn_ctx_use(&edge->ctx, (grn_obj *)com->ev->opaque);
      grn_ql_load(&edge->ctx, NULL);
      grn_ql_recv_handler_set(&edge->ctx, output, edge);
      com->opaque = edge;
      grn_obj_close(&edge->ctx, edge->ctx.impl->outbuf);
      edge->ctx.impl->outbuf = grn_msg_open(&edge->ctx, com, &edge->send_old);
      edge->com = com;
      edge->id = id;
      edge->stat = EDGE_IDLE;
    }
    if (edge->ctx.stat == GRN_QL_QUIT || edge->stat == EDGE_ABORT) {
      grn_msg_close(ctx, msg);
    } else {
      grn_com_queue_enque(ctx, &edge->recv_new, (grn_com_queue_entry *)msg);
      MUTEX_LOCK(q_mutex);
      if (edge->stat == EDGE_IDLE) {
        grn_com_queue_enque(ctx, &ctx_new, (grn_com_queue_entry *)edge);
        edge->stat = EDGE_WAIT;
        if (!nfthreads && nthreads < max_nfthreads) {
          grn_thread thread;
          nthreads++;
          if (THREAD_CREATE(thread, worker, NULL)) { SERR("pthread_create"); }
        }
        COND_SIGNAL(q_cond);
      }
      MUTEX_UNLOCK(q_mutex);
    }
  }
}

#define MAX_CON 0x10000

static int
server(char *path)
{
  int rc = -1;
  grn_com_event ev;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  grn_timeval_now(ctx, &starttime);
  MUTEX_INIT(q_mutex);
  COND_INIT(q_cond);
  MUTEX_INIT(cache_mutex);
  GRN_COM_QUEUE_INIT(&ctx_new);
  GRN_COM_QUEUE_INIT(&ctx_old);
#ifndef WIN32
  {
    struct rlimit lim;
    lim.rlim_cur = 4096;
    lim.rlim_max = 4096;
    // RLIMIT_OFILE
    setrlimit(RLIMIT_NOFILE, &lim);
    lim.rlim_cur = 0;
    lim.rlim_max = 0;
    getrlimit(RLIMIT_NOFILE, &lim);
    GRN_LOG(ctx, GRN_LOG_NOTICE, "RLIMIT_NOFILE(%d,%d)", lim.rlim_cur, lim.rlim_max);
  }
#endif /* WIN32 */
  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
    grn_obj *db;
    db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
    if (db) {
      struct hostent *he;
      if (!(he = gethostbyname(hostname))) {
        SERR("gethostbyname");
        return rc;
      }
      ev.opaque = db;
      edges = grn_hash_create(ctx, NULL, sizeof(grn_com_addr), sizeof(grn_edge), 0);
      if (!grn_com_sopen(ctx, &ev, port, msg_handler, he)) {
        while (!grn_com_event_poll(ctx, &ev, 1000) && grn_gctx.stat != GRN_QL_QUIT) {
          grn_edge *edge;
          while ((edge = (grn_edge *)grn_com_queue_deque(ctx, &ctx_old))) {
            grn_obj *msg;
            while ((msg = (grn_obj *)grn_com_queue_deque(ctx, &edge->send_old))) {
              grn_msg_close(&edge->ctx, msg);
            }
            while ((msg = (grn_obj *)grn_com_queue_deque(ctx, &edge->recv_new))) {
              grn_msg_close(ctx, msg);
            }
            grn_ctx_fin(&edge->ctx);
            if (edge->com->has_sid && edge->com->opaque == edge) {
              grn_com_close(ctx, edge->com);
            }
            grn_hash_delete_by_id(ctx, edges, edge->id, NULL);
          }
        }
        for (;;) {
          MUTEX_LOCK(q_mutex);
          if (nthreads == nfthreads) { break; }
          MUTEX_UNLOCK(q_mutex);
          usleep(1000);
        }
        {
          grn_edge *edge;
          GRN_HASH_EACH(edges, id, NULL, NULL, &edge, {
              grn_obj *obj;
            while ((obj = (grn_obj *)grn_com_queue_deque(ctx, &edge->send_old))) {
              grn_msg_close(&edge->ctx, obj);
            }
            while ((obj = (grn_obj *)grn_com_queue_deque(ctx, &edge->recv_new))) {
              grn_msg_close(ctx, obj);
            }
            grn_ctx_fin(&edge->ctx);
            if (edge->com->has_sid) {
              grn_com_close(ctx, edge->com);
            }
            grn_hash_delete_by_id(ctx, edges, edge->id, NULL);
          });
        }
        {
          grn_com *com;
          GRN_HASH_EACH(ev.hash, id, NULL, NULL, &com, { grn_com_close(ctx, com); });
        }
        rc = 0;
      } else {
        fprintf(stderr, "grn_com_gqtp_sopen failed (%d)\n", port);
      }
      grn_hash_close(ctx, edges);
      grn_db_close(ctx, db);
    } else {
      fprintf(stderr, "db open failed (%s)\n", path);
    }
    grn_com_event_fin(ctx, &ev);
  } else {
    fprintf(stderr, "grn_com_event_init failed\n");
  }
  grn_ctx_fin(ctx);
  return rc;
}

static int
do_daemon(char *path)
{
#ifndef WIN32
  pid_t pid;
  switch (fork()) {
  case 0:
    break;
  case -1:
    perror("fork");
    return -1;
  default:
    wait(NULL);
    return 0;
  }
  switch ((pid = fork())) {
  case 0:
    break;
  case -1:
    perror("fork");
    return -1;
  default:
    fprintf(stderr, "%d\n", pid);
    _exit(0);
  }
#endif /* WIN32 */
  return server(path);
}

enum {
  mode_alone = 0,
  mode_client,
  mode_daemon,
  mode_server,
  mode_usage
};

#define MODE_MASK   0x007f
#define MODE_USE_QL 0x0080
#define MODE_NEW_DB 0x0100

#define SET_LOGLEVEL(x) do {\
  static grn_logger_info info;\
  info.max_level = (x);\
  info.flags = GRN_LOG_TIME|GRN_LOG_MESSAGE;\
  info.func = NULL;\
  info.func_arg = NULL;\
  grn_logger_info_set(&grn_gctx, &info);\
} while(0)

int
main(int argc, char **argv)
{
  grn_encoding enc = GRN_ENC_DEFAULT;
  char *portstr = NULL, *encstr = NULL,
       *max_nfthreadsstr = NULL, *loglevel = NULL,
       *hostnamestr = NULL;
  int r, i, mode = mode_alone;
  static grn_str_getopt_opt opts[] = {
    {'p', NULL, NULL, 0, getopt_op_none},
    {'e', NULL, NULL, 0, getopt_op_none},
    {'t', NULL, NULL, 0, getopt_op_none},
    {'h', "help", NULL, mode_usage, getopt_op_update},
    {'a', NULL, NULL, mode_alone, getopt_op_update},
    {'c', NULL, NULL, mode_client, getopt_op_update},
    {'d', NULL, NULL, mode_daemon, getopt_op_update},
    {'s', NULL, NULL, mode_server, getopt_op_update},
    {'l', NULL, NULL, 0, getopt_op_none},
    {'i', NULL, NULL, 0, getopt_op_none},
    {'q', NULL, NULL, MODE_USE_QL, getopt_op_on},
    {'n', NULL, NULL, MODE_NEW_DB, getopt_op_on},
    {'\0', NULL, NULL, 0, 0}
  };
  opts[0].arg = &portstr;
  opts[1].arg = &encstr;
  opts[2].arg = &max_nfthreadsstr;
  opts[8].arg = &loglevel;
  opts[9].arg = &hostnamestr;
  i = grn_str_getopt(argc, argv, opts, &mode);
  if (i < 0) { mode = mode_usage; }
  if (portstr) { port = atoi(portstr); }
  if (encstr) {
    switch (*encstr) {
    case 'n' :
    case 'N' :
      enc = GRN_ENC_NONE;
      break;
    case 'e' :
    case 'E' :
      enc = GRN_ENC_EUC_JP;
      break;
    case 'u' :
    case 'U' :
      enc = GRN_ENC_UTF8;
      break;
    case 's' :
    case 'S' :
      enc = GRN_ENC_SJIS;
      break;
    case 'l' :
    case 'L' :
      enc = GRN_ENC_LATIN1;
      break;
    case 'k' :
    case 'K' :
      enc = GRN_ENC_KOI8R;
      break;
    }
  }
  if (max_nfthreadsstr) {
    max_nfthreads = atoi(max_nfthreadsstr);
  }
  batchmode = !isatty(0);
  if (grn_init()) { return -1; }
  grn_set_default_encoding(enc);
  if (loglevel) { SET_LOGLEVEL(atoi(loglevel)); }
  if (hostnamestr) {
    size_t hostnamelen = strlen(hostnamestr);
    if (hostnamelen > HOST_NAME_MAX) {
      memcpy(hostname, hostnamestr, HOST_NAME_MAX - 1);
      hostname[HOST_NAME_MAX] = '\0';
    } else {
      strcpy(hostname, hostnamestr);
    }
  } else {
    gethostname(hostname, HOST_NAME_MAX);
  }
  newdb = (mode & MODE_NEW_DB);
  useql = (mode & MODE_USE_QL);
  switch (mode & MODE_MASK) {
  case mode_alone :
    r = do_alone(argc - i, argv + i);
    break;
  case mode_client :
    r = do_client(argc <= i ? DEFAULT_DEST : argv[i]);
    break;
  case mode_daemon :
    r = do_daemon(argc <= i ? NULL : argv[i]);
    break;
  case mode_server :
    r = server(argc <= i ? NULL : argv[i]);
    break;
  default :
    usage(); r = -1;
    break;
  }
  grn_fin();
  return r;
}
