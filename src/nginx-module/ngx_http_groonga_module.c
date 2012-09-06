/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012 Brazil

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <groonga.h>

#define GRN_NO_FLAGS 0

typedef struct {
  ngx_flag_t enabled;
  ngx_str_t database_path;
  char *database_path_cstr;
  ngx_flag_t database_auto_create;
  ngx_str_t base_path;
  char *config_file;
  int config_line;
  char *name;
  grn_ctx context;
} ngx_http_groonga_loc_conf_t;

typedef struct {
  ngx_log_t *log;
  ngx_pool_t *pool;
  ngx_int_t rc;
} ngx_http_groonga_database_callback_data_t;

typedef struct {
  grn_ctx context;
  grn_obj head;
  grn_obj body;
  grn_obj foot;
} ngx_http_groonga_handler_data_t;

typedef void (*ngx_http_groonga_loc_conf_callback_pt)(ngx_http_groonga_loc_conf_t *conf, void *user_data);

ngx_module_t ngx_http_groonga_module;

static char *
ngx_str_null_terminate(ngx_pool_t *pool, const ngx_str_t *string)
{
  char *null_terminated_c_string;

  null_terminated_c_string = ngx_pnalloc(pool, string->len + 1);
  if (!null_terminated_c_string) {
    return NULL;
  }

  memcpy(null_terminated_c_string, string->data, string->len);
  null_terminated_c_string[string->len] = '\0';

  return null_terminated_c_string;
}

static grn_bool
ngx_str_equal_c_string(ngx_str_t *string, const char *c_string)
{
  if (string->len != strlen(c_string)) {
    return GRN_FALSE;
  }

  return memcmp(c_string, string->data, string->len) == 0;
}

static void
ngx_http_groonga_context_log_error(ngx_log_t *log, grn_ctx *context)
{
  if (context->rc == GRN_SUCCESS) {
    return;
  }

  ngx_log_error(NGX_LOG_ERR, log, 0, "%s", context->errbuf);
}

static ngx_int_t
ngx_http_groonga_context_check_error(ngx_log_t *log, grn_ctx *context)
{
  if (context->rc == GRN_SUCCESS) {
    return NGX_OK;
  } else {
    ngx_http_groonga_context_log_error(log, context);
    return NGX_HTTP_BAD_REQUEST;
  }
}

static ngx_buf_t *
ngx_http_groonga_grn_obj_to_ngx_buf(ngx_pool_t *pool, grn_obj *object)
{
  ngx_buf_t *buffer;
  buffer = ngx_pcalloc(pool, sizeof(ngx_buf_t));
  if (buffer == NULL) {
    return NULL;
  }

  /* adjust the pointers of the buffer */
  buffer->pos = (u_char *)GRN_TEXT_VALUE(object);
  buffer->last = (u_char *)GRN_TEXT_VALUE(object) + GRN_TEXT_LEN(object);
  buffer->memory = 1;    /* this buffer is in memory */

  return buffer;
}

static void
ngx_http_groonga_handler_cleanup(void *user_data)
{
  ngx_http_groonga_handler_data_t *data = user_data;
  grn_ctx *context;

  context = &(data->context);
  GRN_OBJ_FIN(context, &(data->head));
  GRN_OBJ_FIN(context, &(data->body));
  GRN_OBJ_FIN(context, &(data->foot));
  grn_ctx_fin(context);
}

static void
ngx_http_groonga_context_receive_handler(grn_ctx *context,
                                         int flags,
                                         void *callback_data)
{
  ngx_http_groonga_handler_data_t *data = callback_data;
  char *result = NULL;
  unsigned int result_size = 0;
  int recv_flags;

  if (!(flags & GRN_CTX_TAIL)) {
    return;
  }

  grn_ctx_recv(context, &result, &result_size, &recv_flags);

  if (recv_flags == GRN_CTX_QUIT) {
    ngx_int_t ngx_rc;
    ngx_int_t ngx_pid;

    if (ngx_process == NGX_PROCESS_SINGLE) {
      ngx_pid = getpid();
    } else {
      ngx_pid = getppid();
    }

    ngx_rc = ngx_os_signal_process((ngx_cycle_t*)ngx_cycle,
                                   "stop",
                                   ngx_pid);
    if (ngx_rc == NGX_OK) {
      context->stat &= ~GRN_CTX_QUIT;
      grn_ctx_recv(context, &result, &result_size, &recv_flags);
      context->stat |= GRN_CTX_QUIT;
    } else {
      context->rc = GRN_OPERATION_NOT_PERMITTED;
      GRN_TEXT_PUTS(context, &(data->body), "false");
      context->stat &= ~GRN_CTX_QUIT;
    }
  }

  if (result_size > 0 ||
      GRN_TEXT_LEN(&(data->body)) > 0 ||
      context->rc != GRN_SUCCESS) {
    if (result_size > 0) {
      GRN_TEXT_PUT(context, &(data->body), result, result_size);
    }

    grn_output_envelope(context,
                        context->rc,
                        &(data->head),
                        &(data->body),
                        &(data->foot),
                        NULL,
                        0);
  }
}

