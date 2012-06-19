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

#ifdef WIN32
# define GROONGA_MAIN
#endif /* WIN32 */
#include "lib/groonga_in.h"

#include "lib/com.h"
#include "lib/ctx_impl.h"
#include "lib/proc.h"
#include "lib/db.h"
#include "lib/util.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif /* HAVE_SYS_SYSCTL_H */

#ifndef USE_MSG_NOSIGNAL
# ifdef MSG_NOSIGNAL
#  undef MSG_NOSIGNAL
# endif
# define MSG_NOSIGNAL 0
#endif /* USE_MSG_NOSIGNAL */

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif /* STDIN_FILENO */
#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif /* STDOUT_FILENO */
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif /* STDERR_FILENO */

#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"
#define DEFAULT_MAX_NFTHREADS 8
#define MAX_CON 0x10000

#define RLIMIT_NOFILE_MINIMUM 4096

static char bind_address[HOST_NAME_MAX + 1];
static char hostname[HOST_NAME_MAX + 1];
static int port = DEFAULT_PORT;
static int batchmode;
static int number_of_lines = 0;
static int newdb;
static int useql;
static grn_bool is_daemon_mode = GRN_FALSE;
static int (*do_client)(int argc, char **argv);
static int (*do_server)(char *path);
static FILE *pid_file = NULL;
static const char *pid_file_path = NULL;
static const char *input_path = NULL;
static FILE *output = NULL;

static grn_encoding encoding;
static grn_command_version default_command_version;
static int64_t default_match_escalation_threshold;
static int log_level;
static uint32_t cache_limit;

static int
grn_rc_to_exit_code(grn_rc rc)
{
  if (rc == GRN_SUCCESS) {
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}

#ifdef WITH_LIBEDIT
#include <locale.h>
#include <histedit.h>
static EditLine   *line_editor = NULL;
static HistoryW   *line_editor_history = NULL;
static HistEventW line_editor_history_event;
static char       line_editor_history_path[PATH_MAX] = "";

static const wchar_t *
line_editor_prompt(EditLine *e __attribute__((unused)))
{
  return L"> ";
}
static const wchar_t * const line_editor_editor = L"emacs";

static void
line_editor_init(int argc __attribute__((unused)), char *argv[])
{
  const char * const HOME_PATH = getenv("HOME");
  const char * const HISTORY_PATH = "/.groonga-history";

  setlocale(LC_ALL, "");

  if (strlen(HOME_PATH) + strlen(HISTORY_PATH) < PATH_MAX) {
    strcpy(line_editor_history_path, HOME_PATH);
    strcat(line_editor_history_path, HISTORY_PATH);
  } else {
    line_editor_history_path[0] = '\0';
  }

  line_editor_history = history_winit();
  history_w(line_editor_history, &line_editor_history_event, H_SETSIZE, 200);
  if (line_editor_history_path[0]) {
    history_w(line_editor_history, &line_editor_history_event,
              H_LOAD, line_editor_history_path);
  }

  line_editor = el_init(argv[0], stdin, stdout, stderr);
  el_wset(line_editor, EL_PROMPT, &line_editor_prompt);
  el_wset(line_editor, EL_EDITOR, line_editor_editor);
  el_wset(line_editor, EL_HIST, history_w, line_editor_history);
  el_source(line_editor, NULL);
}

static void
line_editor_fin(void)
{
  if (line_editor) {
    el_end(line_editor);
    if (line_editor_history) {
      if (line_editor_history_path[0]) {
        history_w(line_editor_history, &line_editor_history_event,
                  H_SAVE, line_editor_history_path);
      }
      history_wend(line_editor_history);
    }
  }
}

static grn_rc
line_editor_fgets(grn_ctx *ctx, grn_obj *buf)
{
  grn_rc rc = GRN_SUCCESS;
  const wchar_t *line;
  int nchar;
  line = el_wgets(line_editor, &nchar);
  if (nchar > 0) {
    int i;
    char multibyte_buf[MB_CUR_MAX];
    size_t multibyte_len;
    mbstate_t ps;
    history_w(line_editor_history, &line_editor_history_event, H_ENTER, line);
    memset(&ps, 0, sizeof(ps));
    wcrtomb(NULL, L'\0', &ps);
    for (i = 0; i < nchar; i++) {
      multibyte_len = wcrtomb(multibyte_buf, line[i], &ps);
      if (multibyte_len == (size_t)-1) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "[prompt][libedit] failed to read input: %s", strerror(errno));
        rc = GRN_INVALID_ARGUMENT;
      } else {
        GRN_TEXT_PUT(ctx, buf, multibyte_buf, multibyte_len);
      }
    }
  } else {
    rc = GRN_END_OF_DATA;
  }
  return rc;
}
#endif /* WITH_LIBEDIT */

inline static grn_rc
prompt(grn_ctx *ctx, grn_obj *buf)
{
  static int the_first_read = GRN_TRUE;
  grn_rc rc = GRN_SUCCESS;
  GRN_BULK_REWIND(buf);
  if (!batchmode) {
#ifdef WITH_LIBEDIT
    rc = line_editor_fgets(ctx, buf);
#else
    fprintf(stderr, "> ");
    rc = grn_text_fgets(ctx, buf, stdin);
#endif
  } else {
    rc = grn_text_fgets(ctx, buf, stdin);
    if (rc != GRN_END_OF_DATA) {
      number_of_lines++;
    }
  }
  if (the_first_read && GRN_TEXT_LEN(buf) > 0) {
    const char bom[] = {0xef, 0xbb, 0xbf};
    if (GRN_CTX_GET_ENCODING(ctx) == GRN_ENC_UTF8 &&
        GRN_TEXT_LEN(buf) > 3 && !memcmp(GRN_TEXT_VALUE(buf), bom, 3)) {
      grn_obj buf_without_bom;
      GRN_TEXT_INIT(&buf_without_bom, 0);
      GRN_TEXT_PUT(ctx, &buf_without_bom,
                   GRN_TEXT_VALUE(buf) + 3, GRN_TEXT_LEN(buf) - 3);
      GRN_TEXT_SET(ctx, buf,
                   GRN_TEXT_VALUE(&buf_without_bom),
                   GRN_TEXT_LEN(&buf_without_bom));
      grn_obj_unlink(ctx, &buf_without_bom);
    }
    the_first_read = GRN_FALSE;
  }
  if (GRN_TEXT_LEN(buf) > 0 &&
      GRN_TEXT_VALUE(buf)[GRN_TEXT_LEN(buf) - 1] == '\n') {
    grn_bulk_truncate(ctx, buf, GRN_TEXT_LEN(buf) - 1);
  }
  if (GRN_TEXT_LEN(buf) > 0 &&
      GRN_TEXT_VALUE(buf)[GRN_TEXT_LEN(buf) - 1] == '\r') {
    grn_bulk_truncate(ctx, buf, GRN_TEXT_LEN(buf) - 1);
  }
  return rc;
}

typedef enum {
  grn_http_request_type_none = 0,
  grn_http_request_type_get,
  grn_http_request_type_post
} grn_http_request_type;

static void
output_envelope(grn_ctx *ctx, grn_rc rc, grn_obj *head, grn_obj *body, grn_obj *foot)
{
  grn_output_envelope(ctx, rc, head, body, foot, input_path, number_of_lines);
}

static void
s_output(grn_ctx *ctx, int flags, void *arg)
{
  if (ctx && ctx->impl && (flags & GRN_CTX_TAIL)) {
    grn_obj *buf = ctx->impl->outbuf;
    grn_obj *command;
    if (GRN_TEXT_LEN(buf) || ctx->rc) {
      FILE * stream = (FILE *) arg;
      grn_obj head, foot;
      GRN_TEXT_INIT(&head, 0);
      GRN_TEXT_INIT(&foot, 0);
      output_envelope(ctx, ctx->rc, &head, buf, &foot);
      fwrite(GRN_TEXT_VALUE(&head), 1, GRN_TEXT_LEN(&head), stream);
      fwrite(GRN_TEXT_VALUE(buf), 1, GRN_TEXT_LEN(buf), stream);
      fwrite(GRN_TEXT_VALUE(&foot), 1, GRN_TEXT_LEN(&foot), stream);
      fputc('\n', stream);
      fflush(stream);
      GRN_BULK_REWIND(buf);
      GRN_OBJ_FIN(ctx, &head);
      GRN_OBJ_FIN(ctx, &foot);
    }
    command = GRN_CTX_USER_DATA(ctx)->ptr;
    GRN_BULK_REWIND(command);
  }
}

static int
do_alone(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;
  char *path = NULL;
  grn_obj *db;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, (useql ? GRN_CTX_USE_QL : 0)|(batchmode ? GRN_CTX_BATCH_MODE : 0));
  if (argc > 0 && argv) { path = *argv++; argc--; }
  db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
  if (db) {
    grn_obj command;
    GRN_TEXT_INIT(&command, 0);
    GRN_CTX_USER_DATA(ctx)->ptr = &command;
    grn_ctx_recv_handler_set(ctx, s_output, output);
    if (!argc) {
      grn_obj text;
      GRN_TEXT_INIT(&text, 0);
      while (prompt(ctx, &text) != GRN_END_OF_DATA) {
        GRN_TEXT_PUT(ctx, &command, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text));
        grn_ctx_send(ctx, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text), 0);
        if (ctx->stat == GRN_CTX_QUIT) { break; }
      }
      exit_code = grn_rc_to_exit_code(ctx->rc);
      grn_obj_unlink(ctx, &text);
    } else {
      grn_rc rc;
      rc = grn_ctx_sendv(ctx, argc, argv, 0);
      exit_code = grn_rc_to_exit_code(rc);
    }
    grn_obj_unlink(ctx, &command);
    grn_obj_close(ctx, db);
  } else {
    fprintf(stderr, "db open failed (%s): %s\n", path, ctx->errbuf);
  }
  grn_ctx_fin(ctx);
  return exit_code;
}

static int
c_output(grn_ctx *ctx)
{
  int flags;
  char *str;
  unsigned int str_len;
  do {
    grn_ctx_recv(ctx, &str, &str_len, &flags);
    /*
    if (ctx->rc) {
      fprintf(stderr, "grn_ctx_recv failed\n");
      return -1;
    }
    */
    if (str_len || ctx->rc) {
      grn_obj head, body, foot;
      GRN_TEXT_INIT(&head, 0);
      GRN_TEXT_INIT(&body, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_INIT(&foot, 0);
      GRN_TEXT_SET(ctx, &body, str, str_len);
      output_envelope(ctx, ctx->rc, &head, &body, &foot);
      fwrite(GRN_TEXT_VALUE(&head), 1, GRN_TEXT_LEN(&head), output);
      fwrite(GRN_TEXT_VALUE(&body), 1, GRN_TEXT_LEN(&body), output);
      fwrite(GRN_TEXT_VALUE(&foot), 1, GRN_TEXT_LEN(&foot), output);
      fputc('\n', output);
      fflush(output);
      GRN_OBJ_FIN(ctx, &head);
      GRN_OBJ_FIN(ctx, &body);
      GRN_OBJ_FIN(ctx, &foot);
    }
  } while ((flags & GRN_CTX_MORE));
  return 0;
}

static int
g_client(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;
  grn_ctx ctx_, *ctx = &ctx_;
  const char *hostname = DEFAULT_DEST;
  if (argc > 0 && argv) { hostname = *argv++; argc--; }
  grn_ctx_init(ctx, (batchmode ? GRN_CTX_BATCH_MODE : 0));
  if (!grn_ctx_connect(ctx, hostname, port, 0)) {
    if (!argc) {
      grn_obj text;
      GRN_TEXT_INIT(&text, 0);
      while (prompt(ctx, &text) != GRN_END_OF_DATA) {
        grn_ctx_send(ctx, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text), 0);
        exit_code = grn_rc_to_exit_code(ctx->rc);
        if (ctx->rc != GRN_SUCCESS) { break; }
        if (c_output(ctx)) { goto exit; }
        if (ctx->stat == GRN_CTX_QUIT) { break; }
      }
      grn_obj_unlink(ctx, &text);
    } else {
      grn_rc rc;
      rc = grn_ctx_sendv(ctx, argc, argv, 0);
      exit_code = grn_rc_to_exit_code(rc);
      if (c_output(ctx)) { goto exit; }
    }
  } else {
    fprintf(stderr, "grn_ctx_connect failed (%s:%d)\n", hostname, port);
  }
exit :
  grn_ctx_fin(ctx);
  return exit_code;
}

/* server */

typedef void (*grn_edge_dispatcher_func)(grn_ctx *ctx, grn_edge *edge);
typedef void (*grn_handler_func)(grn_ctx *ctx, grn_obj *msg);

static grn_com_queue ctx_new;
static grn_com_queue ctx_old;
static grn_mutex q_mutex;
static grn_cond q_cond;
static uint32_t nthreads = 0, nfthreads = 0, max_nfthreads;

static int
daemonize(void)
{
  int exit_code = EXIT_SUCCESS;
#ifndef WIN32
  pid_t pid;

  switch (fork()) {
  case 0:
    break;
  case -1:
    perror("fork");
    return EXIT_FAILURE;
  default:
    wait(NULL);
    _exit(EXIT_SUCCESS);
  }
  switch (fork()) {
  case 0:
    if (pid_file_path) {
      pid_file = fopen(pid_file_path, "w");
    }
    pid = getpid();
    if (!pid_file) {
      fprintf(stderr, "%d\n", pid);
    } else {
      fprintf(pid_file, "%d\n", pid);
      fclose(pid_file);
      pid_file = NULL;
    }
    break;
  case -1:
    perror("fork");
    return EXIT_FAILURE;
  default:
    _exit(EXIT_SUCCESS);
  }
  {
    int null_fd = GRN_OPEN("/dev/null", O_RDWR, 0);
    if (null_fd != -1) {
      dup2(null_fd, STDIN_FILENO);
      dup2(null_fd, STDOUT_FILENO);
      dup2(null_fd, STDERR_FILENO);
      if (null_fd > STDERR_FILENO) { GRN_CLOSE(null_fd); }
    }
  }
#endif /* WIN32 */
  return exit_code;
}

static void
clean_pid_file(void)
{
#ifndef WIN32
  if (pid_file_path) {
    unlink(pid_file_path);
  }
#endif
}

static void
run_server_loop(grn_ctx *ctx, grn_com_event *ev)
{
  while (!grn_com_event_poll(ctx, ev, 1000) && grn_gctx.stat != GRN_CTX_QUIT) {
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
      grn_edges_delete(ctx, edge);
    }
    /* todo : log stat */
  }
  for (;;) {
    MUTEX_LOCK(q_mutex);
    if (nthreads == nfthreads) { break; }
    MUTEX_UNLOCK(q_mutex);
    grn_nanosleep(1000000);
  }
  {
    grn_edge *edge;
    GRN_HASH_EACH(ctx, grn_edges, id, NULL, NULL, &edge, {
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
      grn_edges_delete(ctx, edge);
    });
  }
  {
    grn_com *com;
    GRN_HASH_EACH(ctx, ev->hash, id, NULL, NULL, &com, { grn_com_close(ctx, com); });
  }
}

static int
run_server(grn_ctx *ctx, grn_obj *db, grn_com_event *ev,
           grn_edge_dispatcher_func dispatcher, grn_handler_func handler)
{
  int exit_code = EXIT_SUCCESS;
  struct hostent *he;
  if (!(he = gethostbyname(hostname))) {
    SERR("gethostbyname");
  } else {
    ev->opaque = db;
    grn_edges_init(ctx, dispatcher);
    if (!grn_com_sopen(ctx, ev, bind_address, port, handler, he)) {
      if (is_daemon_mode) {
        exit_code = daemonize();
      }
      if (exit_code == EXIT_SUCCESS) {
        run_server_loop(ctx, ev);
      }
      if (is_daemon_mode) {
        clean_pid_file();
      }
      exit_code = EXIT_SUCCESS;
    } else {
      fprintf(stderr, "grn_com_sopen failed (%s:%d): %s\n",
              bind_address, port, ctx->errbuf);
    }
    grn_edges_fin(ctx);
  }
  return exit_code;
}

#define JSON_CALLBACK_PARAM "callback"

typedef struct {
  grn_obj body;
  grn_msg *msg;
} ht_context;

static void
h_output(grn_ctx *ctx, int flags, void *arg)
{
  grn_rc expr_rc = ctx->rc;
  ht_context *hc = (ht_context *)arg;
  grn_sock fd = hc->msg->u.fd;
  grn_obj *body = &hc->body;
  grn_obj head, foot, *outbuf = ctx->impl->outbuf;
  if (!(flags & GRN_CTX_TAIL)) { return; }
  GRN_TEXT_INIT(&head, 0);
  GRN_TEXT_INIT(&foot, 0);
  if (!expr_rc) {
    grn_obj *expr = ctx->impl->curr_expr;
    grn_obj *jsonp_func = NULL;
    if (expr) {
      jsonp_func = grn_expr_get_var(ctx, expr, JSON_CALLBACK_PARAM,
                                    strlen(JSON_CALLBACK_PARAM));
    }
    if (jsonp_func && GRN_TEXT_LEN(jsonp_func)) {
      GRN_TEXT_PUT(ctx, &head, GRN_TEXT_VALUE(jsonp_func), GRN_TEXT_LEN(jsonp_func));
      GRN_TEXT_PUTC(ctx, &head, '(');
      output_envelope(ctx, expr_rc, &head, outbuf, &foot);
      GRN_TEXT_PUTS(ctx, &foot, ");");
    } else {
      output_envelope(ctx, expr_rc, &head, outbuf, &foot);
    }
    GRN_TEXT_SETS(ctx, body, "HTTP/1.1 200 OK\r\n");
    GRN_TEXT_PUTS(ctx, body, "Connection: close\r\n");
    GRN_TEXT_PUTS(ctx, body, "Content-Type: ");
    GRN_TEXT_PUTS(ctx, body, grn_output_type(ctx));
    GRN_TEXT_PUTS(ctx, body, "\r\nContent-Length: ");
    grn_text_lltoa(ctx, body,
                   GRN_TEXT_LEN(&head) + GRN_TEXT_LEN(outbuf) + GRN_TEXT_LEN(&foot));
    GRN_TEXT_PUTS(ctx, body, "\r\n\r\n");
  } else {
    GRN_BULK_REWIND(outbuf);
    output_envelope(ctx, expr_rc, &head, outbuf, &foot);
    if (expr_rc == GRN_NO_SUCH_FILE_OR_DIRECTORY) {
      GRN_TEXT_SETS(ctx, body, "HTTP/1.1 404 Not Found\r\n");
    } else {
      GRN_TEXT_SETS(ctx, body, "HTTP/1.1 500 Internal Server Error\r\n");
    }
    GRN_TEXT_PUTS(ctx, body, "Content-Type: ");
    GRN_TEXT_PUTS(ctx, body, grn_output_type(ctx));
    GRN_TEXT_PUTS(ctx, body, "\r\n\r\n");
  }
  {
    ssize_t ret, len;
#ifdef WIN32
    WSABUF wsabufs[4];
    wsabufs[0].buf = GRN_TEXT_VALUE(body);
    wsabufs[0].len = GRN_TEXT_LEN(body);
    wsabufs[1].buf = GRN_TEXT_VALUE(&head);
    wsabufs[1].len = GRN_TEXT_LEN(&head);
    wsabufs[2].buf = GRN_TEXT_VALUE(outbuf);
    wsabufs[2].len = GRN_TEXT_LEN(outbuf);
    wsabufs[3].buf = GRN_TEXT_VALUE(&foot);
    wsabufs[3].len = GRN_TEXT_LEN(&foot);
    if (WSASend(fd, wsabufs, 4, &ret, 0, NULL, NULL) == SOCKET_ERROR) {
      SERR("WSASend");
    }
#else /* WIN32 */
    struct iovec msg_iov[4];
    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = msg_iov;
    msg.msg_iovlen = 4;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg_iov[0].iov_base = GRN_TEXT_VALUE(body);
    msg_iov[0].iov_len = GRN_TEXT_LEN(body);
    msg_iov[1].iov_base = GRN_TEXT_VALUE(&head);
    msg_iov[1].iov_len = GRN_TEXT_LEN(&head);
    msg_iov[2].iov_base = GRN_TEXT_VALUE(outbuf);
    msg_iov[2].iov_len = GRN_TEXT_LEN(outbuf);
    msg_iov[3].iov_base = GRN_TEXT_VALUE(&foot);
    msg_iov[3].iov_len = GRN_TEXT_LEN(&foot);
    if ((ret = sendmsg(fd, &msg, MSG_NOSIGNAL)) == -1) {
      SERR("sendmsg");
    }
#endif /* WIN32 */
    len = GRN_TEXT_LEN(body) + GRN_TEXT_LEN(&head) +
      GRN_TEXT_LEN(outbuf) + GRN_TEXT_LEN(&foot);
    if (ret != len) {
      GRN_LOG(&grn_gctx, GRN_LOG_NOTICE,
              "couldn't send all data (%" GRN_FMT_LLD "/%" GRN_FMT_LLD ")",
              (long long int)ret, (long long int)len);
    }
  }
  GRN_BULK_REWIND(body);
  GRN_BULK_REWIND(outbuf);
  GRN_OBJ_FIN(ctx, &foot);
  GRN_OBJ_FIN(ctx, &head);
}

