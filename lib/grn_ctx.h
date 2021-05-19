/*
  Copyright(C) 2009-2016  Brazil
  Copyright(C) 2020-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_error.h"

#include <errno.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#define GRN_BREAK_POINT raise(SIGTRAP)
#endif /* HAVE_SIGNAL_H */

#include "grn_alloc.h"
#include "grn_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/**** api in/out ****/

#define GRN_API_ENTER do {\
  if ((ctx)->seqno & 1) {\
    (ctx)->subno++;\
  } else {\
    (ctx)->errlvl = GRN_OK;\
    if ((ctx)->rc != GRN_CANCEL) {\
      (ctx)->rc = GRN_SUCCESS;\
    }\
    (ctx)->seqno++;\
  }\
  GRN_TEST_YIELD();\
} while (0)

/* CAUTION!! : pass only variables or constants as r */
#define GRN_API_RETURN(r) do {\
  if (ctx->subno) {\
    ctx->subno--;\
  } else {\
    ctx->seqno++;\
  }\
  GRN_TEST_YIELD();\
  return r;\
} while (0)

#ifdef DEBUG
#define GRN_ASSERT(s) grn_assert(ctx,(s),__FILE__,__LINE__,__FUNCTION__)
#else
#define GRN_ASSERT(s)
#endif

void grn_assert(grn_ctx *ctx, int cond, const char* file, int line, const char* func);

/**** grn_ctx ****/

GRN_VAR grn_ctx grn_gctx;
extern int grn_pagesize;
extern grn_critical_section grn_glock;
extern uint32_t grn_gtick;
extern int grn_lock_timeout;

#define GRN_CTX_ALLOCATED                            (0x80)
#define GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND (0x40)

extern grn_timeval grn_starttime;

void grn_ctx_loader_clear(grn_ctx *ctx);
void grn_log_reopen(grn_ctx *ctx);

GRN_API grn_rc grn_ctx_sendv(grn_ctx *ctx, int argc, char **argv, int flags);
void grn_ctx_set_keep_command(grn_ctx *ctx, grn_obj *command);

grn_ctx *grn_ctx_pull_child(grn_ctx *ctx);
grn_rc grn_ctx_release_child(grn_ctx *ctx, grn_ctx *child_ctx);

grn_content_type grn_get_ctype(grn_obj *var);
grn_content_type grn_content_type_parse(grn_ctx *ctx,
                                        grn_obj *var,
                                        grn_content_type default_value);

/**** db_obj ****/

/* flag values used for grn_obj.header.impl_flags */

#define GRN_OBJ_ALLOCATED              (0x01<<2) /* allocated by ctx */
#define GRN_OBJ_EXPRVALUE              (0x01<<3) /* value allocated by grn_expr */
#define GRN_OBJ_EXPRCONST              (0x01<<4) /* constant allocated by grn_expr */

typedef struct _grn_hook grn_hook;

typedef struct {
  grn_obj_header header;
  grn_id range;  /* table: type of subrecords, column: type of values */
  /* -- compatible with grn_accessor -- */
  grn_id id;
  grn_obj *db;
  grn_user_data user_data;
  grn_proc_func *finalizer;
  grn_hook *hooks[5];
  void *source;
  uint32_t source_size;
  uint32_t max_n_subrecs;
  uint8_t subrec_size;
  uint8_t subrec_offset;
  uint8_t record_unit;
  uint8_t subrec_unit;
  struct {
    grn_table_group_flags flags;
    grn_obj *calc_target;
    grn_id aggregated_value_type_id;
    grn_table_group_aggregator **aggregators;
    uint32_t n_aggregators;
  } group;
  uint32_t reference_count;
} grn_db_obj;

#define GRN_DB_OBJ_SET_TYPE(db_obj,obj_type) do {\
  (db_obj)->obj.header.type = (obj_type);\
  (db_obj)->obj.header.impl_flags = 0;\
  (db_obj)->obj.header.flags = 0;\
  (db_obj)->obj.header.domain = GRN_ID_NIL;\
  (db_obj)->obj.id = GRN_ID_NIL;\
  (db_obj)->obj.user_data.ptr = NULL;\
  (db_obj)->obj.finalizer = NULL;\
  (db_obj)->obj.hooks[0] = NULL;\
  (db_obj)->obj.hooks[1] = NULL;\
  (db_obj)->obj.hooks[2] = NULL;\
  (db_obj)->obj.hooks[3] = NULL;\
  (db_obj)->obj.hooks[4] = NULL;\
  (db_obj)->obj.source = NULL;\
  (db_obj)->obj.source_size = 0;\
  (db_obj)->obj.reference_count = 1;\
} while (0)

/**** receive handler ****/

GRN_API void grn_ctx_stream_out_func(grn_ctx *c, int flags, void *stream);

grn_rc grn_db_init_builtin_procs(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif
