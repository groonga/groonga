/*
  Copyright (C) 2015-2019  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga.h>

int
main(int argc, char **argv)
{
  grn_rc rc;
  grn_ctx ctx;
  int n = 10000000;

  rc = grn_init();
  if (rc != GRN_SUCCESS) {
    printf("failed to initialize Groonga: <%d>: %s\n",
           rc, grn_get_global_error_message());
    return EXIT_FAILURE;
  }

  grn_ctx_init(&ctx, 0);
  grn_obj *db = grn_db_open(&ctx, "db/db");
  if (ctx.rc != GRN_SUCCESS) {
    printf("failed to open database: <%d>: %s\n",
           rc, grn_get_global_error_message());
    return EXIT_FAILURE;
  }

  grn_obj *source_table = grn_ctx_get(&ctx, "Sources", -1);
  grn_obj *result_set = grn_table_create(&ctx,
                                         NULL, 0, NULL,
                                         GRN_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
                                         source_table,
                                         NULL);
  grn_timeval start;
  grn_timeval_now(&ctx, &start);
  for (int i = 0; i < n; i++) {
    grn_id id = i;
    grn_hash_add(&ctx, (grn_hash *)result_set, &id, sizeof(grn_id), NULL, NULL);
  }
  grn_timeval end;
  grn_timeval_now(&ctx, &end);
  double elapsed =
    (end.tv_sec + (end.tv_nsec / GRN_TIME_NSEC_PER_SEC_F)) -
    (start.tv_sec + (start.tv_nsec / GRN_TIME_NSEC_PER_SEC_F));
  printf("%f:%d\n", elapsed, grn_table_size(&ctx, result_set));
  grn_obj_close(&ctx, result_set);
  grn_obj_close(&ctx, db);
  grn_ctx_fin(&ctx);

  grn_fin();

  return EXIT_SUCCESS;
}
