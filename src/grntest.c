/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#define __USE_XOPEN
#include "lib/str.h"
#include "lib/com.h"
#include "lib/db.h"

#ifdef WIN32
#include <windows.h>
#include <stddef.h>
#else
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <libgen.h>
#endif /* WIN32 */

/*
#define DEBUG_FTP
#define DEBUG_HTTP
*/

#define FTPUSER "anonymous"
#define FTPPASSWD "grntest"
#define FTPSERVER "ftp.groonga.org"
#define FTPBUF 20000
#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"

#define OUT_JSON 0
#define OUT_TSV  1

static int grntest_outtype = OUT_JSON;

static grn_critical_section grntest_cs;

static int grntest_stop_flag = 0;
static int grntest_detail_on = 0;
static int grntest_remote_mode = 0;
static int grntest_localonly_mode = 0;
static int grntest_owndb_mode = 0;
static int grntest_onmemory_mode = 0;
static int grntest_noftp_mode = 0;
#define TMPFILE "_grntest.tmp"

static grn_ctx grntest_server_context;
FILE *grntest_logfp;

#define OS_LINUX64   "LINUX64"
#define OS_LINUX32   "LINUX32"
#define OS_WINDOWS64 "WINDOWS64"
#define OS_WINDOWS32 "WINDOWS32"

#ifdef WIN32
typedef SOCKET socket_t;
#define SOCKETERROR INVALID_SOCKET
#define socketclose closesocket
#define GROONGA_PATH "groonga.exe"
PROCESS_INFORMATION grntest_pi;
#else
pid_t grntest_server_id = 0;
typedef int socket_t;
#define socketclose close
#define SOCKETERROR -1
#endif /* WIN32 */

char *grntest_osinfo;



grn_obj *grntest_db = NULL;

#define MAX_CON_JOB 10
#define MAX_CON 64

#define MAX_COMMAND 1000000
#define BUF_LEN 1024
#define MAX_PATH_LEN 256
#define MAX_COMMAND_LEN 500000
#define LOGBUF_LEN 200000

#define J_DO_LOCAL  1  /* do_local */
#define J_DO_GQTP   2  /* do_gqtp */
#define J_DO_HTTP   3  /* do_http */
#define J_REP_LOCAL 4  /* rep_local */
#define J_REP_GQTP  5  /* rep_gqtp */
#define J_REP_HTTP  6  /* rep_http */
#define J_OUT_LOCAL 7  /* out_local */
#define J_OUT_GQTP  8  /* out_gqtp */
#define J_OUT_HTTP  9  /* out_http */
#define J_TEST_LOCAL 10  /* test_local */
#define J_TEST_GQTP  11  /* test_gqtp */
#define J_TEST_HTTP  12  /* test_http */

static char grntest_username[BUF_LEN];
static char grntest_scriptname[BUF_LEN];
static char grntest_date[BUF_LEN];
static char grntest_serverhost[BUF_LEN];
static char grntest_log_tmpbuf[LOGBUF_LEN];
static int grntest_serverport;
static const char *grntest_dbpath;

struct commandtable {
  char *command[MAX_COMMAND];
  int num;
};

struct job {
  char jobname[BUF_LEN];
  char commandfile[BUF_LEN];
  int qnum;
  int jobtype;
  int concurrency;
  int ntimes;
  int done;
  long long int max;
  long long int min;
  FILE *outputlog;
  FILE *inputlog;
  char logfile[BUF_LEN];
};

struct task {
  char *file;
  struct commandtable *table;
  int jobtype;
  int ntimes;
  int qnum;
  int job_id;
  long long int max;
  long long int min;
  socket_t http_socket;
  grn_obj http_response;
};

static struct task grntest_task[MAX_CON];
static struct job grntest_job[MAX_CON];
static int grntest_jobdone;
static int grntest_jobnum;
static grn_ctx grntest_ctx[MAX_CON];
static grn_obj *grntest_owndb[MAX_CON];

grn_obj grntest_starttime, grntest_jobs_start;

static
int
grntest_atoi(const char *str, const char *end, const char **rest)
{
  while (grn_isspace(str, GRN_ENC_UTF8) == 1) {
    str++;
  }
  return grn_atoi(str, end, rest);
}

static
int
out_p(int jobtype)
{
  if (jobtype == J_OUT_LOCAL) {
    return 1;
  }
  if (jobtype == J_OUT_GQTP) {
    return 1;
  }
  if (jobtype == J_OUT_HTTP) {
    return 1;
  }
  return 0;
}

static
int
test_p(int jobtype)
{
  if (jobtype == J_TEST_LOCAL) {
    return 1;
  }
  if (jobtype == J_TEST_GQTP) {
    return 1;
  }
  if (jobtype == J_TEST_HTTP) {
    return 1;
  }
  return 0;
}

static
int
report_p(int jobtype)
{
  if (jobtype == J_REP_LOCAL) {
    return 1;
  }
  if (jobtype == J_REP_GQTP) {
    return 1;
  }
  if (jobtype == J_REP_HTTP) {
    return 1;
  }
  return 0;
}

static
int
gqtp_p(int jobtype)
{
  if (jobtype == J_DO_GQTP) {
    return 1;
  }
  if (jobtype == J_REP_GQTP) {
    return 1;
  }
  if (jobtype == J_OUT_GQTP) {
    return 1;
  }
  if (jobtype == J_TEST_GQTP) {
    return 1;
  }
  return 0;
}

static
int
http_p(int jobtype)
{
  if (jobtype == J_DO_HTTP) {
    return 1;
  }
  if (jobtype == J_REP_HTTP) {
    return 1;
  }
  if (jobtype == J_OUT_HTTP) {
    return 1;
  }
  if (jobtype == J_TEST_HTTP) {
    return 1;
  }
  return 0;
}

static
int
error_exit_in_thread(intptr_t code)
{
  fprintf(stderr, "Fatal error! Check script file or database!\n");
  fflush(stderr);
  CRITICAL_SECTION_ENTER(grntest_cs);
  grntest_stop_flag = 1;
  CRITICAL_SECTION_LEAVE(grntest_cs);
#ifdef WIN32
  _endthreadex(code);
#else
  pthread_exit((void *)code);
#endif /* WIN32 */
  return 0;
}


static
int
escape_command(char *in, int ilen,  char *out, int olen)
{
  int i = 0, j = 0;

  while (i < ilen) {
    if (j >= olen) {
      fprintf(stderr, "too many escape:%s\n", in);
      exit(1);
    }

    if ((in[i] == '\\') || (in[i] == '\"') || (in[i] == '/')) {
      out[j] = 0x5C;
      j++;
      out[j] = in[i];
      j++;
      i++;
    } else {
      switch (in[i]) {
      case '\b':
        out[j] = 0x5C;
        j++;
        out[j] = 'b';
        i++;
        break;
      case '\f':
        out[j] = 0x5C;
        j++;
        out[j] = 'f';
        j++;
        i++;
        break;
      case '\n':
        out[j] = 0x5C;
        j++;
        out[j] = 'n';
        j++;
        i++;
        break;
      case '\r':
        out[j] = 0x5C;
        j++;
        out[j] = 'r';
        j++;
        i++;
        break;
      case '\t':
        out[j] = 0x5C;
        j++;
        out[j] = 't';
        j++;
        i++;
        break;
      default:
        out[j] = in[i];
        j++;
        i++;
        break;
      }
    }
  }
  out[j] = '\0';
  return j;
}


static
int
report_load_command(grn_ctx *ctx, char *ret, int task_id, long long int start_time,
                    grn_obj *end_time)
{
  int i, len;
  long long int start, end;
  char rettmp[BUF_LEN];

  if ((ret[0] == '[') && (ret[1] == '[')) {
    i = 2;
    len = 1;
    while (ret[i] != ']') {
      i++;
      len++;
      if (ret[i] == '\0') {
        fprintf(stderr, "Error results:load\n");
        error_exit_in_thread(3);
      }
    }
    len++;
    strncpy(rettmp, &ret[1], len);
    rettmp[len] = '\0';
  } else {
    strcpy(rettmp, ret);
  }

  start = start_time - GRN_TIME_VALUE(&grntest_starttime);
  end = GRN_TIME_VALUE(end_time) - GRN_TIME_VALUE(&grntest_starttime);
  if (grntest_outtype == OUT_TSV) {
    fprintf(grntest_logfp, "report\t%d\tload\t%lld\t%lld\t%s\n",
            task_id,  start, end, rettmp);
  } else {
    fprintf(grntest_logfp, "[%d, \"load\", %lld, %lld, %s],\n",
            task_id,  start, end, rettmp);
  }
  fflush(grntest_logfp);
  return 0;
}

static
int
report_command(grn_ctx *ctx, char *command, char *ret, int task_id,
               grn_obj *start_time, grn_obj *end_time)
{
  int i, len, clen;
  long long int start, end;
  char rettmp[BUF_LEN];
  char command_escaped[MAX_COMMAND_LEN * 2];

  if ((ret[0] == '[') && (ret[1] == '[')) {
    i = 2;
    len = 1;
    while (ret[i] != ']') {
      i++;
      len++;
      if (ret[i] == '\0') {
        fprintf(stderr, "Error results:command=[%s]\n", command);
        error_exit_in_thread(3);
      }
    }
    len++;
    strncpy(rettmp, &ret[1], len);
    rettmp[len] = '\0';
  } else {
    strcpy(rettmp, ret);
  }

  start = GRN_TIME_VALUE(start_time) - GRN_TIME_VALUE(&grntest_starttime);
  end = GRN_TIME_VALUE(end_time) - GRN_TIME_VALUE(&grntest_starttime);
  clen = strlen(command);
  escape_command(command, clen, command_escaped, clen * 2);
  if (grntest_outtype == OUT_TSV) {
    fprintf(grntest_logfp, "report\t%d\t%s\t%lld\t%lld\t%s\n",
            task_id, command_escaped, start, end, rettmp);
  } else {
    fprintf(grntest_logfp, "[%d, \"%s\", %lld, %lld, %s],\n",
            task_id, command_escaped, start, end, rettmp);
  }
  fflush(grntest_logfp);
  return 0;
}

static
int
output_result_final(grn_ctx *ctx, int qnum)
{
  grn_obj end_time;
  long long int latency, self;
  double sec, qps;

  GRN_TIME_INIT(&end_time, 0);
  GRN_TIME_NOW(ctx, &end_time);

  latency = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&grntest_starttime);
  self = latency;
  sec = self / (double)1000000;
  qps = (double)qnum / sec;
  if (grntest_outtype == OUT_TSV) {
    fprintf(grntest_logfp, "total\t%lld\t%f\t%d\n", latency, qps, qnum);
  } else {
    fprintf(grntest_logfp,
           "{\"total\": %lld, \"qps\": %f, \"queries\": %d}]\n", latency, qps, qnum);
  }
  grn_obj_close(ctx, &end_time);
  return 0;
}

