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
  if ((table)->obj.flags & GRN_OBJ_KEY_NORMALIZE) {\
    grn_str *nstr;\
    if ((nstr = grn_str_open(ctx, key, key_size, (table)->encoding, 0))) { \
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
  grn_encoding encoding;
  GRN_API_ENTER;
  encoding = ctx->encoding;
  if (!path || strlen(path) <= PATH_MAX - 14) {
    if ((s = GRN_MALLOC(sizeof(grn_db)))) {
      grn_tiny_array_init(&s->values, ctx, sizeof(grn_obj *),
                          GRN_TINY_ARRAY_CLEAR|
                          GRN_TINY_ARRAY_THREADSAFE|
                          GRN_TINY_ARRAY_USE_MALLOC);
      if ((s->keys = grn_pat_create(ctx, path, GRN_PAT_MAX_KEY_SIZE, 0,
                                    GRN_OBJ_KEY_VAR_SIZE, encoding))) {
        MUTEX_INIT(s->lock);
        GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
        grn_db_obj_init(ctx, NULL, GRN_ID_NIL, &s->obj);
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
      grn_tiny_array_init(&s->values, ctx, sizeof(grn_obj *),
                          GRN_TINY_ARRAY_CLEAR|
                          GRN_TINY_ARRAY_THREADSAFE|
                          GRN_TINY_ARRAY_USE_MALLOC);
      if ((s->keys = grn_pat_open(ctx, path))) {
        char buffer[PATH_MAX];
        gen_pathname(path, buffer, 0);
        if ((s->specs = grn_ja_open(ctx, buffer))) {
          MUTEX_INIT(s->lock);
          GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
          grn_db_obj_init(ctx, NULL, GRN_ID_NIL, &s->obj);
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
  grn_tiny_array_fin(&s->values);
  grn_pat_close(ctx, s->keys);
  if (s->specs) { grn_ja_close(ctx, s->specs); }
  GRN_FREE(s);
  GRN_API_RETURN(GRN_SUCCESS);
}

static grn_id grn_obj_register(grn_ctx *ctx, grn_obj *db,
                               const char *name, unsigned name_size);
static grn_rc grn_obj_delete_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, int removep);

grn_obj *
grn_ctx_lookup(grn_ctx *ctx, const char *name, unsigned name_size)
{
  grn_id id;
  grn_search_flags flags = 0;
  grn_obj *obj = NULL;
  grn_obj *db = ctx->impl->db;
  GRN_API_ENTER;
  if (DB_P(db)) {
    grn_db *s = (grn_db *)db;
    if ((id = grn_pat_lookup(ctx, s->keys, name, name_size, NULL, &flags))) {
      obj = grn_ctx_get(ctx, id);
    }
  }
  GRN_API_RETURN(obj);
}

grn_obj *
grn_ctx_db(grn_ctx *ctx)
{
  return (ctx->impl) ? ctx->impl->db : NULL;
}

#define GRN_PROC_INIT_PREFIX "grn_init_"

grn_rc
grn_db_load(grn_ctx *ctx, const char *path)
{
  grn_id id;
  grn_obj *db;
  GRN_API_ENTER;
  db = ctx->impl->db;
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
        ERR(grn_invalid_format, "init_func not found(%s)", buffer);
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
  const char *name_end = name + name_size;
  while (name < name_end) {
    if (*name++ == GRN_DB_DELIMITER) {
      return GRN_INVALID_ARGUMENT;
    }
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
  grn_obj *db = ctx->impl->db;
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
                const char *name, unsigned name_size,
                const char *path, grn_proc_type type,
                grn_proc_func *init, grn_proc_func *next, grn_proc_func *fin)
{
  grn_proc *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id domain;
  grn_search_flags f = GRN_TABLE_ADD;
  grn_obj *db = ctx->impl->db;
  GRN_API_ENTER;
  domain = path ? grn_dl_lookup(ctx, path) : GRN_ID_NIL;
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
    if (!(id = grn_pat_lookup(ctx, s->keys, name, name_size, NULL, &f))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_pat_lookup failed");
      GRN_API_RETURN(NULL);
    }
    if (!(f & GRN_TABLE_ADDED)) {
      res = (grn_proc *)grn_ctx_get(ctx, id);
      if (res && res->funcs[PROC_INIT]) {
        ERR(GRN_INVALID_ARGUMENT, "already used name");
        GRN_API_RETURN(NULL);
      }
    }
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
    if (f & GRN_TABLE_ADDED) {
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
                 grn_obj *key_type, unsigned value_size, grn_encoding encoding)
{
  grn_id id;
  unsigned char impl_flags = 0;
  grn_id domain = GRN_ID_NIL;
  uint32_t key_size, max_n_subrecs;
  uint8_t subrec_size, subrec_offset;
  grn_obj *res = NULL;
  grn_obj *db = ctx->impl->db;
  char buffer[PATH_MAX];
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
      impl_flags |= GRN_OBJ_CUSTOM_NAME;
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
    res = (grn_obj *)grn_hash_create(ctx, path, key_size, value_size, flags, encoding);
    break;
  case GRN_OBJ_TABLE_PAT_KEY :
    res = (grn_obj *)grn_pat_create(ctx, path, key_size, value_size, flags, encoding);
    break;
  case GRN_OBJ_TABLE_NO_KEY :
    res = (grn_obj *)grn_array_create(ctx, path, value_size, flags);
    break;
  }
  if (res) {
    DB_OBJ(res)->header.impl_flags = impl_flags;
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->range = GRN_ID_NIL;
    DB_OBJ(res)->max_n_subrecs = max_n_subrecs;
    DB_OBJ(res)->subrec_size = subrec_size;
    DB_OBJ(res)->subrec_offset = subrec_offset;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
      grn_obj_close(ctx, res);
      grn_obj_remove(ctx, path);
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
  grn_obj *db = ctx->impl->db;
  GRN_API_ENTER;
  if (!DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  } else {
    grn_obj *res = grn_ctx_lookup(ctx, name, name_size);
    if (res) {
      const char *path2 = grn_obj_path(ctx, res);
      if (path && (!path2 || strcmp(path, path2))) { GRN_API_RETURN(NULL); }
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
        res->header.impl_flags |= GRN_OBJ_CUSTOM_NAME;
        res->header.domain = GRN_ID_NIL; /* unknown */
        grn_db_obj_init(ctx, db, id, DB_OBJ(res));
      }
    }
    GRN_API_RETURN(res);
  }
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
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        if (*flags & GRN_SEARCH_LCP) {
          id = grn_pat_lcp_search(ctx, (grn_pat *)table, key, key_size);
        } else {
          id = grn_pat_lookup(ctx, (grn_pat *)table, key, key_size, NULL, flags);
        }
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_lookup(ctx, (grn_hash *)table, key, key_size, NULL, flags);
      });
      break;
    }
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_add(grn_ctx *ctx, grn_obj *table)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_NO_KEY :
      id = grn_array_add(ctx, (grn_array *)table, NULL);
      break;
    default :
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
            r = grn_array_get_value(ctx, a, id, bulk->u.b.curr - a->value_size);
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

grn_id
grn_table_at(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
             void **value)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_at(ctx, (grn_pat *)table, key, key_size, value);
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_at(ctx, (grn_hash *)table, key, key_size, value);
      });
      break;
    }
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_get(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
              void **value, grn_search_flags *flags)
{
  grn_id id = GRN_ID_NIL;
  if (!key || !key_size) { return GRN_ID_NIL; }
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY :
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_get(ctx, (grn_pat *)table, key, key_size, value, flags);
      });
      break;
    case GRN_TABLE_HASH_KEY :
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_get(ctx, (grn_hash *)table, key, key_size, value, flags);
      });
      break;
    }
  }
  GRN_API_RETURN(id);
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
      if (flags) { *flags = ((grn_pat *)table)->obj.flags; }
      if (encoding) { *encoding = ((grn_pat *)table)->encoding; }
      if (tokenizer) { *tokenizer = ((grn_pat *)table)->tokenizer; }
      rc = GRN_SUCCESS;
      break;
    case GRN_TABLE_HASH_KEY :
      if (flags) { *flags = ((grn_hash *)table)->obj.flags; }
      if (encoding) { *encoding = ((grn_hash *)table)->encoding; }
      if (tokenizer) { *tokenizer = ((grn_hash *)table)->tokenizer; }
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
  if (DB_OBJ(table)->flags & GRN_OBJ_WITH_SUBREC) {
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
            grn_search_flags f = 0;
            grn_id id = grn_pat_lookup(ctx, pat, key, key_size, NULL, &f);
            if (id) {
              grn_search_flags fl = GRN_TABLE_ADD;
              grn_table_lookup(ctx, res, &id, sizeof(grn_id), &fl);
            }
          }
          // todo : support op;
          break;
        case GRN_SEARCH_LCP :
          {
            grn_id id = grn_pat_lcp_search(ctx, pat, key, key_size);
            if (id) {
              grn_search_flags fl = GRN_TABLE_ADD;
              grn_table_lookup(ctx, res, &id, sizeof(grn_id), &fl);
            }
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
                grn_search_flags fl = GRN_TABLE_ADD;
                grn_table_lookup(ctx, res, &tid, sizeof(grn_id), &fl);
                /* todo : nsubrec++ if GRN_OBJ_TABLE_SUBSET assigned */
              }
              if (!(len = grn_charlen(ctx, sp, se, pat->encoding))) { break; }
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
      grn_search_flags f = 0;
      grn_id id;
      WITH_NORMALIZE(hash, key, key_size, {
        id = grn_hash_lookup(ctx, hash, key, key_size, NULL, &f);
      });
      if (id) {
        grn_search_flags fl = GRN_TABLE_ADD;
        grn_table_lookup(ctx, res, &id, sizeof(grn_id), &fl);
      }
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
        const void *key = GRN_BULK_VALUE(query);
        uint32_t key_size = GRN_BULK_LEN(query);
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
        {
          const char *str = GRN_BULK_VALUE(query);
          unsigned int str_len = GRN_BULK_LEN(query);
          rc = grn_ii_sel(ctx, (grn_ii *)obj, str, str_len, (grn_hash *)res);
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
    GRN_OBJ_INIT(&bulk, GRN_BULK, 0);
    {
      grn_table_cursor *tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0);
      if (tc) {
        grn_id id;
        while ((id = grn_table_cursor_next(ctx, tc))) {
          grn_rset_recinfo *ri = NULL;
          GRN_BULK_REWIND(&bulk);
          if (DB_OBJ(table)->flags & GRN_OBJ_WITH_SUBREC) {
            grn_table_cursor_get_value(ctx, tc, (void **)&ri);
          }
          for (k = 0, kp = keys; k < n_keys; k++, kp++) {
            kp->offset = GRN_BULK_VSIZE(&bulk);
            grn_obj_get_value(ctx, kp->key, id, &bulk);
          }
          for (r = 0, rp = results; r < n_results; r++, rp++) {
            void *value;
            grn_search_flags f = GRN_TABLE_ADD;
            int begin = keys[rp->key_begin].offset;
            int end = rp->key_end >= n_keys
              ? GRN_BULK_VSIZE(&bulk)
              : keys[rp->key_end].offset;
            key = bulk.u.b.head + begin;
            if (grn_table_get(ctx, rp->table, key, end - begin, &value, &f)) {
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
      grn_search_flags f = GRN_TABLE_ADD;
      grn_table_get(ctx, table1, key, key_size, &value1, &f);
      memcpy(value1, value2, value_size);
    });
    break;
  case GRN_SEL_AND :
    GRN_TABLE_EACH(ctx, table1, 0, 0, id, &key, &key_size, &value1, {
      if (!grn_table_at(ctx, table2, key, key_size, &value2)) {
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
      if (grn_table_at(ctx, table1, key, key_size, &value1)) {
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
      if ((id1 = grn_table_at(ctx, table1, key, key_size, NULL))) {
        _grn_table_delete_by_id(ctx, table1, id1, NULL);
        _grn_table_delete_by_id(ctx, table2, id, NULL);
      }
    });
  } else {
    GRN_TABLE_EACH(ctx, table1, 0, 0, id, &key, &key_size, NULL, {
      grn_id id2;
      if ((id2 = grn_table_at(ctx, table2, key, key_size, NULL))) {
        _grn_table_delete_by_id(ctx, table1, id, NULL);
        _grn_table_delete_by_id(ctx, table2, id2, NULL);
      }
    });
  }
  return GRN_SUCCESS;
}

grn_obj *
grn_table_column(grn_ctx *ctx, grn_obj *table, const char *name, unsigned name_size)
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
          column = grn_ctx_lookup(ctx, buf, len + name_size);
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
    GRN_OBJ_INIT(&bulk, GRN_BULK, 0);
    grn_pat_get_key2(ctx, s->keys, DB_OBJ(table)->id, &bulk);
    GRN_BULK_PUTC(ctx, &bulk, GRN_DB_DELIMITER);
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
  unsigned char impl_flags = 0;
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
      impl_flags |= GRN_OBJ_CUSTOM_NAME;
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
      res = (grn_obj *)grn_ja_create(ctx, path, value_size, 0); //todo : zlib support
    } else {
      res = (grn_obj *)grn_ra_create(ctx, path, value_size);
    }
    break;
  case GRN_OBJ_COLUMN_ARRAY :
  case GRN_OBJ_COLUMN_VERSES :
  case GRN_OBJ_COLUMN_POSTINGS :
    res = (grn_obj *)grn_ja_create(ctx, path, value_size * 16/*todo*/, 0);
    //todo : zlib support
    break;
  case GRN_OBJ_COLUMN_INDEX :
    res = (grn_obj *)grn_ii_create(ctx, path, table, 0); //todo : ii layout support
    break;
  }
  if (res) {
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->range = range;
    DB_OBJ(res)->flags = flags;
    res->header.flags = flags;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res))) {
      grn_obj_close(ctx, res);
      grn_obj_remove(ctx, path);
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
  res = grn_ctx_lookup(ctx, fullname, name_size);
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
      res->header.impl_flags |= GRN_OBJ_CUSTOM_NAME;
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
        void *v = grn_ra_get(ctx, (grn_ra *)pctx->obj, arg->id);
        if (!v) {
          ERR(GRN_NO_MEMORY_AVAILABLE, "ra get failed");
          return GRN_NO_MEMORY_AVAILABLE;
        }
        memcpy(v, in->u.p.ptr, value_size);
      }
      break;
    case GRN_COLUMN_INDEX :
      // todo : how??
      break;
    }
    return GRN_SUCCESS;
  } else {
    return grn_other_error;
  }
}
*/

