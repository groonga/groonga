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

#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"

static int port = DEFAULT_PORT;
static int batchmode;
static grn_encoding enc = grn_enc_default;

static void
usage(void)
{
  fprintf(stderr,
          "Usage: groonga [options...] [dest]\n"
          "options:\n"
          "  -a:                 run in standalone mode (default)\n"
          "  -c:                 run in client mode\n"
          "  -s:                 run in server mode\n"
          "  -e:                 encoding for new database [none|euc|utf8|sjis|latin1|koi8r]\n"
          "  -p <port number>:   server port number (default: %d)\n"
          "dest: <db pathname> or <dest hostname>\n"
          "  <db pathname>: when standalone/server mode\n"
          "  <dest hostname>: when client mode (default: \"%s\")\n",
          DEFAULT_PORT, DEFAULT_DEST);
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
  grn_ctx_init(ctx, GRN_CTX_USE_QL|(batchmode ? GRN_CTX_BATCH_MODE : 0), enc);
  if (path) { db = grn_db_open(ctx, path); }
  if (!db) { db = grn_db_create(ctx, path, enc); }
  if (db) {
    char *buf = GRN_MALLOC(BUFSIZE);
    if (buf) {
      grn_ql_recv_handler_set(ctx, grn_ctx_stream_out_func, stdout);
      grn_ql_load(ctx, NULL);
      while (prompt(), fgets(buf, BUFSIZE, stdin)) {
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
  grn_ctx_init(ctx, (batchmode ? GRN_CTX_BATCH_MODE : 0), enc);
  if (!grn_ql_connect(ctx, hostname, port, 0)) {
    grn_ql_info info;
    grn_ql_info_get(ctx, &info);
    if (!grn_bulk_reinit(ctx, info.outbuf, BUFSIZE)) {
      if (batchmode) { BATCHMODE(ctx); }
      while (prompt(), fgets(GRN_BULK_HEAD(info.outbuf),
                             GRN_BULK_WSIZE(info.outbuf), stdin)) {
        int flags;
        char *str;
        unsigned int str_len;
        uint32_t size = strlen(GRN_BULK_HEAD(info.outbuf)) - 1;
        if (grn_ql_send(ctx, GRN_BULK_HEAD(info.outbuf), size, 0)) { break; }
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
      rc = 0;
    } else {
      fprintf(stderr, "grn_bulk_reinit failed (%d)\n", BUFSIZE);
    }
  } else {
    fprintf(stderr, "grn_ql_connect failed (%s:%d)\n", hostname, port);
  }
  grn_ctx_fin(ctx);
exit :
  return rc;
}

/* server */

static void
output(grn_ctx *ctx, int flags, void *arg)
{
  static uint32_t info = 0;
  grn_com_sqtp *cs = arg;
  grn_com_sqtp_header header;
  grn_obj *buf = &ctx->impl->outbuf;
  header.size = GRN_BULK_VSIZE(buf);
  header.flags = (flags & GRN_QL_MORE) ? GRN_QL_MORE : GRN_QL_TAIL;
  header.qtype = 1;
  header.level = 0;
  header.status = 0;
  header.info = info++; /* for debug */
  if (ctx->stat == GRN_QL_QUIT) { cs->com.status = grn_com_closing; }
  grn_com_sqtp_send(ctx, cs, &header, GRN_BULK_HEAD(buf));
  GRN_BULK_REWIND(buf);
}

static void
errout(grn_ctx *ctx, grn_com_sqtp *cs, char *msg)
{
  grn_com_sqtp_header header;
  header.size = strlen(msg);
  header.flags = GRN_QL_TAIL;
  grn_com_sqtp_send(ctx, cs, &header, msg);
  GRN_LOG(grn_log_error, "errout: %s", msg);
}

static void
grn_ctx_close(grn_ctx *ctx)
{
  grn_ctx_fin(ctx);
  GRN_GFREE(ctx);
}

inline static void
do_msg(grn_com_event *ev, grn_com_sqtp *cs)
{
  grn_ctx *ctx = (grn_ctx *)cs->userdata;
  if (!ctx) {
    ctx = GRN_GMALLOC(sizeof(grn_ctx));
    grn_ctx_init(ctx, GRN_CTX_USE_QL, enc);
    if (!ctx) {
      cs->com.status = grn_com_closing;
      errout(ctx, cs, "*** ERROR: ctx open failed");
      return;
    }
    grn_ql_recv_handler_set(ctx, output, cs);
    grn_ctx_use(ctx, (grn_obj *)ev->userdata);
    grn_ql_load(ctx, NULL);
    cs->userdata = ctx;
  }
  {
    char *body = GRN_COM_SQTP_MSG_BODY(&cs->msg);
    grn_com_sqtp_header *header = GRN_COM_SQTP_MSG_HEADER(&cs->msg);
    uint32_t size = header->size;
    uint16_t flags = header->flags;
    grn_ql_send(ctx, body, size, flags);
    if (ctx->stat == GRN_QL_QUIT || grn_gctx.stat == GRN_QL_QUIT) {
      cs->com.status = grn_com_closing;
      grn_ctx_close(ctx);
      cs->userdata = NULL;
    } else {
      cs->com.status = grn_com_idle;
    }
    return;
  }
}

/* query queue */

static grn_mutex q_mutex;
static grn_cond q_cond;

typedef struct {
  uint8_t head;
  uint8_t tail;
  grn_com_sqtp *v[0x100];
} queue;

static void
queue_init(queue *q)
{
  q->head = 0;
  q->tail = 0;
}

static grn_rc
queue_enque(queue *q, grn_com_sqtp *c)
{
  uint8_t i = q->tail + 1;
  if (i == q->head) { return grn_other_error; }
  q->v[q->tail] = c;
  q->tail = i;
  return GRN_SUCCESS;
}

static queue qq;

/* worker thread */

#define MAX_NFTHREADS 0x04

static uint32_t nthreads = 0, nfthreads = 0;

static void * CALLBACK
thread_start(void *arg)
{
  grn_com_sqtp *cs;
  GRN_LOG(grn_log_notice, "thread start (%d/%d)", nfthreads, nthreads + 1);
  MUTEX_LOCK(q_mutex);
  nthreads++;
  do {
    nfthreads++;
    while (qq.tail == qq.head) { COND_WAIT(q_cond, q_mutex); }
    nfthreads--;
    if (grn_gctx.stat == GRN_QL_QUIT) { break; }
    cs = qq.v[qq.head++];
    MUTEX_UNLOCK(q_mutex);
    if (cs) { do_msg((grn_com_event *) arg, cs); }
    MUTEX_LOCK(q_mutex);
  } while (nfthreads < MAX_NFTHREADS);
  nthreads--;
  MUTEX_UNLOCK(q_mutex);
  GRN_LOG(grn_log_notice, "thread end (%d/%d)", nfthreads, nthreads);
  return NULL;
}

static void
msg_handler(grn_ctx *ctx, grn_com_event *ev, grn_com *c)
{
  grn_com_sqtp *cs = (grn_com_sqtp *)c;
  if (cs->rc) {
    grn_ctx *ctx = (grn_ctx *)cs->userdata;
    GRN_LOG(grn_log_notice, "connection closed..");
    if (ctx) { grn_ctx_close(ctx); }
    grn_com_sqtp_close(ctx, ev, cs);
    return;
  }
  {
    int i = 0;
    while (queue_enque(&qq, (grn_com_sqtp *)c)) {
      if (i) {
        GRN_LOG(grn_log_notice, "queue is full try=%d qq(%d-%d) thd(%d/%d) %d", i, qq.head, qq.tail, nfthreads, nthreads, *ev->hash->n_entries);
      }
      if (++i == 100) {
        errout(ctx, (grn_com_sqtp *)c, "*** ERROR: query queue is full");
        return;
      }
      usleep(1000);
    }
  }
  MUTEX_LOCK(q_mutex);
  if (!nfthreads && nthreads < MAX_NFTHREADS) {
    grn_thread thread;
    MUTEX_UNLOCK(q_mutex);
    if (THREAD_CREATE(thread, thread_start, ev)) { GSERR("pthread_create"); }
    return;
  }
  COND_SIGNAL(q_cond);
  MUTEX_UNLOCK(q_mutex);
}

#define MAX_CON 0x10000

static int
server(char *path)
{
  int rc = -1;
  grn_com_event ev;
  grn_ctx ctx_, *ctx = &ctx_;
  MUTEX_INIT(q_mutex);
  COND_INIT(q_cond);
  grn_ctx_init(ctx, 0, enc);
  queue_init(&qq);
  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com_sqtp))) {
    grn_obj *db = NULL;
    if (path) { db = grn_db_open(ctx, path); }
    if (!db) { db = grn_db_create(ctx, path, enc); }
    if (db) {
      grn_com_sqtp *cs;
      ev.userdata = db;
      if ((cs = grn_com_sqtp_sopen(ctx, &ev, port, msg_handler))) {
        while (!grn_com_event_poll(ctx, &ev, 3000) && grn_gctx.stat != GRN_QL_QUIT) ;
        for (;;) {
          MUTEX_LOCK(q_mutex);
          if (nthreads == nfthreads) { break; }
          MUTEX_UNLOCK(q_mutex);
          usleep(1000);
        }
        {
          grn_sock *pfd;
          int key_size;
          grn_com_sqtp *com;
          GRN_HASH_EACH(ev.hash, id, &pfd, &key_size, &com, {
            grn_ctx *ctx = (grn_ctx *)com->userdata;
            if (ctx) { grn_ctx_close(ctx); }
            grn_com_sqtp_close(ctx, &ev, com);
          });
        }
        rc = 0;
      } else {
        fprintf(stderr, "grn_com_sqtp_sopen failed (%d)\n", port);
      }
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
do_server(char *path)
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
  mode_server,
  mode_dserver,
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
  char *portstr = NULL, *encstr = NULL, *loglevel = NULL;
  int r, i, mode = mode_alone;
  static grn_str_getopt_opt opts[] = {
    {'p', NULL, NULL, 0, getopt_op_none},
    {'e', NULL, NULL, 0, getopt_op_none},
    {'h', NULL, NULL, mode_usage, getopt_op_update},
    {'a', NULL, NULL, mode_alone, getopt_op_update},
    {'c', NULL, NULL, mode_client, getopt_op_update},
    {'s', NULL, NULL, mode_server, getopt_op_update},
    {'d', NULL, NULL, mode_dserver, getopt_op_update},
    {'l', NULL, NULL, 0, getopt_op_none},
    {'\0', NULL, NULL, 0, 0}
  };
  opts[0].arg = &portstr;
  opts[1].arg = &encstr;
  opts[7].arg = &loglevel;
  i = grn_str_getopt(argc, argv, opts, &mode);
  if (i < 0) { mode = mode_usage; }
  if (portstr) { port = atoi(portstr); }
  if (encstr) {
    switch (*encstr) {
    case 'n' :
    case 'N' :
      enc = grn_enc_none;
      break;
    case 'e' :
    case 'E' :
      enc = grn_enc_euc_jp;
      break;
    case 'u' :
    case 'U' :
      enc = grn_enc_utf8;
      break;
    case 's' :
    case 'S' :
      enc = grn_enc_sjis;
      break;
    case 'l' :
    case 'L' :
      enc = grn_enc_latin1;
      break;
    case 'k' :
    case 'K' :
      enc = grn_enc_koi8r;
      break;
    }
  }
  batchmode = !isatty(0);
  if (grn_init()) { return -1; }
  grn_gctx.encoding = enc; /* todo : make it api */
  if (loglevel) { SET_LOGLEVEL(atoi(loglevel)); }
  switch (mode) {
  case mode_alone :
    r = do_alone(argc <= i ? NULL : argv[i]);
    break;
  case mode_client :
    r = do_client(argc <= i ? DEFAULT_DEST : argv[i]);
    break;
  case mode_server :
    r = do_server(argc <= i ? NULL : argv[i]);
    break;
  case mode_dserver :
    r = server(argc <= i ? NULL : argv[i]);
    break;
  default :
    usage(); r = -1;
    break;
  }
  grn_fin();
  return r;
}
