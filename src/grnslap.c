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

#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"

static int port = DEFAULT_PORT;
static int proto = 'g';

#include <stdarg.h>
static void
lprint(grn_ctx *ctx, const char *fmt, ...)
{
  char buf[1024];
  grn_timeval tv;
  int len;
  va_list argp;
  grn_timeval_now(ctx, &tv);
  grn_timeval2str(ctx, &tv, buf);
  len = strlen(buf);
  buf[len++] = '|';
  va_start(argp, fmt);
  vsnprintf(buf + len, 1023 - len, fmt, argp);
  va_end(argp);
  buf[1023] = '\0';
  puts(buf);
}

static void
usage(void)
{
  fprintf(stderr,
          "Usage: grnslap [options...] [dest]\n"
          "options:\n"
          "  -P <protocol>:      http or gqtp (default: gqtp)\n"
          "  -p <port number>:   server port number (default: %d)\n"
          "dest: hostname (default: \"%s\")\n",
          DEFAULT_PORT, DEFAULT_DEST);
}

#define BUFSIZE 0x1000000

typedef struct _session session;

struct _session {
  grn_com_queue_entry eq;
  grn_com *com;
  struct timeval tv;
  grn_id id;
  int stat;
  int query_id;
  int n_query;
};

static grn_com_event ev;
static grn_com_queue fsessions;
static grn_hash *sessions;
static int done = 0;
static int nsent = 0;
static int nrecv = 0;

static session *
session_open(grn_ctx *ctx, const char *dest, int port)
{
  grn_id id;
  session *s;
  grn_com *com;
  grn_search_flags f = GRN_TABLE_ADD;
  if (!(com = grn_com_copen(ctx, &ev, dest, port))) { return NULL; }
  id = grn_hash_get(ctx, sessions, &com->fd, sizeof(grn_sock), (void **)&s, &f);
  com->opaque = s;
  s->com = com;
  s->id = id;
  s->stat = 1;
  return s;
}

static void
session_close(grn_ctx *ctx, session *s)
{
  if (!s->stat) { return; }
  grn_com_close(ctx, s->com);
  s->stat = 0;
  grn_hash_delete_by_id(ctx, sessions, s->id, NULL);
}

static session *
session_alloc(grn_ctx *ctx, const char *dest, int port)
{
  session *s;
  while ((s = (session *)grn_com_queue_deque(ctx, &fsessions))) {
    if (s->n_query < 1000000 && !s->com->closed) { return s; }
    //session_close(ctx, s);
  }
  return session_open(ctx, dest, port);
}

static void
msg_handler(grn_ctx *ctx, grn_obj *msg)
{
  grn_msg *m = (grn_msg *)msg;
  grn_com *com = ((grn_msg *)msg)->peer;
  session *s = com->opaque;
  s->stat = 3;
  /*
  lprint(ctx, "%d %x %x %04d %04d %d (%d)", (int)ctx->rc,
         m->header.proto, m->header.flags, s->n_query, s->query_id, com->fd,
         (int)GRN_BULK_VSIZE(msg));
  */
  if (ctx->rc) { m->header.proto = 0; }
  switch (m->header.proto) {
  case GRN_COM_PROTO_GQTP :
    if ((m->header.flags & GRN_QL_TAIL)) {
      grn_com_queue_enque(ctx, &fsessions, (grn_com_queue_entry *)s);
      nrecv++;
    }
    break;
  case GRN_COM_PROTO_HTTP :
    nrecv++;
    /* lprint(ctx, "recv: %d, %d", (int)GRN_BULK_VSIZE(msg), nrecv); */
    grn_com_close_(ctx, com);
    grn_com_queue_enque(ctx, &fsessions, (grn_com_queue_entry *)s);
    break;
  default :
    grn_com_close_(ctx, com);
    grn_com_queue_enque(ctx, &fsessions, (grn_com_queue_entry *)s);
    break;
  }
  grn_msg_close(ctx, msg);
}

