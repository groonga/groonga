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
#include <string.h>

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

struct _grn_db {
  grn_db_obj obj;
  grn_pat *keys;
  grn_ja *specs;
  grn_tiny_array values;
  grn_mutex lock;
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
        MUTEX_INIT(s->lock);
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
          MUTEX_INIT(s->lock);
          GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
          s->obj.db = (grn_obj *)s;
          s->obj.header.domain = GRN_ID_NIL;
          DB_OBJ(&s->obj)->range = GRN_ID_NIL;
          grn_ctx_use(ctx, (grn_obj *)s);
          grn_db_init_builtin_tokenizers(ctx);
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
    MUTEX_DESTROY(a->lock);
  }
#endif
  grn_tiny_array_fin(&s->values);
  grn_pat_close(ctx, s->keys);
  MUTEX_DESTROY(s->lock);
  if (s->specs) { grn_ja_close(ctx, s->specs); }
  GRN_FREE(s);
  GRN_API_RETURN(GRN_SUCCESS);
}

static grn_id grn_obj_register(grn_ctx *ctx, grn_obj *db,
                               const char *name, unsigned name_size);
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

#define GRN_DB_DELIMITER '.'

static grn_rc
check_name(grn_ctx *ctx, const char *name, unsigned int name_size)
{
  int len;
  const char *name_end = name + name_size;
  while (name < name_end) {
    if (*name == GRN_DB_DELIMITER) { return GRN_INVALID_ARGUMENT; }
    if (!(len = grn_charlen(ctx, name, name_end))) { break; }
    name += len;
  }
  return GRN_SUCCESS;
}

#define GRN_TYPE_SIZE(type) ((type)->range)

struct _grn_type {
  grn_db_obj obj;
};

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
    ERR(GRN_INVALID_ARGUMENT, "name contains '%c'", GRN_DB_DELIMITER);
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
grn_proc_create(grn_ctx *ctx,
                const char *name, unsigned name_size, const char *path,
                grn_proc_func *init, grn_proc_func *next, grn_proc_func *fin,
                unsigned nargs, unsigned nresults, grn_obj *result_types)
{
  grn_proc *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id domain;
  int added = 0;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  if (nresults > 16) {
    ERR(GRN_INVALID_ARGUMENT, "too many results");
  }
  GRN_API_ENTER;
  domain = path ? grn_dl_get(ctx, path) : GRN_ID_NIL;
  if (check_name(ctx, name, name_size)) {
    ERR(GRN_INVALID_ARGUMENT, "name contains '%c'", GRN_DB_DELIMITER);
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
      res = (grn_proc *)grn_ctx_at(ctx, id);
      if (res && res->funcs[PROC_INIT]) {
        ERR(GRN_INVALID_ARGUMENT, "already used name");
        GRN_API_RETURN(NULL);
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
    res->obj.header.domain = domain;
    res->obj.range = GRN_ID_NIL;
    res->funcs[PROC_INIT] = init;
    res->funcs[PROC_NEXT] = next;
    res->funcs[PROC_FIN] = fin;
    res->nargs = nargs;
    res->nresults = nresults;
    memcpy(res->results, result_types, sizeof(grn_obj) * nresults);
    if (added) {
      if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
        // grn_obj_delete(ctx, db, id);
        GRN_FREE(res);
        GRN_API_RETURN(NULL);
      }
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
    res->obj.header = spec->header;
    if (res->obj.header.domain) {
      // todo : grn_dl_load should be called.
    }
  }
  return (grn_obj *)res;
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

grn_obj *
grn_table_create(grn_ctx *ctx, const char *name, unsigned name_size,
                 const char *path, grn_obj_flags flags,
                 grn_obj *key_type, unsigned value_size)
{
  grn_id id;
  grn_id domain = GRN_ID_NIL;
  uint32_t key_size, max_n_subrecs;
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
    ERR(GRN_INVALID_ARGUMENT, "name contains '%c'", GRN_DB_DELIMITER);
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
  id = grn_obj_register(ctx, db, name, name_size);
  if (ERRP(ctx, GRN_ERROR)) { GRN_API_RETURN(NULL);  }
  if (GRN_OBJ_PERSISTENT & flags) {
    if (!path) {
      if (PERSISTENT_DB_P(db)) {
        gen_pathname(((grn_db *)db)->keys->io->path, buffer, id);
        path = buffer;
      } else {
        ERR(GRN_INVALID_ARGUMENT, "path not assigend for persistent table");
        GRN_API_RETURN(NULL);
      }
    } else {
      flags |= GRN_OBJ_CUSTOM_NAME;
    }
  } else {
    if (path) {
      ERR(GRN_INVALID_ARGUMENT, "path assigend for temporary table");
      GRN_API_RETURN(NULL);
    }
  }
  calc_rec_size(flags, &max_n_subrecs, &subrec_size,
                &subrec_offset, &key_size, &value_size);
  switch (flags & GRN_OBJ_TABLE_TYPE_MASK) {
  case GRN_OBJ_TABLE_HASH_KEY :
    res = (grn_obj *)grn_hash_create(ctx, path, key_size, value_size, flags);
    break;
  case GRN_OBJ_TABLE_PAT_KEY :
    res = (grn_obj *)grn_pat_create(ctx, path, key_size, value_size, flags);
    break;
  case GRN_OBJ_TABLE_NO_KEY :
    res = (grn_obj *)grn_array_create(ctx, path, value_size, flags);
    break;
  }
  if (res) {
    DB_OBJ(res)->header.flags = flags;
    DB_OBJ(res)->header.impl_flags = 0;
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->range = GRN_ID_NIL;
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

grn_id
grn_table_lookup(grn_ctx *ctx, grn_obj *table, const void *key, unsigned key_size,
                 grn_search_flags *flags)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table && key_size) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      {
        grn_pat *pat = (grn_pat *)table;
        WITH_NORMALIZE(pat, key, key_size, {
          if (*flags & GRN_SEARCH_LCP) {
            id = grn_pat_lcp_search(ctx, pat, key, key_size);
          } else {
            if (*flags & GRN_TABLE_ADD) {
              if (grn_io_lock(ctx, pat->io, 10000000)) {
                id = GRN_ID_NIL;
              } else {
                int added;
                id = grn_pat_add(ctx, pat, key, key_size, NULL, &added);
                if (flags && added) { *flags |= GRN_TABLE_ADDED; }
                grn_io_unlock(pat->io);
              }
            } else {
              id = grn_pat_get(ctx, pat, key, key_size, NULL);
            }
          }
        });
      }
      break;
    case GRN_TABLE_HASH_KEY :
      {
        grn_hash *hash = (grn_hash *)table;
        WITH_NORMALIZE(hash, key, key_size, {
          if (*flags & GRN_TABLE_ADD) {
            if (grn_io_lock(ctx, hash->io, 10000000)) {
              id = GRN_ID_NIL;
            } else {
              int added;
              id = grn_hash_add(ctx, hash, key, key_size, NULL, &added);
              if (flags && added) { *flags |= GRN_TABLE_ADDED; }
              grn_io_unlock(hash->io);
            }
          } else {
            id = grn_hash_get(ctx, hash, key, key_size, NULL);
          }
        });
      }
      break;
    }
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_add(grn_ctx *ctx, grn_obj *table, const void *key, unsigned key_size, int *added)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      {
        grn_pat *pat = (grn_pat *)table;
        WITH_NORMALIZE(pat, key, key_size, {
          if (grn_io_lock(ctx, pat->io, 10000000)) {
            id = GRN_ID_NIL;
          } else {
            id = grn_pat_add(ctx, pat, key, key_size, NULL, added);
            grn_io_unlock(pat->io);
          }
        });
      }
      break;
    case GRN_TABLE_HASH_KEY :
      {
        grn_hash *hash = (grn_hash *)table;
        WITH_NORMALIZE(hash, key, key_size, {
          if (grn_io_lock(ctx, hash->io, 10000000)) {
            id = GRN_ID_NIL;
          } else {
            id = grn_hash_add(ctx, hash, key, key_size, NULL, added);
            grn_io_unlock(hash->io);
          }
        });
      }
      break;
    case GRN_TABLE_NO_KEY :
      id = grn_array_add(ctx, (grn_array *)table, NULL);
      if (added) { *added = id ? 1 : 0; }
      break;
    }
  }
  GRN_API_RETURN(id);
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

