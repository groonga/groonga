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

#include <grn.h>
#include <groonga.h>

#include <stdlib.h>
#include <grn_str.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_normalize(void);
void test_normalize(gconstpointer data);
void data_normalize_broken(void);
void test_normalize_broken(gconstpointer data);
void test_normalize_without_database(void);
void data_remove_tokenized_delimiter(void);
void test_remove_tokenized_delimiter(gconstpointer data);
void data_charlen_broken(void);
void test_charlen_broken(gconstpointer data);
void data_urlenc(void);
void test_urlenc(gconstpointer data);
void data_urldec(void);
void test_urldec(gconstpointer data);
void data_cgidec(void);
void test_cgidec(gconstpointer data);
void data_url_path_normalize(void);
void test_url_path_normalize(gconstpointer data);
void data_url_path_normalize_invalid(void);
void test_url_path_normalize_invalid(gconstpointer data);
void data_text_otoj(void);
void test_text_otoj(gconstpointer data);
void data_str_len(void);
void test_str_len(gconstpointer data);
void data_aton(void);
void test_aton(gconstpointer data);
void data_atoull(void);
void test_atoull(gconstpointer data);
void data_itoh(void);
void test_itoh(gconstpointer data);

static grn_ctx context;
static grn_obj *database;
static grn_obj buffer;

void
setup (void)
{
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, NULL, NULL);
  GRN_VOID_INIT(&buffer);
}

void
teardown (void)
{
  GRN_OBJ_FIN(&context, &buffer);
  if (database) {
    grn_obj_close(&context, database);
  }
  grn_ctx_fin(&context);
}

void
data_normalize(void)
{
#define ADD_DATUM_WITH_ENCODING(label, expected, input, encoding)       \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 "encoding", G_TYPE_INT, encoding,                      \
                 NULL)
#define ADD_DATUM_JAPANESE(label, expected, input)                      \
  ADD_DATUM_WITH_ENCODING("Japanese (UTF-8): " label " <" input ">",    \
                          expected, input, GRN_ENC_UTF8);               \
  ADD_DATUM_WITH_ENCODING("Japanese (eucJP): " label " <" input ">",    \
                          expected, input, GRN_ENC_EUC_JP);             \
  ADD_DATUM_WITH_ENCODING("Japanese (Shift_JIS): " label " <" input ">",\
                          expected, input, GRN_ENC_SJIS)

#define ADD_DATUM_JAPANESE_NO_SJIS(label, expected, input)              \
  ADD_DATUM_WITH_ENCODING("Japanese (UTF-8): " label " <" input ">",\
                          expected, input, GRN_ENC_UTF8);               \
  ADD_DATUM_WITH_ENCODING("Japanese (eucJP): " label " <" input ">",    \
                          expected, input, GRN_ENC_EUC_JP)

#define ADD_DATUM_JAPANESE_UTF_8(label, expected, input)                \
  ADD_DATUM_WITH_ENCODING("Japanese (UTF-8): " label " <" input ">",    \
                          expected, input, GRN_ENC_UTF8)

  ADD_DATUM_JAPANESE("with newlines",
                     "groongaは組み込み型の全文検索エンジンです。"
                     "dbmsやスクリプト言語処理系等に組み込むこと"
                     "によって、その全文検索機能を強化することが"
                     "できます。n-gramインデックスと単語インデッ"
                     "クスの特徴を兼ね備えた、高速かつ高精度な転"
                     "置インデックスタイプのエンジンです。コンパ"
                     "クトな実装ですが、大規模な文書量と検索要求"
                     "を処理できるように設計されています。また、"
                     "純粋なn-gramインデックスの作成も可能です。",

                     "groongaは組み込み型の全文検索エンジンです。\n"
                     "DBMSやスクリプト言語処理系等に組み込むこと\n"
                     "によって、その全文検索機能を強化することが\n"
                     "できます。n-gramインデックスと単語インデッ\n"
                     "クスの特徴を兼ね備えた、高速かつ高精度な転\n"
                     "置インデックスタイプのエンジンです。コンパ\n"
                     "クトな実装ですが、大規模な文書量と検索要求\n"
                     "を処理できるように設計されています。また、\n"
                     "純粋なn-gramインデックスの作成も可能です。");

  ADD_DATUM_JAPANESE_UTF_8("large normalization",
                           "キロメートルキロメートルキロメートルキロメートル",
                           "㌖㌖㌖㌖");

  ADD_DATUM_JAPANESE_UTF_8("tilde and fullwidth tilde and wave dash",
                           "~~~",
                           "~～〜");

#undef ADD_DATUM_JAPANESE_NO_SJIS
#undef ADD_DATUM_JAPANESE
#undef ADD_DATUM_WITH_ENCODING
}

