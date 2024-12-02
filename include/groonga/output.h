/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2023  Sutou Kouhei <kou@clear-code.com>

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

typedef struct _grn_obj_format grn_obj_format;

#define GRN_OBJ_FORMAT_WITH_COLUMN_NAMES (0x01 << 0)
#define GRN_OBJ_FORMAT_AS_ARRAY          (0x01 << 3)
/* Deprecated since 4.0.1. It will be removed at 5.0.0.
   Use GRN_OBJ_FORMAT_AS_ARRAY instead.*/
#define GRN_OBJ_FORMAT_ASARRAY     GRN_OBJ_FORMAT_AS_ARRAY
#define GRN_OBJ_FORMAT_WITH_WEIGHT (0x01 << 4)
/* Format weight as float32.
   Since 10.0.3 */
#define GRN_OBJ_FORMAT_WEIGHT_FLOAT32 (0x01 << 5)
/* Call grn_ctx_output_flush() internally for each 1024 records in a table.
   The "1024" value may be changed.
   Since 10.0.3 */
#define GRN_OBJ_FORMAT_AUTO_FLUSH (0x01 << 6)

struct _grn_obj_format {
  grn_obj columns;
  const void *min;
  const void *max;
  unsigned int min_size;
  unsigned int max_size;
  int nhits;
  int offset;
  int limit;
  int hits_offset;
  uint32_t flags;
  grn_obj *expression;
};

GRN_API grn_rc
grn_output_range_normalize(grn_ctx *ctx, int size, int *offset, int *limit);

#define GRN_OBJ_FORMAT_INIT(format,                                            \
                            format_nhits,                                      \
                            format_offset,                                     \
                            format_limit,                                      \
                            format_hits_offset)                                \
  do {                                                                         \
    GRN_PTR_INIT(&(format)->columns, GRN_OBJ_VECTOR, GRN_ID_NIL);              \
    (format)->nhits = (format_nhits);                                          \
    (format)->offset = (format_offset);                                        \
    (format)->limit = (format_limit);                                          \
    (format)->hits_offset = (format_hits_offset);                              \
    (format)->flags = 0;                                                       \
    (format)->expression = NULL;                                               \
  } while (0)

/* Deprecated since 10.0.0. Use grn_obj_format_fin() instead. */
#define GRN_OBJ_FORMAT_FIN(ctx, format) grn_obj_format_fin((ctx), (format))
GRN_API grn_rc
grn_obj_format_fin(grn_ctx *ctx, grn_obj_format *format);

GRN_API void
grn_output_obj(grn_ctx *ctx,
               grn_obj *outbuf,
               grn_content_type output_type,
               grn_obj *obj,
               grn_obj_format *format);
GRN_API void
grn_output_envelope(grn_ctx *ctx,
                    grn_rc rc,
                    grn_obj *head,
                    grn_obj *body,
                    grn_obj *foot,
                    const char *file,
                    int line);
GRN_API void
grn_output_envelope_open(grn_ctx *ctx, grn_obj *output);
GRN_API void
grn_output_envelope_close(
  grn_ctx *ctx, grn_obj *output, grn_rc rc, const char *file, int line);

GRN_API void
grn_output_array_open(grn_ctx *ctx,
                      grn_obj *outbuf,
                      grn_content_type output_type,
                      const char *name,
                      int n_elements);
GRN_API void
grn_output_array_close(grn_ctx *ctx,
                       grn_obj *outbuf,
                       grn_content_type output_type);
GRN_API void
grn_output_map_open(grn_ctx *ctx,
                    grn_obj *outbuf,
                    grn_content_type output_type,
                    const char *name,
                    int n_elements);
GRN_API void
grn_output_map_close(grn_ctx *ctx,
                     grn_obj *outbuf,
                     grn_content_type output_type);
GRN_API void
grn_output_null(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type);
GRN_API void
grn_output_int32(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 int32_t value);
GRN_API void
grn_output_uint32(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  uint32_t value);
GRN_API void
grn_output_int64(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 int64_t value);
GRN_API void
grn_output_uint64(grn_ctx *ctx,
                  grn_obj *outbuf,
                  grn_content_type output_type,
                  uint64_t value);
#ifdef GRN_HAVE_BFLOAT16
GRN_API void
grn_output_bfloat16(grn_ctx *ctx,
                    grn_obj *outbuf,
                    grn_content_type output_type,
                    grn_bfloat16 value);
#endif
GRN_API void
grn_output_float32(grn_ctx *ctx,
                   grn_obj *outbuf,
                   grn_content_type output_type,
                   float value);
GRN_API void
grn_output_float(grn_ctx *ctx,
                 grn_obj *outbuf,
                 grn_content_type output_type,
                 double value);