grn_rc
grn_table_delete(grn_ctx *ctx, grn_obj *table, const void *key, unsigned key_size)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_DB :
      /* todo : delete tables and columns from db */
      break;
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        rc = grn_pat_delete(ctx, (grn_pat *)table, key, key_size, NULL);
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        rc = grn_hash_delete(ctx, (grn_hash *)table, key, key_size, NULL);
      });
      break;
    }
    /* todo : clear_all_column_values */
  }
  GRN_API_RETURN(rc);
}

grn_rc
_grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id,
                       grn_table_delete_optarg *optarg)
{
  if (table) {
    // todo : support optarg
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      return grn_pat_delete_by_id(ctx, (grn_pat *)table, id, optarg);
    case GRN_TABLE_HASH_KEY :
      return grn_hash_delete_by_id(ctx, (grn_hash *)table, id, optarg);
    case GRN_TABLE_NO_KEY :
      return grn_array_delete_by_id(ctx, (grn_array *)table, id, optarg);
    }
  }
  return GRN_INVALID_ARGUMENT;
}

grn_rc
grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id)
{
  grn_rc rc;
  GRN_API_ENTER;
  rc = _grn_table_delete_by_id(ctx, table, id, NULL);
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_truncate(grn_ctx *ctx, grn_obj *table)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    rc = GRN_SUCCESS;
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
    case GRN_TABLE_PAT_KEY :
      n = grn_pat_size(ctx, (grn_pat *)table);
      break;
    case GRN_TABLE_HASH_KEY :
      n = GRN_HASH_SIZE((grn_hash *)table);
      break;
    case GRN_TABLE_NO_KEY :
      n = GRN_ARRAY_SIZE((grn_array *)table);
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

grn_table_cursor *
grn_table_cursor_open(grn_ctx *ctx, grn_obj *table,
                      const void *min, unsigned min_size,
                      const void *max, unsigned max_size,
                      int flags)
{
  grn_table_cursor *tc = NULL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_DB :
      tc = (grn_table_cursor *)grn_pat_cursor_open(ctx, ((grn_db *)table)->keys,
                                                   min, min_size,
                                                   max, max_size, flags);
      break;
    case GRN_TABLE_PAT_KEY :
      tc = (grn_table_cursor *)grn_pat_cursor_open(ctx, (grn_pat *)table,
                                                   min, min_size,
                                                   max, max_size, flags);
      break;
    case GRN_TABLE_HASH_KEY :
      tc = (grn_table_cursor *)grn_hash_cursor_open(ctx, (grn_hash *)table,
                                                    min, min_size,
                                                    max, max_size, flags);
      break;
    case GRN_TABLE_NO_KEY :
      tc = (grn_table_cursor *)grn_array_cursor_open(ctx, (grn_array *)table,
                                                     GRN_ID_NIL, GRN_ID_NIL, flags);
      break;
    }
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
                                                   NULL, 0, NULL, 0, flags);
      break;
    case GRN_TABLE_HASH_KEY :
      tc = (grn_table_cursor *)grn_hash_cursor_open(ctx, (grn_hash *)table,
                                                    NULL, 0, NULL, 0, flags);
      break;
    case GRN_TABLE_NO_KEY :
      tc = (grn_table_cursor *)grn_array_cursor_open(ctx, (grn_array *)table,
                                                     min, max, flags);
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
    }
  }
  GRN_API_RETURN(id);
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
                 grn_search_flags flags, grn_obj *res, grn_sel_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  switch (table->header.type) {
  case GRN_TABLE_PAT_KEY :
    {
      grn_pat *pat = (grn_pat *)table;
      WITH_NORMALIZE(pat, key, key_size, {
        switch (flags) {
        case GRN_SEARCH_EXACT :
          {
            grn_id id = grn_pat_get(ctx, pat, key, key_size, NULL);
            if (id) { grn_table_add(ctx, res, &id, sizeof(grn_id), NULL); }
          }
          // todo : support op;
          break;
        case GRN_SEARCH_LCP :
          {
            grn_id id = grn_pat_lcp_search(ctx, pat, key, key_size);
            if (id) { grn_table_add(ctx, res, &id, sizeof(grn_id), NULL); }
          }
          // todo : support op;
          break;
        case GRN_SEARCH_SUFFIX :
          rc = grn_pat_suffix_search(ctx, pat, key, key_size, (grn_hash *)res);
          // todo : support op;
          break;
        case GRN_SEARCH_PREFIX :
          rc = grn_pat_prefix_search(ctx, pat, key, key_size, (grn_hash *)res);
          // todo : support op;
          break;
        case GRN_SEARCH_TERM_EXTRACT :
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
          ERR(rc, "invalid flag %d", flags);
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
               grn_obj *res, grn_sel_operator op, grn_search_optarg *optarg)
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
        grn_search_flags flags = optarg ? optarg->flags : GRN_SEARCH_EXACT;
        if (!key || !key_size) {
          return GRN_INVALID_ARGUMENT;
        }
        rc = grn_table_search(ctx, obj, key, key_size, flags, res, op);
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
          rc = grn_ii_sel(ctx, (grn_ii *)obj, str, str_len, (grn_hash *)res, op);
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

grn_rc
grn_table_group(grn_ctx *ctx, grn_obj *table,
                grn_table_sort_key *keys, int n_keys,
                grn_table_group_result *results, int n_results)
{
  if (!table || !n_keys || !n_results) {
    ERR(GRN_INVALID_ARGUMENT, "table or n_keys or n_results is void");
    return GRN_INVALID_ARGUMENT;
  }
  GRN_API_ENTER;
  {
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
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0))) {
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
              // tood : support objects except grn_id
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
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0))) {
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
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_table_setoperation(grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res,
                       grn_sel_operator op)
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
  case GRN_SEL_OR :
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, &value2, {
      if (grn_table_add_v(ctx, table1, key, key_size, &value1, NULL)) {
        memcpy(value1, value2, value_size);
      }
    });
    break;
  case GRN_SEL_AND :
    GRN_TABLE_EACH(ctx, table1, 0, 0, id, &key, &key_size, &value1, {
      if (!grn_table_get_v(ctx, table2, key, key_size, &value2)) {
        _grn_table_delete_by_id(ctx, table1, id, NULL);
      }
    });
    break;
  case GRN_SEL_BUT :
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, &value2, {
      grn_table_delete(ctx, table1, key, key_size);
    });
    break;
  case GRN_SEL_ADJUST :
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, &value2, {
      if (grn_table_get_v(ctx, table1, key, key_size, &value1)) {
        memcpy(value1, value2, value_size);
      }
    });
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

grn_obj *
grn_obj_column(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size)
{
  grn_obj *column = NULL;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table)) {
    if (check_name(ctx, name, name_size)) {
      column = grn_obj_get_accessor(ctx, table, name, name_size);
    } else {
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
    }
  } else {
    if (table->header.type == GRN_ACCESSOR) {
      column = grn_obj_get_accessor(ctx, table, name, name_size);
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid table assigned");
    }
  }
  GRN_API_RETURN(column);
}

