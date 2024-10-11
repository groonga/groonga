/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>
  Copyright (C) 2021       Horimoto Yasuhiro <horimoto@clear-code.com>

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

#include "grn.h"
#include "grn_tokenizer.h"

#include "grn_accessor.h"
#include "grn_aggregators.h"
#include "grn_config.h"
#include "grn_db.h"
#include "grn_obj.h"
#include "grn_hash.h"
#include "grn_pat.h"
#include "grn_dat.h"
#include "grn_ii.h"
#include "grn_index_column.h"
#include "grn_ctx_impl.h"
#include "grn_table.h"
#include "grn_table_selector.h"
#include "grn_token_cursor.h"
#include "grn_tokenizers.h"
#include "grn_token_filters.h"
#include "grn_proc.h"
#include "grn_plugin.h"
#include "grn_geo.h"
#include "grn_scorers.h"
#include "grn_snip.h"
#include "grn_string.h"
#include "grn_normalizer.h"
#include "grn_report.h"
#include "grn_util.h"
#include "grn_cache.h"
#include "grn_window_functions.h"
#include "grn_expr.h"
#include "grn_cast.h"
#include "grn_token_column.h"
#include "grn_type.h"
#include "grn_posting.h"
#include "grn_vector.h"
#include "grn_index_cursor.h"
#include "grn_wal.h"

#include <math.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
#  include <share.h>
#endif

static const uint32_t GRN_TABLE_PAT_KEY_CACHE_SIZE = 1 << 15;

#define WITH_NORMALIZE(table, key, key_size, block)                            \
  do {                                                                         \
    if (GRN_BULK_VSIZE(&((table)->normalizers)) > 0 && key && key_size > 0) {  \
      grn_obj *nstr;                                                           \
      if ((nstr =                                                              \
             grn_string_open(ctx, key, key_size, (grn_obj *)(table), 0))) {    \
        const char *key;                                                       \
        unsigned int key_size;                                                 \
        grn_string_get_normalized(ctx, nstr, &key, &key_size, NULL);           \
        do {                                                                   \
          block                                                                \
        } while (false);                                                       \
        grn_obj_close(ctx, nstr);                                              \
      } else {                                                                 \
        char name[GRN_TABLE_MAX_KEY_SIZE];                                     \
        int name_size;                                                         \
        name_size =                                                            \
          grn_obj_name(ctx, (grn_obj *)(table), name, GRN_TABLE_MAX_KEY_SIZE); \
        ERR(GRN_INVALID_ARGUMENT,                                              \
            "[key][normalize] failed to normalize: <%.*s>: <%.*s>(%d)",        \
            name_size,                                                         \
            name,                                                              \
            (int)key_size,                                                     \
            (const char *)key,                                                 \
            (int)key_size);                                                    \
      }                                                                        \
    } else {                                                                   \
      block                                                                    \
    }                                                                          \
  } while (false)

grn_inline static grn_id
grn_table_add_v_inline(grn_ctx *ctx,
                       grn_obj *table,
                       const void *key,
                       int key_size,
                       void **value,
                       int *added);
grn_inline static grn_id
grn_table_cursor_next_inline(grn_ctx *ctx, grn_table_cursor *tc);
grn_inline static int
grn_table_cursor_get_value_inline(grn_ctx *ctx,
                                  grn_table_cursor *tc,
                                  void **value);

static bool grn_enable_reference_count = false;

static uint32_t grn_n_opening_dbs = 0;
static char grn_db_key[GRN_ENV_BUFFER_SIZE];

void
grn_db_init_from_env(void)
{
  grn_getenv("GRN_DB_KEY", grn_db_key, GRN_ENV_BUFFER_SIZE);
  {
    char grn_enable_reference_count_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_ENABLE_REFERENCE_COUNT",
               grn_enable_reference_count_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_enable_reference_count_env[0]) {
      if (strncmp(grn_enable_reference_count_env, "yes", strlen("yes")) == 0) {
        grn_enable_reference_count = true;
      } else {
        grn_enable_reference_count = false;
      }
    }
  }
}

bool
grn_is_reference_count_enable(void)
{
  return grn_enable_reference_count;
}

grn_rc
grn_set_reference_count_enable(bool enable)
{
  if (grn_n_opening_dbs != 0) {
    return GRN_OPERATION_NOT_PERMITTED;
  }
  grn_enable_reference_count = enable;
  return GRN_SUCCESS;
}

const char *
grn_hook_entry_to_string(grn_hook_entry entry)
{
  switch (entry) {
  case GRN_HOOK_SET:
    return "set";
  case GRN_HOOK_GET:
    return "get";
  case GRN_HOOK_INSERT:
    return "insert";
  case GRN_HOOK_DELETE:
    return "delete";
  case GRN_HOOK_SELECT:
    return "select";
  default:
    return "unknown";
  }
}

grn_inline static void
gen_pathname(const char *path, char *buffer, int fno)
{
  size_t len = strlen(path);
  grn_memcpy(buffer, path, len);
  if (fno >= 0) {
    buffer[len] = '.';
    grn_itoh(fno, buffer + len + 1, 7);
    buffer[len + 8] = '\0';
  } else {
    buffer[len] = '\0';
  }
}

void
grn_db_generate_pathname(grn_ctx *ctx, grn_obj *db, grn_id id, char *buffer)
{
  gen_pathname(grn_obj_get_io(ctx, db)->path, buffer, id);
}

typedef struct {
  grn_obj *ptr;
  uint32_t lock;
  uint32_t done;
} db_value;

static const char *GRN_DB_CONFIG_PATH_FORMAT = "%s.conf";

static bool
grn_db_config_create(grn_ctx *ctx,
                     grn_db *s,
                     const char *path,
                     const char *context_tag)
{
  char *config_path;
  char config_path_buffer[PATH_MAX];
  uint32_t flags = GRN_OBJ_KEY_VAR_SIZE;

  if (path) {
    grn_snprintf(config_path_buffer,
                 PATH_MAX,
                 PATH_MAX,
                 GRN_DB_CONFIG_PATH_FORMAT,
                 path);
    config_path = config_path_buffer;
  } else {
    config_path = NULL;
  }
  s->config = grn_hash_create(ctx,
                              config_path,
                              GRN_CONFIG_MAX_KEY_SIZE,
                              GRN_CONFIG_VALUE_SPACE_SIZE,
                              flags);
  if (!s->config) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to create data store for configuration: <%s>",
        context_tag,
        config_path ? config_path : "(temporary)");
    return false;
  }

  return true;
}

static bool
grn_db_config_open(grn_ctx *ctx, grn_db *s, const char *path)
{
  char config_path[PATH_MAX];

  grn_snprintf(config_path,
               PATH_MAX,
               PATH_MAX,
               GRN_DB_CONFIG_PATH_FORMAT,
               path);
  if (grn_path_exist(config_path)) {
    s->config = grn_hash_open(ctx, config_path);
    if (!s->config) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[db][open] failed to open data store for configuration: <%s>",
          config_path);
      return false;
    }
    return true;
  } else {
    return grn_db_config_create(ctx, s, path, "[db][open]");
  }
}

static grn_rc
grn_db_config_remove(grn_ctx *ctx, const char *path)
{
  char config_path[PATH_MAX];

  grn_snprintf(config_path,
               PATH_MAX,
               PATH_MAX,
               GRN_DB_CONFIG_PATH_FORMAT,
               path);
  return grn_hash_remove(ctx, config_path);
}

grn_obj *
grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg)
{
  grn_db *s = NULL;

  GRN_API_ENTER;

  if (path && strlen(path) > PATH_MAX - 14) {
    ERR(GRN_INVALID_ARGUMENT, "too long path");
    goto exit;
  }

  s = GRN_MALLOC(sizeof(grn_db));
  if (!s) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "grn_db alloc failed");
    goto exit;
  }

  CRITICAL_SECTION_INIT(s->lock);
  grn_tiny_array_init(ctx,
                      &s->values,
                      sizeof(db_value),
                      GRN_TINY_ARRAY_CLEAR | GRN_TINY_ARRAY_THREADSAFE |
                        GRN_TINY_ARRAY_USE_MALLOC);
  s->keys = NULL;
  s->specs = NULL;
  s->config = NULL;
  s->cache = NULL;
  s->options = NULL;
  s->is_closing = false;
  s->deferred_unrefs =
    grn_array_create(ctx, NULL, sizeof(grn_deferred_unref), GRN_TABLE_NO_KEY);
  s->is_deferred_unrefing = false;

  {
    bool use_default_db_key = true;
    bool use_pat_as_db_keys = false;
    if (grn_db_key[0]) {
      if (!strcmp(grn_db_key, "pat")) {
        use_default_db_key = false;
        use_pat_as_db_keys = true;
      } else if (!strcmp(grn_db_key, "dat")) {
        use_default_db_key = false;
      }
    }

    if (use_default_db_key && !strcmp(GRN_DEFAULT_DB_KEY, "pat")) {
      use_pat_as_db_keys = true;
    }
    if (use_pat_as_db_keys) {
      s->keys = (grn_obj *)grn_pat_create(ctx,
                                          path,
                                          GRN_TABLE_MAX_KEY_SIZE,
                                          0,
                                          GRN_OBJ_KEY_VAR_SIZE);
    } else {
      s->keys = (grn_obj *)grn_dat_create(ctx,
                                          path,
                                          GRN_TABLE_MAX_KEY_SIZE,
                                          0,
                                          GRN_OBJ_KEY_VAR_SIZE);
    }
  }

  if (!s->keys) {
    goto exit;
  }

  GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
  s->obj.db = (grn_obj *)s;
  s->obj.header.domain = GRN_ID_NIL;
  DB_OBJ(&s->obj)->range = GRN_ID_NIL;
  /* prepare builtin classes and load builtin plugins. */
  if (path) {
    {
      char specs_path[PATH_MAX];
      gen_pathname(path, specs_path, 0);
      s->specs = grn_ja_create(ctx, specs_path, 65536, 0);
      if (!s->specs) {
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "failed to create specs: <%s>",
            specs_path);
        goto exit;
      }
    }
  }

  if (!grn_db_config_create(ctx, s, path, "[db][create]")) {
    goto exit;
  }
  s->options = grn_options_create(ctx, path, "[db][create]");
  if (!s->options) {
    goto exit;
  }
  grn_ctx_use(ctx, (grn_obj *)s);
  grn_db_init_builtin_types(ctx);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  if (path) {
    grn_obj_flush(ctx, (grn_obj *)s);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }
  {
    uint32_t current_n_opening_dbs;
    GRN_ATOMIC_ADD_EX(&grn_n_opening_dbs, 1, current_n_opening_dbs);
  }
  GRN_API_RETURN((grn_obj *)s);

exit:
  if (s) {
    if (s->keys) {
      if (s->keys->header.type == GRN_TABLE_PAT_KEY) {
        grn_pat_close(ctx, (grn_pat *)s->keys);
        grn_pat_remove(ctx, path);
      } else {
        grn_dat_close(ctx, (grn_dat *)s->keys);
        grn_dat_remove(ctx, path);
      }
    }
    if (s->specs) {
      const char *specs_path;
      char specs_path_copy[PATH_MAX];
      specs_path = grn_obj_path(ctx, (grn_obj *)(s->specs));
      grn_strcpy(specs_path_copy, sizeof(specs_path_copy), specs_path);
      grn_ja_close(ctx, s->specs);
      grn_ja_remove(ctx, specs_path_copy);
    }
    if (s->config) {
      grn_hash_close(ctx, s->config);
      grn_db_config_remove(ctx, path);
    }
    if (s->options) {
      grn_options_close(ctx, s->options);
      grn_options_remove(ctx, path);
    }
    grn_tiny_array_fin(&s->values);
    CRITICAL_SECTION_FIN(s->lock);
    GRN_FREE(s);
  }

  GRN_API_RETURN(NULL);
}

#define GRN_TYPE_FLOAT32_NAME     "Float32"
#define GRN_TYPE_FLOAT32_NAME_LEN (sizeof(GRN_TYPE_FLOAT32_NAME) - 1)
#define GRN_TYPE_FLOAT32_FLAGS    GRN_OBJ_KEY_FLOAT
#define GRN_TYPE_FLOAT32_SIZE     sizeof(float)

static bool
grn_db_open_ensure_float32(grn_ctx *ctx, grn_db *db)
{
  if (grn_table_get(ctx,
                    db->keys,
                    GRN_TYPE_FLOAT32_NAME,
                    GRN_TYPE_FLOAT32_NAME_LEN) == GRN_DB_FLOAT32) {
    return false;
  }

  if (db->keys->header.type != GRN_TABLE_DAT_KEY) {
    return false;
  }

  if (grn_table_update_by_id(ctx,
                             db->keys,
                             GRN_DB_FLOAT32,
                             GRN_TYPE_FLOAT32_NAME,
                             GRN_TYPE_FLOAT32_NAME_LEN) != GRN_SUCCESS) {
    return false;
  }

  grn_obj *float32 = grn_type_create_internal(ctx,
                                              GRN_DB_FLOAT32,
                                              GRN_TYPE_FLOAT32_FLAGS,
                                              GRN_TYPE_FLOAT32_SIZE);
  return float32 != NULL;
}

#define GRN_TYPE_BFLOAT16_NAME     "BFloat16"
#define GRN_TYPE_BFLOAT16_NAME_LEN (sizeof(GRN_TYPE_BFLOAT16_NAME) - 1)
#define GRN_TYPE_BFLOAT16_FLAGS    GRN_OBJ_KEY_FLOAT
#ifdef GRN_HAVE_BFLOAT16
#  define GRN_TYPE_BFLOAT16_SIZE sizeof(grn_bfloat16)
#else
#  define GRN_TYPE_BFLOAT16_SIZE sizeof(uint16_t)
#endif

static bool
grn_db_open_ensure_bfloat16(grn_ctx *ctx, grn_db *db)
{
  if (grn_table_get(ctx,
                    db->keys,
                    GRN_TYPE_BFLOAT16_NAME,
                    GRN_TYPE_BFLOAT16_NAME_LEN) == GRN_DB_BFLOAT16) {
    return false;
  }

  if (db->keys->header.type != GRN_TABLE_DAT_KEY) {
    return false;
  }

  if (grn_table_update_by_id(ctx,
                             db->keys,
                             GRN_DB_BFLOAT16,
                             GRN_TYPE_BFLOAT16_NAME,
                             GRN_TYPE_BFLOAT16_NAME_LEN) != GRN_SUCCESS) {
    return false;
  }

  grn_obj *bfloat16 = grn_type_create_internal(ctx,
                                               GRN_DB_BFLOAT16,
                                               GRN_TYPE_BFLOAT16_FLAGS,
                                               GRN_TYPE_BFLOAT16_SIZE);
  return bfloat16 != NULL;
}

grn_obj *
grn_db_open(grn_ctx *ctx, const char *path)
{
  grn_db *s = NULL;

  GRN_API_ENTER;

  if (!path) {
    ERR(GRN_INVALID_ARGUMENT, "[db][open] path is missing");
    goto exit;
  }

  if (strlen(path) > PATH_MAX - 14) {
    ERR(GRN_INVALID_ARGUMENT, "inappropriate path");
    goto exit;
  }

  s = GRN_MALLOC(sizeof(grn_db));
  if (!s) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "grn_db alloc failed");
    goto exit;
  }

  CRITICAL_SECTION_INIT(s->lock);
  grn_tiny_array_init(ctx,
                      &s->values,
                      sizeof(db_value),
                      GRN_TINY_ARRAY_CLEAR | GRN_TINY_ARRAY_THREADSAFE |
                        GRN_TINY_ARRAY_USE_MALLOC);
  s->keys = NULL;
  s->specs = NULL;
  s->config = NULL;
  s->cache = NULL;
  s->options = NULL;
  s->is_closing = false;
  s->deferred_unrefs =
    grn_array_create(ctx, NULL, sizeof(grn_deferred_unref), GRN_TABLE_NO_KEY);
  s->is_deferred_unrefing = false;

  {
    uint32_t type = grn_io_detect_type(ctx, path);
    switch (type) {
    case GRN_TABLE_PAT_KEY:
      s->keys = (grn_obj *)grn_pat_open(ctx, path);
      break;
    case GRN_TABLE_DAT_KEY:
      s->keys = (grn_obj *)grn_dat_open(ctx, path);
      break;
    default:
      s->keys = NULL;
      if (ctx->rc == GRN_SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "[db][open] invalid keys table's type: %#x",
            type);
        goto exit;
      }
      break;
    }
  }

  if (!s->keys) {
    goto exit;
  }

  {
    char specs_path[PATH_MAX];
    gen_pathname(path, specs_path, 0);
    s->specs = grn_ja_open(ctx, specs_path);
    if (!s->specs) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[db][open] failed to open specs: <%s>",
          specs_path);
      goto exit;
    }
  }
  if (!grn_db_config_open(ctx, s, path)) {
    goto exit;
  }
  s->options = grn_options_open(ctx, path, "[db][open]");
  if (!s->options) {
    goto exit;
  }

  GRN_DB_OBJ_SET_TYPE(s, GRN_DB);
  s->obj.db = (grn_obj *)s;
  s->obj.header.domain = GRN_ID_NIL;
  DB_OBJ(&s->obj)->range = GRN_ID_NIL;
  grn_ctx_use(ctx, (grn_obj *)s);
  {
    bool need_flush = false;
    unsigned int n_records;

    n_records = grn_table_size(ctx, (grn_obj *)s);
    if (grn_db_open_ensure_float32(ctx, s)) {
      need_flush = true;
    }
    if (grn_db_open_ensure_bfloat16(ctx, s)) {
      need_flush = true;
    }
#ifdef GRN_WITH_MECAB
    if (grn_db_init_mecab_tokenizer(ctx)) {
      ERRCLR(ctx);
    }
#endif
    grn_db_init_builtin_tokenizers(ctx);
    grn_db_init_builtin_normalizers(ctx);
    grn_db_init_builtin_scorers(ctx);
    grn_db_init_builtin_commands(ctx);
    grn_db_init_builtin_window_functions(ctx);
    grn_db_init_builtin_token_filters(ctx);
    grn_db_init_builtin_aggregators(ctx);

    if (grn_table_size(ctx, (grn_obj *)s) > n_records) {
      need_flush = true;
    }
    if (need_flush) {
      grn_obj_flush(ctx, (grn_obj *)s);
    }
  }
  {
    uint32_t current_n_opening_dbs;
    GRN_ATOMIC_ADD_EX(&grn_n_opening_dbs, 1, current_n_opening_dbs);
  }
  grn_db_wal_recover(ctx, s);
  if (ctx->rc != GRN_SUCCESS) {
    grn_db_close(ctx, (grn_obj *)s);
    s = NULL;
    ctx->impl->db = NULL;
    goto exit;
  }
  GRN_API_RETURN((grn_obj *)s);

exit:
  if (s) {
    grn_options_close(ctx, s->options);
    if (s->config) {
      grn_hash_close(ctx, s->config);
    }
    if (s->specs) {
      grn_ja_close(ctx, s->specs);
    }
    if (s->keys) {
      if (s->keys->header.type == GRN_TABLE_PAT_KEY) {
        grn_pat_close(ctx, (grn_pat *)s->keys);
      } else {
        grn_dat_close(ctx, (grn_dat *)s->keys);
      }
    }
    grn_array_close(ctx, s->deferred_unrefs);
    grn_tiny_array_fin(&s->values);
    CRITICAL_SECTION_FIN(s->lock);
    GRN_FREE(s);
  }

  GRN_API_RETURN(NULL);
}

static grn_id
grn_db_curr_id(grn_ctx *ctx, grn_obj *db)
{
  grn_id curr_id = GRN_ID_NIL;
  grn_db *s = (grn_db *)db;
  switch (s->keys->header.type) {
  case GRN_TABLE_PAT_KEY:
    curr_id = grn_pat_curr_id(ctx, (grn_pat *)s->keys);
    break;
  case GRN_TABLE_DAT_KEY:
    curr_id = grn_dat_curr_id(ctx, (grn_dat *)s->keys);
    break;
  }
  return curr_id;
}

grn_rc
grn_db_add_deferred_unref(grn_ctx *ctx,
                          grn_obj *db,
                          grn_deferred_unref *deferred_unref)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }
  if (deferred_unref->count == 0) {
    return ctx->rc;
  }
  size_t n_ids = GRN_RECORD_VECTOR_SIZE(&(deferred_unref->ids));
  if (n_ids == 0) {
    return ctx->rc;
  }
  grn_db *db_ = (grn_db *)db;
  CRITICAL_SECTION_ENTER(db_->lock);
  void *value;
  grn_id id = grn_array_add(ctx, db_->deferred_unrefs, &value);
  if (id != GRN_ID_NIL) {
    grn_deferred_unref *value_deferred_unrefs = value;
    value_deferred_unrefs->count = deferred_unref->count;
    GRN_RECORD_INIT(&(value_deferred_unrefs->ids), GRN_OBJ_VECTOR, GRN_ID_NIL);
    /* Copy data */
    GRN_TEXT_SET(ctx,
                 &(value_deferred_unrefs->ids),
                 GRN_TEXT_VALUE(&(deferred_unref->ids)),
                 GRN_TEXT_LEN(&(deferred_unref->ids)));
  }
  CRITICAL_SECTION_LEAVE(db_->lock);
  return ctx->rc;
}

grn_rc
grn_db_remove_deferred_unref(grn_ctx *ctx, grn_obj *db, grn_id id)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }
  if (id == GRN_ID_NIL) {
    return ctx->rc;
  }
  grn_db *db_ = (grn_db *)db;
  if (db_->is_closing) {
    return ctx->rc;
  }
  if (db_->is_deferred_unrefing) {
    return ctx->rc;
  }
  CRITICAL_SECTION_ENTER(db_->lock);
  if (grn_array_size(ctx, db_->deferred_unrefs) > 0) {
    GRN_ARRAY_EACH_BEGIN(ctx,
                         db_->deferred_unrefs,
                         cursor,
                         GRN_ID_NIL,
                         GRN_ID_MAX,
                         id_)
    {
      void *value;
      grn_array_cursor_get_value(ctx, cursor, &value);
      grn_deferred_unref *deferred_unref = value;
      size_t n_ids = GRN_RECORD_VECTOR_SIZE(&(deferred_unref->ids));
      size_t i;
      for (i = 0; i < n_ids; i++) {
        if (GRN_RECORD_VALUE_AT(&(deferred_unref->ids), i) == id) {
          GRN_RECORD_VALUE_AT(&(deferred_unref->ids), i) = GRN_ID_NIL;
        }
      }
    }
    GRN_ARRAY_EACH_END(ctx, cursor);
  }
  CRITICAL_SECTION_LEAVE(db_->lock);
  return ctx->rc;
}

grn_rc
grn_db_command_processed(grn_ctx *ctx, grn_obj *db)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }

  grn_db *db_ = (grn_db *)db;
  CRITICAL_SECTION_ENTER(db_->lock);
  if (grn_array_size(ctx, db_->deferred_unrefs) > 0) {
    db_->is_deferred_unrefing = true;
    GRN_ARRAY_EACH_BEGIN(ctx,
                         db_->deferred_unrefs,
                         cursor,
                         GRN_ID_NIL,
                         GRN_ID_MAX,
                         id)
    {
      void *value;
      grn_array_cursor_get_value(ctx, cursor, &value);
      grn_deferred_unref *deferred_unref = value;
      deferred_unref->count--;
      if (deferred_unref->count == 0) {
        size_t n_ids = GRN_RECORD_VECTOR_SIZE(&(deferred_unref->ids));
        size_t i;
        for (i = 0; i < n_ids; i++) {
          grn_id deferred_id = GRN_RECORD_VALUE_AT(&(deferred_unref->ids), i);
          if (deferred_id == GRN_ID_NIL) {
            continue;
          }
          grn_obj *object = grn_ctx_at(ctx, deferred_id);
          if (object) {
            grn_obj_unref(ctx, object);
            grn_obj_unref(ctx, object);
          }
        }
        GRN_OBJ_FIN(ctx, &(deferred_unref->ids));
        grn_array_cursor_delete(ctx, cursor, NULL);
      }
    }
    GRN_ARRAY_EACH_END(ctx, cursor);
    db_->is_deferred_unrefing = false;
  }
  CRITICAL_SECTION_LEAVE(db_->lock);
  return ctx->rc;
}

/* db must be validated by caller */
grn_rc
grn_db_close(grn_ctx *ctx, grn_obj *db)
{
  grn_db *s = (grn_db *)db;
  if (!s) {
    return GRN_INVALID_ARGUMENT;
  }
  GRN_API_ENTER;

  s->is_closing = true;

  GRN_ARRAY_EACH_BEGIN(ctx,
                       s->deferred_unrefs,
                       cursor,
                       GRN_ID_NIL,
                       GRN_ID_MAX,
                       id)
  {
    void *value;
    grn_array_cursor_get_value(ctx, cursor, &value);
    grn_deferred_unref *deferred_unref = value;
    GRN_OBJ_FIN(ctx, &(deferred_unref->ids));
  }
  GRN_ARRAY_EACH_END(ctx, cursor);
  grn_array_close(ctx, s->deferred_unrefs);

  bool ctx_used_db = ctx->impl && ctx->impl->db == db;
  if (ctx_used_db) {
    grn_ctx_loader_clear(ctx);
    if (ctx->impl->parser) {
      grn_expr_parser_close(ctx);
    }
  }

  if (ctx_used_db) {
    if (ctx->impl->values) {
      grn_db_obj *o;
      GRN_ARRAY_EACH(ctx, ctx->impl->values, 0, 0, id, &o, {
        grn_obj_close(ctx, *((grn_obj **)o));
      });
      grn_array_truncate(ctx, ctx->impl->values);
    }
  }

  GRN_TINY_ARRAY_EACH_BEGIN(&s->values, 1, grn_db_curr_id(ctx, db), value)
  {
    db_value *vp = value;
    if (vp->ptr) {
      if (grn_obj_is_proc(ctx, vp->ptr) || grn_obj_is_table(ctx, vp->ptr)) {
        /* Defer */
      } else {
        grn_obj_close(ctx, vp->ptr);
      }
    }
  }
  GRN_TINY_ARRAY_EACH_END();
  GRN_TINY_ARRAY_EACH_BEGIN(&s->values, 1, grn_db_curr_id(ctx, db), value)
  {
    db_value *vp = value;
    if (vp->ptr) {
      if (grn_obj_is_proc(ctx, vp->ptr)) {
        /* Defer */
      } else {
        grn_obj_close(ctx, vp->ptr);
      }
    }
  }
  GRN_TINY_ARRAY_EACH_END();
  GRN_TINY_ARRAY_EACH_BEGIN(&s->values, 1, grn_db_curr_id(ctx, db), value)
  {
    db_value *vp = value;
    if (vp->ptr) {
      grn_obj_close(ctx, vp->ptr);
    }
  }
  GRN_TINY_ARRAY_EACH_END();
  grn_tiny_array_fin(&s->values);

  switch (s->keys->header.type) {
  case GRN_TABLE_PAT_KEY:
    grn_pat_close(ctx, (grn_pat *)s->keys);
    break;
  case GRN_TABLE_DAT_KEY:
    grn_dat_close(ctx, (grn_dat *)s->keys);
    break;
  }
  CRITICAL_SECTION_FIN(s->lock);
  if (s->specs) {
    grn_ja_close(ctx, s->specs);
  }
  grn_hash_close(ctx, s->config);
  grn_options_close(ctx, s->options);
  GRN_FREE(s);

  if (ctx_used_db) {
    ctx->impl->db = NULL;
  }

  {
    uint32_t current_n_opening_dbs;
    GRN_ATOMIC_ADD_EX(&grn_n_opening_dbs, -1, current_n_opening_dbs);
  }

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_db_set_cache(grn_ctx *ctx, grn_obj *db, grn_cache *cache)
{
  GRN_API_ENTER;

  if (!ctx) {
    GRN_API_RETURN(GRN_INVALID_ARGUMENT);
  }

  if (!db) {
    ERR(GRN_INVALID_ARGUMENT, "[db][cache][set] DB must not NULL");
    GRN_API_RETURN(ctx->rc);
  }

  if (db->header.type != GRN_DB) {
    ERR(GRN_INVALID_ARGUMENT,
        "[db][cache][set] must be DB: %d",
        db->header.type);
    GRN_API_RETURN(ctx->rc);
  }

  ((grn_db *)db)->cache = cache;

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_cache *
grn_db_get_cache(grn_ctx *ctx, grn_obj *db)
{
  grn_cache *cache;

  GRN_API_ENTER;

  if (!ctx) {
    GRN_API_RETURN(NULL);
  }

  if (!db) {
    ERR(GRN_INVALID_ARGUMENT, "[db][cache][get] DB must not NULL");
    GRN_API_RETURN(NULL);
  }

  if (db->header.type != GRN_DB) {
    ERR(GRN_INVALID_ARGUMENT,
        "[db][cache][get] must be DB: %d",
        db->header.type);
    GRN_API_RETURN(NULL);
  }

  cache = ((grn_db *)db)->cache;

  GRN_API_RETURN(cache);
}

static grn_obj *
grn_alias_resolve(grn_ctx *ctx,
                  const char *name,
                  int name_size,
                  grn_obj *alias_name_buffer,
                  grn_bool get_object)
{
  grn_db *db;
  grn_obj *alias_table = NULL;
  grn_obj *alias_column = NULL;

  db = (grn_db *)(ctx->impl->db);

  if (name_size < 0) {
    name_size = strlen(name);
  }

  while (GRN_TRUE) {
    if (get_object) {
      grn_id id;

      id = grn_table_get(ctx, db->keys, name, name_size);
      if (id) {
        return grn_ctx_at(ctx, id);
      }
    }

    if (!alias_column) {
      grn_id alias_column_id;
      const char *alias_column_name;
      uint32_t alias_column_name_size;

      grn_config_get(ctx,
                     "alias.column",
                     -1,
                     &alias_column_name,
                     &alias_column_name_size);
      if (!alias_column_name) {
        break;
      }
      alias_column_id =
        grn_table_get(ctx, db->keys, alias_column_name, alias_column_name_size);
      if (!alias_column_id) {
        break;
      }
      alias_column = grn_ctx_at(ctx, alias_column_id);
      if (alias_column->header.type != GRN_COLUMN_VAR_SIZE) {
        break;
      }
      if (alias_column->header.flags & GRN_OBJ_VECTOR) {
        break;
      }
      if (DB_OBJ(alias_column)->range != GRN_DB_SHORT_TEXT) {
        break;
      }
      alias_table = grn_ctx_at(ctx, alias_column->header.domain);
      if (alias_table->header.type == GRN_TABLE_NO_KEY) {
        break;
      }
    }

    {
      grn_id alias_id;
      alias_id = grn_table_get(ctx, alias_table, name, name_size);
      if (!alias_id) {
        break;
      }
      GRN_BULK_REWIND(alias_name_buffer);
      grn_obj_get_value(ctx, alias_column, alias_id, alias_name_buffer);
      name = GRN_TEXT_VALUE(alias_name_buffer);
      name_size = GRN_TEXT_LEN(alias_name_buffer);
    }
  }

  return NULL;
}

grn_obj *
grn_ctx_get(grn_ctx *ctx, const char *name, int name_size)
{
  grn_obj *obj = NULL;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    return NULL;
  }
  GRN_API_ENTER;
  if (GRN_DB_P(db)) {
    grn_obj alias_name_buffer;

    GRN_TEXT_INIT(&alias_name_buffer, 0);
    obj = grn_alias_resolve(ctx, name, name_size, &alias_name_buffer, GRN_TRUE);
    GRN_OBJ_FIN(ctx, &alias_name_buffer);
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
grn_obj_get_last_modified(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return 0;
  }

  return grn_obj_get_io(ctx, obj)->header->last_modified;
}

bool
grn_obj_is_dirty(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return false;
  }

  switch (obj->header.type) {
  case GRN_DB:
    return grn_db_is_dirty(ctx, obj);
  case GRN_TABLE_PAT_KEY:
    return grn_pat_is_dirty(ctx, (grn_pat *)obj);
  case GRN_TABLE_DAT_KEY:
    return grn_dat_is_dirty(ctx, (grn_dat *)obj);
  default:
    return false;
  }
}

uint32_t
grn_db_get_last_modified(grn_ctx *ctx, grn_obj *db)
{
  return grn_obj_get_last_modified(ctx, db);
}

bool
grn_db_is_dirty(grn_ctx *ctx, grn_obj *db)
{
  grn_obj *keys;

  if (!db) {
    return false;
  }

  keys = ((grn_db *)db)->keys;
  return grn_obj_is_dirty(ctx, keys);
}

static grn_rc
grn_db_dirty(grn_ctx *ctx, grn_obj *db)
{
  grn_obj *keys;

  if (!db) {
    return GRN_SUCCESS;
  }

  keys = ((grn_db *)db)->keys;
  switch (keys->header.type) {
  case GRN_TABLE_PAT_KEY:
    return grn_pat_dirty(ctx, (grn_pat *)keys);
  case GRN_TABLE_DAT_KEY:
    return grn_dat_dirty(ctx, (grn_dat *)keys);
  default:
    return GRN_SUCCESS;
  }
}

static grn_rc
grn_db_clean(grn_ctx *ctx, grn_obj *db)
{
  grn_obj *keys;

  if (!db) {
    return GRN_SUCCESS;
  }

  keys = ((grn_db *)db)->keys;
  switch (keys->header.type) {
  case GRN_TABLE_PAT_KEY:
    return grn_pat_clean(ctx, (grn_pat *)keys);
  case GRN_TABLE_DAT_KEY:
    return grn_dat_clean(ctx, (grn_dat *)keys);
  default:
    return GRN_SUCCESS;
  }
}

grn_rc
grn_db_clear_dirty(grn_ctx *ctx, grn_obj *db)
{
  grn_obj *keys;

  if (!db) {
    return GRN_SUCCESS;
  }

  keys = ((grn_db *)db)->keys;
  switch (keys->header.type) {
  case GRN_TABLE_PAT_KEY:
    return grn_pat_clear_dirty(ctx, (grn_pat *)keys);
  case GRN_TABLE_DAT_KEY:
    return grn_dat_clear_dirty(ctx, (grn_dat *)keys);
  default:
    return GRN_SUCCESS;
  }
}

void
grn_db_touch(grn_ctx *ctx, grn_obj *s)
{
  grn_obj_touch(ctx, s, NULL);
}

grn_bool
grn_obj_is_corrupt(grn_ctx *ctx, grn_obj *obj)
{
  bool is_corrupt = false;

  GRN_API_ENTER;

  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "[object][corrupt] object must not be NULL");
    GRN_API_RETURN(GRN_FALSE);
  }

  switch (obj->header.type) {
  case GRN_DB:
    is_corrupt = grn_io_is_corrupt(ctx, grn_obj_get_io(ctx, obj));
    if (!is_corrupt) {
      is_corrupt = grn_io_is_corrupt(ctx, ((grn_db *)obj)->specs->io);
    }
    if (!is_corrupt) {
      is_corrupt = grn_io_is_corrupt(ctx, ((grn_db *)obj)->config->io);
    }
    if (!is_corrupt) {
      is_corrupt = grn_options_is_corrupt(ctx, ((grn_db *)obj)->options);
    }
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
    is_corrupt = grn_io_is_corrupt(ctx, grn_obj_get_io(ctx, obj));
    break;
  case GRN_TABLE_DAT_KEY:
    is_corrupt = grn_dat_is_corrupt(ctx, (grn_dat *)obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
    is_corrupt = grn_io_is_corrupt(ctx, grn_obj_get_io(ctx, obj));
    break;
  case GRN_COLUMN_INDEX:
    is_corrupt = grn_io_is_corrupt(ctx, ((grn_ii *)obj)->seg);
    if (!is_corrupt) {
      is_corrupt = grn_io_is_corrupt(ctx, ((grn_ii *)obj)->chunk);
    }
    break;
  default:
    break;
  }

  GRN_API_RETURN(is_corrupt);
}

#define IS_TEMP(obj) (DB_OBJ(obj)->id & GRN_OBJ_TMP_OBJECT)

static grn_inline void
grn_obj_touch_db(grn_ctx *ctx, grn_obj *obj, grn_timeval *tv)
{
  grn_obj_get_io(ctx, obj)->header->last_modified = (uint32_t)(tv->tv_sec);
  grn_db_dirty(ctx, obj);
}

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
    case GRN_DB:
      grn_obj_touch_db(ctx, obj, tv);
      break;
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
    case GRN_COLUMN_VAR_SIZE:
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_INDEX:
      if (!IS_TEMP(obj)) {
        grn_obj_get_io(ctx, obj)->header->last_modified =
          (uint32_t)(tv->tv_sec);
        grn_obj_touch(ctx, DB_OBJ(obj)->db, tv);
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
  if (name_size > 0 && *name == GRN_DB_PSEUDO_COLUMN_PREFIX) {
    return GRN_INVALID_ARGUMENT;
  }
  while (name < name_end) {
    char c = *name;
    if ((unsigned int)((c | 0x20) - 'a') >= 26u &&
        (unsigned int)(c - '0') >= 10u && c != '_' && c != '-' && c != '#' &&
        c != '@') {
      return GRN_INVALID_ARGUMENT;
    }
    if (!(len = grn_charlen(ctx, name, name_end))) {
      break;
    }
    name += len;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_db_set_option_values(grn_ctx *ctx,
                         grn_obj *db,
                         grn_id id,
                         const char *name,
                         int name_length,
                         grn_obj *values)
{
  grn_db *db_ = (grn_db *)db;
  return grn_options_set(ctx, db_->options, id, name, name_length, values);
}

grn_option_revision
grn_db_get_option_values(grn_ctx *ctx,
                         grn_obj *db,
                         grn_id id,
                         const char *name,
                         int name_length,
                         grn_option_revision revision,
                         grn_obj *values)
{
  grn_db *db_ = (grn_db *)db;
  return grn_options_get(ctx,
                         db_->options,
                         id,
                         name,
                         name_length,
                         revision,
                         values);
}

grn_rc
grn_db_clear_option_values(grn_ctx *ctx, grn_obj *db, grn_id id)
{
  grn_db *db_ = (grn_db *)db;
  return grn_options_clear(ctx, db_->options, id);
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
grn_proc_create(grn_ctx *ctx,
                const char *name,
                int name_size,
                grn_proc_type type,
                grn_proc_func *init,
                grn_proc_func *next,
                grn_proc_func *fin,
                unsigned int nvars,
                grn_expr_var *vars)
{
  grn_proc *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id range = GRN_ID_NIL;
  int added = 0;
  grn_obj *db;
  const char *path;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  path = ctx->impl->plugin_path;
  if (path) {
    range = grn_plugin_reference(ctx, path);
  }
  if (name_size < 0) {
    name_size = strlen(name);
  }
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR("[proc][create]", name, name_size);
    GRN_API_RETURN(NULL);
  }
  if (!GRN_DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
    GRN_API_RETURN(NULL);
  }
  if (name && name_size) {
    grn_db *s = (grn_db *)db;
    if (!(id = grn_table_get(ctx, s->keys, name, name_size))) {
      if (!(id = grn_table_add(ctx, s->keys, name, name_size, &added))) {
        ERR(GRN_NO_MEMORY_AVAILABLE, "grn_table_add failed");
        GRN_API_RETURN(NULL);
      }
    }
    if (!added) {
      db_value *vp;
      if ((vp = grn_tiny_array_at(&s->values, id)) &&
          (res = (grn_proc *)vp->ptr)) {
        /* TODO: Do more robust check. */
        if (res->funcs[PROC_INIT] || res->funcs[PROC_NEXT] ||
            res->funcs[PROC_FIN]) {
          ERR(GRN_INVALID_ARGUMENT,
              "[proc][create] already used name: <%.*s>",
              name_size,
              name);
          GRN_API_RETURN(NULL);
        }
        if (range != GRN_ID_NIL) {
          grn_plugin_close(ctx, range);
        }
        GRN_API_RETURN((grn_obj *)res);
      } else {
        added = 1;
      }
    }
  } else if (ctx->impl && ctx->impl->values) {
    id = grn_array_add(ctx, ctx->impl->values, NULL) | GRN_OBJ_TMP_OBJECT;
    added = 1;
  }
  if (!res) {
    res = GRN_MALLOCN(grn_proc, 1);
  }
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
    memset(&(res->callbacks), 0, sizeof(res->callbacks));
    if (type == GRN_PROC_FUNCTION) {
      res->callbacks.function.selector_op = GRN_OP_NOP;
      res->callbacks.function.is_stable = GRN_TRUE;
    }
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
      grn_obj *v =
        grn_expr_add_var(ctx, (grn_obj *)res, vars->name, vars->name_size);
      if (!v) {
        grn_obj_close(ctx, (grn_obj *)res);
        GRN_API_RETURN(NULL);
      }
      GRN_OBJ_INIT(v, vars->value.header.type, 0, vars->value.header.domain);
      GRN_TEXT_PUT(ctx,
                   v,
                   GRN_TEXT_VALUE(&vars->value),
                   GRN_TEXT_LEN(&vars->value));
      vars++;
    }
  }
  GRN_API_RETURN((grn_obj *)res);
}

/* grn_table */

static void
calc_rec_size(grn_table_flags flags,
              uint32_t max_n_subrecs,
              uint32_t range_size,
              uint32_t additional_value_size,
              uint8_t *subrec_size,
              uint8_t *subrec_offset,
              uint32_t *key_size,
              uint32_t *value_size)
{
  *subrec_size = 0;
  *subrec_offset = 0;
  if (flags & GRN_OBJ_WITH_SUBREC) {
    switch (flags & GRN_OBJ_UNIT_MASK) {
    case GRN_OBJ_UNIT_DOCUMENT_NONE:
      break;
    case GRN_OBJ_UNIT_DOCUMENT_SECTION:
      *subrec_offset = sizeof(grn_id);
      *subrec_size = sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_DOCUMENT_POSITION:
      *subrec_offset = sizeof(grn_id);
      *subrec_size = sizeof(uint32_t) + sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_SECTION_NONE:
      *key_size += sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_SECTION_POSITION:
      *key_size += sizeof(uint32_t);
      *subrec_offset = sizeof(grn_id) + sizeof(uint32_t);
      *subrec_size = sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_POSITION_NONE:
      *key_size += sizeof(uint32_t) + sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_USERDEF_DOCUMENT:
      *subrec_size = range_size;
      break;
    case GRN_OBJ_UNIT_USERDEF_SECTION:
      *subrec_size = range_size + sizeof(uint32_t);
      break;
    case GRN_OBJ_UNIT_USERDEF_POSITION:
      *subrec_size = range_size + sizeof(uint32_t) + sizeof(uint32_t);
      break;
    }
    *value_size =
      (uintptr_t)GRN_RSET_SUBRECS_NTH((((grn_rset_recinfo *)0)->subrecs),
                                      *subrec_size,
                                      max_n_subrecs);
  } else {
    *value_size = range_size;
  }
  *value_size += additional_value_size;
}

static grn_rc
grn_obj_remove_internal(grn_ctx *ctx, grn_obj *obj, uint32_t flags);

static grn_rc
grn_table_create_validate(grn_ctx *ctx,
                          const char *name,
                          unsigned int name_size,
                          const char *path,
                          grn_table_flags flags,
                          grn_obj *key_type,
                          grn_obj *value_type)
{
  grn_table_flags table_type;
  const char *table_type_name = NULL;

  table_type = (flags & GRN_OBJ_TABLE_TYPE_MASK);
  switch (table_type) {
  case GRN_OBJ_TABLE_HASH_KEY:
    table_type_name = "TABLE_HASH_KEY";
    break;
  case GRN_OBJ_TABLE_PAT_KEY:
    table_type_name = "TABLE_PAT_KEY";
    break;
  case GRN_OBJ_TABLE_DAT_KEY:
    table_type_name = "TABLE_DAT_KEY";
    break;
  case GRN_OBJ_TABLE_NO_KEY:
    table_type_name = "TABLE_NO_KEY";
    break;
  default:
    table_type_name = "unknown";
    break;
  }

  if (!key_type && table_type != GRN_OBJ_TABLE_NO_KEY &&
      !(flags & GRN_OBJ_KEY_VAR_SIZE)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][create] "
        "key type is required for TABLE_HASH_KEY, TABLE_PAT_KEY or "
        "TABLE_DAT_KEY: <%.*s>",
        name_size,
        name);
    return ctx->rc;
  }

  if (key_type && table_type == GRN_OBJ_TABLE_NO_KEY) {
    int key_name_size;
    char key_name[GRN_TABLE_MAX_KEY_SIZE];
    key_name_size =
      grn_obj_name(ctx, key_type, key_name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][create] "
        "key isn't available for TABLE_NO_KEY table: <%.*s> (%.*s)",
        name_size,
        name,
        key_name_size,
        key_name);
    return ctx->rc;
  }

  if ((flags & GRN_OBJ_KEY_WITH_SIS) && table_type != GRN_OBJ_TABLE_PAT_KEY) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][create] "
        "key with SIS is available only for TABLE_PAT_KEY table: "
        "<%.*s>(%s)",
        name_size,
        name,
        table_type_name);
    return ctx->rc;
  }

  if ((flags & GRN_OBJ_KEY_NORMALIZE) && table_type == GRN_OBJ_TABLE_NO_KEY) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][create] "
        "key normalization isn't available for TABLE_NO_KEY table: <%.*s>",
        name_size,
        name);
    return ctx->rc;
  }

  if ((flags & GRN_OBJ_KEY_LARGE) && table_type != GRN_OBJ_TABLE_HASH_KEY) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][create] "
        "large key support is available only for TABLE_HASH_KEY key table: "
        "<%.*s>(%s)",
        name_size,
        name,
        table_type_name);
    return ctx->rc;
  }

  return ctx->rc;
}

grn_obj *
grn_table_create_with_max_n_subrecs(grn_ctx *ctx,
                                    const char *name,
                                    unsigned int name_size,
                                    const char *path,
                                    grn_table_flags flags,
                                    grn_obj *key_type,
                                    grn_obj *value_type,
                                    uint32_t max_n_subrecs,
                                    uint32_t additional_value_size)
{
  grn_id id;
  grn_id domain = GRN_ID_NIL, range = GRN_ID_NIL;
  uint32_t key_size, value_size = 0, range_size = 0;
  uint8_t subrec_size, subrec_offset;
  grn_obj *res = NULL;
  grn_obj *db;
  char buffer[PATH_MAX];
  if (!ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[table][create] db not initialized");
    return NULL;
  }
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR("[table][create]", name, name_size);
    return NULL;
  }
  if (!GRN_DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "[table][create] invalid db assigned");
    return NULL;
  }
  if (grn_table_create_validate(ctx,
                                name,
                                name_size,
                                path,
                                flags,
                                key_type,
                                value_type)) {
    return NULL;
  }
  if (key_type) {
    domain = DB_OBJ(key_type)->id;
    switch (key_type->header.type) {
    case GRN_TYPE:
      {
        grn_db_obj *t = (grn_db_obj *)key_type;
        flags |= t->header.flags;
        key_size = GRN_TYPE_SIZE(t);
        if (key_size > GRN_TABLE_MAX_KEY_SIZE) {
          int type_name_size;
          char type_name[GRN_TABLE_MAX_KEY_SIZE];
          type_name_size =
            grn_obj_name(ctx, key_type, type_name, GRN_TABLE_MAX_KEY_SIZE);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][create] key size too big: <%.*s> <%.*s>(%u) (max:%u)",
              name_size,
              name,
              type_name_size,
              type_name,
              key_size,
              GRN_TABLE_MAX_KEY_SIZE);
          return NULL;
        }
      }
      break;
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
      key_size = sizeof(grn_id);
      break;
    default:
      {
        int key_name_size;
        char key_name[GRN_TABLE_MAX_KEY_SIZE];
        key_name_size =
          grn_obj_name(ctx, key_type, key_name, GRN_TABLE_MAX_KEY_SIZE);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][create] key type must be type or table: <%.*s> (%.*s)",
            name_size,
            name,
            key_name_size,
            key_name);
        return NULL;
      }
      break;
    }
  } else {
    key_size =
      (flags & GRN_OBJ_KEY_VAR_SIZE) ? GRN_TABLE_MAX_KEY_SIZE : sizeof(grn_id);
  }
  if (value_type) {
    range = DB_OBJ(value_type)->id;
    switch (value_type->header.type) {
    case GRN_TYPE:
      {
        grn_db_obj *t = (grn_db_obj *)value_type;
        if (t->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
          int type_name_size;
          char type_name[GRN_TABLE_MAX_KEY_SIZE];
          type_name_size =
            grn_obj_name(ctx, value_type, type_name, GRN_TABLE_MAX_KEY_SIZE);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][create] value type must be fixed size: <%.*s> (%.*s)",
              name_size,
              name,
              type_name_size,
              type_name);
          return NULL;
        }
        range_size = GRN_TYPE_SIZE(t);
      }
      break;
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
      range_size = sizeof(grn_id);
      break;
    default:
      {
        int value_name_size;
        char value_name[GRN_TABLE_MAX_KEY_SIZE];
        value_name_size =
          grn_obj_name(ctx, value_type, value_name, GRN_TABLE_MAX_KEY_SIZE);
        ERR(GRN_INVALID_ARGUMENT,
            "[table][create] value type must be type or table: <%.*s> (%.*s)",
            name_size,
            name,
            value_name_size,
            value_name);
        return NULL;
      }
      break;
    }
  }

  id = grn_obj_register(ctx, db, name, name_size);
  if (ERRP(ctx, GRN_ERROR)) {
    return NULL;
  }
  if (GRN_OBJ_PERSISTENT & flags) {
    GRN_LOG(ctx,
            GRN_LOG_NOTICE,
            "DDL:%u:table_create %.*s",
            id,
            name_size,
            name);
    if (!path) {
      if (GRN_DB_PERSISTENT_P(db)) {
        grn_db_generate_pathname(ctx, db, id, buffer);
        path = buffer;
      } else {
        ERR(GRN_INVALID_ARGUMENT, "path not assigned for persistent table");
        grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
        return NULL;
      }
    } else {
      flags |= GRN_OBJ_CUSTOM_NAME;
    }
  } else {
    if (path) {
      ERR(GRN_INVALID_ARGUMENT, "path assigned for temporary table");
      grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
      return NULL;
    }
    if (GRN_DB_PERSISTENT_P(db) && name && name_size) {
      ERR(GRN_INVALID_ARGUMENT, "name assigned for temporary table");
      grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
      return NULL;
    }
  }
  calc_rec_size(flags,
                max_n_subrecs,
                range_size,
                additional_value_size,
                &subrec_size,
                &subrec_offset,
                &key_size,
                &value_size);
  switch (flags & GRN_OBJ_TABLE_TYPE_MASK) {
  case GRN_OBJ_TABLE_HASH_KEY:
    res = (grn_obj *)grn_hash_create(ctx, path, key_size, value_size, flags);
    break;
  case GRN_OBJ_TABLE_PAT_KEY:
    res = (grn_obj *)grn_pat_create(ctx, path, key_size, value_size, flags);
    break;
  case GRN_OBJ_TABLE_DAT_KEY:
    res = (grn_obj *)grn_dat_create(ctx, path, key_size, value_size, flags);
    break;
  case GRN_OBJ_TABLE_NO_KEY:
    domain = range;
    res = (grn_obj *)grn_array_create(ctx, path, value_size, flags);
    break;
  }
  if (res) {
    DB_OBJ(res)->header.impl_flags = 0;
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->range = range;
    DB_OBJ(res)->max_n_subrecs = max_n_subrecs;
    DB_OBJ(res)->subrec_size = subrec_size;
    DB_OBJ(res)->subrec_offset = subrec_offset;
    DB_OBJ(res)->group.flags = 0;
    DB_OBJ(res)->group.calc_target = NULL;
    DB_OBJ(res)->group.aggregated_value_type_id = GRN_DB_INT64;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res)) == GRN_SUCCESS) {
      if (grn_obj_is_persistent(ctx, res)) {
        grn_obj *space;
        space = ctx->impl->temporary_open_spaces.current;
        if (space) {
          GRN_PTR_PUT(ctx, space, res);
        }
      }
    } else {
      grn_obj_remove_internal(ctx, res, 0);
      res = NULL;
    }
  } else {
    grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
  }
  return res;
}

grn_obj *
grn_table_create(grn_ctx *ctx,
                 const char *name,
                 unsigned int name_size,
                 const char *path,
                 grn_table_flags flags,
                 grn_obj *key_type,
                 grn_obj *value_type)
{
  grn_obj *res;
  GRN_API_ENTER;
  res = grn_table_create_with_max_n_subrecs(ctx,
                                            name,
                                            name_size,
                                            path,
                                            flags,
                                            key_type,
                                            value_type,
                                            0,
                                            0);
  GRN_API_RETURN(res);
}

grn_obj *
grn_table_create_for_group(grn_ctx *ctx,
                           const char *name,
                           unsigned int name_size,
                           const char *path,
                           grn_obj *group_key,
                           grn_obj *value_type,
                           uint32_t max_n_subrecs)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  if (group_key) {
    grn_obj *key_type;
    key_type = grn_ctx_at(ctx, grn_obj_get_range(ctx, group_key));
    if (key_type) {
      res = grn_table_create_with_max_n_subrecs(ctx,
                                                name,
                                                name_size,
                                                path,
                                                GRN_OBJ_TABLE_HASH_KEY |
                                                  GRN_OBJ_WITH_SUBREC |
                                                  GRN_OBJ_UNIT_USERDEF_DOCUMENT,
                                                key_type,
                                                value_type,
                                                max_n_subrecs,
                                                0);
      grn_obj_unlink(ctx, key_type);
    }
  } else {
    res = grn_table_create_with_max_n_subrecs(
      ctx,
      name,
      name_size,
      path,
      GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_VAR_SIZE | GRN_OBJ_WITH_SUBREC |
        GRN_OBJ_UNIT_USERDEF_DOCUMENT,
      NULL,
      value_type,
      max_n_subrecs,
      0);
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_table_create_similar(grn_ctx *ctx,
                         const char *name,
                         uint32_t name_size,
                         const char *path,
                         grn_obj *base_table)
{
  GRN_API_ENTER;
  grn_obj *table = grn_table_create_similar_id_map(ctx,
                                                   name,
                                                   name_size,
                                                   path,
                                                   base_table,
                                                   NULL);
  GRN_API_RETURN(table);
}

unsigned int
grn_table_get_subrecs(grn_ctx *ctx,
                      grn_obj *table,
                      grn_id id,
                      grn_id *subrecbuf,
                      int *scorebuf,
                      int buf_size)
{
  unsigned int count = 0;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table)) {
    uint32_t value_size;
    grn_rset_recinfo *ri;
    uint32_t subrec_size = DB_OBJ(table)->subrec_size;
    uint32_t max_n_subrecs = DB_OBJ(table)->max_n_subrecs;
    if (subrec_size < sizeof(grn_id)) {
      goto exit;
    }
    if (!max_n_subrecs) {
      goto exit;
    }
    ri = (grn_rset_recinfo *)grn_obj_get_value_(ctx, table, id, &value_size);
    if (ri) {
      byte *psubrec = (byte *)ri->subrecs;
      uint32_t n_subrecs = (uint32_t)GRN_RSET_N_SUBRECS(ri);
      uint32_t limit = value_size / (GRN_RSET_SCORE_SIZE + subrec_size);
      if ((int)limit > buf_size) {
        limit = buf_size;
      }
      if (limit > n_subrecs) {
        limit = n_subrecs;
      }
      if (limit > max_n_subrecs) {
        limit = max_n_subrecs;
      }
      for (; count < limit; count++) {
        if (scorebuf) {
          scorebuf[count] = *((double *)psubrec);
        }
        psubrec += GRN_RSET_SCORE_SIZE;
        if (subrecbuf) {
          subrecbuf[count] = *((grn_id *)psubrec);
        }
        psubrec += subrec_size;
      }
    }
  }
exit:
  GRN_API_RETURN(count);
}

grn_obj *
grn_table_open(grn_ctx *ctx,
               const char *name,
               unsigned int name_size,
               const char *path)
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
      if (!type) {
        GRN_API_RETURN(NULL);
      }
      switch (type) {
      case GRN_TABLE_HASH_KEY:
        res = (grn_obj *)grn_hash_open(ctx, path);
        break;
      case GRN_TABLE_PAT_KEY:
        res = (grn_obj *)grn_pat_open(ctx, path);
        break;
      case GRN_TABLE_DAT_KEY:
        res = (grn_obj *)grn_dat_open(ctx, path);
        break;
      case GRN_TABLE_NO_KEY:
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
grn_table_lcp_search(grn_ctx *ctx,
                     grn_obj *table,
                     const void *key,
                     unsigned int key_size)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  switch (table->header.type) {
  case GRN_TABLE_PAT_KEY:
    {
      grn_pat *pat = (grn_pat *)table;
      WITH_NORMALIZE(pat, key, key_size, {
        id = grn_pat_lcp_search(ctx, pat, key, key_size);
      });
    }
    break;
  case GRN_TABLE_DAT_KEY:
    {
      grn_dat *dat = (grn_dat *)table;
      WITH_NORMALIZE(dat, key, key_size, {
        id = grn_dat_lcp_search(ctx, dat, key, key_size);
      });
    }
    break;
  case GRN_TABLE_HASH_KEY:
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

grn_obj *
grn_obj_default_set_value_hook(grn_ctx *ctx,
                               int nargs,
                               grn_obj **args,
                               grn_user_data *user_data)
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
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(h);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    int section = data->section;
    if (flags) { /* todo */
    }
    if (target) {
      switch (target->header.type) {
      case GRN_COLUMN_VAR_SIZE:
        grn_token_column_update(ctx,
                                target,
                                GRN_UINT32_VALUE(id),
                                section,
                                oldvalue,
                                newvalue);
        break;
      case GRN_COLUMN_INDEX:
        grn_ii_column_update(ctx,
                             (grn_ii *)target,
                             GRN_UINT32_VALUE(id),
                             section,
                             oldvalue,
                             newvalue,
                             NULL);
        break;
      default:
        break;
      }
      grn_obj_unref(ctx, target);
    }
  }
  return NULL;
}

static grn_id
grn_table_add_internal(grn_ctx *ctx,
                       grn_obj *table,
                       const void *key,
                       unsigned int key_size,
                       grn_table_add_options *options)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    grn_table_add_options local_options = {0};
    if (!options) {
      options = &local_options;
    }
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      {
        grn_pat *pat = (grn_pat *)table;
        WITH_NORMALIZE(pat, key, key_size, {
          if (key_size == 0 && options->ignore_empty_normalized_key) {
            options->ignored = true;
          } else {
            GRN_TABLE_LOCK_BEGIN(ctx, table)
            {
              int added = 0;
              id = grn_pat_add(ctx, pat, key, key_size, NULL, &added);
              options->added = added;
            }
            GRN_TABLE_LOCK_END(ctx);
          }
        });
        if (id == GRN_ID_NIL && !options->ignored) {
          grn_obj buffer;
          GRN_TEXT_INIT(&buffer, 0);
          grn_inspect_key(ctx, &buffer, table, key, key_size);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][add][pat] failed to add: %.*s",
              (int)GRN_TEXT_LEN(&buffer),
              GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
        }
      }
      break;
    case GRN_TABLE_DAT_KEY:
      {
        grn_dat *dat = (grn_dat *)table;
        WITH_NORMALIZE(dat, key, key_size, {
          if (key_size == 0 && options->ignore_empty_normalized_key) {
            options->ignored = true;
          } else {
            GRN_TABLE_LOCK_BEGIN(ctx, table)
            {
              int added = 0;
              id = grn_dat_add(ctx, dat, key, key_size, NULL, &added);
              options->added = added;
            }
            GRN_TABLE_LOCK_END(ctx);
          }
        });
        if (id == GRN_ID_NIL && !options->ignored) {
          grn_obj buffer;
          GRN_TEXT_INIT(&buffer, 0);
          grn_inspect_key(ctx, &buffer, table, key, key_size);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][add][dat] failed to add: %.*s",
              (int)GRN_TEXT_LEN(&buffer),
              GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
        }
      }
      break;
    case GRN_TABLE_HASH_KEY:
      {
        grn_hash *hash = (grn_hash *)table;
        WITH_NORMALIZE(hash, key, key_size, {
          if (key_size == 0 && options->ignore_empty_normalized_key) {
            options->ignored = true;
          } else {
            GRN_TABLE_LOCK_BEGIN(ctx, table)
            {
              int added;
              id = grn_hash_add(ctx, hash, key, key_size, NULL, &added);
              options->added = added;
            }
            GRN_TABLE_LOCK_END(ctx);
          }
        });
        if (id == GRN_ID_NIL && !options->ignored) {
          grn_obj buffer;
          GRN_TEXT_INIT(&buffer, 0);
          grn_inspect_key(ctx, &buffer, table, key, key_size);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][add][hash] failed to add: %.*s",
              (int)GRN_TEXT_LEN(&buffer),
              GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
        }
      }
      break;
    case GRN_TABLE_NO_KEY:
      {
        grn_array *array = (grn_array *)table;
        GRN_TABLE_LOCK_BEGIN(ctx, table)
        {
          id = grn_array_add(ctx, array, NULL);
        }
        GRN_TABLE_LOCK_END(ctx);
        options->added = (id != GRN_ID_NIL);
        if (id == GRN_ID_NIL) {
          grn_obj buffer;
          GRN_TEXT_INIT(&buffer, 0);
          grn_inspect_key(ctx, &buffer, table, key, key_size);
          ERR(GRN_INVALID_ARGUMENT,
              "[table][add][array] failed to add: %.*s",
              (int)GRN_TEXT_LEN(&buffer),
              GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
        }
      }
      break;
    }
    if (options->added) {
      grn_hook *hooks = DB_OBJ(table)->hooks[GRN_HOOK_INSERT];
      if (hooks) {
        // todo : grn_proc_ctx_open()
        grn_obj id_, flags_, oldvalue_, value_;
        grn_proc_ctx pctx;
        grn_proc_ctx_init(&pctx, hooks, 4, 4);
        GRN_UINT32_INIT(&id_, 0);
        GRN_UINT32_INIT(&flags_, 0);
        GRN_VOID_INIT(&oldvalue_);
        grn_obj_reinit_for(ctx, &oldvalue_, table);
        GRN_OBJ_INIT(&value_,
                     GRN_BULK,
                     GRN_OBJ_DO_SHALLOW_COPY,
                     table->header.domain);
        GRN_TEXT_SET_REF(&value_, key, key_size);
        GRN_UINT32_SET(ctx, &id_, id);
        GRN_UINT32_SET(ctx, &flags_, GRN_OBJ_SET);
        while (hooks) {
          grn_ctx_push(ctx, &id_);
          grn_ctx_push(ctx, &oldvalue_);
          grn_ctx_push(ctx, &value_);
          grn_ctx_push(ctx, &flags_);
          pctx.caller = NULL;
          pctx.currh = hooks;
          if (hooks->proc) {
            hooks->proc->funcs[PROC_INIT](ctx, 1, &table, &pctx.user_data);
          } else {
            grn_obj_default_set_value_hook(ctx, 1, &table, &pctx.user_data);
          }
          if (ctx->rc) {
            break;
          }
          hooks = hooks->next;
          pctx.offset++;
        }
        GRN_OBJ_FIN(ctx, &id_);
        GRN_OBJ_FIN(ctx, &flags_);
        GRN_OBJ_FIN(ctx, &oldvalue_);
        GRN_OBJ_FIN(ctx, &value_);
      }
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][add] failed to add: table must not NULL");
  }
  GRN_API_RETURN(id);
}

grn_id
grn_table_add(grn_ctx *ctx,
              grn_obj *table,
              const void *key,
              unsigned int key_size,
              int *added)
{
  grn_table_add_options options = {0};
  grn_id id = grn_table_add_internal(ctx, table, key, key_size, &options);
  if (added) {
    *added = options.added ? 1 : 0;
  }
  return id;
}

grn_id
grn_table_get_by_key(grn_ctx *ctx, grn_obj *table, grn_obj *key)
{
  grn_id id = GRN_ID_NIL;
  if (grn_type_id_is_compatible(ctx,
                                table->header.domain,
                                key->header.domain)) {
    id = grn_table_get(ctx, table, GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key));
  } else {
    grn_rc rc;
    grn_obj buf;
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, table->header.domain);
    if ((rc = grn_obj_cast(ctx, key, &buf, GRN_TRUE))) {
      grn_obj *domain = grn_ctx_at(ctx, table->header.domain);
      ERR_CAST(table, domain, key);
    } else {
      id = grn_table_get(ctx, table, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf));
    }
    GRN_OBJ_FIN(ctx, &buf);
  }
  return id;
}

grn_id
grn_table_add_by_key(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *key,
                     grn_table_add_options *options)
{
  grn_id id = GRN_ID_NIL;
  if (grn_type_id_is_compatible(ctx,
                                table->header.domain,
                                key->header.domain)) {
    id = grn_table_add_internal(ctx,
                                table,
                                GRN_TEXT_VALUE(key),
                                GRN_TEXT_LEN(key),
                                options);
  } else {
    grn_rc rc;
    grn_obj buf;
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, table->header.domain);
    if ((rc = grn_obj_cast(ctx, key, &buf, GRN_TRUE))) {
      grn_obj *domain = grn_ctx_at(ctx, table->header.domain);
      ERR_CAST(table, domain, key);
    } else {
      id = grn_table_add_internal(ctx,
                                  table,
                                  GRN_TEXT_VALUE(&buf),
                                  GRN_TEXT_LEN(&buf),
                                  options);
    }
    GRN_OBJ_FIN(ctx, &buf);
  }
  return id;
}

grn_id
grn_table_get(grn_ctx *ctx,
              grn_obj *table,
              const void *key,
              unsigned int key_size)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    if (table->header.type == GRN_DB) {
      grn_db *db = (grn_db *)table;
      table = db->keys;
    }
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_get(ctx, (grn_pat *)table, key, key_size, NULL);
      });
      break;
    case GRN_TABLE_DAT_KEY:
      WITH_NORMALIZE((grn_dat *)table, key, key_size, {
        id = grn_dat_get(ctx, (grn_dat *)table, key, key_size, NULL);
      });
      break;
    case GRN_TABLE_HASH_KEY:
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
    case GRN_DB:
      {
        grn_db *db = (grn_db *)table;
        id = grn_table_at(ctx, db->keys, id);
      }
      break;
    case GRN_TABLE_PAT_KEY:
      id = grn_pat_at(ctx, (grn_pat *)table, id);
      break;
    case GRN_TABLE_DAT_KEY:
      id = grn_dat_at(ctx, (grn_dat *)table, id);
      break;
    case GRN_TABLE_HASH_KEY:
      id = grn_hash_at(ctx, (grn_hash *)table, id);
      break;
    case GRN_TABLE_NO_KEY:
      id = grn_array_at(ctx, (grn_array *)table, id);
      break;
    default:
      id = GRN_ID_NIL;
    }
  }
  GRN_API_RETURN(id);
}

grn_inline static grn_id
grn_table_add_v_inline(grn_ctx *ctx,
                       grn_obj *table,
                       const void *key,
                       int key_size,
                       void **value,
                       int *added)
{
  grn_id id = GRN_ID_NIL;
  if (!key || !key_size) {
    return GRN_ID_NIL;
  }
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_add(ctx, (grn_pat *)table, key, key_size, value, added);
      });
      break;
    case GRN_TABLE_DAT_KEY:
      WITH_NORMALIZE((grn_dat *)table, key, key_size, {
        id = grn_dat_add(ctx, (grn_dat *)table, key, key_size, value, added);
      });
      break;
    case GRN_TABLE_HASH_KEY:
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_add(ctx, (grn_hash *)table, key, key_size, value, added);
      });
      break;
    case GRN_TABLE_NO_KEY:
      id = grn_array_add(ctx, (grn_array *)table, value);
      if (added) {
        *added = id ? 1 : 0;
      }
      break;
    }
  }
  return id;
}

grn_id
grn_table_add_v(grn_ctx *ctx,
                grn_obj *table,
                const void *key,
                int key_size,
                void **value,
                int *added)
{
  grn_id id;
  GRN_API_ENTER;
  id = grn_table_add_v_inline(ctx, table, key, key_size, value, added);
  GRN_API_RETURN(id);
}

grn_id
grn_table_get_v(
  grn_ctx *ctx, grn_obj *table, const void *key, int key_size, void **value)
{
  grn_id id = GRN_ID_NIL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      WITH_NORMALIZE((grn_pat *)table, key, key_size, {
        id = grn_pat_get(ctx, (grn_pat *)table, key, key_size, value);
      });
      break;
    case GRN_TABLE_DAT_KEY:
      WITH_NORMALIZE((grn_dat *)table, key, key_size, {
        id = grn_dat_get(ctx, (grn_dat *)table, key, key_size, value);
      });
      break;
    case GRN_TABLE_HASH_KEY:
      WITH_NORMALIZE((grn_hash *)table, key, key_size, {
        id = grn_hash_get(ctx, (grn_hash *)table, key, key_size, value);
      });
      break;
    }
  }
  GRN_API_RETURN(id);
}

int
grn_table_get_key(
  grn_ctx *ctx, grn_obj *table, grn_id id, void *keybuf, int buf_size)
{
  int r = 0;
  GRN_API_ENTER;
  if (table) {
    if (table->header.type == GRN_DB) {
      table = ((grn_db *)table)->keys;
    }
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY:
      r = grn_hash_get_key(ctx, (grn_hash *)table, id, keybuf, buf_size);
      break;
    case GRN_TABLE_PAT_KEY:
      r = grn_pat_get_key(ctx, (grn_pat *)table, id, keybuf, buf_size);
      break;
    case GRN_TABLE_DAT_KEY:
      r = grn_dat_get_key(ctx, (grn_dat *)table, id, keybuf, buf_size);
      break;
    case GRN_TABLE_NO_KEY:
      {
        grn_array *a = (grn_array *)table;
        if (a->obj.header.domain) {
          if (buf_size >= (int)(a->value_size)) {
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
    if (table->header.type == GRN_DB) {
      table = ((grn_db *)table)->keys;
    }
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY:
      r = grn_hash_get_key2(ctx, (grn_hash *)table, id, bulk);
      break;
    case GRN_TABLE_PAT_KEY:
      r = grn_pat_get_key2(ctx, (grn_pat *)table, id, bulk);
      break;
    case GRN_TABLE_DAT_KEY:
      r = grn_dat_get_key2(ctx, (grn_dat *)table, id, bulk);
      break;
    case GRN_TABLE_NO_KEY:
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
    grn_id range = DB_OBJ(obj)->range;
    switch (obj->header.type) {
    case GRN_COLUMN_VAR_SIZE:
      if (!grn_obj_is_empty(ctx, obj, id)) {
        grn_obj buf;
        GRN_VOID_INIT(&buf);
        rc = grn_obj_set_value(ctx, obj, id, &buf, GRN_OBJ_SET);
        GRN_OBJ_FIN(ctx, &buf);
      }
      break;
    case GRN_COLUMN_FIX_SIZE:
      {
        grn_obj buf;
        GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
        rc = grn_obj_set_value(ctx, obj, id, &buf, GRN_OBJ_SET);
        GRN_OBJ_FIN(ctx, &buf);
      }
      break;
    default:
      break;
    }
  }
  return rc;
}

grn_rc
grn_table_delete_data_init(grn_ctx *ctx,
                           grn_table_delete_data *data,
                           grn_obj *table)
{
  data->table = table;
  data->id = GRN_ID_NIL;
  data->key = NULL;
  data->key_size = 0;
  data->optarg = NULL;
  GRN_VOID_INIT(&(data->columns));
  GRN_VOID_INIT(&(data->index_columns));
  return GRN_SUCCESS;
}

grn_rc
grn_table_delete_data_fin(grn_ctx *ctx, grn_table_delete_data *data)
{
  grn_obj *columns = &(data->columns);
  if (columns->header.type != GRN_DB_VOID) {
    size_t n = GRN_PTR_VECTOR_SIZE(columns);
    size_t i;
    for (i = 0; i < n; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(columns, i);
      grn_obj_unref(ctx, column);
    }
  }
  GRN_OBJ_FIN(ctx, columns);

  GRN_OBJ_FIN(ctx, &(data->index_columns));

  return GRN_SUCCESS;
}

static inline grn_obj *
grn_table_delete_data_get_columns(grn_ctx *ctx, grn_table_delete_data *data)
{
  grn_obj *columns = &(data->columns);
  if (columns->header.type == GRN_DB_VOID) {
    GRN_PTR_INIT(columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_hash *all_columns = grn_table_all_columns(ctx, data->table);
    if (all_columns) {
      GRN_HASH_EACH_BEGIN(ctx, all_columns, cursor, column_entry_id)
      {
        void *key;
        grn_hash_cursor_get_key(ctx, cursor, &key);
        grn_id column_id = *((grn_id *)key);
        grn_obj *column = grn_ctx_at(ctx, column_id);
        if (column) {
          GRN_PTR_PUT(ctx, columns, column);
        }
      }
      GRN_HASH_EACH_END(ctx, cursor);
    }
  }
  return columns;
}

static inline grn_obj *
grn_table_delete_data_get_index_columns(grn_ctx *ctx,
                                        grn_table_delete_data *data)
{
  grn_obj *index_columns = &(data->index_columns);
  if (index_columns->header.type == GRN_DB_VOID) {
    GRN_PTR_INIT(index_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_obj *columns = grn_table_delete_data_get_columns(ctx, data);
    size_t n = GRN_PTR_VECTOR_SIZE(columns);
    size_t i;
    for (i = 0; i < n; i++) {
      grn_obj *column = GRN_PTR_VALUE_AT(columns, i);
      if (grn_obj_is_index_column(ctx, column)) {
        GRN_PTR_PUT(ctx, index_columns, column);
      }
    }
  }
  return index_columns;
}

static void
call_delete_hook(grn_ctx *ctx, grn_table_delete_data *data)
{
  if (data->id) {
    grn_hook *hooks = DB_OBJ(data->table)->hooks[GRN_HOOK_DELETE];
    if (hooks) {
      // todo : grn_proc_ctx_open()
      grn_obj id_, flags_, oldvalue_, value_;
      grn_proc_ctx pctx;
      grn_proc_ctx_init(&pctx, hooks, 4, 4);
      GRN_UINT32_INIT(&id_, 0);
      GRN_UINT32_INIT(&flags_, 0);
      GRN_TEXT_INIT(&oldvalue_, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_INIT(&value_, 0);
      GRN_TEXT_SET_REF(&oldvalue_, data->key, data->key_size);
      GRN_UINT32_SET(ctx, &id_, data->id);
      GRN_UINT32_SET(ctx, &flags_, GRN_OBJ_SET);
      while (hooks) {
        grn_ctx_push(ctx, &id_);
        grn_ctx_push(ctx, &oldvalue_);
        grn_ctx_push(ctx, &value_);
        grn_ctx_push(ctx, &flags_);
        pctx.caller = NULL;
        pctx.currh = hooks;
        if (hooks->proc) {
          hooks->proc->funcs[PROC_INIT](ctx,
                                        1,
                                        &(data->table),
                                        &(pctx.user_data));
        } else {
          grn_obj_default_set_value_hook(ctx,
                                         1,
                                         &(data->table),
                                         &(pctx.user_data));
        }
        if (ctx->rc) {
          break;
        }
        hooks = hooks->next;
        pctx.offset++;
      }
    }
  }
}

static void
clear_column_values(grn_ctx *ctx, grn_table_delete_data *data)
{
  if (data->id == GRN_ID_NIL) {
    return;
  }

  grn_obj *columns = grn_table_delete_data_get_columns(ctx, data);
  size_t n = GRN_PTR_VECTOR_SIZE(columns);
  size_t i;
  for (i = 0; i < n; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(columns, i);
    grn_obj_clear_value(ctx, column, data->id);
  }
}

static void
delete_reference_records_in_index(grn_ctx *ctx,
                                  grn_table_delete_data *data,
                                  grn_obj *index)
{
  grn_ii *ii = (grn_ii *)index;
  grn_ii_cursor *ii_cursor = NULL;
  grn_posting *posting;
  grn_obj source_ids;
  unsigned int i, n_ids;
  grn_obj sources;
  grn_bool have_reference_source = GRN_FALSE;

  GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
  GRN_PTR_INIT(&sources, GRN_OBJ_VECTOR, 0);

  grn_obj_get_info(ctx, index, GRN_INFO_SOURCE, &source_ids);
  n_ids = GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id);
  if (n_ids == 0) {
    goto exit;
  }

  for (i = 0; i < n_ids; i++) {
    grn_id source_id;
    grn_obj *source;

    source_id = GRN_UINT32_VALUE_AT(&source_ids, i);
    source = grn_ctx_at(ctx, source_id);
    if (grn_obj_get_range(ctx, source) == index->header.domain) {
      GRN_PTR_PUT(ctx, &sources, source);
      have_reference_source = GRN_TRUE;
    } else {
      grn_obj_unlink(ctx, source);
      GRN_PTR_PUT(ctx, &sources, NULL);
    }
  }

  if (!have_reference_source) {
    goto exit;
  }

  ii_cursor = grn_ii_cursor_open(ctx,
                                 ii,
                                 data->id,
                                 GRN_ID_NIL,
                                 GRN_ID_MAX,
                                 ii->n_elements,
                                 0);
  if (!ii_cursor) {
    goto exit;
  }

  while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
    grn_obj *source = GRN_PTR_VALUE_AT(&sources, posting->sid - 1);
    if (!source) {
      continue;
    }
    switch (source->header.type) {
    case GRN_COLUMN_VAR_SIZE:
      switch (source->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
      case GRN_OBJ_COLUMN_SCALAR:
        grn_obj_clear_value(ctx, source, posting->rid);
        break;
      case GRN_OBJ_COLUMN_VECTOR:
        {
          grn_obj value;
          grn_obj new_value;
          GRN_TEXT_INIT(&value, 0);
          grn_obj_get_value(ctx, source, posting->rid, &value);
          if (value.header.type == GRN_UVECTOR) {
            int i, n_ids;
            GRN_RECORD_INIT(&new_value, GRN_OBJ_VECTOR, value.header.domain);
            n_ids = GRN_BULK_VSIZE(&value) / sizeof(grn_id);
            for (i = 0; i < n_ids; i++) {
              grn_id reference_id = GRN_RECORD_VALUE_AT(&value, i);
              if (reference_id == data->id) {
                continue;
              }
              GRN_RECORD_PUT(ctx, &new_value, reference_id);
            }
          } else {
            unsigned int i, n_elements;
            GRN_TEXT_INIT(&new_value, GRN_OBJ_VECTOR);
            n_elements = grn_vector_size(ctx, &value);
            for (i = 0; i < n_elements; i++) {
              const char *content;
              unsigned int content_length;
              uint32_t weight;
              grn_id domain;
              content_length = grn_vector_get_element(ctx,
                                                      &value,
                                                      i,
                                                      &content,
                                                      &weight,
                                                      &domain);
              if (grn_table_get(ctx, data->table, content, content_length) ==
                  data->id) {
                continue;
              }
              grn_vector_add_element(ctx,
                                     &new_value,
                                     content,
                                     content_length,
                                     weight,
                                     domain);
            }
          }
          grn_obj_set_value(ctx, source, posting->rid, &new_value, GRN_OBJ_SET);
          GRN_OBJ_FIN(ctx, &new_value);
          GRN_OBJ_FIN(ctx, &value);
        }
        break;
      }
      break;
    case GRN_COLUMN_FIX_SIZE:
      grn_obj_clear_value(ctx, source, posting->rid);
      break;
    }
  }

exit:
  if (ii_cursor) {
    grn_ii_cursor_close(ctx, ii_cursor);
  }
  grn_obj_unlink(ctx, &source_ids);
  {
    int i, n_sources;
    n_sources = GRN_BULK_VSIZE(&sources) / sizeof(grn_obj *);
    for (i = 0; i < n_sources; i++) {
      grn_obj *source = GRN_PTR_VALUE_AT(&sources, i);
      grn_obj_unlink(ctx, source);
    }
    grn_obj_unlink(ctx, &sources);
  }
}

static grn_rc
delete_reference_records(grn_ctx *ctx, grn_table_delete_data *data)
{
  grn_obj *index_columns = grn_table_delete_data_get_index_columns(ctx, data);
  size_t n = GRN_PTR_VECTOR_SIZE(index_columns);
  size_t i;
  for (i = 0; i < n; i++) {
    grn_obj *index_column = GRN_PTR_VALUE_AT(index_columns, i);
    delete_reference_records_in_index(ctx, data, index_column);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }

  return ctx->rc;
}

static grn_rc
grn_table_delete_prepare(grn_ctx *ctx, grn_table_delete_data *data)
{
  grn_rc rc = delete_reference_records(ctx, data);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  call_delete_hook(ctx, data);
  clear_column_values(ctx, data);

  return rc;
}

grn_rc
grn_table_delete(grn_ctx *ctx,
                 grn_obj *table,
                 const void *key,
                 unsigned int key_size)
{
  grn_id rid = GRN_ID_NIL;
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    if (key && key_size) {
      rid = grn_table_get(ctx, table, key, key_size);
    }
    if (rid) {
      grn_table_delete_data data;
      grn_table_delete_data_init(ctx, &data, table);
      data.id = rid;
      switch (table->header.type) {
      case GRN_DB:
        /* todo : delete tables and columns from db */
        break;
      case GRN_TABLE_PAT_KEY:
        WITH_NORMALIZE((grn_pat *)table, key, key_size, {
          grn_pat *pat = (grn_pat *)table;
          GRN_TABLE_LOCK_BEGIN(ctx, table)
          {
            data.key = key;
            data.key_size = key_size;
            rc = grn_table_delete_prepare(ctx, &data);
            if (rc == GRN_SUCCESS) {
              rc = grn_pat_delete(ctx, pat, key, key_size, NULL);
            }
          }
          GRN_TABLE_LOCK_END(ctx);
        });
        break;
      case GRN_TABLE_DAT_KEY:
        WITH_NORMALIZE((grn_dat *)table, key, key_size, {
          grn_dat *dat = (grn_dat *)table;
          GRN_TABLE_LOCK_BEGIN(ctx, table)
          {
            data.key = key;
            data.key_size = key_size;
            rc = grn_table_delete_prepare(ctx, &data);
            if (rc == GRN_SUCCESS) {
              rc = grn_dat_delete(ctx, dat, key, key_size, NULL);
            }
          }
          GRN_TABLE_LOCK_END(ctx);
        });
        break;
      case GRN_TABLE_HASH_KEY:
        WITH_NORMALIZE((grn_hash *)table, key, key_size, {
          grn_hash *hash = (grn_hash *)table;
          GRN_TABLE_LOCK_BEGIN(ctx, table)
          {
            data.key = key;
            data.key_size = key_size;
            rc = grn_table_delete_prepare(ctx, &data);
            if (rc == GRN_SUCCESS) {
              rc = grn_hash_delete(ctx, hash, key, key_size, NULL);
            }
          }
          GRN_TABLE_LOCK_END(ctx);
        });
        break;
      }
      grn_table_delete_data_fin(ctx, &data);
      if (rc == GRN_SUCCESS) {
        grn_obj_touch(ctx, table, NULL);
      }
    }
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_delete_by_id_without_lock(grn_ctx *ctx, grn_table_delete_data *data)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_obj *table = data->table;
  if (table) {
    grn_id id = data->id;
    if (id != GRN_ID_NIL) {
      if (!data->key && table->header.type != GRN_TABLE_NO_KEY) {
        data->key = _grn_table_key(ctx, table, id, &(data->key_size));
      }
      rc = grn_table_delete_prepare(ctx, data);
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
      grn_table_delete_optarg *optarg = data->optarg;
      // todo : support optarg
      switch (table->header.type) {
      case GRN_TABLE_PAT_KEY:
        rc = grn_pat_delete_by_id(ctx, (grn_pat *)table, id, optarg);
        break;
      case GRN_TABLE_DAT_KEY:
        rc = grn_dat_delete_by_id(ctx, (grn_dat *)table, id, optarg);
        break;
      case GRN_TABLE_HASH_KEY:
        rc = grn_hash_delete_by_id(ctx, (grn_hash *)table, id, optarg);
        break;
      case GRN_TABLE_NO_KEY:
        rc = grn_array_delete_by_id(ctx, (grn_array *)table, id, optarg);
        break;
      }
    }
  }
exit:
  return rc;
}

grn_rc
grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id)
{
  grn_rc rc;
  GRN_API_ENTER;
  rc = ctx->rc;
  grn_table_delete_data data;
  grn_table_delete_data_init(ctx, &data, table);
  data.id = id;
  GRN_TABLE_LOCK_BEGIN(ctx, table)
  {
    rc = grn_table_delete_by_id_without_lock(ctx, &data);
  }
  GRN_TABLE_LOCK_END(ctx);
  grn_table_delete_data_fin(ctx, &data);
  if (rc == GRN_SUCCESS && ctx->rc != GRN_SUCCESS) {
    rc = ctx->rc;
  }
  if (rc == GRN_SUCCESS) {
    grn_obj_touch(ctx, table, NULL);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_ja_truncate(grn_ctx *ctx, grn_ja *ja);
grn_rc
grn_ra_truncate(grn_ctx *ctx, grn_ra *ra);

grn_rc
grn_column_truncate(grn_ctx *ctx, grn_obj *column)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (column) {
    grn_hook *hooks;
    switch (column->header.type) {
    case GRN_COLUMN_INDEX:
      rc = grn_ii_truncate(ctx, (grn_ii *)column);
      break;
    case GRN_COLUMN_VAR_SIZE:
      for (hooks = DB_OBJ(column)->hooks[GRN_HOOK_SET]; hooks;
           hooks = hooks->next) {
        grn_obj_default_set_value_hook_data *data =
          (void *)GRN_NEXT_ADDR(hooks);
        grn_obj *target = grn_ctx_at(ctx, data->target);
        if (target->header.type != GRN_COLUMN_INDEX) {
          continue;
        }
        if ((rc = grn_ii_truncate(ctx, (grn_ii *)target))) {
          goto exit;
        }
      }
      rc = grn_ja_truncate(ctx, (grn_ja *)column);
      break;
    case GRN_COLUMN_FIX_SIZE:
      for (hooks = DB_OBJ(column)->hooks[GRN_HOOK_SET]; hooks;
           hooks = hooks->next) {
        grn_obj_default_set_value_hook_data *data =
          (void *)GRN_NEXT_ADDR(hooks);
        grn_obj *target = grn_ctx_at(ctx, data->target);
        if (target->header.type != GRN_COLUMN_INDEX) {
          continue;
        }
        if ((rc = grn_ii_truncate(ctx, (grn_ii *)target))) {
          goto exit;
        }
      }
      rc = grn_ra_truncate(ctx, (grn_ra *)column);
      break;
    }
    if (rc == GRN_SUCCESS) {
      grn_obj_touch(ctx, column, NULL);
    }
  }
exit:
  GRN_API_RETURN(rc);
}

static grn_rc
grn_table_truncate_reference_objects(grn_ctx *ctx, grn_obj *table)
{
  grn_id table_id = grn_obj_id(ctx, table);
  if (table_id & GRN_OBJ_TMP_OBJECT) {
    /* TODO: Search ctx->impl->values and ctx->imp->temporary_columns when
     * we need to implement this. */
    return ctx->rc;
  }

  bool is_close_opened_object_mode = (grn_thread_get_limit() == 1);

  grn_table_cursor *cursor = grn_table_cursor_open(ctx,
                                                   grn_ctx_db(ctx),
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   0,
                                                   -1,
                                                   GRN_CURSOR_BY_ID);
  if (!cursor) {
    return ctx->rc;
  }

  grn_id id;
  while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    grn_obj *object;

    if (is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    object = grn_ctx_at(ctx, id);
    if (grn_obj_is_column(ctx, object)) {
      if (grn_obj_get_range(ctx, object) == table_id) {
        grn_column_truncate(ctx, object);
      }
    } else if (grn_obj_is_table_with_key(ctx, object)) {
      if (object->header.domain == table_id) {
        grn_table_truncate(ctx, object);
      }
    }
    grn_obj_unlink(ctx, object);

    if (is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }

    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  grn_table_cursor_close(ctx, cursor);

  return ctx->rc;
}

grn_rc
grn_table_truncate(grn_ctx *ctx, grn_obj *table)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    grn_hash *cols;
    grn_obj tokenizer;
    grn_obj normalizers;
    grn_obj token_filters;
    if ((cols = grn_hash_create(ctx,
                                NULL,
                                sizeof(grn_id),
                                0,
                                GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY))) {
      if (grn_table_columns(ctx, table, "", 0, (grn_obj *)cols)) {
        grn_id *key;
        GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
          grn_obj *col = grn_ctx_at(ctx, *key);
          if (col) {
            grn_column_truncate(ctx, col);
          }
        });
      }
      grn_hash_close(ctx, cols);
    }
    if (table->header.type != GRN_TABLE_NO_KEY) {
      GRN_TEXT_INIT(&tokenizer, 0);
      grn_table_get_default_tokenizer_string(ctx, table, &tokenizer);
      GRN_TEXT_INIT(&normalizers, 0);
      grn_table_get_normalizers_string(ctx, table, &normalizers);
      GRN_TEXT_INIT(&token_filters, 0);
      grn_table_get_token_filters_string(ctx, table, &token_filters);

      rc = grn_table_truncate_reference_objects(ctx, table);
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
    }
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      rc = grn_pat_truncate(ctx, (grn_pat *)table);
      break;
    case GRN_TABLE_DAT_KEY:
      rc = grn_dat_truncate(ctx, (grn_dat *)table);
      break;
    case GRN_TABLE_HASH_KEY:
      rc = grn_hash_truncate(ctx, (grn_hash *)table);
      break;
    case GRN_TABLE_NO_KEY:
      rc = grn_array_truncate(ctx, (grn_array *)table);
      break;
    }
    if (table->header.type != GRN_TABLE_NO_KEY) {
      if (GRN_TEXT_LEN(&tokenizer) > 0) {
        grn_obj_set_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER, &tokenizer);
      }
      GRN_OBJ_FIN(ctx, &tokenizer);
      if (GRN_TEXT_LEN(&normalizers) > 0) {
        grn_obj_set_info(ctx, table, GRN_INFO_NORMALIZERS, &normalizers);
      }
      GRN_OBJ_FIN(ctx, &normalizers);
      if (GRN_TEXT_LEN(&token_filters) > 0) {
        grn_obj_set_info(ctx, table, GRN_INFO_TOKEN_FILTERS, &token_filters);
      }
      GRN_OBJ_FIN(ctx, &token_filters);
    }
    if (rc == GRN_SUCCESS) {
      grn_obj_touch(ctx, table, NULL);
    }
  }
exit:
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_get_info(grn_ctx *ctx,
                   grn_obj *table,
                   grn_table_flags *flags,
                   grn_encoding *encoding,
                   grn_obj **tokenizer,
                   grn_obj **normalizer,
                   grn_obj **token_filters)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      if (flags) {
        *flags = ((grn_pat *)table)->header->flags;
      }
      if (encoding) {
        *encoding = ((grn_pat *)table)->encoding;
      }
      if (tokenizer) {
        *tokenizer = ((grn_pat *)table)->tokenizer.proc;
      }
      if (normalizer) {
        *normalizer =
          grn_table_modules_get_proc(ctx,
                                     &(((grn_pat *)table)->normalizers),
                                     0);
      }
      if (token_filters) {
        *token_filters = &(((grn_pat *)table)->token_filter_procs);
      }
      rc = GRN_SUCCESS;
      break;
    case GRN_TABLE_DAT_KEY:
      if (flags) {
        *flags = ((grn_dat *)table)->header->flags;
      }
      if (encoding) {
        *encoding = ((grn_dat *)table)->encoding;
      }
      if (tokenizer) {
        *tokenizer = ((grn_dat *)table)->tokenizer.proc;
      }
      if (normalizer) {
        *normalizer =
          grn_table_modules_get_proc(ctx,
                                     &(((grn_dat *)table)->normalizers),
                                     0);
      }
      if (token_filters) {
        *token_filters = &(((grn_dat *)table)->token_filter_procs);
      }
      rc = GRN_SUCCESS;
      break;
    case GRN_TABLE_HASH_KEY:
      if (flags) {
        *flags = ((grn_hash *)table)->header.common->flags;
      }
      if (encoding) {
        *encoding = ((grn_hash *)table)->encoding;
      }
      if (tokenizer) {
        *tokenizer = ((grn_hash *)table)->tokenizer.proc;
      }
      if (normalizer) {
        *normalizer =
          grn_table_modules_get_proc(ctx,
                                     &(((grn_hash *)table)->normalizers),
                                     0);
      }
      if (token_filters) {
        *token_filters = &(((grn_hash *)table)->token_filter_procs);
      }
      rc = GRN_SUCCESS;
      break;
    case GRN_TABLE_NO_KEY:
      if (flags) {
        *flags = grn_array_get_flags(ctx, ((grn_array *)table));
      }
      if (encoding) {
        *encoding = GRN_ENC_NONE;
      }
      if (tokenizer) {
        *tokenizer = NULL;
      }
      if (normalizer) {
        *normalizer = NULL;
      }
      if (token_filters) {
        *token_filters = NULL;
      }
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
    case GRN_DB:
      n = grn_table_size(ctx, ((grn_db *)table)->keys);
      break;
    case GRN_TABLE_PAT_KEY:
      n = grn_pat_size(ctx, (grn_pat *)table);
      break;
    case GRN_TABLE_DAT_KEY:
      n = grn_dat_size(ctx, (grn_dat *)table);
      break;
    case GRN_TABLE_HASH_KEY:
      n = grn_hash_size(ctx, (grn_hash *)table);
      break;
    case GRN_TABLE_NO_KEY:
      n = grn_array_size(ctx, (grn_array *)table);
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT,
          "[table][size] must be table or DB: <%s>(%u)",
          grn_obj_type_to_string(table->header.type),
          table->header.type);
      break;
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "[table][size] must not NULL");
  }
  GRN_API_RETURN(n);
}

grn_table_cursor *
grn_table_cursor_open(grn_ctx *ctx,
                      grn_obj *table,
                      const void *min,
                      unsigned int min_size,
                      const void *max,
                      unsigned int max_size,
                      int offset,
                      int limit,
                      int flags)
{
  const char *tag = "[table][cursor][open]";
  grn_rc rc;
  grn_table_cursor *tc = NULL;
  unsigned int table_size;
  GRN_API_ENTER;
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT, "%s table must not be NULL", tag);
    GRN_API_RETURN(NULL);
  }
  table_size = grn_table_size(ctx, table);
  if (table_size == 0 && ctx->rc != GRN_SUCCESS) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc, "%s failed to get table size: %s", tag, message);
    GRN_API_RETURN(NULL);
  }
  if (flags & GRN_CURSOR_PREFIX) {
    if (offset < 0) {
      ERR(GRN_TOO_SMALL_OFFSET,
          "%s can't use negative offset with GRN_CURSOR_PREFIX: %d",
          tag,
          offset);
    } else if (offset != 0 && (unsigned int)offset >= table_size) {
      ERR(GRN_TOO_LARGE_OFFSET,
          "%s offset is not less than table size: offset:%d, table_size:%d",
          tag,
          offset,
          table_size);
    } else {
      if (limit < -1) {
        ERR(GRN_TOO_SMALL_LIMIT,
            "%s can't use smaller limit than -1 with GRN_CURSOR_PREFIX: %d",
            tag,
            limit);
      } else if (limit == -1) {
        limit = table_size;
      }
    }
  } else {
    rc = grn_output_range_normalize(ctx, table_size, &offset, &limit);
    if (rc != GRN_SUCCESS) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
      ERR(rc,
          "%s[%.*s] failed to normalize output range: "
          "table_size:%u, "
          "offset:%d, "
          "limit:%d",
          tag,
          name_size,
          name,
          table_size,
          offset,
          limit);
    }
  }
  if (!ctx->rc) {
    if (table->header.type == GRN_DB) {
      table = ((grn_db *)table)->keys;
    }
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      {
        grn_pat *pat = (grn_pat *)table;
        WITH_NORMALIZE(pat, min, min_size, {
          WITH_NORMALIZE(pat, max, max_size, {
            grn_pat_cursor *pat_cursor;
            pat_cursor = grn_pat_cursor_open(ctx,
                                             pat,
                                             min,
                                             min_size,
                                             max,
                                             max_size,
                                             offset,
                                             limit,
                                             flags);
            tc = (grn_table_cursor *)pat_cursor;
          });
        });
      }
      break;
    case GRN_TABLE_DAT_KEY:
      {
        grn_dat *dat = (grn_dat *)table;
        WITH_NORMALIZE(dat, min, min_size, {
          WITH_NORMALIZE(dat, max, max_size, {
            grn_dat_cursor *dat_cursor;
            dat_cursor = grn_dat_cursor_open(ctx,
                                             dat,
                                             min,
                                             min_size,
                                             max,
                                             max_size,
                                             offset,
                                             limit,
                                             flags);
            tc = (grn_table_cursor *)dat_cursor;
          });
        });
      }
      break;
    case GRN_TABLE_HASH_KEY:
      {
        grn_hash *hash = (grn_hash *)table;
        WITH_NORMALIZE(hash, min, min_size, {
          WITH_NORMALIZE(hash, max, max_size, {
            grn_hash_cursor *hash_cursor;
            hash_cursor = grn_hash_cursor_open(ctx,
                                               hash,
                                               min,
                                               min_size,
                                               max,
                                               max_size,
                                               offset,
                                               limit,
                                               flags);
            tc = (grn_table_cursor *)hash_cursor;
          });
        });
      }
      break;
    case GRN_TABLE_NO_KEY:
      tc = (grn_table_cursor *)grn_array_cursor_open(ctx,
                                                     (grn_array *)table,
                                                     GRN_ID_NIL,
                                                     GRN_ID_NIL,
                                                     offset,
                                                     limit,
                                                     flags);
      break;
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
grn_table_cursor_open_by_id(
  grn_ctx *ctx, grn_obj *table, grn_id min, grn_id max, int flags)
{
  grn_table_cursor *tc = NULL;
  GRN_API_ENTER;
  if (table) {
    switch (table->header.type) {
    case GRN_TABLE_PAT_KEY:
      tc = (grn_table_cursor *)grn_pat_cursor_open(ctx,
                                                   (grn_pat *)table,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   0,
                                                   -1,
                                                   flags);
      break;
    case GRN_TABLE_DAT_KEY:
      tc = (grn_table_cursor *)grn_dat_cursor_open(ctx,
                                                   (grn_dat *)table,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   0,
                                                   -1,
                                                   flags);
      break;
    case GRN_TABLE_HASH_KEY:
      tc = (grn_table_cursor *)grn_hash_cursor_open(ctx,
                                                    (grn_hash *)table,
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    0,
                                                    0,
                                                    -1,
                                                    flags);
      break;
    case GRN_TABLE_NO_KEY:
      tc = (grn_table_cursor *)
        grn_array_cursor_open(ctx, (grn_array *)table, min, max, 0, -1, flags);
      break;
    }
  }
  GRN_API_RETURN(tc);
}

grn_rc
grn_table_cursor_close(grn_ctx *ctx, grn_table_cursor *tc)
{
  const char *tag = "[table][cursor][close]";
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  if (!tc) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc, "%s invalid cursor", tag);
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
      for (entry = 0; entry < GRN_N_HOOK_ENTRIES; entry++) {
        grn_hook_free(ctx, DB_OBJ(tc)->hooks[entry]);
      }
      */
      grn_obj_delete_by_id(ctx, DB_OBJ(tc)->db, DB_OBJ(tc)->id, GRN_FALSE);
    }
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      grn_pat_cursor_close(ctx, (grn_pat_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      grn_dat_cursor_close(ctx, (grn_dat_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      grn_hash_cursor_close(ctx, (grn_hash_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      grn_array_cursor_close(ctx, (grn_array_cursor *)tc);
      break;
    default:
      rc = GRN_INVALID_ARGUMENT;
      ERR(rc, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_inline static grn_id
grn_table_cursor_next_inline(grn_ctx *ctx, grn_table_cursor *tc)
{
  const char *tag = "[table][cursor][next]";
  grn_id id = GRN_ID_NIL;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      id = grn_pat_cursor_next(ctx, (grn_pat_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      id = grn_dat_cursor_next(ctx, (grn_dat_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      id = grn_hash_cursor_next(ctx, (grn_hash_cursor *)tc);
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      id = grn_array_cursor_next(ctx, (grn_array_cursor *)tc);
      break;
    case GRN_CURSOR_COLUMN_INDEX:
      {
        grn_posting *ip = grn_index_cursor_next(ctx, (grn_obj *)tc, NULL);
        if (ip) {
          id = ip->rid;
        }
      }
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  return id;
}

grn_id
grn_table_cursor_next(grn_ctx *ctx, grn_table_cursor *tc)
{
  grn_id id;
  GRN_API_ENTER;
  id = grn_table_cursor_next_inline(ctx, tc);
  GRN_API_RETURN(id);
}

grn_rc
grn_table_cursor_foreach(grn_ctx *ctx,
                         grn_table_cursor *cursor,
                         grn_table_cursor_foreach_func func,
                         void *user_data)
{
  const char *tag = "[table][cursor][foreach]";
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  if (!cursor) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (cursor->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      {
        grn_id id;
        while ((id = grn_pat_cursor_next(ctx, (grn_pat_cursor *)cursor))) {
          rc = func(ctx, cursor, id, user_data);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
      }
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      {
        grn_id id;
        while ((id = grn_dat_cursor_next(ctx, (grn_dat_cursor *)cursor))) {
          rc = func(ctx, cursor, id, user_data);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
      }
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      {
        grn_id id;
        while ((id = grn_hash_cursor_next(ctx, (grn_hash_cursor *)cursor))) {
          rc = func(ctx, cursor, id, user_data);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
      }
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      {
        grn_id id;
        while ((id = grn_array_cursor_next(ctx, (grn_array_cursor *)cursor))) {
          rc = func(ctx, cursor, id, user_data);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
      }
      break;
    case GRN_CURSOR_COLUMN_INDEX:
      {
        grn_posting *posting;
        while (
          (posting = grn_index_cursor_next(ctx, (grn_obj *)cursor, NULL))) {
          rc = func(ctx, cursor, posting->rid, user_data);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
      }
      break;
    default:
      rc = GRN_INVALID_ARGUMENT;
      ERR(rc, "%s invalid type %d", tag, cursor->header.type);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

int
grn_table_cursor_get_key(grn_ctx *ctx, grn_table_cursor *tc, void **key)
{
  const char *tag = "[table][cursor][get-key]";
  int len = 0;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      len = grn_pat_cursor_get_key(ctx, (grn_pat_cursor *)tc, key);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      len =
        grn_dat_cursor_get_key(ctx, (grn_dat_cursor *)tc, (const void **)key);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      len = grn_hash_cursor_get_key(ctx, (grn_hash_cursor *)tc, key);
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(len);
}

grn_inline static int
grn_table_cursor_get_value_inline(grn_ctx *ctx,
                                  grn_table_cursor *tc,
                                  void **value)
{
  const char *tag = "[table][cursor][get-value]";
  int len = 0;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      len = grn_pat_cursor_get_value(ctx, (grn_pat_cursor *)tc, value);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      *value = NULL;
      len = 0;
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      len = grn_hash_cursor_get_value(ctx, (grn_hash_cursor *)tc, value);
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      len = grn_array_cursor_get_value(ctx, (grn_array_cursor *)tc, value);
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  return len;
}

int
grn_table_cursor_get_value(grn_ctx *ctx, grn_table_cursor *tc, void **value)
{
  int len;
  GRN_API_ENTER;
  len = grn_table_cursor_get_value_inline(ctx, tc, value);
  GRN_API_RETURN(len);
}

uint32_t
grn_table_cursor_get_key_value(grn_ctx *ctx,
                               grn_table_cursor *tc,
                               void **key,
                               uint32_t *key_size,
                               void **value)
{
  const char *tag = "[table][cursor][get-key-value]";
  uint32_t value_size = 0;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      value_size = grn_pat_cursor_get_key_value(ctx,
                                                (grn_pat_cursor *)tc,
                                                key,
                                                key_size,
                                                value);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      {
        uint32_t dat_key_size =
          grn_dat_cursor_get_key(ctx, (grn_dat_cursor *)tc, (const void **)key);
        if (key_size) {
          *key_size = dat_key_size;
        }
        if (value) {
          *value = NULL;
        }
      }
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      value_size = grn_hash_cursor_get_key_value(ctx,
                                                 (grn_hash_cursor *)tc,
                                                 key,
                                                 key_size,
                                                 value);
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      if (key) {
        *key = NULL;
      }
      if (key_size) {
        *key_size = 0;
      }
      value_size =
        grn_array_cursor_get_value(ctx, (grn_array_cursor *)tc, value);
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(value_size);
}

grn_rc
grn_table_cursor_set_value(grn_ctx *ctx,
                           grn_table_cursor *tc,
                           const void *value,
                           int flags)
{
  const char *tag = "[table][cursor][set-value]";
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      rc = grn_pat_cursor_set_value(ctx, (grn_pat_cursor *)tc, value, flags);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      rc = GRN_OPERATION_NOT_SUPPORTED;
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      rc = grn_hash_cursor_set_value(ctx, (grn_hash_cursor *)tc, value, flags);
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      rc =
        grn_array_cursor_set_value(ctx, (grn_array_cursor *)tc, value, flags);
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_cursor_delete(grn_ctx *ctx, grn_table_cursor *tc)
{
  const char *tag = "[table][cursor][delete]";
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      {
        grn_pat_cursor *pc = (grn_pat_cursor *)tc;
        grn_table_delete_data data;
        grn_table_delete_data_init(ctx, &data, (grn_obj *)(pc->pat));
        data.id = pc->curr_rec;
        GRN_TABLE_LOCK_BEGIN(ctx, data.table)
        {
          data.key = _grn_pat_key(ctx, pc->pat, data.id, &(data.key_size));
          rc = grn_table_delete_prepare(ctx, &data);
          if (rc == GRN_SUCCESS) {
            rc = grn_pat_cursor_delete(ctx, pc, NULL);
          }
        }
        GRN_TABLE_LOCK_END(ctx);
        grn_table_delete_data_fin(ctx, &data);
      }
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      rc = GRN_OPERATION_NOT_SUPPORTED;
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      {
        grn_hash_cursor *hc = (grn_hash_cursor *)tc;
        grn_table_delete_data data;
        grn_table_delete_data_init(ctx, &data, (grn_obj *)(hc->hash));
        data.id = hc->curr_rec;
        GRN_TABLE_LOCK_BEGIN(ctx, data.table)
        {
          data.key = _grn_hash_key(ctx, hc->hash, data.id, &(data.key_size));
          rc = grn_table_delete_prepare(ctx, &data);
          if (rc == GRN_SUCCESS) {
            rc = grn_hash_cursor_delete(ctx, hc, NULL);
          }
        }
        GRN_TABLE_LOCK_END(ctx);
        grn_table_delete_data_fin(ctx, &data);
      }
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      {
        grn_array_cursor *ac = (grn_array_cursor *)tc;
        grn_table_delete_data data;
        grn_table_delete_data_init(ctx, &data, (grn_obj *)(ac->array));
        data.id = ac->curr_rec;
        GRN_TABLE_LOCK_BEGIN(ctx, data.table)
        {
          rc = grn_table_delete_prepare(ctx, &data);
          if (rc == GRN_SUCCESS) {
            rc = grn_array_cursor_delete(ctx, ac, NULL);
          }
        }
        GRN_TABLE_LOCK_END(ctx);
        grn_table_delete_data_fin(ctx, &data);
      }
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

grn_obj *
grn_table_cursor_table(grn_ctx *ctx, grn_table_cursor *tc)
{
  const char *tag = "[table][cursor][table]";
  grn_obj *obj = NULL;
  GRN_API_ENTER;
  if (!tc) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (tc->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      obj = (grn_obj *)(((grn_pat_cursor *)tc)->pat);
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      obj = (grn_obj *)(((grn_dat_cursor *)tc)->dat);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      obj = (grn_obj *)(((grn_hash_cursor *)tc)->hash);
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      obj = (grn_obj *)(((grn_array_cursor *)tc)->array);
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, tc->header.type);
      break;
    }
  }
  GRN_API_RETURN(obj);
}

size_t
grn_table_cursor_get_max_n_records(grn_ctx *ctx, grn_table_cursor *cursor)
{
  const char *tag = "[table][cursor][get-max-n-records]";
  size_t max_n_records = 0;
  GRN_API_ENTER;
  if (!cursor) {
    ERR(GRN_INVALID_ARGUMENT, "%s invalid cursor", tag);
  } else {
    switch (cursor->header.type) {
    case GRN_CURSOR_TABLE_PAT_KEY:
      max_n_records = ((grn_pat_cursor *)cursor)->rest;
      if (max_n_records == GRN_ID_MAX) {
        max_n_records = grn_pat_size(ctx, ((grn_pat_cursor *)cursor)->pat);
      }
      break;
    case GRN_CURSOR_TABLE_DAT_KEY:
      max_n_records =
        grn_dat_cursor_get_max_n_records(ctx, (grn_dat_cursor *)cursor);
      break;
    case GRN_CURSOR_TABLE_HASH_KEY:
      max_n_records = ((grn_hash_cursor *)cursor)->rest;
      if (max_n_records == GRN_ID_MAX) {
        max_n_records = grn_hash_size(ctx, ((grn_hash_cursor *)cursor)->hash);
      }
      break;
    case GRN_CURSOR_TABLE_NO_KEY:
      max_n_records = ((grn_array_cursor *)cursor)->rest;
      if (max_n_records == GRN_ID_MAX) {
        max_n_records =
          grn_array_size(ctx, ((grn_array_cursor *)cursor)->array);
      }
      break;
    default:
      ERR(GRN_INVALID_ARGUMENT, "%s invalid type %d", tag, cursor->header.type);
      break;
    }
  }
  GRN_API_RETURN(max_n_records);
}

grn_rc
grn_table_search(grn_ctx *ctx,
                 grn_obj *table,
                 const void *key,
                 uint32_t key_size,
                 grn_operator mode,
                 grn_obj *res,
                 grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  switch (table->header.type) {
  case GRN_TABLE_PAT_KEY:
    {
      grn_pat *pat = (grn_pat *)table;
      WITH_NORMALIZE(pat, key, key_size, {
        switch (mode) {
        case GRN_OP_EXACT:
          {
            grn_id id = grn_pat_get(ctx, pat, key, key_size, NULL);
            if (id) {
              grn_table_add(ctx, res, &id, sizeof(grn_id), NULL);
            }
          }
          // todo : support op;
          break;
        case GRN_OP_LCP:
          {
            grn_id id = grn_pat_lcp_search(ctx, pat, key, key_size);
            if (id) {
              grn_table_add(ctx, res, &id, sizeof(grn_id), NULL);
            }
          }
          // todo : support op;
          break;
        case GRN_OP_SUFFIX:
          rc = grn_pat_suffix_search(ctx, pat, key, key_size, (grn_hash *)res);
          // todo : support op;
          break;
        case GRN_OP_PREFIX:
          rc = grn_pat_prefix_search(ctx, pat, key, key_size, (grn_hash *)res);
          // todo : support op;
          break;
        case GRN_OP_TERM_EXTRACT:
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
              if (!(len = grn_charlen(ctx, sp, se))) {
                break;
              }
            }
          }
          // todo : support op;
          break;
        default:
          rc = GRN_INVALID_ARGUMENT;
          ERR(rc,
              "[table][search][pat] invalid mode: %s",
              grn_operator_to_string(mode));
          break;
        }
      });
    }
    break;
  case GRN_TABLE_DAT_KEY:
    {
      grn_dat *dat = (grn_dat *)table;
      WITH_NORMALIZE(dat, key, key_size, {
        switch (mode) {
        case GRN_OP_EXACT:
          {
            grn_id id = grn_dat_get(ctx, dat, key, key_size, NULL);
            if (id) {
              grn_table_add(ctx, res, &id, sizeof(grn_id), NULL);
            }
          }
          break;
        case GRN_OP_PREFIX:
          {
            grn_dat_cursor *dc = grn_dat_cursor_open(ctx,
                                                     dat,
                                                     key,
                                                     key_size,
                                                     NULL,
                                                     0,
                                                     0,
                                                     -1,
                                                     GRN_CURSOR_PREFIX);
            if (dc) {
              grn_id id;
              while ((id = grn_dat_cursor_next(ctx, dc))) {
                grn_table_add(ctx, res, &id, sizeof(grn_id), NULL);
              }
              grn_dat_cursor_close(ctx, dc);
            }
          }
          break;
        case GRN_OP_LCP:
          {
            grn_id id = grn_dat_lcp_search(ctx, dat, key, key_size);
            if (id) {
              grn_table_add(ctx, res, &id, sizeof(grn_id), NULL);
            }
          }
          break;
        case GRN_OP_TERM_EXTRACT:
          {
            int len;
            grn_id tid;
            const char *sp = key;
            const char *se = sp + key_size;
            for (; sp < se; sp += len) {
              if ((tid = grn_dat_lcp_search(ctx, dat, sp, se - sp))) {
                grn_table_add(ctx, res, &tid, sizeof(grn_id), NULL);
                /* todo : nsubrec++ if GRN_OBJ_TABLE_SUBSET assigned */
              }
              if (!(len = grn_charlen(ctx, sp, se))) {
                break;
              }
            }
          }
          // todo : support op;
          break;
        default:
          rc = GRN_INVALID_ARGUMENT;
          ERR(rc,
              "[table][search][dat] invalid mode: %s",
              grn_operator_to_string(mode));
          break;
        }
      });
    }
    break;
  case GRN_TABLE_HASH_KEY:
    {
      grn_hash *hash = (grn_hash *)table;
      WITH_NORMALIZE(hash, key, key_size, {
        switch (mode) {
        case GRN_OP_EXACT:
          {
            grn_id id = grn_hash_get(ctx, hash, key, key_size, NULL);
            if (id) {
              grn_table_add(ctx, res, &id, sizeof(grn_id), NULL);
            }
          }
          break;
        default:
          rc = GRN_INVALID_ARGUMENT;
          ERR(rc,
              "[table][search][hash] invalid mode: %s",
              grn_operator_to_string(mode));
          break;
        }
      });
    }
    break;
  default:
    rc = GRN_INVALID_ARGUMENT;
    {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect_type(ctx, &inspected, table->header.type);
      ERR(rc,
          "[table][search] invalid type: %.*s",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    break;
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_fuzzy_search(grn_ctx *ctx,
                       grn_obj *table,
                       const void *key,
                       uint32_t key_size,
                       grn_fuzzy_search_optarg *args,
                       grn_obj *res,
                       grn_operator op)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  switch (table->header.type) {
  case GRN_TABLE_PAT_KEY:
    {
      grn_pat *pat = (grn_pat *)table;
      if (!grn_table_size(ctx, res) && op == GRN_OP_OR) {
        WITH_NORMALIZE(pat, key, key_size, {
          rc = grn_pat_fuzzy_search(ctx,
                                    pat,
                                    key,
                                    key_size,
                                    args,
                                    (grn_hash *)res);
        });
      } else {
        grn_obj *hash;
        hash = grn_table_create(ctx,
                                NULL,
                                0,
                                NULL,
                                GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                table,
                                NULL);
        WITH_NORMALIZE(pat, key, key_size, {
          rc = grn_pat_fuzzy_search(ctx,
                                    pat,
                                    key,
                                    key_size,
                                    args,
                                    (grn_hash *)hash);
        });
        if (rc == GRN_SUCCESS) {
          rc = grn_table_setoperation(ctx, res, hash, res, op);
        }
        grn_obj_unlink(ctx, hash);
      }
    }
    break;
  default:
    rc = GRN_OPERATION_NOT_SUPPORTED;
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
    case GRN_TABLE_PAT_KEY:
      r = grn_pat_next(ctx, (grn_pat *)table, id);
      break;
    case GRN_TABLE_DAT_KEY:
      r = grn_dat_next(ctx, (grn_dat *)table, id);
      break;
    case GRN_TABLE_HASH_KEY:
      r = grn_hash_next(ctx, (grn_hash *)table, id);
      break;
    case GRN_TABLE_NO_KEY:
      r = grn_array_next(ctx, (grn_array *)table, id);
      break;
    }
  }
  GRN_API_RETURN(r);
}

static grn_inline void
grn_obj_search_index_report(grn_ctx *ctx, const char *tag, grn_obj *index)
{
  grn_report_index(ctx, "[object][search]", tag, index);
}

static grn_inline void
grn_obj_search_accessor_report(grn_ctx *ctx, const char *tag, grn_obj *accessor)
{
  grn_report_accessor(ctx, "[object][search]", tag, accessor);
}

static grn_inline grn_rc
grn_obj_search_table_id(grn_ctx *ctx,
                        grn_obj *table,
                        grn_obj *query,
                        grn_obj *res,
                        grn_operator op,
                        grn_search_optarg *optarg)
{
  grn_id id = GRN_ID_NIL;
  if (query->header.domain == DB_OBJ(table)->id) {
    id = GRN_RECORD_VALUE(query);
  } else if (query->header.domain == GRN_DB_UINT32) {
    id = GRN_UINT32_VALUE(query);
  } else {
    grn_obj id_value;
    GRN_UINT32_INIT(&id_value, 0);
    grn_rc rc = grn_obj_cast(ctx, query, &id_value, false);
    if (rc == GRN_SUCCESS) {
      id = GRN_UINT32_VALUE(&id_value);
    }
    GRN_OBJ_FIN(ctx, &id_value);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  if (id != GRN_ID_NIL) {
    if (grn_table_at(ctx, table, id) == id) {
      grn_posting_internal posting = {0};
      posting.sid = 1;
      posting.weight_float = 1.0;
      posting.rid = id;
      grn_ii_posting_add_float(ctx,
                               (grn_posting *)(&posting),
                               (grn_hash *)res,
                               op);
    }
  }
  grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  return GRN_SUCCESS;
}

typedef struct {
  grn_obj *accessor;
  grn_obj *query;
  grn_search_optarg *optarg;
} grn_obj_search_accessor_execute_data;

static grn_rc
grn_obj_search_accessor_execute(grn_ctx *ctx,
                                grn_obj *index,
                                grn_operator mode,
                                grn_obj *res,
                                grn_operator op,
                                void *user_data)
{
  grn_obj_search_accessor_execute_data *data = user_data;
  if (grn_obj_is_accessor(ctx, index)) {
    grn_accessor *last_accessor = (grn_accessor *)index;
    switch (last_accessor->action) {
    case GRN_ACCESSOR_GET_ID:
      if (mode != GRN_OP_EQUAL) {
        return GRN_SUCCESS;
      }
      if (!grn_obj_is_table(ctx, last_accessor->obj)) {
        return GRN_SUCCESS;
      }
      grn_obj_search_accessor_report(ctx, "[id]", data->accessor);
      return grn_obj_search_table_id(ctx,
                                     last_accessor->obj,
                                     data->query,
                                     res,
                                     op,
                                     data->optarg);
    default:
      return grn_obj_search(ctx,
                            last_accessor->obj,
                            data->query,
                            res,
                            op,
                            data->optarg);
    }
  } else {
    return grn_obj_search(ctx, index, data->query, res, op, data->optarg);
  }
}

static grn_rc
grn_obj_search_column_index_by_id(grn_ctx *ctx,
                                  grn_obj *obj,
                                  grn_id tid,
                                  grn_obj *res,
                                  grn_operator op,
                                  grn_search_optarg *optarg)
{
  grn_obj_search_index_report(ctx, "[id]", obj);
  grn_ii_at(ctx, (grn_ii *)obj, tid, (grn_hash *)res, op);
  if (ctx->rc == GRN_SUCCESS) {
    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
  }
  return ctx->rc;
}

static grn_rc
grn_obj_search_column_index_by_key(grn_ctx *ctx,
                                   grn_obj *obj,
                                   grn_obj *query,
                                   grn_obj *res,
                                   grn_operator op,
                                   grn_search_optarg *optarg)
{
  grn_rc rc;
  grn_id key_type = GRN_ID_NIL;
  const char *key;
  unsigned int key_len;
  grn_obj casted_query;
  grn_bool need_cast = GRN_FALSE;
  grn_id *query_domain_keep = optarg ? optarg->query_domain : NULL;

  {
    if (grn_obj_is_index_column(ctx, obj)) {
      grn_obj *lexicon = grn_ctx_at(ctx, obj->header.domain);
      if (lexicon) {
        if (grn_table_have_tokenizer(ctx, lexicon)) {
          grn_obj source_ids;
          GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
          grn_obj_get_info(ctx, obj, GRN_INFO_SOURCE, &source_ids);
          size_t n_source_ids = GRN_UINT32_VECTOR_SIZE(&source_ids);
          if (n_source_ids >= 1) {
            grn_id source_id = GRN_UINT32_VALUE_AT(&source_ids, 0);
            grn_obj *source = grn_ctx_at(ctx, source_id);
            if (source) {
              if (grn_obj_is_table_with_key(ctx, source)) {
                key_type = source->header.domain;
              } else if (grn_obj_is_reference_column(ctx, source)) {
                const grn_id range_id = DB_OBJ(source)->range;
                grn_obj *referenced_table = grn_ctx_at(ctx, range_id);
                key_type = referenced_table->header.domain;
                grn_obj_unref(ctx, referenced_table);
              } else {
                key_type = DB_OBJ(source)->range;
              }
              need_cast =
                !grn_type_id_is_compatible(ctx, query->header.domain, key_type);
              grn_obj_unref(ctx, source);
            }
          }
          GRN_OBJ_FIN(ctx, &source_ids);
        } else {
          key_type = lexicon->header.domain;
          need_cast = (query->header.domain != key_type);
        }
        grn_obj_unref(ctx, lexicon);
      }
    } else {
      grn_obj *table = grn_ctx_at(ctx, obj->header.domain);
      if (table) {
        key_type = table->header.domain;
        need_cast =
          !grn_type_id_is_compatible(ctx, query->header.domain, key_type);
        grn_obj_unref(ctx, table);
      }
    }
  }
  if (need_cast) {
    GRN_OBJ_INIT(&casted_query, GRN_BULK, 0, key_type);
    rc = grn_obj_cast(ctx, query, &casted_query, GRN_FALSE);
    if (rc == GRN_SUCCESS) {
      key = GRN_BULK_HEAD(&casted_query);
      key_len = GRN_BULK_VSIZE(&casted_query);
      if (optarg) {
        optarg->query_domain = &key_type;
      }
    }
  } else {
    rc = GRN_SUCCESS;
    key = GRN_BULK_HEAD(query);
    key_len = GRN_BULK_VSIZE(query);
    if (optarg) {
      optarg->query_domain = &(query->header.domain);
    }
  }
  if (rc == GRN_SUCCESS) {
    if (grn_logger_pass(ctx, GRN_REPORT_INDEX_LOG_LEVEL)) {
      const char *tag;
      if (optarg) {
        switch (optarg->mode) {
        case GRN_OP_EQUAL:
          tag = "[key][equal]";
          break;
        case GRN_OP_MATCH:
          tag = "[key][match]";
          break;
        case GRN_OP_EXACT:
          tag = "[key][exact]";
          break;
        case GRN_OP_NEAR:
          tag = "[key][near]";
          break;
        case GRN_OP_NEAR_NO_OFFSET:
          tag = "[key][near-no-offset]";
          break;
        case GRN_OP_NEAR_PHRASE:
          tag = "[key][near-phrase]";
          break;
        case GRN_OP_ORDERED_NEAR_PHRASE:
          tag = "[key][ordered-near-phrase]";
          break;
        case GRN_OP_NEAR_PHRASE_PRODUCT:
          tag = "[key][near-phrase-product]";
          break;
        case GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT:
          tag = "[key][ordered-near-phrase-product]";
          break;
        case GRN_OP_SIMILAR:
          tag = "[key][similar]";
          break;
        case GRN_OP_REGEXP:
          tag = "[key][regexp]";
          break;
        case GRN_OP_FUZZY:
          tag = "[key][fuzzy]";
          break;
        case GRN_OP_QUORUM:
          tag = "[key][quorum]";
          break;
        default:
          tag = "[key][unknown]";
          break;
        }
      } else {
        tag = "[key][exact]";
      }
      grn_obj_search_index_report(ctx, tag, obj);
    }
    if (optarg && optarg->mode == GRN_OP_EQUAL) {
      grn_obj *lexicon = grn_ctx_at(ctx, obj->header.domain);
      if (!lexicon) {
        rc = GRN_INVALID_ARGUMENT;
        goto exit;
      }
      grn_id token_id = grn_table_get(ctx, lexicon, key, key_len);
      if (token_id != GRN_ID_NIL) {
        grn_ii_at(ctx, (grn_ii *)obj, token_id, (grn_hash *)res, op);
      }
      grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
    } else {
      rc = grn_ii_sel(ctx,
                      (grn_ii *)obj,
                      key,
                      key_len,
                      (grn_hash *)res,
                      op,
                      optarg);
    }
  }

exit:
  if (need_cast) {
    GRN_OBJ_FIN(ctx, &casted_query);
  }

  if (optarg) {
    optarg->query_domain = query_domain_keep;
  }

  return rc;
}

static grn_rc
grn_obj_search_column_index(grn_ctx *ctx,
                            grn_obj *obj,
                            grn_obj *query,
                            grn_obj *res,
                            grn_operator op,
                            grn_search_optarg *optarg)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;

  if (DB_OBJ(obj)->range == res->header.domain) {
    switch (query->header.type) {
    case GRN_BULK:
      if (query->header.domain == obj->header.domain &&
          GRN_BULK_VSIZE(query) == sizeof(grn_id)) {
        grn_id tid = GRN_RECORD_VALUE(query);
        rc = grn_obj_search_column_index_by_id(ctx, obj, tid, res, op, optarg);
      } else {
        rc =
          grn_obj_search_column_index_by_key(ctx, obj, query, res, op, optarg);
      }
      break;
    case GRN_QUERY:
      rc = GRN_FUNCTION_NOT_IMPLEMENTED;
      break;
    }
  }

  return rc;
}

static grn_rc
grn_obj_search_column_data(grn_ctx *ctx,
                           grn_obj *obj,
                           grn_obj *query,
                           grn_obj *res,
                           grn_operator op,
                           grn_search_optarg *optarg)
{
  grn_obj *table = grn_ctx_at(ctx, obj->header.domain);
  grn_obj *expr = NULL;
  grn_obj *variable;
  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expr, variable);
  if (!expr) {
    goto exit;
  }
  grn_expr_append_obj(ctx, expr, obj, GRN_OP_GET_VALUE, 1);
  grn_expr_append_const(ctx, expr, query, GRN_OP_PUSH, 1);
  {
    grn_operator search_op = GRN_OP_MATCH;
    if (optarg && optarg->mode != GRN_OP_EXACT) {
      search_op = optarg->mode;
    }
    grn_expr_append_op(ctx, expr, search_op, 2);
  }

  grn_table_selector table_selector;
  grn_table_selector_init(ctx, &table_selector, table, expr, op);
  grn_table_selector_select(ctx, &table_selector, res);
  grn_table_selector_fin(ctx, &table_selector);

exit:
  if (expr) {
    grn_obj_close(ctx, expr);
  }

  return ctx->rc;
}

grn_rc
grn_obj_search(grn_ctx *ctx,
               grn_obj *obj,
               grn_obj *query,
               grn_obj *res,
               grn_operator op,
               grn_search_optarg *optarg)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (GRN_ACCESSORP(obj)) {
    grn_operator mode = GRN_OP_MATCH;
    if (optarg && optarg->mode != GRN_OP_EXACT) {
      mode = optarg->mode;
    }
    grn_obj_search_accessor_execute_data data;
    data.accessor = obj;
    data.query = query;
    data.optarg = optarg;
    rc = grn_accessor_execute(ctx,
                              obj,
                              grn_obj_search_accessor_execute,
                              &data,
                              mode,
                              res,
                              op);
  } else if (GRN_DB_OBJP(obj)) {
    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_HASH_KEY:
      {
        const void *key = GRN_BULK_HEAD(query);
        uint32_t key_size = GRN_BULK_VSIZE(query);
        bool need_cast = false;
        grn_obj casted_query;
        if (!grn_type_id_is_compatible(ctx,
                                       obj->header.domain,
                                       query->header.domain)) {
          need_cast = true;
          GRN_VALUE_FIX_SIZE_INIT(&casted_query, 0, obj->header.domain);
          if (grn_obj_cast(ctx, query, &casted_query, false) == GRN_SUCCESS) {
            key = GRN_BULK_HEAD(&casted_query);
            key_size = GRN_BULK_VSIZE(&casted_query);
          }
        }
        grn_operator mode = optarg ? optarg->mode : GRN_OP_EXACT;
        if (key && key_size) {
          if (grn_logger_pass(ctx, GRN_REPORT_INDEX_LOG_LEVEL)) {
            const char *tag;
            switch (mode) {
            case GRN_OP_EQUAL:
              tag = "[table][equal]";
              break;
            case GRN_OP_EXACT:
              tag = "[table][exact]";
              break;
            case GRN_OP_LCP:
              tag = "[table][lcp]";
              break;
            case GRN_OP_SUFFIX:
              tag = "[table][suffix]";
              break;
            case GRN_OP_PREFIX:
              tag = "[table][prefix]";
              break;
            case GRN_OP_TERM_EXTRACT:
              tag = "[table][term-extract]";
              break;
            case GRN_OP_FUZZY:
              tag = "[table][fuzzy]";
              break;
            default:
              tag = "[table][unknown]";
              break;
            }
            grn_obj_search_index_report(ctx, tag, obj);
          }
          switch (mode) {
          case GRN_OP_EQUAL:
            {
              grn_posting_internal posting = {0};
              posting.rid = grn_table_get(ctx, obj, key, key_size);
              posting.weight_float = 1.0;
              if (posting.rid == GRN_ID_NIL) {
                rc = GRN_SUCCESS;
              } else {
                rc = grn_ii_posting_add_float(ctx,
                                              (grn_posting *)(&posting),
                                              (grn_hash *)res,
                                              op);
              }
              if (rc == GRN_SUCCESS) {
                grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
              }
            }
            break;
          case GRN_OP_FUZZY:
            rc = grn_table_fuzzy_search(ctx,
                                        obj,
                                        key,
                                        key_size,
                                        &(optarg->fuzzy),
                                        res,
                                        op);
            break;
          default:
            rc = grn_table_search(ctx, obj, key, key_size, mode, res, op);
            break;
          }
        }
        if (need_cast) {
          GRN_OBJ_FIN(ctx, &casted_query);
        }
      }
      break;
    case GRN_COLUMN_INDEX:
      rc = grn_obj_search_column_index(ctx, obj, query, res, op, optarg);
      break;
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE:
      rc = grn_obj_search_column_data(ctx, obj, query, res, op, optarg);
      break;
    }
  }
  GRN_API_RETURN(rc);
}

typedef struct {
  grn_operator op;
  float weight_factor;
  grn_obj *table_dest;
  grn_obj *table_src;
  bool have_subrec;
  uint32_t value_size;
  grn_obj merge_target_vector_columns;
  grn_obj merge_target_number_columns;
} grn_table_setoperation_data;

static grn_inline void
grn_table_setoperation_data_init_value_size(grn_ctx *ctx,
                                            grn_table_setoperation_data *data)
{
  grn_obj *table_dest = data->table_dest;
  grn_obj *table_src = data->table_src;
  data->value_size = 0;
  switch (table_dest->header.type) {
  case GRN_TABLE_HASH_KEY:
    data->value_size = ((grn_hash *)table_dest)->value_size;
    break;
  case GRN_TABLE_PAT_KEY:
    data->value_size = ((grn_pat *)table_dest)->value_size;
    break;
  case GRN_TABLE_DAT_KEY:
    data->value_size = 0;
    break;
  case GRN_TABLE_NO_KEY:
    data->value_size = ((grn_array *)table_dest)->value_size;
    break;
  }
  switch (table_src->header.type) {
  case GRN_TABLE_HASH_KEY:
    if (data->value_size < ((grn_hash *)table_src)->value_size) {
      data->value_size = ((grn_hash *)table_src)->value_size;
    }
    break;
  case GRN_TABLE_PAT_KEY:
    if (data->value_size < ((grn_pat *)table_src)->value_size) {
      data->value_size = ((grn_pat *)table_src)->value_size;
    }
    break;
  case GRN_TABLE_DAT_KEY:
    data->value_size = 0;
    break;
  case GRN_TABLE_NO_KEY:
    if (data->value_size < ((grn_array *)table_src)->value_size) {
      data->value_size = ((grn_array *)table_src)->value_size;
    }
    break;
  default:
    break;
  }
}

static grn_inline void
grn_table_setoperation_data_init_merge_target_columns(
  grn_ctx *ctx, grn_table_setoperation_data *data)
{
  grn_obj *merge_target_vector_columns = &(data->merge_target_vector_columns);
  grn_obj *merge_target_number_columns = &(data->merge_target_number_columns);
  GRN_PTR_INIT(merge_target_vector_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(merge_target_number_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);

  grn_hash *columns_src = grn_table_all_columns(ctx, data->table_src);
  if (!columns_src) {
    return;
  }
  if (grn_hash_size(ctx, columns_src) == 0) {
    return;
  }

  GRN_HASH_EACH_BEGIN(ctx, columns_src, cursor, column_entry_id)
  {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    grn_id column_src_id = *((grn_id *)key);
    grn_obj *column_src = grn_ctx_at(ctx, column_src_id);
    bool is_vector_src = grn_obj_is_vector_column(ctx, column_src);
    if (!(is_vector_src ||
          grn_type_id_is_number_family(ctx, DB_OBJ(column_src)->range))) {
      grn_obj_unref(ctx, column_src);
      continue;
    }
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size =
      grn_column_name(ctx, column_src, name, GRN_TABLE_MAX_KEY_SIZE);
    grn_obj *column_dest =
      grn_table_column(ctx, data->table_dest, name, name_size);
    if (!column_dest) {
      grn_obj_unref(ctx, column_src);
      continue;
    }
    bool is_target = false;
    if (is_vector_src) {
      if (grn_obj_is_vector_column(ctx, column_dest)) {
        GRN_PTR_PUT(ctx, merge_target_vector_columns, column_dest);
        GRN_PTR_PUT(ctx, merge_target_vector_columns, column_src);
        is_target = true;
      }
    } else {
      if (DB_OBJ(column_src)->range == DB_OBJ(column_dest)->range) {
        GRN_PTR_PUT(ctx, merge_target_number_columns, column_dest);
        GRN_PTR_PUT(ctx, merge_target_number_columns, column_src);
        is_target = true;
      }
    }
    if (!is_target) {
      grn_obj_unref(ctx, column_dest);
      grn_obj_unref(ctx, column_src);
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
}

static grn_inline void
grn_table_setoperation_data_init(grn_ctx *ctx,
                                 grn_table_setoperation_data *data)
{
  grn_table_setoperation_data_init_value_size(ctx, data);
  grn_table_setoperation_data_init_merge_target_columns(ctx, data);
}

static grn_inline void
grn_table_setoperation_fin_merge_target_columns(grn_ctx *ctx,
                                                grn_obj *merge_target_columns)
{
  size_t n_elements = GRN_PTR_VECTOR_SIZE(merge_target_columns);
  size_t i;
  for (i = 0; i < n_elements; i++) {
    grn_obj *column = GRN_PTR_VALUE_AT(merge_target_columns, i);
    grn_obj_unref(ctx, column);
  }
  GRN_OBJ_FIN(ctx, merge_target_columns);
}

static grn_inline void
grn_table_setoperation_data_fin(grn_ctx *ctx, grn_table_setoperation_data *data)
{
  grn_table_setoperation_fin_merge_target_columns(
    ctx,
    &(data->merge_target_vector_columns));
  grn_table_setoperation_fin_merge_target_columns(
    ctx,
    &(data->merge_target_number_columns));
}

static grn_inline bool
grn_table_setoperation_need_merge_columns(grn_ctx *ctx,
                                          grn_table_setoperation_data *data)
{
  return (GRN_PTR_VECTOR_SIZE(&(data->merge_target_vector_columns)) > 0) ||
         (GRN_PTR_VECTOR_SIZE(&(data->merge_target_number_columns)) > 0);
}

static grn_inline void
grn_table_setoperation_merge_columns(grn_ctx *ctx,
                                     grn_table_setoperation_data *data,
                                     grn_id id_dest,
                                     grn_id id_src)
{
  {
    grn_obj *merge_target_columns = &(data->merge_target_vector_columns);
    size_t n_elements = GRN_PTR_VECTOR_SIZE(merge_target_columns);
    if (n_elements > 0) {
      size_t i;
      grn_obj buffer;
      GRN_VOID_INIT(&buffer);
      for (i = 0; i < n_elements; i += 2) {
        grn_obj *column_dest = GRN_PTR_VALUE_AT(merge_target_columns, i);
        grn_obj *column_src = GRN_PTR_VALUE_AT(merge_target_columns, i + 1);
        GRN_BULK_REWIND(&buffer);
        grn_obj_get_value(ctx, column_src, id_src, &buffer);
        if (grn_vector_size(ctx, &buffer) > 0) {
          grn_obj_set_value(ctx, column_dest, id_dest, &buffer, GRN_OBJ_APPEND);
        }
      }
      GRN_OBJ_FIN(ctx, &buffer);
    }
  }

  {
    grn_obj *merge_target_columns = &(data->merge_target_number_columns);
    size_t n_elements = GRN_PTR_VECTOR_SIZE(merge_target_columns);
    if (n_elements > 0) {
      size_t i;
      uint64_t zero = 0;
      grn_obj buffer;
      GRN_VOID_INIT(&buffer);
      for (i = 0; i < n_elements; i += 2) {
        grn_obj *column_dest = GRN_PTR_VALUE_AT(merge_target_columns, i);
        grn_obj *column_src = GRN_PTR_VALUE_AT(merge_target_columns, i + 1);
        GRN_BULK_REWIND(&buffer);
        grn_obj_get_value(ctx, column_src, id_src, &buffer);
        if (memcmp(GRN_BULK_HEAD(&buffer), &zero, GRN_BULK_VSIZE(&buffer)) !=
            0) {
          grn_obj_set_value(ctx, column_dest, id_dest, &buffer, GRN_OBJ_INCR);
        }
      }
      GRN_OBJ_FIN(ctx, &buffer);
    }
  }
}

static grn_inline void
grn_table_setoperation_or(grn_ctx *ctx, grn_table_setoperation_data *data)
{
  if (data->have_subrec) {
    GRN_TABLE_EACH_BEGIN(ctx, data->table_src, cursor, id_src)
    {
      void *key_src;
      uint32_t key_src_size;
      void *value_src;
      grn_table_cursor_get_key_value(ctx,
                                     cursor,
                                     &key_src,
                                     &key_src_size,
                                     &value_src);
      void *value_dest;
      int added;
      grn_id id_dest = grn_table_add_v_inline(ctx,
                                              data->table_dest,
                                              key_src,
                                              key_src_size,
                                              &value_dest,
                                              &added);
      if (id_dest == GRN_ID_NIL) {
        if (ctx->rc == GRN_SUCCESS) {
          continue;
        } else {
          break;
        }
      }
      if (added) {
        grn_memcpy(value_dest, value_src, data->value_size);
        grn_rset_recinfo *ri_dest = value_dest;
        ri_dest->score *= data->weight_factor;
      } else {
        grn_rset_recinfo *ri_dest = value_dest;
        grn_rset_recinfo *ri_src = value_src;
        grn_table_add_subrec(ctx,
                             data->table_dest,
                             ri_dest,
                             ri_src->score * data->weight_factor,
                             NULL,
                             0);
      }
      grn_table_setoperation_merge_columns(ctx, data, id_dest, id_src);
    }
    GRN_TABLE_EACH_END(ctx, cursor);
  } else {
    GRN_TABLE_EACH_BEGIN(ctx, data->table_src, cursor, id_src)
    {
      void *key_src;
      uint32_t key_src_size = grn_table_cursor_get_key(ctx, cursor, &key_src);
      void *value_dest;
      grn_id id_dest = grn_table_add_v_inline(ctx,
                                              data->table_dest,
                                              key_src,
                                              key_src_size,
                                              &value_dest,
                                              NULL);
      if (id_dest == GRN_ID_NIL) {
        if (ctx->rc == GRN_SUCCESS) {
          continue;
        } else {
          break;
        }
      }
      grn_table_setoperation_merge_columns(ctx, data, id_dest, id_src);
    }
    GRN_TABLE_EACH_END(ctx, cursor);
  }
}

static grn_inline void
grn_table_setoperation_and(grn_ctx *ctx, grn_table_setoperation_data *data)
{
  grn_table_delete_data delete_data;
  grn_table_delete_data_init(ctx, &delete_data, data->table_dest);
  if (data->have_subrec) {
    GRN_TABLE_EACH_BEGIN(ctx, data->table_dest, cursor, id_dest)
    {
      void *key_dest;
      uint32_t key_dest_size;
      void *value_dest;
      grn_table_cursor_get_key_value(ctx,
                                     cursor,
                                     &key_dest,
                                     &key_dest_size,
                                     &value_dest);
      void *value_src;
      grn_id id_src = grn_table_get_v(ctx,
                                      data->table_src,
                                      key_dest,
                                      key_dest_size,
                                      &value_src);
      if (id_src == GRN_ID_NIL) {
        delete_data.id = id_dest;
        delete_data.key = key_dest;
        delete_data.key_size = key_dest_size;
        grn_table_delete_by_id_without_lock(ctx, &delete_data);
      } else {
        grn_rset_recinfo *ri_dest = value_dest;
        grn_rset_recinfo *ri_src = value_src;
        ri_dest->score += ri_src->score * data->weight_factor;
        grn_table_setoperation_merge_columns(ctx, data, id_dest, id_src);
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
  } else {
    GRN_TABLE_EACH_BEGIN(ctx, data->table_dest, cursor, id_dest)
    {
      void *key_dest;
      uint32_t key_dest_size = grn_table_cursor_get_key(ctx, cursor, &key_dest);
      grn_id id_src =
        grn_table_get(ctx, data->table_src, key_dest, key_dest_size);
      if (id_src == GRN_ID_NIL) {
        delete_data.id = id_dest;
        delete_data.key = key_dest;
        delete_data.key_size = key_dest_size;
        grn_table_delete_by_id_without_lock(ctx, &delete_data);
      } else {
        grn_table_setoperation_merge_columns(ctx, data, id_dest, id_src);
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
  }
  grn_table_delete_data_fin(ctx, &delete_data);
}

static grn_inline void
grn_table_setoperation_and_not(grn_ctx *ctx, grn_table_setoperation_data *data)
{
  GRN_TABLE_EACH_BEGIN(ctx, data->table_src, cursor, id_src)
  {
    void *key_src;
    uint32_t key_src_size = grn_table_cursor_get_key(ctx, cursor, &key_src);
    grn_table_delete(ctx, data->table_dest, key_src, key_src_size);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static grn_inline void
grn_table_setoperation_adjust(grn_ctx *ctx, grn_table_setoperation_data *data)
{
  if (data->have_subrec) {
    GRN_TABLE_EACH_BEGIN(ctx, data->table_src, cursor, id_src)
    {
      void *key_src;
      uint32_t key_src_size;
      void *value_src;
      grn_table_cursor_get_key_value(ctx,
                                     cursor,
                                     &key_src,
                                     &key_src_size,
                                     &value_src);
      void *value_dest;
      grn_id id_dest = grn_table_get_v(ctx,
                                       data->table_dest,
                                       key_src,
                                       key_src_size,
                                       &value_dest);
      if (id_dest != GRN_ID_NIL) {
        grn_rset_recinfo *ri_dest = value_dest;
        grn_rset_recinfo *ri_src = value_src;
        ri_dest->score += ri_src->score * data->weight_factor;
        grn_table_setoperation_merge_columns(ctx, data, id_dest, id_src);
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
  } else {
    if (!grn_table_setoperation_need_merge_columns(ctx, data)) {
      return;
    }
    GRN_TABLE_EACH_BEGIN(ctx, data->table_src, cursor, id_src)
    {
      void *key_src;
      uint32_t key_src_size = grn_table_cursor_get_key(ctx, cursor, &key_src);
      void *value_dest;
      grn_id id_dest = grn_table_get_v(ctx,
                                       data->table_dest,
                                       key_src,
                                       key_src_size,
                                       &value_dest);
      if (id_dest != GRN_ID_NIL) {
        grn_table_setoperation_merge_columns(ctx, data, id_dest, id_src);
      }
    }
    GRN_TABLE_EACH_END(ctx, cursor);
  }
}

grn_rc
grn_table_setoperation(
  grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res, grn_operator op)
{
  return grn_table_setoperation_with_weight_factor(ctx,
                                                   table1,
                                                   table2,
                                                   res,
                                                   op,
                                                   1.0);
}

grn_rc
grn_table_setoperation_with_weight_factor(grn_ctx *ctx,
                                          grn_obj *table1,
                                          grn_obj *table2,
                                          grn_obj *res,
                                          grn_operator op,
                                          float weight_factor)
{
  GRN_API_ENTER;
  if (!table1) {
    ERR(GRN_INVALID_ARGUMENT, "[table][setoperation] table1 is NULL");
    GRN_API_RETURN(ctx->rc);
  }
  if (!table2) {
    ERR(GRN_INVALID_ARGUMENT, "[table][setoperation] table2 is NULL");
    GRN_API_RETURN(ctx->rc);
  }
  if (!res) {
    ERR(GRN_INVALID_ARGUMENT, "[table][setoperation] result table is NULL");
    GRN_API_RETURN(ctx->rc);
  }

  grn_table_setoperation_data data;
  data.op = op;
  data.weight_factor = weight_factor;
  if (table1 == res) {
    data.table_src = table2;
    data.table_dest = table1;
  } else {
    if (table2 == res) {
      data.table_src = table1;
      data.table_dest = table2;
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "[table][setoperation] table1 or table2 must be result table");
      GRN_API_RETURN(ctx->rc);
    }
  }
  data.have_subrec =
    ((DB_OBJ(data.table_src)->header.flags & GRN_OBJ_WITH_SUBREC) &&
     (DB_OBJ(data.table_dest)->header.flags & GRN_OBJ_WITH_SUBREC));
  grn_table_setoperation_data_init(ctx, &data);
  switch (op) {
  case GRN_OP_OR:
    grn_table_setoperation_or(ctx, &data);
    break;
  case GRN_OP_AND:
    grn_table_setoperation_and(ctx, &data);
    break;
  case GRN_OP_AND_NOT:
    grn_table_setoperation_and_not(ctx, &data);
    break;
  case GRN_OP_ADJUST:
    grn_table_setoperation_adjust(ctx, &data);
    break;
  default:
    break;
  }
  grn_table_setoperation_data_fin(ctx, &data);
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_table_difference(
  grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res1, grn_obj *res2)
{
  void *key = NULL;
  uint32_t key_size = 0;
  if (table1 != res1 || table2 != res2) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_table_delete_data data1;
  grn_table_delete_data_init(ctx, &data1, table1);
  grn_table_delete_data data2;
  grn_table_delete_data_init(ctx, &data2, table2);
  if (grn_table_size(ctx, table1) > grn_table_size(ctx, table2)) {
    GRN_TABLE_EACH(ctx, table2, 0, 0, id, &key, &key_size, NULL, {
      grn_id id1;
      if ((id1 = grn_table_get(ctx, table1, key, key_size))) {
        data1.id = id1;
        data1.key = key;
        data1.key_size = key_size;
        grn_table_delete_by_id_without_lock(ctx, &data1);
        data2.id = id;
        data2.key = key;
        data2.key_size = key_size;
        grn_table_delete_by_id_without_lock(ctx, &data2);
      }
    });
  } else {
    GRN_TABLE_EACH(ctx, table1, 0, 0, id, &key, &key_size, NULL, {
      grn_id id2;
      if ((id2 = grn_table_get(ctx, table2, key, key_size))) {
        data1.id = id;
        data1.key = key;
        data1.key_size = key_size;
        grn_table_delete_by_id_without_lock(ctx, &data1);
        data2.id = id2;
        data2.key = key;
        data2.key_size = key_size;
        grn_table_delete_by_id_without_lock(ctx, &data2);
      }
    });
  }
  grn_table_delete_data_fin(ctx, &data1);
  grn_table_delete_data_fin(ctx, &data2);
  return GRN_SUCCESS;
}

static grn_obj *
grn_obj_get_accessor(grn_ctx *ctx,
                     grn_obj *obj,
                     const char *name,
                     uint32_t name_size);

static grn_obj *
grn_obj_column_(grn_ctx *ctx,
                grn_obj *table,
                const char *name,
                uint32_t name_size)
{
  grn_id table_id = DB_OBJ(table)->id;
  grn_obj *column = NULL;

  if (table_id & GRN_OBJ_TMP_OBJECT) {
    char column_name[GRN_TABLE_MAX_KEY_SIZE];
    void *value = NULL;
    grn_snprintf(column_name,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "%u%c%.*s",
                 table_id,
                 GRN_DB_DELIMITER,
                 (int)name_size,
                 name);
    grn_ctx *target_ctx = ctx;
    while (target_ctx->impl->parent) {
      target_ctx = target_ctx->impl->parent;
    }
    grn_pat_get(target_ctx,
                target_ctx->impl->temporary_columns,
                column_name,
                strlen(column_name),
                &value);
    if (value) {
      column = *((grn_obj **)value);
      DB_OBJ(column)->reference_count++;
    }
  } else {
    char buf[GRN_TABLE_MAX_KEY_SIZE];
    int len = grn_obj_name(ctx, table, buf, GRN_TABLE_MAX_KEY_SIZE);
    if (len) {
      buf[len++] = GRN_DB_DELIMITER;
      if (len + name_size <= GRN_TABLE_MAX_KEY_SIZE) {
        grn_obj alias_name_buffer;
        grn_memcpy(buf + len, name, name_size);
        GRN_TEXT_INIT(&alias_name_buffer, 0);
        column = grn_alias_resolve(ctx,
                                   buf,
                                   len + name_size,
                                   &alias_name_buffer,
                                   GRN_TRUE);
        if (!column && GRN_TEXT_LEN(&alias_name_buffer) > 0) {
          const size_t delimiter_len = 1;
          const size_t table_name_len = len - delimiter_len;
          const char *alias_name = GRN_TEXT_VALUE(&alias_name_buffer);
          size_t alias_name_size = GRN_TEXT_LEN(&alias_name_buffer);
          if (alias_name_size > (size_t)len &&
              alias_name[table_name_len] == GRN_DB_DELIMITER &&
              strncmp(alias_name, buf, table_name_len) == 0) {
            alias_name += len;
            alias_name_size -= len;
          }
          column =
            grn_obj_get_accessor(ctx, table, alias_name, alias_name_size);
        }
        GRN_OBJ_FIN(ctx, &alias_name_buffer);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "name is too long");
      }
    }
  }

  return column;
}

grn_obj *
grn_obj_column(grn_ctx *ctx,
               grn_obj *table,
               const char *name,
               uint32_t name_size)
{
  grn_obj *column = NULL;
  GRN_API_ENTER;
  if (GRN_OBJ_TABLEP(table)) {
    if (grn_db_check_name(ctx, name, name_size) == GRN_SUCCESS) {
      column = grn_obj_column_(ctx, table, name, name_size);
    }
    if (!column) {
      column = grn_obj_get_accessor(ctx, table, name, name_size);
    }
  } else if (GRN_ACCESSORP(table)) {
    column = grn_obj_get_accessor(ctx, table, name, name_size);
  }
  GRN_API_RETURN(column);
}

int
grn_table_columns(grn_ctx *ctx,
                  grn_obj *table,
                  const char *name,
                  unsigned int name_size,
                  grn_obj *res)
{
  int n = 0;
  grn_id id;

  GRN_API_ENTER;

  if (!GRN_OBJ_TABLEP(table)) {
    GRN_API_RETURN(n);
  }

  id = DB_OBJ(table)->id;

  if (id == GRN_ID_NIL) {
    GRN_API_RETURN(n);
  }

  if (id & GRN_OBJ_TMP_OBJECT) {
    char search_key[GRN_TABLE_MAX_KEY_SIZE];
    grn_pat_cursor *cursor;
    grn_snprintf(search_key,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "%u%c%.*s",
                 id,
                 GRN_DB_DELIMITER,
                 name_size,
                 name);
    grn_ctx *target_ctx = ctx;
    while (target_ctx->impl->parent) {
      target_ctx = target_ctx->impl->parent;
    }
    cursor = grn_pat_cursor_open(target_ctx,
                                 target_ctx->impl->temporary_columns,
                                 search_key,
                                 strlen(search_key),
                                 NULL,
                                 0,
                                 0,
                                 -1,
                                 GRN_CURSOR_PREFIX);
    if (cursor) {
      grn_id column_id;
      while ((column_id = grn_pat_cursor_next(target_ctx, cursor)) !=
             GRN_ID_NIL) {
        column_id |= GRN_OBJ_TMP_OBJECT | GRN_OBJ_TMP_COLUMN;
        grn_hash_add(ctx,
                     (grn_hash *)res,
                     &column_id,
                     sizeof(grn_id),
                     NULL,
                     NULL);
        n++;
      }
      grn_pat_cursor_close(target_ctx, cursor);
    }
  } else {
    grn_db *s = (grn_db *)DB_OBJ(table)->db;
    if (s->keys) {
      grn_obj bulk;
      GRN_TEXT_INIT(&bulk, 0);
      grn_table_get_key2(ctx, s->keys, id, &bulk);
      GRN_TEXT_PUTC(ctx, &bulk, GRN_DB_DELIMITER);
      grn_bulk_write(ctx, &bulk, name, name_size);
      grn_table_search(ctx,
                       s->keys,
                       GRN_BULK_HEAD(&bulk),
                       GRN_BULK_VSIZE(&bulk),
                       GRN_OP_PREFIX,
                       res,
                       GRN_OP_OR);
      grn_obj_close(ctx, &bulk);
      n = grn_table_size(ctx, res);
    }
  }

  GRN_API_RETURN(n);
}

const char *
_grn_table_key(grn_ctx *ctx, grn_obj *table, grn_id id, uint32_t *key_size)
{
  GRN_ASSERT(table);
  if (table->header.type == GRN_DB) {
    table = ((grn_db *)table)->keys;
  }
  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
    return _grn_hash_key(ctx, (grn_hash *)table, id, key_size);
  case GRN_TABLE_PAT_KEY:
    return _grn_pat_key(ctx, (grn_pat *)table, id, key_size);
  case GRN_TABLE_DAT_KEY:
    return _grn_dat_key(ctx, (grn_dat *)table, id, key_size);
  case GRN_TABLE_NO_KEY:
    {
      grn_array *a = (grn_array *)table;
      const char *v;
      if (a->obj.header.domain && a->value_size &&
          (v = _grn_array_get_value(ctx, a, id))) {
        *key_size = a->value_size;
        return v;
      } else {
        *key_size = 0;
      }
    }
    break;
  }
  return NULL;
}

/* column */

grn_obj *
grn_column_create(grn_ctx *ctx,
                  grn_obj *table,
                  const char *name,
                  unsigned int name_size,
                  const char *path,
                  grn_column_flags flags,
                  grn_obj *type)
{
  grn_db *s;
  uint32_t value_size;
  grn_obj *db, *res = NULL;
  grn_id id = GRN_ID_NIL;
  grn_id range = GRN_ID_NIL;
  grn_id domain = GRN_ID_NIL;
  grn_bool is_persistent_table;
  char fullname[GRN_TABLE_MAX_KEY_SIZE];
  unsigned int fullname_size;
  char buffer[PATH_MAX];

  GRN_API_ENTER;
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT, "[column][create] table is missing");
    goto exit;
  }
  if (!type) {
    ERR(GRN_INVALID_ARGUMENT, "[column][create] type is missing");
    goto exit;
  }
  if (!name || !name_size) {
    ERR(GRN_INVALID_ARGUMENT, "[column][create] name is missing");
    goto exit;
  }
  db = DB_OBJ(table)->db;
  s = (grn_db *)db;
  if (!GRN_DB_P(s)) {
    int table_name_len;
    char table_name[GRN_TABLE_MAX_KEY_SIZE];
    table_name_len =
      grn_obj_name(ctx, table, table_name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_INVALID_ARGUMENT,
        "[column][create] invalid db assigned: <%.*s>.<%.*s>",
        table_name_len,
        table_name,
        name_size,
        name);
    goto exit;
  }

  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR("[column][create]", name, name_size);
    goto exit;
  }

  domain = DB_OBJ(table)->id;
  is_persistent_table = !(domain & GRN_OBJ_TMP_OBJECT);

  if (!domain) {
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[column][create] [todo] table-less column isn't supported yet");
    goto exit;
  }

  {
    int table_name_len;
    if (is_persistent_table) {
      table_name_len = grn_table_get_key(ctx,
                                         s->keys,
                                         domain,
                                         fullname,
                                         GRN_TABLE_MAX_KEY_SIZE);
    } else {
      grn_snprintf(fullname,
                   GRN_TABLE_MAX_KEY_SIZE,
                   GRN_TABLE_MAX_KEY_SIZE,
                   "%u",
                   domain);
      table_name_len = strlen(fullname);
    }
    if (name_size + 1 + table_name_len > GRN_TABLE_MAX_KEY_SIZE) {
      ERR(GRN_INVALID_ARGUMENT,
          "[column][create] too long column name: required name_size(%d) < %d"
          ": <%.*s>.<%.*s>",
          name_size,
          GRN_TABLE_MAX_KEY_SIZE - 1 - table_name_len,
          table_name_len,
          fullname,
          name_size,
          name);
      goto exit;
    }
    fullname[table_name_len] = GRN_DB_DELIMITER;
    grn_memcpy(fullname + table_name_len + 1, name, name_size);
    fullname_size = table_name_len + 1 + name_size;
  }

  range = DB_OBJ(type)->id;
  switch (type->header.type) {
  case GRN_TYPE:
    {
      grn_db_obj *t = (grn_db_obj *)type;
      flags |= t->header.flags & ~GRN_OBJ_KEY_MASK;
      value_size = GRN_TYPE_SIZE(t);
    }
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    value_size = sizeof(grn_id);
    break;
  default:
    /*
    if (type == grn_type_any) {
      value_size = sizeof(grn_id) + sizeof(grn_id);
    }
    */
    value_size = sizeof(grn_id);
  }

  if (is_persistent_table) {
    id = grn_obj_register(ctx, db, fullname, fullname_size);
    if (ERRP(ctx, GRN_ERROR)) {
      goto exit;
    }

    {
      uint32_t table_name_size = 0;
      const char *table_name;
      table_name = _grn_table_key(ctx, ctx->impl->db, domain, &table_name_size);
      GRN_LOG(ctx,
              GRN_LOG_NOTICE,
              "DDL:%u:column_create %.*s %.*s",
              id,
              table_name_size,
              table_name,
              name_size,
              name);
    }
  } else {
    grn_ctx *target_ctx = ctx;
    while (target_ctx->impl->parent) {
      target_ctx = target_ctx->impl->parent;
    }
    if (target_ctx != ctx) {
      CRITICAL_SECTION_ENTER(target_ctx->impl->temporary_objects_lock);
    }
    int added;
    id = grn_pat_add(target_ctx,
                     target_ctx->impl->temporary_columns,
                     fullname,
                     fullname_size,
                     NULL,
                     &added);
    if (target_ctx != ctx) {
      CRITICAL_SECTION_LEAVE(target_ctx->impl->temporary_objects_lock);
    }
    if (!id) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[column][create][temporary] "
          "failed to register temporary column name: <%.*s>",
          fullname_size,
          fullname);
      goto exit;
    } else if (!added) {
      id = GRN_ID_NIL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[column][create][temporary] already used name was assigned: <%.*s>",
          fullname_size,
          fullname);
      goto exit;
    }
    id |= GRN_OBJ_TMP_OBJECT | GRN_OBJ_TMP_COLUMN;
  }

  if (is_persistent_table && flags & GRN_OBJ_PERSISTENT) {
    if (!path) {
      if (GRN_DB_PERSISTENT_P(db)) {
        grn_db_generate_pathname(ctx, db, id, buffer);
        path = buffer;
      } else {
        int table_name_len;
        char table_name[GRN_TABLE_MAX_KEY_SIZE];
        table_name_len =
          grn_obj_name(ctx, table, table_name, GRN_TABLE_MAX_KEY_SIZE);
        ERR(GRN_INVALID_ARGUMENT,
            "[column][create] path not assigned for persistent column"
            ": <%.*s>.<%.*s>",
            table_name_len,
            table_name,
            name_size,
            name);
        goto exit;
      }
    } else {
      flags |= GRN_OBJ_CUSTOM_NAME;
    }
  } else {
    if (path) {
      int table_name_len;
      char table_name[GRN_TABLE_MAX_KEY_SIZE];
      table_name_len =
        grn_obj_name(ctx, table, table_name, GRN_TABLE_MAX_KEY_SIZE);
      ERR(GRN_INVALID_ARGUMENT,
          "[column][create] path assigned for temporary column"
          ": <%.*s>.<%.*s>",
          table_name_len,
          table_name,
          name_size,
          name);
      goto exit;
    }
  }
  switch (flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_SCALAR:
    if ((flags & GRN_OBJ_KEY_VAR_SIZE) || value_size > sizeof(int64_t)) {
      res = (grn_obj *)grn_ja_create(ctx, path, value_size, flags);
    } else {
      res = (grn_obj *)grn_ra_create(ctx, path, value_size, flags);
    }
    break;
  case GRN_OBJ_COLUMN_VECTOR:
    res = (grn_obj *)grn_ja_create(ctx, path, value_size * 30 /*todo*/, flags);
    // todo : zlib support
    break;
  case GRN_OBJ_COLUMN_INDEX:
    res = (grn_obj *)
      grn_ii_create(ctx, path, table, flags); // todo : ii layout support
    break;
  }
  if (res) {
    DB_OBJ(res)->header.domain = domain;
    DB_OBJ(res)->header.impl_flags = 0;
    DB_OBJ(res)->range = range;
    DB_OBJ(res)->header.flags = flags;
    res->header.flags = flags;
    if (grn_db_obj_init(ctx, db, id, DB_OBJ(res)) != GRN_SUCCESS) {
      grn_obj_remove_internal(ctx, res, 0);
      res = NULL;
      goto exit;
    }
    if (grn_obj_is_persistent(ctx, res)) {
      grn_obj *space;
      space = ctx->impl->temporary_open_spaces.current;
      if (space) {
        GRN_PTR_PUT(ctx, space, res);
      }
    }
    grn_ctx_impl_columns_cache_delete(ctx, domain);
    grn_obj_touch(ctx, res, NULL);
  }
exit:
  if (!res && id) {
    grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_column_create_similar(grn_ctx *ctx,
                          grn_obj *table,
                          const char *name,
                          uint32_t name_size,
                          const char *path,
                          grn_obj *base_column)
{
  GRN_API_ENTER;
  grn_obj *column = grn_column_create_similar_id_map(ctx,
                                                     table,
                                                     name,
                                                     name_size,
                                                     path,
                                                     base_column,
                                                     NULL);
  GRN_API_RETURN(column);
}

grn_obj *
grn_column_open(grn_ctx *ctx,
                grn_obj *table,
                const char *name,
                unsigned int name_size,
                const char *path,
                grn_obj *type)
{
  grn_id domain;
  grn_obj *res = NULL;
  grn_db *s;
  char fullname[GRN_TABLE_MAX_KEY_SIZE];
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
    GRN_DB_CHECK_NAME_ERR("[column][open]", name, name_size);
    goto exit;
  }
  if ((domain = DB_OBJ(table)->id)) {
    int len =
      grn_table_get_key(ctx, s->keys, domain, fullname, GRN_TABLE_MAX_KEY_SIZE);
    if (name_size + 1 + len > GRN_TABLE_MAX_KEY_SIZE) {
      ERR(GRN_INVALID_ARGUMENT, "too long column name");
      goto exit;
    }
    fullname[len] = GRN_DB_DELIMITER;
    grn_memcpy(fullname + len + 1, name, name_size);
    name_size += len + 1;
  } else {
    ERR(GRN_INVALID_ARGUMENT, "todo : not supported yet");
    goto exit;
  }
  res = grn_ctx_get(ctx, fullname, name_size);
  if (res) {
    const char *path2 = grn_obj_path(ctx, res);
    if (path && (!path2 || strcmp(path, path2))) {
      goto exit;
    }
  } else if (path) {
    uint32_t dbtype = grn_io_detect_type(ctx, path);
    if (!dbtype) {
      goto exit;
    }
    switch (dbtype) {
    case GRN_COLUMN_VAR_SIZE:
      res = (grn_obj *)grn_ja_open(ctx, path);
      break;
    case GRN_COLUMN_FIX_SIZE:
      res = (grn_obj *)grn_ra_open(ctx, path);
      break;
    case GRN_COLUMN_INDEX:
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
exit:
  GRN_API_RETURN(res);
}

/**** accessor ****/

static grn_accessor *
grn_accessor_new_key(grn_ctx *ctx, grn_obj *table)
{
  grn_accessor *accessor = grn_accessor_new(ctx);
  if (!accessor) {
    return accessor;
  }
  grn_obj_refer(ctx, table);
  accessor->obj = table;
  accessor->action = GRN_ACCESSOR_GET_KEY;
  return accessor;
}

grn_inline static void
grn_accessor_refer(grn_ctx *ctx, grn_obj *accessor)
{
  ((grn_accessor *)accessor)->reference_count++;
}

grn_inline static grn_bool
grn_obj_get_accessor_rset_value(grn_ctx *ctx,
                                grn_obj *obj,
                                grn_accessor **res,
                                uint8_t action)
{
  grn_bool succeeded = GRN_FALSE;
  grn_accessor **rp;

  for (rp = res; GRN_TRUE; rp = &(*rp)->next) {
    *rp = grn_accessor_new(ctx);
    (*rp)->obj = obj;

#define CHECK_GROUP_CALC_FLAG(flag)                                            \
  do {                                                                         \
    if (GRN_TABLE_IS_GROUPED(obj)) {                                           \
      grn_table_group_flags flags;                                             \
      flags = DB_OBJ(obj)->group.flags;                                        \
      if (flags & flag) {                                                      \
        succeeded = GRN_TRUE;                                                  \
        (*rp)->action = action;                                                \
        goto exit;                                                             \
      }                                                                        \
    }                                                                          \
  } while (GRN_FALSE)
    switch (action) {
    case GRN_ACCESSOR_GET_SCORE:
      if (DB_OBJ(obj)->header.flags & GRN_OBJ_WITH_SUBREC) {
        (*rp)->action = action;
        succeeded = GRN_TRUE;
        goto exit;
      }
      break;
    case GRN_ACCESSOR_GET_MAX:
      CHECK_GROUP_CALC_FLAG(GRN_TABLE_GROUP_CALC_MAX);
      break;
    case GRN_ACCESSOR_GET_MIN:
      CHECK_GROUP_CALC_FLAG(GRN_TABLE_GROUP_CALC_MIN);
      break;
    case GRN_ACCESSOR_GET_SUM:
      CHECK_GROUP_CALC_FLAG(GRN_TABLE_GROUP_CALC_SUM);
      break;
    case GRN_ACCESSOR_GET_AVG:
      CHECK_GROUP_CALC_FLAG(GRN_TABLE_GROUP_CALC_AVG);
      break;
    case GRN_ACCESSOR_GET_MEAN:
      CHECK_GROUP_CALC_FLAG(GRN_TABLE_GROUP_CALC_MEAN);
      break;
    case GRN_ACCESSOR_GET_NSUBRECS:
      if (GRN_TABLE_IS_GROUPED(obj)) {
        (*rp)->action = action;
        succeeded = GRN_TRUE;
        goto exit;
      }
      break;
    }
#undef CHECK_GROUP_CALC_FLAG

    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_HASH_KEY:
      (*rp)->action = GRN_ACCESSOR_GET_KEY;
      break;
    case GRN_TABLE_NO_KEY:
      if (!obj->header.domain) {
        goto exit;
      }
      (*rp)->action = GRN_ACCESSOR_GET_VALUE;
      break;
    default:
      /* lookup failed */
      goto exit;
    }
    if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
      goto exit;
    }
  }

exit:
  if (!succeeded) {
    grn_obj_close(ctx, (grn_obj *)*res);
    *res = NULL;
  }

  return succeeded;
}

static grn_obj *
grn_obj_get_accessor(grn_ctx *ctx,
                     grn_obj *obj,
                     const char *name,
                     uint32_t name_size)
{
  grn_accessor *res = NULL, **rp = NULL, **rp0 = NULL;
  grn_bool is_chained = GRN_FALSE;
  bool obj_is_referred = false;
  if (!obj) {
    return NULL;
  }
  GRN_API_ENTER;
  if (obj->header.type == GRN_ACCESSOR) {
    is_chained = GRN_TRUE;
    for (rp0 = (grn_accessor **)&obj; *rp0; rp0 = &(*rp0)->next) {
      res = *rp0;
    }
    switch (res->action) {
    case GRN_ACCESSOR_GET_KEY:
      obj = grn_ctx_at(ctx, res->obj->header.domain);
      obj_is_referred = true;
      break;
    case GRN_ACCESSOR_GET_VALUE:
    case GRN_ACCESSOR_GET_SCORE:
    case GRN_ACCESSOR_GET_NSUBRECS:
    case GRN_ACCESSOR_GET_MAX:
    case GRN_ACCESSOR_GET_MIN:
    case GRN_ACCESSOR_GET_SUM:
    case GRN_ACCESSOR_GET_AVG:
    case GRN_ACCESSOR_GET_MEAN:
      obj = grn_ctx_at(ctx, DB_OBJ(res->obj)->range);
      obj_is_referred = true;
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE:
      obj = grn_ctx_at(ctx, DB_OBJ(res->obj)->range);
      obj_is_referred = true;
      break;
    case GRN_ACCESSOR_LOOKUP:
      /* todo */
      break;
    case GRN_ACCESSOR_FUNCALL:
      /* todo */
      break;
    }
  }
  if (!obj) {
    res = NULL;
    goto exit;
  }
  {
    size_t len;
    const char *sp, *se = name + name_size;
    if (*name == GRN_DB_DELIMITER) {
      name++;
    }
    for (sp = name; (len = grn_charlen(ctx, sp, se)); sp += len) {
      if (*sp == GRN_DB_DELIMITER) {
        break;
      }
    }
    if (!(len = sp - name)) {
      goto exit;
    }
    if (*name == GRN_DB_PSEUDO_COLUMN_PREFIX) { /* pseudo column */
      int done = 0;
      if (len < 2) {
        goto exit;
      }
      switch (name[1]) {
      case 'k': /* key */
        if (len != GRN_COLUMN_NAME_KEY_LEN ||
            memcmp(name, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN)) {
          goto exit;
        }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = grn_accessor_new(ctx);
          if (!obj_is_referred) {
            grn_obj_refer(ctx, obj);
            obj_is_referred = true;
          }
          (*rp)->obj = obj;
          if (GRN_TABLE_IS_MULTI_KEYS_GROUPED(obj)) {
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            done++;
            break;
          }
          if (!(obj = grn_ctx_at(ctx, obj->header.domain))) {
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            goto exit;
          }
          switch (obj->header.type) {
          case GRN_DB:
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            rp = &(*rp)->next;
            *rp = grn_accessor_new(ctx);
            (*rp)->obj = obj;
            (*rp)->action = GRN_ACCESSOR_GET_DB_OBJ;
            done++;
            break;
          case GRN_TYPE:
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            done++;
            break;
          case GRN_TABLE_PAT_KEY:
          case GRN_TABLE_DAT_KEY:
          case GRN_TABLE_HASH_KEY:
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            break;
          case GRN_TABLE_NO_KEY:
            if (obj->header.domain) {
              (*rp)->action = GRN_ACCESSOR_GET_VALUE;
              break;
            }
            /* fallthru */
          default:
            /* lookup failed */
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            grn_obj_unref(ctx, obj);
            goto exit;
          }
        }
        break;
      case 'i': /* id */
        if (len != GRN_COLUMN_NAME_ID_LEN ||
            memcmp(name, GRN_COLUMN_NAME_ID, GRN_COLUMN_NAME_ID_LEN)) {
          goto exit;
        }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = grn_accessor_new(ctx);
          if (!obj_is_referred) {
            grn_obj_refer(ctx, obj);
            obj_is_referred = true;
          }
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
            case GRN_DB:
            case GRN_TYPE:
              (*rp)->action = GRN_ACCESSOR_GET_ID;
              done++;
              break;
            case GRN_TABLE_PAT_KEY:
            case GRN_TABLE_DAT_KEY:
            case GRN_TABLE_HASH_KEY:
            case GRN_TABLE_NO_KEY:
              (*rp)->action = GRN_ACCESSOR_GET_KEY;
              break;
            default:
              /* lookup failed */
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              grn_obj_unref(ctx, obj);
              goto exit;
            }
          }
        }
        break;
      case 'v': /* value */
        if (len != GRN_COLUMN_NAME_VALUE_LEN ||
            memcmp(name, GRN_COLUMN_NAME_VALUE, GRN_COLUMN_NAME_VALUE_LEN)) {
          goto exit;
        }
        for (rp = &res; !done; rp = &(*rp)->next) {
          *rp = grn_accessor_new(ctx);
          if (!obj_is_referred) {
            grn_obj_refer(ctx, obj);
            obj_is_referred = true;
          }
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
            case GRN_TYPE:
              if (DB_OBJ((*rp)->obj)->range) {
                (*rp)->action = GRN_ACCESSOR_GET_VALUE;
                done++;
              } else {
                grn_obj_close(ctx, (grn_obj *)res);
                res = NULL;
                goto exit;
              }
              break;
            case GRN_TABLE_PAT_KEY:
            case GRN_TABLE_DAT_KEY:
            case GRN_TABLE_HASH_KEY:
            case GRN_TABLE_NO_KEY:
              (*rp)->action = GRN_ACCESSOR_GET_KEY;
              break;
            default:
              /* lookup failed */
              grn_obj_close(ctx, (grn_obj *)res);
              res = NULL;
              grn_obj_unref(ctx, obj);
              goto exit;
            }
          }
        }
        break;
      case 's': /* score, sum */
        if (!obj_is_referred) {
          grn_obj_refer(ctx, obj);
          obj_is_referred = true;
        }
        if (len == GRN_COLUMN_NAME_SCORE_LEN &&
            memcmp(name, GRN_COLUMN_NAME_SCORE, GRN_COLUMN_NAME_SCORE_LEN) ==
              0) {
          if (!grn_obj_get_accessor_rset_value(ctx,
                                               obj,
                                               &res,
                                               GRN_ACCESSOR_GET_SCORE)) {
            goto exit;
          }
        } else if (len == GRN_COLUMN_NAME_SUM_LEN &&
                   memcmp(name, GRN_COLUMN_NAME_SUM, GRN_COLUMN_NAME_SUM_LEN) ==
                     0) {
          if (!grn_obj_get_accessor_rset_value(ctx,
                                               obj,
                                               &res,
                                               GRN_ACCESSOR_GET_SUM)) {
            goto exit;
          }
        } else {
          grn_obj_unref(ctx, obj);
          goto exit;
        }
        break;
      case 'n': /* nsubrecs */
        if (len != GRN_COLUMN_NAME_NSUBRECS_LEN ||
            memcmp(name,
                   GRN_COLUMN_NAME_NSUBRECS,
                   GRN_COLUMN_NAME_NSUBRECS_LEN)) {
          goto exit;
        }
        if (!obj_is_referred) {
          grn_obj_refer(ctx, obj);
          obj_is_referred = true;
        }
        if (!grn_obj_get_accessor_rset_value(ctx,
                                             obj,
                                             &res,
                                             GRN_ACCESSOR_GET_NSUBRECS)) {
          goto exit;
        }
        break;
      case 'm': /* max, mean, min */
        if (!obj_is_referred) {
          grn_obj_refer(ctx, obj);
          obj_is_referred = true;
        }
        if (len == GRN_COLUMN_NAME_MAX_LEN &&
            memcmp(name, GRN_COLUMN_NAME_MAX, GRN_COLUMN_NAME_MAX_LEN) == 0) {
          if (!grn_obj_get_accessor_rset_value(ctx,
                                               obj,
                                               &res,
                                               GRN_ACCESSOR_GET_MAX)) {
            goto exit;
          }
        } else if (len == GRN_COLUMN_NAME_MEAN_LEN &&
                   memcmp(name,
                          GRN_COLUMN_NAME_MEAN,
                          GRN_COLUMN_NAME_MEAN_LEN) == 0) {
          if (!grn_obj_get_accessor_rset_value(ctx,
                                               obj,
                                               &res,
                                               GRN_ACCESSOR_GET_MEAN)) {
            goto exit;
          }
        } else if (len == GRN_COLUMN_NAME_MIN_LEN &&
                   memcmp(name, GRN_COLUMN_NAME_MIN, GRN_COLUMN_NAME_MIN_LEN) ==
                     0) {
          if (!grn_obj_get_accessor_rset_value(ctx,
                                               obj,
                                               &res,
                                               GRN_ACCESSOR_GET_MIN)) {
            goto exit;
          }
        } else {
          grn_obj_unref(ctx, obj);
          goto exit;
        }
        break;
      case 'a': /* avg */
        if (!obj_is_referred) {
          grn_obj_refer(ctx, obj);
          obj_is_referred = true;
        }
        if (len == GRN_COLUMN_NAME_AVG_LEN &&
            memcmp(name, GRN_COLUMN_NAME_AVG, GRN_COLUMN_NAME_AVG_LEN) == 0) {
          if (!grn_obj_get_accessor_rset_value(ctx,
                                               obj,
                                               &res,
                                               GRN_ACCESSOR_GET_AVG)) {
            goto exit;
          }
        } else {
          grn_obj_unref(ctx, obj);
          goto exit;
        }
        break;
      default:
        res = NULL;
        goto exit;
      }
    } else {
      /* if obj->header.type == GRN_TYPE ... lookup table */
      for (rp = &res;; rp = &(*rp)->next) {
        grn_obj *column = grn_obj_column_(ctx, obj, name, len);
        if (column) {
          *rp = grn_accessor_new(ctx);
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
          if (obj_is_referred) {
            grn_obj_unref(ctx, obj);
            obj_is_referred = false;
          }
          break;
        } else {
          grn_id next_obj_id;
          next_obj_id = obj->header.domain;
          if (!next_obj_id) {
            // ERR(GRN_INVALID_ARGUMENT, "no such column: <%s>", name);
            if (obj_is_referred) {
              grn_obj_unref(ctx, obj);
            }
            if (!is_chained) {
              grn_obj_close(ctx, (grn_obj *)res);
            }
            res = NULL;
            goto exit;
          }
          *rp = grn_accessor_new(ctx);
          if (!obj_is_referred) {
            grn_obj_refer(ctx, obj);
            obj_is_referred = true;
          }
          (*rp)->obj = obj;
          obj = grn_ctx_at(ctx, next_obj_id);
          if (!obj) {
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            goto exit;
          }
          switch (obj->header.type) {
          case GRN_TABLE_PAT_KEY:
          case GRN_TABLE_DAT_KEY:
          case GRN_TABLE_HASH_KEY:
          case GRN_TABLE_NO_KEY:
            (*rp)->action = GRN_ACCESSOR_GET_KEY;
            break;
          default:
            /* lookup failed */
            grn_obj_close(ctx, (grn_obj *)res);
            res = NULL;
            grn_obj_unref(ctx, obj);
            goto exit;
          }
        }
      }
    }
    if (sp != se) {
      if (!grn_obj_get_accessor(ctx, (grn_obj *)res, sp, se - sp)) {
        if (!is_chained) {
          grn_obj_close(ctx, (grn_obj *)res);
          res = NULL;
          goto exit;
        }
      }
    }
  }
  if (rp0) {
    *rp0 = res;
  }
exit:
  GRN_API_RETURN((grn_obj *)res);
}

grn_inline static bool
grn_column_is_vector(grn_ctx *ctx, grn_obj *column)
{
  grn_obj_flags type;

  if (column->header.type != GRN_COLUMN_VAR_SIZE) {
    return false;
  }

  type = column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK;
  return type == GRN_OBJ_COLUMN_VECTOR;
}

grn_inline static bool
grn_column_is_index(grn_ctx *ctx, grn_obj *column)
{
  grn_obj_flags type;

  if (column->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)column; a; a = a->next) {
      if (a->next) {
        continue;
      }
      if (a->action != GRN_ACCESSOR_GET_COLUMN_VALUE) {
        return false;
      }

      column = a->obj;
    }
  }

  if (column->header.type != GRN_COLUMN_INDEX) {
    return false;
  }

  type = column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK;
  return type == GRN_OBJ_COLUMN_INDEX;
}

void
grn_obj_get_range_info(grn_ctx *ctx,
                       grn_obj *obj,
                       grn_id *range_id,
                       grn_obj_flags *range_flags)
{
  *range_flags = 0;
  if (!obj) {
    *range_id = GRN_ID_NIL;
  } else if (grn_obj_is_expr(ctx, obj)) {
    grn_expr_get_range_info(ctx, obj, range_id, range_flags);
  } else if (grn_obj_is_proc(ctx, obj)) {
    /* TODO */
    *range_id = GRN_ID_NIL;
  } else if (GRN_DB_OBJP(obj)) {
    *range_id = DB_OBJ(obj)->range;
    if (grn_column_is_vector(ctx, obj)) {
      *range_flags |= GRN_OBJ_VECTOR;
    }
    if (grn_obj_is_weight_vector_column(ctx, obj)) {
      *range_flags |= GRN_OBJ_WITH_WEIGHT;
    }
  } else if (obj->header.type == GRN_ACCESSOR) {
    bool is_vector = false;
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID:
        *range_id = GRN_DB_UINT32;
        break;
      case GRN_ACCESSOR_GET_VALUE:
        if (GRN_DB_OBJP(a->obj)) {
          *range_id = DB_OBJ(a->obj)->range;
        }
        break;
      case GRN_ACCESSOR_GET_SCORE:
        *range_id = GRN_DB_FLOAT;
        break;
      case GRN_ACCESSOR_GET_NSUBRECS:
        *range_id = GRN_DB_INT32;
        break;
      case GRN_ACCESSOR_GET_MAX:
      case GRN_ACCESSOR_GET_MIN:
      case GRN_ACCESSOR_GET_SUM:
        *range_id = DB_OBJ(a->obj)->group.aggregated_value_type_id;
        break;
      case GRN_ACCESSOR_GET_AVG:
      case GRN_ACCESSOR_GET_MEAN:
        *range_id = GRN_DB_FLOAT;
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE:
        grn_obj_get_range_info(ctx, a->obj, range_id, range_flags);
        if (*range_flags & GRN_OBJ_VECTOR) {
          is_vector = true;
        }
        if (is_vector) {
          *range_flags |= GRN_OBJ_VECTOR;
        }
        break;
      case GRN_ACCESSOR_GET_KEY:
        if (GRN_DB_OBJP(a->obj)) {
          *range_id = DB_OBJ(a->obj)->header.domain;
        }
        break;
      default:
        if (GRN_DB_OBJP(a->obj)) {
          *range_id = DB_OBJ(a->obj)->range;
        }
        break;
      }
    }
  }
}

grn_id
grn_obj_get_range(grn_ctx *ctx, grn_obj *obj)
{
  grn_id range_id = GRN_ID_NIL;
  grn_obj_flags range_flags = 0;

  grn_obj_get_range_info(ctx, obj, &range_id, &range_flags);

  return range_id;
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
      case GRN_ACCESSOR_GET_SCORE:
      case GRN_ACCESSOR_GET_NSUBRECS:
      case GRN_ACCESSOR_GET_MAX:
      case GRN_ACCESSOR_GET_MIN:
      case GRN_ACCESSOR_GET_SUM:
      case GRN_ACCESSOR_GET_AVG:
      case GRN_ACCESSOR_GET_MEAN:
        res = 0;
        break;
      case GRN_ACCESSOR_GET_ID:
      case GRN_ACCESSOR_GET_VALUE:
      case GRN_ACCESSOR_GET_COLUMN_VALUE:
      case GRN_ACCESSOR_GET_KEY:
        if (GRN_DB_OBJP(a->obj)) {
          res = IS_TEMP(obj) ? 0 : 1;
        }
        break;
      default:
        if (GRN_DB_OBJP(a->obj)) {
          res = IS_TEMP(obj) ? 0 : 1;
        }
        break;
      }
    }
  }
  return res;
}

const char *
grn_accessor_get_value_(grn_ctx *ctx,
                        grn_accessor *a,
                        grn_id id,
                        uint32_t *size)
{
  const char *value = NULL;
  for (;;) {
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID:
      value = (const char *)(uintptr_t)id;
      *size = GRN_OBJ_GET_VALUE_IMD;
      break;
    case GRN_ACCESSOR_GET_KEY:
      value = _grn_table_key(ctx, a->obj, id, size);
      break;
    case GRN_ACCESSOR_GET_VALUE:
      value = grn_obj_get_value_(ctx, a->obj, id, size);
      break;
    case GRN_ACCESSOR_GET_SCORE:
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value = (const char *)&((grn_rset_recinfo *)value)->score;
        *size = sizeof(double);
      }
      break;
    case GRN_ACCESSOR_GET_NSUBRECS:
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value = (const char *)&((grn_rset_recinfo *)value)->n_subrecs;
        *size = sizeof(int);
      }
      break;
    case GRN_ACCESSOR_GET_MAX:
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value =
          (const char *)grn_rset_recinfo_get_max_(ctx,
                                                  (grn_rset_recinfo *)value,
                                                  a->obj);
        *size = GRN_RSET_MAX_SIZE;
      }
      break;
    case GRN_ACCESSOR_GET_MIN:
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value =
          (const char *)grn_rset_recinfo_get_min_(ctx,
                                                  (grn_rset_recinfo *)value,
                                                  a->obj);
        *size = GRN_RSET_MIN_SIZE;
      }
      break;
    case GRN_ACCESSOR_GET_SUM:
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value =
          (const char *)grn_rset_recinfo_get_sum_(ctx,
                                                  (grn_rset_recinfo *)value,
                                                  a->obj);
        *size = GRN_RSET_SUM_SIZE;
      }
      break;
    case GRN_ACCESSOR_GET_AVG:
    case GRN_ACCESSOR_GET_MEAN:
      if ((value = grn_obj_get_value_(ctx, a->obj, id, size))) {
        value =
          (const char *)grn_rset_recinfo_get_mean_(ctx,
                                                   (grn_rset_recinfo *)value,
                                                   a->obj);
        *size = GRN_RSET_MEAN_SIZE;
      }
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE:
      /* todo : support vector */
      value = grn_obj_get_value_(ctx, a->obj, id, size);
      break;
    case GRN_ACCESSOR_GET_DB_OBJ:
      value = _grn_table_key(ctx, ((grn_db *)ctx->impl->db)->keys, id, size);
      break;
    case GRN_ACCESSOR_LOOKUP:
      /* todo */
      break;
    case GRN_ACCESSOR_FUNCALL:
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
    if (!(value = grn_obj_open(ctx, GRN_BULK, 0, 0))) {
      return NULL;
    }
  } else {
    value->header.type = GRN_BULK;
  }
  size0 = GRN_BULK_VSIZE(value);
  for (;;) {
    grn_bulk_truncate(ctx, value, size0);
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID:
      GRN_UINT32_PUT(ctx, value, id);
      value->header.domain = GRN_DB_UINT32;
      vp = GRN_BULK_HEAD(value) + size0;
      vs = GRN_BULK_VSIZE(value) - size0;
      break;
    case GRN_ACCESSOR_GET_KEY:
      if (!a->next && GRN_TABLE_IS_MULTI_KEYS_GROUPED(a->obj)) {
        grn_obj_ensure_vector(ctx, value);
        if (id) {
          grn_obj raw_vector;
          GRN_TEXT_INIT(&raw_vector, 0);
          grn_table_get_key2(ctx, a->obj, id, &raw_vector);
          grn_vector_unpack(ctx,
                            value,
                            GRN_BULK_HEAD(&raw_vector),
                            GRN_BULK_VSIZE(&raw_vector),
                            0,
                            NULL);
          GRN_OBJ_FIN(ctx, &raw_vector);
        }
        vp = NULL;
        vs = 0;
      } else {
        if (id) {
          grn_table_get_key2(ctx, a->obj, id, value);
          vp = GRN_BULK_HEAD(value) + size0;
          vs = GRN_BULK_VSIZE(value) - size0;
        } else {
          vp = NULL;
          vs = 0;
        }
        value->header.domain = a->obj->header.domain;
      }
      break;
    case GRN_ACCESSOR_GET_VALUE:
      grn_obj_get_value(ctx, a->obj, id, value);
      vp = GRN_BULK_HEAD(value) + size0;
      vs = GRN_BULK_VSIZE(value) - size0;
      break;
    case GRN_ACCESSOR_GET_SCORE:
      {
        double score = grn_table_get_score(ctx, a->obj, id);
        GRN_FLOAT_PUT(ctx, value, score);
      }
      value->header.domain = GRN_DB_FLOAT;
      break;
    case GRN_ACCESSOR_GET_NSUBRECS:
      if (id) {
        grn_rset_recinfo *ri =
          (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
        GRN_INT32_PUT(ctx, value, ri->n_subrecs);
      } else {
        GRN_INT32_PUT(ctx, value, 0);
      }
      value->header.domain = GRN_DB_INT32;
      break;
    case GRN_ACCESSOR_GET_MAX:
      value->header.domain = DB_OBJ(a->obj)->group.aggregated_value_type_id;
      {
        grn_rset_aggregated_value max = {0};
        if (id != GRN_ID_NIL) {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          max = grn_rset_recinfo_get_max(ctx, ri, a->obj);
        }
        if (value->header.domain == GRN_DB_INT64) {
          GRN_INT64_PUT(ctx, value, max.value_int64);
        } else {
          GRN_FLOAT_PUT(ctx, value, max.value_double);
        }
      }
      break;
    case GRN_ACCESSOR_GET_MIN:
      value->header.domain = DB_OBJ(a->obj)->group.aggregated_value_type_id;
      {
        grn_rset_aggregated_value min = {0};
        if (id != GRN_ID_NIL) {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          min = grn_rset_recinfo_get_min(ctx, ri, a->obj);
        }
        if (value->header.domain == GRN_DB_INT64) {
          GRN_INT64_PUT(ctx, value, min.value_int64);
        } else {
          GRN_FLOAT_PUT(ctx, value, min.value_double);
        }
      }
      break;
    case GRN_ACCESSOR_GET_SUM:
      value->header.domain = DB_OBJ(a->obj)->group.aggregated_value_type_id;
      {
        grn_rset_aggregated_value sum = {0};
        if (id != GRN_ID_NIL) {
          grn_rset_recinfo *ri =
            (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
          sum = grn_rset_recinfo_get_sum(ctx, ri, a->obj);
        }
        if (value->header.domain == GRN_DB_INT64) {
          GRN_INT64_PUT(ctx, value, sum.value_int64);
        } else {
          GRN_FLOAT_PUT(ctx, value, sum.value_double);
        }
      }
      break;
    case GRN_ACCESSOR_GET_AVG:
    case GRN_ACCESSOR_GET_MEAN:
      if (id) {
        grn_rset_recinfo *ri =
          (grn_rset_recinfo *)grn_obj_get_value_(ctx, a->obj, id, &vs);
        double mean;
        mean = grn_rset_recinfo_get_mean(ctx, ri, a->obj);
        GRN_FLOAT_PUT(ctx, value, mean);
      } else {
        GRN_FLOAT_PUT(ctx, value, 0.0);
      }
      value->header.domain = GRN_DB_FLOAT;
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE:
      if (grn_obj_is_index_column(ctx, a->obj) && a->next) {
        grn_id sub_value_range = grn_obj_get_range(ctx, (grn_obj *)(a->next));
        grn_obj_reinit(ctx, value, sub_value_range, GRN_OBJ_VECTOR);
        grn_obj sub_value;
        GRN_VOID_INIT(&sub_value);
        grn_ii *ii = (grn_ii *)(a->obj);
        grn_ii_cursor *cursor =
          grn_ii_cursor_open(ctx,
                             ii,
                             id,
                             GRN_ID_NIL,
                             GRN_ID_MAX,
                             grn_ii_get_n_elements(ctx, ii),
                             0);
        if (cursor) {
          grn_posting *posting;
          while ((posting = grn_ii_cursor_next(ctx, cursor))) {
            GRN_BULK_REWIND(&sub_value);
            grn_accessor_get_value(ctx, a->next, posting->rid, &sub_value);
            if (value->header.type == GRN_UVECTOR) {
              grn_bulk_write(ctx,
                             value,
                             GRN_BULK_HEAD(&sub_value),
                             GRN_BULK_VSIZE(&sub_value));
            } else {
              grn_vector_add_element(ctx,
                                     value,
                                     GRN_BULK_HEAD(&sub_value),
                                     GRN_BULK_VSIZE(&sub_value),
                                     0,
                                     sub_value.header.domain);
            }
          }
          grn_ii_cursor_close(ctx, cursor);
        }
        GRN_OBJ_FIN(ctx, &sub_value);
        return value;
      } else {
        grn_obj_get_value(ctx, a->obj, id, value);
        if (value->header.type == GRN_UVECTOR && a->next) {
          int i, n;
          grn_id *sub_ids;
          grn_obj sub_records;
          grn_obj sub_value;
          grn_id sub_value_range;

          n = (GRN_BULK_VSIZE(value) - size0) / sizeof(grn_id);
          sub_ids = (grn_id *)(GRN_BULK_HEAD(value) + size0);
          GRN_RECORD_INIT(&sub_records, GRN_OBJ_VECTOR, value->header.domain);
          for (i = 0; i < n; i++) {
            GRN_RECORD_PUT(ctx, &sub_records, sub_ids[i]);
          }

          sub_value_range = grn_obj_get_range(ctx, (grn_obj *)(a->next));
          grn_obj_reinit(ctx, value, sub_value_range, GRN_OBJ_VECTOR);
          GRN_VOID_INIT(&sub_value);
          for (i = 0; i < n; i++) {
            grn_id sub_id = GRN_RECORD_VALUE_AT(&sub_records, i);
            GRN_BULK_REWIND(&sub_value);
            grn_accessor_get_value(ctx, a->next, sub_id, &sub_value);
            if (value->header.type == GRN_UVECTOR) {
              grn_bulk_write(ctx,
                             value,
                             GRN_BULK_HEAD(&sub_value),
                             GRN_BULK_VSIZE(&sub_value));
            } else {
              grn_vector_add_element(ctx,
                                     value,
                                     GRN_BULK_HEAD(&sub_value),
                                     GRN_BULK_VSIZE(&sub_value),
                                     0,
                                     sub_value.header.domain);
            }
          }
          GRN_OBJ_FIN(ctx, &sub_value);
          GRN_OBJ_FIN(ctx, &sub_records);
          return value;
        } else {
          vp = GRN_BULK_HEAD(value) + size0;
          vs = GRN_BULK_VSIZE(value) - size0;
        }
      }
      break;
    case GRN_ACCESSOR_GET_DB_OBJ:
      value = grn_ctx_at(ctx, id);
      grn_obj_close(ctx, value);
      return value;
      break;
    case GRN_ACCESSOR_LOOKUP:
      /* todo */
      break;
    case GRN_ACCESSOR_FUNCALL:
      /* todo */
      break;
    }
    if ((a = a->next)) {
      if (vs > 0) {
        id = *((grn_id *)vp);
      } else {
        id = GRN_ID_NIL;
      }
    } else {
      break;
    }
  }
  return value;
}

static grn_rset_aggregated_value
grn_accessor_set_value_get_rset_aggregated_value(grn_ctx *ctx,
                                                 grn_obj *table,
                                                 grn_obj *value)
{
  grn_rset_aggregated_value aggregated_value = {0};
  grn_id type_id = DB_OBJ(table)->group.aggregated_value_type_id;
  if (type_id == GRN_DB_INT64) {
    if (value->header.type == GRN_DB_INT64) {
      aggregated_value.value_int64 = GRN_INT64_VALUE(value);
    } else {
      grn_obj value_int64;
      GRN_INT64_INIT(&value_int64, 0);
      if (grn_obj_cast(ctx, value, &value_int64, false) == GRN_SUCCESS) {
        aggregated_value.value_int64 = GRN_INT64_VALUE(&value_int64);
      }
      GRN_OBJ_FIN(ctx, &value_int64);
    }
  } else {
    if (value->header.type == GRN_DB_FLOAT) {
      aggregated_value.value_double = GRN_FLOAT_VALUE(value);
    } else {
      grn_obj value_float;
      GRN_FLOAT_INIT(&value_float, 0);
      if (grn_obj_cast(ctx, value, &value_float, false) == GRN_SUCCESS) {
        aggregated_value.value_double = GRN_FLOAT_VALUE(&value_float);
      }
      GRN_OBJ_FIN(ctx, &value_float);
    }
  }
  return aggregated_value;
}

static grn_rc
grn_accessor_set_value(
  grn_ctx *ctx, grn_accessor *a, grn_id id, grn_obj *value, int flags)
{
  grn_rc rc = GRN_SUCCESS;
  if (!value) {
    value = grn_obj_open(ctx, GRN_BULK, 0, 0);
  }
  if (value) {
    grn_obj buf;
    void *vp = NULL;
    GRN_TEXT_INIT(&buf, 0);
    for (;;) {
      GRN_BULK_REWIND(&buf);
      switch (a->action) {
      case GRN_ACCESSOR_GET_KEY:
        grn_table_get_key2(ctx, a->obj, id, &buf);
        vp = GRN_BULK_HEAD(&buf);
        break;
      case GRN_ACCESSOR_GET_VALUE:
        if (a->next) {
          grn_obj_get_value(ctx, a->obj, id, &buf);
          vp = GRN_BULK_HEAD(&buf);
        } else {
          rc = grn_obj_set_value(ctx, a->obj, id, value, flags);
        }
        break;
      case GRN_ACCESSOR_GET_SCORE:
        {
          grn_rset_recinfo *ri;
          if (a->next) {
            grn_obj_get_value(ctx, a->obj, id, &buf);
            ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
            vp = &ri->score;
          } else {
            uint32_t size;
            if ((ri = (grn_rset_recinfo *)
                   grn_obj_get_value_(ctx, a->obj, id, &size))) {
              // todo : flags support
              if (value->header.domain == GRN_DB_FLOAT) {
                ri->score = GRN_FLOAT_VALUE(value);
              } else {
                grn_obj buf;
                GRN_FLOAT_INIT(&buf, 0);
                grn_obj_cast(ctx, value, &buf, GRN_FALSE);
                ri->score = GRN_FLOAT_VALUE(&buf);
                GRN_OBJ_FIN(ctx, &buf);
              }
            }
          }
        }
        break;
      case GRN_ACCESSOR_GET_NSUBRECS:
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          vp = &ri->n_subrecs;
        }
        break;
      case GRN_ACCESSOR_GET_MAX:
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          grn_rset_aggregated_value max =
            grn_accessor_set_value_get_rset_aggregated_value(ctx,
                                                             a->obj,
                                                             value);
          grn_rset_recinfo_set_max(ctx, ri, a->obj, max);
        }
        break;
      case GRN_ACCESSOR_GET_MIN:
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          grn_rset_aggregated_value min =
            grn_accessor_set_value_get_rset_aggregated_value(ctx,
                                                             a->obj,
                                                             value);
          grn_rset_recinfo_set_min(ctx, ri, a->obj, min);
        }
        break;
      case GRN_ACCESSOR_GET_SUM:
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          grn_rset_aggregated_value sum =
            grn_accessor_set_value_get_rset_aggregated_value(ctx,
                                                             a->obj,
                                                             value);
          grn_rset_recinfo_set_sum(ctx, ri, a->obj, sum);
        }
        break;
      case GRN_ACCESSOR_GET_AVG:
      case GRN_ACCESSOR_GET_MEAN:
        grn_obj_get_value(ctx, a->obj, id, &buf);
        {
          grn_rset_recinfo *ri = (grn_rset_recinfo *)GRN_BULK_HEAD(&buf);
          if (value->header.type == GRN_DB_FLOAT) {
            grn_rset_recinfo_set_mean(ctx, ri, a->obj, GRN_FLOAT_VALUE(value));
          } else {
            grn_obj value_float;
            GRN_FLOAT_INIT(&value_float, 0);
            if (!grn_obj_cast(ctx, value, &value_float, GRN_FALSE)) {
              grn_rset_recinfo_set_mean(ctx,
                                        ri,
                                        a->obj,
                                        GRN_FLOAT_VALUE(&value_float));
            }
            GRN_OBJ_FIN(ctx, &value_float);
          }
        }
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE:
        /* todo : support vector */
        if (a->next) {
          grn_obj_get_value(ctx, a->obj, id, &buf);
          vp = GRN_BULK_HEAD(&buf);
        } else {
          rc = grn_obj_set_value(ctx, a->obj, id, value, flags);
        }
        break;
      case GRN_ACCESSOR_LOOKUP:
        /* todo */
        break;
      case GRN_ACCESSOR_FUNCALL:
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

uint32_t
grn_obj_size(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return 0;
  }
  switch (obj->header.type) {
  case GRN_VOID:
  case GRN_BULK:
  case GRN_PTR:
  case GRN_UVECTOR:
  case GRN_PVECTOR:
  case GRN_MSG:
    return GRN_BULK_VSIZE(obj);
  case GRN_VECTOR:
    return obj->u.v.body ? GRN_BULK_VSIZE(obj->u.v.body) : 0;
  default:
    return 0;
  }
}

static inline bool
is_same_value(grn_ctx *ctx, grn_obj *value1, grn_obj *value2)
{
  void *v1 = GRN_BULK_HEAD(value1);
  uint32_t size1 = grn_obj_size(ctx, value1);
  void *v2 = GRN_BULK_HEAD(value2);
  uint32_t size2 = grn_obj_size(ctx, value2);
  if (size1 != size2) {
    return false;
  }
  if (memcmp(v1, v2, size1) != 0) {
    return false;
  }
  uint32_t n_elements1 = 0;
  if (value1 && value1->header.type == GRN_VECTOR) {
    n_elements1 = grn_vector_size(ctx, value1);
  }
  uint32_t n_elements2 = 0;
  if (value2 && value2->header.type == GRN_VECTOR) {
    n_elements2 = grn_vector_size(ctx, value2);
  }
  if (n_elements1 != n_elements2) {
    return false;
  }
  return true;
}

grn_inline static bool
call_hook(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  grn_hook *hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET];
  if (hooks || obj->header.type == GRN_COLUMN_VAR_SIZE) {
    grn_obj oldbuf, *oldvalue;
    GRN_TEXT_INIT(&oldbuf, 0);
    oldvalue = grn_obj_get_value(ctx, obj, id, &oldbuf);
    if (flags & GRN_OBJ_SET) {
      bool need_update = true;
      if (is_same_value(ctx, value, oldvalue)) {
        if (obj->header.type == GRN_COLUMN_FIX_SIZE) {
          /* We don't need to update the same value but fixed zero
           * value must be updated because we can't distinct no value
           * and zero value. For example, grn_obj_get_value() for
           * Int32 scalar column returns 0 as the initial value. */
          need_update = grn_bulk_is_zero(ctx, value);
        } else {
          /* We don't need to update the same value. */
          need_update = false;
        }
      }
      if (!need_update) {
        grn_obj_close(ctx, oldvalue);
        return false;
      }
    }
    if (hooks) {
      // todo : grn_proc_ctx_open()
      grn_obj id_, flags_;
      grn_proc_ctx pctx;
      grn_proc_ctx_init(&pctx, hooks, 4, 4);
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
          grn_obj_default_set_value_hook(ctx, 1, &obj, &pctx.user_data);
        }
        if (ctx->rc) {
          grn_obj_close(ctx, oldvalue);
          return false;
        }
        hooks = hooks->next;
        pctx.offset++;
      }
      GRN_OBJ_FIN(ctx, &id_);
      GRN_OBJ_FIN(ctx, &flags_);
    }
    grn_obj_close(ctx, oldvalue);
  }
  return true;
}

static grn_rc
grn_obj_set_value_table_pat_key(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_id range = DB_OBJ(obj)->range;
  void *v = GRN_BULK_HEAD(value);
  grn_obj buf;

  if (!call_hook(ctx, obj, id, value, flags)) {
    if (ctx->rc) {
      rc = ctx->rc;
    }
    return rc;
  }

  if (range != value->header.domain) {
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
    if (grn_obj_cast(ctx, value, &buf, GRN_TRUE) == GRN_SUCCESS) {
      v = GRN_BULK_HEAD(&buf);
    }
  }
  rc = grn_pat_set_value(ctx, (grn_pat *)obj, id, v, flags);
  if (range != value->header.domain) {
    grn_obj_close(ctx, &buf);
  }

  return rc;
}

static grn_rc
grn_obj_set_value_table_hash_key(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_id range = DB_OBJ(obj)->range;
  void *v = GRN_BULK_HEAD(value);
  grn_obj buf;

  if (!call_hook(ctx, obj, id, value, flags)) {
    if (ctx->rc) {
      rc = ctx->rc;
    }
    return rc;
  }

  if (range != value->header.domain) {
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
    if (grn_obj_cast(ctx, value, &buf, GRN_TRUE) == GRN_SUCCESS) {
      v = GRN_BULK_HEAD(&buf);
    }
  }
  rc = grn_hash_set_value(ctx, (grn_hash *)obj, id, v, flags);
  if (range != value->header.domain) {
    grn_obj_close(ctx, &buf);
  }

  return rc;
}

static grn_rc
grn_obj_set_value_table_no_key(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_id range = DB_OBJ(obj)->range;
  void *v = GRN_BULK_HEAD(value);
  grn_obj buf;

  if (!call_hook(ctx, obj, id, value, flags)) {
    if (ctx->rc) {
      rc = ctx->rc;
    }
    return rc;
  }

  if (range != value->header.domain) {
    GRN_OBJ_INIT(&buf, GRN_BULK, 0, range);
    if (grn_obj_cast(ctx, value, &buf, GRN_TRUE) == GRN_SUCCESS) {
      v = GRN_BULK_HEAD(&buf);
    }
  }
  rc = grn_array_set_value(ctx, (grn_array *)obj, id, v, flags);
  if (range != value->header.domain) {
    grn_obj_close(ctx, &buf);
  }

  return rc;
}

static grn_rc
grn_obj_set_value_column_var_size(
  grn_ctx *ctx, grn_obj *column, grn_id id, grn_obj *value, int flags)
{
  grn_ja *ja = (grn_ja *)column;
  grn_obj buffer;
  GRN_VOID_INIT(&buffer);
  grn_obj *casted_value = grn_ja_cast_value(ctx, ja, value, &buffer, flags);
  grn_rc rc = ctx->rc;
  if (casted_value) {
    if (call_hook(ctx, column, id, casted_value, flags)) {
      switch (casted_value->header.type) {
      case GRN_VOID:
        rc = grn_ja_put(ctx, ja, id, NULL, 0, flags, NULL);
        break;
      case GRN_VECTOR:
        rc = grn_ja_putv(ctx, ja, id, casted_value, flags);
        break;
      default:
        {
          grn_rc pack_rc = GRN_SUCCESS;
          if (casted_value == &buffer) {
            pack_rc = grn_ja_pack_value(ctx, ja, casted_value, flags);
          }
          if (pack_rc == GRN_SUCCESS) {
            rc = grn_ja_put(ctx,
                            ja,
                            id,
                            GRN_BULK_HEAD(casted_value),
                            GRN_BULK_VSIZE(casted_value),
                            flags,
                            NULL);
          } else {
            rc = pack_rc;
          }
        }
        break;
      }
    } else {
      if (ctx->rc != GRN_SUCCESS) {
        rc = ctx->rc;
      }
    }
  }
  GRN_OBJ_FIN(ctx, &buffer);
  return rc;
}

static grn_rc
grn_obj_set_value_column_fix_size(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj buffer;
  GRN_VOID_INIT(&buffer);
  grn_obj *value_ =
    grn_ra_cast_value(ctx, (grn_ra *)obj, value, &buffer, flags);
  if (!value_) {
    goto exit;
  }
  uint32_t value_size = GRN_BULK_VSIZE(value_);
  uint32_t element_size = ((grn_ra *)obj)->header->element_size;
  if (value_size > element_size) {
    GRN_DEFINE_NAME(obj);
    ERR(GRN_INVALID_ARGUMENT,
        "[column][fix][set-value][%.*s] too long value: <%u>: max:<%u>",
        name_size,
        name,
        value_size,
        element_size);
    goto exit;
  }
  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_SET:
  case GRN_OBJ_INCR:
  case GRN_OBJ_DECR:
    if (!call_hook(ctx, obj, id, value_, flags)) {
      if (ctx->rc != GRN_SUCCESS) {
        rc = ctx->rc;
        goto exit;
      }
    }
    rc = grn_ra_set_value(ctx, (grn_ra *)obj, id, value_, flags);
    break;
  default:
    rc = GRN_OPERATION_NOT_SUPPORTED;
    {
      GRN_DEFINE_NAME(obj);
      ERR(rc,
          "[column][fix][set-value][%.*s] unsupported set mode: <%s>(%u)",
          name_size,
          name,
          grn_obj_set_flag_to_string(flags),
          flags & GRN_OBJ_SET_MASK);
    }
    break;
  }
exit:
  GRN_OBJ_FIN(ctx, &buffer);
  return rc;
}

static grn_rc
grn_obj_set_value_column_index(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
{
  char column_name[GRN_TABLE_MAX_KEY_SIZE];
  int column_name_size;
  column_name_size =
    grn_obj_name(ctx, obj, column_name, GRN_TABLE_MAX_KEY_SIZE);
  ERR(GRN_INVALID_ARGUMENT,
      "can't set value to index column directly: <%.*s>",
      column_name_size,
      column_name);
  return ctx->rc;
}

grn_rc
grn_obj_set_value(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)
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
    GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY:
      rc = grn_obj_set_value_table_pat_key(ctx, obj, id, value, flags);
      break;
    case GRN_TABLE_DAT_KEY:
      rc = GRN_OPERATION_NOT_SUPPORTED;
      break;
    case GRN_TABLE_HASH_KEY:
      rc = grn_obj_set_value_table_hash_key(ctx, obj, id, value, flags);
      break;
    case GRN_TABLE_NO_KEY:
      rc = grn_obj_set_value_table_no_key(ctx, obj, id, value, flags);
      break;
    case GRN_COLUMN_VAR_SIZE:
      rc = grn_obj_set_value_column_var_size(ctx, obj, id, value, flags);
      break;
    case GRN_COLUMN_FIX_SIZE:
      rc = grn_obj_set_value_column_fix_size(ctx, obj, id, value, flags);
      break;
    case GRN_COLUMN_INDEX:
      rc = grn_obj_set_value_column_index(ctx, obj, id, value, flags);
      break;
    default:
      break;
    }
    GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
    {
      GRN_DEFINE_NAME(obj);
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "[obj][set-value][slow][%f] "
              "<%.*s>(%u)",
              elapsed_time,
              name_size,
              name,
              id);
    }
    GRN_SLOW_LOG_POP_END(ctx);
  }
  GRN_API_RETURN(rc);
}

const char *
grn_obj_get_value_(grn_ctx *ctx, grn_obj *obj, grn_id id, uint32_t *size)
{
  const char *value = NULL;
  *size = 0;
  switch (obj->header.type) {
  case GRN_ACCESSOR:
    value = grn_accessor_get_value_(ctx, (grn_accessor *)obj, id, size);
    break;
  case GRN_TABLE_PAT_KEY:
    value = grn_pat_get_value_(ctx, (grn_pat *)obj, id, size);
    break;
  case GRN_TABLE_DAT_KEY:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "GRN_TABLE_DAT_KEY not supported");
    break;
  case GRN_TABLE_HASH_KEY:
    value = grn_hash_get_value_(ctx, (grn_hash *)obj, id, size);
    break;
  case GRN_TABLE_NO_KEY:
    if ((value = _grn_array_get_value(ctx, (grn_array *)obj, id))) {
      *size = ((grn_array *)obj)->value_size;
    }
    break;
  case GRN_COLUMN_VAR_SIZE:
    {
      grn_io_win jw;
      if ((value = grn_ja_ref(ctx, (grn_ja *)obj, id, &jw, size))) {
        grn_ja_unref(ctx, &jw);
      }
    }
    break;
  case GRN_COLUMN_FIX_SIZE:
    if ((value = grn_ra_ref(ctx, (grn_ra *)obj, id))) {
      grn_ra_unref(ctx, (grn_ra *)obj, id);
      *size = ((grn_ra *)obj)->header->element_size;
    }
    break;
  case GRN_COLUMN_INDEX:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "todo: GRN_COLUMN_INDEX");
    break;
  }
  return value;
}

static void
grn_obj_get_value_expr(grn_ctx *ctx, grn_obj *expr, grn_id id, grn_obj *value)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *code;

  if (e->codes_curr != 1) {
    return;
  }

  code = e->codes;
  if (code->op != GRN_OP_GET_VALUE) {
    return;
  }

  if (!code->value) {
    return;
  }

  switch (code->value->header.type) {
  case GRN_ACCESSOR:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_FIX_SIZE:
    grn_obj_get_value(ctx, code->value, id, value);
    break;
  default:
    break;
  }
}

static void
grn_obj_get_value_column_index(grn_ctx *ctx,
                               grn_obj *index_column,
                               grn_id id,
                               grn_obj *value)
{
  grn_ii *ii = (grn_ii *)index_column;
  grn_obj_ensure_bulk(ctx, value);
  if (id) {
    GRN_UINT32_SET(ctx, value, grn_ii_estimate_size(ctx, ii, id));
  } else {
    GRN_UINT32_SET(ctx, value, 0);
  }
  value->header.domain = GRN_DB_UINT32;
}

static grn_obj *
grn_obj_get_value_column_vector(grn_ctx *ctx,
                                grn_obj *obj,
                                grn_id id,
                                grn_obj *value)
{
  bool is_var_size_element =
    grn_type_id_is_text_family(ctx, DB_OBJ(obj)->range);
  if (is_var_size_element) {
    grn_obj_ensure_vector(ctx, value);
    value->header.domain = DB_OBJ(obj)->range;
    if (id) {
      grn_ja_get_value(ctx, (grn_ja *)obj, id, value);
    }
  } else {
    grn_obj_ensure_bulk(ctx, value);
    value->header.type = GRN_UVECTOR;
    value->header.domain = DB_OBJ(obj)->range;
    grn_column_flags flags = grn_column_get_flags(ctx, obj);
    if (flags & GRN_OBJ_WITH_WEIGHT) {
      value->header.flags |= GRN_OBJ_WITH_WEIGHT;
    }
    if (id) {
      grn_ja_get_value(ctx, (grn_ja *)obj, id, value);
    }
  }

  return value;
}

grn_obj *
grn_obj_get_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value)
{
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
  case GRN_VOID:
    grn_obj_reinit(ctx, value, GRN_DB_TEXT, 0);
    break;
  case GRN_BULK:
  case GRN_VECTOR:
  case GRN_UVECTOR:
  case GRN_MSG:
    break;
  default:
    ERR(GRN_INVALID_ARGUMENT, "grn_obj_get_value failed");
    goto exit;
  }
  switch (obj->header.type) {
  case GRN_ACCESSOR:
    grn_obj_ensure_bulk(ctx, value);
    value = grn_accessor_get_value(ctx, (grn_accessor *)obj, id, value);
    break;
  case GRN_EXPR:
    grn_obj_get_value_expr(ctx, obj, id, value);
    break;
  case GRN_TABLE_PAT_KEY:
    {
      grn_pat *pat = (grn_pat *)obj;
      uint32_t size = pat->value_size;
      grn_obj_ensure_bulk(ctx, value);
      if (id) {
        if (grn_bulk_space(ctx, value, size)) {
          MERR("grn_bulk_space failed");
          goto exit;
        }
        {
          char *curr = GRN_BULK_CURR(value);
          grn_pat_get_value(ctx, pat, id, curr - size);
        }
      }
      value->header.type = GRN_BULK;
      value->header.domain = grn_obj_get_range(ctx, obj);
    }
    break;
  case GRN_TABLE_DAT_KEY:
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "GRN_TABLE_DAT_KEY not supported");
    break;
  case GRN_TABLE_HASH_KEY:
    {
      grn_bool processed = GRN_FALSE;
      grn_obj_ensure_bulk(ctx, value);
      value->header.domain = grn_obj_get_range(ctx, obj);
      if (id) {
        if (GRN_TABLE_IS_MULTI_KEYS_GROUPED(obj)) {
          grn_obj *domain;
          domain = grn_ctx_at(ctx, value->header.domain);
          if (GRN_OBJ_TABLEP(domain)) {
            grn_id subrec_id;
            if (grn_table_get_subrecs(ctx, obj, id, &subrec_id, NULL, 1) == 1) {
              GRN_RECORD_PUT(ctx, value, subrec_id);
              processed = GRN_TRUE;
            }
          }
          grn_obj_unref(ctx, domain);
        }
        if (!processed) {
          grn_hash *hash = (grn_hash *)obj;
          uint32_t size = hash->value_size;
          if (grn_bulk_space(ctx, value, size)) {
            MERR("grn_bulk_space failed");
            goto exit;
          }
          {
            char *curr = GRN_BULK_CURR(value);
            grn_hash_get_value(ctx, hash, id, curr - size);
          }
        }
      }
    }
    break;
  case GRN_TABLE_NO_KEY:
    {
      grn_array *array = (grn_array *)obj;
      uint32_t size = array->value_size;
      grn_obj_ensure_bulk(ctx, value);
      if (id) {
        if (grn_bulk_space(ctx, value, size)) {
          MERR("grn_bulk_space failed");
          goto exit;
        }
        {
          char *curr = GRN_BULK_CURR(value);
          grn_array_get_value(ctx, array, id, curr - size);
        }
      }
      value->header.type = GRN_BULK;
      value->header.domain = grn_obj_get_range(ctx, obj);
    }
    break;
  case GRN_COLUMN_VAR_SIZE:
    switch (obj->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
    case GRN_OBJ_COLUMN_VECTOR:
      grn_obj_get_value_column_vector(ctx, obj, id, value);
      break;
    case GRN_OBJ_COLUMN_SCALAR:
      grn_obj_ensure_bulk(ctx, value);
      if (id) {
        grn_ja_get_value(ctx, (grn_ja *)obj, id, value);
      }
      value->header.type = GRN_BULK;
      break;
    default:
      ERR(GRN_FILE_CORRUPT, "invalid GRN_OBJ_COLUMN_TYPE");
      break;
    }
    value->header.domain = grn_obj_get_range(ctx, obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
    grn_obj_ensure_bulk(ctx, value);
    value->header.type = GRN_BULK;
    value->header.domain = grn_obj_get_range(ctx, obj);
    if (id) {
      unsigned int element_size;
      void *v = grn_ra_ref(ctx, (grn_ra *)obj, id);
      if (v) {
        element_size = ((grn_ra *)obj)->header->element_size;
        grn_bulk_write(ctx, value, v, element_size);
        grn_ra_unref(ctx, (grn_ra *)obj, id);
      }
    }
    break;
  case GRN_COLUMN_INDEX:
    grn_obj_get_value_column_index(ctx, obj, id, value);
    break;
  }
exit:
  GRN_API_RETURN(value);
}

int
grn_obj_get_values(grn_ctx *ctx, grn_obj *obj, grn_id offset, void **values)
{
  int nrecords = -1;
  GRN_API_ENTER;
  if (obj->header.type == GRN_COLUMN_FIX_SIZE) {
    grn_obj *domain = grn_column_table(ctx, obj);
    if (domain) {
      unsigned int table_size = grn_table_size(ctx, domain);
      if (0 < offset && offset <= table_size) {
        grn_ra *ra = (grn_ra *)obj;
        void *p = grn_ra_ref(ctx, ra, offset);
        if (p) {
          if ((offset >> ra->element_width) ==
              (table_size >> ra->element_width)) {
            nrecords =
              (table_size & ra->element_mask) + 1 - (offset & ra->element_mask);
          } else {
            nrecords = ra->element_mask + 1 - (offset & ra->element_mask);
          }
          if (values) {
            *values = p;
          }
          grn_ra_unref(ctx, ra, offset);
        } else {
          ERR(GRN_NO_MEMORY_AVAILABLE, "ra get failed");
        }
      } else {
        nrecords = 0;
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT, "no domain found");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "obj is not a fix sized column");
  }
  GRN_API_RETURN(nrecords);
}

grn_rc
grn_column_index_update(grn_ctx *ctx,
                        grn_obj *column,
                        grn_id id,
                        unsigned int section,
                        grn_obj *oldvalue,
                        grn_obj *newvalue)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (column->header.type != GRN_COLUMN_INDEX) {
    ERR(GRN_INVALID_ARGUMENT, "invalid column assigned");
  } else {
    rc = grn_ii_column_update(ctx,
                              (grn_ii *)column,
                              id,
                              section,
                              oldvalue,
                              newvalue,
                              NULL);
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
grn_obj_get_info(grn_ctx *ctx,
                 grn_obj *obj,
                 grn_info_type type,
                 grn_obj *valuebuf)
{
  const char *tag = "[obj][get-info]";
  GRN_API_ENTER;
  switch (type) {
  case GRN_INFO_SUPPORT_ZLIB:
    if (!valuebuf &&
        !(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_BOOL))) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[support-zlib] failed to allocate value buffer",
          tag);
      goto exit;
    }
#ifdef GRN_WITH_ZLIB
    GRN_BOOL_PUT(ctx, valuebuf, GRN_TRUE);
#else
    GRN_BOOL_PUT(ctx, valuebuf, GRN_FALSE);
#endif
    break;
  case GRN_INFO_SUPPORT_LZ4:
    if (!valuebuf &&
        !(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_BOOL))) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[support-lz4] failed to allocate value buffer",
          tag);
      goto exit;
    }
#ifdef GRN_WITH_LZ4
    GRN_BOOL_PUT(ctx, valuebuf, GRN_TRUE);
#else  /* GRN_WITH_LZ4 */
    GRN_BOOL_PUT(ctx, valuebuf, GRN_FALSE);
#endif /* GRN_WITH_LZ4 */
    break;
  case GRN_INFO_SUPPORT_ZSTD:
    if (!valuebuf &&
        !(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_BOOL))) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[support-zstd] failed to allocate value buffer",
          tag);
      goto exit;
    }
#ifdef GRN_WITH_ZSTD
    GRN_BOOL_PUT(ctx, valuebuf, GRN_TRUE);
#else  /* GRN_WITH_ZSTD */
    GRN_BOOL_PUT(ctx, valuebuf, GRN_FALSE);
#endif /* GRN_WITH_ZSTD */
    break;
  case GRN_INFO_SUPPORT_APACHE_ARROW:
    if (!valuebuf &&
        !(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_BOOL))) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[support-apache-arrow] failed to allocate value buffer",
          tag);
      goto exit;
    }
#ifdef GRN_WITH_APACHE_ARROW
    GRN_BOOL_PUT(ctx, valuebuf, GRN_TRUE);
#else  /* GRN_WITH_APACHE_ARROW */
    GRN_BOOL_PUT(ctx, valuebuf, GRN_FALSE);
#endif /* GRN_WITH_APACHE_ARROW */
    break;
  default:
    if (!obj) {
      ERR(GRN_INVALID_ARGUMENT, "%s obj is NULL", tag);
      goto exit;
    }
    switch (type) {
    case GRN_INFO_ENCODING:
      if (!valuebuf) {
        if (!(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, 0))) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s[encoding] failed to allocate value buffer",
              tag);
          goto exit;
        }
      }
      {
        grn_encoding enc;
        if (obj->header.type == GRN_DB) {
          obj = ((grn_db *)obj)->keys;
        }
        switch (obj->header.type) {
        case GRN_TABLE_PAT_KEY:
          enc = ((grn_pat *)obj)->encoding;
          grn_bulk_write(ctx,
                         valuebuf,
                         (const char *)&enc,
                         sizeof(grn_encoding));
          break;
        case GRN_TABLE_DAT_KEY:
          enc = ((grn_dat *)obj)->encoding;
          grn_bulk_write(ctx,
                         valuebuf,
                         (const char *)&enc,
                         sizeof(grn_encoding));
          break;
        case GRN_TABLE_HASH_KEY:
          enc = ((grn_hash *)obj)->encoding;
          grn_bulk_write(ctx,
                         valuebuf,
                         (const char *)&enc,
                         sizeof(grn_encoding));
          break;
        default:
          ERR(GRN_INVALID_ARGUMENT,
              "%s[encoding] target object must be one of %s, %s and %s: %s",
              tag,
              grn_obj_type_to_string(GRN_TABLE_HASH_KEY),
              grn_obj_type_to_string(GRN_TABLE_PAT_KEY),
              grn_obj_type_to_string(GRN_TABLE_DAT_KEY),
              grn_obj_type_to_string(obj->header.type));
          break;
        }
      }
      break;
    case GRN_INFO_SOURCE:
      if (!valuebuf) {
        if (!(valuebuf = grn_obj_open(ctx, GRN_BULK, 0, 0))) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s[source] failed to allocate value buffer",
              tag);
          goto exit;
        }
      }
      if (!GRN_DB_OBJP(obj)) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[source] target object must be a grn_db_obj: %s",
            tag,
            grn_obj_type_to_string(obj->header.type));
        goto exit;
      }
      grn_bulk_write(ctx,
                     valuebuf,
                     DB_OBJ(obj)->source,
                     DB_OBJ(obj)->source_size);
      break;
    case GRN_INFO_DEFAULT_TOKENIZER:
      switch (DB_OBJ(obj)->header.type) {
      case GRN_TABLE_HASH_KEY:
        valuebuf = ((grn_hash *)obj)->tokenizer.proc;
        break;
      case GRN_TABLE_PAT_KEY:
        valuebuf = ((grn_pat *)obj)->tokenizer.proc;
        break;
      case GRN_TABLE_DAT_KEY:
        valuebuf = ((grn_dat *)obj)->tokenizer.proc;
        break;
      }
      break;
    case GRN_INFO_NORMALIZER:
      {
        grn_obj *normalizers = NULL;
        switch (obj->header.type) {
        case GRN_TABLE_HASH_KEY:
          normalizers = &(((grn_hash *)obj)->normalizers);
          break;
        case GRN_TABLE_PAT_KEY:
          normalizers = &(((grn_pat *)obj)->normalizers);
          break;
        case GRN_TABLE_DAT_KEY:
          normalizers = &(((grn_dat *)obj)->normalizers);
          break;
        default:
          break;
        }
        if (normalizers &&
            GRN_BULK_VSIZE(normalizers) >= sizeof(grn_table_module)) {
          grn_table_module *module =
            (grn_table_module *)GRN_BULK_HEAD(normalizers);
          valuebuf = module->proc;
        }
      }
      break;
    case GRN_INFO_NORMALIZERS:
      if (!valuebuf) {
        if (!(valuebuf = grn_obj_open(ctx, GRN_PVECTOR, 0, 0))) {
          ERR(GRN_NO_MEMORY_AVAILABLE,
              "%s[normalizers] failed to allocate value buffer",
              tag);
          goto exit;
        }
      }
      {
        grn_obj *normalizers = NULL;
        switch (obj->header.type) {
        case GRN_TABLE_HASH_KEY:
          normalizers = &(((grn_hash *)obj)->normalizers);
          break;
        case GRN_TABLE_PAT_KEY:
          normalizers = &(((grn_pat *)obj)->normalizers);
          break;
        case GRN_TABLE_DAT_KEY:
          normalizers = &(((grn_dat *)obj)->normalizers);
          break;
        default:
          ERR(GRN_INVALID_ARGUMENT,
              "%s[normalizers] target object must be one of %s, %s and %s: %s",
              tag,
              grn_obj_type_to_string(GRN_TABLE_HASH_KEY),
              grn_obj_type_to_string(GRN_TABLE_PAT_KEY),
              grn_obj_type_to_string(GRN_TABLE_DAT_KEY),
              grn_obj_type_to_string(obj->header.type));
          break;
        }
        if (normalizers) {
          grn_table_module *raw_normalizers =
            (grn_table_module *)GRN_BULK_HEAD(normalizers);
          size_t n = GRN_BULK_VSIZE(normalizers) / sizeof(grn_table_module);
          size_t i;
          for (i = 0; i < n; i++) {
            grn_obj *normalizer = raw_normalizers[i].proc;
            GRN_PTR_PUT(ctx, valuebuf, normalizer);
          }
        }
      }
      break;
    case GRN_INFO_TOKEN_FILTERS:
      if (!valuebuf) {
        if (!(valuebuf = grn_obj_open(ctx, GRN_PVECTOR, 0, 0))) {
          ERR(GRN_NO_MEMORY_AVAILABLE,
              "%s[token-filters] failed to allocate value buffer",
              tag);
          goto exit;
        }
      }
      {
        grn_obj *token_filters = NULL;
        switch (obj->header.type) {
        case GRN_TABLE_HASH_KEY:
          token_filters = &(((grn_hash *)obj)->token_filter_procs);
          break;
        case GRN_TABLE_PAT_KEY:
          token_filters = &(((grn_pat *)obj)->token_filter_procs);
          break;
        case GRN_TABLE_DAT_KEY:
          token_filters = &(((grn_dat *)obj)->token_filter_procs);
          break;
        default:
          ERR(GRN_INVALID_ARGUMENT,
              "[info][get][token-filters] target object must be one of "
              "%s, %s and %s: %s",
              grn_obj_type_to_string(GRN_TABLE_HASH_KEY),
              grn_obj_type_to_string(GRN_TABLE_PAT_KEY),
              grn_obj_type_to_string(GRN_TABLE_DAT_KEY),
              grn_obj_type_to_string(obj->header.type));
          break;
        }
        if (token_filters) {
          grn_bulk_write(ctx,
                         valuebuf,
                         GRN_BULK_HEAD(token_filters),
                         GRN_BULK_VSIZE(token_filters));
        }
      }
      break;
    default:
      /* todo */
      break;
    }
  }
exit:
  GRN_API_RETURN(valuebuf);
}

static void
update_source_hook(grn_ctx *ctx, grn_obj *obj)
{
  grn_id *s = DB_OBJ(obj)->source;
  int i, n = DB_OBJ(obj)->source_size / sizeof(grn_id);
  grn_obj_default_set_value_hook_data hook_data = {DB_OBJ(obj)->id, 0};
  grn_obj *source, data;
  GRN_TEXT_INIT(&data, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET_REF(&data, &hook_data, sizeof(hook_data));
  for (i = 1; i <= n; i++, s++) {
    hook_data.section = i;
    if ((source = grn_ctx_at(ctx, *s))) {
      switch (source->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
        grn_obj_add_hook(ctx, source, GRN_HOOK_INSERT, 0, NULL, &data);
        grn_obj_add_hook(ctx, source, GRN_HOOK_DELETE, 0, NULL, &data);
        break;
      case GRN_COLUMN_FIX_SIZE:
      case GRN_COLUMN_VAR_SIZE:
      case GRN_COLUMN_INDEX:
        grn_obj_add_hook(ctx, source, GRN_HOOK_SET, 0, NULL, &data);
        break;
      default:
        /* invalid target */
        break;
      }
      grn_obj_unref(ctx, source);
    }
  }
  grn_obj_close(ctx, &data);
}

static void
del_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, grn_id target_id)
{
  grn_obj offsets;
  GRN_INT32_INIT(&offsets, GRN_OBJ_VECTOR);

  int i;
  grn_hook *hook;
  for (i = 0, hook = DB_OBJ(obj)->hooks[entry]; hook; i++, hook = hook->next) {
    if (hook->proc) {
      continue;
    }
    if (hook->hld_size != sizeof(grn_obj_default_set_value_hook_data)) {
      continue;
    }
    grn_obj_default_set_value_hook_data *data =
      (grn_obj_default_set_value_hook_data *)GRN_NEXT_ADDR(hook);
    if (data->target == target_id) {
      GRN_INT32_PUT(ctx, &offsets, i);
    }
  }

  int n = GRN_INT32_VECTOR_SIZE(&offsets);
  for (i = 0; i < n; i++) {
    /* This must be larger -> smaller order. If we use smaller ->
     * larger order, offset is changed. */
    int offset = GRN_INT32_VALUE_AT(&offsets, n - i - 1);
    grn_obj_delete_hook(ctx, obj, entry, offset);
  }

  GRN_OBJ_FIN(ctx, &offsets);
}

static void
delete_source_hook(grn_ctx *ctx, grn_obj *obj)
{
  grn_id id = DB_OBJ(obj)->id;
  grn_id *source_ids = DB_OBJ(obj)->source;
  int i, n = DB_OBJ(obj)->source_size / sizeof(grn_id);
  for (i = 0; i < n; i++) {
    grn_id source_id = source_ids[i];
    grn_obj *source = grn_ctx_at(ctx, source_id);
    if (!source) {
      ERRCLR(ctx);
      continue;
    }

    switch (source->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      del_hook(ctx, source, GRN_HOOK_INSERT, id);
      del_hook(ctx, source, GRN_HOOK_DELETE, id);
      break;
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE:
      del_hook(ctx, source, GRN_HOOK_SET, id);
      break;
    default:
      /* invalid target */
      break;
    }
  }
}

/* This should be used only when the target object can't be opened. If
 * the target object can be opened, use delete_source_hook() instead. */
/* static void */
static void
delete_source_hook_full_scan(grn_ctx *ctx, grn_id target_id, uint32_t flags)
{
  bool is_close_opened_object_mode = false;
  if (grn_thread_get_limit() == 1) {
    is_close_opened_object_mode = true;
  }

  GRN_DB_EACH_SPEC_BEGIN(ctx, cursor, id, spec)
  {
    if (id == target_id) {
      continue;
    }

    bool may_have_source = false;
    switch (spec->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
    case GRN_COLUMN_VAR_SIZE:
    case GRN_COLUMN_FIX_SIZE:
      may_have_source = true;
      break;
    default:
      break;
    }
    if (!may_have_source) {
      continue;
    }

    if (is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    /* TODO: We should not access to internal variable in
     * GRN_DB_EACH_SPEC_BEGIN(). */
    const char *raw_hooks;
    uint32_t size = grn_vector_get_element(ctx,
                                           &decoded_spec,
                                           GRN_SERIALIZED_SPEC_INDEX_HOOK,
                                           &raw_hooks,
                                           NULL,
                                           NULL);

    grn_obj *source = NULL;
    /* TODO: Unify with grn_hook_unpack() */
    grn_hook_entry entry;
    const uint8_t *current = (const uint8_t *)raw_hooks;
    const uint8_t *end = current + size;
    for (entry = 0; entry < GRN_N_HOOK_ENTRIES && ctx->rc == GRN_SUCCESS;
         entry++) {
      int i;
      for (i = 0; true; i++) {
        uint32_t hook_proc_id_plus_one;
        GRN_B_DEC(hook_proc_id_plus_one, current);
        if (hook_proc_id_plus_one == 0) {
          break;
        }
        grn_id hook_proc_id = hook_proc_id_plus_one - 1;
        if (current >= end) {
          break;
        }
        uint32_t hold_size;
        GRN_B_DEC(hold_size, current);
        if (current >= end) {
          break;
        }
        if (current + hold_size >= end) {
          break;
        }
        if (hook_proc_id == GRN_ID_NIL) {
          grn_obj_default_set_value_hook_data *data =
            (grn_obj_default_set_value_hook_data *)current;
          if (data->target == target_id) {
            if (!source) {
              source = grn_ctx_at(ctx, id);
              if (!source) {
                break;
              }
            }
            grn_obj_delete_hook(ctx, source, entry, i);
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
        }
        current += hold_size;
      }
    }

    if (is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
  }
  GRN_DB_EACH_SPEC_END(ctx, cursor);

  return;
}

grn_rc
grn_hook_pack(grn_ctx *ctx, grn_db_obj *obj, grn_obj *buf)
{
  grn_rc rc;
  grn_hook_entry e;
  for (e = 0; e < GRN_N_HOOK_ENTRIES; e++) {
    grn_hook *hooks;
    for (hooks = obj->hooks[e]; hooks; hooks = hooks->next) {
      grn_id id = hooks->proc ? hooks->proc->obj.id : 0;
      if ((rc = grn_text_benc(ctx, buf, id + 1))) {
        goto exit;
      }
      if ((rc = grn_text_benc(ctx, buf, hooks->hld_size))) {
        goto exit;
      }
      if ((rc = grn_bulk_write(ctx,
                               buf,
                               (char *)GRN_NEXT_ADDR(hooks),
                               hooks->hld_size))) {
        goto exit;
      }
    }
    if ((rc = grn_text_benc(ctx, buf, 0))) {
      goto exit;
    }
  }
exit:
  return rc;
}

static grn_rc
grn_hook_unpack(grn_ctx *ctx,
                grn_db_obj *obj,
                const char *buf,
                uint32_t buf_size)
{
  grn_hook_entry e;
  const uint8_t *p = (uint8_t *)buf, *pe = p + buf_size;
  for (e = 0; e < GRN_N_HOOK_ENTRIES; e++) {
    grn_hook *new, **last = &obj->hooks[e];
    for (;;) {
      grn_id id;
      uint32_t hld_size;
      GRN_B_DEC(id, p);
      if (!id--) {
        break;
      }
      if (p >= pe) {
        return GRN_FILE_CORRUPT;
      }
      GRN_B_DEC(hld_size, p);
      if (p >= pe) {
        return GRN_FILE_CORRUPT;
      }
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
        grn_memcpy(GRN_NEXT_ADDR(new), p, hld_size);
        p += hld_size;
      }
      *last = new;
      last = &new->next;
      if (p >= pe) {
        return GRN_FILE_CORRUPT;
      }
    }
    *last = NULL;
  }
  return GRN_SUCCESS;
}

static void
grn_table_modules_pack(grn_ctx *ctx, grn_obj *table_modules, grn_obj *buffer)
{
  grn_table_module *raw_table_modules =
    (grn_table_module *)GRN_BULK_HEAD(table_modules);
  size_t n = GRN_BULK_VSIZE(table_modules) / sizeof(grn_table_module);
  size_t i;
  for (i = 0; i < n; i++) {
    grn_obj *proc = raw_table_modules[i].proc;
    grn_id proc_id = grn_obj_id(ctx, proc);
    GRN_RECORD_PUT(ctx, buffer, proc_id);
  }
}

static grn_bool
grn_obj_encoded_spec_equal(grn_ctx *ctx,
                           grn_obj *encoded_spec1,
                           grn_obj *encoded_spec2)
{
  unsigned int i, n_elements;

  if (encoded_spec1->header.type != GRN_VECTOR) {
    return GRN_FALSE;
  }

  if (encoded_spec1->header.type != encoded_spec2->header.type) {
    return GRN_FALSE;
  }

  n_elements = grn_vector_size(ctx, encoded_spec1);
  if (grn_vector_size(ctx, encoded_spec2) != n_elements) {
    return GRN_FALSE;
  }

  for (i = 0; i < n_elements; i++) {
    const char *content1;
    const char *content2;
    unsigned int content_size1;
    unsigned int content_size2;
    uint32_t weight1;
    uint32_t weight2;
    grn_id domain1;
    grn_id domain2;

    content_size1 = grn_vector_get_element(ctx,
                                           encoded_spec1,
                                           i,
                                           &content1,
                                           &weight1,
                                           &domain1);
    content_size2 = grn_vector_get_element(ctx,
                                           encoded_spec2,
                                           i,
                                           &content2,
                                           &weight2,
                                           &domain2);
    if (content_size1 != content_size2) {
      return GRN_FALSE;
    }
    if (memcmp(content1, content2, content_size1) != 0) {
      return GRN_FALSE;
    }
    if (weight1 != weight2) {
      return GRN_FALSE;
    }
    if (domain1 != domain2) {
      return GRN_FALSE;
    }
  }

  return GRN_TRUE;
}

void
grn_obj_spec_save(grn_ctx *ctx, grn_db_obj *obj)
{
  grn_db *s;
  grn_obj v, *b;
  grn_obj_spec spec;
  grn_bool need_update = GRN_TRUE;

  if (obj->id & GRN_OBJ_TMP_OBJECT) {
    return;
  }
  if (!ctx->impl || !GRN_DB_OBJP(obj)) {
    return;
  }
  if (!(s = (grn_db *)ctx->impl->db) || !s->specs) {
    return;
  }
  if (obj->header.type == GRN_PROC && obj->range == GRN_ID_NIL) {
    return;
  }
  GRN_OBJ_INIT(&v, GRN_VECTOR, 0, GRN_DB_TEXT);
  if (!(b = grn_vector_body(ctx, &v))) {
    return;
  }
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
  case GRN_TABLE_HASH_KEY:
    grn_table_modules_pack(ctx, &(((grn_hash *)obj)->token_filters), b);
    grn_vector_delimit(ctx, &v, 0, 0);
    grn_table_modules_pack(ctx, &(((grn_hash *)obj)->normalizers), b);
    grn_vector_delimit(ctx, &v, 0, 0);
    break;
  case GRN_TABLE_PAT_KEY:
    grn_table_modules_pack(ctx, &(((grn_pat *)obj)->token_filters), b);
    grn_vector_delimit(ctx, &v, 0, 0);
    grn_table_modules_pack(ctx, &(((grn_pat *)obj)->normalizers), b);
    grn_vector_delimit(ctx, &v, 0, 0);
    break;
  case GRN_TABLE_DAT_KEY:
    grn_table_modules_pack(ctx, &(((grn_dat *)obj)->token_filters), b);
    grn_vector_delimit(ctx, &v, 0, 0);
    grn_table_modules_pack(ctx, &(((grn_dat *)obj)->normalizers), b);
    grn_vector_delimit(ctx, &v, 0, 0);
    break;
  case GRN_EXPR:
    grn_expr_pack(ctx, b, (grn_obj *)obj);
    grn_vector_delimit(ctx, &v, 0, 0);
    break;
  }

  {
    grn_io_win jw;
    uint32_t current_spec_raw_len;
    char *current_spec_raw;

    current_spec_raw =
      grn_ja_ref(ctx, s->specs, obj->id, &jw, &current_spec_raw_len);
    if (current_spec_raw) {
      grn_rc rc;
      grn_obj current_spec;

      GRN_OBJ_INIT(&current_spec, GRN_VECTOR, 0, GRN_DB_TEXT);
      rc = grn_vector_unpack(ctx,
                             &current_spec,
                             current_spec_raw,
                             current_spec_raw_len,
                             0,
                             NULL);
      if (rc == GRN_SUCCESS) {
        need_update = !grn_obj_encoded_spec_equal(ctx, &v, &current_spec);
      }
      GRN_OBJ_FIN(ctx, &current_spec);
      grn_ja_unref(ctx, &jw);
    }
  }

  if (!need_update) {
    grn_obj_close(ctx, &v);
    return;
  }

  {
    const char *name;
    uint32_t name_size = 0;
    const char *range_name = NULL;
    uint32_t range_name_size = 0;

    name = _grn_table_key(ctx, s->keys, obj->id, &name_size);
    switch (obj->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
    case GRN_TABLE_NO_KEY:
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE:
    case GRN_COLUMN_INDEX:
      if (obj->range != GRN_ID_NIL) {
        range_name = _grn_table_key(ctx, s->keys, obj->range, &range_name_size);
      }
      break;
    default:
      break;
    }
    /* TODO: reduce log level. */
    GRN_LOG(ctx,
            GRN_LOG_NOTICE,
            "spec:%u:update:%.*s:%u(%s):%u%s%.*s%s",
            obj->id,
            name_size,
            name,
            obj->header.type,
            grn_obj_type_to_string(obj->header.type),
            obj->range,
            range_name_size == 0 ? "" : "(",
            range_name_size,
            range_name,
            range_name_size == 0 ? "" : ")");
  }
  grn_ja_putv(ctx, s->specs, obj->id, &v, GRN_OBJ_SET);
  grn_obj_close(ctx, &v);
}

grn_inline static void
grn_obj_set_info_source_invalid_lexicon_error(grn_ctx *ctx,
                                              const char *message,
                                              grn_obj *actual_type,
                                              grn_obj *expected_type,
                                              grn_obj *index_column,
                                              grn_obj *source)
{
  char actual_type_name[GRN_TABLE_MAX_KEY_SIZE];
  int actual_type_name_size;
  char expected_type_name[GRN_TABLE_MAX_KEY_SIZE];
  int expected_type_name_size;
  char index_column_name[GRN_TABLE_MAX_KEY_SIZE];
  int index_column_name_size;
  char source_name[GRN_TABLE_MAX_KEY_SIZE];
  int source_name_size;

  actual_type_name_size =
    grn_obj_name(ctx, actual_type, actual_type_name, GRN_TABLE_MAX_KEY_SIZE);
  expected_type_name_size = grn_obj_name(ctx,
                                         expected_type,
                                         expected_type_name,
                                         GRN_TABLE_MAX_KEY_SIZE);
  index_column_name_size =
    grn_obj_name(ctx, index_column, index_column_name, GRN_TABLE_MAX_KEY_SIZE);

  source_name_size =
    grn_obj_name(ctx, source, source_name, GRN_TABLE_MAX_KEY_SIZE);
  if (grn_obj_is_table(ctx, source)) {
    source_name[source_name_size] = '\0';
    grn_strncat(source_name,
                GRN_TABLE_MAX_KEY_SIZE,
                "._key",
                GRN_TABLE_MAX_KEY_SIZE - source_name_size - 1);
    source_name_size = strlen(source_name);
  }

  ERR(GRN_INVALID_ARGUMENT,
      "[column][index][source] %s: "
      "<%.*s> -> <%.*s>: "
      "index-column:<%.*s> "
      "source:<%.*s>",
      message,
      actual_type_name_size,
      actual_type_name,
      expected_type_name_size,
      expected_type_name,
      index_column_name_size,
      index_column_name,
      source_name_size,
      source_name);
}

grn_inline static grn_rc
grn_obj_set_info_source_validate(grn_ctx *ctx, grn_obj *obj, grn_obj *value)
{
  grn_id lexicon_id;
  grn_obj *lexicon = NULL;
  grn_id lexicon_domain_id;
  grn_obj *lexicon_domain = NULL;
  bool lexicon_domain_is_table = false;
  grn_bool lexicon_have_tokenizer;
  grn_bool is_full_text_search_index;
  grn_id *source_ids;
  int i, n_source_ids;

  lexicon_id = obj->header.domain;
  lexicon = grn_ctx_at(ctx, lexicon_id);
  if (!lexicon) {
    goto exit;
  }

  lexicon_domain_id = lexicon->header.domain;
  lexicon_domain = grn_ctx_at(ctx, lexicon_domain_id);
  if (!lexicon_domain) {
    goto exit;
  }
  lexicon_domain_is_table = grn_obj_is_table(ctx, lexicon_domain);
  {
    grn_obj *tokenizer;
    grn_table_get_info(ctx, lexicon, NULL, NULL, &tokenizer, NULL, NULL);
    lexicon_have_tokenizer = (tokenizer != NULL);

    is_full_text_search_index =
      (grn_obj_is_index_column(ctx, obj) &&
       (obj->header.flags & GRN_OBJ_WITH_POSITION) && lexicon_have_tokenizer &&
       grn_obj_id(ctx, tokenizer) != GRN_DB_DELIMIT);
  }

  source_ids = (grn_id *)GRN_BULK_HEAD(value);
  n_source_ids = GRN_BULK_VSIZE(value) / sizeof(grn_id);

  if (n_source_ids > 1 && !(obj->header.flags & GRN_OBJ_WITH_SECTION)) {
    char index_name[GRN_TABLE_MAX_KEY_SIZE];
    int index_name_size;
    index_name_size =
      grn_obj_name(ctx, obj, index_name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_INVALID_ARGUMENT,
        "grn_obj_set_info(): GRN_INFO_SOURCE: "
        "multi column index must be created with WITH_SECTION flag: <%.*s>",
        index_name_size,
        index_name);
    goto exit;
  }

  if (is_full_text_search_index) {
    grn_bool have_vector_source_column = GRN_FALSE;

    for (i = 0; i < n_source_ids; i++) {
      grn_obj *source;

      source = grn_ctx_at(ctx, source_ids[i]);
      bool source_is_vector_column = grn_obj_is_vector_column(ctx, source);
      if (!grn_obj_is_temporary(ctx, source)) {
        grn_obj_unlink(ctx, source);
      }
      if (!source_is_vector_column) {
        continue;
      }

      have_vector_source_column = GRN_TRUE;
      if (!(obj->header.flags & GRN_OBJ_WITH_SECTION)) {
        char index_name[GRN_TABLE_MAX_KEY_SIZE];
        int index_name_size;
        index_name_size =
          grn_obj_name(ctx, obj, index_name, GRN_TABLE_MAX_KEY_SIZE);
        ERR(GRN_INVALID_ARGUMENT,
            "grn_obj_set_info(): GRN_INFO_SOURCE: "
            "full text index for vector column "
            "must be created with WITH_SECTION flag: <%.*s>",
            index_name_size,
            index_name);
        goto exit;
      }
    }

    if (have_vector_source_column && n_source_ids > 1) {
      char index_name[GRN_TABLE_MAX_KEY_SIZE];
      int index_name_size;
      index_name_size =
        grn_obj_name(ctx, obj, index_name, GRN_TABLE_MAX_KEY_SIZE);
      ERR(
        GRN_INVALID_ARGUMENT,
        "grn_obj_set_info(): GRN_INFO_SOURCE: "
        "multi column full text index with vector column isn't supported yet: "
        "<%.*s>",
        index_name_size,
        index_name);
      goto exit;
    }
  }

  for (i = 0; i < n_source_ids; i++) {
    grn_id source_id = source_ids[i];
    grn_obj *source;
    grn_id source_type_id;
    grn_obj *source_type;

    source = grn_ctx_at(ctx, source_id);
    if (!source) {
      continue;
    }
    if (grn_obj_is_table(ctx, source)) {
      source_type_id = source->header.domain;
    } else {
      source_type_id = DB_OBJ(source)->range;
    }
    source_type = grn_ctx_at(ctx, source_type_id);
    if (!lexicon_have_tokenizer) {
      if (grn_obj_is_table(ctx, source_type)) {
        if (lexicon_id != source_type_id) {
          grn_obj_set_info_source_invalid_lexicon_error(
            ctx,
            "index table must equal to source type",
            lexicon,
            source_type,
            obj,
            source);
        }
      } else {
        if (!grn_type_id_is_compatible(ctx,
                                       lexicon_domain_id,
                                       source_type_id)) {
          grn_obj_set_info_source_invalid_lexicon_error(
            ctx,
            "index table's key must equal source type",
            lexicon_domain,
            source_type,
            obj,
            source);
        }
      }
    }
    grn_obj_unref(ctx, source_type);
    grn_obj_unref(ctx, source);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

exit:
  if (lexicon_domain && lexicon_domain_is_table &&
      !grn_obj_is_temporary(ctx, lexicon_domain)) {
    grn_obj_unlink(ctx, lexicon_domain);
  }
  if (lexicon && !grn_obj_is_temporary(ctx, lexicon)) {
    grn_obj_unlink(ctx, lexicon);
  }

  return ctx->rc;
}

grn_inline static void
grn_obj_set_info_source_log(grn_ctx *ctx, grn_obj *obj, grn_obj *value)
{
  grn_obj buf;
  grn_id *vp = (grn_id *)GRN_BULK_HEAD(value);
  uint32_t vs = GRN_BULK_VSIZE(value), s = 0;
  grn_id id;
  const char *n;

  id = DB_OBJ(obj)->id;
  n = _grn_table_key(ctx, ctx->impl->db, id, &s);
  GRN_TEXT_INIT(&buf, 0);
  GRN_TEXT_PUT(ctx, &buf, n, s);
  GRN_TEXT_PUTC(ctx, &buf, ' ');
  while (vs) {
    n = _grn_table_key(ctx, ctx->impl->db, *vp++, &s);
    GRN_TEXT_PUT(ctx, &buf, n, s);
    vs -= sizeof(grn_id);
    if (vs) {
      GRN_TEXT_PUTC(ctx, &buf, ',');
    }
  }
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "DDL:%u:set_source %.*s",
          id,
          (int)GRN_BULK_VSIZE(&buf),
          GRN_BULK_HEAD(&buf));
  GRN_OBJ_FIN(ctx, &buf);
}

grn_inline static grn_rc
grn_obj_set_info_source_update(grn_ctx *ctx, grn_obj *obj, grn_obj *value)
{
  void *v = GRN_BULK_HEAD(value);
  uint32_t s = GRN_BULK_VSIZE(value);
  if (s) {
    void *v2 = GRN_MALLOC(s);
    if (!v2) {
      return ctx->rc;
    }
    grn_memcpy(v2, v, s);
    if (DB_OBJ(obj)->source) {
      GRN_FREE(DB_OBJ(obj)->source);
    }
    DB_OBJ(obj)->source = v2;
    DB_OBJ(obj)->source_size = s;
    grn_obj_spec_save(ctx, DB_OBJ(obj));

    switch (obj->header.type) {
    case GRN_COLUMN_VAR_SIZE:
      update_source_hook(ctx, obj);
      grn_token_column_build(ctx, obj);
      break;
    case GRN_COLUMN_INDEX:
      update_source_hook(ctx, obj);
      grn_index_column_build(ctx, obj);
      break;
    default:
      break;
    }
  } else {
    DB_OBJ(obj)->source = NULL;
    DB_OBJ(obj)->source_size = 0;
    grn_obj_spec_save(ctx, DB_OBJ(obj));
  }

  return GRN_SUCCESS;
}

grn_inline static grn_rc
grn_obj_set_info_source(grn_ctx *ctx, grn_obj *obj, grn_obj *value)
{
  grn_rc rc;

  rc = grn_obj_set_info_source_validate(ctx, obj, value);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  grn_obj_set_info_source_log(ctx, obj, value);
  rc = grn_obj_set_info_source_update(ctx, obj, value);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  return rc;
}

static grn_bool
grn_obj_set_info_is_funcall_call_bulk(grn_ctx *ctx, grn_obj *bulk)
{
  const char *current;
  const char *end;

  current = GRN_TEXT_VALUE(bulk);
  end = current + GRN_TEXT_LEN(bulk);
  while (current < end) {
    int char_length;

    char_length = grn_charlen(ctx, current, end);
    if (char_length != 1) {
      return GRN_TRUE;
    }

    if (current[0] == '(') {
      return GRN_TRUE;
    }

    current += char_length;
  }

  return GRN_FALSE;
}

static grn_rc
grn_obj_set_info_require_key_table(grn_ctx *ctx,
                                   grn_obj *table,
                                   const char *context_tag)
{
  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    return ctx->rc;
  default:
    ERR(GRN_INVALID_ARGUMENT,
        "%s target object must be one of "
        "GRN_TABLE_HASH_KEY, GRN_TABLE_PAT_KEY and GRN_TABLE_DAT_KEY: <%s>",
        context_tag,
        grn_obj_type_to_string(table->header.type));
    return ctx->rc;
  }
}

static grn_rc
grn_obj_set_info_table_module_raw(grn_ctx *ctx,
                                  grn_obj *table,
                                  grn_info_type type,
                                  grn_table_module *table_module,
                                  grn_id *proc_id,
                                  unsigned int i,
                                  grn_obj *module,
                                  const char *context_tag,
                                  const char *module_name)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  unsigned int name_size;
  grn_obj *proc = NULL;
  grn_obj *expression = NULL;
  grn_obj options;

  GRN_TEXT_INIT(&options, GRN_OBJ_VECTOR);

  name_size = grn_obj_name(ctx, table, name, sizeof(name));
  if (name_size == 0) {
    grn_strcpy(name, sizeof(name), "(anonymous)");
    name_size = strlen(name);
  }
  if (grn_obj_is_text_family_bulk(ctx, module)) {
    if (GRN_TEXT_LEN(module) == 0) {
      proc = NULL;
    } else if (grn_obj_set_info_is_funcall_call_bulk(ctx, module)) {
      grn_obj *unused;
      GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expression, unused);
      grn_expr_parse(ctx,
                     expression,
                     GRN_TEXT_VALUE(module),
                     GRN_TEXT_LEN(module),
                     NULL,
                     GRN_OP_MATCH,
                     GRN_OP_AND,
                     GRN_EXPR_SYNTAX_SCRIPT);
      if (ctx->rc != GRN_SUCCESS) {
        char errbuf[GRN_CTX_MSGSIZE];
        grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] failed to parse %s options: <%.*s>: %s",
            context_tag,
            (int)name_size,
            name,
            module_name,
            (int)GRN_TEXT_LEN(module),
            GRN_TEXT_VALUE(module),
            errbuf);
        goto exit;
      }
      if (!grn_expr_is_simple_function_call(ctx, expression)) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] must be %s(option1, option2, ...) format: <%.*s>",
            context_tag,
            (int)name_size,
            name,
            module_name,
            (int)GRN_TEXT_LEN(module),
            GRN_TEXT_VALUE(module));
        goto exit;
      }
      proc = grn_expr_simple_function_call_get_function(ctx, expression);
      grn_expr_simple_function_call_get_arguments(ctx, expression, &options);
    } else {
      proc = grn_ctx_get(ctx, GRN_TEXT_VALUE(module), GRN_TEXT_LEN(module));
      if (!proc) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] unknown %s: <%.*s>",
            context_tag,
            (int)name_size,
            name,
            module_name,
            (int)GRN_TEXT_LEN(module),
            GRN_TEXT_VALUE(module));
        goto exit;
      }
    }
  } else {
    proc = module;
  }

  if (proc) {
    bool is_valid_proc = false;

    switch (type) {
    case GRN_INFO_DEFAULT_TOKENIZER:
      is_valid_proc = grn_obj_is_tokenizer_proc(ctx, proc);
      break;
    case GRN_INFO_NORMALIZER:
    case GRN_INFO_NORMALIZERS:
      is_valid_proc = grn_obj_is_normalizer_proc(ctx, proc);
      break;
    case GRN_INFO_TOKEN_FILTERS:
      is_valid_proc = grn_obj_is_token_filter_proc(ctx, proc);
      break;
    default:
      break;
    }

    if (!is_valid_proc) {
      char proc_name[GRN_TABLE_MAX_KEY_SIZE];
      unsigned int proc_name_size;

      proc_name_size = grn_obj_name(ctx, proc, proc_name, sizeof(proc_name));
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] invalid %s: <%.*s>",
          context_tag,
          (int)name_size,
          name,
          module_name,
          (int)proc_name_size,
          proc_name);
      goto exit;
    }
  }

  if (table_module) {
    grn_table_module_set_proc(ctx, table_module, proc);
  }
  if (proc) {
    *proc_id = grn_obj_id(ctx, proc);
  } else {
    *proc_id = GRN_ID_NIL;
  }
  switch (type) {
  case GRN_INFO_DEFAULT_TOKENIZER:
    grn_table_set_default_tokenizer_options(ctx, table, &options);
    if (DB_OBJ(table)->header.type == GRN_TABLE_PAT_KEY) {
      grn_pat_cache_enable(ctx, (grn_pat *)table, GRN_TABLE_PAT_KEY_CACHE_SIZE);
    }
    break;
  case GRN_INFO_NORMALIZER:
    grn_table_set_normalizer_options(ctx, table, &options);
    break;
  case GRN_INFO_TOKEN_FILTERS:
    grn_table_set_token_filters_options(ctx, table, i, &options);
    break;
  default:
    break;
  }

exit:
  GRN_OBJ_FIN(ctx, &options);

  if (expression) {
    grn_obj_close(ctx, expression);
  }

  return ctx->rc;
}

static grn_rc
grn_obj_set_info_table_module(grn_ctx *ctx,
                              grn_obj *table,
                              grn_info_type type,
                              grn_obj *module,
                              const char *context_tag,
                              const char *module_name)
{
  grn_table_module *table_module = NULL;
  grn_id *proc_id = NULL;

  if (grn_obj_set_info_require_key_table(ctx, table, context_tag) !=
      GRN_SUCCESS) {
    return ctx->rc;
  }

  switch (DB_OBJ(table)->header.type) {
  case GRN_TABLE_HASH_KEY:
    switch (type) {
    case GRN_INFO_DEFAULT_TOKENIZER:
      table_module = &(((grn_hash *)table)->tokenizer);
      proc_id = &(((grn_hash *)table)->header.common->tokenizer);
      break;
    case GRN_INFO_NORMALIZER:
      table_module = NULL;
      proc_id = &(((grn_hash *)table)->header.common->normalizer);
      break;
    default:
      break;
    }
    break;
  case GRN_TABLE_PAT_KEY:
    switch (type) {
    case GRN_INFO_DEFAULT_TOKENIZER:
      table_module = &(((grn_pat *)table)->tokenizer);
      proc_id = &(((grn_pat *)table)->header->tokenizer);
      break;
    case GRN_INFO_NORMALIZER:
      table_module = NULL;
      proc_id = &(((grn_pat *)table)->header->normalizer);
      break;
    default:
      break;
    }
    break;
  case GRN_TABLE_DAT_KEY:
    switch (type) {
    case GRN_INFO_DEFAULT_TOKENIZER:
      table_module = &(((grn_dat *)table)->tokenizer);
      proc_id = &(((grn_dat *)table)->header->tokenizer);
      break;
    case GRN_INFO_NORMALIZER:
      table_module = NULL;
      proc_id = &(((grn_dat *)table)->header->normalizer);
      break;
    default:
      break;
    }
  default:
    break;
  }

  return grn_obj_set_info_table_module_raw(ctx,
                                           table,
                                           type,
                                           table_module,
                                           proc_id,
                                           0,
                                           module,
                                           context_tag,
                                           module_name);
}

static grn_rc
grn_obj_set_info_table_modules_text(grn_ctx *ctx,
                                    grn_obj *table,
                                    grn_info_type type,
                                    grn_obj *table_modules,
                                    grn_obj *procs,
                                    grn_obj *modules,
                                    const char *context_tag,
                                    const char *module_name)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  unsigned int name_size;
  grn_obj *unused;
  grn_obj *expression = NULL;
  size_t i, n;
  grn_obj options;

  GRN_TEXT_INIT(&options, GRN_OBJ_VECTOR);

  name_size = grn_obj_name(ctx, table, name, sizeof(name));
  if (name_size == 0) {
    grn_strcpy(name, sizeof(name), "(anonymous)");
    name_size = strlen(name);
  }

  if (GRN_TEXT_LEN(modules) == 0) {
    goto exit;
  }

  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expression, unused);
  grn_expr_parse(ctx,
                 expression,
                 GRN_TEXT_VALUE(modules),
                 GRN_TEXT_LEN(modules),
                 NULL,
                 GRN_OP_MATCH,
                 GRN_OP_AND,
                 GRN_EXPR_SYNTAX_SCRIPT);
  if (ctx->rc != GRN_SUCCESS) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] failed to parse %s: <%.*s>: %s",
        context_tag,
        (int)name_size,
        name,
        module_name,
        (int)GRN_TEXT_LEN(modules),
        GRN_TEXT_VALUE(modules),
        errbuf);
    goto exit;
  }
  if (!grn_expr_is_module_list(ctx, expression)) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must be %s(option1, option2, ...), ... format: <%.*s>",
        context_tag,
        (int)name_size,
        name,
        module_name,
        (int)GRN_TEXT_LEN(modules),
        GRN_TEXT_VALUE(modules));
    goto exit;
  }

  n = grn_expr_module_list_get_n_modules(ctx, expression);
  for (i = 0; i < n; i++) {
    grn_obj *proc;
    grn_id proc_id;

    proc = grn_expr_module_list_get_function(ctx, expression, i);
    GRN_BULK_REWIND(&options);
    grn_expr_module_list_get_arguments(ctx, expression, i, &options);

    bool is_valid_proc = false;
    switch (type) {
    case GRN_INFO_NORMALIZERS:
      is_valid_proc = grn_obj_is_normalizer_proc(ctx, proc);
      break;
    case GRN_INFO_TOKEN_FILTERS:
      is_valid_proc = grn_obj_is_token_filter_proc(ctx, proc);
      break;
    default:
      break;
    }

    if (!is_valid_proc) {
      char proc_name[GRN_TABLE_MAX_KEY_SIZE];
      unsigned int proc_name_size;

      proc_name_size = grn_obj_name(ctx, proc, proc_name, sizeof(proc_name));
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] invalid %s: <%.*s>",
          context_tag,
          (int)name_size,
          name,
          module_name,
          (int)proc_name_size,
          proc_name);
      goto exit;
    }

    proc_id = grn_obj_id(ctx, proc);

    grn_table_modules_add(ctx, table_modules, proc);

    switch (type) {
    case GRN_INFO_NORMALIZERS:
      grn_table_set_normalizers_options(ctx, table, i, &options);
      break;
    case GRN_INFO_TOKEN_FILTERS:
      GRN_PTR_PUT(ctx, procs, proc);
      grn_table_set_token_filters_options(ctx, table, i, &options);
      break;
    default:
      break;
    }
  }

exit:
  GRN_OBJ_FIN(ctx, &options);

  if (expression) {
    grn_obj_close(ctx, expression);
  }

  return ctx->rc;
}

static grn_rc
grn_obj_set_info_table_modules_vector(grn_ctx *ctx,
                                      grn_obj *table,
                                      grn_info_type type,
                                      grn_obj *table_modules,
                                      grn_obj *procs,
                                      grn_obj *modules,
                                      const char *context_tag,
                                      const char *module_name)
{
  bool is_names = grn_obj_is_vector(ctx, modules);
  unsigned int n_modules;
  if (is_names) {
    n_modules = grn_vector_size(ctx, modules);
  } else {
    n_modules = GRN_PTR_VECTOR_SIZE(modules);
  }

  unsigned int i;
  for (i = 0; i < n_modules; i++) {
    grn_obj name_buffer;
    grn_obj *module;
    if (is_names) {
      const char *name;
      unsigned int name_size =
        grn_vector_get_element(ctx, modules, i, &name, NULL, NULL);
      GRN_TEXT_INIT(&name_buffer, GRN_OBJ_DO_SHALLOW_COPY);
      GRN_TEXT_SET(ctx, &name_buffer, name, name_size);
      module = &name_buffer;
    } else {
      module = GRN_PTR_VALUE_AT(modules, i);
    }

    unsigned int current_table_modules_size = GRN_BULK_VSIZE(table_modules);
    grn_bulk_space(ctx, table_modules, sizeof(grn_table_module));
    grn_table_module *table_module =
      ((grn_table_module *)GRN_BULK_HEAD(table_modules)) + i;
    grn_table_module_init(ctx, table_module, GRN_ID_NIL);
    grn_id proc_id = GRN_ID_NIL;
    grn_obj_set_info_table_module_raw(ctx,
                                      table,
                                      type,
                                      table_module,
                                      &proc_id,
                                      i,
                                      module,
                                      context_tag,
                                      module_name);
    if (ctx->rc != GRN_SUCCESS) {
      grn_table_module_fin(ctx, table_module);
      grn_bulk_truncate(ctx, table_modules, current_table_modules_size);
      return ctx->rc;
    }
    if (procs) {
      GRN_PTR_PUT(ctx, procs, table_module->proc);
    }

    if (module == &name_buffer) {
      GRN_OBJ_FIN(ctx, &name_buffer);
    }
  }

  return ctx->rc;
}

static grn_rc
grn_obj_set_info_table_modules(grn_ctx *ctx,
                               grn_obj *table,
                               grn_info_type type,
                               grn_obj *modules,
                               const char *context_tag,
                               const char *module_name,
                               const char *module_id)
{
  if (grn_obj_set_info_require_key_table(ctx, table, context_tag) !=
      GRN_SUCCESS) {
    return ctx->rc;
  }

  grn_obj *table_modules = NULL;
  grn_obj *procs = NULL;
  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY:
    switch (type) {
    case GRN_INFO_NORMALIZERS:
      table_modules = &(((grn_hash *)table)->normalizers);
      break;
    case GRN_INFO_TOKEN_FILTERS:
      table_modules = &(((grn_hash *)table)->token_filters);
      procs = &(((grn_hash *)table)->token_filter_procs);
      break;
    default:
      break;
    }
    break;
  case GRN_TABLE_PAT_KEY:
    switch (type) {
    case GRN_INFO_NORMALIZERS:
      table_modules = &(((grn_pat *)table)->normalizers);
      break;
    case GRN_INFO_TOKEN_FILTERS:
      table_modules = &(((grn_pat *)table)->token_filters);
      procs = &(((grn_pat *)table)->token_filter_procs);
      break;
    default:
      break;
    }
    break;
  case GRN_TABLE_DAT_KEY:
    switch (type) {
    case GRN_INFO_NORMALIZERS:
      table_modules = &(((grn_dat *)table)->normalizers);
      break;
    case GRN_INFO_TOKEN_FILTERS:
      table_modules = &(((grn_dat *)table)->token_filters);
      procs = &(((grn_dat *)table)->token_filter_procs);
      break;
    default:
      break;
    }
  default:
    break;
  }

  grn_table_modules_rewind(ctx, table_modules);
  if (procs) {
    GRN_BULK_REWIND(procs);
  }

  if (grn_obj_is_text_family_bulk(ctx, modules)) {
    grn_obj_set_info_table_modules_text(ctx,
                                        table,
                                        type,
                                        table_modules,
                                        procs,
                                        modules,
                                        context_tag,
                                        module_name);
  } else {
    grn_obj_set_info_table_modules_vector(ctx,
                                          table,
                                          type,
                                          table_modules,
                                          procs,
                                          modules,
                                          context_tag,
                                          module_name);
  }
  if (ctx->rc != GRN_SUCCESS) {
    return ctx->rc;
  }

  {
    grn_obj names;
    GRN_TEXT_INIT(&names, 0);
    size_t n = GRN_BULK_VSIZE(table_modules) / sizeof(grn_table_module);
    size_t i;
    for (i = 0; i < n; i++) {
      grn_obj *proc =
        ((grn_table_module *)GRN_BULK_HEAD(table_modules))[i].proc;

      if (i > 0) {
        GRN_TEXT_PUTC(ctx, &names, ',');
      }

      char name[GRN_TABLE_MAX_KEY_SIZE];
      unsigned int name_size =
        grn_obj_name(ctx, proc, name, GRN_TABLE_MAX_KEY_SIZE);
      GRN_TEXT_PUT(ctx, &names, name, name_size);
    }
    grn_id table_id = DB_OBJ(table)->id;
    grn_log_level log_level =
      (table_id & GRN_OBJ_TMP_OBJECT) ? GRN_LOG_DEBUG : GRN_LOG_NOTICE;
    GRN_LOG(ctx,
            log_level,
            "DDL:%u:set_%s%s%.*s",
            DB_OBJ(table)->id,
            module_id,
            n == 0 ? "" : " ",
            (int)GRN_TEXT_LEN(&names),
            GRN_TEXT_VALUE(&names));
    GRN_OBJ_FIN(ctx, &names);
  }
  grn_obj_spec_save(ctx, DB_OBJ(table));

  return GRN_SUCCESS;
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
  grn_obj value_buffer;
  switch (type) {
  case GRN_INFO_SOURCE:
    if (!GRN_DB_OBJP(obj)) {
      ERR(GRN_INVALID_ARGUMENT, "only db_obj can accept GRN_INFO_SOURCE");
      goto exit;
    }
    rc = grn_obj_set_info_source(ctx, obj, value);
    break;
  case GRN_INFO_DEFAULT_TOKENIZER:
    rc = grn_obj_set_info_table_module(ctx,
                                       obj,
                                       type,
                                       value,
                                       "[info][set][default-tokenizer]",
                                       "tokenizer");
    break;
  case GRN_INFO_NORMALIZER:
    rc = grn_obj_set_info_table_module(ctx,
                                       obj,
                                       type,
                                       NULL,
                                       "[info][set][normalizer]",
                                       "normalizer");
    if (rc != GRN_SUCCESS) {
      break;
    }
    type = GRN_INFO_NORMALIZERS;
    if (!grn_obj_is_text_family_bulk(ctx, value)) {
      GRN_PTR_INIT(&value_buffer, GRN_OBJ_VECTOR, GRN_ID_NIL);
      GRN_PTR_PUT(ctx, &value_buffer, value);
      value = &value_buffer;
    }
    /* FALLTHROUGH */
  case GRN_INFO_NORMALIZERS:
    rc = grn_obj_set_info_table_modules(ctx,
                                        obj,
                                        type,
                                        value,
                                        "[info][set][normalizers]",
                                        "normalizers",
                                        "normalizers");
    break;
  case GRN_INFO_TOKEN_FILTERS:
    rc = grn_obj_set_info_table_modules(ctx,
                                        obj,
                                        type,
                                        value,
                                        "[info][set][token-filters]",
                                        "token filters",
                                        "token_filters");
    break;
  default:
    /* todo */
    break;
  }
  if (value == &value_buffer) {
    GRN_OBJ_FIN(ctx, &value_buffer);
  }
exit:
  GRN_API_RETURN(rc);
}

grn_obj *
grn_obj_get_element_info(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_info_type type, grn_obj *valuebuf)
{
  GRN_API_ENTER;
  GRN_API_RETURN(valuebuf);
}

grn_rc
grn_obj_set_element_info(
  grn_ctx *ctx, grn_obj *obj, grn_id id, grn_info_type type, grn_obj *value)
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
grn_obj_add_hook(grn_ctx *ctx,
                 grn_obj *obj,
                 grn_hook_entry entry,
                 int offset,
                 grn_obj *proc,
                 grn_obj *hld)
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
      grn_memcpy(GRN_NEXT_ADDR(new), hld_value, hld_size);
    }
    for (i = 0; i != offset && *last; i++) {
      last = &(*last)->next;
    }
    new->next = *last;
    *last = new;
    if (grn_logger_pass(ctx, GRN_LOG_NOTICE)) {
      grn_id id = DB_OBJ(obj)->id;
      uint32_t name_size = 0;
      const char *name = _grn_table_key(ctx, ctx->impl->db, id, &name_size);
      grn_obj extra_info;
      GRN_TEXT_INIT(&extra_info, 0);
      GRN_TEXT_PUTS(ctx, &extra_info, " [");
      grn_hook *hook;
      for (hook = new; hook; hook = hook->next) {
        if (hook != new) {
          GRN_TEXT_PUTC(ctx, &extra_info, ',');
        }
        if (!hook->proc &&
            hook->hld_size == sizeof(grn_obj_default_set_value_hook_data)) {
          grn_obj_default_set_value_hook_data *data =
            (grn_obj_default_set_value_hook_data *)GRN_NEXT_ADDR(hook);
          grn_table_get_key2(ctx, ctx->impl->db, data->target, &extra_info);
          grn_text_printf(ctx, &extra_info, "(%u)", data->target);
        } else {
          grn_id hook_proc_id = DB_OBJ(hook->proc)->id;
          grn_table_get_key2(ctx, ctx->impl->db, hook_proc_id, &extra_info);
          grn_text_printf(ctx, &extra_info, "(%u)", hook_proc_id);
        }
      }
      GRN_TEXT_PUTS(ctx, &extra_info, "]");
      GRN_LOG(ctx,
              GRN_LOG_NOTICE,
              "DDL:%u:add_hook:%s%s%.*s%.*s",
              id,
              grn_hook_entry_to_string(entry),
              name_size == 0 ? "" : " ",
              (int)name_size,
              name,
              (int)GRN_TEXT_LEN(&extra_info),
              GRN_TEXT_VALUE(&extra_info));
      GRN_OBJ_FIN(ctx, &extra_info);
    }
    grn_obj_spec_save(ctx, DB_OBJ(obj));
  }
exit:
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
grn_obj_get_hook(
  grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset, grn_obj *hldbuf)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  {
    int i;
    grn_hook *hook = DB_OBJ(obj)->hooks[entry];
    for (i = 0; i < offset; i++) {
      hook = hook->next;
      if (!hook) {
        return NULL;
      }
    }
    res = (grn_obj *)hook->proc;
    grn_bulk_write(ctx, hldbuf, (char *)GRN_NEXT_ADDR(hook), hook->hld_size);
  }
  GRN_API_RETURN(res);
}

grn_rc
grn_obj_delete_hook(grn_ctx *ctx,
                    grn_obj *obj,
                    grn_hook_entry entry,
                    int offset)
{
  GRN_API_ENTER;
  grn_obj extra_info;
  GRN_TEXT_INIT(&extra_info, 0);
  bool removed = false;
  {
    int i = 0;
    grn_hook *h, **last = &DB_OBJ(obj)->hooks[entry];
    for (;;) {
      if (!(h = *last)) {
        return GRN_INVALID_ARGUMENT;
      }
      if (++i > offset) {
        removed = true;
        if (!h->proc &&
            h->hld_size == sizeof(grn_obj_default_set_value_hook_data)) {
          grn_obj_default_set_value_hook_data *data =
            (grn_obj_default_set_value_hook_data *)GRN_NEXT_ADDR(h);
          GRN_TEXT_PUTC(ctx, &extra_info, ' ');
          grn_table_get_key2(ctx, ctx->impl->db, data->target, &extra_info);
          grn_text_printf(ctx, &extra_info, "(%u)", data->target);
        }
        break;
      }
      last = &h->next;
    }
    *last = h->next;
    GRN_FREE(h);
  }
  if (removed) {
    grn_id id = DB_OBJ(obj)->id;
    uint32_t name_size = 0;
    const char *name = _grn_table_key(ctx, ctx->impl->db, id, &name_size);
    GRN_TEXT_PUTS(ctx, &extra_info, " [");
    grn_hook *first_hook = DB_OBJ(obj)->hooks[entry];
    grn_hook *hook;
    for (hook = first_hook; hook; hook = hook->next) {
      if (hook != first_hook) {
        GRN_TEXT_PUTC(ctx, &extra_info, ',');
      }
      if (!hook->proc &&
          hook->hld_size == sizeof(grn_obj_default_set_value_hook_data)) {
        grn_obj_default_set_value_hook_data *data =
          (grn_obj_default_set_value_hook_data *)GRN_NEXT_ADDR(hook);
        grn_table_get_key2(ctx, ctx->impl->db, data->target, &extra_info);
        grn_text_printf(ctx, &extra_info, "(%u)", data->target);
      } else {
        grn_id hook_proc_id = DB_OBJ(hook->proc)->id;
        grn_table_get_key2(ctx, ctx->impl->db, hook_proc_id, &extra_info);
        grn_text_printf(ctx, &extra_info, "(%u)", hook_proc_id);
      }
    }
    GRN_TEXT_PUTS(ctx, &extra_info, "]");
    GRN_LOG(ctx,
            GRN_LOG_NOTICE,
            "DDL:%u:delete_hook:%s%s%.*s%.*s",
            id,
            grn_hook_entry_to_string(entry),
            name_size == 0 ? "" : " ",
            (int)name_size,
            name,
            (int)GRN_TEXT_LEN(&extra_info),
            GRN_TEXT_VALUE(&extra_info));
  }
  GRN_OBJ_FIN(ctx, &extra_info);
  grn_obj_spec_save(ctx, DB_OBJ(obj));
  GRN_API_RETURN(GRN_SUCCESS);
}

static grn_rc
remove_index(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, uint32_t flags)
{
  const char *tag = "[object][remove][index]";
  grn_rc rc = GRN_SUCCESS;
  grn_hook *hooks = DB_OBJ(obj)->hooks[entry];
  DB_OBJ(obj)->hooks[entry] = NULL; /* avoid mutual recursive call */
  if (!hooks) {
    return rc;
  }
  GRN_DEFINE_NAME(obj);
  while (hooks) {
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    if (!target) {
      char hook_name[GRN_TABLE_MAX_KEY_SIZE];
      int hook_name_length;

      hook_name_length = grn_table_get_key(ctx,
                                           ctx->impl->db,
                                           data->target,
                                           hook_name,
                                           GRN_TABLE_MAX_KEY_SIZE);
      ERR(GRN_OBJECT_CORRUPT,
          "%s[%s][%s] hook has a dangling reference: <%.*s>(%u)",
          tag,
          name,
          grn_hook_entry_to_string(entry),
          hook_name_length,
          hook_name,
          data->target);
      if (flags & GRN_OBJ_REMOVE_ENSURE) {
        ERRCLR(ctx);
        if (data->target != GRN_ID_NIL) {
          grn_ctx_remove_by_id(ctx, data->target, flags);
        }
      }
      rc = ctx->rc;
    } else if (target->header.type == GRN_COLUMN_INDEX) {
      // TODO: multicolumn  MULTI_COLUMN_INDEXP
      rc = grn_obj_remove_internal(ctx, target, flags);
    } else {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect_limited(ctx, &inspected, target);
      ERR(GRN_UNKNOWN_ERROR,
          "%s[%s][%s] hook has an unsupported index target: %u: %.*s",
          tag,
          name,
          grn_hook_entry_to_string(entry),
          data->target,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      rc = ctx->rc;
    }
    if (rc != GRN_SUCCESS) {
      DB_OBJ(obj)->hooks[entry] = hooks;
      break;
    }
    grn_hook *current_hook = hooks;
    hooks = hooks->next;
    GRN_FREE(current_hook);
  }
  return rc;
}

static grn_rc
grn_ctx_remove_by_id_internal(grn_ctx *ctx,
                              grn_id id,
                              uint32_t flags,
                              bool is_top_level);

/* This should be used only when the target object can't be opened. If
 * the target object can be opened, use remove_index() instead. */
static grn_rc
remove_index_full_scan(grn_ctx *ctx, grn_id target_id, uint32_t flags)
{
  GRN_DB_EACH_SPEC_BEGIN(ctx, cursor, id, spec)
  {
    if (id == target_id) {
      continue;
    }
    if (grn_obj_type_is_column(spec->header.type)) {
      /* TODO: We should not access to internal variable in
       * GRN_DB_EACH_SPEC_BEGIN(). */
      const char *raw_source_ids;
      uint32_t size = grn_vector_get_element(ctx,
                                             &decoded_spec,
                                             GRN_SERIALIZED_SPEC_INDEX_SOURCE,
                                             &raw_source_ids,
                                             NULL,
                                             NULL);
      if (size > 0) {
        grn_id *source_ids = (grn_id *)raw_source_ids;
        size_t n_ids = size / sizeof(grn_id);
        size_t i;
        for (i = 0; i < n_ids; i++) {
          grn_id source_id = source_ids[i];
          if (source_id == target_id) {
            grn_ctx_remove_by_id_internal(ctx, id, flags, false);
            break;
          }
        }
      }
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
  }
  GRN_DB_EACH_SPEC_END(ctx, cursor);
  return ctx->rc;
}

static grn_rc
remove_columns_raw(grn_ctx *ctx,
                   grn_id table_id,
                   const char *table_name,
                   uint32_t table_name_size,
                   uint32_t flags)
{
  const char *tag = "[table][remove][columns]";
  grn_rc rc = GRN_SUCCESS;

  char prefix[GRN_TABLE_MAX_KEY_SIZE];
  grn_strncpy(prefix, GRN_TABLE_MAX_KEY_SIZE, table_name, table_name_size);
  prefix[table_name_size] = GRN_DB_DELIMITER;
  GRN_TABLE_EACH_BEGIN_MIN(ctx,
                           ctx->impl->db,
                           cursor,
                           column_id,
                           prefix,
                           table_name_size + 1 /* GRN_DB_DELIMITER */,
                           GRN_CURSOR_PREFIX)
  {
    grn_obj *column = grn_ctx_at(ctx, column_id);
    if (column) {
      rc = grn_obj_remove_internal(ctx, column, flags);
      if (rc != GRN_SUCCESS) {
        grn_obj_unlink(ctx, column);
        break;
      }
    } else {
      uint32_t column_name_size;
      const char *column_name =
        _grn_table_key(ctx, ctx->impl->db, column_id, &column_name_size);
      if (ctx->rc == GRN_SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s] column is broken: <%.*s>(%u)",
            tag,
            (int)table_name_size,
            table_name,
            column_name_size,
            column_name,
            column_id);
      } else {
        char errbuf[GRN_CTX_MSGSIZE];
        grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
        ERR(ctx->rc,
            "%s[%.*s] column is broken: <%.*s>(%u): %s",
            tag,
            (int)table_name_size,
            table_name,
            column_name_size,
            column_name,
            column_id,
            errbuf);
      }
      if (flags & GRN_OBJ_REMOVE_ENSURE) {
        ERRCLR(ctx);
        grn_ctx_remove_by_id(ctx, column_id, flags);
      }
      rc = ctx->rc;
      if (rc != GRN_SUCCESS) {
        break;
      }
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);

  return rc;
}

static grn_rc
remove_columns(grn_ctx *ctx, grn_obj *table, uint32_t flags)
{
  grn_id table_id = DB_OBJ(table)->id;
  uint32_t table_name_size;
  const char *table_name =
    _grn_table_key(ctx, ctx->impl->db, table_id, &table_name_size);
  return remove_columns_raw(ctx, table_id, table_name, table_name_size, flags);
}

static grn_rc
grn_obj_remove_db_index_columns(grn_ctx *ctx, grn_obj *db)
{
  grn_rc rc = GRN_SUCCESS;
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, 0))) {
    grn_id id;
    while ((id = grn_table_cursor_next_inline(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *obj = grn_ctx_at(ctx, id);
      if (obj && obj->header.type == GRN_COLUMN_INDEX) {
        rc = grn_obj_remove_internal(ctx, obj, 0);
        if (rc != GRN_SUCCESS) {
          grn_obj_unlink(ctx, obj);
          break;
        }
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
  return rc;
}

static grn_rc
grn_obj_remove_db_reference_columns(grn_ctx *ctx, grn_obj *db)
{
  grn_rc rc = GRN_SUCCESS;
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, 0))) {
    grn_id id;
    while ((id = grn_table_cursor_next_inline(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *obj = grn_ctx_at(ctx, id);
      grn_obj *range = NULL;

      if (!obj) {
        continue;
      }

      switch (obj->header.type) {
      case GRN_COLUMN_FIX_SIZE:
      case GRN_COLUMN_VAR_SIZE:
        if (!DB_OBJ(obj)->range) {
          break;
        }

        range = grn_ctx_at(ctx, DB_OBJ(obj)->range);
        if (!range) {
          break;
        }

        switch (range->header.type) {
        case GRN_TABLE_NO_KEY:
        case GRN_TABLE_HASH_KEY:
        case GRN_TABLE_PAT_KEY:
        case GRN_TABLE_DAT_KEY:
          rc = grn_obj_remove_internal(ctx, obj, 0);
          break;
        }
        break;
      }

      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
  return rc;
}

static grn_rc
grn_obj_remove_db_reference_tables(grn_ctx *ctx, grn_obj *db)
{
  grn_rc rc = GRN_SUCCESS;
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, 0))) {
    grn_id id;
    while ((id = grn_table_cursor_next_inline(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *obj = grn_ctx_at(ctx, id);
      grn_obj *domain = NULL;

      if (!obj) {
        continue;
      }

      switch (obj->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
        if (!obj->header.domain) {
          break;
        }

        domain = grn_ctx_at(ctx, obj->header.domain);
        if (!domain) {
          break;
        }

        switch (domain->header.type) {
        case GRN_TABLE_NO_KEY:
        case GRN_TABLE_HASH_KEY:
        case GRN_TABLE_PAT_KEY:
        case GRN_TABLE_DAT_KEY:
          rc = grn_obj_remove_internal(ctx, obj, 0);
          break;
        }
        break;
      }

      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
  return rc;
}

static grn_rc
grn_obj_remove_db_all_tables(grn_ctx *ctx, grn_obj *db)
{
  grn_rc rc = GRN_SUCCESS;
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, 0))) {
    grn_id id;
    while ((id = grn_table_cursor_next_inline(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *obj = grn_ctx_at(ctx, id);

      if (!obj) {
        continue;
      }

      switch (obj->header.type) {
      case GRN_TABLE_NO_KEY:
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
        rc = grn_obj_remove_internal(ctx, obj, 0);
        break;
      }

      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
  return rc;
}

static grn_rc
grn_obj_remove_db(
  grn_ctx *ctx, grn_obj *obj, grn_obj *db, grn_id id, const char *path)
{
  grn_rc rc = GRN_SUCCESS;
  const char *io_spath;
  char *spath;
  grn_db *s = (grn_db *)db;
  unsigned char key_type;

  rc = grn_obj_remove_db_index_columns(ctx, db);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_obj_remove_db_reference_columns(ctx, db);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_obj_remove_db_reference_tables(ctx, db);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_obj_remove_db_all_tables(ctx, db);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (s->specs && (io_spath = grn_obj_path(ctx, (grn_obj *)s->specs)) &&
      *io_spath != '\0') {
    if (!(spath = GRN_STRDUP(io_spath))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_spath);
      return ctx->rc;
    }
  } else {
    spath = NULL;
  }

  key_type = s->keys->header.type;

  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    if (spath) {
      GRN_FREE(spath);
    }
    return rc;
  }

  if (spath) {
    rc = grn_ja_remove(ctx, spath);
    GRN_FREE(spath);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  if (path) {
    switch (key_type) {
    case GRN_TABLE_PAT_KEY:
      rc = grn_pat_remove(ctx, path);
      break;
    case GRN_TABLE_DAT_KEY:
      rc = grn_dat_remove(ctx, path);
      break;
    }
    if (rc == GRN_SUCCESS) {
      rc = grn_db_config_remove(ctx, path);
    } else {
      grn_db_config_remove(ctx, path);
    }
    if (rc == GRN_SUCCESS) {
      rc = grn_options_remove(ctx, path);
    } else {
      grn_options_remove(ctx, path);
    }
  }

  return rc;
}

static grn_rc
remove_reference_tables_raw(grn_ctx *ctx,
                            grn_id table_id,
                            const char *table_name,
                            uint32_t table_name_size,
                            grn_obj *db,
                            uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  grn_bool is_close_opened_object_mode = GRN_FALSE;
  grn_table_cursor *cursor;

  if (grn_thread_get_limit() == 1) {
    is_close_opened_object_mode = GRN_TRUE;
  }

  if ((cursor = grn_table_cursor_open(ctx,
                                      db,
                                      NULL,
                                      0,
                                      NULL,
                                      0,
                                      0,
                                      -1,
                                      GRN_CURSOR_BY_ID))) {
    grn_id id;
    while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
      grn_obj *object;
      bool is_removed = false;

      if (is_close_opened_object_mode) {
        grn_ctx_push_temporary_open_space(ctx);
      }

      object = grn_ctx_at(ctx, id);
      if (!object) {
        ERRCLR(ctx);
        if (is_close_opened_object_mode) {
          grn_ctx_pop_temporary_open_space(ctx);
        }
        continue;
      }

      switch (object->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
        if (DB_OBJ(object)->id == table_id) {
          break;
        }

        if (object->header.domain == table_id) {
          rc = grn_obj_remove_internal(ctx, object, flags);
          is_removed = (grn_table_at(ctx, db, id) == GRN_ID_NIL);
        }
        break;
      case GRN_TABLE_NO_KEY:
        break;
      case GRN_COLUMN_VAR_SIZE:
      case GRN_COLUMN_FIX_SIZE:
        if (object->header.domain == table_id) {
          break;
        }
        if (DB_OBJ(object)->range == table_id) {
          rc = grn_obj_remove_internal(ctx, object, flags);
          is_removed = (grn_table_at(ctx, db, id) == GRN_ID_NIL);
        }
        break;
      case GRN_COLUMN_INDEX:
        break;
      default:
        break;
      }

      if (is_close_opened_object_mode) {
        grn_ctx_pop_temporary_open_space(ctx);
      } else {
        if (!is_removed) {
          grn_obj_unlink(ctx, object);
        }
      }

      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    grn_table_cursor_close(ctx, cursor);
  }

  return rc;
}

static grn_rc
remove_reference_tables(grn_ctx *ctx,
                        grn_obj *table,
                        grn_obj *db,
                        uint32_t flags)
{
  grn_id table_id = DB_OBJ(table)->id;
  uint32_t table_name_size;
  const char *table_name = _grn_table_key(ctx, db, table_id, &table_name_size);
  return remove_reference_tables_raw(ctx,
                                     table_id,
                                     table_name,
                                     table_name_size,
                                     db,
                                     flags);
}

static bool
is_removable_table_raw(grn_ctx *ctx,
                       grn_id table_id,
                       const char *table_name,
                       uint32_t table_name_size,
                       grn_obj *db)
{
  grn_id reference_object_id;

  if (table_id & GRN_OBJ_TMP_OBJECT) {
    return true;
  }

  reference_object_id = grn_table_find_reference_object_raw(ctx, table_id);
  if (reference_object_id == GRN_ID_NIL) {
    return true;
  }

  {
    grn_obj *db;
    grn_obj *reference_object;
    const char *reference_object_name;
    int reference_object_name_size;

    db = grn_ctx_db(ctx);

    reference_object = grn_ctx_at(ctx, reference_object_id);
    reference_object_name =
      _grn_table_key(ctx, db, reference_object_id, &reference_object_name_size);
    if (reference_object) {
      if (grn_obj_is_table(ctx, reference_object)) {
        ERR(GRN_OPERATION_NOT_PERMITTED,
            "[table][remove] a table that references the table exists: "
            "<%.*s._key> -> <%.*s>",
            reference_object_name_size,
            reference_object_name,
            table_name_size,
            table_name);
      } else {
        ERR(GRN_OPERATION_NOT_PERMITTED,
            "[table][remove] a column that references the table exists: "
            "<%.*s> -> <%.*s>",
            reference_object_name_size,
            reference_object_name,
            table_name_size,
            table_name);
      }
    } else {
      ERR(GRN_OPERATION_NOT_PERMITTED,
          "[table][remove] a dangling object that references the table exists: "
          "<%.*s(%u)> -> <%.*s>",
          reference_object_name_size,
          reference_object_name,
          reference_object_id,
          table_name_size,
          table_name);
    }
  }

  return false;
}

static bool
is_removable_table(grn_ctx *ctx, grn_obj *table, grn_obj *db)
{
  grn_id table_id = DB_OBJ(table)->id;
  const char *table_name;
  int table_name_size;
  table_name = _grn_table_key(ctx, db, table_id, &table_name_size);
  return is_removable_table_raw(ctx, table_id, table_name, table_name_size, db);
}

static grn_inline void
grn_obj_remove_log_spec_remove(grn_ctx *ctx,
                               grn_obj *db,
                               grn_id id,
                               uint8_t type)
{
  const char *name;
  uint32_t name_size = 0;

  name = _grn_table_key(ctx, db, id, &name_size);
  /* TODO: reduce log level. */
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "spec:%u:remove:%.*s:%u(%s)",
          id,
          name_size,
          name,
          type,
          grn_obj_type_to_string(type));
}

static void
grn_ctx_generate_removing_path(grn_ctx *ctx,
                               grn_id id,
                               char *path,
                               size_t path_size)
{
  const char *suffix = ".removing";
  grn_obj_path_by_id(ctx, ctx->impl->db, id, path);
  grn_strcat(path, path_size, suffix);
}

static void
grn_ctx_mark_removing(grn_ctx *ctx, grn_id id)
{
  char path[PATH_MAX];
  grn_ctx_generate_removing_path(ctx, id, path, PATH_MAX);
  FILE *removing = grn_fopen(path, "w");
  if (removing) {
    fclose(removing);
  }
}

bool
grn_ctx_is_removing(grn_ctx *ctx, grn_id id)
{
  char path[PATH_MAX];
  grn_ctx_generate_removing_path(ctx, id, path, PATH_MAX);
  struct stat s;
  return stat(path, &s) == 0;
}

static void
grn_ctx_done_removing(grn_ctx *ctx, grn_id id)
{
  char path[PATH_MAX];
  grn_ctx_generate_removing_path(ctx, id, path, PATH_MAX);
  grn_unlink(path);
}

static grn_rc
grn_obj_remove_pat(grn_ctx *ctx,
                   grn_obj *obj,
                   grn_obj *db,
                   grn_id id,
                   const char *path,
                   uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  if (flags & GRN_OBJ_REMOVE_DEPENDENT) {
    rc = remove_reference_tables(ctx, obj, db, flags);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  } else {
    if (!is_removable_table(ctx, obj, db)) {
      return ctx->rc;
    }
  }

  rc = remove_index(ctx, obj, GRN_HOOK_INSERT, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = remove_columns(ctx, obj, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_pat_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_dat(grn_ctx *ctx,
                   grn_obj *obj,
                   grn_obj *db,
                   grn_id id,
                   const char *path,
                   uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  if (flags & GRN_OBJ_REMOVE_DEPENDENT) {
    rc = remove_reference_tables(ctx, obj, db, flags);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  } else {
    if (!is_removable_table(ctx, obj, db)) {
      return ctx->rc;
    }
  }

  rc = remove_index(ctx, obj, GRN_HOOK_INSERT, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = remove_columns(ctx, obj, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_dat_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_hash(grn_ctx *ctx,
                    grn_obj *obj,
                    grn_obj *db,
                    grn_id id,
                    const char *path,
                    uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  if (flags & GRN_OBJ_REMOVE_DEPENDENT) {
    rc = remove_reference_tables(ctx, obj, db, flags);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  } else {
    if (!is_removable_table(ctx, obj, db)) {
      return ctx->rc;
    }
  }

  rc = remove_index(ctx, obj, GRN_HOOK_INSERT, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = remove_columns(ctx, obj, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_hash_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_array(grn_ctx *ctx,
                     grn_obj *obj,
                     grn_obj *db,
                     grn_id id,
                     const char *path,
                     uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  if (flags & GRN_OBJ_REMOVE_DEPENDENT) {
    rc = remove_reference_tables(ctx, obj, db, flags);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  } else {
    if (!is_removable_table(ctx, obj, db)) {
      return ctx->rc;
    }
  }

  rc = remove_columns(ctx, obj, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_array_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_ja(grn_ctx *ctx,
                  grn_obj *obj,
                  grn_obj *db,
                  grn_id id,
                  const char *path,
                  uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  delete_source_hook(ctx, obj);
  rc = remove_index(ctx, obj, GRN_HOOK_SET, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_ja_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_ra(grn_ctx *ctx,
                  grn_obj *obj,
                  grn_obj *db,
                  grn_id id,
                  const char *path,
                  uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  rc = remove_index(ctx, obj, GRN_HOOK_SET, flags);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_ra_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_index(grn_ctx *ctx,
                     grn_obj *obj,
                     grn_obj *db,
                     grn_id id,
                     const char *path,
                     uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  delete_source_hook(ctx, obj);
  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_ii_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_ctx_done_removing(ctx, id);
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_touch(ctx, db, NULL);
  }

  return rc;
}

static grn_rc
grn_obj_remove_db_obj(
  grn_ctx *ctx, grn_obj *obj, grn_obj *db, grn_id id, const char *path)
{
  grn_rc rc = GRN_SUCCESS;
  uint8_t type;

  type = obj->header.type;

  rc = grn_obj_close(ctx, obj);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (path) {
    grn_ctx_mark_removing(ctx, id);
    rc = grn_io_remove(ctx, path);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  if (!(id & GRN_OBJ_TMP_OBJECT)) {
    grn_obj_remove_log_spec_remove(ctx, db, id, type);
    rc = grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    grn_obj_touch(ctx, db, NULL);
  }

  if (path) {
    grn_ctx_done_removing(ctx, id);
  }

  return rc;
}

static grn_rc
grn_obj_remove_other(
  grn_ctx *ctx, grn_obj *obj, grn_obj *db, grn_id id, const char *path)
{
  return grn_obj_close(ctx, obj);
}

static grn_rc
grn_obj_remove_internal(grn_ctx *ctx, grn_obj *obj, uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  grn_id id = GRN_ID_NIL;
  grn_obj *db = NULL;
  const char *io_path;
  char *path;
  grn_bool is_temporary_open_target = GRN_FALSE;

  if (ctx->impl && ctx->impl->db) {
    grn_id id;
    uint32_t s = 0;
    const char *n;

    id = DB_OBJ(obj)->id;
    n = _grn_table_key(ctx, ctx->impl->db, id, &s);
    if (s > 0) {
      GRN_LOG(ctx, GRN_LOG_NOTICE, "DDL:%u:obj_remove %.*s", id, s, n);
    }
  }
  if (obj->header.type != GRN_PROC && (io_path = grn_obj_path(ctx, obj)) &&
      *io_path != '\0') {
    if (!(path = GRN_STRDUP(io_path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_path);
      return ctx->rc;
    }
  } else {
    path = NULL;
  }
  if (GRN_DB_OBJP(obj)) {
    id = DB_OBJ(obj)->id;
    db = DB_OBJ(obj)->db;
    rc = grn_obj_clear_option_values(ctx, obj);
  }
  switch (obj->header.type) {
  case GRN_DB:
    rc = grn_obj_remove_db(ctx, obj, db, id, path);
    break;
  case GRN_TABLE_PAT_KEY:
    grn_ctx_impl_columns_cache_delete(ctx, id);
    rc = grn_obj_remove_pat(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  case GRN_TABLE_DAT_KEY:
    grn_ctx_impl_columns_cache_delete(ctx, id);
    rc = grn_obj_remove_dat(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  case GRN_TABLE_HASH_KEY:
    grn_ctx_impl_columns_cache_delete(ctx, id);
    rc = grn_obj_remove_hash(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  case GRN_TABLE_NO_KEY:
    grn_ctx_impl_columns_cache_delete(ctx, id);
    rc = grn_obj_remove_array(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  case GRN_COLUMN_VAR_SIZE:
    grn_ctx_impl_columns_cache_delete(ctx, obj->header.domain);
    rc = grn_obj_remove_ja(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  case GRN_COLUMN_FIX_SIZE:
    grn_ctx_impl_columns_cache_delete(ctx, obj->header.domain);
    rc = grn_obj_remove_ra(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  case GRN_COLUMN_INDEX:
    grn_ctx_impl_columns_cache_delete(ctx, obj->header.domain);
    rc = grn_obj_remove_index(ctx, obj, db, id, path, flags);
    is_temporary_open_target = GRN_TRUE;
    break;
  default:
    if (GRN_DB_OBJP(obj)) {
      rc = grn_obj_remove_db_obj(ctx, obj, db, id, path);
    } else {
      rc = grn_obj_remove_other(ctx, obj, db, id, path);
    }
  }
  if (path) {
    GRN_FREE(path);
  } else {
    is_temporary_open_target = GRN_FALSE;
  }

  if (is_temporary_open_target && rc == GRN_SUCCESS) {
    grn_obj *space;
    space = ctx->impl->temporary_open_spaces.current;
    if (space) {
      unsigned int i, n_elements;
      n_elements = GRN_PTR_VECTOR_SIZE(space);
      for (i = 0; i < n_elements; i++) {
        if (GRN_PTR_VALUE_AT(space, i) == obj) {
          GRN_PTR_VALUE_AT(space, i) = NULL;
        }
      }
    }
  }

  return rc;
}

grn_rc
grn_obj_remove(grn_ctx *ctx, grn_obj *obj)
{
  return grn_obj_remove_flags(ctx, obj, 0);
}

grn_rc
grn_obj_remove_dependent(grn_ctx *ctx, grn_obj *obj)
{
  return grn_obj_remove_flags(ctx, obj, GRN_OBJ_REMOVE_DEPENDENT);
}

grn_rc
grn_obj_remove_flags(grn_ctx *ctx, grn_obj *obj, uint32_t flags)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  if (ctx->impl && ctx->impl->db && ctx->impl->db != obj) {
    grn_io *io = grn_obj_get_io(ctx, ctx->impl->db);
    rc = grn_io_lock(ctx, io, grn_lock_timeout);
    if (rc == GRN_SUCCESS) {
      rc = grn_obj_remove_internal(ctx, obj, flags);
      grn_io_unlock(ctx, io);
    }
  } else {
    rc = grn_obj_remove_internal(ctx, obj, flags);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_remove_force(grn_ctx *ctx, const char *name, int name_size)
{
  grn_obj *db;

  GRN_API_ENTER;

  if (!(ctx->impl && ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[object][remove][force] database isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  db = ctx->impl->db;
  if (name_size == -1) {
    name_size = strlen(name);
  }
  grn_id id = grn_table_get(ctx, db, name, name_size);
  if (id == GRN_ID_NIL) {
    ERR(GRN_INVALID_ARGUMENT,
        "[object][remove][force] nonexistent object: <%.*s>",
        name_size,
        name);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj_delete_by_id(ctx, db, id, true);
  {
    char path[PATH_MAX];
    grn_obj_path_by_id(ctx, db, id, path);
    grn_io_remove_if_exist(ctx, path);
    grn_strcat(path, PATH_MAX, ".c");
    grn_io_remove_if_exist(ctx, path);
  }

  GRN_API_RETURN(ctx->rc);
}

static uint8_t
grn_ctx_get_type(grn_ctx *ctx, grn_id id)
{
  grn_ja *specs = ((grn_db *)(ctx->impl->db))->specs;
  grn_io_win iw;
  uint32_t encoded_spec_size;
  void *encoded_spec = grn_ja_ref(ctx, specs, id, &iw, &encoded_spec_size);
  if (encoded_spec_size == 0) {
    return GRN_DB_VOID;
  }

  uint8_t type = GRN_DB_VOID;
  grn_obj_spec *spec;
  grn_obj decoded_spec;
  GRN_TEXT_INIT(&decoded_spec, GRN_OBJ_VECTOR);
  if (grn_obj_spec_unpack(ctx,
                          id,
                          encoded_spec,
                          encoded_spec_size,
                          &spec,
                          &decoded_spec,
                          "[ctx][is-table]")) {
    type = spec->header.type;
  }
  GRN_OBJ_FIN(ctx, &decoded_spec);
  grn_ja_unref(ctx, &iw);

  return type;
}

static grn_rc
grn_ctx_remove_internal(
  grn_ctx *ctx, grn_id id, const char *name, uint32_t name_size, uint32_t flags)
{
  grn_obj *db = ctx->impl->db;
  uint8_t type = grn_ctx_get_type(ctx, id);
  if (grn_obj_type_is_table(type)) {
    if (name_size > 0) {
      if (flags & GRN_OBJ_REMOVE_DEPENDENT) {
        grn_rc rc =
          remove_reference_tables_raw(ctx, id, name, name_size, db, flags);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      } else {
        if (!is_removable_table_raw(ctx, id, name, name_size, db)) {
          return ctx->rc;
        }
      }
    }

    grn_rc rc = remove_index_full_scan(ctx, id, flags);
    if (rc != GRN_SUCCESS) {
      return rc;
    }

    if (name_size > 0) {
      rc = remove_columns_raw(ctx, id, name, name_size, flags);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
    }
  } else if (grn_obj_type_is_column(type)) {
    switch (type) {
    case GRN_COLUMN_VAR_SIZE:
    case GRN_COLUMN_INDEX:
      delete_source_hook_full_scan(ctx, id, flags);
      if (ctx->rc != GRN_SUCCESS) {
        return ctx->rc;
      }
      break;
    default:
      break;
    }
    grn_rc rc = remove_index_full_scan(ctx, id, flags);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  grn_obj_delete_by_id(ctx, db, id, true);
  {
    char path[PATH_MAX];
    grn_obj_path_by_id(ctx, db, id, path);
    grn_io_remove_if_exist(ctx, path);
    grn_strcat(path, PATH_MAX, ".c");
    grn_io_remove_if_exist(ctx, path);
  }
  grn_ctx_done_removing(ctx, id);

  return ctx->rc;
}

grn_rc
grn_ctx_remove(grn_ctx *ctx, const char *name, int name_size, uint32_t flags)
{
  const char *tag = "[ctx][remove]";

  GRN_API_ENTER;

  if (name_size < 0) {
    name_size = strlen(name);
  }

  if (!(ctx->impl && ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] database isn't initialized",
        tag,
        name_size,
        name);
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj *obj = grn_ctx_get(ctx, name, name_size);
  if (obj) {
    grn_rc rc = grn_obj_remove_flags(ctx, obj, flags);
    if (rc == GRN_SUCCESS) {
      GRN_API_RETURN(rc);
    }
    if (!(flags & GRN_OBJ_REMOVE_ENSURE)) {
      GRN_API_RETURN(rc);
    }
  } else {
    if (!(flags & GRN_OBJ_REMOVE_ENSURE)) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] object doesn't exist",
          tag,
          name_size,
          name);
      GRN_API_RETURN(ctx->rc);
    }
  }
  ERRCLR(ctx);

  grn_obj *db = ctx->impl->db;
  grn_id id = grn_table_get(ctx, db, name, name_size);
  if (id == GRN_ID_NIL) {
    if (!(flags & GRN_OBJ_REMOVE_ENSURE)) {
      ERR(GRN_INVALID_ARGUMENT,
          "[ctx][remove] nonexistent object: <%.*s>",
          name_size,
          name);
    }
    GRN_API_RETURN(ctx->rc);
  }
  grn_rc rc = grn_ctx_remove_internal(ctx, id, name, name_size, flags);
  GRN_API_RETURN(rc);
}

static grn_rc
grn_ctx_remove_by_id_internal(grn_ctx *ctx,
                              grn_id id,
                              uint32_t flags,
                              bool is_top_level)
{
  const char *tag = "[ctx][remove][id]";

  if (id == GRN_ID_NIL) {
    return GRN_SUCCESS;
  }

  grn_obj *obj = grn_ctx_at(ctx, id);
  if (obj) {
    grn_rc rc;
    if (is_top_level) {
      rc = grn_obj_remove_flags(ctx, obj, flags);
    } else {
      rc = grn_obj_remove_internal(ctx, obj, flags);
    }
    if (rc == GRN_SUCCESS) {
      return rc;
    }
    if (!(flags & GRN_OBJ_REMOVE_ENSURE)) {
      return rc;
    }
  } else {
    if (!(flags & GRN_OBJ_REMOVE_ENSURE)) {
      ERR(GRN_INVALID_ARGUMENT, "%s[%u] object doesn't exist", tag, id);
      return ctx->rc;
    }
  }
  ERRCLR(ctx);

  grn_obj *db = ctx->impl->db;
  int name_size;
  const char *name = _grn_table_key(ctx, db, id, &name_size);
  return grn_ctx_remove_internal(ctx, id, name, name_size, flags);
}

grn_rc
grn_ctx_remove_by_id(grn_ctx *ctx, grn_id id, uint32_t flags)
{
  const char *tag = "[ctx][remove][id]";

  GRN_API_ENTER;

  if (!(ctx->impl && ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "%s[%u] database isn't initialized", tag, id);
    GRN_API_RETURN(ctx->rc);
  }

  grn_rc rc = grn_ctx_remove_by_id_internal(ctx, id, flags, true);
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_update_by_id(grn_ctx *ctx,
                       grn_obj *table,
                       grn_id id,
                       const void *dest_key,
                       unsigned int dest_key_size)
{
  grn_rc rc = GRN_OPERATION_NOT_SUPPORTED;
  GRN_API_ENTER;
  if (table->header.type == GRN_TABLE_DAT_KEY) {
    grn_dat *dat = (grn_dat *)table;
    GRN_TABLE_LOCK_BEGIN(ctx, table)
    {
      rc = grn_dat_update_by_id(ctx, dat, id, dest_key, dest_key_size);
    }
    GRN_TABLE_LOCK_END(ctx);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_update(grn_ctx *ctx,
                 grn_obj *table,
                 const void *src_key,
                 unsigned int src_key_size,
                 const void *dest_key,
                 unsigned int dest_key_size)
{
  grn_rc rc = GRN_OPERATION_NOT_SUPPORTED;
  GRN_API_ENTER;
  if (table->header.type == GRN_TABLE_DAT_KEY) {
    rc = grn_dat_update(ctx,
                        (grn_dat *)table,
                        src_key,
                        src_key_size,
                        dest_key,
                        dest_key_size);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_rename(grn_ctx *ctx,
               grn_obj *obj,
               const char *name,
               unsigned int name_size)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (ctx && ctx->impl && GRN_DB_P(ctx->impl->db) && GRN_DB_OBJP(obj) &&
      !IS_TEMP(obj)) {
    grn_db *s = (grn_db *)ctx->impl->db;
    grn_obj *keys = (grn_obj *)s->keys;
    rc = grn_table_update_by_id(ctx, keys, DB_OBJ(obj)->id, name, name_size);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_table_rename(grn_ctx *ctx,
                 grn_obj *table,
                 const char *name,
                 unsigned int name_size)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  grn_hash *cols;

  GRN_API_ENTER;

  if (!GRN_OBJ_TABLEP(table)) {
    char table_name[GRN_TABLE_MAX_KEY_SIZE];
    int table_name_size;
    table_name_size =
      grn_obj_name(ctx, table, table_name, GRN_TABLE_MAX_KEY_SIZE);
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][rename] isn't table: <%.*s> -> <%.*s>",
        table_name_size,
        table_name,
        name_size,
        name);
    goto exit;
  }
  if (IS_TEMP(table)) {
    rc = GRN_INVALID_ARGUMENT;
    ERR(rc,
        "[table][rename] temporary table doesn't have name: "
        "(anonymous) -> <%.*s>",
        name_size,
        name);
    goto exit;
  }

  if ((cols = grn_hash_create(ctx,
                              NULL,
                              sizeof(grn_id),
                              0,
                              GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY))) {
    grn_table_columns(ctx, table, "", 0, (grn_obj *)cols);
    if (!(rc = grn_obj_rename(ctx, table, name, name_size))) {
      grn_id *key;
      char fullname[GRN_TABLE_MAX_KEY_SIZE];
      grn_memcpy(fullname, name, name_size);
      fullname[name_size] = GRN_DB_DELIMITER;
      GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
        grn_obj *col = grn_ctx_at(ctx, *key);
        if (col) {
          int colname_len =
            grn_column_name(ctx,
                            col,
                            fullname + name_size + 1,
                            GRN_TABLE_MAX_KEY_SIZE - name_size - 1);
          if (colname_len) {
            if ((rc = grn_obj_rename(ctx,
                                     col,
                                     fullname,
                                     name_size + 1 + colname_len))) {
              break;
            }
          }
        }
      });
    }
    grn_hash_close(ctx, cols);
  }
exit:
  GRN_API_RETURN(rc);
}

grn_rc
grn_column_rename(grn_ctx *ctx,
                  grn_obj *column,
                  const char *name,
                  unsigned int name_size)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(column)) {
    char fullname[GRN_TABLE_MAX_KEY_SIZE];
    grn_db *s = (grn_db *)DB_OBJ(column)->db;
    int len = grn_table_get_key(ctx,
                                s->keys,
                                DB_OBJ(column)->header.domain,
                                fullname,
                                GRN_TABLE_MAX_KEY_SIZE);
    if (name_size + 1 + len > GRN_TABLE_MAX_KEY_SIZE) {
      ERR(GRN_INVALID_ARGUMENT,
          "[column][rename] too long column name: required name_size(%d) < %d"
          ": <%.*s>.<%.*s>",
          name_size,
          GRN_TABLE_MAX_KEY_SIZE - 1 - len,
          len,
          fullname,
          name_size,
          name);
      goto exit;
    }
    fullname[len] = GRN_DB_DELIMITER;
    grn_memcpy(fullname + len + 1, name, name_size);
    name_size += len + 1;
    rc = grn_obj_rename(ctx, column, fullname, name_size);
    if (rc == GRN_SUCCESS) {
      grn_obj_touch(ctx, column, NULL);
    }
  }
exit:
  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_path_rename(grn_ctx *ctx, const char *old_path, const char *new_path)
{
  GRN_API_ENTER;
  GRN_API_RETURN(GRN_SUCCESS);
}

/* db must be validated by caller */
grn_id
grn_obj_register(grn_ctx *ctx,
                 grn_obj *db,
                 const char *name,
                 unsigned int name_size)
{
  grn_id id = GRN_ID_NIL;
  if (name && name_size) {
    grn_db *s = (grn_db *)db;
    int added;
    if (!(id = grn_table_add(ctx, s->keys, name, name_size, &added))) {
      grn_rc rc;
      char errbuf[GRN_CTX_MSGSIZE];
      rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_NO_MEMORY_AVAILABLE;
      }
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(rc,
          "[object][register] failed to register a name: <%.*s>%s%s%s",
          name_size,
          name,
          ctx->rc == GRN_SUCCESS ? "" : ": <",
          ctx->rc == GRN_SUCCESS ? "" : errbuf,
          ctx->rc == GRN_SUCCESS ? "" : ">");
    } else if (!added) {
      ERR(GRN_INVALID_ARGUMENT,
          "[object][register] already used name was assigned: <%.*s>",
          name_size,
          name);
      id = GRN_ID_NIL;
    }
  } else {
    grn_ctx *target_ctx = ctx;
    while (target_ctx->impl->parent) {
      target_ctx = target_ctx->impl->parent;
    }
    if (target_ctx->impl && target_ctx->impl->values) {
      if (target_ctx != ctx) {
        CRITICAL_SECTION_ENTER(target_ctx->impl->temporary_objects_lock);
      }
      id = grn_array_add(target_ctx, target_ctx->impl->values, NULL);
      if (id == GRN_ID_NIL) {
        grn_rc rc = ctx->rc;
        if (rc == GRN_SUCCESS) {
          rc = GRN_NO_MEMORY_AVAILABLE;
        }
        ERR(rc,
            "[object][register] failed to register a temporary object: %p(%p)",
            target_ctx,
            ctx);
      } else {
        id |= GRN_OBJ_TMP_OBJECT;
      }
      if (target_ctx != ctx) {
        CRITICAL_SECTION_LEAVE(target_ctx->impl->temporary_objects_lock);
      }
    }
  }
  return id;
}

grn_rc
grn_obj_delete_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, bool remove_p)
{
  grn_rc rc = GRN_INVALID_ARGUMENT;
  GRN_API_ENTER;
  if (id) {
    if (id & GRN_OBJ_TMP_OBJECT) {
      grn_ctx *target_ctx = ctx;
      while (target_ctx->impl->parent) {
        target_ctx = target_ctx->impl->parent;
      }
      if (target_ctx->impl) {
        if (id & GRN_OBJ_TMP_COLUMN) {
          if (target_ctx->impl->temporary_columns) {
            if (target_ctx != ctx) {
              CRITICAL_SECTION_ENTER(target_ctx->impl->temporary_objects_lock);
            }
            grn_log_reference_count("%p: delete: %u\n", target_ctx, id);
            rc = grn_pat_delete_by_id(
              target_ctx,
              target_ctx->impl->temporary_columns,
              id & ~(GRN_OBJ_TMP_COLUMN | GRN_OBJ_TMP_OBJECT),
              NULL);
            if (target_ctx != ctx) {
              CRITICAL_SECTION_LEAVE(target_ctx->impl->temporary_objects_lock);
            }
          }
        } else {
          if (target_ctx->impl->values) {
            if (target_ctx != ctx) {
              CRITICAL_SECTION_ENTER(target_ctx->impl->temporary_objects_lock);
            }
            grn_log_reference_count("%p: delete: %u\n", ctx, id);
            rc = grn_array_delete_by_id(target_ctx,
                                        target_ctx->impl->values,
                                        id & ~GRN_OBJ_TMP_OBJECT,
                                        NULL);
            if (target_ctx != ctx) {
              CRITICAL_SECTION_LEAVE(target_ctx->impl->temporary_objects_lock);
            }
          }
        }
      }
    } else {
      db_value *vp;
      grn_db *s = (grn_db *)db;
      if ((vp = grn_tiny_array_at(&s->values, id))) {
        grn_log_reference_count("%p: delete: %u: %u\n", ctx, id, vp->lock);
        GRN_ASSERT(!vp->lock);
        vp->lock = 0;
        vp->ptr = NULL;
        vp->done = 0;
      }
      if (remove_p) {
        rc = grn_ja_put(ctx, s->specs, id, NULL, 0, GRN_OBJ_SET, NULL);
        grn_rc delete_rc = GRN_SUCCESS;
        switch (s->keys->header.type) {
        case GRN_TABLE_PAT_KEY:
          delete_rc = grn_pat_delete_by_id(ctx, (grn_pat *)s->keys, id, NULL);
          break;
        case GRN_TABLE_DAT_KEY:
          delete_rc = grn_dat_delete_by_id(ctx, (grn_dat *)s->keys, id, NULL);
          break;
        }
        if (rc == GRN_SUCCESS) {
          rc = delete_rc;
        }
      } else {
        rc = GRN_SUCCESS;
      }
    }
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_path_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, char *buffer)
{
  grn_rc rc = GRN_SUCCESS;
  GRN_API_ENTER;
  if (!GRN_DB_P(db) || !buffer) {
    rc = GRN_INVALID_ARGUMENT;
  } else {
    grn_db_generate_pathname(ctx, db, id, buffer);
  }
  GRN_API_RETURN(rc);
}

/* db must be validated by caller */
grn_rc
grn_db_obj_init(grn_ctx *ctx, grn_obj *db, grn_id id, grn_db_obj *obj)
{
  grn_rc rc = GRN_SUCCESS;
  if (id) {
    if (id & GRN_OBJ_TMP_OBJECT) {
      grn_ctx *target_ctx = ctx;
      while (target_ctx->impl->parent) {
        target_ctx = target_ctx->impl->parent;
      }
      if (id & GRN_OBJ_TMP_COLUMN) {
        if (target_ctx->impl && target_ctx->impl->temporary_columns) {
          grn_id real_id = id & ~(GRN_OBJ_TMP_COLUMN | GRN_OBJ_TMP_OBJECT);
          if (target_ctx != ctx) {
            CRITICAL_SECTION_ENTER(target_ctx->impl->temporary_objects_lock);
          }
          rc = grn_pat_set_value(target_ctx,
                                 target_ctx->impl->temporary_columns,
                                 real_id,
                                 &obj,
                                 GRN_OBJ_SET);
          if (target_ctx != ctx) {
            CRITICAL_SECTION_LEAVE(target_ctx->impl->temporary_objects_lock);
          }
        }
      } else {
        if (target_ctx->impl && target_ctx->impl->values) {
          if (target_ctx != ctx) {
            CRITICAL_SECTION_ENTER(target_ctx->impl->temporary_objects_lock);
          }
          rc = grn_array_set_value(target_ctx,
                                   target_ctx->impl->values,
                                   id & ~GRN_OBJ_TMP_OBJECT,
                                   &obj,
                                   GRN_OBJ_SET);
          if (target_ctx != ctx) {
            CRITICAL_SECTION_LEAVE(target_ctx->impl->temporary_objects_lock);
          }
        }
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
      grn_log_reference_count("%p: init: %u: %u\n", ctx, id, vp->lock);
    }
  }
  obj->id = id;
  obj->db = db;
  obj->source = NULL;
  obj->source_size = 0;
  {
    grn_hook_entry entry;
    for (entry = 0; entry < GRN_N_HOOK_ENTRIES; entry++) {
      obj->hooks[entry] = NULL;
    }
  }
  grn_obj_spec_save(ctx, obj);
  return rc;
}

/* The size of buffer's must be PATH_MAX */
void
grn_obj_spec_get_path(grn_ctx *ctx,
                      grn_obj_spec *spec,
                      grn_id id,
                      char *buffer,
                      grn_db *db,
                      grn_obj *decoded_spec)
{
  if (spec->header.flags & GRN_OBJ_CUSTOM_NAME) {
    const char *path;
    unsigned int size = grn_vector_get_element(ctx,
                                               decoded_spec,
                                               GRN_SERIALIZED_SPEC_INDEX_PATH,
                                               &path,
                                               NULL,
                                               NULL);
    if (size >= PATH_MAX) {
      ERR(GRN_FILENAME_TOO_LONG,
          "[spec][path] too long path: %u >= %u: <%.*s>",
          size,
          PATH_MAX,
          (int)size,
          path);
    }
    grn_memcpy(buffer, path, size);
    buffer[size] = '\0';
  } else if (spec->header.flags & GRN_OBJ_PERSISTENT) {
    grn_db_generate_pathname(ctx, (grn_obj *)db, id, buffer);
  } else {
    buffer[0] = '\0';
  }
}

#define UNPACK_INFO(spec, decoded_spec)                                        \
  do {                                                                         \
    if (vp->ptr) {                                                             \
      const char *p;                                                           \
      uint32_t size;                                                           \
      grn_db_obj *r = DB_OBJ(vp->ptr);                                         \
      r->header = spec->header;                                                \
      r->id = id;                                                              \
      r->range = spec->range;                                                  \
      r->db = (grn_obj *)s;                                                    \
      size = grn_vector_get_element(ctx,                                       \
                                    decoded_spec,                              \
                                    GRN_SERIALIZED_SPEC_INDEX_SOURCE,          \
                                    &p,                                        \
                                    NULL,                                      \
                                    NULL);                                     \
      if (size) {                                                              \
        if ((r->source = GRN_MALLOC(size))) {                                  \
          grn_memcpy(r->source, p, size);                                      \
          r->source_size = size;                                               \
        }                                                                      \
      }                                                                        \
      size = grn_vector_get_element(ctx,                                       \
                                    decoded_spec,                              \
                                    GRN_SERIALIZED_SPEC_INDEX_HOOK,            \
                                    &p,                                        \
                                    NULL,                                      \
                                    NULL);                                     \
      grn_hook_unpack(ctx, r, p, size);                                        \
    }                                                                          \
  } while (0)

static void
grn_table_modules_unpack(grn_ctx *ctx,
                         grn_obj *spec_vector,
                         uint32_t index,
                         grn_obj *table_modules,
                         grn_obj *table_module_procs,
                         const char *tag)
{
  if (grn_vector_size(ctx, spec_vector) <= index) {
    return;
  }

  grn_table_modules_rewind(ctx, table_modules);

  grn_id *ids;
  unsigned int element_size = grn_vector_get_element(ctx,
                                                     spec_vector,
                                                     index,
                                                     (const char **)(&ids),
                                                     NULL,
                                                     NULL);
  size_t n_ids = element_size / sizeof(grn_id);
  size_t i;
  for (i = 0; i < n_ids; i++) {
    grn_id id = ids[i];
    grn_obj *proc = grn_ctx_at(ctx, id);
    if (!proc) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s nonexistent table module proc ID: %d",
          tag,
          id);
      return;
    }
    grn_table_modules_add(ctx, table_modules, proc);
    if (table_module_procs) {
      GRN_PTR_PUT(ctx, table_module_procs, proc);
    }
  }
}

bool
grn_obj_spec_unpack(grn_ctx *ctx,
                    grn_id id,
                    void *encoded_spec,
                    uint32_t encoded_spec_size,
                    grn_obj_spec **spec,
                    grn_obj *decoded_spec,
                    const char *error_message_tag)
{
  grn_obj *db;
  grn_db *db_raw;
  grn_rc rc;
  uint32_t spec_size;

  db = ctx->impl->db;
  db_raw = (grn_db *)db;

  rc = grn_vector_unpack(ctx,
                         decoded_spec,
                         encoded_spec,
                         encoded_spec_size,
                         0,
                         NULL);
  if (rc != GRN_SUCCESS) {
    const char *name;
    uint32_t name_size;
    name = _grn_table_key(ctx, db, id, &name_size);
    GRN_LOG((ctx),
            GRN_LOG_ERROR,
            "%s: failed to decode spec: <%u>(<%.*s>):<%u>: %s",
            error_message_tag,
            id,
            name_size,
            name,
            encoded_spec_size,
            grn_rc_to_string(rc));
    return false;
  }

  spec_size = grn_vector_get_element(ctx,
                                     decoded_spec,
                                     GRN_SERIALIZED_SPEC_INDEX_SPEC,
                                     (const char **)spec,
                                     NULL,
                                     NULL);
  if (spec_size == 0) {
    const char *name;
    uint32_t name_size;
    name = _grn_table_key(ctx, db, id, &name_size);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s: spec value is empty: <%u>(<%.*s>)",
            error_message_tag,
            id,
            name_size,
            name);
    return false;
  }

  return true;
}

static inline bool
grn_db_value_lock(grn_ctx *ctx,
                  grn_id id,
                  db_value *vp,
                  uint32_t *current_lock_output)
{
  bool is_locked = true;
  uint32_t current_lock = 0;
  uint32_t *lock_pointer = &(vp->lock);
  uint32_t n_trials = 0;
  for (;; n_trials++) {
    GRN_ATOMIC_ADD_EX(lock_pointer, 1, current_lock);
    if (current_lock < GRN_IO_MAX_REF) {
      break;
    }
    GRN_ATOMIC_ADD_EX(lock_pointer, -1, current_lock);
    if (n_trials >= 1000) {
      GRN_LOG(ctx,
              GRN_LOG_NOTICE,
              "[db][value][lock] failed to lock: "
              "n_trials:<%u> "
              "id:<%u> "
              "lock:<%u> "
              "address:<%p>",
              n_trials,
              id,
              vp->lock,
              vp->ptr);
      is_locked = false;
      break;
    }
    GRN_FUTEX_WAIT(lock_pointer);
  }
  grn_log_reference_count("%p: lock: %u: %u\n", ctx, id, current_lock);
  *current_lock_output = current_lock;
  return is_locked;
}

static inline uint32_t
grn_db_value_unlock(grn_ctx *ctx, grn_id id, db_value *vp)
{
  uint32_t current_lock = 0;
  uint32_t *lock_pointer = &(vp->lock);
  GRN_ATOMIC_ADD_EX(lock_pointer, -1, current_lock);
  grn_log_reference_count("%p: unlock: %u: %u\n", ctx, id, current_lock);
  return current_lock;
}

static inline bool
grn_db_value_wait(grn_ctx *ctx, grn_id id, db_value *vp)
{
  bool succeeded = true;
  uint32_t n_trials;
  grn_log_reference_count("%p: wait: start: %u: %u\n", ctx, id, vp->lock);
  for (n_trials = 0; !vp->ptr; n_trials++) {
    if (n_trials >= 3000) {
      GRN_LOG(ctx,
              GRN_LOG_NOTICE,
              "[db][value][wait] failed to wait: "
              "n_trials:<%u> "
              "id:<%u> "
              "lock:<%u> "
              "address:<%p>",
              n_trials,
              id,
              vp->lock,
              vp->ptr);
      succeeded = false;
      break;
    }
    GRN_FUTEX_WAIT(&vp->ptr);
  }
  grn_log_reference_count("%p: wait: done: %u: %u\n", ctx, id, vp->lock);
  return succeeded;
}

static inline bool
grn_obj_type_is_open_close_log_target(grn_ctx *ctx, uint8_t type)
{
  switch (type) {
  case GRN_TYPE:
  case GRN_PROC:
    return false;
  default:
    return true;
  }
}

#define GRN_LOG_REFERENCE_COUNT GRN_LOG_DUMP

static inline bool
grn_reference_count_should_log(grn_ctx *ctx, uint8_t type)
{
  if (!grn_enable_reference_count) {
    return false;
  }
  if (!grn_obj_type_is_open_close_log_target(ctx, type)) {
    return false;
  }
  return grn_logger_pass(ctx, GRN_LOG_REFERENCE_COUNT);
}

grn_obj *
grn_ctx_at(grn_ctx *ctx, grn_id id)
{
  grn_obj *res = NULL;
  if (!ctx || !ctx->impl || !id) {
    return res;
  }
  GRN_API_ENTER;
  if (id & GRN_OBJ_TMP_OBJECT) {
    grn_ctx *target_ctx = ctx;
    while (target_ctx->impl->parent) {
      target_ctx = target_ctx->impl->parent;
    }
    if (id & GRN_OBJ_TMP_COLUMN) {
      if (target_ctx->impl->temporary_columns) {
        grn_id real_id = id & ~(GRN_OBJ_TMP_COLUMN | GRN_OBJ_TMP_OBJECT);
        grn_log_reference_count("%p: at: start: %u\n", target_ctx, real_id);
        grn_obj **tmp_obj;
        uint32_t size;
        tmp_obj =
          (grn_obj **)grn_pat_get_value_(target_ctx,
                                         target_ctx->impl->temporary_columns,
                                         real_id,
                                         &size);
        if (tmp_obj) {
          res = *tmp_obj;
        }
      }
    } else {
      if (target_ctx->impl->values) {
        grn_id real_id = id & ~GRN_OBJ_TMP_OBJECT;
        grn_log_reference_count("%p: at: start: %u\n", target_ctx, real_id);
        grn_obj **tmp_obj;
        tmp_obj =
          _grn_array_get_value(target_ctx, target_ctx->impl->values, real_id);
        if (tmp_obj) {
          res = *tmp_obj;
        }
      }
    }
    if (res) {
      DB_OBJ(res)->reference_count++;
      grn_log_reference_count("%p: at: done: %u: %u\n",
                              target_ctx,
                              DB_OBJ(res)->id,
                              DB_OBJ(res)->reference_count);
    }
  } else {
    grn_db *s = (grn_db *)ctx->impl->db;
    if (s) {
      db_value *vp;
      uint32_t lock = 0;
      if (!(vp = grn_tiny_array_at(&s->values, id))) {
        goto exit;
      }
      /* We can skip opened check with lock for built-in objects
       * because built-in objects are opened on DB open and never
       * closed until DB is closed at least with the current
       * design. So the object is opened (vp->ptr != NULL, vp->ptr is
       * NULL only when no associated object), we can safely return
       * the opened object directly without lock. */
      if (id < GRN_N_RESERVED_TYPES && vp->ptr) {
        res = vp->ptr;
        goto exit;
      }
      grn_log_reference_count("%p: at: start: %u\n", ctx, id);
      if (grn_enable_reference_count) {
        if (!grn_db_value_lock(ctx, id, vp, &lock)) {
          const char *name;
          uint32_t name_size = 0;
          name = _grn_table_key(ctx, (grn_obj *)s, id, &name_size);
          ERR(GRN_NO_LOCKS_AVAILABLE,
              "[at] failed to lock: <%u>(<%.*s>)",
              id,
              name_size,
              name);
          goto exit;
        }
      }
      if (s->specs && !vp->ptr /* && !vp->done */) {
        if (!grn_enable_reference_count) {
          if (!grn_db_value_lock(ctx, id, vp, &lock)) {
            const char *name;
            uint32_t name_size = 0;
            name = _grn_table_key(ctx, (grn_obj *)s, id, &name_size);
            ERR(GRN_NO_LOCKS_AVAILABLE,
                "[at] failed to lock: "
                "<%u>(<%.*s>)",
                id,
                name_size,
                name);
            goto exit;
          }
        }
        if (lock == 0) {
          grn_io_win iw;
          uint32_t encoded_spec_size;
          void *encoded_spec;

          encoded_spec = grn_ja_ref(ctx, s->specs, id, &iw, &encoded_spec_size);
          if (encoded_spec) {
            bool success;
            grn_obj_spec *spec;
            grn_obj decoded_spec;

            GRN_OBJ_INIT(&decoded_spec, GRN_VECTOR, 0, GRN_DB_TEXT);
            success = grn_obj_spec_unpack(ctx,
                                          id,
                                          encoded_spec,
                                          encoded_spec_size,
                                          &spec,
                                          &decoded_spec,
                                          "grn_ctx_at");
            if (success) {
              char buffer[PATH_MAX];
              switch (spec->header.type) {
              case GRN_TYPE:
                vp->ptr = (grn_obj *)grn_type_open(ctx, spec);
                UNPACK_INFO(spec, &decoded_spec);
                break;
              case GRN_TABLE_HASH_KEY:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                vp->ptr = (grn_obj *)grn_hash_open(ctx, buffer);
                if (vp->ptr) {
                  grn_hash *hash = (grn_hash *)(vp->ptr);
                  grn_obj_flags flags = vp->ptr->header.flags;
                  UNPACK_INFO(spec, &decoded_spec);
                  vp->ptr->header.flags = flags;
                  grn_table_modules_unpack(
                    ctx,
                    &decoded_spec,
                    GRN_SERIALIZED_SPEC_INDEX_TOKEN_FILTERS,
                    &(hash->token_filters),
                    &(hash->token_filter_procs),
                    "[at][token-filters]");
                  grn_table_modules_unpack(
                    ctx,
                    &decoded_spec,
                    GRN_SERIALIZED_SPEC_INDEX_NORMALIZERS,
                    &(hash->normalizers),
                    NULL,
                    "[at][normalizers]");
                }
                break;
              case GRN_TABLE_PAT_KEY:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                vp->ptr = (grn_obj *)grn_pat_open(ctx, buffer);
                if (vp->ptr) {
                  grn_pat *pat = (grn_pat *)(vp->ptr);
                  grn_obj_flags flags = vp->ptr->header.flags;
                  UNPACK_INFO(spec, &decoded_spec);
                  vp->ptr->header.flags = flags;
                  grn_table_modules_unpack(
                    ctx,
                    &decoded_spec,
                    GRN_SERIALIZED_SPEC_INDEX_TOKEN_FILTERS,
                    &(pat->token_filters),
                    &(pat->token_filter_procs),
                    "[at][token-filters]");
                  grn_table_modules_unpack(
                    ctx,
                    &decoded_spec,
                    GRN_SERIALIZED_SPEC_INDEX_NORMALIZERS,
                    &(pat->normalizers),
                    NULL,
                    "[at][normalizers]");
                  if (pat->tokenizer.proc) {
                    grn_pat_cache_enable(ctx,
                                         pat,
                                         GRN_TABLE_PAT_KEY_CACHE_SIZE);
                  }
                }
                break;
              case GRN_TABLE_DAT_KEY:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                vp->ptr = (grn_obj *)grn_dat_open(ctx, buffer);
                if (vp->ptr) {
                  grn_dat *dat = (grn_dat *)(vp->ptr);
                  grn_obj_flags flags = vp->ptr->header.flags;
                  UNPACK_INFO(spec, &decoded_spec);
                  vp->ptr->header.flags = flags;
                  grn_table_modules_unpack(
                    ctx,
                    &decoded_spec,
                    GRN_SERIALIZED_SPEC_INDEX_TOKEN_FILTERS,
                    &(dat->token_filters),
                    &(dat->token_filter_procs),
                    "[at][token-filters]");
                  grn_table_modules_unpack(
                    ctx,
                    &decoded_spec,
                    GRN_SERIALIZED_SPEC_INDEX_NORMALIZERS,
                    &(dat->normalizers),
                    NULL,
                    "[at][normalizers]");
                }
                break;
              case GRN_TABLE_NO_KEY:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                vp->ptr = (grn_obj *)grn_array_open(ctx, buffer);
                UNPACK_INFO(spec, &decoded_spec);
                break;
              case GRN_COLUMN_VAR_SIZE:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                vp->ptr = (grn_obj *)grn_ja_open(ctx, buffer);
                UNPACK_INFO(spec, &decoded_spec);
                break;
              case GRN_COLUMN_FIX_SIZE:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                vp->ptr = (grn_obj *)grn_ra_open(ctx, buffer);
                UNPACK_INFO(spec, &decoded_spec);
                break;
              case GRN_COLUMN_INDEX:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                {
                  grn_obj *table = grn_ctx_at(ctx, spec->header.domain);
                  vp->ptr = (grn_obj *)grn_ii_open(ctx, buffer, table);
                  grn_obj_unlink(ctx, table);
                }
                UNPACK_INFO(spec, &decoded_spec);
                break;
              case GRN_PROC:
                grn_obj_spec_get_path(ctx, spec, id, buffer, s, &decoded_spec);
                grn_plugin_register(ctx, buffer);
                if (grn_enable_reference_count && vp->ptr) {
                  /* Registered proc by plugin must not be freed by
                   * grn_obj_unlink() for now. In the future, we may
                   * support reference count for proc registered by
                   * plugin.
                   *
                   * Registered proc by plugin isn't freed by
                   * grn_obj_unlink() in the process that executes
                   * plugin_register. Because plugin doesn't call
                   * grn_obj_unlink() against the returned object by
                   * grn_proc_create().
                   *
                   * Registered proc by plugin is freed by
                   * grn_obj_unlink() without this increment in the
                   * process that does not execute
                   * plugin_register. Because the initial reference
                   * count ("vp->lock = 1") in grn_db_obj_init()
                   * (called from grn_proc_create()) is reused by this
                   * grn_ctx_at(). If we increment the reference count
                   * here, the registered proc by plugin here isn't
                   * freed by grn_obj_unlink() in the process that
                   * does not execute plugin_register. */
                  vp->lock++;
                  grn_log_reference_count("%p: at: proc: increment: %u: %u\n",
                                          ctx,
                                          id,
                                          vp->lock);
                }
                break;
              case GRN_EXPR:
                {
                  const char *p;
                  uint32_t size;
                  uint8_t *u;
                  size = grn_vector_get_element(ctx,
                                                &decoded_spec,
                                                GRN_SERIALIZED_SPEC_INDEX_EXPR,
                                                &p,
                                                NULL,
                                                NULL);
                  u = (uint8_t *)p;
                  vp->ptr = grn_expr_open(ctx, spec, u, u + size);
                }
                break;
              }
              if (vp->ptr) {
                if (grn_reference_count_should_log(ctx, spec->header.type)) {
                  const char *name;
                  uint32_t name_size = 0;
                  name = _grn_table_key(ctx, (grn_obj *)s, id, &name_size);
                  GRN_LOG(ctx,
                          GRN_LOG_REFERENCE_COUNT,
                          "[obj][open] <%u>(<%.*s>):<%u>(<%s>)",
                          id,
                          name_size,
                          name,
                          spec->header.type,
                          grn_obj_type_to_string(spec->header.type));
                }

                switch (vp->ptr->header.type) {
                case GRN_TABLE_HASH_KEY:
                case GRN_TABLE_PAT_KEY:
                case GRN_TABLE_DAT_KEY:
                case GRN_TABLE_NO_KEY:
                case GRN_COLUMN_FIX_SIZE:
                case GRN_COLUMN_VAR_SIZE:
                case GRN_COLUMN_INDEX:
                  {
                    grn_obj *space;
                    space = ctx->impl->temporary_open_spaces.current;
                    if (space) {
                      GRN_PTR_PUT(ctx, space, vp->ptr);
                    }
                  }
                  break;
                }
              } else {
                const char *name;
                uint32_t name_size = 0;
                name = _grn_table_key(ctx, (grn_obj *)s, id, &name_size);
                GRN_LOG(ctx,
                        GRN_LOG_ERROR,
                        "grn_ctx_at: failed to open object: "
                        "<%u>(<%.*s>):<%u>(<%s>)",
                        id,
                        name_size,
                        name,
                        spec->header.type,
                        grn_obj_type_to_string(spec->header.type));
              }
            }
            GRN_OBJ_FIN(ctx, &decoded_spec);
            grn_ja_unref(ctx, &iw);
          }
          if (grn_enable_reference_count) {
            if (!vp->ptr) {
              grn_db_value_unlock(ctx, id, vp);
            }
          } else {
            grn_db_value_unlock(ctx, id, vp);
          }
          vp->done = 1;
          GRN_FUTEX_WAKE(&vp->ptr);
        } else {
          if (!grn_db_value_wait(ctx, id, vp)) {
            const char *name;
            uint32_t name_size = 0;
            name = _grn_table_key(ctx, (grn_obj *)s, id, &name_size);
            grn_log_reference_count(
              "%p: at: done: failed to wait: %u: %u: %p\n",
              ctx,
              id,
              vp->lock,
              vp->ptr);
            ERR(GRN_NO_LOCKS_AVAILABLE,
                "[at] failed to wait: "
                "<%u>(<%.*s>)",
                id,
                name_size,
                name);
            grn_db_value_unlock(ctx, id, vp);
            goto exit;
          }
          if (!grn_enable_reference_count) {
            grn_db_value_unlock(ctx, id, vp);
          }
        }
      }
      res = vp->ptr;
      if (res && res->header.type == GRN_PROC) {
        grn_plugin_ensure_registered(ctx, res);
      }
      grn_log_reference_count("%p: at: done: %u: %u: %p\n",
                              ctx,
                              id,
                              vp->lock,
                              vp->ptr);
    }
  }
exit:
  GRN_API_RETURN(res);
}

bool
grn_ctx_is_opened(grn_ctx *ctx, grn_id id)
{
  bool is_opened = false;

  if (!ctx || !ctx->impl || !id) {
    return false;
  }

  GRN_API_ENTER;
  if (id & GRN_OBJ_TMP_OBJECT) {
    grn_ctx *target_ctx = ctx;
    while (target_ctx->impl->parent) {
      target_ctx = target_ctx->impl->parent;
    }
    if (target_ctx->impl->values) {
      grn_obj **tmp_obj;
      tmp_obj = _grn_array_get_value(target_ctx,
                                     target_ctx->impl->values,
                                     id & ~GRN_OBJ_TMP_OBJECT);
      if (tmp_obj) {
        is_opened = true;
      }
    }
  } else {
    grn_db *s = (grn_db *)ctx->impl->db;
    if (s) {
      db_value *vp;
      vp = grn_tiny_array_at(&s->values, id);
      if (vp && vp->ptr) {
        is_opened = true;
      }
    }
  }
  GRN_API_RETURN(is_opened);
}

grn_obj *
grn_obj_open(grn_ctx *ctx,
             unsigned char type,
             grn_obj_flags flags,
             grn_id domain)
{
  grn_obj *obj = GRN_CALLOC(sizeof(grn_obj));
  if (obj) {
    uint8_t init_flags =
      (uint8_t)(flags & (GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY));
    GRN_OBJ_INIT(obj, type, init_flags, domain);
    obj->header.impl_flags |= GRN_OBJ_ALLOCATED;
  }
  return obj;
}

grn_obj *
grn_obj_graft(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj *new = grn_obj_open(ctx,
                              obj->header.type,
                              obj->header.impl_flags,
                              obj->header.domain);
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
grn_pvector_fin(grn_ctx *ctx, grn_obj *obj)
{
  grn_rc rc;
  if (obj->header.impl_flags & GRN_OBJ_OWN) {
    /*
     * Note that GRN_OBJ_OWN should not be used outside the DB API function
     * because grn_obj_close is a DB API function.
     */
    unsigned int i, n_elements;
    n_elements = GRN_PTR_VECTOR_SIZE(obj);
    for (i = 0; i < n_elements; i++) {
      grn_obj *element = GRN_PTR_VALUE_AT(obj, n_elements - i - 1);
      if (element) {
        grn_obj_close(ctx, element);
      }
    }
  }
  obj->header.type = GRN_VOID;
  rc = grn_bulk_fin(ctx, obj);
  if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) {
    GRN_FREE(obj);
  }
  return rc;
}

typedef void
grn_traverse_func(grn_ctx *ctx, grn_obj *obj, void *user_data);

typedef struct {
  bool is_close_opened_object_mode;
  grn_traverse_func *traverse;
  void *user_data;
  const char *tag;
} grn_obj_traverse_recursive_data;

static void
grn_obj_traverse_recursive_dispatch(grn_ctx *ctx,
                                    grn_obj_traverse_recursive_data *data,
                                    grn_obj *obj);

static void
grn_obj_traverse_recursive_db(grn_ctx *ctx,
                              grn_obj_traverse_recursive_data *data,
                              grn_obj *db)
{
  GRN_TABLE_EACH_BEGIN(ctx, db, cursor, id)
  {
    if (data->is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    grn_obj *object = grn_ctx_at(ctx, id);
    if (grn_obj_is_table(ctx, object)) {
      grn_obj_traverse_recursive_dispatch(ctx, data, object);
    } else {
      if (ctx->rc != GRN_SUCCESS) {
        ERRCLR(ctx);
      }
    }
    if (object) {
      grn_obj_unref(ctx, object);
    }

    if (data->is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }

    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  data->traverse(ctx, db, data->user_data);
}

static void
grn_obj_traverse_recursive_table(grn_ctx *ctx,
                                 grn_obj_traverse_recursive_data *data,
                                 grn_obj *table)
{
  grn_hash *columns = grn_hash_create(ctx,
                                      NULL,
                                      sizeof(grn_id),
                                      0,
                                      GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    char table_name[GRN_TABLE_MAX_KEY_SIZE];
    int table_name_size;
    table_name_size =
      grn_obj_name(ctx, table, table_name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(rc,
        "%s[recursive] "
        "failed to create internal hash table "
        "to store columns: <%.*s>: %s",
        data->tag,
        table_name_size,
        table_name,
        message);
    return;
  }

  if (grn_table_columns(ctx, table, "", 0, (grn_obj *)columns) > 0) {
    GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id)
    {
      void *key;
      if (grn_hash_cursor_get_key(ctx, cursor, &key) == 0) {
        continue;
      }

      if (data->is_close_opened_object_mode) {
        grn_ctx_push_temporary_open_space(ctx);
      }
      grn_id *column_id = key;
      grn_obj *column = grn_ctx_at(ctx, *column_id);
      if (column) {
        grn_obj_traverse_recursive_dispatch(ctx, data, column);
        grn_obj_unref(ctx, column);
      }
      if (data->is_close_opened_object_mode) {
        grn_ctx_pop_temporary_open_space(ctx);
      }

      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    GRN_HASH_EACH_END(ctx, cursor);
  }
  grn_hash_close(ctx, columns);
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  data->traverse(ctx, table, data->user_data);
}

static void
grn_obj_traverse_recursive_column(grn_ctx *ctx,
                                  grn_obj_traverse_recursive_data *data,
                                  grn_obj *column)
{
  data->traverse(ctx, column, data->user_data);
}

static void
grn_obj_traverse_recursive_dispatch(grn_ctx *ctx,
                                    grn_obj_traverse_recursive_data *data,
                                    grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_DB:
    grn_obj_traverse_recursive_db(ctx, data, obj);
    break;
  case GRN_TABLE_NO_KEY:
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    grn_obj_traverse_recursive_table(ctx, data, obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
  case GRN_COLUMN_INDEX:
    grn_obj_traverse_recursive_column(ctx, data, obj);
    break;
  default:
    {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect_limited(ctx, &inspected, obj);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[recursive] "
          "object must be DB, table or column: %.*s",
          data->tag,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    break;
  }
}

static void
grn_obj_traverse_recursive(grn_ctx *ctx,
                           grn_obj_traverse_recursive_data *data,
                           grn_obj *obj)
{
  data->is_close_opened_object_mode = (grn_thread_get_limit() == 1);
  grn_obj_traverse_recursive_dispatch(ctx, data, obj);
}

typedef struct {
  grn_hash *traversed;
  bool is_close_opened_object_mode;
  grn_obj *top_level_object;
  bool for_reference;
  grn_traverse_func *traverse;
  void *user_data;
  const char *tag;
} grn_obj_traverse_recursive_dependent_data;

static bool
grn_obj_traverse_recursive_dependent_need_traverse(
  grn_ctx *ctx, grn_obj_traverse_recursive_dependent_data *data, grn_id id)
{
  int added = 0;
  grn_id traversed_id =
    grn_hash_add(ctx, data->traversed, &id, sizeof(grn_id), NULL, &added);
  if (traversed_id == GRN_ID_NIL) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(rc,
        "%s[recursive][dependent] "
        "failed to register flushed ID: <%u>: %s",
        data->tag,
        id,
        message);
    return false;
  }
  return added != 0;
}

static void
grn_obj_traverse_recursive_dependent_dispatch(
  grn_ctx *ctx, grn_obj_traverse_recursive_dependent_data *data, grn_obj *obj);

static void
grn_obj_traverse_recursive_dependent_db(
  grn_ctx *ctx, grn_obj_traverse_recursive_dependent_data *data, grn_obj *db)
{
  GRN_TABLE_EACH_BEGIN(ctx, db, cursor, id)
  {
    if (!grn_obj_traverse_recursive_dependent_need_traverse(ctx, data, id)) {
      if (ctx->rc == GRN_SUCCESS) {
        continue;
      } else {
        break;
      }
    }

    if (data->is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    grn_obj *object = grn_ctx_at(ctx, id);
    if (grn_obj_is_table(ctx, object)) {
      data->top_level_object = object;
      grn_obj_traverse_recursive_dependent_dispatch(ctx, data, object);
    } else {
      if (ctx->rc != GRN_SUCCESS) {
        ERRCLR(ctx);
      }
    }
    if (object) {
      grn_obj_unref(ctx, object);
    }

    if (data->is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }

    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  data->traverse(ctx, db, data->user_data);
}

static bool
grn_obj_traverse_recursive_dependent_table_is_target_column(
  grn_ctx *ctx,
  grn_obj_traverse_recursive_dependent_data *data,
  grn_obj *table,
  grn_obj *column)
{
  if (!data->for_reference) {
    return true;
  }

  if (table == data->top_level_object) {
    return true;
  }

  if (!grn_obj_is_index_column(ctx, column)) {
    return true;
  }

  if (grn_obj_is_table(ctx, data->top_level_object)) {
    return DB_OBJ(column)->range == DB_OBJ(data->top_level_object)->id;
  } else {
    if (DB_OBJ(column)->range != data->top_level_object->header.domain) {
      return false;
    }

    bool is_target = false;
    grn_obj source_ids;
    GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
    grn_obj_get_info(ctx, column, GRN_INFO_SOURCE, &source_ids);
    const size_t n = GRN_UINT32_VECTOR_SIZE(&source_ids);
    for (size_t i = 0; i < n; i++) {
      grn_id source_id = GRN_UINT32_VALUE_AT(&source_ids, i);
      if (source_id == DB_OBJ(data->top_level_object)->id) {
        is_target = true;
        break;
      }
    }
    GRN_OBJ_FIN(ctx, &source_ids);
    return is_target;
  }
}

static void
grn_obj_traverse_recursive_dependent_table(
  grn_ctx *ctx, grn_obj_traverse_recursive_dependent_data *data, grn_obj *table)
{
  if (grn_obj_is_table_with_key(ctx, table)) {
    grn_id domain_id = table->header.domain;
    if (grn_obj_traverse_recursive_dependent_need_traverse(ctx,
                                                           data,
                                                           domain_id)) {
      if (data->is_close_opened_object_mode) {
        grn_ctx_push_temporary_open_space(ctx);
      }
      grn_obj *domain = grn_ctx_at(ctx, domain_id);
      if (domain) {
        if (grn_obj_is_table(ctx, domain)) {
          grn_obj_traverse_recursive_dependent_dispatch(ctx, data, domain);
        }
        grn_obj_unref(ctx, domain);
      }
      if (data->is_close_opened_object_mode) {
        grn_ctx_pop_temporary_open_space(ctx);
      }
    }
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }

    for (grn_hook *hooks = DB_OBJ(table)->hooks[GRN_HOOK_INSERT]; hooks;
         hooks = hooks->next) {
      grn_obj_default_set_value_hook_data *hook_data =
        (grn_obj_default_set_value_hook_data *)GRN_NEXT_ADDR(hooks);
      if (grn_obj_traverse_recursive_dependent_need_traverse(
            ctx,
            data,
            hook_data->target)) {
        if (data->is_close_opened_object_mode) {
          grn_ctx_push_temporary_open_space(ctx);
        }
        grn_obj *index = grn_ctx_at(ctx, hook_data->target);
        if (index) {
          grn_obj_traverse_recursive_dependent_dispatch(ctx, data, index);
          grn_obj_unref(ctx, index);
        }
        if (data->is_close_opened_object_mode) {
          grn_ctx_pop_temporary_open_space(ctx);
        }
        if (ctx->rc != GRN_SUCCESS) {
          return;
        }
      }
    }
  }

  if (data->for_reference || table == data->top_level_object) {
    grn_hash *columns = grn_hash_create(ctx,
                                        NULL,
                                        sizeof(grn_id),
                                        0,
                                        GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
    if (!columns) {
      grn_rc rc = ctx->rc;
      if (rc == GRN_SUCCESS) {
        rc = GRN_NO_MEMORY_AVAILABLE;
      }
      char message[GRN_CTX_MSGSIZE];
      grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
      char table_name[GRN_TABLE_MAX_KEY_SIZE];
      int table_name_size;
      table_name_size =
        grn_obj_name(ctx, table, table_name, GRN_TABLE_MAX_KEY_SIZE);
      ERR(rc,
          "%s[recursive][dependent] "
          "failed to create internal hash table "
          "to store columns: <%.*s>: %s",
          data->tag,
          table_name_size,
          table_name,
          message);
      return;
    }

    if (grn_table_columns(ctx, table, "", 0, (grn_obj *)columns) > 0) {
      GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id)
      {
        void *key;
        if (grn_hash_cursor_get_key(ctx, cursor, &key) == 0) {
          continue;
        }

        grn_id *column_id = key;
        if (!grn_obj_traverse_recursive_dependent_need_traverse(ctx,
                                                                data,
                                                                *column_id)) {
          if (ctx->rc == GRN_SUCCESS) {
            continue;
          } else {
            break;
          }
        }

        if (data->is_close_opened_object_mode) {
          grn_ctx_push_temporary_open_space(ctx);
        }
        grn_obj *column = grn_ctx_at(ctx, *column_id);
        if (column) {
          if (grn_obj_traverse_recursive_dependent_table_is_target_column(
                ctx,
                data,
                table,
                column)) {
            grn_obj_traverse_recursive_dependent_dispatch(ctx, data, column);
          }
          grn_obj_unref(ctx, column);
        }
        if (data->is_close_opened_object_mode) {
          grn_ctx_pop_temporary_open_space(ctx);
        }

        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      }
      GRN_HASH_EACH_END(ctx, cursor);
    }
    grn_hash_close(ctx, columns);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  data->traverse(ctx, table, data->user_data);
}

static void
grn_obj_traverse_recursive_dependent_column_data(
  grn_ctx *ctx,
  grn_obj_traverse_recursive_dependent_data *data,
  grn_obj *column)
{
  if (column == data->top_level_object) {
    const grn_id table_id = column->header.domain;
    if (grn_obj_traverse_recursive_dependent_need_traverse(ctx,
                                                           data,
                                                           table_id)) {
      if (data->is_close_opened_object_mode) {
        grn_ctx_push_temporary_open_space(ctx);
      }
      grn_obj *table = grn_ctx_at(ctx, table_id);
      if (table) {
        grn_obj_traverse_recursive_dependent_dispatch(ctx, data, table);
        grn_obj_unref(ctx, table);
      }
      if (data->is_close_opened_object_mode) {
        grn_ctx_pop_temporary_open_space(ctx);
      }
    }
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  const grn_id range_id = grn_obj_get_range(ctx, column);
  if (grn_obj_traverse_recursive_dependent_need_traverse(ctx, data, range_id)) {
    if (data->is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }
    grn_obj *range = grn_ctx_at(ctx, range_id);
    if (range) {
      if (grn_obj_is_table(ctx, range)) {
        grn_obj_traverse_recursive_dependent_dispatch(ctx, data, range);
      }
      grn_obj_unref(ctx, range);
    }
    if (data->is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
  }
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  for (grn_hook *hooks = DB_OBJ(column)->hooks[GRN_HOOK_SET]; hooks;
       hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *hook_data =
      (grn_obj_default_set_value_hook_data *)GRN_NEXT_ADDR(hooks);
    if (grn_obj_traverse_recursive_dependent_need_traverse(ctx,
                                                           data,
                                                           hook_data->target)) {
      if (data->is_close_opened_object_mode) {
        grn_ctx_push_temporary_open_space(ctx);
      }
      grn_obj *index = grn_ctx_at(ctx, hook_data->target);
      if (index) {
        grn_obj_traverse_recursive_dependent_dispatch(ctx, data, index);
        grn_obj_unref(ctx, index);
      }
      if (data->is_close_opened_object_mode) {
        grn_ctx_pop_temporary_open_space(ctx);
      }
      if (ctx->rc != GRN_SUCCESS) {
        return;
      }
    }
  }

  data->traverse(ctx, column, data->user_data);
}

static void
grn_obj_traverse_recursive_dependent_column_index(
  grn_ctx *ctx,
  grn_obj_traverse_recursive_dependent_data *data,
  grn_obj *column)
{
  grn_id lexicon_id = column->header.domain;
  if (grn_obj_traverse_recursive_dependent_need_traverse(ctx,
                                                         data,
                                                         lexicon_id)) {
    if (data->is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }
    grn_obj *lexicon = grn_ctx_at(ctx, lexicon_id);
    if (lexicon) {
      grn_obj_traverse_recursive_dependent_dispatch(ctx, data, lexicon);
      grn_obj_unref(ctx, lexicon);
    }
    if (data->is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
  }
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  grn_id range_id = DB_OBJ(column)->range;
  if (grn_obj_traverse_recursive_dependent_need_traverse(ctx, data, range_id)) {
    if (data->is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }
    grn_obj *source_table = grn_ctx_at(ctx, range_id);
    if (source_table) {
      grn_obj_traverse_recursive_dependent_dispatch(ctx, data, source_table);
      grn_obj_unref(ctx, source_table);
    }
    if (data->is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
  }
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  grn_obj source_ids;
  GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
  grn_obj_get_info(ctx, column, GRN_INFO_SOURCE, &source_ids);
  const size_t n = GRN_UINT32_VECTOR_SIZE(&source_ids);
  for (size_t i = 0; i < n; i++) {
    grn_id source_id = GRN_UINT32_VALUE_AT(&source_ids, i);
    if (!grn_obj_traverse_recursive_dependent_need_traverse(ctx,
                                                            data,
                                                            source_id)) {
      if (ctx->rc == GRN_SUCCESS) {
        continue;
      } else {
        break;
      }
    }
    if (data->is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }
    grn_obj *source = grn_ctx_at(ctx, source_id);
    if (source) {
      grn_obj_traverse_recursive_dependent_dispatch(ctx, data, source);
      grn_obj_unref(ctx, source);
    }
    if (data->is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_OBJ_FIN(ctx, &source_ids);
  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  data->traverse(ctx, column, data->user_data);
}

static void
grn_obj_traverse_recursive_dependent_dispatch(
  grn_ctx *ctx, grn_obj_traverse_recursive_dependent_data *data, grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_DB:
    grn_obj_traverse_recursive_dependent_db(ctx, data, obj);
    break;
  case GRN_TABLE_NO_KEY:
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    grn_obj_traverse_recursive_dependent_table(ctx, data, obj);
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
    grn_obj_traverse_recursive_dependent_column_data(ctx, data, obj);
    break;
  case GRN_COLUMN_INDEX:
    grn_obj_traverse_recursive_dependent_column_index(ctx, data, obj);
    break;
  default:
    {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect_limited(ctx, &inspected, obj);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[recursive][dependent] "
          "object must be DB, table or column: %.*s",
          data->tag,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    break;
  }
}

static void
grn_obj_traverse_recursive_dependent(
  grn_ctx *ctx, grn_obj_traverse_recursive_dependent_data *data, grn_obj *obj)
{
  data->traversed = grn_hash_create(ctx,
                                    NULL,
                                    sizeof(grn_id),
                                    0,
                                    GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!data->traversed) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(rc,
        "%s[recursive][dependent] "
        "failed to create an internal hash table "
        "to manage traversed objects: <%.*s>: %s",
        data->tag,
        name_size,
        name,
        message);
    return;
  }

  data->is_close_opened_object_mode = (grn_thread_get_limit() == 1);

  grn_id id = grn_obj_id(ctx, obj);
  if (grn_obj_traverse_recursive_dependent_need_traverse(ctx, data, id)) {
    data->top_level_object = obj;
    grn_obj_traverse_recursive_dependent_dispatch(ctx, data, obj);
  }

  grn_hash_close(ctx, data->traversed);
}

static void
grn_table_close_columns(grn_ctx *ctx, grn_obj *table)
{
  grn_hash *columns;
  int n_columns;

  columns = grn_hash_create(ctx,
                            NULL,
                            sizeof(grn_id),
                            0,
                            GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    return;
  }

  n_columns = grn_table_columns(ctx, table, "", 0, (grn_obj *)columns);
  if (n_columns > 0) {
    grn_hash_cursor *cursor;
    cursor = grn_hash_cursor_open(ctx, columns, NULL, 0, NULL, 0, 0, -1, 0);
    if (cursor) {
      while (grn_hash_cursor_next(ctx, cursor) != GRN_ID_NIL) {
        grn_id *id;
        grn_obj *column;

        grn_hash_cursor_get_key(ctx, cursor, (void **)&id);
        column = grn_ctx_at(ctx, *id);
        if (column) {
          grn_obj_close(ctx, column);
        }
      }
      grn_hash_cursor_close(ctx, cursor);
    }
  }

  grn_hash_close(ctx, columns);
}

grn_rc
grn_obj_close(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_SUCCESS;
  }

  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    grn_id id = DB_OBJ(obj)->id;
    grn_obj *db = DB_OBJ(obj)->db;
    grn_hook_entry entry;

    grn_log_reference_count("%p: close: %u: %p\n", ctx, id, obj);

    if (id != GRN_ID_NIL && !(id & GRN_OBJ_TMP_OBJECT) && db) {
      grn_db_remove_deferred_unref(ctx, db, id);
      if (grn_reference_count_should_log(ctx, obj->header.type)) {
        const char *name;
        uint32_t name_size = 0;
        name = _grn_table_key(ctx, db, id, &name_size);
        GRN_LOG(ctx,
                GRN_LOG_REFERENCE_COUNT,
                "[obj][close] <%u>(<%.*s>):<%u>(<%s>)",
                id,
                name_size,
                name,
                obj->header.type,
                grn_obj_type_to_string(obj->header.type));
      }
    }

    if (grn_obj_is_table(ctx, obj) && (id & GRN_OBJ_TMP_OBJECT)) {
      grn_table_close_columns(ctx, obj);
    }
    if (DB_OBJ(obj)->finalizer) {
      DB_OBJ(obj)->finalizer(ctx, 1, &obj, &DB_OBJ(obj)->user_data);
    }
    if (DB_OBJ(obj)->source) {
      GRN_FREE(DB_OBJ(obj)->source);
    }
    for (entry = 0; entry < GRN_N_HOOK_ENTRIES; entry++) {
      grn_hook_free(ctx, DB_OBJ(obj)->hooks[entry]);
    }
    if (id & GRN_OBJ_TMP_OBJECT) {
      grn_obj_clear_option_values(ctx, obj);
      if (grn_obj_is_table(ctx, obj)) {
        grn_ctx_impl_columns_cache_delete(ctx, id);
      } else if (grn_obj_is_column(ctx, obj)) {
        grn_ctx_impl_columns_cache_delete(ctx, obj->header.domain);
      }
    }
    grn_obj_delete_by_id(ctx, DB_OBJ(obj)->db, id, GRN_FALSE);
  }
  switch (obj->header.type) {
  case GRN_VECTOR:
    if (obj->u.v.body && !(obj->header.impl_flags & GRN_OBJ_REFER)) {
      grn_obj_close(ctx, obj->u.v.body);
    }
    if (obj->u.v.sections) {
      GRN_FREE(obj->u.v.sections);
    }
    if (obj->header.impl_flags & GRN_OBJ_ALLOCATED) {
      GRN_FREE(obj);
    }
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_VOID:
  case GRN_BULK:
  case GRN_UVECTOR:
  case GRN_MSG:
    {
      obj->header.type = GRN_VOID;
      bool is_allocated = (obj->header.impl_flags & GRN_OBJ_ALLOCATED);
      grn_rc rc = grn_bulk_fin(ctx, obj);
      if (is_allocated) {
        GRN_FREE(obj);
      }
      GRN_API_RETURN(rc);
    }
  case GRN_PTR:
    {
      if (obj->header.impl_flags & GRN_OBJ_OWN) {
        if (GRN_BULK_VSIZE(obj) == sizeof(grn_obj *)) {
          grn_obj_close(ctx, GRN_PTR_VALUE(obj));
        }
      }
      obj->header.type = GRN_VOID;
      bool is_allocated = (obj->header.impl_flags & GRN_OBJ_ALLOCATED);
      grn_rc rc = grn_bulk_fin(ctx, obj);
      if (is_allocated) {
        GRN_FREE(obj);
      }
      GRN_API_RETURN(rc);
    }
  case GRN_PVECTOR:
    {
      grn_rc rc = grn_pvector_fin(ctx, obj);
      GRN_API_RETURN(rc);
    }
  case GRN_ACCESSOR:
    {
      grn_accessor *p, *n;
      for (p = (grn_accessor *)obj; p; p = n) {
        grn_obj_unref(ctx, p->obj);
        n = p->next;
        GRN_FREE(p);
      }
    }
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_SNIP:
    {
      grn_rc rc = grn_snip_close(ctx, (grn_snip *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_STRING:
    {
      grn_rc rc = grn_string_close(ctx, obj);
      GRN_API_RETURN(rc);
    }
  case GRN_HIGHLIGHTER:
    {
      grn_rc rc = grn_highlighter_close(ctx, (grn_highlighter *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_CURSOR_TABLE_PAT_KEY:
    grn_pat_cursor_close(ctx, (grn_pat_cursor *)obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_CURSOR_TABLE_DAT_KEY:
    grn_dat_cursor_close(ctx, (grn_dat_cursor *)obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_CURSOR_TABLE_HASH_KEY:
    grn_hash_cursor_close(ctx, (grn_hash_cursor *)obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_CURSOR_TABLE_NO_KEY:
    grn_array_cursor_close(ctx, (grn_array_cursor *)obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_CURSOR_COLUMN_INDEX:
    grn_index_cursor_close(ctx, obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_CURSOR_COLUMN_GEO_INDEX:
    grn_geo_cursor_close(ctx, obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_CURSOR_CONFIG:
    grn_config_cursor_close(ctx, (grn_config_cursor *)obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_TYPE:
    GRN_FREE(obj);
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_DB:
    {
      grn_rc rc = grn_db_close(ctx, obj);
      GRN_API_RETURN(rc);
    }
  case GRN_TABLE_PAT_KEY:
    {
      grn_rc rc = grn_pat_close(ctx, (grn_pat *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_TABLE_DAT_KEY:
    {
      grn_rc rc = grn_dat_close(ctx, (grn_dat *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_TABLE_HASH_KEY:
    {
      grn_rc rc = grn_hash_close(ctx, (grn_hash *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_TABLE_NO_KEY:
    {
      grn_rc rc = grn_array_close(ctx, (grn_array *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_COLUMN_VAR_SIZE:
    {
      grn_rc rc = grn_ja_close(ctx, (grn_ja *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_COLUMN_FIX_SIZE:
    {
      grn_rc rc = grn_ra_close(ctx, (grn_ra *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_COLUMN_INDEX:
    {
      grn_rc rc = grn_ii_close(ctx, (grn_ii *)obj);
      GRN_API_RETURN(rc);
    }
  case GRN_PROC:
    {
      uint32_t i;
      grn_proc *p = (grn_proc *)obj;
      /*
        if (obj->header.domain) {
          grn_hash_delete(ctx, ctx->impl->qe, &obj->header.domain,
          sizeof(grn_id), NULL);
        }
      */
      for (i = 0; i < p->nvars; i++) {
        grn_obj_close(ctx, &p->vars[i].value);
      }
      GRN_REALLOC(p->vars, 0);
      grn_obj_close(ctx, &p->name_buf);
      if (p->obj.range != GRN_ID_NIL) {
        grn_plugin_close(ctx, p->obj.range);
      }
      GRN_FREE(obj);
    }
    GRN_API_RETURN(GRN_SUCCESS);
  case GRN_EXPR:
    {
      grn_rc rc = grn_expr_close(ctx, obj);
      GRN_API_RETURN(rc);
    }
  default:
    GRN_API_RETURN(GRN_SUCCESS);
  }
}

grn_rc
grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_SUCCESS;
  }

  if (obj->header.type == GRN_DB) {
    return grn_obj_close(ctx, obj);
  }

  if (obj->header.type == GRN_ACCESSOR) {
    if (grn_enable_reference_count) {
      grn_accessor *accessor = (grn_accessor *)obj;
      GRN_API_ENTER;
      grn_log_reference_count("%p: unlink: start: accessor: %p: %u\n",
                              ctx,
                              obj,
                              accessor->reference_count);
      accessor->reference_count--;
      uint32_t current_reference_count = accessor->reference_count;
      grn_rc rc = GRN_SUCCESS;
      if (current_reference_count == 0) {
        rc = grn_obj_close(ctx, obj);
      }
      grn_log_reference_count("%p: unlink: done: accessor: %p: %u\n",
                              ctx,
                              obj,
                              current_reference_count);
      GRN_API_RETURN(rc);
    } else {
      return grn_obj_close(ctx, obj);
    }
  }

  if (!GRN_DB_OBJP(obj)) {
    return grn_obj_close(ctx, obj);
  }

  grn_db_obj *db_obj = DB_OBJ(obj);
  grn_id id = db_obj->id;

  if (id == GRN_ID_NIL || (id & GRN_OBJ_TMP_OBJECT)) {
    if (grn_enable_reference_count) {
      GRN_API_ENTER;
      grn_log_reference_count("%p: unlink: start: %u: %p: %u\n",
                              ctx,
                              id,
                              obj,
                              db_obj->reference_count);
      db_obj->reference_count--;
      uint32_t current_reference_count = db_obj->reference_count;
      grn_rc rc = GRN_SUCCESS;
      if (current_reference_count == 0) {
        rc = grn_obj_close(ctx, obj);
      }
      grn_log_reference_count("%p: unlink: done: %u: %p: %u\n",
                              ctx,
                              id,
                              obj,
                              current_reference_count);
      GRN_API_RETURN(rc);
    } else {
      return grn_obj_close(ctx, obj);
    }
  }

  /* Don't unlink built-in objects. We must update the same condition
   * in grn_ctx_at() when we need to unlink built-in objects. */
  if (id < GRN_N_RESERVED_TYPES) {
    return GRN_SUCCESS;
  }

  if (!grn_enable_reference_count) {
    return GRN_SUCCESS;
  }

  GRN_API_ENTER;
  grn_db *s = (grn_db *)(db_obj->db);
  db_value *vp = grn_tiny_array_at(&s->values, id);
  if (vp) {
    grn_log_reference_count("%p: unlink: start: %u: %p: %u: %p\n",
                            ctx,
                            id,
                            obj,
                            vp->lock,
                            vp->ptr);
    if (vp->lock == 0) {
      grn_log_reference_count("%p: unlink: done: not referenced: "
                              "%u: %p: %u: %p\n",
                              ctx,
                              id,
                              obj,
                              vp->lock,
                              vp->ptr);
      ERR(GRN_INVALID_ARGUMENT,
          "[obj][unlink] not referenced object: "
          "id:<%u> "
          "obj:<%p> "
          "lock:<%u> "
          "address:<%p>",
          id,
          obj,
          vp->lock,
          vp->ptr);
      GRN_API_RETURN(ctx->rc);
    }
    uint32_t current_lock;
    uint32_t *lock_pointer = &vp->lock;
    GRN_ATOMIC_ADD_EX(lock_pointer, -1, current_lock);
#ifdef GRN_REFERENCE_COUNT_DEBUG
    uint32_t current_reference_count = vp->lock;
#endif
    if (current_lock == 1) {
      grn_log_reference_count("%p: unlink: lock: %u: %p: %u: %u\n",
                              ctx,
                              id,
                              obj,
                              current_lock,
                              current_reference_count);
      GRN_ATOMIC_ADD_EX(lock_pointer, GRN_IO_MAX_REF, current_lock);
      uint32_t reset_max_ref_by_overflow = GRN_IO_MAX_REF - 1;
#ifdef GRN_REFERENCE_COUNT_DEBUG
      current_reference_count = vp->lock;
#endif
      grn_log_reference_count("%p: unlink: locked: %u: %p: %u: %u\n",
                              ctx,
                              id,
                              obj,
                              current_lock,
                              current_reference_count);
      if (current_lock == 0) {
        grn_rc rc = grn_obj_close(ctx, obj);
        grn_log_reference_count("%p: unlink: done: %u: %p: %u\n",
                                ctx,
                                id,
                                obj,
                                current_reference_count);
        GRN_API_RETURN(rc);
      } else {
        grn_log_reference_count("%p: unlink: unlock: %u: %p: %u: %u\n",
                                ctx,
                                id,
                                obj,
                                current_lock,
                                current_reference_count);
        GRN_ATOMIC_ADD_EX(lock_pointer,
                          reset_max_ref_by_overflow,
                          current_lock);
#ifdef GRN_REFERENCE_COUNT_DEBUG
        current_reference_count = vp->lock;
#endif
        grn_log_reference_count("%p: unlink: unlocked: %u: %p: %u: %u\n",
                                ctx,
                                id,
                                obj,
                                current_lock,
                                current_reference_count);
      }
      GRN_FUTEX_WAKE(lock_pointer);
    }
    grn_log_reference_count("%p: unlink: done: %u: %p: %u\n",
                            ctx,
                            id,
                            obj,
                            current_reference_count);
    GRN_API_RETURN(GRN_SUCCESS);
  }
  GRN_API_RETURN(GRN_SUCCESS);
}

void
grn_obj_unref(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_enable_reference_count) {
    return;
  }

  grn_obj_unlink(ctx, obj);
}

static void
grn_obj_unref_traverse(grn_ctx *ctx, grn_obj *obj, void *user_data)
{
  if (obj->header.type == GRN_DB) {
    return;
  }
  grn_obj_unref(ctx, obj);
}

void
grn_obj_unref_recursive(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_enable_reference_count) {
    return;
  }

  GRN_API_ENTER;

  grn_obj_traverse_recursive_data data;
  data.tag = "[obj][unref]";
  data.traverse = grn_obj_unref_traverse;
  data.user_data = NULL;
  grn_obj_traverse_recursive(ctx, &data, obj);

  GRN_API_RETURN();
}

void
grn_obj_unref_recursive_dependent(grn_ctx *ctx, grn_obj *obj)
{
  if (!grn_enable_reference_count) {
    return;
  }

  GRN_API_ENTER;

  grn_obj_traverse_recursive_dependent_data data;
  data.tag = "[obj][unref]";
  data.traverse = grn_obj_unref_traverse;
  data.user_data = NULL;
  data.for_reference = true;
  grn_obj_traverse_recursive_dependent(ctx, &data, obj);

  GRN_API_RETURN();
}

static grn_rc
grn_obj_refer_record(grn_ctx *ctx, grn_obj *obj, grn_obj *ids)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }

  if (!obj) {
    return ctx->rc;
  }

  if (grn_obj_is_accessor(ctx, obj)) {
    grn_accessor_refer(ctx, obj);
    return ctx->rc;
  }

  if (GRN_DB_OBJP(obj)) {
    grn_id id = grn_obj_id(ctx, obj);
    if (grn_ctx_at(ctx, id) && ids) {
      GRN_RECORD_PUT(ctx, ids, id);
    }
    return ctx->rc;
  }

  return ctx->rc;
}

grn_rc
grn_obj_refer_auto_release(grn_ctx *ctx, grn_obj *obj, uint32_t count)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }

  if (count > 0) {
    grn_deferred_unref deferred_unref;
    deferred_unref.count = count;
    GRN_RECORD_INIT(&(deferred_unref.ids), GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_obj_refer_record(ctx, obj, &(deferred_unref.ids));
    if (ctx->rc == GRN_SUCCESS) {
      grn_db_add_deferred_unref(ctx, grn_ctx_db(ctx), &deferred_unref);
    }
    GRN_OBJ_FIN(ctx, &(deferred_unref.ids));
    return ctx->rc;
  } else {
    return grn_obj_refer_record(ctx, obj, NULL);
  }
}

grn_rc
grn_obj_refer(grn_ctx *ctx, grn_obj *obj)
{
  return grn_obj_refer_auto_release(ctx, obj, 0);
}

static void
grn_obj_refer_traverse(grn_ctx *ctx, grn_obj *obj, void *user_data)
{
  if (obj->header.type == GRN_DB) {
    return;
  }
  grn_deferred_unref *deferred_unref = user_data;
  if (deferred_unref->count == 0) {
    grn_obj_refer(ctx, obj);
  } else {
    grn_obj_refer_record(ctx, obj, &(deferred_unref->ids));
  }
}

grn_rc
grn_obj_refer_recursive_auto_release(grn_ctx *ctx, grn_obj *obj, uint32_t count)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }

  GRN_API_ENTER;

  grn_deferred_unref deferred_unref;
  deferred_unref.count = count;
  GRN_RECORD_INIT(&(deferred_unref.ids), GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_obj_traverse_recursive_data data;
  data.tag = "[obj][refer]";
  data.traverse = grn_obj_refer_traverse;
  data.user_data = &deferred_unref;
  grn_obj_traverse_recursive(ctx, &data, obj);
  if (ctx->rc == GRN_SUCCESS && deferred_unref.count > 0) {
    grn_db_add_deferred_unref(ctx, grn_ctx_db(ctx), &deferred_unref);
  }
  GRN_OBJ_FIN(ctx, &(deferred_unref.ids));

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_refer_recursive(grn_ctx *ctx, grn_obj *obj)
{
  return grn_obj_refer_recursive_auto_release(ctx, obj, 0);
}

grn_rc
grn_obj_refer_recursive_dependent_auto_release(grn_ctx *ctx,
                                               grn_obj *obj,
                                               uint32_t count)
{
  if (!grn_enable_reference_count) {
    return ctx->rc;
  }

  GRN_API_ENTER;

  grn_deferred_unref deferred_unref;
  deferred_unref.count = count;
  GRN_RECORD_INIT(&(deferred_unref.ids), GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_obj_traverse_recursive_dependent_data data;
  data.tag = "[obj][refer]";
  data.traverse = grn_obj_refer_traverse;
  data.user_data = &deferred_unref;
  data.for_reference = true;
  grn_obj_traverse_recursive_dependent(ctx, &data, obj);
  if (ctx->rc == GRN_SUCCESS && deferred_unref.count > 0) {
    grn_db_add_deferred_unref(ctx, grn_ctx_db(ctx), &deferred_unref);
  }
  GRN_OBJ_FIN(ctx, &(deferred_unref.ids));

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_refer_recursive_dependent(grn_ctx *ctx, grn_obj *obj)
{
  return grn_obj_refer_recursive_dependent_auto_release(ctx, obj, 0);
}

/* Internal function for debug */
uint32_t
grn_obj_reference_count(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return 0;
  }

  if (obj->header.type == GRN_DB) {
    return 0;
  }

  if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *accessor = (grn_accessor *)obj;
    return accessor->reference_count;
  }

  if (!GRN_DB_OBJP(obj)) {
    return 0;
  }

  grn_db_obj *db_obj = DB_OBJ(obj);
  grn_id id = db_obj->id;

  if (id == GRN_ID_NIL || (id & GRN_OBJ_TMP_OBJECT)) {
    return db_obj->reference_count;
  }

  grn_db *s = (grn_db *)(db_obj->db);
  db_value *vp = grn_tiny_array_at(&s->values, id);
  if (vp) {
    return vp->lock;
  } else {
    return 0;
  }
}

grn_inline static void
grn_vector_clear(grn_ctx *ctx, grn_obj *obj)
{
  if (obj->u.v.body && !(obj->header.impl_flags & GRN_OBJ_REFER)) {
    grn_obj_close(ctx, obj->u.v.body);
  }
  if (obj->u.v.sections) {
    GRN_FREE(obj->u.v.sections);
  }
  obj->header.impl_flags &= ~GRN_OBJ_DO_SHALLOW_COPY;
  memset(&(obj->u), 0, sizeof(obj->u));
}

void
grn_obj_ensure_vector(grn_ctx *ctx, grn_obj *obj)
{
  if (obj->header.type != GRN_VECTOR) {
    grn_bulk_fin(ctx, obj);
  }
  obj->header.type = GRN_VECTOR;
  obj->header.flags &= ~GRN_OBJ_WITH_WEIGHT;
}

void
grn_obj_ensure_bulk(grn_ctx *ctx, grn_obj *obj)
{
  if (obj->header.type == GRN_VECTOR) {
    grn_vector_clear(ctx, obj);
  }
  obj->header.type = GRN_BULK;
  obj->header.flags &= ~GRN_OBJ_WITH_WEIGHT;
}

grn_rc
grn_obj_reinit(grn_ctx *ctx, grn_obj *obj, grn_id domain, uint8_t flags)
{
  if (!GRN_OBJ_MUTABLE(obj)) {
    ERR(GRN_INVALID_ARGUMENT, "invalid obj assigned");
  } else {
    switch (obj->header.type) {
    case GRN_PTR:
      if (obj->header.impl_flags & GRN_OBJ_OWN) {
        if (GRN_BULK_VSIZE(obj) == sizeof(grn_obj *)) {
          grn_obj_close(ctx, GRN_PTR_VALUE(obj));
        }
        obj->header.impl_flags &= ~GRN_OBJ_OWN;
      }
      break;
    case GRN_PVECTOR:
      if (obj->header.impl_flags & GRN_OBJ_OWN) {
        unsigned int i, n_elements;
        n_elements = GRN_BULK_VSIZE(obj) / sizeof(grn_obj *);
        for (i = 0; i < n_elements; i++) {
          grn_obj *element = GRN_PTR_VALUE_AT(obj, i);
          grn_obj_close(ctx, element);
        }
        obj->header.impl_flags &= ~GRN_OBJ_OWN;
      }
      break;
    default:
      break;
    }

    switch (domain) {
    case GRN_DB_VOID:
      obj->header.domain = domain;
      if (flags & GRN_OBJ_VECTOR) {
        if (obj->header.type == GRN_VECTOR) {
          grn_vector_clear(ctx, obj);
        } else {
          grn_bulk_fin(ctx, obj);
          obj->header.type = GRN_VECTOR;
          if (obj->u.v.body) {
            if (obj->header.impl_flags & GRN_OBJ_DO_SHALLOW_COPY) {
              obj->u.v.body = NULL;
              obj->header.impl_flags &= ~GRN_OBJ_DO_SHALLOW_COPY;
            } else {
              grn_obj_reinit(ctx, obj->u.v.body, domain, 0);
            }
          }
          obj->u.v.n_sections = 0;
        }
      } else {
        if (obj->header.type == GRN_VECTOR) {
          grn_vector_clear(ctx, obj);
        }
        obj->header.type = GRN_VOID;
      }
      GRN_BULK_REWIND(obj);
      break;
    case GRN_DB_OBJECT:
    case GRN_DB_BOOL:
    case GRN_DB_INT8:
    case GRN_DB_UINT8:
    case GRN_DB_INT16:
    case GRN_DB_UINT16:
    case GRN_DB_INT32:
    case GRN_DB_UINT32:
    case GRN_DB_INT64:
    case GRN_DB_UINT64:
    case GRN_DB_BFLOAT16:
    case GRN_DB_FLOAT32:
    case GRN_DB_FLOAT:
    case GRN_DB_TIME:
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT:
      if (obj->header.type == GRN_VECTOR) {
        grn_vector_clear(ctx, obj);
      }
      obj->header.type = (flags & GRN_OBJ_VECTOR) ? GRN_UVECTOR : GRN_BULK;
      obj->header.domain = domain;
      GRN_BULK_REWIND(obj);
      break;
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT:
      if (flags & GRN_OBJ_VECTOR) {
        if (obj->header.type != GRN_VECTOR) {
          grn_bulk_fin(ctx, obj);
        }
        obj->header.type = GRN_VECTOR;
        if (obj->u.v.body) {
          if (obj->header.impl_flags & GRN_OBJ_DO_SHALLOW_COPY) {
            obj->u.v.body = NULL;
            obj->header.impl_flags &= ~GRN_OBJ_DO_SHALLOW_COPY;
          } else {
            grn_obj_reinit(ctx, obj->u.v.body, domain, 0);
          }
        }
        obj->u.v.n_sections = 0;
      } else {
        if (obj->header.type == GRN_VECTOR) {
          grn_vector_clear(ctx, obj);
        }
        obj->header.type = GRN_BULK;
      }
      obj->header.domain = domain;
      GRN_BULK_REWIND(obj);
      break;
    default:
      if (grn_type_id_is_text_family(ctx, domain)) {
        if (flags & GRN_OBJ_VECTOR) {
          if (obj->header.type != GRN_VECTOR) {
            grn_bulk_fin(ctx, obj);
          }
          obj->header.type = GRN_VECTOR;
        } else {
          if (obj->header.type == GRN_VECTOR) {
            grn_vector_clear(ctx, obj);
          }
          obj->header.type = GRN_BULK;
        }
      } else {
        if (obj->header.type == GRN_VECTOR) {
          grn_vector_clear(ctx, obj);
        }
        obj->header.type = (flags & GRN_OBJ_VECTOR) ? GRN_UVECTOR : GRN_BULK;
      }
      obj->header.domain = domain;
      GRN_BULK_REWIND(obj);
      break;
    }
  }
  return ctx->rc;
}

grn_rc
grn_obj_reinit_for(grn_ctx *ctx, grn_obj *obj, grn_obj *domain_obj)
{
  grn_id domain = GRN_ID_NIL;
  grn_obj_flags flags = 0;

  if (!GRN_DB_OBJP(domain_obj) && domain_obj->header.type != GRN_ACCESSOR) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, domain_obj);
    ERR(GRN_INVALID_ARGUMENT,
        "[reinit] invalid domain object: <%.*s>",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return ctx->rc;
  }

  if (grn_column_is_index(ctx, domain_obj)) {
    domain = GRN_DB_UINT32;
  } else {
    grn_obj_get_range_info(ctx, domain_obj, &domain, &flags);
    if (GRN_OBJ_TABLEP(domain_obj) &&
        domain_obj->header.type != GRN_TABLE_NO_KEY) {
      domain = domain_obj->header.domain;
    }
  }
  uint8_t reinit_flags =
    (uint8_t)(flags & (GRN_OBJ_VECTOR | GRN_OBJ_DO_SHALLOW_COPY));
  return grn_obj_reinit(ctx, obj, domain, reinit_flags);
}

const char *
grn_obj_path(grn_ctx *ctx, grn_obj *obj)
{
  grn_io *io;
  const char *path = NULL;
  GRN_API_ENTER;
  if (obj->header.type == GRN_PROC) {
    path = grn_plugin_path(ctx, DB_OBJ(obj)->range);
    GRN_API_RETURN(path);
  }
  io = grn_obj_get_io(ctx, obj);
  if (io && !(io->flags & GRN_IO_TEMPORARY)) {
    path = io->path;
  }
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
      grn_id id = DB_OBJ(obj)->id;
      if (id & GRN_OBJ_TMP_OBJECT) {
        if (id & GRN_OBJ_TMP_COLUMN) {
          grn_id real_id = id & ~(GRN_OBJ_TMP_OBJECT | GRN_OBJ_TMP_COLUMN);
          grn_ctx *target_ctx = ctx;
          while (target_ctx->impl->parent) {
            target_ctx = target_ctx->impl->parent;
          }
          len = grn_pat_get_key(target_ctx,
                                target_ctx->impl->temporary_columns,
                                real_id,
                                namebuf,
                                buf_size);
        }
      } else {
        len = grn_table_get_key(ctx, s->keys, id, namebuf, buf_size);
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
  if (!obj) {
    return len;
  }
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    grn_id id = DB_OBJ(obj)->id;
    if (id & GRN_OBJ_TMP_OBJECT) {
      if (id & GRN_OBJ_TMP_COLUMN) {
        grn_id real_id = id & ~(GRN_OBJ_TMP_OBJECT | GRN_OBJ_TMP_COLUMN);
        grn_ctx *target_ctx = ctx;
        while (target_ctx->impl->parent) {
          target_ctx = target_ctx->impl->parent;
        }
        len = grn_pat_get_key(target_ctx,
                              target_ctx->impl->temporary_columns,
                              real_id,
                              buf,
                              GRN_TABLE_MAX_KEY_SIZE);
      }
    } else if (id && id < GRN_ID_MAX) {
      grn_db *s = (grn_db *)DB_OBJ(obj)->db;
      len = grn_table_get_key(ctx, s->keys, id, buf, GRN_TABLE_MAX_KEY_SIZE);
    }
    if (len) {
      int cl;
      char *p = buf, *p0 = p, *pe = p + len;
      for (; p < pe && (cl = grn_charlen(ctx, p, pe)); p += cl) {
        if (*p == GRN_DB_DELIMITER && cl == 1) {
          p0 = p + cl;
        }
      }
      len = pe - p0;
      if (len && len <= buf_size) {
        grn_memcpy(namebuf, p0, len);
      }
    }
  } else if (obj->header.type == GRN_ACCESSOR) {
    grn_obj name;
    grn_accessor *a;

    GRN_TEXT_INIT(&name, 0);

#define ADD_DELMITER()                                                         \
  do {                                                                         \
    if (GRN_TEXT_LEN(&name) > 0) {                                             \
      GRN_TEXT_PUTC(ctx, &name, GRN_DB_DELIMITER);                             \
    }                                                                          \
  } while (GRN_FALSE)

    for (a = (grn_accessor *)obj; a; a = a->next) {
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_ID);
        break;
      case GRN_ACCESSOR_GET_KEY:
        if (!a->next) {
          ADD_DELMITER();
          GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_KEY);
        }
        break;
      case GRN_ACCESSOR_GET_VALUE:
        if (!a->next) {
          ADD_DELMITER();
          GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_VALUE);
        }
        break;
      case GRN_ACCESSOR_GET_SCORE:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_SCORE);
        break;
      case GRN_ACCESSOR_GET_NSUBRECS:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_NSUBRECS);
        break;
      case GRN_ACCESSOR_GET_MAX:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_MAX);
        break;
      case GRN_ACCESSOR_GET_MIN:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_MIN);
        break;
      case GRN_ACCESSOR_GET_SUM:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_SUM);
        break;
      case GRN_ACCESSOR_GET_AVG:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_AVG);
        break;
      case GRN_ACCESSOR_GET_MEAN:
        ADD_DELMITER();
        GRN_TEXT_PUTS(ctx, &name, GRN_COLUMN_NAME_MEAN);
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE:
        ADD_DELMITER();
        {
          char column_name[GRN_TABLE_MAX_KEY_SIZE];
          int column_name_size;
          column_name_size =
            grn_column_name(ctx, a->obj, column_name, GRN_TABLE_MAX_KEY_SIZE);
          GRN_TEXT_PUT(ctx, &name, column_name, column_name_size);
        }
        break;
      case GRN_ACCESSOR_GET_DB_OBJ:
      case GRN_ACCESSOR_LOOKUP:
      case GRN_ACCESSOR_FUNCALL:
        break;
      }
    }
#undef ADD_DELIMITER

    len = GRN_TEXT_LEN(&name);
    if (len > 0 && len <= buf_size) {
      grn_memcpy(namebuf, GRN_TEXT_VALUE(&name), len);
    }

    GRN_OBJ_FIN(ctx, &name);
  }
  GRN_API_RETURN(len);
}

grn_rc
grn_column_name_(grn_ctx *ctx, grn_obj *obj, grn_obj *buf)
{
  if (GRN_DB_OBJP(obj)) {
    uint32_t len = 0;
    const char *p = NULL;
    grn_id id = DB_OBJ(obj)->id;
    if (id & GRN_OBJ_TMP_OBJECT) {
      if (id & GRN_OBJ_TMP_COLUMN) {
        grn_id real_id = id & ~(GRN_OBJ_TMP_OBJECT | GRN_OBJ_TMP_COLUMN);
        grn_ctx *target_ctx = ctx;
        while (target_ctx->impl->parent) {
          target_ctx = target_ctx->impl->parent;
        }
        p = _grn_pat_key(target_ctx,
                         target_ctx->impl->temporary_columns,
                         real_id,
                         &len);
      }
    } else if (id && id < GRN_ID_MAX) {
      grn_db *s = (grn_db *)DB_OBJ(obj)->db;
      p = _grn_table_key(ctx, s->keys, id, &len);
    }
    if (len) {
      int cl;
      const char *p0 = p, *pe = p + len;
      for (; p < pe && (cl = grn_charlen(ctx, p, pe)); p += cl) {
        if (*p == GRN_DB_DELIMITER && cl == 1) {
          p0 = p + cl;
        }
      }
      GRN_TEXT_PUT(ctx, buf, p0, pe - p0);
    }
  } else if (obj->header.type == GRN_ACCESSOR) {
    grn_accessor *a;
    for (a = (grn_accessor *)obj; a; a = a->next) {
      switch (a->action) {
      case GRN_ACCESSOR_GET_ID:
        GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_ID, GRN_COLUMN_NAME_ID_LEN);
        break;
      case GRN_ACCESSOR_GET_KEY:
        if (!a->next) {
          GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN);
        }
        break;
      case GRN_ACCESSOR_GET_VALUE:
        if (!a->next) {
          GRN_TEXT_PUT(ctx,
                       buf,
                       GRN_COLUMN_NAME_VALUE,
                       GRN_COLUMN_NAME_VALUE_LEN);
        }
        break;
      case GRN_ACCESSOR_GET_SCORE:
        GRN_TEXT_PUT(ctx,
                     buf,
                     GRN_COLUMN_NAME_SCORE,
                     GRN_COLUMN_NAME_SCORE_LEN);
        break;
      case GRN_ACCESSOR_GET_NSUBRECS:
        GRN_TEXT_PUT(ctx,
                     buf,
                     GRN_COLUMN_NAME_NSUBRECS,
                     GRN_COLUMN_NAME_NSUBRECS_LEN);
        break;
      case GRN_ACCESSOR_GET_MAX:
        GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_MAX, GRN_COLUMN_NAME_MAX_LEN);
        break;
      case GRN_ACCESSOR_GET_MIN:
        GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_MIN, GRN_COLUMN_NAME_MIN_LEN);
        break;
      case GRN_ACCESSOR_GET_SUM:
        GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_SUM, GRN_COLUMN_NAME_SUM_LEN);
        break;
      case GRN_ACCESSOR_GET_AVG:
        GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_AVG, GRN_COLUMN_NAME_AVG_LEN);
        break;
      case GRN_ACCESSOR_GET_MEAN:
        GRN_TEXT_PUT(ctx, buf, GRN_COLUMN_NAME_MEAN, GRN_COLUMN_NAME_MEAN_LEN);
        break;
      case GRN_ACCESSOR_GET_COLUMN_VALUE:
        grn_column_name_(ctx, a->obj, buf);
        if (a->next) {
          GRN_TEXT_PUTC(ctx, buf, '.');
        }
        break;
      case GRN_ACCESSOR_GET_DB_OBJ:
      case GRN_ACCESSOR_LOOKUP:
      case GRN_ACCESSOR_FUNCALL:
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
  rc = grn_io_lock(ctx, grn_obj_get_io(ctx, obj), timeout);
  if (rc == GRN_SUCCESS && obj && obj->header.type == GRN_COLUMN_INDEX) {
    rc = grn_io_lock(ctx, ((grn_ii *)obj)->chunk, timeout);
  }
  GRN_API_RETURN(rc);
}

grn_rc
grn_obj_unlock(grn_ctx *ctx, grn_obj *obj, grn_id id)
{
  GRN_API_ENTER;
  if (obj && obj->header.type == GRN_COLUMN_INDEX) {
    grn_io_unlock(ctx, ((grn_ii *)obj)->chunk);
  }
  grn_io_unlock(ctx, grn_obj_get_io(ctx, obj));
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_user_data *
grn_obj_user_data(grn_ctx *ctx, grn_obj *obj)
{
  if (!GRN_DB_OBJP(obj)) {
    return NULL;
  }
  return &DB_OBJ(obj)->user_data;
}

grn_rc
grn_obj_set_finalizer(grn_ctx *ctx, grn_obj *obj, grn_proc_func *func)
{
  if (!GRN_DB_OBJP(obj)) {
    return GRN_INVALID_ARGUMENT;
  }
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
        while ((id = grn_table_cursor_next_inline(ctx, cur)) != GRN_ID_NIL) {
          grn_obj *tbl = grn_ctx_at(ctx, id);
          if (tbl) {
            switch (tbl->header.type) {
            case GRN_TABLE_HASH_KEY:
            case GRN_TABLE_PAT_KEY:
            case GRN_TABLE_DAT_KEY:
            case GRN_TABLE_NO_KEY:
              grn_obj_clear_lock(ctx, tbl);
              break;
            }
          } else {
            if (ctx->rc != GRN_SUCCESS) {
              ERRCLR(ctx);
            }
          }
        }
        grn_table_cursor_close(ctx, cur);
      }
    }
    grn_io_clear_lock(grn_obj_get_io(ctx, obj));
    {
      grn_db *db = (grn_db *)obj;
      if (db->specs) {
        grn_obj_clear_lock(ctx, (grn_obj *)(db->specs));
      }
      grn_obj_clear_lock(ctx, (grn_obj *)(db->config));
      grn_options_clear_lock(ctx, db->options);
    }
    break;
  case GRN_TABLE_NO_KEY:
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
    {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx,
                                  NULL,
                                  sizeof(grn_id),
                                  0,
                                  GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, obj, "", 0, (grn_obj *)cols)) {
          grn_id *key;
          GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col = grn_ctx_at(ctx, *key);
            if (col) {
              grn_obj_clear_lock(ctx, col);
            }
          });
        }
        grn_hash_close(ctx, cols);
      }
      grn_io_clear_lock(grn_obj_get_io(ctx, obj));
    }
    break;
  case GRN_COLUMN_FIX_SIZE:
  case GRN_COLUMN_VAR_SIZE:
    grn_io_clear_lock(grn_obj_get_io(ctx, obj));
    break;
  case GRN_COLUMN_INDEX:
    grn_io_clear_lock(grn_obj_get_io(ctx, obj));
    if (obj) {
      grn_io_clear_lock(((grn_ii *)obj)->chunk);
    }
    break;
  }
  GRN_API_RETURN(GRN_SUCCESS);
}

unsigned int
grn_obj_is_locked(grn_ctx *ctx, grn_obj *obj)
{
  unsigned int res = 0;
  GRN_API_ENTER;
  res = grn_io_is_locked(grn_obj_get_io(ctx, obj));
  if (obj) {
    switch (obj->header.type) {
    case GRN_DB:
      {
        grn_db *db = (grn_db *)obj;
        if (db->specs) {
          res += grn_obj_is_locked(ctx, (grn_obj *)(db->specs));
        }
        res += grn_obj_is_locked(ctx, (grn_obj *)db->config);
        if (grn_options_is_locked(ctx, db->options)) {
          res++;
        }
      }
      break;
    case GRN_COLUMN_INDEX:
      res += grn_io_is_locked(((grn_ii *)obj)->chunk);
      break;
    }
  }
  GRN_API_RETURN(res);
}

static grn_rc
grn_obj_flush_raw(grn_ctx *ctx, grn_obj *obj)
{
  grn_rc rc = GRN_OPERATION_NOT_SUPPORTED;
  grn_io *io = grn_obj_get_io(ctx, obj);
  if (io) {
    rc = grn_io_flush(ctx, io);
  }
  return rc;
}

static grn_rc
grn_obj_flush_without_lock(grn_ctx *ctx, grn_obj *obj, const char *tag)
{
  grn_rc rc = GRN_SUCCESS;

  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_size = 0;
  bool flushed = false;
  switch (obj->header.type) {
  case GRN_DB:
    {
      grn_db *db = (grn_db *)obj;
      rc = grn_obj_flush_without_lock(ctx, db->keys, tag);
      if (rc == GRN_SUCCESS && db->specs) {
        rc = grn_obj_flush_without_lock(ctx, (grn_obj *)(db->specs), tag);
      }
      if (rc == GRN_SUCCESS) {
        rc = grn_obj_flush_without_lock(ctx, (grn_obj *)(db->config), tag);
      }
      if (rc == GRN_SUCCESS) {
        rc = grn_options_flush(ctx, db->options);
      }
    }
    grn_strcpy(name, GRN_TABLE_MAX_KEY_SIZE, "(DB)");
    name_size = strlen(name);
    flushed = true;
    break;
  case GRN_TABLE_DAT_KEY:
    rc = grn_dat_flush(ctx, (grn_dat *)obj);
    flushed = true;
    break;
  case GRN_COLUMN_INDEX:
    rc = grn_ii_flush(ctx, (grn_ii *)obj);
    flushed = true;
    break;
  default:
    {
      grn_rc rc_raw = grn_obj_flush_raw(ctx, obj);
      if (rc_raw != GRN_OPERATION_NOT_SUPPORTED) {
        if (rc_raw != GRN_SUCCESS) {
          rc = rc_raw;
        }
        flushed = true;
      }
    }
    break;
  }

  if (flushed && rc == GRN_SUCCESS) {
    grn_wal_clear(ctx, obj, false, tag);
  }

  if (flushed && name_size == 0) {
    name_size = grn_obj_name(ctx, obj, name, GRN_TABLE_MAX_KEY_SIZE);
    if (name_size == 0) {
      grn_strcpy(name, GRN_TABLE_MAX_KEY_SIZE, "(anonymous:");
      grn_strcat(name,
                 GRN_TABLE_MAX_KEY_SIZE,
                 grn_obj_type_to_string(obj->header.type));
      grn_strcat(name, GRN_TABLE_MAX_KEY_SIZE, ")");
      name_size = strlen(name);
    }
  }

  if (name_size > 0) {
    GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE, ":", "flush[%.*s]", name_size, name);
  }

  if (rc == GRN_SUCCESS && GRN_DB_OBJP(obj) && DB_OBJ(obj)->id != GRN_ID_NIL &&
      !IS_TEMP(obj)) {
    rc = grn_db_clean(ctx, DB_OBJ(obj)->db);
  }

  return rc;
}

static grn_rc
grn_obj_flush_lock(grn_ctx *ctx, grn_obj *obj, const char *tag)
{
  if (!grn_obj_get_io(ctx, obj)) {
    return GRN_SUCCESS;
  }

  grn_rc rc = grn_obj_lock(ctx, obj, GRN_ID_NIL, grn_lock_timeout);
  if (rc != GRN_SUCCESS) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, obj);
    ERR(rc,
        "%s failed to lock%s%s: <%.*s>",
        tag,
        errbuf[0] == '\0' ? "" : ": ",
        errbuf,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
  }
  return rc;
}

static grn_rc
grn_obj_flush_unlock(grn_ctx *ctx, grn_obj *obj, const char *tag)
{
  if (!grn_obj_get_io(ctx, obj)) {
    return GRN_SUCCESS;
  }

  grn_rc rc = grn_obj_unlock(ctx, obj, GRN_ID_NIL);
  if (rc != GRN_SUCCESS) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, obj);
    ERR(rc,
        "%s failed to unlock%s%s: <%.*s>",
        tag,
        errbuf[0] == '\0' ? "" : ": ",
        errbuf,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return rc;
  }

  /* For flushing io->lock */
  return grn_obj_flush_raw(ctx, obj);
}

static grn_rc
grn_obj_flush_internal(grn_ctx *ctx, grn_obj *obj, const char *tag)
{
  bool need_lock = ((GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_PRIMARY) &&
                    grn_wal_exist(ctx, obj));
  grn_rc rc = GRN_SUCCESS;
  if (need_lock) {
    rc = grn_obj_flush_lock(ctx, obj, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  rc = grn_obj_flush_without_lock(ctx, obj, tag);
  if (need_lock) {
    grn_rc rc_unlock = grn_obj_flush_unlock(ctx, obj, tag);
    if (rc == GRN_SUCCESS && rc_unlock != GRN_SUCCESS) {
      rc = rc_unlock;
    }
  }

  return rc;
}

grn_rc
grn_obj_flush(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;
  grn_rc rc = grn_obj_flush_internal(ctx, obj, "[obj][flush]");
  GRN_API_RETURN(rc);
}

typedef struct {
  const char *tag;
  grn_obj *top_obj;
} grn_obj_flush_traverse_data;

static void
grn_obj_flush_traverse(grn_ctx *ctx, grn_obj *obj, void *user_data)
{
  grn_obj_flush_traverse_data *data = user_data;
  if (obj == data->top_obj) {
    grn_obj_flush_without_lock(ctx, obj, data->tag);
  } else {
    grn_obj_flush_internal(ctx, obj, data->tag);
  }
}

grn_rc
grn_obj_flush_recursive(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;

  grn_obj_flush_traverse_data traverse_data;
  traverse_data.tag = "[obj][flush][recursive]";
  traverse_data.top_obj = obj;
  grn_obj_traverse_recursive_data data;
  data.tag = "[obj][flush]";
  data.traverse = grn_obj_flush_traverse;
  data.user_data = &traverse_data;

  grn_obj *db = grn_ctx_db(ctx);
  grn_rc rc = grn_obj_flush_lock(ctx, db, traverse_data.tag);
  if (rc != GRN_SUCCESS) {
    GRN_API_RETURN(rc);
  }
  if (obj != db) {
    rc = grn_obj_flush_lock(ctx, obj, traverse_data.tag);
    if (rc != GRN_SUCCESS) {
      grn_obj_flush_unlock(ctx, db, traverse_data.tag);
      GRN_API_RETURN(rc);
    }
  }
  grn_obj_traverse_recursive(ctx, &data, obj);
  if (obj != db) {
    grn_obj_flush_unlock(ctx, obj, traverse_data.tag);
  }
  grn_obj_flush_unlock(ctx, db, traverse_data.tag);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_flush_recursive_dependent(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;

  grn_obj_flush_traverse_data traverse_data;
  traverse_data.tag = "[obj][flush][recursive][dependent]";
  traverse_data.top_obj = obj;
  grn_obj_traverse_recursive_dependent_data data;
  data.tag = "[obj][flush]";
  data.traverse = grn_obj_flush_traverse;
  data.user_data = &traverse_data;
  data.for_reference = false;

  grn_obj *db = grn_ctx_db(ctx);
  grn_rc rc = grn_obj_flush_lock(ctx, db, traverse_data.tag);
  if (rc != GRN_SUCCESS) {
    GRN_API_RETURN(rc);
  }
  if (obj != db) {
    rc = grn_obj_flush_lock(ctx, obj, traverse_data.tag);
    if (rc != GRN_SUCCESS) {
      grn_obj_flush_unlock(ctx, db, traverse_data.tag);
      GRN_API_RETURN(rc);
    }
  }
  grn_obj_traverse_recursive_dependent(ctx, &data, obj);
  if (obj != db) {
    grn_obj_flush_unlock(ctx, obj, traverse_data.tag);
  }
  grn_obj_flush_unlock(ctx, db, traverse_data.tag);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_flush_only_opened(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;

  const char *tag = "[obj][flush][only-opened]";

  if (!grn_obj_is_db(ctx, obj)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, obj);
    ERR(GRN_INVALID_ARGUMENT,
        "%s DB is only support for now: <%.*s>",
        tag,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  grn_rc rc = grn_obj_flush_lock(ctx, obj, tag);
  if (rc != GRN_SUCCESS) {
    GRN_API_RETURN(rc);
  }

  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, obj, cursor, id, GRN_CURSOR_BY_ID)
  {
    if (id < GRN_N_RESERVED_TYPES) {
      continue;
    }

    if (!grn_ctx_is_opened(ctx, id)) {
      continue;
    }

    grn_obj *sub_obj = grn_ctx_at(ctx, id);
    rc = grn_obj_flush_internal(ctx, sub_obj, tag);
    grn_obj_unref(ctx, sub_obj);
    if (rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);
  if (rc == GRN_SUCCESS) {
    rc = grn_obj_flush_without_lock(ctx, obj, tag);
  }

  grn_rc rc_unlock = grn_obj_flush_unlock(ctx, obj, tag);
  if (rc == GRN_SUCCESS && rc_unlock != GRN_SUCCESS) {
    rc = rc_unlock;
  }

  GRN_API_RETURN(rc);
}

grn_obj *
grn_obj_db(grn_ctx *ctx, grn_obj *obj)
{
  grn_obj *db = NULL;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    db = DB_OBJ(obj)->db;
  }
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

int
grn_obj_defrag(grn_ctx *ctx, grn_obj *obj, int threshold)
{
  int r = 0;
  GRN_API_ENTER;
  switch (obj->header.type) {
  case GRN_DB:
    {
      grn_table_cursor *cur;
      if ((cur = grn_table_cursor_open(ctx, obj, NULL, 0, NULL, 0, 0, -1, 0))) {
        grn_id id;
        while ((id = grn_table_cursor_next_inline(ctx, cur)) != GRN_ID_NIL) {
          grn_obj *ja = grn_ctx_at(ctx, id);
          if (ja && ja->header.type == GRN_COLUMN_VAR_SIZE) {
            r += grn_ja_defrag(ctx, (grn_ja *)ja, threshold);
          }
        }
        grn_table_cursor_close(ctx, cur);
      }
    }
    break;
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    {
      grn_hash *cols;
      if ((cols = grn_hash_create(ctx,
                                  NULL,
                                  sizeof(grn_id),
                                  0,
                                  GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY))) {
        if (grn_table_columns(ctx, obj, "", 0, (grn_obj *)cols)) {
          grn_id *key;
          GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
            grn_obj *col = grn_ctx_at(ctx, *key);
            if (col) {
              r += grn_obj_defrag(ctx, col, threshold);
              grn_obj_unlink(ctx, col);
            }
          });
        }
        grn_hash_close(ctx, cols);
      }
    }
    break;
  case GRN_COLUMN_VAR_SIZE:
    r = grn_ja_defrag(ctx, (grn_ja *)obj, threshold);
    break;
  }
  GRN_API_RETURN(r);
}

static grn_obj *
deftype(grn_ctx *ctx, const char *name, grn_obj_flags flags, unsigned int size)
{
  grn_obj *o = grn_ctx_get(ctx, name, strlen(name));
  if (!o) {
    o = grn_type_create(ctx, name, strlen(name), flags, size);
  }
  return o;
}

grn_rc
grn_db_init_builtin_types(grn_ctx *ctx)
{
  grn_id id;
  grn_obj *obj, *db = ctx->impl->db;
  char buf[] = "Sys00";
  grn_obj_register(ctx, db, buf, 5);
  obj = deftype(ctx, "Object", GRN_OBJ_KEY_UINT, sizeof(uint64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_OBJECT) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Bool", GRN_OBJ_KEY_UINT, sizeof(uint8_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_BOOL) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Int8", GRN_OBJ_KEY_INT, sizeof(int8_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT8) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "UInt8", GRN_OBJ_KEY_UINT, sizeof(uint8_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT8) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Int16", GRN_OBJ_KEY_INT, sizeof(int16_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT16) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "UInt16", GRN_OBJ_KEY_UINT, sizeof(uint16_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT16) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Int32", GRN_OBJ_KEY_INT, sizeof(int32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT32) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "UInt32", GRN_OBJ_KEY_UINT, sizeof(uint32_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT32) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Int64", GRN_OBJ_KEY_INT, sizeof(int64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_INT64) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "UInt64", GRN_OBJ_KEY_UINT, sizeof(uint64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_UINT64) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Float", GRN_OBJ_KEY_FLOAT, sizeof(double));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_FLOAT) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Time", GRN_OBJ_KEY_INT, sizeof(int64_t));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TIME) {
    return GRN_FILE_CORRUPT;
  }
  obj =
    deftype(ctx, "ShortText", GRN_OBJ_KEY_VAR_SIZE, GRN_TYPE_SHORT_TEXT_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_SHORT_TEXT) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "Text", GRN_OBJ_KEY_VAR_SIZE, GRN_TYPE_TEXT_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TEXT) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx, "LongText", GRN_OBJ_KEY_VAR_SIZE, GRN_TYPE_LONG_TEXT_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_LONG_TEXT) {
    return GRN_FILE_CORRUPT;
  }
  obj =
    deftype(ctx, "TokyoGeoPoint", GRN_OBJ_KEY_GEO_POINT, sizeof(grn_geo_point));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_TOKYO_GEO_POINT) {
    return GRN_FILE_CORRUPT;
  }
  obj =
    deftype(ctx, "WGS84GeoPoint", GRN_OBJ_KEY_GEO_POINT, sizeof(grn_geo_point));
  if (!obj || DB_OBJ(obj)->id != GRN_DB_WGS84_GEO_POINT) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx,
                GRN_TYPE_FLOAT32_NAME,
                GRN_TYPE_FLOAT32_FLAGS,
                GRN_TYPE_FLOAT32_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_FLOAT32) {
    return GRN_FILE_CORRUPT;
  }
  obj = deftype(ctx,
                GRN_TYPE_BFLOAT16_NAME,
                GRN_TYPE_BFLOAT16_FLAGS,
                GRN_TYPE_BFLOAT16_SIZE);
  if (!obj || DB_OBJ(obj)->id != GRN_DB_BFLOAT16) {
    return GRN_FILE_CORRUPT;
  }
  for (id = grn_db_curr_id(ctx, db) + 1; id < GRN_DB_MECAB; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
#ifdef GRN_WITH_MECAB
  if (grn_db_init_mecab_tokenizer(ctx)) {
    ERRCLR(ctx);
#endif
    grn_obj_register(ctx, db, "TokenMecab", 10);
#ifdef GRN_WITH_MECAB
  }
#endif
  grn_db_init_builtin_tokenizers(ctx);
  grn_db_init_builtin_normalizers(ctx);
  grn_db_init_builtin_scorers(ctx);
  for (id = grn_db_curr_id(ctx, db) + 1; id < 128; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
  grn_db_init_builtin_commands(ctx);
  grn_db_init_builtin_window_functions(ctx);
  grn_db_init_builtin_token_filters(ctx);
  grn_db_init_builtin_aggregators(ctx);
  for (id = grn_db_curr_id(ctx, db) + 1; id < GRN_N_RESERVED_TYPES; id++) {
    grn_itoh(id, buf + 3, 2);
    grn_obj_register(ctx, db, buf, 5);
  }
  return ctx->rc;
}

#define MULTI_COLUMN_INDEXP(i) (DB_OBJ(i)->source_size > sizeof(grn_id))

static void
report_hook_has_dangling_reference_error(grn_ctx *ctx,
                                         grn_obj *target,
                                         grn_id reference_id,
                                         const char *context_tag)
{
  char target_name[GRN_TABLE_MAX_KEY_SIZE];
  int target_name_length;
  char reference_name[GRN_TABLE_MAX_KEY_SIZE];
  int reference_name_length;

  target_name_length =
    grn_obj_name(ctx, target, target_name, GRN_TABLE_MAX_KEY_SIZE);
  reference_name_length = grn_table_get_key(ctx,
                                            ctx->impl->db,
                                            reference_id,
                                            reference_name,
                                            GRN_TABLE_MAX_KEY_SIZE);
  ERR(GRN_OBJECT_CORRUPT,
      "%s hook has a dangling reference: <%.*s> -> <%.*s>(%u)",
      context_tag,
      target_name_length,
      target_name,
      reference_name_length,
      reference_name,
      reference_id);
}

static grn_bool
is_full_text_searchable_index(grn_ctx *ctx, grn_obj *index_column)
{
  grn_obj *tokenizer;

  tokenizer = grn_index_column_get_tokenizer(ctx, index_column);
  return tokenizer != NULL;
}

typedef struct {
  grn_operator op;
  grn_index_datum *index_data;
  uint32_t n_index_data;
  grn_obj **index_buffer;
  uint32_t n_index_buffers;
  int *section_buffer;
  grn_obj referred_objects;
  uint32_t n_found_indexes;
} grn_find_index_data;

static void
grn_find_index_data_init(grn_ctx *ctx,
                         grn_find_index_data *data,
                         grn_operator op,
                         grn_index_datum *index_data,
                         unsigned int n_index_data,
                         grn_obj **index_buffer,
                         int n_index_buffers,
                         int *section_buffer)
{
  data->op = op;
  data->index_data = index_data;
  data->n_index_data = n_index_data;
  data->index_buffer = index_buffer;
  data->n_index_buffers = n_index_buffers;
  data->section_buffer = section_buffer;
  if (data->section_buffer) {
    *(data->section_buffer) = 0;
  }
  GRN_PTR_INIT(&(data->referred_objects), GRN_OBJ_VECTOR, GRN_ID_NIL);
  data->n_found_indexes = 0;
}

static void
grn_find_index_data_fin(grn_ctx *ctx, grn_find_index_data *data)
{
  if (grn_enable_reference_count) {
    size_t i;
    size_t n = GRN_PTR_VECTOR_SIZE(&(data->referred_objects));
    for (i = 0; i < n; i++) {
      grn_obj *obj = GRN_PTR_VALUE_AT(&(data->referred_objects), i);
      grn_obj_unlink(ctx, obj);
    }
  }
  GRN_OBJ_FIN(ctx, &(data->referred_objects));
}

static void
grn_find_index_data_referred(grn_ctx *ctx,
                             grn_find_index_data *data,
                             grn_obj *index)
{
  if (grn_enable_reference_count) {
    GRN_PTR_PUT(ctx, &(data->referred_objects), index);
  }
}

static void
grn_find_index_data_refer(grn_ctx *ctx, grn_find_index_data *data, grn_obj *obj)
{
  if (!grn_enable_reference_count) {
    return;
  }

  grn_obj_refer(ctx, obj);
  grn_find_index_data_referred(ctx, data, obj);
}

static void
grn_find_index_data_found(grn_ctx *ctx,
                          grn_find_index_data *data,
                          grn_obj *index,
                          int section)
{
  if (grn_enable_reference_count) {
    size_t n_referred_objects = GRN_PTR_VECTOR_SIZE(&(data->referred_objects));
    if (n_referred_objects > 0) {
      grn_obj *last_referred_object =
        GRN_PTR_VALUE_AT(&(data->referred_objects), n_referred_objects - 1);
      if (last_referred_object == index) {
        grn_obj *found_object;
        GRN_PTR_POP(&(data->referred_objects), found_object);
      }
    }
  }

  if (data->section_buffer) {
    *(data->section_buffer) = section;
  }
  if (data->n_found_indexes < data->n_index_buffers) {
    data->index_buffer[data->n_found_indexes] = index;
  }
  if (data->n_found_indexes < data->n_index_data) {
    data->index_data[data->n_found_indexes].index = index;
    data->index_data[data->n_found_indexes].section = section;
  }
  data->n_found_indexes++;
}

static void
grn_column_find_index_data_column_equal(grn_ctx *ctx,
                                        grn_obj *obj,
                                        grn_find_index_data *data)
{
  grn_hook *hooks;

  for (hooks = DB_OBJ(obj)->hooks[GRN_HOOK_SET]; hooks; hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *hook_data =
      (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, hook_data->target);

    if (!target) {
      report_hook_has_dangling_reference_error(ctx,
                                               obj,
                                               hook_data->target,
                                               "[column][index]"
                                               "[column][equal]");
      continue;
    }

    grn_find_index_data_referred(ctx, data, target);
    if (target->header.type != GRN_COLUMN_INDEX) {
      continue;
    }
    if (!grn_index_column_is_usable(ctx, target, data->op)) {
      continue;
    }
    if (obj->header.type != GRN_COLUMN_FIX_SIZE) {
      if (is_full_text_searchable_index(ctx, target)) {
        continue;
      }
    }

    int section = (MULTI_COLUMN_INDEXP(target)) ? hook_data->section : 0;
    grn_find_index_data_found(ctx, data, target, section);
  }
}

static void
grn_column_find_index_data_column_match(grn_ctx *ctx,
                                        grn_obj *obj,
                                        grn_find_index_data *data)
{
  grn_hook_entry hook_entry;
  grn_hook *hooks;
  bool prefer_full_text_search_index = false;

  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    hook_entry = GRN_HOOK_INSERT;
    break;
  default:
    hook_entry = GRN_HOOK_SET;
    break;
  }

  switch (data->op) {
  case GRN_OP_PREFIX:
    switch (obj->header.type) {
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      grn_find_index_data_refer(ctx, data, obj);
      grn_find_index_data_found(ctx, data, obj, 0);
      break;
    default:
      break;
    }
    break;
  case GRN_OP_SUFFIX:
    if (obj->header.type == GRN_TABLE_PAT_KEY &&
        obj->header.flags & GRN_OBJ_KEY_WITH_SIS) {
      grn_find_index_data_refer(ctx, data, obj);
      grn_find_index_data_found(ctx, data, obj, 0);
    }
    break;
  case GRN_OP_MATCH:
    prefer_full_text_search_index = !grn_column_is_vector(ctx, obj);
    break;
  case GRN_OP_NEAR:
  case GRN_OP_NEAR_NO_OFFSET:
  case GRN_OP_NEAR_PHRASE:
  case GRN_OP_ORDERED_NEAR_PHRASE:
  case GRN_OP_NEAR_PHRASE_PRODUCT:
  case GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT:
  case GRN_OP_SIMILAR:
    prefer_full_text_search_index = true;
    break;
  default:
    break;
  }

  if (prefer_full_text_search_index) {
    for (hooks = DB_OBJ(obj)->hooks[hook_entry]; hooks; hooks = hooks->next) {
      grn_obj_default_set_value_hook_data *hook_data =
        (void *)GRN_NEXT_ADDR(hooks);
      grn_obj *target = grn_ctx_at(ctx, hook_data->target);
      if (!target) {
        report_hook_has_dangling_reference_error(ctx,
                                                 obj,
                                                 hook_data->target,
                                                 "[column][index]"
                                                 "[column][match]"
                                                 "[prefer-full-text-search]");
        continue;
      }

      grn_find_index_data_referred(ctx, data, target);
      if (target->header.type != GRN_COLUMN_INDEX) {
        continue;
      }
      if (!grn_index_column_is_usable(ctx, target, data->op)) {
        continue;
      }
      if (!is_full_text_searchable_index(ctx, target)) {
        continue;
      }

      int section = (MULTI_COLUMN_INDEXP(target)) ? hook_data->section : 0;
      grn_find_index_data_found(ctx, data, target, section);
    }
  }

  for (hooks = DB_OBJ(obj)->hooks[hook_entry]; hooks; hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *hook_data =
      (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, hook_data->target);

    if (!target) {
      report_hook_has_dangling_reference_error(ctx,
                                               obj,
                                               hook_data->target,
                                               "[column][index]"
                                               "[column][match]");
      continue;
    }

    grn_find_index_data_referred(ctx, data, target);
    if (target->header.type != GRN_COLUMN_INDEX) {
      continue;
    }
    if (!grn_index_column_is_usable(ctx, target, data->op)) {
      continue;
    }
    if (prefer_full_text_search_index) {
      if (is_full_text_searchable_index(ctx, target)) {
        continue;
      }
    }

    int section = (MULTI_COLUMN_INDEXP(target)) ? hook_data->section : 0;
    grn_find_index_data_found(ctx, data, target, section);
  }
}

static void
grn_column_find_index_data_column_range(grn_ctx *ctx,
                                        grn_obj *obj,
                                        grn_find_index_data *data)
{
  grn_hook_entry hook_entry;
  grn_hook *hooks;

  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    hook_entry = GRN_HOOK_INSERT;
    break;
  default:
    hook_entry = GRN_HOOK_SET;
    break;
  }

  for (hooks = DB_OBJ(obj)->hooks[hook_entry]; hooks; hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *hook_data =
      (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, hook_data->target);

    if (!target) {
      report_hook_has_dangling_reference_error(ctx,
                                               obj,
                                               hook_data->target,
                                               "[column][index]"
                                               "[column][range]");
      continue;
    }

    grn_find_index_data_referred(ctx, data, target);
    if (target->header.type != GRN_COLUMN_INDEX) {
      continue;
    }
    if (!grn_index_column_is_usable(ctx, target, data->op)) {
      continue;
    }

    int section = (MULTI_COLUMN_INDEXP(target)) ? hook_data->section : 0;
    grn_obj *lexicon = grn_ctx_at(ctx, target->header.domain);
    grn_obj *tokenizer;
    if (!lexicon) {
      report_hook_has_dangling_reference_error(ctx,
                                               target,
                                               target->header.domain,
                                               "[column][index]"
                                               "[column][range][lexicon]");
      continue;
    }
    /* FIXME: GRN_TABLE_DAT_KEY should be supported */
    if (lexicon->header.type != GRN_TABLE_PAT_KEY) {
      grn_obj_unref(ctx, lexicon);
      continue;
    }
    grn_table_get_info(ctx, lexicon, NULL, NULL, &tokenizer, NULL, NULL);
    grn_obj_unref(ctx, lexicon);
    if (tokenizer) {
      continue;
    }
    grn_find_index_data_found(ctx, data, target, section);
  }
}

static int
find_section(grn_ctx *ctx, grn_obj *index_column, grn_obj *indexed_column)
{
  int section = 0;
  grn_id indexed_column_id;
  grn_id *source_ids;
  int i, n_source_ids;

  indexed_column_id = DB_OBJ(indexed_column)->id;

  source_ids = DB_OBJ(index_column)->source;
  n_source_ids = DB_OBJ(index_column)->source_size / sizeof(grn_id);
  for (i = 0; i < n_source_ids; i++) {
    grn_id source_id = source_ids[i];
    if (source_id == indexed_column_id) {
      section = i + 1;
      break;
    }
  }

  return section;
}

static bool
grn_column_find_index_data_accessor_index_column(grn_ctx *ctx,
                                                 grn_accessor *a,
                                                 grn_find_index_data *data)
{
  grn_obj *index_column = a->obj;
  int section = 0;

  if (!grn_index_column_is_usable(ctx, index_column, data->op)) {
    return false;
  }

  /* Available patterns:
   *
   *   1. No section specified index: Index with section=0 is returned.
   *
   *      Example: select Memos --match_columns Tags.memos_tag
   *
   *        table_create Tags TABLE_HASH_KEY ShortText
   *        table_create Memos TABLE_HASH_KEY ShortText
   *        column_create Memos tag COLUMN_SCALAR Tags
   *        column_create Tags memos_tag COLUMN_INDEX Memos tag
   *
   *   2. Section specified index: Index with the specified section is returned.
   *
   *      Example: select Memos --match_columns Tags.memos_tag
   *
   *        table_create Memos TABLE_NO_KEY
   *        column_create Memos title COLUMN_SCALAR ShortText
   *        column_create Memos content COLUMN_SCALAR Text
   *        table_create Terms TABLE_PAT_KEY ShortText \
   *          --default_tokenizer TokenNgram \
   *          --normalizer NormalizerNFKC130
   *        column_create Terms memos \
   *          COLUMN_INDEX|WITH_POSITION|WITH_SECTION Memos title,content
   */

  if (a->next) {
    int specified_section;
    grn_bool is_invalid_section;
    if (a->next->next) {
      return false;
    }
    specified_section = find_section(ctx, index_column, a->next->obj);
    is_invalid_section = (specified_section == 0);
    if (is_invalid_section) {
      return false;
    }
    section = specified_section;
  }

  grn_find_index_data_refer(ctx, data, index_column);
  grn_find_index_data_found(ctx, data, index_column, section);

  return true;
}

static grn_bool
grn_column_find_index_data_accessor_is_key_search(grn_ctx *ctx,
                                                  grn_accessor *accessor,
                                                  grn_operator op)
{
  if (accessor->next) {
    return GRN_FALSE;
  }

  if (accessor->action != GRN_ACCESSOR_GET_KEY) {
    return GRN_FALSE;
  }

  if (!grn_obj_is_table(ctx, accessor->obj)) {
    return GRN_FALSE;
  }

  switch (op) {
  case GRN_OP_LESS:
  case GRN_OP_GREATER:
  case GRN_OP_LESS_EQUAL:
  case GRN_OP_GREATER_EQUAL:
  case GRN_OP_PREFIX:
    switch (accessor->obj->header.type) {
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      return GRN_TRUE;
    default:
      return GRN_FALSE;
    }
  case GRN_OP_SUFFIX:
    return (accessor->obj->header.type == GRN_TABLE_PAT_KEY &&
            (accessor->obj->header.flags & GRN_OBJ_KEY_WITH_SIS));
  case GRN_OP_EQUAL:
  case GRN_OP_NOT_EQUAL:
    switch (accessor->obj->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      return GRN_TRUE;
    default:
      return GRN_FALSE;
    }
  case GRN_OP_FUZZY:
    return (accessor->obj->header.type == GRN_TABLE_PAT_KEY);
  case GRN_OP_TERM_EXTRACT:
    switch (accessor->obj->header.type) {
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY:
      return true;
    default:
      return false;
    }
  default:
    return GRN_FALSE;
  }
}

static void
grn_column_find_index_data_accessor(grn_ctx *ctx,
                                    grn_obj *obj,
                                    grn_find_index_data *data)
{
  grn_accessor *a;
  int depth = 0;
  for (a = (grn_accessor *)obj; a; a = a->next, depth++) {
    grn_hook *hooks;
    bool found = false;
    bool nested_index = false;
    grn_hook_entry entry = (grn_hook_entry)-1;

    switch (a->action) {
    case GRN_ACCESSOR_GET_ID:
      if (!a->next) {
        found = true;
        grn_find_index_data_refer(ctx, data, obj);
        grn_find_index_data_found(ctx, data, obj, 0);
      }
      break;
    case GRN_ACCESSOR_GET_KEY:
      entry = GRN_HOOK_INSERT;
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE:
      if (GRN_OBJ_INDEX_COLUMNP(a->obj)) {
        if (grn_column_find_index_data_accessor_index_column(ctx, a, data)) {
          return;
        }
        /* Available pattern: Nested index.
         *
         * Example: select Tags --match_columns memos_tag._key
         *
         *   table_create Tags TABLE_HASH_KEY ShortText
         *   table_create Memos TABLE_HASH_KEY ShortText
         *   column_create Memos tag COLUMN_SCALAR Tags
         *   column_create Tags memos_tag COLUMN_INDEX Memos tag
         *   table_create Terms TABLE_PAT_KEY ShortText    \
         *     --default_tokenizer TokenNgram              \
         *     --normalizer NormalizerNFKC130
         *   column_create Terms memos_key COLUMN_INDEX|WITH_POSITION Memos _key
         */
        nested_index = true;
      } else {
        entry = GRN_HOOK_SET;
      }
      break;
    default:
      break;
    }

    if (nested_index) {
      if (!grn_index_column_is_usable(ctx, a->obj, GRN_OP_EQUAL)) {
        break;
      }
      continue;
    }

    if (entry == (grn_hook_entry)-1) {
      break;
    }

    for (hooks = DB_OBJ(a->obj)->hooks[entry]; hooks; hooks = hooks->next) {
      grn_obj_default_set_value_hook_data *hook_data =
        (void *)GRN_NEXT_ADDR(hooks);
      grn_obj *target = grn_ctx_at(ctx, hook_data->target);

      if (!target) {
        report_hook_has_dangling_reference_error(ctx,
                                                 obj,
                                                 hook_data->target,
                                                 "[column][index]"
                                                 "[accessor][match]");
        continue;
      }
      grn_find_index_data_referred(ctx, data, target);

      if (target->header.type != GRN_COLUMN_INDEX) {
        continue;
      }
      if (a->next) {
        if (!grn_index_column_is_usable(ctx, target, GRN_OP_EQUAL)) {
          continue;
        }
      }

      found = GRN_TRUE;
      if (!a->next) {
        if (!grn_index_column_is_usable(ctx, target, data->op)) {
          continue;
        }

        int section = (MULTI_COLUMN_INDEXP(target)) ? hook_data->section : 0;
        if (depth == 0) {
          grn_find_index_data_found(ctx, data, target, section);
        } else {
          grn_find_index_data_refer(ctx, data, obj);
          grn_find_index_data_found(ctx, data, obj, section);
        }
      }
    }

    if (!found &&
        grn_column_find_index_data_accessor_is_key_search(ctx, a, data->op)) {
      grn_find_index_data_refer(ctx, data, obj);
      grn_find_index_data_found(ctx, data, obj, 0);
    }

    if (!found && a->next && grn_obj_is_table(ctx, a->obj)) {
      found = true;
    }

    if (!found) {
      break;
    }
  }
}

static void
grn_find_index_data_dispatch(grn_ctx *ctx,
                             grn_obj *obj,
                             grn_find_index_data *data)
{
  if (GRN_DB_OBJP(obj)) {
    switch (data->op) {
    case GRN_OP_EQUAL:
    case GRN_OP_NOT_EQUAL:
      grn_column_find_index_data_column_equal(ctx, obj, data);
      break;
    case GRN_OP_PREFIX:
    case GRN_OP_SUFFIX:
    case GRN_OP_MATCH:
    case GRN_OP_NEAR:
    case GRN_OP_NEAR_NO_OFFSET:
    case GRN_OP_NEAR_PHRASE:
    case GRN_OP_ORDERED_NEAR_PHRASE:
    case GRN_OP_NEAR_PHRASE_PRODUCT:
    case GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT:
    case GRN_OP_SIMILAR:
    case GRN_OP_REGEXP:
    case GRN_OP_FUZZY:
    case GRN_OP_QUORUM:
      grn_column_find_index_data_column_match(ctx, obj, data);
      break;
    case GRN_OP_LESS:
    case GRN_OP_GREATER:
    case GRN_OP_LESS_EQUAL:
    case GRN_OP_GREATER_EQUAL:
    case GRN_OP_CALL:
      grn_column_find_index_data_column_range(ctx, obj, data);
      break;
    default:
      break;
    }
  } else if (GRN_ACCESSORP(obj)) {
    grn_column_find_index_data_accessor(ctx, obj, data);
  }
}

int
grn_column_index(grn_ctx *ctx,
                 grn_obj *obj,
                 grn_operator op,
                 grn_obj **index_buf,
                 int buf_size,
                 int *section_buf)
{
  GRN_API_ENTER;
  grn_find_index_data data;
  grn_find_index_data_init(ctx,
                           &data,
                           op,
                           NULL,
                           0,
                           index_buf,
                           buf_size,
                           section_buf);
  grn_find_index_data_dispatch(ctx, obj, &data);
  int n = data.n_found_indexes;
  grn_find_index_data_fin(ctx, &data);
  GRN_API_RETURN(n);
}

unsigned int
grn_column_find_index_data(grn_ctx *ctx,
                           grn_obj *obj,
                           grn_operator op,
                           grn_index_datum *index_data,
                           unsigned int n_index_data)
{
  GRN_API_ENTER;
  grn_find_index_data data;
  grn_find_index_data_init(ctx,
                           &data,
                           op,
                           index_data,
                           n_index_data,
                           NULL,
                           0,
                           NULL);
  grn_find_index_data_dispatch(ctx, obj, &data);
  unsigned int n = data.n_found_indexes;
  grn_find_index_data_fin(ctx, &data);
  GRN_API_RETURN(n);
}

static uint32_t
grn_column_get_all_index_data_column(grn_ctx *ctx,
                                     grn_obj *obj,
                                     grn_index_datum *index_data,
                                     uint32_t n_index_data,
                                     grn_obj *index_columns)
{
  uint32_t n = 0;
  grn_hook_entry hook_entry;
  grn_hook *hooks;

  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    hook_entry = GRN_HOOK_INSERT;
    break;
  default:
    hook_entry = GRN_HOOK_SET;
    break;
  }

  for (hooks = DB_OBJ(obj)->hooks[hook_entry]; hooks; hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    int section = 0;
    if (!target) {
      report_hook_has_dangling_reference_error(
        ctx,
        obj,
        data->target,
        "[column][indexes][all][column]");
      continue;
    }
    if (target->header.type != GRN_COLUMN_INDEX) {
      grn_obj_unref(ctx, target);
      continue;
    }
    if (!grn_obj_is_visible(ctx, target)) {
      grn_obj_unref(ctx, target);
      continue;
    }
    if (MULTI_COLUMN_INDEXP(target)) {
      section = data->section;
    }
    bool returned = false;
    if (n < n_index_data) {
      index_data[n].index = target;
      index_data[n].section = section;
      returned = true;
    }
    n++;
    if (index_columns) {
      GRN_PTR_PUT(ctx, index_columns, target);
      returned = true;
    }
    if (!returned) {
      grn_obj_unref(ctx, target);
    }
  }

  return n;
}

static uint32_t
grn_column_get_all_index_data_accessor_index_column(grn_ctx *ctx,
                                                    grn_accessor *a,
                                                    grn_index_datum *index_data,
                                                    uint32_t n_index_data,
                                                    grn_obj *index_columns)
{
  grn_obj *index_column = a->obj;
  int section = 0;

  if (a->next) {
    int specified_section;
    grn_bool is_invalid_section;
    if (a->next->next) {
      return 0;
    }
    specified_section = find_section(ctx, index_column, a->next->obj);
    is_invalid_section = (specified_section == 0);
    if (is_invalid_section) {
      return 0;
    }
    section = specified_section;
  }
  if (n_index_data > 0) {
    index_data[0].index = index_column;
    index_data[0].section = section;
  }
  if (index_columns) {
    GRN_PTR_PUT(ctx, index_columns, index_column);
  }

  return 1;
}

static uint32_t
grn_column_get_all_index_data_accessor(grn_ctx *ctx,
                                       grn_obj *obj,
                                       grn_index_datum *index_data,
                                       uint32_t n_index_data,
                                       grn_obj *index_columns)
{
  uint32_t n = 0;
  grn_accessor *a = (grn_accessor *)obj;

  while (a) {
    grn_hook *hooks;
    grn_bool found = GRN_FALSE;
    grn_hook_entry entry = (grn_hook_entry)-1;

    if (a->action == GRN_ACCESSOR_GET_COLUMN_VALUE &&
        GRN_OBJ_INDEX_COLUMNP(a->obj)) {
      return grn_column_get_all_index_data_accessor_index_column(ctx,
                                                                 a,
                                                                 index_data,
                                                                 n_index_data,
                                                                 index_columns);
    }

    switch (a->action) {
    case GRN_ACCESSOR_GET_KEY:
      entry = GRN_HOOK_INSERT;
      break;
    case GRN_ACCESSOR_GET_COLUMN_VALUE:
      entry = GRN_HOOK_SET;
      break;
    default:
      break;
    }

    if (entry == (grn_hook_entry)-1) {
      break;
    }

    for (hooks = DB_OBJ(a->obj)->hooks[entry]; hooks; hooks = hooks->next) {
      grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
      grn_obj *target = grn_ctx_at(ctx, data->target);

      if (!target) {
        report_hook_has_dangling_reference_error(ctx,
                                                 obj,
                                                 data->target,
                                                 "[column][indexes]"
                                                 "[all][accessor]");
        continue;
      }
      if (target->header.type != GRN_COLUMN_INDEX) {
        continue;
      }
      if (!grn_obj_is_visible(ctx, target)) {
        continue;
      }

      found = GRN_TRUE;
      if (!a->next) {
        int section = 0;

        if (MULTI_COLUMN_INDEXP(target)) {
          section = data->section;
        }
        if (n < n_index_data) {
          index_data[n].index = target;
          index_data[n].section = section;
        }
        n++;
        if (index_columns) {
          GRN_PTR_PUT(ctx, index_columns, target);
        }
      }
    }

    if (!found) {
      break;
    }
    a = a->next;
  }

  return n;
}

uint32_t
grn_column_get_all_index_data(grn_ctx *ctx,
                              grn_obj *obj,
                              grn_index_datum *index_data,
                              uint32_t n_index_data)
{
  uint32_t n = 0;
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    n = grn_column_get_all_index_data_column(ctx,
                                             obj,
                                             index_data,
                                             n_index_data,
                                             NULL);
  } else if (GRN_ACCESSORP(obj)) {
    n = grn_column_get_all_index_data_accessor(ctx,
                                               obj,
                                               index_data,
                                               n_index_data,
                                               NULL);
  }
  GRN_API_RETURN(n);
}

grn_rc
grn_column_get_all_index_columns(grn_ctx *ctx,
                                 grn_obj *obj,
                                 grn_obj *index_columns)
{
  GRN_API_ENTER;
  if (GRN_DB_OBJP(obj)) {
    grn_column_get_all_index_data_column(ctx, obj, NULL, 0, index_columns);
  } else if (GRN_ACCESSORP(obj)) {
    grn_column_get_all_index_data_accessor(ctx, obj, NULL, 0, index_columns);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_column_get_all_token_columns(grn_ctx *ctx,
                                 grn_obj *obj,
                                 grn_obj *token_columns)
{
  GRN_API_ENTER;

  if (!GRN_DB_OBJP(obj)) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_hook_entry hook_entry;
  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    hook_entry = GRN_HOOK_INSERT;
    break;
  default:
    hook_entry = GRN_HOOK_SET;
    break;
  }

  grn_hook *hooks;
  for (hooks = DB_OBJ(obj)->hooks[hook_entry]; hooks; hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    if (!target) {
      report_hook_has_dangling_reference_error(ctx,
                                               obj,
                                               data->target,
                                               "[column][token-columns][all]");
      continue;
    }
    if (target->header.type != GRN_COLUMN_VAR_SIZE) {
      grn_obj_unref(ctx, target);
      continue;
    }
    if (!grn_obj_is_visible(ctx, target)) {
      grn_obj_unref(ctx, target);
      continue;
    }
    GRN_PTR_PUT(ctx, token_columns, target);
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_column_get_all_hooked_columns(grn_ctx *ctx, grn_obj *obj, grn_obj *columns)
{
  GRN_API_ENTER;

  if (!GRN_DB_OBJP(obj)) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_hook_entry hook_entry;
  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY:
  case GRN_TABLE_PAT_KEY:
  case GRN_TABLE_DAT_KEY:
  case GRN_TABLE_NO_KEY:
    hook_entry = GRN_HOOK_INSERT;
    break;
  default:
    hook_entry = GRN_HOOK_SET;
    break;
  }

  grn_hook *hooks;
  for (hooks = DB_OBJ(obj)->hooks[hook_entry]; hooks; hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    if (!target) {
      report_hook_has_dangling_reference_error(ctx,
                                               obj,
                                               data->target,
                                               "[column][hooked-columns][all]");
      continue;
    }
    if (!grn_obj_is_visible(ctx, target)) {
      grn_obj_unref(ctx, target);
      continue;
    }
    GRN_PTR_PUT(ctx, columns, target);
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_obj_columns(grn_ctx *ctx,
                grn_obj *table,
                const char *str,
                unsigned int str_size,
                grn_obj *res)
{
  grn_obj *col;
  const char *p = (char *)str, *q, *r, *pe = p + str_size, *tokbuf[256];
  while (p < pe) {
    int i, n = grn_tokenize(p, pe - p, tokbuf, 256, &q);
    for (i = 0; i < n; i++) {
      r = tokbuf[i];
      while (p < r && (' ' == *p || ',' == *p)) {
        p++;
      }
      if (p < r) {
        if (r[-1] == '*') {
          grn_hash *cols =
            grn_hash_create(ctx,
                            NULL,
                            sizeof(grn_id),
                            0,
                            GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
          if (cols) {
            grn_id *key;
            grn_table_columns(ctx, table, p, r - p - 1, (grn_obj *)cols);
            GRN_HASH_EACH(ctx, cols, id, &key, NULL, NULL, {
              if ((col = grn_ctx_at(ctx, *key))) {
                GRN_PTR_PUT(ctx, res, col);
              }
            });
            grn_hash_close(ctx, cols);
          }
          {
            grn_obj *type = grn_ctx_at(ctx, table->header.domain);
            if (GRN_OBJ_TABLEP(type)) {
              grn_obj *ai = grn_obj_column(ctx,
                                           table,
                                           GRN_COLUMN_NAME_ID,
                                           GRN_COLUMN_NAME_ID_LEN);
              if (ai) {
                if (ai->header.type == GRN_ACCESSOR) {
                  grn_accessor *id_accessor;
                  for (id_accessor = ((grn_accessor *)ai)->next; id_accessor;
                       id_accessor = id_accessor->next) {
                    grn_obj *target_table = id_accessor->obj;

                    cols =
                      grn_hash_create(ctx,
                                      NULL,
                                      sizeof(grn_id),
                                      0,
                                      GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
                    if (!cols) {
                      continue;
                    }

                    grn_table_columns(ctx,
                                      target_table,
                                      p,
                                      r - p - 1,
                                      (grn_obj *)cols);
                    GRN_HASH_EACH_BEGIN(ctx, cols, cursor, id)
                    {
                      void *key;
                      grn_hash_cursor_get_key(ctx, cursor, &key);
                      grn_id column_id = *((grn_id *)key);
                      col = grn_ctx_at(ctx, column_id);
                      if (!col) {
                        continue;
                      }

                      grn_accessor *a;
                      grn_accessor *ac;
                      ac = grn_accessor_new(ctx);
                      GRN_PTR_PUT(ctx, res, (grn_obj *)ac);
                      for (a = (grn_accessor *)ai; a; a = a->next) {
                        if (a != id_accessor) {
                          ac->action = a->action;
                          ac->obj = a->obj;
                          grn_obj_refer(ctx, ac->obj);
                          ac->next = grn_accessor_new(ctx);
                          if (!(ac = ac->next)) {
                            break;
                          }
                        } else {
                          ac->action = GRN_ACCESSOR_GET_COLUMN_VALUE;
                          ac->obj = col;
                          ac->next = NULL;
                          break;
                        }
                      }
                    }
                    GRN_HASH_EACH_END(ctx, cursor);
                    grn_hash_close(ctx, cols);
                  }
                }
                grn_obj_unlink(ctx, ai);
              }
            }
            grn_obj_unref(ctx, type);
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

grn_obj *
grn_table_column(grn_ctx *ctx,
                 grn_obj *table,
                 const char *name,
                 ssize_t name_size)
{
  GRN_API_ENTER;

  if (name_size < 0) {
    name_size = strlen(name);
  }

  grn_obj *column = grn_obj_column_(ctx, table, name, name_size);
  if (grn_obj_is_accessor(ctx, column)) {
    grn_obj_unlink(ctx, column);
    column = NULL;
  }

  GRN_API_RETURN(column);
}

grn_rc
grn_table_parse_load_columns(grn_ctx *ctx,
                             grn_obj *table,
                             const char *input,
                             size_t input_size,
                             grn_obj *columns)
{
  GRN_API_ENTER;
  const char *current = input;
  const char *end = input + input_size;
#define TOKEN_MAX 256
  const char *tokens[TOKEN_MAX];
  while (current < end) {
    const char *rest;
    int n = grn_tokenize(current, end - current, tokens, TOKEN_MAX, &rest);
    int i;
    for (i = 0; i < n; i++) {
      const char *next_start = tokens[i];
      while (current < next_start && (' ' == *current || ',' == *current)) {
        current++;
      }
      if (current < next_start) {
        grn_raw_string column_name;
        column_name.value = current;
        column_name.length = next_start - current;
        if (column_name.value[0] == '_') {
          if (grn_obj_is_table_with_key(ctx, table) &&
              GRN_RAW_STRING_EQUAL_CSTRING(column_name, GRN_COLUMN_NAME_KEY)) {
            grn_accessor *key_accessor = grn_accessor_new_key(ctx, table);
            if (key_accessor) {
              GRN_PTR_PUT(ctx, columns, key_accessor);
            }
          }
        } else {
          grn_obj *column =
            grn_table_column(ctx, table, current, next_start - current);
          if (column) {
            GRN_PTR_PUT(ctx, columns, column);
          }
        }
      }
      current = next_start;
    }
    current = rest;
  }
#undef TOKEN_MAX
  GRN_API_RETURN(ctx->rc);
}

bool
grn_table_is_grouped(grn_ctx *ctx, grn_obj *table)
{
  if (GRN_OBJ_TABLEP(table) && GRN_TABLE_IS_GROUPED(table)) {
    return true;
  }
  return false;
}

unsigned int
grn_table_max_n_subrecs(grn_ctx *ctx, grn_obj *table)
{
  if (GRN_OBJ_TABLEP(table)) {
    return DB_OBJ(table)->max_n_subrecs;
  }
  return 0;
}

grn_obj *
grn_table_tokenize(grn_ctx *ctx,
                   grn_obj *table,
                   const char *str,
                   unsigned int str_len,
                   grn_obj *buf,
                   bool addp)
{
  grn_token_cursor *token_cursor = NULL;
  grn_tokenize_mode mode = addp ? GRN_TOKENIZE_ADD : GRN_TOKENIZE_GET;
  GRN_API_ENTER;
  if (!(token_cursor =
          grn_token_cursor_open(ctx, table, str, str_len, mode, 0))) {
    goto exit;
  }
  if (buf) {
    GRN_BULK_REWIND(buf);
  } else {
    if (!(buf = grn_obj_open(ctx, GRN_UVECTOR, 0, DB_OBJ(table)->id))) {
      goto exit;
    }
  }
  while (token_cursor->status != GRN_TOKEN_CURSOR_DONE &&
         token_cursor->status != GRN_TOKEN_CURSOR_DONE_SKIP) {
    grn_id tid;
    if ((tid = grn_token_cursor_next(ctx, token_cursor))) {
      GRN_RECORD_PUT(ctx, buf, tid);
    }
  }
exit:
  if (token_cursor) {
    grn_token_cursor_close(ctx, token_cursor);
  }
  GRN_API_RETURN(buf);
}

static void
grn_db_recover_database_remove_orphan_inspect(grn_ctx *ctx, grn_obj *db)
{
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, db, cursor, id, GRN_CURSOR_BY_ID)
  {
    void *key;
    int key_size;

    key_size = grn_table_cursor_get_key(ctx, cursor, &key);
#define INSPECT     "inspect"
#define INSPECT_LEN (sizeof(INSPECT) - 1)
    if (key_size == INSPECT_LEN && memcmp(key, INSPECT, INSPECT_LEN) == 0) {
      if (!grn_ctx_at(ctx, id)) {
        ERRCLR(ctx);
        grn_obj_delete_by_id(ctx, db, id, GRN_TRUE);
        GRN_LOG(ctx,
                GRN_LOG_INFO,
                "[db][recover] removed orphan 'inspect' object: <%u>",
                id);
      }
      break;
    }
#undef INSPECT
#undef INSPECT_LEN
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static void
grn_db_recover_database(grn_ctx *ctx, grn_obj *db)
{
  if (grn_obj_is_locked(ctx, db)) {
    ERR(GRN_OBJECT_CORRUPT,
        "[db][recover] database may be broken. Please re-create the database");
    return;
  }

  grn_db_clear_dirty(ctx, db);
  grn_db_recover_database_remove_orphan_inspect(ctx, db);
}

static bool
grn_db_recover_table_is_broken(grn_ctx *ctx, grn_obj *table)
{
  if (grn_obj_is_locked(ctx, table)) {
    return true;
  }

  if (grn_obj_is_table_with_key(ctx, table) &&
      grn_table_have_duplicated_keys(ctx, table)) {
    return true;
  }

  return false;
}

static void
grn_db_recover_table(grn_ctx *ctx, grn_obj *table)
{
  if (!grn_db_recover_table_is_broken(ctx, table)) {
    return;
  }

  if (grn_obj_is_lexicon_without_data_column(ctx, table)) {
    grn_table_truncate(ctx, table);
    grn_obj_reindex(ctx, table);
  } else {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    unsigned int name_size;
    name_size = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_OBJECT_CORRUPT,
        "[db][recover] table may be broken: <%.*s>: "
        "please truncate the table (or clear lock of the table) "
        "and load data again",
        (int)name_size,
        name);
  }
}

static void
grn_db_recover_data_column(grn_ctx *ctx, grn_obj *data_column)
{
  if (!grn_obj_is_locked(ctx, data_column)) {
    return;
  }

  {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    unsigned int name_size;
    name_size = grn_obj_name(ctx, data_column, name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_OBJECT_CORRUPT,
        "[db][recover] column may be broken: <%.*s>: "
        "please truncate the column (or clear lock of the column) "
        "and load data again",
        (int)name_size,
        name);
  }
}

static void
grn_db_recover_index_column(grn_ctx *ctx, grn_obj *index_column)
{
  if (!grn_obj_is_locked(ctx, index_column)) {
    return;
  }

  grn_index_column_rebuild(ctx, index_column);
}

static grn_bool
grn_db_recover_is_builtin(grn_ctx *ctx, grn_id id, grn_table_cursor *cursor)
{
  void *key;
  const char *name;
  int name_size;

  if (id < GRN_N_RESERVED_TYPES) {
    return GRN_TRUE;
  }

  name_size = grn_table_cursor_get_key(ctx, cursor, &key);
  name = key;

#define NAME_EQUAL(value)                                                      \
  (name_size == strlen(value) && memcmp(name, value, strlen(value)) == 0)

  if (NAME_EQUAL("inspect")) {
    /* Just for compatibility. It's needed for users who used
       Groonga master at between 2016-02-03 and 2016-02-26. */
    return GRN_TRUE;
  }

#undef NAME_EQUAL

  return GRN_FALSE;
}

grn_rc
grn_db_recover(grn_ctx *ctx, grn_obj *db)
{
  grn_table_cursor *cursor;
  grn_id id;
  grn_bool is_close_opened_object_mode;

  GRN_API_ENTER;

  is_close_opened_object_mode = (grn_thread_get_limit() == 1);

  grn_db_recover_database(ctx, db);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_API_RETURN(ctx->rc);
  }

  cursor =
    grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
  if (!cursor) {
    GRN_API_RETURN(ctx->rc);
  }

  while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    grn_obj *object;

    if (is_close_opened_object_mode) {
      grn_ctx_push_temporary_open_space(ctx);
    }

    if ((object = grn_ctx_at(ctx, id))) {
      switch (object->header.type) {
      case GRN_TABLE_NO_KEY:
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
        grn_db_recover_table(ctx, object);
        break;
      case GRN_COLUMN_FIX_SIZE:
      case GRN_COLUMN_VAR_SIZE:
        grn_db_recover_data_column(ctx, object);
        break;
      case GRN_COLUMN_INDEX:
        grn_db_recover_index_column(ctx, object);
        break;
      default:
        break;
      }
      grn_obj_unlink(ctx, object);
    } else {
      if (grn_db_recover_is_builtin(ctx, id, cursor)) {
        ERRCLR(ctx);
      }
    }

    if (is_close_opened_object_mode) {
      grn_ctx_pop_temporary_open_space(ctx);
    }

    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  grn_table_cursor_close(ctx, cursor);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_db_unmap(grn_ctx *ctx, grn_obj *db)
{
  grn_db *s = (grn_db *)db;

  GRN_API_ENTER;

  GRN_TINY_ARRAY_EACH_BEGIN(&s->values, 1, grn_db_curr_id(ctx, db), value)
  {
    db_value *vp = value;
    grn_obj *obj = vp->ptr;

    if (obj) {
      switch (obj->header.type) {
      case GRN_COLUMN_FIX_SIZE:
      case GRN_COLUMN_VAR_SIZE:
      case GRN_COLUMN_INDEX:
        grn_obj_close(ctx, obj);
        break;
      }
    }
  }
  GRN_TINY_ARRAY_EACH_END();

  GRN_TINY_ARRAY_EACH_BEGIN(&s->values, 1, grn_db_curr_id(ctx, db), value)
  {
    db_value *vp = value;
    grn_obj *obj = vp->ptr;

    if (obj) {
      switch (obj->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
      case GRN_TABLE_NO_KEY:
        grn_obj_close(ctx, obj);
        break;
      }
    }
  }
  GRN_TINY_ARRAY_EACH_END();

  GRN_API_RETURN(ctx->rc);
}

static grn_rc
grn_ctx_get_all_objects(grn_ctx *ctx,
                        grn_obj *objects_buffer,
                        grn_bool (*predicate)(grn_ctx *ctx, grn_obj *object))
{
  grn_obj *db;
  grn_table_cursor *cursor;
  grn_id id;

  GRN_API_ENTER;

  db = ctx->impl->db;
  if (!db) {
    ERR(GRN_INVALID_ARGUMENT, "DB isn't associated");
    GRN_API_RETURN(ctx->rc);
  }

  cursor = grn_table_cursor_open(ctx, db, NULL, 0, NULL, 0, 0, -1, 0);
  if (!cursor) {
    GRN_API_RETURN(ctx->rc);
  }

  while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    grn_obj *object;

    if ((object = grn_ctx_at(ctx, id))) {
      if (predicate(ctx, object)) {
        GRN_PTR_PUT(ctx, objects_buffer, object);
      } else {
        grn_obj_unlink(ctx, object);
      }
    } else {
      if (ctx->rc != GRN_SUCCESS) {
        ERRCLR(ctx);
      }
    }
  }
  grn_table_cursor_close(ctx, cursor);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_ctx_get_all_tables(grn_ctx *ctx, grn_obj *tables_buffer)
{
  return grn_ctx_get_all_objects(ctx, tables_buffer, grn_obj_is_table);
}

grn_rc
grn_ctx_get_all_types(grn_ctx *ctx, grn_obj *types_buffer)
{
  return grn_ctx_get_all_objects(ctx, types_buffer, grn_obj_is_type);
}

grn_rc
grn_ctx_get_all_tokenizers(grn_ctx *ctx, grn_obj *tokenizers_buffer)
{
  return grn_ctx_get_all_objects(ctx,
                                 tokenizers_buffer,
                                 grn_obj_is_tokenizer_proc);
}

grn_rc
grn_ctx_get_all_normalizers(grn_ctx *ctx, grn_obj *normalizers_buffer)
{
  return grn_ctx_get_all_objects(ctx,
                                 normalizers_buffer,
                                 grn_obj_is_normalizer_proc);
}

grn_rc
grn_ctx_get_all_token_filters(grn_ctx *ctx, grn_obj *token_filters_buffer)
{
  return grn_ctx_get_all_objects(ctx,
                                 token_filters_buffer,
                                 grn_obj_is_token_filter_proc);
}

grn_rc
grn_ctx_push_temporary_open_space(grn_ctx *ctx)
{
  grn_obj *stack;
  grn_obj *space;
  grn_obj buffer;

  if (grn_enable_reference_count) {
    return ctx->rc;
  }

  GRN_API_ENTER;

  stack = &(ctx->impl->temporary_open_spaces.stack);
  GRN_VOID_INIT(&buffer);
  grn_bulk_write(ctx, stack, (const char *)&buffer, sizeof(grn_obj));
  space = ((grn_obj *)GRN_BULK_CURR(stack)) - 1;
  GRN_PTR_INIT(space, GRN_OBJ_VECTOR | GRN_OBJ_OWN, GRN_ID_NIL);

  ctx->impl->temporary_open_spaces.current = space;

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_ctx_pop_temporary_open_space(grn_ctx *ctx)
{
  grn_obj *stack;
  grn_obj *space;

  if (grn_enable_reference_count) {
    return ctx->rc;
  }

  GRN_API_ENTER;

  stack = &(ctx->impl->temporary_open_spaces.stack);
  if (GRN_BULK_EMPTYP(stack)) {
    ERR(GRN_INVALID_ARGUMENT, "[ctx][temporary-open-spaces][pop] too much pop");
    GRN_API_RETURN(ctx->rc);
  }

  space = ctx->impl->temporary_open_spaces.current;
  GRN_OBJ_FIN(ctx, space);
  grn_bulk_truncate(ctx, stack, GRN_BULK_VSIZE(stack) - sizeof(grn_obj));

  if (GRN_BULK_EMPTYP(stack)) {
    space = NULL;
  } else {
    space = ((grn_obj *)GRN_BULK_CURR(stack)) - 1;
  }
  ctx->impl->temporary_open_spaces.current = space;

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_ctx_merge_temporary_open_space(grn_ctx *ctx)
{
  grn_obj *stack;
  grn_obj *space;
  grn_obj *next_space;

  if (grn_enable_reference_count) {
    return ctx->rc;
  }

  GRN_API_ENTER;

  stack = &(ctx->impl->temporary_open_spaces.stack);
  if (GRN_BULK_VSIZE(stack) < sizeof(grn_obj) * 2) {
    ERR(GRN_INVALID_ARGUMENT,
        "[ctx][temporary-open-spaces][merge] "
        "merge requires at least two spaces");
    GRN_API_RETURN(ctx->rc);
  }

  space = ctx->impl->temporary_open_spaces.current;
  next_space = ctx->impl->temporary_open_spaces.current - 1;
  {
    unsigned int i, n_elements;
    n_elements = GRN_PTR_VECTOR_SIZE(space);
    for (i = 0; i < n_elements; i++) {
      grn_obj *element = GRN_PTR_VALUE_AT(space, i);
      GRN_PTR_PUT(ctx, next_space, element);
    }
  }
  GRN_BULK_REWIND(space);
  GRN_OBJ_FIN(ctx, space);
  grn_bulk_truncate(ctx, stack, GRN_BULK_VSIZE(stack) - sizeof(grn_obj));

  if (GRN_BULK_EMPTYP(stack)) {
    space = NULL;
  } else {
    space = ((grn_obj *)GRN_BULK_CURR(stack)) - 1;
  }
  ctx->impl->temporary_open_spaces.current = space;

  GRN_API_RETURN(ctx->rc);
}
