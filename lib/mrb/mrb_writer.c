/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil
  Copyright(C) 2020 Sutou Kouhei <kou@clear-code.com>

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
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>

#include "../grn_mrb.h"
#include "../grn_output.h"
#include "mrb_ctx.h"
#include "mrb_writer.h"
#include "mrb_options.h"

static mrb_value
writer_write(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value target;

  mrb_get_args(mrb, "o", &target);

  switch (mrb_type(target)) {
  case MRB_TT_FALSE :
    if (mrb_nil_p(target)) {
      GRN_OUTPUT_NULL();
    } else {
      GRN_OUTPUT_BOOL(GRN_FALSE);
    }
    break;
  case MRB_TT_TRUE :
    GRN_OUTPUT_BOOL(GRN_TRUE);
    break;
  case MRB_TT_FIXNUM :
    GRN_OUTPUT_INT64(mrb_integer(target));
    break;
  case MRB_TT_FLOAT :
    GRN_OUTPUT_FLOAT(mrb_float(target));
    break;
  case MRB_TT_SYMBOL :
    {
      const char *name;
      mrb_int name_length;

      name = mrb_sym2name_len(mrb, mrb_symbol(target), &name_length);
      GRN_OUTPUT_STR(name, name_length);
    }
    break;
  case MRB_TT_STRING :
    GRN_OUTPUT_STR(RSTRING_PTR(target), RSTRING_LEN(target));
    break;
  default :
    mrb_raisef(mrb, E_ARGUMENT_ERROR,
               "must be nil, true, false, number, float, symbol or string: "
               "%S",
               target);
    break;
  }

  return mrb_nil_value();
}

static mrb_value
writer_flush(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  int flags = 0;
  grn_ctx_output_flush(ctx, flags);
  return mrb_nil_value();
}

static mrb_value
writer_open_array(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  char *name;
  mrb_int n_elements;

  mrb_get_args(mrb, "zi", &name, &n_elements);
  GRN_OUTPUT_ARRAY_OPEN(name, n_elements);

  return mrb_nil_value();
}

static mrb_value
writer_close_array(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;

  GRN_OUTPUT_ARRAY_CLOSE();

  return mrb_nil_value();
}

static mrb_value
writer_open_map(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  char *name;
  mrb_int n_elements;

  mrb_get_args(mrb, "zi", &name, &n_elements);
  GRN_OUTPUT_MAP_OPEN(name, n_elements);

  return mrb_nil_value();
}

static mrb_value
writer_close_map(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;

  GRN_OUTPUT_MAP_CLOSE();

  return mrb_nil_value();
}

static mrb_value
writer_write_table_columns(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_table;
  char *columns;
  mrb_int columns_size;
  grn_obj *table;
  grn_obj_format format;
  int n_hits = 0;
  int offset = 0;
  int limit = 0;
  int hits_offset = 0;

  mrb_get_args(mrb, "os", &mrb_table, &columns, &columns_size);

  table = DATA_PTR(mrb_table);
  GRN_OBJ_FORMAT_INIT(&format, n_hits, offset, limit, hits_offset);
  format.flags |= GRN_OBJ_FORMAT_WITH_COLUMN_NAMES;
  {
    grn_rc rc;
    rc = grn_obj_format_set_columns(ctx, &format,
                                    table, columns, columns_size);
    if (rc != GRN_SUCCESS) {
      grn_obj_format_fin(ctx, &format);
      grn_mrb_ctx_check(mrb);
    }
  }
  GRN_OUTPUT_TABLE_COLUMNS(table, &format);
  grn_obj_format_fin(ctx, &format);

  return mrb_nil_value();
}

static mrb_value
writer_open_table_records(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_int mrb_n_records;
  mrb_get_args(mrb, "i", &mrb_n_records);
  grn_ctx_output_table_records_open(ctx, mrb_n_records);

  return mrb_nil_value();
}

