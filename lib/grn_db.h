/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018-2020 Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx.h"
#include "grn_options.h"
#include "grn_rset.h"
#include "grn_store.h"

#include <groonga/command.h>
#include <groonga/token_filter.h>
#include <groonga/scorer.h>

#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_DB_DELIMITER            '.'
#define GRN_DB_PSEUDO_COLUMN_PREFIX '_'

#define GRN_N_RESERVED_TYPES 256

/* #define GRN_REFERENCE_COUNT_DEBUG */
#ifdef GRN_REFERENCE_COUNT_DEBUG
# define grn_log_reference_count(...) printf(__VA_ARGS__)
#else
# define grn_log_reference_count(...)
#endif

#define GRN_TABLE_GROUPED (0x01<<0)
#define GRN_TABLE_IS_GROUPED(table)\
  ((table)->header.impl_flags & GRN_TABLE_GROUPED)
#define GRN_TABLE_GROUPED_ON(table)\
  ((table)->header.impl_flags |= GRN_TABLE_GROUPED)
#define GRN_TABLE_IS_MULTI_KEYS_GROUPED(table)\
  (GRN_TABLE_IS_GROUPED(table) &&\
   table->header.domain == GRN_ID_NIL)

extern bool grn_enable_reference_count;

typedef struct _grn_db grn_db;
typedef struct _grn_proc grn_proc;

struct _grn_db {
  grn_db_obj obj;
  grn_obj *keys;
  grn_ja *specs;
  grn_hash *config;
  grn_tiny_array values;
  grn_critical_section lock;
  grn_cache *cache;
  grn_options *options;
};

#define GRN_SERIALIZED_SPEC_INDEX_SPEC   0
#define GRN_SERIALIZED_SPEC_INDEX_PATH   1
#define GRN_SERIALIZED_SPEC_INDEX_SOURCE 2
#define GRN_SERIALIZED_SPEC_INDEX_HOOK   3
#define GRN_SERIALIZED_SPEC_INDEX_TOKEN_FILTERS 4
#define GRN_SERIALIZED_SPEC_INDEX_EXPR   4

typedef struct {
  grn_obj_header header;
  grn_id range;
} grn_obj_spec;

grn_bool grn_db_spec_unpack(grn_ctx *ctx,
                            grn_id id,
                            void *encoded_spec,
                            uint32_t encoded_spec_size,
                            grn_obj_spec **spec,
                            grn_obj *decoded_spec,
                            const char *error_message_tag);

#define GRN_DB_SPEC_EACH_BEGIN(ctx, cursor, id, spec) do {              \
  grn_obj *db = grn_ctx_db((ctx));                                      \
  grn_db *db_raw = (grn_db *)db;                                        \
  grn_obj decoded_spec;                                                 \
  grn_io_win iw;                                                        \
  grn_bool iw_need_unref = GRN_FALSE;                                   \
  GRN_OBJ_INIT(&decoded_spec, GRN_VECTOR, 0, GRN_DB_TEXT);              \
  GRN_TABLE_EACH_BEGIN((ctx), db, cursor, id) {                         \
    void *encoded_spec;                                                 \
    uint32_t encoded_spec_size;                                         \
    grn_bool success;                                                   \
    grn_obj_spec *spec;                                                 \
                                                                        \
    if (iw_need_unref) {                                                \
      grn_ja_unref(ctx, &iw);                                           \
      iw_need_unref = GRN_FALSE;                                        \
    }                                                                   \
    encoded_spec = grn_ja_ref((ctx),                                    \
                              db_raw->specs,                            \
                              id,                                       \
                              &iw,                                      \
                              &encoded_spec_size);                      \
    if (!encoded_spec) {                                                \
      continue;                                                         \
    }                                                                   \
    iw_need_unref = GRN_TRUE;                                           \
                                                                        \
    GRN_BULK_REWIND(&decoded_spec);                                     \
    success = grn_db_spec_unpack(ctx,                                   \
                                 id,                                    \
                                 encoded_spec,                          \
                                 encoded_spec_size,                     \
                                 &spec,                                 \
                                 &decoded_spec,                         \
                                 __FUNCTION__);                         \
   if (!success) {                                                      \
     continue;                                                          \
   }                                                                    \

#define GRN_DB_SPEC_EACH_END(ctx, cursor)         \
  } GRN_TABLE_EACH_END(ctx, cursor);              \
  if (iw_need_unref) {                            \
    grn_ja_unref(ctx, &iw);                       \
  }                                               \
  GRN_OBJ_FIN((ctx), &decoded_spec);              \
} while(GRN_FALSE)

void grn_db_init_from_env(void);

GRN_API grn_rc grn_db_close(grn_ctx *ctx, grn_obj *db);

grn_obj *grn_db_keys(grn_obj *s);

void grn_db_generate_pathname(grn_ctx *ctx,
                              grn_obj *db,
                              grn_id id,
                              char *buffer);
grn_rc grn_db_clear_dirty(grn_ctx *ctx, grn_obj *db);

grn_rc grn_db_set_option_values(grn_ctx *ctx,
                                grn_obj *db,
                                grn_id id,
                                const char *name,
                                int name_length,
                                grn_obj *values);
grn_option_revision
grn_db_get_option_values(grn_ctx *ctx,
                         grn_obj *db,
                         grn_id id,
                         const char *name,
                         int name_length,
                         grn_option_revision revision,
                         grn_obj *values);
grn_rc grn_db_clear_option_values(grn_ctx *ctx,
                                  grn_obj *db,
                                  grn_id id);

grn_rc _grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id,
                               grn_table_delete_optarg *optarg);

grn_id grn_table_get_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                       void **value);
grn_id grn_table_add_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                       void **value, int *added);
grn_id grn_table_add_by_key(grn_ctx *ctx,
                            grn_obj *table,
                            grn_obj *key,
                            int *added);
GRN_API grn_rc grn_table_get_info(grn_ctx *ctx, grn_obj *table, grn_table_flags *flags,
                                  grn_encoding *encoding, grn_obj **tokenizer,
                                  grn_obj **normalizer,
                                  grn_obj **token_filters);
const char *_grn_table_key(grn_ctx *ctx, grn_obj *table, grn_id id, uint32_t *key_size);

grn_rc grn_table_search(grn_ctx *ctx, grn_obj *table,
                        const void *key, uint32_t key_size,
                        grn_operator mode, grn_obj *res, grn_operator op);

grn_rc grn_table_fuzzy_search(grn_ctx *ctx, grn_obj *table,
                              const void *key, uint32_t key_size,
                              grn_fuzzy_search_optarg *args, grn_obj *res, grn_operator op);

grn_id grn_table_next(grn_ctx *ctx, grn_obj *table, grn_id id);

int grn_table_get_key2(grn_ctx *ctx, grn_obj *table, grn_id id, grn_obj *bulk);

grn_table_cursor *grn_table_cursor_open_by_id(grn_ctx *ctx, grn_obj *table,
                                              grn_id min, grn_id max, int flags);

void grn_table_add_subrec(grn_obj *table, grn_rset_recinfo *ri, double score,
                          grn_rset_posinfo *pi, int dir);

grn_obj *grn_obj_graft(grn_ctx *ctx, grn_obj *obj);

grn_rc grn_column_name_(grn_ctx *ctx, grn_obj *obj, grn_obj *buf);


typedef enum {
  PROC_INIT = 0,
  PROC_NEXT,
  PROC_FIN
} grn_proc_phase;

struct _grn_type {
  grn_db_obj obj;
};

#define GRN_TYPE_SIZE(type) ((type)->range)

#define GRN_TABLE_SORT_GEO            (0x02<<0)

#define GRN_OBJ_TMP_OBJECT 0x80000000
#define GRN_OBJ_TMP_COLUMN 0x40000000

