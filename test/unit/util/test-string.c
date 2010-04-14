/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <groonga.h>

#include <groonga_in.h>
#include <stdlib.h>
#include <str.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define GRN_TOKYO_GEO_POINT_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_TOKYO_GEO_POINT)
#define GRN_WGS84_GEO_POINT_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_WGS84_GEO_POINT)

#define GRN_GEO_POINT_PACK(latitude,longitude) \
  (((long long unsigned int)(latitude)) + \
   (((long long unsigned int)(longitude)) << 32))
#define GRN_GEO_POINT_SET(ctx,obj,latitude,longitude) \
  GRN_UINT64_SET(ctx, obj, GRN_GEO_POINT_PACK(latitude, longitude))

void data_normalize(void);
void test_normalize(gconstpointer data);
void data_normalize_broken(void);
void test_normalize_broken(gconstpointer data);
void data_charlen_broken(void);
void test_charlen_broken(gconstpointer data);
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

static grn_ctx context;

static const gchar text_ja_utf8[] =
  "Groongaは組み込み型の全文検索エンジンです。DBMSやスクリプト言語処理系等に\n"
  "組み込むことによって、その全文検索機能を強化することができます。n-gram\n"
  "インデックスと単語インデックスの特徴を兼ね備えた、高速かつ高精度な転置\n"
  "インデックスタイプのエンジンです。コンパクトな実装ですが、大規模な文書\n"
  "量と検索要求を処理できるように設計されています。また、純粋なn-gramイン\n"
  "デックスの作成も可能です。";

static const gchar normalized_text_ja_utf8[] =
  "groongaは組み込み型の全文検索エンジンです。dbmsやスクリプト言語処理系等に"
  "組み込むことによって、その全文検索機能を強化することができます。n-gram"
  "インデックスと単語インデックスの特徴を兼ね備えた、高速かつ高精度な転置"
  "インデックスタイプのエンジンです。コンパクトな実装ですが、大規模な文書"
  "量と検索要求を処理できるように設計されています。また、純粋なn-gramイン"
  "デックスの作成も可能です。";

void
setup (void)
{
  grn_ctx_init(&context, GRN_CTX_USE_QL);
}

void
teardown (void)
{
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
  const gchar *encoded;
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
  grn_str *string;
  const gchar *normalized_text;
  guint normalized_text_len;
  int flags;
  grn_encoding encoding;

  encoding = gcut_data_get_int(data, "encoding");
  GRN_CTX_SET_ENCODING(&context, encoding);
  flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;
  utf8_input = gcut_data_get_string(data, "input");
  encoded_input = convert_encoding(utf8_input, encoding);
  string = grn_str_open(&context, encoded_input, strlen(encoded_input), flags);
  normalized_text = cut_take_strndup(string->norm, string->norm_blen);
  normalized_text_len = string->norm_blen;
  grn_test_assert(grn_str_close(&context, string));

  utf8_expected = gcut_data_get_string(data, "expected");
  encoded_expected = convert_encoding(utf8_expected, encoding);
  cut_assert_equal_string(encoded_expected, normalized_text);
  cut_assert_equal_int(strlen(encoded_expected), normalized_text_len);
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
  grn_str *string;
  const gchar *input, *encoded_input;
  grn_encoding input_encoding, context_encoding;
  gint input_length;
  int flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;

  context_encoding = gcut_data_get_int(data, "context-encoding");
  GRN_CTX_SET_ENCODING(&context, context_encoding);

  input = gcut_data_get_string(data, "input");
  input_encoding = gcut_data_get_int(data, "input-encoding");
  input_length = gcut_data_get_int(data, "input-length");
  encoded_input = convert_encoding(input, input_encoding);
  if (input_length < 0) {
    input_length = strlen(encoded_input);
  }
  string = grn_str_open(&context, encoded_input, input_length, flags);
  cut_assert_equal_string("", string->norm);
  cut_assert_equal_int(0, string->norm_blen);
  grn_test_assert(grn_str_close(&context, string));
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
#define ADD_DATUM(label, expected, input, input_length, end_char)       \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "input", G_TYPE_STRING, input,                         \
                 "input-length", G_TYPE_INT, input_length,              \
                 "end-char", G_TYPE_CHAR, end_char,                     \
                 NULL)

  ADD_DATUM("Japanese",
            " 日本語です。 ",
            "+%e6%97%a5%e6%9c%ac%e8%aa%9e%e3%81%a7%e3%81%99%e3%80%82+$yo",
            -1,
            '$');
  ADD_DATUM("invalid", "%1%2%3", "%1%2%3", -1, '\0');

#undef ADD_DATUM
}

void
test_cgidec(gconstpointer data)
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
  grn_text_cgidec(&context, &buffer, input, input + input_length, end_char);
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
  /* NOTE: not in GRN_API_ENTER, rc is not set. */
  grn_test_assert_error(GRN_SUCCESS, error_message, &context);
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

  ADD_DATUM("Void", "null", GRN_DB_VOID, NULL);
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
  ADD_DATUM("Int64 (min)", cut_take_printf("%ld", INT64_MIN), GRN_DB_INT64,
            "value", G_TYPE_INT64, INT64_MIN,
            NULL);
  ADD_DATUM("Int64 (max)", cut_take_printf("%ld", INT64_MAX), GRN_DB_INT64,
            "value", G_TYPE_INT64, INT64_MAX,
            NULL);
  ADD_DATUM("UInt64 (min)", "0", GRN_DB_UINT64,
            "value", G_TYPE_UINT64, G_GUINT64_CONSTANT(0),
            NULL);
  ADD_DATUM("UInt64 (max)", cut_take_printf("%lu", UINT64_MAX), GRN_DB_UINT64,
            "value", G_TYPE_UINT64, UINT64_MAX,
            NULL);
  ADD_DATUM("Float", cut_take_printf("%g", 2.9), GRN_DB_FLOAT,
            "value", G_TYPE_DOUBLE, 2.9,
            NULL);
  ADD_DATUM("Time", "1271053050.21148", GRN_DB_TIME,
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
            "\"\\\"'\\\\aAzZ09 \\n\\t\\r日本語\"", GRN_DB_TEXT,
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
    GRN_BOOL_SET(&context, object, gcut_data_get_boolean(data, "value"));
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
  const gchar *expected;

  GRN_TEXT_INIT(&json, 0);

  expected = gcut_data_get_string(data, "expected");
  type = gcut_data_get_int(data, "type");
  cut_trace(construct_object(data, type, &object));
  grn_text_otoj(&context, &json, &object, NULL);
  grn_obj_unlink(&context, &object);
  cut_assert_equal_substring(expected,
                             GRN_TEXT_VALUE(&json), GRN_TEXT_LEN(&json));
  grn_obj_unlink(&context, &json);
}

void
data_str_len(void)
{
#define ADD_DATUM(label, expected, input)               \
  gcut_add_datum(label,                                 \
                 "expected", GCUT_TYPE_SIZE, expected,  \
                 "input", G_TYPE_STRING, input,         \
                 NULL)

  ADD_DATUM("halfwidth",
            11,
            "ABC! & ABC!");

  ADD_DATUM("with newlines",
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
#undef ADD_DATUM
}

/* TODO: support all encoding supported by groonga */
void
test_str_len(gpointer data)
{
  size_t result, expected;
  const gchar *input;
  const char *input_end;

  input = gcut_data_get_string(data, "input");
  input_end = strchr(input, '\0');
  result = grn_str_len(&context, input, GRN_ENC_UTF8, &input_end);
  expected = gcut_data_get_size(data, "expected");
  cut_assert_equal_size(expected, result);
}