static void * CALLBACK
receiver(void *arg)
{
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0, GRN_ENC_DEFAULT);
  while (!grn_com_event_poll(ctx, &ev, 100)) {
    if (nsent == nrecv && done) { break; }
    /*
    {
      session *s;
      GRN_HASH_EACH(sessions, id, NULL, NULL, &s, {
          printf("id=%d: fd=%d stat=%d q=%d n=%d\n", s->id, s->com->fd, s->stat, s->query_id, s->n_query);
      });
    }
    */
  }
  grn_ctx_fin(ctx);
  return NULL;
}

#define MAX_CON 100

static int
do_client(char *hostname)
{
  int rc = -1;
  char *buf;
  grn_thread thread;
  grn_com_header sheader;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0, GRN_ENC_DEFAULT);
  GRN_COM_QUEUE_INIT(&fsessions);
  sessions = grn_hash_create(ctx, NULL, sizeof(grn_sock), sizeof(session), 0, 0);
  sheader.proto = GRN_COM_PROTO_GQTP;
  sheader.qtype = 0;
  sheader.keylen = 0;
  sheader.level = 0;
  sheader.flags = 0;
  sheader.status = 0;
  sheader.opaque = 0;
  sheader.cas = 0;
  if ((buf = GRN_MALLOC(BUFSIZE))) {
    if (!grn_com_event_init(ctx, &ev, 1000, sizeof(grn_com))) {
      ev.msg_handler = msg_handler;
      if (!THREAD_CREATE(thread, receiver, NULL)) {
        lprint(ctx, "begin: %d", nsent);
        while (fgets(buf, BUFSIZE, stdin)) {
          uint32_t size = strlen(buf) - 1;
          session *s = session_alloc(ctx, hostname, port);
          if (s) {
            gettimeofday(&s->tv, NULL);
            s->n_query++;
            s->query_id = ++nsent;
            switch (proto) {
            case 'H' :
            case 'h' :
              if (grn_com_send_text(ctx, s->com, buf, size, 0)) {
                fprintf(stderr, "grn_com_send_text failed\n");
              }
              s->stat = 2;
              /*
              lprint(ctx, "sent %04d %04d %d",
                     s->n_query, s->query_id, s->com->fd);
              */
              break;
            default :
              if (grn_com_send(ctx, s->com, &sheader, buf, size, 0)) {
                fprintf(stderr, "grn_com_send failed\n");
              }
              break;
            }
          } else {
            fprintf(stderr, "grn_com_copen failed\n");
          }
          while ((nsent - nrecv) > MAX_CON) {
            /* lprint(ctx, "s:%d r:%d", nsent, nrecv); */
            usleep(1000);
          }
          if (!(nsent % 1000)) { lprint(ctx, "     : %d", nsent); }
        }
        done = 1;
        pthread_join(thread, NULL);
        {
          session *s;
          GRN_HASH_EACH(sessions, id, NULL, NULL, &s, {
            session_close(ctx, s);
          });
        }
        lprint(ctx, "end  : %d", nsent);
        rc = 0;
      } else {
        fprintf(stderr, "THREAD_CREATE failed\n");
      }
      grn_com_event_fin(ctx, &ev);
    } else {
      fprintf(stderr, "grn_com_event_init failed\n");
    }
    GRN_FREE(buf);
  }
  grn_hash_close(ctx, sessions);
  grn_ctx_fin(ctx);
  return rc;
}

enum {
  mode_client,
  mode_usage
};

int
main(int argc, char **argv)
{
  char *portstr = NULL, *protostr = NULL;
  int r, i, mode = mode_client;
  static grn_str_getopt_opt opts[] = {
    {'p', NULL, NULL, 0, getopt_op_none},
    {'P', NULL, NULL, 0, getopt_op_none},
    {'h', NULL, NULL, mode_usage, getopt_op_update},
    {'\0', NULL, NULL, 0, 0}
  };
  opts[0].arg = &portstr;
  opts[1].arg = &protostr;
  i = grn_str_getopt(argc, argv, opts, &mode);
  if (portstr) { port = atoi(portstr); }
  if (protostr) { proto = *protostr; }
  if (grn_init()) { return -1; }
  switch (mode) {
  case mode_client :
    r = do_client(argc <= i ? DEFAULT_DEST : argv[i]);
    break;
  default :
    usage(); r = -1;
    break;
  }
  grn_fin();
  return r;
}
