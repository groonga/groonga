/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

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

#ifndef __GRN_TEST_HASH_FACTORY_H__
#define __GRN_TEST_HASH_FACTORY_H__

#include <grn_hash.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define GRN_TEST_HASH_FACTORY_ERROR           (grn_test_hash_factory_error_quark())

#define GRN_TYPE_TEST_HASH_FACTORY            (grn_test_hash_factory_get_type())
#define GRN_TEST_HASH_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GRN_TYPE_TEST_HASH_FACTORY, GrnTestHashFactory))
#define GRN_TEST_HASH_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GRN_TYPE_TEST_HASH_FACTORY, GrnTestHashFactoryClass))
#define GRN_IS_TEST_HASH_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GRN_TYPE_TEST_HASH_FACTORY))
#define GRN_IS_TEST_HASH_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GRN_TYPE_TEST_HASH_FACTORY))
#define GRN_TEST_HASH_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GRN_TYPE_TEST_HASH_FACTORY, GrnTestHashFactoryClass))

typedef struct _GrnTestHashFactory         GrnTestHashFactory;
typedef struct _GrnTestHashFactoryClass    GrnTestHashFactoryClass;

struct _GrnTestHashFactory
{
  GObject object;
};

struct _GrnTestHashFactoryClass
{
  GObjectClass parent_class;
};

#define GRN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE 64

typedef enum
{
  GRN_TEST_HASH_FACTORY_ERROR_CONTEXT_NULL,
  GRN_TEST_HASH_FACTORY_ERROR_NULL,
  GRN_TEST_HASH_FACTORY_ERROR_CURSOR_NULL
} GrnTestHashFactoryError;

typedef enum
{
  GRN_TEST_HASH_FLAG_NONE = 0L << 0,
  GRN_TEST_HASH_FLAG_TINY = GRN_HASH_TINY,
  GRN_TEST_HASH_FLAG_KEY_STRING = GRN_OBJ_KEY_VAR_SIZE
} GrnTestHashFlags;

typedef enum
{
  GRN_TEST_ENCODING_DEFAULT = GRN_ENC_DEFAULT,
  GRN_TEST_ENCODING_NONE = GRN_ENC_NONE,
  GRN_TEST_ENCODING_EUC_JP = GRN_ENC_EUC_JP,
  GRN_TEST_ENCODING_UTF8 = GRN_ENC_UTF8,
  GRN_TEST_ENCODING_SJIS = GRN_ENC_SJIS,
  GRN_TEST_ENCODING_LATIN1 = GRN_ENC_LATIN1,
  GRN_TEST_ENCODING_KOI8R = GRN_ENC_KOI8R
} GrnTestEncoding;

typedef enum
{
  GRN_TEST_CONTEXT_FLAG_NONE = 0 << 0,
  GRN_TEST_CONTEXT_FLAG_USE_QL = GRN_CTX_USE_QL,
  GRN_TEST_CONTEXT_FLAG_BATCH_MODE = GRN_CTX_BATCH_MODE
} GrnTestContextFlags;

typedef enum
{
  GRN_TEST_CURSOR_FLAG_NONE = 0 << 0,
  GRN_TEST_CURSOR_FLAG_DESCENDING = GRN_CURSOR_DESCENDING,
  GRN_TEST_CURSOR_FLAG_ASCENDING = GRN_CURSOR_ASCENDING,
  GRN_TEST_CURSOR_FLAG_GE = GRN_CURSOR_GE,
  GRN_TEST_CURSOR_FLAG_GT = GRN_CURSOR_GT,
  GRN_TEST_CURSOR_FLAG_LE = GRN_CURSOR_LE,
  GRN_TEST_CURSOR_FLAG_LT = GRN_CURSOR_LT
} GrnTestCursorFlags;

GQuark              grn_test_hash_factory_error_quark (void);

GType               grn_test_hash_factory_get_type (void) G_GNUC_CONST;

GrnTestHashFactory *grn_test_hash_factory_new      (void);

grn_hash           *grn_test_hash_factory_open     (GrnTestHashFactory *factory,
                                                    GError **error);
grn_hash           *grn_test_hash_factory_create   (GrnTestHashFactory *factory,
                                                    GError **error);

grn_hash_cursor    *grn_test_hash_factory_open_cursor
                                                   (GrnTestHashFactory *factory,
                                                    GError **error);

grn_ctx            *grn_test_hash_factory_get_context
                                                   (GrnTestHashFactory *factory);
GrnTestContextFlags grn_test_hash_factory_get_context_flags
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_context_flags
                                                   (GrnTestHashFactory *factory,
                                                    GrnTestContextFlags flags);
grn_logger_info    *grn_test_hash_factory_get_logger
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_logger
                                                   (GrnTestHashFactory *factory,
                                                    grn_logger_info *logger);
const gchar        *grn_test_hash_factory_get_path (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_path (GrnTestHashFactory *factory,
                                                    const gchar *path);
guint32             grn_test_hash_factory_get_key_size
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_key_size
                                                   (GrnTestHashFactory *factory,
                                                    guint32 key_size);
guint32             grn_test_hash_factory_get_value_size
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_value_size
                                                   (GrnTestHashFactory *factory,
                                                    guint32 value_size);
GrnTestHashFlags    grn_test_hash_factory_get_flags(GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_flags(GrnTestHashFactory *factory,
                                                    GrnTestHashFlags flags);
void                grn_test_hash_factory_add_flags(GrnTestHashFactory *factory,
                                                    GrnTestHashFlags flags);
GrnTestEncoding     grn_test_hash_factory_get_encoding
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_encoding
                                                   (GrnTestHashFactory *factory,
                                                    GrnTestEncoding encoding);

const gchar        *grn_test_hash_factory_get_cursor_min
                                                   (GrnTestHashFactory *factory);
guint32             grn_test_hash_factory_get_cursor_min_size
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_cursor_min
                                                   (GrnTestHashFactory *factory,
                                                    const gchar *min,
                                                    guint32 size);
const gchar        *grn_test_hash_factory_get_cursor_max
                                                   (GrnTestHashFactory *factory);
guint32             grn_test_hash_factory_get_cursor_max_size
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_cursor_max
                                                   (GrnTestHashFactory *factory,
                                                    const gchar *max,
                                                    guint32 size);
GrnTestCursorFlags  grn_test_hash_factory_get_cursor_flags
                                                   (GrnTestHashFactory *factory);
void                grn_test_hash_factory_set_cursor_flags
                                                   (GrnTestHashFactory *factory,
                                                    GrnTestCursorFlags flags);
void                grn_test_hash_factory_add_cursor_flags
                                                   (GrnTestHashFactory *factory,
                                                    GrnTestCursorFlags flags);



G_END_DECLS

#endif
