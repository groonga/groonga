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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef WIN32
#define GROONGA_MAIN
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
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif /* HAVE_SYS_SYSCTL_H */

#ifndef USE_MSG_NOSIGNAL
#  ifdef MSG_NOSIGNAL
#    undef MSG_NOSIGNAL
#  endif
#  define MSG_NOSIGNAL 0
#endif /* USE_MSG_NOSIGNAL */

#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"
#define DEFAULT_MAX_NFTHREADS 8
#define MAX_CON 0x10000

static char bind_address[HOST_NAME_MAX + 1];
static char hostname[HOST_NAME_MAX + 1];
static int port = DEFAULT_PORT;
static int batchmode;
static int number_of_lines = 0;
static int newdb;
static int useql;
static int (*do_client)(int argc, char **argv);
static int (*do_server)(char *path);
static const char *pidfile_path = NULL;
static const char *input_path = NULL;

static grn_command_version default_command_version;
static int64_t default_match_escalation_threshold;
static int log_level;
static uint32_t cache_limit;

#ifdef HAVE_LIBEDIT
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

void line_editor_init(int argc __attribute__((unused)), char *argv[])
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

void line_editor_fin(void)
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

grn_rc
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
#endif /* HAVE_LIBEDIT */

inline static grn_rc
prompt(grn_ctx *ctx, grn_obj *buf)
{
  static int the_first_read = GRN_TRUE;
  grn_rc rc = GRN_SUCCESS;
  GRN_BULK_REWIND(buf);
  if (!batchmode) {
#ifdef HAVE_LIBEDIT
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

typedef enum {
  XML_START,
  XML_START_ELEMENT,
  XML_END_ELEMENT,
  XML_TEXT
} xml_status;

typedef enum {
  XML_PLACE_NONE,
  XML_PLACE_COLUMN,
  XML_PLACE_HIT
} xml_place;

static char *
transform_xml_next_column(grn_obj *columns, int n)
{
  char *column = GRN_TEXT_VALUE(columns);
  while (n--) {
    while (*column) {
      column++;
    }
    column++;
  }
  return column;
}

static void
transform_xml(grn_ctx *ctx, grn_obj *output, grn_obj *transformed)
{
  char *s, *e;
  xml_status status = XML_START;
  xml_place place = XML_PLACE_NONE;
  grn_obj buf, name, columns, *expr;
  unsigned int len;
  int offset = 0, limit = 0, record_n = 0;
  int column_n = 0, column_text_n = 0, result_set_n = -1;
  int in_vector = 0, first_vector_element = 0;

  s = GRN_TEXT_VALUE(output);
  e = GRN_BULK_CURR(output);
  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_INIT(&name, 0);
  GRN_TEXT_INIT(&columns, 0);

  expr = ctx->impl->curr_expr;

#define EQUAL_NAME_P(_name) \
  (GRN_TEXT_LEN(&name) == strlen(_name) && \
   !memcmp(GRN_TEXT_VALUE(&name), _name, strlen(_name)))

  while (s < e) {
    switch (*s) {
    case '<' :
      s++;
      switch (*s) {
      case '/' :
        status = XML_END_ELEMENT;
        s++;
        break;
      default :
        status = XML_START_ELEMENT;
        break;
      }
      GRN_BULK_REWIND(&name);
      break;
    case '>' :
      switch (status) {
      case XML_START_ELEMENT :
        if (EQUAL_NAME_P("COLUMN")) {
          place = XML_PLACE_COLUMN;
          column_text_n = 0;
        } else if (EQUAL_NAME_P("HIT")) {
          place = XML_PLACE_HIT;
          column_n = 0;
          if (result_set_n == 0) {
            GRN_TEXT_PUTS(ctx, transformed, "<HIT NO=\"");
            grn_text_itoa(ctx, transformed, record_n++);
            GRN_TEXT_PUTS(ctx, transformed, "\">\n");
          } else {
            GRN_TEXT_PUTS(ctx, transformed, "<NAVIGATIONELEMENT ");
          }
        } else if (EQUAL_NAME_P("RESULTSET")) {
          GRN_BULK_REWIND(&columns);
          result_set_n++;
          if (result_set_n == 0) {
          } else {
            GRN_TEXT_PUTS(ctx, transformed, "<NAVIGATIONENTRY>\n");
          }
        } else if (EQUAL_NAME_P("VECTOR")) {
          char *c = transform_xml_next_column(&columns, column_n++);
          in_vector = 1;
          first_vector_element = 1;
          GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
          GRN_TEXT_PUTS(ctx, transformed, c);
          GRN_TEXT_PUTS(ctx, transformed, "\">");
        }
        break;
      case XML_END_ELEMENT :
        if (EQUAL_NAME_P("HIT")) {
          place = XML_PLACE_NONE;
          if (result_set_n == 0) {
            GRN_TEXT_PUTS(ctx, transformed, "</HIT>\n");
          } else {
            GRN_TEXT_PUTS(ctx, transformed, "/>\n");
          }
        } else if (EQUAL_NAME_P("RESULTSET")) {
          place = XML_PLACE_NONE;
          if (result_set_n == 0) {
            GRN_TEXT_PUTS(ctx, transformed, "</RESULTSET>\n");
          } else {
            GRN_TEXT_PUTS(ctx, transformed,
                          "</NAVIGATIONELEMENTS>\n"
                          "</NAVIGATIONENTRY>\n");
          }
        } else if (EQUAL_NAME_P("RESULT")) {
          GRN_TEXT_PUTS(ctx, transformed,
                        "</RESULTPAGE>\n"
                        "</SEGMENT>\n"
                        "</SEGMENTS>\n");
        } else if (EQUAL_NAME_P("VECTOR")) {
          in_vector = 0;
          first_vector_element = 0;
          GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
        } else {
          switch (place) {
          case XML_PLACE_HIT :
            if (result_set_n == 0) {
              if (!in_vector) {
                char *c = transform_xml_next_column(&columns, column_n++);
                GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
                GRN_TEXT_PUTS(ctx, transformed, c);
                GRN_TEXT_PUTS(ctx, transformed, "\">");
              }
              if (in_vector && !first_vector_element) {
                GRN_TEXT_PUTS(ctx, transformed, ", ");
              }
              GRN_TEXT_PUT(ctx, transformed,
                           GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
              if (!in_vector) {
                GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
              }
            } else {
              char *c = transform_xml_next_column(&columns, column_n++);
              GRN_TEXT_PUTS(ctx, transformed, c);
              GRN_TEXT_PUTS(ctx, transformed, "=\"");
              GRN_TEXT_PUT(ctx, transformed,
                           GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
              GRN_TEXT_PUTS(ctx, transformed, "\" ");
            }
            first_vector_element = 0;
            break;
          default :
            if (EQUAL_NAME_P("NHITS")) {
              if (result_set_n == 0) {
                uint32_t nhits;
                grn_obj *offset_value, *limit_value;

                nhits = grn_atoui(GRN_TEXT_VALUE(&buf), GRN_BULK_CURR(&buf),
                                  NULL);
                offset_value = grn_expr_get_var(ctx, expr,
                                                "offset", strlen("offset"));
                limit_value = grn_expr_get_var(ctx, expr,
                                               "limit", strlen("limit"));
                if (GRN_TEXT_LEN(offset_value)) {
                  offset = grn_atoi(GRN_TEXT_VALUE(offset_value),
                                    GRN_BULK_CURR(offset_value),
                                    NULL);
                } else {
                  offset = 0;
                }
                if (GRN_TEXT_LEN(limit_value)) {
                  limit = grn_atoi(GRN_TEXT_VALUE(limit_value),
                                   GRN_BULK_CURR(limit_value),
                                   NULL);
                } else {
#define DEFAULT_LIMIT 10
                  limit = DEFAULT_LIMIT;
#undef DEFAULT_LIMIT
                }
                grn_normalize_offset_and_limit(ctx, nhits, &offset, &limit);
                record_n = offset + 1;
                GRN_TEXT_PUTS(ctx, transformed,
                              "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                              "<SEGMENTS>\n"
                              "<SEGMENT>\n"
                              "<RESULTPAGE>\n"
                              "<RESULTSET OFFSET=\"");
                grn_text_lltoa(ctx, transformed, offset);
                GRN_TEXT_PUTS(ctx, transformed, "\" LIMIT=\"");
                grn_text_lltoa(ctx, transformed, limit);
                GRN_TEXT_PUTS(ctx, transformed, "\" NHITS=\"");
                grn_text_lltoa(ctx, transformed, nhits);
                GRN_TEXT_PUTS(ctx, transformed, "\">\n");
              } else {
                GRN_TEXT_PUTS(ctx, transformed,
                              "<NAVIGATIONELEMENTS COUNT=\"");
                GRN_TEXT_PUT(ctx, transformed,
                             GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
                GRN_TEXT_PUTS(ctx, transformed,
                              "\">\n");
              }
            } else if (EQUAL_NAME_P("TEXT")) {
              switch (place) {
              case XML_PLACE_COLUMN :
                if (column_text_n == 0) {
                  GRN_TEXT_PUT(ctx, &columns,
                               GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
                  GRN_TEXT_PUTC(ctx, &columns, '\0');
                }
                column_text_n++;
                break;
              default :
                break;
              }
            }
          }
        }
      default :
        break;
      }
      s++;
      GRN_BULK_REWIND(&buf);
      status = XML_TEXT;
      break;
    default :
      len = grn_charlen(ctx, s, e);
      switch (status) {
      case XML_START_ELEMENT :
      case XML_END_ELEMENT :
        GRN_TEXT_PUT(ctx, &name, s, len);
        break;
      default :
        GRN_TEXT_PUT(ctx, &buf, s, len);
        break;
      }
      s += len;
      break;
    }
  }
#undef EQUAL_NAME_P

  GRN_OBJ_FIN(ctx, &buf);
  GRN_OBJ_FIN(ctx, &name);
  GRN_OBJ_FIN(ctx, &columns);
}

#ifdef HAVE_MESSAGE_PACK
typedef struct {
  grn_ctx *ctx;
  grn_obj *buffer;
} msgpack_writer_ctx;

static inline int
msgpack_buffer_writer(void* data, const char* buf, unsigned int len)
{
  msgpack_writer_ctx *writer_ctx = (msgpack_writer_ctx *)data;
  return grn_bulk_write(writer_ctx->ctx, writer_ctx->buffer, buf, len);
}
#endif

static void
print_return_code(grn_ctx *ctx, grn_rc rc, grn_obj *head, grn_obj *body, grn_obj *foot)
{
  double started, finished, elapsed;

  grn_timeval tv_now;
  grn_timeval_now(ctx, &tv_now);
  started = ctx->impl->tv.tv_sec;
  started += ctx->impl->tv.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  finished = tv_now.tv_sec;
  finished += tv_now.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  elapsed = finished - started;

  switch (ctx->impl->output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTS(ctx, head, "[[");
    grn_text_itoa(ctx, head, rc);
    GRN_TEXT_PUTC(ctx, head, ',');
    grn_text_ftoa(ctx, head, started);
    GRN_TEXT_PUTC(ctx, head, ',');
    grn_text_ftoa(ctx, head, elapsed);
    if (rc != GRN_SUCCESS) {
      GRN_TEXT_PUTC(ctx, head, ',');
      grn_text_esc(ctx, head, ctx->errbuf, strlen(ctx->errbuf));
      if (ctx->errfunc && ctx->errfile) {
        grn_obj *command;
        /* TODO: output backtrace */
        GRN_TEXT_PUTS(ctx, head, ",[[");
        grn_text_esc(ctx, head, ctx->errfunc, strlen(ctx->errfunc));
        GRN_TEXT_PUTC(ctx, head, ',');
        grn_text_esc(ctx, head, ctx->errfile, strlen(ctx->errfile));
        GRN_TEXT_PUTC(ctx, head, ',');
        grn_text_itoa(ctx, head, ctx->errline);
        if (input_path && (command = GRN_CTX_USER_DATA(ctx)->ptr)) {
          GRN_TEXT_PUTC(ctx, head, ',');
          grn_text_esc(ctx, head, input_path, strlen(input_path));
          GRN_TEXT_PUTC(ctx, head, ',');
          grn_text_itoa(ctx, head, number_of_lines);
          GRN_TEXT_PUTC(ctx, head, ',');
          grn_text_esc(ctx, head, GRN_TEXT_VALUE(command), GRN_TEXT_LEN(command));
        }
        GRN_TEXT_PUTS(ctx, head, "]]");
      }
    }
    GRN_TEXT_PUTC(ctx, head, ']');
    if (GRN_TEXT_LEN(body)) { GRN_TEXT_PUTC(ctx, head, ','); }
    GRN_TEXT_PUTC(ctx, foot, ']');
    break;
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, head, rc);
    GRN_TEXT_PUTC(ctx, head, '\t');
    grn_text_ftoa(ctx, head, started);
    GRN_TEXT_PUTC(ctx, head, '\t');
    grn_text_ftoa(ctx, head, elapsed);
    if (rc != GRN_SUCCESS) {
      GRN_TEXT_PUTC(ctx, head, '\t');
      grn_text_esc(ctx, head, ctx->errbuf, strlen(ctx->errbuf));
      if (ctx->errfunc && ctx->errfile) {
        /* TODO: output backtrace */
        GRN_TEXT_PUTC(ctx, head, '\t');
        grn_text_esc(ctx, head, ctx->errfunc, strlen(ctx->errfunc));
        GRN_TEXT_PUTC(ctx, head, '\t');
        grn_text_esc(ctx, head, ctx->errfile, strlen(ctx->errfile));
        GRN_TEXT_PUTC(ctx, head, '\t');
        grn_text_itoa(ctx, head, ctx->errline);
      }
    }
    GRN_TEXT_PUTS(ctx, head, "\n");
    GRN_TEXT_PUTS(ctx, foot, "\nEND");
    break;
  case GRN_CONTENT_XML:
    {
      char buf[GRN_TABLE_MAX_KEY_SIZE];
      int is_select = 0;
      if (!rc && ctx->impl->curr_expr) {
        int len = grn_obj_name(ctx, ctx->impl->curr_expr,
                               buf, GRN_TABLE_MAX_KEY_SIZE);
        buf[len] = '\0';
        is_select = strcmp(buf, "select") == 0;
      }
      if (is_select) {
        grn_obj transformed;
        GRN_TEXT_INIT(&transformed, 0);
        transform_xml(ctx, body, &transformed);
        GRN_TEXT_SET(ctx, body,
                     GRN_TEXT_VALUE(&transformed), GRN_TEXT_LEN(&transformed));
        GRN_OBJ_FIN(ctx, &transformed);
      } else {
        GRN_TEXT_PUTS(ctx, head, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<RESULT CODE=\"");
        grn_text_itoa(ctx, head, rc);
        GRN_TEXT_PUTS(ctx, head, "\" UP=\"");
        grn_text_ftoa(ctx, head, started);
        GRN_TEXT_PUTS(ctx, head, "\" ELAPSED=\"");
        grn_text_ftoa(ctx, head, elapsed);
        GRN_TEXT_PUTS(ctx, head, "\">\n");
        if (rc != GRN_SUCCESS) {
          GRN_TEXT_PUTS(ctx, head, "<ERROR>");
          grn_text_escape_xml(ctx, head, ctx->errbuf, strlen(ctx->errbuf));
          if (ctx->errfunc && ctx->errfile) {
            /* TODO: output backtrace */
            GRN_TEXT_PUTS(ctx, head, "<INFO FUNC=\"");
            grn_text_escape_xml(ctx, head, ctx->errfunc, strlen(ctx->errfunc));
            GRN_TEXT_PUTS(ctx, head, "\" FILE=\"");
            grn_text_escape_xml(ctx, head, ctx->errfile, strlen(ctx->errfile));
            GRN_TEXT_PUTS(ctx, head, "\" LINE=\"");
            grn_text_itoa(ctx, head, ctx->errline);
            GRN_TEXT_PUTS(ctx, head, "\">");
          }
          GRN_TEXT_PUTS(ctx, head, "</ERROR>");
        }
        GRN_TEXT_PUTS(ctx, foot, "\n</RESULT>");
      }
    }
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef HAVE_MESSAGE_PACK
    {
      msgpack_writer_ctx head_writer_ctx;
      msgpack_packer header_packer;
      int header_size;

      head_writer_ctx.ctx = ctx;
      head_writer_ctx.buffer = head;
      msgpack_packer_init(&header_packer, &head_writer_ctx, msgpack_buffer_writer);

       /* [HEAD, (BODY)] */
      if (GRN_TEXT_LEN(body) > 0) {
        msgpack_pack_array(&header_packer, 2);
      } else {
        msgpack_pack_array(&header_packer, 1);
      }

      /* HEAD := [rc, started, elapsed, (error, (ERROR DETAIL))] */
      header_size = 3;
      if (rc != GRN_SUCCESS) {
        header_size++;
        if (ctx->errfunc && ctx->errfile) {
          header_size++;
        }
      }
      msgpack_pack_array(&header_packer, header_size);
      msgpack_pack_int(&header_packer, rc);

      msgpack_pack_double(&header_packer, started);
      msgpack_pack_double(&header_packer, elapsed);

      if (rc != GRN_SUCCESS) {
        msgpack_pack_raw(&header_packer, strlen(ctx->errbuf));
        msgpack_pack_raw_body(&header_packer, ctx->errbuf, strlen(ctx->errbuf));
        if (ctx->errfunc && ctx->errfile) {
          grn_obj *command = GRN_CTX_USER_DATA(ctx)->ptr;
          int error_detail_size;

          /* ERROR DETAIL := [[errfunc, errfile, errline,
                               (input_path, number_of_lines, command)]] */
          /* TODO: output backtrace */
          msgpack_pack_array(&header_packer, 1);
          error_detail_size = 3;
          if (command) {
            error_detail_size += 3;
          }
          msgpack_pack_array(&header_packer, error_detail_size);

          msgpack_pack_raw(&header_packer, strlen(ctx->errfunc));
          msgpack_pack_raw_body(&header_packer, ctx->errfunc, strlen(ctx->errfunc));

          msgpack_pack_raw(&header_packer, strlen(ctx->errfile));
          msgpack_pack_raw_body(&header_packer, ctx->errfile, strlen(ctx->errfile));

          msgpack_pack_int(&header_packer, ctx->errline);

          if (command) {
            if (input_path) {
              msgpack_pack_raw(&header_packer, strlen(input_path));
              msgpack_pack_raw_body(&header_packer, input_path, strlen(input_path));
            } else {
              msgpack_pack_raw(&header_packer, 7);
              msgpack_pack_raw_body(&header_packer, "(stdin)", 7);
            }

            msgpack_pack_int(&header_packer, number_of_lines);

            msgpack_pack_raw(&header_packer, GRN_TEXT_LEN(command));
            msgpack_pack_raw_body(&header_packer, GRN_TEXT_VALUE(command), GRN_TEXT_LEN(command));
          }
        }
      }
    }
#endif
    break;
  case GRN_CONTENT_NONE:
    break;
  }
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
      print_return_code(ctx, ctx->rc, &head, buf, &foot);
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
  int rc = -1;
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
    grn_ctx_recv_handler_set(ctx, s_output, stdout);
    if (!argc) {
      grn_obj text;
      GRN_TEXT_INIT(&text, 0);
      while (prompt(ctx, &text) != GRN_END_OF_DATA) {
        GRN_TEXT_PUT(ctx, &command, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text));
        grn_ctx_send(ctx, GRN_TEXT_VALUE(&text), GRN_TEXT_LEN(&text), 0);
        if (ctx->stat == GRN_CTX_QUIT) { break; }
      }
      rc = ctx->rc;
      grn_obj_unlink(ctx, &text);
    } else {
      rc = grn_ctx_sendv(ctx, argc, argv, 0);
    }
    grn_obj_unlink(ctx, &command);
    grn_obj_close(ctx, db);
  } else {
    fprintf(stderr, "db open failed (%s): %s\n", path, ctx->errbuf);
  }
  grn_ctx_fin(ctx);
  return rc;
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
      print_return_code(ctx, ctx->rc, &head, &body, &foot);
      fwrite(GRN_TEXT_VALUE(&head), 1, GRN_TEXT_LEN(&head), stdout);
      fwrite(GRN_TEXT_VALUE(&body), 1, GRN_TEXT_LEN(&body), stdout);
      fwrite(GRN_TEXT_VALUE(&foot), 1, GRN_TEXT_LEN(&foot), stdout);
      fputc('\n', stdout);
      fflush(stdout);
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
  int rc = -1;
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
        rc = ctx->rc;
        if (rc) { break; }
        if (c_output(ctx)) { goto exit; }
        if (ctx->stat == GRN_CTX_QUIT) { break; }
      }
      grn_obj_unlink(ctx, &text);
    } else {
      rc = grn_ctx_sendv(ctx, argc, argv, 0);
      if (c_output(ctx)) { goto exit; }
    }
  } else {
    fprintf(stderr, "grn_ctx_connect failed (%s:%d)\n", hostname, port);
  }
exit :
  grn_ctx_fin(ctx);
  return rc;
}

