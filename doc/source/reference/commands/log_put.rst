.. -*- rst -*-

``log_put``
===========

Summary
-------

log_put - ログ出力

groonga組込コマンドの一つであるlog_putについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

log_putは、ログにmessageを出力します。

Syntax
------
::

 log_put level message

Usage
-----
::

 log_put ERROR ****MESSAGE****
 [true]

Parameters
----------

``level``

  設定するログ出力レベルの値を以下のいずれかで指定します。

     EMERG
     ALERT
     CRIT
     error
     warning
     notice
     info
     debug

``message``

  出力する文字列を指定します。

Return value
------------

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

  エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

See also
--------

:doc:`log_level`
:doc:`log_reopen`
