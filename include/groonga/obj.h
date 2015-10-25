/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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
#ifndef GROONGA_OBJ_H
#define GROONGA_OBJ_H

#ifdef  __cplusplus
extern "C" {
#endif

#define GRN_OBJ_IS_TRUE(ctx, obj, result) do {  \
  grn_obj *obj_ = (obj);                        \
  switch (obj_->header.type) {                  \
  case GRN_BULK :                               \
    switch (obj_->header.domain) {              \
    case GRN_DB_BOOL :                          \
      result = GRN_BOOL_VALUE(obj_);            \
      break;                                    \
    case GRN_DB_INT32 :                         \
      result = GRN_INT32_VALUE(obj_) != 0;      \
      break;                                    \
    case GRN_DB_UINT32 :                        \
      result = GRN_UINT32_VALUE(obj_) != 0;     \
      break;                                    \
    case GRN_DB_FLOAT :                         \
      {                                         \
        double float_value;                     \
        float_value = GRN_FLOAT_VALUE(obj_);    \
        result = (float_value < -DBL_EPSILON || \
                  DBL_EPSILON < float_value);   \
      }                                         \
      break;                                    \
    case GRN_DB_SHORT_TEXT :                    \
    case GRN_DB_TEXT :                          \
    case GRN_DB_LONG_TEXT :                     \
      result = GRN_TEXT_LEN(obj_) != 0;         \
      break;                                    \
    default :                                   \
      result = GRN_FALSE;                       \
      break;                                    \
    }                                           \
    break;                                      \
  case GRN_VECTOR :                             \
    result = GRN_TRUE;                          \
    break;                                      \
  default :                                     \
    result = GRN_FALSE;                         \
    break;                                      \
  }                                             \
} while (0)


GRN_API grn_bool grn_obj_is_builtin(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_table(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_type(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_tokenizer_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_function_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_selector_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_selector_only_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_normalizer_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_token_filter_proc(grn_ctx *ctx, grn_obj *obj);
GRN_API grn_bool grn_obj_is_scorer_proc(grn_ctx *ctx, grn_obj *obj);

GRN_API grn_rc grn_obj_cast(grn_ctx *ctx,
                            grn_obj *src,
                            grn_obj *dest,
                            grn_bool add_record_if_not_exist);

#ifdef __cplusplus
}
#endif

#endif /* GROONGA_OBJ_H */