static
int
output_sysinfo(char *sysinfo)
{
  if (grntest_outtype == OUT_TSV) {
    fprintf(grntest_logfp, "%s", sysinfo);
  } else {
    fprintf(grntest_logfp, "[%s\n", sysinfo);
  }
  return 0;
}

static
int
shutdown_server(void)
{
  char *res;
  int flags, res_len;

  if (grntest_remote_mode) {
    return 0;
  }
  grn_ctx_send(&grntest_server_context, "shutdown", 8, 0);
  if (grntest_server_context.rc) {
    fprintf(stderr, "ctx_send:rc=%d\n", grntest_server_context.rc);
    exit(1);
  }
  grn_ctx_recv(&grntest_server_context, &res, &res_len, &flags);

  return 0;
}

static
int
error_command(grn_ctx *ctx, char *command, int task_id)
{
  fprintf(stderr, "error!:command=[%s] task_id = %d\n", command, task_id);
  fflush(stderr);
  error_exit_in_thread(1);
  return 0;
}

static
int
diff_result(char *expect, int elen, char *result, int rlen)
{
  int i;
  char *e = NULL, *r = NULL;

  i = 0;
  while(i < elen) {
    if (!strncmp(&expect[i], "],", 2)) {
      e = &expect[i] + 2;
      break;
    }
    i++;
  }
  if (e == NULL) {
/*
    fputc('[', stderr);
    fwrite(expect, 1, elen, stderr);
    fputc(']', stderr);
    fprintf(stderr, " is not groonga command output\n", );
    return 1;
*/
    e = expect;
  }

  i = 0;
  while(i < rlen) {
    if (!strncmp(&result[i], "],", 2)) {
      r = &result[i] + 2;
      break;
    }
    i++;
  }
  if (r == NULL) {
/*
    fputc('[', stderr);
    fwrite(result, 1, rlen, stderr);
    fputc(']', stderr);
    fprintf(stderr, " is not groonga command output\n");
    return 1;
*/
    r = result;
  }

  return strncmp(e, r, strlen(e));
}

static
socket_t
open_socket(char *host, int port)
{
  socket_t sock;
  struct hostent *servhost;
  struct sockaddr_in server;
  u_long inaddr;
  int ret;

  servhost = gethostbyname(host);
  if (servhost == NULL){
    fprintf(stderr, "Bad hostname [%s]\n", host);
    return -1;
  }
  inaddr = *(u_long*)(servhost->h_addr_list[0]);

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr = *(struct in_addr*)&inaddr;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    fprintf(stderr, "socket error\n");
    return -1;
  }
  ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
  if (ret == -1) {
    fprintf(stderr, "connect error\n");
    return -1;
  }
  return sock;
}

static
int
write_to_server(socket_t socket, char *buf)
{
#ifdef DEBUG_FTP
  fprintf(stderr, "send:%s", buf);
#endif
  send(socket, buf, strlen(buf), 0);
  return 0;
}

#define OUTPUT_TYPE "output_type"
#define OUTPUT_TYPE_LEN (sizeof(OUTPUT_TYPE) - 1)

static
void
command_line_to_uri_path(grn_ctx *ctx, grn_obj *uri, char *command)
{
  char tok_type;
  int offset = 0, have_key = 0;
  const char *p, *e, *v;
  grn_obj buf, *expr = NULL;
  grn_expr_var *vars;
  unsigned nvars;

  GRN_TEXT_INIT(&buf, 0);
  p = command;
  e = command + strlen(command);
  p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
  if ((expr = grn_ctx_get(ctx, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf)))) {
    grn_obj params, output_type;

    GRN_TEXT_INIT(&params, 0);
    GRN_TEXT_INIT(&output_type, 0);
    vars = ((grn_proc *)expr)->vars;
    nvars = ((grn_proc *)expr)->nvars;
    GRN_TEXT_PUTS(ctx, uri, "/d/");
    GRN_TEXT_PUT(ctx, uri, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
    while (p < e) {
      GRN_BULK_REWIND(&buf);
      p = grn_text_unesc_tok(ctx, &buf, p, e, &tok_type);
      v = GRN_TEXT_VALUE(&buf);
      switch (tok_type) {
      case GRN_TOK_VOID :
        p = e;
        break;
      case GRN_TOK_SYMBOL :
        if (GRN_TEXT_LEN(&buf) > 2 && v[0] == '-' && v[1] == '-') {
          int l = GRN_TEXT_LEN(&buf) - 2;
          v += 2;
          if (l == OUTPUT_TYPE_LEN && !memcmp(v, OUTPUT_TYPE, OUTPUT_TYPE_LEN)) {
            GRN_BULK_REWIND(&output_type);
            p = grn_text_unesc_tok(ctx, &output_type, p, e, &tok_type);
            break;
          }
          if (GRN_TEXT_LEN(&params)) {
            GRN_TEXT_PUTS(ctx, &params, "&");
          }
          grn_text_urlenc(ctx, &params, v, l);
          have_key = 1;
          break;
        }
        /* fallthru */
      case GRN_TOK_STRING :
      case GRN_TOK_QUOTE :
        if (!have_key) {
          if (offset < nvars) {
            if (GRN_TEXT_LEN(&params)) {
              GRN_TEXT_PUTS(ctx, &params, "&");
            }
            grn_text_urlenc(ctx, &params,
                            vars[offset].name, vars[offset].name_size);
            offset++;
          }
        }
        GRN_TEXT_PUTS(ctx, &params, "=");
        grn_text_urlenc(ctx, &params, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
        have_key = 0;
        break;
      }
    }
    GRN_TEXT_PUTS(ctx, uri, ".");
    if (GRN_TEXT_LEN(&output_type)) {
      GRN_TEXT_PUT(ctx, uri,
                   GRN_TEXT_VALUE(&output_type), GRN_TEXT_LEN(&output_type));
    } else {
      GRN_TEXT_PUTS(ctx, uri, "json");
    }
    if (GRN_TEXT_LEN(&params) > 0) {
      GRN_TEXT_PUTS(ctx, uri, "?");
      GRN_TEXT_PUT(ctx, uri, GRN_TEXT_VALUE(&params), GRN_TEXT_LEN(&params));
    }
    GRN_OBJ_FIN(ctx, &params);
    GRN_OBJ_FIN(ctx, &output_type);
  }
  GRN_OBJ_FIN(ctx, &buf);
}

static
void
command_send_http(grn_ctx *ctx, char *command, int type, int task_id)
{
  socket_t http_socket;
  grn_obj buf;

  http_socket = open_socket(grntest_serverhost, grntest_serverport);
  if (http_socket == SOCKETERROR) {
    fprintf(stderr, "failed to connect to groonga at %s:%d via HTTP: ",
            grntest_serverhost, grntest_serverport);
#ifdef WIN32
    fprintf(stderr, "%d\n", GetLastError());
#else
    fprintf(stderr, "%s\n", strerror(errno));
#endif
    error_exit_in_thread(100);
  }
  grntest_task[task_id].http_socket = http_socket;
  GRN_BULK_REWIND(&grntest_task[task_id].http_response);

  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_PUTS(ctx, &buf, "GET ");
  command_line_to_uri_path(ctx, &buf, command);
#ifdef DEBUG_HTTP
  fprintf(stderr, "command: <%s>\n", command);
  fprintf(stderr, "path:    <%.*s>\n",
          (int)GRN_TEXT_LEN(&buf), GRN_TEXT_VALUE(&buf));
#endif
  GRN_TEXT_PUTS(ctx, &buf, " HTTP/1.1\r\n");
  GRN_TEXT_PUTS(ctx, &buf, "Host: ");
  GRN_TEXT_PUTS(ctx, &buf, grntest_serverhost);
  GRN_TEXT_PUTS(ctx, &buf, "\r\n");
  GRN_TEXT_PUTS(ctx, &buf, "User-Agent: grntest/");
  GRN_TEXT_PUTS(ctx, &buf, grn_get_version());
  GRN_TEXT_PUTS(ctx, &buf, "\r\n");
  GRN_TEXT_PUTS(ctx, &buf, "Connection: close\r\n");
  GRN_TEXT_PUTS(ctx, &buf, "\r\n");
  GRN_TEXT_PUTC(ctx, &buf, '\0');
  write_to_server(http_socket, GRN_TEXT_VALUE(&buf));
  GRN_OBJ_FIN(ctx, &buf);
}

static
void
command_send_ctx(grn_ctx *ctx, char *command, int type, int task_id)
{
  grn_ctx_send(ctx, command, strlen(command), 0);
/* fix me.
   when command fails, ctx->rc is not 0 in local mode!
  if (ctx->rc) {
    fprintf(stderr, "ctx_send:rc=%d:command:%s\n", ctx->rc, command);
    error_exit_in_thread(1);
  }
*/
}

static
void
command_send(grn_ctx *ctx, char *command, int type, int task_id)
{
  if (http_p(type)) {
    command_send_http(ctx, command, type, task_id);
  } else {
    command_send_ctx(ctx, command, type, task_id);
  }
}

static
void
command_recv_http(grn_ctx *ctx, int type, int task_id,
                  char **res, int *res_len, int *flags)
{
  int len;
  char buf[BUF_LEN];
  char *p, *e;
  socket_t http_socket;
  grn_obj *http_response;

  http_socket = grntest_task[task_id].http_socket;
  http_response = &grntest_task[task_id].http_response;
  while ((len = recv(http_socket, buf, BUF_LEN - 1, 0))) {
#ifdef DEBUG_HTTP
    fprintf(stderr, "receive: <%.*s>\n", len, buf);
#endif
    GRN_TEXT_PUT(ctx, http_response, buf, len);
  }

  p = GRN_TEXT_VALUE(http_response);
  e = p + GRN_TEXT_LEN(http_response);
  while (p < e) {
    if (p[0] != '\r') {
      p++;
      continue;
    }
    if (e - p >= 4) {
      if (!memcmp(p, "\r\n\r\n", 4)) {
        *res = p + 4;
        *res_len = e - *res;
#ifdef DEBUG_HTTP
        fprintf(stderr, "body: <%.*s>\n", *res_len, *res);
#endif
        break;
      }
      p += 4;
    } else {
      *res = NULL;
      *res_len = 0;
      break;
    }
  }

  socketclose(http_socket);
  grntest_task[task_id].http_socket = 0;
}

static
void
command_recv_ctx(grn_ctx *ctx, int type, int task_id,
                 char **res, int *res_len, int *flags)
{
  grn_ctx_recv(ctx, res, res_len, flags);
  if (ctx->rc) {
    fprintf(stderr, "ctx_recv:rc=%d\n", ctx->rc);
    error_exit_in_thread(1);
  }
}

static
void
command_recv(grn_ctx *ctx, int type, int task_id,
             char **res, int *res_len, int *flags)
{
  if (http_p(type)) {
    command_recv_http(ctx, type, task_id, res, res_len, flags);
  } else {
    command_recv_ctx(ctx, type, task_id, res, res_len, flags);
  }
}

static
int
do_load_command(grn_ctx *ctx, char *command, int type, int task_id,
                long long int *load_start)
{
  char *res;
  int res_len, flags, ret;
  grn_obj start_time, end_time;

  if (*load_start == 0) {
    GRN_TIME_INIT(&start_time, 0);
    GRN_TIME_NOW(ctx, &start_time);
    *load_start = GRN_TIME_VALUE(&start_time);
    grn_obj_close(ctx, &start_time);
  }

  command_send(ctx, command, type, task_id);
  do {
    command_recv(ctx, type, task_id, &res, &res_len, &flags);
    if (res_len) {
      long long int self;
      GRN_TIME_INIT(&end_time, 0);
      GRN_TIME_NOW(ctx, &end_time);

      self = GRN_TIME_VALUE(&end_time) - *load_start;

      if (grntest_task[task_id].max < self) {
        grntest_task[task_id].max = self;
      }
      if (grntest_task[task_id].min > self) {
        grntest_task[task_id].min = self;
      }

      if (report_p(grntest_task[task_id].jobtype)) {
        unsigned char tmpbuf[BUF_LEN];

        if (res_len < BUF_LEN) {
          strncpy(tmpbuf, res, res_len);
          tmpbuf[res_len] = '\0';
        } else {
          strncpy(tmpbuf, res, BUF_LEN - 2);
          tmpbuf[BUF_LEN -2] = '\0';
        }
        report_load_command(ctx, tmpbuf, task_id, *load_start, &end_time);
      }
      if (out_p(grntest_task[task_id].jobtype)) {
        fwrite(res, 1, res_len, grntest_job[grntest_task[task_id].job_id].outputlog);
        fputc('\n', grntest_job[grntest_task[task_id].job_id].outputlog);
        fflush(grntest_job[grntest_task[task_id].job_id].outputlog);
      }
      if (test_p(grntest_task[task_id].jobtype)) {
        char logbuf[LOGBUF_LEN];
        int loglen;
        FILE *outfp = grntest_job[grntest_task[task_id].job_id].outputlog;
        logbuf[LOGBUF_LEN-2] = '\0';
        if (fgets(logbuf, LOGBUF_LEN,
                  grntest_job[grntest_task[task_id].job_id].inputlog) == NULL) {
          fprintf(stderr, "Cannot get input-log\n");
          error_exit_in_thread(55);
        }
        if (logbuf[LOGBUF_LEN-2] != '\0') {
          fprintf(stderr, "too long line input-log\n");
          error_exit_in_thread(56);
        }
        loglen = strlen(logbuf);
        loglen--;
        logbuf[loglen] = '\0';

        if (diff_result(logbuf, loglen, res, res_len)) {
          fprintf(outfp, "DIFF:command:%s\n", command);
          fprintf(outfp, "DIFF:result:");
          fwrite(res, 1, res_len, outfp);
          fputc('\n', outfp);
          fprintf(outfp, "DIFF:expect:%s\n", logbuf);
          fflush(outfp);
        }
      }
      grn_obj_close(ctx, &end_time);
      ret = 1;
      break;
    } else {
      ret = 0;
      break;
    }
  } while ((flags & GRN_CTX_MORE));

  return ret;
}


static
int
do_command(grn_ctx *ctx, char *command, int type, int task_id)
{
  char *res;
  int res_len, flags;
  grn_obj start_time, end_time;

  GRN_TIME_INIT(&start_time, 0);
  GRN_TIME_NOW(ctx, &start_time);

  command_send(ctx, command, type, task_id);
  do {
    command_recv(ctx, type, task_id, &res, &res_len, &flags);
    if (res_len) {
      long long int self;
      GRN_TIME_INIT(&end_time, 0);
      GRN_TIME_NOW(ctx, &end_time);

      self = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&start_time);

      if (grntest_task[task_id].max < self) {
        grntest_task[task_id].max = self;
      }
      if (grntest_task[task_id].min > self) {
        grntest_task[task_id].min = self;
      }

      if (report_p(grntest_task[task_id].jobtype)) {
        unsigned char tmpbuf[BUF_LEN];

        if (res_len < BUF_LEN) {
          strncpy(tmpbuf, res, res_len);
          tmpbuf[res_len] = '\0';
        } else {
          strncpy(tmpbuf, res, BUF_LEN - 2);
          tmpbuf[BUF_LEN -2] = '\0';
        }
        report_command(ctx, command, tmpbuf, task_id, &start_time, &end_time);
      }
      if (out_p(grntest_task[task_id].jobtype)) {
        fwrite(res, 1, res_len, grntest_job[grntest_task[task_id].job_id].outputlog);
        fputc('\n', grntest_job[grntest_task[task_id].job_id].outputlog);
        fflush(grntest_job[grntest_task[task_id].job_id].outputlog);
      }
      if (test_p(grntest_task[task_id].jobtype)) {
        char logbuf[LOGBUF_LEN];
        int loglen;
        FILE *outfp = grntest_job[grntest_task[task_id].job_id].outputlog;
        logbuf[LOGBUF_LEN-2] = '\0';
        if (fgets(logbuf, LOGBUF_LEN,
                  grntest_job[grntest_task[task_id].job_id].inputlog) == NULL) {
          fprintf(stderr, "Cannot get inputlog\n");
          error_exit_in_thread(55);
        }
        if (logbuf[LOGBUF_LEN-2] != '\0') {
          fprintf(stderr, "too long line input-log\n");
          error_exit_in_thread(56);
        }
        loglen = strlen(logbuf);
        loglen--;
        logbuf[loglen] = '\0';

        if (diff_result(logbuf, loglen, res, res_len)) {
          fprintf(outfp, "DIFF:command:%s\n", command);
          fprintf(outfp, "DIFF:result:");
          fwrite(res, 1, res_len, outfp);
          fputc('\n', outfp);
          fprintf(outfp, "DIFF:expect:%s\n", logbuf);
          fflush(outfp);
        }
      }
      grn_obj_close(ctx, &end_time);
      break;
    } else {
/*
      error_command(ctx, command, task_id);
*/
    }
  } while ((flags & GRN_CTX_MORE));

  grn_obj_close(ctx, &start_time);

  return 0;
}

