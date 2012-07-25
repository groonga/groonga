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

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <groonga.h>

#define GRN_NO_FLAGS 0

typedef struct {
  ngx_flag_t enabled;
  ngx_str_t database_path;
  char *database_path_cstr;
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

static void
ngx_http_groonga_log_context_error(ngx_log_t *log, grn_ctx *context)
{
  if (context->rc == GRN_SUCCESS) {
    return;
  }

  ngx_log_error(NGX_LOG_ERR, log, 0, "%s", context->errbuf);
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
ngx_http_groonga_handler(ngx_http_request_t *r)
{
  ngx_int_t    rc;
  ngx_buf_t   *head_buf, *body_buf, *foot_buf;
  ngx_chain_t  head_chain, body_chain, foot_chain;

  ngx_http_cleanup_t *cleanup;
  ngx_http_groonga_handler_data_t *data;
  const char *content_type;

  grn_ctx *context;
  grn_obj uri;
  u_char *unparsed_path;
  ngx_int_t unparsed_path_length;

  ngx_http_core_loc_conf_t *http_location_conf;
  ngx_http_groonga_loc_conf_t *location_conf;

  http_location_conf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
  location_conf = ngx_http_get_module_loc_conf(r, ngx_http_groonga_module);

  unparsed_path = r->unparsed_uri.data + http_location_conf->name.len;
  unparsed_path_length = r->unparsed_uri.len - http_location_conf->name.len;
  if (unparsed_path_length > 0 && unparsed_path[0] == '/') {
    unparsed_path += 1;
    unparsed_path_length -= 1;
  }
  if (unparsed_path_length == 0) {
    return NGX_HTTP_BAD_REQUEST;
  }

  cleanup = ngx_http_cleanup_add(r, sizeof(ngx_http_groonga_handler_data_t));
  cleanup->handler = ngx_http_groonga_handler_cleanup;
  data = cleanup->data;

  context = &(data->context);
  grn_ctx_init(context, GRN_NO_FLAGS);
  grn_ctx_use(context, grn_ctx_db(&(location_conf->context)));
  GRN_TEXT_INIT(&(data->head), GRN_NO_FLAGS);
  GRN_TEXT_INIT(&(data->body), GRN_NO_FLAGS);
  GRN_TEXT_INIT(&(data->foot), GRN_NO_FLAGS);

  grn_ctx_recv_handler_set(context,
                           ngx_http_groonga_context_receive_handler,
                           data);
  GRN_TEXT_INIT(&uri, 0);
  GRN_TEXT_PUTS(context, &uri, "/d/");
  GRN_TEXT_PUT(context, &uri, unparsed_path, unparsed_path_length);
  grn_ctx_send(context, GRN_TEXT_VALUE(&uri), GRN_TEXT_LEN(&uri), GRN_NO_FLAGS);
  GRN_OBJ_FIN(context, &uri);

  /* we response to 'GET' and 'HEAD' requests only */
  if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
    return NGX_HTTP_NOT_ALLOWED;
  }

  /* discard request body, since we don't need it here */
  rc = ngx_http_discard_request_body(r);

  if (rc != NGX_OK) {
    return rc;
  }

  /* set the 'Content-type' header */
  content_type = grn_ctx_get_mime_type(context);
  r->headers_out.content_type.len = strlen(content_type);
  r->headers_out.content_type.data = (u_char *) content_type;

  /* allocate buffers for a response body */
  head_buf = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &(data->head));
  if (head_buf == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  body_buf = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &(data->body));
  if (body_buf == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  foot_buf = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &(data->foot));
  if (foot_buf == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
  foot_buf->last_buf = 1;  /* this is the last buffer in the buffer chain */

  /* attach buffers to the buffer chain */
  head_chain.buf = head_buf;
  head_chain.next = &body_chain;
  body_chain.buf = body_buf;
  body_chain.next = &foot_chain;
  foot_chain.buf = foot_buf;
  foot_chain.next = NULL;

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
  rc = ngx_http_output_filter(r, &head_chain);

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
  if (context->rc != GRN_SUCCESS) {
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
  ngx_http_groonga_log_context_error(data->log, context);
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
  grn_rc rc;
  ngx_http_conf_ctx_t *http_conf;
  ngx_http_groonga_database_callback_data_t data;

  http_conf =
    (ngx_http_conf_ctx_t *)ngx_get_conf(cycle->conf_ctx, ngx_http_module);
  data.log = cycle->log;
  data.pool = cycle->pool;
  ngx_http_groonga_each_loc_conf(http_conf,
                                 ngx_http_groonga_close_database_callback,
                                 &data);

  rc = grn_fin();
  if (rc != GRN_SUCCESS) {
    /* there is nothing we can at this situation... */
  }

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
