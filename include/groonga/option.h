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

#ifdef __cplusplus
extern "C" {
#endif

typedef void * grn_option_revision;
#define GRN_OPTION_REVISION_NONE ((grn_option_revision)0)
#define GRN_OPTION_REVISION_UNCHANGED ((grn_option_revision)1)

#define GRN_OPTION_VALUES_EACH_BEGIN(ctx,                       \
                                     option_values,             \
                                     i,                         \
                                     name,                      \
                                     name_size) do {            \
  grn_ctx *ctx_ = (ctx);                                        \
  grn_obj *option_values_ = (option_values);                    \
  unsigned int i_, n_;                                          \
                                                                \
  n_ = grn_vector_size(ctx_, option_values_);                   \
  for (i_ = 0; i_ < n_; i_ += 2) {                              \
    unsigned int i = i_ + 1;                                    \
    const char *name;                                           \
    unsigned int name_size;                                     \
    grn_id name_domain_;                                        \
                                                                \
    name_size = grn_vector_get_element(ctx_,                    \
                                       option_values_,          \
                                       i_,                      \
                                       &name,                   \
                                       NULL,                    \
                                       &name_domain_);          \
    if (!grn_type_id_is_text_family(ctx_, name_domain_)) {      \
      continue;                                                 \
    }

#define GRN_OPTION_VALUES_EACH_END()            \
  }                                             \
} while (GRN_FALSE)

#ifdef __cplusplus
}
#endif
