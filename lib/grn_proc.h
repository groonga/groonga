/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2019-2021  Sutou Kouhei <kou@clear-code.com>

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

#define GRN_SELECT_DEFAULT_LIMIT           10
#define GRN_SELECT_DEFAULT_OUTPUT_COLUMNS_FOR_NO_KEY  "_id, *"
#define GRN_SELECT_DEFAULT_OUTPUT_COLUMNS_FOR_WITH_KEY  "_id, _key, *"

void grn_proc_init_from_env(void);
void grn_proc_query_init_from_env(void);

GRN_VAR const char *grn_document_root;
void grn_db_init_builtin_commands(grn_ctx *ctx);

void grn_proc_init_cast_loose(grn_ctx *ctx);
void grn_proc_init_clearlock(grn_ctx *ctx);
void grn_proc_init_column_copy(grn_ctx *ctx);
void grn_proc_init_column_create(grn_ctx *ctx);
void grn_proc_init_column_create_similar(grn_ctx *ctx);
void grn_proc_init_column_list(grn_ctx *ctx);
void grn_proc_init_column_remove(grn_ctx *ctx);
void grn_proc_init_column_rename(grn_ctx *ctx);
void grn_proc_init_config_get(grn_ctx *ctx);
void grn_proc_init_config_set(grn_ctx *ctx);
void grn_proc_init_config_delete(grn_ctx *ctx);
void grn_proc_init_define_selector(grn_ctx *ctx);
void grn_proc_init_dump(grn_ctx *ctx);
void grn_proc_init_edit_distance(grn_ctx *ctx);
void grn_proc_init_fuzzy_search(grn_ctx *ctx);
void grn_proc_init_highlight(grn_ctx *ctx);
void grn_proc_init_highlight_full(grn_ctx *ctx);
void grn_proc_init_highlight_html(grn_ctx *ctx);
void grn_proc_init_in_records(grn_ctx *ctx);
void grn_proc_init_index_column_diff(grn_ctx *ctx);
void grn_proc_init_lock_acquire(grn_ctx *ctx);
void grn_proc_init_lock_clear(grn_ctx *ctx);
void grn_proc_init_lock_release(grn_ctx *ctx);
void grn_proc_init_log_level(grn_ctx *ctx);
void grn_proc_init_log_put(grn_ctx *ctx);
void grn_proc_init_log_reopen(grn_ctx *ctx);
void grn_proc_init_normalize(grn_ctx *ctx);
void grn_proc_init_object_exist(grn_ctx *ctx);
void grn_proc_init_object_inspect(grn_ctx *ctx);
void grn_proc_init_object_list(grn_ctx *ctx);
void grn_proc_init_object_remove(grn_ctx *ctx);
void grn_proc_init_object_set_visibility(grn_ctx *ctx);
void grn_proc_init_object_warm(grn_ctx *ctx);
void grn_proc_init_query(grn_ctx *ctx);
void grn_proc_init_query_parallel_or(grn_ctx *ctx);
void grn_proc_init_query_expand(grn_ctx *ctx);
void grn_proc_init_query_log_flags_get(grn_ctx *ctx);
void grn_proc_init_query_log_flags_set(grn_ctx *ctx);
void grn_proc_init_query_log_flags_add(grn_ctx *ctx);
void grn_proc_init_query_log_flags_remove(grn_ctx *ctx);
void grn_proc_init_reference_acquire(grn_ctx *ctx);
void grn_proc_init_reference_release(grn_ctx *ctx);
void grn_proc_init_schema(grn_ctx *ctx);
void grn_proc_init_select(grn_ctx *ctx);
void grn_proc_init_snippet(grn_ctx *ctx);
void grn_proc_init_snippet_html(grn_ctx *ctx);
void grn_proc_init_table_copy(grn_ctx *ctx);
void grn_proc_init_table_create(grn_ctx *ctx);
void grn_proc_init_table_create_similar(grn_ctx *ctx);
void grn_proc_init_table_list(grn_ctx *ctx);
void grn_proc_init_table_remove(grn_ctx *ctx);
void grn_proc_init_table_rename(grn_ctx *ctx);
void grn_proc_init_table_tokenize(grn_ctx *ctx);
void grn_proc_init_thread_dump(grn_ctx *ctx);
void grn_proc_init_thread_limit(grn_ctx *ctx);
void grn_proc_init_tokenize(grn_ctx *ctx);

grn_bool grn_proc_option_value_bool(grn_ctx *ctx,
                                    grn_obj *option,
                                    grn_bool default_value);
int32_t grn_proc_option_value_int32(grn_ctx *ctx,
                                    grn_obj *option,
                                    int32_t default_value);
