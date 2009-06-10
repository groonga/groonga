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
#ifndef GRN_QL_H
#define GRN_QL_H

#ifndef GRN_STORE_H
#include "store.h"
#endif /* GRN_STORE_H */

#ifndef GRN_COM_H
#include "com.h"
#endif /* GRN_COM_H */

#ifndef GRN_CTX_H
#include "ctx.h"
#endif /* GRN_CTX_H */

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif /* _ISOC99_SOURCE */
#if WIN32
#define _USE_MATH_DEFINES
#endif /* WIN32 */

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**** grn_cell ****/

/* flag values used for grn_cell.header.impl_flags */
/* bit 0~2 are reserved */
#define GRN_CELL_SYMBOL                (1L<<3) /* ql: registered in the symbol table */
#define GRN_CELL_PROMISE               (1L<<4) /* ql: promise object */
#define GRN_CELL_NATIVE                (1L<<5) /* ql: native function */
#define GRN_CELL_MARKED                (1L<<6) /* ql: used in garbage collection */
#define GRN_CELL_MARK2                 (1L<<7) /* ql: used in callback function */

/* type values used for grn_cell.header.type */
#define GRN_CELL_INT                   (0x70)
#define GRN_CELL_FLOAT                 (0x71)
#define GRN_CELL_STR                   (0x72)
#define GRN_CELL_OP                    (0x73)
#define GRN_CELL_OBJECT                (0x74)
#define GRN_CELL_PROC                  (0x75)
#define GRN_CELL_SYNTAX                (0x76)
#define GRN_CELL_TIME                  (0x77)
#define GRN_CELL_PSEUDO_COLUMN         (0x78)
#define GRN_CELL_LIST                  (0x80)
#define GRN_CELL_CLOSURE               (0x81)
#define GRN_CELL_MACRO                 (0x82)
#define GRN_CELL_CONTINUATION          (0x83)

typedef struct _grn_ql_co grn_ql_co;
typedef struct _grn_cell grn_cell;
typedef grn_cell *grn_ql_native_func(grn_ctx *, grn_cell *, grn_ql_co *);
typedef struct _grn_tmp_db_obj grn_tmp_db_obj;

struct _grn_cell {
  grn_obj_header header;
  union {
    struct {
      grn_id id;
      grn_ql_native_func *func;
    } o;
    struct {
      grn_obj *value;
      grn_ql_native_func *func;
    } p;
    struct {
      char *value;
      uint32_t size;
    } b;
    struct {
      grn_cell *car;
      grn_cell *cdr;
    } l;
    struct {
      int64_t i;
    } i;
    struct {
      double d;
    } d;
    struct {
      int8_t op;
      int8_t mode;
      int16_t weight;
      int32_t option;
    } op;
    grn_timeval tv;
  } u;
};

struct _grn_ql_co {
  uint16_t mode;
  uint16_t last;
  grn_ql_native_func *func;
  void *data;
};

struct _grn_tmp_db_obj {
  grn_db_obj *obj;
  grn_cell cell;
};

grn_cell *grn_get(const char *key);
grn_cell *grn_at(const char *key);
grn_rc grn_del(const char *key);

/**** grn_ctx_impl ****/

struct _grn_ctx_impl {
  grn_encoding encoding;

  /* memory pool portion */
  int32_t lifoseg;
  int32_t currseg;
  grn_io_mapinfo mi;
  grn_io_mapinfo *segs;

#ifdef USE_DYNAMIC_MALLOC_CHANGE
  /* memory allocation portion */
  grn_malloc_func malloc_func;
  grn_calloc_func calloc_func;
  grn_realloc_func realloc_func;
  grn_strdup_func strdup_func;
#endif

  /* qe portion */
  grn_obj *stack[16]; // fixme
  uint32_t stack_curr;
  grn_hash *qe;

  /* ql portion */
  uint32_t ncells;
  uint32_t seqno;
  uint32_t lseqno;
  uint32_t nbinds;
  uint32_t nunbinds;
  uint8_t feed_mode;
  uint8_t batchmode;
  uint8_t gc_verbose;
  uint8_t op;
  int tok;
  char *cur;
  char *str_end;
  grn_cell **pht;       /* tail of placeholders */
  grn_cell arg;         /* wait_data container (for coroutine) */
  grn_obj *db;
  uint32_t n_entries;
  grn_array *objects;       /* objects */
  grn_array *values;        /* temporary objects */
  grn_hash *symbols;        /* symbol table */
  grn_cell *phs;        /* list of placeholders */
  grn_ql_co co;             /* coroutine info */
  grn_cell *args;       /* register for arguments of function */
  grn_cell *envir;      /* stack register for current environment */
  grn_cell *code;       /* register for current code */
  grn_cell *dump;       /* stack register for next evaluation */
  grn_cell *value;      /* evaluated value */
  grn_cell *global_env; /* global variables */
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
};

