/*
  Copyright (C) 2015  Brazil

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

#include <grn_pat.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

/*
 * N_EXECS (5) and N_OPS (1000) may not be enough to detect a rare bug, but it
 * will take too long time if enough values are used, such as N_EXECS (100) and
 * N_OPS (10000).
 */

#define N_EXECS   5    /* The number of execution in each test case. */
#define N_OPS     1000 /* The number of operations in each execution.  */
#define N_RECS    8    /* The maximum number of records */
                       /* This value is used as the buffer size. */
#define MAX_LEN   8    /* The maximum value length. */
                       /* This value is used as the buffer size. */
#define MIN_LABEL '0'  /* The minimum value label. */
#define MAX_LABEL '7'  /* The maximum value label. */

/* Value. */
typedef struct {
  uint8_t buf[MAX_LEN];
  int     len;
} val_t;

static int val_cmp(const val_t *lhs, const val_t *rhs)
{
  int min_len = (lhs->len < rhs->len) ? lhs->len : rhs->len;
  int cmp = memcmp(lhs->buf, rhs->buf, min_len);
  return cmp ? cmp : lhs->len - rhs->len;
}

/* Operation type. */
typedef enum {
  OP_ADD,
  OP_DEL
} op_type;

/* Operation. */
typedef struct {
  op_type type;
  val_t   val;
} op_t;

/* Record. */
typedef struct {
  grn_id id;
  val_t  key;
} rec_t;

static int rec_cmp(const void *lhs, const void *rhs)
{
  const rec_t *lhs_rec = (const rec_t *)lhs;
  const rec_t *rhs_rec = (const rec_t *)rhs;
  return val_cmp(&lhs_rec->key, &rhs_rec->key);
}

grn_ctx *ctx;
grn_pat *pat;

int     max_len;    /* The maximum value length. */
int     max_n_recs; /* The maximum number of records. */

op_t    ops[N_OPS];
int     n_ops;
rec_t   recs[N_RECS];
int     n_recs;

/* Random number generator (128-bit XorShift) */
struct {
  uint32_t x, y, z, w;
} rng;

static void rng_init(uint32_t seed)
{
  rng.x = 123456789 ^ seed;
  rng.y = 362436069;
  rng.z = 521288629;
  rng.w = 88675123;
}

static uint32_t rng_gen(void)
{
  uint32_t t;
  t = rng.x ^ (rng.x << 11);
  rng.x = rng.y;
  rng.y = rng.z;
  rng.z = rng.w;
  rng.w = (rng.w ^ (rng.w >> 19)) ^ (t ^ (t >> 8));
  return rng.w;
}

static int rng_gen_len(void)
{
  return (rng_gen() % max_len) + 1;
}

static uint8_t rng_gen_label(void)
{
  return (uint8_t)(MIN_LABEL + (rng_gen() % (MAX_LABEL - MIN_LABEL + 1)));
}

static val_t rng_gen_val(void)
{
  int i;
  val_t val;
  val.len = rng_gen_len();
  for (i = 0; i < val.len; i++) {
    val.buf[i] = rng_gen_label();
  }
  return val;
}

static op_type rng_gen_op_type(void)
{
  if (n_recs >= max_n_recs) {
    return OP_DEL;
  }
  return (rng_gen() & 1) ? OP_ADD : OP_DEL;
}

static op_t rng_gen_op(void)
{
  op_t op;
  op.type = rng_gen_op_type();
  if (op.type == OP_ADD) {
    op.val = rng_gen_val();
  } else {
    if (n_recs && (rng_gen() & 1)) {
      op.val = recs[rng_gen() % n_recs].key;
    } else {
      op.val = rng_gen_val();
    }
  }
  return op;
}

static void exec_add(op_t *op)
{
  int i, added, added_expected = 1;
  grn_id id, id_expected = 0;
  for (i = 0; i < n_recs; i++) {
    if (!val_cmp(&op->val, &recs[i].key)) {
      id_expected = recs[i].id;
      added_expected = 0;
      break;
    }
  }
  id = grn_pat_add(ctx, pat, op->val.buf, op->val.len, NULL, &added);
  if (added_expected) {
    cut_assert(id && added);
    recs[n_recs].id = id;
    recs[n_recs].key = op->val;
    n_recs++;
  } else {
    cut_assert(id && !added);
  }
}

