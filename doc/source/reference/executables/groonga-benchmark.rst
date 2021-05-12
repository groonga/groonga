.. -*- rst -*-

.. program:: groonga-benchmark

groonga-benchmark
=================

名前
----

groonga-benchmark - groongaテストプログラム

書式
----

::

  groonga-benchmark  [options...] [script] [db]

説明
----

groonga-benchmarkは、groonga汎用ベンチマークツールです。

groongaを単独のプロセスとして利用する場合はもちろん、サーバプログラムとして利用する場合の動作確認や実行速度測定が可能です。

groonga-benchmark用のデータファイルは自分で作成することも既存のものを利用することもできます。既存のデータファイルは、ftp.groonga.orgから必要に応じダウンロードします。そのため、groonga及びgroonga-benchmarkが動作し、インターネットに接続できる環境であればgroongaコマンドの知識がなくてもgroongaの動作を確認できます。

現在は、Linux 及びWindows上で動作します。make installしてもインストールは行われません。

オプション
----------
.. option:: -i, --host <ip/hostname>

  接続するgroongaサーバを、ipアドレスまたはホスト名で指定します。指定先にgroongaサーバが立ち上がっていない場合、接続不能となることに注意してください。このオプションを指定しない場合、groonga-benchmarkは自動的にlocalhostのgroongaサーバを起動して接続します。


.. option:: -p, --port <port number>

  自動的に起動するgroongaサーバ、または明示的に指定した接続先のgroonga サーバが利用するポート番号を指定します。接続先のgroongaサーバが利用しているポートと、このオプションで指定したポート番号が異なる場合、接続不能となることに注意してください。


.. option:: --dir

  ftp.groonga.org に用意されているスクリプトファイルを表示します。

.. option:: --ftp

  ftp.groonga.orgとFTP通信を行い、scriptファイルの同期やログファイルの送信を行います。

.. option:: --log-output-dir

  デフォルトでは、groonga-benchmark終了後のログファイルの出力先ははカレントディレクトリです。このオプションを利用すると、任意のディレクトリに出力先を変更することができます。

.. option:: --groonga <groonga_path>

  groongaコマンドのパスを指定します。デフォルトでは、PATHの中からgroongaコマンドを探します。

.. option:: --protocol <gqtp|http>

  groongaコマンドが使うプロトコルとして `gqtp` または `http` を指定します。

引数
----

.. option:: script

  groonga-benchmarkの動作方法(以下、groonga-benchmark命令と呼びます)を記述したテキストファイルです。拡張子は.scrです。

.. option:: db

  groonga-benchmarkが利用するgroonga データベースです。指定されたデータベースが存在しない場合、groonga-benchmarkが新規に作成します。またgroonga サーバを自動的に起動する場合もこの引数で指定したデータベースが利用されます。接続するgroonga サーバを明示的に指定した場合に利用するデータベースは、接続先サーバが使用中のデータベースになることに注意してください。


使い方
------
まず、シェル上(Windowsならコマンドプロンプト上)で::

   groonga-benchmark test.scr 任意のDB名

とタイプしてください。もしgroonga-benchmarkが正常に動作すれば、::

   test-ユーザ名-数字.log

というファイルが作成されるはずです。作成されない場合、このドキュメントの「トラブルシューティング」の章を参照してください。


スクリプトファイル
------------------

スクリプトファイルは、groonga-benchmark命令を記述したテキストファイルです。
";"セミコロンを利用して、一行に複数のgroonga-benchmark命令を記述することができます。一行に複数のgroonga-benchmark命令がある場合、各命令は並列に実行されます。
"#"で始まる行はコメントとして扱われます。



groonga-benchmark命令
^^^^^^^^^^^^^^^^^^^^^

