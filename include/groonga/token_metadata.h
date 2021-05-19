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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

GRN_API size_t
grn_token_metadata_get_size(grn_ctx *ctx,
                            grn_obj *metadata);
GRN_API grn_rc
grn_token_metadata_at(grn_ctx *ctx,
                      grn_obj *metadata,
                      size_t i,
                      grn_obj *name,
                      grn_obj *value);
GRN_API grn_rc
grn_token_metadata_get(grn_ctx *ctx,
                       grn_obj *metadata,
                       const char *name,
                       int name_length,
                       grn_obj *value);
GRN_API grn_rc
grn_token_metadata_add(grn_ctx *ctx,
                       grn_obj *metadata,
                       const char *name,
                       int name_length,
                       grn_obj *value);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */
