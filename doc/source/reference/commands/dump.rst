.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_dump

``dump``
========

Summary
-------

dump - データベースのスキーマとデータを出力する

Groonga組込コマンドの一つであるdumpについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、
またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

dumpはデータベースのスキーマとデータを後から読み込めるフォーマットで出力します。dumpの結果は大きくなるため、
主にコマンドラインから使うことを想定しています。データベースのバックアップが主な利用方法です。

dumpが出力するフォーマットは直接Groongaが解釈できるフォーマットです。そのため、以下のようにしてデータベース>をコピーすることができます。::

  % groonga original/db dump > dump.grn
  % mkdir backup
  % groonga -n backup/db < dump.grn

Syntax
------
::

   dump [tables]
        [dump_plugins]
        [dump_schema]
        [dump_records]
        [dump_indexes]

Usage
-----

データベース内のすべてのデータを出力::

  > dump
  table_create LocalNames 48 ShortText
  table_create Entries 48 ShortText
  column_create Entries local_name 0 LocalNames
  column_create LocalNames Entries_local_name 2 Entries local_name
  ...
  load --table LocalNames
  [
  ["_key"],
  ["Items"],
  ["BLT"],
  ...
  ]
  ...

データベース内のスキーマと特定のテーブルのデータのみ出力::

  > dump --tables Users,Sites
  table_create Users TABLE_HASH_KEY ShortText
  column_create Users name COLUMN_SCALAR ShortText
  table_create Comments TABLE_PAT_KEY ShortText
  column_create Comments text COLUMN_SCALAR ShortText
  table_create Sites TABLE_NO_KEY
  column_create Sites url COLUMN_SCALAR ShortText
  load --table Users
  [
  ["_key"],
  ["mori"],
  ["yu"],
  ...
  ]
  load --table Sites
  [
  ["_id","url"],
  [1,"http://groonga.org/"],
  [2,"http://qwik.jp/senna/"],
  ...
  ]

Parameters
----------

There are optional parameters.

Optional parameters
^^^^^^^^^^^^^^^^^^^

``tables``
""""""""""

  出力対象のテーブルを「,」（カンマ）区切りで指定します。存在しないテーブルを指定した場合は無視されます。

``dump_plugins``
""""""""""""""""

You can customize the output whether it contains registered plugins or not.
To exclude registered plugins from the output, specify ``no``.

The default value is ``yes``.

``dump_schema``
"""""""""""""""

You can customize the output whether it contains database schema or not.
To exclude database schema from the output, specify ``no``.

The default value is ``yes``.

``dump_records``
""""""""""""""""

You can customize the output whether it contains records or not.
To exclude records from the output, specify ``no``.

The default value is ``yes``.

``dump_indexes``
""""""""""""""""

You can customize the output whether it contains indexes or not.
To exclude indexes from the output, specify ``no``.

The default value is ``yes``.

Return value
------------

データベースのスキーマとデータをGroongaの組み込みコマンド呼び出し形式で出力します。output_type指定は無視されます。