現在サポートされているgroonga-benchmark命令は以下の11種類です。

  do_local コマンドファイル [スレッド数] [繰り返し数]

    コマンドファイルをgroonga-benchmark単体で実行します。スレッド数が指定されている場合、複数のスレッドで同じコマンドファイルを同時に実行します。繰り返し数が指定されてい場合、コマンドファイルの内容を繰り返し実行します。スレッド数、繰り返し数とも省略時は1です。1スレッドで複数回動作させたい場合は、do_local コマンドファイル 1 [繰り返し数]と明示的に指定してください。

  do_gqpt コマンドファイル [スレッド数] [繰り返し数]

    コマンドファイルをgroongaサーバでGQTP経由で実行します。スレッド数や繰り返し数の意味はdo_localの場合と同じです。

  do_http コマンドファイル [スレッド数] [繰り返し数]

    コマンドファイルをgroongaサーバでHTTP経由で実行します。スレッド数や繰り返し数の意味はdo_localの場合と同じです。

  rep_local コマンドファイル [スレッド数] [繰り返し数]

    コマンドファイルをgroonga-benchmark単体で実行し、より詳細な報告を行います。

  rep_gqpt コマンドファイル [スレッド数] [繰り返し数]

    コマンドファイルをgroongaサーバでGQTP経由で実行し、より詳細な報告を行います。 スレッド数や繰り返し数の意味はdo_localと 同じです。

  rep_http コマンドファイル [スレッド数] [繰り返し数]

    コマンドファイルをgroongaサーバでHTTP経由で実行し、より詳細な報告を行います。 スレッド数や繰り返し数の意味はdo_localと 同じです。

  out_local コマンドファイル 入力ファイル名

    コマンドファイルをgroonga-benchmark単体で実行し、各コマンドの実行結果をすべて”出力ファイル"に書きだします。この結果は、test_local,　test_gqtp命令で利用します。なおこの命令の「出力ファイル」とは、groonga-benchmark実行時に自動的に作成されるログとは別のものです。groonga-benchmarkではコメントが利用できる以外、::

     groonga < コマンドファイル > 出力ファイル

    とした場合と同じです。

  out_gqtp コマンドファイル 出力ファイル名

    コマンドファイルをgroongaサーバでGQTP経由で実行します。その他はout_local命令と同等です。

  out_http コマンドファイル 出力ファイル名

    コマンドファイルをgroongaサーバでHTTP経由で実行します。その他はout_local命令と同等です。

  test_local コマンドファイル 入力ファイル名
    コマンドファイルをgroonga-benchmark単体で実行し、各コマンドの実行結果を入力ファイルと比較します。処理時間など本質的要素以外に差分があった場合、差分を、入力ファイル.diffというファイルに書きだします。


コマンドファイル
^^^^^^^^^^^^^^^^

コマンドファイルは、groonga組み込みコマンドを1行に1つずつ記述したテキストファイルです。拡張子に制限はありません。groonga組み込みコマンドに関しては :doc:`/reference/command` を参照してください。

サンプル
^^^^^^^^

スクリプトファイルのサンプルです。::

  # sample script
  rep_local test.ddl
  do_local test.load;
  do_gqtp test.select 10 10; do_local test.status 10


上記の意味は以下のとおりです。

  1行目
    コメント行。
  2行目
    test.ddl というコマンドファイルをgroonga単体で実行し、詳細に報告する。
  3行目
    test.load というコマンドファイルをgroonga単体で実行する。(最後の";"セミコロンは複数のgroonga-benchmark命令を記述する場合に必要ですが、この例のように1つのgroonga-benchmark命令を実行する場合に付与しても問題ありません。)
  4行目
    test.select というコマンドファイルをgroongaサーバで10個のスレッドで同時に実行する。各スレッドはtest.selectの中身を10回繰り返す。また同時に、groonga単体でtest.statusというコマンドファイルを10個のスレッドで実行する。

特殊命令
^^^^^^^^

スクリプトファイルのコメント行には特殊コマンドを埋め込むことが可能です。現在サポートされている特殊命令は以下の二つです。

  #SET_HOST <ip/hostname>
    -i, --hostオプションと同等の機能です。コマンドラインオプションに指定したIPアドレス/ホスト名と、SET_HOSTで指定したIPアドレス/ホスト名が異なる場合、またコマンドラインオプションを指定しなかった場合にもSET_HOSTが優先されます。SET_HOSTを利用した場合、サーバが自動的には起動されないのもコマンドラインオプションで指定した場合と同様です。

  #SET_PORT <port number>
    -p, --port オプションと同等の機能です。コマンドラインオプションに指定したポート番号とSET_PORTで指定したポート番号が異なる場合、またコマンドラインオプションを指定しなかった場合にもSET_PORTが優先されます。


特殊命令はスクリプトファイルの任意の場所に書き込むことができます。同一ファイル内に複数回特殊命令を記述した場合、「最後の」特殊命令が有効となります。

例えば、

::

  $ ./groonga-benchmark --port 20010 test.scr testdb

とコマンド上でポートを指定した場合でも、もしtest.scrの中身が

::

  #SET_PORT 10900
  rep_local test.ddl
  do_local test.load;
  rep_gqtp test.select 10 10; rep_local test.status 10
  #SET_PORT 10400

であれば、自動的に起動されるgroongaサーバはポート番号10400を利用します。


groonga-benchmark実行結果
-------------------------

