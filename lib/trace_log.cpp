/*
  Copyright (C) 2023  Sutou Kouhei <kou@clear-code.com>

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

#include <chrono>

namespace {
  uint64_t
  now_nanoseconds()
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
  }
}; // namespace

extern "C" {
void
grn_ctx_trace_log_enable(grn_ctx *ctx)
{
  if (grn_ctx_trace_log_is_enabled(ctx)) {
    grn_ctx_trace_log_disable(ctx);
  }
  ctx->impl->trace_log.start_time = now_nanoseconds();
  GRN_UINT16_PUT(ctx, &(ctx->impl->trace_log.sequence_stack), 0);
}

void
grn_ctx_trace_log_disable(grn_ctx *ctx)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  ctx->impl->trace_log.start_time = 0;
  ctx->impl->trace_log.current_depth = 0;
  GRN_BULK_REWIND(&(ctx->impl->trace_log.depths));
  GRN_BULK_REWIND(&(ctx->impl->trace_log.sequence_stack));
  GRN_BULK_REWIND(&(ctx->impl->trace_log.sequences));
  GRN_BULK_REWIND(&(ctx->impl->trace_log.names));
  GRN_BULK_REWIND(&(ctx->impl->trace_log.values));
  GRN_BULK_REWIND(&(ctx->impl->trace_log.elapsed_times));
}

uint16_t
grn_ctx_trace_log_get_current_depth(grn_ctx *ctx)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return 0;
  }
  return ctx->impl->trace_log.current_depth;
}

static inline void
grn_ctx_trace_log_push_common(grn_ctx *ctx)
{
  GRN_UINT16_PUT(ctx, &(ctx->impl->trace_log.sequence_stack), 0);
}

void
grn_ctx_trace_log_push(grn_ctx *ctx)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  ctx->impl->trace_log.current_depth++;
  grn_ctx_trace_log_push_common(ctx);
}

static inline void
grn_ctx_trace_log_pop_common(grn_ctx *ctx)
{
  uint16_t sequence;
  GRN_UINT16_POP(&(ctx->impl->trace_log.sequence_stack), sequence);
}

void
grn_ctx_trace_log_pop(grn_ctx *ctx)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  ctx->impl->trace_log.current_depth--;
  grn_ctx_trace_log_pop_common(ctx);
}

void
grn_ctx_trace_log_set_current_depth(grn_ctx *ctx, uint16_t depth)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  if (depth == ctx->impl->trace_log.current_depth) {
    return;
  }
  uint16_t i;
  if (depth > ctx->impl->trace_log.current_depth) {
    uint16_t n = depth - ctx->impl->trace_log.current_depth;
    for (i = 0; i < n; i++) {
      grn_ctx_trace_log_push_common(ctx);
    }
  } else {
    uint16_t n = ctx->impl->trace_log.current_depth - depth;
    for (i = 0; i < n; i++) {
      grn_ctx_trace_log_pop_common(ctx);
    }
  }
  ctx->impl->trace_log.current_depth = depth;
}

static void
grn_ctx_trace_log_emit_common(grn_ctx *ctx, const char *name)
{
  uint64_t elapsed_time = now_nanoseconds() - ctx->impl->trace_log.start_time;
  GRN_UINT16_PUT(ctx,
                 &(ctx->impl->trace_log.depths),
                 ctx->impl->trace_log.current_depth);
  grn_obj *sequence_stack = &(ctx->impl->trace_log.sequence_stack);
  uint16_t sequence =
    GRN_UINT16_VALUE_AT(sequence_stack,
                        GRN_UINT16_VECTOR_SIZE(sequence_stack) - 1)++;
  GRN_UINT16_PUT(ctx, &(ctx->impl->trace_log.sequences), sequence);
  grn_vector_add_element_float(ctx,
                               &(ctx->impl->trace_log.names),
                               name,
                               strlen(name),
                               0.0,
                               GRN_DB_TEXT);
  GRN_UINT64_PUT(ctx, &(ctx->impl->trace_log.elapsed_times), elapsed_time);
}

void
grn_ctx_trace_log_emit_string(grn_ctx *ctx,
                              const char *name,
                              const char *value,
                              size_t value_length)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  grn_ctx_trace_log_emit_common(ctx, name);
  grn_vector_add_element_float(ctx,
                               &(ctx->impl->trace_log.values),
                               value,
                               value_length,
                               0.0,
                               GRN_DB_TEXT);
}

void
grn_ctx_trace_log_emit_cstring(grn_ctx *ctx,
                               const char *name,
                               const char *value)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  grn_ctx_trace_log_emit_string(ctx, name, value, strlen(value));
}

void
grn_ctx_trace_log_emit_uint32(grn_ctx *ctx, const char *name, uint32_t n)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  grn_ctx_trace_log_emit_common(ctx, name);
  grn_vector_add_element_float(ctx,
                               &(ctx->impl->trace_log.values),
                               (const char *)(&n),
                               sizeof(n),
                               0.0,
                               GRN_DB_UINT32);
}

void
grn_ctx_trace_log_emit_record_key(grn_ctx *ctx,
                                  const char *name,
                                  grn_obj *table,
                                  grn_id id)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  char key[GRN_TABLE_MAX_KEY_SIZE];
  int key_size;
  key_size = grn_table_get_key(ctx, table, id, key, GRN_TABLE_MAX_KEY_SIZE);
  if (key_size == 0) {
    return;
  }
  /* TODO: Support non ShortText key */
  grn_ctx_trace_log_emit_string(ctx, name, key, key_size);
}

void
grn_ctx_trace_log_emit_object(grn_ctx *ctx, const char *name, grn_obj *object)
{
  if (!grn_ctx_trace_log_is_enabled(ctx)) {
    return;
  }
  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, object);
  grn_ctx_trace_log_emit_string(ctx,
                                name,
                                GRN_TEXT_VALUE(&inspected),
                                GRN_TEXT_LEN(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
}
}
