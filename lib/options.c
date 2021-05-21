/*
  Copyright(C) 2018 Brazil

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

#include "grn_db.h"
#include "grn_msgpack.h"
#include "grn_options.h"
#include "grn_util.h"

#include <stdio.h>

#ifdef GRN_WITH_MESSAGE_PACK
# include <groonga/msgpack.h>
#endif /* GRN_WITH_MESSAGE_PACK */

struct _grn_options {
  grn_ja *values;
};

static const char *GRN_OPTIONS_PATH_FORMAT = "%s.options";
static const unsigned int GRN_OPTIONS_MAX_VALUE_SIZE = 65536;

grn_options *
grn_options_create(grn_ctx *ctx,
                   const char *path,
                   const char *context_tag)
{
  char *options_path;
  char options_path_buffer[PATH_MAX];
  grn_options *options;
  uint32_t flags = 0;

  if (path) {
    grn_snprintf(options_path_buffer,
                 PATH_MAX,
                 PATH_MAX,
                 GRN_OPTIONS_PATH_FORMAT,
                 path);
    options_path = options_path_buffer;
  } else {
    options_path = NULL;
  }

  options = GRN_MALLOC(sizeof(grn_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options: <%s>",
        context_tag,
        options_path ? options_path : "(temporary)");
    return NULL;
  }

  options->values = grn_ja_create(ctx,
                                  options_path,
                                  GRN_OPTIONS_MAX_VALUE_SIZE,
                                  flags);
  if (!options->values) {
    GRN_FREE(options);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to create data store for options: <%s>",
        context_tag,
        options_path ? options_path : "(temporary)");
    return NULL;
  }

  return options;
}

grn_options *
grn_options_open(grn_ctx *ctx,
                 const char *path,
                 const char *context_tag)
{
  char options_path[PATH_MAX];
  grn_options *options;

  grn_snprintf(options_path,
               PATH_MAX,
               PATH_MAX,
               GRN_OPTIONS_PATH_FORMAT,
               path);
  if (!grn_path_exist(options_path)) {
    return grn_options_create(ctx, path, context_tag);
  }

  options = GRN_MALLOC(sizeof(grn_options));
  if (!options) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate memory for options: <%s>",
        context_tag,
        options_path);
    return NULL;
  }

  options->values = grn_ja_open(ctx, options_path);
  if (!options->values) {
    GRN_FREE(options);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to open data store for options: <%s>",
        context_tag,
        options_path);
    return NULL;
  }

  return options;
}

grn_rc
grn_options_close(grn_ctx *ctx, grn_options *options)
{
  grn_rc rc;

  if (!options) {
    return GRN_SUCCESS;
  }

  rc = grn_ja_close(ctx, options->values);
  GRN_FREE(options);

  return rc;
}

grn_rc
grn_options_remove(grn_ctx *ctx, const char *path)
{
  char options_path[PATH_MAX];

  grn_snprintf(options_path,
               PATH_MAX,
               PATH_MAX,
               GRN_OPTIONS_PATH_FORMAT,
               path);
  return grn_ja_remove(ctx, options_path);
}

grn_bool
grn_options_is_locked(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_is_locked(ctx, (grn_obj *)(options->values));
}

grn_rc
grn_options_clear_lock(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_clear_lock(ctx, (grn_obj *)(options->values));
}

grn_bool
grn_options_is_corrupt(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_is_corrupt(ctx, (grn_obj *)(options->values));
}

grn_rc
grn_options_flush(grn_ctx *ctx, grn_options *options)
{
  return grn_obj_flush(ctx, (grn_obj *)(options->values));
}

#ifdef GRN_WITH_MESSAGE_PACK
typedef struct {
  grn_ctx *ctx;
  grn_obj *buffer;
} grn_options_msgpack_write_data;

static int
grn_options_msgpack_write(void *data, const char *buf, msgpack_size_t len)
{
  grn_options_msgpack_write_data *write_data = data;
  return grn_bulk_write(write_data->ctx,
                        write_data->buffer,
                        buf,
                        len);
}
#endif /* GRN_WITH_MESSAGE_PACK */