/* server */

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
  const char *mime_type = ctx->impl->mime_type;
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
      print_return_code(ctx, expr_rc, &head, outbuf, &foot);
      GRN_TEXT_PUTS(ctx, &foot, ");");
    } else {
      print_return_code(ctx, expr_rc, &head, outbuf, &foot);
    }
    GRN_TEXT_SETS(ctx, body, "HTTP/1.1 200 OK\r\n");
    GRN_TEXT_PUTS(ctx, body, "Connection: close\r\n");
    GRN_TEXT_PUTS(ctx, body, "Content-Type: ");
    GRN_TEXT_PUTS(ctx, body, mime_type);
    GRN_TEXT_PUTS(ctx, body, "\r\nContent-Length: ");
    grn_text_lltoa(ctx, body,
                   GRN_TEXT_LEN(&head) + GRN_TEXT_LEN(outbuf) + GRN_TEXT_LEN(&foot));
    GRN_TEXT_PUTS(ctx, body, "\r\n\r\n");
  } else {
    GRN_BULK_REWIND(outbuf);
    print_return_code(ctx, expr_rc, &head, outbuf, &foot);
    if (expr_rc == GRN_NO_SUCH_FILE_OR_DIRECTORY) {
      GRN_TEXT_SETS(ctx, body, "HTTP/1.1 404 Not Found\r\n");
    } else {
      GRN_TEXT_SETS(ctx, body, "HTTP/1.1 500 Internal Server Error\r\n");
    }
    GRN_TEXT_PUTS(ctx, body, "Content-Type: ");
    GRN_TEXT_PUTS(ctx, body, mime_type);
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
      GRN_LOG(&grn_gctx, GRN_LOG_NOTICE, "couldn't send all data (%d/%d)",
              ret, len);
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

#define MBRES(ctx,re,status,key_len,extra_len,flags) {\
  grn_msg_set_property((ctx), (re), (status), (key_len), (extra_len));\
  grn_msg_send((ctx), (re), (flags));\
}

