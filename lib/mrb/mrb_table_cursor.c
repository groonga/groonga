/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014-2015 Brazil

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
#include <mruby/string.h>
#include <mruby/hash.h>

#include "mrb_ctx.h"
#include "mrb_bulk.h"
#include "mrb_table_cursor.h"
#include "mrb_options.h"

static struct mrb_data_type mrb_grn_table_cursor_type = {
  "Groonga::TableCursor",
  NULL
};

typedef struct {
  grn_obj from;
  grn_obj to;
  union {
    int64_t time_value;
  } value;
} border_value_buffer;

static void
mrb_value_to_border_value(mrb_state *mrb,
                          grn_id domain_id,
                          const char *type,
                          mrb_value mrb_border_value,
                          border_value_buffer *buffer,
                          void **border_value,
                          unsigned int *border_value_size)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_bool try_cast = GRN_FALSE;

  if (mrb_nil_p(mrb_border_value)) {
    return;
  }

  switch (mrb_type(mrb_border_value)) {
  case MRB_TT_STRING :
    switch (domain_id) {
    case GRN_DB_SHORT_TEXT :
    case GRN_DB_TEXT :
    case GRN_DB_LONG_TEXT :
      *border_value = RSTRING_PTR(mrb_border_value);
      *border_value_size = RSTRING_LEN(mrb_border_value);
      break;
    default :
      try_cast = GRN_TRUE;
      break;
    }
    break;
  default :
    {
      struct RClass *klass;

      klass = mrb_class(mrb, mrb_border_value);
      if (domain_id == GRN_DB_TIME &&
          klass == ctx->impl->mrb.builtin.time_class) {
        mrb_value mrb_sec;
        mrb_value mrb_usec;

        mrb_sec = mrb_funcall(mrb, mrb_border_value, "to_i", 0);
        mrb_usec = mrb_funcall(mrb, mrb_border_value, "usec", 0);
        buffer->value.time_value = GRN_TIME_PACK(mrb_fixnum(mrb_sec),
                                                 mrb_fixnum(mrb_usec));
        *border_value = &(buffer->value.time_value);
        *border_value_size = sizeof(buffer->value.time_value);
      } else {
        try_cast = GRN_TRUE;
      }
    }
    break;
  }

  if (!try_cast) {
    return;
  }

  grn_mrb_value_to_bulk(mrb, mrb_border_value, &(buffer->from));
  if (!grn_mrb_bulk_cast(mrb, &(buffer->from), &(buffer->to), domain_id)) {
    grn_obj *domain;
    char domain_name[GRN_TABLE_MAX_KEY_SIZE];
    int domain_name_size;

    domain = grn_ctx_at(ctx, domain_id);
    domain_name_size = grn_obj_name(ctx, domain, domain_name,
                                    GRN_TABLE_MAX_KEY_SIZE);
    mrb_raisef(mrb, E_ARGUMENT_ERROR,
               "failed to convert to %S: %S",
               mrb_str_new_static(mrb, domain_name, domain_name_size),
               mrb_border_value);
  }
  *border_value = GRN_BULK_HEAD(&(buffer->to));
  *border_value_size = GRN_BULK_VSIZE(&(buffer->to));
}

static mrb_value
mrb_grn_table_cursor_singleton_open_raw(mrb_state *mrb, mrb_value klass)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_table;
  mrb_value mrb_options = mrb_nil_value();
  grn_table_cursor *table_cursor;
  grn_obj *table;
  void *min = NULL;
  unsigned int min_size = 0;
  border_value_buffer min_buffer;
  void *max = NULL;
  unsigned int max_size = 0;
  border_value_buffer max_buffer;
  int offset = 0;
  int limit = -1;
  int flags = 0;

  mrb_get_args(mrb, "o|H", &mrb_table, &mrb_options);

  table = DATA_PTR(mrb_table);
  GRN_VOID_INIT(&(min_buffer.from));
  GRN_VOID_INIT(&(min_buffer.to));
  GRN_VOID_INIT(&(max_buffer.from));
  GRN_VOID_INIT(&(max_buffer.to));
  if (!mrb_nil_p(mrb_options)) {
    grn_id table_domain;
    mrb_value mrb_min;
    mrb_value mrb_max;
    mrb_value mrb_flags;

    table_domain = table->header.domain;

    mrb_min = grn_mrb_options_get_lit(mrb, mrb_options, "min");
    mrb_value_to_border_value(mrb, table_domain,
                              "min", mrb_min, &min_buffer, &min, &min_size);

    mrb_max = grn_mrb_options_get_lit(mrb, mrb_options, "max");
    mrb_value_to_border_value(mrb, table_domain,
                              "max", mrb_max, &max_buffer, &max, &max_size);

    mrb_flags = grn_mrb_options_get_lit(mrb, mrb_options, "flags");
    if (!mrb_nil_p(mrb_flags)) {
      flags = mrb_fixnum(mrb_flags);
    }
  }
  table_cursor = grn_table_cursor_open(ctx, table,
                                       min, min_size,
                                       max, max_size,
                                       offset, limit, flags);
  GRN_OBJ_FIN(ctx, &(min_buffer.from));
  GRN_OBJ_FIN(ctx, &(min_buffer.to));
  GRN_OBJ_FIN(ctx, &(max_buffer.from));
  GRN_OBJ_FIN(ctx, &(max_buffer.to));
  grn_mrb_ctx_check(mrb);

  return mrb_funcall(mrb, klass, "new", 1, mrb_cptr_value(mrb, table_cursor));
}

static mrb_value
mrb_grn_table_cursor_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_table_cursor_ptr;

  mrb_get_args(mrb, "o", &mrb_table_cursor_ptr);
  DATA_TYPE(self) = &mrb_grn_table_cursor_type;
  DATA_PTR(self) = mrb_cptr(mrb_table_cursor_ptr);

  return self;
}

static mrb_value
mrb_grn_table_cursor_close(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_table_cursor *table_cursor;

  table_cursor = DATA_PTR(self);
  if (table_cursor) {
    DATA_PTR(self) = NULL;
    grn_table_cursor_close(ctx, table_cursor);
    grn_mrb_ctx_check(mrb);
  }

  return mrb_nil_value();
}

static mrb_value
mrb_grn_table_cursor_next(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_id id;

  id = grn_table_cursor_next(ctx, DATA_PTR(self));
  grn_mrb_ctx_check(mrb);

  return mrb_fixnum_value(id);
}

static mrb_value
mrb_grn_table_cursor_count(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  int n_records = 0;

  while (grn_table_cursor_next(ctx, DATA_PTR(self)) != GRN_ID_NIL) {
    n_records++;
  }

  return mrb_fixnum_value(n_records);
}

void
grn_mrb_table_cursor_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "TableCursor", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_singleton_method(mrb, (struct RObject *)klass, "open_raw",
                              mrb_grn_table_cursor_singleton_open_raw,
                              MRB_ARGS_ARG(1, 1));

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_table_cursor_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "close",
                    mrb_grn_table_cursor_close, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "next",
                    mrb_grn_table_cursor_next, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "count",
                    mrb_grn_table_cursor_count, MRB_ARGS_NONE());
}
#endif
