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

#include "grn_ctx.h"
#include "grn_posting.h"

grn_posting *
grn_posting_open(grn_ctx *ctx)
{
  GRN_API_ENTER;
  grn_posting *posting = GRN_CALLOC(sizeof(grn_posting));
  if (!posting) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "[posting][open] failed to allocate");
  }
  GRN_API_RETURN(posting);
}

void
grn_posting_close(grn_ctx *ctx, grn_posting *posting)
{
  GRN_API_ENTER;
  GRN_FREE(posting);
  GRN_API_RETURN();
}

grn_id
grn_posting_get_record_id(grn_ctx *ctx, grn_posting *posting)
{
  return posting->rid;
}

uint32_t
grn_posting_get_section_id(grn_ctx *ctx, grn_posting *posting)
{
  return posting->sid;
}

uint32_t
grn_posting_get_position(grn_ctx *ctx, grn_posting *posting)
{
  return posting->pos;
}

uint32_t
grn_posting_get_tf(grn_ctx *ctx, grn_posting *posting)
{
  return posting->tf;
}

uint32_t
grn_posting_get_weight(grn_ctx *ctx, grn_posting *posting)
{
  return posting->weight;
}

float
grn_posting_get_weight_float(grn_ctx *ctx, grn_posting *posting)
{
  return ((grn_posting_internal *)posting)->weight_float;
}

uint32_t
grn_posting_get_rest(grn_ctx *ctx, grn_posting *posting)
{
  return posting->rest;
}

void
grn_posting_set_weight(grn_ctx *ctx, grn_posting *posting, uint32_t weight)
{
  posting->weight = weight;
  ((grn_posting_internal *)posting)->weight_float = weight;
}

void
grn_posting_set_weight_float(grn_ctx *ctx, grn_posting *posting, float weight)
{
  posting->weight = weight;
  ((grn_posting_internal *)posting)->weight_float = weight;
}
