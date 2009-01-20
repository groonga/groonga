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

#ifdef WIN32
void
log_sockerr(const char *msg)
{
  int e = WSAGetLastError();
  const char *m;
  switch (e) {
  case WSANOTINITIALISED: m = "please call grn_com_init first"; break;
  case WSAEFAULT: m = "bad address"; break;
  case WSAEINVAL: m = "invalid argument"; break;
  case WSAEMFILE: m = "too many sockets"; break;
  case WSAEWOULDBLOCK: m = "operation would block"; break;
  case WSAENOTSOCK: m = "given fd is not socket fd"; break;
  case WSAEOPNOTSUPP: m = "operation is not supported"; break;
  case WSAEADDRINUSE: m = "address is already in use"; break;
  case WSAEADDRNOTAVAIL: m = "address is not available"; break;
  case WSAENETDOWN: m = "network is down"; break;
  case WSAENOBUFS: m = "no buffer"; break;
  case WSAEISCONN: m = "socket is already connected"; break;
  case WSAENOTCONN: m = "socket is not connected"; break;
  case WSAESHUTDOWN: m = "socket is already shutdowned"; break;
  case WSAETIMEDOUT: m = "connection time out"; break;
  case WSAECONNREFUSED: m = "connection refused"; break;
  default:
    GRN_LOG(grn_log_error, "%s: socket error (%d)", msg, e);
    return;
  }
  GRN_LOG(grn_log_error, "%s: %s", msg, m);
}
#define LOG_SOCKERR(m) log_sockerr((m))
#else /* WIN32 */
#define LOG_SOCKERR(m) GRN_LOG(grn_log_error, "%s: %s", (m), strerror(errno))
#endif /* WIN32 */

/******* grn_com ********/

grn_rc
grn_com_init(void)
{
#ifdef WIN32
  WSADATA wd;
  if (WSAStartup(MAKEWORD(2, 0), &wd) != 0) {
    GERR(grn_external_error, "WSAStartup failed");
  }
#else /* WIN32 */
#ifndef USE_MSG_NOSIGNAL
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    GERR(grn_external_error, "signal");
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
  grn_rc rc = GRN_NO_MEMORY_AVAILABLE;
  ev->max_nevents = max_nevents;
  if ((ev->hash = grn_hash_create(ctx, NULL, sizeof(grn_sock), data_size, 0, grn_enc_none))) {
#ifndef USE_SELECT
#ifdef USE_EPOLL
    if ((ev->events = GRN_MALLOC(sizeof(struct epoll_event) * max_nevents))) {
      if ((ev->epfd = epoll_create(max_nevents)) != -1) {
        rc = GRN_SUCCESS;
        goto exit;
      } else {
        LOG_SOCKERR("epoll_create");
        rc = grn_external_error;
      }
      GRN_FREE(ev->events);
    }
#else /* USE_EPOLL */
    if ((ev->events = GRN_MALLOC(sizeof(struct pollfd) * max_nevents))) {
      rc = GRN_SUCCESS;
      goto exit;
    }
#endif /* USE_EPOLL */
    grn_hash_close(ctx, ev->hash);
    ev->hash = NULL;
    ev->events = NULL;
#else /* USE_SELECT */
    rc = GRN_SUCCESS;
#endif /* USE_SELECT */
  }
exit :
  return rc;
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
    if (ev) { GRN_LOG(grn_log_error, "too many connections (%d)", ev->max_nevents); }
    return GRN_INVALID_ARGUMENT;
  }
#ifdef USE_EPOLL
  {
    struct epoll_event e;
    memset(&e, 0, sizeof(struct epoll_event));
    e.data.fd = (fd);
    e.events = (__uint32_t) events;
    if (epoll_ctl(ev->epfd, EPOLL_CTL_ADD, (fd), &e) == -1) {
      LOG_SOCKERR("epoll_ctl");
      return grn_external_error;
    }
  }
#endif /* USE_EPOLL*/
  {
    grn_search_flags f = GRN_TABLE_ADD;
    if (grn_hash_get(ctx, ev->hash, &fd, sizeof(grn_sock), (void **)&c, &f)) {
      c->fd = fd;
      c->events = events;
      if (com) { *com = c; }
      return GRN_SUCCESS;
    }
  }
  return grn_internal_error;
}