static const gchar *
convert_encoding(const gchar *utf8, grn_encoding encoding)
{
  const gchar *encoded = NULL;
  GError *error = NULL;

  switch (encoding) {
  case GRN_ENC_DEFAULT:
  case GRN_ENC_NONE:
  case GRN_ENC_UTF8:
    encoded = utf8;
    break;
  case GRN_ENC_EUC_JP:
    encoded = cut_take_string(g_convert(utf8, -1, "eucJP", "UTF-8",
                                        NULL, NULL, &error));
    break;
  case GRN_ENC_SJIS:
    encoded = cut_take_string(g_convert(utf8, -1, "CP932", "UTF-8",
                                        NULL, NULL, &error));
    break;
  case GRN_ENC_LATIN1:
    encoded = cut_take_string(g_convert(utf8, -1, "CP1252", "UTF-8",
                                        NULL, NULL, &error));
    break;
  case GRN_ENC_KOI8R:
    encoded = cut_take_string(g_convert(utf8, -1, "KOI8-R", "UTF-8",
                                        NULL, NULL, &error));
    break;
  }
  gcut_assert_error(error);

  return encoded;
}

void
test_normalize(gconstpointer data)
{
  const gchar *utf8_expected, *encoded_expected;
  const gchar *utf8_input, *encoded_input;
  grn_obj *string;
  const gchar *normalized_text;
  guint normalized_text_length;
  guint normalized_text_n_characters;
  int flags;
  grn_encoding encoding;

  encoding = gcut_data_get_int(data, "encoding");
  GRN_CTX_SET_ENCODING(&context, encoding);
  flags = GRN_STRING_WITH_CHECKS | GRN_STRING_WITH_TYPES;
  utf8_input = gcut_data_get_string(data, "input");
  encoded_input = convert_encoding(utf8_input, encoding);
  string = grn_string_open(&context,
                           encoded_input,
                           strlen(encoded_input),
                           GRN_NORMALIZER_AUTO,
                           flags);
  grn_string_get_normalized(&context, string,
                            &normalized_text,
                            &normalized_text_length,
                            &normalized_text_n_characters);
  normalized_text = cut_take_strndup(normalized_text, normalized_text_length);
  grn_obj_unlink(&context, string);

  utf8_expected = gcut_data_get_string(data, "expected");
  encoded_expected = convert_encoding(utf8_expected, encoding);
  cut_assert_equal_string(encoded_expected, normalized_text);
  cut_assert_equal_uint(strlen(encoded_expected), normalized_text_length);
  cut_assert_equal_uint(g_utf8_strlen(utf8_expected, -1),
                        normalized_text_n_characters);
}

void
data_normalize_broken(void)
{
#define ADD_DATUM(label, input, input_encoding, input_length,           \
                  context_encoding)                                     \
  gcut_add_datum(label,                                                 \
                 "input", G_TYPE_STRING, input,                         \
                 "input-encoding", G_TYPE_INT, input_encoding,          \
                 "input-length", G_TYPE_INT, input_length,              \
                 "context-encoding", G_TYPE_INT, context_encoding,      \
                 NULL)

  ADD_DATUM("short", "あ", GRN_ENC_UTF8, 1, GRN_ENC_UTF8);
  ADD_DATUM("NULL", "\0", GRN_ENC_UTF8, 1, GRN_ENC_UTF8);

#define ADD_DATUM_JAPANESE_NON_UTF8(label, input, input_length)         \
  ADD_DATUM("eucJP with UTF-8 context: " label " <" input ">",          \
            input, GRN_ENC_EUC_JP, input_length, GRN_ENC_UTF8);         \
  ADD_DATUM("ShiftJIS with UTF-8 context : " label " <" input ">",      \
            input, GRN_ENC_SJIS, input_length, GRN_ENC_UTF8);

  ADD_DATUM_JAPANESE_NON_UTF8("different encoding", "日本語", -1);

#undef ADD_DATUM_JAPANESE_NON_UTF8

#undef ADD_DATUM
}

void
test_normalize_broken(gconstpointer data)
{
  grn_obj *string;
  const gchar *input, *encoded_input;
  const gchar *normalized_text;
  grn_encoding input_encoding, context_encoding;
  gint input_length;
  guint normalized_text_length, normalized_text_n_characters;
  int flags = GRN_STRING_WITH_CHECKS | GRN_STRING_WITH_TYPES;

  context_encoding = gcut_data_get_int(data, "context-encoding");
  GRN_CTX_SET_ENCODING(&context, context_encoding);

  input = gcut_data_get_string(data, "input");
  input_encoding = gcut_data_get_int(data, "input-encoding");
  input_length = gcut_data_get_int(data, "input-length");
  encoded_input = convert_encoding(input, input_encoding);
  if (input_length < 0) {
    input_length = strlen(encoded_input);
  }
  string = grn_string_open(&context, encoded_input, input_length,
                           GRN_NORMALIZER_AUTO, flags);
  grn_string_get_normalized(&context, string,
                            &normalized_text,
                            &normalized_text_length,
                            &normalized_text_n_characters);
  normalized_text = cut_take_strndup(normalized_text, normalized_text_length);
  grn_obj_unlink(&context, string);

  cut_assert_equal_string("", normalized_text);
  cut_assert_equal_int(0, normalized_text_length);
  cut_assert_equal_int(0, normalized_text_n_characters);
}

void
test_normalize_without_database(void)
{
  grn_obj *string;
  const char *input = "Groonga";
  int flags = 0;

  grn_obj_close(&context, database);
  database = NULL;

  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);
  string = grn_string_open(&context,
                           input,
                           strlen(input),
                           GRN_NORMALIZER_AUTO,
                           flags);
  cut_assert_null(string);
  grn_test_assert_error(GRN_INVALID_ARGUMENT,
                        "[string][open] "
                        "NormalizerAuto normalizer isn't available",
                        &context);
}

void
data_remove_tokenized_delimiter(void)
{
#define ADD_DATUM(label, expected, input, flags)                        \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

#define UFFFE_IN_UTF8 "\xef\xbf\xbe"

  ADD_DATUM("normalize",
            "abあい",
            UFFFE_IN_UTF8 "A"
            UFFFE_IN_UTF8 "B"
            UFFFE_IN_UTF8 "あ"
            UFFFE_IN_UTF8 "い"
            UFFFE_IN_UTF8,
            GRN_OBJ_KEY_NORMALIZE);
  ADD_DATUM("not normalize",
            "ABあい",
            UFFFE_IN_UTF8 "A"
            UFFFE_IN_UTF8 "B"
            UFFFE_IN_UTF8 "あ"
            UFFFE_IN_UTF8 "い"
            UFFFE_IN_UTF8,
            0);

#undef UFFFE_IN_UTF8

#undef ADD_DATUM
}

void
test_remove_tokenized_delimiter(gconstpointer data)
{
  grn_obj *string;
  grn_obj *normalizer = NULL;
  const gchar *expected;
  const gchar *input;
  const gchar *normalized;
  unsigned int length_in_bytes;
  int flags = GRN_STRING_REMOVE_TOKENIZED_DELIMITER;

  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);

  input = gcut_data_get_string(data, "input");
  flags |= gcut_data_get_int(data, "flags");
  if (flags & GRN_OBJ_KEY_NORMALIZE) {
    normalizer = GRN_NORMALIZER_AUTO;
  }

  string = grn_string_open(&context, input, strlen(input), normalizer, flags);
  grn_string_get_normalized(&context, string,
                            &normalized, &length_in_bytes, NULL);
  normalized = cut_take_strndup(normalized, length_in_bytes);
  grn_obj_unlink(&context, string);

  expected = gcut_data_get_string(data, "expected");
  cut_assert_equal_string(expected, normalized);
}

void
data_charlen_broken(void)
{
#define ADD_DATUM_WITH_ENCODING(label, input, input_length, encoding)   \
  gcut_add_datum(label,                                                 \
                 "input", G_TYPE_STRING, input,                         \
                 "input-length", G_TYPE_INT, input_length,              \
                 "encoding", G_TYPE_INT, encoding,                      \
                 NULL)

#define ADD_DATUM(label, input, input_length)                           \
  ADD_DATUM_WITH_ENCODING("(None): " label " <" input ">",              \
                          input, input_length, GRN_ENC_NONE);           \
  ADD_DATUM_WITH_ENCODING("(UTF-8): " label " <" input ">",             \
                          input, input_length, GRN_ENC_UTF8);           \
  ADD_DATUM_WITH_ENCODING("(eucJP): " label " <" input ">",             \
                          input, input_length, GRN_ENC_EUC_JP);         \
  ADD_DATUM_WITH_ENCODING("(Shift_JIS): " label " <" input ">",         \
                          input, input_length, GRN_ENC_SJIS);           \
  ADD_DATUM_WITH_ENCODING("(Latin1): " label " <" input ">",            \
                          input, input_length, GRN_ENC_LATIN1);         \
  ADD_DATUM_WITH_ENCODING("(KOI8R): " label " <" input ">",             \
                          input, input_length, GRN_ENC_KOI8R)

#define ADD_DATUM_JAPANESE(label, input, input_length)                  \
  ADD_DATUM_WITH_ENCODING("Japanese (UTF-8): " label " <" input ">",    \
                          input, input_length, GRN_ENC_UTF8);           \
  ADD_DATUM_WITH_ENCODING("Japanese (eucJP): " label " <" input ">",    \
                          input, input_length, GRN_ENC_EUC_JP);         \
  ADD_DATUM_WITH_ENCODING("Japanese (Shift_JIS): " label " <" input ">",\
                          input, input_length, GRN_ENC_SJIS)

  ADD_DATUM_JAPANESE("short length", "あ", 1);

  ADD_DATUM_WITH_ENCODING("NULL", "\0", 1, GRN_ENC_UTF8);

#undef ADD_DATUM_JAPANESE
#undef ADD_DATUM
#undef ADD_DATUM_WITH_ENCODING
}

