/*
  Copyright (C) 2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_token_cursor.h"

#include "grn_pat.h"
#include "grn_str.h"

#define GRN_HIGHLIGHTER_DEFAULT_OPEN_TAG  "<span class=\"keyword\">"
#define GRN_HIGHLIGHTER_DEFAULT_CLOSE_TAG "</span>"

typedef struct {
  grn_id chunk_id;
  uint64_t offset;
  uint32_t length;
  bool have_overlap;
  uint32_t first_character_length;
} grn_highlighter_location;

static int
grn_highlighter_location_compare(const void *data1, const void *data2)
{
  const grn_highlighter_location *location1 = data1;
  const grn_highlighter_location *location2 = data2;

  if (location1->offset == location2->offset) {
    return (int)(location1->length - location2->length);
  } else {
    return (int)(location1->offset - location2->offset);
  }
}

struct _grn_highlighter {
  grn_obj_header header;

  bool is_html_mode;
  bool need_prepared;
  grn_obj raw_keywords;

  bool is_sequential_class_tag_mode;
  struct {
    grn_obj open;
    grn_obj close;
  } sequential_class_tag;

  struct {
    grn_obj open;
    grn_obj close;
  } default_tag;

  grn_obj open_tags;
  grn_obj close_tags;
  size_t n_tags;

  /* For lexicon mode */
  struct {
    grn_obj *object;
    grn_encoding encoding;
    grn_obj lazy_keywords;
    grn_obj lazy_keyword_ids;
    grn_obj *token_id_chunks;
    grn_obj token_id_chunk;
    grn_obj token_ids;
    grn_obj token_locations;
    grn_obj candidates;
  } lexicon;

  /* For patricia trie mode */
  struct {
    grn_obj *keywords;
    grn_obj normalizers;
  } pat;
};

grn_highlighter *
grn_highlighter_open(grn_ctx *ctx)
{
  grn_highlighter *highlighter;

  GRN_API_ENTER;

  highlighter = GRN_CALLOC(sizeof(grn_highlighter));
  if (!highlighter) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc, "[highlighter][open] failed to allocate memory: %s", errbuf);
    GRN_API_RETURN(NULL);
  }

  highlighter->header.type = GRN_HIGHLIGHTER;
  highlighter->header.impl_flags = 0;
  highlighter->header.flags = 0;
  highlighter->header.domain = GRN_ID_NIL;

  highlighter->is_html_mode = true;
  highlighter->need_prepared = true;
  GRN_TEXT_INIT(&(highlighter->raw_keywords), GRN_OBJ_VECTOR);

  highlighter->is_sequential_class_tag_mode = false;
  GRN_TEXT_INIT(&(highlighter->sequential_class_tag.open), 0);
  GRN_TEXT_INIT(&(highlighter->sequential_class_tag.close), 0);

  GRN_TEXT_INIT(&(highlighter->default_tag.open), 0);
  grn_highlighter_set_default_open_tag(ctx,
                                       highlighter,
                                       GRN_HIGHLIGHTER_DEFAULT_OPEN_TAG,
                                       -1);
  GRN_TEXT_INIT(&(highlighter->default_tag.close), 0);
  grn_highlighter_set_default_close_tag(ctx,
                                        highlighter,
                                        GRN_HIGHLIGHTER_DEFAULT_CLOSE_TAG,
                                        -1);

  GRN_TEXT_INIT(&(highlighter->open_tags), GRN_OBJ_VECTOR);
  GRN_TEXT_INIT(&(highlighter->close_tags), GRN_OBJ_VECTOR);
  highlighter->n_tags = 0;

  highlighter->lexicon.object = NULL;
  highlighter->lexicon.encoding = GRN_ENC_NONE;
  GRN_TEXT_INIT(&(highlighter->lexicon.lazy_keywords), GRN_OBJ_VECTOR);
  GRN_RECORD_INIT(&(highlighter->lexicon.lazy_keyword_ids),
                  GRN_OBJ_VECTOR,
                  GRN_ID_NIL);
  highlighter->lexicon.token_id_chunks = NULL;
  GRN_TEXT_INIT(&(highlighter->lexicon.token_id_chunk), 0);
  GRN_RECORD_INIT(&(highlighter->lexicon.token_ids),
                  GRN_OBJ_VECTOR,
                  GRN_ID_NIL);
  GRN_TEXT_INIT(&(highlighter->lexicon.token_locations), 0);
  GRN_TEXT_INIT(&(highlighter->lexicon.candidates), 0);

  highlighter->pat.keywords = NULL;
  GRN_TEXT_INIT(&(highlighter->pat.normalizers), 0);
  grn_highlighter_set_normalizers(ctx, highlighter, "NormalizerAuto", -1);

  GRN_API_RETURN(highlighter);
}

grn_rc
grn_highlighter_close(grn_ctx *ctx, grn_highlighter *highlighter)
{
  GRN_API_ENTER;

  if (!highlighter) {
    GRN_API_RETURN(ctx->rc);
  }

  GRN_OBJ_FIN(ctx, &(highlighter->pat.normalizers));
  if (highlighter->pat.keywords) {
    grn_obj_close(ctx, highlighter->pat.keywords);
  }

  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.lazy_keywords));
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.lazy_keyword_ids));
  if (highlighter->lexicon.token_id_chunks) {
    grn_obj_close(ctx, highlighter->lexicon.token_id_chunks);
  }
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.token_id_chunk));
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.candidates));
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.token_locations));
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.token_ids));

  GRN_OBJ_FIN(ctx, &(highlighter->open_tags));
  GRN_OBJ_FIN(ctx, &(highlighter->close_tags));

  GRN_OBJ_FIN(ctx, &(highlighter->default_tag.open));
  GRN_OBJ_FIN(ctx, &(highlighter->default_tag.close));

  GRN_OBJ_FIN(ctx, &(highlighter->sequential_class_tag.open));
  GRN_OBJ_FIN(ctx, &(highlighter->sequential_class_tag.close));

  GRN_OBJ_FIN(ctx, &(highlighter->raw_keywords));
  GRN_FREE(highlighter);

  GRN_API_RETURN(ctx->rc);
}

