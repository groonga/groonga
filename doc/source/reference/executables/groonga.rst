.. -*- rst -*-

.. program:: groonga

``groonga`` executable file
===========================

Summary
-------

``groonga`` executable file provides the following features:

  * Fulltext search server
  * Fulltext search shell
  * Client for Groonga fulltext search server

Groonga can be used as a library. If you want to use Groonga as a
library, you need to write a program in C, C++ and so on. Library use
is useful for embedding fulltext search feature to your application,
but it's not easy to use.

You can use ``groonga`` executable file to get fulltext search
feature.

If you want to try Groonga, fulltext search shell usage is useful. You
don't need any server and client. You just need one terminal. You can
try Groonga like the following::

  % groonga -n db
  > status
  [[0,1429687763.70845,0.000115633010864258],{"alloc_count":195,...}]
  > quit
  %

If you want to create an application that has fulltext search feature,
fulltext search server usage is useful. You can use Groonga as a
server like RDBMS (Relational DataBase Management
System). Client-server model is a popular architecture.

Normally, client for Groonga fulltext server usage isn't used.


Syntax
------

``groonga`` executable file has the following four modes:

  * Standalone mode
  * Server mode
  * Daemon mode
  * Client mode

There are common options in these modes. These common options are
described later section.

.. _groonga-executable-file-standalone-mode:

Standalone mode
^^^^^^^^^^^^^^^

In standalone mode, ``groonga`` executable file runs one or more
Groonga :doc:`/reference/command` against a local Groonga database.

Here is the syntax to run shell that executes Groonga command against
temporary database::

  groonga [options]

Here is the syntax to create a new database and run shell that
executes Groonga command against the new database::

  groonga [options] -n DB_PATH

Here is the syntax to run shell that executes Groonga command against
existing database::

  groonga [options] DB_PATH

Here is the syntax to run Groonga command against existing database
and exit::

  groonga [options] DB_PATH COMMAND [command arguments]

.. _groonga-executable-file-server-mode:

Server mode
^^^^^^^^^^^

In server mode, ``groonga`` executable file runs as a server. The
server accepts connections from other processes at local machine or
remote machine and executes received Groonga :doc:`/reference/command`
against a local Groonga database.

You can choose one protocol from :doc:`/server/http` and
:doc:`/server/gqtp`. Normally, HTTP is suitable but GQTP is the
default protocol. This section describes only about HTTP protocol
usage.

In server mode, ``groonga`` executable file runs in the foreground. If
you want to run Groonga server in the background, see 
:ref:`groonga-executable-file-daemon-mode`.

Here is the syntax to run Groonga server with temporary database::

  groonga [options] --protocol http -s

Here is the syntax to create a new database and run Groonga server
with the new database::

  groonga [options] --protocol http -s -n DB_PATH

Here is the syntax to run Groonga server with existing database::

  groonga [options] --protocol http -s DB_PATH

.. _groonga-executable-file-daemon-mode:

Daemon mode
^^^^^^^^^^^

In daemon mode, ``groonga`` executable file runs as a daemon. Daemon
is similar to server but it runs in the background. See
:ref:`groonga-executable-file-server-mode` about server.

Here is the syntax to run Groonga daemon with temporary database::

  groonga [options] --protocol http -d

Here is the syntax to create a new database and run Groonga daemon
with the new database::

  groonga [options] --protocol http -d -n DB_PATH

Here is the syntax to run Groonga daemon with existing database::

  groonga [options] --protocol http -d DB_PATH

:option:`--pid-path` option will be useful for daemon mode.

.. _groonga-executable-file-client-mode:

Client mode
^^^^^^^^^^^

In client mode, ``groonga`` executable file runs as a client for GQTP
protocol Groonga server. Its usage is similar to
:ref:`groonga-executable-file-standalone-mode`. You can run shell and
execute one command. You need to specify server address instead of
local database.

Note that you can't use ``groonga`` executable file as a client for
Groonga server that uses HTTP.