typedef struct {
  grn_id target;
  unsigned int section;
} default_set_value_hook_data;

static grn_rc
default_set_value_hook(grn_ctx *ctx, grn_obj *obj, grn_proc_data *user_data,
                       int argc, grn_proc_data *argv)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  if (!pctx || argc != 4) {
    ERR(GRN_INVALID_ARGUMENT, "default_set_value_hook failed");
    return GRN_INVALID_ARGUMENT;
  } else {
    //grn_obj *obj = pctx->obj;
    grn_id id = argv[0].id;
    grn_obj *oldvalue = argv[1].ptr;
    grn_obj *newvalue = argv[2].ptr;
    //    int flags = argv[3].int_value;
    grn_hook *h = pctx->currh;
    default_set_value_hook_data *data = (void *)NEXT_ADDR(h);
    grn_obj *target = grn_ctx_get(ctx, data->target);
    int section = data->section;
    switch (target->header.type) {
    case GRN_COLUMN_INDEX :
      return grn_ii_column_update(ctx, (grn_ii *)target, id, section, oldvalue, newvalue);
    default :
      return GRN_SUCCESS;
    }
  }
}

/**** grn_vector ****/

#define VECTOR(obj) ((grn_vector *)obj)

#define INITIAL_VECTOR_SIZE 256

grn_obj *
grn_vector_open(grn_ctx *ctx, grn_obj_flags flags)
{
  grn_obj *obj = (grn_obj *)GRN_MALLOCN(grn_vector, 1);
  if (obj) {
    GRN_OBJ_INIT(obj, GRN_VECTOR, flags);
    obj->header.impl_flags |= GRN_OBJ_ALLOCATED;
    VECTOR(obj)->offsets = NULL;
    VECTOR(obj)->n_entries = 0;
  }
  return obj;
}

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

int
grn_vector_size(grn_ctx *ctx, grn_obj *vector)
{
  return VECTOR(vector)->n_entries;
}

const char *
grn_vector_fetch(grn_ctx *ctx, grn_obj *vector, int i, unsigned int *size)
{
  grn_vector *v = VECTOR(vector);
  if (i < 0) {
    i += v->n_entries;
    if (i < 0) { return NULL; }
  }
  if (v->n_entries <= i) { return NULL; }
  {
    size_t off = i ? v->offsets[i - 1] : 0;
    *size = v->offsets[i] - off;
    return vector->u.b.head + off;
  }
}

grn_rc
grn_vector_decode(grn_ctx *ctx, grn_obj *vector, const char *data, uint32_t data_size)
{
  grn_vector *v = VECTOR(vector);
  uint8_t *p = (uint8_t *)data;
  uint8_t *pe = p + data_size;
  GRN_B_DEC(v->n_entries, p);
  {
    int n = (v->n_entries + (INITIAL_VECTOR_SIZE - 1)) & ~(INITIAL_VECTOR_SIZE - 1);
    uint32_t *offsets = GRN_REALLOC(v->offsets, sizeof(uint32_t) * n);
    if (!offsets) { return GRN_NO_MEMORY_AVAILABLE; }
    v->offsets = offsets;
  }
  {
    int i;
    uint32_t o = 0, l;
    for (i = 0; i < v->n_entries; i++) {
      if (pe <= p) { return GRN_INVALID_ARGUMENT; }
      GRN_B_DEC(l, p);
      o += l;
      v->offsets[i] = o;
    }
  }
  return grn_bulk_write(ctx, vector, (char *)p, pe - p);
}

