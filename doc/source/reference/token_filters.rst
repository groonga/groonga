.. -*- rst -*-

.. highlightlang:: none

Token filters
=============

Summary
-------

Groonga has token filter module that some processes tokenized token.

Token filter module can be added as a plugin.

You can customize tokenized token by registering your token filters plugins to Groonga.

A table can have zero or more token filters. You can attach token
filters to a table by :ref:`table-create-token-filters` option in
:doc:`/reference/commands/table_create`.

Here is an example ``table_create`` that uses ``TokenFilterStopWord``
token filter module:

.. groonga-command
.. database: token_filters_example
.. include:: ../example/reference/token_filters/example-table-create.log
.. plugin_register token_filters/stop_word
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters TokenFilterStopWord

Available token filters
-----------------------

Here is the list of available token filters:

* ``TokenFilterStopWord``
* ``TokenFilterStem``

.. _token-filter-stop-word:

``TokenFilterStopWord``
^^^^^^^^^^^^^^^^^^^^^^^

``TokenFilterStopWord`` removes stop words from tokenized token
in searching the documents.

``TokenFilterStopWord`` can specify stop word after adding the
documents because it removes token in searching the documents.

The stop word is specified ``is_stop_word`` column on lexicon table.

Here is an example that uses ``TokenFilterStopWord`` token filter:

.. groonga-command
.. database: token_filters_stop_word
.. include:: ../example/reference/token_filters/stop_word.log
.. plugin_register token_filters/stop_word
.. table_create Memos TABLE_NO_KEY
.. column_create Memos content COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters TokenFilterStopWord
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

``and`` token is marked as stop word in ``Terms`` table.

``"Hello"`` that doesn't have ``and`` in content is matched. Because
``and`` is a stop word and ``and`` is removed from query.

.. _token-filter-stem:

``TokenFilterStem``
^^^^^^^^^^^^^^^^^^^

``TokenFilterStem`` stems tokenized token.

Here is an example that uses ``TokenFilterStem`` token filter:

.. groonga-command
.. database: token_filters_stem
.. include:: ../example/reference/token_filters/stem.log
.. plugin_register token_filters/stem
.. table_create Memos TABLE_NO_KEY
.. column_create Memos content COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters TokenFilterStem
.. column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
.. load --table Memos
.. [
.. {"content": "I develop Groonga"},
.. {"content": "I'm developing Groonga"},
.. {"content": "I developed Groonga"}
.. ]
.. select Memos --match_columns content --query "develops"

All of ``develop``, ``developing``, ``developed`` and ``develops``
tokens are stemmed as ``develop``. So we can find ``develop``,
``developing`` and ``developed`` by ``develops`` query.

See also
--------

* :doc:`/reference/commands/table_create`
