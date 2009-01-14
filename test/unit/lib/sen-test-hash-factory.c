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

#include "sen-test-hash-factory.h"

#define SEN_TEST_HASH_FACTORY_GET_PRIVATE(obj)                          \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj), SEN_TYPE_TEST_HASH_FACTORY,       \
                               SenTestHashFactoryPrivate))

typedef struct _SenTestHashFactoryPrivate	SenTestHashFactoryPrivate;
struct _SenTestHashFactoryPrivate
{
  sen_ctx *context;
  sen_hash *hash;
  sen_hash_cursor *cursor;
  SenTestContextFlags context_flags;
  sen_logger_info *logger;
  gchar *path;
  guint32 key_size;
  guint32 value_size;
  SenTestHashFlags flags;
  SenTestEncoding encoding;
  gchar *cursor_min;
  guint32 cursor_min_size;
  gchar *cursor_max;
  guint32 cursor_max_size;
  SenTestCursorFlags cursor_flags;
};


G_DEFINE_TYPE(SenTestHashFactory, sen_test_hash_factory, G_TYPE_OBJECT)

static void dispose         (GObject               *object);

static void
sen_test_hash_factory_class_init(SenTestHashFactoryClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->dispose = dispose;

  g_type_class_add_private(gobject_class, sizeof(SenTestHashFactoryPrivate));
}

static void
sen_test_hash_factory_init(SenTestHashFactory *factory)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);

  priv->context = NULL;
  priv->hash = NULL;
  priv->cursor = NULL;
  priv->context_flags = SEN_TEST_CONTEXT_FLAG_USE_QL;
  priv->logger = NULL;
  priv->path = NULL;
  priv->key_size = sizeof(guint32);
  priv->value_size = SEN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE;
  priv->flags = SEN_TEST_HASH_FLAG_NONE;
  priv->encoding = SEN_TEST_ENCODING_DEFAULT;
  priv->cursor_min = NULL;
  priv->cursor_min_size = 0;
  priv->cursor_max = NULL;
  priv->cursor_max_size = 0;
  priv->cursor_flags = SEN_TEST_CURSOR_FLAG_NONE;
}

static void
cursor_free(SenTestHashFactoryPrivate *priv)
{
  if (priv->cursor) {
    sen_hash_cursor_close(priv->context, priv->cursor);
    priv->cursor = NULL;
  }
}

static void
hash_free(SenTestHashFactoryPrivate *priv)
{
  if (priv->hash) {
    cursor_free(priv);
    sen_hash_close(priv->context, priv->hash);
    priv->hash = NULL;
  }
}

static void
context_free(SenTestHashFactoryPrivate *priv)
{
  if (priv->context) {
    hash_free(priv);
    sen_ctx_close(priv->context);
    priv->context = NULL;
  }
}

static void
dispose(GObject *object)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(object);

  context_free(priv);

  if (priv->logger)
    priv->logger = NULL;

  if (priv->path) {
    g_free(priv->path);
    priv->path = NULL;
  }

  if (priv->cursor_min) {
    g_free(priv->cursor_min);
    priv->cursor_min = NULL;
  }

  if (priv->cursor_max) {
    g_free(priv->cursor_max);
    priv->cursor_max = NULL;
  }

  G_OBJECT_CLASS(sen_test_hash_factory_parent_class)->dispose(object);
}

GQuark
sen_test_hash_factory_error_quark(void)
{
  return g_quark_from_static_string("sen-test-hash-factory-error-quark");
}

SenTestHashFactory *
sen_test_hash_factory_new(void)
{
  return g_object_new(SEN_TYPE_TEST_HASH_FACTORY, NULL);
}

static gboolean
sen_test_hash_factory_ensure_context(SenTestHashFactory *factory, GError **error)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  context_free(priv);

  priv->context = sen_ctx_open(NULL, priv->context_flags);
  if (!priv->context) {
    g_set_error(error,
                SEN_TEST_HASH_FACTORY_ERROR,
                SEN_TEST_HASH_FACTORY_ERROR_CONTEXT_NULL,
                "failed to open sen_ctx"
                /* FIXME: add error detail */);
    return FALSE;
  }

  return TRUE;
}

sen_hash *
sen_test_hash_factory_open(SenTestHashFactory *factory, GError **error)
{
  SenTestHashFactoryPrivate *priv;

  if (!sen_test_hash_factory_ensure_context(factory, error))
    return NULL;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  priv->hash = sen_hash_open(priv->context, priv->path);
  if (!priv->hash) {
    g_set_error(error,
                SEN_TEST_HASH_FACTORY_ERROR,
                SEN_TEST_HASH_FACTORY_ERROR_NULL,
                "failed to open sen_hash"
                /* FIXME: add error detail */);
  }
  return priv->hash;
}

