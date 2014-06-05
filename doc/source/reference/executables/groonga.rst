.. -*- rst -*-

.. highlightlang:: none

groonga command
===============

名前
----

groonga - 列指向データベース機能を持つ全文検索エンジンソフトウェア

書式
----

::

 groonga [options] [dest] [command [args]]

説明
----

Groongaは列指向のデータベース機能を持つ高速でスケーラブルな全文検索エンジンです。
Groongaのデータベースは、groongaコマンドかCライブラリインタフェースを通して操作することができます。このマニュアルページでは、groongaコマンドの使い方について説明します。

オプション
----------

.. cmdoption:: -n

   新たなデータベースを作成します。

.. cmdoption:: -c

   クライアントモードで実行します。

.. cmdoption:: -s

   サーバモードで実行します。

.. cmdoption:: -d

   デーモンモードで実行します。(forkする点がサーバモードと異なる)

.. cmdoption:: -e, --encoding <encoding>

   データベースで使用する文字エンコーディング方式を指定します。新たなデータベースを作成する時のみ有効です。none, euc, utf8, sjis, latin, koi8rのいずれかが指定できます。

.. cmdoption:: -l, --log-level <log level>

   ログレベルを指定します。0〜8までの数値が指定可能で、数が大きいほど多くのログが出力されます。

.. cmdoption:: -a, --address <ip/hostname>

   .. deprecated:: 1.2.2
      Use :option:`--bind-address` instead.

.. cmdoption:: --bind-address <ip/hostname>

   .. versionadded:: 1.2.2

   サーバモードかデーモンモードで実行するとき、listenするアドレスを指定します。(デフォルトは `hostname` の返すホスト名)

.. cmdoption:: -p, --port <port number>

   クライアント、サーバ、またはデーモンモードで使用するTCPポート番号。
   (クライアントモードのデフォルトは10043番、サーバ、またはデーモンモードのデフォルトは、HTTPの場合、10041番、GQTPの場合、10043番)

.. cmdoption:: -i, --server-id <ip/hostname>

   サーバモードかデーモンモードで実行するとき、サーバのIDとなるアドレスを指定します。(デフォルトは`hostname`の返すホスト名)

.. cmdoption:: -h, --help

   ヘルプメッセージを出力します。

.. cmdoption:: --document-root <path>

   httpサーバとしてgroongaを使用する場合に静的ページを格納するディレクトリを指定します。

   デフォルトでは、データベースを管理するための汎用的なページに対応するファイルが/usr/share/groonga/admin_html以下にインストールされます。このディレクトリをdocument-rootオプションの値に指定して起動した場合、ウェブブラウザでhttp://hostname:port/index.htmlにアクセスすると、ウェブベースのデータベース管理ツールを使用できます。

.. cmdoption:: --protocol <protocol>

   http,gqtpのいずれかを指定します。(デフォルトはgqtp)

.. cmdoption:: --log-path <path>

   ログを出力するファイルのパスを指定します。(デフォルトは/var/log/groonga/groonga.logです)

.. cmdoption:: --query-log-path <path>

   クエリーログを出力するファイルのパスを指定します。(デフォルトでは出力されません)

.. cmdoption:: -t, --max-threads <max threasd>

   最大で利用するスレッド数を指定します。(デフォルトはマシンのCPUコア数と同じ数です)

.. cmdoption:: --pid-path <path>

   PIDを保存するパスを指定します。(デフォルトでは保存しません)

.. cmdoption:: --config-path <path>

   設定ファイルのパスを指定します。設定ファイルは以下のようなフォーマットになります。::

     # '#'以降はコメント。
     ; ';'以降もコメント。

     # 'キー = 値'でオプションを指定。
     pid-file = /var/run/groonga.pid

     # '='の前後の空白はは無視される。↓は↑と同じ意味。
     pid-file=/var/run/groonga.pid

     # 'キー'は'--XXX'スタイルのオプション名と同じものが使える。
     # 例えば、'--pid-path'に対応するキーは'pid-path'。
     # ただし、キーが'config-path'のオプションは無視される。

