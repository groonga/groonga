.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: token_filters

Token filters
=============

Summary
-------

Groonga has token filter module that some processes tokenized token.

Token filter module can be added as a plugin.

You can customize tokenized token by registering your token filters plugins to Groonga.

A token filter module is attached to a table. A token filter can have zero or
N token filter module. You can attach a normalizer module to a table
``token_filters`` option in :doc:`/reference/commands/table_create`.

Here is an example ``table_create`` that uses ``TokenFilterStopWord``
token filter module:

.. groonga-command
.. include:: ../example/reference/token_filters/example-table-create.log
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto --token_filters TokenFilterStopWord

Available token filters
-----------------------

Here are the list of available token filters:

* ``TokenFilterStopWord``
* ``TokenFilterStem``

``TokenFilterStopWord``
^^^^^^^^^^^^^^^^^^^^^^^

``TokenFilterStopWord`` removes stop words from tokenized token
in searching the documents.

``TokenFilterStopWord`` can specify stop word after adding the
documents, because It removes token in searching the documents.

The stop word is specified ``is_stop_word`` column on lexicon table.

Here is an example that uses ``TokenFilterStopWord`` token filter:

.. groonga-command
.. include:: ../example/reference/token_filters/stop_word.log
.. register token_filters/stop_word
.. table_create Memos TABLE_NO_KEY
.. column_create Memos content COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText   --default_tokenizer TokenBigram   --normalizer NormalizerAuto   --token_filters TokenFilterStopWord
.. column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
.. column_create Terms is_stop_word COLUMN_SCALAR Bool
.. load --table Terms
.. [
.. {"_key": "and", "is_stop_word": true}
.. ]
.. load --table Memos
.. [
.. {"content": "Hello"},
.. {"content": "Hello and Good-bye"},
.. {"content": "Good-bye"}
.. ]
.. select Memos --match_columns content --query "Hello and"

``TokenFilterStem``
^^^^^^^^^^^^^^^^^^^

``TokenFilterStem`` stemming tokenized token.

Here is an example that uses ``TokenFilterStem`` token filter:

.. groonga-command
.. include:: ../example/reference/token_filters/stem.log
.. register token_filters/stop_word
.. register token_filters/stem
.. table_create Memos TABLE_NO_KEY
.. column_create Memos content COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText   --default_tokenizer TokenBigram   --normalizer NormalizerAuto   --token_filters TokenFilterStem
.. column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
.. load --table Memos
.. [
.. {"content": "I develop Groonga"},
.. {"content": "I'm developing Groonga"},
.. {"content": "I developed Groonga"}
.. ]
.. select Memos --match_columns content --query "develops"

See also
--------

* :doc:`/reference/commands/table_create`
