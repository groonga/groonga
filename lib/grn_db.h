/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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

#define DB_OBJ(obj) ((grn_db_obj *)obj)

#define GRN_DEFINE_NAME_CUSTOM(obj, prefix)                             \
  const char *prefix;                                                   \
  char prefix ## _buffer[GRN_TABLE_MAX_KEY_SIZE];                       \
  int prefix ## _size;                                                  \
  do {                                                                  \
    if (!obj) {                                                         \
      prefix = "(NULL)";                                                \
      prefix ## _size = (int)strlen(prefix);                            \
    } else if (DB_OBJ(obj)->id == GRN_ID_NIL) {                         \
      prefix = "(temporary)";                                           \
      prefix ## _size = (int)strlen(prefix);                            \
    } else {                                                            \
      prefix ## _size = grn_obj_name(ctx, (grn_obj *)obj,               \
                                     prefix ## _buffer,                 \
                                     GRN_TABLE_MAX_KEY_SIZE);           \
      prefix = prefix ## _buffer;                                       \
      if (prefix ## _size == 0) {                                       \
        prefix = "(anonymous)";                                         \
        prefix ## _size = (int)strlen(prefix);                          \
      } else if (prefix ## _size < GRN_TABLE_MAX_KEY_SIZE) {            \
        prefix ## _buffer[prefix ## _size] = '\0';                      \
      }                                                                 \
    }                                                                   \
  } while (false)

#define GRN_DEFINE_NAME(obj) GRN_DEFINE_NAME_CUSTOM(obj, name)

typedef struct _grn_db grn_db;
typedef struct _grn_proc grn_proc;

typedef struct {
  uint32_t count;
  grn_obj ids;
} grn_deferred_unref;

struct _grn_db {
  grn_db_obj obj;
  grn_obj *keys;
  grn_ja *specs;
  grn_hash *config;
  grn_tiny_array values;
  grn_critical_section lock;
  grn_cache *cache;
  grn_options *options;
  bool is_closing;
  grn_array *deferred_unrefs;
  bool is_deferred_unrefing;
};

#define GRN_SERIALIZED_SPEC_INDEX_SPEC   0
#define GRN_SERIALIZED_SPEC_INDEX_PATH   1
#define GRN_SERIALIZED_SPEC_INDEX_SOURCE 2
#define GRN_SERIALIZED_SPEC_INDEX_HOOK   3
#define GRN_SERIALIZED_SPEC_INDEX_TOKEN_FILTERS 4
#define GRN_SERIALIZED_SPEC_INDEX_NORMALIZERS 5
#define GRN_SERIALIZED_SPEC_INDEX_EXPR   4

typedef struct {
  grn_obj_header header;
  grn_id range;
} grn_obj_spec;

bool grn_obj_spec_unpack(grn_ctx *ctx,
                         grn_id id,
                         void *encoded_spec,
                         uint32_t encoded_spec_size,
                         grn_obj_spec **spec,
                         grn_obj *decoded_spec,
                         const char *error_message_tag);

void grn_obj_spec_get_path(grn_ctx *ctx,
                           grn_obj_spec *spec,
                           grn_id id,
                           char *buffer,
                           grn_db *db,
                           grn_obj *decoded_spec);

#define GRN_DB_EACH_SPEC_BEGIN(ctx, cursor, id, spec) do {              \
  grn_obj *db = grn_ctx_db((ctx));                                      \
  grn_db *db_raw = (grn_db *)db;                                        \
  grn_obj decoded_spec;                                                 \
  grn_io_win iw;                                                        \
  grn_bool iw_need_unref = GRN_FALSE;                                   \
  GRN_OBJ_INIT(&decoded_spec, GRN_VECTOR, 0, GRN_DB_TEXT);              \
  GRN_TABLE_EACH_BEGIN((ctx), db, cursor, id) {                         \
    void *encoded_spec;                                                 \
    uint32_t encoded_spec_size;                                         \
    bool success;                                                       \
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
    success = grn_obj_spec_unpack(ctx,                                  \
                                  id,                                   \
                                  encoded_spec,                         \
                                  encoded_spec_size,                    \
                                  &spec,                                \
                                  &decoded_spec,                        \
                                  __FUNCTION__);                        \
   if (!success) {                                                      \
     continue;                                                          \
   }                                                                    \

#define GRN_DB_EACH_SPEC_END(ctx, cursor)         \
  } GRN_TABLE_EACH_END(ctx, cursor);              \
  if (iw_need_unref) {                            \
    grn_ja_unref(ctx, &iw);                       \
  }                                               \
  GRN_OBJ_FIN((ctx), &decoded_spec);              \
} while(GRN_FALSE)

void grn_db_init_from_env(void);

void grn_db_wal_recover(grn_ctx *ctx, grn_db *db);

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
grn_rc grn_obj_refer_auto_release(grn_ctx *ctx,
                                  grn_obj *obj,
                                  uint32_t count);
grn_rc grn_obj_refer_recursive_auto_release(grn_ctx *ctx,
                                            grn_obj *obj,
                                            uint32_t count);
grn_rc grn_obj_refer_recursive_dependent_auto_release(grn_ctx *ctx,
                                                      grn_obj *obj,
                                                      uint32_t count);
grn_rc
grn_db_add_deferred_unref(grn_ctx *ctx,
                          grn_obj *db,
                          grn_deferred_unref *deferred_unref);
grn_rc
grn_db_remove_deferred_unref(grn_ctx *ctx,
                             grn_obj *db,
                             grn_id id);
grn_rc
grn_db_command_processed(grn_ctx *ctx,
                         grn_obj *db);

grn_rc _grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id,
                               grn_table_delete_optarg *optarg);

grn_id grn_table_get_v(grn_ctx *ctx, grn_obj *table, const void *key, int key_size,
                       void **value);
grn_id grn_table_get_by_key(grn_ctx *ctx,
                            grn_obj *table,
                            grn_obj *key);
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

grn_inline static void
grn_table_add_subrec(grn_ctx *ctx,
                     grn_obj *table,
                     grn_rset_recinfo *ri,
                     double score,
                     grn_rset_posinfo *pi,
                     int dir)
{
  if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
    ri->score += score;
    ri->n_subrecs += 1;
    /* This is a duplicated check but it reduces the number of
     * function calls. It improves performance when many records are
     * matched. */
    uint32_t limit = DB_OBJ(table)->max_n_subrecs;
    if (limit > 0) {
      grn_rset_add_subrec(ctx, ri, table, score, pi, dir);
    }
  }
}

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
      grn_applier_func *applier;
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
  GRN_ACCESSOR_GET_MEAN,
  GRN_ACCESSOR_GET_COLUMN_VALUE,
  GRN_ACCESSOR_GET_DB_OBJ,
  GRN_ACCESSOR_LOOKUP,
  GRN_ACCESSOR_FUNCALL
} grn_accessor_action;

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

grn_obj *grn_table_column(grn_ctx *ctx,
                          grn_obj *table,
                          const char *name,
                          ssize_t name_size);
grn_rc grn_table_parse_load_columns(grn_ctx *ctx,
                                    grn_obj *table,
                                    const char *input,
                                    size_t input_size,
                                    grn_obj *columns);

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
grn_rc
grn_column_get_all_hooked_columns(grn_ctx *ctx,
                                  grn_obj *obj,
                                  grn_obj *hooked_columns);

grn_rc grn_pvector_fin(grn_ctx *ctx, grn_obj *obj);


