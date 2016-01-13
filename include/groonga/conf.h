/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015-2016 Brazil

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
#endif

#define GRN_CONF_MAX_KEY_SIZE GRN_TABLE_MAX_KEY_SIZE
#define GRN_CONF_MAX_VALUE_SIZE                                         \
  (GRN_CONF_VALUE_SPACE_SIZE - sizeof(uint32_t) - 1) /* 1 is for '\0' */
#define GRN_CONF_VALUE_SPACE_SIZE (4 * 1024)

GRN_API grn_rc grn_conf_set(grn_ctx *ctx,
                            const char *key, int key_size,
                            const char *value, int value_size);
GRN_API grn_rc grn_conf_get(grn_ctx *ctx,
                            const char *key, int key_size,
                            const char **value, uint32_t *value_size);
GRN_API grn_rc grn_conf_delete(grn_ctx *ctx,
                               const char *key, int key_size);

#ifdef __cplusplus
}
#endif
