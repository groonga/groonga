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

#pragma once

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_http_client grn_http_client;

grn_http_client *
grn_http_client_open(grn_ctx *ctx);
grn_rc
grn_http_client_close(grn_ctx *ctx, grn_http_client *http_client);
grn_rc
grn_http_client_set_user_agent(grn_ctx *ctx,
                               grn_http_client *client,
                               const char *user_agent);
grn_rc
grn_http_client_set_url(grn_ctx *ctx,
                        grn_http_client *client,
                        const char *url);
grn_rc
grn_http_client_add_header(grn_ctx *ctx,
                           grn_http_client *client,
                           const char *header);
grn_rc
grn_http_client_set_output_path(grn_ctx *ctx,
                                grn_http_client *client,
                                const char *path);
grn_rc
grn_http_client_download(grn_ctx *ctx,
                         grn_http_client *http_client);
grn_obj *
grn_http_client_get_output(grn_ctx *ctx,
                           grn_http_client *http_client);

#ifdef __cplusplus
}
#endif