static void exec_del(op_t *op)
{
  int i, rec_id = -1;
  grn_rc rc;
  for (i = 0; i < n_recs; i++) {
    if (!val_cmp(&op->val, &recs[i].key)) {
      rec_id = i;
      break;
    }
  }
  rc = grn_pat_delete(ctx, pat, op->val.buf, op->val.len, NULL);
  if (rec_id != -1) {
    grn_test_assert_equal_rc(GRN_SUCCESS, rc);
    n_recs--;
    for (i = rec_id; i < n_recs; i++) {
      recs[i] = recs[i + 1];
    }
  } else {
    grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT, rc);
  }
}

static void exec_check(void)
{
  int i;
  grn_pat_cursor *cursor;

  /* Test grn_pat_get(). */
  for (i = 0; i < n_recs; i++) {
    const val_t *key = &recs[i].key;
    grn_id id = grn_pat_get(ctx, pat, key->buf, key->len, NULL);
    cut_assert_equal_int(recs[i].id, id);
  }

  /* Test grn_pat_cursor (GRN_CURSOR_BY_KEY). */
  qsort(recs, n_recs, sizeof(rec_t), rec_cmp);
  cursor = grn_pat_cursor_open(ctx, pat, NULL, 0, NULL, 0, 0, -1,
                               GRN_CURSOR_BY_KEY);
  cut_assert_not_null(cursor);
  for (i = 0; i < n_recs; i++) {
    grn_id id = grn_pat_cursor_next(ctx, cursor);
    cut_assert_equal_int(recs[i].id, id);
  }
  grn_id id = grn_pat_cursor_next(ctx, cursor);
  cut_assert_equal_int(GRN_ID_NIL, id);
  grn_pat_cursor_close(ctx, cursor);
}

static void exec_test(uint32_t seed)
{
  rng_init(seed);
  n_ops = 0;
  n_recs = 0;
  while (n_ops < N_OPS) {
    op_t *op = &ops[n_ops];
    ops[n_ops++] = rng_gen_op();
    if (op->type == OP_ADD) {
      exec_add(op);
    } else {
      exec_del(op);
    }
    exec_check();
  }
}

static void run_test(void)
{
  int i;
  for (i = 0; i < N_EXECS; i++) {
    pat = grn_pat_create(ctx, NULL, GRN_PAT_MAX_KEY_SIZE, 0,
                         GRN_OBJ_KEY_VAR_SIZE | GRN_OBJ_TEMPORARY);
    cut_assert_not_null(pat);

    exec_test(i);

    grn_pat_close(ctx, pat);
    pat = NULL;
  }
}

void cut_setup(void)
{
  ctx = NULL;
  pat = NULL;

  ctx = grn_ctx_open(0);
  cut_assert_not_null(ctx);
}

void cut_teardown(void)
{
  if (ctx) {
    if (pat) {
      grn_pat_close(ctx, pat);
    }
    grn_ctx_fin(ctx);
  }
}

#define GRN_TEST_DEFINE(_max_len, _max_n_recs)\
  void test_len ## _max_len ## _recs ## _max_n_recs(void)\
  {\
    max_len = _max_len;\
    max_n_recs = _max_n_recs;\
    run_test();\
  }
GRN_TEST_DEFINE(1, 1)
GRN_TEST_DEFINE(1, 2)
GRN_TEST_DEFINE(1, 3)
GRN_TEST_DEFINE(2, 1)
GRN_TEST_DEFINE(2, 2)
GRN_TEST_DEFINE(2, 3)
GRN_TEST_DEFINE(3, 1)
GRN_TEST_DEFINE(3, 2)
GRN_TEST_DEFINE(3, 3)
#undef GRN_TEST_DEFINE_TEST
