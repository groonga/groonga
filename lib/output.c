/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2012 Brazil

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

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

#include <string.h>
#include "str.h"
#include "db.h"
#include "util.h"
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
    // if (DEPTH == 1 && ((level & 3) != 3)) { GRN_TEXT_PUTC(ctx, outbuf, '\n'); }
    break;
  case GRN_CONTENT_XML:
    if (!DEPTH) { return; }
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    break;
  case GRN_CONTENT_TSV:
    if (level < 2) { return; }
    if (DEPTH <= 2) {
      GRN_TEXT_PUTC(ctx, outbuf, ((level & 3) == 3) ? '\t' : '\n');
    } else {
      GRN_TEXT_PUTC(ctx, outbuf, '\t');
    }
  case GRN_CONTENT_MSGPACK :
    // do nothing
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
    if (DEPTH > 2) { GRN_TEXT_PUTS(ctx, outbuf, "[\t"); }
    break;
  case GRN_CONTENT_MSGPACK :
#ifdef WITH_MESSAGE_PACK
    if (nelements < 0) {
      GRN_LOG(ctx, GRN_LOG_DEBUG,
              "grn_output_array_open nelements (%d) for <%s>",
              nelements,
              name);
    }
    msgpack_pack_array(&ctx->impl->msgpacker, nelements);
#endif
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
    if (DEPTH > 3) {
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
    // do nothing
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
    if (DEPTH > 2) { GRN_TEXT_PUTS(ctx, outbuf, "{\t"); }
    break;
  case GRN_CONTENT_MSGPACK :
#ifdef WITH_MESSAGE_PACK
    if (nelements < 0) {
      GRN_LOG(ctx, GRN_LOG_DEBUG,
              "grn_output_map_open nelements (%d) for <%s>",
              nelements,
              name);
    }
    msgpack_pack_map(&ctx->impl->msgpacker, nelements / 2);
#endif
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
    if (DEPTH > 3) {
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
    // do nothing
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
#ifdef WITH_MESSAGE_PACK
    msgpack_pack_int32(&ctx->impl->msgpacker, value);
#endif
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
#ifdef WITH_MESSAGE_PACK
    msgpack_pack_int64(&ctx->impl->msgpacker, value);
#endif
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
#ifdef WITH_MESSAGE_PACK
    msgpack_pack_uint64(&ctx->impl->msgpacker, value);
#endif
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
#ifdef WITH_MESSAGE_PACK
    msgpack_pack_double(&ctx->impl->msgpacker, value);
#endif
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
#ifdef WITH_MESSAGE_PACK
    msgpack_pack_raw(&ctx->impl->msgpacker, value_len);
    msgpack_pack_raw_body(&ctx->impl->msgpacker, value, value_len);
#endif
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
grn_output_bool(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type, grn_bool value)
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
#ifdef WITH_MESSAGE_PACK
    if (value) {
      msgpack_pack_true(&ctx->impl->msgpacker);
    } else {
      msgpack_pack_false(&ctx->impl->msgpacker);
    }
#endif
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
#ifdef WITH_MESSAGE_PACK
      msgpack_pack_nil(&ctx->impl->msgpacker);
#endif
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
#ifdef WITH_MESSAGE_PACK
    msgpack_pack_double(&ctx->impl->msgpacker, dv);
#endif
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
      GRN_TEXT_PUTS(ctx, outbuf, "null");
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
#ifdef WITH_MESSAGE_PACK
    if (value) {
      grn_obj buf;
      GRN_TEXT_INIT(&buf, 0);
      grn_text_itoa(ctx, &buf, value->latitude);
      GRN_TEXT_PUTC(ctx, &buf, 'x');
      grn_text_itoa(ctx, &buf, value->longitude);
      msgpack_pack_raw(&ctx->impl->msgpacker, GRN_TEXT_LEN(&buf));
      msgpack_pack_raw_body(&ctx->impl->msgpacker,
                            GRN_TEXT_VALUE(&buf),
                            GRN_TEXT_LEN(&buf));
      grn_obj_close(ctx, &buf);
    } else {
      msgpack_pack_nil(&ctx->impl->msgpacker);
    }
#endif
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
      buf.header.domain = grn_obj_get_range(ctx, obj);
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
        if (GRN_BULK_VSIZE(&buf) >= sizeof(grn_id)) {
          id = *((grn_id *)GRN_BULK_HEAD(&buf));
        } else {
          id = GRN_ID_NIL;
        }
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
        grn_output_array_open(ctx, outbuf, output_type, "VECTOR", ve - v);
        if (v < ve) {
          grn_obj key;
          GRN_OBJ_INIT(&key, GRN_BULK, 0, range->header.domain);
          for (;;) {
            if (range->header.type != GRN_TABLE_NO_KEY) {
              grn_table_get_key2(ctx, range, *v, &key);
              grn_output_obj(ctx, outbuf, output_type, &key, NULL);
              GRN_BULK_REWIND(&key);
            } else {
              grn_obj id;
              GRN_UINT32_INIT(&id, 0);
              GRN_UINT32_SET(ctx, &id, *v);
              grn_output_obj(ctx, outbuf, output_type, &id, NULL);
              GRN_OBJ_FIN(ctx, &id);
            }
            v++;
            if (v < ve) {

            } else {
              break;
            }
          }
          GRN_OBJ_FIN(ctx, &key);
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
      grn_output_array_open(ctx, outbuf, output_type, "VECTOR", n);
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
      GRN_OBJ_FIN(ctx, &value);
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
      grn_obj **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);
      grn_table_cursor *tc = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0,
                                                   format->offset, format->limit,
                                                   GRN_CURSOR_ASCENDING);
      int resultset_size = -1;
      if (!tc) { ERRCLR(ctx); }
#ifdef WITH_MESSAGE_PACK
      resultset_size = 1; /* [NHITS, (COLUMNS), (HITS)] */
      if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
        resultset_size++;
      }
      resultset_size += format->limit;
