= groonga C API マニュアル

groongaのC言語APIは、1)共通API、2)QL API、3)DB API、4)低レベルAPIの4種に大別される。
共通APIは他の全てのAPIと組み合わせて共通に使用される。残りの3種のAPIは階層構造をなしている。
QL APIは最も上位に位置し、query languageを介してデータストアを操作する機能を提供する。
DB APIはQL APIの下位に位置し、データストアを構成する各オブジェクトを操作する機能を提供する。
低レベルAPIはDB APIのさらに下位に位置し、データオブジェクトの構成要素を直接操作する機能を提供する。

= 共通API

== grn_init

:NAME

  grn_init, grn_fin - groongaライブラリを初期化/解放する

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_init(void);
  grn_rc grn_fin(void);

:DESCRIPTION

  grn_init()は、libgroongaが必要とする資源の初期化を行う。libgroongaを使用するプログラムは他のgroonga API関数を呼び出す前に一度だけgrn_init()を呼び出さなければならない。

  grn_fin()はlibgroongaが使用する資源を解放する。grn_fin()実行後にgroonga API関数を実行してはならない。

:RETURN VALUE

  成功した場合はGRN_SUCCESSを返す。
  grn_init()は、資源の確保に失敗した場合にGRN_NO_MEMORY_AVAILABLEを返す。

:NOTES

  grn_init(), grn_fin()はリエントラントではない。その他のgroongaのC API関数は全てリエントラントである。

== grn_ctx_init

:NAME

  grn_ctx_init, grn_ctx_fin - grn_ctx構造体を初期化/解放する

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ctx_init(grn_ctx *ctx, int flags, grn_encoding encoding);
  grn_rc grn_ctx_fin(grn_ctx *ctx);

:DESCRIPTION

  grn_ctxは、1)エラー情報の通知、2)API内部で使用するメモリの管理、3)クエリ言語処理系の管理のために使用される構造体である。ほとんどのAPI関数は第一引数にgrn_ctx構造体を要求する。grn_ctx構造体は<groonga/groonga.h>で定義されている。

   typedef struct _grn_ctx grn_ctx;
   struct _grn_ctx {
     grn_rc rc;                       /* 最後に実行したAPIの終了コード */
     int flags;                       /* 内部処理用 */
     grn_encoding encoding;           /* デフォルトの文字エンコーディング */
     unsigned char ntrace;            /* バックトレースの数 */
     unsigned char errlvl;            /* 最後に発生したエラーのレベル */
     unsigned char stat;              /* 内部処理用 */
     unsigned int seqno;              /* 内部処理用 */
     unsigned int subno;              /* 内部処理用 */
     unsigned int seqno2;             /* 内部処理用 */
     unsigned int errline;            /* エラーが発生した箇所の行番号 */
     grn_ctx *prev;                   /* 内部処理用 */
     grn_ctx *next;                   /* 内部処理用 */
     const char *errfile;             /* エラーが発生したソースファイル名 */
     const char *errfunc;             /* エラーが発生した関数名 */
     struct _grn_ctx_impl *impl;      /* 内部処理用 */
     void *trace[16];                 /* バックトレースポインタ配列 */
     char errbuf[GRN_CTX_MSGSIZE];    /* 最後に発生したエラーに関するメッセージ */
   };

  grn_ctx構造体はAPIで使用する前にgrn_ctx_init()で初期化しなければならない。初期化するgrn_ctx構造体へのポインタを'ctx'に指定する。'flags'にGRN_CTX_USE_QLを指定すると、grn_ctx内部にquery language処理系を生成する。GRN_CTX_USE_QLに加えてGRN_CTX_BATCH_MODEを指定した場合は、バッチモードでquery language処理系を生成する。'encoding'には、grn_ctxで文字列を処理する際のデフォルトの符号化方式を指定する。

  API実行中にエラーが発生した場合には、grn_ctx構造体のrc, ntrace, errlvl, errline, errfile, errfunc, trace, errbufメンバに情報がセットされる。

  grn_ctx_finはgrn_ctx構造体の使用する資源を解放する。grn_ctxを通して生成した一時オブジェクトも全て一括して解放される。

:RETURN VALUE

  成功した場合はGRN_SUCCESSを返す。
  flagsにGRN_CTX_USE_QLを指定してgrn_ctx_init()を実行した場合は、資源の確保に失敗するとGRN_NO_MEMORY_AVAILABLEを返す。GRN_CTX_USE_QLを指定せずにgrn_ctx_init()を実行した場合は必ずGRN_SUCCESSを返す。

:NOTES

  同一のgrn_ctx構造体を複数のスレッドが同時に使ってはいけない。スレッド固有データにgrn_ctxを保存し、スレッドとgrn_ctxと1:1に保てばこの制約は簡単に守ることができる。しかし、例えば非常に多くのクライアントからの接続を同時に受け付けるサーバシステムの中でgroongaを使用する場合には、スレッドとgrn_ctxとを動的に対応づけた方が有利かも知れない。

== grn_snip_open

:NAME

  grn_snip_open - snippet生成のための構造体を初期化する

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_snip *grn_snip_open(grn_ctx *ctx, grn_encoding encoding,
                          int flags, unsigned int width,
                          unsigned int max_results,
                          const char *defaultopentag,
                          unsigned int defaultopentag_len,
                          const char *defaultclosetag,
                          unsigned int defaultclosetag_len,
                          grn_snip_mapping *mapping);

:DESCRIPTION

  grn_snip_open()は、snippet生成のために用いる構造体grn_snipを確保し、そのポインタを返す。

:RETURN VALUE

  成功した場合はgrn_snip構造体のポインタを返す。
  構造体の確保に失敗した場合にはNULLを返す。

:NOTE

  同条件でsnippetを生成する場合には、grn_snip構造体を使いまわすことができる。

