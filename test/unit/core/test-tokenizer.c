/*
  Copyright (C) 2012-2014  Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>
#include <groonga/tokenizer.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void data_is_tokenized_delimiter(void);
void test_is_tokenized_delimiter(gconstpointer data);
void data_have_tokenized_delimiter(void);
void test_have_tokenized_delimiter(gconstpointer data);

static grn_ctx context;
static grn_obj *db;

void
setup (void)
{
  grn_ctx_init(&context, 0);
  db = grn_db_create(&context, NULL, NULL);
}

void
teardown (void)
{
  grn_obj_unlink(&context, db);
  grn_ctx_fin(&context);
}

void
data_is_tokenized_delimiter(void)
{
#define ADD_DATUM(label, expected, input, encoding)                     \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "input",    G_TYPE_STRING,  input,                     \
                 "encoding", G_TYPE_INT,     encoding,                  \
                 NULL)

  ADD_DATUM("U+FFFE (UTF-8)",     GRN_TRUE,  "\xEF\xBF\xBE", GRN_ENC_UTF8);
  ADD_DATUM("U+FFFE (EUC-JP)",    GRN_FALSE, "\xEF\xBF\xBE", GRN_ENC_EUC_JP);
  ADD_DATUM("U+FFFE (Shift_JIS)", GRN_FALSE, "\xEF\xBF\xBE", GRN_ENC_SJIS);
  ADD_DATUM("U+FFFE (NONE)",      GRN_FALSE, "\xEF\xBF\xBE", GRN_ENC_NONE);
  ADD_DATUM("U+FFFE (LATIN1)",    GRN_FALSE, "\xEF\xBF\xBE", GRN_ENC_LATIN1);
  ADD_DATUM("U+FFFE (KOI8R)",     GRN_FALSE, "\xEF\xBF\xBE", GRN_ENC_KOI8R);

  ADD_DATUM("U+FFFF",             GRN_FALSE, "\xEF\xBF\xBF", GRN_ENC_UTF8);

#undef ADD_DATUM
}

void
test_is_tokenized_delimiter(gconstpointer data)
{
  const gchar *input;
  grn_encoding encoding;

  encoding = gcut_data_get_int(data, "encoding");
  GRN_CTX_SET_ENCODING(&context, encoding);
  input = gcut_data_get_string(data, "input");
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_tokenizer_is_tokenized_delimiter(&context,
                                                         input, strlen(input),
                                                         encoding));
  } else {
    cut_assert_false(grn_tokenizer_is_tokenized_delimiter(&context,
                                                          input, strlen(input),
                                                          encoding));
  }
}

void
data_have_tokenized_delimiter(void)
{
#define ADD_DATUM(label, expected, input)                               \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "input",    G_TYPE_STRING,  input,                     \
                 NULL)

#define UFFFE_IN_UTF8 "\xef\xbf\xbe"

  ADD_DATUM("have",     GRN_TRUE,  "a" UFFFE_IN_UTF8 "b");
  ADD_DATUM("not have", GRN_FALSE, "ab");

#undef UFFFE_IN_UTF8

#undef ADD_DATUM
}

void
test_have_tokenized_delimiter(gconstpointer data)
{
  const gchar *input;
  grn_encoding encoding = GRN_ENC_UTF8;

  GRN_CTX_SET_ENCODING(&context, encoding);
  input = gcut_data_get_string(data, "input");
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_tokenizer_have_tokenized_delimiter(&context,
                                                           input, strlen(input),
                                                           encoding));
  } else {
    cut_assert_false(grn_tokenizer_have_tokenized_delimiter(&context,
                                                            input, strlen(input),
                                                            encoding));
  }
}
