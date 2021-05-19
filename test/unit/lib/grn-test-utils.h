/*
  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>

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

#ifndef __GRN_TEST_UTILS_H__
#define __GRN_TEST_UTILS_H__

#include <groonga.h>

#include <gcutter.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_TEST_ENV_TABLE_PATH "GRN_TEST_TABLE_PATH"
#define GRN_TEST_ENV_TABLE_TYPE "GRN_TEST_TABLE_TYPE"
#define GRN_TEST_ENV_HASH_PATH "GRN_TEST_HASH_PATH"
#define GRN_TEST_ENV_PATRICIA_TRIE_PATH "GRN_TEST_PATRICIA_TRIE_PATH"
#define GRN_TEST_ENV_MULTI_THREAD "GRN_TEST_MULTI_THREAD"
#define GRN_TEST_ENV_N_PROCESSES "GRN_TEST_N_PROCESSES"
#define GRN_TEST_ENV_PROCESS_NUMBER "GRN_TEST_PROCESS_NUMBER"

#define GRN_TEST_GEO_COORDINATE(hours, minutes, seconds)     \
  ((hours) * 3600 + (minutes) * 60 + (seconds)) * 1000

#define GRN_TEST_GEO_POINT_STRING(latitude, longitude) \
  g_strdup_printf("%dx%d", latitude, longitude)

#if !GLIB_CHECK_VERSION(2, 32, 0)
#  define g_thread_new(name, func, data) \
  g_thread_create(func, data, TRUE, NULL)
#endif

typedef void (*grn_test_set_parameters_func) (void);

const gchar *grn_rc_to_string              (grn_rc rc);
const gchar *grn_test_get_base_dir         (void);
const gchar *grn_test_get_build_dir        (void);
const gchar *grn_test_get_tmp_dir          (void);

grn_logger_info *
             grn_collect_logger_new        (void);
void         grn_collect_logger_clear_messages
                                           (grn_logger_info *logger);
const GList *grn_collect_logger_get_messages
                                           (grn_logger_info *logger);
const gchar *grn_collect_logger_to_string  (grn_logger_info *logger);
void         grn_collect_logger_print_messages
                                           (grn_logger_info *logger);
void         grn_collect_logger_free       (grn_logger_info  *logger);

grn_logger_info *
             setup_grn_logger              (void);
void         teardown_grn_logger           (grn_logger_info  *logger);

GString     *grn_long_path_new             (const gchar      *base_path,
                                            gssize            max_size);
GString     *grn_long_name_new             (gssize            max_size);

GList       *grn_test_pat_cursor_get_keys  (grn_ctx          *context,
                                            grn_table_cursor *cursor);
GList       *grn_test_pat_get_keys         (grn_ctx          *context,
                                            grn_obj          *patricia_trie);
GList       *grn_test_pat_cursor_get_pairs (grn_ctx          *context,
                                            grn_table_cursor *cursor);
GHashTable  *grn_test_pat_get_pairs        (grn_ctx          *context,
                                            grn_obj          *patricia_trie);
const gchar *grn_test_type_inspect         (grn_ctx          *context,
                                            unsigned char     type);
void         grn_test_object_inspect       (GString          *output,
                                            grn_ctx          *context,
                                            grn_obj          *object);

const gchar *grn_test_send_command         (grn_ctx          *context,
                                            const gchar      *command);
void         grn_test_send_commands        (grn_ctx          *context,
                                            const gchar      *line_separated_commands);

const GList *grn_test_table_collect_string (grn_ctx          *context,
                                            grn_obj          *table,
                                            const gchar      *text_column_name);

gint         grn_test_coordinate_in_milliseconds
                                           (gdouble           coordinate_in_degree);
gdouble      grn_test_coordinate_in_degree (gint              coordinate_in_milliseconds);
const gchar *grn_test_location_string      (gdouble           latitude_in_degree,
                                            gdouble           longitude_in_degree);

#ifdef __cplusplus
}
#endif

#endif
