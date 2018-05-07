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

#include "grn.h"
#include "grn_pat.h"

/*
 * TODO:
 *   * Support non HTML mode.
 *   * Support custom tag.
 */
struct _grn_highlighter {
  grn_obj_header header;

  grn_bool is_html_mode;
  grn_bool need_prepared;
  grn_obj raw_keywords;

  struct {
    const char *open;
    size_t open_length;
    const char *close;
    size_t close_length;
  } tag;

  /* For lexicon mode */
  grn_obj *lexicon;
  grn_obj keyword_token_ids;

  /* For patricia trie mode */
  grn_obj *keywords;
};

grn_highlighter *
grn_highlighter_open(grn_ctx *ctx)
{
  grn_highlighter *highlighter;

  GRN_API_ENTER;

  highlighter = GRN_MALLOCN(grn_highlighter, 1);
  if (!highlighter) {
    ERR(ctx->rc,
        "[highlighter][open] failed to allocate memory: %s",
        ctx->errbuf);
    GRN_API_RETURN(NULL);
  }

  highlighter->header.type = GRN_HIGHLIGHTER;
  highlighter->header.impl_flags = 0;
  highlighter->header.flags = 0;
  highlighter->header.domain = GRN_ID_NIL;

  highlighter->is_html_mode = GRN_TRUE;
  highlighter->need_prepared = GRN_TRUE;
  GRN_TEXT_INIT(&(highlighter->raw_keywords), GRN_OBJ_VECTOR);

  highlighter->tag.open = "<span class=\"keyword\">";
  highlighter->tag.open_length = strlen(highlighter->tag.open);
  highlighter->tag.close = "</span>";
  highlighter->tag.close_length = strlen(highlighter->tag.close);

  highlighter->lexicon = NULL;
  GRN_RECORD_INIT(&(highlighter->keyword_token_ids), GRN_OBJ_VECTOR, GRN_ID_NIL);

  highlighter->keywords = NULL;

  GRN_API_RETURN(highlighter);
}

grn_rc
grn_highlighter_close(grn_ctx *ctx,
                      grn_highlighter *highlighter)
{
  GRN_API_ENTER;

  if (!highlighter) {
    GRN_API_RETURN(ctx->rc);
  }

  if (highlighter->keywords) {
    grn_obj_close(ctx, highlighter->keywords);
  }

  GRN_OBJ_FIN(ctx, &(highlighter->keyword_token_ids));

  GRN_OBJ_FIN(ctx, &(highlighter->raw_keywords));
  GRN_FREE(highlighter);

  GRN_API_RETURN(ctx->rc);
}

static void
grn_highlighter_prepare_lexicon(grn_ctx *ctx,
                                grn_highlighter *highlighter)
{
  /* TODO */
}

static void
grn_highlighter_prepare_patricia_trie(grn_ctx *ctx,
                                      grn_highlighter *highlighter)
{
  if (highlighter->keywords) {
    grn_obj_close(ctx, highlighter->keywords);
  }

  highlighter->keywords = grn_table_create(ctx,
                                           NULL, 0,
                                           NULL,
                                           GRN_OBJ_TABLE_PAT_KEY,
                                           grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                           NULL);
  if (!highlighter->keywords) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_UNKNOWN_ERROR;
    }
    ERR(rc,
        "[highlighter][prepare][no-lexicon] "
        "failed to create an internal patricia trie: %s",
        ctx->errbuf);
    return;
  }

  grn_obj_set_info(ctx,
                   highlighter->keywords,
                   GRN_INFO_NORMALIZER,
                   grn_ctx_get(ctx, "NormalizerAuto", -1));

  {
    unsigned int i, n;

    n = grn_vector_size(ctx, &(highlighter->raw_keywords));
    for (i = 0; i < n; i++) {
      const char *keyword;
      unsigned int keyword_size;

      keyword_size = grn_vector_get_element(ctx,
                                            &(highlighter->raw_keywords),
                                            i,
                                            &keyword,
                                            NULL,
                                            NULL);
      grn_table_add(ctx,
                    highlighter->keywords,
                    keyword,
                    keyword_size,
                    NULL);
    }
  }
}