grn_rc
grn_com_event_mod(grn_ctx *ctx, grn_com_event *ev, grn_sock fd, int events, grn_com **com)
{
  grn_com *c;
  if (!ev) { return GRN_INVALID_ARGUMENT; }
  if (grn_hash_at(ctx, ev->hash, &fd, sizeof(grn_sock), (void **)&c)) {
    if (c->fd != fd) {
      GRN_LOG(grn_log_error, "grn_com_event_mod fd unmatch %d != %d", c->fd, fd);
      return grn_invalid_format;
    }
    if (com) { *com = c; }
    if (c->events != events) {
#ifdef USE_EPOLL
      struct epoll_event e;
      memset(&e, 0, sizeof(struct epoll_event));
      e.data.fd = (fd);
      e.events = (__uint32_t) events;
      if (epoll_ctl(ev->epfd, EPOLL_CTL_MOD, (fd), &e) == -1) {
        LOG_SOCKERR("epoll_ctl");
        return grn_external_error;
      }
#endif /* USE_EPOLL*/
      c->events = events;
    }
    return GRN_SUCCESS;
  }
  return grn_internal_error;
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
        LOG_SOCKERR("epoll_ctl");
        return grn_external_error;
      }
#endif /* USE_EPOLL*/
      return grn_hash_delete_by_id(ctx, ev->hash, id, NULL);
    } else {
      GRN_LOG(grn_log_error, "%04x| fd(%d) not found in ev(%p)", getpid(), fd, ev);
      return grn_internal_error;
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
  GRN_HASH_EACH(ev->hash, eh, &pfd, &dummy, &com, {
    if ((com->events & GRN_COM_POLLIN)) { FD_SET(*pfd, &rfds); }
    if ((com->events & GRN_COM_POLLOUT)) { FD_SET(*pfd, &wfds); }
    if (*pfd > nfds) { nfds = *pfd; }
  });
  nevents = select(nfds + 1, &rfds, &wfds, NULL, (timeout >= 0) ? &tv : NULL);
  if (nevents < 0) {
#ifdef WIN32
    if (WSAGetLastError() == WSAEINTR) { return GRN_SUCCESS; }
#else /* WIN32 */
    if (errno == EINTR) { return GRN_SUCCESS; }
#endif /* WIN32 */
    LOG_SOCKERR("select");
    return grn_external_error;
  }
  if (timeout < 0 && !nevents) { GRN_LOG(grn_log_notice, "select returns 0 events"); }
  GRN_HASH_EACH(ev->hash, eh, &pfd, &dummy, &com, {
    if (FD_ISSET(*pfd, &rfds)) { com->ev_in(ev, com); }
    if (FD_ISSET(*pfd, &wfds)) { com->ev_out(ev, com); }
  });
#else /* USE_SELECT */
#ifdef USE_EPOLL
  struct epoll_event *ep;
  nevents = epoll_wait(ev->epfd, ev->events, ev->max_nevents, timeout);
#else /* USE_EPOLL */
  int nfd = 0, *pfd;
  struct pollfd *ep = ev->events;
  GRN_HASH_EACH(ev->hash, eh, &pfd, &dummy, &com, {
    ep->fd = *pfd;
    //    ep->events =(short) com->events;
    ep->events = POLLIN;
    ep->revents = 0;
    ep++;
    nfd++;
  });
  nevents = poll(ev->events, nfd, timeout);
#endif /* USE_EPOLL */
  if (nevents < 0) {
    if (errno == EINTR) { return GRN_SUCCESS; }
    LOG_SOCKERR("poll");
    return grn_external_error;
  }
  if (timeout < 0 && !nevents) { GRN_LOG(grn_log_notice, "poll returns 0 events"); }
  for (ep = ev->events; nevents; ep++) {
    int efd;
#ifdef USE_EPOLL
    efd = ep->data.fd;
    nevents--;
    if (!grn_hash_at(ctx, ev->hash, &efd, sizeof(grn_sock), (void *)&com)) {
      struct epoll_event e;
      GRN_LOG(grn_log_error, "fd(%d) not found in ev->hash", efd);
      memset(&e, 0, sizeof(struct epoll_event));
      e.data.fd = efd;
      e.events = ep->events;
      if (epoll_ctl(ev->epfd, EPOLL_CTL_DEL, efd, &e) == -1) { LOG_SOCKERR("epoll_ctl"); }
      if (grn_sock_close(efd) == -1) { LOG_SOCKERR("close"); }
      continue;
    }
    if ((ep->events & GRN_COM_POLLIN)) { com->ev_in(ctx, ev, com); }
    if ((ep->events & GRN_COM_POLLOUT)) { com->ev_out(ctx, ev, com); }
#else /* USE_EPOLL */
    efd = ep->fd;
    if (!(ep->events & ep->revents)) { continue; }
    nevents--;
    if (!grn_hash_at(ctx, ev->hash, &efd, sizeof(grn_sock), (void *)&com)) {
      GRN_LOG(grn_log_error, "fd(%d) not found in ev->hash", efd);
      if (grn_sock_close(efd) == -1) { LOG_SOCKERR("close"); }
      continue;
    }
    if ((ep->revents & GRN_COM_POLLIN)) { com->ev_in(ctx, ev, com); }
    if ((ep->revents & GRN_COM_POLLOUT)) { com->ev_out(ctx, ev, com); }
#endif /* USE_EPOLL */
  }
#endif /* USE_SELECT */
  return GRN_SUCCESS;
}

