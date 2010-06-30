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
  switch (output_type) {
  case GRN_CONTENT_JSON:
    if (level < 2) { return; }
    GRN_TEXT_PUTC(ctx, outbuf, ((level & 3) == 3) ? ':' : ',');
    if (DEPTH == 1 && ((level & 3) != 3)) { GRN_TEXT_PUTC(ctx, outbuf, '\n'); }
    break;
  case GRN_CONTENT_XML:
    if (!DEPTH) { return; }
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    break;
  case GRN_CONTENT_TSV:
    if (level < 2) { return; }
    if (DEPTH == 1) {
      GRN_TEXT_PUTC(ctx, outbuf, ((level & 3) == 3) ? '\t' : '\n');
    } else if (DEPTH > 1) {
      GRN_TEXT_PUTC(ctx, outbuf, '\t');
    }
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
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
    GRN_TEXT_PUTC(ctx, outbuf, '<');
    GRN_TEXT_PUTS(ctx, outbuf, name);
    GRN_TEXT_PUTC(ctx, outbuf, '>');
    grn_vector_add_element(ctx, &ctx->impl->names, name, strlen(name), 0, GRN_DB_SHORT_TEXT);
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 1) { GRN_TEXT_PUTS(ctx, outbuf, "[\t"); }
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
    {
      const char *name;
      unsigned int name_len = grn_vector_pop_element(ctx, &ctx->impl->names, &name, NULL, NULL);
      GRN_TEXT_PUTS(ctx, outbuf, "</");
      GRN_TEXT_PUT(ctx, outbuf, name, name_len);
      GRN_TEXT_PUTC(ctx, outbuf, '>');
    }
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
    GRN_TEXT_PUTC(ctx, outbuf, '<');
    GRN_TEXT_PUTS(ctx, outbuf, name);
    GRN_TEXT_PUTC(ctx, outbuf, '>');
    grn_vector_add_element(ctx, &ctx->impl->names, name, strlen(name), 0, GRN_DB_SHORT_TEXT);
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 1) { GRN_TEXT_PUTS(ctx, outbuf, "{\t"); }
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
    {
      const char *name;
      unsigned int name_len = grn_vector_pop_element(ctx, &ctx->impl->names, &name, NULL, NULL);
      GRN_TEXT_PUTS(ctx, outbuf, "</");
      GRN_TEXT_PUT(ctx, outbuf, name, name_len);
      GRN_TEXT_PUTC(ctx, outbuf, '>');
    }
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
    GRN_TEXT_PUTS(ctx, outbuf, "<INT>");
    grn_text_itoa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</INT>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
    GRN_TEXT_PUTS(ctx, outbuf, "<INT>");
    grn_text_lltoa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</INT>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_uint64(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, int64_t value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_ulltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_TSV:
    grn_text_ulltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<INT>");
    grn_text_ulltoa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</INT>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_float(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, double value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_ftoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_TSV:
    grn_text_ftoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<FLOAT>");
    grn_text_ftoa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</FLOAT>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
    GRN_TEXT_PUTS(ctx, outbuf, "<TEXT>");
    grn_text_escape_xml(ctx, outbuf, value, value_len);
    GRN_TEXT_PUTS(ctx, outbuf, "</TEXT>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
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
grn_output_bool(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, char value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    GRN_TEXT_PUTS(ctx, outbuf, value ? "true" : "false");
    break;
  case GRN_CONTENT_TSV:
    GRN_TEXT_PUTS(ctx, outbuf, value ? "true" : "false");
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<BOOL>");
    GRN_TEXT_PUTS(ctx, outbuf, value ? "true" : "false");
    GRN_TEXT_PUTS(ctx, outbuf, "</BOOL>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_void(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                const char *value, size_t value_len)
{
  if (value_len == sizeof(grn_id) && *(grn_id *)value == GRN_ID_NIL) {
    put_delimiter(ctx, outbuf, output_type);
    switch (output_type) {
    case GRN_CONTENT_JSON:
      GRN_TEXT_PUTS(ctx, outbuf, "null");
      break;
    case GRN_CONTENT_TSV:
      break;
    case GRN_CONTENT_XML:
      GRN_TEXT_PUTS(ctx, outbuf, "<NULL/>");
      break;
    case GRN_CONTENT_MSGPACK :
      // todo
      break;
    case GRN_CONTENT_NONE:
      break;
    }
    INCR_LENGTH;
  } else {
    grn_output_str(ctx, outbuf, output_type, value, value_len);
  }
}

void
grn_output_time(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, int64_t value)
{
  double dv = value;
  dv /= 1000000.0;
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    grn_text_ftoa(ctx, outbuf, dv);
    break;
  case GRN_CONTENT_TSV:
    grn_text_ftoa(ctx, outbuf, dv);
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<DATE>");
    grn_text_ftoa(ctx, outbuf, dv);
    GRN_TEXT_PUTS(ctx, outbuf, "</DATE>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_geo_point(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                     grn_geo_point *value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    if (value) {
      GRN_TEXT_PUTC(ctx, outbuf, '"');
      grn_text_itoa(ctx, outbuf, value->latitude);
      GRN_TEXT_PUTC(ctx, outbuf, 'x');
      grn_text_itoa(ctx, outbuf, value->longitude);
      GRN_TEXT_PUTC(ctx, outbuf, '"');
    } else {
      GRN_TEXT_PUTS(ctx, outbuf, "\"\"");
    }
    break;
  case GRN_CONTENT_TSV:
    if (value) {
      GRN_TEXT_PUTC(ctx, outbuf, '"');
      grn_text_itoa(ctx, outbuf, value->latitude);
      GRN_TEXT_PUTC(ctx, outbuf, 'x');
      grn_text_itoa(ctx, outbuf, value->longitude);
      GRN_TEXT_PUTC(ctx, outbuf, '"');
    } else {
      GRN_TEXT_PUTS(ctx, outbuf, "\"\"");
    }
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<GEO_POINT>");
    if (value) {
      grn_text_itoa(ctx, outbuf, value->latitude);
      GRN_TEXT_PUTC(ctx, outbuf, 'x');
      grn_text_itoa(ctx, outbuf, value->longitude);
    }
    GRN_TEXT_PUTS(ctx, outbuf, "</GEO_POINT>");
    break;
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

static void
grn_text_atoj(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
              grn_obj *obj, grn_id id)
{
  int vs;
  grn_obj buf;
  if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a = (grn_accessor *)obj;
    GRN_TEXT_INIT(&buf, 0);
    for (;;) {
      GRN_BULK_REWIND(&buf);
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID :
        GRN_UINT32_PUT(ctx, &buf, id);
        buf.header.domain = GRN_DB_UINT32;
        break;
      case GRN_ACCESSOR_GET_KEY :
        grn_table_get_key2(ctx, a->obj, id, &buf);
        buf.header.domain = DB_OBJ(a->obj)->header.domain;
        break;
      case GRN_ACCESSOR_GET_VALUE :
        grn_obj_get_value(ctx, a->obj, id, &buf);
        buf.header.domain = GRN_DB_INT32; /* fix me */
        break;
      case GRN_ACCESSOR_GET_SCORE :
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          GRN_INT32_PUT(ctx, &buf, ri->score);
        }
        buf.header.domain = GRN_DB_INT32;
        break;
      case GRN_ACCESSOR_GET_NSUBRECS :
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          GRN_INT32_PUT(ctx, &buf, ri->n_subrecs);
        }
        buf.header.domain = GRN_DB_INT32;
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE :
        if ((a->obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR) {
          if (a->next) {
            grn_id *idp;
            grn_obj_get_value(ctx, a->obj, id, &buf);
            idp = (grn_id *)GRN_BULK_HEAD(&buf);
            vs = GRN_BULK_VSIZE(&buf) / sizeof(grn_id);
            grn_output_array_open(ctx, outbuf, output_type, "COLUMN", vs);
            for (; vs--; idp++) {
              grn_text_atoj(ctx, outbuf, output_type, (grn_obj *)a->next, *idp);
            }
            grn_output_array_close(ctx, outbuf, output_type);
          } else {
            grn_text_atoj(ctx, outbuf, output_type, a->obj, id);
          }
          goto exit;
        } else {
          grn_obj_get_value(ctx, a->obj, id, &buf);
        }
        break;
      case GRN_ACCESSOR_GET_DB_OBJ :
        /* todo */
        break;
      case GRN_ACCESSOR_LOOKUP :
        /* todo */
        break;
      case GRN_ACCESSOR_FUNCALL :
        /* todo */
        break;
      }
      if (a->next) {
        a = a->next;
        id = *((grn_id *)GRN_BULK_HEAD(&buf));
      } else {
        break;
      }
    }
  } else {
    switch (obj->header.type) {
    case GRN_COLUMN_FIX_SIZE :
      GRN_VALUE_FIX_SIZE_INIT(&buf, 0, DB_OBJ(obj)->range);
      break;
    case GRN_COLUMN_VAR_SIZE :
      if ((obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR) {
        grn_obj *range = grn_ctx_at(ctx, DB_OBJ(obj)->range);
        if (range->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
          GRN_VALUE_VAR_SIZE_INIT(&buf, GRN_OBJ_VECTOR, DB_OBJ(obj)->range);
        } else {
          GRN_VALUE_FIX_SIZE_INIT(&buf, GRN_OBJ_VECTOR, DB_OBJ(obj)->range);
        }
      } else {
        GRN_VALUE_VAR_SIZE_INIT(&buf, 0, DB_OBJ(obj)->range);
      }
      break;
    case GRN_COLUMN_INDEX :
      GRN_UINT32_INIT(&buf, 0);
      break;
    default:
      GRN_TEXT_INIT(&buf, 0);
      break;
    }
    grn_obj_get_value(ctx, obj, id, &buf);
  }
  grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
exit :
  grn_obj_close(ctx, &buf);
}

static void
grn_text_atoj_o(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                grn_obj *obj, grn_obj *id)
{
  grn_id *idp = (grn_id *)GRN_BULK_HEAD(id);
  uint32_t ids = GRN_BULK_VSIZE(id);
  for (;;) {
    if (ids < sizeof(grn_id)) {
      ERR(GRN_INVALID_ARGUMENT, "invalid id.");
      return;
    }
    if (obj->header.type == GRN_ACCESSOR_VIEW) {
      uint32_t n;
      grn_accessor_view *v = (grn_accessor_view *)obj;
      n = *idp;
      if (n >= v->naccessors) {
        ERR(GRN_INVALID_ARGUMENT, "invalid id");
        return;
      }
      if (!(obj = v->accessors[n])) { return ; }
      idp++;
      ids -= sizeof(grn_id);
    } else {
      break;
    }
  }
  grn_text_atoj(ctx, outbuf, output_type, obj, *idp);
}

#ifdef USE_GRN_TEXT_OTOJ

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
  case GRN_CONTENT_MSGPACK :
    // todo
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

#else /* USE_GRN_TEXT_OTOJ */

void
grn_output_obj(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
               grn_obj *obj, grn_obj_format *format)
{
  grn_obj buf;
  GRN_TEXT_INIT(&buf, 0);
  switch (obj->header.type) {
  case GRN_BULK :
    switch (obj->header.domain) {
    case GRN_DB_VOID :
      grn_output_void(ctx, outbuf, output_type, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
      break;
    case GRN_DB_SHORT_TEXT :
    case GRN_DB_TEXT :
    case GRN_DB_LONG_TEXT :
      grn_output_str(ctx, outbuf, output_type, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
      break;
    case GRN_DB_BOOL :
      grn_output_bool(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_UINT8_VALUE(obj) : 0);
      break;
    case GRN_DB_INT8 :
      grn_output_int32(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_INT8_VALUE(obj) : 0);
      break;
    case GRN_DB_UINT8 :
      grn_output_int32(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_UINT8_VALUE(obj) : 0);
      break;
    case GRN_DB_INT16 :
      grn_output_int32(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_INT16_VALUE(obj) : 0);
      break;
    case GRN_DB_UINT16 :
      grn_output_int32(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_UINT16_VALUE(obj) : 0);
      break;
    case GRN_DB_INT32 :
      grn_output_int32(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_INT32_VALUE(obj) : 0);
      break;
    case GRN_DB_UINT32 :
      grn_output_int64(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_UINT32_VALUE(obj) : 0);
      break;
    case GRN_DB_INT64 :
      grn_output_int64(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_INT64_VALUE(obj) : 0);
      break;
    case GRN_DB_UINT64 :
      grn_output_uint64(ctx, outbuf, output_type,
                        GRN_BULK_VSIZE(obj) ? GRN_UINT64_VALUE(obj) : 0);
      break;
    case GRN_DB_FLOAT :
      grn_output_float(ctx, outbuf, output_type,
                       GRN_BULK_VSIZE(obj) ? GRN_FLOAT_VALUE(obj) : 0);
      break;
    case GRN_DB_TIME :
      grn_output_time(ctx, outbuf, output_type,
                      GRN_BULK_VSIZE(obj) ? GRN_INT64_VALUE(obj) : 0);
      break;
    case GRN_DB_TOKYO_GEO_POINT :
    case GRN_DB_WGS84_GEO_POINT :
      grn_output_geo_point(ctx, outbuf, output_type,
                           GRN_BULK_VSIZE(obj) ? (grn_geo_point *)GRN_BULK_HEAD(obj) : NULL);
      break;
    default :
      if (format) {
        int j;
        int ncolumns = GRN_BULK_VSIZE(&format->columns)/sizeof(grn_obj *);
        grn_obj **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);
        if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
          grn_output_array_open(ctx, outbuf, output_type, "COLUMNS", ncolumns);
          for (j = 0; j < ncolumns; j++) {
            grn_id range_id;
            grn_output_array_open(ctx, outbuf, output_type, "COLUMN", 2);
            GRN_BULK_REWIND(&buf);
            grn_column_name_(ctx, columns[j], &buf);
            grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
            /* column range */
            range_id = grn_obj_get_range(ctx, columns[j]);
            if (range_id == GRN_ID_NIL) {
              GRN_TEXT_PUTS(ctx, outbuf, "null");
            } else {
              int name_len;
              grn_obj *range_obj;
              char name_buf[GRN_TABLE_MAX_KEY_SIZE];

              range_obj = grn_ctx_at(ctx, range_id);
              name_len = grn_obj_name(ctx, range_obj, name_buf,
                                      GRN_TABLE_MAX_KEY_SIZE);
              GRN_BULK_REWIND(&buf);
              GRN_TEXT_PUT(ctx, &buf, name_buf, name_len);
              grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
            }
            grn_output_array_close(ctx, outbuf, output_type);
          }
          grn_output_array_close(ctx, outbuf, output_type);
        }
        grn_output_array_open(ctx, outbuf, output_type, "HIT", ncolumns);
        for (j = 0; j < ncolumns; j++) {
          grn_text_atoj_o(ctx, outbuf, output_type, columns[j], obj);
        }
        grn_output_array_close(ctx, outbuf, output_type);
      } else {
        grn_obj *table = grn_ctx_at(ctx, obj->header.domain);
        grn_id id = *((grn_id *)GRN_BULK_HEAD(obj));
        if (table && table->header.type != GRN_TABLE_NO_KEY) {
          grn_obj *accessor = grn_obj_column(ctx, table, "_key", 4);
          if (accessor) {
            grn_obj_get_value(ctx, accessor, id, &buf);
            grn_obj_unlink(ctx, accessor);
          }
          grn_output_obj(ctx, outbuf, output_type, &buf, format);
        } else {
          grn_output_int64(ctx, outbuf, output_type, id);
        }
      }
      break;
    }
    break;
  case GRN_UVECTOR :
    if (format) {
      int i, j;
      grn_id *v = (grn_id *)GRN_BULK_HEAD(obj), *ve = (grn_id *)GRN_BULK_CURR(obj);
      int ncolumns = GRN_BULK_VSIZE(&format->columns) / sizeof(grn_obj *);
      grn_obj **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);
      grn_output_array_open(ctx, outbuf, output_type, "RESULTSET", -1);
      grn_output_array_open(ctx, outbuf, output_type, "NHITS", 1);
      grn_text_itoa(ctx, outbuf, ve - v);
      grn_output_array_close(ctx, outbuf, output_type);
      if (v < ve) {
        if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
          grn_output_array_open(ctx, outbuf, output_type, "COLUMNS", -1);
          for (j = 0; j < ncolumns; j++) {
            grn_id range_id;
            grn_output_array_open(ctx, outbuf, output_type, "COLUMN", -1);
            GRN_BULK_REWIND(&buf);
            grn_column_name_(ctx, columns[j], &buf);
            grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
            /* column range */
            range_id = grn_obj_get_range(ctx, columns[j]);
            if (range_id == GRN_ID_NIL) {
              GRN_TEXT_PUTS(ctx, outbuf, "null");
            } else {
              int name_len;
              grn_obj *range_obj;
              char name_buf[GRN_TABLE_MAX_KEY_SIZE];

              range_obj = grn_ctx_at(ctx, range_id);
              name_len = grn_obj_name(ctx, range_obj, name_buf,
                                      GRN_TABLE_MAX_KEY_SIZE);
              GRN_BULK_REWIND(&buf);
              GRN_TEXT_PUT(ctx, &buf, name_buf, name_len);
              grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
            }
            grn_output_array_close(ctx, outbuf, output_type);
          }
          grn_output_array_close(ctx, outbuf, output_type);
        }
        for (i = 0;; i++) {
          grn_output_array_open(ctx, outbuf, output_type, "HITS", -1);
          for (j = 0; j < ncolumns; j++) {
            GRN_BULK_REWIND(&buf);
            grn_obj_get_value(ctx, columns[j], *v, &buf);
            grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
          }
          grn_output_array_close(ctx, outbuf, output_type);
          v++;
          if (v < ve) {

          } else {
            break;
          }
        }
      }
      grn_output_array_close(ctx, outbuf, output_type);
    } else {
      grn_obj *range = grn_ctx_at(ctx, obj->header.domain);
      if (range && range->header.type == GRN_TYPE) {
        int value_size = ((struct _grn_type *)range)->obj.range;
        char *v = (char *)GRN_BULK_HEAD(obj),
             *ve = (char *)GRN_BULK_CURR(obj);
        grn_output_array_open(ctx, outbuf, output_type, "VECTOR", -1);
        if (v < ve) {
          for (;;) {
            grn_obj value;
            GRN_OBJ_INIT(&value, GRN_BULK, 0, obj->header.domain);
            grn_bulk_write_from(ctx, &value, v, 0, value_size);
            grn_output_obj(ctx, outbuf, output_type, &value, NULL);

            v += value_size;
            if (v < ve) {

            } else {
              break;
            }
          }
        }
        grn_output_array_close(ctx, outbuf, output_type);
      } else {
        grn_id *v = (grn_id *)GRN_BULK_HEAD(obj),
               *ve = (grn_id *)GRN_BULK_CURR(obj);
        grn_output_array_open(ctx, outbuf, output_type, "VECTOR", -1);
        if (v < ve) {
          for (;;) {
            grn_obj key;
            if (range->header.type != GRN_TABLE_NO_KEY) {
              GRN_OBJ_INIT(&key, GRN_BULK, 0, range->header.domain);
              grn_table_get_key2(ctx, range, *v, &key);
              grn_output_obj(ctx, outbuf, output_type, &key, NULL);
            } else {
              grn_text_lltoa(ctx, outbuf, *v);
            }
            v++;
            if (v < ve) {

            } else {
              break;
            }
          }
        }
        grn_output_array_close(ctx, outbuf, output_type);
      }
    }
    break;
  case GRN_VECTOR :
    if (obj->header.domain == GRN_DB_VOID) {
      ERR(GRN_INVALID_ARGUMENT, "invalid obj->header.domain");
    }
    if (format) {
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "cannot print GRN_VECTOR using grn_obj_format");
    } else {
      unsigned int i, n;
      grn_obj value;
      GRN_VOID_INIT(&value);
      n = grn_vector_size(ctx, obj);
      grn_output_array_open(ctx, outbuf, output_type, "VECTOR", -1);
      for (i = 0; i < n; i++) {
        const char *_value;
        unsigned int weight, length;
        grn_id domain;

        length = grn_vector_get_element(ctx, obj, i,
                                        &_value, &weight, &domain);
        if (domain != GRN_DB_VOID) {
          grn_obj_reinit(ctx, &value, domain, 0);
        } else {
          grn_obj_reinit(ctx, &value, obj->header.domain, 0);
        }
        grn_bulk_write(ctx, &value, _value, length);
        grn_output_obj(ctx, outbuf, output_type, &value, NULL);
      }
      grn_output_array_close(ctx, outbuf, output_type);
    }
    break;
  case GRN_PVECTOR :
    if (format) {
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "cannot print GRN_PVECTOR using grn_obj_format");
    } else {
      unsigned int i, n;
      grn_output_array_open(ctx, outbuf, output_type, "VECTOR", -1);
      n = GRN_BULK_VSIZE(obj) / sizeof(grn_obj *);
      for (i = 0; i < n; i++) {
        grn_obj *value;

        value = GRN_PTR_VALUE_AT(obj, i);
        grn_output_obj(ctx, outbuf, output_type, value, NULL);
      }
      grn_output_array_close(ctx, outbuf, output_type);
    }
    break;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_NO_KEY :
  case GRN_TABLE_VIEW :
    if (format) {
      int i, j;
      int ncolumns = GRN_BULK_VSIZE(&format->columns)/sizeof(grn_obj *);
      grn_obj id, **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);
      grn_table_cursor *tc = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0,
                                                   format->offset, format->limit,
                                                   GRN_CURSOR_ASCENDING);
      if (!tc) { ERRCLR(ctx); }
      grn_output_array_open(ctx, outbuf, output_type, "RESULTSET", -1);
      grn_output_array_open(ctx, outbuf, output_type, "NHITS", -1);
      grn_text_itoa(ctx, outbuf, format->nhits);
      grn_output_array_close(ctx, outbuf, output_type);
      if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
        grn_output_array_open(ctx, outbuf, output_type, "COLUMNS", -1);
        for (j = 0; j < ncolumns; j++) {
          grn_id range_id;
          grn_output_array_open(ctx, outbuf, output_type, "COLUMN", -1);
          GRN_BULK_REWIND(&buf);
          grn_column_name_(ctx, columns[j], &buf);
          grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
          /* column range */
          range_id = grn_obj_get_range(ctx, columns[j]);
          if (range_id == GRN_ID_NIL) {
            GRN_TEXT_PUTS(ctx, outbuf, "null");
          } else {
            int name_len;
            grn_obj *range_obj;
            char name_buf[GRN_TABLE_MAX_KEY_SIZE];

            range_obj = grn_ctx_at(ctx, range_id);
            name_len = grn_obj_name(ctx, range_obj, name_buf,
                                    GRN_TABLE_MAX_KEY_SIZE);
            GRN_BULK_REWIND(&buf);
            GRN_TEXT_PUT(ctx, &buf, name_buf, name_len);
            grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
          }
          grn_output_array_close(ctx, outbuf, output_type);
        }
        grn_output_array_close(ctx, outbuf, output_type);
      }
      GRN_TEXT_INIT(&id, 0);
      if (tc) {
        for (i = 0; !grn_table_cursor_next_o(ctx, tc, &id); i++) {
          grn_output_array_open(ctx, outbuf, output_type, "HIT", ncolumns);
          for (j = 0; j < ncolumns; j++) {
            grn_text_atoj_o(ctx, outbuf, output_type, columns[j], &id);
          }
          grn_output_array_close(ctx, outbuf, output_type);
        }
        grn_table_cursor_close(ctx, tc);
      }
      grn_output_array_close(ctx, outbuf, output_type);
    } else {
      int i;
      grn_obj id, *column = grn_obj_column(ctx, obj, "_key", 4);
      grn_table_cursor *tc = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0,
                                                   0, -1, GRN_CURSOR_ASCENDING);
      grn_output_array_open(ctx, outbuf, output_type, "HIT", -1);
      if (tc) {
        GRN_TEXT_INIT(&id, 0);
        for (i = 0; !grn_table_cursor_next_o(ctx, tc, &id); i++) {
          /* todo:
          grn_text_atoj_o(ctx, outbuf, output_type, column, &id);
          */
          GRN_BULK_REWIND(&buf);
          grn_obj_get_value_o(ctx, column, &id, &buf);
          grn_text_esc(ctx, outbuf, GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf));
        }
        grn_table_cursor_close(ctx, tc);
      }
      grn_output_array_close(ctx, outbuf, output_type);
      grn_obj_unlink(ctx, column);
    }
    break;
  }
  GRN_OBJ_FIN(ctx, &buf);
}

#endif /* USE_GRN_TEXT_OTOJ */