static void
grn_highlighter_prepare_lexicon_add_token_id_chunks(
  grn_ctx *ctx,
  grn_highlighter *highlighter,
  grn_token **tokens,
  size_t n_tokens,
  grn_obj *token_id_chunk)
{
  if (n_tokens == 0) {
    if (GRN_TEXT_LEN(token_id_chunk) == 0) {
      return;
    }
    grn_encoding encoding = ctx->encoding;
    /* token_id_chunk is a binary data */
    ctx->encoding = GRN_ENC_NONE;
    grn_table_add(ctx,
                  highlighter->lexicon.token_id_chunks,
                  GRN_TEXT_VALUE(token_id_chunk),
                  (unsigned int)GRN_TEXT_LEN(token_id_chunk),
                  NULL);
    ctx->encoding = encoding;
  } else {
    grn_token *token = tokens[0];
    grn_obj *token_data = grn_token_get_data(ctx, token);
    /* Expand immature token later. */
    if (grn_token_get_force_prefix_search(ctx, token) &&
        highlighter->lexicon.object->header.type != GRN_TABLE_HASH_KEY) {
      grn_obj *lazy_keywords = &(highlighter->lexicon.lazy_keywords);
      grn_vector_add_element(ctx,
                             lazy_keywords,
                             GRN_TEXT_VALUE(token_data),
                             (uint32_t)GRN_TEXT_LEN(token_data),
                             0,
                             GRN_DB_TEXT);
    }
    grn_token_status token_status = grn_token_get_status(ctx, token);
    if (token_status & GRN_TOKEN_UNMATURED &&
        token_status & GRN_TOKEN_OVERLAP) {
      /* This skips the current token.
       *
       * GRN_TOKENIZE_GET don't emit immature and overlapped token
       * such as "c" in ["ab", "bc", "c"] for 'abc' with
       * TokenNgram. But GRN_TOKENIZE_ONLY doesn't do it. So we do it
       * manually here. */
      grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                          highlighter,
                                                          tokens + 1,
                                                          n_tokens - 1,
                                                          token_id_chunk);
    } else {
      grn_id token_id = GRN_ID_NIL;
      /* We should not use grn_table_get() here. If we use
       * grn_table_get(), token is normalized twice. */
      switch (highlighter->lexicon.object->header.type) {
      case GRN_TABLE_HASH_KEY:
        token_id = grn_hash_get(ctx,
                                (grn_hash *)(highlighter->lexicon.object),
                                GRN_TEXT_VALUE(token_data),
                                GRN_TEXT_LEN(token_data),
                                NULL);
        break;
      case GRN_TABLE_PAT_KEY:
        token_id = grn_pat_get(ctx,
                               (grn_pat *)(highlighter->lexicon.object),
                               GRN_TEXT_VALUE(token_data),
                               GRN_TEXT_LEN(token_data),
                               NULL);
        break;
      case GRN_TABLE_DAT_KEY:
        token_id = grn_dat_get(ctx,
                               (grn_dat *)(highlighter->lexicon.object),
                               GRN_TEXT_VALUE(token_data),
                               GRN_TEXT_LEN(token_data),
                               NULL);
        break;
      }
      /* GRN_TOKENIZE_ONLY may emit a token that doesn't exist in
       * lexicon. */
      if (token_id == GRN_ID_NIL) {
        /* Lexicon: ["ab", "bc", "c"]
         * Input: "abd"
         * Tokens: ["ab", "bd", "d"]
         *                ^
         *                The current token
         *
         * Register the current token ID chunk (["ab"]), ignore "bd"
         * and start a new token ID chunk ([]) with the next token
         * ("d").
         */

        /* Register the current token ID chunk. */
        grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                            highlighter,
                                                            NULL,
                                                            0,
                                                            token_id_chunk);
        /* Ignore the current token and start a new token ID chunk
         * from here. */
        GRN_BULK_REWIND(token_id_chunk);
        grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                            highlighter,
                                                            tokens + 1,
                                                            n_tokens - 1,
                                                            token_id_chunk);
      } else {
        /* This is for loose mode. In loose mode, we have two
         * tokenized blocks that are separated by
         * GRN_TOKENIER_END_MARK_UTF8 with GRN_TOKENIZE_ONLY.
         *
         * Example:
         *   Tokenizer: TokenNgram("loose_blank", true)
         *   Normalizer: NormalizerNFKC150
         *   Input: "a b"
         *   Tokens: ["a", "b", "U+FFF0", "ab"]
         *                      ^
         *                      GRN_TOKENIZER_END_MARK_UTF8
         *
         * If we found the separator, we split tokens with the
         * separator and create two token ID chunks. */
        if (GRN_TEXT_LEN(token_data) == GRN_TOKENIZER_END_MARK_UTF8_LEN &&
            memcmp(GRN_TEXT_VALUE(token_data),
                   GRN_TOKENIZER_END_MARK_UTF8,
                   GRN_TOKENIZER_END_MARK_UTF8_LEN) == 0) {
          /* Register the current token ID chunk. */
          grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                              highlighter,
                                                              NULL,
                                                              0,
                                                              token_id_chunk);
          if (n_tokens > 1) {
            /* If we have more tokens after we skip the
             * GRN_TOKENIZER_END_MARK_UTF8 token, clear the current
             * token ID chunk and start a new token ID chunk for
             * tokens followed by the GRN_TOKENIZER_END_MARK_UTF8
             * token. */
            GRN_BULK_REWIND(token_id_chunk);
            grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                                highlighter,
                                                                tokens + 1,
                                                                n_tokens - 1,
                                                                token_id_chunk);
          }
        } else {
          /* This is an existing token. We just add the current token
           * to the current token ID chunk and continue collecting
           * more tokens. */
          GRN_RECORD_PUT(ctx, token_id_chunk, token_id);
          grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                              highlighter,
                                                              tokens + 1,
                                                              n_tokens - 1,
                                                              token_id_chunk);
          grn_id used_token_id;
          GRN_RECORD_POP(token_id_chunk, used_token_id);
        }
      }
    }
  }
}

