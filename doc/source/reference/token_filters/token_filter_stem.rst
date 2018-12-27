.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: token_filters_example

``TokenFilterStem``
===================

Summary
-------

``TokenFilterStem`` stems tokenized token.

Syntax
------

``TokenFilterStopWord`` has optional parameter::

  TokenFilterStopStem

  TokenFilterStem("algorithm", "steming_algorithm")

Usage
-----

Here is an example that uses ``TokenFilterStem`` token filter:

.. groonga-command
.. database: token_filters_stem
.. include:: ../../example/reference/token_filters/stem.log
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

You can specify steming algorithm except English with ``algorithm`` option as below.

.. groonga-command
.. database: token_filters_stem
.. include:: ../../example/reference/token_filters/stem-algorithm-option.log
.. plugin_register token_filters/stem
.. table_create Memos TABLE_NO_KEY
.. column_create Memos content COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters 'TokenFilterStem("algorithm", "french")'
.. column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
.. load --table Memos
.. [
.. {"content": "maintenait"},
.. {"content": "maintenant"}
.. ]
.. select Memos --match_columns content --query "maintenir"

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There is a optional parameters ``algorithm``.

``algorithm``
"""""""""""""

Specify a steming algorithm.

Here are support steming algorithm::

  French
  Spanish
  Portuguese
  Italian
  Romanian
  German
  Dutch
  Swedish
  Norwegian
  Danish
  Russian
  Finnish
