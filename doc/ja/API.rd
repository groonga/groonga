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

  grn_ctx構造体はAPIで使用する前にgrn_ctx_init()で初期化しなければならない。初期化するgrn_ctx構造体へのポインタをctxに指定する。flagsにGRN_CTX_USE_QLを指定すると、grn_ctx内部にquery language処理系を生成する。GRN_CTX_USE_QLに加えてGRN_CTX_BATCH_MODEを指定した場合は、バッチモードでquery language処理系を生成する。encodingには、grn_ctxで文字列を処理する際のデフォルトの符号化方式を指定する。

  API実行中にエラーが発生した場合には、grn_ctx構造体のrc, ntrace, errlvl, errline, errfile, errfunc, trace, errbufメンバに情報がセットされる。

  grn_ctx_finはgrn_ctx構造体の使用する資源を解放する。grn_ctxを通して生成した一時オブジェクトも全て一括して解放される。

:RETURN VALUE

  成功した場合はGRN_SUCCESSを返す。
  flagsにGRN_CTX_USE_QLを指定してgrn_ctx_init()を実行した場合は、資源の確保に失敗するとGRN_NO_MEMORY_AVAILABLEを返す。GRN_CTX_USE_QLを指定せずにgrn_ctx_init()を実行した場合は必ずGRN_SUCCESSを返す。

:NOTES

  同一のgrn_ctx構造体を複数のスレッドが同時に使ってはいけない。スレッド固有データにgrn_ctxを保存し、スレッドとgrn_ctxと1:1に保てばこの制約は簡単に守ることができる。しかし、例えば非常に多くのクライアントからの接続を同時に受け付けるサーバシステムの中でgroongaを使用する場合には、スレッドとgrn_ctxとを動的に対応づけた方が有利かも知れない。

= QL API

== grn_ql_connect

:NAME

  grn_ql_connect - つなぐ

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ql_connect(grn_ctx *ctx, const char *host, int port, int flags);

:DESCRIPTION

  つなぐ

== grn_ql_send

:NAME

  grn_ql_send - おくる

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ql_send(grn_ctx *ctx, char *str, unsigned int str_len, int flags);

:DESCRIPTION

  おくる

== grn_ql_recv

:NAME

  grn_ql_send - うけとる

:SYNOPSIS
 ((' '))

  #include <groonga/groonga.h>

  grn_rc grn_ql_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags);

:DESCRIPTION

  うけとる

= DB API

== grn_db_create

:NAME

  grn_db_create - つくる

:SYNOPSIS
 ((' '))
  #include <groonga/groonga.h>

:DESCRIPTION

  つくる

:RETURN VALUE

= low-level API

== grn_hoge

:NAME

:SYNOPSIS
 ((' '))
  #include <groonga/groonga.h>

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