static
int
comment_p(char *command)
{
  if (command[0] == '#') {
    return 1;
  }
  return 0;
}

static
int
load_command_p(char *command)
{
  int i = 0;

  while (grn_isspace(&command[i], GRN_ENC_UTF8) == 1) {
    i++;
  }
  if (command[i] == '\0') {
    return 0;
  }
  if (!strncmp(&command[i], "load", 4)) {
    return 1;
  }
  return 0;
}

static
int
worker_sub(intptr_t task_id)
{
  int i, load_mode, load_count;
  grn_obj end_time;
  long long int latency, self;
  double sec, qps;
  long long int load_start;

  grntest_task[task_id].max = 0LL;
  grntest_task[task_id].min = 9223372036854775807LL;
  grntest_task[task_id].qnum = 0;

  for (i = 0; i < grntest_task[task_id].ntimes; i++) {
    if (grntest_task[task_id].file != NULL) {
      FILE *fp;
      char tmpbuf[MAX_COMMAND_LEN];
      fp = fopen(grntest_task[task_id].file, "r");
      if (!fp) {
        fprintf(stderr, "Cannot open %s\n",grntest_task[task_id].file);
        error_exit_in_thread(1);
      }
      tmpbuf[MAX_COMMAND_LEN-2] = '\0';
      load_mode = 0;
      load_count = 0;
      load_start = 0LL;
      while (fgets(tmpbuf, MAX_COMMAND_LEN, fp) != NULL) {
        if (tmpbuf[MAX_COMMAND_LEN-2] != '\0') {
          fprintf(stderr, "Too long commmand in %s\n",grntest_task[task_id].file);
          error_exit_in_thread(1);
        }
        tmpbuf[strlen(tmpbuf)-1] = '\0';
        if (comment_p(tmpbuf)) {
          continue;
        }
        if (tmpbuf[0] == '\0') {
          continue;
        }
        if (load_command_p(tmpbuf)) {
          load_mode = 1;
          load_count = 1;
        }
        if (load_mode == 1) {
          if (do_load_command(&grntest_ctx[task_id], tmpbuf,
                              grntest_task[task_id].jobtype,
                              task_id, &load_start)) {
            grntest_task[task_id].qnum += load_count;
            load_mode = 0;
            load_count = 0;
            load_start = 0LL;
          }
          load_count++;
          continue;
        }
        do_command(&grntest_ctx[task_id], tmpbuf,
                     grntest_task[task_id].jobtype,
                     task_id);
        grntest_task[task_id].qnum++;
      }
      fclose(fp);
    } else {
      int line;
      if (grntest_task[task_id].table == NULL) {
        error_exit_in_thread(1);
      }
      load_mode = 0;
      for (line = 0; line < grntest_task[task_id].table->num; line++) {
        if (load_command_p(grntest_task[task_id].table->command[line])) {
          load_mode = 1;
        }
        if (load_mode == 1) {
          if (do_load_command(&grntest_ctx[task_id],
                              grntest_task[task_id].table->command[line],
                              grntest_task[task_id].jobtype, task_id, &load_start)) {
            load_mode = 0;
            load_start = 0LL;
            grntest_task[task_id].qnum++;
          }
          continue;
        }
        do_command(&grntest_ctx[task_id],
                   grntest_task[task_id].table->command[line],
                   grntest_task[task_id].jobtype, task_id);
        grntest_task[task_id].qnum++;
      }
    }
  }

  GRN_TIME_INIT(&end_time, 0);
  GRN_TIME_NOW(&grntest_ctx[task_id], &end_time);
  latency = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&grntest_starttime);
  self = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&grntest_jobs_start);

  CRITICAL_SECTION_ENTER(grntest_cs);
  if (grntest_job[grntest_task[task_id].job_id].max < grntest_task[task_id].max) {
    grntest_job[grntest_task[task_id].job_id].max = grntest_task[task_id].max;
  }
  if (grntest_job[grntest_task[task_id].job_id].min > grntest_task[task_id].min) {
    grntest_job[grntest_task[task_id].job_id].min = grntest_task[task_id].min;
  }

  grntest_job[grntest_task[task_id].job_id].qnum += grntest_task[task_id].qnum;
  grntest_job[grntest_task[task_id].job_id].done++;
  if (grntest_job[grntest_task[task_id].job_id].done ==
      grntest_job[grntest_task[task_id].job_id].concurrency) {
    char tmpbuf[BUF_LEN];
    sec = self / (double)1000000;
    qps = (double)grntest_job[grntest_task[task_id].job_id].qnum/ sec;
    grntest_jobdone++;
    if (grntest_outtype == OUT_TSV) {
      sprintf(tmpbuf,
              "job\t%s\t%lld\t%lld\t%f\t%lld\t%lld\t%d\n",
              grntest_job[grntest_task[task_id].job_id].jobname, latency, self, qps,
              grntest_job[grntest_task[task_id].job_id].min,
              grntest_job[grntest_task[task_id].job_id].max,
              grntest_job[grntest_task[task_id].job_id].qnum);
    } else {
      sprintf(tmpbuf,
              "{\"job\": \"%s\", \"latency\": %lld, \"self\": %lld, \"qps\": %f, \"min\": %lld, \"max\": %lld, \"queries\": %d}",
              grntest_job[grntest_task[task_id].job_id].jobname, latency, self, qps,
              grntest_job[grntest_task[task_id].job_id].min,
              grntest_job[grntest_task[task_id].job_id].max,
              grntest_job[grntest_task[task_id].job_id].qnum);
      if (grntest_jobdone < grntest_jobnum) {
        strcat(tmpbuf, ",");
      }
    }
    strcat(grntest_log_tmpbuf, tmpbuf);
    if (grntest_log_tmpbuf[LOGBUF_LEN - 2] != '\0') {
      error_exit_in_thread(1);
    }
    if (grntest_jobdone == grntest_jobnum) {
      if (grntest_outtype == OUT_TSV) {
        fprintf(grntest_logfp, "%s", grntest_log_tmpbuf);
      } else {
        if (grntest_detail_on) {
          fseek(grntest_logfp, -2, SEEK_CUR);
          fprintf(grntest_logfp, "], \n");
        }
        fprintf(grntest_logfp, "\"summary\" :[");
        fprintf(grntest_logfp, "%s", grntest_log_tmpbuf);
        fprintf(grntest_logfp, "]");
      }
      fflush(grntest_logfp);
    }
  }
  grn_obj_close(&grntest_ctx[task_id], &end_time);
  CRITICAL_SECTION_LEAVE(grntest_cs);

  return 0;
}

#ifdef WIN32
static
int
__stdcall
worker(void *val)
{
  worker_sub((intptr_t) val);
  return 0;
}
#else
static
void *
worker(void *val)
{
  worker_sub((intptr_t) val);
  return NULL;
}
#endif /* WIN32 */

#ifdef WIN32
int
thread_main(grn_ctx *ctx, int num)
{
  int  i;
  int  ret;
  HANDLE pthread[MAX_CON];

  for (i = 0; i < num; i++) {
    pthread[i] = (HANDLE)_beginthreadex(NULL, 0, worker, (void *)i,
                                        0, NULL);
    if (pthread[i]== (HANDLE)0) {
       fprintf(stderr, "thread failed:%d\n", i);
       error_exit_in_thread(1);
    }
  }

  ret = WaitForMultipleObjects(num, pthread, TRUE, INFINITE);
  if (ret == WAIT_TIMEOUT) {
     fprintf(stderr, "timeout\n");
     error_exit_in_thread(1);
  }

  for (i = 0; i < num; i++) {
    CloseHandle(pthread[i]);
  }
  return 0;
}
#else
int
thread_main(grn_ctx *ctx, int num)
{
  intptr_t i;
  int ret;
  pthread_t pthread[MAX_CON];

  for (i = 0; i < num; i++) {
    ret = pthread_create(&pthread[i], NULL, worker, (void *)i);
    if (ret) {
      fprintf(stderr, "Cannot create thread:ret=%d\n", ret);
      error_exit_in_thread(1);
    }
  }

  for (i = 0; i < num; i++) {
    ret = pthread_join(pthread[i], NULL);
    if (ret) {
      fprintf(stderr, "Cannot join thread:ret=%d\n", ret);
      error_exit_in_thread(1);
    }
  }
  return 0;
}
#endif

static
int
error_exit(grn_ctx *ctx, int ret)
{
  fflush(stderr);
  shutdown_server();
  grn_ctx_fin(ctx);
  grn_fin();
  exit(ret);
}

static
int
get_sysinfo(const char *path, char *result, int olen)
{
  char tmpbuf[256];

#ifdef WIN32
  int cinfo[4];
  ULARGE_INTEGER dinfo;
  char cpustring[64];
  SYSTEM_INFO sinfo;
  MEMORYSTATUSEX minfo;
  OSVERSIONINFO osinfo;

  if (grntest_outtype == OUT_TSV) {
    result[0] = '\0';
    sprintf(tmpbuf, "script\t%s\n", grntest_scriptname);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "user\t%s\n", grntest_username);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "date\t%s\n", grntest_date);
    strcat(result, tmpbuf);
  } else {
    strcpy(result, "{");
    sprintf(tmpbuf, "\"script\": \"%s.scr\",\n", grntest_scriptname);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "  \"user\": \"%s\",\n", grntest_username);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "  \"date\": \"%s\",\n", grntest_date);
    strcat(result, tmpbuf);
  }

  memset(cpustring, 0, 64);
#ifndef __GNUC__
  __cpuid(cinfo, 0x80000002);
  memcpy(cpustring, cinfo, 16);
  __cpuid(cinfo, 0x80000003);
  memcpy(cpustring+16, cinfo, 16);
  __cpuid(cinfo, 0x80000004);
  memcpy(cpustring+32, cinfo, 16);
#endif

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s\n", cpustring);
  } else {
    sprintf(tmpbuf, "  \"CPU\": \"%s\",\n", cpustring);
  }
  strcat(result, tmpbuf);

  if (sizeof(int *) == 8) {
    grntest_osinfo = OS_WINDOWS64;
    if (grntest_outtype == OUT_TSV) {
      sprintf(tmpbuf, "64BIT\n");
    } else {
      sprintf(tmpbuf, "  \"BIT\": 64,\n");
    }
  } else {
    grntest_osinfo = OS_WINDOWS32;
    if (grntest_outtype == OUT_TSV) {
      sprintf(tmpbuf, "32BIT\n");
    } else {
      sprintf(tmpbuf, "  \"BIT\": 32,\n");
    }
  }
  strcat(result, tmpbuf);

  GetSystemInfo(&sinfo);
  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "CORE\t%d\n", sinfo.dwNumberOfProcessors);
  } else {
    sprintf(tmpbuf, "  \"CORE\": %d,\n", sinfo.dwNumberOfProcessors);
  }
  strcat(result, tmpbuf);

  minfo.dwLength = sizeof(minfo);
  GlobalMemoryStatusEx(&minfo);
  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "RAM\t%I64dMByte\n", minfo.ullTotalPhys/(1024*1024));
  } else {
    sprintf(tmpbuf, "  \"RAM\": \"%I64dMByte\",\n", minfo.ullTotalPhys/(1024*1024));
  }
  strcat(result, tmpbuf);

  GetDiskFreeSpaceEx(NULL, NULL, &dinfo, NULL);
  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "HDD\t%I64dKBytes\n", dinfo.QuadPart/1024 );
  } else {
    sprintf(tmpbuf, "  \"HDD\": \"%I64dKBytes\",\n", dinfo.QuadPart/1024 );
  }
  strcat(result, tmpbuf);

  osinfo.dwOSVersionInfoSize = sizeof(osinfo); GetVersionEx(&osinfo);
  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "Windows %d.%d\n", osinfo.dwMajorVersion, osinfo.dwMinorVersion);
  } else {
    sprintf(tmpbuf, "  \"OS\": \"Windows %d.%d\",\n", osinfo.dwMajorVersion,
            osinfo.dwMinorVersion);
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s\n", grntest_serverhost);
  } else {
    sprintf(tmpbuf, "  \"HOST\": \"%s\",\n", grntest_serverhost);
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%d\n", grntest_serverport);
  } else {
    sprintf(tmpbuf, "  \"PORT\": \"%d\",\n", grntest_serverport);
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s\"\n", grn_get_version());
  } else {
    sprintf(tmpbuf, "  \"VERSION\": \"%s\"\n", grn_get_version());
  }

  strcat(result, tmpbuf);
  if (grntest_outtype != OUT_TSV) {
    strcat(result, "}");
  }

#else /* linux only */
  FILE *fp;
  int ret;
  int cpunum = 0;
  int minfo = 0;
  char cpustring[256];
  struct utsname ubuf;
  struct statvfs vfsbuf;

  if (grntest_outtype == OUT_TSV) {
    result[0] = '\0';
    sprintf(tmpbuf, "sctipt\t%s\n", grntest_scriptname);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "user\t%s\n", grntest_username);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "date\t%s\n", grntest_date);
    strcat(result, tmpbuf);
  } else {
    strcpy(result, "{");
    sprintf(tmpbuf, "\"script\": \"%s.scr\",\n", grntest_scriptname);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "  \"user\": \"%s\",\n", grntest_username);
    strcat(result, tmpbuf);
    sprintf(tmpbuf, "  \"date\": \"%s\",\n", grntest_date);
    strcat(result, tmpbuf);
  }

  fp = fopen("/proc/cpuinfo", "r");
  if (!fp) {
    fprintf(stderr, "Cannot open cpuinfo\n");
    exit(1);
  }
  while (fgets(tmpbuf, 256, fp) != NULL) {
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    if (!strncmp(tmpbuf, "model name\t: ", 13)) {
      strcpy(cpustring, &tmpbuf[13]);
    }
  }
  fclose(fp);
  cpunum = sysconf(_SC_NPROCESSORS_CONF);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s\n", cpustring);
  } else {
    sprintf(tmpbuf, "  \"CPU\": %s\",\n", cpustring);
  }
  strcat(result, tmpbuf);

  if (sizeof(int *) == 8) {
    grntest_osinfo = OS_LINUX64;
    if (grntest_outtype == OUT_TSV) {
      sprintf(tmpbuf, "64BIT\n");
    } else {
      sprintf(tmpbuf, "  \"BIT\": 64,\n");
    }
  } else {
    grntest_osinfo = OS_LINUX32;
    if (grntest_outtype == OUT_TSV) {
      sprintf(tmpbuf, "32BIT\n");
    } else {
      sprintf(tmpbuf, "  \"BIT\": 32,\n");
    }
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "CORE\t%d\n", cpunum);
  } else {
    sprintf(tmpbuf, "  \"CORE\": %d,\n", cpunum);
  }
  strcat(result, tmpbuf);

  fp = fopen("/proc/meminfo", "r");
  if (!fp) {
    fprintf(stderr, "Cannot open meminfo\n");
    exit(1);
  }
  while (fgets(tmpbuf, 256, fp) != NULL) {
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    if (!strncmp(tmpbuf, "MemTotal:", 9)) {
      minfo = grntest_atoi(&tmpbuf[10], &tmpbuf[10] + 40, NULL);
    }
  }
  fclose(fp);
  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%dMBytes\n", minfo/1024);
  } else {
    sprintf(tmpbuf, "  \"RAM\": \"%dMBytes\",\n", minfo/1024);
  }
  strcat(result, tmpbuf);

  ret = statvfs(path, &vfsbuf);
  if (ret) {
    fprintf(stderr, "Cannot access %s\n", path);
    exit(1);
  }

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%ldKBytes\n", vfsbuf.f_blocks * 4);
  } else {
    sprintf(tmpbuf, "  \"HDD\": \"%ldKBytes\",\n", vfsbuf.f_blocks * 4);
  }
  strcat(result, tmpbuf);

  uname(&ubuf);
  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s %s\n", ubuf.sysname, ubuf.release);
  } else {
    sprintf(tmpbuf, "  \"OS\": \"%s %s\",\n", ubuf.sysname, ubuf.release);
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s\n", grntest_serverhost);
  } else {
    sprintf(tmpbuf, "  \"HOST\": \"%s\",\n", grntest_serverhost);
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%d\n", grntest_serverport);
  } else {
    sprintf(tmpbuf, "  \"PORT\": \"%d\",\n", grntest_serverport);
  }
  strcat(result, tmpbuf);

  if (grntest_outtype == OUT_TSV) {
    sprintf(tmpbuf, "%s\n", grn_get_version());
  } else {
    sprintf(tmpbuf, "  \"VERSION\": \"%s\"\n", grn_get_version());
  }
  strcat(result, tmpbuf);

  if (grntest_outtype != OUT_TSV) {
    strcat(result, "},");
  }
