/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

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
#ifndef GRN_DB_H
#define GRN_DB_H

#ifndef GROONGA_H
#include "groonga_in.h"
#endif /* GROONGA_H */

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
  int score;
  int n_subrecs;
  int subrecs[1];
} grn_rset_recinfo;

typedef struct {
  grn_id rid;
  uint32_t sid;
  uint32_t pos;
} grn_rset_posinfo;

#define GRN_RSET_UTIL_BIT (0x80000000)

#define GRN_RSET_SCORE_SIZE (sizeof(int))

#define GRN_RSET_N_SUBRECS(ri) ((ri)->n_subrecs & ~GRN_RSET_UTIL_BIT)

#define GRN_RSET_SUBRECS_CMP(a,b,dir) (((a) - (b))*(dir) > 0)
#define GRN_RSET_SUBRECS_NTH(subrecs,size,n) \
  ((int *)((byte *)subrecs + n * (size + GRN_RSET_SCORE_SIZE)))
#define GRN_RSET_SUBRECS_COPY(subrecs,size,n,src) \
  (memcpy(GRN_RSET_SUBRECS_NTH(subrecs, size, n), src, size + GRN_RSET_SCORE_SIZE))

typedef struct _grn_db grn_db;
typedef struct _grn_proc grn_proc;

grn_rc grn_db_close(grn_ctx *ctx, grn_obj *db);

grn_obj *grn_db_keys(grn_obj *s);

grn_rc _grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id,
                               grn_table_delete_optarg *optarg);

grn_id grn_table_get_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                       void **value);
grn_id grn_table_add_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                       void **value, int *added);
grn_rc grn_table_get_info(grn_ctx *ctx, grn_obj *table, grn_obj_flags *flags,
                          grn_encoding *encoding, grn_obj **tokenizer);
const char *_grn_table_key(grn_ctx *ctx, grn_obj *table, grn_id id, uint32_t *key_size);

grn_rc grn_table_search(grn_ctx *ctx, grn_obj *table,
                        const void *key, uint32_t key_size,
                        grn_search_flags flags, grn_obj *res, grn_sel_operator op);

grn_id grn_table_next(grn_ctx *ctx, grn_obj *table, grn_id id);

int grn_table_get_key2(grn_ctx *ctx, grn_obj *table, grn_id id, grn_obj *bulk);

grn_table_cursor *grn_table_cursor_open_by_id(grn_ctx *ctx, grn_obj *table,
                                              grn_id min, grn_id max, int flags);

void grn_table_add_subrec(grn_obj *table, grn_rset_recinfo *ri, int score,
                          grn_rset_posinfo *pi, int dir);


grn_obj *grn_obj_get_accessor(grn_ctx *ctx, grn_obj *obj,
                              const char *name, unsigned name_size);

grn_obj *grn_obj_graft(grn_ctx *ctx, grn_obj *obj);

typedef struct _grn_hook grn_hook;

struct _grn_hook {
  grn_hook *next;
  grn_proc *proc;
  uint32_t hld_size;
};

typedef enum {
  PROC_INIT = 0,
  PROC_NEXT,
  PROC_FIN
} grn_proc_phase;

typedef struct {
  grn_obj_header header;
  grn_id id;
  grn_id range;  /* table: type of subrecords, column: type of values */
  grn_obj *db;
  grn_hook *hooks[5];
  void *source;
  uint32_t source_size;
  uint32_t max_n_subrecs;
  uint8_t subrec_size;
  uint8_t subrec_offset;
  uint8_t record_unit;
  uint8_t subrec_unit;
  //  grn_obj_flags flags;
} grn_db_obj;

#define GRN_OBJ_TMP_OBJECT 0x80000000

