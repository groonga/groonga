.. -*- rst -*-

.. groonga-command
.. database: reindex

``reindex``
===========

Summary
-------

.. versionadded:: 5.1.0

``reindex`` command recreates one or more index columns.

If you specify a database as target object, all index columns are
recreated.

If you specify a table as target object, all index columns in the
table are recreated.

If you specify a data column as target object, all index columns for
the data column are recreated.

If you specify an index column as target object, the index column is
recreated.

This command is useful when your index column is broken. The target
object is one of database, table and column.

.. note::

   You can't use target index columns while ``reindex`` command is
   running. If you use the same database from multiple processes, all
   processes except running ``reindex`` should reopen the
   database. You can use :doc:`database_unmap` for reopening database.

Syntax
------

This command takes only one optional parameter::

  reindex [target_name=null]

If ``target_name`` parameters is omitted, database is used for the
target object. It means that all index columns in the database are
recreated.

Usage
-----

Here is an example to recreate all index columns in the database:

.. groonga-command
.. include:: ../../example/reference/commands/reindex/database.log
.. reindex

Here is an example to recreate all index columns
(``Lexicon.entry_key`` and ``Lexicon.entry_body``) in ``Lexicon``
table:

.. groonga-command
.. include:: ../../example/reference/commands/reindex/table.log
.. table_create Entry TABLE_HASH_KEY ShortText
.. column_create Entry body COLUMN_SCALAR Text
..
.. table_create Lexicon TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Lexicon entry_key COLUMN_INDEX|WITH_POSITION \
..   Entry _key
.. column_create Lexicon entry_body COLUMN_INDEX|WITH_POSITION \
..   Entry body
..
.. reindex Lexicon

Here is an example to recreate all index columns
(``BigramLexicon.site_title`` and ``RegexpLexicon.site_title``) of
``Site.title`` data column:

.. groonga-command
.. include:: ../../example/reference/commands/reindex/data_column.log
.. table_create Site TABLE_HASH_KEY ShortText
.. column_create Site title COLUMN_SCALAR ShortText
..
.. table_create BigramLexicon TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create BigramLexicon site_title COLUMN_INDEX|WITH_POSITION \
..   Site title
..
.. table_create RegexpLexicon TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenRegexp \
..   --normalizer NormalizerAuto
.. column_create RegexpLexicon site_title COLUMN_INDEX|WITH_POSITION \
..   Site title
..
.. reindex Site.title

Here is an example to recreate an index column (``Timestamp.index``):

.. groonga-command
.. include:: ../../example/reference/commands/reindex/index_column.log
.. table_create Logs TABLE_NO_KEY
.. column_create Logs timestamp COLUMN_SCALAR Time
..
.. table_create Timestamp TABLE_PAT_KEY Time
.. column_create Timestamp logs_timestamp COLUMN_INDEX  Logs timestamp
..
.. reindex Timestamp.logs_timestamp

Parameters
----------

This section describes all parameters.

``target_name``
"""""""""""""""

Specifies the name of table or column.

If you don't specify it, database is used for the target object.

The default is none. It means that the target object is database.

Return value
------------

``reindex`` command returns whether recreation is succeeded or not::

  [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on
  error.