#endif /* WIN32 */
  if (strlen(result) >= olen) {
    fprintf(stderr, "buffer overrun in get_sysinfo!\n");
    exit(1);
  }
  return 0;
}

static
int
start_server(const char *dbpath, int r)
{
  int ret;
  char optbuf[BUF_LEN];
#ifdef WIN32
  char tmpbuf[BUF_LEN];

  STARTUPINFO si;

  if (strlen(dbpath) > BUF_LEN - 100) {
    fprintf(stderr, "too long dbpath!\n");
    exit(1);
  }

  strcpy(tmpbuf, GROONGA_PATH);
  strcat(tmpbuf, " -s ");
  strcat(tmpbuf, "-p ");
  sprintf(optbuf, "%d ", grntest_serverport);
  strcat(tmpbuf, optbuf);
  strcat(tmpbuf, dbpath);
  memset(&si, 0, sizeof(si));
  si.cb=sizeof(si);
  ret = CreateProcess(NULL, tmpbuf, NULL, NULL, FALSE,
		      0, NULL, NULL, &si, &grntest_pi);

  if (ret == 0) {
    fprintf(stderr, "Cannot start groonga server:error=%d\n", GetLastError());
    exit(1);
  }

#else
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "Cannot start groonga server:Cannot fork\n");
    exit(1);
  }
  sprintf(optbuf, "%d", grntest_serverport);
  if (pid == 0) {
    ret = execlp("groonga", "groonga", "-s", "-p", optbuf, dbpath, (char*)NULL);
    if (ret == -1) {
      fprintf(stderr, "Cannot start groonga server:errno=%d\n", errno);
      exit(1);
    }
  }
  else {
    grntest_server_id = pid;
  }

#endif /* WIN32 */

  return 0;
}

static
int
parse_line(char *buf, int start, int end, int num)
{
  int i, j, error_flag = 0, out_or_test = 0;
  char tmpbuf[BUF_LEN];

  grntest_job[num].concurrency = 1;
  grntest_job[num].ntimes = 1;
  grntest_job[num].done = 0;
  grntest_job[num].qnum = 0;
  grntest_job[num].max = 0LL;
  grntest_job[num].min = 9223372036854775807LL;
  grntest_job[num].outputlog = NULL;
  grntest_job[num].inputlog = NULL;

  strncpy(grntest_job[num].jobname, &buf[start], end - start);
  grntest_job[num].jobname[end - start] = '\0';
  i = start;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      i++;
      continue;
    }
    if (!strncmp(&buf[i], "do_local", 8)) {
      grntest_job[num].jobtype = J_DO_LOCAL;
      i = i + 8;
      break;
    }
    if (!strncmp(&buf[i], "do_gqtp", 7)) {
      grntest_job[num].jobtype = J_DO_GQTP;
      i = i + 7;
      break;
    }
    if (!strncmp(&buf[i], "do_http", 7)) {
      grntest_job[num].jobtype = J_DO_HTTP;
      i = i + 7;
      break;
    }
    if (!strncmp(&buf[i], "rep_local", 9)) {
      grntest_job[num].jobtype = J_REP_LOCAL;
      i = i + 9;
      break;
    }
    if (!strncmp(&buf[i], "rep_gqtp", 8)) {
      grntest_job[num].jobtype = J_REP_GQTP;
      i = i + 8;
      break;
    }
    if (!strncmp(&buf[i], "rep_http", 8)) {
      grntest_job[num].jobtype = J_REP_HTTP;
      i = i + 8;
      break;
    }
    if (!strncmp(&buf[i], "out_local", 9)) {
      grntest_job[num].jobtype = J_OUT_LOCAL;
      i = i + 9;
      out_or_test = 1;
      break;
    }
    if (!strncmp(&buf[i], "out_gqtp", 8)) {
      grntest_job[num].jobtype = J_OUT_GQTP;
      i = i + 8;
      out_or_test = 1;
      break;
    }
    if (!strncmp(&buf[i], "out_http", 8)) {
      grntest_job[num].jobtype = J_OUT_HTTP;
      i = i + 8;
      out_or_test = 1;
      break;
    }
    if (!strncmp(&buf[i], "test_local", 10)) {
      grntest_job[num].jobtype = J_TEST_LOCAL;
      i = i + 10;
      out_or_test = 1;
      break;
    }
    if (!strncmp(&buf[i], "test_gqtp", 9)) {
      grntest_job[num].jobtype = J_TEST_GQTP;
      i = i + 9;
      out_or_test = 1;
      break;
    }
    if (!strncmp(&buf[i], "test_http", 9)) {
      grntest_job[num].jobtype = J_TEST_HTTP;
      i = i + 9;
      out_or_test = 1;
      break;
    }
    error_flag = 1;
    i++;
  }

  if (error_flag) {
    return 3;
  }
  if (i == end) {
    return 1;
  }

  if (grn_isspace(&buf[i], GRN_ENC_UTF8) != 1) {
    return 4;
  }
  i++;

  while (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
    i++;
    continue;
  }
  j = 0;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      break;
    }
    grntest_job[num].commandfile[j] = buf[i];
    i++;
    j++;
    if (j > 255) {
      return 5;
    }
  }
  grntest_job[num].commandfile[j] = '\0';

  while (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
    i++;
  }

  if (i == end) {
    if (out_or_test) {
      fprintf(stderr, "log(test)_local(gqtp|http) needs log(test)_filename\n");
      return 11;
    }
    return 0;
  }

  j = 0;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      break;
    }
    tmpbuf[j] = buf[i];
    i++;
    j++;
    if (j >= BUF_LEN) {
      return 6;
    }
  }
  tmpbuf[j] ='\0';
  if (out_or_test) {
    if (out_p(grntest_job[num].jobtype)) {
      grntest_job[num].outputlog = fopen(tmpbuf, "wb");
      if (grntest_job[num].outputlog == NULL) {
        fprintf(stderr, "Cannot open %s\n", tmpbuf);
        return 13;
      }
    } else {
      char outlog[BUF_LEN];
      grntest_job[num].inputlog = fopen(tmpbuf, "rb");
      if (grntest_job[num].inputlog == NULL) {
        fprintf(stderr, "Cannot open %s\n", tmpbuf);
        return 14;
      }
      sprintf(outlog, "%s.diff", tmpbuf);
      grntest_job[num].outputlog = fopen(outlog, "wb");
      if (grntest_job[num].outputlog == NULL) {
        fprintf(stderr, "Cannot open %s\n", outlog);
        return 15;
      }
    }
    strcpy(grntest_job[num].logfile, tmpbuf);
    return 0;
  } else {
    grntest_job[num].concurrency = grntest_atoi(tmpbuf, tmpbuf + j, NULL);
    if (grntest_job[num].concurrency == 0) {
      return 7;
    }
  }

  while (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
    i++;
  }

  if (i == end) {
    return 0;
  }

  j = 0;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      break;
    }
    tmpbuf[j] = buf[i];
    i++;
    j++;
    if (j > 16) {
      return 8;
    }
  }
  tmpbuf[j] ='\0';
  grntest_job[num].ntimes = grntest_atoi(tmpbuf, tmpbuf + j, NULL);
  if (grntest_job[num].ntimes == 0) {
    return 9;
  }
  if (i == end) {
    return 0;
  }

  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      i++;
      continue;
    }
    return 10;
  }
  return 0;
}

static
int
get_jobs(grn_ctx *ctx, char *buf, int line)
{
  int i, len, start, end, ret;
  int jnum = 0;

  len = strlen(buf);
  i = 0;
  while (i < len) {
    if ((buf[i] == '#') || (buf[i] == '\r') || (buf[i] == '\n')) {
      buf[i] = '\0';
      len = i;
      break;
    }
    i++;
  }

  i = 0;
  start = 0;
  while (i < len) {
    if (buf[i] == ';') {
      end = i;
      ret = parse_line(buf, start, end, jnum);
      if (ret) {
        if (ret > 1) {
          fprintf(stderr, "Syntax error:line=%d:ret=%d:%s\n", line, ret, buf);
          error_exit(ctx, 1);
        }
      } else {
        jnum++;
      }
      start = end + 1;
    }
    i++;
  }
  end = len;
  ret = parse_line(buf, start, end, jnum);
  if (ret) {
    if (ret > 1) {
      fprintf(stderr, "Syntax error:line=%d:ret=%d:%s\n", line, ret, buf);
      error_exit(ctx, 1);
    }
  } else {
    jnum++;
  }
  return jnum;
}