/******* grn_com_sqtp ********/

grn_rc
grn_com_sqtp_send(grn_ctx *ctx, grn_com_sqtp *cs, grn_com_sqtp_header *header, char *body)
{
  ssize_t ret, whole_size = sizeof(grn_com_sqtp_header) + header->size;
  header->proto = GRN_COM_PROTO_SQTP;
  if (cs->com.status == grn_com_closing) { header->flags |= GRN_QL_QUIT; }
  GRN_LOG(grn_log_info, "send (%d,%x,%d,%02x,%02x,%04x,%08x)", header->size, header->flags, header->proto, header->qtype, header->level, header->status, header->info);

  if (header->size) {
#ifdef WIN32
    ssize_t reth;
    if ((reth = send(cs->com.fd, header, sizeof(grn_com_sqtp_header), 0)) == -1) {
      LOG_SOCKERR("send size");
      cs->rc = grn_external_error;
      goto exit;
    }
    if ((ret = send(cs->com.fd, body, header->size, 0)) == -1) {
      LOG_SOCKERR("send body");
      cs->rc = grn_external_error;
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
    msg_iov[0].iov_len = sizeof(grn_com_sqtp_header);
    msg_iov[1].iov_base = body;
    msg_iov[1].iov_len = header->size;
    while ((ret = sendmsg(cs->com.fd, &msg, MSG_NOSIGNAL)) == -1) {
      LOG_SOCKERR("sendmsg");
      if (errno == EAGAIN || errno == EINTR) { continue; }
      cs->rc = grn_external_error;
      goto exit;
    }
#endif /* WIN32 */
  } else {
    while ((ret = send(cs->com.fd, header, whole_size, MSG_NOSIGNAL)) == -1) {
#ifdef WIN32
      int e = WSAGetLastError();
      LOG_SOCKERR("send");
      if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
#else /* WIN32 */
      LOG_SOCKERR("send");
      if (errno == EAGAIN || errno == EINTR) { continue; }
#endif /* WIN32 */
      cs->rc = grn_external_error;
      goto exit;
    }
  }
  if (ret != whole_size) {
    GRN_LOG(grn_log_error, "sendmsg: %d < %d", ret, whole_size);
    cs->rc = grn_external_error;
    goto exit;
  }
  cs->rc = GRN_SUCCESS;
exit :
  return cs->rc;
}

grn_rc
grn_com_sqtp_recv(grn_ctx *ctx, grn_com_sqtp *cs, grn_obj *buf,
                  unsigned int *status, unsigned int *info)
{
  ssize_t ret;
  grn_com_sqtp_header *header;
  size_t rest = sizeof(grn_com_sqtp_header);
  if (GRN_BULK_WSIZE(buf) < rest) {
    if ((cs->rc = grn_bulk_reinit(ctx, buf, rest))) {
      *status = grn_com_emem; *info = 1; goto exit;
    }
  } else {
    GRN_BULK_REWIND(buf);
  }
  do {
    // todo : also support non blocking mode (use MSG_DONTWAIT)
    if ((ret = recv(cs->com.fd, buf->u.b.curr, rest, MSG_WAITALL)) <= 0) {
      if (ret < 0) {
#ifdef WIN32
        int e = WSAGetLastError();
        LOG_SOCKERR("recv size");
        if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
        *info = e;
#else /* WIN32 */
        LOG_SOCKERR("recv size");
        if (errno == EAGAIN || errno == EINTR) { continue; }
        *info = errno;
#endif /* WIN32 */
      }
      cs->rc = grn_external_error;
      *status = grn_com_erecv_head;
      goto exit;
    }
    rest -= ret, buf->u.b.curr += ret;
  } while (rest);
  header = GRN_COM_SQTP_MSG_HEADER(buf);
  GRN_LOG(grn_log_info, "recv (%d,%x,%d,%02x,%02x,%04x,%08x)", header->size, header->flags, header->proto, header->qtype, header->level, header->status, header->info);
  *status = header->status;
  *info = header->info;
  {
    uint16_t proto = header->proto;
    size_t value_size = header->size;
    size_t whole_size = sizeof(grn_com_sqtp_header) + value_size;
    if (proto != GRN_COM_PROTO_SQTP) {
      GRN_LOG(grn_log_error, "illegal header: %d", proto);
      cs->rc = grn_invalid_format;
      *status = grn_com_eproto;
      *info = proto;
      goto exit;
    }
    if (GRN_BULK_WSIZE(buf) < whole_size) {
      if ((cs->rc = grn_bulk_resize(ctx, buf, whole_size))) {
        *status = grn_com_emem; *info = 2;
        goto exit;
      }
    }
    for (rest = value_size; rest;) {
      if ((ret = recv(cs->com.fd, buf->u.b.curr, rest, MSG_WAITALL)) <= 0) {
        if (ret < 0) {
#ifdef WIN32
          int e = WSAGetLastError();
          LOG_SOCKERR("recv body");
          if (e == WSAEWOULDBLOCK || e == WSAEINTR) { continue; }
          *info = e;
#else /* WIN32 */
          LOG_SOCKERR("recv body");
          if (errno == EAGAIN || errno == EINTR) { continue; }
          *info = errno;
#endif /* WIN32 */
        }
        cs->rc = grn_external_error;
        *status = grn_com_erecv_body;
        goto exit;
      }
      rest -= ret, buf->u.b.curr += ret;
    }
    *buf->u.b.curr = '\0';
  }
  cs->rc = GRN_SUCCESS;
exit :
  return cs->rc;
}

grn_com_sqtp *
grn_com_sqtp_copen(grn_ctx *ctx, grn_com_event *ev, const char *dest, int port)
{
  grn_sock fd;
  grn_com_sqtp *cs = NULL;
  struct hostent *he;
  struct sockaddr_in addr;
  if (!(he = gethostbyname(dest))) {
    LOG_SOCKERR("gethostbyname");
    goto exit;
  }
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, he->h_addr, he->h_length);
  addr.sin_port = htons(port);
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_SOCKERR("socket");
    goto exit;
  }
  {
    int v = 1;
    if (setsockopt(fd, 6, TCP_NODELAY, (void *) &v, sizeof(int)) == -1) {
      LOG_SOCKERR("setsockopt");
    }
  }
  while (connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
#ifdef WIN32
    if (WSAGetLastError() == WSAECONNREFUSED)
#else /* WIN32 */
    if (errno == ECONNREFUSED)
#endif /* WIN32 */
    {
      GRN_LOG(grn_log_notice, "connect retrying..");
      sleep(2);
      continue;
    }
    LOG_SOCKERR("connect");
    goto exit;
  }
  if (ev) {
    if (grn_com_event_add(ctx, ev, fd, GRN_COM_POLLIN, (grn_com **)&cs)) { goto exit; }
  } else {
    if (!(cs = GRN_CALLOC(sizeof(grn_com_sqtp)))) { goto exit; }
    cs->com.fd = fd;
  }
  grn_bulk_init(ctx, &cs->msg, 0);
