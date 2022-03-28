/*
  Copyright (C) 2010-2015  Brazil
  Copyright (C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_ii.h"
#include "grn_index_cursor.h"

typedef struct {
  grn_db_obj obj;
  grn_obj *index_column;
  grn_table_cursor *tc;
  grn_ii_cursor *iic;
  bool next_called;
  grn_id term_id;
  grn_id input_term_id;
  grn_id rid_min;
  grn_id rid_max;
  int flags;
  uint32_t section_id;
  float scale;
  float *scales;
  size_t n_scales;
  struct {
    bool specified;
    uint32_t start;
  } position;
} grn_index_cursor;

grn_obj *
grn_index_cursor_open(grn_ctx *ctx,
                      grn_table_cursor *tc,
                      grn_obj *index_column,
                      grn_id rid_min,
                      grn_id rid_max,
                      int flags)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = GRN_CALLOC(sizeof(grn_index_cursor));
  if (!ic) {
    GRN_API_RETURN(NULL);
  }

  ic->tc = tc;
  ic->index_column = index_column;
  ic->iic = NULL;
  ic->next_called = false;
  ic->term_id = GRN_ID_NIL;
  ic->input_term_id = GRN_ID_NIL;
  ic->rid_min = rid_min;
  ic->rid_max = rid_max;
  ic->flags = flags;
  ic->section_id = 0;
  ic->scale = 1.0;
  ic->scales = NULL;
  ic->n_scales = 0;
  ic->position.specified = false;
  ic->position.start = 0;
  GRN_DB_OBJ_SET_TYPE(ic, GRN_CURSOR_COLUMN_INDEX);
  {
    grn_id id = grn_obj_register(ctx, ctx->impl->db, NULL, 0);
    DB_OBJ(ic)->header.domain = GRN_ID_NIL;
    DB_OBJ(ic)->range = GRN_ID_NIL;
    grn_db_obj_init(ctx, ctx->impl->db, id, DB_OBJ(ic));
  }
  GRN_API_RETURN((grn_obj *)ic);
}

void
grn_index_cursor_close(grn_ctx *ctx, grn_obj *index_cursor)
{
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic->iic) {
    grn_ii_cursor_close(ctx, ic->iic);
  }
  GRN_FREE(ic);
}

grn_obj *
grn_index_cursor_get_index_column(grn_ctx *ctx,
                                    grn_obj *index_cursor)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  grn_obj *index_column = NULL;
  if (ic) {
    index_column = ic->index_column;
  }
  GRN_API_RETURN(index_column);
}

grn_rc
grn_index_cursor_set_term_id(grn_ctx *ctx,
                             grn_obj *index_cursor,
                             grn_id term_id)
{
  grn_index_cursor *cursor = (grn_index_cursor *)index_cursor;
  GRN_API_ENTER;
  if (!cursor) {
    ERR(GRN_INVALID_ARGUMENT, "[index-cursor][set-term-id] must not NULL");
    goto exit;
  }
  if (cursor->tc) {
    ERR(GRN_INVALID_ARGUMENT,
        "[index-cursor][set-term-id] "
        "setting term ID against index cursor with table cursor "
        "isn't supported");
    goto exit;
  }
  cursor->input_term_id = term_id;
exit :
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_index_cursor_set_section_id(grn_ctx *ctx,
                                grn_obj *index_cursor,
                                uint32_t section_id)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic) {
    ic->section_id = section_id;
  }
  GRN_API_RETURN(ctx->rc);
}

uint32_t
grn_index_cursor_get_section_id(grn_ctx *ctx,
                                grn_obj *index_cursor)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic) {
    GRN_API_RETURN(ic->section_id);
  } else {
    GRN_API_RETURN(0);
  }
}

grn_rc
grn_index_cursor_set_scale(grn_ctx *ctx,
                           grn_obj *index_cursor,
                           float scale)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic) {
    ic->scale = scale;
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_index_cursor_set_scales(grn_ctx *ctx,
                            grn_obj *index_cursor,
                            float *scales,
                            size_t n_scales)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic) {
    ic->scales = scales;
    ic->n_scales = n_scales;
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_index_cursor_set_start_position(grn_ctx *ctx,
                                    grn_obj *index_cursor,
                                    uint32_t position)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic) {
    ic->position.specified = true;
    ic->position.start = position;
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_index_cursor_reset_start_position(grn_ctx *ctx,
                                      grn_obj *index_cursor)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic) {
    ic->position.specified = false;
  }
  GRN_API_RETURN(ctx->rc);
}

uint32_t
grn_index_cursor_get_start_position(grn_ctx *ctx,
                                    grn_obj *index_cursor)
{
  GRN_API_ENTER;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  if (ic && ic->position.specified) {
    GRN_API_RETURN(ic->position.start);
  } else {
    GRN_API_RETURN(0);
  }
}

grn_posting *
grn_index_cursor_next_internal(grn_ctx *ctx,
                               grn_obj *index_cursor,
                               grn_id *term_id)
{
  grn_posting *posting = NULL;
  grn_index_cursor *ic = (grn_index_cursor *)index_cursor;
  while (true) {
    if (ic->iic) {
      if (ic->flags & GRN_OBJ_WITH_POSITION) {
        while (true) {
          if (ic->next_called) {
            posting = grn_ii_cursor_next_pos(ctx, ic->iic);
            if (posting) {
              if (ic->position.specified) {
                if (posting->pos == ic->position.start) {
                  break;
                }
              } else {
                break;
              }
            }
          }

          grn_posting *ii_posting;
          while ((ii_posting = grn_ii_cursor_next(ctx, ic->iic))) {
            if (!(ic->section_id == 0 || ii_posting->sid == ic->section_id)) {
              continue;
            }
            break;
          }
          ic->next_called = true;
          if (!ii_posting) {
            break;
          }
        }
      } else {
        while ((posting = grn_ii_cursor_next(ctx, ic->iic))) {
          if (!(ic->section_id == 0 || posting->sid == ic->section_id)) {
            continue;
          }
          if (ic->position.specified) {
            while ((posting = grn_ii_cursor_next_pos(ctx, ic->iic))) {
              if (posting->pos == ic->position.start) {
                break;
              }
            }
            if (!posting) {
              continue;
            }
          }
          break;
        }
      }
    }
    if (posting) {
      break;
    }

    if (ic->tc) {
      ic->term_id = grn_table_cursor_next(ctx, ic->tc);
    } else {
      ic->term_id = ic->input_term_id;
      ic->input_term_id = GRN_ID_NIL;
    }
    if (ic->term_id == GRN_ID_NIL) {
      break;
    }
    grn_ii *ii = (grn_ii *)ic->index_column;
    if (ic->iic) {
      grn_ii_cursor_close(ctx, ic->iic);
    }
    ic->iic = grn_ii_cursor_open(ctx,
                                 ii,
                                 ic->term_id,
                                 ic->rid_min,
                                 ic->rid_max,
                                 (int)(ii->n_elements),
                                 ic->flags);
    if (ic->iic) {
      if (ic->n_scales > 0) {
        grn_ii_cursor_set_scales(ctx, ic->iic, ic->scales, ic->n_scales);
      } else {
        grn_ii_cursor_set_scale(ctx, ic->iic, ic->scale);
      }
    }
    ic->next_called = false;
  }
  if (term_id) {
    *term_id = ic->term_id;
  }
  return posting;
}

grn_posting *
grn_index_cursor_next(grn_ctx *ctx,
                      grn_obj *index_cursor,
                      grn_id *term_id)
{
  GRN_API_ENTER;
  grn_posting *posting = grn_index_cursor_next_internal(ctx,
                                                        index_cursor,
                                                        term_id);
  GRN_API_RETURN(posting);
}