static
int
make_task_table(grn_ctx *ctx, int jobnum)
{
  int i, j, len, line;
  int tid = 0;
  FILE *fp;
  struct commandtable *ctable = NULL;
  char tmpbuf[MAX_COMMAND_LEN];

  for (i = 0; i < jobnum; i++) {
    if ((grntest_job[i].concurrency == 1) && (!grntest_onmemory_mode)) {
      grntest_task[tid].file = grntest_job[i].commandfile;
      grntest_task[tid].table = NULL;
      grntest_task[tid].ntimes = grntest_job[i].ntimes;
      grntest_task[tid].jobtype = grntest_job[i].jobtype;
      grntest_task[tid].job_id = i;
      tid++;
      continue;
    }
    for (j = 0; j < grntest_job[i].concurrency; j++) {
      if (j == 0) {
        ctable = GRN_MALLOC(sizeof(struct commandtable));
        if (!ctable) {
          fprintf(stderr, "Cannot alloc commandtable\n");
          error_exit(ctx, 1);
        }
        fp = fopen(grntest_job[i].commandfile, "r");
        if (!fp) {
          fprintf(stderr, "Cannot alloc commandfile:%s\n",
                   grntest_job[i].commandfile);
          error_exit(ctx, 1);
        }
        line = 0;
        tmpbuf[MAX_COMMAND_LEN-2] = '\0';
        while (fgets(tmpbuf, MAX_COMMAND_LEN, fp) != NULL) {
          if (tmpbuf[MAX_COMMAND_LEN-2] != '\0') {
            tmpbuf[MAX_COMMAND_LEN-2] = '\0';
            fprintf(stderr, "Too long command in %s\n",
                   grntest_job[i].commandfile);
            fprintf(stderr, "line =%d:%s\n", line + 1, tmpbuf);
            error_exit(ctx, 1);
          }
          len = strlen(tmpbuf);
          len--;
          tmpbuf[len] = '\0';
          if (comment_p(tmpbuf)) {
            continue;
          }
          if (tmpbuf[0] == '\0') {
            continue;
          }
          ctable->command[line] = GRN_STRDUP(tmpbuf);
          if (ctable->command[line] == NULL) {
            fprintf(stderr, "Cannot alloc commandfile:%s\n",
                    grntest_job[i].commandfile);
            error_exit(ctx, 1);
          }
          ctable->num = line;
          line++;
          if (line >= MAX_COMMAND) {
            fprintf(stderr, "Too many commands in %s\n",
                   grntest_job[i].commandfile);
            error_exit(ctx, 1);
          }
        }
        ctable->num = line;
      }
      grntest_task[tid].file = NULL;
      grntest_task[tid].table = ctable;
      grntest_task[tid].ntimes = grntest_job[i].ntimes;
      grntest_task[tid].jobtype = grntest_job[i].jobtype;
      grntest_task[tid].job_id = i;
      tid++;
    }
  }
  return tid;
}

/*
static
int
print_commandlist(int task_id)
{
  int i;

  for (i = 0; i < grntest_task[task_id].table->num; i++) {
    printf("%s\n", grntest_task[task_id].table->command[i]);
  }
  return 0;
}
*/

/* return num of query */
static
int
do_jobs(grn_ctx *ctx, int jobnum, int line)
{
  int i, j, task_num, ret, qnum = 0,thread_num = 0;

  for (i = 0; i < jobnum; i++) {
/*
printf("%d:type =%d:file=%s:con=%d:ntimes=%d\n", i, grntest_job[i].jobtype,
        grntest_job[i].commandfile, JobTable[i].concurrency, JobTable[i].ntimes);

*/
    thread_num = thread_num + grntest_job[i].concurrency;
  }

  if (thread_num >= MAX_CON) {
    fprintf(stderr, "Too many threads requested(MAX=64):line=%d\n", line);
    error_exit(ctx, 1);
  }

  task_num = make_task_table(ctx, jobnum);
  if (task_num != thread_num) {
    fprintf(stderr, "Logical error\n");
    error_exit(ctx, 9);
  }

  grntest_detail_on = 0;
  for (i = 0; i < task_num; i++) {
    grn_ctx_init(&grntest_ctx[i], 0);
    grntest_owndb[i] = NULL;
    if (gqtp_p(grntest_task[i].jobtype)) {
      ret = grn_ctx_connect(&grntest_ctx[i], grntest_serverhost, grntest_serverport, 0);
      if (ret) {
        fprintf(stderr, "Cannot connect groonga server:host=%s:port=%d:ret=%d\n",
                grntest_serverhost, grntest_serverport, ret);
        error_exit(ctx, 1);
      }
    } else if (http_p(grntest_task[i].jobtype)) {
      grntest_task[i].http_socket = 0;
      GRN_TEXT_INIT(&grntest_task[i].http_response, 0);
      if (grntest_owndb_mode) {
        grntest_owndb[i] = grn_db_open(&grntest_ctx[i], grntest_dbpath);
        if (grntest_owndb[i] == NULL) {
          fprintf(stderr, "Cannot open db:%s\n", grntest_dbpath);
          exit(1);
        }
      } else {
        grntest_owndb[i] = grn_db_create(&grntest_ctx[i], NULL, NULL);
      }
    } else {
      if (grntest_owndb_mode) {
        grntest_owndb[i] = grn_db_open(&grntest_ctx[i], grntest_dbpath);
        if (grntest_owndb[i] == NULL) {
          fprintf(stderr, "Cannot open db:%s\n", grntest_dbpath);
          exit(1);
        }
      }
      else {
        grn_ctx_use(&grntest_ctx[i], grntest_db);
      }
    }
    if (report_p(grntest_task[i].jobtype)) {
      grntest_detail_on++;
    }
  }
  if (grntest_detail_on) {
    if (grntest_outtype == OUT_TSV) {
      ;
    }
    else {
      fprintf(grntest_logfp, "\"detail\": [\n");
    }
    fflush(grntest_logfp);
  }

  grntest_log_tmpbuf[0] = '\0';
  grntest_log_tmpbuf[LOGBUF_LEN-2] = '\0';
  thread_main(ctx, task_num);

  for (i = 0; i < task_num; i++) {
    if (grntest_owndb[i]) {
      grn_obj_close(&grntest_ctx[i], grntest_owndb[i]);
    }
    if (http_p(grntest_task[i].jobtype)) {
      GRN_OBJ_FIN(&grntest_ctx[i], &grntest_task[i].http_response);
    }
    grn_ctx_fin(&grntest_ctx[i]);
    qnum = qnum + grntest_task[i].qnum;
  }

  i = 0;
  while (i < task_num) {
    int job_id;
    if (grntest_task[i].table != NULL) {
      job_id = grntest_task[i].job_id;
      for (j = 0; j < grntest_task[i].table->num; j++) {
        GRN_FREE(grntest_task[i].table->command[j]);
      }
      GRN_FREE(grntest_task[i].table);
      while (job_id == grntest_task[i].job_id) {
        i++;
      }
    } else {
      i++;
    }
  }
  for (i = 0; i < jobnum; i++) {
    if (grntest_job[i].outputlog) {
      int ret;
      ret = fclose(grntest_job[i].outputlog);
      if (ret) {
        fprintf(stderr, "Cannot close %s\n", grntest_job[i].logfile);
        exit(1);
      }
    }
    if (grntest_job[i].inputlog) {
      int ret;
      ret = fclose(grntest_job[i].inputlog);
      if (ret) {
        fprintf(stderr, "Cannot close %s\n", grntest_job[i].logfile);
        exit(1);
      }
    }
  }
  return qnum;
}

/* return num of query */
static
int
do_script(grn_ctx *ctx, const char *sfile)
{
  int line = 0;
  int job_num;
  int qnum, qnum_total = 0;
  FILE *fp;
  char buf[BUF_LEN];

  fp = fopen(sfile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open script file:%s\n", sfile);
    error_exit(ctx, 1);
  }
  buf[BUF_LEN-2] = '\0';
  while (fgets(buf, BUF_LEN, fp) != NULL) {
    line++;
    if (buf[BUF_LEN-2] != '\0') {
      fprintf(stderr, "Too long line in script file:%d\n", line);
      error_exit(ctx, 1);
    }
    grntest_jobdone = 0;
    job_num = get_jobs(ctx, buf, line);
    grntest_jobnum = job_num;

    if (job_num > 0) {
      GRN_TIME_INIT(&grntest_jobs_start, 0);
      GRN_TIME_NOW(ctx, &grntest_jobs_start);
      if (grntest_outtype == OUT_TSV) {
        fprintf(grntest_logfp, "jobs-start\t%s\n", buf);
      } else {
        fprintf(grntest_logfp, "{\"jobs\": \"%s\",\n", buf);
      }
      qnum = do_jobs(ctx, job_num, line);
      if (grntest_outtype == OUT_TSV) {
        fprintf(grntest_logfp, "jobs-end\t%s\n", buf);
      } else {
        fprintf(grntest_logfp, "},\n");
      }
      qnum_total = qnum_total + qnum;

      grn_obj_close(ctx, &grntest_jobs_start);
    }
    if (grntest_stop_flag) {
      fprintf(stderr, "Error:Quit\n");
      break;
    }
  }
  fclose(fp);
  return qnum_total;
}

static
int
start_local(grn_ctx *ctx, const char *dbpath)
{
  grntest_db = grn_db_open(ctx, dbpath);
  if (!grntest_db) {
    grntest_db = grn_db_create(ctx, dbpath, NULL);
  }
  if (!grntest_db) {
    fprintf(stderr, "Cannot open db:%s\n", dbpath);
    exit(1);
  }
  return 0;
}

static
int
check_server(grn_ctx *ctx)
{
  int ret, retry = 0;
  while (1) {
    ret = grn_ctx_connect(ctx, grntest_serverhost, grntest_serverport, 0);
    if (ret == GRN_CONNECTION_REFUSED) {
      sleep(1);
      retry++;
      if (retry > 5) {
        fprintf(stderr, "Cannot connect groonga server:host=%s:port=%d:ret=%d\n",
                grntest_serverhost, grntest_serverport, ret);
        return 1;
      }
      continue;
    }
    if (ret) {
      fprintf(stderr, "Cannot connect groonga server:host=%s:port=%d:ret=%d\n",
              grntest_serverhost, grntest_serverport, ret);
      return 1;
    }
    break;
  }
  return 0;
}

#define MODE_LIST 1
#define MODE_GET  2
#define MODE_PUT  3
#define MODE_TIME 4

static
int
check_response(char *buf)
{
  if (buf[0] == '1') {
    return 1;
  }
  if (buf[0] == '2') {
    return 1;
  }
  if (buf[0] == '3') {
    return 1;
  }
  return 0;
}

static
int
read_response(socket_t socket, char *buf)
{
  int ret;
  ret = recv(socket, buf, BUF_LEN - 1, 0);
  if (ret == -1) {
    fprintf(stderr, "recv error:3\n");
    exit(1);
  }
  buf[ret] ='\0';
#ifdef DEBUG_FTP
  fprintf(stderr, "recv:%s", buf);
#endif
  return ret;
}

static
int
put_file(socket_t socket, const char *filename)
{
  FILE *fp;
  int c, ret, size = 0;
  char buf[1];

  fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "LOCAL:no such file:%s\n", filename);
    return 0;
  }

  while ((c = fgetc(fp)) != EOF) {
    buf[0] = c;
    ret = send(socket, buf, 1, 0);
    if (ret == -1) {
      fprintf(stderr, "send error\n");
      exit(1);
    }
    size++;
  }
  fclose(fp);
  return size;
}

static
int
ftp_list(socket_t data_socket)
{
  int ret;
  char buf[BUF_LEN];

  while (1) {
    ret = recv(data_socket, buf, BUF_LEN - 2, 0);
    if (ret == 0) {
      fflush(stdout);
      return 0;
    }
    buf[ret] = '\0';
    fprintf(stdout, "%s", buf);
  }

  return 0;
}

static
int
get_file(socket_t socket, const char *filename, int size)
{
  FILE *fp;
  int ret, total;
  char buf[FTPBUF];

  fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "Cannot open %s\n",  filename);
    return -1;
  }

  total = 0;
  while (total != size) {
    ret = recv(socket, buf, FTPBUF, 0);
    if (ret == -1) {
      fprintf(stderr, "recv error:2:ret=%d:size=%d:total\n", ret, size);
      return -1;
    }
    if (ret == 0) {
      break;
    }
    fwrite(buf, ret, 1, fp);
    total = total + ret;
  }

  fclose(fp);
  return size;
}

