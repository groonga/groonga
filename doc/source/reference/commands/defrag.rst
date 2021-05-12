.. -*- rst -*-

``defrag``
==========

Summary
-------

``defrag`` command resolves fragmentation of specified objects.

Groonga組込コマンドの一つであるdefragについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力
、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

defragは、対象となるオブジェクト(データベースか可変長サイズカラム)を指定し、オブジェクトのフラグメンテーショ
ンを解消します。

Syntax
------
::

 defrag objname threshold

Usage
-----

開いているデータベースのフラグメンテーションを解消する::

 defrag
 [300]

テーブル名 Entry のカラム body のフラグメンテーションを解消する::

 defrag Entry.body
 [30]

Parameters
----------

``objname``

  対象となるオブジェクト名を指定します。空の場合、開いているdbオブジェクトが対象となります。

Return value
------------

::

 [フラグメンテーション解消を実行したセグメントの数]

``フラグメンテーション解消を実行したセグメントの数``

  フラグメンテーション解消を実行したセグメントの数を返す。

