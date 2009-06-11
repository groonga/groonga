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

#ifndef GRN_COM_H
#define GRN_COM_H

#ifndef GROONGA_H
#include "groonga_in.h"
#endif /* GROONGA_H */

#ifndef GRN_STR_H
#include "str.h"
#endif /* GRN_STR_H */

#ifndef GRN_HASH_H
#include "hash.h"
#endif /* GRN_HASH_H */

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#ifdef	__cplusplus
extern "C" {
#endif

/******* grn_com_queue ********/

typedef struct _grn_com_queue grn_com_queue;
typedef struct _grn_com_queue_entry grn_com_queue_entry;

#define GRN_COM_QUEUE_BINSIZE (0x100)

struct _grn_com_queue_entry {
  grn_obj obj;
  struct _grn_com_queue_entry *next;
};

struct _grn_com_queue {
  grn_com_queue_entry *bins[GRN_COM_QUEUE_BINSIZE];
  grn_com_queue_entry *next;
  grn_com_queue_entry **tail;
  uint8_t first;
  uint8_t last;
  grn_mutex mutex;
};

#define GRN_COM_QUEUE_INIT(q) {\
  (q)->next = NULL;\
  (q)->tail = &(q)->next;\
  (q)->first = 0;\
  (q)->last = 0;\
  MUTEX_INIT((q)->mutex);\
}

#define GRN_COM_QUEUE_EMPTYP(q) (((q)->first == (q)->last) && !(q)->next)

grn_rc grn_com_queue_enque(grn_ctx *ctx, grn_com_queue *q, grn_com_queue_entry *e);
grn_com_queue_entry *grn_com_queue_deque(grn_ctx *ctx, grn_com_queue *q);

/******* grn_com ********/

#ifdef USE_SELECT
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */
#define GRN_COM_POLLIN  1
#define GRN_COM_POLLOUT 2
#else /* USE_SELECT */
#ifdef USE_EPOLL
#include <sys/epoll.h>
#define GRN_COM_POLLIN  EPOLLIN
#define GRN_COM_POLLOUT EPOLLOUT
#else /* USE_EPOLL */
#ifdef USE_KQUEUE
#include <sys/event.h>
#define GRN_COM_POLLIN  EVFILT_READ
#define GRN_COM_POLLOUT EVFILT_WRITE
#else /* USE_KQUEUE */
#include <sys/poll.h>
#define GRN_COM_POLLIN  POLLIN
#define GRN_COM_POLLOUT POLLOUT
#endif /* USE_KQUEUE */
#endif /* USE_EPOLL */
#endif /* USE_SELECT */

typedef struct _grn_com grn_com;
typedef struct _grn_com_event grn_com_event;
typedef struct _grn_com_addr grn_com_addr;
typedef void grn_com_callback(grn_ctx *ctx, grn_com_event *, grn_com *);
typedef void grn_msg_handler(grn_ctx *ctx, grn_obj *msg);

enum {
  grn_com_ok = 0,
  grn_com_emem,
  grn_com_erecv_head,
  grn_com_erecv_body,
  grn_com_eproto,
};

struct _grn_com_addr {
  uint32_t addr;
  uint16_t port;
  uint16_t sid;
};

struct _grn_com {
  grn_sock fd;
  int events;
  uint16_t sid;
  uint8_t has_sid;
  uint8_t closed;
  grn_com_queue new;
  grn_com_event *ev;
  void *opaque;
};

struct _grn_com_event {
  struct _grn_hash *hash;
  int max_nevents;
  grn_ctx *ctx;
  grn_mutex mutex;
  grn_cond cond;
  grn_com_queue recv_old;
  grn_msg_handler *msg_handler;
  grn_com_addr curr_edge_id;
  grn_com *acceptor;
  void *opaque;
#ifndef USE_SELECT
#ifdef USE_EPOLL
  int epfd;
  struct epoll_event *events;
#else /* USE_EPOLL */
#ifdef USE_KQUEUE
  int kqfd;
  struct kevent *events;
#else /* USE_KQUEUE */
  int dummy; /* dummy */
  struct pollfd *events;
#endif /* USE_KQUEUE */
#endif /* USE_EPOLL */
#endif /* USE_SELECT */
};

grn_rc grn_com_init(void);
void grn_com_fin(void);
grn_rc grn_com_event_init(grn_ctx *ctx, grn_com_event *ev, int max_nevents, int data_size);
grn_rc grn_com_event_fin(grn_ctx *ctx, grn_com_event *ev);
grn_rc grn_com_event_add(grn_ctx *ctx, grn_com_event *ev, grn_sock fd, int events, grn_com **com);
grn_rc grn_com_event_mod(grn_ctx *ctx, grn_com_event *ev, grn_sock fd, int events, grn_com **com);
grn_rc grn_com_event_del(grn_ctx *ctx, grn_com_event *ev, grn_sock fd);
grn_rc grn_com_event_poll(grn_ctx *ctx, grn_com_event *ev, int timeout);
grn_rc grn_com_event_each(grn_ctx *ctx, grn_com_event *ev, grn_com_callback *func);

/******* grn_com_gqtp ********/

#define GRN_COM_PROTO_HTTP   0x47
#define GRN_COM_PROTO_GQTP   0xc7
#define GRN_COM_PROTO_MBREQ  0x80
#define GRN_COM_PROTO_MBRES  0x81

typedef struct _grn_com_header grn_com_header;

struct _grn_com_header {
  uint8_t proto;
  uint8_t qtype;
  uint16_t keylen;
  uint8_t level;
  uint8_t flags;
  uint16_t status;
  uint32_t size;
  uint32_t opaque;
  uint64_t cas;
};

grn_com *grn_com_copen(grn_ctx *ctx, grn_com_event *ev, const char *dest, int port);
grn_rc grn_com_sopen(grn_ctx *ctx, grn_com_event *ev, int port,
                     grn_msg_handler *func, struct hostent *he);

void grn_com_close_(grn_ctx *ctx, grn_com *com);
grn_rc grn_com_close(grn_ctx *ctx, grn_com *com);

grn_rc grn_com_send(grn_ctx *ctx, grn_com *cs,
                    grn_com_header *header, char *body, uint32_t size, int flags);
grn_rc grn_com_recv(grn_ctx *ctx, grn_com *cs, grn_com_header *header, grn_obj *buf);
grn_rc grn_com_send_text(grn_ctx *ctx, grn_com *cs, const char *body, uint32_t size, int flags);

/******* grn_msg ********/

typedef struct _grn_msg grn_msg;

struct _grn_msg {
  grn_com_queue_entry qe;
  grn_com *peer;
  grn_ctx *ctx;
  grn_com_queue *old;
  grn_com_header header;
  grn_com_addr edge_id;
};

grn_rc grn_msg_send(grn_ctx *ctx, grn_obj *msg, int flags);
grn_obj *grn_msg_open_for_reply(grn_ctx *ctx, grn_obj *query, grn_com_queue *old);
grn_obj *grn_msg_open(grn_ctx *ctx, grn_com *com, grn_com_queue *old);
grn_rc grn_msg_set_property(grn_ctx *ctx, grn_obj *obj,
                            uint16_t status, uint32_t key_size, uint8_t extra_size);
grn_rc grn_msg_close(grn_ctx *ctx, grn_obj *msg);

#ifdef __cplusplus
}
#endif

#endif /* GRN_COM_H */
