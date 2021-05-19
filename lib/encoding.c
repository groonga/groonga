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

#include <stdio.h>

#include "grn.h"
#include "grn_ctx.h"
#include "grn_encoding.h"
#include "grn_windows.h"

#include <string.h>

#ifdef WIN32
static const char *
grn_encoding_convert(grn_ctx *ctx,
                     const char *context,
                     UINT from_code_page,
                     UINT to_code_page,
                     const char *from_string,
                     ssize_t from_string_size,
                     size_t *converted_string_size)
{
  char *converted_string = NULL;

  if (from_string_size < 0) {
    from_string_size = strlen(from_string);
  }

  if (from_code_page == to_code_page) {
    goto exit;
  }

  {
    WCHAR *utf16_string;
    DWORD n_utf16_chars;
    size_t converted_string_size_;

    n_utf16_chars = MultiByteToWideChar(from_code_page,
                                        0,
                                        from_string,
                                        from_string_size,
                                        NULL,
                                        0);
    if (n_utf16_chars == 0) {
      SERR("%s failed to estimate the number of UTF-16 characters",
           context);
      goto exit;
    }
    utf16_string = GRN_MALLOCN(WCHAR, n_utf16_chars);
    n_utf16_chars = MultiByteToWideChar(from_code_page,
                                        0,
                                        from_string,
                                        from_string_size,
                                        utf16_string,
                                        n_utf16_chars);
    if (n_utf16_chars == 0) {
      SERR("%s failed to convert to UTF-16 characters",
           context);
      GRN_FREE(utf16_string);
      goto exit;
    }

    converted_string_size_ = WideCharToMultiByte(to_code_page,
                                                 0,
                                                 utf16_string,
                                                 n_utf16_chars,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 NULL);
    if (converted_string_size_ == 0) {
      SERR("%s failed to estimate required buffer size for converted string",
           context);
      GRN_FREE(utf16_string);
      goto exit;
    }

    converted_string = GRN_MALLOCN(char, converted_string_size_ + 1);
    converted_string_size_ = WideCharToMultiByte(to_code_page,
                                                 0,
                                                 utf16_string,
                                                 n_utf16_chars,
                                                 converted_string,
                                                 converted_string_size_,
                                                 NULL,
                                                 NULL);
    GRN_FREE(utf16_string);
    if (converted_string_size_ == 0) {
      SERR("%s failed to estimate required buffer size for converted string",
           context);
      GRN_FREE(converted_string);
      converted_string = NULL;
      goto exit;
    }
    converted_string[converted_string_size_] = '\0';
    if (converted_string_size) {
      *converted_string_size = converted_string_size_;
    }
  }

exit :
  if (!converted_string) {
    converted_string = GRN_MALLOCN(char, from_string_size + 1);
    if (converted_string) {
      grn_memcpy(converted_string, from_string, from_string_size);
      converted_string[from_string_size] = '\0';
      if (converted_string_size) {
        *converted_string_size = from_string_size;
      }
    } else {
      ERR(ctx->rc,
          "%s failed to allocate a buffer for converted string",
          context);
      if (converted_string_size) {
        *converted_string_size = 0;
      }
    }
  }

  return converted_string;
}

const char *
grn_encoding_convert_to_locale(grn_ctx *ctx,
                               const char *grn_encoding_string,
                               ssize_t grn_encoding_string_size,
                               size_t *converted_string_size)
{
  return grn_encoding_convert(ctx,
                              "[encoding][convert][grn->locale]",
                              grn_windows_encoding_to_code_page(ctx->encoding),
                              CP_ACP,
                              grn_encoding_string,
                              grn_encoding_string_size,
                              converted_string_size);
}

const char *
grn_encoding_convert_to_utf8(grn_ctx *ctx,
                             const char *grn_encoding_string,
                             ssize_t grn_encoding_string_size,
                             size_t *converted_string_size)
{
  return grn_encoding_convert(ctx,
                              "[encoding][convert][grn->utf8]",
                              grn_windows_encoding_to_code_page(ctx->encoding),
                              CP_UTF8,
                              grn_encoding_string,
                              grn_encoding_string_size,
                              converted_string_size);
}

const char *
grn_encoding_convert_from_locale(grn_ctx *ctx,
                                 const char *locale_string,
                                 ssize_t locale_string_size,
                                 size_t *converted_string_size)
{
  return grn_encoding_convert(ctx,
                              "[encoding][convert][locale->grn]",
                              CP_ACP,
                              grn_windows_encoding_to_code_page(ctx->encoding),
                              locale_string,
                              locale_string_size,
                              converted_string_size);
}

const char *
grn_encoding_convert_from_utf8(grn_ctx *ctx,
                               const char *utf8_string,
                               ssize_t utf8_string_size,
                               size_t *converted_string_size)
{
  return grn_encoding_convert(ctx,
                              "[encoding][convert][utf8->grn]",
                              CP_UTF8,
                              grn_windows_encoding_to_code_page(ctx->encoding),
                              utf8_string,
                              utf8_string_size,
                              converted_string_size);
}

const char *
grn_encoding_convert_to_utf8_from_locale(grn_ctx *ctx,
                                         const char *locale_string,
                                         ssize_t locale_string_size,
                                         size_t *converted_string_size)
{
  return grn_encoding_convert(ctx,
                              "[encoding][convert][locale->utf8]",
                              CP_ACP,
                              CP_UTF8,
                              locale_string,
                              locale_string_size,
                              converted_string_size);
}

const char *
grn_encoding_convert_to_locale_from_utf8(grn_ctx *ctx,
                                         const char *utf8_string,
                                         ssize_t utf8_string_size,
                                         size_t *converted_string_size)
{
  return grn_encoding_convert(ctx,
                              "[encoding][convert][utf8->locale]",
                              CP_UTF8,
                              CP_ACP,
                              utf8_string,
                              utf8_string_size,
                              converted_string_size);
}

void
grn_encoding_converted_free(grn_ctx *ctx, const char *converted_string)
{
  GRN_FREE((char *)converted_string);
}
#else /* WIN32 */
const char *
grn_encoding_convert_to_locale(grn_ctx *ctx,
                               const char *grn_encoding_string,
                               ssize_t grn_encoding_string_size,
                               size_t *converted_string_size)
{
  if (converted_string_size) {
    if (grn_encoding_string_size < 0) {
      *converted_string_size = strlen(grn_encoding_string);
    } else {
      *converted_string_size = grn_encoding_string_size;
    }
  }
  return grn_encoding_string;
}

const char *
grn_encoding_convert_to_utf8(grn_ctx *ctx,
                             const char *grn_encoding_string,
                             ssize_t grn_encoding_string_size,
                             size_t *converted_string_size)
{
  if (converted_string_size) {
    if (grn_encoding_string_size < 0) {
      *converted_string_size = strlen(grn_encoding_string);
    } else {
      *converted_string_size = grn_encoding_string_size;
    }
  }
  return grn_encoding_string;
}

const char *
grn_encoding_convert_from_locale(grn_ctx *ctx,
                                 const char *locale_string,
                                 ssize_t locale_string_size,
                                 size_t *converted_string_size)
{
  if (converted_string_size) {
    if (locale_string_size < 0) {
      *converted_string_size = strlen(locale_string);
    } else {
      *converted_string_size = locale_string_size;
    }
  }
  return locale_string;
}

const char *
grn_encoding_convert_from_utf8(grn_ctx *ctx,
                               const char *utf8_string,
                               ssize_t utf8_string_size,
                               size_t *converted_string_size)
{
  if (converted_string_size) {
    if (utf8_string_size < 0) {
      *converted_string_size = strlen(utf8_string);
    } else {
      *converted_string_size = utf8_string_size;
    }
  }
  return utf8_string;
}

const char *
grn_encoding_convert_to_utf8_from_locale(grn_ctx *ctx,
                                         const char *locale_string,
                                         ssize_t locale_string_size,
                                         size_t *converted_string_size)
{
  if (converted_string_size) {
    if (locale_string_size < 0) {
      *converted_string_size = strlen(locale_string);
    } else {
      *converted_string_size = locale_string_size;
    }
  }
  return locale_string;
}

const char *
grn_encoding_convert_to_locale_from_utf8(grn_ctx *ctx,
                                         const char *utf8_string,
                                         ssize_t utf8_string_size,
                                         size_t *converted_string_size)
{
  if (converted_string_size) {
    if (utf8_string_size < 0) {
      *converted_string_size = strlen(utf8_string);
    } else {
      *converted_string_size = utf8_string_size;
    }
  }
  return utf8_string;
}

void
grn_encoding_converted_free(grn_ctx *ctx, const char *converted_string)
{
}
#endif /* WIN32 */
