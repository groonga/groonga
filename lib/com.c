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

#include "groonga_in.h"

#include <string.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif /* HAVE_NETINET_TCP_H */
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#include "com.h"
#include "ctx.h"

#ifndef PF_INET
#define PF_INET AF_INET
#endif /* PF_INET */

#ifdef USE_MSG_NOSIGNAL
#if __FreeBSD__ >= 2 && __FreeBSD_version >= 600020
#define MSG_NOSIGNAL 0x20000
#endif /* __FreeBSD__ >= 2 && __FreeBSD_version >= 600020 */
#else /* USE_MSG_NOSIGNAL */
#define MSG_NOSIGNAL 0
#endif /* USE_MSG_NOSIGNAL */

/******* grn_com_queue ********/

grn_rc
grn_com_queue_enque(grn_ctx *ctx, grn_com_queue *q, grn_com_queue_entry *e)
{
  uint8_t i = q->last + 1;
  e->next = NULL;
  if (q->first == i || q->next) {
    MUTEX_LOCK(q->mutex);
    if (q->first == i || q->next) {
      *q->tail = e;
      q->tail = &e->next;
    } else {
      q->bins[q->last] = e;
      q->last = i;
    }
    MUTEX_UNLOCK(q->mutex);
  } else {
    q->bins[q->last] = e;
    q->last = i;
  }
  return GRN_SUCCESS;
}

grn_com_queue_entry *
grn_com_queue_deque(grn_ctx *ctx, grn_com_queue *q)
{
  grn_com_queue_entry *e = NULL;
  if (q->first == q->last) {
    if (q->next) {
      MUTEX_LOCK(q->mutex);
      e = q->next;
      q->next = e->next;
      MUTEX_UNLOCK(q->mutex);
    }
  } else {
    e = q->bins[q->first++];
  }
  return e;
}

#define GRN_COM_QUEUE_EMPTYP(q) (((q)->first == (q)->last) && !(q)->next)

/******* grn_msg ********/

grn_obj *
grn_msg_open(grn_ctx *ctx, grn_com *com, grn_com_queue *old)
{
  grn_msg *msg = NULL;
  if (old && (msg = (grn_msg *)grn_com_queue_deque(ctx, old))) {
    if (msg->ctx != ctx) {
      ERR(GRN_INVALID_ARGUMENT, "ctx unmatch");
      return NULL;
    }
    GRN_BULK_REWIND(&msg->qe.obj);
    msg->qe.next = NULL;
    msg->peer = com;
    msg->old = old;
  } else if ((msg = GRN_MALLOCN(grn_msg, 1))) {
    GRN_OBJ_INIT(&msg->qe.obj, GRN_MSG, 0);
    msg->qe.obj.header.impl_flags |= GRN_OBJ_ALLOCATED;
    msg->qe.next = NULL;
    msg->peer = com;
    msg->ctx = ctx;
    msg->old = old;
  }
  return (grn_obj *)msg;
}

grn_obj *
grn_msg_open_for_reply(grn_ctx *ctx, grn_obj *query, grn_com_queue *old)
{
  grn_msg *req = (grn_msg *)query, *msg = NULL;
  if (req && (msg = (grn_msg *)grn_msg_open(ctx, req->peer, old))) {
    msg->edge_id = req->edge_id;
    msg->query_id = req->query_id;
    msg->flags = req->flags;
    msg->protocol = req->protocol == GRN_COM_PROTO_MBREQ
      ? GRN_COM_PROTO_MBRES : req->protocol;
  }
  return (grn_obj *)msg;
}

grn_rc
grn_msg_close(grn_ctx *ctx, grn_obj *obj)
{
  grn_msg *msg = (grn_msg *)obj;
  if (ctx == msg->ctx) { return grn_obj_close(ctx, obj); }
  return grn_com_queue_enque(ctx, msg->old, (grn_com_queue_entry *)msg);
}

/******* sender

static void * CALLBACK
sender(void *arg)
{
  grn_com_event *ev = (grn_com_event *)arg;
  MUTEX_LOCK(ev->mutex);
  while (ev->ctx->stat != GRN_QL_QUIT) {
    while (!(peers_with_msg = get_out_msgs(peers))) {
      COND_WAIT(ev->cond, ev->mutex);
      if (ev->ctx->stat == GRN_QL_QUIT) { goto exit; }
    }
    MUTEX_UNLOCK(ev->mutex);
    peers_writable = poll(peers_with_msg, POLLOUT);
    for (peer in peers_writable) {
      msgs = queue_peek_all(peer->send_new);
      if (!send(msgs)) {
        queue_deque(peer->send_new, msgs);
        for (msg in msgs) { enque(msg->edge->send_old, msg); }
      }
    }
    MUTEX_LOCK(ev->mutex);
  }
exit :
  MUTEX_UNLOCK(ev->mutex);
  return NULL;
}

********/

grn_rc
grn_msg_send(grn_ctx *ctx, grn_obj *msg, int flags)
{
  grn_rc rc;
  grn_msg *m = (grn_msg *)msg;
  grn_com *peer = m->peer;
  if (GRN_COM_QUEUE_EMPTYP(peer->new)) {
    switch (m->protocol) {
    case GRN_COM_PROTO_GQTP :
      {
        grn_com_gqtp_header sheader;
        if ((flags & GRN_QL_MORE)) { flags |= GRN_QL_QUIET; }
        sheader.qtype = 0;
        sheader.keylen = 0;
        sheader.level = 0;
        sheader.flags = flags;
        sheader.status = 0;
        sheader.opaque = 0;
        sheader.cas = 0;
        //todo : MSG_DONTWAIT
        rc = grn_com_gqtp_send(ctx, peer, &sheader,
                               GRN_BULK_HEAD(msg), GRN_BULK_VSIZE(msg));
        if (rc != GRN_OPERATION_WOULD_BLOCK) { return rc; }
      }
      break;
    case GRN_COM_PROTO_MBREQ :
      return GRN_FUNCTION_NOT_IMPLEMENTED;
    case GRN_COM_PROTO_MBRES :
      // todo:
      // grn_com_mbres_send(ctx, peer, header, &buf, MBRES_SUCCESS, 0, 4);
      break;
    default :
      return GRN_INVALID_ARGUMENT;
    }
  }
  MUTEX_LOCK(peer->ev->mutex);
  rc = grn_com_queue_enque(ctx, peer->new, (grn_com_queue_entry *)msg);
  COND_SIGNAL(peer->ev->cond);
  MUTEX_UNLOCK(peer->ev->mutex);
  return rc;
}

/******* grn_com ********/

grn_rc
grn_com_init(void)
{
#ifdef WIN32
  WSADATA wd;
  if (WSAStartup(MAKEWORD(2, 0), &wd) != 0) {
    grn_ctx *ctx = &grn_gctx;
    SERR("WSAStartup");
  }
#else /* WIN32 */
#ifndef USE_MSG_NOSIGNAL
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    grn_ctx *ctx = &grn_gctx;
    SERR("signal");
  }
#endif /* USE_MSG_NOSIGNAL */
#endif /* WIN32 */
  return grn_gctx.rc;
}

void
grn_com_fin(void)
{
#ifdef WIN32
  WSACleanup();
#endif /* WIN32 */
}

grn_rc
grn_com_event_init(grn_ctx *ctx, grn_com_event *ev, int max_nevents, int data_size)
{
  ev->max_nevents = max_nevents;
  if ((ev->hash = grn_hash_create(ctx, NULL, sizeof(grn_sock), data_size, 0, GRN_ENC_NONE))) {
    MUTEX_INIT(ev->mutex);
    COND_INIT(ev->cond);
    GRN_COM_QUEUE_INIT(&ev->recv_old);
    /*
    if (THREAD_CREATE(thread, sender, ev)) { SERR("pthread_create"); }
    */
#ifndef USE_SELECT
#ifdef USE_EPOLL
    if ((ev->events = GRN_MALLOC(sizeof(struct epoll_event) * max_nevents))) {
      if ((ev->epfd = epoll_create(max_nevents)) != -1) {
        goto exit;
      } else {
        SERR("epoll_create");
      }
      GRN_FREE(ev->events);
    }
#else /* USE_EPOLL */
    if ((ev->events = GRN_MALLOC(sizeof(struct pollfd) * max_nevents))) {
      goto exit;
    }
#endif /* USE_EPOLL */
    grn_hash_close(ctx, ev->hash);
    ev->hash = NULL;
    ev->events = NULL;
#else /* USE_SELECT */

#endif /* USE_SELECT */
  }
exit :
  return ctx->rc;
}

grn_rc
grn_com_event_fin(grn_ctx *ctx, grn_com_event *ev)
{
  if (ev->hash) { grn_hash_close(ctx, ev->hash); }
#ifndef USE_SELECT
  if (ev->events) { GRN_FREE(ev->events); }
#endif /* USE_SELECT */
  return GRN_SUCCESS;
}

grn_rc
grn_com_event_add(grn_ctx *ctx, grn_com_event *ev, grn_sock fd, int events, grn_com **com)
{
  grn_com *c;
  /* todo : expand events */
  if (!ev || *ev->hash->n_entries == ev->max_nevents) {
    if (ev) { GRN_LOG(ctx, GRN_LOG_ERROR, "too many connections (%d)", ev->max_nevents); }
    return GRN_INVALID_ARGUMENT;
  }
#ifdef USE_EPOLL
  {
    struct epoll_event e;
    memset(&e, 0, sizeof(struct epoll_event));
    e.data.fd = (fd);
    e.events = (__uint32_t) events;
    if (epoll_ctl(ev->epfd, EPOLL_CTL_ADD, (fd), &e) == -1) {
      SERR("epoll_ctl");
      return ctx->rc;
    }
  }
#endif /* USE_EPOLL*/
  {
    grn_search_flags f = GRN_TABLE_ADD;
    if (grn_hash_get(ctx, ev->hash, &fd, sizeof(grn_sock), (void **)&c, &f)) {
      c->fd = fd;
      c->events = events;
      c->status = grn_com_idle;
      if (com) { *com = c; }
    }
  }
  return ctx->rc;
}

grn_rc
grn_com_event_mod(grn_ctx *ctx, grn_com_event *ev, grn_sock fd, int events, grn_com **com)
{
  grn_com *c;
  if (!ev) { return GRN_INVALID_ARGUMENT; }
  if (grn_hash_at(ctx, ev->hash, &fd, sizeof(grn_sock), (void **)&c)) {
    if (c->fd != fd) {
      GRN_LOG(ctx, GRN_LOG_ERROR, "grn_com_event_mod fd unmatch %d != %d", c->fd, fd);
      return GRN_OBJECT_CORRUPT;
    }
    if (com) { *com = c; }
    if (c->events != events) {
#ifdef USE_EPOLL
      struct epoll_event e;
      memset(&e, 0, sizeof(struct epoll_event));
      e.data.fd = (fd);
      e.events = (__uint32_t) events;
      if (epoll_ctl(ev->epfd, EPOLL_CTL_MOD, (fd), &e) == -1) {
        SERR("epoll_ctl");
        return ctx->rc;
      }
#endif /* USE_EPOLL*/
      c->events = events;
    }
    return GRN_SUCCESS;
  }
  return GRN_INVALID_ARGUMENT;
}

grn_rc
grn_com_event_del(grn_ctx *ctx, grn_com_event *ev, grn_sock fd)
{
  if (!ev) { return GRN_INVALID_ARGUMENT; }
  {
    grn_com *c;
    grn_id id = grn_hash_at(ctx, ev->hash, &fd, sizeof(grn_sock), (void **)&c);
    if (id) {
#ifdef USE_EPOLL
      struct epoll_event e;
      memset(&e, 0, sizeof(struct epoll_event));
      e.data.fd = fd;
      e.events = c->events;
      if (epoll_ctl(ev->epfd, EPOLL_CTL_DEL, fd, &e) == -1) {
        SERR("epoll_ctl");
        return ctx->rc;
      }
#endif /* USE_EPOLL*/
      return grn_hash_delete_by_id(ctx, ev->hash, id, NULL);
    } else {
      GRN_LOG(ctx, GRN_LOG_ERROR, "%04x| fd(%d) not found in ev(%p)", getpid(), fd, ev);
      return GRN_INVALID_ARGUMENT;
    }
  }
}

