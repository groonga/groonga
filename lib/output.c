/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2023  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_expr.h"
#include "grn_float.h"
#include "grn_output.h"
#include "grn_output_columns.h"
#include "grn_str.h"
#include "grn_table.h"
#include "grn_util.h"

#include <string.h>
#include <math.h>

uint32_t grn_output_auto_flush_interval = 1024;

void
grn_output_init_from_env(void)
{
  {
    char grn_output_auto_flush_interval_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_OUTPUT_AUTO_FLUSH_INTERVAL",
               grn_output_auto_flush_interval_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_output_auto_flush_interval_env[0]) {
      size_t env_len = strlen(grn_output_auto_flush_interval_env);
      uint32_t interval =
        grn_atoui(grn_output_auto_flush_interval_env,
                  grn_output_auto_flush_interval_env + env_len,
                  NULL);
      if (interval > 0) {
        grn_output_auto_flush_interval = interval;
      }
    }
  }
}

#define LEVELS        (&ctx->impl->output.levels)
#define DEPTH         (GRN_BULK_VSIZE(LEVELS) >> 2)
#define CURR_LEVEL    (DEPTH ? (GRN_UINT32_VALUE_AT(LEVELS, (DEPTH - 1))) : 0)
#define INCR_DEPTH(i) GRN_UINT32_PUT(ctx, LEVELS, i)
#define DECR_DEPTH                                                             \
  (DEPTH ? grn_bulk_truncate(ctx,                                              \
                             LEVELS,                                           \
                             GRN_BULK_VSIZE(LEVELS) - sizeof(uint32_t))        \
         : 0)
#define INCR_LENGTH                                                            \
  (DEPTH ? (GRN_UINT32_VALUE_AT(LEVELS, (DEPTH - 1)) += 2) : 0)

grn_rc
grn_output_range_normalize(grn_ctx *ctx, int size, int *offset, int *limit)
{
  int end;
  int normalized_offset = *offset;
  int normalized_limit = *limit;

  if (normalized_limit < 0) {
    normalized_limit += size + 1;
    if (normalized_limit < 0) {
      *offset = 0;
      *limit = 0;
      return GRN_TOO_SMALL_LIMIT;
    }
  } else if (normalized_limit > size) {
    normalized_limit = size;
  }

  if (normalized_offset < 0) {
    normalized_offset += size;
    if (normalized_offset < 0) {
      if (normalized_limit + normalized_offset >= 0) {
        normalized_limit += normalized_offset;
        normalized_offset = 0;
      } else {
        *offset = 0;
        *limit = 0;
        return GRN_TOO_SMALL_OFFSET;
      }
    }
  } else if (normalized_offset != 0 && normalized_offset >= size) {
    *offset = 0;
    *limit = 0;
    return GRN_TOO_LARGE_OFFSET;
  }

  /* At this point, offset and limit must be zero or positive. */
  end = normalized_offset + normalized_limit;
  if (end > size) {
    normalized_limit -= end - size;
  }
  *offset = normalized_offset;
  *limit = normalized_limit;
  return GRN_SUCCESS;
}

static void
indent(grn_ctx *ctx, grn_obj *outbuf, size_t level)
{
  size_t i;
  for (i = 0; i < level; i++) {
    GRN_TEXT_PUTS(ctx, outbuf, "  ");
  }
}

static void
json_array_open(grn_ctx *ctx, grn_obj *outbuf, size_t *indent_level)
{
  GRN_TEXT_PUTC(ctx, outbuf, '[');
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    (*indent_level)++;
    indent(ctx, outbuf, *indent_level);
  }
}

static void
json_array_close(grn_ctx *ctx, grn_obj *outbuf, size_t *indent_level)
{
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    (*indent_level)--;
    indent(ctx, outbuf, *indent_level);
  }
  GRN_TEXT_PUTC(ctx, outbuf, ']');
}

static void
json_element_end(grn_ctx *ctx, grn_obj *outbuf, size_t indent_level)
{
  GRN_TEXT_PUTC(ctx, outbuf, ',');
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    indent(ctx, outbuf, indent_level);
  }
}

static void
json_map_open(grn_ctx *ctx, grn_obj *outbuf, size_t *indent_level)
{
  GRN_TEXT_PUTC(ctx, outbuf, '{');
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    (*indent_level)++;
    indent(ctx, outbuf, *indent_level);
  }
}

static void
json_map_close(grn_ctx *ctx, grn_obj *outbuf, size_t *indent_level)
{
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    (*indent_level)--;
    indent(ctx, outbuf, *indent_level);
  }
  GRN_TEXT_PUTC(ctx, outbuf, '}');
}

static void
json_key_end(grn_ctx *ctx, grn_obj *outbuf)
{
  GRN_TEXT_PUTC(ctx, outbuf, ':');
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, ' ');
  }
}

static void
json_key(grn_ctx *ctx, grn_obj *outbuf, const char *key)
{
  grn_text_esc(ctx, outbuf, key, strlen(key));
  json_key_end(ctx, outbuf);
}

static void
json_value_end(grn_ctx *ctx, grn_obj *outbuf, size_t indent_level)
{
  GRN_TEXT_PUTC(ctx, outbuf, ',');
  if (ctx->impl->output.is_pretty) {
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    indent(ctx, outbuf, indent_level);
  }
}

static void
put_delimiter(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type)
{
  uint32_t level = CURR_LEVEL;
  switch (output_type) {
  case GRN_CONTENT_JSON:
    if (level < 2) {
      if (DEPTH > 0 && ctx->impl->output.is_pretty) {
        GRN_TEXT_PUTC(ctx, outbuf, '\n');
        indent(ctx, outbuf, DEPTH + 1);
      }
      return;
    }
    if ((level & 3) == 3) {
      GRN_TEXT_PUTC(ctx, outbuf, ':');
      if (ctx->impl->output.is_pretty) {
        GRN_TEXT_PUTC(ctx, outbuf, ' ');
      }
    } else {
      json_element_end(ctx, outbuf, DEPTH + 1);
    }
    // if (DEPTH == 1 && ((level & 3) != 3)) { GRN_TEXT_PUTC(ctx, outbuf, '\n');
    // }
    break;
  case GRN_CONTENT_XML:
    if (!DEPTH) {
      return;
    }
    GRN_TEXT_PUTC(ctx, outbuf, '\n');
    break;
  case GRN_CONTENT_TSV:
    if (level < 2) {
      return;
    }
    if (DEPTH <= 2) {
      GRN_TEXT_PUTC(ctx, outbuf, ((level & 3) == 3) ? '\t' : '\n');
    } else {
      GRN_TEXT_PUTC(ctx, outbuf, '\t');
    }
  case GRN_CONTENT_MSGPACK:
    // do nothing
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
}

void
grn_output_array_open(grn_ctx *ctx,
                      grn_obj *outbuf,
                      grn_content_type output_type,
                      const char *name,
                      int nelements)
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
    grn_vector_add_element(ctx,
                           &ctx->impl->output.names,
                           name,
                           (uint32_t)strlen(name),
                           0,
                           GRN_DB_SHORT_TEXT);
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 2) {
      GRN_TEXT_PUTS(ctx, outbuf, "[\t");
    }
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    if (nelements < 0) {
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "grn_output_array_open nelements (%d) for <%s>",
              nelements,
              name);
    }
    msgpack_pack_array(&ctx->impl->output.msgpacker, (size_t)nelements);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_DEPTH(0);
}

void
grn_output_array_close(grn_ctx *ctx,
                       grn_obj *outbuf,
                       grn_content_type output_type)
{
  switch (output_type) {
  case GRN_CONTENT_JSON:
    if (ctx->impl->output.is_pretty) {
      GRN_TEXT_PUTC(ctx, outbuf, '\n');
      indent(ctx, outbuf, DEPTH);
    }
    GRN_TEXT_PUTC(ctx, outbuf, ']');
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 3) {
      if (CURR_LEVEL >= 2) {
        GRN_TEXT_PUTC(ctx, outbuf, '\t');
      }
      GRN_TEXT_PUTC(ctx, outbuf, ']');
    }
    break;
  case GRN_CONTENT_XML:
    {
      const char *name;
      unsigned int name_len;
      name_len = grn_vector_pop_element(ctx,
                                        &ctx->impl->output.names,
                                        &name,
                                        NULL,
                                        NULL);
      GRN_TEXT_PUTS(ctx, outbuf, "</");
      GRN_TEXT_PUT(ctx, outbuf, name, name_len);
      GRN_TEXT_PUTC(ctx, outbuf, '>');
    }
    break;
  case GRN_CONTENT_MSGPACK:
    // do nothing
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  DECR_DEPTH;
  INCR_LENGTH;
}

void
grn_output_map_open(grn_ctx *ctx,
                    grn_obj *outbuf,
                    grn_content_type output_type,
                    const char *name,
                    int nelements)
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
    grn_vector_add_element(ctx,
                           &ctx->impl->output.names,
                           name,
                           (uint32_t)strlen(name),
                           0,
                           GRN_DB_SHORT_TEXT);
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 2) {
      GRN_TEXT_PUTS(ctx, outbuf, "{\t");
    }
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    if (nelements < 0) {
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "grn_output_map_open nelements (%d) for <%s>",
              nelements,
              name);
    }
    msgpack_pack_map(&ctx->impl->output.msgpacker, (size_t)nelements);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_DEPTH(1);
}

void
grn_output_map_close(grn_ctx *ctx,
                     grn_obj *outbuf,
                     grn_content_type output_type)
{
  switch (output_type) {
  case GRN_CONTENT_JSON:
    if (ctx->impl->output.is_pretty) {
      GRN_TEXT_PUTC(ctx, outbuf, '\n');
      indent(ctx, outbuf, DEPTH);
    }
    GRN_TEXT_PUTS(ctx, outbuf, "}");
    break;
  case GRN_CONTENT_TSV:
    if (DEPTH > 3) {
      if (CURR_LEVEL >= 2) {
        GRN_TEXT_PUTC(ctx, outbuf, '\t');
      }
      GRN_TEXT_PUTC(ctx, outbuf, '}');
    }
    break;
  case GRN_CONTENT_XML:
    {
      const char *name;
      unsigned int name_len;
      name_len = grn_vector_pop_element(ctx,
                                        &ctx->impl->output.names,
                                        &name,
                                        NULL,
                                        NULL);
      GRN_TEXT_PUTS(ctx, outbuf, "</");
      GRN_TEXT_PUT(ctx, outbuf, name, name_len);
      GRN_TEXT_PUTC(ctx, outbuf, '>');
    }
    break;
  case GRN_CONTENT_MSGPACK:
    // do nothing
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  DECR_DEPTH;
  INCR_LENGTH;
}

static void
grn_output_int8(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                int8_t value)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_int8(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_itoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_int8(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

static void
grn_output_uint8(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 uint8_t value)
{
  /* TODO */
  grn_output_uint32(ctx, outbuf, output_type, value);
}

static void
grn_output_int16(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 int16_t value)
{
  /* TODO */
  grn_output_int32(ctx, outbuf, output_type, value);
}

static void
grn_output_uint16(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  uint16_t value)
{
  /* TODO */
  grn_output_uint32(ctx, outbuf, output_type, value);
}

void
grn_output_int32(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 int32_t value)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_int32(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_itoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_int32(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_uint32(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  uint32_t value)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_uint32(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_ulltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_uint32(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_int64(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 int64_t value)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_int64(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_lltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_int64(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_uint64(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  uint64_t value)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_uint64(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_ulltoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_uint64(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

#ifdef GRN_HAVE_BFLOAT16
void
grn_output_bfloat16(grn_ctx *ctx,
                    grn_obj *outbuf,
                    grn_content_type output_type,
                    grn_bfloat16 value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    switch (fpclassify(grn_bfloat16_to_float32(value))) {
    case FP_NAN:
    case FP_INFINITE:
      GRN_TEXT_PUTS(ctx, outbuf, "null");
      break;
    default:
      grn_text_bf16toa(ctx, outbuf, value);
      break;
    }
    break;
  case GRN_CONTENT_TSV:
    grn_text_bf16toa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<BFLOAT16>");
    grn_text_bf16toa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</BFLOAT16>");
    break;
  case GRN_CONTENT_MSGPACK:
#  ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_float(&ctx->impl->output.msgpacker,
                       grn_bfloat16_to_float32(value));
#  endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_bf16toa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_bfloat16(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}
#endif

void
grn_output_float32(grn_ctx *ctx,
                   grn_obj *outbuf,
                   grn_content_type output_type,
                   float value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    switch (fpclassify(value)) {
    case FP_NAN:
    case FP_INFINITE:
      GRN_TEXT_PUTS(ctx, outbuf, "null");
      break;
    default:
      grn_text_f32toa(ctx, outbuf, value);
      break;
    }
    break;
  case GRN_CONTENT_TSV:
    grn_text_f32toa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<FLOAT32>");
    grn_text_f32toa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</FLOAT32>");
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_float(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_f32toa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_float32(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_float(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 double value)
{
  put_delimiter(ctx, outbuf, output_type);
  switch (output_type) {
  case GRN_CONTENT_JSON:
    switch (fpclassify(value)) {
    case FP_NAN:
    case FP_INFINITE:
      GRN_TEXT_PUTS(ctx, outbuf, "null");
      break;
    default:
      grn_text_ftoa(ctx, outbuf, value);
      break;
    }
    break;
  case GRN_CONTENT_TSV:
    grn_text_ftoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_XML:
    GRN_TEXT_PUTS(ctx, outbuf, "<FLOAT>");
    grn_text_ftoa(ctx, outbuf, value);
    GRN_TEXT_PUTS(ctx, outbuf, "</FLOAT>");
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_double(&ctx->impl->output.msgpacker, value);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_ftoa(ctx, outbuf, value);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_float(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_str(grn_ctx *ctx,
               grn_obj *outbuf,
               grn_content_type output_type,
               const char *value,
               size_t value_len)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_str(&ctx->impl->output.msgpacker, value_len);
    msgpack_pack_str_body(&ctx->impl->output.msgpacker, value, value_len);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    GRN_TEXT_PUT(ctx, outbuf, value, value_len);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_text(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        value,
        value_len);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_cstr(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                const char *value)
{
  grn_output_str(ctx, outbuf, output_type, value, strlen(value));
}

void
grn_output_bool(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                bool value)
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    if (value) {
      msgpack_pack_true(&ctx->impl->output.msgpacker);
    } else {
      msgpack_pack_false(&ctx->impl->output.msgpacker);
    }
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    GRN_TEXT_PUTS(ctx, outbuf, value ? "true" : "false");
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_null(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type)
{
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_nil(&ctx->impl->output.msgpacker);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

static grn_inline void
grn_output_bulk_void(grn_ctx *ctx,
                     grn_obj *outbuf,
                     grn_content_type output_type,
                     const char *value,
                     size_t value_len)
{
  if (value_len == sizeof(grn_id) && *(grn_id *)value == GRN_ID_NIL) {
    grn_output_null(ctx, outbuf, output_type);
  } else {
    grn_output_str(ctx, outbuf, output_type, value, value_len);
  }
}

void
grn_output_time(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                int64_t value)
{
  double dv = (double)value;
  dv /= GRN_TIME_USEC_PER_SEC_F;
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_pack_double(&ctx->impl->output.msgpacker, dv);
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    grn_text_ftoa(ctx, outbuf, dv);
    break;
  case GRN_CONTENT_APACHE_ARROW:
    if (ctx->impl->output.arrow_stream_writer) {
      grn_timeval timeval;
      timeval.tv_sec = GRN_TIME_USEC_TO_SEC(value);
      timeval.tv_nsec =
        GRN_TIME_USEC_TO_NSEC((int32_t)(value % GRN_TIME_USEC_PER_SEC));
      grn_arrow_stream_writer_add_column_timestamp(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        timeval);
    }
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

void
grn_output_geo_point(grn_ctx *ctx,
                     grn_obj *outbuf,
                     grn_content_type output_type,
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
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    if (value) {
      grn_obj buf;
      GRN_TEXT_INIT(&buf, 0);
      grn_text_itoa(ctx, &buf, value->latitude);
      GRN_TEXT_PUTC(ctx, &buf, 'x');
      grn_text_itoa(ctx, &buf, value->longitude);
      msgpack_pack_str(&ctx->impl->output.msgpacker, GRN_TEXT_LEN(&buf));
      msgpack_pack_str_body(&ctx->impl->output.msgpacker,
                            GRN_TEXT_VALUE(&buf),
                            GRN_TEXT_LEN(&buf));
      grn_obj_close(ctx, &buf);
    } else {
      msgpack_pack_nil(&ctx->impl->output.msgpacker);
    }
#endif
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
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
  case GRN_CONTENT_APACHE_ARROW:
    break;
  case GRN_CONTENT_NONE:
    break;
  }
  INCR_LENGTH;
}

static void
grn_output_record(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  grn_obj *record)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_add_column_record(
        ctx,
        ctx->impl->output.arrow_stream_writer,
        record);
    }
  } else {
    grn_obj *table = grn_ctx_at(ctx, record->header.domain);
    grn_id id = GRN_RECORD_VALUE(record);
    if (table && table->header.type != GRN_TABLE_NO_KEY) {
      grn_obj buf;
      grn_obj *accessor = grn_obj_column(ctx,
                                         table,
                                         GRN_COLUMN_NAME_KEY,
                                         GRN_COLUMN_NAME_KEY_LEN);
      GRN_VOID_INIT(&buf);
      if (accessor) {
        if (id == GRN_ID_NIL) {
          grn_obj_reinit_for(ctx, &buf, accessor);
        } else {
          grn_obj_get_value(ctx, accessor, id, &buf);
        }
        grn_obj_unlink(ctx, accessor);
      }
      grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
      GRN_OBJ_FIN(ctx, &buf);
    } else {
      grn_output_int64(ctx, outbuf, output_type, id);
    }
    if (table) {
      grn_obj_unref(ctx, table);
    }
  }
}

typedef struct {
  grn_obj bulk;
  grn_obj vector;
  grn_obj sub_ids_stack;
  size_t next_sub_ids;
  size_t n_processed_records;
  bool auto_flush;
} grn_output_table_data;

static void
grn_output_table_data_init(grn_ctx *ctx,
                           grn_output_table_data *data,
                           grn_obj_format *format)
{
  GRN_TEXT_INIT(&(data->bulk), 0);
  GRN_TEXT_INIT(&(data->vector), GRN_OBJ_VECTOR);
  GRN_TEXT_INIT(&(data->sub_ids_stack), 0);
  data->next_sub_ids = 0;
  data->n_processed_records = 0;
  data->auto_flush = (format && (format->flags & GRN_OBJ_FORMAT_AUTO_FLUSH));
}

static grn_obj *
grn_output_table_data_push_sub_ids(grn_ctx *ctx, grn_output_table_data *data)
{
  grn_obj *sub_ids = NULL;
  size_t n_sub_ids = GRN_BULK_VSIZE(&(data->sub_ids_stack)) / sizeof(grn_obj);
  bool reuse = (data->next_sub_ids < n_sub_ids);
  if (!reuse) {
    grn_obj dummy = {0};
    grn_bulk_write(ctx,
                   &(data->sub_ids_stack),
                   (void *)&dummy,
                   sizeof(grn_obj));
  }
  grn_obj *sub_ids_stack_raw = (grn_obj *)GRN_BULK_HEAD(&(data->sub_ids_stack));
  sub_ids = &(sub_ids_stack_raw[data->next_sub_ids]);
  if (reuse) {
    GRN_BULK_REWIND(sub_ids);
  } else {
    GRN_RECORD_INIT(sub_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
  }
  data->next_sub_ids++;
  return sub_ids;
}

static void
grn_output_table_data_pop_sub_ids(grn_ctx *ctx, grn_output_table_data *data)
{
  data->next_sub_ids--;
}

static void
grn_output_table_data_open_record(grn_ctx *ctx, grn_output_table_data *data)
{
  if (!data->auto_flush) {
    return;
  }
  if (data->n_processed_records == 0) {
    return;
  }

  if ((data->n_processed_records % grn_output_auto_flush_interval) == 0) {
    const int flags = 0;
    grn_ctx_output_flush(ctx, flags);
  }
}

static void
grn_output_table_data_close_record(grn_ctx *ctx, grn_output_table_data *data)
{
  data->n_processed_records++;
}

static void
grn_output_table_data_fin(grn_ctx *ctx, grn_output_table_data *data)
{
  GRN_OBJ_FIN(ctx, &(data->bulk));
  GRN_OBJ_FIN(ctx, &(data->vector));
  size_t i;
  size_t n_sub_ids = GRN_BULK_VSIZE(&(data->sub_ids_stack)) / sizeof(grn_obj);
  grn_obj *sub_ids_stack_raw = (grn_obj *)GRN_BULK_HEAD(&(data->sub_ids_stack));
  for (i = 0; i < n_sub_ids; i++) {
    GRN_OBJ_FIN(ctx, &(sub_ids_stack_raw[i]));
  }
  GRN_OBJ_FIN(ctx, &(data->sub_ids_stack));
}

static void
grn_output_table_column_value(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              grn_obj *obj,
                              grn_id id,
                              grn_output_table_data *data)
{
  uint32_t vs;
  grn_obj *bulk = &(data->bulk);
  grn_obj *vector = &(data->vector);
  if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a = (grn_accessor *)obj;
    grn_obj_reinit(ctx, bulk, GRN_DB_TEXT, 0);
    for (;;) {
      bulk->header.domain = grn_obj_get_range(ctx, obj);
      GRN_BULK_REWIND(bulk);
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID:
        GRN_UINT32_PUT(ctx, bulk, id);
        bulk->header.domain = GRN_DB_UINT32;
        break;
      case GRN_ACCESSOR_GET_KEY:
        grn_table_get_key2(ctx, a->obj, id, bulk);
        bulk->header.domain = DB_OBJ(a->obj)->header.domain;
        break;
      case GRN_ACCESSOR_GET_VALUE:
        grn_obj_get_value(ctx, a->obj, id, bulk);
        bulk->header.domain = DB_OBJ(a->obj)->range;
        break;
      case GRN_ACCESSOR_GET_SCORE:
        {
          double score = grn_table_get_score(ctx, a->obj, id);
          if (grn_ctx_get_command_version(ctx) == GRN_COMMAND_VERSION_1) {
            int32_t int32_score = (int32_t)score;
            GRN_INT32_PUT(ctx, bulk, int32_score);
            bulk->header.domain = GRN_DB_INT32;
          } else {
            GRN_FLOAT_PUT(ctx, bulk, score);
            bulk->header.domain = GRN_DB_FLOAT;
          }
        }
        break;
      case GRN_ACCESSOR_GET_NSUBRECS:
        {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          GRN_INT32_PUT(ctx, bulk, ri->n_subrecs);
        }
        bulk->header.domain = GRN_DB_INT32;
        break;
      case GRN_ACCESSOR_GET_MAX:
        bulk->header.domain = DB_OBJ(a->obj)->group.aggregated_value_type_id;
        {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          grn_rset_aggregated_value max =
            grn_rset_recinfo_get_max(ctx, ri, a->obj);
          if (bulk->header.domain == GRN_DB_INT64) {
            GRN_INT64_PUT(ctx, bulk, max.value_int64);
          } else {
            GRN_FLOAT_PUT(ctx, bulk, max.value_double);
          }
        }
        break;
      case GRN_ACCESSOR_GET_MIN:
        bulk->header.domain = DB_OBJ(a->obj)->group.aggregated_value_type_id;
        {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          grn_rset_aggregated_value min =
            grn_rset_recinfo_get_min(ctx, ri, a->obj);
          if (bulk->header.domain == GRN_DB_INT64) {
            GRN_INT64_PUT(ctx, bulk, min.value_int64);
          } else {
            GRN_FLOAT_PUT(ctx, bulk, min.value_double);
          }
        }
        break;
      case GRN_ACCESSOR_GET_SUM:
        bulk->header.domain = DB_OBJ(a->obj)->group.aggregated_value_type_id;
        {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          grn_rset_aggregated_value sum =
            grn_rset_recinfo_get_sum(ctx, ri, a->obj);
          if (bulk->header.domain == GRN_DB_INT64) {
            GRN_INT64_PUT(ctx, bulk, sum.value_int64);
          } else {
            GRN_FLOAT_PUT(ctx, bulk, sum.value_double);
          }
        }
        break;
      case GRN_ACCESSOR_GET_AVG:
      case GRN_ACCESSOR_GET_MEAN:
        {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          double mean;
          mean = grn_rset_recinfo_get_mean(ctx, ri, a->obj);
          GRN_FLOAT_PUT(ctx, bulk, mean);
        }
        bulk->header.domain = GRN_DB_FLOAT;
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE:
        switch (a->obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
        case GRN_OBJ_COLUMN_VECTOR:
          if (a->next) {
            grn_obj *sub_ids = grn_output_table_data_push_sub_ids(ctx, data);
            sub_ids->header.domain = DB_OBJ(a->obj)->range;
            grn_obj_get_value(ctx, a->obj, id, sub_ids);
            size_t n = GRN_RECORD_VECTOR_SIZE(sub_ids);
            grn_output_array_open(ctx, outbuf, output_type, "VECTOR", (int)n);
            size_t i;
            for (i = 0; i < n; i++) {
              grn_id sub_id = GRN_RECORD_VALUE_AT(sub_ids, i);
              grn_output_table_column_value(ctx,
                                            outbuf,
                                            output_type,
                                            (grn_obj *)a->next,
                                            sub_id,
                                            data);
            }
            grn_output_table_data_pop_sub_ids(ctx, data);
            grn_output_array_close(ctx, outbuf, output_type);
          } else {
            grn_output_table_column_value(ctx,
                                          outbuf,
                                          output_type,
                                          a->obj,
                                          id,
                                          data);
          }
          return;
        case GRN_OBJ_COLUMN_INDEX:
          if (a->next) {
            grn_ii *ii = (grn_ii *)(a->obj);
            grn_obj *sub_ids = grn_output_table_data_push_sub_ids(ctx, data);
            sub_ids->header.domain = DB_OBJ(ii)->range;
            grn_ii_cursor *cursor =
              grn_ii_cursor_open(ctx,
                                 ii,
                                 id,
                                 GRN_ID_NIL,
                                 GRN_ID_MAX,
                                 (int)grn_ii_get_n_elements(ctx, ii),
                                 0);
            if (cursor) {
              grn_posting *posting;
              while ((posting = grn_ii_cursor_next(ctx, cursor))) {
                GRN_RECORD_PUT(ctx, sub_ids, posting->rid);
              }
              grn_ii_cursor_close(ctx, cursor);
            }
            size_t n = GRN_RECORD_VECTOR_SIZE(sub_ids);
            grn_output_array_open(ctx, outbuf, output_type, "VECTOR", (int)n);
            size_t i;
            for (i = 0; i < n; i++) {
              grn_id sub_id = GRN_RECORD_VALUE_AT(sub_ids, i);
              grn_output_table_column_value(ctx,
                                            outbuf,
                                            output_type,
                                            (grn_obj *)(a->next),
                                            sub_id,
                                            data);
            }
            grn_output_table_data_pop_sub_ids(ctx, data);
            grn_output_array_close(ctx, outbuf, output_type);
            return;
          } else {
            grn_obj_get_value(ctx, a->obj, id, bulk);
            break;
          }
        default:
          grn_obj_get_value(ctx, a->obj, id, bulk);
          break;
        }
        break;
      case GRN_ACCESSOR_GET_DB_OBJ:
        /* todo */
        break;
      case GRN_ACCESSOR_LOOKUP:
        /* todo */
        break;
      case GRN_ACCESSOR_FUNCALL:
        /* todo */
        break;
      }
      if (a->next) {
        a = a->next;
        if (GRN_BULK_VSIZE(bulk) >= sizeof(grn_id)) {
          id = *((grn_id *)GRN_BULK_HEAD(bulk));
        } else {
          id = GRN_ID_NIL;
        }
      } else {
        break;
      }
    }
    grn_output_obj(ctx, outbuf, output_type, bulk, NULL);
  } else {
    grn_obj *buf = NULL;
    grn_obj_format *format_argument = NULL;
    grn_obj_format format;
    GRN_OBJ_FORMAT_INIT(&format, 0, 0, 0, 0);
    switch (obj->header.type) {
    case GRN_COLUMN_FIX_SIZE:
      grn_obj_reinit(ctx, bulk, DB_OBJ(obj)->range, 0);
      buf = bulk;
      break;
    case GRN_COLUMN_VAR_SIZE:
      {
        grn_column_flags flags = grn_column_get_flags(ctx, obj);
        if ((flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR) {
          bool is_var_size_element =
            grn_type_id_is_text_family(ctx, DB_OBJ(obj)->range);
          if (is_var_size_element) {
            buf = vector;
          } else {
            buf = bulk;
          }
          grn_obj_reinit(ctx, buf, DB_OBJ(obj)->range, GRN_OBJ_VECTOR);
          if (flags & GRN_OBJ_WITH_WEIGHT) {
            format.flags |= GRN_OBJ_FORMAT_WITH_WEIGHT;
            if (flags & (GRN_OBJ_WEIGHT_FLOAT32 | GRN_OBJ_WEIGHT_BFLOAT16)) {
              format.flags |= GRN_OBJ_FORMAT_WEIGHT_FLOAT32;
            }
            format_argument = &format;
          }
        } else {
          grn_obj_reinit(ctx, bulk, DB_OBJ(obj)->range, 0);
          buf = bulk;
        }
      }
      break;
    case GRN_COLUMN_INDEX:
      grn_obj_reinit(ctx, bulk, GRN_DB_UINT32, 0);
      buf = bulk;
      break;
    default:
      grn_obj_reinit(ctx, bulk, GRN_DB_TEXT, 0);
      buf = bulk;
      break;
    }
    grn_obj_get_value(ctx, obj, id, buf);
    grn_output_obj(ctx, outbuf, output_type, buf, format_argument);
  }
}

static grn_inline void
grn_output_void(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                grn_obj *bulk,
                grn_obj_format *format)
{
  grn_output_null(ctx, outbuf, output_type);
}

static grn_inline void
grn_output_bulk(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                grn_obj *bulk,
                grn_obj_format *format)
{
  grn_obj buf;
  GRN_TEXT_INIT(&buf, 0);
  switch (bulk->header.domain) {
  case GRN_DB_VOID:
    grn_output_bulk_void(ctx,
                         outbuf,
                         output_type,
                         GRN_BULK_HEAD(bulk),
                         GRN_BULK_VSIZE(bulk));
    break;
  case GRN_DB_SHORT_TEXT:
  case GRN_DB_TEXT:
  case GRN_DB_LONG_TEXT:
    grn_output_str(ctx,
                   outbuf,
                   output_type,
                   GRN_BULK_HEAD(bulk),
                   GRN_BULK_VSIZE(bulk));
    break;
  case GRN_DB_BOOL:
    grn_output_bool(ctx,
                    outbuf,
                    output_type,
                    GRN_BULK_VSIZE(bulk) ? GRN_UINT8_VALUE(bulk) : 0);
    break;
  case GRN_DB_INT8:
    grn_output_int8(ctx,
                    outbuf,
                    output_type,
                    GRN_BULK_VSIZE(bulk) ? GRN_INT8_VALUE(bulk) : 0);
    break;
  case GRN_DB_UINT8:
    grn_output_uint8(ctx,
                     outbuf,
                     output_type,
                     GRN_BULK_VSIZE(bulk) ? GRN_UINT8_VALUE(bulk) : 0);
    break;
  case GRN_DB_INT16:
    grn_output_int16(ctx,
                     outbuf,
                     output_type,
                     GRN_BULK_VSIZE(bulk) ? GRN_INT16_VALUE(bulk) : 0);
    break;
  case GRN_DB_UINT16:
    grn_output_uint16(ctx,
                      outbuf,
                      output_type,
                      GRN_BULK_VSIZE(bulk) ? GRN_UINT16_VALUE(bulk) : 0);
    break;
  case GRN_DB_INT32:
    grn_output_int32(ctx,
                     outbuf,
                     output_type,
                     GRN_BULK_VSIZE(bulk) ? GRN_INT32_VALUE(bulk) : 0);
    break;
  case GRN_DB_UINT32:
    grn_output_uint32(ctx,
                      outbuf,
                      output_type,
                      GRN_BULK_VSIZE(bulk) ? GRN_UINT32_VALUE(bulk) : 0);
    break;
  case GRN_DB_INT64:
    grn_output_int64(ctx,
                     outbuf,
                     output_type,
                     GRN_BULK_VSIZE(bulk) ? GRN_INT64_VALUE(bulk) : 0);
    break;
  case GRN_DB_UINT64:
    grn_output_uint64(ctx,
                      outbuf,
                      output_type,
                      GRN_BULK_VSIZE(bulk) ? GRN_UINT64_VALUE(bulk) : 0);
    break;
#ifdef GRN_HAVE_BFLOAT16
  case GRN_DB_BFLOAT16:
    grn_output_bfloat16(ctx,
                        outbuf,
                        output_type,
                        GRN_BULK_VSIZE(bulk) > 0 ? GRN_BFLOAT16_VALUE(bulk)
                                                 : 0);
    break;
#endif
  case GRN_DB_FLOAT32:
    grn_output_float32(ctx,
                       outbuf,
                       output_type,
                       GRN_BULK_VSIZE(bulk) ? GRN_FLOAT32_VALUE(bulk) : 0);
    break;
  case GRN_DB_FLOAT:
    grn_output_float(ctx,
                     outbuf,
                     output_type,
                     GRN_BULK_VSIZE(bulk) ? GRN_FLOAT_VALUE(bulk) : 0);
    break;
  case GRN_DB_TIME:
    grn_output_time(ctx,
                    outbuf,
                    output_type,
                    GRN_BULK_VSIZE(bulk) ? GRN_INT64_VALUE(bulk) : 0);
    break;
  case GRN_DB_TOKYO_GEO_POINT:
  case GRN_DB_WGS84_GEO_POINT:
    grn_output_geo_point(
      ctx,
      outbuf,
      output_type,
      GRN_BULK_VSIZE(bulk) ? (grn_geo_point *)GRN_BULK_HEAD(bulk) : NULL);
    break;
  default:
    if (format) {
      size_t j;
      size_t ncolumns = GRN_BULK_VSIZE(&format->columns) / sizeof(grn_obj *);
      grn_id id = GRN_RECORD_VALUE(bulk);
      grn_obj **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);
      if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
        grn_output_array_open(ctx,
                              outbuf,
                              output_type,
                              "COLUMNS",
                              (int)ncolumns);
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
            name_len =
              grn_obj_name(ctx, range_obj, name_buf, GRN_TABLE_MAX_KEY_SIZE);
            GRN_BULK_REWIND(&buf);
            GRN_TEXT_PUT(ctx, &buf, name_buf, name_len);
            grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
          }
          grn_output_array_close(ctx, outbuf, output_type);
        }
        grn_output_array_close(ctx, outbuf, output_type);
      }
      grn_output_array_open(ctx, outbuf, output_type, "HIT", (int)ncolumns);
      {
        grn_output_table_data data;
        grn_output_table_data_init(ctx, &data, format);
        for (j = 0; j < ncolumns; j++) {
          grn_output_table_column_value(ctx,
                                        outbuf,
                                        output_type,
                                        columns[j],
                                        id,
                                        &data);
        }
        grn_output_table_data_fin(ctx, &data);
      }
      grn_output_array_close(ctx, outbuf, output_type);
    } else {
      grn_output_record(ctx, outbuf, output_type, bulk);
    }
    break;
  }
  GRN_OBJ_FIN(ctx, &buf);
}

static void
grn_output_uvector_result_set(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              grn_obj *uvector,
                              grn_obj_format *format)
{
  uint32_t i_hit, n_hits;
  size_t i_column, n_columns;
  uint32_t n_elements;
  grn_obj **columns;
  grn_obj buf;
  bool with_column_names = false;

  n_hits = grn_vector_size(ctx, uvector);

  n_columns = GRN_BULK_VSIZE(&format->columns) / sizeof(grn_obj *);
  columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);

  GRN_TEXT_INIT(&buf, 0);

  if (n_hits > 0 && format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
    with_column_names = true;
  }

  n_elements = 1; /* for NHITS */
  if (with_column_names) {
    n_elements += 1; /* for COLUMNS */
  }
  n_elements += n_hits; /* for HITS */
  grn_output_array_open(ctx, outbuf, output_type, "RESULTSET", (int)n_elements);

  grn_output_array_open(ctx, outbuf, output_type, "NHITS", 1);
  grn_text_itoa(ctx, outbuf, (int)n_hits);
  grn_output_array_close(ctx, outbuf, output_type);

  if (with_column_names) {
    grn_output_array_open(ctx, outbuf, output_type, "COLUMNS", (int)n_columns);
    for (i_column = 0; i_column < n_columns; i_column++) {
      grn_id range_id;
      grn_output_array_open(ctx, outbuf, output_type, "COLUMN", 2);

      /* name */
      GRN_BULK_REWIND(&buf);
      grn_column_name_(ctx, columns[i_column], &buf);
      grn_output_obj(ctx, outbuf, output_type, &buf, NULL);

      /* type */
      range_id = grn_obj_get_range(ctx, columns[i_column]);
      if (range_id == GRN_ID_NIL) {
        GRN_TEXT_PUTS(ctx, outbuf, "null");
      } else {
        int name_len;
        grn_obj *range_obj;
        char name_buf[GRN_TABLE_MAX_KEY_SIZE];

        range_obj = grn_ctx_at(ctx, range_id);
        name_len =
          grn_obj_name(ctx, range_obj, name_buf, GRN_TABLE_MAX_KEY_SIZE);
        GRN_BULK_REWIND(&buf);
        GRN_TEXT_PUT(ctx, &buf, name_buf, name_len);
        grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
      }

      grn_output_array_close(ctx, outbuf, output_type);
    }
    grn_output_array_close(ctx, outbuf, output_type);
  }

  for (i_hit = 0; i_hit < n_hits++; i_hit++) {
    grn_id id;

    id = grn_uvector_get_element(ctx, uvector, i_hit, NULL);
    grn_output_array_open(ctx, outbuf, output_type, "HITS", (int)n_columns);
    for (i_column = 0; i_column < n_columns; i_column++) {
      GRN_BULK_REWIND(&buf);
      grn_obj_get_value(ctx, columns[i_column], id, &buf);
      grn_output_obj(ctx, outbuf, output_type, &buf, NULL);
    }
    grn_output_array_close(ctx, outbuf, output_type);
  }

  grn_output_array_close(ctx, outbuf, output_type);

  GRN_OBJ_FIN(ctx, &buf);
}

static grn_inline void
grn_output_uvector_apache_arrow(grn_ctx *ctx, grn_obj *outbuf, grn_obj *uvector)
{
  if (!ctx->impl->output.arrow_stream_writer) {
    return;
  }

  grn_arrow_stream_writer_add_column_uvector(
    ctx,
    ctx->impl->output.arrow_stream_writer,
    uvector);
}

static grn_inline void
grn_output_uvector(grn_ctx *ctx,
                   grn_obj *outbuf,
                   grn_content_type output_type,
                   grn_obj *uvector,
                   grn_obj_format *format)
{
  bool output_result_set = false;
  bool with_weight = false;
  bool is_weight_float32 = false;

  if (format) {
    if (GRN_BULK_VSIZE(&(format->columns)) > 0) {
      output_result_set = true;
    }
    if (format->flags & GRN_OBJ_FORMAT_WITH_WEIGHT) {
      with_weight = true;
    }
    if (format->flags & GRN_OBJ_FORMAT_WEIGHT_FLOAT32) {
      is_weight_float32 = true;
    }
  }

  if (output_result_set) {
    grn_output_uvector_result_set(ctx, outbuf, output_type, uvector, format);
    return;
  }

  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    grn_output_uvector_apache_arrow(ctx, outbuf, uvector);
    return;
  }

  if (grn_type_id_is_builtin(ctx, uvector->header.domain)) {
    size_t i, n;
    char *raw_elements;
    size_t element_size;
    grn_obj element;

    raw_elements = GRN_BULK_HEAD(uvector);
    element_size = grn_type_id_size(ctx, uvector->header.domain);
    n = GRN_BULK_VSIZE(uvector) / element_size;

    grn_output_array_open(ctx, outbuf, output_type, "VECTOR", (int)n);
    GRN_OBJ_INIT(&element, GRN_BULK, 0, uvector->header.domain);
    for (i = 0; i < n; i++) {
      GRN_BULK_REWIND(&element);
      grn_bulk_write_from(ctx,
                          &element,
                          raw_elements + (element_size * i),
                          0,
                          element_size);
      grn_output_obj(ctx, outbuf, output_type, &element, NULL);
    }
    GRN_OBJ_FIN(ctx, &element);
    grn_output_array_close(ctx, outbuf, output_type);
  } else {
    grn_obj *range = grn_ctx_at(ctx, uvector->header.domain);

    uint32_t i, n;
    grn_obj id_value;
    grn_obj key_value;

    GRN_UINT32_INIT(&id_value, 0);
    GRN_OBJ_INIT(&key_value, GRN_BULK, 0, range->header.domain);

    n = grn_vector_size(ctx, uvector);
    if (with_weight) {
      grn_output_map_open(ctx, outbuf, output_type, "WEIGHT_VECTOR", (int)n);
    } else {
      grn_output_array_open(ctx, outbuf, output_type, "VECTOR", (int)n);
    }

    for (i = 0; i < n; i++) {
      grn_id id;
      float weight;

      id = grn_uvector_get_element_record(ctx, uvector, i, &weight);
      if (range->header.type == GRN_TABLE_NO_KEY) {
        GRN_UINT32_SET(ctx, &id_value, id);
        grn_output_obj(ctx, outbuf, output_type, &id_value, NULL);
      } else {
        GRN_BULK_REWIND(&key_value);
        grn_table_get_key2(ctx, range, id, &key_value);
        grn_output_obj(ctx, outbuf, output_type, &key_value, NULL);
      }

      if (with_weight) {
        if (is_weight_float32) {
          grn_output_float32(ctx, outbuf, output_type, weight);
        } else {
          grn_output_uint64(ctx, outbuf, output_type, (uint64_t)weight);
        }
      }
    }

    if (with_weight) {
      grn_output_map_close(ctx, outbuf, output_type);
    } else {
      grn_output_array_close(ctx, outbuf, output_type);
    }

    GRN_OBJ_FIN(ctx, &id_value);
    GRN_OBJ_FIN(ctx, &key_value);

    grn_obj_unref(ctx, range);
  }
}

static grn_inline void
grn_output_vector(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  grn_obj *vector,
                  grn_obj_format *format)
{
  bool with_weight = false;
  bool is_weight_float32 = false;

  if (format) {
    if (format->flags & GRN_OBJ_FORMAT_WITH_WEIGHT) {
      with_weight = true;
    }
    if (format->flags & GRN_OBJ_FORMAT_WEIGHT_FLOAT32) {
      is_weight_float32 = true;
    }
  }

  if (with_weight) {
    uint32_t i, n;
    grn_obj value;

    GRN_VOID_INIT(&value);
    n = grn_vector_size(ctx, vector);
    grn_output_map_open(ctx, outbuf, output_type, "WEIGHT_VECTOR", (int)n);
    for (i = 0; i < n; i++) {
      const char *_value;
      uint32_t weight;
      float weight_float;
      unsigned int length;
      grn_id domain;

      if (is_weight_float32) {
        length = grn_vector_get_element_float(ctx,
                                              vector,
                                              i,
                                              &_value,
                                              &weight_float,
                                              &domain);
      } else {
        length =
          grn_vector_get_element(ctx, vector, i, &_value, &weight, &domain);
      }

      if (domain != GRN_DB_VOID) {
        grn_obj_reinit(ctx, &value, domain, 0);
      } else {
        grn_obj_reinit(ctx, &value, vector->header.domain, 0);
      }
      grn_bulk_write(ctx, &value, _value, length);
      grn_output_obj(ctx, outbuf, output_type, &value, NULL);
      if (is_weight_float32) {
        grn_output_float32(ctx, outbuf, output_type, weight_float);
      } else {
        grn_output_uint64(ctx, outbuf, output_type, weight);
      }
    }
    grn_output_map_close(ctx, outbuf, output_type);
    GRN_OBJ_FIN(ctx, &value);
  } else {
    uint32_t i, n;
    grn_obj value;
    GRN_VOID_INIT(&value);
    n = grn_vector_size(ctx, vector);
    grn_output_array_open(ctx, outbuf, output_type, "VECTOR", (int)n);
    for (i = 0; i < n; i++) {
      const char *_value;
      uint32_t weight;
      unsigned int length;
      grn_id domain;

      length =
        grn_vector_get_element(ctx, vector, i, &_value, &weight, &domain);
      if (domain != GRN_DB_VOID) {
        grn_obj_reinit(ctx, &value, domain, 0);
      } else {
        grn_obj_reinit(ctx, &value, vector->header.domain, 0);
      }
      grn_bulk_write(ctx, &value, _value, length);
      grn_output_obj(ctx, outbuf, output_type, &value, NULL);
    }
    grn_output_array_close(ctx, outbuf, output_type);
    GRN_OBJ_FIN(ctx, &value);
  }
}

static grn_inline void
grn_output_pvector(grn_ctx *ctx,
                   grn_obj *outbuf,
                   grn_content_type output_type,
                   grn_obj *pvector,
                   grn_obj_format *format)
{
  if (format) {
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "cannot print GRN_PVECTOR using grn_obj_format");
  } else {
    size_t i, n;
    grn_output_array_open(ctx, outbuf, output_type, "VECTOR", -1);
    n = GRN_PTR_VECTOR_SIZE(pvector);
    for (i = 0; i < n; i++) {
      grn_obj *value;

      value = GRN_PTR_VALUE_AT(pvector, i);
      grn_output_obj(ctx, outbuf, output_type, value, NULL);
    }
    grn_output_array_close(ctx, outbuf, output_type);
  }
}

static grn_inline void
grn_output_result_set_n_hits_v1(grn_ctx *ctx,
                                grn_obj *outbuf,
                                grn_content_type output_type,
                                grn_obj_format *format)
{
  grn_output_array_open(ctx, outbuf, output_type, "NHITS", 1);
  if (output_type == GRN_CONTENT_XML) {
    grn_text_itoa(ctx, outbuf, format->nhits);
  } else {
    grn_output_int32(ctx, outbuf, output_type, format->nhits);
  }
  grn_output_array_close(ctx, outbuf, output_type);
}

static grn_inline void
grn_output_result_set_n_hits_v3(grn_ctx *ctx,
                                grn_obj *outbuf,
                                grn_content_type output_type,
                                grn_obj_format *format)
{
  grn_output_cstr(ctx, outbuf, output_type, "n_hits");
  grn_output_int32(ctx, outbuf, output_type, format->nhits);
}

static grn_inline void
grn_output_result_set_n_hits(grn_ctx *ctx,
                             grn_obj *outbuf,
                             grn_content_type output_type,
                             grn_obj_format *format)
{
  if (format->nhits == -1) {
    return;
  }

  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    grn_obj buffer;
    GRN_TEXT_INIT(&buffer, 0);
    grn_text_itoa(ctx, &buffer, format->nhits);
    GRN_TEXT_PUTC(ctx, &buffer, '\0');
    grn_arrow_stream_writer_add_metadata(ctx,
                                         ctx->impl->output.arrow_stream_writer,
                                         "GROONGA:n_hits",
                                         GRN_TEXT_VALUE(&buffer));
    GRN_OBJ_FIN(ctx, &buffer);
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_result_set_n_hits_v1(ctx, outbuf, output_type, format);
  } else {
    grn_output_result_set_n_hits_v3(ctx, outbuf, output_type, format);
  }
}

static grn_inline void
grn_output_table_column_info(grn_ctx *ctx,
                             grn_obj *outbuf,
                             grn_content_type output_type,
                             const char *name,
                             const char *type,
                             grn_obj *column)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    grn_arrow_stream_writer_add_field(ctx,
                                      ctx->impl->output.arrow_stream_writer,
                                      name,
                                      column);
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_array_open(ctx, outbuf, output_type, "COLUMN", 2);
    if (name) {
      grn_output_cstr(ctx, outbuf, output_type, name);
    } else {
      grn_output_null(ctx, outbuf, output_type);
    }
    if (type) {
      grn_output_cstr(ctx, outbuf, output_type, type);
    } else {
      grn_output_null(ctx, outbuf, output_type);
    }
    grn_output_array_close(ctx, outbuf, output_type);
  } else {
    grn_output_map_open(ctx, outbuf, output_type, "column", 2);
    grn_output_cstr(ctx, outbuf, output_type, "name");
    if (name) {
      grn_output_cstr(ctx, outbuf, output_type, name);
    } else {
      grn_output_null(ctx, outbuf, output_type);
    }
    grn_output_cstr(ctx, outbuf, output_type, "type");
    if (type) {
      grn_output_cstr(ctx, outbuf, output_type, type);
    } else {
      grn_output_null(ctx, outbuf, output_type);
    }
    grn_output_map_close(ctx, outbuf, output_type);
  }
}

static bool
is_score_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *a;

  if (obj->header.type != GRN_ACCESSOR) {
    return false;
  }

  for (a = (grn_accessor *)obj; a->next; a = a->next) {
  }
  return a->action == GRN_ACCESSOR_GET_SCORE;
}

static grn_inline void
grn_output_table_column(grn_ctx *ctx,
                        grn_obj *outbuf,
                        grn_content_type output_type,
                        grn_obj *column,
                        grn_obj *buf)
{
  grn_id range_id = GRN_ID_NIL;

  if (!column) {
    grn_output_table_column_info(ctx, outbuf, output_type, NULL, NULL, NULL);
    return;
  }

  GRN_BULK_REWIND(buf);
  grn_column_name_(ctx, column, buf);
  GRN_TEXT_PUTC(ctx, buf, '\0');

  if (column->header.type == GRN_COLUMN_INDEX) {
    range_id = GRN_DB_UINT32;
  } else if (is_score_accessor(ctx, column)) {
    if (grn_ctx_get_command_version(ctx) == GRN_COMMAND_VERSION_1) {
      range_id = GRN_DB_INT32;
    } else {
      range_id = GRN_DB_FLOAT;
    }
  }
  if (range_id == GRN_ID_NIL) {
    range_id = grn_obj_get_range(ctx, column);
  }
  if (range_id == GRN_ID_NIL) {
    grn_output_table_column_info(ctx,
                                 outbuf,
                                 output_type,
                                 GRN_TEXT_VALUE(buf),
                                 NULL,
                                 column);
  } else {
    grn_obj *range_obj;
    char type_name[GRN_TABLE_MAX_KEY_SIZE];
    int type_name_len;

    range_obj = grn_ctx_at(ctx, range_id);
    type_name_len =
      grn_obj_name(ctx, range_obj, type_name, GRN_TABLE_MAX_KEY_SIZE);
    type_name[type_name_len] = '\0';
    grn_output_table_column_info(ctx,
                                 outbuf,
                                 output_type,
                                 GRN_TEXT_VALUE(buf),
                                 type_name,
                                 column);
    grn_obj_unref(ctx, range_obj);
  }
}

static grn_inline void
grn_output_table_column_by_expression(grn_ctx *ctx,
                                      grn_obj *outbuf,
                                      grn_content_type output_type,
                                      grn_obj *expr,
                                      grn_obj *buf)
{
  grn_expr *e = (grn_expr *)expr;
  if (e->codes_curr == 1 && e->codes[0].op == GRN_OP_GET_VALUE) {
    grn_output_table_column(ctx, outbuf, output_type, e->codes[0].value, buf);
    return;
  } else if (e->codes_curr == 3 && e->codes[2].op == GRN_OP_GET_MEMBER) {
    GRN_BULK_REWIND(buf);
    grn_column_name_(ctx, e->codes[0].value, buf);
    GRN_TEXT_PUTC(ctx, buf, '[');
    grn_inspect(ctx, buf, e->codes[1].value);
    GRN_TEXT_PUTC(ctx, buf, ']');
    GRN_TEXT_PUTC(ctx, buf, '\0');

    grn_output_table_column_info(ctx,
                                 outbuf,
                                 output_type,
                                 GRN_TEXT_VALUE(buf),
                                 NULL,
                                 e->codes[0].value);
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_obj *column = NULL;
    if (e->codes_curr > 0) {
      column = e->codes[0].value;
    }
    grn_output_table_column(ctx, outbuf, output_type, column, buf);
    return;
  }

  grn_id range_id = grn_obj_get_range(ctx, expr);
  grn_obj *range_obj = NULL;
  char type_name[GRN_TABLE_MAX_KEY_SIZE];
  int type_name_len = 0;
  grn_obj name_buffer;
  GRN_TEXT_INIT(&name_buffer, 0);
  const char *name = NULL;

  if (range_id != GRN_ID_NIL) {
    range_obj = grn_ctx_at(ctx, range_id);
    type_name_len =
      grn_obj_name(ctx, range_obj, type_name, GRN_TABLE_MAX_KEY_SIZE);
    type_name[type_name_len] = '\0';
  }
  grn_expr_to_script_syntax(ctx, expr, &name_buffer);
  if (GRN_TEXT_LEN(&name_buffer) > 0) {
    GRN_TEXT_PUTC(ctx, &name_buffer, '\0');
    name = GRN_TEXT_VALUE(&name_buffer);
  }
  grn_output_table_column_info(ctx,
                               outbuf,
                               output_type,
                               name,
                               type_name_len > 0 ? type_name : NULL,
                               expr);
  if (range_obj) {
    grn_obj_unref(ctx, range_obj);
  }
  GRN_OBJ_FIN(ctx, &name_buffer);
}

static grn_inline void
grn_output_table_columns_open(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              int n_columns)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_array_open(ctx, outbuf, output_type, "COLUMNS", n_columns);
  } else {
    grn_output_cstr(ctx, outbuf, output_type, "columns");
    grn_output_array_open(ctx, outbuf, output_type, "columns", n_columns);
  }
}

static grn_inline void
grn_output_table_columns_close(grn_ctx *ctx,
                               grn_obj *outbuf,
                               grn_content_type output_type)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    grn_arrow_stream_writer_write_schema(ctx,
                                         ctx->impl->output.arrow_stream_writer);
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_array_close(ctx, outbuf, output_type);
  } else {
    grn_output_array_close(ctx, outbuf, output_type);
  }
}

static grn_inline void
grn_output_table_columns_by_expression(grn_ctx *ctx,
                                       grn_obj *outbuf,
                                       grn_content_type output_type,
                                       grn_obj *table,
                                       grn_obj_format *format,
                                       grn_obj *buf)
{
  grn_expr *expr = (grn_expr *)format->expression;
  grn_obj offsets;
  size_t i, n;

  GRN_UINT32_INIT(&offsets, GRN_OBJ_VECTOR);
  grn_output_columns_get_offsets(ctx, format->expression, &offsets);

  n = GRN_UINT32_VECTOR_SIZE(&offsets) / 2;

  grn_output_table_columns_open(ctx, outbuf, output_type, (int)n);

  grn_expr_code *codes = expr->codes;
  uint32_t codes_curr = expr->codes_curr;
  for (i = 0; i < n; i++) {
    uint32_t code_start_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2);
    uint32_t code_end_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2 + 1);
    expr->codes = codes + code_start_offset;
    expr->codes_curr = code_end_offset - code_start_offset;
    grn_output_table_column_by_expression(ctx,
                                          outbuf,
                                          output_type,
                                          (grn_obj *)expr,
                                          buf);
  }
  expr->codes = codes;
  expr->codes_curr = codes_curr;

  grn_output_table_columns_close(ctx, outbuf, output_type);

  GRN_OBJ_FIN(ctx, &offsets);
}

