.. -*- rst -*-

.. highlightlang:: none

table_remove
============

名前
----

table_remove - テーブルの削除

書式
----
::

 table_remove name

説明
----

groonga組込コマンドの一つであるtable_removeについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

table_removeはテーブルと定義されているカラムを削除します。カラムに付随するインデックスも再帰的に削除されます。

引数
----

``name``
  削除対象のカラムが定義されているテーブルの名前を指定します。

返値
----

json形式
^^^^^^^^

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

  エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

例
--
::

 table_remove Entry

 [true]
