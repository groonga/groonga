/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2021-2023  Sutou Kouhei <kou@clear-code.com>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "grn.h"
#include "grn_str.h"
#include "grn_db.h"
#include "grn_onigmo.h"

#define ASIZE                      256U
#define MAX_SNIP_TAG_COUNT         512U
#define DEFAULT_SNIP_COND_CAPACITY 32U
#define MAX_SNIP_RESULT_COUNT      16U

#ifdef __cplusplus
extern "C" {
#endif

#define SNIPCOND_NONSTOP          0
#define SNIPCOND_STOP             1
#define SNIPCOND_ACROSS           2

#define GRN_QUERY_SCAN_ALLOCCONDS 0x0002

typedef struct _snip_cond {
  /* initial parameters */
  const char *opentag;
  const char *closetag;
  size_t opentag_len;
  size_t closetag_len;
  grn_obj *keyword;

  /* Tuned BM pre */
  ptrdiff_t bmBc[ASIZE];
  ptrdiff_t shift;

  /* Tuned BM temporal result */
  ptrdiff_t found;
  ptrdiff_t last_found;
  size_t last_offset;
  size_t start_offset;
  size_t end_offset;
  ptrdiff_t found_alpha_head;

  /* search result */
  int count;

  /* stop flag */
  int_least8_t stopflag;
} snip_cond;

typedef struct {
  size_t start_offset;
  size_t end_offset;
  snip_cond *cond;
} _snip_tag_result;

typedef struct {
  size_t start_offset;
  size_t end_offset;
  unsigned int first_tag_result_idx;
  unsigned int last_tag_result_idx;
  unsigned int tag_count;
} _snip_result;

typedef struct _grn_snip {
  grn_db_obj obj;
  grn_encoding encoding;
  int flags;
  size_t width;
  unsigned int max_results;
  const char *defaultopentag;
  const char *defaultclosetag;
  size_t defaultopentag_len;
  size_t defaultclosetag_len;

  grn_snip_mapping *mapping;

  snip_cond *cond;
  size_t cond_capacity;
  size_t cond_len;

  unsigned int tag_count;
  unsigned int snip_count;

  const char *string;
  grn_obj *nstr;

  _snip_result snip_result[MAX_SNIP_RESULT_COUNT];
  _snip_tag_result tag_result[MAX_SNIP_TAG_COUNT];

  size_t max_tagged_len;

  grn_obj *normalizer;
  grn_obj *lexicon;
  grn_obj normalizers;

  char *delimiter_pattern;
  size_t delimiter_pattern_length;
#ifdef GRN_SUPPORT_REGEXP
  OnigRegex delimiter_regex;
#endif
} grn_snip;

grn_rc
grn_snip_close(grn_ctx *ctx, grn_snip *snip);
grn_rc
grn_snip_cond_close(grn_ctx *ctx, snip_cond *cond);
void
grn_bm_tunedbm(grn_ctx *ctx, snip_cond *cond, grn_obj *string, int flags);

#ifdef __cplusplus
}
#endif
