#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <groonga.h>

grn_ctx ctx;
grn_obj *db;

#ifdef SEQUENTIAL
#define GENKEY(i) (i)
#else /* SEQUENTIAL */
#define GENKEY(i) (rand())
#endif /* SEQUENTIAL */

int nloops = 1000000;
unsigned key_size = 8;
unsigned value_size = 8;

#define EVAL(ctx, exp) (grn_ql_send(ctx, (exp), strlen(exp), 0))

int
ql_put(void)
{
  int i, key;
  grn_obj buf;
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  EVAL(&ctx, "(ptable '<t1>)");
  EVAL(&ctx, "(<t1> ::def :c1 <text>)");
  EVAL(&ctx, "(<t1> ::load :c1)");
  for (i = 0; i < nloops; i++) {
    key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    GRN_BULK_PUTC(&ctx, &buf, '\t');
    grn_bulk_itoh(&ctx, &buf, key, value_size);
    GRN_BULK_PUTC(&ctx, &buf, '\0');
    EVAL(&ctx, GRN_BULK_HEAD(&buf));
  }
  grn_obj_close(&ctx, &buf);
  return 0;
}

int
ql_get(void)
{
  int i, key;
  grn_obj buf;
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    GRN_BULK_PUTS(&ctx, &buf, "(<t1> : \"");
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    GRN_BULK_PUTS(&ctx, &buf, "\").c1");
    GRN_BULK_PUTC(&ctx, &buf, '\0');
    EVAL(&ctx, GRN_BULK_HEAD(&buf));
  }
  grn_obj_close(&ctx, &buf);
  return 0;
}

