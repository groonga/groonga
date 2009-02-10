/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __SEN_TEST_UTILS_H__
#define __SEN_TEST_UTILS_H__

#include <groonga.h>

#include <glib.h>

#include <string.h>

#define SEN_TEST_ENV_SPACE_PATH "SEN_TEST_SPACE_PATH"
#define SEN_TEST_ENV_TABLE_PATH "SEN_TEST_TABLE_PATH"
#define SEN_TEST_ENV_TABLE_TYPE "SEN_TEST_TABLE_TYPE"
#define SEN_TEST_ENV_HASH_PATH "SEN_TEST_HASH_PATH"
#define SEN_TEST_ENV_PATRICIA_TRIE_PATH "SEN_TEST_PATRICIA_TRIE_PATH"
#define SEN_TEST_ENV_MULTI_THREAD "SEN_TEST_MULTI_THREAD"
#define SEN_TEST_ENV_N_PROCESSES "SEN_TEST_N_PROCESSES"
#define SEN_TEST_ENV_PROCESS_NUMBER "SEN_TEST_PROCESS_NUMBER"

typedef void (*sen_test_set_parameters_func) (void);

const gchar *sen_rc_to_string(sen_rc rc);
const gchar *sen_test_get_base_dir(void);

sen_logger_info *sen_collect_logger_new(void);
void sen_collect_logger_clear_messages(sen_logger_info *logger);
const GList *sen_collect_logger_get_messages(sen_logger_info *logger);
void sen_collect_logger_free(sen_logger_info *logger);

sen_logger_info *setup_sen_logger(void);
void teardown_sen_logger(sen_logger_info *logger);

GString *sen_long_path_new(const gchar *base_path, gssize max_size);

GList      *sen_test_pat_cursor_get_keys  (sen_ctx          *context,
                                           sen_table_cursor *cursor);
GList      *sen_test_pat_get_keys         (sen_ctx          *context,
                                           sen_obj          *patricia_trie);
GList      *sen_test_pat_cursor_get_pairs (sen_ctx          *context,
                                           sen_table_cursor *cursor);
GHashTable *sen_test_pat_get_pairs        (sen_ctx          *context,
                                           sen_obj          *patricia_trie);

#endif