int
grn_table_columns(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size,
                  grn_obj *res)
{
  int n = 0;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table)) {
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
    ERR(GRN_INVALID_ARGUMENT, "name contains '%c'", GRN_DB_DELIMITER);
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
        ERR(GRN_INVALID_ARGUMENT, "path not assigend for persistent table");
        goto exit;
      }
    } else {
      flags |= GRN_OBJ_CUSTOM_NAME;
    }
  } else {
    if (path) {
      ERR(GRN_INVALID_ARGUMENT, "path assigend for temporary table");
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
    ERR(GRN_INVALID_ARGUMENT, "name contains '%c'", GRN_DB_DELIMITER);
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
  grn_proc_data *data = grn_proc_ctx_get_local_data(pctx);
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

typedef struct {
  grn_id target;
  unsigned int section;
} default_set_value_hook_data;

static grn_rc
default_set_value_hook(grn_ctx *ctx, grn_obj *obj, grn_proc_data *user_data)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  if (!pctx) {
    ERR(GRN_INVALID_ARGUMENT, "default_set_value_hook failed");
    return GRN_INVALID_ARGUMENT;
  } else {
    grn_obj *flags = grn_ctx_pop(ctx);
    grn_obj *newvalue = grn_ctx_pop(ctx);
    grn_obj *oldvalue = grn_ctx_pop(ctx);
    grn_obj *id = grn_ctx_pop(ctx);
    grn_hook *h = pctx->currh;
    default_set_value_hook_data *data = (void *)NEXT_ADDR(h);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    int section = data->section;
    if (flags) { /* tood */ }
    switch (target->header.type) {
    case GRN_COLUMN_INDEX :
      return grn_ii_column_update(ctx, (grn_ii *)target,
                                  GRN_UINT32_VALUE(id),
                                  section, oldvalue, newvalue);
    default :
      return GRN_SUCCESS;
    }
  }
}

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
  uint8_t action;
  int offset;
  grn_obj *obj;
  grn_accessor *next;
};

enum {
  GRN_ACCESSOR_VOID = 0,
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

grn_obj *
grn_obj_get_accessor(grn_ctx *ctx, grn_obj *obj, const char *name, unsigned name_size)
{
  grn_accessor *res = NULL, **rp = NULL, **rp0 = NULL;
  if (!obj) { return NULL; }
  GRN_API_ENTER;
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
    if (*name == ':') { /* pseudo column */
      int done = 0;
      switch (name[1]) {
      case 'k' : /* key */
      case 'K' :
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          obj = grn_ctx_at(ctx, obj->header.domain);
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
      case 's' : /* score */
      case 'S' :
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
      case 'N' :
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
        grn_obj *column = grn_obj_column(ctx, obj, name, len);
        *rp = accessor_new(ctx);
        if (column) {
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
  }
  return range;
}

const char *grn_obj_get_value_(grn_ctx *ctx, grn_obj *obj, grn_id id, uint32_t *size);

const char *
grn_accessor_get_value_(grn_ctx *ctx, grn_accessor *a, grn_id id, uint32_t *size)
{
  const char *value = NULL;
  for (;;) {
    switch (a->action) {
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
  size_t vs = 0;
  uint32_t size0;
  void *vp = NULL;
  if (!value) {
    if (!(value = grn_obj_open(ctx, GRN_BULK, 0, 0))) { return NULL; }
  }
  size0 = GRN_BULK_VSIZE(value);
  for (;;) {
    grn_bulk_truncate(ctx, value, size0);
    switch (a->action) {
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
        grn_rset_recinfo *ri = (grn_rset_recinfo *)(GRN_BULK_HEAD(value) + size0);
        vp = &ri->score;
        vs = sizeof(int);
      }
      break;
    case GRN_ACCESSOR_GET_NSUBRECS :
      grn_obj_get_value(ctx, a->obj, id, value);
      {
        grn_rset_recinfo *ri = (grn_rset_recinfo *)(GRN_BULK_HEAD(value) + size0);
        vp = &ri->n_subrecs;
        vs = sizeof(int);
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
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          vp = &ri->score;
          vs = sizeof(int);
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
    grn_hook *hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET];
    void *v = GRN_BULK_HEAD(value);
    unsigned int s = GRN_BULK_VSIZE(value);
    if (hooks || obj->header.type == GRN_COLUMN_VAR_SIZE) {
      grn_obj oldbuf, *oldvalue;
      GRN_TEXT_INIT(&oldbuf, 0);
      oldvalue = grn_obj_get_value(ctx, obj, id, &oldbuf);
      if (flags & GRN_OBJ_SET) {
        void *ov;
        unsigned int os;
        ov = GRN_BULK_HEAD(oldvalue);
        os = GRN_BULK_VSIZE(oldvalue);
        if (ov && v && os == s && !memcmp(ov, v, s)) {
          grn_bulk_fin(ctx, oldvalue);
          rc = GRN_SUCCESS;
          goto exit;
        }
      }
      if (hooks) {
        // todo : grn_proc_ctx_open()
        grn_obj id_, flags_;
        grn_proc_ctx pctx = {{0}, obj, hooks, hooks, PROC_INIT, 4, 4};
        GRN_UINT32_INIT(&id_, 0);
        GRN_UINT32_INIT(&flags_, 0);
        GRN_UINT32_SET(ctx, &id_, id);
        GRN_UINT32_SET(ctx, &flags_, flags);
        grn_ctx_push(ctx, &id_);
        grn_ctx_push(ctx, oldvalue);
        grn_ctx_push(ctx, value);
        grn_ctx_push(ctx, &flags_);
        while (hooks) {
          pctx.currh = hooks;
          if (hooks->proc) {
            rc = hooks->proc->funcs[PROC_INIT](ctx, obj, &pctx.user_data);
          } else {
            rc = default_set_value_hook(ctx, obj, &pctx.user_data);
          }
          if (rc) { goto exit; }
          hooks = hooks->next;
          pctx.offset++;
        }
      }
      grn_obj_close(ctx, oldvalue);
    }
    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY :
      rc = grn_pat_set_value(ctx, (grn_pat *)obj, id, v, flags);
      break;
    case GRN_TABLE_HASH_KEY :
      rc = grn_hash_set_value(ctx, (grn_hash *)obj, id, v, flags);
      break;
    case GRN_TABLE_NO_KEY :
      rc = grn_array_set_value(ctx, (grn_array *)obj, id, v, flags);
      break;
    case GRN_COLUMN_VAR_SIZE :
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
              {
                int j;
                grn_section *v;
                const char *head = GRN_BULK_HEAD(value->u.v.body);
                for (j = value->u.v.n_sections, v = value->u.v.sections; j; j--, v++) {
                  grn_id tid = grn_table_add(ctx, lexicon,
                                             head + v->offset, v->length, NULL);
                  grn_bulk_write(ctx, &buf, (char *)&tid, sizeof(grn_id));
                }
                rc = grn_ja_put(ctx, (grn_ja *)obj, id,
                                GRN_BULK_HEAD(&buf), GRN_BULK_VSIZE(&buf), flags);
              }
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "vecotr or bulk required");
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
            case GRN_VECTOR :
              rc = grn_ja_putv(ctx, (grn_ja *)obj, id, value, 0);
              break;
            default :
              ERR(GRN_INVALID_ARGUMENT, "vecotr or bulk required");
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
          memcpy(p, v, s);
          rc = GRN_SUCCESS;
          break;
        case GRN_OBJ_INCR :
          INCRDECR(+=);
          break;
        case GRN_OBJ_DECR :
          INCRDECR(-=);
          break;
        default :
          rc = GRN_OPERATION_NOT_SUPPORTED;
          break;
        }
        grn_ra_unref(ctx, (grn_ra *)obj, id);
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
  switch (obj->header.type) {
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
  case GRN_ATOM :
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
  case GRN_ACCESSOR :
    value = grn_accessor_get_value(ctx, (grn_accessor *)obj, id, value);
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
          default :
            ERR(GRN_INVALID_ARGUMENT, "vecotr or bulk required");
            break;
          }
        }
      }
      break;
    case GRN_OBJ_COLUMN_SCALAR :
      {
        grn_io_win jw;
        void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, &len);
        if (!v) { len = 0; goto exit; }
        // todo : reduce copy
        // todo : grn_vector_add_element when vector assigned
        grn_bulk_write(ctx, value, v, len);
        grn_ja_unref(ctx, &jw);
      }
      break;
    default :
      ERR(GRN_FILE_CORRUPT, "invalid GRN_OBJ_COLUMN_TYPE");
      break;
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
    {
      unsigned element_size;
      void *v = grn_ra_ref(ctx, (grn_ra *)obj, id);
      if (!v) { goto exit; }
      element_size = ((grn_ra *)obj)->header->element_size;
      grn_bulk_write(ctx, value, v, element_size);
      grn_ra_unref(ctx, (grn_ra *)obj, id);
      len = element_size;
    }
    break;
  case GRN_COLUMN_INDEX :
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "todo: GRN_COLUMN_INDEX");
    break;
  }
  value->header.domain = grn_obj_get_range(ctx, obj);