static mrb_value
writer_write_table_records_content_internal(mrb_state *mrb,
                                            mrb_value self,
                                            bool only_content)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_table;
  mrb_value mrb_options = mrb_nil_value();
  char *columns;
  mrb_int columns_size;
  grn_obj *table;
  grn_obj_format format;
  grn_obj *condition = NULL;
  int n_hits = 0;
  int offset = 0;
  int limit = -1;
  int hits_offset = 0;
  bool auto_flush = false;

  mrb_get_args(mrb, "os|H", &mrb_table, &columns, &columns_size, &mrb_options);

  table = DATA_PTR(mrb_table);
  if (!mrb_nil_p(mrb_options)) {
    mrb_value mrb_offset = grn_mrb_options_get_lit(mrb, mrb_options, "offset");
    if (!mrb_nil_p(mrb_offset)) {
      offset = mrb_integer(mrb_offset);
    }

    mrb_value mrb_limit = grn_mrb_options_get_lit(mrb, mrb_options, "limit");
    if (!mrb_nil_p(mrb_limit)) {
      limit = mrb_integer(mrb_limit);
    }

    mrb_value mrb_auto_flush =
      grn_mrb_options_get_lit(mrb, mrb_options, "auto_flush");
    if (!mrb_nil_p(mrb_auto_flush)) {
      auto_flush = mrb_bool(mrb_auto_flush);
    }

    mrb_value mrb_condition =
      grn_mrb_options_get_lit(mrb, mrb_options, "condition");
    if (!mrb_nil_p(mrb_condition)) {
      condition = DATA_PTR(mrb_condition);
    }
  }
  if (limit < 0) {
    limit = grn_table_size(ctx, table) + limit + 1;
  }
  GRN_OBJ_FORMAT_INIT(&format, n_hits, offset, limit, hits_offset);
  if (auto_flush) {
    format.flags |= GRN_OBJ_FORMAT_AUTO_FLUSH;
  }
  {
    grn_rc rc;
    rc = grn_obj_format_set_columns(ctx, &format,
                                    table, columns, columns_size);
    if (rc != GRN_SUCCESS) {
      grn_obj_format_fin(ctx, &format);
      grn_mrb_ctx_check(mrb);
    }
    if (format.expression && condition) {
      grn_expr_set_condition(ctx, format.expression, condition);
    }
  }
  if (only_content) {
    grn_ctx_output_table_records_content(ctx, table, &format);
  } else {
    grn_ctx_output_table_records(ctx, table, &format);
  }
  grn_obj_format_fin(ctx, &format);

  return mrb_nil_value();
}

static mrb_value
writer_write_table_records_content(mrb_state *mrb, mrb_value self)
{
  return writer_write_table_records_content_internal(mrb, self, true);
}

static mrb_value
writer_close_table_records(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_ctx_output_table_records_close(ctx);
  return mrb_nil_value();
}

static mrb_value
writer_write_table_records(mrb_state *mrb, mrb_value self)
{
  return writer_write_table_records_content_internal(mrb, self, false);
}

static mrb_value
writer_set_content_type(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_int content_type;

  mrb_get_args(mrb, "i", &content_type);

  grn_ctx_set_output_type(ctx, content_type);

  return mrb_nil_value();
}

void
grn_mrb_writer_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Writer", mrb->object_class);

  mrb_define_method(mrb, klass, "write", writer_write, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "flush", writer_flush, MRB_ARGS_REQ(0));
  mrb_define_method(mrb, klass, "open_array",
                    writer_open_array, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, klass, "close_array",
                    writer_close_array, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "open_map",
                    writer_open_map, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, klass, "close_map",
                    writer_close_map, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "write_table_columns",
                    writer_write_table_columns, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, klass, "open_table_records",
                    writer_open_table_records, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "write_table_records_content",
                    writer_write_table_records_content, MRB_ARGS_ARG(2, 1));
  mrb_define_method(mrb, klass, "close_table_records",
                    writer_close_table_records, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "write_table_records",
                    writer_write_table_records, MRB_ARGS_ARG(2, 1));

  mrb_define_method(mrb, klass, "content_type=",
                    writer_set_content_type, MRB_ARGS_REQ(1));
}
#endif