GRN_API void
grn_output_cstr(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                const char *value);
GRN_API void
grn_output_str(grn_ctx *ctx,
               grn_obj *outbuf,
               grn_content_type output_type,
               const char *value,
               size_t value_len);
GRN_API void
grn_output_bool(grn_ctx *ctx,
                grn_obj *outbuf,
                grn_content_type output_type,
                bool value);

GRN_API void
grn_ctx_output_flush(grn_ctx *ctx, int flags);
GRN_API void
grn_ctx_output_array_open(grn_ctx *ctx, const char *name, int nelements);
GRN_API void
grn_ctx_output_array_close(grn_ctx *ctx);
GRN_API void
grn_ctx_output_map_open(grn_ctx *ctx, const char *name, int nelements);
GRN_API void
grn_ctx_output_map_close(grn_ctx *ctx);
GRN_API void
grn_ctx_output_null(grn_ctx *ctx);
GRN_API void
grn_ctx_output_int32(grn_ctx *ctx, int32_t value);
GRN_API void
grn_ctx_output_uint32(grn_ctx *ctx, uint32_t value);
GRN_API void
grn_ctx_output_int64(grn_ctx *ctx, int64_t value);
GRN_API void
grn_ctx_output_uint64(grn_ctx *ctx, uint64_t value);
GRN_API void
grn_ctx_output_float32(grn_ctx *ctx, float value);
GRN_API void
grn_ctx_output_float(grn_ctx *ctx, double value);
GRN_API void
grn_ctx_output_cstr(grn_ctx *ctx, const char *value);
GRN_API void
grn_ctx_output_str(grn_ctx *ctx, const char *value, size_t value_len);
GRN_API void
grn_ctx_output_bool(grn_ctx *ctx, bool value);
GRN_API void
grn_ctx_output_obj(grn_ctx *ctx, grn_obj *value, grn_obj_format *format);
GRN_API void
grn_ctx_output_result_set_open_metadata(grn_ctx *ctx,
                                        grn_obj *result_set,
                                        grn_obj_format *format,
                                        uint32_t n_additional_elements);
GRN_API void
grn_ctx_output_result_set_open(grn_ctx *ctx,
                               grn_obj *result_set,
                               grn_obj_format *format,
                               uint32_t n_additional_elements);
GRN_API void
grn_ctx_output_result_set_close(grn_ctx *ctx,
                                grn_obj *result_set,
                                grn_obj_format *format);
GRN_API void
grn_ctx_output_result_set(grn_ctx *ctx,
                          grn_obj *result_set,
                          grn_obj_format *format);
GRN_API void
grn_ctx_output_table_columns(grn_ctx *ctx,
                             grn_obj *table,
                             grn_obj_format *format);
GRN_API void
grn_ctx_output_table_records_open(grn_ctx *ctx, int n_records);
GRN_API void
grn_ctx_output_table_records_content(grn_ctx *ctx,
                                     grn_obj *table,
                                     grn_obj_format *format);
GRN_API void
grn_ctx_output_table_records_close(grn_ctx *ctx);
GRN_API void
grn_ctx_output_table_records(grn_ctx *ctx,
                             grn_obj *table,
                             grn_obj_format *format);

/**
 * \brief Get the current output type of the context.
 *
 * \note Normally, you don't need to call this function unless you need to
 *       check the output type explicitly.
 *
 * \param ctx The context object.
 *
 * \return Return the output type of the context, which is one of the values
 *         defined in \ref grn_content_type. Return \ref GRN_CONTENT_NONE If the
 *         context is invalid.
 */
GRN_API grn_content_type
grn_ctx_get_output_type(grn_ctx *ctx);
/**
 * \brief Set the output type for the context when executing a command.
 *
 * The output type determines the format of the result when executing a command
 * using \ref grn_expr_exec. If you use \ref grn_ctx_send, the new output type
 * isn't used because \ref grn_ctx_send sets the output type internally based on
 * the command.
 *
 * \note Normally, you don't need to call this function unless you need to
 * change the output type explicitly when executing a command using \ref
 * grn_expr_exec.
 *
 * \param ctx The context object.
 * \param type The new output type to set.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if `type` is
 *         invalid.
 */
GRN_API grn_rc
grn_ctx_set_output_type(grn_ctx *ctx, grn_content_type type);
GRN_API const char *
grn_ctx_get_mime_type(grn_ctx *ctx);

/* obsolete */
GRN_API grn_rc
grn_text_otoj(grn_ctx *ctx,
              grn_obj *bulk,
              grn_obj *obj,
              grn_obj_format *format);

#ifdef __cplusplus
}
#endif
