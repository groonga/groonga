/*
  Copyright (C) 2008-2019  Sutou Kouhei <kou@clear-code.com>

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

#include "grn-test-hash-factory.h"

#define GRN_TEST_HASH_FACTORY_GET_PRIVATE(obj)                          \
  ((GrnTestHashFactoryPrivate *)                                        \
   grn_test_hash_factory_get_instance_private(GRN_TEST_HASH_FACTORY(obj)))

typedef struct _GrnTestHashFactoryPrivate	GrnTestHashFactoryPrivate;
struct _GrnTestHashFactoryPrivate
{
  grn_ctx *context;
  grn_hash *hash;
  grn_hash_cursor *cursor;
  GrnTestContextFlags context_flags;
  grn_logger_info *logger;
  gchar *path;
  guint32 key_size;
  guint32 value_size;
  GrnTestHashFlags flags;
  GrnTestEncoding encoding;
  gchar *cursor_min;
  guint32 cursor_min_size;
  gchar *cursor_max;
  guint32 cursor_max_size;
  GrnTestCursorFlags cursor_flags;
};


G_DEFINE_TYPE_WITH_PRIVATE(GrnTestHashFactory,
                           grn_test_hash_factory,
                           G_TYPE_OBJECT)

static void dispose         (GObject               *object);

static void
grn_test_hash_factory_class_init(GrnTestHashFactoryClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->dispose = dispose;
}

static void
grn_test_hash_factory_init(GrnTestHashFactory *factory)
{
  GrnTestHashFactoryPrivate *priv;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);

  priv->context = NULL;
  priv->hash = NULL;
  priv->cursor = NULL;
  priv->context_flags = GRN_TEST_CONTEXT_FLAG_USE_QL;
  priv->logger = NULL;
  priv->path = NULL;
  priv->key_size = sizeof(guint32);
  priv->value_size = GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE;
  priv->flags = GRN_TEST_HASH_FLAG_NONE;
  priv->encoding = GRN_TEST_ENCODING_DEFAULT;
  priv->cursor_min = NULL;
  priv->cursor_min_size = 0;
  priv->cursor_max = NULL;
  priv->cursor_max_size = 0;
  priv->cursor_flags = GRN_TEST_CURSOR_FLAG_NONE;
}

static void
cursor_free(GrnTestHashFactoryPrivate *priv)
{
  if (priv->cursor) {
    grn_hash_cursor_close(priv->context, priv->cursor);
    priv->cursor = NULL;
  }
}

static void
hash_free(GrnTestHashFactoryPrivate *priv)
{
  if (priv->hash) {
    cursor_free(priv);
    grn_hash_close(priv->context, priv->hash);
    priv->hash = NULL;
  }
}

static void
context_free(GrnTestHashFactoryPrivate *priv)
{
  if (priv->context) {
    hash_free(priv);
    grn_ctx_fin(priv->context);
    g_free(priv->context);
    priv->context = NULL;
  }
}

static void
dispose(GObject *object)
{
  GrnTestHashFactoryPrivate *priv;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(object);

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

  G_OBJECT_CLASS(grn_test_hash_factory_parent_class)->dispose(object);
}

GQuark
grn_test_hash_factory_error_quark(void)
{
  return g_quark_from_static_string("grn-test-hash-factory-error-quark");
}

GrnTestHashFactory *
grn_test_hash_factory_new(void)
{
  return g_object_new(GRN_TYPE_TEST_HASH_FACTORY, NULL);
}

static gboolean
grn_test_hash_factory_ensure_context(GrnTestHashFactory *factory, GError **error)
{
  GrnTestHashFactoryPrivate *priv;
  grn_rc result;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  context_free(priv);

  priv->context = g_new0(grn_ctx, 1);
  result = grn_ctx_init(priv->context, priv->context_flags);
  GRN_CTX_SET_ENCODING(priv->context, ((grn_encoding)priv->encoding));
  if (result != GRN_SUCCESS) {
    g_set_error(error,
                GRN_TEST_HASH_FACTORY_ERROR,
                GRN_TEST_HASH_FACTORY_ERROR_CONTEXT_NULL,
                "failed to init grn_ctx"
                /* FIXME: add error detail */);
    return FALSE;
  }

  return TRUE;
}

grn_hash *
grn_test_hash_factory_open(GrnTestHashFactory *factory, GError **error)
{
  GrnTestHashFactoryPrivate *priv;

  if (!grn_test_hash_factory_ensure_context(factory, error))
    return NULL;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  priv->hash = grn_hash_open(priv->context, priv->path);
  if (!priv->hash) {
    g_set_error(error,
                GRN_TEST_HASH_FACTORY_ERROR,
                GRN_TEST_HASH_FACTORY_ERROR_NULL,
                "failed to open grn_hash"
                /* FIXME: add error detail */);
  }
  return priv->hash;
}

grn_hash *
grn_test_hash_factory_create(GrnTestHashFactory *factory, GError **error)
{
  GrnTestHashFactoryPrivate *priv;

  if (!grn_test_hash_factory_ensure_context(factory, error))
    return NULL;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  GRN_CTX_SET_ENCODING(priv->context, (grn_encoding)(priv->encoding));
  priv->hash = grn_hash_create(priv->context, priv->path, priv->key_size,
                               priv->value_size, priv->flags);
  if (!priv->hash) {
    g_set_error(error,
                GRN_TEST_HASH_FACTORY_ERROR,
                GRN_TEST_HASH_FACTORY_ERROR_NULL,
                "failed to create grn_hash"
                /* FIXME: add error detail */);
  }

  return priv->hash;
}

grn_hash_cursor *
grn_test_hash_factory_open_cursor(GrnTestHashFactory *factory, GError **error)
{
  GrnTestHashFactoryPrivate *priv;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (!priv->hash) {
    g_set_error(error,
                GRN_TEST_HASH_FACTORY_ERROR,
                GRN_TEST_HASH_FACTORY_ERROR_NULL,
                "grn_hash is neither opened nor created.");
    return NULL;
  }

  cursor_free(priv);
  priv->cursor = grn_hash_cursor_open(priv->context, priv->hash,
                                      priv->cursor_min, priv->cursor_min_size,
                                      priv->cursor_max, priv->cursor_max_size,
                                      0, -1, priv->cursor_flags);
  if (!priv->cursor) {
    g_set_error(error,
                GRN_TEST_HASH_FACTORY_ERROR,
                GRN_TEST_HASH_FACTORY_ERROR_CURSOR_NULL,
                "failed to open grn_hash_cursor"
                /* FIXME: add error detail */);
  }

  return priv->cursor;
}

grn_ctx *
grn_test_hash_factory_get_context(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->context;
}

GrnTestContextFlags
grn_test_hash_factory_get_context_flags(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->context_flags;
}

void
grn_test_hash_factory_set_context_flags(GrnTestHashFactory *factory,
                                        GrnTestContextFlags flags)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->context_flags = flags;
}

grn_logger_info *
grn_test_hash_factory_get_logger(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->logger;
}

void
grn_test_hash_factory_set_logger(GrnTestHashFactory *factory,
                                 grn_logger_info *logger)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->logger = logger;
}

const gchar *
grn_test_hash_factory_get_path(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->path;
}

void
grn_test_hash_factory_set_path(GrnTestHashFactory *factory,
                               const gchar *path)
{
  GrnTestHashFactoryPrivate *priv;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (priv->path)
    g_free(priv->path);
  priv->path = g_strdup(path);
}

guint32
grn_test_hash_factory_get_key_size(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->key_size;
}

void
grn_test_hash_factory_set_key_size(GrnTestHashFactory *factory,
                                   guint32 key_size)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->key_size = key_size;
}

guint32
grn_test_hash_factory_get_value_size(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->value_size;
}

void
grn_test_hash_factory_set_value_size(GrnTestHashFactory *factory,
                                     guint32 value_size)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->value_size = value_size;
}

GrnTestHashFlags
grn_test_hash_factory_get_flags(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->flags;
}

void
grn_test_hash_factory_set_flags(GrnTestHashFactory *factory,
                                GrnTestHashFlags flags)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->flags = flags;
}

void
grn_test_hash_factory_add_flags(GrnTestHashFactory *factory,
                                GrnTestHashFlags flags)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->flags |= flags;
}

GrnTestEncoding
grn_test_hash_factory_get_encoding(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->encoding;
}

void
grn_test_hash_factory_set_encoding(GrnTestHashFactory *factory,
                                   GrnTestEncoding encoding)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->encoding = encoding;
}

const gchar *
grn_test_hash_factory_get_cursor_min(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_min;
}

guint32
grn_test_hash_factory_get_cursor_min_size(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_min_size;
}

void
grn_test_hash_factory_set_cursor_min(GrnTestHashFactory *factory,
                                     const gchar *min,
                                     guint32 size)
{
  GrnTestHashFactoryPrivate *priv;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (priv->cursor_min)
    g_free(priv->cursor_min);
  priv->cursor_min = g_memdup(min, size);
  priv->cursor_min_size = size;
}

const gchar *
grn_test_hash_factory_get_cursor_max(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_max;
}

guint32
grn_test_hash_factory_get_cursor_max_size(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_max_size;
}

void
grn_test_hash_factory_set_cursor_max(GrnTestHashFactory *factory,
                                     const gchar *max,
                                     guint32 size)
{
  GrnTestHashFactoryPrivate *priv;

  priv = GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory);
  if (priv->cursor_max)
    g_free(priv->cursor_max);
  priv->cursor_max = g_memdup(max, size);
  priv->cursor_max_size = size;
}

GrnTestCursorFlags
grn_test_hash_factory_get_cursor_flags(GrnTestHashFactory *factory)
{
  return GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_flags;
}

void
grn_test_hash_factory_set_cursor_flags(GrnTestHashFactory *factory,
                                       GrnTestCursorFlags flags)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_flags = flags;
}

void
grn_test_hash_factory_add_cursor_flags(GrnTestHashFactory *factory,
                                       GrnTestCursorFlags flags)
{
  GRN_TEST_HASH_FACTORY_GET_PRIVATE(factory)->cursor_flags |= flags;
}