static grn_inline void
grn_output_table_columns_by_columns(grn_ctx *ctx,
                                    grn_obj *outbuf,
                                    grn_content_type output_type,
                                    grn_obj *table,
                                    grn_obj_format *format,
                                    grn_obj *buf)
{
  size_t i;
  size_t ncolumns = GRN_PTR_VECTOR_SIZE(&format->columns);
  grn_obj **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);

  grn_output_table_columns_open(ctx, outbuf, output_type, (int)ncolumns);
  for (i = 0; i < ncolumns; i++) {
    grn_output_table_column(ctx, outbuf, output_type, columns[i], buf);
  }
  grn_output_table_columns_close(ctx, outbuf, output_type);
}

void
grn_output_table_columns(grn_ctx *ctx,
                         grn_obj *outbuf,
                         grn_content_type output_type,
                         grn_obj *table,
                         grn_obj_format *format)
{
  grn_obj buf;

  GRN_TEXT_INIT(&buf, 0);
  if (format->expression) {
    grn_output_table_columns_by_expression(ctx,
                                           outbuf,
                                           output_type,
                                           table,
                                           format,
                                           &buf);
  } else {
    grn_output_table_columns_by_columns(ctx,
                                        outbuf,
                                        output_type,
                                        table,
                                        format,
                                        &buf);
  }
  GRN_OBJ_FIN(ctx, &buf);
}

static grn_inline void
grn_output_table_record_open(grn_ctx *ctx,
                             grn_obj *outbuf,
                             grn_content_type output_type,
                             int n_columns)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    grn_arrow_stream_writer_open_record(ctx,
                                        ctx->impl->output.arrow_stream_writer);
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_array_open(ctx, outbuf, output_type, "HIT", n_columns);
  } else {
    grn_output_array_open(ctx, outbuf, output_type, "record", n_columns);
  }
}