.. cmdoption:: --cache-limit <limit>

   キャッシュ数の最大値を指定します。(デフォルトは100です)

.. cmdoption:: --default-match-escalation-threshold <threshold>

   検索の挙動をエスカレーションする閾値を指定します。(デフォルトは0です)

引数
----

.. cmdoption:: dest

   使用するデータベースのパス名を指定します。

   クライアントモードの場合は接続先のホスト名とポート番号を指定します(デフォルト値は'localhost:10043')。ポート番号を指定しない場合には、10043が指定されたものとします。

.. cmdoption:: command [args]

   スタンドアロンおよびクライアントモードの場合は、実行するコマンドとその引数をコマンドライン引数に指定できます。コマンドライン引数にcommandを与えなかった場合は、標準入力から一行ずつEOFに達するまでコマンド文字列を読み取り、順次実行します。

.. _command-list-with-continuous-line:

コマンド
--------

groongaコマンドを通してデータベースを操作する命令をコマンドと呼びます。コマンドは主にC言語で記述され、groongaプロセスにロードすることによって使用できるようになります。
それぞれのコマンドは一意な名前と、0個以上の引数を持ちます。

引数は以下の2種類の方法のいずれかで指定することができます。::

 形式1: コマンド名 値1 値2,..

 形式2: コマンド名 --引数名1 値1 --引数名2 値2,..

形式1でコマンドを実行する場合は、定義された順番で値を指定しなければならず、途中の引数の値を省略することはできません。形式2でコマンドを実行する場合は、「--引数名」のように引数の名前を明示しなければならない代わりに、任意の順番で引数を指定することが可能で、途中の引数の指定を省略することもできます。

標準入力からコマンド文字列を与える場合は、コマンド名と引数名と値は、空白( )で区切ります。空白や、記号「"'()\」のうちいずれかを含む値を指定したい場合は、シングルクォート(')かダブルクォート(")で値を囲みます。値として指定する文字列の中では、改行文字は'\n'に置き換えて指定します。また、引用符に使用した文字を値の中で指定する場合には、その文字の前にバックスラッシュ('\') を指定します。バックスラッシュ文字自身を値として指定する場合には、その前にバックスラッシュを指定します。

You can write command list with continuous line which is represented by '\\' character.::

  table_create --name Terms \
               --flags TABLE_PAT_KEY \
               --key_type ShortText \
               --default_tokenizer TokenBigram

組み込みコマンド
----------------

以下のコマンドは組み込みコマンドとして予め定義されています。

 ``status``
   groongaプロセスの状態を表示します。

 ``table_list``
   DBに定義されているテーブルのリストを表示します。

 ``column_list``
   テーブルに定義されているカラムのリストを表示します。

 ``table_create``
   DBにテーブルを追加します。

 ``column_create``
   テーブルにカラムを追加します。

 ``table_remove``
   DBに定義されているテーブルを削除します。

 ``column_remove``
   テーブルに定義されているカラムを削除します。

 ``load``
   テーブルにレコードを挿入します。

 ``select``
   テーブルに含まれるレコードを検索して表示します。

 ``define_selector``
   検索条件をカスタマイズした新たな検索コマンドを定義します。

 ``quit``
   データベースとのセッションを終了します。

 ``shutdown``
   サーバ(デーモン)プロセスを停止します。

 ``log_level``
   ログ出力レベルを設定します。

 ``log_put``
   ログ出力を行います。

 ``clearlock``
   ロックを解除します。


例
--

新しいデータベースを作成します。::

   % groonga -n /tmp/hoge.db quit
   %

作成済みのデータベースにテーブルを定義します。::

   % groonga /tmp/hoge.db table_create Table 0 ShortText
   [[0]]
   %

サーバを起動します。::

   % groonga -d /tmp/hoge.db
   %

httpサーバとして起動します。::

   % groonga -d -p 80 --protocol http --document-root /usr/share/groonga/admin_html /tmp/hoge.db
   %

サーバに接続し、テーブル一覧を表示します。::

   % groonga -c localhost table_list
   [[0],[["id","name","path","flags","domain"],[256,"Table","/tmp/hoge.db.0000100",49152,14]]]
   %