void
test_charlen_broken(gconstpointer data)
{
  const gchar *input, *encoded_input, *encoded_input_end;
  grn_encoding encoding;
  gint input_length;

  encoding = gcut_data_get_int(data, "encoding");
  GRN_CTX_SET_ENCODING(&context, encoding);

  input = gcut_data_get_string(data, "input");
  input_length = gcut_data_get_int(data, "input-length");
  encoded_input = convert_encoding(input, encoding);
  if (input_length < 0) {
    input_length = strlen(encoded_input);
  }
  encoded_input_end = encoded_input + input_length;
  cut_assert_equal_uint(0, grn_charlen(&context,
                                       encoded_input,
                                       encoded_input_end));
}

void
data_urlenc(void)
{
#define ADD_DATUM(label, expected, input, input_length)                 \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 "input-length", G_TYPE_INT, input_length,              \
                 NULL)

  ADD_DATUM("Japanese",
            "%20%E6%97%A5%E6%9C%AC%E8%AA%9E%E3%81%A7%E3%81%99%E3%80%82%20",
            " 日本語です。 ",
            -1);
  ADD_DATUM("percent", "%251%252%253", "%1%2%3", -1);
  ADD_DATUM("plus", "%2B%20%2B", "+ +", -1);

#undef ADD_DATUM
}

void
test_urlenc(gconstpointer data)
{
  grn_obj buffer;
  const gchar *expected, *input;
  gint input_length;

  expected = gcut_data_get_string(data, "expected");
  input = gcut_data_get_string(data, "input");
  input_length = gcut_data_get_int(data, "input-length");

  if (input_length < 0) {
    input_length = strchr(input, '\0') - input;
  }

  GRN_TEXT_INIT(&buffer, 0);
  grn_text_urlenc(&context, &buffer, input, input_length);
  cut_assert_equal_substring(expected,
                             GRN_TEXT_VALUE(&buffer),
                             GRN_TEXT_LEN(&buffer));
  GRN_OBJ_FIN(&context, &buffer);
}

void
data_urldec(void)
{
#define ADD_DATUM(label, expected, input, input_length, end_char)       \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 "input-length", G_TYPE_INT, input_length,              \
                 "end-char", G_TYPE_CHAR, end_char,                     \
                 NULL)

  ADD_DATUM("Japanese",
            "+日本語です。+",
            "+%e6%97%a5%e6%9c%ac%e8%aa%9e%e3%81%a7%e3%81%99%e3%80%82+$yo",
            -1,
            '$');
  ADD_DATUM("invalid", "%1%2%3", "%1%2%3", -1, '\0');

#undef ADD_DATUM
}

void
test_urldec(gconstpointer data)
{
  grn_obj buffer;
  const gchar *expected, *input;
  gint input_length;
  gchar end_char;

  expected = gcut_data_get_string(data, "expected");
  input = gcut_data_get_string(data, "input");
  input_length = gcut_data_get_int(data, "input-length");
  end_char = gcut_data_get_char(data, "end-char");

  if (input_length < 0) {
    input_length = strchr(input, '\0') - input;
  }

  GRN_TEXT_INIT(&buffer, 0);
  grn_text_urldec(&context, &buffer, input, input + input_length, end_char);
  cut_assert_equal_substring(expected,
                             GRN_TEXT_VALUE(&buffer),
                             GRN_TEXT_LEN(&buffer));
  grn_obj_unlink(&context, &buffer);
}

void
data_cgidec(void)
{
#define ADD_DATUM(label, expected, input, input_length, delimiter)      \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 "input-length", G_TYPE_INT, input_length,              \
                 "delimiter", G_TYPE_STRING, delimiter,                 \
                 NULL)

  ADD_DATUM("?", "/d/select?table=users&limit=10", "/d/select", -1, "?");
  ADD_DATUM("&", "table=users&limit=10", "table=users", -1, "&;");
  ADD_DATUM(";", "table=users;limit=10", "table=users", -1, "&;");
  ADD_DATUM("Japanese",
            " 日本語です。 ",
            "+%e6%97%a5%e6%9c%ac%e8%aa%9e%e3%81%a7%e3%81%99%e3%80%82+$yo",
            -1,
            "$");
  ADD_DATUM("invalid", "%1%2%3", "%1%2%3", -1, "");

#undef ADD_DATUM
}

void
test_cgidec(gconstpointer data)
{
  grn_obj buffer;
  const gchar *expected, *input;
  gint input_length;
  const gchar *delimiter;

  expected = gcut_data_get_string(data, "expected");
  input = gcut_data_get_string(data, "input");
  input_length = gcut_data_get_int(data, "input-length");
  delimiter = gcut_data_get_string(data, "delimiter");

  if (input_length < 0) {
    input_length = strchr(input, '\0') - input;
  }

  GRN_TEXT_INIT(&buffer, 0);
  grn_text_cgidec(&context, &buffer, input, input + input_length, delimiter);
  cut_assert_equal_substring(expected,
                             GRN_TEXT_VALUE(&buffer),
                             GRN_TEXT_LEN(&buffer));
  grn_obj_unlink(&context, &buffer);
}

void
data_url_path_normalize(void)
{
#define ADD_DATUM(label, expected, input)                               \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 NULL)

  ADD_DATUM("no '.' and '..'",
            "/a/b/c/",
            "/a/b/c/");
  ADD_DATUM("with '.' and '..'",
            "/a/c/d/e",
            "/a/b/../c/d/././e");

#undef ADD_DATUM
}

void
test_url_path_normalize(gconstpointer data)
{
#define BUFFER_SIZE 1024
  gchar buffer[BUFFER_SIZE];
  const gchar *expected, *input;

  expected = gcut_data_get_string(data, "expected");
  input = gcut_data_get_string(data, "input");

  grn_str_url_path_normalize(&context, input, strlen(input),
                             buffer, BUFFER_SIZE);
  cut_assert_equal_string(expected, buffer);
#undef BUFFER_SIZE
}

void
data_url_path_normalize_invalid(void)
{
#define ADD_DATUM(label, error_message, input)                          \
  gcut_add_datum(label,                                                 \
                 "error-message", G_TYPE_STRING, error_message,         \
                 "input", G_TYPE_STRING, input,                         \
                 NULL)

  ADD_DATUM("too many '..'",
            "parent path doesn't exist.",
            "/a/../../b");

#undef ADD_DATUM
}

void
test_url_path_normalize_invalid(gconstpointer data)
{
#define BUFFER_SIZE 1024
  gchar buffer[BUFFER_SIZE];
  const gchar *error_message, *input;

  error_message = gcut_data_get_string(data, "error-message");
  input = gcut_data_get_string(data, "input");

  grn_str_url_path_normalize(&context, input, strlen(input),
                             buffer, BUFFER_SIZE);
  grn_test_assert_error(GRN_INVALID_ARGUMENT, error_message, &context);
#undef BUFFER_SIZE
}

void
data_text_otoj(void)
{
#define ADD_DATUM(label, expected, type, ...)                   \
  gcut_add_datum(label,                                         \
                 "expected", G_TYPE_STRING, expected,           \
                 "type", G_TYPE_INT, type,                      \
                 __VA_ARGS__);

  ADD_DATUM("Void", "", GRN_DB_VOID, NULL);
  ADD_DATUM("Bool", "true", GRN_DB_BOOL,
            "value", G_TYPE_BOOLEAN, TRUE,
            NULL);
  ADD_DATUM("Bool", "false", GRN_DB_BOOL,
            "value", G_TYPE_BOOLEAN, FALSE,
            NULL);
  ADD_DATUM("Int8 (min)", cut_take_printf("%d", INT8_MIN), GRN_DB_INT8,
            "value", G_TYPE_INT, INT8_MIN,
            NULL);
  ADD_DATUM("Int8 (max)", cut_take_printf("%d", INT8_MAX), GRN_DB_INT8,
            "value", G_TYPE_INT, INT8_MAX,
            NULL);
  ADD_DATUM("UInt8 (min)", "0", GRN_DB_UINT8,
            "value", G_TYPE_UINT, 0,
            NULL);
  ADD_DATUM("UInt8 (max)", cut_take_printf("%u", UINT8_MAX), GRN_DB_UINT8,
            "value", G_TYPE_UINT, UINT8_MAX,
            NULL);
  ADD_DATUM("Int16 (min)", cut_take_printf("%d", INT16_MIN), GRN_DB_INT16,
            "value", G_TYPE_INT, INT16_MIN,
            NULL);
  ADD_DATUM("Int16 (max)", cut_take_printf("%d", INT16_MAX), GRN_DB_INT16,
            "value", G_TYPE_INT, INT16_MAX,
            NULL);
  ADD_DATUM("UInt16 (min)", "0", GRN_DB_UINT16,
            "value", G_TYPE_UINT, 0,
            NULL);
  ADD_DATUM("UInt16 (max)", cut_take_printf("%u", UINT16_MAX), GRN_DB_UINT16,
            "value", G_TYPE_UINT, UINT16_MAX,
            NULL);
  ADD_DATUM("Int32 (min)", cut_take_printf("%d", INT32_MIN), GRN_DB_INT32,
            "value", G_TYPE_INT, INT32_MIN,
            NULL);
  ADD_DATUM("Int32 (max)", cut_take_printf("%d", INT32_MAX), GRN_DB_INT32,
            "value", G_TYPE_INT, INT32_MAX,
            NULL);
  ADD_DATUM("UInt32 (min)", "0", GRN_DB_UINT32,
            "value", G_TYPE_UINT, 0,
            NULL);
  ADD_DATUM("UInt32 (max)", cut_take_printf("%u", UINT32_MAX), GRN_DB_UINT32,
            "value", G_TYPE_UINT, UINT32_MAX,
            NULL);
  ADD_DATUM("Int64 (min)",
            cut_take_printf("%" G_GINT64_FORMAT, INT64_MIN), GRN_DB_INT64,
            "value", G_TYPE_INT64, INT64_MIN,
            NULL);
  ADD_DATUM("Int64 (max)",
            cut_take_printf("%" G_GINT64_FORMAT, INT64_MAX), GRN_DB_INT64,
            "value", G_TYPE_INT64, INT64_MAX,
            NULL);
  ADD_DATUM("UInt64 (min)", "0", GRN_DB_UINT64,
            "value", G_TYPE_UINT64, G_GUINT64_CONSTANT(0),
            NULL);
  ADD_DATUM("UInt64 (max)",
            cut_take_printf("%" G_GUINT64_FORMAT, UINT64_MAX), GRN_DB_UINT64,
            "value", G_TYPE_UINT64, UINT64_MAX,
            NULL);
  ADD_DATUM("Float", cut_take_printf("%g", 2.9), GRN_DB_FLOAT,
            "value", G_TYPE_DOUBLE, 2.9,
            NULL);
  ADD_DATUM("Time", "1271053050.211479", GRN_DB_TIME,
            "value", G_TYPE_INT64, GRN_TIME_PACK(1271053050, 211479),
            NULL);
  ADD_DATUM("ShortText",
            "\"\\\"'\\\\aAzZ09 \\n\\t\\r日本語\"", GRN_DB_SHORT_TEXT,
            "value", G_TYPE_STRING, "\"'\\aAzZ09 \n\t\r日本語",
            NULL);
  ADD_DATUM("Text",
            "\"\\\"'\\\\aAzZ09 \\n\\t\\r日本語\"", GRN_DB_TEXT,
            "value", G_TYPE_STRING, "\"'\\aAzZ09 \n\t\r日本語",
            NULL);
  ADD_DATUM("LongText",
            "\"\\\"'\\\\aAzZ09 \\n\\t\\r日本語\"", GRN_DB_LONG_TEXT,
            "value", G_TYPE_STRING, "\"'\\aAzZ09 \n\t\r日本語",
            NULL);
  ADD_DATUM("TokyoGeoPoint", "\"35681396x139766049\"", GRN_DB_TOKYO_GEO_POINT,
            "latitude", G_TYPE_INT, 35681396,
            "longitude", G_TYPE_INT, 139766049,
            NULL);
  ADD_DATUM("WGS84GeoPoint", "\"36032548x140164867\"", GRN_DB_WGS84_GEO_POINT,
            "latitude", G_TYPE_INT, 36032548,
            "longitude", G_TYPE_INT, 140164867,
            NULL);

  /* FIXME* unknown bulk */
  /* FIXME: GRN_UVECTOR */
  /* FIXME: GRN_VECTOR */
  /* FIXME: table with format */
  /* FIXME: table without format */
  /* FIXME: grn_text_atoj */

#undef ADD_DATUM
}

