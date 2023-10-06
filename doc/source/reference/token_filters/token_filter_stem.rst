.. -*- rst -*-

.. groonga-command
.. database: token_filters_stem

.. _token-filter-stem:

``TokenFilterStem``
===================

Summary
-------

``TokenFilterStem`` stems tokenized token.

You need to install an additional package to using ``TokenFilterStem``. For more detail of how to installing an additional package, see :doc:`/install` .

Syntax
------

``TokenFilterStem`` has optional parameter::

  TokenFilterStem

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
.. table_create FrenchMemos TABLE_NO_KEY
.. column_create FrenchMemos content COLUMN_SCALAR ShortText
.. table_create FrenchTerms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters 'TokenFilterStem("algorithm", "french")'
.. column_create FrenchTerms french_memos_content \
..    COLUMN_INDEX|WITH_POSITION FrenchMemos content
.. load --table FrenchMemos
.. [
.. {"content": "maintenait"},
.. {"content": "maintenant"}
.. ]
.. select FrenchMemos --match_columns content --query "maintenir"

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There is a optional parameters ``algorithm``.

``algorithm``
"""""""""""""

Specify a steming algorithm.

Steming algorithm is extract the stem. It is prepared for each language.

You can extract the stem of each language by changing steming algorithm.
For example, if you want extract the stem of the French, you specify French to ``algorithm`` option.

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