static void
grn_highlighter_prepare_lexicon(grn_ctx *ctx, grn_highlighter *highlighter)
{
  grn_obj *lazy_keywords = &(highlighter->lexicon.lazy_keywords);
  grn_id lexicon_id;
  size_t i, n_keywords;
  grn_obj *token_id_chunk = &(highlighter->lexicon.token_id_chunk);

  GRN_BULK_REWIND(lazy_keywords);

  lexicon_id = grn_obj_id(ctx, highlighter->lexicon.object);
  highlighter->lexicon.lazy_keyword_ids.header.domain = lexicon_id;
  highlighter->lexicon.token_ids.header.domain = lexicon_id;

  grn_table_get_info(ctx,
                     highlighter->lexicon.object,
                     NULL,
                     &(highlighter->lexicon.encoding),
                     NULL,
                     NULL,
                     NULL);

  if (highlighter->lexicon.token_id_chunks) {
    grn_table_truncate(ctx, highlighter->lexicon.token_id_chunks);
  } else {
    highlighter->lexicon.token_id_chunks =
      grn_table_create(ctx,
                       NULL,
                       0,
                       NULL,
                       GRN_OBJ_TABLE_PAT_KEY,
                       grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                       NULL);
    if (!highlighter->lexicon.token_id_chunks) {
      grn_rc rc = ctx->rc;
      char errbuf[GRN_CTX_MSGSIZE];
      if (rc == GRN_SUCCESS) {
        rc = GRN_UNKNOWN_ERROR;
      }
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(rc,
          "[highlighter][prepare][lexicon] "
          "failed to create an internal patricia trie: %s",
          errbuf);
      return;
    }
  }

  n_keywords = grn_vector_size(ctx, &(highlighter->raw_keywords));
  for (i = 0; i < n_keywords; i++) {
    const char *keyword;
    unsigned int keyword_length;
    grn_token_cursor *cursor;

    keyword_length = grn_vector_get_element(ctx,
                                            &(highlighter->raw_keywords),
                                            (uint32_t)i,
                                            &keyword,
                                            NULL,
                                            NULL);

    grn_obj tokens;
    /* grn_token isn't grn_obj but we use GRN_PTR here for easy to implement...
     */
    GRN_PTR_INIT(&tokens, GRN_OBJ_VECTOR, GRN_ID_NIL);
    cursor = grn_token_cursor_open(ctx,
                                   highlighter->lexicon.object,
                                   keyword,
                                   keyword_length,
                                   GRN_TOKENIZE_ONLY,
                                   0);
    if (!cursor) {
      continue;
    }
    GRN_BULK_REWIND(token_id_chunk);
    while (cursor->status != GRN_TOKEN_CURSOR_DONE) {
      grn_token_cursor_next(ctx, cursor);
      grn_token *token = grn_token_cursor_get_token(ctx, cursor);
      /* This may be needless. */
      grn_obj *token_data = grn_token_get_data(ctx, token);
      if (GRN_TEXT_LEN(token_data) == 0) {
        break;
      }
      grn_token *copied_token = GRN_MALLOC(sizeof(grn_token));
      grn_token_init_deep(ctx, copied_token);
      grn_token_copy(ctx, copied_token, token);
      GRN_PTR_PUT(ctx, &tokens, copied_token);
    }
    grn_token_cursor_close(ctx, cursor);

    size_t n_tokens = GRN_PTR_VECTOR_SIZE(&tokens);
    if (n_tokens == 0) {
      GRN_OBJ_FIN(ctx, &tokens);
      continue;
    }
    grn_token **raw_tokens = (grn_token **)GRN_BULK_HEAD(&tokens);
    grn_highlighter_prepare_lexicon_add_token_id_chunks(ctx,
                                                        highlighter,
                                                        raw_tokens,
                                                        n_tokens,
                                                        token_id_chunk);
    size_t i;
    for (i = 0; i < n_tokens; i++) {
      grn_token_fin(ctx, raw_tokens[i]);
      GRN_FREE(raw_tokens[i]);
    }
    GRN_OBJ_FIN(ctx, &tokens);
  }
}