static ngx_int_t
ngx_http_groonga_extract_command_path(ngx_http_request_t *r,
                                      ngx_str_t *command_path)
{
  size_t base_path_length;

  ngx_http_core_loc_conf_t *http_location_conf;
  ngx_http_groonga_loc_conf_t *location_conf;

  http_location_conf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
  location_conf = ngx_http_get_module_loc_conf(r, ngx_http_groonga_module);

  command_path->data = r->unparsed_uri.data;
  command_path->len = r->unparsed_uri.len;
  base_path_length = http_location_conf->name.len;
  if (location_conf->base_path.len > 0) {
    if (command_path->len < location_conf->base_path.len) {
      ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                    "requestd URI is shorter than groonga_base_path: "
                    "URI: <%V>, groonga_base_path: <%V>",
                    &(r->unparsed_uri), &(location_conf->base_path));
    } else if (strncmp((const char *)command_path->data,
                       (const char *)(location_conf->base_path.data),
                       location_conf->base_path.len) < 0) {
      ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                    "groonga_base_path doesn't match requested URI: "
                    "URI: <%V>, groonga_base_path: <%V>",
                    &(r->unparsed_uri), &(location_conf->base_path));
    } else {
      base_path_length = location_conf->base_path.len;
    }
  }
  command_path->data += base_path_length;
  command_path->len -= base_path_length;
  if (command_path->len > 0 && command_path->data[0] == '/') {
    command_path->data += 1;
    command_path->len -= 1;
  }
  if (command_path->len == 0) {
    return NGX_HTTP_BAD_REQUEST;
  }

  return NGX_OK;
}

static void
ngx_http_groonga_handler_set_content_type(ngx_http_request_t *r,
                                          const char *content_type)
{
  r->headers_out.content_type.len = strlen(content_type);
  r->headers_out.content_type.data = (u_char *)content_type;
  r->headers_out.content_type_len = r->headers_out.content_type.len;
}

static ngx_int_t
ngx_http_groonga_handler_create_data(ngx_http_request_t *r,
                                     ngx_http_groonga_handler_data_t **data_return)
{
  ngx_int_t rc;

  ngx_http_groonga_loc_conf_t *location_conf;

  ngx_http_cleanup_t *cleanup;
  ngx_http_groonga_handler_data_t *data;

  grn_ctx *context;

  location_conf = ngx_http_get_module_loc_conf(r, ngx_http_groonga_module);

  cleanup = ngx_http_cleanup_add(r, sizeof(ngx_http_groonga_handler_data_t));
  cleanup->handler = ngx_http_groonga_handler_cleanup;
  data = cleanup->data;
  *data_return = data;

  context = &(data->context);
  grn_ctx_init(context, GRN_NO_FLAGS);
  GRN_TEXT_INIT(&(data->head), GRN_NO_FLAGS);
  GRN_TEXT_INIT(&(data->body), GRN_NO_FLAGS);
  GRN_TEXT_INIT(&(data->foot), GRN_NO_FLAGS);
  grn_ctx_use(context, grn_ctx_db(&(location_conf->context)));
  rc = ngx_http_groonga_context_check_error(r->connection->log, context);
  if (rc != NGX_OK) {
    return rc;
  }

  grn_ctx_recv_handler_set(context,
                           ngx_http_groonga_context_receive_handler,
                           data);

  return NGX_OK;
}

static ngx_int_t
ngx_http_groonga_handler_process_command_path(ngx_http_request_t *r,
                                              ngx_str_t *command_path,
                                              ngx_http_groonga_handler_data_t *data)
{
  grn_ctx *context;
  grn_obj uri;

  context = &(data->context);
  GRN_TEXT_INIT(&uri, 0);
  GRN_TEXT_PUTS(context, &uri, "/d/");
  GRN_TEXT_PUT(context, &uri, command_path->data, command_path->len);
  grn_ctx_send(context, GRN_TEXT_VALUE(&uri), GRN_TEXT_LEN(&uri),
               GRN_NO_FLAGS);
  ngx_http_groonga_context_log_error(r->connection->log, context);
  GRN_OBJ_FIN(context, &uri);

  return NGX_OK;
}

