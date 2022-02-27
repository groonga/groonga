/*
  Copyright(C) 2013-2018  Brazil
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_expr_code.h"
#include "grn_expr_executor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_EXPR_CONST_BLK_SIZE GRN_STACK_SIZE

struct _grn_expr {
  grn_db_obj obj;
  grn_obj name_buf;
  grn_expr_var *vars;
  uint32_t nvars;
  /* -- compatible with grn_proc -- */

  uint16_t cacheable;
  uint16_t taintable;
  grn_obj **const_blks;
  grn_obj *values;
  grn_expr_code *codes;
  uint32_t nconsts;
  uint32_t values_curr;
  uint32_t values_tail;
  uint32_t values_size;
  uint32_t codes_curr;
  uint32_t codes_size;

  grn_obj objs;
  grn_obj dfi;
  grn_expr_code *code0;

  struct {
    grn_expr_code *codes;
    uint32_t codes_curr;
    grn_expr_executor executor;
  } cache;

  grn_obj query_log_tag_prefix;
  grn_obj *parent;
  grn_obj *condition;
};

#define SCAN_ACCESSOR                  (0x01)
#define SCAN_PUSH                      (0x02)
#define SCAN_POP                       (0x04)
#define SCAN_PRE_CONST                 (0x08)

typedef enum {
  SCAN_START = 0,
  SCAN_VAR,
  SCAN_COL1,
  SCAN_COL2,
  SCAN_CONST
} scan_stat;

typedef struct _grn_scan_info scan_info;
typedef grn_bool (*grn_scan_info_each_arg_callback)(grn_ctx *ctx, grn_obj *obj, void *user_data);

void grn_expr_init_from_env(void);

grn_obj *
grn_expr_get_query_options(grn_ctx *ctx,
                           grn_obj *expr);
grn_rc
grn_expr_set_query_options(grn_ctx *ctx,
                           grn_obj *expr,
                           grn_obj *query_options);

scan_info **grn_scan_info_build(grn_ctx *ctx, grn_obj *expr, int *n,
                                grn_operator op, grn_bool record_exist);

scan_info *grn_scan_info_open(grn_ctx *ctx, int start);
void grn_scan_info_close(grn_ctx *ctx, scan_info *si);
void grn_scan_info_put_index(grn_ctx *ctx,
                             scan_info *si,
                             grn_obj *index,
                             uint32_t sid,
                             float weight,
                             grn_obj *scorer,
                             grn_obj *scorer_args_expr,
                             uint32_t scorer_args_expr_offset);
scan_info **grn_scan_info_put_logical_op(grn_ctx *ctx,
                                         scan_info **sis,
                                         int *ip,
                                         grn_operator op,
                                         int start,
                                         float weight_factor);
int grn_scan_info_get_flags(scan_info *si);
void grn_scan_info_set_flags(scan_info *si, int flags);
grn_operator grn_scan_info_get_logical_op(scan_info *si);
void grn_scan_info_set_logical_op(scan_info *si, grn_operator logical_op);
grn_operator grn_scan_info_get_op(scan_info *si);
void grn_scan_info_set_weight_factor(scan_info *si, float factor);
float grn_scan_info_get_weight_factor(scan_info *si);
void grn_scan_info_set_op(scan_info *si, grn_operator op);
void grn_scan_info_set_end(scan_info *si, uint32_t end);
void grn_scan_info_set_query(scan_info *si, grn_obj *query);
int grn_scan_info_get_max_interval(scan_info *si);
void grn_scan_info_set_max_interval(scan_info *si, int max_interval);
int grn_scan_info_get_additional_last_interval(scan_info *si);
void grn_scan_info_set_additional_last_interval(scan_info *si,
                                                int additional_last_interval);
grn_obj *grn_scan_info_get_max_element_intervals(scan_info *si);
void grn_scan_info_set_max_element_intervals(grn_ctx *ctx,
                                             scan_info *si,
                                             grn_obj *max_element_intervals);
int grn_scan_info_get_similarity_threshold(scan_info *si);
void grn_scan_info_set_similarity_threshold(scan_info *si, int similarity_threshold);
int grn_scan_info_get_quorum_threshold(scan_info *si);
void grn_scan_info_set_quorum_threshold(scan_info *si, int quorum_threshold);
grn_bool grn_scan_info_push_arg(grn_ctx *ctx, scan_info *si, grn_obj *arg);
grn_obj *grn_scan_info_get_arg(grn_ctx *ctx, scan_info *si, int i);
int grn_scan_info_get_start_position(scan_info *si);
void grn_scan_info_set_start_position(scan_info *si, uint32_t start);
void grn_scan_info_reset_position(scan_info *si);

float grn_expr_code_get_weight(grn_ctx *ctx, grn_expr_code *ec, uint32_t *offset);
grn_rc grn_expr_code_inspect_indented(grn_ctx *ctx,
                                      grn_obj *buffer,
                                      grn_expr_code *code,
                                      const char *indent);
void grn_p_expr_code(grn_ctx *ctx, grn_expr_code *code);

grn_obj *grn_expr_alloc_const(grn_ctx *ctx, grn_obj *expr);

grn_rc grn_ctx_expand_stack(grn_ctx *ctx);

grn_bool grn_expr_is_simple_function_call(grn_ctx *ctx, grn_obj *expr);
grn_obj *grn_expr_simple_function_call_get_function(grn_ctx *ctx, grn_obj *expr);
grn_rc grn_expr_simple_function_call_get_arguments(grn_ctx *ctx,
                                                   grn_obj *expr,
                                                   grn_obj *arguments);

grn_bool grn_expr_is_module_list(grn_ctx *ctx, grn_obj *expr);
unsigned int grn_expr_module_list_get_n_modules(grn_ctx *ctx,
                                                grn_obj *expr);
grn_obj *grn_expr_module_list_get_function(grn_ctx *ctx,
                                           grn_obj *expr,
                                           unsigned int i);
grn_rc grn_expr_module_list_get_arguments(grn_ctx *ctx,
                                          grn_obj *expr,
                                          unsigned int i,
                                          grn_obj *arguments);

grn_rc
grn_expr_match_columns_split(grn_ctx *ctx,
                             grn_obj *expr,
                             grn_obj *splitted_match_columns);

grn_obj *
grn_expr_slice(grn_ctx *ctx,
               grn_obj *expr,
               uint32_t code_start_offset,
               uint32_t code_end_offset);
grn_rc
grn_expr_to_script_syntax(grn_ctx *ctx,
                          grn_obj *expr,
                          grn_obj *buffer);
void
grn_expr_get_range_info(grn_ctx *ctx,
                        grn_obj *expr,
                        grn_id *range_id,
                        grn_obj_flags *range_flags);

typedef enum {
  GRN_EXPR_V1_FORMAT_TYPE_OUTPUT_COLUMNS,
  GRN_EXPR_V1_FORMAT_TYPE_SORT_KEYS,
  GRN_EXPR_V1_FORMAT_TYPE_GROUP_KEYS,
} grn_expr_v1_format_type;

bool
grn_expr_is_v1_format(grn_ctx *ctx,
                      const char *raw_text,
                      ssize_t raw_text_len,
                      grn_expr_v1_format_type type);


#ifdef __cplusplus
}
#endif
