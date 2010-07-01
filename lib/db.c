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
#include "module.h"
#include "util.h"
#include <string.h>
#include <float.h>

#define NEXT_ADDR(p) (((byte *)(p)) + sizeof *(p))

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

typedef struct {
  grn_obj *ptr;
  uint32_t lock;
  uint32_t done;
} db_value;

grn_obj *
grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg)
{
  grn_db *s;
  GRN_API_ENTER;
  if (!path || strlen(path) <= PATH_MAX - 14) {
    if ((s = GRN_MALLOC(sizeof(grn_db)))) {
      grn_tiny_array_init(ctx, &s->values, sizeof(db_value),
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
  grn_ctx *ctx_ = ctx;
  GRN_API_ENTER;
  if (path && strlen(path) <= PATH_MAX - 14) {
    if ((s = GRN_MALLOC(sizeof(grn_db)))) {
      grn_tiny_array_init(ctx, &s->values, sizeof(db_value),
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
          grn_ctx_use(ctx_, (grn_obj *)s);
#ifdef WITH_MECAB
          grn_db_init_mecab_tokenizer(ctx);
#endif
          grn_db_init_builtin_tokenizers(ctx);
          grn_db_init_builtin_query(ctx);
          GRN_API_RETURN((grn_obj *)s);
        }
        grn_pat_close(ctx, s->keys);
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
  db_value *vp;
  grn_db *s = (grn_db *)db;
  if (!s) { return GRN_INVALID_ARGUMENT; }
  GRN_API_ENTER;
  GRN_TINY_ARRAY_EACH(&s->values, 1, grn_pat_curr_id(ctx, s->keys), id, vp, {
    if (vp->ptr) { grn_obj_close(ctx, vp->ptr); }
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
  if (ctx->impl && ctx->impl->db == db) {
    grn_cache_expire(-1);
    ctx->impl->db = NULL;
  }
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
  if (GRN_DB_P(db)) {
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

grn_obj *
grn_db_keys(grn_obj *s)
{
  return (grn_obj *)(((grn_db *)s)->keys);
}

uint32_t
grn_db_lastmod(grn_obj *s)
{
  return (((grn_db *)s)->keys)->io->header->lastmod;
}

void
grn_db_touch(grn_ctx *ctx, grn_obj *s)
{
  grn_timeval tv;
  grn_timeval_now(ctx, &tv);
  (((grn_db *)s)->keys)->io->header->lastmod = tv.tv_sec;
}

#define IS_TEMP(obj) (DB_OBJ(obj)->id & GRN_OBJ_TMP_OBJECT)

void
grn_obj_touch(grn_ctx *ctx, grn_obj *obj, grn_timeval *tv)
{
  grn_timeval tv_;
  if (!tv) {
    grn_timeval_now(ctx, &tv_);
    tv = &tv_;
  }
  if (obj) {
    switch (obj->header.type) {
    case GRN_DB :
      ((grn_db *)obj)->keys->io->header->lastmod = tv->tv_sec;
      break;
    case GRN_TABLE_PAT_KEY :
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_NO_KEY :
    case GRN_COLUMN_VAR_SIZE :
    case GRN_COLUMN_FIX_SIZE :
    case GRN_COLUMN_INDEX :
      if (!IS_TEMP(obj)) {
        ((grn_db *)DB_OBJ(obj)->db)->keys->io->header->lastmod = tv->tv_sec;
      }
      break;
    }
  }
}

grn_rc
grn_db_check_name(grn_ctx *ctx, const char *name, unsigned int name_size)
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

#define GRN_TYPE_SIZE(type) ((type)->range)

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
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!GRN_DB_P(db)) {
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
grn_proc_create(grn_ctx *ctx, const char *name, unsigned name_size, grn_proc_type type,
                grn_proc_func *init, grn_proc_func *next, grn_proc_func *fin,
                unsigned nvars, grn_expr_var *vars)
{
  grn_proc *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id range;
  int added = 0;
  grn_obj *db;
  const char *path = ctx->impl->module_path;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  range = path ? grn_module_get(ctx, path) : GRN_ID_NIL;
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!GRN_DB_P(db)) {
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
      db_value *vp;
      if ((vp = grn_tiny_array_at(&s->values, id)) && (res = (grn_proc *)vp->ptr)) {
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
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR();
    GRN_API_RETURN(NULL);
  }
  if (!GRN_DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  }
  if (key_type) {
    if ((flags & GRN_OBJ_TABLE_TYPE_MASK) == GRN_OBJ_TABLE_NO_KEY) {
      if (name_size > 0) {
        ERR(GRN_INVALID_ARGUMENT,
            "key_type assigned for no key table: <%.*s>", name_size, name);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "key_type assigned for no key table");
      }
      GRN_API_RETURN(NULL);
    }
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
    GRN_LOG(ctx, GRN_LOG_NOTICE, "DDL:table_create %.*s", name_size, name);
    if (!path) {
      if (GRN_DB_PERSISTENT_P(db)) {
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
    if (GRN_DB_PERSISTENT_P(db) && name && name_size) {
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
  if (!GRN_DB_P(db)) {
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

struct _grn_hook {
  grn_hook *next;
  grn_proc *proc;
  uint32_t hld_size;
};

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
    if (target) {
      switch (target->header.type) {
      case GRN_COLUMN_INDEX :
        grn_ii_column_update(ctx, (grn_ii *)target,
                             GRN_UINT32_VALUE(id),
                             section, oldvalue, newvalue, NULL);
      }
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
      res->limit = -1;
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
grn_table_at(grn_ctx *ctx, grn_obj *table, grn_id id)
{
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      id = grn_pat_at(ctx, (grn_pat *)table, id);
      break;
    case GRN_TABLE_HASH_KEY :
      id = grn_hash_at(ctx, (grn_hash *)table, id);
      break;
    case GRN_TABLE_NO_KEY :
      id = grn_array_at(ctx, (grn_array *)table, id);
      break;
    default :
      id = GRN_ID_NIL;
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
call_delete_hook(grn_ctx *ctx, grn_obj *table, grn_id rid, const void *key, unsigned key_size)
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
      GRN_TEXT_SET_REF(&oldvalue_, key, key_size);
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
      call_delete_hook(ctx, table, rid, key, key_size);
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
    grn_obj_touch(ctx, table, NULL);
  }
  GRN_API_RETURN(rc);
}

grn_rc
_grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id,
                       grn_table_delete_optarg *optarg)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  if (table) {
    const void *key;
    unsigned key_size;
    if ((key = _grn_table_key(ctx, table, id, &key_size))) {
      call_delete_hook(ctx, table, id, key, key_size);
    }
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
    case GRN_TABLE_VIEW :
      io = ((grn_view *)obj)->hash->io;
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
  grn_obj_touch(ctx, table, NULL);
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_truncate(grn_ctx *ctx, grn_obj *table)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;

  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "grn_table_truncate() is not implemented.");
  rc = GRN_FUNCTION_NOT_IMPLEMENTED;
#if 0
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
    grn_obj_touch(ctx, table, NULL);
  }
#endif
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
      if (v->limit >= 0 && v->limit < vc->rest) { vc->rest = v->limit; }
      return vc;
    }
    GRN_FREE(vc);
  }
  return NULL;
}

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
      break;
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
      if (DB_OBJ(obj)->range == res->header.domain) {
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

static int
accelerated_table_group(grn_ctx *ctx, grn_obj *table, grn_obj *key, grn_obj *res)
{
  if (key->header.type == GRN_ACCESSOR) {
    grn_accessor *a = (grn_accessor *)key;
    if (a->action == GRN_ACCESSOR_GET_KEY &&
        a->next && a->next->action == GRN_ACCESSOR_GET_COLUMN_VALUE &&
        a->next->obj && !a->next->next) {
      grn_obj *range = grn_ctx_at(ctx, grn_obj_get_range(ctx, key));
      int idp = GRN_OBJ_TABLEP(range);
      grn_table_cursor *tc;
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
        switch (a->next->obj->header.type) {
        case GRN_COLUMN_FIX_SIZE :
          {
            grn_id id;
            grn_ra *ra = (grn_ra *)a->next->obj;
            unsigned element_size = (ra)->header->element_size;
            grn_ra_cache cache;
            GRN_RA_CACHE_INIT(ra, &cache);
            while ((id = grn_table_cursor_next(ctx, tc))) {
              void *v, *value;
              grn_id *id_;
              uint32_t key_size;
              grn_rset_recinfo *ri = NULL;
              if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
                grn_table_cursor_get_value(ctx, tc, (void **)&ri);
              }
              id_ = (grn_id *)_grn_table_key(ctx, table, id, &key_size);
              v = grn_ra_ref_cache(ctx, ra, *id_, &cache);
              if (idp && *((grn_id *)v) &&
                  grn_table_at(ctx, range, *((grn_id *)v)) == GRN_ID_NIL) {
                continue;
              }
              if ((!idp || *((grn_id *)v)) &&
                  grn_table_add_v(ctx, res, v, element_size, &value, NULL)) {
                grn_table_add_subrec(res, value, ri ? ri->score : 0, NULL, 0);
              }
            }
            GRN_RA_CACHE_FIN(ra, &cache);
          }
          break;
        case GRN_COLUMN_VAR_SIZE :
          if (idp) { /* todo : support other type */
            grn_id id;
            grn_ja *ja = (grn_ja *)a->next->obj;
            while ((id = grn_table_cursor_next(ctx, tc))) {
              grn_io_win jw;
              unsigned int len = 0;
              void *value;
              grn_id *v, *id_;
              uint32_t key_size;
              grn_rset_recinfo *ri = NULL;
              if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
                grn_table_cursor_get_value(ctx, tc, (void **)&ri);
              }
              id_ = (grn_id *)_grn_table_key(ctx, table, id, &key_size);
              if ((v = grn_ja_ref(ctx, ja, *id_, &jw, &len))) {
                while (len) {
                  if ((*v != GRN_ID_NIL) &&
                      grn_table_add_v(ctx, res, v, sizeof(grn_id), &value, NULL)) {
                    grn_table_add_subrec(res, value, ri ? ri->score : 0, NULL, 0);
                  }
                  v++;
                  len -= sizeof(grn_id);
                }
                grn_ja_unref(ctx, &jw);
              }
            }
          } else {
            return 0;
          }
          break;
        default :
          return 0;
        }
        grn_table_cursor_close(ctx, tc);
        return 1;
      }
    }
  }
  return 0;
}

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
      if (!accelerated_table_group(ctx, table, keys->key, results->table)) {
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

grn_obj *
grn_obj_column(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size)
{
  grn_obj *column = NULL;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table)) {
    if (table->header.type == GRN_TABLE_VIEW) {
      column = grn_view_get_accessor(ctx, table, name, name_size);
    } else {
      if (grn_db_check_name(ctx, name, name_size) ||
          !(column = grn_obj_column_(ctx, table, name, name_size))) {
        column = grn_obj_get_accessor(ctx, table, name, name_size);
      }
    }
  } else if (GRN_ACCESSORP(table)) {
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
  case GRN_DB :
    return _grn_pat_key(ctx, ((grn_db *)table)->keys, id, key_size);
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
  if (!GRN_DB_P(s)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    goto exit;
  }
  if (DB_OBJ(table)->id & GRN_OBJ_TMP_OBJECT) {
    ERR(GRN_INVALID_ARGUMENT, "temporary table doesn't support column");
    goto exit;
  }
  {
    uint32_t s = 0;
    const char *n = _grn_table_key(ctx, ctx->impl->db, DB_OBJ(table)->id, &s);
    GRN_LOG(ctx, GRN_LOG_NOTICE, "DDL:column_create %.*s %.*s", s, n, name_size, name);
  }
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR();
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
      if (GRN_DB_PERSISTENT_P(db)) {
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
    grn_obj_touch(ctx, res, NULL);
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
  if (!GRN_DB_P(s)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    goto exit;
  }
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR();
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

unsigned int
grn_vector_pop_element(grn_ctx *ctx, grn_obj *vector,
                       const char **str, unsigned int *weight, grn_id *domain)
{
  unsigned int offset, length = 0;
  GRN_API_ENTER;
  if (!vector || vector->header.type != GRN_VECTOR) {
    ERR(GRN_INVALID_ARGUMENT, "invalid vector");
    goto exit;
  }
  if (!vector->u.v.n_sections) {
    ERR(GRN_RANGE_ERROR, "offset out of range");
    goto exit;
  }
  offset = --vector->u.v.n_sections;
  {
    grn_section *vp = &vector->u.v.sections[offset];
    grn_obj *body = grn_vector_body(ctx, vector);
    *str = GRN_BULK_HEAD(body) + vp->offset;
    if (weight) { *weight = vp->weight; }
    if (domain) { *domain = vp->domain; }
    length = vp->length;
    grn_bulk_truncate(ctx, body, vp->offset);
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
            if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
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
            if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
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
            if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
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
            if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              goto exit;
            }
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
          if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            goto exit;
          }
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
      case GRN_ACCESSOR_GET_VALUE :/* fix me */
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

int
grn_obj_is_persistent(grn_ctx *ctx, grn_obj *obj)
{
  int res = 0;
  if (GRN_DB_OBJP(obj)) {
    res = IS_TEMP(obj) ? 0 : 1;
  } else if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      switch (a->action) {
      case GRN_ACCESSOR_GET_SCORE :
      case GRN_ACCESSOR_GET_NSUBRECS :
        res = 0;
        break;
      case GRN_ACCESSOR_GET_ID :
      case GRN_ACCESSOR_GET_VALUE :
      case GRN_ACCESSOR_GET_COLUMN_VALUE :
      case GRN_ACCESSOR_GET_KEY :
        if (GRN_DB_OBJP(a->obj)) { res = IS_TEMP(obj) ? 0 : 1; }
        break;
      default :
        if (GRN_DB_OBJP(a->obj)) { res = IS_TEMP(obj) ? 0 : 1; }
        break;
      }
    }
  } else if (obj->header.type == GRN_ACCESSOR_VIEW) {
    /* todo */
  }
  return res;
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
  case GRN_DB_INT8 :
    NUM2DEST(GRN_INT8_VALUE, grn_text_itoa, NUM2BOOL);
    break;
  case GRN_DB_UINT8 :
    NUM2DEST(GRN_UINT8_VALUE, grn_text_lltoa, NUM2BOOL);
    break;
  case GRN_DB_INT16 :
    NUM2DEST(GRN_INT16_VALUE, grn_text_itoa, NUM2BOOL);
    break;
  case GRN_DB_UINT16 :
    NUM2DEST(GRN_UINT16_VALUE, grn_text_lltoa, NUM2BOOL);
    break;
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
        int latitude, longitude;
        const char *cur, *str = GRN_TEXT_VALUE(src);
        const char *str_end = GRN_BULK_CURR(src);
        latitude = grn_atoi(str, str_end, &cur);
        if (cur + 1 < str_end) {
          longitude = grn_atoi(cur + 1, str_end, &cur);
          if (cur == str_end) {
            GRN_GEO_POINT_SET(ctx, dest, latitude, longitude);
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
            if (GRN_BULK_VSIZE(p_key)) {
              id = addp ? grn_table_add_by_key(ctx, table, p_key, NULL)
                        : grn_table_get_by_key(ctx, table, p_key);
              if (id) { GRN_RECORD_SET(ctx, dest, id); }
            } else {
              GRN_RECORD_SET(ctx, dest, GRN_ID_NIL);
            }
            GRN_OBJ_FIN(ctx, &key);
          } else {
            grn_obj record_id;
            GRN_UINT32_INIT(&record_id, 0);
            grn_obj_cast(ctx, src, &record_id, 1);
            id = GRN_UINT32_VALUE(&record_id);
            if (id) { GRN_RECORD_SET(ctx, dest, id); }
          }
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
  case GRN_DB_INT8 :\
    if (s == sizeof(int8_t)) {\
      int8_t *vp = (int8_t *)p;\
      *vp op *(int8_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_UINT8 :\
    if (s == sizeof(uint8_t)) {\
      uint8_t *vp = (uint8_t *)p;\
      *vp op *(int8_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_INT16 :\
    if (s == sizeof(int16_t)) {\
      int16_t *vp = (int16_t *)p;\
      *vp op *(int16_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_UINT16 :\
    if (s == sizeof(uint16_t)) {\
      uint16_t *vp = (uint16_t *)p;\
      *vp op *(int16_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
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
  if (!id) { goto exit; }
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

static void
del_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, grn_obj *hld)
{
  int i;
  void *hld_value = NULL;
  uint32_t hld_size = 0;
  grn_hook **last;
  hld_value = GRN_BULK_HEAD(hld);
  hld_size = GRN_BULK_VSIZE(hld);
  if (!hld_size) { return; }
  for (i = 0, last = &DB_OBJ(obj)->hooks[entry]; *last; i++, last = &(*last)->next) {
    if (!memcmp(NEXT_ADDR(*last), hld_value, hld_size)) {
      grn_obj_delete_hook(ctx, obj, entry, i);
      return;
    }
  }
}

static void
delete_source_hook(grn_ctx *ctx, grn_obj *obj)
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
        del_hook(ctx, source, GRN_HOOK_INSERT, &data);
        del_hook(ctx, source, GRN_HOOK_DELETE, &data);
        break;
      case GRN_COLUMN_FIX_SIZE :
      case GRN_COLUMN_VAR_SIZE :
        del_hook(ctx, source, GRN_HOOK_SET, &data);
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

void
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
      grn_obj buf;
      grn_id *vp = (grn_id *)GRN_BULK_HEAD(value);
      uint32_t vs = GRN_BULK_VSIZE(value), s = 0;
      const char *n = _grn_table_key(ctx, ctx->impl->db, DB_OBJ(obj)->id, &s);
      GRN_TEXT_INIT(&buf, 0);
      GRN_TEXT_PUT(ctx, &buf, n, s);
      GRN_TEXT_PUTC(ctx, &buf, ' ');
      while (vs) {
        n = _grn_table_key(ctx, ctx->impl->db, *vp++, &s);
        GRN_TEXT_PUT(ctx, &buf, n, s);
        vs -= sizeof(grn_id);
        if (vs) { GRN_TEXT_PUTC(ctx, &buf, ','); }
      }
      GRN_LOG(ctx, GRN_LOG_NOTICE, "DDL:set_source %.*s",
              GRN_BULK_VSIZE(&buf), GRN_BULK_HEAD(&buf));
      GRN_OBJ_FIN(ctx, &buf);
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
    int i = 0;
    grn_hook *h, **last = &DB_OBJ(obj)->hooks[entry];
    for (;;) {
      if (!(h = *last)) { return GRN_INVALID_ARGUMENT; }
      if (++i > offset) { break; }
      last = &h->next;
    }
    *last = h->next;
    GRN_FREE(h);
  }
  grn_obj_spec_save(ctx, DB_OBJ(obj));
  GRN_API_RETURN(GRN_SUCCESS);
}

static void
remove_index(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry)
{
  grn_hook *h0, *hooks = DB_OBJ(obj)->hooks[entry];
  DB_OBJ(obj)->hooks[entry] = NULL; /* avoid mutual recursive call */
  while (hooks) {
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
    h0 = hooks;
    hooks = hooks->next;
    GRN_FREE(h0);
  }
}

static void
remove_columns(grn_ctx *ctx, grn_obj *obj)
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
}

grn_rc
grn_obj_remove(grn_ctx *ctx, grn_obj *obj)
{
  grn_id id;
  char *path;
  grn_obj *db;
  GRN_API_ENTER;
  if (ctx->impl && ctx->impl->db) {
    uint32_t s = 0;
    const char *n = _grn_table_key(ctx, ctx->impl->db, DB_OBJ(obj)->id, &s);
    GRN_LOG(ctx, GRN_LOG_NOTICE, "DDL:obj_remove %.*s", s, n);
  }
  if ((path = (char *)grn_obj_path(ctx, obj)) && *path != '\0') {
    if (!(path = GRN_STRDUP(path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path.");
      goto exit;
    }
  } else {
    path = NULL;
  }
  if (GRN_DB_OBJP(obj)) {
    id = DB_OBJ(obj)->id;
    db = DB_OBJ(obj)->db;
  }
  switch (obj->header.type) {
  case GRN_DB :
    {
      char *spath;
      grn_table_cursor *cur;
      grn_db *s = (grn_db *)db;
      if (s->specs &&
          (spath = (char *)grn_obj_path(ctx, (grn_obj *)s->specs)) && *spath != '\0') {
        if (!(spath = GRN_STRDUP(spath))) {
          ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path.");
          if (path) { GRN_FREE(path); }
          goto exit;
        }
      } else {
        spath = NULL;
      }
      if ((cur = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0, 0, -1, 0))) {
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
      grn_obj_close(ctx, obj);
      if (spath) {
        grn_ja_remove(ctx, spath);
        GRN_FREE(spath);
      }
      if (path) { grn_pat_remove(ctx, path); }
    }
    break;
  case GRN_TABLE_PAT_KEY :
    remove_index(ctx, obj, GRN_HOOK_INSERT);
    remove_columns(ctx, obj);
    grn_obj_close(ctx, obj);
    if (path) {
      grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
      grn_obj_delete_by_id(ctx, db, id, 1);
      grn_pat_remove(ctx, path);
    }
    grn_obj_touch(ctx, db, NULL);
    break;
  case GRN_TABLE_HASH_KEY :
    remove_index(ctx, obj, GRN_HOOK_INSERT);
    remove_columns(ctx, obj);
    grn_obj_close(ctx, obj);
    if (path) {
      grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
      grn_obj_delete_by_id(ctx, db, id, 1);
      grn_hash_remove(ctx, path);
    }
    grn_obj_touch(ctx, db, NULL);
    break;
  case GRN_TABLE_NO_KEY :
    remove_columns(ctx, obj);
    grn_obj_close(ctx, obj);
    if (path) {
      grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
      grn_obj_delete_by_id(ctx, db, id, 1);
      grn_array_remove(ctx, path);
    }
    grn_obj_touch(ctx, db, NULL);
    break;
  case GRN_COLUMN_VAR_SIZE :
    remove_index(ctx, obj, GRN_HOOK_SET);
    grn_obj_close(ctx, obj);
    if (path) {
      grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
      grn_obj_delete_by_id(ctx, db, id, 1);
      grn_ja_remove(ctx, path);
    }
    grn_obj_touch(ctx, db, NULL);
    break;
  case GRN_COLUMN_FIX_SIZE :
    remove_index(ctx, obj, GRN_HOOK_SET);
    grn_obj_close(ctx, obj);
    if (path) {
      grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
      grn_obj_delete_by_id(ctx, db, id, 1);
      grn_ra_remove(ctx, path);
    }
    grn_obj_touch(ctx, db, NULL);
    break;
  case GRN_COLUMN_INDEX :
    delete_source_hook(ctx, obj);
    grn_obj_close(ctx, obj);
    if (path) {
      grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
      grn_obj_delete_by_id(ctx, db, id, 1);
      grn_ii_remove(ctx, path);
    }
    grn_obj_touch(ctx, db, NULL);
    break;
  default :
    if (GRN_DB_OBJP(obj)) {
      grn_obj_close(ctx, obj);
      if (!(id & GRN_OBJ_TMP_OBJECT)) {
        grn_ja_put(ctx, ((grn_db *)db)->specs, id, NULL, 0, GRN_OBJ_SET);
        grn_obj_delete_by_id(ctx, db, id, 1);
      }
      grn_obj_touch(ctx, db, NULL);
    } else {
      grn_obj_close(ctx, obj);
    }
  }
  if (path) { GRN_FREE(path); }
exit :
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_rename(grn_ctx *ctx, const char *old_path, const char *new_path)
{
  GRN_API_ENTER;
  GRN_API_RETURN(GRN_SUCCESS);
}

/* db must be validated by caller */
grn_id
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
      db_value *vp;
      grn_db *s = (grn_db *)db;
      if ((vp = grn_tiny_array_at(&s->values, id))) {
        GRN_ASSERT(!vp->lock);
        vp->lock = 0;
        vp->ptr = NULL;
        vp->done = 0;
      }
      return removep ? grn_pat_delete_by_id(ctx, s->keys, id, NULL) : GRN_SUCCESS;
    }
  }
  return GRN_INVALID_ARGUMENT;
}

/* db must be validated by caller */
grn_rc
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
      db_value *vp;
      vp = grn_tiny_array_at(&((grn_db *)db)->values, id);
      if (!vp) {
        rc = GRN_NO_MEMORY_AVAILABLE;
        ERR(rc, "grn_tiny_array_at failed (%d)", id);
        return rc;
      }
      vp->lock = 1;
      vp->ptr = (grn_obj *)obj;
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

#define UNPACK_INFO() {\
  if (vp->ptr) {\
    grn_db_obj *r = DB_OBJ(vp->ptr);\
    r->header = spec->header;\
    r->id = id;\
    r->range = spec->range;\
    r->db = (grn_obj *)s;\
    size = grn_vector_get_element(ctx, &v, 2, &p, NULL, NULL);\
    if (size) {\
      if ((r->source = GRN_MALLOC(size))) {\
        memcpy(r->source, p, size);\
        r->source_size = size;\
      }\
    }\
    size = grn_vector_get_element(ctx, &v, 3, &p, NULL, NULL);\
    grn_hook_unpack(ctx, r, p, size);\
  }\
}

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
    grn_db *s = (grn_db *)ctx->impl->db;
    if (s) {
      db_value *vp;
      uint32_t l, *pl, ntrial;
      if (!(vp = grn_tiny_array_at(&s->values, id))) { goto exit; }
#ifdef USE_NREF
      pl = &vp->lock;
      for (ntrial = 0;; ntrial++) {
        GRN_ATOMIC_ADD_EX(pl, 1, l);
        if (l < GRN_IO_MAX_REF) { break; }
        if (ntrial >= 10) {
          GRN_LOG(ctx, GRN_LOG_NOTICE, "max trial in ctx_at(%p,%d)", vp->ptr, vp->lock);
          break;
        }
        GRN_ATOMIC_ADD_EX(pl, -1, l);
        GRN_FUTEX_WAIT(pl);
      }
#endif /* USE_NREF */
      if (s->specs && !vp->ptr /* && !vp->done */) {
#ifndef USE_NREF
        pl = &vp->lock;
        for (ntrial = 0;; ntrial++) {
          GRN_ATOMIC_ADD_EX(pl, 1, l);
          if (l < GRN_IO_MAX_REF) { break; }
          if (ntrial >= 10) {
            GRN_LOG(ctx, GRN_LOG_NOTICE, "max trial in ctx_at(%p,%d)", vp->ptr, vp->lock);
            break;
          }
          GRN_ATOMIC_ADD_EX(pl, -1, l);
          GRN_FUTEX_WAIT(pl);
        }
#endif /* USE_NREF */
        if (!l) {
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
                  vp->ptr = (grn_obj *)grn_type_open(ctx, spec);
                  UNPACK_INFO();
                  break;
                case GRN_TABLE_HASH_KEY :
                  GET_PATH(spec, buffer, s, id);
                  vp->ptr = (grn_obj *)grn_hash_open(ctx, buffer);
                  UNPACK_INFO();
                  break;
                case GRN_TABLE_PAT_KEY :
                  GET_PATH(spec, buffer, s, id);
                  vp->ptr = (grn_obj *)grn_pat_open(ctx, buffer);
                  UNPACK_INFO();
                  break;
                case GRN_TABLE_NO_KEY :
                  GET_PATH(spec, buffer, s, id);
                  vp->ptr = (grn_obj *)grn_array_open(ctx, buffer);
                  UNPACK_INFO();
                  break;
                case GRN_TABLE_VIEW :
                  GET_PATH(spec, buffer, s, id);
                  vp->ptr = grn_view_open(ctx, buffer);
                  UNPACK_INFO();
                  break;
                case GRN_COLUMN_VAR_SIZE :
                  GET_PATH(spec, buffer, s, id);
                  vp->ptr = (grn_obj *)grn_ja_open(ctx, buffer);
                  UNPACK_INFO();
                  break;
                case GRN_COLUMN_FIX_SIZE :
                  GET_PATH(spec, buffer, s, id);
                  vp->ptr = (grn_obj *)grn_ra_open(ctx, buffer);
                  UNPACK_INFO();
                  break;
                case GRN_COLUMN_INDEX :
                  GET_PATH(spec, buffer, s, id);
                  {
                    grn_obj *table = grn_ctx_at(ctx, spec->header.domain);
                    vp->ptr = (grn_obj *)grn_ii_open(ctx, buffer, table);
                  }
                  UNPACK_INFO();
                  break;
                case GRN_PROC :
                  GET_PATH(spec, buffer, s, id);
                  grn_db_register(ctx, buffer);
                  break;
                case GRN_EXPR :
                  {
                    size = grn_vector_get_element(ctx, &v, 4, &p, NULL, NULL);
                    vp->ptr = grn_expr_open(ctx, spec, p, p + size);
                  }
                  break;
                }
              }
              grn_obj_close(ctx, &v);
            }
            grn_ja_unref(ctx, &jw);
          }
#ifndef USE_NREF
          GRN_ATOMIC_ADD_EX(pl, -1, l);
#endif /* USE_NREF */
          vp->done = 1;
          GRN_FUTEX_WAKE(&vp->ptr);
        } else {
          for (ntrial = 0; !vp->ptr; ntrial++) {
            if (ntrial >= 1000) {
              GRN_LOG(ctx, GRN_LOG_NOTICE, "max trial in ctx_at(%d,%p,%d)!", id, vp->ptr, vp->lock);
              break;
            }
            GRN_FUTEX_WAIT(&vp->ptr);
          }
        }
      }
      res = vp->ptr;
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
        grn_module_close(ctx, p->obj.range);
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

void
grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)
{
  if (obj &&
      (!GRN_DB_OBJP(obj) ||
       (((grn_db_obj *)obj)->id & GRN_OBJ_TMP_OBJECT) ||
       obj->header.type == GRN_DB)) {
    grn_obj_close(ctx, obj);
  } else if (GRN_DB_OBJP(obj)) {
#ifdef USE_NREF
    grn_db_obj *dob = DB_OBJ(obj);
    grn_db *s = (grn_db *)dob->db;
    db_value *vp = grn_tiny_array_at(&s->values, dob->id);
    if (vp) {
      uint32_t l, *pl = &vp->lock;
      if (!vp->lock) {
        GRN_LOG(ctx, GRN_LOG_ERROR, "invalid unlink(%p,%d)", obj, vp->lock);
        return;
      }
      GRN_ATOMIC_ADD_EX(pl, -1, l);
      if (l == 1) {
        GRN_ATOMIC_ADD_EX(pl, GRN_IO_MAX_REF, l);
        if (l == GRN_IO_MAX_REF) {
#ifdef CALL_FINALIZER
          grn_obj_close(ctx, obj);
          vp->done = 0;
          if (dob->finalizer) {
            dob->finalizer(ctx, 1, &obj, &dob->user_data);
            dob->finalizer = NULL;
            dob->user_data.ptr = NULL;
          }
#endif /* CALL_FINALIZER */
        }
        GRN_ATOMIC_ADD_EX(pl, -GRN_IO_MAX_REF, l);
        GRN_FUTEX_WAKE(pl);
      }
    }
#endif /* USE_NREF */
  }
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
  if (obj->header.type == GRN_PROC) {
    return grn_module_path(ctx, DB_OBJ(obj)->range);
  }
  GRN_API_ENTER;
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
    rv->limit = limit;
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
    if (i > limit) { i = limit; }
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
            case GRN_OBJ_KEY_GEO_POINT :
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
                GRN_OBJ_KEY_GEO_POINT, sizeof(grn_geo_point));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TOKYO_GEO_POINT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "WGS84GeoPoint",
                GRN_OBJ_KEY_GEO_POINT, sizeof(grn_geo_point));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_WGS84_GEO_POINT) { return GRN_FILE_CORRUPT; }
  for (id = grn_pat_curr_id(ctx, ((grn_db *)db)->keys) + 1; id < GRN_DB_MECAB; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
#ifdef WITH_MECAB
  if (grn_db_init_mecab_tokenizer(ctx)) {
#endif
    grn_obj_register(ctx, db, "TokenMecab", 10);
#ifdef WITH_MECAB
  }
#endif
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
    case GRN_OP_PREFIX :
    case GRN_OP_SUFFIX :
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
    case GRN_OP_CALL :
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
  } else if (GRN_ACCESSORP(obj)) {
    switch (op) {
    case GRN_OP_EQUAL :
    case GRN_OP_TERM_EXTRACT :
      if (buf_size) { indexbuf[n++] = obj; }
      break;
    case GRN_OP_PREFIX :
      {
        grn_accessor *a = (grn_accessor *)obj;
        if (a->action == GRN_ACCESSOR_GET_KEY) {
          if (a->obj->header.type == GRN_TABLE_PAT_KEY) {
            if (buf_size) { indexbuf[n++] = obj; }
          }
        }
      }
      break;
    case GRN_OP_SUFFIX :
      {
        grn_accessor *a = (grn_accessor *)obj;
        if (a->action == GRN_ACCESSOR_GET_KEY) {
          if (a->obj->header.type == GRN_TABLE_PAT_KEY &&
              a->obj->header.flags & GRN_OBJ_KEY_WITH_SIS) {
            if (buf_size) { indexbuf[n++] = obj; }
          }
        }
      }
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

/* todo : refine */
static int
tokenize(const char *str, size_t str_len, const char **tokbuf, int buf_size, const char **rest)
{
  const char **tok = tokbuf, **tok_end = tokbuf + buf_size;
  if (buf_size > 0) {
    const char *str_end = str + str_len;
    while (str < str_end && (' ' == *str || ',' == *str)) { str++; }
    for (;;) {
      if (str == str_end) {
        *tok++ = str;
        break;
      }
      if (' ' == *str || ',' == *str) {
        // *str = '\0';
        *tok++ = str;
        if (tok == tok_end) { break; }
        do { str++; } while (str < str_end && (' ' == *str || ',' == *str));
      } else {
        str++;
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
      while (p < r && (' ' == *p || ',' == *p)) { p++; }
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
      p = r;
    }
    p = q;
  }
  return ctx->rc;
}

grn_table_sort_key *
grn_table_sort_key_from_str(grn_ctx *ctx, const char *str, unsigned str_size,
                            grn_obj *table, unsigned *nkeys)
{
  const char *p = str;
  const char **tokbuf;
  grn_table_sort_key *keys = NULL, *k = NULL;
  if ((tokbuf = GRN_MALLOCN(const char *, str_size))) {
    int i, n = tokenize(str, str_size, tokbuf, str_size, NULL);
    if ((keys = GRN_MALLOCN(grn_table_sort_key, n))) {
      k = keys;
      for (i = 0; i < n; i++) {
        const char *r = tokbuf[i];
        while (p < r && (' ' == *p || ',' == *p)) { p++; }
        if (p < r) {
          k->flags = GRN_TABLE_SORT_ASC;
          k->offset = 0;
          if (*p == '+') {
            p++;
          } else if (*p == '-') {
            k->flags = GRN_TABLE_SORT_DESC;
            p++;
          }
          if (!(k->key = grn_obj_column(ctx, table, p, r - p))) {
            WARN(GRN_INVALID_ARGUMENT, "invalid sort key: <%.*s>(<%.*s>)",
                 tokbuf[i] - p, p, str_size, str);
            break;
          }
          k++;
        }
        p = r;
      }
    }
    GRN_FREE(tokbuf);
  }
  if (!ctx->rc) {
    *nkeys = k - keys;
  } else {
    if (keys) { GRN_FREE(keys); }
    *nkeys =0;
    keys = NULL;
  }
  return keys;
}

grn_rc
grn_table_sort_key_close(grn_ctx *ctx, grn_table_sort_key *keys, unsigned nkeys)
{
  int i;
  if (keys) {
    for (i = 0; i < nkeys; i++) {
      grn_obj_unlink(ctx, keys[i].key);
    }
    GRN_FREE(keys);
  }
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
    result = grn_expr_exec(ctx, loader->ifexists, 0);
    GRN_TRUEP(ctx, result, result_boolean);
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

#define KEY_NAME "_key"
#define ID_NAME "_id"

static inline int
name_equal(const char *p, unsigned size, const char *name)
{
  if (strlen(name) != size) { return 0; }
  if (*p != GRN_DB_PSEUDO_COLUMN_PREFIX) { return 0; }
  return !memcmp(p + 1, name + 1, size - 1);
}

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
          grn_obj *key_value = NULL;
          while (ndata--) {
            char *column_name = GRN_TEXT_VALUE(value);
            unsigned column_name_size = GRN_TEXT_LEN(value);
            if (value->header.domain == GRN_DB_TEXT &&
                (name_equal(column_name, column_name_size, KEY_NAME) ||
                 name_equal(column_name, column_name_size, ID_NAME))) {
              if (loader->key_offset != -1) {
                GRN_LOG(ctx, GRN_LOG_ERROR, "duplicated key columns: %.*s at %d and %.*s at %i",
                        GRN_TEXT_LEN(key_value), GRN_TEXT_VALUE(key_value), loader->key_offset,
                        column_name_size, column_name, i);
                return;
              }
              key_value = value;
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
          grn_obj *v, *key_value = 0;
          for (v = value; v + 1 < ve; v = values_next(ctx, v)) {
            char *column_name = GRN_TEXT_VALUE(v);
            unsigned column_name_size = GRN_TEXT_LEN(v);
            if (v->header.domain == GRN_DB_TEXT &&
                (name_equal(column_name, column_name_size, KEY_NAME) ||
                 name_equal(column_name, column_name_size, ID_NAME))) {
              if (key_value) {
                GRN_LOG(ctx, GRN_LOG_ERROR, "duplicated key columns: %.*s and %.*s",
                        GRN_TEXT_LEN(key_value), GRN_TEXT_VALUE(key_value),
                        column_name_size, column_name);
                return;
              }
              key_value = value;
              v++;
              id = loader_add(ctx, v);
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
          } else {
            GRN_LOG(ctx, GRN_LOG_ERROR, "invalid column('%.*s')", (int)name_size, name);
          }
          value = values_next(ctx, value);
        }
        loader->nrecords++;
      } else {
        GRN_LOG(ctx, GRN_LOG_ERROR, "neither _key nor _id is assigned");
      }
    }
    loader->values_size = begin;
  }
}

static void
json_read(grn_ctx *ctx, grn_loader *loader, const char *str, unsigned str_len)
{
  const char *const beg = str;
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
        if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
          loader->stat = GRN_LOADER_SYMBOL;
          values_add(ctx, loader);
        } else {
          if ((len = grn_charlen(ctx, str, se))) {
            GRN_LOG(ctx, GRN_LOG_ERROR, "ignored invalid char('%c') at", c);
            GRN_LOG(ctx, GRN_LOG_ERROR, "%.*s", (int)(str - beg) + len, beg);
            GRN_LOG(ctx, GRN_LOG_ERROR, "%*s", (int)(str - beg) + 1, "^");
            str += len;
          } else {
            GRN_LOG(ctx, GRN_LOG_ERROR, "ignored invalid char(\\x%.2x) after", c);
            GRN_LOG(ctx, GRN_LOG_ERROR, "%.*s", (int)(str - beg), beg);
            str = se;
          }
        }
        break;
      }
      break;
    case GRN_LOADER_SYMBOL :
      if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
        GRN_TEXT_PUTC(ctx, loader->last, c);
        str++;
      } else {
        char *v = GRN_TEXT_VALUE(loader->last);
        switch (*v) {
        case 'n' :
          if (GRN_TEXT_LEN(loader->last) == 4 && !memcmp(v, "null", 4)) {
            loader->last->header.domain = GRN_DB_VOID;
            GRN_UINT32_SET(ctx, loader->last, GRN_ID_NIL);
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
          GRN_LOG(ctx, GRN_LOG_ERROR, "ignored invalid char(\\x%.2x) after", c);
          GRN_LOG(ctx, GRN_LOG_ERROR, "%.*s", (int)(str - beg), beg);
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

static grn_com_addr *addr;

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
  if (table && table_len) {
    grn_ctx_loader_clear(ctx);
    loader->table = grn_ctx_get(ctx, table, table_len);
    if (loader->table && columns && columns_len) {
      int i, n_columns;
      grn_obj parsed_columns;

      GRN_PTR_INIT(&parsed_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
      grn_obj_columns(ctx, loader->table, columns, columns_len,
                      &parsed_columns);
      n_columns = GRN_BULK_VSIZE(&parsed_columns) / sizeof(grn_obj *);
      for (i = 0; i < n_columns; i++) {
        grn_obj *column;
        column = GRN_PTR_VALUE_AT(&parsed_columns, i);
        if (column->header.type == GRN_ACCESSOR &&
            ((grn_accessor *)column)->action == GRN_ACCESSOR_GET_KEY) {
          loader->key_offset = i;
        } else {
          GRN_PTR_PUT(ctx, &loader->columns, column);
        }
      }
      GRN_OBJ_FIN(ctx, &parsed_columns);
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
    loader->input_type = input_type;
  } else {
    input_type = loader->input_type;
  }
  switch (input_type) {
  case GRN_CONTENT_JSON :
    json_read(ctx, loader, values, values_len);
    break;
  case GRN_CONTENT_NONE :
  case GRN_CONTENT_TSV :
  case GRN_CONTENT_XML :
  case GRN_CONTENT_MSGPACK :
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "unsupported input_type");
    // todo
    break;
  }
  GRN_API_RETURN(ctx->rc);
}