static
int
get_port(char *buf, char *host, int *port)
{
  int ret,d1,d2,d3,d4,d5,d6;
  ret = sscanf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
         &d1, &d2, &d3, &d4, &d5, &d6);
  if (ret != 6) {
    fprintf(stderr, "Cannot enter passsive mode\n");
    return 0;
  }

  *port = d5 * 256 + d6;
  sprintf(host, "%d.%d.%d.%d", d1, d2, d3, d4);
  return 1;
}

static
char *
get_ftp_date(char *buf)
{
  while (*buf !=' ') {
    buf++;
    if (*buf == '\0') {
      return NULL;
    }
  }
  buf++;

  return buf;
}

static
int
get_size(char *buf)
{
  int size;

  while (*buf !='(') {
    buf++;
    if (*buf == '\0') {
      return 0;
    }
  }
  buf++;
  size = grntest_atoi(buf, buf + strlen(buf), NULL);

  return size;
}

int
ftp_sub(char *user, char *passwd, char *host, const char *filename,
         int mode, char *cd_dirname, char *retval)
{
  int size = 0;
  int status = 0;
  socket_t command_socket, data_socket;
  int data_port;
  char data_host[BUF_LEN];
  char send_mesg[BUF_LEN];
  char buf[BUF_LEN];
#ifdef WIN32
  char base[BUF_LEN];
  char fname[BUF_LEN];
  char ext[BUF_LEN];
#else
  char *base;
#endif /* WIN32 */

#ifdef WIN32
  WSADATA ws;

  WSAStartup(MAKEWORD(2,0), &ws);
#endif /* WIN32 */

  if ((filename != NULL) && (strlen(filename) >= MAX_PATH_LEN)) {
    fprintf(stderr, "too long filename\n");
    exit(1);
  }

  if ((cd_dirname != NULL) && (strlen(cd_dirname) >= MAX_PATH_LEN)) {
    fprintf(stderr, "too long dirname\n");
    exit(1);
  }

  command_socket = open_socket(host, 21);
  if (command_socket == SOCKETERROR) {
    return 0;
  }

  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send username */
  sprintf(send_mesg, "USER %s\r\n", user);
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send passwd */
  sprintf(send_mesg, "PASS %s\r\n", passwd);
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send TYPE I */
  sprintf(send_mesg, "TYPE I\r\n");
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send PASV */
  sprintf(send_mesg, "PASV\r\n");
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  if (!get_port(buf, data_host, &data_port)) {
    goto exit;
  }

  data_socket = open_socket(data_host, data_port);
  if (data_socket == SOCKETERROR) {
    goto exit;
  }

  if (cd_dirname) {
    sprintf(send_mesg, "CWD %s\r\n", cd_dirname);
    write_to_server(command_socket, send_mesg);
  }

  read_response(command_socket, buf);
  if (!check_response(buf)) {
    socketclose(data_socket);
    goto exit;
  }

#ifdef WIN32
  _splitpath(filename, NULL, NULL, fname, ext);
  strcpy(base, fname);
  strcat(base, ext);
#else
  strcpy(buf, filename);
  base = basename(buf);
#endif /* WIN32 */

  switch (mode) {
  case MODE_LIST:
    if (filename) {
      sprintf(send_mesg, "LIST %s\r\n", filename);
    } else {
      sprintf(send_mesg, "LIST \r\n");
    }
    write_to_server(command_socket, send_mesg);
    break;
  case MODE_PUT:
    sprintf(send_mesg, "STOR %s\r\n", base);
    write_to_server(command_socket, send_mesg);
    break;
  case MODE_GET:
    sprintf(send_mesg, "RETR %s\r\n", base);
    write_to_server(command_socket, send_mesg);
    break;
  case MODE_TIME:
    sprintf(send_mesg, "MDTM %s\r\n", base);
    write_to_server(command_socket, send_mesg);
    break;
  default:
    fprintf(stderr, "invalid mode\n");
    socketclose(data_socket);
    goto exit;
  }

  read_response(command_socket, buf);
  if (!check_response(buf)) {
    socketclose(data_socket);
    goto exit;
  }
  if (!strncmp(buf, "150", 3)) {
    size = get_size(buf);
  }
  if (!strncmp(buf, "213", 3)) {
    retval[BUF_LEN-2] = '\0';
    strcpy(retval, get_ftp_date(buf));
    if (retval[BUF_LEN-2] != '\0' ) {
      fprintf(stderr, "buffer over run in ftp\n");
      exit(1);
    }
  }

  switch (mode) {
  case MODE_LIST:
    ftp_list(data_socket);
    break;
  case MODE_GET:
    if (get_file(data_socket, filename, size) == -1) {
      socketclose(data_socket);
      goto exit;
    }
    fprintf(stderr, "get:%s\n", filename);
    break;
  case MODE_PUT:
    if (put_file(data_socket, filename) == -1) {
      socketclose(data_socket);
      goto exit;
    }
    fprintf(stderr, "put:%s\n", filename);
    break;
  default:
    break;
  }

  socketclose(data_socket);
  if ((mode == MODE_GET) || (mode == MODE_PUT)) {
    read_response(command_socket, buf);
  }
  write_to_server(command_socket, "QUIT\n");
  status = 1;
exit:
  socketclose(command_socket);

#ifdef WIN32
  WSACleanup();
#endif
  return status;
}

/*
static
int
ftp_main(int argc, char **argv)
{
  char val[BUF_LEN];
  val[0] = '\0';
  ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, argv[2],
          grntest_atoi(argv[3], argv[3] + strlen(argv[3]), NULL), argv[4], val);
  if (val[0] != '\0') {
    printf("val=%s\n", val);
  }
  return 0;
}
*/

static
int
get_username(char *name, int maxlen)
{
  char *env=NULL;
  strcpy(name, "nobody");
#ifdef WIN32
  env = getenv("USERNAME");
#else
  env = getenv("USER");
#endif /* WIN32 */
  if (strlen(env) > maxlen) {
    fprintf(stderr, "too long username:%s\n", env);
    exit(1);
  }
  if (env) {
    strcpy(name, env);
  }
  return 0;
}

static
int
get_date(char *date, time_t *sec)
{
#ifdef __GNUC__
#  ifdef HAVE_LOCALTIME_R
  struct tm result;
  struct tm *tm = &result;
  localtime_r(sec, tm);
#  else /* HAVE_LOCALTIME_R */
  struct tm *tm = localtime(sec);
#  endif /* HAVE_LOCALTIME_R */
#else /* __GNUC__ */
  struct tm tmbuf;
  struct tm *tm = &tmbuf;
  localtime_s(tm, sec);
#endif /* __GNUC__ */

#ifdef WIN32
  strftime(date, 128, "%Y-%m-%d %H:%M:%S", tm);
#else
  strftime(date, 128, "%F %T", tm);
#endif /* WIN32 */

  return 1;
}

static
int
get_scriptname(const char *path, char *name, char *suffix)
{
  int slen = strlen(suffix);
  int len = strlen(path);

  if (len >= BUF_LEN) {
    fprintf(stderr, "too long script name\n");
    exit(1);
  }
  if (slen > len) {
    fprintf(stderr, "too long suffux\n");
    exit(1);
  }

  strcpy(name, path);
  if (strncmp(&name[len-slen], suffix, slen)) {
    name[0] = '\0';
    return 0;
  }
  name[len-slen] = '\0';
  return 1;
}

#ifdef WIN32
static
int
get_tm_from_serverdate(char *serverdate, struct tm *tm)
{
  int res;
  int year, month, day, hour, minute, second;

  res = sscanf(serverdate, "%4d%2d%2d%2d%2d%2d",
               &year, &month, &day, &hour, &minute, &second);

/*
  printf("%d %d %d %d %d %d\n", year, month, day, hour, minute, second);
*/

  tm->tm_sec = second;
  tm->tm_min = minute;
  tm->tm_hour = hour;
  tm->tm_mday = day;
  tm->tm_mon = month - 1;
  tm->tm_year = year - 1900;
  tm->tm_isdst = -1;

  return 0;
}
#endif /* WIN32 */



static
int
sync_sub(grn_ctx *ctx, const char *filename)
{
  int ret;
  char serverdate[BUF_LEN];
#ifdef WIN32
  struct _stat statbuf;
#else
  struct stat statbuf;
#endif /* WIN32 */
  time_t st, lt;
  struct tm stm;

  ret = ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, filename, MODE_TIME, "data",
               serverdate);
  if (ret == 0) {
    fprintf(stderr, "[%s] does not exist in server\n", filename);
    return 0;
  }
#ifdef WIN32
  get_tm_from_serverdate(serverdate, &stm);
#else
  strptime(serverdate, "%Y%m%d %H%M%S", &stm);
#endif /* WIN32 */

  /* fixme! needs timezone info */
  st = mktime(&stm) + 3600 * 9;
  lt = st;

#ifdef WIN32
  ret = _stat(filename, &statbuf);
#else
  ret = stat(filename, &statbuf);
#endif /* WIN32 */

  if (!ret) {
    lt = statbuf.st_mtime;
    if (lt < st) {
      fprintf(stderr, "newer [%s] exists in server\n", filename);
      fflush(stderr);
      ret = ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, filename, MODE_GET, "data",
                    NULL);
      return ret;
    }
  } else {
    fprintf(stderr, "[%s] does not exist in local\n", filename);
    fflush(stderr);
    ret = ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, filename, MODE_GET, "data",
                  NULL);
    return ret;
  }
  return 0;
}

static
int
cache_file(grn_ctx *ctx, char **flist, char *file, int fnum)
{
  int i;

  for (i = 0; i < fnum; i++) {
    if (!strcmp(flist[i], file) ) {
      return fnum;
    }
  }
  flist[fnum] = GRN_STRDUP(file);
  fnum++;
  if (fnum >= BUF_LEN) {
    fprintf(stderr, "too many uniq commands file!\n");
    exit(1);
  }
  return fnum;
}

static
int
sync_datafile(grn_ctx *ctx, const char *sfile)
{
  int line = 0;
  int fnum = 0;
  int i, job_num;
  FILE *fp;
  char buf[BUF_LEN];
  char *filelist[BUF_LEN];

  fp = fopen(sfile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open script file:%s\n", sfile);
    error_exit(ctx, 1);
  }
  buf[BUF_LEN-2] = '\0';
  while (fgets(buf, BUF_LEN, fp) != NULL) {
    line++;
    if (buf[BUF_LEN-2] != '\0') {
      fprintf(stderr, "Too long line in script file:%d\n", line);
      error_exit(ctx, 1);
    }
    job_num = get_jobs(ctx, buf, line);

    if (job_num > 0) {
      for (i = 0; i < job_num; i++) {
/*
printf("commandfile=[%s]:buf=%s\n", grntest_job[i].commandfile, buf);
*/
        fnum = cache_file(ctx, filelist, grntest_job[i].commandfile, fnum);
      }
    }
  }
  for (i = 0; i < fnum; i++) {
    if (sync_sub(ctx, filelist[i])) {
      fprintf(stderr, "updated!:%s\n", filelist[i]);
      fflush(stderr);
    }
    GRN_FREE(filelist[i]);
  }
  fclose(fp);
  return fnum;
}

static
int
sync_script(grn_ctx *ctx, const char *filename)
{
  int ret, filenum;

  ret = sync_sub(ctx, filename);
  if (!ret) {
    return 0;
  }

  fprintf(stderr, "updated!:%s\n", filename);
  fflush(stderr);
  filenum = sync_datafile(ctx, filename);
  return 1;
}

static
void
usage(void)
{
  fprintf(stderr,
         "Usage: grntest [options...] [script] [db]\n"
         "options:\n"
         "  --dir:                     show script files on ftp server\n"
         "  -i, --host <ip/hostname>:  server address to listen (default: %s)\n"
         "  --localonly:               omit server connection\n"
         "  --log-output-dir:          specify output dir (default: current)\n"
         "  --noftp:                   omit ftp connection\n"
         "  --onmemory:                load all commands into memory\n"
         "  --output-type <tsv/json>:  specify output-type (default: json)\n"
         "  --owndb:                   open dbs for each ctx\n"
         "  -p, --port <port number>:  server port number (default: %d)\n",
         DEFAULT_DEST, DEFAULT_PORT);
  exit(1);
}

enum {
  mode_default = 0,
  mode_list,
  mode_usage,
};

#define MODE_MASK      0x007f
#define MODE_NOFTP     0x0080
#define MODE_LOCALONLY 0x0100
#define MODE_OWNDB     0x0800
#define MODE_ONMEMORY  0x1000


static
int
get_token(char *line, char *token, int maxlen, char **next)
{
  int i = 0;

  *next = NULL;
  token[i] = '\0';

  while (*line) {
    if (grn_isspace(line, GRN_ENC_UTF8) == 1) {
      line++;
      continue;
    }
    if (*line == ';') {
      token[0] = ';';
      token[1] = '\0';
      *next = line + 1;
      return 1;
    }
    if (*line == '#') {
      token[0] = ';';
      token[1] = '\0';
      *next = line + 1;
      return 1;
    }
    break;
  }

  while (*line) {
    token[i] = *line;
    i++;
    if (grn_isspace(line + 1, GRN_ENC_UTF8) == 1) {
      token[i] = '\0';
      *next = line + 1;
      return 1;
    }
    if (*(line + 1) == ';') {
      token[i] = '\0';
      *next = line + 1;
      return 1;
    }
    if (*(line + 1) == '#') {
      token[i] = '\0';
      *next = line + 1;
      return 1;
    }
    if (*(line + 1) == '\0') {
      token[i] = '\0';
      return 1;
    }

    line++;
  }
  return 0;
}

/* SET_PORT and SET_HOST */
static
int
check_script(const char *scrname)
{
  FILE *fp;
  char tmpbuf[BUF_LEN];
  char token[BUF_LEN];
  char prev[BUF_LEN];
  char *next = NULL;

  fp = fopen(scrname, "r");
  if (!fp) {
    fprintf(stderr, "check_script:Cannot open script:%s\n", scrname);
    exit(1);
  }

  tmpbuf[BUF_LEN-2] = '\0';
  while (fgets(tmpbuf, BUF_LEN, fp) != NULL) {
    if (tmpbuf[BUF_LEN-2] != '\0') {
      fprintf(stderr, "Too long line in script:%s\n", scrname);
      exit(1);
    }
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    get_token(tmpbuf, token, BUF_LEN, &next);
    strcpy(prev, token);

    while (next) {
      get_token(next, token, BUF_LEN, &next);
      if (!strncmp(prev, "SET_PORT", 8)) {
        grntest_serverport = grn_atoi(token, token + strlen(token), NULL);
      }
      if (!strncmp(prev, "SET_HOST", 8)) {
        strcpy(grntest_serverhost, token);
        grntest_remote_mode = 1;
      }
      strcpy(prev, token);
    }
  }

  fclose(fp);
  return 0;
}

#ifndef WIN32
static
void
timeout(int sig)
{
  fprintf(stderr, "timeout:groonga server cannot shutdown!!\n");
  fprintf(stderr, "Use \"kill -9 %d\"\n",  grntest_server_id);
  alarm(0);
}

static
int
setsigalarm(int sec)
{
  int	ret;
  struct sigaction sig;

  alarm(sec);
  sig.sa_handler = timeout;
  sig.sa_flags = 0;
  sigemptyset(&sig.sa_mask);
  ret = sigaction(SIGALRM, &sig, NULL);
  if (ret == -1) {
    fprintf(stderr, "setsigalarm:errno= %d\n", errno);
  }
  return ret;
}
#endif /* WIN32 */

int
main(int argc, char **argv)
{
  int qnum, i, mode = 0;
  grn_ctx context;
  char sysinfo[BUF_LEN];
  char log[BUF_LEN];
  const char *portstr=NULL, *hoststr=NULL, *dbname=NULL, *scrname=NULL, *outdir=NULL, *outtype=NULL;
  time_t sec;

  static grn_str_getopt_opt opts[] = {
    {'i', "host", NULL, 0, getopt_op_none},
    {'p', "port", NULL, 0, getopt_op_none},
    {'\0', "log-output-dir", NULL, 0, getopt_op_none},
    {'\0', "output-type", NULL, 0, getopt_op_none},
    {'\0', "dir", NULL, mode_list, getopt_op_update},
    {'\0', "noftp", NULL, MODE_NOFTP, getopt_op_on},
    {'h', "help", NULL, mode_usage, getopt_op_update},
    {'\0', "localonly", NULL, MODE_LOCALONLY, getopt_op_on},
    {'\0', "onmemory", NULL, MODE_ONMEMORY, getopt_op_on},
    {'\0', "owndb", NULL, MODE_OWNDB, getopt_op_on},
    {'\0', NULL, NULL, 0, 0}
  };

  opts[0].arg = &hoststr;
  opts[1].arg = &portstr;
  opts[2].arg = &outdir;
  opts[3].arg = &outtype;

  i = grn_str_getopt(argc, argv, opts, &mode);
  if (i < 0) {
    usage();
  }

  switch (mode & MODE_MASK) {
  case mode_list :
    ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, "*.scr", 1, "data",
             NULL);
    return 0;
    break;
  case mode_usage :
    usage();
    break;
  default :
    break;
  }
  
  if (i < argc) {
    scrname = argv[i];
  }
  if (i < argc - 1) {
    dbname = argv[i+1];
  }
  grntest_dbpath = dbname;

  if (mode & MODE_LOCALONLY) {
    grntest_localonly_mode = 1;
    grntest_remote_mode = 1;
  }

  if (mode & MODE_OWNDB) {
    grntest_localonly_mode = 1;
    grntest_remote_mode = 1;
    grntest_owndb_mode = 1;
  }

  if (mode & MODE_ONMEMORY) {
    grntest_onmemory_mode= 1;
  }

  if (mode & MODE_NOFTP) {
    grntest_noftp_mode= 1;
  }

  if ((scrname == NULL) || (dbname == NULL)) {
    usage();
  }

  strcpy(grntest_serverhost, DEFAULT_DEST);
  if (hoststr) {
    grntest_remote_mode = 1;
    strcpy(grntest_serverhost, hoststr);
  }
  grntest_serverport = DEFAULT_PORT;
  if (portstr) {
    grntest_serverport = grn_atoi(portstr, portstr + strlen(portstr), NULL);
  }

  if (outtype && !strcmp(outtype, "tsv")) {
    grntest_outtype = OUT_TSV;
  }

  grn_init();
  CRITICAL_SECTION_INIT(grntest_cs);

  grn_ctx_init(&context, 0);
  grn_ctx_init(&grntest_server_context, 0);
  grn_set_default_encoding(GRN_ENC_UTF8);

  if (!grntest_noftp_mode) {
    sync_script(&context, scrname);
  }
  check_script(scrname);

  start_local(&context, dbname);
  if (!grntest_remote_mode) {
    start_server(dbname, 0);
  }

  if (!grntest_localonly_mode) {
    if (check_server(&grntest_server_context)) {
      goto exit;
    }
  }

  get_scriptname(scrname, grntest_scriptname, ".scr");
  get_username(grntest_username, 256);

  GRN_TIME_INIT(&grntest_starttime, 0);
  GRN_TIME_NOW(&context, &grntest_starttime);
  sec = (time_t)(GRN_TIME_VALUE(&grntest_starttime)/1000000);
  get_date(grntest_date, &sec);

  if (outdir) {
    sprintf(log, "%s/%s-%s-%lld-%s.log", outdir, grntest_scriptname,
            grntest_username, GRN_TIME_VALUE(&grntest_starttime), grn_get_version());
  } else {
    sprintf(log, "%s-%s-%lld-%s.log", grntest_scriptname,
            grntest_username, GRN_TIME_VALUE(&grntest_starttime), grn_get_version());
  }

  grntest_logfp = fopen(log, "w+b");
  if (!grntest_logfp) {
    fprintf(stderr, "Cannot open logfile:%s\n", log);
    goto exit;
  }

  get_sysinfo(dbname, sysinfo, BUF_LEN);
  output_sysinfo(sysinfo);

  qnum = do_script(&context, scrname);
  output_result_final(&context, qnum);
  fclose(grntest_logfp);

  if (!grntest_noftp_mode) {
    ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, log, 3,
            "report", NULL);
  }
  fprintf(stderr, "grntest done. logfile=%s\n", log);

exit:
  shutdown_server();
#ifdef WIN32
  if (!grntest_remote_mode) {
    int ret;
    ret = WaitForSingleObject(grntest_pi.hProcess, 20000);
    if (ret == WAIT_TIMEOUT) {
      fprintf(stderr, "timeout:groonga server cannot shutdown!!\n");
      fprintf(stderr, "Cannot wait\n");
      exit(1);
    }
  }
#else
  if (grntest_server_id) {
    int ret, pstatus;
    setsigalarm(20);
    ret = waitpid(grntest_server_id, &pstatus, 0);
    if (ret < 0) {
      fprintf(stderr, "Cannot wait\n");
      exit(1);
    }
/*
    else {
      fprintf(stderr, "pstatus = %d\n", pstatus);
    }
*/
    alarm(0);
  }
#endif /* WIN32 */
  CRITICAL_SECTION_FIN(grntest_cs);
  grn_obj_close(&context, &grntest_starttime);
  grn_obj_close(&context, grntest_db);
  grn_ctx_fin(&context);
  grn_ctx_fin(&grntest_server_context);
  grn_fin();
  return 0;
}
