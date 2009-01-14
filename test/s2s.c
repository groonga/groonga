#include <stdio.h>
#include <stdlib.h>
//#include "groonga/groonga.h"

#include "set.h"

#define DICT "/home/mori/work/mysql/test/random.replace.delete/prime-dict"

char buffer[4096];

static void
do_sym(int n, char *fn)
{
  int i, key, r = 0;
  sen_id id;
  sen_sym *s = sen_sym_create(fn, 4, 0, 0);
  srandom(1);
  for (i = 0; i < n; i++) {
    key = random();
    id = sen_sym_get(s, &key);
  }
  printf("%d\n", sen_sym_size(s));
  srandom(1);
  for (i = 0; i < n; i++) {
    key = random();
    id = sen_sym_at(s, &key);
    if (id != i - r + 1) { r++; }
  }
  printf("%d, %d\n", sen_sym_size(s), r);
}

static void
do_set(int n)
{
  int i, key, r = 0;
  int *v;
  sen_set_eh *e;
  sen_set *s = sen_set_open(4, 4, 0);
  srandom(1);
  sen_set_array_init(s, n);
  for (i = 0; i < n; i++) {
    key = random();

    /*
    {
      struct _sen_set_element *e;
      sen_tiny_array *a = &s->a;
      sen_id id = ++s->n_entries;
      SEN_TINY_ARRAY_AT(&s->a, id, e);
      e->key = key;
      v = (void *)e->dummy;\
    }
    */

      /*
{
    struct _sen_set_element *e;
    e = sen_tiny_array_next(&s->a);
    e->key = key;
}
*/
  SEN_SET_INT_ADD(s, (&key), v);
  if (sen_tiny_array_id(&s->a, v) != i) {
    printf("%d != %d\n", sen_tiny_array_id(&s->a, v), i);
  }
//    e = sen_set_get(s, &key, (void **) &v);
//  *v = i;

  }
  sen_set_info(s, NULL, NULL, &n);
  printf("%d\n", n);
  /*
  srandom(1);
  for (i = 0; i < n; i++) {
    key = random();
    e = sen_set_at(s, &key, (void **)&v);
    if (*v != i) { r++; }
  }
  sen_set_info(s, NULL, NULL, &n);
  printf("%d, %d\n", n, r);
  */
}

static void
do_sets(int n)
{
  sen_id id;
  void *v;
  sen_set_eh *e;
  FILE *fp = fopen(DICT, "r");
  sen_set *s = sen_set_open(0, 0, 0);
  while (!feof(fp)) {
    fgets(buffer, 4096, fp);
    e = sen_set_get(s, buffer, &v);
  }
  sen_set_info(s, NULL, NULL, &n);
  printf("%d\n", n);
  fclose(fp);

  fp = fopen(DICT, "r");
  while (!feof(fp)) {
    fgets(buffer, 4096, fp);
    e = sen_set_at(s, buffer, &v);
  }
  sen_set_info(s, NULL, NULL, &n);
  printf("%d\n", n);
  fclose(fp);
}

static void
do_syms(int n, char *fn)
{
  sen_id id;
  FILE *fp = fopen(DICT, "r");
  sen_sym *s = sen_sym_create(fn, 0, 0, 0);
  while (!feof(fp)) {
    fgets(buffer, 4096, fp);
    id = sen_sym_get(s, buffer);
  }
  printf("%d\n", sen_sym_size(s));
  fclose(fp);
  fp = fopen(DICT, "r");
  while (!feof(fp)) {
    fgets(buffer, 4096, fp);
    id = sen_sym_at(s, buffer);
  }
  printf("%d\n", sen_sym_size(s));
  fclose(fp);
}

int
main(int argc, char **argv)
{
  int n;
  if (argc < 3) {
    return -1;
  }
  n = atoi(argv[2]);
  if (argv[1][1] == 'y') {
    do_sym(n, argc > 3 ? argv[3] : NULL);
  } else {
    do_set(n);
  }
  printf("%d\n", n);
  //  sleep(30);
  return 0;
}
