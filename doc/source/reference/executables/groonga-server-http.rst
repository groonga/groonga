.. -*- rst -*-

.. highlightlang:: none

groonga HTTPサーバー
====================

名前
----

groonga HTTPサーバー

書式
----
::

 groonga -d --protocol http DB_PATH

説明
----

groongaサーバを起動する時に--protocolオプションにhttpを指定すると、httpで通信可能になります。また、--document-root によって静的ページのパスを指定すると、httpリクエストに指定されたURIに対応する、パス配下に置かれたファイルを出力します。

groongaにはHTML + JavaScriptで実装された管理ツールが標準で付属しています。--document-rootを指定しない場合は管理ツールがインストールされているパスが指定されたとみなされますので、ウェブブラウザでhttp://hostname:port/にアクセスすると、管理ツールを利用できます。

コマンド
--------

httpを指定して起動したgroongaサーバに対しても、他のモードで起動したgroongaと同じコマンドが使用できます。

コマンドは、複数の引数をとります。引数にはそれぞれ名前があります。また、特殊な引数である「output_type」と「command_version」があります。

スタンドアロンやクライアントモードでは、コマンドは以下のような形式で指定します。

 形式1: コマンド名 値1 値2,..

 形式2: コマンド名 --引数名1 値1 --引数名2 値2,..

形式1と形式2は混在させることができます。これらの形式では、output_typeという引数名を用いてoutput_typeを指定します。

httpでgroongaサーバと通信する際には、以下のような形式でコマンドを指定します。::

 形式: /d/コマンド名.output_type?引数名1=値1&引数名2=値2&...

ただし、コマンド名、引数名、値はURLエンコードが必要です。

GETメソッドのみが使用可能です。

output_typeにはjson, tsv, xmlが指定可能です。

command_versionはコマンドの仕様の互換性を指定します。詳細は :doc:`/reference/command/command_version` を参照してください。

返値
----

output_typeの指定に従って、コマンドの実行結果を出力します。
