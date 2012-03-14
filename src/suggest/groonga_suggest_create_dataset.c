/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010- Brazil

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
#include <string.h>
#include <unistd.h>
#include <groonga.h>

static void
usage(FILE *output, int argc, char **argv)
{
  fprintf(output, "Usage: %s DB_PATH DATASET_NAME\n", argv[0]);
  fprintf(output, " e.g.: %s /tmp/db shops\n", argv[0]);
}

static void
output(grn_ctx *ctx)
{
  int flags = 0;
  char *str;
  unsigned int str_len;

  do {
    grn_ctx_recv(ctx, &str, &str_len, &flags);
    if (str_len > 0 || ctx->rc) {
      if (ctx->rc) {
        printf("ERROR (%d): %s\n", ctx->rc, ctx->errbuf);
      }
      if (str_len > 0) {
        printf("%.*s\n", str_len, str);
      }
    }
  } while (flags & GRN_CTX_MORE);
}

static void
send_command(grn_ctx *ctx, grn_obj *buffer, const char *command,
             const char *dataset_name)
{
  const char *p = command;
  const char *dataset_place_holder = "${DATASET}";
  char *dataset_place_holder_position;

  if (ctx->rc != GRN_SUCCESS) {
    return;
  }

  GRN_BULK_REWIND(buffer);
  while ((dataset_place_holder_position = strstr(p, dataset_place_holder))) {
    GRN_TEXT_PUT(ctx, buffer, p, dataset_place_holder_position - p);
    GRN_TEXT_PUTS(ctx, buffer, dataset_name);
    p = dataset_place_holder_position + strlen(dataset_place_holder);
  }
  GRN_TEXT_PUTS(ctx, buffer, p);
  printf("> %.*s\n", (int)GRN_TEXT_LEN(buffer), GRN_TEXT_VALUE(buffer));
  grn_ctx_send(ctx, GRN_TEXT_VALUE(buffer), GRN_TEXT_LEN(buffer), 0);
  output(ctx);
}


int
main(int argc, char **argv)
{
  const char *db_path;
  const char *dataset_name;
  grn_ctx ctx_, *ctx;
  grn_obj *db;
  grn_bool success = GRN_TRUE;

  if (argc != 3) {
    usage(stderr, argc, argv);
    return(EXIT_FAILURE);
  }

  db_path = argv[1];
  dataset_name = argv[2];

  grn_init();

  ctx = &ctx_;
  grn_ctx_init(ctx, 0);
  if (access(db_path, F_OK) == 0) {
    db = grn_db_open(ctx, db_path);
    if (!db) {
      fprintf(stderr, "DB open failed (%s): %s\n", db_path, ctx->errbuf);
    }
  } else {
    db = grn_db_create(ctx, db_path, NULL);
    if (!db) {
      fprintf(stderr, "DB create failed (%s): %s\n", db_path, ctx->errbuf);
    }
  }

  if (db) {
    grn_obj text;
    GRN_TEXT_INIT(&text, 0);
#define SEND(string) send_command(ctx, &text, string, dataset_name)
    SEND("register suggest/suggest");
    SEND("table_create event_type TABLE_HASH_KEY ShortText");
    SEND("table_create bigram TABLE_PAT_KEY|KEY_NORMALIZE ShortText "
         "--default_tokenizer TokenBigram");
    SEND("table_create kana TABLE_PAT_KEY|KEY_NORMALIZE ShortText");
    SEND("table_create item_${DATASET} TABLE_PAT_KEY|KEY_NORMALIZE "
         "ShortText --default_tokenizer TokenDelimit");
    SEND("column_create bigram item_${DATASET}_key "
         "COLUMN_INDEX|WITH_POSITION item_${DATASET} _key");
    SEND("column_create item_${DATASET} kana COLUMN_VECTOR kana");
    SEND("column_create kana item_${DATASET}_kana COLUMN_INDEX "
         "item_${DATASET} kana");
    SEND("column_create item_${DATASET} freq COLUMN_SCALAR Int32");
    SEND("column_create item_${DATASET} last COLUMN_SCALAR Time");
    SEND("column_create item_${DATASET} boost COLUMN_SCALAR Int32");
    SEND("column_create item_${DATASET} freq2 COLUMN_SCALAR Int32");
    SEND("column_create item_${DATASET} buzz COLUMN_SCALAR Int32");

    SEND("table_create pair_${DATASET} TABLE_HASH_KEY UInt64");
    SEND("column_create pair_${DATASET} pre COLUMN_SCALAR item_${DATASET}");
    SEND("column_create pair_${DATASET} post COLUMN_SCALAR item_${DATASET}");
    SEND("column_create pair_${DATASET} freq0 COLUMN_SCALAR Int32");
    SEND("column_create pair_${DATASET} freq1 COLUMN_SCALAR Int32");
    SEND("column_create pair_${DATASET} freq2 COLUMN_SCALAR Int32");
    SEND("column_create item_${DATASET} co COLUMN_INDEX pair_${DATASET} pre");

    SEND("table_create sequence_${DATASET} TABLE_HASH_KEY ShortText");
    SEND("table_create event_${DATASET} TABLE_NO_KEY");
    SEND("column_create sequence_${DATASET} events "
         "COLUMN_VECTOR|RING_BUFFER event_${DATASET}");
    SEND("column_create event_${DATASET} type COLUMN_SCALAR event_type");
    SEND("column_create event_${DATASET} time COLUMN_SCALAR Time");
    SEND("column_create event_${DATASET} item COLUMN_SCALAR item_${DATASET}");
    SEND("column_create event_${DATASET} sequence COLUMN_SCALAR "
         "sequence_${DATASET}");
#undef SEND
    success = ctx->rc == GRN_SUCCESS;
    GRN_OBJ_FIN(ctx, &text);
    GRN_OBJ_FIN(ctx, db);
  } else {
    success = GRN_FALSE;
  }
  grn_ctx_fin(ctx);
  grn_fin();

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
