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

static void
usage(void)
{
  fprintf(stderr,
          "Usage: grnslap [options...] [dest]\n"
          "options:\n"
          "  -p <port number>:   server port number (default: %d)\n"
          "dest: hostname (default: \"%s\")\n",
          DEFAULT_PORT, DEFAULT_DEST);
}

#define BUFSIZE 0x1000000

typedef struct _session session;

struct _session {
  grn_com_queue_entry eq;
  session **prev;
  session *next;
  grn_com *com;
  struct timeval tv;
  int query_id;
  int n_query;
};

static grn_com_event ev;
static grn_com_queue fsessions;
static session *sessions = NULL;
static int done = 0;
static int nsent = 0;
static int nrecv = 0;

static session *
session_open(grn_ctx *ctx, const char *dest, int port)
{
  grn_com *com;
  session *s = GRN_CALLOC(sizeof(session));
  if (!s) { return NULL; }
  if (!(com = grn_com_copen(ctx, &ev, dest, port))) {
    GRN_FREE(s);
    return NULL;
  }
  com->opaque = s;
  s->com = com;
  s->next = sessions;
  s->prev = &sessions;
  sessions = s;
  return s;
}

static void
session_close(grn_ctx *ctx, session *s)
{
  grn_com_close(ctx, s->com);
  if (s->next) { s->next->prev = s->prev; }
  *s->prev = s->next;
  GRN_FREE(s);
}

static session *
session_alloc(grn_ctx *ctx, const char *dest, int port)
{
  session *s;
  while ((s = (session *)grn_com_queue_deque(ctx, &fsessions))) {
    if (s->n_query < 1000000) { return s; }
    session_close(ctx, s);
  }
  return session_open(ctx, dest, port);
}

static void
msg_handler(grn_ctx *ctx, grn_obj *msg)
{
  grn_com *com = ((grn_msg *)msg)->peer;
  session *s = com->opaque;
  //  printf("%d ", ctx->rc);
  nrecv++;
  grn_com_queue_enque(ctx, &fsessions, (grn_com_queue_entry *)s);
  grn_msg_close(ctx, msg);
}

static void * CALLBACK
receiver(void *arg)
{
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0, GRN_ENC_DEFAULT);
  while (!grn_com_event_poll(ctx, &ev, 100)) {
    if (nsent == nrecv && done) { break; }
  }
  grn_ctx_fin(ctx);
  return NULL;
}

#define MAX_CON 0x10000

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
  sheader.proto = GRN_COM_PROTO_GQTP;
  sheader.qtype = 0;
  sheader.keylen = 0;
  sheader.level = 0;
  sheader.flags = 0;
  sheader.status = 0;
  sheader.opaque = 0;
  sheader.cas = 0;
  if ((buf = GRN_MALLOC(BUFSIZE))) {
    if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
      ev.msg_handler = msg_handler;
      if (!THREAD_CREATE(thread, receiver, NULL)) {
        while (fgets(buf, BUFSIZE, stdin)) {
          uint32_t size = strlen(buf) - 1;
          session *s = session_alloc(ctx, hostname, port);
          if (s) {
            gettimeofday(&s->tv, NULL);
            s->n_query++;
            s->query_id = nsent++;
            if (grn_com_send(ctx, s->com, &sheader, buf, size, 0)) {
              fprintf(stderr, "grn_com_send failed\n");
            }
          } else {
            fprintf(stderr, "grn_com_copen failed\n");
          }
        }
        done = 1;
        pthread_join(thread, NULL);
        while (sessions) { session_close(ctx, sessions); }
        printf("%d\n", nsent);
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
  char *portstr = NULL;
  int r, i, mode = mode_client;
  static grn_str_getopt_opt opts[] = {
    {'p', NULL, NULL, 0, getopt_op_none},
    {'h', NULL, NULL, mode_usage, getopt_op_update},
    {'\0', NULL, NULL, 0, 0}
  };
  opts[0].arg = &portstr;
  i = grn_str_getopt(argc, argv, opts, &mode);
  if (portstr) { port = atoi(portstr); }
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