static void
grn_highlighter_prepare(grn_ctx *ctx,
                        grn_highlighter *highlighter)
{
  if (highlighter->lexicon) {
    grn_highlighter_prepare_lexicon(ctx, highlighter);
  } else {
    grn_highlighter_prepare_patricia_trie(ctx, highlighter);
  }
}

static void
grn_highlighter_highlight_lexicon(grn_ctx *ctx,
                                  grn_highlighter *highlighter,
                                  const char *text,
                                  size_t text_length,
                                  grn_obj *output)
{
  /* TODO */
}

static void
grn_highlighter_highlight_patricia_trie(grn_ctx *ctx,
                                        grn_highlighter *highlighter,
                                        const char *text,
                                        size_t text_length,
                                        grn_obj *output)
{
  const char *current = text;
  size_t current_length = text_length;

  while (current_length > 0) {
#define MAX_N_HITS 16
    grn_pat_scan_hit hits[MAX_N_HITS];
    const char *rest;
    int i, n_hits;
    size_t previous_length = 0;
    size_t chunk_length;

    n_hits = grn_pat_scan(ctx,
                          (grn_pat *)(highlighter->keywords),
                          current, current_length,
                          hits, MAX_N_HITS,
                          &rest);
    for (i = 0; i < n_hits; i++) {
      if ((hits[i].offset - previous_length) > 0) {
        grn_text_escape_xml(ctx,
                            output,
                            current + previous_length,
                            hits[i].offset - previous_length);
      }
      GRN_TEXT_PUT(ctx,
                   output,
                   highlighter->tag.open,
                   highlighter->tag.open_length);
      grn_text_escape_xml(ctx,
                          output,
                          current + hits[i].offset,
                          hits[i].length);
      GRN_TEXT_PUT(ctx,
                   output,
                   highlighter->tag.close,
                   highlighter->tag.close_length);
      previous_length = hits[i].offset + hits[i].length;
    }

    chunk_length = rest - current;
    if ((chunk_length - previous_length) > 0) {
      grn_text_escape_xml(ctx,
                          output,
                          current + previous_length,
                          current_length - previous_length);
    }
    current_length -= chunk_length;
    current = rest;
#undef MAX_N_HITS
  }
}

grn_rc
grn_highlighter_highlight(grn_ctx *ctx,
                          grn_highlighter *highlighter,
                          const char *text,
                          int64_t text_length,
                          grn_obj *output)
{
  GRN_API_ENTER;

  if (text_length < 0) {
    text_length = strlen(text);
  }

  if (grn_vector_size(ctx, &(highlighter->raw_keywords)) == 0) {
    if (highlighter->is_html_mode) {
      grn_text_escape_xml(ctx,
                          output,
                          text,
                          text_length);
    } else {
      GRN_TEXT_PUT(ctx, output, text, text_length);
    }
    goto exit;
  }

  if (highlighter->need_prepared) {
    grn_highlighter_prepare(ctx, highlighter);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

  if (highlighter->lexicon) {
    grn_highlighter_highlight_lexicon(ctx,
                                      highlighter,
                                      text,
                                      text_length,
                                      output);
  } else {
    grn_highlighter_highlight_patricia_trie(ctx,
                                            highlighter,
                                            text,
                                            text_length,
                                            output);
  }

exit :
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_set_lexicon(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            grn_obj *lexicon)
{
  GRN_API_ENTER;

  highlighter->lexicon = lexicon;

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_highlighter_get_lexicon(grn_ctx *ctx,
                            grn_highlighter *highlighter)
{
  GRN_API_ENTER;

  GRN_API_RETURN(highlighter->lexicon);
}

grn_rc
grn_highlighter_add_keyword(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            const char *keyword,
                            int64_t keyword_length)
{
  GRN_API_ENTER;

  if (keyword_length < 0) {
    keyword_length = strlen(keyword);
  }

  if (keyword_length == 0) {
    goto exit;
  }

  grn_vector_add_element(ctx,
                         &(highlighter->raw_keywords),
                         keyword,
                         keyword_length,
                         0,
                         GRN_DB_TEXT);
  highlighter->need_prepared = GRN_TRUE;

exit :
  GRN_API_RETURN(ctx->rc);
}