#endif
      grn_output_array_open(ctx, outbuf, output_type, "RESULTSET", resultset_size);
      grn_output_array_open(ctx, outbuf, output_type, "NHITS", 1);
      if (output_type == GRN_CONTENT_XML) {
        grn_text_itoa(ctx, outbuf, format->nhits);
      } else {
        grn_output_int32(ctx, outbuf, output_type, format->nhits);
      }
      grn_output_array_close(ctx, outbuf, output_type);
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
      if (tc) {
        grn_obj id;
        GRN_TEXT_INIT(&id, 0);
        for (i = 0; !grn_table_cursor_next_o(ctx, tc, &id); i++) {
          grn_output_array_open(ctx, outbuf, output_type, "HIT", ncolumns);
          for (j = 0; j < ncolumns; j++) {
            grn_text_atoj_o(ctx, outbuf, output_type, columns[j], &id);
          }
          grn_output_array_close(ctx, outbuf, output_type);
        }
        GRN_OBJ_FIN(ctx, &id);
        grn_table_cursor_close(ctx, tc);
      }
      grn_output_array_close(ctx, outbuf, output_type);
    } else {
      int i;
      grn_obj *column = grn_obj_column(ctx, obj, "_key", 4);
      grn_table_cursor *tc = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0,
                                                   0, -1, GRN_CURSOR_ASCENDING);
      grn_output_array_open(ctx, outbuf, output_type, "HIT", -1);
      if (tc) {
        grn_obj id;
        GRN_TEXT_INIT(&id, 0);
        for (i = 0; !grn_table_cursor_next_o(ctx, tc, &id); i++) {
          /* todo:
          grn_text_atoj_o(ctx, outbuf, output_type, column, &id);
          */
          GRN_BULK_REWIND(&buf);
          grn_obj_get_value_o(ctx, column, &id, &buf);
          grn_text_esc(ctx, outbuf, GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf));
        }
        GRN_OBJ_FIN(ctx, &id);
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

typedef enum {
  XML_START,
  XML_START_ELEMENT,
  XML_END_ELEMENT,
  XML_TEXT
} xml_status;

typedef enum {
  XML_PLACE_NONE,
  XML_PLACE_COLUMN,
  XML_PLACE_HIT
} xml_place;

static char *
transform_xml_next_column(grn_obj *columns, int n)
{
  char *column = GRN_TEXT_VALUE(columns);
  while (n--) {
    while (*column) {
      column++;
    }
    column++;
  }
  return column;
}