static void
construct_object(gconstpointer data, grn_builtin_type type, grn_obj *object)
{
  switch (type) {
  case GRN_DB_VOID:
    GRN_VOID_INIT(object);
    break;
  case GRN_DB_BOOL:
    GRN_BOOL_INIT(object, 0);
    {
      bool value = gcut_data_get_boolean(data, "value");
      GRN_BOOL_SET(&context, object, value);
    }
    break;
  case GRN_DB_INT8:
    GRN_INT8_INIT(object, 0);
    GRN_INT8_SET(&context, object, gcut_data_get_int(data, "value"));
    break;
  case GRN_DB_UINT8:
    GRN_UINT8_INIT(object, 0);
    GRN_UINT8_SET(&context, object, gcut_data_get_uint(data, "value"));
    break;
  case GRN_DB_INT16:
    GRN_INT16_INIT(object, 0);
    GRN_INT16_SET(&context, object, gcut_data_get_int(data, "value"));
    break;
  case GRN_DB_UINT16:
    GRN_UINT16_INIT(object, 0);
    GRN_UINT16_SET(&context, object, gcut_data_get_uint(data, "value"));
    break;
  case GRN_DB_INT32:
    GRN_INT32_INIT(object, 0);
    GRN_INT32_SET(&context, object, gcut_data_get_int(data, "value"));
    break;
  case GRN_DB_UINT32:
    GRN_UINT32_INIT(object, 0);
    GRN_UINT32_SET(&context, object, gcut_data_get_uint(data, "value"));
    break;
  case GRN_DB_INT64:
    GRN_INT64_INIT(object, 0);
    GRN_INT64_SET(&context, object, gcut_data_get_int64(data, "value"));
    break;
  case GRN_DB_UINT64:
    GRN_UINT64_INIT(object, 0);
    GRN_UINT64_SET(&context, object, gcut_data_get_uint64(data, "value"));
    break;
  case GRN_DB_FLOAT:
    GRN_FLOAT_INIT(object, 0);
    GRN_FLOAT_SET(&context, object, gcut_data_get_double(data, "value"));
    break;
  case GRN_DB_TIME:
    GRN_TIME_INIT(object, 0);
    GRN_TIME_SET(&context, object, gcut_data_get_int64(data, "value"));
    break;
  case GRN_DB_SHORT_TEXT:
    GRN_SHORT_TEXT_INIT(object, 0);
    GRN_TEXT_SETS(&context, object, gcut_data_get_string(data, "value"));
    break;
  case GRN_DB_TEXT:
    GRN_TEXT_INIT(object, 0);
    GRN_TEXT_SETS(&context, object, gcut_data_get_string(data, "value"));
    break;
  case GRN_DB_LONG_TEXT:
    GRN_LONG_TEXT_INIT(object, 0);
    GRN_TEXT_SETS(&context, object, gcut_data_get_string(data, "value"));
    break;
  case GRN_DB_TOKYO_GEO_POINT:
    GRN_TOKYO_GEO_POINT_INIT(object, 0);
    GRN_GEO_POINT_SET(&context, object,
                      gcut_data_get_int(data, "latitude"),
                      gcut_data_get_int(data, "longitude"));
    break;
  case GRN_DB_WGS84_GEO_POINT:
    GRN_WGS84_GEO_POINT_INIT(object, 0);
    GRN_GEO_POINT_SET(&context, object,
                      gcut_data_get_int(data, "latitude"),
                      gcut_data_get_int(data, "longitude"));
    break;
  default:
    cut_fail("unknown type: %d", type);
    break;
  }
}

