/*
  Copyright(C) 2021-2022  Sutou Kouhei <kou@clear-code.com>

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
grn_wal_event_to_string(grn_wal_event event)
{
  const char *string = "unknown";
  switch (event) {
  case GRN_WAL_EVENT_NIL :
    string = "nil";
    break;
  case GRN_WAL_EVENT_SET_VALUE :
    string = "set-value";
    break;
  case GRN_WAL_EVENT_NEW_SEGMENT :
    string = "new-segment";
    break;
  case GRN_WAL_EVENT_USE_SEGMENT :
    string = "use-segment";
    break;
  case GRN_WAL_EVENT_REUSE_SEGMENT :
    string = "reuse-segment";
    break;
  case GRN_WAL_EVENT_FREE_SEGMENT :
    string = "free-segment";
    break;
  case GRN_WAL_EVENT_ADD_ENTRY :
    string = "add-entry";
    break;
  case GRN_WAL_EVENT_ADD_SHARED_ENTRY :
    string = "add-shared-entry";
    break;
  case GRN_WAL_EVENT_REUSE_ENTRY :
    string = "reuse-entry";
    break;
  case GRN_WAL_EVENT_REUSE_SHARED_ENTRY :
    string = "reuse-shared-entry";
    break;
  case GRN_WAL_EVENT_RESET_ENTRY :
    string = "reset-entry";
    break;
  case GRN_WAL_EVENT_ENABLE_ENTRY :
    string = "enable-entry";
    break;
  case GRN_WAL_EVENT_SET_ENTRY_KEY :
    string = "set-entry-key";
    break;
  case GRN_WAL_EVENT_DELETE_ENTRY :
    string = "delete-entry";
    break;
  case GRN_WAL_EVENT_UPDATE_ENTRY :
    string = "update-entry";
    break;
  case GRN_WAL_EVENT_REHASH :
    string = "rehash";
    break;
  case GRN_WAL_EVENT_DELETE_INFO_PHASE1 :
    string = "delete-info-phase1";
    break;
  case GRN_WAL_EVENT_DELETE_INFO_PHASE2 :
    string = "delete-info-phase2";
    break;
  case GRN_WAL_EVENT_DELETE_INFO_PHASE3 :
    string = "delete-info-phase3";
    break;
  }
  return string;
}

const char *
grn_wal_segment_type_to_string(grn_wal_segment_type type)
{
  const char *string = "unknown";
  switch (type) {
  case GRN_WAL_SEGMENT_SEQUENTIAL :
    string = "sequential";
    break;
  case GRN_WAL_SEGMENT_CHUNK :
    string = "chunk";
    break;
  case GRN_WAL_SEGMENT_HUGE :
    string = "huge";
    break;
  case GRN_WAL_SEGMENT_EINFO :
    string = "einfo";
    break;
  case GRN_WAL_SEGMENT_GINFO :
    string = "ginfo";
    break;
  }
  return string;
}

const char *
grn_wal_key_to_string(grn_wal_key key)
{
  const char *string = "unknown";
  switch (key) {
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
  case GRN_WAL_KEY_RECORD_DIRECTION :
    string = "record-direction";
    break;
  case GRN_WAL_KEY_ELEMENT_SIZE :
    string = "element-size";
    break;
  case GRN_WAL_KEY_KEY :
    string = "key";
    break;
  case GRN_WAL_KEY_KEY_SIZE :
    string = "key-size";
    break;
  case GRN_WAL_KEY_KEY_HASH_VALUE :
    string = "key-hash-value";
    break;
  case GRN_WAL_KEY_KEY_OFFSET :
    string = "key-offset";
    break;
  case GRN_WAL_KEY_SHARED_KEY_OFFSET :
    string = "shared-key-offset";
    break;
  case GRN_WAL_KEY_IS_SHARED :
    string = "is-shared";
    break;
  case GRN_WAL_KEY_CHECK :
    string = "check";
    break;
  case GRN_WAL_KEY_NEW_KEY :
    string = "new-key";
    break;
  case GRN_WAL_KEY_PARENT_RECORD_ID :
    string = "parent-record-id";
    break;
  case GRN_WAL_KEY_PARENT_RECORD_DIRECTION :
    string = "parent-record-direction";
    break;
  case GRN_WAL_KEY_PARENT_CHECK :
    string = "parent-check";
    break;
  case GRN_WAL_KEY_GRANDPARENT_RECORD_ID :
    string = "grandparent-record-id";
    break;
  case GRN_WAL_KEY_OTHERSIDE_RECORD_ID :
    string = "otherside-record-id";
    break;
  case GRN_WAL_KEY_OTHERSIDE_CHECK :
    string = "otherside-check";
    break;
  case GRN_WAL_KEY_LEFT_RECORD_ID :
    string = "left-record-id";
    break;
  case GRN_WAL_KEY_RIGHT_RECORD_ID :
    string = "right-record-id";
    break;
  case GRN_WAL_KEY_VALUE :
    string = "value";
    break;
  case GRN_WAL_KEY_INDEX_HASH_VALUE :
    string = "index-hash-value";
    break;
  case GRN_WAL_KEY_SEGMENT :
    string = "segment";
    break;
  case GRN_WAL_KEY_POSITION :
    string = "position";
    break;
  case GRN_WAL_KEY_SEGMENT_TYPE :
    string = "segment-type";
    break;
  case GRN_WAL_KEY_SEGMENT_INFO :
    string = "segment-info";
    break;
  case GRN_WAL_KEY_GARBAGE_SEGMENT :
    string = "garbage-segment";
    break;
  case GRN_WAL_KEY_PREVIOUS_GARBAGE_SEGMENT :
    string = "previous-garbage-segment";
    break;
  case GRN_WAL_KEY_NEXT_GARBAGE_SEGMENT :
    string = "next-garbage-segment";
    break;
  case GRN_WAL_KEY_GARBAGE_SEGMENT_HEAD :
    string = "garbage-segment-head";
    break;
  case GRN_WAL_KEY_GARBAGE_SEGMENT_TAIL :
    string = "garbage-segment-tail";
    break;
  case GRN_WAL_KEY_GARBAGE_SEGMENT_N_RECORDS :
    string = "garbage-n-records";
    break;
  case GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID :
    string = "next-garbage-record-id";
    break;
  case GRN_WAL_KEY_N_GARBAGES :
    string = "n-garbages";
    break;
  case GRN_WAL_KEY_N_ENTRIES :
    string = "n-entries";
    break;
  case GRN_WAL_KEY_FOUND_GARBAGE :
    string = "found-garbage";
    break;
  case GRN_WAL_KEY_MAX_OFFSET :
    string = "max-offset";
    break;
  case GRN_WAL_KEY_EXPECTED_N_ENTRIES :
    string = "expected-n-entries";
    break;
  case GRN_WAL_KEY_DELETE_INFO_INDEX :
    string = "delete-info-index";
    break;
  case GRN_WAL_KEY_DELETE_INFO_PHASE1_INDEX :
    string = "delete-info-phase1-index";
    break;
  case GRN_WAL_KEY_DELETE_INFO_PHASE2_INDEX :
    string = "delete-info-phase2-index";
    break;
  case GRN_WAL_KEY_DELETE_INFO_PHASE3_INDEX :
    string = "delete-info-phase3-index";
    break;
  case GRN_WAL_KEY_PREVIOUS_RECORD_ID :
    string = "previous-record-id";
    break;
  }
  return string;
}

const char *
grn_wal_reader_data_type_to_string(grn_wal_reader_data_type type)
{
  const char *string = "unknown";
  switch (type) {
  case GRN_WAL_READER_DATA_NIL :
    string = "nil";
    break;
  case GRN_WAL_READER_DATA_BOOLEAN :
    string = "boolean";
    break;
  case GRN_WAL_READER_DATA_INT64 :
    string = "int64";
    break;
  case GRN_WAL_READER_DATA_UINT64 :
    string = "uint64";
    break;
  case GRN_WAL_READER_DATA_FLOAT32 :
    string = "float32";
    break;
  case GRN_WAL_READER_DATA_FLOAT64 :
    string = "float64";
    break;
  case GRN_WAL_READER_DATA_BINARY :
    string = "binary";
    break;
  case GRN_WAL_READER_DATA_STRING :
    string = "string";
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
                  grn_wal_key key,
                  ...)
{
  va_list args;

  va_start(args, key);
  grn_rc rc = grn_wal_add_entryv(ctx,
                                 obj,
                                 need_lock,
                                 wal_id,
                                 tag,
                                 key,
                                 args);
  va_end(args);

  return rc;
}

#ifdef GRN_WITH_MESSAGE_PACK
typedef struct {
  grn_ctx *ctx;
# ifdef _WIN32
  HANDLE output;
# else
  FILE *output;
# endif
} grn_wal_msgpack_write_data;

static int
grn_wal_msgpack_write(void *data, const char *buf, msgpack_size_t len)
{
  grn_wal_msgpack_write_data *write_data = data;
# ifdef _WIN32
  DWORD n_written = 0;
  if (WriteFile(write_data->output, buf, len, &n_written, NULL)) {
    return n_written;
  } else {
    return 0;
  }
# else
  return fwrite(buf, len, 1, write_data->output);
# endif
}
#endif

static void
grn_wal_generate_path(grn_ctx *ctx,
                      const char *path,
                      char *wal_path)
{
  grn_strcpy(wal_path, PATH_MAX, path);
  grn_strcat(wal_path, PATH_MAX, ".wal");
}

grn_rc
grn_wal_add_entryv(grn_ctx *ctx,
                   grn_obj *obj,
                   bool need_lock,
                   uint64_t *wal_id,
                   const char *tag,
                   grn_wal_key key,
                   va_list args)
{
  if (GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_NONE) {
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
  grn_wal_generate_path(ctx, io->path, path);
# ifdef _WIN32
  HANDLE output =
    CreateFile(path,
               FILE_APPEND_DATA | SYNCHRONIZE,
               FILE_SHARE_READ | FILE_SHARE_DELETE,
               NULL,
               OPEN_ALWAYS,
               FILE_ATTRIBUTE_NORMAL,
               NULL);
  bool open_success = (output != INVALID_HANDLE_VALUE);
# else
  FILE *output = grn_fopen(path, "ab");
  bool open_success = (output != NULL);
# endif
  if (!open_success) {
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
    va_list copied_args;
    va_copy(copied_args, args);
    grn_wal_key current_key;
    for (current_key = key;
         current_key != GRN_WAL_KEY_END;
         current_key = va_arg(copied_args, grn_wal_key)) {
      n_keys++;
      grn_wal_value_type value_type = va_arg(copied_args, grn_wal_value_type);
      switch (value_type) {
      case GRN_WAL_VALUE_EVENT :
        va_arg(copied_args, grn_wal_event);
        break;
      case GRN_WAL_VALUE_RECORD_ID :
        va_arg(copied_args, grn_id);
        break;
      case GRN_WAL_VALUE_SEGMENT_TYPE :
        va_arg(copied_args, grn_wal_segment_type);
        break;
      case GRN_WAL_VALUE_BOOLEAN :
        va_arg(copied_args, int);
        break;
      case GRN_WAL_VALUE_INT32 :
        va_arg(copied_args, int32_t);
        break;
      case GRN_WAL_VALUE_UINT32 :
        va_arg(copied_args, uint32_t);
        break;
      case GRN_WAL_VALUE_INT64 :
        va_arg(copied_args, int64_t);
        break;
      case GRN_WAL_VALUE_UINT64 :
        va_arg(copied_args, uint64_t);
        break;
      case GRN_WAL_VALUE_BINARY :
        va_arg(copied_args, void *);
        va_arg(copied_args, size_t);
        break;
      default :
        break;
      }
    }
    va_end(copied_args);
  }

  msgpack_pack_map(&packer, n_keys + 1);
  msgpack_pack_uint8(&packer, GRN_WAL_KEY_ID);
  *wal_id = grn_wal_generate_id(ctx);
  msgpack_pack_uint64(&packer, *wal_id);
  {
    grn_wal_key current_key;
    for (current_key = key;
         current_key != GRN_WAL_KEY_END;
         current_key = va_arg(args, grn_wal_key)) {
      msgpack_pack_uint8(&packer, current_key);
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
      case GRN_WAL_VALUE_SEGMENT_TYPE :
        {
          grn_wal_segment_type value = va_arg(args, grn_wal_segment_type);
          msgpack_pack_uint32(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_BOOLEAN :
        {
          int value = va_arg(args, int);
          if (value) {
            msgpack_pack_true(&packer);
          } else {
            msgpack_pack_false(&packer);
          }
        }
        break;
      case GRN_WAL_VALUE_INT32 :
        {
          int32_t value = va_arg(args, int32_t);
          msgpack_pack_int32(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_UINT32 :
        {
          uint32_t value = va_arg(args, uint32_t);
          msgpack_pack_uint32(&packer, value);
        }
        break;
      case GRN_WAL_VALUE_INT64 :
        {
          int64_t value = va_arg(args, int64_t);
          msgpack_pack_int64(&packer, value);
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
    }
  }

exit :
  if (open_success) {
# ifndef _WIN32
    fflush(output);
# endif
    /*
     * Synchronizing on each WAL write has very large write
     * performance penalty.  We can't accept it... We disable
     * synchronizing for now.
     *
     * This works on application crash not OS crash because OS will
     * flush the buffered data after application crash. But not
     * flushed data are lost on OS crash.
     */
    /*
# ifdef _WIN32
    FlushFileBuffers(output);
# else
#  ifdef HAVE_FDATASYNC
    fdatasync(fileno(output));
#  elif defined(HAVE_FSYNC)
    fsync(fileno(output));
#  endif
# endif
    */
