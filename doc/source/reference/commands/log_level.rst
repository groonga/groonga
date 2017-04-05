.. -*- rst -*-

.. highlightlang:: none

``log_level``
=============

Summary
-------

log_level - ログ出力レベルの設定

Groonga組込コマンドの一つであるlog_levelについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

log_levelは、ログ出力レベルを設定します。

Syntax
------
::

 log_level level

Usage
-----
::

 log_level warning
 [true]

Parameters
----------

``level``

  設定するログ出力レベルの値を以下のいずれかで指定します。

     emergency
     alert
     critical
     error
     warning
     notice
     info
     debug

Return value
------------

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

  エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

See also
--------

:doc:`log_put`
:doc:`log_reopen`
