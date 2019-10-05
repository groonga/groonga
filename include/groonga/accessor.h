/*
  Copyright(C) 2012-2018 Brazil
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

GRN_API grn_rc grn_accessor_resolve(grn_ctx *ctx,
                                    grn_obj *accessor,
                                    int depth,
                                    grn_obj *base_res,
                                    grn_obj *res,
                                    grn_operator op);

typedef grn_rc (*grn_accessor_execute_func)(grn_ctx *ctx,
                                            grn_obj *index,
                                            grn_operator op,
                                            grn_obj *res,
                                            grn_operator logical_op,
                                            void *user_data);

GRN_API grn_rc grn_accessor_execute(grn_ctx *ctx,
                                    grn_obj *accessor,
                                    grn_accessor_execute_func execute,
                                    void *execute_data,
                                    grn_operator execute_op,
                                    grn_obj *res,
                                    grn_operator logical_op);

GRN_API grn_rc grn_accessor_name(grn_ctx *ctx,
                                 grn_obj *accessor,
                                 grn_obj *name);

#ifdef __cplusplus
}
#endif