static void
grn_highlighter_prepare_patricia_trie(grn_ctx *ctx,
                                      grn_highlighter *highlighter)
{
  if (highlighter->pat.keywords) {
    grn_table_truncate(ctx, highlighter->pat.keywords);
  } else {
    highlighter->pat.keywords =
      grn_table_create(ctx,
                       NULL,
                       0,
                       NULL,
                       GRN_OBJ_TABLE_PAT_KEY,
                       grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                       NULL);
    if (!highlighter->pat.keywords) {
      grn_rc rc = ctx->rc;
      char errbuf[GRN_CTX_MSGSIZE];
      if (rc == GRN_SUCCESS) {
        rc = GRN_UNKNOWN_ERROR;
      }
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(rc,
          "[highlighter][prepare][no-lexicon] "
          "failed to create an internal patricia trie: %s",
          errbuf);
      return;
    }
  }

  if (highlighter->lexicon.object) {
    grn_obj normalizers;
    GRN_TEXT_INIT(&normalizers, 0);
    grn_table_get_normalizers_string(ctx,
                                     highlighter->lexicon.object,
                                     &normalizers);
    grn_obj_set_info(ctx,
                     highlighter->pat.keywords,
                     GRN_INFO_NORMALIZERS,
                     &normalizers);
    GRN_OBJ_FIN(ctx, &normalizers);
  } else {
    grn_obj_set_info(ctx,
                     highlighter->pat.keywords,
                     GRN_INFO_NORMALIZERS,
                     &(highlighter->pat.normalizers));
  }
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

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
                    highlighter->pat.keywords,
                    keyword,
                    keyword_size,
                    NULL);
    }
  }
}

static void
grn_highlighter_prepare(grn_ctx *ctx, grn_highlighter *highlighter)
{
  bool use_lexicon = false;
  if (highlighter->lexicon.object) {
    grn_obj *tokenizer;
    grn_table_get_info(ctx,
                       highlighter->lexicon.object,
                       NULL,
                       NULL,
                       &tokenizer,
                       NULL,
                       NULL);
    use_lexicon = (tokenizer != NULL);
  }
  if (use_lexicon) {
    grn_highlighter_prepare_lexicon(ctx, highlighter);
    if (highlighter->pat.keywords) {
      grn_obj_close(ctx, highlighter->pat.keywords);
      highlighter->pat.keywords = NULL;
    }
  } else {
    grn_highlighter_prepare_patricia_trie(ctx, highlighter);
  }
  size_t n_open_tags = grn_vector_size(ctx, &(highlighter->open_tags));
  size_t n_close_tags = grn_vector_size(ctx, &(highlighter->close_tags));
  highlighter->n_tags = (n_open_tags == n_close_tags ? n_open_tags : 0);
  highlighter->need_prepared = false;
}

static bool
grn_ids_is_included(grn_id *ids, size_t n_ids, grn_id id)
{
  size_t i;

  for (i = 0; i < n_ids; i++) {
    if (ids[i] == id) {
      return true;
    }
  }

  return false;
}

static void
grn_highlighter_log_location(grn_ctx *ctx,
                             grn_log_level level,
                             const char *tag,
                             grn_highlighter_location *location,
                             const char *text,
                             size_t text_length)
{
  if (!grn_logger_pass(ctx, level)) {
    return;
  }

  GRN_LOG(ctx,
          level,
          "%s[location] "
          "[%" GRN_FMT_INT64U "...%" GRN_FMT_INT64U "](%u)[%s] <%.*s>",
          tag,
          location->offset,
          location->offset + location->length,
          location->first_character_length,
          location->have_overlap ? "overlapped" : "not-overlapped",
          (int)location->length,
          text + location->offset);
}

static void
grn_highlighter_highlight_get_ith_tag(grn_ctx *ctx,
                                      grn_highlighter *highlighter,
                                      size_t i,
                                      const char **open_tag,
                                      size_t *open_tag_length,
                                      const char **close_tag,
                                      size_t *close_tag_length)
{
  if (highlighter->is_sequential_class_tag_mode) {
    size_t n_keywords = grn_vector_size(ctx, &(highlighter->raw_keywords));
    i = i % n_keywords;
    GRN_BULK_REWIND(&(highlighter->sequential_class_tag.open));
    grn_text_printf(ctx,
                    &(highlighter->sequential_class_tag.open),
                    "<mark class=\"keyword-%" GRN_FMT_SIZE "\">",
                    i);
    GRN_TEXT_SETS(ctx, &(highlighter->sequential_class_tag.close), "</mark>");
    *open_tag = GRN_TEXT_VALUE(&(highlighter->sequential_class_tag.open));
    *open_tag_length = GRN_TEXT_LEN(&(highlighter->sequential_class_tag.open));
    *close_tag = GRN_TEXT_VALUE(&(highlighter->sequential_class_tag.close));
    *close_tag_length =
      GRN_TEXT_LEN(&(highlighter->sequential_class_tag.close));
  } else if (highlighter->n_tags == 0) {
    *open_tag = GRN_TEXT_VALUE(&(highlighter->default_tag.open));
    *open_tag_length = GRN_TEXT_LEN(&(highlighter->default_tag.open)) - 1;
    *close_tag = GRN_TEXT_VALUE(&(highlighter->default_tag.close));
    *close_tag_length = GRN_TEXT_LEN(&(highlighter->default_tag.close)) - 1;
  } else {
    i = i % highlighter->n_tags;
    *open_tag_length = grn_vector_get_element(ctx,
                                              &(highlighter->open_tags),
                                              i,
                                              open_tag,
                                              NULL,
                                              NULL);
    *close_tag_length = grn_vector_get_element(ctx,
                                               &(highlighter->close_tags),
                                               i,
                                               close_tag,
                                               NULL,
                                               NULL);
  }
}

