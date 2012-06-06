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

typedef struct {
  ngx_str_t database;
  char *database_cstr;
  grn_ctx *global_context;
} ngx_http_groonga_loc_conf_t;

static char *ngx_http_groonga(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_http_groonga_commands[] = {
  { ngx_string("groonga"),
    NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
    ngx_http_groonga,
    0,
    0,
    NULL },

  { ngx_string("groonga_database"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_groonga_loc_conf_t, database),
    NULL },

  ngx_null_command
};

static const char content_type[] = "application/json";

static void *ngx_http_groonga_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_groonga_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_http_module_t ngx_http_groonga_module_ctx = {
  NULL,                          /* preconfiguration */
  NULL,                          /* postconfiguration */

  NULL,                          /* create main configuration */
  NULL,                          /* init main configuration */

  NULL,                          /* create server configuration */
  NULL,                          /* merge server configuration */

  ngx_http_groonga_create_loc_conf, /* create location configuration */
  ngx_http_groonga_merge_loc_conf,  /* merge location configuration */
};

static ngx_int_t ngx_http_groonga_init_module(ngx_cycle_t *cycle);
static void ngx_http_groonga_exit_master(ngx_cycle_t *cycle);

ngx_module_t ngx_http_groonga_module = {
  NGX_MODULE_V1,
  &ngx_http_groonga_module_ctx,  /* module context */
  ngx_http_groonga_commands,     /* module directives */
  NGX_HTTP_MODULE,               /* module type */
  NULL,                          /* init master */
  ngx_http_groonga_init_module,  /* init module */
  NULL,                          /* init process */
  NULL,                          /* init thread */
  NULL,                          /* exit thread */
  NULL,                          /* exit process */
  ngx_http_groonga_exit_master,  /* exit master */
  NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_groonga_context_check(grn_ctx *context);

static char *
ngx_str_null_terminate(const ngx_str_t *string) {
  char *result = malloc(string->len + 1);
  memcpy(result, string->data, string->len);
  result[string->len] = '\0';
  return result;
}

static ngx_int_t
ngx_http_groonga_context_receive(grn_ctx *context, char **result, unsigned int *result_size)
{
  ngx_int_t rc;

  int flags = 0;
  grn_ctx_recv(context, result, result_size, &flags);
  rc = ngx_http_groonga_context_check(context);
  return rc;
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

static ngx_int_t
ngx_http_groonga_handler(ngx_http_request_t *r)
{
  static const int no_flags = 0;

  ngx_int_t    rc;
  ngx_buf_t   *b;
  ngx_chain_t  out;

  grn_ctx context_;
  grn_ctx *context = &context_;
  char *result = NULL;
  unsigned int result_size = 0;
  unsigned char *body_data;

  grn_obj body;

  ngx_http_groonga_loc_conf_t *loc_conf;
  loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_groonga_module);

  if (!loc_conf->global_context) {
    context = malloc(sizeof(grn_ctx));

    grn_ctx_init(context, no_flags);

    if (!loc_conf->database_cstr) {
      loc_conf->database_cstr = ngx_str_null_terminate(&loc_conf->database);
    }

    grn_db_open(context, loc_conf->database_cstr);
    rc = ngx_http_groonga_context_check(context);
    if (rc != NGX_OK) {
      return rc;
    }
    loc_conf->global_context = context;
  }

  context = loc_conf->global_context;

  printf("database_path: %s\n", loc_conf->database_cstr);
  printf("version: %s\n", grn_get_version());
  printf("uri: %.*s\n", (int)r->unparsed_uri.len, r->unparsed_uri.data);

  grn_ctx_send(context, (char *)r->unparsed_uri.data, r->unparsed_uri.len, no_flags);
  rc = ngx_http_groonga_context_check(context);
  if (rc != NGX_OK) {
    return rc;
  }

  rc = ngx_http_groonga_context_receive(context, &result, &result_size);
  if (rc != NGX_OK) {
    return rc;
  }

  body_data = ngx_pcalloc(r->pool, result_size);
  if (body_data == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
  ngx_memcpy(body_data, result, result_size);

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
  r->headers_out.content_type_len = sizeof(content_type) - 1;
  r->headers_out.content_type.len = sizeof(content_type) - 1;
  r->headers_out.content_type.data = (u_char *) content_type;

  /* allocate a buffer for your response body */
  GRN_TEXT_SET(context, &body, body_data, result_size);
  b = ngx_http_groonga_grn_obj_to_ngx_buf(r->pool, &body);
  if (b == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
  b->last_buf = 1;  /* this is the last buffer in the buffer chain */

  /* attach this buffer to the buffer chain */
  out.buf = b;
  out.next = NULL;

  /* set the status line */
  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_length_n = result_size;

  /* send the headers of your response */
  rc = ngx_http_send_header(r);

  if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
    return rc;
  }

  /* send the buffer chain of your response */
  return ngx_http_output_filter(r, &out);
}

static char *
ngx_http_groonga(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_http_core_loc_conf_t *location_conf;

  location_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  /* handler to process the 'groonga' directive */
  location_conf->handler = ngx_http_groonga_handler;

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

  conf->database.data = NULL;
  conf->database.len = 0;
  conf->database_cstr = NULL;
  conf->global_context = NULL;

  return conf;
}

static char *
ngx_http_groonga_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  ngx_http_groonga_loc_conf_t *prev = parent;
  ngx_http_groonga_loc_conf_t *conf = child;

  ngx_conf_merge_str_value(conf->database, prev->database, NULL);

  return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_groonga_init_module(ngx_cycle_t *cycle)
{
  grn_rc rc;

  printf("grn_init() is called\n");
  rc = grn_init();
  if (rc) {
    printf("grn_init() failed..\n");
    return NGX_ERROR;
  }

  return NGX_OK;
}

static void
ngx_http_groonga_exit_master(ngx_cycle_t *cycle)
{
  grn_rc rc;

  printf("grn_fin() is called\n");
  rc = grn_fin();
  if (rc) {
    printf("grn_fin() failed..\n");
    // there is nothing we can at this situation...
  }

  return;
}

static ngx_int_t
ngx_http_groonga_context_check(grn_ctx *context) {
  if (context->rc == GRN_SUCCESS) {
    return NGX_OK;
  } else {
    /* TODO: proper error handling */
    printf("%s\n", context->errbuf);
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
}