static void
do_htreq(grn_ctx *ctx, grn_msg *msg)
{
  grn_sock fd = msg->u.fd;
  grn_http_request_type t = grn_http_request_type_none;
  grn_com_header *header = &msg->header;
  switch (header->qtype) {
  case 'G' : /* GET */
    t = grn_http_request_type_get;
    break;
  case 'P' : /* POST */
    t = grn_http_request_type_post;
    break;
  }
  if (t) {
    char *path = NULL;
    char *pathe = GRN_BULK_HEAD((grn_obj *)msg);
    char *e = GRN_BULK_CURR((grn_obj *)msg);
    for (;; pathe++) {
      if (e <= pathe + 6) {
        /* invalid request */
        goto exit;
      }
      if (*pathe == ' ') {
        if (!path) {
          path = pathe + 1;
        } else {
          if (!memcmp(pathe + 1, "HTTP/1", 6)) {
            break;
          }
        }
      }
    }
    grn_ctx_send(ctx, path, pathe - path, 0);
  }
exit :
  /* TODO: support "Connection: keep-alive" */
  ctx->stat = GRN_CTX_QUIT;
  /* TODO: support a command in multi requests. e.g.: load command */
  grn_ctx_set_next_expr(ctx, NULL);
  /* if (ctx->rc != GRN_OPERATION_WOULD_BLOCK) {...} */
  grn_msg_close(ctx, (grn_obj *)msg);
  /* if not keep alive connection */
  grn_sock_close(fd);
  grn_com_event_start_accept(ctx, msg->acceptor->ev);
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

static grn_critical_section cache_lock;
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
  CRITICAL_SECTION_ENTER(cache_lock);
  if (!cache_cas) {
    if ((cache_table = CTX_GET("Memcache"))) {
      cache_value = CTX_GET("Memcache.value");
      cache_flags = CTX_GET("Memcache.flags");
      cache_expire = CTX_GET("Memcache.expire");
      cache_cas = CTX_GET("Memcache.cas");
    } else {
      if (!cache_table) {
        grn_obj *uint32_type = grn_ctx_at(ctx, GRN_DB_UINT32);
        grn_obj *uint64_type = grn_ctx_at(ctx, GRN_DB_UINT64);
        grn_obj *shorttext_type = grn_ctx_at(ctx, GRN_DB_SHORT_TEXT);
        if ((cache_table = grn_table_create(ctx, "Memcache", 8, NULL,
                                            GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                                            shorttext_type, NULL))) {
          cache_value = grn_column_create(ctx, cache_table, "value", 5, NULL,
                                          GRN_OBJ_PERSISTENT, shorttext_type);
          cache_flags = grn_column_create(ctx, cache_table, "flags", 5, NULL,
                                          GRN_OBJ_PERSISTENT, uint32_type);
          cache_expire = grn_column_create(ctx, cache_table, "expire", 6, NULL,
                                           GRN_OBJ_PERSISTENT, uint32_type);
          cache_cas = grn_column_create(ctx, cache_table, "cas", 3, NULL,
                                        GRN_OBJ_PERSISTENT, uint64_type);
        }
      }
    }
  }
  CRITICAL_SECTION_LEAVE(cache_lock);
  return cache_cas;
}

#define RELATIVE_TIME_THRESH 1000000000

#define MBRES(ctx,re,status,key_len,extra_len,flags) do {\
  grn_msg_set_property((ctx), (re), (status), (key_len), (extra_len));\
  grn_msg_send((ctx), (re), (flags));\
} while (0)

#define GRN_MSG_MBRES(block) do {\
  if (!quiet) {\
    grn_obj *re = grn_msg_open_for_reply(ctx, (grn_obj *)msg, &edge->send_old);\
    ((grn_msg *)re)->header.qtype = header->qtype;\
    block\
  }\
} while (0)

