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

#ifdef	__cplusplus
extern "C" {
#endif

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
#include <sys/poll.h>
#define GRN_COM_POLLIN  POLLIN
#define GRN_COM_POLLOUT POLLOUT
#endif /* USE_EPOLL */
#endif /* USE_SELECT */

typedef struct _grn_com grn_com;
typedef struct _grn_com_event grn_com_event;
typedef void grn_com_callback(grn_ctx *ctx, grn_com_event *, grn_com *);

enum {
  grn_com_idle = 0,
  grn_com_head,
  grn_com_body,
  grn_com_doing,
  grn_com_done,
  grn_com_connecting,
  grn_com_error,
  grn_com_closing
};

enum {
  grn_com_ok = 0,
  grn_com_emem,
  grn_com_erecv_head,
  grn_com_erecv_body,
  grn_com_eproto,
};

struct _grn_com {
  grn_sock fd;
  uint8_t status;
  int events;
  grn_com_callback *ev_in;
  grn_com_callback *ev_out;
};

struct _grn_com_event {
  struct _grn_hash *hash;
  int max_nevents;
  void *userdata;
#ifndef USE_SELECT
#ifdef USE_EPOLL
  int epfd;
  struct epoll_event *events;
#else /* USE_EPOLL */
  int dummy; /* dummy */
  struct pollfd *events;
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

#define GRN_COM_PROTO_GQTP   0x47
#define GRN_COM_PROTO_MBREQ  0x80
#define GRN_COM_PROTO_MBRES  0x81

typedef struct _grn_com_gqtp grn_com_gqtp;
typedef struct _grn_com_gqtp_header grn_com_gqtp_header;

struct _grn_com_gqtp_header {
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

struct _grn_com_gqtp {
  grn_com com;
  grn_rc rc;
  size_t rest;
  grn_obj msg;
  grn_com_callback *msg_in;
  void *userdata;
};

grn_com_gqtp *grn_com_gqtp_copen(grn_ctx *ctx, grn_com_event *ev, const char *dest, int port);
grn_com_gqtp *grn_com_gqtp_sopen(grn_ctx *ctx, grn_com_event *ev, int port, grn_com_callback *func);
grn_rc grn_com_gqtp_close(grn_ctx *ctx, grn_com_event *ev, grn_com_gqtp *cs);

grn_rc grn_com_gqtp_send(grn_ctx *ctx, grn_com_gqtp *cs,
                         grn_com_gqtp_header *header, char *body, uint32_t size);
grn_rc grn_com_gqtp_recv(grn_ctx *ctx, grn_com_gqtp *cs, grn_obj *buf, unsigned int *status);

#define GRN_COM_GQTP_MSG_HEADER(buf) ((grn_com_gqtp_header *)(buf)->u.b.head)
#define GRN_COM_GQTP_MSG_BODY(buf) ((buf)->u.b.head + sizeof(grn_com_gqtp_header))

#ifdef __cplusplus
}
#endif

#endif /* GRN_COM_H */