static void
transform_xml(grn_ctx *ctx, grn_obj *output, grn_obj *transformed)
{
  char *s, *e;
  xml_status status = XML_START;
  xml_place place = XML_PLACE_NONE;
  grn_obj buf, name, columns, *expr;
  unsigned int len;
  int offset = 0, limit = 0, record_n = 0;
  int column_n = 0, column_text_n = 0, result_set_n = -1;
  int in_vector = 0, first_vector_element = 0;

  s = GRN_TEXT_VALUE(output);
  e = GRN_BULK_CURR(output);
  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_INIT(&name, 0);
  GRN_TEXT_INIT(&columns, 0);

  expr = ctx->impl->curr_expr;

#define EQUAL_NAME_P(_name) \
  (GRN_TEXT_LEN(&name) == strlen(_name) && \
   !memcmp(GRN_TEXT_VALUE(&name), _name, strlen(_name)))

  while (s < e) {
    switch (*s) {
    case '<' :
      s++;
      switch (*s) {
      case '/' :
        status = XML_END_ELEMENT;
        s++;
        break;
      default :
        status = XML_START_ELEMENT;
        break;
      }
      GRN_BULK_REWIND(&name);
      break;
    case '>' :
      switch (status) {
      case XML_START_ELEMENT :
        if (EQUAL_NAME_P("COLUMN")) {
          place = XML_PLACE_COLUMN;
          column_text_n = 0;
        } else if (EQUAL_NAME_P("HIT")) {
          place = XML_PLACE_HIT;
          column_n = 0;
          if (result_set_n == 0) {
            GRN_TEXT_PUTS(ctx, transformed, "<HIT NO=\"");
            grn_text_itoa(ctx, transformed, record_n++);
            GRN_TEXT_PUTS(ctx, transformed, "\">\n");
          } else {
            GRN_TEXT_PUTS(ctx, transformed, "<NAVIGATIONELEMENT ");
          }
        } else if (EQUAL_NAME_P("RESULTSET")) {
          GRN_BULK_REWIND(&columns);
          result_set_n++;
          if (result_set_n == 0) {
          } else {
            GRN_TEXT_PUTS(ctx, transformed, "<NAVIGATIONENTRY>\n");
          }
        } else if (EQUAL_NAME_P("VECTOR")) {
          char *c = transform_xml_next_column(&columns, column_n++);
          in_vector = 1;
          first_vector_element = 1;
          GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
          GRN_TEXT_PUTS(ctx, transformed, c);
          GRN_TEXT_PUTS(ctx, transformed, "\">");
        }
        break;
      case XML_END_ELEMENT :
        if (EQUAL_NAME_P("HIT")) {
          place = XML_PLACE_NONE;
          if (result_set_n == 0) {
            GRN_TEXT_PUTS(ctx, transformed, "</HIT>\n");
          } else {
            GRN_TEXT_PUTS(ctx, transformed, "/>\n");
          }
        } else if (EQUAL_NAME_P("RESULTSET")) {
          place = XML_PLACE_NONE;
          if (result_set_n == 0) {
            GRN_TEXT_PUTS(ctx, transformed, "</RESULTSET>\n");
          } else {
            GRN_TEXT_PUTS(ctx, transformed,
                          "</NAVIGATIONELEMENTS>\n"
                          "</NAVIGATIONENTRY>\n");
          }
        } else if (EQUAL_NAME_P("RESULT")) {
          GRN_TEXT_PUTS(ctx, transformed,
                        "</RESULTPAGE>\n"
                        "</SEGMENT>\n"
                        "</SEGMENTS>\n");
        } else if (EQUAL_NAME_P("VECTOR")) {
          in_vector = 0;
          first_vector_element = 0;
          GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
        } else {
          switch (place) {
          case XML_PLACE_HIT :
            if (result_set_n == 0) {
              if (!in_vector) {
                char *c = transform_xml_next_column(&columns, column_n++);
                GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
                GRN_TEXT_PUTS(ctx, transformed, c);
                GRN_TEXT_PUTS(ctx, transformed, "\">");
              }
              if (in_vector && !first_vector_element) {
                GRN_TEXT_PUTS(ctx, transformed, ", ");
              }
              GRN_TEXT_PUT(ctx, transformed,
                           GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
              if (!in_vector) {
                GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
              }
            } else {
              char *c = transform_xml_next_column(&columns, column_n++);
              GRN_TEXT_PUTS(ctx, transformed, c);
              GRN_TEXT_PUTS(ctx, transformed, "=\"");
              GRN_TEXT_PUT(ctx, transformed,
                           GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
              GRN_TEXT_PUTS(ctx, transformed, "\" ");
            }
            first_vector_element = 0;
            break;
          default :
            if (EQUAL_NAME_P("NHITS")) {
              if (result_set_n == 0) {
                uint32_t nhits;
                grn_obj *offset_value, *limit_value;

                nhits = grn_atoui(GRN_TEXT_VALUE(&buf), GRN_BULK_CURR(&buf),
                                  NULL);
                offset_value = grn_expr_get_var(ctx, expr,
                                                "offset", strlen("offset"));
                limit_value = grn_expr_get_var(ctx, expr,
                                               "limit", strlen("limit"));
                if (GRN_TEXT_LEN(offset_value)) {
                  offset = grn_atoi(GRN_TEXT_VALUE(offset_value),
                                    GRN_BULK_CURR(offset_value),
                                    NULL);
                } else {
                  offset = 0;
                }
                if (GRN_TEXT_LEN(limit_value)) {
                  limit = grn_atoi(GRN_TEXT_VALUE(limit_value),
                                   GRN_BULK_CURR(limit_value),
                                   NULL);
                } else {
#define DEFAULT_LIMIT 10
                  limit = DEFAULT_LIMIT;
#undef DEFAULT_LIMIT
                }
                grn_normalize_offset_and_limit(ctx, nhits, &offset, &limit);
                record_n = offset + 1;
                GRN_TEXT_PUTS(ctx, transformed,
                              "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                              "<SEGMENTS>\n"
                              "<SEGMENT>\n"
                              "<RESULTPAGE>\n"
                              "<RESULTSET OFFSET=\"");
                grn_text_lltoa(ctx, transformed, offset);
                GRN_TEXT_PUTS(ctx, transformed, "\" LIMIT=\"");
                grn_text_lltoa(ctx, transformed, limit);
                GRN_TEXT_PUTS(ctx, transformed, "\" NHITS=\"");
                grn_text_lltoa(ctx, transformed, nhits);
                GRN_TEXT_PUTS(ctx, transformed, "\">\n");
              } else {
                GRN_TEXT_PUTS(ctx, transformed,
                              "<NAVIGATIONELEMENTS COUNT=\"");
                GRN_TEXT_PUT(ctx, transformed,
                             GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
                GRN_TEXT_PUTS(ctx, transformed,
                              "\">\n");
              }
            } else if (EQUAL_NAME_P("TEXT")) {
              switch (place) {
              case XML_PLACE_COLUMN :
                if (column_text_n == 0) {
                  GRN_TEXT_PUT(ctx, &columns,
                               GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
                  GRN_TEXT_PUTC(ctx, &columns, '\0');
                }
                column_text_n++;
                break;
              default :
                break;
              }
            }
          }
        }
      default :
        break;
      }
      s++;
      GRN_BULK_REWIND(&buf);
      status = XML_TEXT;
      break;
    default :
      len = grn_charlen(ctx, s, e);
      switch (status) {
      case XML_START_ELEMENT :
      case XML_END_ELEMENT :
        GRN_TEXT_PUT(ctx, &name, s, len);
        break;
      default :
        GRN_TEXT_PUT(ctx, &buf, s, len);
        break;
      }
      s += len;
      break;
    }
  }
#undef EQUAL_NAME_P

  GRN_OBJ_FIN(ctx, &buf);
  GRN_OBJ_FIN(ctx, &name);
  GRN_OBJ_FIN(ctx, &columns);
}

#ifdef WITH_MESSAGE_PACK
typedef struct {
  grn_ctx *ctx;
  grn_obj *buffer;
} msgpack_writer_ctx;

static inline int
msgpack_buffer_writer(void* data, const char* buf, unsigned int len)
{
  msgpack_writer_ctx *writer_ctx = (msgpack_writer_ctx *)data;
  return grn_bulk_write(writer_ctx->ctx, writer_ctx->buffer, buf, len);
}
#endif

#define JSON_CALLBACK_PARAM "callback"

void
grn_output_envelope(grn_ctx *ctx,
                    grn_rc rc,
                    grn_obj *head,
                    grn_obj *body,
                    grn_obj *foot,
                    const char *file,
                    int line)

{
  double started, finished, elapsed;
  grn_obj *expr = NULL;
  grn_obj *jsonp_func = NULL;

  grn_timeval tv_now;
  grn_timeval_now(ctx, &tv_now);
  started = ctx->impl->tv.tv_sec;
  started += ctx->impl->tv.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  finished = tv_now.tv_sec;
  finished += tv_now.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  elapsed = finished - started;

  switch (ctx->impl->output_type) {
  case GRN_CONTENT_JSON:
    expr = ctx->impl->curr_expr;
    if (expr) {
      jsonp_func = grn_expr_get_var(ctx, expr, JSON_CALLBACK_PARAM,
                                    strlen(JSON_CALLBACK_PARAM));
    }
    if (jsonp_func && GRN_TEXT_LEN(jsonp_func)) {
      GRN_TEXT_PUT(ctx, head, GRN_TEXT_VALUE(jsonp_func), GRN_TEXT_LEN(jsonp_func));
      GRN_TEXT_PUTC(ctx, head, '(');
    }
    GRN_TEXT_PUTS(ctx, head, "[[");
    grn_text_itoa(ctx, head, rc);
    GRN_TEXT_PUTC(ctx, head, ',');
    grn_text_ftoa(ctx, head, started);
    GRN_TEXT_PUTC(ctx, head, ',');
    grn_text_ftoa(ctx, head, elapsed);
    if (rc != GRN_SUCCESS) {
      GRN_TEXT_PUTC(ctx, head, ',');
      grn_text_esc(ctx, head, ctx->errbuf, strlen(ctx->errbuf));
      if (ctx->errfunc && ctx->errfile) {
        grn_obj *command;
        /* TODO: output backtrace */
        GRN_TEXT_PUTS(ctx, head, ",[[");
        grn_text_esc(ctx, head, ctx->errfunc, strlen(ctx->errfunc));
        GRN_TEXT_PUTC(ctx, head, ',');
        grn_text_esc(ctx, head, ctx->errfile, strlen(ctx->errfile));
        GRN_TEXT_PUTC(ctx, head, ',');
        grn_text_itoa(ctx, head, ctx->errline);
        if (file && (command = GRN_CTX_USER_DATA(ctx)->ptr)) {
          GRN_TEXT_PUTC(ctx, head, ',');
          grn_text_esc(ctx, head, file, strlen(file));
          GRN_TEXT_PUTC(ctx, head, ',');
          grn_text_itoa(ctx, head, line);
          GRN_TEXT_PUTC(ctx, head, ',');
          grn_text_esc(ctx, head, GRN_TEXT_VALUE(command), GRN_TEXT_LEN(command));
        }
        GRN_TEXT_PUTS(ctx, head, "]]");
      }
    }
    GRN_TEXT_PUTC(ctx, head, ']');
    if (GRN_TEXT_LEN(body)) { GRN_TEXT_PUTC(ctx, head, ','); }
    GRN_TEXT_PUTC(ctx, foot, ']');
    if (jsonp_func && GRN_TEXT_LEN(jsonp_func)) {
      GRN_TEXT_PUTS(ctx, foot, ");");
    }
    break;
  case GRN_CONTENT_TSV:
    grn_text_itoa(ctx, head, rc);
    GRN_TEXT_PUTC(ctx, head, '\t');
    grn_text_ftoa(ctx, head, started);
    GRN_TEXT_PUTC(ctx, head, '\t');
    grn_text_ftoa(ctx, head, elapsed);
    if (rc != GRN_SUCCESS) {
      GRN_TEXT_PUTC(ctx, head, '\t');
      grn_text_esc(ctx, head, ctx->errbuf, strlen(ctx->errbuf));
      if (ctx->errfunc && ctx->errfile) {
        /* TODO: output backtrace */
        GRN_TEXT_PUTC(ctx, head, '\t');
        grn_text_esc(ctx, head, ctx->errfunc, strlen(ctx->errfunc));
        GRN_TEXT_PUTC(ctx, head, '\t');
        grn_text_esc(ctx, head, ctx->errfile, strlen(ctx->errfile));
        GRN_TEXT_PUTC(ctx, head, '\t');
        grn_text_itoa(ctx, head, ctx->errline);
      }
    }
    GRN_TEXT_PUTS(ctx, head, "\n");
    GRN_TEXT_PUTS(ctx, foot, "\nEND");
    break;
  case GRN_CONTENT_XML:
    {
      char buf[GRN_TABLE_MAX_KEY_SIZE];
      int is_select = 0;
      if (!rc && ctx->impl->curr_expr) {
        int len = grn_obj_name(ctx, ctx->impl->curr_expr,
                               buf, GRN_TABLE_MAX_KEY_SIZE);
        buf[len] = '\0';
        is_select = strcmp(buf, "select") == 0;
      }
      if (is_select) {
        grn_obj transformed;
        GRN_TEXT_INIT(&transformed, 0);
        transform_xml(ctx, body, &transformed);
        GRN_TEXT_SET(ctx, body,
                     GRN_TEXT_VALUE(&transformed), GRN_TEXT_LEN(&transformed));
        GRN_OBJ_FIN(ctx, &transformed);
      } else {
        GRN_TEXT_PUTS(ctx, head, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<RESULT CODE=\"");
        grn_text_itoa(ctx, head, rc);
        GRN_TEXT_PUTS(ctx, head, "\" UP=\"");
        grn_text_ftoa(ctx, head, started);
        GRN_TEXT_PUTS(ctx, head, "\" ELAPSED=\"");
        grn_text_ftoa(ctx, head, elapsed);
        GRN_TEXT_PUTS(ctx, head, "\">\n");
        if (rc != GRN_SUCCESS) {
          GRN_TEXT_PUTS(ctx, head, "<ERROR>");
          grn_text_escape_xml(ctx, head, ctx->errbuf, strlen(ctx->errbuf));
          if (ctx->errfunc && ctx->errfile) {
            /* TODO: output backtrace */
            GRN_TEXT_PUTS(ctx, head, "<INFO FUNC=\"");
            grn_text_escape_xml(ctx, head, ctx->errfunc, strlen(ctx->errfunc));
            GRN_TEXT_PUTS(ctx, head, "\" FILE=\"");
            grn_text_escape_xml(ctx, head, ctx->errfile, strlen(ctx->errfile));
            GRN_TEXT_PUTS(ctx, head, "\" LINE=\"");
            grn_text_itoa(ctx, head, ctx->errline);
            GRN_TEXT_PUTS(ctx, head, "\"/>");
          }
          GRN_TEXT_PUTS(ctx, head, "</ERROR>");
        }
        GRN_TEXT_PUTS(ctx, foot, "\n</RESULT>");
      }
    }
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef WITH_MESSAGE_PACK
    {
      msgpack_writer_ctx head_writer_ctx;
      msgpack_packer header_packer;
      int header_size;

      head_writer_ctx.ctx = ctx;
      head_writer_ctx.buffer = head;
      msgpack_packer_init(&header_packer, &head_writer_ctx, msgpack_buffer_writer);

       /* [HEAD, (BODY)] */
      if (GRN_TEXT_LEN(body) > 0) {
        msgpack_pack_array(&header_packer, 2);
      } else {
        msgpack_pack_array(&header_packer, 1);
      }

      /* HEAD := [rc, started, elapsed, (error, (ERROR DETAIL))] */
      header_size = 3;
      if (rc != GRN_SUCCESS) {
        header_size++;
        if (ctx->errfunc && ctx->errfile) {
          header_size++;
        }
      }
      msgpack_pack_array(&header_packer, header_size);
      msgpack_pack_int(&header_packer, rc);

      msgpack_pack_double(&header_packer, started);
      msgpack_pack_double(&header_packer, elapsed);

      if (rc != GRN_SUCCESS) {
        msgpack_pack_raw(&header_packer, strlen(ctx->errbuf));
        msgpack_pack_raw_body(&header_packer, ctx->errbuf, strlen(ctx->errbuf));
        if (ctx->errfunc && ctx->errfile) {
          grn_obj *command = GRN_CTX_USER_DATA(ctx)->ptr;
          int error_detail_size;

          /* ERROR DETAIL := [[errfunc, errfile, errline,
                               (file, line, command)]] */
          /* TODO: output backtrace */
          msgpack_pack_array(&header_packer, 1);
          error_detail_size = 3;
          if (command) {
            error_detail_size += 3;
          }
          msgpack_pack_array(&header_packer, error_detail_size);

          msgpack_pack_raw(&header_packer, strlen(ctx->errfunc));
          msgpack_pack_raw_body(&header_packer, ctx->errfunc, strlen(ctx->errfunc));

          msgpack_pack_raw(&header_packer, strlen(ctx->errfile));
          msgpack_pack_raw_body(&header_packer, ctx->errfile, strlen(ctx->errfile));

          msgpack_pack_int(&header_packer, ctx->errline);

          if (command) {
            if (file) {
              msgpack_pack_raw(&header_packer, strlen(file));
              msgpack_pack_raw_body(&header_packer, file, strlen(file));
            } else {
              msgpack_pack_raw(&header_packer, 7);
              msgpack_pack_raw_body(&header_packer, "(stdin)", 7);
            }

            msgpack_pack_int(&header_packer, line);

            msgpack_pack_raw(&header_packer, GRN_TEXT_LEN(command));
            msgpack_pack_raw_body(&header_packer, GRN_TEXT_VALUE(command), GRN_TEXT_LEN(command));
          }
        }
      }
    }
#endif
    break;
  case GRN_CONTENT_NONE:
    break;
  }
}
