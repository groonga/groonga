/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2023  Sutou Kouhei <kou@clear-code.com>

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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef WIN32
#  define GROONGA_MAIN
#endif /* WIN32 */
#include <grn.h>

#include <grn_com.h>
#include <grn_ctx_impl.h>
#include <grn_proc.h>
#include <grn_db.h>
#include <grn_util.h>
#include <grn_error.h>

#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifndef WIN32
#  include <netinet/in.h>
#endif /* WIN32 */

#ifdef HAVE_SYS_RESOURCE_H
#  include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef WIN32
#  include <io.h>
#  include <direct.h>
#else /* WIN32 */
#  include <sys/uio.h>
#endif /* WIN32 */

#if !defined(WIN32) && defined(HAVE_SIGNAL_H)
#  define ENABLE_LOG_REOPEN_BY_SIGNAL
#endif

#ifdef ENABLE_LOG_REOPEN_BY_SIGNAL
#  include <signal.h>
#endif

#ifndef USE_MSG_NOSIGNAL
#  ifdef MSG_NOSIGNAL
#    undef MSG_NOSIGNAL
#  endif
#  define MSG_NOSIGNAL 0
#endif /* USE_MSG_NOSIGNAL */

#ifndef STDIN_FILENO
#  define STDIN_FILENO 0
#endif /* STDIN_FILENO */
#ifndef STDOUT_FILENO
#  define STDOUT_FILENO 1
#endif /* STDOUT_FILENO */
#ifndef STDERR_FILENO
#  define STDERR_FILENO 2
#endif /* STDERR_FILENO */

#define DEFAULT_HTTP_PORT              10041
#define DEFAULT_GQTP_PORT              10043
#define DEFAULT_DEST                   "localhost"
#define DEFAULT_MAX_N_FLOATING_THREADS 8
#define MAX_CON                        0x10000

#define RLIMIT_NOFILE_MINIMUM          4096

static char bind_address[HOST_NAME_MAX + 1];
static char hostname[HOST_NAME_MAX + 1];
static int port = DEFAULT_GQTP_PORT;
static int batchmode;
static int number_of_lines = 0;
static int newdb;
static grn_bool is_daemon_mode = GRN_FALSE;
static int listen_backlog = GRN_COM_EVENT_LISTEN_BACKLOG_DEFAULT;
static int (*do_client)(int argc, char **argv);
static int (*do_server)(char *path);
static const char *pid_file_path = NULL;
static const char *input_path = NULL;
static grn_file_reader *input_reader = NULL;
static FILE *output = NULL;
static grn_bool is_memcached_mode = GRN_FALSE;
static const char *memcached_column_name = NULL;

static int ready_notify_pipe[2];
#define PIPE_READ  0
#define PIPE_WRITE 1

static grn_encoding encoding;
static const char *windows_event_source_name = "Groonga";
static grn_bool use_windows_event_log = GRN_FALSE;
static grn_obj http_response_server_line;
static grn_wal_role wal_role = GRN_WAL_ROLE_NONE;
static grn_wal_role worker_wal_role = GRN_WAL_ROLE_NONE;

static grn_bool running_event_loop = GRN_FALSE;

#ifdef ENABLE_LOG_REOPEN_BY_SIGNAL
static bool usr1_received = false;

static void
usr1_handler(int signal_number, siginfo_t *info, void *context)
{
  usr1_received = true;
}

static void
set_usr1_handler(void)
{
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = usr1_handler;
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigaction(SIGUSR1, &action, NULL);
}

static void
reopen_log_by_signal(grn_ctx *ctx)
{
  if (!usr1_received) {
    return;
  }

  usr1_received = false;
  grn_log_reopen(ctx);
}
#endif