#define GRN_DB_OBJ_SET_TYPE(db_obj,obj_type) {\
  (db_obj)->obj.header.type = (obj_type);\
  (db_obj)->obj.header.impl_flags = 0;\
  (db_obj)->obj.header.flags = 0;\
  (db_obj)->obj.id = GRN_ID_NIL;\
  (db_obj)->obj.hooks[0] = NULL;\
  (db_obj)->obj.hooks[1] = NULL;\
  (db_obj)->obj.hooks[2] = NULL;\
  (db_obj)->obj.hooks[3] = NULL;\
  (db_obj)->obj.hooks[4] = NULL;\
  (db_obj)->obj.source = NULL;\
  (db_obj)->obj.source_size = 0;\
}

#define GRN_DB_OBJP(obj) \
  (obj &&\
   (GRN_TYPE <= ((grn_db_obj *)obj)->header.type) &&\
   (((grn_db_obj *)obj)->header.type <= GRN_COLUMN_INDEX))

#define GRN_OBJ_TABLEP(obj) \
  (obj &&\
   (GRN_TABLE_HASH_KEY <= ((grn_db_obj *)obj)->header.type) &&\
   (((grn_db_obj *)obj)->header.type <= GRN_DB))

typedef struct _grn_proc_ctx grn_proc_ctx;
struct _grn_proc_ctx {
  grn_proc_data user_data;
  grn_obj *obj;
  grn_hook *hooks;
  grn_hook *currh;
  grn_proc_phase phase;
  unsigned short nargs;
  unsigned short offset;
  grn_proc_data data[16];
};

struct _grn_proc {
  grn_db_obj obj;
  grn_proc_func *funcs[3];
  uint32_t nargs;
  uint32_t nresults;
  grn_obj results[16];
};

/* vector */

/*
typedef struct _grn_vector grn_vector;

struct _grn_vector {
  grn_obj str;
  uint32_t *offsets;
  int n_entries;
};

const char *grn_vector_fetch(grn_ctx *ctx, grn_obj *vector, int i, unsigned int *size);
int grn_vector_delimit(grn_ctx *ctx, grn_obj *vector);
int grn_vector_size(grn_ctx *ctx, grn_obj *vector);
*/

grn_rc grn_vector_delimit(grn_ctx *ctx, grn_obj *v, unsigned int weight, grn_id domain);

grn_rc grn_db_init_builtin_types(grn_ctx *ctx);

/* flag values used for grn_obj.header.impl_flags */

#define GRN_OBJ_ALLOCATED              (0x01<<2) /* allocated by ctx */
#define GRN_OBJ_EXPRVALUE              (0x01<<3) /* value allocated by grn_expr */
#define GRN_OBJ_EXPRCONST              (0x01<<4) /* constant allocated by grn_expr */

/* flag value used for grn_obj.header.flags */

#define GRN_OBJ_CUSTOM_NAME            (0x01<<12) /* db_obj which has custom name */

#define GRN_OBJ_RESOLVE(ctx,obj) \
  (((obj)->header.type != GRN_PTR)\
   ? (obj)\
   : ((obj)->u.b.head\
      ? (grn_obj *)(obj)->u.b.head\
      : grn_ctx_at((ctx), (obj)->header.domain)))

/* expr */

void grn_obj_unlink(grn_ctx *ctx, grn_obj *obj);

grn_rc grn_ctx_push(grn_ctx *ctx, grn_obj *obj);
grn_obj *grn_ctx_pop(grn_ctx *ctx);

typedef struct _grn_expr grn_expr;

typedef struct {
  grn_op op;
  grn_obj *value;
} grn_expr_code;

struct _grn_expr {
  grn_db_obj obj;
  grn_obj *names;
  grn_obj *vars;
  grn_obj *consts;
  grn_obj *values;
  grn_obj **stack;
  grn_expr_code *codes;
  uint32_t nvars;
  uint32_t nconsts;
  uint32_t values_curr;
  uint32_t values_tail;
  uint32_t values_size;
  uint32_t codes_curr;
  uint32_t codes_size;
  uint32_t stack_curr;
  uint32_t stack_size;
};

#ifdef __cplusplus
}
#endif

#endif /* GRN_DB_H */
