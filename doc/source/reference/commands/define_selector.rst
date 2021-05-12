.. -*- rst -*-

``define_selector``
===================

Summary
-------

define_selector - 検索コマンドを定義

Groonga組込コマンドの一つであるdefine_selectorについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

define_selectorは、検索条件をカスタマイズした新たな検索コマンドを定義します。

Syntax
------
::

 define_selector name table [match_columns [query [filter [scorer [sortby
                 [output_columns [offset [limit [drilldown [drilldown_sortby
                 [drilldown_output_columns [drilldown_offset [drilldown_limit]]]]]]]]]]]]]

Usage
-----

テーブルEntryの全レコード・全カラムの値を出力するselectorコマンドを定義します。::

 define_selector entry_selector Entry
 [true]

Parameters
----------

``name``

  定義するselectorコマンドの名前を指定します。

``table``

  検索対象のテーブルを指定します。

``match_columns``

  追加するselectorコマンドのmatch_columns引数のデフォルト値を指定します。

``query``

  追加するselectorコマンドのquery引数のデフォルト値を指定します。

``filter``

  追加するselectorコマンドのfilter引数のデフォルト値を指定します。

``scorer``

  追加するselectorコマンドのscorer引数のデフォルト値を指定します。

``sortby``

  追加するselectorコマンドのsortby引数のデフォルト値を指定します。

``output_columns``

  追加するselectorコマンドのoutput_columns引数のデフォルト値を指定します。

``offset``

  追加するselectorコマンドのoffset引数のデフォルト値を指定します。

``limit``

  追加するselectorコマンドのlimit引数のデフォルト値を指定します。

``drilldown``

  追加するselectorコマンドのdrilldown引数のデフォルト値を指定します。

``drilldown_sortby``

  追加するselectorコマンドのdrilldown_sortby引数のデフォルト値を指定します。

``drilldown_output_columns``

  追加するselectorコマンドのdrilldown_output_columns引数のデフォルト値を指定します。

``drilldown_offset``

  追加するselectorコマンドのdrilldown_offset引数のデフォルト値を指定します。

``drilldown_limit``

  追加するselectorコマンドのdrilldown_limit引数のデフォルト値を指定します。

Return value
------------

::

 [成功かどうかのフラグ]

``成功かどうかのフラグ``

  エラーが生じなかった場合にはtrue、エラーが生じた場合にはfalseを返す。

See also
--------

:doc:`/reference/grn_expr`
