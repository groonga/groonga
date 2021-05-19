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

#include <groonga.h>
#include <msgpack.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_MSGPACK_OBJECT_EXT_TIME 0

grn_rc grn_msgpack_pack_raw(grn_ctx *ctx,
                            msgpack_packer *packer,
                            const char *value,
                            unsigned int value_size,
                            grn_id value_domain);

grn_rc grn_msgpack_pack(grn_ctx *ctx,
                        msgpack_packer *packer,
                        grn_obj *value);

grn_rc grn_msgpack_unpack_array(grn_ctx *ctx,
                                msgpack_object_array *array,
                                grn_obj *vector);
# if MSGPACK_VERSION_MAJOR >= 1
int64_t grn_msgpack_unpack_ext_time(grn_ctx *ctx,
                                    msgpack_object_ext *ext);
# endif /* MSGPACK_VERSION_MAJOR >= 1 */

#ifdef __cplusplus
}
#endif
