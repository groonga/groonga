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

void data_normalize(void);
void test_normalize(gconstpointer data);
void data_normalize_broken(void);
void test_normalize_broken(gconstpointer data);
void test_normalize_utf8_short_strlen(void);
void test_normalize_utf8_null_str(void);
void test_normalize_utf8_euc(void);
void test_normalize_utf8_sjis(void);
void test_charlen_utf8_nonnull_broken(void);
void test_charlen_utf8_null_broken(gpointer data);
void test_urldec(void);
void test_urldec_invalid(void);
void test_cgidec(void);
void test_cgidec_invalid(void);

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
test_charlen_utf8_nonnull_broken(void)
{
  const gchar utf8[] = "あ";
  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);
  cut_assert_equal_uint(0, grn_charlen(&context, utf8, utf8 + 1));
}

void
test_charlen_utf8_null_broken(gpointer data)
{
  const gchar utf8[] = "\0";
  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);
  cut_assert_equal_uint(0, grn_charlen(&context, utf8, utf8 + 1));
}

void
test_urldec(void)
{
  grn_obj buf;
  const char *str = "+%e6%97%a5%e6%9c%ac%e8%aa%9e%e3%81%a7%e3%81%99%e3%80%82+$yo";

  GRN_TEXT_INIT(&buf, 0);
  grn_text_urldec(&context, &buf, str, strchr(str, 0), '$');
  GRN_TEXT_PUTC(&context, &buf, '\0');
  cut_assert_equal_string("+日本語です。+", GRN_TEXT_VALUE(&buf));
  grn_obj_unlink(&context, &buf);
}

void
test_urldec_invalid(void)
{
  grn_obj buf;
  const char *str = "%1%2%3";

  GRN_TEXT_INIT(&buf, 0);
  grn_text_urldec(&context, &buf, str, strchr(str, 0), '\0');
  GRN_TEXT_PUTC(&context, &buf, '\0');
  cut_assert_equal_string("%1%2%3", GRN_TEXT_VALUE(&buf));
  grn_obj_unlink(&context, &buf);
}

void
test_cgidec(void)
{
  grn_obj buf;
  const char *str = "+%e6%97%a5%e6%9c%ac%e8%aa%9e%e3%81%a7%e3%81%99%e3%80%82+$yo";

  GRN_TEXT_INIT(&buf, 0);
  grn_text_cgidec(&context, &buf, str, strchr(str, 0), '$');
  GRN_TEXT_PUTC(&context, &buf, '\0');
  cut_assert_equal_string(" 日本語です。 ", GRN_TEXT_VALUE(&buf));
  grn_obj_unlink(&context, &buf);
}

void
test_cgidec_invalid(void)
{
  grn_obj buf;
  const char *str = "%1%2%3";

  GRN_TEXT_INIT(&buf, 0);
  grn_text_cgidec(&context, &buf, str, strchr(str, 0), '\0');
  GRN_TEXT_PUTC(&context, &buf, '\0');
  cut_assert_equal_string("%1%2%3", GRN_TEXT_VALUE(&buf));
  grn_obj_unlink(&context, &buf);
}

void
test_url_path_normalize(void)
{
  char buf[1024];
  const char *str = "/a/b/../c/d/././e";

  grn_str_url_path_normalize(&context, str, strlen(str), buf, 1024);
  cut_assert_equal_string("/a/c/d/e", buf);
}

void
test_url_path_normalize_above_parent(void)
{
  char buf[1024];
  const char *str = "/a/../../b";

  grn_str_url_path_normalize(&context, str, strlen(str), buf, 1024);
  /* NOTE: not in GRN_API_ENTER, rc is not seted */
  grn_test_assert_error(GRN_SUCCESS,
                        "parent path doesn't exist.",
                        &context);
}

void
test_text_otoj(void)
{
  /* TODO: json spec. */
  grn_obj obj, buf;
  const char str_pattern[] = "\"'\\aAzZ09 \n\t\r日本語",
             str_esc_pattern[] = "\"\\\"'\\\\aAzZ09 \\n\\t\\r日本語\"";

  cut_omit("grn_text_otoj for obj of GRN_VOID type may be null.");

  GRN_TEXT_INIT(&buf, 0);

  /* SHORT_TEXT */
  GRN_SHORT_TEXT_INIT(&obj, 0);
  GRN_TEXT_PUTS(&context, &obj, str_pattern);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &obj, '\0');
  cut_assert_equal_string(str_esc_pattern, GRN_TEXT_VALUE(&buf));
  GRN_BULK_REWIND(&buf); grn_obj_unlink(&context, &obj);

  /* TEXT */
  GRN_TEXT_INIT(&obj, 0);
  GRN_TEXT_PUTS(&context, &obj, str_pattern);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &obj, '\0');
  cut_assert_equal_string(str_esc_pattern, GRN_TEXT_VALUE(&buf));
  GRN_BULK_REWIND(&buf); grn_obj_unlink(&context, &obj);

  /* LONG_TEXT */
  GRN_LONG_TEXT_INIT(&obj, 0);
  GRN_TEXT_PUTS(&context, &obj, str_pattern);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &obj, '\0');
  cut_assert_equal_string(str_esc_pattern, GRN_TEXT_VALUE(&buf));
  GRN_BULK_REWIND(&buf); grn_obj_unlink(&context, &obj);

  /* VOID */
  GRN_VOID_INIT(&obj);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* BOOL */
  GRN_BOOL_INIT(&obj, 0);
  GRN_BOOL_SET(&context, &obj, 1);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_BOOL_SET(&context, &obj, 0);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* INT8 */
  GRN_INT8_INIT(&obj, 0);
  GRN_INT8_SET(&context, &obj, INT8_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_INT8_SET(&context, &obj, INT8_MIN);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* UINT8 */
  GRN_UINT8_INIT(&obj, 0);
  GRN_UINT8_SET(&context, &obj, UINT8_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_UINT8_SET(&context, &obj, 0);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* INT16 */
  GRN_INT16_INIT(&obj, 0);
  GRN_INT16_SET(&context, &obj, INT16_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_INT16_SET(&context, &obj, INT16_MIN);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* UINT16 */
  GRN_UINT16_INIT(&obj, 0);
  GRN_UINT16_SET(&context, &obj, UINT16_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_UINT16_SET(&context, &obj, 0);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* INT32 */
  GRN_INT32_INIT(&obj, 0);
  GRN_INT32_SET(&context, &obj, INT32_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_INT32_SET(&context, &obj, INT32_MIN);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* UINT32 */
  GRN_UINT32_INIT(&obj, 0);
  GRN_UINT32_SET(&context, &obj, UINT32_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_UINT32_SET(&context, &obj, 0);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* INT64 */
  GRN_INT64_INIT(&obj, 0);
  GRN_INT64_SET(&context, &obj, INT64_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_INT64_SET(&context, &obj, INT64_MIN);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* UINT64 */
  GRN_UINT64_INIT(&obj, 0);
  GRN_UINT64_SET(&context, &obj, UINT64_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_UINT64_SET(&context, &obj, 0);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, ' ');
  /* TIME: TIME is same as INT64  */
  GRN_TIME_INIT(&obj, 0);
  GRN_TIME_SET(&context, &obj, INT64_MAX);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  GRN_TEXT_PUTC(&context, &buf, ' ');
  GRN_TIME_SET(&context, &obj, INT64_MIN);
  grn_test_assert(grn_text_otoj(&context, &buf, &obj, NULL));
  grn_obj_unlink(&context, &obj);
  GRN_TEXT_PUTC(&context, &buf, '\0');

  cut_assert_equal_string(cut_take_printf(
    "%s "
    "%s %s "
    "%" PRId8 " %" PRId8 " "
    "%" PRIu8 " %" PRIu8 " "
    "%" PRId16 " %" PRId16 " "
    "%" PRIu16 " %" PRIu16 " "
    "%" PRId32 " %" PRId32 " "
    "%" PRIu32 " %" PRIu32 " "
    "%" PRId64 " %" PRId64 " "
    "%" PRIu64 " %" PRIu64 " "
    "%#.15g %#.15g", /* Time sec float*/
    "null",
    "true", "false",
    INT8_MAX, INT8_MIN,
    UINT8_MAX, (uint8_t)0,
    INT16_MAX, INT16_MIN,
    UINT16_MAX, (uint16_t)0,
    INT32_MAX, INT32_MIN,
    UINT32_MAX, (uint32_t)0,
    INT64_MAX, INT64_MIN,
    UINT64_MAX, (uint64_t)0,
    (double)INT64_MAX / GRN_TIME_USEC_PER_SEC, (double)INT64_MIN / GRN_TIME_USEC_PER_SEC
  ), GRN_TEXT_VALUE(&buf));

  /* FIXME: FLOAT */

  /* FIXME: create macro for TOKYO_GEO_POINT/WGS84_GEO_POINT */
  /* FIXME* unknown bulk */
  /* FIXME: GRN_UVECTOR */
  /* FIXME: GRN_VECTOR */
  /* FIXME: table with format */
  /* FIXME: table without format */
  /* FIXME: grn_text_atoj */

  grn_obj_unlink(&context, &buf);
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