static uint64_t
get_mbreq_cas_id()
{
  static uint64_t cas_id = 0;
  /* FIXME: use GRN_ATOMIC_ADD_EX_64, but it is not implemented */
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
    flags = GRN_CTX_MORE;
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
        grn_timeval tv;
        uint32_t expire;
        {
          grn_obj expire_buf;
          GRN_UINT32_INIT(&expire_buf, 0);
          grn_obj_get_value(ctx, cache_expire, rid, &expire_buf);
          expire = GRN_UINT32_VALUE(&expire_buf);
          grn_obj_close(ctx, &expire_buf);
        }
        grn_timeval_now(ctx, &tv);
        if (expire && expire < tv.tv_sec) {
          grn_table_delete_by_id(ctx, cache_table, rid);
          GRN_MSG_MBRES({
            MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
          });
        } else {
          grn_obj cas_buf;
          GRN_UINT64_INIT(&cas_buf, 0);
          grn_obj_get_value(ctx, cache_cas, rid, &cas_buf);
          GRN_MSG_MBRES({
            grn_obj_get_value(ctx, cache_flags, rid, re);
            grn_obj_get_value(ctx, cache_value, rid, re);
            ((grn_msg *)re)->header.cas = GRN_UINT64_VALUE(&cas_buf);
            MBRES(ctx, re, MBRES_SUCCESS, 0, 4, flags);
          });
          grn_obj_close(ctx, &cas_buf);
        }
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
          if (header->cas) {
            GRN_MSG_MBRES({
              MBRES(ctx, re, MBRES_EINVAL, 0, 0, 0);
            });
          } else {
            grn_obj text_buf, uint32_buf;
            GRN_TEXT_INIT(&text_buf, GRN_OBJ_DO_SHALLOW_COPY);
            GRN_TEXT_SET_REF(&text_buf, value, valuelen);
            grn_obj_set_value(ctx, cache_value, rid, &text_buf, GRN_OBJ_SET);
            GRN_UINT32_INIT(&uint32_buf, 0);
            GRN_UINT32_SET(ctx, &uint32_buf, flags);
            grn_obj_set_value(ctx, cache_flags, rid, &uint32_buf, GRN_OBJ_SET);
            if (expire && expire < RELATIVE_TIME_THRESH) {
              grn_timeval tv;
              grn_timeval_now(ctx, &tv);
              expire += tv.tv_sec;
            }
            GRN_UINT32_SET(ctx, &uint32_buf, expire);
            grn_obj_set_value(ctx, cache_expire, rid, &uint32_buf, GRN_OBJ_SET);
            grn_obj_close(ctx, &uint32_buf);
            {
              grn_obj cas_buf;
              uint64_t cas_id = get_mbreq_cas_id();
              GRN_UINT64_INIT(&cas_buf, 0);
              GRN_UINT64_SET(ctx, &cas_buf, cas_id);
              grn_obj_set_value(ctx, cache_cas, rid, &cas_buf, GRN_OBJ_SET);
              grn_obj_close(ctx, &cas_buf);
              GRN_MSG_MBRES({
                ((grn_msg *)re)->header.cas = cas_id;
                MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
              });
            }
          }
        } else {
          if (header->qtype != MBCMD_SET && header->qtype != MBCMD_SETQ) {
            grn_obj uint32_buf;
            grn_timeval tv;
            uint32_t oexpire;

            GRN_UINT32_INIT(&uint32_buf, 0);
            grn_obj_get_value(ctx, cache_expire, rid, &uint32_buf);
            oexpire = GRN_UINT32_VALUE(&uint32_buf);
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
          }
          {
            if (header->cas) {
              grn_obj cas_buf;
              GRN_UINT64_INIT(&cas_buf, 0);
              grn_obj_get_value(ctx, cache_cas, rid, &cas_buf);
              if (header->cas != GRN_UINT64_VALUE(&cas_buf)) {
                GRN_MSG_MBRES({
                  MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0);
                });
              }
            }
            {
              grn_obj text_buf, uint32_buf;
              GRN_TEXT_INIT(&text_buf, GRN_OBJ_DO_SHALLOW_COPY);
              GRN_TEXT_SET_REF(&text_buf, value, valuelen);
              grn_obj_set_value(ctx, cache_value, rid, &text_buf, GRN_OBJ_SET);
              GRN_UINT32_INIT(&uint32_buf, 0);
              GRN_UINT32_SET(ctx, &uint32_buf, flags);
              grn_obj_set_value(ctx, cache_flags, rid, &uint32_buf, GRN_OBJ_SET);
              if (expire && expire < RELATIVE_TIME_THRESH) {
                grn_timeval tv;
                grn_timeval_now(ctx, &tv);
                expire += tv.tv_sec;
              }
              GRN_UINT32_SET(ctx, &uint32_buf, expire);
              grn_obj_set_value(ctx, cache_expire, rid, &uint32_buf, GRN_OBJ_SET);
              {
                grn_obj cas_buf;
                uint64_t cas_id = get_mbreq_cas_id();
                GRN_UINT64_INIT(&cas_buf, 0);
                GRN_UINT64_SET(ctx, &cas_buf, cas_id);
                grn_obj_set_value(ctx, cache_cas, rid, &cas_buf, GRN_OBJ_SET);
                GRN_MSG_MBRES({
                  ((grn_msg *)re)->header.cas = cas_id;
                  MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
                });
              }
            }
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
        /* GRN_LOG(ctx, GRN_LOG_NOTICE, "GET k=%d not found", keylen); */
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
        grn_obj uint32_buf, text_buf;
        GRN_UINT32_INIT(&uint32_buf, 0);
        GRN_TEXT_INIT(&text_buf, GRN_OBJ_DO_SHALLOW_COPY);
        if (added) {
          GRN_TEXT_SET_REF(&text_buf, &init, 8);
          grn_obj_set_value(ctx, cache_value, rid, &text_buf, GRN_OBJ_SET);
          GRN_UINT32_SET(ctx, &uint32_buf, 0);
          grn_obj_set_value(ctx, cache_flags, rid, &uint32_buf, GRN_OBJ_SET);
        } else {
          grn_timeval tv;
          uint32_t oexpire;

          grn_obj_get_value(ctx, cache_expire, rid, &uint32_buf);
          oexpire = GRN_UINT32_VALUE(&uint32_buf);
          grn_timeval_now(ctx, &tv);

          if (oexpire && oexpire < tv.tv_sec) {
            if (expire == 0xffffffffU) {
              GRN_MSG_MBRES({
                MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
              });
              break;
            } else {
              GRN_TEXT_SET_REF(&text_buf, &init, 8);
              grn_obj_set_value(ctx, cache_value, rid, &text_buf, GRN_OBJ_SET);
              GRN_UINT32_SET(ctx, &uint32_buf, 0);
              grn_obj_set_value(ctx, cache_flags, rid, &uint32_buf, GRN_OBJ_SET);
            }
          } else {
            grn_obj uint64_buf;
            GRN_UINT64_INIT(&uint64_buf, 0);
            GRN_UINT64_SET(ctx, &uint64_buf, delta);
            grn_obj_set_value(ctx, cache_value, rid, &uint64_buf,
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
        GRN_UINT32_SET(ctx, &uint32_buf, expire);
        grn_obj_set_value(ctx, cache_expire, rid, &uint32_buf, GRN_OBJ_SET);
        GRN_MSG_MBRES({
          /* TODO: get_mbreq_cas_id() */
          grn_obj_get_value(ctx, cache_value, rid, re);
          grn_hton(&delta, (uint64_t *)GRN_BULK_HEAD(re), 8);
          GRN_TEXT_SET(ctx, re, &delta, sizeof(uint64_t));
          MBRES(ctx, re, MBRES_SUCCESS, 0, sizeof(uint64_t), 0);
        });
      }
    }
    break;
  case MBCMD_FLUSHQ :
    quiet = 1;
    /* fallthru */
  case MBCMD_FLUSH :
    {
      uint32_t expire;
      uint8_t extralen = header->level;
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
      {
        grn_obj exp_buf;
        GRN_UINT32_INIT(&exp_buf, 0);
        GRN_UINT32_SET(ctx, &exp_buf, expire);
        GRN_TABLE_EACH(ctx, cache_table, 0, 0, rid, NULL, NULL, NULL, {
          grn_obj_set_value(ctx, cache_expire, rid, &exp_buf, GRN_OBJ_SET);
        });
        GRN_MSG_MBRES({
          MBRES(ctx, re, MBRES_SUCCESS, 0, 4, 0);
        });
        grn_obj_close(ctx, &exp_buf);
      }
    }
    break;
  case MBCMD_NOOP :
    break;
  case MBCMD_VERSION :
    GRN_MSG_MBRES({
      grn_bulk_write(ctx, re, PACKAGE_VERSION, strlen(PACKAGE_VERSION));
      MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
    });
    break;
  case MBCMD_GETKQ :
    flags = GRN_CTX_MORE;
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
        grn_obj uint32_buf;
        grn_timeval tv;
        uint32_t expire;
        GRN_UINT32_INIT(&uint32_buf, 0);
        grn_obj_get_value(ctx, cache_expire, rid, &uint32_buf);
        expire = GRN_UINT32_VALUE(&uint32_buf);
        grn_timeval_now(ctx, &tv);
        if (expire && expire < tv.tv_sec) {
          grn_table_delete_by_id(ctx, cache_table, rid);
          GRN_MSG_MBRES({
            MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0);
          });
        } else {
          grn_obj uint64_buf;
          GRN_UINT64_INIT(&uint64_buf, 0);
          grn_obj_get_value(ctx, cache_cas, rid, &uint64_buf);
          GRN_MSG_MBRES({
            grn_obj_get_value(ctx, cache_flags, rid, re);
            grn_bulk_write(ctx, re, key, keylen);
            grn_obj_get_value(ctx, cache_value, rid, re);
            ((grn_msg *)re)->header.cas = GRN_UINT64_VALUE(&uint64_buf);
            MBRES(ctx, re, MBRES_SUCCESS, keylen, 4, flags);
          });
        }
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
    ctx->stat = GRN_CTX_QUIT;
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

static void * CALLBACK
h_worker(void *arg)
{
  ht_context hc;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  GRN_TEXT_INIT(&hc.body, 0);
  grn_ctx_use(ctx, (grn_obj *)arg);
  grn_ctx_recv_handler_set(ctx, h_output, &hc);
  GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "thread start (%d/%d)", nfthreads, nthreads + 1);
  MUTEX_LOCK(q_mutex);
  do {
    grn_obj *msg;
    nfthreads++;
    while (!(msg = (grn_obj *)grn_com_queue_deque(&grn_gctx, &ctx_new))) {
      COND_WAIT(q_cond, q_mutex);
      if (grn_gctx.stat == GRN_CTX_QUIT) {
        nfthreads--;
        goto exit;
      }
    }
    nfthreads--;
    MUTEX_UNLOCK(q_mutex);
    hc.msg = (grn_msg *)msg;
    do_htreq(ctx, (grn_msg *)msg);
    MUTEX_LOCK(q_mutex);
  } while (nfthreads < max_nfthreads && grn_gctx.stat != GRN_CTX_QUIT);
exit :
  nthreads--;
  MUTEX_UNLOCK(q_mutex);
  GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "thread end (%d/%d)", nfthreads, nthreads);
  GRN_OBJ_FIN(ctx, &hc.body);
  grn_ctx_fin(ctx);
  return NULL;
}

static void
h_handler(grn_ctx *ctx, grn_obj *msg)
{
  grn_com *com = ((grn_msg *)msg)->u.peer;
  if (ctx->rc) {
    grn_com_close(ctx, com);
    grn_msg_close(ctx, msg);
  } else {
    grn_sock fd = com->fd;
    void *arg = com->ev->opaque;
    /* if not keep alive connection */
    grn_com_event_del(ctx, com->ev, fd);
    ((grn_msg *)msg)->u.fd = fd;
    MUTEX_LOCK(q_mutex);
    grn_com_queue_enque(ctx, &ctx_new, (grn_com_queue_entry *)msg);
    if (!nfthreads && nthreads < max_nfthreads) {
      grn_thread thread;
      nthreads++;
      if (THREAD_CREATE(thread, h_worker, arg)) { SERR("pthread_create"); }
    }
    COND_SIGNAL(q_cond);
    MUTEX_UNLOCK(q_mutex);
  }
}