#define GRN_MSG_MBRES(block) \
do {\
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
        // GRN_LOG(ctx, GRN_LOG_NOTICE, "GET k=%d not found", keylen);
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

static grn_com_queue ctx_new;
static grn_com_queue ctx_old;
static grn_mutex q_mutex;
static grn_cond q_cond;
static uint32_t nthreads = 0, nfthreads = 0, max_nfthreads;

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
  int rc = -1;
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
    // RLIMIT_OFILE
    setrlimit(RLIMIT_NOFILE, &lim);
    lim.rlim_cur = 0;
    lim.rlim_max = 0;
    getrlimit(RLIMIT_NOFILE, &lim);
    GRN_LOG(ctx, GRN_LOG_NOTICE, "RLIMIT_NOFILE(%d,%d)", lim.rlim_cur, lim.rlim_max);
  }
#endif /* WIN32 */
  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
    grn_obj *db;
    db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
    if (db) {
      struct hostent *he;
      if (!(he = gethostbyname(hostname))) {
        SERR("gethostbyname");
      } else {
        ev.opaque = db;
        grn_edges_init(ctx, NULL);
        if (!grn_com_sopen(ctx, &ev, bind_address, port, h_handler, he)) {
          while (!grn_com_event_poll(ctx, &ev, 1000) && grn_gctx.stat != GRN_CTX_QUIT) {
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
            // todo : log stat
          }
          for (;;) {
            MUTEX_LOCK(q_mutex);
            if (nthreads == nfthreads) { break; }
            MUTEX_UNLOCK(q_mutex);
            usleep(1000);
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
            GRN_HASH_EACH(ctx, ev.hash, id, NULL, NULL, &com, { grn_com_close(ctx, com); });
          }
          rc = 0;
        } else {
          fprintf(stderr, "grn_com_sopen failed (%s:%d): %s\n",
                  bind_address, port, ctx->errbuf);
        }
        grn_edges_fin(ctx);
      }
      grn_obj_close(ctx, db);
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
dispatcher(grn_ctx *ctx, grn_edge *edge)
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
      dispatcher(ctx, edge);
    }
  }
}

