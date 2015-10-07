/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

#include "grn_scanner.h"

static void
sis_free(grn_ctx *ctx, scan_info **sis, unsigned int n_sis)
{
  int i;
  for (i = 0; i < n_sis; i++) {
    grn_scan_info_close(ctx, sis[i]);
  }
  GRN_FREE(sis);
}

grn_scanner *
grn_scanner_open(grn_ctx *ctx,
                 grn_obj *expr,
                 grn_operator op,
                 grn_bool record_exist)
{
  grn_scanner *scanner;
  scan_info **sis;
  unsigned int n_sis;

  sis = grn_scan_info_build(ctx, expr, &n_sis, op, record_exist);
  if (!sis) {
    return NULL;
  }

  scanner = GRN_MALLOC(sizeof(grn_scanner));
  if (!scanner) {
    sis_free(ctx, sis, n_sis);
    return NULL;
  }

  scanner->expr = expr;
  scanner->rewritten_expr = NULL;
  scanner->sis = sis;
  scanner->n_sis = n_sis;

  return scanner;
}

void
grn_scanner_close(grn_ctx *ctx, grn_scanner *scanner)
{
  if (!scanner) {
    return;
  }

  if (scanner->rewritten_expr) {
    grn_obj_close(ctx, scanner->rewritten_expr);
  }

  if (scanner->sis) {
    sis_free(ctx, scanner->sis, scanner->n_sis);
  }

  GRN_FREE(scanner);
}