static int
grn_rc_to_exit_code(grn_rc rc)
{
  if (rc == GRN_SUCCESS) {
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}

static void
break_accept_event_loop(grn_ctx *ctx)
{
  grn_com *client;
  const char *address;

  if (strcmp(bind_address, "0.0.0.0") == 0) {
    address = "127.0.0.1";
  } else if (strcmp(bind_address, "::") == 0) {
    address = "::1";
  } else {
    address = bind_address;
  }
  client = grn_com_copen(ctx, NULL, address, port);
  if (client) {
    grn_com_close(ctx, client);
  }
}

#ifdef GRN_WITH_LIBEDIT
#  include <locale.h>
#  include <histedit.h>
static EditLine *line_editor = NULL;
static HistoryW *line_editor_history = NULL;
static HistEventW line_editor_history_event;
static char line_editor_history_path[PATH_MAX] = "";

static const wchar_t *
line_editor_prompt(EditLine *e __attribute__((unused)))
{
  return L"> ";
}
static const wchar_t *const line_editor_editor = L"emacs";

static void
line_editor_init(int argc __attribute__((unused)), char *argv[])
{
  const char *const HOME_PATH = getenv("HOME");
  const char *const HISTORY_PATH = "/.groonga-history";

  setlocale(LC_ALL, "");

  if (strlen(HOME_PATH) + strlen(HISTORY_PATH) < PATH_MAX) {
    grn_strcpy(line_editor_history_path, PATH_MAX, HOME_PATH);
    grn_strcat(line_editor_history_path, PATH_MAX, HISTORY_PATH);
  } else {
    line_editor_history_path[0] = '\0';
  }

  line_editor_history = history_winit();
  history_w(line_editor_history, &line_editor_history_event, H_SETSIZE, 200);
  if (line_editor_history_path[0]) {
    history_w(line_editor_history,
              &line_editor_history_event,
              H_LOAD,
              line_editor_history_path);
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
        history_w(line_editor_history,
                  &line_editor_history_event,
                  H_SAVE,
                  line_editor_history_path);
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
    for (i = 0; i < nchar; i++) {
      multibyte_len = wcrtomb(multibyte_buf, line[i], &ps);
      if (multibyte_len == (size_t)-1) {
        GRN_LOG(ctx,
                GRN_LOG_WARNING,
                "[prompt][libedit] failed to read input: %s",
                strerror(errno));
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
#endif /* GRN_WITH_LIBEDIT */

grn_inline static grn_rc
read_next_line(grn_ctx *ctx, grn_obj *buf)
{
  static int the_first_read = GRN_TRUE;
  grn_rc rc = GRN_SUCCESS;
  if (!batchmode) {
#ifdef GRN_WITH_LIBEDIT
    rc = line_editor_fgets(ctx, buf);
#else
    fprintf(stderr, "> ");
    fflush(stderr);
    rc = grn_file_reader_read_line(ctx, input_reader, buf);
#endif
  } else {
    rc = grn_file_reader_read_line(ctx, input_reader, buf);
    if (rc != GRN_END_OF_DATA) {
      number_of_lines++;
    }
  }
  if (the_first_read && GRN_TEXT_LEN(buf) > 0) {
    const char bom[] = {0xef, 0xbb, 0xbf};
    if (GRN_CTX_GET_ENCODING(ctx) == GRN_ENC_UTF8 && GRN_TEXT_LEN(buf) > 3 &&
        !memcmp(GRN_TEXT_VALUE(buf), bom, 3)) {
      grn_obj buf_without_bom;
      GRN_TEXT_INIT(&buf_without_bom, 0);
      GRN_TEXT_PUT(ctx,
                   &buf_without_bom,
                   GRN_TEXT_VALUE(buf) + 3,
                   GRN_TEXT_LEN(buf) - 3);
      GRN_TEXT_SET(ctx,
                   buf,
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

grn_inline static grn_rc
prompt(grn_ctx *ctx, grn_obj *buf)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_BULK_REWIND(buf);
  while (rc == GRN_SUCCESS) {
    rc = read_next_line(ctx, buf);
    if (rc == GRN_SUCCESS && GRN_TEXT_LEN(buf) > 0 &&
        GRN_TEXT_VALUE(buf)[GRN_TEXT_LEN(buf) - 1] == '\\') {
      rc = grn_bulk_truncate(ctx, buf, GRN_TEXT_LEN(buf) - 1);
    } else {
      break;
    }
  }
  return rc;
}

static void
output_envelope(
  grn_ctx *ctx, grn_rc rc, grn_obj *head, grn_obj *body, grn_obj *foot)
{
  grn_output_envelope(ctx, rc, head, body, foot, input_path, number_of_lines);
}

typedef struct {
  FILE *output;
  grn_bool is_outputting;
} standalone_output_context;

static void
s_output_raw(grn_ctx *ctx, int flags, standalone_output_context *output_context)
{
  FILE *stream = output_context->output;
  char *chunk = NULL;
  unsigned int chunk_size = 0;
  int recv_flags;
  size_t written = 0;

  grn_ctx_recv(ctx, &chunk, &chunk_size, &recv_flags);
  if (chunk_size > 0) {
    fwrite(chunk, 1, chunk_size, stream);
    written += chunk_size;
  }

  grn_bool is_last_message = (flags & GRN_CTX_TAIL);
  if (is_last_message) {
    grn_obj *command;

    if (grn_ctx_get_output_type(ctx) == GRN_CONTENT_GROONGA_COMMAND_LIST &&
        chunk_size > 0 && chunk[chunk_size - 1] != '\n') {
      fwrite("\n", 1, 1, stream);
      written += 1;
    }
    fflush(stream);

    command = GRN_CTX_USER_DATA(ctx)->ptr;
    GRN_BULK_REWIND(command);
  }

  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_SIZE,
                ":",
                "send(%" GRN_FMT_SIZE ")",
                written);
}

static void
s_output_typed(grn_ctx *ctx,
               int flags,
               standalone_output_context *output_context)
{
  if (!ctx) {
    return;
  }

  if (!ctx->impl) {
    return;
  }

  FILE *stream = output_context->output;
  char *chunk = NULL;
  unsigned int chunk_size = 0;
  int recv_flags;
  grn_obj body;
  grn_obj *command;
  bool is_last_message = (flags & GRN_CTX_TAIL);

  GRN_TEXT_INIT(&body, GRN_OBJ_DO_SHALLOW_COPY);
  grn_ctx_recv(ctx, &chunk, &chunk_size, &recv_flags);
  GRN_TEXT_SET(ctx, &body, chunk, chunk_size);

  if (GRN_TEXT_LEN(&body) == 0 && ctx->rc == GRN_SUCCESS) {
    GRN_OBJ_FIN(ctx, &body);
    return;
  }

  grn_obj head, foot;
  GRN_TEXT_INIT(&head, 0);
  GRN_TEXT_INIT(&foot, 0);
  if (!output_context->is_outputting) {
    if (is_last_message) {
      output_envelope(ctx, ctx->rc, &head, &body, &foot);
    } else {
      grn_output_envelope_open(ctx, &head);
    }
    output_context->is_outputting = true;
  } else {
    if (is_last_message) {
      grn_output_envelope_close(ctx,
                                &foot,
                                ctx->rc,
                                input_path,
                                number_of_lines);
    }
  }

  fwrite(GRN_TEXT_VALUE(&head), 1, GRN_TEXT_LEN(&head), stream);
  fwrite(GRN_TEXT_VALUE(&body), 1, GRN_TEXT_LEN(&body), stream);
  fwrite(GRN_TEXT_VALUE(&foot), 1, GRN_TEXT_LEN(&foot), stream);
  size_t content_size =
    GRN_TEXT_LEN(&head) + GRN_TEXT_LEN(&body) + GRN_TEXT_LEN(&foot);
  if (is_last_message && content_size > 0) {
    fputc('\n', stream);
    content_size++;
  }
  fflush(stream);
  if (content_size > 0) {
    GRN_QUERY_LOG(ctx,
                  GRN_QUERY_LOG_SIZE,
                  ":",
                  "send(%" GRN_FMT_SIZE ")",
                  content_size);
  }
  GRN_OBJ_FIN(ctx, &head);
  GRN_OBJ_FIN(ctx, &foot);

  if (is_last_message) {
    command = GRN_CTX_USER_DATA(ctx)->ptr;
    GRN_BULK_REWIND(command);

    output_context->is_outputting = false;
  }

  GRN_OBJ_FIN(ctx, &body);
}

static void
s_output(grn_ctx *ctx, int flags, void *arg)
{
  standalone_output_context *output_context = arg;

  switch (grn_ctx_get_output_type(ctx)) {
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
  case GRN_CONTENT_NONE:
    s_output_raw(ctx, flags, output_context);
    break;
  default:
    s_output_typed(ctx, flags, output_context);
    break;
  }
}

static void
create_pid_file(void)
{
  FILE *pid_file = NULL;

  if (!pid_file_path) {
    return;
  }

  pid_file = fopen(pid_file_path, "w");
  if (!pid_file) {
    fprintf(stderr,
            "Failed to open PID file: <%s>: <%s>\n",
            pid_file_path,
            grn_strerror(errno));
    return;
  }

  {
#ifdef WIN32
    DWORD pid;
    pid = GetCurrentProcessId();
    fprintf(pid_file, "%" GRN_FMT_DWORD "\n", pid);
#else  /* WIN32 */
    pid_t pid;
    pid = grn_getpid();
    fprintf(pid_file, "%d\n", pid);
#endif /* WIN32 */
  }
  fclose(pid_file);
}

static void
clean_pid_file(void)
{
  if (pid_file_path) {
    grn_unlink(pid_file_path);
  }
}

static bool
do_alone_allow_db_reopen(grn_ctx *ctx,
                         grn_obj *text,
                         grn_obj **db,
                         const char *db_path)
{
  bool is_close = false;
  bool is_reopen = false;
  if (GRN_TEXT_EQUAL_CSTRING(text, "_database_close")) {
    is_close = true;
  } else if (GRN_TEXT_EQUAL_CSTRING(text, "_database_reopen")) {
    is_reopen = true;
  } else {
    return false;
  }

  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_COMMAND,
                ">",
                "%.*s",
                (int)GRN_TEXT_LEN(text),
                GRN_TEXT_VALUE(text));
  if (is_close) {
    grn_obj_close(ctx, *db);
    *db = NULL;
  } else if (is_reopen) {
    if (*db) {
      grn_obj_close(ctx, *db);
    }
    if (db_path) {
      *db = grn_db_open(ctx, db_path);
    } else {
      *db = grn_db_create(ctx, NULL, NULL);
    }
  }
  return true;
}

static int
do_alone(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;
  char *path = NULL;
  grn_obj *db;
  grn_ctx ctx_, *ctx = &ctx_;
  create_pid_file();
  grn_ctx_init(ctx, 0);
  grn_ctx_set_wal_role(ctx, wal_role);
  if (argc > 0 && argv) {
    path = *argv++;
    argc--;
  }
  db =
    (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
  if (db) {
    const char *allow_db_reopen_env = getenv("GROONGA_ALLOW_DATABASE_REOPEN");
    bool allow_db_reopen =
      (allow_db_reopen_env && (strcmp(allow_db_reopen_env, "yes") == 0));
    grn_obj command;
    GRN_TEXT_INIT(&command, 0);
    GRN_CTX_USER_DATA(ctx)->ptr = &command;
    standalone_output_context output_context;
    output_context.output = output;
    output_context.is_outputting = false;
    grn_ctx_recv_handler_set(ctx, s_output, &output_context);
    grn_obj *load_command = grn_ctx_get(ctx, "load", -1);
    if (!argc) {
      grn_obj text;
      GRN_TEXT_INIT(&text, 0);
      while (prompt(ctx, &text) == GRN_SUCCESS) {
        const bool is_loading =
          (ctx->impl->command.keep.command == load_command);
        if (!is_loading) {
          GRN_TEXT_PUT(ctx,
                       &command,
                       GRN_TEXT_VALUE(&text),
                       GRN_TEXT_LEN(&text));
          if (allow_db_reopen) {
            bool processed = do_alone_allow_db_reopen(ctx, &text, &db, path);
            if (processed) {
              ctx->impl->curr_expr = NULL;
              grn_ctx_set_output_type(ctx, GRN_CONTENT_JSON);
              grn_ctx_output_bool(ctx, ctx->rc == GRN_SUCCESS);
              grn_ctx_output_flush(ctx, GRN_CTX_TAIL);
              GRN_QUERY_LOG(ctx,
                            GRN_QUERY_LOG_RESULT_CODE,
                            "<",
                            "rc=%d",
                            ctx->rc);
              if (ctx->rc == GRN_SUCCESS) {
                continue;
              } else {
                break;
              }
            }
          }
        }
        grn_ctx_send(ctx, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text), 0);
        if (ctx->stat == GRN_CTX_QUIT) {
          break;
        }
        if (!is_loading) {
          ERRCLR(ctx);
          if (ctx->rc == GRN_CANCEL) {
            ctx->rc = GRN_SUCCESS;
          }
        }
      }
      exit_code = grn_rc_to_exit_code(ctx->rc);
      grn_obj_unlink(ctx, &text);
    } else {
      grn_rc rc;
      rc = grn_ctx_sendv(ctx, argc, argv, 0);
      exit_code = grn_rc_to_exit_code(rc);
    }
    GRN_OBJ_FIN(ctx, &command);
    grn_obj_close(ctx, db);
  } else {
    fprintf(stderr, "db open failed (%s): %s\n", path, ctx->errbuf);
  }
  grn_ctx_fin(ctx);
  clean_pid_file();
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
      if (ctx->rc == GRN_SUCCESS) {
        GRN_TEXT_SET(ctx, &body, str, str_len);
      } else {
        ERR(ctx->rc, "%.*s", str_len, str);
      }
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
  if (argc > 0 && argv) {
    hostname = *argv++;
    argc--;
  }
  grn_ctx_init(ctx, 0);
  if (!grn_ctx_connect(ctx, hostname, port, 0)) {
    if (!argc) {
      grn_obj text;
      GRN_TEXT_INIT(&text, 0);
      while (prompt(ctx, &text) == GRN_SUCCESS) {
        grn_ctx_send(ctx, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text), 0);
        exit_code = grn_rc_to_exit_code(ctx->rc);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
        if (c_output(ctx)) {
          goto exit;
        }
        if (ctx->stat == GRN_CTX_QUIT) {
          break;
        }
      }
      grn_obj_unlink(ctx, &text);
    } else {
      grn_rc rc;
      rc = grn_ctx_sendv(ctx, argc, argv, 0);
      exit_code = grn_rc_to_exit_code(rc);
      if (c_output(ctx)) {
        goto exit;
      }
    }
  } else {
    fprintf(stderr, "grn_ctx_connect failed (%s:%d)\n", hostname, port);
  }
exit:
  grn_ctx_fin(ctx);
  return exit_code;
}

/* server */

typedef void (*grn_edge_dispatcher_func)(grn_ctx *ctx, grn_edge *edge);
typedef void (*grn_handler_func)(grn_ctx *ctx, grn_obj *msg);

static grn_com_queue ctx_new;
static grn_com_queue ctx_old;
static grn_critical_section q_critical_section;
static grn_cond q_cond;
static uint32_t n_running_threads = 0;
static uint32_t n_floating_threads = 0;
static uint32_t max_n_floating_threads;

static uint32_t
groonga_get_thread_limit(void *data)
{
  return max_n_floating_threads;
}

static void
groonga_set_thread_limit_with_ctx(grn_ctx *ctx, uint32_t new_limit, void *data)
{
  uint32_t i;
  uint32_t current_n_floating_threads;
  uint32_t current_max_n_floating_threads;
  static uint32_t n_changing_threads = 0;
  uint32_t prev_n_changing_threads;

  GRN_ATOMIC_ADD_EX(&n_changing_threads, 1, prev_n_changing_threads);

  CRITICAL_SECTION_ENTER(q_critical_section);
  current_n_floating_threads = n_floating_threads;
  current_max_n_floating_threads = max_n_floating_threads;
  max_n_floating_threads = new_limit;
  CRITICAL_SECTION_LEAVE(q_critical_section);

  if (prev_n_changing_threads > 0) {
    GRN_ATOMIC_ADD_EX(&n_changing_threads, -1, prev_n_changing_threads);
    return;
  }

  if (current_n_floating_threads > new_limit) {
    for (i = 0; i < current_n_floating_threads; i++) {
      CRITICAL_SECTION_ENTER(q_critical_section);
      COND_SIGNAL(q_cond);
      CRITICAL_SECTION_LEAVE(q_critical_section);
    }
  }

  while (GRN_TRUE) {
    grn_bool is_reduced;
    CRITICAL_SECTION_ENTER(q_critical_section);
    is_reduced = (n_running_threads <= max_n_floating_threads);
    if (!is_reduced && n_floating_threads > 0) {
      COND_SIGNAL(q_cond);
    }
    CRITICAL_SECTION_LEAVE(q_critical_section);
    if (is_reduced) {
      break;
    }
    if (ctx && ctx->rc == GRN_CANCEL) {
      CRITICAL_SECTION_ENTER(q_critical_section);
      max_n_floating_threads = current_max_n_floating_threads;
      CRITICAL_SECTION_LEAVE(q_critical_section);
      break;
    }
    grn_nanosleep(1000000);
  }

  GRN_ATOMIC_ADD_EX(&n_changing_threads, -1, prev_n_changing_threads);
}

static void
groonga_set_thread_limit(uint32_t new_limit, void *data)
{
  groonga_set_thread_limit_with_ctx(NULL, new_limit, data);
}

typedef struct {
  grn_mutex mutex;
  grn_ctx ctx;
  grn_pat *entries;
  uint64_t earliest_unix_time_msec;
} request_timer_data;
static request_timer_data the_request_timer_data;

static void *
request_timer_register(const char *request_id,
                       unsigned int request_id_size,
                       double timeout,
                       void *user_data)
{
  request_timer_data *data = user_data;
  grn_id id = GRN_ID_NIL;

  {
    grn_ctx *ctx = &(data->ctx);
    grn_bool is_first_timer;
    grn_timeval tv;
    uint64_t timeout_unix_time_msec;
    void *value;

    MUTEX_LOCK(data->mutex);
    is_first_timer = (grn_pat_size(ctx, data->entries) == 0);
    grn_timeval_now(ctx, &tv);
    timeout_unix_time_msec = GRN_TIMEVAL_TO_MSEC(&tv) + (timeout * 1000);
    while (GRN_TRUE) {
      int added;
      id = grn_pat_add(ctx,
                       data->entries,
                       &timeout_unix_time_msec,
                       sizeof(uint64_t),
                       &value,
                       &added);
      if (added != 0) {
        break;
      }
      timeout_unix_time_msec++;
    }
    grn_memcpy(value, &request_id_size, sizeof(unsigned int));
    grn_memcpy(((uint8_t *)value) + sizeof(unsigned int),
               request_id,
               request_id_size);
    if (data->earliest_unix_time_msec == 0 ||
        data->earliest_unix_time_msec > timeout_unix_time_msec) {
      data->earliest_unix_time_msec = timeout_unix_time_msec;
    }
    if (is_first_timer) {
      break_accept_event_loop(ctx);
    }
    MUTEX_UNLOCK(data->mutex);
  }

  return (void *)(uint64_t)id;
}

static void
request_timer_unregister(void *timer_id, void *user_data)
{
  request_timer_data *data = user_data;
  grn_id id = (grn_id)(uint64_t)timer_id;

  {
    grn_ctx *ctx = &(data->ctx);
    uint64_t timeout_unix_time_msec;
    int key_size;

    MUTEX_LOCK(data->mutex);
    key_size = grn_pat_get_key(ctx,
                               data->entries,
                               id,
                               &timeout_unix_time_msec,
                               sizeof(uint64_t));
    if (key_size > 0) {
      grn_pat_delete_by_id(ctx, data->entries, id, NULL);
      if (data->earliest_unix_time_msec >= timeout_unix_time_msec) {
        data->earliest_unix_time_msec = 0;
      }
    }
    MUTEX_UNLOCK(data->mutex);
  }
}

static void
request_timer_fin(void *user_data)
{
  request_timer_data *data = user_data;

  {
    grn_ctx *ctx = &(data->ctx);
    grn_pat_close(ctx, data->entries);
    grn_ctx_fin(ctx);
    MUTEX_FIN(data->mutex);
  }
}

static void
request_timer_init(void)
{
  static grn_request_timer timer;
  request_timer_data *data = &the_request_timer_data;
  grn_ctx *ctx;

  MUTEX_INIT(data->mutex);
  ctx = &(data->ctx);
  grn_ctx_init(ctx, 0);
  data->entries = grn_pat_create(ctx,
                                 NULL,
                                 sizeof(uint64_t),
                                 GRN_TABLE_MAX_KEY_SIZE,
                                 GRN_OBJ_KEY_UINT);
  data->earliest_unix_time_msec = 0;

  timer.user_data = data;
  timer.register_func = request_timer_register;
  timer.unregister_func = request_timer_unregister;
  timer.fin_func = request_timer_fin;

  grn_request_timer_set(&timer);
}

static grn_bool
request_timer_ensure_earliest_unix_time_msec(void)
{
  request_timer_data *data = &the_request_timer_data;
  grn_ctx *ctx;
  grn_pat_cursor *cursor;

  if (data->earliest_unix_time_msec > 0) {
    return GRN_TRUE;
  }

  ctx = &(data->ctx);
  cursor = grn_pat_cursor_open(ctx,
                               data->entries,
                               NULL,
                               0,
                               NULL,
                               0,
                               0,
                               1,
                               GRN_CURSOR_ASCENDING);
  if (!cursor) {
    return GRN_FALSE;
  }
  while (grn_pat_cursor_next(ctx, cursor) != GRN_ID_NIL) {
    void *key;
    uint64_t timeout_unix_time_msec;

    grn_pat_cursor_get_key(ctx, cursor, &key);
    timeout_unix_time_msec = *(uint64_t *)key;
    data->earliest_unix_time_msec = timeout_unix_time_msec;
    break;
  }
  grn_pat_cursor_close(ctx, cursor);

  return data->earliest_unix_time_msec > 0;
}

static int
request_timer_get_poll_timeout(void)
{
  request_timer_data *data = &the_request_timer_data;
  int timeout = 1000;
  grn_ctx *ctx;
  grn_timeval tv;

  MUTEX_LOCK(data->mutex);
  ctx = &(data->ctx);
  if (grn_pat_size(ctx, data->entries) == 0) {
    goto exit;
  }

  if (!request_timer_ensure_earliest_unix_time_msec()) {
    goto exit;
  }

  grn_timeval_now(ctx, &tv);
  timeout = data->earliest_unix_time_msec - GRN_TIMEVAL_TO_MSEC(&tv);
  if (timeout < 0) {
    timeout = 0;
  } else if (timeout > 1000) {
    timeout = 1000;
  }

exit:
  MUTEX_UNLOCK(data->mutex);

  return timeout;
}

static void
request_timer_process_timeout(void)
{
  request_timer_data *data = &the_request_timer_data;
  grn_ctx *ctx;
  grn_timeval tv;
  uint64_t max;
  grn_pat_cursor *cursor;

  ctx = &(data->ctx);
  if (grn_pat_size(ctx, data->entries) == 0) {
    return;
  }

  grn_timeval_now(ctx, &tv);
  max = GRN_TIMEVAL_TO_MSEC(&tv);
  cursor = grn_pat_cursor_open(ctx,
                               data->entries,
                               NULL,
                               0,
                               &max,
                               sizeof(uint64_t),
                               0,
                               -1,
                               GRN_CURSOR_ASCENDING);
  if (!cursor) {
    return;
  }

  grn_id id;
  while ((id = grn_pat_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    void *value;
    const char *request_id;
    unsigned int request_id_size;

    grn_pat_cursor_get_value(ctx, cursor, &value);
    request_id_size = *((unsigned int *)value);
    request_id = (const char *)(((uint8_t *)value) + sizeof(unsigned int));
    grn_request_canceler_cancel(request_id, request_id_size);
  }
  grn_pat_cursor_close(ctx, cursor);
}

static void
reset_ready_notify_pipe(void)
{
  ready_notify_pipe[PIPE_READ] = 0;
  ready_notify_pipe[PIPE_WRITE] = 0;
}

static void
close_ready_notify_pipe(void)
{
  if (ready_notify_pipe[PIPE_READ] > 0) {
    close(ready_notify_pipe[PIPE_READ]);
  }
  if (ready_notify_pipe[PIPE_WRITE] > 0) {
    close(ready_notify_pipe[PIPE_WRITE]);
  }
  reset_ready_notify_pipe();
}

/* FIXME: callers ignore the return value of send_ready_notify. */
static grn_bool
send_ready_notify(void)
{
  if (ready_notify_pipe[PIPE_WRITE] > 0) {
    const char *ready_notify_message = "ready";
    const size_t ready_notify_message_len = strlen(ready_notify_message);
    size_t n = 0;
    do {
      ssize_t m = write(ready_notify_pipe[PIPE_WRITE],
                        ready_notify_message + n,
                        ready_notify_message_len - n);
      if (m == -1) {
        close_ready_notify_pipe();
        return GRN_FALSE;
      }
      n += m;
    } while (n < ready_notify_message_len);
  }
  close_ready_notify_pipe();
  return GRN_TRUE;
}

static int
daemonize(void)
{
  int exit_code = EXIT_SUCCESS;
#ifndef WIN32

  if (pipe(ready_notify_pipe) == -1) {
    reset_ready_notify_pipe();
  }

  switch (fork()) {
  case 0:
    break;
  case -1:
    perror("fork");
    return EXIT_FAILURE;
  default:
    wait(NULL);
    if (ready_notify_pipe[PIPE_READ] > 0) {
      int max_fd;
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(ready_notify_pipe[PIPE_READ], &read_fds);
      max_fd = ready_notify_pipe[PIPE_READ] + 1;
      select(max_fd, &read_fds, NULL, NULL, NULL);
    }
    close_ready_notify_pipe();
    _exit(EXIT_SUCCESS);
  }
  switch (fork()) {
  case 0:
    if (pid_file_path) {
      create_pid_file();
    } else {
      pid_t pid;
      pid = grn_getpid();
      fprintf(stderr, "%d\n", pid);
    }
    break;
  case -1:
    perror("fork");
    return EXIT_FAILURE;
  default:
    close_ready_notify_pipe();
    _exit(EXIT_SUCCESS);
  }
  {
    int null_fd;
    grn_open(null_fd, "/dev/null", O_RDWR);
    if (null_fd != -1) {
      dup2(null_fd, STDIN_FILENO);
      dup2(null_fd, STDOUT_FILENO);
      dup2(null_fd, STDERR_FILENO);
      if (null_fd > STDERR_FILENO) {
        grn_close(null_fd);
      }
    }
  }
#endif /* WIN32 */
  return exit_code;
}

static void
run_server_loop(grn_ctx *ctx, grn_com_event *ev)
{
  request_timer_init();
  running_event_loop = GRN_TRUE;
  while (!grn_com_event_poll(ctx, ev, request_timer_get_poll_timeout()) &&
         grn_gctx.stat != GRN_CTX_QUIT) {
#ifdef ENABLE_LOG_REOPEN_BY_SIGNAL
    reopen_log_by_signal(ctx);
#endif
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
    request_timer_process_timeout();
    /* todo : log stat */
  }
  running_event_loop = GRN_FALSE;
  for (;;) {
    uint32_t i;
    CRITICAL_SECTION_ENTER(q_critical_section);
    for (i = 0; i < n_floating_threads; i++) {
      COND_SIGNAL(q_cond);
    }
    CRITICAL_SECTION_LEAVE(q_critical_section);
    if (n_running_threads == 0) {
      break;
    }
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
    GRN_HASH_EACH(ctx, ev->hash, id, NULL, NULL, &com, {
      grn_com_close(ctx, com);
    });
  }
}

static int
run_server(grn_ctx *ctx,
           grn_obj *db,
           grn_com_event *ev,
           grn_edge_dispatcher_func dispatcher,
           grn_handler_func handler)
{
  int exit_code = EXIT_SUCCESS;
  struct hostent *he;
  if (!(he = gethostbyname(hostname))) {
    send_ready_notify();
    SOERR("gethostbyname");
  } else {
    ev->opaque = db;
    grn_edges_init(ctx, dispatcher);
    grn_job_queue_current_set(ctx, &ctx_new);
    if (!grn_com_sopen(ctx, ev, bind_address, port, handler, he)) {
      send_ready_notify();
      run_server_loop(ctx, ev);
      exit_code = EXIT_SUCCESS;
    } else {
      send_ready_notify();
      fprintf(stderr,
              "grn_com_sopen failed (%s:%d): %s\n",
              bind_address,
              port,
              ctx->errbuf);
    }
    grn_job_queue_current_set(ctx, NULL);
    grn_edges_fin(ctx);
  }
  return exit_code;
}

static grn_bool
memcached_init(grn_ctx *ctx);

static int
start_service(grn_ctx *ctx,
              const char *db_path,
              grn_edge_dispatcher_func dispatcher,
              grn_handler_func handler)
{
  int exit_code = EXIT_SUCCESS;
  grn_com_event ev;

  if (is_daemon_mode) {
    exit_code = daemonize();
    if (exit_code != EXIT_SUCCESS) {
      return exit_code;
    }
  } else {
    create_pid_file();
  }

  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
    grn_obj *db;
    db = (newdb || !db_path) ? grn_db_create(ctx, db_path, NULL)
                             : grn_db_open(ctx, db_path);
    if (db) {
      if (is_memcached_mode) {
        if (!memcached_init(ctx)) {
          fprintf(stderr,
                  "failed to initialize memcached mode: %s\n",
                  ctx->errbuf);
          exit_code = EXIT_FAILURE;
          send_ready_notify();
        }
      }
      if (exit_code == EXIT_SUCCESS) {
        grn_com_event_set_listen_backlog(ctx, &ev, listen_backlog);
        exit_code = run_server(ctx, db, &ev, dispatcher, handler);
      }
      grn_obj_close(ctx, db);
    } else {
      fprintf(stderr, "db open failed (%s): %s\n", db_path, ctx->errbuf);
      exit_code = EXIT_FAILURE;
      send_ready_notify();
    }
    grn_com_event_fin(ctx, &ev);
  } else {
    fprintf(stderr, "grn_com_event_init failed\n");
    exit_code = EXIT_FAILURE;
    send_ready_notify();
  }

  clean_pid_file();

  return exit_code;
}

typedef struct {
  grn_msg *msg;
  grn_bool in_body;
  grn_bool is_chunked;
} ht_context;

static void
h_output_set_header(grn_ctx *ctx,
                    grn_obj *header,
                    grn_rc rc,
                    int64_t content_length,
                    grn_obj *foot)
{
  switch (rc) {
  case GRN_SUCCESS:
    GRN_TEXT_SETS(ctx, header, "HTTP/1.1 200 OK\r\n");
    break;
  case GRN_INVALID_ARGUMENT:
  case GRN_FUNCTION_NOT_IMPLEMENTED:
  case GRN_SYNTAX_ERROR:
    GRN_TEXT_SETS(ctx, header, "HTTP/1.1 400 Bad Request\r\n");
    break;
  case GRN_NO_SUCH_FILE_OR_DIRECTORY:
    GRN_TEXT_SETS(ctx, header, "HTTP/1.1 404 Not Found\r\n");
    break;
  case GRN_CANCEL:
    GRN_TEXT_SETS(ctx, header, "HTTP/1.1 408 Request Timeout\r\n");
    break;
  default:
    GRN_TEXT_SETS(ctx, header, "HTTP/1.1 500 Internal Server Error\r\n");
    break;
  }
  GRN_TEXT_PUT(ctx,
               header,
               GRN_TEXT_VALUE(&http_response_server_line),
               GRN_TEXT_LEN(&http_response_server_line));
  GRN_TEXT_PUTS(ctx, header, "Content-Type: ");
  if (grn_ctx_get_output_type(ctx) == GRN_CONTENT_JSON && foot &&
      GRN_TEXT_LEN(foot) > 0 &&
      GRN_TEXT_VALUE(foot)[GRN_TEXT_LEN(foot) - 1] == ';') {
    GRN_TEXT_PUTS(ctx, header, "application/javascript");
  } else {
    GRN_TEXT_PUTS(ctx, header, grn_ctx_get_mime_type(ctx));
  }
  GRN_TEXT_PUTS(ctx, header, "\r\n");
  GRN_TEXT_PUTS(ctx, header, "Connection: close\r\n");
  if (content_length >= 0) {
    GRN_TEXT_PUTS(ctx, header, "Content-Length: ");
    grn_text_lltoa(ctx, header, content_length);
    GRN_TEXT_PUTS(ctx, header, "\r\n");
  } else {
    GRN_TEXT_PUTS(ctx, header, "Transfer-Encoding: chunked\r\n");
  }
  GRN_TEXT_PUTS(ctx, header, "\r\n");
}

static void
h_output_send(grn_ctx *ctx,
              grn_sock fd,
              grn_obj *header,
              grn_obj *head,
              grn_obj *body,
              grn_obj *foot)
{
  ssize_t ret;
  ssize_t len = 0;
#ifdef WIN32
  int n_buffers = 0;
  WSABUF wsabufs[4];
  if (header) {
    wsabufs[n_buffers].buf = GRN_TEXT_VALUE(header);
    wsabufs[n_buffers].len = GRN_TEXT_LEN(header);
    len += GRN_TEXT_LEN(header);
    n_buffers++;
  }
  if (head) {
    wsabufs[n_buffers].buf = GRN_TEXT_VALUE(head);
    wsabufs[n_buffers].len = GRN_TEXT_LEN(head);
    len += GRN_TEXT_LEN(head);
    n_buffers++;
  }
  if (body) {
    wsabufs[n_buffers].buf = GRN_TEXT_VALUE(body);
    wsabufs[n_buffers].len = GRN_TEXT_LEN(body);
    len += GRN_TEXT_LEN(body);
    n_buffers++;
  }
  if (foot) {
    wsabufs[n_buffers].buf = GRN_TEXT_VALUE(foot);
    wsabufs[n_buffers].len = GRN_TEXT_LEN(foot);
    len += GRN_TEXT_LEN(foot);
    n_buffers++;
  }
  {
    DWORD sent = 0;
    if (WSASend(fd, wsabufs, n_buffers, &sent, 0, NULL, NULL) == SOCKET_ERROR) {
      SOERR("WSASend");
    }
    ret = sent;
  }
#else  /* WIN32 */
  struct iovec msg_iov[4];
  struct msghdr msg;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = msg_iov;
  msg.msg_iovlen = 0;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  if (header) {
    msg_iov[msg.msg_iovlen].iov_base = GRN_TEXT_VALUE(header);
    msg_iov[msg.msg_iovlen].iov_len = GRN_TEXT_LEN(header);
    len += GRN_TEXT_LEN(header);
    msg.msg_iovlen++;
  }
  if (head) {
    msg_iov[msg.msg_iovlen].iov_base = GRN_TEXT_VALUE(head);
    msg_iov[msg.msg_iovlen].iov_len = GRN_TEXT_LEN(head);
    len += GRN_TEXT_LEN(head);
    msg.msg_iovlen++;
  }
  if (body) {
    msg_iov[msg.msg_iovlen].iov_base = GRN_TEXT_VALUE(body);
    msg_iov[msg.msg_iovlen].iov_len = GRN_TEXT_LEN(body);
    len += GRN_TEXT_LEN(body);
    msg.msg_iovlen++;
  }
  if (foot) {
    msg_iov[msg.msg_iovlen].iov_base = GRN_TEXT_VALUE(foot);
    msg_iov[msg.msg_iovlen].iov_len = GRN_TEXT_LEN(foot);
    len += GRN_TEXT_LEN(foot);
    msg.msg_iovlen++;
  }
  if ((ret = sendmsg(fd, &msg, MSG_NOSIGNAL)) == -1) {
    SOERR("sendmsg");
  }
#endif /* WIN32 */
  if (ret != len) {
    GRN_LOG(&grn_gctx,
            GRN_LOG_NOTICE,
            "couldn't send all data (%" GRN_FMT_SSIZE "/%" GRN_FMT_SSIZE ")",
            ret,
            len);
  }
  GRN_QUERY_LOG(ctx,
                GRN_QUERY_LOG_SIZE,
                ":",
                "send(%" GRN_FMT_SSIZE "): %" GRN_FMT_SSIZE "/%" GRN_FMT_SSIZE,
                len,
                ret,
                len);
}

static void
h_output_raw(grn_ctx *ctx, int flags, ht_context *hc)
{
  grn_rc expr_rc = ctx->rc;
  grn_sock fd = hc->msg->u.fd;
  grn_obj header_;
  grn_obj head_;
  grn_obj body_;
  grn_obj foot_;
  grn_obj *header = NULL;
  grn_obj *head = NULL;
  grn_obj *body = NULL;
  grn_obj *foot = NULL;
  char *chunk = NULL;
  unsigned int chunk_size = 0;
  int recv_flags;
  grn_bool is_last_message = (flags & GRN_CTX_TAIL);

  GRN_TEXT_INIT(&header_, 0);
  GRN_TEXT_INIT(&head_, 0);
  GRN_TEXT_INIT(&body_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_INIT(&foot_, 0);

  grn_ctx_recv(ctx, &chunk, &chunk_size, &recv_flags);
  GRN_TEXT_SET(ctx, &body_, chunk, chunk_size);

  if (!hc->in_body) {
    if (is_last_message) {
      h_output_set_header(ctx, &header_, expr_rc, GRN_TEXT_LEN(&body_), NULL);
      hc->is_chunked = GRN_FALSE;
    } else {
      h_output_set_header(ctx, &header_, expr_rc, -1, NULL);
      hc->is_chunked = GRN_TRUE;
    }
    header = &header_;
    hc->in_body = GRN_TRUE;
  }

  if (GRN_TEXT_LEN(&body_) > 0) {
    if (hc->is_chunked) {
      grn_text_printf(ctx,
                      &head_,
                      "%x\r\n",
                      (unsigned int)GRN_TEXT_LEN(&body_));
      head = &head_;
      GRN_TEXT_PUTS(ctx, &foot_, "\r\n");
      foot = &foot_;
    }
    body = &body_;
  }

  if (is_last_message) {
    if (hc->is_chunked) {
      GRN_TEXT_PUTS(ctx, &foot_, "0\r\n");
      GRN_TEXT_PUTS(ctx, &foot_, "\r\n");
      foot = &foot_;
    }
  }

  h_output_send(ctx, fd, header, head, body, foot);

  GRN_OBJ_FIN(ctx, &foot_);
  GRN_OBJ_FIN(ctx, &body_);
  GRN_OBJ_FIN(ctx, &head_);
  GRN_OBJ_FIN(ctx, &header_);
}

static void
h_output_typed(grn_ctx *ctx, int flags, ht_context *hc)
{
  grn_rc expr_rc = ctx->rc;
  grn_sock fd = hc->msg->u.fd;
  grn_obj header_;
  grn_obj head_;
  grn_obj body_;
  grn_obj foot_;
  grn_obj *header = NULL;
  grn_obj *head = NULL;
  grn_obj *body = NULL;
  grn_obj *foot = NULL;
  char *chunk = NULL;
  unsigned int chunk_size = 0;
  int recv_flags;
  grn_bool should_return_body;
  grn_bool is_last_message = (flags & GRN_CTX_TAIL);

  switch (hc->msg->header.qtype) {
  case 'G':
  case 'P':
    should_return_body = GRN_TRUE;
    break;
  default:
    should_return_body = GRN_FALSE;
    break;
  }

  GRN_TEXT_INIT(&header_, 0);
  GRN_TEXT_INIT(&head_, 0);
  GRN_TEXT_INIT(&body_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_INIT(&foot_, 0);

  grn_ctx_recv(ctx, &chunk, &chunk_size, &recv_flags);
  GRN_TEXT_SET(ctx, &body_, chunk, chunk_size);

  if (!hc->in_body) {
    if (is_last_message) {
      output_envelope(ctx, expr_rc, &head_, &body_, &foot_);
      h_output_set_header(ctx,
                          &header_,
                          expr_rc,
                          GRN_TEXT_LEN(&head_) + GRN_TEXT_LEN(&body_) +
                            GRN_TEXT_LEN(&foot_),
                          &foot_);
      head = &head_;
      body = &body_;
      foot = &foot_;
      hc->is_chunked = false;
    } else {
      grn_obj first_output;
      GRN_TEXT_INIT(&first_output, 0);
      grn_output_envelope_open(ctx, &first_output);
      size_t first_output_length = GRN_TEXT_LEN(&first_output);
      if (first_output_length > 0) {
        grn_text_printf(ctx,
                        &head_,
                        "%x\r\n",
                        (unsigned int)first_output_length);
        GRN_TEXT_PUT(ctx,
                     &head_,
                     GRN_TEXT_VALUE(&first_output),
                     first_output_length);
        GRN_TEXT_PUTS(ctx, &head_, "\r\n");
        head = &head_;
      }
      GRN_OBJ_FIN(ctx, &first_output);

      h_output_set_header(ctx, &header_, expr_rc, -1, NULL);
      hc->is_chunked = true;
    }
    header = &header_;
    hc->in_body = true;
  }

  if (!should_return_body) {
    h_output_send(ctx, fd, header, NULL, NULL, NULL);
    goto exit;
  }

  if (!hc->is_chunked) {
    if (GRN_TEXT_LEN(&body_) > 0) {
      body = &body_;
    }
    h_output_send(ctx, fd, header, head, body, foot);
    goto exit;
  }

  grn_obj chunk_;
  GRN_TEXT_INIT(&chunk_, GRN_OBJ_DO_SHALLOW_COPY);
  const size_t max_chunk_size = 64 * 1024 * 1024;
  size_t offset = 0;
  for (offset = 0; GRN_TEXT_LEN(&body_) - offset > max_chunk_size;
       offset += max_chunk_size) {
    GRN_TEXT_SET(ctx, &chunk_, GRN_TEXT_VALUE(&body_) + offset, max_chunk_size);
    grn_text_printf(ctx, &head_, "%x\r\n", (unsigned int)GRN_TEXT_LEN(&chunk_));
    GRN_TEXT_PUTS(ctx, &foot_, "\r\n");
    h_output_send(ctx, fd, header, &head_, &chunk_, &foot_);
    GRN_BULK_REWIND(&head_);
    GRN_BULK_REWIND(&foot_);
    header = NULL;
    head = NULL;
    foot = NULL;
  }
  if (GRN_TEXT_LEN(&body_) - offset > 0) {
    GRN_TEXT_SET(ctx,
                 &chunk_,
                 GRN_TEXT_VALUE(&body_) + offset,
                 GRN_TEXT_LEN(&body_) - offset);
    grn_text_printf(ctx, &head_, "%x\r\n", (unsigned int)GRN_TEXT_LEN(&chunk_));
    head = &head_;
    GRN_TEXT_PUTS(ctx, &foot_, "\r\n");
    foot = &foot_;
    body = &chunk_;
  }
  if (is_last_message) {
    grn_obj last_output;
    GRN_TEXT_INIT(&last_output, 0);
    grn_output_envelope_close(ctx,
                              &last_output,
                              expr_rc,
                              input_path,
                              number_of_lines);
    size_t last_output_length = GRN_TEXT_LEN(&last_output);
    grn_text_printf(ctx, &foot_, "%x\r\n", (unsigned int)last_output_length);
    GRN_TEXT_PUT(ctx, &foot_, GRN_TEXT_VALUE(&last_output), last_output_length);
    GRN_TEXT_PUTS(ctx, &foot_, "\r\n");
    GRN_OBJ_FIN(ctx, &last_output);

    GRN_TEXT_PUTS(ctx, &foot_, "0\r\n");
    GRN_TEXT_PUTS(ctx, &foot_, "\r\n");
    foot = &foot_;
  }
  h_output_send(ctx, fd, header, head, body, foot);
  GRN_OBJ_FIN(ctx, &chunk_);

exit:
  GRN_OBJ_FIN(ctx, &foot_);
  GRN_OBJ_FIN(ctx, &body_);
  GRN_OBJ_FIN(ctx, &head_);
  GRN_OBJ_FIN(ctx, &header_);
}

static void
h_output(grn_ctx *ctx, int flags, void *arg)
{
  ht_context *hc = (ht_context *)arg;

  switch (grn_ctx_get_output_type(ctx)) {
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
  case GRN_CONTENT_NONE:
    h_output_raw(ctx, flags, hc);
    break;
  default:
    h_output_typed(ctx, flags, hc);
    break;
  }
}

typedef enum {
  HTTP_CONTENT_TYPE_UNKNOWN,
  HTTP_CONTENT_TYPE_WWW_FORM_URLENCODED,
  HTTP_CONTENT_TYPE_JSON,
  HTTP_CONTENT_TYPE_APACHE_ARROW_STREAMING,
} http_content_type;

typedef struct {
  const char *method_start;
  int method_length;
  const char *path_start;
  int path_length;
  http_content_type content_type;
  const char *content_type_raw;
  long long int content_type_raw_length;
  long long int content_length;
  bool have_100_continue;
  bool is_chunked_request;
  const char *body_start;
  const char *body_end;
} h_header;

#define STRING_EQUAL(string, string_length, constant_string)                   \
  (string_length == strlen(constant_string) &&                                 \
   strncmp(string, constant_string, string_length) == 0)

#define STRING_EQUAL_CI(string, string_length, constant_string)                \
  (string_length == strlen(constant_string) &&                                 \
   grn_strncasecmp(string, constant_string, string_length) == 0)

static const char *
h_parse_header_request_line(grn_ctx *ctx,
                            const char *start,
                            const char *end,
                            h_header *header)
{
  const char *current;

  {
    header->method_start = start;
    for (current = header->method_start; current < end; current++) {
      if (current[0] == '\n') {
        return NULL;
      }
      if (current[0] == ' ') {
        header->method_length = current - header->method_start;
        current++;
        break;
      }
    }
    if (header->method_length == -1) {
      return NULL;
    }
  }

  {
    header->path_start = current;
    header->path_length = -1;
    for (; current < end; current++) {
      if (current[0] == '\n') {
        return NULL;
      }
      if (current[0] == ' ') {
        header->path_length = current - header->path_start;
        current++;
        break;
      }
    }
    if (header->path_length == -1) {
      return NULL;
    }
  }

  {
    const char *http_version_start = current;
    int http_version_length = -1;
    for (; current < end; current++) {
      if (current[0] == '\n') {
        http_version_length = current - http_version_start;
        if (http_version_length > 0 &&
            http_version_start[http_version_length - 1] == '\r') {
          http_version_length--;
        }
        current++;
        break;
      }
    }
    if (http_version_length == -1) {
      return NULL;
    }
    if (!(STRING_EQUAL_CI(http_version_start,
                          http_version_length,
                          "HTTP/1.0") ||
          STRING_EQUAL_CI(http_version_start,
                          http_version_length,
                          "HTTP/1.1"))) {
      return NULL;
    }
  }

  return current;
}

static const char *
h_parse_header_values(grn_ctx *ctx,
                      const char *start,
                      const char *end,
                      h_header *header)
{
  const char *current;
  const char *name = start;
  int name_length = -1;
  const char *value = NULL;
  int value_length = -1;

  for (current = start; current < end; current++) {
    switch (current[0]) {
    case '\n':
      if (name_length == -1) {
        if (current - name == 1 && current[-1] == '\r') {
          return current + 1;
        } else {
          /* No ":" header line. TODO: report error. */
          return NULL;
        }
      } else {
        while (value < current && value[0] == ' ') {
          value++;
        }
        value_length = current - value;
        if (value_length > 0 && value[value_length - 1] == '\r') {
          value_length--;
        }
        if (STRING_EQUAL_CI(name, name_length, "Content-Type")) {
          header->content_type_raw = value;
          header->content_type_raw_length = value_length;
          const char *content_type = value;
          int content_type_length = value_length;
          int i;
          for (i = 0; i < content_type_length; i++) {
            if (content_type[i] == ';') {
              content_type_length = i;
              break;
            }
          }
          if (STRING_EQUAL_CI(content_type,
                              content_type_length,
                              "application/x-www-form-urlencoded")) {
            header->content_type = HTTP_CONTENT_TYPE_WWW_FORM_URLENCODED;
          } else if (STRING_EQUAL_CI(content_type,
                                     content_type_length,
                                     "application/json")) {
            header->content_type = HTTP_CONTENT_TYPE_JSON;
          } else if (STRING_EQUAL_CI(content_type,
                                     content_type_length,
                                     "application/vnd.apache.arrow.stream")) {
            header->content_type = HTTP_CONTENT_TYPE_APACHE_ARROW_STREAMING;
          } else if (STRING_EQUAL_CI(content_type,
                                     content_type_length,
                                     "application/x-apache-arrow-streaming")) {
            header->content_type = HTTP_CONTENT_TYPE_APACHE_ARROW_STREAMING;
          } else {
            header->content_type = HTTP_CONTENT_TYPE_UNKNOWN;
          }
        } else if (STRING_EQUAL_CI(name, name_length, "Content-Length")) {
          const char *rest;
          header->content_length =
            grn_atoll(value, value + value_length, &rest);
          if (rest != value + value_length) {
            /* Invalid Content-Length value. TODO: report error. */
            header->content_length = -1;
          }
        } else if (STRING_EQUAL_CI(name, name_length, "Expect")) {
          if (STRING_EQUAL(value, value_length, "100-continue")) {
            header->have_100_continue = true;
          }
        } else if (STRING_EQUAL_CI(name, name_length, "Transfer-Encoding")) {
          if (STRING_EQUAL(value, value_length, "chunked")) {
            header->is_chunked_request = true;
          }
        }
      }
      name = current + 1;
      name_length = -1;
      value = NULL;
      value_length = -1;
      break;
    case ':':
      if (name_length == -1) {
        name_length = current - name;
        value = current + 1;
      }
      break;
    default:
      break;
    }
  }

  return NULL;
}

static grn_bool
h_parse_header(grn_ctx *ctx,
               const char *start,
               const char *end,
               h_header *header)
{
  const char *current;

  current = h_parse_header_request_line(ctx, start, end, header);
  if (!current) {
    return GRN_FALSE;
  }
  current = h_parse_header_values(ctx, current, end, header);
  if (!current) {
    return GRN_FALSE;
  }

  if (current == end) {
    header->body_start = NULL;
  } else {
    header->body_start = current;
  }

  return GRN_TRUE;
}

static void
do_htreq_get(grn_ctx *ctx, ht_context *hc)
{
  grn_msg *msg = hc->msg;
  h_header header;
  header.method_start = NULL;
  header.method_length = -1;
  header.path_start = NULL;
  header.path_length = -1;
  if (!h_parse_header_request_line(ctx,
                                   GRN_BULK_HEAD((grn_obj *)msg),
                                   GRN_BULK_CURR((grn_obj *)msg),
                                   &header)) {
    return;
  }
  grn_ctx_send(ctx, header.path_start, header.path_length, GRN_CTX_TAIL);
}

typedef void (*grn_htreq_post_process_body_chunk_func)(grn_ctx *ctx,
                                                       const char *chunk,
                                                       size_t chunk_size,
                                                       void *user_data);

/* #define GRN_HTTP_POST_DEBUG */
#ifdef GRN_HTTP_POST_DEBUG
#  define grn_http_post_p(...) printf(__VA_ARGS__)
#else
#  define grn_http_post_p(...)
#endif

static const char *
do_htreq_post_process_body_chunked_read_chunk_size(grn_ctx *ctx,
                                                   const char *data,
                                                   size_t data_size,
                                                   size_t *chunk_size,
                                                   size_t total_body_size)
{
  const char *current = data;
  const char *data_end = data + data_size;
  while (data_end - current >= 2) {
    switch (current[0]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      /* OK */
      break;
    case '\r':
      if (current[1] == '\n') {
        if (data == current) {
          ERR(GRN_INVALID_ARGUMENT,
              "[http][post][%" GRN_FMT_SIZE "] "
              "chunked request size line doesn't have content",
              total_body_size);
          return NULL;
        }
        *chunk_size = grn_htoui(data, current, NULL);
        grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] "
                        "read chunk size: %" GRN_FMT_SIZE ": <%.*s>\n",
                        total_body_size,
                        *chunk_size,
                        (int)(current - data),
                        data);
        return current + 2;
      } else {
        ERR(GRN_INVALID_ARGUMENT,
            "[http][post][%" GRN_FMT_SIZE "] "
            "chunked request size line isn't ended by CRLF: "
            "<%.*s>",
            total_body_size,
            (int)((current + 2) - data),
            data);
        return NULL;
      }
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT,
          "[http][post][%" GRN_FMT_SIZE "] "
          "chunked request size line has garbage: "
          "<%.*s>",
          total_body_size,
          (int)((current + 1) - data),
          data);
      return NULL;
    }
    current++;
  }
  return NULL;
}

static void
do_htreq_post_process_body_chunked(grn_ctx *ctx,
                                   h_header *header,
                                   grn_sock fd,
                                   grn_htreq_post_process_body_chunk_func func,
                                   void *user_data)
{
  grn_obj buffer;
  GRN_TEXT_INIT(&buffer, 0);
  size_t buffer_offset = 0;
  const char *data = NULL;
  size_t data_size = 0;
  bool need_chunk_size = true;
  bool need_chunk_end_cr = false;
  bool need_chunk_end_lf = false;
  size_t chunk_size = 0;
  size_t total_body_size = 0;

  if (header->body_start) {
    data = header->body_start;
    data_size = header->body_end - header->body_start;
  }

  while (true) {
    if (!data) {
      const size_t buffer_size = 8192;
      if (GRN_BULK_REST(&buffer) < buffer_size) {
        grn_bulk_reserve(ctx, &buffer, buffer_size);
      }
      int recv_flags = 0;
      ssize_t recv_length =
        recv(fd, GRN_BULK_CURR(&buffer), GRN_BULK_REST(&buffer), recv_flags);
      if (recv_length == 0) {
        grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] no data\n",
                        total_body_size);
        break;
      }
      if (recv_length == -1) {
        SOERR("recv");
        break;
      }
      GRN_BULK_INCR_LEN(&buffer, recv_length);
      data = GRN_BULK_HEAD(&buffer) + buffer_offset;
      data_size = GRN_BULK_VSIZE(&buffer) - buffer_offset;
    }

    if (need_chunk_end_cr) {
      grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] need chunk end CR\n",
                      total_body_size);
      if (data[0] != '\r') {
        ERR(GRN_INVALID_ARGUMENT,
            "[http][post][%" GRN_FMT_SIZE "] "
            "chunk end CR doesn't exit: <%c>",
            total_body_size,
            data[0]);
        break;
      }
      need_chunk_end_cr = false;
      if (data_size == 1) {
        data = NULL;
        data_size = 0;
        GRN_BULK_REWIND(&buffer);
        continue;
      }
      data++;
      data_size--;
    }

    if (need_chunk_end_lf) {
      grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] need chunk end LF\n",
                      total_body_size);
      if (data[0] != '\n') {
        ERR(GRN_INVALID_ARGUMENT,
            "[http][post][%" GRN_FMT_SIZE "] chunk end LF doesn't exit: <%c>",
            total_body_size,
            data[0]);
        break;
      }
      need_chunk_end_lf = false;
      if (data_size == 1) {
        data = NULL;
        data_size = 0;
        GRN_BULK_REWIND(&buffer);
        buffer_offset = 0;
        continue;
      }
      data++;
      data_size--;
    }

    if (need_chunk_size) {
      grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] need chunk size\n",
                      total_body_size);
      const char *chunk_start =
        do_htreq_post_process_body_chunked_read_chunk_size(ctx,
                                                           data,
                                                           data_size,
                                                           &chunk_size,
                                                           total_body_size);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      if (chunk_size == 0) {
        grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] "
                        "zero chunk_size: <%.*s>\n",
                        total_body_size,
                        (int)((data + data_size) - chunk_start),
                        chunk_start);
        break;
      }
      if (!chunk_start) {
        /* TODO: Test this */
        grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] "
                        "no chunk start: %p:%p:%p\n",
                        total_body_size,
                        data,
                        GRN_BULK_HEAD(&buffer),
                        GRN_BULK_CURR(&buffer));
        if (GRN_BULK_HEAD(&buffer) <= data && data < GRN_BULK_CURR(&buffer)) {
          buffer_offset = data - GRN_BULK_HEAD(&buffer);
        } else {
          GRN_TEXT_PUT(ctx, &buffer, data, data_size);
        }
        data = NULL;
        data_size = 0;
        continue;
      }
      size_t read_chunk_size = (data + data_size) - chunk_start;
      data = chunk_start;
      data_size = read_chunk_size;
    }

    grn_http_post_p("[http][post][%" GRN_FMT_SIZE "] "
                    "data: %" GRN_FMT_SIZE ":%" GRN_FMT_SIZE "\n",
                    total_body_size,
                    data_size,
                    chunk_size);
    if (data_size >= chunk_size) {
      func(ctx, data, chunk_size, user_data);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      total_body_size += chunk_size;
      if (data_size == chunk_size) {
        data = NULL;
        data_size = 0;
        GRN_BULK_REWIND(&buffer);
        buffer_offset = 0;
      } else {
        data += chunk_size;
        data_size -= chunk_size;
      }
      need_chunk_end_cr = true;
      need_chunk_end_lf = true;
      need_chunk_size = true;
    } else {
      func(ctx, data, data_size, user_data);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      total_body_size += data_size;
      chunk_size -= data_size;
      data = NULL;
      data_size = 0;
      GRN_BULK_REWIND(&buffer);
      buffer_offset = 0;
      need_chunk_size = false;
    }
  }

  GRN_OBJ_FIN(ctx, &buffer);
}