static void
grn_highlighter_highlight_add_normal_text(grn_ctx *ctx,
                                          grn_highlighter *highlighter,
                                          grn_obj *output,
                                          const char *text,
                                          size_t text_length)
{
  if (highlighter->is_html_mode) {
    grn_text_escape_xml(ctx, output, text, text_length);
  } else {
    GRN_TEXT_PUT(ctx, output, text, text_length);
  }
}

static uint64_t
grn_highlighter_highlight_lexicon_flush(grn_ctx *ctx,
                                        grn_log_level log_level,
                                        const char *tag,
                                        grn_highlighter *highlighter,
                                        const char *text,
                                        size_t text_length,
                                        grn_obj *output,
                                        grn_highlighter_location *location,
                                        uint64_t offset)
{
  grn_highlighter_log_location(ctx,
                               log_level,
                               tag,
                               location,
                               text,
                               text_length);
  if (location->offset > offset) {
    grn_highlighter_highlight_add_normal_text(
      ctx,
      highlighter,
      output,
      text + offset,
      (size_t)(location->offset - offset));
  }

  const char *open_tag;
  size_t open_tag_length;
  const char *close_tag;
  size_t close_tag_length;
  grn_highlighter_highlight_get_ith_tag(ctx,
                                        highlighter,
                                        /* We need "- 1" here
                                         * because grn_id doesn't
                                         * use 0 (GRN_ID_NIL). */
                                        location->chunk_id - 1,
                                        &open_tag,
                                        &open_tag_length,
                                        &close_tag,
                                        &close_tag_length);
  GRN_TEXT_PUT(ctx, output, open_tag, open_tag_length);
  grn_highlighter_highlight_add_normal_text(ctx,
                                            highlighter,
                                            output,
                                            text + location->offset,
                                            location->length);
  GRN_TEXT_PUT(ctx, output, close_tag, close_tag_length);
  return location->offset + location->length;
}

static void
grn_highlighter_tokenize_lexicon(grn_ctx *ctx,
                                 grn_highlighter *highlighter,
                                 const char *text,
                                 size_t text_length)
{
  const char *tag = "[highlighter][highlight][lexicon]";
  grn_log_level log_level = GRN_LOG_DEBUG;
  grn_obj *token_ids = &(highlighter->lexicon.token_ids);
  grn_obj *token_locations = &(highlighter->lexicon.token_locations);

  GRN_BULK_REWIND(token_ids);
  GRN_BULK_REWIND(token_locations);
  grn_token_cursor *cursor = grn_token_cursor_open(ctx,
                                                   highlighter->lexicon.object,
                                                   text,
                                                   text_length,
                                                   GRN_TOKENIZE_ADD,
                                                   0);
  if (!cursor) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc, "%s failed to start tokenizing: %s", tag, errbuf);
    return;
  }

  GRN_LOG(ctx, log_level, "%s[tokenize][start]", tag);
  while (cursor->status == GRN_TOKEN_CURSOR_DOING) {
    grn_id token_id = grn_token_cursor_next(ctx, cursor);
    grn_highlighter_location location;
    grn_token *token;

    if (token_id == GRN_ID_NIL) {
      continue;
    }
    token = grn_token_cursor_get_token(ctx, cursor);
    GRN_RECORD_PUT(ctx, token_ids, token_id);
    location.offset = grn_token_get_source_offset(ctx, token);
    location.length = grn_token_get_source_length(ctx, token);
    location.first_character_length =
      grn_token_get_source_first_character_length(ctx, token);
    location.have_overlap = grn_token_have_overlap(ctx, token);
    GRN_TEXT_PUT(ctx, token_locations, &location, sizeof(location));
    grn_highlighter_log_location(ctx,
                                 log_level,
                                 tag,
                                 &location,
                                 text,
                                 text_length);
  }
  grn_token_cursor_close(ctx, cursor);
  GRN_LOG(ctx, log_level, "%s[tokenize][end]", tag);
}

