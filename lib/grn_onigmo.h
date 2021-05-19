/*
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#ifdef GRN_SUPPORT_REGEXP
# include <onigmo.h>
#endif /* GRN_SUPPORT_REGEXP */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GRN_SUPPORT_REGEXP

#define GRN_ONIGMO_OPTION_DEFAULT               \
  (ONIG_OPTION_ASCII_RANGE |                    \
   ONIG_OPTION_MULTILINE)

#define GRN_ONIGMO_SYNTAX_DEFAULT ONIG_SYNTAX_RUBY

grn_bool
grn_onigmo_is_valid_encoding(grn_ctx *ctx);

GRN_API OnigRegex
grn_onigmo_new(grn_ctx *ctx,
               const char *pattern,
               size_t pattern_length,
               OnigOptionType option,
               const OnigSyntaxType *syntax,
               const char *context);

#endif /* GRN_SUPPORT_REGEXP */

#ifdef __cplusplus
}
#endif