exit :
  if (!cs) { grn_sock_close(fd); }
  return cs;
}

static void
grn_com_sqtp_receiver(grn_ctx *ctx, grn_com_event *ev, grn_com *c)
{
  unsigned int status, info;
  grn_com_sqtp *cs = (grn_com_sqtp *)c;
  if (cs->com.status == grn_com_closing) {
    grn_com_sqtp_close(ctx, ev, cs);
    return;
  }
  if (cs->com.status != grn_com_idle) {
    GRN_LOG(grn_log_info, "waiting to be idle.. (%d) %d", c->fd, *ev->hash->n_entries);
    usleep(1000);
    return;
  }
  grn_com_sqtp_recv(ctx, cs, &cs->msg, &status, &info);
  cs->com.status = grn_com_doing;
  cs->msg_in(ctx, ev, c);
}

static void
grn_com_sqtp_acceptor(grn_ctx *ctx, grn_com_event *ev, grn_com *c)
{
  grn_com_sqtp *cs = (grn_com_sqtp *)c, *ncs;
  grn_sock fd = accept(cs->com.fd, NULL, NULL);
  if (fd == -1) {
    LOG_SOCKERR("accept");
    return;
  }
  if (grn_com_event_add(ctx, ev, fd, GRN_COM_POLLIN, (grn_com **)&ncs)) {
    grn_sock_close(fd);
    return;
  }
  ncs->com.ev_in = grn_com_sqtp_receiver;
  grn_bulk_init(ctx, &ncs->msg, 0);
  ncs->msg_in = cs->msg_in;
}