int
column_put(void)
{
  int i, s = 0;
  grn_obj buf;
  grn_obj *key_type = grn_ctx_get(&ctx, GRN_DB_SHORTTEXT);
  grn_obj *table = grn_table_create(&ctx, "<t1>", 4, NULL,
                                    GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_PERSISTENT,
                                    key_type, 0);
  grn_obj *value_type = grn_ctx_get(&ctx, GRN_DB_TEXT);
  grn_obj *column = grn_column_create(&ctx, table, "c1", 2, NULL,
                                      GRN_OBJ_PERSISTENT, value_type);
  if (!table || !column) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      grn_search_flags flags = GRN_TABLE_ADD;
      grn_id rid = grn_table_lookup(&ctx, table,
                                    GRN_BULK_HEAD(&buf), key_size, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        unsigned int v = key % value_size;
        GRN_BULK_REWIND(&buf);
        if (v) {
          grn_bulk_space(&ctx, &buf, v -1);
          GRN_BULK_PUTC(&ctx, &buf, GRN_BULK_HEAD(&buf)[0]);
          s += v;
        }
        if (grn_obj_set_value(&ctx, column, rid, &buf, GRN_OBJ_SET)) {
          fprintf(stderr, "grn_obj_set_value failed");
        }
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  printf("total size: %d\n", s);
  return 0;
}

int
column_get(void)
{
  int i, s = 0;
  grn_obj buf;
  grn_obj *table = grn_ctx_lookup(&ctx, "<t1>", 4);
  grn_obj *column = grn_ctx_lookup(&ctx, "<t1>.c1", 7);
  if (!table || !column) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      grn_search_flags flags = 0;
      grn_id rid = grn_table_lookup(&ctx, table,
                                    GRN_BULK_HEAD(&buf), key_size, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed\n");
      } else {
        grn_obj obj, *p;
        unsigned int v = key % value_size;
        GRN_OBJ_INIT(&obj, GRN_BULK, 0);
        p = grn_obj_get_value(&ctx, column, rid, &obj);
        if (!p) {
          fprintf(stderr, "grn_obj_get_value failed\n");
        } else {
          if (GRN_BULK_VSIZE(p) != v) {
            fprintf(stderr, "value_size unmatch %d (%ld:%u)\n", i, GRN_BULK_VSIZE(p), v);
          } else {
            if (v && GRN_BULK_HEAD(p)[v-1] != GRN_BULK_HEAD(&buf)[0]) {
              fprintf(stderr, "value unmatch\n");
            } else {
              s++;
            }
          }
          grn_obj_close(&ctx, p);
        }
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  if (i != s) { printf("successed: %d\n", s); }
  return 0;
}

int
table_put(void)
{
  int i;
  grn_obj buf;
  grn_obj *key_type = grn_ctx_get(&ctx, GRN_DB_SHORTTEXT);
  grn_obj *table = grn_table_create(&ctx, "<t1>", 4, NULL,
                                    GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_PERSISTENT,
                                    key_type, value_size);
  if (!table) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      grn_search_flags flags = GRN_TABLE_ADD;
      grn_id rid = grn_table_lookup(&ctx, table,
                                    GRN_BULK_HEAD(&buf), key_size, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        if (grn_obj_set_value(&ctx, table, rid, &buf, GRN_OBJ_SET)) {
          fprintf(stderr, "grn_obj_set_value failed");
        }
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  return 0;
}

int
table_get(void)
{
  int i;
  grn_obj buf;
  grn_obj *table = grn_ctx_lookup(&ctx, "<t1>", 4);
  if (!table) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      grn_search_flags flags = 0;
      grn_id rid = grn_table_lookup(&ctx, table,
                                    GRN_BULK_HEAD(&buf), key_size, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        grn_obj obj, *p;
        GRN_OBJ_INIT(&obj, GRN_BULK, 0);
        p = grn_obj_get_value(&ctx, table, rid, &obj);
        if (!p) {
          fprintf(stderr, "grn_obj_get_value failed\n");
        } else {
          if (memcmp(GRN_BULK_HEAD(p), GRN_BULK_HEAD(&buf), value_size)) {
            fprintf(stderr, "value unmatch\n");
          }
          grn_obj_close(&ctx, p);
        }
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  return 0;
}

int
hash_put(const char *path)
{
  int i;
  grn_obj buf;
  grn_hash *hash = grn_hash_create(&ctx, path, key_size, value_size,
                                   GRN_OBJ_PERSISTENT|GRN_OBJ_KEY_VAR_SIZE);
  if (!hash) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      void *value;
      grn_search_flags flags = GRN_TABLE_ADD;
      grn_id rid = grn_hash_lookup(&ctx, hash,
                                   GRN_BULK_HEAD(&buf), key_size, &value, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        memcpy(value, GRN_BULK_HEAD(&buf), value_size);
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  grn_hash_close(&ctx, hash);
  return 0;
}

int
hash_get(const char *path)
{
  int i;
  grn_obj buf;
  grn_hash *hash = grn_hash_open(&ctx, path);
  if (!hash) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      void *value;
      grn_search_flags flags = 0;
      grn_id rid = grn_hash_lookup(&ctx, hash,
                                   GRN_BULK_HEAD(&buf), key_size, &value, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        if (memcmp(value, GRN_BULK_HEAD(&buf), value_size)) {
          fprintf(stderr, "value unmatch %d: %d != %d\n", i, *((int *)value), key);
        }
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  grn_hash_close(&ctx, hash);
  return 0;
}

int
pat_put(const char *path)
{
  int i;
  grn_obj buf;
  grn_pat *pat = grn_pat_create(&ctx, path, key_size, value_size,
                                   GRN_OBJ_PERSISTENT|GRN_OBJ_KEY_VAR_SIZE);
  if (!pat) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      void *value;
      grn_search_flags flags = GRN_TABLE_ADD;
      grn_id rid = grn_pat_lookup(&ctx, pat,
                                   GRN_BULK_HEAD(&buf), key_size, &value, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        memcpy(value, GRN_BULK_HEAD(&buf), value_size);
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  grn_pat_close(&ctx, pat);
  return 0;
}

int
pat_get(const char *path)
{
  int i;
  grn_obj buf;
  grn_pat *pat = grn_pat_open(&ctx, path);
  if (!pat) { return -1; }
  GRN_OBJ_INIT(&buf, GRN_BULK, 0);
  for (i = 0; i < nloops; i++) {
    int key = GENKEY(i);
    GRN_BULK_REWIND(&buf);
    grn_bulk_itoh(&ctx, &buf, key, key_size);
    {
      void *value;
      grn_search_flags flags = 0;
      grn_id rid = grn_pat_lookup(&ctx, pat,
                                   GRN_BULK_HEAD(&buf), key_size, &value, &flags);
      if (!rid) {
        fprintf(stderr, "table_lookup failed");
      } else {
        if (memcmp(value, GRN_BULK_HEAD(&buf), value_size)) {
          fprintf(stderr, "value unmatch %d: %d != %d\n", i, *((int *)value), key);
        }
      }
    }
  }
  grn_obj_close(&ctx, &buf);
  grn_pat_close(&ctx, pat);
  return 0;
}

int
main(int argc, char **argv)
{
  int r, op = 'p', method = 'c';
  const char *path;
  if (argc < 2) {
    fprintf(stderr, "usage: kv dbpath [put|get] [col|table|hash|pat|ql] [value_size]\n");
    return -1;
  }
  path = *argv[1] ? argv[1] : NULL;
  if (argc > 2) { op = *argv[2]; }
  if (argc > 3) { method = *argv[3]; }
  if (argc > 4) { value_size = atoi(argv[4]); }
  if (argc > 5) { nloops = atoi(argv[5]); }
  if (grn_init()) {
    fprintf(stderr, "grn_init() failed\n");
    return -1;
  }
  if (grn_ctx_init(&ctx, (method == 'q' ? GRN_CTX_USE_QL|GRN_CTX_BATCH_MODE : 0))) {
    fprintf(stderr, "grn_ctx_init() failed\n");
    return -1;
  }
  srand(0);
  if (method == 'h' || method == 'p') {
    switch (method) {
    case 'h' :
      r = (op == 'p') ? hash_put(path) : hash_get(path);
      break;
    case 'p' :
      r = (op == 'p') ? pat_put(path) : pat_get(path);
      break;
    default :
      r = -1;
      fprintf(stderr, "invalid method\n");
      break;
    }
  } else {
    if (path) { db = grn_db_open(&ctx, path); }
    if (!db) { db = grn_db_create(&ctx, path, NULL); }
    if (!db) {
      fprintf(stderr, "db initialize failed\n");
      return -1;
    }
    switch (method) {
    case 'q' :
      r = (op == 'p') ? ql_put() : ql_get();
      break;
    case 'c' :
      r = (op == 'p') ? column_put() : column_get();
      break;
    case 't' :
      r = (op == 'p') ? table_put() : table_get();
      break;
    default :
      r = -1;
      fprintf(stderr, "invalid method\n");
      break;
    }
    if (grn_obj_close(&ctx, db)) {
      fprintf(stderr, "grn_obj_close() failed\n");
      return -1;
    }
  }
  if (grn_ctx_fin(&ctx)) {
    fprintf(stderr, "grn_ctx_fin() failed\n");
    return -1;
  }
  if (grn_fin()) {
    fprintf(stderr, "grn_fin() failed\n");
    return -1;
  }
  return r;
}
