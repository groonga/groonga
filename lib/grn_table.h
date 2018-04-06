/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Brazil

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

typedef struct {
  grn_obj *proc;
  void *options;
  grn_option_revision options_revision;
  grn_close_func options_close_func;
  grn_critical_section lock;
} grn_table_tokenizer;

void grn_table_tokenizer_init(grn_ctx *ctx,
                              grn_table_tokenizer *tokenizer,
                              grn_id tokenizer_id);
void grn_table_tokenizer_set_proc(grn_ctx *ctx,
                                  grn_table_tokenizer *tokenizer,
                                  grn_obj *proc);
void grn_table_tokenizer_set_options(grn_ctx *ctx,
                                     grn_table_tokenizer *tokenizer,
                                     void *options,
                                     grn_option_revision revision,
                                     grn_close_func close_func);
void grn_table_tokenizer_fin(grn_ctx *ctx,
                             grn_table_tokenizer *tokenizer);

#ifdef __cplusplus
}
#endif