static ngx_int_t
ngx_http_groonga_handler_validate_post_command(ngx_http_request_t *r,
                                               ngx_str_t *command_path,
                                               ngx_http_groonga_handler_data_t *data)
{
  grn_ctx *context;
  ngx_str_t command;

  command.data = command_path->data;
  if (r->args.len == 0) {
    command.len = command_path->len;
  } else {
    command.len = command_path->len - r->args.len - strlen("?");
  }
  if (ngx_str_equal_c_string(&command, "load")) {
    return NGX_OK;
  }

  context = &(data->context);
  ngx_http_groonga_handler_set_content_type(r, "text/plain");
  GRN_TEXT_PUTS(context, &(data->body), "command for POST must be <load>: <");
  GRN_TEXT_PUT(context, &(data->body), command.data, command.len);
  GRN_TEXT_PUTS(context, &(data->body), ">");

  return NGX_HTTP_BAD_REQUEST;
}

static ngx_int_t
ngx_http_groonga_handler_process_body(ngx_http_request_t *r,
                                      ngx_http_groonga_handler_data_t *data)
{
  ngx_int_t rc;

  grn_ctx *context;

  ngx_buf_t *body;
  u_char *line_start, *current;

  context = &(data->context);

  body = r->request_body->buf;
  if (!body) {
    ngx_http_groonga_handler_set_content_type(r, "text/plain");
    GRN_TEXT_PUTS(context, &(data->body), "must send load data as body");
    return NGX_HTTP_BAD_REQUEST;
  }

  for (line_start = current = body->pos; current < body->last; current++) {
    if (*current != '\n') {
      continue;
    }

    grn_ctx_send(context, (const char *)line_start, current - line_start,
                 GRN_NO_FLAGS);
    rc = ngx_http_groonga_context_check_error(r->connection->log, context);
    if (rc != NGX_OK) {
      return rc;
    }
    line_start = current + 1;
  }
  if (line_start < current) {
    grn_ctx_send(context, (const char *)line_start, current - line_start,
                 GRN_NO_FLAGS);
    rc = ngx_http_groonga_context_check_error(r->connection->log, context);
    if (rc != NGX_OK) {
      return rc;
    }
  }

  return NGX_OK;
}


static ngx_int_t
ngx_http_groonga_handler_process_load(ngx_http_request_t *r,
                                      ngx_str_t *command_path,
                                      ngx_http_groonga_handler_data_t *data)
{
  ngx_int_t rc;

  rc = ngx_http_groonga_handler_validate_post_command(r, command_path, data);
  if (rc != NGX_OK) {
    return rc;
  }

  rc = ngx_http_groonga_handler_process_command_path(r, command_path, data);
  if (rc != NGX_OK) {
    return rc;
  }

  rc = ngx_http_groonga_handler_process_body(r, data);
  if (rc != NGX_OK) {
    return rc;
  }

  return NGX_OK;
}

static ngx_chain_t *
ngx_http_groonga_attach_chain(ngx_chain_t *chain, ngx_chain_t *new_chain)
{
  ngx_chain_t *last_chain;

  if (new_chain->buf->last == new_chain->buf->pos) {
    return chain;
  }

  new_chain->buf->last_buf = 1;
  new_chain->next = NULL;
  if (!chain) {
    return new_chain;
  }

  chain->buf->last_buf = 0;
  last_chain = chain;
  while (last_chain->next) {
    last_chain = last_chain->next;
  }
  last_chain->next = new_chain;
  return chain;
}