static inline grn_hash *
grn_id_map_open(grn_ctx *ctx)
{
  return grn_hash_create(ctx,
                         NULL,
                         sizeof(grn_id),
                         sizeof(grn_id),
                         GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
}

static inline grn_rc
grn_id_map_close(grn_ctx *ctx, grn_hash *id_map)
{
  return grn_hash_close(ctx, id_map);
}

static inline void
grn_id_map_add(grn_ctx *ctx, grn_hash *id_map, grn_id old_id, grn_id new_id)
{
  void *value;
  grn_hash_add(ctx, id_map, &old_id, sizeof(grn_id), &value, NULL);
  *(grn_id *)value = new_id;
}

static inline grn_id
grn_id_map_resolve(grn_ctx *ctx, grn_hash *id_map, grn_id id)
{
  if (id == GRN_ID_NIL) {
    return id;
  }
  if (!id_map) {
    return id;
  }
  void *value;
  if (grn_hash_get(ctx, id_map, &id, sizeof(grn_id), &value) == GRN_ID_NIL) {
    return id;
  }
  return *(grn_id *)value;
}

#define GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)                                      \
  do {                                                                         \
    grn_wal_role wal_role_current = grn_ctx_get_wal_role(ctx);            \
    grn_ctx_set_wal_role(ctx, GRN_WAL_ROLE_NONE);

#define GRN_DB_WAL_DISABLE_WAL_END(ctx)                                        \
  grn_ctx_set_wal_role(ctx, wal_role_current);                             \
  }                                                                            \
  while (false)

static inline grn_obj *
grn_table_create_similar_id_map(grn_ctx *ctx,
                                const char *name,
                                uint32_t name_size,
                                const char *path,
                                grn_obj *base_table,
                                grn_hash *id_map)
{
  const char *tag = "[table][create][similar]";
  if (!grn_obj_is_table(ctx, base_table)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, base_table);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be table: %.*s",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  grn_table_flags flags = 0;
  grn_id domain = GRN_ID_NIL;
  grn_id range = GRN_ID_NIL;
  grn_obj *key_type = NULL;
  grn_obj *value_type = NULL;

  if (grn_obj_is_persistent(ctx, base_table)) {
    flags |= GRN_OBJ_PERSISTENT;
  }

  switch (base_table->header.type) {
  case GRN_TABLE_HASH_KEY:
    flags |= GRN_OBJ_TABLE_HASH_KEY;
    if (((grn_hash *)base_table)->header.common->flags & GRN_OBJ_KEY_LARGE) {
      flags |= GRN_OBJ_KEY_LARGE;
    }
    domain = base_table->header.domain;
    range = DB_OBJ(base_table)->range;
    break;
  case GRN_TABLE_PAT_KEY:
    flags |= GRN_OBJ_TABLE_PAT_KEY;
    if (base_table->header.flags & GRN_OBJ_KEY_WITH_SIS) {
      flags |= GRN_OBJ_KEY_WITH_SIS;
    }
    domain = base_table->header.domain;
    range = DB_OBJ(base_table)->range;
    break;
  case GRN_TABLE_DAT_KEY:
    flags |= GRN_OBJ_TABLE_DAT_KEY;
    domain = base_table->header.domain;
    break;
  case GRN_TABLE_NO_KEY:
    flags |= GRN_OBJ_TABLE_NO_KEY;
    range = DB_OBJ(base_table)->range;
    break;
  default:
    break;
  }
  if (domain != GRN_ID_NIL) {
    domain = grn_id_map_resolve(ctx, id_map, domain);
    key_type = grn_ctx_at(ctx, domain);
  }
  if (range != GRN_ID_NIL) {
    range = grn_id_map_resolve(ctx, id_map, range);
    value_type = grn_ctx_at(ctx, range);
  }
  grn_obj *table =
    grn_table_create(ctx, name, name_size, path, flags, key_type, value_type);
  GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)
  {
    if (key_type) {
      grn_obj_unref(ctx, key_type);
    }
    if (value_type) {
      grn_obj_unref(ctx, value_type);
    }
  }
  GRN_DB_WAL_DISABLE_WAL_END(ctx);
  if (!table) {
    return NULL;
  }

  grn_obj buffer;
  GRN_TEXT_INIT(&buffer, 0);
  if (grn_obj_is_table_with_key(ctx, base_table)) {
    GRN_BULK_REWIND(&buffer);
    grn_table_get_default_tokenizer_string(ctx, base_table, &buffer);
    if (GRN_TEXT_LEN(&buffer) > 0) {
      grn_obj_set_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER, &buffer);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }

    GRN_BULK_REWIND(&buffer);
    grn_table_get_normalizers_string(ctx, base_table, &buffer);
    if (GRN_TEXT_LEN(&buffer) > 0) {
      grn_obj_set_info(ctx, table, GRN_INFO_NORMALIZERS, &buffer);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }

    GRN_BULK_REWIND(&buffer);
    grn_table_get_token_filters_string(ctx, base_table, &buffer);
    if (GRN_TEXT_LEN(&buffer) > 0) {
      grn_obj_set_info(ctx, table, GRN_INFO_TOKEN_FILTERS, &buffer);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }
  }

exit:
  GRN_OBJ_FIN(ctx, &buffer);

  if (ctx->rc != GRN_SUCCESS) {
    grn_obj_remove(ctx, table);
    return NULL;
  }

  return table;
}

