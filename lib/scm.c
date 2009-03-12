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

/*  Groonga Query Language is based on Mini-Scheme, original credits follow  */

/*
 *      ---------- Mini-Scheme Interpreter Version 0.85 ----------
 *
 *                coded by Atsushi Moriwaki (11/5/1989)
 *
 *            E-MAIL :  moriwaki@kurims.kurims.kyoto-u.ac.jp
 *
 *               THIS SOFTWARE IS IN THE PUBLIC DOMAIN
 *               ------------------------------------
 * This software is completely free to copy, modify and/or re-distribute.
 * But I would appreciate it if you left my name on the code as the author.
 *
 */
/*--
 *
 *  This version has been modified by R.C. Secrist.
 *
 *  Mini-Scheme is now maintained by Akira KIDA.
 *
 *  This is a revised and modified version by Akira KIDA.
 *  current version is 0.85k4 (15 May 1994)
 *
 *  Please send suggestions, bug reports and/or requests to:
 *    <SDI00379@niftyserve.or.jp>
 *--
 */

#include "groonga_in.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ql.h"

#define InitFile "init.scm"

/* global variables */

grn_cell *grn_ql_nil;  /* special cell representing empty cell */
grn_cell *grn_ql_t;    /* special cell representing #t */
grn_cell *grn_ql_f;    /* special cell representing #f */

/* sen query language */

/* todo : update set-car! set-cdr!

inline static void
obj_ref(grn_cell *o)
{
  if (o->nrefs < 0xffff) { o->nrefs++; }
  if (PAIRP(o)) { // todo : check cycle
    if (CAR(o) != NIL) { obj_ref(CAR(o)); }
    if (CDR(o) != NIL) { obj_ref(CDR(o)); }
  }
}

inline static void
obj_unref(grn_cell *o)
{
  if (!o->nrefs) {
    GRN_LOG(ctx, GRN_LOG_ERROR, "o->nrefs corrupt");
    return;
  }
  if (o->nrefs < 0xffff) { o->nrefs--; }
  if (PAIRP(o)) { // todo : check cycle
    if (CAR(o) != NIL) { obj_unref(CAR(o)); }
    if (CDR(o) != NIL) { obj_unref(CDR(o)); }
  }
}

inline static void
rplaca(grn_ctx *ctx, grn_cell *a, grn_cell *b)
{
  if (a->nrefs) {
    ctx->impl->nbinds++;
    if (a->u.l.car) {
      ctx->impl->nunbinds++;
      obj_unref(a->u.l.car);
    }
    if (b) { obj_ref(b); }
  }
  a->u.l.car = b;
}

inline static void
rplacd(grn_ctx *ctx, grn_cell *a, grn_cell *b)
{
  if (a->nrefs) {
    ctx->impl->nbinds++;
    if (a->u.l.cdr) {
      ctx->impl->nunbinds++;
      obj_unref(a->u.l.cdr);
    }
    if (b) { obj_ref(b); }
  }
  a->u.l.cdr = b;
}

*/

grn_rc
grn_obj2int(grn_ctx *ctx, grn_cell *o)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  if (o) {
    switch (o->header.type) {
    case GRN_CELL_STR :
      if (o->u.b.size) {
        const char *end = o->u.b.value + o->u.b.size, *rest;
        int64_t i = grn_atoll(o->u.b.value, end, &rest);
        if (rest == end) {
          grn_cell_clear(ctx, o);
          SETINT(o, i);
          rc = GRN_SUCCESS;
        }
      }
      break;
    case GRN_CELL_INT :
      rc = GRN_SUCCESS;
      break;
    default :
      break;
    }
  }
  return rc;
}

static void
symbol2str(void *x, char *buf)
{
  uint16_t symname_size;
  const char *symname = _grn_hash_strkey_by_val(x, &symname_size);
  memcpy(buf, symname, symname_size);
  buf[symname_size] = '\0';
}

static int
keywordp(void *x)
{
  uint16_t symname_size;
  const char *symname = _grn_hash_strkey_by_val(x, &symname_size);
  return symname_size && *symname == ':';
}

/* get new symbol */
grn_cell *
grn_ql_mk_symbol(grn_ctx *ctx, const char *name, int name_size)
{
  grn_cell *x;
  grn_search_flags f = GRN_TABLE_ADD;
  if (!grn_hash_get(ctx, ctx->impl->symbols, name, name_size, (void **) &x, &f)) {
    return F;
  }
  if (!x->header.impl_flags) {
    x->header.impl_flags |= GRN_CELL_SYMBOL;
    x->header.type = GRN_VOID;
  }
  if (x->header.type == GRN_VOID && ctx->impl->db) {
    uint16_t symname_size;
    const char *symname = _grn_hash_strkey_by_val(x, &symname_size);
    grn_obj *obj = grn_ctx_lookup(ctx, symname, symname_size);
    if (obj) { grn_ql_obj_bind(obj, x); }
  }
  return x;
}

grn_cell *
grn_ql_at(grn_ctx *ctx, const char *key)
{
  grn_cell *o;
  if (!grn_hash_at(ctx, ctx->impl->symbols, key, strlen(key), (void **) &o)) {
    return NULL;
  }
  return o;
}

grn_cell *
grn_ql_def_native_func(grn_ctx *ctx, const char *name, grn_ql_native_func *func)
{
  grn_cell *o = INTERN(name);
  if (o != F) {
    o->header.type = GRN_VOID;
    o->header.impl_flags |= GRN_CELL_NATIVE;
    o->u.o.func = func;
  }
  return o;
}

/*
inline static void
grn_ctx_igc(grn_ctx *ctx)
{
  uint32_t i;
  grn_cell *o;
  grn_set_eh *ep;
  for (i = ctx->impl->lseqno; i != ctx->impl->seqno; i++) {
    if ((ep = grn_set_at(ctx->impl->objects, &i, (void **) &o))) {
      if (ctx->impl->nbinds &&
          (o->nrefs ||
           (BULKP(o) && (o->header.impl_flags & GRN_OBJ_ALLOCATED)))) { continue; }
      grn_cell_clear(ctx, o);
      grn_set_del(ctx->impl->objects, ep);
    }
  }
  ctx->impl->lseqno = ctx->impl->seqno;
  ctx->impl->nbinds = 0;
}
*/

#define MARKP(p)        ((p)->header.impl_flags & GRN_CELL_MARKED)
#define REFERERP(p)     ((p)->header.type & GRN_CELL_LIST)
#define SETREFERER(p)   ((p)->header.type |= GRN_CELL_LIST)
#define UNSETREFERER(p) ((p)->header.type &= ~GRN_CELL_LIST)

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec.3.5) for marking.
 */
inline static void
obj_mark(grn_ctx *ctx, grn_cell *o)
{
  grn_cell *t, *q, *p;
  t = NULL;
  p = o;
  // if (MARKP(o)) { return; }
E2:
  p->header.impl_flags |= GRN_CELL_MARKED;
  // if (!o->nrefs) { GRN_LOG(ctx, GRN_LOG_ERROR, "obj->nrefs corrupt"); }
  if (BULKP(o) && !(o->header.impl_flags & GRN_OBJ_ALLOCATED)) {
    char *b = GRN_MALLOC(o->u.b.size + 1);
    if (b) {
      memcpy(b, o->u.b.value, o->u.b.size);
      b[o->u.b.size] = '\0';
      o->u.b.value = b;
      o->header.impl_flags |= GRN_OBJ_ALLOCATED;
    }
  }
  if (!REFERERP(p)) { goto E6; }
  q = CAR(p);
  if (q && !MARKP(q)) {
    UNSETREFERER(p);
    CAR(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E5:
  q = CDR(p);
  if (q && !MARKP(q)) {
    CDR(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E6:
  if (!t) { return; }
  q = t;
  if (!REFERERP(q)) {
    SETREFERER(q);
    t = CAR(q);
    CAR(q) = p;
    p = q;
    goto E5;
  } else {
    t = CDR(q);
    CDR(q) = p;
    p = q;
    goto E6;
  }
}

#define MARK2P(p)        ((p)->header.impl_flags & GRN_CELL_MARK2)

grn_rc
grn_ql_obj_mark(grn_ctx *ctx, grn_cell *o)
{
  grn_cell *t, *q, *p;
  t = NULL;
  p = o;
  if (MARK2P(o)) { return GRN_INVALID_ARGUMENT; }
E2:
  p->header.impl_flags |= GRN_CELL_MARK2;
  if (!REFERERP(p)) { goto E6; }
  q = CAR(p);
  if (q && !MARK2P(q)) {
    UNSETREFERER(p);
    CAR(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E5:
  q = CDR(p);
  if (q && !MARK2P(q)) {
    CDR(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E6:
  if (!t) { return GRN_SUCCESS; }
  q = t;
  if (!REFERERP(q)) {
    SETREFERER(q);
    t = CAR(q);
    CAR(q) = p;
    p = q;
    goto E5;
  } else {
    t = CDR(q);
    CDR(q) = p;
    p = q;
    goto E6;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ql_obj_unmark(grn_ctx *ctx, grn_cell *o)
{
  grn_cell *t, *q, *p;
  t = NULL;
  p = o;
  if (!MARK2P(o)) { return GRN_INVALID_ARGUMENT; }
E2:
  p->header.impl_flags &= ~GRN_CELL_MARK2;
  if (!REFERERP(p)) { goto E6; }
  q = CAR(p);
  if (q && MARK2P(q)) {
    UNSETREFERER(p);
    CAR(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E5:
  q = CDR(p);
  if (q && MARK2P(q)) {
    CDR(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E6:
  if (!t) { return GRN_SUCCESS; }
  q = t;
  if (!REFERERP(q)) {
    SETREFERER(q);
    t = CAR(q);
    CAR(q) = p;
    p = q;
    goto E5;
  } else {
    t = CDR(q);
    CDR(q) = p;
    p = q;
    goto E6;
  }
  return GRN_SUCCESS;
}

inline static grn_rc
grn_ctx_mgc(grn_ctx *ctx)
{
  /*
  if (!(sc = grn_set_cursor_open(ctx->impl->symbols))) { return GRN_NO_MEMORY_AVAILABLE; }
  {
    grn_cell *o;
    while (grn_set_cursor_next(sc, NULL, (void **) &o)) { obj_mark(o); }
    grn_set_cursor_close(sc);
  }
  */
  obj_mark(ctx, ctx->impl->global_env);

  /* mark current registers */
  obj_mark(ctx, ctx->impl->args);
  obj_mark(ctx, ctx->impl->envir);
  obj_mark(ctx, ctx->impl->code);
  obj_mark(ctx, ctx->impl->dump);
  obj_mark(ctx, ctx->impl->value);
  obj_mark(ctx, ctx->impl->phs);

  ctx->impl->n_entries = 0;
  {
    grn_cell *o;
    GRN_ARRAY_EACH(ctx->impl->objects, 0, 0, id, &o, {
      if (o->header.impl_flags & (GRN_CELL_MARKED|GRN_CELL_MARK2)) {
        o->header.impl_flags &= ~GRN_CELL_MARKED;
        ctx->impl->n_entries++;
      } else {
        grn_cell_clear(ctx, o);
        grn_array_delete_by_id(ctx, ctx->impl->objects, id, NULL);
      }
    });
  }
  {
    grn_tmp_db_obj *o;
    GRN_ARRAY_EACH(ctx->impl->values, 0, 0, id, &o, {
      if (o->cell.header.impl_flags & (GRN_CELL_MARKED|GRN_CELL_MARK2)) {
        o->cell.header.impl_flags &= ~GRN_CELL_MARKED;
        ctx->impl->n_entries++;
      } else {
        grn_obj_close(ctx, (grn_obj *)o->obj);
      }
    });
  }
  ctx->impl->lseqno = ctx->impl->seqno;
  ctx->impl->nbinds = 0;
  ctx->impl->nunbinds = 0;
  return GRN_SUCCESS;
}

inline static void Eval_Cycle(grn_ctx *ctx);

/* ========== Evaluation Cycle ========== */

/* operator code */

enum {
  OP_T0LVL = GRN_OP_T0LVL,
  OP_ERR0 = GRN_OP_ERR0,
  OP_LOAD,
  OP_T1LVL,
  OP_READ,
  OP_VALUEPRINT,
  OP_EVAL,
  OP_E0ARGS,
  OP_E1ARGS,
  OP_APPLY,
  OP_DOMACRO,
  OP_LAMBDA,
  OP_QUOTE,
  OP_DEF0,
  OP_DEF1,
  OP_BEGIN,
  OP_IF0,
  OP_IF1,
  OP_SET0,
  OP_SET1,
  OP_LET0,
  OP_LET1,
  OP_LET2,
  OP_LET0AST,
  OP_LET1AST,
  OP_LET2AST,
  OP_LET0REC,
  OP_LET1REC,
  OP_LET2REC,
  OP_COND0,
  OP_COND1,
  OP_DELAY,
  OP_AND0,
  OP_AND1,
  OP_OR0,
  OP_OR1,
  OP_C0STREAM,
  OP_C1STREAM,
  OP_0MACRO,
  OP_1MACRO,
  OP_CASE0,
  OP_CASE1,
  OP_CASE2,
  OP_PEVAL,
  OP_PAPPLY,
  OP_CONTINUATION,
  OP_SETCAR,
  OP_SETCDR,
  OP_FORCE,
  OP_ERR1,
  OP_PUT,
  OP_GET,
  OP_QUIT,
  OP_SDOWN,
  OP_RDSEXPR,
  OP_RDLIST,
  OP_RDDOT,
  OP_RDQUOTE,
  OP_RDQQUOTE,
  OP_RDUNQUOTE,
  OP_RDUQTSP,
  OP_NATIVE,
  OP_QQUOTE0,
  OP_QQUOTE1,
  OP_QQUOTE2
};

grn_cell *
grn_ql_feed(grn_ctx *ctx, char *str, uint32_t str_size, int mode)
{
  if (GRN_QL_WAITINGP(ctx)) {
    GRN_BULK_REWIND(&ctx->impl->outbuf);
    GRN_BULK_REWIND(&ctx->impl->subbuf);
    ctx->impl->bufcur = 0;
  }
  for (;;) {
    switch (ctx->stat) {
    case GRN_QL_TOPLEVEL :
      ctx->impl->co.mode &= ~GRN_QL_HEAD;
      Eval_Cycle(ctx);
      break;
    case GRN_QL_WAIT_EXPR :
      ctx->impl->co.mode = mode;
      ctx->impl->cur = str;
      ctx->impl->str_end = str + str_size;
      Eval_Cycle(ctx);
      break;
    case GRN_QL_WAIT_ARG :
      ctx->impl->co.mode = mode;
      if ((mode & GRN_QL_HEAD)) {
        ctx->impl->cur = str;
        ctx->impl->str_end = str + str_size;
      } else {
        char *buf;
        grn_cell *ph = CAR(ctx->impl->phs);
        if (!(buf = GRN_MALLOC(str_size + 1))) {
          return NIL;
        }
        memcpy(buf, str, str_size);
        buf[str_size] = '\0';
        ph->header.impl_flags |= GRN_OBJ_ALLOCATED;
        ph->u.b.value = buf;
        ph->u.b.size = str_size;
        ctx->impl->phs = CDR(ctx->impl->phs);
      }
      if ((ctx->impl->phs == NIL) || (mode & (GRN_QL_HEAD|GRN_QL_TAIL))) {
        ctx->stat = GRN_QL_EVAL;
      }
      break;
    case GRN_QL_EVAL :
      Eval_Cycle(ctx);
      break;
    case GRN_QL_WAIT_DATA :
      ctx->impl->co.mode = mode;
      if ((mode & GRN_QL_HEAD)) {
        ctx->impl->args = NIL;
        ctx->impl->cur = str;
        ctx->impl->str_end = str + str_size;
      } else {
        ctx->impl->arg.u.b.value = str;
        ctx->impl->arg.u.b.size = str_size;
        ctx->impl->arg.header.type = GRN_CELL_STR;
        ctx->impl->args = &ctx->impl->arg;
      }
      /* fall through */
    case GRN_QL_NATIVE :
      GRN_ASSERT(ctx->impl->co.func);
      ctx->impl->value = ctx->impl->co.func(ctx, ctx->impl->args, &ctx->impl->co);
      if (ERRP(ctx, GRN_ERROR)) { ctx->stat = GRN_QL_QUITTING; return F; }
      ERRCLR(ctx);
      if (ctx->impl->co.last && !(ctx->impl->co.mode & (GRN_QL_HEAD|GRN_QL_TAIL))) {
        ctx->stat = GRN_QL_WAIT_DATA;
      } else {
        ctx->impl->co.mode = 0;
        Eval_Cycle(ctx);
      }
      break;
    case GRN_QL_QUITTING :
    case GRN_QL_QUIT :
      return NIL;
    }
    if (ERRP(ctx, GRN_ERROR)) { ctx->stat = GRN_QL_QUITTING; return F; }
    if (GRN_QL_WAITINGP(ctx)) { /* waiting input data */
      if (ctx->impl->inbuf) {
        GRN_FREE(ctx->impl->inbuf);
        ctx->impl->inbuf = NULL;
      }
      break;
    }
    if ((ctx->stat & 0x40) && GRN_QL_GET_MODE(ctx) == grn_ql_step) {
      break;
    }
  }
  return NIL;
}

/**** sexp parser ****/

inline static void
skipline(grn_ctx *ctx)
{
  while (ctx->impl->cur < ctx->impl->str_end) {
    if (*ctx->impl->cur++ == '\n') { break; }
  }
}

/*************** scheme interpreter ***************/

# define BACKQUOTE '`'

#include <stdio.h>
#include <ctype.h>

/* macros for cell operations */
#define HASPROP(p)       ((p)->header.impl_flags & GRN_CELL_SYMBOL)
#define SYMPROP(p)       CDR(p)
#define SYNTAXP(p)       ((p)->header.type == GRN_CELL_SYNTAX)
#define SYNTAXNUM(p)     ((p)->header.domain)
#define PROCNUM(p)       IVALUE(p)
#define MACROP(p)        ((p)->header.type == GRN_CELL_MACRO)
#define CLOSURE_CODE(p)  CAR(p)
#define CLOSURE_ENV(p)   CDR(p)
#define CONT_DUMP(p)     CDR(p)
#define PROMISEP(p)      ((p)->header.impl_flags & GRN_CELL_PROMISE)
#define SETPROMISE(p)    (p)->header.impl_flags |= GRN_CELL_PROMISE
#define LAMBDA           (INTERN("lambda"))
#define QUOTE            (INTERN("quote"))
#define QQUOTE           (INTERN("quasiquote"))
#define UNQUOTE          (INTERN("unquote"))
#define UNQUOTESP        (INTERN("unquote-splicing"))

/* get new cell.  parameter a, b is marked by gc. */
#define GET_CELL(ctx,a,b,o) GRN_CELL_NEW(ctx, o)

/* get number atom */
inline static grn_cell *
mk_number(grn_ctx *ctx, int64_t num)
{
  grn_cell *x;
  GRN_CELL_NEW(ctx, x);
  SETINT(x, num);
  return x;
}

/* get new string */
grn_cell *
grn_ql_mk_string(grn_ctx *ctx, const char *str, unsigned int len)
{
  grn_cell *x = grn_cell_alloc(ctx, len);
  if (!x) { return F; }
  memcpy(x->u.b.value, str, len);
  x->u.b.value[len] = '\0';
  return x;
}

inline static grn_cell *
mk_const_string(grn_ctx *ctx, const char *str)
{
  grn_cell *x;
  GRN_CELL_NEW(ctx, x);
  x->header.impl_flags = 0;
  x->header.type = GRN_CELL_STR;
  x->u.b.value = (char *)str;
  x->u.b.size = strlen(str);
  return x;
}

grn_cell *
grn_ql_mk_symbol2(grn_ctx *ctx, const char *q, unsigned int len, int kwdp)
{
  char buf[GRN_TABLE_MAX_KEY_SIZE], *p = buf;
  if (len + 1 >= GRN_TABLE_MAX_KEY_SIZE) { QLERR("too long symbol"); }
  if (kwdp) {
    *p++ = ':';
    memcpy(p, q, len);
    len++;
  } else {
    memcpy(p, q, len);
  }
  return grn_ql_mk_symbol(ctx, buf, len);
}

static grn_cell *
str2num(grn_ctx *ctx, char *str, unsigned int len)
{
  const char *cur, *str_end = str + len;
  int64_t i = grn_atoll(str, str_end, &cur);
  if (cur == str_end) { return mk_number(ctx, i); }
  if (cur != str) { /* todo : support #i notation */
    char *end, buf0[128], *buf = len < 128 ? buf0 : GRN_MALLOC(len + 1);
    if (buf) {
      double d;
      memcpy(buf, str, len);
      buf[len] = '\0';
      errno = 0;
      d = strtod(buf, &end);
      if (!(len < 128)) { GRN_FREE(buf); }
      if (!errno && buf + len == end) {
        grn_cell *x;
        GRN_CELL_NEW(ctx, x);
        SETFLOAT(x, d);
        return x;
      }
    }
  }
  return NIL;
}

/* make symbol or number atom from string */
static grn_cell *
mk_atom(grn_ctx *ctx, char *str, unsigned int len, grn_cell *v)
{
  grn_cell **vp = &v, *p;
  const char *cur, *last, *str_end = str + len;
  if ((p = str2num(ctx, str, len)) != NIL) { return p; }
  for (last = cur = str; cur < str_end; cur += len) {
    if (!(len = grn_charlen(ctx, cur, str_end, ctx->encoding))) { break; }
    if (*cur == '.') {
      if (last < cur) { *vp = grn_ql_mk_symbol2(ctx, last, cur - last, str != last); }
      v = CONS(v, CONS(NIL, NIL));
      vp = &CADR(v);
      last = cur + 1;
    }
  }
  if (last < cur) { *vp = grn_ql_mk_symbol2(ctx, last, cur - last, str != last); }
  return v;
}

/* make constant */
inline static grn_cell *
mk_const(grn_ctx *ctx, char *name, unsigned int len)
{
  char    tmp[256];
  char    tmp2[256];
  /* todo : rewirte with grn_str_* functions */
  if (len == 1) {
    if (*name == 't') {
      return T;
    } else if (*name == 'f') {
      return F;
    }
  } else if (len > 1) {
    if (*name == 'p' && name[1] == '<' && name[12] == '>') {/* #p (GRN_CELL_OBJECT) */
      grn_id cls = grn_btoi(name + 2);
      if (cls) {
        grn_id self = grn_btoi(name + 7);
        if (self) {
          grn_cell * v = grn_ql_obj_new(ctx, cls, self);
          if (len > 13 && name[13] == '.') {
            return mk_atom(ctx, name + 13, len - 13, v);
          } else {
            return v;
          }
        }
      }
    } else if (*name == ':' && name[1] == '<') {/* #: (GRN_CELL_TIME) */
      grn_cell *x;
      grn_timeval tv;
      const char *cur;
      tv.tv_sec = grn_atoi(name + 2, name + len, &cur);
      if (cur >= name + len || *cur != '.') {
        QLERR("illegal time format '%s'", name);
      }
      tv.tv_usec = grn_atoi(cur + 1, name + len, &cur);
      if (cur >= name + len || *cur != '>') {
        QLERR("illegal time format '%s'", name);
      }
      GRN_CELL_NEW(ctx, x);
      SETTIME(x, &tv);
      return x;
    } else if (*name == 'o') {/* #o (octal) */
      long long unsigned int x;
      len = (len > 255) ? 255 : len - 1;
      memcpy(tmp2, name + 1, len);
      tmp2[len] = '\0';
      sprintf(tmp, "0%s", tmp2);
      sscanf(tmp, "%Lo", &x);
      return mk_number(ctx, x);
    } else if (*name == 'd') {  /* #d (decimal) */
      long long int x;
      sscanf(&name[1], "%Ld", &x);
      return mk_number(ctx, x);
    } else if (*name == 'x') {  /* #x (hex) */
      long long unsigned int x;
      len = (len > 255) ? 255 : len - 1;
      memcpy(tmp2, name + 1, len);
      tmp2[len] = '\0';
      sprintf(tmp, "0x%s", tmp2);
      sscanf(tmp, "%Lx", &x);
      return mk_number(ctx, x);
    }
  }
  return NIL;
}

grn_rc
grn_ql_load(grn_ctx *ctx, const char *filename)
{
  if (!ctx || !ctx->impl) { return GRN_INVALID_ARGUMENT; }
  if (!filename) { filename = InitFile; }
  ctx->impl->args = CONS(mk_const_string(ctx, filename), NIL);
  ctx->stat = GRN_QL_TOPLEVEL;
  ctx->impl->op = OP_LOAD;
  grn_ql_feed(ctx, "init", 4, 0);
  return ctx->rc;
}

/* ========== Routines for Reading ========== */

#define TOK_LPAREN  0
#define TOK_RPAREN  1
#define TOK_DOT     2
#define TOK_ATOM    3
#define TOK_QUOTE   4
#define TOK_COMMENT 5
#define TOK_DQUOTE  6
#define TOK_BQUOTE  7
#define TOK_COMMA   8
#define TOK_ATMARK  9
#define TOK_SHARP   10
#define TOK_EOS     11
#define TOK_QUESTION 12

#define lparenp(c) ((c) == '(' || (c) == '[')
#define rparenp(c) ((c) == ')' || (c) == ']')

/* read chacters to delimiter */
inline static char
readstr(grn_ctx *ctx, char **str, unsigned int *size)
{
  char *start, *end;
  for (start = end = ctx->impl->cur;;) {
    unsigned int len;
    /* null check and length check */
    if (!(len = grn_charlen(ctx, end, ctx->impl->str_end, ctx->encoding))) {
      ctx->impl->cur = ctx->impl->str_end;
      break;
    }
    if (grn_isspace(end, ctx->encoding) ||
        *end == ';' || lparenp(*end) || rparenp(*end)) {
      ctx->impl->cur = end;
      break;
    }
    end += len;
  }
  if (start < end || ctx->impl->cur < ctx->impl->str_end) {
    *str = start;
    *size = (unsigned int)(end - start);
    return TOK_ATOM;
  } else {
    return TOK_EOS;
  }
}

/* read string expression "xxx...xxx" */
inline static char
readstrexp(grn_ctx *ctx, char **str, unsigned int *size)
{
  char *start, *src, *dest;
  for (start = src = dest = ctx->impl->cur;;) {
    unsigned int len;
    /* null check and length check */
    if (!(len = grn_charlen(ctx, src, ctx->impl->str_end, ctx->encoding))) {
      ctx->impl->cur = ctx->impl->str_end;
      if (start < dest) {
        *str = start;
        *size = (unsigned int)(dest - start);
        return TOK_ATOM;
      }
      return TOK_EOS;
    }
    if (src[0] == '"' && len == 1) {
      ctx->impl->cur = src + 1;
      *str = start;
      *size = (unsigned int)(dest - start);
      return TOK_ATOM;
    } else if (src[0] == '\\' && src + 1 < ctx->impl->str_end && len == 1) {
      src++;
      switch (*src) {
      case 'n' :
        *dest++ = '\n';
        break;
      case 'r' :
        *dest++ = '\r';
        break;
      case 't' :
        *dest++ = '\t';
        break;
      default :
        *dest++ = *src;
        break;
      }
      src++;
    } else {
      while (len--) { *dest++ = *src++; }
    }
  }
}

/* get token */
inline static char
token(grn_ctx *ctx)
{
  SKIPSPACE(ctx->impl);
  if (ctx->impl->cur >= ctx->impl->str_end) { return TOK_EOS; }
  switch (*ctx->impl->cur) {
  case '(':
  case '[':
    ctx->impl->cur++;
    return TOK_LPAREN;
  case ')':
  case ']':
    ctx->impl->cur++;
    return TOK_RPAREN;
  case '.':
    ctx->impl->cur++;
    if (ctx->impl->cur == ctx->impl->str_end ||
        grn_isspace(ctx->impl->cur, ctx->encoding) ||
        *ctx->impl->cur == ';' || lparenp(*ctx->impl->cur) || rparenp(*ctx->impl->cur)) {
      return TOK_DOT;
    } else {
      ctx->impl->cur--;
      return TOK_ATOM;
    }
  case '\'':
    ctx->impl->cur++;
    return TOK_QUOTE;
  case ';':
    ctx->impl->cur++;
    return TOK_COMMENT;
  case '"':
    ctx->impl->cur++;
    return TOK_DQUOTE;
  case BACKQUOTE:
    ctx->impl->cur++;
    return TOK_BQUOTE;
  case ',':
    ctx->impl->cur++;
    if (ctx->impl->cur < ctx->impl->str_end && *ctx->impl->cur == '@') {
      ctx->impl->cur++;
      return TOK_ATMARK;
    } else {
      return TOK_COMMA;
    }
  case '#':
    ctx->impl->cur++;
    return TOK_SHARP;
  case '?':
    ctx->impl->cur++;
    return TOK_QUESTION;
  default:
    return TOK_ATOM;
  }
}

/* ========== Routines for Printing ========== */
#define  ok_abbrev(x)  (PAIRP(x) && CDR(x) == NIL)

void
grn_obj_inspect(grn_ctx *ctx, grn_cell *obj, grn_obj *buf, int flags)
{
  if (!obj) {
    GRN_BULK_PUTS(ctx, buf, "NULL");
  } else if (obj == NIL) {
    GRN_BULK_PUTS(ctx, buf, "()");
  } else if (obj == T) {
    GRN_BULK_PUTS(ctx, buf, "#t");
  } else if (obj == F) {
    GRN_BULK_PUTS(ctx, buf, "#f");
  } else {
    if (SYMBOLP(obj)) {
      char b[GRN_TABLE_MAX_KEY_SIZE + 1];
      symbol2str(obj, b);
      if (flags & GRN_OBJ_INSPECT_SYMBOL_AS_STR) {
        grn_bulk_esc(ctx, buf, (*b == ':') ? b + 1 : b,
                     strlen(b) - (*b == ':') ? 1 : 0, ctx->encoding);
      } else {
        GRN_BULK_PUTS(ctx, buf, b);
      }
      return;
    }
    switch (obj->header.type) {
    case GRN_VOID :
      if (SYMBOLP(obj)) {
        char b[GRN_TABLE_MAX_KEY_SIZE + 1];
        symbol2str(obj, b);
        GRN_BULK_PUTS(ctx, buf, b);
      } else {
        GRN_BULK_PUTS(ctx, buf, "#<VOID>");
      }
      break;
    case GRN_CELL_OBJECT :
      if (flags & GRN_OBJ_INSPECT_ESC) {
        GRN_BULK_PUTS(ctx, buf, "#p<");
        grn_bulk_itob(ctx, buf, obj->header.domain);
        grn_bulk_itob(ctx, buf, obj->u.o.id);
        GRN_BULK_PUTC(ctx, buf, '>');
      } else {
        grn_ql_obj_key(ctx, obj, buf);
      }
      break;
    case GRN_SNIP :
    case GRN_PATSNIP :
      GRN_BULK_PUTS(ctx, buf, "#<SNIP>");
      break;
    case GRN_CELL_STR :
      if (flags & GRN_OBJ_INSPECT_ESC) {
        grn_bulk_esc(ctx, buf, STRVALUE(obj), STRSIZE(obj), ctx->encoding);
      } else {
        grn_bulk_write(ctx, buf, STRVALUE(obj), STRSIZE(obj));
      }
      break;
    case GRN_CELL_INT :
      grn_bulk_lltoa(ctx, buf, IVALUE(obj));
      break;
    case GRN_CELL_FLOAT :
      grn_bulk_ftoa(ctx, buf, FVALUE(obj));
      break;
    case GRN_CELL_TIME :
      GRN_BULK_PUTS(ctx, buf, "#:<");
      grn_bulk_itoa(ctx, buf, obj->u.tv.tv_sec);
      GRN_BULK_PUTS(ctx, buf, ".");
      grn_bulk_itoa(ctx, buf, obj->u.tv.tv_usec);
      GRN_BULK_PUTC(ctx, buf, '>');
      break;
    case GRN_QUERY :
      GRN_BULK_PUTS(ctx, buf, "#<QUERY>");
      break;
    case GRN_VECTOR :
      GRN_BULK_PUTS(ctx, buf, "#<VECTOR>");
      break;
    case GRN_UVECTOR :
      GRN_BULK_PUTS(ctx, buf, "#<UVECTOR>");
      break;
    case GRN_CELL_OP :
      GRN_BULK_PUTS(ctx, buf, "#<OP>");
      break;
    case GRN_CELL_SYNTAX :
      GRN_BULK_PUTS(ctx, buf, "#<SYNTAX>");
      break;
    case GRN_CELL_PROC :
      GRN_BULK_PUTS(ctx, buf, "#<PROCEDURE ");
      grn_bulk_itoa(ctx, buf, PROCNUM(obj));
      GRN_BULK_PUTS(ctx, buf, ">");
      break;
    case GRN_CELL_MACRO :
      GRN_BULK_PUTS(ctx, buf, "#<MACRO>");
      break;
    case GRN_CELL_CLOSURE :
      GRN_BULK_PUTS(ctx, buf, "#<CLOSURE>");
      break;
    case GRN_CELL_CONTINUATION :
      GRN_BULK_PUTS(ctx, buf, "#<CONTINUATION>");
      break;
    case GRN_TYPE :
      GRN_BULK_PUTS(ctx, buf, "#<TYPE>");
      break;
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_NO_KEY :
    case GRN_TABLE_PAT_KEY :
      GRN_BULK_PUTS(ctx, buf, "#<TABLE>");
      break;
    case GRN_COLUMN_FIX_SIZE :
      GRN_BULK_PUTS(ctx, buf, "#<RA_COLUMN>");
      break;
    case GRN_COLUMN_VAR_SIZE :
      GRN_BULK_PUTS(ctx, buf, "#<JA_COLUMN>");
      break;
    case GRN_COLUMN_INDEX :
      GRN_BULK_PUTS(ctx, buf, "#<IDX_COLUMN>");
      break;
    case GRN_CELL_LIST :
      /* todo : detect loop */
      if (CAR(obj) == QUOTE && ok_abbrev(CDR(obj))) {
        GRN_BULK_PUTC(ctx, buf, '\'');
        grn_obj_inspect(ctx, CADR(obj), buf, flags);
      } else if (CAR(obj) == QQUOTE && ok_abbrev(CDR(obj))) {
        GRN_BULK_PUTC(ctx, buf, '`');
        grn_obj_inspect(ctx, CADR(obj), buf, flags);
      } else if (CAR(obj) == UNQUOTE && ok_abbrev(CDR(obj))) {
        GRN_BULK_PUTC(ctx, buf, ',');
        grn_obj_inspect(ctx, CADR(obj), buf, flags);
      } else if (CAR(obj) == UNQUOTESP && ok_abbrev(CDR(obj))) {
        GRN_BULK_PUTS(ctx, buf, ",@");
        grn_obj_inspect(ctx, CADR(obj), buf, flags);
      } else {
        GRN_BULK_PUTC(ctx, buf, '(');
        for (;;) {
          grn_obj_inspect(ctx, CAR(obj), buf, flags);
          if ((obj = CDR(obj)) && (obj != NIL)) {
            if (PAIRP(obj)) {
              GRN_BULK_PUTC(ctx, buf, ' ');
            } else {
              GRN_BULK_PUTS(ctx, buf, " . ");
              grn_obj_inspect(ctx, obj, buf, flags);
              GRN_BULK_PUTC(ctx, buf, ')');
              break;
            }
          } else {
            GRN_BULK_PUTC(ctx, buf, ')');
            break;
          }
        }
      }
      break;
    default :
      if (SYMBOLP(obj)) {
        char b[GRN_TABLE_MAX_KEY_SIZE + 1];
        symbol2str(obj, b);
        GRN_BULK_PUTS(ctx, buf, b);
      } else {
        GRN_BULK_PUTS(ctx, buf, "#<?(");
        grn_bulk_itoa(ctx, buf, obj->header.type);
        GRN_BULK_PUTS(ctx, buf, ")?>");
      }
      break;
    }
  }
}

/* ========== Routines for Evaluation Cycle ========== */

/* make closure. c is code. e is environment */
inline static grn_cell *
mk_closure(grn_ctx *ctx, grn_cell *c, grn_cell *e)
{
  grn_cell *x;
  GET_CELL(ctx, c, e, x);
  x->header.type = GRN_CELL_CLOSURE;
  x->header.impl_flags = 0;
  CAR(x) = c;
  CDR(x) = e;
  return x;
}

/* make continuation. */
inline static grn_cell *
mk_continuation(grn_ctx *ctx, grn_cell *d)
{
  grn_cell *x;
  GET_CELL(ctx, NIL, d, x);
  x->header.type = GRN_CELL_CONTINUATION;
  x->header.impl_flags = 0;
  CONT_DUMP(x) = d;
  return x;
}

/* reverse list -- make new cells */
inline static grn_cell *
reverse(grn_ctx *ctx, grn_cell *a) /* a must be checked by gc */
{
  grn_cell *p = NIL;
  for ( ; PAIRP(a); a = CDR(a)) {
    p = CONS(CAR(a), p);
    if (ERRP(ctx, GRN_ERROR)) { return F; }
  }
  return p;
}

/* reverse list --- no make new cells */
inline static grn_cell *
non_alloc_rev(grn_cell *term, grn_cell *list)
{
  grn_cell *p = list, *result = term, *q;
  while (p != NIL) {
    q = CDR(p);
    CDR(p) = result;
    result = p;
    p = q;
  }
  return result;
}

/* append list -- make new cells */
inline static grn_cell *
append(grn_ctx *ctx, grn_cell *a, grn_cell *b)
{
  grn_cell *p = b, *q;
  if (a != NIL) {
    a = reverse(ctx, a);
    if (ERRP(ctx, GRN_ERROR)) { return F; }
    while (a != NIL) {
      q = CDR(a);
      CDR(a) = p;
      p = a;
      a = q;
    }
  }
  return p;
}

/* equivalence of atoms */
inline static int
eqv(grn_cell *a, grn_cell *b)
{
  if (a == b) { return 1; }
  if (a->header.type != b->header.type) { return 0; }
  switch (a->header.type) {
  case GRN_CELL_OBJECT :
    return (a->u.o.id == b->u.o.id && a->header.domain == b->header.domain);
    break;
  case GRN_CELL_STR :
    return (a->u.b.size == b->u.b.size &&
            !memcmp(a->u.b.value, b->u.b.value, a->u.b.size));
    break;
  case GRN_CELL_INT :
    return (IVALUE(a) == IVALUE(b));
    break;
  case GRN_CELL_FLOAT :
    return !islessgreater(FVALUE(a), FVALUE(b));
    break;
  case GRN_CELL_TIME :
    return (!memcmp(&a->u.tv, &b->u.tv, sizeof(grn_timeval)));
    break;
  default :
    /* todo : support other types */
    return 0;
    break;
  }
}

/* expr (&) by int atoms */
inline static int
logtest(grn_cell *a, grn_cell *b)
{
  if (a == b) { return 1; }
  if (a->header.type != b->header.type) { return 0; }
  switch (a->header.type) {
  case GRN_CELL_INT :
    return (IVALUE(a) & IVALUE(b) ? 1 : 0);
    break;
  default :
    return 0;
    break;
  }
}
/* true or false value macro */
#define istrue(p)       ((p) != NIL && (p) != F)
#define isfalse(p)      ((p) == F)

/* control macros for Eval_Cycle */
#define s_goto(ctx,a) do {\
  ctx->impl->op = (a);\
  return T;\
} while (0)

#define s_save(ctx,a,b,args) (\
    ctx->impl->dump = CONS(ctx->impl->envir, CONS((args), ctx->impl->dump)),\
    ctx->impl->dump = CONS((b), ctx->impl->dump),\
    ctx->impl->dump = CONS(mk_number(ctx, (int64_t)(a)), ctx->impl->dump))

#define s_return(ctx,a) do {\
    ctx->impl->value = (a);\
    ctx->impl->op = IVALUE(CAR(ctx->impl->dump));\
    ctx->impl->args = CADR(ctx->impl->dump);\
    ctx->impl->envir = CADDR(ctx->impl->dump);\
    ctx->impl->code = CADDDR(ctx->impl->dump);\
    ctx->impl->dump = CDDDDR(ctx->impl->dump);\
    return T;\
} while (0)

#define RTN_NIL_IF_HEAD(ctx) do {\
  if (((ctx)->impl->co.mode & GRN_QL_HEAD)) { s_goto(ctx, OP_T0LVL); }\
} while (0)

#define RTN_NIL_IF_TAIL(ctx) do {\
  if (((ctx)->impl->co.mode & GRN_QL_TAIL)) { s_return((ctx), NIL); } else { return NIL; }\
} while (0)

static grn_cell *
list_deep_copy(grn_ctx *ctx, grn_cell *c) {
  /* NOTE: only list is copied */
  if (PAIRP(c)) {
    /* TODO: convert recursion to loop */
    return CONS(list_deep_copy(ctx, CAR(c)), list_deep_copy(ctx, CDR(c)));
  } else {
    return c;
  }
}

static void
qquote_uquotelist(grn_ctx *ctx, grn_cell *cl, grn_cell *pcl, int level) {
  /* reverse list */
  grn_cell *x, *y;
  while (PAIRP(cl)) {
    x = CAR(cl);
    if (PAIRP(x)) {
      y = CAR(x);
      if (y == UNQUOTE) {
        if (level) {
          qquote_uquotelist(ctx, CDR(x), x, level - 1);
        } else {
          CDR(ctx->impl->args) = CONS(cl, CDR(ctx->impl->args)); /* save (unquote ...) cell */
        }
      } else if (y == UNQUOTESP) {
        if (level) {
          qquote_uquotelist(ctx, CDR(x), x, level - 1);
        } else {
          CDR(ctx->impl->args) = CONS(pcl, CDR(ctx->impl->args)); /* save pre (unquote-splicing) cell */
        }
      } else {
        qquote_uquotelist(ctx, x, cl, level);
      }
    } else if (x == QQUOTE) {
      qquote_uquotelist(ctx, CDR(cl), cl, level + 1);
      return;
    }
    if (!level && CADR(cl) == UNQUOTE) {
      CDR(ctx->impl->args) = CONS(cl, CDR(ctx->impl->args)); /* save (a . ,b) cell */
      return;
    }
    pcl = cl;
    cl = CDR(cl);
  }
}

#define GC_THRESHOLD 1000000

static grn_cell *
opexe(grn_ctx *ctx)
{
  register grn_cell *x, *y;
  if (ctx->impl->op == OP_T0LVL || ctx->impl->n_entries > ctx->impl->ncells + GC_THRESHOLD) {
    if (ctx->impl->gc_verbose) {
      grn_obj buf;
      grn_bulk_init(ctx, &buf, 0);
      grn_obj_inspect(ctx, ctx->impl->envir, &buf, GRN_OBJ_INSPECT_ESC);
      *buf.u.b.curr = '\0';
      GRN_LOG(ctx, GRN_LOG_NOTICE, "mgc > ncells=%d envir=<%s>", ctx->impl->n_entries, buf.u.b.head);
      grn_bulk_fin(ctx, &buf);
    }
    grn_ctx_mgc(ctx);
    if (ctx->impl->gc_verbose) {
      GRN_LOG(ctx, GRN_LOG_NOTICE, "mgc < ncells=%d", ctx->impl->n_entries);
    }
    ctx->impl->ncells = ctx->impl->n_entries;
  }
  switch (ctx->impl->op) {
  case OP_LOAD:    /* load */
    if (BULKP(CAR(ctx->impl->args))) {
      struct stat st;
      char *fname = STRVALUE(CAR(ctx->impl->args));
      if (fname && !stat(fname, &st)) {
        if (ctx->impl->inbuf) { GRN_FREE(ctx->impl->inbuf); }
        if ((ctx->impl->inbuf = GRN_MALLOC(st.st_size))) {
          int fd;
          if ((fd = open(fname, O_RDONLY)) != -1) {
            if (read(fd, ctx->impl->inbuf, st.st_size) == st.st_size) {
              GRN_BULK_PUTS(ctx, &ctx->impl->outbuf, "loading ");
              GRN_BULK_PUTS(ctx, &ctx->impl->outbuf, STRVALUE(CAR(ctx->impl->args)));
              ctx->impl->cur = ctx->impl->inbuf;
              ctx->impl->str_end = ctx->impl->inbuf + st.st_size;
            }
            close(fd);
          }
          if (ctx->impl->cur != ctx->impl->inbuf) {
            GRN_FREE(ctx->impl->inbuf);
            ctx->impl->inbuf = NULL;
          }
        }
      }
    }
    s_goto(ctx, OP_T0LVL);

  case OP_T0LVL:  /* top level */
    ctx->impl->dump = NIL;
    ctx->impl->envir = ctx->impl->global_env;
    if (ctx->impl->batchmode) {
      s_save(ctx, OP_T0LVL, NIL, NIL);
    } else {
      s_save(ctx, OP_VALUEPRINT, NIL, NIL);
    }
    s_save(ctx, OP_T1LVL, NIL, NIL);
    ctx->impl->pht = &ctx->impl->phs;
    *ctx->impl->pht = NIL;
    s_goto(ctx, OP_READ);

  case OP_T1LVL:  /* top level */
    // verbose check?
    if (ctx->impl->phs != NIL &&
        !(ctx->impl->co.mode & (GRN_QL_HEAD|GRN_QL_TAIL))) { RTN_NIL_IF_TAIL(ctx); }
    // GRN_BULK_PUTC(ctx, &ctx->impl->outbuf, '\n');
    ctx->impl->code = ctx->impl->value;
    s_goto(ctx, OP_EVAL);

  case OP_READ:    /* read */
    RTN_NIL_IF_HEAD(ctx);
    if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
    s_goto(ctx, OP_RDSEXPR);

  case OP_VALUEPRINT:  /* print evalution result */
    ctx->impl->args = ctx->impl->value;
    s_save(ctx, OP_T0LVL, NIL, NIL);
    grn_obj_inspect(ctx, ctx->impl->args, &ctx->impl->outbuf, GRN_OBJ_INSPECT_ESC);
    s_return(ctx, T);

  case OP_EVAL:    /* main part of evalution */
    // fixme : quick hack.
    if (SYMBOLP(ctx->impl->code)) {  /* symbol */
      if (keywordp(ctx->impl->code)) { s_return(ctx, ctx->impl->code); }
      for (x = ctx->impl->envir; x != NIL; x = CDR(x)) {
        for (y = CAR(x); y != NIL; y = CDR(y))
          if (CAAR(y) == ctx->impl->code)
            break;
        if (y != NIL)
          break;
      }
      if (x != NIL) {
        s_return(ctx, CDAR(y));
      } else {
        if (PROCP(ctx->impl->code)) { s_return(ctx, ctx->impl->code); }
        if (NATIVE_FUNCP(ctx->impl->code)) { s_return(ctx, ctx->impl->code); }
        {
          char buf[GRN_TABLE_MAX_KEY_SIZE + 1];
          symbol2str(ctx->impl->code, buf);
          QLERR("Unbounded variable %s", buf);
        }
      }
    } else if (PAIRP(ctx->impl->code)) {
      if (SYNTAXP(x = CAR(ctx->impl->code))) {  /* SYNTAX */
        ctx->impl->code = CDR(ctx->impl->code);
        s_goto(ctx, SYNTAXNUM(x));
      } else {/* first, eval top element and eval arguments */
        s_save(ctx, OP_E0ARGS, NIL, ctx->impl->code);
        ctx->impl->code = CAR(ctx->impl->code);
        // if (NATIVE_FUNCP(ctx->impl->code)) { s_return(ctx, ctx->impl->code); } /* call native funcs. fast */
        s_goto(ctx, OP_EVAL);
      }
    } else {
      s_return(ctx, ctx->impl->code);
    }

  case OP_E0ARGS:  /* eval arguments */
    if (MACROP(ctx->impl->value)) {  /* macro expansion */
      s_save(ctx, OP_DOMACRO, NIL, NIL);
      ctx->impl->args = CONS(ctx->impl->code, NIL);
      ctx->impl->code = ctx->impl->value;
      s_goto(ctx, OP_APPLY);
    } else {
      ctx->impl->code = CDR(ctx->impl->code);
      s_goto(ctx, OP_E1ARGS);
    }

  case OP_E1ARGS:  /* eval arguments */
    ctx->impl->args = CONS(ctx->impl->value, ctx->impl->args);
    if (PAIRP(ctx->impl->code)) {  /* continue */
      s_save(ctx, OP_E1ARGS, ctx->impl->args, CDR(ctx->impl->code));
      ctx->impl->code = CAR(ctx->impl->code);
      ctx->impl->args = NIL;
      s_goto(ctx, OP_EVAL);
    } else {  /* end */
      ctx->impl->args = reverse(ctx, ctx->impl->args);
      ctx->impl->code = CAR(ctx->impl->args);
      ctx->impl->args = CDR(ctx->impl->args);
      s_goto(ctx, OP_APPLY);
    }

  case OP_APPLY:    /* apply 'code' to 'args' */
    if (NATIVE_FUNCP(ctx->impl->code)) {
      ctx->impl->dump = CONS(ctx->impl->code, ctx->impl->dump);
      ctx->impl->co.func = ctx->impl->code->u.o.func;
      s_goto(ctx, OP_NATIVE);
    } else if (PROCP(ctx->impl->code)) {
      s_goto(ctx, PROCNUM(ctx->impl->code));  /* PROCEDURE */
    } else if (CLOSUREP(ctx->impl->code)) {  /* CLOSURE */
      /* make environment */
      ctx->impl->envir = CONS(NIL, CLOSURE_ENV(ctx->impl->code));
      for (x = CAR(CLOSURE_CODE(ctx->impl->code)), y = ctx->impl->args;
           PAIRP(x); x = CDR(x), y = CDR(y)) {
        if (y == NIL) {
          QLERR("Few arguments");
        } else {
          CAR(ctx->impl->envir) = CONS(CONS(CAR(x), CAR(y)), CAR(ctx->impl->envir));
        }
      }
      if (x == NIL) {
        /*--
         * if (y != NIL) {
         *   QLERR("Many arguments");
         * }
         */
      } else if (SYMBOLP(x))
        CAR(ctx->impl->envir) = CONS(CONS(x, y), CAR(ctx->impl->envir));
      else {
        QLERR("Syntax error in closure");
      }
      ctx->impl->code = CDR(CLOSURE_CODE(ctx->impl->code));
      ctx->impl->args = NIL;
      s_goto(ctx, OP_BEGIN);
    } else if (CONTINUATIONP(ctx->impl->code)) {  /* CONTINUATION */
      ctx->impl->dump = CONT_DUMP(ctx->impl->code);
      s_return(ctx, ctx->impl->args != NIL ? CAR(ctx->impl->args) : NIL);
    } else {
      QLERR("Illegal function");
    }

  case OP_DOMACRO:  /* do macro */
    ctx->impl->code = ctx->impl->value;
    s_goto(ctx, OP_EVAL);

  case OP_LAMBDA:  /* lambda */
    s_return(ctx, mk_closure(ctx, ctx->impl->code, ctx->impl->envir));

  case OP_QUOTE:    /* quote */
    s_return(ctx, CAR(ctx->impl->code));

  case OP_DEF0:  /* define */
    if (PAIRP(CAR(ctx->impl->code))) {
      x = CAAR(ctx->impl->code);
      ctx->impl->code = CONS(LAMBDA, CONS(CDAR(ctx->impl->code), CDR(ctx->impl->code)));
    } else {
      x = CAR(ctx->impl->code);
      ctx->impl->code = CADR(ctx->impl->code);
    }
    if (!SYMBOLP(x)) {
      QLERR("Variable is not symbol");
    }
    s_save(ctx, OP_DEF1, NIL, x);
    s_goto(ctx, OP_EVAL);

  case OP_DEF1:  /* define */
    for (x = CAR(ctx->impl->envir); x != NIL; x = CDR(x))
      if (CAAR(x) == ctx->impl->code)
        break;
    if (x != NIL)
      CDAR(x) = ctx->impl->value;
    else
      CAR(ctx->impl->envir) = CONS(CONS(ctx->impl->code, ctx->impl->value), CAR(ctx->impl->envir));
    s_return(ctx, ctx->impl->code);

  case OP_SET0:    /* set! */
    s_save(ctx, OP_SET1, NIL, CAR(ctx->impl->code));
    ctx->impl->code = CADR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_SET1:    /* set! */
    for (x = ctx->impl->envir; x != NIL; x = CDR(x)) {
      for (y = CAR(x); y != NIL; y = CDR(y))
        if (CAAR(y) == ctx->impl->code)
          break;
      if (y != NIL)
        break;
    }
    if (x != NIL) {
      CDAR(y) = ctx->impl->value;
      s_return(ctx, ctx->impl->value);
    } else {
      if (SYMBOLP(ctx->impl->code)) {
        char buf[GRN_TABLE_MAX_KEY_SIZE + 1];
        symbol2str(ctx->impl->code, buf);
        QLERR("Unbounded variable %s", buf);
      } else {
        QLERR("Unbounded variable");
      }
    }

  case OP_BEGIN:    /* begin */
    if (!PAIRP(ctx->impl->code)) {
      s_return(ctx, ctx->impl->code);
    }
    if (CDR(ctx->impl->code) != NIL) {
      s_save(ctx, OP_BEGIN, NIL, CDR(ctx->impl->code));
    }
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_IF0:    /* if */
    s_save(ctx, OP_IF1, NIL, CDR(ctx->impl->code));
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_IF1:    /* if */
    if (istrue(ctx->impl->value))
      ctx->impl->code = CAR(ctx->impl->code);
    else
      ctx->impl->code = CADR(ctx->impl->code);  /* (if #f 1) ==> () because
             * CAR(NIL) = NIL */
    s_goto(ctx, OP_EVAL);

  case OP_LET0:    /* let */
    ctx->impl->args = NIL;
    ctx->impl->value = ctx->impl->code;
    ctx->impl->code = SYMBOLP(CAR(ctx->impl->code)) ? CADR(ctx->impl->code) : CAR(ctx->impl->code);
    s_goto(ctx, OP_LET1);

  case OP_LET1:    /* let (caluculate parameters) */
    ctx->impl->args = CONS(ctx->impl->value, ctx->impl->args);
    if (PAIRP(ctx->impl->code)) {  /* continue */
      QLASSERT(LISTP(CAR(ctx->impl->code)) && LISTP(CDAR(ctx->impl->code)));
      s_save(ctx, OP_LET1, ctx->impl->args, CDR(ctx->impl->code));
      ctx->impl->code = CADAR(ctx->impl->code);
      ctx->impl->args = NIL;
      s_goto(ctx, OP_EVAL);
    } else {  /* end */
      ctx->impl->args = reverse(ctx, ctx->impl->args);
      ctx->impl->code = CAR(ctx->impl->args);
      ctx->impl->args = CDR(ctx->impl->args);
      s_goto(ctx, OP_LET2);
    }

  case OP_LET2:    /* let */
    ctx->impl->envir = CONS(NIL, ctx->impl->envir);
    for (x = SYMBOLP(CAR(ctx->impl->code)) ? CADR(ctx->impl->code) : CAR(ctx->impl->code), y = ctx->impl->args;
         y != NIL; x = CDR(x), y = CDR(y))
      CAR(ctx->impl->envir) = CONS(CONS(CAAR(x), CAR(y)), CAR(ctx->impl->envir));
    if (SYMBOLP(CAR(ctx->impl->code))) {  /* named let */
      for (x = CADR(ctx->impl->code), ctx->impl->args = NIL; PAIRP(x); x = CDR(x))
        ctx->impl->args = CONS(CAAR(x), ctx->impl->args);
      x = mk_closure(ctx, CONS(reverse(ctx, ctx->impl->args), CDDR(ctx->impl->code)),
                     ctx->impl->envir);
      CAR(ctx->impl->envir) = CONS(CONS(CAR(ctx->impl->code), x), CAR(ctx->impl->envir));
      ctx->impl->code = CDDR(ctx->impl->code);
      ctx->impl->args = NIL;
    } else {
      ctx->impl->code = CDR(ctx->impl->code);
      ctx->impl->args = NIL;
    }
    s_goto(ctx, OP_BEGIN);

  case OP_LET0AST:  /* let* */
    if (CAR(ctx->impl->code) == NIL) {
      ctx->impl->envir = CONS(NIL, ctx->impl->envir);
      ctx->impl->code = CDR(ctx->impl->code);
      s_goto(ctx, OP_BEGIN);
    }
    s_save(ctx, OP_LET1AST, CDR(ctx->impl->code), CAR(ctx->impl->code));
    QLASSERT(LISTP(CAR(ctx->impl->code)) &&
             LISTP(CAAR(ctx->impl->code)) && LISTP((CDR(CAAR(ctx->impl->code)))));
    ctx->impl->code = CADAAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_LET1AST:  /* let* (make new frame) */
    ctx->impl->envir = CONS(NIL, ctx->impl->envir);
    s_goto(ctx, OP_LET2AST);

  case OP_LET2AST:  /* let* (caluculate parameters) */
    CAR(ctx->impl->envir) = CONS(CONS(CAAR(ctx->impl->code), ctx->impl->value), CAR(ctx->impl->envir));
    ctx->impl->code = CDR(ctx->impl->code);
    if (PAIRP(ctx->impl->code)) {  /* continue */
      QLASSERT(LISTP(CAR(ctx->impl->code)) && LISTP(CDAR(ctx->impl->code)));
      s_save(ctx, OP_LET2AST, ctx->impl->args, ctx->impl->code);
      ctx->impl->code = CADAR(ctx->impl->code);
      ctx->impl->args = NIL;
      s_goto(ctx, OP_EVAL);
    } else {  /* end */
      ctx->impl->code = ctx->impl->args;
      ctx->impl->args = NIL;
      s_goto(ctx, OP_BEGIN);
    }

  case OP_LET0REC:  /* letrec */
    ctx->impl->envir = CONS(NIL, ctx->impl->envir);
    ctx->impl->args = NIL;
    ctx->impl->value = ctx->impl->code;
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_LET1REC);

  case OP_LET1REC:  /* letrec (caluculate parameters) */
    ctx->impl->args = CONS(ctx->impl->value, ctx->impl->args);
    if (PAIRP(ctx->impl->code)) {  /* continue */
      QLASSERT(LISTP(CAR(ctx->impl->code)) && LISTP(CDAR(ctx->impl->code)));
      s_save(ctx, OP_LET1REC, ctx->impl->args, CDR(ctx->impl->code));
      ctx->impl->code = CADAR(ctx->impl->code);
      ctx->impl->args = NIL;
      s_goto(ctx, OP_EVAL);
    } else {  /* end */
      ctx->impl->args = reverse(ctx, ctx->impl->args);
      ctx->impl->code = CAR(ctx->impl->args);
      ctx->impl->args = CDR(ctx->impl->args);
      s_goto(ctx, OP_LET2REC);
    }

  case OP_LET2REC:  /* letrec */
    for (x = CAR(ctx->impl->code), y = ctx->impl->args; y != NIL; x = CDR(x), y = CDR(y))
      CAR(ctx->impl->envir) = CONS(CONS(CAAR(x), CAR(y)), CAR(ctx->impl->envir));
    ctx->impl->code = CDR(ctx->impl->code);
    ctx->impl->args = NIL;
    s_goto(ctx, OP_BEGIN);

  case OP_COND0:    /* cond */
    if (!PAIRP(ctx->impl->code)) {
      QLERR("Syntax error in cond");
    }
    s_save(ctx, OP_COND1, NIL, ctx->impl->code);
    ctx->impl->code = CAAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_COND1:    /* cond */
    if (istrue(ctx->impl->value)) {
      if ((ctx->impl->code = CDAR(ctx->impl->code)) == NIL) {
        s_return(ctx, ctx->impl->value);
      }
      s_goto(ctx, OP_BEGIN);
    } else {
      if ((ctx->impl->code = CDR(ctx->impl->code)) == NIL) {
        s_return(ctx, NIL);
      } else {
        s_save(ctx, OP_COND1, NIL, ctx->impl->code);
        ctx->impl->code = CAAR(ctx->impl->code);
        s_goto(ctx, OP_EVAL);
      }
    }

  case OP_DELAY:    /* delay */
    x = mk_closure(ctx, CONS(NIL, ctx->impl->code), ctx->impl->envir);
    if (ERRP(ctx, GRN_ERROR)) { return F; }
    SETPROMISE(x);
    s_return(ctx, x);

  case OP_AND0:    /* and */
    if (ctx->impl->code == NIL) {
      s_return(ctx, T);
    }
    s_save(ctx, OP_AND1, NIL, CDR(ctx->impl->code));
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_AND1:    /* and */
    if (isfalse(ctx->impl->value)) {
      s_return(ctx, ctx->impl->value);
    } else if (ctx->impl->code == NIL) {
      s_return(ctx, ctx->impl->value);
    } else {
      s_save(ctx, OP_AND1, NIL, CDR(ctx->impl->code));
      ctx->impl->code = CAR(ctx->impl->code);
      s_goto(ctx, OP_EVAL);
    }

  case OP_OR0:    /* or */
    if (ctx->impl->code == NIL) {
      s_return(ctx, F);
    }
    s_save(ctx, OP_OR1, NIL, CDR(ctx->impl->code));
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_OR1:    /* or */
    if (istrue(ctx->impl->value)) {
      s_return(ctx, ctx->impl->value);
    } else if (ctx->impl->code == NIL) {
      s_return(ctx, ctx->impl->value);
    } else {
      s_save(ctx, OP_OR1, NIL, CDR(ctx->impl->code));
      ctx->impl->code = CAR(ctx->impl->code);
      s_goto(ctx, OP_EVAL);
    }

  case OP_C0STREAM:  /* cons-stream */
    s_save(ctx, OP_C1STREAM, NIL, CDR(ctx->impl->code));
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_C1STREAM:  /* cons-stream */
    ctx->impl->args = ctx->impl->value;  /* save ctx->impl->value to register ctx->impl->args for gc */
    x = mk_closure(ctx, CONS(NIL, ctx->impl->code), ctx->impl->envir);
    if (ERRP(ctx, GRN_ERROR)) { return F; }
    SETPROMISE(x);
    s_return(ctx, CONS(ctx->impl->args, x));

  case OP_0MACRO:  /* macro */
    x = CAR(ctx->impl->code);
    ctx->impl->code = CADR(ctx->impl->code);
    if (!SYMBOLP(x)) {
      QLERR("Variable is not symbol");
    }
    s_save(ctx, OP_1MACRO, NIL, x);
    s_goto(ctx, OP_EVAL);

  case OP_1MACRO:  /* macro */
    ctx->impl->value->header.type = GRN_CELL_MACRO;
    for (x = CAR(ctx->impl->envir); x != NIL; x = CDR(x))
      if (CAAR(x) == ctx->impl->code)
        break;
    if (x != NIL)
      CDAR(x) = ctx->impl->value;
    else
      CAR(ctx->impl->envir) = CONS(CONS(ctx->impl->code, ctx->impl->value), CAR(ctx->impl->envir));
    s_return(ctx, ctx->impl->code);

  case OP_CASE0:    /* case */
    s_save(ctx, OP_CASE1, NIL, CDR(ctx->impl->code));
    ctx->impl->code = CAR(ctx->impl->code);
    s_goto(ctx, OP_EVAL);

  case OP_CASE1:    /* case */
    for (x = ctx->impl->code; x != NIL; x = CDR(x)) {
      if (!PAIRP(y = CAAR(x)))
        break;
      for ( ; y != NIL; y = CDR(y))
        if (eqv(CAR(y), ctx->impl->value))
          break;
      if (y != NIL)
        break;
    }
    if (x != NIL) {
      if (PAIRP(CAAR(x))) {
        ctx->impl->code = CDAR(x);
        s_goto(ctx, OP_BEGIN);
      } else {/* else */
        s_save(ctx, OP_CASE2, NIL, CDAR(x));
        ctx->impl->code = CAAR(x);
        s_goto(ctx, OP_EVAL);
      }
    } else {
      s_return(ctx, NIL);
    }

  case OP_CASE2:    /* case */
    if (istrue(ctx->impl->value)) {
      s_goto(ctx, OP_BEGIN);
    } else {
      s_return(ctx, NIL);
    }
  case OP_PAPPLY:  /* apply */
    ctx->impl->code = CAR(ctx->impl->args);
    ctx->impl->args = CADR(ctx->impl->args);
    s_goto(ctx, OP_APPLY);

  case OP_PEVAL:  /* eval */
    ctx->impl->code = CAR(ctx->impl->args);
    ctx->impl->args = NIL;
    s_goto(ctx, OP_EVAL);

  case OP_CONTINUATION:  /* call-with-current-continuation */
    ctx->impl->code = CAR(ctx->impl->args);
    ctx->impl->args = CONS(mk_continuation(ctx, ctx->impl->dump), NIL);
    s_goto(ctx, OP_APPLY);

  case OP_SETCAR:  /* set-car! */
    if (PAIRP(CAR(ctx->impl->args))) {
      CAAR(ctx->impl->args) = CADR(ctx->impl->args);
      s_return(ctx, CAR(ctx->impl->args));
    } else {
      QLERR("Unable to set-car! for non-cons cell");
    }

  case OP_SETCDR:  /* set-cdr! */
    if (PAIRP(CAR(ctx->impl->args))) {
      CDAR(ctx->impl->args) = CADR(ctx->impl->args);
      s_return(ctx, CAR(ctx->impl->args));
    } else {
      QLERR("Unable to set-cdr! for non-cons cell");
    }

  case OP_FORCE:    /* force */
    ctx->impl->code = CAR(ctx->impl->args);
    if (PROMISEP(ctx->impl->code)) {
      ctx->impl->args = NIL;
      s_goto(ctx, OP_APPLY);
    } else {
      s_return(ctx, ctx->impl->code);
    }

  case OP_ERR0:  /* error */
    GRN_BULK_PUTS(ctx, &ctx->impl->outbuf, "*** ERROR: ");
    GRN_BULK_PUTS(ctx, &ctx->impl->outbuf, ctx->errbuf);
    GRN_BULK_PUTC(ctx, &ctx->impl->outbuf, '\n');
    ctx->impl->args = NIL;
    s_goto(ctx, OP_T0LVL);

  case OP_ERR1:  /* error */
    GRN_BULK_PUTS(ctx, &ctx->impl->outbuf, "*** ERROR:");
    while (ctx->impl->args != NIL) {
      GRN_BULK_PUTC(ctx, &ctx->impl->outbuf, ' ');
      grn_obj_inspect(ctx, CAR(ctx->impl->args), &ctx->impl->outbuf, GRN_OBJ_INSPECT_ESC);
      ctx->impl->args = CDR(ctx->impl->args);
    }
    GRN_BULK_PUTC(ctx, &ctx->impl->outbuf, '\n');
    s_goto(ctx, OP_T0LVL);

  case OP_PUT:    /* put */
    if (!HASPROP(CAR(ctx->impl->args)) || !HASPROP(CADR(ctx->impl->args))) {
      QLERR("Illegal use of put");
    }
    for (x = SYMPROP(CAR(ctx->impl->args)), y = CADR(ctx->impl->args); x != NIL; x = CDR(x))
      if (CAAR(x) == y)
        break;
    if (x != NIL)
      CDAR(x) = CADDR(ctx->impl->args);
    else
      SYMPROP(CAR(ctx->impl->args)) = CONS(CONS(y, CADDR(ctx->impl->args)),
              SYMPROP(CAR(ctx->impl->args)));
    s_return(ctx, T);

  case OP_GET:    /* get */
    if (!HASPROP(CAR(ctx->impl->args)) || !HASPROP(CADR(ctx->impl->args))) {
      QLERR("Illegal use of get");
    }
    for (x = SYMPROP(CAR(ctx->impl->args)), y = CADR(ctx->impl->args); x != NIL; x = CDR(x))
      if (CAAR(x) == y)
        break;
    if (x != NIL) {
      s_return(ctx, CDAR(x));
    } else {
      s_return(ctx, NIL);
    }

  case OP_SDOWN:   /* shutdown */
    GRN_LOG(ctx, GRN_LOG_NOTICE, "shutting down..");
    grn_gctx.stat = GRN_QL_QUIT;
    s_goto(ctx, OP_QUIT);

  case OP_RDSEXPR:
    {
      char tok, *str;
      unsigned len;
      RTN_NIL_IF_HEAD(ctx);
      switch (ctx->impl->tok) {
      case TOK_COMMENT:
        skipline(ctx);
        if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        s_goto(ctx, OP_RDSEXPR);
      case TOK_LPAREN:
        if ((tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        ctx->impl->tok = tok;
        if (ctx->impl->tok == TOK_RPAREN) {
          s_return(ctx, NIL);
        } else if (ctx->impl->tok == TOK_DOT) {
          QLERR("syntax error: illegal dot expression");
        } else {
          s_save(ctx, OP_RDLIST, NIL, NIL);
          s_goto(ctx, OP_RDSEXPR);
        }
      case TOK_QUOTE:
        s_save(ctx, OP_RDQUOTE, NIL, NIL);
        if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        s_goto(ctx, OP_RDSEXPR);
      case TOK_BQUOTE:
        s_save(ctx, OP_RDQQUOTE, NIL, NIL);
        if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        s_goto(ctx, OP_RDSEXPR);
      case TOK_COMMA:
        s_save(ctx, OP_RDUNQUOTE, NIL, NIL);
        if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        s_goto(ctx, OP_RDSEXPR);
      case TOK_ATMARK:
        s_save(ctx, OP_RDUQTSP, NIL, NIL);
        if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        s_goto(ctx, OP_RDSEXPR);
      case TOK_ATOM:
        if (readstr(ctx, &str, &len) == TOK_EOS) { ctx->impl->tok = TOK_EOS; RTN_NIL_IF_TAIL(ctx); }
        s_return(ctx, mk_atom(ctx, str, len, NIL));
      case TOK_DQUOTE:
        if (readstrexp(ctx, &str, &len) == TOK_EOS) {
          QLERR("unterminated string");
        }
        s_return(ctx, grn_ql_mk_string(ctx, str, len));
      case TOK_SHARP:
        if ((readstr(ctx, &str, &len) == TOK_EOS) ||
            (x = mk_const(ctx, str, len)) == NIL) {
          QLERR("Undefined sharp expression");
        } else {
          s_return(ctx, x);
        }
      case TOK_EOS :
        if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
        s_goto(ctx, OP_RDSEXPR);
      case TOK_QUESTION:
        {
          grn_cell *o, *p;
          GRN_CELL_NEW(ctx, o);
          p = CONS(o, NIL);
          o->header.type = GRN_CELL_STR;
          o->header.impl_flags = 0;
          o->u.b.size = 1;
          o->u.b.value = "?";
          *ctx->impl->pht = p;
          ctx->impl->pht = &CDR(p);
          s_return(ctx, o);
        }
      default:
        QLERR("syntax error: illegal token");
      }
    }
    break;

  case OP_RDLIST:
    RTN_NIL_IF_HEAD(ctx);
    if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
    if (ctx->impl->tok == TOK_COMMENT) {
      skipline(ctx);
      s_goto(ctx, OP_RDLIST);
    }
    ctx->impl->args = CONS(ctx->impl->value, ctx->impl->args);
    if (ctx->impl->tok == TOK_RPAREN) {
      grn_cell *v = non_alloc_rev(NIL, ctx->impl->args);
      if (ctx->impl->cur < ctx->impl->str_end && *ctx->impl->cur == '.') {
        char *str = NULL;
        unsigned len = 0;
        if (readstr(ctx, &str, &len) != TOK_ATOM) { /* error */ }
        s_return(ctx, mk_atom(ctx, str, len, v));
      } else {
        s_return(ctx, v);
      }
    } else if (ctx->impl->tok == TOK_DOT) {
      s_save(ctx, OP_RDDOT, ctx->impl->args, NIL);
      if ((ctx->impl->tok = token(ctx)) == TOK_EOS) {
        ctx->impl->op = OP_RDSEXPR; RTN_NIL_IF_TAIL(ctx);
      }
      s_goto(ctx, OP_RDSEXPR);
    } else {
      s_save(ctx, OP_RDLIST, ctx->impl->args, NIL);;
      s_goto(ctx, OP_RDSEXPR);
    }

  case OP_RDDOT:
    RTN_NIL_IF_HEAD(ctx);
    if ((ctx->impl->tok = token(ctx)) == TOK_EOS) { RTN_NIL_IF_TAIL(ctx); }
    if (ctx->impl->tok != TOK_RPAREN) {
      QLERR("syntax error: illegal dot expression");
    } else {
      grn_cell *v = non_alloc_rev(ctx->impl->value, ctx->impl->args);
      if (ctx->impl->cur < ctx->impl->str_end && *ctx->impl->cur == '.') {
        char *str = NULL;
        unsigned len = 0;
        if (readstr(ctx, &str, &len) != TOK_ATOM) { /* error */ }
        s_return(ctx, mk_atom(ctx, str, len, v));
      } else {
        s_return(ctx, v);
      }
    }

  case OP_RDQUOTE:
    s_return(ctx, CONS(QUOTE, CONS(ctx->impl->value, NIL)));

  case OP_RDQQUOTE:
    s_return(ctx, CONS(QQUOTE, CONS(ctx->impl->value, NIL)));

  case OP_RDUNQUOTE:
    s_return(ctx, CONS(UNQUOTE, CONS(ctx->impl->value, NIL)));

  case OP_RDUQTSP:
    s_return(ctx, CONS(UNQUOTESP, CONS(ctx->impl->value, NIL)));

  case OP_NATIVE:
    ctx->impl->dump = CDR(ctx->impl->dump);
    s_return(ctx, ctx->impl->value);
  case OP_QQUOTE0:
    ctx->impl->code = list_deep_copy(ctx, ctx->impl->code);
    ctx->impl->args = CONS(ctx->impl->code, NIL);
    qquote_uquotelist(ctx, ctx->impl->code, ctx->impl->code, 0);
    ctx->impl->code = CDR(ctx->impl->args);
    s_goto(ctx, OP_QQUOTE1);
  case OP_QQUOTE1:
    while (PAIRP(ctx->impl->code)) {
      x = CAR(ctx->impl->code);
      if (PAIRP(x) && LISTP(CDR(x))) {
        s_save(ctx, OP_QQUOTE2, ctx->impl->args, ctx->impl->code);
        y = CADR(x);
        if (y == UNQUOTE) {
          QLASSERT(LISTP(CDDR(x)));
          ctx->impl->code = CADDR(x);
        } else if (CAR(y) == UNQUOTESP) {
          QLASSERT(LISTP(CDR(y)));
          ctx->impl->code = CADR(y);
        } else {
          y = CAR(x);
          if (CAR(y) == UNQUOTE) {
            ctx->impl->code = CADR(y);
          } else if (CAAR(y) == UNQUOTESP) {
            ctx->impl->code = CADAR(y);
          } else {
            /* error */
          }
        }
        s_goto(ctx, OP_EVAL);
      }
      ctx->impl->code = CDR(ctx->impl->code);
    }
    s_return(ctx, CAAR(ctx->impl->args));
  case OP_QQUOTE2:
    x = CAR(ctx->impl->code);
    y = CADR(x);
    if (y == UNQUOTE) {
      CDR(x) = ctx->impl->value;
    } else if (CAR(y) == UNQUOTESP) {
      if (ctx->impl->value == NIL) {
        CDR(x) = CDDR(x);
      } else if (!PAIRP(ctx->impl->value) ) {
        /* error */
      } else {
        ctx->impl->value = list_deep_copy(ctx, ctx->impl->value);
        for (y = ctx->impl->value; CDR(y) != NIL; y = CDR(y)) {}
        CDR(y) = CDDR(x);
        CDR(x) = ctx->impl->value;
      }
    } else {
      y = CAAR(x);
      if (y == UNQUOTE) {
        CAR(x) = ctx->impl->value;
      } else if (CAR(y) == UNQUOTESP) {
        if (ctx->impl->value == NIL) {
          CAR(x) = CDAR(x);
        } else if (!PAIRP(ctx->impl->value) ) {
          /* error */
        } else {
          ctx->impl->value = list_deep_copy(ctx, ctx->impl->value);
          for (y = ctx->impl->value; CDR(y) != NIL; y = CDR(y)) {}
          CDR(y) = CDAR(x);
          CAR(x) = ctx->impl->value;
        }
      } else {
        /* error */
      }
    }
    ctx->impl->code = CDR(ctx->impl->code);
    s_goto(ctx, OP_QQUOTE1);
  }
  GRN_LOG(ctx, GRN_LOG_ERROR, "illegal op (%d)", ctx->impl->op);
  return NIL;
}

/* kernel of this intepreter */
inline static void
Eval_Cycle(grn_ctx *ctx)
{
  ctx->impl->co.func = NULL;
  ctx->impl->co.last = 0;
  while (opexe(ctx) != NIL) {
    switch (ctx->impl->op) {
    case OP_NATIVE :
      ctx->stat = GRN_QL_NATIVE;
      return;
    case OP_T0LVL :
      ctx->stat = GRN_QL_TOPLEVEL;
      return;
    case OP_T1LVL :
      ctx->stat = (ctx->impl->phs != NIL) ? GRN_QL_WAIT_ARG : GRN_QL_EVAL;
      return;
    case OP_QUIT :
      ctx->stat = GRN_QL_QUITTING;
      return;
    default :
      break;
    }
    if (ERRP(ctx, GRN_ERROR)) { return; }
  }
  ctx->stat = GRN_QL_WAIT_EXPR;
}

grn_cell *
grn_ql_eval(grn_ctx *ctx, grn_cell *code, grn_cell *objs)
{
  grn_ql_co co;
  uint8_t op = ctx->impl->op;
  uint8_t stat = ctx->stat;
  uint8_t feed_mode = ctx->impl->feed_mode;
  grn_cell *o, *code_ = ctx->impl->code;
  o = CONS(objs, ctx->impl->envir);
  memcpy(&co, &ctx->impl->co, sizeof(grn_ql_co));
  s_save(ctx, OP_QUIT, ctx->impl->args, o);
  ctx->impl->op = OP_EVAL;
  ctx->stat = GRN_QL_EVAL;
  ctx->impl->code = code;
  ctx->impl->feed_mode = grn_ql_atonce;
  grn_ql_feed(ctx, NULL, 0, 0);
  ctx->impl->feed_mode = feed_mode;
  ctx->stat = stat;
  ctx->impl->op = op;
  ctx->impl->envir = CDR(o);
  ctx->impl->code = code_;
  memcpy(&ctx->impl->co, &co, sizeof(grn_ql_co));
  return ctx->impl->value;
}

/* ========== native functions ========== */

#define s_retbool(tf)  do { return (tf) ? T : F; } while (0)

#define do_op(x,y,op) do {\
  switch ((x)->header.type) {\
  case GRN_CELL_INT :\
    switch ((y)->header.type) {\
    case GRN_CELL_INT :\
      IVALUE(x) = IVALUE(x) op IVALUE(y);\
      break;\
    case GRN_CELL_FLOAT :\
      SETFLOAT(x, ((double) IVALUE(x)) op FVALUE(y));\
      break;\
    default :\
      if (grn_obj2int(ctx, y)) { QLERR("can't convert into numeric value"); }\
      IVALUE(x) = IVALUE(x) op IVALUE(y);\
    }\
    break;\
  case GRN_CELL_FLOAT :\
    switch ((y)->header.type) {\
    case GRN_CELL_INT :\
      FVALUE(x) = FVALUE(x) op IVALUE(y);\
      break;\
    case GRN_CELL_FLOAT :\
      FVALUE(x) = FVALUE(x) op FVALUE(y);\
      break;\
    default :\
      if (grn_obj2int(ctx, y)) { QLERR("can't convert into numeric value"); }\
      FVALUE(x) = FVALUE(x) op IVALUE(y);\
    }\
    break;\
  default :\
    QLERR("can't convert into numeric");\
  }\
} while (0)

#define do_compare(x,y,r,op) do {\
  switch (x->header.type) {\
  case GRN_CELL_INT :\
    switch (y->header.type) {\
    case GRN_CELL_INT :\
      r = (IVALUE(x) op IVALUE(y));\
      break;\
    case GRN_CELL_FLOAT :\
      r = (IVALUE(x) op FVALUE(y));\
      break;\
    default :\
      if (grn_obj2int(ctx, y)) { QLERR("can't convert into numeric value"); }\
      r = (IVALUE(x) op IVALUE(y));\
    }\
    break;\
  case GRN_CELL_FLOAT :\
    switch (y->header.type) {\
    case GRN_CELL_INT :\
      r = (FVALUE(x) op IVALUE(y));\
      break;\
    case GRN_CELL_FLOAT :\
      r = (FVALUE(x) op FVALUE(y));\
      break;\
    default :\
      if (grn_obj2int(ctx, y)) { QLERR("can't convert into numeric value"); }\
      r = (FVALUE(x) op IVALUE(y));\
    }\
    break;\
  case GRN_CELL_STR :\
    if (y->header.type == GRN_CELL_STR) {\
      int r_;\
      uint32_t la = x->u.b.size, lb = y->u.b.size;\
      if (la > lb) {\
        if (!(r_ = memcmp(x->u.b.value, y->u.b.value, lb))) {\
          r_ = 1;\
        }\
      } else {\
        if (!(r_ = memcmp(x->u.b.value, y->u.b.value, la))) {\
          r_ = la == lb ? 0 : -1;\
        }\
      }\
      r = (r_ op 0);\
    } else {\
      QLERR("can't compare");\
    }\
    break;\
  case GRN_CELL_TIME :\
    if (y->header.type == GRN_CELL_TIME) {\
      if (x->u.tv.tv_sec != y->u.tv.tv_sec) {\
        r = (x->u.tv.tv_sec op y->u.tv.tv_sec);\
      } else {\
        r = (x->u.tv.tv_usec op y->u.tv.tv_usec);\
      }\
    } else {\
      QLERR("can't compare");\
    }\
    break;\
  default :\
    r = (memcmp(&x->u.tv, &y->u.tv, sizeof(grn_timeval)) op 0);\
  }\
} while (0)

#define time_op(x,y,v,op) {\
  switch (y->header.type) {\
  case GRN_CELL_TIME :\
    {\
      double dv= x->u.tv.tv_sec op y->u.tv.tv_sec;\
      dv += (x->u.tv.tv_usec op y->u.tv.tv_usec) / 1000000.0;\
      SETFLOAT(v, dv);\
    }\
    break;\
  case GRN_CELL_INT :\
    {\
      grn_timeval tv;\
      int64_t sec = x->u.tv.tv_sec op IVALUE(y);\
      if (sec < INT32_MIN || INT32_MAX < sec) { QLERR("time val overflow"); }\
      tv.tv_sec = (int)sec;\
      tv.tv_usec = x->u.tv.tv_usec;\
      SETTIME(v, &tv);\
    }\
    break;\
  case GRN_CELL_FLOAT :\
    {\
      grn_timeval tv;\
      double sec = x->u.tv.tv_sec op (int)FVALUE(y);\
      int32_t usec = x->u.tv.tv_usec op (int)((FVALUE(y) - (int)FVALUE(y)) * 1000000);\
      if (sec < INT32_MIN || INT32_MAX < sec) { QLERR("time val overflow"); }\
      tv.tv_sec = (int)sec;\
      if (usec < 0) {\
        tv.tv_sec--;\
        usec += 1000000;\
      } else if (usec >= 1000000) {\
        tv.tv_sec++;\
        usec -= 1000000;\
      }\
      tv.tv_usec = usec;\
      SETTIME(v, &tv);\
    }\
    break;\
  default :\
    QLERR("can't convert into numeric value");\
    break;\
  }\
} while (0)

static grn_cell *
nf_add(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *x, *v;
  if (!PAIRP(args)) { QLERR("list required"); }
  switch (CAR(args)->header.type) {
  case GRN_CELL_STR :
    {
      grn_obj buf;
      grn_bulk_init(ctx, &buf, 0);
      while (PAIRP(args)) {
        POP(x, args);
        grn_obj_inspect(ctx, x, &buf, 0);
      }
      GRN_STR2OBJ(ctx, &buf, v);
    }
    break;
  case GRN_CELL_TIME :
    if (PAIRP(CDR(args)) && NUMBERP(CADR(args))) {
      GRN_CELL_NEW(ctx, v);
      time_op(CAR(args), CADR(args), v, +);
    } else {
      QLERR("can't convert into numeric value");
    }
    break;
  default :
    v = mk_number(ctx, 0);
    while (PAIRP(args)) {
      POP(x, args);
      do_op(v, x, +);
    }
    break;
  }
  return v;
}

static grn_cell *
nf_sub(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *v = mk_number(ctx, 0);
  register grn_cell *x;
  if (PAIRP(args) && CDR(args) != NIL) {
    if (CAR(args)->header.type == GRN_CELL_TIME) {
      time_op(CAR(args), CADR(args), v, -);
      return v;
    }
    POP(x, args);
    do_op(v, x, +);
  }
  while (PAIRP(args)) {
    POP(x, args);
    do_op(v, x, -);
  }
  return v;
}

static grn_cell *
nf_mul(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *v, *x;
  if (CAR(args)->header.type == GRN_CELL_STR && CADR(args)->header.type == GRN_CELL_INT) {
    int i, n = (int)IVALUE(CADR(args));
    grn_obj buf;
    grn_bulk_init(ctx, &buf, 0);
    POP(x, args);
    for (i = 0; i < n; i++) {
      grn_obj_inspect(ctx, x, &buf, 0);
    }
    GRN_STR2OBJ(ctx, &buf, v);
  } else {
    v = mk_number(ctx, 1);
    while (PAIRP(args)) {
      POP(x, args);
      do_op(v, x, *);
    }
  }
  return v;
}

static grn_cell *
nf_div(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *v;
  register grn_cell *x;
  if (PAIRP(args) && CDR(args) != NIL) {
    v = mk_number(ctx, 0);
    POP(x, args);
    do_op(v, x, +);
  } else {
    v = mk_number(ctx, 1);
  }
  while (PAIRP(args)) {
    POP(x, args);
    if (x->header.type == GRN_CELL_INT && IVALUE(x) == 0 && v->header.type == GRN_CELL_INT) {
      SETFLOAT(v, (double)IVALUE(v));
    }
    do_op(v, x, /);
  }
  return v;
}
static grn_cell *
nf_rem(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register int64_t v;
  register grn_cell *x;
  x = args;
  if (grn_obj2int(ctx, CAR(x))) {
    QLERR("can't convert into integer");
  }
  v = IVALUE(CAR(x));
  while (CDR(x) != NIL) {
    x = CDR(x);
    if (grn_obj2int(ctx, CAR(x))) {
      QLERR("can't convert into integer");
    }
    if (IVALUE(CAR(x)) != 0)
      v %= IVALUE(CAR(x));
    else {
      QLERR("Divided by zero");
    }
  }
  return mk_number(ctx, v);
}
static grn_cell *
nf_car(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  if (PAIRP(CAR(args))) {
    return CAAR(args);
  } else {
    QLERR("Unable to car for non-cons cell");
  }
}
static grn_cell *
nf_cdr(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  if (PAIRP(CAR(args))) {
    return CDAR(args);
  } else {
    QLERR("Unable to cdr for non-cons cell");
  }
}
static grn_cell *
nf_cons(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  CDR(args) = CADR(args);
  return args;
}
static grn_cell *
nf_not(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(isfalse(CAR(args)));
}
static grn_cell *
nf_bool(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(CAR(args) == F || CAR(args) == T);
}
static grn_cell *
nf_null(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(CAR(args) == NIL);
}
static grn_cell *
nf_zerop(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_INT :
    s_retbool(IVALUE(x) == 0);
    break;
  case GRN_CELL_FLOAT :
    s_retbool(!(islessgreater(FVALUE(x), 0.0)));
    break;
  default :
    QLERR("can't convert into numeric value");
  }
}
static grn_cell *
nf_posp(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_INT :
    s_retbool(IVALUE(x) > 0);
    break;
  case GRN_CELL_FLOAT :
    s_retbool(!(isgreater(FVALUE(x), 0.0)));
    break;
  default :
    QLERR("can't convert into numeric value");
  }
}
static grn_cell *
nf_negp(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_INT :
    s_retbool(IVALUE(x) < 0);
    break;
  case GRN_CELL_FLOAT :
    s_retbool(!(isless(FVALUE(x), 0.0)));
    break;
  default :
    QLERR("can't convert into numeric value");
  }
}
static grn_cell *
nf_neq(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int r = 1;
  register grn_cell *x, *y;
  POP(x, args);
  if (!PAIRP(args)) { QLERR("Few arguments"); }
  do {
    POP(y, args);
    switch (x->header.type) {
    case GRN_CELL_INT :
      switch (y->header.type) {
      case GRN_CELL_INT :
        r = (IVALUE(x) == IVALUE(y));
        break;
      case GRN_CELL_FLOAT :
        r = (IVALUE(x) <= FVALUE(y) && IVALUE(x) >= FVALUE(y));
        break;
      default :
        if (grn_obj2int(ctx, y)) { QLERR("can't convert into numeric value"); }
        r = (IVALUE(x) == IVALUE(y));
      }
      break;
    case GRN_CELL_FLOAT :
      switch (y->header.type) {
      case GRN_CELL_INT :
        r = (FVALUE(x) <= IVALUE(y) && FVALUE(x) >= IVALUE(y));
        break;
      case GRN_CELL_FLOAT :
        r = (FVALUE(x) <= FVALUE(y) && FVALUE(x) >= FVALUE(y));
        break;
      default :
        if (grn_obj2int(ctx, y)) { QLERR("can't convert into numeric value"); }
        r = (FVALUE(x) <= IVALUE(y) && FVALUE(x) >= IVALUE(y));
      }
      break;
    case GRN_CELL_STR :
      if (y->header.type == GRN_CELL_STR) {
        int r_;
        uint32_t la = x->u.b.size, lb = y->u.b.size;
        if (la > lb) {
          if (!(r_ = memcmp(x->u.b.value, y->u.b.value, lb))) {
            r_ = 1;
          }
        } else {
          if (!(r_ = memcmp(x->u.b.value, y->u.b.value, la))) {
            r_ = la == lb ? 0 : -1;
          }
        }
        r = (r_ == 0);
      } else {
        QLERR("can't compare");
      }
      break;
    case GRN_CELL_TIME :
      if (y->header.type == GRN_CELL_TIME) {
        if (x->u.tv.tv_sec != y->u.tv.tv_sec) {
          r = (x->u.tv.tv_sec == y->u.tv.tv_sec);
        } else {
          r = (x->u.tv.tv_usec == y->u.tv.tv_usec);
        }
      } else {
        QLERR("can't compare");
      }
      break;
    case GRN_CELL_OBJECT :
      r = (y->header.type == GRN_CELL_OBJECT &&
           x->u.o.id == y->u.o.id && x->header.domain == y->header.domain);
      break;
    default :
      r = (memcmp(&x->u.tv, &y->u.tv, sizeof(grn_timeval)) == 0);
      break;
    }
    x = y;
  } while (PAIRP(args) && r);
  return r ? T : F;
}
static grn_cell *
nf_less(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int r = 1;
  register grn_cell *x, *y;
  POP(x, args);
  if (!PAIRP(args)) { QLERR("Few arguments"); }
  do {
    POP(y, args);
    do_compare(x, y, r, <);
    x = y;
  } while (PAIRP(args) && r);
  return r ? T : F;
}
static grn_cell *
nf_gre(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int r = 1;
  register grn_cell *x, *y;
  POP(x, args);
  if (!PAIRP(args)) { QLERR("Few arguments"); }
  do {
    POP(y, args);
    do_compare(x, y, r, >);
    x = y;
  } while (PAIRP(args) && r);
  return r ? T : F;
}
static grn_cell *
nf_leq(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int r = 1;
  register grn_cell *x, *y;
  POP(x, args);
  if (!PAIRP(args)) { QLERR("Few arguments"); }
  do {
    POP(y, args);
    do_compare(x, y, r, <=);
    x = y;
  } while (PAIRP(args) && r);
  return r ? T : F;
}
static grn_cell *
nf_geq(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int r = 1;
  register grn_cell *x, *y;
  POP(x, args);
  if (!PAIRP(args)) { QLERR("Few arguments"); }
  do {
    POP(y, args);
    do_compare(x, y, r, >=);
    x = y;
  } while (PAIRP(args) && r);
  return r ? T : F;
}
static grn_cell *
nf_symbol(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(SYMBOLP(CAR(args)));
}
static grn_cell *
nf_number(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(NUMBERP(CAR(args)));
}
static grn_cell *
nf_string(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(BULKP(CAR(args)));
}
static grn_cell *
nf_proc(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  /*--
   * continuation should be procedure by the example
   * (call-with-current-continuation procedure?) ==> #t
   * in R^3 report sec. 6.9
   */
  s_retbool(PROCEDUREP(CAR(args)));
}
static grn_cell *
nf_pair(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(PAIRP(CAR(args)));
}
static grn_cell *
nf_eq(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(CAR(args) == CADR(args));
}
static grn_cell *
nf_eqv(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(eqv(CAR(args), CADR(args)));
}
static grn_cell *
nf_logtest(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(logtest(CAR(args), CADR(args)));
}
static grn_cell *
nf_write(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  args = CAR(args);
  grn_obj_inspect(ctx, args, &ctx->impl->outbuf, GRN_OBJ_INSPECT_ESC);
  return T;
}
static grn_cell *
nf_display(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  args = CAR(args);
  grn_obj_inspect(ctx, args, &ctx->impl->outbuf, 0);
  return T;
}
static grn_cell *
nf_newline(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  GRN_BULK_PUTC(ctx, &ctx->impl->outbuf, '\n');
  return T;
}
static grn_cell *
nf_reverse(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  return reverse(ctx, CAR(args));
}
static grn_cell *
nf_append(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  return append(ctx, CAR(args), CADR(args));
}
static grn_cell *
nf_gc(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_ctx_mgc(ctx);
  return T;
}
static grn_cell *
nf_gcverb(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int  was = ctx->impl->gc_verbose;
  ctx->impl->gc_verbose = (CAR(args) != F);
  s_retbool(was);
}
static grn_cell *
nf_nativep(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(NATIVE_FUNCP(CAR(args)));
}
static grn_cell *
nf_length(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register long v;
  register grn_cell *x;
  for (x = CAR(args), v = 0; PAIRP(x); x = CDR(x)) { ++v; }
  return mk_number(ctx, v);
}
static grn_cell *
nf_assq(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  register grn_cell *x, *y;
  x = CAR(args);
  for (y = CADR(args); PAIRP(y); y = CDR(y)) {
    if (!PAIRP(CAR(y))) {
      QLERR("Unable to handle non pair element");
    }
    if (x == CAAR(y))
      break;
  }
  if (PAIRP(y)) {
    return CAR(y);
  } else {
    return F;
  }
}
static grn_cell *
nf_get_closure(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  args = CAR(args);
  if (args == NIL) {
    return F;
  } else if (CLOSUREP(args)) {
    return CONS(LAMBDA, CLOSURE_CODE(ctx->impl->value));
  }  else {
    return F;
  }
}
static grn_cell *
nf_closurep(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  /*
   * Note, macro object is also a closure.
   * Therefore, (closure? <#MACRO>) ==> #t
   */
  if (CAR(args) == NIL) {
      return F;
  }
  s_retbool(CLOSUREP(CAR(args)));
}
static grn_cell *
nf_macrop(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  if (CAR(args) == NIL) {
      return F;
  }
  s_retbool(MACROP(CAR(args)));
}
static grn_cell *
nf_voidp(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  s_retbool(CAR(args)->header.type == GRN_VOID);
}
static grn_cell *
nf_list(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  if (PAIRP(args)) {
    return args;
  } else {
    QLERR("Unable to handle non-cons argument");
  }
}
static grn_cell *
nf_batchmode(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  if (CAR(args) == F) {
    ctx->impl->batchmode = 0;
    return F;
  } else {
    ctx->impl->batchmode = 1;
    return T;
  }
}
static grn_cell *
nf_loglevel(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  static grn_logger_info info;
  grn_cell *x = CAR(args);
  if (grn_obj2int(ctx, x)) { QLERR("can't convert into integer"); }
  info.max_level = IVALUE(x);
  info.flags = GRN_LOG_TIME|GRN_LOG_MESSAGE;
  info.func = NULL;
  info.func_arg = NULL;
  return (grn_logger_info_set(ctx, &info)) ? F : T;
}
static grn_cell *
nf_now(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *x;
  grn_timeval tv;
  if (grn_timeval_now(ctx, &tv)) { QLERR("sysdate failed"); }
  GRN_CELL_NEW(ctx, x);
  SETTIME(x, &tv);
  return x;
}
static grn_cell *
nf_timestr(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_timeval tv;
  char buf[GRN_TIMEVAL_STR_SIZE];
  grn_cell *x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_STR :
    if (grn_obj2int(ctx, x)) { QLERR("can't convert into integer"); }
    /* fallthru */
  case GRN_CELL_INT :
    tv.tv_sec = IVALUE(x);
    tv.tv_usec = 0;
    break;
  case GRN_CELL_FLOAT :
    tv.tv_sec = (int32_t) FVALUE(x);
    tv.tv_usec = (int32_t) ((FVALUE(x) - tv.tv_sec) * 1000000);
    break;
  case GRN_CELL_TIME :
    memcpy(&tv, &x->u.tv, sizeof(grn_timeval));
    break;
  default :
    QLERR("can't convert into time");
  }
  if (grn_timeval2str(ctx, &tv, buf)) { QLERR("timeval2str failed"); }
  return grn_ql_mk_string(ctx, buf, strlen(buf));
}
static grn_cell *
nf_tonumber(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *x, *v;
  if (!PAIRP(args)) { QLERR("list required"); }
  x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_STR :
    if ((v = str2num(ctx, STRVALUE(x), x->u.b.size)) == NIL) { v = mk_number(ctx, 0); }
    break;
  case GRN_CELL_INT :
  case GRN_CELL_FLOAT :
    v = x;
    break;
  case GRN_CELL_TIME :
    {
      double dv= x->u.tv.tv_sec;
      dv += x->u.tv.tv_usec / 1000000.0;
      GRN_CELL_NEW(ctx, v);
      SETFLOAT(v, dv);
    }
    break;
  default :
    v = mk_number(ctx, 0);
    break;
  }
  return v;
}
static grn_cell *
nf_totime(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_timeval tv;
  grn_cell *x, *v;
  if (!PAIRP(args)) { QLERR("list required"); }
  x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_STR :
    {
      /*
      if (PAIRP(CDR(args)) && BULKP(CADR(args))) { fmt = STRVALUE(CADR(args)); }
      */
      if (grn_str2timeval(STRVALUE(x), x->u.b.size, &tv)) {
        QLERR("cast error");
      }
      GRN_CELL_NEW(ctx, v);
      SETTIME(v, &tv);
    }
    break;
  case GRN_CELL_INT :
    tv.tv_sec = (int32_t) IVALUE(x);
    tv.tv_usec = 0;
    GRN_CELL_NEW(ctx, v);
    SETTIME(v, &tv);
    break;
  case GRN_CELL_FLOAT :
    tv.tv_sec = (int32_t) FVALUE(x);
    tv.tv_usec = (int32_t) ((FVALUE(x) - tv.tv_sec) * 1000000);
    GRN_CELL_NEW(ctx, v);
    SETTIME(v, &tv);
    break;
  case GRN_CELL_TIME :
    v = x;
    break;
  default :
    QLERR("can't convert into number");
  }
  return v;
}
static grn_cell *
nf_substrb(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *str, *s, *e;
  int64_t is, ie;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(str, args);
  if (!BULKP(str)) { QLERR("string required"); }
  POP(s, args);
  if (!INTP(s)) { QLERR("integer required"); }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  is = IVALUE(s);
  ie = IVALUE(e) + 1;
  if (ie <= 0) {
    ie = str->u.b.size + ie;
    if (ie < 0) { ie = 0; }
  } else if (ie > str->u.b.size) {
    ie = str->u.b.size;
  }
  if (is < 0) {
    is = str->u.b.size + is + 1;
    if (is < 0) { is = 0; }
  } else if (is > str->u.b.size) {
    is = str->u.b.size;
  }
  if (is < ie) {
    return grn_ql_mk_string(ctx, STRVALUE(str) + is, ie - is);
  } else {
    grn_cell *o;
    GRN_CELL_NEW(ctx, o);
    o->header.impl_flags = 0;
    o->header.type = GRN_CELL_STR;
    o->u.b.size = 0;
    o->u.b.value = NULL;
    return o;
  }
}
static grn_cell *
nf_tob32h(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *x, *v;
  if (!PAIRP(args)) { QLERR("list required"); }
  x = CAR(args);
  switch (x->header.type) {
  case GRN_CELL_INT :
    {
      grn_obj buf;
      grn_bulk_init(ctx, &buf, 13);
      if (grn_bulk_lltob32h(ctx, &buf, IVALUE(x))) {
        grn_bulk_fin(ctx, &buf);
        QLERR("lltob32h failed");
      }
      GRN_STR2OBJ(ctx, &buf, v);
    }
    break;
  case GRN_CELL_FLOAT :
    {
      grn_obj buf;
      grn_bulk_init(ctx, &buf, 13);
      if (grn_bulk_lltob32h(ctx, &buf, (int64_t)FVALUE(x))) {
        grn_bulk_fin(ctx, &buf);
        QLERR("lltob32h failed");
      }
      GRN_STR2OBJ(ctx, &buf, v);
    }
    break;
  default :
    QLERR("can't convert into int");
  }
  return v;
}
static grn_cell *
nf_intern(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *x, *v;
  if (!PAIRP(args)) { QLERR("list required"); }
  x = CAR(args);
  if SYMBOLP(x) { return x; }
  switch (x->header.type) {
  case GRN_CELL_STR :
    v = grn_ql_mk_symbol2(ctx, STRVALUE(x), STRSIZE(x), 0);
    break;
  default :
    QLERR("can't convert into string");
  }
  return v;
}
static grn_cell *
nf_containp(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  int r = 0;
  grn_id id;
  grn_cell *e, *car;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(e, args);
  switch (e->header.type) {
  case GRN_UVECTOR :
    {
      grn_obj *u = e->u.p.value, *range = grn_ctx_get(ctx, u->header.domain);
      grn_id *v, *ve = (grn_id *)GRN_BULK_CURR(u);
      POP(e, args);
      switch (e->header.type) {
      case GRN_CELL_LIST :
        r = 1;
        while (r && PAIRP(e)) {
          POP(car, e);
          switch (car->header.type) {
          case GRN_CELL_OBJECT :
            if (car->header.domain == u->header.domain && (id = car->u.o.id)) {
              for (v = (grn_id *)GRN_BULK_HEAD(u);; v++) {
                if (v == ve) { r = 0; break; }
                if (*v == id) { break; }
              }
            } else {
              r = 0;
            }
            break;
          case GRN_CELL_STR :
            id = grn_table_at(ctx, range, STRVALUE(car), STRSIZE(car), NULL);
            if (id) {
              for (v = (grn_id *)GRN_BULK_HEAD(u);; v++) {
                if (v == ve) { r = 0; break; }
                if (*v == id) { break; }
              }
            } else {
              r = 0;
            }
            break;
          default :
            r = 0;
            break;
          }
        }
        break;
      case GRN_CELL_STR :
        id = grn_table_at(ctx, range, STRVALUE(car), STRSIZE(car), NULL);
        if (id) {
          for (v = (grn_id *)GRN_BULK_HEAD(u);; v++) {
            if (v == ve) { r = 0; break; }
            if (*v == id) { r = 1; break; }
          }
        } else {
          r = 0;
        }
        break;
      case GRN_CELL_OBJECT :
        if (e->header.domain == u->header.domain && (id = e->u.o.id)) {
          for (v = (grn_id *)GRN_BULK_HEAD(u);; v++) {
            if (v == ve) { r = 0; break; }
            if (*v == id) { r = 1; break; }
          }
        } else {
          r = 0;
        }
        break;
      default :
        r = 0;
        break;
      }
    }
    break;
  case GRN_CELL_OBJECT :
    {
      grn_id domain = e->header.domain;
      id = e->u.o.id;
      POP(e, args);
      if (e->header.type == GRN_CELL_LIST) { e = CAR(e); }
      switch (e->header.type) {
      case GRN_CELL_STR :
        {
          grn_obj *range = grn_ctx_get(ctx, domain);
          if (grn_table_at(ctx, range, STRVALUE(e), STRSIZE(e), NULL) == id) { r = 1; }
        }
        break;
      case GRN_CELL_OBJECT :
        if (e->header.domain == domain && e->u.o.id == id) { r = 1; }
        break;
      default :
        r = 0;
        break;
      }
    }
    break;
  default :
    QLERR("uvector or object required");
    break;
  }
  return r ? T : F;
}

#define GEO_RESOLUTION   3600000
#define GEO_RADIOUS      6357303
#define GEO_BES_C1       6334834
#define GEO_BES_C2       6377397
#define GEO_BES_C3       0.006674
#define GEO_GRS_C1       6335439
#define GEO_GRS_C2       6378137
#define GEO_GRS_C3       0.006694
#define GEO_INT2RAD(x)   ((M_PI * x) / (GEO_RESOLUTION * 180))

static grn_cell *
nf_geo_distance1(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *e;
  double lng1, lat1, lng2, lat2, x, y, d;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng2 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat2 = GEO_INT2RAD(IVALUE(e));
  x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
  y = (lat2 - lat1);
  d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
  GRN_CELL_NEW(ctx, e);
  SETFLOAT(e, d);
  return e;
}
static grn_cell *
nf_geo_distance2(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *e;
  double lng1, lat1, lng2, lat2, x, y, d;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng2 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat2 = GEO_INT2RAD(IVALUE(e));
  x = sin(fabs(lng2 - lng1) * 0.5);
  y = sin(fabs(lat2 - lat1) * 0.5);
  d = asin(sqrt((y * y) + cos(lat1) * cos(lat2) * x * x)) * 2 * GEO_RADIOUS;
  GRN_CELL_NEW(ctx, e);
  SETFLOAT(e, d);
  return e;
}
static grn_cell *
nf_geo_distance3(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *e;
  double lng1, lat1, lng2, lat2, p, q, m, n, x, y, d;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng2 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat2 = GEO_INT2RAD(IVALUE(e));
  p = (lat1 + lat2) * 0.5;
  q = (1 - GEO_BES_C3 * sin(p) * sin(p));
  m = GEO_BES_C1 / sqrt(q * q * q);
  n = GEO_BES_C2 / sqrt(q);
  x = n * cos(p) * fabs(lng1 - lng2);
  y = m * fabs(lat1 - lat2);
  d = sqrt((x * x) + (y * y));
  GRN_CELL_NEW(ctx, e);
  SETFLOAT(e, d);
  return e;
}
static grn_cell *
nf_geo_distance4(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *e;
  double lng1, lat1, lng2, lat2, p, q, m, n, x, y, d;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat1 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lng2 = GEO_INT2RAD(IVALUE(e));
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  lat2 = GEO_INT2RAD(IVALUE(e));
  p = (lat1 + lat2) * 0.5;
  q = (1 - GEO_GRS_C3 * sin(p) * sin(p));
  m = GEO_GRS_C1 / sqrt(q * q * q);
  n = GEO_GRS_C2 / sqrt(q);
  x = n * cos(p) * fabs(lng1 - lng2);
  y = m * fabs(lat1 - lat2);
  d = sqrt((x * x) + (y * y));
  GRN_CELL_NEW(ctx, e);
  SETFLOAT(e, d);
  return e;
}
static grn_cell *
nf_geo_withinp(grn_ctx *ctx, grn_cell *args, grn_ql_co *co)
{
  grn_cell *e;
  int64_t ln0, la0, ln1, la1, ln2, la2, ln3, la3;
  double lng0, lat0, lng1, lat1, lng2, lat2, x, y, d;
  if (!PAIRP(args)) { QLERR("list required"); }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  ln0 = IVALUE(e);
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  la0 = IVALUE(e);
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  ln1 = IVALUE(e);
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  la1 = IVALUE(e);
  if (args == NIL) { return T; }
  POP(e, args);
  if (args == NIL) {
    lng0 = GEO_INT2RAD(ln0);
    lat0 = GEO_INT2RAD(la0);
    lng1 = GEO_INT2RAD(ln1);
    lat1 = GEO_INT2RAD(la1);
    x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
    y = (lat1 - lat0);
    d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
    switch (e->header.type) {
    case GRN_CELL_INT : return d <= IVALUE(e) ? T : F;
    case GRN_CELL_FLOAT : return d <= FVALUE(e) ? T : F;
    default : QLERR("integer or float value required");
    }
  }
  if (!INTP(e)) { QLERR("integer required"); }
  ln2 = IVALUE(e);
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  la2 = IVALUE(e);
  if (args == NIL) {
    lng0 = GEO_INT2RAD(ln0);
    lat0 = GEO_INT2RAD(la0);
    lng1 = GEO_INT2RAD(ln1);
    lat1 = GEO_INT2RAD(la1);
    lng2 = GEO_INT2RAD(ln2);
    lat2 = GEO_INT2RAD(la2);
    x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
    y = (lat1 - lat0);
    d = (x * x) + (y * y);
    x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
    y = (lat2 - lat1);
    return d <= (x * x) + (y * y) ? T : F;
  }
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  ln3 = IVALUE(e);
  POP(e, args);
  if (!INTP(e)) { QLERR("integer required"); }
  la3 = IVALUE(e);
  return ((ln2 <= ln0) && (ln0 <= ln3) && (la2 <= la0) && (la0 <= la3)) ? T : F;
}

/* ========== Initialization of internal keywords ========== */

inline static void
mk_syntax(grn_ctx *ctx, uint8_t op, char *name)
{
  grn_cell *x;
  if ((x = INTERN(name)) != F) {
    x->header.type = GRN_CELL_SYNTAX;
    SYNTAXNUM(x) = op;
  }
}

inline static void
mk_proc(grn_ctx *ctx, uint8_t op, char *name)
{
  grn_cell *x;
  if ((x = INTERN(name)) != F) {
    x->header.type = GRN_CELL_PROC;
    IVALUE(x) = (int64_t) op;
  }
}

void
grn_ql_init_const(void)
{
  static grn_cell _NIL, _T, _F;
  /* init NIL */
  NIL = &_NIL;
  NIL->header.type = GRN_VOID;
  CAR(NIL) = CDR(NIL) = NIL;
  /* init T */
  T = &_T;
  T->header.type = GRN_VOID;
  CAR(T) = CDR(T) = T;
  /* init F */
  F = &_F;
  F->header.type = GRN_VOID;
  CAR(F) = CDR(F) = F;
}

inline static void
init_vars_global(grn_ctx *ctx)
{
  grn_cell *x;
  /* init global_env */
  ctx->impl->global_env = CONS(NIL, NIL);
  /* init else */
  if ((x = INTERN("else")) != F) {
    CAR(ctx->impl->global_env) = CONS(CONS(x, T), CAR(ctx->impl->global_env));
  }
}

inline static void
init_syntax(grn_ctx *ctx)
{
  /* init syntax */
  mk_syntax(ctx, OP_LAMBDA, "lambda");
  mk_syntax(ctx, OP_QUOTE, "quote");
  mk_syntax(ctx, OP_DEF0, "define");
  mk_syntax(ctx, OP_IF0, "if");
  mk_syntax(ctx, OP_BEGIN, "begin");
  mk_syntax(ctx, OP_SET0, "set!");
  mk_syntax(ctx, OP_LET0, "let");
  mk_syntax(ctx, OP_LET0AST, "let*");
  mk_syntax(ctx, OP_LET0REC, "letrec");
  mk_syntax(ctx, OP_COND0, "cond");
  mk_syntax(ctx, OP_DELAY, "delay");
  mk_syntax(ctx, OP_AND0, "and");
  mk_syntax(ctx, OP_OR0, "or");
  mk_syntax(ctx, OP_C0STREAM, "cons-stream");
  mk_syntax(ctx, OP_0MACRO, "define-macro");
  mk_syntax(ctx, OP_CASE0, "case");
  mk_syntax(ctx, OP_QQUOTE0, "quasiquote");
}

inline static void
init_procs(grn_ctx *ctx)
{
  /* init procedure */
  mk_proc(ctx, OP_PEVAL, "eval");
  mk_proc(ctx, OP_PAPPLY, "apply");
  mk_proc(ctx, OP_CONTINUATION, "call-with-current-continuation");
  mk_proc(ctx, OP_FORCE, "force");
  mk_proc(ctx, OP_SETCAR, "set-car!");
  mk_proc(ctx, OP_SETCDR, "set-cdr!");
  mk_proc(ctx, OP_READ, "read");
  mk_proc(ctx, OP_LOAD, "load");
  mk_proc(ctx, OP_ERR1, "error");
  mk_proc(ctx, OP_PUT, "put");
  mk_proc(ctx, OP_GET, "get");
  mk_proc(ctx, OP_QUIT, "quit");
  mk_proc(ctx, OP_SDOWN, "shutdown");
  grn_ql_def_native_func(ctx, "+", nf_add);
  grn_ql_def_native_func(ctx, "-", nf_sub);
  grn_ql_def_native_func(ctx, "*", nf_mul);
  grn_ql_def_native_func(ctx, "/", nf_div);
  grn_ql_def_native_func(ctx, "remainder", nf_rem);
  grn_ql_def_native_func(ctx, "car", nf_car);
  grn_ql_def_native_func(ctx, "cdr", nf_cdr);
  grn_ql_def_native_func(ctx, "cons", nf_cons);
  grn_ql_def_native_func(ctx, "not", nf_not);
  grn_ql_def_native_func(ctx, "boolean?", nf_bool);
  grn_ql_def_native_func(ctx, "symbol?", nf_symbol);
  grn_ql_def_native_func(ctx, "number?", nf_number);
  grn_ql_def_native_func(ctx, "string?", nf_string);
  grn_ql_def_native_func(ctx, "procedure?", nf_proc);
  grn_ql_def_native_func(ctx, "pair?", nf_pair);
  grn_ql_def_native_func(ctx, "eqv?", nf_eqv);
  grn_ql_def_native_func(ctx, "logtest", nf_logtest);
  grn_ql_def_native_func(ctx, "eq?", nf_eq);
  grn_ql_def_native_func(ctx, "null?", nf_null);
  grn_ql_def_native_func(ctx, "zero?", nf_zerop);
  grn_ql_def_native_func(ctx, "positive?", nf_posp);
  grn_ql_def_native_func(ctx, "negative?", nf_negp);
  grn_ql_def_native_func(ctx, "=", nf_neq);
  grn_ql_def_native_func(ctx, "<", nf_less);
  grn_ql_def_native_func(ctx, ">", nf_gre);
  grn_ql_def_native_func(ctx, "<=", nf_leq);
  grn_ql_def_native_func(ctx, ">=", nf_geq);
  grn_ql_def_native_func(ctx, "write", nf_write);
  grn_ql_def_native_func(ctx, "display", nf_display);
  grn_ql_def_native_func(ctx, "newline", nf_newline);
  grn_ql_def_native_func(ctx, "reverse", nf_reverse);
  grn_ql_def_native_func(ctx, "append", nf_append);
  grn_ql_def_native_func(ctx, "gc", nf_gc);
  grn_ql_def_native_func(ctx, "gc-verbose", nf_gcverb);
  grn_ql_def_native_func(ctx, "native?", nf_nativep);
  grn_ql_def_native_func(ctx, "length", nf_length);  /* a.k */
  grn_ql_def_native_func(ctx, "assq", nf_assq);  /* a.k */
  grn_ql_def_native_func(ctx, "get-closure-code", nf_get_closure);  /* a.k */
  grn_ql_def_native_func(ctx, "closure?", nf_closurep);  /* a.k */
  grn_ql_def_native_func(ctx, "macro?", nf_macrop);  /* a.k */
  grn_ql_def_native_func(ctx, "void?", nf_voidp);
  grn_ql_def_native_func(ctx, "list", nf_list);
  grn_ql_def_native_func(ctx, "batchmode", nf_batchmode);
  grn_ql_def_native_func(ctx, "loglevel", nf_loglevel);
  grn_ql_def_native_func(ctx, "now", nf_now);
  grn_ql_def_native_func(ctx, "timestr", nf_timestr);
  grn_ql_def_native_func(ctx, "x->time", nf_totime);
  grn_ql_def_native_func(ctx, "x->number", nf_tonumber);
  grn_ql_def_native_func(ctx, "substrb", nf_substrb);
  grn_ql_def_native_func(ctx, "x->b32h", nf_tob32h);
  grn_ql_def_native_func(ctx, "intern", nf_intern);
  grn_ql_def_native_func(ctx, "contain?", nf_containp);
  grn_ql_def_native_func(ctx, "geo-distance1", nf_geo_distance1);
  grn_ql_def_native_func(ctx, "geo-distance2", nf_geo_distance2);
  grn_ql_def_native_func(ctx, "geo-distance3", nf_geo_distance3);
  grn_ql_def_native_func(ctx, "geo-distance4", nf_geo_distance4);
  grn_ql_def_native_func(ctx, "geo-within?", nf_geo_withinp);
}

/* initialize several globals */
void
grn_ql_init_globals(grn_ctx *ctx)
{
  init_vars_global(ctx);
  init_syntax(ctx);
  init_procs(ctx);
  ctx->impl->output = grn_ctx_concat_func;
  /* initialization of global pointers to special symbols */
}
