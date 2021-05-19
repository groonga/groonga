/*
  Copyright(C) 2012-2018 Brazil

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

#include <groonga/plugin.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
  grn_tokenizer_query is a structure for storing a query. See the following
  functions.

  Deprecated since 8.0.2. Use accessors to get data.
 */
typedef struct _grn_tokenizer_query_deprecated grn_tokenizer_query;

struct _grn_tokenizer_query_deprecated {
  grn_obj *normalized_query;
  char *query_buf;
  const char *ptr;
  unsigned int length;
  grn_encoding encoding;
  unsigned int flags;
  grn_bool have_tokenized_delimiter;
  /* Deprecated since 4.0.8. Use tokenize_mode instead. */
  grn_token_mode token_mode;
  grn_tokenize_mode tokenize_mode;
};

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */
