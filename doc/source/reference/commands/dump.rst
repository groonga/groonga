.. -*- rst -*-

.. highlightlang:: none

dump
====

名前
----

dump - データベースのスキーマとデータを出力する

書式
----
::

  dump [tables]

説明
----

Groonga組込コマンドの一つであるdumpについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

dumpはデータベースのスキーマとデータを後から読み込めるフォーマットで出力します。dumpの結果は大きくなるため、主にコマンドラインから使うことを想定しています。データベースのバックアップが主な利用方法です。

dumpが出力するフォーマットは直接Groongaが解釈できるフォーマットです。そのため、以下のようにしてデータベースをコピーすることができます。::

  % groonga original/db dump > dump.grn
  % mkdir backup
  % groonga -n backup/db < dump.grn

引数
----

``tables``

  出力対象のテーブルを「,」（カンマ）区切りで指定します。存在しないテーブルを指定した場合は無視されます。

返値
----

データベースのスキーマとデータをGroongaの組み込みコマンド呼び出し形式で出力します。output_type指定は無視されます。

例
--

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
