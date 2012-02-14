/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2012  Kouhei Sutou <kou@clear-code.com>

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

/*
  groonga: 5632ce3e39c0d8bf3c6b758d4cbf5e012cfa00b0
  CFLAGS: -O3
  CPU: Intel(R) Core(TM) i5 CPU         650  @ 3.20GHz
  % make -j8 > /dev/null && GROONGA_BENCH_N=10000 test/benchmark/bench-normalize

  groonga: 5632ce3e39c0d8bf3c6b758d4cbf5e012cfa00b0
  CFLAGS: -O3 -ggdb3
  CPU: Intel(R) Core(TM) i5 CPU         650  @ 3.20GHz
  % make -j8 > /dev/null && GROONGA_BENCH_N=10000 test/benchmark/bench-normalize
                  (time)
    NFKC: plugin: (3.05917)
    NFKC: bundle: (3.13312)
*/

#include <string.h>

#include <locale.h>

#include <db.h>
#include <groonga.h>
#include <groonga_in.h>

#define grn_nfkc_ctype bundle_grn_nfkc_ctype
#define grn_nfkc_map1 bundle_grn_nfkc_map1
#define grn_nfkc_map2 bundle_grn_nfkc_map2

#include "plugins/normalizers/nfkc-core.c"

#define GRN_STR_REMOVEBLANK  (0x01<<0)
#define GRN_STR_WITH_TYPES   (0x01<<1)
#define GRN_STR_WITH_CHECKS  (0x01<<2)

#define GRN_STR_BLANK 0x80

enum {
  grn_str_null = 0,
  grn_str_alpha,
  grn_str_digit,
  grn_str_symbol,
  grn_str_hiragana,
  grn_str_katakana,
  grn_str_kanji,
  grn_str_others
};

inline static grn_obj *
utf8_nfkc_normalize(grn_ctx *ctx, grn_str *nstr)
{
  int16_t *ch;
  const unsigned char *s, *s_, *s__ = NULL, *p, *p2, *pe, *e;
  unsigned char *d, *d_, *de;
  uint_least8_t *cp;
  size_t length = 0, ls, lp, size = nstr->orig_blen, ds = size * 3;
  int removeblankp = nstr->flags & GRN_STR_REMOVEBLANK;
  if (!(nstr->norm = GRN_MALLOC(ds + 1))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[normalizer][utf8][nfkc] failed to allocate normalized text space");
    return NULL;
  }
  if (nstr->flags & GRN_STR_WITH_CHECKS) {
    if (!(nstr->checks = GRN_MALLOC(ds * sizeof(int16_t) + 1))) {
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][utf8][nfkc] failed to allocate checks space");
      return NULL;
    }
  }
  ch = nstr->checks;
  if (nstr->flags & GRN_STR_WITH_CTYPES) {
    if (!(nstr->ctypes = GRN_MALLOC(ds + 1))) {
      if (nstr->checks) { GRN_FREE(nstr->checks); nstr->checks = NULL; }
      GRN_FREE(nstr->norm);
      nstr->norm = NULL;
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[normalizer][utf8][nfkc] failed to allocate character types space");
      return NULL;
    }
  }
  cp = nstr->ctypes;
  d = (unsigned char *)nstr->norm;
  de = d + ds;
  d_ = NULL;
  e = (unsigned char *)nstr->orig + size;
  for (s = s_ = (unsigned char *)nstr->orig; ; s += ls) {
    if (!(ls = grn_charlen_utf8(ctx, s, e))) {
      break;
    }
    if ((p = (unsigned char *)grn_nfkc_map1(s))) {
      pe = p + strlen((char *)p);
    } else {
      p = s;
      pe = p + ls;
    }
    if (d_ && (p2 = (unsigned char *)grn_nfkc_map2(d_, p))) {
      p = p2;
      pe = p + strlen((char *)p);
      if (cp) { cp--; }
      if (ch) {
        ch -= (d - d_);
        s_ = s__;
      }
      d = d_;
      length--;
    }
    for (; ; p += lp) {
      if (!(lp = grn_charlen_utf8(ctx, p, pe))) {
        break;
      }
      if ((*p == ' ' && removeblankp) || *p < 0x20  /* skip unprintable ascii */ ) {
        if (cp > nstr->ctypes) { *(cp - 1) |= GRN_STR_BLANK; }
      } else {
        if (de <= d + lp) {
          unsigned char *norm;
          ds += (ds >> 1) + lp;
          if (!(norm = GRN_REALLOC(nstr->norm, ds + 1))) {
            if (nstr->ctypes) { GRN_FREE(nstr->ctypes); nstr->ctypes = NULL; }
            if (nstr->checks) { GRN_FREE(nstr->checks); nstr->checks = NULL; }
            GRN_FREE(nstr->norm); nstr->norm = NULL;
            ERR(GRN_NO_MEMORY_AVAILABLE,
                "[normalizer][utf8][nfkc] "
                "failed to reallocate normalized text space");
            return NULL;
          }
          de = norm + ds;
          d = norm + (d - (unsigned char *)nstr->norm);
          nstr->norm = norm;
          if (ch) {
            int16_t *checks;
            if (!(checks = GRN_REALLOC(nstr->checks, ds * sizeof(int16_t)+ 1))) {
              if (nstr->ctypes) { GRN_FREE(nstr->ctypes); nstr->ctypes = NULL; }
              GRN_FREE(nstr->checks); nstr->checks = NULL;
              GRN_FREE(nstr->norm); nstr->norm = NULL;
              ERR(GRN_NO_MEMORY_AVAILABLE,
                  "[normalizer][utf8][nfkc] "
                  "failed to reallocate checks space");
              return NULL;
            }
            ch = checks + (ch - nstr->checks);
            nstr->checks = checks;
          }
          if (cp) {
            uint_least8_t *ctypes;
            if (!(ctypes = GRN_REALLOC(nstr->ctypes, ds + 1))) {
              GRN_FREE(nstr->ctypes); nstr->ctypes = NULL;
              if (nstr->checks) { GRN_FREE(nstr->checks); nstr->checks = NULL; }
              GRN_FREE(nstr->norm); nstr->norm = NULL;
              ERR(GRN_NO_MEMORY_AVAILABLE,
                  "[normalizer][utf8][nfkc] "
                  "failed to reallocate character types space");
              return NULL;
            }
            cp = ctypes + (cp - nstr->ctypes);
            nstr->ctypes = ctypes;
          }
        }
        memcpy(d, p, lp);
        d_ = d;
        d += lp;
        length++;
        if (cp) { *cp++ = grn_nfkc_ctype(p); }
        if (ch) {
          size_t i;
          if (s_ == s + ls) {
            *ch++ = -1;
          } else {
            *ch++ = (int16_t)(s + ls - s_);
            s__ = s_;
            s_ = s + ls;
          }
          for (i = lp; i > 1; i--) { *ch++ = 0; }
        }
      }
    }
  }
  if (cp) { *cp = grn_str_null; }
  *d = '\0';
  nstr->length = length;
  nstr->norm_blen = (size_t)(d - (unsigned char *)nstr->norm);
  return NULL;
}

