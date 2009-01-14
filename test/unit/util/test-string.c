/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <groonga.h>

#include <groonga_in.h>
#include <stdlib.h>
#include <str.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/sen-assertions.h"

void test_normalize_utf8(void);
void test_charlen_nonnull_broken_utf8(void);

#define BUFFER_SIZE 4096
static gchar normalized_text[BUFFER_SIZE];
static const gchar text_ja_utf8[] =
  "Sennaは組み込み型の全文検索エンジンです。DBMSやスクリプト言語処理系等に\n"
  "組み込むことによって、その全文検索機能を強化することができます。n-gram\n"
  "インデックスと単語インデックスの特徴を兼ね備えた、高速かつ高精度な転置\n"
  "インデックスタイプのエンジンです。コンパクトな実装ですが、大規模な文書\n"
  "量と検索要求を処理できるように設計されています。また、純粋なn-gramイン\n"
  "デックスの作成も可能です。";
static const gchar normalized_text_ja_utf8[] =
  "sennaは組み込み型の全文検索エンジンです。dbmsやスクリプト言語処理系等に"
  "組み込むことによって、その全文検索機能を強化することができます。n-gram"
  "インデックスと単語インデックスの特徴を兼ね備えた、高速かつ高精度な転置"
  "インデックスタイプのエンジンです。コンパクトな実装ですが、大規模な文書"
  "量と検索要求を処理できるように設計されています。また、純粋なn-gramイン"
  "デックスの作成も可能です。";

void
test_normalize_utf8(void)
{
  int flags;

  flags = SEN_STR_WITH_CHECKS | SEN_STR_WITH_CTYPES;
  cut_assert_equal_int(strlen(normalized_text_ja_utf8),
                       sen_str_normalize(text_ja_utf8,
                                         strlen(text_ja_utf8),
                                         sen_enc_utf8,
                                         flags,
                                         normalized_text,
                                         BUFFER_SIZE));
  cut_assert_equal_string(normalized_text_ja_utf8, normalized_text);
}

void
test_charlen_nonnull_broken_utf8(void)
{
  const gchar utf8[] = "あ";

  cut_assert_equal_uint(0,
                        sen_str_charlen_nonnull(utf8, utf8 + 1, sen_enc_utf8));
}
