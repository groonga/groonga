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
#include "groonga_in.h"
#include "db.h"
#include "hash.h"
#include "pat.h"
#include "ii.h"
#include "ql.h"
#include "token.h"
#include "proc.h"
#include <string.h>
#include <float.h>

#define NEXT_ADDR(p) (((byte *)(p)) + sizeof *(p))

#define DB_OBJ(obj) ((grn_db_obj *)obj)

#define WITH_NORMALIZE(table,key,key_size,block) {\
  if ((table)->obj.header.flags & GRN_OBJ_KEY_NORMALIZE) {\
    grn_str *nstr;\
    if ((nstr = grn_str_open(ctx, key, key_size, GRN_STR_NORMALIZE))) { \
      char *key = nstr->norm;\
      unsigned key_size = nstr->norm_blen;\
      block\
      grn_str_close(ctx, nstr);\
    }\
  } else {\
    block\
  }\
}

#define GRN_INT8_SET(ctx,obj,val) do {\
  int8_t _val = (int8_t)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int8_t));\
} while (0)
#define GRN_UINT8_SET(ctx,obj,val) do {\
  uint8_t _val = (uint8_t)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(uint8_t));\
} while (0)
#define GRN_INT16_SET(ctx,obj,val) do {\
  int16_t _val = (int16_t)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int16_t));\
} while (0)
#define GRN_UINT16_SET(ctx,obj,val) do {\
  uint16_t _val = (uint16_t)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(uint16_t));\
} while (0)

struct _grn_db {
  grn_db_obj obj;
  grn_pat *keys;
  grn_ja *specs;
  grn_tiny_array values;
  grn_critical_section lock;
};

static grn_rc grn_db_obj_init(grn_ctx *ctx, grn_obj *db, grn_id id, grn_db_obj *obj);

inline static void
gen_pathname(const char *path, char *buffer, int fno)
{
  size_t len = strlen(path);
  memcpy(buffer, path, len);
  if (fno >= 0) {
    buffer[len] = '.';
    grn_itoh(fno, buffer + len + 1, 7);
  } else {
    buffer[len] = '\0';
  }
}

#define DB_P(s) ((s) && ((grn_db *)s)->obj.header.type == GRN_DB)
#define PERSISTENT_DB_P(s) (((grn_db *)s)->specs)

grn_obj *
grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg)
{
  grn_db *s;
  GRN_API_ENTER;
  if (!path || strlen(path) <= PATH_MAX - 14) {
    if ((s = GRN_MALLOC(sizeof(grn_db)))) {
      grn_tiny_array_init(ctx, &s->values, sizeof(grn_obj *),
                          GRN_TINY_ARRAY_CLEAR|
                          GRN_TINY_ARRAY_THREADSAFE|
                          GRN_TINY_ARRAY_USE_MALLOC);
      if ((s->keys = grn_pat_create(ctx, path, GRN_PAT_MAX_KEY_SIZE, 0,
                                    GRN_OBJ_KEY_VAR_SIZE))) {
        CRITICAL_SECTION_INIT(s->lock);
        GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
        s->obj.db = (grn_obj *)s;
        s->obj.header.domain = GRN_ID_NIL;
        DB_OBJ(&s->obj)->range = GRN_ID_NIL;
        // prepare builtin classes and load builtin plugins.
        if (path) {
          char buffer[PATH_MAX];
          gen_pathname(path, buffer, 0);
          if ((s->specs = grn_ja_create(ctx, buffer, 65536, 0))) {
            grn_ctx_use(ctx, (grn_obj *)s);
            grn_db_init_builtin_types(ctx);
            GRN_API_RETURN((grn_obj *)s);
          } else {
            ERR(GRN_NO_MEMORY_AVAILABLE, "ja create failed");
          }
        } else {
          s->specs = NULL;
          grn_ctx_use(ctx, (grn_obj *)s);
          grn_db_init_builtin_types(ctx);
          GRN_API_RETURN((grn_obj *)s);
        }
        grn_pat_close(ctx, s->keys);
        grn_pat_remove(ctx, path);
      } else {
        ERR(GRN_NO_MEMORY_AVAILABLE, "s->keys create failed");
      }
      grn_tiny_array_fin(&s->values);
      GRN_FREE(s);
    } else {
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_db alloc failed");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "too long path");
  }
  GRN_API_RETURN(NULL);
}

grn_obj *
grn_db_open(grn_ctx *ctx, const char *path)
{
  grn_db *s;
  GRN_API_ENTER;
  if (path && strlen(path) <= PATH_MAX - 14) {
    if ((s = GRN_MALLOC(sizeof(grn_db)))) {
      grn_tiny_array_init(ctx, &s->values, sizeof(grn_obj *),
                          GRN_TINY_ARRAY_CLEAR|
                          GRN_TINY_ARRAY_THREADSAFE|
                          GRN_TINY_ARRAY_USE_MALLOC);
      if ((s->keys = grn_pat_open(ctx, path))) {
        char buffer[PATH_MAX];
        gen_pathname(path, buffer, 0);
        if ((s->specs = grn_ja_open(ctx, buffer))) {
          CRITICAL_SECTION_INIT(s->lock);
          GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
          s->obj.db = (grn_obj *)s;
          s->obj.header.domain = GRN_ID_NIL;
          DB_OBJ(&s->obj)->range = GRN_ID_NIL;
          grn_ctx_use(ctx, (grn_obj *)s);
          grn_db_init_builtin_tokenizers(ctx);
          grn_db_init_builtin_query(ctx);
          GRN_API_RETURN((grn_obj *)s);
        } else {
          ERR(GRN_NO_MEMORY_AVAILABLE, "ja open failed");
        }
        grn_pat_close(ctx, s->keys);
      } else {
        ERR(GRN_NO_MEMORY_AVAILABLE, "s->keys open failed");
      }
      grn_tiny_array_fin(&s->values);
      GRN_FREE(s);
    } else {
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_db alloc failed");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "inappropriate path");
  }
  GRN_API_RETURN(NULL);
}

/* s must be validated by caller */
grn_rc
grn_db_close(grn_ctx *ctx, grn_obj *db)
{
  grn_id id;
  grn_obj **vp;
  grn_db *s = (grn_db *)db;
  if (!s) { return GRN_INVALID_ARGUMENT; }
  GRN_API_ENTER;
  GRN_TINY_ARRAY_EACH(&s->values, 1, grn_pat_curr_id(ctx, s->keys), id, vp, {
    if (*vp) { grn_obj_close(ctx, *vp); }
  });
/* grn_tiny_array_fin should be refined.. */ 
#ifdef WIN32
  {
    grn_tiny_array *a = &s->values;
    CRITICAL_SECTION_FIN(a->lock);
  }
#endif
  grn_tiny_array_fin(&s->values);
  grn_pat_close(ctx, s->keys);
  CRITICAL_SECTION_FIN(s->lock);
  if (s->specs) { grn_ja_close(ctx, s->specs); }
  GRN_FREE(s);
  if (ctx->impl && ctx->impl->db == db) { ctx->impl->db = NULL; }
  GRN_API_RETURN(GRN_SUCCESS);
}

static grn_rc grn_obj_delete_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, int removep);

grn_obj *
grn_ctx_get(grn_ctx *ctx, const char *name, unsigned name_size)
{
  grn_id id;
  grn_obj *obj = NULL;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    return NULL;
  }
  GRN_API_ENTER;
  if (DB_P(db)) {
    grn_db *s = (grn_db *)db;
    if ((id = grn_pat_get(ctx, s->keys, name, name_size, NULL))) {
      obj = grn_ctx_at(ctx, id);
    }
  }
  GRN_API_RETURN(obj);
}

grn_obj *
grn_ctx_db(grn_ctx *ctx)
{
  return (ctx && ctx->impl) ? ctx->impl->db : NULL;
}

#define GRN_PROC_INIT_PREFIX "grn_init_"

grn_rc
grn_db_load(grn_ctx *ctx, const char *path)
{
  grn_id id;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return ctx->rc;
  }
  GRN_API_ENTER;
  if (DB_P(db)) {
    if ((id = grn_dl_open(ctx, path))) {
      grn_proc_init_func *func;
      const char *p;
      char buffer[PATH_MAX];
      for (p = path + strlen(path); path < p; p--) {
        if (*p == PATH_SEPARATOR[0]) { p++; break; }
      }
      strcpy(buffer, GRN_PROC_INIT_PREFIX);
      strcat(buffer, p);
      if ((func = grn_dl_sym(ctx, id, buffer))) {
        ctx->rc = func(ctx, path);
      } else {
        ERR(GRN_INVALID_FORMAT, "init_func not found(%s)", buffer);
      }
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_db_keys(grn_obj *s)
{
  return (grn_obj *)(((grn_db *)s)->keys);
}

static grn_rc
check_name(grn_ctx *ctx, const char *name, unsigned int name_size)
{
  int len;
  const char *name_end = name + name_size;
  if (name < name_end &&
      (*name == GRN_DB_PSEUDO_COLUMN_PREFIX ||
       (unsigned int)(*name - '0') < 10u)) {
    return GRN_INVALID_ARGUMENT;
  }
  while (name < name_end) {
    char c = *name;
    if ((unsigned int)((c | 0x20) - 'a') >= 26u &&
        (unsigned int)(c - '0') >= 10u && c != '_') {
      return GRN_INVALID_ARGUMENT;
    }
    if (!(len = grn_charlen(ctx, name, name_end))) { break; }
    name += len;
  }
  return GRN_SUCCESS;
}

#define CHECK_NAME_ERR() ERR(GRN_INVALID_ARGUMENT, "name can't start with '%c' and 0-9, and contains only 0-9, A-Z, a-z, or _", GRN_DB_PSEUDO_COLUMN_PREFIX)

#define GRN_TYPE_SIZE(type) ((type)->range)

static grn_id grn_obj_register(grn_ctx *ctx, grn_obj *db,
                               const char *name, unsigned name_size);

grn_obj *
grn_type_create(grn_ctx *ctx, const char *name, unsigned name_size,
                grn_obj_flags flags, unsigned int size)
{
  grn_id id;
  struct _grn_type *res = NULL;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  if (check_name(ctx, name, name_size)) {
    CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  }
  id = grn_obj_register(ctx, db, name, name_size);
  if (id && (res = GRN_MALLOC(sizeof(grn_db_obj)))) {
    GRN_DB_OBJ_SET_TYPE(res, GRN_TYPE);
    res->obj.header.flags = flags;
    res->obj.header.domain = GRN_ID_NIL;
    GRN_TYPE_SIZE(&res->obj) = size;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
      // grn_obj_delete(ctx, db, id);
      GRN_FREE(res);
      GRN_API_RETURN(NULL);
    }
  }
  GRN_API_RETURN((grn_obj *)res);
}

typedef struct {
  grn_obj_header header;
  grn_id range;
} grn_obj_spec;

static grn_obj *
grn_type_open(grn_ctx *ctx, grn_obj_spec *spec)
{
  struct _grn_type *res;
  res = GRN_MALLOC(sizeof(struct _grn_type));
  if (res) {
    GRN_DB_OBJ_SET_TYPE(res, GRN_TYPE);
    res->obj.header = spec->header;
    GRN_TYPE_SIZE(&res->obj) = GRN_TYPE_SIZE(spec);
  }
  return (grn_obj *)res;
}

grn_obj *
grn_proc_create(grn_ctx *ctx, const char *name, unsigned name_size,
                const char *path, grn_proc_type type,
                grn_proc_func *init, grn_proc_func *next, grn_proc_func *fin,
                unsigned nvars, grn_expr_var *vars)
{
  grn_proc *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id range;
  int added = 0;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  range = path ? grn_dl_get(ctx, path) : GRN_ID_NIL;
  if (check_name(ctx, name, name_size)) {
    CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  }
  if (name && name_size) {
    grn_db *s = (grn_db *)db;
    if (!(id = grn_pat_add(ctx, s->keys, name, name_size, NULL, &added))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_pat_add failed");
      GRN_API_RETURN(NULL);
    }
    if (!added) {
      grn_proc **vp;
      if ((vp = grn_tiny_array_at(&s->values, id)) && (res = (*vp))) {
        if (res->funcs[PROC_INIT]) {
          ERR(GRN_INVALID_ARGUMENT, "already used name");
          GRN_API_RETURN(NULL);
        }
      } else {
        added = 1;
      }
    }
  } else if (ctx->impl && ctx->impl->values) {
    id = grn_array_add(ctx, ctx->impl->values, NULL) | GRN_OBJ_TMP_OBJECT;
    added = 1;
  }
  if (!res) { res = GRN_MALLOCN(grn_proc, 1); }
  if (res) {
    GRN_DB_OBJ_SET_TYPE(res, GRN_PROC);
    res->obj.db = db;
    res->obj.id = id;
    res->obj.header.domain = GRN_ID_NIL;
    res->obj.header.flags = path ? GRN_OBJ_CUSTOM_NAME : 0;
    res->obj.range = range;
    res->type = type;
    res->funcs[PROC_INIT] = init;
    res->funcs[PROC_NEXT] = next;
    res->funcs[PROC_FIN] = fin;
    GRN_TEXT_INIT(&res->name_buf, 0);
    res->vars = NULL;
    res->nvars = 0;
    if (added) {
      if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
        // grn_obj_delete(ctx, db, id);
        GRN_FREE(res);
        GRN_API_RETURN(NULL);
      }
    }
    while (nvars--) {
      grn_obj *v = grn_expr_add_var(ctx, (grn_obj *)res, vars->name, vars->name_size);
      GRN_OBJ_INIT(v, vars->value.header.type, 0, vars->value.header.domain);
      GRN_TEXT_PUT(ctx, v, GRN_TEXT_VALUE(&vars->value), GRN_TEXT_LEN(&vars->value));
      vars++;
    }
  }
  GRN_API_RETURN((grn_obj *)res);
}

static grn_obj *
grn_proc_open(grn_ctx *ctx, grn_obj_spec *spec)
{
  grn_proc *res;
  res = GRN_MALLOC(sizeof(grn_proc));
  if (res) {
    GRN_DB_OBJ_SET_TYPE(res, GRN_PROC);
    res->funcs[PROC_INIT] = NULL;
    res->funcs[PROC_NEXT] = NULL;
    res->funcs[PROC_FIN] = NULL;
    GRN_TEXT_INIT(&res->name_buf, 0);
    res->vars = NULL;
    res->nvars = 0;
    res->obj.header = spec->header;
    if (res->obj.range) {
      // todo : grn_dl_load should be called.
    }
  }
  return (grn_obj *)res;
}

grn_obj *
grn_expr_alloc(grn_ctx *ctx, grn_obj *expr, grn_id domain, grn_obj_flags flags)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  if (e) {
    if (e->values_curr >= e->values_size) {
      // todo : expand values.
      ERR(GRN_NO_MEMORY_AVAILABLE, "no more e->values");
      return NULL;
    }
    res = &e->values[e->values_curr++];
    if (e->values_curr > e->values_tail) { e->values_tail = e->values_curr; }
    grn_obj_reinit(ctx, res, domain, flags);
  }
  return res;
}

grn_expr_var *
grn_expr_get_vars(grn_ctx *ctx, grn_obj *expr, unsigned *nvars)
{
  grn_expr_var *vars = NULL;
  *nvars = 0;
  if (expr->header.type == GRN_PROC || expr->header.type == GRN_EXPR) {
    grn_id id = DB_OBJ(expr)->id;
    grn_expr *e = (grn_expr *)expr;
    if (id & GRN_OBJ_TMP_OBJECT) {
      vars = e->vars;
      *nvars = e->nvars;
    } else {
      int added = 0;
      grn_expr_vars *vp;
      if (grn_hash_add(ctx, ctx->impl->expr_vars, &id, sizeof(grn_id), (void **)&vp, &added)) {
        if (!vp->vars) {
          grn_expr_var *v, *v0;
          if ((vars = vp->vars = GRN_MALLOCN(grn_expr_var, e->nvars))) {
            uint32_t i;
            for (v0 = e->vars, v = vars, i = e->nvars; i; v0++, v++, i--) {
              v->name = v0->name;
              v->name_size = v0->name_size;
              GRN_OBJ_INIT(&v->value, v0->value.header.type, 0, v0->value.header.domain);
              GRN_TEXT_PUT(ctx, &v->value, GRN_TEXT_VALUE(&v0->value), GRN_TEXT_LEN(&v0->value));
            }
            vp->nvars = e->nvars;
          }
        }
        *nvars = vp->nvars;
        vars = vp->vars;
      }
    }
  }
  return vars;
}

grn_rc
grn_expr_clear_vars(grn_ctx *ctx, grn_obj *expr)
{
  if (expr->header.type == GRN_PROC || expr->header.type == GRN_EXPR) {
    uint32_t i;
    grn_expr_var *v;
    grn_id id = DB_OBJ(expr)->id;
    grn_expr *e = (grn_expr *)expr;
    if (id & GRN_OBJ_TMP_OBJECT) {
      for (v = e->vars, i = e->nvars; i; v++, i--) {
        GRN_OBJ_FIN(ctx, &v->value);
      }
    } else {
      grn_id eid;
      grn_expr_vars *vp;
      if ((eid = grn_hash_get(ctx, ctx->impl->expr_vars, &id, sizeof(grn_id), (void **)&vp))) {
        if (vp->vars) {
          for (v = vp->vars, i = vp->nvars; i; v++, i--) {
            GRN_OBJ_FIN(ctx, &v->value);
          }
          GRN_FREE(vp->vars);
        }
        grn_hash_delete_by_id(ctx, ctx->impl->expr_vars, eid, NULL);
      }
    }
  }
  return ctx->rc;
}

grn_obj *
grn_proc_get_info(grn_ctx *ctx, grn_user_data *user_data,
                  grn_expr_var **vars, unsigned *nvars, grn_obj **caller)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  if (caller) { *caller = pctx->caller; }
  if (pctx->proc) {
    *vars = grn_expr_get_vars(ctx, (grn_obj *)pctx->proc, nvars);
  } else {
    *vars = NULL;
    *nvars = 0;
  }
  return (grn_obj *)pctx->proc;
}

/* grn_table */

static void
calc_rec_size(grn_obj_flags flags, uint32_t *max_n_subrecs,
              uint8_t *subrec_size, uint8_t *subrec_offset,
              uint32_t *key_size, uint32_t *value_size)
{
  *max_n_subrecs = 0;
  *subrec_size = 0;
  *subrec_offset = 0;
  if (flags & GRN_OBJ_WITH_SUBREC) {
    switch (flags & GRN_OBJ_UNIT_MASK) {
    case GRN_OBJ_UNIT_DOCUMENT_NONE :
      break;
    case GRN_OBJ_UNIT_DOCUMENT_SECTION :
      *max_n_subrecs = *value_size;
      *subrec_offset = sizeof(grn_id);
      *subrec_size = sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_DOCUMENT_POSITION :
      *max_n_subrecs = *value_size;
      *subrec_offset = sizeof(grn_id);
      *subrec_size = sizeof(uint32_t) + sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_SECTION_NONE :
      *key_size += sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_SECTION_POSITION :
      *key_size += sizeof(uint32_t);
      *max_n_subrecs = *value_size;
      *subrec_offset = sizeof(grn_id) + sizeof(uint32_t);
      *subrec_size = sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_POSITION_NONE :
      *key_size += sizeof(uint32_t) + sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_USERDEF_DOCUMENT :
      *max_n_subrecs = *value_size;
      *subrec_size = sizeof(grn_id);
      break;
    case GRN_OBJ_UNIT_USERDEF_SECTION :
      *max_n_subrecs = *value_size;
      *subrec_size = sizeof(grn_id) + sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_USERDEF_POSITION :
      *max_n_subrecs = *value_size;
      *subrec_size = sizeof(grn_id) + sizeof(uint32_t) + sizeof(uint32_t);
      break;
    }
    *value_size = (uintptr_t)GRN_RSET_SUBRECS_NTH((((grn_rset_recinfo *)0)->subrecs),
                                                  *subrec_size, *max_n_subrecs);
  }
}

static grn_obj *grn_view_create(grn_ctx *ctx, const char *path, grn_obj_flags flags);
static grn_obj *grn_view_transcript(grn_ctx *ctx, const char *path, grn_obj *key_type,
                                    grn_obj *value_type, grn_obj_flags flags);

grn_obj *
grn_table_create(grn_ctx *ctx, const char *name, unsigned name_size,
                 const char *path, grn_obj_flags flags,
                 grn_obj *key_type, grn_obj *value_type)
{
  grn_id id;
  grn_id domain = GRN_ID_NIL, range = GRN_ID_NIL;
  uint32_t key_size, value_size, max_n_subrecs;
  uint8_t subrec_size, subrec_offset;
  grn_obj *res = NULL;
  grn_obj *db;
  char buffer[PATH_MAX];
  if (!ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  if (check_name(ctx, name, name_size)) {
    CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  }
  if (key_type) {
    domain = DB_OBJ(key_type)->id;
    switch (key_type->header.type) {
    case GRN_TYPE :
      {
        grn_db_obj *t = (grn_db_obj *)key_type;
        flags |= t->header.flags;
        key_size = GRN_TYPE_SIZE(t);
      }
      break;
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_NO_KEY :
    case GRN_TABLE_PAT_KEY :
      key_size = sizeof(grn_id);
      break;
    default :
      /*
      if (key_type == grn_type_any) {
        key_size = sizeof(grn_id) + sizeof(grn_id);
      }
      */
      key_size = sizeof(grn_id);
    }
  } else {
    key_size = (flags & GRN_OBJ_KEY_VAR_SIZE) ? GRN_TABLE_MAX_KEY_SIZE : sizeof(grn_id);
  }
  if (value_type) {
    range = DB_OBJ(value_type)->id;
    switch (value_type->header.type) {
    case GRN_TYPE :
      {
        grn_db_obj *t = (grn_db_obj *)value_type;
        if (t->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
          ERR(GRN_INVALID_ARGUMENT, "value_type must be fixed size");
          GRN_API_RETURN(NULL);
        }
        value_size = GRN_TYPE_SIZE(t);
      }
      break;
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_NO_KEY :
    case GRN_TABLE_PAT_KEY :
      value_size = sizeof(grn_id);
      break;
    default :
      /*
      if (value_type == grn_type_any) {
        value_size = sizeof(grn_id) + sizeof(grn_id);
      }
      */
      value_size = sizeof(grn_id);
    }
  } else {
    value_size = 0;
  }

  id = grn_obj_register(ctx, db, name, name_size);
  if (ERRP(ctx, GRN_ERROR)) { GRN_API_RETURN(NULL);  }
  if (GRN_OBJ_PERSISTENT & flags) {
    if (!path) {
      if (PERSISTENT_DB_P(db)) {
        gen_pathname(((grn_db *)db)->keys->io->path, buffer, id);
        path = buffer;
      } else {
        ERR(GRN_INVALID_ARGUMENT, "path not assigned for persistent table");
        GRN_API_RETURN(NULL);
      }
    } else {
      flags |= GRN_OBJ_CUSTOM_NAME;
    }
  } else {
    if (path) {
      ERR(GRN_INVALID_ARGUMENT, "path assigned for temporary table");
      GRN_API_RETURN(NULL);
    }
    if (PERSISTENT_DB_P(db) && name && name_size) {
      ERR(GRN_INVALID_ARGUMENT, "name assigned for temporary table");
      GRN_API_RETURN(NULL);
    }
  }
  calc_rec_size(flags, &max_n_subrecs, &subrec_size,
                &subrec_offset, &key_size, &value_size);
  switch (flags & GRN_OBJ_TABLE_TYPE_MASK) {
  case GRN_OBJ_TABLE_HASH_KEY :
    if (key_type && key_type->header.type == GRN_TABLE_VIEW) {
      res = grn_view_transcript(ctx, path, key_type, value_type, flags);
    } else {
      res = (grn_obj *)grn_hash_create(ctx, path, key_size, value_size, flags);
    }
    break;
  case GRN_OBJ_TABLE_PAT_KEY :
    if (key_type && key_type->header.type == GRN_TABLE_VIEW) {
      res = grn_view_transcript(ctx, path, key_type, value_type, flags);
    } else {
      res = (grn_obj *)grn_pat_create(ctx, path, key_size, value_size, flags);
    }
    break;
  case GRN_OBJ_TABLE_NO_KEY :
    if (key_type) {
      GRN_LOG(ctx, GRN_LOG_WARNING, "key_type assigned for no key table");
    }
    if (value_type && value_type->header.type == GRN_TABLE_VIEW) {
      res = grn_view_transcript(ctx, path, key_type, value_type, flags);
    } else {
      domain = range;
      res = (grn_obj *)grn_array_create(ctx, path, value_size, flags);
    }
    break;
  case GRN_OBJ_TABLE_VIEW :
    domain = range = GRN_ID_NIL;
    res = grn_view_create(ctx, path, flags);
    break;
  }
  if (res) {
    DB_OBJ(res)->header.flags = flags;
    DB_OBJ(res)->header.impl_flags = 0;
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->range = range;
    DB_OBJ(res)->max_n_subrecs = max_n_subrecs;
    DB_OBJ(res)->subrec_size = subrec_size;
    DB_OBJ(res)->subrec_offset = subrec_offset;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
      grn_obj_remove(ctx, res);
      res = NULL;
    }
  } else {
    grn_obj_delete_by_id(ctx, db, id, 1);
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_table_open(grn_ctx *ctx, const char *name, unsigned name_size, const char *path)
{
  grn_obj *db;
  if (!ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  } else {
    grn_obj *res = grn_ctx_get(ctx, name, name_size);
    if (res) {
      const char *path2 = grn_obj_path(ctx, res);
      if (path && (!path2 || strcmp(path, path2))) {
        ERR(GRN_INVALID_ARGUMENT, "path unmatch");
        GRN_API_RETURN(NULL);
      }
    } else if (path) {
      uint32_t type = grn_io_detect_type(ctx, path);
      if (!type) { GRN_API_RETURN(NULL); }
      switch (type) {
      case GRN_TABLE_HASH_KEY :
        res = (grn_obj *)grn_hash_open(ctx, path);
        break;
      case GRN_TABLE_PAT_KEY :
        res = (grn_obj *)grn_pat_open(ctx, path);
        break;
      case GRN_TABLE_NO_KEY :
        res = (grn_obj *)grn_array_open(ctx, path);
        break;
      }
      if (res) {
        grn_id id = grn_obj_register(ctx, db, name, name_size);
        res->header.flags |= GRN_OBJ_CUSTOM_NAME;
        res->header.domain = GRN_ID_NIL; /* unknown */
        DB_OBJ(res)->range = GRN_ID_NIL; /* unknown */
        grn_db_obj_init(ctx, db, id, DB_OBJ(res));
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT, "path is missing");
    }
    GRN_API_RETURN(res);
  }
}

grn_id
grn_table_lcp_search(grn_ctx *ctx, grn_obj *table, const void *key, unsigned key_size)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  switch (table->header.type) {
  case GRN_TABLE_PAT_KEY :
    {
      grn_pat *pat = (grn_pat *)table;
      WITH_NORMALIZE(pat, key, key_size, {
        id = grn_pat_lcp_search(ctx, pat, key, key_size);
      });
    }
    break;
  case GRN_TABLE_HASH_KEY :
    {
      grn_hash *hash = (grn_hash *)table;
      WITH_NORMALIZE(hash, key, key_size, {
        id = grn_hash_get(ctx, hash, key, key_size, NULL);
      });
    }
    break;
  }
  GRN_API_RETURN(id);
}

typedef struct {
  grn_id target;
  unsigned int section;
} default_set_value_hook_data;

static grn_obj *
default_set_value_hook(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  if (!pctx) {
    ERR(GRN_INVALID_ARGUMENT, "default_set_value_hook failed");
  } else {
    grn_obj *flags = grn_ctx_pop(ctx);
    grn_obj *newvalue = grn_ctx_pop(ctx);
    grn_obj *oldvalue = grn_ctx_pop(ctx);
    grn_obj *id = grn_ctx_pop(ctx);
    grn_hook *h = pctx->currh;
    default_set_value_hook_data *data = (void *)NEXT_ADDR(h);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    int section = data->section;
    if (flags) { /* todo */ }
    switch (target->header.type) {
    case GRN_COLUMN_INDEX :
      grn_ii_column_update(ctx, (grn_ii *)target,
                           GRN_UINT32_VALUE(id),
                           section, oldvalue, newvalue, NULL);
    }
  }
  return NULL;
}

grn_id
grn_table_add(grn_ctx *ctx, grn_obj *table, const void *key, unsigned key_size, int *added)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    int added_ = 0;
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      {
        grn_pat *pat = (grn_pat *)table;
        WITH_NORMALIZE(pat, key, key_size, {
          if (grn_io_lock(ctx, pat->io, 10000000)) {
            id = GRN_ID_NIL;
          } else {
            id = grn_pat_add(ctx, pat, key, key_size, NULL, &added_);
            grn_io_unlock(pat->io);
          }
        });
        if (added) { *added = added_; }
      }
      break;
    case GRN_TABLE_HASH_KEY :
      {
        grn_hash *hash = (grn_hash *)table;
        WITH_NORMALIZE(hash, key, key_size, {
          if (grn_io_lock(ctx, hash->io, 10000000)) {
            id = GRN_ID_NIL;
          } else {
            id = grn_hash_add(ctx, hash, key, key_size, NULL, &added_);
            grn_io_unlock(hash->io);
          }
        });
        if (added) { *added = added_; }
      }
      break;
    case GRN_TABLE_NO_KEY :
      id = grn_array_add(ctx, (grn_array *)table, NULL);
      if (added) { *added = id ? 1 : 0; }
      break;
    }
    if (added_) {
      grn_hook *hooks = DB_OBJ(table)->hooks[GRN_HOOK_INSERT];
      if (hooks) {
        // todo : grn_proc_ctx_open()
        grn_obj id_, flags_, oldvalue_, value_;
        grn_proc_ctx pctx = {{0}, hooks->proc, NULL, hooks, hooks, PROC_INIT, 4, 4};
        GRN_UINT32_INIT(&id_, 0);
        GRN_UINT32_INIT(&flags_, 0);
        GRN_TEXT_INIT(&oldvalue_, 0);
        GRN_TEXT_INIT(&value_, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET_REF(&value_, key, key_size);
        GRN_UINT32_SET(ctx, &id_, id);
        GRN_UINT32_SET(ctx, &flags_, GRN_OBJ_SET);
        grn_ctx_push(ctx, &id_);
        grn_ctx_push(ctx, &oldvalue_);
        grn_ctx_push(ctx, &value_);
        grn_ctx_push(ctx, &flags_);
        while (hooks) {
          pctx.caller = NULL;
          pctx.currh = hooks;
          if (hooks->proc) {
            hooks->proc->funcs[PROC_INIT](ctx, 1, &table, &pctx.user_data);
          } else {
            default_set_value_hook(ctx, 1, &table, &pctx.user_data);
          }
          if (ctx->rc) { break; }
          hooks = hooks->next;
          pctx.offset++;
        }
      }
    }
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_get_by_key(grn_ctx *ctx, grn_obj *table, grn_obj *key)
{
  grn_id id = GRN_ID_NIL;
  if (table->header.domain == key->header.domain) {
    id = grn_table_get(ctx, table, GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key));
  } else {
    grn_rc rc;
    grn_obj buf;
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, table->header.domain);
    if ((rc = grn_obj_cast(ctx, key, &buf, 1))) {
      ERR(rc, "cast failed");
    } else {
      id = grn_table_get(ctx, table, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
    }
    GRN_OBJ_FIN(ctx, &buf);
  }
  return id;
}

grn_id
grn_table_add_by_key(grn_ctx *ctx, grn_obj *table, grn_obj *key, int *added)
{
  grn_id id = GRN_ID_NIL;
  if (table->header.domain == key->header.domain) {
    id = grn_table_add(ctx, table, GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key), added);
  } else {
    grn_rc rc;
    grn_obj buf;
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, table->header.domain);
    if ((rc = grn_obj_cast(ctx, key, &buf, 1))) {
      ERR(rc, "cast failed");
    } else {
      id = grn_table_add(ctx, table, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf), added);
    }
    GRN_OBJ_FIN(ctx, &buf);
  }
  return id;
}

static grn_obj *
grn_view_create(grn_ctx *ctx, const char *path, grn_obj_flags flags)
{
  grn_view *res;
  if ((res = GRN_MALLOC(sizeof(grn_view)))) {
    if ((res->hash = grn_hash_create(ctx, path, sizeof(grn_id), 0, flags))) {
      GRN_DB_OBJ_SET_TYPE(res, GRN_TABLE_VIEW);
      res->obj.header.flags = flags;
      res->obj.header.domain = GRN_ID_NIL;
      res->n_keys = 0;
      res->offset = 0;
      res->keys = NULL;
      return (grn_obj *)res;
    }
    GRN_FREE(res);
  }
  return NULL;
}

static grn_obj *
grn_view_open(grn_ctx *ctx, const char *path)
{
  grn_view *res;
  if ((res = GRN_MALLOC(sizeof(grn_view)))) {
    if ((res->hash = grn_hash_open(ctx, path))) {
      GRN_DB_OBJ_SET_TYPE(res, GRN_TABLE_VIEW);
      res->n_keys = 0;
      res->offset = 0;
      res->keys = NULL;
      return (grn_obj *)res;
    }
    GRN_FREE(res);
  }
  return NULL;
}

grn_rc
grn_view_close(grn_ctx *ctx, grn_view *v)
{
  grn_hash_close(ctx, v->hash);
  GRN_FREE(v);
  return ctx->rc;
}

static grn_obj *
grn_view_transcript(grn_ctx *ctx, const char *path,
                    grn_obj *key_type, grn_obj *value_type, grn_obj_flags flags)
{
  grn_id *tp;
  grn_obj *res = grn_view_create(ctx, path, flags);
  /* todo : support persistent view */
  if (key_type) {
    grn_view *v = (grn_view *)key_type;
    GRN_HASH_EACH(ctx, v->hash, id, &tp, NULL, NULL, {
      grn_view_add(ctx, res,
                   grn_table_create(ctx, NULL, 0, NULL, flags,
                                    grn_ctx_at(ctx, *tp), value_type));
    });
  } else if (value_type) {
    grn_view *v = (grn_view *)value_type;
    GRN_HASH_EACH(ctx, v->hash, id, &tp, NULL, NULL, {
      grn_view_add(ctx, res,
                   grn_table_create(ctx, NULL, 0, NULL, flags,
                                    NULL, grn_ctx_at(ctx, *tp)));
    });
  }
  return res;
}

grn_id
grn_view_add(grn_ctx *ctx, grn_obj *view, grn_obj *table)
{
  if (!view || view->header.type != GRN_TABLE_VIEW) {
    ERR(GRN_INVALID_ARGUMENT, "invalid view");
    return ctx->rc;
  }
  if (!GRN_OBJ_TABLEP(table)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid table");
    return ctx->rc;
  }
  {
    grn_view *v = (grn_view *)view;
    grn_id id = DB_OBJ(table)->id;
    return grn_hash_add(ctx, v->hash, (void *)&id, sizeof(grn_id), NULL, NULL);
  }
}

uint32_t
grn_view_size(grn_ctx *ctx, grn_view *view)
{
  grn_id *tp;
  uint32_t n = 0;
  GRN_HASH_EACH(ctx, view->hash, id, &tp, NULL, NULL, {
    n += grn_table_size(ctx, grn_ctx_at(ctx, *tp));
  });
  return n;
}

grn_id
grn_table_get(grn_ctx *ctx, grn_obj *table, const void *key, unsigned int key_size)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_get(ctx, (grn_pat *)table, key, key_size, NULL);
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_get(ctx, (grn_hash *)table, key, key_size, NULL);
      });
      break;
    }
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_add_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                void **value, int *added)
{
  grn_id id = GRN_ID_NIL;
  if (!key || !key_size) { return GRN_ID_NIL; }
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_add(ctx, (grn_pat *)table, key, key_size, value, added);
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_add(ctx, (grn_hash *)table, key, key_size, value, added);
      });
      break;
    case GRN_TABLE_NO_KEY :
      id = grn_array_add(ctx, (grn_array *)table, value);
      if (added) { *added = id ? 1 : 0; }
      break;
    }
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_get_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                void **value)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_get(ctx, (grn_pat *)table, key, key_size, value);
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_get(ctx, (grn_hash *)table, key, key_size, value);
      });
      break;
    }
  }
  GRN_API_RETURN(id);
}

int
grn_table_get_key(grn_ctx *ctx, grn_obj *table, grn_id id, void *keybuf, int buf_size)
{
  int r = 0;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY :
      r = grn_hash_get_key(ctx, (grn_hash *)table, id, keybuf, buf_size);
      break;
    case GRN_TABLE_PAT_KEY :
      r = grn_pat_get_key(ctx, (grn_pat *)table, id, keybuf, buf_size);
      break;
    case GRN_TABLE_NO_KEY :
      {
        grn_array *a = (grn_array *)table;
        if (a->obj.header.domain) {
          if (buf_size >= a->value_size) {
            r = grn_array_get_value(ctx, a, id, keybuf);
          } else {
            r = a->value_size;
          }
        }
      }
      break;
    }
  }
  GRN_API_RETURN(r);
}

int
grn_table_get_key2(grn_ctx *ctx, grn_obj *table, grn_id id, grn_obj *bulk)
{
  int r = 0;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY :
      r = grn_hash_get_key2(ctx, (grn_hash *)table, id, bulk);
      break;
    case GRN_TABLE_PAT_KEY :
      r = grn_pat_get_key2(ctx, (grn_pat *)table, id, bulk);
      break;
    case GRN_TABLE_NO_KEY :
      {
        grn_array *a = (grn_array *)table;
        if (a->obj.header.domain) {
          if (!grn_bulk_space(ctx, bulk, a->value_size)) {
            char *curr = GRN_BULK_CURR(bulk);
            r = grn_array_get_value(ctx, a, id, curr - a->value_size);
          }
        }
      }
      break;
    }
  }
  GRN_API_RETURN(r);
}


static grn_rc
grn_obj_clear_value(grn_ctx *ctx, grn_obj *obj, grn_id id)
{
  grn_rc rc = GRN_SUCCESS;
  if (GRN_DB_OBJP(obj)) {
    grn_obj buf;
    grn_id range = DB_OBJ(obj)->range;
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
    switch (obj->header.type) {
    case GRN_COLUMN_VAR_SIZE :
      rc = grn_obj_set_value(ctx, obj, id, &buf, GRN_OBJ_SET);
      break;
    case GRN_COLUMN_FIX_SIZE :
      grn_bulk_truncate(ctx, &buf, ((grn_ra *)obj)->header->element_size);
      rc = grn_obj_set_value(ctx, obj, id, &buf, GRN_OBJ_SET);
      break;
    }
    GRN_OBJ_FIN(ctx, &buf);
  }
  return rc;
}

static void
call_delete_hook(grn_ctx *ctx, grn_obj *table, grn_id rid)
{
  if (rid) {
    grn_hook *hooks = DB_OBJ(table)->hooks[GRN_HOOK_DELETE];
    if (hooks) {
      // todo : grn_proc_ctx_open()
      grn_obj id_, flags_, oldvalue_, value_;
      grn_proc_ctx pctx = {{0}, hooks->proc, NULL, hooks, hooks, PROC_INIT, 4, 4};
      GRN_UINT32_INIT(&id_, 0);
      GRN_UINT32_INIT(&flags_, 0);
      GRN_TEXT_INIT(&oldvalue_, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_INIT(&value_, 0);
      // GRN_TEXT_SET_REF(&oldvalue_, key, key_size);
      GRN_UINT32_SET(ctx, &id_, rid);
      GRN_UINT32_SET(ctx, &flags_, GRN_OBJ_SET);
      grn_ctx_push(ctx, &id_);
      grn_ctx_push(ctx, &oldvalue_);
      grn_ctx_push(ctx, &value_);
      grn_ctx_push(ctx, &flags_);
      while (hooks) {
        pctx.caller = NULL;
        pctx.currh = hooks;
        if (hooks->proc) {
          hooks->proc->funcs[PROC_INIT](ctx, 1, &table, &pctx.user_data);
        } else {
          default_set_value_hook(ctx, 1, &table, &pctx.user_data);
        }
        if (ctx->rc) { break; }
        hooks = hooks->next;
        pctx.offset++;
      }
    }
  }
}

static void
clear_column_values(grn_ctx *ctx, grn_obj *table, grn_id rid)
{
  if (rid) {
    grn_hash *cols;
    if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
      if (grn_table_columns(ctx, table, "", 0, (grn_obj *)cols)) {
        grn_id *key;
        GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
          grn_obj *col = grn_ctx_at(ctx, *key);
          if (col) { grn_obj_clear_value(ctx, col, rid); }
        });
      }
      grn_hash_close(ctx, cols);
    }
  }
}

grn_rc
grn_table_delete(grn_ctx *ctx, grn_obj *table, const void *key, unsigned key_size)
{
  grn_id rid = GRN_ID_NIL;
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    if (key && key_size) {
      rid = grn_table_get(ctx, table, key, key_size);
      call_delete_hook(ctx, table, rid);
    }
    switch (table->header.type) {
    case GRN_DB :
      /* todo : delete tables and columns from db */
      break;
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        grn_pat *pat = (grn_pat *)table;
        if (pat->io && !(pat->io->flags & GRN_IO_TEMPORARY)) {
          if (!(rc = grn_io_lock(ctx, pat->io, 10000000))) {
            rc = grn_pat_delete(ctx, pat, key, key_size, NULL);
            grn_io_unlock(pat->io);
          }
        } else {
          rc = grn_pat_delete(ctx, pat, key, key_size, NULL);
        }
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        grn_hash *hash = (grn_hash *)table;
        if (hash->io && !(hash->io->flags & GRN_IO_TEMPORARY)) {
          if (!(rc = grn_io_lock(ctx, hash->io, 10000000))) {
            rc = grn_hash_delete(ctx, hash, key, key_size, NULL);
            grn_io_unlock(hash->io);
          }
        } else {
          rc = grn_hash_delete(ctx, hash, key, key_size, NULL);
        }
      });
      break;
    }
    clear_column_values(ctx, table, rid);
  }
  GRN_API_RETURN(rc);
}

grn_rc
_grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id,
                       grn_table_delete_optarg *optarg)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  if (table) {
    call_delete_hook(ctx, table, id);
    // todo : support optarg
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      rc = grn_pat_delete_by_id(ctx, (grn_pat *)table, id, optarg);
      break;
    case GRN_TABLE_HASH_KEY :
      rc = grn_hash_delete_by_id(ctx, (grn_hash *)table, id, optarg);
      break;
    case GRN_TABLE_NO_KEY :
      rc = grn_array_delete_by_id(ctx, (grn_array *)table, id, optarg);
      break;
    }
    clear_column_values(ctx, table, id);
  }
  return rc;
}

static grn_io*
grn_obj_io(grn_obj *obj)
{
  grn_io *io = NULL;
  if (obj) {
    switch (obj->header.type) {
    case GRN_DB :
      io = ((grn_db *)obj)->keys->io;
      break;
    case GRN_TABLE_PAT_KEY :
      io = ((grn_pat *)obj)->io;
      break;
    case GRN_TABLE_HASH_KEY :
      io = ((grn_hash *)obj)->io;
      break;
    case GRN_TABLE_NO_KEY :
      io = ((grn_array *)obj)->io;
      break;
    case GRN_COLUMN_VAR_SIZE :
      io = ((grn_ja *)obj)->io;
      break;
    case GRN_COLUMN_FIX_SIZE :
      io = ((grn_ra *)obj)->io;
      break;
    case GRN_COLUMN_INDEX :
      io = ((grn_ii *)obj)->seg;
      break;
    }
  }
  return io;
}

grn_rc
grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id)
{
  grn_rc rc;
  grn_io *io;
  GRN_API_ENTER;
  if ((io = grn_obj_io(table)) && !(io->flags & GRN_IO_TEMPORARY)) {
    if (!(rc = grn_io_lock(ctx, io, 10000000))) {
      rc = _grn_table_delete_by_id(ctx, table, id, NULL);
      grn_io_unlock(io);
    }
  } else {
    rc = _grn_table_delete_by_id(ctx, table, id, NULL);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_truncate(grn_ctx *ctx, grn_obj *table)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      rc = grn_pat_truncate(ctx, (grn_pat *)table);
      break;
    case GRN_TABLE_HASH_KEY :
      rc = grn_hash_truncate(ctx, (grn_hash *)table);
      break;
    case GRN_TABLE_NO_KEY :
      rc = grn_array_truncate(ctx, (grn_array *)table);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_get_info(grn_ctx *ctx, grn_obj *table, grn_obj_flags *flags,
                   grn_encoding *encoding, grn_obj **tokenizer)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      if (flags) { *flags = ((grn_pat *)table)->obj.header.flags; }
      if (encoding) { *encoding = ((grn_pat *)table)->encoding; }
      if (tokenizer) { *tokenizer = ((grn_pat *)table)->tokenizer; }
      rc = GRN_SUCCESS;
      break;
    case GRN_TABLE_HASH_KEY :
      if (flags) { *flags = ((grn_hash *)table)->obj.header.flags; }
      if (encoding) { *encoding = ((grn_hash *)table)->encoding; }
      if (tokenizer) { *tokenizer = ((grn_hash *)table)->tokenizer; }
      rc = GRN_SUCCESS;
      break;
    case GRN_TABLE_NO_KEY :
      if (flags) { *flags = 0; }
      if (encoding) { *encoding = GRN_ENC_NONE; }
      if (tokenizer) { *tokenizer = grn_uvector_tokenizer; }
      rc = GRN_SUCCESS;
      break;
    }
  }
  GRN_API_RETURN(rc);
}

unsigned int
grn_table_size(grn_ctx *ctx, grn_obj *table)
{
  unsigned int n = 0;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_DB :
      n = grn_pat_size(ctx, ((grn_db *)table)->keys);
      break;
    case GRN_TABLE_PAT_KEY :
      n = grn_pat_size(ctx, (grn_pat *)table);
      break;
    case GRN_TABLE_HASH_KEY :
      n = GRN_HASH_SIZE((grn_hash *)table);
      break;
    case GRN_TABLE_NO_KEY :
      n = GRN_ARRAY_SIZE((grn_array *)table);
      break;
    case GRN_TABLE_VIEW :
      n = grn_view_size(ctx, (grn_view *)table);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "not supported");
      break;
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid table assigned");
  }
  GRN_API_RETURN(n);
}

inline static void
subrecs_push(byte *subrecs, int size, int n_subrecs, int score, void *body, int dir)
{
  byte *v;
  int *c2;
  int n = n_subrecs - 1, n2;
  while (n) {
    n2 = (n - 1) >> 1;
    c2 = GRN_RSET_SUBRECS_NTH(subrecs,size,n2);
    if (GRN_RSET_SUBRECS_CMP(score, *c2, dir)) { break; }
    GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
    n = n2;
  }
  v = subrecs + n * (size + GRN_RSET_SCORE_SIZE);
  *((int *)v) = score;
  memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

inline static void
subrecs_replace_min(byte *subrecs, int size, int n_subrecs, int score, void *body, int dir)
{
  byte *v;
  int n = 0, n1, n2, *c1, *c2;
  for (;;) {
    n1 = n * 2 + 1;
    n2 = n1 + 1;
    c1 = n1 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n1) : NULL;
    c2 = n2 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n2) : NULL;
    if (c1 && GRN_RSET_SUBRECS_CMP(score, *c1, dir)) {
      if (c2 &&
          GRN_RSET_SUBRECS_CMP(score, *c2, dir) &&
          GRN_RSET_SUBRECS_CMP(*c1, *c2, dir)) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c1);
        n = n1;
      }
    } else {
      if (c2 && GRN_RSET_SUBRECS_CMP(score, *c2, dir)) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        break;
      }
    }
  }
  v = subrecs + n * (size + GRN_RSET_SCORE_SIZE);
  memcpy(v, &score, GRN_RSET_SCORE_SIZE);
  memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

void
grn_table_add_subrec(grn_obj *table, grn_rset_recinfo *ri, int score,
                     grn_rset_posinfo *pi, int dir)
{
  if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
    int limit = DB_OBJ(table)->max_n_subrecs;
    ri->score += score;
    ri->n_subrecs += 1;
    if (limit) {
      int subrec_size = DB_OBJ(table)->subrec_size;
      int n_subrecs = GRN_RSET_N_SUBRECS(ri);
      if (pi) {
        byte *body = (byte *)pi + DB_OBJ(table)->subrec_offset;
        if (limit < n_subrecs) {
          if (GRN_RSET_SUBRECS_CMP(score, *ri->subrecs, dir)) {
            subrecs_replace_min((byte *)ri->subrecs, subrec_size, limit, score, body, dir);
          }
        } else {
          subrecs_push((byte *)ri->subrecs, subrec_size, n_subrecs, score, body, dir);
        }
      }
    }
  }
}

typedef struct {
  grn_db_obj obj;
  grn_id curr_rec;
  grn_view *view;
  int n_entries;
  uint32_t rest;
  grn_table_cursor **bins;
} grn_view_cursor;

static int compare_cursor(grn_ctx *ctx, grn_table_cursor *a, grn_table_cursor *b, int n_keys);
static grn_id grn_view_cursor_next(grn_ctx *ctx, grn_view_cursor *vc);

#define VIEW_CURSOR_OFFSET(tc) (DB_OBJ(tc)->max_n_subrecs)
#define VIEW_CURSOR_DELAY(tc) (DB_OBJ(tc)->subrec_size)

static grn_view_cursor *
grn_view_cursor_open(grn_ctx *ctx, grn_obj *view,
                     const void *min, unsigned int min_size,
                     const void *max, unsigned int max_size,
                     int offset, int limit, int flags)
{
  grn_id *tp;
  grn_view_cursor *vc;
  grn_view *v = (grn_view *)view;
  if (v && (vc = GRN_MALLOCN(grn_view_cursor, 1))) {
    vc->view = v;
    vc->curr_rec = GRN_ID_NIL;
    VIEW_CURSOR_DELAY(vc) = 1;
    GRN_DB_OBJ_SET_TYPE(vc, GRN_CURSOR_TABLE_VIEW);
    if ((vc->bins = GRN_MALLOCN(grn_table_cursor *, GRN_HASH_SIZE(v->hash)))) {
      int i = 0, n, n2;
      uint32_t view_cursor_offset = 0;
      grn_hash *hash = v->hash;
      grn_table_cursor *c, *c2;
      GRN_HASH_EACH(ctx, hash, id, &tp, NULL, NULL, {
        c = grn_table_cursor_open(ctx, grn_ctx_at(ctx, *tp),
                                  min, min_size, max, max_size, 0, offset + limit, flags);
        if (!c) { break; }
        VIEW_CURSOR_OFFSET(c) = view_cursor_offset++;
        if (!grn_table_cursor_next(ctx, c)) {
          grn_table_cursor_close(ctx, c);
          continue;
        }
        for (n = i++; n; n = n2) {
          n2 = (n - 1) >> 1;
          c2 = vc->bins[n2];
          if (!compare_cursor(ctx, c2, c, v->n_keys)) { break; }
          vc->bins[n] = c2;
        }
        vc->bins[n] = c;
      });
      vc->n_entries = i;
      vc->rest = GRN_ID_MAX;
      offset += v->offset;
      while (offset--) { if (!grn_view_cursor_next(ctx, vc)) { break; } }
      vc->rest = (limit < 0) ? GRN_ID_MAX : limit;
      return vc;
    }
    GRN_FREE(vc);
  }
  return NULL;
}

const char *grn_obj_get_value_(grn_ctx *ctx, grn_obj *obj, grn_id id, uint32_t *size);

const char *
grn_table_cursor_get_sort_key_value_(grn_ctx *ctx, grn_table_cursor *tc, int offset,
                                     uint32_t *size, grn_table_sort_key **key)
{
  const char * res = NULL;
  *size = 0;
  while (tc->header.type == GRN_CURSOR_TABLE_VIEW) {
    tc = ((grn_view_cursor *)tc)->bins[0];
  }
  if (tc->header.type == GRN_CURSOR_TABLE_NO_KEY) {
    grn_id id;
    grn_array_cursor *ac = (grn_array_cursor *)tc;
    if (ac->array->keys && offset < ac->array->n_keys &&
        grn_array_get_value(ctx, ac->array, ac->curr_rec, &id) == sizeof(grn_id)) {
      *key = ac->array->keys + offset;
      res = grn_obj_get_value_(ctx, (*key)->key, id, size);
    }
  }
  return res;
}

static void
grn_view_cursor_recalc_min(grn_ctx *ctx, grn_view_cursor *vc)
{
  int n = 0, n1, n2, m;
  int n_keys = vc->view->n_keys;
  if ((m = vc->n_entries) > 1) {
    grn_table_cursor *c = vc->bins[0], *c1, *c2;
    for (;;) {
      n1 = n * 2 + 1;
      n2 = n1 + 1;
      c1 = n1 < m ? vc->bins[n1] : NULL;
      c2 = n2 < m ? vc->bins[n2] : NULL;
      if (c1 && compare_cursor(ctx, c, c1, n_keys)) {
        if (c2 && compare_cursor(ctx, c, c2, n_keys) && compare_cursor(ctx, c1, c2, n_keys)) {
          vc->bins[n] = c2;
          n = n2;
        } else {
          vc->bins[n] = c1;
          n = n1;
        }
      } else {
        if (c2 && compare_cursor(ctx, c, c2, n_keys)) {
          vc->bins[n] = c2;
          n = n2;
        } else {
          vc->bins[n] = c;
          break;
        }
      }
    }
  }
}

/*
static int
grn_table_cursor_target_id(grn_ctx *ctx, grn_table_cursor *tc, grn_obj *id)
{
  int len;
  switch (tc->header.type) {
  case GRN_CURSOR_TABLE_PAT_KEY :
    GRN_RECORD_PUT(ctx, id, ((grn_pat_cursor *)tc)->pat->obj.id);
    len = sizeof(grn_id);
    break;
  case GRN_CURSOR_TABLE_HASH_KEY :
    GRN_RECORD_PUT(ctx, id, ((grn_hash_cursor *)tc)->hash->obj.id);
    len = sizeof(grn_id);
    break;
  case GRN_CURSOR_TABLE_NO_KEY :
    GRN_RECORD_PUT(ctx, id, ((grn_array_cursor *)tc)->array->obj.id);
    len = sizeof(grn_id);
    break;
  case GRN_CURSOR_TABLE_VIEW :
    GRN_RECORD_PUT(ctx, id, ((grn_view_cursor *)tc)->view->obj.id);
    len = sizeof(grn_id);
    break;
  default :
    len = 0;
    break;
  }
  return len;
}
*/

static grn_rc grn_table_cursor_next_o_(grn_ctx *ctx, grn_table_cursor *tc, grn_obj *id);

static grn_id
grn_table_cursor_curr(grn_ctx *ctx, grn_table_cursor *tc)
{
  grn_id id = GRN_ID_NIL;
  while (tc && tc->header.type == GRN_CURSOR_TABLE_VIEW) {
    grn_view_cursor *vc = (grn_view_cursor *)tc;
    if (!vc->n_entries) { return GRN_ID_NIL; }
    tc = vc->bins[0];
  }
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      id = ((grn_pat_cursor *)tc)->curr_rec;
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      id = ((grn_hash_cursor *)tc)->curr_rec;
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      id = ((grn_array_cursor *)tc)->curr_rec;
      break;
    }
  }
  return id;
}

static grn_id
grn_view_cursor_next(grn_ctx *ctx, grn_view_cursor *vc)
{
  if (!vc->rest) { return GRN_ID_NIL; }
  if (VIEW_CURSOR_DELAY(vc)) {
    VIEW_CURSOR_DELAY(vc) = 0;
  } else {
    grn_table_cursor *tc = vc->bins[0];
    if (!grn_table_cursor_next(ctx, tc)) {
      grn_table_cursor_close(ctx, tc);
      vc->bins[0] = vc->bins[--vc->n_entries];
    }
    grn_view_cursor_recalc_min(ctx, vc);
  }
  if (vc->n_entries) {
    grn_table_cursor *tc = vc->bins[0];
    vc->rest--;
    return grn_table_cursor_curr(ctx, tc);
  } else {
    return GRN_ID_NIL;
  }
}

static grn_rc
grn_table_cursor_curr_o(grn_ctx *ctx, grn_table_cursor *tc, grn_obj *id)
{
  while (tc->header.type == GRN_CURSOR_TABLE_VIEW) {
    grn_view_cursor *vc = (grn_view_cursor *)tc;
    if (!vc->n_entries) { return GRN_END_OF_DATA; }
    tc = vc->bins[0];
    GRN_UINT32_PUT(ctx, id, VIEW_CURSOR_OFFSET(tc));
  }
  GRN_RECORD_PUT(ctx, id, grn_table_cursor_curr(ctx, tc));
  return ctx->rc;
}

static grn_rc
grn_view_cursor_next_o(grn_ctx *ctx, grn_view_cursor *vc, grn_obj *id)
{
  if (!vc->rest) { return GRN_END_OF_DATA; }
  if (VIEW_CURSOR_DELAY(vc)) {
    VIEW_CURSOR_DELAY(vc) = 0;
  } else {
    grn_table_cursor *tc = vc->bins[0];
    if (!grn_table_cursor_next(ctx, tc)) {
      grn_table_cursor_close(ctx, tc);
      vc->bins[0] = vc->bins[--vc->n_entries];
    }
    grn_view_cursor_recalc_min(ctx, vc);
  }
  if (vc->n_entries) {
    grn_table_cursor *tc = vc->bins[0];
    GRN_UINT32_PUT(ctx, id, VIEW_CURSOR_OFFSET(tc));
    vc->rest--;
    return grn_table_cursor_curr_o(ctx, tc, id);
  } else {
    return GRN_END_OF_DATA;
  }
}

static void
grn_view_cursor_close(grn_ctx *ctx, grn_view_cursor *vc)
{
  int i = vc->n_entries;
  while (i--) { grn_table_cursor_close(ctx, vc->bins[i]); }
  GRN_FREE(vc->bins);
  GRN_FREE(vc);
}

grn_table_cursor *
grn_table_cursor_open(grn_ctx *ctx, grn_obj *table,
                      const void *min, unsigned min_size,
                      const void *max, unsigned max_size,
                      int offset, int limit, int flags)
{
  grn_table_cursor *tc = NULL;
  GRN_API_ENTER;
  if (table && !grn_normalize_offset_and_limit(ctx, grn_table_size(ctx, table),
                                               &offset, &limit)) {
    switch (table->header.type) {
    case GRN_DB :
      tc = (grn_table_cursor *)grn_pat_cursor_open(ctx, ((grn_db *)table)->keys,
                                                   min, min_size,
                                                   max, max_size, offset, limit, flags);
      break;
    case GRN_TABLE_PAT_KEY :
      tc = (grn_table_cursor *)grn_pat_cursor_open(ctx, (grn_pat *)table,
                                                   min, min_size,
                                                   max, max_size, offset, limit, flags);
      break;
    case GRN_TABLE_HASH_KEY :
      tc = (grn_table_cursor *)grn_hash_cursor_open(ctx, (grn_hash *)table,
                                                    min, min_size,
                                                    max, max_size, offset, limit, flags);
      break;
    case GRN_TABLE_NO_KEY :
      tc = (grn_table_cursor *)grn_array_cursor_open(ctx, (grn_array *)table,
                                                     GRN_ID_NIL, GRN_ID_NIL,
                                                     offset, limit, flags);
      break;
    case GRN_TABLE_VIEW :
      tc = (grn_table_cursor *)grn_view_cursor_open(ctx, table,
                                                    min, min_size,
                                                    max, max_size, offset, limit, flags);
    }
  }
  if (tc) {
    grn_id id = grn_obj_register(ctx, ctx->impl->db, NULL, 0);
    DB_OBJ(tc)->header.domain = GRN_ID_NIL;
    DB_OBJ(tc)->range = GRN_ID_NIL;
    grn_db_obj_init(ctx, ctx->impl->db, id, DB_OBJ(tc));
  }
  GRN_API_RETURN(tc);
}

grn_table_cursor *
grn_table_cursor_open_by_id(grn_ctx *ctx, grn_obj *table,
                            grn_id min, grn_id max, int flags)
{
  grn_table_cursor *tc = NULL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      tc = (grn_table_cursor *)grn_pat_cursor_open(ctx, (grn_pat *)table,
                                                   NULL, 0, NULL, 0, 0, -1, flags);
      break;
    case GRN_TABLE_HASH_KEY :
      tc = (grn_table_cursor *)grn_hash_cursor_open(ctx, (grn_hash *)table,
                                                    NULL, 0, NULL, 0, 0, -1, flags);
      break;
    case GRN_TABLE_NO_KEY :
      tc = (grn_table_cursor *)grn_array_cursor_open(ctx, (grn_array *)table,
                                                     min, max, 0, -1, flags);
      break;
    }
  }
  GRN_API_RETURN(tc);
}

grn_rc
grn_table_cursor_close(grn_ctx *ctx, grn_table_cursor *tc)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
    rc = GRN_INVALID_ARGUMENT;
  } else {
    {
      if (DB_OBJ(tc)->finalizer) {
        DB_OBJ(tc)->finalizer(ctx, 1, (grn_obj **)&tc, &DB_OBJ(tc)->user_data);
      }
      if (DB_OBJ(tc)->source) {
        GRN_FREE(DB_OBJ(tc)->source);
      }
      /*
      grn_hook_entry entry;
      for (entry = 0; entry < N_HOOK_ENTRIES; entry++) {
        grn_hook_free(ctx, DB_OBJ(tc)->hooks[entry]);
      }
      */
      grn_obj_delete_by_id(ctx, DB_OBJ(tc)->db, DB_OBJ(tc)->id, 0);
    }
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      grn_pat_cursor_close(ctx, (grn_pat_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      grn_hash_cursor_close(ctx, (grn_hash_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      grn_array_cursor_close(ctx, (grn_array_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_VIEW :
      grn_view_cursor_close(ctx, (grn_view_cursor *)tc);
    default :
      rc = GRN_INVALID_ARGUMENT;
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_id
grn_table_cursor_next(grn_ctx *ctx, grn_table_cursor *tc)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      id = grn_pat_cursor_next(ctx, (grn_pat_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      id = grn_hash_cursor_next(ctx, (grn_hash_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      id = grn_array_cursor_next(ctx, (grn_array_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_VIEW :
      id = grn_view_cursor_next(ctx, (grn_view_cursor *)tc);
      break;
    }
  }
  GRN_API_RETURN(id);
}

static grn_rc
grn_table_cursor_next_o_(grn_ctx *ctx, grn_table_cursor *tc, grn_obj *id)
{
  if (tc->header.type == GRN_CURSOR_TABLE_VIEW) {
    return grn_view_cursor_next_o(ctx, (grn_view_cursor *)tc, id);
  } else {
    grn_id rid = grn_table_cursor_next(ctx, tc);
    if (rid) {
      GRN_RECORD_PUT(ctx, id, rid);
      return ctx->rc;
    } else {
      return GRN_END_OF_DATA;
    }
  }
}

grn_rc
grn_table_cursor_next_o(grn_ctx *ctx, grn_table_cursor *tc, grn_obj *id)
{
  GRN_BULK_REWIND(id);
  return grn_table_cursor_next_o_(ctx, tc, id);
}

int
grn_table_cursor_get_key(grn_ctx *ctx, grn_table_cursor *tc, void **key)
{
  int len = 0;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      len = grn_pat_cursor_get_key(ctx, (grn_pat_cursor *)tc, key);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      len = grn_hash_cursor_get_key(ctx, (grn_hash_cursor *)tc, key);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "invalid type %d", tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(len);
}

int
grn_table_cursor_get_value(grn_ctx *ctx, grn_table_cursor *tc, void **value)
{
  int len = 0;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      len = grn_pat_cursor_get_value(ctx, (grn_pat_cursor *)tc, value);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      len = grn_hash_cursor_get_value(ctx, (grn_hash_cursor *)tc, value);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      len = grn_array_cursor_get_value(ctx, (grn_array_cursor *)tc, value);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "invalid type %d", tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(len);
}

grn_rc
grn_table_cursor_set_value(grn_ctx *ctx, grn_table_cursor *tc,
                           void *value, int flags)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      rc = grn_pat_cursor_set_value(ctx, (grn_pat_cursor *)tc, value, flags);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      rc = grn_hash_cursor_set_value(ctx, (grn_hash_cursor *)tc, value, flags);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      rc = grn_array_cursor_set_value(ctx, (grn_array_cursor *)tc, value, flags);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "invalid type %d", tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_cursor_delete(grn_ctx *ctx, grn_table_cursor *tc)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      rc = grn_pat_cursor_delete(ctx, (grn_pat_cursor *)tc, NULL);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      rc = grn_hash_cursor_delete(ctx, (grn_hash_cursor *)tc, NULL);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      rc = grn_array_cursor_delete(ctx, (grn_array_cursor *)tc, NULL);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "invalid type %d", tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_obj *
grn_table_cursor_table(grn_ctx *ctx, grn_table_cursor *tc)
{
  grn_obj *obj = NULL;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "tc is null");
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY :
      obj = (grn_obj *)(((grn_pat_cursor *)tc)->pat);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      obj = (grn_obj *)(((grn_hash_cursor *)tc)->hash);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      obj = (grn_obj *)(((grn_array_cursor *)tc)->array);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "invalid type %d", tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(obj);
}

grn_rc
grn_table_search(grn_ctx *ctx, grn_obj *table, const void *key, uint32_t key_size,
                 grn_operator mode, grn_obj *res, grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  switch (table->header.type) {
  case GRN_TABLE_PAT_KEY :
    {
      grn_pat *pat = (grn_pat *)table;
      WITH_NORMALIZE(pat, key, key_size, {
        switch (mode) {
        case GRN_OP_EXACT :
          {
            grn_id id = grn_pat_get(ctx, pat, key, key_size, NULL);
            if (id) { grn_table_add(ctx, res, &id, sizeof(grn_id), NULL); }
          }
          // todo : support op;
          break;
        case GRN_OP_LCP :
          {
            grn_id id = grn_pat_lcp_search(ctx, pat, key, key_size);
            if (id) { grn_table_add(ctx, res, &id, sizeof(grn_id), NULL); }
          }
          // todo : support op;
          break;
        case GRN_OP_SUFFIX :
          rc = grn_pat_suffix_search(ctx, pat, key, key_size, (grn_hash *)res);
          // todo : support op;
          break;
        case GRN_OP_PREFIX :
          rc = grn_pat_prefix_search(ctx, pat, key, key_size, (grn_hash *)res);
          // todo : support op;
          break;
        case GRN_OP_TERM_EXTRACT :
          {
            int len;
            grn_id tid;
            const char *sp = key;
            const char *se = sp + key_size;
            for (; sp < se; sp += len) {
              if ((tid = grn_pat_lcp_search(ctx, pat, sp, se - sp))) {
                grn_table_add(ctx, res, &tid, sizeof(grn_id), NULL);
                /* todo : nsubrec++ if GRN_OBJ_TABLE_SUBSET assigned */
              }
              if (!(len = grn_charlen(ctx, sp, se))) { break; }
            }
          }
          // todo : support op;
          break;
        default :
          rc = GRN_INVALID_ARGUMENT;
          ERR(rc, "invalid mode %d", mode);
        }
      });
    }
    break;
  case GRN_TABLE_HASH_KEY :
    {
      grn_hash *hash = (grn_hash *)table;
      grn_id id;
      WITH_NORMALIZE(hash, key, key_size, {
        id = grn_hash_get(ctx, hash, key, key_size, NULL);
      });
      if (id) { grn_table_add(ctx, res, &id, sizeof(grn_id), NULL); }
    }
    break;
  }
  GRN_API_RETURN(rc);
}

grn_id
grn_table_next(grn_ctx *ctx, grn_obj *table, grn_id id)
{
  grn_id r = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      r = grn_pat_next(ctx, (grn_pat *)table, id);
      break;
    case GRN_TABLE_HASH_KEY :
      r = grn_hash_next(ctx, (grn_hash *)table, id);
      break;
    case GRN_TABLE_NO_KEY :
      r = grn_array_next(ctx, (grn_array *)table, id);
      break;
    }
  }
  GRN_API_RETURN(r);
}

grn_rc
grn_obj_search(grn_ctx *ctx, grn_obj *obj, grn_obj *query,
               grn_obj *res, grn_operator op, grn_search_optarg *optarg)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY :
    case GRN_TABLE_HASH_KEY :
      {
        const void *key = GRN_BULK_HEAD(query);
        uint32_t key_size = GRN_BULK_VSIZE(query);
        grn_operator mode = optarg ? optarg->mode : GRN_OP_EXACT;
        if (!key || !key_size) {
          return GRN_INVALID_ARGUMENT;
        }
        rc = grn_table_search(ctx, obj, key, key_size, mode, res, op);
      }
      break;
    case GRN_COLUMN_INDEX :
      switch (query->header.type) {
      case GRN_BULK :
        if (query->header.domain == obj->header.domain &&
            GRN_BULK_VSIZE(query) == sizeof(grn_id)) {
          grn_id tid = *((grn_id *)GRN_BULK_HEAD(query));
          grn_ii_cursor *c = grn_ii_cursor_open(ctx, (grn_ii *)obj, tid,
                                                GRN_ID_NIL, GRN_ID_MAX, 1, 0);
          if (c) {
            grn_ii_posting *pos;
            grn_hash *s = (grn_hash *)res;
            while ((pos = grn_ii_cursor_next(ctx, c))) {
              /* todo: support orgarg(op)
              res_add(ctx, s, (grn_rset_posinfo *) pos,
                      get_weight(ctx, s, pos->rid, pos->sid, wvm, optarg), op);
              */
              grn_hash_add(ctx, s, pos, s->key_size, NULL, NULL);
            }
            grn_ii_cursor_close(ctx, c);
          }
          return GRN_SUCCESS;
        } else {
          const char *str = GRN_BULK_HEAD(query);
          unsigned int str_len = GRN_BULK_VSIZE(query);
          rc = grn_ii_sel(ctx, (grn_ii *)obj, str, str_len, (grn_hash *)res, op, optarg);
        }
        break;
      case GRN_QUERY :
        rc = grn_query_search(ctx, (grn_ii *)obj, (grn_query *)query, (grn_hash *)res, op);
        break;
      }
      break;
    }
  }
  GRN_API_RETURN(rc);
}

#define GRN_TABLE_GROUP_BY_KEY           0
#define GRN_TABLE_GROUP_BY_VALUE         1
#define GRN_TABLE_GROUP_BY_COLUMN_VALUE  2

#define GRN_TABLE_GROUP_FILTER_PREFIX    0
#define GRN_TABLE_GROUP_FILTER_SUFFIX    (1L<<2)

static grn_rc grn_view_group(grn_ctx *ctx, grn_obj *table,
                             grn_table_sort_key *keys, int n_keys,
                             grn_table_group_result *results, int n_results);

grn_rc
grn_table_group(grn_ctx *ctx, grn_obj *table,
                grn_table_sort_key *keys, int n_keys,
                grn_table_group_result *results, int n_results)
{
  grn_rc rc = GRN_SUCCESS;
  if (!table || !n_keys || !n_results) {
    ERR(GRN_INVALID_ARGUMENT, "table or n_keys or n_results is void");
    return GRN_INVALID_ARGUMENT;
  }
  GRN_API_ENTER;
  if (table->header.type == GRN_TABLE_VIEW) {
    rc = grn_view_group(ctx, table, keys, n_keys, results, n_results);
  } else {
    int k, r;
    void *key;
    grn_obj bulk;
    grn_table_cursor *tc;
    grn_table_sort_key *kp;
    grn_table_group_result *rp;
    for (k = 0, kp = keys; k < n_keys; k++, kp++) {
      if ((kp->flags & GRN_TABLE_GROUP_BY_COLUMN_VALUE) && !kp->key) {
        ERR(GRN_INVALID_ARGUMENT, "column missing in (%d)", k);
        goto exit;
      }
    }
    for (r = 0, rp = results; r < n_results; r++, rp++) {
      if (!rp->table) {
        ERR(GRN_INVALID_ARGUMENT, "table missing in (%d)", r);
        goto exit;
      }
    }
    GRN_TEXT_INIT(&bulk, 0);
    if (n_keys == 1 && n_results == 1) {
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
        grn_id id;
        grn_obj *range = grn_ctx_at(ctx, grn_obj_get_range(ctx, keys->key));
        int idp = GRN_OBJ_TABLEP(range);
        while ((id = grn_table_cursor_next(ctx, tc))) {
          void *value;
          grn_rset_recinfo *ri = NULL;
          GRN_BULK_REWIND(&bulk);
          if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
            grn_table_cursor_get_value(ctx, tc, (void **)&ri);
          }
          grn_obj_get_value(ctx, keys->key, id, &bulk);
          switch (bulk.header.type) {
          case GRN_UVECTOR :
            {
              // todo : support objects except grn_id
              grn_id *v = (grn_id *)GRN_BULK_HEAD(&bulk);
              grn_id *ve = (grn_id *)GRN_BULK_CURR(&bulk);
              while (v < ve) {
                if ((*v != GRN_ID_NIL) &&
                    grn_table_add_v(ctx, results->table, v, sizeof(grn_id), &value, NULL)) {
                  grn_table_add_subrec(results->table, value, ri ? ri->score : 0, NULL, 0);
                }
                v++;
              }
            }
            break;
          case GRN_VECTOR :
            ERR(GRN_OPERATION_NOT_SUPPORTED, "sorry.. not implemented yet");
            /* todo */
            break;
          case GRN_BULK :
            {
              if ((!idp || *((grn_id *)GRN_BULK_HEAD(&bulk))) &&
                  grn_table_add_v(ctx, results->table,
                                  GRN_BULK_HEAD(&bulk), GRN_BULK_VSIZE(&bulk), &value, NULL)) {
                grn_table_add_subrec(results->table, value, ri ? ri->score : 0, NULL, 0);
              }
            }
            break;
          default :
            ERR(GRN_INVALID_ARGUMENT, "invalid column");
            break;
          }
        }
        grn_table_cursor_close(ctx, tc);
      }
    } else {
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
        grn_id id;
        while ((id = grn_table_cursor_next(ctx, tc))) {
          grn_rset_recinfo *ri = NULL;
          GRN_BULK_REWIND(&bulk);
          if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
            grn_table_cursor_get_value(ctx, tc, (void **)&ri);
          }
          for (k = 0, kp = keys; k < n_keys; k++, kp++) {
            kp->offset = GRN_BULK_VSIZE(&bulk);
            grn_obj_get_value(ctx, kp->key, id, &bulk);
          }
          for (r = 0, rp = results; r < n_results; r++, rp++) {
            void *value;
            int begin = keys[rp->key_begin].offset;
            int end = rp->key_end >= n_keys
              ? GRN_BULK_VSIZE(&bulk)
              : keys[rp->key_end].offset;
            key = GRN_BULK_HEAD(&bulk) + begin;
            // todo : cut off GRN_ID_NIL
            if (grn_table_add_v(ctx, rp->table, key, end - begin, &value, NULL)) {
              grn_table_add_subrec(rp->table, value, ri ? ri->score : 0, NULL, 0);
            }
          }
        }
        grn_table_cursor_close(ctx, tc);
      }
    }
    grn_obj_close(ctx, &bulk);
  }
exit :
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_setoperation(grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res,
                       grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  void *key, *value1, *value2;
  uint32_t value_size = 0;
  uint32_t key_size;
  if (table1 != res) {
    if (table2 == res) {
      grn_obj *t = table1;
      table1 = table2;
      table2 = t;
    } else {
      return GRN_INVALID_ARGUMENT;
    }
  }
  switch (table1->header.type) {
  case GRN_TABLE_HASH_KEY :
    value_size = ((grn_hash *)table1)->value_size;
    break;
  case GRN_TABLE_PAT_KEY :
    value_size = ((grn_pat *)table1)->value_size;
    break;
  case GRN_TABLE_NO_KEY :
    value_size = ((grn_array *)table1)->value_size;
    break;
  }
  switch (table2->header.type) {
  case GRN_TABLE_HASH_KEY :
    if (value_size < ((grn_hash *)table2)->value_size) {
      value_size = ((grn_hash *)table2)->value_size;
    }
    break;
  case GRN_TABLE_PAT_KEY :
    if (value_size < ((grn_pat *)table2)->value_size) {
      value_size = ((grn_pat *)table2)->value_size;
    }
    break;
  case GRN_TABLE_NO_KEY :
    if (value_size < ((grn_array *)table2)->value_size) {
      value_size = ((grn_array *)table2)->value_size;
    }
    break;
  }
  switch (op) {
  case GRN_OP_OR :
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, &value2, {
      if (grn_table_add_v(ctx, table1, key, key_size, &value1, NULL)) {
        memcpy(value1, value2, value_size);
      }
    });
    break;
  case GRN_OP_AND :
    GRN_TABLE_EACH(ctx, table1, 0, 0, id, &key, &key_size, &value1, {
      if (!grn_table_get_v(ctx, table2, key, key_size, &value2)) {
        _grn_table_delete_by_id(ctx, table1, id, NULL);
      }
    });
    break;
  case GRN_OP_BUT :
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, &value2, {
      grn_table_delete(ctx, table1, key, key_size);
    });
    break;
  case GRN_OP_ADJUST :
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, &value2, {
      if (grn_table_get_v(ctx, table1, key, key_size, &value1)) {
        memcpy(value1, value2, value_size);
      }
    });
    break;
  default :
    break;
  }
  return rc;
}

grn_rc
grn_table_difference(grn_ctx *ctx, grn_obj *table1, grn_obj *table2,
                     grn_obj *res1, grn_obj *res2)
{
  void *key;
  uint32_t key_size;
  if (table1 != res1 || table2 != res2) { return GRN_INVALID_ARGUMENT; }
  if (grn_table_size(ctx, table1) > grn_table_size(ctx, table2)) {
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, NULL, {
      grn_id id1;
      if ((id1 = grn_table_get(ctx, table1, key, key_size))) {
        _grn_table_delete_by_id(ctx, table1, id1, NULL);
        _grn_table_delete_by_id(ctx, table2, id, NULL);
      }
    });
  } else {
    GRN_TABLE_EACH(ctx, table1, 0, 0, id, &key, &key_size, NULL, {
      grn_id id2;
      if ((id2 = grn_table_get(ctx, table2, key, key_size))) {
        _grn_table_delete_by_id(ctx, table1, id, NULL);
        _grn_table_delete_by_id(ctx, table2, id2, NULL);
      }
    });
  }
  return GRN_SUCCESS;
}

static grn_obj *grn_obj_get_accessor(grn_ctx *ctx, grn_obj *obj,
                                     const char *name, unsigned name_size);
static grn_obj *grn_view_get_accessor(grn_ctx *ctx, grn_obj *obj,
                                      const char *name, unsigned name_size);

static grn_obj *
grn_obj_column_(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size)
{
  grn_obj *column = NULL;
  char buf[GRN_PAT_MAX_KEY_SIZE];
  int len = grn_obj_name(ctx, table, buf, GRN_PAT_MAX_KEY_SIZE);
  if (len) {
    buf[len++] = GRN_DB_DELIMITER;
    if (len + name_size <= GRN_PAT_MAX_KEY_SIZE) {
      memcpy(buf + len, name, name_size);
      column = grn_ctx_get(ctx, buf, len + name_size);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "name is too long");
    }
  } else {
    /* todo : support temporary table */
  }
  return column;
}

#define ACCESSORP(obj) \
  ((obj) && (((grn_obj *)(obj))->header.type == GRN_ACCESSOR ||\
             ((grn_obj *)(obj))->header.type == GRN_ACCESSOR_VIEW))

grn_obj *
grn_obj_column(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size)
{
  grn_obj *column = NULL;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table)) {
    if (table->header.type == GRN_TABLE_VIEW) {
      column = grn_view_get_accessor(ctx, table, name, name_size);
    } else {
      if (check_name(ctx, name, name_size) ||
          !(column = grn_obj_column_(ctx, table, name, name_size))) {
        column = grn_obj_get_accessor(ctx, table, name, name_size);
      }
    }
  } else if (ACCESSORP(table)) {
    column = grn_obj_get_accessor(ctx, table, name, name_size);
  }
  GRN_API_RETURN(column);
}

int
grn_table_columns(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size,
                  grn_obj *res)
{
  int n = 0;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table) && !(DB_OBJ(table)->id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj bulk;
    grn_db *s = (grn_db *)DB_OBJ(table)->db;
    GRN_TEXT_INIT(&bulk, 0);
    grn_pat_get_key2(ctx, s->keys, DB_OBJ(table)->id, &bulk);
    GRN_TEXT_PUTC(ctx, &bulk, GRN_DB_DELIMITER);
    grn_bulk_write(ctx, &bulk, name, name_size);
    grn_pat_prefix_search(ctx, s->keys, GRN_BULK_HEAD(&bulk), GRN_BULK_VSIZE(&bulk),
                          (grn_hash *)res);
    grn_obj_close(ctx, &bulk);
    n = grn_table_size(ctx, res);
  }
  GRN_API_RETURN(n);
}

const char *
_grn_table_key(grn_ctx *ctx, grn_obj *table, grn_id id, uint32_t *key_size)
{
  GRN_ASSERT(table);
  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY :
    return _grn_hash_key(ctx, (grn_hash *)table, id, key_size);
  case GRN_TABLE_PAT_KEY :
    return _grn_pat_key(ctx, (grn_pat *)table, id, key_size);
  case GRN_TABLE_NO_KEY :
    {
      grn_array *a = (grn_array *)table;
      if (a->obj.header.domain && a->value_size) {
        *key_size = a->value_size;
        return _grn_array_get_value(ctx, a, id);
      }
    }
    break;
  }
  return NULL;
}

/* column */

grn_obj *
grn_column_create(grn_ctx *ctx, grn_obj *table,
                  const char *name, unsigned name_size,
                  const char *path, grn_obj_flags flags, grn_obj *type)
{
  grn_db *s;
  uint32_t value_size;
  grn_obj *db, *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id range = GRN_ID_NIL;
  grn_id domain = GRN_ID_NIL;
  char fullname[GRN_PAT_MAX_KEY_SIZE];
  char buffer[PATH_MAX];
  GRN_API_ENTER;
  if (!table || !type || !name || !name_size) {
    ERR(GRN_INVALID_ARGUMENT, "missing type or name");
    goto exit;
  }
  db = DB_OBJ(table)->db;
  s = (grn_db *)db;
  if (!DB_P(s)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    goto exit;
  }
  if (check_name(ctx, name, name_size)) {
    CHECK_NAME_ERR();
    goto exit;
  }
  if ((domain = DB_OBJ(table)->id)) {
    int len = grn_pat_get_key(ctx, s->keys, domain,
                              fullname, GRN_PAT_MAX_KEY_SIZE);
    if (name_size + 1 + len > GRN_PAT_MAX_KEY_SIZE) {
      ERR(GRN_INVALID_ARGUMENT, "too long column name");
      goto exit;
    }
    fullname[len] = GRN_DB_DELIMITER;
    memcpy(fullname + len + 1, name, name_size);
    name_size += len + 1;
  } else {
    ERR(GRN_INVALID_ARGUMENT, "todo : not supported yet");
    goto exit;
  }
  range = DB_OBJ(type)->id;
  switch (type->header.type) {
  case GRN_TYPE :
    {
      grn_db_obj *t = (grn_db_obj *)type;
      flags |= t->header.flags;
      value_size = GRN_TYPE_SIZE(t);
    }
    break;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_NO_KEY :
  case GRN_TABLE_PAT_KEY :
    value_size = sizeof(grn_id);
    break;
  default :
    /*
    if (type == grn_type_any) {
      value_size = sizeof(grn_id) + sizeof(grn_id);
    }
    */
    value_size = sizeof(grn_id);
  }
  id = grn_obj_register(ctx, db, fullname, name_size);
  if (ERRP(ctx, GRN_ERROR)) { goto exit;  }
  if (GRN_OBJ_PERSISTENT & flags) {
    if (!path) {
      if (PERSISTENT_DB_P(db)) {
        gen_pathname(s->keys->io->path, buffer, id);
        path = buffer;
      } else {
        ERR(GRN_INVALID_ARGUMENT, "path not assigned for persistent table");
        goto exit;
      }
    } else {
      flags |= GRN_OBJ_CUSTOM_NAME;
    }
  } else {
    if (path) {
      ERR(GRN_INVALID_ARGUMENT, "path assigned for temporary table");
      goto exit;
    }
  }
  switch (flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_SCALAR :
    if ((flags & GRN_OBJ_KEY_VAR_SIZE) || value_size > sizeof(int64_t)) {
      res = (grn_obj *)grn_ja_create(ctx, path, value_size, flags);
    } else {
      res = (grn_obj *)grn_ra_create(ctx, path, value_size);
    }
    break;
  case GRN_OBJ_COLUMN_VECTOR :
    res = (grn_obj *)grn_ja_create(ctx, path, value_size * 16/*todo*/, flags);
    //todo : zlib support
    break;
  case GRN_OBJ_COLUMN_INDEX :
    res = (grn_obj *)grn_ii_create(ctx, path, table, flags); //todo : ii layout support
    break;
  }
  if (res) {
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->header.impl_flags = 0;
    DB_OBJ(res)->range = range;
    DB_OBJ(res)->header.flags = flags;
    res->header.flags = flags;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
      grn_obj_remove(ctx, res);
      res = NULL;
    }
  }
exit :
  if (!res && id) { grn_obj_delete_by_id(ctx, db, id, 1); }
  GRN_API_RETURN(res);
}

grn_obj *
grn_column_open(grn_ctx *ctx, grn_obj *table,
                const char *name, unsigned name_size,
                const char *path, grn_obj *type)
{
  grn_id domain;
  grn_obj *res = NULL;
  grn_db *s;
  char fullname[GRN_PAT_MAX_KEY_SIZE];
  GRN_API_ENTER;
  if (!table || !type || !name || !name_size) {
    ERR(GRN_INVALID_ARGUMENT, "missing type or name");
    goto exit;
  }
  s = (grn_db *)DB_OBJ(table)->db;
  if (!DB_P(s)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    goto exit;
  }
  if (check_name(ctx, name, name_size)) {
    CHECK_NAME_ERR();
    goto exit;
  }
  if ((domain = DB_OBJ(table)->id)) {
    int len = grn_pat_get_key(ctx, s->keys, domain, fullname, GRN_PAT_MAX_KEY_SIZE);
    if (name_size + 1 + len > GRN_PAT_MAX_KEY_SIZE) {
      ERR(GRN_INVALID_ARGUMENT, "too long column name");
      goto exit;
    }
    fullname[len] = GRN_DB_DELIMITER;
    memcpy(fullname + len + 1, name, name_size);
    name_size += len + 1;
  } else {
    ERR(GRN_INVALID_ARGUMENT, "todo : not supported yet");
    goto exit;
  }
  res = grn_ctx_get(ctx, fullname, name_size);
  if (res) {
    const char *path2 = grn_obj_path(ctx, res);
    if (path && (!path2 || strcmp(path, path2))) { goto exit; }
  } else if (path) {
    uint32_t dbtype = grn_io_detect_type(ctx, path);
    if (!dbtype) { goto exit; }
    switch (dbtype) {
    case GRN_COLUMN_VAR_SIZE :
      res = (grn_obj *)grn_ja_open(ctx, path);
      break;
    case GRN_COLUMN_FIX_SIZE :
      res = (grn_obj *)grn_ra_open(ctx, path);
      break;
    case GRN_COLUMN_INDEX :
      res = (grn_obj *)grn_ii_open(ctx, path, table);
      break;
    }
    if (res) {
      grn_id id = grn_obj_register(ctx, (grn_obj *)s, fullname, name_size);
      DB_OBJ(res)->header.domain = domain;
      DB_OBJ(res)->range = DB_OBJ(type)->id;
      res->header.flags |= GRN_OBJ_CUSTOM_NAME;
      grn_db_obj_init(ctx, (grn_obj *)s, id, DB_OBJ(res));
    }
  }
exit :
  GRN_API_RETURN(res);
}

/*
typedef struct {
  grn_id id;
  int flags;
} grn_column_set_value_arg;

static grn_rc
default_column_set_value(grn_ctx *ctx, grn_proc_ctx *pctx, grn_obj *in, grn_obj *out)
{
  grn_user_data *data = grn_proc_ctx_get_local_data(pctx);
  if (data) {
    grn_column_set_value_arg *arg = data->ptr;
    unsigned int value_size = in->u.p.size; //todo
    if (!pctx->obj) { return GRN_ID_NIL; }
    switch (pctx->obj->header.type) {
    case GRN_COLUMN_VAR_SIZE :
      return grn_ja_put(ctx, (grn_ja *)pctx->obj, arg->id,
                        in->u.p.ptr, value_size, 0); // todo type->flag
    case GRN_COLUMN_FIX_SIZE :
      if (((grn_ra *)pctx->obj)->header->element_size < value_size) {
        ERR(GRN_INVALID_ARGUMENT, "too long value (%d)", value_size);
        return GRN_INVALID_ARGUMENT;
      } else {
        void *v = grn_ra_ref(ctx, (grn_ra *)pctx->obj, arg->id);
        if (!v) {
          ERR(GRN_NO_MEMORY_AVAILABLE, "ra get failed");
          return GRN_NO_MEMORY_AVAILABLE;
        }
        memcpy(v, in->u.p.ptr, value_size);
        grn_ra_unref(ctx, (grn_ra *)pctx->obj, arg->id);
      }
      break;
    case GRN_COLUMN_INDEX :
      // todo : how??
      break;
    }
    return GRN_SUCCESS;
  } else {
    ERR(GRN_OBJECT_CORRUPT, "grn_proc_ctx_get_local_data failed");
    return ctx->rc;
  }
}
*/

/**** grn_vector ****/

//#define VECTOR(obj) ((grn_vector *)obj)

/*
#define INITIAL_VECTOR_SIZE 256

int
grn_vector_delimit(grn_ctx *ctx, grn_obj *vector)
{
  grn_vector *v = VECTOR(vector);
  uint32_t *offsets;
  if (!(v->n_entries & (INITIAL_VECTOR_SIZE - 1))) {
    offsets = GRN_REALLOC(v->offsets, sizeof(uint32_t) *
                          (v->n_entries + INITIAL_VECTOR_SIZE));
    if (!offsets) { return -1; }
    v->offsets = offsets;
  }
  v->offsets[v->n_entries] = GRN_BULK_VSIZE(vector);
  return ++(v->n_entries);
}
*/

unsigned int
grn_vector_size(grn_ctx *ctx, grn_obj *vector)
{
  unsigned int size;
  if (!vector) {
    ERR(GRN_INVALID_ARGUMENT, "vector is null");
    return 0;
  }
  GRN_API_ENTER;
  switch (vector->header.type) {
  case GRN_BULK :
    size = GRN_BULK_VSIZE(vector);
    break;
  case GRN_UVECTOR :
    size = GRN_BULK_VSIZE(vector) / sizeof(grn_id);
    break;
  case GRN_VECTOR :
    size = vector->u.v.n_sections;
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT, "not vector");
    size = 0;
    break;
  }
  GRN_API_RETURN(size);
}

static grn_obj *
grn_vector_body(grn_ctx *ctx, grn_obj *v)
{
  if (!v) {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument");
    return NULL;
  }
  switch (v->header.type) {
  case GRN_VECTOR :
    if (!v->u.v.body) {
      v->u.v.body = grn_obj_open(ctx, GRN_BULK, 0, v->header.domain);
    }
    return v->u.v.body;
  case GRN_BULK :
  case GRN_UVECTOR :
    return v;
  default :
    return NULL;
  }
}

unsigned int
grn_vector_get_element(grn_ctx *ctx, grn_obj *vector,
                       unsigned int offset, const char **str,
                       unsigned int *weight, grn_id *domain)
{
  unsigned int length = 0;
  GRN_API_ENTER;
  if (!vector || vector->header.type != GRN_VECTOR) {
    ERR(GRN_INVALID_ARGUMENT, "invalid vector");
    goto exit;
  }
  if (vector->u.v.n_sections <= offset) {
    ERR(GRN_RANGE_ERROR, "offset out of range");
    goto exit;
  }
  {
    grn_section *vp = &vector->u.v.sections[offset];
    grn_obj *body = grn_vector_body(ctx, vector);
    *str = GRN_BULK_HEAD(body) + vp->offset;
    if (weight) { *weight = vp->weight; }
    if (domain) { *domain = vp->domain; }
    length = vp->length;
  }
exit :
  GRN_API_RETURN(length);
}

#define W_SECTIONS_UNIT 8
#define S_SECTIONS_UNIT (1 << W_SECTIONS_UNIT)
#define M_SECTIONS_UNIT (S_SECTIONS_UNIT - 1)

grn_rc
grn_vector_delimit(grn_ctx *ctx, grn_obj *v, unsigned int weight, grn_id domain)
{
  if (v->header.type != GRN_VECTOR) { return GRN_INVALID_ARGUMENT; }
  if (!(v->u.v.n_sections & M_SECTIONS_UNIT)) {
    grn_section *vp = GRN_REALLOC(v->u.v.sections, sizeof(grn_section) *
                                  (v->u.v.n_sections + S_SECTIONS_UNIT));
    if (!vp) { return GRN_NO_MEMORY_AVAILABLE; }
    v->u.v.sections = vp;
  }
  {
    grn_obj *body = grn_vector_body(ctx, v);
    grn_section *vp = &v->u.v.sections[v->u.v.n_sections];
    vp->offset = v->u.v.n_sections ? vp[-1].offset + vp[-1].length : 0;
    vp->length = GRN_BULK_VSIZE(body) - vp->offset;
    vp->weight = weight;
    vp->domain = domain;
  }
  v->u.v.n_sections++;
  return GRN_SUCCESS;
}

grn_rc
grn_vector_decode(grn_ctx *ctx, grn_obj *v, const char *data, uint32_t data_size)
{
  uint8_t *p = (uint8_t *)data;
  uint8_t *pe = p + data_size;
  uint32_t n, n0 = v->u.v.n_sections;
  GRN_B_DEC(n, p);
  if (((n0 + M_SECTIONS_UNIT) >> W_SECTIONS_UNIT) !=
      ((n0 + n + M_SECTIONS_UNIT) >> W_SECTIONS_UNIT)) {
    grn_section *vp = GRN_REALLOC(v->u.v.sections, sizeof(grn_section) *
                                  ((n0 + n + M_SECTIONS_UNIT) & ~M_SECTIONS_UNIT));
    if (!vp) { return GRN_NO_MEMORY_AVAILABLE; }
    v->u.v.sections = vp;
  }
  {
    grn_section *vp;
    uint32_t o = 0, l, i;
    for (i = n, vp = v->u.v.sections + n0; i; i--, vp++) {
      if (pe <= p) { return GRN_INVALID_ARGUMENT; }
      GRN_B_DEC(l, p);
      vp->length = l;
      vp->offset = o;
      vp->weight = 0;
      vp->domain = 0;
      o += l;
    }
    if (pe < p + o) { return GRN_INVALID_ARGUMENT; }
    {
      grn_obj *body = grn_vector_body(ctx, v);
      grn_bulk_write(ctx, body, (char *)p, o);
    }
    p += o;
    if (p < pe) {
      for (i = n, vp = v->u.v.sections + n0; i; i--, vp++) {
        if (pe <= p) { return GRN_INVALID_ARGUMENT; }
        GRN_B_DEC(vp->weight, p);
        GRN_B_DEC(vp->domain, p);
      }
    }
  }
  v->u.v.n_sections += n;
  return ctx->rc;
}

grn_rc
grn_vector_add_element(grn_ctx *ctx, grn_obj *vector,
                       const char *str, unsigned int str_len,
                       unsigned int weight, grn_id domain)
{
  grn_obj *body;
  GRN_API_ENTER;
  if (!vector) {
    ERR(GRN_INVALID_ARGUMENT, "vector is null");
    goto exit;
  }
  if ((body = grn_vector_body(ctx, vector))) {
    grn_bulk_write(ctx, body, str, str_len);
    grn_vector_delimit(ctx, vector, weight, domain);
  }
exit :
  GRN_API_RETURN(ctx->rc);
}

/*
grn_obj *
grn_sections_to_vector(grn_ctx *ctx, grn_obj *sections)
{
  grn_obj *vector = grn_vector_open(ctx, 0);
  if (vector) {
    grn_section *vp;
    int i;
    for (i = sections->u.v.n_sections, vp = sections->u.v.sections; i; i--, vp++) {
      grn_text_benc(ctx, vector, vp->weight);
      grn_text_benc(ctx, vector, vp->domain);
      grn_bulk_write(ctx, vector, vp->str, vp->str_len);
      grn_vector_delimit(ctx, vector);
    }
  }
  return vector;
}

grn_obj *
grn_vector_to_sections(grn_ctx *ctx, grn_obj *vector, grn_obj *sections)
{
  if (!sections) {
    sections = grn_obj_open(ctx, GRN_VECTOR, GRN_OBJ_DO_SHALLOW_COPY, 0);
  }
  if (sections) {
    int i, n = grn_vector_size(ctx, vector);
    sections->u.v.src = vector;
    for (i = 0; i < n; i++) {
      unsigned int size;
      const uint8_t *pe, *p = (uint8_t *)grn_vector_fetch(ctx, vector, i, &size);
      if (p) {
        grn_id domain;
        unsigned int weight;
        pe = p + size;
        if (p < pe) {
          GRN_B_DEC(weight, p);
          if (p < pe) {
            GRN_B_DEC(domain, p);
            if (p <= pe) {
              grn_vector_add(ctx, sections, (char *)p, pe - p, weight, domain);
            }
          }
        }
      }
    }
  }
  return sections;
}
*/

/**** accessor ****/

typedef struct _grn_accessor grn_accessor;

struct _grn_accessor {
  grn_obj_header header;
  grn_id range;
  /* -- compatible with grn_db_obj -- */
  uint8_t action;
  int offset;
  grn_obj *obj;
  grn_accessor *next;
};

enum {
  GRN_ACCESSOR_VOID = 0,
  GRN_ACCESSOR_GET_ID,
  GRN_ACCESSOR_GET_KEY,
  GRN_ACCESSOR_GET_VALUE,
  GRN_ACCESSOR_GET_SCORE,
  GRN_ACCESSOR_GET_NSUBRECS,
  GRN_ACCESSOR_GET_COLUMN_VALUE,
  GRN_ACCESSOR_GET_DB_OBJ,
  GRN_ACCESSOR_LOOKUP,
  GRN_ACCESSOR_FUNCALL
};

static grn_accessor *
accessor_new(grn_ctx *ctx)
{
  grn_accessor *res = GRN_MALLOCN(grn_accessor, 1);
  if (res) {
    res->header.type = GRN_ACCESSOR;
    res->header.impl_flags = GRN_OBJ_ALLOCATED;
    res->header.flags = 0;
    res->header.domain = GRN_ID_NIL;
    res->action = GRN_ACCESSOR_VOID;
    res->offset = 0;
    res->next = NULL;
  }
  return res;
}

static grn_obj *
grn_obj_get_accessor(grn_ctx *ctx, grn_obj *obj, const char *name, unsigned name_size)
{
  grn_accessor *res = NULL, **rp = NULL, **rp0 = NULL;
  if (!obj) { return NULL; }
  GRN_API_ENTER;
  // todo : support GRN_ACCESSOR_VIEW
  if (obj->header.type == GRN_ACCESSOR) {
    for (rp0 = (grn_accessor **)&obj; *rp0; rp0 = &(*rp0)->next) {
      res = *rp0;
    }
    switch (res->action) {
    case GRN_ACCESSOR_GET_KEY :
      obj = grn_ctx_at(ctx, res->obj->header.domain);
      break;
    case GRN_ACCESSOR_GET_VALUE :
    case GRN_ACCESSOR_GET_SCORE :
    case GRN_ACCESSOR_GET_NSUBRECS :
      obj = grn_ctx_at(ctx, DB_OBJ(res->obj)->range);
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      obj = grn_ctx_at(ctx, DB_OBJ(res->obj)->range);
      break;
    case GRN_ACCESSOR_LOOKUP :
      /* todo */
      break;
    case GRN_ACCESSOR_FUNCALL :
      /* todo */
      break;
    }
  }
  {
    size_t len;
    const char *sp, *se = name + name_size;
    if (*name == GRN_DB_DELIMITER) { name++; }
    for (sp = name; (len = grn_charlen(ctx, sp, se)); sp += len) {
      if (*sp == GRN_DB_DELIMITER) { break; }
    }
    if (!(len = sp - name)) { goto exit; }
    if (*name == GRN_DB_PSEUDO_COLUMN_PREFIX) { /* pseudo column */
      int done = 0;
      if (len < 2) { goto exit; }
      switch (name[1]) {
      case 'k' : /* key */
        if (len != 4 || memcmp(name, "_key", 4)) { goto exit; }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            goto exit;
          }
          switch (obj->header.type) {
          case GRN_DB :
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            rp = &(*rp)->next;
            *rp = accessor_new(ctx);
            (*rp)->obj = obj;
            (*rp)->action = GRN_ACCESSOR_GET_DB_OBJ;
            done++;
            break;
          case GRN_TYPE :
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            done++;
            break;
          case GRN_TABLE_PAT_KEY :
          case GRN_TABLE_HASH_KEY :
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            break;
          case GRN_TABLE_NO_KEY :
            if (obj->header.domain) {
              (*rp)->action = GRN_ACCESSOR_GET_VALUE;
              break;
            }
            /* fallthru */
          default :
            /* lookup failed */
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            goto exit;
          }
        }
        break;
      case 'i' : /* id */
        if (len != 3 || memcmp(name, "_id", 3)) { goto exit; }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          if (!obj->header.domain) {
            (*rp)->action = GRN_ACCESSOR_GET_ID;
            done++;
          } else {
            obj = grn_ctx_at(ctx, obj->header.domain);
            switch (obj->header.type) {
            case GRN_DB :
            case GRN_TYPE :
              (*rp)->action = GRN_ACCESSOR_GET_ID;
              done++;
              break;
            case GRN_TABLE_PAT_KEY :
            case GRN_TABLE_HASH_KEY :
            case GRN_TABLE_NO_KEY :
              (*rp)->action = GRN_ACCESSOR_GET_KEY;
              break;
            default :
              /* lookup failed */
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
          }
        }
        break;
      case 'v' : /* value */
        if (len != 6 || memcmp(name, "_value", 6)) { goto exit; }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          if (!obj->header.domain) {
            if (DB_OBJ((*rp)->obj)->range) {
              (*rp)->action = GRN_ACCESSOR_GET_VALUE;
              done++;
            } else {
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
            done++;
          } else {
            obj = grn_ctx_at(ctx, obj->header.domain);
            switch (obj->header.type) {
            case GRN_DB :
            case GRN_TYPE :
              if (DB_OBJ((*rp)->obj)->range) {
                (*rp)->action = GRN_ACCESSOR_GET_VALUE;
                done++;
              } else {
                grn_obj_close(ctx, (grn_obj *)res);
                res = NULL;
                goto exit;
              }
              break;
            case GRN_TABLE_PAT_KEY :
            case GRN_TABLE_HASH_KEY :
            case GRN_TABLE_NO_KEY :
             (*rp)->action = GRN_ACCESSOR_GET_KEY;
              break;
            default :
              /* lookup failed */
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
          }
        }
        break;
      case 's' : /* score */
        if (len != 6 || memcmp(name, "_score", 6)) { goto exit; }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          if (DB_OBJ(obj)->header.flags & GRN_OBJ_WITH_SUBREC) {
            (*rp)->action = GRN_ACCESSOR_GET_SCORE;
            done++;
          } else {
            switch (obj->header.type) {
            case GRN_TABLE_PAT_KEY :
            case GRN_TABLE_HASH_KEY :
              (*rp)->action = GRN_ACCESSOR_GET_KEY;
              break;
            case GRN_TABLE_NO_KEY :
              if (obj->header.domain) {
                (*rp)->action = GRN_ACCESSOR_GET_VALUE;
                break;
              }
              /* fallthru */
            default :
              /* lookup failed */
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
            obj = grn_ctx_at(ctx, obj->header.domain);
          }
        }
        break;
      case 'n' : /* nsubrecs */
        if (len != 9 || memcmp(name, "_nsubrecs", 9)) { goto exit; }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          if (DB_OBJ(obj)->header.flags & GRN_OBJ_WITH_SUBREC) {
            (*rp)->action = GRN_ACCESSOR_GET_NSUBRECS;
            done++;
          } else {
            switch (obj->header.type) {
            case GRN_TABLE_PAT_KEY :
            case GRN_TABLE_HASH_KEY :
              (*rp)->action = GRN_ACCESSOR_GET_KEY;
              break;
            case GRN_TABLE_NO_KEY :
              if (obj->header.domain) {
                (*rp)->action = GRN_ACCESSOR_GET_VALUE;
                break;
              }
              /* fallthru */
            default :
              /* lookup failed */
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
            obj = grn_ctx_at(ctx, obj->header.domain);
          }
        }
        break;
      default :
        res = NULL;
        goto exit;
      }
    } else {
      /* if obj->header.type == GRN_TYPE ... lookup table */
      for (rp = &res; ; rp = &(*rp)->next) {
        grn_obj *column = grn_obj_column_(ctx, obj, name, len);
        if (column) {
          *rp = accessor_new(ctx);
          (*rp)->obj = column;
          /*
          switch (column->header.type) {
          case GRN_COLUMN_VAR_SIZE :
            break;
          case GRN_COLUMN_FIX_SIZE :
            break;
          case GRN_COLUMN_INDEX :
            break;
          }
          */
          (*rp)->action = GRN_ACCESSOR_GET_COLUMN_VALUE;
          break;
        } else {
          if (!obj->header.domain) {
            // ERR(GRN_INVALID_ARGUMENT, "no such column: <%s>", name);
            res = NULL;
            goto exit;
          }
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          obj = grn_ctx_at(ctx, obj->header.domain);
          switch (obj->header.type) {
          case GRN_TABLE_PAT_KEY :
          case GRN_TABLE_HASH_KEY :
          case GRN_TABLE_NO_KEY :
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            break;
          default :
            /* lookup failed */
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            goto exit;
          }
        }
      }
    }
    if (sp != se) { grn_obj_get_accessor(ctx, (grn_obj *)res, sp, se - sp); }
  }
  if (rp0) { *rp0 = res; }
 exit :
  GRN_API_RETURN((grn_obj *)res);
}

typedef struct _grn_accessor_view grn_accessor_view;

struct _grn_accessor_view {
  grn_obj_header header;
  grn_id range;
  /* -- compatible with grn_db_obj -- */
  uint32_t naccessors;
  grn_obj **accessors;
};

static grn_obj *
grn_view_get_accessor(grn_ctx *ctx, grn_obj *obj, const char *name, unsigned name_size)
{
  int n = 0;
  grn_id *tp;
  grn_obj **ap;
  grn_view *v = (grn_view *)obj;
  grn_hash *hash = v->hash;
  grn_accessor_view *a = GRN_MALLOCN(grn_accessor_view, 1);
  if (!a) { return NULL; }
  a->header.type = GRN_ACCESSOR_VIEW;
  a->header.impl_flags = GRN_OBJ_ALLOCATED;
  a->header.flags = 0;
  a->header.domain = DB_OBJ(obj)->id;
  a->range = GRN_ID_NIL;
  a->naccessors = GRN_HASH_SIZE(hash);
  ap = a->accessors = GRN_MALLOCN(grn_obj *, a->naccessors);
  GRN_HASH_EACH(ctx, hash, id, &tp, NULL, NULL, {
    grn_obj *table = grn_ctx_at(ctx, *tp);
    grn_obj *column = grn_obj_column(ctx, table, name, name_size);
    *ap++ = column;
    if (column) { n++; }
  });
  if (!n) {
    GRN_FREE(a->accessors);
    GRN_FREE(a);
    a = NULL;
  }
  return (grn_obj *)a;
}

static grn_rc
grn_accessor_view_close(grn_ctx *ctx, grn_obj *obj)
{
  int i;
  grn_accessor_view *a = (grn_accessor_view *)obj;
  for (i = 0; i < a->naccessors; i++) {
    grn_obj_unlink(ctx, a->accessors[i]);
  }
  GRN_FREE(a->accessors);
  GRN_FREE(a);
  return ctx->rc;
}

grn_obj *
grn_table_create_for_group(grn_ctx *ctx, const char *name, unsigned name_size,
                           const char *path, grn_obj_flags flags,
                           grn_obj *group_key, grn_obj *value_type)
{
  if (group_key->header.type != GRN_ACCESSOR_VIEW) {
    grn_obj *key_type = grn_ctx_at(ctx, grn_obj_get_range(ctx, group_key));
    return grn_table_create(ctx, name, name_size, path, flags, key_type, value_type);
  } else {
    int n;
    grn_obj **ap;
    grn_accessor_view *a = (grn_accessor_view *)group_key;
    grn_obj *res = grn_table_create(ctx, name, name_size, path,
                                    (flags & ~GRN_OBJ_TABLE_TYPE_MASK)|GRN_OBJ_TABLE_VIEW,
                                    NULL, value_type);
    if (res) {
      for (n = a->naccessors, ap = a->accessors; n; n--, ap++) {
        grn_view_add(ctx, res,
                     grn_table_create_for_group(ctx, NULL, 0, NULL, flags, *ap, value_type));
      }
    }
    return res;
  }
}

static grn_rc
grn_view_group(grn_ctx *ctx, grn_obj *table,
               grn_table_sort_key *keys, int n_keys,
               grn_table_group_result *results, int n_results)
{
  if (n_keys != 1 || n_results != 1) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  } else {
    grn_obj *t, *r;
    grn_id *tp, rid;
    grn_view *tv = (grn_view *)table;
    grn_view *rv = (grn_view *)results->table;
    grn_hash *th = tv->hash;
    grn_hash *rh = rv->hash;
    grn_table_sort_key *keys_ = GRN_MALLOCN(grn_table_sort_key, n_keys);
    grn_table_group_result *results_ = GRN_MALLOCN(grn_table_group_result, n_results);
    grn_table_sort_key *ks, *kd, *ke = keys + n_keys;
    if (keys_) {
      if (results_) {
        memcpy(results_, results, sizeof(grn_table_group_result) * n_results);
        for (ks = keys, kd =keys_; ks < ke ; ks++, kd++) { kd->flags = ks->flags; }
        GRN_HASH_EACH(ctx, th, id, &tp, NULL, NULL, {
          grn_hash_get_key(ctx, rh, id, &rid, sizeof(grn_id));
          t = grn_ctx_at(ctx, *tp);
          r = grn_ctx_at(ctx, rid);
          for (ks = keys, kd =keys_; ks < ke ; ks++, kd++) {
            kd->key = ((grn_accessor_view *)ks->key)->accessors[id - 1];
          }
          results_->table = r;
          /* todo : sampling */
          grn_table_group(ctx, t, keys_, n_keys, results_, n_results);
        });
        /* todo : merge */
        GRN_FREE(results_);
      }
      GRN_FREE(keys_);
    }
  }
  return GRN_SUCCESS;
}

grn_id
grn_obj_get_range(grn_ctx *ctx, grn_obj *obj)
{
  grn_id range = GRN_ID_NIL;
  if (GRN_DB_OBJP(obj)) {
    range = DB_OBJ(obj)->range;
  } else if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID :
        range = GRN_DB_UINT32;
        break;
      case GRN_ACCESSOR_GET_VALUE :
      case GRN_ACCESSOR_GET_SCORE :
      case GRN_ACCESSOR_GET_NSUBRECS :
        range = GRN_DB_INT32;
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE :
        if (GRN_DB_OBJP(a->obj)) { range = DB_OBJ(a->obj)->range; }
        break;
      case GRN_ACCESSOR_GET_KEY :
        if (GRN_DB_OBJP(a->obj)) { range = DB_OBJ(a->obj)->header.domain; }
        break;
      default :
        if (GRN_DB_OBJP(a->obj)) { range = DB_OBJ(a->obj)->range; }
        break;
      }
    }
  } else if (obj->header.type == GRN_ACCESSOR_VIEW) {
    range = GRN_DB_OBJECT;
  }
  return range;
}

#define NUM2DEST(getvalue,totext,tobool)\
  switch (dest->header.domain) {\
  case GRN_DB_BOOL :\
    tobool(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_INT8 :\
    GRN_INT8_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_UINT8 :\
    GRN_UINT8_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_INT16 :\
    GRN_INT16_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_UINT16 :\
    GRN_UINT16_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_INT32 :\
    GRN_INT32_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_UINT32 :\
    GRN_UINT32_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_TIME :\
    GRN_TIME_SET(ctx, dest, (long long int)(getvalue(src)) * GRN_TIME_USEC_PER_SEC);\
    break;\
  case GRN_DB_INT64 :\
    GRN_INT64_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_UINT64 :\
    GRN_UINT64_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_FLOAT :\
    GRN_FLOAT_SET(ctx, dest, getvalue(src));\
    break;\
  case GRN_DB_SHORT_TEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONG_TEXT :\
    totext(ctx, dest, getvalue(src));\
    break;\
  default :\
    rc = GRN_FUNCTION_NOT_IMPLEMENTED;\
    break;\
  }

#define TEXT2DEST(type,tonum,setvalue) {\
  const char *cur, *str = GRN_TEXT_VALUE(src);\
  const char *str_end = GRN_BULK_CURR(src);\
  type i = tonum(str, str_end, &cur);\
  if (cur == str_end) {\
    setvalue(ctx, dest, i);\
  } else if (cur != str) {\
    double d;\
    char *end;\
    grn_obj buf;\
    GRN_TEXT_INIT(&buf, 0);\
    GRN_TEXT_PUT(ctx, &buf, str, GRN_TEXT_LEN(src));\
    GRN_TEXT_PUTC(ctx, &buf, '\0');\
    errno = 0;\
    d = strtod(GRN_TEXT_VALUE(&buf), &end);\
    if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {\
      setvalue(ctx, dest, d);\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    GRN_OBJ_FIN(ctx, &buf);\
  } else {\
    rc = GRN_INVALID_ARGUMENT;\
  }\
}

#define NUM2BOOL(ctx, dest, value) GRN_BOOL_SET(ctx, dest, value != 0)
#define FLOAT2BOOL(ctx, dest, value)\
  {\
    double value_ = value;\
    GRN_BOOL_SET(ctx, dest, value_ < -DBL_EPSILON || DBL_EPSILON < value_);\
  }


grn_rc
grn_obj_cast(grn_ctx *ctx, grn_obj *src, grn_obj *dest, int addp)
{
  grn_rc rc = GRN_SUCCESS;
  switch (src->header.domain) {
  case GRN_DB_INT32 :
    NUM2DEST(GRN_INT32_VALUE, grn_text_itoa, NUM2BOOL);
    break;
  case GRN_DB_UINT32 :
    NUM2DEST(GRN_UINT32_VALUE, grn_text_lltoa, NUM2BOOL);
    break;
  case GRN_DB_INT64 :
    NUM2DEST(GRN_INT64_VALUE, grn_text_lltoa, NUM2BOOL);
    break;
  case GRN_DB_TIME :
    NUM2DEST(GRN_TIME_VALUE, grn_text_lltoa, NUM2BOOL);
    break;
  case GRN_DB_UINT64 :
    NUM2DEST(GRN_UINT64_VALUE, grn_text_lltoa, NUM2BOOL);
    break;
  case GRN_DB_FLOAT :
    NUM2DEST(GRN_FLOAT_VALUE, grn_text_ftoa, FLOAT2BOOL);
    break;
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    switch (dest->header.domain) {
    case GRN_DB_BOOL :
      GRN_BOOL_SET(ctx, dest, GRN_TEXT_LEN(src) > 0);
      break;
    case GRN_DB_INT8 :
      TEXT2DEST(int8_t, grn_atoi8, GRN_INT8_SET);
      break;
    case GRN_DB_UINT8 :
      TEXT2DEST(uint8_t, grn_atoui8, GRN_UINT8_SET);
      break;
    case GRN_DB_INT16 :
      TEXT2DEST(int16_t, grn_atoi16, GRN_INT16_SET);
      break;
    case GRN_DB_UINT16 :
      TEXT2DEST(uint16_t, grn_atoui16, GRN_UINT16_SET);
      break;
    case GRN_DB_INT32 :
      TEXT2DEST(int32_t, grn_atoi, GRN_INT32_SET);
      break;
    case GRN_DB_UINT32 :
      TEXT2DEST(uint32_t, grn_atoui, GRN_UINT32_SET);
      break;
    case GRN_DB_TIME :
      {
        grn_timeval v;
        int len = GRN_TEXT_LEN(src);
        char *str = GRN_TEXT_VALUE(src);
        if (grn_str2timeval(str, len, &v)) {
          double d;
          char *end;
          grn_obj buf;
          GRN_TEXT_INIT(&buf, 0);
          GRN_TEXT_PUT(ctx, &buf, str, len);
          GRN_TEXT_PUTC(ctx, &buf, '\0');
          errno = 0;
          d = strtod(GRN_TEXT_VALUE(&buf), &end);
          if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
            v.tv_sec = d;
            v.tv_usec = ((d - v.tv_sec) * GRN_TIME_USEC_PER_SEC);
          } else {
            rc = GRN_INVALID_ARGUMENT;
          }
          GRN_OBJ_FIN(ctx, &buf);
        }
        GRN_TIME_SET(ctx, dest, GRN_TIME_PACK((int64_t)v.tv_sec, v.tv_usec));
      }
      break;
    case GRN_DB_INT64 :
      TEXT2DEST(int64_t, grn_atoll, GRN_INT64_SET);
      break;
    case GRN_DB_UINT64 :
      TEXT2DEST(int64_t, grn_atoll, GRN_UINT64_SET);
      break;
    case GRN_DB_FLOAT :
      {
        double d;
        char *end;
        grn_obj buf;
        GRN_TEXT_INIT(&buf, 0);
        GRN_TEXT_PUT(ctx, &buf, GRN_TEXT_VALUE(src), GRN_TEXT_LEN(src));
        GRN_TEXT_PUTC(ctx, &buf, '\0');
        errno = 0;
        d = strtod(GRN_TEXT_VALUE(&buf), &end);
        if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
          GRN_FLOAT_SET(ctx, dest, d);
        } else {
          rc = GRN_INVALID_ARGUMENT;
        }
        GRN_OBJ_FIN(ctx, &buf);
      }
      break;
    case GRN_DB_SHORT_TEXT :
    case GRN_DB_TEXT :
    case GRN_DB_LONG_TEXT :
      GRN_TEXT_PUT(ctx, dest, GRN_TEXT_VALUE(src), GRN_TEXT_LEN(src));
      break;
    case GRN_DB_TOKYO_GEO_POINT :
    case GRN_DB_WGS84_GEO_POINT :
      {
        grn_geo_point gp;
        const char *cur, *str = GRN_TEXT_VALUE(src);
        const char *str_end = GRN_BULK_CURR(src);
        gp.latitude = grn_atoi(str, str_end, &cur);
        if (cur + 1 < str_end) {
          gp.longitude = grn_atoi(cur + 1, str_end, &cur);
          if (cur == str_end) {
            GRN_GEOPOINT_SET(ctx, dest, gp);
          } else {
            rc = GRN_INVALID_ARGUMENT;
          }
        } else {
          rc = GRN_INVALID_ARGUMENT;
        }
      }
      break;
    default :
      {
        grn_obj *table = grn_ctx_at(ctx, dest->header.domain);
        if (GRN_OBJ_TABLEP(table)) {
          grn_obj *p_key = src;
          grn_id id;
          if (table->header.type != GRN_TABLE_NO_KEY) {
            grn_obj key;
            GRN_OBJ_INIT(&key, GRN_BULK, 0, table->header.domain);
            if (src->header.domain != table->header.domain) {
               grn_obj_cast(ctx, src, &key, 1);
               p_key = &key;
            }
            id = addp ? grn_table_add_by_key(ctx, table, p_key, NULL)
                      : grn_table_get_by_key(ctx, table, p_key);
            GRN_OBJ_FIN(ctx, &key);
          } else {
            grn_obj record_id;
            GRN_UINT32_INIT(&record_id, 0);
            grn_obj_cast(ctx, src, &record_id, 1);
            id = GRN_UINT32_VALUE(&record_id);
          }
          /* if not symbol */
          if (id) { GRN_RECORD_SET(ctx, dest, id); }
        } else {
          rc = GRN_FUNCTION_NOT_IMPLEMENTED;
        }
      }
      break;
    }
    break;
  default :
    rc = GRN_FUNCTION_NOT_IMPLEMENTED;
    break;
  }
  return rc;
}

#define GRN_OBJ_GET_VALUE_IMD (0xffffffffU)

const char *
grn_accessor_get_value_(grn_ctx *ctx, grn_accessor *a, grn_id id, uint32_t *size)
{
  const char *value = NULL;
  for (;;) {
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID :
      value = (const char *)(uintptr_t)id;
      *size = GRN_OBJ_GET_VALUE_IMD;
      break;
    case GRN_ACCESSOR_GET_KEY :
      value = _grn_table_key(ctx, a->obj, id, size);
      break;
    case GRN_ACCESSOR_GET_VALUE :
      value = grn_obj_get_value_(ctx, a->obj, id, size);
      break;
    case GRN_ACCESSOR_GET_SCORE :
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value = (const char *)&((grn_rset_recinfo *)value)->score;
        *size = sizeof(int);
      }
      break;
    case GRN_ACCESSOR_GET_NSUBRECS :
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value = (const char *)&((grn_rset_recinfo *)value)->n_subrecs;
        *size = sizeof(int);
      }
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      /* todo : support vector */
      value = grn_obj_get_value_(ctx, a->obj, id, size);
      break;
    case GRN_ACCESSOR_GET_DB_OBJ :
      value = _grn_pat_key(ctx, ((grn_db *)ctx->impl->db)->keys, id, size);
      break;
    case GRN_ACCESSOR_LOOKUP :
      /* todo */
      break;
    case GRN_ACCESSOR_FUNCALL :
      /* todo */
      break;
    }
    if (value && (a = a->next)) {
      id = *((grn_id *)value);
    } else {
      break;
    }
  }
  return value;
}

static grn_obj *
grn_accessor_get_value(grn_ctx *ctx, grn_accessor *a, grn_id id, grn_obj *value)
{
  uint32_t vs = 0;
  uint32_t size0;
  void *vp = NULL;
  if (!value) {
    if (!(value = grn_obj_open(ctx, GRN_BULK, 0, 0))) { return NULL; }
  } else {
    value->header.type = GRN_BULK;
  }
  size0 = GRN_BULK_VSIZE(value);
  for (;;) {
    grn_bulk_truncate(ctx, value, size0);
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID :
      GRN_UINT32_PUT(ctx, value, id);
      vp = GRN_BULK_HEAD(value) + size0;
      vs = GRN_BULK_VSIZE(value) - size0;
      break;
    case GRN_ACCESSOR_GET_KEY :
      grn_table_get_key2(ctx, a->obj, id, value);
      vp = GRN_BULK_HEAD(value) + size0;
      vs = GRN_BULK_VSIZE(value) - size0;
      break;
    case GRN_ACCESSOR_GET_VALUE :
      grn_obj_get_value(ctx, a->obj, id, value);
      vp = GRN_BULK_HEAD(value) + size0;
      vs = GRN_BULK_VSIZE(value) - size0;
      break;
    case GRN_ACCESSOR_GET_SCORE :
      grn_obj_get_value(ctx, a->obj, id, value);
      {
        grn_rset_recinfo *ri = (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
        GRN_INT32_PUT(ctx, value, ri->score);
      }
      break;
    case GRN_ACCESSOR_GET_NSUBRECS :
      {
        grn_rset_recinfo *ri = (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
        GRN_INT32_PUT(ctx, value, ri->n_subrecs);
      }
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      /* todo : support vector */
      grn_obj_get_value(ctx, a->obj, id, value);
      vp = GRN_BULK_HEAD(value) + size0;
      vs = GRN_BULK_VSIZE(value) - size0;
      break;
    case GRN_ACCESSOR_GET_DB_OBJ :
      value = grn_ctx_at(ctx, id);
      grn_obj_close(ctx, value);
      return value;
      break;
    case GRN_ACCESSOR_LOOKUP :
      /* todo */
      break;
    case GRN_ACCESSOR_FUNCALL :
      /* todo */
      break;
    }
    if ((a = a->next)) {
      id = *((grn_id *)vp);
    } else {
      break;
    }
  }
  return value;
}

static grn_rc
grn_accessor_set_value(grn_ctx *ctx, grn_accessor *a, grn_id id,
                       grn_obj *value, int flags)
{
  grn_rc rc = GRN_SUCCESS;
  if (!value) { value = grn_obj_open(ctx, GRN_BULK, 0, 0); }
  if (value) {
    grn_obj buf;
    void *vp = NULL;
    size_t vs;
    GRN_TEXT_INIT(&buf, 0);
    for (;;) {
      GRN_BULK_REWIND(&buf);
      switch (a->action) {
      case GRN_ACCESSOR_GET_KEY :
        grn_table_get_key2(ctx, a->obj, id, &buf);
        vp = GRN_BULK_HEAD(&buf);
        vs = GRN_BULK_VSIZE(&buf);
        break;
      case GRN_ACCESSOR_GET_VALUE :
        if (a->next) {
          grn_obj_get_value(ctx, a->obj, id, &buf);
          vp = GRN_BULK_HEAD(&buf);
          vs = GRN_BULK_VSIZE(&buf);
        } else {
          rc = grn_obj_set_value(ctx, a->obj, id, value, flags);
        }
        break;
      case GRN_ACCESSOR_GET_SCORE :
        {
          grn_rset_recinfo *ri;
          if (a->next) {
            grn_obj_get_value(ctx, a->obj, id, &buf);
            ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
            vp = &ri->score;
            vs = sizeof(int);
          } else {
            uint32_t size;
            if ((ri = (grn_rset_recinfo *) grn_obj_get_value_(ctx, a->obj, id, &size))) {
              vp = &ri->score;
              // todo : flags support
              if (value->header.domain == GRN_DB_INT32) {
                memcpy(vp, GRN_BULK_HEAD(value), sizeof(int));
              } else {
                grn_obj buf;
                GRN_INT32_INIT(&buf, 0);
                grn_obj_cast(ctx, value, &buf, 0);
                memcpy(vp, GRN_BULK_HEAD(&buf), sizeof(int));
                GRN_OBJ_FIN(ctx, &buf);
              }
            }
          }
        }
        break;
      case GRN_ACCESSOR_GET_NSUBRECS :
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          vp = &ri->n_subrecs;
          vs = sizeof(int);
        }
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE :
        /* todo : support vector */
        if (a->next) {
          grn_obj_get_value(ctx, a->obj, id, &buf);
          vp = GRN_BULK_HEAD(&buf);
          vs = GRN_BULK_VSIZE(&buf);
        } else {
          rc = grn_obj_set_value(ctx, a->obj, id, value, flags);
        }
        break;
      case GRN_ACCESSOR_LOOKUP :
        /* todo */
        break;
      case GRN_ACCESSOR_FUNCALL :
        /* todo */
        break;
      }
      if ((a = a->next)) {
        id = *((grn_id *)vp);
      } else {
        break;
      }
    }
    grn_obj_close(ctx, &buf);
  }
  return rc;
}

#define INCRDECR(op) \
  switch (DB_OBJ(obj)->range) {\
  case GRN_DB_INT32 :\
    if (s == sizeof(int32_t)) {\
      int32_t *vp = (int32_t *)p;\
      *vp op *(int32_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_UINT32 :\
    if (s == sizeof(uint32_t)) {\
      uint32_t *vp = (uint32_t *)p;\
      *vp op *(int32_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_INT64 :\
  case GRN_DB_TIME :\
    if (s == sizeof(int64_t)) {\
      int64_t *vp = (int64_t *)p;\
      *vp op *(int64_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_FLOAT :\
    if (s == sizeof(double)) {\
      double *vp = (double *)p;\
      *vp op *(double *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  default :\
    rc = GRN_OPERATION_NOT_SUPPORTED;\
    break;\
  }

uint32_t
grn_obj_size(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) { return 0; }
  switch (obj->header.type) {
  case GRN_VOID :
  case GRN_BULK :
  case GRN_PTR :
  case GRN_UVECTOR :
  case GRN_PVECTOR :
  case GRN_MSG :
    return GRN_BULK_VSIZE(obj);
  case GRN_VECTOR :
    return obj->u.v.body ? GRN_BULK_VSIZE(obj->u.v.body) : 0;
  default :
    return 0;
  }
}

inline static int
call_hook(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  grn_hook *hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET];
  void *v = GRN_BULK_HEAD(value);
  unsigned int s = grn_obj_size(ctx, value);
  if (hooks || obj->header.type == GRN_COLUMN_VAR_SIZE) {
    grn_obj oldbuf, *oldvalue;
    GRN_TEXT_INIT(&oldbuf, 0);
    oldvalue = grn_obj_get_value(ctx, obj, id, &oldbuf);
    if (flags & GRN_OBJ_SET) {
      void *ov;
      unsigned int os;
      ov = GRN_BULK_HEAD(oldvalue);
      os = grn_obj_size(ctx, oldvalue);
      if (ov && v && os == s && !memcmp(ov, v, s)) {
        grn_bulk_fin(ctx, oldvalue);
        return 0;
      }
    }
    if (hooks) {
      // todo : grn_proc_ctx_open()
      grn_obj id_, flags_;
      grn_proc_ctx pctx = {{0}, hooks->proc, NULL, hooks, hooks, PROC_INIT, 4, 4};
      GRN_UINT32_INIT(&id_, 0);
      GRN_UINT32_INIT(&flags_, 0);
      GRN_UINT32_SET(ctx, &id_, id);
      GRN_UINT32_SET(ctx, &flags_, flags);
      while (hooks) {
        grn_ctx_push(ctx, &id_);
        grn_ctx_push(ctx, oldvalue);
        grn_ctx_push(ctx, value);
        grn_ctx_push(ctx, &flags_);
        pctx.caller = NULL;
        pctx.currh = hooks;
        if (hooks->proc) {
          hooks->proc->funcs[PROC_INIT](ctx, 1, &obj, &pctx.user_data);
        } else {
          default_set_value_hook(ctx, 1, &obj, &pctx.user_data);
        }
        if (ctx->rc) {
          grn_bulk_fin(ctx, oldvalue);
          return 1;
        }
        hooks = hooks->next;
        pctx.offset++;
      }
    }
    grn_obj_close(ctx, oldvalue);
  }
  return 0;
}

grn_rc
grn_obj_set_value(grn_ctx *ctx, grn_obj *obj, grn_id id,
                  grn_obj *value, int flags)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!GRN_DB_OBJP(obj)) {
    if (obj->header.type == GRN_ACCESSOR) {
      rc = grn_accessor_set_value(ctx, (grn_accessor *)obj, id, value, flags);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "not db_obj");
    }
  } else {
    grn_id range = DB_OBJ(obj)->range;
    void *v = GRN_BULK_HEAD(value);
    unsigned int s = grn_obj_size(ctx, value);
    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY :
      {
        grn_obj buf;
        if (call_hook(ctx, obj, id, value, flags)) { goto exit; }
        if (range != value->header.domain) {
          GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
          if (grn_obj_cast(ctx, value, &buf, 1) == GRN_SUCCESS) {
            v = GRN_BULK_HEAD(&buf);
          }
        }
        rc = grn_pat_set_value(ctx, (grn_pat *)obj, id, v, flags);
      }
      break;
    case GRN_TABLE_HASH_KEY :
      {
        grn_obj buf;
        if (call_hook(ctx, obj, id, value, flags)) { goto exit; }
        if (range != value->header.domain) {
          GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
          if (grn_obj_cast(ctx, value, &buf, 1) == GRN_SUCCESS) {
            v = GRN_BULK_HEAD(&buf);
          }
        }
        rc = grn_hash_set_value(ctx, (grn_hash *)obj, id, v, flags);
      }
      break;
    case GRN_TABLE_NO_KEY :
      {
        grn_obj buf;
        if (call_hook(ctx, obj, id, value, flags)) { goto exit; }
        if (range != value->header.domain) {
          GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
          if (grn_obj_cast(ctx, value, &buf, 1) == GRN_SUCCESS) {
            v = GRN_BULK_HEAD(&buf);
          }
        }
        rc = grn_array_set_value(ctx, (grn_array *)obj, id, v, flags);
      }
      break;
    case GRN_COLUMN_VAR_SIZE :
      if (call_hook(ctx, obj, id, value, flags)) { goto exit; }
      switch (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
      case GRN_OBJ_COLUMN_SCALAR :
        rc = grn_ja_put(ctx, (grn_ja *)obj, id, v, s, flags);
        break;
      case GRN_OBJ_COLUMN_VECTOR :
        {
          grn_obj *lexicon = grn_ctx_at(ctx, DB_OBJ(obj)->range);
          if (GRN_OBJ_TABLEP(lexicon)) {
            grn_obj buf;
            GRN_TEXT_INIT(&buf, 0);
            switch (value->header.type) {
            case GRN_BULK :
              {
                grn_token *token;
                if (v && s &&
                    (token = grn_token_open(ctx, lexicon, v, s, GRN_TABLE_ADD))) {
                  while (!token->status) {
                    grn_id tid = grn_token_next(ctx, token);
                    grn_bulk_write(ctx, &buf, (char *)&tid, sizeof(grn_id));
                  }
                  grn_token_close(ctx, token);
                }
                rc = grn_ja_put(ctx, (grn_ja *)obj, id,
                                GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf), flags);
              }
              break;
            case GRN_VECTOR :
              if (value->u.v.body) {
                int j;
                grn_section *v;
                const char *head = GRN_BULK_HEAD(value->u.v.body);
                for (j = value->u.v.n_sections, v = value->u.v.sections; j; j--, v++) {
                  grn_id tid = grn_table_add(ctx, lexicon,
                                             head + v->offset, v->length, NULL);
                  grn_bulk_write(ctx, &buf, (char *)&tid, sizeof(grn_id));
                }
              }
              rc = grn_ja_put(ctx, (grn_ja *)obj, id,
                              GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf), flags);
              break;
            case GRN_UVECTOR :
              rc = grn_ja_put(ctx, (grn_ja *)obj, id, v, s, flags);
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "vector, uvector or bulk required");
              break;
            }
            grn_obj_close(ctx, &buf);
          } else {
            switch (value->header.type) {
            case GRN_BULK :
              {
                grn_obj v;
                GRN_OBJ_INIT(&v, GRN_VECTOR, GRN_OBJ_DO_SHALLOW_COPY, GRN_DB_TEXT);
                v.u.v.body = value;
                grn_vector_delimit(ctx, &v, 0, GRN_ID_NIL);
                rc = grn_ja_putv(ctx, (grn_ja *)obj, id, &v, 0);
                grn_obj_close(ctx, &v);
              }
              break;
            case GRN_UVECTOR :
              rc = grn_ja_put(ctx, (grn_ja *)obj, id, v, s, flags);
              break;
            case GRN_VECTOR :
              rc = grn_ja_putv(ctx, (grn_ja *)obj, id, value, 0);
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "vector or bulk required");
              break;
            }
          }
        }
        break;
      default :
        ERR(GRN_FILE_CORRUPT, "invalid GRN_OBJ_COLUMN_TYPE");
        break;
      }
      break;
    case GRN_COLUMN_FIX_SIZE :
      {
        grn_obj buf, *value_ = value;
        GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
        if (range != value->header.domain) {
          grn_obj_cast(ctx, value, &buf, 1);
          value_ = &buf;
          v = GRN_BULK_HEAD(&buf);
          s = GRN_BULK_VSIZE(&buf);
        }
        if (((grn_ra *)obj)->header->element_size < s) {
          ERR(GRN_INVALID_ARGUMENT, "too long value (%d)", s);
        } else {
          void *p = grn_ra_ref(ctx, (grn_ra *)obj, id);
          if (!p) {
            ERR(GRN_NO_MEMORY_AVAILABLE, "ra get failed");
            rc = GRN_NO_MEMORY_AVAILABLE;
            goto exit;
          }
          switch (flags & GRN_OBJ_SET_MASK) {
          case GRN_OBJ_SET :
            if (call_hook(ctx, obj, id, value_, flags)) {
              GRN_OBJ_FIN(ctx, &buf);
              grn_ra_unref(ctx, (grn_ra *)obj, id);
              goto exit;
            }
            memcpy(p, v, s);
            rc = GRN_SUCCESS;
            break;
          case GRN_OBJ_INCR :
            /* todo : support hook */
            INCRDECR(+=);
            break;
          case GRN_OBJ_DECR :
            /* todo : support hook */
            INCRDECR(-=);
            break;
          default :
            rc = GRN_OPERATION_NOT_SUPPORTED;
            break;
          }
          grn_ra_unref(ctx, (grn_ra *)obj, id);
        }
        GRN_OBJ_FIN(ctx, &buf);
      }
      break;
    case GRN_COLUMN_INDEX :
      // todo : how??
      break;
    }
  }
exit :
  GRN_API_RETURN(rc);
}

const char *
grn_obj_get_value_(grn_ctx *ctx, grn_obj *obj, grn_id id, uint32_t *size)
{
  const char *value = NULL;
  *size = 0;
  switch (obj->header.type) {
  case GRN_ACCESSOR_VIEW :
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "GRN_ACCESSOR_VIEW not supported");
    break;
  case GRN_ACCESSOR :
    value = grn_accessor_get_value_(ctx, (grn_accessor *)obj, id, size);
    break;
  case GRN_TABLE_PAT_KEY :
    value = grn_pat_get_value_(ctx, (grn_pat *)obj, id, size);
    break;
  case GRN_TABLE_HASH_KEY :
    value = grn_hash_get_value_(ctx, (grn_hash *)obj, id, size);
    break;
  case GRN_TABLE_NO_KEY :
    if ((value = _grn_array_get_value(ctx, (grn_array *)obj, id))) {
      *size = ((grn_array *)obj)->value_size;
    }
    break;
  case GRN_COLUMN_VAR_SIZE :
    {
      grn_io_win jw;
      if ((value = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, size))) {
        grn_ja_unref(ctx, &jw);
      }
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
    if ((value = grn_ra_ref(ctx, (grn_ra *)obj, id))) {
      grn_ra_unref(ctx, (grn_ra *)obj, id);
      *size = ((grn_ra *)obj)->header->element_size;
    }
    break;
  case GRN_COLUMN_INDEX :
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "todo: GRN_COLUMN_INDEX");
    break;
  }
  return value;
}

grn_obj *
grn_obj_get_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value)
{
  unsigned int len = 0;
  GRN_API_ENTER;
  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_value failed");
    goto exit;
  }
  if (!value) {
    if (!(value = grn_obj_open(ctx, GRN_BULK, 0, 0))) {
      ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_value failed");
      goto exit;
    }
  }
  switch (value->header.type) {
  case GRN_VOID :
    GRN_TEXT_INIT(value, 0);
    break;
  case GRN_BULK :
  case GRN_VECTOR :
  case GRN_UVECTOR :
  case GRN_MSG :
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_value failed");
    goto exit;
  }
  switch (obj->header.type) {
  case GRN_ACCESSOR_VIEW :
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "GRN_ACCESSOR_VIEW not supported");
    break;
  case GRN_ACCESSOR :
    value = grn_accessor_get_value(ctx, (grn_accessor *)obj, id, value);
    value->header.domain = grn_obj_get_range(ctx, obj);
    break;
  case GRN_TABLE_PAT_KEY :
    {
      grn_pat *pat = (grn_pat *)obj;
      uint32_t size = pat->value_size;
      if (grn_bulk_space(ctx, value, size)) {
        MERR("grn_bulk_space failed");
        goto exit;
      }
      {
        char *curr = GRN_BULK_CURR(value);
        len = grn_pat_get_value(ctx, pat, id, curr - size);
      }
      value->header.type = GRN_BULK;
      value->header.domain = grn_obj_get_range(ctx, obj);
    }
    break;
  case GRN_TABLE_HASH_KEY :
    {
      grn_hash *hash = (grn_hash *)obj;
      uint32_t size = hash->value_size;
      if (grn_bulk_space(ctx, value, size)) {
        MERR("grn_bulk_space failed");
        goto exit;
      }
      {
        char *curr = GRN_BULK_CURR(value);
        len = grn_hash_get_value(ctx, hash, id, curr - size);
      }
      value->header.type = GRN_BULK;
      value->header.domain = grn_obj_get_range(ctx, obj);
    }
    break;
  case GRN_TABLE_NO_KEY :
    {
      grn_array *array = (grn_array *)obj;
      uint32_t size = array->value_size;
      if (grn_bulk_space(ctx, value, size)) {
        MERR("grn_bulk_space failed");
        goto exit;
      }
      {
        char *curr = GRN_BULK_CURR(value);
        len = grn_array_get_value(ctx, array, id, curr - size);
      }
      value->header.type = GRN_BULK;
      value->header.domain = grn_obj_get_range(ctx, obj);
    }
    break;
  case GRN_COLUMN_VAR_SIZE :
    switch (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
    case GRN_OBJ_COLUMN_VECTOR :
      {
        grn_obj *lexicon = grn_ctx_at(ctx, DB_OBJ(obj)->range);
        if (GRN_OBJ_TABLEP(lexicon)) {
          grn_io_win jw;
          void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, &len);
          if (v) {
            // todo : reduce copy
            // todo : grn_vector_add_element when vector assigned
            grn_bulk_write(ctx, value, v, len);
            grn_ja_unref(ctx, &jw);
          }
          value->header.type = GRN_UVECTOR;
        } else {
          switch (value->header.type) {
          case GRN_VECTOR :
            {
              grn_io_win jw;
              void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, &len);
              if (v) {
                grn_vector_decode(ctx, value, v, len);
                grn_ja_unref(ctx, &jw);
              }
            }
            break;
          case GRN_UVECTOR :
          case GRN_BULK :
            {
              grn_io_win jw;
              void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, &len);
              if (v) {
                grn_bulk_write(ctx, value, v, len);
                grn_ja_unref(ctx, &jw);
              }
            }
            break;
          default :
            ERR(GRN_INVALID_ARGUMENT, "vector or bulk required");
            break;
          }
        }
      }
      break;
    case GRN_OBJ_COLUMN_SCALAR :
      {
        grn_io_win jw;
        void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, &len);
        value->header.type = GRN_BULK;
        if (v) {
          // todo : reduce copy
          // todo : grn_vector_add_element when vector assigned
          grn_bulk_write(ctx, value, v, len);
          grn_ja_unref(ctx, &jw);
        }
      }
      break;
    default :
      ERR(GRN_FILE_CORRUPT, "invalid GRN_OBJ_COLUMN_TYPE");
      break;
    }
    value->header.domain = grn_obj_get_range(ctx, obj);
    break;
  case GRN_COLUMN_FIX_SIZE :
    {
      unsigned element_size;
      void *v = grn_ra_ref(ctx, (grn_ra *)obj, id);
      value->header.type = GRN_BULK;
      value->header.domain = grn_obj_get_range(ctx, obj);
      if (v) {
        element_size = ((grn_ra *)obj)->header->element_size;
        grn_bulk_write(ctx, value, v, element_size);
        grn_ra_unref(ctx, (grn_ra *)obj, id);
      }
    }
    break;
  case GRN_COLUMN_INDEX :
    GRN_UINT32_SET(ctx, value, grn_ii_estimate_size(ctx, (grn_ii *)obj, id));
    value->header.domain = GRN_DB_UINT32;
    break;
  }
exit :
  GRN_API_RETURN(value);
}

grn_obj *
grn_obj_get_value_o(grn_ctx *ctx, grn_obj *obj, grn_obj *id, grn_obj *value)
{
  grn_id *idp = (grn_id *)GRN_BULK_HEAD(id);
  uint32_t ids = GRN_BULK_VSIZE(id);
  while (obj->header.type == GRN_ACCESSOR_VIEW) {
    uint32_t n;
    grn_accessor_view *v = (grn_accessor_view *)obj;
    if (ids < sizeof(grn_id)) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "invalid id.");
      return NULL;
    }
    n = *idp;
    if (n >= v->naccessors) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "invalid id");
      return NULL;
    }
    if (!(obj = v->accessors[n])) { return value; }
    idp++;
    ids -= sizeof(grn_id);
  }
  return grn_obj_get_value(ctx, obj, *idp, value);
}

grn_rc
grn_obj_set_value_o(grn_ctx *ctx, grn_obj *obj, grn_obj *id,
                    grn_obj *value, int flags)
{
  grn_id *idp = (grn_id *)GRN_BULK_HEAD(id);
  uint32_t ids = GRN_BULK_VSIZE(id);
  while (obj->header.type == GRN_ACCESSOR_VIEW) {
    uint32_t n;
    grn_accessor_view *v = (grn_accessor_view *)obj;
    if (ids < sizeof(grn_id)) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "invalid id.");
      return ctx->rc;
    }
    n = *idp;
    if (n >= v->naccessors) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "invalid id");
      return ctx->rc;
    }
    obj = v->accessors[n];
    idp++;
    ids -= sizeof(grn_id);
  }
  return grn_obj_set_value(ctx, obj, *idp, value, flags);
}

grn_rc
grn_column_index_update(grn_ctx *ctx, grn_obj *column,
                        grn_id id, unsigned int section,
                        grn_obj *oldvalue, grn_obj *newvalue)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (column->header.type != GRN_COLUMN_INDEX) {
    ERR(GRN_INVALID_ARGUMENT, "invalid column assigned");
  } else {
    rc = grn_ii_column_update(ctx, (grn_ii *)column, id, section, oldvalue, newvalue, NULL);
  }
  GRN_API_RETURN(rc);
}

grn_obj *
grn_column_table(grn_ctx *ctx, grn_obj *column)
{
  grn_obj *obj = NULL;
  grn_db_obj *col = DB_OBJ(column);
  GRN_API_ENTER;
  if (col) {
    obj = grn_ctx_at(ctx, col->header.domain);
  }
  GRN_API_RETURN(obj);
}

grn_obj *
grn_obj_get_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *valuebuf)
{
  GRN_API_ENTER;
  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_info failed");
    goto exit;
  }
  switch (type) {
  case GRN_INFO_ENCODING :
    if (!valuebuf) {
      if (!(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, 0))) {
        ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_info failed");
        goto exit;
      }
    }
    {
      grn_encoding enc;
      switch (obj->header.type) {
      case GRN_DB :
        enc = ((grn_db *)obj)->keys->encoding;
        grn_bulk_write(ctx, valuebuf, (const char *)&enc, sizeof(grn_encoding));
        break;
      case GRN_TABLE_PAT_KEY :
        enc = ((grn_pat *)obj)->encoding;
        grn_bulk_write(ctx, valuebuf, (const char *)&enc, sizeof(grn_encoding));
        break;
      case GRN_TABLE_HASH_KEY :
        enc = ((grn_hash *)obj)->encoding;
        grn_bulk_write(ctx, valuebuf, (const char *)&enc, sizeof(grn_encoding));
        break;
      default :
        ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_info failed");
      }
    }
    break;
  case GRN_INFO_SOURCE :
    if (!valuebuf) {
      if (!(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, 0))) {
        ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_info failed");
        goto exit;
      }
    }
    if (!GRN_DB_OBJP(obj)) {
      ERR(GRN_INVALID_ARGUMENT, "only db_obj can accept GRN_INFO_SOURCE");
      goto exit;
    }
    grn_bulk_write(ctx, valuebuf, DB_OBJ(obj)->source, DB_OBJ(obj)->source_size);
    break;
  case GRN_INFO_DEFAULT_TOKENIZER :
    switch (DB_OBJ(obj)->header.type) {
    case GRN_TABLE_HASH_KEY :
      valuebuf = ((grn_hash *)obj)->tokenizer;
      break;
    case GRN_TABLE_PAT_KEY :
      valuebuf = ((grn_pat *)obj)->tokenizer;
      break;
    }
    break;
  default :
    /* todo */
    break;
  }
exit :
  GRN_API_RETURN(valuebuf);
}

static void
update_source_hook(grn_ctx *ctx, grn_obj *obj)
{
  grn_id *s = DB_OBJ(obj)->source;
  int i, n = DB_OBJ(obj)->source_size / sizeof(grn_id);
  default_set_value_hook_data hook_data = { DB_OBJ(obj)->id, 0 };
  grn_obj *source, data;
  GRN_TEXT_INIT(&data, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET_REF(&data, &hook_data, sizeof hook_data);
  for (i = 1; i <= n; i++, s++) {
    hook_data.section = i;
    if ((source = grn_ctx_at(ctx, *s))) {
      switch (source->header.type) {
      case GRN_TABLE_HASH_KEY :
      case GRN_TABLE_PAT_KEY :
        grn_obj_add_hook(ctx, source, GRN_HOOK_INSERT, 0, NULL, &data);
        grn_obj_add_hook(ctx, source, GRN_HOOK_DELETE, 0, NULL, &data);
        break;
      case GRN_COLUMN_FIX_SIZE :
      case GRN_COLUMN_VAR_SIZE :
        grn_obj_add_hook(ctx, source, GRN_HOOK_SET, 0, NULL, &data);
        break;
      default :
        /* invalid target */
        break;
      }
    }
  }
  grn_obj_close(ctx, &data);
}

#define N_HOOK_ENTRIES 5

grn_rc
grn_hook_pack(grn_ctx *ctx, grn_db_obj *obj, grn_obj *buf)
{
  grn_rc rc;
  grn_hook_entry e;
  for (e = 0; e < N_HOOK_ENTRIES; e++) {
    grn_hook *hooks;
    for (hooks = obj->hooks[e]; hooks; hooks = hooks->next) {
      grn_id id = hooks->proc ? hooks->proc->obj.id : 0;
      if ((rc = grn_text_benc(ctx, buf, id + 1))) { goto exit; }
      if ((rc = grn_text_benc(ctx, buf, hooks->hld_size))) { goto exit; }
      if ((rc = grn_bulk_write(ctx, buf, (char *)NEXT_ADDR(hooks), hooks->hld_size))) { goto exit; }
    }
    if ((rc = grn_text_benc(ctx, buf, 0))) { goto exit; }
  }
exit :
  return rc;
}

static grn_rc
grn_hook_unpack(grn_ctx *ctx, grn_db_obj *obj, const char *buf, uint32_t buf_size)
{
  grn_hook_entry e;
  const uint8_t *p = (uint8_t *)buf, *pe = p + buf_size;
  for (e = 0; e < N_HOOK_ENTRIES; e++) {
    grn_hook *new, **last = &obj->hooks[e];
    for (;;) {
      grn_id id;
      uint32_t hld_size;
      GRN_B_DEC(id, p);
      if (!id--) { break; }
      if (p >= pe) { return GRN_FILE_CORRUPT; }
      GRN_B_DEC(hld_size, p);
      if (p >= pe) { return GRN_FILE_CORRUPT; }
      if (!(new = GRN_MALLOC(sizeof(grn_hook) + hld_size))) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      if (id) {
        new->proc = (grn_proc *)grn_ctx_at(ctx, id);
        if (!new->proc) {
          GRN_FREE(new);
          return ctx->rc;
        }
      } else {
        new->proc = NULL;
      }
      if ((new->hld_size = hld_size)) {
        memcpy(NEXT_ADDR(new), p, hld_size);
        p += hld_size;
      }
      *last = new;
      last = &new->next;
      if (p >= pe) { return GRN_FILE_CORRUPT; }
    }
    *last = NULL;
  }
  return GRN_SUCCESS;
}

static void grn_expr_pack(grn_ctx *ctx, grn_obj *buf, grn_obj *expr);

static void
grn_obj_spec_save(grn_ctx *ctx, grn_db_obj *obj)
{
  grn_db *s;
  grn_obj v, *b;
  grn_obj_spec spec;
  if (obj->id & GRN_OBJ_TMP_OBJECT) { return; }
  if (!ctx->impl || !GRN_DB_OBJP(obj)) { return; }
  if (!(s = (grn_db *)ctx->impl->db) || !s->specs) { return; }
  GRN_OBJ_INIT(&v, GRN_VECTOR, 0, GRN_DB_TEXT);
  if (!(b = grn_vector_body(ctx, &v))) { return; }
  spec.header = obj->header;
  spec.range = obj->range;
  grn_bulk_write(ctx, b, (void *)&spec, sizeof(grn_obj_spec));
  grn_vector_delimit(ctx, &v, 0, 0);
  if (obj->header.flags & GRN_OBJ_CUSTOM_NAME) {
    GRN_TEXT_PUTS(ctx, b, grn_obj_path(ctx, (grn_obj *)obj));
  }
  grn_vector_delimit(ctx, &v, 0, 0);
  grn_bulk_write(ctx, b, obj->source, obj->source_size);
  grn_vector_delimit(ctx, &v, 0, 0);
  grn_hook_pack(ctx, obj, b);
  grn_vector_delimit(ctx, &v, 0, 0);
  switch (obj->header.type) {
  case GRN_EXPR :
    grn_expr_pack(ctx, b, (grn_obj *)obj);
    grn_vector_delimit(ctx, &v, 0, 0);
    break;
  }
  grn_ja_putv(ctx, s->specs, obj->id, &v, 0);
  grn_obj_close(ctx, &v);
}

grn_rc
grn_obj_set_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *value)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "grn_obj_set_info failed");
    goto exit;
  }
  switch (type) {
  case GRN_INFO_SOURCE :
    if (!GRN_DB_OBJP(obj)) {
      ERR(GRN_INVALID_ARGUMENT, "only db_obj can accept GRN_INFO_SOURCE");
      goto exit;
    }
    {
      void *v = GRN_BULK_HEAD(value);
      uint32_t s = GRN_BULK_VSIZE(value);
      if (s) {
        void *v2 = GRN_MALLOC(s);
        if (!v2) {
          rc = ctx->rc;
          goto exit;
        }
        memcpy(v2, v, s);
        if (DB_OBJ(obj)->source) { GRN_FREE(DB_OBJ(obj)->source); }
        DB_OBJ(obj)->source = v2;
        DB_OBJ(obj)->source_size = s;

        if (obj->header.type == GRN_COLUMN_INDEX) {
          update_source_hook(ctx, obj);
        }

      } else {
        DB_OBJ(obj)->source = NULL;
        DB_OBJ(obj)->source_size = 0;
      }
    }
    grn_obj_spec_save(ctx, DB_OBJ(obj));
    rc = GRN_SUCCESS;
    break;
  case GRN_INFO_DEFAULT_TOKENIZER :
    if (!value || DB_OBJ(value)->header.type == GRN_PROC) {
      switch (DB_OBJ(obj)->header.type) {
      case GRN_TABLE_HASH_KEY :
        ((grn_hash *)obj)->tokenizer = value;
        ((grn_hash *)obj)->header->tokenizer = grn_obj_id(ctx, value);
        rc = GRN_SUCCESS;
        break;
      case GRN_TABLE_PAT_KEY :
        ((grn_pat *)obj)->tokenizer = value;
        ((grn_pat *)obj)->header->tokenizer = grn_obj_id(ctx, value);
        rc = GRN_SUCCESS;
        break;
      }
    }
  default :
    /* todo */
    break;
  }
exit :
  GRN_API_RETURN(rc);
}

grn_obj *
grn_obj_get_element_info(grn_ctx *ctx, grn_obj *obj, grn_id id,
                         grn_info_type type, grn_obj *valuebuf)
{
  GRN_API_ENTER;
  GRN_API_RETURN(valuebuf);
}

grn_rc
grn_obj_set_element_info(grn_ctx *ctx, grn_obj *obj, grn_id id,
                         grn_info_type type, grn_obj *value)
{
  GRN_API_ENTER;
  GRN_API_RETURN(GRN_SUCCESS);
}

static void
grn_hook_free(grn_ctx *ctx, grn_hook *h)
{
  grn_hook *curr, *next;
  for (curr = h; curr; curr = next) {
    next = curr->next;
    GRN_FREE(curr);
  }
}

grn_rc
grn_obj_add_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry,
                 int offset, grn_obj *proc, grn_obj *hld)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  if (!GRN_DB_OBJP(obj)) {
    rc = GRN_INVALID_ARGUMENT;
  } else {
    int i;
    void *hld_value = NULL;
    uint32_t hld_size = 0;
    grn_hook *new, **last = &DB_OBJ(obj)->hooks[entry];
    if (hld) {
      hld_value = GRN_BULK_HEAD(hld);
      hld_size = GRN_BULK_VSIZE(hld);
    }
    if (!(new = GRN_MALLOC(sizeof(grn_hook) + hld_size))) {
      rc = GRN_NO_MEMORY_AVAILABLE;
      goto exit;
    }
    new->proc = (grn_proc *)proc;
    new->hld_size = hld_size;
    if (hld_size) {
      memcpy(NEXT_ADDR(new), hld_value, hld_size);
    }
    for (i = 0; i != offset && *last; i++) { last = &(*last)->next; }
    new->next = *last;
    *last = new;
    grn_obj_spec_save(ctx, DB_OBJ(obj));
  }
exit :
  GRN_API_RETURN(rc);
}

int
grn_obj_get_nhooks(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry)
{
  int res = 0;
  GRN_API_ENTER;
  {
    grn_hook *hook = DB_OBJ(obj)->hooks[entry];
    while (hook) {
      res++;
      hook = hook->next;
    }
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_obj_get_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry,
                      int offset, grn_obj *hldbuf)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  {
    int i;
    grn_hook *hook = DB_OBJ(obj)->hooks[entry];
    for (i = 0; i < offset; i++) {
      hook = hook->next;
      if (!hook) { return NULL; }
    }
    res = (grn_obj *)hook->proc;
    grn_bulk_write(ctx, hldbuf, (char *)NEXT_ADDR(hook), hook->hld_size);
  }
  GRN_API_RETURN(res);
}

grn_rc
grn_obj_delete_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset)
{
  GRN_API_ENTER;
  {
    int i;
    grn_hook *h = NULL, **last = &DB_OBJ(obj)->hooks[entry];
    for (i = 0; i < offset; i++) {
      if (!(h = *last)) { return GRN_INVALID_ARGUMENT; }
      last = &(*last)->next;
    }
    *last = h->next;
    GRN_FREE(h);
  }
  grn_obj_spec_save(ctx, DB_OBJ(obj));
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_obj_remove(grn_ctx *ctx, grn_obj *obj)
{
  char *path;
  GRN_API_ENTER;
  path = (char *)grn_obj_path(ctx, obj);
  if (path) { path = GRN_STRDUP(path); }
  switch (obj->header.type) {
  case GRN_DB :
    {
      grn_table_cursor *cur;
      if ((cur = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0, 0, -1, 0))) {
        grn_id id;
        while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
          grn_obj *tbl = grn_ctx_at(ctx, id);
          if (tbl) {
            switch (tbl->header.type) {
            case GRN_TABLE_HASH_KEY :
            case GRN_TABLE_NO_KEY:
            case GRN_TABLE_PAT_KEY:
              grn_obj_remove(ctx, tbl);
            }
          }
        }
        grn_table_cursor_close(ctx, cur);
      }
    }
    break;
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_NO_KEY :
    {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, obj, "", 0, (grn_obj *)cols)) {
          grn_id *key;
          GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col = grn_ctx_at(ctx, *key);
            if (col) { grn_obj_remove(ctx, col); }
          });
        }
        grn_hash_close(ctx, cols);
      }
      grn_obj_delete_by_id(ctx, DB_OBJ(obj)->db, DB_OBJ(obj)->id, 1);
    }
    break;
  case GRN_COLUMN_VAR_SIZE :
  case GRN_COLUMN_FIX_SIZE :
    {
      grn_hook *hooks;
      for (hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET]; hooks; hooks = hooks->next) {
        default_set_value_hook_data *data = (void *)NEXT_ADDR(hooks);
        grn_obj *target = grn_ctx_at(ctx, data->target);
        if (target->header.type == GRN_COLUMN_INDEX) {
          //TODO: multicolumn  MULTI_COLUMN_INDEXP
          grn_obj_remove(ctx, target);
        } else {
          //TODO: err
          char fn[GRN_TABLE_MAX_KEY_SIZE];
          int flen;
          flen = grn_obj_name(ctx, target, fn, GRN_TABLE_MAX_KEY_SIZE);
          fn[flen] = '\0';
          ERR(GRN_UNKNOWN_ERROR, "column has unsupported hooks, col=%s",fn);
        }
      }
      grn_obj_delete_by_id(ctx, DB_OBJ(obj)->db, DB_OBJ(obj)->id, 1);
    }
    break;
  case GRN_COLUMN_INDEX :
    grn_obj_delete_by_id(ctx, DB_OBJ(obj)->db, DB_OBJ(obj)->id, 1);
    break;
  }
  grn_obj_close(ctx, obj);
  if (path) {
    grn_ja_put(ctx, ((grn_db *)ctx->impl->db)->specs, DB_OBJ(obj)->id, "", 0, GRN_OBJ_SET);
    grn_io_remove(ctx, path);
    GRN_FREE(path);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_rename(grn_ctx *ctx, const char *old_path, const char *new_path)
{
  GRN_API_ENTER;
  GRN_API_RETURN(GRN_SUCCESS);
}

/* db must be validate by caller */
static grn_id
grn_obj_register(grn_ctx *ctx, grn_obj *db, const char *name, unsigned name_size)
{
  grn_id id = GRN_ID_NIL;
  if (name && name_size) {
    grn_db *s = (grn_db *)db;
    int added;
    if (!(id = grn_pat_add(ctx, s->keys, name, name_size, NULL, &added))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_pat_add failed");
    } else if (!added) {
      ERR(GRN_INVALID_ARGUMENT, "already used name was assigned");
      id = GRN_ID_NIL;
    }
  } else if (ctx->impl && ctx->impl->values) {
    id = grn_array_add(ctx, ctx->impl->values, NULL) | GRN_OBJ_TMP_OBJECT;
  }
  return id;
}

static grn_rc
grn_obj_delete_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, int removep)
{
  if (id) {
    if (id & GRN_OBJ_TMP_OBJECT) {
      if (ctx->impl && ctx->impl->values) {
        return grn_array_delete_by_id(ctx, ctx->impl->values,
                                      id & ~GRN_OBJ_TMP_OBJECT, NULL);
      }
    } else {
      grn_obj **vp;
      grn_db *s = (grn_db *)db;
      if ((vp = grn_tiny_array_at(&s->values, id))) {
        *vp = NULL;
      }
      return removep ? grn_pat_delete_by_id(ctx, s->keys, id, NULL) : GRN_SUCCESS;
    }
  }
  return GRN_INVALID_ARGUMENT;
}

/* db must be validate by caller */
static grn_rc
grn_db_obj_init(grn_ctx *ctx, grn_obj *db, grn_id id, grn_db_obj *obj)
{
  grn_rc rc = GRN_SUCCESS;
  if (id) {
    if (id & GRN_OBJ_TMP_OBJECT) {
      if (ctx->impl && ctx->impl->values) {
        grn_tmp_db_obj tmp_obj;
        tmp_obj.obj = obj;
        memset(&tmp_obj.cell, 0, sizeof(grn_cell));
        rc = grn_array_set_value(ctx, ctx->impl->values,
                                 id & ~GRN_OBJ_TMP_OBJECT, &tmp_obj, GRN_OBJ_SET);
      }
    } else {
      void **vp;
      vp = grn_tiny_array_at(&((grn_db *)db)->values, id);
      if (!vp) {
        rc = GRN_NO_MEMORY_AVAILABLE;
        ERR(rc, "grn_tiny_array_at failed (%d)", id);
        return rc;
      }
      *vp = (grn_obj *)obj;
    }
  }
  obj->id = id;
  obj->db = db;
  obj->source = NULL;
  obj->source_size = 0;
  {
    grn_hook_entry entry;
    for (entry = 0; entry < N_HOOK_ENTRIES; entry++) {
      obj->hooks[entry] = NULL;
    }
  }
  grn_obj_spec_save(ctx, obj);
  return rc;
}

#define GET_PATH(spec,buffer,s,id) {\
  if (spec->header.flags & GRN_OBJ_CUSTOM_NAME) {\
    const char *path;\
    unsigned int size = grn_vector_get_element(ctx, &v, 1, &path, NULL, NULL); \
    if (size > PATH_MAX) { ERR(GRN_FILENAME_TOO_LONG, "too long path"); }\
    memcpy(buffer, path, size);\
    buffer[size] = '\0';\
  } else {\
    gen_pathname(s->keys->io->path, buffer, id);\
  }\
}

static grn_obj *grn_expr_open(grn_ctx *ctx, grn_obj_spec *spec,
                              const uint8_t *p, const uint8_t *pe);

grn_obj *
grn_ctx_at(grn_ctx *ctx, grn_id id)
{
  grn_obj *res = NULL;
  if (!ctx || !ctx->impl || !id) { return res; }
  GRN_API_ENTER;
  if (id & GRN_OBJ_TMP_OBJECT) {
    if (ctx->impl->values) {
      grn_tmp_db_obj *tmp_obj;
      if ((tmp_obj = _grn_array_get_value(ctx, ctx->impl->values, id & ~GRN_OBJ_TMP_OBJECT))) {
        res = (grn_obj *)tmp_obj->obj;
      }
    }
  } else {
    grn_obj *db = ctx->impl->db;
    if (db) {
      grn_db *s = (grn_db *)db;
      grn_obj **vp;
      if (!(vp = grn_tiny_array_at(&s->values, id))) { goto exit; }
      if (s->specs && !*vp) {
        grn_io_win jw;
        uint32_t value_len;
        char *value = grn_ja_ref(ctx, s->specs, id, &jw, &value_len);
        if (value) {
          grn_obj v;
          GRN_OBJ_INIT(&v, GRN_VECTOR, 0, GRN_DB_TEXT);
          if (!grn_vector_decode(ctx, &v, value, value_len)) {
            const char *p;
            uint32_t size;
            grn_obj_spec *spec;
            char buffer[PATH_MAX];
            size = grn_vector_get_element(ctx, &v, 0, (const char **)&spec, NULL, NULL);
            if (size) {
              switch (spec->header.type) {
              case GRN_TYPE :
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_type_open(ctx, spec); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_TABLE_HASH_KEY :
                GET_PATH(spec, buffer, s, id);
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_hash_open(ctx, buffer); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_TABLE_PAT_KEY :
                GET_PATH(spec, buffer, s, id);
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_pat_open(ctx, buffer); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_TABLE_NO_KEY :
                GET_PATH(spec, buffer, s, id);
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_array_open(ctx, buffer); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_TABLE_VIEW :
                GET_PATH(spec, buffer, s, id);
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = grn_view_open(ctx, buffer); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_COLUMN_VAR_SIZE :
                GET_PATH(spec, buffer, s, id);
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_ja_open(ctx, buffer); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_COLUMN_FIX_SIZE :
                GET_PATH(spec, buffer, s, id);
                CRITICAL_SECTION_ENTER(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_ra_open(ctx, buffer); }
                CRITICAL_SECTION_LEAVE(s->lock);
                break;
              case GRN_COLUMN_INDEX :
                GET_PATH(spec, buffer, s, id);
                {
                  grn_obj *table = grn_ctx_at(ctx, spec->header.domain);
                  CRITICAL_SECTION_ENTER(s->lock);
                  if (!*vp) { *vp = (grn_obj *)grn_ii_open(ctx, buffer, table); }
                  CRITICAL_SECTION_LEAVE(s->lock);
                }
                break;
              case GRN_PROC :
                if (!*vp) { *vp = grn_proc_open(ctx, spec); }
                break;
              case GRN_EXPR :
                {
                  size = grn_vector_get_element(ctx, &v, 4, &p, NULL, NULL);
                  if (!*vp) { *vp = grn_expr_open(ctx, spec, p, p + size); }
                }
                break;
              }
              if (*vp) {
                grn_db_obj *r = DB_OBJ(*vp);
                r->header = spec->header;
                r->id = id;
                r->range = spec->range;
                r->db = db;
                size = grn_vector_get_element(ctx, &v, 2, &p, NULL, NULL);
                if (size) {
                  if ((r->source = GRN_MALLOC(size))) {
                    memcpy(r->source, p, size);
                    r->source_size = size;
                  }
                }
                size = grn_vector_get_element(ctx, &v, 3, &p, NULL, NULL);
                grn_hook_unpack(ctx, r, p, size);
              }
            }
            grn_obj_close(ctx, &v);
          }
          grn_ja_unref(ctx, &jw);
        }
      }
      res = *vp;
    }
  }
exit :
  GRN_API_RETURN(res);
}

grn_obj *
grn_obj_open(grn_ctx *ctx, unsigned char type, grn_obj_flags flags, grn_id domain)
{
  grn_obj *obj = GRN_MALLOCN(grn_obj, 1);
  if (obj) {
    GRN_OBJ_INIT(obj, type, flags, domain);
    obj->header.impl_flags |= GRN_OBJ_ALLOCATED;
  }
  return obj;
}

grn_obj *
grn_obj_graft(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj *new = grn_obj_open(ctx, obj->header.type, obj->header.impl_flags, obj->header.domain);
  if (new) {
    /* todo : deep copy if (obj->header.impl_flags & GRN_OBJ_DO_SHALLOW_COPY) */
    new->u.b.head = obj->u.b.head;
    new->u.b.curr = obj->u.b.curr;
    new->u.b.tail = obj->u.b.tail;
    obj->u.b.head = NULL;
    obj->u.b.curr = NULL;
    obj->u.b.tail = NULL;
  }
  return new;
}

grn_rc
grn_obj_close(grn_ctx *ctx, grn_obj *obj)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (obj) {
    if (GRN_DB_OBJP(obj)) {
      grn_hook_entry entry;
      if (DB_OBJ(obj)->finalizer) {
        DB_OBJ(obj)->finalizer(ctx, 1, &obj, &DB_OBJ(obj)->user_data);
      }
      if (DB_OBJ(obj)->source) {
        GRN_FREE(DB_OBJ(obj)->source);
      }
      for (entry = 0; entry < N_HOOK_ENTRIES; entry++) {
        grn_hook_free(ctx, DB_OBJ(obj)->hooks[entry]);
      }
      grn_obj_delete_by_id(ctx, DB_OBJ(obj)->db, DB_OBJ(obj)->id, 0);
    }
    switch (obj->header.type) {
    case GRN_VECTOR :
      if (obj->u.v.body && !(obj->header.impl_flags & GRN_OBJ_REFER)) {
        grn_obj_close(ctx, obj->u.v.body);
      }
      if (obj->u.v.sections) { GRN_FREE(obj->u.v.sections); }
      if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) { GRN_FREE(obj); }
      rc = GRN_SUCCESS;
      break;
    case GRN_VOID :
    case GRN_BULK :
    case GRN_PTR :
    case GRN_UVECTOR :
    case GRN_PVECTOR :
    case GRN_MSG :
      obj->header.type = GRN_VOID;
      rc = grn_bulk_fin(ctx, obj);
      if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) { GRN_FREE(obj); }
      break;
    case GRN_ACCESSOR :
      {
        grn_accessor *p, *n;
        for (p = (grn_accessor *)obj; p; p = n) {
          n = p->next;
          GRN_FREE(p);
        }
      }
      rc = GRN_SUCCESS;
      break;
    case GRN_ACCESSOR_VIEW :
      rc = grn_accessor_view_close(ctx, obj);
      break;
    case GRN_CURSOR_TABLE_PAT_KEY :
      grn_pat_cursor_close(ctx, (grn_pat_cursor *)obj);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY :
      grn_hash_cursor_close(ctx, (grn_hash_cursor *)obj);
      break;
    case GRN_CURSOR_TABLE_NO_KEY :
      grn_array_cursor_close(ctx, (grn_array_cursor *)obj);
      break;
    case GRN_CURSOR_TABLE_VIEW :
      grn_view_cursor_close(ctx, (grn_view_cursor *)obj);
      break;
    case GRN_TYPE :
      GRN_FREE(obj);
      rc = GRN_SUCCESS;
      break;
    case GRN_DB :
      rc = grn_db_close(ctx, obj);
      break;
    case GRN_TABLE_PAT_KEY :
      rc = grn_pat_close(ctx, (grn_pat *)obj);
      break;
    case GRN_TABLE_HASH_KEY :
      rc = grn_hash_close(ctx, (grn_hash *)obj);
      break;
    case GRN_TABLE_NO_KEY :
      rc = grn_array_close(ctx, (grn_array *)obj);
      break;
    case GRN_TABLE_VIEW :
      rc = grn_view_close(ctx, (grn_view *)obj);
      break;
    case GRN_QUERY :
      rc = grn_query_close(ctx, (grn_query *)obj);
      break;
    case GRN_COLUMN_VAR_SIZE :
      rc = grn_ja_close(ctx, (grn_ja *)obj);
      break;
    case GRN_COLUMN_FIX_SIZE :
      rc = grn_ra_close(ctx, (grn_ra *)obj);
      break;
    case GRN_COLUMN_INDEX :
      rc = grn_ii_close(ctx, (grn_ii *)obj);
      break;
    case GRN_PROC :
      {
        uint32_t i;
        grn_proc *p = (grn_proc *)obj;
        /*
        if (obj->header.domain) {
          grn_hash_delete(ctx, ctx->impl->qe, &obj->header.domain, sizeof(grn_id), NULL);
        }
        */
        for (i = 0; i < p->nvars; i++) {
          grn_obj_close(ctx, &p->vars[i].value);
        }
        GRN_REALLOC(p->vars, 0);
        grn_obj_close(ctx, &p->name_buf);
        GRN_FREE(obj);
        rc = GRN_SUCCESS;
      }
      break;
    case GRN_EXPR :
      rc = grn_expr_close(ctx, obj);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

#define VECTOR_CLEAR(ctx,obj) {\
  if ((obj)->u.v.body && !((obj)->header.impl_flags & GRN_OBJ_REFER)) {\
    grn_obj_close((ctx), (obj)->u.v.body);\
  }\
  if ((obj)->u.v.sections) { GRN_FREE((obj)->u.v.sections); }\
  (obj)->header.impl_flags &= ~GRN_OBJ_DO_SHALLOW_COPY;\
  (obj)->u.b.head = NULL;\
  (obj)->u.b.curr = NULL;\
  (obj)->u.b.tail = NULL;\
}

grn_rc
grn_obj_reinit(grn_ctx *ctx, grn_obj *obj, grn_id domain, unsigned char flags)
{
  if (!GRN_OBJ_MUTABLE(obj)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid obj assigned");
  } else {
    switch (domain) {
    case GRN_DB_VOID :
      if (obj->header.type == GRN_VECTOR) { VECTOR_CLEAR(ctx,obj); }
      obj->header.type = GRN_VOID;
      obj->header.domain = domain;
      GRN_BULK_REWIND(obj);
      break;
    case GRN_DB_OBJECT :
    case GRN_DB_BOOL :
    case GRN_DB_INT8 :
    case GRN_DB_UINT8 :
    case GRN_DB_INT16 :
    case GRN_DB_UINT16 :
    case GRN_DB_INT32 :
    case GRN_DB_UINT32 :
    case GRN_DB_INT64 :
    case GRN_DB_UINT64 :
    case GRN_DB_FLOAT :
    case GRN_DB_TIME :
      if (obj->header.type == GRN_VECTOR) { VECTOR_CLEAR(ctx,obj); }
      obj->header.type = (flags & GRN_OBJ_VECTOR) ? GRN_UVECTOR : GRN_BULK;
      obj->header.domain = domain;
      GRN_BULK_REWIND(obj);
      break;
    case GRN_DB_SHORT_TEXT :
    case GRN_DB_TEXT :
    case GRN_DB_LONG_TEXT :
      if (flags & GRN_OBJ_VECTOR) {
        if (obj->header.type != GRN_VECTOR) { grn_bulk_fin(ctx, obj); }
        obj->header.type = GRN_VECTOR;
      } else {
        if (obj->header.type == GRN_VECTOR) { VECTOR_CLEAR(ctx,obj); }
        obj->header.type = GRN_BULK;
      }
      obj->header.domain = domain;
      GRN_BULK_REWIND(obj);
      break;
    default :
      {
        grn_obj *d = grn_ctx_at(ctx, domain);
        if (!d) {
          ERR(GRN_INVALID_ARGUMENT, "invalid domain assigned");
        } else {
          if (d->header.type == GRN_TYPE && (d->header.flags & GRN_OBJ_KEY_VAR_SIZE)) {
            if (flags & GRN_OBJ_VECTOR) {
              if (obj->header.type != GRN_VECTOR) { grn_bulk_fin(ctx, obj); }
              obj->header.type = GRN_VECTOR;
            } else {
              if (obj->header.type == GRN_VECTOR) { VECTOR_CLEAR(ctx,obj); }
              obj->header.type = GRN_BULK;
            }
          } else {
            if (obj->header.type == GRN_VECTOR) { VECTOR_CLEAR(ctx,obj); }
            obj->header.type = (flags & GRN_OBJ_VECTOR) ? GRN_UVECTOR : GRN_BULK;
          }
          obj->header.domain = domain;
          GRN_BULK_REWIND(obj);
        }
      }
      break;
    }
  }
  return ctx->rc;
}

const char *
grn_obj_path(grn_ctx *ctx, grn_obj *obj)
{
  grn_io *io;
  char *path = NULL;
  GRN_API_ENTER;
  if (obj->header.type == GRN_PROC) {
    return grn_dl_path(ctx, DB_OBJ(obj)->range);
  }
  io = grn_obj_io(obj);
  if (io && !(io->flags & GRN_IO_TEMPORARY)) { path = io->path; }
  GRN_API_RETURN(path);
}

int
grn_obj_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size)
{
  int len = 0;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    if (DB_OBJ(obj)->id) {
      grn_db *s = (grn_db *)DB_OBJ(obj)->db;
      if (!(DB_OBJ(obj)->id & GRN_OBJ_TMP_OBJECT)) {
        len = grn_pat_get_key(ctx, s->keys, DB_OBJ(obj)->id, namebuf, buf_size);
      }
    }
  }
  GRN_API_RETURN(len);
}

int
grn_column_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size)
{
  int len = 0;
  char buf[GRN_TABLE_MAX_KEY_SIZE];
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    if (DB_OBJ(obj)->id && DB_OBJ(obj)->id < GRN_ID_MAX) {
      grn_db *s = (grn_db *)DB_OBJ(obj)->db;
      len = grn_pat_get_key(ctx, s->keys, DB_OBJ(obj)->id, buf, GRN_TABLE_MAX_KEY_SIZE);
      if (len) {
        int cl;
        char *p = buf, *p0 = p, *pe = p + len;
        for (; p < pe && (cl = grn_charlen(ctx, p, pe)); p += cl) {
          if (*p == GRN_DB_DELIMITER && cl == 1) { p0 = p + cl; }
        }
        len = pe - p0;
        if (len && len <= buf_size) {
          memcpy(namebuf, p0, len);
        }
      }
    }
  }
  GRN_API_RETURN(len);
}

grn_rc
grn_column_name_(grn_ctx *ctx, grn_obj *obj, grn_obj *buf)
{
  while (obj->header.type == GRN_ACCESSOR_VIEW) {
    grn_accessor_view *a = (grn_accessor_view *)obj;
    uint32_t n = a->naccessors;
    for (;;) {
      if (!n) { return ctx->rc; }
      if ((obj = a->accessors[--n])) { break; }
    }
  }
  if (GRN_DB_OBJP(obj)) {
    if (DB_OBJ(obj)->id && DB_OBJ(obj)->id < GRN_ID_MAX) {
      uint32_t len;
      grn_db *s = (grn_db *)DB_OBJ(obj)->db;
      const char *p = _grn_pat_key(ctx, s->keys, DB_OBJ(obj)->id, &len);
      if (len) {
        int cl;
        const char *p0 = p, *pe = p + len;
        for (; p < pe && (cl = grn_charlen(ctx, p, pe)); p += cl) {
          if (*p == GRN_DB_DELIMITER && cl == 1) { p0 = p + cl; }
        }
        GRN_TEXT_PUT(ctx, buf, p0, pe - p0);
      }
    }
  } else if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID :
        GRN_TEXT_PUTS(ctx, buf, "_id");
        break;
      case GRN_ACCESSOR_GET_KEY :
        if (!a->next) {
          GRN_TEXT_PUTS(ctx, buf, "_key");
        }
        break;
      case GRN_ACCESSOR_GET_VALUE :
        if (!a->next) {
          GRN_TEXT_PUTS(ctx, buf, "_value");
        }
        break;
      case GRN_ACCESSOR_GET_SCORE :
        GRN_TEXT_PUTS(ctx, buf, "_score");
        break;
      case GRN_ACCESSOR_GET_NSUBRECS :
        GRN_TEXT_PUTS(ctx, buf, "_nsubrecs");
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE :
        grn_column_name_(ctx, a->obj, buf);
        if (a->next) { GRN_TEXT_PUTC(ctx, buf, '.'); }
        break;
      case GRN_ACCESSOR_GET_DB_OBJ :
      case GRN_ACCESSOR_LOOKUP :
      case GRN_ACCESSOR_FUNCALL :
        break;
      }
    }
  }
  return ctx->rc;
}

int
grn_obj_expire(grn_ctx *ctx, grn_obj *obj, int threshold)
{
  GRN_API_ENTER;
  GRN_API_RETURN(0);
}

int
grn_obj_check(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;
  GRN_API_RETURN(0);
}

grn_rc
grn_obj_lock(grn_ctx *ctx, grn_obj *obj, grn_id id, int timeout)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  rc = grn_io_lock(ctx, grn_obj_io(obj), timeout);
  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_unlock(grn_ctx *ctx, grn_obj *obj, grn_id id)
{
  GRN_API_ENTER;
  grn_io_unlock(grn_obj_io(obj));
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_user_data *
grn_obj_user_data(grn_ctx *ctx, grn_obj *obj)
{
  if (!GRN_DB_OBJP(obj)) { return NULL; }
  return &DB_OBJ(obj)->user_data;
}

grn_rc
grn_obj_set_finalizer(grn_ctx *ctx, grn_obj *obj, grn_proc_func *func)
{
  if (!GRN_DB_OBJP(obj)) { return GRN_INVALID_ARGUMENT; }
  DB_OBJ(obj)->finalizer = func;
  return GRN_SUCCESS;
}

grn_rc
grn_obj_clear_lock(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;
  switch (obj->header.type) {
  case GRN_DB:
    {
      grn_table_cursor *cur;
      if ((cur = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0, 0, -1, 0))) {
        grn_id id;
        while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
          grn_obj *tbl = grn_ctx_at(ctx, id);
          if (tbl) {
            switch (tbl->header.type) {
            case GRN_TABLE_HASH_KEY :
            case GRN_TABLE_NO_KEY:
            case GRN_TABLE_PAT_KEY:
              grn_obj_clear_lock(ctx, tbl);
            }
          }
        }
        grn_table_cursor_close(ctx, cur);
      }
    }
    break;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_NO_KEY :
  case GRN_TABLE_PAT_KEY :
    {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, obj, "", 0, (grn_obj *)cols)) {
          grn_id *key;
          GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col = grn_ctx_at(ctx, *key);
            if (col) { grn_obj_clear_lock(ctx, col); }
          });
        }
        grn_hash_close(ctx, cols);
      }
      grn_io_clear_lock(grn_obj_io(obj));
    }
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_INDEX:
    grn_io_clear_lock(grn_obj_io(obj));
    break;
  }
  GRN_API_RETURN(GRN_SUCCESS);
}

unsigned int
grn_obj_is_locked(grn_ctx *ctx, grn_obj *obj)
{
  unsigned int res = 0;
  GRN_API_ENTER;
  res = grn_io_is_locked(grn_obj_io(obj));
  GRN_API_RETURN(res);
}

grn_obj *
grn_obj_db(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj *db = NULL;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) { db = DB_OBJ(obj)->db; }
  GRN_API_RETURN(db);
}

grn_id
grn_obj_id(grn_ctx *ctx, grn_obj *obj)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    id = DB_OBJ(obj)->id;
  }
  GRN_API_RETURN(id);
}

/**** sort ****/

typedef struct {
  grn_id id;
  uint32_t size;
  const void *value;
} sort_entry;

enum {
  KEY_ID = 0,
  KEY_BULK,
  KEY_INT8,
  KEY_INT16,
  KEY_INT32,
  KEY_INT64,
  KEY_UINT8,
  KEY_UINT16,
  KEY_UINT32,
  KEY_UINT64,
  KEY_FLOAT32,
  KEY_FLOAT64,
};

#define CMPNUM(type) {\
  type va = *((type *)(ap));\
  type vb = *((type *)(bp));\
  if (va != vb) { return va > vb; }\
}

inline static int
compare_value(grn_ctx *ctx, sort_entry *a, sort_entry *b,
              grn_table_sort_key *keys, int n_keys)
{
  int i;
  uint8_t type;
  uint32_t as, bs;
  const char *ap, *bp;
  for (i = 0; i < n_keys; i++, keys++) {
    if (i) {
      if (keys->flags & GRN_TABLE_SORT_DESC) {
        ap = grn_obj_get_value_(ctx, keys->key, b->id, &as);
        bp = grn_obj_get_value_(ctx, keys->key, a->id, &bs);
      } else {
        ap = grn_obj_get_value_(ctx, keys->key, a->id, &as);
        bp = grn_obj_get_value_(ctx, keys->key, b->id, &bs);
      }
    } else {
      if (keys->flags & GRN_TABLE_SORT_DESC) {
        ap = b->value; as = b->size;
        bp = a->value; bs = a->size;
      } else {
        ap = a->value; as = a->size;
        bp = b->value; bs = b->size;
      }
    }
    type = keys->offset;
    switch (type) {
    case KEY_ID :
      if (ap != bp) { return ap > bp; }
      break;
    case KEY_BULK :
      for (;; ap++, bp++, as--, bs--) {
        if (!as) { if (bs) { return 0; } else { break; } }
        if (!bs) { return 1; }
        if (*ap < *bp) { return 0; }
        if (*ap > *bp) { return 1; }
      }
      break;
    case KEY_INT8 :
      CMPNUM(int8_t);
      break;
    case KEY_INT16 :
      CMPNUM(int16_t);
      break;
    case KEY_INT32 :
      CMPNUM(int32_t);
      break;
    case KEY_INT64 :
      CMPNUM(int64_t);
      break;
    case KEY_UINT8 :
      CMPNUM(uint8_t);
      break;
    case KEY_UINT16 :
      CMPNUM(uint16_t);
      break;
    case KEY_UINT32 :
      CMPNUM(uint32_t);
      break;
    case KEY_UINT64 :
      CMPNUM(uint64_t);
      break;
    case KEY_FLOAT32 :
      {
        float va = *((float *)(ap));
        float vb = *((float *)(bp));
        if (va < vb || va > vb) { return va > vb; }
      }
      break;
    case KEY_FLOAT64 :
      {
        double va = *((double *)(ap));
        double vb = *((double *)(bp));
        if (va < vb || va > vb) { return va > vb; }
      }
      break;
    }
  }
  return 0;
}

inline static void
swap(sort_entry *a, sort_entry *b)
{
  sort_entry c_ = *a;
  *a = *b;
  *b = c_;
}

inline static sort_entry *
part(grn_ctx *ctx, sort_entry *b, sort_entry *e, grn_table_sort_key *keys, int n_keys)
{
  sort_entry *c;
  intptr_t d = e - b;
  if (compare_value(ctx, b, e, keys, n_keys)) {
    swap(b, e);
  }
  if (d < 2) { return NULL; }
  c = b + (d >> 1);
  if (compare_value(ctx, b, c, keys, n_keys)) {
    swap(b, c);
  } else {
    if (compare_value(ctx, c, e, keys, n_keys)) {
      swap(c, e);
    }
  }
  if (d < 3) { return NULL; }
  b++;
  swap(b, c);
  c = b;
  for (;;) {
    do {
      b++;
    } while (compare_value(ctx, c, b, keys, n_keys));
    do {
      e--;
    } while (compare_value(ctx, e, c, keys, n_keys));
    if (b >= e) { break; }
    swap(b, e);
  }
  swap(c, e);
  return e;
}

static void
_sort(grn_ctx *ctx, sort_entry *head, sort_entry *tail, int from, int to,
      grn_table_sort_key *keys, int n_keys)
{
  sort_entry *c;
  if (head < tail && (c = part(ctx, head, tail, keys, n_keys))) {
    intptr_t m = c - head + 1;
    if (from < m - 1) { _sort(ctx, head, c - 1, from, to, keys, n_keys); }
    if (m < to) { _sort(ctx, c + 1, tail, from - m, to - m, keys, n_keys); }
  }
}

static sort_entry *
pack(grn_ctx *ctx, grn_obj *table, sort_entry *head, sort_entry *tail,
     grn_table_sort_key *keys, int n_keys)
{
  int i = 0;
  sort_entry e, c;
  grn_table_cursor *tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0);
  if (!tc) { return NULL; }
  if ((c.id = grn_table_cursor_next(ctx, tc))) {
    c.value = grn_obj_get_value_(ctx, keys->key, c.id, &c.size);
    while ((e.id = grn_table_cursor_next(ctx, tc))) {
      e.value = grn_obj_get_value_(ctx, keys->key, e.id, &e.size);
      if (compare_value(ctx, &c, &e, keys, n_keys)) {
        *head++ = e;
      } else {
        *tail-- = e;
      }
      i++;
    }
    *head = c;
    i++;
  }
  grn_table_cursor_close(ctx, tc);
  return i > 2 ? head : NULL;
}

static int
compare_cursor(grn_ctx *ctx, grn_table_cursor *a, grn_table_cursor *b, int n_keys)
{
  int i;
  uint8_t type;
  uint32_t as, bs, cs;
  const char *ap, *bp, *cp;
  grn_table_sort_key *ak, *bk;
  for (i = 0; i < n_keys; i++) {
    ap = grn_table_cursor_get_sort_key_value_(ctx, a, i, &as, &ak);
    bp = grn_table_cursor_get_sort_key_value_(ctx, b, i, &bs, &bk);
    if (ak->flags & GRN_TABLE_SORT_DESC) {
      cp = ap; ap = bp; bp = cp;
      cs = as; as = bs; bs = cs;
    }
    type = ak->offset;
    switch (type) {
    case KEY_ID :
      if (ap != bp) { return ap > bp; }
      break;
    case KEY_BULK :
      for (;; ap++, bp++, as--, bs--) {
        if (!as) { if (bs) { return 0; } else { break; } }
        if (!bs) { return 1; }
        if (*ap < *bp) { return 0; }
        if (*ap > *bp) { return 1; }
      }
      break;
    case KEY_INT8 :
      CMPNUM(int8_t);
      break;
    case KEY_INT16 :
      CMPNUM(int16_t);
      break;
    case KEY_INT32 :
      CMPNUM(int32_t);
      break;
    case KEY_INT64 :
      CMPNUM(int64_t);
      break;
    case KEY_UINT8 :
      CMPNUM(uint8_t);
      break;
    case KEY_UINT16 :
      CMPNUM(uint16_t);
      break;
    case KEY_UINT32 :
      CMPNUM(uint32_t);
      break;
    case KEY_UINT64 :
      CMPNUM(uint64_t);
      break;
    case KEY_FLOAT32 :
      {
        float va = *((float *)(ap));
        float vb = *((float *)(bp));
        if (va < vb || va > vb) { return va > vb; }
      }
      break;
    case KEY_FLOAT64 :
      {
        double va = *((double *)(ap));
        double vb = *((double *)(bp));
        if (va < vb || va > vb) { return va > vb; }
      }
      break;
    }
  }
  return VIEW_CURSOR_OFFSET(a) > VIEW_CURSOR_OFFSET(b);
}

static int
grn_view_sort(grn_ctx *ctx, grn_obj *table, int offset, int limit,
              grn_obj *result, grn_table_sort_key *keys, int n_keys)
{
  int i = 0;
  grn_obj *t, *r;
  grn_id *tp, rid;
  grn_view *tv = (grn_view *)table;
  grn_view *rv = (grn_view *)result;
  grn_hash *th = tv->hash;
  grn_hash *rh = rv->hash;
  grn_table_sort_key *keys_ = GRN_MALLOCN(grn_table_sort_key, n_keys);
  grn_table_sort_key *ks, *kd, *ke = keys + n_keys;
  if (keys_) {
    rv->keys = keys;
    rv->n_keys = n_keys;
    rv->offset = offset;
    for (ks = keys, kd =keys_; ks < ke ; ks++, kd++) { kd->flags = ks->flags; }
    GRN_HASH_EACH(ctx, th, id, &tp, NULL, NULL, {
      grn_hash_get_key(ctx, rh, id, &rid, sizeof(grn_id));
      t = grn_ctx_at(ctx, *tp);
      r = grn_ctx_at(ctx, rid);
      for (ks = keys, kd =keys_; ks < ke ; ks++, kd++) {
        kd->key = ((grn_accessor_view *)ks->key)->accessors[id - 1];
      }
      i += grn_table_sort(ctx, t, 0, offset + limit, r, keys_, n_keys);
      if (r->header.type == GRN_TABLE_NO_KEY) {
        grn_array_copy_sort_key(ctx, (grn_array *)r, keys_, n_keys);
      }
    });
    GRN_FREE(keys_);
  }
  return i;
}

static int
range_is_idp(grn_obj *obj)
{
  if (obj && obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      if (a->action == GRN_ACCESSOR_GET_ID) { return 1; }
    }
  }
  return 0;
}

int
grn_table_sort(grn_ctx *ctx, grn_obj *table, int offset, int limit,
               grn_obj *result, grn_table_sort_key *keys, int n_keys)
{
  grn_obj *index;
  int n, e, i = 0;
  sort_entry *array, *ep;
  GRN_API_ENTER;
  if (!n_keys || !keys) {
    WARN(GRN_INVALID_ARGUMENT, "keys is null");
    goto exit;
  }
  if (!table) {
    WARN(GRN_INVALID_ARGUMENT, "table is null");
    goto exit;
  }
  if (table->header.type == GRN_TABLE_VIEW) {
    i = grn_view_sort(ctx, table, offset, limit, result, keys, n_keys);
    goto exit;
  }
  if (!(result && result->header.type == GRN_TABLE_NO_KEY)) {
    WARN(GRN_INVALID_ARGUMENT, "result is not a array");
    goto exit;
  }
  n = grn_table_size(ctx, table);
  if (grn_normalize_offset_and_limit(ctx, n, &offset, &limit)) {
    goto exit;
  } else {
    e = offset + limit;
  }
  if (n_keys == 1 && grn_column_index(ctx, keys->key, GRN_OP_LESS, &index, 1, NULL)) {
    grn_id tid;
    grn_pat *lexicon = (grn_pat *)grn_ctx_at(ctx, index->header.domain);
    grn_pat_cursor *pc = grn_pat_cursor_open(ctx, lexicon, NULL, 0, NULL, 0,
                                             0 /* offset : can be used in unique index */,
                                             -1 /* limit : can be used in unique index  */,
                                             (keys->flags & GRN_TABLE_SORT_DESC)
                                             ? GRN_CURSOR_DESCENDING
                                             : GRN_CURSOR_ASCENDING);
    if (pc) {
      while (i < e && (tid = grn_pat_cursor_next(ctx, pc))) {
        grn_ii_cursor *ic = grn_ii_cursor_open(ctx, (grn_ii *)index, tid, 0, 0, 1, 0);
        if (ic) {
          grn_ii_posting *posting;
          while (i < e && (posting = grn_ii_cursor_next(ctx, ic))) {
            if (offset <= i) {
              grn_id *v;
              if (!grn_array_add(ctx, (grn_array *)result, (void **)&v)) { break; }
              *v = posting->rid;
            }
            i++;
          }
          grn_ii_cursor_close(ctx, ic);
        }
      }
      grn_pat_cursor_close(ctx, pc);
    }
  } else {
    int j;
    grn_table_sort_key *kp;
    for (kp = keys, j = n_keys; j; kp++, j--) {
      if (range_is_idp(kp->key)) {
        kp->offset = KEY_ID;
      } else {
        grn_obj *range = grn_ctx_at(ctx, grn_obj_get_range(ctx, kp->key));
        if (range->header.type == GRN_TYPE) {
          if (range->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
            kp->offset = KEY_BULK;
          } else {
            uint8_t key_type = range->header.flags & GRN_OBJ_KEY_MASK;
            switch (key_type) {
            case GRN_OBJ_KEY_UINT :
              switch (GRN_TYPE_SIZE(DB_OBJ(range))) {
              case 1 :
                kp->offset = KEY_UINT8;
                break;
              case 2 :
                kp->offset = KEY_UINT16;
                break;
              case 4 :
                kp->offset = KEY_UINT32;
                break;
              case 8 :
                kp->offset = KEY_UINT64;
                break;
              default :
                ERR(GRN_INVALID_ARGUMENT, "unsupported uint value");
                goto exit;
              }
              break;
            case GRN_OBJ_KEY_INT :
              switch (GRN_TYPE_SIZE(DB_OBJ(range))) {
              case 1 :
                kp->offset = KEY_INT8;
                break;
              case 2 :
                kp->offset = KEY_INT16;
                break;
              case 4 :
                kp->offset = KEY_INT32;
                break;
              case 8 :
                kp->offset = KEY_INT64;
                break;
              default :
                ERR(GRN_INVALID_ARGUMENT, "unsupported int value");
                goto exit;
              }
              break;
            case GRN_OBJ_KEY_FLOAT :
              switch (GRN_TYPE_SIZE(DB_OBJ(range))) {
              case 4 :
                kp->offset = KEY_FLOAT32;
                break;
              case 8 :
                kp->offset = KEY_FLOAT64;
                break;
              default :
                ERR(GRN_INVALID_ARGUMENT, "unsupported float value");
                goto exit;
              }
              break;
            }
          }
        } else {
          kp->offset = KEY_UINT32;
        }
      }
    }
    if (!(array = GRN_MALLOC(sizeof(sort_entry) * n))) {
      goto exit;
    }
    if ((ep = pack(ctx, table, array, array + n - 1, keys, n_keys))) {
      intptr_t m = ep - array + 1;
      if (offset < m - 1) { _sort(ctx, array, ep - 1, offset, e, keys, n_keys); }
      if (m < e) { _sort(ctx, ep + 1, array + n - 1, offset - m, e - m, keys, n_keys); }
    }
    {
      grn_id *v;
      for (i = 0, ep = array + offset; i < limit && ep < array + n; i++, ep++) {
        if (!grn_array_add(ctx, (grn_array *)result, (void **)&v)) { break; }
        *v = ep->id;
      }
      GRN_FREE(array);
    }
  }
exit :
  GRN_API_RETURN(i);
}

static grn_obj *
deftype(grn_ctx *ctx, const char *name,
        grn_obj_flags flags,  unsigned int size)
{
  grn_obj *o = grn_ctx_get(ctx, name, strlen(name));
  if (!o) { o = grn_type_create(ctx, name, strlen(name), flags, size); }
  return o;
}

#define N_RESERVED_TYPES      256

grn_rc
grn_db_init_builtin_types(grn_ctx *ctx)
{
  grn_id id;
  grn_obj *obj, *db = ctx->impl->db;
  char buf[] = "Sys00";
  grn_obj_register(ctx, db, buf, 5);
  obj = deftype(ctx, "Object",
                GRN_OBJ_KEY_UINT, sizeof(uint64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_OBJECT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Bool",
                GRN_OBJ_KEY_UINT, sizeof(uint8_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_BOOL) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Int8",
                GRN_OBJ_KEY_INT, sizeof(int8_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT8) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "UInt8",
                GRN_OBJ_KEY_UINT, sizeof(uint8_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT8) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Int16",
                GRN_OBJ_KEY_INT, sizeof(int16_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT16) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "UInt16",
                GRN_OBJ_KEY_UINT, sizeof(uint16_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT16) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Int32",
                GRN_OBJ_KEY_INT, sizeof(int32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT32) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "UInt32",
                GRN_OBJ_KEY_UINT, sizeof(uint32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT32) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Int64",
                GRN_OBJ_KEY_INT, sizeof(int64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT64) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "UInt64",
                GRN_OBJ_KEY_UINT, sizeof(uint64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT64) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Float",
                GRN_OBJ_KEY_FLOAT, sizeof(double));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_FLOAT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Time",
                GRN_OBJ_KEY_INT, sizeof(int64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TIME) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "ShortText",
                GRN_OBJ_KEY_VAR_SIZE, GRN_TABLE_MAX_KEY_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_SHORT_TEXT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "Text",
                GRN_OBJ_KEY_VAR_SIZE, 1 << 16);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TEXT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "LongText",
                GRN_OBJ_KEY_VAR_SIZE, 1 << 31);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_LONG_TEXT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "TokyoGeoPoint",
                GRN_OBJ_KEY_UINT, sizeof(grn_geo_point));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TOKYO_GEO_POINT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "WGS84GeoPoint",
                GRN_OBJ_KEY_UINT, sizeof(grn_geo_point));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_WGS84_GEO_POINT) { return GRN_FILE_CORRUPT; }
  for (id = grn_pat_curr_id(ctx, ((grn_db *)db)->keys) + 1; id < GRN_DB_MECAB; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
#ifdef NO_MECAB
  grn_obj_register(ctx, db, "TokenMecab", 10);
#endif /* NO_MECAB */
  grn_db_init_builtin_tokenizers(ctx);
  for (id = grn_pat_curr_id(ctx, ((grn_db *)db)->keys) + 1; id < 128; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
  grn_db_init_builtin_query(ctx);
  for (id = grn_pat_curr_id(ctx, ((grn_db *)db)->keys) + 1; id < N_RESERVED_TYPES; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
  return ctx->rc;
}

#define MULTI_COLUMN_INDEXP(i) (DB_OBJ(i)->source_size > sizeof(grn_id))

int
grn_column_index(grn_ctx *ctx, grn_obj *obj, grn_operator op,
                 grn_obj **indexbuf, int buf_size, int *section)
{
  int n = 0;
  grn_hook *hooks;
  grn_obj **ip = indexbuf;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    switch (op) {
    case GRN_OP_EQUAL :
      for (hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET]; hooks; hooks = hooks->next) {
        default_set_value_hook_data *data = (void *)NEXT_ADDR(hooks);
        grn_obj *target = grn_ctx_at(ctx, data->target);
        if (target->header.type != GRN_COLUMN_INDEX) { continue; }
        if (section) { *section = (MULTI_COLUMN_INDEXP(target)) ? data->section : 0; }
        {
          grn_obj *tokenizer, *lexicon = grn_ctx_at(ctx, target->header.domain);
          if (!lexicon) { continue; }
          grn_table_get_info(ctx, lexicon, NULL, NULL, &tokenizer);
          if (tokenizer) { continue; }
        }
        if (n < buf_size) {
          *ip++ = target;
        }
        n++;
      }
      break;
    case GRN_OP_MATCH :
      for (hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET]; hooks; hooks = hooks->next) {
        default_set_value_hook_data *data = (void *)NEXT_ADDR(hooks);
        grn_obj *target = grn_ctx_at(ctx, data->target);
        if (target->header.type != GRN_COLUMN_INDEX) { continue; }
        if (section) { *section = (MULTI_COLUMN_INDEXP(target)) ? data->section : 0; }
        if (n < buf_size) {
          *ip++ = target;
        }
        n++;
      }
      break;
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
      for (hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET]; hooks; hooks = hooks->next) {
        default_set_value_hook_data *data = (void *)NEXT_ADDR(hooks);
        grn_obj *target = grn_ctx_at(ctx, data->target);
        if (target->header.type != GRN_COLUMN_INDEX) { continue; }
        if (section) { *section = (MULTI_COLUMN_INDEXP(target)) ? data->section : 0; }
        {
          grn_obj *tokenizer, *lexicon = grn_ctx_at(ctx, target->header.domain);
          if (!lexicon) { continue; }
          if (lexicon->header.type != GRN_TABLE_PAT_KEY) { continue; }
          grn_table_get_info(ctx, lexicon, NULL, NULL, &tokenizer);
          if (tokenizer) { continue; }
        }
        if (n < buf_size) {
          *ip++ = target;
        }
        n++;
      }
      break;
    default :
      break;
    }
  } else if (ACCESSORP(obj)) {
    switch (op) {
    case GRN_OP_EQUAL :
      if (buf_size) { indexbuf[n++] = obj; }
      break;
    case GRN_OP_MATCH :
      {
        grn_accessor *a = (grn_accessor *)obj;
        if (a->action == GRN_ACCESSOR_GET_KEY) {
          obj = a->obj;
          for (hooks = DB_OBJ(obj)->hooks[GRN_HOOK_INSERT]; hooks; hooks = hooks->next) {
            default_set_value_hook_data *data = (void *)NEXT_ADDR(hooks);
            grn_obj *target = grn_ctx_at(ctx, data->target);
            /* todo : data->section */
            if (section) { *section = 0; }
            if (target->header.type != GRN_COLUMN_INDEX) { continue; }
            if (n < buf_size) {
              *ip++ = target;
            }
            n++;
          }
        }
      }
      break;
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
      /* todo */
      break;
    default :
      break;
    }
  }
  GRN_API_RETURN(n);
}

/* grn_expr */

static char *opstrs[] = {
  "PUSH",
  "POP",
  "NOP",
  "CALL",
  "INTERN",
  "GET_REF",
  "GET_VALUE",
  "AND",
  "BUT",
  "OR",
  "ASSIGN",
  "STAR_ASSIGN",
  "SLASH_ASSIGN",
  "MOD_ASSIGN",
  "PLUS_ASSIGN",
  "MINUS_ASSIGN",
  "SHIFTL_ASSIGN",
  "SHIFTR_ASSIGN",
  "SHIFTRR_ASSIGN",
  "AND_ASSIGN",
  "XOR_ASSIGN",
  "OR_ASSIGN",
  "JUMP",
  "CJUMP",
  "COMMA",
  "BITWISE_OR",
  "BITWISE_XOR",
  "BITWISE_AND",
  "BITWISE_NOT",
  "EQUAL",
  "NOT_EQUAL",
  "LESS",
  "GREATER",
  "LESS_EQUAL",
  "GREATER_EQUAL",
  "IN",
  "MATCH",
  "NEAR",
  "NEAR2",
  "SIMILAR",
  "TERM_EXTRACT",
  "SHIFTL",
  "SHIFTR",
  "SHIFTRR",
  "PLUS",
  "MINUS",
  "STAR",
  "SLASH",
  "MOD",
  "DELETE",
  "INCR",
  "DECR",
  "INCR_POST",
  "DECR_POST",
  "NOT",
  "ADJUST",
  "EXACT",
  "LCP",
  "PARTIAL",
  "UNSPLIT",
  "PREFIX",
  "SUFFIX",
  "GEO_DISTANCE1",
  "GEO_DISTANCE2",
  "GEO_DISTANCE3",
  "GEO_DISTANCE4",
  "GEO_WITHINP5",
  "GEO_WITHINP6",
  "GEO_WITHINP8",
  "OBJ_SEARCH",
  "EXPR_GET_VAR",
  "TABLE_CREATE",
  "TABLE_SELECT",
  "TABLE_SORT",
  "TABLE_GROUP",
  "JSON_PUT"
};

static void
put_value(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  int len;
  char namebuf[GRN_TABLE_MAX_KEY_SIZE];
  if ((len = grn_column_name(ctx, obj, namebuf, GRN_TABLE_MAX_KEY_SIZE))) {
    GRN_TEXT_PUT(ctx, buf, namebuf, len);
  } else {
    grn_text_otoj(ctx, buf, obj, NULL);
  }
}

grn_rc
grn_expr_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *expr)
{
  uint32_t i, j;
  grn_expr_var *var;
  grn_expr_code *code;
  grn_expr *e = (grn_expr *)expr;
  GRN_TEXT_PUTS(ctx, buf, "noname");
  GRN_TEXT_PUTC(ctx, buf, '(');
  for (i = 0, var = e->vars; i < e->nvars; i++, var++) {
    if (i) { GRN_TEXT_PUTC(ctx, buf, ','); }
    GRN_TEXT_PUTC(ctx, buf, '?');
    if (var->name_size) {
      GRN_TEXT_PUT(ctx, buf, var->name, var->name_size);
    } else {
      grn_text_itoa(ctx, buf, (int)i);
    }
    GRN_TEXT_PUTC(ctx, buf, ':');
    put_value(ctx, buf, &var->value);
  }
  GRN_TEXT_PUTC(ctx, buf, ')');
  GRN_TEXT_PUTC(ctx, buf, '{');
  for (j = 0, code = e->codes; j < e->codes_curr; j++, code++) {
    if (j) { GRN_TEXT_PUTC(ctx, buf, ','); }
    grn_text_itoa(ctx, buf, code->modify);
    if (code->op == GRN_OP_PUSH) {
      for (i = 0, var = e->vars; i < e->nvars; i++, var++) {
        if (&var->value == code->value) {
          GRN_TEXT_PUTC(ctx, buf, '?');
          if (var->name_size) {
            GRN_TEXT_PUT(ctx, buf, var->name, var->name_size);
          } else {
            grn_text_itoa(ctx, buf, (int)i);
          }
          break;
        }
      }
      if (i == e->nvars) {
        put_value(ctx, buf, code->value);
      }
    } else {
      if (code->value) {
        put_value(ctx, buf, code->value);
        GRN_TEXT_PUTC(ctx, buf, ' ');
      }
      GRN_TEXT_PUTS(ctx, buf, opstrs[code->op]);
    }
  }
  GRN_TEXT_PUTC(ctx, buf, '}');
  return GRN_SUCCESS;
}

grn_obj *
grn_ctx_pop(grn_ctx *ctx)
{
  if (ctx && ctx->impl && ctx->impl->stack_curr) {
    return ctx->impl->stack[--ctx->impl->stack_curr];
  }
  return NULL;
}

grn_rc
grn_ctx_push(grn_ctx *ctx, grn_obj *obj)
{
  if (ctx && ctx->impl && ctx->impl->stack_curr < GRN_STACK_SIZE) {
    ctx->impl->stack[ctx->impl->stack_curr++] = obj;
    return GRN_SUCCESS;
  }
  return GRN_STACK_OVER_FLOW;
}

static grn_obj *
const_new(grn_ctx *ctx, grn_expr *e)
{
  if (!e->consts) {
    if (!(e->consts = GRN_MALLOCN(grn_obj, GRN_STACK_SIZE))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "malloc failed");
      return NULL;
    }
  }
  if (e->nconsts < GRN_STACK_SIZE) {
    return e->consts + e->nconsts++;
  } else {
    ERR(GRN_STACK_OVER_FLOW, "too many constants.");
    return NULL;
  }
}

void
grn_obj_pack(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_text_benc(ctx, buf, obj->header.type);
  if (GRN_DB_OBJP(obj)) {
    grn_text_benc(ctx, buf, DB_OBJ(obj)->id);
  } else {
    // todo : support vector, query, accessor, snip..
    uint32_t vs = GRN_BULK_VSIZE(obj);
    grn_text_benc(ctx, buf, obj->header.domain);
    grn_text_benc(ctx, buf, vs);
    if (vs) { GRN_TEXT_PUT(ctx, buf, GRN_BULK_HEAD(obj), vs); }
  }
}

const uint8_t *
grn_obj_unpack(grn_ctx *ctx, const uint8_t *p, const uint8_t *pe, uint8_t type, uint8_t flags, grn_obj *obj)
{
  grn_id domain;
  uint32_t vs;
  GRN_B_DEC(domain, p);
  GRN_OBJ_INIT(obj, type, flags, domain);
  GRN_B_DEC(vs, p);
  if (pe < p + vs) {
    ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
    return p;
  }
  grn_bulk_write(ctx, obj, p, vs);
  return p + vs;
}

static void
grn_expr_pack(grn_ctx *ctx, grn_obj *buf, grn_obj *expr)
{
  grn_expr_code *c;
  grn_expr_var *v;
  grn_expr *e = (grn_expr *)expr;
  uint32_t i, j;
  grn_text_benc(ctx, buf, e->nvars);
  for (i = e->nvars, v = e->vars; i; i--, v++) {
    grn_text_benc(ctx, buf, v->name_size);
    if (v->name_size) { GRN_TEXT_PUT(ctx, buf, v->name, v->name_size); }
    grn_obj_pack(ctx, buf, &v->value);
  }
  i = e->codes_curr;
  grn_text_benc(ctx, buf, i);
  for (c = e->codes; i; i--, c++) {
    grn_text_benc(ctx, buf, c->op);
    grn_text_benc(ctx, buf, c->nargs);
    if (!c->value) {
      grn_text_benc(ctx, buf, 0); /* NULL */
    } else {
      for (j = 0, v = e->vars; j < e->nvars; j++, v++) {
        if (&v->value == c->value) {
          grn_text_benc(ctx, buf, 1); /* variable */
          grn_text_benc(ctx, buf, j);
          break;
        }
      }
      if (j == e->nvars) {
        grn_text_benc(ctx, buf, 2); /* others */
        grn_obj_pack(ctx, buf, c->value);
      }
    }
  }
}

const uint8_t *
grn_expr_unpack(grn_ctx *ctx, const uint8_t *p, const uint8_t *pe, grn_obj *expr)
{
  grn_obj *v;
  uint8_t type;
  uint32_t i, n, ns;
  grn_expr_code *code;
  grn_expr *e = (grn_expr *)expr;
  GRN_B_DEC(n, p);
  for (i = 0; i < n; i++) {
    GRN_B_DEC(ns, p);
    v = grn_expr_add_var(ctx, expr, ns ? p : NULL, ns);
    p += ns;
    GRN_B_DEC(type, p);
    if (GRN_TYPE <= type && type <= GRN_COLUMN_INDEX) { /* error */ }
    p = grn_obj_unpack(ctx, p, pe, type, 0, v);
    if (pe < p) {
      ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
      return p;
    }
  }
  GRN_B_DEC(n, p);
  /* confirm e->codes_size >= n */
  e->codes_curr = n;
  for (i = 0, code = e->codes; i < n; i++, code++) {
    GRN_B_DEC(code->op, p);
    GRN_B_DEC(code->nargs, p);
    GRN_B_DEC(type, p);
    switch (type) {
    case 0 : /* NULL */
      code->value = NULL;
      break;
    case 1 : /* variable */
      {
        uint32_t offset;
        GRN_B_DEC(offset, p);
        code->value = &e->vars[i].value;
      }
      break;
    case 2 : /* others */
      GRN_B_DEC(type, p);
      if (GRN_TYPE <= type && type <= GRN_COLUMN_INDEX) {
        grn_id id;
        GRN_B_DEC(id, p);
        code->value = grn_ctx_at(ctx, id);
      } else {
        if (!(v = const_new(ctx, e))) { return NULL; }
        p = grn_obj_unpack(ctx, p, pe, type, GRN_OBJ_EXPRCONST, v);
        code->value = v;
      }
      break;
    }
    if (pe < p) {
      ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
      return p;
    }
  }
  return p;
}

static grn_obj *
grn_expr_open(grn_ctx *ctx, grn_obj_spec *spec, const uint8_t *p, const uint8_t *pe)
{
  grn_expr *expr = NULL;
  if ((expr = GRN_MALLOCN(grn_expr, 1))) {
    int size = 256;
    expr->consts = NULL;
    expr->nconsts = 0;
    GRN_TEXT_INIT(&expr->name_buf, 0);
    GRN_TEXT_INIT(&expr->dfi, 0);
    GRN_PTR_INIT(&expr->objs, GRN_OBJ_VECTOR, GRN_ID_NIL);
    expr->vars = NULL;
    expr->nvars = 0;
    GRN_DB_OBJ_SET_TYPE(expr, GRN_EXPR);
    if ((expr->values = GRN_MALLOCN(grn_obj, size))) {
      int i;
      for (i = 0; i < size; i++) {
        GRN_OBJ_INIT(&expr->values[i], GRN_BULK, GRN_OBJ_EXPRVALUE, GRN_ID_NIL);
      }
      expr->values_curr = 0;
      expr->values_tail = 0;
      expr->values_size = size;
      if ((expr->codes = GRN_MALLOCN(grn_expr_code, size))) {
        expr->codes_curr = 0;
        expr->codes_size = size;
        expr->obj.header = spec->header;
        if (grn_expr_unpack(ctx, p, pe, (grn_obj *)expr) == pe) {
          goto exit;
        } else {
          ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
        }
        GRN_FREE(expr->codes);
      }
      GRN_FREE(expr->values);
    }
    GRN_FREE(expr);
    expr = NULL;
  }
exit :
  return (grn_obj *)expr;
}

/* data flow info */
typedef struct {
  grn_expr_code *code;
  grn_id domain;
  unsigned char type;
} grn_expr_dfi;

#define DFI_POP(e,d) {\
  if (GRN_BULK_VSIZE(&(e)->dfi) >= sizeof(grn_expr_dfi)) {\
    GRN_BULK_INCR_LEN((&(e)->dfi), -(sizeof(grn_expr_dfi)));\
    (d) = (grn_expr_dfi *)(GRN_BULK_CURR(&(e)->dfi));\
    (e)->code0 = (d)->code;\
  } else {\
    (d) = NULL;\
    (e)->code0 = NULL;\
  }\
}

#define DFI_PUT(e,t,d,c) {\
  grn_expr_dfi dfi;\
  dfi.type = (t);\
  dfi.domain = (d);\
  dfi.code = (c);\
  if ((e)->code0) { (e)->code0->modify = (c) ? ((c) - (e)->code0) : 0; }\
  grn_bulk_write(ctx, &(e)->dfi, (char *)&dfi, sizeof(grn_expr_dfi));\
  (e)->code0 = NULL;\
}

grn_expr_dfi *
dfi_value_at(grn_expr *expr, int offset)
{
  grn_obj *obj = &expr->dfi;
  int size = GRN_BULK_VSIZE(obj) / sizeof(grn_expr_dfi);
  if (offset < 0) { offset = size + offset; }
  return (0 <= offset && offset < size)
    ? &(((grn_expr_dfi *)GRN_BULK_HEAD(obj))[offset])
    : NULL;
}

grn_obj *
grn_expr_create(grn_ctx *ctx, const char *name, unsigned name_size)
{
  grn_id id;
  grn_obj *db;
  grn_expr *expr = NULL;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  if (name_size) {
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "");
    return NULL;
  }
  GRN_API_ENTER;
  if (check_name(ctx, name, name_size)) {
    CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "named expr is not supported");
    GRN_API_RETURN(NULL);
  }
  id = grn_obj_register(ctx, db, name, name_size);
  if (id && (expr = GRN_MALLOCN(grn_expr, 1))) {
    int size = 256;
    expr->consts = NULL;
    expr->nconsts = 0;
    GRN_TEXT_INIT(&expr->name_buf, 0);
    GRN_TEXT_INIT(&expr->dfi, 0);
    GRN_PTR_INIT(&expr->objs, GRN_OBJ_VECTOR, GRN_ID_NIL);
    expr->code0 = NULL;
    expr->vars = NULL;
    expr->nvars = 0;
    expr->values_curr = 0;
    expr->values_tail = 0;
    expr->values_size = size;
    expr->codes_curr = 0;
    expr->codes_size = size;
    GRN_DB_OBJ_SET_TYPE(expr, GRN_EXPR);
    expr->obj.header.domain = GRN_ID_NIL;
    expr->obj.range = GRN_ID_NIL;
    if (!grn_db_obj_init(ctx, db, id, DB_OBJ(expr))) {
      if ((expr->values = GRN_MALLOCN(grn_obj, size))) {
        int i;
        for (i = 0; i < size; i++) {
          GRN_OBJ_INIT(&expr->values[i], GRN_BULK, GRN_OBJ_EXPRVALUE, GRN_ID_NIL);
        }
        if ((expr->codes = GRN_MALLOCN(grn_expr_code, size))) {
          goto exit;
        }
        GRN_FREE(expr->values);
      }
    }
    GRN_FREE(expr);
    expr = NULL;
  }
exit :
  GRN_API_RETURN((grn_obj *)expr);
}

#define GRN_PTR_POP(obj,value) {\
  if (GRN_BULK_VSIZE(obj) >= sizeof(grn_obj *)) {\
    GRN_BULK_INCR_LEN((obj), -(sizeof(grn_obj *)));\
    value = *(grn_obj **)(GRN_BULK_CURR(obj));\
  } else {\
    value = NULL;\
  }\
}

grn_rc
grn_expr_close(grn_ctx *ctx, grn_obj *expr)
{
  uint32_t i;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  /*
  if (e->obj.header.domain) {
    grn_hash_delete(ctx, ctx->impl->qe, &e->obj.header.domain, sizeof(grn_id), NULL);
  }
  */
  for (i = 0; i < e->nconsts; i++) {
    grn_obj_close(ctx, &e->consts[i]);
  }
  if (e->consts) { GRN_FREE(e->consts);  }
  grn_obj_close(ctx, &e->name_buf);
  grn_obj_close(ctx, &e->dfi);
  for (;;) {
    grn_obj *obj;
    GRN_PTR_POP(&e->objs, obj);
    if (obj) {
      grn_obj_unlink(ctx, obj);
    } else { break; }
  }
  grn_obj_close(ctx, &e->objs);
  for (i = 0; i < e->nvars; i++) {
    grn_obj_close(ctx, &e->vars[i].value);
  }
  GRN_FREE(e->vars);
  for (i = 0; i < e->values_tail; i++) {
    grn_obj_close(ctx, &e->values[i]);
  }
  GRN_FREE(e->values);
  GRN_FREE(e->codes);
  GRN_FREE(e);
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_expr_add_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned name_size)
{
  uint32_t i;
  char *p;
  grn_expr_var *v;
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (!e->vars) {
    if (!(e->vars = GRN_MALLOCN(grn_expr_var, GRN_STACK_SIZE))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "malloc failed");
    }
  }
  if (e->vars && e->nvars < GRN_STACK_SIZE) {
    v = e->vars + e->nvars++;
    GRN_TEXT_PUT(ctx, &e->name_buf, name, name_size);
    v->name_size = name_size;
    res = &v->value;
    GRN_VOID_INIT(res);
    for (i = e->nvars, p = GRN_TEXT_VALUE(&e->name_buf), v = e->vars; i; i--, v++) {
      v->name = p;
      p += v->name_size;
    }
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_get_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned name_size)
{
  uint32_t n;
  grn_expr_var *v;
  for (v = grn_expr_get_vars(ctx, expr, &n); n--; v++) {
    if (v->name_size == name_size && !memcmp(v->name, name, name_size)) {
      return &v->value;
    }
  }
  return NULL;
}

grn_obj *
grn_expr_get_var_by_offset(grn_ctx *ctx, grn_obj *expr, unsigned int offset)
{
  uint32_t n;
  grn_expr_var *v;
  v = grn_expr_get_vars(ctx, expr, &n);
  return (offset < n) ? &v[offset].value : NULL;
}

#define EXPRVP(x) ((x)->header.impl_flags & GRN_OBJ_EXPRVALUE)

#define CONSTP(obj) ((obj) && ((obj)->header.impl_flags & GRN_OBJ_EXPRCONST))

#define PUSH_CODE(e,o,v,n,c) {\
  (c) = &(e)->codes[e->codes_curr++];\
  (c)->value = (v);\
  (c)->nargs = (n);\
  (c)->op = (o);\
  (c)->flags = 0;\
  (c)->modify = 0;\
}

#define APPEND_UNARY_MINUS_OP(e) {                              \
  grn_expr_code *code_;                                         \
  grn_id domain;                                                \
  unsigned char type;                                           \
  grn_obj *x;                                                   \
  DFI_POP(e, dfi);                                              \
  code_ = dfi->code;                                            \
  domain = dfi->domain;                                         \
  type = dfi->type;                                             \
  x = code_->value;                                             \
  if (CONSTP(x)) {                                              \
    switch (domain) {                                           \
    case GRN_DB_UINT32:                                         \
      {                                                         \
        unsigned int value;                                     \
        value = GRN_UINT32_VALUE(x);                            \
        if (value > (unsigned int)0x80000000) {                 \
          domain = GRN_DB_INT64;                                \
          x->header.domain = domain;                            \
          GRN_INT64_SET(ctx, x, -((long long int)value));       \
        } else {                                                \
          domain = GRN_DB_INT32;                                \
          x->header.domain = domain;                            \
          GRN_INT32_SET(ctx, x, -((int)value));                 \
        }                                                       \
      }                                                         \
      break;                                                    \
    case GRN_DB_INT64:                                          \
      GRN_INT64_SET(ctx, x, -GRN_INT64_VALUE(x));               \
      break;                                                    \
    case GRN_DB_FLOAT:                                          \
      GRN_FLOAT_SET(ctx, x, -GRN_FLOAT_VALUE(x));               \
      break;                                                    \
    default:                                                    \
      PUSH_CODE(e, op, obj, nargs, code);                       \
      break;                                                    \
    }                                                           \
  } else {                                                      \
    PUSH_CODE(e, op, obj, nargs, code);                         \
  }                                                             \
  DFI_PUT(e, type, domain, code_);                              \
}

#define PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code) {    \
  PUSH_CODE(e, op, obj, nargs, code);                           \
  {                                                             \
    int i = nargs;                                              \
    while (i--) {                                               \
      DFI_POP(e, dfi);                                          \
    }                                                           \
  }                                                             \
  DFI_PUT(e, type, domain, code);                               \
}

grn_obj *
grn_expr_append_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj, grn_operator op, int nargs)
{
  uint8_t type = GRN_VOID;
  grn_id domain = GRN_ID_NIL;
  grn_expr_dfi *dfi;
  grn_expr_code *code;
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (e->codes_curr >= e->codes_size) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "stack is full");
    goto exit;
  }
  {
    switch (op) {
    case GRN_OP_PUSH :
      if (obj) {
        PUSH_CODE(e, op, obj, nargs, code);
        DFI_PUT(e, obj->header.type, GRN_OBJ_GET_DOMAIN(obj), code);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "obj not assigned for GRN_OP_PUSH");
        goto exit;
      }
      break;
    case GRN_OP_NOP :
      /* nop */
      break;
    case GRN_OP_POP :
      if (obj) {
        ERR(GRN_INVALID_ARGUMENT, "obj assigned for GRN_OP_POP");
        goto exit;
      } else {
        PUSH_CODE(e, op, obj, nargs, code);
        DFI_POP(e, dfi);
      }
      break;
    case GRN_OP_CALL :
      PUSH_CODE(e, op, obj, nargs, code);
      {
        int i = nargs;
        while (i--) { DFI_POP(e, dfi); }
      }
      if (!obj) { DFI_POP(e, dfi); }
      // todo : increment e->values_tail.
      DFI_PUT(e, type, domain, code); /* cannot identify type of return value */
      break;
    case GRN_OP_INTERN :
      if (obj && CONSTP(obj)) {
        grn_obj *value;
        value = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj));
        if (!value) { value = grn_ctx_get(ctx, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj)); }
        if (value) {
          obj = value;
          op = GRN_OP_PUSH;
          type = obj->header.type;
          domain = GRN_OBJ_GET_DOMAIN(obj);
        }
      }
      PUSH_CODE(e, op, obj, nargs, code);
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_EQUAL :
      PUSH_CODE(e, op, obj, nargs, code);
      if (nargs) {
        grn_id xd, yd;
        grn_obj *x, *y;
        int i = nargs - 1;
        if (obj) {
          xd = GRN_OBJ_GET_DOMAIN(obj);
          x = obj;
        } else {
          DFI_POP(e, dfi);
          x = dfi->code->value;
          xd = dfi->domain;
        }
        while (i--) {
          DFI_POP(e, dfi);
          y = dfi->code->value;
          yd = dfi->domain;
        }
        if (CONSTP(x)) {
          if (CONSTP(y)) {
            /* todo */
          } else {
            grn_obj dest;
            if (xd != yd) {
              GRN_OBJ_INIT(&dest, GRN_BULK, 0, yd);
              if (!grn_obj_cast(ctx, x, &dest, 0)) {
                grn_obj_reinit(ctx, x, yd, 0);
                grn_bulk_write(ctx, x, GRN_BULK_HEAD(&dest), GRN_BULK_VSIZE(&dest));
              }
              GRN_OBJ_FIN(ctx, &dest);
            }
          }
        } else {
          if (CONSTP(y)) {
            grn_obj dest;
            if (xd != yd) {
              GRN_OBJ_INIT(&dest, GRN_BULK, 0, xd);
              if (!grn_obj_cast(ctx, y, &dest, 0)) {
                grn_obj_reinit(ctx, y, xd, 0);
                grn_bulk_write(ctx, y, GRN_BULK_HEAD(&dest), GRN_BULK_VSIZE(&dest));
              }
              GRN_OBJ_FIN(ctx, &dest);
            }
          }
        }
      }
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_TABLE_CREATE :
    case GRN_OP_EXPR_GET_VAR :
    case GRN_OP_MATCH :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_GEO_DISTANCE1 :
    case GRN_OP_GEO_DISTANCE2 :
    case GRN_OP_GEO_DISTANCE3 :
    case GRN_OP_GEO_DISTANCE4 :
    case GRN_OP_GEO_WITHINP5 :
    case GRN_OP_GEO_WITHINP6 :
    case GRN_OP_GEO_WITHINP8 :
    case GRN_OP_OBJ_SEARCH :
    case GRN_OP_TABLE_SELECT :
    case GRN_OP_TABLE_SORT :
    case GRN_OP_TABLE_GROUP :
    case GRN_OP_JSON_PUT :
    case GRN_OP_GET_REF :
    case GRN_OP_ADJUST :
      PUSH_CODE(e, op, obj, nargs, code);
      if (nargs) {
        int i = nargs - 1;
        if (!obj) { DFI_POP(e, dfi); }
        while (i--) { DFI_POP(e, dfi); }
      }
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_BUT :
      PUSH_CODE(e, op, obj, nargs, code);
      if (nargs != 2) {
        GRN_LOG(ctx, GRN_LOG_WARNING, "nargs(%d) != 2 in relative op", nargs);
      }
      if (obj) {
        GRN_LOG(ctx, GRN_LOG_WARNING, "obj assigned to relative op");
      }
      {
        int i = nargs;
        while (i--) {
          DFI_POP(e, dfi);
          if (dfi) {
            dfi->code->flags |= GRN_EXPR_CODE_RELATIONAL_EXPRESSION;
          } else {
            GRN_LOG(ctx, GRN_LOG_WARNING, "stack under flow in relative op");
          }
        }
      }
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_PLUS :
      if (nargs > 1) {
        PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code);
      }
      break;
    case GRN_OP_MINUS :
      if (nargs == 1) {
        APPEND_UNARY_MINUS_OP(e);
      } else {
        PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code);
      }
      break;
    case GRN_OP_BITWISE_NOT :
      PUSH_CODE(e, op, obj, nargs, code);
      break;
    case GRN_OP_STAR :
    case GRN_OP_SLASH :
    case GRN_OP_MOD :
    case GRN_OP_SHIFTL :
    case GRN_OP_SHIFTR :
    case GRN_OP_SHIFTRR :
    case GRN_OP_BITWISE_OR :
    case GRN_OP_BITWISE_XOR :
    case GRN_OP_BITWISE_AND :
      PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code);
      break;
    case GRN_OP_INCR :
    case GRN_OP_DECR :
    case GRN_OP_INCR_POST :
    case GRN_OP_DECR_POST :
      {
        DFI_POP(e, dfi);
        if (dfi) {
          type = dfi->type;
          domain = dfi->domain;
          if (dfi->code) {
            if (dfi->code->op == GRN_OP_GET_VALUE) {
              dfi->code->op = GRN_OP_GET_REF;
            }
          }
        }
        PUSH_CODE(e, op, obj, nargs, code);
      }
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_GET_VALUE :
      {
        grn_id vdomain = GRN_ID_NIL;
        if (obj) {
          if (nargs == 1) {
            grn_obj *v = grn_expr_get_var_by_offset(ctx, expr, 0);
            if (v) { vdomain = GRN_OBJ_GET_DOMAIN(v); }
          } else {
            DFI_POP(e, dfi);
            vdomain = dfi->domain;
          }
          if (vdomain && CONSTP(obj) && obj->header.type == GRN_BULK) {
            grn_obj *table = grn_ctx_at(ctx, vdomain);
            grn_obj *col = grn_obj_column(ctx, table, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
            if (col) {
              obj = col;
              type = col->header.type;
              domain = grn_obj_get_range(ctx, col);
              grn_obj_unlink(ctx, col);
            }
          } else {
            domain = grn_obj_get_range(ctx, obj);
          }
          PUSH_CODE(e, op, obj, nargs, code);
        } else {
          grn_expr_dfi *dfi0;
          DFI_POP(e, dfi0);
          if (nargs == 1) {
            grn_obj *v = grn_expr_get_var_by_offset(ctx, expr, 0);
            if (v) { vdomain = GRN_OBJ_GET_DOMAIN(v); }
          } else {
            DFI_POP(e, dfi);
            vdomain = dfi->domain;
          }
          if (dfi0->code->op == GRN_OP_PUSH) {
            dfi0->code->op = op;
            dfi0->code->nargs = nargs;
            obj = dfi0->code->value;
            if (vdomain && obj && CONSTP(obj) && obj->header.type == GRN_BULK) {
              grn_obj *table = grn_ctx_at(ctx, vdomain);
              grn_obj *col = grn_obj_column(ctx, table, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
              if (col) {
                dfi0->code->value = col;
                type = col->header.type;
                domain = grn_obj_get_range(ctx, col);
                grn_obj_unlink(ctx, col);
              }
            } else {
              domain = grn_obj_get_range(ctx, obj);
            }
            code = dfi0->code;
          } else {
            PUSH_CODE(e, op, obj, nargs, code);
          }
        }
      }
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_ASSIGN :
    case GRN_OP_STAR_ASSIGN :
    case GRN_OP_SLASH_ASSIGN :
    case GRN_OP_MOD_ASSIGN :
    case GRN_OP_PLUS_ASSIGN :
    case GRN_OP_MINUS_ASSIGN :
    case GRN_OP_SHIFTL_ASSIGN :
    case GRN_OP_SHIFTR_ASSIGN :
    case GRN_OP_SHIFTRR_ASSIGN :
    case GRN_OP_AND_ASSIGN :
    case GRN_OP_OR_ASSIGN :
    case GRN_OP_XOR_ASSIGN :
      {
        if (obj) {
          type = obj->header.type;
          domain = GRN_OBJ_GET_DOMAIN(obj);
        } else {
          DFI_POP(e, dfi);
          if (dfi) {
            type = dfi->type;
            domain = dfi->domain;
          }
        }
        DFI_POP(e, dfi);
        if (dfi && (dfi->code)) {
          if (dfi->code->op == GRN_OP_GET_VALUE) {
            dfi->code->op = GRN_OP_GET_REF;
          }
        }
        PUSH_CODE(e, op, obj, nargs, code);
      }
      DFI_PUT(e, type, domain, code);
      break;
    case GRN_OP_JUMP :
      DFI_POP(e, dfi);
      PUSH_CODE(e, op, obj, nargs, code);
      break;
    case GRN_OP_CJUMP :
      DFI_POP(e, dfi);
      PUSH_CODE(e, op, obj, nargs, code);
      break;
    default :
      break;
    }
  }
exit :
  if (!ctx->rc) { res = obj; }
  GRN_API_RETURN(res);
}
#undef PUSH_N_ARGS_ARITHMETIC_OP
#undef APPEND_UNARY_MINUS_OP

grn_obj *
grn_expr_append_const(grn_ctx *ctx, grn_obj *expr, grn_obj *obj,
                      grn_operator op, int nargs)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  if (!obj) { return NULL; }
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj) || ACCESSORP(obj)) {
    res = obj;
  } else {
    if ((res = const_new(ctx, e))) {
      switch (obj->header.type) {
      case GRN_BULK :
      case GRN_UVECTOR :
        GRN_OBJ_INIT(res, obj->header.type, 0, obj->header.domain);
        grn_bulk_write(ctx, res, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
        break;
      default :
        res = NULL;
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "unsupported type");
        goto exit;
      }
      res->header.impl_flags |= GRN_OBJ_EXPRCONST;
    }
  }
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
exit :
  GRN_API_RETURN(res);
}

static grn_obj *
grn_expr_add_str(grn_ctx *ctx, grn_obj *expr, const char *str, unsigned str_size)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  if ((res = const_new(ctx, e))) {
    GRN_TEXT_INIT(res, 0);
    grn_bulk_write(ctx, res, str, str_size);
    res->header.impl_flags |= GRN_OBJ_EXPRCONST;
  }
  return res;
}

grn_obj *
grn_expr_append_const_str(grn_ctx *ctx, grn_obj *expr, const char *str, unsigned str_size,
                          grn_operator op, int nargs)
{
  grn_obj *res;
  GRN_API_ENTER;
  res = grn_expr_add_str(ctx, expr, str, str_size);
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_append_const_int(grn_ctx *ctx, grn_obj *expr, int i,
                          grn_operator op, int nargs)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if ((res = const_new(ctx, e))) {
    GRN_INT32_INIT(res, 0);
    GRN_INT32_SET(ctx, res, i);
    res->header.impl_flags |= GRN_OBJ_EXPRCONST;
  }
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
  GRN_API_RETURN(res);
}

grn_rc
grn_expr_append_op(grn_ctx *ctx, grn_obj *expr, grn_operator op, int nargs)
{
  grn_expr_append_obj(ctx, expr, NULL, op, nargs);
  return ctx->rc;
}

grn_rc
grn_expr_compile(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj_spec_save(ctx, DB_OBJ(expr));
  return ctx->rc;
}

void
grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)
{
  if (obj &&
      (!GRN_DB_OBJP(obj) ||
       (((grn_db_obj *)obj)->id & GRN_OBJ_TMP_OBJECT) ||
       obj->header.type == GRN_DB)) {
    grn_obj_close(ctx, obj);
  }
#ifdef CALL_FINALIZER
  else if (GRN_DB_OBJP(obj)) {
    grn_db_obj *dob = DB_OBJ(obj);
    if (dob->finalizer) {
      dob->finalizer(ctx, 1, &obj, &dob->user_data);
      dob->finalizer = NULL;
      dob->user_data.ptr = NULL;
    }
  }
#endif /* CALL_FINALIZER */
}

#define WITH_SPSAVE(block) {\
  ctx->impl->stack_curr = sp - ctx->impl->stack;\
  e->values_curr = vp - e->values;\
  block\
  vp = e->values + e->values_curr;\
  sp = ctx->impl->stack + ctx->impl->stack_curr;\
  s0 = sp[-1];\
  s1 = sp[-2];\
}

#define do_compare_sub(op) {\
  switch (y->header.domain) {\
  case GRN_DB_INT32 :\
    r = (x_ op GRN_INT32_VALUE(y));\
    break;\
  case GRN_DB_UINT32 :\
    r = (x_ op GRN_UINT32_VALUE(y));\
    break;\
  case GRN_DB_INT64 :\
    r = (x_ op GRN_INT64_VALUE(y));\
    break;\
  case GRN_DB_TIME :\
    r = (GRN_TIME_PACK(x_,0) op GRN_INT64_VALUE(y));\
    break;\
  case GRN_DB_UINT64 :\
    r = (x_ op GRN_UINT64_VALUE(y));\
    break;\
  case GRN_DB_FLOAT :\
    r = (x_ op GRN_FLOAT_VALUE(y));\
    break;\
  case GRN_DB_SHORT_TEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONG_TEXT :\
    {\
      const char *p_ = GRN_TEXT_VALUE(y);\
      int i_ = grn_atoi(p_, p_ + GRN_TEXT_LEN(y), NULL);\
      r = (x_ op i_);\
    }\
    break;\
  default :\
    r = 0;\
    break;\
  }\
}

#define do_compare(x,y,r,op) {\
  switch (x->header.domain) {\
  case GRN_DB_INT32 :\
    {\
      int32_t x_ = GRN_INT32_VALUE(x);\
      do_compare_sub(op);\
    }\
    break;\
  case GRN_DB_UINT32 :\
    {\
      uint32_t x_ = GRN_UINT32_VALUE(x);\
      do_compare_sub(op);\
    }\
    break;\
  case GRN_DB_TIME :\
    {\
      int64_t x_ = GRN_INT64_VALUE(x);\
      switch (y->header.domain) {\
      case GRN_DB_INT32 :\
        r = (x_ op GRN_TIME_PACK(GRN_INT32_VALUE(y), 0));\
        break;\
      case GRN_DB_UINT32 :\
        r = (x_ op GRN_TIME_PACK(GRN_UINT32_VALUE(y), 0));\
        break;\
      case GRN_DB_INT64 :\
      case GRN_DB_TIME :\
        r = (x_ op GRN_INT64_VALUE(y));\
        break;\
      case GRN_DB_UINT64 :\
        r = (x_ op GRN_UINT64_VALUE(y));\
        break;\
      case GRN_DB_FLOAT :\
        r = (x_ op GRN_TIME_PACK(GRN_FLOAT_VALUE(y), 0));\
        break;\
      case GRN_DB_SHORT_TEXT :\
      case GRN_DB_TEXT :\
      case GRN_DB_LONG_TEXT :\
        {\
          const char *p_ = GRN_TEXT_VALUE(y);\
          int i_ = grn_atoi(p_, p_ + GRN_TEXT_LEN(y), NULL);\
          r = (x_ op GRN_TIME_PACK(i_, 0));\
        }\
        break;\
      default :\
        r = 0;\
        break;\
      }\
    }\
    break;\
  case GRN_DB_INT64 :\
    {\
      int64_t x_ = GRN_INT64_VALUE(x);\
      do_compare_sub(op);\
    }\
    break;\
  case GRN_DB_UINT64 :\
    {\
      uint64_t x_ = GRN_UINT64_VALUE(x);\
      do_compare_sub(op);\
    }\
    break;\
  case GRN_DB_FLOAT :\
    {\
      double x_ = GRN_FLOAT_VALUE(x);\
      do_compare_sub(op);\
    }\
    break;\
  case GRN_DB_SHORT_TEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONG_TEXT :\
    if (GRN_DB_SHORT_TEXT <= y->header.domain && y->header.domain <= GRN_DB_LONG_TEXT) {\
      int r_;\
      uint32_t la = GRN_TEXT_LEN(x), lb = GRN_TEXT_LEN(y);\
      if (la > lb) {\
        if (!(r_ = memcmp(GRN_TEXT_VALUE(x), GRN_TEXT_VALUE(y), lb))) {\
          r_ = 1;\
        }\
      } else {\
        if (!(r_ = memcmp(GRN_TEXT_VALUE(x), GRN_TEXT_VALUE(y), la))) {\
          r_ = la == lb ? 0 : -1;\
        }\
      }\
      r = (r_ op 0);\
    } else {\
      const char *q_ = GRN_TEXT_VALUE(x);\
      int x_ = grn_atoi(q_, q_ + GRN_TEXT_LEN(x), NULL);\
      do_compare_sub(op);\
    }\
    break;\
  default :\
    r = 0;\
    break;\
  }\
}

#define do_eq_sub {\
  switch (y->header.domain) {\
  case GRN_DB_INT32 :\
    r = (x_ == GRN_INT32_VALUE(y));\
    break;\
  case GRN_DB_UINT32 :\
    r = (x_ == GRN_UINT32_VALUE(y));\
    break;\
  case GRN_DB_INT64 :\
    r = (x_ == GRN_INT64_VALUE(y));\
    break;\
  case GRN_DB_TIME :\
    r = (GRN_TIME_PACK(x_,0) == GRN_INT64_VALUE(y));\
    break;\
  case GRN_DB_UINT64 :\
    r = (x_ == GRN_UINT64_VALUE(y));\
    break;\
  case GRN_DB_FLOAT :\
    r = ((x_ <= GRN_FLOAT_VALUE(y)) && (x_ >= GRN_FLOAT_VALUE(y)));\
    break;\
  case GRN_DB_SHORT_TEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONG_TEXT :\
    {\
      const char *p_ = GRN_TEXT_VALUE(y);\
      int i_ = grn_atoi(p_, p_ + GRN_TEXT_LEN(y), NULL);\
      r = (x_ == i_);\
    }\
    break;\
  default :\
    r = 0;\
    break;\
  }\
}\

#define do_eq(x,y,r) {\
  switch (x->header.domain) {\
  case GRN_DB_VOID :\
    r = 0;\
    break;\
  case GRN_DB_INT32 :\
    {\
      int32_t x_ = GRN_INT32_VALUE(x);\
      do_eq_sub;\
    }\
    break;\
  case GRN_DB_UINT32 :\
    {\
      uint32_t x_ = GRN_UINT32_VALUE(x);\
      do_eq_sub;\
    }\
    break;\
  case GRN_DB_INT64 :\
    {\
      int64_t x_ = GRN_INT64_VALUE(x);\
      do_eq_sub;\
    }\
    break;\
  case GRN_DB_TIME :\
    {\
      int64_t x_ = GRN_INT64_VALUE(x);\
      switch (y->header.domain) {\
      case GRN_DB_INT32 :\
        r = (x_ == GRN_TIME_PACK(GRN_INT32_VALUE(y), 0));\
        break;\
      case GRN_DB_UINT32 :\
        r = (x_ == GRN_TIME_PACK(GRN_UINT32_VALUE(y), 0));\
        break;\
      case GRN_DB_INT64 :\
      case GRN_DB_TIME :\
        r = (x_ == GRN_INT64_VALUE(y));\
        break;\
      case GRN_DB_UINT64 :\
        r = (x_ == GRN_UINT64_VALUE(y));\
        break;\
      case GRN_DB_FLOAT :\
        r = (x_ == GRN_TIME_PACK(GRN_FLOAT_VALUE(y), 0));\
        break;\
      case GRN_DB_SHORT_TEXT :\
      case GRN_DB_TEXT :\
      case GRN_DB_LONG_TEXT :\
        {\
          const char *p_ = GRN_TEXT_VALUE(y);\
          int i_ = grn_atoi(p_, p_ + GRN_TEXT_LEN(y), NULL);\
          r = (x_ == GRN_TIME_PACK(i_, 0));\
        }\
        break;\
      default :\
        r = 0;\
        break;\
      }\
    }\
    break;\
  case GRN_DB_UINT64 :\
    {\
      uint64_t x_ = GRN_UINT64_VALUE(x);\
      do_eq_sub;\
    }\
    break;\
  case GRN_DB_FLOAT :\
    {\
      double x_ = GRN_FLOAT_VALUE(x);\
      switch (y->header.domain) {\
      case GRN_DB_INT32 :\
        r = ((x_ <= GRN_INT32_VALUE(y)) && (x_ >= GRN_INT32_VALUE(y)));\
        break;\
      case GRN_DB_UINT32 :\
        r = ((x_ <= GRN_UINT32_VALUE(y)) && (x_ >= GRN_UINT32_VALUE(y)));\
        break;\
      case GRN_DB_INT64 :\
      case GRN_DB_TIME :\
        r = ((x_ <= GRN_INT64_VALUE(y)) && (x_ >= GRN_INT64_VALUE(y)));\
        break;\
      case GRN_DB_UINT64 :\
        r = ((x_ <= GRN_UINT64_VALUE(y)) && (x_ >= GRN_UINT64_VALUE(y)));\
        break;\
      case GRN_DB_FLOAT :\
        r = ((x_ <= GRN_FLOAT_VALUE(y)) && (x_ >= GRN_FLOAT_VALUE(y)));\
        break;\
      case GRN_DB_SHORT_TEXT :\
      case GRN_DB_TEXT :\
      case GRN_DB_LONG_TEXT :\
        {\
          const char *p_ = GRN_TEXT_VALUE(y);\
          int i_ = grn_atoi(p_, p_ + GRN_TEXT_LEN(y), NULL);\
          r = (x_ <= i_ && x_ >= i_);\
        }\
        break;\
      default :\
        r = 0;\
        break;\
      }\
    }\
    break;\
  case GRN_DB_SHORT_TEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONG_TEXT :\
    if (GRN_DB_SHORT_TEXT <= y->header.domain && y->header.domain <= GRN_DB_LONG_TEXT) {\
      uint32_t la = GRN_TEXT_LEN(x), lb = GRN_TEXT_LEN(y);\
      r =  (la == lb && !memcmp(GRN_TEXT_VALUE(x), GRN_TEXT_VALUE(y), lb));\
    } else {\
      const char *q_ = GRN_TEXT_VALUE(x);\
      int x_ = grn_atoi(q_, q_ + GRN_TEXT_LEN(x), NULL);\
      do_eq_sub;\
    }\
    break;\
  default :\
    if ((x->header.domain == y->header.domain)) {\
      r = (GRN_BULK_VSIZE(x) == GRN_BULK_VSIZE(y) &&\
           !(memcmp(GRN_BULK_HEAD(x), GRN_BULK_HEAD(y), GRN_BULK_VSIZE(x))));\
    } else {\
      grn_obj dest;\
      if (x->header.domain < y->header.domain) {\
        GRN_OBJ_INIT(&dest, GRN_BULK, 0, y->header.domain);\
        if (!grn_obj_cast(ctx, x, &dest, 0)) {\
          r = (GRN_BULK_VSIZE(&dest) == GRN_BULK_VSIZE(y) &&\
               !memcmp(GRN_BULK_HEAD(&dest), GRN_BULK_HEAD(y), GRN_BULK_VSIZE(y))); \
        }\
      } else {\
        GRN_OBJ_INIT(&dest, GRN_BULK, 0, x->header.domain);\
        if (!grn_obj_cast(ctx, y, &dest, 0)) {\
          r = (GRN_BULK_VSIZE(&dest) == GRN_BULK_VSIZE(x) &&\
               !memcmp(GRN_BULK_HEAD(&dest), GRN_BULK_HEAD(x), GRN_BULK_VSIZE(x))); \
        }\
      }\
      GRN_OBJ_FIN(ctx, &dest);\
    }\
    break;\
  }\
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

#define VAR_SET_VALUE(ctx,var,value) {\
  if (GRN_DB_OBJP(value)) {\
    (var)->header.type = GRN_PTR;\
    (var)->header.domain = DB_OBJ(value)->id;\
    GRN_PTR_SET(ctx, (var), (value));\
  } else {\
    (var)->header.type = (value)->header.type;\
    (var)->header.domain = (value)->header.domain;\
    GRN_TEXT_SET(ctx, (var), GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));\
  }\
}

grn_rc
grn_proc_call(grn_ctx *ctx, grn_obj *proc, int nargs, grn_obj *caller)
{
  grn_proc_ctx pctx;
  grn_obj *obj = NULL, **args;
  grn_proc *p = (grn_proc *)proc;
  if (nargs > ctx->impl->stack_curr) { return GRN_INVALID_ARGUMENT; }
  GRN_API_ENTER;
  args = ctx->impl->stack + ctx->impl->stack_curr - nargs;
  pctx.proc = p;
  pctx.caller = caller;
  pctx.user_data.ptr = NULL;
  if (p->funcs[PROC_INIT]) {
    obj = p->funcs[PROC_INIT](ctx, nargs, args, &pctx.user_data);
  }
  pctx.phase = PROC_NEXT;
  if (p->funcs[PROC_NEXT]) {
    obj = p->funcs[PROC_NEXT](ctx, nargs, args, &pctx.user_data);
  }
  pctx.phase = PROC_FIN;
  if (p->funcs[PROC_FIN]) {
    obj = p->funcs[PROC_FIN](ctx, nargs, args, &pctx.user_data);
  }
  ctx->impl->stack_curr -= nargs;
  grn_ctx_push(ctx, obj);
  GRN_API_RETURN(ctx->rc);
}

#define PUSH1(v) {\
  if (EXPRVP(v)) { vp++; }\
  s1 = s0;\
  *sp++ = s0 = v;\
}

#define POP1(v) {\
  if (EXPRVP(s0)) { vp--; }\
  v = s0;\
  s0 = s1;\
  sp--;\
  if (sp < s_) { ERR(GRN_INVALID_ARGUMENT, "stack underflow"); goto exit; }\
  s1 = sp[-2];\
}

#define ALLOC1(value) {\
  s1 = s0;\
  *sp++ = s0 = value = vp++;\
  if (vp - e->values > e->values_tail) { e->values_tail = vp - e->values; }\
}

#define POP1ALLOC1(arg,value) {\
  arg = s0;\
  if (EXPRVP(s0)) {\
    value = s0;\
  } else {\
    if (sp < s_ + 1) { ERR(GRN_INVALID_ARGUMENT, "stack underflow"); goto exit; }\
    sp[-1] = s0 = value = vp++;\
    s0->header.impl_flags |= GRN_OBJ_EXPRVALUE;\
  }\
}

#define POP2ALLOC1(arg1,arg2,value) {\
  if (EXPRVP(s0)) { vp--; }\
  if (EXPRVP(s1)) { vp--; }\
  arg2 = s0;\
  arg1 = s1;\
  sp--;\
  if (sp < s_ + 1) { ERR(GRN_INVALID_ARGUMENT, "stack underflow"); goto exit; }\
  s1 = sp[-2];\
  sp[-1] = s0 = value = vp++;\
  s0->header.impl_flags |= GRN_OBJ_EXPRVALUE;\
}

#define truep(ctx, v, result) {                         \
  switch (v->header.type) {                             \
  case GRN_BULK :                                       \
    switch (v->header.domain) {                         \
    case GRN_DB_BOOL :                                  \
      result = GRN_BOOL_VALUE(v);                       \
      break;                                            \
    case GRN_DB_INT32 :                                 \
      result = GRN_INT32_VALUE(v) != 0;                 \
      break;                                            \
    case GRN_DB_UINT32 :                                \
      result = GRN_UINT32_VALUE(v) != 0;                \
      break;                                            \
    case GRN_DB_FLOAT :                                 \
      {                                                 \
        double float_value;                             \
        float_value = GRN_FLOAT_VALUE(v);               \
        result = (float_value < -DBL_EPSILON ||         \
                  DBL_EPSILON < float_value);           \
      }                                                 \
      break;                                            \
    case GRN_DB_SHORT_TEXT :                            \
    case GRN_DB_TEXT :                                  \
    case GRN_DB_LONG_TEXT :                             \
      result = GRN_TEXT_LEN(v) != 0;                    \
      break;                                            \
    default :                                           \
      result = GRN_FALSE;                               \
      break;                                            \
    }                                                   \
    break;                                              \
  default :                                             \
    result = GRN_FALSE;                                 \
    break;                                              \
  }                                                     \
}

#define INTEGER_ARITHMETIC_OPERATION_PLUS(x, y) ((x) + (y))
#define FLOAT_ARITHMETIC_OPERATION_PLUS(x, y) ((double)(x) + (double)(y))
#define INTEGER_ARITHMETIC_OPERATION_MINUS(x, y) ((x) - (y))
#define FLOAT_ARITHMETIC_OPERATION_MINUS(x, y) ((double)(x) - (double)(y))
#define INTEGER_ARITHMETIC_OPERATION_STAR(x, y) ((x) * (y))
#define FLOAT_ARITHMETIC_OPERATION_STAR(x, y) ((double)(x) * (double)(y))
#define INTEGER_ARITHMETIC_OPERATION_SLASH(x, y) ((x) / (y))
#define FLOAT_ARITHMETIC_OPERATION_SLASH(x, y) ((double)(x) / (double)(y))
#define INTEGER_ARITHMETIC_OPERATION_MOD(x, y) ((x) % (y))
#define FLOAT_ARITHMETIC_OPERATION_MOD(x, y) (fmod((x), (y)))
#define INTEGER_ARITHMETIC_OPERATION_SHIFTL(x, y) ((x) << (y))
#define FLOAT_ARITHMETIC_OPERATION_SHIFTL(x, y)                         \
  ((long long int)(x) << (long long int)(y))
#define INTEGER_ARITHMETIC_OPERATION_SHIFTR(x, y) ((x) >> (y))
#define FLOAT_ARITHMETIC_OPERATION_SHIFTR(x, y)                         \
  ((long long int)(x) >> (long long int)(y))
#define INTEGER32_ARITHMETIC_OPERATION_SHIFTRR(x, y)            \
  ((unsigned int)(x) >> (y))
#define INTEGER64_ARITHMETIC_OPERATION_SHIFTRR(x, y)            \
  ((long long unsigned int)(x) >> (y))
#define FLOAT_ARITHMETIC_OPERATION_SHIFTRR(x, y)                \
  ((long long unsigned int)(x) >> (long long unsigned int)(y))

#define INTEGER_ARITHMETIC_OPERATION_BITWISE_OR(x, y) ((x) | (y))
#define FLOAT_ARITHMETIC_OPERATION_BITWISE_OR(x, y)                \
  ((long long int)(x) | (long long int)(y))
#define INTEGER_ARITHMETIC_OPERATION_BITWISE_XOR(x, y) ((x) ^ (y))
#define FLOAT_ARITHMETIC_OPERATION_BITWISE_XOR(x, y)                \
  ((long long int)(x) ^ (long long int)(y))
#define INTEGER_ARITHMETIC_OPERATION_BITWISE_AND(x, y) ((x) & (y))
#define FLOAT_ARITHMETIC_OPERATION_BITWISE_AND(x, y)                \
  ((long long int)(x) & (long long int)(y))

#define INTEGER_UNARY_ARITHMETIC_OPERATION_MINUS(x) (-(x))
#define FLOAT_UNARY_ARITHMETIC_OPERATION_MINUS(x) (-(x))
#define INTEGER_UNARY_ARITHMETIC_OPERATION_BITWISE_NOT(x) (~(x))
#define FLOAT_UNARY_ARITHMETIC_OPERATION_BITWISE_NOT(x) \
  (~((long long int)(x)))

#define TEXT_ARITHMETIC_OPERATION(operator)                             \
{                                                                       \
  long long int x_;                                                     \
  long long int y_;                                                     \
                                                                        \
  res->header.domain = GRN_DB_INT64;                                    \
                                                                        \
  GRN_INT64_SET(ctx, res, 0);                                           \
  grn_obj_cast(ctx, x, res, GRN_FALSE);                                 \
  x_ = GRN_INT64_VALUE(res);                                            \
                                                                        \
  GRN_INT64_SET(ctx, res, 0);                                           \
  grn_obj_cast(ctx, y, res, GRN_FALSE);                                 \
  y_ = GRN_INT64_VALUE(res);                                            \
                                                                        \
  GRN_INT64_SET(ctx, res, x_ operator y_);                              \
}

#define TEXT_UNARY_ARITHMETIC_OPERATION(unary_operator) \
{                                                       \
  long long int x_;                                     \
                                                        \
  res->header.domain = GRN_DB_INT64;                    \
                                                        \
  GRN_INT64_SET(ctx, res, 0);                           \
  grn_obj_cast(ctx, x, res, GRN_FALSE);                 \
  x_ = GRN_INT64_VALUE(res);                            \
                                                        \
  GRN_INT64_SET(ctx, res, unary_operator x_);           \
}

#define ARITHMETIC_OPERATION_NO_CHECK(y) do {} while (0)
#define ARITHMETIC_OPERATION_ZERO_DIVISION_CHECK(y) do {        \
  if ((long long int)y == 0) {                                  \
    ERR(GRN_INVALID_ARGUMENT, "dividend should not be 0");      \
    goto exit;                                                  \
  }                                                             \
} while (0)


#define NUMERIC_ARITHMETIC_OPERATION_DISPATCH(set, get, x_, y, res,     \
                                              integer_operation,        \
                                              float_operation,          \
                                              right_expression_check,   \
                                              invalid_type_error) {     \
  switch (y->header.domain) {                                           \
  case GRN_DB_INT32 :                                                   \
    {                                                                   \
      int y_;                                                           \
      y_ = GRN_INT32_VALUE(y);                                          \
      right_expression_check(y_);                                       \
      set(ctx, res, integer_operation(x_, y_));                         \
    }                                                                   \
    break;                                                              \
  case GRN_DB_UINT32 :                                                  \
    {                                                                   \
      unsigned int y_;                                                  \
      y_ = GRN_UINT32_VALUE(y);                                         \
      right_expression_check(y_);                                       \
      set(ctx, res, integer_operation(x_, y_));                         \
    }                                                                   \
    break;                                                              \
  case GRN_DB_TIME :                                                    \
    {                                                                   \
      long long int y_;                                                 \
      y_ = GRN_TIME_VALUE(y);                                           \
      right_expression_check(y_);                                       \
      set(ctx, res, integer_operation(x_, y_));                         \
    }                                                                   \
    break;                                                              \
  case GRN_DB_INT64 :                                                   \
    {                                                                   \
      long long int y_;                                                 \
      y_ = GRN_INT64_VALUE(y);                                          \
      right_expression_check(y_);                                       \
      set(ctx, res, integer_operation(x_, y_));                         \
    }                                                                   \
    break;                                                              \
  case GRN_DB_UINT64 :                                                  \
    {                                                                   \
      long long unsigned int y_;                                        \
      y_ = GRN_UINT64_VALUE(y);                                         \
      right_expression_check(y_);                                       \
      set(ctx, res, integer_operation(x_, y_));                         \
    }                                                                   \
    break;                                                              \
  case GRN_DB_FLOAT :                                                   \
    {                                                                   \
      double y_;                                                        \
      y_ = GRN_FLOAT_VALUE(y);                                          \
      right_expression_check(y_);                                       \
      res->header.domain = GRN_DB_FLOAT;                                \
      GRN_FLOAT_SET(ctx, res, float_operation(x_, y_));                 \
    }                                                                   \
    break;                                                              \
  case GRN_DB_SHORT_TEXT :                                              \
  case GRN_DB_TEXT :                                                    \
  case GRN_DB_LONG_TEXT :                                               \
    set(ctx, res, 0);                                                   \
    if (grn_obj_cast(ctx, y, res, GRN_FALSE)) {                         \
      ERR(GRN_INVALID_ARGUMENT,                                         \
          "not a numerical format: <%.*s>",                             \
          GRN_TEXT_LEN(y), GRN_TEXT_VALUE(y));                          \
      goto exit;                                                        \
    }                                                                   \
    set(ctx, res, integer_operation(x_, get(res)));                     \
    break;                                                              \
  default :                                                             \
    invalid_type_error;                                                 \
    break;                                                              \
  }                                                                     \
}


#define ARITHMETIC_OPERATION_DISPATCH(x, y, res,                        \
                                      integer32_operation,              \
                                      integer64_operation,              \
                                      float_operation,                  \
                                      left_expression_check,            \
                                      right_expression_check,           \
                                      text_operation,                   \
                                      invalid_type_error) {             \
  switch (x->header.domain) {                                           \
  case GRN_DB_INT32 :                                                   \
    {                                                                   \
      int x_;                                                           \
      x_ = GRN_INT32_VALUE(x);                                          \
      left_expression_check(x_);                                        \
      NUMERIC_ARITHMETIC_OPERATION_DISPATCH(GRN_INT32_SET,              \
                                            GRN_INT32_VALUE,            \
                                            x_, y, res,                 \
                                            integer32_operation,        \
                                            float_operation,            \
                                            right_expression_check,     \
                                            invalid_type_error);        \
    }                                                                   \
    break;                                                              \
  case GRN_DB_UINT32 :                                                  \
    {                                                                   \
      unsigned int x_;                                                  \
      x_ = GRN_UINT32_VALUE(x);                                         \
      left_expression_check(x_);                                        \
      NUMERIC_ARITHMETIC_OPERATION_DISPATCH(GRN_UINT32_SET,             \
                                            GRN_UINT32_VALUE,           \
                                            x_, y, res,                 \
                                            integer32_operation,        \
                                            float_operation,            \
                                            right_expression_check,     \
                                            invalid_type_error);        \
    }                                                                   \
    break;                                                              \
  case GRN_DB_INT64 :                                                   \
    {                                                                   \
      long long int x_;                                                 \
      x_ = GRN_INT64_VALUE(x);                                          \
      left_expression_check(x_);                                        \
      NUMERIC_ARITHMETIC_OPERATION_DISPATCH(GRN_UINT64_SET,             \
                                            GRN_UINT64_VALUE,           \
                                            x_, y, res,                 \
                                            integer64_operation,        \
                                            float_operation,            \
                                            right_expression_check,     \
                                            invalid_type_error);        \
    }                                                                   \
    break;                                                              \
  case GRN_DB_TIME :                                                    \
    {                                                                   \
      long long int x_;                                                 \
      x_ = GRN_TIME_VALUE(x);                                           \
      left_expression_check(x_);                                        \
      NUMERIC_ARITHMETIC_OPERATION_DISPATCH(GRN_TIME_SET,               \
                                            GRN_TIME_VALUE,             \
                                            x_, y, res,                 \
                                            integer64_operation,        \
                                            float_operation,            \
                                            right_expression_check,     \
                                            invalid_type_error);        \
    }                                                                   \
    break;                                                              \
  case GRN_DB_UINT64 :                                                  \
    {                                                                   \
      long long unsigned int x_;                                        \
      x_ = GRN_UINT64_VALUE(x);                                         \
      left_expression_check(x_);                                        \
      NUMERIC_ARITHMETIC_OPERATION_DISPATCH(GRN_UINT64_SET,             \
                                            GRN_UINT64_VALUE,           \
                                            x_, y, res,                 \
                                            integer64_operation,        \
                                            float_operation,            \
                                            right_expression_check,     \
                                            invalid_type_error);        \
    }                                                                   \
    break;                                                              \
  case GRN_DB_FLOAT :                                                   \
    {                                                                   \
      double x_;                                                        \
      x_ = GRN_FLOAT_VALUE(x);                                          \
      left_expression_check(x_);                                        \
      NUMERIC_ARITHMETIC_OPERATION_DISPATCH(GRN_FLOAT_SET,              \
                                            GRN_FLOAT_VALUE,            \
                                            x_, y, res,                 \
                                            float_operation,            \
                                            float_operation,            \
                                            right_expression_check,     \
                                            invalid_type_error);        \
    }                                                                   \
    break;                                                              \
  case GRN_DB_SHORT_TEXT :                                              \
  case GRN_DB_TEXT :                                                    \
  case GRN_DB_LONG_TEXT :                                               \
    text_operation;                                                     \
    break;                                                              \
  default:                                                              \
    invalid_type_error;                                                 \
    break;                                                              \
  }                                                                     \
  code++;                                                               \
}

#define ARITHMETIC_BINARY_OPERATION_DISPATCH(integer32_operation,       \
                                             integer64_operation,       \
                                             float_operation,           \
                                             left_expression_check,     \
                                             right_expression_check,    \
                                             text_operation,            \
                                             invalid_type_error) {      \
  grn_obj *x, *y;                                                       \
                                                                        \
  POP2ALLOC1(x, y, res);                                                \
  res->header.domain = x->header.domain;                                \
  ARITHMETIC_OPERATION_DISPATCH(x, y, res,                              \
                                integer32_operation,                    \
                                integer64_operation,                    \
                                float_operation,                        \
                                left_expression_check,                  \
                                right_expression_check,                 \
                                text_operation,                         \
                                invalid_type_error);                    \
}

#define ARITHMETIC_UNARY_OPERATION_DISPATCH(integer_operation,          \
                                            float_operation,            \
                                            left_expression_check,      \
                                            right_expression_check,     \
                                            text_operation,             \
                                            invalid_type_error) {       \
  grn_obj *x;                                                           \
  POP1ALLOC1(x, res);                                                   \
  res->header.domain = x->header.domain;                                \
  switch (x->header.domain) {                                           \
  case GRN_DB_INT32 :                                                   \
    {                                                                   \
      int x_;                                                           \
      x_ = GRN_INT32_VALUE(x);                                          \
      left_expression_check(x_);                                        \
      GRN_INT32_SET(ctx, res, integer_operation(x_));                   \
    }                                                                   \
    break;                                                              \
  case GRN_DB_UINT32 :                                                  \
    {                                                                   \
      unsigned int x_;                                                  \
      x_ = GRN_UINT32_VALUE(x);                                         \
      left_expression_check(x_);                                        \
      GRN_UINT32_SET(ctx, res, integer_operation(x_));                  \
    }                                                                   \
    break;                                                              \
  case GRN_DB_INT64 :                                                   \
    {                                                                   \
      long long int x_;                                                 \
      x_ = GRN_INT64_VALUE(x);                                          \
      left_expression_check(x_);                                        \
      GRN_INT64_SET(ctx, res, integer_operation(x_));                   \
    }                                                                   \
    break;                                                              \
  case GRN_DB_TIME :                                                    \
    {                                                                   \
      long long int x_;                                                 \
      x_ = GRN_TIME_VALUE(x);                                           \
      left_expression_check(x_);                                        \
      GRN_TIME_SET(ctx, res, integer_operation(x_));                    \
    }                                                                   \
    break;                                                              \
  case GRN_DB_UINT64 :                                                  \
    {                                                                   \
      long long unsigned int x_;                                        \
      x_ = GRN_UINT64_VALUE(x);                                         \
      left_expression_check(x_);                                        \
      GRN_UINT64_SET(ctx, res, integer_operation(x_));                  \
    }                                                                   \
    break;                                                              \
  case GRN_DB_FLOAT :                                                   \
    {                                                                   \
      double x_;                                                        \
      x_ = GRN_FLOAT_VALUE(x);                                          \
      left_expression_check(x_);                                        \
      GRN_FLOAT_SET(ctx, res, float_operation(x_));                     \
    }                                                                   \
    break;                                                              \
  case GRN_DB_SHORT_TEXT :                                              \
  case GRN_DB_TEXT :                                                    \
  case GRN_DB_LONG_TEXT :                                               \
    text_operation;                                                     \
    break;                                                              \
  default:                                                              \
    invalid_type_error;                                                 \
    break;                                                              \
  }                                                                     \
  code++;                                                               \
}

#define EXEC_OPERATE(operate_sentence, assign_sentence)   \
  operate_sentence                                        \
  assign_sentence

#define EXEC_OPERATE_POST(operate_sentence, assign_sentence)    \
  assign_sentence                                               \
  operate_sentence

#define UNARY_OPERATE_AND_ASSIGN_DISPATCH(exec_operate, delta,          \
                                          set_flags) {                  \
  grn_obj *var, *col, value;                                            \
  grn_id rid;                                                           \
                                                                        \
  POP1ALLOC1(var, res);                                                 \
  if (var->header.type != GRN_PTR) {                                    \
    ERR(GRN_INVALID_ARGUMENT, "invalid variable type: 0x%0x",           \
        var->header.type);                                              \
    goto exit;                                                          \
  }                                                                     \
  if (GRN_BULK_VSIZE(var) != (sizeof(grn_obj *) + sizeof(grn_id))) {    \
    ERR(GRN_INVALID_ARGUMENT,                                           \
        "invalid variable size: expected: %ld, actual: %ld",            \
        (sizeof(grn_obj *) + sizeof(grn_id)), GRN_BULK_VSIZE(var));     \
    goto exit;                                                          \
  }                                                                     \
  col = GRN_PTR_VALUE(var);                                             \
  rid = *(grn_id *)(GRN_BULK_HEAD(var) + sizeof(grn_obj *));            \
  res->header.type = GRN_VOID;                                          \
  res->header.domain = DB_OBJ(col)->range;                              \
  switch (DB_OBJ(col)->range) {                                         \
  case GRN_DB_INT32 :                                                   \
    GRN_INT32_INIT(&value, 0);                                          \
    GRN_INT32_SET(ctx, &value, delta);                                  \
    break;                                                              \
  case GRN_DB_UINT32 :                                                  \
    GRN_UINT32_INIT(&value, 0);                                         \
    GRN_UINT32_SET(ctx, &value, delta);                                 \
    break;                                                              \
  case GRN_DB_INT64 :                                                   \
    GRN_INT64_INIT(&value, 0);                                          \
    GRN_INT64_SET(ctx, &value, delta);                                  \
    break;                                                              \
  case GRN_DB_UINT64 :                                                  \
    GRN_UINT64_INIT(&value, 0);                                         \
    GRN_UINT64_SET(ctx, &value, delta);                                 \
    break;                                                              \
  case GRN_DB_FLOAT :                                                   \
    GRN_FLOAT_INIT(&value, 0);                                          \
    GRN_FLOAT_SET(ctx, &value, delta);                                  \
    break;                                                              \
  case GRN_DB_TIME :                                                    \
    GRN_TIME_INIT(&value, 0);                                           \
    GRN_TIME_SET(ctx, &value, GRN_TIME_PACK(delta, 0));                 \
    break;                                                              \
  default:                                                              \
    ERR(GRN_INVALID_ARGUMENT,                                           \
        "invalid increment target type: %d "                            \
        "(FIXME: type name is needed)", DB_OBJ(col)->range);            \
    goto exit;                                                          \
    break;                                                              \
  }                                                                     \
  exec_operate(grn_obj_set_value(ctx, col, rid, &value, set_flags);,    \
               grn_obj_get_value(ctx, col, rid, res););                 \
  code++;                                                               \
}

#define ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(integer32_operation,   \
                                                 integer64_operation,   \
                                                 float_operation,       \
                                                 left_expression_check, \
                                                 right_expression_check,\
                                                 text_operation)        \
{                                                                       \
  grn_obj *value, *casted_value, *variable_value, *var;                 \
  if (code->value) {                                                    \
    value = code->value;                                                \
  } else {                                                              \
    POP1(value);                                                        \
  }                                                                     \
  value = GRN_OBJ_RESOLVE(ctx, value);                                  \
  POP1(var);                                                            \
  if (var->header.type == GRN_PTR &&                                    \
      GRN_BULK_VSIZE(var) == (sizeof(grn_obj *) + sizeof(grn_id))) {    \
    grn_obj *col = GRN_PTR_VALUE(var);                                  \
    grn_id rid = *(grn_id *)(GRN_BULK_HEAD(var) + sizeof(grn_obj *));   \
                                                                        \
    ALLOC1(res);                                                        \
    ALLOC1(variable_value);                                             \
    ALLOC1(casted_value);                                               \
    grn_obj_reinit(ctx, variable_value, col->header.domain, 0);         \
    grn_obj_get_value(ctx, col, rid, variable_value);                   \
                                                                        \
    casted_value->header.type = GRN_BULK;                               \
    casted_value->header.domain = variable_value->header.domain;        \
    if (grn_obj_cast(ctx, value, casted_value, GRN_FALSE)) {            \
      ERR(GRN_INVALID_ARGUMENT, "invalid value: string");               \
      goto exit;                                                        \
    }                                                                   \
    res->header.type = GRN_BULK;                                        \
    res->header.domain = variable_value->header.domain;                 \
    ARITHMETIC_OPERATION_DISPATCH(variable_value, casted_value, res,    \
                                  integer32_operation,                  \
                                  integer64_operation,                  \
                                  float_operation,                      \
                                  left_expression_check,                \
                                  right_expression_check,               \
                                  text_operation,);                     \
    POP1(casted_value);                                                 \
    POP1(variable_value);                                               \
    grn_obj_set_value(ctx, col, rid, res, GRN_OBJ_SET);                 \
  }                                                                     \
}

grn_rc
grn_expr_exec(grn_ctx *ctx, grn_obj *expr, int nargs)
{
  uint32_t stack_curr = ctx->impl->stack_curr;
  GRN_API_ENTER;
  if (expr->header.type == GRN_PROC) {
    grn_proc_call(ctx, expr, nargs, expr);
  } else {
    grn_expr *e = (grn_expr *)expr;
    register grn_obj **s_ = ctx->impl->stack, *s0 = NULL, *s1 = NULL, **sp, *vp = e->values;
    grn_obj *res = NULL, *v0 = grn_expr_get_var_by_offset(ctx, expr, 0);
    grn_expr_code *code = e->codes, *ce = &e->codes[e->codes_curr];
    sp = s_ + stack_curr;
    while (code < ce) {
      switch (code->op) {
      case GRN_OP_NOP :
        code++;
        break;
      case GRN_OP_PUSH :
        PUSH1(code->value);
        code++;
        break;
      case GRN_OP_POP :
        {
          grn_obj *obj;
          POP1(obj);
          code++;
        }
        break;
      case GRN_OP_GET_REF :
        {
          grn_obj *col, *rec;
          if (code->nargs == 1) {
            rec = v0;
            if (code->value) {
              col = code->value;
              ALLOC1(res);
            } else {
              POP1ALLOC1(col, res);
            }
          } else {
            if (code->value) {
              col = code->value;
              POP1ALLOC1(rec, res);
            } else {
              POP2ALLOC1(rec, col, res);
            }
          }
          if (col->header.type == GRN_BULK) {
            grn_obj *table = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(rec));
            col = grn_obj_column(ctx, table, GRN_BULK_HEAD(col), GRN_BULK_VSIZE(col));
            if (col) { GRN_PTR_PUT(ctx, &e->objs, col); }
          }
          if (col) {
            res->header.type = GRN_PTR;
            res->header.domain = GRN_ID_NIL;
            GRN_PTR_SET(ctx, res, col);
            GRN_UINT32_PUT(ctx, res, GRN_RECORD_VALUE(rec));
          } else {
            ERR(GRN_INVALID_ARGUMENT, "col resolve failed");
            goto exit;
          }
          code++;
        }
        break;
      case GRN_OP_CALL :
        {
          grn_obj *proc;
          if (code->value) {
            if (sp < s_ + code->nargs) {
              ERR(GRN_INVALID_ARGUMENT, "stack error");
              goto exit;
            }
            proc = code->value;
            WITH_SPSAVE({
              grn_proc_call(ctx, proc, code->nargs, expr);
            });
          } else {
            int offset = code->nargs + 1;
            if (sp < s_ + offset) {
              ERR(GRN_INVALID_ARGUMENT, "stack error");
              goto exit;
            }
            proc = sp[-offset];
            WITH_SPSAVE({
              grn_proc_call(ctx, proc, code->nargs, expr);
            });
            POP1(res);
            {
              grn_obj *proc_;
              POP1(proc_);
              if (proc != proc_) {
                GRN_LOG(ctx, GRN_LOG_WARNING, "stack may be corrupt");
              }
            }
            PUSH1(res);
          }
        }
        code++;
        break;
      case GRN_OP_INTERN :
        {
          grn_obj *obj;
          POP1(obj);
          obj = GRN_OBJ_RESOLVE(ctx, obj);
          res = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj));
          if (!res) { res = grn_ctx_get(ctx, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj)); }
          if (!res) {
            ERR(GRN_INVALID_ARGUMENT, "intern failed");
            goto exit;
          }
          PUSH1(res);
        }
        code++;
        break;
      case GRN_OP_TABLE_CREATE :
        {
          grn_obj *value_type, *key_type, *flags, *name;
          POP1(value_type);
          value_type = GRN_OBJ_RESOLVE(ctx, value_type);
          POP1(key_type);
          key_type = GRN_OBJ_RESOLVE(ctx, key_type);
          POP1(flags);
          flags = GRN_OBJ_RESOLVE(ctx, flags);
          POP1(name);
          name = GRN_OBJ_RESOLVE(ctx, name);
          res = grn_table_create(ctx, GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name),
                                 NULL, GRN_UINT32_VALUE(flags),
                                 key_type, value_type);
          PUSH1(res);
        }
        code++;
        break;
      case GRN_OP_EXPR_GET_VAR :
        {
          grn_obj *name, *expr;
          POP1(name);
          name = GRN_OBJ_RESOLVE(ctx, name);
          POP1(expr);
          expr = GRN_OBJ_RESOLVE(ctx, expr);
          switch (name->header.domain) {
          case GRN_DB_INT32 :
            res = grn_expr_get_var_by_offset(ctx, expr, (unsigned int) GRN_INT32_VALUE(name));
            break;
          case GRN_DB_UINT32 :
            res = grn_expr_get_var_by_offset(ctx, expr, (unsigned int) GRN_UINT32_VALUE(name));
            break;
          case GRN_DB_INT64 :
            res = grn_expr_get_var_by_offset(ctx, expr, (unsigned int) GRN_INT64_VALUE(name));
            break;
          case GRN_DB_UINT64 :
            res = grn_expr_get_var_by_offset(ctx, expr, (unsigned int) GRN_UINT64_VALUE(name));
            break;
          case GRN_DB_SHORT_TEXT :
          case GRN_DB_TEXT :
          case GRN_DB_LONG_TEXT :
            res = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name));
            break;
          default :
            ERR(GRN_INVALID_ARGUMENT, "invalid type");
            goto exit;
          }
          PUSH1(res);
        }
        code++;
        break;
      case GRN_OP_ASSIGN :
        {
          grn_obj *value, *var;
          if (code->value) {
            value = code->value;
          } else {
            POP1(value);
          }
          value = GRN_OBJ_RESOLVE(ctx, value);
          POP1(var);
          // var = GRN_OBJ_RESOLVE(ctx, var);
          if (var->header.type == GRN_PTR &&
              GRN_BULK_VSIZE(var) == (sizeof(grn_obj *) + sizeof(grn_id))) {
            grn_obj *col = GRN_PTR_VALUE(var);
            grn_id rid = *(grn_id *)(GRN_BULK_HEAD(var) + sizeof(grn_obj *));
            grn_obj_set_value(ctx, col, rid, value, GRN_OBJ_SET);
          } else {
            VAR_SET_VALUE(ctx, var, value);
          }
          PUSH1(value);
        }
        code++;
        break;
      case GRN_OP_STAR_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_STAR,
          INTEGER_ARITHMETIC_OPERATION_STAR,
          FLOAT_ARITHMETIC_OPERATION_STAR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable *= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_SLASH_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_SLASH,
          INTEGER_ARITHMETIC_OPERATION_SLASH,
          FLOAT_ARITHMETIC_OPERATION_SLASH,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable /= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_MOD_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_MOD,
          INTEGER_ARITHMETIC_OPERATION_MOD,
          FLOAT_ARITHMETIC_OPERATION_MOD,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable %= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_PLUS_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_PLUS,
          INTEGER_ARITHMETIC_OPERATION_PLUS,
          FLOAT_ARITHMETIC_OPERATION_PLUS,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable += \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_MINUS_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_MINUS,
          INTEGER_ARITHMETIC_OPERATION_MINUS,
          FLOAT_ARITHMETIC_OPERATION_MINUS,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable -= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_SHIFTL_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_SHIFTL,
          INTEGER_ARITHMETIC_OPERATION_SHIFTL,
          FLOAT_ARITHMETIC_OPERATION_SHIFTL,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable <<= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_SHIFTR_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_SHIFTR,
          INTEGER_ARITHMETIC_OPERATION_SHIFTR,
          FLOAT_ARITHMETIC_OPERATION_SHIFTR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable >>= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_SHIFTRR_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER32_ARITHMETIC_OPERATION_SHIFTRR,
          INTEGER64_ARITHMETIC_OPERATION_SHIFTRR,
          FLOAT_ARITHMETIC_OPERATION_SHIFTRR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT,
                "variable >>>= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_AND_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_BITWISE_AND,
          INTEGER_ARITHMETIC_OPERATION_BITWISE_AND,
          FLOAT_ARITHMETIC_OPERATION_BITWISE_AND,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable &= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_OR_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_BITWISE_OR,
          INTEGER_ARITHMETIC_OPERATION_BITWISE_OR,
          FLOAT_ARITHMETIC_OPERATION_BITWISE_OR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable |= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_XOR_ASSIGN :
        ARITHMETIC_OPERATION_AND_ASSIGN_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_BITWISE_XOR,
          INTEGER_ARITHMETIC_OPERATION_BITWISE_XOR,
          FLOAT_ARITHMETIC_OPERATION_BITWISE_XOR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT, "variable ^= \"string\" isn't supported");
            goto exit;
          });
        break;
      case GRN_OP_JUMP :
        code += code->nargs + 1;
        break;
      case GRN_OP_CJUMP :
        {
          grn_obj *v;
          unsigned int v_boolean;
          POP1(v);
          truep(ctx, v, v_boolean);
          if (!v_boolean) { code += code->nargs; }
        }
        code++;
        break;
      case GRN_OP_GET_VALUE :
        {
          grn_obj *col, *rec;
          do {
            uint32_t size;
            const char *value;
            if (code->nargs == 1) {
              rec = v0;
              if (code->value) {
                col = code->value;
                ALLOC1(res);
              } else {
                POP1ALLOC1(col, res);
              }
            } else {
              if (code->value) {
                col = code->value;
                POP1ALLOC1(rec, res);
              } else {
                POP2ALLOC1(rec, col, res);
              }
            }
            if (col->header.type == GRN_BULK) {
              grn_obj *table = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(rec));
              col = grn_obj_column(ctx, table, GRN_BULK_HEAD(col), GRN_BULK_VSIZE(col));
              if (col) { GRN_PTR_PUT(ctx, &e->objs, col); }
            }
            if (col) {
              value = grn_obj_get_value_(ctx, col, GRN_RECORD_VALUE(rec), &size);
            } else {
              ERR(GRN_INVALID_ARGUMENT, "col resolve failed");
              goto exit;
            }
            if (size == GRN_OBJ_GET_VALUE_IMD) {
              GRN_RECORD_SET(ctx, res, (uintptr_t)value);
            } else {
              grn_bulk_write_from(ctx, res, value, 0, size);
            }
            res->header.domain = grn_obj_get_range(ctx, col);
            code++;
          } while (code < ce && code->op == GRN_OP_GET_VALUE);
        }
        break;
      case GRN_OP_OBJ_SEARCH :
        {
          grn_obj *op, *query, *index;
          // todo : grn_search_optarg optarg;
          POP1(op);
          op = GRN_OBJ_RESOLVE(ctx, op);
          POP1(res);
          res = GRN_OBJ_RESOLVE(ctx, res);
          POP1(query);
          query = GRN_OBJ_RESOLVE(ctx, query);
          POP1(index);
          index = GRN_OBJ_RESOLVE(ctx, index);
          grn_obj_search(ctx, index, query, res,
                         (grn_operator)GRN_UINT32_VALUE(op), NULL);
        }
        code++;
        break;
      case GRN_OP_TABLE_SELECT :
        {
          grn_obj *op, *res, *expr, *table;
          POP1(op);
          op = GRN_OBJ_RESOLVE(ctx, op);
          POP1(res);
          res = GRN_OBJ_RESOLVE(ctx, res);
          POP1(expr);
          expr = GRN_OBJ_RESOLVE(ctx, expr);
          POP1(table);
          table = GRN_OBJ_RESOLVE(ctx, table);
          WITH_SPSAVE({
            grn_table_select(ctx, table, expr, res, (grn_operator)GRN_UINT32_VALUE(op));
          });
          PUSH1(res);
        }
        code++;
        break;
      case GRN_OP_TABLE_SORT :
        {
          grn_obj *keys_, *res, *limit, *table;
          POP1(keys_);
          keys_ = GRN_OBJ_RESOLVE(ctx, keys_);
          POP1(res);
          res = GRN_OBJ_RESOLVE(ctx, res);
          POP1(limit);
          limit = GRN_OBJ_RESOLVE(ctx, limit);
          POP1(table);
          table = GRN_OBJ_RESOLVE(ctx, table);
          {
            grn_table_sort_key *keys;
            const char *p = GRN_BULK_HEAD(keys_), *tokbuf[256];
            int n = grn_str_tok(p, GRN_BULK_VSIZE(keys_), ' ', tokbuf, 256, NULL);
            if ((keys = GRN_MALLOCN(grn_table_sort_key, n))) {
              int i, n_keys = 0;
              for (i = 0; i < n; i++) {
                uint32_t len = (uint32_t) (tokbuf[i] - p);
                grn_obj *col = grn_obj_column(ctx, table, p, len);
                if (col) {
                  keys[n_keys].key = col;
                  keys[n_keys].flags = GRN_TABLE_SORT_ASC;
                  keys[n_keys].offset = 0;
                  n_keys++;
                } else {
                  if (p[0] == ':' && p[1] == 'd' && len == 2 && n_keys) {
                    keys[n_keys - 1].flags |= GRN_TABLE_SORT_DESC;
                  }
                }
                p = tokbuf[i] + 1;
              }
              WITH_SPSAVE({
                grn_table_sort(ctx, table, 0, GRN_INT32_VALUE(limit), res, keys, n_keys);
              });
              for (i = 0; i < n_keys; i++) {
                grn_obj_unlink(ctx, keys[i].key);
              }
              GRN_FREE(keys);
            }
          }
        }
        code++;
        break;
      case GRN_OP_TABLE_GROUP :
        {
          grn_obj *res, *keys_, *table;
          POP1(res);
          res = GRN_OBJ_RESOLVE(ctx, res);
          POP1(keys_);
          keys_ = GRN_OBJ_RESOLVE(ctx, keys_);
          POP1(table);
          table = GRN_OBJ_RESOLVE(ctx, table);
          {
            grn_table_sort_key *keys;
            grn_table_group_result results;
            const char *p = GRN_BULK_HEAD(keys_), *tokbuf[256];
            int n = grn_str_tok(p, GRN_BULK_VSIZE(keys_), ' ', tokbuf, 256, NULL);
            if ((keys = GRN_MALLOCN(grn_table_sort_key, n))) {
              int i, n_keys = 0;
              for (i = 0; i < n; i++) {
                uint32_t len = (uint32_t) (tokbuf[i] - p);
                grn_obj *col = grn_obj_column(ctx, table, p, len);
                if (col) {
                  keys[n_keys].key = col;
                  keys[n_keys].flags = GRN_TABLE_SORT_ASC;
                  keys[n_keys].offset = 0;
                  n_keys++;
                } else if (n_keys) {
                  if (p[0] == ':' && p[1] == 'd' && len == 2) {
                    keys[n_keys - 1].flags |= GRN_TABLE_SORT_DESC;
                  } else {
                    keys[n_keys - 1].offset = grn_atoi(p, p + len, NULL);
                  }
                }
                p = tokbuf[i] + 1;
              }
              /* todo : support multi-results */
              results.table = res;
              results.key_begin = 0;
              results.key_end = 0;
              results.limit = 0;
              results.flags = 0;
              results.op = GRN_OP_OR;
              WITH_SPSAVE({
                grn_table_group(ctx, table, keys, n_keys, &results, 1);
              });
              for (i = 0; i < n_keys; i++) {
                grn_obj_unlink(ctx, keys[i].key);
              }
              GRN_FREE(keys);
            }
          }
        }
        code++;
        break;
      case GRN_OP_JSON_PUT :
        {
          grn_obj_format format;
          grn_obj *str, *table, *res;
          POP1(res);
          res = GRN_OBJ_RESOLVE(ctx, res);
          POP1(str);
          str = GRN_OBJ_RESOLVE(ctx, str);
          POP1(table);
          table = GRN_OBJ_RESOLVE(ctx, table);
          GRN_OBJ_FORMAT_INIT(&format, grn_table_size(ctx, table), 0, -1);
          format.flags = 0;
          grn_obj_columns(ctx, table,
                          GRN_TEXT_VALUE(str), GRN_TEXT_LEN(str), &format.columns);
          grn_text_otoj(ctx, res, table, &format);
          GRN_OBJ_FORMAT_FIN(ctx, &format);
        }
        code++;
        break;
      case GRN_OP_AND :
        {
          grn_obj *x, *y;
          unsigned int x_boolean, y_boolean;
          POP2ALLOC1(x, y, res);
          truep(ctx, x, x_boolean);
          truep(ctx, y, y_boolean);
          if (x_boolean && y_boolean) {
            GRN_INT32_SET(ctx, res, 1);
          } else {
            GRN_INT32_SET(ctx, res, 0);
          }
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_OR :
        {
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          if (GRN_INT32_VALUE(x) == 0 && GRN_INT32_VALUE(y) == 0) {
            GRN_INT32_SET(ctx, res, 0);
          } else {
            GRN_INT32_SET(ctx, res, 1);
          }
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_BUT :
        {
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          if (GRN_INT32_VALUE(x) == 0 || GRN_INT32_VALUE(y) == 1) {
            GRN_INT32_SET(ctx, res, 0);
          } else {
            GRN_INT32_SET(ctx, res, 1);
          }
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_ADJUST :
        {
          /* todo */
        }
        code++;
        break;
      case GRN_OP_MATCH :
        {
          /* todo */
        }
        code++;
        break;
      case GRN_OP_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          do_eq(x, y, r);
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_NOT_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          do_eq(x, y, r);
          GRN_INT32_SET(ctx, res, 1 - r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_LESS :
        {
          int r;
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          do_compare(x, y, r, <);
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GREATER :
        {
          int r;
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          do_compare(x, y, r, >);
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_LESS_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          do_compare(x, y, r, <=);
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GREATER_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2ALLOC1(x, y, res);
          do_compare(x, y, r, >=);
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GEO_DISTANCE1 :
        {
          grn_obj *e;
          double lng1, lat1, lng2, lat2, x, y, d;
          POP1(e);
          lng1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1ALLOC1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
          y = (lat2 - lat1);
          d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_FLOAT;
          GRN_FLOAT_SET(ctx, res, d);
        }
        code++;
        break;
      case GRN_OP_GEO_DISTANCE2 :
        {
          grn_obj *e;
          double lng1, lat1, lng2, lat2, x, y, d;
          POP1(e);
          lng1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1ALLOC1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          x = sin(fabs(lng2 - lng1) * 0.5);
          y = sin(fabs(lat2 - lat1) * 0.5);
          d = asin(sqrt((y * y) + cos(lat1) * cos(lat2) * x * x)) * 2 * GEO_RADIOUS;
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_FLOAT;
          GRN_FLOAT_SET(ctx, res, d);
        }
        code++;
        break;
      case GRN_OP_GEO_DISTANCE3 :
        {
          grn_obj *e;
          double lng1, lat1, lng2, lat2, p, q, m, n, x, y, d;
          POP1(e);
          lng1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1ALLOC1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          p = (lat1 + lat2) * 0.5;
          q = (1 - GEO_BES_C3 * sin(p) * sin(p));
          m = GEO_BES_C1 / sqrt(q * q * q);
          n = GEO_BES_C2 / sqrt(q);
          x = n * cos(p) * fabs(lng1 - lng2);
          y = m * fabs(lat1 - lat2);
          d = sqrt((x * x) + (y * y));
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_FLOAT;
          GRN_FLOAT_SET(ctx, res, d);
        }
        code++;
        break;
      case GRN_OP_GEO_DISTANCE4 :
        {
          grn_obj *e;
          double lng1, lat1, lng2, lat2, p, q, m, n, x, y, d;
          POP1(e);
          lng1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1ALLOC1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          p = (lat1 + lat2) * 0.5;
          q = (1 - GEO_GRS_C3 * sin(p) * sin(p));
          m = GEO_GRS_C1 / sqrt(q * q * q);
          n = GEO_GRS_C2 / sqrt(q);
          x = n * cos(p) * fabs(lng1 - lng2);
          y = m * fabs(lat1 - lat2);
          d = sqrt((x * x) + (y * y));
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_FLOAT;
          GRN_FLOAT_SET(ctx, res, d);
        }
        code++;
        break;
      case GRN_OP_GEO_WITHINP5 :
        {
          int r;
          grn_obj *e;
          double lng0, lat0, lng1, lat1, x, y, d;
          POP1(e);
          lng0 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat0 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1ALLOC1(e, res);
          x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
          y = (lat1 - lat0);
          d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
          switch (e->header.domain) {
          case GRN_DB_INT32 :
            r = d <= GRN_INT32_VALUE(e);
            break;
          case GRN_DB_FLOAT :
            r = d <= GRN_FLOAT_VALUE(e);
            break;
          default :
            r = 0;
            break;
          }
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GEO_WITHINP6 :
        {
          int r;
          grn_obj *e;
          double lng0, lat0, lng1, lat1, lng2, lat2, x, y, d;
          POP1(e);
          lng0 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat0 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lat1 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1(e);
          lng2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          POP1ALLOC1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
          y = (lat1 - lat0);
          d = (x * x) + (y * y);
          x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
          y = (lat2 - lat1);
          r = d <= (x * x) + (y * y);
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GEO_WITHINP8 :
        {
          int r;
          grn_obj *e;
          int64_t ln0, la0, ln1, la1, ln2, la2, ln3, la3;
          POP1(e);
          ln0 = GRN_INT32_VALUE(e);
          POP1(e);
          la0 = GRN_INT32_VALUE(e);
          POP1(e);
          ln1 = GRN_INT32_VALUE(e);
          POP1(e);
          la1 = GRN_INT32_VALUE(e);
          POP1(e);
          ln2 = GRN_INT32_VALUE(e);
          POP1(e);
          la2 = GRN_INT32_VALUE(e);
          POP1(e);
          ln3 = GRN_INT32_VALUE(e);
          POP1ALLOC1(e, res);
          la3 = GRN_INT32_VALUE(e);
          r = ((ln2 <= ln0) && (ln0 <= ln3) && (la2 <= la0) && (la0 <= la3));
          GRN_INT32_SET(ctx, res, r);
          res->header.type = GRN_BULK;
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_PLUS :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_PLUS,
          INTEGER_ARITHMETIC_OPERATION_PLUS,
          FLOAT_ARITHMETIC_OPERATION_PLUS,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            GRN_BULK_REWIND(res);
            grn_obj_cast(ctx, x, res, GRN_FALSE);
            grn_obj_cast(ctx, y, res, GRN_FALSE);
          }
          ,);
        break;
      case GRN_OP_MINUS :
        if (code->nargs == 1) {
          ARITHMETIC_UNARY_OPERATION_DISPATCH(
            INTEGER_UNARY_ARITHMETIC_OPERATION_MINUS,
            FLOAT_UNARY_ARITHMETIC_OPERATION_MINUS,
            ARITHMETIC_OPERATION_NO_CHECK,
            ARITHMETIC_OPERATION_NO_CHECK,
            {
              long long int x_;

              res->header.type = GRN_BULK;
              res->header.domain = GRN_DB_INT64;

              GRN_INT64_SET(ctx, res, 0);
              grn_obj_cast(ctx, x, res, GRN_FALSE);
              x_ = GRN_INT64_VALUE(res);

              GRN_INT64_SET(ctx, res, -x_);
            }
            ,);
        } else {
          ARITHMETIC_BINARY_OPERATION_DISPATCH(
            INTEGER_ARITHMETIC_OPERATION_MINUS,
            INTEGER_ARITHMETIC_OPERATION_MINUS,
            FLOAT_ARITHMETIC_OPERATION_MINUS,
            ARITHMETIC_OPERATION_NO_CHECK,
            ARITHMETIC_OPERATION_NO_CHECK,
            {
              ERR(GRN_INVALID_ARGUMENT,
                  "\"string\" - \"string\" "
                  "isn't supported");
              goto exit;
            }
            ,);
        }
        break;
      case GRN_OP_STAR :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_STAR,
          INTEGER_ARITHMETIC_OPERATION_STAR,
          FLOAT_ARITHMETIC_OPERATION_STAR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT,
                "\"string\" * \"string\" "
                "isn't supported");
            goto exit;
          }
          ,);
        break;
      case GRN_OP_SLASH :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_SLASH,
          INTEGER_ARITHMETIC_OPERATION_SLASH,
          FLOAT_ARITHMETIC_OPERATION_SLASH,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_ZERO_DIVISION_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT,
                "\"string\" / \"string\" "
                "isn't supported");
            goto exit;
          }
          ,);
        break;
      case GRN_OP_MOD :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_MOD,
          INTEGER_ARITHMETIC_OPERATION_MOD,
          FLOAT_ARITHMETIC_OPERATION_MOD,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_ZERO_DIVISION_CHECK,
          {
            ERR(GRN_INVALID_ARGUMENT,
                "\"string\" % \"string\" "
                "isn't supported");
            goto exit;
          }
          ,);
        break;
      case GRN_OP_BITWISE_NOT :
        ARITHMETIC_UNARY_OPERATION_DISPATCH(
          INTEGER_UNARY_ARITHMETIC_OPERATION_BITWISE_NOT,
          FLOAT_UNARY_ARITHMETIC_OPERATION_BITWISE_NOT,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          TEXT_UNARY_ARITHMETIC_OPERATION(~),);
        break;
      case GRN_OP_BITWISE_OR :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_BITWISE_OR,
          INTEGER_ARITHMETIC_OPERATION_BITWISE_OR,
          FLOAT_ARITHMETIC_OPERATION_BITWISE_OR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          TEXT_ARITHMETIC_OPERATION(|),);
        break;
      case GRN_OP_BITWISE_XOR :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_BITWISE_XOR,
          INTEGER_ARITHMETIC_OPERATION_BITWISE_XOR,
          FLOAT_ARITHMETIC_OPERATION_BITWISE_XOR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          TEXT_ARITHMETIC_OPERATION(^),);
        break;
      case GRN_OP_BITWISE_AND :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_BITWISE_AND,
          INTEGER_ARITHMETIC_OPERATION_BITWISE_AND,
          FLOAT_ARITHMETIC_OPERATION_BITWISE_AND,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          TEXT_ARITHMETIC_OPERATION(&),);
        break;
      case GRN_OP_SHIFTL :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_SHIFTL,
          INTEGER_ARITHMETIC_OPERATION_SHIFTL,
          FLOAT_ARITHMETIC_OPERATION_SHIFTL,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          TEXT_ARITHMETIC_OPERATION(<<),);
        break;
      case GRN_OP_SHIFTR :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER_ARITHMETIC_OPERATION_SHIFTR,
          INTEGER_ARITHMETIC_OPERATION_SHIFTR,
          FLOAT_ARITHMETIC_OPERATION_SHIFTR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          TEXT_ARITHMETIC_OPERATION(>>),);
        break;
      case GRN_OP_SHIFTRR :
        ARITHMETIC_BINARY_OPERATION_DISPATCH(
          INTEGER32_ARITHMETIC_OPERATION_SHIFTRR,
          INTEGER64_ARITHMETIC_OPERATION_SHIFTRR,
          FLOAT_ARITHMETIC_OPERATION_SHIFTRR,
          ARITHMETIC_OPERATION_NO_CHECK,
          ARITHMETIC_OPERATION_NO_CHECK,
          {
            long long unsigned int x_;
            long long unsigned int y_;

            res->header.type = GRN_BULK;
            res->header.domain = GRN_DB_INT64;

            GRN_INT64_SET(ctx, res, 0);
            grn_obj_cast(ctx, x, res, GRN_FALSE);
            x_ = GRN_INT64_VALUE(res);

            GRN_INT64_SET(ctx, res, 0);
            grn_obj_cast(ctx, y, res, GRN_FALSE);
            y_ = GRN_INT64_VALUE(res);

            GRN_INT64_SET(ctx, res, x_ >> y_);
          }
          ,);
        break;
      case GRN_OP_INCR :
        UNARY_OPERATE_AND_ASSIGN_DISPATCH(EXEC_OPERATE, 1, GRN_OBJ_INCR);
        break;
      case GRN_OP_DECR :
        UNARY_OPERATE_AND_ASSIGN_DISPATCH(EXEC_OPERATE, 1, GRN_OBJ_DECR);
        break;
      case GRN_OP_INCR_POST :
        UNARY_OPERATE_AND_ASSIGN_DISPATCH(EXEC_OPERATE_POST, 1, GRN_OBJ_INCR);
        break;
      case GRN_OP_DECR_POST :
        UNARY_OPERATE_AND_ASSIGN_DISPATCH(EXEC_OPERATE_POST, 1, GRN_OBJ_DECR);
        break;
      default :
        break;
      }
    }
    ctx->impl->stack_curr = sp - s_;
  }
  if (ctx->impl->stack_curr + nargs != stack_curr + 1) {
    /*
    GRN_LOG(ctx, GRN_LOG_WARNING, "nargs=%d stack balance=%d",
            nargs, stack_curr - ctx->impl->stack_curr);
    */
    ctx->impl->stack_curr = stack_curr + 1 - nargs;
  }
exit :
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_expr_get_value(grn_ctx *ctx, grn_obj *expr, int offset)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (0 <= offset && offset < e->values_size) {
    res = &e->values[offset];
  }
  GRN_API_RETURN(res);
}

inline static void
res_add(grn_ctx *ctx, grn_hash *s, grn_rset_posinfo *pi, uint32_t score,
        grn_operator op)
{
  grn_rset_recinfo *ri;
  switch (op) {
  case GRN_OP_OR :
    if (grn_hash_add(ctx, s, pi, s->key_size, (void **)&ri, NULL)) {
      grn_table_add_subrec((grn_obj *)s, ri, score, pi, 1);
    }
    break;
  case GRN_OP_AND :
    if (grn_hash_get(ctx, s, pi, s->key_size, (void **)&ri)) {
      ri->n_subrecs |= GRN_RSET_UTIL_BIT;
      grn_table_add_subrec((grn_obj *)s, ri, score, pi, 1);
    }
    break;
  case GRN_OP_BUT :
    {
      grn_id id;
      if ((id = grn_hash_get(ctx, s, pi, s->key_size, (void **)&ri))) {
        grn_hash_delete_by_id(ctx, s, id, NULL);
      }
    }
    break;
  case GRN_OP_ADJUST :
    if (grn_hash_get(ctx, s, pi, s->key_size, (void **)&ri)) {
      ri->score += score;
    }
    break;
  default :
    break;
  }
}

#define SCAN_ACCESSOR                  (0x01)
#define SCAN_PUSH                      (0x02)
#define SCAN_POP                       (0x04)
#define SCAN_PRE_CONST                 (0x08)

typedef struct {
  uint32_t start;
  uint32_t end;
  int32_t nargs;
  int flags;
  grn_operator op;
  grn_operator logical_op;
  grn_obj wv;
  grn_obj index;
  grn_obj *query;
  grn_obj *args[8];
} scan_info;

typedef enum {
  SCAN_START = 0,
  SCAN_VAR,
  SCAN_COL1,
  SCAN_COL2,
  SCAN_CONST
} scan_stat;

#define SI_FREE(si) {\
  GRN_OBJ_FIN(ctx, &(si)->wv);\
  GRN_OBJ_FIN(ctx, &(si)->index);\
  GRN_FREE(si);\
}

#define SI_ALLOC(si, i, st) {\
  if (!((si) = GRN_MALLOCN(scan_info, 1))) {\
    int j;\
    for (j = 0; j < i; j++) { SI_FREE(sis[j]); }\
    GRN_FREE(sis);\
    return NULL;\
  }\
  GRN_INT32_INIT(&(si)->wv, GRN_OBJ_VECTOR);\
  GRN_PTR_INIT(&(si)->index, GRN_OBJ_VECTOR, GRN_ID_NIL);\
  (si)->logical_op = GRN_OP_OR;\
  (si)->flags = SCAN_PUSH;\
  (si)->nargs = 0;\
  (si)->start = (st);\
}

static scan_info **
put_logical_op(grn_ctx *ctx, scan_info **sis, int *ip, grn_operator op, int start)
{
  int nparens = 1, ndifops = 0, i = *ip, j = i, r = 0;
  grn_operator op_ = op == GRN_OP_BUT ? GRN_OP_AND : op;
  while (j--) {
    scan_info *s_ = sis[j];
    if (s_->flags & SCAN_POP) {
      ndifops++;
      nparens++;
    } else {
      if (s_->flags & SCAN_PUSH) {
        if (!(--nparens)) {
          if (!r) {
            if (ndifops) {
              if (j) {
                nparens = 1;
                ndifops = 0;
                r = j;
              } else {
                SI_ALLOC(s_, i, start);
                s_->flags = SCAN_POP;
                s_->logical_op = op;
                sis[i++] = s_;
                *ip = i;
                break;
              }
            } else {
              s_->flags &= ~SCAN_PUSH;
              s_->logical_op = op;
              break;
            }
          } else {
            if (ndifops) {
              SI_ALLOC(s_, i, start);
              s_->flags = SCAN_POP;
              s_->logical_op = op;
              sis[i++] = s_;
              *ip = i;
            } else {
              s_->flags &= ~SCAN_PUSH;
              s_->logical_op = op;
              memcpy(&sis[i], &sis[j], sizeof(scan_info *) * (r - j));
              memcpy(&sis[j], &sis[r], sizeof(scan_info *) * (i - r));
              memcpy(&sis[i + j - r], &sis[i], sizeof(scan_info *) * (r - j));
            }
            break;
          }
        }
      } else {
        if (op_ != (s_->logical_op == GRN_OP_BUT ? GRN_OP_AND : s_->logical_op)) {
          ndifops++;
        }
      }
    }
  }
  if (j < 0) {
    ERR(GRN_INVALID_ARGUMENT, "unmatched nesting level");
    for (j = 0; j < i; j++) { SI_FREE(sis[j]); }
    GRN_FREE(sis);
    return NULL;
  }
  return sis;
}


#define EXPRLOG(name,expr) {\
  grn_obj strbuf;\
  GRN_TEXT_INIT(&strbuf, 0);\
  grn_expr_inspect(ctx, &strbuf, (expr));\
  GRN_TEXT_PUTC(ctx, &strbuf, '\0');\
  GRN_LOG(ctx, GRN_LOG_NOTICE, "%s=(%s)", (name), GRN_TEXT_VALUE(&strbuf));\
  GRN_OBJ_FIN(ctx, &strbuf);\
}

static void
scan_info_put_index(grn_ctx *ctx, scan_info *si, grn_obj *index, uint32_t sid, int32_t weight)
{
  GRN_PTR_PUT(ctx, &si->index, index);
  GRN_UINT32_PUT(ctx, &si->wv, sid);
  GRN_INT32_PUT(ctx, &si->wv, weight);
  {
    int i, ni = (GRN_BULK_VSIZE(&si->index) / sizeof(grn_obj *)) - 1;
    grn_obj **pi = &GRN_PTR_VALUE_AT(&si->index, ni);
    for (i = 0; i < ni; i++, pi--) {
      if (index == pi[-1]) {
        if (i) {
          int32_t *pw = &GRN_INT32_VALUE_AT(&si->wv, (ni - i) * 2);
          memmove(pw + 2, pw, sizeof(int32_t) * 2 * i);
          pw[0] = (int32_t) sid;
          pw[1] = weight;
          memmove(pi + 1, pi, sizeof(grn_obj *) * i);
          pi[0] = index;
        }
        return;
      }
    }
  }
}

static int32_t
get_weight(grn_expr_code *ec)
{
  return (ec->modify == 2 && ec[2].op == GRN_OP_STAR &&
          ec[1].value && ec[1].value->header.type == GRN_BULK &&
          ec[1].value->header.domain == GRN_DB_UINT32) ? GRN_INT32_VALUE(ec[1].value) : 1;
}

static scan_info **
scan_info_build(grn_ctx *ctx, grn_obj *expr, int *n,
                grn_operator op, uint32_t size)
{
  grn_obj *var;
  scan_stat stat;
  int i, m = 0, o = 0;
  scan_info **sis, *si = NULL;
  grn_expr_code *c, *ce;
  grn_expr *e = (grn_expr *)expr;  if (!e->nvars || !(var = &e->vars[0].value)) { return NULL; }
  for (stat = SCAN_START, c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    switch (c->op) {
    case GRN_OP_MATCH :
    case GRN_OP_EQUAL :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_GEO_WITHINP5 :
    case GRN_OP_GEO_WITHINP6 :
    case GRN_OP_GEO_WITHINP8 :
      if (stat < SCAN_COL1 || SCAN_CONST < stat) { return NULL; }
      stat = SCAN_START;
      m++;
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_BUT :
    case GRN_OP_ADJUST :
      if (stat != SCAN_START) { return NULL; }
      o++;
      if (o >= m) { return NULL; }
      break;
    case GRN_OP_PUSH :
      stat = (c->value == var) ? SCAN_VAR : SCAN_CONST;
      break;
    case GRN_OP_GET_VALUE :
      switch (stat) {
      case SCAN_START :
      case SCAN_CONST :
      case SCAN_VAR :
        stat = SCAN_COL1;
        break;
      case SCAN_COL1 :
        stat = SCAN_COL2;
        break;
      case SCAN_COL2 :
        break;
      default :
        return NULL;
        break;
      }
      break;
    case GRN_OP_CALL :
      if (c->flags & GRN_EXPR_CODE_RELATIONAL_EXPRESSION) {
        stat = SCAN_START;
        m++;
      } else {
        stat = SCAN_COL2;
      }
      break;
    default :
      return NULL;
      break;
    }
  }
  if (stat || m != o + 1) { return NULL; }
  if (!(sis = GRN_MALLOCN(scan_info *, m + m + o))) { return NULL; }
  for (i = 0, stat = SCAN_START, c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    switch (c->op) {
    case GRN_OP_MATCH :
    case GRN_OP_EQUAL :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_GEO_WITHINP5 :
    case GRN_OP_GEO_WITHINP6 :
    case GRN_OP_GEO_WITHINP8 :
      stat = SCAN_START;
      si->op = c->op;
      si->end = c - e->codes;
      sis[i++] = si;
      {
        uint32_t sid;
        grn_obj *index, **p = si->args, **pe = si->args + si->nargs;
        for (; p < pe; p++) {
          if ((*p)->header.type == GRN_EXPR) {
            uint32_t j;
            grn_expr_code *ec;
            grn_expr *e = (grn_expr *)(*p);
            for (j = e->codes_curr, ec = e->codes; j--; ec++) {
              if (ec->value) {
                switch (ec->value->header.type) {
                case GRN_ACCESSOR :
                case GRN_ACCESSOR_VIEW :
                  if (grn_column_index(ctx, ec->value, c->op, &index, 1, &sid)) {
                    si->flags |= SCAN_ACCESSOR;
                    scan_info_put_index(ctx, si, index, sid, get_weight(ec));
                  }
                  break;
                case GRN_COLUMN_FIX_SIZE :
                case GRN_COLUMN_VAR_SIZE :
                  if (grn_column_index(ctx, ec->value, c->op, &index, 1, &sid)) {
                    scan_info_put_index(ctx, si, index, sid, get_weight(ec));
                  }
                  break;
                case GRN_COLUMN_INDEX :
                  scan_info_put_index(ctx, si, ec->value, 0, get_weight(ec));
                  break;
                }
              }
            }
          } else if (GRN_DB_OBJP(*p)) {
            if (grn_column_index(ctx, *p, c->op, &index, 1, &sid)) {
              scan_info_put_index(ctx, si, index, sid, 1);
            }
          } else if (ACCESSORP(*p)) {
            si->flags |= SCAN_ACCESSOR;
            if (grn_column_index(ctx, *p, c->op, &index, 1, &sid)) {
              scan_info_put_index(ctx, si, index, sid, 1);
            }
          } else {
            si->query = *p;
          }
        }
      }
      si = NULL;
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_BUT :
    case GRN_OP_ADJUST :
      if (!put_logical_op(ctx, sis, &i, c->op, c - e->codes)) { return NULL; }
      stat = SCAN_START;
      break;
    case GRN_OP_PUSH :
      if (!si) { SI_ALLOC(si, i, c - e->codes); }
      if (c->value == var) {
        stat = SCAN_VAR;
      } else {
        if (si->nargs < 8) {
          si->args[si->nargs++] = c->value;
        }
        if (stat == SCAN_START) { si->flags |= SCAN_PRE_CONST; }
        stat = SCAN_CONST;
      }
      break;
    case GRN_OP_GET_VALUE :
      switch (stat) {
      case SCAN_START :
        if (!si) { SI_ALLOC(si, i, c - e->codes); }
        // fallthru
      case SCAN_CONST :
      case SCAN_VAR :
        stat = SCAN_COL1;
        if (si->nargs < 8) {
          si->args[si->nargs++] = c->value;
        }
        break;
      case SCAN_COL1 :
        si->args[si->nargs - 1] = NULL;
        stat = SCAN_COL2;
        break;
      case SCAN_COL2 :
        break;
      default :
        break;
      }
      break;
    case GRN_OP_CALL :
      if (!si) { SI_ALLOC(si, i, c - e->codes); }
      if (c->flags & GRN_EXPR_CODE_RELATIONAL_EXPRESSION) {
        stat = SCAN_START;
        si->op = c->op;
        si->end = c - e->codes;
        sis[i++] = si;
        /* index may be applicable occasionaly
        {
          grn_obj **p = si->args, **pe = si->args + si->nargs;
          for (; p < pe; p++) {
            if (GRN_DB_OBJP(*p)) {
              grn_column_index(ctx, *p, c->op, &si->index, 1, &sid);
            } else if (ACCESSORP(*p)) {
              si->flags |= SCAN_ACCESSOR;
              grn_column_index(ctx, *p, c->op, &si->index, 1, &sid);
            } else {
              si->query = *p;
            }
          }
        }
        */
        si = NULL;
      } else {
        stat = SCAN_COL2;
      }
      break;
    default :
      break;
    }
  }
  if (op == GRN_OP_OR && !size) {
    // for debug
    if (!(sis[0]->flags & SCAN_PUSH) || (sis[0]->logical_op != op)) {
      int j;
      ERR(GRN_INVALID_ARGUMENT, "invalid expr");
      for (j = 0; j < i; j++) { SI_FREE(sis[j]); }
      GRN_FREE(sis);
      return NULL;
    } else {
      sis[0]->flags &= ~SCAN_PUSH;
      sis[0]->logical_op = op;
    }
  } else {
    if (!put_logical_op(ctx, sis, &i, op, c - e->codes)) { return NULL; }
  }
  *n = i;
  return sis;
}

static void
grn_table_select_(grn_ctx *ctx, grn_obj *table, grn_obj *expr, grn_obj *v,
                  grn_obj *res, grn_operator op)
{
  int32_t score;
  grn_id id, *idp;
  grn_table_cursor *tc;
  grn_hash_cursor *hc;
  grn_hash *s = (grn_hash *)res;
  grn_obj *r;
  GRN_RECORD_INIT(v, 0, grn_obj_id(ctx, table));
  switch (op) {
  case GRN_OP_OR :
    if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
      while ((id = grn_table_cursor_next(ctx, tc))) {
        GRN_RECORD_SET(ctx, v, id);
        grn_expr_exec(ctx, expr, 0);
        r = grn_ctx_pop(ctx);
        if (r && (score = GRN_UINT32_VALUE(r))) {
          grn_rset_recinfo *ri;
          if (grn_hash_add(ctx, s, &id, s->key_size, (void **)&ri, NULL)) {
            grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)&id, 1);
          }
        }
      }
      grn_table_cursor_close(ctx, tc);
    }
    break;
  case GRN_OP_AND :
    if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0))) {
      while (grn_hash_cursor_next(ctx, hc)) {
        grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
        GRN_RECORD_SET(ctx, v, *idp);
        grn_expr_exec(ctx, expr, 0);
        r = grn_ctx_pop(ctx);
        if (r && (score = GRN_UINT32_VALUE(r))) {
          grn_rset_recinfo *ri;
          grn_hash_cursor_get_value(ctx, hc, (void **) &ri);
          grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)idp, 1);
        } else {
          grn_hash_cursor_delete(ctx, hc, NULL);
        }
      }
      grn_hash_cursor_close(ctx, hc);
    }
    break;
  case GRN_OP_BUT :
    if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0))) {
      while (grn_hash_cursor_next(ctx, hc)) {
        grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
        GRN_RECORD_SET(ctx, v, *idp);
        grn_expr_exec(ctx, expr, 0);
        r = grn_ctx_pop(ctx);
        if (r && (score = GRN_UINT32_VALUE(r))) {
          grn_hash_cursor_delete(ctx, hc, NULL);
        }
      }
      grn_hash_cursor_close(ctx, hc);
    }
    break;
  case GRN_OP_ADJUST :
    if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0))) {
      while (grn_hash_cursor_next(ctx, hc)) {
        grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
        GRN_RECORD_SET(ctx, v, *idp);
        grn_expr_exec(ctx, expr, 0);
        r = grn_ctx_pop(ctx);
        if (r && (score = GRN_UINT32_VALUE(r))) {
          grn_rset_recinfo *ri;
          grn_hash_cursor_get_value(ctx, hc, (void **) &ri);
          grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)idp, 1);
        }
      }
      grn_hash_cursor_close(ctx, hc);
    }
    break;
  default :
    break;
  }
}

grn_obj *
grn_view_select(grn_ctx *ctx, grn_obj *table, grn_obj *expr,
                grn_obj *res, grn_operator op)
{
  if (res) {
    if (res->header.type != GRN_TABLE_VIEW ||
        (res->header.domain != DB_OBJ(table)->id)) {
      ERR(GRN_INVALID_ARGUMENT, "view table required");
      return NULL;
    }
  } else {
    if (!(res = grn_table_create(ctx, NULL, 0, NULL,
                                 GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL))) {
      return NULL;
    }
  }
  {
    grn_obj *t, *r;
    grn_id *tp, rid;
    grn_view *tv = (grn_view *)table;
    grn_view *rv = (grn_view *)res;
    grn_hash *th = tv->hash;
    grn_hash *rh = rv->hash;
    grn_expr *e = (grn_expr *)expr;
    grn_expr_code *cs, *cd, *c0 = e->codes, *ce = e->codes + e->codes_curr;
    if ((e->codes = GRN_MALLOCN(grn_expr_code, e->codes_curr))) {
      memcpy(e->codes, c0, sizeof(grn_expr_code) * e->codes_curr);
      GRN_HASH_EACH(ctx, th, id, &tp, NULL, NULL, {
        grn_hash_get_key(ctx, rh, id, &rid, sizeof(grn_id));
        t = grn_ctx_at(ctx, *tp);
        r = grn_ctx_at(ctx, rid);
        for (cs = c0, cd = e->codes; cs < ce; cs++, cd++) {
          if (cs->value && cs->value->header.type == GRN_ACCESSOR_VIEW) {
            grn_accessor_view *a = (grn_accessor_view *)cs->value;
            if (!(cd->value = a->accessors[id - 1])) {
              cd->value = grn_null;
            }
          }
        }
        grn_table_select(ctx, t, expr, r, op);
      });

      GRN_FREE(e->codes);
    }
    e->codes = c0;
  }
  return res;
}

grn_obj *
grn_table_select(grn_ctx *ctx, grn_obj *table, grn_obj *expr,
                 grn_obj *res, grn_operator op)
{
  grn_obj *v;
  unsigned int res_size;
  if (table->header.type == GRN_TABLE_VIEW) {
    return grn_view_select(ctx, table, expr, res, op);
  }
  if (res) {
    if (res->header.type != GRN_TABLE_HASH_KEY ||
        (res->header.domain != DB_OBJ(table)->id)) {
      ERR(GRN_INVALID_ARGUMENT, "hash table required");
      return NULL;
    }
  } else {
    if (!(res = grn_table_create(ctx, NULL, 0, NULL,
                                 GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL))) {
      return NULL;
    }
  }
  if (!(v = grn_expr_get_var_by_offset(ctx, expr, 0))) {
    ERR(GRN_INVALID_ARGUMENT, "at least one variable must be defined");
    return NULL;
  }
  GRN_API_ENTER;
  res_size = GRN_HASH_SIZE((grn_hash *)res);
  if (op == GRN_OP_OR || res_size) {
    int i, n;
    scan_info **sis;
    if ((sis = scan_info_build(ctx, expr, &n, op, res_size))) {
      grn_obj res_stack;
      grn_expr *e = (grn_expr *)expr;
      grn_expr_code *codes = e->codes;
      uint32_t codes_curr = e->codes_curr;
      GRN_PTR_INIT(&res_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
      for (i = 0; i < n; i++) {
        int done = 0;
        scan_info *si = sis[i];
        if (si->flags & SCAN_POP) {
          grn_obj *res_;
          GRN_PTR_POP(&res_stack, res_);
          grn_table_setoperation(ctx, res_, res, res_, si->logical_op);
          grn_obj_close(ctx, res);
          res = res_;
        } else {
          if (si->flags & SCAN_PUSH) {
            grn_obj *res_;
            res_ = grn_table_create(ctx, NULL, 0, NULL,
                                    GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL);
            if (res_) {
              GRN_PTR_PUT(ctx, &res_stack, res);
              res = res_;
            }
          }
          if (GRN_BULK_VSIZE(&si->index)) {
            grn_obj *index = GRN_PTR_VALUE(&si->index);
            switch (si->op) {
            case GRN_OP_EQUAL :
              if (si->flags & SCAN_ACCESSOR) {
                if (index->header.type == GRN_ACCESSOR &&
                    !((grn_accessor *)index)->next) {
                  grn_obj dest;
                  grn_accessor *a = (grn_accessor *)index;
                  grn_rset_posinfo pi;
                  switch (a->action) {
                  case GRN_ACCESSOR_GET_ID :
                    GRN_UINT32_INIT(&dest, 0);
                    if (!grn_obj_cast(ctx, si->query, &dest, 0)) {
                      memcpy(&pi, GRN_BULK_HEAD(&dest), GRN_BULK_VSIZE(&dest));
                      if (pi.rid) {
                        res_add(ctx, (grn_hash *)res, &pi, 1, si->logical_op);
                      }
                      done++;
                    }
                    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, si->logical_op);
                    GRN_OBJ_FIN(ctx, &dest);
                    break;
                  case GRN_ACCESSOR_GET_KEY :
                    GRN_OBJ_INIT(&dest, GRN_BULK, 0, table->header.domain);
                    if (!grn_obj_cast(ctx, si->query, &dest, 0)) {
                      if ((pi.rid = grn_table_get(ctx, table,
                                                  GRN_BULK_HEAD(&dest),
                                                  GRN_BULK_VSIZE(&dest)))) {
                        res_add(ctx, (grn_hash *)res, &pi, 1, si->logical_op);
                      }
                      done++;
                    }
                    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, si->logical_op);
                    GRN_OBJ_FIN(ctx, &dest);
                    break;
                  }
                }
              } else {
                /* table ?? si->index.domain ? */
                grn_id tid = grn_table_get(ctx, table,
                                           GRN_BULK_HEAD(si->query),
                                           GRN_BULK_VSIZE(si->query));
                grn_ii_at(ctx, (grn_ii *)index, tid, (grn_hash *)res, si->logical_op);
                grn_ii_resolve_sel_and(ctx, (grn_hash *)res, si->logical_op);
                done++;
              }
              break;
            case GRN_OP_MATCH :
              {
                grn_obj wv, **ip = &GRN_PTR_VALUE(&si->index);
                int j = GRN_BULK_VSIZE(&si->index)/sizeof(grn_obj *);
                int32_t *wp = &GRN_INT32_VALUE(&si->wv);
                grn_search_optarg optarg;
                optarg.mode = GRN_OP_EXACT;
                optarg.similarity_threshold = 0;
                optarg.max_interval = 0;
                optarg.weight_vector = (int *)GRN_BULK_HEAD(&wv);
                /* optarg.vector_size = GRN_BULK_VSIZE(&si->wv); */
                optarg.vector_size = 1;
                optarg.proc = NULL;
                optarg.max_size = 0;
                ctx->flags |= GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND;
                GRN_INT32_INIT(&wv, GRN_OBJ_VECTOR);
                for (; j--; ip++, wp += 2) {
                  uint32_t sid = (uint32_t) wp[0];
                  int32_t weight = wp[1];
                  if (sid) {
                    GRN_INT32_SET_AT(ctx, &wv, sid - 1, weight);
                    optarg.weight_vector = &GRN_INT32_VALUE(&wv);
                    optarg.vector_size = GRN_BULK_VSIZE(&wv)/sizeof(int32_t);
                  } else {
                    optarg.weight_vector = NULL;
                    optarg.vector_size = weight;
                  }
                  if (j) {
                    if (sid && ip[0] == ip[1]) { continue; }
                  } else {
                    ctx->flags &= ~GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND;
                  }
                  grn_obj_search(ctx, ip[0], si->query, res, si->logical_op, &optarg);
                  GRN_BULK_REWIND(&wv);
                }
                GRN_OBJ_FIN(ctx, &wv);
              }
              done++;
              break;
            default :
              /* todo : implement */
              /* todo : handle SCAN_PRE_CONST */
              break;
            }
          }
          if (!done) {
            e->codes = codes + si->start;
            e->codes_curr = si->end - si->start + 1;
            grn_table_select_(ctx, table, expr, v, res, si->logical_op);
          }
        }
        SI_FREE(si);
      }
      GRN_OBJ_FIN(ctx, &res_stack);
      GRN_FREE(sis);
      e->codes = codes;
      e->codes_curr = codes_curr;
    } else {
      if (!ctx->rc) {
        grn_table_select_(ctx, table, expr, v, res, op);
      }
    }
  }
  GRN_API_RETURN(res);
}

/* todo : refine */
static int
tokenize(const char *str, size_t str_len, const char **tokbuf, int buf_size, const char **rest)
{
  const char **tok = tokbuf, **tok_end = tokbuf + buf_size;
  if (buf_size > 0) {
    const char *str_end = str + str_len;
    for (;;str++) {
      if (str == str_end) {
        *tok++ = str;
        break;
      }
      if (' ' == *str || ',' == *str) {
        // *str = '\0';
        *tok++ = str;
        if (tok == tok_end) { break; }
      }
    }
  }
  if (rest) { *rest = str; }
  return tok - tokbuf;
}

// todo : support view
grn_rc
grn_obj_columns(grn_ctx *ctx, grn_obj *table,
                const char *str, unsigned str_size, grn_obj *res)
{
  grn_obj *col;
  const char *p = (char *)str, *q, *r, *pe = p + str_size, *tokbuf[256];
  while (p < pe) {
    int i, n = tokenize(p, pe - p, tokbuf, 256, &q);
    for (i = 0; i < n; i++) {
      r = tokbuf[i];
      if (p < r) {
        if (r[-1] == '*') {
          grn_hash *cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                           GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY);
          if (cols) {
            grn_id *key;
            grn_table_columns(ctx, table, p, r - p - 1, (grn_obj *)cols);
            GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
              if ((col = grn_ctx_at(ctx, *key))) { GRN_PTR_PUT(ctx, res, col); }
            });
            grn_hash_close(ctx, cols);
          }
          {
            grn_obj *type = grn_ctx_at(ctx, table->header.domain);
            if (GRN_OBJ_TABLEP(type)) {
              grn_obj *ai = grn_obj_column(ctx, table, "_id", 3);
              if (ai) {
                if (ai->header.type == GRN_ACCESSOR) {
                  cols = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                         GRN_OBJ_TABLE_HASH_KEY|GRN_HASH_TINY);
                  if (cols) {
                    grn_id *key;
                    grn_accessor *a, *ac;
                    for (a = (grn_accessor *)ai; a; a = a->next) { table = a->obj; }
                    grn_table_columns(ctx, table, p, r - p - 1, (grn_obj *)cols);
                    GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
                      if ((col = grn_ctx_at(ctx, *key))) {
                        ac = accessor_new(ctx);
                        GRN_PTR_PUT(ctx, res, (grn_obj *)ac);
                        for (a = (grn_accessor *)ai; a; a = a->next) {
                          if (a->action != GRN_ACCESSOR_GET_ID) {
                            ac->action = a->action;
                            ac->obj = a->obj;
                            ac->next = accessor_new(ctx);
                            if (!(ac = ac->next)) { break; }
                          } else {
                            ac->action = GRN_ACCESSOR_GET_COLUMN_VALUE;
                            ac->obj = col;
                            ac->next = NULL;
                            break;
                          }
                        }
                      }
                    });
                    grn_hash_close(ctx, cols);
                  }
                }
                grn_obj_unlink(ctx, ai);
              }
            }
          }
        } else if ((col = grn_obj_column(ctx, table, p, r - p))) {
          GRN_PTR_PUT(ctx, res, col);
        }
      }
      p = r + 1;
    }
    p = q;
  }
  return ctx->rc;
}

/* grn_expr_create_from_str */

#include "snip.h"

#define DEFAULT_WEIGHT 5
#define DEFAULT_DECAYSTEP 2
#define DEFAULT_MAX_INTERVAL 10
#define DEFAULT_SIMILARITY_THRESHOLD 10
#define DEFAULT_TERM_EXTRACT_POLICY 0
#define DEFAULT_WEIGHT_VECTOR_SIZE 4096

typedef struct {
  grn_ctx *ctx;
  grn_obj *e;
  grn_obj *v;
  const char *str;
  const char *cur;
  const char *str_end;
  grn_obj *table;
  grn_obj *default_column;
  grn_obj buf;
  grn_obj token_stack;
  grn_obj column_stack;
  grn_obj op_stack;
  grn_obj mode_stack;
  grn_operator default_op;
  grn_select_optarg opt;
  grn_operator default_mode;
  grn_expr_flags flags;
  grn_expr_flags default_flags;
  int escalation_threshold;
  int escalation_decaystep;
  int weight_offset;
  grn_hash *weight_set;
  snip_cond *snip_conds;
} efs_info;

typedef struct {
  grn_operator op;
  int weight;
} efs_op;

inline static void
skip_space(grn_ctx *ctx, efs_info *q)
{
  unsigned int len;
  while (q->cur < q->str_end && grn_isspace(q->cur, ctx->encoding)) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, q->cur, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    q->cur += len;
  }
}

static grn_rc get_expr(grn_ctx *ctx, efs_info *q, grn_obj *column, grn_operator mode);
static grn_rc get_token(grn_ctx *ctx, efs_info *q, efs_op *op, grn_obj *column, grn_operator mode);

static grn_rc
get_phrase(grn_ctx *ctx, efs_info *q, grn_obj *column, int mode, int option)
{
  const char *start, *s;
  start = s = q->cur;
  GRN_BULK_REWIND(&q->buf);
  while (1) {
    unsigned int len;
    if (s >= q->str_end) {
      q->cur = s;
      break;
    }
    len = grn_charlen(ctx, s, q->str_end);
    if (len == 0) {
      /* invalid string containing malformed multibyte char */
      return GRN_END_OF_DATA;
    } else if (len == 1) {
      if (*s == GRN_QUERY_QUOTER) {
        q->cur = s + 1;
        break;
      } else if (*s == GRN_QUERY_ESCAPE && s + 1 < q->str_end) {
        s++;
        len = grn_charlen(ctx, s, q->str_end);
      }
    }
    GRN_TEXT_PUT(ctx, &q->buf, s, len);
    s += len;
  }
  grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
  grn_expr_append_const(ctx, q->e, column, GRN_OP_PUSH, 1);
  grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
  grn_expr_append_const(ctx, q->e, &q->buf, GRN_OP_PUSH, 1);
  if (mode == GRN_OP_MATCH || mode == GRN_OP_EXACT) {
    grn_expr_append_op(ctx, q->e, mode, 2);
  } else {
    grn_expr_append_const_int(ctx, q->e, option, GRN_OP_PUSH, 1);
    grn_expr_append_op(ctx, q->e, mode, 3);
  }
  return GRN_SUCCESS;
}

static grn_rc
get_geocond(grn_ctx *ctx, efs_info *q, grn_obj *longitude, grn_obj *latitude)
{
  unsigned int len;
  const char *start = q->cur, *end;
  for (end = q->cur;; ) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, end, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    if (grn_isspace(end, ctx->encoding) ||
        *end == GRN_QUERY_PARENR) {
      q->cur = end;
      break;
    }
  }
  {
    const char *tokbuf[8];
    int32_t lng0, lat0, lng1, lat1, lng2, lat2, r;
    int32_t n = grn_str_tok((char *)start, end - start, ',', tokbuf, 8, NULL);
    switch (n) {
    case 3 :
      lng0 = grn_atoi(start, tokbuf[0], NULL);
      lat0 = grn_atoi(tokbuf[0] + 1, tokbuf[1], NULL);
      r = grn_atoi(tokbuf[1] + 1, tokbuf[2], NULL);
      grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
      grn_expr_append_const(ctx, q->e, longitude, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
      grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
      grn_expr_append_const(ctx, q->e, latitude, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
      grn_expr_append_const_int(ctx, q->e, lng0, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lat0, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, r, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GEO_WITHINP5, 5);
      break;
    case 4 :
      lng0 = grn_atoi(start, tokbuf[0], NULL);
      lat0 = grn_atoi(tokbuf[0] + 1, tokbuf[1], NULL);
      lng1 = grn_atoi(tokbuf[1] + 1, tokbuf[2], NULL);
      lat1 = grn_atoi(tokbuf[2] + 1, tokbuf[3], NULL);
      grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
      grn_expr_append_const(ctx, q->e, longitude, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
      grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
      grn_expr_append_const(ctx, q->e, latitude, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
      grn_expr_append_const_int(ctx, q->e, lng0, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lat0, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lng1, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lat1, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GEO_WITHINP6, 6);
      break;
    case 6 :
      lng0 = grn_atoi(start, tokbuf[0], NULL);
      lat0 = grn_atoi(tokbuf[0] + 1, tokbuf[1], NULL);
      lng1 = grn_atoi(tokbuf[1] + 1, tokbuf[2], NULL);
      lat1 = grn_atoi(tokbuf[2] + 1, tokbuf[3], NULL);
      lng2 = grn_atoi(tokbuf[3] + 1, tokbuf[4], NULL);
      lat2 = grn_atoi(tokbuf[4] + 1, tokbuf[5], NULL);
      grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
      grn_expr_append_const(ctx, q->e, longitude, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
      grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
      grn_expr_append_const(ctx, q->e, latitude, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
      grn_expr_append_const_int(ctx, q->e, lng0, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lat0, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lng1, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lat1, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lng2, GRN_OP_PUSH, 1);
      grn_expr_append_const_int(ctx, q->e, lat2, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, GRN_OP_GEO_WITHINP8, 8);
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "invalid geocond");
      break;
    }
  }
  return ctx->rc;
}

static grn_rc
get_word(grn_ctx *ctx, efs_info *q, grn_obj *column, int mode, int option)
{
  const char *start = q->cur, *end;
  unsigned int len;
  for (end = q->cur;; ) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, end, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    if (grn_isspace(end, ctx->encoding) ||
        *end == GRN_QUERY_PARENR) {
      q->cur = end;
      break;
    }
    if (*end == GRN_QUERY_COLUMN) {
      grn_obj *c = grn_obj_column(ctx, q->table, start, end - start);
      if (c && end + 1 < q->str_end) {
        efs_op op;
        switch (end[1]) {
        case '!' :
          mode = GRN_OP_NOT_EQUAL;
          q->cur = end + 2;
          break;
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            mode = GRN_OP_ASSIGN;
            q->cur = end + 2;
          } else {
            get_token(ctx, q, &op, c, mode);
          }
          break;
        case '<' :
          if (end + 2 < q->str_end && end[2] == '=') {
            mode = GRN_OP_LESS_EQUAL;
            q->cur = end + 3;
          } else {
            mode = GRN_OP_LESS;
            q->cur = end + 2;
          }
          break;
        case '>' :
          if (end + 2 < q->str_end && end[2] == '=') {
            mode = GRN_OP_GREATER_EQUAL;
            q->cur = end + 3;
          } else {
            mode = GRN_OP_GREATER;
            q->cur = end + 2;
          }
          break;
        case '%' :
          mode = GRN_OP_MATCH;
          q->cur = end + 2;
          break;
        case '@' :
          q->cur = end + 2;
          return get_geocond(ctx, q, column, c);
          break;
        default :
          mode = GRN_OP_EQUAL;
          q->cur = end + 1;
          break;
        }
        return get_token(ctx, q, &op, c, mode);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "column lookup failed");
        return ctx->rc;
      }
    } else if (*end == GRN_QUERY_PREFIX) {
      mode = GRN_OP_PREFIX;
      q->cur = end + 1;
      break;
    }
    end += len;
  }
  if (!column) {
    ERR(GRN_INVALID_ARGUMENT, "column missing");
    return ctx->rc;
  }
  if (mode == GRN_OP_ASSIGN) {
    grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
    grn_expr_append_const(ctx, q->e, column, GRN_OP_PUSH, 1);
    grn_expr_append_const_str(ctx, q->e, start, end - start, GRN_OP_PUSH, 1);
    grn_expr_append_op(ctx, q->e, GRN_OP_ASSIGN, 2);
  } else {
    grn_expr_append_obj(ctx, q->e, q->v, GRN_OP_PUSH, 1);
    grn_expr_append_const(ctx, q->e, column, GRN_OP_PUSH, 1);
    grn_expr_append_op(ctx, q->e, GRN_OP_GET_VALUE, 2);
    grn_expr_append_const_str(ctx, q->e, start, end - start, GRN_OP_PUSH, 1);
    switch (mode) {
    case GRN_OP_NEAR :
    case GRN_OP_NEAR2 :
    case GRN_OP_SIMILAR :
    case GRN_OP_TERM_EXTRACT :
      grn_expr_append_const_int(ctx, q->e, option, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, mode, 3);
      break;
    default :
      grn_expr_append_op(ctx, q->e, mode, 2);
      break;
    }
  }
  return GRN_SUCCESS;
}

static void
get_op(efs_info *q, efs_op *op, grn_operator *mode, int *option)
{
  const char *start, *end = q->cur;
  switch (*end) {
  case 'S' :
    *mode = GRN_OP_SIMILAR;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_SIMILARITY_THRESHOLD; }
    q->cur = end;
    break;
  case 'N' :
    *mode = GRN_OP_NEAR;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_MAX_INTERVAL; }
    q->cur = end;
    break;
  case 'n' :
    *mode = GRN_OP_NEAR2;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_MAX_INTERVAL; }
    q->cur = end;
    break;
  case 'T' :
    *mode = GRN_OP_TERM_EXTRACT;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_TERM_EXTRACT_POLICY; }
    q->cur = end;
    break;
  case 'X' : /* force exact mode */
    op->op = GRN_OP_AND;
    *mode = GRN_OP_EXACT;
    *option = 0;
    start = ++end;
    q->cur = end;
    break;
  default :
    break;
  }
}

static grn_rc
get_token(grn_ctx *ctx, efs_info *q, efs_op *op, grn_obj *column, grn_operator mode)
{
  int option = 0;
  op->op = q->default_op;
  op->weight = DEFAULT_WEIGHT;
  for (;;) {
    skip_space(ctx, q);
    if (q->cur >= q->str_end) { return GRN_END_OF_DATA; }
    switch (*q->cur) {
    case '\0' :
      return GRN_END_OF_DATA;
      break;
    case GRN_QUERY_PARENR :
      q->cur++;
      return GRN_END_OF_DATA;
      break;
    case GRN_QUERY_QUOTEL :
      q->cur++;
      return get_phrase(ctx, q, column, mode, option);
      break;
    case GRN_QUERY_PREFIX :
      q->cur++;
      get_op(q, op, &mode, &option);
      break;
    case GRN_QUERY_AND :
      q->cur++;
      op->op = GRN_OP_AND;
      break;
    case GRN_QUERY_BUT :
      q->cur++;
      op->op = GRN_OP_BUT;
      break;
    case GRN_QUERY_ADJ_INC :
      q->cur++;
      if (op->weight < 127) { op->weight++; }
      op->op = GRN_OP_ADJUST;
      break;
    case GRN_QUERY_ADJ_DEC :
      q->cur++;
      if (op->weight > -128) { op->weight--; }
      op->op = GRN_OP_ADJUST;
      break;
    case GRN_QUERY_ADJ_NEG :
      q->cur++;
      op->op = GRN_OP_ADJUST;
      op->weight = -1;
      break;
    case GRN_QUERY_PARENL :
      q->cur++;
      return get_expr(ctx, q, column, mode);
      break;
    case 'O' :
      if (q->cur[1] == 'R' && q->cur[2] == ' ') {
        q->cur += 2;
        op->op = GRN_OP_OR;
        break;
      }
      /* fallthru */
    default :
      return get_word(ctx, q, column, mode, option);
      break;
    }
  }
  return GRN_SUCCESS;
}

static grn_rc
get_expr(grn_ctx *ctx, efs_info *q, grn_obj *column, grn_operator mode)
{
  efs_op op;
  grn_rc rc = get_token(ctx, q, &op, column, mode);
  if (rc) { return rc; }
  while (!(rc = get_token(ctx, q, &op, column, mode))) {
    if (op.op == GRN_OP_ADJUST) {
      grn_expr_append_const_int(ctx, q->e, op.weight, GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx, q->e, op.op, 3);
    } else {
      grn_expr_append_op(ctx, q->e, op.op, 2);
    }
  }
  return rc;
}

static const char *
get_weight_vector(grn_ctx *ctx, efs_info *query, const char *source)
{
  const char *p;

  if (!query->opt.weight_vector &&
      !query->weight_set &&
      !(query->opt.weight_vector = GRN_CALLOC(sizeof(int) * DEFAULT_WEIGHT_VECTOR_SIZE))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "get_weight_vector malloc fail");
    return source;
  }
  for (p = source; p < query->str_end; ) {
    unsigned int key;
    int value;

    /* key, key is not zero */
    key = grn_atoui(p, query->str_end, &p);
    if (!key || key > GRN_ID_MAX) { break; }

    /* value */
    if (*p == ':') {
      p++;
      value = grn_atoi(p, query->str_end, &p);
    } else {
      value = 1;
    }

    if (query->weight_set) {
      int *pval;
      if (grn_hash_add(ctx, query->weight_set, &key, sizeof(unsigned int), (void **)&pval, NULL)) {
        *pval = value;
      }
    } else if (key < DEFAULT_WEIGHT_VECTOR_SIZE) {
      query->opt.weight_vector[key - 1] = value;
    } else {
      GRN_FREE(query->opt.weight_vector);
      query->opt.weight_vector = NULL;
      if (!(query->weight_set = grn_hash_create(ctx, NULL, sizeof(unsigned int), sizeof(int),
                                                0))) {
        return source;
      }
      p = source;           /* reparse */
      continue;
    }
    if (*p != ',') { break; }
    p++;
  }
  return p;
}

static void
get_pragma(grn_ctx *ctx, efs_info *q)
{
  const char *start, *end = q->cur;
  while (end < q->str_end && *end == GRN_QUERY_PREFIX) {
    if (++end >= q->str_end) { break; }
    switch (*end) {
    case 'E' :
      start = ++end;
      q->escalation_threshold = grn_atoi(start, q->str_end, (const char **)&end);
      while (end < q->str_end && (('0' <= *end && *end <= '9') || *end == '-')) { end++; }
      if (*end == ',') {
        start = ++end;
        q->escalation_decaystep = grn_atoi(start, q->str_end, (const char **)&end);
      }
      q->cur = end;
      break;
    case 'D' :
      start = ++end;
      while (end < q->str_end && *end != GRN_QUERY_PREFIX && !grn_isspace(end, ctx->encoding)) {
        end++;
      }
      if (end > start) {
        switch (*start) {
        case 'O' :
          q->default_op = GRN_OP_OR;
          break;
        case GRN_QUERY_AND :
          q->default_op = GRN_OP_AND;
          break;
        case GRN_QUERY_BUT :
          q->default_op = GRN_OP_BUT;
          break;
        case GRN_QUERY_ADJ_INC :
          q->default_op = GRN_OP_ADJUST;
          break;
        }
      }
      q->cur = end;
      break;
    case 'W' :
      start = ++end;
      end = (char *)get_weight_vector(ctx, q, start);
      q->cur = end;
      break;
    }
  }
}

static int
section_weight_cb(grn_ctx *ctx, grn_hash *r, const void *rid, int sid, void *arg)
{
  int *w;
  grn_hash *s = (grn_hash *)arg;
  if (s && grn_hash_get(ctx, s, &sid, sizeof(grn_id), (void **)&w)) {
    return *w;
  } else {
    return 0;
  }
}

grn_table_sort_key *
grn_table_sort_key_from_str(grn_ctx *ctx, const char *str, unsigned str_size,
                            grn_obj *table, unsigned *nkeys)
{
  const char **tokbuf;
  grn_table_sort_key *keys = NULL, *k = NULL;
  if ((tokbuf = GRN_MALLOCN(const char *, str_size))) {
    int i, n = tokenize(str, str_size, tokbuf, str_size, NULL);
    if ((keys = GRN_MALLOCN(grn_table_sort_key, n))) {
      k = keys;
      for (i = 0; i < n; i++) {
        k->flags = GRN_TABLE_SORT_ASC;
        k->offset = 0;
        if (*str == '+') {
          str++;
        } else if (*str == '-') {
          k->flags = GRN_TABLE_SORT_DESC;
          str++;
        }
        if ((k->key = grn_obj_column(ctx, table, str, tokbuf[i] - str))) {
          k++;
        }
        str = tokbuf[i] + 1;
      }
    }
    GRN_FREE(tokbuf);
  }
  *nkeys = k - keys;
  return keys;
}

grn_rc
grn_table_sort_key_close(grn_ctx *ctx, grn_table_sort_key *keys, unsigned nkeys)
{
  int i;
  for (i = 0; i < nkeys; i++) {
    grn_obj_unlink(ctx, keys[i].key);
  }
  GRN_FREE(keys);
  return ctx->rc;
}

grn_rc
grn_select(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
           const char *table, unsigned table_len,
           const char *match_columns, unsigned match_columns_len,
           const char *query, unsigned query_len,
           const char *filter, unsigned filter_len,
           const char *scorer, unsigned scorer_len,
           const char *sortby, unsigned sortby_len,
           const char *output_columns, unsigned output_columns_len,
           int offset, int limit,
           const char *drilldown, unsigned drilldown_len,
           const char *drilldown_sortby, unsigned drilldown_sortby_len,
           const char *drilldown_output_columns, unsigned drilldown_output_columns_len,
           int drilldown_offset, int drilldown_limit)
{
  uint32_t nkeys, nhits;
  grn_obj_format format;
  grn_table_sort_key *keys;
  grn_obj *table_, *match_columns_ = NULL, *cond, *scorer_, *res = NULL, *sorted;
  /*
  GRN_LOG(ctx, GRN_LOG_NONE,
          "%d(%.*s)(%.*s)(%.*s)(%.*s)(%.*s)(%.*s)(%.*s)(%d)(%d)(%.*s)(%.*s)(%.*s)(%d)(%d)",
          ctx->seqno,
          table_len, table,
          match_columns_len, match_columns,
          query_len, query,
          filter_len, filter,
          scorer_len, scorer,
          sortby_len, sortby,
          output_columns_len, output_columns,
          offset, limit,
          drilldown, drilldown_len,
          drilldown_sortby, drilldown_sortby_len,
          drilldown_output_columns, drilldown_output_columns_len,
          drilldown_offset, drilldown_limit);
  */
  if ((table_ = grn_ctx_get(ctx, table, table_len))) {
    // match_columns_ = grn_obj_column(ctx, table_, match_columns, match_columns_len);
    if (query_len || filter_len) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, cond, v);
      if (cond) {
        if (match_columns_len) {
          GRN_EXPR_CREATE_FOR_QUERY(ctx, table_, match_columns_, v);
          if (match_columns_) {
            grn_expr_parse(ctx, match_columns_, match_columns, match_columns_len,
                           NULL, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
          } else {
            /* todo */
          }
        }
        if (query_len) {
          grn_expr_parse(ctx, cond, query, query_len,
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
          if (filter_len) {
            grn_expr_parse(ctx, cond, filter, filter_len,
                           match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                           GRN_EXPR_SYNTAX_SCRIPT);
            grn_expr_append_op(ctx, cond, GRN_OP_AND, 2);
          }
        } else {
          grn_expr_parse(ctx, cond, filter, filter_len,
                         match_columns_, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT);
        }
        /*
        grn_obj strbuf;
        GRN_TEXT_INIT(&strbuf, 0);
        grn_expr_inspect(ctx, &strbuf, cond);
        GRN_TEXT_PUTC(ctx, &strbuf, '\0');
        GRN_LOG(ctx, GRN_LOG_NOTICE, "query=(%s)", GRN_TEXT_VALUE(&strbuf));
        GRN_OBJ_FIN(ctx, &strbuf);
        */
        if (!ctx->rc) { res = grn_table_select(ctx, table_, cond, NULL, GRN_OP_OR); }
        if (match_columns_) { grn_obj_unlink(ctx, match_columns_); }
        grn_obj_unlink(ctx, cond);
      } else {
        /* todo */
        ERRCLR(ctx);
      }
    } else {
      res = table_;
    }
    switch (output_type) {
    case GRN_CONTENT_JSON:
      GRN_TEXT_PUTS(ctx, outbuf, "[[");
      grn_text_itoa(ctx, outbuf, ctx->rc);
      {
        double dv;
        grn_timeval tv;
        grn_timeval_now(ctx, &tv);
        dv = ctx->impl->tv.tv_sec;
        dv += ctx->impl->tv.tv_usec / 1000000.0;
        GRN_TEXT_PUTC(ctx, outbuf, ',');
        grn_text_ftoa(ctx, outbuf, dv);
        dv = (tv.tv_sec - ctx->impl->tv.tv_sec);
        dv += (tv.tv_usec - ctx->impl->tv.tv_usec) / 1000000.0;
        GRN_TEXT_PUTC(ctx, outbuf, ',');
        grn_text_ftoa(ctx, outbuf, dv);
      }
      break;
    case GRN_CONTENT_XML:
      GRN_TEXT_PUTS(ctx, outbuf, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                    "<SEGMENTS>\n<SEGMENT>\n<RESULTPAGE>\n");
      break;
    case GRN_CONTENT_TSV:
      grn_text_itoa(ctx, outbuf, ctx->rc);
    case GRN_CONTENT_NONE:
      break;
    }
    if (ctx->rc) {
      switch (output_type) {
      case GRN_CONTENT_JSON:
        GRN_TEXT_PUTC(ctx, outbuf, ',');
        grn_text_esc(ctx, outbuf, ctx->errbuf, strlen(ctx->errbuf));
        break;
      case GRN_CONTENT_XML:
        break;
      case GRN_CONTENT_TSV:
        GRN_TEXT_PUTC(ctx, outbuf, '\t');
        GRN_TEXT_PUTS(ctx, outbuf, ctx->errbuf);
        break;
      case GRN_CONTENT_NONE:
        break;
      }
    }
    switch (output_type) {
    case GRN_CONTENT_JSON:
      GRN_TEXT_PUTS(ctx, outbuf, "],[");
      break;
    case GRN_CONTENT_TSV:
      GRN_TEXT_PUTC(ctx, outbuf, '\n');
      break;
    case GRN_CONTENT_XML:
    case GRN_CONTENT_NONE:
      break;
    }
    if (res) {
      if (scorer && scorer_len) {
        grn_obj *v;
        GRN_EXPR_CREATE_FOR_QUERY(ctx, res, scorer_, v);
        if (scorer_ && v) {
          grn_table_cursor *tc;
          grn_expr_parse(ctx, scorer_, scorer, scorer_len, NULL, GRN_OP_MATCH, GRN_OP_AND,
                         GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
          if ((tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, 0))) {
            while (!grn_table_cursor_next_o(ctx, tc, v)) {
              grn_expr_exec(ctx, scorer_, 0);
              grn_ctx_pop(ctx);
            }
            grn_table_cursor_close(ctx, tc);
          }
          grn_obj_unlink(ctx, scorer_);
        }
      }
      nhits = grn_table_size(ctx, res);
      if (sortby_len) {
        if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
          if ((keys = grn_table_sort_key_from_str(ctx, sortby, sortby_len, res, &nkeys))) {
            grn_table_sort(ctx, res, offset, limit, sorted, keys, nkeys);
            GRN_OBJ_FORMAT_INIT(&format, nhits, 0, limit);
            grn_obj_columns(ctx, sorted, output_columns, output_columns_len, &format.columns);
            switch (output_type) {
            case GRN_CONTENT_JSON:
              format.flags = GRN_OBJ_FORMAT_WITH_COLUMN_NAMES;
              grn_text_otoj(ctx, outbuf, sorted, &format);
              break;
            case GRN_CONTENT_TSV:
              GRN_TEXT_PUTC(ctx, outbuf, '\n');
              /* TODO: implement grn_text_ototsv */
              break;
            case GRN_CONTENT_XML:
              format.flags = GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
              grn_text_otoxml(ctx, outbuf, sorted, &format);
              break;
            case GRN_CONTENT_NONE:
              break;
            }
            GRN_OBJ_FORMAT_FIN(ctx, &format);
            grn_table_sort_key_close(ctx, keys, nkeys);
          }
          grn_obj_unlink(ctx, sorted);
        }
      } else {
        GRN_OBJ_FORMAT_INIT(&format, nhits, offset, limit);
        grn_obj_columns(ctx, res, output_columns, output_columns_len, &format.columns);
        switch (output_type) {
        case GRN_CONTENT_JSON:
          format.flags = GRN_OBJ_FORMAT_WITH_COLUMN_NAMES;
          grn_text_otoj(ctx, outbuf, res, &format);
          break;
        case GRN_CONTENT_TSV:
          GRN_TEXT_PUTC(ctx, outbuf, '\n');
          /* TODO: implement */
          break;
        case GRN_CONTENT_XML:
          format.flags = GRN_OBJ_FORMAT_XML_ELEMENT_RESULTSET;
          grn_text_otoxml(ctx, outbuf, res, &format);
          break;
        case GRN_CONTENT_NONE:
          break;
        }
        GRN_OBJ_FORMAT_FIN(ctx, &format);
      }
      if (drilldown_len) {
        uint32_t i, ngkeys;
        grn_table_sort_key *gkeys;
        grn_table_group_result g = {NULL, 0, 0, 1, GRN_TABLE_GROUP_CALC_COUNT, 0};
        gkeys = grn_table_sort_key_from_str(ctx, drilldown, drilldown_len, res, &ngkeys);
        for (i = 0; i < ngkeys; i++) {
          if ((g.table = grn_table_create_for_group(ctx, NULL, 0, NULL,
                                                    GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                                    gkeys[i].key, NULL))) {
            grn_table_group(ctx, res, &gkeys[i], 1, &g, 1);
            nhits = grn_table_size(ctx, g.table);
            if (drilldown_sortby_len) {
              if ((keys = grn_table_sort_key_from_str(ctx,
                                                      drilldown_sortby, drilldown_sortby_len,
                                                      g.table, &nkeys))) {
                if ((sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY,
                                               NULL, g.table))) {
                  grn_table_sort(ctx, g.table, drilldown_offset, drilldown_limit,
                                 sorted, keys, nkeys);
                  GRN_OBJ_FORMAT_INIT(&format, nhits, 0, drilldown_limit);
                  grn_obj_columns(ctx, sorted,
                                  drilldown_output_columns, drilldown_output_columns_len,
                                  &format.columns);
                  switch (output_type) {
                  case GRN_CONTENT_JSON:
                    format.flags = GRN_OBJ_FORMAT_WITH_COLUMN_NAMES;
                    GRN_TEXT_PUTC(ctx, outbuf, ',');
                    grn_text_otoj(ctx, outbuf, sorted, &format);
                    break;
                  case GRN_CONTENT_XML:
                    format.flags = GRN_OBJ_FORMAT_XML_ELEMENT_NAVIGATIONENTRY;
                    grn_text_otoxml(ctx, outbuf, sorted, &format);
                    break;
                  case GRN_CONTENT_NONE:
                  case GRN_CONTENT_TSV:
                    /* TODO: implement */
                    break;
                  }
                  GRN_OBJ_FORMAT_FIN(ctx, &format);
                  grn_obj_unlink(ctx, sorted);
                }
                grn_table_sort_key_close(ctx, keys, nkeys);
              }
            } else {
              GRN_OBJ_FORMAT_INIT(&format, nhits, drilldown_offset, drilldown_limit);
              grn_obj_columns(ctx, g.table, drilldown_output_columns,
                              drilldown_output_columns_len, &format.columns);
              switch (output_type) {
              case GRN_CONTENT_JSON:
                format.flags = GRN_OBJ_FORMAT_WITH_COLUMN_NAMES;
                GRN_TEXT_PUTC(ctx, outbuf, ',');
                grn_text_otoj(ctx, outbuf, g.table, &format);
                break;
              case GRN_CONTENT_XML:
                format.flags = GRN_OBJ_FORMAT_XML_ELEMENT_NAVIGATIONENTRY;
                grn_text_otoxml(ctx, outbuf, g.table, &format);
                break;
              case GRN_CONTENT_NONE:
              case GRN_CONTENT_TSV:
                /* TODO: implement */
                break;
              }
              GRN_OBJ_FORMAT_FIN(ctx, &format);
            }
            grn_obj_unlink(ctx, g.table);
          }
        }
        grn_table_sort_key_close(ctx, gkeys, ngkeys);
      }
      if (res != table_) { grn_obj_unlink(ctx, res); }
    }
    switch (output_type) {
    case GRN_CONTENT_JSON:
      GRN_TEXT_PUTS(ctx, outbuf, "]]");
      break;
    case GRN_CONTENT_TSV:
      GRN_TEXT_PUTC(ctx, outbuf, '\n');
      break;
    case GRN_CONTENT_XML:
      GRN_TEXT_PUTS(ctx, outbuf, "</RESULTPAGE>\n</SEGMENT>\n</SEGMENTS>\n");
      break;
    case GRN_CONTENT_NONE:
      break;
    }
    grn_obj_unlink(ctx, table_);
  }
  /* GRN_LOG(ctx, GRN_LOG_NONE, "%d", ctx->seqno); */
  return ctx->rc;
}

/* grn_load */

static grn_obj *
values_add(grn_ctx *ctx, grn_loader *loader)
{
  grn_obj *res;
  uint32_t curr_size = loader->values_size * sizeof(grn_obj);
  if (curr_size < GRN_TEXT_LEN(&loader->values)) {
    res = (grn_obj *)(GRN_TEXT_VALUE(&loader->values) + curr_size);
    res->header.domain = GRN_DB_TEXT;
    GRN_BULK_REWIND(res);
  } else {
    if (grn_bulk_space(ctx, &loader->values, sizeof(grn_obj))) { return NULL; }
    res = (grn_obj *)(GRN_TEXT_VALUE(&loader->values) + curr_size);
    GRN_TEXT_INIT(res, 0);
  }
  loader->values_size++;
  loader->last = res;
  return res;
}

#define GRN_UINT32_POP(obj,value) {\
  if (GRN_BULK_VSIZE(obj) >= sizeof(uint32_t)) {\
    GRN_BULK_INCR_LEN((obj), -(sizeof(uint32_t)));\
    value = *(uint32_t *)(GRN_BULK_CURR(obj));\
  } else {\
    value = 0;\
  }\
}

#define OPEN_BRACKET 0x40000000
#define OPEN_BRACE   0x40000001

static grn_obj *
values_next(grn_ctx *ctx, grn_obj *value)
{
  if (value->header.domain & OPEN_BRACKET) {
    value += GRN_UINT32_VALUE(value);
  }
  return value + 1;
}

static int
values_len(grn_ctx *ctx, grn_obj *head, grn_obj *tail)
{
  int len;
  for (len = 0; head < tail; head = values_next(ctx, head), len++) ;
  return len;
}

static grn_id
loader_add(grn_ctx *ctx, grn_obj *key)
{
  int added = 0;
  grn_loader *loader = &ctx->impl->loader;
  grn_id id = grn_table_add_by_key(ctx, loader->table, key, &added);
  if (!added && loader->ifexists) {
    grn_obj *v = grn_expr_get_var_by_offset(ctx, loader->ifexists, 0);
    grn_obj *result;
    unsigned int result_boolean;
    GRN_RECORD_SET(ctx, v, id);
    grn_expr_exec(ctx, loader->ifexists, 0);
    result = grn_ctx_pop(ctx);
    truep(ctx, result, result_boolean);
    if (!result_boolean) { id = 0; }
  }
  return id;
}

static void
set_vector(grn_ctx *ctx, grn_obj *column, grn_id id, grn_obj *vector)
{
  int n = GRN_UINT32_VALUE(vector);
  grn_obj buf, *v = vector + 1;
  grn_id range_id;
  grn_obj *range;

  range_id = DB_OBJ(column)->range;
  range = grn_ctx_at(ctx, range_id);
  if (GRN_OBJ_TABLEP(range)) {
    GRN_RECORD_INIT(&buf, GRN_OBJ_VECTOR, range_id);
    while (n--) {
      if (v->header.domain == GRN_DB_TEXT) {
        grn_obj record, *element = v;
        GRN_RECORD_INIT(&record, 0, range_id);
        grn_obj_cast(ctx, element, &record, 1);
        element = &record;
        GRN_UINT32_PUT(ctx, &buf, GRN_RECORD_VALUE(element));
      } else {
        ERR(GRN_ERROR, "bad syntax.");
      }
      v = values_next(ctx, v);
    }
  } else {
    if (((struct _grn_type *)grn_ctx_at(ctx, range_id))->obj.header.flags &
        GRN_OBJ_KEY_VAR_SIZE) {
      GRN_TEXT_INIT(&buf, GRN_OBJ_VECTOR);
      while (n--) {
        if (v->header.domain == GRN_DB_TEXT) {
          grn_obj cast_element, *element = v;
          if (range_id != element->header.domain) {
            GRN_OBJ_INIT(&cast_element, GRN_BULK, 0, range_id);
            grn_obj_cast(ctx, element, &cast_element, 1);
            element = &cast_element;
          }
          grn_vector_add_element(ctx, &buf,
                                 GRN_TEXT_VALUE(element),
                                 GRN_TEXT_LEN(element), 0, GRN_ID_NIL);
        } else {
          ERR(GRN_ERROR, "bad syntax.");
        }
        v = values_next(ctx, v);
      }
    } else {
      grn_id value_size = ((grn_db_obj *)grn_ctx_at(ctx, range_id))->range;
      GRN_VALUE_FIX_SIZE_INIT(&buf, GRN_OBJ_VECTOR, range_id);
      while (n--) {
        if (v->header.domain == GRN_DB_TEXT) {
          grn_obj cast_element, *element = v;
          if (range_id != element->header.domain) {
            GRN_OBJ_INIT(&cast_element, GRN_BULK, 0, range_id);
            grn_obj_cast(ctx, element, &cast_element, 1);
            element = &cast_element;
          }
          grn_bulk_write(ctx, &buf, GRN_TEXT_VALUE(element), value_size);
        } else {
          ERR(GRN_ERROR, "bad syntax.");
        }
        v = values_next(ctx, v);
      }
    }
  }
  grn_obj_set_value(ctx, column, id, &buf, GRN_OBJ_SET);
  GRN_OBJ_FIN(ctx, &buf);
}

#define PKEY_NAME "_key"

static void
bracket_close(grn_ctx *ctx, grn_loader *loader)
{
  grn_obj *value, *col, *ve;
  grn_id id = GRN_ID_NIL;
  grn_obj **cols = (grn_obj **)GRN_BULK_HEAD(&loader->columns);
  uint32_t begin, ndata, ncols = GRN_BULK_VSIZE(&loader->columns) / sizeof(grn_obj *);
  GRN_UINT32_POP(&loader->level, begin);
  value = ((grn_obj *)(GRN_TEXT_VALUE(&loader->values))) + begin;
  ve = ((grn_obj *)(GRN_TEXT_VALUE(&loader->values))) + loader->values_size;
  GRN_ASSERT(value->header.domain & OPEN_BRACKET);
  GRN_UINT32_SET(ctx, value, loader->values_size - begin - 1);
  value++;
  if (GRN_BULK_VSIZE(&loader->level) <= sizeof(uint32_t)) {
    ndata = values_len(ctx, value, ve);
    if (loader->table) {
      switch (loader->table->header.type) {
      case GRN_TABLE_HASH_KEY :
      case GRN_TABLE_PAT_KEY :
        if (loader->key_offset != -1 && ndata == ncols + 1) {
          id = loader_add(ctx, value + loader->key_offset);
        } else if (loader->key_offset == -1) {
          int i = 0;
          while (ndata--) {
            char *column_name = GRN_TEXT_VALUE(value);
            unsigned column_name_size = GRN_TEXT_LEN(value);
            if (value->header.domain == GRN_DB_TEXT &&
                column_name_size == strlen(PKEY_NAME) &&
                (*column_name == ':' || *column_name == '_') &&
                !memcmp(column_name + 1, "key", 3)) {
              loader->key_offset = i;
            } else {
              col = grn_obj_column(ctx, loader->table,
                                   column_name, column_name_size);
              if (!col) {
                ERR(GRN_ERROR, "couldn't get column: %.*s",
                    column_name_size, column_name);
                return;
              }
              GRN_PTR_PUT(ctx, &loader->columns, col);
            }
            value++;
            i++;
          }
        }
        break;
      case GRN_TABLE_NO_KEY :
        if ((GRN_BULK_VSIZE(&loader->level)) > 0 &&
            loader->key_offset != -1 && (ndata == 0 || ndata == ncols)) {
          id = grn_table_add(ctx, loader->table, NULL, 0, NULL);
        } else if (!ncols) {
          while (ndata--) {
            col = grn_obj_column(ctx, loader->table,
                                 GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
            GRN_PTR_PUT(ctx, &loader->columns, col);
            value++;
          }
          loader->key_offset = 0;
        }
        break;
      default :
        break;
      }
      if (id) {
        int i = 0;
        while (ndata--) {
          if ((loader->table->header.type == GRN_TABLE_HASH_KEY ||
               loader->table->header.type == GRN_TABLE_PAT_KEY) &&
              i == loader->key_offset) {
              /* skip this value, because it's already used as key value */
             value = values_next(ctx, value);
             i++;
             continue;
          }
          if (value->header.domain == OPEN_BRACKET) {
            set_vector(ctx, *cols, id, value);
          } else if (value->header.domain == OPEN_BRACE) {
            /* todo */
          } else {
            grn_obj_set_value(ctx, *cols, id, value, GRN_OBJ_SET);
          }
          value = values_next(ctx, value);
          cols++;
          i++;
        }
        loader->nrecords++;
      }
    }
    loader->values_size = begin;
  }
}

static void
brace_close(grn_ctx *ctx, grn_loader *loader)
{
  uint32_t begin;
  grn_obj *value, *ve;
  grn_id id = GRN_ID_NIL;
  GRN_UINT32_POP(&loader->level, begin);
  value = ((grn_obj *)(GRN_TEXT_VALUE(&loader->values))) + begin;
  ve = ((grn_obj *)(GRN_TEXT_VALUE(&loader->values))) + loader->values_size;
  GRN_ASSERT(value->header.domain & OPEN_BRACKET);
  GRN_UINT32_SET(ctx, value, loader->values_size - begin - 1);
  value++;
  if (GRN_BULK_VSIZE(&loader->level) <= sizeof(uint32_t)) {
    if (loader->table) {
      switch (loader->table->header.type) {
      case GRN_TABLE_HASH_KEY :
      case GRN_TABLE_PAT_KEY :
        {
          grn_obj *v;
          for (v = value; v + 1 < ve; v = values_next(ctx, v)) {
            char *p = GRN_TEXT_VALUE(v);
            if (v->header.domain == GRN_DB_TEXT &&
                GRN_TEXT_LEN(v) == strlen(PKEY_NAME) &&
                (*p == ':' || *p == '_') && !memcmp(p + 1, "key", 3)) {
              v++;
              if (v->header.domain == GRN_DB_TEXT) {
                id = loader_add(ctx, v);
              }
              break;
            } else {
              v = values_next(ctx, v);
            }
          }
        }
        break;
      case GRN_TABLE_NO_KEY :
        id = grn_table_add(ctx, loader->table, NULL, 0, NULL);
        break;
      default :
        break;
      }
      if (id) {
        grn_obj *col;
        const char *name;
        unsigned name_size;
        while (value + 1 < ve) {
          if (value->header.domain != GRN_DB_TEXT) { break; /* error */ }
          name = GRN_TEXT_VALUE(value);
          name_size = GRN_TEXT_LEN(value);
          col = grn_obj_column(ctx, loader->table, name, name_size);
          value++;
          /* auto column create
          if (!col) {
            if (value->header.domain == OPEN_BRACKET) {
              grn_obj *v = value + 1;
              col = grn_column_create(ctx, loader->table, name, name_size,
                                      NULL, GRN_OBJ_PERSISTENT|GRN_OBJ_COLUMN_VECTOR,
                                      grn_ctx_at(ctx, v->header.domain));
            } else {
              col = grn_column_create(ctx, loader->table, name, name_size,
                                      NULL, GRN_OBJ_PERSISTENT,
                                      grn_ctx_at(ctx, value->header.domain));
            }
          }
          */
          if (col) {
            if (value->header.domain == OPEN_BRACKET) {
              set_vector(ctx, col, id, value);
            } else if (value->header.domain == OPEN_BRACE) {
              /* todo */
            } else {
              grn_obj_set_value(ctx, col, id, value, GRN_OBJ_SET);
            }
            grn_obj_unlink(ctx, col);
          }
          value = values_next(ctx, value);
        }
        loader->nrecords++;
      }
    }
    loader->values_size = begin;
  }
}

static void
json_read(grn_ctx *ctx, grn_loader *loader, const char *str, unsigned str_len)
{
  char c;
  int len;
  const char *se = str + str_len;
  while (str < se) {
    c = *str;
    switch (loader->stat) {
    case GRN_LOADER_BEGIN :
    case GRN_LOADER_TOKEN :
      if ((len = grn_isspace(str, ctx->encoding))) {
        str += len;
        continue;
      }
      switch (c) {
      case '"' :
        loader->stat = GRN_LOADER_STRING;
        values_add(ctx, loader);
        str++;
        break;
      case '[' :
        GRN_UINT32_PUT(ctx, &loader->level, loader->values_size);
        values_add(ctx, loader);
        loader->last->header.domain = OPEN_BRACKET;
        loader->stat = GRN_LOADER_TOKEN;
        str++;
        break;
      case '{' :
        GRN_UINT32_PUT(ctx, &loader->level, loader->values_size);
        values_add(ctx, loader);
        loader->last->header.domain = OPEN_BRACE;
        loader->stat = GRN_LOADER_TOKEN;
        str++;
        break;
      case ':' :
        str++;
        break;
      case ',' :
        str++;
        break;
      case ']' :
        bracket_close(ctx, loader);
        loader->stat = GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        str++;
        break;
      case '}' :
        brace_close(ctx, loader);
        loader->stat = GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        str++;
        break;
      case '+' : case '-' : case '0' : case '1' : case '2' : case '3' :
      case '4' : case '5' : case '6' : case '7' : case '8' : case '9' :
        loader->stat = GRN_LOADER_NUMBER;
        values_add(ctx, loader);
        break;
      default :
        if ('A' <= c && c <= 'z') {
          loader->stat = GRN_LOADER_SYMBOL;
          values_add(ctx, loader);
        } else {
          if ((len = grn_charlen(ctx, str, se))) {
            str += len;
          } else {
            str = se;
          }
        }
        break;
      }
      break;
    case GRN_LOADER_SYMBOL :
      if ('A' <= c && c <= 'z') {
        GRN_TEXT_PUTC(ctx, loader->last, c);
        str++;
      } else {
#ifdef CAST_IN_JSON_READ
        char *v = GRN_TEXT_VALUE(loader->last);
        switch (*v) {
        case 'n' :
          if (GRN_TEXT_LEN(loader->last) == 4 && !memcmp(v, "null", 4)) {
            loader->last->header.domain = GRN_DB_VOID;
            GRN_BULK_REWIND(loader->last);
          }
          break;
        case 't' :
          if (GRN_TEXT_LEN(loader->last) == 4 && !memcmp(v, "true", 4)) {
            loader->last->header.domain = GRN_DB_BOOL;
            GRN_BOOL_SET(ctx, loader->last, GRN_TRUE);
          }
          break;
        case 'f' :
          if (GRN_TEXT_LEN(loader->last) == 5 && !memcmp(v, "false", 5)) {
            loader->last->header.domain = GRN_DB_BOOL;
            GRN_BOOL_SET(ctx, loader->last, GRN_FALSE);
          }
          break;
        default :
          break;
        }
#endif /* CAST_IN_JSON_READ */
        loader->stat = GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
      }
      break;
    case GRN_LOADER_NUMBER :
      switch (c) {
      case '+' : case '-' : case '.' : case 'e' : case 'E' :
      case '0' : case '1' : case '2' : case '3' : case '4' :
      case '5' : case '6' : case '7' : case '8' : case '9' :
        GRN_TEXT_PUTC(ctx, loader->last, c);
        str++;
        break;
      default :
#ifdef CAST_IN_JSON_READ
        {
          const char *cur, *str = GRN_BULK_HEAD(loader->last);
          const char *str_end = GRN_BULK_CURR(loader->last);
          int64_t i = grn_atoll(str, str_end, &cur);
          if (cur == str_end) {
            loader->last->header.domain = GRN_DB_INT64;
            GRN_INT64_SET(ctx, loader->last, i);
          } else if (cur != str) {
            double d;
            char *end;
            grn_obj buf;
            GRN_TEXT_INIT(&buf, 0);
            GRN_TEXT_PUT(ctx, &buf, str, GRN_BULK_VSIZE(loader->last));
            GRN_TEXT_PUTC(ctx, &buf, '\0');
            errno = 0;
            d = strtod(GRN_TEXT_VALUE(&buf), &end);
            if (!errno && end + 1 == GRN_BULK_CURR(&buf)) {
              loader->last->header.domain = GRN_DB_FLOAT;
              GRN_FLOAT_SET(ctx, loader->last, d);
            }
            GRN_OBJ_FIN(ctx, &buf);
          }
        }
#endif /* CAST_IN_JSON_READ */
        loader->stat = GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        break;
      }
      break;
    case GRN_LOADER_STRING :
      switch (c) {
      case '\\' :
        loader->stat = GRN_LOADER_STRING_ESC;
        str++;
        break;
      case '"' :
        str++;
        loader->stat = GRN_BULK_VSIZE(&loader->level) ? GRN_LOADER_TOKEN : GRN_LOADER_END;
        /*
        *(GRN_BULK_CURR(loader->last)) = '\0';
        GRN_LOG(ctx, GRN_LOG_ALERT, "read str(%s)", GRN_TEXT_VALUE(loader->last));
        */
        break;
      default :
        if ((len = grn_charlen(ctx, str, se))) {
          GRN_TEXT_PUT(ctx, loader->last, str, len);
          str += len;
        } else {
          str = se;
        }
        break;
      }
      break;
    case GRN_LOADER_STRING_ESC :
      switch (c) {
      case 'b' :
        GRN_TEXT_PUTC(ctx, loader->last, '\b');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'f' :
        GRN_TEXT_PUTC(ctx, loader->last, '\f');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'n' :
        GRN_TEXT_PUTC(ctx, loader->last, '\n');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'r' :
        GRN_TEXT_PUTC(ctx, loader->last, '\r');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 't' :
        GRN_TEXT_PUTC(ctx, loader->last, '\t');
        loader->stat = GRN_LOADER_STRING;
        break;
      case 'u' :
        loader->stat = GRN_LOADER_UNICODE0;
        break;
      default :
        GRN_TEXT_PUTC(ctx, loader->last, c);
        loader->stat = GRN_LOADER_STRING;
        break;
      }
      str++;
      break;
    case GRN_LOADER_UNICODE0 :
      switch (c) {
      case '0' : case '1' : case '2' : case '3' : case '4' :
      case '5' : case '6' : case '7' : case '8' : case '9' :
        loader->unichar = (c - '0') * 0x1000;
        break;
      case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
        loader->unichar = (c - 'a' + 10) * 0x1000;
        break;
      case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
        loader->unichar = (c - 'A' + 10) * 0x1000;
        break;
      default :
        ;// todo : error
      }
      loader->stat = GRN_LOADER_UNICODE1;
      str++;
      break;
    case GRN_LOADER_UNICODE1 :
      switch (c) {
      case '0' : case '1' : case '2' : case '3' : case '4' :
      case '5' : case '6' : case '7' : case '8' : case '9' :
        loader->unichar += (c - '0') * 0x100;
        break;
      case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
        loader->unichar += (c - 'a' + 10) * 0x100;
        break;
      case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
        loader->unichar += (c - 'A' + 10) * 0x100;
        break;
      default :
        ;// todo : error
      }
      loader->stat = GRN_LOADER_UNICODE2;
      str++;
      break;
    case GRN_LOADER_UNICODE2 :
      switch (c) {
      case '0' : case '1' : case '2' : case '3' : case '4' :
      case '5' : case '6' : case '7' : case '8' : case '9' :
        loader->unichar += (c - '0') * 0x10;
        break;
      case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
        loader->unichar += (c - 'a' + 10) * 0x10;
        break;
      case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
        loader->unichar += (c - 'A' + 10) * 0x10;
        break;
      default :
        ;// todo : error
      }
      loader->stat = GRN_LOADER_UNICODE3;
      str++;
      break;
    case GRN_LOADER_UNICODE3 :
      switch (c) {
      case '0' : case '1' : case '2' : case '3' : case '4' :
      case '5' : case '6' : case '7' : case '8' : case '9' :
        loader->unichar += (c - '0');
        break;
      case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
        loader->unichar += (c - 'a' + 10);
        break;
      case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
        loader->unichar += (c - 'A' + 10);
        break;
      default :
        ;// todo : error
      }
      {
        uint32_t u = loader->unichar;
        if (u < 0x80) {
          GRN_TEXT_PUTC(ctx, loader->last, u);
        } else {
          if (u < 0x800) {
            GRN_TEXT_PUTC(ctx, loader->last, ((u >> 6) & 0x1f) | 0xc0);
          } else {
            GRN_TEXT_PUTC(ctx, loader->last, (u >> 12) | 0xe0);
            GRN_TEXT_PUTC(ctx, loader->last, ((u >> 6) & 0x3f) | 0x80);
          }
          GRN_TEXT_PUTC(ctx, loader->last, (u & 0x3f) | 0x80);
        }
      }
      loader->stat = GRN_LOADER_STRING;
      str++;
      break;
    case GRN_LOADER_END :
      str = se;
      break;
    }
  }
}

grn_com_addr *addr;

grn_rc
grn_load(grn_ctx *ctx, grn_content_type input_type,
         const char *table, unsigned table_len,
         const char *columns, unsigned columns_len,
         const char *values, unsigned values_len,
         const char *ifexists, unsigned ifexists_len)
{
  grn_loader *loader;
  if (!ctx || !ctx->impl) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return ctx->rc;
  }
  GRN_API_ENTER;
  loader = &ctx->impl->loader;

  if (ctx->impl->edge) {
    grn_edge *edge = grn_edges_add_communicator(ctx, addr);
    grn_obj *msg = grn_msg_open(ctx, edge->com, &ctx->impl->edge->send_old);
    /* build msg */
    grn_edge_dispatch(ctx, edge, msg);
  }

  switch (input_type) {
  case GRN_CONTENT_JSON :
    if (table && table_len) {
      grn_ctx_loader_clear(ctx);
      loader->table = grn_ctx_get(ctx, table, table_len);
      if (loader->table && columns && columns_len) {
        grn_obj_columns(ctx, loader->table, columns, columns_len, &loader->columns);
      }
    }
    if (ifexists && ifexists_len) {
      grn_obj *v;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, loader->table, loader->ifexists, v);
      if (loader->ifexists && v) {
        grn_expr_parse(ctx, loader->ifexists, ifexists, ifexists_len,
                       NULL, GRN_OP_EQUAL, GRN_OP_AND,
                       GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
      }
    }
    json_read(ctx, loader, values, values_len);
    break;
  case GRN_CONTENT_NONE :
  case GRN_CONTENT_TSV :
  case GRN_CONTENT_XML :
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "unsupported input_type");
    // todo
    break;
  }
  GRN_API_RETURN(ctx->rc);
}

/* grn_expr_parse */

grn_obj *
grn_ptr_value_at(grn_obj *obj, int offset)
{
  int size = GRN_BULK_VSIZE(obj) / sizeof(grn_obj *);
  if (offset < 0) { offset = size + offset; }
  return (0 <= offset && offset < size)
    ? (((grn_obj **)GRN_BULK_HEAD(obj))[offset])
    : NULL;
}

int32_t
grn_int32_value_at(grn_obj *obj, int offset)
{
  int size = GRN_BULK_VSIZE(obj) / sizeof(int32_t);
  if (offset < 0) { offset = size + offset; }
  return (0 <= offset && offset < size)
    ? (((int32_t *)GRN_BULK_HEAD(obj))[offset])
    : 0;
}

#include "expr.h"
#include "expr.c"

static grn_rc
grn_expr_parser_open(grn_ctx *ctx)
{
  if (!ctx->impl->parser) {
    yyParser *pParser = GRN_MALLOCN(yyParser, 1);
    if (pParser) {
      pParser->yyidx = -1;
#if YYSTACKDEPTH<=0
      yyGrowStack(pParser);
#endif
      ctx->impl->parser = pParser;
    }
  }
  return ctx->rc;
}

#define PARSE(token) grn_expr_parser(ctx->impl->parser, (token), 0, q)

static grn_rc
get_word_(grn_ctx *ctx, efs_info *q)
{
  grn_operator mode;
  const char *start = q->cur, *end;
  unsigned int len;
  for (end = q->cur;; ) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, end, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    if (grn_isspace(end, ctx->encoding) ||
        *end == GRN_QUERY_PARENL || *end == GRN_QUERY_PARENR) {
      q->cur = end;
      break;
    }
    if (q->flags & GRN_EXPR_ALLOW_COLUMN && *end == GRN_QUERY_COLUMN) {
      grn_obj *c = grn_obj_column(ctx, q->table, start, end - start);
      if (c && end + 1 < q->str_end) {
        //        efs_op op;
        switch (end[1]) {
        case '!' :
          mode = GRN_OP_NOT_EQUAL;
          q->cur = end + 2;
          break;
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            mode = GRN_OP_ASSIGN;
            q->cur = end + 2;
          } else {
            mode = GRN_OP_EQUAL;
            q->cur = end + 1;
          }
          break;
        case '<' :
          if (end + 2 < q->str_end && end[2] == '=') {
            mode = GRN_OP_LESS_EQUAL;
            q->cur = end + 3;
          } else {
            mode = GRN_OP_LESS;
            q->cur = end + 2;
          }
          break;
        case '>' :
          if (end + 2 < q->str_end && end[2] == '=') {
            mode = GRN_OP_GREATER_EQUAL;
            q->cur = end + 3;
          } else {
            mode = GRN_OP_GREATER;
            q->cur = end + 2;
          }
          break;
        case '@' :
        case '%' : /* deprecated */
          mode = GRN_OP_MATCH;
          q->cur = end + 2;
          break;
        default :
          mode = GRN_OP_EQUAL;
          q->cur = end + 1;
          break;
        }
      } else {
        ERR(GRN_INVALID_ARGUMENT, "column lookup failed");
        q->cur = q->str_end;
        return ctx->rc;
      }
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      PARSE(GRN_EXPR_TOKEN_RELATIVE_OP);

      GRN_PTR_PUT(ctx, &((grn_expr *)(q->e))->objs, c);
      GRN_PTR_PUT(ctx, &q->column_stack, c);
      GRN_INT32_PUT(ctx, &q->mode_stack, mode);

      return GRN_SUCCESS;
    } else if (*end == GRN_QUERY_PREFIX) {
      mode = GRN_OP_PREFIX;
      q->cur = end + 1;
      break;
    }
    end += len;
  }
  GRN_PTR_PUT(ctx, &q->token_stack, grn_expr_add_str(ctx, q->e, start, end - start));

  PARSE(GRN_EXPR_TOKEN_QSTRING);
{
  grn_obj *column, *token;
  efs_info *efsi = q;
  GRN_PTR_POP(&efsi->token_stack, token);
  column = grn_ptr_value_at(&efsi->column_stack, -1);
  grn_expr_append_const(efsi->ctx, efsi->e, column, GRN_OP_GET_VALUE, 1);
  grn_expr_append_obj(efsi->ctx, efsi->e, token, GRN_OP_PUSH, 1);
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->mode_stack, -1), 2);
}
  return GRN_SUCCESS;
}

static grn_rc
parse_query(grn_ctx *ctx, efs_info *q)
{
  int option = 0;
  grn_operator mode;
  efs_op op_, *op = &op_;
  op->op = q->default_op;
  op->weight = DEFAULT_WEIGHT;
  for (;;) {
    skip_space(ctx, q);
    if (q->cur >= q->str_end) { goto exit; }
    switch (*q->cur) {
    case '\0' :
      goto exit;
      break;
    case GRN_QUERY_PARENR :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_PARENR);
      break;
    case GRN_QUERY_QUOTEL :
      q->cur++;

      {
        const char *start, *s;
        start = s = q->cur;
        GRN_BULK_REWIND(&q->buf);
        while (1) {
          unsigned int len;
          if (s >= q->str_end) {
            q->cur = s;
            break;
          }
          len = grn_charlen(ctx, s, q->str_end);
          if (len == 0) {
            /* invalid string containing malformed multibyte char */
            goto exit;
          } else if (len == 1) {
            if (*s == GRN_QUERY_QUOTER) {
              q->cur = s + 1;
              break;
            } else if (*s == GRN_QUERY_ESCAPE && s + 1 < q->str_end) {
              s++;
              len = grn_charlen(ctx, s, q->str_end);
            }
          }
          GRN_TEXT_PUT(ctx, &q->buf, s, len);
          s += len;

        }
        GRN_PTR_PUT(ctx, &q->token_stack, grn_expr_add_str(ctx, q->e,
                                                           GRN_TEXT_VALUE(&q->buf),
                                                           GRN_TEXT_LEN(&q->buf)));
          PARSE(GRN_EXPR_TOKEN_QSTRING);
{
  grn_obj *column, *token;
  efs_info *efsi = q;
  GRN_PTR_POP(&efsi->token_stack, token);
  column = grn_ptr_value_at(&efsi->column_stack, -1);
  grn_expr_append_const(efsi->ctx, efsi->e, column, GRN_OP_GET_VALUE, 1);
  grn_expr_append_obj(efsi->ctx, efsi->e, token, GRN_OP_PUSH, 1);
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->mode_stack, -1), 2);
}

      }

      break;
    case GRN_QUERY_PREFIX :
      q->cur++;
      get_op(q, op, &mode, &option);
      PARSE(GRN_EXPR_TOKEN_MATCH);
      break;
    case GRN_QUERY_AND :
      q->cur++;
      op->op = GRN_OP_AND;
      PARSE(GRN_EXPR_TOKEN_LOGICAL_AND);
      break;
    case GRN_QUERY_BUT :
      q->cur++;
      op->op = GRN_OP_BUT;
      PARSE(GRN_EXPR_TOKEN_LOGICAL_BUT);
      break;
    case GRN_QUERY_ADJ_INC :
      q->cur++;
      if (op->weight < 127) { op->weight++; }
      op->op = GRN_OP_ADJUST;
      PARSE(GRN_EXPR_TOKEN_ADJUST);
      break;
    case GRN_QUERY_ADJ_DEC :
      q->cur++;
      if (op->weight > -128) { op->weight--; }
      op->op = GRN_OP_ADJUST;
      PARSE(GRN_EXPR_TOKEN_ADJUST);
      break;
    case GRN_QUERY_ADJ_NEG :
      q->cur++;
      op->op = GRN_OP_ADJUST;
      op->weight = -1;
      PARSE(GRN_EXPR_TOKEN_ADJUST);
      break;
    case GRN_QUERY_PARENL :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_PARENL);
      break;
    case '=' :
    case 'O' :
      if (q->cur[1] == 'R' && q->cur[2] == ' ') {
        q->cur += 2;
        PARSE(GRN_EXPR_TOKEN_LOGICAL_OR);
        break;
      }
      /* fallthru */
    default :
      get_word_(ctx, q);
      break;
    }
  }
exit :
  PARSE(0);
  return GRN_SUCCESS;
}

static grn_rc
get_string(grn_ctx *ctx, efs_info *q)
{
  const char *s;
  unsigned int len;
  grn_rc rc = GRN_END_OF_DATA;
  GRN_BULK_REWIND(&q->buf);
  for (s = q->cur + 1; s < q->str_end; s += len) {
    if (!(len = grn_charlen(ctx, s, q->str_end))) { break; }
    if (len == 1) {
      if (*s == GRN_QUERY_QUOTER) {
        s++;
        rc = GRN_SUCCESS;
        break;
      }
      if (*s == GRN_QUERY_ESCAPE && s + 1 < q->str_end) {
        s++;
        if (!(len = grn_charlen(ctx, s, q->str_end))) { break; }
      }
    }
    GRN_TEXT_PUT(ctx, &q->buf, s, len);
  }
  q->cur = s;
  return rc;
}

static grn_rc
get_identifier(grn_ctx *ctx, efs_info *q)
{
  const char *s;
  unsigned int len;
  grn_rc rc = GRN_SUCCESS;
  for (s = q->cur; s < q->str_end; s += len) {
    if (!(len = grn_charlen(ctx, s, q->str_end))) {
      rc = GRN_END_OF_DATA;
      goto exit;
    }
    if (grn_isspace(s, ctx->encoding)) { goto done; }
    if (len == 1) {
      switch (*s) {
      case '\0' : case '(' : case ')' : case '{' : case '}' :
      case '[' : case ']' : case ',' : case ':' : case '@' :
      case '?' : case '"' : case '*' : case '+' : case '-' :
      case '|' : case '/' : case '%' : case '!' : case '^' :
      case '&' : case '>' : case '<' : case '=' : case '~' :
        /* case '.' : */
        goto done;
        break;
      }
    }
  }
done :
  switch (*q->cur) {
  case 'd' :
    if (len == 6 && !memcmp(q->cur, "delete", 6)) {
      PARSE(GRN_EXPR_TOKEN_DELETE);
      goto exit;
    }
    break;
  case 'f' :
    if (len == 6 && !memcmp(q->cur, "false", 6)) {
      PARSE(GRN_EXPR_TOKEN_BOOLEAN);
      goto exit;
    }
    break;
  case 'i' :
    if (len == 2 && !memcmp(q->cur, "in", 2)) {
      PARSE(GRN_EXPR_TOKEN_IN);
      goto exit;
    }
    break;
  case 'n' :
    if (len == 4 && !memcmp(q->cur, "null", 4)) {
      PARSE(GRN_EXPR_TOKEN_NULL);
      goto exit;
    }
    break;
  case 't' :
    if (len == 4 && !memcmp(q->cur, "true", 4)) {
      PARSE(GRN_EXPR_TOKEN_BOOLEAN);
      goto exit;
    }
    break;
  }
  {
    grn_obj *obj;
    const char *name = q->cur;
    unsigned name_size = s - q->cur;
    if ((obj = grn_expr_get_var(ctx, q->e, name, name_size))) {
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      grn_expr_append_obj(ctx, q->e, obj, GRN_OP_PUSH, 1);
      goto exit;
    }
    if ((obj = grn_obj_column(ctx, q->table, name, name_size))) {
      GRN_PTR_PUT(ctx, &((grn_expr *)q->e)->objs, obj);
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      grn_expr_append_obj(ctx, q->e, obj, GRN_OP_GET_VALUE, 1);
      goto exit;
    }
    if ((obj = grn_ctx_get(ctx, name, name_size))) {
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      grn_expr_append_obj(ctx, q->e, obj, GRN_OP_PUSH, 1);
      goto exit;
    }
    rc = GRN_SYNTAX_ERROR;
  }
exit :
  q->cur = s;
  return rc;
}

static void
set_tos_minor_to_curr(grn_ctx *ctx, efs_info *q)
{
  yyParser *pParser = ctx->impl->parser;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];
  yytos->minor.yy0 = ((grn_expr *)(q->e))->codes_curr;
}

static grn_rc
parse_script(grn_ctx *ctx, efs_info *q)
{
  grn_rc rc = GRN_SUCCESS;
  for (;;) {
    skip_space(ctx, q);
    if (q->cur >= q->str_end) { rc = GRN_END_OF_DATA; goto exit; }
    switch (*q->cur) {
    case '\0' :
      rc = GRN_END_OF_DATA;
      goto exit;
      break;
    case '(' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_PARENL);
      break;
    case ')' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_PARENR);
      break;
    case '{' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_BRACEL);
      break;
    case '}' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_BRACER);
      break;
    case '[' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_BRACKETL);
      break;
    case ']' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_BRACKETR);
      break;
    case ',' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_COMMA);
      break;
    case '.' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_DOT);
      break;
    case ':' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_COLON);
      set_tos_minor_to_curr(ctx, q);
      grn_expr_append_op(ctx, q->e, GRN_OP_JUMP, 0);
      break;
    case '@' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_MATCH);
      break;
    case '~' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_BITWISE_NOT);
      break;
    case '?' :
      q->cur++;
      PARSE(GRN_EXPR_TOKEN_QUESTION);
      set_tos_minor_to_curr(ctx, q);
      grn_expr_append_op(ctx, q->e, GRN_OP_CJUMP, 0);
      break;
    case '"' :
      if ((rc = get_string(ctx, q))) { goto exit; }
      PARSE(GRN_EXPR_TOKEN_STRING);
      grn_expr_append_const(ctx, q->e, &q->buf, GRN_OP_PUSH, 1);
      break;
    case '*' :
      q->cur++;
      switch (*q->cur) {
      case 'N' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_NEAR);
        break;
      case 'S' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_SIMILAR);
        break;
      case 'T' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_TERM_EXTRACT);
        break;
      case '>' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_ADJUST);
        break;
      case '<' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_ADJUST);
        break;
      case '~' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_ADJUST);
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_STAR_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'*=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_STAR);
        break;
      }
      break;
    case '+' :
      q->cur++;
      switch (*q->cur) {
      case '+' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_INCR);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'++' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_PLUS_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'+=' is not allowed (%.*s)", q->str_end - q->str, q->str);
          goto exit;
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_PLUS);
        break;
      }
      break;
    case '-' :
      q->cur++;
      switch (*q->cur) {
      case '-' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_DECR);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'--' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_MINUS_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'-=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_MINUS);
        break;
      }
      break;
    case '|' :
      q->cur++;
      switch (*q->cur) {
      case '|' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_LOGICAL_OR);
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_OR_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'|=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_BITWISE_OR);
        break;
      }
      break;
    case '/' :
      q->cur++;
      switch (*q->cur) {
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_SLASH_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'/=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_SLASH);
        break;
      }
      break;
    case '%' :
      q->cur++;
      switch (*q->cur) {
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_MOD_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'%%=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_MOD);
        break;
      }
      break;
    case '!' :
      q->cur++;
      switch (*q->cur) {
      case '=' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_NOT_EQUAL);
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_NOT);
        break;
      }
      break;
    case '^' :
      q->cur++;
      switch (*q->cur) {
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_XOR_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'^=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_BITWISE_XOR);
        break;
      }
      break;
    case '&' :
      q->cur++;
      switch (*q->cur) {
      case '&' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_LOGICAL_AND);
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur++;
          PARSE(GRN_EXPR_TOKEN_AND_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'&=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      case '!' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_LOGICAL_BUT);
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_BITWISE_AND);
        break;
      }
      break;
    case '>' :
      q->cur++;
      switch (*q->cur) {
      case '>' :
        q->cur++;
        switch (*q->cur) {
        case '>' :
          q->cur++;
          switch (*q->cur) {
          case '=' :
            if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
              q->cur++;
              PARSE(GRN_EXPR_TOKEN_SHIFTRR_ASSIGN);
            } else {
              ERR(GRN_UPDATE_NOT_ALLOWED,
                  "'>>>=' is not allowed (%.*s)", q->str_end - q->str, q->str);
            }
            break;
          default :
            PARSE(GRN_EXPR_TOKEN_SHIFTRR);
            break;
          }
          break;
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            q->cur++;
            PARSE(GRN_EXPR_TOKEN_SHIFTR_ASSIGN);
          } else {
            ERR(GRN_UPDATE_NOT_ALLOWED,
                "'>>=' is not allowed (%.*s)", q->str_end - q->str, q->str);
          }
          break;
        default :
          PARSE(GRN_EXPR_TOKEN_SHIFTR);
          break;
        }
        break;
      case '=' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_GREATER_EQUAL);
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_GREATER);
        break;
      }
      break;
    case '<' :
      q->cur++;
      switch (*q->cur) {
      case '<' :
        q->cur++;
        switch (*q->cur) {
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            q->cur++;
            PARSE(GRN_EXPR_TOKEN_SHIFTL_ASSIGN);
          } else {
            ERR(GRN_UPDATE_NOT_ALLOWED,
                "'<<=' is not allowed (%.*s)", q->str_end - q->str, q->str);
          }
          break;
        default :
          PARSE(GRN_EXPR_TOKEN_SHIFTL);
          break;
        }
        break;
      case '=' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_LESS_EQUAL);
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_LESS);
        break;
      }
      break;
    case '=' :
      q->cur++;
      switch (*q->cur) {
      case '=' :
        q->cur++;
        PARSE(GRN_EXPR_TOKEN_EQUAL);
        break;
      default :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'=' is not allowed (%.*s)", q->str_end - q->str, q->str);
        }
        break;
      }
      break;
    case '0' : case '1' : case '2' : case '3' : case '4' :
    case '5' : case '6' : case '7' : case '8' : case '9' :
      {
        const char *rest;
        int64_t int64 = grn_atoll(q->cur, q->str_end, &rest);
        // checks to see grn_atoll was appropriate
        // (NOTE: *q->cur begins with a digit. Thus, grn_atoll parses at leaset
        //        one char.)
        if (q->str_end != rest &&
            (*rest == '.' || *rest == 'e' || *rest == 'E' ||
             (*rest >= '0' && *rest <= '9'))) {
          char *rest_float;
          double d = strtod(q->cur, &rest_float);
          grn_obj floatbuf;
          GRN_FLOAT_INIT(&floatbuf, 0);
          GRN_FLOAT_SET(ctx, &floatbuf, d);
          grn_expr_append_const(ctx, q->e, &floatbuf, GRN_OP_PUSH, 1);
          rest = rest_float;
        } else {
          const char *rest64 = rest;
          unsigned int uint32 = grn_atoui(q->cur, q->str_end, &rest);
          // checks to see grn_atoi failed (see above NOTE)
          if (q->str_end != rest && *rest >= '0' && *rest <= '9') {
            grn_obj int64buf;
            GRN_INT64_INIT(&int64buf, 0);
            GRN_INT64_SET(ctx, &int64buf, int64);
            grn_expr_append_const(ctx, q->e, &int64buf, GRN_OP_PUSH, 1);
            rest = rest64;
          } else {
            grn_obj uint32buf;
            GRN_UINT32_INIT(&uint32buf, 0);
            GRN_UINT32_SET(ctx, &uint32buf, uint32);
            grn_expr_append_const(ctx, q->e, &uint32buf, GRN_OP_PUSH, 1);
          }
        }
        q->cur = rest;
        PARSE(GRN_EXPR_TOKEN_DECIMAL);
      }
      break;
    default :
      if ((rc = get_identifier(ctx, q))) { goto exit; }
      break;
    }
    if (ctx->rc) { rc = ctx->rc; break; }
  }
exit :
  PARSE(0);
  return rc;
}

grn_rc
grn_expr_parse(grn_ctx *ctx, grn_obj *expr,
               const char *str, unsigned str_size,
               grn_obj *default_column, grn_operator default_mode,
               grn_operator default_op, grn_expr_flags flags)
{
  efs_info efsi;
  if (grn_expr_parser_open(ctx)) { return ctx->rc; }
  GRN_API_ENTER;
  efsi.ctx = ctx;
  efsi.str = str;
  if ((efsi.v = grn_expr_get_var_by_offset(ctx, expr, 0)) &&
      (efsi.table = grn_ctx_at(ctx, efsi.v->header.domain))) {
    GRN_TEXT_INIT(&efsi.buf, 0);
    GRN_UINT32_INIT(&efsi.op_stack, GRN_OBJ_VECTOR);
    GRN_UINT32_INIT(&efsi.mode_stack, GRN_OBJ_VECTOR);
    GRN_PTR_INIT(&efsi.column_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
    GRN_PTR_INIT(&efsi.token_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
    efsi.e = expr;
    efsi.str = str;
    efsi.cur = str;
    efsi.str_end = str + str_size;
    efsi.default_column = default_column;
    GRN_PTR_PUT(ctx, &efsi.column_stack, default_column);
    GRN_INT32_PUT(ctx, &efsi.op_stack, default_op);
    GRN_INT32_PUT(ctx, &efsi.mode_stack, default_mode);
    efsi.default_flags = efsi.flags = flags;
    efsi.escalation_threshold = GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD;
    efsi.escalation_decaystep = DEFAULT_DECAYSTEP;
    efsi.weight_offset = 0;
    efsi.opt.weight_vector = NULL;
    efsi.weight_set = NULL;

    if (flags & GRN_EXPR_SYNTAX_SCRIPT) {
      parse_script(ctx, &efsi);
    } else {
      parse_query(ctx, &efsi);
    }

    /*
        grn_obj strbuf;
        GRN_TEXT_INIT(&strbuf, 0);
        grn_expr_inspect(ctx, &strbuf, expr);
        GRN_TEXT_PUTC(ctx, &strbuf, '\0');
        GRN_LOG(ctx, GRN_LOG_NOTICE, "query=(%s)", GRN_TEXT_VALUE(&strbuf));
        GRN_OBJ_FIN(ctx, &strbuf);
    */

    /*
    efsi.opt.vector_size = DEFAULT_WEIGHT_VECTOR_SIZE;
    efsi.opt.func = efsi.weight_set ? section_weight_cb : NULL;
    efsi.opt.func_arg = efsi.weight_set;
    efsi.snip_conds = NULL;
    */
    GRN_OBJ_FIN(ctx, &efsi.op_stack);
    GRN_OBJ_FIN(ctx, &efsi.mode_stack);
    GRN_OBJ_FIN(ctx, &efsi.column_stack);
    GRN_OBJ_FIN(ctx, &efsi.token_stack);
    GRN_OBJ_FIN(ctx, &efsi.buf);
  } else {
    ERR(GRN_INVALID_ARGUMENT, "variable is not defined correctly");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_expr_parser_close(grn_ctx *ctx)
{
  if (ctx->impl->parser) {
    yyParser *pParser = (yyParser*)ctx->impl->parser;
    while (pParser->yyidx >= 0) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
    free(pParser->yystack);
#endif
    GRN_FREE(pParser);
    ctx->impl->parser = NULL;
  }
  return ctx->rc;
}

grn_snip *
grn_expr_snip(grn_ctx *ctx, grn_obj *expr, int flags,
              unsigned int width, unsigned int max_results,
              unsigned int n_tags,
              const char **opentags, unsigned int *opentag_lens,
              const char **closetags, unsigned int *closetag_lens,
              grn_snip_mapping *mapping)
{
  int i, n;
  scan_info **sis, *si;
  grn_snip *res = NULL;
  GRN_API_ENTER;
  if ((sis = scan_info_build(ctx, expr, &n, GRN_OP_OR, 0))) {
    if ((res = grn_snip_open(ctx, flags, width, max_results,
                           NULL, 0, NULL, 0, mapping))) {
      int butp = 0, nparens = 0, npbut = 0;
      grn_obj but_stack;
      grn_obj snip_stack;
      GRN_UINT32_INIT(&but_stack, GRN_OBJ_VECTOR);
      GRN_PTR_INIT(&snip_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
      for (i = n; i--;) {
        si = sis[i];
        if (si->flags & SCAN_POP) {
          nparens++;
          if (si->logical_op == GRN_OP_BUT) {
            GRN_UINT32_PUT(ctx, &but_stack, npbut);
            npbut = nparens;
            butp = 1 - butp;
          }
        } else {
          if (si->op == GRN_OP_MATCH && si->query) {
            if (butp == (si->logical_op == GRN_OP_BUT)) {
              GRN_PTR_PUT(ctx, &snip_stack, si->query);
            }
          }
          if (si->flags & SCAN_PUSH) {
            if (nparens == npbut) {
              butp = 1 - butp;
              GRN_UINT32_POP(&but_stack, npbut);
            }
            nparens--;
          }
        }
      }
      if (n_tags) {
        for (i = 0;; i = (i + 1) % n_tags) {
          grn_obj *q;
          GRN_PTR_POP(&snip_stack, q);
          if (!q) { break; }
          grn_snip_add_cond(ctx, res, GRN_TEXT_VALUE(q), GRN_TEXT_LEN(q),
                            opentags[i], opentag_lens[i], closetags[i], closetag_lens[i]);
        }
      } else {
        for (;;) {
          grn_obj *q;
          GRN_PTR_POP(&snip_stack, q);
          if (!q) { break; }
          grn_snip_add_cond(ctx, res, GRN_TEXT_VALUE(q), GRN_TEXT_LEN(q),
                            NULL, 0, NULL, 0);
        }
      }
      GRN_OBJ_FIN(ctx, &but_stack);
      GRN_OBJ_FIN(ctx, &snip_stack);
    }
    for (i = n; i--;) { SI_FREE(sis[i]); }
    GRN_FREE(sis);
  }
  GRN_API_RETURN(res);
}

grn_rc
grn_normalize_offset_and_limit(grn_ctx *ctx, int size, int *p_offset, int *p_limit)
{
  int end;
  int offset = *p_offset;
  int limit = *p_limit;

  if (offset < 0) {
    offset += size;
    if (offset < 0) {
      ERR(GRN_INVALID_ARGUMENT, "too small offset");
      return ctx->rc;
    }
  } else if (offset != 0 && offset >= size) {
    ERR(GRN_INVALID_ARGUMENT, "too large offset");
    return ctx->rc;
  }

  if (limit < 0) {
    limit += size + 1;
    if (limit < 0) {
      ERR(GRN_INVALID_ARGUMENT, "too small limit");
      return ctx->rc;
    }
  } else if (limit > size) {
    limit = size;
  }

  /* At this point, offset and limit must be zero or positive. */
  end = offset + limit;
  if (end > size) {
    limit -= end - size;
  }
  *p_offset = offset;
  *p_limit = limit;
  return GRN_SUCCESS;
}

void
grn_p(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj buffer;

  if (!obj) {
    printf("(NULL)\n");
    return;
  }

  GRN_TEXT_INIT(&buffer, 0);
  if (obj->header.type == GRN_EXPR) {
    grn_expr_inspect(ctx, &buffer, obj);
  } else {
    grn_text_otoj(ctx, &buffer, obj, NULL);
  }
  printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));
  grn_obj_unlink(ctx, &buffer);
}
