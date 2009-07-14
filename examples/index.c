#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <groonga.h>

/* grn_index : sen_index like API */

typedef struct {
  grn_obj *db;
  grn_obj *keys;
  grn_obj *lexicon;
  grn_obj *inv;
} grn_index;

# ifndef PATH_MAX
#  if defined(MAXPATHLEN)
#   define PATH_MAX MAXPATHLEN
#  else /* MAXPATHLEN */
#   define PATH_MAX 1024
#  endif /* MAXPATHLEN */
# endif /* PATH_MAX */

grn_index *
grn_index_create(grn_ctx *ctx, const char *path)
{
  grn_obj *db, *keys, *key_type, *lexicon, *inv, *tokenizer;
  if ((db = grn_db_create(ctx, NULL, NULL))) {
    char buffer[PATH_MAX];
    strcpy(buffer, path);
    strcat(buffer, ".SEN");
    if ((key_type = grn_ctx_at(ctx, GRN_DB_SHORT_TEXT))) {
      if ((keys = grn_table_create(ctx, "<keys>", 6, buffer,
                                   GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_PERSISTENT,
                                   key_type, 0))) {
        strcpy(buffer, path);
        strcat(buffer, ".SEN.l");
        if ((lexicon = grn_table_create(ctx, "<lexicon>", 9, buffer,
                                        GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                                        key_type, 0))) {
          if ((tokenizer = grn_ctx_at(ctx, GRN_DB_MECAB))) {
            grn_obj_set_info(ctx, lexicon, GRN_INFO_DEFAULT_TOKENIZER, tokenizer);
            strcpy(buffer, path);
            strcat(buffer, ".SEN.i");
            if ((inv = grn_column_create(ctx, lexicon, "inv", 3, buffer,
                                         GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT,
                                         keys))) {
              grn_index *index;
              if ((index = malloc(sizeof(grn_index)))) {
                index->db = db;
                index->keys = keys;
                index->lexicon = lexicon;
                index->inv = inv;
                return index;
              }
            }
          }
        }
      }
    }
  }
  return NULL;
}

grn_index *
grn_index_open(grn_ctx *ctx, const char *path)
{
  grn_obj *db, *keys, *key_type, *lexicon, *inv, *tokenizer;
  if ((db = grn_db_create(ctx, NULL, NULL))) {
    char buffer[PATH_MAX];
    strcpy(buffer, path);
    strcat(buffer, ".SEN");
    if ((key_type = grn_ctx_at(ctx, GRN_DB_SHORT_TEXT))) {
      if ((keys = grn_table_open(ctx, "<keys>", 6, buffer))) {
        strcpy(buffer, path);
        strcat(buffer, ".SEN.l");
        if ((lexicon = grn_table_open(ctx, "<lexicon>", 9, buffer))) {
          if ((tokenizer = grn_ctx_at(ctx, GRN_DB_MECAB))) {
            grn_obj_set_info(ctx, lexicon, GRN_INFO_DEFAULT_TOKENIZER, tokenizer);
            strcpy(buffer, path);
            strcat(buffer, ".SEN.i");
            if ((inv = grn_column_open(ctx, lexicon, "inv", 3, buffer, keys))) {
              grn_index *index;
              if ((index = malloc(sizeof(grn_index)))) {
                index->db = db;
                index->keys = keys;
                index->lexicon = lexicon;
                index->inv = inv;
                return index;
              }
            }
          }
        }
      }
    }
  }
  return NULL;
}

