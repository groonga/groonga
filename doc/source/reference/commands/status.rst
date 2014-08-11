.. -*- rst -*-

.. highlightlang:: none

.. _command-status:

``status``
==========

Summary
-------

status - groongaプロセスの状態表示

Groonga組込コマンドの一つであるstatusについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

statusコマンドは、groongaプロセスの状態を表示します。主にgroongaサーバプロセスに対して使用することを想定しています。

Syntax
------
::

 status

Usage
-----

.. groonga-command
.. include:: ../../example/reference/commands/status.log
.. status

Parameters
----------

ありません。

Return value
------------

::

  下記の項目がハッシュ形式で出力されます。

``alloc_count``

  groongaプロセスの内部でアロケートされ、まだ解放されてないメモリブロックの数を示します。Groongaをbuildする際に、configureオプションで --enable-exact-alloc-countが指定されていたならば、正確な値を返します。それ以外の場合は不正確な値を返す場合があります。

``starttime``

  groongaプロセスが起動した時刻のtvsec値を返します。

``uptime``

    groongaプロセスが起動してから経過した秒数を返します。

