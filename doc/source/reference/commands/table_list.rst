.. -*- rst -*-

.. groonga-command
.. database: table_list

``table_list``
==============

Summary
-------

table_list - DBに定義されているテーブルをリスト表示

Groonga組込コマンドの一つであるtable_listについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

table_listは、DBに定義されているテーブルのリストを表示します。

Syntax
------
::

 table_list

Usage
-----
.. groonga-command
.. include:: ../../example/reference/commands/table_list.log
.. table_list

Parameters
----------

ありません。

Return value
------------

テーブル名一覧が以下の形式で返却されます。::

  [[[テーブル情報名1,テーブル情報型1],...], テーブル情報1,...]

``テーブル情報名n``

  ``テーブル情報n`` には複数の情報が含まれますが、そこに入る情報がどんな内容かを示す名前を出力します。
  情報名は以下の通りです。

  ``id``

    テーブルオブジェクトに割り当てられたID

  ``name``

    テーブル名

  ``path``

    テーブルのレコードを格納するファイル名

  ``flags``

    テーブルのflags属性

  ``domain``

    主キー値の属する型

  ``range``

    valueが属する型

``テーブル情報型n``

  テーブル情報の型を出力します。

``テーブル情報n``

  ``テーブル情報名n`` で示された情報の配列を出力します。
  情報の順序は ``テーブル情報名n`` の順序と同じです。