static ngx_int_t
ngx_http_groonga_handler_send_response(ngx_http_request_t *r,
                                       ngx_http_groonga_handler_data_t *data)
{
  ngx_int_t rc;
  grn_ctx *context;
  const char *content_type;
  ngx_buf_t *head_buf, *body_buf, *foot_buf;
  ngx_chain_t head_chain, body_chain, foot_chain;
  ngx_chain_t *output_chain = NULL;

  context = &(data->context);

  /* set the 'Content-type' header */
  if (r->headers_out.content_type.len == 0) {
    content_type = grn_ctx_get_mime_type(context);
    ngx_http_groonga_handler_set_content_type(r, content_type);
  }

  /* allocate buffers for a response body */
  head_buf = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &(data->head));
  if (!head_buf) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  body_buf = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &(data->body));
  if (!body_buf) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  foot_buf = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &(data->foot));
  if (!foot_buf) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  /* attach buffers to the buffer chain */
  head_chain.buf = head_buf;
  output_chain = ngx_http_groonga_attach_chain(output_chain, &head_chain);
  body_chain.buf = body_buf;
  output_chain = ngx_http_groonga_attach_chain(output_chain, &body_chain);
  foot_chain.buf = foot_buf;
  output_chain = ngx_http_groonga_attach_chain(output_chain, &foot_chain);

  /* set the status line */
  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_length_n = GRN_TEXT_LEN(&(data->head)) +
                                    GRN_TEXT_LEN(&(data->body)) +
                                    GRN_TEXT_LEN(&(data->foot));

  /* send the headers of your response */
  rc = ngx_http_send_header(r);

  if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
    return rc;
  }

  /* send the buffer chain of your response */
  rc = ngx_http_output_filter(r, output_chain);

  return rc;
}

static ngx_int_t
ngx_http_groonga_handler_get(ngx_http_request_t *r)
{
  ngx_int_t rc;
  ngx_str_t command_path;
  ngx_http_groonga_handler_data_t *data;

  rc = ngx_http_groonga_extract_command_path(r, &command_path);
  if (rc != NGX_OK) {
    return rc;
  }

  rc = ngx_http_groonga_handler_create_data(r, &data);
  if (rc != NGX_OK) {
    return rc;
  }

  rc = ngx_http_groonga_handler_process_command_path(r, &command_path, data);
  if (rc != NGX_OK) {
    return rc;
  }

  /* discard request body, since we don't need it here */
  rc = ngx_http_discard_request_body(r);
  if (rc != NGX_OK) {
    return rc;
  }

  rc = ngx_http_groonga_handler_send_response(r, data);

  return rc;
}

static void
ngx_http_groonga_handler_post(ngx_http_request_t *r)
{
  ngx_int_t rc;
  ngx_str_t command_path;
  ngx_http_groonga_handler_data_t *data;

  rc = ngx_http_groonga_extract_command_path(r, &command_path);
  if (rc == NGX_OK) {
    rc = ngx_http_groonga_handler_create_data(r, &data);
  }
  if (rc == NGX_OK) {
    rc = ngx_http_groonga_handler_process_load(r, &command_path, data);
  }

  ngx_http_groonga_handler_send_response(r, data);
  ngx_http_finalize_request(r, rc);
}

static ngx_int_t
ngx_http_groonga_handler(ngx_http_request_t *r)
{
  ngx_int_t rc;

  switch (r->method) {
  case NGX_HTTP_GET:
  case NGX_HTTP_HEAD:
    rc = ngx_http_groonga_handler_get(r);
    break;
  case NGX_HTTP_POST:
    rc = ngx_http_read_client_request_body(r, ngx_http_groonga_handler_post);
    if (rc < NGX_HTTP_SPECIAL_RESPONSE) {
      rc = NGX_DONE;
    }
    break;
  default:
    rc = NGX_HTTP_NOT_ALLOWED;
    break;
  }

  return rc;
}

static char *
ngx_http_groonga_conf_set_groonga_slot(ngx_conf_t *cf, ngx_command_t *cmd,
                                       void *conf)
{
  char *status;
  ngx_http_core_loc_conf_t *location_conf;
  ngx_http_groonga_loc_conf_t *groonga_location_conf = conf;

  status = ngx_conf_set_flag_slot(cf, cmd, conf);
  if (status != NGX_CONF_OK) {
    return status;
  }

  location_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  if (groonga_location_conf->enabled) {
    location_conf->handler = ngx_http_groonga_handler;
    groonga_location_conf->name =
      ngx_str_null_terminate(cf->pool, &(location_conf->name));
    groonga_location_conf->config_file =
      ngx_str_null_terminate(cf->pool, &(cf->conf_file->file.name));
    groonga_location_conf->config_line = cf->conf_file->line;
  } else {
    location_conf->handler = NULL;
  }

  return NGX_CONF_OK;
}

static void *
ngx_http_groonga_create_loc_conf(ngx_conf_t *cf)
{
  ngx_http_groonga_loc_conf_t *conf;
  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_groonga_loc_conf_t));
  if (conf == NULL) {
    return NGX_CONF_ERROR;
  }

  conf->enabled = NGX_CONF_UNSET;
  conf->database_path.data = NULL;
  conf->database_path.len = 0;
  conf->database_path_cstr = NULL;
  conf->database_auto_create = NGX_CONF_UNSET;
  conf->base_path.data = NULL;
  conf->base_path.len = 0;
  conf->config_file = NULL;
  conf->config_line = 0;

  return conf;
}

