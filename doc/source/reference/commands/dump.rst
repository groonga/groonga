.. -*- rst -*-

.. groonga-command
.. database: commands_dump

``dump``
========

Summary
-------

dump - Output a schema and data of a database.

We explain about dump that one of built-in command of Groonga.
Built-in commands of Groonga execute by sending a request to groonga server via argument of groonga execution file, standard input, and socket.

The dump command outputs schemas and data of a database as a format that can read from after.

The dump command mainly uses from a command line. Because a dump result is big.
The dump command mainly uses for the backup of databases.

Groonga can directly understand the format of the dump command. Therefore, we can copy databases as below. ::

  % groonga original/db dump > dump.grn
  % mkdir backup
  % groonga -n backup/db < dump.grn

Syntax
------
::

   dump [tables=null]
        [dump_plugins=yes]
        [dump_schema=yes]
        [dump_records=yes]
        [dump_indexes=yes]
        [dump_configs=yes]
        [sort_hash_table=no]

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
  {"_key":"PGroonga", "title":"Introduction to PGroonga"},
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
  ["PGroonga","Introduction to PGroonga"],
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
  ["PGroonga","Introduction to PGroonga"],
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
  ["PGroonga","Introduction to PGroonga"],
  ["Mroonga","Introduction to Mroonga"]
  ]

Dump schema only::

  > dump --dump_records no --dump_plugins no --dump_indexes no
  table_create Sites TABLE_NO_KEY
  column_create Sites url COLUMN_SCALAR ShortText

  table_create Bookmarks TABLE_HASH_KEY ShortText
  column_create Bookmarks title COLUMN_SCALAR ShortText

  table_create Lexicon TABLE_PAT_KEY ShortText

Dump sorted hash table data::

  > dump Bookmarks --sort_hash_table yes
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
  ["Mroonga","Introduction to Mroonga"],
  ["PGroonga","Introduction to PGroonga"]
  ]

  column_create Lexicon bookmark_title COLUMN_INDEX Bookmarks title

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

``sort_hash_table``
"""""""""""""""""""

.. versionadded:: 7.0.5

You can ascending sort by ``_key`` the output of hash table when it contains hash table.
To sort the output of hash table, specify ``yes``.

The default value is ``no``.

Return value
------------

データベースのスキーマとデータをGroongaの組み込みコマンド呼び出し形式で出力します。output_type指定は無視されます。