Here is the syntax to run shell that executes Groonga command against
Groonga server that is running at ``192.168.0.1:10043``::

  groonga [options] -c --port 10043 192.168.0.1

Here is the syntax to run Groonga command against Groonga server that
is running at ``192.168.0.1:10043`` and exit::

  groonga [options] -c --port 10043 192.168.0.1 COMMAND [command arguments]

Options
-------

.. option:: -n

   Creates new database.

.. option:: -c

   Executes ``groonga`` command in client mode.

.. option:: -s

   Executes ``groonga`` command in server mode. Use "Ctrl+C" to stop the ``groonga`` process.

.. option:: -d

   Executes ``groonga`` command in daemon mode. In contrast to server mode, ``groonga`` command forks in daemon mode. For example, to stop local daemon process, use "curl http://127.0.0.1:10041/d/shutdown".

.. option:: -e, --encoding <encoding>

   Specifies encoding which is used for Groonga database. This option is effective when you create new Groonga database.  This parameter specifies one of the following values: ``none``, ``euc``, ``utf8``, ``sjis``, ``latin`` or ``koi8r``.

.. option:: -l, --log-level <log level>

   Specifies log level. Log level must be a log level name or an
   integer value. Here are available log levels:

.. list-table::
   :header-rows: 1

   * - Log level
     - Name
     - Integer value
   * - None
     - ``none``
     - ``0``
   * - Emergency
     - ``E``, ``emerge`` or ``emergency``
     - ``1``
   * - Alert
     - ``A`` or ``alert``
     - ``2``
   * - Critical
     - ``C``, ``crit`` or ``critical``
     - ``3``
   * - Error
     - ``e`` or ``error``
     - ``4``
   * - Warning
     - ``w``, ``warn`` or ``warning``
     - ``5``
   * - Notice
     - ``n`` or ``notice``
     - ``6``
   * - Information
     - ``i`` or ``info``
     - ``7``
   * - Debug
     - ``d`` or ``debug``
     - ``8``
   * - Dump
     - ``-`` or ``dump``
     - ``9``

.. option:: -a, --address <ip/hostname>

   .. deprecated:: 1.2.2
      Use :option:`--bind-address` instead.

.. option:: --bind-address <ip/hostname>

   .. versionadded:: 1.2.2

   サーバモードかデーモンモードで実行するとき、listenするアドレスを指定します。(デフォルトは `hostname` の返すホスト名)

.. option:: -p, --port <port number>

   クライアント、サーバ、またはデーモンモードで使用するTCPポート番号。
   (クライアントモードのデフォルトは10043番、サーバ、またはデーモンモードのデフォルトは、HTTPの場合、10041番、GQTPの場合、10043番)

.. option:: -i, --server-id <ip/hostname>

   サーバモードかデーモンモードで実行するとき、サーバのIDとなるアドレスを指定します。(デフォルトは`hostname`の返すホスト名)

.. option:: -h, --help

   ヘルプメッセージを出力します。

.. option:: --document-root <path>

   httpサーバとしてgroongaを使用する場合に静的ページを格納するディレクトリを指定します。

   デフォルトでは、データベースを管理するための汎用的なページに対応するファイルが/usr/share/groonga/admin_html以下にインストールされます。このディレクトリをdocument-rootオプションの値に指定して起動した場合、ウェブブラウザでhttp://hostname:port/index.htmlにアクセスすると、ウェブベースのデータベース管理ツールを使用できます。

.. option:: --protocol <protocol>

   http,gqtpのいずれかを指定します。(デフォルトはgqtp)

.. option:: --log-path <path>

   ログを出力するファイルのパスを指定します。(デフォルトは/var/log/groonga/groonga.logです)

