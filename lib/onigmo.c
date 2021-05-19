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

#include "grn_ctx.h"
#include "grn_onigmo.h"

#ifdef GRN_SUPPORT_REGEXP
grn_bool
grn_onigmo_is_valid_encoding(grn_ctx *ctx)
{
  switch (ctx->encoding) {
  case GRN_ENC_EUC_JP :
  case GRN_ENC_UTF8 :
  case GRN_ENC_SJIS :
  case GRN_ENC_LATIN1 :
  case GRN_ENC_KOI8R :
    return GRN_TRUE;
  default :
    return GRN_FALSE;
  }
}

OnigRegex
grn_onigmo_new(grn_ctx *ctx,
               const char *pattern,
               size_t pattern_length,
               OnigOptionType option,
               const OnigSyntaxType *syntax,
               const char *context)
{
  OnigEncoding onig_encoding;
  OnigRegex onig_regex;
  int onig_result;
  OnigErrorInfo onig_error_info;

  switch (ctx->encoding) {
  case GRN_ENC_EUC_JP :
    onig_encoding = ONIG_ENCODING_EUC_JP;
    break;
  case GRN_ENC_UTF8 :
    onig_encoding = ONIG_ENCODING_UTF8;
    break;
  case GRN_ENC_SJIS :
    onig_encoding = ONIG_ENCODING_CP932;
    break;
  case GRN_ENC_LATIN1 :
    onig_encoding = ONIG_ENCODING_ISO_8859_1;
    break;
  case GRN_ENC_KOI8R :
    onig_encoding = ONIG_ENCODING_KOI8_R;
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT,
        "%s[regexp][new] invalid encoding: <%.*s>: <%s>",
        context,
        (int)pattern_length,
        pattern,
        grn_encoding_to_string(ctx->encoding));
    return NULL;
  }

  onig_result = onig_new(&onig_regex,
                         pattern,
                         pattern + pattern_length,
                         option,
                         onig_encoding,
                         syntax,
                         &onig_error_info);
  if (onig_result != ONIG_NORMAL) {
    char message[ONIG_MAX_ERROR_MESSAGE_LEN];
    onig_error_code_to_str(message, onig_result, &onig_error_info);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[regexp][new] "
        "failed to create regular expression object: <%.*s>: %s",
        context,
        (int)pattern_length,
        pattern,
        message);
    return NULL;
  }

  return onig_regex;
}

#endif /* GRN_SUPPORT_REGEXP */
