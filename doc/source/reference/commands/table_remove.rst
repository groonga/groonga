.. -*- rst -*-

.. highlightlang:: none

``table_remove``
================

Summary
-------

table_remove - テーブルの削除

groonga組込コマンドの一つであるtable_removeについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

table_removeはテーブルと定義されているカラムを削除します。カラムに付随するインデックスも再帰的に削除されます。

Syntax
------
::

 table_remove name

Usage
-----
::

 table_remove Entry

 [true]

Parameters
----------

``name``
  削除対象のカラムが定義されているテーブルの名前を指定します。

Return value
------------

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

  エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

