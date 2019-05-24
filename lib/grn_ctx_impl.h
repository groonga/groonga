/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018-2019 Kouhei Sutou <kou@clear-code.com>

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

typedef enum {
  GRN_LOADER_BEGIN = 0,
  GRN_LOADER_TOKEN,
  GRN_LOADER_STRING,
  GRN_LOADER_SYMBOL,
  GRN_LOADER_NUMBER,
  GRN_LOADER_STRING_ESC,
  GRN_LOADER_UNICODE0,
  GRN_LOADER_UNICODE1,
  GRN_LOADER_UNICODE2,
  GRN_LOADER_UNICODE3,
  GRN_LOADER_END
} grn_loader_stat;

/*
 * Status of target columns used in Format 1.
 * Target columns are specified via --columns or the first array in a Format 1
 * JSON object.
 */
typedef enum {
  GRN_LOADER_COLUMNS_UNSET = 0, /* Columns are not available. */
  GRN_LOADER_COLUMNS_SET,       /* Columns are available. */
  GRN_LOADER_COLUMNS_BROKEN     /* Columns are specified but broken. */
} grn_loader_columns_status;

typedef struct {
  grn_obj values;
  grn_obj level;
  grn_obj columns;
  grn_obj ids;
  grn_obj return_codes;
  grn_obj error_messages;
  uint32_t emit_level;
  int32_t id_offset;  /* Position of _id in values or -1 if _id is N/A. */
  int32_t key_offset; /* Position of _key in values or -1 if _key is N/A. */
  grn_obj *table;
  grn_obj *last;
  grn_obj *ifexists;
  grn_obj *each;
  uint32_t unichar;
  uint32_t unichar_hi;
  uint32_t values_size;
  uint32_t nrecords;
  uint32_t n_record_errors;
  uint32_t n_column_errors;
  grn_loader_stat stat;
  grn_content_type input_type;
  grn_loader_columns_status columns_status;
  grn_rc rc;
  char errbuf[GRN_CTX_MSGSIZE];
  grn_bool output_ids;
  grn_bool output_errors;
  grn_bool lock_table;
} grn_loader;

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

  /* loader portion */
  grn_edge *edge;
  grn_loader loader;

  /* plugin portion */
  const char *plugin_path;

  /* output portion */
  struct {
    grn_obj *buf;
    void (*func)(grn_ctx *, int, void *);
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
};

#ifdef __cplusplus
}
#endif
