/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Brazil

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

#include "grn_options.h"
#include "grn_db.h"
#include "grn_util.h"

#include <stdio.h>

struct _grn_options {
  grn_ja *values;
};

static const char *GRN_OPTIONS_PATH_FORMAT = "%s.options";
static const unsigned int GRN_OPTIONS_MAX_VALUE_SIZE = 65536;

grn_options *
grn_options_create(grn_ctx *ctx,
                   const char *path,
                   const char *context_tag)
{
  char *options_path;
  char options_path_buffer[PATH_MAX];
  grn_options *options;
  uint32_t flags = 0;

  if (path) {
    grn_snprintf(options_path_buffer,
                 PATH_MAX,
                 PATH_MAX,
                 GRN_OPTIONS_PATH_FORMAT,
                 path);
    options_path = options_path_buffer;
  } else {
    options_path = NULL;
  }

  options = GRN_MALLOC(sizeof(grn_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options: <%s>",
        context_tag,
        options_path ? options_path : "(temporary)");
    return NULL;
  }

  options->values = grn_ja_create(ctx,
                                  options_path,
                                  GRN_OPTIONS_MAX_VALUE_SIZE,
                                  flags);
  if (!options->values) {
    GRN_FREE(options);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to create data store for options: <%s>",
        context_tag,
        options_path ? options_path : "(temporary)");
    return NULL;
  }

  return options;
}

grn_options *
grn_options_open(grn_ctx *ctx,
                 const char *path,
                 const char *context_tag)
{
  char options_path[PATH_MAX];
  grn_options *options;

  grn_snprintf(options_path,
               PATH_MAX,
               PATH_MAX,
               GRN_OPTIONS_PATH_FORMAT,
               path);
  if (!grn_path_exist(options_path)) {
    return grn_options_create(ctx, path, context_tag);
  }

  options = GRN_MALLOC(sizeof(grn_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options: <%s>",
        context_tag,
        options_path);
    return NULL;
  }

  options->values = grn_ja_open(ctx, options_path);
  if (!options->values) {
    GRN_FREE(options);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to open data store for options: <%s>",
        context_tag,
        options_path);
    return NULL;
  }

  return options;
}

grn_rc
grn_options_close(grn_ctx *ctx, grn_options *options)
{
  grn_rc rc;

  if (!options) {
    return GRN_SUCCESS;
  }

  rc = grn_ja_close(ctx, options->values);
  GRN_FREE(options);

  return rc;
}

grn_rc
grn_options_remove(grn_ctx *ctx, const char *path)
{
  char options_path[PATH_MAX];

  grn_snprintf(options_path,
               PATH_MAX,
               PATH_MAX,
               GRN_OPTIONS_PATH_FORMAT,
               path);
  return grn_ja_remove(ctx, options_path);
}

grn_bool
grn_options_is_locked(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_is_locked(ctx, (grn_obj *)(options->values));
}

grn_rc
grn_options_clear_lock(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_clear_lock(ctx, (grn_obj *)(options->values));
}

grn_bool
grn_options_is_corrupt(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_is_corrupt(ctx, (grn_obj *)(options->values));
}

grn_rc
grn_options_flush(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_flush(ctx, (grn_obj *)(options->values));
}

grn_rc
grn_options_set(grn_ctx *ctx,
                grn_options *options,
                grn_id id,
                grn_obj *values)
{
  return grn_ja_putv(ctx, options->values, id, values, 0);
}

grn_bool
grn_options_get(grn_ctx *ctx,
                grn_options *options,
                grn_id id,
                grn_obj *value)
{
  grn_io_win iw;
  void *raw_value;
  uint32_t length;

  raw_value = grn_ja_ref(ctx, options->values, id, &iw, &length);
  if (!raw_value) {
    return GRN_FALSE;
  }

  if (value->header.type != GRN_VECTOR) {
    grn_obj_reinit(ctx, value, value->header.domain, GRN_OBJ_VECTOR);
  }
  grn_vector_decode(ctx, value, raw_value, length);

  grn_ja_unref(ctx, &iw);

  return GRN_TRUE;
}