const char *grn_proc_option_value_string(grn_ctx *ctx,
                                         grn_obj *option,
                                         size_t *size);
grn_content_type grn_proc_option_value_content_type(grn_ctx *ctx,
                                                    grn_obj *option,
                                                    grn_content_type default_value);
bool grn_proc_get_value_bool(grn_ctx *ctx,
                             grn_obj *value,
                             bool default_value,
                             const char *tag);
int64_t grn_proc_get_value_int64(grn_ctx *ctx,
                                 grn_obj *value,
                                 int64_t default_value_raw,
                                 const char *tag);
grn_operator grn_proc_get_value_mode(grn_ctx *ctx,
                                     grn_obj *value,
                                     grn_operator default_mode,
                                     const char *tag);
grn_operator grn_proc_get_value_operator(grn_ctx *ctx,
                                         grn_obj *value,
                                         grn_operator default_operator,
                                         const char *tag);
grn_tokenize_mode
grn_proc_get_value_tokenize_mode(grn_ctx *ctx,
                                 grn_obj *value,
                                 grn_tokenize_mode default_mode,
                                 const char *tag);
uint32_t
grn_proc_get_value_token_cursor_flags(grn_ctx *ctx,
                                      grn_obj *value,
                                      uint32_t default_flags,
                                      const char *tag);
double
grn_proc_get_value_double(grn_ctx *ctx,
                          grn_obj *value,
                          double default_value_raw,
                          const char *tag);
grn_obj *
grn_proc_get_value_object(grn_ctx *ctx,
                          grn_obj *value,
                          const char *tag);
grn_obj *
grn_proc_get_value_column(grn_ctx *ctx,
                          grn_obj *value,
                          grn_obj *table,
                          const char *tag);

void grn_proc_output_object_name(grn_ctx *ctx, grn_obj *obj);
void grn_proc_output_object_id_name(grn_ctx *ctx, grn_id id);

grn_bool grn_proc_table_set_token_filters(grn_ctx *ctx,
                                          grn_obj *table,
                                          grn_raw_string *token_filters_raw);

grn_column_flags grn_proc_column_parse_flags(grn_ctx *ctx,
                                             const char *error_message_tag,
                                             const char *text,
                                             const char *end);

grn_bool grn_proc_select_output_columns_open(grn_ctx *ctx,
                                             grn_obj_format *format,
                                             grn_obj *result_set,
                                             int n_hits,
                                             int offset,
                                             int limit,
                                             const char *columns,
                                             int columns_len,
                                             grn_obj *condition,
                                             uint32_t n_additional_elements);
grn_bool grn_proc_select_output_columns_close(grn_ctx *ctx,
                                              grn_obj_format *format,
                                              grn_obj *result_set);
grn_bool grn_proc_select_output_columns(grn_ctx *ctx,
                                        grn_obj *res,
                                        int n_hits,
                                        int offset,
                                        int limit,
                                        const char *columns,
                                        int columns_len,
                                        grn_obj *condition);

grn_rc grn_proc_syntax_expand_query(grn_ctx *ctx,
                                    const char *query,
                                    unsigned int query_len,
                                    grn_expr_flags flags,
                                    const char *query_expander_name,
                                    unsigned int query_expander_name_len,
                                    const char *term_column_name,
                                    unsigned int term_column_name_len,
                                    const char *expanded_term_column_name,
                                    unsigned int expanded_term_column_name_len,
                                    grn_obj *expanded_query,
                                    const char *error_message_tag);

grn_expr_flags grn_proc_expr_query_flags_parse(grn_ctx *ctx,
                                               grn_obj *query_flags,
                                               const char *error_message_tag);

grn_obj *grn_proc_lexicon_open(grn_ctx *ctx,
                               grn_raw_string *tokenizer_raw,
                               grn_raw_string *normalizer_raw,
                               grn_raw_string *token_filters_raw,
                               const char *context_tag);

grn_bool grn_proc_text_include_special_character(grn_ctx *ctx,
                                                 const char *text,
                                                 size_t size);

typedef struct {
  grn_obj *found;
  grn_obj *table;
  grn_obj *records;
} grn_proc_selector_to_function_data;

bool
grn_proc_selector_to_function_data_init(grn_ctx *ctx,
                                        grn_proc_selector_to_function_data *data,
                                        grn_user_data *user_data);
void
grn_proc_selector_to_function_data_selected(
  grn_ctx *ctx,
  grn_proc_selector_to_function_data *data);
void
grn_proc_selector_to_function_data_fin(grn_ctx *ctx,
                                       grn_proc_selector_to_function_data *data);



#ifdef __cplusplus
}
#endif