.. option:: --log-flags <log flags>

   .. versionadded:: 8.1.1

   Specify log flags. Default value is ``time|message``.

   ``+`` prefix means that ``add the flag``.

   ``-`` prefix means that ``remove the flag``.

   No prefix means that ``replace existing flags``.

   Multiple log flags can be specified by separating flags with ``|``.

   We can specify flags below.

   ``none``
     Output nothing into the log.

   ``time``
     Output timestamp into the log.

   ``message``
     Output log message into the log.

   ``location``
     Output process id and a location of an output of the log (file name, line and function name) into the log.

   ``process_id``
     Output process id into the log.

   ``pid``
     This flag is an alias of ``process_id``.

   ``thread_id``
     Output thread id into log

   ``all``
     This flag specifies all flags except ``none`` and ``default`` flags.

   ``default``
     Output timestamp and log message into the log.

.. option:: --log-rotate-threshold-size <threshold>

   .. versionadded:: 5.0.3

   Specifies threshold for log rotation. Log file is rotated when log file size is larger than or equals to the threshold (default: 0; disabled).

.. option:: --query-log-path <path>

   クエリーログを出力するファイルのパスを指定します。(デフォルトでは出力されません)

.. option:: --query-log-rotate-threshold-size <threshold>

   .. versionadded:: 5.0.3

   Specifies threshold for query log rotation. Query log file is rotated when query log file size is larger than or equals to the threshold (default: 0; disabled).

.. option:: -t, --max-threads <max threasd>

   最大で利用するスレッド数を指定します。(デフォルトはマシンのCPUコア数と同じ数です)

.. option:: --pid-path <path>

   PIDを保存するパスを指定します。(デフォルトでは保存しません)

.. option:: --config-path <path>

   設定ファイルのパスを指定します。設定ファイルは以下のようなフォーマットになります。::

     # '#'以降はコメント。
     ; ';'以降もコメント。

     # 'キー = 値'でオプションを指定。
     pid-path = /var/run/groonga.pid

     # '='の前後の空白はは無視される。↓は↑と同じ意味。
     pid-path=/var/run/groonga.pid

     # 'キー'は'--XXX'スタイルのオプション名と同じものが使える。
     # 例えば、'--pid-path'に対応するキーは'pid-path'。
     # ただし、キーが'config-path'のオプションは無視される。

.. option:: --cache-limit <limit>

   キャッシュ数の最大値を指定します。(デフォルトは100です)

.. option:: --default-match-escalation-threshold <threshold>

   検索の挙動をエスカレーションする閾値を指定します。(デフォルトは0です)

.. option:: --default-request-timeout <timeout>

   Specifies the default request timeout in seconds.

   You can specify timeout less than 1 second by decimal such as
   ``0.1``. ``0.1`` means that 100 milliseconds.

   If you specify ``0`` or less value, request timeout is disabled by
   default.

   The default value is ``0``.

   .. seealso:: :doc:`/reference/command/request_timeout`

.. option:: --cache-base-path <path>

   .. versionadded:: 7.0.2

   Specifies the base path for cache. It enables persistent cache
   feature.

   You can get the following merits by persistent cache feature:

     * You can reuse cache after ``groonga`` process is restarted. You
       don't need to warm up your cache each restart.

     * You can share cache with multiple ``groonga`` processes.

   You must specify the base path on memory file system. If you
   specify the base path on disk, your cache will be slow. It's not
   make sense.

   The default is nothing. It means that persistent cache is
   disabled. On memory cache is used.

   Persistent cache is a bit slower than on memory cache. Normally,
   the difference has little influence on performance.

Command line parameters
-----------------------

.. option:: dest

   使用するデータベースのパス名を指定します。

   クライアントモードの場合は接続先のホスト名とポート番号を指定します(デフォルト値は'localhost:10043')。ポート番号を指定しない場合には、10043が指定されたものとします。

.. option:: command [args]

   スタンドアロンおよびクライアントモードの場合は、実行するコマンドとその引数をコマンドライン引数に指定できます。コマンドライン引数にcommandを与えなかった場合は、標準入力から一行ずつEOFに達するまでコマンド文字列を読み取り、順次実行します。

.. _command-list-with-continuous-line:

Command
-------

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

Builtin command
---------------

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


Usage
-----

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