/**** query language ****/

#define GRN_QL_CO_BEGIN(c) if (c) { switch((c)->last) { case 0:
#define GRN_QL_CO_WAIT(c,d) \
  (c)->last = __LINE__; (c)->data = (d); return NULL;\
  case __LINE__: (d) = (c)->data;
#define GRN_QL_CO_END(c) (c)->last = 0; }}

#define GRN_QL_PLACEHOLDER_CHAR '?'

#define GRN_QL_SET_MODE(c,m) ((c)->impl->feed_mode = (m))
#define GRN_QL_GET_MODE(c) ((c)->impl->feed_mode)
#define GRN_QL_GET_STAT(c) ((c)->stat)

enum {
  grn_ql_atonce = 0,
  grn_ql_step
};

#define GRN_QL_TOPLEVEL  0x00
#define GRN_QL_QUITTING  0x0f
#define GRN_QL_EVAL      0x40
#define GRN_QL_NATIVE    0x41
#define GRN_QL_WAIT_EXPR 0xc0
#define GRN_QL_WAIT_ARG  0xc1
#define GRN_QL_WAIT_DATA 0xc2

#define GRN_QL_WAITINGP(c) ((c)->stat & 0x80)

extern grn_cell *grn_ql_nil;  /* special cell representing empty cell */
extern grn_cell *grn_ql_t;    /* special cell representing #t */
extern grn_cell *grn_ql_f;    /* special cell representing #f */

#define NIL grn_ql_nil
#define T grn_ql_t
#define F grn_ql_f

void grn_ql_init_globals(grn_ctx *c);

grn_cell *grn_ql_at(grn_ctx *c, const char *key);
grn_rc grn_ql_def_db_funcs(grn_ctx *c);

#define GRN_OBJ_INSPECT_ESC 1
#define GRN_OBJ_INSPECT_SYMBOL_AS_STR 2

void grn_obj_inspect(grn_ctx *c, grn_cell *obj, grn_obj *buf, int flags);
grn_cell *grn_ql_def_native_func(grn_ctx *c, const char *name, grn_ql_native_func *func);
grn_cell *grn_ql_feed(grn_ctx *c, char *str, uint32_t str_size, int mode);
grn_cell *grn_ql_eval(grn_ctx *c, grn_cell *code, grn_cell *objs);
grn_rc grn_obj2int(grn_ctx *c, grn_cell *o);

const char *_grn_ql_obj_key(grn_ctx *c, grn_cell *obj, uint32_t *key_size);
grn_obj *grn_ql_obj_key(grn_ctx *ctx, grn_cell *obj, grn_obj *value);
grn_cell *grn_ql_mk_symbol(grn_ctx *ctx, const char *name, int name_size);
grn_cell *grn_ql_obj_new(grn_ctx *c, grn_id domain, grn_id self);
void grn_ql_obj_bind(grn_obj *obj, grn_cell *symbol);

grn_cell *grn_ql_mk_string(grn_ctx *c, const char *str, unsigned int len);
grn_cell *grn_ql_mk_symbol2(grn_ctx *ctx, const char *q, unsigned int len, int kwdp);

grn_cell *grn_cell_new(grn_ctx *c);
grn_cell *grn_cell_alloc(grn_ctx *c, uint32_t size);
void grn_cell_clear(grn_ctx *c, grn_cell *o);
grn_cell *grn_cell_cons(grn_ctx *ctx, grn_cell *a, grn_cell *b);
void grn_ql_init_const(void);

grn_rc grn_ql_obj_mark(grn_ctx *ctx, grn_cell *o);
grn_rc grn_ql_obj_unmark(grn_ctx *ctx, grn_cell *o);

grn_rc grn_ctx_mgc(grn_ctx *ctx);

typedef struct _patsnip_spec patsnip_spec;
void grn_obj_patsnip_spec_close(grn_ctx *ctx, patsnip_spec *ss);

#define GRN_OBJ2VALUE(o,v,s) ((v) = (o)->u.b.value, (s) = (o)->u.b.size)
#define GRN_VALUE2OBJ(o,v,s) ((o)->u.b.value = (v), (o)->u.b.size = (s))