exit :
  GRN_API_RETURN(value);
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
    rc = grn_ii_column_update(ctx, (grn_ii *)column, id, section, oldvalue, newvalue);
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
      grn_obj_add_hook(ctx, source, GRN_HOOK_SET, 0, NULL, &data);
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
  if (obj->header.type == GRN_EXPR) {
    grn_expr_pack(ctx, b, (grn_obj *)obj);
    grn_vector_delimit(ctx, &v, 0, 0);
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
  }
  grn_obj_spec_save(ctx, DB_OBJ(obj));
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
    /* todo : remove all tables and columns */
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
          GRN_HASH_EACH(cols, id, &key, NULL, NULL, {
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
  case GRN_COLUMN_INDEX :
    grn_obj_delete_by_id(ctx, DB_OBJ(obj)->db, DB_OBJ(obj)->id, 1);
    break;
  }
  grn_obj_close(ctx, obj);
  if (path) {
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
      ERR(GRN_INVALID_ARGUMENT, "already used name was assigend");
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

static grn_obj *grn_expr_dup(grn_ctx *ctx, grn_obj *expr);

grn_obj *
grn_ctx_at(grn_ctx *ctx, grn_id id)
{
  grn_obj *res = NULL;
  if (!ctx || !ctx->impl || !id) { return res; }
  GRN_API_ENTER;
  if (id & GRN_OBJ_TMP_OBJECT) {
    if (ctx->impl->values) {
      grn_tmp_db_obj *tmp_obj;
      tmp_obj = _grn_array_get_value(ctx, ctx->impl->values, id & ~GRN_OBJ_TMP_OBJECT);
      res = (grn_obj *)tmp_obj->obj;
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
                MUTEX_LOCK(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_type_open(ctx, spec); }
                MUTEX_UNLOCK(s->lock);
                break;
              case GRN_TABLE_HASH_KEY :
                GET_PATH(spec, buffer, s, id);
                MUTEX_LOCK(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_hash_open(ctx, buffer); }
                MUTEX_UNLOCK(s->lock);
                break;
              case GRN_TABLE_PAT_KEY :
                GET_PATH(spec, buffer, s, id);
                MUTEX_LOCK(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_pat_open(ctx, buffer); }
                MUTEX_UNLOCK(s->lock);
                break;
              case GRN_TABLE_NO_KEY :
                GET_PATH(spec, buffer, s, id);
                MUTEX_LOCK(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_array_open(ctx, buffer); }
                MUTEX_UNLOCK(s->lock);
                break;
              case GRN_COLUMN_VAR_SIZE :
                GET_PATH(spec, buffer, s, id);
                MUTEX_LOCK(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_ja_open(ctx, buffer); }
                MUTEX_UNLOCK(s->lock);
                break;
              case GRN_COLUMN_FIX_SIZE :
                GET_PATH(spec, buffer, s, id);
                MUTEX_LOCK(s->lock);
                if (!*vp) { *vp = (grn_obj *)grn_ra_open(ctx, buffer); }
                MUTEX_UNLOCK(s->lock);
                break;
              case GRN_COLUMN_INDEX :
                GET_PATH(spec, buffer, s, id);
                {
                  grn_obj *table = grn_ctx_at(ctx, spec->header.domain);
                  MUTEX_LOCK(s->lock);
                  if (!*vp) { *vp = (grn_obj *)grn_ii_open(ctx, buffer, table); }
                  MUTEX_UNLOCK(s->lock);
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
      if (res->header.type == GRN_EXPR) {
        if (!grn_hash_add(ctx, ctx->impl->qe, &id, sizeof(grn_id), (void **)&vp, NULL)) {
          res = NULL;
        } else {
          *vp = grn_expr_dup(ctx, res);
          res = *vp;
        }
      }
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
    case GRN_ATOM :
    case GRN_BULK :
    case GRN_UVECTOR :
    case GRN_MSG :
      obj->header.type = GRN_VOID;
      if (obj->header.impl_flags & GRN_OBJ_REFER) {
        obj->u.b.head = NULL;
        obj->u.b.tail = NULL;
        rc = GRN_SUCCESS;
      } else {
        rc = grn_bulk_fin(ctx, obj);
      }
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
      GRN_FREE(obj);
      rc = GRN_SUCCESS;
      break;
    case GRN_EXPR :
      rc = grn_expr_close(ctx, obj);
      break;
    }
  }
  GRN_API_RETURN(rc);
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

const char *
grn_obj_path(grn_ctx *ctx, grn_obj *obj)
{
  grn_io *io;
  char *path = NULL;
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
    if (DB_OBJ(obj)->id && DB_OBJ(obj)->id < GRN_ID_MAX) {
      grn_db *s = (grn_db *)DB_OBJ(obj)->db;
      len = grn_pat_get_key(ctx, s->keys, DB_OBJ(obj)->id, namebuf, buf_size);
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

grn_rc
grn_obj_clear_lock(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;
  grn_io_clear_lock(grn_obj_io(obj));
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
  if (GRN_DB_OBJP(obj)) { id = DB_OBJ(obj)->id; }
  GRN_API_RETURN(id);
}

/**** sort ****/

typedef struct {
  grn_id id;
  uint32_t size;
  const void *value;
} sort_entry;

enum {
  KEY_BULK = 0,
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
_sort(grn_ctx *ctx, sort_entry *head, sort_entry *tail, int limit,
      grn_table_sort_key *keys, int n_keys)
{
  sort_entry *c;
  if (head < tail && (c = part(ctx, head, tail, keys, n_keys))) {
    intptr_t rest = limit - 1 - (c - head);
    _sort(ctx, head, c - 1, limit, keys, n_keys);
    if (rest > 0) { _sort(ctx, c + 1, tail, (int)rest, keys, n_keys); }
  }
}

static sort_entry *
pack(grn_ctx *ctx, grn_obj *table, sort_entry *head, sort_entry *tail,
     grn_table_sort_key *keys, int n_keys)
{
  int i = 0;
  sort_entry e, c;
  grn_table_cursor *tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0);
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

int
grn_table_sort(grn_ctx *ctx, grn_obj *table, int limit,
               grn_obj *result, grn_table_sort_key *keys, int n_keys)
{
  int n, i = 0;
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
  if (!(result && result->header.type == GRN_TABLE_NO_KEY)) {
    WARN(GRN_INVALID_ARGUMENT, "result is not a array");
    goto exit;
  }
  n = grn_table_size(ctx, table);
  if (limit <= 0) {
    limit += n;
    if (limit <= 0) {
      WARN(GRN_INVALID_ARGUMENT, "limit is too small in grn_table_sort !");
      goto exit;
    }
  }
  {
    int j;
    grn_table_sort_key *kp;
    for (kp = keys, j = n_keys; j; kp++, j--) {
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
  if (limit > n) { limit = n; }
  if ((ep = pack(ctx, table, array, array + n - 1, keys, n_keys))) {
    intptr_t rest = limit - 1 - (ep - array);
    _sort(ctx, array, ep - 1, limit, keys, n_keys);
    if (rest > 0 ) {
      _sort(ctx, ep + 1, array + n - 1, (int)rest, keys, n_keys);
    }
  }
  {
    grn_id *v;
    for (i = 0, ep = array; i < limit; i++, ep++) {
      if (!grn_array_add(ctx, (grn_array *)result, (void **)&v)) { break; }
      if (!(*v = ep->id)) { break; }
    }
    GRN_FREE(array);
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

#define N_RESERVED_TYPES 255

grn_rc
grn_db_init_builtin_types(grn_ctx *ctx)
{
  grn_obj *obj;
  obj = deftype(ctx, "<int>",
                GRN_OBJ_KEY_INT, sizeof(int32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT32) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<uint>",
                GRN_OBJ_KEY_UINT, sizeof(uint32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT32) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<int64>",
                GRN_OBJ_KEY_INT, sizeof(int64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT64) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<uint64>",
                GRN_OBJ_KEY_UINT, sizeof(uint64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT64) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<float>",
                GRN_OBJ_KEY_FLOAT, sizeof(double));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_FLOAT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<time>",
                GRN_OBJ_KEY_UINT, sizeof(grn_timeval));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TIME) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<shorttext>",
                GRN_OBJ_KEY_VAR_SIZE, GRN_TABLE_MAX_KEY_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_SHORTTEXT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<text>",
                GRN_OBJ_KEY_VAR_SIZE, 1 << 16);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TEXT) { return GRN_FILE_CORRUPT; }
  obj = deftype(ctx, "<longtext>",
                GRN_OBJ_KEY_VAR_SIZE, 1 << 31);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_LONGTEXT) { return GRN_FILE_CORRUPT; }
  grn_db_init_builtin_tokenizers(ctx);
  {
    grn_obj *db = ctx->impl->db;
    grn_id id = grn_pat_curr_id(ctx, ((grn_db *)db)->keys);
    char buf[] = "<sys:00>";
    while (id < N_RESERVED_TYPES) {
      grn_itoh(++id, buf + 5, 2);
      buf[5 + 2] = '>';
      grn_obj_register(ctx, db, buf, 8);
    }
  }
  return ctx->rc;
}

/* grn_expr */

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
  if (ctx && ctx->impl && ctx->impl->stack_curr < 16) {
    ctx->impl->stack[ctx->impl->stack_curr++] = obj;
    return GRN_SUCCESS;
  }
  return GRN_NO_MEMORY_AVAILABLE;
}

#define APPEND_OBJ(p,n,x) {\
  if (!((n) % 16)) {\
    grn_obj *p0 = GRN_REALLOC((p), sizeof(grn_obj) * ((n) + 16));\
    if (p0) {\
      (p) = p0;\
      (x) = (p) + (n)++;\
    } else {\
      (x) = NULL;\
    }\
  } else {\
    (x) = (p) + (n)++;\
  }\
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
  grn_obj *n, *v;
  grn_expr_code *c;
  grn_expr *e = (grn_expr *)expr;
  uint32_t i = e->nvars;
  grn_text_benc(ctx, buf, i);
  for (n = e->names, v = e->vars; i; i--, v++, n++) {
    uint32_t ns = GRN_TEXT_LEN(n);
    grn_text_benc(ctx, buf, ns);
    if (ns) { GRN_TEXT_PUT(ctx, buf, GRN_TEXT_VALUE(n), ns); }
    grn_obj_pack(ctx, buf, v);
  }
  i = e->codes_curr;
  grn_text_benc(ctx, buf, i);
  for (c = e-> codes; i; i--, c++) {
    grn_text_benc(ctx, buf, c->op);
    if (!c->value) {
      grn_text_benc(ctx, buf, 0); /* NULL */
    } else if (e->vars <= c->value && c->value < e->vars + e->nvars) {
      grn_text_benc(ctx, buf, 1); /* variable */
      grn_text_benc(ctx, buf, c->value - e->vars);
    } else {
      grn_text_benc(ctx, buf, 2); /* others */
      grn_obj_pack(ctx, buf, c->value);
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
    GRN_B_DEC(type, p);
    switch (type) {
    case 0 : /* NULL */
      code->value = NULL;
      break;
    case 1 : /* variable */
      {
        uint32_t offset;
        GRN_B_DEC(offset, p);
        code->value = e->vars + offset;
      }
      break;
    case 2 : /* others */
      GRN_B_DEC(type, p);
      if (GRN_TYPE <= type && type <= GRN_COLUMN_INDEX) {
        grn_id id;
        GRN_B_DEC(id, p);
        code->value = grn_ctx_at(ctx, id);
      } else {
        APPEND_OBJ(e->consts, e->nconsts, v);
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
    int size = 10;
    expr->consts = NULL;
    expr->nconsts = 0;
    expr->names = NULL;
    expr->vars = NULL;
    expr->nvars = 0;
    GRN_DB_OBJ_SET_TYPE(expr, GRN_EXPR);
    if ((expr->values = GRN_MALLOCN(grn_obj, size))) {
      int i;
      for (i = 0; i < size; i++) {
        GRN_OBJ_INIT(&expr->values[i], GRN_ATOM, GRN_OBJ_EXPRVALUE, GRN_ID_NIL);
      }
      expr->values_curr = 0;
      expr->values_tail = 0;
      expr->values_size = size;
      if ((expr->codes = GRN_MALLOCN(grn_expr_code, size))) {
        expr->codes_curr = 0;
        expr->codes_size = size;
        if ((expr->stack = GRN_MALLOCN(grn_obj *, size))) {
          expr->stack_curr = 0;
          expr->stack_size = size;
          expr->obj.header = spec->header;
          if (grn_expr_unpack(ctx, p, pe, (grn_obj *)expr) == pe) {
            goto exit;
          } else {
            ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
          }
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
  GRN_API_ENTER;
  if (check_name(ctx, name, name_size)) {
    ERR(GRN_INVALID_ARGUMENT, "name contains '%c'", GRN_DB_DELIMITER);
    GRN_API_RETURN(NULL);
  }
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  }
  id = grn_obj_register(ctx, db, name, name_size);
  if (id && (expr = GRN_MALLOCN(grn_expr, 1))) {
    int size = 256;
    expr->consts = NULL;
    expr->nconsts = 0;
    expr->names = NULL;
    expr->vars = NULL;
    expr->nvars = 0;
    expr->values_curr = 0;
    expr->values_tail = 0;
    expr->values_size = size;
    expr->codes_curr = 0;
    expr->codes_size = size;
    expr->stack_curr = 0;
    expr->stack_size = size;
    GRN_DB_OBJ_SET_TYPE(expr, GRN_EXPR);
    expr->obj.header.domain = GRN_ID_NIL;
    expr->obj.range = GRN_ID_NIL;
    if (!grn_db_obj_init(ctx, db, id, DB_OBJ(expr))) {
      if ((expr->values = GRN_MALLOCN(grn_obj, size))) {
        int i;
        for (i = 0; i < size; i++) {
          GRN_OBJ_INIT(&expr->values[i], GRN_ATOM, GRN_OBJ_EXPRVALUE, GRN_ID_NIL);
        }
        if ((expr->codes = GRN_MALLOCN(grn_expr_code, size))) {
          if ((expr->stack = GRN_MALLOCN(grn_obj *, size))) {
            goto exit;
          }
          GRN_FREE(expr->codes);
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

static grn_obj *
grn_expr_dup(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr *res = (grn_expr *)grn_expr_create(ctx, NULL, 0);
  if (res) {
    uint32_t i;
    grn_expr_code *c, *c0;
    grn_obj *n0, *v0, *v;
    res->obj.header.domain = e->obj.id;
    for (n0 = e->names, v0 = e->vars, i = e->nvars; i; n0++, v0++, i--) {
      if ((v = grn_expr_add_var(ctx, (grn_obj *)res,
                                GRN_TEXT_VALUE(n0), GRN_TEXT_LEN(n0)))) {
        GRN_OBJ_INIT(v, v0->header.type, 0, v0->header.domain);
        GRN_TEXT_PUT(ctx, v, GRN_TEXT_VALUE(v0), GRN_TEXT_LEN(v0));
      }
    }
    for (c0 = e->codes, c = res->codes, i = e->codes_curr; i; c0++, c++, i--) {
      off_t g = c0->value - e->vars;
      c->op = c0->op;
      c->value = (0 <= g && g < e->nvars) ? res->vars + g : c0->value;
    }
    res->codes_curr = e->codes_curr;
  }
  return (grn_obj *)res;
}

grn_rc
grn_expr_close(grn_ctx *ctx, grn_obj *expr)
{
  uint32_t i;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (e->obj.header.domain) {
    grn_hash_delete(ctx, ctx->impl->qe, &e->obj.header.domain, sizeof(grn_id), NULL);
  }
  for (i = 0; i < e->nconsts; i++) {
    grn_obj_close(ctx, &e->consts[i]);
  }
  GRN_REALLOC(e->consts, 0);
  for (i = 0; i < e->nvars; i++) {
    grn_obj_close(ctx, &e->names[i]);
    grn_obj_close(ctx, &e->vars[i]);
  }
  GRN_REALLOC(e->vars, 0);
  GRN_REALLOC(e->names, 0);
  for (i = 0; i < e->values_curr; i++) {
    grn_obj_close(ctx, &e->values[i]);
  }
  GRN_FREE(e->values);
  GRN_FREE(e->stack);
  GRN_FREE(e->codes);
  GRN_FREE(e);
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_expr_add_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned name_size)
{
  uint32_t n;
  grn_obj *name_obj, *res;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  n = e->nvars;
  APPEND_OBJ(e->names, n, name_obj);
  APPEND_OBJ(e->vars, e->nvars, res);
  if (name_obj) {
    GRN_TEXT_INIT(name_obj, 0);
    GRN_TEXT_PUT(ctx, name_obj, name, name_size);
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_get_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned name_size)
{
  grn_obj *obj, *res;
  grn_expr *e = (grn_expr *)expr;
  uint32_t n = e->nvars;
  for (obj = e->names, res = e->vars; n--; res++, obj++) {
    if (GRN_BULK_VSIZE(obj) == name_size &&
        !(memcmp(GRN_BULK_HEAD(obj), name, name_size))) {
      return res;
    }
  }
  return NULL;
}

grn_obj *
grn_expr_get_var_by_offset(grn_ctx *ctx, grn_obj *expr, unsigned int offset)
{
  grn_expr *e = (grn_expr *)expr;
  return (offset < e->nvars) ? &e->vars[offset] : NULL;
}

static void
grn_expr_append_code(grn_ctx *ctx, grn_expr *expr, grn_obj *obj, grn_op op)
{
  if (expr->codes_curr >= expr->codes_size) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "stack is full");
  } else {
    grn_expr_code *code = &expr->codes[expr->codes_curr++];
    code->op = op;
    code->value = obj;
    expr->stack[expr->stack_curr++] = obj;
  }
}

#define EXPR_P0(expr) ((expr)->stack[(expr)->stack_curr - 1])
#define EXPR_P1(expr) ((expr)->stack[(expr)->stack_curr - 2])

#define EXPR_POP(x,expr) {\
  (x) = (expr)->stack[--(expr)->stack_curr];\
  if (EXPRVP(x)) { (expr)->values_curr--; }\
}

#define EXPR_PUSH(x,expr) {\
  (expr)->stack[(expr)->stack_curr++] = (x);\
}

#define EXPR_PUSH_ALLOC(x,expr) {\
  grn_expr_stack *_s;\
  if ((expr)->values_tail < (expr)->values_curr) {\
    (x) = &(expr)->values[(expr)->values_tail++];\
    grn_obj_close(ctx, (x));\
  } else {\
    (x) = &(expr)->values[(expr)->values_curr++];\
    (expr)->values_tail = (expr)->values_curr;\
  }\
  _s = &(expr)->stack[(expr)->stack_curr++];\
  _s->value = (x);\
  _s->flags = 1;\
}

#define EXPRVP(x) ((x)->header.impl_flags & GRN_OBJ_EXPRVALUE)

#define EXPR_ALLOC(x,expr,type,domain) {\
  (x) = &(expr)->values[(expr)->values_curr++];\
  GRN_OBJ_INIT(x, type, GRN_OBJ_EXPRVALUE, domain);\
}

grn_obj *
grn_expr_append_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  switch (obj->header.type) {
  case GRN_PROC :
    {
      grn_obj *o;
      uint32_t i;
      grn_proc *p = (grn_proc *)obj;
      grn_expr_code *code = &e->codes[e->codes_curr++];
      code->op = GRN_OP_CALL;
      code->value = obj;
      for (i = p->nargs; i; i--) { EXPR_POP(o, e); }
      for (i = p->nresults, o = p->results; i; i--, o++) { EXPR_PUSH(o, e); }
    }
    break;
  default :
    grn_expr_append_code(ctx, e, obj, GRN_OP_PUSH);
    break;
  }
  if (!ctx->rc) { res = obj; }
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_append_const(grn_ctx *ctx, grn_obj *expr, grn_obj *obj)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  if (!obj) { return NULL; }
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    res = obj;
  } else {
    APPEND_OBJ(e->consts, e->nconsts, res);
    if (res) {
      switch (obj->header.type) {
      case GRN_ATOM :
        memcpy(res, obj, sizeof(grn_obj));
        break;
      case GRN_BULK :
      case GRN_UVECTOR :
        GRN_OBJ_INIT(res, obj->header.type, 0, obj->header.domain);
        grn_bulk_write(ctx, res, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
        break;
      default :
        res = NULL;
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "unspported type");
        goto exit;
      }
      res->header.impl_flags |= GRN_OBJ_EXPRCONST;
    }
  }
  grn_expr_append_code(ctx, e, res, GRN_OP_PUSH); /* constant */
exit :
  GRN_API_RETURN(res);
}

#define CONSTP(obj) ((obj)->header.impl_flags & GRN_OBJ_EXPRCONST)

#define PUSH_CODE(expr,o,v) {\
  grn_expr_code *code = &(expr)->codes[(expr)->codes_curr++];\
  code->op = (o);\
  code->value = (v);\
}

grn_rc
grn_expr_append_op(grn_ctx *ctx, grn_obj *expr, grn_op op, int nargs)
{
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (e->codes_curr >= e->codes_size) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "stack is full");
  } else {
    grn_expr_code *code;
    switch (op) {
    case GRN_OP_PUSH :
    case GRN_OP_CALL :
      ERR(GRN_INVALID_ARGUMENT, "invalid operator assigned");
      break;
    case GRN_OP_INTERN :
      {
        grn_obj *obj = EXPR_P0(e);
        if (CONSTP(obj)) {
          grn_obj *value;
          EXPR_POP(obj, e);
          value = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj));
          if (!value) { value = grn_ctx_get(ctx, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj)); }
          if (!value) {
            ERR(GRN_INVALID_ARGUMENT, "intern failed");
            goto exit;
          }
          PUSH_CODE(e, GRN_OP_PUSH, value);
        } else {
          PUSH_CODE(e, op, NULL);
        }
      }
      break;
    case GRN_OP_TABLE_CREATE :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_VAR_SET_VALUE :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_OBJ_GET_VALUE :
    case GRN_OP_OBJ_SET_VALUE :
      {
        grn_obj *xv, *yv, *obj, *col, *rv;
        code = &e->codes[e->codes_curr - 1];
        code->op = op;
        EXPR_POP(xv, e);
        EXPR_POP(yv, e);
        obj = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(yv));
        col = grn_obj_column(ctx, obj, GRN_BULK_HEAD(xv), GRN_BULK_VSIZE(xv));
        code->value = col;
        rv = &e->values[e->values_curr++];
        rv->header.domain = grn_obj_get_range(ctx, col);
        e->stack[e->stack_curr++] = rv;
      }
      break;
    case GRN_OP_OBJ_SEARCH :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_TABLE_SCAN :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_TABLE_SORT :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_TABLE_GROUP :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_JSON_PUT :
      PUSH_CODE(e, op, NULL);
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_EQUAL :
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
      PUSH_CODE(e, op, NULL);
      break;
    }
  }
exit :
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_expr_compile(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj_spec_save(ctx, DB_OBJ(expr));
  return ctx->rc;
}

#define PUSH1(v) {\
  s1 = s0;\
  *sp++ = s0 = v;\
}

#define POP1(v) {\
  if (EXPRVP(s0)) { vp--; }\
  v = s0;\
  s0 = s1;\
  sp--;\
  s1 = sp[-2];\
}

#define POP1PUSH1(arg,value) {\
  arg = s0;\
  if (EXPRVP(s0)) {\
    value = s0;\
  } else {\
    s0 = value = vp++;\
    s0->header.impl_flags = GRN_OBJ_EXPRVALUE;\
  }\
}

#define POP2PUSH1(arg1,arg2,value) {\
  if (EXPRVP(s0)) { vp--; }\
  if (EXPRVP(s1)) { vp--; }\
  arg2 = s0;\
  arg1 = s1;\
  sp--;\
  s1 = sp[-2];\
  s0 = value = vp++;\
  s0->header.impl_flags = GRN_OBJ_EXPRVALUE;\
}

void
grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)
{
  if (obj && (!GRN_DB_OBJP(obj) || (((grn_db_obj *)obj)->id & GRN_OBJ_TMP_OBJECT))) {
    grn_obj_close(ctx, obj);
  }
}

#define WITH_SPSAVE(block) {\
  ctx->impl->stack_curr = sp - ctx->impl->stack;\
  block\
  if (sp != ctx->impl->stack + ctx->impl->stack_curr) {\
    sp = ctx->impl->stack + ctx->impl->stack_curr;\
    s0 = sp[-1];\
    s1 = sp[-2];\
  }\
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
  case GRN_DB_UINT64 :\
  case GRN_DB_TIME :\
    r = (x_ op GRN_UINT64_VALUE(y));\
    break;\
  case GRN_DB_FLOAT :\
    r = (x_ op GRN_FLOAT_VALUE(y));\
    break;\
  case GRN_DB_SHORTTEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONGTEXT :\
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
}\

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
  case GRN_DB_SHORTTEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONGTEXT :\
    if (GRN_DB_SHORTTEXT <= y->header.domain && y->header.domain <= GRN_DB_LONGTEXT) {\
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
  case GRN_DB_UINT64 :\
  case GRN_DB_TIME :\
    r = (x_ == GRN_UINT64_VALUE(y));\
    break;\
  case GRN_DB_FLOAT :\
    r = ((x_ <= GRN_FLOAT_VALUE(y)) && (x_ >= GRN_FLOAT_VALUE(y)));\
    break;\
  case GRN_DB_SHORTTEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONGTEXT :\
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
        r = ((x_ <= GRN_INT64_VALUE(y)) && (x_ >= GRN_INT64_VALUE(y)));\
        break;\
      case GRN_DB_UINT64 :\
      case GRN_DB_TIME :\
        r = ((x_ <= GRN_UINT64_VALUE(y)) && (x_ >= GRN_UINT64_VALUE(y)));\
        break;\
      case GRN_DB_FLOAT :\
        r = ((x_ <= GRN_FLOAT_VALUE(y)) && (x_ >= GRN_FLOAT_VALUE(y)));\
        break;\
      case GRN_DB_SHORTTEXT :\
      case GRN_DB_TEXT :\
      case GRN_DB_LONGTEXT :\
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
  case GRN_DB_SHORTTEXT :\
  case GRN_DB_TEXT :\
  case GRN_DB_LONGTEXT :\
    if (GRN_DB_SHORTTEXT <= y->header.domain && y->header.domain <= GRN_DB_LONGTEXT) {\
      uint32_t la = GRN_TEXT_LEN(x), lb = GRN_TEXT_LEN(y);\
      r =  (la == lb && !memcmp(GRN_TEXT_VALUE(x), GRN_TEXT_VALUE(y), lb));\
    } else {\
      const char *q_ = GRN_TEXT_VALUE(x);\
      int x_ = grn_atoi(q_, q_ + GRN_TEXT_LEN(x), NULL);\
      do_eq_sub;\
    }\
    break;\
  default :\
    r = 0;\
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

grn_obj *
grn_expr_exec(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  {
    register grn_obj *s0 = NULL, *s1 = NULL, **sp, *vp = e->values;
    grn_expr_code *code = e->codes, *ce = &e->codes[e->codes_curr];
    sp = ctx->impl->stack + ctx->impl->stack_curr;
    while (code < ce) {
      switch (code->op) {
      case GRN_OP_PUSH :
        PUSH1(code->value);
        code++;
        break;
      case GRN_OP_CALL :
        {
          grn_proc *p = (grn_proc *)code->value;
          p->funcs[PROC_INIT](ctx, NULL, NULL);
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
          grn_obj *value_size, *key_type, *flags, *name;
          POP1(value_size);
          value_size = GRN_OBJ_RESOLVE(ctx, value_size);
          POP1(key_type);
          key_type = GRN_OBJ_RESOLVE(ctx, key_type);
          POP1(flags);
          flags = GRN_OBJ_RESOLVE(ctx, flags);
          POP1(name);
          name = GRN_OBJ_RESOLVE(ctx, name);
          res = grn_table_create(ctx, GRN_TEXT_VALUE(name), GRN_TEXT_LEN(name),
                                 NULL, GRN_UINT32_VALUE(flags),
                                 key_type, GRN_UINT32_VALUE(value_size));
          PUSH1(res);
        }
        code++;
        break;
      case GRN_OP_VAR_SET_VALUE :
        {
          grn_obj *value, *var;
          POP1(var);
          var = GRN_OBJ_RESOLVE(ctx, var);
          POP1(value);
          value = GRN_OBJ_RESOLVE(ctx, value);
          if (GRN_DB_OBJP(value)) {
            var->header.type = GRN_PTR;
            var->header.domain = DB_OBJ(value)->id;
            var->u.b.head = (char *)value;
          } else {
            // todo : copy obj type.
            GRN_TEXT_SET(ctx, var, GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
          }
        }
        code++;
        break;
      case GRN_OP_OBJ_GET_VALUE :
        {
          grn_obj *col, *rec;
          do {
            uint32_t size;
            const char *value;
            POP1PUSH1(rec, res);
            col = code->value;
            value = grn_obj_get_value_(ctx, col, GRN_RECORD_VALUE(rec), &size);
            grn_bulk_write_from(ctx, res, value, 0, size);
            code++;
          } while (code < ce && code->op == GRN_OP_OBJ_GET_VALUE);
          res->header.domain = grn_obj_get_range(ctx, col);
        }
        break;
      case GRN_OP_OBJ_SET_VALUE :
        {
          grn_rc rc;
          grn_obj *col, *rec, *val;
          POP2PUSH1(val, rec, res);
          col = code->value;
          rc = grn_obj_set_value(ctx, col, GRN_RECORD_VALUE(rec), val, GRN_OBJ_SET);
          res->header.domain = GRN_DB_INT32;
          GRN_INT32_SET(ctx, res, rc);
          code++;
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
                         (grn_sel_operator)GRN_UINT32_VALUE(op), NULL);
        }
        code++;
        break;
      case GRN_OP_TABLE_SCAN :
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
            grn_table_scan(ctx, table, expr, res, (grn_sel_operator)GRN_UINT32_VALUE(op));
          });
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
            char *p = GRN_BULK_HEAD(keys_), *tokbuf[256];
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
                grn_table_sort(ctx, table, GRN_INT32_VALUE(limit), res, keys, n_keys);
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
            char *p = GRN_BULK_HEAD(keys_), *tokbuf[256];
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
              results.op = GRN_SEL_OR;
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
          grn_obj *str, *table, *res;
          POP1(res);
          res = GRN_OBJ_RESOLVE(ctx, res);
          POP1(str);
          str = GRN_OBJ_RESOLVE(ctx, str);
          POP1(table);
          table = GRN_OBJ_RESOLVE(ctx, table);
          {
            grn_obj_format format;
            grn_obj *cols[256];
            char *p = GRN_BULK_HEAD(str), *tokbuf[256];
            int i, n = grn_str_tok(p, GRN_BULK_VSIZE(str), ' ', tokbuf, 256, NULL);
            for (i = 0; i < n; i++) {
              cols[i] = grn_obj_column(ctx, table, p, tokbuf[i] - p);
              p = tokbuf[i] + 1;
            }
            format.ncolumns = n;
            format.columns = cols;
            grn_text_otoj(ctx, res, table, &format);
            for (i = 0; i < n; i++) {
              grn_obj_unlink(ctx, cols[i]);
            }
          }
        }
        code++;
        break;
      case GRN_OP_AND :
        {
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          if (GRN_INT32_VALUE(x) == 0 || GRN_INT32_VALUE(y) == 0) {
            GRN_INT32_SET(ctx, res, 0);
          } else {
            GRN_INT32_SET(ctx, res, 1);
          }
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_OR :
        {
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          if (GRN_INT32_VALUE(x) == 0 && GRN_INT32_VALUE(y) == 0) {
            GRN_INT32_SET(ctx, res, 0);
          } else {
            GRN_INT32_SET(ctx, res, 1);
          }
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          do_eq(x, y, r);
          GRN_INT32_SET(ctx, res, r);
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_NOT_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          do_eq(x, y, r);
          GRN_INT32_SET(ctx, res, 1 - r);
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_LESS :
        {
          int r;
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          do_compare(x, y, r, <);
          GRN_INT32_SET(ctx, res, r);
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GREATER :
        {
          int r;
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          do_compare(x, y, r, >);
          GRN_INT32_SET(ctx, res, r);
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_LESS_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          do_compare(x, y, r, <=);
          GRN_INT32_SET(ctx, res, r);
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      case GRN_OP_GREATER_EQUAL :
        {
          int r;
          grn_obj *x, *y;
          POP2PUSH1(x, y, res);
          do_compare(x, y, r, >=);
          GRN_INT32_SET(ctx, res, r);
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
          POP1PUSH1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
          y = (lat2 - lat1);
          d = sqrt((x * x) + (y * y)) * GEO_RADIOUS;
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
          POP1PUSH1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          x = sin(fabs(lng2 - lng1) * 0.5);
          y = sin(fabs(lat2 - lat1) * 0.5);
          d = asin(sqrt((y * y) + cos(lat1) * cos(lat2) * x * x)) * 2 * GEO_RADIOUS;
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
          POP1PUSH1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          p = (lat1 + lat2) * 0.5;
          q = (1 - GEO_BES_C3 * sin(p) * sin(p));
          m = GEO_BES_C1 / sqrt(q * q * q);
          n = GEO_BES_C2 / sqrt(q);
          x = n * cos(p) * fabs(lng1 - lng2);
          y = m * fabs(lat1 - lat2);
          d = sqrt((x * x) + (y * y));
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
          POP1PUSH1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          p = (lat1 + lat2) * 0.5;
          q = (1 - GEO_GRS_C3 * sin(p) * sin(p));
          m = GEO_GRS_C1 / sqrt(q * q * q);
          n = GEO_GRS_C2 / sqrt(q);
          x = n * cos(p) * fabs(lng1 - lng2);
          y = m * fabs(lat1 - lat2);
          d = sqrt((x * x) + (y * y));
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
          POP1PUSH1(e, res);
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
          POP1PUSH1(e, res);
          lat2 = GEO_INT2RAD(GRN_INT32_VALUE(e));
          x = (lng1 - lng0) * cos((lat0 + lat1) * 0.5);
          y = (lat1 - lat0);
          d = (x * x) + (y * y);
          x = (lng2 - lng1) * cos((lat1 + lat2) * 0.5);
          y = (lat2 - lat1);
          r = d <= (x * x) + (y * y);
          GRN_INT32_SET(ctx, res, r);
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
          POP1PUSH1(e, res);
          la3 = GRN_INT32_VALUE(e);
          r = ((ln2 <= ln0) && (ln0 <= ln3) && (la2 <= la0) && (la0 <= la3));
          GRN_INT32_SET(ctx, res, r);
          res->header.domain = GRN_DB_INT32;
        }
        code++;
        break;
      }
    }
    res = s0;
    if (res) { sp--; }
    ctx->impl->stack_curr = sp - ctx->impl->stack;
  }
exit :
  GRN_API_RETURN(res);
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
        grn_sel_operator op)
{
  grn_rset_recinfo *ri;
  switch (op) {
  case GRN_SEL_OR :
    if (grn_hash_add(ctx, s, pi, s->key_size, (void **)&ri, NULL)) {
      grn_table_add_subrec((grn_obj *)s, ri, score, pi, 1);
    }
    break;
  case GRN_SEL_AND :
    if (grn_hash_get(ctx, s, pi, s->key_size, (void **)&ri)) {
      ri->n_subrecs |= GRN_RSET_UTIL_BIT;
      grn_table_add_subrec((grn_obj *)s, ri, score, pi, 1);
    }
    break;
  case GRN_SEL_BUT :
    {
      grn_id id;
      if ((id = grn_hash_get(ctx, s, pi, s->key_size, (void **)&ri))) {
        grn_hash_delete_by_id(ctx, s, id, NULL);
      }
    }
    break;
  case GRN_SEL_ADJUST :
    if (grn_hash_get(ctx, s, pi, s->key_size, (void **)&ri)) {
      ri->score += score;
    }
    break;
  }
}

grn_rc
grn_table_scan(grn_ctx *ctx, grn_obj *table, grn_obj *expr,
               grn_obj *res, grn_sel_operator op)
{
  if (res->header.type != GRN_TABLE_HASH_KEY ||
      (res->header.domain != DB_OBJ(table)->id)) {
    ERR(GRN_INVALID_ARGUMENT, "hash table required");
    return ctx->rc;
  }
  GRN_API_ENTER;
  {
    int32_t score;
    grn_id id, *idp;
    grn_table_cursor *tc;
    grn_hash_cursor *hc;
    grn_hash *s = (grn_hash *)res;
    grn_obj *r, *v = grn_expr_get_var_by_offset(ctx, expr, 0);
    if (!v) {
      ERR(GRN_INVALID_ARGUMENT, "at least one variable must be defined");
      goto exit;
    }
    GRN_RECORD_INIT(v, 0, grn_obj_id(ctx, table));
    switch (op) {
    case GRN_SEL_OR :
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0))) {
        while ((id = grn_table_cursor_next(ctx, tc))) {
          GRN_RECORD_SET(ctx, v, id);
          r = grn_expr_exec(ctx, expr);
          if ((score = GRN_UINT32_VALUE(r))) {
            grn_rset_recinfo *ri;
            if (grn_hash_add(ctx, s, &id, s->key_size, (void **)&ri, NULL)) {
              grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)&id, 1);
            }
          }
        }
        grn_table_cursor_close(ctx, tc);
      }
      break;
    case GRN_SEL_AND :
      if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0))) {
        while (grn_hash_cursor_next(ctx, hc)) {
          grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
          GRN_RECORD_SET(ctx, v, *idp);
          r = grn_expr_exec(ctx, expr);
          if ((score = GRN_UINT32_VALUE(r))) {
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
    case GRN_SEL_BUT :
      if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0))) {
        while (grn_hash_cursor_next(ctx, hc)) {
          grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
          GRN_RECORD_SET(ctx, v, *idp);
          r = grn_expr_exec(ctx, expr);
          if ((score = GRN_UINT32_VALUE(r))) {
            grn_hash_cursor_delete(ctx, hc, NULL);
          }
        }
        grn_hash_cursor_close(ctx, hc);
      }
      break;
    case GRN_SEL_ADJUST :
      if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0))) {
        while (grn_hash_cursor_next(ctx, hc)) {
          grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
          GRN_RECORD_SET(ctx, v, *idp);
          r = grn_expr_exec(ctx, expr);
          if ((score = GRN_UINT32_VALUE(r))) {
            grn_rset_recinfo *ri;
            grn_hash_cursor_get_value(ctx, hc, (void **) &ri);
            grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)idp, 1);
          }
        }
        grn_hash_cursor_close(ctx, hc);
      }
      break;
    }
  }
exit :
  GRN_API_RETURN(ctx->rc);
}