static int
h_server(char *path)
{
  int exit_code = EXIT_FAILURE;
  grn_com_event ev;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  MUTEX_INIT(q_mutex);
  COND_INIT(q_cond);
  CRITICAL_SECTION_INIT(cache_lock);
  GRN_COM_QUEUE_INIT(&ctx_new);
  GRN_COM_QUEUE_INIT(&ctx_old);
#ifndef WIN32
  {
    struct rlimit lim;
    lim.rlim_cur = 4096;
    lim.rlim_max = 4096;
    /* RLIMIT_OFILE */
    setrlimit(RLIMIT_NOFILE, &lim);
    lim.rlim_cur = 0;
    lim.rlim_max = 0;
    getrlimit(RLIMIT_NOFILE, &lim);
    GRN_LOG(ctx, GRN_LOG_NOTICE, "RLIMIT_NOFILE(%" GRN_FMT_LLD ",%" GRN_FMT_LLD ")",
            (long long int)lim.rlim_cur, (long long int)lim.rlim_max);
  }
#endif /* WIN32 */
  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
    grn_obj *db;
    db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
    if (db) {
      exit_code = run_server(ctx, db, &ev, NULL, h_handler);
      grn_obj_close(ctx, db);
    } else {
      fprintf(stderr, "db open failed (%s)\n", path);
    }
    grn_com_event_fin(ctx, &ev);
  } else {
    fprintf(stderr, "grn_com_event_init failed\n");
  }
  grn_ctx_fin(ctx);
  return exit_code;
}

static void * CALLBACK
g_worker(void *arg)
{
  GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "thread start (%d/%d)", nfthreads, nthreads + 1);
  MUTEX_LOCK(q_mutex);
  do {
    grn_ctx *ctx;
    grn_edge *edge;
    nfthreads++;
    while (!(edge = (grn_edge *)grn_com_queue_deque(&grn_gctx, &ctx_new))) {
      COND_WAIT(q_cond, q_mutex);
      if (grn_gctx.stat == GRN_CTX_QUIT) {
        nfthreads--;
        goto exit;
      }
    }
    ctx = &edge->ctx;
    nfthreads--;
    if (edge->stat == EDGE_DOING) { continue; }
    if (edge->stat == EDGE_WAIT) {
      edge->stat = EDGE_DOING;
      while (!GRN_COM_QUEUE_EMPTYP(&edge->recv_new)) {
        grn_obj *msg;
        MUTEX_UNLOCK(q_mutex);
        /* if (edge->flags == GRN_EDGE_WORKER) */
        while (ctx->stat != GRN_CTX_QUIT &&
               (edge->msg = (grn_msg *)grn_com_queue_deque(ctx, &edge->recv_new))) {
          grn_com_header *header = &edge->msg->header;
          msg = (grn_obj *)edge->msg;
          switch (header->proto) {
          case GRN_COM_PROTO_MBREQ :
            do_mbreq(ctx, edge);
            break;
          case GRN_COM_PROTO_GQTP :
            grn_ctx_send(ctx, GRN_BULK_HEAD(msg), GRN_BULK_VSIZE(msg), header->flags);
            ERRCLR(ctx);
            break;
          default :
            ctx->stat = GRN_CTX_QUIT;
            break;
          }
          grn_msg_close(ctx, msg);
        }
        while ((msg = (grn_obj *)grn_com_queue_deque(ctx, &edge->send_old))) {
          grn_msg_close(ctx, msg);
        }
        MUTEX_LOCK(q_mutex);
        if (ctx->stat == GRN_CTX_QUIT || edge->stat == EDGE_ABORT) { break; }
      }
    }
    if (ctx->stat == GRN_CTX_QUIT || edge->stat == EDGE_ABORT) {
      grn_com_queue_enque(&grn_gctx, &ctx_old, (grn_com_queue_entry *)edge);
      edge->stat = EDGE_ABORT;
    } else {
      edge->stat = EDGE_IDLE;
    }
  } while (nfthreads < max_nfthreads && grn_gctx.stat != GRN_CTX_QUIT);
exit :
  nthreads--;
  MUTEX_UNLOCK(q_mutex);
  GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "thread end (%d/%d)", nfthreads, nthreads);
  return NULL;
}

static void
g_dispatcher(grn_ctx *ctx, grn_edge *edge)
{
  MUTEX_LOCK(q_mutex);
  if (edge->stat == EDGE_IDLE) {
    grn_com_queue_enque(ctx, &ctx_new, (grn_com_queue_entry *)edge);
    edge->stat = EDGE_WAIT;
    if (!nfthreads && nthreads < max_nfthreads) {
      grn_thread thread;
      nthreads++;
      if (THREAD_CREATE(thread, g_worker, NULL)) { SERR("pthread_create"); }
    }
    COND_SIGNAL(q_cond);
  }
  MUTEX_UNLOCK(q_mutex);
}

static void
g_output(grn_ctx *ctx, int flags, void *arg)
{
  grn_edge *edge = arg;
  grn_com *com = edge->com;
  grn_msg *req = edge->msg, *msg = (grn_msg *)ctx->impl->outbuf;
  msg->edge_id = req->edge_id;
  msg->header.proto = req->header.proto == GRN_COM_PROTO_MBREQ
    ? GRN_COM_PROTO_MBRES : req->header.proto;
  if (grn_msg_send(ctx, (grn_obj *)msg,
                   (flags & GRN_CTX_MORE) ? GRN_CTX_MORE : GRN_CTX_TAIL)) {
    edge->stat = EDGE_ABORT;
  }
  ctx->impl->outbuf = grn_msg_open(ctx, com, &edge->send_old);
}

static void
g_handler(grn_ctx *ctx, grn_obj *msg)
{
  grn_edge *edge;
  grn_com *com = ((grn_msg *)msg)->u.peer;
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
    edge = grn_edges_add(ctx, &((grn_msg *)msg)->edge_id, &added);
    if (added) {
      grn_ctx_init(&edge->ctx, (useql ? GRN_CTX_USE_QL : 0));
      GRN_COM_QUEUE_INIT(&edge->recv_new);
      GRN_COM_QUEUE_INIT(&edge->send_old);
      grn_ctx_use(&edge->ctx, (grn_obj *)com->ev->opaque);
      grn_ctx_recv_handler_set(&edge->ctx, g_output, edge);
      com->opaque = edge;
      grn_obj_close(&edge->ctx, edge->ctx.impl->outbuf);
      edge->ctx.impl->outbuf = grn_msg_open(&edge->ctx, com, &edge->send_old);
      edge->com = com;
      edge->stat = EDGE_IDLE;
      edge->flags = GRN_EDGE_WORKER;
    }
    if (edge->ctx.stat == GRN_CTX_QUIT || edge->stat == EDGE_ABORT) {
      grn_msg_close(ctx, msg);
    } else {
      grn_com_queue_enque(ctx, &edge->recv_new, (grn_com_queue_entry *)msg);
      g_dispatcher(ctx, edge);
    }
  }
}

static int
g_server(char *path)
{
  int exit_code = EXIT_FAILURE;
  grn_com_event ev;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  MUTEX_INIT(q_mutex);
  COND_INIT(q_cond);
  CRITICAL_SECTION_INIT(cache_lock);
  GRN_COM_QUEUE_INIT(&ctx_new);
  GRN_COM_QUEUE_INIT(&ctx_old);
#ifndef WIN32
  {
    struct rlimit limit;
    limit.rlim_cur = 0;
    limit.rlim_max = 0;
    getrlimit(RLIMIT_NOFILE, &limit);
    if (limit.rlim_cur < RLIMIT_NOFILE_MINIMUM) {
      limit.rlim_cur = RLIMIT_NOFILE_MINIMUM;
      limit.rlim_max = RLIMIT_NOFILE_MINIMUM;
      setrlimit(RLIMIT_NOFILE, &limit);
      limit.rlim_cur = 0;
      limit.rlim_max = 0;
      getrlimit(RLIMIT_NOFILE, &limit);
    }
    GRN_LOG(ctx, GRN_LOG_NOTICE,
            "RLIMIT_NOFILE(%" GRN_FMT_LLD ",%" GRN_FMT_LLD ")",
            (long long int)limit.rlim_cur, (long long int)limit.rlim_max);
  }
#endif /* WIN32 */
  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
    grn_obj *db;
    db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
    if (db) {
      exit_code = run_server(ctx, db, &ev, g_dispatcher, g_handler);
      grn_obj_close(ctx, db);
    } else {
      fprintf(stderr, "db open failed (%s)\n", path);
    }
    grn_com_event_fin(ctx, &ev);
  } else {
    fprintf(stderr, "grn_com_event_init failed\n");
  }
  grn_ctx_fin(ctx);
  return exit_code;
}

enum {
  mode_alone = 0,
  mode_client,
  mode_daemon,
  mode_server,
  mode_usage,
  mode_version,
  mode_config,
  mode_error
};

#define MODE_MASK   0x007f
#define MODE_USE_QL 0x0080
#define MODE_NEW_DB 0x0100

static uint32_t
get_core_number(void)
{
#ifdef WIN32
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  return sinfo.dwNumberOfProcessors;
#else /* WIN32 */
#  ifdef _SC_NPROCESSORS_CONF
  return sysconf(_SC_NPROCESSORS_CONF);
#  else
  int n_processors;
  size_t length = sizeof(n_processors);
  int mib[] = {CTL_HW, HW_NCPU};
  if (sysctl(mib, sizeof(mib) / sizeof(mib[0]),
             &n_processors, &length, NULL, 0) == 0 &&
      length == sizeof(n_processors) &&
      0 < n_processors) {
    return n_processors;
  } else {
    return 1;
  }
#  endif /* _SC_NPROCESSORS_CONF */
#endif /* WIN32 */
}

/*
 * The length of each line, including an end-of-line, in config file should be
 * shorter than (CONFIG_FILE_BUF_SIZE - 1) bytes. Too long lines are ignored.
 * Note that both '\r' and '\n' are handled as end-of-lines.
 *
 * '#' and ';' are special symbols to start comments. A comment ends with an
 * end-of-line.
 *
 * Format: name[=value]
 * - Preceding/trailing white-spaces of each line are removed.
 * - White-spaces aroung '=' are removed.
 * - name does not allow white-spaces.
 */
#define CONFIG_FILE_BUF_SIZE 4096
#define CONFIG_FILE_MAX_NAME_LENGTH 128
#define CONFIG_FILE_MAX_VALUE_LENGTH 2048

typedef enum {
  CONFIG_FILE_SUCCESS,
  CONFIG_FILE_FORMAT_ERROR,
  CONFIG_FILE_FOPEN_ERROR,
  CONFIG_FILE_MALLOC_ERROR,
  CONFIG_FILE_ATEXIT_ERROR
} config_file_status;

/*
 * The node type of a linked list for storing values. Note that a value is
 * stored in the extra space of an object.
 */
typedef struct _config_file_entry {
  struct _config_file_entry *next;
} config_file_entry;

static config_file_entry *config_file_entry_head = NULL;

static void
config_file_clear(void) {
  while (config_file_entry_head) {
    config_file_entry *next = config_file_entry_head->next;
    free(config_file_entry_head);
    config_file_entry_head = next;
  }
}

static config_file_status
config_file_register(const char *path, const grn_str_getopt_opt *opts,
                     int *flags, const char *name, size_t name_length,
                     const char *value, size_t value_length)
{
  char name_buf[CONFIG_FILE_MAX_NAME_LENGTH + 3];
  config_file_entry *entry = NULL;
  char *args[4];

  name_buf[0] = name_buf[1] = '-';
  strcpy(name_buf + 2, name);

  if (value) {
    const size_t entry_size = sizeof(config_file_entry) + value_length + 1;
    entry = (config_file_entry *)malloc(entry_size);
    if (!entry) {
      fprintf(stderr, "memory allocation failed: %u bytes\n",
              (unsigned int)entry_size);
      return CONFIG_FILE_MALLOC_ERROR;
    }
    strcpy((char *)(entry + 1), value);
    entry->next = config_file_entry_head;
    if (!config_file_entry_head) {
      if (atexit(config_file_clear)) {
        free(entry);
        return CONFIG_FILE_ATEXIT_ERROR;
      }
    }
    config_file_entry_head = entry;
  }

  args[0] = (char *)path;
  args[1] = name_buf;
  args[2] = entry ? (char *)(entry + 1) : NULL;
  args[3] = NULL;
  grn_str_getopt(entry ? 3 : 2, args, opts, flags);
  return CONFIG_FILE_SUCCESS;
}

static config_file_status
config_file_parse(const char *path, const grn_str_getopt_opt *opts,
                  int *flags, char *buf) {
  char *ptr, *name, *value;
  size_t name_length, value_length;

  while (isspace(*buf)) {
    buf++;
  }

  ptr = buf;
  while (*ptr && *ptr != '#' && *ptr != ';') {
    ptr++;
  }

  do {
    *ptr-- = '\0';
  } while (ptr >= buf && isspace(*ptr));

  if (!*buf) {
    return CONFIG_FILE_SUCCESS;
  }

  name = ptr = buf;
  while (*ptr && !isspace(*ptr) && *ptr != '=') {
    ptr++;
  }
  while (isspace(*ptr)) {
    *ptr++ = '\0';
  }

  name_length = strlen(name);
  if (name_length == 0) {
    return CONFIG_FILE_SUCCESS;
  } else if (name_length > CONFIG_FILE_MAX_NAME_LENGTH) {
    fprintf(stderr, "too long name in config file: %u bytes\n",
            (unsigned int)name_length);
    return CONFIG_FILE_FORMAT_ERROR;
  }

  if (*ptr == '=') {
    *ptr++ = '\0';
    while (isspace(*ptr)) {
      ptr++;
    }
    value = ptr;
  } else if (*ptr) {
    fprintf(stderr, "invalid name in config file\n");
    return CONFIG_FILE_FORMAT_ERROR;
  } else {
    value = NULL;
  }

  value_length = value ? strlen(value) : 0;
  if (value_length > CONFIG_FILE_MAX_VALUE_LENGTH) {
    fprintf(stderr, "too long value in config file: %u bytes\n",
            (unsigned int)value_length);
    return CONFIG_FILE_FORMAT_ERROR;
  }

  return config_file_register(path, opts, flags,
                              name, name_length, value, value_length);
}

static config_file_status
config_file_load(const char *path, const grn_str_getopt_opt *opts, int *flags)
{
  config_file_status status = CONFIG_FILE_SUCCESS;
  char buf[CONFIG_FILE_BUF_SIZE];
  size_t length = 0;
  FILE * const file = fopen(path, "rb");
  if (!file) {
    return CONFIG_FILE_FOPEN_ERROR;
  }

  for ( ; ; ) {
    int c = fgetc(file);
    if (c == '\r' || c == '\n' || c == EOF) {
      if (length < sizeof(buf) - 1) {
        buf[length] = '\0';
        status = config_file_parse(path, opts, flags, buf);
        if (status != CONFIG_FILE_SUCCESS) {
          break;
        }
      }
      length = 0;
    } else if (c == '\0') {
      fprintf(stderr, "prohibited '\\0' in config file: %s\n", path);
      status = CONFIG_FILE_FORMAT_ERROR;
      break;
    } else {
      if (length < sizeof(buf) - 1) {
        buf[length] = (char)c;
      }
      length++;
    }

    if (c == EOF) {
      break;
    }
  }

  fclose(file);
  return status;
}

static const int default_port = DEFAULT_PORT;
static grn_encoding default_encoding = GRN_ENC_DEFAULT;
static uint32_t default_max_num_threads = DEFAULT_MAX_NFTHREADS;
static const int default_mode = mode_alone;
static const int default_log_level = GRN_LOG_DEFAULT_LEVEL;
static const char * const default_protocol = "gqtp";
static const char *default_hostname = "localhost";
static const char * const default_dest = "localhost";
static const char *default_log_path = "";
static const char *default_query_log_path = "";
static const char *default_config_path = "";
static uint32_t default_cache_limit = 0;
static const char *default_document_root = "";
static grn_command_version default_default_command_version =
    GRN_COMMAND_VERSION_DEFAULT;
static int64_t default_default_match_escalation_threshold = 0;
static const char * const default_bind_address = "0.0.0.0";

static void
init_default_settings(void)
{
  output = stdout;

  default_encoding = grn_strtoenc(GRN_DEFAULT_ENCODING);

  {
    const uint32_t num_cores = get_core_number();
    if (num_cores != 0) {
      default_max_num_threads = num_cores;
    }
  }

  {
    static char hostname[HOST_NAME_MAX + 1];
    hostname[HOST_NAME_MAX] = '\0';
    if (gethostname(hostname, HOST_NAME_MAX)) {
      fprintf(stderr, "gethostname failed: %s\n", strerror(errno));
    } else {
      int error_code;
      struct addrinfo hints, *result;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_addr = NULL;
      hints.ai_canonname = NULL;
      hints.ai_next = NULL;
      error_code = getaddrinfo(hostname, NULL, &hints, &result);
      if (error_code) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(error_code));
      } else {
        freeaddrinfo(result);
        default_hostname = hostname;
      }
    }
  }

  if (grn_log_path) {
    default_log_path = grn_log_path;
  }
  if (grn_qlog_path) {
    default_query_log_path = grn_qlog_path;
  }

  default_config_path = getenv("GRN_CONFIG_PATH");
  if (!default_config_path) {
    default_config_path = GRN_CONFIG_PATH;
    if (!default_config_path) {
      default_config_path = "";
    }
  }

  default_cache_limit = *grn_cache_max_nentries();

#ifdef WIN32
  {
    static char win32_default_document_root[PATH_MAX];
    size_t document_root_length = strlen(grn_win32_base_dir()) + 1 +
        strlen(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT) + 1;
    if (document_root_length >= PATH_MAX) {
      fprintf(stderr, "can't use default root: too long path\n");
    } else {
      strcpy(win32_default_document_root, grn_win32_base_dir());
      strcat(win32_default_document_root, "/");
      strcat(win32_default_document_root, GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT);
      default_document_root = win32_default_document_root;
    }
  }
#else
  default_document_root = GRN_DEFAULT_DOCUMENT_ROOT;
#endif

  default_default_command_version = grn_get_default_command_version();
  default_default_match_escalation_threshold =
      grn_get_default_match_escalation_threshold();
}

static void
show_config(FILE *out, const grn_str_getopt_opt *opts, int flags)
{
  const grn_str_getopt_opt *o;

  for (o = opts; o->opt || o->longopt; o++) {
    switch (o->op) {
    case getopt_op_none:
      if (o->arg && *o->arg) {
        if (o->longopt && strcmp(o->longopt, "config-path")) {
          fprintf(out, "%s=%s\n", o->longopt, *o->arg);
        }
      }
      break;
    case getopt_op_on:
      if (flags & o->flag) {
        goto no_arg;
      }
      break;
    case getopt_op_off:
      if (!(flags & o->flag)) {
        goto no_arg;
      }
      break;
    case getopt_op_update:
      if (flags == o->flag) {
      no_arg:
        if (o->longopt) {
          fprintf(out, "%s\n", o->longopt);
        }
      }
      break;
    }
  }
}