void
test_text_otoj(gconstpointer data)
{
  grn_obj object, json;
  grn_builtin_type type;
  const gchar *expected, *actual;

  GRN_TEXT_INIT(&json, 0);

  expected = gcut_data_get_string(data, "expected");
  type = gcut_data_get_int(data, "type");
  cut_trace(construct_object(data, type, &object));
  grn_text_otoj(&context, &json, &object, NULL);
  grn_obj_unlink(&context, &object);
  actual = cut_take_printf("%.*s",
                           (int)GRN_TEXT_LEN(&json), GRN_TEXT_VALUE(&json));
  grn_obj_unlink(&context, &json);
  cut_assert_equal_string(expected, actual);
}

void
data_str_len(void)
{
#define ADD_DATUM(label, expected, input, encoding)     \
  gcut_add_datum(label,                                 \
                 "expected", GCUT_TYPE_SIZE, expected,  \
                 "input", G_TYPE_STRING, input,         \
                 "encoding", G_TYPE_INT, encoding,      \
                 NULL)

#define ADD_DATUM_ALL_ENCODING(label, expected, input)  \
  ADD_DATUM(label " (none) <" input ">",                \
            expected, input, GRN_ENC_NONE);             \
  ADD_DATUM(label " (EUC-JP) <" input ">",              \
            expected, input, GRN_ENC_EUC_JP);           \
  ADD_DATUM(label " (UTF-8) <" input ">",               \
            expected, input, GRN_ENC_UTF8);             \
  ADD_DATUM(label " (Shift_JIS) <" input ">",           \
            expected, input, GRN_ENC_SJIS);             \
  ADD_DATUM(label " (Latin1) <" input ">",              \
            expected, input, GRN_ENC_LATIN1);           \
  ADD_DATUM(label " (KOI8R) <" input ">",               \
            expected, input, GRN_ENC_KOI8R);

#define ADD_DATUM_JAPANESE(label, expected, input)                      \
  ADD_DATUM("Japanese: " label " (EUC-JP) <" input ">",                 \
            expected, cut_take_convert(input, "eucJP", "UTF-8"),        \
            GRN_ENC_EUC_JP);                                            \
  ADD_DATUM("Japanese: " label " (UTF-8) <" input ">",                  \
            expected, input, GRN_ENC_UTF8);                             \
  ADD_DATUM("Japanese: " label " (Shift_JIS) <" input ">",              \
            expected, cut_take_convert(input, "CP932", "UTF-8"),        \
            GRN_ENC_SJIS);

  ADD_DATUM_ALL_ENCODING("half width", 11, "ABC! & ABC!");

  ADD_DATUM_JAPANESE("with newlines",
                     209,
                     "groongaは組み込み型の全文検索エンジンです。\n"
                     "DBMSやスクリプト言語処理系等に組み込むこと\n"
                     "によって、その全文検索機能を強化することが\n"
                     "できます。n-gramインデックスと単語インデッ\n"
                     "クスの特徴を兼ね備えた、高速かつ高精度な転\n"
                     "置インデックスタイプのエンジンです。コンパ\n"
                     "クトな実装ですが、大規模な文書量と検索要求\n"
                     "を処理できるように設計されています。また、\n"
                     "純粋なn-gramインデックスの作成も可能です。");

#undef ADD_DATUM_JAPANESE
#undef ADD_DATUM_ALL_ENCODING
#undef ADD_DATUM
}

void
test_str_len(gconstpointer data)
{
  size_t result, expected;
  const gchar *input;
  const char *input_end;
  grn_encoding encoding;

  input = gcut_data_get_string(data, "input");
  input_end = strchr(input, '\0');
  encoding = gcut_data_get_int(data, "encoding");
  result = grn_str_len(&context, input, input_end, encoding, NULL);
  expected = gcut_data_get_size(data, "expected");
  cut_assert_equal_size(expected, result);
}

void
data_aton(void)
{
#define ADD_DATUM(label, grn_type, g_type, expected, input)     \
  gcut_add_datum(label " - " #expected,                         \
                 "type", G_TYPE_INT, grn_type,                  \
                 "expected", g_type, expected,                  \
                 "input", G_TYPE_STRING, input,                 \
                 NULL)

  ADD_DATUM("int32",
            GRN_DB_INT32,
            G_TYPE_INT, 344494643,
            "+344494643");
  ADD_DATUM("int32 - negative",
            GRN_DB_INT32,
            G_TYPE_INT, -344494643,
            "-344494643");
  ADD_DATUM("uint32",
            GRN_DB_UINT32,
            G_TYPE_UINT, (guint32)G_GUINT64_CONSTANT(2147483648),
            "2147483648");
  ADD_DATUM("int64",
            GRN_DB_INT64,
            G_TYPE_INT64, G_GINT64_CONSTANT(344494643000000),
            "344494643000000");
  ADD_DATUM("int64 - negative",
            GRN_DB_INT64,
            G_TYPE_INT64, G_GINT64_CONSTANT(-344494643000000),
            "-344494643000000");
  /* TODO: support uint64.
  ADD_DATUM("uint64",
            GRN_DB_UINT64,
            G_TYPE_UINT64, G_GUINT64_CONSTANT(9223372036854775808),
            "9223372036854775808");
  */
  ADD_DATUM("float",
            GRN_DB_FLOAT,
            G_TYPE_DOUBLE, 3.44494643e14,
            "3.44494643e14");

#undef ADD_DATUM
}

void
test_aton(gconstpointer data)
{
  const gchar *input, *input_end, *rest;
  grn_builtin_type type;
  grn_rc rc;

  type = gcut_data_get_int(data, "type");
  input = gcut_data_get_string(data, "input");
  input_end = strchr(input, '\0');
  rc = grn_aton(&context, input, input_end, &rest, &buffer);
  grn_test_assert(rc);
  cut_assert_equal_string(input_end, rest);
  cut_assert_equal_int(type, buffer.header.domain);
  switch (type) {
  case GRN_DB_INT32 :
    cut_assert_equal_int(gcut_data_get_int(data, "expected"),
                         GRN_INT32_VALUE(&buffer));
    break;
  case GRN_DB_UINT32 :
    cut_assert_equal_uint(gcut_data_get_uint(data, "expected"),
                          GRN_UINT32_VALUE(&buffer));
    break;
  case GRN_DB_INT64 :
    gcut_assert_equal_int64(gcut_data_get_int64(data, "expected"),
                            GRN_INT64_VALUE(&buffer));
    break;
  case GRN_DB_UINT64 :
    gcut_assert_equal_uint64(gcut_data_get_uint64(data, "expected"),
                             GRN_UINT64_VALUE(&buffer));
    break;
  case GRN_DB_FLOAT :
    cut_assert_equal_double(gcut_data_get_double(data, "expected"),
                            0.000001,
                            GRN_FLOAT_VALUE(&buffer));
    break;
  default :
    cut_error("unknown type: %d", type);
    break;
  }
}

void
data_atoull(void)
{
#define ADD_DATUM(label, expected, input)                       \
  gcut_add_datum(label " - " #expected,                         \
                 "expected", G_TYPE_UINT64, expected,           \
                 "input", G_TYPE_STRING, input,                 \
                 NULL)

  ADD_DATUM("uint32 max",
            G_GUINT64_CONSTANT(4294967295),
            "4294967295");
  ADD_DATUM("int64 max",
            G_GUINT64_CONSTANT(344494643000000),
            "344494643000000");
  ADD_DATUM("uint64 max",
            G_GUINT64_CONSTANT(18446744073709551615),
            "18446744073709551615");

#undef ADD_DATUM
}

void
test_atoull(gconstpointer data)
{
  const gchar *input, *input_end, *rest;
  uint64_t value;

  input = gcut_data_get_string(data, "input");
  input_end = strchr(input, '\0');
  value = grn_atoull(input, input_end, &rest);
  cut_assert_equal_string(input_end, rest);
  gcut_assert_equal_uint64(gcut_data_get_uint64(data, "expected"),
                           value);
}

void
data_itoh(void)
{
#define ADD_DATUM(label, expected, input, length)                       \
  gcut_add_datum(label " - " #input " [" #length "] (" expected ")",    \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_UINT, input,                           \
                 "length", G_TYPE_UINT, length,                         \
                 NULL)

  ADD_DATUM("no alphabet",
            "9", 9, 1);
  ADD_DATUM("alphabet",
            "A", 10, 1);
  ADD_DATUM("many number of digits",
            "0013579BDF", 324508639, 10);

#undef ADD_DATUM
}

void
test_itoh(gconstpointer data)
{
  const gchar *expected;
  gchar *actual;
  guint input;
  guint length;

  input = gcut_data_get_uint(data, "input");
  length = gcut_data_get_uint(data, "length");
  expected = gcut_data_get_string(data, "expected");

  actual = g_new0(gchar, length);
  cut_take(actual, g_free);
  grn_itoh(input, actual, length);
  cut_assert_equal_substring(expected, actual, length);
}
