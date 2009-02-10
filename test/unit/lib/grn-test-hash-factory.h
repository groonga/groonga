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

#ifndef __SEN_TEST_HASH_FACTORY_H__
#define __SEN_TEST_HASH_FACTORY_H__

#include <hash.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define SEN_TEST_HASH_FACTORY_ERROR           (sen_test_hash_factory_error_quark())

#define SEN_TYPE_TEST_HASH_FACTORY            (sen_test_hash_factory_get_type())
#define SEN_TEST_HASH_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SEN_TYPE_TEST_HASH_FACTORY, SenTestHashFactory))
#define SEN_TEST_HASH_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SEN_TYPE_TEST_HASH_FACTORY, SenTestHashFactoryClass))
#define SEN_IS_TEST_HASH_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SEN_TYPE_TEST_HASH_FACTORY))
#define SEN_IS_TEST_HASH_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SEN_TYPE_TEST_HASH_FACTORY))
#define SEN_TEST_HASH_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), SEN_TYPE_TEST_HASH_FACTORY, SenTestHashFactoryClass))

typedef struct _SenTestHashFactory         SenTestHashFactory;
typedef struct _SenTestHashFactoryClass    SenTestHashFactoryClass;

struct _SenTestHashFactory
{
  GObject object;
};

struct _SenTestHashFactoryClass
{
  GObjectClass parent_class;
};

#define SEN_TEST_HASH_FACTORY_DEFAULT_VALUE_SIZE 64

typedef enum
{
  SEN_TEST_HASH_FACTORY_ERROR_CONTEXT_NULL,
  SEN_TEST_HASH_FACTORY_ERROR_NULL,
  SEN_TEST_HASH_FACTORY_ERROR_CURSOR_NULL
} SenTestHashFactoryError;

typedef enum
{
  SEN_TEST_HASH_FLAG_NONE = 0L << 0,
  SEN_TEST_HASH_FLAG_TINY = SEN_HASH_TINY,
  SEN_TEST_HASH_FLAG_KEY_STRING = SEN_OBJ_KEY_VAR_SIZE
} SenTestHashFlags;

typedef enum
{
  SEN_TEST_ENCODING_DEFAULT = sen_enc_default,
  SEN_TEST_ENCODING_NONE = sen_enc_none,
  SEN_TEST_ENCODING_EUC_JP = sen_enc_euc_jp,
  SEN_TEST_ENCODING_UTF8 = sen_enc_utf8,
  SEN_TEST_ENCODING_SJIS = sen_enc_sjis,
  SEN_TEST_ENCODING_LATIN1 = sen_enc_latin1,
  SEN_TEST_ENCODING_KOI8R = sen_enc_koi8r
} SenTestEncoding;

typedef enum
{
  SEN_TEST_CONTEXT_FLAG_NONE = 0 << 0,
  SEN_TEST_CONTEXT_FLAG_USE_QL = SEN_CTX_USE_QL,
  SEN_TEST_CONTEXT_FLAG_BATCH_MODE = SEN_CTX_BATCH_MODE
} SenTestContextFlags;

typedef enum
{
  SEN_TEST_CURSOR_FLAG_NONE = 0 << 0,
  SEN_TEST_CURSOR_FLAG_DESCENDING = SEN_CURSOR_DESCENDING,
  SEN_TEST_CURSOR_FLAG_ASCENDING = SEN_CURSOR_ASCENDING,
  SEN_TEST_CURSOR_FLAG_GE = SEN_CURSOR_GE,
  SEN_TEST_CURSOR_FLAG_GT = SEN_CURSOR_GT,
  SEN_TEST_CURSOR_FLAG_LE = SEN_CURSOR_LE,
  SEN_TEST_CURSOR_FLAG_LT = SEN_CURSOR_LT
} SenTestCursorFlags;

GQuark              sen_test_hash_factory_error_quark (void);

GType               sen_test_hash_factory_get_type (void) G_GNUC_CONST;

SenTestHashFactory *sen_test_hash_factory_new      (void);

sen_hash           *sen_test_hash_factory_open     (SenTestHashFactory *factory,
                                                    GError **error);
sen_hash           *sen_test_hash_factory_create   (SenTestHashFactory *factory,
                                                    GError **error);

sen_hash_cursor    *sen_test_hash_factory_open_cursor
                                                   (SenTestHashFactory *factory,
                                                    GError **error);

sen_ctx            *sen_test_hash_factory_get_context
                                                   (SenTestHashFactory *factory);
SenTestContextFlags sen_test_hash_factory_get_context_flags
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_context_flags
                                                   (SenTestHashFactory *factory,
                                                    SenTestContextFlags flags);
sen_logger_info    *sen_test_hash_factory_get_logger
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_logger
                                                   (SenTestHashFactory *factory,
                                                    sen_logger_info *logger);
const gchar        *sen_test_hash_factory_get_path (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_path (SenTestHashFactory *factory,
                                                    const gchar *path);
guint32             sen_test_hash_factory_get_key_size
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_key_size
                                                   (SenTestHashFactory *factory,
                                                    guint32 key_size);
guint32             sen_test_hash_factory_get_value_size
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_value_size
                                                   (SenTestHashFactory *factory,
                                                    guint32 value_size);
SenTestHashFlags    sen_test_hash_factory_get_flags(SenTestHashFactory *factory);
void                sen_test_hash_factory_set_flags(SenTestHashFactory *factory,
                                                    SenTestHashFlags flags);
void                sen_test_hash_factory_add_flags(SenTestHashFactory *factory,
                                                    SenTestHashFlags flags);
SenTestEncoding     sen_test_hash_factory_get_encoding
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_encoding
                                                   (SenTestHashFactory *factory,
                                                    SenTestEncoding encoding);

const gchar        *sen_test_hash_factory_get_cursor_min
                                                   (SenTestHashFactory *factory);
guint32             sen_test_hash_factory_get_cursor_min_size
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_cursor_min
                                                   (SenTestHashFactory *factory,
                                                    const gchar *min,
                                                    guint32 size);
const gchar        *sen_test_hash_factory_get_cursor_max
                                                   (SenTestHashFactory *factory);
guint32             sen_test_hash_factory_get_cursor_max_size
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_cursor_max
                                                   (SenTestHashFactory *factory,
                                                    const gchar *max,
                                                    guint32 size);
SenTestCursorFlags  sen_test_hash_factory_get_cursor_flags
                                                   (SenTestHashFactory *factory);
void                sen_test_hash_factory_set_cursor_flags
                                                   (SenTestHashFactory *factory,
                                                    SenTestCursorFlags flags);
void                sen_test_hash_factory_add_cursor_flags
                                                   (SenTestHashFactory *factory,
                                                    SenTestCursorFlags flags);



G_END_DECLS

#endif
