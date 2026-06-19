.. -*- rst -*-

.. groonga-command

.. _token-filter-stop-word:

``TokenFilterStopWord``
=======================

Summary
-------

``TokenFilterStopWord`` removes stop words from tokenized token
in searching the documents.

``TokenFilterStopWord`` can specify stop word after adding the
documents because it removes token in searching the documents.

The stop word is specified ``is_stop_word`` column on lexicon table
when you don't specify ``column`` option.

Syntax
------

``TokenFilterStopWord`` has optional parameter::

  TokenFilterStopWord

  TokenFilterStopWord("column", "ignore")

Usage
-----

Here is an example that uses ``TokenFilterStopWord`` token filter:

.. groonga-command
.. database: token_filters_stop_word
.. include:: ../../example/reference/token_filters/stop_word.log
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

You can specify stop word in column except ``is_stop_columns`` by ``columns`` option as below.

.. groonga-command
.. database: token_filters_stop_word_options
.. include:: ../../example/reference/token_filters/stop-word-columns-option.log
.. plugin_register token_filters/stop_word
.. table_create Memos TABLE_NO_KEY
.. column_create Memos content COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters 'TokenFilterStopWord("column", "ignore")'
.. column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
.. column_create Terms ignore COLUMN_SCALAR Bool
.. load --table Terms
.. [
.. {"_key": "and", "ignore": true}
.. ]
.. load --table Memos
.. [
.. {"content": "Hello"},
.. {"content": "Hello and Good-bye"},
.. {"content": "Good-bye"}
.. ]
.. select Memos --match_columns content --query "Hello and"

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There is a optional parameters ``columns``.

``columns``
"""""""""""

Specify a column that specified a stop word.
