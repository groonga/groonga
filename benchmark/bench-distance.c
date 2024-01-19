/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

/*
  groonga: 4e4921264d149ec2736305b58ee34054ffaddb0b
  CAMKE_BUILD_TYPE: Release
  CPU: AMD Ryzen 9 3900X 12-Core Processor
  (AVX2 is available but AVX512 isn't available.)

  $ benchmark/bench-distance
                            (total)    (average)  (median)
    cosine (Float32, D=75): (0.0014s)  (0.0014ms) (0.0010ms)
   cosine (Float32, D=300): (0.0015s)  (0.0015ms) (0.0010ms)
   cosine (Float32, D=512): (0.0017s)  (0.0017ms) (0.0020ms)
   cosine (Float32, D=600): (0.0017s)  (0.0017ms) (0.0020ms)
  cosine (Float32, D=1024): (0.0020s)  (0.0020ms) (0.0020ms)
      cosine (Float, D=75): (0.0014s)  (0.0014ms) (0.0010ms)
     cosine (Float, D=300): (0.0017s)  (0.0017ms) (0.0020ms)
     cosine (Float, D=512): (0.0020s)  (0.0020ms) (0.0020ms)
     cosine (Float, D=600): (0.0022s)  (0.0022ms) (0.0020ms)
    cosine (Float, D=1024): (0.0028s)  (0.0028ms) (0.0030ms)

  $ GRN_DISTANCE_SIMD=no benchmark/bench-distance
                            (total)    (average)  (median)
    cosine (Float32, D=75): (0.0012s)  (0.0012ms) (0.0010ms)
   cosine (Float32, D=300): (0.0016s)  (0.0016ms) (0.0020ms)
   cosine (Float32, D=512): (0.0021s)  (0.0021ms) (0.0020ms)
   cosine (Float32, D=600): (0.0022s)  (0.0022ms) (0.0020ms)
  cosine (Float32, D=1024): (0.0031s)  (0.0031ms) (0.0030ms)
      cosine (Float, D=75): (0.0013s)  (0.0013ms) (0.0010ms)
     cosine (Float, D=300): (0.0028s)  (0.0028ms) (0.0030ms)
     cosine (Float, D=512): (0.0041s)  (0.0041ms) (0.0040ms)
     cosine (Float, D=600): (0.0049s)  (0.0049ms) (0.0050ms)
    cosine (Float, D=1024): (0.0075s)  (0.0075ms) (0.0070ms)
 */

#include <string.h>
#include <stdlib.h>

#include <grn_db.h>
#include <groonga.h>

#include "lib/benchmark.h"

typedef struct _BenchmarkData {
  grn_ctx context;
  grn_obj *database;
  grn_obj vector1;
  grn_obj vector2;
} BenchmarkData;

static void
bench_distance_cosine(gpointer user_data)
{
  BenchmarkData *data = user_data;

  grn_distance_cosine(&(data->context), &(data->vector1), &(data->vector2));
}

static void
bench_distance_inner_product(gpointer user_data)
{
  BenchmarkData *data = user_data;

  grn_distance_inner_product(&(data->context),
                             &(data->vector1),
                             &(data->vector2));
}

static void
bench_setup_common(gpointer user_data)
{
  BenchmarkData *data = user_data;

  grn_ctx_init(&(data->context), 0);
  data->database = grn_db_create(&(data->context), NULL, NULL);
}

static void
bench_setup_float32_n(gpointer user_data, size_t n)
{
  BenchmarkData *data = user_data;

  bench_setup_common(user_data);
  GRN_FLOAT32_INIT(&(data->vector1), GRN_OBJ_VECTOR);
  GRN_FLOAT32_INIT(&(data->vector2), GRN_OBJ_VECTOR);

  g_random_set_seed(29);
  size_t i;
  for (i = 0; i < n; i++) {
    GRN_FLOAT32_PUT(&(data->context), &(data->vector1), g_random_double());
    GRN_FLOAT32_PUT(&(data->context), &(data->vector2), g_random_double());
  }
}

static void
bench_setup_float32_75(gpointer user_data)
{
  bench_setup_float32_n(user_data, 75);
}

static void
bench_setup_float32_300(gpointer user_data)
{
  bench_setup_float32_n(user_data, 300);
}

static void
bench_setup_float32_512(gpointer user_data)
{
  bench_setup_float32_n(user_data, 512);
}

