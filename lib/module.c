/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

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

#include <string.h>
#include "db.h"
#include "ql.h"

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
  range = path ? grn_dl_get(ctx, path) : GRN_ID_NIL;
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
  if (GRN_DB_P(db)) {
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
        ctx->impl->module_path = path;
        ctx->rc = func(ctx, path);
        ctx->impl->module_path = NULL;
      } else {
        ERR(GRN_INVALID_FORMAT, "init_func not found(%s)", buffer);
      }
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
  }
  GRN_API_RETURN(ctx->rc);
}
