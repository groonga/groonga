.. -*- rst -*-

``clearlock``
=============

Summary
-------

.. deprecated:: 4.0.9
   Use :doc:`lock_clear` instead.

clearlock - オブジェクトにセットされたロックを解除する

Groonga組込コマンドの一つであるclearlockについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

clearlockは、対象となるオブジェクト(データベース,テーブル,インデックス等)を指定し、オブジェクトにかけられたロックを再帰的に解除します。

Syntax
------
::

 clearlock objname

Usage
-----

開いているデータベースのロックをすべて解除する::

 clearlock
 [true]

テーブル名 Entry のカラム body のロックを解除する::

 clearlock Entry.body
 [true]

Parameters
----------

``objname``

  対象となるオブジェクト名を指定します。空の場合、開いているdbオブジェクトが対象となります。

Return value
------------

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

  エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

See also
--------

:doc:`load`
