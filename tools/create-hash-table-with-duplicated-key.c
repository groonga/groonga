/*
  Copyright (C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include <stdio.h>
#include <stdlib.h>

#include "../lib/grn_io.h"
#include "../lib/grn_obj.h"

/* These implementations must be synced with the implementations in
 * lib/hash.c. */
#define GRN_HASH_ENTRY_SEGMENT 1

grn_inline static void *
grn_io_array_at_inline(grn_ctx *ctx, grn_io *io, uint32_t segment_id,
                       uint64_t offset, int flags)
{
  return grn_io_array_at(ctx, io, segment_id, offset, &flags);
}

typedef struct {
  uint32_t hash_value;
  uint16_t flag;
  uint16_t key_size;
  union {
    uint8_t buf[sizeof(uint32_t)];
    uint32_t offset;
  } key;
  uint8_t value[1];
} grn_io_hash_entry_normal;

int
main(int argc, char **argv)
{
  grn_rc rc = grn_init();
  if (rc != GRN_SUCCESS) {
    printf("failed to initialize Groonga: <%d>: %s\n",
           rc, grn_get_global_error_message());
    return EXIT_FAILURE;
  }

  int exit_code = EXIT_SUCCESS;
  grn_ctx ctx;
  grn_ctx_init(&ctx, 0);
  if (ctx.rc != GRN_SUCCESS) {
    goto exit;
  }

  grn_db_create(&ctx, "db", NULL);
  if (ctx.rc != GRN_SUCCESS) {
    goto exit;
  }

  const char *table_name = "TableWithDuplicatedKey";
  grn_obj *table = grn_table_create(&ctx,
                                    table_name, strlen(table_name),
                                    NULL,
                                    GRN_TABLE_HASH_KEY | GRN_OBJ_PERSISTENT,
                                    grn_ctx_at(&ctx, GRN_DB_SHORT_TEXT),
                                    NULL);
  if (ctx.rc != GRN_SUCCESS) {
    goto exit;
  }

  const char *key = "a";
  grn_table_add(&ctx, table, key, strlen(key), NULL);
  if (ctx.rc != GRN_SUCCESS) {
    goto exit;
  }
  grn_id id_for_duplicated_entry = grn_table_add(&ctx, table, "b", 1, NULL);
  if (ctx.rc != GRN_SUCCESS) {
    goto exit;
  }
  grn_io_hash_entry_normal *entry =
    grn_io_array_at_inline(&ctx,
                           grn_obj_get_io(&ctx, table),
                           GRN_HASH_ENTRY_SEGMENT,
                           id_for_duplicated_entry,
                           GRN_TABLE_ADD);
  grn_memcpy(entry->key.buf, key, strlen(key));

exit :
  if (ctx.rc != GRN_SUCCESS) {
    printf("failed: <%d>: %s\n", ctx.rc, ctx.errbuf);
    exit_code = EXIT_FAILURE;
  }
  grn_ctx_fin(&ctx);

  grn_fin();

  return exit_code;
}
