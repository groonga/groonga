.. -*- rst -*-

.. highlightlang:: none

log_put
=======

名前
----

log_put - ログ出力

書式
----
::

 log_put level message

説明
----

groonga組込コマンドの一つであるlog_putについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

log_putは、ログにmessageを出力します。

引数
----

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

 log_put ERROR ****MESSAGE****
 [true]

関連項目
--------

:doc:`log_level`
:doc:`log_reopen`