static void
show_version(void)
{
  printf("%s %s [",
         grn_get_package(),
         grn_get_version());

  /* FIXME: Should we detect host information dynamically on Windows? */
#ifdef HOST_OS
  printf("%s,", HOST_OS);
#endif
#ifdef HOST_CPU
  printf("%s,", HOST_CPU);
#endif
  printf("%s", GRN_DEFAULT_ENCODING);

  printf(",match-escalation-threshold=%" GRN_FMT_LLD,
         grn_get_default_match_escalation_threshold());

#ifndef NO_NFKC
  printf(",nfkc");
#endif
#ifdef WITH_MECAB
  printf(",mecab");
#endif
#ifdef WITH_MESSAGE_PACK
  printf(",msgpack");
#endif
#ifndef NO_ZLIB
  printf(",zlib");
#endif
#ifndef NO_LZO
  printf(",lzo");
#endif
#ifdef USE_KQUEUE
  printf(",kqueue");
#endif
#ifdef USE_EPOLL
  printf(",epoll");
#endif
#ifdef USE_POLL
  printf(",poll");
#endif
  printf("]\n");

#ifdef CONFIGURE_OPTIONS
  printf("\n");
  printf("configure options: <%s>\n", CONFIGURE_OPTIONS);
#endif
}

static void
show_usage(FILE *output)
{
  fprintf(output,
          "Usage: groonga [options...] [dest]\n"
          "\n"
          "Mode options: (default: standalone)\n"
          " By default, groonga runs in standalone mode.\n"
          "  -c:   run in client mode\n"
          "  -s:   run in server mode\n"
          "  -d:   run in daemon mode\n"
          "\n"
          "Database creation options:\n"
          "  -n:                  create new database (except client mode)\n"
          "  -e, --encoding <encoding>:\n"
          "                       specify encoding for new database\n"
          "                       [none|euc|utf8|sjis|latin1|koi8r] (default: %s)\n"
          "\n"
          "Standalone/client options:\n"
          "      --file <path>:          read commands from specified file\n"
          "      --input-fd <FD>:        read commands from specified file descriptor\n"
          "                              --file has a prioriry over --input-fd\n"
          "      --output-fd <FD>:       output response to specifid file descriptor\n"
          "  -p, --port <port number>:   specify server port number (client mode only)\n"
          "                              (default: %d)\n"
          "\n"
          "Server/daemon options:\n"
          "      --bind-address <ip/hostname>:\n"
          "                                specify server address to bind\n"
          "                                (default: %s)\n"
          "  -p, --port <port number>:     specify server port number (default: %d)\n"
          "  -i, --server-id <ip/hostname>:\n"
          "                                specify server ID address (default: %s)\n"
          "      --protocol <protocol>:    specify server protocol to listen\n"
          "                                [gqtp|http|memcached] (default: %s)\n"
          "      --document-root <path>:   specify document root path (http only)\n"
          "                                (default: %s)\n"
          "      --cache-limit <limit>:    specify max number of cache data (default: %u)\n"
          "  -t, --max-threads <max threads>:\n"
          "                                specify max number of threads (default: %u)\n"
          "      --pid-path <path>:        specify file to write process ID to\n"
          "                                (daemon mode only)\n"
          "\n"
          "Logging options:\n"
          "  -l, --log-level <log level>:\n"
          "                           specify log level (default: %d)\n"
          "      --log-path <path>:   specify log path\n"
          "                           (default: %s)\n"
          "      --query-log-path <path>:\n"
          "                           specify query log path\n"
          "                           (default: %s)\n"
          "\n"
          "Common options:\n"
          "      --config-path <path>:\n"
          "                       specify config file path\n"
          "                       (default: %s)\n"
          "      --default-command-version <version>:\n"
          "                       specify default command version (default: %d)\n"
          "      --default-match-escalation-threshold <threshold>:\n"
          "                       specify default match escalation threshold"
          " (default: %" GRN_FMT_LLD ")\n"
          "\n"
          "      --show-config:   show config\n"
          "  -h, --help:          show usage\n"
          "      --version:       show groonga version\n"
          "\n"
          "dest:\n"
          "  <db pathname> [<commands>]: in standalone mode\n"
          "  <db pathname>: in server/daemon mode\n"
          "  <dest hostname> [<commands>]: in client mode (default: %s)\n",
          grn_enctostr(default_encoding), default_port, default_bind_address,
          default_port, default_hostname, default_protocol,
          default_document_root, default_cache_limit, default_max_num_threads,
          default_log_level, default_log_path, default_query_log_path,
          default_config_path, default_default_command_version,
          (long long int)default_default_match_escalation_threshold,
          default_dest);
}

int
main(int argc, char **argv)
{
  const char *port_arg = NULL, *encoding_arg = NULL,
    *max_num_threads_arg = NULL, *log_level_arg = NULL,
    *bind_address_arg = NULL, *hostname_arg = NULL, *protocol_arg = NULL,
    *log_path_arg = NULL, *query_log_path_arg = NULL,
    *cache_limit_arg = NULL, *document_root_arg = NULL,
    *default_command_version_arg = NULL,
    *default_match_escalation_threshold_arg = NULL,
    *input_fd_arg = NULL, *output_fd_arg = NULL;
  const char *config_path = NULL;
  int exit_code = EXIT_SUCCESS;
  int i, mode = mode_alone;
  static grn_str_getopt_opt opts[] = {
    {'p', "port", NULL, 0, getopt_op_none},
    {'e', "encoding", NULL, 0, getopt_op_none},
    {'t', "max-threads", NULL, 0, getopt_op_none},
    {'h', "help", NULL, mode_usage, getopt_op_update},
    {'c', NULL, NULL, mode_client, getopt_op_update},
    {'d', NULL, NULL, mode_daemon, getopt_op_update},
    {'s', NULL, NULL, mode_server, getopt_op_update},
    {'l', "log-level", NULL, 0, getopt_op_none},
    {'i', "server-id", NULL, 0, getopt_op_none},
    {'q', NULL, NULL, MODE_USE_QL, getopt_op_on},
    {'n', NULL, NULL, MODE_NEW_DB, getopt_op_on},
    {'\0', "protocol", NULL, 0, getopt_op_none},
    {'\0', "version", NULL, mode_version, getopt_op_update},
    {'\0', "log-path", NULL, 0, getopt_op_none},
    {'\0', "query-log-path", NULL, 0, getopt_op_none},
    {'\0', "pid-path", NULL, 0, getopt_op_none},
    {'\0', "config-path", NULL, 0, getopt_op_none},
    {'\0', "show-config", NULL, mode_config, getopt_op_update},
    {'\0', "cache-limit", NULL, 0, getopt_op_none},
    {'\0', "file", NULL, 0, getopt_op_none},
    {'\0', "document-root", NULL, 0, getopt_op_none},
    {'\0', "default-command-version", NULL, 0, getopt_op_none},
    {'\0', "default-match-escalation-threshold", NULL, 0, getopt_op_none},
    {'\0', "bind-address", NULL, 0, getopt_op_none},
    {'\0', "input-fd", NULL, 0, getopt_op_none},
    {'\0', "output-fd", NULL, 0, getopt_op_none},
    {'\0', NULL, NULL, 0, 0}
  };
  opts[0].arg = &port_arg;
  opts[1].arg = &encoding_arg;
  opts[2].arg = &max_num_threads_arg;
  opts[7].arg = &log_level_arg;
  opts[8].arg = &hostname_arg;
  opts[11].arg = &protocol_arg;
  opts[13].arg = &log_path_arg;
  opts[14].arg = &query_log_path_arg;
  opts[15].arg = &pid_file_path;
  opts[16].arg = &config_path;
  opts[18].arg = &cache_limit_arg;
  opts[19].arg = &input_path;
  opts[20].arg = &document_root_arg;
  opts[21].arg = &default_command_version_arg;
  opts[22].arg = &default_match_escalation_threshold_arg;
  opts[23].arg = &bind_address_arg;
  opts[24].arg = &input_fd_arg;
  opts[25].arg = &output_fd_arg;

  init_default_settings();

  /* only for parsing --config-path. */
  i = grn_str_getopt(argc, argv, opts, &mode);
  if (i < 0) {
    show_usage(stderr);
    return EXIT_FAILURE;
  }

  if (config_path) {
    const config_file_status status = config_file_load(config_path, opts, &mode);
    if (status == CONFIG_FILE_FOPEN_ERROR) {
      fprintf(stderr, "%s: can't open config file: %s (%s)\n",
              argv[0], config_path, strerror(errno));
      return EXIT_FAILURE;
    } else if (status != CONFIG_FILE_SUCCESS) {
      fprintf(stderr, "%s: failed to parse config file: %s (%s)\n",
              argv[0], config_path,
              (status == CONFIG_FILE_FORMAT_ERROR) ? "Invalid format" : strerror(errno));
      return EXIT_FAILURE;
    }
  } else if (*default_config_path) {
    const config_file_status status =
        config_file_load(default_config_path, opts, &mode);
    if (status != CONFIG_FILE_SUCCESS && status != CONFIG_FILE_FOPEN_ERROR) {
      fprintf(stderr, "%s: failed to parse config file: %s (%s)\n",
              argv[0], default_config_path,
              (status == CONFIG_FILE_FORMAT_ERROR) ? "Invalid format" : strerror(errno));
      return EXIT_FAILURE;
    }
  }
  /* ignore mode option in config file */
  mode = (mode == mode_error) ? default_mode :
    ((mode & ~MODE_MASK) | default_mode);

  i = grn_str_getopt(argc, argv, opts, &mode);
  if (i < 0) { mode = mode_error; }
  switch (mode & MODE_MASK) {
  case mode_version :
    show_version();
    return EXIT_SUCCESS;
  case mode_usage :
    show_usage(output);
    return EXIT_SUCCESS;
  case mode_config :
    show_config(output, opts, mode & ~MODE_MASK);
    return EXIT_SUCCESS;
  case mode_error :
    show_usage(stderr);
    return EXIT_FAILURE;
  }

  if (port_arg) {
    const char * const end = port_arg + strlen(port_arg);
    const char *rest = NULL;
    const int value = grn_atoi(port_arg, end, &rest);
    if (rest != end || value <= 0 || value > 65535) {
      fprintf(stderr, "invalid port number: <%s>\n", port_arg);
      return EXIT_FAILURE;
    }
    port = value;
  } else {
    port = default_port;
  }

  if (encoding_arg) {
    switch (*encoding_arg) {
    case 'n' :
    case 'N' :
      encoding = GRN_ENC_NONE;
      break;
    case 'e' :
    case 'E' :
      encoding = GRN_ENC_EUC_JP;
      break;
    case 'u' :
    case 'U' :
      encoding = GRN_ENC_UTF8;
      break;
    case 's' :
    case 'S' :
      encoding = GRN_ENC_SJIS;
      break;
    case 'l' :
    case 'L' :
      encoding = GRN_ENC_LATIN1;
      break;
    case 'k' :
    case 'K' :
      encoding = GRN_ENC_KOI8R;
      break;
    default:
      encoding = GRN_ENC_DEFAULT;
      break;
    }
  } else {
    encoding = GRN_ENC_DEFAULT;
  }

  if (!grn_document_root) {
    grn_document_root = default_document_root;
  }

  if (protocol_arg) {
    switch (*protocol_arg) {
    case 'g' :
    case 'G' :
      do_client = g_client;
      do_server = g_server;
      break;
    case 'h' :
    case 'H' :
      do_client = g_client;
      do_server = h_server;
      break;
    case 'm' :
    case 'M' :
      do_client = g_client;
      do_server = g_server;
      break;
    default :
      do_client = g_client;
      do_server = g_server;
      break;
    }
  } else {
    do_client = g_client;
    do_server = g_server;
  }

  if (log_path_arg) {
    grn_log_path = log_path_arg;
  }

  if (query_log_path_arg) {
    grn_qlog_path = query_log_path_arg;
  }

  if (max_num_threads_arg) {
    const char * const end = max_num_threads_arg + strlen(max_num_threads_arg);
    const char *rest = NULL;
    const uint32_t value = grn_atoui(max_num_threads_arg, end, &rest);
    if (end != rest || value < 1 || value > 100) {
      fprintf(stderr, "invalid max number of threads: <%s>\n",
              max_num_threads_arg);
      return EXIT_FAILURE;
    }
    max_nfthreads = value;
  } else {
    max_nfthreads = default_max_num_threads;
  }

  if (input_path) {
    if (!freopen(input_path, "r", stdin)) {
      fprintf(stderr, "can't open input file: %s (%s)\n",
              input_path, strerror(errno));
      return EXIT_FAILURE;
    }
    batchmode = GRN_TRUE;
  } else {
    if (input_fd_arg) {
      const char * const end = input_fd_arg + strlen(input_fd_arg);
      const char *rest = NULL;
      const int input_fd = grn_atoi(input_fd_arg, end, &rest);
      if (rest != end || input_fd == 0) {
        fprintf(stderr, "invalid input FD: <%s>\n", input_fd_arg);
        return EXIT_FAILURE;
      }
      if (dup2(input_fd, STDIN_FILENO) == -1) {
        fprintf(stderr, "can't open input FD: %d (%s)\n",
                input_fd, strerror(errno));
        return EXIT_FAILURE;
      }
      batchmode = GRN_TRUE;
    } else {
      if (argc - i > 1) {
        batchmode = GRN_TRUE;
      } else {
        batchmode = !isatty(0);
      }
    }
  }

  if (output_fd_arg) {
    const char * const end = output_fd_arg + strlen(output_fd_arg);
    const char *rest = NULL;
    const int output_fd = grn_atoi(output_fd_arg, end, &rest);
    if (rest != end || output_fd == 0) {
      fprintf(stderr, "invalid output FD: <%s>\n", output_fd_arg);
      return EXIT_FAILURE;
    }
    output = fdopen(output_fd, "w");
    if (!output) {
      fprintf(stderr, "can't open output FD: %d (%s)\n",
              output_fd, strerror(errno));
      return EXIT_FAILURE;
    }
  }


  if (bind_address_arg) {
    const size_t bind_address_length = strlen(bind_address_arg);
    if (bind_address_length > HOST_NAME_MAX) {
      fprintf(stderr, "too long bind address: %s (%u bytes):"
                      " must not be longer than %u bytes\n",
              bind_address_arg, (unsigned int)bind_address_length, HOST_NAME_MAX);
      return EXIT_FAILURE;
    }
    strcpy(bind_address, bind_address_arg);
  } else {
    strcpy(bind_address, default_bind_address);
  }

  if (hostname_arg) {
    const size_t hostname_length = strlen(hostname_arg);
    if (hostname_length > HOST_NAME_MAX) {
      fprintf(stderr, "too long hostname: %s (%u bytes):"
                      " must not be longer than %u bytes\n",
              hostname_arg, (unsigned int)hostname_length, HOST_NAME_MAX);
      return EXIT_FAILURE;
    }
    strcpy(hostname, hostname_arg);
  } else {
    strcpy(hostname, default_hostname);
  }

  if (document_root_arg) {
    grn_document_root = document_root_arg;
  }

  if (default_command_version_arg) {
    const char * const end = default_command_version_arg
        + strlen(default_command_version_arg);
    const char *rest = NULL;
    const int value = grn_atoi(default_command_version_arg, end, &rest);
    if (end != rest || value < GRN_COMMAND_VERSION_MIN ||
        value > GRN_COMMAND_VERSION_MAX) {
      fprintf(stderr, "invalid command version: <%s>\n",
              default_command_version_arg);
      return EXIT_FAILURE;
    }
    switch (value) {
    case 1 :
      default_command_version = GRN_COMMAND_VERSION_1;
      break;
    case 2 :
      default_command_version = GRN_COMMAND_VERSION_2;
      break;
    default :
      fprintf(stderr, "invalid command version: <%s>\n",
              default_command_version_arg);
      return EXIT_FAILURE;
    }
  } else {
    default_command_version = default_default_command_version;
  }

  if (default_match_escalation_threshold_arg) {
    const char * const end = default_match_escalation_threshold_arg
        + strlen(default_match_escalation_threshold_arg);
    const char *rest = NULL;
    const int64_t value = grn_atoll(default_match_escalation_threshold_arg, end, &rest);
    if (end != rest || value < 0) {
      fprintf(stderr, "invalid match escalation threshold: <%s>\n",
              default_match_escalation_threshold_arg);
      return EXIT_FAILURE;
    }
    default_match_escalation_threshold = value;
  } else {
    default_match_escalation_threshold = default_default_match_escalation_threshold;
  }

  if (log_level_arg) {
    const char * const end = log_level_arg + strlen(log_level_arg);
    const char *rest = NULL;
    const int value = grn_atoi(log_level_arg, end, &rest);
    if (end != rest || value < 0 || value > 9) {
      fprintf(stderr, "invalid log level: <%s>\n", log_level_arg);
      return EXIT_FAILURE;
    }
    log_level = value;
  } else {
    log_level = default_log_level;
  }

  if (cache_limit_arg) {
    const char * const end = cache_limit_arg + strlen(cache_limit_arg);
    const char *rest = NULL;
    const uint32_t value = grn_atoui(cache_limit_arg, end, &rest);
    if (end != rest) {
      fprintf(stderr, "invalid --cache-limit value: <%s>\n", cache_limit_arg);
      return EXIT_FAILURE;
    }
    cache_limit = value;
  } else {
    cache_limit = default_cache_limit;
  }

#ifdef WITH_LIBEDIT
  if (!batchmode) {
    line_editor_init(argc, argv);
  }
#endif
  if (grn_init()) { return EXIT_FAILURE; }

  grn_set_default_encoding(encoding);

  if (default_command_version_arg) {
    grn_set_default_command_version(default_command_version);
  }

  if (default_match_escalation_threshold_arg) {
    grn_set_default_match_escalation_threshold(default_match_escalation_threshold);
  }

  if (log_level_arg) {
    static grn_logger_info logger_info;
    logger_info.max_level = log_level;
    logger_info.flags = GRN_LOG_TIME | GRN_LOG_MESSAGE;
    logger_info.func = NULL;
    logger_info.func_arg = NULL;
    grn_logger_info_set(&grn_gctx, &logger_info);
  }

  grn_set_segv_handler();
  grn_set_int_handler();
  grn_set_term_handler();

  if (cache_limit_arg) {
    *grn_cache_max_nentries() = cache_limit;
  }

  newdb = (mode & MODE_NEW_DB);
  useql = (mode & MODE_USE_QL);
  switch (mode & MODE_MASK) {
  case mode_alone :
    exit_code = do_alone(argc - i, argv + i);
    break;
  case mode_client :
    exit_code = do_client(argc - i, argv + i);
    break;
  case mode_daemon :
    is_daemon_mode = GRN_TRUE;
    /* fallthru */
  case mode_server :
    exit_code = do_server(argc > i ? argv[i] : NULL);
    break;
  default:
    exit_code = EXIT_FAILURE;
    break;
  }

#ifdef WITH_LIBEDIT
  if (!batchmode) {
    line_editor_fin();
  }
#endif
  if (output != stdout) {
    fclose(output);
  }
  grn_fin();
  return exit_code;
}