#define LISTEN_BACKLOG 0x1000

grn_com_sqtp *
grn_com_sqtp_sopen(grn_ctx *ctx, grn_com_event *ev, int port, grn_com_callback *func)
{
  grn_sock lfd;
  grn_com_sqtp *cs = NULL;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if ((lfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_SOCKERR("socket");
    return NULL;
  }
  {
    int v = 1;
    if (setsockopt(lfd, 6, TCP_NODELAY, (void *) &v, sizeof(int)) == -1) {
      LOG_SOCKERR("setsockopt");
      goto exit;
    }
  }
  {
    int retry = 0;
    for (;;) {
      if (bind(lfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
#ifdef WIN32
        if (WSAGetLastError() == WSAEADDRINUSE)
#else /* WIN32 */
        if (errno == EADDRINUSE)
#endif /* WIN32 */
        {
          GRN_LOG(grn_log_notice, "bind retrying..(%d)", port);
          if (++retry < 10) { sleep(2); continue; }
        }
        LOG_SOCKERR("bind");
        goto exit;
      }
      break;
    }
  }
  if (listen(lfd, LISTEN_BACKLOG) < 0) {
    LOG_SOCKERR("listen");
    goto exit;
  }
  if (ev) {
    if (grn_com_event_add(ctx, ev, lfd, GRN_COM_POLLIN, (grn_com **)&cs)) { goto exit; }
  } else {
    if (!(cs = GRN_MALLOC(sizeof(grn_com_sqtp)))) { goto exit; }
    cs->com.fd = lfd;
  }
exit :
  if (cs) {
    cs->com.ev_in = grn_com_sqtp_acceptor;
    cs->msg_in = func;
  } else {
    grn_sock_close(lfd);
  }
  return cs;
}

grn_rc
grn_com_sqtp_close(grn_ctx *ctx, grn_com_event *ev, grn_com_sqtp *cs)
{
  grn_sock fd = cs->com.fd;
  grn_bulk_fin(ctx, &cs->msg);
  if (ev) {
    grn_com_event_del(ctx, ev, fd);
  } else {
    GRN_FREE(cs);
  }
  if (shutdown(fd, SHUT_RDWR) == -1) { /* LOG_SOCKERR("shutdown"); */ }
  if (grn_sock_close(fd) == -1) {
    LOG_SOCKERR("close");
    return grn_external_error;
  }
  return GRN_SUCCESS;
}
