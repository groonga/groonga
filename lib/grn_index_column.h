/*
  Copyright (C) 2015-2016  Brazil
  Copyright (C) 2019-2025  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

void
grn_index_column_init_from_env(void);
grn_rc
grn_index_column_build(grn_ctx *ctx, grn_obj *index_column);
grn_rc
grn_index_column_rebuild(grn_ctx *ctx, grn_obj *index_column);
grn_obj *
grn_index_column_get_tokenizer(grn_ctx *ctx, grn_obj *index_column);

struct grn_index_column_diff_options {
  grn_log_level progress_log_level;
};

grn_rc
grn_index_column_diff_options_init(grn_ctx *ctx,
                                   grn_index_column_diff_options *options);
grn_rc
grn_index_column_diff_options_fin(grn_ctx *ctx,
                                  grn_index_column_diff_options *options);

#ifdef __cplusplus
}
#endif
