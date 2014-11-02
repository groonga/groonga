/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014 Brazil

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

#include "grn.h"

static const char *operator_names[] = {
  "push",
  "pop",
  "nop",
  "call",
  "intern",
  "get_ref",
  "get_value",
  "and",
  "and_not",
  "or",
  "assign",
  "star_assign",
  "slash_assign",
  "mod_assign",
  "plus_assign",
  "minus_assign",
  "shiftl_assign",
  "shiftr_assign",
  "shiftrr_assign",
  "and_assign",
  "xor_assign",
  "or_assign",
  "jump",
  "cjump",
  "comma",
  "bitwise_or",
  "bitwise_xor",
  "bitwise_and",
  "bitwise_not",
  "equal",
  "not_equal",
  "less",
  "greater",
  "less_equal",
  "greater_equal",
  "in",
  "match",
  "near",
  "near2",
  "similar",
  "term_extract",
  "shiftl",
  "shiftr",
  "shiftrr",
  "plus",
  "minus",
  "star",
  "slash",
  "mod",
  "delete",
  "incr",
  "decr",
  "incr_post",
  "decr_post",
  "not",
  "adjust",
  "exact",
  "lcp",
  "partial",
  "unsplit",
  "prefix",
  "suffix",
  "geo_distance1",
  "geo_distance2",
  "geo_distance3",
  "geo_distance4",
  "geo_withinp5",
  "geo_withinp6",
  "geo_withinp8",
  "obj_search",
  "expr_get_var",
  "table_create",
  "table_select",
  "table_sort",
  "table_group",
  "json_put",
  "get_member"
};

const char *
grn_operator_to_string(grn_operator op)
{
  if (GRN_OP_PUSH <= op && op <= GRN_OP_GET_MEMBER) {
    return operator_names[op];
  } else {
    return "unknown";
  }
}