grn_rc
grn_index_upd(grn_ctx *ctx, grn_index *index, const char *key,
              const char *oldvalue, unsigned int oldvalue_len,
              const char *newvalue, unsigned int newvalue_len)
{
  grn_id rid = grn_table_add(ctx, index->keys, key, strlen(key), NULL);
  if (rid) {
    grn_obj old, new;
    GRN_TEXT_INIT(&old, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_INIT(&new, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET_REF(&old, oldvalue, oldvalue_len);
    GRN_TEXT_SET_REF(&new, newvalue, newvalue_len);
    grn_column_index_update(ctx, index->inv, rid, 1, &old, &new);
    grn_obj_close(ctx, &old);
    grn_obj_close(ctx, &new);
  }
  return ctx->rc;
}

grn_obj *
grn_index_sel(grn_ctx *ctx, grn_index *index,
              const char *string, unsigned int string_len)
{
  grn_obj *res;
  grn_obj query;
  GRN_TEXT_INIT(&query, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET_REF(&query, string, string_len);
  if ((res = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_HASH_KEY,
                              index->keys, 0))) {
    if ((grn_obj_search(ctx, index->inv, &query, res, GRN_OP_OR, NULL))) {
      grn_obj_close(ctx, res);
      res =  NULL;
    }
  }
  return res;
}

grn_rc
grn_index_close(grn_ctx *ctx, grn_index *index)
{
  grn_obj_close(ctx, index->db);
  free(index);
  return ctx->rc;
}


/***************/

void
do_file(grn_ctx *ctx, grn_index *index, const char *path)
{
  int fd;
  struct stat stat;
  if ((fd = open(path, O_RDONLY)) == -1) { return; }
  if (fstat(fd, &stat) != -1) {
    char *buf, *bp;
    off_t rest = stat.st_size;
    if ((buf = malloc(rest))) {
      ssize_t ss;
      for (bp = buf; rest; rest -= ss, bp += ss) {
        if ((ss = read(fd, bp, rest)) == -1) { goto exit; }
      }
      if (grn_index_upd(ctx, index, path, NULL, 0, buf, stat.st_size)) {
        fprintf(stderr, "grn_index_upd failed(%s)\n", path);
      }
    }
    free(buf);
  }
exit :
  close(fd);
}

int
do_index(grn_ctx *ctx, grn_index *index, const char *path)
{
  struct dirent *de;
  char pathbuf[PATH_MAX];
  DIR *d = opendir(path);
  size_t pathlen = strlen(path);
  if (!d) {
    fprintf(stderr, "opendir: %s\n", strerror(errno));
    return -1;
  }
  memcpy(pathbuf, path, pathlen);
  if (pathbuf[pathlen - 1] != '/') { pathbuf[pathlen++] = '/'; }
  while ((de = readdir(d)) != NULL) {
    if (de->d_name[0] == '.') { continue; }
    strcpy(pathbuf + pathlen, de->d_name);
    do_file(ctx, index, pathbuf);
  }
  closedir(d);
  return 0;
}

void
do_search(grn_ctx *ctx, grn_index *index)
{
  char buf[4096];
  for (;;) {
    grn_obj *res;
    if (isatty(0)) { fputs("> ", stderr); }
    if (!fgets(buf, 4096, stdin)) { break; }
    if ((res = grn_index_sel(ctx, index, buf, strlen(buf) - 1))) {
      unsigned int n = grn_table_size(ctx, res);
      printf("%u hits\n", n);
      if (n) {
        grn_table_cursor *tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0);
        if (tc) {
          while (grn_table_cursor_next(ctx, tc)) {
            grn_id *ridp;
            int keylen;
            char keybuf[GRN_TABLE_MAX_KEY_SIZE];
            if (!grn_table_cursor_get_key(ctx, tc, (void *)&ridp)) {
              fprintf(stderr, "grn_table_cursor_get_key failed(%d)\n", ctx->rc);
            } else {
              if (!(keylen = grn_table_get_key(ctx, index->keys, *ridp, keybuf, GRN_TABLE_MAX_KEY_SIZE))) {
                fprintf(stderr, "grn_table_get_key failed(%d)\n", ctx->rc);
              } else {
                keybuf[keylen] = '\0';
                puts(keybuf);
              }
            }
          }
          grn_table_cursor_close(ctx, tc);
        } else {
          fprintf(stderr, "grn_table_cursor_open failed(%d)\n", ctx->rc);
        }
      }
      grn_obj_close(ctx, res);
    }
  }
}

int
main(int argc, char **argv)
{
  int r = 0;
  grn_ctx ctx;
  grn_index *index;
  if (argc < 2) {
    fprintf(stderr, "usage: %s db_pathname [target_dir]\n", argv[0]);
    return -1;
  }
  if (grn_init()) {
    fprintf(stderr, "grn_init() failed\n");
    return -1;
  }
  if (grn_ctx_init(&ctx, 0)) {
    fprintf(stderr, "grn_ctx_init() failed\n");
    return -1;
  }
  if (argc > 2) {
    index = grn_index_create(&ctx, argv[1]);
    do_index(&ctx, index, argv[2]);
  } else {
    index = grn_index_open(&ctx, argv[1]);
    do_search(&ctx, index);
  }
  if (index) { grn_index_close(&ctx, index); }
  grn_ctx_fin(&ctx);
  grn_fin();
  return r;
}