#define VOIDP(c) ((c) == NIL || !(c) || (c)->header.type == GRN_VOID)
#define OBJECTP(c) ((c)->header.type == GRN_CELL_OBJECT)
#define RECORDSP(c) ((c)->header.type == GRN_TABLE_HASH_KEY)
#define SNIPP(c) ((c)->header.type == GRN_SNIP)
#define BULKP(c) ((c)->header.type == GRN_CELL_STR)
#define PAIRP(c) (((c)->header.type == GRN_CELL_LIST))
#define LISTP(c) (PAIRP(c) || (c) == NIL)
#define INTP(c) ((c)->header.type == GRN_CELL_INT)
#define TABLEP(c) (GRN_TABLE_HASH_KEY <= (c)->header.type && (c)->header.type <= GRN_DB)
#define RAW_TABLEP(c) ((c)->header.type == GRN_TYPE)
#define NATIVE_FUNCP(c) ((c)->header.impl_flags & GRN_CELL_NATIVE)
#define QUERYP(c) ((c)->header.type == GRN_QUERY)
#define RA_COLUMNP(c) ((c)->header.type == GRN_COLUMN_FIX_SIZE)
#define JA_COLUMNP(c) ((c)->header.type == GRN_COLUMN_VAR_SIZE)
#define IDX_COLUMNP(c) ((c)->header.type == GRN_COLUMN_INDEX)
#define COLUMNP(c) (RA_COLUMNP(c) || JA_COLUMNP(c) || IDX_COLUMNP(c))
#define OPP(c) ((c)->header.type == GRN_CELL_OP)
#define STRVALUE(c) ((c)->u.b.value)
#define STRSIZE(c) ((c)->u.b.size)
#define NUMBERP(c) ((c)->header.type == GRN_CELL_INT || (c)->header.type == GRN_CELL_FLOAT)
#define SYMBOLP(c) ((c)->header.impl_flags & GRN_CELL_SYMBOL)
#define CONTINUATIONP(p) ((p)->header.type == GRN_CELL_CONTINUATION)
#define CLOSUREP(p) ((p)->header.type == GRN_CELL_CLOSURE || (p)->header.type == GRN_CELL_MACRO)
#define PROCP(p) ((p)->header.type == GRN_CELL_PROC)
#define PROCEDUREP(p) (PROCP(p) || CLOSUREP(p) || CONTINUATIONP(p) || NATIVE_FUNCP(p))

#define SETINT(c,v) ((c)->header.type = GRN_CELL_INT, (c)->u.i.i = (v))
#define SETFLOAT(c,v) ((c)->header.type = GRN_CELL_FLOAT, (c)->u.d.d = (v))
#define SETBULK(c,v,s) ((c)->header.type = GRN_CELL_STR, (c)->u.b.value = (v), (c)->u.b.size = (s))
#define SETTIME(c,v) ((c)->header.type = GRN_CELL_TIME, memcpy(&(c)->u.tv, v, sizeof(grn_timeval)))

#define CAR(c)          ((c)->u.l.car)
#define CDR(c)          ((c)->u.l.cdr)
#define CAAR(c)         CAR(CAR(c))
#define CADR(c)         CAR(CDR(c))
#define CDAR(c)         CDR(CAR(c))
#define CDDR(c)         CDR(CDR(c))
#define CADDR(c)        CAR(CDDR(c))
#define CADAR(p)        CAR(CDAR(p))
#define CADAAR(p)       CAR(CDR(CAAR(p)))
#define CADDDR(p)       CAR(CDR(CDDR(p)))
#define CDDDDR(p)       CDR(CDR(CDDR(p)))

#define IVALUE(p)       ((p)->u.i.i)
#define FVALUE(p)       ((p)->u.d.d)

#define GRN_CELL_NEW(ctx,o) do {\
  if (!((o) = grn_cell_new(ctx))) { QLERR("obj_new failed"); }\
} while(0)

#define GRN_STR2OBJ(ctx,bulk,o) do {\
  GRN_TEXT_PUTC(ctx, (bulk), '\0'); \
  GRN_CELL_NEW(ctx, (o));\
  (o)->header.impl_flags = GRN_OBJ_ALLOCATED;\
  (o)->header.type = GRN_CELL_STR;\
  (o)->u.b.value = GRN_BULK_HEAD(bulk);\
  (o)->u.b.size = GRN_BULK_VSIZE(bulk) - 1;\
} while(0)

#define CONS(a,b) (grn_cell_cons(ctx, a, b))

#define INTERN(s) (grn_ql_mk_symbol(ctx, s, strlen(s)))

#define POP(x,c) (PAIRP(c) ? ((x) = CAR(c), (c) = CDR(c), (x)) : (x = NIL))

#define SKIPSPACE(c) do {\
  unsigned int len;\
  while ((c)->cur < (c)->str_end && grn_isspace((c)->cur, (c)->encoding)) {\
    if (!(len = grn_charlen(ctx, (c)->cur, (c)->str_end))) { \
      (c)->cur = (c)->str_end;\
      break;\
    }\
    (c)->cur += len;\
  }\
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* GRN_QL_H */