# ifdef _WIN32
    CloseHandle(output);
# else
    fclose(output);
# endif
  }

  if (need_lock) {
    grn_io_unlock(ctx, io);
  }

  return rc;
#else
  return GRN_SUCCESS;
#endif
}

grn_rc
grn_wal_touch(grn_ctx *ctx,
              grn_obj *obj,
              bool need_lock,
              const char *tag)
{
  if (GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_NONE) {
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
  grn_wal_generate_path(ctx, io->path, path);
  FILE *output = grn_fopen(path, "ab");
  if (output) {
    fclose(output);
  } else {
    GRN_DEFINE_NAME(obj);
    SERR("[wal][touch][%.*s]%s failed to open file: <%s>",
         name_size, name,
         tag,
         path);
    rc = ctx->rc;
  }

  if (need_lock) {
    grn_io_unlock(ctx, io);
  }

  return rc;
#else
  return GRN_SUCCESS;
#endif
}

bool
grn_wal_exist(grn_ctx *ctx, grn_obj *obj)
{
  grn_io *io = grn_obj_get_io(ctx, obj);
  if (io->path[0] == '\0') {
    return false;
  }

  char wal_path[PATH_MAX];
  grn_wal_generate_path(ctx, io->path, wal_path);
  struct stat s;
  return stat(wal_path, &s) == 0;
}

static grn_rc
grn_wal_remove_raw(grn_ctx *ctx,
                   grn_obj *obj,
                   const char *path,
                   const char *system_tag,
                   const char *tag)
{
  if (path[0] == '\0') {
    return GRN_SUCCESS;
  }

  char wal_path[PATH_MAX];
  grn_wal_generate_path(ctx, path, wal_path);
  struct stat s;
  if (stat(wal_path, &s) != 0) {
    return GRN_SUCCESS;
  }

  if (grn_unlink(wal_path) == 0) {
    if (obj) {
      GRN_DEFINE_NAME(obj);
      GRN_LOG(ctx, GRN_LOG_DEBUG,
              "[wal]%s[%.*s]%s removed: <%s>",
              system_tag,
              name_size, name,
              tag,
              wal_path);
    } else {
      GRN_LOG(ctx, GRN_LOG_DEBUG,
                "[wal]%s%s removed: <%s>",
              system_tag,
              tag,
              wal_path);
    }
    return GRN_SUCCESS;
  } else {
    if (obj) {
      GRN_DEFINE_NAME(obj);
      SERR("[wal]%s[%.*s]%s failed to remove: <%s>",
           system_tag,
           name_size, name,
           tag,
           wal_path);
    } else {
      SERR("[wal]%s%s failed to remove: <%s>",
           system_tag,
           tag,
           wal_path);
    }
    return ctx->rc;
  }
}

grn_rc
grn_wal_remove(grn_ctx *ctx,
               const char* path,
               const char *tag)
{
  return grn_wal_remove_raw(ctx, NULL, path, "[remove]", tag);
}

grn_rc
grn_wal_clear(grn_ctx *ctx,
              grn_obj *obj,
              bool need_lock,
              const char *tag)
{
  if (GRN_CTX_GET_WAL_ROLE(ctx) != GRN_WAL_ROLE_PRIMARY) {
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

  rc = grn_wal_remove_raw(ctx, obj, io->path, "[clear]", tag);

  if (need_lock) {
    grn_io_unlock(ctx, io);
  }

  return rc;
}

struct grn_wal_reader_ {
  grn_obj *obj;
  const char *tag;
  FILE *input;
  grn_wal_key key;
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
  grn_wal_generate_path(ctx, io->path, path);
  FILE *input = grn_fopen(path, "rb");
  if (!input) {
    return NULL;
  }

  grn_wal_reader *reader = GRN_CALLOC(sizeof(grn_wal_reader));
  if (!reader) {
    GRN_DEFINE_NAME(obj);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[wal][reader][open][%.*s]%s failed to allocate grn_wal_reader",
        name_size, name,
        tag);
    return NULL;
  }

  if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
    GRN_DEFINE_NAME(obj);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[wal][reader][open][%.*s]%s <%s>",
            name_size, name,
            tag,
            path);
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
  if (GRN_CTX_GET_WAL_ROLE(ctx) != GRN_WAL_ROLE_PRIMARY) {
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

#ifdef GRN_WITH_MESSAGE_PACK
static void
grn_wal_reader_read_data(grn_ctx *ctx,
                         grn_wal_reader *reader,
                         grn_wal_reader_data *data,
                         msgpack_object *value,
                         const char *tag)
{
  switch (value->type) {
  case MSGPACK_OBJECT_BOOLEAN :
    data->type = GRN_WAL_READER_DATA_BOOLEAN;
    data->content.boolean = value->via.boolean;
    break;
  case MSGPACK_OBJECT_POSITIVE_INTEGER :
    data->type = GRN_WAL_READER_DATA_UINT64;
    data->content.uint64 = value->via.u64;
    break;
  case MSGPACK_OBJECT_NEGATIVE_INTEGER :
    data->type = GRN_WAL_READER_DATA_INT64;
    data->content.int64 = value->via.i64;
    break;
  case MSGPACK_OBJECT_FLOAT32 :
    data->type = GRN_WAL_READER_DATA_FLOAT32;
    data->content.float32 = value->via.f64;
    break;
  case MSGPACK_OBJECT_FLOAT64 :
    data->type = GRN_WAL_READER_DATA_FLOAT64;
    data->content.float64 = value->via.f64;
    break;
  case MSGPACK_OBJECT_STR :
    data->type = GRN_WAL_READER_DATA_STRING;
    data->content.string.data = value->via.str.ptr;
    data->content.string.size = value->via.str.size;
    break;
  case MSGPACK_OBJECT_BIN :
    data->type = GRN_WAL_READER_DATA_BINARY;
    data->content.binary.data = value->via.bin.ptr;
    data->content.binary.size = value->via.bin.size;
    break;
  default :
    grn_obj_set_error(ctx,
                      reader->obj,
                      GRN_INVALID_ARGUMENT,
                      GRN_ID_NIL,
                      tag,
                      "unsupported MessagePack type: <%s>(%u)",
                      grn_msgpack_object_type_to_string(value->type),
                      value->type);
    break;
  }
}
#endif

grn_rc
grn_wal_reader_read_entry(grn_ctx *ctx,
                          grn_wal_reader *reader,
                          grn_wal_reader_entry *entry)
{
  if (!reader) {
    return GRN_END_OF_DATA;
  }

#ifdef GRN_WITH_MESSAGE_PACK
  if (!grn_wal_reader_read_next(ctx, reader)) {
    return GRN_END_OF_DATA;
  }

  grn_obj tag_buffer;
  GRN_TEXT_INIT(&tag_buffer, 0);
  GRN_TEXT_PUTS(ctx, &tag_buffer, "[reader][read-entry]");
  GRN_TEXT_PUTS(ctx, &tag_buffer, reader->tag);
  GRN_TEXT_PUTC(ctx, &tag_buffer, '\0');
  const char *tag = GRN_TEXT_VALUE(&tag_buffer);
  if (reader->unpacked.data.type != MSGPACK_OBJECT_MAP) {
    grn_obj_set_error(
      ctx,
      reader->obj,
      GRN_FILE_CORRUPT,
      GRN_ID_NIL,
      tag,
      "must be map: <%s>(%u)",
      grn_msgpack_object_type_to_string(reader->unpacked.data.type),
      reader->unpacked.data.type);
    goto exit;
  }

  msgpack_object_map *map = &(reader->unpacked.data.via.map);
  uint32_t i;
  grn_obj dump_buffer;
  GRN_TEXT_INIT(&dump_buffer, 0);
  bool need_log = grn_logger_pass(ctx, GRN_LOG_DEBUG);
  for (i = 0; i < map->size; i++) {
    msgpack_object_kv *kv = &(map->ptr[i]);
    if (kv->key.type != MSGPACK_OBJECT_POSITIVE_INTEGER) {
      continue;
    }
    grn_wal_key key = kv->key.via.u64;
    if (need_log) {
      if (GRN_TEXT_LEN(&dump_buffer) > 0) {
        GRN_TEXT_PUTS(ctx, &dump_buffer, ", ");
      }
      grn_text_printf(ctx,
                      &dump_buffer,
                      "%s(%d):",
                      grn_wal_key_to_string(key),
                      key);
    }
    msgpack_object *value = &(kv->val);
    switch (key) {
    case GRN_WAL_KEY_ID :
      entry->id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%" GRN_FMT_INT64U, entry->id);
      }
      break;
    case GRN_WAL_KEY_EVENT :
      entry->event = value->via.u64;
      if (need_log) {
        GRN_TEXT_PUTS(ctx, &dump_buffer, grn_wal_event_to_string(entry->event));
        grn_text_printf(ctx, &dump_buffer, "(%d)", entry->event);
      }
      break;
    case GRN_WAL_KEY_RECORD_ID :
      entry->record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->record_id);
      }
      break;
    case GRN_WAL_KEY_RECORD_DIRECTION :
      entry->record_direction = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->record_direction);
      }
      break;
    case GRN_WAL_KEY_ELEMENT_SIZE :
      entry->element_size = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->element_size);
      }
      break;
    case GRN_WAL_KEY_KEY :
      grn_wal_reader_read_data(ctx, reader, &(entry->key), value, tag);
      if (need_log) {
        GRN_TEXT_PUTS(ctx,
                      &dump_buffer,
                      grn_wal_reader_data_type_to_string(entry->key.type));
      }
      break;
    case GRN_WAL_KEY_KEY_SIZE :
      entry->key.content.uint64 = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%" GRN_FMT_INT64U,
                        entry->key.content.uint64);
      }
      break;
    case GRN_WAL_KEY_KEY_HASH_VALUE :
      entry->key_hash_value = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->key_hash_value);
      }
      break;
    case GRN_WAL_KEY_KEY_OFFSET :
      entry->key_offset = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%" GRN_FMT_INT64U,
                        entry->key_offset);
      }
      break;
    case GRN_WAL_KEY_SHARED_KEY_OFFSET :
      entry->shared_key_offset = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%" GRN_FMT_INT64U,
                        entry->shared_key_offset);
      }
      break;
    case GRN_WAL_KEY_IS_SHARED :
      entry->is_shared = value->via.boolean;
      if (need_log) {
        GRN_TEXT_PUTS(ctx, &dump_buffer, entry->is_shared ? "true" : "false");
      }
      break;
    case GRN_WAL_KEY_CHECK :
      entry->check = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->check);
      }
      break;
    case GRN_WAL_KEY_NEW_KEY :
      grn_wal_reader_read_data(ctx, reader, &(entry->new_key), value, tag);
      if (need_log) {
        GRN_TEXT_PUTS(ctx,
                      &dump_buffer,
                      grn_wal_reader_data_type_to_string(entry->new_key.type));
      }
      break;
    case GRN_WAL_KEY_PARENT_RECORD_ID :
      entry->parent_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->parent_record_id);
      }
      break;
    case GRN_WAL_KEY_PARENT_RECORD_DIRECTION :
      entry->parent_record_direction = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->parent_record_direction);
      }
      break;
    case GRN_WAL_KEY_PARENT_CHECK :
      entry->parent_check = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->parent_check);
      }
      break;
    case GRN_WAL_KEY_GRANDPARENT_RECORD_ID :
      entry->grandparent_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->grandparent_record_id);
      }
      break;
    case GRN_WAL_KEY_OTHERSIDE_RECORD_ID :
      entry->otherside_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->otherside_record_id);
      }
      break;
    case GRN_WAL_KEY_OTHERSIDE_CHECK :
      entry->otherside_check = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->otherside_check);
      }
      break;
    case GRN_WAL_KEY_LEFT_RECORD_ID :
      entry->left_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->left_record_id);
      }
      break;
    case GRN_WAL_KEY_RIGHT_RECORD_ID :
      entry->right_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->right_record_id);
      }
      break;
    case GRN_WAL_KEY_VALUE :
      grn_wal_reader_read_data(ctx, reader, &(entry->value), value, tag);
      if (need_log) {
        GRN_TEXT_PUTS(ctx,
                      &dump_buffer,
                      grn_wal_reader_data_type_to_string(entry->value.type));
      }
      break;
    case GRN_WAL_KEY_INDEX_HASH_VALUE :
      entry->index_hash_value = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->index_hash_value);
      }
      break;
    case GRN_WAL_KEY_SEGMENT :
      entry->segment = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->segment);
      }
      break;
    case GRN_WAL_KEY_POSITION :
      entry->position = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->position);
      }
      break;
    case GRN_WAL_KEY_SEGMENT_TYPE :
      entry->segment_type = value->via.u64;
      if (need_log) {
        GRN_TEXT_PUTS(ctx,
                      &dump_buffer,
                      grn_wal_segment_type_to_string(entry->segment_type));
      }
      break;
    case GRN_WAL_KEY_SEGMENT_INFO :
      entry->segment_info = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->segment_info);
      }
      break;
    case GRN_WAL_KEY_GARBAGE_SEGMENT :
      entry->garbage_segment = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->garbage_segment);
      }
      break;
    case GRN_WAL_KEY_PREVIOUS_GARBAGE_SEGMENT :
      entry->previous_garbage_segment = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->previous_garbage_segment);
      }
      break;
    case GRN_WAL_KEY_NEXT_GARBAGE_SEGMENT :
      entry->next_garbage_segment = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->next_garbage_segment);
      }
      break;
    case GRN_WAL_KEY_GARBAGE_SEGMENT_HEAD :
      entry->garbage_segment_head = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->garbage_segment_head);
      }
      break;
    case GRN_WAL_KEY_GARBAGE_SEGMENT_TAIL :
      entry->garbage_segment_tail = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->garbage_segment_tail);
      }
      break;
    case GRN_WAL_KEY_GARBAGE_SEGMENT_N_RECORDS :
      entry->garbage_segment_n_records = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->garbage_segment_n_records);
      }
      break;
    case GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID :
      entry->next_garbage_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->next_garbage_record_id);
      }
      break;
    case GRN_WAL_KEY_N_GARBAGES :
      entry->n_garbages = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->n_garbages);
      }
      break;
    case GRN_WAL_KEY_N_ENTRIES :
      entry->n_entries = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->n_entries);
      }
      break;
    case GRN_WAL_KEY_FOUND_GARBAGE :
      entry->found_garbage = value->via.boolean;
      if (need_log) {
        GRN_TEXT_PUTS(ctx,
                      &dump_buffer,
                      entry->found_garbage ? "true" : "false");
      }
      break;
    case GRN_WAL_KEY_MAX_OFFSET :
      entry->max_offset = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->max_offset);
      }
      break;
    case GRN_WAL_KEY_EXPECTED_N_ENTRIES :
      entry->expected_n_entries = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->expected_n_entries);
      }
      break;
    case GRN_WAL_KEY_DELETE_INFO_INDEX :
      entry->delete_info_index = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->delete_info_index);
      }
      break;
    case GRN_WAL_KEY_DELETE_INFO_PHASE1_INDEX :
      entry->delete_info_phase1_index = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->delete_info_phase1_index);
      }
      break;
    case GRN_WAL_KEY_DELETE_INFO_PHASE2_INDEX :
      entry->delete_info_phase2_index = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->delete_info_phase2_index);
      }
      break;
    case GRN_WAL_KEY_DELETE_INFO_PHASE3_INDEX :
      entry->delete_info_phase3_index = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx,
                        &dump_buffer,
                        "%u",
                        entry->delete_info_phase3_index);
      }
      break;
    case GRN_WAL_KEY_PREVIOUS_RECORD_ID :
      entry->previous_record_id = value->via.u64;
      if (need_log) {
        grn_text_printf(ctx, &dump_buffer, "%u", entry->previous_record_id);
      }
      break;
    default :
      grn_obj_set_error(ctx,
                        reader->obj,
                        GRN_INVALID_ARGUMENT,
                        GRN_ID_NIL,
                        tag,
                        "unsupported key: <%s>(%u)",
                        grn_wal_key_to_string(key),
                        key);
      break;
    }
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  if (need_log) {
    GRN_LOG(ctx, GRN_LOG_DEBUG,
            "[wal][reader][read][entry] %.*s",
            (int)GRN_TEXT_LEN(&dump_buffer),
            GRN_TEXT_VALUE(&dump_buffer));
  }
  GRN_OBJ_FIN(ctx, &dump_buffer);
