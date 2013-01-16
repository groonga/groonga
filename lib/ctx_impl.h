/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2012 Brazil

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
#ifndef GRN_CTX_IMPL_H
#define GRN_CTX_IMPL_H

#ifndef GRN_CTX_H
#include "ctx.h"
#endif /* GRN_CTX_H */

#ifndef GRN_COM_H
#include "com.h"
#endif /* GRN_COM_H */

#ifdef WITH_MESSAGE_PACK
#include <msgpack.h>
#endif

#ifdef WITH_MRUBY
# include <mruby.h>
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

typedef struct {
  grn_obj values;
  grn_obj level;
  grn_obj columns;
  uint32_t emit_level;
  int32_t key_offset;
  grn_obj *table;
  grn_obj *last;
  grn_obj *ifexists;
  grn_obj *each;
  uint32_t unichar;
  uint32_t values_size;
  uint32_t nrecords;
  grn_loader_stat stat;
  grn_content_type input_type;
} grn_loader;

#define GRN_CTX_N_SEGMENTS 512

#ifdef USE_MEMORY_DEBUG
typedef struct _grn_alloc_info grn_alloc_info;
struct _grn_alloc_info
{
  void *address;
  int freed;
  char alloc_backtrace[4096];
  char free_backtrace[4096];
  grn_alloc_info *next;
};
#endif

struct _grn_ctx_impl {
  grn_encoding encoding;

  /* memory pool portion */
  int32_t lifoseg;
  int32_t currseg;
  grn_critical_section lock;
  grn_io_mapinfo segs[GRN_CTX_N_SEGMENTS];

#ifdef USE_DYNAMIC_MALLOC_CHANGE
  /* memory allocation portion */
  grn_malloc_func malloc_func;
  grn_calloc_func calloc_func;
  grn_realloc_func realloc_func;
  grn_strdup_func strdup_func;
#endif

#ifdef USE_MEMORY_DEBUG
  /* memory debug portion */
  grn_alloc_info *alloc_info;
#endif

  /* qe portion */
  grn_obj *stack[GRN_STACK_SIZE];
  uint32_t stack_curr;
  grn_hash *expr_vars;
  grn_obj *curr_expr;
  grn_obj *qe_next;
  void *parser;
  grn_timeval tv;

  /* loader portion */
  grn_edge *edge;
  grn_loader loader;

  /* plugin portion */
  const char *plugin_path;

  /* output portion */
  grn_content_type output_type;
  const char *mime_type;
  grn_obj names;
  grn_obj levels;

  /* command portion */
  grn_command_version command_version;

  /* match escalation portion */
  int64_t match_escalation_threshold;

  /* lifetime portion */
  grn_proc_func *finalizer;

  /* ql portion */
  uint8_t batchmode;
  uint8_t op;
  int tok;
  char *cur;
  char *str_end;
  grn_obj *db;
  uint32_t n_entries;
  grn_array *values;        /* temporary objects */
  grn_hash *ios;        /* IOs */
  char *inbuf;
  grn_obj *outbuf;
  grn_obj subbuf;
  unsigned int bufcur;
  void (*output)(grn_ctx *, int, void *);
  grn_com *com;
  unsigned int com_status;
  union {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
  } data;

#ifdef WITH_MESSAGE_PACK
  msgpack_packer msgpacker;
#endif
#ifdef WITH_MRUBY
  mrb_state *mrb;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* GRN_CTX_IMPL_H */