#include "lib/benchmark.h"

#define GET(context, name) (grn_ctx_get(context, name, strlen(name)))

static const gchar *text =
"1. groongaの特徴\n"
"1.1. groonga の概要\n"
"\n"
"groonga は転置索引を用いた高速・高精度な全文検索エンジンであり、登録された文書をすぐに検索結果に反映できます。また、参照をブロックせずに更新できることから、即時更新の必要なアプリケーションにおいても高い性能を発揮します。\n"
"\n"
"全文検索エンジンとして開発された groonga ですが、独自のカラムストアを持つ列指向のデータベースとしての側面も持っています。そのため、MySQL や PostgreSQL など、既存の代表的なデータベースが苦手とする集計クエリを高速に処理できるという特徴があり、組み合わせによって弱点を補うような使い方もできます。\n"
"\n"
"groonga の基本機能は C ライブラリとして提供されていますが、MySQL や PostgreSQL と連携させたり、Ruby から呼び出したりすることもできます。そのため、任意のアプリケーションに組み込むことが可能であり、多様な使い方が考えられます。 興味のある方は 利用例 をご覧ください。\n"
"1.2. 全文検索と即時更新\n"
"\n"
"一般的なデータベースにおいては、追加・削除などの操作がすぐに反映されます。一方、全文検索においては、転置索引が逐次更新の難しいデータ構造であることから、文書の追加・削除に対応しないエンジンが少なくありません。\n"
"\n"
"これに対し、転置索引を用いた全文検索エンジンでありながら、groonga は文書を短時間で追加・削除することができます。その上、更新しながらでも検索できるという優れた特徴を持っているため、全文検索エンジンとしてはとても柔軟性があります。また、複数の転置索引を統合するような重い処理を必要としないので、安定して高い性能を発揮することが期待できます。\n"
"1.3. カラムストアと集計クエリ\n"
"\n"
"現代は、インターネットを情報源とすれば、いくらでも情報を収集できる時代です。しかし、膨大な情報から有益な情報を引き出すのは困難であり、多面的な分析による試行錯誤が必要となります。たとえば、日付や時間帯により絞り込んでみたり、地域により絞り込んでみたり、性別や年齢により絞り込んでみたりすることでしょう。そして、そのようなときに便利な存在が集計クエリです。\n"
"\n"
"集計クエリとは、指定したカラムの値によってレコードをグループ化し、各グループに含まれるレコードの数を求めるクエリです。たとえば、地域の ID を格納しているカラムを指定すれば、地域毎のレコード数が求まります。日付のカラムを指定したときの出力をグラフ化すれば、レコード数の時間変化を視覚化することができます。さらに、地域による絞り込みと日付に対する集計クエリを組み合わせれば、特定の地域におけるレコード数の時間変化を視覚化ことも可能です。このように、尺度を自由に選択して絞り込み・集計できることは、膨大な情報を扱う上でとても重要になります。\n"
"\n"
"groonga が集計クエリを高速に処理できる理由は、データベースの論理構造にカラムストアを採用しているからです。集計クエリが参照するのは指定されたカラムのみであるため、カラム単位でデータを格納する列指向のデータベースでは、必要なカラムのみを無駄なく読み出せることが利点となります。一方、レコード単位でデータを格納する行指向のデータベースでは、隣接するカラムをまとめて読み出してしまうことが欠点となります。\n"
"1.4. 転置索引とトークナイザ\n"
"\n"
"転置索引は大規模な全文検索に用いられる伝統的なデータ構造です。転置索引を用いた全文検索エンジンでは、文書を追加するときに索引語を記録しておき、検索するときはクエリを索引語に分割して出現文書を求めます。そのため、文書やクエリから索引語を抜き出す方法が重要になります。\n"
"\n"
"トークナイザは、文字列から索引語を抜き出すモジュールです。日本語を対象とする全文検索においては、形態素を索引語として抜き出す方式と文字 N-gram を抜き出す方式のいずれか、あるいは両方を用いるのが一般的です。形態素方式は検索時間や索引サイズの面で優れているほか、検索結果に不要な文書が含まれにくいという利点を持っています。一方、N-gram 方式には検索漏れが発生しにくいという利点があり、状況によって適した方式を選択することが望ましいとされています。\n"
"\n"
"groonga は形態素方式と N-gram 方式の両方に対応しています。初期状態で利用できるトークナイザは空白を区切り文字として用いる方式と N-gram 方式のみですが、形態素解析器 MeCab を組み込んだときは MeCab による分かち書きの結果を用いる形態素方式が有効になります。トークナイザはプラグインとして追加できるため、特徴的なキーワードのみを索引語として採用するなど、独自のトークナイザを開発することが可能です。\n"
"1.5. 共有可能なストレージと参照ロックフリー\n"
"\n"
"CPU のマルチコア化が進んでいるため、同時に複数のクエリを実行したり、一つのクエリを複数のスレッドで実行したりすることの重要性はますます高まっています。\n"
"\n"
"groonga のストレージは、複数のスレッド・プロセスで共有することができます。また、参照ロックフリーなデータ構造を採用しているため、更新クエリを実行している状況でも参照クエリを実行することができます。参照クエリを実行できる状態を維持しながら更新クエリを実行できるので、リアルタイムなシステムに適しています。さらには、MySQL を介して更新クエリを実行している最中に groonga の HTTP サーバを介して参照クエリを実行するなど、多彩な運用が可能となっています。\n"
"1.6. 位置情報（緯度・経度）検索\n"
"\n"
"GPS に代表される測位システムを搭載した高機能な携帯端末の普及などによって、位置情報を扱うサービスはますます便利になっています。たとえば、近くにあるレストランを探しているときは、現在地からの距離を基準として検索をおこない、検索結果を地図上に表示してくれるようなサービスが便利です。そのため、位置情報検索を高速に実現できることが重要になっています。\n"
"\n"
"groonga では転置索引を応用して高速な位置情報検索を実現しています。矩形・円による範囲検索に対応しているほか、基準点の近くを優先的に探索させることができます。また、距離計算をサポートしているので、位置情報検索の結果を基準点からの距離によって整列することも可能です。\n"
"1.7. groonga ライブラリ\n"
"\n"
"Groonga の基本機能は C ライブラリとして提供されているので、任意のアプリケーションに組み込んで利用することができます。C/C++ 以外については、Ruby から groonga を利用するライブラリなどが関連プロジェクトにおいて提供されています。詳しくは 関連プロジェクト を参照してください。\n"
"1.8. groonga サーバ\n"
"\n"
"groonga にはサーバ機能があるため、レンタルサーバなどの新しいライブラリをインストールできない環境においても利用できます。対応しているのは HTTP, memcached binary プロトコル、およびに groonga の独自プロトコルである gqtp です。サーバとして利用するときはクエリのキャッシュ機能が有効になるため、同じクエリを受け取ったときは応答時間が短くなるという特徴があります。\n"
"1.9. groonga ストレージエンジン\n"
"\n"
"groonga は独自のカラムストアを持つ列指向のデータベースとしての側面を持っていますが、既存の RDBMS のストレージエンジンとして利用することもできます。たとえば、groonga をベースとする MySQL のストレージエンジンとして mroonga が開発されています。mroonga は MySQL のプラグインとして動的にロードすることが可能であり、groonga のカラムストアをストレージとして利用したり、全文検索エンジンとして groonga を MyISAM や InnoDB と連携させたりすることができます。groonga 単体での利用、およびに MyISAM, InnoDB との連携には一長一短があるので、用途に応じて適切な組み合わせを選ぶことが大切です。詳しくは 関連プロジェクト を参照してください。\n";
static guint text_length;

typedef struct _BenchmarkData
{
  gboolean report_result;

  grn_ctx *context;
  grn_obj *database;
  grn_obj *normalizer;

  grn_obj *normalized_text;
  grn_str *nstr;
} BenchmarkData;

static void
bench_setup(gpointer user_data)
{
  BenchmarkData *data = user_data;
  data->normalized_text = NULL;
  data->nstr = NULL;
}

static void
bench_plugin(gpointer user_data)
{
  BenchmarkData *data = user_data;
  data->normalized_text = grn_normalized_text_open(data->context,
                                                   data->normalizer,
                                                   text, text_length,
                                                   GRN_ENC_UTF8,
                                                   GRN_NORMALIZE_REMOVE_BLANK |
                                                   GRN_NORMALIZE_WITH_TYPES |
                                                   GRN_NORMALIZE_WITH_CHECKS);
}

static void
bench_bundle(gpointer user_data)
{
  BenchmarkData *data = user_data;
  grn_ctx *ctx = data->context;
  data->nstr = GRN_MALLOC(sizeof(grn_str));
  data->nstr->orig = text;
  data->nstr->orig_blen = text_length;
  data->nstr->checks = NULL;
  data->nstr->ctypes = NULL;
  data->nstr->flags =
    GRN_STR_REMOVEBLANK |
    GRN_STR_WITH_TYPES |
    GRN_STR_WITH_CHECKS;
  utf8_nfkc_normalize(data->context, data->nstr);
}

static void
bench_teardown(gpointer user_data)
{
  BenchmarkData *data = user_data;

  if (data->report_result) {
    if (data->normalized_text) {
      const gchar *value;
      guint length_in_bytes;
      grn_normalized_text_get_value(data->context, data->normalized_text,
                                    &value, NULL, &length_in_bytes);
      g_print("result: %.*s\n", length_in_bytes, value);
    } else {
      g_print("result: %.*s\n",
              data->nstr->norm_blen,
              data->nstr->norm);
    }
  }

  if (data->normalized_text) {
    grn_obj_unlink(data->context, data->normalized_text);
  } else {
    grn_ctx *ctx = data->context;
    GRN_FREE(data->nstr);
  }
}

static void
setup_database(BenchmarkData *data)
{
  data->database = grn_db_create(data->context, NULL, NULL);

  data->normalizer = GET(data->context, "NormalizerUTF8NFKC");
}

static void
teardown_database(BenchmarkData *data)
{
  grn_obj_unlink(data->context, data->normalizer);
  grn_obj_unlink(data->context, data->database);
}

int
main(int argc, gchar **argv)
{
  BenchmarkData data;
  BenchReporter *reporter;
  gint n = 100;

  setlocale(LC_ALL, "");

  grn_init();
  bench_init(&argc, &argv);

  text_length = strlen(text);

  data.report_result = g_getenv("GROONGA_BENCH_REPORT_RESULT") != NULL;

  data.context = g_new(grn_ctx, 1);
  grn_ctx_init(data.context, 0);

  setup_database(&data);

  {
    const gchar *groonga_bench_n;
    groonga_bench_n = g_getenv("GROONGA_BENCH_N");
    if (groonga_bench_n) {
      n = atoi(groonga_bench_n);
    }
  }

  reporter = bench_reporter_new();

#define REGISTER(label, type)                                           \
  bench_reporter_register(reporter,                                     \
                          label,                                        \
                          n,                                            \
                          bench_setup,                                  \
                          bench_ ## type,                               \
                          bench_teardown,                               \
                          &data)
  REGISTER("NFKC: plugin", plugin);
  REGISTER("NFKC: bundle", bundle);
#undef REGISTER

  bench_reporter_run(reporter);
  g_object_unref(reporter);

  teardown_database(&data);

  grn_ctx_fin(data.context);
  g_free(data.context);

  bench_quit();
  grn_fin();

  return 0;
}
