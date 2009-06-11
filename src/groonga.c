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

#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"
#define DEFAULT_MAX_NFTHREADS 8

static char hostname[HOST_NAME_MAX];
static int port = DEFAULT_PORT;
static int batchmode;

static void
usage(void)
{
  gethostname(hostname, HOST_NAME_MAX);
  fprintf(stderr,
          "Usage: groonga [options...] [dest]\n"
          "options:\n"
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
          "dest: <db pathname> or <dest hostname>\n"
          "  <db pathname>: when standalone/server mode\n"
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

static int
do_alone(char *path)
{
  int rc = -1;
  grn_obj *db = NULL;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, GRN_CTX_USE_QL|(batchmode ? GRN_CTX_BATCH_MODE : 0));
  if (path) { db = grn_db_open(ctx, path); }
  if (!db) { db = grn_db_create(ctx, path, NULL); }
  if (db) {
    char *buf = GRN_MALLOC(BUFSIZE);
    if (buf) {
      grn_ql_recv_handler_set(ctx, grn_ctx_stream_out_func, stdout);
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
      char *p = GRN_BULK_HEAD((grn_obj *)msg);
      char *e = GRN_BULK_CURR((grn_obj *)msg);
      char *p0 = p + 4;
      for (; p + 4 < e; p++) {
        if (*p == ' ' && !memcmp(p + 1, "HTTP", 4)) {
          break; }
      }
      grn_ql_send(ctx, p0, p - p0, header->flags);
    }
    break;
  }
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
        grn_obj *shorttext_type = grn_ctx_at(ctx, GRN_DB_SHORTTEXT);
        if ((cache_table = grn_table_create(ctx, "<cache>", 7, NULL,
                                            GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                                            shorttext_type, 0))) {
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
      grn_search_flags f = (header->qtype == MBCMD_REPLACE ||
                            header->qtype == MBCMD_REPLACEQ)
                           ? 0 : GRN_TABLE_ADD;
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
      grn_ctx_init(&edge->ctx, GRN_CTX_USE_QL);
      GRN_COM_QUEUE_INIT(&edge->recv_new);
      GRN_COM_QUEUE_INIT(&edge->send_old);
      grn_ql_recv_handler_set(&edge->ctx, output, edge);
      grn_ctx_use(&edge->ctx, (grn_obj *)com->ev->opaque);
      grn_ql_load(&edge->ctx, NULL);
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
    grn_obj *db = NULL;
    if (path) { db = grn_db_open(ctx, path); }
    if (!db) { db = grn_db_create(ctx, path, NULL); }
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
  mode_alone,
  mode_client,
  mode_daemon,
  mode_server,
  mode_usage
};

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
  switch (mode) {
  case mode_alone :
    r = do_alone(argc <= i ? NULL : argv[i]);
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