static int
g_server(char *path)
{
  int rc = -1;
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
    // RLIMIT_OFILE
    setrlimit(RLIMIT_NOFILE, &lim);
    lim.rlim_cur = 0;
    lim.rlim_max = 0;
    getrlimit(RLIMIT_NOFILE, &lim);
    GRN_LOG(ctx, GRN_LOG_NOTICE, "RLIMIT_NOFILE(%d,%d)", lim.rlim_cur, lim.rlim_max);
  }
#endif /* WIN32 */
  if (!grn_com_event_init(ctx, &ev, MAX_CON, sizeof(grn_com))) {
    grn_obj *db;
    db = (newdb || !path) ? grn_db_create(ctx, path, NULL) : grn_db_open(ctx, path);
    if (db) {
      struct hostent *he;
      if (!(he = gethostbyname(hostname))) {
        SERR("gethostbyname");
      } else {
        ev.opaque = db;
        grn_edges_init(ctx, dispatcher);
        if (!grn_com_sopen(ctx, &ev, bind_address, port, g_handler, he)) {
          while (!grn_com_event_poll(ctx, &ev, 1000) && grn_gctx.stat != GRN_CTX_QUIT) {
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
            // todo : log stat
          }
          for (;;) {
            MUTEX_LOCK(q_mutex);
            if (nthreads == nfthreads) { break; }
            MUTEX_UNLOCK(q_mutex);
            usleep(1000);
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
            GRN_HASH_EACH(ctx, ev.hash, id, NULL, NULL, &com, { grn_com_close(ctx, com); });
          }
          rc = 0;
        } else {
          fprintf(stderr, "grn_com_sopen failed (%s:%d): %s\n",
                  bind_address, port, ctx->errbuf);
        }
        grn_edges_fin(ctx);
      }
      grn_obj_close(ctx, db);
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
do_daemon(char *path)
{
  int rc;
#ifndef WIN32
  pid_t pid;
  FILE *pidfile = NULL;

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
  if (pidfile_path) {
    pidfile = fopen(pidfile_path, "w");
  }
  switch ((pid = fork())) {
  case 0:
    break;
  case -1:
    perror("fork");
    return -1;
  default:
    if (!pidfile) {
      fprintf(stderr, "%d\n", pid);
    } else {
      fprintf(pidfile, "%d\n", pid);
      fclose(pidfile);
    }
    _exit(0);
  }
  {
    int null_fd = GRN_OPEN("/dev/null", O_RDWR, 0);
    if (null_fd != -1) {
      dup2(null_fd, 0);
      dup2(null_fd, 1);
      dup2(null_fd, 2);
      if (null_fd > 2) { GRN_CLOSE(null_fd); }
    }
  }
#endif /* WIN32 */
  rc = do_server(path);
#ifndef WIN32
  if (pidfile) {
    fclose(pidfile);
    unlink(pidfile_path);
  }
#endif

  return rc;
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

#define SET_LOGLEVEL(x) do {\
  static grn_logger_info info;\
  info.max_level = (x);\
  info.flags = GRN_LOG_TIME|GRN_LOG_MESSAGE;\
  info.func = NULL;\
  info.func_arg = NULL;\
  grn_logger_info_set(&grn_gctx, &info);\
} while(0)

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

static inline char *
skipspace(char *str)
{
  while (*str == ' ' || *str == '\t') { ++str; }
  return str;
}

static int
load_config_file(const char *path,
                 const grn_str_getopt_opt *opts, int *flags)
{
  int name_len, value_len;
  char buf[1024+2], *str, *name, *args[4];
  FILE *file;

  if (!(file = fopen(path, "r"))) return 0;

  args[0] = (char *)path;
  args[3] = NULL;
  while ((str = fgets(buf + 2, sizeof(buf) - 2, file))) {
    char *value = NULL;
    str = skipspace(str);
    switch (*str) {
    case '#': case ';': case '\0':
      continue;
    }
    name = str;
    while (*str && !isspace(*str) && *str != '=') { str++; }
    if ((name_len = (int)(str - name)) == 0) {
      continue;
    }
    value_len = 0;
    if (*str && (*str == '=' || *(str = skipspace(str)) == '=')) {
      str++;
      value = str = skipspace(str);
      while (*str && *str != '#' && *str != ';') {
        if (!isspace(*str)) {
          value_len = (int)(str - value) + 1;
        }
        str++;
      }
      value[value_len] = '\0';
    }
    name[name_len] = '\0';
    memset(name -= 2, '-', 2);
    args[1] = name;
    args[2] = value ? strdup(value) : NULL;
    grn_str_getopt((value_len > 0) + 2, args, opts, flags);
  }
  fclose(file);

  return 1;
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

  default_log_path = grn_log_path;
  default_query_log_path = grn_qlog_path;

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
#ifdef HAVE_MESSAGE_PACK
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
          "options:\n"
          "  -n:                               create new database\n"
          "  -e, --encoding <encoding>:        encoding for new database [none|euc|utf8|sjis|latin1|koi8r]\n"
          "  -c:                               run in client mode\n"
          "  -s:                               run in server mode\n"
          "  -d:                               run in daemon mode\n"
          "  --bind-address <ip/hostname>:     server address to bind (default: %s)\n"
          "  -p, --port <port number>:         server port number (default: %d)\n"
          "  -i, --server-id <ip/hostname>:    server ID address (default: %s)\n"
          "  --protocol <protocol>:            server protocol to listen (default: gqtp)\n"
          "  --document-root <path>:           document root path\n"
          "  --cache-limit <limit>:            specify the max number of cache data\n"
          "  -t, --max-threads <max threads>:  max number of free threads (default: %d)\n"
          "  --file <path>:                    read commands from specified file\n"
          "  -l, --log-level <log level>:      log level\n"
          "  --log-path <path>:                specify log path\n"
          "  --query-log-path <path>:          specify query log path\n"
          "  --pid-path <path>:                specify pid file path (daemon mode only)\n"
          "  --config-path <path>:             specify config file path\n"
          "  --default-command-version <version>:\n"
          "                                    specify default command version\n"
          "  --default-match-escalation-threshold <threshold>:\n"
          "                                    specify default match escalation threshold\n"
          "      --show-config:                show config\n"
          "  -h, --help:                       show usage\n"
          "  --version:                        show groonga version\n"
          "\n"
          "dest: <db pathname> [<command>] or <dest hostname>\n"
          "  <db pathname> [<command>]: when standalone/server mode\n"
          "  <dest hostname>: when client mode (default: \"%s\")\n",
          default_bind_address, default_port, default_hostname,
          default_max_num_threads, default_dest);
}

int
main(int argc, char **argv)
{
  grn_encoding enc = GRN_ENC_DEFAULT;
  const char *port_arg = NULL, *encstr = NULL,
    *max_num_threads_arg = NULL, *log_level_arg = NULL,
    *bind_address_arg = NULL, *hostname_arg = NULL, *protocol = NULL,
    *cache_limit_arg = NULL, *default_command_version_arg = NULL,
    *default_match_escalation_threshold_arg = NULL;
  const char *config_path = NULL;
  int r, i, mode = mode_alone;
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
    {'\0', NULL, NULL, 0, 0}
  };
  opts[0].arg = &port_arg;
  opts[1].arg = &encstr;
  opts[2].arg = &max_num_threads_arg;
  opts[7].arg = &log_level_arg;
  opts[8].arg = &hostname_arg;
  opts[11].arg = &protocol;
  opts[13].arg = &grn_log_path;
  opts[14].arg = &grn_qlog_path;
  opts[15].arg = &pidfile_path;
  opts[16].arg = &config_path;
  opts[18].arg = &cache_limit_arg;
  opts[19].arg = &input_path;
  opts[20].arg = &grn_document_root;
  opts[21].arg = &default_command_version_arg;
  opts[22].arg = &default_match_escalation_threshold_arg;
  opts[23].arg = &bind_address_arg;

  init_default_settings();

  i = grn_str_getopt(argc, argv, opts, &mode);
  if (i < 0) {
    show_usage(stderr);
    return EXIT_FAILURE;
  }

  if (config_path) {
    if (!load_config_file(config_path, opts, &mode)) {
      fprintf(stderr, "%s: can't open config file: %s (%s)\n",
              argv[0], config_path, strerror(errno));
      return EXIT_FAILURE;
    }
  } else if (*default_config_path) {
    load_config_file(default_config_path, opts, &mode);
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
    show_usage(stdout);
    return EXIT_SUCCESS;
  case mode_config :
    show_config(stdout, opts, mode & ~MODE_MASK);
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

  if (encstr) {
    switch (*encstr) {
    case 'n' :
    case 'N' :
      enc = GRN_ENC_NONE;
      break;
    case 'e' :
    case 'E' :
      enc = GRN_ENC_EUC_JP;
      break;
    case 'u' :
    case 'U' :
      enc = GRN_ENC_UTF8;
      break;
    case 's' :
    case 'S' :
      enc = GRN_ENC_SJIS;
      break;
    case 'l' :
    case 'L' :
      enc = GRN_ENC_LATIN1;
      break;
    case 'k' :
    case 'K' :
      enc = GRN_ENC_KOI8R;
      break;
    default:
      enc = GRN_ENC_DEFAULT;
      break;
    }
  }

  if (!grn_document_root) {
    grn_document_root = default_document_root;
  }

  if (protocol) {
    switch (*protocol) {
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
    batchmode = !isatty(0);
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
  }

#ifdef HAVE_LIBEDIT
  if (!batchmode) {
    line_editor_init(argc, argv);
  }
#endif
  if (grn_init()) { return -1; }

  grn_set_default_encoding(enc);

  if (default_command_version_arg) {
    grn_set_default_command_version(default_command_version);
  }

  if (default_match_escalation_threshold_arg) {
    grn_set_default_match_escalation_threshold(default_match_escalation_threshold);
  }

  if (log_level_arg) {
    SET_LOGLEVEL(log_level);
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
    r = do_alone(argc - i, argv + i);
    break;
  case mode_client :
    r = do_client(argc - i, argv + i);
    break;
  case mode_daemon :
    r = do_daemon(argc > i ? argv[i] : NULL);
    break;
  case mode_server :
    r = do_server(argc > i ? argv[i] : NULL);
    break;
  default:
    r = -1;
    break;
  }

#ifdef HAVE_LIBEDIT
  if (!batchmode) {
    line_editor_fin();
  }
#endif
  grn_fin();
  return r;
}
