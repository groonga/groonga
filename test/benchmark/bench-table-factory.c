/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>

#include <groonga.h>
extern sen_ctx sen_gctx;

#include "lib/benchmark.h"

#define DEFAULT_FLAGS (SEN_OBJ_PERSISTENT | SEN_OBJ_TABLE_PAT_KEY)
#define DEFAULT_VALUE_SIZE (1024)

typedef struct _BenchmarkData
{
  gchar *base_dir;
  gchar *space_path;

  sen_ctx *context;
  sen_obj *space;
  const char *name;
  unsigned name_size;
  char *path;
  sen_obj_flags flags;
  sen_obj *key_type;
  unsigned value_size;
  sen_encoding encoding;
} BenchmarkData;

static void
bench_normal(gpointer user_data)
{
  BenchmarkData *data = user_data;
  sen_obj *table;

  table = sen_table_create(data->context,
                           data->name, data->name_size,
                           data->path, data->flags,
                           data->key_type, data->value_size,
                           data->encoding);
  sen_obj_close(data->context, table);
}

static void
bench_normal_temporary(gpointer user_data)
{
  BenchmarkData *data = user_data;
  sen_obj *table;

  table = sen_table_create(data->context,
                           data->name, data->name_size,
                           NULL, data->flags & ~SEN_OBJ_PERSISTENT,
                           data->key_type, data->value_size,
                           data->encoding);
  sen_obj_close(data->context, table);
}

typedef struct _sen_table_factory
{
  sen_ctx *context;
  sen_obj *space;
  char *name;
  unsigned name_size;
  char *path;
  sen_obj_flags flags;
  sen_obj *key_type;
  unsigned value_size;
  sen_encoding encoding;
} sen_table_factory;

static sen_table_factory *
sen_table_factory_create(void)
{
  sen_table_factory *factory;

  factory = g_new0(sen_table_factory, 1);

  factory->context = &sen_gctx;
  factory->space = NULL;
  factory->name = NULL;
  factory->name_size = 0;
  factory->path = NULL;
  factory->flags = DEFAULT_FLAGS;
  factory->key_type = NULL;
  factory->value_size = DEFAULT_VALUE_SIZE;
  factory->encoding = sen_enc_default;

  return factory;
}

static void
sen_table_factory_set_context(sen_table_factory *factory, sen_ctx *context)
{
  factory->context = context;
}

static void
sen_table_factory_set_space(sen_table_factory *factory, sen_obj *space)
{
  factory->space = space;
}

static void
sen_table_factory_set_name(sen_table_factory *factory, const char *name)
{
  factory->name = g_strdup(name);
  factory->name_size = strlen(name);
}

static void
sen_table_factory_set_path(sen_table_factory *factory, const char *path)
{
  factory->path = g_strdup(path);
  if (path)
    factory->flags |= SEN_OBJ_PERSISTENT;
  else
    factory->flags &= ~SEN_OBJ_PERSISTENT;
}

static void
sen_table_factory_set_key_type(sen_table_factory *factory, sen_obj *key_type)
{
  factory->key_type = key_type;
}

static sen_obj *
sen_table_factory_make(sen_table_factory *factory)
{
  return sen_table_create(factory->context,
                          factory->name, factory->name_size,
                          factory->path, factory->flags,
                          factory->key_type, factory->value_size,
                          factory->encoding);
}

static void
sen_table_factory_close(sen_table_factory *factory)
{
  g_free(factory->name);
  g_free(factory->path);
  g_free(factory);
}

static void
bench_factory(gpointer user_data)
{
  BenchmarkData *data = user_data;
  sen_table_factory *factory;
  sen_obj *table;

  factory = sen_table_factory_create();
  sen_table_factory_set_context(factory, data->context);
  sen_table_factory_set_space(factory, data->space);
  sen_table_factory_set_name(factory, data->name);
  sen_table_factory_set_path(factory, data->path);
  sen_table_factory_set_key_type(factory, data->key_type);

  table = sen_table_factory_make(factory);
  sen_obj_close(data->context, table);

  sen_table_factory_close(factory);
}

static void
bench_factory_temporary(gpointer user_data)
{
  BenchmarkData *data = user_data;
  sen_table_factory *factory;
  sen_obj *table;

  factory = sen_table_factory_create();
  sen_table_factory_set_context(factory, data->context);
  sen_table_factory_set_space(factory, data->space);
  sen_table_factory_set_name(factory, data->name);
  sen_table_factory_set_key_type(factory, data->key_type);

  table = sen_table_factory_make(factory);
  sen_obj_close(data->context, table);

  sen_table_factory_close(factory);
}

static void
bench_setup(gpointer user_data)
{
  BenchmarkData *data = user_data;
  const gchar *type_name;

  bench_utils_remove_path_recursive_force(data->base_dir);
  g_mkdir_with_parents(data->base_dir, 0755);

  data->context = sen_ctx_open(NULL, SEN_CTX_USE_QL);
  data->space = sen_space_create(data->context,
                                 data->space_path, sen_enc_default);

  type_name = "name";
  data->key_type = sen_type_create(data->context,
                                   type_name, strlen(type_name),
                                   SEN_OBJ_KEY_UINT, sizeof(sen_id));

}

static void
bench_teardown(gpointer user_data)
{
  BenchmarkData *data = user_data;

  sen_obj_close(data->context, data->key_type);
  sen_obj_close(data->context, data->space);
  sen_ctx_close(data->context);
  bench_utils_remove_path_recursive_force(data->base_dir);
}

int
main(int argc, gchar **argv)
{
  BenchmarkData data;
  BenchReporter *reporter;
  gint n = 100;

  sen_init();
  bench_init(&argc, &argv);

  data.base_dir = g_build_filename(g_get_tmp_dir(), "groonga-bench", NULL);
  data.space_path = g_build_filename(data.base_dir, "space", NULL);
  data.name = "table";
  data.name_size = strlen(data.name);
  data.path = g_build_filename(data.base_dir, "table", NULL);
  data.flags = DEFAULT_FLAGS;
  data.key_type = NULL;
  data.value_size = DEFAULT_VALUE_SIZE;
  data.encoding = sen_enc_default;

  reporter = bench_reporter_new();
  bench_reporter_register(reporter, "normal (persistent)", n,
                          bench_setup, bench_normal, bench_teardown, &data);
  bench_reporter_register(reporter, "factory (persistent)", n,
                          bench_setup, bench_factory, bench_teardown, &data);
  bench_reporter_register(reporter, "normal (temporary)", n,
                          bench_setup, bench_normal_temporary, bench_teardown,
                          &data);
  bench_reporter_register(reporter, "factory (temporary)", n,
                          bench_setup, bench_factory_temporary, bench_teardown,
                          &data);
  bench_reporter_run(reporter);
  g_object_unref(reporter);

  bench_utils_remove_path_recursive_force(data.base_dir);

  g_free(data.path);
  g_free(data.space_path);
  g_free(data.base_dir);

  bench_quit();
  sen_fin();

  return 0;
}
