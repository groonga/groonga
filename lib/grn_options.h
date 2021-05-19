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

#pragma once

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_options grn_options;

grn_options *grn_options_create(grn_ctx *ctx,
                                const char *path,
                                const char *context_tag);
grn_options *grn_options_open(grn_ctx *ctx,
                              const char *path,
                              const char *context_tag);
grn_rc grn_options_close(grn_ctx *ctx, grn_options *options);
grn_rc grn_options_remove(grn_ctx *ctx, const char *path);

grn_bool grn_options_is_locked(grn_ctx *ctx, grn_options *options);
grn_rc grn_options_clear_lock(grn_ctx *ctx, grn_options *options);
grn_bool grn_options_is_corrupt(grn_ctx *ctx, grn_options *options);
grn_rc grn_options_flush(grn_ctx *ctx, grn_options *options);

grn_rc grn_options_set(grn_ctx *ctx,
                       grn_options *options,
                       grn_id id,
                       const char *name,
                       int name_length,
                       grn_obj *values);
grn_option_revision grn_options_get(grn_ctx *ctx,
                                    grn_options *options,
                                    grn_id id,
                                    const char *name,
                                    int name_length,
                                    grn_option_revision revision,
                                    grn_obj *values);
grn_rc grn_options_clear(grn_ctx *ctx,
                         grn_options *options,
                         grn_id id);

#ifdef __cplusplus
}
#endif
