/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7.12-4996"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 7 "src/parse.y"

#undef PARSER_DEBUG

#define YYDEBUG 1
#define YYERROR_VERBOSE 1
/*
 * Force yacc to use our memory management.  This is a little evil because
 * the macros assume that "parser_state *p" is in scope
 */
#define YYMALLOC(n)    mrb_malloc(p->mrb, (n))
#define YYFREE(o)      mrb_free(p->mrb, (o))
#define YYSTACK_USE_ALLOCA 0

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/proc.h"
#include "node.h"

#define YYLEX_PARAM p

typedef mrb_ast_node node;
typedef struct mrb_parser_state parser_state;
typedef struct mrb_parser_heredoc_info parser_heredoc_info;

static int yylex(void *lval, parser_state *p);
static void yyerror(parser_state *p, const char *s);
static void yywarn(parser_state *p, const char *s);
static void yywarning(parser_state *p, const char *s);
static void backref_error(parser_state *p, node *n);

#ifndef isascii
#define isascii(c) (((c) & ~0x7f) == 0)
#endif

#define identchar(c) (isalnum(c) || (c) == '_' || !isascii(c))

typedef unsigned int stack_type;

#define BITSTACK_PUSH(stack, n) ((stack) = ((stack)<<1)|((n)&1))
#define BITSTACK_POP(stack)     ((stack) = (stack) >> 1)
#define BITSTACK_LEXPOP(stack)  ((stack) = ((stack) >> 1) | ((stack) & 1))
#define BITSTACK_SET_P(stack)   ((stack)&1)

#define COND_PUSH(n)    BITSTACK_PUSH(p->cond_stack, (n))
#define COND_POP()      BITSTACK_POP(p->cond_stack)
#define COND_LEXPOP()   BITSTACK_LEXPOP(p->cond_stack)
#define COND_P()        BITSTACK_SET_P(p->cond_stack)

#define CMDARG_PUSH(n)  BITSTACK_PUSH(p->cmdarg_stack, (n))
#define CMDARG_POP()    BITSTACK_POP(p->cmdarg_stack)
#define CMDARG_LEXPOP() BITSTACK_LEXPOP(p->cmdarg_stack)
#define CMDARG_P()      BITSTACK_SET_P(p->cmdarg_stack)

#define sym(x) ((mrb_sym)(intptr_t)(x))
#define nsym(x) ((node*)(intptr_t)(x))

static inline mrb_sym
intern_gen(parser_state *p, const char *s)
{
  return mrb_intern(p->mrb, s);
}
#define intern(s) intern_gen(p,(s))

static inline mrb_sym
intern_gen2(parser_state *p, const char *s, size_t len)
{
  return mrb_intern2(p->mrb, s, len);
}
#define intern2(s,len) intern_gen2(p,(s),(len))

static inline mrb_sym
intern_gen_c(parser_state *p, const char c)
{
  return mrb_intern2(p->mrb, &c, 1);
}
#define intern_c(c) intern_gen_c(p,(c))

static void
cons_free_gen(parser_state *p, node *cons)
{
  cons->cdr = p->cells;
  p->cells = cons;
}
#define cons_free(c) cons_free_gen(p, (c))

static void*
parser_palloc(parser_state *p, size_t size)
{
  void *m = mrb_pool_alloc(p->pool, size);

  if (!m) {
    longjmp(p->jmp, 1);
  }
  return m;
}

static node*
cons_gen(parser_state *p, node *car, node *cdr)
{
  node *c;

  if (p->cells) {
    c = p->cells;
    p->cells = p->cells->cdr;
  }
  else {
    c = (node *)parser_palloc(p, sizeof(mrb_ast_node));
  }

  c->car = car;
  c->cdr = cdr;
  c->lineno = p->lineno;
  c->filename_index = p->current_filename_index;
  return c;
}
#define cons(a,b) cons_gen(p,(a),(b))

static node*
list1_gen(parser_state *p, node *a)
{
  return cons(a, 0);
}
#define list1(a) list1_gen(p, (a))

static node*
list2_gen(parser_state *p, node *a, node *b)
{
  return cons(a, cons(b,0));
}
#define list2(a,b) list2_gen(p, (a),(b))

static node*
list3_gen(parser_state *p, node *a, node *b, node *c)
{
  return cons(a, cons(b, cons(c,0)));
}
#define list3(a,b,c) list3_gen(p, (a),(b),(c))

static node*
list4_gen(parser_state *p, node *a, node *b, node *c, node *d)
{
  return cons(a, cons(b, cons(c, cons(d, 0))));
}
#define list4(a,b,c,d) list4_gen(p, (a),(b),(c),(d))

static node*
list5_gen(parser_state *p, node *a, node *b, node *c, node *d, node *e)
{
  return cons(a, cons(b, cons(c, cons(d, cons(e, 0)))));
}
#define list5(a,b,c,d,e) list5_gen(p, (a),(b),(c),(d),(e))

static node*
list6_gen(parser_state *p, node *a, node *b, node *c, node *d, node *e, node *f)
{
  return cons(a, cons(b, cons(c, cons(d, cons(e, cons(f, 0))))));
}
#define list6(a,b,c,d,e,f) list6_gen(p, (a),(b),(c),(d),(e),(f))

static node*
append_gen(parser_state *p, node *a, node *b)
{
  node *c = a;

  if (!a) return b;
  while (c->cdr) {
    c = c->cdr;
  }
  if (b) {
    c->cdr = b;
  }
  return a;
}
#define append(a,b) append_gen(p,(a),(b))
#define push(a,b) append_gen(p,(a),list1(b))

static char*
parser_strndup(parser_state *p, const char *s, size_t len)
{
  char *b = (char *)parser_palloc(p, len+1);

  memcpy(b, s, len);
  b[len] = '\0';
  return b;
}
#define strndup(s,len) parser_strndup(p, s, len)

static char*
parser_strdup(parser_state *p, const char *s)
{
  return parser_strndup(p, s, strlen(s));
}
#undef strdup
#define strdup(s) parser_strdup(p, s)

// xxx -----------------------------

static node*
local_switch(parser_state *p)
{
  node *prev = p->locals;

  p->locals = cons(0, 0);
  return prev;
}

static void
local_resume(parser_state *p, node *prev)
{
  p->locals = prev;
}

static void
local_nest(parser_state *p)
{
  p->locals = cons(0, p->locals);
}

static void
local_unnest(parser_state *p)
{
  p->locals = p->locals->cdr;
}

static int
local_var_p(parser_state *p, mrb_sym sym)
{
  node *l = p->locals;

  while (l) {
    node *n = l->car;
    while (n) {
      if (sym(n->car) == sym) return 1;
      n = n->cdr;
    }
    l = l->cdr;
  }
  return 0;
}

static void
local_add_f(parser_state *p, mrb_sym sym)
{
  p->locals->car = push(p->locals->car, nsym(sym));
}

static void
local_add(parser_state *p, mrb_sym sym)
{
  if (!local_var_p(p, sym)) {
    local_add_f(p, sym);
  }
}

// (:scope (vars..) (prog...))
static node*
new_scope(parser_state *p, node *body)
{
  return cons((node*)NODE_SCOPE, cons(p->locals->car, body));
}

// (:begin prog...)
static node*
new_begin(parser_state *p, node *body)
{
  if (body)
    return list2((node*)NODE_BEGIN, body);
  return cons((node*)NODE_BEGIN, 0);
}

#define newline_node(n) (n)

// (:rescue body rescue else)
static node*
new_rescue(parser_state *p, node *body, node *resq, node *els)
{
  return list4((node*)NODE_RESCUE, body, resq, els);
}

// (:ensure body ensure)
static node*
new_ensure(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_ENSURE, cons(a, cons(0, b)));
}

// (:nil)
static node*
new_nil(parser_state *p)
{
  return list1((node*)NODE_NIL);
}

// (:true)
static node*
new_true(parser_state *p)
{
  return list1((node*)NODE_TRUE);
}

// (:false)
static node*
new_false(parser_state *p)
{
  return list1((node*)NODE_FALSE);
}

// (:alias new old)
static node*
new_alias(parser_state *p, mrb_sym a, mrb_sym b)
{
  return cons((node*)NODE_ALIAS, cons(nsym(a), nsym(b)));
}

// (:if cond then else)
static node*
new_if(parser_state *p, node *a, node *b, node *c)
{
  return list4((node*)NODE_IF, a, b, c);
}

// (:unless cond then else)
static node*
new_unless(parser_state *p, node *a, node *b, node *c)
{
  return list4((node*)NODE_IF, a, c, b);
}

// (:while cond body)
static node*
new_while(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_WHILE, cons(a, b));
}

// (:until cond body)
static node*
new_until(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_UNTIL, cons(a, b));
}

// (:for var obj body)
static node*
new_for(parser_state *p, node *v, node *o, node *b)
{
  return list4((node*)NODE_FOR, v, o, b);
}

// (:case a ((when ...) body) ((when...) body))
static node*
new_case(parser_state *p, node *a, node *b)
{
  node *n = list2((node*)NODE_CASE, a);
  node *n2 = n;

  while (n2->cdr) {
    n2 = n2->cdr;
  }
  n2->cdr = b;
  return n;
}

// (:postexe a)
static node*
new_postexe(parser_state *p, node *a)
{
  return cons((node*)NODE_POSTEXE, a);
}

// (:self)
static node*
new_self(parser_state *p)
{
  return list1((node*)NODE_SELF);
}

// (:call a b c)
static node*
new_call(parser_state *p, node *a, mrb_sym b, node *c)
{
  return list4((node*)NODE_CALL, a, nsym(b), c);
}

// (:fcall self mid args)
static node*
new_fcall(parser_state *p, mrb_sym b, node *c)
{
  return list4((node*)NODE_FCALL, new_self(p), nsym(b), c);
}

// (:super . c)
static node*
new_super(parser_state *p, node *c)
{
  return cons((node*)NODE_SUPER, c);
}

// (:zsuper)
static node*
new_zsuper(parser_state *p)
{
  return list1((node*)NODE_ZSUPER);
}

// (:yield . c)
static node*
new_yield(parser_state *p, node *c)
{
  if (c) {
    if (c->cdr) {
      yyerror(p, "both block arg and actual block given");
    }
    return cons((node*)NODE_YIELD, c->car);
  }
  return cons((node*)NODE_YIELD, 0);
}

// (:return . c)
static node*
new_return(parser_state *p, node *c)
{
  return cons((node*)NODE_RETURN, c);
}

// (:break . c)
static node*
new_break(parser_state *p, node *c)
{
  return cons((node*)NODE_BREAK, c);
}

// (:next . c)
static node*
new_next(parser_state *p, node *c)
{
  return cons((node*)NODE_NEXT, c);
}

// (:redo)
static node*
new_redo(parser_state *p)
{
  return list1((node*)NODE_REDO);
}

// (:retry)
static node*
new_retry(parser_state *p)
{
  return list1((node*)NODE_RETRY);
}

// (:dot2 a b)
static node*
new_dot2(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_DOT2, cons(a, b));
}

// (:dot3 a b)
static node*
new_dot3(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_DOT3, cons(a, b));
}

// (:colon2 b c)
static node*
new_colon2(parser_state *p, node *b, mrb_sym c)
{
  return cons((node*)NODE_COLON2, cons(b, nsym(c)));
}

// (:colon3 . c)
static node*
new_colon3(parser_state *p, mrb_sym c)
{
  return cons((node*)NODE_COLON3, nsym(c));
}

// (:and a b)
static node*
new_and(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_AND, cons(a, b));
}

// (:or a b)
static node*
new_or(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_OR, cons(a, b));
}

// (:array a...)
static node*
new_array(parser_state *p, node *a)
{
  return cons((node*)NODE_ARRAY, a);
}

// (:splat . a)
static node*
new_splat(parser_state *p, node *a)
{
  return cons((node*)NODE_SPLAT, a);
}

// (:hash (k . v) (k . v)...)
static node*
new_hash(parser_state *p, node *a)
{
  return cons((node*)NODE_HASH, a);
}

// (:sym . a)
static node*
new_sym(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_SYM, nsym(sym));
}

static mrb_sym
new_strsym(parser_state *p, node* str)
{
  const char *s = (const char*)str->cdr->car;
  size_t len = (size_t)str->cdr->cdr;

  return mrb_intern2(p->mrb, s, len);
}

// (:lvar . a)
static node*
new_lvar(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_LVAR, nsym(sym));
}

// (:gvar . a)
static node*
new_gvar(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_GVAR, nsym(sym));
}

// (:ivar . a)
static node*
new_ivar(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_IVAR, nsym(sym));
}

// (:cvar . a)
static node*
new_cvar(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_CVAR, nsym(sym));
}

// (:const . a)
static node*
new_const(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_CONST, nsym(sym));
}

// (:undef a...)
static node*
new_undef(parser_state *p, mrb_sym sym)
{
  return list2((node*)NODE_UNDEF, nsym(sym));
}

// (:class class super body)
static node*
new_class(parser_state *p, node *c, node *s, node *b)
{
  return list4((node*)NODE_CLASS, c, s, cons(p->locals->car, b));
}

// (:sclass obj body)
static node*
new_sclass(parser_state *p, node *o, node *b)
{
  return list3((node*)NODE_SCLASS, o, cons(p->locals->car, b));
}

// (:module module body)
static node*
new_module(parser_state *p, node *m, node *b)
{
  return list3((node*)NODE_MODULE, m, cons(p->locals->car, b));
}

// (:def m lv (arg . body))
static node*
new_def(parser_state *p, mrb_sym m, node *a, node *b)
{
  return list5((node*)NODE_DEF, nsym(m), p->locals->car, a, b);
}

// (:sdef obj m lv (arg . body))
static node*
new_sdef(parser_state *p, node *o, mrb_sym m, node *a, node *b)
{
  return list6((node*)NODE_SDEF, o, nsym(m), p->locals->car, a, b);
}

// (:arg . sym)
static node*
new_arg(parser_state *p, mrb_sym sym)
{
  return cons((node*)NODE_ARG, nsym(sym));
}

// (m o r m2 b)
// m: (a b c)
// o: ((a . e1) (b . e2))
// r: a
// m2: (a b c)
// b: a
static node*
new_args(parser_state *p, node *m, node *opt, mrb_sym rest, node *m2, mrb_sym blk)
{
  node *n;

  n = cons(m2, nsym(blk));
  n = cons(nsym(rest), n);
  n = cons(opt, n);
  return cons(m, n);
}

// (:block_arg . a)
static node*
new_block_arg(parser_state *p, node *a)
{
  return cons((node*)NODE_BLOCK_ARG, a);
}

// (:block arg body)
static node*
new_block(parser_state *p, node *a, node *b)
{
  return list4((node*)NODE_BLOCK, p->locals->car, a, b);
}

// (:lambda arg body)
static node*
new_lambda(parser_state *p, node *a, node *b)
{
  return list4((node*)NODE_LAMBDA, p->locals->car, a, b);
}

// (:asgn lhs rhs)
static node*
new_asgn(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_ASGN, cons(a, b));
}

// (:masgn mlhs=(pre rest post)  mrhs)
static node*
new_masgn(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_MASGN, cons(a, b));
}

// (:asgn lhs rhs)
static node*
new_op_asgn(parser_state *p, node *a, mrb_sym op, node *b)
{
  return list4((node*)NODE_OP_ASGN, a, nsym(op), b);
}

// (:int . i)
static node*
new_int(parser_state *p, const char *s, int base)
{
  return list3((node*)NODE_INT, (node*)strdup(s), (node*)(intptr_t)base);
}

// (:float . i)
static node*
new_float(parser_state *p, const char *s)
{
  return cons((node*)NODE_FLOAT, (node*)strdup(s));
}

// (:str . (s . len))
static node*
new_str(parser_state *p, const char *s, int len)
{
  return cons((node*)NODE_STR, cons((node*)strndup(s, len), (node*)(intptr_t)len));
}

// (:dstr . a)
static node*
new_dstr(parser_state *p, node *a)
{
  return cons((node*)NODE_DSTR, a);
}

// (:str . (s . len))
static node*
new_xstr(parser_state *p, const char *s, int len)
{
  return cons((node*)NODE_XSTR, cons((node*)strndup(s, len), (node*)(intptr_t)len));
}

// (:xstr . a)
static node*
new_dxstr(parser_state *p, node *a)
{
  return cons((node*)NODE_DXSTR, a);
}

// (:dsym . a)
static node*
new_dsym(parser_state *p, node *a)
{
  return cons((node*)NODE_DSYM, new_dstr(p, a));
}

// (:str . (a . a))
static node*
new_regx(parser_state *p, const char *p1, const char* p2)
{
  return cons((node*)NODE_REGX, cons((node*)p1, (node*)p2));
}

// (:dregx . a)
static node*
new_dregx(parser_state *p, node *a, node *b)
{
  return cons((node*)NODE_DREGX, cons(a, b));
}

// (:backref . n)
static node*
new_back_ref(parser_state *p, int n)
{
  return cons((node*)NODE_BACK_REF, (node*)(intptr_t)n);
}

// (:nthref . n)
static node*
new_nth_ref(parser_state *p, int n)
{
  return cons((node*)NODE_NTH_REF, (node*)(intptr_t)n);
}

// (:heredoc . a)
static node*
new_heredoc(parser_state *p)
{
  parser_heredoc_info *inf = (parser_heredoc_info *)parser_palloc(p, sizeof(parser_heredoc_info));
  return cons((node*)NODE_HEREDOC, (node*)inf);
}

static void
new_bv(parser_state *p, mrb_sym id)
{
}

static node*
new_literal_delim(parser_state *p)
{
  return cons((node*)NODE_LITERAL_DELIM, 0);
}

// (:words . a)
static node*
new_words(parser_state *p, node *a)
{
  return cons((node*)NODE_WORDS, a);
}

// (:symbols . a)
static node*
new_symbols(parser_state *p, node *a)
{
  return cons((node*)NODE_SYMBOLS, a);
}

// xxx -----------------------------

// (:call a op)
static node*
call_uni_op(parser_state *p, node *recv, char *m)
{
  return new_call(p, recv, intern(m), 0);
}

// (:call a op b)
static node*
call_bin_op(parser_state *p, node *recv, char *m, node *arg1)
{
  return new_call(p, recv, intern(m), list1(list1(arg1)));
}

static void
args_with_block(parser_state *p, node *a, node *b)
{
  if (b) {
    if (a->cdr) {
      yyerror(p, "both block arg and actual block given");
    }
    a->cdr = b;
  }
}

static void
call_with_block(parser_state *p, node *a, node *b)
{
  node *n;

  if (a->car == (node*)NODE_SUPER ||
      a->car == (node*)NODE_ZSUPER) {
    if (!a->cdr) a->cdr = cons(0, b);
    else {
      args_with_block(p, a->cdr, b);
    }
  }
  else {
    n = a->cdr->cdr->cdr;
    if (!n->car) n->car = cons(0, b);
    else {
      args_with_block(p, n->car, b);
    }
  }
}

static node*
negate_lit(parser_state *p, node *n)
{
  return cons((node*)NODE_NEGATE, n);
}

static node*
cond(node *n)
{
  return n;
}

static node*
ret_args(parser_state *p, node *n)
{
  if (n->cdr) {
    yyerror(p, "block argument should not be given");
    return NULL;
  }
  if (!n->car->cdr) return n->car->car;
  return new_array(p, n->car);
}

static void
assignable(parser_state *p, node *lhs)
{
  if ((int)(intptr_t)lhs->car == NODE_LVAR) {
    local_add(p, sym(lhs->cdr));
  }
}

static node*
var_reference(parser_state *p, node *lhs)
{
  node *n;

  if ((int)(intptr_t)lhs->car == NODE_LVAR) {
    if (!local_var_p(p, sym(lhs->cdr))) {
      n = new_fcall(p, sym(lhs->cdr), 0);
      cons_free(lhs);
      return n;
    }
  }

  return lhs;
}

typedef enum mrb_string_type  string_type;

static node*
new_strterm(parser_state *p, string_type type, int term, int paren)
{
  return cons((node*)(intptr_t)type, cons((node*)0, cons((node*)(intptr_t)paren, (node*)(intptr_t)term)));
}

static void
end_strterm(parser_state *p)
{
  cons_free(p->lex_strterm->cdr->cdr);
  cons_free(p->lex_strterm->cdr);
  cons_free(p->lex_strterm);
  p->lex_strterm = NULL;
}

parser_heredoc_info *
parsing_heredoc_inf(parser_state *p)
{
  node *nd = p->parsing_heredoc;
  if (nd == NULL)
    return NULL;
  /* mrb_assert(nd->car->car == NODE_HEREDOC); */
  return (parser_heredoc_info*)nd->car->cdr;
}

static void
heredoc_end(parser_state *p)
{
  p->parsing_heredoc = p->parsing_heredoc->cdr;
  if (p->parsing_heredoc == NULL) {
    p->lstate = EXPR_BEG;
    p->cmd_start = TRUE;
    end_strterm(p);
    p->heredoc_end_now = TRUE;
  } else {
    /* next heredoc */
    p->lex_strterm->car = (node*)(intptr_t)parsing_heredoc_inf(p)->type;
  }
}
#define is_strterm_type(p,str_func) ((int)(intptr_t)((p)->lex_strterm->car) & (str_func))

// xxx -----------------------------


/* Line 371 of yacc.c  */
#line 999 "/home/kou/work/c/groonga.central/vendor/mruby/mruby.master/build/host/src/y.tab.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     keyword_class = 258,
     keyword_module = 259,
     keyword_def = 260,
     keyword_undef = 261,
     keyword_begin = 262,
     keyword_rescue = 263,
     keyword_ensure = 264,
     keyword_end = 265,
     keyword_if = 266,
     keyword_unless = 267,
     keyword_then = 268,
     keyword_elsif = 269,
     keyword_else = 270,
     keyword_case = 271,
     keyword_when = 272,
     keyword_while = 273,
     keyword_until = 274,
     keyword_for = 275,
     keyword_break = 276,
     keyword_next = 277,
     keyword_redo = 278,
     keyword_retry = 279,
     keyword_in = 280,
     keyword_do = 281,
     keyword_do_cond = 282,
     keyword_do_block = 283,
     keyword_do_LAMBDA = 284,
     keyword_return = 285,
     keyword_yield = 286,
     keyword_super = 287,
     keyword_self = 288,
     keyword_nil = 289,
     keyword_true = 290,
     keyword_false = 291,
     keyword_and = 292,
     keyword_or = 293,
     keyword_not = 294,
     modifier_if = 295,
     modifier_unless = 296,
     modifier_while = 297,
     modifier_until = 298,
     modifier_rescue = 299,
     keyword_alias = 300,
     keyword_BEGIN = 301,
     keyword_END = 302,
     keyword__LINE__ = 303,
     keyword__FILE__ = 304,
     keyword__ENCODING__ = 305,
     tIDENTIFIER = 306,
     tFID = 307,
     tGVAR = 308,
     tIVAR = 309,
     tCONSTANT = 310,
     tCVAR = 311,
     tLABEL = 312,
     tINTEGER = 313,
     tFLOAT = 314,
     tCHAR = 315,
     tXSTRING = 316,
     tREGEXP = 317,
     tSTRING = 318,
     tSTRING_PART = 319,
     tSTRING_MID = 320,
     tNTH_REF = 321,
     tBACK_REF = 322,
     tREGEXP_END = 323,
     tUPLUS = 324,
     tUMINUS = 325,
     tPOW = 326,
     tCMP = 327,
     tEQ = 328,
     tEQQ = 329,
     tNEQ = 330,
     tGEQ = 331,
     tLEQ = 332,
     tANDOP = 333,
     tOROP = 334,
     tMATCH = 335,
     tNMATCH = 336,
     tDOT2 = 337,
     tDOT3 = 338,
     tAREF = 339,
     tASET = 340,
     tLSHFT = 341,
     tRSHFT = 342,
     tCOLON2 = 343,
     tCOLON3 = 344,
     tOP_ASGN = 345,
     tASSOC = 346,
     tLPAREN = 347,
     tLPAREN_ARG = 348,
     tRPAREN = 349,
     tLBRACK = 350,
     tLBRACE = 351,
     tLBRACE_ARG = 352,
     tSTAR = 353,
     tAMPER = 354,
     tLAMBDA = 355,
     tSYMBEG = 356,
     tREGEXP_BEG = 357,
     tWORDS_BEG = 358,
     tSYMBOLS_BEG = 359,
     tSTRING_BEG = 360,
     tXSTRING_BEG = 361,
     tSTRING_DVAR = 362,
     tLAMBEG = 363,
     tHEREDOC_BEG = 364,
     tHEREDOC_END = 365,
     tLITERAL_DELIM = 366,
     tLOWEST = 367,
     tUMINUS_NUM = 368,
     idNULL = 369,
     idRespond_to = 370,
     idIFUNC = 371,
     idCFUNC = 372,
     id_core_set_method_alias = 373,
     id_core_set_variable_alias = 374,
     id_core_undef_method = 375,
     id_core_define_method = 376,
     id_core_define_singleton_method = 377,
     id_core_set_postexe = 378,
     tLAST_TOKEN = 379
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 942 "src/parse.y"

    node *nd;
    mrb_sym id;
    int num;
    unsigned int stack;
    const struct vtable *vars;


/* Line 387 of yacc.c  */
#line 1172 "/home/kou/work/c/groonga.central/vendor/mruby/mruby.master/build/host/src/y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (parser_state *p);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 1199 "/home/kou/work/c/groonga.central/vendor/mruby/mruby.master/build/host/src/y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif


/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   10895

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  151
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  154
/* YYNRULES -- Number of rules.  */
#define YYNRULES  544
/* YYNRULES -- Number of states.  */
#define YYNSTATES  956

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   379

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     150,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   127,     2,     2,     2,   125,   120,     2,
     146,   147,   123,   121,   144,   122,   143,   124,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   115,   149,
     117,   113,   116,   114,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   142,     2,   148,   119,     2,   145,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   140,   118,   141,   128,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   126,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    12,    14,    18,    21,
      23,    24,    30,    35,    38,    40,    42,    46,    49,    50,
      55,    58,    62,    66,    70,    74,    78,    83,    85,    89,
      93,   100,   106,   112,   118,   124,   128,   132,   136,   140,
     142,   146,   150,   152,   156,   160,   164,   167,   169,   171,
     173,   175,   177,   182,   183,   189,   192,   196,   201,   207,
     212,   218,   221,   224,   227,   230,   233,   235,   239,   241,
     245,   247,   250,   254,   260,   263,   268,   271,   276,   278,
     282,   284,   288,   291,   295,   297,   300,   302,   307,   311,
     315,   319,   323,   326,   328,   330,   335,   339,   343,   347,
     351,   354,   356,   358,   360,   363,   365,   369,   371,   373,
     375,   377,   379,   381,   383,   385,   386,   391,   393,   395,
     397,   399,   401,   403,   405,   407,   409,   411,   413,   415,
     417,   419,   421,   423,   425,   427,   429,   431,   433,   435,
     437,   439,   441,   443,   445,   447,   449,   451,   453,   455,
     457,   459,   461,   463,   465,   467,   469,   471,   473,   475,
     477,   479,   481,   483,   485,   487,   489,   491,   493,   495,
     497,   499,   501,   503,   505,   507,   509,   511,   513,   515,
     517,   519,   521,   523,   525,   527,   529,   533,   539,   543,
     549,   556,   562,   568,   574,   580,   585,   589,   593,   597,
     601,   605,   609,   613,   617,   621,   626,   631,   634,   637,
     641,   645,   649,   653,   657,   661,   665,   669,   673,   677,
     681,   685,   689,   692,   695,   699,   703,   707,   711,   718,
     720,   722,   724,   727,   732,   735,   739,   741,   743,   745,
     747,   750,   755,   758,   760,   763,   766,   771,   773,   774,
     777,   780,   783,   785,   787,   790,   794,   799,   803,   808,
     811,   813,   815,   817,   819,   821,   823,   825,   827,   828,
     833,   834,   839,   840,   844,   848,   852,   855,   859,   863,
     865,   870,   874,   876,   881,   885,   888,   890,   893,   894,
     899,   906,   913,   914,   915,   923,   924,   925,   933,   939,
     944,   945,   946,   956,   957,   964,   965,   966,   975,   976,
     982,   983,   990,   991,   992,  1002,  1004,  1006,  1008,  1010,
    1012,  1014,  1016,  1019,  1021,  1023,  1025,  1031,  1033,  1036,
    1038,  1040,  1042,  1046,  1048,  1052,  1054,  1059,  1066,  1070,
    1076,  1079,  1084,  1086,  1090,  1097,  1106,  1111,  1118,  1123,
    1126,  1133,  1136,  1141,  1148,  1151,  1156,  1159,  1164,  1166,
    1168,  1170,  1174,  1176,  1181,  1183,  1188,  1190,  1194,  1196,
    1198,  1203,  1205,  1209,  1213,  1214,  1220,  1223,  1228,  1234,
    1240,  1243,  1248,  1253,  1257,  1261,  1265,  1268,  1270,  1275,
    1276,  1282,  1283,  1289,  1295,  1297,  1299,  1306,  1308,  1310,
    1312,  1314,  1317,  1319,  1322,  1324,  1326,  1328,  1330,  1332,
    1334,  1336,  1339,  1343,  1345,  1348,  1350,  1351,  1356,  1358,
    1361,  1365,  1368,  1372,  1374,  1376,  1378,  1380,  1383,  1385,
    1388,  1391,  1395,  1397,  1402,  1405,  1407,  1409,  1411,  1413,
    1415,  1418,  1421,  1425,  1427,  1429,  1432,  1435,  1437,  1439,
    1441,  1443,  1445,  1447,  1449,  1451,  1453,  1455,  1457,  1459,
    1461,  1463,  1465,  1467,  1468,  1473,  1476,  1480,  1483,  1490,
    1499,  1504,  1511,  1516,  1523,  1526,  1531,  1538,  1541,  1546,
    1549,  1554,  1556,  1557,  1559,  1561,  1563,  1565,  1567,  1569,
    1571,  1575,  1577,  1581,  1585,  1589,  1591,  1595,  1597,  1601,
    1603,  1605,  1608,  1610,  1612,  1614,  1617,  1620,  1622,  1624,
    1625,  1630,  1632,  1635,  1637,  1641,  1645,  1648,  1650,  1652,
    1654,  1656,  1658,  1660,  1662,  1664,  1666,  1668,  1670,  1672,
    1673,  1675,  1676,  1678,  1681,  1684,  1685,  1687,  1689,  1691,
    1693,  1694,  1698,  1700,  1703
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     152,     0,    -1,    -1,   153,   154,    -1,   155,   295,    -1,
     304,    -1,   156,    -1,   155,   303,   156,    -1,     1,   156,
      -1,   161,    -1,    -1,    46,   157,   140,   154,   141,    -1,
     159,   244,   222,   247,    -1,   160,   295,    -1,   304,    -1,
     161,    -1,   160,   303,   161,    -1,     1,   161,    -1,    -1,
      45,   182,   162,   182,    -1,     6,   183,    -1,   161,    40,
     165,    -1,   161,    41,   165,    -1,   161,    42,   165,    -1,
     161,    43,   165,    -1,   161,    44,   161,    -1,    47,   140,
     159,   141,    -1,   163,    -1,   171,   113,   166,    -1,   266,
      90,   166,    -1,   218,   142,   192,   298,    90,   166,    -1,
     218,   143,    51,    90,   166,    -1,   218,   143,    55,    90,
     166,    -1,   218,    88,    55,    90,   166,    -1,   218,    88,
      51,    90,   166,    -1,   268,    90,   166,    -1,   178,   113,
     199,    -1,   171,   113,   188,    -1,   171,   113,   199,    -1,
     164,    -1,   178,   113,   166,    -1,   178,   113,   163,    -1,
     166,    -1,   164,    37,   164,    -1,   164,    38,   164,    -1,
      39,   296,   164,    -1,   127,   166,    -1,   187,    -1,   164,
      -1,   170,    -1,   167,    -1,   237,    -1,   237,   294,   292,
     194,    -1,    -1,    97,   169,   228,   159,   141,    -1,   291,
     194,    -1,   291,   194,   168,    -1,   218,   143,   292,   194,
      -1,   218,   143,   292,   194,   168,    -1,   218,    88,   292,
     194,    -1,   218,    88,   292,   194,   168,    -1,    32,   194,
      -1,    31,   194,    -1,    30,   193,    -1,    21,   193,    -1,
      22,   193,    -1,   173,    -1,    92,   172,   297,    -1,   173,
      -1,    92,   172,   297,    -1,   175,    -1,   175,   174,    -1,
     175,    98,   177,    -1,   175,    98,   177,   144,   176,    -1,
     175,    98,    -1,   175,    98,   144,   176,    -1,    98,   177,
      -1,    98,   177,   144,   176,    -1,    98,    -1,    98,   144,
     176,    -1,   177,    -1,    92,   172,   297,    -1,   174,   144,
      -1,   175,   174,   144,    -1,   174,    -1,   175,   174,    -1,
     265,    -1,   218,   142,   192,   298,    -1,   218,   143,    51,
      -1,   218,    88,    51,    -1,   218,   143,    55,    -1,   218,
      88,    55,    -1,    89,    55,    -1,   268,    -1,   265,    -1,
     218,   142,   192,   298,    -1,   218,   143,    51,    -1,   218,
      88,    51,    -1,   218,   143,    55,    -1,   218,    88,    55,
      -1,    89,    55,    -1,   268,    -1,    51,    -1,    55,    -1,
      89,   179,    -1,   179,    -1,   218,    88,   179,    -1,    51,
      -1,    55,    -1,    52,    -1,   185,    -1,   186,    -1,   181,
      -1,   261,    -1,   182,    -1,    -1,   183,   144,   184,   182,
      -1,   118,    -1,   119,    -1,   120,    -1,    72,    -1,    73,
      -1,    74,    -1,    80,    -1,    81,    -1,   116,    -1,    76,
      -1,   117,    -1,    77,    -1,    75,    -1,    86,    -1,    87,
      -1,   121,    -1,   122,    -1,   123,    -1,    98,    -1,   124,
      -1,   125,    -1,    71,    -1,   127,    -1,   128,    -1,    69,
      -1,    70,    -1,    84,    -1,    85,    -1,   145,    -1,    48,
      -1,    49,    -1,    50,    -1,    46,    -1,    47,    -1,    45,
      -1,    37,    -1,     7,    -1,    21,    -1,    16,    -1,     3,
      -1,     5,    -1,    26,    -1,    15,    -1,    14,    -1,    10,
      -1,     9,    -1,    36,    -1,    20,    -1,    25,    -1,     4,
      -1,    22,    -1,    34,    -1,    39,    -1,    38,    -1,    23,
      -1,     8,    -1,    24,    -1,    30,    -1,    33,    -1,    32,
      -1,    13,    -1,    35,    -1,     6,    -1,    17,    -1,    31,
      -1,    11,    -1,    12,    -1,    18,    -1,    19,    -1,   178,
     113,   187,    -1,   178,   113,   187,    44,   187,    -1,   266,
      90,   187,    -1,   266,    90,   187,    44,   187,    -1,   218,
     142,   192,   298,    90,   187,    -1,   218,   143,    51,    90,
     187,    -1,   218,   143,    55,    90,   187,    -1,   218,    88,
      51,    90,   187,    -1,   218,    88,    55,    90,   187,    -1,
      89,    55,    90,   187,    -1,   268,    90,   187,    -1,   187,
      82,   187,    -1,   187,    83,   187,    -1,   187,   121,   187,
      -1,   187,   122,   187,    -1,   187,   123,   187,    -1,   187,
     124,   187,    -1,   187,   125,   187,    -1,   187,    71,   187,
      -1,   126,    58,    71,   187,    -1,   126,    59,    71,   187,
      -1,    69,   187,    -1,    70,   187,    -1,   187,   118,   187,
      -1,   187,   119,   187,    -1,   187,   120,   187,    -1,   187,
      72,   187,    -1,   187,   116,   187,    -1,   187,    76,   187,
      -1,   187,   117,   187,    -1,   187,    77,   187,    -1,   187,
      73,   187,    -1,   187,    74,   187,    -1,   187,    75,   187,
      -1,   187,    80,   187,    -1,   187,    81,   187,    -1,   127,
     187,    -1,   128,   187,    -1,   187,    86,   187,    -1,   187,
      87,   187,    -1,   187,    78,   187,    -1,   187,    79,   187,
      -1,   187,   114,   187,   296,   115,   187,    -1,   200,    -1,
     187,    -1,   304,    -1,   198,   299,    -1,   198,   144,   289,
     299,    -1,   289,   299,    -1,   146,   192,   297,    -1,   304,
      -1,   190,    -1,   304,    -1,   193,    -1,   198,   144,    -1,
     198,   144,   289,   144,    -1,   289,   144,    -1,   170,    -1,
     198,   197,    -1,   289,   197,    -1,   198,   144,   289,   197,
      -1,   196,    -1,    -1,   195,   193,    -1,    99,   188,    -1,
     144,   196,    -1,   304,    -1,   188,    -1,    98,   188,    -1,
     198,   144,   188,    -1,   198,   144,    98,   188,    -1,   198,
     144,   188,    -1,   198,   144,    98,   188,    -1,    98,   188,
      -1,   248,    -1,   249,    -1,   253,    -1,   254,    -1,   255,
      -1,   267,    -1,   268,    -1,    52,    -1,    -1,     7,   201,
     158,    10,    -1,    -1,    93,   164,   202,   297,    -1,    -1,
      93,   203,   297,    -1,    92,   159,   147,    -1,   218,    88,
      55,    -1,    89,    55,    -1,    95,   189,   148,    -1,    96,
     288,   141,    -1,    30,    -1,    31,   146,   193,   297,    -1,
      31,   146,   297,    -1,    31,    -1,    39,   146,   164,   297,
      -1,    39,   146,   297,    -1,   291,   239,    -1,   238,    -1,
     238,   239,    -1,    -1,   100,   204,   233,   234,    -1,    11,
     165,   219,   159,   221,    10,    -1,    12,   165,   219,   159,
     222,    10,    -1,    -1,    -1,    18,   205,   165,   220,   206,
     159,    10,    -1,    -1,    -1,    19,   207,   165,   220,   208,
     159,    10,    -1,    16,   165,   295,   242,    10,    -1,    16,
     295,   242,    10,    -1,    -1,    -1,    20,   223,    25,   209,
     165,   220,   210,   159,    10,    -1,    -1,     3,   180,   269,
     211,   158,    10,    -1,    -1,    -1,     3,    86,   164,   212,
     300,   213,   158,    10,    -1,    -1,     4,   180,   214,   158,
      10,    -1,    -1,     5,   181,   215,   271,   158,    10,    -1,
      -1,    -1,     5,   286,   294,   216,   181,   217,   271,   158,
      10,    -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,
     200,    -1,   300,    -1,    13,    -1,   300,    13,    -1,   300,
      -1,    27,    -1,   222,    -1,    14,   165,   219,   159,   221,
      -1,   304,    -1,    15,   159,    -1,   178,    -1,   171,    -1,
     274,    -1,    92,   226,   297,    -1,   224,    -1,   225,   144,
     224,    -1,   225,    -1,   225,   144,    98,   274,    -1,   225,
     144,    98,   274,   144,   225,    -1,   225,   144,    98,    -1,
     225,   144,    98,   144,   225,    -1,    98,   274,    -1,    98,
     274,   144,   225,    -1,    98,    -1,    98,   144,   225,    -1,
     276,   144,   279,   144,   282,   285,    -1,   276,   144,   279,
     144,   282,   144,   276,   285,    -1,   276,   144,   279,   285,
      -1,   276,   144,   279,   144,   276,   285,    -1,   276,   144,
     282,   285,    -1,   276,   144,    -1,   276,   144,   282,   144,
     276,   285,    -1,   276,   285,    -1,   279,   144,   282,   285,
      -1,   279,   144,   282,   144,   276,   285,    -1,   279,   285,
      -1,   279,   144,   276,   285,    -1,   282,   285,    -1,   282,
     144,   276,   285,    -1,   284,    -1,   304,    -1,   229,    -1,
     118,   230,   118,    -1,    79,    -1,   118,   227,   230,   118,
      -1,   296,    -1,   296,   149,   231,   296,    -1,   232,    -1,
     231,   144,   232,    -1,    51,    -1,   273,    -1,   146,   272,
     230,   147,    -1,   272,    -1,   108,   159,   141,    -1,    29,
     159,    10,    -1,    -1,    28,   236,   228,   159,    10,    -1,
     170,   235,    -1,   237,   294,   292,   191,    -1,   237,   294,
     292,   191,   239,    -1,   237,   294,   292,   194,   235,    -1,
     291,   190,    -1,   218,   143,   292,   191,    -1,   218,    88,
     292,   190,    -1,   218,    88,   293,    -1,   218,   143,   190,
      -1,   218,    88,   190,    -1,    32,   190,    -1,    32,    -1,
     218,   142,   192,   298,    -1,    -1,   140,   240,   228,   159,
     141,    -1,    -1,    26,   241,   228,   159,    10,    -1,    17,
     198,   219,   159,   243,    -1,   222,    -1,   242,    -1,     8,
     245,   246,   219,   159,   244,    -1,   304,    -1,   188,    -1,
     199,    -1,   304,    -1,    91,   178,    -1,   304,    -1,     9,
     159,    -1,   304,    -1,   264,    -1,   260,    -1,   259,    -1,
     263,    -1,    60,    -1,    63,    -1,   105,    63,    -1,   105,
     250,    63,    -1,   251,    -1,   250,   251,    -1,    65,    -1,
      -1,    64,   252,   159,   141,    -1,   111,    -1,   106,    61,
      -1,   106,   250,    61,    -1,   102,    62,    -1,   102,   250,
      62,    -1,   109,    -1,   304,    -1,   257,    -1,   258,    -1,
     257,   258,    -1,   110,    -1,   250,   110,    -1,   103,    63,
      -1,   103,   250,    63,    -1,   261,    -1,   101,   105,   251,
      63,    -1,   101,   262,    -1,   181,    -1,    54,    -1,    53,
      -1,    56,    -1,    63,    -1,   105,    63,    -1,   104,    63,
      -1,   104,   250,    63,    -1,    58,    -1,    59,    -1,   126,
      58,    -1,   126,    59,    -1,    51,    -1,    54,    -1,    53,
      -1,    56,    -1,    55,    -1,   265,    -1,   265,    -1,    34,
      -1,    33,    -1,    35,    -1,    36,    -1,    49,    -1,    48,
      -1,    66,    -1,    67,    -1,   300,    -1,    -1,   117,   270,
     165,   300,    -1,     1,   300,    -1,   146,   272,   297,    -1,
     272,   300,    -1,   276,   144,   280,   144,   282,   285,    -1,
     276,   144,   280,   144,   282,   144,   276,   285,    -1,   276,
     144,   280,   285,    -1,   276,   144,   280,   144,   276,   285,
      -1,   276,   144,   282,   285,    -1,   276,   144,   282,   144,
     276,   285,    -1,   276,   285,    -1,   280,   144,   282,   285,
      -1,   280,   144,   282,   144,   276,   285,    -1,   280,   285,
      -1,   280,   144,   276,   285,    -1,   282,   285,    -1,   282,
     144,   276,   285,    -1,   284,    -1,    -1,    55,    -1,    54,
      -1,    53,    -1,    56,    -1,   273,    -1,    51,    -1,   274,
      -1,    92,   226,   297,    -1,   275,    -1,   276,   144,   275,
      -1,    51,   113,   188,    -1,    51,   113,   218,    -1,   278,
      -1,   279,   144,   278,    -1,   277,    -1,   280,   144,   277,
      -1,   123,    -1,    98,    -1,   281,    51,    -1,   281,    -1,
     120,    -1,    99,    -1,   283,    51,    -1,   144,   284,    -1,
     304,    -1,   267,    -1,    -1,   146,   287,   164,   297,    -1,
     304,    -1,   289,   299,    -1,   290,    -1,   289,   144,   290,
      -1,   188,    91,   188,    -1,    57,   188,    -1,    51,    -1,
      55,    -1,    52,    -1,    51,    -1,    55,    -1,    52,    -1,
     185,    -1,    51,    -1,    52,    -1,   185,    -1,   143,    -1,
      88,    -1,    -1,   303,    -1,    -1,   301,    -1,   296,   147,
      -1,   296,   148,    -1,    -1,   301,    -1,   144,    -1,   149,
      -1,   301,    -1,    -1,   150,   302,   256,    -1,   300,    -1,
     303,   149,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1100,  1100,  1100,  1110,  1116,  1120,  1124,  1128,  1134,
    1136,  1135,  1147,  1173,  1179,  1183,  1187,  1191,  1197,  1197,
    1201,  1205,  1209,  1213,  1217,  1221,  1225,  1230,  1231,  1235,
    1239,  1243,  1247,  1251,  1256,  1260,  1265,  1269,  1273,  1277,
    1280,  1284,  1291,  1292,  1296,  1300,  1304,  1308,  1311,  1318,
    1319,  1322,  1323,  1327,  1326,  1339,  1343,  1348,  1352,  1357,
    1361,  1366,  1370,  1374,  1378,  1382,  1388,  1392,  1398,  1399,
    1405,  1409,  1413,  1417,  1421,  1425,  1429,  1433,  1437,  1441,
    1447,  1448,  1454,  1458,  1464,  1468,  1474,  1478,  1482,  1486,
    1490,  1494,  1500,  1506,  1513,  1517,  1521,  1525,  1529,  1533,
    1539,  1545,  1552,  1556,  1559,  1563,  1567,  1573,  1574,  1575,
    1576,  1581,  1588,  1589,  1592,  1596,  1596,  1602,  1603,  1604,
    1605,  1606,  1607,  1608,  1609,  1610,  1611,  1612,  1613,  1614,
    1615,  1616,  1617,  1618,  1619,  1620,  1621,  1622,  1623,  1624,
    1625,  1626,  1627,  1628,  1629,  1630,  1633,  1633,  1633,  1634,
    1634,  1635,  1635,  1635,  1636,  1636,  1636,  1636,  1637,  1637,
    1637,  1638,  1638,  1638,  1639,  1639,  1639,  1639,  1640,  1640,
    1640,  1640,  1641,  1641,  1641,  1641,  1642,  1642,  1642,  1642,
    1643,  1643,  1643,  1643,  1644,  1644,  1647,  1651,  1655,  1659,
    1663,  1667,  1671,  1675,  1679,  1684,  1689,  1694,  1698,  1702,
    1706,  1710,  1714,  1718,  1722,  1726,  1730,  1734,  1738,  1742,
    1746,  1750,  1754,  1758,  1762,  1766,  1770,  1774,  1778,  1782,
    1786,  1790,  1794,  1798,  1802,  1806,  1810,  1814,  1818,  1822,
    1828,  1835,  1836,  1840,  1844,  1850,  1856,  1857,  1860,  1861,
    1862,  1866,  1870,  1876,  1880,  1884,  1888,  1892,  1898,  1898,
    1909,  1915,  1919,  1925,  1929,  1933,  1937,  1943,  1947,  1951,
    1957,  1958,  1959,  1960,  1961,  1962,  1963,  1964,  1969,  1968,
    1979,  1979,  1983,  1983,  1987,  1991,  1995,  1999,  2003,  2007,
    2011,  2015,  2019,  2023,  2027,  2031,  2035,  2036,  2042,  2041,
    2054,  2061,  2068,  2068,  2068,  2074,  2074,  2074,  2080,  2086,
    2091,  2093,  2090,  2100,  2099,  2112,  2117,  2111,  2130,  2129,
    2142,  2141,  2154,  2155,  2154,  2168,  2172,  2176,  2180,  2186,
    2193,  2194,  2195,  2198,  2199,  2202,  2203,  2211,  2212,  2218,
    2222,  2225,  2229,  2235,  2239,  2245,  2249,  2253,  2257,  2261,
    2265,  2269,  2273,  2277,  2283,  2287,  2291,  2295,  2299,  2303,
    2307,  2311,  2315,  2319,  2323,  2327,  2331,  2335,  2339,  2345,
    2346,  2353,  2358,  2363,  2370,  2374,  2380,  2381,  2384,  2389,
    2392,  2396,  2402,  2406,  2413,  2412,  2425,  2435,  2439,  2444,
    2451,  2455,  2459,  2463,  2467,  2471,  2475,  2479,  2483,  2490,
    2489,  2500,  2499,  2511,  2519,  2528,  2531,  2538,  2541,  2545,
    2546,  2549,  2553,  2556,  2560,  2563,  2564,  2565,  2566,  2569,
    2570,  2571,  2575,  2581,  2582,  2588,  2593,  2592,  2603,  2609,
    2613,  2619,  2623,  2629,  2632,  2633,  2636,  2637,  2640,  2645,
    2652,  2656,  2663,  2667,  2674,  2681,  2682,  2683,  2684,  2685,
    2689,  2695,  2699,  2705,  2706,  2707,  2711,  2717,  2721,  2725,
    2729,  2733,  2739,  2745,  2749,  2753,  2757,  2761,  2765,  2772,
    2781,  2782,  2785,  2790,  2789,  2798,  2805,  2811,  2817,  2821,
    2825,  2829,  2833,  2837,  2841,  2845,  2849,  2853,  2857,  2861,
    2865,  2869,  2874,  2880,  2885,  2890,  2895,  2902,  2906,  2913,
    2917,  2923,  2927,  2933,  2940,  2947,  2951,  2957,  2961,  2967,
    2968,  2971,  2976,  2983,  2984,  2987,  2994,  2998,  3005,  3010,
    3010,  3035,  3036,  3042,  3046,  3052,  3056,  3062,  3063,  3064,
    3067,  3068,  3069,  3070,  3073,  3074,  3075,  3078,  3079,  3082,
    3083,  3086,  3087,  3090,  3093,  3096,  3097,  3098,  3101,  3102,
    3106,  3105,  3112,  3113,  3117
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "keyword_class", "keyword_module",
  "keyword_def", "keyword_undef", "keyword_begin", "keyword_rescue",
  "keyword_ensure", "keyword_end", "keyword_if", "keyword_unless",
  "keyword_then", "keyword_elsif", "keyword_else", "keyword_case",
  "keyword_when", "keyword_while", "keyword_until", "keyword_for",
  "keyword_break", "keyword_next", "keyword_redo", "keyword_retry",
  "keyword_in", "keyword_do", "keyword_do_cond", "keyword_do_block",
  "keyword_do_LAMBDA", "keyword_return", "keyword_yield", "keyword_super",
  "keyword_self", "keyword_nil", "keyword_true", "keyword_false",
  "keyword_and", "keyword_or", "keyword_not", "modifier_if",
  "modifier_unless", "modifier_while", "modifier_until", "modifier_rescue",
  "keyword_alias", "keyword_BEGIN", "keyword_END", "keyword__LINE__",
  "keyword__FILE__", "keyword__ENCODING__", "tIDENTIFIER", "tFID", "tGVAR",
  "tIVAR", "tCONSTANT", "tCVAR", "tLABEL", "tINTEGER", "tFLOAT", "tCHAR",
  "tXSTRING", "tREGEXP", "tSTRING", "tSTRING_PART", "tSTRING_MID",
  "tNTH_REF", "tBACK_REF", "tREGEXP_END", "tUPLUS", "tUMINUS", "tPOW",
  "tCMP", "tEQ", "tEQQ", "tNEQ", "tGEQ", "tLEQ", "tANDOP", "tOROP",
  "tMATCH", "tNMATCH", "tDOT2", "tDOT3", "tAREF", "tASET", "tLSHFT",
  "tRSHFT", "tCOLON2", "tCOLON3", "tOP_ASGN", "tASSOC", "tLPAREN",
  "tLPAREN_ARG", "tRPAREN", "tLBRACK", "tLBRACE", "tLBRACE_ARG", "tSTAR",
  "tAMPER", "tLAMBDA", "tSYMBEG", "tREGEXP_BEG", "tWORDS_BEG",
  "tSYMBOLS_BEG", "tSTRING_BEG", "tXSTRING_BEG", "tSTRING_DVAR", "tLAMBEG",
  "tHEREDOC_BEG", "tHEREDOC_END", "tLITERAL_DELIM", "tLOWEST", "'='",
  "'?'", "':'", "'>'", "'<'", "'|'", "'^'", "'&'", "'+'", "'-'", "'*'",
  "'/'", "'%'", "tUMINUS_NUM", "'!'", "'~'", "idNULL", "idRespond_to",
  "idIFUNC", "idCFUNC", "id_core_set_method_alias",
  "id_core_set_variable_alias", "id_core_undef_method",
  "id_core_define_method", "id_core_define_singleton_method",
  "id_core_set_postexe", "tLAST_TOKEN", "'{'", "'}'", "'['", "'.'", "','",
  "'`'", "'('", "')'", "']'", "';'", "'\\n'", "$accept", "program", "$@1",
  "top_compstmt", "top_stmts", "top_stmt", "@2", "bodystmt", "compstmt",
  "stmts", "stmt", "$@3", "command_asgn", "expr", "expr_value",
  "command_call", "block_command", "cmd_brace_block", "$@4", "command",
  "mlhs", "mlhs_inner", "mlhs_basic", "mlhs_item", "mlhs_list",
  "mlhs_post", "mlhs_node", "lhs", "cname", "cpath", "fname", "fsym",
  "undef_list", "$@5", "op", "reswords", "arg", "arg_value", "aref_args",
  "paren_args", "opt_paren_args", "opt_call_args", "call_args",
  "command_args", "@6", "block_arg", "opt_block_arg", "args", "mrhs",
  "primary", "$@7", "$@8", "$@9", "@10", "$@11", "$@12", "$@13", "$@14",
  "$@15", "$@16", "@17", "@18", "@19", "@20", "@21", "$@22", "@23",
  "primary_value", "then", "do", "if_tail", "opt_else", "for_var",
  "f_marg", "f_marg_list", "f_margs", "block_param", "opt_block_param",
  "block_param_def", "opt_bv_decl", "bv_decls", "bvar", "f_larglist",
  "lambda_body", "do_block", "$@24", "block_call", "method_call",
  "brace_block", "$@25", "$@26", "case_body", "cases", "opt_rescue",
  "exc_list", "exc_var", "opt_ensure", "literal", "string", "string_rep",
  "string_interp", "@27", "xstring", "regexp", "heredoc",
  "opt_heredoc_bodies", "heredoc_bodies", "heredoc_body", "words",
  "symbol", "basic_symbol", "sym", "symbols", "numeric", "variable",
  "var_lhs", "var_ref", "backref", "superclass", "$@28", "f_arglist",
  "f_args", "f_bad_arg", "f_norm_arg", "f_arg_item", "f_arg", "f_opt",
  "f_block_opt", "f_block_optarg", "f_optarg", "restarg_mark",
  "f_rest_arg", "blkarg_mark", "f_block_arg", "opt_f_block_arg",
  "singleton", "$@29", "assoc_list", "assocs", "assoc", "operation",
  "operation2", "operation3", "dot_or_colon", "opt_terms", "opt_nl",
  "rparen", "rbracket", "trailer", "term", "nl", "$@30", "terms", "none", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,    61,    63,    58,    62,    60,   124,    94,
      38,    43,    45,    42,    47,    37,   368,    33,   126,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     123,   125,    91,    46,    44,    96,    40,    41,    93,    59,
      10
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   151,   153,   152,   154,   155,   155,   155,   155,   156,
     157,   156,   158,   159,   160,   160,   160,   160,   162,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     163,   163,   164,   164,   164,   164,   164,   164,   165,   166,
     166,   167,   167,   169,   168,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   171,   171,   172,   172,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     174,   174,   175,   175,   176,   176,   177,   177,   177,   177,
     177,   177,   177,   177,   178,   178,   178,   178,   178,   178,
     178,   178,   179,   179,   180,   180,   180,   181,   181,   181,
     181,   181,   182,   182,   183,   184,   183,   185,   185,   185,
     185,   185,   185,   185,   185,   185,   185,   185,   185,   185,
     185,   185,   185,   185,   185,   185,   185,   185,   185,   185,
     185,   185,   185,   185,   185,   185,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     188,   189,   189,   189,   189,   190,   191,   191,   192,   192,
     192,   192,   192,   193,   193,   193,   193,   193,   195,   194,
     196,   197,   197,   198,   198,   198,   198,   199,   199,   199,
     200,   200,   200,   200,   200,   200,   200,   200,   201,   200,
     202,   200,   203,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   204,   200,
     200,   200,   205,   206,   200,   207,   208,   200,   200,   200,
     209,   210,   200,   211,   200,   212,   213,   200,   214,   200,
     215,   200,   216,   217,   200,   200,   200,   200,   200,   218,
     219,   219,   219,   220,   220,   221,   221,   222,   222,   223,
     223,   224,   224,   225,   225,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   227,   227,   227,   227,   227,   227,
     227,   227,   227,   227,   227,   227,   227,   227,   227,   228,
     228,   229,   229,   229,   230,   230,   231,   231,   232,   232,
     233,   233,   234,   234,   236,   235,   237,   237,   237,   237,
     238,   238,   238,   238,   238,   238,   238,   238,   238,   240,
     239,   241,   239,   242,   243,   243,   244,   244,   245,   245,
     245,   246,   246,   247,   247,   248,   248,   248,   248,   249,
     249,   249,   249,   250,   250,   251,   252,   251,   251,   253,
     253,   254,   254,   255,   256,   256,   257,   257,   258,   258,
     259,   259,   260,   260,   261,   262,   262,   262,   262,   262,
     262,   263,   263,   264,   264,   264,   264,   265,   265,   265,
     265,   265,   266,   267,   267,   267,   267,   267,   267,   267,
     268,   268,   269,   270,   269,   269,   271,   271,   272,   272,
     272,   272,   272,   272,   272,   272,   272,   272,   272,   272,
     272,   272,   272,   273,   273,   273,   273,   274,   274,   275,
     275,   276,   276,   277,   278,   279,   279,   280,   280,   281,
     281,   282,   282,   283,   283,   284,   285,   285,   286,   287,
     286,   288,   288,   289,   289,   290,   290,   291,   291,   291,
     292,   292,   292,   292,   293,   293,   293,   294,   294,   295,
     295,   296,   296,   297,   298,   299,   299,   299,   300,   300,
     302,   301,   303,   303,   304
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     1,     1,     3,     2,     1,
       0,     5,     4,     2,     1,     1,     3,     2,     0,     4,
       2,     3,     3,     3,     3,     3,     4,     1,     3,     3,
       6,     5,     5,     5,     5,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     3,     2,     1,     1,     1,
       1,     1,     4,     0,     5,     2,     3,     4,     5,     4,
       5,     2,     2,     2,     2,     2,     1,     3,     1,     3,
       1,     2,     3,     5,     2,     4,     2,     4,     1,     3,
       1,     3,     2,     3,     1,     2,     1,     4,     3,     3,
       3,     3,     2,     1,     1,     4,     3,     3,     3,     3,
       2,     1,     1,     1,     2,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     5,     3,     5,
       6,     5,     5,     5,     5,     4,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     4,     4,     2,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     3,     3,     3,     3,     6,     1,
       1,     1,     2,     4,     2,     3,     1,     1,     1,     1,
       2,     4,     2,     1,     2,     2,     4,     1,     0,     2,
       2,     2,     1,     1,     2,     3,     4,     3,     4,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     4,
       0,     4,     0,     3,     3,     3,     2,     3,     3,     1,
       4,     3,     1,     4,     3,     2,     1,     2,     0,     4,
       6,     6,     0,     0,     7,     0,     0,     7,     5,     4,
       0,     0,     9,     0,     6,     0,     0,     8,     0,     5,
       0,     6,     0,     0,     9,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,     5,     1,     2,     1,
       1,     1,     3,     1,     3,     1,     4,     6,     3,     5,
       2,     4,     1,     3,     6,     8,     4,     6,     4,     2,
       6,     2,     4,     6,     2,     4,     2,     4,     1,     1,
       1,     3,     1,     4,     1,     4,     1,     3,     1,     1,
       4,     1,     3,     3,     0,     5,     2,     4,     5,     5,
       2,     4,     4,     3,     3,     3,     2,     1,     4,     0,
       5,     0,     5,     5,     1,     1,     6,     1,     1,     1,
       1,     2,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     2,     3,     1,     2,     1,     0,     4,     1,     2,
       3,     2,     3,     1,     1,     1,     1,     2,     1,     2,
       2,     3,     1,     4,     2,     1,     1,     1,     1,     1,
       2,     2,     3,     1,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     4,     2,     3,     2,     6,     8,
       4,     6,     4,     6,     2,     4,     6,     2,     4,     2,
       4,     1,     0,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     3,     3,     3,     1,     3,     1,     3,     1,
       1,     2,     1,     1,     1,     2,     2,     1,     1,     0,
       4,     1,     2,     1,     3,     3,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     0,     1,     2,     2,     0,     1,     1,     1,     1,
       0,     3,     1,     2,     0
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     0,     1,     0,     0,     0,     0,     0,   268,
       0,     0,   529,   292,   295,     0,   315,   316,   317,   318,
     279,   248,   248,   455,   454,   456,   457,   531,     0,    10,
       0,   459,   458,   447,   519,   449,   448,   451,   450,   443,
     444,   409,   410,   460,   461,     0,     0,     0,     0,   272,
     544,   544,    78,   288,     0,     0,     0,     0,     0,     0,
     423,     0,     0,     0,     3,   529,     6,     9,    27,    39,
      42,    50,    49,     0,    66,     0,    70,    80,     0,    47,
     229,     0,    51,   286,   260,   261,   262,   263,   264,   407,
     406,   432,   408,   405,   453,     0,   265,   266,   248,     5,
       8,   315,   316,   279,   282,   387,     0,   102,   103,     0,
       0,     0,     0,   105,     0,   319,     0,   453,   266,     0,
     308,   156,   166,   157,   179,   153,   172,   162,   161,   182,
     183,   177,   160,   159,   155,   180,   184,   185,   164,   154,
     167,   171,   173,   165,   158,   174,   181,   176,   175,   168,
     178,   163,   152,   170,   169,   151,   149,   150,   146,   147,
     148,   107,   109,   108,   141,   142,   138,   120,   121,   122,
     129,   126,   128,   123,   124,   143,   144,   130,   131,   135,
     125,   127,   117,   118,   119,   132,   133,   134,   136,   137,
     139,   140,   145,   509,   310,   110,   111,   508,     0,   175,
     168,   178,   163,   146,   147,   107,   108,     0,   112,   114,
      20,   113,     0,     0,    48,     0,     0,     0,   453,     0,
     266,     0,   538,   540,   529,     0,   542,   539,   530,     0,
       0,     0,   330,   329,     0,     0,   453,   266,     0,     0,
       0,     0,   243,   230,   253,    64,   247,   544,   544,   513,
      65,    63,   531,    62,     0,   544,   386,    61,   531,     0,
     532,    18,     0,     0,   207,     0,   208,   276,     0,     0,
       0,   529,    15,   531,    68,    14,   270,   531,     0,   535,
     535,   231,     0,     0,   535,   511,     0,     0,    76,     0,
      86,    93,   482,   437,   436,   438,   439,     0,   435,   434,
     421,   416,   415,   418,     0,   413,   430,     0,   441,     0,
     411,     0,   419,     0,   445,   446,    46,   222,   223,     4,
     530,     0,     0,     0,     0,     0,     0,     0,   374,   376,
       0,    82,     0,    74,    71,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   544,     0,   528,   527,     0,   391,   389,
     287,     0,     0,   380,    55,   285,   305,   102,   103,   104,
     445,   446,     0,   463,   303,   462,     0,   544,     0,     0,
       0,   482,   312,     0,   115,     0,   544,   276,   321,     0,
     320,     0,     0,   544,     0,     0,     0,     0,   544,     0,
       0,     0,   543,     0,     0,   276,     0,   544,     0,   300,
     516,   254,   250,     0,     0,   244,   252,     0,   245,   531,
       0,   281,   249,   531,   239,   544,   544,   238,   531,   284,
      45,     0,     0,     0,     0,     0,     0,    17,   531,   274,
      13,   530,    67,   531,   273,   277,   537,   232,   536,   537,
     234,   278,   512,    92,    84,     0,    79,     0,     0,   544,
       0,   488,   485,   484,   483,   486,     0,   500,   504,   503,
     499,   482,     0,   371,   487,   489,   491,   544,   497,   544,
     502,   544,     0,   481,   440,     0,     0,   422,   414,   431,
     442,   412,   420,     0,     0,     7,    21,    22,    23,    24,
      25,    43,    44,   544,     0,    28,    37,     0,    38,   531,
       0,    72,    83,    41,    40,     0,   186,   253,    36,   204,
     212,   217,   218,   219,   214,   216,   226,   227,   220,   221,
     197,   198,   224,   225,   531,   213,   215,   209,   210,   211,
     199,   200,   201,   202,   203,   520,   525,   521,   526,   385,
     248,   383,   531,   520,   522,   521,   523,   384,   248,   520,
     521,   248,   544,   544,    29,   188,    35,   196,    53,    56,
       0,   465,     0,     0,   102,   103,   106,     0,   531,   544,
       0,   531,   482,     0,     0,     0,     0,   269,   544,   544,
     397,   544,   322,   186,   524,   521,   531,   520,   521,   544,
     428,     0,   541,   425,   426,   424,     0,     0,   299,   324,
     293,   323,   296,   524,   275,   531,   520,   521,     0,   515,
       0,   255,   251,   544,   514,   280,   533,   235,   240,   242,
     283,    19,     0,    26,   195,    69,    16,   271,   535,    85,
      77,    89,    91,   531,   520,   521,     0,   488,     0,   342,
     333,   335,   531,   331,   531,     0,     0,   289,     0,   474,
     507,     0,   477,   501,     0,   479,   505,   433,     0,   205,
     206,   362,   531,     0,   360,   359,   259,     0,    81,    75,
       0,     0,     0,     0,     0,     0,   382,    59,     0,   388,
       0,     0,   237,   381,    57,   236,   377,    52,     0,     0,
       0,   544,   306,     0,     0,   388,   309,   510,   531,     0,
     467,   313,   116,   398,   399,   544,   400,     0,   544,   327,
       0,     0,   325,     0,     0,   388,     0,     0,     0,   429,
     427,   298,     0,     0,     0,     0,   388,     0,   256,   246,
     544,    11,   233,    87,   493,   531,     0,   340,     0,   490,
       0,   364,     0,     0,   492,   544,   544,   506,   544,   498,
     544,   544,   417,   488,   531,     0,   544,   495,   544,   544,
     358,     0,     0,   257,    73,   187,     0,    34,   193,    33,
     194,    60,   534,     0,    31,   191,    32,   192,    58,   378,
     379,     0,     0,   189,     0,     0,   464,   304,   466,   311,
     482,     0,     0,   402,   328,     0,    12,   404,     0,   290,
       0,   291,   255,   544,     0,     0,   301,   241,   332,   343,
       0,   338,   334,   370,     0,   373,   372,     0,   470,     0,
     472,     0,   478,     0,   475,   480,     0,     0,   361,   349,
     351,     0,   354,     0,   356,   375,   258,   228,    30,   190,
     392,   390,     0,     0,     0,     0,   401,     0,    94,   101,
       0,   403,     0,   394,   395,   393,   294,   297,     0,     0,
     341,     0,   336,   368,   531,   366,   369,   544,   544,   544,
     544,     0,   494,   363,   544,   544,   544,   496,   544,   544,
      54,   307,     0,   100,     0,   544,     0,   544,   544,     0,
     339,     0,     0,   365,   471,     0,   468,   473,   476,   276,
       0,     0,   346,     0,   348,   355,     0,   352,   357,   314,
     524,    99,   531,   520,   521,   396,   326,   302,   337,   367,
     544,   524,   275,   544,   544,   544,   544,   388,   469,   347,
       0,   344,   350,   353,   544,   345
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,    64,    65,    66,   262,   395,   396,   271,
     272,   441,    68,    69,   215,    70,    71,   579,   711,    72,
      73,   273,    74,    75,    76,   466,    77,   216,   113,   114,
     208,   209,   210,   596,   195,   196,    79,   244,   278,   559,
     703,   433,   434,   253,   254,   246,   425,   435,   518,    80,
     212,   453,   277,   292,   229,   744,   230,   745,   628,   878,
     583,   580,   805,   389,   391,   595,   810,   265,   399,   620,
     731,   732,   235,   660,   661,   662,   774,   683,   684,   760,
     884,   885,   482,   667,   329,   513,    82,    83,   375,   573,
     572,   411,   875,   599,   725,   812,   816,    84,    85,   611,
     305,   496,    86,    87,    88,   612,   613,   614,    89,    90,
      91,   299,    92,    93,   218,   219,    96,   220,   384,   582,
     593,   594,   484,   485,   486,   487,   488,   777,   778,   489,
     490,   491,   492,   767,   669,   198,   390,   283,   436,   249,
     119,   587,   561,   367,   225,   430,   431,   699,   457,   400,
     260,   408,   228,   275
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -745
static const yytype_int16 yypact[] =
{
    -745,   109,  2594,  -745,  7293,  9101,  9428,  5731,  6790,  -745,
    8762,  8762,  5204,  -745,  -745,  9210,  7519,  7519,  -745,  -745,
    7519,  3298,  2863,  -745,  -745,  -745,  -745,   -13,  6790,  -745,
       1,  -745,  -745,  5862,  3008,  -745,  -745,  5993,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  8875,  8875,    94,  4515,  8762,
    7745,  8084,  7061,  -745,  6504,   671,   707,   751,   771,   314,
    -745,   125,  8988,  8875,  -745,   189,  -745,   905,  -745,   491,
    -745,  -745,   124,    67,  -745,    43,  9319,  -745,    99,  2845,
     228,   271,    24,    76,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,   392,    86,  -745,   436,    63,  -745,
    -745,  -745,  -745,  -745,   102,   123,   168,   328,   418,  8762,
     104,  4665,   283,  -745,   160,  -745,   278,  -745,  -745,    63,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,    56,    59,
     214,   234,  -745,  -745,  -745,  -745,  -745,  -745,   260,   274,
    -745,   292,  -745,   295,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,    24,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  6647,  -745,  -745,
      93,  -745,  3682,   150,   491,    92,   218,   284,    49,   237,
      75,    92,  -745,  -745,   189,   332,  -745,  -745,   235,  8762,
    8762,   312,  -745,  -745,   304,   364,    88,    95,  8875,  8875,
    8875,  8875,  -745,  2845,   300,  -745,  -745,   261,   266,  -745,
    -745,  -745,  5091,  -745,  7519,  7519,  -745,  -745,  5339,  8762,
    -745,  -745,   279,  4815,  -745,   330,   370,   472,  7406,  4515,
     301,   189,   905,   308,   338,  -745,   491,   308,   315,    19,
     145,  -745,   300,   324,   145,  -745,   421,  9537,   334,   354,
     373,   378,   745,  -745,  -745,  -745,  -745,   939,  -745,  -745,
    -745,  -745,  -745,  -745,   696,  -745,  -745,   943,  -745,   961,
    -745,   971,  -745,   388,   409,   415,  -745,  -745,  -745,  -745,
    5452,  8762,  8762,  8762,  8762,  7406,  8762,  8762,  -745,  -745,
    8197,  -745,  4515,  7177,   357,  8197,  8875,  8875,  8875,  8875,
    8875,  8875,  8875,  8875,  8875,  8875,  8875,  8875,  8875,  8875,
    8875,  8875,  8875,  8875,  8875,  8875,  8875,  8875,  8875,  8875,
    8875,  8875,  2206,  7519,  9814,  -745,  -745, 10750,  -745,  -745,
    -745,  8988,  8988,  -745,   395,  -745,   491,  -745,   381,  -745,
    -745,  -745,   189,  -745,  -745,  -745,  9892,  7519,  9970,  3682,
    8762,   875,  -745,   441,  -745,   503,   523,    91,  -745,  3823,
     520,  8875, 10048,  7519, 10126,  8875,  8875,  4095,   802,   332,
    8310,   525,  -745,    51,    51,   110, 10204,  7519, 10282,  -745,
    -745,  -745,  -745,  8875,  7632,  -745,  -745,  7858,  -745,   308,
     408,  -745,  -745,   308,  -745,   424,   428,  -745,    90,  -745,
    -745,  6790,  4230,   442, 10048, 10126,  8875,   905,   308,  -745,
    -745,  5584,   445,   308,  -745,  -745,  7971,  -745,  -745,  8084,
    -745,  -745,  -745,   381,    43,  9537,  -745,  9537, 10360,  7519,
   10438,   446,  -745,  -745,  -745,  -745,  1004,  -745,  -745,  -745,
    -745,   986,    58,  -745,  -745,  -745,  -745,   459,  -745,   462,
     539,   478,   582,  -745,  -745,   567,  4815,  -745,  -745,  -745,
    -745,  -745,  -745,  8875,  8875,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,    55,  8875,  -745,   490,   494,  -745,   308,
    9537,   499,  -745,  -745,  -745,   533,  1174,  -745,  -745,   370,
    1773,  1773,  1773,  1773,   837,   837,  1845,  2028,  1773,  1773,
    2990,  2990,   558,   558,  2700,   837,   837,   752,   752,   812,
     385,   385,   370,   370,   370,  3443,  6242,  3532,  6360,  -745,
     123,  -745,   308,   580,  -745,   584,  -745,  -745,  3153,  -745,
    -745,  1199,    55,    55,  -745,  1283,  -745,  2845,  -745,  -745,
     189,  -745,  8762,  3682,   579,    48,  -745,   123,   308,   123,
     637,    90,   986,  3682,   189,  6933,  6790,  -745,  8423,   634,
    -745,   563,  -745,  2099,  6124,  2718,   308,   412,   432,   634,
    -745,   853,  -745,   802,  -745,  -745,   642,    79,  -745,  -745,
    -745,  -745,  -745,   139,   219,   308,   120,   128,  8762,  -745,
    8875,   300,  -745,   266,  -745,  -745,  -745,  -745,  7632,  7858,
    -745,  -745,   512,  -745,  2845,    30,   905,  -745,   145,   357,
    -745,   579,    48,   308,    35,    45,  8875,  -745,  1004,   647,
    -745,   510,   308,  -745,   308,  4956,  4815,  -745,   986,  -745,
    -745,   986,  -745,  -745,  1095,  -745,  -745,  -745,   514,   370,
     370,  -745,   734,  4956,  -745,  -745,   517,  8536,  -745,  -745,
    9537,  8988,  8875,   547,  8988,  8988,  -745,   395,   516,   598,
    8988,  8988,  -745,  -745,   395,  -745,    76,   124,  4956,  4815,
    8875,    55,  -745,   189,   658,  -745,  -745,  -745,   308,   659,
    -745,  -745,  -745,   490,  -745,   585,  -745,  4380,   662,  -745,
    8762,   663,  -745,  8875,  8875,   454,  8875,  8875,   675,  -745,
    -745,  -745,  8649,  3959,  4956,  4956,   147,    51,  -745,  -745,
     542,  -745,  -745,   348,  -745,   308,  1135,   545,  1076,  -745,
     543,   550,   702,   565,  -745,   569,   571,  -745,   573,  -745,
     574,   573,  -745,   607,   308,   611,   587,  -745,   593,   594,
    -745,   730,  8875,   599,  -745,  2845,  8875,  -745,  2845,  -745,
    2845,  -745,  -745,  8988,  -745,  2845,  -745,  2845,  -745,  -745,
    -745,   735,   603,  2845,  4815,  3682,  -745,  -745,  -745,  -745,
     875,  9646,    92,  -745,  -745,  4956,  -745,  -745,    92,  -745,
    8875,  -745,  -745,   196,   737,   740,  -745,  7858,  -745,   610,
    1135,   795,  -745,  -745,  1088,  -745,  -745,   986,  -745,  1095,
    -745,  1095,  -745,  1095,  -745,  -745,  9755,   633,  -745,  1215,
    -745,  1215,  -745,  1095,  -745,  -745,   612,  2845,  -745,  2845,
    -745,  -745,   621,   755,  3682,   712,  -745,   396,   373,   378,
    3682,  -745,  3823,  -745,  -745,  -745,  -745,  -745,  4956,  1135,
     610,  1135,   629,  -745,   333,  -745,  -745,   573,   631,   573,
     573,   723,   410,  -745,   635,   639,   573,  -745,   648,   573,
    -745,  -745,   776,   381, 10516,  7519, 10594,   523,   563,   784,
     610,  1135,  1088,  -745,  -745,  1095,  -745,  -745,  -745,  -745,
   10672,  1215,  -745,  1095,  -745,  -745,  1095,  -745,  -745,  -745,
     113,    48,   308,   129,   138,  -745,  -745,  -745,   610,  -745,
     573,   656,   660,   573,   667,   573,   573,   185,  -745,  -745,
    1095,  -745,  -745,  -745,   573,  -745
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -745,  -745,  -745,   375,  -745,    32,  -745,  -350,   285,  -745,
      61,  -745,  -318,    -3,    22,   -59,  -745,  -582,  -745,    -5,
     797,  -146,    28,   -63,  -236,  -424,   -27,  1718,   -79,   814,
       7,   -12,  -745,  -745,  -248,  -745,  1213,   681,  -745,     5,
     253,  -323,   101,    77,  -745,  -403,  -243,    21,  -283,    14,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,  -745,
    -745,  -745,  -745,  -745,  -745,  -745,  -745,   288,  -217,  -359,
     -81,  -537,  -745,  -672,  -673,   170,  -745,  -443,  -745,  -603,
    -745,   -83,  -745,  -745,   133,  -745,  -745,  -745,   -82,  -745,
    -745,  -391,  -745,   -76,  -745,  -745,  -745,  -745,  -745,  1009,
    -216,  -745,  -745,  -745,  -745,  -745,  -745,   232,  -745,  -745,
       2,  -745,  -745,  -745,  1319,  1671,   840,  1691,  -745,  -745,
      42,  -264,  -734,  -165,  -567,   148,  -618,  -744,     4,   188,
    -745,  -141,  -745,  -260,  1306,  -745,  -745,  -745,     6,  -382,
     793,  -320,  -745,   665,    10,   -25,  -223,  -521,  -211,    -6,
      46,  -745,     9,    -2
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -545
static const yytype_int16 yytable[] =
{
      99,   370,   259,   316,   407,   428,   226,   214,   214,   214,
     211,   242,   242,   334,   194,   242,   261,   523,   616,   115,
     115,   632,   248,   248,   632,   288,   248,   256,   483,   115,
     211,   379,   493,   221,   224,   439,   100,   247,   247,   590,
     562,   247,   560,   650,   568,   634,   276,   571,   281,   285,
     452,   465,   528,   769,   454,   622,   280,   284,   227,   226,
     -88,   298,   728,    67,   588,    67,   115,   715,   589,   460,
     -90,   279,   738,   462,   320,   319,   274,   634,   619,   775,
     606,   495,   560,   829,   568,   735,   832,   665,   498,   368,
     115,   498,   398,   498,   625,   498,   689,   498,   589,   257,
     886,   764,   368,   373,   746,   398,   376,   897,   385,     3,
     256,   227,   365,   -94,   558,   791,   566,   245,   250,   566,
    -101,   251,   798,   448,   373,   589,   -97,   326,   327,   708,
     709,   493,   753,   258,   681,  -100,  -275,   223,   558,  -452,
     566,   263,   -96,   -67,  -455,   -96,   653,  -454,   -88,   267,
     589,   -98,   328,   -98,   558,   377,   566,   880,   -90,   378,
     227,   382,   -94,   456,   -97,   406,   666,   366,   558,   223,
     566,   847,   -95,   682,   -81,   374,   371,   897,   886,   -88,
     330,   446,   -88,   314,   315,   -88,   519,   331,  -101,   -90,
    -275,  -275,   -90,   428,  -521,   -90,   558,   566,   -95,  -455,
     222,   223,  -454,   369,  -100,   397,   635,   832,   910,   255,
     637,   727,   335,   410,   298,   640,   369,   664,   226,   769,
     558,   493,   566,   742,   464,   645,   214,   214,   222,   223,
     647,   465,   -86,   714,   409,   632,   632,   394,   938,   -93,
     223,   222,   223,   719,   -99,   426,   426,   242,   252,   242,
     242,   413,   414,   437,   -92,   438,   440,   634,   248,  -520,
     248,   227,   -97,   -97,   -88,   226,   784,   227,   804,   255,
     227,   515,   -90,   247,   764,   247,   524,   383,   -96,   -96,
     451,   450,   764,   -89,   465,  -520,   873,   -98,   -98,   459,
      81,   -87,    81,   116,   116,   223,   688,   274,   217,   217,
     217,   115,  -456,   234,   217,   217,   521,   586,   217,   222,
     223,   663,   574,   576,   258,   724,  -319,   227,   214,   214,
     214,   214,  -457,   511,   512,   458,   458,   405,   718,   447,
     458,   401,   493,   270,   -95,   -95,    81,   217,   222,   223,
     289,   380,   381,   506,   507,   508,   509,   115,  -459,   410,
     217,   517,   505,   429,  -517,   432,   517,  -456,   242,   362,
     274,   437,  -458,   -91,   289,  -521,   386,   415,   717,   567,
    -319,  -319,   402,   523,  -388,   312,   581,  -457,   301,   302,
    -447,    67,   242,  -451,   412,   437,   510,   591,   826,   419,
     749,   423,   416,   567,   600,   498,   270,   217,   242,    81,
     743,   437,   649,  -459,   464,   424,   615,   621,   621,   567,
     427,   947,   242,   363,   364,   437,  -447,  -458,   444,   442,
     387,   388,   780,   567,   632,   303,   403,   404,   227,   641,
     633,   617,   874,   426,   426,  -447,  -388,   752,  -451,   759,
      99,   336,   468,   211,  -518,   634,   417,   418,   449,   502,
     567,   -66,   301,   302,   465,   863,   336,   464,   223,   227,
     227,  -453,   648,   455,   242,   461,  -266,   437,  -517,  -276,
    -447,  -447,   403,   445,  -517,   567,   463,   912,   467,   115,
     503,   115,  -452,   223,   904,   670,   504,   670,  -388,   670,
    -388,  -388,   578,   663,   757,   808,   469,   470,   920,   303,
      81,   522,   736,    67,   494,   -94,  -451,   749,   359,   360,
     361,   685,   646,   597,   902,  -453,  -453,   217,   217,   693,
    -266,  -266,   737,  -276,  -276,   -96,   372,   766,   326,   327,
     770,   598,   828,   602,   115,   618,   -86,   698,   905,   906,
     217,   779,   217,   217,   820,   -98,   217,   217,   443,  -101,
     493,    81,   387,   388,   270,   636,    81,    81,  -518,   656,
    -451,  -451,   446,   698,  -518,   696,   705,   -95,   638,   705,
     685,   685,   639,   702,   712,   289,   702,   730,   727,   214,
     -93,   698,   932,   643,   722,  -100,   589,   705,   720,   -81,
     673,   663,   696,   663,   702,   870,   726,   729,   211,   729,
     698,   872,   721,   668,   713,  -524,   671,   729,    81,   217,
     217,   217,   217,    81,   217,   217,   -92,   270,   217,   517,
      81,   289,   674,   217,   799,   214,   227,   464,   698,   336,
     677,   426,   524,   676,  -253,   787,   789,   697,   687,   761,
     227,   794,   796,   690,   750,   704,   691,   716,   707,   727,
     747,   217,   741,   751,   758,   772,   558,   761,   566,   217,
     217,  -254,   786,   227,   792,   663,   882,  -524,   807,   809,
     700,   815,   558,   819,   701,   217,   811,    81,   217,   357,
     358,   359,   360,   361,   601,   821,   827,    81,   793,   830,
     833,   217,   609,   -96,   458,    81,   888,   -98,   657,   834,
     472,   473,   474,   475,   115,   217,   836,   806,   895,   685,
     898,   -95,   835,   837,   663,   839,   663,   841,   843,  -524,
     846,  -524,  -524,   813,   -88,  -520,   817,   214,   -90,   848,
      81,   849,   282,   300,   858,   301,   302,   851,   853,    81,
     855,   621,   -87,  -255,   861,   860,   663,   876,   426,   761,
     877,   893,   818,   289,   879,   289,  -256,   217,   497,   227,
     301,   302,   900,   670,   670,   901,   670,   903,   670,   670,
     306,   301,   302,   911,   670,   915,   670,   670,   919,   921,
     944,   678,   303,   923,    81,   773,   929,   472,   473,   474,
     475,   756,   926,   227,   937,    98,   471,    98,   472,   473,
     474,   475,  -520,    98,    98,    98,  -521,   303,   289,    98,
      98,   950,   232,    98,   308,   301,   302,   642,   303,   768,
     120,   729,   771,   336,   706,   115,   476,   936,   755,   939,
     776,   935,   477,   478,   310,   301,   302,   476,   349,   350,
     800,    98,    98,   477,   478,   740,   657,   197,   472,   473,
     474,   475,   864,   894,   479,    98,   765,   480,   227,   913,
     115,     0,   303,   392,   227,   479,   301,   302,   480,     0,
     217,    81,   356,   357,   358,   359,   360,   361,     0,     0,
       0,    81,   303,   336,   223,   670,   670,   670,   670,     0,
       0,   481,   670,   670,   670,     0,   670,   670,   349,   350,
     242,     0,    98,   437,    98,   600,   729,   698,   336,     0,
       0,   567,   610,   303,     0,     0,   217,   301,   302,   420,
     421,   422,     0,   349,   350,     0,   471,     0,   472,   473,
     474,   475,     0,   357,   358,   359,   360,   361,   670,   881,
       0,   670,   670,   670,   670,   321,   322,   323,   324,   325,
     762,   763,   670,    81,    81,   354,   355,   356,   357,   358,
     359,   360,   361,   739,   303,     0,     0,   476,   781,     0,
       0,    81,     0,   477,   478,     0,     0,     0,   289,   217,
       0,     0,   217,   217,     0,   887,     0,   889,   217,   217,
       0,   890,     0,   801,   802,   479,    81,    81,   480,   896,
       0,   899,   494,   301,   302,    98,   499,   301,   302,     0,
       0,   516,   814,     0,     0,    81,   527,     0,   217,     0,
       0,   592,    98,    98,   500,   301,   302,     0,   823,   824,
     825,    81,    81,    81,   501,   301,   302,   471,     0,   472,
     473,   474,   475,     0,     0,    98,     0,    98,    98,     0,
     303,    98,    98,     0,   303,   657,    98,   472,   473,   474,
     475,    98,    98,   940,   304,   307,   309,   311,   313,   943,
       0,   945,   303,     0,   946,     0,     0,     0,   476,     0,
       0,   217,   303,     0,   477,   478,     0,     0,     0,   862,
       0,   527,    81,    81,     0,     0,   658,     0,   954,   867,
     871,     0,   659,    81,   629,   631,   479,     0,   282,   480,
       0,     0,     0,    98,    98,    98,    98,    98,    98,    98,
      98,     0,     0,    98,     0,    98,     0,   657,    98,   472,
     473,   474,   475,     0,   892,     0,     0,   631,     0,   883,
     282,   472,   473,   474,   475,     0,   657,     0,   472,   473,
     474,   475,    81,     0,     0,   907,    98,   908,    81,     0,
      81,     0,     0,   909,    98,    98,    81,     0,   658,     0,
       0,     0,     0,     0,   831,     0,     0,     0,     0,     0,
      98,     0,    98,    98,     0,     0,   657,   476,   472,   473,
     474,   475,    98,   217,   478,   686,    98,     0,     0,  -544,
      98,     0,     0,     0,     0,     0,     0,  -544,  -544,  -544,
      98,     0,  -544,  -544,  -544,   479,  -544,     0,   692,     0,
       0,     0,     0,     0,     0,  -544,  -544,   658,     0,   243,
     243,     0,     0,   243,     0,    98,  -544,  -544,     0,  -544,
    -544,  -544,  -544,  -544,    98,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   264,   266,
     349,   350,    98,   243,   243,     0,   773,     0,   472,   473,
     474,   475,     0,     0,     0,   317,   318,     0,     0,   723,
       0,     0,     0,     0,     0,     0,     0,  -544,   351,    98,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
       0,     0,     0,     0,     0,     0,     0,   476,     0,     0,
       0,   748,     0,   477,   478,     0,     0,     0,  -230,   631,
     282,    94,     0,    94,   117,   117,   117,   710,     0,     0,
       0,     0,     0,     0,   236,   479,     0,   754,   480,  -544,
    -544,     0,  -544,     0,     0,   255,  -544,     0,  -544,  -544,
       0,     0,     0,     0,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,    94,   783,   349,
     350,   290,     0,     0,     0,    98,    98,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    98,     0,     0,     0,
       0,     0,     0,     0,     0,   290,     0,   351,     0,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    98,     0,   822,     0,     0,     0,     0,     0,     0,
      94,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   243,   243,   243,   317,     0,     0,     0,    98,    98,
       0,     0,     0,   856,     0,   243,     0,   243,   243,     0,
       0,     0,     0,     0,     0,     0,    98,     0,     0,     0,
       0,     0,     0,     0,    98,     0,     0,    98,    98,     0,
       0,     0,     0,    98,    98,     0,     0,     0,     0,     0,
       0,    98,    98,     0,     0,     0,     0,     0,   282,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      98,     0,     0,    98,     0,     0,     0,     0,     0,     0,
       0,    94,     0,     0,     0,     0,    98,    98,    98,     0,
       0,     0,     0,   243,     0,     0,     0,     0,   526,   529,
     530,   531,   532,   533,   534,   535,   536,   537,   538,   539,
     540,   541,   542,   543,   544,   545,   546,   547,   548,   549,
     550,   551,   552,   553,   554,     0,   243,     0,     0,     0,
       0,     0,    94,     0,   575,   577,    98,    94,    94,     0,
       0,     0,     0,     0,     0,     0,     0,    98,    98,     0,
     243,     0,     0,     0,     0,     0,   290,     0,    98,     0,
       0,     0,     0,     0,   603,     0,   243,     0,   575,   577,
       0,     0,     0,   243,     0,     0,     0,     0,     0,     0,
     243,     0,     0,     0,     0,     0,   243,   243,     0,    94,
     243,     0,     0,     0,    94,     0,     0,     0,     0,     0,
       0,    94,   290,     0,     0,     0,     0,    98,     0,   644,
       0,     0,     0,    98,     0,    98,     0,     0,     0,   243,
       0,    98,   243,    95,     0,    95,     0,     0,     0,     0,
       0,     0,   243,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    97,     0,    97,   118,   118,    98,     0,
       0,     0,     0,     0,     0,     0,   237,     0,    94,     0,
       0,     0,     0,     0,     0,     0,   679,   680,    94,    95,
      78,     0,    78,     0,     0,     0,    94,   243,     0,     0,
       0,     0,     0,   233,     0,     0,     0,     0,     0,    97,
       0,     0,     0,   291,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    94,     0,     0,     0,     0,    78,   291,     0,     0,
      94,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    95,     0,   290,     0,   290,     0,     0,     0,
       0,     0,     0,     0,     0,   672,     0,   675,     0,     0,
       0,     0,    97,     0,     0,     0,     0,     0,     0,     0,
       0,   243,     0,     0,     0,    94,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   290,
       0,     0,     0,   243,   336,  -545,  -545,  -545,  -545,   341,
     342,   243,   243,  -545,  -545,     0,     0,     0,     0,   349,
     350,     0,     0,     0,     0,     0,     0,     0,     0,   243,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    95,     0,     0,     0,     0,     0,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,     0,
     243,     0,    94,    97,   603,   785,     0,   788,   790,     0,
       0,     0,    94,   795,   797,     0,   336,   337,   338,   339,
     340,   341,   342,   803,     0,   345,   346,     0,     0,     0,
      78,   349,   350,     0,    95,     0,     0,     0,     0,    95,
      95,     0,     0,     0,     0,     0,   788,   790,     0,   795,
     797,     0,     0,     0,    97,   243,     0,     0,     0,    97,
      97,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,     0,     0,     0,     0,     0,     0,     0,   291,     0,
       0,    78,     0,     0,    94,    94,    78,    78,     0,     0,
       0,    95,     0,     0,     0,   243,    95,     0,     0,   857,
       0,     0,    94,    95,     0,     0,   859,     0,     0,   290,
       0,    97,     0,     0,     0,     0,    97,     0,     0,     0,
       0,     0,     0,    97,   291,     0,     0,    94,    94,     0,
       0,     0,     0,   859,     0,     0,     0,     0,    78,     0,
     243,     0,     0,    78,     0,     0,    94,     0,     0,     0,
      78,     0,     0,   525,     0,     0,     0,     0,     0,     0,
      95,     0,    94,    94,    94,     0,     0,     0,     0,     0,
      95,   838,   840,     0,   842,     0,   844,   845,    95,     0,
      97,     0,   850,     0,   852,   854,     0,     0,     0,     0,
      97,     0,     0,     0,     0,     0,     0,     0,    97,   336,
     337,   338,   339,   340,   341,   342,   343,    78,   345,   346,
       0,     0,     0,    95,   349,   350,     0,    78,   243,     0,
       0,     0,    95,    94,    94,    78,     0,     0,     0,     0,
     868,     0,     0,    97,    94,     0,     0,     0,     0,     0,
       0,     0,    97,   692,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,     0,     0,   291,     0,   291,     0,
      78,     0,     0,     0,     0,   117,     0,    95,     0,    78,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,    94,     0,   349,   350,    97,     0,    94,
       0,    94,     0,   914,   916,   917,   918,    94,     0,     0,
     922,   924,   925,     0,   927,   928,     0,     0,     0,     0,
       0,   291,     0,   351,    78,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   948,     0,     0,   949,
     951,   952,   953,     0,    95,     0,     0,   555,   556,     0,
     955,   557,     0,     0,    95,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    97,   164,   165,   166,   167,   168,
     169,   170,   171,   172,    97,     0,   173,   174,     0,     0,
     175,   176,   177,   178,     0,     0,     0,     0,     0,     0,
       0,    78,     0,     0,   179,     0,     0,     0,     0,     0,
       0,    78,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,     0,   190,   191,     0,    95,    95,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   192,   255,     0,    95,     0,    97,    97,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    97,     0,     0,     0,     0,    95,
      95,   291,     0,    78,    78,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    95,    97,
      97,    78,     0,     0,     0,     0,     0,     0,     0,   525,
       0,     0,     0,     0,    95,    95,    95,     0,    97,     0,
       0,     0,     0,     0,     0,     0,    78,    78,     0,     0,
       0,     0,     0,     0,    97,    97,    97,     0,     0,     0,
       0,     0,     0,     0,     0,    78,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    78,    78,    78,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    95,    95,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,    97,     0,     0,     0,
       0,     0,   869,     0,     0,     0,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    78,    78,     0,     0,     0,     0,     0,   866,
       0,     0,     0,    78,     0,    95,     0,   118,     0,     0,
       0,    95,     0,    95,     0,     0,     0,     0,     0,    95,
       0,     0,     0,     0,     0,    97,     0,     0,     0,     0,
       0,    97,     0,    97,     0,     0,     0,     0,     0,    97,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    78,     0,     0,     0,     0,     0,    78,     0,
      78,     0,     0,     0,  -544,     4,    78,     5,     6,     7,
       8,     9,     0,     0,     0,    10,    11,     0,     0,     0,
      12,     0,    13,    14,    15,    16,    17,    18,    19,     0,
       0,     0,     0,     0,    20,    21,    22,    23,    24,    25,
      26,     0,     0,    27,     0,     0,     0,     0,     0,    28,
      29,    30,    31,    32,     0,    33,    34,    35,    36,    37,
      38,     0,    39,    40,    41,     0,     0,    42,     0,     0,
      43,    44,     0,    45,    46,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    47,     0,     0,    48,    49,     0,    50,
      51,     0,    52,     0,    53,    54,    55,    56,    57,    58,
      59,     0,     0,    60,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  -275,     0,
      61,    62,    63,     0,     0,     0,  -275,  -275,  -275,     0,
       0,  -275,  -275,  -275,     0,  -275,     0,     0,     0,     0,
       0,     0,     0,  -544,  -544,  -275,  -275,  -275,     0,     0,
       0,     0,     0,     0,     0,  -275,  -275,     0,  -275,  -275,
    -275,  -275,  -275,     0,     0,     0,     0,     0,     0,     0,
       0,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,     0,     0,   349,   350,     0,  -275,
    -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,     0,     0,  -275,  -275,  -275,     0,   734,  -275,
       0,     0,     0,     0,   351,  -275,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,  -275,     0,     0,     0,
       0,   -99,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,  -275,  -275,     0,     0,     0,     0,     0,     0,
     223,     0,     0,     0,     0,     0,     0,     0,     0,  -275,
    -275,  -275,  -275,  -387,     0,  -275,  -275,  -275,  -275,     0,
       0,  -387,  -387,  -387,     0,     0,  -387,  -387,  -387,     0,
    -387,     0,     0,     0,     0,     0,     0,     0,     0,  -387,
    -387,  -387,     0,     0,     0,     0,     0,     0,     0,     0,
    -387,  -387,     0,  -387,  -387,  -387,  -387,  -387,     0,     0,
       0,     0,     0,     0,     0,     0,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,     0,
       0,   349,   350,     0,  -387,  -387,  -387,  -387,  -387,  -387,
    -387,  -387,  -387,  -387,  -387,  -387,  -387,     0,     0,  -387,
    -387,  -387,     0,     0,  -387,     0,     0,     0,     0,   351,
    -387,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,     0,     0,     0,     0,     0,     0,  -387,     0,  -387,
    -387,  -387,  -387,  -387,  -387,  -387,  -387,  -387,  -387,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -387,  -387,  -387,  -387,  -387,  -267,   255,
    -387,  -387,  -387,  -387,     0,     0,  -267,  -267,  -267,     0,
       0,  -267,  -267,  -267,     0,  -267,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -267,  -267,  -267,     0,     0,
       0,     0,     0,     0,     0,  -267,  -267,     0,  -267,  -267,
    -267,  -267,  -267,     0,     0,     0,     0,     0,     0,     0,
       0,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,  -545,  -545,     0,     0,   349,   350,     0,  -267,
    -267,  -267,  -267,  -267,  -267,  -267,  -267,  -267,  -267,  -267,
    -267,  -267,     0,     0,  -267,  -267,  -267,     0,     0,  -267,
       0,     0,     0,     0,     0,  -267,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,  -267,     0,     0,     0,
       0,     0,  -267,  -267,  -267,  -267,  -267,  -267,  -267,  -267,
    -267,  -267,  -267,  -267,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -267,
    -267,  -267,  -267,  -544,     0,  -267,  -267,  -267,  -267,     0,
       0,  -544,  -544,  -544,     0,     0,  -544,  -544,  -544,     0,
    -544,     0,     0,     0,     0,     0,     0,     0,     0,  -544,
    -544,  -544,     0,     0,     0,     0,     0,     0,     0,     0,
    -544,  -544,     0,  -544,  -544,  -544,  -544,  -544,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -544,  -544,  -544,  -544,  -544,  -544,
    -544,  -544,  -544,  -544,  -544,  -544,  -544,     0,     0,  -544,
    -544,  -544,     0,     0,  -544,     0,     0,     0,     0,     0,
    -544,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  -544,     0,  -544,
    -544,  -544,  -544,  -544,  -544,  -544,  -544,  -544,  -544,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -544,  -544,  -544,  -544,  -544,  -282,   255,
    -544,  -544,  -544,  -544,     0,     0,  -282,  -282,  -282,     0,
       0,  -282,  -282,  -282,     0,  -282,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -282,  -282,     0,     0,     0,
       0,     0,     0,     0,     0,  -282,  -282,     0,  -282,  -282,
    -282,  -282,  -282,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,     0,     0,  -282,  -282,  -282,     0,     0,  -282,
       0,     0,     0,     0,     0,  -282,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -282,     0,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -282,
    -282,  -282,  -282,  -524,   252,  -282,  -282,  -282,  -282,     0,
       0,  -524,  -524,  -524,     0,     0,     0,  -524,  -524,     0,
    -524,     0,     0,     0,     0,     0,     0,     0,     0,  -524,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    -524,  -524,     0,  -524,  -524,  -524,  -524,  -524,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -524,  -524,  -524,  -524,  -524,  -524,
    -524,  -524,  -524,  -524,  -524,  -524,  -524,     0,     0,  -524,
    -524,  -524,  -275,   694,     0,     0,     0,     0,     0,     0,
    -275,  -275,  -275,     0,     0,     0,  -275,  -275,     0,  -275,
       0,     0,     0,     0,     0,     0,   -97,  -524,     0,  -524,
    -524,  -524,  -524,  -524,  -524,  -524,  -524,  -524,  -524,  -275,
    -275,     0,  -275,  -275,  -275,  -275,  -275,     0,     0,     0,
       0,     0,     0,  -524,  -524,  -524,  -524,   -89,     0,     0,
    -524,     0,  -524,  -524,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,  -275,  -275,  -275,  -275,     0,     0,  -275,  -275,
    -275,     0,   695,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   -99,  -275,     0,  -275,  -275,
    -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -275,  -275,  -275,   -91,     0,     0,  -275,
       0,  -275,  -275,   268,     0,     5,     6,     7,     8,     9,
    -544,  -544,  -544,    10,    11,     0,     0,  -544,    12,     0,
      13,    14,    15,    16,    17,    18,    19,     0,     0,     0,
       0,     0,    20,    21,    22,    23,    24,    25,    26,     0,
       0,    27,     0,     0,     0,     0,     0,    28,     0,    30,
      31,    32,     0,    33,    34,    35,    36,    37,    38,     0,
      39,    40,    41,     0,     0,    42,     0,     0,    43,    44,
       0,    45,    46,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    47,     0,     0,    48,    49,     0,    50,    51,     0,
      52,     0,    53,    54,    55,    56,    57,    58,    59,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    61,    62,
      63,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   268,     0,     5,     6,     7,     8,
       9,  -544,  -544,  -544,    10,    11,     0,  -544,  -544,    12,
       0,    13,    14,    15,    16,    17,    18,    19,     0,     0,
       0,     0,     0,    20,    21,    22,    23,    24,    25,    26,
       0,     0,    27,     0,     0,     0,     0,     0,    28,     0,
      30,    31,    32,     0,    33,    34,    35,    36,    37,    38,
       0,    39,    40,    41,     0,     0,    42,     0,     0,    43,
      44,     0,    45,    46,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    47,     0,     0,    48,    49,     0,    50,    51,
       0,    52,     0,    53,    54,    55,    56,    57,    58,    59,
       0,     0,    60,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    61,
      62,    63,     0,     0,     0,     0,     0,     0,     0,     0,
     268,     0,     5,     6,     7,     8,     9,     0,     0,  -544,
      10,    11,  -544,  -544,  -544,    12,  -544,    13,    14,    15,
      16,    17,    18,    19,     0,     0,     0,     0,     0,    20,
      21,    22,    23,    24,    25,    26,     0,     0,    27,     0,
       0,     0,     0,     0,    28,     0,    30,    31,    32,     0,
      33,    34,    35,    36,    37,    38,     0,    39,    40,    41,
       0,     0,    42,     0,     0,    43,    44,     0,    45,    46,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    47,     0,
       0,    48,    49,     0,    50,    51,     0,    52,     0,    53,
      54,    55,    56,    57,    58,    59,     0,     0,    60,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    61,    62,    63,     0,     0,
       0,     0,     0,     0,     0,     0,   268,     0,     5,     6,
       7,     8,     9,     0,     0,  -544,    10,    11,  -544,  -544,
    -544,    12,     0,    13,    14,    15,    16,    17,    18,    19,
       0,     0,     0,     0,     0,    20,    21,    22,    23,    24,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
      28,     0,    30,    31,    32,     0,    33,    34,    35,    36,
      37,    38,     0,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    47,     0,     0,    48,    49,     0,
      50,    51,     0,    52,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     4,     0,     5,     6,     7,     8,     9,     0,     0,
       0,    10,    11,     0,  -544,  -544,    12,     0,    13,    14,
      15,    16,    17,    18,    19,     0,     0,     0,     0,     0,
      20,    21,    22,    23,    24,    25,    26,     0,     0,    27,
       0,     0,     0,     0,     0,    28,    29,    30,    31,    32,
       0,    33,    34,    35,    36,    37,    38,     0,    39,    40,
      41,     0,     0,    42,     0,     0,    43,    44,     0,    45,
      46,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    47,
       0,     0,    48,    49,     0,    50,    51,     0,    52,     0,
      53,    54,    55,    56,    57,    58,    59,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    61,    62,    63,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  -544,     0,     0,     0,     0,     0,     0,     0,  -544,
    -544,   268,     0,     5,     6,     7,     8,     9,     0,  -544,
    -544,    10,    11,     0,     0,     0,    12,     0,    13,    14,
      15,    16,    17,    18,    19,     0,     0,     0,     0,     0,
      20,    21,    22,    23,    24,    25,    26,     0,     0,    27,
       0,     0,     0,     0,     0,    28,     0,    30,    31,    32,
       0,    33,    34,    35,    36,    37,    38,     0,    39,    40,
      41,     0,     0,    42,     0,     0,    43,    44,     0,    45,
      46,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    47,
       0,     0,    48,    49,     0,    50,    51,     0,    52,     0,
      53,    54,    55,    56,    57,    58,    59,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    61,    62,    63,     0,
       0,     0,     0,     0,     0,     0,   268,     0,     5,     6,
       7,     8,     9,     0,     0,     0,    10,    11,     0,  -544,
    -544,    12,     0,    13,    14,    15,    16,    17,    18,    19,
       0,     0,     0,     0,     0,    20,    21,    22,    23,    24,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
      28,     0,    30,    31,    32,     0,    33,    34,    35,    36,
      37,    38,     0,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    47,     0,     0,   269,    49,     0,
      50,    51,     0,    52,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -544,     0,  -544,  -544,   268,     0,     5,     6,
       7,     8,     9,     0,     0,     0,    10,    11,     0,     0,
       0,    12,     0,    13,    14,    15,    16,    17,    18,    19,
       0,     0,     0,     0,     0,    20,    21,    22,    23,    24,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
      28,     0,    30,    31,    32,     0,    33,    34,    35,    36,
      37,    38,     0,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    47,     0,     0,    48,    49,     0,
      50,    51,     0,    52,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -544,     0,  -544,  -544,   268,     0,     5,     6,
       7,     8,     9,     0,     0,     0,    10,    11,     0,     0,
       0,    12,     0,    13,    14,    15,    16,    17,    18,    19,
       0,     0,     0,     0,     0,    20,    21,    22,    23,    24,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
      28,     0,    30,    31,    32,     0,    33,    34,    35,    36,
      37,    38,     0,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    47,     0,     0,    48,    49,     0,
      50,    51,     0,    52,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -544,   268,     0,     5,
       6,     7,     8,     9,  -544,  -544,  -544,    10,    11,     0,
       0,     0,    12,     0,    13,    14,    15,    16,    17,    18,
      19,     0,     0,     0,     0,     0,    20,    21,    22,    23,
      24,    25,    26,     0,     0,    27,     0,     0,     0,     0,
       0,    28,     0,    30,    31,    32,     0,    33,    34,    35,
      36,    37,    38,     0,    39,    40,    41,     0,     0,    42,
       0,     0,    43,    44,     0,    45,    46,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    47,     0,     0,    48,    49,
       0,    50,    51,     0,    52,     0,    53,    54,    55,    56,
      57,    58,    59,     0,     0,    60,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    61,    62,    63,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     7,     0,     9,     0,
       0,     0,    10,    11,     0,  -544,  -544,    12,     0,    13,
      14,    15,    16,    17,    18,    19,     0,     0,     0,     0,
       0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,    31,
      32,     0,    33,    34,    35,    36,    37,    38,   238,    39,
      40,    41,     0,     0,    42,     0,     0,    43,    44,     0,
      45,    46,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     213,     0,     0,   111,    49,     0,    50,    51,     0,   239,
     240,    53,    54,    55,    56,    57,    58,    59,     0,     0,
      60,     0,     0,     0,     0,     0,     0,     5,     6,     7,
       0,     9,     0,     0,     0,    10,    11,    61,   241,    63,
      12,     0,    13,    14,    15,    16,    17,    18,    19,     0,
       0,     0,     0,     0,    20,    21,    22,    23,    24,    25,
      26,   223,     0,    27,     0,     0,     0,     0,     0,     0,
       0,     0,    31,    32,     0,    33,    34,    35,    36,    37,
      38,     0,    39,    40,    41,     0,     0,    42,     0,     0,
      43,    44,     0,    45,    46,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   213,     0,     0,   111,    49,     0,    50,
      51,     0,     0,     0,    53,    54,    55,    56,    57,    58,
      59,     0,     0,    60,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      61,    62,    63,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     5,     6,     7,     0,     9,     0,     0,     0,
      10,    11,     0,   222,   223,    12,     0,    13,    14,    15,
      16,    17,    18,    19,     0,     0,     0,     0,     0,    20,
      21,    22,    23,    24,    25,    26,     0,     0,    27,     0,
       0,     0,     0,     0,     0,     0,     0,    31,    32,     0,
      33,    34,    35,    36,    37,    38,     0,    39,    40,    41,
       0,     0,    42,     0,     0,    43,    44,     0,    45,    46,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   213,     0,
       0,   111,    49,     0,    50,    51,     0,     0,     0,    53,
      54,    55,    56,    57,    58,    59,     0,     0,    60,     0,
       0,     0,     0,     0,     0,     5,     6,     7,     8,     9,
       0,     0,     0,    10,    11,    61,    62,    63,    12,     0,
      13,    14,    15,    16,    17,    18,    19,     0,     0,     0,
       0,     0,    20,    21,    22,    23,    24,    25,    26,   223,
       0,    27,     0,     0,     0,     0,     0,    28,    29,    30,
      31,    32,     0,    33,    34,    35,    36,    37,    38,     0,
      39,    40,    41,     0,     0,    42,     0,     0,    43,    44,
       0,    45,    46,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    47,     0,     0,    48,    49,     0,    50,    51,     0,
      52,     0,    53,    54,    55,    56,    57,    58,    59,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    61,    62,
      63,     0,     0,     0,     0,     0,     0,     5,     6,     7,
       8,     9,     0,     0,     0,    10,    11,     0,     0,     0,
      12,   412,    13,    14,    15,    16,    17,    18,    19,     0,
       0,     0,     0,     0,    20,    21,    22,    23,    24,    25,
      26,     0,     0,    27,     0,     0,     0,     0,     0,    28,
       0,    30,    31,    32,     0,    33,    34,    35,    36,    37,
      38,     0,    39,    40,    41,     0,     0,    42,     0,     0,
      43,    44,     0,    45,    46,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    47,     0,     0,    48,    49,     0,    50,
      51,     0,    52,     0,    53,    54,    55,    56,    57,    58,
      59,     0,     0,    60,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      61,    62,    63,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   412,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,     0,     0,
       0,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,     0,     0,     0,     0,     0,   155,   156,   157,   158,
     159,   160,   161,   162,    35,    36,   163,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     164,   165,   166,   167,   168,   169,   170,   171,   172,     0,
       0,   173,   174,     0,     0,   175,   176,   177,   178,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   179,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,     0,   190,   191,
       0,     0,     0,     0,     0,  -517,  -517,  -517,     0,  -517,
       0,     0,     0,  -517,  -517,     0,   192,   193,  -517,     0,
    -517,  -517,  -517,  -517,  -517,  -517,  -517,     0,  -517,     0,
       0,     0,  -517,  -517,  -517,  -517,  -517,  -517,  -517,     0,
       0,  -517,     0,     0,     0,     0,     0,     0,     0,     0,
    -517,  -517,     0,  -517,  -517,  -517,  -517,  -517,  -517,  -517,
    -517,  -517,  -517,     0,     0,  -517,     0,     0,  -517,  -517,
       0,  -517,  -517,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  -517,     0,     0,  -517,  -517,     0,  -517,  -517,     0,
    -517,  -517,  -517,  -517,  -517,  -517,  -517,  -517,  -517,     0,
       0,  -517,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  -517,  -517,
    -517,     0,     0,     0,     0,     0,  -518,  -518,  -518,     0,
    -518,     0,  -517,     0,  -518,  -518,     0,     0,  -517,  -518,
       0,  -518,  -518,  -518,  -518,  -518,  -518,  -518,     0,  -518,
       0,     0,     0,  -518,  -518,  -518,  -518,  -518,  -518,  -518,
       0,     0,  -518,     0,     0,     0,     0,     0,     0,     0,
       0,  -518,  -518,     0,  -518,  -518,  -518,  -518,  -518,  -518,
    -518,  -518,  -518,  -518,     0,     0,  -518,     0,     0,  -518,
    -518,     0,  -518,  -518,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -518,     0,     0,  -518,  -518,     0,  -518,  -518,
       0,  -518,  -518,  -518,  -518,  -518,  -518,  -518,  -518,  -518,
       0,     0,  -518,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -518,
    -518,  -518,     0,     0,     0,     0,     0,  -520,  -520,  -520,
       0,  -520,     0,  -518,     0,  -520,  -520,     0,     0,  -518,
    -520,     0,  -520,  -520,  -520,  -520,  -520,  -520,  -520,     0,
       0,     0,     0,     0,  -520,  -520,  -520,  -520,  -520,  -520,
    -520,     0,     0,  -520,     0,     0,     0,     0,     0,     0,
       0,     0,  -520,  -520,     0,  -520,  -520,  -520,  -520,  -520,
    -520,  -520,  -520,  -520,  -520,     0,     0,  -520,     0,     0,
    -520,  -520,     0,  -520,  -520,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -520,   733,     0,  -520,  -520,     0,  -520,
    -520,     0,  -520,  -520,  -520,  -520,  -520,  -520,  -520,  -520,
    -520,     0,     0,  -520,     0,     0,     0,   -97,     0,     0,
       0,     0,     0,     0,     0,  -522,  -522,  -522,     0,  -522,
    -520,  -520,  -520,  -522,  -522,     0,     0,     0,  -522,     0,
    -522,  -522,  -522,  -522,  -522,  -522,  -522,     0,     0,     0,
    -520,     0,  -522,  -522,  -522,  -522,  -522,  -522,  -522,     0,
       0,  -522,     0,     0,     0,     0,     0,     0,     0,     0,
    -522,  -522,     0,  -522,  -522,  -522,  -522,  -522,  -522,  -522,
    -522,  -522,  -522,     0,     0,  -522,     0,     0,  -522,  -522,
       0,  -522,  -522,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  -522,     0,     0,  -522,  -522,     0,  -522,  -522,     0,
    -522,  -522,  -522,  -522,  -522,  -522,  -522,  -522,  -522,     0,
       0,  -522,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -523,  -523,  -523,     0,  -523,  -522,  -522,
    -522,  -523,  -523,     0,     0,     0,  -523,     0,  -523,  -523,
    -523,  -523,  -523,  -523,  -523,     0,     0,     0,  -522,     0,
    -523,  -523,  -523,  -523,  -523,  -523,  -523,     0,     0,  -523,
       0,     0,     0,     0,     0,     0,     0,     0,  -523,  -523,
       0,  -523,  -523,  -523,  -523,  -523,  -523,  -523,  -523,  -523,
    -523,     0,     0,  -523,     0,     0,  -523,  -523,     0,  -523,
    -523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -523,
       0,     0,  -523,  -523,     0,  -523,  -523,     0,  -523,  -523,
    -523,  -523,  -523,  -523,  -523,  -523,  -523,     0,     0,  -523,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -523,  -523,  -523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -523,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,     0,     0,     0,   145,   146,   147,   199,   200,   201,
     202,   152,   153,   154,     0,     0,     0,     0,     0,   155,
     156,   157,   203,   204,   160,   205,   162,   293,   294,   206,
     295,     0,     0,     0,     0,     0,     0,   296,     0,     0,
       0,     0,     0,   164,   165,   166,   167,   168,   169,   170,
     171,   172,     0,     0,   173,   174,     0,     0,   175,   176,
     177,   178,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   179,     0,     0,     0,     0,     0,     0,   297,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
       0,   190,   191,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   192,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,     0,     0,     0,   145,   146,   147,
     199,   200,   201,   202,   152,   153,   154,     0,     0,     0,
       0,     0,   155,   156,   157,   203,   204,   160,   205,   162,
     293,   294,   206,   295,     0,     0,     0,     0,     0,     0,
     296,     0,     0,     0,     0,     0,   164,   165,   166,   167,
     168,   169,   170,   171,   172,     0,     0,   173,   174,     0,
       0,   175,   176,   177,   178,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   179,     0,     0,     0,     0,
       0,     0,   393,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,     0,   190,   191,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   192,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,     0,     0,     0,
     145,   146,   147,   199,   200,   201,   202,   152,   153,   154,
       0,     0,     0,     0,     0,   155,   156,   157,   203,   204,
     160,   205,   162,     0,     0,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   164,
     165,   166,   167,   168,   169,   170,   171,   172,     0,     0,
     173,   174,     0,     0,   175,   176,   177,   178,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   179,     0,
       0,   207,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,     0,   190,   191,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   192,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
       0,     0,     0,   145,   146,   147,   199,   200,   201,   202,
     152,   153,   154,     0,     0,     0,     0,     0,   155,   156,
     157,   203,   204,   160,   205,   162,     0,     0,   206,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   164,   165,   166,   167,   168,   169,   170,   171,
     172,     0,     0,   173,   174,     0,     0,   175,   176,   177,
     178,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   179,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,     0,
     190,   191,     0,     0,     5,     6,     7,     0,     9,     0,
       0,     0,    10,    11,     0,     0,     0,    12,   192,    13,
      14,    15,   101,   102,    18,    19,     0,     0,     0,     0,
       0,   103,   104,   105,    23,    24,    25,    26,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,    31,
      32,     0,    33,    34,    35,    36,    37,    38,     0,    39,
      40,    41,     0,     0,    42,     0,     0,    43,    44,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     286,     0,     0,   111,    49,     0,    50,    51,     0,     0,
       0,    53,    54,    55,    56,    57,    58,    59,     0,     0,
      60,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     6,     7,     0,     9,     0,     0,   112,    10,    11,
       0,     0,     0,    12,     0,    13,    14,    15,   101,   102,
      18,    19,     0,     0,     0,   287,     0,   103,   104,   105,
      23,    24,    25,    26,     0,     0,   106,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    32,     0,    33,    34,
      35,    36,    37,    38,     0,    39,    40,    41,     0,     0,
      42,     0,     0,    43,    44,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   286,     0,     0,   111,
      49,     0,    50,    51,     0,     0,     0,    53,    54,    55,
      56,    57,    58,    59,     0,     0,    60,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     5,     6,     7,     8,
       9,     0,     0,   112,    10,    11,     0,     0,     0,    12,
       0,    13,    14,    15,    16,    17,    18,    19,     0,     0,
       0,   520,     0,    20,    21,    22,    23,    24,    25,    26,
       0,     0,    27,     0,     0,     0,     0,     0,    28,    29,
      30,    31,    32,     0,    33,    34,    35,    36,    37,    38,
       0,    39,    40,    41,     0,     0,    42,     0,     0,    43,
      44,     0,    45,    46,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    47,     0,     0,    48,    49,     0,    50,    51,
       0,    52,     0,    53,    54,    55,    56,    57,    58,    59,
       0,     0,    60,     0,     0,     0,     0,     0,     0,     5,
       6,     7,     8,     9,     0,     0,     0,    10,    11,    61,
      62,    63,    12,     0,    13,    14,    15,    16,    17,    18,
      19,     0,     0,     0,     0,     0,    20,    21,    22,    23,
      24,    25,    26,     0,     0,    27,     0,     0,     0,     0,
       0,    28,     0,    30,    31,    32,     0,    33,    34,    35,
      36,    37,    38,     0,    39,    40,    41,     0,     0,    42,
       0,     0,    43,    44,     0,    45,    46,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    47,     0,     0,    48,    49,
       0,    50,    51,     0,    52,     0,    53,    54,    55,    56,
      57,    58,    59,     0,     0,    60,     0,     0,     0,     0,
       0,     0,     5,     6,     7,     0,     9,     0,     0,     0,
      10,    11,    61,    62,    63,    12,     0,    13,    14,    15,
      16,    17,    18,    19,     0,     0,     0,     0,     0,    20,
      21,    22,    23,    24,    25,    26,     0,     0,   106,     0,
       0,     0,     0,     0,     0,     0,     0,    31,    32,     0,
      33,    34,    35,    36,    37,    38,   238,    39,    40,    41,
       0,     0,    42,     0,     0,    43,    44,     0,    45,    46,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   213,     0,
       0,   111,    49,     0,    50,    51,     0,   239,   240,    53,
      54,    55,    56,    57,    58,    59,     0,     0,    60,     0,
       0,     0,     0,     0,     0,     5,     6,     7,     0,     9,
       0,     0,     0,    10,    11,    61,   241,    63,    12,     0,
      13,    14,    15,   101,   102,    18,    19,     0,     0,     0,
       0,     0,   103,   104,   105,    23,    24,    25,    26,     0,
       0,   106,     0,     0,     0,     0,     0,     0,     0,     0,
      31,    32,     0,    33,    34,    35,    36,    37,    38,   238,
      39,    40,    41,     0,     0,    42,     0,     0,    43,    44,
       0,    45,    46,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   213,     0,     0,   111,    49,     0,    50,    51,     0,
     630,   240,    53,    54,    55,    56,    57,    58,    59,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     5,     6,
       7,     0,     9,     0,     0,     0,    10,    11,    61,   241,
      63,    12,     0,    13,    14,    15,   101,   102,    18,    19,
       0,     0,     0,     0,     0,   103,   104,   105,    23,    24,
      25,    26,     0,     0,   106,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    32,     0,    33,    34,    35,    36,
      37,    38,   238,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   213,     0,     0,   111,    49,     0,
      50,    51,     0,   239,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     5,     6,     7,     0,     9,     0,     0,     0,    10,
      11,    61,   241,    63,    12,     0,    13,    14,    15,   101,
     102,    18,    19,     0,     0,     0,     0,     0,   103,   104,
     105,    23,    24,    25,    26,     0,     0,   106,     0,     0,
       0,     0,     0,     0,     0,     0,    31,    32,     0,    33,
      34,    35,    36,    37,    38,   238,    39,    40,    41,     0,
       0,    42,     0,     0,    43,    44,     0,    45,    46,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   213,     0,     0,
     111,    49,     0,    50,    51,     0,     0,   240,    53,    54,
      55,    56,    57,    58,    59,     0,     0,    60,     0,     0,
       0,     0,     0,     0,     5,     6,     7,     0,     9,     0,
       0,     0,    10,    11,    61,   241,    63,    12,     0,    13,
      14,    15,   101,   102,    18,    19,     0,     0,     0,     0,
       0,   103,   104,   105,    23,    24,    25,    26,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,    31,
      32,     0,    33,    34,    35,    36,    37,    38,   238,    39,
      40,    41,     0,     0,    42,     0,     0,    43,    44,     0,
      45,    46,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     213,     0,     0,   111,    49,     0,    50,    51,     0,   630,
       0,    53,    54,    55,    56,    57,    58,    59,     0,     0,
      60,     0,     0,     0,     0,     0,     0,     5,     6,     7,
       0,     9,     0,     0,     0,    10,    11,    61,   241,    63,
      12,     0,    13,    14,    15,   101,   102,    18,    19,     0,
       0,     0,     0,     0,   103,   104,   105,    23,    24,    25,
      26,     0,     0,   106,     0,     0,     0,     0,     0,     0,
       0,     0,    31,    32,     0,    33,    34,    35,    36,    37,
      38,   238,    39,    40,    41,     0,     0,    42,     0,     0,
      43,    44,     0,    45,    46,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   213,     0,     0,   111,    49,     0,    50,
      51,     0,     0,     0,    53,    54,    55,    56,    57,    58,
      59,     0,     0,    60,     0,     0,     0,     0,     0,     0,
       5,     6,     7,     0,     9,     0,     0,     0,    10,    11,
      61,   241,    63,    12,     0,    13,    14,    15,    16,    17,
      18,    19,     0,     0,     0,     0,     0,    20,    21,    22,
      23,    24,    25,    26,     0,     0,   106,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    32,     0,    33,    34,
      35,    36,    37,    38,     0,    39,    40,    41,     0,     0,
      42,     0,     0,    43,    44,     0,    45,    46,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   213,     0,     0,   111,
      49,     0,    50,    51,     0,   514,     0,    53,    54,    55,
      56,    57,    58,    59,     0,     0,    60,     0,     0,     0,
       0,     0,     0,     5,     6,     7,     0,     9,     0,     0,
       0,    10,    11,    61,   241,    63,    12,     0,    13,    14,
      15,   101,   102,    18,    19,     0,     0,     0,     0,     0,
     103,   104,   105,    23,    24,    25,    26,     0,     0,   106,
       0,     0,     0,     0,     0,     0,     0,     0,    31,    32,
       0,    33,    34,    35,    36,    37,    38,     0,    39,    40,
      41,     0,     0,    42,     0,     0,    43,    44,     0,    45,
      46,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   213,
       0,     0,   111,    49,     0,    50,    51,     0,   239,     0,
      53,    54,    55,    56,    57,    58,    59,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     5,     6,     7,     0,
       9,     0,     0,     0,    10,    11,    61,   241,    63,    12,
       0,    13,    14,    15,   101,   102,    18,    19,     0,     0,
       0,     0,     0,   103,   104,   105,    23,    24,    25,    26,
       0,     0,   106,     0,     0,     0,     0,     0,     0,     0,
       0,    31,    32,     0,    33,    34,    35,    36,    37,    38,
       0,    39,    40,    41,     0,     0,    42,     0,     0,    43,
      44,     0,    45,    46,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   213,     0,     0,   111,    49,     0,    50,    51,
       0,   514,     0,    53,    54,    55,    56,    57,    58,    59,
       0,     0,    60,     0,     0,     0,     0,     0,     0,     5,
       6,     7,     0,     9,     0,     0,     0,    10,    11,    61,
     241,    63,    12,     0,    13,    14,    15,   101,   102,    18,
      19,     0,     0,     0,     0,     0,   103,   104,   105,    23,
      24,    25,    26,     0,     0,   106,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,     0,    33,    34,    35,
      36,    37,    38,     0,    39,    40,    41,     0,     0,    42,
       0,     0,    43,    44,     0,    45,    46,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   213,     0,     0,   111,    49,
       0,    50,    51,     0,   782,     0,    53,    54,    55,    56,
      57,    58,    59,     0,     0,    60,     0,     0,     0,     0,
       0,     0,     5,     6,     7,     0,     9,     0,     0,     0,
      10,    11,    61,   241,    63,    12,     0,    13,    14,    15,
     101,   102,    18,    19,     0,     0,     0,     0,     0,   103,
     104,   105,    23,    24,    25,    26,     0,     0,   106,     0,
       0,     0,     0,     0,     0,     0,     0,    31,    32,     0,
      33,    34,    35,    36,    37,    38,     0,    39,    40,    41,
       0,     0,    42,     0,     0,    43,    44,     0,    45,    46,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   213,     0,
       0,   111,    49,     0,    50,    51,     0,   630,     0,    53,
      54,    55,    56,    57,    58,    59,     0,     0,    60,     0,
       0,     0,     0,     0,     0,     5,     6,     7,     0,     9,
       0,     0,     0,    10,    11,    61,   241,    63,    12,     0,
      13,    14,    15,    16,    17,    18,    19,     0,     0,     0,
       0,     0,    20,    21,    22,    23,    24,    25,    26,     0,
       0,    27,     0,     0,     0,     0,     0,     0,     0,     0,
      31,    32,     0,    33,    34,    35,    36,    37,    38,     0,
      39,    40,    41,     0,     0,    42,     0,     0,    43,    44,
       0,    45,    46,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   213,     0,     0,   111,    49,     0,    50,    51,     0,
       0,     0,    53,    54,    55,    56,    57,    58,    59,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     5,     6,
       7,     0,     9,     0,     0,     0,    10,    11,    61,    62,
      63,    12,     0,    13,    14,    15,   101,   102,    18,    19,
       0,     0,     0,     0,     0,   103,   104,   105,    23,    24,
      25,    26,     0,     0,   106,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    32,     0,    33,    34,    35,    36,
      37,    38,     0,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   213,     0,     0,   111,    49,     0,
      50,    51,     0,     0,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     5,     6,     7,     0,     9,     0,     0,     0,    10,
      11,    61,   241,    63,    12,     0,    13,    14,    15,    16,
      17,    18,    19,     0,     0,     0,     0,     0,    20,    21,
      22,    23,    24,    25,    26,     0,     0,   106,     0,     0,
       0,     0,     0,     0,     0,     0,    31,    32,     0,    33,
      34,    35,    36,    37,    38,     0,    39,    40,    41,     0,
       0,    42,     0,     0,    43,    44,     0,    45,    46,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   213,     0,     0,
     111,    49,     0,    50,    51,     0,     0,     0,    53,    54,
      55,    56,    57,    58,    59,     0,     0,    60,     0,     0,
       0,     0,     0,     0,     5,     6,     7,     0,     9,     0,
       0,     0,    10,    11,    61,   241,    63,    12,     0,    13,
      14,    15,   101,   102,    18,    19,     0,     0,     0,     0,
       0,   103,   104,   105,    23,    24,    25,    26,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,    31,
      32,     0,   107,    34,    35,    36,   108,    38,     0,    39,
      40,    41,     0,     0,    42,     0,     0,    43,    44,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   109,     0,     0,
     110,     0,     0,   111,    49,     0,    50,    51,     0,     0,
       0,    53,    54,    55,    56,    57,    58,    59,     0,     0,
      60,     0,     0,     5,     6,     7,     0,     9,     0,     0,
       0,    10,    11,     0,     0,     0,    12,   112,    13,    14,
      15,   101,   102,    18,    19,     0,     0,     0,     0,     0,
     103,   104,   105,    23,    24,    25,    26,     0,     0,   106,
       0,     0,     0,     0,     0,     0,     0,     0,    31,    32,
       0,    33,    34,    35,    36,    37,    38,     0,    39,    40,
      41,     0,     0,    42,     0,     0,    43,    44,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   231,
       0,     0,    48,    49,     0,    50,    51,     0,    52,     0,
      53,    54,    55,    56,    57,    58,    59,     0,     0,    60,
       0,     0,     5,     6,     7,     0,     9,     0,     0,     0,
      10,    11,     0,     0,     0,    12,   112,    13,    14,    15,
     101,   102,    18,    19,     0,     0,     0,     0,     0,   103,
     104,   105,    23,    24,    25,    26,     0,     0,   106,     0,
       0,     0,     0,     0,     0,     0,     0,    31,    32,     0,
      33,    34,    35,    36,    37,    38,     0,    39,    40,    41,
       0,     0,    42,     0,     0,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   286,     0,
       0,   332,    49,     0,    50,    51,     0,   333,     0,    53,
      54,    55,    56,    57,    58,    59,     0,     0,    60,     0,
       0,     5,     6,     7,     0,     9,     0,     0,     0,    10,
      11,     0,     0,     0,    12,   112,    13,    14,    15,   101,
     102,    18,    19,     0,     0,     0,     0,     0,   103,   104,
     105,    23,    24,    25,    26,     0,     0,   106,     0,     0,
       0,     0,     0,     0,     0,     0,    31,    32,     0,   107,
      34,    35,    36,   108,    38,     0,    39,    40,    41,     0,
       0,    42,     0,     0,    43,    44,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   110,     0,     0,
     111,    49,     0,    50,    51,     0,     0,     0,    53,    54,
      55,    56,    57,    58,    59,     0,     0,    60,     0,     0,
       5,     6,     7,     0,     9,     0,     0,     0,    10,    11,
       0,     0,     0,    12,   112,    13,    14,    15,   101,   102,
      18,    19,     0,     0,     0,     0,     0,   103,   104,   105,
      23,    24,    25,    26,     0,     0,   106,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    32,     0,    33,    34,
      35,    36,    37,    38,     0,    39,    40,    41,     0,     0,
      42,     0,     0,    43,    44,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   286,     0,     0,   332,
      49,     0,    50,    51,     0,     0,     0,    53,    54,    55,
      56,    57,    58,    59,     0,     0,    60,     0,     0,     5,
       6,     7,     0,     9,     0,     0,     0,    10,    11,     0,
       0,     0,    12,   112,    13,    14,    15,   101,   102,    18,
      19,     0,     0,     0,     0,     0,   103,   104,   105,    23,
      24,    25,    26,     0,     0,   106,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,     0,    33,    34,    35,
      36,    37,    38,     0,    39,    40,    41,     0,     0,    42,
       0,     0,    43,    44,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   865,     0,     0,   111,    49,
       0,    50,    51,     0,     0,     0,    53,    54,    55,    56,
      57,    58,    59,     0,     0,    60,     0,     0,     5,     6,
       7,     0,     9,     0,     0,     0,    10,    11,     0,     0,
       0,    12,   112,    13,    14,    15,   101,   102,    18,    19,
       0,     0,     0,     0,     0,   103,   104,   105,    23,    24,
      25,    26,     0,     0,   106,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    32,     0,    33,    34,    35,    36,
      37,    38,     0,    39,    40,    41,     0,     0,    42,     0,
       0,    43,    44,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   891,     0,     0,   111,    49,     0,
      50,    51,     0,     0,     0,    53,    54,    55,    56,    57,
      58,    59,     0,     0,    60,   563,   564,     0,     0,   565,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   112,     0,   164,   165,   166,   167,   168,   169,   170,
     171,   172,     0,     0,   173,   174,     0,     0,   175,   176,
     177,   178,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   179,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
       0,   190,   191,   584,   556,     0,     0,   585,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   192,
     255,   164,   165,   166,   167,   168,   169,   170,   171,   172,
       0,     0,   173,   174,     0,     0,   175,   176,   177,   178,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     179,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,     0,   190,
     191,   569,   564,     0,     0,   570,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   192,   255,   164,
     165,   166,   167,   168,   169,   170,   171,   172,     0,     0,
     173,   174,     0,     0,   175,   176,   177,   178,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   179,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,     0,   190,   191,   604,
     556,     0,     0,   605,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   192,   255,   164,   165,   166,
     167,   168,   169,   170,   171,   172,     0,     0,   173,   174,
       0,     0,   175,   176,   177,   178,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   179,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,     0,   190,   191,   607,   564,     0,
       0,   608,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   192,   255,   164,   165,   166,   167,   168,
     169,   170,   171,   172,     0,     0,   173,   174,     0,     0,
     175,   176,   177,   178,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   179,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,     0,   190,   191,   623,   556,     0,     0,   624,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   192,   255,   164,   165,   166,   167,   168,   169,   170,
     171,   172,     0,     0,   173,   174,     0,     0,   175,   176,
     177,   178,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   179,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
       0,   190,   191,   626,   564,     0,     0,   627,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   192,
     255,   164,   165,   166,   167,   168,   169,   170,   171,   172,
       0,     0,   173,   174,     0,     0,   175,   176,   177,   178,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     179,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,     0,   190,
     191,   651,   556,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   192,   255,   164,
     165,   166,   167,   168,   169,   170,   171,   172,     0,     0,
     173,   174,     0,     0,   175,   176,   177,   178,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   179,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,     0,   190,   191,   654,
     564,     0,     0,   655,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   192,   255,   164,   165,   166,
     167,   168,   169,   170,   171,   172,     0,     0,   173,   174,
       0,     0,   175,   176,   177,   178,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   179,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,     0,   190,   191,   930,   556,     0,
       0,   931,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   192,   255,   164,   165,   166,   167,   168,
     169,   170,   171,   172,     0,     0,   173,   174,     0,     0,
     175,   176,   177,   178,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   179,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,     0,   190,   191,   933,   564,     0,     0,   934,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   192,   255,   164,   165,   166,   167,   168,   169,   170,
     171,   172,     0,     0,   173,   174,     0,     0,   175,   176,
     177,   178,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   179,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
       0,   190,   191,   941,   556,     0,     0,   942,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   192,
     255,   164,   165,   166,   167,   168,   169,   170,   171,   172,
       0,     0,   173,   174,     0,     0,   175,   176,   177,   178,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     179,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,     0,   190,
     191,   569,   564,     0,     0,   570,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   192,   255,   164,
     165,   166,   167,   168,   169,   170,   171,   172,     0,     0,
     173,   174,     0,     0,   175,   176,   177,   178,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   179,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,     0,   190,   191,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   192
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-745)))

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-545)))

static const yytype_int16 yycheck[] =
{
       2,    83,    27,    62,   221,   248,    12,    10,    11,    12,
       8,    16,    17,    76,     7,    20,    28,   335,   409,     5,
       6,   424,    16,    17,   427,    52,    20,    22,   292,    15,
      28,   110,   292,    11,    12,   258,     4,    16,    17,   389,
     363,    20,   362,   467,   364,   427,    49,   367,    50,    51,
     273,   287,   335,   671,   277,   414,    50,    51,    12,    65,
      25,    54,   599,     2,   387,     4,    52,   588,   388,   280,
      25,    50,   609,   284,    65,    65,    48,   459,    27,   682,
     403,   297,   402,   756,   404,   606,   758,    29,   304,    26,
      76,   307,    13,   309,   417,   311,   520,   313,   418,    22,
     834,   668,    26,    98,   625,    13,   109,   851,   114,     0,
     105,    65,    88,    25,   362,   697,   364,    16,    17,   367,
      25,    20,   704,   269,   119,   445,    13,    37,    38,   572,
     573,   391,   653,   146,    79,    25,    88,   150,   386,    90,
     388,   140,    13,   113,    88,    25,   469,    88,   113,    55,
     470,    13,    28,    25,   402,    51,   404,   830,   113,    55,
     114,     1,   113,   144,    25,    90,   108,   143,   416,   150,
     418,   774,    25,   118,   144,    98,    90,   921,   912,   144,
     113,    90,   147,    58,    59,   150,   332,   144,   113,   144,
     142,   143,   147,   436,   146,   150,   444,   445,    13,   143,
     149,   150,   143,   140,   113,    55,   429,   879,   881,   146,
     433,    15,   113,    17,   207,   438,   140,   481,   224,   837,
     468,   481,   470,   144,   287,   448,   229,   230,   149,   150,
     453,   467,   144,   583,   224,   638,   639,   144,   911,   144,
     150,   149,   150,   593,    25,   247,   248,   252,   146,   254,
     255,   229,   230,   255,   144,   258,   259,   639,   252,   146,
     254,   215,   149,   150,   144,   271,   690,   221,   711,   146,
     224,   330,   144,   252,   841,   254,   335,   117,   149,   150,
     271,   271,   849,   144,   520,   146,   823,   149,   150,   144,
       2,   144,     4,     5,     6,   150,   519,   269,    10,    11,
      12,   287,    88,    15,    16,    17,   333,   386,    20,   149,
     150,   476,   371,   372,   146,   598,    88,   271,   321,   322,
     323,   324,    88,   326,   327,   279,   280,    90,   592,   268,
     284,   113,   592,    48,   149,   150,    48,    49,   149,   150,
      52,    58,    59,   321,   322,   323,   324,   333,    88,    17,
      62,   330,   320,   252,    26,   254,   335,   143,   363,    88,
     332,   363,    88,   144,    76,   146,    88,    55,   591,   364,
     142,   143,    88,   691,    26,    61,   382,   143,    64,    65,
      88,   320,   387,    88,   149,   387,   325,   390,   747,    25,
     633,    91,    88,   388,   396,   611,   111,   109,   403,   111,
     617,   403,   465,   143,   467,   144,   408,   413,   414,   404,
     144,   932,   417,   142,   143,   417,    88,   143,    88,   140,
     142,   143,   682,   418,   827,   111,   142,   143,   382,   441,
     424,   410,   823,   435,   436,   143,    88,   648,   143,   662,
     442,    71,    88,   441,    26,   827,   142,   143,   147,    61,
     445,   113,    64,    65,   690,   805,    71,   520,   150,   413,
     414,    88,   456,   148,   469,   141,    88,   469,   140,    88,
     142,   143,   142,   143,   146,   470,    55,   144,   144,   465,
      71,   467,    90,   150,    88,   487,    71,   489,   140,   491,
     142,   143,    97,   658,   659,   718,   142,   143,    88,   111,
     212,   144,    90,   442,    63,   113,    88,   750,   123,   124,
     125,   513,   451,    10,   864,   142,   143,   229,   230,   544,
     142,   143,    90,   142,   143,   113,    90,   668,    37,    38,
     671,     8,   755,    13,   520,    10,   144,   562,   142,   143,
     252,   682,   254,   255,    90,   113,   258,   259,   263,   113,
     810,   263,   142,   143,   269,   147,   268,   269,   140,   113,
     142,   143,    90,   588,   146,   560,   568,   113,   144,   571,
     572,   573,   144,   568,   580,   287,   571,    14,    15,   582,
     144,   606,   905,   141,   596,   113,   906,   589,   594,   144,
      51,   756,   587,   758,   589,   812,   598,   599,   596,   601,
     625,   818,   595,   144,   582,    26,   144,   609,   320,   321,
     322,   323,   324,   325,   326,   327,   144,   332,   330,   598,
     332,   333,   144,   335,   706,   628,   580,   690,   653,    71,
      63,   633,   691,    51,   144,   694,   695,   560,   144,   664,
     594,   700,   701,   144,   638,   568,   113,    10,   571,    15,
     628,   363,    10,   141,   144,   141,   904,   682,   906,   371,
     372,   144,   115,   617,   148,   830,   831,    88,    10,    10,
      90,     9,   920,    10,    90,   387,    91,   389,   390,   121,
     122,   123,   124,   125,   399,    10,   144,   399,    90,   144,
     147,   403,   407,   113,   648,   407,   837,   113,    51,   149,
      53,    54,    55,    56,   690,   417,   141,   713,   849,   711,
     851,   113,    10,   144,   879,   144,   881,   144,   144,   140,
     113,   142,   143,   725,   144,   146,   728,   730,   144,   118,
     442,   144,    51,    62,   793,    64,    65,   144,   144,   451,
      10,   747,   144,   144,   141,    10,   911,    10,   750,   774,
      10,   118,   730,   465,   144,   467,   144,   469,    62,   713,
      64,    65,   141,   765,   766,    10,   768,    55,   770,   771,
      63,    64,    65,   144,   776,   144,   778,   779,    55,   144,
     921,   496,   111,   144,   496,    51,    10,    53,    54,    55,
      56,   144,   144,   747,    10,     2,    51,     4,    53,    54,
      55,    56,   146,    10,    11,    12,   146,   111,   520,    16,
      17,   144,    15,    20,    63,    64,    65,   442,   111,   671,
       6,   823,   674,    71,   571,   811,    92,   908,   658,   912,
     682,   907,    98,    99,    63,    64,    65,    92,    86,    87,
     707,    48,    49,    98,    99,   613,    51,     7,    53,    54,
      55,    56,   810,   849,   120,    62,   668,   123,   812,   884,
     846,    -1,   111,   198,   818,   120,    64,    65,   123,    -1,
     582,   583,   120,   121,   122,   123,   124,   125,    -1,    -1,
      -1,   593,   111,    71,   150,   887,   888,   889,   890,    -1,
      -1,   146,   894,   895,   896,    -1,   898,   899,    86,    87,
     905,    -1,   109,   905,   111,   907,   908,   932,    71,    -1,
      -1,   906,   110,   111,    -1,    -1,   628,    64,    65,   238,
     239,   240,    -1,    86,    87,    -1,    51,    -1,    53,    54,
      55,    56,    -1,   121,   122,   123,   124,   125,   940,   144,
      -1,   943,   944,   945,   946,    40,    41,    42,    43,    44,
     665,   666,   954,   665,   666,   118,   119,   120,   121,   122,
     123,   124,   125,   110,   111,    -1,    -1,    92,   683,    -1,
      -1,   683,    -1,    98,    99,    -1,    -1,    -1,   690,   691,
      -1,    -1,   694,   695,    -1,   837,    -1,   839,   700,   701,
      -1,   843,    -1,   708,   709,   120,   708,   709,   123,   851,
      -1,   853,    63,    64,    65,   212,    63,    64,    65,    -1,
      -1,   330,   727,    -1,    -1,   727,   335,    -1,   730,    -1,
      -1,   146,   229,   230,    63,    64,    65,    -1,   743,   744,
     745,   743,   744,   745,    63,    64,    65,    51,    -1,    53,
      54,    55,    56,    -1,    -1,   252,    -1,   254,   255,    -1,
     111,   258,   259,    -1,   111,    51,   263,    53,    54,    55,
      56,   268,   269,   915,    55,    56,    57,    58,    59,   921,
      -1,   923,   111,    -1,   926,    -1,    -1,    -1,    92,    -1,
      -1,   793,   111,    -1,    98,    99,    -1,    -1,    -1,   804,
      -1,   410,   804,   805,    -1,    -1,    92,    -1,   950,   811,
     815,    -1,    98,   815,   423,   424,   120,    -1,   427,   123,
      -1,    -1,    -1,   320,   321,   322,   323,   324,   325,   326,
     327,    -1,    -1,   330,    -1,   332,    -1,    51,   335,    53,
      54,    55,    56,    -1,   846,    -1,    -1,   456,    -1,    51,
     459,    53,    54,    55,    56,    -1,    51,    -1,    53,    54,
      55,    56,   864,    -1,    -1,   870,   363,   872,   870,    -1,
     872,    -1,    -1,   878,   371,   372,   878,    -1,    92,    -1,
      -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,
     387,    -1,   389,   390,    -1,    -1,    51,    92,    53,    54,
      55,    56,   399,   905,    99,   514,   403,    -1,    -1,     0,
     407,    -1,    -1,    -1,    -1,    -1,    -1,     8,     9,    10,
     417,    -1,    13,    14,    15,   120,    17,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    26,    27,    92,    -1,    16,
      17,    -1,    -1,    20,    -1,   442,    37,    38,    -1,    40,
      41,    42,    43,    44,   451,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    45,    46,
      86,    87,   469,    50,    51,    -1,    51,    -1,    53,    54,
      55,    56,    -1,    -1,    -1,    62,    63,    -1,    -1,   598,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,   114,   496,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    -1,    -1,
      -1,   630,    -1,    98,    99,    -1,    -1,    -1,   144,   638,
     639,     2,    -1,     4,     5,     6,     7,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    15,   120,    -1,   656,   123,   140,
     141,    -1,   143,    -1,    -1,   146,   147,    -1,   149,   150,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    48,   687,    86,
      87,    52,    -1,    -1,    -1,   582,   583,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   593,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    76,    -1,   114,    -1,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   628,    -1,   742,    -1,    -1,    -1,    -1,    -1,    -1,
     111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   238,   239,   240,   241,    -1,    -1,    -1,   665,   666,
      -1,    -1,    -1,   782,    -1,   252,    -1,   254,   255,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   683,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   691,    -1,    -1,   694,   695,    -1,
      -1,    -1,    -1,   700,   701,    -1,    -1,    -1,    -1,    -1,
      -1,   708,   709,    -1,    -1,    -1,    -1,    -1,   827,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     727,    -1,    -1,   730,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   212,    -1,    -1,    -1,    -1,   743,   744,   745,    -1,
      -1,    -1,    -1,   330,    -1,    -1,    -1,    -1,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,    -1,   363,    -1,    -1,    -1,
      -1,    -1,   263,    -1,   371,   372,   793,   268,   269,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   804,   805,    -1,
     387,    -1,    -1,    -1,    -1,    -1,   287,    -1,   815,    -1,
      -1,    -1,    -1,    -1,   401,    -1,   403,    -1,   405,   406,
      -1,    -1,    -1,   410,    -1,    -1,    -1,    -1,    -1,    -1,
     417,    -1,    -1,    -1,    -1,    -1,   423,   424,    -1,   320,
     427,    -1,    -1,    -1,   325,    -1,    -1,    -1,    -1,    -1,
      -1,   332,   333,    -1,    -1,    -1,    -1,   864,    -1,   446,
      -1,    -1,    -1,   870,    -1,   872,    -1,    -1,    -1,   456,
      -1,   878,   459,     2,    -1,     4,    -1,    -1,    -1,    -1,
      -1,    -1,   469,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     2,    -1,     4,     5,     6,   905,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    15,    -1,   389,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   503,   504,   399,    48,
       2,    -1,     4,    -1,    -1,    -1,   407,   514,    -1,    -1,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    48,
      -1,    -1,    -1,    52,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   442,    -1,    -1,    -1,    -1,    48,    76,    -1,    -1,
     451,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   111,    -1,   465,    -1,   467,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   489,    -1,   491,    -1,    -1,
      -1,    -1,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   598,    -1,    -1,    -1,   496,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   520,
      -1,    -1,    -1,   630,    71,    72,    73,    74,    75,    76,
      77,   638,   639,    80,    81,    -1,    -1,    -1,    -1,    86,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   656,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   212,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
     687,    -1,   583,   212,   691,   692,    -1,   694,   695,    -1,
      -1,    -1,   593,   700,   701,    -1,    71,    72,    73,    74,
      75,    76,    77,   710,    -1,    80,    81,    -1,    -1,    -1,
     212,    86,    87,    -1,   263,    -1,    -1,    -1,    -1,   268,
     269,    -1,    -1,    -1,    -1,    -1,   733,   734,    -1,   736,
     737,    -1,    -1,    -1,   263,   742,    -1,    -1,    -1,   268,
     269,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   287,    -1,
      -1,   263,    -1,    -1,   665,   666,   268,   269,    -1,    -1,
      -1,   320,    -1,    -1,    -1,   782,   325,    -1,    -1,   786,
      -1,    -1,   683,   332,    -1,    -1,   793,    -1,    -1,   690,
      -1,   320,    -1,    -1,    -1,    -1,   325,    -1,    -1,    -1,
      -1,    -1,    -1,   332,   333,    -1,    -1,   708,   709,    -1,
      -1,    -1,    -1,   820,    -1,    -1,    -1,    -1,   320,    -1,
     827,    -1,    -1,   325,    -1,    -1,   727,    -1,    -1,    -1,
     332,    -1,    -1,   335,    -1,    -1,    -1,    -1,    -1,    -1,
     389,    -1,   743,   744,   745,    -1,    -1,    -1,    -1,    -1,
     399,   765,   766,    -1,   768,    -1,   770,   771,   407,    -1,
     389,    -1,   776,    -1,   778,   779,    -1,    -1,    -1,    -1,
     399,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   407,    71,
      72,    73,    74,    75,    76,    77,    78,   389,    80,    81,
      -1,    -1,    -1,   442,    86,    87,    -1,   399,   905,    -1,
      -1,    -1,   451,   804,   805,   407,    -1,    -1,    -1,    -1,
     811,    -1,    -1,   442,   815,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   451,    44,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,    -1,    -1,   465,    -1,   467,    -1,
     442,    -1,    -1,    -1,    -1,   846,    -1,   496,    -1,   451,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,   864,    -1,    86,    87,   496,    -1,   870,
      -1,   872,    -1,   887,   888,   889,   890,   878,    -1,    -1,
     894,   895,   896,    -1,   898,   899,    -1,    -1,    -1,    -1,
      -1,   520,    -1,   114,   496,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   940,    -1,    -1,   943,
     944,   945,   946,    -1,   583,    -1,    -1,    51,    52,    -1,
     954,    55,    -1,    -1,   593,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   583,    69,    70,    71,    72,    73,
      74,    75,    76,    77,   593,    -1,    80,    81,    -1,    -1,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   583,    -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,
      -1,   593,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,    -1,   127,   128,    -1,   665,   666,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   145,   146,    -1,   683,    -1,   665,   666,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   683,    -1,    -1,    -1,    -1,   708,
     709,   690,    -1,   665,   666,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   727,   708,
     709,   683,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   691,
      -1,    -1,    -1,    -1,   743,   744,   745,    -1,   727,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   708,   709,    -1,    -1,
      -1,    -1,    -1,    -1,   743,   744,   745,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   727,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   743,   744,   745,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   804,   805,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   815,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   804,   805,    -1,    -1,    -1,
      -1,    -1,   811,    -1,    -1,    -1,   815,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   804,   805,    -1,    -1,    -1,    -1,    -1,   811,
      -1,    -1,    -1,   815,    -1,   864,    -1,   846,    -1,    -1,
      -1,   870,    -1,   872,    -1,    -1,    -1,    -1,    -1,   878,
      -1,    -1,    -1,    -1,    -1,   864,    -1,    -1,    -1,    -1,
      -1,   870,    -1,   872,    -1,    -1,    -1,    -1,    -1,   878,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   864,    -1,    -1,    -1,    -1,    -1,   870,    -1,
     872,    -1,    -1,    -1,     0,     1,   878,     3,     4,     5,
       6,     7,    -1,    -1,    -1,    11,    12,    -1,    -1,    -1,
      16,    -1,    18,    19,    20,    21,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    47,    48,    49,    -1,    51,    52,    53,    54,    55,
      56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,    -1,
      66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,
      96,    -1,    98,    -1,   100,   101,   102,   103,   104,   105,
     106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     0,    -1,
     126,   127,   128,    -1,    -1,    -1,     8,     9,    10,    -1,
      -1,    13,    14,    15,    -1,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   149,   150,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    40,    41,
      42,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    -1,    -1,    86,    87,    -1,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    -1,    -1,    86,    87,    88,    -1,    90,    91,
      -1,    -1,    -1,    -1,   114,    97,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   108,    -1,    -1,    -1,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,    -1,    -1,    -1,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,   143,   144,     0,    -1,   147,   148,   149,   150,    -1,
      -1,     8,     9,    10,    -1,    -1,    13,    14,    15,    -1,
      17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      27,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    -1,    40,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    -1,
      -1,    86,    87,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    -1,    -1,    86,
      87,    88,    -1,    -1,    91,    -1,    -1,    -1,    -1,   114,
      97,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   140,   141,   142,   143,   144,     0,   146,
     147,   148,   149,   150,    -1,    -1,     8,     9,    10,    -1,
      -1,    13,    14,    15,    -1,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    40,    41,
      42,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    -1,    -1,    86,    87,    -1,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    -1,    -1,    86,    87,    88,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    97,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   108,    -1,    -1,    -1,
      -1,    -1,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,   143,   144,     0,    -1,   147,   148,   149,   150,    -1,
      -1,     8,     9,    10,    -1,    -1,    13,    14,    15,    -1,
      17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      27,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    -1,    40,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    -1,    -1,    86,
      87,    88,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,
      97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   140,   141,   142,   143,   144,     0,   146,
     147,   148,   149,   150,    -1,    -1,     8,     9,    10,    -1,
      -1,    13,    14,    15,    -1,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    40,    41,
      42,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    -1,    -1,    86,    87,    88,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    97,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,   143,   144,     0,   146,   147,   148,   149,   150,    -1,
      -1,     8,     9,    10,    -1,    -1,    -1,    14,    15,    -1,
      17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    -1,    40,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    -1,    -1,    86,
      87,    88,     0,    90,    -1,    -1,    -1,    -1,    -1,    -1,
       8,     9,    10,    -1,    -1,    -1,    14,    15,    -1,    17,
      -1,    -1,    -1,    -1,    -1,    -1,   113,   114,    -1,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    37,
      38,    -1,    40,    41,    42,    43,    44,    -1,    -1,    -1,
      -1,    -1,    -1,   140,   141,   142,   143,   144,    -1,    -1,
     147,    -1,   149,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    -1,    -1,    86,    87,
      88,    -1,    90,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   113,   114,    -1,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,   143,   144,    -1,    -1,   147,
      -1,   149,   150,     1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    47,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    -1,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,
      -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,
      98,    -1,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,   127,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     1,    -1,     3,     4,     5,     6,
       7,   149,   150,    10,    11,    12,    -1,    14,    15,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    -1,    -1,
      -1,    -1,    -1,    30,    31,    32,    33,    34,    35,    36,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      47,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      -1,    58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,
      67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,
      -1,    98,    -1,   100,   101,   102,   103,   104,   105,   106,
      -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
     127,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,     3,     4,     5,     6,     7,    -1,    -1,    10,
      11,    12,   149,   150,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,
      31,    32,    33,    34,    35,    36,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    47,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    -1,    58,    59,    60,
      -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,
      -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,   109,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   126,   127,   128,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,     4,
       5,     6,     7,    -1,    -1,    10,    11,    12,   149,   150,
      15,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    98,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,     3,     4,     5,     6,     7,    -1,    -1,
      -1,    11,    12,    -1,   149,   150,    16,    -1,    18,    19,
      20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,    49,
      -1,    51,    52,    53,    54,    55,    56,    -1,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,   109,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   126,   127,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   149,
     150,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    -1,    18,    19,
      20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    47,    48,    49,
      -1,    51,    52,    53,    54,    55,    56,    -1,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,   109,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   126,   127,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,     4,
       5,     6,     7,    -1,    -1,    -1,    11,    12,    -1,   149,
     150,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    98,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,    -1,   149,   150,     1,    -1,     3,     4,
       5,     6,     7,    -1,    -1,    -1,    11,    12,    -1,    -1,
      -1,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    98,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,    -1,   149,   150,     1,    -1,     3,     4,
       5,     6,     7,    -1,    -1,    -1,    11,    12,    -1,    -1,
      -1,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    98,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   141,     1,    -1,     3,
       4,     5,     6,     7,   149,   150,    10,    11,    12,    -1,
      -1,    -1,    16,    -1,    18,    19,    20,    21,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    47,    48,    49,    -1,    51,    52,    53,
      54,    55,    56,    -1,    58,    59,    60,    -1,    -1,    63,
      -1,    -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,
      -1,    95,    96,    -1,    98,    -1,   100,   101,   102,   103,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,    -1,
      -1,    -1,    11,    12,    -1,   149,   150,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    30,    31,    32,    33,    34,    35,    36,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,
      69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,   106,    -1,    -1,
     109,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,    -1,    -1,    -1,    11,    12,   126,   127,   128,
      16,    -1,    18,    19,    20,    21,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      36,   150,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    49,    -1,    51,    52,    53,    54,    55,
      56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,    -1,
      66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,
      96,    -1,    -1,    -1,   100,   101,   102,   103,   104,   105,
     106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,
      11,    12,    -1,   149,   150,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,
      31,    32,    33,    34,    35,    36,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    -1,    58,    59,    60,
      -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,
      -1,    92,    93,    -1,    95,    96,    -1,    -1,    -1,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,   109,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
      -1,    -1,    -1,    11,    12,   126,   127,   128,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    30,    31,    32,    33,    34,    35,    36,   150,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    -1,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,
      -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,
      98,    -1,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,   127,
     128,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,    -1,    -1,    -1,    11,    12,    -1,    -1,    -1,
      16,   149,    18,    19,    20,    21,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    47,    48,    49,    -1,    51,    52,    53,    54,    55,
      56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,    -1,
      66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,
      96,    -1,    98,    -1,   100,   101,   102,   103,   104,   105,
     106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   149,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    -1,    -1,
      -1,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    -1,
      -1,    80,    81,    -1,    -1,    84,    85,    86,    87,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,    -1,   127,   128,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
      -1,    -1,    -1,    11,    12,    -1,   145,   146,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    -1,    26,    -1,
      -1,    -1,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,
      -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,   127,
     128,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,
       7,    -1,   140,    -1,    11,    12,    -1,    -1,   146,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    34,    35,    36,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,
      67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
      -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
     127,   128,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,    -1,   140,    -1,    11,    12,    -1,    -1,   146,
      16,    -1,    18,    19,    20,    21,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    49,    -1,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    -1,    63,    -1,    -1,
      66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    89,    90,    -1,    92,    93,    -1,    95,
      96,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,    -1,    -1,   109,    -1,    -1,    -1,   113,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
     126,   127,   128,    11,    12,    -1,    -1,    -1,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    -1,    -1,    -1,
     146,    -1,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,
      -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,    -1,     7,   126,   127,
     128,    11,    12,    -1,    -1,    -1,    16,    -1,    18,    19,
      20,    21,    22,    23,    24,    -1,    -1,    -1,   146,    -1,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,
      -1,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,   109,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   126,   127,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   146,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,
      -1,    -1,    -1,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    -1,    80,    81,    -1,    -1,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    80,    81,    -1,
      -1,    84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,    -1,
      -1,    -1,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,    -1,   127,   128,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   145,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    -1,    -1,    -1,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      80,    81,    -1,    -1,    84,    85,    86,    87,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
      -1,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,    -1,   127,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   145,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    -1,    -1,    80,    81,    -1,    -1,    84,    85,    86,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
     127,   128,    -1,    -1,     3,     4,     5,    -1,     7,    -1,
      -1,    -1,    11,    12,    -1,    -1,    -1,    16,   145,    18,
      19,    20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    30,    31,    32,    33,    34,    35,    36,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    -1,    58,
      59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,    -1,
      -1,   100,   101,   102,   103,   104,   105,   106,    -1,    -1,
     109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,    -1,     7,    -1,    -1,   126,    11,    12,
      -1,    -1,    -1,    16,    -1,    18,    19,    20,    21,    22,
      23,    24,    -1,    -1,    -1,   144,    -1,    30,    31,    32,
      33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    -1,    58,    59,    60,    -1,    -1,
      63,    -1,    -1,    66,    67,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,
      93,    -1,    95,    96,    -1,    -1,    -1,   100,   101,   102,
     103,   104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,    -1,    -1,   126,    11,    12,    -1,    -1,    -1,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    -1,    -1,
      -1,   144,    -1,    30,    31,    32,    33,    34,    35,    36,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      -1,    58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,
      67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,
      -1,    98,    -1,   100,   101,   102,   103,   104,   105,   106,
      -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,    -1,    -1,    -1,    11,    12,   126,
     127,   128,    16,    -1,    18,    19,    20,    21,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    47,    48,    49,    -1,    51,    52,    53,
      54,    55,    56,    -1,    58,    59,    60,    -1,    -1,    63,
      -1,    -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,
      -1,    95,    96,    -1,    98,    -1,   100,   101,   102,   103,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,
      11,    12,   126,   127,   128,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,
      31,    32,    33,    34,    35,    36,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,
      -1,    92,    93,    -1,    95,    96,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,   109,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
      -1,    -1,    -1,    11,    12,   126,   127,   128,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,
      -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,    -1,     7,    -1,    -1,    -1,    11,    12,   126,   127,
     128,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    98,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,    11,
      12,   126,   127,   128,    16,    -1,    18,    19,    20,    21,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      -1,    63,    -1,    -1,    66,    67,    -1,    69,    70,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,
      92,    93,    -1,    95,    96,    -1,    -1,    99,   100,   101,
     102,   103,   104,   105,   106,    -1,    -1,   109,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,    -1,
      -1,    -1,    11,    12,   126,   127,   128,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    30,    31,    32,    33,    34,    35,    36,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,
      69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,
      -1,   100,   101,   102,   103,   104,   105,   106,    -1,    -1,
     109,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,    -1,    -1,    -1,    11,    12,   126,   127,   128,
      16,    -1,    18,    19,    20,    21,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    49,    -1,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    -1,    63,    -1,    -1,
      66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,
      96,    -1,    -1,    -1,   100,   101,   102,   103,   104,   105,
     106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,    -1,     7,    -1,    -1,    -1,    11,    12,
     126,   127,   128,    16,    -1,    18,    19,    20,    21,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,
      33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    -1,    58,    59,    60,    -1,    -1,
      63,    -1,    -1,    66,    67,    -1,    69,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,
      93,    -1,    95,    96,    -1,    98,    -1,   100,   101,   102,
     103,   104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,
      -1,    11,    12,   126,   127,   128,    16,    -1,    18,    19,
      20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,
      -1,    51,    52,    53,    54,    55,    56,    -1,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,   109,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,
       7,    -1,    -1,    -1,    11,    12,   126,   127,   128,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    -1,    -1,
      -1,    -1,    -1,    30,    31,    32,    33,    34,    35,    36,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      -1,    58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,
      67,    -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,
      -1,    98,    -1,   100,   101,   102,   103,   104,   105,   106,
      -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,     7,    -1,    -1,    -1,    11,    12,   126,
     127,   128,    16,    -1,    18,    19,    20,    21,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    48,    49,    -1,    51,    52,    53,
      54,    55,    56,    -1,    58,    59,    60,    -1,    -1,    63,
      -1,    -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,
      -1,    95,    96,    -1,    98,    -1,   100,   101,   102,   103,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,
      11,    12,   126,   127,   128,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,
      31,    32,    33,    34,    35,    36,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    -1,    58,    59,    60,
      -1,    -1,    63,    -1,    -1,    66,    67,    -1,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,
      -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,   109,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
      -1,    -1,    -1,    11,    12,   126,   127,   128,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    -1,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,
      -1,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,
      -1,    -1,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,   109,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,    -1,     7,    -1,    -1,    -1,    11,    12,   126,   127,
     128,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    -1,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,    11,
      12,   126,   127,   128,    16,    -1,    18,    19,    20,    21,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,    51,
      52,    53,    54,    55,    56,    -1,    58,    59,    60,    -1,
      -1,    63,    -1,    -1,    66,    67,    -1,    69,    70,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,
      92,    93,    -1,    95,    96,    -1,    -1,    -1,   100,   101,
     102,   103,   104,   105,   106,    -1,    -1,   109,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,    -1,
      -1,    -1,    11,    12,   126,   127,   128,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    30,    31,    32,    33,    34,    35,    36,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    -1,    58,
      59,    60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,
      89,    -1,    -1,    92,    93,    -1,    95,    96,    -1,    -1,
      -1,   100,   101,   102,   103,   104,   105,   106,    -1,    -1,
     109,    -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,
      -1,    11,    12,    -1,    -1,    -1,    16,   126,    18,    19,
      20,    21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,
      -1,    51,    52,    53,    54,    55,    56,    -1,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,    66,    67,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      -1,    -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,   109,
      -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,
      11,    12,    -1,    -1,    -1,    16,   126,    18,    19,    20,
      21,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,
      31,    32,    33,    34,    35,    36,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    -1,    58,    59,    60,
      -1,    -1,    63,    -1,    -1,    66,    67,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,
      -1,    92,    93,    -1,    95,    96,    -1,    98,    -1,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,   109,    -1,
      -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,    11,
      12,    -1,    -1,    -1,    16,   126,    18,    19,    20,    21,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,    51,
      52,    53,    54,    55,    56,    -1,    58,    59,    60,    -1,
      -1,    63,    -1,    -1,    66,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,
      92,    93,    -1,    95,    96,    -1,    -1,    -1,   100,   101,
     102,   103,   104,   105,   106,    -1,    -1,   109,    -1,    -1,
       3,     4,     5,    -1,     7,    -1,    -1,    -1,    11,    12,
      -1,    -1,    -1,    16,   126,    18,    19,    20,    21,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,
      33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    -1,    58,    59,    60,    -1,    -1,
      63,    -1,    -1,    66,    67,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,
      93,    -1,    95,    96,    -1,    -1,    -1,   100,   101,   102,
     103,   104,   105,   106,    -1,    -1,   109,    -1,    -1,     3,
       4,     5,    -1,     7,    -1,    -1,    -1,    11,    12,    -1,
      -1,    -1,    16,   126,    18,    19,    20,    21,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    48,    49,    -1,    51,    52,    53,
      54,    55,    56,    -1,    58,    59,    60,    -1,    -1,    63,
      -1,    -1,    66,    67,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,
      -1,    95,    96,    -1,    -1,    -1,   100,   101,   102,   103,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,     3,     4,
       5,    -1,     7,    -1,    -1,    -1,    11,    12,    -1,    -1,
      -1,    16,   126,    18,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    66,    67,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,    -1,
      95,    96,    -1,    -1,    -1,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,   109,    51,    52,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   126,    -1,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    -1,    80,    81,    -1,    -1,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,   127,   128,    51,    52,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,
     146,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,    -1,    80,    81,    -1,    -1,    84,    85,    86,    87,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,   127,
     128,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,   146,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      80,    81,    -1,    -1,    84,    85,    86,    87,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,    -1,   127,   128,    51,
      52,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   145,   146,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    80,    81,
      -1,    -1,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,    -1,   127,   128,    51,    52,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   145,   146,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    80,    81,    -1,    -1,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,    -1,   127,   128,    51,    52,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   145,   146,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    -1,    80,    81,    -1,    -1,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,   127,   128,    51,    52,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,
     146,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,    -1,    80,    81,    -1,    -1,    84,    85,    86,    87,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,   127,
     128,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,   146,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      80,    81,    -1,    -1,    84,    85,    86,    87,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,    -1,   127,   128,    51,
      52,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   145,   146,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    80,    81,
      -1,    -1,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,    -1,   127,   128,    51,    52,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   145,   146,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    80,    81,    -1,    -1,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,    -1,   127,   128,    51,    52,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   145,   146,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    -1,    80,    81,    -1,    -1,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,   127,   128,    51,    52,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,
     146,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,    -1,    80,    81,    -1,    -1,    84,    85,    86,    87,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,   127,
     128,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,   146,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      80,    81,    -1,    -1,    84,    85,    86,    87,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,    -1,   127,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   145
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   152,   153,     0,     1,     3,     4,     5,     6,     7,
      11,    12,    16,    18,    19,    20,    21,    22,    23,    24,
      30,    31,    32,    33,    34,    35,    36,    39,    45,    46,
      47,    48,    49,    51,    52,    53,    54,    55,    56,    58,
      59,    60,    63,    66,    67,    69,    70,    89,    92,    93,
      95,    96,    98,   100,   101,   102,   103,   104,   105,   106,
     109,   126,   127,   128,   154,   155,   156,   161,   163,   164,
     166,   167,   170,   171,   173,   174,   175,   177,   178,   187,
     200,   218,   237,   238,   248,   249,   253,   254,   255,   259,
     260,   261,   263,   264,   265,   266,   267,   268,   291,   304,
     156,    21,    22,    30,    31,    32,    39,    51,    55,    86,
      89,    92,   126,   179,   180,   200,   218,   265,   268,   291,
     180,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    45,    46,    47,    48,    49,
      50,    51,    52,    55,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    80,    81,    84,    85,    86,    87,    98,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     127,   128,   145,   146,   181,   185,   186,   267,   286,    33,
      34,    35,    36,    48,    49,    51,    55,   101,   181,   182,
     183,   261,   201,    89,   164,   165,   178,   218,   265,   266,
     268,   165,   149,   150,   165,   295,   300,   301,   303,   205,
     207,    89,   171,   178,   218,   223,   265,   268,    57,    98,
      99,   127,   170,   187,   188,   193,   196,   198,   289,   290,
     193,   193,   146,   194,   195,   146,   190,   194,   146,   296,
     301,   182,   157,   140,   187,   218,   187,    55,     1,    92,
     159,   160,   161,   172,   173,   304,   164,   203,   189,   198,
     289,   304,   188,   288,   289,   304,    89,   144,   177,   218,
     265,   268,   204,    53,    54,    56,    63,   105,   181,   262,
      62,    64,    65,   111,   250,   251,    63,   250,    63,   250,
      63,   250,    61,   250,    58,    59,   166,   187,   187,   295,
     303,    40,    41,    42,    43,    44,    37,    38,    28,   235,
     113,   144,    92,    98,   174,   113,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    86,
      87,   114,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,    88,   142,   143,    88,   143,   294,    26,   140,
     239,    90,    90,   190,   194,   239,   164,    51,    55,   179,
      58,    59,     1,   117,   269,   300,    88,   142,   143,   214,
     287,   215,   294,   105,   144,   158,   159,    55,    13,   219,
     300,   113,    88,   142,   143,    90,    90,   219,   302,   295,
      17,   242,   149,   165,   165,    55,    88,   142,   143,    25,
     188,   188,   188,    91,   144,   197,   304,   144,   197,   193,
     296,   297,   193,   192,   193,   198,   289,   304,   164,   297,
     164,   162,   140,   159,    88,   143,    90,   161,   172,   147,
     295,   303,   297,   202,   297,   148,   144,   299,   301,   144,
     299,   141,   299,    55,   174,   175,   176,   144,    88,   142,
     143,    51,    53,    54,    55,    56,    92,    98,    99,   120,
     123,   146,   233,   272,   273,   274,   275,   276,   277,   280,
     281,   282,   283,   284,    63,   251,   252,    62,   251,    63,
      63,    63,    61,    71,    71,   156,   165,   165,   165,   165,
     161,   164,   164,   236,    98,   166,   188,   198,   199,   172,
     144,   177,   144,   163,   166,   178,   187,   188,   199,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,    51,    52,    55,   185,   190,
     292,   293,   192,    51,    52,    55,   185,   190,   292,    51,
      55,   292,   241,   240,   166,   187,   166,   187,    97,   168,
     212,   300,   270,   211,    51,    55,   179,   292,   192,   292,
     158,   164,   146,   271,   272,   216,   184,    10,     8,   244,
     304,   159,    13,   187,    51,    55,   192,    51,    55,   159,
     110,   250,   256,   257,   258,   304,   242,   198,    10,    27,
     220,   300,   220,    51,    55,   192,    51,    55,   209,   188,
      98,   188,   196,   289,   290,   297,   147,   297,   144,   144,
     297,   182,   154,   141,   187,   297,   161,   297,   289,   174,
     176,    51,    55,   192,    51,    55,   113,    51,    92,    98,
     224,   225,   226,   274,   272,    29,   108,   234,   144,   285,
     304,   144,   285,    51,   144,   285,    51,    63,   159,   187,
     187,    79,   118,   228,   229,   304,   188,   144,   297,   176,
     144,   113,    44,   296,    90,    90,   190,   194,   296,   298,
      90,    90,   190,   191,   194,   304,   191,   194,   228,   228,
      44,   169,   300,   165,   158,   298,    10,   297,   272,   158,
     300,   181,   182,   188,   199,   245,   304,    15,   222,   304,
      14,   221,   222,    90,    90,   298,    90,    90,   222,   110,
     258,    10,   144,   219,   206,   208,   298,   165,   188,   197,
     289,   141,   299,   298,   188,   226,   144,   274,   144,   297,
     230,   296,   159,   159,   275,   280,   282,   284,   276,   277,
     282,   276,   141,    51,   227,   230,   276,   278,   279,   282,
     284,   159,    98,   188,   176,   187,   115,   166,   187,   166,
     187,   168,   148,    90,   166,   187,   166,   187,   168,   239,
     235,   159,   159,   187,   228,   213,   300,    10,   297,    10,
     217,    91,   246,   304,   159,     9,   247,   304,   165,    10,
      90,    10,   188,   159,   159,   159,   220,   144,   297,   225,
     144,    98,   224,   147,   149,    10,   141,   144,   285,   144,
     285,   144,   285,   144,   285,   285,   113,   230,   118,   144,
     285,   144,   285,   144,   285,    10,   188,   187,   166,   187,
      10,   141,   159,   158,   271,    89,   178,   218,   265,   268,
     219,   159,   219,   222,   242,   243,    10,    10,   210,   144,
     225,   144,   274,    51,   231,   232,   273,   276,   282,   276,
     276,    89,   218,   118,   279,   282,   276,   278,   282,   276,
     141,    10,   158,    55,    88,   142,   143,   159,   159,   159,
     225,   144,   144,   296,   285,   144,   285,   285,   285,    55,
      88,   144,   285,   144,   285,   285,   144,   285,   285,    10,
      51,    55,   192,    51,    55,   244,   221,    10,   225,   232,
     276,    51,    55,   276,   282,   276,   276,   298,   285,   285,
     144,   285,   285,   285,   276,   285
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (p, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, p)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, p); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_state *p)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, p)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_state *p;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (p);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_state *p)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, p)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_state *p;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, p);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, parser_state *p)
#else
static void
yy_reduce_print (yyvsp, yyrule, p)
    YYSTYPE *yyvsp;
    int yyrule;
    parser_state *p;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , p);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, p); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_state *p)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, p)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    parser_state *p;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (p);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YYUSE (yytype);
}




/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (parser_state *p)
#else
int
yyparse (p)
    parser_state *p;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;


#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
static YYSTYPE yyval_default;
# define YY_INITIAL_VALUE(Value) = Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
/* Line 1787 of yacc.c  */
#line 1100 "src/parse.y"
    {
		     p->lstate = EXPR_BEG;
		     if (!p->locals) p->locals = cons(0,0);
		   }
    break;

  case 3:
/* Line 1787 of yacc.c  */
#line 1105 "src/parse.y"
    {
		      p->tree = new_scope(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 4:
/* Line 1787 of yacc.c  */
#line 1111 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 5:
/* Line 1787 of yacc.c  */
#line 1117 "src/parse.y"
    {
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 6:
/* Line 1787 of yacc.c  */
#line 1121 "src/parse.y"
    {
		      (yyval.nd) = new_begin(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 7:
/* Line 1787 of yacc.c  */
#line 1125 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), newline_node((yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 8:
/* Line 1787 of yacc.c  */
#line 1129 "src/parse.y"
    {
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 10:
/* Line 1787 of yacc.c  */
#line 1136 "src/parse.y"
    {
		      (yyval.nd) = local_switch(p);
		    }
    break;

  case 11:
/* Line 1787 of yacc.c  */
#line 1140 "src/parse.y"
    {
		      yyerror(p, "BEGIN not supported");
		      local_resume(p, (yyvsp[(2) - (5)].nd));
		      (yyval.nd) = 0;
		    }
    break;

  case 12:
/* Line 1787 of yacc.c  */
#line 1151 "src/parse.y"
    {
		      if ((yyvsp[(2) - (4)].nd)) {
			(yyval.nd) = new_rescue(p, (yyvsp[(1) - (4)].nd), (yyvsp[(2) - (4)].nd), (yyvsp[(3) - (4)].nd));
		      }
		      else if ((yyvsp[(3) - (4)].nd)) {
			yywarn(p, "else without rescue is useless");
			(yyval.nd) = push((yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].nd));
		      }
		      else {
			(yyval.nd) = (yyvsp[(1) - (4)].nd);
		      }
		      if ((yyvsp[(4) - (4)].nd)) {
			if ((yyval.nd)) {
			  (yyval.nd) = new_ensure(p, (yyval.nd), (yyvsp[(4) - (4)].nd));
			}
			else {
			  (yyval.nd) = push((yyvsp[(4) - (4)].nd), new_nil(p));
			}
		      }
		    }
    break;

  case 13:
/* Line 1787 of yacc.c  */
#line 1174 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 1180 "src/parse.y"
    {
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 1184 "src/parse.y"
    {
		      (yyval.nd) = new_begin(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 16:
/* Line 1787 of yacc.c  */
#line 1188 "src/parse.y"
    {
			(yyval.nd) = push((yyvsp[(1) - (3)].nd), newline_node((yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 1192 "src/parse.y"
    {
		      (yyval.nd) = new_begin(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 1197 "src/parse.y"
    {p->lstate = EXPR_FNAME;}
    break;

  case 19:
/* Line 1787 of yacc.c  */
#line 1198 "src/parse.y"
    {
		      (yyval.nd) = new_alias(p, (yyvsp[(2) - (4)].id), (yyvsp[(4) - (4)].id));
		    }
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 1202 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 21:
/* Line 1787 of yacc.c  */
#line 1206 "src/parse.y"
    {
			(yyval.nd) = new_if(p, cond((yyvsp[(3) - (3)].nd)), (yyvsp[(1) - (3)].nd), 0);
		    }
    break;

  case 22:
/* Line 1787 of yacc.c  */
#line 1210 "src/parse.y"
    {
		      (yyval.nd) = new_unless(p, cond((yyvsp[(3) - (3)].nd)), (yyvsp[(1) - (3)].nd), 0);
		    }
    break;

  case 23:
/* Line 1787 of yacc.c  */
#line 1214 "src/parse.y"
    {
		      (yyval.nd) = new_while(p, cond((yyvsp[(3) - (3)].nd)), (yyvsp[(1) - (3)].nd));
		    }
    break;

  case 24:
/* Line 1787 of yacc.c  */
#line 1218 "src/parse.y"
    {
		      (yyval.nd) = new_until(p, cond((yyvsp[(3) - (3)].nd)), (yyvsp[(1) - (3)].nd));
		    }
    break;

  case 25:
/* Line 1787 of yacc.c  */
#line 1222 "src/parse.y"
    {
		      (yyval.nd) = new_rescue(p, (yyvsp[(1) - (3)].nd), list1(list3(0, 0, (yyvsp[(3) - (3)].nd))), 0);
		    }
    break;

  case 26:
/* Line 1787 of yacc.c  */
#line 1226 "src/parse.y"
    {
		      yyerror(p, "END not suported");
		      (yyval.nd) = new_postexe(p, (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 1232 "src/parse.y"
    {
		      (yyval.nd) = new_masgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 29:
/* Line 1787 of yacc.c  */
#line 1236 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(2) - (3)].id), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 30:
/* Line 1787 of yacc.c  */
#line 1240 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (6)].nd), intern2("[]",2), (yyvsp[(3) - (6)].nd)), (yyvsp[(5) - (6)].id), (yyvsp[(6) - (6)].nd));
		    }
    break;

  case 31:
/* Line 1787 of yacc.c  */
#line 1244 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), 0), (yyvsp[(4) - (5)].id), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 32:
/* Line 1787 of yacc.c  */
#line 1248 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), 0), (yyvsp[(4) - (5)].id), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 33:
/* Line 1787 of yacc.c  */
#line 1252 "src/parse.y"
    {
		      yyerror(p, "constant re-assignment");
		      (yyval.nd) = 0;
		    }
    break;

  case 34:
/* Line 1787 of yacc.c  */
#line 1257 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), 0), (yyvsp[(4) - (5)].id), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 35:
/* Line 1787 of yacc.c  */
#line 1261 "src/parse.y"
    {
		      backref_error(p, (yyvsp[(1) - (3)].nd));
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 1266 "src/parse.y"
    {
		      (yyval.nd) = new_asgn(p, (yyvsp[(1) - (3)].nd), new_array(p, (yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 37:
/* Line 1787 of yacc.c  */
#line 1270 "src/parse.y"
    {
		      (yyval.nd) = new_masgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 1274 "src/parse.y"
    {
		      (yyval.nd) = new_masgn(p, (yyvsp[(1) - (3)].nd), new_array(p, (yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 40:
/* Line 1787 of yacc.c  */
#line 1281 "src/parse.y"
    {
		      (yyval.nd) = new_asgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 41:
/* Line 1787 of yacc.c  */
#line 1285 "src/parse.y"
    {
		      (yyval.nd) = new_asgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 43:
/* Line 1787 of yacc.c  */
#line 1293 "src/parse.y"
    {
		      (yyval.nd) = new_and(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 44:
/* Line 1787 of yacc.c  */
#line 1297 "src/parse.y"
    {
		      (yyval.nd) = new_or(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 1301 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, cond((yyvsp[(3) - (3)].nd)), "!");
		    }
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 1305 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, cond((yyvsp[(2) - (2)].nd)), "!");
		    }
    break;

  case 48:
/* Line 1787 of yacc.c  */
#line 1312 "src/parse.y"
    {
		      if (!(yyvsp[(1) - (1)].nd)) (yyval.nd) = new_nil(p);
		      else (yyval.nd) = (yyvsp[(1) - (1)].nd);
		    }
    break;

  case 53:
/* Line 1787 of yacc.c  */
#line 1327 "src/parse.y"
    {
		      local_nest(p);
		    }
    break;

  case 54:
/* Line 1787 of yacc.c  */
#line 1333 "src/parse.y"
    {
		      (yyval.nd) = new_block(p, (yyvsp[(3) - (5)].nd), (yyvsp[(4) - (5)].nd));
		      local_unnest(p);
		    }
    break;

  case 55:
/* Line 1787 of yacc.c  */
#line 1340 "src/parse.y"
    {
		      (yyval.nd) = new_fcall(p, (yyvsp[(1) - (2)].id), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 56:
/* Line 1787 of yacc.c  */
#line 1344 "src/parse.y"
    {
		      args_with_block(p, (yyvsp[(2) - (3)].nd), (yyvsp[(3) - (3)].nd));
		      (yyval.nd) = new_fcall(p, (yyvsp[(1) - (3)].id), (yyvsp[(2) - (3)].nd));
		    }
    break;

  case 57:
/* Line 1787 of yacc.c  */
#line 1349 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 58:
/* Line 1787 of yacc.c  */
#line 1353 "src/parse.y"
    {
		      args_with_block(p, (yyvsp[(4) - (5)].nd), (yyvsp[(5) - (5)].nd));
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), (yyvsp[(4) - (5)].nd));
		   }
    break;

  case 59:
/* Line 1787 of yacc.c  */
#line 1358 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 60:
/* Line 1787 of yacc.c  */
#line 1362 "src/parse.y"
    {
		      args_with_block(p, (yyvsp[(4) - (5)].nd), (yyvsp[(5) - (5)].nd));
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), (yyvsp[(4) - (5)].nd));
		    }
    break;

  case 61:
/* Line 1787 of yacc.c  */
#line 1367 "src/parse.y"
    {
		      (yyval.nd) = new_super(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 62:
/* Line 1787 of yacc.c  */
#line 1371 "src/parse.y"
    {
		      (yyval.nd) = new_yield(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 63:
/* Line 1787 of yacc.c  */
#line 1375 "src/parse.y"
    {
		      (yyval.nd) = new_return(p, ret_args(p, (yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 64:
/* Line 1787 of yacc.c  */
#line 1379 "src/parse.y"
    {
		      (yyval.nd) = new_break(p, ret_args(p, (yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 65:
/* Line 1787 of yacc.c  */
#line 1383 "src/parse.y"
    {
		      (yyval.nd) = new_next(p, ret_args(p, (yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 66:
/* Line 1787 of yacc.c  */
#line 1389 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		    }
    break;

  case 67:
/* Line 1787 of yacc.c  */
#line 1393 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		    }
    break;

  case 69:
/* Line 1787 of yacc.c  */
#line 1400 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(2) - (3)].nd));
		    }
    break;

  case 70:
/* Line 1787 of yacc.c  */
#line 1406 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 71:
/* Line 1787 of yacc.c  */
#line 1410 "src/parse.y"
    {
		      (yyval.nd) = list1(push((yyvsp[(1) - (2)].nd),(yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 72:
/* Line 1787 of yacc.c  */
#line 1414 "src/parse.y"
    {
		      (yyval.nd) = list2((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 73:
/* Line 1787 of yacc.c  */
#line 1418 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].nd), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 74:
/* Line 1787 of yacc.c  */
#line 1422 "src/parse.y"
    {
		      (yyval.nd) = list2((yyvsp[(1) - (2)].nd), new_nil(p));
		    }
    break;

  case 75:
/* Line 1787 of yacc.c  */
#line 1426 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (4)].nd), new_nil(p), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 76:
/* Line 1787 of yacc.c  */
#line 1430 "src/parse.y"
    {
		      (yyval.nd) = list2(0, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 77:
/* Line 1787 of yacc.c  */
#line 1434 "src/parse.y"
    {
		      (yyval.nd) = list3(0, (yyvsp[(2) - (4)].nd), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 78:
/* Line 1787 of yacc.c  */
#line 1438 "src/parse.y"
    {
		      (yyval.nd) = list2(0, new_nil(p));
		    }
    break;

  case 79:
/* Line 1787 of yacc.c  */
#line 1442 "src/parse.y"
    {
		      (yyval.nd) = list3(0, new_nil(p), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 81:
/* Line 1787 of yacc.c  */
#line 1449 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		    }
    break;

  case 82:
/* Line 1787 of yacc.c  */
#line 1455 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (2)].nd));
		    }
    break;

  case 83:
/* Line 1787 of yacc.c  */
#line 1459 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(2) - (3)].nd));
		    }
    break;

  case 84:
/* Line 1787 of yacc.c  */
#line 1465 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 85:
/* Line 1787 of yacc.c  */
#line 1469 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (2)].nd), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 86:
/* Line 1787 of yacc.c  */
#line 1475 "src/parse.y"
    {
		      assignable(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 87:
/* Line 1787 of yacc.c  */
#line 1479 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), intern2("[]",2), (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 88:
/* Line 1787 of yacc.c  */
#line 1483 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 89:
/* Line 1787 of yacc.c  */
#line 1487 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 90:
/* Line 1787 of yacc.c  */
#line 1491 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 91:
/* Line 1787 of yacc.c  */
#line 1495 "src/parse.y"
    {
		      if (p->in_def || p->in_single)
			yyerror(p, "dynamic constant assignment");
		      (yyval.nd) = new_colon2(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id));
		    }
    break;

  case 92:
/* Line 1787 of yacc.c  */
#line 1501 "src/parse.y"
    {
		      if (p->in_def || p->in_single)
			yyerror(p, "dynamic constant assignment");
		      (yyval.nd) = new_colon3(p, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 93:
/* Line 1787 of yacc.c  */
#line 1507 "src/parse.y"
    {
		      backref_error(p, (yyvsp[(1) - (1)].nd));
		      (yyval.nd) = 0;
		    }
    break;

  case 94:
/* Line 1787 of yacc.c  */
#line 1514 "src/parse.y"
    {
		      assignable(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 95:
/* Line 1787 of yacc.c  */
#line 1518 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), intern2("[]",2), (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 96:
/* Line 1787 of yacc.c  */
#line 1522 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 97:
/* Line 1787 of yacc.c  */
#line 1526 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 98:
/* Line 1787 of yacc.c  */
#line 1530 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 99:
/* Line 1787 of yacc.c  */
#line 1534 "src/parse.y"
    {
		      if (p->in_def || p->in_single)
			yyerror(p, "dynamic constant assignment");
		      (yyval.nd) = new_colon2(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id));
		    }
    break;

  case 100:
/* Line 1787 of yacc.c  */
#line 1540 "src/parse.y"
    {
		      if (p->in_def || p->in_single)
			yyerror(p, "dynamic constant assignment");
		      (yyval.nd) = new_colon3(p, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 101:
/* Line 1787 of yacc.c  */
#line 1546 "src/parse.y"
    {
		      backref_error(p, (yyvsp[(1) - (1)].nd));
		      (yyval.nd) = 0;
		    }
    break;

  case 102:
/* Line 1787 of yacc.c  */
#line 1553 "src/parse.y"
    {
		      yyerror(p, "class/module name must be CONSTANT");
		    }
    break;

  case 104:
/* Line 1787 of yacc.c  */
#line 1560 "src/parse.y"
    {
		      (yyval.nd) = cons((node*)1, nsym((yyvsp[(2) - (2)].id)));
		    }
    break;

  case 105:
/* Line 1787 of yacc.c  */
#line 1564 "src/parse.y"
    {
		      (yyval.nd) = cons((node*)0, nsym((yyvsp[(1) - (1)].id)));
		    }
    break;

  case 106:
/* Line 1787 of yacc.c  */
#line 1568 "src/parse.y"
    {
		      (yyval.nd) = cons((yyvsp[(1) - (3)].nd), nsym((yyvsp[(3) - (3)].id)));
		    }
    break;

  case 110:
/* Line 1787 of yacc.c  */
#line 1577 "src/parse.y"
    {
		      p->lstate = EXPR_ENDFN;
		      (yyval.id) = (yyvsp[(1) - (1)].id);
		    }
    break;

  case 111:
/* Line 1787 of yacc.c  */
#line 1582 "src/parse.y"
    {
		      p->lstate = EXPR_ENDFN;
		      (yyval.id) = (yyvsp[(1) - (1)].id);
		    }
    break;

  case 114:
/* Line 1787 of yacc.c  */
#line 1593 "src/parse.y"
    {
		      (yyval.nd) = new_undef(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 115:
/* Line 1787 of yacc.c  */
#line 1596 "src/parse.y"
    {p->lstate = EXPR_FNAME;}
    break;

  case 116:
/* Line 1787 of yacc.c  */
#line 1597 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (4)].nd), nsym((yyvsp[(4) - (4)].id)));
		    }
    break;

  case 117:
/* Line 1787 of yacc.c  */
#line 1602 "src/parse.y"
    { (yyval.id) = intern_c('|'); }
    break;

  case 118:
/* Line 1787 of yacc.c  */
#line 1603 "src/parse.y"
    { (yyval.id) = intern_c('^'); }
    break;

  case 119:
/* Line 1787 of yacc.c  */
#line 1604 "src/parse.y"
    { (yyval.id) = intern_c('&'); }
    break;

  case 120:
/* Line 1787 of yacc.c  */
#line 1605 "src/parse.y"
    { (yyval.id) = intern2("<=>",3); }
    break;

  case 121:
/* Line 1787 of yacc.c  */
#line 1606 "src/parse.y"
    { (yyval.id) = intern2("==",2); }
    break;

  case 122:
/* Line 1787 of yacc.c  */
#line 1607 "src/parse.y"
    { (yyval.id) = intern2("===",3); }
    break;

  case 123:
/* Line 1787 of yacc.c  */
#line 1608 "src/parse.y"
    { (yyval.id) = intern2("=~",2); }
    break;

  case 124:
/* Line 1787 of yacc.c  */
#line 1609 "src/parse.y"
    { (yyval.id) = intern2("!~",2); }
    break;

  case 125:
/* Line 1787 of yacc.c  */
#line 1610 "src/parse.y"
    { (yyval.id) = intern_c('>'); }
    break;

  case 126:
/* Line 1787 of yacc.c  */
#line 1611 "src/parse.y"
    { (yyval.id) = intern2(">=",2); }
    break;

  case 127:
/* Line 1787 of yacc.c  */
#line 1612 "src/parse.y"
    { (yyval.id) = intern_c('<'); }
    break;

  case 128:
/* Line 1787 of yacc.c  */
#line 1613 "src/parse.y"
    { (yyval.id) = intern2("<=",2); }
    break;

  case 129:
/* Line 1787 of yacc.c  */
#line 1614 "src/parse.y"
    { (yyval.id) = intern2("!=",2); }
    break;

  case 130:
/* Line 1787 of yacc.c  */
#line 1615 "src/parse.y"
    { (yyval.id) = intern2("<<",2); }
    break;

  case 131:
/* Line 1787 of yacc.c  */
#line 1616 "src/parse.y"
    { (yyval.id) = intern2(">>",2); }
    break;

  case 132:
/* Line 1787 of yacc.c  */
#line 1617 "src/parse.y"
    { (yyval.id) = intern_c('+'); }
    break;

  case 133:
/* Line 1787 of yacc.c  */
#line 1618 "src/parse.y"
    { (yyval.id) = intern_c('-'); }
    break;

  case 134:
/* Line 1787 of yacc.c  */
#line 1619 "src/parse.y"
    { (yyval.id) = intern_c('*'); }
    break;

  case 135:
/* Line 1787 of yacc.c  */
#line 1620 "src/parse.y"
    { (yyval.id) = intern_c('*'); }
    break;

  case 136:
/* Line 1787 of yacc.c  */
#line 1621 "src/parse.y"
    { (yyval.id) = intern_c('/'); }
    break;

  case 137:
/* Line 1787 of yacc.c  */
#line 1622 "src/parse.y"
    { (yyval.id) = intern_c('%'); }
    break;

  case 138:
/* Line 1787 of yacc.c  */
#line 1623 "src/parse.y"
    { (yyval.id) = intern2("**",2); }
    break;

  case 139:
/* Line 1787 of yacc.c  */
#line 1624 "src/parse.y"
    { (yyval.id) = intern_c('!'); }
    break;

  case 140:
/* Line 1787 of yacc.c  */
#line 1625 "src/parse.y"
    { (yyval.id) = intern_c('~'); }
    break;

  case 141:
/* Line 1787 of yacc.c  */
#line 1626 "src/parse.y"
    { (yyval.id) = intern2("+@",2); }
    break;

  case 142:
/* Line 1787 of yacc.c  */
#line 1627 "src/parse.y"
    { (yyval.id) = intern2("-@",2); }
    break;

  case 143:
/* Line 1787 of yacc.c  */
#line 1628 "src/parse.y"
    { (yyval.id) = intern2("[]",2); }
    break;

  case 144:
/* Line 1787 of yacc.c  */
#line 1629 "src/parse.y"
    { (yyval.id) = intern2("[]=",3); }
    break;

  case 145:
/* Line 1787 of yacc.c  */
#line 1630 "src/parse.y"
    { (yyval.id) = intern_c('`'); }
    break;

  case 186:
/* Line 1787 of yacc.c  */
#line 1648 "src/parse.y"
    {
		      (yyval.nd) = new_asgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 187:
/* Line 1787 of yacc.c  */
#line 1652 "src/parse.y"
    {
		      (yyval.nd) = new_asgn(p, (yyvsp[(1) - (5)].nd), new_rescue(p, (yyvsp[(3) - (5)].nd), list1(list3(0, 0, (yyvsp[(5) - (5)].nd))), 0));
		    }
    break;

  case 188:
/* Line 1787 of yacc.c  */
#line 1656 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, (yyvsp[(1) - (3)].nd), (yyvsp[(2) - (3)].id), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 189:
/* Line 1787 of yacc.c  */
#line 1660 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, (yyvsp[(1) - (5)].nd), (yyvsp[(2) - (5)].id), new_rescue(p, (yyvsp[(3) - (5)].nd), list1(list3(0, 0, (yyvsp[(5) - (5)].nd))), 0));
		    }
    break;

  case 190:
/* Line 1787 of yacc.c  */
#line 1664 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (6)].nd), intern2("[]",2), (yyvsp[(3) - (6)].nd)), (yyvsp[(5) - (6)].id), (yyvsp[(6) - (6)].nd));
		    }
    break;

  case 191:
/* Line 1787 of yacc.c  */
#line 1668 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), 0), (yyvsp[(4) - (5)].id), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 192:
/* Line 1787 of yacc.c  */
#line 1672 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), 0), (yyvsp[(4) - (5)].id), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 193:
/* Line 1787 of yacc.c  */
#line 1676 "src/parse.y"
    {
		      (yyval.nd) = new_op_asgn(p, new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), 0), (yyvsp[(4) - (5)].id), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 194:
/* Line 1787 of yacc.c  */
#line 1680 "src/parse.y"
    {
		      yyerror(p, "constant re-assignment");
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 195:
/* Line 1787 of yacc.c  */
#line 1685 "src/parse.y"
    {
		      yyerror(p, "constant re-assignment");
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 196:
/* Line 1787 of yacc.c  */
#line 1690 "src/parse.y"
    {
		      backref_error(p, (yyvsp[(1) - (3)].nd));
		      (yyval.nd) = new_begin(p, 0);
		    }
    break;

  case 197:
/* Line 1787 of yacc.c  */
#line 1695 "src/parse.y"
    {
		      (yyval.nd) = new_dot2(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 198:
/* Line 1787 of yacc.c  */
#line 1699 "src/parse.y"
    {
		      (yyval.nd) = new_dot3(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 199:
/* Line 1787 of yacc.c  */
#line 1703 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "+", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 200:
/* Line 1787 of yacc.c  */
#line 1707 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "-", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 201:
/* Line 1787 of yacc.c  */
#line 1711 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "*", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 202:
/* Line 1787 of yacc.c  */
#line 1715 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "/", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 203:
/* Line 1787 of yacc.c  */
#line 1719 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "%", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 204:
/* Line 1787 of yacc.c  */
#line 1723 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "**", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 205:
/* Line 1787 of yacc.c  */
#line 1727 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, call_bin_op(p, (yyvsp[(2) - (4)].nd), "**", (yyvsp[(4) - (4)].nd)), "-@");
		    }
    break;

  case 206:
/* Line 1787 of yacc.c  */
#line 1731 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, call_bin_op(p, (yyvsp[(2) - (4)].nd), "**", (yyvsp[(4) - (4)].nd)), "-@");
		    }
    break;

  case 207:
/* Line 1787 of yacc.c  */
#line 1735 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, (yyvsp[(2) - (2)].nd), "+@");
		    }
    break;

  case 208:
/* Line 1787 of yacc.c  */
#line 1739 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, (yyvsp[(2) - (2)].nd), "-@");
		    }
    break;

  case 209:
/* Line 1787 of yacc.c  */
#line 1743 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "|", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 210:
/* Line 1787 of yacc.c  */
#line 1747 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "^", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 211:
/* Line 1787 of yacc.c  */
#line 1751 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "&", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 212:
/* Line 1787 of yacc.c  */
#line 1755 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "<=>", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 213:
/* Line 1787 of yacc.c  */
#line 1759 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), ">", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 214:
/* Line 1787 of yacc.c  */
#line 1763 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), ">=", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 215:
/* Line 1787 of yacc.c  */
#line 1767 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "<", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 216:
/* Line 1787 of yacc.c  */
#line 1771 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "<=", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 217:
/* Line 1787 of yacc.c  */
#line 1775 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "==", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 218:
/* Line 1787 of yacc.c  */
#line 1779 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "===", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 219:
/* Line 1787 of yacc.c  */
#line 1783 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "!=", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 220:
/* Line 1787 of yacc.c  */
#line 1787 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "=~", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 221:
/* Line 1787 of yacc.c  */
#line 1791 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "!~", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 222:
/* Line 1787 of yacc.c  */
#line 1795 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, cond((yyvsp[(2) - (2)].nd)), "!");
		    }
    break;

  case 223:
/* Line 1787 of yacc.c  */
#line 1799 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, cond((yyvsp[(2) - (2)].nd)), "~");
		    }
    break;

  case 224:
/* Line 1787 of yacc.c  */
#line 1803 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), "<<", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 225:
/* Line 1787 of yacc.c  */
#line 1807 "src/parse.y"
    {
		      (yyval.nd) = call_bin_op(p, (yyvsp[(1) - (3)].nd), ">>", (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 226:
/* Line 1787 of yacc.c  */
#line 1811 "src/parse.y"
    {
		      (yyval.nd) = new_and(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 227:
/* Line 1787 of yacc.c  */
#line 1815 "src/parse.y"
    {
		      (yyval.nd) = new_or(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 228:
/* Line 1787 of yacc.c  */
#line 1819 "src/parse.y"
    {
		      (yyval.nd) = new_if(p, cond((yyvsp[(1) - (6)].nd)), (yyvsp[(3) - (6)].nd), (yyvsp[(6) - (6)].nd));
		    }
    break;

  case 229:
/* Line 1787 of yacc.c  */
#line 1823 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		    }
    break;

  case 230:
/* Line 1787 of yacc.c  */
#line 1829 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		      if (!(yyval.nd)) (yyval.nd) = new_nil(p);
		    }
    break;

  case 232:
/* Line 1787 of yacc.c  */
#line 1837 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 233:
/* Line 1787 of yacc.c  */
#line 1841 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (4)].nd), new_hash(p, (yyvsp[(3) - (4)].nd)));
		    }
    break;

  case 234:
/* Line 1787 of yacc.c  */
#line 1845 "src/parse.y"
    {
		      (yyval.nd) = cons(new_hash(p, (yyvsp[(1) - (2)].nd)), 0);
		    }
    break;

  case 235:
/* Line 1787 of yacc.c  */
#line 1851 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		    }
    break;

  case 240:
/* Line 1787 of yacc.c  */
#line 1863 "src/parse.y"
    {
		      (yyval.nd) = cons((yyvsp[(1) - (2)].nd),0);
		    }
    break;

  case 241:
/* Line 1787 of yacc.c  */
#line 1867 "src/parse.y"
    {
		      (yyval.nd) = cons(push((yyvsp[(1) - (4)].nd), new_hash(p, (yyvsp[(3) - (4)].nd))), 0);
		    }
    break;

  case 242:
/* Line 1787 of yacc.c  */
#line 1871 "src/parse.y"
    {
		      (yyval.nd) = cons(list1(new_hash(p, (yyvsp[(1) - (2)].nd))), 0);
		    }
    break;

  case 243:
/* Line 1787 of yacc.c  */
#line 1877 "src/parse.y"
    {
		      (yyval.nd) = cons(list1((yyvsp[(1) - (1)].nd)), 0);
		    }
    break;

  case 244:
/* Line 1787 of yacc.c  */
#line 1881 "src/parse.y"
    {
		      (yyval.nd) = cons((yyvsp[(1) - (2)].nd), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 245:
/* Line 1787 of yacc.c  */
#line 1885 "src/parse.y"
    {
		      (yyval.nd) = cons(list1(new_hash(p, (yyvsp[(1) - (2)].nd))), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 246:
/* Line 1787 of yacc.c  */
#line 1889 "src/parse.y"
    {
		      (yyval.nd) = cons(push((yyvsp[(1) - (4)].nd), new_hash(p, (yyvsp[(3) - (4)].nd))), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 247:
/* Line 1787 of yacc.c  */
#line 1893 "src/parse.y"
    {
		      (yyval.nd) = cons(0, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 248:
/* Line 1787 of yacc.c  */
#line 1898 "src/parse.y"
    {
		      (yyval.stack) = p->cmdarg_stack;
		      CMDARG_PUSH(1);
		    }
    break;

  case 249:
/* Line 1787 of yacc.c  */
#line 1903 "src/parse.y"
    {
		      p->cmdarg_stack = (yyvsp[(1) - (2)].stack);
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 250:
/* Line 1787 of yacc.c  */
#line 1910 "src/parse.y"
    {
		      (yyval.nd) = new_block_arg(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 251:
/* Line 1787 of yacc.c  */
#line 1916 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 252:
/* Line 1787 of yacc.c  */
#line 1920 "src/parse.y"
    {
		      (yyval.nd) = 0;
		    }
    break;

  case 253:
/* Line 1787 of yacc.c  */
#line 1926 "src/parse.y"
    {
		      (yyval.nd) = cons((yyvsp[(1) - (1)].nd), 0);
		    }
    break;

  case 254:
/* Line 1787 of yacc.c  */
#line 1930 "src/parse.y"
    {
		      (yyval.nd) = cons(new_splat(p, (yyvsp[(2) - (2)].nd)), 0);
		    }
    break;

  case 255:
/* Line 1787 of yacc.c  */
#line 1934 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 256:
/* Line 1787 of yacc.c  */
#line 1938 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (4)].nd), new_splat(p, (yyvsp[(4) - (4)].nd)));
		    }
    break;

  case 257:
/* Line 1787 of yacc.c  */
#line 1944 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 258:
/* Line 1787 of yacc.c  */
#line 1948 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (4)].nd), new_splat(p, (yyvsp[(4) - (4)].nd)));
		    }
    break;

  case 259:
/* Line 1787 of yacc.c  */
#line 1952 "src/parse.y"
    {
		      (yyval.nd) = list1(new_splat(p, (yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 267:
/* Line 1787 of yacc.c  */
#line 1965 "src/parse.y"
    {
		      (yyval.nd) = new_fcall(p, (yyvsp[(1) - (1)].id), 0);
		    }
    break;

  case 268:
/* Line 1787 of yacc.c  */
#line 1969 "src/parse.y"
    {
		      (yyvsp[(1) - (1)].stack) = p->cmdarg_stack;
		      p->cmdarg_stack = 0;
		    }
    break;

  case 269:
/* Line 1787 of yacc.c  */
#line 1975 "src/parse.y"
    {
		      p->cmdarg_stack = (yyvsp[(1) - (4)].stack);
		      (yyval.nd) = (yyvsp[(3) - (4)].nd);
		    }
    break;

  case 270:
/* Line 1787 of yacc.c  */
#line 1979 "src/parse.y"
    {p->lstate = EXPR_ENDARG;}
    break;

  case 271:
/* Line 1787 of yacc.c  */
#line 1980 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (4)].nd);
		    }
    break;

  case 272:
/* Line 1787 of yacc.c  */
#line 1983 "src/parse.y"
    {p->lstate = EXPR_ENDARG;}
    break;

  case 273:
/* Line 1787 of yacc.c  */
#line 1984 "src/parse.y"
    {
		      (yyval.nd) = 0;
		    }
    break;

  case 274:
/* Line 1787 of yacc.c  */
#line 1988 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		    }
    break;

  case 275:
/* Line 1787 of yacc.c  */
#line 1992 "src/parse.y"
    {
		      (yyval.nd) = new_colon2(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id));
		    }
    break;

  case 276:
/* Line 1787 of yacc.c  */
#line 1996 "src/parse.y"
    {
		      (yyval.nd) = new_colon3(p, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 277:
/* Line 1787 of yacc.c  */
#line 2000 "src/parse.y"
    {
		      (yyval.nd) = new_array(p, (yyvsp[(2) - (3)].nd));
		    }
    break;

  case 278:
/* Line 1787 of yacc.c  */
#line 2004 "src/parse.y"
    {
		      (yyval.nd) = new_hash(p, (yyvsp[(2) - (3)].nd));
		    }
    break;

  case 279:
/* Line 1787 of yacc.c  */
#line 2008 "src/parse.y"
    {
		      (yyval.nd) = new_return(p, 0);
		    }
    break;

  case 280:
/* Line 1787 of yacc.c  */
#line 2012 "src/parse.y"
    {
		      (yyval.nd) = new_yield(p, (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 281:
/* Line 1787 of yacc.c  */
#line 2016 "src/parse.y"
    {
		      (yyval.nd) = new_yield(p, 0);
		    }
    break;

  case 282:
/* Line 1787 of yacc.c  */
#line 2020 "src/parse.y"
    {
		      (yyval.nd) = new_yield(p, 0);
		    }
    break;

  case 283:
/* Line 1787 of yacc.c  */
#line 2024 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, cond((yyvsp[(3) - (4)].nd)), "!");
		    }
    break;

  case 284:
/* Line 1787 of yacc.c  */
#line 2028 "src/parse.y"
    {
		      (yyval.nd) = call_uni_op(p, new_nil(p), "!");
		    }
    break;

  case 285:
/* Line 1787 of yacc.c  */
#line 2032 "src/parse.y"
    {
		      (yyval.nd) = new_fcall(p, (yyvsp[(1) - (2)].id), cons(0, (yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 287:
/* Line 1787 of yacc.c  */
#line 2037 "src/parse.y"
    {
		      call_with_block(p, (yyvsp[(1) - (2)].nd), (yyvsp[(2) - (2)].nd));
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 288:
/* Line 1787 of yacc.c  */
#line 2042 "src/parse.y"
    {
		      local_nest(p);
		      (yyval.num) = p->lpar_beg;
		      p->lpar_beg = ++p->paren_nest;
		    }
    break;

  case 289:
/* Line 1787 of yacc.c  */
#line 2049 "src/parse.y"
    {
		      p->lpar_beg = (yyvsp[(2) - (4)].num);
		      (yyval.nd) = new_lambda(p, (yyvsp[(3) - (4)].nd), (yyvsp[(4) - (4)].nd));
		      local_unnest(p);
		    }
    break;

  case 290:
/* Line 1787 of yacc.c  */
#line 2058 "src/parse.y"
    {
		      (yyval.nd) = new_if(p, cond((yyvsp[(2) - (6)].nd)), (yyvsp[(4) - (6)].nd), (yyvsp[(5) - (6)].nd));
		    }
    break;

  case 291:
/* Line 1787 of yacc.c  */
#line 2065 "src/parse.y"
    {
		      (yyval.nd) = new_unless(p, cond((yyvsp[(2) - (6)].nd)), (yyvsp[(4) - (6)].nd), (yyvsp[(5) - (6)].nd));
		    }
    break;

  case 292:
/* Line 1787 of yacc.c  */
#line 2068 "src/parse.y"
    {COND_PUSH(1);}
    break;

  case 293:
/* Line 1787 of yacc.c  */
#line 2068 "src/parse.y"
    {COND_POP();}
    break;

  case 294:
/* Line 1787 of yacc.c  */
#line 2071 "src/parse.y"
    {
		      (yyval.nd) = new_while(p, cond((yyvsp[(3) - (7)].nd)), (yyvsp[(6) - (7)].nd));
		    }
    break;

  case 295:
/* Line 1787 of yacc.c  */
#line 2074 "src/parse.y"
    {COND_PUSH(1);}
    break;

  case 296:
/* Line 1787 of yacc.c  */
#line 2074 "src/parse.y"
    {COND_POP();}
    break;

  case 297:
/* Line 1787 of yacc.c  */
#line 2077 "src/parse.y"
    {
		      (yyval.nd) = new_until(p, cond((yyvsp[(3) - (7)].nd)), (yyvsp[(6) - (7)].nd));
		    }
    break;

  case 298:
/* Line 1787 of yacc.c  */
#line 2083 "src/parse.y"
    {
		      (yyval.nd) = new_case(p, (yyvsp[(2) - (5)].nd), (yyvsp[(4) - (5)].nd));
		    }
    break;

  case 299:
/* Line 1787 of yacc.c  */
#line 2087 "src/parse.y"
    {
		      (yyval.nd) = new_case(p, 0, (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 300:
/* Line 1787 of yacc.c  */
#line 2091 "src/parse.y"
    {COND_PUSH(1);}
    break;

  case 301:
/* Line 1787 of yacc.c  */
#line 2093 "src/parse.y"
    {COND_POP();}
    break;

  case 302:
/* Line 1787 of yacc.c  */
#line 2096 "src/parse.y"
    {
		      (yyval.nd) = new_for(p, (yyvsp[(2) - (9)].nd), (yyvsp[(5) - (9)].nd), (yyvsp[(8) - (9)].nd));
		    }
    break;

  case 303:
/* Line 1787 of yacc.c  */
#line 2100 "src/parse.y"
    {
		      if (p->in_def || p->in_single)
			yyerror(p, "class definition in method body");
		      (yyval.nd) = local_switch(p);
		    }
    break;

  case 304:
/* Line 1787 of yacc.c  */
#line 2107 "src/parse.y"
    {
		      (yyval.nd) = new_class(p, (yyvsp[(2) - (6)].nd), (yyvsp[(3) - (6)].nd), (yyvsp[(5) - (6)].nd));
		      local_resume(p, (yyvsp[(4) - (6)].nd));
		    }
    break;

  case 305:
/* Line 1787 of yacc.c  */
#line 2112 "src/parse.y"
    {
		      (yyval.num) = p->in_def;
		      p->in_def = 0;
		    }
    break;

  case 306:
/* Line 1787 of yacc.c  */
#line 2117 "src/parse.y"
    {
		      (yyval.nd) = cons(local_switch(p), (node*)(intptr_t)p->in_single);
		      p->in_single = 0;
		    }
    break;

  case 307:
/* Line 1787 of yacc.c  */
#line 2123 "src/parse.y"
    {
		      (yyval.nd) = new_sclass(p, (yyvsp[(3) - (8)].nd), (yyvsp[(7) - (8)].nd));
		      local_resume(p, (yyvsp[(6) - (8)].nd)->car);
		      p->in_def = (yyvsp[(4) - (8)].num);
		      p->in_single = (int)(intptr_t)(yyvsp[(6) - (8)].nd)->cdr;
		    }
    break;

  case 308:
/* Line 1787 of yacc.c  */
#line 2130 "src/parse.y"
    {
		      if (p->in_def || p->in_single)
			yyerror(p, "module definition in method body");
		      (yyval.nd) = local_switch(p);
		    }
    break;

  case 309:
/* Line 1787 of yacc.c  */
#line 2137 "src/parse.y"
    {
		      (yyval.nd) = new_module(p, (yyvsp[(2) - (5)].nd), (yyvsp[(4) - (5)].nd));
		      local_resume(p, (yyvsp[(3) - (5)].nd));
		    }
    break;

  case 310:
/* Line 1787 of yacc.c  */
#line 2142 "src/parse.y"
    {
		      p->in_def++;
		      (yyval.nd) = local_switch(p);
		    }
    break;

  case 311:
/* Line 1787 of yacc.c  */
#line 2149 "src/parse.y"
    {
		      (yyval.nd) = new_def(p, (yyvsp[(2) - (6)].id), (yyvsp[(4) - (6)].nd), (yyvsp[(5) - (6)].nd));
		      local_resume(p, (yyvsp[(3) - (6)].nd));
		      p->in_def--;
		    }
    break;

  case 312:
/* Line 1787 of yacc.c  */
#line 2154 "src/parse.y"
    {p->lstate = EXPR_FNAME;}
    break;

  case 313:
/* Line 1787 of yacc.c  */
#line 2155 "src/parse.y"
    {
		      p->in_single++;
		      p->lstate = EXPR_ENDFN; /* force for args */
		      (yyval.nd) = local_switch(p);
		    }
    break;

  case 314:
/* Line 1787 of yacc.c  */
#line 2163 "src/parse.y"
    {
		      (yyval.nd) = new_sdef(p, (yyvsp[(2) - (9)].nd), (yyvsp[(5) - (9)].id), (yyvsp[(7) - (9)].nd), (yyvsp[(8) - (9)].nd));
		      local_resume(p, (yyvsp[(6) - (9)].nd));
		      p->in_single--;
		    }
    break;

  case 315:
/* Line 1787 of yacc.c  */
#line 2169 "src/parse.y"
    {
		      (yyval.nd) = new_break(p, 0);
		    }
    break;

  case 316:
/* Line 1787 of yacc.c  */
#line 2173 "src/parse.y"
    {
		      (yyval.nd) = new_next(p, 0);
		    }
    break;

  case 317:
/* Line 1787 of yacc.c  */
#line 2177 "src/parse.y"
    {
		      (yyval.nd) = new_redo(p);
		    }
    break;

  case 318:
/* Line 1787 of yacc.c  */
#line 2181 "src/parse.y"
    {
		      (yyval.nd) = new_retry(p);
		    }
    break;

  case 319:
/* Line 1787 of yacc.c  */
#line 2187 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		      if (!(yyval.nd)) (yyval.nd) = new_nil(p);
		    }
    break;

  case 326:
/* Line 1787 of yacc.c  */
#line 2206 "src/parse.y"
    {
		      (yyval.nd) = new_if(p, cond((yyvsp[(2) - (5)].nd)), (yyvsp[(4) - (5)].nd), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 328:
/* Line 1787 of yacc.c  */
#line 2213 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 329:
/* Line 1787 of yacc.c  */
#line 2219 "src/parse.y"
    {
		      (yyval.nd) = list1(list1((yyvsp[(1) - (1)].nd)));
		    }
    break;

  case 331:
/* Line 1787 of yacc.c  */
#line 2226 "src/parse.y"
    {
		      (yyval.nd) = new_arg(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 332:
/* Line 1787 of yacc.c  */
#line 2230 "src/parse.y"
    {
		      (yyval.nd) = new_masgn(p, (yyvsp[(2) - (3)].nd), 0);
		    }
    break;

  case 333:
/* Line 1787 of yacc.c  */
#line 2236 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 334:
/* Line 1787 of yacc.c  */
#line 2240 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 335:
/* Line 1787 of yacc.c  */
#line 2246 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (1)].nd),0,0);
		    }
    break;

  case 336:
/* Line 1787 of yacc.c  */
#line 2250 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (4)].nd), new_arg(p, (yyvsp[(4) - (4)].id)), 0);
		    }
    break;

  case 337:
/* Line 1787 of yacc.c  */
#line 2254 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (6)].nd), new_arg(p, (yyvsp[(4) - (6)].id)), (yyvsp[(6) - (6)].nd));
		    }
    break;

  case 338:
/* Line 1787 of yacc.c  */
#line 2258 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (3)].nd), (node*)-1, 0);
		    }
    break;

  case 339:
/* Line 1787 of yacc.c  */
#line 2262 "src/parse.y"
    {
		      (yyval.nd) = list3((yyvsp[(1) - (5)].nd), (node*)-1, (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 340:
/* Line 1787 of yacc.c  */
#line 2266 "src/parse.y"
    {
		      (yyval.nd) = list3(0, new_arg(p, (yyvsp[(2) - (2)].id)), 0);
		    }
    break;

  case 341:
/* Line 1787 of yacc.c  */
#line 2270 "src/parse.y"
    {
		      (yyval.nd) = list3(0, new_arg(p, (yyvsp[(2) - (4)].id)), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 342:
/* Line 1787 of yacc.c  */
#line 2274 "src/parse.y"
    {
		      (yyval.nd) = list3(0, (node*)-1, 0);
		    }
    break;

  case 343:
/* Line 1787 of yacc.c  */
#line 2278 "src/parse.y"
    {
		      (yyval.nd) = list3(0, (node*)-1, (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 344:
/* Line 1787 of yacc.c  */
#line 2284 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (6)].nd), (yyvsp[(3) - (6)].nd), (yyvsp[(5) - (6)].id), 0, (yyvsp[(6) - (6)].id));
		    }
    break;

  case 345:
/* Line 1787 of yacc.c  */
#line 2288 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (8)].nd), (yyvsp[(3) - (8)].nd), (yyvsp[(5) - (8)].id), (yyvsp[(7) - (8)].nd), (yyvsp[(8) - (8)].id));
		    }
    break;

  case 346:
/* Line 1787 of yacc.c  */
#line 2292 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].nd), 0, 0, (yyvsp[(4) - (4)].id));
		    }
    break;

  case 347:
/* Line 1787 of yacc.c  */
#line 2296 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (6)].nd), (yyvsp[(3) - (6)].nd), 0, (yyvsp[(5) - (6)].nd), (yyvsp[(6) - (6)].id));
		    }
    break;

  case 348:
/* Line 1787 of yacc.c  */
#line 2300 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (4)].nd), 0, (yyvsp[(3) - (4)].id), 0, (yyvsp[(4) - (4)].id));
		    }
    break;

  case 349:
/* Line 1787 of yacc.c  */
#line 2304 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (2)].nd), 0, 1, 0, 0);
		    }
    break;

  case 350:
/* Line 1787 of yacc.c  */
#line 2308 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (6)].nd), 0, (yyvsp[(3) - (6)].id), (yyvsp[(5) - (6)].nd), (yyvsp[(6) - (6)].id));
		    }
    break;

  case 351:
/* Line 1787 of yacc.c  */
#line 2312 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (2)].nd), 0, 0, 0, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 352:
/* Line 1787 of yacc.c  */
#line 2316 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), 0, (yyvsp[(4) - (4)].id));
		    }
    break;

  case 353:
/* Line 1787 of yacc.c  */
#line 2320 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (6)].nd), (yyvsp[(3) - (6)].id), (yyvsp[(5) - (6)].nd), (yyvsp[(6) - (6)].id));
		    }
    break;

  case 354:
/* Line 1787 of yacc.c  */
#line 2324 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (2)].nd), 0, 0, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 355:
/* Line 1787 of yacc.c  */
#line 2328 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (4)].nd), 0, (yyvsp[(3) - (4)].nd), (yyvsp[(4) - (4)].id));
		    }
    break;

  case 356:
/* Line 1787 of yacc.c  */
#line 2332 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, 0, (yyvsp[(1) - (2)].id), 0, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 357:
/* Line 1787 of yacc.c  */
#line 2336 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, 0, (yyvsp[(1) - (4)].id), (yyvsp[(3) - (4)].nd), (yyvsp[(4) - (4)].id));
		    }
    break;

  case 358:
/* Line 1787 of yacc.c  */
#line 2340 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, 0, 0, 0, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 360:
/* Line 1787 of yacc.c  */
#line 2347 "src/parse.y"
    {
		      p->cmd_start = TRUE;
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		    }
    break;

  case 361:
/* Line 1787 of yacc.c  */
#line 2354 "src/parse.y"
    {
		      local_add_f(p, 0);
		      (yyval.nd) = 0;
		    }
    break;

  case 362:
/* Line 1787 of yacc.c  */
#line 2359 "src/parse.y"
    {
		      local_add_f(p, 0);
		      (yyval.nd) = 0;
		    }
    break;

  case 363:
/* Line 1787 of yacc.c  */
#line 2364 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (4)].nd);
		    }
    break;

  case 364:
/* Line 1787 of yacc.c  */
#line 2371 "src/parse.y"
    {
		      (yyval.nd) = 0;
		    }
    break;

  case 365:
/* Line 1787 of yacc.c  */
#line 2375 "src/parse.y"
    {
		      (yyval.nd) = 0;
		    }
    break;

  case 368:
/* Line 1787 of yacc.c  */
#line 2385 "src/parse.y"
    {
		      local_add_f(p, (yyvsp[(1) - (1)].id));
		      new_bv(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 370:
/* Line 1787 of yacc.c  */
#line 2393 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (4)].nd);
		    }
    break;

  case 371:
/* Line 1787 of yacc.c  */
#line 2397 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		    }
    break;

  case 372:
/* Line 1787 of yacc.c  */
#line 2403 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		    }
    break;

  case 373:
/* Line 1787 of yacc.c  */
#line 2407 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		    }
    break;

  case 374:
/* Line 1787 of yacc.c  */
#line 2413 "src/parse.y"
    {
		      local_nest(p);
		    }
    break;

  case 375:
/* Line 1787 of yacc.c  */
#line 2419 "src/parse.y"
    {
		      (yyval.nd) = new_block(p,(yyvsp[(3) - (5)].nd),(yyvsp[(4) - (5)].nd));
		      local_unnest(p);
		    }
    break;

  case 376:
/* Line 1787 of yacc.c  */
#line 2426 "src/parse.y"
    {
		      if ((yyvsp[(1) - (2)].nd)->car == (node*)NODE_YIELD) {
			yyerror(p, "block given to yield");
		      }
		      else {
			call_with_block(p, (yyvsp[(1) - (2)].nd), (yyvsp[(2) - (2)].nd));
		      }
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 377:
/* Line 1787 of yacc.c  */
#line 2436 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 378:
/* Line 1787 of yacc.c  */
#line 2440 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), (yyvsp[(4) - (5)].nd));
		      call_with_block(p, (yyval.nd), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 379:
/* Line 1787 of yacc.c  */
#line 2445 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (5)].nd), (yyvsp[(3) - (5)].id), (yyvsp[(4) - (5)].nd));
		      call_with_block(p, (yyval.nd), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 380:
/* Line 1787 of yacc.c  */
#line 2452 "src/parse.y"
    {
		      (yyval.nd) = new_fcall(p, (yyvsp[(1) - (2)].id), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 381:
/* Line 1787 of yacc.c  */
#line 2456 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 382:
/* Line 1787 of yacc.c  */
#line 2460 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), (yyvsp[(4) - (4)].nd));
		    }
    break;

  case 383:
/* Line 1787 of yacc.c  */
#line 2464 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].id), 0);
		    }
    break;

  case 384:
/* Line 1787 of yacc.c  */
#line 2468 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), intern2("call",4), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 385:
/* Line 1787 of yacc.c  */
#line 2472 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (3)].nd), intern2("call",4), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 386:
/* Line 1787 of yacc.c  */
#line 2476 "src/parse.y"
    {
		      (yyval.nd) = new_super(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 387:
/* Line 1787 of yacc.c  */
#line 2480 "src/parse.y"
    {
		      (yyval.nd) = new_zsuper(p);
		    }
    break;

  case 388:
/* Line 1787 of yacc.c  */
#line 2484 "src/parse.y"
    {
		      (yyval.nd) = new_call(p, (yyvsp[(1) - (4)].nd), intern2("[]",2), (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 389:
/* Line 1787 of yacc.c  */
#line 2490 "src/parse.y"
    {
		      local_nest(p);
		    }
    break;

  case 390:
/* Line 1787 of yacc.c  */
#line 2495 "src/parse.y"
    {
		      (yyval.nd) = new_block(p,(yyvsp[(3) - (5)].nd),(yyvsp[(4) - (5)].nd));
		      local_unnest(p);
		    }
    break;

  case 391:
/* Line 1787 of yacc.c  */
#line 2500 "src/parse.y"
    {
		      local_nest(p);
		    }
    break;

  case 392:
/* Line 1787 of yacc.c  */
#line 2505 "src/parse.y"
    {
		      (yyval.nd) = new_block(p,(yyvsp[(3) - (5)].nd),(yyvsp[(4) - (5)].nd));
		      local_unnest(p);
		    }
    break;

  case 393:
/* Line 1787 of yacc.c  */
#line 2514 "src/parse.y"
    {
		      (yyval.nd) = cons(cons((yyvsp[(2) - (5)].nd), (yyvsp[(4) - (5)].nd)), (yyvsp[(5) - (5)].nd));
		    }
    break;

  case 394:
/* Line 1787 of yacc.c  */
#line 2520 "src/parse.y"
    {
		      if ((yyvsp[(1) - (1)].nd)) {
			(yyval.nd) = cons(cons(0, (yyvsp[(1) - (1)].nd)), 0);
		      }
		      else {
			(yyval.nd) = 0;
		      }
		    }
    break;

  case 396:
/* Line 1787 of yacc.c  */
#line 2534 "src/parse.y"
    {
		      (yyval.nd) = list1(list3((yyvsp[(2) - (6)].nd), (yyvsp[(3) - (6)].nd), (yyvsp[(5) - (6)].nd)));
		      if ((yyvsp[(6) - (6)].nd)) (yyval.nd) = append((yyval.nd), (yyvsp[(6) - (6)].nd));
		    }
    break;

  case 398:
/* Line 1787 of yacc.c  */
#line 2542 "src/parse.y"
    {
			(yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 401:
/* Line 1787 of yacc.c  */
#line 2550 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 403:
/* Line 1787 of yacc.c  */
#line 2557 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 411:
/* Line 1787 of yacc.c  */
#line 2572 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 412:
/* Line 1787 of yacc.c  */
#line 2576 "src/parse.y"
    {
		      (yyval.nd) = new_dstr(p, push((yyvsp[(2) - (3)].nd), (yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 414:
/* Line 1787 of yacc.c  */
#line 2583 "src/parse.y"
    {
		      (yyval.nd) = append((yyvsp[(1) - (2)].nd), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 415:
/* Line 1787 of yacc.c  */
#line 2589 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 416:
/* Line 1787 of yacc.c  */
#line 2593 "src/parse.y"
    {
		      (yyval.nd) = p->lex_strterm;
		      p->lex_strterm = NULL;
		    }
    break;

  case 417:
/* Line 1787 of yacc.c  */
#line 2599 "src/parse.y"
    {
		      p->lex_strterm = (yyvsp[(2) - (4)].nd);
		      (yyval.nd) = list2((yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].nd));
		    }
    break;

  case 418:
/* Line 1787 of yacc.c  */
#line 2604 "src/parse.y"
    {
		      (yyval.nd) = list1(new_literal_delim(p));
		    }
    break;

  case 419:
/* Line 1787 of yacc.c  */
#line 2610 "src/parse.y"
    {
			(yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 420:
/* Line 1787 of yacc.c  */
#line 2614 "src/parse.y"
    {
		      (yyval.nd) = new_dxstr(p, push((yyvsp[(2) - (3)].nd), (yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 421:
/* Line 1787 of yacc.c  */
#line 2620 "src/parse.y"
    {
			(yyval.nd) = (yyvsp[(2) - (2)].nd);
		    }
    break;

  case 422:
/* Line 1787 of yacc.c  */
#line 2624 "src/parse.y"
    {
		      (yyval.nd) = new_dregx(p, (yyvsp[(2) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 428:
/* Line 1787 of yacc.c  */
#line 2641 "src/parse.y"
    {
		      parsing_heredoc_inf(p)->doc = list1(new_str(p, "", 0));
		      heredoc_end(p);
		    }
    break;

  case 429:
/* Line 1787 of yacc.c  */
#line 2646 "src/parse.y"
    {
		      parsing_heredoc_inf(p)->doc = (yyvsp[(1) - (2)].nd);
		      heredoc_end(p);
		    }
    break;

  case 430:
/* Line 1787 of yacc.c  */
#line 2653 "src/parse.y"
    {
		      (yyval.nd) = new_words(p, list1((yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 431:
/* Line 1787 of yacc.c  */
#line 2657 "src/parse.y"
    {
		      (yyval.nd) = new_words(p, push((yyvsp[(2) - (3)].nd), (yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 432:
/* Line 1787 of yacc.c  */
#line 2664 "src/parse.y"
    {
		      (yyval.nd) = new_sym(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 433:
/* Line 1787 of yacc.c  */
#line 2668 "src/parse.y"
    {
		      p->lstate = EXPR_END;
		      (yyval.nd) = new_dsym(p, push((yyvsp[(3) - (4)].nd), (yyvsp[(4) - (4)].nd)));
		    }
    break;

  case 434:
/* Line 1787 of yacc.c  */
#line 2675 "src/parse.y"
    {
		      p->lstate = EXPR_END;
		      (yyval.id) = (yyvsp[(2) - (2)].id);
		    }
    break;

  case 439:
/* Line 1787 of yacc.c  */
#line 2686 "src/parse.y"
    {
		      (yyval.id) = new_strsym(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 440:
/* Line 1787 of yacc.c  */
#line 2690 "src/parse.y"
    {
		      (yyval.id) = new_strsym(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 441:
/* Line 1787 of yacc.c  */
#line 2696 "src/parse.y"
    {
		      (yyval.nd) = new_symbols(p, list1((yyvsp[(2) - (2)].nd)));
		    }
    break;

  case 442:
/* Line 1787 of yacc.c  */
#line 2700 "src/parse.y"
    {
		      (yyval.nd) = new_symbols(p, push((yyvsp[(2) - (3)].nd), (yyvsp[(3) - (3)].nd)));
		    }
    break;

  case 445:
/* Line 1787 of yacc.c  */
#line 2708 "src/parse.y"
    {
		      (yyval.nd) = negate_lit(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 446:
/* Line 1787 of yacc.c  */
#line 2712 "src/parse.y"
    {
		      (yyval.nd) = negate_lit(p, (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 447:
/* Line 1787 of yacc.c  */
#line 2718 "src/parse.y"
    {
		      (yyval.nd) = new_lvar(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 448:
/* Line 1787 of yacc.c  */
#line 2722 "src/parse.y"
    {
		      (yyval.nd) = new_ivar(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 449:
/* Line 1787 of yacc.c  */
#line 2726 "src/parse.y"
    {
		      (yyval.nd) = new_gvar(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 450:
/* Line 1787 of yacc.c  */
#line 2730 "src/parse.y"
    {
		      (yyval.nd) = new_cvar(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 451:
/* Line 1787 of yacc.c  */
#line 2734 "src/parse.y"
    {
		      (yyval.nd) = new_const(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 452:
/* Line 1787 of yacc.c  */
#line 2740 "src/parse.y"
    {
		      assignable(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 453:
/* Line 1787 of yacc.c  */
#line 2746 "src/parse.y"
    {
		      (yyval.nd) = var_reference(p, (yyvsp[(1) - (1)].nd));
		    }
    break;

  case 454:
/* Line 1787 of yacc.c  */
#line 2750 "src/parse.y"
    {
		      (yyval.nd) = new_nil(p);
		    }
    break;

  case 455:
/* Line 1787 of yacc.c  */
#line 2754 "src/parse.y"
    {
		      (yyval.nd) = new_self(p);
   		    }
    break;

  case 456:
/* Line 1787 of yacc.c  */
#line 2758 "src/parse.y"
    {
		      (yyval.nd) = new_true(p);
   		    }
    break;

  case 457:
/* Line 1787 of yacc.c  */
#line 2762 "src/parse.y"
    {
		      (yyval.nd) = new_false(p);
   		    }
    break;

  case 458:
/* Line 1787 of yacc.c  */
#line 2766 "src/parse.y"
    {
		      if (!p->filename) {
			p->filename = "(null)";
		      }
		      (yyval.nd) = new_str(p, p->filename, strlen(p->filename));
		    }
    break;

  case 459:
/* Line 1787 of yacc.c  */
#line 2773 "src/parse.y"
    {
		      char buf[16];

		      snprintf(buf, sizeof(buf), "%d", p->lineno);
		      (yyval.nd) = new_int(p, buf, 10);
		    }
    break;

  case 462:
/* Line 1787 of yacc.c  */
#line 2786 "src/parse.y"
    {
		      (yyval.nd) = 0;
		    }
    break;

  case 463:
/* Line 1787 of yacc.c  */
#line 2790 "src/parse.y"
    {
		      p->lstate = EXPR_BEG;
		      p->cmd_start = TRUE;
		    }
    break;

  case 464:
/* Line 1787 of yacc.c  */
#line 2795 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(3) - (4)].nd);
		    }
    break;

  case 465:
/* Line 1787 of yacc.c  */
#line 2799 "src/parse.y"
    {
		      yyerrok;
		      (yyval.nd) = 0;
		    }
    break;

  case 466:
/* Line 1787 of yacc.c  */
#line 2806 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(2) - (3)].nd);
		      p->lstate = EXPR_BEG;
		      p->cmd_start = TRUE;
		    }
    break;

  case 467:
/* Line 1787 of yacc.c  */
#line 2812 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 468:
/* Line 1787 of yacc.c  */
#line 2818 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (6)].nd), (yyvsp[(3) - (6)].nd), (yyvsp[(5) - (6)].id), 0, (yyvsp[(6) - (6)].id));
		    }
    break;

  case 469:
/* Line 1787 of yacc.c  */
#line 2822 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (8)].nd), (yyvsp[(3) - (8)].nd), (yyvsp[(5) - (8)].id), (yyvsp[(7) - (8)].nd), (yyvsp[(8) - (8)].id));
		    }
    break;

  case 470:
/* Line 1787 of yacc.c  */
#line 2826 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].nd), 0, 0, (yyvsp[(4) - (4)].id));
		    }
    break;

  case 471:
/* Line 1787 of yacc.c  */
#line 2830 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (6)].nd), (yyvsp[(3) - (6)].nd), 0, (yyvsp[(5) - (6)].nd), (yyvsp[(6) - (6)].id));
		    }
    break;

  case 472:
/* Line 1787 of yacc.c  */
#line 2834 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (4)].nd), 0, (yyvsp[(3) - (4)].id), 0, (yyvsp[(4) - (4)].id));
		    }
    break;

  case 473:
/* Line 1787 of yacc.c  */
#line 2838 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (6)].nd), 0, (yyvsp[(3) - (6)].id), (yyvsp[(5) - (6)].nd), (yyvsp[(6) - (6)].id));
		    }
    break;

  case 474:
/* Line 1787 of yacc.c  */
#line 2842 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, (yyvsp[(1) - (2)].nd), 0, 0, 0, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 475:
/* Line 1787 of yacc.c  */
#line 2846 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (4)].nd), (yyvsp[(3) - (4)].id), 0, (yyvsp[(4) - (4)].id));
		    }
    break;

  case 476:
/* Line 1787 of yacc.c  */
#line 2850 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (6)].nd), (yyvsp[(3) - (6)].id), (yyvsp[(5) - (6)].nd), (yyvsp[(6) - (6)].id));
		    }
    break;

  case 477:
/* Line 1787 of yacc.c  */
#line 2854 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (2)].nd), 0, 0, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 478:
/* Line 1787 of yacc.c  */
#line 2858 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, (yyvsp[(1) - (4)].nd), 0, (yyvsp[(3) - (4)].nd), (yyvsp[(4) - (4)].id));
		    }
    break;

  case 479:
/* Line 1787 of yacc.c  */
#line 2862 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, 0, (yyvsp[(1) - (2)].id), 0, (yyvsp[(2) - (2)].id));
		    }
    break;

  case 480:
/* Line 1787 of yacc.c  */
#line 2866 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, 0, (yyvsp[(1) - (4)].id), (yyvsp[(3) - (4)].nd), (yyvsp[(4) - (4)].id));
		    }
    break;

  case 481:
/* Line 1787 of yacc.c  */
#line 2870 "src/parse.y"
    {
		      (yyval.nd) = new_args(p, 0, 0, 0, 0, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 482:
/* Line 1787 of yacc.c  */
#line 2874 "src/parse.y"
    {
		      local_add_f(p, 0);
		      (yyval.nd) = new_args(p, 0, 0, 0, 0, 0);
		    }
    break;

  case 483:
/* Line 1787 of yacc.c  */
#line 2881 "src/parse.y"
    {
		      yyerror(p, "formal argument cannot be a constant");
		      (yyval.nd) = 0;
		    }
    break;

  case 484:
/* Line 1787 of yacc.c  */
#line 2886 "src/parse.y"
    {
		      yyerror(p, "formal argument cannot be an instance variable");
		      (yyval.nd) = 0;
		    }
    break;

  case 485:
/* Line 1787 of yacc.c  */
#line 2891 "src/parse.y"
    {
		      yyerror(p, "formal argument cannot be a global variable");
		      (yyval.nd) = 0;
		    }
    break;

  case 486:
/* Line 1787 of yacc.c  */
#line 2896 "src/parse.y"
    {
		      yyerror(p, "formal argument cannot be a class variable");
		      (yyval.nd) = 0;
		    }
    break;

  case 487:
/* Line 1787 of yacc.c  */
#line 2903 "src/parse.y"
    {
		      (yyval.id) = 0;
		    }
    break;

  case 488:
/* Line 1787 of yacc.c  */
#line 2907 "src/parse.y"
    {
		      local_add_f(p, (yyvsp[(1) - (1)].id));
		      (yyval.id) = (yyvsp[(1) - (1)].id);
		    }
    break;

  case 489:
/* Line 1787 of yacc.c  */
#line 2914 "src/parse.y"
    {
		      (yyval.nd) = new_arg(p, (yyvsp[(1) - (1)].id));
		    }
    break;

  case 490:
/* Line 1787 of yacc.c  */
#line 2918 "src/parse.y"
    {
		      (yyval.nd) = new_masgn(p, (yyvsp[(2) - (3)].nd), 0);
		    }
    break;

  case 491:
/* Line 1787 of yacc.c  */
#line 2924 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 492:
/* Line 1787 of yacc.c  */
#line 2928 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 493:
/* Line 1787 of yacc.c  */
#line 2934 "src/parse.y"
    {
		      local_add_f(p, (yyvsp[(1) - (3)].id));
		      (yyval.nd) = cons(nsym((yyvsp[(1) - (3)].id)), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 494:
/* Line 1787 of yacc.c  */
#line 2941 "src/parse.y"
    {
		      local_add_f(p, (yyvsp[(1) - (3)].id));
		      (yyval.nd) = cons(nsym((yyvsp[(1) - (3)].id)), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 495:
/* Line 1787 of yacc.c  */
#line 2948 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 496:
/* Line 1787 of yacc.c  */
#line 2952 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 497:
/* Line 1787 of yacc.c  */
#line 2958 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 498:
/* Line 1787 of yacc.c  */
#line 2962 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 501:
/* Line 1787 of yacc.c  */
#line 2972 "src/parse.y"
    {
		      local_add_f(p, (yyvsp[(2) - (2)].id));
		      (yyval.id) = (yyvsp[(2) - (2)].id);
		    }
    break;

  case 502:
/* Line 1787 of yacc.c  */
#line 2977 "src/parse.y"
    {
		      local_add_f(p, 0);
		      (yyval.id) = -1;
		    }
    break;

  case 505:
/* Line 1787 of yacc.c  */
#line 2988 "src/parse.y"
    {
		      local_add_f(p, (yyvsp[(2) - (2)].id));
		      (yyval.id) = (yyvsp[(2) - (2)].id);
		    }
    break;

  case 506:
/* Line 1787 of yacc.c  */
#line 2995 "src/parse.y"
    {
		      (yyval.id) = (yyvsp[(2) - (2)].id);
		    }
    break;

  case 507:
/* Line 1787 of yacc.c  */
#line 2999 "src/parse.y"
    {
		      local_add_f(p, 0);
		      (yyval.id) = 0;
		    }
    break;

  case 508:
/* Line 1787 of yacc.c  */
#line 3006 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (1)].nd);
		      if (!(yyval.nd)) (yyval.nd) = new_nil(p);
		    }
    break;

  case 509:
/* Line 1787 of yacc.c  */
#line 3010 "src/parse.y"
    {p->lstate = EXPR_BEG;}
    break;

  case 510:
/* Line 1787 of yacc.c  */
#line 3011 "src/parse.y"
    {
		      if ((yyvsp[(3) - (4)].nd) == 0) {
			yyerror(p, "can't define singleton method for ().");
		      }
		      else {
			switch ((enum node_type)(int)(intptr_t)(yyvsp[(3) - (4)].nd)->car) {
			case NODE_STR:
			case NODE_DSTR:
			case NODE_XSTR:
			case NODE_DXSTR:
			case NODE_DREGX:
			case NODE_MATCH:
			case NODE_FLOAT:
			case NODE_ARRAY:
			case NODE_HEREDOC:
			  yyerror(p, "can't define singleton method for literals");
			default:
			  break;
			}
		      }
		      (yyval.nd) = (yyvsp[(3) - (4)].nd);
		    }
    break;

  case 512:
/* Line 1787 of yacc.c  */
#line 3037 "src/parse.y"
    {
		      (yyval.nd) = (yyvsp[(1) - (2)].nd);
		    }
    break;

  case 513:
/* Line 1787 of yacc.c  */
#line 3043 "src/parse.y"
    {
		      (yyval.nd) = list1((yyvsp[(1) - (1)].nd));
		    }
    break;

  case 514:
/* Line 1787 of yacc.c  */
#line 3047 "src/parse.y"
    {
		      (yyval.nd) = push((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 515:
/* Line 1787 of yacc.c  */
#line 3053 "src/parse.y"
    {
		      (yyval.nd) = cons((yyvsp[(1) - (3)].nd), (yyvsp[(3) - (3)].nd));
		    }
    break;

  case 516:
/* Line 1787 of yacc.c  */
#line 3057 "src/parse.y"
    {
		      (yyval.nd) = cons(new_sym(p, (yyvsp[(1) - (2)].id)), (yyvsp[(2) - (2)].nd));
		    }
    break;

  case 538:
/* Line 1787 of yacc.c  */
#line 3101 "src/parse.y"
    {yyerrok;}
    break;

  case 540:
/* Line 1787 of yacc.c  */
#line 3106 "src/parse.y"
    {
		      p->lineno++;
		      p->column = 0;
		    }
    break;

  case 543:
/* Line 1787 of yacc.c  */
#line 3113 "src/parse.y"
    {yyerrok;}
    break;

  case 544:
/* Line 1787 of yacc.c  */
#line 3117 "src/parse.y"
    {
		      (yyval.nd) = 0;
		    }
    break;


/* Line 1787 of yacc.c  */
#line 8657 "/home/kou/work/c/groonga.central/vendor/mruby/mruby.master/build/host/src/y.tab.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (p, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (p, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, p);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, p);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (p, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, p);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, p);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2050 of yacc.c  */
#line 3121 "src/parse.y"

#define yylval  (*((YYSTYPE*)(p->ylval)))

static void
yyerror(parser_state *p, const char *s)
{
  char* c;
  int n;

  if (! p->capture_errors) {
#ifdef ENABLE_STDIO
    if (p->filename) {
      fprintf(stderr, "%s:%d:%d: %s\n", p->filename, p->lineno, p->column, s);
    }
    else {
      fprintf(stderr, "line %d:%d: %s\n", p->lineno, p->column, s);
    }
#endif
  }
  else if (p->nerr < sizeof(p->error_buffer) / sizeof(p->error_buffer[0])) {
    n = strlen(s);
    c = (char *)parser_palloc(p, n + 1);
    memcpy(c, s, n + 1);
    p->error_buffer[p->nerr].message = c;
    p->error_buffer[p->nerr].lineno = p->lineno;
    p->error_buffer[p->nerr].column = p->column;
  }
  p->nerr++;
}

static void
yyerror_i(parser_state *p, const char *fmt, int i)
{
  char buf[256];

  snprintf(buf, sizeof(buf), fmt, i);
  yyerror(p, buf);
}

static void
yywarn(parser_state *p, const char *s)
{
  char* c;
  int n;

  if (! p->capture_errors) {
#ifdef ENABLE_STDIO
    if (p->filename) {
      fprintf(stderr, "%s:%d:%d: %s\n", p->filename, p->lineno, p->column, s);
    }
    else {
      fprintf(stderr, "line %d:%d: %s\n", p->lineno, p->column, s);
    }
#endif
  }
  else if (p->nwarn < sizeof(p->warn_buffer) / sizeof(p->warn_buffer[0])) {
    n = strlen(s);
    c = (char *)parser_palloc(p, n + 1);
    memcpy(c, s, n + 1);
    p->warn_buffer[p->nwarn].message = c;
    p->warn_buffer[p->nwarn].lineno = p->lineno;
    p->warn_buffer[p->nwarn].column = p->column;
  }
  p->nwarn++;
}

static void
yywarning(parser_state *p, const char *s)
{
  yywarn(p, s);
}

static void
yywarning_s(parser_state *p, const char *fmt, const char *s)
{
  char buf[256];

  snprintf(buf, sizeof(buf), fmt, s);
  yywarning(p, buf);
}

static void
backref_error(parser_state *p, node *n)
{
  int c;

  c = (int)(intptr_t)n->car;

  if (c == NODE_NTH_REF) {
    yyerror_i(p, "can't set variable $%d", (int)(intptr_t)n->cdr);
  } else if (c == NODE_BACK_REF) {
    yyerror_i(p, "can't set variable $%c", (int)(intptr_t)n->cdr);
  } else {
    mrb_bug(p->mrb, "Internal error in backref_error() : n=>car == %d", c);
  }
}

static int peeks(parser_state *p, const char *s);
static int skips(parser_state *p, const char *s);

static inline int
nextc(parser_state *p)
{
  int c;

  if (p->pb) {
    node *tmp;

    c = (int)(intptr_t)p->pb->car;
    tmp = p->pb;
    p->pb = p->pb->cdr;
    cons_free(tmp);
  }
  else {
#ifdef ENABLE_STDIO
    if (p->f) {
      if (feof(p->f)) goto end_retry;
      c = fgetc(p->f);
      if (c == EOF) goto end_retry;
    }
    else
#endif
    if (!p->s || p->s >= p->send) {
       goto end_retry;
    }
    else {
      c = (unsigned char)*p->s++;
    }
  }
  p->column++;
  return c;

 end_retry:
  if (!p->cxt) return -1;
  else {
    mrbc_context *cxt = p->cxt;

    if (cxt->partial_hook(p) < 0) return -1;
    p->cxt = NULL;
    c = nextc(p);
    p->cxt = cxt;
    return c;
  }
}

static void
pushback(parser_state *p, int c)
{
  if (c < 0) return;
  p->column--;
  p->pb = cons((node*)(intptr_t)c, p->pb);
}

static void
skip(parser_state *p, char term)
{
  int c;

  for (;;) {
    c = nextc(p);
    if (c < 0) break;
    if (c == term) break;
  }
}

static int
peek_n(parser_state *p, int c, int n)
{
  node *list = 0;
  int c0;

  do {
    c0 = nextc(p);
    if (c0 < 0) return FALSE;
    list = push(list, (node*)(intptr_t)c0);
  } while(n--);
  if (p->pb) {
    p->pb = append(p->pb, (node*)list);
  }
  else {
    p->pb = list;
  }
  if (c0 == c) return TRUE;
  return FALSE;
}
#define peek(p,c) peek_n((p), (c), 0)

static int
peeks(parser_state *p, const char *s)
{
  int len = strlen(s);

#ifdef ENABLE_STDIO
  if (p->f) {
    int n = 0;
    while (*s) {
      if (!peek_n(p, *s++, n++)) return FALSE;
    }
    return TRUE;
  }
  else
#endif
  if (p->s && p->s + len >= p->send) {
    if (memcmp(p->s, s, len) == 0) return TRUE;
  }
  return FALSE;
}

static int
skips(parser_state *p, const char *s)
{
  int c;

  for (;;) {
    // skip until first char
    for (;;) {
      c = nextc(p);
      if (c < 0) return c;
      if (c == *s) break;
    }
    s++;
    if (peeks(p, s)) {
      int len = strlen(s);

      while (len--) {
	nextc(p);
      }
      return TRUE;
    }
	else{
      s--;
    }
  }
  return FALSE;
}


static int
newtok(parser_state *p)
{
  p->bidx = 0;
  return p->column - 1;
}

static void
tokadd(parser_state *p, int c)
{
  if (p->bidx < MRB_PARSER_BUF_SIZE) {
    p->buf[p->bidx++] = c;
  }
}

static int
toklast(parser_state *p)
{
  return p->buf[p->bidx-1];
}

static void
tokfix(parser_state *p)
{
  if (p->bidx >= MRB_PARSER_BUF_SIZE) {
    yyerror(p, "string too long (truncated)");
  }
  p->buf[p->bidx] = '\0';
}

static const char*
tok(parser_state *p)
{
  return p->buf;
}

static int
toklen(parser_state *p)
{
  return p->bidx;
}

#define IS_ARG() (p->lstate == EXPR_ARG || p->lstate == EXPR_CMDARG)
#define IS_END() (p->lstate == EXPR_END || p->lstate == EXPR_ENDARG || p->lstate == EXPR_ENDFN)
#define IS_BEG() (p->lstate == EXPR_BEG || p->lstate == EXPR_MID || p->lstate == EXPR_VALUE || p->lstate == EXPR_CLASS)
#define IS_SPCARG(c) (IS_ARG() && space_seen && !ISSPACE(c))
#define IS_LABEL_POSSIBLE() ((p->lstate == EXPR_BEG && !cmd_state) || IS_ARG())
#define IS_LABEL_SUFFIX(n) (peek_n(p, ':',(n)) && !peek_n(p, ':', (n)+1))

static int
scan_oct(const int *start, int len, int *retlen)
{
  const int *s = start;
  int retval = 0;

  /* mrb_assert(len <= 3) */
  while (len-- && *s >= '0' && *s <= '7') {
    retval <<= 3;
    retval |= *s++ - '0';
  }
  *retlen = s - start;

  return retval;
}

static int
scan_hex(const int *start, int len, int *retlen)
{
  static const char hexdigit[] = "0123456789abcdef0123456789ABCDEF";
  register const int *s = start;
  register int retval = 0;
  char *tmp;

  /* mrb_assert(len <= 2) */
  while (len-- && *s && (tmp = (char*)strchr(hexdigit, *s))) {
    retval <<= 4;
    retval |= (tmp - hexdigit) & 15;
    s++;
  }
  *retlen = s - start;

  return retval;
}

static int
read_escape(parser_state *p)
{
  int c;

  switch (c = nextc(p)) {
  case '\\':	/* Backslash */
    return c;

  case 'n':	/* newline */
    return '\n';

  case 't':	/* horizontal tab */
    return '\t';

  case 'r':	/* carriage-return */
    return '\r';

  case 'f':	/* form-feed */
    return '\f';

  case 'v':	/* vertical tab */
    return '\13';

  case 'a':	/* alarm(bell) */
    return '\007';

  case 'e':	/* escape */
    return 033;

  case '0': case '1': case '2': case '3': /* octal constant */
  case '4': case '5': case '6': case '7':
    {
      int buf[3];
      int i;

      buf[0] = c;
      for (i=1; i<3; i++) {
        buf[i] = nextc(p);
        if (buf[i] == -1) goto eof;
        if (buf[i] < '0' || '7' < buf[i]) {
          pushback(p, buf[i]);
          break;
        }
      }
      c = scan_oct(buf, i, &i);
    }
    return c;

  case 'x':	/* hex constant */
    {
      int buf[2];
      int i;

      for (i=0; i<2; i++) {
	buf[i] = nextc(p);
	if (buf[i] == -1) goto eof;
	if (!ISXDIGIT(buf[i])) {
	  pushback(p, buf[i]);
	  break;
	}
      }
      c = scan_hex(buf, i, &i);
      if (i == 0) {
	yyerror(p, "Invalid escape character syntax");
	return 0;
      }
    }
    return c;

  case 'b':	/* backspace */
    return '\010';

  case 's':	/* space */
    return ' ';

  case 'M':
    if ((c = nextc(p)) != '-') {
      yyerror(p, "Invalid escape character syntax");
      pushback(p, c);
      return '\0';
    }
    if ((c = nextc(p)) == '\\') {
      return read_escape(p) | 0x80;
    }
    else if (c == -1) goto eof;
    else {
      return ((c & 0xff) | 0x80);
    }

  case 'C':
    if ((c = nextc(p)) != '-') {
      yyerror(p, "Invalid escape character syntax");
      pushback(p, c);
      return '\0';
    }
  case 'c':
    if ((c = nextc(p))== '\\') {
      c = read_escape(p);
    }
    else if (c == '?')
      return 0177;
    else if (c == -1) goto eof;
    return c & 0x9f;

  eof:
  case -1:
    yyerror(p, "Invalid escape character syntax");
    return '\0';

  default:
    return c;
  }
}


static int
parse_string(parser_state *p)
{
  int c;
  string_type type = (string_type)(intptr_t)p->lex_strterm->car;
  int nest_level = (intptr_t)p->lex_strterm->cdr->car;
  int beg = (intptr_t)p->lex_strterm->cdr->cdr->car;
  int end = (intptr_t)p->lex_strterm->cdr->cdr->cdr;
  parser_heredoc_info *hinf = (type & STR_FUNC_HEREDOC) ? parsing_heredoc_inf(p) : NULL;

  newtok(p);
  while ((c = nextc(p)) != end || nest_level != 0) {
    if (hinf && (c == '\n' || c == -1)) {
      int line_head;
      tokadd(p, '\n');
      tokfix(p);
      p->lineno++;
      p->column = 0;
      line_head = hinf->line_head;
      hinf->line_head = TRUE;
      if (line_head) {
	/* check whether end of heredoc */
	const char *s = tok(p);
	int len = toklen(p);
	if (hinf->allow_indent) {
	  while (ISSPACE(*s) && len > 0) {
	    ++s;
	    --len;
	  }
	}
	if ((len-1 == hinf->term_len) && (strncmp(s, hinf->term, len-1) == 0)) {
	  return tHEREDOC_END;
	}
      }
      if (c == -1) {
	char buf[256];
	snprintf(buf, sizeof(buf), "can't find string \"%s\" anywhere before EOF", hinf->term);
	yyerror(p, buf);
	return 0;
      }
      yylval.nd = new_str(p, tok(p), toklen(p));
      return tSTRING_MID;
    }
    if (c == -1) {
      yyerror(p, "unterminated string meets end of file");
      return 0;
    }
    else if (c == beg) {
      nest_level++;
      p->lex_strterm->cdr->car = (node*)(intptr_t)nest_level;
    }
    else if (c == end) {
      nest_level--;
      p->lex_strterm->cdr->car = (node*)(intptr_t)nest_level;
    }
    else if (c == '\\') {
      c = nextc(p);
      if (type & STR_FUNC_EXPAND) {
	if (c == end || c == beg) {
	  tokadd(p, c);
	}
	else if ((c == '\n') && (type & STR_FUNC_ARRAY)) {
	  p->lineno++;
	  p->column = 0;
	  tokadd(p, '\n');
	}
	else {
	  if (type & STR_FUNC_REGEXP) {
	    tokadd(p, '\\');
	    if (c != -1)
	      tokadd(p, c);
	  } else {
	    pushback(p, c);
	    tokadd(p, read_escape(p));
	  }
	  if (hinf)
	    hinf->line_head = FALSE;
	}
      } else {
	if (c != beg && c != end) {
	  switch (c) {
	  case '\n':
	    p->lineno++;
	    p->column = 0;
	    break;

	  case '\\':
	    break;

	  default:
	    if (! ISSPACE(c))
	      tokadd(p, '\\');
	  }
	}
	tokadd(p, c);
      }
      continue;
    }
    else if ((c == '#') && (type & STR_FUNC_EXPAND)) {
      c = nextc(p);
      if (c == '{') {
	tokfix(p);
	p->lstate = EXPR_BEG;
	p->cmd_start = TRUE;
	yylval.nd = new_str(p, tok(p), toklen(p));
	if (hinf)
	  hinf->line_head = FALSE;
	return tSTRING_PART;
      }
      tokadd(p, '#');
      pushback(p, c);
      continue;
    }
    if ((type & STR_FUNC_ARRAY) && ISSPACE(c)) {
      if (toklen(p) == 0) {
	do {
	  if (c == '\n') {
	    p->lineno++;
	    p->column = 0;
	  }
	} while (ISSPACE(c = nextc(p)));
	pushback(p, c);
	return tLITERAL_DELIM;
      } else {
	pushback(p, c);
	tokfix(p);
	yylval.nd = new_str(p, tok(p), toklen(p));
	return tSTRING_MID;
      }
    }

    tokadd(p, c);

  }

  tokfix(p);
  p->lstate = EXPR_END;
  end_strterm(p);

  if (type & STR_FUNC_XQUOTE) {
    yylval.nd = new_xstr(p, tok(p), toklen(p));
    return tXSTRING;
  }

  if (type & STR_FUNC_REGEXP) {
    int f = 0;
    int c;
    char *s = strndup(tok(p), toklen(p));
    char flags[3];
    char *flag = flags;
    char *dup;

    newtok(p);
    while (c = nextc(p), c != -1 && ISALPHA(c)) {
      switch (c) {
      case 'i': f |= 1; break;
      case 'x': f |= 2; break;
      case 'm': f |= 4; break;
      default: tokadd(p, c); break;
      }
    }
    pushback(p, c);
    if (toklen(p)) {
      char msg[128];
      tokfix(p);
      snprintf(msg, sizeof(msg), "unknown regexp option%s - %s",
	  toklen(p) > 1 ? "s" : "", tok(p));
      yyerror(p, msg);
    }
    if (f != 0) {
      if (f & 1) *flag++ = 'i';
      if (f & 2) *flag++ = 'x';
      if (f & 4) *flag++ = 'm';
      dup = strndup(flags, (size_t)(flag - flags));
    }
    else {
      dup = NULL;
    }
    yylval.nd = new_regx(p, s, dup);

    return tREGEXP;
  }

  yylval.nd = new_str(p, tok(p), toklen(p));
  return tSTRING;
}


static int
heredoc_identifier(parser_state *p)
{
  int c;
  int type = str_heredoc;
  int indent = FALSE;
  int quote = FALSE;
  node *newnode;
  parser_heredoc_info *info;

  c = nextc(p);
  if (ISSPACE(c) || c == '=') {
    pushback(p, c);
    return 0;
  }
  if (c == '-') {
    indent = TRUE;
    c = nextc(p);
  }
  if (c == '\'' || c == '"') {
    int term = c;
    if (c == '\'')
      quote = TRUE;
    newtok(p);
    while ((c = nextc(p)) != -1 && c != term) {
      if (c == '\n') {
	c = -1;
	break;
      }
      tokadd(p, c);
    }
    if (c == -1) {
      yyerror(p, "unterminated here document identifier");
      return 0;
    }
  } else {
    if (! identchar(c)) {
      pushback(p, c);
      if (indent) pushback(p, '-');
      return 0;
    }
    newtok(p);
    do {
      tokadd(p, c);
    } while ((c = nextc(p)) != -1 && identchar(c));
    pushback(p, c);
  }
  tokfix(p);
  newnode = new_heredoc(p);
  info = (parser_heredoc_info*)newnode->cdr;
  info->term = strndup(tok(p), toklen(p));
  info->term_len = toklen(p);
  if (! quote)
    type |= STR_FUNC_EXPAND;
  info->type = (string_type)type;
  info->allow_indent = indent;
  info->line_head = TRUE;
  info->doc = NULL;
  p->heredocs = push(p->heredocs, newnode);
  if (p->parsing_heredoc == NULL) {
    node *n = p->heredocs;
    while (n->cdr)
      n = n->cdr;
    p->parsing_heredoc = n;
  }
  p->heredoc_starts_nextline = TRUE;
  p->lstate = EXPR_END;

  yylval.nd = newnode;
  return tHEREDOC_BEG;
}

static int
arg_ambiguous(parser_state *p)
{
  yywarning(p, "ambiguous first argument; put parentheses or even spaces");
  return 1;
}

#include "lex.def"

static int
parser_yylex(parser_state *p)
{
  register int c;
  int space_seen = 0;
  int cmd_state;
  enum mrb_lex_state_enum last_state;
  int token_column;

  if (p->lex_strterm) {
    if (is_strterm_type(p, STR_FUNC_HEREDOC)) {
      if ((p->parsing_heredoc != NULL) && (! p->heredoc_starts_nextline))
	return parse_string(p);
    }
    else
      return parse_string(p);
  }
  cmd_state = p->cmd_start;
  p->cmd_start = FALSE;
 retry:
  last_state = p->lstate;
  switch (c = nextc(p)) {
  case '\0':    /* NUL */
  case '\004':  /* ^D */
  case '\032':  /* ^Z */
  case -1:      /* end of script. */
    return 0;

  /* white spaces */
  case ' ': case '\t': case '\f': case '\r':
  case '\13':   /* '\v' */
    space_seen = 1;
    goto retry;

  case '#':     /* it's a comment */
    skip(p, '\n');
  /* fall through */
  case '\n':
    p->heredoc_starts_nextline = FALSE;
    if (p->parsing_heredoc != NULL) {
      p->lex_strterm = new_strterm(p, parsing_heredoc_inf(p)->type, 0, 0);
      goto normal_newline;
    }
    switch (p->lstate) {
    case EXPR_BEG:
    case EXPR_FNAME:
    case EXPR_DOT:
    case EXPR_CLASS:
    case EXPR_VALUE:
      p->lineno++;
      p->column = 0;
      goto retry;
    default:
      break;
    }
    while ((c = nextc(p))) {
      switch (c) {
      case ' ': case '\t': case '\f': case '\r':
      case '\13': /* '\v' */
	space_seen = 1;
	break;
      case '.':
	if ((c = nextc(p)) != '.') {
	  pushback(p, c);
	  pushback(p, '.');
	  goto retry;
	}
      case -1:			/* EOF */
	goto normal_newline;
      default:
	pushback(p, c);
	goto normal_newline;
      }
    }
  normal_newline:
    p->cmd_start = TRUE;
    p->lstate = EXPR_BEG;
    return '\n';

  case '*':
    if ((c = nextc(p)) == '*') {
      if ((c = nextc(p)) == '=') {
	yylval.id = intern2("**",2);
	p->lstate = EXPR_BEG;
	return tOP_ASGN;
      }
      pushback(p, c);
      c = tPOW;
    }
    else {
      if (c == '=') {
	yylval.id = intern_c('*');
	p->lstate = EXPR_BEG;
	return tOP_ASGN;
      }
      pushback(p, c);
      if (IS_SPCARG(c)) {
	yywarning(p, "`*' interpreted as argument prefix");
	c = tSTAR;
      }
      else if (IS_BEG()) {
	c = tSTAR;
      }
      else {
	c = '*';
      }
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    return c;

  case '!':
    c = nextc(p);
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
      if (c == '@') {
	return '!';
      }
    }
    else {
      p->lstate = EXPR_BEG;
    }
    if (c == '=') {
      return tNEQ;
    }
    if (c == '~') {
      return tNMATCH;
    }
    pushback(p, c);
    return '!';

  case '=':
    if (p->column == 1) {
      if (peeks(p, "begin\n")) {
	skips(p, "\n=end\n");
      goto retry;
      }
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    if ((c = nextc(p)) == '=') {
      if ((c = nextc(p)) == '=') {
	return tEQQ;
      }
      pushback(p, c);
      return tEQ;
    }
    if (c == '~') {
      return tMATCH;
    }
    else if (c == '>') {
      return tASSOC;
    }
    pushback(p, c);
    return '=';

  case '<':
    last_state = p->lstate;
    c = nextc(p);
    if (c == '<' &&
	p->lstate != EXPR_DOT &&
	p->lstate != EXPR_CLASS &&
	!IS_END() &&
	(!IS_ARG() || space_seen)) {
      int token = heredoc_identifier(p);
      if (token)
	return token;
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
      if (p->lstate == EXPR_CLASS) {
	p->cmd_start = TRUE;
      }
    }
    if (c == '=') {
      if ((c = nextc(p)) == '>') {
	return tCMP;
      }
      pushback(p, c);
      return tLEQ;
    }
    if (c == '<') {
      if ((c = nextc(p)) == '=') {
	yylval.id = intern2("<<",2);
	p->lstate = EXPR_BEG;
	return tOP_ASGN;
      }
      pushback(p, c);
      return tLSHFT;
    }
    pushback(p, c);
    return '<';

  case '>':
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    if ((c = nextc(p)) == '=') {
      return tGEQ;
    }
    if (c == '>') {
      if ((c = nextc(p)) == '=') {
	yylval.id = intern2(">>",2);
	p->lstate = EXPR_BEG;
	return tOP_ASGN;
      }
      pushback(p, c);
      return tRSHFT;
    }
    pushback(p, c);
    return '>';

  case '"':
    p->lex_strterm = new_strterm(p, str_dquote, '"', 0);
    return tSTRING_BEG;

  case '\'':
    p->lex_strterm = new_strterm(p, str_squote, '\'', 0);
    return parse_string(p);

  case '`':
    if (p->lstate == EXPR_FNAME) {
      p->lstate = EXPR_ENDFN;
      return '`';
    }
    if (p->lstate == EXPR_DOT) {
      if (cmd_state)
        p->lstate = EXPR_CMDARG;
      else
        p->lstate = EXPR_ARG;
      return '`';
    }
    p->lex_strterm = new_strterm(p, str_xquote, '`', 0);
    return tXSTRING_BEG;

  case '?':
    if (IS_END()) {
      p->lstate = EXPR_VALUE;
      return '?';
    }
    c = nextc(p);
    if (c == -1) {
      yyerror(p, "incomplete character syntax");
      return 0;
    }
    if (isspace(c)) {
      if (!IS_ARG()) {
	int c2;
	switch (c) {
	case ' ':
	  c2 = 's';
	  break;
	case '\n':
	  c2 = 'n';
	  break;
	case '\t':
	  c2 = 't';
	  break;
	case '\v':
	  c2 = 'v';
	  break;
	case '\r':
	  c2 = 'r';
	  break;
	case '\f':
	  c2 = 'f';
	  break;
	default:
	  c2 = 0;
	  break;
	}
	if (c2) {
	  char buf[256];
	  snprintf(buf, sizeof(buf), "invalid character syntax; use ?\\%c", c2);
	  yyerror(p, buf);
	}
      }
    ternary:
      pushback(p, c);
      p->lstate = EXPR_VALUE;
      return '?';
    }
    token_column = newtok(p);
    // need support UTF-8 if configured
    if ((isalnum(c) || c == '_')) {
      int c2 = nextc(p);
      pushback(p, c2);
      if ((isalnum(c2) || c2 == '_')) {
	goto ternary;
      }
    }
    if (c == '\\') {
      c = nextc(p);
      if (c == 'u') {
#if 0
	tokadd_utf8(p);
#endif
      }
      else {
	pushback(p, c);
	c = read_escape(p);
	tokadd(p, c);
      }
    }
    else {
      tokadd(p, c);
    }
    tokfix(p);
    yylval.nd = new_str(p, tok(p), toklen(p));
    p->lstate = EXPR_END;
    return tCHAR;

  case '&':
    if ((c = nextc(p)) == '&') {
      p->lstate = EXPR_BEG;
      if ((c = nextc(p)) == '=') {
	yylval.id = intern2("&&",2);
	p->lstate = EXPR_BEG;
	return tOP_ASGN;
      }
      pushback(p, c);
      return tANDOP;
    }
    else if (c == '=') {
      yylval.id = intern_c('&');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    pushback(p, c);
    if (IS_SPCARG(c)) {
      yywarning(p, "`&' interpreted as argument prefix");
      c = tAMPER;
    }
    else if (IS_BEG()) {
      c = tAMPER;
    }
    else {
      c = '&';
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    return c;

  case '|':
    if ((c = nextc(p)) == '|') {
      p->lstate = EXPR_BEG;
      if ((c = nextc(p)) == '=') {
	yylval.id = intern2("||",2);
	p->lstate = EXPR_BEG;
	return tOP_ASGN;
      }
      pushback(p, c);
      return tOROP;
    }
    if (c == '=') {
      yylval.id = intern_c('|');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    }
    else {
      p->lstate = EXPR_BEG;
    }
    pushback(p, c);
    return '|';

  case '+':
    c = nextc(p);
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
      if (c == '@') {
	return tUPLUS;
      }
      pushback(p, c);
      return '+';
    }
    if (c == '=') {
      yylval.id = intern_c('+');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    if (IS_BEG() || (IS_SPCARG(c) && arg_ambiguous(p))) {
      p->lstate = EXPR_BEG;
      pushback(p, c);
      if (c != -1 && ISDIGIT(c)) {
	c = '+';
	goto start_num;
      }
      return tUPLUS;
    }
    p->lstate = EXPR_BEG;
    pushback(p, c);
    return '+';

  case '-':
    c = nextc(p);
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
      if (c == '@') {
	return tUMINUS;
      }
      pushback(p, c);
      return '-';
    }
    if (c == '=') {
      yylval.id = intern_c('-');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    if (c == '>') {
      p->lstate = EXPR_ENDFN;
      return tLAMBDA;
    }
    if (IS_BEG() || (IS_SPCARG(c) && arg_ambiguous(p))) {
      p->lstate = EXPR_BEG;
      pushback(p, c);
      if (c != -1 && ISDIGIT(c)) {
	return tUMINUS_NUM;
      }
      return tUMINUS;
    }
    p->lstate = EXPR_BEG;
    pushback(p, c);
    return '-';

  case '.':
    p->lstate = EXPR_BEG;
    if ((c = nextc(p)) == '.') {
      if ((c = nextc(p)) == '.') {
	return tDOT3;
      }
      pushback(p, c);
      return tDOT2;
    }
    pushback(p, c);
    if (c != -1 && ISDIGIT(c)) {
      yyerror(p, "no .<digit> floating literal anymore; put 0 before dot");
    }
    p->lstate = EXPR_DOT;
    return '.';

  start_num:
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      int is_float, seen_point, seen_e, nondigit;

      is_float = seen_point = seen_e = nondigit = 0;
      p->lstate = EXPR_END;
      token_column = newtok(p);
      if (c == '-' || c == '+') {
	tokadd(p, c);
	c = nextc(p);
      }
      if (c == '0') {
#define no_digits() do {yyerror(p,"numeric literal without digits"); return 0;} while (0)
	int start = toklen(p);
	c = nextc(p);
	if (c == 'x' || c == 'X') {
	  /* hexadecimal */
	  c = nextc(p);
	  if (c != -1 && ISXDIGIT(c)) {
	    do {
	      if (c == '_') {
		if (nondigit) break;
		nondigit = c;
		continue;
	      }
	      if (!ISXDIGIT(c)) break;
	      nondigit = 0;
	      tokadd(p, tolower(c));
	    } while ((c = nextc(p)) != -1);
	  }
	  pushback(p, c);
	  tokfix(p);
	  if (toklen(p) == start) {
	    no_digits();
	  }
	  else if (nondigit) goto trailing_uc;
	  yylval.nd = new_int(p, tok(p), 16);
	  return tINTEGER;
	}
	if (c == 'b' || c == 'B') {
	  /* binary */
	  c = nextc(p);
	  if (c == '0' || c == '1') {
	    do {
	      if (c == '_') {
		if (nondigit) break;
		nondigit = c;
		continue;
	      }
	      if (c != '0' && c != '1') break;
	      nondigit = 0;
	      tokadd(p, c);
	    } while ((c = nextc(p)) != -1);
	  }
	  pushback(p, c);
	  tokfix(p);
	  if (toklen(p) == start) {
	    no_digits();
	  }
	  else if (nondigit) goto trailing_uc;
	  yylval.nd = new_int(p, tok(p), 2);
	  return tINTEGER;
	}
	if (c == 'd' || c == 'D') {
	  /* decimal */
	  c = nextc(p);
	  if (c != -1 && ISDIGIT(c)) {
	    do {
	      if (c == '_') {
		if (nondigit) break;
		nondigit = c;
		continue;
	      }
	      if (!ISDIGIT(c)) break;
	      nondigit = 0;
	      tokadd(p, c);
	    } while ((c = nextc(p)) != -1);
	  }
	  pushback(p, c);
	  tokfix(p);
	  if (toklen(p) == start) {
	    no_digits();
	  }
	  else if (nondigit) goto trailing_uc;
	  yylval.nd = new_int(p, tok(p), 10);
	  return tINTEGER;
	}
	if (c == '_') {
	  /* 0_0 */
	  goto octal_number;
	}
	if (c == 'o' || c == 'O') {
	  /* prefixed octal */
	  c = nextc(p);
	  if (c == -1 || c == '_' || !ISDIGIT(c)) {
	    no_digits();
	  }
	}
	if (c >= '0' && c <= '7') {
	  /* octal */
	octal_number:
	  do {
	    if (c == '_') {
	      if (nondigit) break;
	      nondigit = c;
	      continue;
	    }
	    if (c < '0' || c > '9') break;
	    if (c > '7') goto invalid_octal;
	    nondigit = 0;
	    tokadd(p, c);
	  } while ((c = nextc(p)) != -1);

	  if (toklen(p) > start) {
	    pushback(p, c);
	    tokfix(p);
	    if (nondigit) goto trailing_uc;
	    yylval.nd = new_int(p, tok(p), 8);
	    return tINTEGER;
	  }
	  if (nondigit) {
	    pushback(p, c);
	    goto trailing_uc;
	  }
	}
	if (c > '7' && c <= '9') {
	invalid_octal:
	  yyerror(p, "Invalid octal digit");
	}
	else if (c == '.' || c == 'e' || c == 'E') {
	  tokadd(p, '0');
	}
	else {
	  pushback(p, c);
	  yylval.nd = new_int(p, "0", 10);
	  return tINTEGER;
	}
      }

      for (;;) {
	switch (c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  nondigit = 0;
	  tokadd(p, c);
	  break;

	case '.':
	  if (nondigit) goto trailing_uc;
	  if (seen_point || seen_e) {
	    goto decode_num;
	  }
	  else {
	    int c0 = nextc(p);
	    if (c0 == -1 || !ISDIGIT(c0)) {
	      pushback(p, c0);
	      goto decode_num;
	    }
	    c = c0;
	  }
	  tokadd(p, '.');
	  tokadd(p, c);
	  is_float++;
	  seen_point++;
	  nondigit = 0;
	  break;

	case 'e':
	case 'E':
	  if (nondigit) {
	    pushback(p, c);
	    c = nondigit;
	    goto decode_num;
	  }
	  if (seen_e) {
	    goto decode_num;
	  }
	  tokadd(p, c);
	  seen_e++;
	  is_float++;
	  nondigit = c;
	  c = nextc(p);
	  if (c != '-' && c != '+') continue;
	  tokadd(p, c);
	  nondigit = c;
	  break;

	case '_':	/* `_' in number just ignored */
	  if (nondigit) goto decode_num;
	  nondigit = c;
	  break;

	default:
	  goto decode_num;
	}
	c = nextc(p);
      }

    decode_num:
      pushback(p, c);
      if (nondigit) {
      trailing_uc:
	yyerror_i(p, "trailing `%c' in number", nondigit);
      }
      tokfix(p);
      if (is_float) {
	double d;
	char *endp;

	errno = 0;
	d = strtod(tok(p), &endp);
	if (d == 0 && endp == tok(p)) {
	  yywarning_s(p, "corrupted float value %s", tok(p));
	}
	else if (errno == ERANGE) {
	  yywarning_s(p, "float %s out of range", tok(p));
	  errno = 0;
	}
	yylval.nd = new_float(p, tok(p));
	return tFLOAT;
      }
      yylval.nd = new_int(p, tok(p), 10);
      return tINTEGER;
    }

  case ')':
  case ']':
    p->paren_nest--;
  case '}':
    COND_LEXPOP();
    CMDARG_LEXPOP();
    if (c == ')')
      p->lstate = EXPR_ENDFN;
    else
      p->lstate = EXPR_ENDARG;
    return c;

  case ':':
    c = nextc(p);
    if (c == ':') {
      if (IS_BEG() || p->lstate == EXPR_CLASS || IS_SPCARG(-1)) {
	p->lstate = EXPR_BEG;
	return tCOLON3;
      }
      p->lstate = EXPR_DOT;
      return tCOLON2;
    }
    if (IS_END() || ISSPACE(c)) {
      pushback(p, c);
      p->lstate = EXPR_BEG;
      return ':';
    }
    pushback(p, c);
    p->lstate = EXPR_FNAME;
    return tSYMBEG;

  case '/':
    if (IS_BEG()) {
      p->lex_strterm = new_strterm(p, str_regexp, '/', 0);
      return tREGEXP_BEG;
    }
    if ((c = nextc(p)) == '=') {
      yylval.id = intern_c('/');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    pushback(p, c);
    if (IS_SPCARG(c)) {
      p->lex_strterm = new_strterm(p, str_regexp, '/', 0);
      return tREGEXP_BEG;
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    return '/';

  case '^':
    if ((c = nextc(p)) == '=') {
      yylval.id = intern_c('^');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    pushback(p, c);
    return '^';

  case ';':
    p->lstate = EXPR_BEG;
    return ';';

  case ',':
    p->lstate = EXPR_BEG;
    return ',';

  case '~':
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      if ((c = nextc(p)) != '@') {
	pushback(p, c);
      }
      p->lstate = EXPR_ARG;
    }
    else {
      p->lstate = EXPR_BEG;
    }
    return '~';

  case '(':
    if (IS_BEG()) {
      c = tLPAREN;
    }
    else if (IS_SPCARG(-1)) {
      c = tLPAREN_ARG;
    }
    p->paren_nest++;
    COND_PUSH(0);
    CMDARG_PUSH(0);
    p->lstate = EXPR_BEG;
    return c;

  case '[':
    p->paren_nest++;
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
      if ((c = nextc(p)) == ']') {
	if ((c = nextc(p)) == '=') {
	  return tASET;
	}
	pushback(p, c);
	return tAREF;
      }
      pushback(p, c);
      return '[';
    }
    else if (IS_BEG()) {
      c = tLBRACK;
    }
    else if (IS_ARG() && space_seen) {
      c = tLBRACK;
    }
    p->lstate = EXPR_BEG;
    COND_PUSH(0);
    CMDARG_PUSH(0);
    return c;

  case '{':
    if (p->lpar_beg && p->lpar_beg == p->paren_nest) {
      p->lstate = EXPR_BEG;
      p->lpar_beg = 0;
      p->paren_nest--;
      COND_PUSH(0);
      CMDARG_PUSH(0);
      return tLAMBEG;
    }
    if (IS_ARG() || p->lstate == EXPR_END || p->lstate == EXPR_ENDFN)
      c = '{';          /* block (primary) */
    else if (p->lstate == EXPR_ENDARG)
      c = tLBRACE_ARG;  /* block (expr) */
    else
      c = tLBRACE;      /* hash */
    COND_PUSH(0);
    CMDARG_PUSH(0);
    p->lstate = EXPR_BEG;
    return c;

  case '\\':
    c = nextc(p);
    if (c == '\n') {
      p->lineno++;
      p->column = 0;
      space_seen = 1;
      goto retry; /* skip \\n */
    }
    pushback(p, c);
    return '\\';

  case '%':
    if (IS_BEG()) {
      int term;
      int paren;

      c = nextc(p);
    quotation:
      if (c == -1 || !ISALNUM(c)) {
	term = c;
	c = 'Q';
      }
      else {
	term = nextc(p);
	if (isalnum(term)) {
	  yyerror(p, "unknown type of %string");
	  return 0;
	}
      }
      if (c == -1 || term == -1) {
	yyerror(p, "unterminated quoted string meets end of file");
	return 0;
      }
      paren = term;
      if (term == '(') term = ')';
      else if (term == '[') term = ']';
      else if (term == '{') term = '}';
      else if (term == '<') term = '>';
      else paren = 0;

      switch (c) {
      case 'Q':
	p->lex_strterm = new_strterm(p, str_dquote, term, paren);
	return tSTRING_BEG;

      case 'q':
	p->lex_strterm = new_strterm(p, str_squote, term, paren);
	return parse_string(p);

      case 'W':
	p->lex_strterm = new_strterm(p, str_dword, term, paren);
	return tWORDS_BEG;

      case 'w':
	p->lex_strterm = new_strterm(p, str_sword, term, paren);
	return tWORDS_BEG;

      case 'x':
	p->lex_strterm = new_strterm(p, str_xquote, term, paren);
	return tXSTRING_BEG;

      case 'r':
	p->lex_strterm = new_strterm(p, str_regexp, term, paren);
	return tREGEXP_BEG;

      case 's':
	p->lex_strterm = new_strterm(p, str_ssym, term, paren);
	return tSYMBEG;

      case 'I':
	p->lex_strterm = new_strterm(p, str_dsymbols, term, paren);
	return tSYMBOLS_BEG;

      case 'i':
	p->lex_strterm = new_strterm(p, str_ssymbols, term, paren);
	return tSYMBOLS_BEG;

      default:
	yyerror(p, "unknown type of %string");
	return 0;
      }
    }
    if ((c = nextc(p)) == '=') {
      yylval.id = intern_c('%');
      p->lstate = EXPR_BEG;
      return tOP_ASGN;
    }
    if (IS_SPCARG(c)) {
      goto quotation;
    }
    if (p->lstate == EXPR_FNAME || p->lstate == EXPR_DOT) {
      p->lstate = EXPR_ARG;
    } else {
      p->lstate = EXPR_BEG;
    }
    pushback(p, c);
    return '%';

  case '$':
    p->lstate = EXPR_END;
    token_column = newtok(p);
    c = nextc(p);
    if (c == -1) {
      yyerror(p, "incomplete global variable syntax");
      return 0;
    }
    switch (c) {
    case '_':     /* $_: last read line string */
      c = nextc(p);
      if (c != -1 && identchar(c)) { /* if there is more after _ it is a variable */
        tokadd(p, '$');
        tokadd(p, c);
        break;
      }
      pushback(p, c);
      c = '_';
      /* fall through */
    case '~':     /* $~: match-data */
    case '*':     /* $*: argv */
    case '$':     /* $$: pid */
    case '?':     /* $?: last status */
    case '!':     /* $!: error string */
    case '@':     /* $@: error position */
    case '/':     /* $/: input record separator */
    case '\\':    /* $\: output record separator */
    case ';':     /* $;: field separator */
    case ',':     /* $,: output field separator */
    case '.':     /* $.: last read line number */
    case '=':     /* $=: ignorecase */
    case ':':     /* $:: load path */
    case '<':     /* $<: reading filename */
    case '>':     /* $>: default output handle */
    case '\"':    /* $": already loaded files */
      tokadd(p, '$');
      tokadd(p, c);
      tokfix(p);
      yylval.id = intern(tok(p));
      return tGVAR;

    case '-':
      tokadd(p, '$');
      tokadd(p, c);
      c = nextc(p);
      pushback(p, c);
    gvar:
      tokfix(p);
      yylval.id = intern(tok(p));
      return tGVAR;

    case '&':     /* $&: last match */
    case '`':     /* $`: string before last match */
    case '\'':    /* $': string after last match */
    case '+':     /* $+: string matches last pattern */
      if (last_state == EXPR_FNAME) {
	tokadd(p, '$');
	tokadd(p, c);
	goto gvar;
      }
      yylval.nd = new_back_ref(p, c);
      return tBACK_REF;

    case '1': case '2': case '3':
    case '4': case '5': case '6':
    case '7': case '8': case '9':
      do {
	tokadd(p, c);
	c = nextc(p);
      } while (c != -1 && isdigit(c));
      pushback(p, c);
      if (last_state == EXPR_FNAME) goto gvar;
      tokfix(p);
      yylval.nd = new_nth_ref(p, atoi(tok(p)));
      return tNTH_REF;

    default:
      if (!identchar(c)) {
	pushback(p,  c);
	return '$';
      }
    case '0':
      tokadd(p, '$');
    }
    break;

  case '@':
    c = nextc(p);
    token_column = newtok(p);
    tokadd(p, '@');
    if (c == '@') {
      tokadd(p, '@');
      c = nextc(p);
    }
    if (c == -1) {
      if (p->bidx == 1) {
	yyerror(p, "incomplete instance variable syntax");
      }
      else {
	yyerror(p, "incomplete class variable syntax");
      }
      return 0;
    }
    else if (isdigit(c)) {
      if (p->bidx == 1) {
	yyerror_i(p, "`@%c' is not allowed as an instance variable name", c);
      }
      else {
	yyerror_i(p, "`@@%c' is not allowed as a class variable name", c);
      }
      return 0;
    }
    if (!identchar(c)) {
      pushback(p, c);
      return '@';
    }
    break;

  case '_':
    token_column = newtok(p);
    break;

  default:
    if (!identchar(c)) {
      yyerror_i(p,  "Invalid char `\\x%02X' in expression", c);
      goto retry;
    }

    token_column = newtok(p);
    break;
  }

  do {
    tokadd(p, c);
    c = nextc(p);
    if (c < 0) break;
  } while (identchar(c));
  if (token_column == 0 && toklen(p) == 7 && (c < 0 || c == '\n') &&
      strncmp(tok(p), "__END__", toklen(p)) == 0)
    return -1;

  switch (tok(p)[0]) {
  case '@': case '$':
    pushback(p, c);
    break;
  default:
    if ((c == '!' || c == '?') && !peek(p, '=')) {
      tokadd(p, c);
    }
    else {
      pushback(p, c);
    }
  }
  tokfix(p);
  {
    int result = 0;

    last_state = p->lstate;
    switch (tok(p)[0]) {
    case '$':
      p->lstate = EXPR_END;
      result = tGVAR;
      break;
    case '@':
      p->lstate = EXPR_END;
      if (tok(p)[1] == '@')
	result = tCVAR;
      else
	result = tIVAR;
      break;

    default:
      if (toklast(p) == '!' || toklast(p) == '?') {
	result = tFID;
      }
      else {
	if (p->lstate == EXPR_FNAME) {
	  if ((c = nextc(p)) == '=' && !peek(p, '~') && !peek(p, '>') &&
	      (!peek(p, '=') || (peek_n(p, '>', 1)))) {
	    result = tIDENTIFIER;
	    tokadd(p, c);
	    tokfix(p);
	  }
	  else {
	    pushback(p, c);
	  }
	}
	if (result == 0 && isupper((int)(unsigned char)tok(p)[0])) {
	  result = tCONSTANT;
	}
	else {
	  result = tIDENTIFIER;
	}
      }

      if (IS_LABEL_POSSIBLE()) {
	if (IS_LABEL_SUFFIX(0)) {
	  p->lstate = EXPR_BEG;
	  nextc(p);
	  tokfix(p);
	  yylval.id = intern(tok(p));
	  return tLABEL;
	}
      }
      if (p->lstate != EXPR_DOT) {
	const struct kwtable *kw;

	/* See if it is a reserved word.  */
	kw = mrb_reserved_word(tok(p), toklen(p));
	if (kw) {
	  enum mrb_lex_state_enum state = p->lstate;
	  p->lstate = kw->state;
	  if (state == EXPR_FNAME) {
	    yylval.id = intern(kw->name);
	    return kw->id[0];
	  }
	  if (p->lstate == EXPR_BEG) {
	    p->cmd_start = TRUE;
	  }
	  if (kw->id[0] == keyword_do) {
	    if (p->lpar_beg && p->lpar_beg == p->paren_nest) {
	      p->lpar_beg = 0;
	      p->paren_nest--;
	      return keyword_do_LAMBDA;
	    }
	    if (COND_P()) return keyword_do_cond;
	    if (CMDARG_P() && state != EXPR_CMDARG)
	      return keyword_do_block;
	    if (state == EXPR_ENDARG || state == EXPR_BEG)
	      return keyword_do_block;
	    return keyword_do;
	  }
	  if (state == EXPR_BEG || state == EXPR_VALUE)
	    return kw->id[0];
	  else {
	    if (kw->id[0] != kw->id[1])
	      p->lstate = EXPR_BEG;
	    return kw->id[1];
	  }
	}
      }

      if (IS_BEG() || p->lstate == EXPR_DOT || IS_ARG()) {
	if (cmd_state) {
	  p->lstate = EXPR_CMDARG;
	}
	else {
	  p->lstate = EXPR_ARG;
	}
      }
      else if (p->lstate == EXPR_FNAME) {
	p->lstate = EXPR_ENDFN;
      }
      else {
	p->lstate = EXPR_END;
      }
    }
    {
      mrb_sym ident = intern(tok(p));

      yylval.id = ident;
#if 0
      if (last_state != EXPR_DOT && islower(tok(p)[0]) && lvar_defined(ident)) {
	p->lstate = EXPR_END;
      }
#endif
    }
    return result;
  }
}

static int
yylex(void *lval, parser_state *p)
{
  int t;

  p->ylval = lval;
  t = parser_yylex(p);

  return t;
}

static void
parser_init_cxt(parser_state *p, mrbc_context *cxt)
{
  if (!cxt) return;
  if (cxt->lineno) p->lineno = cxt->lineno;
  if (cxt->filename) mrb_parser_set_filename(p, cxt->filename);
  if (cxt->syms) {
    int i;

    p->locals = cons(0,0);
    for (i=0; i<cxt->slen; i++) {
      local_add_f(p, cxt->syms[i]);
    }
  }
  p->capture_errors = cxt->capture_errors;
  if (cxt->partial_hook) {
    p->cxt = cxt;
  }
}

static void
parser_update_cxt(parser_state *p, mrbc_context *cxt)
{
  node *n, *n0;
  int i = 0;

  if (!cxt) return;
  if ((int)(intptr_t)p->tree->car != NODE_SCOPE) return;
  n0 = n = p->tree->cdr->car;
  while (n) {
    i++;
    n = n->cdr;
  }
  cxt->syms = (mrb_sym *)mrb_realloc(p->mrb, cxt->syms, i*sizeof(mrb_sym));
  cxt->slen = i;
  for (i=0, n=n0; n; i++,n=n->cdr) {
    cxt->syms[i] = sym(n->car);
  }
}

void codedump_all(mrb_state*, int);
void parser_dump(mrb_state *mrb, node *tree, int offset);

void
mrb_parser_parse(parser_state *p, mrbc_context *c)
{
  if (setjmp(p->jmp) != 0) {
    yyerror(p, "memory allocation error");
    p->nerr++;
    p->tree = 0;
    return;
  }

  p->cmd_start = TRUE;
  p->in_def = p->in_single = FALSE;
  p->nerr = p->nwarn = 0;
  p->lex_strterm = NULL;

  parser_init_cxt(p, c);
  yyparse(p);
  if (!p->tree) {
    p->tree = new_nil(p);
  }
  parser_update_cxt(p, c);
  if (c && c->dump_result) {
    parser_dump(p->mrb, p->tree, 0);
  }
}

parser_state*
mrb_parser_new(mrb_state *mrb)
{
  mrb_pool *pool;
  parser_state *p;
  static const parser_state parser_state_zero = { 0 };

  pool = mrb_pool_open(mrb);
  if (!pool) return 0;
  p = (parser_state *)mrb_pool_alloc(pool, sizeof(parser_state));
  if (!p) return 0;

  *p = parser_state_zero;
  p->mrb = mrb;
  p->pool = pool;
  p->in_def = p->in_single = 0;

  p->s = p->send = NULL;
#ifdef ENABLE_STDIO
  p->f = NULL;
#endif

  p->cmd_start = TRUE;
  p->in_def = p->in_single = FALSE;

  p->capture_errors = 0;
  p->lineno = 1;
  p->column = 0;
#if defined(PARSER_TEST) || defined(PARSER_DEBUG)
  yydebug = 1;
#endif

  p->lex_strterm = NULL;
  p->heredocs = p->parsing_heredoc = NULL;

  p->current_filename_index = -1;
  p->filename_table = NULL;
  p->filename_table_length = 0;

  return p;
}

void
mrb_parser_free(parser_state *p) {
  mrb_pool_close(p->pool);
}

mrbc_context*
mrbc_context_new(mrb_state *mrb)
{
  mrbc_context *c;

  c = (mrbc_context *)mrb_calloc(mrb, 1, sizeof(mrbc_context));
  return c;
}

void
mrbc_context_free(mrb_state *mrb, mrbc_context *cxt)
{
  mrb_free(mrb, cxt->syms);
  mrb_free(mrb, cxt);
}

const char*
mrbc_filename(mrb_state *mrb, mrbc_context *c, const char *s)
{
  if (s) {
    int len = strlen(s);
    char *p = (char *)mrb_alloca(mrb, len + 1);

    memcpy(p, s, len + 1);
    c->filename = p;
  }
  return c->filename;
}

void
mrbc_partial_hook(mrb_state *mrb, mrbc_context *c, int (*func)(struct mrb_parser_state*), void *data)
{
  c->partial_hook = func;
  c->partial_data = data;
}

void
mrb_parser_set_filename(struct mrb_parser_state *p, const char *f)
{
  mrb_sym sym;
  size_t len;
  size_t i;
  mrb_sym* new_table;

  sym = mrb_intern_cstr(p->mrb, f);
  p->filename = mrb_sym2name_len(p->mrb, sym, &len);
  p->lineno = (p->filename_table_length > 0)? 0 : 1;
  
  for(i = 0; i < p->filename_table_length; ++i) {
    if(p->filename_table[i] == sym) {
      p->current_filename_index = i;
      return;
    }
  }

  p->current_filename_index = p->filename_table_length++;

  new_table = parser_palloc(p, sizeof(mrb_sym) * p->filename_table_length);
  if (p->filename_table) {
    memcpy(new_table, p->filename_table, sizeof(mrb_sym) * p->filename_table_length);
  }
  p->filename_table = new_table;
  p->filename_table[p->filename_table_length - 1] = sym;
}

char const* mrb_parser_get_filename(struct mrb_parser_state* p, uint16_t idx) {
  if (idx >= p->filename_table_length) { return NULL; }
  else {
    size_t len;
    return mrb_sym2name_len(p->mrb, p->filename_table[idx], &len);
  }
}

#ifdef ENABLE_STDIO
parser_state*
mrb_parse_file(mrb_state *mrb, FILE *f, mrbc_context *c)
{
  parser_state *p;

  p = mrb_parser_new(mrb);
  if (!p) return 0;
  p->s = p->send = NULL;
  p->f = f;

  mrb_parser_parse(p, c);
  return p;
}
#endif

parser_state*
mrb_parse_nstring(mrb_state *mrb, const char *s, int len, mrbc_context *c)
{
  parser_state *p;

  p = mrb_parser_new(mrb);
  if (!p) return 0;
  p->s = s;
  p->send = s + len;

  mrb_parser_parse(p, c);
  return p;
}

parser_state*
mrb_parse_string(mrb_state *mrb, const char *s, mrbc_context *c)
{
  return mrb_parse_nstring(mrb, s, strlen(s), c);
}

static mrb_value
load_exec(mrb_state *mrb, parser_state *p, mrbc_context *c)
{
  struct RClass *target = mrb->object_class;
  struct RProc *proc;
  int n;
  mrb_value v;

  if (!p) {
    return mrb_undef_value();
  }
  if (!p->tree || p->nerr) {
    if (p->capture_errors) {
      char buf[256];

      n = snprintf(buf, sizeof(buf), "line %d: %s\n",
      p->error_buffer[0].lineno, p->error_buffer[0].message);
      mrb->exc = mrb_obj_ptr(mrb_exc_new(mrb, E_SYNTAX_ERROR, buf, n));
      mrb_parser_free(p);
      return mrb_undef_value();
    }
    else {
      static const char msg[] = "syntax error";
      mrb->exc = mrb_obj_ptr(mrb_exc_new(mrb, E_SYNTAX_ERROR, msg, sizeof(msg) - 1));
      mrb_parser_free(p);
      return mrb_undef_value();
    }
  }
  n = mrb_generate_code(mrb, p);
  mrb_parser_free(p);
  if (n < 0) {
    static const char msg[] = "codegen error";
    mrb->exc = mrb_obj_ptr(mrb_exc_new(mrb, E_SCRIPT_ERROR, msg, sizeof(msg) - 1));
    return mrb_nil_value();
  }
  if (c) {
    if (c->dump_result) codedump_all(mrb, n);
    if (c->no_exec) return mrb_fixnum_value(n);
    if (c->target_class) {
      target = c->target_class;
    }
  }
  proc = mrb_proc_new(mrb, mrb->irep[n]);
  proc->target_class = target;
  if (mrb->c->ci) {
    mrb->c->ci->target_class = target;
  }
  v = mrb_run(mrb, proc, mrb_top_self(mrb));
  if (mrb->exc) return mrb_nil_value();
  return v;
}

#ifdef ENABLE_STDIO
mrb_value
mrb_load_file_cxt(mrb_state *mrb, FILE *f, mrbc_context *c)
{
  return load_exec(mrb, mrb_parse_file(mrb, f, c), c);
}

mrb_value
mrb_load_file(mrb_state *mrb, FILE *f)
{
  return mrb_load_file_cxt(mrb, f, NULL);
}
#endif

mrb_value
mrb_load_nstring_cxt(mrb_state *mrb, const char *s, int len, mrbc_context *c)
{
  return load_exec(mrb, mrb_parse_nstring(mrb, s, len, c), c);
}

mrb_value
mrb_load_nstring(mrb_state *mrb, const char *s, int len)
{
  return mrb_load_nstring_cxt(mrb, s, len, NULL);
}

mrb_value
mrb_load_string_cxt(mrb_state *mrb, const char *s, mrbc_context *c)
{
  return mrb_load_nstring_cxt(mrb, s, strlen(s), c);
}

mrb_value
mrb_load_string(mrb_state *mrb, const char *s)
{
  return mrb_load_string_cxt(mrb, s, NULL);
}

#ifdef ENABLE_STDIO

static void
dump_prefix(int offset)
{
  while (offset--) {
    putc(' ', stdout);
    putc(' ', stdout);
  }
}

static void
dump_recur(mrb_state *mrb, node *tree, int offset)
{
  while (tree) {
    parser_dump(mrb, tree->car, offset);
    tree = tree->cdr;
  }
}

#endif

void
parser_dump(mrb_state *mrb, node *tree, int offset)
{
#ifdef ENABLE_STDIO
  int n;

  if (!tree) return;
 again:
  dump_prefix(offset);
  n = (int)(intptr_t)tree->car;
  tree = tree->cdr;
  switch (n) {
  case NODE_BEGIN:
    printf("NODE_BEGIN:\n");
    dump_recur(mrb, tree, offset+1);
    break;

  case NODE_RESCUE:
    printf("NODE_RESCUE:\n");
    if (tree->car) {
      dump_prefix(offset+1);
      printf("body:\n");
      parser_dump(mrb, tree->car, offset+2);
    }
    tree = tree->cdr;
    if (tree->car) {
      node *n2 = tree->car;

      dump_prefix(offset+1);
      printf("rescue:\n");
      while (n2) {
	node *n3 = n2->car;
	if (n3->car) {
	  dump_prefix(offset+2);
	  printf("handle classes:\n");
	  dump_recur(mrb, n3->car, offset+3);
	}
	if (n3->cdr->car) {
	  dump_prefix(offset+2);
	  printf("exc_var:\n");
	  parser_dump(mrb, n3->cdr->car, offset+3);
	}
	if (n3->cdr->cdr->car) {
	  dump_prefix(offset+2);
	  printf("rescue body:\n");
	  parser_dump(mrb, n3->cdr->cdr->car, offset+3);
	}
	n2 = n2->cdr;
      }
    }
    tree = tree->cdr;
    if (tree->car) {
      dump_prefix(offset+1);
      printf("else:\n");
      parser_dump(mrb, tree->car, offset+2);
    }
    break;

  case NODE_ENSURE:
    printf("NODE_ENSURE:\n");
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->car, offset+2);
    dump_prefix(offset+1);
    printf("ensure:\n");
    parser_dump(mrb, tree->cdr->cdr, offset+2);
    break;

  case NODE_LAMBDA:
    printf("NODE_BLOCK:\n");
    goto block;

  case NODE_BLOCK:
  block:
    printf("NODE_BLOCK:\n");
    tree = tree->cdr;
    if (tree->car) {
      node *n = tree->car;

      if (n->car) {
	dump_prefix(offset+1);
	printf("mandatory args:\n");
	dump_recur(mrb, n->car, offset+2);
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("optional args:\n");
	{
	  node *n2 = n->car;

	  while (n2) {
	    dump_prefix(offset+2);
	    printf("%s=", mrb_sym2name(mrb, sym(n2->car->car)));
	    parser_dump(mrb, n2->car->cdr, 0);
	    n2 = n2->cdr;
	  }
	}
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("rest=*%s\n", mrb_sym2name(mrb, sym(n->car)));
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("post mandatory args:\n");
	dump_recur(mrb, n->car, offset+2);
      }
      n = n->cdr;
      if (n) {
	dump_prefix(offset+1);
	printf("blk=&%s\n", mrb_sym2name(mrb, sym(n)));
      }
    }
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->cdr->car, offset+2);
    break;

  case NODE_IF:
    printf("NODE_IF:\n");
    dump_prefix(offset+1);
    printf("cond:\n");
    parser_dump(mrb, tree->car, offset+2);
    dump_prefix(offset+1);
    printf("then:\n");
    parser_dump(mrb, tree->cdr->car, offset+2);
    if (tree->cdr->cdr->car) {
      dump_prefix(offset+1);
      printf("else:\n");
      parser_dump(mrb, tree->cdr->cdr->car, offset+2);
    }
    break;

  case NODE_AND:
    printf("NODE_AND:\n");
    parser_dump(mrb, tree->car, offset+1);
    parser_dump(mrb, tree->cdr, offset+1);
    break;

  case NODE_OR:
    printf("NODE_OR:\n");
    parser_dump(mrb, tree->car, offset+1);
    parser_dump(mrb, tree->cdr, offset+1);
    break;

  case NODE_CASE:
    printf("NODE_CASE:\n");
    if (tree->car) {
      parser_dump(mrb, tree->car, offset+1);
    }
    tree = tree->cdr;
    while (tree) {
      dump_prefix(offset+1);
      printf("case:\n");
      dump_recur(mrb, tree->car->car, offset+2);
      dump_prefix(offset+1);
      printf("body:\n");
      parser_dump(mrb, tree->car->cdr, offset+2);
      tree = tree->cdr;
    }
    break;

  case NODE_WHILE:
    printf("NODE_WHILE:\n");
    dump_prefix(offset+1);
    printf("cond:\n");
    parser_dump(mrb, tree->car, offset+2);
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->cdr, offset+2);
    break;

  case NODE_UNTIL:
    printf("NODE_UNTIL:\n");
    dump_prefix(offset+1);
    printf("cond:\n");
    parser_dump(mrb, tree->car, offset+2);
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->cdr, offset+2);
    break;

  case NODE_FOR:
    printf("NODE_FOR:\n");
    dump_prefix(offset+1);
    printf("var:\n");
    {
      node *n2 = tree->car;

      if (n2->car) {
	dump_prefix(offset+2);
	printf("pre:\n");
	dump_recur(mrb, n2->car, offset+3);
      }
      n2 = n2->cdr;
      if (n2) {
	if (n2->car) {
	  dump_prefix(offset+2);
	  printf("rest:\n");
	  parser_dump(mrb, n2->car, offset+3);
	}
	n2 = n2->cdr;
	if (n2) {
	  if (n2->car) {
	    dump_prefix(offset+2);
	    printf("post:\n");
	    dump_recur(mrb, n2->car, offset+3);
	  }
	}
      }
    }
    tree = tree->cdr;
    dump_prefix(offset+1);
    printf("in:\n");
    parser_dump(mrb, tree->car, offset+2);
    tree = tree->cdr;
    dump_prefix(offset+1);
    printf("do:\n");
    parser_dump(mrb, tree->car, offset+2);
    break;

  case NODE_SCOPE:
    printf("NODE_SCOPE:\n");
    {
      node *n2 = tree->car;

      if (n2  && (n2->car || n2->cdr)) {
	dump_prefix(offset+1);
	printf("local variables:\n");
	dump_prefix(offset+2);
	while (n2) {
	  if (n2->car) {
	    if (n2 != tree->car) printf(", ");
	    printf("%s", mrb_sym2name(mrb, sym(n2->car)));
	  }
	  n2 = n2->cdr;
	}
	printf("\n");
      }
    }
    tree = tree->cdr;
    offset++;
    goto again;

  case NODE_FCALL:
  case NODE_CALL:
    printf("NODE_CALL:\n");
    parser_dump(mrb, tree->car, offset+1);
    dump_prefix(offset+1);
    printf("method='%s' (%d)\n",
    mrb_sym2name(mrb, sym(tree->cdr->car)),
    (int)(intptr_t)tree->cdr->car);
    tree = tree->cdr->cdr->car;
    if (tree) {
      dump_prefix(offset+1);
      printf("args:\n");
      dump_recur(mrb, tree->car, offset+2);
      if (tree->cdr) {
	dump_prefix(offset+1);
	printf("block:\n");
	parser_dump(mrb, tree->cdr, offset+2);
      }
    }
    break;

  case NODE_DOT2:
    printf("NODE_DOT2:\n");
    parser_dump(mrb, tree->car, offset+1);
    parser_dump(mrb, tree->cdr, offset+1);
    break;

  case NODE_DOT3:
    printf("NODE_DOT3:\n");
    parser_dump(mrb, tree->car, offset+1);
    parser_dump(mrb, tree->cdr, offset+1);
    break;

  case NODE_COLON2:
    printf("NODE_COLON2:\n");
    parser_dump(mrb, tree->car, offset+1);
    dump_prefix(offset+1);
    printf("::%s\n", mrb_sym2name(mrb, sym(tree->cdr)));
    break;

  case NODE_COLON3:
    printf("NODE_COLON3:\n");
    dump_prefix(offset+1);
    printf("::%s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_ARRAY:
    printf("NODE_ARRAY:\n");
    dump_recur(mrb, tree, offset+1);
    break;

  case NODE_HASH:
    printf("NODE_HASH:\n");
    while (tree) {
      dump_prefix(offset+1);
      printf("key:\n");
      parser_dump(mrb, tree->car->car, offset+2);
      dump_prefix(offset+1);
      printf("value:\n");
      parser_dump(mrb, tree->car->cdr, offset+2);
      tree = tree->cdr;
    }
    break;

  case NODE_SPLAT:
    printf("NODE_SPLAT:\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_ASGN:
    printf("NODE_ASGN:\n");
    dump_prefix(offset+1);
    printf("lhs:\n");
    parser_dump(mrb, tree->car, offset+2);
    dump_prefix(offset+1);
    printf("rhs:\n");
    parser_dump(mrb, tree->cdr, offset+2);
    break;

  case NODE_MASGN:
    printf("NODE_MASGN:\n");
    dump_prefix(offset+1);
    printf("mlhs:\n");
    {
      node *n2 = tree->car;

      if (n2->car) {
	dump_prefix(offset+2);
	printf("pre:\n");
	dump_recur(mrb, n2->car, offset+3);
      }
      n2 = n2->cdr;
      if (n2) {
	if (n2->car) {
	  dump_prefix(offset+2);
	  printf("rest:\n");
	  if (n2->car == (node*)-1) {
	    dump_prefix(offset+2);
	    printf("(empty)\n");
	  }
	  else {
	    parser_dump(mrb, n2->car, offset+3);
	  }
	}
	n2 = n2->cdr;
	if (n2) {
	  if (n2->car) {
	    dump_prefix(offset+2);
	    printf("post:\n");
	    dump_recur(mrb, n2->car, offset+3);
	  }
	}
      }
    }
    dump_prefix(offset+1);
    printf("rhs:\n");
    parser_dump(mrb, tree->cdr, offset+2);
    break;

  case NODE_OP_ASGN:
    printf("NODE_OP_ASGN:\n");
    dump_prefix(offset+1);
    printf("lhs:\n");
    parser_dump(mrb, tree->car, offset+2);
    tree = tree->cdr;
    dump_prefix(offset+1);
    printf("op='%s' (%d)\n", mrb_sym2name(mrb, sym(tree->car)), (int)(intptr_t)tree->car);
    tree = tree->cdr;
    parser_dump(mrb, tree->car, offset+1);
    break;

  case NODE_SUPER:
    printf("NODE_SUPER:\n");
    if (tree) {
      dump_prefix(offset+1);
      printf("args:\n");
      dump_recur(mrb, tree->car, offset+2);
      if (tree->cdr) {
	dump_prefix(offset+1);
	printf("block:\n");
	parser_dump(mrb, tree->cdr, offset+2);
      }
    }
    break;

  case NODE_ZSUPER:
    printf("NODE_ZSUPER\n");
    break;

  case NODE_RETURN:
    printf("NODE_RETURN:\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_YIELD:
    printf("NODE_YIELD:\n");
    dump_recur(mrb, tree, offset+1);
    break;

  case NODE_BREAK:
    printf("NODE_BREAK:\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_NEXT:
    printf("NODE_NEXT:\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_REDO:
    printf("NODE_REDO\n");
    break;

  case NODE_RETRY:
    printf("NODE_RETRY\n");
    break;

  case NODE_LVAR:
    printf("NODE_LVAR %s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_GVAR:
    printf("NODE_GVAR %s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_IVAR:
    printf("NODE_IVAR %s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_CVAR:
    printf("NODE_CVAR %s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_CONST:
    printf("NODE_CONST %s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_MATCH:
    printf("NODE_MATCH:\n");
    dump_prefix(offset + 1);
    printf("lhs:\n");
    parser_dump(mrb, tree->car, offset + 2);
    dump_prefix(offset + 1);
    printf("rhs:\n");
    parser_dump(mrb, tree->cdr, offset + 2);
    break;

  case NODE_BACK_REF:
    printf("NODE_BACK_REF: $%c\n", (int)(intptr_t)tree);
    break;

  case NODE_NTH_REF:
    printf("NODE_NTH_REF: $%d\n", (int)(intptr_t)tree);
    break;

  case NODE_ARG:
    printf("NODE_ARG %s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_BLOCK_ARG:
    printf("NODE_BLOCK_ARG:\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_INT:
    printf("NODE_INT %s base %d\n", (char*)tree->car, (int)(intptr_t)tree->cdr->car);
    break;

  case NODE_FLOAT:
    printf("NODE_FLOAT %s\n", (char*)tree);
    break;

  case NODE_NEGATE:
    printf("NODE_NEGATE\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_STR:
    printf("NODE_STR \"%s\" len %d\n", (char*)tree->car, (int)(intptr_t)tree->cdr);
    break;

  case NODE_DSTR:
    printf("NODE_DSTR\n");
    dump_recur(mrb, tree, offset+1);
    break;

  case NODE_XSTR:
    printf("NODE_XSTR \"%s\" len %d\n", (char*)tree->car, (int)(intptr_t)tree->cdr);
    break;

  case NODE_DXSTR:
    printf("NODE_DXSTR\n");
    dump_recur(mrb, tree, offset+1);
    break;

  case NODE_REGX:
    printf("NODE_REGX /%s/%s\n", (char*)tree->car, (char*)tree->cdr);
    break;

  case NODE_DREGX:
    printf("NODE_DREGX\n");
    dump_recur(mrb, tree->car, offset+1);
    dump_prefix(offset);
    printf("tail: %s\n", (char*)tree->cdr->cdr->car);
    dump_prefix(offset);
    printf("opt: %s\n", (char*)tree->cdr->cdr->cdr);
    break;

  case NODE_SYM:
    printf("NODE_SYM :%s\n", mrb_sym2name(mrb, sym(tree)));
    break;

  case NODE_SELF:
    printf("NODE_SELF\n");
    break;

  case NODE_NIL:
    printf("NODE_NIL\n");
    break;

  case NODE_TRUE:
    printf("NODE_TRUE\n");
    break;

  case NODE_FALSE:
    printf("NODE_FALSE\n");
    break;

  case NODE_ALIAS:
    printf("NODE_ALIAS %s %s:\n",
	    mrb_sym2name(mrb, sym(tree->car)),
	    mrb_sym2name(mrb, sym(tree->cdr)));
    break;

  case NODE_UNDEF:
    printf("NODE_UNDEF");
    {
      node *t = tree;
      while (t) {
	printf(" %s", mrb_sym2name(mrb, sym(t->car)));
	t = t->cdr;
      }
    }
    printf(":\n");
    break;

  case NODE_CLASS:
    printf("NODE_CLASS:\n");
    if (tree->car->car == (node*)0) {
      dump_prefix(offset+1);
      printf(":%s\n", mrb_sym2name(mrb, sym(tree->car->cdr)));
    }
    else if (tree->car->car == (node*)1) {
      dump_prefix(offset+1);
      printf("::%s\n", mrb_sym2name(mrb, sym(tree->car->cdr)));
    }
    else {
      parser_dump(mrb, tree->car->car, offset+1);
      dump_prefix(offset+1);
      printf("::%s\n", mrb_sym2name(mrb, sym(tree->car->cdr)));
    }
    if (tree->cdr->car) {
      dump_prefix(offset+1);
      printf("super:\n");
      parser_dump(mrb, tree->cdr->car, offset+2);
    }
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->cdr->cdr->car->cdr, offset+2);
    break;

  case NODE_MODULE:
    printf("NODE_MODULE:\n");
    if (tree->car->car == (node*)0) {
      dump_prefix(offset+1);
      printf(":%s\n", mrb_sym2name(mrb, sym(tree->car->cdr)));
    }
    else if (tree->car->car == (node*)1) {
      dump_prefix(offset+1);
      printf("::%s\n", mrb_sym2name(mrb, sym(tree->car->cdr)));
    }
    else {
      parser_dump(mrb, tree->car->car, offset+1);
      dump_prefix(offset+1);
      printf("::%s\n", mrb_sym2name(mrb, sym(tree->car->cdr)));
    }
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->cdr->car->cdr, offset+2);
    break;

  case NODE_SCLASS:
    printf("NODE_SCLASS:\n");
    parser_dump(mrb, tree->car, offset+1);
    dump_prefix(offset+1);
    printf("body:\n");
    parser_dump(mrb, tree->cdr->car->cdr, offset+2);
    break;

  case NODE_DEF:
    printf("NODE_DEF:\n");
    dump_prefix(offset+1);
    printf("%s\n", mrb_sym2name(mrb, sym(tree->car)));
    tree = tree->cdr;
    {
      node *n2 = tree->car;

      if (n2 && (n2->car || n2->cdr)) {
	dump_prefix(offset+1);
	printf("local variables:\n");
	dump_prefix(offset+2);
	while (n2) {
	  if (n2->car) {
	    if (n2 != tree->car) printf(", ");
	    printf("%s", mrb_sym2name(mrb, sym(n2->car)));
	  }
	  n2 = n2->cdr;
	}
	printf("\n");
      }
    }
    tree = tree->cdr;
    if (tree->car) {
      node *n = tree->car;

      if (n->car) {
	dump_prefix(offset+1);
	printf("mandatory args:\n");
	dump_recur(mrb, n->car, offset+2);
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("optional args:\n");
	{
	  node *n2 = n->car;

	  while (n2) {
	    dump_prefix(offset+2);
	    printf("%s=", mrb_sym2name(mrb, sym(n2->car->car)));
	    parser_dump(mrb, n2->car->cdr, 0);
	    n2 = n2->cdr;
	  }
	}
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("rest=*%s\n", mrb_sym2name(mrb, sym(n->car)));
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("post mandatory args:\n");
	dump_recur(mrb, n->car, offset+2);
      }
      n = n->cdr;
      if (n) {
	dump_prefix(offset+1);
	printf("blk=&%s\n", mrb_sym2name(mrb, sym(n)));
      }
    }
    parser_dump(mrb, tree->cdr->car, offset+1);
    break;

  case NODE_SDEF:
    printf("NODE_SDEF:\n");
    parser_dump(mrb, tree->car, offset+1);
    tree = tree->cdr;
    dump_prefix(offset+1);
    printf(":%s\n", mrb_sym2name(mrb, sym(tree->car)));
    tree = tree->cdr->cdr;
    if (tree->car) {
      node *n = tree->car;

      if (n->car) {
	dump_prefix(offset+1);
	printf("mandatory args:\n");
	dump_recur(mrb, n->car, offset+2);
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("optional args:\n");
	{
	  node *n2 = n->car;

	  while (n2) {
	    dump_prefix(offset+2);
	    printf("%s=", mrb_sym2name(mrb, sym(n2->car->car)));
	    parser_dump(mrb, n2->car->cdr, 0);
	    n2 = n2->cdr;
	  }
	}
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("rest=*%s\n", mrb_sym2name(mrb, sym(n->car)));
      }
      n = n->cdr;
      if (n->car) {
	dump_prefix(offset+1);
	printf("post mandatory args:\n");
	dump_recur(mrb, n->car, offset+2);
      }
      n = n->cdr;
      if (n) {
	dump_prefix(offset+1);
	printf("blk=&%s\n", mrb_sym2name(mrb, sym(n)));
      }
    }
    tree = tree->cdr;
    parser_dump(mrb, tree->car, offset+1);
    break;

  case NODE_POSTEXE:
    printf("NODE_POSTEXE:\n");
    parser_dump(mrb, tree, offset+1);
    break;

  case NODE_HEREDOC:
    printf("NODE_HEREDOC:\n");
    parser_dump(mrb, ((parser_heredoc_info*)tree)->doc, offset+1);
    break;

  default:
    printf("node type: %d (0x%x)\n", (int)n, (int)n);
    break;
  }
#endif
}