grn_rc
grn_com_event_poll(grn_ctx *ctx, grn_com_event *ev, int timeout)
{
  int nevents;
  grn_com *com;
#ifdef USE_SELECT
  uint32_t dummy;
  grn_sock *pfd;
  int nfds = 0;
  fd_set rfds;
  fd_set wfds;
  struct timeval tv;
  if (timeout >= 0) {
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
  }
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  ctx->errlvl = GRN_OK;
  ctx->rc = GRN_SUCCESS;
  GRN_HASH_EACH(ev->hash, eh, &pfd, &dummy, &com, {
    if (com->status == grn_com_closed) { continue; }
    if ((com->events & GRN_COM_POLLIN)) { FD_SET(*pfd, &rfds); }
    if ((com->events & GRN_COM_POLLOUT)) { FD_SET(*pfd, &wfds); }
    if (*pfd > nfds) { nfds = *pfd; }
  });
  errno = 0;
  nevents = select(nfds + 1, &rfds, &wfds, NULL, (timeout >= 0) ? &tv : NULL);
  if (nevents < 0) {
#ifdef WIN32
    if (WSAGetLastError() == WSAEINTR) { return GRN_SUCCESS; }
#else /* WIN32 */
    if (errno == EINTR) { return GRN_SUCCESS; }
#endif /* WIN32 */
    SERR("select");
    return ctx->rc;
  }
  if (timeout < 0 && !nevents) { GRN_LOG(ctx, GRN_LOG_NOTICE, "select returns 0 events"); }
  GRN_HASH_EACH(ev->hash, eh, &pfd, &dummy, &com, {
    if (FD_ISSET(*pfd, &rfds)) { com->ev_in(ctx, ev, com); }
    if (FD_ISSET(*pfd, &wfds)) { com->ev_out(ctx, ev, com); }
  });
#else /* USE_SELECT */
#ifdef USE_EPOLL
  struct epoll_event *ep;
  ctx->errlvl = GRN_OK;
  ctx->rc = GRN_SUCCESS;
  errno = 0;
  nevents = epoll_wait(ev->epfd, ev->events, ev->max_nevents, timeout);
#else /* USE_EPOLL */
  uint32_t dummy;
  int nfd = 0, *pfd;
  struct pollfd *ep = ev->events;
  ctx->errlvl = GRN_OK;
  ctx->rc = GRN_SUCCESS;
  GRN_HASH_EACH(ev->hash, eh, &pfd, &dummy, &com, {
    if (com->status == grn_com_closed) { continue; }
    ep->fd = *pfd;
    //    ep->events =(short) com->events;
    ep->events = POLLIN;
    ep->revents = 0;
    ep++;
    nfd++;
  });
  errno = 0;
  nevents = poll(ev->events, nfd, timeout);
#endif /* USE_EPOLL */
  if (nevents < 0) {
    if (errno == EINTR) { return GRN_SUCCESS; }
    SERR("poll");
    return ctx->rc;
  }
  if (timeout < 0 && !nevents) { GRN_LOG(ctx, GRN_LOG_NOTICE, "poll returns 0 events"); }
  for (ep = ev->events; nevents; ep++) {
    int efd;
#ifdef USE_EPOLL
    efd = ep->data.fd;
    nevents--;
    if (!grn_hash_at(ctx, ev->hash, &efd, sizeof(grn_sock), (void *)&com)) {
      struct epoll_event e;
      GRN_LOG(ctx, GRN_LOG_ERROR, "fd(%d) not found in ev->hash", efd);
      memset(&e, 0, sizeof(struct epoll_event));
      e.data.fd = efd;
      e.events = ep->events;
      if (epoll_ctl(ev->epfd, EPOLL_CTL_DEL, efd, &e) == -1) { SERR("epoll_ctl"); }
      if (grn_sock_close(efd) == -1) { SERR("close"); }
      continue;
    }
    if ((ep->events & GRN_COM_POLLIN)) { com->ev_in(ctx, ev, com); }
    if ((ep->events & GRN_COM_POLLOUT)) { com->ev_out(ctx, ev, com); }
#else /* USE_EPOLL */
    efd = ep->fd;
    if (!(ep->events & ep->revents)) { continue; }
    nevents--;
    if (!grn_hash_at(ctx, ev->hash, &efd, sizeof(grn_sock), (void *)&com)) {
      GRN_LOG(ctx, GRN_LOG_ERROR, "fd(%d) not found in ev->hash", efd);
      if (grn_sock_close(efd) == -1) { SERR("close"); }
      continue;
    }
    if ((ep->revents & GRN_COM_POLLIN)) { com->ev_in(ctx, ev, com); }
    if ((ep->revents & GRN_COM_POLLOUT)) { com->ev_out(ctx, ev, com); }
#endif /* USE_EPOLL */
  }
#endif /* USE_SELECT */
  /* todo :
  while (!(msg = (grn_com_msg *)grn_com_queue_deque(&recv_old))) {
    grn_msg_close(ctx, msg);
  }
  */
  return GRN_SUCCESS;
}