static void
grn_highlighter_highlight_lexicon(grn_ctx *ctx,
                                  grn_highlighter *highlighter,
                                  const char *text,
                                  size_t text_length,
                                  grn_obj *output)
{
  const char *tag = "[highlighter][highlight][lexicon]";
  grn_log_level log_level = GRN_LOG_DEBUG;
  grn_obj *lazy_keyword_ids = &(highlighter->lexicon.lazy_keyword_ids);
  grn_obj *token_ids = &(highlighter->lexicon.token_ids);
  grn_obj *token_locations = &(highlighter->lexicon.token_locations);
  grn_obj *candidates = &(highlighter->lexicon.candidates);

  GRN_BULK_REWIND(lazy_keyword_ids);

  {
    grn_obj *lexicon = highlighter->lexicon.object;
    grn_obj *chunks = highlighter->lexicon.token_id_chunks;
    grn_obj *lazy_keywords = &(highlighter->lexicon.lazy_keywords);
    size_t i;
    size_t n_keywords;

    n_keywords = grn_vector_size(ctx, lazy_keywords);
    for (i = 0; i < n_keywords; i++) {
      const char *keyword;
      unsigned int keyword_length;

      keyword_length = grn_vector_get_element(ctx,
                                              lazy_keywords,
                                              (uint32_t)i,
                                              &keyword,
                                              NULL,
                                              NULL);
      GRN_LOG(ctx,
              log_level,
              "%s[prefix-search][start] %" GRN_FMT_SIZE ":<%.*s>",
              tag,
              i,
              (int)keyword_length,
              keyword);
      GRN_TABLE_EACH_BEGIN_MIN(ctx,
                               lexicon,
                               cursor,
                               id,
                               keyword,
                               keyword_length,
                               GRN_CURSOR_PREFIX)
      {
        int added = 0;

        {
          grn_encoding encoding = ctx->encoding;
          ctx->encoding = GRN_ENC_NONE;
          grn_table_add(ctx, chunks, &id, sizeof(grn_id), &added);
          ctx->encoding = encoding;
        }
        if (grn_logger_pass(ctx, log_level)) {
          void *key;
          int key_size;
          key_size = grn_table_cursor_get_key(ctx, cursor, &key);
          GRN_LOG(ctx,
                  log_level,
                  "%s[prefix-search][%s] %" GRN_FMT_SIZE ":<%.*s>:<%.*s>",
                  tag,
                  added ? "added" : "not-added",
                  i,
                  (int)keyword_length,
                  keyword,
                  key_size,
                  (const char *)key);
        }
        if (added) {
          GRN_RECORD_PUT(ctx, lazy_keyword_ids, id);
        }
      }
      GRN_TABLE_EACH_END(ctx, cursor);
      GRN_LOG(ctx,
              log_level,
              "%s[prefix-search][end] %" GRN_FMT_SIZE ":<%.*s>",
              tag,
              i,
              (int)keyword_length,
              keyword);
    }
  }

  GRN_BULK_REWIND(candidates);
  {
    grn_obj *lazy_keyword_ids = &(highlighter->lexicon.lazy_keyword_ids);
    grn_id *raw_lazy_keyword_ids;
    size_t n_lazy_keyword_ids;
    grn_pat *chunks = (grn_pat *)(highlighter->lexicon.token_id_chunks);
    size_t i;
    size_t n_token_ids = GRN_BULK_VSIZE(token_ids) / sizeof(grn_id);
    grn_id *raw_token_ids = (grn_id *)GRN_BULK_HEAD(token_ids);
    grn_highlighter_location *raw_token_locations =
      (grn_highlighter_location *)GRN_BULK_HEAD(token_locations);

    raw_lazy_keyword_ids = (grn_id *)GRN_BULK_HEAD(lazy_keyword_ids);
    n_lazy_keyword_ids = GRN_BULK_VSIZE(lazy_keyword_ids) / sizeof(grn_id);

    for (i = 0; i < n_token_ids; i++) {
      grn_id chunk_id;

      GRN_LOG(ctx, log_level, "%s[lcp-search][start] %" GRN_FMT_SIZE, tag, i);
      {
        grn_encoding encoding = ctx->encoding;
        /* token_id_chunk is a binary data */
        ctx->encoding = GRN_ENC_NONE;
        chunk_id =
          grn_pat_lcp_search(ctx,
                             chunks,
                             raw_token_ids + i,
                             (unsigned int)(n_token_ids - i) * sizeof(grn_id));
        ctx->encoding = encoding;
      }
      if (chunk_id == GRN_ID_NIL) {
        GRN_LOG(ctx,
                log_level,
                "%s[lcp-search][end][nonexistent] %" GRN_FMT_SIZE,
                tag,
                i);
        continue;
      }

      {
        grn_id *ids;
        uint32_t key_size;
        size_t n_ids;
        grn_highlighter_location candidate;
        grn_highlighter_location *first = raw_token_locations + i;

        candidate.chunk_id = chunk_id;
        candidate.have_overlap = false;
        candidate.first_character_length = 0;

        {
          grn_encoding encoding = ctx->encoding;
          ctx->encoding = GRN_ENC_NONE;
          ids = (grn_id *)_grn_pat_key(ctx, chunks, chunk_id, &key_size);
          ctx->encoding = encoding;
        }
        n_ids = key_size / sizeof(grn_id);
        candidate.offset = first->offset;
        if (n_ids == 1) {
          if (grn_ids_is_included(raw_lazy_keyword_ids,
                                  n_lazy_keyword_ids,
                                  ids[0])) {
            candidate.length = first->first_character_length;
          } else {
            candidate.length = first->length;
          }
        } else {
          grn_highlighter_location *last = raw_token_locations + i + n_ids - 1;
          candidate.length = last->offset + last->length - candidate.offset;
        }
        GRN_TEXT_PUT(ctx, candidates, &candidate, sizeof(candidate));
        grn_highlighter_log_location(ctx,
                                     log_level,
                                     tag,
                                     &candidate,
                                     text,
                                     text_length);
        GRN_LOG(ctx, log_level, "%s[lcp-search][end] %" GRN_FMT_SIZE, tag, i);
        i += n_ids - 1;
      }
    }
  }

  {
    grn_obj *chunks = highlighter->lexicon.token_id_chunks;
    size_t i, n_ids;

    n_ids = GRN_BULK_VSIZE(lazy_keyword_ids) / sizeof(grn_id);
    for (i = 0; i < n_ids; i++) {
      grn_id id = GRN_RECORD_VALUE_AT(lazy_keyword_ids, i);
      grn_encoding encoding = ctx->encoding;
      ctx->encoding = GRN_ENC_NONE;
      grn_table_delete(ctx, chunks, &id, sizeof(grn_id));
      ctx->encoding = encoding;
    }
  }

  {
    size_t i;
    size_t n_candidates =
      GRN_BULK_VSIZE(candidates) / sizeof(grn_highlighter_location);
    grn_highlighter_location *raw_candidates =
      (grn_highlighter_location *)GRN_BULK_HEAD(candidates);

    GRN_LOG(ctx,
            log_level,
            "%s[highlight][start] %" GRN_FMT_SIZE,
            tag,
            n_candidates);
    if (n_candidates == 0) {
      grn_text_escape_xml(ctx, output, text, text_length);
    } else {
      grn_highlighter_location *previous = NULL;
      uint64_t offset = 0;

      qsort(raw_candidates,
            n_candidates,
            sizeof(grn_highlighter_location),
            grn_highlighter_location_compare);
      previous = raw_candidates;
      for (i = 1; i < n_candidates; i++) {
        grn_highlighter_location *current = raw_candidates + i;
        if (previous->offset + previous->length >= current->offset) {
          if (previous->offset + previous->length >
              current->offset + current->length) {
            current->length = previous->length;
          } else {
            current->length += (uint32_t)(current->offset - previous->offset);
          }
          current->offset = previous->offset;
          previous = current;
          continue;
        }
        if (current->offset > previous->offset) {
          offset = grn_highlighter_highlight_lexicon_flush(ctx,
                                                           log_level,
                                                           tag,
                                                           highlighter,
                                                           text,
                                                           text_length,
                                                           output,
                                                           previous,
                                                           offset);
        }
        previous = current;
      }
      offset = grn_highlighter_highlight_lexicon_flush(ctx,
                                                       log_level,
                                                       tag,
                                                       highlighter,
                                                       text,
                                                       text_length,
                                                       output,
                                                       previous,
                                                       offset);
      if (offset < text_length) {
        grn_highlighter_highlight_add_normal_text(
          ctx,
          highlighter,
          output,
          text + offset,
          (size_t)(text_length - offset));
      }
    }
    GRN_LOG(ctx,
            log_level,
            "%s[highlight][end] %" GRN_FMT_SIZE,
            tag,
            n_candidates);
  }
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
                          (grn_pat *)(highlighter->pat.keywords),
                          current,
                          (unsigned int)current_length,
                          hits,
                          MAX_N_HITS,
                          &rest);
    for (i = 0; i < n_hits; i++) {
      const char *open_tag;
      size_t open_tag_length;
      const char *close_tag;
      size_t close_tag_length;
      grn_highlighter_highlight_get_ith_tag(ctx,
                                            highlighter,
                                            /* We need "- 1" here
                                             * because grn_id doesn't
                                             * use 0 (GRN_ID_NIL). */
                                            hits[i].id - 1,
                                            &open_tag,
                                            &open_tag_length,
                                            &close_tag,
                                            &close_tag_length);

      if ((hits[i].offset - previous_length) > 0) {
        grn_highlighter_highlight_add_normal_text(ctx,
                                                  highlighter,
                                                  output,
                                                  current + previous_length,
                                                  hits[i].offset -
                                                    previous_length);
      }
      GRN_TEXT_PUT(ctx, output, open_tag, open_tag_length);
      grn_highlighter_highlight_add_normal_text(ctx,
                                                highlighter,
                                                output,
                                                current + hits[i].offset,
                                                hits[i].length);
      GRN_TEXT_PUT(ctx, output, close_tag, close_tag_length);
      previous_length = hits[i].offset + hits[i].length;
    }

    chunk_length = (size_t)(rest - current);
    if ((chunk_length - previous_length) > 0) {
      grn_highlighter_highlight_add_normal_text(ctx,
                                                highlighter,
                                                output,
                                                current + previous_length,
                                                current_length -
                                                  previous_length);
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
    text_length = (int64_t)strlen(text);
  }

  if (text_length == 0) {
    goto exit;
  }

  if (grn_vector_size(ctx, &(highlighter->raw_keywords)) == 0) {
    grn_highlighter_highlight_add_normal_text(ctx,
                                              highlighter,
                                              output,
                                              text,
                                              (size_t)text_length);
    goto exit;
  }

  if (highlighter->lexicon.object) {
    /* We need to tokenize the target text with GRN_TOKENIZE_ADD
     * before we call grn_highlighter_prepare_lexicon() because
     * grn_highlighter_prepare_lexicon() assumes that the tokens of
     * the target text exists in highlighter->lexicon.object.
     *
     * If we have any new tokens, we need to re-prepare. */
    uint32_t size_before = grn_table_size(ctx, highlighter->lexicon.object);
    grn_highlighter_tokenize_lexicon(ctx, highlighter, text, text_length);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    uint32_t size_after = grn_table_size(ctx, highlighter->lexicon.object);
    if (size_after > size_before) {
      highlighter->need_prepared = true;
    }
  }

  if (highlighter->need_prepared) {
    grn_highlighter_prepare(ctx, highlighter);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

  if (highlighter->pat.keywords) {
    grn_highlighter_highlight_patricia_trie(ctx,
                                            highlighter,
                                            text,
                                            (size_t)text_length,
                                            output);
  } else {
    grn_highlighter_highlight_lexicon(ctx,
                                      highlighter,
                                      text,
                                      (size_t)text_length,
                                      output);
  }

exit:
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_set_lexicon(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            grn_obj *lexicon)
{
  GRN_API_ENTER;

  if (highlighter->lexicon.object != lexicon) {
    highlighter->need_prepared = true;
    highlighter->lexicon.object = lexicon;
  }

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_highlighter_get_lexicon(grn_ctx *ctx, grn_highlighter *highlighter)
{
  GRN_API_ENTER;

  GRN_API_RETURN(highlighter->lexicon.object);
}

grn_rc
grn_highlighter_set_default_open_tag(grn_ctx *ctx,
                                     grn_highlighter *highlighter,
                                     const char *tag,
                                     int64_t tag_length)
{
  GRN_API_ENTER;
  if (tag_length < 0) {
    tag_length = strlen(tag);
  }
  GRN_TEXT_SET(ctx, &(highlighter->default_tag.open), tag, tag_length);
  GRN_TEXT_PUTC(ctx, &(highlighter->default_tag.open), '\0');
  GRN_API_RETURN(ctx->rc);
}

const char *
grn_highlighter_get_default_open_tag(grn_ctx *ctx, grn_highlighter *highlighter)
{
  return GRN_TEXT_VALUE(&(highlighter->default_tag.open));
}

grn_rc
grn_highlighter_set_default_close_tag(grn_ctx *ctx,
                                      grn_highlighter *highlighter,
                                      const char *tag,
                                      int64_t tag_length)
{
  GRN_API_ENTER;
  if (tag_length < 0) {
    tag_length = strlen(tag);
  }
  GRN_TEXT_SET(ctx, &(highlighter->default_tag.close), tag, tag_length);
  GRN_TEXT_PUTC(ctx, &(highlighter->default_tag.close), '\0');
  GRN_API_RETURN(ctx->rc);
}

const char *
grn_highlighter_get_default_close_tag(grn_ctx *ctx,
                                      grn_highlighter *highlighter)
{
  return GRN_TEXT_VALUE(&(highlighter->default_tag.close));
}

grn_rc
grn_highlighter_set_sequential_class_tag_mode(grn_ctx *ctx,
                                              grn_highlighter *highlighter,
                                              bool mode)
{
  GRN_API_ENTER;
  highlighter->is_sequential_class_tag_mode = mode;
  GRN_API_RETURN(ctx->rc);
}

bool
grn_highlighter_get_sequential_class_tag_mode(grn_ctx *ctx,
                                              grn_highlighter *highlighter)
{
  return highlighter->is_sequential_class_tag_mode;
}

grn_rc
grn_highlighter_set_normalizers(grn_ctx *ctx,
                                grn_highlighter *highlighter,
                                const char *normalizers,
                                int64_t normalizers_length)
{
  GRN_API_ENTER;
  if (normalizers_length < 0) {
    normalizers_length = strlen(normalizers);
  }
  GRN_TEXT_SET(ctx,
               &(highlighter->pat.normalizers),
               normalizers,
               normalizers_length);
  highlighter->need_prepared = true;
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_highlighter_get_normalizers(grn_ctx *ctx, grn_highlighter *highlighter)
{
  return &(highlighter->pat.normalizers);
}

grn_rc
grn_highlighter_set_html_mode(grn_ctx *ctx,
                              grn_highlighter *highlighter,
                              bool html_mode)
{
  GRN_API_ENTER;
  highlighter->is_html_mode = html_mode;
  GRN_API_RETURN(ctx->rc);
}

bool
grn_highlighter_get_html_mode(grn_ctx *ctx, grn_highlighter *highlighter)
{
  return highlighter->is_html_mode;
}

grn_rc
grn_highlighter_add_keyword(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            const char *keyword,
                            int64_t keyword_length)
{
  unsigned int i, n_keywords;
  grn_obj *raw_keywords = &(highlighter->raw_keywords);

  GRN_API_ENTER;

  if (keyword_length < 0) {
    keyword_length = (int64_t)strlen(keyword);
  }

  if (keyword_length == 0) {
    goto exit;
  }

  n_keywords = grn_vector_size(ctx, raw_keywords);
  for (i = 0; i < n_keywords; i++) {
    const char *existing_keyword;
    unsigned int length;

    length = grn_vector_get_element(ctx,
                                    raw_keywords,
                                    i,
                                    &existing_keyword,
                                    NULL,
                                    NULL);
    if (length == keyword_length &&
        memcmp(keyword, existing_keyword, length) == 0) {
      goto exit;
    }
  }

  grn_vector_add_element(ctx,
                         raw_keywords,
                         keyword,
                         (uint32_t)keyword_length,
                         0,
                         GRN_DB_TEXT);
  highlighter->need_prepared = true;

exit:
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_clear_keywords(grn_ctx *ctx, grn_highlighter *highlighter)
{
  grn_obj *raw_keywords = &(highlighter->raw_keywords);

  GRN_API_ENTER;

  GRN_BULK_REWIND(raw_keywords);
  highlighter->need_prepared = true;

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_add_open_tag(grn_ctx *ctx,
                             grn_highlighter *highlighter,
                             const char *tag,
                             int64_t tag_length)
{
  GRN_API_ENTER;
  if (tag_length < 0) {
    tag_length = strlen(tag);
  }
  grn_vector_add_element(ctx,
                         &(highlighter->open_tags),
                         tag,
                         tag_length,
                         0,
                         GRN_DB_TEXT);
  highlighter->need_prepared = true;
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_add_close_tag(grn_ctx *ctx,
                              grn_highlighter *highlighter,
                              const char *tag,
                              int64_t tag_length)
{
  GRN_API_ENTER;
  if (tag_length < 0) {
    tag_length = strlen(tag);
  }
  grn_vector_add_element(ctx,
                         &(highlighter->close_tags),
                         tag,
                         tag_length,
                         0,
                         GRN_DB_TEXT);
  highlighter->need_prepared = true;
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_clear_tags(grn_ctx *ctx, grn_highlighter *highlighter)
{
  GRN_API_ENTER;
  GRN_BULK_REWIND(&(highlighter->open_tags));
  GRN_BULK_REWIND(&(highlighter->close_tags));
  highlighter->need_prepared = true;
  GRN_API_RETURN(ctx->rc);
}
