/*
  Copyright(C) 2018  Brazil
  Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_highlighter grn_highlighter;

GRN_API grn_highlighter *
grn_highlighter_open(grn_ctx *ctx);
GRN_API grn_rc
grn_highlighter_close(grn_ctx *ctx, grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_highlight(grn_ctx *ctx,
                          grn_highlighter *highlighter,
                          const char *text,
                          int64_t text_length,
                          grn_obj *output);
GRN_API grn_rc
grn_highlighter_set_lexicon(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            grn_obj *lexicon);
GRN_API grn_obj *
grn_highlighter_get_lexicon(grn_ctx *ctx, grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_set_cycled_class_tag_mode(grn_ctx *ctx,
                                          grn_highlighter *highlighter,
                                          bool mode);
GRN_API bool
grn_highlighter_get_cycled_class_tag_mode(grn_ctx *ctx,
                                          grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_set_default_open_tag(grn_ctx *ctx,
                                     grn_highlighter *highlighter,
                                     const char *tag,
                                     int64_t tag_length);
GRN_API const char *
grn_highlighter_get_default_open_tag(grn_ctx *ctx,
                                     grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_set_default_close_tag(grn_ctx *ctx,
                                      grn_highlighter *highlighter,
                                      const char *tag,
                                      int64_t tag_length);
GRN_API const char *
grn_highlighter_get_default_close_tag(grn_ctx *ctx,
                                      grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_set_normalizers(grn_ctx *ctx,
                                grn_highlighter *highlighter,
                                const char *normalizers,
                                int64_t normalizers_length);
GRN_API grn_obj *
grn_highlighter_get_normalizers(grn_ctx *ctx, grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_set_html_mode(grn_ctx *ctx,
                              grn_highlighter *highlighter,
                              bool html_mode);
GRN_API bool
grn_highlighter_get_html_mode(grn_ctx *ctx, grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_add_keyword(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            const char *keyword,
                            int64_t keyword_length);
GRN_API grn_rc
grn_highlighter_clear_keywords(grn_ctx *ctx, grn_highlighter *highlighter);
GRN_API grn_rc
grn_highlighter_add_open_tag(grn_ctx *ctx,
                             grn_highlighter *highlighter,
                             const char *tag,
                             int64_t tag_length);
GRN_API grn_rc
grn_highlighter_add_close_tag(grn_ctx *ctx,
                              grn_highlighter *highlighter,
                              const char *tag,
                              int64_t tag_length);
GRN_API grn_rc
grn_highlighter_clear_tags(grn_ctx *ctx, grn_highlighter *highlighter);

#ifdef __cplusplus
}
#endif