groonga-benchmarkが正常に終了すると、(拡張子を除いた)スクリプト名-ユーザ名-実行開始時刻.logという形式のログファイルがカレントディレクトリに作られます。ログファイルは自動的にftp.groonga.org
に送信されます。ログファイルは以下のようなjson形式のテキストです。

::

  [{"script": "test.scr",
    "user": "homepage",
    "date": "2010-04-14 22:47:04",
    "CPU": Intel(R) Pentium(R) 4 CPU 2.80GHz",
    "BIT": 32,
    "CORE": 1,
    "RAM": "975MBytes",
    "HDD": "257662232KBytes",
    "OS": "Linux 2.4.20-24.7-i686",
    "HOST": "localhost",
    "PORT": "10041",
    "VERSION": "0.1.8-100-ga54c5f8"
  },
  {"jobs": "rep_local test.ddl",
  "detail": [
  [0, "table_create res_table --key_type ShortText", 1490, 3086, [0,1271252824.25846,0.00144
  7]],
  [0, "column_create res_table res_column --type Text", 3137, 5956, [0,1271252824.2601,0.002
  741]],
  [0, "column_create res_table user_column --type Text", 6020, 8935, [0,1271252824.26298,0.0
  02841]],
  [0, "column_create res_table mail_column --type Text", 8990, 11925, [0,1271252824.26595,0.
  002861]],
  [0, "column_create res_table time_column --type Time", 12008, 13192, [0,1271252824.26897,0
  .001147]],
  [0, "status", 13214, 13277, [0,1271252824.27018,3.0e-05]],
  [0, "table_create thread_table --key_type ShortText", 13289, 14541, [0,1271252824.27025,0.
  001213]],
  [0, "column_create thread_table thread_title_column --type ShortText", 14570, 17380, [0,12
  71252824.27153,0.002741]],
  [0, "status", 17435, 17480, [0,1271252824.2744,2.7e-05]],
  [0, "table_create lexicon_table --flags 129 --key_type ShortText --default_tokenizer Token
  Bigram", 17491, 18970, [0,1271252824.27446,0.001431]],
  [0, "column_create lexicon_table inv_res_column 514 res_table res_column ", 18998, 33248,
  [0,1271252824.27596,0.01418]],
  [0, "column_create lexicon_table inv_thread_column 514 thread_table thread_title_column ",
   33285, 48472, [0,1271252824.29025,0.015119]],
  [0, "status", 48509, 48554, [0,1271252824.30547,2.7e-05]]],
  "summary" :[{"job": "rep_local test.ddl", "latency": 48607, "self": 47719, "qps": 272.4281
  73, "min": 45, "max": 15187, "queries": 13}]},
  {"jobs": "do_local test.load; ",
  "summary" :[{"job": "do_local test.load", "latency": 68693, "self": 19801, "qps": 1010.049
  997, "min": 202, "max": 5453, "queries": 20}]},
  {"jobs": "do_gqtp test.select 10 10; do_local test.status 10",
  "summary" :[{"job": " do_local test.status 10", "latency": 805990, "self": 737014, "qps":
  54.273053, "min": 24, "max": 218, "queries": 40},{"job": "do_gqtp test.select 10 10", "lat
  ency": 831495, "self": 762519, "qps": 1967.164097, "min": 73, "max": 135631, "queries": 15
  00}]},
  {"total": 915408, "qps": 1718.359464, "queries": 1573}]



制限事項
--------

* スクリプトファイルの一行には複数のgroonga-benchmark命令を記述できますが、すべてのスレッド数の合計は最大64までに制限されます。

* コマンドファイル中のgroongaコマンドの長さは最長5000000byteです。


トラブルシューティング
----------------------

もし、groonga-benchmarkが正常に動作しない場合、まず以下を確認してください。

* インターネットに接続しているか？ `--ftp` オプションを指定すると、groonga-benchmarkは動作のたびにftp.groonga.orgと通信します。ftp.groonga.orgと通信可能でない場合、groonga-benchmarkは正常に動作しません。

* groonga サーバが動作していないか？　groonga-benchmarkは、-i, --host オプションで明示的にサーバを指定しないかぎり、自動的にlocalhostのgroongaサーバを立ち上げます。すでにgroongaサーバが動作している場合、groonga-benchmarkは正常に動作しない可能性があります。

* 指定したDBが適切か？ groonga-benchmarkは、引数で指定したDBの中身はチェックしません。もし指定されたDBが存在しなければ自動的にDBを作成しますが、もしファイルとして存在する場合は中身に関わらず動作を続けてしまい、結果が異常になる可能性があります。

以上の原因でなければ、問題はgroonga-benchmarkかgroongaにあります。ご報告をお願いします。