static grn_inline void
grn_output_table_record_close(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    grn_arrow_stream_writer_close_record(ctx,
                                         ctx->impl->output.arrow_stream_writer);
    return;
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_array_close(ctx, outbuf, output_type);
  } else {
    grn_output_array_close(ctx, outbuf, output_type);
  }
}

static grn_inline void
grn_output_table_record_by_column(grn_ctx *ctx,
                                  grn_obj *outbuf,
                                  grn_content_type output_type,
                                  grn_obj *column,
                                  grn_id id,
                                  grn_output_table_data *data)
{
  grn_output_table_column_value(ctx, outbuf, output_type, column, id, data);
}

static grn_inline void
grn_output_table_record_by_expression(grn_ctx *ctx,
                                      grn_obj *outbuf,
                                      grn_content_type output_type,
                                      grn_obj *expression,
                                      uint32_t code_start_offset,
                                      uint32_t code_end_offset,
                                      grn_obj *record,
                                      grn_output_table_data *data)
{
  grn_expr *expr = (grn_expr *)expression;
  grn_expr_code *codes = expr->codes;
  uint32_t codes_curr = expr->codes_curr;

  expr->codes += code_start_offset;
  expr->codes_curr = code_end_offset - code_start_offset;

  if (expr->codes_curr == 1 && expr->codes[0].op == GRN_OP_GET_VALUE) {
    grn_obj *column = expr->codes[0].value;
    grn_output_table_record_by_column(ctx,
                                      outbuf,
                                      output_type,
                                      column,
                                      GRN_RECORD_VALUE(record),
                                      data);
  } else {
    grn_obj *result;
    result = grn_expr_exec(ctx, expression, 0);
    if (result) {
      grn_output_obj(ctx, outbuf, output_type, result, NULL);
    } else {
      grn_output_cstr(ctx, outbuf, output_type, ctx->errbuf);
    }
  }

  expr->codes = codes;
  expr->codes_curr = codes_curr;
}

static grn_inline void
grn_output_table_records_by_expression(grn_ctx *ctx,
                                       grn_obj *outbuf,
                                       grn_content_type output_type,
                                       grn_table_cursor *tc,
                                       grn_obj_format *format,
                                       grn_output_table_data *data)
{
  grn_obj offsets;
  size_t n;
  grn_id id;
  grn_obj *record;

  GRN_UINT32_INIT(&offsets, GRN_OBJ_VECTOR);
  grn_output_columns_get_offsets(ctx, format->expression, &offsets);
  n = GRN_UINT32_VECTOR_SIZE(&offsets) / 2;
  record = grn_expr_get_var_by_offset(ctx, format->expression, 0);
  while ((id = grn_table_cursor_next(ctx, tc)) != GRN_ID_NIL) {
    grn_output_table_data_open_record(ctx, data);
    GRN_RECORD_SET(ctx, record, id);
    grn_output_table_record_open(ctx, outbuf, output_type, (int)n);
    size_t i;
    for (i = 0; i < n; i++) {
      uint32_t code_start_offset;
      uint32_t code_end_offset;

      code_start_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2);
      code_end_offset = GRN_UINT32_VALUE_AT(&offsets, i * 2 + 1);
      grn_output_table_record_by_expression(ctx,
                                            outbuf,
                                            output_type,
                                            format->expression,
                                            code_start_offset,
                                            code_end_offset,
                                            record,
                                            data);
    }
    grn_output_table_record_close(ctx, outbuf, output_type);
    grn_output_table_data_close_record(ctx, data);
  }

  GRN_OBJ_FIN(ctx, &offsets);
}

static grn_inline void
grn_output_table_records_by_columns(grn_ctx *ctx,
                                    grn_obj *outbuf,
                                    grn_content_type output_type,
                                    grn_table_cursor *tc,
                                    grn_obj_format *format,
                                    grn_output_table_data *data)
{
  size_t i;
  grn_id id;
  size_t ncolumns = GRN_PTR_VECTOR_SIZE(&format->columns);
  grn_obj **columns = (grn_obj **)GRN_BULK_HEAD(&format->columns);
  while (ctx->rc == GRN_SUCCESS &&
         (id = grn_table_cursor_next(ctx, tc)) != GRN_ID_NIL) {
    grn_output_table_data_open_record(ctx, data);
    grn_output_table_record_open(ctx, outbuf, output_type, (int)ncolumns);
    for (i = 0; i < ncolumns; i++) {
      grn_output_table_record_by_column(ctx,
                                        outbuf,
                                        output_type,
                                        columns[i],
                                        id,
                                        data);
    }
    grn_output_table_record_close(ctx, outbuf, output_type);
    grn_output_table_data_close_record(ctx, data);
  }
}

void
grn_output_table_records_open(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              int n_records)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    return;
  }

  if (grn_ctx_get_command_version(ctx) >= GRN_COMMAND_VERSION_3) {
    grn_output_cstr(ctx, outbuf, output_type, "records");
    grn_output_array_open(ctx, outbuf, output_type, "records", n_records);
  }
}

void
grn_output_table_records_close(grn_ctx *ctx,
                               grn_obj *outbuf,
                               grn_content_type output_type)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    return;
  }

  if (grn_ctx_get_command_version(ctx) >= GRN_COMMAND_VERSION_3) {
    grn_output_array_close(ctx, outbuf, output_type);
  }
}

void
grn_output_table_records_content(grn_ctx *ctx,
                                 grn_obj *outbuf,
                                 grn_content_type output_type,
                                 grn_obj *table,
                                 grn_obj_format *format)
{

  if (format) {
    grn_table_cursor *tc = grn_table_cursor_open(ctx,
                                                 table,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 format->offset,
                                                 format->limit,
                                                 GRN_CURSOR_ASCENDING);
    if (!tc) {
      ERRCLR(ctx);
      return;
    }
    grn_output_table_data data;
    grn_output_table_data_init(ctx, &data, format);
    if (format->expression) {
      grn_output_table_records_by_expression(ctx,
                                             outbuf,
                                             output_type,
                                             tc,
                                             format,
                                             &data);
    } else {
      grn_output_table_records_by_columns(ctx,
                                          outbuf,
                                          output_type,
                                          tc,
                                          format,
                                          &data);
    }
    grn_output_table_data_fin(ctx, &data);
    grn_table_cursor_close(ctx, tc);
  } else {
    grn_obj *column =
      grn_obj_column(ctx, table, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN);
    grn_obj buf;
    GRN_TEXT_INIT(&buf, 0);
    GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id)
    {
      GRN_BULK_REWIND(&buf);
      grn_obj_get_value(ctx, column, id, &buf);
      grn_text_esc(ctx, outbuf, GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf));
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &buf);
    grn_obj_unlink(ctx, column);
  }
}

void
grn_output_table_records(grn_ctx *ctx,
                         grn_obj *outbuf,
                         grn_content_type output_type,
                         grn_obj *table,
                         grn_obj_format *format)
{
  grn_output_table_records_open(ctx, outbuf, output_type, format->limit);
  grn_output_table_records_content(ctx, outbuf, output_type, table, format);
  grn_output_table_records_close(ctx, outbuf, output_type);
}

static void
grn_output_result_set_open_v1(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              grn_obj *table,
                              grn_obj_format *format,
                              uint32_t n_additional_elements)
{
  if (format) {
    int resultset_size = 1;
    /* resultset: [NHITS, (COLUMNS), (HITS)] */
    if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
      resultset_size++;
    }
    resultset_size += format->limit;
    resultset_size += (int)n_additional_elements;
    grn_output_array_open(ctx,
                          outbuf,
                          output_type,
                          "RESULTSET",
                          resultset_size);
    grn_output_result_set_n_hits(ctx, outbuf, output_type, format);
    if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
      grn_output_table_columns(ctx, outbuf, output_type, table, format);
    }
    grn_output_table_records_open(ctx, outbuf, output_type, format->limit);
  } else {
    grn_output_array_open(ctx, outbuf, output_type, "HIT", -1);
  }
  grn_output_table_records_content(ctx, outbuf, output_type, table, format);
}

static void
grn_output_result_set_close_v1(grn_ctx *ctx,
                               grn_obj *outbuf,
                               grn_content_type output_type,
                               grn_obj *table,
                               grn_obj_format *format)
{
  grn_output_array_close(ctx, outbuf, output_type);
}

static void
grn_output_result_set_open_v3(grn_ctx *ctx,
                              grn_obj *outbuf,
                              grn_content_type output_type,
                              grn_obj *result_set,
                              grn_obj_format *format,
                              uint32_t n_additional_elements)
{
  grn_obj buf;
  GRN_TEXT_INIT(&buf, 0);
  if (format) {
    int n_elements = 2;
    /* result_set: {"n_hits": N, ("columns": COLUMNS,) "records": records} */
    if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
      n_elements++;
    }
    n_elements += (int)n_additional_elements;
    grn_output_map_open(ctx, outbuf, output_type, "result_set", n_elements);
    grn_output_result_set_n_hits(ctx, outbuf, output_type, format);
    if (format->flags & GRN_OBJ_FORMAT_WITH_COLUMN_NAMES) {
      grn_output_table_columns(ctx, outbuf, output_type, result_set, format);
    }
    grn_output_table_records(ctx, outbuf, output_type, result_set, format);
  } else {
    grn_obj *column;
    unsigned int n_records;
    int n_elements = 1;

    column = grn_obj_column(ctx,
                            result_set,
                            GRN_COLUMN_NAME_KEY,
                            GRN_COLUMN_NAME_KEY_LEN);
    n_elements += (int)n_additional_elements;
    grn_output_map_open(ctx, outbuf, output_type, "result_set", n_elements);
    n_records = grn_table_size(ctx, result_set);
    grn_output_cstr(ctx, outbuf, output_type, "keys");
    grn_output_array_open(ctx, outbuf, output_type, "keys", (int)n_records);
    GRN_TABLE_EACH_BEGIN(ctx, result_set, cursor, id)
    {
      GRN_BULK_REWIND(&buf);
      grn_obj_get_value(ctx, column, id, &buf);
      grn_text_esc(ctx, outbuf, GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf));
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    grn_output_array_close(ctx, outbuf, output_type);
    grn_obj_unlink(ctx, column);
  }
  GRN_OBJ_FIN(ctx, &buf);
}

static void
grn_output_result_set_close_v3(grn_ctx *ctx,
                               grn_obj *outbuf,
                               grn_content_type output_type,
                               grn_obj *result_set,
                               grn_obj_format *format)
{
  grn_output_map_close(ctx, outbuf, output_type);
}

void
grn_output_result_set_open(grn_ctx *ctx,
                           grn_obj *outbuf,
                           grn_content_type output_type,
                           grn_obj *result_set,
                           grn_obj_format *format,
                           uint32_t n_additional_elements)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_close(ctx, ctx->impl->output.arrow_stream_writer);
    }
    ctx->impl->output.arrow_stream_writer =
      grn_arrow_stream_writer_open(ctx, outbuf);
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_result_set_open_v1(ctx,
                                  outbuf,
                                  output_type,
                                  result_set,
                                  format,
                                  n_additional_elements);
  } else {
    grn_output_result_set_open_v3(ctx,
                                  outbuf,
                                  output_type,
                                  result_set,
                                  format,
                                  n_additional_elements);
  }
}

void
grn_output_result_set_close(grn_ctx *ctx,
                            grn_obj *outbuf,
                            grn_content_type output_type,
                            grn_obj *result_set,
                            grn_obj_format *format)
{
  if (output_type == GRN_CONTENT_APACHE_ARROW) {
    if (ctx->impl->output.arrow_stream_writer) {
      grn_arrow_stream_writer_close(ctx, ctx->impl->output.arrow_stream_writer);
      ctx->impl->output.arrow_stream_writer = NULL;
    }
  }

  if (grn_ctx_get_command_version(ctx) < GRN_COMMAND_VERSION_3) {
    grn_output_result_set_close_v1(ctx,
                                   outbuf,
                                   output_type,
                                   result_set,
                                   format);
  } else {
    grn_output_result_set_close_v3(ctx,
                                   outbuf,
                                   output_type,
                                   result_set,
                                   format);
  }
}

void
grn_output_result_set(grn_ctx *ctx,
                      grn_obj *outbuf,
                      grn_content_type output_type,
                      grn_obj *result_set,
                      grn_obj_format *format)
{
  uint32_t n_additional_elements = 0;

  grn_output_result_set_open(ctx,
                             outbuf,
                             output_type,
                             result_set,
                             format,
                             n_additional_elements);
  grn_output_result_set_close(ctx, outbuf, output_type, result_set, format);
}