== grn_snip_close

:NAME

  grn_snip_close - snippet生成のための構造体を開放する

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_snip_close(grn_ctx *ctx, grn_snip *snip);

:DESCRIPTION

  grn_snip_close()は、grn_snip_openで確保されたsnippet生成のための構造体grn_snipを開放する。

:RETURN VALUE

  GRN_SUCCESSを返す。

== grn_snip_add_cond

:NAME

  grn_snip_add_cond - 検索対象の単語と、その単語の前後に付与する文字列を指定する。

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_snip_add_cond(grn_ctx *ctx, grn_snip *snip,
                           const char *keyword, unsigned int keyword_len,
                           const char *opentag, unsigned int opentag_len,
                           const char *closetag, unsigned int closetag_len);

:DESCRIPTION

  grn_snip_add_cond()は、検索対象の単語と、その単語の前後に付与する文字列を指定する。
  ctxは、grn_ctx_initで初期化したgrn_ctxインスタンスを指定します。
  snipは、grn_snip_openで生成したgrn_snipインスタンスを指定します。
  keywordは、検索対象の単語を指定します。
  keyword_lenは、keywordのバイト長を指定します。
  opentagは、snippet中の検索単語の前につける文字列を指定します。 NULLを指定した場合には、grn_snip_openで指定したdefaultopentagが使用されます。
  opentag_lenは、opentagのバイト長を指定します。
  closetagは、snippet中の検索単語の後につける文字列を指定します。 NULLを指定した場合には、grn_snip_openで指定したdefaultclosetagが使用されます。
  closetag_lenは、closetagのバイト長を指定します。

:RETURN VALUE

  引数の値が不正な場合、GRN_INVALID_ARGUMENTを返す。
  成功した場合、GRN_SUCCESSを返す。

:NOTE

  opentag, closetagの指す内容はコピーされない。sen_snip_closeを呼ぶまで、ポインタの開放や内容の変更は行えない。

== grn_snip_exec

:NAME

  grn_snip_exec - 検索対象の単語を検索し、snippetを生成します。

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_snip_exec(grn_ctx *ctx, grn_snip *snip,
                       const char *string, unsigned int string_len,
                       unsigned int *nresults, unsigned int *max_tagged_len);

:DESCRIPTION

  grn_snip_exec()は、検索対象の単語と、その単語の前後に付与する文字列を指定する。
  検索対象の単語を検索し、snippetを生成します。
  ctxは、grn_ctx_initで初期化したgrn_ctxインスタンスを指定します。
  snipは、grn_snip_openで生成したgrn_snipインスタンスを指定します。
  stringには、snippetを生成する対象の文字列を指定します。
  string_lenには、stringのバイト長を指定します。
  nresultsには、snippetを実際に生成できた個数が格納されます。
  max_tagged_lenには、生成されたsnippetのうち、一番長いsnippetについて末尾のNULLを含めた長さが格納されます。

:RETURN VALUE

  引数の値が不正な場合、GRN_INVALID_ARGUMENTを返す。
  結果保持用のメモリが確保できない場合は、GRN_NO_MEMORY_AVAILABLEを返す。
  成功した場合は、GRN_SUCCESSを返す。

== grn_snip_get_result

:NAME

  grn_snip_get_result - sen_snip_execで生成したsnippetを取り出します。

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_snip_get_result(grn_ctx *ctx, grn_snip *snip,
                             const unsigned int index,
                             char *result, unsigned int *result_len);

:DESCRIPTION

sen_snip_execで生成したsnippetを取り出します。
ctxは、grn_ctx_initで初期化したgrn_ctxインスタンスを指定します。
snipは、sen_snip_execで生成したsnippetを取り出します。
indexは、snippetの0からはじまるインデックスを指定します。
resultには、snippetの文字列が格納されます。
result_lenには、resultのバイト長が格納されます。

:RETURN VALUE

  引数の値が不正な場合、GRN_INVALID_ARGUMENTを返す。
  結果保持用のメモリが確保できない場合は、GRN_NO_MEMORY_AVAILABLEを返す。
  成功した場合は、GRN_SUCCESSを返す。

= QL API

== grn_ctx_connect

:NAME

  grn_ctx_connect - つなぐ

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ctx_connect(grn_ctx *ctx, const char *host, int port, int flags);

:DESCRIPTION

  つなぐ

== grn_ctx_send

:NAME

  grn_ctx_send - おくる

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ctx_send(grn_ctx *ctx, char *str, unsigned int str_len, int flags);

:DESCRIPTION

  おくる

== grn_ctx_recv

:NAME

  grn_ctx_recv - うけとる

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ctx_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags);

:DESCRIPTION

  うけとる

= DB API

== grn_db_create

:NAME

  grn_db_create, grn_db_open - データベースを生成する

:SYNOPSIS
 ((' '))
  #include <groonga/groonga.h>

  grn_obj *grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg);
  grn_obj *grn_db_open(grn_ctx *ctx, const char *path);

:DESCRIPTION

  つくる・ひらく

:RETURN VALUE

= low-level API

== grn_hash_create

:NAME

  grn_hash_create, grn_hash_open - ハッシュテーブルを生成する

:SYNOPSIS
 ((' '))
  #include <groonga/groonga.h>

  grn_hash *grn_hash_create(grn_ctx *ctx, const char *path, unsigned int key_size,
                            unsigned int value_size, unsigned int flags,
                            grn_encoding encoding);
  grn_hash *grn_hash_open(grn_ctx *ctx, const char *path);

:DESCRIPTION
:RETURN VALUE
:ERRORS
:CONFORMING TO
:AVAILABILITY
:NOTES
:BUGS
:EXAMPLE
:SEE ALSO
:COLOPHON

