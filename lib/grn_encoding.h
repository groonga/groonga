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

/*
 * grn_ctx::errbuf: grn_encoding
 * grn_logger_put: grn_encoding
 * mruby: UTF-8
 * path: locale
 */

GRN_API const char *
grn_encoding_convert_to_locale(grn_ctx *ctx,
                               const char *grn_encoding_string,
                               ssize_t grn_encoding_string_size,
                               size_t *converted_string_size);
GRN_API const char *
grn_encoding_convert_to_utf8(grn_ctx *ctx,
                             const char *grn_encoding_string,
                             ssize_t grn_encoding_string_size,
                             size_t *converted_string_size);
GRN_API const char *
grn_encoding_convert_from_locale(grn_ctx *ctx,
                                 const char *locale_string,
                                 ssize_t locale_string_size,
                                 size_t *converted_string_size);
GRN_API const char *
grn_encoding_convert_from_utf8(grn_ctx *ctx,
                               const char *utf8_string,
                               ssize_t utf8_string_size,
                               size_t *converted_string_size);
GRN_API const char *
grn_encoding_convert_to_utf8_from_locale(grn_ctx *ctx,
                                         const char *locale_string,
                                         ssize_t locale_string_size,
                                         size_t *converted_string_size);
GRN_API const char *
grn_encoding_convert_to_locale_from_utf8(grn_ctx *ctx,
                                         const char *utf8_string,
                                         ssize_t utf8_string_size,
                                         size_t *converted_string_size);
GRN_API void
grn_encoding_converted_free(grn_ctx *ctx,
                            const char *converted_string);

#ifdef __cplusplus
}
#endif
