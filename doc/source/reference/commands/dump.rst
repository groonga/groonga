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
        [dump_configs]

Usage
-----

Here is the sample schema and data to check dump behaviour::

  plugin_register token_filters/stop_word
  table_create Bookmarks TABLE_HASH_KEY ShortText
  column_create Bookmarks title COLUMN_SCALAR ShortText
  table_create Lexicon TABLE_PAT_KEY ShortText
  table_create Sites TABLE_NO_KEY
  column_create Sites url COLUMN_SCALAR ShortText
  column_create Lexicon bookmark_title COLUMN_INDEX Bookmarks title
  load --table Bookmarks
  [
  {"_key":"Groonga", "title":"Introduction to Groonga"},
  {"_key":"Mroonga", "title":"Introduction to Mroonga"}
  ]
  load --table Sites
  [
  {"_id": 1, "url":"http://groonga.org"},
  {"_id": 2, "url":"http://mroonga.org"}
  ]

Dump all data in database::

  > dump
  plugin_register token_filters/stop_word
  
  table_create Sites TABLE_NO_KEY
  column_create Sites url COLUMN_SCALAR ShortText
  
  table_create Bookmarks TABLE_HASH_KEY ShortText
  column_create Bookmarks title COLUMN_SCALAR ShortText
  
  table_create Lexicon TABLE_PAT_KEY ShortText
  
  load --table Sites
  [
  ["_id","url"],
  [1,"http://groonga.org"],
  [2,"http://mroonga.org"]
  ]
  
  load --table Bookmarks
  [
  ["_key","title"],
  ["Groonga","Introduction to Groonga"],
  ["Mroonga","Introduction to Mroonga"]
  ]
  
  create Lexicon bookmark_title COLUMN_INDEX Bookmarks title

Dump schema and specific table data::

  > dump Bookmarks
  plugin_register token_filters/stop_word
  
  table_create Sites TABLE_NO_KEY
  column_create Sites url COLUMN_SCALAR ShortText
  
  table_create Bookmarks TABLE_HASH_KEY ShortText
  column_create Bookmarks title COLUMN_SCALAR ShortText
  
  table_create Lexicon TABLE_PAT_KEY ShortText
  
  load --table Bookmarks
  [
  ["_key","title"],
  ["Groonga","Introduction to Groonga"],
  ["Mroonga","Introduction to Mroonga"]
  ]
  
  column_create Lexicon bookmark_title COLUMN_INDEX Bookmarks title

Dump plugin only::

  > dump --dump_schema no --dump_records no --dump_indexes no
  plugin_register token_filters/stop_word

Dump records only::

  > dump --dump_schema no --dump_plugins no --dump_indexes no
  load --table Sites
  [
  ["_id","url"],
  [1,"http://groonga.org"],
  [2,"http://mroonga.org"]
  ]
  
  load --table Bookmarks
  [
  ["_key","title"],
  ["Groonga","Introduction to Groonga"],
  ["Mroonga","Introduction to Mroonga"]
  ]

Dump schema only::

  > dump --dump_records no --dump_plugins no --dump_indexes no
  table_create Sites TABLE_NO_KEY
  column_create Sites url COLUMN_SCALAR ShortText
  
  table_create Bookmarks TABLE_HASH_KEY ShortText
  column_create Bookmarks title COLUMN_SCALAR ShortText
  
  table_create Lexicon TABLE_PAT_KEY ShortText

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

.. versionadded:: 5.0.3

You can customize the output whether it contains registered plugins or not.
To exclude registered plugins from the output, specify ``no``.

The default value is ``yes``.

``dump_schema``
"""""""""""""""

.. versionadded:: 5.0.3

You can customize the output whether it contains database schema or not.
To exclude database schema from the output, specify ``no``.

The default value is ``yes``.

``dump_records``
""""""""""""""""

.. versionadded:: 5.0.3

You can customize the output whether it contains records or not.
To exclude records from the output, specify ``no``.

The default value is ``yes``.

``dump_indexes``
""""""""""""""""

.. versionadded:: 5.0.3

You can customize the output whether it contains indexes or not.
To exclude indexes from the output, specify ``no``.

The default value is ``yes``.

Return value
------------

データベースのスキーマとデータをGroongaの組み込みコマンド呼び出し形式で出力します。output_type指定は無視されます。

