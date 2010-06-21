/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2010 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

#include <string.h>
#include "str.h"
#include "ql.h"
#include "db.h"
#include "output.h"

#define LEVELS (&ctx->impl->levels)
#define DEPTH (GRN_BULK_VSIZE(LEVELS)>>2)
#define CURR_LEVEL (DEPTH ? (GRN_UINT32_VALUE_AT(LEVELS, (DEPTH - 1))) : 0)
#define INCR_DEPTH(i) GRN_UINT32_PUT(ctx, LEVELS, i)
#define DECR_DEPTH (DEPTH ? grn_bulk_truncate(ctx, LEVELS, GRN_BULK_VSIZE(LEVELS) - sizeof(uint32_t)) : 0)
#define INCR_LENGTH (DEPTH ? (GRN_UINT32_VALUE_AT(LEVELS, (DEPTH - 1)) += 2) : 0)

static void
put_delimiter(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type)
{
  uint32_t level = CURR_LEVEL;
  if (level < 2) { return; }
  switch (output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, outbuf, ((level & 3) == 3) ? ':' : ',');
    break;
  case GRN_CONTENT_XML:
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH == 1) {
      GRN_TEXT_PUTC(ctx, outbuf, ((level & 3) == 3) ? '\t' : '\n');
    } else if (DEPTH > 1) {
      GRN_TEXT_PUTC(ctx, outbuf, '\t');
    }
  case GRN_CONTENT_NONE:
    break;
  }
}

void
grn_output_array_open(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                      const char *name, int nelements)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, outbuf, '[');
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                  "<SEGMENTS>\n<SEGMENT>\n<RESULTPAGE>\n");
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 1) { GRN_TEXT_PUTS(ctx, outbuf, "[\t"); }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_DEPTH(0);
}

void
grn_output_array_close(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type)
{
  switch (output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTC(ctx, outbuf, ']');
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 2) {
      if (CURR_LEVEL >= 2) { GRN_TEXT_PUTC(ctx, outbuf, '\t'); }
      GRN_TEXT_PUTC(ctx, outbuf, ']');
    }
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "</RESULTPAGE>\n</SEGMENT>\n</SEGMENTS>");
    if (DEPTH > 1) { GRN_TEXT_PUTC(ctx, outbuf, '\n'); }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  DECR_DEPTH;
  INCR_LENGTH;
}

void
grn_output_map_open(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                    const char *name, int nelements)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTS(ctx, outbuf, "{");
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                  "<SEGMENTS>\n<SEGMENT>\n<RESULTPAGE>\n");
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 1) { GRN_TEXT_PUTS(ctx, outbuf, "{\t"); }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_DEPTH(1);
}

void
grn_output_map_close(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type)
{
  switch (output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTS(ctx, outbuf, "}");
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 2) {
      if (CURR_LEVEL >= 2) { GRN_TEXT_PUTC(ctx, outbuf, '\t'); }
      GRN_TEXT_PUTC(ctx, outbuf, '}');
    }
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "</RESULTPAGE>\n</SEGMENT>\n</SEGMENTS>");
    if (DEPTH > 1) { GRN_TEXT_PUTC(ctx, outbuf, '\n'); }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  DECR_DEPTH;
  INCR_LENGTH;
}

void
grn_output_int32(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, int value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_itoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    grn_text_itoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_int64(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, int64_t value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_lltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_TSV:
    grn_text_lltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    grn_text_lltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_str(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
               const char *value, size_t value_len)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_esc(ctx, outbuf, value, value_len);
    break;
  case GRN_CONTENT_TSV:
    grn_text_esc(ctx, outbuf, value, value_len);
    break;
  case GRN_CONTENT_XML:
    grn_text_escape_xml(ctx, outbuf, value, value_len);
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_cstr(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                const char *value)
{
  grn_output_str(ctx, outbuf, output_type, value, strlen(value));
}

void
grn_output_obj(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, grn_obj *obj, grn_obj_format *format)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_otoj(ctx, outbuf, obj, format);
    break;
  case GRN_CONTENT_TSV:
    grn_text_otoj(ctx, outbuf, obj, format);
    break;
  case GRN_CONTENT_XML:
    grn_text_otoxml(ctx, outbuf, obj, format);
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