/******* grn_com_gqtp ********/

grn_rc
grn_com_gqtp_send(grn_ctx *ctx, grn_com_gqtp *cs,
                  grn_com_gqtp_header *header, char *body, uint32_t size)
{
  ssize_t ret, whole_size = sizeof(grn_com_gqtp_header) + size;
  header->proto = GRN_COM_PROTO_GQTP;
  header->size = htonl(size);
  if (cs->com.status == grn_com_closing) { header->flags |= GRN_QL_QUIT; }
  GRN_LOG(ctx, GRN_LOG_INFO, "send (%d,%x,%d,%02x,%02x,%04x)", size, header->flags, header->proto, header->qtype, header->level, header->status);

  if (size) {
#ifdef WIN32
    ssize_t reth;
    if ((reth = send(cs->com.fd, header, sizeof(grn_com_gqtp_header), 0)) == -1) {
      SERR("send size");
      cs->rc = ctx->rc;
      goto exit;
    }
    if ((ret = send(cs->com.fd, body, size, 0)) == -1) {
      SERR("send body");
      cs->rc = ctx->rc;
      goto exit;
    }
    ret += reth;
#else /* WIN32 */
    struct iovec msg_iov[2];
    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = msg_iov;
    msg.msg_iovlen = 2;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg_iov[0].iov_base = header;
    msg_iov[0].iov_len = sizeof(grn_com_gqtp_header);
    msg_iov[1].iov_base = body;
    msg_iov[1].iov_len = size;
    while ((errno = 0, (ret = sendmsg(cs->com.fd, &msg, MSG_NOSIGNAL)) == -1)) {
      SERR("sendmsg");
      if (errno == EAGAIN || errno == EINTR) { continue; }
      cs->rc = ctx->rc;
      goto exit;
    }
#endif /* WIN32 */
  } else {
    while ((errno = 0, (ret = send(cs->com.fd, header, whole_size, MSG_NOSIGNAL)) == -1)) {
#ifdef WIN32
      int e = WSAGetLastError();
      SERR("send");
      if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
      SERR("send");
      if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
      cs->rc = ctx->rc;
      goto exit;
    }
  }
  if (ret != whole_size) {
    GRN_LOG(ctx, GRN_LOG_ERROR, "sendmsg: %d < %d", ret, whole_size);
    cs->rc = ctx->rc;
    goto exit;
  }
  cs->rc = GRN_SUCCESS;
exit :
  return cs->rc;
}

grn_rc
grn_com_gqtp_recv(grn_ctx *ctx, grn_com_gqtp *cs, grn_obj *buf, unsigned int *status)
{
  ssize_t ret;
  grn_com_gqtp_header *header;
  size_t rest = sizeof(grn_com_gqtp_header);
  if (GRN_BULK_WSIZE(buf) < rest) {
    if ((cs->rc = grn_bulk_reinit(ctx, buf, rest))) {
      *status = grn_com_emem;
      goto exit;
    }
  } else {
    GRN_BULK_REWIND(buf);
  }
  do {
    // todo : also support non blocking mode (use MSG_DONTWAIT)
    errno = 0;
    if ((ret = recv(cs->com.fd, buf->u.b.curr, rest, MSG_WAITALL)) <= 0) {
#ifdef WIN32
        int e = WSAGetLastError();
        SERR("recv size");
        if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
        SERR("recv size");
        if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
      cs->rc = ctx->rc;
      *status = grn_com_erecv_head;
      goto exit;
    }
    rest -= ret, buf->u.b.curr += ret;
  } while (rest);
  header = GRN_COM_GQTP_MSG_HEADER(buf);
  GRN_LOG(ctx, GRN_LOG_INFO, "recv (%d,%x,%d,%02x,%02x,%04x)", ntohl(header->size), header->flags, header->proto, header->qtype, header->level, header->status);
  *status = header->status;
  {
    uint8_t proto = header->proto;
    size_t value_size = ntohl(header->size);
    size_t whole_size = sizeof(grn_com_gqtp_header) + value_size;
    switch (proto) {
    case GRN_COM_PROTO_GQTP :
    case GRN_COM_PROTO_MBREQ :
      if (GRN_BULK_WSIZE(buf) < whole_size) {
        if ((cs->rc = grn_bulk_resize(ctx, buf, whole_size))) {
          *status = grn_com_emem;
          goto exit;
        }
      }
      for (rest = value_size; rest;) {
        errno = 0;
        if ((ret = recv(cs->com.fd, buf->u.b.curr, rest, MSG_WAITALL)) <= 0) {
#ifdef WIN32
            int e = WSAGetLastError();
            SERR("recv body");
            if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
            SERR("recv body");
            if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
          cs->rc = ctx->rc;
          *status = grn_com_erecv_body;
          goto exit;
        }
        rest -= ret, buf->u.b.curr += ret;
      }
      *buf->u.b.curr = '\0';
      break;
    default :
      GRN_LOG(ctx, GRN_LOG_ERROR, "illegal header: %d", proto);
      cs->rc = GRN_INVALID_FORMAT;
      *status = grn_com_eproto;
      goto exit;
    }
  }
  cs->rc = GRN_SUCCESS;
exit :
  return cs->rc;
}