static void
grn_output_table_hash_key(grn_ctx *ctx,
                          grn_obj *outbuf,
                          grn_content_type output_type,
                          grn_obj *obj,
                          grn_obj_format *format)
{
  const unsigned int n_elements = grn_table_size(ctx, obj);
  grn_obj key;
  GRN_OBJ_INIT(&key, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY, obj->header.domain);
  grn_output_map_open(ctx, outbuf, output_type, "object", (int)n_elements);
  GRN_TABLE_EACH_BEGIN(ctx, obj, cursor, id)
  {
    void *raw_key;
    const int key_size = grn_table_cursor_get_key(ctx, cursor, &raw_key);
    GRN_TEXT_SET(ctx, &key, raw_key, key_size);
    grn_output_obj(ctx, outbuf, output_type, &key, format);
    void *raw_value;
    const int value_size = grn_table_cursor_get_value(ctx, cursor, &raw_value);
    if (value_size == 0) {
      grn_output_null(ctx, outbuf, output_type);
    } else {
      grn_obj *value = raw_value;
      grn_output_obj(ctx, outbuf, output_type, value, format);
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  grn_output_map_close(ctx, outbuf, output_type);
}

void
grn_output_obj(grn_ctx *ctx,
               grn_obj *outbuf,
               grn_content_type output_type,
               grn_obj *obj,
               grn_obj_format *format)
{
  grn_obj buf;
  GRN_TEXT_INIT(&buf, 0);
  switch (obj->header.type) {
  case GRN_VOID:
    grn_output_void(ctx, outbuf, output_type, obj, format);
    break;
  case GRN_BULK:
    grn_output_bulk(ctx, outbuf, output_type, obj, format);
    break;
  case GRN_UVECTOR:
    grn_output_uvector(ctx, outbuf, output_type, obj, format);
    break;
  case GRN_VECTOR:
    grn_output_vector(ctx, outbuf, output_type, obj, format);
    break;
  case GRN_PVECTOR:
    grn_output_pvector(ctx, outbuf, output_type, obj, format);
    break;
  case GRN_TABLE_HASH_KEY:
    if (obj->header.flags & GRN_HASH_TINY) {
      grn_output_table_hash_key(ctx, outbuf, output_type, obj, format);
    } else {
      /* Deprecated. Use grn_output_result_set() directly. */
      grn_output_result_set(ctx, outbuf, output_type, obj, format);
    }
    break;
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    /* Deprecated. Use grn_output_result_set() directly. */
    grn_output_result_set(ctx, outbuf, output_type, obj, format);
    break;
  }
  GRN_OBJ_FIN(ctx, &buf);
}

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
  int len;
  int offset = 0, limit = 0, record_n = 0;
  int column_n = 0, column_text_n = 0, result_set_n = -1;
  bool in_vector = false;
  unsigned int vector_element_n = 0;
  bool in_weight_vector = false;
  unsigned int weight_vector_item_n = 0;

  s = GRN_TEXT_VALUE(output);
  e = GRN_BULK_CURR(output);
  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_INIT(&name, 0);
  GRN_TEXT_INIT(&columns, 0);

  expr = ctx->impl->curr_expr;

#define EQUAL_NAME_P(_name)                                                    \
  (GRN_TEXT_LEN(&name) == strlen(_name) &&                                     \
   !memcmp(GRN_TEXT_VALUE(&name), _name, strlen(_name)))

  while (s < e) {
    switch (*s) {
    case '<':
      s++;
      switch (*s) {
      case '/':
        status = XML_END_ELEMENT;
        s++;
        break;
      default:
        status = XML_START_ELEMENT;
        break;
      }
      GRN_BULK_REWIND(&name);
      break;
    case '>':
      switch (status) {
      case XML_START_ELEMENT:
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
          in_vector = true;
          vector_element_n = 0;
          GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
          GRN_TEXT_PUTS(ctx, transformed, c);
          GRN_TEXT_PUTS(ctx, transformed, "\">");
        } else if (EQUAL_NAME_P("WEIGHT_VECTOR")) {
          char *c = transform_xml_next_column(&columns, column_n++);
          in_weight_vector = true;
          weight_vector_item_n = 0;
          GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
          GRN_TEXT_PUTS(ctx, transformed, c);
          GRN_TEXT_PUTS(ctx, transformed, "\">");
        }
        break;
      case XML_END_ELEMENT:
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
            GRN_TEXT_PUTS(ctx,
                          transformed,
                          "</NAVIGATIONELEMENTS>\n"
                          "</NAVIGATIONENTRY>\n");
          }
        } else if (EQUAL_NAME_P("RESULT")) {
          GRN_TEXT_PUTS(ctx,
                        transformed,
                        "</RESULTPAGE>\n"
                        "</SEGMENT>\n"
                        "</SEGMENTS>\n");
        } else if (EQUAL_NAME_P("VECTOR")) {
          in_vector = false;
          GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
        } else if (EQUAL_NAME_P("WEIGHT_VECTOR")) {
          in_weight_vector = false;
          GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
        } else {
          switch (place) {
          case XML_PLACE_HIT:
            if (result_set_n == 0) {
              if (in_vector) {
                if (vector_element_n > 0) {
                  GRN_TEXT_PUTS(ctx, transformed, ", ");
                }
                GRN_TEXT_PUT(ctx,
                             transformed,
                             GRN_TEXT_VALUE(&buf),
                             GRN_TEXT_LEN(&buf));
                vector_element_n++;
              } else if (in_weight_vector) {
                bool is_key = ((weight_vector_item_n % 2) == 0);
                if (is_key) {
                  unsigned int weight_vector_key_n;
                  weight_vector_key_n = weight_vector_item_n / 2;
                  if (weight_vector_key_n > 0) {
                    GRN_TEXT_PUTS(ctx, transformed, ", ");
                  }
                } else {
                  GRN_TEXT_PUTS(ctx, transformed, ":");
                }
                GRN_TEXT_PUT(ctx,
                             transformed,
                             GRN_TEXT_VALUE(&buf),
                             GRN_TEXT_LEN(&buf));
                weight_vector_item_n++;
              } else {
                char *c = transform_xml_next_column(&columns, column_n++);
                GRN_TEXT_PUTS(ctx, transformed, "<FIELD NAME=\"");
                GRN_TEXT_PUTS(ctx, transformed, c);
                GRN_TEXT_PUTS(ctx, transformed, "\">");
                GRN_TEXT_PUT(ctx,
                             transformed,
                             GRN_TEXT_VALUE(&buf),
                             GRN_TEXT_LEN(&buf));
                GRN_TEXT_PUTS(ctx, transformed, "</FIELD>\n");
              }
            } else {
              char *c = transform_xml_next_column(&columns, column_n++);
              GRN_TEXT_PUTS(ctx, transformed, c);
              GRN_TEXT_PUTS(ctx, transformed, "=\"");
              GRN_TEXT_PUT(ctx,
                           transformed,
                           GRN_TEXT_VALUE(&buf),
                           GRN_TEXT_LEN(&buf));
              GRN_TEXT_PUTS(ctx, transformed, "\" ");
            }
            break;
          default:
            if (EQUAL_NAME_P("NHITS")) {
              if (result_set_n == 0) {
                uint32_t nhits;
                grn_obj *offset_value, *limit_value;

                nhits =
                  grn_atoui(GRN_TEXT_VALUE(&buf), GRN_BULK_CURR(&buf), NULL);
                offset_value =
                  grn_expr_get_var(ctx, expr, "offset", strlen("offset"));
                limit_value =
                  grn_expr_get_var(ctx, expr, "limit", strlen("limit"));
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
                grn_output_range_normalize(ctx, (int)nhits, &offset, &limit);
                record_n = offset + 1;
                GRN_TEXT_PUTS(ctx,
                              transformed,
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
                GRN_TEXT_PUTS(ctx, transformed, "<NAVIGATIONELEMENTS COUNT=\"");
                GRN_TEXT_PUT(ctx,
                             transformed,
                             GRN_TEXT_VALUE(&buf),
                             GRN_TEXT_LEN(&buf));
                GRN_TEXT_PUTS(ctx, transformed, "\">\n");
              }
            } else if (EQUAL_NAME_P("TEXT")) {
              switch (place) {
              case XML_PLACE_COLUMN:
                if (column_text_n == 0) {
                  GRN_TEXT_PUT(ctx,
                               &columns,
                               GRN_TEXT_VALUE(&buf),
                               GRN_TEXT_LEN(&buf));
                  GRN_TEXT_PUTC(ctx, &columns, '\0');
                }
                column_text_n++;
                break;
              default:
                break;
              }
            }
          }
        }
      default:
        break;
      }
      s++;
      GRN_BULK_REWIND(&buf);
      status = XML_TEXT;
      break;
    default:
      len = grn_charlen(ctx, s, e);
      switch (status) {
      case XML_START_ELEMENT:
      case XML_END_ELEMENT:
        GRN_TEXT_PUT(ctx, &name, s, len);
        break;
      default:
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

#ifdef GRN_WITH_MESSAGE_PACK
typedef struct {
  grn_ctx *ctx;
  grn_obj *buffer;
} msgpack_writer_ctx;

static grn_inline int
msgpack_buffer_writer(void *data, const char *buf, msgpack_size_t len)
{
  msgpack_writer_ctx *writer_ctx = (msgpack_writer_ctx *)data;
  return grn_bulk_write(writer_ctx->ctx, writer_ctx->buffer, buf, len);
}
#endif

#define JSON_CALLBACK_PARAM "callback"

static void
grn_output_envelope_open_json_v1(grn_ctx *ctx,
                                 grn_obj *head,
                                 bool is_stream_mode,
                                 bool have_body)
{
  size_t indent_level = 0;

  json_array_open(ctx, head, &indent_level);
}

static void
grn_output_envelope_close_json_v1(grn_ctx *ctx,
                                  grn_obj *head,
                                  grn_obj *foot,
                                  bool is_stream_mode,
                                  bool have_body,
                                  grn_rc rc,
                                  double started,
                                  double elapsed,
                                  const char *file,
                                  int line)
{
  size_t indent_level = 0;
  if (ctx->impl->output.is_pretty) {
    indent_level++;
  }

  {
    if (is_stream_mode && have_body) {
      json_element_end(ctx, head, indent_level);
    }

    json_array_open(ctx, head, &indent_level);
    {
      grn_text_itoa(ctx, head, rc);

      json_element_end(ctx, head, indent_level);
      grn_text_ftoa(ctx, head, started);

      json_element_end(ctx, head, indent_level);
      grn_text_ftoa(ctx, head, elapsed);

      if (rc != GRN_SUCCESS) {
        json_element_end(ctx, head, indent_level);
        grn_text_esc(ctx, head, ctx->errbuf, strlen(ctx->errbuf));

        if (ctx->errfunc && ctx->errfile) {
          grn_obj *command;

          json_element_end(ctx, head, indent_level);
          json_array_open(ctx, head, &indent_level);
          {
            json_array_open(ctx, head, &indent_level);
            {
              grn_text_esc(ctx, head, ctx->errfunc, strlen(ctx->errfunc));

              json_element_end(ctx, head, indent_level);
              grn_text_esc(ctx, head, ctx->errfile, strlen(ctx->errfile));

              json_element_end(ctx, head, indent_level);
              grn_text_ulltoa(ctx, head, ctx->errline);
            }
            json_array_close(ctx, head, &indent_level);

            if (file && (command = GRN_CTX_USER_DATA(ctx)->ptr)) {
              json_element_end(ctx, head, indent_level);
              json_array_open(ctx, head, &indent_level);
              {
                grn_text_esc(ctx, head, file, strlen(file));

                json_element_end(ctx, head, indent_level);
                grn_text_itoa(ctx, head, line);

                json_element_end(ctx, head, indent_level);
                grn_text_esc(ctx,
                             head,
                             GRN_TEXT_VALUE(command),
                             GRN_TEXT_LEN(command));
              }
              json_array_close(ctx, head, &indent_level);
            }
          }
          json_array_close(ctx, head, &indent_level);
        }
      }
    }
    json_array_close(ctx, head, &indent_level);
  }

  if (!is_stream_mode && have_body) {
    json_element_end(ctx, head, indent_level);
  }

  json_array_close(ctx, foot, &indent_level);
}

static void
grn_output_envelope_json_v1(grn_ctx *ctx,
                            grn_obj *head,
                            grn_obj *body,
                            grn_obj *foot,
                            grn_rc rc,
                            double started,
                            double elapsed,
                            const char *file,
                            int line)
{
  bool is_stream_mode = false;
  bool have_body = GRN_TEXT_LEN(body) > 0;
  grn_output_envelope_open_json_v1(ctx, head, is_stream_mode, have_body);
  grn_output_envelope_close_json_v1(ctx,
                                    head,
                                    foot,
                                    is_stream_mode,
                                    have_body,
                                    rc,
                                    started,
                                    elapsed,
                                    file,
                                    line);
}

static void
grn_output_envelope_open_json(grn_ctx *ctx,
                              grn_obj *head,
                              bool is_stream_mode,
                              bool have_body)
{
  size_t indent_level = 0;

  json_map_open(ctx, head, &indent_level);
  {
    if (is_stream_mode && have_body) {
      json_key(ctx, head, "body");
    }
  }
}

static void
grn_output_envelope_close_json_header(grn_ctx *ctx,
                                      grn_obj *head,
                                      grn_rc rc,
                                      double started,
                                      double elapsed,
                                      const char *file,
                                      int line,
                                      size_t *indent_level)
{
  json_key(ctx, head, "header");
  json_map_open(ctx, head, indent_level);
  {
    json_key(ctx, head, "return_code");
    grn_text_itoa(ctx, head, rc);

    json_value_end(ctx, head, *indent_level);
    json_key(ctx, head, "start_time");
    grn_text_ftoa(ctx, head, started);

    json_value_end(ctx, head, *indent_level);
    json_key(ctx, head, "elapsed_time");
    grn_text_ftoa(ctx, head, elapsed);

    if (rc != GRN_SUCCESS) {
      json_value_end(ctx, head, *indent_level);
      json_key(ctx, head, "error");
      json_map_open(ctx, head, indent_level);
      {
        json_key(ctx, head, "message");
        grn_text_esc(ctx, head, ctx->errbuf, strlen(ctx->errbuf));

        if (ctx->errfunc && ctx->errfile) {
          json_value_end(ctx, head, *indent_level);
          json_key(ctx, head, "function");
          grn_text_esc(ctx, head, ctx->errfunc, strlen(ctx->errfunc));

          json_value_end(ctx, head, *indent_level);
          json_key(ctx, head, "file");
          grn_text_esc(ctx, head, ctx->errfile, strlen(ctx->errfile));

          json_value_end(ctx, head, *indent_level);
          json_key(ctx, head, "line");
          grn_text_ulltoa(ctx, head, ctx->errline);
        }

        if (file) {
          grn_obj *command;

          command = GRN_CTX_USER_DATA(ctx)->ptr;
          if (command) {
            json_value_end(ctx, head, *indent_level);
            json_key(ctx, head, "input");
            json_map_open(ctx, head, indent_level);
            {
              json_key(ctx, head, "file");
              grn_text_esc(ctx, head, file, strlen(file));

              json_value_end(ctx, head, *indent_level);
              json_key(ctx, head, "line");
              grn_text_itoa(ctx, head, line);

              json_value_end(ctx, head, *indent_level);
              json_key(ctx, head, "command");
              grn_text_esc(ctx,
                           head,
                           GRN_TEXT_VALUE(command),
                           GRN_TEXT_LEN(command));
            }
            json_map_close(ctx, head, indent_level);
          }
        }
      }
      json_map_close(ctx, head, indent_level);
    }
  }
  json_map_close(ctx, head, indent_level);
}

static void
grn_output_envelope_close_json_trace_log(grn_ctx *ctx,
                                         grn_obj *head,
                                         size_t *indent_level)
{
  json_value_end(ctx, head, *indent_level);
  json_key(ctx, head, "trace_log");
  json_map_open(ctx, head, indent_level);
  {
    json_key(ctx, head, "columns");
    json_array_open(ctx, head, indent_level);
    {
      json_map_open(ctx, head, indent_level);
      {
        json_key(ctx, head, "name");
        grn_text_esc(ctx, head, "depth", strlen("depth"));
      }
      json_map_close(ctx, head, indent_level);

      json_element_end(ctx, head, *indent_level);
      json_map_open(ctx, head, indent_level);
      {
        json_key(ctx, head, "name");
        grn_text_esc(ctx, head, "sequence", strlen("sequence"));
      }
      json_map_close(ctx, head, indent_level);

      json_element_end(ctx, head, *indent_level);
      json_map_open(ctx, head, indent_level);
      {
        json_key(ctx, head, "name");
        grn_text_esc(ctx, head, "name", strlen("name"));
      }
      json_map_close(ctx, head, indent_level);

      json_element_end(ctx, head, *indent_level);
      json_map_open(ctx, head, indent_level);
      {
        json_key(ctx, head, "name");
        grn_text_esc(ctx, head, "value", strlen("value"));
      }
      json_map_close(ctx, head, indent_level);

      json_element_end(ctx, head, *indent_level);
      json_map_open(ctx, head, indent_level);
      {
        json_key(ctx, head, "name");
        grn_text_esc(ctx, head, "elapsed_time", strlen("elapsed_time"));
      }
      json_map_close(ctx, head, indent_level);
    }
    json_array_close(ctx, head, indent_level);

    json_value_end(ctx, head, *indent_level);
    json_key(ctx, head, "logs");
    json_array_open(ctx, head, indent_level);
    {
      grn_obj *depths = &(ctx->impl->trace_log.depths);
      grn_obj *sequences = &(ctx->impl->trace_log.sequences);
      grn_obj *names = &(ctx->impl->trace_log.names);
      grn_obj *values = &(ctx->impl->trace_log.values);
      grn_obj *elapsed_times = &(ctx->impl->trace_log.elapsed_times);
      uint32_t n_logs = grn_vector_size(ctx, names);
      uint32_t i;
      for (i = 0; i < n_logs; i++) {
        if (i > 0) {
          json_element_end(ctx, head, *indent_level);
        }
        json_array_open(ctx, head, indent_level);
        {
          uint16_t depth = GRN_UINT16_VALUE_AT(depths, i);
          grn_text_ulltoa(ctx, head, depth);

          json_element_end(ctx, head, *indent_level);
          uint16_t sequence = GRN_UINT16_VALUE_AT(sequences, i);
          grn_text_ulltoa(ctx, head, sequence);

          json_element_end(ctx, head, *indent_level);
          const char *name;
          uint32_t name_size =
            grn_vector_get_element_float(ctx, names, i, &name, NULL, NULL);
          grn_text_esc(ctx, head, name, name_size);

          json_element_end(ctx, head, *indent_level);
          const char *value;
          grn_id value_domain;
          uint32_t value_size = grn_vector_get_element_float(ctx,
                                                             values,
                                                             i,
                                                             &value,
                                                             NULL,
                                                             &value_domain);
          switch (value_domain) {
          case GRN_DB_UINT32:
            {
              uint32_t value_uint32 =
                (value_size == 0) ? 0 : *((const uint32_t *)value);
              grn_text_ulltoa(ctx, head, value_uint32);
            }
            break;
          default:
            grn_text_esc(ctx, head, value, value_size);
            break;
          }

          json_element_end(ctx, head, *indent_level);
          uint64_t elapsed_time = GRN_UINT64_VALUE_AT(elapsed_times, i);
          grn_text_ulltoa(ctx, head, elapsed_time);
        }
        json_array_close(ctx, head, indent_level);
      }
    }
    json_array_close(ctx, head, indent_level);
  }
  json_map_close(ctx, head, indent_level);
}

static void
grn_output_envelope_close_json(grn_ctx *ctx,
                               grn_obj *head,
                               grn_obj *foot,
                               bool is_stream_mode,
                               bool have_body,
                               grn_rc rc,
                               double started,
                               double elapsed,
                               const char *file,
                               int line)
{
  size_t indent_level = 0;
  if (ctx->impl->output.is_pretty) {
    indent_level++;
  }

  {
    if (is_stream_mode && have_body) {
      json_value_end(ctx, head, indent_level);
    }

    grn_output_envelope_close_json_header(ctx,
                                          head,
                                          rc,
                                          started,
                                          elapsed,
                                          file,
                                          line,
                                          &indent_level);

    if (grn_ctx_trace_log_is_enabled(ctx)) {
      grn_output_envelope_close_json_trace_log(ctx, head, &indent_level);
    }

    if (!is_stream_mode && have_body) {
      json_value_end(ctx, head, indent_level);
      json_key(ctx, head, "body");
    }
  }
  json_map_close(ctx, foot, &indent_level);
}

static void
grn_output_envelope_json(grn_ctx *ctx,
                         grn_obj *head,
                         grn_obj *body,
                         grn_obj *foot,
                         grn_rc rc,
                         double started,
                         double elapsed,
                         const char *file,
                         int line)
{
  bool is_stream_mode = false;
  bool have_body = GRN_TEXT_LEN(body) > 0;
  grn_output_envelope_open_json(ctx, head, is_stream_mode, have_body);
  grn_output_envelope_close_json(ctx,
                                 head,
                                 foot,
                                 is_stream_mode,
                                 have_body,
                                 rc,
                                 started,
                                 elapsed,
                                 file,
                                 line);
}

#ifdef GRN_WITH_MESSAGE_PACK
static void
msgpack_pack_cstr(msgpack_packer *packer, const char *string)
{
  size_t size;

  size = strlen(string);
  msgpack_pack_str(packer, size);
  msgpack_pack_str_body(packer, string, size);
}

static void
grn_output_envelope_msgpack_v1(grn_ctx *ctx,
                               grn_rc rc,
                               grn_obj *head,
                               grn_obj *body,
                               grn_obj *foot,
                               double started,
                               double elapsed,
                               const char *file,
                               int line)
{
  msgpack_writer_ctx head_writer_ctx;
  msgpack_packer header_packer;
  size_t header_size;

  head_writer_ctx.ctx = ctx;
  head_writer_ctx.buffer = head;
  msgpack_packer_init(&header_packer, &head_writer_ctx, msgpack_buffer_writer);

  /* [HEADER, (BODY)] */
  if (GRN_TEXT_LEN(body) > 0) {
    msgpack_pack_array(&header_packer, 2);
  } else {
    msgpack_pack_array(&header_packer, 1);
  }

  /* HEADER := [rc, started, elapsed, (error, (ERROR DETAIL))] */
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
    msgpack_pack_str(&header_packer, strlen(ctx->errbuf));
    msgpack_pack_str_body(&header_packer, ctx->errbuf, strlen(ctx->errbuf));
    if (ctx->errfunc && ctx->errfile) {
      grn_obj *command = GRN_CTX_USER_DATA(ctx)->ptr;
      size_t error_detail_size;

      /* ERROR DETAIL := [[errfunc, errfile, errline, (file, line, command)]] */
      /* TODO: output backtrace */
      msgpack_pack_array(&header_packer, 1);
      error_detail_size = 3;
      if (command) {
        error_detail_size += 3;
      }
      msgpack_pack_array(&header_packer, error_detail_size);

      msgpack_pack_str(&header_packer, strlen(ctx->errfunc));
      msgpack_pack_str_body(&header_packer, ctx->errfunc, strlen(ctx->errfunc));

      msgpack_pack_str(&header_packer, strlen(ctx->errfile));
      msgpack_pack_str_body(&header_packer, ctx->errfile, strlen(ctx->errfile));

      msgpack_pack_unsigned_int(&header_packer, ctx->errline);

      if (command) {
        if (file) {
          msgpack_pack_str(&header_packer, strlen(file));
          msgpack_pack_str_body(&header_packer, file, strlen(file));
        } else {
          msgpack_pack_str(&header_packer, 7);
          msgpack_pack_str_body(&header_packer, "(stdin)", 7);
        }

        msgpack_pack_int(&header_packer, line);

        msgpack_pack_str(&header_packer, GRN_TEXT_LEN(command));
        msgpack_pack_str_body(&header_packer,
                              GRN_TEXT_VALUE(command),
                              GRN_TEXT_LEN(command));
      }
    }
  }
}

static void
grn_output_envelope_msgpack(grn_ctx *ctx,
                            grn_rc rc,
                            grn_obj *head,
                            grn_obj *body,
                            grn_obj *foot,
                            double started,
                            double elapsed,
                            const char *file,
                            int line)
{
  msgpack_writer_ctx writer_ctx;
  msgpack_packer packer;
  size_t n_elements;

  writer_ctx.ctx = ctx;
  writer_ctx.buffer = head;
  msgpack_packer_init(&packer, &writer_ctx, msgpack_buffer_writer);

  /*
   * ENVELOPE := {
   *   "header": HEADER,
   *   "body":   BODY    (optional)
   * }
   */
  if (GRN_TEXT_LEN(body) > 0) {
    n_elements = 2;
  } else {
    n_elements = 1;
  }

  msgpack_pack_map(&packer, n_elements);
  {
    size_t n_header_elements = 3;

    /*
     * HEADER := {
     *   "return_code":  rc,
     *   "start_time":   started,
     *   "elapsed_time": elapsed,
     "   "error": {                   (optional)
     *      "message":  errbuf,
     *      "function": errfunc,
     *      "file":     errfile,
     *      "line":     errline,
     *      "input": {                (optional)
     *        "file":    input_file,
     *        "line":    line,
     *        "command": command
     *      }
     *   }
     * }
     */

    if (rc != GRN_SUCCESS) {
      n_header_elements++;
    }

    msgpack_pack_cstr(&packer, "header");
    msgpack_pack_map(&packer, n_header_elements);
    {
      msgpack_pack_cstr(&packer, "return_code");
      msgpack_pack_int(&packer, rc);

      msgpack_pack_cstr(&packer, "start_time");
      msgpack_pack_double(&packer, started);

      msgpack_pack_cstr(&packer, "elapsed_time");
      msgpack_pack_double(&packer, elapsed);

      if (rc != GRN_SUCCESS) {
        size_t n_error_elements = 1;
        grn_obj *command;

        if (ctx->errfunc) {
          n_error_elements++;
        }
        if (ctx->errfile) {
          n_error_elements += 2;
        }

        command = GRN_CTX_USER_DATA(ctx)->ptr;
        if (file || command) {
          n_error_elements++;
        }

        msgpack_pack_cstr(&packer, "error");
        msgpack_pack_map(&packer, n_error_elements);
        {
          msgpack_pack_cstr(&packer, "message");
          msgpack_pack_cstr(&packer, ctx->errbuf);

          if (ctx->errfunc) {
            msgpack_pack_cstr(&packer, "function");
            msgpack_pack_cstr(&packer, ctx->errfunc);
          }

          if (ctx->errfile) {
            msgpack_pack_cstr(&packer, "file");
            msgpack_pack_cstr(&packer, ctx->errfile);

            msgpack_pack_cstr(&packer, "line");
            msgpack_pack_unsigned_int(&packer, ctx->errline);
          }

          if (file || command) {
            size_t n_input_elements = 0;

            if (file) {
              n_input_elements += 2;
            }
            if (command) {
              n_input_elements++;
            }

            msgpack_pack_cstr(&packer, "input");
            msgpack_pack_map(&packer, n_input_elements);

            if (file) {
              msgpack_pack_cstr(&packer, "file");
              msgpack_pack_cstr(&packer, file);

              msgpack_pack_cstr(&packer, "line");
              msgpack_pack_int(&packer, line);
            }

            if (command) {
              msgpack_pack_cstr(&packer, "command");
              msgpack_pack_str(&packer, GRN_TEXT_LEN(command));
              msgpack_pack_str_body(&packer,
                                    GRN_TEXT_VALUE(command),
                                    GRN_TEXT_LEN(command));
            }
          }
        }
      }
    }

    if (GRN_TEXT_LEN(body) > 0) {
      msgpack_pack_cstr(&packer, "body");
    }
  }
}
#endif /* GRN_WITH_MESSAGE_PACK */

static void
grn_output_envelope_close_apache_arrow_metadata(grn_ctx *ctx,
                                                grn_obj *output,
                                                grn_rc rc,
                                                double started,
                                                double elapsed,
                                                const char *file,
                                                int line)
{
  grn_arrow_stream_writer *writer = grn_arrow_stream_writer_open(ctx, output);
  if (!writer) {
    return;
  }

  grn_arrow_stream_writer_add_metadata(ctx,
                                       writer,
                                       "GROONGA:data_type",
                                       "metadata");
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "return_code",
                                    grn_ctx_at(ctx, GRN_DB_INT32));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "start_time",
                                    grn_ctx_at(ctx, GRN_DB_TIME));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "elapsed_time",
                                    grn_ctx_at(ctx, GRN_DB_FLOAT));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_message",
                                    grn_ctx_at(ctx, GRN_DB_SHORT_TEXT));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_file",
                                    grn_ctx_at(ctx, GRN_DB_SHORT_TEXT));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_line",
                                    grn_ctx_at(ctx, GRN_DB_UINT32));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_function",
                                    grn_ctx_at(ctx, GRN_DB_SHORT_TEXT));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_input_file",
                                    grn_ctx_at(ctx, GRN_DB_SHORT_TEXT));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_input_line",
                                    grn_ctx_at(ctx, GRN_DB_INT32));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "error_input_command",
                                    grn_ctx_at(ctx, GRN_DB_SHORT_TEXT));
  grn_arrow_stream_writer_write_schema(ctx, writer);

  grn_arrow_stream_writer_open_record(ctx, writer);
  grn_arrow_stream_writer_add_column_int32(ctx, writer, rc);
  grn_arrow_stream_writer_add_column_timestamp(
    ctx,
    writer,
    grn_timeval_from_double(ctx, started));
  grn_arrow_stream_writer_add_column_float(ctx, writer, elapsed);
  if (rc == GRN_SUCCESS) {
    /* error_message */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
    /* error_file */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
    /* error_line */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
    /* error_function */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
    /* error_input_file */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
    /* error_input_line */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
    /* error_input_command */
    grn_arrow_stream_writer_add_column_null(ctx, writer);
  } else {
    grn_arrow_stream_writer_add_column_text(ctx,
                                            writer,
                                            ctx->errbuf,
                                            strlen(ctx->errbuf));
    if (ctx->errfile && ctx->errfunc) {
      grn_arrow_stream_writer_add_column_text(ctx,
                                              writer,
                                              ctx->errfile,
                                              strlen(ctx->errfile));
      grn_arrow_stream_writer_add_column_uint32(ctx, writer, ctx->errline);
      grn_arrow_stream_writer_add_column_text(ctx,
                                              writer,
                                              ctx->errfunc,
                                              strlen(ctx->errfunc));
    } else {
      grn_arrow_stream_writer_add_column_null(ctx, writer);
      grn_arrow_stream_writer_add_column_null(ctx, writer);
      grn_arrow_stream_writer_add_column_null(ctx, writer);
    }
    grn_obj *command = GRN_CTX_USER_DATA(ctx)->ptr;
    if (file && command) {
      grn_arrow_stream_writer_add_column_text(ctx, writer, file, strlen(file));
      grn_arrow_stream_writer_add_column_int32(ctx, writer, line);
      grn_arrow_stream_writer_add_column_text(ctx,
                                              writer,
                                              GRN_TEXT_VALUE(command),
                                              GRN_TEXT_LEN(command));
    } else {
      grn_arrow_stream_writer_add_column_null(ctx, writer);
      grn_arrow_stream_writer_add_column_null(ctx, writer);
      grn_arrow_stream_writer_add_column_null(ctx, writer);
    }
  }
  grn_arrow_stream_writer_close_record(ctx, writer);
  grn_arrow_stream_writer_close(ctx, writer);
}

