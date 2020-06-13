/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _grn_aggregator_data grn_aggregator_data;
typedef void *grn_aggregator_init_func(grn_ctx *ctx,
                                       grn_aggregator_data *data);
typedef grn_rc grn_aggregator_update_func(grn_ctx *ctx,
                                          grn_aggregator_data *data);
typedef grn_rc grn_aggregator_fin_func(grn_ctx *ctx,
                                       grn_aggregator_data *data);

GRN_API grn_id
grn_aggregator_data_get_group_id(grn_ctx *ctx,
                                 grn_aggregator_data *data);
GRN_API grn_id
grn_aggregator_data_get_source_id(grn_ctx *ctx,
                                  grn_aggregator_data *data);
GRN_API grn_obj *
grn_aggregator_data_get_group_table(grn_ctx *ctx,
                                    grn_aggregator_data *data);
GRN_API grn_obj *
grn_aggregator_data_get_source_table(grn_ctx *ctx,
                                     grn_aggregator_data *data);
GRN_API grn_obj *
grn_aggregator_data_get_output_column(grn_ctx *ctx,
                                      grn_aggregator_data *data);
GRN_API grn_obj *
grn_aggregator_data_get_aggregator(grn_ctx *ctx,
                                   grn_aggregator_data *data);
GRN_API grn_obj *
grn_aggregator_data_get_args(grn_ctx *ctx,
                             grn_aggregator_data *data);
GRN_API void *
grn_aggregator_data_get_user_data(grn_ctx *ctx,
                                  grn_aggregator_data *data);

GRN_API grn_obj *
grn_aggregator_create(grn_ctx *ctx,
                      const char *name,
                      int name_size,
                      grn_aggregator_init_func *init,
                      grn_aggregator_update_func *update,
                      grn_aggregator_fin_func *fin);

#ifdef __cplusplus
}
#endif