grn_com_gqtp *
grn_com_gqtp_copen(grn_ctx *ctx, grn_com_event *ev, const char *dest, int port)
{
  grn_sock fd = -1;
  grn_com_gqtp *cs = NULL;
  struct hostent *he;
  struct sockaddr_in addr;
  if (!(he = gethostbyname(dest))) {
    SERR("gethostbyname");
    goto exit;
  }
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, he->h_addr, he->h_length);
  addr.sin_port = htons(port);
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    SERR("socket");
    return NULL;
  }
  {
    int v = 1;
    if (setsockopt(fd, 6, TCP_NODELAY, (void *) &v, sizeof(int)) == -1) {
      SERR("setsockopt");
    }
  }
  while ((errno = 0, connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1)) {
#ifdef WIN32
    if (WSAGetLastError() == WSAECONNREFUSED)
#else /* WIN32 */
    if (errno == ECONNREFUSED)
#endif /* WIN32 */
    {
      GRN_LOG(ctx, GRN_LOG_NOTICE, "connect retrying..");
      sleep(2);
      continue;
    }
    SERR("connect");
    goto exit;
  }
  if (ev) {
    if (grn_com_event_add(ctx, ev, fd, GRN_COM_POLLIN, (grn_com **)&cs)) { goto exit; }
  } else {
    if (!(cs = GRN_CALLOC(sizeof(grn_com_gqtp)))) { goto exit; }
    cs->com.fd = fd;
  }
  grn_bulk_init(ctx, &cs->msg, 0);
exit :
  if (!cs) { grn_sock_close(fd); }
  return cs;
}

grn_rc
grn_com_close(grn_ctx *ctx, grn_com *com)
{
  grn_sock fd = com->fd;
  grn_com_event *ev = com->ev;
  if (ev) { grn_com_event_del(ctx, ev, fd); }
  if (com->status != grn_com_closed) {
    if (shutdown(fd, SHUT_RDWR) == -1) { /* SERR("shutdown"); */ }
    if (grn_sock_close(fd) == -1) {
      SERR("close");
      return ctx->rc;
    }
  }
  GRN_LOG(ctx, GRN_LOG_NOTICE, "closed (%d)", fd);
  if (!ev) { GRN_FREE(com); }
  return GRN_SUCCESS;
}

grn_rc
grn_com_recv(grn_ctx *ctx, grn_msg *msg)
{
  ssize_t ret;
  grn_com *com = msg->peer;
  grn_com_gqtp_header *header = &msg->header;
  byte *p = (byte *)header;
  size_t rest = sizeof(grn_com_gqtp_header);
  do {
    errno = 0;
    if ((ret = recv(com->fd, p, rest, MSG_WAITALL)) <= 0) {
#ifdef WIN32
        int e = WSAGetLastError();
        SERR("recv size");
        if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
        SERR("recv size");
        if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
      goto exit;
    }
    rest -= ret, p += ret;
  } while (rest);
  GRN_LOG(ctx, GRN_LOG_INFO, "recv (%d,%x,%d,%02x,%02x,%04x)", ntohl(header->size), header->flags, header->proto, header->qtype, header->level, header->status);
  {
    uint8_t proto = header->proto;
    size_t value_size = ntohl(header->size);
    grn_obj *buf = &msg->qe.obj;
    GRN_BULK_REWIND(buf);
    switch (proto) {
    case GRN_COM_PROTO_GQTP :
    case GRN_COM_PROTO_MBREQ :
      if (GRN_BULK_WSIZE(buf) < value_size) {
        if ((grn_bulk_resize(ctx, buf, value_size))) {
          goto exit;
        }
      }
      for (rest = value_size; rest;) {
        errno = 0;
        if ((ret = recv(com->fd, buf->u.b.curr, rest, MSG_WAITALL)) <= 0) {
#ifdef WIN32
            int e = WSAGetLastError();
            SERR("recv body");
            if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
            SERR("recv body");
            if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
          goto exit;
        }
        rest -= ret, buf->u.b.curr += ret;
      }
      //*buf->u.b.curr = '\0';
      break;
    default :
      GRN_LOG(ctx, GRN_LOG_ERROR, "illegal header: %d", proto);
      ctx->rc = GRN_INVALID_FORMAT;
      goto exit;
    }
  }
exit :
  return ctx->rc;
}

static void
grn_com_receiver(grn_ctx *ctx, grn_com *com)
{
  grn_com_event *ev = com->ev;
  grn_msg *msg = (grn_msg *)grn_msg_open(ctx, com, &ev->recv_old);
  /*
  if (com->status == grn_com_closing || com->status == grn_com_closed) {
    grn_com_close(ctx, com);
  }
  */
  grn_com_recv(ctx, msg);
  ev->msg_handler(ctx, (grn_obj *)msg);
}

static void
grn_com_gqtp_receiver(grn_ctx *ctx, grn_com_event *ev, grn_com *c)
{
  unsigned int status;
  grn_com_gqtp *cs = (grn_com_gqtp *)c;
  if (cs->com.status == grn_com_closing || cs->com.status == grn_com_closed) {
    grn_com_gqtp_close(ctx, ev, cs);
    return;
  }
  if (cs->com.status != grn_com_idle) {
    //    GRN_LOG(ctx, GRN_LOG_NOTICE, "waiting to be idle.. (%d) %d", c->fd, *ev->hash->n_entries);
    usleep(1);
    return;
  }
  grn_com_gqtp_recv(ctx, cs, &cs->msg, &status);
  cs->com.status = grn_com_doing;
  cs->msg_in(ctx, ev, c);
}

static void
grn_com_gqtp_acceptor(grn_ctx *ctx, grn_com_event *ev, grn_com *c)
{
  grn_com_gqtp *cs = (grn_com_gqtp *)c, *ncs;
  grn_sock fd = accept(cs->com.fd, NULL, NULL);
  if (fd == -1) {
    SERR("accept");
    return;
  }
  if (grn_com_event_add(ctx, ev, fd, GRN_COM_POLLIN, (grn_com **)&ncs)) {
    grn_sock_close(fd);
    return;
  }
  // GRN_LOG(ctx, GRN_LOG_NOTICE, "accepted (%d)", fd);
  ncs->com.ev_in = grn_com_gqtp_receiver;
  grn_bulk_init(ctx, &ncs->msg, 0);
  ncs->msg_in = cs->msg_in;
}

#define LISTEN_BACKLOG 0x1000

grn_com_gqtp *
grn_com_gqtp_sopen(grn_ctx *ctx, grn_com_event *ev, int port, grn_com_callback *func)
{
  grn_sock lfd;
  grn_com_gqtp *cs = NULL;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if ((lfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    SERR("socket");
    return NULL;
  }
  {
    int v = 1;
    if (setsockopt(lfd, 6, TCP_NODELAY, (void *) &v, sizeof(int)) == -1) {
      SERR("setsockopt");
      goto exit;
    }
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void *) &v, sizeof(int)) == -1) {
      SERR("setsockopt");
      goto exit;
    }
  }
  {
    int retry = 0;
    for (;;) {
      errno = 0;
      if (bind(lfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
#ifdef WIN32
        if (WSAGetLastError() == WSAEADDRINUSE)
#else /* WIN32 */
        if (errno == EADDRINUSE)
#endif /* WIN32 */
        {
          GRN_LOG(ctx, GRN_LOG_NOTICE, "bind retrying..(%d)", port);
          if (++retry < 10) { sleep(2); continue; }
        }
        SERR("bind");
        goto exit;
      }
      break;
    }
  }
  if (listen(lfd, LISTEN_BACKLOG) < 0) {
    SERR("listen");
    goto exit;
  }
  if (ev) {
    if (grn_com_event_add(ctx, ev, lfd, GRN_COM_POLLIN, (grn_com **)&cs)) { goto exit; }
  } else {
    if (!(cs = GRN_MALLOC(sizeof(grn_com_gqtp)))) { goto exit; }
    cs->com.fd = lfd;
  }
exit :
  if (cs) {
    cs->com.ev_in = grn_com_gqtp_acceptor;
    cs->msg_in = func;
  } else {
    grn_sock_close(lfd);
  }
  return cs;
}

grn_rc
grn_com_gqtp_close(grn_ctx *ctx, grn_com_event *ev, grn_com_gqtp *cs)
{
  grn_sock fd = cs->com.fd;
  grn_bulk_fin(ctx, &cs->msg);
  if (ev) {
    grn_com_event_del(ctx, ev, fd);
  } else {
    GRN_FREE(cs);
  }
  if (cs->com.status != grn_com_closed) {
    if (shutdown(fd, SHUT_RDWR) == -1) { /* SERR("shutdown"); */ }
    if (grn_sock_close(fd) == -1) {
      SERR("close");
      return ctx->rc;
    }
  }
  GRN_LOG(ctx, GRN_LOG_NOTICE, "closed (%d)", fd);
  return GRN_SUCCESS;
}

/* memcache binary protocol */

grn_rc
grn_com_mbres_send(grn_ctx *ctx, grn_com_gqtp *cs,
                   grn_com_gqtp_header *header, grn_obj *body,
                   uint16_t status, uint32_t key_size, uint32_t extra_size)
{
  uint32_t size = GRN_BULK_VSIZE(body);
  ssize_t ret, whole_size = sizeof(grn_com_gqtp_header) + size;
  header->proto = GRN_COM_PROTO_MBRES;
  header->keylen = htons(key_size);
  header->level = extra_size;
  header->flags = 0x00;
  header->status = htons(status);
  header->size = htonl(size);
  if (size) {
#ifdef WIN32
    ssize_t reth;
    if ((reth = send(cs->com.fd, header, sizeof(grn_com_gqtp_header), 0)) == -1) {
      SERR("send size");
      cs->rc = ctx->rc;
      goto exit;
    }
    if ((ret = send(cs->com.fd, GRN_BULK_HEAD(body), size, 0)) == -1) {
      SERR("send body");
      cs->rc = ctx->rc;
      goto exit;
    }
    ret += reth;
#else /* WIN32 */
    struct iovec msg_iov[2];
    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = msg_iov;
    msg.msg_iovlen = 2;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg_iov[0].iov_base = header;
    msg_iov[0].iov_len = sizeof(grn_com_gqtp_header);
    msg_iov[1].iov_base = GRN_BULK_HEAD(body);
    msg_iov[1].iov_len = size;
    while ((errno = 0, (ret = sendmsg(cs->com.fd, &msg, MSG_NOSIGNAL)) == -1)) {
      SERR("sendmsg");
      if (errno == EAGAIN || errno == EINTR) { continue; }
      cs->rc = ctx->rc;
      goto exit;
    }
#endif /* WIN32 */
  } else {
    errno = 0;
    while ((ret = send(cs->com.fd, header, whole_size, MSG_NOSIGNAL)) == -1) {
#ifdef WIN32
      int e = WSAGetLastError();
      SERR("send");
      if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
      SERR("send");
      if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
      cs->rc = ctx->rc;
      goto exit;
    }
  }
  if (ret != whole_size) {
    GRN_LOG(ctx, GRN_LOG_ERROR, "sendmsg: %d < %d", ret, whole_size);
    cs->rc = ctx->rc;
    goto exit;
  }
  cs->rc = GRN_SUCCESS;
exit :
  return cs->rc;
}