static inline grn_obj *
grn_column_create_similar_id_map(grn_ctx *ctx,
                                 grn_obj *table,
                                 const char *name,
                                 uint32_t name_size,
                                 const char *path,
                                 grn_obj *base_column,
                                 grn_hash *id_map)
{
  const char *tag = "[column][create][similar]";

  if (!grn_obj_is_table(ctx, table)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, table);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be table: %.*s",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }
  if (!name || name_size == 0) {
    ERR(GRN_INVALID_ARGUMENT, "%s name is missing", tag);
    return NULL;
  }
  if (!grn_obj_is_column(ctx, base_column)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, base_column);
    ERR(GRN_INVALID_ARGUMENT,
        "%s must be column: %.*s",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  grn_column_flags flags = 0;
  if (grn_obj_is_persistent(ctx, table)) {
    flags |= GRN_OBJ_PERSISTENT;
  }
  grn_id type_id = DB_OBJ(base_column)->range;
  type_id = grn_id_map_resolve(ctx, id_map, type_id);
  grn_obj *type = grn_ctx_at(ctx, type_id);
  flags |= grn_column_get_flags(ctx, base_column);
  grn_obj *column =
    grn_column_create(ctx, table, name, name_size, path, flags, type);
  if (!column) {
    return NULL;
  }

  switch (column->header.type) {
  case GRN_COLUMN_VAR_SIZE:
    {
      grn_obj base_source_ids;
      GRN_RECORD_INIT(&base_source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
      grn_obj_get_info(ctx, base_column, GRN_INFO_SOURCE, &base_source_ids);
      size_t n = GRN_RECORD_VECTOR_SIZE(&base_source_ids);
      if (n > 0) {
        grn_obj source_ids;
        GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
        size_t i;
        for (i = 0; i < n; i++) {
          grn_id base_source_column_id =
            GRN_RECORD_VALUE_AT(&base_source_ids, i);
          grn_obj *base_source_column = grn_ctx_at(ctx, base_source_column_id);
          if (!base_source_column) {
            ERR(GRN_INVALID_ARGUMENT,
                "%s base column's source doesn't exist: %u",
                tag,
                base_source_column_id);
            break;
          }
          char source_column_name[GRN_TABLE_MAX_KEY_SIZE];
          int source_column_name_length =
            grn_column_name(ctx,
                            base_source_column,
                            source_column_name,
                            sizeof(source_column_name));
          GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)
          {
            grn_obj_unref(ctx, base_source_column);
          }
          GRN_DB_WAL_DISABLE_WAL_END(ctx);
          grn_obj *source_column = grn_table_column(ctx,
                                                    table,
                                                    source_column_name,
                                                    source_column_name_length);
          if (!source_column) {
            ERR(GRN_INVALID_ARGUMENT,
                "%s source column doesn't exist: <%.*s.%.*s>",
                tag,
                (int)name_size,
                name,
                source_column_name_length,
                source_column_name);
            break;
          }
          GRN_RECORD_PUT(ctx, &source_ids, DB_OBJ(source_column)->id);
          GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)
          {
            grn_obj_unref(ctx, source_column);
          }
          GRN_DB_WAL_DISABLE_WAL_END(ctx);
        }
        if (ctx->rc == GRN_SUCCESS) {
          grn_obj_set_info(ctx, column, GRN_INFO_SOURCE, &source_ids);
        }
        GRN_OBJ_FIN(ctx, &source_ids);
      }
      GRN_OBJ_FIN(ctx, &base_source_ids);
      if (ctx->rc != GRN_SUCCESS) {
        grn_rc original_rc = ctx->rc;
        ctx->rc = GRN_SUCCESS;
        grn_obj_remove(ctx, column);
        ctx->rc = original_rc;
        return NULL;
      }
    }
    break;
  case GRN_COLUMN_INDEX:
    {
      grn_obj source_ids;
      GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
      grn_obj_get_info(ctx, base_column, GRN_INFO_SOURCE, &source_ids);
      size_t n_source_ids = GRN_RECORD_VECTOR_SIZE(&source_ids);
      if (n_source_ids > 0) {
        grn_obj resolved_source_ids;
        GRN_RECORD_INIT(&resolved_source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
        size_t i;
        for (i = 0; i < n_source_ids; i++) {
          grn_id source_id = GRN_RECORD_VALUE_AT(&source_ids, i);
          grn_id resolved_source_id =
            grn_id_map_resolve(ctx, id_map, source_id);
          GRN_RECORD_PUT(ctx, &resolved_source_ids, resolved_source_id);
        }
        grn_obj_set_info(ctx, column, GRN_INFO_SOURCE, &resolved_source_ids);
        GRN_OBJ_FIN(ctx, &resolved_source_ids);
      }
      GRN_OBJ_FIN(ctx, &source_ids);
      if (ctx->rc != GRN_SUCCESS) {
        grn_rc original_rc = ctx->rc;
        ctx->rc = GRN_SUCCESS;
        grn_obj_remove(ctx, column);
        ctx->rc = original_rc;
        return NULL;
      }
    }
    break;
  default:
    break;
  }

  return column;
}

#ifdef __cplusplus
}
#endif
