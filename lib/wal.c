/*
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_msgpack.h"
#include "grn_obj.h"
#include "grn_wal.h"

#include <sys/stat.h>

#ifdef WIN32
# include <share.h>
#endif

const char *
grn_wal_key_type_to_string(grn_wal_key_type type)
{
  const char *string = "unknown";
  switch (type) {
  case GRN_WAL_KEY_END :
    string = "end";
    break;
  case GRN_WAL_KEY_ID :
    string = "id";
    break;
  case GRN_WAL_KEY_EVENT :
    string = "event";
    break;
  case GRN_WAL_KEY_RECORD_ID :
    string = "record-id";
    break;
  case GRN_WAL_KEY_VALUE :
    string = "value";
    break;
  }
  return string;
}

const char *
grn_wal_reader_value_type_to_string(grn_wal_reader_value_type type)
{
  const char *string = "unknown";
  switch (type) {
  case GRN_WAL_READER_VALUE_NIL :
    string = "nil";
    break;
  case GRN_WAL_READER_VALUE_INT64 :
    string = "int64";
    break;
  case GRN_WAL_READER_VALUE_UINT64 :
    string = "uint64";
    break;
  case GRN_WAL_READER_VALUE_BINARY :
    string = "binary";
    break;
  }
  return string;
}

/* TODO: object local counter is enough only for crash recovery. But
 * timestamp is needed for computing delta from WAL. */
uint64_t
grn_wal_generate_id(grn_ctx *ctx)
{
  grn_timeval tv;
  grn_timeval_now(ctx, &tv);
  return GRN_TIME_PACK(tv.tv_sec,
                       GRN_TIME_NSEC_TO_USEC(tv.tv_nsec));
}

grn_rc
grn_wal_add_entry(grn_ctx *ctx,
                  grn_obj *obj,
                  bool need_lock,
                  uint64_t *wal_id,
                  const char *tag,
                  grn_wal_key_type key_type,
                  ...)
{
  va_list args;

  va_start(args, key_type);
  grn_rc rc = grn_wal_add_entryv(ctx,
                                 obj,
                                 need_lock,
                                 wal_id,
                                 tag,
                                 key_type,
                                 args);
  va_end(args);

  return rc;
}

#ifdef GRN_WITH_MESSAGE_PACK
typedef struct {
  grn_ctx *ctx;
  FILE *output;
} grn_wal_msgpack_write_data;

static int
grn_wal_msgpack_write(void *data, const char *buf, msgpack_size_t len)
{
  grn_wal_msgpack_write_data *write_data = data;
  return fwrite(buf, len, 1, write_data->output);
}
#endif

static void
grn_wal_generate_path(grn_ctx *ctx,
                      grn_io *io,
                      char *path)
{
  grn_strcpy(path, PATH_MAX, io->path);
  grn_strcat(path, PATH_MAX, ".wal");
}

grn_rc
grn_wal_add_entryv(grn_ctx *ctx,
                   grn_obj *obj,
                   bool need_lock,
                   uint64_t *wal_id,
                   const char *tag,
                   grn_wal_key_type key_type,
                   va_list args)
{
  if (ctx->impl->wal.role == GRN_WAL_ROLE_NONE) {
    return GRN_SUCCESS;
  }

#ifdef GRN_WITH_MESSAGE_PACK
  grn_io *io = grn_obj_get_io(ctx, obj);
  if (io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  grn_rc rc = GRN_SUCCESS;
  if (need_lock) {
    rc = grn_io_lock(ctx, io, grn_lock_timeout);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  char path[PATH_MAX];
  grn_wal_generate_path(ctx, io, path);
  FILE *output = grn_fopen(path, "ab");
  if (!output) {
    GRN_DEFINE_NAME(obj);
    SERR("[wal][add][%.*s]%s failed to open file: <%s>",
         name_size, name,
         tag,
         path);
    rc = ctx->rc;
    goto exit;
  }

  msgpack_packer packer;
  grn_wal_msgpack_write_data data;
  data.ctx = ctx;
  data.output = output;
  msgpack_packer_init(&packer, &data, grn_wal_msgpack_write);

  uint8_t n_keys = 0;
  {
    grn_wal_key_type current_key_type = key_type;
    va_list copied_args;
    va_copy(copied_args, args);
    while (current_key_type != GRN_WAL_KEY_END) {
      n_keys++;
      grn_wal_value_type value_type = va_arg(copied_args, grn_wal_value_type);
      switch (value_type) {
      case GRN_WAL_VALUE_EVENT :
        va_arg(copied_args, grn_wal_event_type);
        break;
      case GRN_WAL_VALUE_RECORD_ID :
        va_arg(copied_args, grn_id);
        break;
      case GRN_WAL_VALUE_BINARY :
        va_arg(copied_args, void *);
        va_arg(copied_args, uint32_t);
        break;
      default :
        break;
      }
      current_key_type = va_arg(copied_args, grn_wal_key_type);
    }
    va_end(copied_args);
  }

  msgpack_pack_map(&packer, n_keys + 1);
  msgpack_pack_uint8(&packer, GRN_WAL_KEY_ID);
  *wal_id = grn_wal_generate_id(ctx);
  msgpack_pack_uint64(&packer, *wal_id);
  {
    grn_wal_key_type current_key_type = key_type;
    while (current_key_type != GRN_WAL_KEY_END) {
      msgpack_pack_uint8(&packer, current_key_type);
      grn_wal_value_type value_type = va_arg(args, grn_wal_value_type);
      switch (value_type) {
      case GRN_WAL_VALUE_NIL :
        msgpack_pack_nil(&packer);
        break;
      case GRN_WAL_VALUE_EVENT :
        {
          grn_wal_value_type value = va_arg(args, grn_wal_value_type);
          msgpack_pack_int32(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_RECORD_ID :
        {
          grn_id value = va_arg(args, grn_id);
          msgpack_pack_uint32(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_INT64 :
        {
          int64_t value = va_arg(args, int64_t);
          msgpack_pack_uint64(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_UINT64 :
        {
          uint64_t value = va_arg(args, uint64_t);
          msgpack_pack_uint64(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_BINARY :
        {
          void *value = va_arg(args, void *);
          size_t size = va_arg(args, size_t);
          msgpack_pack_bin(&packer, size);
          msgpack_pack_bin_body(&packer, value, size);
        }
        break;
      default :
        {
          GRN_DEFINE_NAME(obj);
          rc = GRN_INVALID_ARGUMENT;
          ERR(rc,
              "[wal][add][%.*s]%s unknown value type: <%u>",
              name_size, name,
              tag,
              value_type);
          goto exit;
        }
        break;
      }
      current_key_type = va_arg(args, grn_wal_key_type);
    }
  }

exit :
  if (output) {
    fflush(output);
# ifdef HAVE_FSYNC
    fsync(fileno(output));
# endif
    fclose(output);
  }

  if (need_lock) {
    grn_io_unlock(io);
  }

  return rc;
#else
  return GRN_SUCCESS;
#endif
}

grn_rc
grn_wal_clear(grn_ctx *ctx,
              grn_obj *obj,
              bool need_lock,
              const char *tag)
{
  if (ctx->impl->wal.role == GRN_WAL_ROLE_NONE) {
    return GRN_SUCCESS;
  }

  grn_io *io = grn_obj_get_io(ctx, obj);
  if (io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  grn_rc rc = GRN_SUCCESS;
  if (need_lock) {
    rc = grn_io_lock(ctx, io, grn_lock_timeout);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  char path[PATH_MAX];
  grn_wal_generate_path(ctx, io, path);
  struct stat s;
  if (stat(path, &s) == 0) {
    GRN_DEFINE_NAME(obj);
    if (grn_unlink(path) == 0) {
      GRN_LOG(ctx, GRN_LOG_DEBUG,
              "[wal][clear][%.*s][%s] removed: <%s>",
              name_size, name,
              tag,
              path);
    } else {
      SERR("[wal][clear][%.*s]%s failed to remove: <%s>",
           name_size, name,
           tag,
           path);
      rc = ctx->rc;
    }
  }

  if (need_lock) {
    grn_io_unlock(io);
  }

  return rc;
}

struct grn_wal_reader_ {
  grn_obj *obj;
  const char *tag;
  FILE *input;
  grn_wal_key_type key_type;
  grn_wal_value_type value_type;
#ifdef GRN_WITH_MESSAGE_PACK
  msgpack_unpacker unpacker;
  msgpack_unpacked unpacked;
  msgpack_object_kv *kv;
#endif
  size_t kv_offset;
};

static grn_wal_reader *
grn_wal_reader_open_internal(grn_ctx *ctx,
                             grn_obj *obj,
                             const char *tag)
{
  grn_io *io = grn_obj_get_io(ctx, obj);
  if (io->path[0] == '\0') {
    return NULL;
  }

#ifdef GRN_WITH_MESSAGE_PACK
  char path[PATH_MAX];
  grn_wal_generate_path(ctx, io, path);
  FILE *input = grn_fopen(path, "rb");
  if (!input) {
    return NULL;
  }

  grn_wal_reader *reader = GRN_MALLOCN(grn_wal_reader, 1);
  if (!reader) {
    GRN_DEFINE_NAME(obj);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[wal][reader][open][%.*s]%s failed to allocate grn_wal_reader",
        name_size, name,
        tag);
    return NULL;
  }

  reader->obj = obj;
  reader->tag = tag;
  reader->input = input;
  msgpack_unpacker_init(&(reader->unpacker), MSGPACK_UNPACKER_INIT_BUFFER_SIZE);
  msgpack_unpacked_init(&(reader->unpacked));
  reader->kv = NULL;
  reader->kv_offset = 0;

  return reader;
#else
  return NULL;
#endif
}

grn_wal_reader *
grn_wal_reader_open(grn_ctx *ctx,
                    grn_obj *obj,
                    const char *tag)
{
  if (ctx->impl->wal.role == GRN_WAL_ROLE_NONE) {
    return NULL;
  }

  return grn_wal_reader_open_internal(ctx, obj, tag);
}

grn_rc
grn_wal_reader_close(grn_ctx *ctx,
                     grn_wal_reader *reader)
{
  if (!reader) {
    return GRN_SUCCESS;
  }

#ifdef GRN_WITH_MESSAGE_PACK
  msgpack_unpacked_destroy(&(reader->unpacked));
  msgpack_unpacker_destroy(&(reader->unpacker));
  fclose(reader->input);
  GRN_FREE(reader);
#endif
  return GRN_SUCCESS;
}

#ifdef GRN_WITH_MESSAGE_PACK
static bool
grn_wal_reader_read_next(grn_ctx *ctx,
                         grn_wal_reader *reader)
{
  while (true) {
    if (MSGPACK_UNPACKER_NEXT(&(reader->unpacker), &(reader->unpacked))) {
      return true;
    }
    size_t buffer_size = 4096;
    msgpack_unpacker_reserve_buffer(&(reader->unpacker), buffer_size);
    size_t read_size = fread(msgpack_unpacker_buffer(&(reader->unpacker)),
                             1,
                             buffer_size,
                             reader->input);
    if (read_size == 0) {
      return false;
    }
    msgpack_unpacker_buffer_consumed(&(reader->unpacker), read_size);
  }
  return false;
}
#endif

grn_rc
grn_wal_reader_read_entry(grn_ctx *ctx,
                          grn_wal_reader *reader,
                          grn_wal_key_type key_type,
                          ...)
{
  if (!reader) {
    return GRN_END_OF_DATA;
  }

#ifdef GRN_WITH_MESSAGE_PACK
  const char *tag = "[tag][reader][read-record]";
  if (!grn_wal_reader_read_next(ctx, reader)) {
    return GRN_END_OF_DATA;
  }

  if (reader->unpacked.data.type != MSGPACK_OBJECT_MAP) {
    GRN_DEFINE_NAME(reader->obj);
    ERR(GRN_FILE_CORRUPT,
        "%s[%.*s]%s must be map: <%s>(%u)",
        tag,
        name_size, name,
        reader->tag,
        grn_msgpack_object_type_to_string(reader->unpacked.data.type),
        reader->unpacked.data.type);
    return ctx->rc;
  }

  msgpack_object_map *map = &(reader->unpacked.data.via.map);

  va_list args;
  va_start(args, key_type);
  for (;
       key_type != GRN_WAL_KEY_END;
       key_type = va_arg(args, grn_wal_key_type)) {
    msgpack_object *value = NULL;
    uint32_t i;
    for (i = 0; i < map->size; i++) {
      msgpack_object_kv *kv = &(map->ptr[i]);
      if (kv->key.type != MSGPACK_OBJECT_POSITIVE_INTEGER) {
        continue;
      }
      grn_wal_key_type current_key_type = kv->key.via.u64;
      if (key_type != current_key_type) {
        continue;
      }
      value = &(kv->val);
      break;
    }
    if (!value) {
      GRN_DEFINE_NAME(reader->obj);
      ERR(GRN_FILE_CORRUPT,
          "%s[%.*s]%s requested key is missing: <%s>(%u)",
          tag,
          name_size, name,
          reader->tag,
          grn_wal_key_type_to_string(key_type),
          key_type);
      break;
    }
    switch (key_type) {
    case GRN_WAL_KEY_ID :
      {
        uint64_t *output = va_arg(args, uint64_t *);
        *output = value->via.u64;
      }
      break;
    case GRN_WAL_KEY_EVENT :
      {
        grn_wal_event_type *output = va_arg(args, grn_wal_event_type *);
        *output = value->via.u64;
      }
      break;
    case GRN_WAL_KEY_RECORD_ID :
      {
        grn_id *output = va_arg(args, grn_id *);
        *output = value->via.u64;
      }
      break;
    case GRN_WAL_KEY_VALUE :
      switch (value->type) {
      case MSGPACK_OBJECT_BOOLEAN :
        {
          bool *output = va_arg(args, bool *);
          *output = value->via.boolean;
        }
        break;
      case MSGPACK_OBJECT_POSITIVE_INTEGER :
        {
          uint64_t *output = va_arg(args, uint64_t *);
          *output = value->via.u64;
        }
        break;
      case MSGPACK_OBJECT_NEGATIVE_INTEGER :
        {
          int64_t *output = va_arg(args, int64_t *);
          *output = value->via.i64;
        }
        break;
      case MSGPACK_OBJECT_FLOAT32 :
        {
          float *output = va_arg(args, float *);
          *output = value->via.f64;
        }
        break;
      case MSGPACK_OBJECT_FLOAT64 :
        {
          double *output = va_arg(args, double *);
          *output = value->via.f64;
        }
        break;
      case MSGPACK_OBJECT_STR :
        {
          const char **output = va_arg(args, const char **);
          size_t *output_size = va_arg(args, size_t *);
          *output = value->via.str.ptr;
          *output_size = value->via.str.size;
        }
        break;
      case MSGPACK_OBJECT_BIN :
        {
          const void **output = va_arg(args, const void **);
          size_t *output_size = va_arg(args, size_t *);
          *output = value->via.bin.ptr;
          *output_size = value->via.bin.size;
        }
        break;
      default :
        {
          GRN_DEFINE_NAME(reader->obj);
          ERR(GRN_INVALID_ARGUMENT,
              "%s[%.*s]%s unsupported MessagePack type: <%s>(%u)",
              tag,
              name_size, name,
              reader->tag,
              grn_msgpack_object_type_to_string(value->type),
              value->type);
        }
        break;
      }
      break;
    default :
      {
        GRN_DEFINE_NAME(reader->obj);
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%.*s]%s unsupported key type: <%s>(%u)",
            tag,
            name_size, name,
            reader->tag,
            grn_wal_key_type_to_string(key_type),
            key_type);
      }
      break;
    }
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  va_end(args);
#endif
  return ctx->rc;
}

grn_rc
grn_wal_dump(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;

  grn_wal_reader *reader = grn_wal_reader_open_internal(ctx, obj, "");
  if (!reader) {
    GRN_API_RETURN(ctx->rc);
  }

#ifdef GRN_WITH_MESSAGE_PACK
  GRN_DEFINE_NAME(obj);
  printf("wal:start:%.*s\n", name_size, name);
  uint32_t i;
  for (i = 0; grn_wal_reader_read_next(ctx, reader); i++) {
    if (reader->unpacked.data.type != MSGPACK_OBJECT_MAP) {
      continue;
    }

    printf("entry:start:%u\n", i);
    msgpack_object_map *map = &(reader->unpacked.data.via.map);
    uint32_t j;
    for (j = 0; j < map->size; j++) {
      msgpack_object_kv *kv = &(map->ptr[j]);
      grn_wal_key_type key_type = kv->key.via.u64;
      printf("%u:%s(%u):%s(%u):",
             j,
             grn_wal_key_type_to_string(key_type),
             key_type,
             grn_msgpack_object_type_to_string(kv->val.type),
             kv->val.type);
      switch (kv->val.type) {
      case MSGPACK_OBJECT_NIL :
        printf("(nil)");
        break;
      case MSGPACK_OBJECT_BOOLEAN :
        printf("%s", kv->val.via.boolean ? "true" : "false");
        break;
      case MSGPACK_OBJECT_POSITIVE_INTEGER :
        printf("%" GRN_FMT_INT64U, kv->val.via.u64);
        break;
      case MSGPACK_OBJECT_NEGATIVE_INTEGER :
        printf("%" GRN_FMT_INT64D, kv->val.via.i64);
        break;
      case MSGPACK_OBJECT_FLOAT32 :
      case MSGPACK_OBJECT_FLOAT64 :
        printf("%f", kv->val.via.f64);
        break;
      case MSGPACK_OBJECT_STR :
        printf("%.*s",
               (int)(kv->val.via.str.size),
               kv->val.via.str.ptr);
        break;
      default :
        printf("...");
        break;
      }
      printf("\n");
    }
    printf("entry:end:%u\n", i);
  }
  printf("wal:end:%.*s\n", name_size, name);
  grn_wal_reader_close(ctx, reader);
#endif
  GRN_API_RETURN(ctx->rc);
}