static void
do_htreq_post_process_body_raw(grn_ctx *ctx,
                               h_header *header,
                               grn_sock fd,
                               grn_htreq_post_process_body_chunk_func func,
                               void *user_data)
{
  long long int read_content_length = 0;
  if (header->body_start) {
    size_t read_body_length = header->body_end - header->body_start;
    read_content_length += read_body_length;
    func(ctx, header->body_start, read_body_length, user_data);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  while (read_content_length < header->content_length) {
#define POST_BUFFER_SIZE 8192
    char buffer[POST_BUFFER_SIZE];
    int recv_flags = 0;
    ssize_t recv_length = recv(fd, buffer, POST_BUFFER_SIZE, recv_flags);
#undef POST_BUFFER_SIZE
    if (recv_length == 0) {
      break;
    }
    if (recv_length == -1) {
      SOERR("recv");
      break;
    }
    func(ctx, buffer, recv_length, user_data);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    read_content_length += recv_length;
  }
}

static void
do_htreq_post_process_body(grn_ctx *ctx,
                           h_header *header,
                           grn_sock fd,
                           grn_htreq_post_process_body_chunk_func func,
                           void *user_data)
{
  if (header->is_chunked_request) {
    do_htreq_post_process_body_chunked(ctx, header, fd, func, user_data);
  } else {
    do_htreq_post_process_body_raw(ctx, header, fd, func, user_data);
  }
}

typedef struct {
  grn_obj command;
  bool have_question;
  bool is_first_chunk;
} http_post_www_form_urlencoded_data;

static void
do_htreq_post_process_body_chunks_www_form_urlencoded(grn_ctx *ctx,
                                                      const char *chunk,
                                                      size_t chunk_size,
                                                      void *user_data)
{
  http_post_www_form_urlencoded_data *data = user_data;

  if (data->is_first_chunk) {
    if (data->have_question) {
      GRN_TEXT_PUTC(ctx, &(data->command), '&');
    } else {
      GRN_TEXT_PUTC(ctx, &(data->command), '?');
    }
    data->is_first_chunk = false;
  }
  GRN_TEXT_PUT(ctx, &(data->command), chunk, chunk_size);
}

static void
do_htreq_post_process_body_www_form_urlencoded(grn_ctx *ctx,
                                               h_header *header,
                                               grn_sock fd)
{
  http_post_www_form_urlencoded_data data;
  GRN_TEXT_INIT(&(data.command), 0);
  GRN_TEXT_PUT(ctx, &(data.command), header->path_start, header->path_length);
  data.have_question = false;
  {
    int i;
    for (i = 0; i < header->path_length; i++) {
      if (header->path_start[i] == '?') {
        data.have_question = true;
        break;
      }
    }
  }
  data.is_first_chunk = true;

  do_htreq_post_process_body(
    ctx,
    header,
    fd,
    do_htreq_post_process_body_chunks_www_form_urlencoded,
    &data);
  if (ctx->rc == GRN_SUCCESS) {
    grn_ctx_send(ctx,
                 GRN_TEXT_VALUE(&(data.command)),
                 GRN_TEXT_LEN(&(data.command)),
                 GRN_CTX_TAIL);
  }
  GRN_OBJ_FIN(ctx, &(data.command));
}

typedef struct {
  grn_obj chunk_buffer1;
  grn_obj chunk_buffer2;
  grn_obj *chunk_buffer_old;
  grn_obj *chunk_buffer_current;
} http_post_load_data;

static void
http_post_load_data_init(grn_ctx *ctx, http_post_load_data *data)
{
  GRN_TEXT_INIT(&(data->chunk_buffer1), 0);
  GRN_TEXT_INIT(&(data->chunk_buffer2), 0);
  data->chunk_buffer_old = &(data->chunk_buffer1);
  data->chunk_buffer_current = &(data->chunk_buffer2);
}

static void
http_post_load_data_fin(grn_ctx *ctx, http_post_load_data *data)
{
  if (ctx->rc == GRN_SUCCESS &&
      (GRN_TEXT_LEN(data->chunk_buffer_old) > 0 ||
       GRN_TEXT_LEN(data->chunk_buffer_current) > 0)) {
    if (GRN_TEXT_LEN(data->chunk_buffer_old) > 0) {
      int flags;
      if (GRN_TEXT_LEN(data->chunk_buffer_current) > 0) {
        flags = GRN_CTX_MORE;
      } else {
        flags = GRN_CTX_TAIL;
      }
      grn_ctx_send(ctx,
                   GRN_TEXT_VALUE(data->chunk_buffer_old),
                   GRN_TEXT_LEN(data->chunk_buffer_old),
                   flags);
    }
    if (GRN_TEXT_LEN(data->chunk_buffer_current) > 0) {
      grn_ctx_send(ctx,
                   GRN_TEXT_VALUE(data->chunk_buffer_current),
                   GRN_TEXT_LEN(data->chunk_buffer_current),
                   GRN_CTX_TAIL);
    }
  } else {
    grn_ctx_send(ctx, NULL, 0, GRN_CTX_TAIL);
  }
  GRN_OBJ_FIN(ctx, &(data->chunk_buffer1));
  GRN_OBJ_FIN(ctx, &(data->chunk_buffer2));
}

static void
http_post_load_data_flush(grn_ctx *ctx, http_post_load_data *data)
{
  if (GRN_TEXT_LEN(data->chunk_buffer_old) > 0) {
    grn_ctx_send(ctx,
                 GRN_TEXT_VALUE(data->chunk_buffer_old),
                 GRN_TEXT_LEN(data->chunk_buffer_old),
                 GRN_CTX_MORE);
    GRN_BULK_REWIND(data->chunk_buffer_old);
  }

  grn_obj *tmp = data->chunk_buffer_current;
  data->chunk_buffer_current = data->chunk_buffer_old;
  data->chunk_buffer_old = tmp;
}

static void
do_htreq_post_process_body_load_chunks_json(grn_ctx *ctx,
                                            const char *chunk,
                                            size_t chunk_size,
                                            void *user_data)
{
  http_post_load_data *data = user_data;

  const char *chunk_start = chunk;
  const char *chunk_end = chunk + chunk_size;
  const char *chunk_current;
  ;
  for (chunk_current = chunk_end - 1; chunk_current > chunk; chunk_current--) {
    bool is_separator;
    switch (chunk_current[0]) {
    case '\n':
    case ',':
      is_separator = true;
      break;
    default:
      is_separator = false;
      break;
    }
    if (!is_separator) {
      continue;
    }

    GRN_TEXT_PUT(ctx,
                 data->chunk_buffer_current,
                 chunk,
                 chunk_current + 1 - chunk);
    http_post_load_data_flush(ctx, data);
    chunk_start = chunk_current + 1;
    break;
  }

  if (chunk_end > chunk_start) {
    GRN_TEXT_PUT(ctx,
                 data->chunk_buffer_current,
                 chunk_start,
                 chunk_end - chunk_start);
  }
}

static void
do_htreq_post_process_body_load_json(grn_ctx *ctx,
                                     h_header *header,
                                     grn_sock fd)
{
  http_post_load_data data;
  http_post_load_data_init(ctx, &data);
  do_htreq_post_process_body(ctx,
                             header,
                             fd,
                             do_htreq_post_process_body_load_chunks_json,
                             &data);
  http_post_load_data_fin(ctx, &data);
}

static void
do_htreq_post_process_body_load_chunks_generic(grn_ctx *ctx,
                                               const char *chunk,
                                               size_t chunk_size,
                                               void *user_data)
{
  http_post_load_data *data = user_data;
  GRN_TEXT_PUT(ctx, data->chunk_buffer_current, chunk, chunk_size);
  http_post_load_data_flush(ctx, data);
}

static void
do_htreq_post_process_body_load_generic(grn_ctx *ctx,
                                        h_header *header,
                                        grn_sock fd)
{
  http_post_load_data data;
  http_post_load_data_init(ctx, &data);
  do_htreq_post_process_body(ctx,
                             header,
                             fd,
                             do_htreq_post_process_body_load_chunks_generic,
                             &data);
  http_post_load_data_fin(ctx, &data);
}

static void
do_htreq_post(grn_ctx *ctx, ht_context *hc)
{
  grn_msg *msg = hc->msg;
  grn_sock fd = msg->u.fd;
  h_header header;

  header.method_start = NULL;
  header.method_length = -1;
  header.path_start = NULL;
  header.path_length = -1;
  header.content_type = HTTP_CONTENT_TYPE_UNKNOWN;
  header.content_type_raw = NULL;
  header.content_type_raw_length = -1;
  header.content_length = -1;
  header.body_start = NULL;
  header.body_end = GRN_BULK_CURR((grn_obj *)msg);
  header.have_100_continue = false;
  header.is_chunked_request = false;

  if (!h_parse_header(ctx,
                      GRN_BULK_HEAD((grn_obj *)msg),
                      header.body_end,
                      &header)) {
    return;
  }
  if (!STRING_EQUAL(header.method_start, header.method_length, "POST")) {
    return;
  }

  if (header.content_type == HTTP_CONTENT_TYPE_UNKNOWN) {
    const char *specified_content_type = header.content_type_raw;
    int specified_content_type_length = header.content_type_raw_length;
    if (!specified_content_type) {
      specified_content_type = "";
      specified_content_type_length = 0;
    }
    ERR(GRN_INVALID_ARGUMENT,
        "[http][post] Content-Type must be one of "
        "application/x-www-form-urlencoded, "
        "application/json or "
        "application/vnd.apache.arrow.stream "
        "(application/x-apache-arrow-streaming): "
        "<%.*s>",
        specified_content_type_length,
        specified_content_type);
    ht_context context;
    context.msg = msg;
    context.in_body = GRN_FALSE;
    context.is_chunked = GRN_FALSE;
    grn_ctx_set_output_type(ctx, GRN_CONTENT_JSON);
    h_output(ctx, GRN_CTX_TAIL, &context);
    return;
  }

  if (header.have_100_continue) {
    const char *continue_message = "HTTP/1.1 100 Continue\r\n\r\n";
    ssize_t send_size;
    int send_flags = MSG_NOSIGNAL;
    send_size =
      send(fd, continue_message, strlen(continue_message), send_flags);
    if (send_size == -1) {
      SOERR("send");
      return;
    }
  }

  if (header.content_type == HTTP_CONTENT_TYPE_WWW_FORM_URLENCODED) {
    do_htreq_post_process_body_www_form_urlencoded(ctx, &header, fd);
  } else {
    grn_ctx_send(ctx, header.path_start, header.path_length, GRN_CTX_MORE);
    if (ctx->rc != GRN_SUCCESS) {
      ht_context context;
      context.msg = msg;
      context.in_body = GRN_FALSE;
      context.is_chunked = GRN_FALSE;
      h_output(ctx, GRN_CTX_TAIL, &context);
      return;
    }

    if (header.content_type == HTTP_CONTENT_TYPE_JSON) {
      do_htreq_post_process_body_load_json(ctx, &header, fd);
    } else {
      do_htreq_post_process_body_load_generic(ctx, &header, fd);
    }
  }
}

static void
do_htreq(grn_ctx *ctx, ht_context *hc)
{
  grn_msg *msg = hc->msg;
  grn_com_header *header = &msg->header;
  switch (header->qtype) {
  case 'G': /* GET */
  case 'H': /* HEAD */
    do_htreq_get(ctx, hc);
    break;
  case 'P': /* POST */
    do_htreq_post(ctx, hc);
    break;
  }
  ERRCLR(ctx);
  if (ctx->rc == GRN_CANCEL) {
    ctx->rc = GRN_SUCCESS;
  }
  /* if (ctx->rc != GRN_OPERATION_WOULD_BLOCK) {...} */
  grn_msg_close(ctx, (grn_obj *)msg);
  /* if not keep alive connection */
  grn_sock_close(msg->u.fd);
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

static grn_obj *cache_table = NULL;
static grn_obj *cache_value = NULL;
static grn_obj *cache_flags = NULL;
static grn_obj *cache_expire = NULL;
static grn_obj *cache_cas = NULL;

#define CTX_GET(name) (grn_ctx_get(ctx, (name), strlen(name)))

static grn_bool
memcached_setup_flags_column(grn_ctx *ctx, const char *name)
{
  cache_flags = grn_obj_column(ctx, cache_table, name, strlen(name));
  if (cache_flags) {
    return GRN_TRUE;
  }

  cache_flags = grn_column_create(ctx,
                                  cache_table,
                                  name,
                                  strlen(name),
                                  NULL,
                                  GRN_OBJ_COLUMN_SCALAR | GRN_OBJ_PERSISTENT,
                                  grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!cache_flags) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_bool
memcached_setup_expire_column(grn_ctx *ctx, const char *name)
{
  cache_expire = grn_obj_column(ctx, cache_table, name, strlen(name));
  if (cache_expire) {
    return GRN_TRUE;
  }

  cache_expire = grn_column_create(ctx,
                                   cache_table,
                                   name,
                                   strlen(name),
                                   NULL,
                                   GRN_OBJ_COLUMN_SCALAR | GRN_OBJ_PERSISTENT,
                                   grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!cache_expire) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_bool
memcached_setup_cas_column(grn_ctx *ctx, const char *name)
{
  cache_cas = grn_obj_column(ctx, cache_table, name, strlen(name));
  if (cache_cas) {
    return GRN_TRUE;
  }

  cache_cas = grn_column_create(ctx,
                                cache_table,
                                name,
                                strlen(name),
                                NULL,
                                GRN_OBJ_COLUMN_SCALAR | GRN_OBJ_PERSISTENT,
                                grn_ctx_at(ctx, GRN_DB_UINT64));
  if (!cache_cas) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static grn_bool
memcached_init(grn_ctx *ctx)
{
  if (memcached_column_name) {
    cache_value = CTX_GET(memcached_column_name);
    if (!cache_value) {
      ERR(GRN_INVALID_ARGUMENT,
          "memcached column doesn't exist: <%s>",
          memcached_column_name);
      return GRN_FALSE;
    }
    if (!(grn_obj_is_column(ctx, cache_value) &&
          ((cache_value->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) ==
           GRN_OBJ_COLUMN_SCALAR))) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, cache_value);
      ERR(GRN_INVALID_ARGUMENT,
          "memcached column must be scalar column: <%.*s>",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return GRN_FALSE;
    }
    if (!(GRN_DB_SHORT_TEXT <= grn_obj_get_range(ctx, cache_value) &&
          grn_obj_get_range(ctx, cache_value) <= GRN_DB_LONG_TEXT)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, cache_value);
      ERR(GRN_INVALID_ARGUMENT,
          "memcached column must be text column: <%.*s>",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return GRN_FALSE;
    }

    cache_table = grn_ctx_at(ctx, cache_value->header.domain);
    if (cache_table->header.type == GRN_TABLE_NO_KEY) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, cache_table);
      ERR(
        GRN_INVALID_ARGUMENT,
        "memcached column's table must be HASH_KEY, PAT_KEY or DAT_KEY table: "
        "<%.*s>",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return GRN_FALSE;
    }

    {
      char column_name[GRN_TABLE_MAX_KEY_SIZE];
      char value_column_name[GRN_TABLE_MAX_KEY_SIZE];
      int value_column_name_size;

      value_column_name_size = grn_column_name(ctx,
                                               cache_value,
                                               value_column_name,
                                               GRN_TABLE_MAX_KEY_SIZE);
      grn_snprintf(column_name,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%.*s_memcached_flags",
                   value_column_name_size,
                   value_column_name);
      if (!memcached_setup_flags_column(ctx, column_name)) {
        return GRN_FALSE;
      }
      grn_snprintf(column_name,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%.*s_memcached_expire",
                   value_column_name_size,
                   value_column_name);
      if (!memcached_setup_expire_column(ctx, column_name)) {
        return GRN_FALSE;
      }
      grn_snprintf(column_name,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%.*s_memcached_cas",
                   value_column_name_size,
                   value_column_name);
      if (!memcached_setup_cas_column(ctx, column_name)) {
        return GRN_FALSE;
      }
    }
  } else {
    const char *table_name = "Memcache";
    const char *value_column_name = "value";

    cache_table = CTX_GET(table_name);
    if (!cache_table) {
      cache_table = grn_table_create(ctx,
                                     table_name,
                                     strlen(table_name),
                                     NULL,
                                     GRN_OBJ_TABLE_PAT_KEY | GRN_OBJ_PERSISTENT,
                                     grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                     NULL);
      if (!cache_table) {
        return GRN_FALSE;
      }
    }

    cache_value = grn_obj_column(ctx,
                                 cache_table,
                                 value_column_name,
                                 strlen(value_column_name));
    if (!cache_value) {
      cache_value =
        grn_column_create(ctx,
                          cache_table,
                          value_column_name,
                          strlen(value_column_name),
                          NULL,
                          GRN_OBJ_COLUMN_SCALAR | GRN_OBJ_PERSISTENT,
                          grn_ctx_at(ctx, GRN_DB_SHORT_TEXT));
      if (!cache_value) {
        return GRN_FALSE;
      }
    }

    if (!memcached_setup_flags_column(ctx, "flags")) {
      return GRN_FALSE;
    }
    if (!memcached_setup_expire_column(ctx, "expire")) {
      return GRN_FALSE;
    }
    if (!memcached_setup_cas_column(ctx, "cas")) {
      return GRN_FALSE;
    }
  }

  return GRN_TRUE;
}

#define RELATIVE_TIME_THRESH 1000000000

#define MBRES(ctx, re, status, key_len, extra_len, flags)                      \
  do {                                                                         \
    grn_msg_set_property((ctx), (re), (status), (key_len), (extra_len));       \
    grn_msg_send((ctx), (re), (flags));                                        \
  } while (0)

#define GRN_MSG_MBRES(block)                                                   \
  do {                                                                         \
    if (!quiet) {                                                              \
      grn_obj *re =                                                            \
        grn_msg_open_for_reply(ctx, (grn_obj *)msg, &edge->send_old);          \
      ((grn_msg *)re)->header.qtype = header->qtype;                           \
      block                                                                    \
    }                                                                          \
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
  case MBCMD_GETQ:
    flags = GRN_CTX_MORE;
    /* fallthru */
  case MBCMD_GET:
    {
      grn_id rid;
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      rid = grn_table_get(ctx, cache_table, key, keylen);
      if (!rid) {
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
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
          GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
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
  case MBCMD_SETQ:
  case MBCMD_ADDQ:
  case MBCMD_REPLACEQ:
    quiet = 1;
    /* fallthru */
  case MBCMD_SET:
  case MBCMD_ADD:
  case MBCMD_REPLACE:
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
      int f =
        (header->qtype == MBCMD_REPLACE || header->qtype == MBCMD_REPLACEQ)
          ? 0
          : GRN_TABLE_ADD;
      GRN_ASSERT(extralen == 8);
      if (header->qtype == MBCMD_REPLACE || header->qtype == MBCMD_REPLACEQ) {
        rid = grn_table_get(ctx, cache_table, key, keylen);
      } else {
        rid = grn_table_add(ctx, cache_table, key, keylen, &added);
      }
      if (!rid) {
        GRN_MSG_MBRES({
          MBRES(ctx,
                re,
                (f & GRN_TABLE_ADD) ? MBRES_ENOMEM : MBRES_NOT_STORED,
                0,
                0,
                0);
        });
      } else {
        if (added) {
          if (header->cas) {
            GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_EINVAL, 0, 0, 0); });
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
                GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0); });
                break;
              }
            } else if (header->qtype == MBCMD_ADD ||
                       header->qtype == MBCMD_ADDQ) {
              GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0); });
              break;
            }
          }
          {
            if (header->cas) {
              grn_obj cas_buf;
              GRN_UINT64_INIT(&cas_buf, 0);
              grn_obj_get_value(ctx, cache_cas, rid, &cas_buf);
              if (header->cas != GRN_UINT64_VALUE(&cas_buf)) {
                GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_NOT_STORED, 0, 0, 0); });
              }
            }
            {
              grn_obj text_buf, uint32_buf;
              GRN_TEXT_INIT(&text_buf, GRN_OBJ_DO_SHALLOW_COPY);
              GRN_TEXT_SET_REF(&text_buf, value, valuelen);
              grn_obj_set_value(ctx, cache_value, rid, &text_buf, GRN_OBJ_SET);
              GRN_UINT32_INIT(&uint32_buf, 0);
              GRN_UINT32_SET(ctx, &uint32_buf, flags);
              grn_obj_set_value(ctx,
                                cache_flags,
                                rid,
                                &uint32_buf,
                                GRN_OBJ_SET);
              if (expire && expire < RELATIVE_TIME_THRESH) {
                grn_timeval tv;
                grn_timeval_now(ctx, &tv);
                expire += tv.tv_sec;
              }
              GRN_UINT32_SET(ctx, &uint32_buf, expire);
              grn_obj_set_value(ctx,
                                cache_expire,
                                rid,
                                &uint32_buf,
                                GRN_OBJ_SET);
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
  case MBCMD_DELETEQ:
    quiet = 1;
    /* fallthru */
  case MBCMD_DELETE:
    {
      grn_id rid;
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      rid = grn_table_get(ctx, cache_table, key, keylen);
      if (!rid) {
        /* GRN_LOG(ctx, GRN_LOG_NOTICE, "GET k=%d not found", keylen); */
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
      } else {
        grn_table_delete_by_id(ctx, cache_table, rid);
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_SUCCESS, 0, 4, 0); });
      }
    }
    break;
  case MBCMD_INCREMENTQ:
  case MBCMD_DECREMENTQ:
    quiet = 1;
    /* fallthru */
  case MBCMD_INCREMENT:
  case MBCMD_DECREMENT:
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
      if (expire == 0xffffffff) {
        rid = grn_table_get(ctx, cache_table, key, keylen);
      } else {
        rid = grn_table_add(ctx, cache_table, key, keylen, &added);
      }
      if (!rid) {
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
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
              GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
              break;
            } else {
              GRN_TEXT_SET_REF(&text_buf, &init, 8);
              grn_obj_set_value(ctx, cache_value, rid, &text_buf, GRN_OBJ_SET);
              GRN_UINT32_SET(ctx, &uint32_buf, 0);
              grn_obj_set_value(ctx,
                                cache_flags,
                                rid,
                                &uint32_buf,
                                GRN_OBJ_SET);
            }
          } else {
            grn_obj uint64_buf;
            GRN_UINT64_INIT(&uint64_buf, 0);
            GRN_UINT64_SET(ctx, &uint64_buf, delta);
            grn_obj_set_value(ctx,
                              cache_value,
                              rid,
                              &uint64_buf,
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
  case MBCMD_FLUSHQ:
    quiet = 1;
    /* fallthru */
  case MBCMD_FLUSH:
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
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_SUCCESS, 0, 4, 0); });
        grn_obj_close(ctx, &exp_buf);
      }
    }
    break;
  case MBCMD_NOOP:
    break;
  case MBCMD_VERSION:
    GRN_MSG_MBRES({
      grn_bulk_write(ctx, re, PACKAGE_VERSION, strlen(PACKAGE_VERSION));
      MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0);
    });
    break;
  case MBCMD_GETKQ:
    flags = GRN_CTX_MORE;
    /* fallthru */
  case MBCMD_GETK:
    {
      grn_id rid;
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      rid = grn_table_get(ctx, cache_table, key, keylen);
      if (!rid) {
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
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
          GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_KEY_ENOENT, 0, 0, 0); });
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
  case MBCMD_APPENDQ:
  case MBCMD_PREPENDQ:
    quiet = 1;
    /* fallthru */
  case MBCMD_APPEND:
  case MBCMD_PREPEND:
    {
      grn_id rid;
      uint32_t size = ntohl(header->size);
      uint16_t keylen = ntohs(header->keylen);
      char *key = GRN_BULK_HEAD((grn_obj *)msg);
      char *value = key + keylen;
      uint32_t valuelen = size - keylen;
      rid = grn_table_add(ctx, cache_table, key, keylen, NULL);
      if (!rid) {
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_ENOMEM, 0, 0, 0); });
      } else {
        /* FIXME: check expire */
        grn_obj buf;
        int flags =
          header->qtype == MBCMD_APPEND ? GRN_OBJ_APPEND : GRN_OBJ_PREPEND;
        GRN_TEXT_INIT(&buf, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET_REF(&buf, value, valuelen);
        grn_obj_set_value(ctx, cache_value, rid, &buf, flags);
        GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0); });
      }
    }
    break;
  case MBCMD_STAT:
    {
      pid_t pid = grn_getpid();
      GRN_MSG_MBRES({
        grn_bulk_write(ctx, re, "pid", 3);
        grn_text_itoa(ctx, re, pid);
        MBRES(ctx, re, MBRES_SUCCESS, 3, 0, 0);
      });
    }
    break;
  case MBCMD_QUITQ:
    quiet = 1;
    /* fallthru */
  case MBCMD_QUIT:
    GRN_MSG_MBRES({ MBRES(ctx, re, MBRES_SUCCESS, 0, 0, 0); });
    /* fallthru */
  default:
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

static void
check_rlimit_nofile(grn_ctx *ctx)
{
#ifndef WIN32
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
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "RLIMIT_NOFILE(%" GRN_FMT_LLD ",%" GRN_FMT_LLD ")",
          (long long int)limit.rlim_cur,
          (long long int)limit.rlim_max);
#endif /* WIN32 */
}

static grn_thread_func_result CALLBACK
h_worker(void *arg)
{
  ht_context hc;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  grn_ctx_set_wal_role(ctx, worker_wal_role);
  grn_ctx_use(ctx, (grn_obj *)arg);
  grn_ctx_recv_handler_set(ctx, h_output, &hc);
  CRITICAL_SECTION_ENTER(q_critical_section);
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "thread start (%u/%u)",
          n_floating_threads,
          n_running_threads);
  while (n_running_threads <= max_n_floating_threads &&
         grn_gctx.stat != GRN_CTX_QUIT) {
    grn_obj *msg;
    if (ctx->rc == GRN_CANCEL) {
      ctx->rc = GRN_SUCCESS;
    }
    n_floating_threads++;
    while (!(msg = (grn_obj *)grn_com_queue_deque(&grn_gctx, &ctx_new))) {
      COND_WAIT(q_cond, q_critical_section);
      if (grn_gctx.stat == GRN_CTX_QUIT) {
        n_floating_threads--;
        goto exit;
      }
      if (n_running_threads > max_n_floating_threads) {
        n_floating_threads--;
        goto exit;
      }
    }
    n_floating_threads--;
    CRITICAL_SECTION_LEAVE(q_critical_section);
    hc.msg = (grn_msg *)msg;
    hc.in_body = GRN_FALSE;
    hc.is_chunked = GRN_FALSE;
    do_htreq(ctx, &hc);
    CRITICAL_SECTION_ENTER(q_critical_section);
  }
