/*
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#include "grn_token_metadata.h"

void
grn_token_metadata_init(grn_ctx *ctx,
                        grn_obj *metadata)
{
  GRN_TEXT_INIT(metadata, GRN_OBJ_VECTOR);
}

void
grn_token_metadata_fin(grn_ctx *ctx,
                       grn_obj *metadata)
{
  GRN_OBJ_FIN(ctx, metadata);
}

void
grn_token_metadata_reset(grn_ctx *ctx,
                         grn_obj *metadata)
{
  GRN_BULK_REWIND(metadata);
}

void
grn_token_metadata_copy(grn_ctx *ctx,
                        grn_obj *metadata,
                        grn_obj *source)
{
  int i;
  int n;

  n = grn_vector_size(ctx, source);
  for (i = 0; i < n; i++) {
    const char *value;
    unsigned int value_length;
    int domain;
    value_length = grn_vector_get_element(ctx, source, i, &value, NULL, &domain);
    grn_vector_add_element(ctx,
                           metadata,
                           value,
                           value_length,
                           0,
                           domain);
  }
}

size_t
grn_token_metadata_get_size(grn_ctx *ctx,
                            grn_obj *metadata)
{
  size_t size;
  GRN_API_ENTER;
  if (!metadata) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][metadata][get][size] token metadata must not be NULL");
    GRN_API_RETURN(0);
  }
  size = grn_vector_size(ctx, metadata) / 2;
  GRN_API_RETURN(size);
}

grn_rc
grn_token_metadata_at(grn_ctx *ctx,
                      grn_obj *metadata,
                      size_t i,
                      grn_obj *name,
                      grn_obj *value)
{
  size_t n;
  const char *raw_name;
  unsigned int raw_name_length;
  grn_id name_domain;
  const char *raw_value;
  unsigned int raw_value_length;
  grn_id value_domain;

  GRN_API_ENTER;
  if (!metadata) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][metadata][at] token metadata must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }

  n = grn_vector_size(ctx, metadata) / 2;
  if (i >= n) {
    GRN_BULK_REWIND(name);
    GRN_BULK_REWIND(value);
    GRN_API_RETURN(GRN_SUCCESS);
  }

  raw_name_length = grn_vector_get_element(ctx,
                                           metadata,
                                           i * 2,
                                           &raw_name,
                                           NULL,
                                           &name_domain);
  grn_obj_reinit(ctx, name, name_domain, 0);
  grn_bulk_write(ctx, name, raw_name, raw_name_length);

  raw_value_length = grn_vector_get_element(ctx,
                                            metadata,
                                            i * 2 + 1,
                                            &raw_value,
                                            NULL,
                                            &value_domain);
  grn_obj_reinit(ctx, value, value_domain, 0);
  grn_bulk_write(ctx, value, raw_value, raw_value_length);

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_token_metadata_get(grn_ctx *ctx,
                       grn_obj *metadata,
                       const char *name,
                       int name_length,
                       grn_obj *value)
{
  size_t i;
  size_t n;

  GRN_API_ENTER;
  if (!metadata) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][metadata][get] token metadata must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (name_length < 0) {
    name_length = strlen(name);
  }

  n = grn_vector_size(ctx, metadata) / 2;
  for (i = 0; i < n; i++) {
    const char *current_name;
    unsigned int current_name_length;

    current_name_length = grn_vector_get_element(ctx,
                                                 metadata,
                                                 i * 2,
                                                 &current_name,
                                                 NULL,
                                                 NULL);
    if (name_length == current_name_length &&
        memcmp(name, current_name, name_length) == 0) {
      const char *raw_value;
      unsigned int raw_value_length;
      grn_id domain;

      raw_value_length = grn_vector_get_element(ctx,
                                                metadata,
                                                i * 2 + 1,
                                                &raw_value,
                                                NULL,
                                                &domain);
      grn_obj_reinit(ctx, value, domain, 0);
      grn_bulk_write(ctx, value, raw_value, raw_value_length);
      GRN_API_RETURN(GRN_SUCCESS);
    }
  }
  GRN_BULK_REWIND(value);

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_token_metadata_add(grn_ctx *ctx,
                       grn_obj *metadata,
                       const char *name,
                       int name_length,
                       grn_obj *value)
{
  GRN_API_ENTER;
  if (!metadata) {
    ERR(GRN_INVALID_ARGUMENT,
        "[token][metadata][add] token metadata must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (name_length < 0) {
    name_length = strlen(name);
  }

  grn_vector_add_element(ctx, metadata, name, name_length, 0, GRN_DB_TEXT);
  if (ctx->rc == GRN_SUCCESS) {
    grn_vector_add_element(ctx,
                           metadata,
                           GRN_BULK_HEAD(value),
                           GRN_BULK_VSIZE(value),
                           0,
                           value->header.domain);
  }

  GRN_API_RETURN(ctx->rc);
}