static void
grn_output_envelope_close_apache_arrow_trace_log(grn_ctx *ctx, grn_obj *output)
{
  grn_arrow_stream_writer *writer = grn_arrow_stream_writer_open(ctx, output);
  if (!writer) {
    return;
  }

  grn_arrow_stream_writer_add_metadata(ctx,
                                       writer,
                                       "GROONGA:data_type",
                                       "trace_log");
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "depth",
                                    grn_ctx_at(ctx, GRN_DB_UINT16));
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "sequence",
                                    grn_ctx_at(ctx, GRN_DB_UINT16));
  grn_arrow_stream_writer_add_field_text_dictionary(
    ctx,
    writer,
    "name",
    grn_ctx_at(ctx, GRN_DB_INT16));
  enum value_type {
    VALUE_TYPE_UINT32,
    VALUE_TYPE_SHORT_TEXT,
    N_VALUE_TYPES,
  };
  grn_obj *value_types[N_VALUE_TYPES];
  value_types[VALUE_TYPE_UINT32] = grn_ctx_at(ctx, GRN_DB_UINT32);
  value_types[VALUE_TYPE_SHORT_TEXT] = grn_ctx_at(ctx, GRN_DB_SHORT_TEXT);
  grn_arrow_stream_writer_add_field_union(ctx,
                                          writer,
                                          "value",
                                          value_types,
                                          N_VALUE_TYPES);
  grn_arrow_stream_writer_add_field(ctx,
                                    writer,
                                    "elapsed_time",
                                    grn_ctx_at(ctx, GRN_DB_UINT64));
  grn_arrow_stream_writer_write_schema(ctx, writer);

  grn_obj *depths = &(ctx->impl->trace_log.depths);
  grn_obj *sequences = &(ctx->impl->trace_log.sequences);
  grn_obj *names = &(ctx->impl->trace_log.names);
  grn_obj *values = &(ctx->impl->trace_log.values);
  grn_obj *elapsed_times = &(ctx->impl->trace_log.elapsed_times);
  uint32_t n_logs = grn_vector_size(ctx, names);
  uint32_t i;
  for (i = 0; i < n_logs; i++) {
    grn_arrow_stream_writer_open_record(ctx, writer);

    uint16_t depth = GRN_UINT16_VALUE_AT(depths, i);
    grn_arrow_stream_writer_add_column_uint16(ctx, writer, depth);

    uint16_t sequence = GRN_UINT16_VALUE_AT(sequences, i);
    grn_arrow_stream_writer_add_column_uint16(ctx, writer, sequence);

    const char *name;
    uint32_t name_size =
      grn_vector_get_element_float(ctx, names, i, &name, NULL, NULL);
    grn_arrow_stream_writer_add_column_text_dictionary(ctx,
                                                       writer,
                                                       name,
                                                       name_size);

    const char *value;
    grn_id value_domain;
    uint32_t value_size =
      grn_vector_get_element_float(ctx, values, i, &value, NULL, &value_domain);
    switch (value_domain) {
    case GRN_DB_UINT32:
      {
        grn_arrow_stream_writer_add_column_union(ctx,
                                                 writer,
                                                 VALUE_TYPE_UINT32);
        uint32_t value_uint32 =
          (value_size == 0) ? 0 : *((const uint32_t *)value);
        grn_arrow_stream_writer_add_column_uint32(ctx, writer, value_uint32);
      }
      break;
    default:
      grn_arrow_stream_writer_add_column_union(ctx,
                                               writer,
                                               VALUE_TYPE_SHORT_TEXT);
      grn_arrow_stream_writer_add_column_text(ctx, writer, value, value_size);
      break;
    }

    uint64_t elapsed_time = GRN_UINT64_VALUE_AT(elapsed_times, i);
    grn_arrow_stream_writer_add_column_uint64(ctx, writer, elapsed_time);

    grn_arrow_stream_writer_close_record(ctx, writer);
  }
  grn_arrow_stream_writer_close(ctx, writer);
}