exit :
  GRN_OBJ_FIN(ctx, &tag_buffer);
#endif
  return ctx->rc;
}

void
grn_wal_set_recover_error(grn_ctx *ctx,
                          grn_rc rc,
                          grn_obj *object,
                          grn_wal_reader_entry *entry,
                          const char *tag,
                          const char *message)
{
  GRN_DEFINE_NAME(object);
  grn_id object_id = grn_obj_id(ctx, object);
  char path[PATH_MAX];
  bool have_wal_file = false;
  grn_io *io = grn_obj_get_io(ctx, object);
  if (io) {
    grn_wal_generate_path(ctx, io->path, path);
    struct stat s;
    have_wal_file = (stat(path, &s) == 0);
  }
  if (!have_wal_file) {
    grn_strcpy(path, sizeof(path), "(nonexistent)");
  }
  ERR(rc,
      "[wal][recover][error]%s[%.*s(%u)] %s: "
      "id:%" GRN_FMT_INT64U " "
      "event:%s(%u) "
      "record-id:%u "
      "element-size:%u "
      "key-type:%s(%u) "
      "key-hash-value:%u "
      "key-offset:%" GRN_FMT_INT64U " "
      "shared-key-offset:%" GRN_FMT_INT64U " "
      "is-shared:%s "
      "check:%u "
      "new-key-type:%s(%u) "
      "parent-record-id:%u "
      "parent-record-direction:%u "
      "parent-check:%u "
      "grandparent-record-id:%u "
      "otherside-record-id:%u "
      "otherside-check:%u "
      "left-record-id:%u "
      "right-record-id:%u "
      "value-type:%s(%u) "
      "index-hash-value:%u "
      "segment:%u "
      "position:%u "
      "segment-type:%s(%u) "
      "segment-info:<%s|%u>(%u) "
      "garbage-segment:%u "
      "garbage-segment-tail:%u "
      "garbage-segment-n-records:%u "
      "previous-garbage-segment:%u "
      "next-garbage-segment:%u "
      "n-garbages:%u "
      "n-entries:%u "
      "max-offset:%u "
      "expected-n-entries:%u "
      "delete-info-index:%u "
      "delete-info-phase1-index:%u "
      "delete-info-phase2-index:%u "
      "delete-info-phase3-index:%u "
      "previous-record-id:%u "
      "path:<%s>",
      tag,
      name_size, name,
      object_id,
      message,
      entry->id,
      grn_wal_event_to_string(entry->event),
      entry->event,
      entry->record_id,
      entry->element_size,
      grn_wal_reader_data_type_to_string(entry->key.type),
      entry->key.type,
      entry->key_hash_value,
      entry->key_offset,
      entry->shared_key_offset,
      entry->is_shared ? "true" : "false",
      entry->check,
      grn_wal_reader_data_type_to_string(entry->new_key.type),
      entry->new_key.type,
      entry->parent_record_id,
      entry->parent_record_direction,
      entry->parent_check,
      entry->grandparent_record_id,
      entry->otherside_record_id,
      entry->otherside_check,
      entry->left_record_id,
      entry->right_record_id,
      grn_wal_reader_data_type_to_string(entry->value.type),
      entry->value.type,
      entry->index_hash_value,
      entry->segment,
      entry->position,
      grn_wal_segment_type_to_string(entry->segment_type),
      entry->segment_type,
      grn_ja_segment_info_type_name(ctx, entry->segment_info),
      grn_ja_segment_info_value(ctx, entry->segment_info),
      entry->segment_info,
      entry->garbage_segment,
      entry->garbage_segment_tail,
      entry->garbage_segment_n_records,
      entry->previous_garbage_segment,
      entry->next_garbage_segment,
      entry->n_garbages,
      entry->n_entries,
      entry->max_offset,
      entry->expected_n_entries,
      entry->delete_info_index,
      entry->delete_info_phase1_index,
      entry->delete_info_phase2_index,
      entry->delete_info_phase3_index,
      entry->previous_record_id,
      path);
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
      grn_wal_key key = kv->key.via.u64;
      printf("%u:%s(%u):%s(%u):",
             j,
             grn_wal_key_to_string(key),
             key,
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
        switch (key) {
        case GRN_WAL_KEY_EVENT :
          {
            grn_wal_event event = kv->val.via.u64;
            printf("event(%s)<%d>",
                   grn_wal_event_to_string(event),
                   event);
          }
          break;
        case GRN_WAL_KEY_SEGMENT_TYPE :
          {
            grn_wal_segment_type type = kv->val.via.u64;
            printf("segment-type(%s)<%d>",
                   grn_wal_segment_type_to_string(type),
                   type);
          }
          break;
        case GRN_WAL_KEY_SEGMENT_INFO :
          {
            uint32_t info = kv->val.via.u64;
            printf("segment-info(%s)(%u)<%u>",
                   grn_ja_segment_info_type_name(ctx, info),
                   grn_ja_segment_info_value(ctx, info),
                   info);
          }
          break;
        default :
          printf("%" GRN_FMT_INT64U, kv->val.via.u64);
          break;
        }
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
