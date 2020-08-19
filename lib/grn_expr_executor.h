/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017-2018  Brazil
  Copyright(C) 2019-2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  grn_obj source_buffer;
  grn_obj *source;
  grn_column_cache *cache;
  size_t n_required_args;
} grn_expr_executor_scorer_data;

typedef struct _grn_expr_executor grn_expr_executor;

typedef double (*grn_expr_executor_scorer_func)(grn_ctx *ctx,
                                                grn_expr_executor *executor,
                                                grn_id id,
                                                grn_obj *args,
                                                grn_expr_executor_scorer_data *data);

typedef union {
  struct {
    grn_obj *value;
  } constant;
  struct {
    grn_obj *column;
    grn_obj value_buffer;
  } value;
  struct {
    grn_obj result_buffer;
    void *regex;
    grn_obj value_buffer;
    grn_obj *normalizer;
  } simple_regexp;
  struct {
    grn_obj result_buffer;
    grn_obj *normalizer;
    void *regex;
    grn_obj *normalized_sub_text;
    const char *normalized_sub_text_raw;
    unsigned int normalized_sub_text_raw_length_in_bytes;
    grn_obj value_buffer;
  } simple_match;
  struct {
    grn_proc_ctx proc_ctx;
    grn_obj **args;
    int n_args;
    grn_obj *buffers;
    int n_buffers;
  } simple_proc;
  struct {
    grn_obj result_buffer;
  } simple_condition_constant;
  struct {
    grn_obj result_buffer;
    grn_ra *ra;
    grn_ra_cache ra_cache;
    unsigned int ra_element_size;
    grn_obj value_buffer;
    grn_obj constant_buffer;
    grn_operator_exec_func *exec;
  } simple_condition_ra;
  struct {
    bool need_exec;
    grn_obj result_buffer;
    grn_obj value_buffer;
    grn_obj constant_buffer;
    grn_operator_exec_func *exec;
  } simple_condition;
  struct {
    grn_obj *score_column;
    grn_obj args;
    grn_obj score_buffer;
    grn_expr_executor_scorer_func *funcs;
    grn_expr_executor_scorer_data *datas;
    size_t n_funcs;
    size_t n_allocated_funcs;
  } scorer;
} grn_expr_executor_data;

typedef grn_obj *(*grn_expr_executor_exec_func)(grn_ctx *ctx,
                                                grn_expr_executor *executor,
                                                grn_id id);
typedef void (*grn_expr_executor_fin_func)(grn_ctx *ctx,
                                           grn_expr_executor *executor);

struct _grn_expr_executor {
  grn_obj *expr;
  grn_obj *variable;
  grn_expr_executor_exec_func exec;
  grn_expr_executor_fin_func fin;
  grn_expr_executor_data data;
};

grn_rc
grn_expr_executor_init(grn_ctx *ctx,
                       grn_expr_executor *executor,
                       grn_obj *expr);
grn_rc
grn_expr_executor_fin(grn_ctx *ctx,
                      grn_expr_executor *executor);

grn_expr_executor *
grn_expr_executor_open(grn_ctx *ctx,
                       grn_obj *expr);
grn_rc
grn_expr_executor_close(grn_ctx *ctx,
                        grn_expr_executor *executor);

grn_obj *
grn_expr_executor_exec(grn_ctx *ctx,
                       grn_expr_executor *executor,
                       grn_id id);

grn_rc
grn_expr_executor_exec_batch(grn_ctx *ctx,
                             grn_expr_executor *executor,
                             grn_table_cursor *cursor);

#ifdef __cplusplus
}
#endif