grn_rc
grn_options_set(grn_ctx *ctx,
                grn_options *options,
                grn_id id,
                const char *name,
                int name_length,
                grn_obj *values)
{
#ifdef GRN_WITH_MESSAGE_PACK
  msgpack_packer packer;
  grn_options_msgpack_write_data data;
  grn_obj buffer;
  grn_io_win iw;
  void *raw_value;
  uint32_t length;
  unsigned int i, n;

  if (name_length < 0) {
    name_length = strlen(name);
  }

  GRN_TEXT_INIT(&buffer, 0);
  data.ctx = ctx;
  data.buffer = &buffer;

  msgpack_packer_init(&packer, &data, grn_options_msgpack_write);

  raw_value = grn_ja_ref(ctx, options->values, id, &iw, &length);
  if (raw_value) {
    msgpack_unpacker unpacker;
    msgpack_unpacked unpacked;

    msgpack_unpacker_init(&unpacker, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);
    msgpack_unpacked_init(&unpacked);

    msgpack_unpacker_reserve_buffer(&unpacker, length);
    grn_memcpy(msgpack_unpacker_buffer(&unpacker), raw_value, length);
    msgpack_unpacker_buffer_consumed(&unpacker, length);
    while (MSGPACK_UNPACKER_NEXT(&unpacker, &unpacked)) {
      msgpack_object *object = &(unpacked.data);
      if (object->type == MSGPACK_OBJECT_MAP) {
        msgpack_object_map *map = &(object->via.map);
        uint32_t i;
        grn_bool have_same_key = GRN_FALSE;
        uint32_t same_key_index = 0;

        for (i = 0; i < map->size; i++) {
          msgpack_object_kv *kv = &(map->ptr[i]);
          msgpack_object *key = &(kv->key);
          if (key->type == MSGPACK_OBJECT_STR &&
              name_length == MSGPACK_OBJECT_STR_SIZE(key) &&
              memcmp(name, MSGPACK_OBJECT_STR_PTR(key), name_length) == 0) {
            have_same_key = GRN_TRUE;
            same_key_index = i;
            break;
          }
        }
        if (have_same_key) {
          msgpack_pack_map(&packer, map->size);
        } else {
          msgpack_pack_map(&packer, map->size + 1);
        }
        for (i = 0; i < map->size; i++) {
          msgpack_object_kv *kv = &(map->ptr[i]);
          if (have_same_key && i == same_key_index) {
            continue;
          }
          msgpack_pack_object(&packer, kv->key);
          msgpack_pack_object(&packer, kv->val);
        }
      }
    }

    msgpack_unpacked_destroy(&unpacked);
    msgpack_unpacker_destroy(&unpacker);

    grn_ja_unref(ctx, &iw);
  } else {
    msgpack_pack_map(&packer, 1);
  }

  msgpack_pack_str(&packer, name_length);
  msgpack_pack_str_body(&packer, name, name_length);

  n = grn_vector_size(ctx, values);
  msgpack_pack_array(&packer, n);
  for (i = 0; i < n; i++) {
    const char *value;
    unsigned int value_size;
    grn_id value_domain;

    value_size = grn_vector_get_element(ctx,
                                        values,
                                        i,
                                        &value,
                                        NULL,
                                        &value_domain);
    grn_msgpack_pack_raw_internal(ctx, &packer, value, value_size, value_domain);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }

  if (ctx->rc == GRN_SUCCESS) {
    grn_ja_put(ctx,
               options->values,
               id,
               GRN_TEXT_VALUE(&buffer),
               GRN_TEXT_LEN(&buffer),
               GRN_OBJ_SET,
               NULL);
  }
  GRN_OBJ_FIN(ctx, &buffer);
#else /* GRN_WITH_MESSAGE_PACK */
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "[options] MessagePack is required");
#endif /* GRN_WITH_MESSAGE_PACK */
  return ctx->rc;
}

grn_option_revision
grn_options_get(grn_ctx *ctx,
                grn_options *options,
                grn_id id,
                const char *name,
                int name_length,
                grn_option_revision revision,
                grn_obj *value)
{
#ifdef GRN_WITH_MESSAGE_PACK
  grn_bool found = GRN_FALSE;
  grn_io_win iw;
  void *raw_value;
  uint32_t length;
  msgpack_unpacker unpacker;
  msgpack_unpacked unpacked;

  raw_value = grn_ja_ref(ctx, options->values, id, &iw, &length);
  if (!raw_value) {
    return GRN_OPTION_REVISION_NONE;
  }

  if (raw_value == revision) {
    grn_ja_unref(ctx, &iw);
    return GRN_OPTION_REVISION_UNCHANGED;
  }

  if (name_length < 0) {
    name_length = strlen(name);
  }

  msgpack_unpacker_init(&unpacker, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);
  msgpack_unpacker_reserve_buffer(&unpacker, length);
  msgpack_unpacked_init(&unpacked);
  grn_memcpy(msgpack_unpacker_buffer(&unpacker),
             iw.addr,
             length);
  msgpack_unpacker_buffer_consumed(&unpacker, length);
  while (MSGPACK_UNPACKER_NEXT(&unpacker, &unpacked)) {
    msgpack_object *object = &(unpacked.data);
    if (object->type == MSGPACK_OBJECT_MAP) {
      msgpack_object_map *map = &(object->via.map);
      uint32_t i;
      for (i = 0; i < map->size; i++) {
        msgpack_object_kv *kv = &(map->ptr[i]);
        if (kv->key.type == MSGPACK_OBJECT_STR &&
            MSGPACK_OBJECT_STR_SIZE(&(kv->key)) == name_length &&
            memcmp(MSGPACK_OBJECT_STR_PTR(&(kv->key)), name, name_length) == 0) {
          if (kv->val.type == MSGPACK_OBJECT_ARRAY) {
            grn_msgpack_unpack_array_internal(ctx,
                                              &(kv->val.via.array),
                                              value);
            found = GRN_TRUE;
          }
          break;
        }
      }
    }
  }
  msgpack_unpacked_destroy(&unpacked);
  msgpack_unpacker_destroy(&unpacker);

  grn_ja_unref(ctx, &iw);

  return raw_value;
#else /* GRN_WITH_MESSAGE_PACK */
  return GRN_OPTION_REVISION_NONE;
#endif /* GRN_WITH_MESSAGE_PACK */
}

grn_rc
grn_options_clear(grn_ctx *ctx,
                  grn_options *options,
                  grn_id id)
{
  return grn_ja_put(ctx, options->values, id, NULL, 0, GRN_OBJ_SET, NULL);
}