static char *
ngx_http_groonga_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  ngx_http_groonga_loc_conf_t *prev = parent;
  ngx_http_groonga_loc_conf_t *conf = child;

  ngx_conf_merge_str_value(conf->database_path, prev->database_path, NULL);

  return NGX_CONF_OK;
}

static void
ngx_http_groonga_each_loc_conf_in_tree(ngx_http_location_tree_node_t *node,
                                       ngx_http_groonga_loc_conf_callback_pt callback,
                                       void *user_data)
{
  if (!node) {
    return;
  }

  if (node->exact && node->exact->handler == ngx_http_groonga_handler) {
    callback(node->exact->loc_conf[ngx_http_groonga_module.ctx_index],
             user_data);
  }

  if (node->inclusive && node->inclusive->handler == ngx_http_groonga_handler) {
    callback(node->inclusive->loc_conf[ngx_http_groonga_module.ctx_index],
             user_data);
  }

  ngx_http_groonga_each_loc_conf_in_tree(node->left, callback, user_data);
  ngx_http_groonga_each_loc_conf_in_tree(node->right, callback, user_data);
  ngx_http_groonga_each_loc_conf_in_tree(node->tree, callback, user_data);
}

static void
ngx_http_groonga_each_loc_conf(ngx_http_conf_ctx_t *http_conf,
                               ngx_http_groonga_loc_conf_callback_pt callback,
                               void *user_data)
{
  ngx_http_core_main_conf_t *main_conf;
  ngx_http_core_srv_conf_t **server_confs;
  ngx_uint_t i;

  if (!http_conf) {
    return;
  }

  main_conf = http_conf->main_conf[ngx_http_core_module.ctx_index];
  server_confs = main_conf->servers.elts;
  for (i = 0; i < main_conf->servers.nelts; i++) {
    ngx_http_core_srv_conf_t *server_conf;
    ngx_http_core_loc_conf_t *location_conf;

    server_conf = server_confs[i];
    location_conf = server_conf->ctx->loc_conf[ngx_http_core_module.ctx_index];
    ngx_http_groonga_each_loc_conf_in_tree(location_conf->static_locations,
                                           callback,
                                           user_data);
  }
}

static ngx_int_t
ngx_http_groonga_mkdir_p(ngx_log_t *log, const char *dir_name)
{
  char sub_path[PATH_MAX];
  size_t i, dir_name_length;

  dir_name_length = strlen(dir_name);
  sub_path[0] = dir_name[0];
  for (i = 1; i < dir_name_length + 1; i++) {
    if (dir_name[i] == '/' || dir_name[i] == '\0') {
      struct stat stat_buffer;
      sub_path[i] = '\0';
      if (stat(sub_path, &stat_buffer) == -1) {
        if (ngx_create_dir(sub_path, 0700) == -1) {
          ngx_log_error(NGX_LOG_EMERG, log, 0,
                        "failed to create directory: %s (%s): %s",
                        sub_path, dir_name,
                        strerror(errno));
          return NGX_ERROR;
        }
      }
    }
    sub_path[i] = dir_name[i];
  }

  return NGX_OK;
}

static void
ngx_http_groonga_create_database(ngx_http_groonga_loc_conf_t *location_conf,
                                 ngx_http_groonga_database_callback_data_t *data)
{
  const char *database_base_name;
  grn_ctx *context;

  database_base_name = strrchr(location_conf->database_path_cstr, '/');
  if (database_base_name) {
    char database_dir[PATH_MAX];
    database_dir[0] = '\0';
    strncat(database_dir,
            location_conf->database_path_cstr,
            database_base_name - location_conf->database_path_cstr);
    data->rc = ngx_http_groonga_mkdir_p(data->log, database_dir);
    if (data->rc != NGX_OK) {
      return;
    }
  }

  context = &(location_conf->context);
  grn_db_create(context, location_conf->database_path_cstr, NULL);
  if (context->rc == GRN_SUCCESS) {
    return;
  }

  ngx_log_error(NGX_LOG_EMERG, data->log, 0,
                "failed to create groonga database: %s",
                context->errbuf);
  data->rc = NGX_ERROR;
}

