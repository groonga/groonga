/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include "grn_ctx.h"
#include "grn_com.h"
#include "grn_options.h"
#include "grn_msgpack.h"
#include "grn_load.h"
#include "grn_arrow.h"

#ifdef GRN_WITH_MRUBY
# include <mruby.h>
#endif

#ifdef GRN_WITH_LUAJIT
# include <lauxlib.h>
# include <lualib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**** grn_expr ****/

#define GRN_EXPR_MISSING_NAME          "expr_missing"

/**** grn_ctx_impl ****/

#define GRN_CTX_INITED    0x00
#define GRN_CTX_QUITTING  0x0f

#define GRN_CTX_N_SEGMENTS 512

#ifdef USE_MEMORY_DEBUG
typedef struct _grn_alloc_info grn_alloc_info;
struct _grn_alloc_info
{
  void *address;
  int freed;
  size_t size;
  char alloc_backtrace[4096];
  char free_backtrace[4096];
  char *file;
  int line;
  char *func;
  grn_alloc_info *next;
};
#endif

typedef struct _grn_mrb_data grn_mrb_data;
struct _grn_mrb_data {
  grn_bool initialized;
#ifdef GRN_WITH_MRUBY
  mrb_state *state;
  char base_directory[PATH_MAX];
  struct RClass *module;
  struct RClass *object_class;
  grn_hash *checked_procs;
  grn_hash *registered_plugins;
  struct {
    grn_obj from;
    grn_obj to;
  } buffer;
  struct {
    struct RClass *time_class;
  } builtin;
  struct {
    struct RClass *operator_class;
  } groonga;
#endif
};

typedef struct _grn_lua_data grn_lua_data;
struct _grn_lua_data {
  grn_bool initialized;
#ifdef GRN_WITH_LUAJIT
  lua_State *state;
#endif
};

struct _grn_ctx_impl {
  grn_encoding encoding;

  /* memory pool portion */
  int32_t lifoseg;
  int32_t currseg;
  grn_critical_section lock;
  grn_io_mapinfo segs[GRN_CTX_N_SEGMENTS];

#ifdef USE_MEMORY_DEBUG
  /* memory debug portion */
  grn_alloc_info *alloc_info;
#endif

  /* expression portion */
  grn_obj **stack;
  uint32_t stack_curr;
  uint32_t stack_size;
  grn_hash *expr_vars;
  grn_obj *curr_expr;
  grn_obj current_request_id;
  void *current_request_timer_id;
  void *parser;
  grn_timeval tv;
  grn_selector_data *current_selector_data;

  /* loader portion */
  grn_edge *edge;
  grn_loader loader;
  grn_arrow_stream_loader *arrow_stream_loader;

  /* plugin portion */
  const char *plugin_path;

  /* output portion */
  struct {
    grn_obj *buf;
    grn_recv_handler_func func;
    union {
      void *ptr;
      int fd;
      uint32_t u32;
      uint64_t u64;
    } data;
    grn_content_type type;
    const char *mime_type;
    grn_bool is_pretty;
    grn_obj names;
    grn_obj levels;
#ifdef GRN_WITH_MESSAGE_PACK
    msgpack_packer msgpacker;
#endif
    grn_arrow_stream_writer *arrow_stream_writer;
  } output;

  struct {
    int flags;
    grn_command_version version;
    struct {
      grn_obj *command;
      grn_command_version version;
    } keep;
  } command;

  /* match escalation portion */
  int64_t match_escalation_threshold;
  grn_bool force_match_escalation;

  /* lifetime portion */
  grn_proc_func *finalizer;

  grn_obj *db;
  grn_array *values;        /* temporary objects */
  grn_pat *temporary_columns;
  grn_options *temporary_options;
  grn_critical_section columns_cache_lock;
  grn_hash *columns_cache;
  grn_hash *ios;        /* IOs */
  grn_com *com;
  unsigned int com_status;

  grn_obj query_log_buf;

  char previous_errbuf[GRN_CTX_MSGSIZE];
  unsigned int n_same_error_messages;

  grn_mrb_data mrb;
  grn_lua_data lua;

  struct {
    grn_obj stack;
    grn_obj *current;
  } temporary_open_spaces;

  grn_hash *variables;

  struct {
    grn_critical_section lock;
    grn_obj pool;
  } children;
  grn_ctx *parent;
};

void grn_ctx_impl_columns_cache_delete(grn_ctx *ctx, grn_id table_id);
void grn_ctx_impl_columns_cache_clear(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif
