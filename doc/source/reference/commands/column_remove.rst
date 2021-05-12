.. -*- rst -*-

``column_remove``
=================

Summary
-------

column_remove - テーブルに定義されているカラムの削除

Groonga組込コマンドの一つであるcolumn_removeについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

column_removeはテーブルに定義されているカラムを削除します。
また、付随するインデックスも削除されます。[#]_

Syntax
------
::

 column_remove table name

Usage
-----
::

 column_remove Entry body

 [true]

.. rubric:: 脚注

.. [#] マルチセクションインデックスの一部である場合も、インデックスが削除されます。

Parameters
----------

``table``

  削除対象のカラムが定義されているテーブルの名前を指定します。

``name``

  削除対象のカラム名を指定します。

Return value
------------

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

   エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