#define GRN_DB_OBJP(obj) \
  (obj &&\
   ((GRN_SNIP == ((grn_db_obj *)obj)->header.type) ||\
    ((GRN_CURSOR_TABLE_HASH_KEY <= ((grn_db_obj *)obj)->header.type) &&\
     (((grn_db_obj *)obj)->header.type <= GRN_COLUMN_INDEX))))

#define GRN_OBJ_TABLEP(obj) \
  (obj &&\
   (GRN_TABLE_HASH_KEY <= ((grn_db_obj *)obj)->header.type) &&\
   (((grn_db_obj *)obj)->header.type <= GRN_DB))

#define GRN_OBJ_INDEX_COLUMNP(obj) \
  (obj &&\
   DB_OBJ(obj)->header.type == GRN_COLUMN_INDEX)

#define GRN_OBJ_VECTOR_COLUMNP(obj) \
  (obj &&\
   DB_OBJ(obj)->header.type == GRN_COLUMN_VAR_SIZE &&\
   (DB_OBJ(obj)->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR)

#define GRN_OBJ_WEIGHT_VECTOR_COLUMNP(obj) \
  (GRN_OBJ_VECTOR_COLUMNP(obj) &&\
   (DB_OBJ(obj)->header.flags & GRN_OBJ_WITH_WEIGHT))

struct _grn_hook {
  grn_hook *next;
  grn_proc *proc;
  uint32_t hld_size;
};

typedef struct _grn_proc_ctx grn_proc_ctx;

struct _grn_proc_ctx {
  grn_user_data user_data;
  grn_proc *proc;
  grn_obj *caller;
  //  grn_obj *obj;
  grn_hook *hooks;
  grn_hook *currh;
  grn_proc_phase phase;
  unsigned short nargs;
  unsigned short offset;
  grn_user_data data[16];
};

static grn_inline void
grn_proc_ctx_init(grn_proc_ctx *pctx,
                  grn_hook *hooks,
                  unsigned short n_args,
                  unsigned short offset)
{
  memset(pctx, 0, sizeof(*pctx));
  pctx->proc = hooks->proc;
  pctx->hooks = hooks;
  pctx->currh = hooks;
  pctx->phase = PROC_INIT;
  pctx->nargs = n_args;
  pctx->offset = offset;
}

struct _grn_proc {
  grn_db_obj obj;
  grn_obj name_buf;
  grn_expr_var *vars;
  uint32_t nvars;
  /* -- compatible with grn_expr -- */
  grn_proc_type type;
  grn_proc_func *funcs[3];

  union {
    struct {
      grn_selector_func *selector;
      grn_operator selector_op;
      grn_bool is_stable;
    } function;
    struct {
      grn_command_run_func *run;
    } command;
    struct {
      grn_tokenizer_init_func *init;
      grn_tokenizer_next_func *next;
      grn_tokenizer_fin_func  *fin;
    } tokenizer;
    struct {
      grn_token_filter_init_func   *init;
      grn_token_filter_init_query_func *init_query;
      grn_token_filter_filter_func *filter;
      grn_token_filter_fin_func    *fin;
    } token_filter;
    struct {
      grn_scorer_score_func *score;
    } scorer;
    grn_window_function_func *window_function;
    struct {
      grn_aggregator_init_func *init;
      grn_aggregator_update_func *update;
      grn_aggregator_fin_func *fin;
    } aggregator;
  } callbacks;

  void *user_data;

  grn_id module;
  //  uint32_t nargs;
  //  uint32_t nresults;
  //  grn_obj results[16];
};

#define GRN_PROC_GET_VARS() (grn_proc_get_vars(ctx, user_data))
#define GRN_PROC_GET_VAR(name) (grn_proc_get_var(ctx, user_data, name, strlen(name)))
#define GRN_PROC_GET_VAR_BY_OFFSET(offset) (grn_proc_get_var_by_offset(ctx, user_data, offset))
#define GRN_PROC_GET_OR_ADD_VAR(name) (grn_proc_get_or_add_var(ctx, user_data, name, strlen(name)))
#define GRN_PROC_ALLOC(domain, flags) (grn_proc_alloc(ctx, user_data, domain, flags))

grn_obj *grn_proc_get_vars(grn_ctx *ctx, grn_user_data *user_data);

grn_obj *grn_proc_get_var(grn_ctx *ctx, grn_user_data *user_data,
                          const char *name, unsigned int name_size);

GRN_API grn_obj *grn_proc_get_var_by_offset(grn_ctx *ctx, grn_user_data *user_data,
                                            unsigned int offset);
GRN_API grn_obj *grn_proc_get_or_add_var(grn_ctx *ctx, grn_user_data *user_data,
                                         const char *name, unsigned int name_size);

GRN_API grn_obj *grn_proc_alloc(grn_ctx *ctx, grn_user_data *user_data,
                                grn_id domain, unsigned char flags);

GRN_API grn_rc grn_proc_call(grn_ctx *ctx, grn_obj *proc,
                             int nargs, grn_obj *caller);

grn_obj *grn_expr_get_or_add_var(grn_ctx *ctx, grn_obj *expr,
                                 const char *name, unsigned int name_size);


typedef struct _grn_accessor grn_accessor;

struct _grn_accessor {
  grn_obj_header header;
  grn_id range;
  /* -- compatible with grn_db_obj -- */
  uint8_t action;
  int offset;
  grn_obj *obj;
  grn_accessor *next;
  uint32_t reference_count;
};

typedef enum {
  GRN_ACCESSOR_VOID = 0,
  GRN_ACCESSOR_GET_ID,
  GRN_ACCESSOR_GET_KEY,
  GRN_ACCESSOR_GET_VALUE,
  GRN_ACCESSOR_GET_SCORE,
  GRN_ACCESSOR_GET_NSUBRECS,
  GRN_ACCESSOR_GET_MAX,
  GRN_ACCESSOR_GET_MIN,
  GRN_ACCESSOR_GET_SUM,
  GRN_ACCESSOR_GET_AVG,
  GRN_ACCESSOR_GET_COLUMN_VALUE,
  GRN_ACCESSOR_GET_DB_OBJ,
  GRN_ACCESSOR_LOOKUP,
  GRN_ACCESSOR_FUNCALL
} grn_accessor_action;

#define DB_OBJ(obj) ((grn_db_obj *)obj)

GRN_API const char *grn_obj_get_value_(grn_ctx *ctx, grn_obj *obj, grn_id id, uint32_t *size);
void grn_obj_get_range_info(grn_ctx *ctx,
                            grn_obj *obj,
                            grn_id *range_id,
                            grn_obj_flags *range_flags);


grn_rc grn_db_init_builtin_types(grn_ctx *ctx);

/* flag value used for grn_obj.header.flags */

#define GRN_OBJ_CUSTOM_NAME            (0x01<<12) /* db_obj which has custom name */

#define GRN_OBJ_RESOLVE(ctx,obj) \
  (((obj)->header.type != GRN_PTR)\
   ? (obj)\
   : GRN_PTR_VALUE(obj)\
      ? GRN_PTR_VALUE(obj)\
      : grn_ctx_at((ctx), (obj)->header.domain))

/* expr */

typedef struct _grn_expr grn_expr;

grn_rc grn_expr_parser_close(grn_ctx *ctx);

/**
 * grn_table_open:
 * @name: The table name to be opened. `NULL` means anonymous table.
 * @path: The path of the table to be opened.
 *
 * Opens an existing table. The table is associated with @name in DB
 * that is used by @ctx. grn_ctx_get() is better rather than this
 * function when you want to open a permanent named table that is
 * registered in DB.
 **/
GRN_API grn_obj *grn_table_open(grn_ctx *ctx,
                                const char *name, unsigned int name_size,
                                const char *path);

/**
 * grn_column_open:
 * @table: The table for the opened column.
 * @name: The column name to be opened.
 * @path: The path of the column to be opened.
 * @type: The type of the column value.
 *
 * Opens an existing permanent column. The column is associated with
 * @name in @table. grn_ctx_get() is better rather than this function
 * when you want to open a column of an permanent table in DB.
 **/
grn_obj *grn_column_open(grn_ctx *ctx, grn_obj *table,
                         const char *name, unsigned int name_size,
                         const char *path, grn_obj *type);

/**
 * grn_obj_path_rename:
 * @old_path: The current file path.
 * @new_path: The new file path.
 *
 * It renames object's path that is stored in @old_path to @new_path.
 **/
grn_rc grn_obj_path_rename(grn_ctx *ctx, const char *old_path, const char *new_path);

grn_rc grn_db_check_name(grn_ctx *ctx, const char *name, unsigned int name_size);
#define GRN_DB_CHECK_NAME_ERR(error_context, name, name_size) \
  ERR(GRN_INVALID_ARGUMENT,\
       "%s name can't start with '%c' and contains only 0-9, A-Z, a-z, #, @, - or _: <%.*s>",\
      error_context, GRN_DB_PSEUDO_COLUMN_PREFIX, name_size, name)

#define GRN_DB_P(s) ((s) && ((grn_db *)s)->obj.header.type == GRN_DB)
#define GRN_DB_PERSISTENT_P(s) (((grn_db *)s)->specs)

#define GRN_OBJ_GET_VALUE_IMD (0xffffffffU)

grn_rc grn_db_obj_init(grn_ctx *ctx, grn_obj *db, grn_id id, grn_db_obj *obj);

#define GRN_ACCESSORP(obj) \
  ((obj) && (((grn_obj *)(obj))->header.type == GRN_ACCESSOR))

grn_rc grn_obj_refer(grn_ctx *ctx, grn_obj *obj);
uint32_t grn_obj_reference_count(grn_ctx *ctx, grn_obj *obj);

grn_id grn_obj_register(grn_ctx *ctx, grn_obj *db, const char *name, unsigned int name_size);
int grn_obj_is_persistent(grn_ctx *ctx, grn_obj *obj);
void grn_obj_spec_save(grn_ctx *ctx, grn_db_obj *obj);

grn_rc grn_obj_reinit_for(grn_ctx *ctx, grn_obj *obj, grn_obj *domain_obj);

void grn_obj_ensure_bulk(grn_ctx *ctx, grn_obj *obj);
void grn_obj_ensure_vector(grn_ctx *ctx, grn_obj *obj);

void grn_expr_pack(grn_ctx *ctx, grn_obj *buf, grn_obj *expr);
GRN_API grn_rc grn_expr_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *expr);
grn_hash *grn_expr_get_vars(grn_ctx *ctx, grn_obj *expr, unsigned int *nvars);
grn_obj *grn_expr_open(grn_ctx *ctx, grn_obj_spec *spec, const uint8_t *p, const uint8_t *pe);

GRN_API grn_rc grn_table_group_with_range_gap(grn_ctx *ctx, grn_obj *table,
                                              grn_table_sort_key *group_key,
                                              grn_obj *result_set,
                                              uint32_t range_gap);

GRN_API grn_rc grn_column_filter(grn_ctx *ctx, grn_obj *column,
                                 grn_operator op,
                                 grn_obj *value, grn_obj *result_set,
                                 grn_operator set_op);

typedef struct {
  grn_id target;
  unsigned int section;
} grn_obj_default_set_value_hook_data;

grn_obj *grn_obj_default_set_value_hook(grn_ctx *ctx,
                                        int nargs,
                                        grn_obj **args,
                                        grn_user_data *user_data);

grn_rc
grn_column_get_all_token_columns(grn_ctx *ctx,
                                 grn_obj *obj,
                                 grn_obj *token_columns);

grn_rc grn_pvector_fin(grn_ctx *ctx, grn_obj *obj);

#ifdef __cplusplus
}
#endif