static void
ngx_http_groonga_open_database_callback(ngx_http_groonga_loc_conf_t *location_conf,
                                        void *user_data)
{
  ngx_http_groonga_database_callback_data_t *data = user_data;
  grn_ctx *context;

  context = &(location_conf->context);
  grn_ctx_init(context, GRN_NO_FLAGS);

  if (!location_conf->database_path.data) {
    ngx_log_error(NGX_LOG_EMERG, data->log, 0,
                  "%s: \"groonga_database\" must be specified in block at %s:%d",
                  location_conf->name,
                  location_conf->config_file,
                  location_conf->config_line);
    data->rc = NGX_ERROR;
    return;
  }

  if (!location_conf->database_path_cstr) {
    location_conf->database_path_cstr =
      ngx_str_null_terminate(data->pool, &(location_conf->database_path));
  }

  grn_db_open(context, location_conf->database_path_cstr);
  if (context->rc == GRN_SUCCESS) {
    return;
  }

  if (location_conf->database_auto_create) {
    ngx_http_groonga_create_database(location_conf, data);
  } else {
    ngx_log_error(NGX_LOG_EMERG, data->log, 0,
                  "failed to open groonga database: %s",
                  context->errbuf);
    data->rc = NGX_ERROR;
  }
}

static void
ngx_http_groonga_close_database_callback(ngx_http_groonga_loc_conf_t *location_conf,
                                         void *user_data)
{
  ngx_http_groonga_database_callback_data_t *data = user_data;
  grn_ctx *context;

  context = &(location_conf->context);

  grn_obj_close(context, grn_ctx_db(context));
  ngx_http_groonga_context_log_error(data->log, context);
  grn_ctx_fin(context);
}

static ngx_int_t
ngx_http_groonga_init_process(ngx_cycle_t *cycle)
{
  grn_rc rc;
  ngx_http_conf_ctx_t *http_conf;
  ngx_http_groonga_database_callback_data_t data;

  rc = grn_init();
  if (rc != GRN_SUCCESS) {
    return NGX_ERROR;
  }

  http_conf =
    (ngx_http_conf_ctx_t *)ngx_get_conf(cycle->conf_ctx, ngx_http_module);
  data.log = cycle->log;
  data.pool = cycle->pool;
  data.rc = NGX_OK;
  ngx_http_groonga_each_loc_conf(http_conf,
                                 ngx_http_groonga_open_database_callback,
                                 &data);

  return data.rc;
}

static void
ngx_http_groonga_exit_process(ngx_cycle_t *cycle)
{
  ngx_http_conf_ctx_t *http_conf;
  ngx_http_groonga_database_callback_data_t data;

  http_conf =
    (ngx_http_conf_ctx_t *)ngx_get_conf(cycle->conf_ctx, ngx_http_module);
  data.log = cycle->log;
  data.pool = cycle->pool;
  ngx_http_groonga_each_loc_conf(http_conf,
                                 ngx_http_groonga_close_database_callback,
                                 &data);

  grn_fin();

  return;
}

/* entry point */
static ngx_command_t ngx_http_groonga_commands[] = {
  { ngx_string("groonga"),
    NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_http_groonga_conf_set_groonga_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_groonga_loc_conf_t, enabled),
    NULL },

  { ngx_string("groonga_database"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_groonga_loc_conf_t, database_path),
    NULL },

  { ngx_string("groonga_database_auto_create"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_groonga_loc_conf_t, database_auto_create),
    NULL },

  { ngx_string("groonga_base_path"),
    NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_groonga_loc_conf_t, base_path),
    NULL },

  ngx_null_command
};

static ngx_http_module_t ngx_http_groonga_module_ctx = {
  NULL, /* preconfiguration */
  NULL, /* postconfiguration */

  NULL, /* create main configuration */
  NULL, /* init main configuration */

  NULL, /* create server configuration */
  NULL, /* merge server configuration */

  ngx_http_groonga_create_loc_conf, /* create location configuration */
  ngx_http_groonga_merge_loc_conf, /* merge location configuration */
};

ngx_module_t ngx_http_groonga_module = {
  NGX_MODULE_V1,
  &ngx_http_groonga_module_ctx, /* module context */
  ngx_http_groonga_commands, /* module directives */
  NGX_HTTP_MODULE, /* module type */
  NULL, /* init master */
  NULL, /* init module */
  ngx_http_groonga_init_process, /* init process */
  NULL, /* init thread */
  NULL, /* exit thread */
  ngx_http_groonga_exit_process, /* exit process */
  NULL, /* exit master */
  NGX_MODULE_V1_PADDING
};
