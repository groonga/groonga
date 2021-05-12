.. -*- rst -*-

テスト方法
===========

TODO: Write in English.

TODO: Write about ``test/command/run-test.sh``.

テスト環境の構築
----------------

Cutterのインストール
^^^^^^^^^^^^^^^^^^^^

Groongaは、テストのフレームワークとして Cutter_ を用いています。

Cutterのインストール方法は プラットフォーム毎のCutterのインストール方法_ をご覧下さい。

.. _Cutter: http://cutter.sourceforge.net/
.. _プラットフォーム毎のCutterのインストール方法: http://cutter.sourceforge.net/reference/ja/install.html

lcovのインストール
^^^^^^^^^^^^^^^^^^

カバレッジ情報を計測するためには、lcov 1.6以上が必要です。DebianやUbuntuでは以下のようにしてインストールできます。::

 % sudo aptitude install -y lcov

clangのインストール
^^^^^^^^^^^^^^^^^^^

ソースコードの静的解析を行うためには、clang(scan-build)をインストールする必要があります。DebianやUbuntuでは以下のようにしてインストールできます。::

 % sudo aptitude install -y clang

libmemcachedのインストール
^^^^^^^^^^^^^^^^^^^^^^^^^^

memcachedのバイナリプロトコルのテストを動作させるためには、libmemcachedの導入が必要です。squeeze以降のDebianやKarmic以降のUubntuでは以下の用にしてインストールできます。::

 % sudo aptitude install -y libmemcached-dev

テストの動作
------------

Groongaのトップディレクトリで、以下のコマンドを実行します。::

 make check

カバレッジ情報
--------------

Groongaのトップディレクトリで、以下のコマンドを実行します。::

 make coverage

すると、coverageディレクトリ以下に、カバレッジ情報が入ったhtmlが出力されます。

カバレッジには、Lines/Functions/Branchesの3つの対象があります。それぞれ、行／関数／分岐に対応します。Functionsがもっとも重要な対象です。すべての関数がテストされるようになっていることを心がけてください。

テストがカバーしていない部分の編集は慎重に行ってください。また、テストがカバーしている部分を増やすことも重要です。

様々なテスト
------------

テストは、test/unitディレクトリにおいて、./run-test.shを実行することによっても行えます。run-test.shはいくつかのオプションをとります。詳細は、./run-test.sh --helpを実行しヘルプをご覧ください。

特定のテスト関数のみテストする
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

特定のテスト関数(Cutterではテストと呼ぶ)のみをテストすることができます。

実行例::

 % ./run-test.sh -n test_text_otoj

特定のテストファイルのみテストする
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

特定のテストファイル(Cutterではテストケースと呼ぶ)のみテストすることができます。

実行例::

 % ./run-test.sh -t test_string

不正メモリアクセス・メモリリーク検出
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

環境変数CUTTER_CHECK_LEAKをyesと設定すると、valgrindを用いて不正メモリアクセスやメモリリークを検出しつつ、テストを動作させることができます。

run-test.shのみならず、make checkでも利用可能です。

実行例::

 % CUTTER_CHECK_LEAK=yes make check

デバッガ上でのテスト実行
^^^^^^^^^^^^^^^^^^^^^^^^

環境変数CUTTER_DEBUGをyesと設定すると、テストが実行できる環境が整ったgdbが実行されます。gdb上でrunを行うと、テストの実行が開始されます。

run-test.shのみならず、make checkでも利用可能です。

実行例::

 % CUTTER_DEBUG=yes make check

静的解析
--------

scan-buildを用いて、ソースコードの静的解析を行うことができます。scan_buildというディレクトリに解析結果のhtmlが出力されます。::

 % scan-build ./configure --prefix=/usr
 % make clean
 % scan-build -o ./scan_build make -j4

configureは１度のみ実行する必要があります。