sen_hash *
sen_test_hash_factory_create(SenTestHashFactory *factory, GError **error)
{
  SenTestHashFactoryPrivate *priv;

  if (!sen_test_hash_factory_ensure_context(factory, error))
    return NULL;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  priv->hash = sen_hash_create(priv->context, priv->path, priv->key_size,
                               priv->value_size, priv->flags, priv->encoding);
  if (!priv->hash) {
    g_set_error(error,
                SEN_TEST_HASH_FACTORY_ERROR,
                SEN_TEST_HASH_FACTORY_ERROR_NULL,
                "failed to create sen_hash"
                /* FIXME: add error detail */);
  }

  return priv->hash;
}

sen_hash_cursor *
sen_test_hash_factory_open_cursor(SenTestHashFactory *factory, GError **error)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (!priv->hash) {
    g_set_error(error,
                SEN_TEST_HASH_FACTORY_ERROR,
                SEN_TEST_HASH_FACTORY_ERROR_NULL,
                "sen_hash is neither opened nor created.");
    return NULL;
  }

  cursor_free(priv);
  priv->cursor = sen_hash_cursor_open(priv->context, priv->hash,
                                      priv->cursor_min, priv->cursor_min_size,
                                      priv->cursor_max, priv->cursor_max_size,
                                      priv->cursor_flags);
  if (!priv->cursor) {
    g_set_error(error,
                SEN_TEST_HASH_FACTORY_ERROR,
                SEN_TEST_HASH_FACTORY_ERROR_CURSOR_NULL,
                "failed to open sen_hash_cursor"
                /* FIXME: add error detail */);
  }

  return priv->cursor;
}

sen_ctx *
sen_test_hash_factory_get_context(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->context;
}

SenTestContextFlags
sen_test_hash_factory_get_context_flags(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->context_flags;
}

void
sen_test_hash_factory_set_context_flags(SenTestHashFactory *factory,
                                        SenTestContextFlags flags)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->context_flags = flags;
}

sen_logger_info *
sen_test_hash_factory_get_logger(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->logger;
}

void
sen_test_hash_factory_set_logger(SenTestHashFactory *factory,
                                 sen_logger_info *logger)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->logger = logger;
}

const gchar *
sen_test_hash_factory_get_path(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->path;
}

void
sen_test_hash_factory_set_path(SenTestHashFactory *factory,
                               const gchar *path)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (priv->path)
    g_free(priv->path);
  priv->path = g_strdup(path);
}

guint32
sen_test_hash_factory_get_key_size(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->key_size;
}

void
sen_test_hash_factory_set_key_size(SenTestHashFactory *factory,
                                   guint32 key_size)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->key_size = key_size;
}

guint32
sen_test_hash_factory_get_value_size(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->value_size;
}

void
sen_test_hash_factory_set_value_size(SenTestHashFactory *factory,
                                     guint32 value_size)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->value_size = value_size;
}

SenTestHashFlags
sen_test_hash_factory_get_flags(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->flags;
}

void
sen_test_hash_factory_set_flags(SenTestHashFactory *factory,
                                SenTestHashFlags flags)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->flags = flags;
}

void
sen_test_hash_factory_add_flags(SenTestHashFactory *factory,
                                SenTestHashFlags flags)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->flags |= flags;
}

SenTestEncoding
sen_test_hash_factory_get_encoding(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->encoding;
}

void
sen_test_hash_factory_set_encoding(SenTestHashFactory *factory,
                                   SenTestEncoding encoding)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->encoding = encoding;
}

const gchar *
sen_test_hash_factory_get_cursor_min(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_min;
}

guint32
sen_test_hash_factory_get_cursor_min_size(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_min_size;
}

void
sen_test_hash_factory_set_cursor_min(SenTestHashFactory *factory,
                                     const gchar *min,
                                     guint32 size)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (priv->cursor_min)
    g_free(priv->cursor_min);
  priv->cursor_min = g_memdup(min, size);
  priv->cursor_min_size = size;
}

const gchar *
sen_test_hash_factory_get_cursor_max(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_max;
}

guint32
sen_test_hash_factory_get_cursor_max_size(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_max_size;
}

void
sen_test_hash_factory_set_cursor_max(SenTestHashFactory *factory,
                                     const gchar *max,
                                     guint32 size)
{
  SenTestHashFactoryPrivate *priv;

  priv = SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (priv->cursor_max)
    g_free(priv->cursor_max);
  priv->cursor_max = g_memdup(max, size);
  priv->cursor_max_size = size;
}

SenTestCursorFlags
sen_test_hash_factory_get_cursor_flags(SenTestHashFactory *factory)
{
  return SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_flags;
}

void
sen_test_hash_factory_set_cursor_flags(SenTestHashFactory *factory,
                                       SenTestCursorFlags flags)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_flags = flags;
}

void
sen_test_hash_factory_add_cursor_flags(SenTestHashFactory *factory,
                                       SenTestCursorFlags flags)
{
  SEN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_flags |= flags;
}