static void
grn_output_envelope_close_apache_arrow(grn_ctx *ctx,
                                       grn_obj *output,
                                       grn_rc rc,
                                       double started,
                                       double elapsed,
                                       const char *file,
                                       int line)
{
  if (ctx->impl->output.arrow_stream_writer) {
    grn_arrow_stream_writer_close(ctx, ctx->impl->output.arrow_stream_writer);
    ctx->impl->output.arrow_stream_writer = NULL;
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  grn_output_envelope_close_apache_arrow_metadata(ctx,
                                                  output,
                                                  rc,
                                                  started,
                                                  elapsed,
                                                  file,
                                                  line);
  if (grn_ctx_trace_log_is_enabled(ctx)) {
    grn_output_envelope_close_apache_arrow_trace_log(ctx, output);
  }
}

static void
grn_output_envelope_apache_arrow(grn_ctx *ctx,
                                 grn_obj *head,
                                 grn_obj *body,
                                 grn_obj *foot,
                                 grn_rc rc,
                                 double started,
                                 double elapsed,
                                 const char *file,
                                 int line)
{
  grn_output_envelope_close_apache_arrow(ctx,
                                         head,
                                         rc,
                                         started,
                                         elapsed,
                                         file,
                                         line);
}

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

  grn_timeval tv_now;
  grn_timeval_now(ctx, &tv_now);
  started = (double)(ctx->impl->tv.tv_sec);
  started += ctx->impl->tv.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  finished = (double)(tv_now.tv_sec);
  finished += tv_now.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  elapsed = finished - started;

  switch (ctx->impl->output.type) {
  case GRN_CONTENT_JSON:
    {
      grn_obj *expr;
      grn_obj *jsonp_func = NULL;

      expr = ctx->impl->curr_expr;
      if (expr) {
        jsonp_func = grn_expr_get_var(ctx,
                                      expr,
                                      JSON_CALLBACK_PARAM,
                                      strlen(JSON_CALLBACK_PARAM));
      }
      if (jsonp_func && GRN_TEXT_LEN(jsonp_func)) {
        GRN_TEXT_PUT(ctx,
                     head,
                     GRN_TEXT_VALUE(jsonp_func),
                     GRN_TEXT_LEN(jsonp_func));
        GRN_TEXT_PUTC(ctx, head, '(');
      }

      if (grn_ctx_get_command_version(ctx) <= GRN_COMMAND_VERSION_2) {
        grn_output_envelope_json_v1(ctx,
                                    head,
                                    body,
                                    foot,
                                    rc,
                                    started,
                                    elapsed,
                                    file,
                                    line);
      } else {
        grn_output_envelope_json(ctx,
                                 head,
                                 body,
                                 foot,
                                 rc,
                                 started,
                                 elapsed,
                                 file,
                                 line);
      }

      if (jsonp_func && GRN_TEXT_LEN(jsonp_func)) {
        GRN_TEXT_PUTS(ctx, foot, ");");
      }
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
        grn_text_ulltoa(ctx, head, ctx->errline);
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
        int len =
          grn_obj_name(ctx, ctx->impl->curr_expr, buf, GRN_TABLE_MAX_KEY_SIZE);
        buf[len] = '\0';
        is_select = strcmp(buf, "select") == 0;
      }
      if (is_select) {
        grn_obj transformed;
        GRN_TEXT_INIT(&transformed, 0);
        transform_xml(ctx, body, &transformed);
        if (body->header.impl_flags & GRN_OBJ_REFER) {
          body->header.impl_flags =
            (uint8_t)(body->header.impl_flags &
                      ~((uint8_t)GRN_OBJ_DO_SHALLOW_COPY));
        }
        GRN_TEXT_SET(ctx,
                     body,
                     GRN_TEXT_VALUE(&transformed),
                     GRN_TEXT_LEN(&transformed));
        GRN_OBJ_FIN(ctx, &transformed);
      } else {
        GRN_TEXT_PUTS(
          ctx,
          head,
          "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<RESULT CODE=\"");
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
            grn_text_ulltoa(ctx, head, ctx->errline);
            GRN_TEXT_PUTS(ctx, head, "\"/>");
          }
          GRN_TEXT_PUTS(ctx, head, "</ERROR>");
        }
        GRN_TEXT_PUTS(ctx, foot, "\n</RESULT>");
      }
    }
    break;
  case GRN_CONTENT_MSGPACK:
#ifdef GRN_WITH_MESSAGE_PACK
    if (grn_ctx_get_command_version(ctx) <= GRN_COMMAND_VERSION_2) {
      grn_output_envelope_msgpack_v1(ctx,
                                     rc,
                                     head,
                                     body,
                                     foot,
                                     started,
                                     elapsed,
                                     file,
                                     line);
    } else {
      grn_output_envelope_msgpack(ctx,
                                  rc,
                                  head,
                                  body,
                                  foot,
                                  started,
                                  elapsed,
                                  file,
                                  line);
    }
#endif /* GRN_WITH_MESSAGE_PACK */
    break;
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    grn_output_envelope_apache_arrow(ctx,
                                     head,
                                     body,
                                     foot,
                                     rc,
                                     started,
                                     elapsed,
                                     file,
                                     line);
    break;
  case GRN_CONTENT_NONE:
    break;
  }
}

void
grn_output_envelope_open(grn_ctx *ctx, grn_obj *output)
{
  switch (ctx->impl->output.type) {
  case GRN_CONTENT_JSON:
    {
      grn_obj *expr;
      grn_obj *jsonp_func = NULL;

      expr = ctx->impl->curr_expr;
      if (expr) {
        jsonp_func = grn_expr_get_var(ctx,
                                      expr,
                                      JSON_CALLBACK_PARAM,
                                      strlen(JSON_CALLBACK_PARAM));
      }
      if (jsonp_func && GRN_TEXT_LEN(jsonp_func) > 0) {
        GRN_TEXT_PUT(ctx,
                     output,
                     GRN_TEXT_VALUE(jsonp_func),
                     GRN_TEXT_LEN(jsonp_func));
        GRN_TEXT_PUTC(ctx, output, '(');
      }

      bool is_stream_mode = true;
      bool have_body = true;
      if (grn_ctx_get_command_version(ctx) <= GRN_COMMAND_VERSION_2) {
        grn_output_envelope_open_json_v1(ctx,
                                         output,
                                         is_stream_mode,
                                         have_body);
      } else {
        grn_output_envelope_open_json(ctx, output, is_stream_mode, have_body);
      }
    }
    break;
  case GRN_CONTENT_TSV:
  case GRN_CONTENT_XML:
  case GRN_CONTENT_MSGPACK:
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
  case GRN_CONTENT_APACHE_ARROW:
  case GRN_CONTENT_NONE:
    break;
  }
}

void
grn_output_envelope_close(
  grn_ctx *ctx, grn_obj *output, grn_rc rc, const char *file, int line)
{
  double started, finished, elapsed;

  grn_timeval tv_now;
  grn_timeval_now(ctx, &tv_now);
  started = (double)(ctx->impl->tv.tv_sec);
  started += ctx->impl->tv.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  finished = (double)(tv_now.tv_sec);
  finished += tv_now.tv_nsec / GRN_TIME_NSEC_PER_SEC_F;
  elapsed = finished - started;

  switch (ctx->impl->output.type) {
  case GRN_CONTENT_JSON:
    {
      grn_obj *expr;
      grn_obj *jsonp_func = NULL;

      expr = ctx->impl->curr_expr;
      if (expr) {
        jsonp_func = grn_expr_get_var(ctx,
                                      expr,
                                      JSON_CALLBACK_PARAM,
                                      strlen(JSON_CALLBACK_PARAM));
      }

      bool is_stream_mode = true;
      bool have_body = true;
      if (grn_ctx_get_command_version(ctx) <= GRN_COMMAND_VERSION_2) {
        grn_output_envelope_close_json_v1(ctx,
                                          output,
                                          output,
                                          is_stream_mode,
                                          have_body,
                                          rc,
                                          started,
                                          elapsed,
                                          file,
                                          line);
      } else {
        grn_output_envelope_close_json(ctx,
                                       output,
                                       output,
                                       is_stream_mode,
                                       have_body,
                                       rc,
                                       started,
                                       elapsed,
                                       file,
                                       line);
      }

      if (jsonp_func && GRN_TEXT_LEN(jsonp_func) > 0) {
        GRN_TEXT_PUTS(ctx, output, ");");
      }
    }
    break;
  case GRN_CONTENT_TSV:
  case GRN_CONTENT_XML:
  case GRN_CONTENT_MSGPACK:
  case GRN_CONTENT_GROONGA_COMMAND_LIST:
    break;
  case GRN_CONTENT_APACHE_ARROW:
    grn_output_envelope_close_apache_arrow(ctx,
                                           output,
                                           rc,
                                           started,
                                           elapsed,
                                           file,
                                           line);
    break;
  case GRN_CONTENT_NONE:
    break;
  }
}

grn_rc
grn_obj_format_fin(grn_ctx *ctx, grn_obj_format *format)
{
  GRN_API_ENTER;

  size_t n_columns = GRN_PTR_VECTOR_SIZE(&(format->columns));
  size_t i;
  for (i = 0; i < n_columns; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(&(format->columns), i);
    if (grn_is_reference_count_enable() ||
        column->header.type == GRN_ACCESSOR) {
      grn_obj_unlink(ctx, column);
    }
  }
  GRN_OBJ_FIN(ctx, &(format->columns));
  if (format->expression) {
    GRN_OBJ_FIN(ctx, format->expression);
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_format_set_columns(grn_ctx *ctx,
                           grn_obj_format *format,
                           grn_obj *table,
                           const char *columns,
                           unsigned int columns_len)
{
  grn_rc rc;

  if (grn_expr_is_v1_format(ctx,
                            columns,
                            columns_len,
                            GRN_EXPR_V1_FORMAT_TYPE_OUTPUT_COLUMNS)) {
    rc = grn_obj_columns(ctx, table, columns, columns_len, &(format->columns));
  } else {
    format->expression =
      grn_output_columns_parse(ctx, table, columns, columns_len);
    rc = ctx->rc;
  }

  return rc;
}
