/*
  Copyright(C) 2012-2016 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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
#include "grn_ctx.h"
#include "grn_db.h"
#include "grn_nfkc.h"
#include "grn_string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_NORMALIZER_AUTO_NAME "NormalizerAuto"

grn_rc grn_normalizer_init(void);
grn_rc grn_normalizer_fin(void);

grn_rc grn_normalizer_normalize(grn_ctx *ctx,
                                grn_obj *normalizer,
                                grn_obj *string);

grn_rc grn_nfkc_normalize(grn_ctx *ctx,
                          grn_obj *string,
                          grn_nfkc_normalize_options *options);

grn_rc grn_db_init_builtin_normalizers(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif
