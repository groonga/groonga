/* -*- c-basic-offset: 2 -*- */
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

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include "mrb_ctx.h"
#include "mrb_table_with_key.h"
#include "mrb_converter.h"

static mrb_value
mrb_grn_table_with_key_array_reference(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *table;
  grn_id key_domain_id;
  mrb_value mrb_key;
  grn_id record_id;
  grn_mrb_value_to_raw_data_buffer buffer;
  void *key;
  unsigned int key_size;

  mrb_get_args(mrb, "o", &mrb_key);

  table = DATA_PTR(self);
  if (table->header.type == GRN_DB) {
    key_domain_id = GRN_DB_SHORT_TEXT;
  } else {
    key_domain_id = table->header.domain;
  }

  grn_mrb_value_to_raw_data_buffer_init(mrb, &buffer);
  grn_mrb_value_to_raw_data(mrb, "key", mrb_key, key_domain_id,
                            &buffer, &key, &key_size);
  record_id = grn_table_get(ctx, table, key, key_size);
  grn_mrb_value_to_raw_data_buffer_fin(mrb, &buffer);

  if (record_id == GRN_ID_NIL) {
    return mrb_nil_value();
  } else {
    return mrb_fixnum_value(record_id);
  }
}

static mrb_value
mrb_grn_table_with_key_get_duplicated_keys(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *table = DATA_PTR(self);
  grn_obj *duplicated_keys = NULL;
  grn_table_get_duplicated_keys(ctx, table, &duplicated_keys);
  grn_mrb_ctx_check(mrb);
  return grn_mrb_value_from_grn_obj(mrb, duplicated_keys);
}

static mrb_value
mrb_grn_table_with_key_have_duplicated_keys(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *table = DATA_PTR(self);
  bool have_duplicated_keys = grn_table_have_duplicated_keys(ctx, table);
  grn_mrb_ctx_check(mrb);
  return mrb_bool_value(have_duplicated_keys);
}

static mrb_value
mrb_grn_table_with_key_is_lexicon_without_data_column(mrb_state *mrb,
                                                      mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *table = DATA_PTR(self);
  return mrb_bool_value(grn_obj_is_lexicon_without_data_column(ctx, table));
}

void
grn_mrb_table_with_key_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *table_class;
  struct RClass *klass;

  table_class = mrb_class_get_under(mrb, module, "Table");
  klass = mrb_define_class_under(mrb, module, "TableWithKey", table_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "[]",
                    mrb_grn_table_with_key_array_reference,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "duplicated_keys",
                    mrb_grn_table_with_key_get_duplicated_keys,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "have_duplicated_keys?",
                    mrb_grn_table_with_key_have_duplicated_keys,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "lexicon_without_data_column?",
                    mrb_grn_table_with_key_is_lexicon_without_data_column,
                    MRB_ARGS_NONE());
}
#endif
