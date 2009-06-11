/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define OBJECT(name) (grn_ctx_get(&context, (name), strlen(name)))

void test_array_set_data(void);
void data_temporary_table_no_path(void);
void test_temporary_table_no_path(gpointer data);
void data_temporary_table_default_tokenizer(void);
void test_temporary_table_default_tokenizer(gpointer data);
void data_temporary_table_add(void);
void test_temporary_table_add(gpointer data);

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj *database;

void
cut_setup(void)
{
  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, NULL, NULL);
}

void
cut_teardown(void)
{
  grn_ctx_fin(&context);
  teardown_grn_logger(logger);
}

void
test_array_set_data(void)
{
  grn_obj *table;
  grn_id record_id;
  gchar value[] = "sample value";
  grn_obj *record_value;
  grn_obj *retrieved_record_value;

  table = grn_table_create(&context, NULL, 0, NULL,
                           GRN_OBJ_TABLE_NO_KEY,
                           NULL, sizeof(value));
  record_id = grn_table_add(&context, table, NULL, 0, NULL);

  record_value = grn_obj_open(&context, GRN_BULK, 0, 0);
  grn_bulk_write(&context, record_value, value, sizeof(value));
  grn_test_assert(grn_obj_set_value(&context, table, record_id,
                                    record_value, GRN_OBJ_SET));

  retrieved_record_value = grn_obj_get_value(&context, table, record_id, NULL);
  cut_assert_equal_string(value, GRN_BULK_HEAD(retrieved_record_value));
}

void
data_temporary_table_no_path(void)
{
#define ADD_DATA(label, flags)                                          \
  cut_add_data(label, GINT_TO_POINTER(flags), NULL, NULL)

  ADD_DATA("no-key", GRN_OBJ_TABLE_NO_KEY);
  ADD_DATA("hash", GRN_OBJ_TABLE_HASH_KEY);
  ADD_DATA("patricia trie", GRN_OBJ_TABLE_PAT_KEY);

#undef ADD_DATA
}

void
test_temporary_table_no_path(gpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = GPOINTER_TO_INT(data);

  table = grn_table_create(&context, NULL, 0, NULL,
                           flags,
                           NULL, sizeof(grn_id));
  cut_assert_equal_string(NULL, grn_obj_path(&context, table));
}

void
test_temporary_table_default_tokenizer(gpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = GPOINTER_TO_INT(data);
  grn_obj *tokenizer = NULL;
  char name[1024];

  table = grn_table_create(&context, NULL, 0, NULL,
                           flags,
                           NULL, sizeof(grn_id));
  grn_obj_set_info(&context, table, GRN_INFO_DEFAULT_TOKENIZER,
                   OBJECT("<token:trigram>"));
  tokenizer = grn_obj_get_info(&context, table, GRN_INFO_DEFAULT_TOKENIZER, NULL);
  grn_obj_name(&context, tokenizer, name, sizeof(name));
  cut_assert_equal_string("<token:trigram>", name);
}

void
data_temporary_table_add(void)
{
#define ADD_DATA(label, flags)                                          \
  cut_add_data(label, GINT_TO_POINTER(flags), NULL, NULL)

  ADD_DATA("no-key", GRN_OBJ_TABLE_NO_KEY);
  ADD_DATA("hash", GRN_OBJ_TABLE_HASH_KEY);
  ADD_DATA("patricia trie", GRN_OBJ_TABLE_PAT_KEY);

#undef ADD_DATA
}

void
test_temporary_table_add(gpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = GPOINTER_TO_INT(data);
  gchar key[] = "key";

  table = grn_table_create(&context, NULL, 0, NULL,
                           flags,
                           OBJECT("<shorttext>"),
                           sizeof(grn_id));
  if ((flags & GRN_OBJ_TABLE_TYPE_MASK) == GRN_OBJ_TABLE_NO_KEY) {
    grn_table_add(&context, table, NULL, 0, NULL);
  } else {
    grn_table_add(&context, table, key, strlen(key), NULL);
  }

  cut_assert_equal_int(1, grn_table_size(&context, table));
}