exit:
  n_running_threads--;
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "thread end (%u/%u)",
          n_floating_threads,
          n_running_threads);
  if (grn_gctx.stat == GRN_CTX_QUIT && running_event_loop) {
    break_accept_event_loop(ctx);
  }
  grn_ctx_fin(ctx);
  CRITICAL_SECTION_LEAVE(q_critical_section);
  return GRN_THREAD_FUNC_RETURN_VALUE;
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
    CRITICAL_SECTION_ENTER(q_critical_section);
    const char *immediate_path = NULL;
    int immediate_path_length = -1;
    if (n_floating_threads == 0 &&
        n_running_threads == max_n_floating_threads) {
      h_header header;
      header.method_start = NULL;
      header.method_length = -1;
      header.path_start = NULL;
      header.path_length = -1;
      if (h_parse_header_request_line(ctx,
                                      GRN_BULK_HEAD((grn_obj *)msg),
                                      GRN_BULK_CURR((grn_obj *)msg),
                                      &header) &&
          STRING_EQUAL(header.path_start,
                       header.path_length,
                       "/d/shutdown?mode=immediate")) {
        immediate_path = header.path_start;
        immediate_path_length = header.path_length;
      }
    }
    if (immediate_path) {
      ht_context hc;
      grn_ctx ctx_shutdown;
      grn_ctx_init(&ctx_shutdown, 0);
      grn_ctx_use(&ctx_shutdown, (grn_obj *)arg);
      grn_ctx_recv_handler_set(&ctx_shutdown, h_output, &hc);
      hc.msg = (grn_msg *)msg;
      hc.in_body = GRN_FALSE;
      hc.is_chunked = GRN_FALSE;
      grn_ctx_send(&ctx_shutdown,
                   immediate_path,
                   immediate_path_length,
                   GRN_CTX_TAIL);
      grn_ctx_fin(&ctx_shutdown);
    } else {
      grn_com_queue_enque(ctx, &ctx_new, (grn_com_queue_entry *)msg);
      if (n_floating_threads == 0 &&
          n_running_threads < max_n_floating_threads) {
        grn_thread thread;
        n_running_threads++;
        if (THREAD_CREATE(thread, h_worker, arg)) {
          n_running_threads--;
          SERR("pthread_create");
        }
      }
      COND_SIGNAL(q_cond);
    }
    CRITICAL_SECTION_LEAVE(q_critical_section);
  }
}

