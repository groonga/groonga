/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn_ctx.h"
#include "grn_http_client.h"

#ifdef GRN_WITH_CURL
#  include <curl/curl.h>
#endif

#include <stdio.h>

struct _grn_http_client {
  grn_obj_header header;

  /* This is just for grn_http_client_write() */
  grn_ctx *ctx;

#ifdef GRN_WITH_CURL
  CURL *curl;
  struct curl_slist *headers;
#endif
  grn_obj url;
  grn_obj output_buffer;
  grn_obj output_path;
  FILE *output_file;
};

#ifdef GRN_WITH_CURL
static inline grn_rc
grn_curl_code_to_rc(CURLcode code)
{
  /* TODO: Add more codes: https://curl.se/libcurl/c/libcurl-errors.html */
  switch (code) {
  case CURLE_OK:
    return GRN_SUCCESS;
  case CURLE_UNSUPPORTED_PROTOCOL:
    return GRN_OPERATION_NOT_SUPPORTED;
  case CURLE_URL_MALFORMAT:
    return GRN_INVALID_ARGUMENT;
  default:
    return GRN_UNKNOWN_ERROR;
  }
}

static size_t
grn_http_client_write(char *data,
                      size_t size, /* size is always 1 */
                      size_t n_elements,
                      void *user_data)
{
  grn_http_client *client = user_data;

  if (client->output_file) {
    return fwrite(data, size, n_elements, client->output_file);
  } else {
    GRN_TEXT_PUT(client->ctx, &(client->output_buffer), data, size * n_elements);
    return size * n_elements;
  }
}
#endif

grn_http_client *
grn_http_client_open(grn_ctx *ctx)
{
  const char *tag = "[http-client][open]";

  GRN_API_ENTER;

  grn_http_client *client = GRN_CALLOC(sizeof(grn_http_client));
  if (!client) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc, "%s failed to allocate memory: %s", tag, errbuf);
    GRN_API_RETURN(NULL);
  }

  client->header.type = GRN_HTTP_CLIENT;
  client->header.impl_flags = 0;
  client->header.flags = 0;
  client->header.domain = GRN_ID_NIL;

  client->ctx = ctx;
#ifdef GRN_WITH_CURL
  client->curl = curl_easy_init();
  if (!client->curl) {
    ERR(GRN_UNKNOWN_ERROR, "%s failed to initialize curl", tag);
    GRN_FREE(client);
    GRN_API_RETURN(NULL);
  }
  curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(client->curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(client->curl, CURLOPT_USERAGENT, "Groonga/" VERSION);
  curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, client);
  curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, grn_http_client_write);
  /* We can debug HTTP request/response by this. */
  /* curl_easy_setopt(client->curl, CURLOPT_VERBOSE, 1L); */
  client->headers = NULL;
#endif
  GRN_TEXT_INIT(&(client->url), 0);
  GRN_TEXT_INIT(&(client->output_buffer), 0);
  GRN_TEXT_INIT(&(client->output_path), 0);
  client->output_file = NULL;

  GRN_API_RETURN(client);
}

grn_rc
grn_http_client_close(grn_ctx *ctx, grn_http_client *client)
{
  GRN_API_ENTER;

  if (!client) {
    GRN_API_RETURN(ctx->rc);
  }

#ifdef GRN_WITH_CURL
  curl_easy_cleanup(client->curl);
  curl_slist_free_all(client->headers);
#endif
  GRN_OBJ_FIN(ctx, &(client->url));
  GRN_OBJ_FIN(ctx, &(client->output_buffer));
  GRN_OBJ_FIN(ctx, &(client->output_path));
  if (client->output_file) {
    fclose(client->output_file);
  }

  GRN_FREE(client);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_http_client_set_user_agent(grn_ctx *ctx,
                               grn_http_client *client,
                               const char *user_agent)
{
  GRN_API_ENTER;

#ifdef GRN_WITH_CURL
  curl_easy_setopt(client->curl, CURLOPT_USERAGENT, user_agent);
#endif

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_http_client_set_url(grn_ctx *ctx,
                        grn_http_client *client,
                        const char *url)
{
  GRN_API_ENTER;

#ifdef GRN_WITH_CURL
  curl_easy_setopt(client->curl, CURLOPT_URL, url);
#endif
  GRN_TEXT_SETS(ctx, &(client->url), url);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_http_client_add_header(grn_ctx *ctx,
                           grn_http_client *client,
                           const char *header)
{
  GRN_API_ENTER;

#ifdef GRN_WITH_CURL
  client->headers = curl_slist_append(client->headers, header);
  curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers);
#endif

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_http_client_set_output_path(grn_ctx *ctx,
                                grn_http_client *client,
                                const char *path)
{
  GRN_API_ENTER;

  if (path) {
    GRN_TEXT_SETS(ctx, &(client->output_path), path);
  } else {
    GRN_BULK_REWIND(&(client->output_path));
  }
  if (GRN_TEXT_LEN(&(client->output_path)) > 0) {
    GRN_TEXT_PUTC(ctx, &(client->output_path), '\0');
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_http_client_download(grn_ctx *ctx, grn_http_client *client)
{
  const char *tag = "[http-client][download]";

  GRN_API_ENTER;

#ifdef GRN_WITH_CURL
  GRN_BULK_REWIND(&(client->output_buffer));
  if (client->output_file) {
    fclose(client->output_file);
    client->output_file = NULL;
  }
  if (GRN_TEXT_LEN(&(client->output_path)) > 0) {
    /* TODO: We must use "wb" on Windows but we will use Windows
     * native HTTP API instead of curl on Windows. */
    client->output_file = fopen(GRN_TEXT_VALUE(&(client->output_path)), "w");
  }

  CURLcode code = curl_easy_perform(client->curl);
  if (code != CURLE_OK) {
    ERR(grn_curl_code_to_rc(code),
        "%s failed: <%.*s>: %s",
        tag,
        (int)GRN_TEXT_LEN(&(client->url)),
        GRN_TEXT_VALUE(&(client->url)),
        curl_easy_strerror(code));
    goto exit;
  }

  long response_code;
  curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response_code);
  if (response_code != 200) {
    ERR(grn_curl_code_to_rc(code),
        "%s failed: <%.*s>: <%d>: <%.*s>",
        tag,
        (int)GRN_TEXT_LEN(&(client->url)),
        GRN_TEXT_VALUE(&(client->url)),
        (int)response_code,
        (int)GRN_TEXT_LEN(&(client->output_buffer)),
        GRN_TEXT_VALUE(&(client->output_buffer)));
    goto exit;
  }

exit:
  if (client->output_file) {
    fclose(client->output_file);
    client->output_file = NULL;
  }
#else
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s curl isn't enabled", tag);
#endif

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_http_client_get_output(grn_ctx *ctx, grn_http_client *client)
{
  return &(client->output_buffer);
}