static void
bench_setup_float32_600(gpointer user_data)
{
  bench_setup_float32_n(user_data, 600);
}

static void
bench_setup_float32_1024(gpointer user_data)
{
  bench_setup_float32_n(user_data, 1024);
}

static void
bench_setup_float_n(gpointer user_data, size_t n)
{
  BenchmarkData *data = user_data;

  bench_setup_common(user_data);
  GRN_FLOAT_INIT(&(data->vector1), GRN_OBJ_VECTOR);
  GRN_FLOAT_INIT(&(data->vector2), GRN_OBJ_VECTOR);

  g_random_set_seed(29);
  size_t i;
  for (i = 0; i < n; i++) {
    GRN_FLOAT_PUT(&(data->context), &(data->vector1), g_random_double());
    GRN_FLOAT_PUT(&(data->context), &(data->vector2), g_random_double());
  }
}

static void
bench_setup_float_75(gpointer user_data)
{
  bench_setup_float_n(user_data, 75);
}

static void
bench_setup_float_300(gpointer user_data)
{
  bench_setup_float_n(user_data, 300);
}

static void
bench_setup_float_512(gpointer user_data)
{
  bench_setup_float_n(user_data, 512);
}

static void
bench_setup_float_600(gpointer user_data)
{
  bench_setup_float_n(user_data, 600);
}

static void
bench_setup_float_1024(gpointer user_data)
{
  bench_setup_float_n(user_data, 1024);
}

static void
bench_teardown(gpointer user_data)
{
  BenchmarkData *data = user_data;

  GRN_OBJ_FIN(&(data->context), &(data->vector1));
  GRN_OBJ_FIN(&(data->context), &(data->vector2));
  grn_obj_unlink(&(data->context), data->database);
  grn_ctx_fin(&(data->context));
}

int
main(int argc, gchar **argv)
{
  grn_rc rc;
  BenchmarkData data;
  BenchReporter *reporter;
  gint n = 1000;

  rc = grn_init();
  if (rc != GRN_SUCCESS) {
    g_print("failed to initialize Groonga: <%d>: %s\n",
            rc,
            grn_get_global_error_message());
    return EXIT_FAILURE;
  }
  bench_init(&argc, &argv);

  {
    const gchar *groonga_bench_n;
    groonga_bench_n = g_getenv("GROONGA_BENCH_N");
    if (groonga_bench_n) {
      n = atoi(groonga_bench_n);
    }
  }

  reporter = bench_reporter_new();

#define REGISTER(label, algorithm, setup)                                      \
  bench_reporter_register(reporter,                                            \
                          #algorithm " " label,                                \
                          n,                                                   \
                          bench_setup_##setup,                                 \
                          bench_distance_##algorithm,                          \
                          bench_teardown,                                      \
                          &data)
  REGISTER("(Float32, D=75)", cosine, float32_75);
  REGISTER("(Float32, D=300)", cosine, float32_300);
  REGISTER("(Float32, D=512)", cosine, float32_512);
  REGISTER("(Float32, D=600)", cosine, float32_600);
  REGISTER("(Float32, D=1024)", cosine, float32_1024);

  REGISTER("(Float, D=75)", cosine, float_75);
  REGISTER("(Float, D=300)", cosine, float_300);
  REGISTER("(Float, D=512)", cosine, float_512);
  REGISTER("(Float, D=600)", cosine, float_600);
  REGISTER("(Float, D=1024)", cosine, float_1024);

  REGISTER("(Float32, D=75)", inner_product, float32_75);
  REGISTER("(Float32, D=300)", inner_product, float32_300);
  REGISTER("(Float32, D=512)", inner_product, float32_512);
  REGISTER("(Float32, D=600)", inner_product, float32_600);
  REGISTER("(Float32, D=1024)", inner_product, float32_1024);

  REGISTER("(Float, D=75)", inner_product, float_75);
  REGISTER("(Float, D=300)", inner_product, float_300);
  REGISTER("(Float, D=512)", inner_product, float_512);
  REGISTER("(Float, D=600)", inner_product, float_600);
  REGISTER("(Float, D=1024)", inner_product, float_1024);
#undef REGISTER

  bench_reporter_run(reporter);
  g_object_unref(reporter);

  bench_quit();
  grn_fin();

  return EXIT_SUCCESS;
}
