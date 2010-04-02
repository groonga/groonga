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
void test_normalize(gpointer data);
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
#define ADD_DATUM(label, expected, input)               \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_STRING, expected,   \
                 "input", G_TYPE_STRING, input,         \
                 NULL)

  ADD_DATUM("with newlines",
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

  ADD_DATUM("large normalization",
            "キロメートルキロメートルキロメートルキロメートル",
            "㌖㌖㌖㌖");

  ADD_DATUM("tilde and fullwidth tilde and wave dash",
            "~~~",
            "~～〜");

#undef ADD_DATUM
}

/* TODO: clean up for multiple encoding */
/* TODO: support all encoding supported by groonga */
void
test_normalize(gpointer data)
{
  const gchar *expected, *input;
  gchar *conv_input, *conv_input_check, *conv_expected;
  grn_str *string;
  const gchar *normalized_text;
  guint normalized_text_len;
  int flags;

  /* utf8 */
  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);
  flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;
  input = gcut_data_get_string(data, "input");
  string = grn_str_open(&context, input, strlen(input), flags);
  normalized_text = cut_take_strndup(string->norm, string->norm_blen);
  normalized_text_len = string->norm_blen;
  grn_test_assert(grn_str_close(&context, string));

  expected = gcut_data_get_string(data, "expected");
  cut_assert_equal_string(expected, normalized_text);
  cut_assert_equal_int(strlen(expected), normalized_text_len);

  /* Shift-JIS */
  GRN_CTX_SET_ENCODING(&context, GRN_ENC_SJIS);
  flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;
  input = gcut_data_get_string(data, "input");
  conv_input = g_convert(input, -1, "CP932", "UTF-8", NULL, NULL, NULL);
  if (conv_input) {
    conv_input_check = g_convert(conv_input, -1, "UTF-8", "CP932", NULL, NULL, NULL);
    if (conv_input_check) {
      if (!strcmp(input, conv_input_check)) {
        string = grn_str_open(&context, conv_input, strlen(conv_input), flags);
        normalized_text = cut_take_strndup(string->norm, string->norm_blen);
        normalized_text_len = string->norm_blen;
        grn_test_assert(grn_str_close(&context, string));

        expected = gcut_data_get_string(data, "expected");
        conv_expected = g_convert_with_fallback(expected, -1, "CP932", "UTF-8", NULL, NULL, NULL, NULL);
        cut_assert_equal_string(conv_expected, normalized_text);
        cut_assert_equal_int(strlen(conv_expected), normalized_text_len);
        g_free(conv_expected);
      }
      g_free(conv_input_check);
    }
    g_free(conv_input);
  }
}

void
test_normalize_utf8_short_strlen(void)
{
  grn_str *string;
  const gchar *utf8;
  int flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;

  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);

  utf8 = "あ";
  string = grn_str_open(&context, utf8, 1, flags);
  cut_assert_equal_string("", string->norm);
  cut_assert_equal_int(string->norm_blen, 0);
  grn_test_assert(grn_str_close(&context, string));
}

void
test_normalize_utf8_null_str(void)
{
  grn_str *string;
  const gchar *utf8;
  int flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;

  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);

  utf8 = "\0";
  string = grn_str_open(&context, utf8, 1, flags);
  cut_assert_equal_string("", string->norm);
  cut_assert_equal_int(string->norm_blen, 0);
  grn_test_assert(grn_str_close(&context, string));
}

void
test_normalize_utf8_euc(void)
{
  grn_str *string;
  const gchar *utf8;
  int flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;

  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);

  utf8 = "\xC6\xFC\xCB\xDC\xB8\xEC"; /* 日本語 on EUC */
  string = grn_str_open(&context, utf8, strlen(utf8), flags);
  cut_assert_equal_string("", string->norm);
  cut_assert_equal_int(string->norm_blen, 0);
  grn_test_assert(grn_str_close(&context, string));
}

void
test_normalize_utf8_sjis(void)
{
  grn_str *string;
  const gchar *utf8;
  int flags = GRN_STR_NORMALIZE | GRN_STR_WITH_CHECKS | GRN_STR_WITH_CTYPES;

  GRN_CTX_SET_ENCODING(&context, GRN_ENC_UTF8);

  utf8 = "\x93\xFA\x96\x7B\x8C\xEA"; /* 日本語 on ShiftJIS */
  cut_assert_equal_uint(0, grn_charlen(&context, utf8, strchr(utf8, 0)));
  string = grn_str_open(&context, utf8, strlen(utf8), flags);
  cut_assert_equal_string("", string->norm);
  cut_assert_equal_int(string->norm_blen, 0);
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
  grn_text_urldec(&context, &buf, str, strchr(str, 0), '\0');
  GRN_TEXT_PUTC(&context, &buf, '\0');
  cut_assert_equal_string("%1%2%3", GRN_TEXT_VALUE(&buf));
  grn_obj_unlink(&context, &buf);
}