/**** grn_verses ****/

#define INITIAL_VERSES_SIZE 256

grn_rc
grn_verses_add(grn_ctx *ctx, grn_obj *v, const char *str, unsigned int str_len,
               unsigned int weight, grn_id domain)
{
  grn_verse *vp;
  if (!str_len) { return GRN_SUCCESS; }
  if (!v || !str) {
    GRN_LOG(ctx, GRN_LOG_WARNING, "grn_verses_add: invalid argument");
    return GRN_INVALID_ARGUMENT;
  }
  if (!(v->u.v.n_verses & (INITIAL_VERSES_SIZE - 1))) {
    vp = GRN_REALLOC(v->u.v.verses, sizeof(grn_verse) *
                     (v->u.v.n_verses + INITIAL_VERSES_SIZE));
    if (!vp) { return GRN_NO_MEMORY_AVAILABLE; }
    v->u.v.verses = vp;
  }
  vp = &v->u.v.verses[v->u.v.n_verses];
  if (v->header.flags & GRN_OBJ_DO_SHALLOW_COPY) {
    vp->str = (char *)str;
  } else {
    if (!(vp->str = GRN_MALLOC(str_len))) { return GRN_NO_MEMORY_AVAILABLE; }
    memcpy(vp->str, str, str_len);
  }
  vp->str_len = str_len;
  vp->weight = weight;
  vp->domain = domain;
  v->u.v.n_verses++;
  return GRN_SUCCESS;
}

grn_obj *
grn_verses_to_vector(grn_ctx *ctx, grn_obj *verses)
{
  grn_obj *vector = grn_vector_open(ctx, 0);
  if (vector) {
    grn_verse *vp;
    int i;
    for (i = verses->u.v.n_verses, vp = verses->u.v.verses; i; i--, vp++) {
      grn_bulk_benc(ctx, vector, vp->weight);
      grn_bulk_benc(ctx, vector, vp->domain);
      grn_bulk_write(ctx, vector, vp->str, vp->str_len);
      grn_vector_delimit(ctx, vector);
    }
  }
  return vector;
}

grn_obj *
grn_vector_to_verses(grn_ctx *ctx, grn_obj *vector, grn_obj *verses)
{
  if (!verses) {
    verses = grn_obj_open(ctx, GRN_VERSES, GRN_OBJ_DO_SHALLOW_COPY);
  }
  if (verses) {
    int i, n = grn_vector_size(ctx, vector);
    verses->u.v.src = vector;
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
              grn_verses_add(ctx, verses, (char *)p, pe - p, weight, domain);
            }
          }
        }
      }
    }
  }
  return verses;
}

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

grn_accessor *
accessor_new(grn_ctx *ctx)
{
  grn_accessor *res = GRN_MALLOCN(grn_accessor, 1);
  if (res) {
    res->header.type = GRN_ACCESSOR;
    res->header.impl_flags = GRN_OBJ_ALLOCATED;
    res->header.flags = 0;
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
      obj = grn_ctx_get(ctx, res->obj->header.domain);
      break;
    case GRN_ACCESSOR_GET_VALUE :
    case GRN_ACCESSOR_GET_SCORE :
    case GRN_ACCESSOR_GET_NSUBRECS :
      obj = grn_ctx_get(ctx, DB_OBJ(res->obj)->range);
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      obj = grn_ctx_get(ctx, DB_OBJ(res->obj)->range);
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
    for (sp = name; (len = grn_charlen(ctx, sp, se, ctx->encoding)); sp += len) {
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
          obj = grn_ctx_get(ctx, obj->header.domain);
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
          if (DB_OBJ(obj)->flags & GRN_OBJ_WITH_SUBREC) {
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
            obj = grn_ctx_get(ctx, obj->header.domain);
          }
        }
        break;
      case 'n' : /* nsubrecs */
      case 'N' :
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = accessor_new(ctx);
          (*rp)->obj = obj;
          if (DB_OBJ(obj)->flags & GRN_OBJ_WITH_SUBREC) {
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
            obj = grn_ctx_get(ctx, obj->header.domain);
          }
        }
        break;
      }
    } else {
      /* if obj->header.type == GRN_TYPE ... lookup table */
      for (rp = &res; ; rp = &(*rp)->next) {
        grn_obj *column = grn_table_column(ctx, obj, name, len);
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
          obj = grn_ctx_get(ctx, obj->header.domain);
          switch (obj->header.type) {
          case GRN_TABLE_PAT_KEY :
          case GRN_TABLE_HASH_KEY :
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            break;
          case GRN_TABLE_NO_KEY :
            /* todo */
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
        range = GRN_DB_INT;
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
  grn_obj buf;
  void *vp = NULL;
  size_t vs = 0;
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (;;) {
    GRN_BULK_REWIND(&buf);
    switch (a->action) {
    case GRN_ACCESSOR_GET_KEY :
      grn_table_get_key2(ctx, a->obj, id, &buf);
      vp = GRN_BULK_VALUE(&buf);
      vs = GRN_BULK_VSIZE(&buf);
      break;
    case GRN_ACCESSOR_GET_VALUE :
      grn_obj_get_value(ctx, a->obj, id, &buf);
      vp = GRN_BULK_VALUE(&buf);
      vs = GRN_BULK_VSIZE(&buf);
      break;
    case GRN_ACCESSOR_GET_SCORE :
      grn_obj_get_value(ctx, a->obj, id, &buf);
      {
        grn_rset_recinfo *ri = (grn_rset_recinfo *)buf.u.b.head;
        vp = &ri->score;
        vs = sizeof(int);
      }
      break;
    case GRN_ACCESSOR_GET_NSUBRECS :
      grn_obj_get_value(ctx, a->obj, id, &buf);
      {
        grn_rset_recinfo *ri = (grn_rset_recinfo *)buf.u.b.head;
        vp = &ri->n_subrecs;
        vs = sizeof(int);
      }
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE :
      /* todo : support vector */
      grn_obj_get_value(ctx, a->obj, id, &buf);
      vp = GRN_BULK_VALUE(&buf);
      vs = GRN_BULK_VSIZE(&buf);
      break;
    case GRN_ACCESSOR_GET_DB_OBJ :
      value = grn_ctx_get(ctx, id);
      grn_obj_close(ctx, &buf);
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
  if (!value) { value = grn_obj_open(ctx, GRN_BULK, 0); }
  if (value) { grn_bulk_write(ctx, value, vp, vs); }
  grn_obj_close(ctx, &buf);
  return value;
}

static grn_rc
grn_accessor_set_value(grn_ctx *ctx, grn_accessor *a, grn_id id,
                       grn_obj *value, int flags)
{
  grn_rc rc = GRN_SUCCESS;
  if (!value) { value = grn_obj_open(ctx, GRN_BULK, 0); }
  if (value) {
    grn_obj buf;
    void *vp = NULL;
    size_t vs;
    GRN_OBJ_INIT(&buf, GRN_BULK, 0);
    for (;;) {
      GRN_BULK_REWIND(&buf);
      switch (a->action) {
      case GRN_ACCESSOR_GET_KEY :
        grn_table_get_key2(ctx, a->obj, id, &buf);
        vp = GRN_BULK_VALUE(&buf);
        vs = GRN_BULK_VSIZE(&buf);
        break;
      case GRN_ACCESSOR_GET_VALUE :
        if (a->next) {
          grn_obj_get_value(ctx, a->obj, id, &buf);
          vp = GRN_BULK_VALUE(&buf);
          vs = GRN_BULK_VSIZE(&buf);
        } else {
          rc = grn_obj_set_value(ctx, a->obj, id, value, flags);
        }
        break;
      case GRN_ACCESSOR_GET_SCORE :
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)buf.u.b.head;
          vp = &ri->score;
          vs = sizeof(int);
        }
        break;
      case GRN_ACCESSOR_GET_NSUBRECS :
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)buf.u.b.head;
          vp = &ri->n_subrecs;
          vs = sizeof(int);
        }
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE :
        /* todo : support vector */
        if (a->next) {
          grn_obj_get_value(ctx, a->obj, id, &buf);
          vp = GRN_BULK_VALUE(&buf);
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
    void *v;
    unsigned int s;
    grn_hook *hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET];
    grn_obj oldbuf, *oldvalue;
    GRN_OBJ_INIT(&oldbuf, GRN_BULK, 0);
    oldvalue = grn_obj_get_value(ctx, obj, id, &oldbuf);
    if (flags & GRN_OBJ_SET) {
      void *ov;
      unsigned int os;
      v = GRN_BULK_VALUE(value);
      s = GRN_BULK_LEN(value);
      ov = GRN_BULK_VALUE(oldvalue);
      os = GRN_BULK_LEN(oldvalue);
      if (ov && v && os == s && !memcmp(ov, v, s)) {
        grn_bulk_fin(ctx, oldvalue);
        rc = GRN_SUCCESS;
        goto exit;
      }
    }
    if (hooks) {
      // todo : grn_proc_ctx_open()
      grn_proc_ctx pctx = {{0}, obj, hooks, hooks, PROC_INIT, 4, 4};
      pctx.data[0].id = id;
      pctx.data[1].ptr = oldvalue;
      pctx.data[2].ptr = value;
      pctx.data[3].int_value = flags;
      while (hooks) {
        pctx.currh = hooks;
        if (hooks->proc) {
          rc = hooks->proc->funcs[PROC_INIT](ctx, obj, &pctx.user_data, 4, pctx.data);
        } else {
          rc = default_set_value_hook(ctx, obj, &pctx.user_data, 4, pctx.data);
        }
        if (rc) { goto exit; }
        hooks = hooks->next;
        pctx.offset++;
      }
    }
    grn_obj_close(ctx, oldvalue);
    v = GRN_BULK_VALUE(value);
    s = GRN_BULK_LEN(value);
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
      if (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
        grn_obj *vector;
        /* (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VERSES */
        /* if DB_OBJ(obj)->range is a table, then verses2updspecs() */
        if (value->header.type != GRN_VERSES) {
          /* todo : convert */
          ERR(GRN_INVALID_ARGUMENT, "verses required");
          rc = GRN_INVALID_ARGUMENT;
          goto exit;
        }
        vector = grn_verses_to_vector(ctx, value);
        rc = grn_ja_putv(ctx, (grn_ja *)obj, id, vector, 0);
        grn_obj_close(ctx, vector);
      } else {
        rc = grn_ja_put(ctx, (grn_ja *)obj, id, v, s, 0);
      }
      break;
      // todo type->flag
    case GRN_COLUMN_FIX_SIZE :
      if (((grn_ra *)obj)->header->element_size < s) {
        ERR(GRN_INVALID_ARGUMENT, "too long value (%d)", s);
      } else {
        void *p = grn_ra_get(ctx, (grn_ra *)obj, id);
        if (!p) {
          ERR(GRN_NO_MEMORY_AVAILABLE, "ra get failed");
          rc = GRN_NO_MEMORY_AVAILABLE;
          goto exit;
        }
        memcpy(p, v, s);
        rc = GRN_SUCCESS;
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
    if ((value = grn_ja_ref(ctx, (grn_ja *)obj, id, size))) {
      grn_ja_unref(ctx, (grn_ja *)obj, id, (void *)value, *size);
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
    if ((value = grn_ra_at(ctx, (grn_ra *)obj, id))) {
      *size = ((grn_ra *)obj)->header->element_size;
    }
    break;
  case GRN_COLUMN_INDEX :
    // todo : how??
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
    if (!(value = grn_obj_open(ctx, GRN_BULK, 0))) {
      ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_value failed");
      goto exit;
    }
  }
  switch (value->header.type) {
  case GRN_VOID :
    value->header.type = GRN_BULK;
    grn_bulk_init(ctx, value, 0);
    break;
  case GRN_BULK :
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
      len = grn_pat_get_value(ctx, pat, id, value->u.b.curr - size);
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
      len = grn_hash_get_value(ctx, hash, id, value->u.b.curr - size);
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
      len = grn_array_get_value(ctx, array, id, value->u.b.curr - size);
    }
    break;
  case GRN_COLUMN_VAR_SIZE :
    if (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
      /* (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VERSES */
      grn_obj *vector;
      void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &len);
      if (!v) { len = 0; goto exit; }
      vector = grn_vector_open(ctx, 0);
      if (!grn_vector_decode(ctx, vector, v, len)) {
        value->header.type = GRN_VERSES;
        value->header.flags |= GRN_OBJ_DO_SHALLOW_COPY;
        value = grn_vector_to_verses(ctx, vector, value);
      } else {
        grn_obj_close(ctx, vector);
      }
      grn_ja_unref(ctx, (grn_ja *)obj, id, v, len);
    } else {
      void *v = grn_ja_ref(ctx, (grn_ja *)obj, id, &len);
      if (!v) { len = 0; goto exit; }
      // todo : reduce copy
      grn_bulk_write(ctx, value, v, len);
      grn_ja_unref(ctx, (grn_ja *)obj, id, v, len);
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
    {
      unsigned element_size;
      void *v = grn_ra_at(ctx, (grn_ra *)obj, id);
      if (!v) { goto exit; }
      element_size = ((grn_ra *)obj)->header->element_size;
      grn_bulk_write(ctx, value, v, element_size);
      len = element_size;
    }
    break;
  case GRN_COLUMN_INDEX :
    // todo : how??
    break;
  }
  if (value->header.type == GRN_BULK) {
    value->header.domain = grn_obj_get_range(ctx, obj);
  }
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
    obj = grn_ctx_get(ctx, col->header.domain);
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
  if (!valuebuf) {
    if (!(valuebuf = grn_obj_open(ctx, GRN_BULK, 0))) {
      ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_info failed");
      goto exit;
    }
  }
  switch (type) {
  case GRN_INFO_SOURCE :
    if (!GRN_DB_OBJP(obj)) {
      ERR(GRN_INVALID_ARGUMENT, "only db_obj can accept GRN_INFO_SOURCE");
      goto exit;
    }
    GRN_OBJ_INIT(valuebuf, GRN_BULK, 0);
    GRN_BULK_SET(ctx, valuebuf, DB_OBJ(obj)->source, DB_OBJ(obj)->source_size);
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
  GRN_OBJ_INIT(&data, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_BULK_SET(ctx, &data, &hook_data, sizeof hook_data);
  for (i = 1; i <= n; i++, s++) {
    hook_data.section = i;
    if ((source = grn_ctx_get(ctx, *s))) {
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
      if ((rc = grn_bulk_benc(ctx, buf, id + 1))) { goto exit; }
      if ((rc = grn_bulk_benc(ctx, buf, hooks->hld_size))) { goto exit; }
      if ((rc = grn_bulk_write(ctx, buf, (char *)NEXT_ADDR(hooks), hooks->hld_size))) { goto exit; }
    }
    if ((rc = grn_bulk_benc(ctx, buf, 0))) { goto exit; }
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
      if (p >= pe) { return grn_invalid_format; }
      GRN_B_DEC(hld_size, p);
      if (p >= pe) { return grn_invalid_format; }
      if (!(new = GRN_MALLOC(sizeof(grn_hook) + hld_size))) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      if (id) {
        new->proc = (grn_proc *)grn_ctx_get(ctx, id);
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
      if (p >= pe) { return grn_invalid_format; }
    }
    *last = NULL;
  }
  return GRN_SUCCESS;
}

static void
grn_obj_spec_save(grn_ctx *ctx, grn_db_obj *obj)
{
  grn_obj *v;
  grn_db *s;
  grn_obj_spec spec;
  if (obj->id & GRN_OBJ_TMP_OBJECT) { return; }
  if (!ctx->impl || !GRN_DB_OBJP(obj)) { return; }
  if (!(s = (grn_db *)ctx->impl->db) || !s->specs) { return; }
  v = grn_vector_open(ctx, 0);
  spec.header = obj->header;
  spec.range = obj->range;
  grn_bulk_write(ctx, v, (void *)&spec, sizeof(grn_obj_spec));
  grn_vector_delimit(ctx, v);
  if (obj->header.impl_flags & GRN_OBJ_CUSTOM_NAME) {
    GRN_BULK_PUTS(ctx, v, grn_obj_path(ctx, (grn_obj *)obj));
  }
  grn_vector_delimit(ctx, v);
  grn_bulk_write(ctx, v, obj->source, obj->source_size);
  grn_vector_delimit(ctx, v);
  grn_hook_pack(ctx, obj, v);
  grn_vector_delimit(ctx, v);
  grn_ja_putv(ctx, s->specs, obj->id, v, 0);
  grn_obj_close(ctx, v);
}

grn_rc
grn_obj_set_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *value)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_value failed");
    goto exit;
  }
  switch (type) {
  case GRN_INFO_SOURCE :
    if (!GRN_DB_OBJP(obj)) {
      ERR(GRN_INVALID_ARGUMENT, "only db_obj can accept GRN_INFO_SOURCE");
      goto exit;
    }
    {
      void *v = GRN_BULK_VALUE(value);
      uint32_t s = GRN_BULK_LEN(value);
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
    if (value && DB_OBJ(value)->header.type == GRN_PROC) {
      switch (DB_OBJ(obj)->header.type) {
      case GRN_TABLE_HASH_KEY :
        ((grn_hash *)obj)->tokenizer = value;
        ((grn_hash *)obj)->header->tokenizer = DB_OBJ(value)->id;
        rc = GRN_SUCCESS;
        break;
      case GRN_TABLE_PAT_KEY :
        ((grn_pat *)obj)->tokenizer = value;
        ((grn_pat *)obj)->header->tokenizer = DB_OBJ(value)->id;
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
      hld_value = GRN_BULK_VALUE(hld);
      hld_size = GRN_BULK_LEN(hld);
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
grn_obj_remove(grn_ctx *ctx, const char *path)
{
  GRN_API_ENTER;
  GRN_API_RETURN(GRN_SUCCESS);
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
    grn_search_flags f = GRN_TABLE_ADD;
    if (!(id = grn_pat_lookup(ctx, s->keys, name, name_size, NULL, &f))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_pat_lookup failed");
    } else if (!(f & GRN_TABLE_ADDED)) {
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
  if (spec->header.impl_flags & GRN_OBJ_CUSTOM_NAME) {\
    const char *path = grn_vector_fetch(ctx, v, 1, &size);\
    if (size > PATH_MAX) { ERR(grn_invalid_format, "too long path"); }\
    memcpy(buffer, path, size);\
    buffer[size] = '\0';\
  } else {\
    gen_pathname(s->keys->io->path, buffer, id);\
  }\
}

grn_obj *
grn_ctx_get(grn_ctx *ctx, grn_id id)
{
  grn_obj *res = NULL;
  if (!ctx || !ctx->impl) { return res; }
  GRN_API_ENTER;
  if (id & GRN_OBJ_TMP_OBJECT) {
    if (ctx->impl->values) {
      grn_array_get_value(ctx, ctx->impl->values, id & ~GRN_OBJ_TMP_OBJECT, &res);
    }
  } else {
    grn_obj *db = ctx->impl->db;
    if (db) {
      grn_db *s = (grn_db *)db;
      grn_obj **vp;
      if (id == GRN_ID_NIL) { res = ctx->impl->db; goto exit; }
      if (!(vp = grn_tiny_array_at(&s->values, id))) { goto exit; }
      if (s->specs && !*vp) {
        uint32_t value_len;
        char *value = grn_ja_ref(ctx, s->specs, id, &value_len);
        if (value) {
          grn_obj *v = grn_vector_open(ctx, 0);
          if (v) {
            uint32_t size;
            grn_obj_spec *spec;
            char buffer[PATH_MAX];
            grn_vector_decode(ctx, v, value, value_len);
            spec = (grn_obj_spec *)grn_vector_fetch(ctx, v, 0, &size);
            if (spec) {
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
                  grn_obj *table = grn_ctx_get(ctx, spec->header.domain);
                  MUTEX_LOCK(s->lock);
                  if (!*vp) { *vp = (grn_obj *)grn_ii_open(ctx, buffer, table); }
                  MUTEX_UNLOCK(s->lock);
                }
                break;
              case GRN_PROC :
                if (!*vp) { *vp = grn_proc_open(ctx, spec); }
                break;
              }
              if (*vp) {
                const char *p;
                grn_db_obj *r = DB_OBJ(*vp);
                r->header = spec->header;
                r->id = id;
                r->range = spec->range;
                r->db = db;
                p  = grn_vector_fetch(ctx, v, 2, &size);
                if (size) {
                  if ((r->source = GRN_MALLOC(size))) {
                    memcpy(r->source, p, size);
                    r->source_size = size;
                  }
                }
                p  = grn_vector_fetch(ctx, v, 3, &size);
                grn_hook_unpack(ctx, r, p, size);
              }
            }
            grn_obj_close(ctx, v);
          }
          grn_ja_unref(ctx, s->specs, id, value, value_len);
        }
      }
      res = *vp;
    }
  }
exit :
  GRN_API_RETURN(res);
}

grn_obj *
grn_obj_open(grn_ctx *ctx, unsigned char type, grn_obj_flags flags)
{
  switch (type) {
  case GRN_VECTOR :
    return grn_vector_open(ctx, flags);
  default :
    {
      grn_obj *obj = GRN_MALLOCN(grn_obj, 1);
      if (obj) {
        GRN_OBJ_INIT(obj, type, flags);
        obj->header.impl_flags |= GRN_OBJ_ALLOCATED;
      }
      return obj;
    }
  }
}

grn_obj *
grn_obj_graft(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj *new = grn_obj_open(ctx, obj->header.type, obj->header.flags);
  if (new) {
    /* todo : deep copy if (obj->header.flags & GRN_OBJ_DO_SHALLOW_COPY) */
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
    case GRN_BULK :
      obj->header.type = GRN_VOID;
      if (obj->header.flags & GRN_OBJ_DO_SHALLOW_COPY) {
        obj->u.b.head = NULL;
        obj->u.b.tail = NULL;
        rc = GRN_SUCCESS;
      } else {
        rc = grn_bulk_fin(ctx, obj);
      }
      if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) {
        GRN_FREE(obj);
      }
      break;
    case GRN_ACCESSOR :
      {
        grn_accessor *p, *n;
        for (p = (grn_accessor *)obj; p; p = n) {
          n = p->next;
          GRN_FREE(p);
        }
      }
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
    case GRN_VERSES :
      if (obj->u.v.verses) {
        if (!(obj->header.flags & GRN_OBJ_DO_SHALLOW_COPY)) {
          int n = obj->u.v.n_verses;
          grn_verse *vp = obj->u.v.verses;
          for (n = obj->u.v.n_verses, vp = obj->u.v.verses; n; n--, vp++) {
            GRN_FREE(vp->str);
          }
        }
        GRN_FREE(obj->u.v.verses);
      }
      if (obj->u.v.src) { grn_obj_close(ctx, obj->u.v.src); }
      if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) {
        GRN_FREE(obj);
      }
      rc = GRN_SUCCESS;
      break;
    case GRN_VECTOR :
      grn_bulk_fin(ctx, obj);
      if (VECTOR(obj)->offsets) {
        GRN_FREE(VECTOR(obj)->offsets);
      }
      if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) {
        GRN_FREE(obj);
      }
      rc = GRN_SUCCESS;
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
  path = io ? io->path : NULL;
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
  GRN_API_ENTER;
  GRN_API_RETURN(obj);
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
      if (keys->flags & GRN_TABLE_SORT_ASC) {
        ap = grn_obj_get_value_(ctx, keys->key, a->id, &as);
        bp = grn_obj_get_value_(ctx, keys->key, b->id, &bs);
      } else {
        ap = grn_obj_get_value_(ctx, keys->key, b->id, &as);
        bp = grn_obj_get_value_(ctx, keys->key, a->id, &bs);
      }
    } else {
      if (keys->flags & GRN_TABLE_SORT_ASC) {
        ap = a->value; as = a->size;
        bp = b->value; bs = b->size;
      } else {
        ap = b->value; as = b->size;
        bp = a->value; bs = a->size;
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
    ERR(GRN_INVALID_ARGUMENT, "keys is null");
    goto exit;
  }
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT, "table is null");
    goto exit;
  }
  if (!(result && result->header.type == GRN_TABLE_NO_KEY)) {
    ERR(GRN_INVALID_ARGUMENT, "result is not a array");
    goto exit;
  }
  n = grn_table_size(ctx, table);
  if (limit <= 0) {
    limit += n;
    if (limit <= 0) {
      GRN_LOG(ctx, GRN_LOG_ALERT, "limit is too small in grn_table_sort !");
      goto exit;
    }
  }
  {
    int j;
    grn_table_sort_key *kp;
    for (kp = keys, j = n_keys; j; kp++, j--) {
      grn_obj *range = grn_ctx_get(ctx, grn_obj_get_range(ctx, kp->key));
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
  grn_obj *o = grn_ctx_lookup(ctx, name, strlen(name));
  if (!o) { o = grn_type_create(ctx, name, strlen(name), flags, size); }
  return o;
}

grn_rc
grn_db_init_builtin_types(grn_ctx *ctx)
{
  grn_obj *obj;
  obj = deftype(ctx, "<int>",
                GRN_OBJ_KEY_INT, sizeof(int32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT) { return grn_invalid_format; }
  obj = deftype(ctx, "<uint>",
                GRN_OBJ_KEY_UINT, sizeof(uint32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT) { return grn_invalid_format; }
  obj = deftype(ctx, "<int64>",
                GRN_OBJ_KEY_INT, sizeof(int64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT64) { return grn_invalid_format; }
  obj = deftype(ctx, "<float>",
                GRN_OBJ_KEY_FLOAT, sizeof(double));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_FLOAT) { return grn_invalid_format; }
  obj = deftype(ctx, "<time>",
                GRN_OBJ_KEY_UINT, sizeof(grn_timeval));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TIME) { return grn_invalid_format; }
  obj = deftype(ctx, "<shorttext>",
                GRN_OBJ_KEY_VAR_SIZE, GRN_TABLE_MAX_KEY_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_SHORTTEXT) { return grn_invalid_format; }
  obj = deftype(ctx, "<text>",
                GRN_OBJ_KEY_VAR_SIZE, 1 << 16);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TEXT) { return grn_invalid_format; }
  obj = deftype(ctx, "<longtext>",
                GRN_OBJ_KEY_VAR_SIZE, 1 << 31);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_LONGTEXT) { return grn_invalid_format; }
  return grn_db_init_builtin_tokenizers(ctx);
}
