/*
  Copyright(C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

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

#define CAST_FAILED(caster) do {                                \
  uint32_t invalid_mode =                                       \
    ((caster)->flags & GRN_OBJ_INVALID_MASK);                   \
  if (invalid_mode != GRN_OBJ_INVALID_IGNORE) {                 \
    GRN_DEFINE_NAME_CUSTOM((caster)->target, target_name);      \
    grn_obj *type =                                             \
      grn_ctx_at(ctx, (caster)->dest->header.domain);           \
    GRN_DEFINE_NAME_CUSTOM(type, type_name);                    \
    grn_obj inspected;                                          \
    GRN_TEXT_INIT(&inspected, 0);                               \
    grn_inspect(ctx, &inspected, (caster)->src);                \
    if (invalid_mode == GRN_OBJ_INVALID_WARN) {                 \
      GRN_LOG(ctx,                                              \
              GRN_LOG_WARNING,                                  \
              "<%.*s>: failed to cast to <%.*s>: <%.*s>",       \
              target_name_size,                                 \
              target_name,                                      \
              type_name_size,                                   \
              type_name,                                        \
              (int)GRN_TEXT_LEN(&inspected),                    \
              GRN_TEXT_VALUE(&inspected));                      \
    } else {                                                    \
      ERR(GRN_INVALID_ARGUMENT,                                 \
          "<%.*s>: failed to cast to <%.*s>: <%.*s>",           \
          target_name_size,                                     \
          target_name,                                          \
          type_name_size,                                       \
          type_name,                                            \
          (int)GRN_TEXT_LEN(&inspected),                        \
          GRN_TEXT_VALUE(&inspected));                          \
    }                                                           \
    GRN_OBJ_FIN(ctx, &inspected);                               \
    grn_obj_unref(ctx, type);                                   \
  }                                                             \
} while (0)

#define ERR_CAST(column, range, src) do {                     \
  grn_obj dest;                                               \
  dest.header.domain = DB_OBJ(range)->id;                     \
  grn_caster caster = {                                       \
    src,                                                      \
    &dest,                                                    \
    GRN_OBJ_INVALID_ERROR,                                    \
    column,                                                   \
  };                                                          \
  CAST_FAILED(&caster);                                       \
} while (false)

grn_rc
grn_caster_cast_text_to_uvector(grn_ctx *ctx, grn_caster *caster);
grn_rc
grn_caster_cast_text_to_text_vector(grn_ctx *ctx, grn_caster *caster);

#ifdef __cplusplus
}
#endif