static int
h_server(char *path)
{
  int exit_code = EXIT_FAILURE;
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  grn_ctx_set_wal_role(ctx, wal_role);
  GRN_COM_QUEUE_INIT(&ctx_new);
  GRN_COM_QUEUE_INIT(&ctx_old);
  check_rlimit_nofile(ctx);
  GRN_TEXT_INIT(&http_response_server_line, 0);
  grn_text_printf(ctx,
                  &http_response_server_line,
                  "Server: %s/%s\r\n",
                  grn_get_package_label(),
                  grn_get_version());
  exit_code = start_service(ctx, path, NULL, h_handler);
  GRN_OBJ_FIN(ctx, &http_response_server_line);
  grn_ctx_fin(ctx);
  return exit_code;
}

static grn_thread_func_result CALLBACK
g_worker(void *arg)
{
  CRITICAL_SECTION_ENTER(q_critical_section);
  GRN_LOG(&grn_gctx,
          GRN_LOG_NOTICE,
          "thread start (%u/%u)",
          n_floating_threads,
          n_running_threads);
  while (n_running_threads <= max_n_floating_threads &&
         grn_gctx.stat != GRN_CTX_QUIT) {
    grn_ctx *ctx;
    grn_edge *edge;
    n_floating_threads++;
    while (!(edge = (grn_edge *)grn_com_queue_deque(&grn_gctx, &ctx_new))) {
      COND_WAIT(q_cond, q_critical_section);
      if (grn_gctx.stat == GRN_CTX_QUIT) {
        n_floating_threads--;
        goto exit;
      }
      if (n_running_threads > max_n_floating_threads) {
        n_floating_threads--;
        goto exit;
      }
    }
    ctx = &edge->ctx;
    n_floating_threads--;
    if (edge->stat == EDGE_DOING) {
      continue;
    }
    if (edge->stat == EDGE_WAIT) {
      edge->stat = EDGE_DOING;
      while (!GRN_COM_QUEUE_EMPTYP(&edge->recv_new)) {
        grn_obj *msg;
        CRITICAL_SECTION_LEAVE(q_critical_section);
        /* if (edge->flags == GRN_EDGE_WORKER) */
        while (
          ctx->stat != GRN_CTX_QUIT &&
          (edge->msg = (grn_msg *)grn_com_queue_deque(ctx, &edge->recv_new))) {
          grn_com_header *header = &edge->msg->header;
          msg = (grn_obj *)edge->msg;
          switch (header->proto) {
          case GRN_COM_PROTO_MBREQ:
            do_mbreq(ctx, edge);
            break;
          case GRN_COM_PROTO_GQTP:
            grn_ctx_send(ctx,
                         GRN_BULK_HEAD(msg),
                         GRN_BULK_VSIZE(msg),
                         header->flags);
            ERRCLR(ctx);
            if (ctx->rc == GRN_CANCEL) {
              ctx->rc = GRN_SUCCESS;
            }
            break;
          default:
            ctx->stat = GRN_CTX_QUIT;
            break;
          }
          grn_msg_close(ctx, msg);
        }
        while ((msg = (grn_obj *)grn_com_queue_deque(ctx, &edge->send_old))) {
          grn_msg_close(ctx, msg);
        }
        CRITICAL_SECTION_ENTER(q_critical_section);
        if (ctx->stat == GRN_CTX_QUIT || edge->stat == EDGE_ABORT) {
          break;
        }
      }
    }
    if (ctx->stat == GRN_CTX_QUIT || edge->stat == EDGE_ABORT) {
      grn_com_queue_enque(&grn_gctx, &ctx_old, (grn_com_queue_entry *)edge);
      edge->stat = EDGE_ABORT;
    } else {
      edge->stat = EDGE_IDLE;
    }
  };
exit:
  n_running_threads--;
  GRN_LOG(&grn_gctx,
          GRN_LOG_NOTICE,
          "thread end (%u/%u)",
          n_floating_threads,
          n_running_threads);
  CRITICAL_SECTION_LEAVE(q_critical_section);
  return GRN_THREAD_FUNC_RETURN_VALUE;
}

static void
g_dispatcher(grn_ctx *ctx, grn_edge *edge)
{
  CRITICAL_SECTION_ENTER(q_critical_section);
  if (edge->stat == EDGE_IDLE) {
    grn_com_queue_enque(ctx, &ctx_new, (grn_com_queue_entry *)edge);
    edge->stat = EDGE_WAIT;
    if (n_floating_threads == 0 && n_running_threads < max_n_floating_threads) {
      grn_thread thread;
      n_running_threads++;
      if (THREAD_CREATE(thread, g_worker, NULL)) {
        n_running_threads--;
        SERR("pthread_create");
      }
    }
    COND_SIGNAL(q_cond);
  }
  CRITICAL_SECTION_LEAVE(q_critical_section);
}

static void
g_output(grn_ctx *ctx, int flags, void *arg)
{
  grn_edge *edge = arg;
  grn_com *com = edge->com;
  grn_msg *req = edge->msg, *msg = (grn_msg *)ctx->impl->output.buf;
  msg->edge_id = req->edge_id;
  msg->header.proto = req->header.proto == GRN_COM_PROTO_MBREQ
                        ? GRN_COM_PROTO_MBRES
                        : req->header.proto;
  if (ctx->rc != GRN_SUCCESS && GRN_BULK_VSIZE(ctx->impl->output.buf) == 0) {
    GRN_TEXT_PUTS(ctx, ctx->impl->output.buf, ctx->errbuf);
  }
  if (grn_msg_send(ctx,
                   (grn_obj *)msg,
                   (flags & GRN_CTX_MORE) ? GRN_CTX_MORE : GRN_CTX_TAIL)) {
    edge->stat = EDGE_ABORT;
  }
  ctx->impl->output.buf = grn_msg_open(ctx, com, &edge->send_old);
}

static void
g_handler(grn_ctx *ctx, grn_obj *msg)
{
  grn_edge *edge;
  grn_com *com = ((grn_msg *)msg)->u.peer;
  if (ctx->rc) {
    if (com->has_sid) {
      if ((edge = com->opaque)) {
        CRITICAL_SECTION_ENTER(q_critical_section);
        if (edge->stat == EDGE_IDLE) {
          grn_com_queue_enque(ctx, &ctx_old, (grn_com_queue_entry *)edge);
        }
        edge->stat = EDGE_ABORT;
        CRITICAL_SECTION_LEAVE(q_critical_section);
      } else {
        grn_com_close(ctx, com);
      }
    }
    grn_msg_close(ctx, msg);
  } else {
    int added;
    edge = grn_edges_add(ctx, &((grn_msg *)msg)->edge_id, &added);
    if (added) {
      grn_ctx_init(&edge->ctx, 0);
      grn_ctx_set_wal_role(ctx, worker_wal_role);
      GRN_COM_QUEUE_INIT(&edge->recv_new);
      GRN_COM_QUEUE_INIT(&edge->send_old);
      grn_ctx_use(&edge->ctx, (grn_obj *)com->ev->opaque);
      grn_ctx_recv_handler_set(&edge->ctx, g_output, edge);
      com->opaque = edge;
      grn_obj_close(&edge->ctx, edge->ctx.impl->output.buf);
      edge->ctx.impl->output.buf =
        grn_msg_open(&edge->ctx, com, &edge->send_old);
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
  grn_ctx ctx_, *ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  grn_ctx_set_wal_role(ctx, wal_role);
  GRN_COM_QUEUE_INIT(&ctx_new);
  GRN_COM_QUEUE_INIT(&ctx_old);
  check_rlimit_nofile(ctx);
  exit_code = start_service(ctx, path, g_dispatcher, g_handler);
  grn_ctx_fin(ctx);
  return exit_code;
}

enum {
  ACTION_USAGE = 1,
  ACTION_VERSION,
  ACTION_SHOW_CONFIG,
  ACTION_ERROR
};

#define ACTION_MASK                (0x0f)
#define MODE_MASK                  (0xf0)
#define FLAG_MODE_ALONE            (1 << 4)
#define FLAG_MODE_CLIENT           (1 << 5)
#define FLAG_MODE_DAEMON           (1 << 6)
#define FLAG_MODE_SERVER           (1 << 7)
#define FLAG_NEW_DB                (1 << 8)
#define FLAG_USE_WINDOWS_EVENT_LOG (1 << 9)

static uint32_t
get_core_number(void)
{
#ifdef WIN32
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  return sinfo.dwNumberOfProcessors;
#else
#  ifdef _SC_NPROCESSORS_CONF
  return sysconf(_SC_NPROCESSORS_CONF);
#  else
  return 1;
#  endif
#endif
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
#define CONFIG_FILE_BUF_SIZE         4096
#define CONFIG_FILE_MAX_NAME_LENGTH  128
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
config_file_clear(void)
{
  while (config_file_entry_head) {
    config_file_entry *next = config_file_entry_head->next;
    free(config_file_entry_head);
    config_file_entry_head = next;
  }
}

static config_file_status
config_file_register(const char *path,
                     const grn_str_getopt_opt *opts,
                     int *flags,
                     const char *name,
                     size_t name_length,
                     const char *value,
                     size_t value_length)
{
  char name_buf[CONFIG_FILE_MAX_NAME_LENGTH + 3];
  config_file_entry *entry = NULL;
  char *args[4];

  name_buf[0] = name_buf[1] = '-';
  grn_strcpy(name_buf + 2, CONFIG_FILE_MAX_NAME_LENGTH + 1, name);

  if (value) {
    const size_t entry_size = sizeof(config_file_entry) + value_length + 1;
    entry = (config_file_entry *)malloc(entry_size);
    if (!entry) {
      fprintf(stderr,
              "memory allocation failed: %u bytes\n",
              (unsigned int)entry_size);
      return CONFIG_FILE_MALLOC_ERROR;
    }
    grn_strcpy((char *)(entry + 1), value_length + 1, value);
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
config_file_parse(const char *path,
                  const grn_str_getopt_opt *opts,
                  int *flags,
                  char *buf)
{
  char *ptr, *name, *value;
  size_t name_length, value_length;

  while (isspace((unsigned char)*buf)) {
    buf++;
  }

  ptr = buf;
  while (*ptr && *ptr != '#' && *ptr != ';') {
    ptr++;
  }

  do {
    *ptr-- = '\0';
  } while (ptr >= buf && isspace((unsigned char)*ptr));

  if (!*buf) {
    return CONFIG_FILE_SUCCESS;
  }

  name = ptr = buf;
  while (*ptr && !isspace((unsigned char)*ptr) && *ptr != '=') {
    ptr++;
  }
  while (isspace((unsigned char)*ptr)) {
    *ptr++ = '\0';
  }

  name_length = strlen(name);
  if (name_length == 0) {
    return CONFIG_FILE_SUCCESS;
  } else if (name_length > CONFIG_FILE_MAX_NAME_LENGTH) {
    fprintf(stderr,
            "too long name in config file: %u bytes\n",
            (unsigned int)name_length);
    return CONFIG_FILE_FORMAT_ERROR;
  }

  if (*ptr == '=') {
    *ptr++ = '\0';
    while (isspace((unsigned char)*ptr)) {
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
    fprintf(stderr,
            "too long value in config file: %u bytes\n",
            (unsigned int)value_length);
    return CONFIG_FILE_FORMAT_ERROR;
  }

  return config_file_register(path,
                              opts,
                              flags,
                              name,
                              name_length,
                              value,
                              value_length);
}

static config_file_status
config_file_load(const char *path, const grn_str_getopt_opt *opts, int *flags)
{
  config_file_status status = CONFIG_FILE_SUCCESS;
  char buf[CONFIG_FILE_BUF_SIZE];
  size_t length = 0;
  FILE *const file = fopen(path, "rb");
  if (!file) {
    return CONFIG_FILE_FOPEN_ERROR;
  }

  for (;;) {
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

static const int default_http_port = DEFAULT_HTTP_PORT;
static const int default_gqtp_port = DEFAULT_GQTP_PORT;
static grn_encoding default_encoding = GRN_ENC_DEFAULT;
static uint32_t default_max_n_threads = DEFAULT_MAX_N_FLOATING_THREADS;
static const grn_log_level default_log_level = GRN_LOG_DEFAULT_LEVEL;
static const char *const default_protocol = "gqtp";
static const char *default_hostname = "localhost";
static const char *const default_dest = "localhost";
static const char *default_log_path = "";
static const char *default_query_log_path = "";
static const char *default_config_path = "";
static const char *default_document_root = "";
static grn_command_version default_default_command_version =
  GRN_COMMAND_VERSION_DEFAULT;
static int64_t default_default_match_escalation_threshold = 0;
static const char *const default_bind_address = "0.0.0.0";
static double default_default_request_timeout = 0.0;

static void
init_default_hostname(void)
{
  static char hostname[HOST_NAME_MAX + 1];
  struct addrinfo hints, *result;

  hostname[HOST_NAME_MAX] = '\0';
  if (gethostname(hostname, HOST_NAME_MAX) == -1) return;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL;
  if (getaddrinfo(hostname, NULL, &hints, &result) != 0) return;
  freeaddrinfo(result);

  default_hostname = hostname;
}

static void
init_default_settings(void)
{
  output = stdout;

  default_encoding = grn_encoding_parse(GRN_DEFAULT_ENCODING);

  {
    const uint32_t n_cores = get_core_number();
    if (n_cores != 0) {
      default_max_n_threads = n_cores;
    }
  }

  init_default_hostname();

  default_log_path = grn_default_logger_get_path();
  default_query_log_path = grn_default_query_logger_get_path();

  default_config_path = getenv("GRN_CONFIG_PATH");
  if (!default_config_path) {
    default_config_path = GRN_CONFIG_PATH;
    if (!default_config_path) {
      default_config_path = "";
    }
  }

#ifdef WIN32
  {
    static char windows_default_document_root[PATH_MAX];
    size_t document_root_length = strlen(grn_windows_base_dir()) + 1 +
                                  strlen(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT) +
                                  1;
    if (document_root_length >= PATH_MAX) {
      fprintf(stderr, "can't use default root: too long path\n");
    } else {
      grn_strcpy(windows_default_document_root,
                 PATH_MAX,
                 grn_windows_base_dir());
      grn_strcat(windows_default_document_root, PATH_MAX, "/");
      grn_strcat(windows_default_document_root,
                 PATH_MAX,
                 GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT);
      default_document_root = windows_default_document_root;
    }
  }
#else
  default_document_root = GRN_DEFAULT_DOCUMENT_ROOT;
#endif

  default_default_command_version = grn_get_default_command_version();
  default_default_match_escalation_threshold =
    grn_get_default_match_escalation_threshold();
  default_default_request_timeout = grn_get_default_request_timeout();
}

static void
show_config(FILE *out, const grn_str_getopt_opt *opts, int flags)
{
  const grn_str_getopt_opt *o;

  for (o = opts; o->opt || o->longopt; o++) {
    switch (o->op) {
    case GETOPT_OP_NONE:
      if (o->arg && *o->arg) {
        if (o->longopt && strcmp(o->longopt, "config-path")) {
          fprintf(out, "%s=%s\n", o->longopt, *o->arg);
        }
      }
      break;
    case GETOPT_OP_ON:
      if (flags & o->flag) {
        goto no_arg;
      }
      break;
    case GETOPT_OP_OFF:
      if (!(flags & o->flag)) {
        goto no_arg;
      }
      break;
    case GETOPT_OP_UPDATE:
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
  printf("%s %s [", grn_get_package_label(), grn_get_version());

  /* FIXME: Should we detect host information dynamically on Windows? */
#ifdef HOST_OS
  printf("%s,", HOST_OS);
#endif
#ifdef HOST_CPU
  printf("%s,", HOST_CPU);
#endif
  printf("%s", GRN_DEFAULT_ENCODING);

  printf(",match-escalation-threshold=%" GRN_FMT_INT64D,
         grn_get_default_match_escalation_threshold());

#ifndef NO_NFKC
  printf(",nfkc");
#endif
#ifdef GRN_WITH_MECAB
  printf(",mecab");
#endif
#ifdef GRN_WITH_MESSAGE_PACK
  printf(",message-pack");
#endif
#ifdef GRN_WITH_MRUBY
  printf(",mruby");
#endif
#ifdef GRN_WITH_ONIGMO
  printf(",onigmo");
#endif
#ifdef GRN_WITH_ZLIB
  printf(",zlib");
#endif
#ifdef GRN_WITH_LZ4
  printf(",lz4");
#endif
#ifdef GRN_WITH_ZSTD
  printf(",zstandard");
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
#ifdef GRN_WITH_RAPIDJSON
  printf(",rapidjson");
#endif
#ifdef GRN_WITH_APACHE_ARROW
  printf(",apache-arrow");
#endif
#ifdef GRN_WITH_XXHASH
  printf(",xxhash");
#endif
#ifdef GRN_WITH_BLOSC
  printf(",blosc");
#endif
#ifdef GRN_HAVE_BFLOAT16
  printf(",bfloat16");
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
  uint32_t default_cache_limit = GRN_CACHE_DEFAULT_MAX_N_ENTRIES;

  fprintf(
    output,
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
    "      --input-fd <FD>:        read commands from specified file "
    "descriptor\n"
    "                              --file has a prioriry over --input-fd\n"
    "      --output-fd <FD>:       output response to specified file "
    "descriptor\n"
    "  -p, --port <port number>:   specify server port number (client mode "
    "only)\n"
    "                              (default: %d)\n"
    "\n"
    "Server/daemon options:\n"
    "      --bind-address <ip/hostname>:\n"
    "                                specify server address to bind\n"
    "                                (default: %s)\n"
    "  -p, --port <port number>:     specify server port number\n"
    "                                (HTTP default: %d, GQTP default: %d)\n"
    "  -i, --server-id <ip/hostname>:\n"
    "                                specify server ID address (default: %s)\n"
    "      --protocol <protocol>:    specify server protocol to listen\n"
    "                                [gqtp|http|memcached] (default: %s)\n"
    "      --document-root <path>:   specify document root path (http only)\n"
    "                                (default: %s)\n"
    "      --cache-limit <limit>:    specify max number of cache data "
    "(default: %u)\n"
    "  -t, --max-threads <max threads>:\n"
    "                                specify max number of threads (default: "
    "%u)\n"
    "      --pid-path <path>:        specify file to write process ID to\n"
    "      --default-request-timeout <timeout>:\n"
    "                                specify the default request timeout in "
    "seconds\n"
    "                                (default: %f)\n"
    "      --cache-base-path <path>: specify the cache base path\n"
    "                                You can make cache persistent by this "
    "option\n"
    "                                You must specify path on memory file "
    "system\n"
    "                                (default: none; disabled)\n"
    "      --listen-backlog <backlog>: specify the backlog for listen(2)\n"
    "                                (default: %d)\n"
    "\n"
    "Memcached options:\n"
    "      --memcached-column <column>:\n"
    "                                specify column to access by memcached "
    "protocol\n"
    "                                The column must be text type column and\n"
    "                                its table must be not NO_KEY table\n"
    "\n"
    "Logging options:\n"
    "  -l, --log-level <log level>:\n"
    "                           specify log level\n"
    "                           [none|emergency|alert|critical|\n"
    "                            error|warning|notice|info|debug|dump]\n"
    "                           (default: %s)\n"
    "      --log-flags <log flags>:\n"
    "                           specify log flags\n"
    "                           '+' prefix means that 'add the flag'\n"
    "                           '-' prefix means that 'remove the flag'\n"
    "                           No prefix means that 'replace existing flags'\n"
    "                           Multiple log flags can be specified by\n"
    "                           separating flags with '|'\n"
    "                           Example: default|+pid|-time\n"
    "                           [none|time|title|message|location|\n"
    "                            pid|process_id|thread_id|\n"
    "                            all|default]\n"
    "                           (default: %s)\n"
    "      --log-path <path>:   specify log path\n"
    "                           (default: %s)\n"
    "      --log-rotate-threshold-size <threshold>:\n"
    "                           specify threshold for log rotate\n"
    "                           Log file is rotated when\n"
    "                           log file size is larger than or\n"
    "                           equals to the threshold\n"
    "                           (default: 0; disabled)\n"
#ifdef WIN32
    "      --use-windows-event-log:\n"
    "                           report logs as Windows events\n"
#endif /* WIN32 */
    "      --query-log-path <path>:\n"
    "                           specify query log path\n"
    "                           (default: %s)\n"
    "      --query-log-rotate-threshold-size <threshold>:\n"
    "                           specify threshold for query log rotate\n"
    "                           Query log file is rotated when\n"
    "                           query log file size is larger than or\n"
    "                           equals to the threshold\n"
    "                           (default: 0; disabled)\n"
    "\n"
    "Common options:\n"
    "      --working-directory <path>:\n"
    "                       specify working directory path\n"
    "                       (none)\n"
    "      --config-path <path>:\n"
    "                       specify config file path\n"
    "                       (default: %s)\n"
    "      --default-command-version <version>:\n"
    "                       specify default command version (default: %d)\n"
    "      --default-match-escalation-threshold <threshold>:\n"
    "                       specify default match escalation threshold"
    " (default: %" GRN_FMT_INT64D ")\n"
    "\n"
    "      --show-config:   show config\n"
    "  -h, --help:          show usage\n"
    "      --version:       show groonga version\n"
    "\n"
    "dest:\n"
    "  <db pathname> [<commands>]: in standalone mode\n"
    "  <db pathname>: in server/daemon mode\n"
    "  <dest hostname> [<commands>]: in client mode (default: %s)\n",
    grn_encoding_to_string(default_encoding),
    default_gqtp_port,
    default_bind_address,
    default_http_port,
    default_gqtp_port,
    default_hostname,
    default_protocol,
    default_document_root,
    default_cache_limit,
    default_max_n_threads,
    default_default_request_timeout,
    listen_backlog,
    grn_log_level_to_string(default_log_level),
    "time|message", /* TODO: Generate from GRN_LOG_DEFAULT */
    default_log_path,
    default_query_log_path,
    default_config_path,
    default_default_command_version,
    default_default_match_escalation_threshold,
    default_dest);
}

int
main(int argc, char **argv)
{
  const char *port_arg = NULL;
  const char *encoding_arg = NULL;
  const char *max_n_threads_arg = NULL;
  const char *log_level_arg = NULL;
  const char *bind_address_arg = NULL;
  const char *hostname_arg = NULL;
  const char *protocol_arg = NULL;
  const char *log_path_arg = GRN_LOG_PATH;
  const char *log_rotate_threshold_size_arg = NULL;
  const char *query_log_path_arg = NULL;
  const char *query_log_rotate_threshold_size_arg = NULL;
  const char *cache_limit_arg = NULL;
  const char *document_root_arg = NULL;
  const char *default_command_version_arg = NULL;
  const char *default_match_escalation_threshold_arg = NULL;
  const char *input_fd_arg = NULL;
  const char *output_fd_arg = NULL;
  const char *working_directory_arg = NULL;
  const char *config_path = NULL;
  const char *default_request_timeout_arg = NULL;
  const char *cache_base_path = NULL;
  const char *listen_backlog_arg = NULL;
  const char *log_flags_arg = NULL;
  const char *wal_role_arg = NULL;
  int exit_code = EXIT_SUCCESS;
  int i;
  int flags = 0;
  uint32_t cache_limit = 0;
  grn_command_version default_command_version;
  int64_t default_match_escalation_threshold = 0;
  double default_request_timeout = 0.0;
  grn_bool need_line_editor = GRN_FALSE;
  static grn_str_getopt_opt opts[] = {
    {'p', "port", NULL, 0, GETOPT_OP_NONE},
    {'e', "encoding", NULL, 0, GETOPT_OP_NONE},
    {'t', "max-threads", NULL, 0, GETOPT_OP_NONE},
    {'h', "help", NULL, ACTION_USAGE, GETOPT_OP_UPDATE},
    {'c', NULL, NULL, FLAG_MODE_CLIENT, GETOPT_OP_ON},
    {'d', NULL, NULL, FLAG_MODE_DAEMON, GETOPT_OP_ON},
    {'s', NULL, NULL, FLAG_MODE_SERVER, GETOPT_OP_ON},
    {'l', "log-level", NULL, 0, GETOPT_OP_NONE},
    {'i', "server-id", NULL, 0, GETOPT_OP_NONE},
    {'n', NULL, NULL, FLAG_NEW_DB, GETOPT_OP_ON},
    {'\0', "protocol", NULL, 0, GETOPT_OP_NONE},
    {'\0', "version", NULL, ACTION_VERSION, GETOPT_OP_UPDATE},
    {'\0', "log-path", NULL, 0, GETOPT_OP_NONE},
    {'\0', "log-rotate-threshold-size", NULL, 0, GETOPT_OP_NONE},
    {'\0', "query-log-path", NULL, 0, GETOPT_OP_NONE},
    {'\0', "query-log-rotate-threshold-size", NULL, 0, GETOPT_OP_NONE},
    {'\0', "pid-path", NULL, 0, GETOPT_OP_NONE},
    {'\0', "config-path", NULL, 0, GETOPT_OP_NONE},
    {'\0', "show-config", NULL, ACTION_SHOW_CONFIG, GETOPT_OP_UPDATE},
    {'\0', "cache-limit", NULL, 0, GETOPT_OP_NONE},
    {'\0', "file", NULL, 0, GETOPT_OP_NONE},
    {'\0', "document-root", NULL, 0, GETOPT_OP_NONE},
    {'\0', "default-command-version", NULL, 0, GETOPT_OP_NONE},
    {'\0', "default-match-escalation-threshold", NULL, 0, GETOPT_OP_NONE},
    {'\0', "bind-address", NULL, 0, GETOPT_OP_NONE},
    {'\0', "input-fd", NULL, 0, GETOPT_OP_NONE},
    {'\0', "output-fd", NULL, 0, GETOPT_OP_NONE},
    {'\0', "working-directory", NULL, 0, GETOPT_OP_NONE},
    {'\0',
     "use-windows-event-log",
     NULL,
     FLAG_USE_WINDOWS_EVENT_LOG,
     GETOPT_OP_ON},
    {'\0', "memcached-column", NULL, 0, GETOPT_OP_NONE},
    {'\0', "default-request-timeout", NULL, 0, GETOPT_OP_NONE},
    {'\0', "cache-base-path", NULL, 0, GETOPT_OP_NONE},
    {'\0', "listen-backlog", NULL, 0, GETOPT_OP_NONE},
    {'\0', "log-flags", NULL, 0, GETOPT_OP_NONE},
    {'\0', "wal-role", NULL, 0, GETOPT_OP_NONE},
    {'\0', NULL, NULL, 0, 0}};
  opts[0].arg = &port_arg;
  opts[1].arg = &encoding_arg;
  opts[2].arg = &max_n_threads_arg;
  opts[7].arg = &log_level_arg;
  opts[8].arg = &hostname_arg;
  opts[10].arg = &protocol_arg;
  opts[12].arg = &log_path_arg;
  opts[13].arg = &log_rotate_threshold_size_arg;
  opts[14].arg = &query_log_path_arg;
  opts[15].arg = &query_log_rotate_threshold_size_arg;
  opts[16].arg = &pid_file_path;
  opts[17].arg = &config_path;
  opts[19].arg = &cache_limit_arg;
  opts[20].arg = &input_path;
  opts[21].arg = &document_root_arg;
  opts[22].arg = &default_command_version_arg;
  opts[23].arg = &default_match_escalation_threshold_arg;
  opts[24].arg = &bind_address_arg;
  opts[25].arg = &input_fd_arg;
  opts[26].arg = &output_fd_arg;
  opts[27].arg = &working_directory_arg;
  opts[29].arg = &memcached_column_name;
  opts[30].arg = &default_request_timeout_arg;
  opts[31].arg = &cache_base_path;
  opts[32].arg = &listen_backlog_arg;
  opts[33].arg = &log_flags_arg;
  opts[34].arg = &wal_role_arg;

  reset_ready_notify_pipe();

  init_default_settings();

  /* only for parsing --config-path. */
  i = grn_str_getopt(argc, argv, opts, &flags);
  if (i < 0) {
    show_usage(stderr);
    return EXIT_FAILURE;
  }

  if (config_path) {
    const config_file_status status =
      config_file_load(config_path, opts, &flags);
    if (status == CONFIG_FILE_FOPEN_ERROR) {
      fprintf(stderr,
              "%s: can't open config file: %s (%s)\n",
              argv[0],
              config_path,
              strerror(errno));
      return EXIT_FAILURE;
    } else if (status != CONFIG_FILE_SUCCESS) {
      fprintf(stderr,
              "%s: failed to parse config file: %s (%s)\n",
              argv[0],
              config_path,
              (status == CONFIG_FILE_FORMAT_ERROR) ? "Invalid format"
                                                   : strerror(errno));
      return EXIT_FAILURE;
    }
  } else if (*default_config_path) {
    const config_file_status status =
      config_file_load(default_config_path, opts, &flags);
    if (status != CONFIG_FILE_SUCCESS && status != CONFIG_FILE_FOPEN_ERROR) {
      fprintf(stderr,
              "%s: failed to parse config file: %s (%s)\n",
              argv[0],
              default_config_path,
              (status == CONFIG_FILE_FORMAT_ERROR) ? "Invalid format"
                                                   : strerror(errno));
      return EXIT_FAILURE;
    }
  }

  if (working_directory_arg) {
    if (chdir(working_directory_arg) == -1) {
      fprintf(stderr,
              "%s: failed to change directory: %s: %s\n",
              argv[0],
              working_directory_arg,
              strerror(errno));
      return EXIT_FAILURE;
    }
  }

  if (cache_base_path) {
    grn_set_default_cache_base_path(cache_base_path);
  }

  /* ignore mode option in config file */
  flags = (flags == ACTION_ERROR) ? 0 : (flags & ~ACTION_MASK);

  i = grn_str_getopt(argc, argv, opts, &flags);
  if (i < 0) {
    flags = ACTION_ERROR;
  }
  switch (flags & ACTION_MASK) {
  case ACTION_VERSION:
    show_version();
    return EXIT_SUCCESS;
  case ACTION_USAGE:
    show_usage(output);
    return EXIT_SUCCESS;
  case ACTION_SHOW_CONFIG:
    show_config(output, opts, flags & ~ACTION_MASK);
    return EXIT_SUCCESS;
  case ACTION_ERROR:
    show_usage(stderr);
    return EXIT_FAILURE;
  }

  if ((flags & MODE_MASK) == 0) {
    flags |= FLAG_MODE_ALONE;
  }

  if (port_arg) {
    const char *const end = port_arg + strlen(port_arg);
    const char *rest = NULL;
    const int value = grn_atoi(port_arg, end, &rest);
    if (rest != end || value <= 0 || value > 65535) {
      fprintf(stderr, "invalid port number: <%s>\n", port_arg);
      return EXIT_FAILURE;
    }
    port = value;
  } else {
    if (protocol_arg) {
      if (*protocol_arg == 'h' || *protocol_arg == 'H') {
        port = default_http_port;
      }
    }
  }

  if (encoding_arg) {
    switch (*encoding_arg) {
    case 'n':
    case 'N':
      encoding = GRN_ENC_NONE;
      break;
    case 'e':
    case 'E':
      encoding = GRN_ENC_EUC_JP;
      break;
    case 'u':
    case 'U':
      encoding = GRN_ENC_UTF8;
      break;
    case 's':
    case 'S':
      encoding = GRN_ENC_SJIS;
      break;
    case 'l':
    case 'L':
      encoding = GRN_ENC_LATIN1;
      break;
    case 'k':
    case 'K':
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
    case 'g':
    case 'G':
      do_client = g_client;
      do_server = g_server;
      break;
    case 'h':
    case 'H':
      do_client = g_client;
      do_server = h_server;
      break;
    case 'm':
    case 'M':
      is_memcached_mode = GRN_TRUE;
      do_client = g_client;
      do_server = g_server;
      break;
    default:
      do_client = g_client;
      do_server = g_server;
      break;
    }
  } else {
    do_client = g_client;
    do_server = g_server;
  }

#ifdef WIN32
  if (flags & FLAG_USE_WINDOWS_EVENT_LOG) {
    use_windows_event_log = GRN_TRUE;
  }
#endif /* WIN32 */

  if (use_windows_event_log) {
    grn_windows_event_logger_set(NULL, windows_event_source_name);
  }

  if (log_path_arg) {
    grn_default_logger_set_path(log_path_arg);
  }

  if (log_rotate_threshold_size_arg) {
    const char *const end =
      log_rotate_threshold_size_arg + strlen(log_rotate_threshold_size_arg);
    const char *rest = NULL;
    const uint64_t value =
      grn_atoull(log_rotate_threshold_size_arg, end, &rest);
    if (end != rest) {
      fprintf(stderr,
              "invalid log rotate threshold size: <%s>\n",
              log_rotate_threshold_size_arg);
      return EXIT_FAILURE;
    }
    grn_default_logger_set_rotate_threshold_size(value);
  }

  if (log_flags_arg) {
    int log_flags = GRN_LOG_DEFAULT;
    if (!grn_log_flags_parse(log_flags_arg, -1, &log_flags)) {
      fprintf(stderr, "invalid log flags: <%s>\n", log_flags_arg);
      return EXIT_FAILURE;
    }
    grn_default_logger_set_flags(log_flags);
  }

  if (query_log_path_arg) {
    grn_default_query_logger_set_path(query_log_path_arg);
  }

  if (query_log_rotate_threshold_size_arg) {
    const char *const end = query_log_rotate_threshold_size_arg +
                            strlen(query_log_rotate_threshold_size_arg);
    const char *rest = NULL;
    const uint64_t value =
      grn_atoull(query_log_rotate_threshold_size_arg, end, &rest);
    if (end != rest) {
      fprintf(stderr,
              "invalid query log rotate threshold size: <%s>\n",
              query_log_rotate_threshold_size_arg);
      return EXIT_FAILURE;
    }
    grn_default_query_logger_set_rotate_threshold_size(value);
  }

  {
    grn_log_level log_level;

    if (log_level_arg) {
      if (!grn_log_level_parse(log_level_arg, &log_level)) {
        const char *const end = log_level_arg + strlen(log_level_arg);
        const char *rest = NULL;
        const int value = grn_atoi(log_level_arg, end, &rest);
        if (end != rest || value < GRN_LOG_NONE || value > GRN_LOG_DUMP) {
          fprintf(stderr, "invalid log level: <%s>\n", log_level_arg);
          return EXIT_FAILURE;
        }
        log_level = value;
      }
    } else {
      log_level = default_log_level;
    }

    grn_default_logger_set_max_level(log_level);
  }

  if (max_n_threads_arg) {
    const char *const end = max_n_threads_arg + strlen(max_n_threads_arg);
    const char *rest = NULL;
    const uint32_t value = grn_atoui(max_n_threads_arg, end, &rest);
    if (end != rest || value < 1 || value > 100) {
      fprintf(stderr,
              "invalid max number of threads: <%s>\n",
              max_n_threads_arg);
      return EXIT_FAILURE;
    }
    max_n_floating_threads = value;
  } else {
    if (flags & FLAG_MODE_ALONE) {
      max_n_floating_threads = 1;
    } else {
      max_n_floating_threads = default_max_n_threads;
    }
  }

  grn_thread_set_get_limit_func(groonga_get_thread_limit, NULL);
  grn_thread_set_set_limit_func(groonga_set_thread_limit, NULL);
  grn_thread_set_set_limit_with_ctx_func(groonga_set_thread_limit_with_ctx,
                                         NULL);

  if (output_fd_arg) {
    const char *const end = output_fd_arg + strlen(output_fd_arg);
    const char *rest = NULL;
    const int output_fd = grn_atoi(output_fd_arg, end, &rest);
    if (rest != end || output_fd == 0) {
      fprintf(stderr, "invalid output FD: <%s>\n", output_fd_arg);
      return EXIT_FAILURE;
    }
    output = fdopen(output_fd, "w");
    if (!output) {
      fprintf(stderr,
              "can't open output FD: %d (%s)\n",
              output_fd,
              strerror(errno));
      return EXIT_FAILURE;
    }
  }

  if (bind_address_arg) {
    const size_t bind_address_length = strlen(bind_address_arg);
    if (bind_address_length > HOST_NAME_MAX) {
      fprintf(stderr,
              "too long bind address: %s (%u bytes):"
              " must not be longer than %u bytes\n",
              bind_address_arg,
              (unsigned int)bind_address_length,
              HOST_NAME_MAX);
      return EXIT_FAILURE;
    }
    grn_strcpy(bind_address, HOST_NAME_MAX + 1, bind_address_arg);
  } else {
    grn_strcpy(bind_address, HOST_NAME_MAX + 1, default_bind_address);
  }

  if (hostname_arg) {
    const size_t hostname_length = strlen(hostname_arg);
    if (hostname_length > HOST_NAME_MAX) {
      fprintf(stderr,
              "too long hostname: %s (%u bytes):"
              " must not be longer than %u bytes\n",
              hostname_arg,
              (unsigned int)hostname_length,
              HOST_NAME_MAX);
      return EXIT_FAILURE;
    }
    grn_strcpy(hostname, HOST_NAME_MAX + 1, hostname_arg);
  } else {
    grn_strcpy(hostname, HOST_NAME_MAX + 1, default_hostname);
  }

  if (document_root_arg) {
    grn_document_root = document_root_arg;
  }

  if (default_command_version_arg) {
    const char *const end =
      default_command_version_arg + strlen(default_command_version_arg);
    const char *rest = NULL;
    const int value = grn_atoi(default_command_version_arg, end, &rest);
    if (end != rest || value < GRN_COMMAND_VERSION_MIN ||
        value > GRN_COMMAND_VERSION_MAX) {
      fprintf(stderr,
              "invalid command version: <%s>\n",
              default_command_version_arg);
      return EXIT_FAILURE;
    }
    default_command_version = value;
  } else {
    default_command_version = default_default_command_version;
  }

  if (default_match_escalation_threshold_arg) {
    const char *const end = default_match_escalation_threshold_arg +
                            strlen(default_match_escalation_threshold_arg);
    const char *rest = NULL;
    const int64_t value =
      grn_atoll(default_match_escalation_threshold_arg, end, &rest);
    if (end != rest) {
      fprintf(stderr,
              "invalid match escalation threshold: <%s>\n",
              default_match_escalation_threshold_arg);
      return EXIT_FAILURE;
    }
    default_match_escalation_threshold = value;
  } else {
    default_match_escalation_threshold =
      default_default_match_escalation_threshold;
  }

  if (cache_limit_arg) {
    const char *const end = cache_limit_arg + strlen(cache_limit_arg);
    const char *rest = NULL;
    const uint32_t value = grn_atoui(cache_limit_arg, end, &rest);
    if (end != rest) {
      fprintf(stderr, "invalid --cache-limit value: <%s>\n", cache_limit_arg);
      return EXIT_FAILURE;
    }
    cache_limit = value;
  }

  if (default_request_timeout_arg) {
    const char *const end =
      default_request_timeout_arg + strlen(default_request_timeout_arg);
    char *rest = NULL;
    double value;
    value = strtod(default_request_timeout_arg, &rest);
    if (end != rest) {
      fprintf(stderr,
              "invalid default request timeout: <%s>\n",
              default_request_timeout_arg);
      return EXIT_FAILURE;
    }
    default_request_timeout = value;
  } else {
    default_request_timeout = default_default_request_timeout;
  }

  if (listen_backlog_arg) {
    const char *const end = listen_backlog_arg + strlen(listen_backlog_arg);
    const char *rest = NULL;
    int value;
    value = grn_atoi(listen_backlog_arg, end, &rest);
    if (rest != end || value <= 0) {
      fprintf(stderr, "invalid listen backlog: <%s>\n", listen_backlog_arg);
      return EXIT_FAILURE;
    }
    listen_backlog = value;
  }

  if (!wal_role_arg) {
    wal_role_arg = getenv("GRN_WAL_ROLE");
  }
  if (wal_role_arg) {
    if (strcmp(wal_role_arg, "none") == 0) {
    } else if (strcmp(wal_role_arg, "primary") == 0) {
      wal_role = GRN_WAL_ROLE_PRIMARY;
      worker_wal_role = GRN_WAL_ROLE_SECONDARY;
    } else if (strcmp(wal_role_arg, "secondary") == 0) {
      wal_role = GRN_WAL_ROLE_SECONDARY;
      worker_wal_role = GRN_WAL_ROLE_SECONDARY;
    } else {
      fprintf(stderr, "invalid WAL role: <%s>\n", wal_role_arg);
      fprintf(stderr, "available WAL roles: none, primary, secondary\n");
      return EXIT_FAILURE;
    }
  }

  grn_gctx.errbuf[0] = '\0';
  if (grn_init()) {
    fprintf(stderr, "failed to initialize Groonga: %s\n", grn_gctx.errbuf);
    return EXIT_FAILURE;
  }

  grn_set_default_encoding(encoding);

  if (default_command_version_arg) {
    grn_set_default_command_version(default_command_version);
  }

  if (default_match_escalation_threshold_arg) {
    grn_set_default_match_escalation_threshold(
      default_match_escalation_threshold);
  }

  if (default_request_timeout_arg) {
    grn_set_default_request_timeout(default_request_timeout);
  }

  grn_set_segv_handler();
  grn_set_abrt_handler();
  grn_set_int_handler();
  grn_set_term_handler();
#ifdef ENABLE_LOG_REOPEN_BY_SIGNAL
  set_usr1_handler();
#endif

  if (cache_limit_arg) {
    grn_cache *cache;
    cache = grn_cache_current_get(&grn_gctx);
    grn_cache_set_max_n_entries(&grn_gctx, cache, cache_limit);
  }

  CRITICAL_SECTION_INIT(q_critical_section);
  COND_INIT(q_cond);

  if (input_path) {
    input_reader = grn_file_reader_open(&grn_gctx, input_path);
    if (!input_reader) {
      fprintf(stderr,
              "can't open input file: <%s>: %s\n",
              input_path,
              grn_gctx.errbuf);
      return EXIT_FAILURE;
    }
    batchmode = GRN_TRUE;
  } else {
    if (input_fd_arg) {
      const char *const end = input_fd_arg + strlen(input_fd_arg);
      const char *rest = NULL;
      const int input_fd = grn_atoi(input_fd_arg, end, &rest);
      if (rest != end || input_fd == 0) {
        fprintf(stderr, "invalid input FD: <%s>\n", input_fd_arg);
        return EXIT_FAILURE;
      }
      if (dup2(input_fd, STDIN_FILENO) == -1) {
        fprintf(stderr,
                "can't open input FD: %d (%s)\n",
                input_fd,
                strerror(errno));
        return EXIT_FAILURE;
      }
      input_reader = grn_file_reader_open(&grn_gctx, "-");
      if (!input_reader) {
        fprintf(stderr, "%s", grn_gctx.errbuf);
        return EXIT_FAILURE;
      }
      batchmode = GRN_TRUE;
    } else {
      input_reader = grn_file_reader_open(&grn_gctx, "-");
      if (!input_reader) {
        fprintf(stderr, "%s", grn_gctx.errbuf);
        return EXIT_FAILURE;
      }
      if (argc - i > 1) {
        batchmode = GRN_TRUE;
      } else {
        batchmode = !grn_isatty(0);
      }
    }
  }

  if ((flags & (FLAG_MODE_ALONE | FLAG_MODE_CLIENT)) && !batchmode) {
    need_line_editor = GRN_TRUE;
  }

#ifdef GRN_WITH_LIBEDIT
  if (need_line_editor) {
    line_editor_init(argc, argv);
  }
#endif

  newdb = (flags & FLAG_NEW_DB);
  is_daemon_mode = (flags & FLAG_MODE_DAEMON);
  if (flags & FLAG_MODE_CLIENT) {
    exit_code = do_client(argc - i, argv + i);
  } else if (is_daemon_mode || (flags & FLAG_MODE_SERVER)) {
    exit_code = do_server(argc > i ? argv[i] : NULL);
  } else {
    exit_code = do_alone(argc - i, argv + i);
  }

  COND_FIN(q_cond);
  CRITICAL_SECTION_FIN(q_critical_section);

  if (input_reader) {
    grn_file_reader_close(&grn_gctx, input_reader);
  }
#ifdef GRN_WITH_LIBEDIT
  if (need_line_editor) {
    line_editor_fin();
  }
#endif
  if (output != stdout) {
    fclose(output);
  }
  grn_fin();
  return exit_code;
}
